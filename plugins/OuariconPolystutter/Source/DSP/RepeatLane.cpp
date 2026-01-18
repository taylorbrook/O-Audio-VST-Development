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
    maxCaptureSamples = static_cast<int>(spec.sampleRate * 5.0);

    // v1.1.0: Allocate dedicated capture buffer for non-destructive repeat playback
    // This fixes the pitch shifting and audio corruption issues caused by using popSample()
    captureBuffer.setSize(2, maxCaptureSamples);
    captureBuffer.clear();
    captureWritePosition = 0;

    // Allocate freeze buffer (5 seconds max, 2 channels)
    freezeBuffer.setSize(2, maxCaptureSamples);
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
    // v1.1.0: Reset capture buffer instead of delay lines
    captureBuffer.clear();
    captureWritePosition = 0;

    isTriggered = false;
    currentRepeat = 0;
    playbackPosition = 0;
    fractionalPlaybackPosition = 0.0;
    samplesUntilNextRepeat = 0;
    currentGain = 1.0f;
    captureLength = 0;

    // v1.1.1: Reset global envelope state
    globalEnvelopeGain = 0.0f;
    fadeInSamplesRemaining = 0;
    fadeOutActive = false;
    fadeOutSamplesRemaining = 0;
    fadeOutStartGain = 0.0f;
}

void RepeatLane::processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!enabled)
        return;

    // v1.1.0: Capture incoming audio to dedicated capture buffer (circular)
    // This is non-destructive - we always have the last N samples available
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float leftSample = buffer.getSample(0, sample);
        float rightSample = buffer.getNumChannels() > 1 ? buffer.getSample(1, sample) : leftSample;

        // Write to circular capture buffer
        captureBuffer.setSample(0, captureWritePosition, leftSample);
        captureBuffer.setSample(1, captureWritePosition, rightSample);

        // Advance write position (circular)
        captureWritePosition = (captureWritePosition + 1) % maxCaptureSamples;
    }

    // v1.1.1: Handle fade-out when repeats are finished
    // Instead of abruptly clearing, we fade out over crossfadeSamples
    // v1.2.1 FIX: Also check that the current repeat has finished playing (timer expired)
    // This fixes clicks with reps=1 where fade-out started before the repeat played
    if (!isTriggered || (currentRepeat >= maxRepeats && samplesUntilNextRepeat <= 0))
    {
        // Check if we need to start or continue a fade-out
        if (globalEnvelopeGain > 0.0001f && !fadeOutActive)
        {
            // Start fade-out
            fadeOutActive = true;
            fadeOutSamplesRemaining = crossfadeSamples;
            fadeOutStartGain = globalEnvelopeGain;
        }

        if (fadeOutActive && fadeOutSamplesRemaining > 0)
        {
            // Continue fade-out: output fading silence
            for (int sample = 0; sample < numSamples && fadeOutSamplesRemaining > 0; ++sample)
            {
                // Calculate fade-out gain (linear ramp from fadeOutStartGain to 0)
                float fadeProgress = static_cast<float>(fadeOutSamplesRemaining) / static_cast<float>(crossfadeSamples);
                float fadeGain = fadeOutStartGain * fadeProgress;

                // Output silence with fade (the buffer already has input signal, we just fade it)
                buffer.setSample(0, sample, 0.0f);
                if (buffer.getNumChannels() > 1)
                    buffer.setSample(1, sample, 0.0f);

                fadeOutSamplesRemaining--;
                globalEnvelopeGain = fadeGain;
            }

            // Clear any remaining samples in the buffer after fade completes
            if (fadeOutSamplesRemaining <= 0)
            {
                fadeOutActive = false;
                globalEnvelopeGain = 0.0f;
            }
        }
        else
        {
            // Fade-out complete or not needed, clear buffer
            buffer.clear();
            globalEnvelopeGain = 0.0f;
        }
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

        // Read from capture buffer at current playback position
        float leftOut = 0.0f;
        float rightOut = 0.0f;

        // Use fractional position for pitch shifting (supports sub-sample interpolation)
        double effectivePosition = fractionalPlaybackPosition;

        // Handle pitch-shifted playback length
        // When pitch ratio != 1.0, the effective capture length changes
        double effectiveCaptureLength = static_cast<double>(captureLength) / pitchRatio;

        if (effectivePosition < effectiveCaptureLength)
        {
            // Determine which buffer to read from (freeze or live capture)
            if (freezeEnabled && freezeBufferReady)
            {
                // Read from frozen snapshot with linear interpolation
                double readPos = reverseEnabled
                    ? (captureLength - 1.0 - (effectivePosition * pitchRatio))
                    : (effectivePosition * pitchRatio);

                readPos = juce::jlimit(0.0, static_cast<double>(captureLength - 1), readPos);

                // Linear interpolation for smooth pitch shifting
                int pos0 = static_cast<int>(readPos);
                int pos1 = juce::jmin(pos0 + 1, captureLength - 1);
                float frac = static_cast<float>(readPos - pos0);

                float left0 = freezeBuffer.getSample(0, pos0);
                float left1 = freezeBuffer.getSample(0, pos1);
                float right0 = freezeBuffer.getSample(1, pos0);
                float right1 = freezeBuffer.getSample(1, pos1);

                leftOut = left0 + frac * (left1 - left0);
                rightOut = right0 + frac * (right1 - right0);
            }
            else
            {
                // v1.1.0: Read from capture buffer NON-DESTRUCTIVELY with interpolation
                // Calculate read position in the circular buffer
                // captureStartPosition points to the oldest sample we want to read
                double readOffset = reverseEnabled
                    ? (captureLength - 1.0 - (effectivePosition * pitchRatio))
                    : (effectivePosition * pitchRatio);

                readOffset = juce::jlimit(0.0, static_cast<double>(captureLength - 1), readOffset);

                // Convert to circular buffer position
                // captureStartPosition is where the capture began
                int basePos = (captureStartPosition + static_cast<int>(readOffset)) % maxCaptureSamples;
                int nextPos = (basePos + 1) % maxCaptureSamples;
                float frac = static_cast<float>(readOffset - static_cast<int>(readOffset));

                // Linear interpolation for smooth pitch shifting
                float left0 = captureBuffer.getSample(0, basePos);
                float left1 = captureBuffer.getSample(0, nextPos);
                float right0 = captureBuffer.getSample(1, basePos);
                float right1 = captureBuffer.getSample(1, nextPos);

                leftOut = left0 + frac * (left1 - left0);
                rightOut = right0 + frac * (right1 - right0);
            }

            // Apply crossfade at loop boundaries for click-free looping
            int intPosition = static_cast<int>(effectivePosition);
            int effectiveLength = static_cast<int>(effectiveCaptureLength);
            const int safeCrossfade = juce::jmin(crossfadeSamples, effectiveLength / 2);

            if (safeCrossfade > 0 && intPosition < safeCrossfade)
            {
                // Fade in at start
                float fadeInGain = getCrossfadeGain(intPosition, safeCrossfade, true);
                leftOut *= fadeInGain;
                rightOut *= fadeInGain;
            }
            else if (safeCrossfade > 0 && intPosition > effectiveLength - safeCrossfade - 1)
            {
                // Fade out at end
                int fadePos = intPosition - (effectiveLength - safeCrossfade);
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

            // Advance playback position (pitch ratio affects playback speed)
            fractionalPlaybackPosition += 1.0;
            playbackPosition = static_cast<int>(fractionalPlaybackPosition);
        }

        // v1.1.1: Apply global envelope for click-free stutter start/end
        // Handle fade-in during first crossfadeSamples after trigger
        if (fadeInSamplesRemaining > 0)
        {
            // Calculate fade-in progress (0.0 at start, 1.0 at end of fade)
            float fadeProgress = 1.0f - (static_cast<float>(fadeInSamplesRemaining) / static_cast<float>(crossfadeSamples));
            globalEnvelopeGain = fadeProgress;
            fadeInSamplesRemaining--;
        }
        else
        {
            // Fade-in complete, maintain full gain
            globalEnvelopeGain = 1.0f;
        }

        // Apply global envelope to output
        leftOut *= globalEnvelopeGain;
        rightOut *= globalEnvelopeGain;

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

    // v1.1.1: If we're currently fading out, cancel it
    fadeOutActive = false;
    fadeOutSamplesRemaining = 0;

    // v1.1.1: Start global fade-in for click-free onset
    // Only reset envelope if we're not already active (avoid clicks on retrigger)
    if (!isTriggered || currentRepeat >= maxRepeats)
    {
        globalEnvelopeGain = 0.0f;
        fadeInSamplesRemaining = crossfadeSamples;
    }

    // Start new repeat cycle
    isTriggered = true;
    currentRepeat = 0;
    playbackPosition = 0;
    fractionalPlaybackPosition = 0.0;
    currentGain = 1.0f;

    // Set capture length to subdivision length (clamped to buffer size)
    captureLength = juce::jmin(static_cast<int>(subdivisionSamples), maxCaptureSamples);

    // v1.1.0: Calculate start position in circular buffer
    // The capture starts at (writePosition - captureLength), wrapping around
    captureStartPosition = (captureWritePosition - captureLength + maxCaptureSamples) % maxCaptureSamples;

    // If freeze is enabled, copy capture buffer to freeze buffer (linearized)
    if (freezeEnabled)
    {
        // Copy from circular capture buffer to linear freeze buffer
        for (int i = 0; i < captureLength && i < freezeBuffer.getNumSamples(); ++i)
        {
            int srcPos = (captureStartPosition + i) % maxCaptureSamples;

            float leftSample = captureBuffer.getSample(0, srcPos);
            float rightSample = captureBuffer.getSample(1, srcPos);

            freezeBuffer.setSample(0, i, leftSample);
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
    // Reset playback position (integer and fractional)
    playbackPosition = 0;
    fractionalPlaybackPosition = 0.0;

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

void RepeatLane::setPitch(float semitones)
{
    // Store semitones and calculate pitch ratio
    pitchSemitones = juce::jlimit(-12.0f, 12.0f, semitones);

    // Pitch ratio = 2^(semitones/12)
    // Positive semitones = higher pitch = faster playback
    // Negative semitones = lower pitch = slower playback
    pitchRatio = std::pow(2.0f, pitchSemitones / 12.0f);
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
