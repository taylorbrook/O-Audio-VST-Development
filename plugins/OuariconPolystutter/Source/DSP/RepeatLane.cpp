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

    // Max buffer size: 2 seconds at max sample rate (192kHz) = 384,000 samples
    const int maxDelaySamples = static_cast<int>(spec.sampleRate * 2.0);

    // Prepare delay lines
    delayLineLeft.prepare(spec);
    delayLineRight.prepare(spec);

    delayLineLeft.setMaximumDelayInSamples(maxDelaySamples);
    delayLineRight.setMaximumDelayInSamples(maxDelaySamples);

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
            // Calculate delay time (how far back to read)
            float delayTime = static_cast<float>(captureLength - playbackPosition);

            // Read from delay lines
            leftOut = delayLineLeft.popSample(0, delayTime);
            rightOut = delayLineRight.popSample(0, delayTime);

            // Apply crossfade at loop boundaries for click-free looping
            if (playbackPosition < crossfadeSamples)
            {
                // Fade in at start
                float fadeInGain = getCrossfadeGain(playbackPosition, crossfadeSamples, true);
                leftOut *= fadeInGain;
                rightOut *= fadeInGain;
            }
            else if (playbackPosition >= captureLength - crossfadeSamples)
            {
                // Fade out at end
                int fadePos = playbackPosition - (captureLength - crossfadeSamples);
                float fadeOutGain = getCrossfadeGain(fadePos, crossfadeSamples, false);
                leftOut *= fadeOutGain;
                rightOut *= fadeOutGain;
            }

            // Apply gain (decay per repeat)
            leftOut *= currentGain * volumeLevel;
            rightOut *= currentGain * volumeLevel;

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

    // Start new repeat cycle
    isTriggered = true;
    currentRepeat = 0;
    playbackPosition = 0;
    currentGain = 1.0f;

    // Set capture length to subdivision length
    captureLength = static_cast<int>(subdivisionSamples);

    // Start first repeat immediately
    samplesUntilNextRepeat = 0;
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
    currentBPM = bpm;
    currentSampleRate = sampleRate;
    subdivisionSamples = calculateSubdivisionSamples(subdivisionIndex, bpm, sampleRate);
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

    // Set timer for next repeat
    samplesUntilNextRepeat = static_cast<int>(subdivisionSamples);

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
