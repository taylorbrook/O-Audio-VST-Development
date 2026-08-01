/*
   This file is part of O-Polystutter, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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

    // v1.12.3 (WR-05): Snapshot buffer holds the frozen slice for playback
    snapshotBuffer.setSize(2, maxCaptureSamples);
    snapshotBuffer.clear();

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

    // v1.12.1: Reset retrigger crossfade state
    retriggerCrossfadeActive = false;
    retriggerCrossfadeSamplesRemaining = 0;
    lastOutputLeft = 0.0f;
    lastOutputRight = 0.0f;
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
            // v1.12.1: Fade from last output level to silence (was writing zeros)
            int sample = 0;
            for (; sample < numSamples && fadeOutSamplesRemaining > 0; ++sample)
            {
                float fadeProgress = static_cast<float>(fadeOutSamplesRemaining) / static_cast<float>(crossfadeSamples);
                float fadeGain = fadeOutStartGain * fadeProgress;

                buffer.setSample(0, sample, lastOutputLeft * fadeGain);
                if (buffer.getNumChannels() > 1)
                    buffer.setSample(1, sample, lastOutputRight * fadeGain);

                fadeOutSamplesRemaining--;
                globalEnvelopeGain = fadeGain;
            }

            // Clear any remaining samples after fade completes
            for (; sample < numSamples; ++sample)
            {
                buffer.setSample(0, sample, 0.0f);
                if (buffer.getNumChannels() > 1)
                    buffer.setSample(1, sample, 0.0f);
            }

            if (fadeOutSamplesRemaining <= 0)
            {
                fadeOutActive = false;
                globalEnvelopeGain = 0.0f;
                lastOutputLeft = 0.0f;
                lastOutputRight = 0.0f;
            }
        }
        else
        {
            // Fade-out complete or not needed, clear buffer
            buffer.clear();
            globalEnvelopeGain = 0.0f;
            lastOutputLeft = 0.0f;
            lastOutputRight = 0.0f;
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
        // Prevent division by zero for extreme pitch values
        double safePitchRatio = pitchRatio < 0.001 ? 0.001 : pitchRatio;
        double effectiveCaptureLength = static_cast<double>(captureLength) / safePitchRatio;

        if (effectivePosition < effectiveCaptureLength)
        {
            // v1.12.3 (WR-05): Read from the trigger-time snapshot, not the
            // circular capture buffer — the live write head keeps advancing
            // and would corrupt long repeat tails. Snapshot index 0 is the
            // start of the captured slice, so no circular math is needed.
            double readOffset = reverseEnabled
                ? (captureLength - 1.0 - (effectivePosition * pitchRatio))
                : (effectivePosition * pitchRatio);

            readOffset = juce::jlimit(0.0, static_cast<double>(captureLength - 1), readOffset);

            // Clamp the interpolation neighbor inside the slice (also fixes
            // the old wrap into one sample of unrelated audio at the loop end)
            int basePos = static_cast<int>(readOffset);
            int nextPos = juce::jmin(basePos + 1, captureLength - 1);
            float frac = static_cast<float>(readOffset - basePos);

            // Linear interpolation for smooth pitch shifting
            float left0 = snapshotBuffer.getSample(0, basePos);
            float left1 = snapshotBuffer.getSample(0, nextPos);
            float right0 = snapshotBuffer.getSample(1, basePos);
            float right1 = snapshotBuffer.getSample(1, nextPos);

            leftOut = left0 + frac * (left1 - left0);
            rightOut = right0 + frac * (right1 - right0);

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

        // v1.12.1: Retrigger crossfade - blend from old output to new
        if (retriggerCrossfadeActive && retriggerCrossfadeSamplesRemaining > 0)
        {
            float t = 1.0f - (static_cast<float>(retriggerCrossfadeSamplesRemaining) / static_cast<float>(crossfadeSamples));
            leftOut = lastOutputLeft * (1.0f - t) + leftOut * t;
            rightOut = lastOutputRight * (1.0f - t) + rightOut * t;
            retriggerCrossfadeSamplesRemaining--;
            if (retriggerCrossfadeSamplesRemaining <= 0)
                retriggerCrossfadeActive = false;
        }

        // Track last output for retrigger crossfade and fade-out
        lastOutputLeft = leftOut;
        lastOutputRight = rightOut;

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

    // v1.12.1: If mid-repeat, enable retrigger crossfade for click-free transition
    if (isTriggered && currentRepeat < maxRepeats)
    {
        retriggerCrossfadeActive = true;
        retriggerCrossfadeSamplesRemaining = crossfadeSamples;
    }

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

    // v1.12.3 (WR-05): Snapshot the captured slice so the live write head
    // can't overwrite it mid-tail (repeats × subdivision can exceed the 5s
    // circular buffer). Bounded copy of pre-allocated memory — RT-safe.
    if (captureLength > 0)
    {
        const int firstLen = juce::jmin(captureLength, maxCaptureSamples - captureStartPosition);
        for (int ch = 0; ch < 2; ++ch)
        {
            snapshotBuffer.copyFrom(ch, 0, captureBuffer, ch, captureStartPosition, firstLen);
            if (firstLen < captureLength)
                snapshotBuffer.copyFrom(ch, firstLen, captureBuffer, ch, 0, captureLength - firstLen);
        }
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

    // v1.7.0: Generate new random pitch offset for this repeat
    if (pitchRandEnabled)
    {
        // Generate random value in [min, max] range
        float range = pitchRandMax - pitchRandMin;
        currentRandomPitch = pitchRandMin + randomGenerator.nextFloat() * range;

        // Quantize to semitones if enabled
        if (pitchRandQuantize)
        {
            currentRandomPitch = std::round(currentRandomPitch);
        }

        // Calculate effective pitch ratio (base pitch + random offset)
        float effectivePitch = pitchSemitones + currentRandomPitch;
        effectivePitch = juce::jlimit(-24.0f, 24.0f, effectivePitch); // Safety clamp
        pitchRatio = std::pow(2.0f, effectivePitch / 12.0f);
    }
    else
    {
        // No randomization, use base pitch
        currentRandomPitch = 0.0f;
        pitchRatio = std::pow(2.0f, pitchSemitones / 12.0f);
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
    // Store semitones for use in pitch calculation
    pitchSemitones = juce::jlimit(-12.0f, 12.0f, semitones);

    // v1.7.0 FIX: Only recalculate pitchRatio here when randomization is DISABLED.
    // When enabled, pitchRatio is calculated in startNewRepeat() with random offset.
    // This prevents overwriting the randomized pitch on every processBlock call.
    if (!pitchRandEnabled)
    {
        // Pitch ratio = 2^(semitones/12)
        // Positive semitones = higher pitch = faster playback
        // Negative semitones = lower pitch = slower playback
        pitchRatio = std::pow(2.0f, pitchSemitones / 12.0f);
    }
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

    // v1.12.1: Wrap at euclideanSteps when Euclidean mode is active,
    // otherwise wrap at 16. This ensures Euclidean patterns cycle correctly
    // (e.g., E(3,8) cycles every 8 steps, not every 16).
    int wrapCount = (euclideanEnabled && euclideanSteps > 0) ? euclideanSteps : 16;

    // Calculate current step from PPQ position
    int newStep = static_cast<int>((ppqPosition - patternStartPPQ) / subdivisionPPQ) % wrapCount;

    // Handle negative modulo (wrap around)
    if (newStep < 0)
        newStep += wrapCount;

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

void RepeatLane::setManualTimeEnabled(bool shouldEnable)
{
    manualTimeEnabled = shouldEnable;
}

// v1.5.0: Progress reporting for UI progress bars
float RepeatLane::getProgress() const
{
    // Return 0 if not actively repeating or no valid capture
    if (!isTriggered || captureLength == 0)
        return 0.0f;

    // Account for pitch ratio affecting effective capture length
    // Higher pitch = faster playback = shorter effective length
    // Prevent division by zero for extreme pitch values
    double safePitchRatio = pitchRatio < 0.001 ? 0.001 : pitchRatio;
    double effectiveCaptureLength = static_cast<double>(captureLength) / safePitchRatio;

    if (effectiveCaptureLength <= 0.0)
        return 0.0f;

    // Calculate progress as position within the effective capture length
    double progress = fractionalPlaybackPosition / effectiveCaptureLength;

    // Clamp to 0.0-1.0 range
    return static_cast<float>(juce::jlimit(0.0, 1.0, progress));
}

// v1.7.0: Pitch randomization setters
void RepeatLane::setPitchRandEnabled(bool shouldEnable)
{
    pitchRandEnabled = shouldEnable;

    // If disabling, reset pitch ratio to base pitch
    if (!shouldEnable)
    {
        currentRandomPitch = 0.0f;
        pitchRatio = std::pow(2.0f, pitchSemitones / 12.0f);
    }
}

void RepeatLane::setPitchRandMin(float minSemitones)
{
    pitchRandMin = juce::jlimit(-12.0f, 12.0f, minSemitones);
}

void RepeatLane::setPitchRandMax(float maxSemitones)
{
    pitchRandMax = juce::jlimit(-12.0f, 12.0f, maxSemitones);
}

void RepeatLane::setPitchRandQuantize(bool shouldQuantize)
{
    pitchRandQuantize = shouldQuantize;
}

// v1.9.0: Euclidean rhythm generator
void RepeatLane::setEuclideanEnabled(bool shouldEnable)
{
    if (euclideanEnabled != shouldEnable)
    {
        euclideanEnabled = shouldEnable;
        if (euclideanEnabled)
            regenerateEuclideanPattern();
    }
}

void RepeatLane::setEuclideanPulses(int numPulses)
{
    int clamped = juce::jlimit(1, 16, numPulses);
    if (euclideanPulses != clamped)
    {
        euclideanPulses = clamped;
        if (euclideanEnabled)
            regenerateEuclideanPattern();
    }
}

void RepeatLane::setEuclideanSteps(int numSteps)
{
    int clamped = juce::jlimit(2, 16, numSteps);
    if (euclideanSteps != clamped)
    {
        euclideanSteps = clamped;
        if (euclideanEnabled)
            regenerateEuclideanPattern();
    }
}

void RepeatLane::regenerateEuclideanPattern()
{
    // Clamp pulses to not exceed steps
    int effectivePulses = juce::jmin(euclideanPulses, euclideanSteps);
    generateEuclideanPattern(effectivePulses, euclideanSteps, patternSteps);
}

void RepeatLane::generateEuclideanPattern(int pulses, int steps, bool result[16])
{
    // Initialize all steps to false
    for (int i = 0; i < 16; ++i)
        result[i] = false;

    if (steps <= 0 || pulses <= 0)
        return;

    if (pulses >= steps)
    {
        for (int i = 0; i < steps && i < 16; ++i)
            result[i] = true;
        return;
    }

    // Bjorklund's algorithm (proper implementation)
    // Produces canonical Euclidean rhythms:
    //   E(3,8) = [1,0,0,1,0,0,1,0] (Cuban tresillo)
    //   E(5,8) = [1,0,1,1,0,1,1,0] (Cuban cinquillo)
    //   E(5,16) = [1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,0] (Bossa nova)

    // Work with sequences of sub-patterns
    // Start: 'pulses' copies of [1] and '(steps-pulses)' copies of [0]
    // Repeatedly distribute the smaller group across the larger, like Euclidean GCD

    // Use flat arrays to avoid allocations (max 16 steps)
    // Each "sequence" is stored as a list of bool values with lengths tracked
    bool sequences[16][16];  // sequences[i] is the i-th sub-pattern
    int seqLengths[16];      // length of each sub-pattern
    int numA = pulses;       // count of "A" group sequences
    int numB = steps - pulses; // count of "B" group sequences

    // Initialize: A sequences are [1], B sequences are [0]
    for (int i = 0; i < numA; ++i)
    {
        sequences[i][0] = true;
        seqLengths[i] = 1;
    }
    for (int i = 0; i < numB; ++i)
    {
        sequences[numA + i][0] = false;
        seqLengths[numA + i] = 1;
    }

    int totalSeqs = numA + numB;

    // Iterate until B group has 0 or 1 elements
    while (numB > 1)
    {
        int pairs = juce::jmin(numA, numB);

        // Append each B sequence to corresponding A sequence
        for (int i = 0; i < pairs; ++i)
        {
            int aIdx = i;
            int bIdx = numA + i;
            int aLen = seqLengths[aIdx];
            int bLen = seqLengths[bIdx];

            // Append B to A
            for (int j = 0; j < bLen; ++j)
                sequences[aIdx][aLen + j] = sequences[bIdx][j];
            seqLengths[aIdx] = aLen + bLen;
        }

        // Unpaired sequences of EITHER group become the new remainder group.
        // When numB > numA the leftover B's at [numA + pairs, numA + numB)
        // must move down to [pairs, pairs + remaining). When numA > numB the
        // leftover A's are already contiguous at [pairs, numA) and only need
        // counting — the old code counted only leftover B's, silently dropping
        // leftover A's (and their pulses) for E(3,8), E(5,8), E(7,16), etc.
        int remaining = juce::jmax(numA, numB) - pairs;
        if (numB > numA)
        {
            for (int i = 0; i < remaining; ++i)
            {
                int srcIdx = numA + pairs + i;
                int dstIdx = pairs + i;
                if (srcIdx != dstIdx)
                {
                    for (int j = 0; j < seqLengths[srcIdx]; ++j)
                        sequences[dstIdx][j] = sequences[srcIdx][j];
                    seqLengths[dstIdx] = seqLengths[srcIdx];
                }
            }
        }

        totalSeqs = pairs + remaining;
        numA = pairs;
        numB = remaining;
    }

    // Flatten all sequences into result
    int pos = 0;
    for (int i = 0; i < totalSeqs && pos < 16; ++i)
    {
        for (int j = 0; j < seqLengths[i] && pos < 16; ++j)
        {
            result[pos++] = sequences[i][j];
        }
    }
}
