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

    // v1.1.12: Calculate crossfade samples (10ms for click-free looping)
    // 10ms covers 2+ cycles at 200Hz, providing smoother transitions for low-frequency content
    crossfadeSamples = static_cast<int>(spec.sampleRate * 0.010);

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

    // v1.1.5: Reset deferred capture flag
    pendingCapture = false;

    // v1.1.6: Reset retrigger crossfade state
    retriggerCrossfadeActive = false;
    retriggerCrossfadeSamplesRemaining = 0;
    oldCaptureStartPosition = 0;
    oldFractionalPlaybackPosition = 0.0;
    oldCurrentGain = 1.0f;

    // v1.1.14: Reset loop boundary crossfade state
    loopCrossfadeActive = false;
    loopCrossfadeSamplesRemaining = 0;
    loopOldPlaybackPosition = 0.0;
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

    // v1.1.5: Handle deferred capture - calculate position AFTER writing current block
    // This fixes ENV trigger artifacts by ensuring the transient is included in capture
    if (pendingCapture)
    {
        pendingCapture = false;

        // Now captureWritePosition points to END of current block
        // Capture the last subdivisionSamples, which now includes the transient
        captureStartPosition = (captureWritePosition - captureLength + maxCaptureSamples) % maxCaptureSamples;

        // If freeze is enabled, copy capture buffer to freeze buffer (linearized)
        if (freezeEnabled)
        {
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
    }

    // v1.1.1: Handle fade-out when repeats are finished
    // Instead of abruptly clearing, we fade out over crossfadeSamples
    if (!isTriggered || currentRepeat >= maxRepeats)
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

            // v1.1.6: Handle retrigger crossfade blending
            // When retriggering during active playback, blend old audio with new
            if (retriggerCrossfadeActive && retriggerCrossfadeSamplesRemaining > 0)
            {
                // Calculate crossfade progress (0.0 = all old, 1.0 = all new)
                float crossfadeProgress = 1.0f - (static_cast<float>(retriggerCrossfadeSamplesRemaining) / static_cast<float>(crossfadeSamples));

                // v1.1.12: Use equal-power weights for the blend (matches getCrossfadeGain)
                float newWeight = std::sin(crossfadeProgress * juce::MathConstants<float>::halfPi);
                float oldWeight = std::cos(crossfadeProgress * juce::MathConstants<float>::halfPi);

                double oldEffectiveLength = static_cast<double>(captureLength) / pitchRatio;

                float oldLeftOut = 0.0f;
                float oldRightOut = 0.0f;

                // v1.1.12: Only read old audio if position is still valid
                // When exhausted, old contribution naturally fades to zero via crossfade
                // (Previous clamping caused DC-like artifacts from repeating last sample)
                if (oldFractionalPlaybackPosition < oldEffectiveLength)
                {
                    // Read from old capture position with linear interpolation
                    double oldReadOffset = reverseEnabled
                        ? (captureLength - 1.0 - (oldFractionalPlaybackPosition * pitchRatio))
                        : (oldFractionalPlaybackPosition * pitchRatio);

                    oldReadOffset = juce::jlimit(0.0, static_cast<double>(captureLength - 1), oldReadOffset);

                    int oldBasePos = (oldCaptureStartPosition + static_cast<int>(oldReadOffset)) % maxCaptureSamples;
                    int oldNextPos = (oldBasePos + 1) % maxCaptureSamples;
                    float oldFrac = static_cast<float>(oldReadOffset - static_cast<int>(oldReadOffset));

                    float oldLeft0 = captureBuffer.getSample(0, oldBasePos);
                    float oldLeft1 = captureBuffer.getSample(0, oldNextPos);
                    float oldRight0 = captureBuffer.getSample(1, oldBasePos);
                    float oldRight1 = captureBuffer.getSample(1, oldNextPos);

                    oldLeftOut = oldLeft0 + oldFrac * (oldLeft1 - oldLeft0);
                    oldRightOut = oldRight0 + oldFrac * (oldRight1 - oldRight0);

                    // Advance old playback position
                    oldFractionalPlaybackPosition += 1.0;
                }
                // else: oldLeftOut/oldRightOut remain 0, contribution fades via equal-power weights

                // Blend old and new audio with equal-power weights
                leftOut = oldLeftOut * oldWeight + leftOut * newWeight;
                rightOut = oldRightOut * oldWeight + rightOut * newWeight;

                // Decrement crossfade counter
                retriggerCrossfadeSamplesRemaining--;

                if (retriggerCrossfadeSamplesRemaining <= 0)
                {
                    retriggerCrossfadeActive = false;
                }
            }

            // v1.1.14: Loop boundary overlap-add crossfade
            // Detect when we're approaching the end of capture and blend with loop start
            // This starts the crossfade BEFORE we hit the boundary (not after)
            // Skip if retrigger crossfade is active (to avoid double-crossfading)
            // Skip during global fade-in (first few ms after trigger)
            int intPosition = static_cast<int>(effectivePosition);
            int effectiveLength = static_cast<int>(effectiveCaptureLength);
            const int safeCrossfade = juce::jmin(crossfadeSamples, effectiveLength / 4);

            // Check if we're in the crossfade zone (last safeCrossfade samples of the loop)
            // AND we have more repeats to do (otherwise let it fade out naturally)
            // AND not during retrigger crossfade or global fade-in
            if (safeCrossfade > 0 && intPosition >= effectiveLength - safeCrossfade &&
                currentRepeat < maxRepeats && !retriggerCrossfadeActive && fadeInSamplesRemaining <= 0)
            {
                // Calculate how far into the crossfade zone we are
                int crossfadePosition = intPosition - (effectiveLength - safeCrossfade);
                float crossfadeProgress = static_cast<float>(crossfadePosition) / static_cast<float>(safeCrossfade);
                crossfadeProgress = juce::jlimit(0.0f, 1.0f, crossfadeProgress);

                // Equal-power crossfade weights
                // At start of zone: oldWeight=1, newWeight=0
                // At end of zone: oldWeight=0, newWeight=1
                float newWeight = std::sin(crossfadeProgress * juce::MathConstants<float>::halfPi);
                float oldWeight = std::cos(crossfadeProgress * juce::MathConstants<float>::halfPi);

                // Current audio (from near end of loop) is the "old" audio being faded out
                // We already have it in leftOut/rightOut

                // Read "new" audio from the START of the loop (what we'll crossfade into)
                float newLeftOut = 0.0f;
                float newRightOut = 0.0f;

                // Calculate position in the START of the loop (mirrors the crossfade position)
                double newPosition = static_cast<double>(crossfadePosition);

                if (newPosition < effectiveCaptureLength)
                {
                    double newReadOffset = reverseEnabled
                        ? (captureLength - 1.0 - (newPosition * pitchRatio))
                        : (newPosition * pitchRatio);

                    newReadOffset = juce::jlimit(0.0, static_cast<double>(captureLength - 1), newReadOffset);

                    int newBasePos = (captureStartPosition + static_cast<int>(newReadOffset)) % maxCaptureSamples;
                    int newNextPos = (newBasePos + 1) % maxCaptureSamples;
                    float newFrac = static_cast<float>(newReadOffset - static_cast<int>(newReadOffset));

                    float newLeft0 = captureBuffer.getSample(0, newBasePos);
                    float newLeft1 = captureBuffer.getSample(0, newNextPos);
                    float newRight0 = captureBuffer.getSample(1, newBasePos);
                    float newRight1 = captureBuffer.getSample(1, newNextPos);

                    newLeftOut = newLeft0 + newFrac * (newLeft1 - newLeft0);
                    newRightOut = newRight0 + newFrac * (newRight1 - newRight0);
                }

                // Blend: old (current position near end) fading out, new (loop start) fading in
                leftOut = leftOut * oldWeight + newLeftOut * newWeight;
                rightOut = rightOut * oldWeight + newRightOut * newWeight;
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

    // Set capture length to subdivision length (clamped to buffer size)
    // (Moved before retrigger check so captureLength is valid for immediate capture)
    int newCaptureLength = juce::jmin(static_cast<int>(subdivisionSamples), maxCaptureSamples);

    // v1.1.6: Handle retrigger during active playback
    // Save old state for crossfade blending to prevent clicks
    if (isTriggered && currentRepeat < maxRepeats)
    {
        // We're retriggering during active playback - start crossfade
        retriggerCrossfadeActive = true;
        retriggerCrossfadeSamplesRemaining = crossfadeSamples;
        oldCaptureStartPosition = captureStartPosition;
        oldFractionalPlaybackPosition = fractionalPlaybackPosition;
        oldCurrentGain = currentGain;

        // v1.1.12: Calculate capture position IMMEDIATELY for retriggers
        // Buffer already contains audio from previous blocks - no need to defer
        // Deferring during retrigger causes stale position reads during crossfade
        captureLength = newCaptureLength;
        captureStartPosition = (captureWritePosition - captureLength + maxCaptureSamples) % maxCaptureSamples;
        pendingCapture = false;  // Don't defer for retriggers
    }
    else
    {
        // v1.1.1: Start global fade-in for click-free onset (first trigger or after repeats finished)
        globalEnvelopeGain = 0.0f;
        fadeInSamplesRemaining = crossfadeSamples;

        // Set capture length
        captureLength = newCaptureLength;

        // v1.1.5: Use DEFERRED capture calculation for correct ENV trigger timing
        // When trigger() is called from PluginProcessor BEFORE processBlock(), the capture
        // buffer doesn't contain the current block yet. Setting pendingCapture = true
        // defers the captureStartPosition calculation to AFTER the current block is written.
        // This ensures transients that triggered the effect are actually captured.
        pendingCapture = true;
    }

    // Start new repeat cycle
    isTriggered = true;
    currentRepeat = 0;
    playbackPosition = 0;
    fractionalPlaybackPosition = 0.0;
    currentGain = 1.0f;

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
    // v1.1.14: Loop boundary overlap-add is now handled in processBlock()
    // by detecting when we approach the end of the capture and starting
    // the blend early. This ensures the old position is still valid.

    // Reset playback position (integer and fractional)
    playbackPosition = 0;
    fractionalPlaybackPosition = 0.0;

    // Reset loop crossfade state (in case it was active)
    loopCrossfadeActive = false;
    loopCrossfadeSamplesRemaining = 0;
    loopOldPlaybackPosition = 0.0;

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

    // v1.1.12: Equal-power crossfade using sine/cosine curves
    // Maintains constant perceived loudness during blend (no -6dB dip at midpoint)
    if (fadeIn)
        return std::sin(ratio * juce::MathConstants<float>::halfPi);
    else
        return std::cos(ratio * juce::MathConstants<float>::halfPi);
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
