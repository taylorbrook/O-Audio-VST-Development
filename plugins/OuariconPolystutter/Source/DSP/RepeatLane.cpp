/*
  ==============================================================================

    RepeatLane.cpp
    Ouaricon Polystutter - Single-Lane Beat Repeater Implementation
    Phase 2.1: Core single-lane repeat engine with beat sync

  ==============================================================================
*/

#include "RepeatLane.h"

RepeatLane::RepeatLane()
{
}

void RepeatLane::prepare(const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;

    // Max buffer size: 5 seconds per architecture spec (supports long captures at any sample rate)
    // At 192kHz = 960,000 samples per channel
    const int maxDelaySamples = static_cast<int>(spec.sampleRate * 5.0);

    // Prepare delay lines
    delayLineLeft.prepare(spec);
    delayLineRight.prepare(spec);

    delayLineLeft.setMaximumDelayInSamples(maxDelaySamples);
    delayLineRight.setMaximumDelayInSamples(maxDelaySamples);

    // Allocate freeze buffer (5 seconds max, 2 channels)
    freezeBuffer.setSize(2, maxDelaySamples);
    freezeBuffer.clear();
    freezeBufferReady = false;

    // Calculate crossfade samples (5ms for click-free looping)
    crossfadeSamples = static_cast<int>(spec.sampleRate * 0.005);

    // Initialize timing
    updateTempo(currentBPM, currentSampleRate);

    reset();
}

void RepeatLane::reset()
{
    delayLineLeft.reset();
    delayLineRight.reset();

    isTriggered = false;
    currentRepeat = 0;
    playbackPosition = 0;
    samplesUntilNextRepeat = 0;
    currentGain = 1.0f;
    captureLength = 0;
}

void RepeatLane::processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!enabled)
        return;

    // Capture incoming audio to delay buffer (always capture, even when not repeating)
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Stereo capture
        float leftSample = buffer.getSample(0, sample);
        float rightSample = buffer.getNumChannels() > 1 ? buffer.getSample(1, sample) : leftSample;

        // Push samples into delay lines
        delayLineLeft.pushSample(0, leftSample);
        delayLineRight.pushSample(0, rightSample);
    }

    // If not triggered or all repeats finished, output silence (dry signal handled by processor)
    if (!isTriggered || currentRepeat >= maxRepeats)
    {
        buffer.clear();
        return;
    }

    // Process repeats
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Check if we need to start a new repeat
        if (samplesUntilNextRepeat <= 0 && currentRepeat < maxRepeats)
        {
            startNewRepeat();
        }

        // Read from delay buffer at current playback position
        float leftOut = 0.0f;
        float rightOut = 0.0f;

        if (playbackPosition < captureLength)
        {
            // Determine which buffer to read from (freeze or live capture)
            if (freezeEnabled && freezeBufferReady)
            {
                // Read from frozen snapshot
                // Handle reverse playback
                int readPosition = reverseEnabled ? (captureLength - 1 - playbackPosition) : playbackPosition;
                readPosition = juce::jlimit(0, captureLength - 1, readPosition);

                if (readPosition < freezeBuffer.getNumSamples())
                {
                    leftOut = freezeBuffer.getSample(0, readPosition);
                    rightOut = freezeBuffer.getNumChannels() > 1 ?
                               freezeBuffer.getSample(1, readPosition) : leftOut;
                }
            }
            else
            {
                // Read from live delay lines
                // Forward: oldest to newest (high delay to low delay)
                // Reverse: newest to oldest (low delay to high delay, but offset by 1 to avoid write position)
                int readPosition = reverseEnabled
                    ? (playbackPosition + 1)  // Start at delay=1 (avoid write position at 0)
                    : (captureLength - playbackPosition);  // Start at oldest sample
                float delayTime = static_cast<float>(readPosition);

                leftOut = delayLineLeft.popSample(0, delayTime);
                rightOut = delayLineRight.popSample(0, delayTime);
            }

            // Apply crossfade at loop boundaries for click-free looping
            // Guard against edge case where captureLength < 2*crossfadeSamples
            const int safeCrossfade = juce::jmin(crossfadeSamples, captureLength / 2);

            if (safeCrossfade > 0 && playbackPosition < safeCrossfade)
            {
                // Fade in at start
                float fadeInGain = getCrossfadeGain(playbackPosition, safeCrossfade, true);
                leftOut *= fadeInGain;
                rightOut *= fadeInGain;
            }
            else if (safeCrossfade > 0 && playbackPosition > captureLength - safeCrossfade - 1)
            {
                // Fade out at end (fixed off-by-one: use > instead of >=)
                int fadePos = playbackPosition - (captureLength - safeCrossfade);
                float fadeOutGain = getCrossfadeGain(fadePos, safeCrossfade, false);
                leftOut *= fadeOutGain;
                rightOut *= fadeOutGain;
            }

            // Apply gain (decay per repeat)
            leftOut *= currentGain * volumeLevel;
            rightOut *= currentGain * volumeLevel;

            // Calculate pan position (with ping-pong if enabled)
            float effectivePan = panPosition;
            if (pingPongEnabled)
            {
                // Alternate left/right based on repeat number (even = -0.5, odd = +0.5)
                float pingPongOffset = (currentRepeat % 2 == 0) ? -0.5f : 0.5f;
                effectivePan = juce::jlimit(-1.0f, 1.0f, panPosition + pingPongOffset);
            }

            // Apply constant-power pan law
            float leftGain = std::cos((effectivePan + 1.0f) * juce::MathConstants<float>::pi / 4.0f);
            float rightGain = std::sin((effectivePan + 1.0f) * juce::MathConstants<float>::pi / 4.0f);

            leftOut *= leftGain;
            rightOut *= rightGain;

            playbackPosition++;
        }

        // Write to output buffer
        buffer.setSample(0, sample, leftOut);
        if (buffer.getNumChannels() > 1)
            buffer.setSample(1, sample, rightOut);

        // Decrement repeat timer
        samplesUntilNextRepeat--;
    }
}

void RepeatLane::trigger()
{
    if (!enabled)
        return;

    // Check pattern sequencer gate
    if (!patternSteps[currentPatternStep])
        return;

    // Check probability gate (use member random to avoid lock in audio thread)
    if (randomGenerator.nextFloat() >= probabilityAmount)
        return;

    // Start new repeat cycle
    isTriggered = true;
    currentRepeat = 0;
    playbackPosition = 0;
    currentGain = 1.0f;

    // Set capture length to subdivision length
    captureLength = static_cast<int>(subdivisionSamples);

    // If freeze is enabled, copy delay line to freeze buffer
    if (freezeEnabled)
    {
        // Copy current delay line contents to freeze buffer
        // We need to extract samples from the delay line
        for (int i = 0; i < captureLength && i < freezeBuffer.getNumSamples(); ++i)
        {
            float delayTime = static_cast<float>(captureLength - i);
            float leftSample = delayLineLeft.popSample(0, delayTime);
            float rightSample = delayLineRight.popSample(0, delayTime);

            freezeBuffer.setSample(0, i, leftSample);
            if (freezeBuffer.getNumChannels() > 1)
                freezeBuffer.setSample(1, i, rightSample);
        }
        freezeBufferReady = true;
    }

    // Start first repeat immediately (with swing offset if applicable)
    int swingOffset = calculateSwingOffset(0);
    samplesUntilNextRepeat = swingOffset;
}

void RepeatLane::setEnabled(bool shouldBeEnabled)
{
    enabled = shouldBeEnabled;
    if (!enabled)
    {
        isTriggered = false;
        currentRepeat = 0;
    }
}

void RepeatLane::setSubdivision(int subdivIndex)
{
    subdivisionIndex = juce::jlimit(0, 5, subdivIndex);
    updateTempo(currentBPM, currentSampleRate);
}

void RepeatLane::setRepeats(int numRepeats)
{
    maxRepeats = juce::jlimit(1, 16, numRepeats);
}

void RepeatLane::setDecay(float decayPercent)
{
    decayAmount = juce::jlimit(0.0f, 100.0f, decayPercent) / 100.0f;
}

void RepeatLane::setVolume(float volumePercent)
{
    volumeLevel = juce::jlimit(0.0f, 100.0f, volumePercent) / 100.0f;
}

void RepeatLane::updateTempo(double bpm, double sampleRate)
{
    // Validate BPM to prevent division by zero (20-999 BPM is reasonable range)
    currentBPM = juce::jlimit(20.0, 999.0, bpm);
    currentSampleRate = juce::jmax(1.0, sampleRate);  // Prevent divide by zero
    subdivisionSamples = calculateSubdivisionSamples(subdivisionIndex, currentBPM, currentSampleRate);
}

double RepeatLane::calculateSubdivisionSamples(int subdivIndex, double bpm, double sampleRate)
{
    // Calculate quarter note duration in samples
    double quarterNoteSamples = (60.0 / bpm) * sampleRate;

    // Calculate subdivision based on index
    switch (subdivIndex)
    {
        case 0: return quarterNoteSamples;        // 1/4
        case 1: return quarterNoteSamples / 2.0;  // 1/8
        case 2: return quarterNoteSamples / 4.0;  // 1/16
        case 3: return quarterNoteSamples / 8.0;  // 1/32
        case 4: return quarterNoteSamples / 3.0;  // 1/8T (triplet)
        case 5: return quarterNoteSamples / 6.0;  // 1/16T (triplet)
        default: return quarterNoteSamples / 4.0; // Default to 1/16
    }
}

void RepeatLane::startNewRepeat()
{
    // Reset playback position
    playbackPosition = 0;

    // Apply decay to gain
    if (currentRepeat > 0)
    {
        currentGain *= decayAmount;
    }

    // Set timer for next repeat (with swing offset)
    int swingOffset = calculateSwingOffset(currentRepeat);
    samplesUntilNextRepeat = static_cast<int>(subdivisionSamples) + swingOffset;

    // Increment repeat counter
    currentRepeat++;
}

float RepeatLane::getCrossfadeGain(int sampleIndex, int fadeLength, bool fadeIn)
{
    if (fadeLength <= 0)
        return 1.0f;

    float ratio = static_cast<float>(sampleIndex) / static_cast<float>(fadeLength);
    ratio = juce::jlimit(0.0f, 1.0f, ratio);

    // Linear crossfade
    return fadeIn ? ratio : (1.0f - ratio);
}

void RepeatLane::setPan(float panValue)
{
    // Convert -100 to +100 range to -1.0 to +1.0
    panPosition = juce::jlimit(-1.0f, 1.0f, panValue / 100.0f);
}

void RepeatLane::setProbability(float probPercent)
{
    // Convert 0-100% to 0.0-1.0
    probabilityAmount = juce::jlimit(0.0f, 1.0f, probPercent / 100.0f);
}

void RepeatLane::setSwing(float swingPercent)
{
    // Convert 0-100% to 0.0-1.0
    swingAmount = juce::jlimit(0.0f, 1.0f, swingPercent / 100.0f);
}

void RepeatLane::setPatternStep(int stepIndex, bool stepEnabled)
{
    if (stepIndex >= 0 && stepIndex < 16)
    {
        patternSteps[stepIndex] = stepEnabled;
    }
}

void RepeatLane::updatePatternPosition(double ppqPosition, int subdivIndex)
{
    // Calculate subdivision in PPQ units
    double subdivisionPPQ = 0.0;
    switch (subdivIndex)
    {
        case 0: subdivisionPPQ = 1.0;       // 1/4 note
            break;
        case 1: subdivisionPPQ = 0.5;       // 1/8 note
            break;
        case 2: subdivisionPPQ = 0.25;      // 1/16 note
            break;
        case 3: subdivisionPPQ = 0.125;     // 1/32 note
            break;
        case 4: subdivisionPPQ = 1.0 / 3.0; // 1/8T (triplet)
            break;
        case 5: subdivisionPPQ = 1.0 / 6.0; // 1/16T (triplet)
            break;
        default: subdivisionPPQ = 0.25;     // Default 1/16
            break;
    }

    // Calculate current step from PPQ position
    int newStep = static_cast<int>((ppqPosition - patternStartPPQ) / subdivisionPPQ) % 16;

    // Handle negative modulo (wrap around)
    if (newStep < 0)
        newStep += 16;

    // Update current step
    currentPatternStep = newStep;
    lastPPQPosition = ppqPosition;
}

int RepeatLane::calculateSwingOffset(int repeatNumber) const
{
    // Swing only applies to even-numbered repeats (0, 2, 4, 6...)
    if (repeatNumber % 2 != 0)
        return 0;

    // Swing ratio: 0% = 0.5 (straight), 100% = 0.66 (triplet feel)
    double swingRatio = 0.5 + (swingAmount * 0.16);

    // Calculate offset in samples
    double offset = subdivisionSamples * (swingRatio - 0.5);

    return static_cast<int>(offset);
}

// Phase 2.3: Advanced mode setters
void RepeatLane::setPingPong(bool shouldEnable)
{
    pingPongEnabled = shouldEnable;
}

void RepeatLane::setReverse(bool shouldEnable)
{
    reverseEnabled = shouldEnable;
}

void RepeatLane::setFreeze(bool shouldEnable)
{
    freezeEnabled = shouldEnable;

    // Clear freeze buffer when disabled
    if (!shouldEnable)
    {
        freezeBufferReady = false;
    }
}

void RepeatLane::setManualTimeEnabled(bool shouldEnable)
{
    manualTimeEnabled = shouldEnable;
}
