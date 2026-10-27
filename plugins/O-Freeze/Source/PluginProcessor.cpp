/*
   This file is part of O-Freeze, an Ouaricon Audio plugin.
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

    O-Freeze - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
// PluginEditor.h is deliberately NOT included at the top of this TU — the
// include lives inside the #if JUCE_WEB_BROWSER guard directly above
// createEditor(), so a console target that compiles this TU with
// JUCE_WEB_BROWSER=0 and no editor sources (scripts/param-dump) links.

juce::AudioProcessorValueTreeState::ParameterLayout OFreezeAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // FREEZE - Manual trigger button
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "FREEZE", 1 },
        "Freeze",
        false));

    // THRESHOLD - Auto-freeze level
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "THRESHOLD", 1 },
        "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
        -40.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // MODE - Trigger mode selection
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MODE", 1 },
        "Mode",
        juce::StringArray { "Manual", "Threshold" },
        0));

    // DRIFT - Grain position randomization range
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DRIFT", 1 },
        "Drift",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // MIX - Dry/Wet blend
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // GRAIN_SIZE - Granular grain size in milliseconds
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "GRAIN_SIZE", 1 },
        "Grain Size",
        juce::NormalisableRange<float>(50.0f, 1000.0f, 1.0f),
        400.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // GRAIN_COUNT - Number of active grains (2-32, integer steps)
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "GRAIN_COUNT", 1 },
        "Grain Count",
        2, 32, 8));

    // LFO_RATE - LFO frequency in Hz
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "LFO_RATE", 1 },
        "LFO Rate",
        juce::NormalisableRange<float>(0.01f, 10.0f, 0.01f, 0.4f),
        0.5f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    // LFO_DEPTH - LFO modulation depth
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "LFO_DEPTH", 1 },
        "LFO Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // LFO_SHAPE - LFO waveform shape
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "LFO_SHAPE", 1 },
        "LFO Shape",
        juce::StringArray { "Sine", "Triangle", "Random" },
        0));

    // REVERSE - Toggle reverse grain playback direction
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "REVERSE", 1 },
        "Reverse",
        false));

    // DETUNE - Per-grain pitch micro-detuning range in cents
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DETUNE", 1 },
        "Detune",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f),
        5.0f,
        juce::AudioParameterFloatAttributes().withLabel("cents")));

    return layout;
}

OFreezeAudioProcessor::OFreezeAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OFreezeAudioProcessor::~OFreezeAudioProcessor()
{
}

void OFreezeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store sample rate
    currentSampleRate = sampleRate;

    // Prepare DSP spec
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Pre-allocate freeze buffer for 2 seconds at max sample rate (192kHz = 384,000 samples)
    const int maxSampleRate = 192000;
    const int bufferLengthSeconds = 2;
    const int bufferLength = maxSampleRate * bufferLengthSeconds;
    freezeBuffer.setSize(getTotalNumOutputChannels(), bufferLength);
    freezeBuffer.clear();

    // Reset positions
    writePosition = 0;
    bufferFrozen = false;

    // Initialize crossfade smoother (50ms fade-in, 100ms fade-out)
    freezeGain.reset(sampleRate, 0.050); // 50ms ramp time
    freezeGain.setCurrentAndTargetValue(0.0f);

    // Prepare dry/wet mixer
    dryWetMixer.prepare(spec);
    dryWetMixer.reset();

    // Initialize threshold gate RMS detection (20ms rolling window)
    rmsSamplesPerWindow = static_cast<int>(sampleRate * 0.020);
    rmsBuffer.resize(rmsSamplesPerWindow);
    std::fill(rmsBuffer.begin(), rmsBuffer.end(), 0.0f);
    rmsWriteIndex = 0;
    rmsLevel = 0.0f;
    gateState = GateState::Idle;

    // Initialize granular synthesis components
    maxGrainSize = static_cast<int>(sampleRate * 1.0);

    // v2.7.0: fixed raw Hann table, read by each grain's own phase. No COLA scaling —
    // per-grain jitter breaks COLA alignment; level comes from the density law.
    for (int i = 0; i <= WINDOW_LUT_SIZE; ++i)
    {
        double phase = static_cast<double>(i) / static_cast<double>(WINDOW_LUT_SIZE);
        windowLut[static_cast<size_t>(i)] = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * juce::MathConstants<double>::pi * phase)));
    }

    densityPower = 0.0f;
    densityCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (kDensitySmoothSeconds * sampleRate)));

    // Read initial grain size and grain count from parameters
    auto* grainSizeParam = parameters.getRawParameterValue("GRAIN_SIZE");
    auto* grainCountParam = parameters.getRawParameterValue("GRAIN_COUNT");
    float grainSizeMs = grainSizeParam->load();
    numGrains = static_cast<int>(grainCountParam->load());
    lastGrainSizeMs = grainSizeMs;
    lastGrainCount = numGrains;
    // WR-04: clamp to maxGrainSize exactly as processBlock does, so the grain length
    // (and the freeze-buffer reach it implies) stays bounded if the GRAIN_SIZE range or
    // the maxGrainSize constant ever drift out of sync.
    grainSize = juce::jmax(1, juce::jmin(static_cast<int>(sampleRate * grainSizeMs / 1000.0), maxGrainSize));
    grainTriggerInterval = grainSize / numGrains;

    // Reset all grains to inactive
    for (auto& grain : grains)
        grain = Grain {};

    grainTriggerCounter = 0;
    nextTriggerInterval = grainTriggerInterval;
    stopTriggeringNewGrains = false;

    // Initialize WSOLA state
    lastGrainTail.fill(0.0f);
    hasLastGrainTail = false;

    // Initialize drift state
    frozenDriftOffset = 0.0f;
    smoothedDriftOffset.reset(sampleRate, 0.015); // 15ms smoothing to eliminate clicks
    smoothedDriftOffset.setCurrentAndTargetValue(0.0f);

    // Initialize LFO state
    lfoPhase = 0.0f;
    lfoCurrentValue = 0.0f;
    randomHoldValue = 0.0f;
}

void OFreezeAudioProcessor::releaseResources()
{
    freezeBuffer.setSize(0, 0);

    for (auto& grain : grains)
        grain = Grain {};

    densityPower = 0.0f;
    freezeGain.setCurrentAndTargetValue(0.0f);
    bufferFrozen = false;
    stopTriggeringNewGrains = false;

    lastGrainTail.fill(0.0f);
    hasLastGrainTail = false;
}

void OFreezeAudioProcessor::startGrain(Grain& g, int basePos, int jitterOffset, float playbackRate, int startSample) noexcept
{
    g = Grain {};
    g.active = true;
    g.length = grainSize;
    g.invLength = 1.0 / static_cast<double>(grainSize);
    g.startSample = startSample;
    g.position = basePos;
    g.jitterOffset = jitterOffset;
    g.playbackRate = playbackRate;
    g.fractionalPosition = static_cast<double>(startSample) * static_cast<double>(playbackRate);
}

int OFreezeAudioProcessor::findFreeGrainSlot() noexcept
{
    for (int i = 0; i < POOL_GRAINS; ++i)
        if (!grains[static_cast<size_t>(i)].active)
            return i;

    // Pool full: steal the oldest grain (furthest through its window, so the quietest
    // in the common case). It moves to a spare slot and fades out there; the new grain
    // takes its slot. When every spare is busy, reuse the one nearest silence.
    int victim = 0;
    double oldest = -1.0;
    for (int i = 0; i < POOL_GRAINS; ++i)
    {
        const auto& g = grains[static_cast<size_t>(i)];
        const double phase = static_cast<double>(g.startSample) * g.invLength;
        if (phase > oldest)
        {
            oldest = phase;
            victim = i;
        }
    }

    int spare = POOL_GRAINS;
    for (int i = POOL_GRAINS; i < MAX_GRAINS; ++i)
    {
        if (!grains[static_cast<size_t>(i)].active)
        {
            spare = i;
            break;
        }
        if (grains[static_cast<size_t>(i)].fadeGain < grains[static_cast<size_t>(spare)].fadeGain)
            spare = i;
    }

    auto& moved = grains[static_cast<size_t>(spare)];
    moved = grains[static_cast<size_t>(victim)];
    moved.fadeGain = 1.0f;
    moved.fadeStep = static_cast<float>(1.0 / (kStealFadeSeconds * currentSampleRate));
    return victim;
}

float OFreezeAudioProcessor::randomDetuneRate(float detuneCents) noexcept
{
    // Per-grain pitch micro-detuning (cents → rate multiplier).
    // r is a uniform random spread in [-1, 1]; the knob is the half-width in
    // cents, so the rate is 2^(r·c/1200). v2.3.0: this was the linear
    // 1 + r·c/1200, which is not cents — at knob 50 it reached +70.67 /
    // -73.68 ct (1.41x / 1.47x the label), measured on a frozen 440 Hz sine.
    return std::pow(2.0f, (random.nextFloat() * 2.0f - 1.0f) * (detuneCents / 1200.0f));
}

void OFreezeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), freezeBuffer.getNumChannels());

    if (numSamples == 0)
        return;

    // Read parameters (atomic, real-time safe)
    auto* freezeParam = parameters.getRawParameterValue("FREEZE");
    auto* modeParam = parameters.getRawParameterValue("MODE");
    auto* thresholdParam = parameters.getRawParameterValue("THRESHOLD");
    auto* mixParam = parameters.getRawParameterValue("MIX");
    auto* grainSizeParam = parameters.getRawParameterValue("GRAIN_SIZE");
    auto* grainCountParam = parameters.getRawParameterValue("GRAIN_COUNT");
    auto* reverseParam = parameters.getRawParameterValue("REVERSE");
    auto* detuneParam = parameters.getRawParameterValue("DETUNE");

    int modeValue = static_cast<int>(modeParam->load());
    float thresholdDB = thresholdParam->load();
    float mixValue = mixParam->load() / 100.0f; // Convert 0-100% to 0-1
    float grainSizeMs = grainSizeParam->load();
    int grainCountValue = static_cast<int>(grainCountParam->load());
    bool reverseActive = reverseParam->load() > 0.5f;
    float detuneValue = detuneParam->load();

    // Recalculate grain parameters when size or count changes
    bool grainParamsChanged = (grainSizeMs != lastGrainSizeMs) || (grainCountValue != lastGrainCount);
    if (grainParamsChanged)
    {
        lastGrainSizeMs = grainSizeMs;
        lastGrainCount = grainCountValue;
        numGrains = grainCountValue;
        grainSize = juce::jmax(1, juce::jmin(static_cast<int>(currentSampleRate * grainSizeMs / 1000.0), maxGrainSize));
        grainTriggerInterval = grainSize / numGrains;
        nextTriggerInterval = grainTriggerInterval;

        // v2.7.0: nothing is rebuilt or deactivated here. Live grains keep their
        // latched length and read the fixed window table by phase; the new size and
        // count reach only the grains started from now on (and the trigger rate).
    }

    // Determine freeze state based on mode
    bool freezeActive = false;

    if (modeValue == 0) // Manual mode
    {
        freezeActive = freezeParam->load() > 0.5f;
    }
    else // Threshold mode
    {
        // Calculate RMS level from input buffer
        // WR-01: sum channels to a mono value per time instant BEFORE pushing one squared
        // sample. Writing each channel serially inflated the window rate (halving the
        // effective 20 ms window for stereo) and interleaved L/R out of order.
        float sumSquares = 0.0f;
        const int rmsChannels = juce::jmax(1, numChannels);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mono = 0.0f;
            for (int channel = 0; channel < numChannels; ++channel)
                mono += buffer.getReadPointer(channel)[sample];
            mono /= static_cast<float>(rmsChannels);

            // Update RMS buffer (circular)
            rmsBuffer[rmsWriteIndex] = mono * mono;
            rmsWriteIndex = (rmsWriteIndex + 1) % rmsSamplesPerWindow;
        }

        // Calculate RMS from rolling window
        for (float squaredSample : rmsBuffer)
            sumSquares += squaredSample;

        float rms = std::sqrt(sumSquares / static_cast<float>(rmsSamplesPerWindow));
        rmsLevel = 20.0f * std::log10(rms + 1e-6f); // Convert to dB

        // Threshold gate with hysteresis (3dB)
        const float hysteresisDB = 3.0f;

        if (gateState == GateState::Idle && rmsLevel < thresholdDB)
        {
            gateState = GateState::Frozen;
        }
        else if (gateState == GateState::Frozen && rmsLevel > (thresholdDB + hysteresisDB))
        {
            gateState = GateState::Idle;
        }

        freezeActive = (gateState == GateState::Frozen);
    }

    // Read DRIFT parameter
    auto* driftParam = parameters.getRawParameterValue("DRIFT");
    float driftValue = driftParam->load() / 100.0f; // Convert 0-100% to 0-1

    // Read LFO parameters
    auto* lfoRateParam = parameters.getRawParameterValue("LFO_RATE");
    auto* lfoDepthParam = parameters.getRawParameterValue("LFO_DEPTH");
    auto* lfoShapeParam = parameters.getRawParameterValue("LFO_SHAPE");
    float lfoRate = lfoRateParam->load();
    float lfoDepth = lfoDepthParam->load() / 100.0f; // Convert 0-100% to 0-1
    int lfoShape = static_cast<int>(lfoShapeParam->load());

    // Compute LFO per-block (not per-sample) for efficiency
    if (bufferFrozen && lfoDepth > 0.0f)
    {
        // Advance LFO phase
        float phaseIncrement = lfoRate * static_cast<float>(numSamples) / static_cast<float>(currentSampleRate);
        lfoPhase += phaseIncrement;

        // Wrap phase and handle random S&H trigger
        if (lfoPhase >= 1.0f)
        {
            lfoPhase -= std::floor(lfoPhase);
            // New random value each cycle for S&H
            randomHoldValue = random.nextFloat() * 2.0f - 1.0f;
        }

        // Compute LFO value based on shape (-1 to +1)
        switch (lfoShape)
        {
            case 0: // Sine
                lfoCurrentValue = std::sin(lfoPhase * juce::MathConstants<float>::twoPi);
                break;
            case 1: // Triangle
                lfoCurrentValue = 4.0f * std::abs(lfoPhase - 0.5f) - 1.0f;
                break;
            case 2: // Random (Sample & Hold)
                lfoCurrentValue = randomHoldValue;
                break;
            default:
                lfoCurrentValue = 0.0f;
                break;
        }
    }
    else if (!bufferFrozen)
    {
        lfoCurrentValue = 0.0f;
    }

    // Push dry signal to mixer
    dryWetMixer.pushDrySamples(juce::dsp::AudioBlock<float>(buffer));

    // Update freeze state and crossfade envelope
    if (freezeActive != bufferFrozen)
    {
        bufferFrozen = freezeActive;

        if (bufferFrozen)
        {
            // Freeze engaged: fade in frozen signal
            // WR-02: reset() calls setCurrentAndTargetValue() internally, snapping the gain
            // to its old target and discarding any in-progress fade (click on rapid toggle).
            // Snapshot the current level and restore it so the new ramp starts from where
            // the previous fade actually was.
            float heldGain = freezeGain.getCurrentValue();
            freezeGain.reset(currentSampleRate, 0.050); // 50ms fade-in
            freezeGain.setCurrentAndTargetValue(heldGain);
            freezeGain.setTargetValue(1.0f);
            stopTriggeringNewGrains = false;  // Ensure we're triggering grains

            // v2.7.0: re-engaged while a release tail is still rendering. The write head
            // has been held for the whole tail (WR-03), so the live grains still read the
            // frozen audio: keep them and resume triggering. (Before, grains 1..31 were
            // hard-cut here mid-fade.)
            bool tailLive = false;
            if (heldGain > 0.001f)
                for (const auto& grain : grains)
                    tailLive = tailLive || grain.active;

            if (!tailLive)
            {
                // Reset WSOLA state for fresh freeze
                hasLastGrainTail = false;

                // LOCK drift offset at freeze moment - all grains share this for COLA
                // Pick a random offset (adds variation between freeze engages)
                frozenDriftOffset = random.nextFloat();

                for (auto& grain : grains)
                    grain = Grain {};

                // v2.7.0: start from the steady state — GRAIN_COUNT grains at staggered
                // phases, as if they had been triggered one interval apart — and seed the
                // density estimate to match. With the window-sum divide gone, a single
                // first grain would fade in over half a grain; this keeps the 50 ms engage.
                const int freezeBufLen = freezeBuffer.getNumSamples();
                int startPos = reverseActive
                    ? (writePosition - 1 + freezeBufLen) % freezeBufLen
                    : (writePosition - grainSize + freezeBufLen) % freezeBufLen;
                const float jitterRange = static_cast<float>(grainSize) * driftValue;

                for (int i = 0; i < numGrains; ++i)
                {
                    // Grain 0 starts now, clean (no jitter, no detune) as before
                    const int jitter = i == 0 ? 0 : static_cast<int>(random.nextFloat() * jitterRange);
                    const float rate = i == 0 ? 1.0f : randomDetuneRate(detuneValue);
                    startGrain(grains[static_cast<size_t>(i)], startPos, jitter, rate,
                               static_cast<int>(static_cast<juce::int64>(grainSize) * i / numGrains));
                }

                densityPower = static_cast<float>(numGrains) * kHannMu2;
                grainTriggerCounter = 0;
                nextTriggerInterval = grainTriggerInterval;  // Reset to nominal for clean first cycle
            }
        }
        else
        {
            // Freeze released: soft deactivation
            // Stop triggering NEW grains, but let active grains complete their cycle
            stopTriggeringNewGrains = true;

            // Fade-out covers grain completion time + 50ms safety margin
            // WR-02: preserve the current fade level across the reset() (see fade-in branch)
            // so releasing mid-fade-in doesn't snap to full before ramping down.
            float heldGain = freezeGain.getCurrentValue();
            // v2.7.0: grains keep their latched lengths, so the tail lasts as long as the
            // longest remaining grain, not the current GRAIN_SIZE.
            int remaining = 0;
            for (const auto& grain : grains)
                if (grain.active)
                    remaining = juce::jmax(remaining, grain.length - grain.startSample);
            float grainDurationSec = static_cast<float>(remaining) / static_cast<float>(currentSampleRate);
            freezeGain.reset(currentSampleRate, static_cast<double>(grainDurationSec) + 0.050);
            freezeGain.setCurrentAndTargetValue(heldGain);
            freezeGain.setTargetValue(0.0f);

            // DON'T deactivate grains here - they'll naturally complete
            // when startSample >= length (already handled in grain advance loop)
        }
    }

    const int freezeBufferLength = freezeBuffer.getNumSamples();

    // Compute target drift offset in samples (smoothed per-sample to prevent clicks)
    {
        float modulatedDriftOffset = frozenDriftOffset + (lfoCurrentValue * lfoDepth);
        modulatedDriftOffset = juce::jlimit(0.0f, 1.0f, modulatedDriftOffset);
        float driftRangeF = static_cast<float>(grainSize) * driftValue;
        float targetDriftSamples = modulatedDriftOffset * driftRangeF;
        smoothedDriftOffset.setTargetValue(targetDriftSamples);
    }

    // Process sample-by-sample (all channels together) so grain state stays in sync
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Only trigger new grains if frozen AND not in soft release mode
        if (bufferFrozen && !stopTriggeringNewGrains)
        {
            // IMPORTANT: Drift offset is LOCKED when freeze engages (set in freeze activation)
            // Do NOT update drift while frozen - all grains must share the same offset
            // for COLA phase alignment to work correctly

            if (grainTriggerCounter >= nextTriggerInterval)
            {
                // Store BASE position only - drift applied at read time
                int basePos = reverseActive
                    ? (writePosition - 1 + freezeBufferLength) % freezeBufferLength
                    : (writePosition - grainSize + freezeBufferLength) % freezeBufferLength;

                // WSOLA: search for best-overlap position with previous grain's tail
                if (hasLastGrainTail)
                {
                    // CR-01: cap the search span to a musically-meaningful window (~5 ms)
                    // so the per-trigger cross-correlation cost stays within the RT budget.
                    // Uncapped, grainSize/4 reached ~48k at 192 kHz / 1000 ms grains, giving
                    // millions of MACs inside one sample iteration → audio-deadline xruns.
                    const int maxSearch = static_cast<int>(currentSampleRate * 0.005); // 5 ms
                    int searchRange = juce::jmin(grainSize / 4, maxSearch);
                    int bestOffset = 0;
                    float bestCorrelation = -1.0f;

                    // Precompute energy of lastGrainTail (constant across all offsets)
                    float sumX2 = 0.0f;
                    for (int i = 0; i < WSOLA_TAIL_SIZE; ++i)
                        sumX2 += lastGrainTail[i] * lastGrainTail[i];

                    for (int offset = -searchRange; offset <= searchRange; offset += 4)
                    {
                        int candidateStart = (basePos + offset + freezeBufferLength) % freezeBufferLength;

                        float sumXY = 0.0f;
                        float sumY2 = 0.0f;
                        for (int i = 0; i < WSOLA_TAIL_SIZE; ++i)
                        {
                            int samplePos = reverseActive
                                ? (candidateStart - i + freezeBufferLength) % freezeBufferLength
                                : (candidateStart + i) % freezeBufferLength;

                            float mono = 0.0f;
                            for (int ch = 0; ch < numChannels; ++ch)
                                mono += freezeBuffer.getReadPointer(ch)[samplePos];
                            mono /= static_cast<float>(numChannels);

                            sumXY += lastGrainTail[i] * mono;
                            sumY2 += mono * mono;
                        }

                        float denom = std::sqrt(sumX2 * sumY2);
                        float corr = (denom > 1e-6f) ? (sumXY / denom) : 0.0f;

                        if (corr > bestCorrelation)
                        {
                            bestCorrelation = corr;
                            bestOffset = offset;
                        }
                    }

                    basePos = (basePos + bestOffset + freezeBufferLength) % freezeBufferLength;
                }

                // Per-grain random jitter ON TOP of WSOLA-chosen position
                // Range scales with grain size and drift amount
                float jitterRange = static_cast<float>(grainSize) * driftValue;
                const int positionJitter = static_cast<int>(random.nextFloat() * jitterRange);

                // v2.7.0: any free pool slot (fade-on-steal when the pool is full),
                // replacing the round-robin restart of a still-sounding grain (IN-03).
                startGrain(grains[static_cast<size_t>(findFreeGrainSlot())], basePos, positionJitter,
                           randomDetuneRate(detuneValue), 0);

                // Reset trigger counter and compute jittered interval for next trigger
                grainTriggerCounter = 0;
                int jitter = static_cast<int>((random.nextFloat() * 2.0f - 1.0f) * grainTriggerInterval * 0.3f);
                nextTriggerInterval = juce::jmax(grainTriggerInterval / 2, grainTriggerInterval + jitter);
            }
            else
            {
                grainTriggerCounter++;
            }
        }

        // Smoothed drift offset (ramps per-sample to prevent clicks from knob/LFO changes)
        int sharedDriftOffset = static_cast<int>(smoothedDriftOffset.getNextValue());

        // Get current window values and interpolated positions for all active grains (before advancing)
        float liveGrains = 0.0f;

        for (int g = 0; g < MAX_GRAINS; ++g)
        {
            if (grains[g].active)
            {
                liveGrains += 1.0f;
                windowValues[g] = windowAt(grains[g]) * grains[g].fadeGain;

                // Fractional position for pitch-shifted grain read
                double fp = grains[g].fractionalPosition;
                int intPart = static_cast<int>(fp);
                float frac = static_cast<float>(fp - static_cast<double>(intPart));

                // Compute interpolation sample positions (base + fractional offset + jitter + drift)
                if (reverseActive)
                {
                    grainPos0[g] = (grains[g].position - intPart + grains[g].jitterOffset + sharedDriftOffset + freezeBufferLength * 2) % freezeBufferLength;
                    grainPos1[g] = (grainPos0[g] - 1 + freezeBufferLength) % freezeBufferLength;
                }
                else
                {
                    grainPos0[g] = (grains[g].position + intPart + grains[g].jitterOffset + sharedDriftOffset + freezeBufferLength) % freezeBufferLength;
                    grainPos1[g] = (grainPos0[g] + 1) % freezeBufferLength;
                }
                grainFrac[g] = frac;
            }
        }

        // Advance freeze crossfade smoother and density estimate once per sample (NOT per channel)
        float currentFreezeGain = freezeGain.getNextValue();
        const float densityGain = nextDensityGain(liveGrains);

        // WR-03: while the release tail is still rendering, active grains keep reading the
        // freeze buffer. Suppress live writes AND freeze the write head for the whole tail,
        // otherwise the advancing write head overwrites the region the grains are reading
        // and corrupts the fade-out (audible for large grain sizes).
        bool renderingTail = bufferFrozen || freezeGain.getCurrentValue() > 0.001f;

        // Process each channel using the same grain state
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            auto* freezeData = freezeBuffer.getWritePointer(channel);

            float inputSample = channelData[sample];
            float outputSample = inputSample;

            // Write to freeze buffer (only when not frozen AND not rendering a release tail)
            if (!renderingTail)
            {
                freezeData[writePosition] = inputSample;
            }

            // Granular synthesis (if frozen or fading out)
            if (renderingTail)
            {
                // Sum all active grains (overlap-add synthesis)
                float granularSum = 0.0f;

                for (int g = 0; g < MAX_GRAINS; ++g)
                {
                    if (grains[g].active)
                    {
                        // Linear interpolation between adjacent buffer samples
                        float s0 = freezeData[grainPos0[g]];
                        float s1 = freezeData[grainPos1[g]];
                        float grainSample = s0 + grainFrac[g] * (s1 - s0);
                        granularSum += grainSample * windowValues[g];
                    }
                }

                // v2.7.0: live-N density law (see nextDensityGain), not a window-sum divide
                float frozenSample = granularSum * densityGain;

                // Apply crossfade envelope
                outputSample = inputSample * (1.0f - currentFreezeGain) + frozenSample * currentFreezeGain;
            }

            channelData[sample] = outputSample;
        }

        // Advance grain state AFTER processing all channels
        for (auto& grain : grains)
        {
            if (grain.active)
            {
                grain.startSample++;
                grain.fractionalPosition += grain.playbackRate;

                // Stolen grain: ends silently when its fade completes (no WSOLA tail)
                if (grain.fadeStep > 0.0f)
                {
                    grain.fadeGain -= grain.fadeStep;
                    if (grain.fadeGain <= 0.0f || grain.startSample >= grain.length)
                        grain.active = false;
                    continue;
                }

                // Deactivate grain when complete
                if (grain.startSample >= grain.length)
                {
                    // Compute final buffer position from fractional offset
                    int endOffset = static_cast<int>(grain.fractionalPosition);
                    int endPos = reverseActive
                        ? (grain.position - endOffset + freezeBufferLength * 2) % freezeBufferLength
                        : (grain.position + endOffset) % freezeBufferLength;

                    // Capture final 64 samples for WSOLA overlap matching (mono sum)
                    for (int i = 0; i < WSOLA_TAIL_SIZE; ++i)
                    {
                        int pos = reverseActive
                            ? (endPos + WSOLA_TAIL_SIZE - i + freezeBufferLength) % freezeBufferLength
                            : (endPos - WSOLA_TAIL_SIZE + i + freezeBufferLength) % freezeBufferLength;

                        float mono = 0.0f;
                        for (int ch = 0; ch < numChannels; ++ch)
                            mono += freezeBuffer.getReadPointer(ch)[pos];
                        lastGrainTail[i] = mono / static_cast<float>(numChannels);
                    }
                    hasLastGrainTail = true;

                    grain.active = false;
                }
            }
        }

        // Advance write position AFTER processing all channels
        // WR-03: hold the write head frozen for the whole release tail (not just while
        // bufferFrozen) so grains reading the tail aren't overwritten.
        if (!renderingTail)
        {
            writePosition = (writePosition + 1) % freezeBufferLength;
        }
    }

    // Apply dry/wet mix
    dryWetMixer.setWetMixProportion(mixValue);
    dryWetMixer.mixWetSamples(juce::dsp::AudioBlock<float>(buffer));
}

#if JUCE_WEB_BROWSER
#include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* OFreezeAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OFreezeAudioProcessorEditor(*this);
#else
    // The param-dump console target builds with JUCE_WEB_BROWSER=0 and no
    // editor sources. It never opens an editor; this keeps the TU linkable.
    return new juce::GenericAudioProcessorEditor(*this);
#endif
}

void OFreezeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // v2.1.0: the UI language rides the same tree as one more plain property.
    // Not a parameter (see PluginProcessor.h), so it is saved and restored here
    // rather than by the APVTS parameter round-trip. Written as a STRING
    // ("en"/"fr") rather than the atomic's int index, so a hand-inspected
    // session file says what it means.
    state.setProperty("uiLanguage",
                      languageCode(uiLanguage.load(std::memory_order_acquire)), nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OFreezeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));

        // v2.1.0: the UI language. isVoid() is the ONLY correct guard and
        // toString() the only correct read. getStateInformation writes a STRING
        // var, but even a bool or int written there would not survive: the XML
        // round-trip does not preserve the type, because
        // NamedValueSet::setFromXmlAttributes rebuilds every property as
        // `var (value)` over the attribute STRING
        // (critical_valuetree_xml_roundtrip_loses_type). A pre-2.1.0 session has
        // no such property at all and the default (English) stands.
        // languageIndex() clamps anything that is not "fr" to 0, so a
        // hand-edited value degrades to English rather than to a bad index.
        //
        // The editor PULLS this through the getUiLanguage native fn at page
        // init rather than being pushed from here — a push would race the
        // WebView's load.
        const juce::var lang = parameters.state.getProperty("uiLanguage");

        if (! lang.isVoid())
            uiLanguage.store(languageIndex(lang.toString()), std::memory_order_release);
    }
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OFreezeAudioProcessor();
}
