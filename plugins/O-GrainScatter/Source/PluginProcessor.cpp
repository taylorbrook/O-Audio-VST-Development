/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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
#include "PluginProcessor.h"
#include "PluginEditor.h"

GrainScatterProcessor::GrainScatterProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    grainSizeParam       = parameters.getRawParameterValue("grain_size");
    densityParam         = parameters.getRawParameterValue("density");
    pitchRandomParam     = parameters.getRawParameterValue("pitch_random");
    panRandomParam       = parameters.getRawParameterValue("pan_random");
    scaleParam           = parameters.getRawParameterValue("scale");
    rootNoteParam        = parameters.getRawParameterValue("root_note");
    reverseParam         = parameters.getRawParameterValue("reverse");
    feedbackParam        = parameters.getRawParameterValue("feedback");
    dryWetParam          = parameters.getRawParameterValue("dry_wet");
    syncModeParam        = parameters.getRawParameterValue("sync_mode");
    probabilityParam     = parameters.getRawParameterValue("probability");
    repeatsParam         = parameters.getRawParameterValue("repeats");
    spreadParam          = parameters.getRawParameterValue("spread");
    pitchModeParam       = parameters.getRawParameterValue("pitch_mode");
    freezeParam          = parameters.getRawParameterValue("freeze");
    euclideanPulsesParam   = parameters.getRawParameterValue("euclidean_pulses");
    euclideanStepsParam    = parameters.getRawParameterValue("euclidean_steps");
    euclideanRotationParam = parameters.getRawParameterValue("euclidean_rotation");
    euclideanSwingParam    = parameters.getRawParameterValue("euclidean_swing");
    stutterGateParam       = parameters.getRawParameterValue("stutter_gate");
    sizeRandomParam      = parameters.getRawParameterValue("size_random");
    ampRandomParam       = parameters.getRawParameterValue("amp_random");
    scanPositionParam    = parameters.getRawParameterValue("scan_position");
    grainShapeParam      = parameters.getRawParameterValue("grain_shape");

    // Spatial parameters
    spatialModeParam     = parameters.getRawParameterValue("spatial_mode");
    azimuthParam         = parameters.getRawParameterValue("azimuth");
    elevationParam       = parameters.getRawParameterValue("elevation");
    azSpreadParam        = parameters.getRawParameterValue("az_spread");
    elSpreadParam        = parameters.getRawParameterValue("el_spread");
    distanceParam        = parameters.getRawParameterValue("distance");
    spatialWidthParam    = parameters.getRawParameterValue("spatial_width");
    trajectoryParam      = parameters.getRawParameterValue("trajectory");
    trajSpeedParam       = parameters.getRawParameterValue("traj_speed");
    distLpfParam         = parameters.getRawParameterValue("dist_lpf");
    dopplerParam         = parameters.getRawParameterValue("doppler");
    spatialSmoothParam   = parameters.getRawParameterValue("spatial_smooth");
}

GrainScatterProcessor::~GrainScatterProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout GrainScatterProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // === Core Granular Engine ===
    auto coreGroup = std::make_unique<juce::AudioProcessorParameterGroup>("core", "Core", "|");

    coreGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "grain_size", 1 },
        "Grain Size",
        juce::NormalisableRange<float>(10.0f, 500.0f, 0.1f, 0.5f),
        100.0f));

    coreGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "density", 1 },
        "Density",
        juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f),
        50.0f));

    coreGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitch_random", 1 },
        "Pitch Random",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    coreGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pan_random", 1 },
        "Pan Random",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    coreGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "scale", 1 },
        "Scale",
        juce::StringArray { "Chromatic", "Major", "Minor", "Pentatonic", "Whole Tone" },
        0));

    coreGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "root_note", 1 },
        "Root Note",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" },
        0));

    coreGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverse", 1 },
        "Reverse",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    coreGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "feedback", 1 },
        "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    coreGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "dry_wet", 1 },
        "Dry/Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f));

    coreGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "grain_shape", 1 },
        "Grain Shape",
        juce::StringArray { "Hann", "Triangle", "Trapezoid", "Tukey", "Blackman", "Exp Decay" },
        0));

    coreGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "size_random", 1 },
        "Size Random",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    coreGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "amp_random", 1 },
        "Amp Random",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    coreGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "scan_position", 1 },
        "Scan Position",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    layout.add(std::move(coreGroup));

    // === Beat Sync ===
    auto syncGroup = std::make_unique<juce::AudioProcessorParameterGroup>("sync", "Sync", "|");

    syncGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "sync_mode", 1 },
        "Sync Mode",
        juce::StringArray { "Free", "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T" },
        0));

    syncGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "probability", 1 },
        "Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f));

    syncGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "repeats", 1 },
        "Repeats",
        1, 16, 4));

    syncGroup->addChild(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "stutter_gate", 1 },
        "Stutter Gate",
        false));

    layout.add(std::move(syncGroup));

    // === Spread & Pitch ===
    auto spreadGroup = std::make_unique<juce::AudioProcessorParameterGroup>("spread", "Spread & Pitch", "|");

    spreadGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "spread", 1 },
        "Spread",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    spreadGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "pitch_mode", 1 },
        "Pitch Mode",
        juce::StringArray { "Random", "Ladder Up", "Ladder Down", "Pendulum" },
        0));

    spreadGroup->addChild(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "freeze", 1 },
        "Freeze",
        false));

    layout.add(std::move(spreadGroup));

    // === Euclidean Rhythms ===
    auto euclideanGroup = std::make_unique<juce::AudioProcessorParameterGroup>("euclidean", "Euclidean", "|");

    euclideanGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "euclidean_pulses", 1 },
        "Euclidean Pulses",
        1, 16, 4));

    euclideanGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "euclidean_steps", 1 },
        "Euclidean Steps",
        2, 16, 8));

    euclideanGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "euclidean_rotation", 1 },
        "Euclidean Rotation",
        0, 15, 0));

    euclideanGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "euclidean_swing", 1 },
        "Swing",
        juce::NormalisableRange<float>(50.0f, 75.0f, 0.1f),
        50.0f));

    layout.add(std::move(euclideanGroup));

    // === Spatial Audio ===
    auto spatialGroup = std::make_unique<juce::AudioProcessorParameterGroup>("spatial", "Spatial", "|");

    spatialGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "spatial_mode", 1 },
        "Spatial Mode",
        juce::StringArray { "Off", "Scatter", "Trajectory" },
        0));

    spatialGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "azimuth", 1 },
        "Azimuth",
        juce::NormalisableRange<float>(0.0f, 360.0f, 0.1f),
        0.0f));

    spatialGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "elevation", 1 },
        "Elevation",
        juce::NormalisableRange<float>(-90.0f, 90.0f, 0.1f),
        0.0f));

    spatialGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "az_spread", 1 },
        "Az. Spread",
        juce::NormalisableRange<float>(0.0f, 360.0f, 0.1f),
        90.0f));

    spatialGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "el_spread", 1 },
        "El. Spread",
        juce::NormalisableRange<float>(0.0f, 180.0f, 0.1f),
        45.0f));

    spatialGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "distance", 1 },
        "Distance",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f));

    spatialGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "spatial_width", 1 },
        "Spatial Width",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f));

    spatialGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "trajectory", 1 },
        "Trajectory",
        juce::StringArray { "Static", "Orbital", "Spiral", "Random" },
        0));

    spatialGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "traj_speed", 1 },
        "Trajectory Speed",
        juce::NormalisableRange<float>(0.0f, 400.0f, 0.1f),
        100.0f));

    spatialGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "dist_lpf", 1 },
        "Distance LPF",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f));

    spatialGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "doppler", 1 },
        "Doppler",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    spatialGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "spatial_smooth", 1 },
        "Spatial Smooth",
        juce::NormalisableRange<float>(1.0f, 200.0f, 0.1f, 0.4f),
        5.0f));

    layout.add(std::move(spatialGroup));

    return layout;
}

void GrainScatterProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    delayBuffer.prepare(sampleRate, 2.0f);
    scheduler.prepare(sampleRate);
    tempoTracker.prepare(sampleRate);

    int maxFreezeSamples = static_cast<int>(sampleRate * 2.0) + 1;
    freezeManager.prepare(sampleRate, maxFreezeSamples);

    dryWetSmoothed.reset(sampleRate, 0.02);
    feedbackSmoothed.reset(sampleRate, 0.02);

    feedbackL = 0.0f;
    feedbackR = 0.0f;

    spawnRequests.reserve(128);

    // Initialize Euclidean pattern
    int steps = static_cast<int>(euclideanStepsParam->load());
    int pulses = static_cast<int>(euclideanPulsesParam->load());
    euclideanPattern = EuclideanGenerator::generate(steps, pulses);
    cachedEuclideanSteps = steps;
    cachedEuclideanPulses = pulses;

    wasFrozen = false;
    cachedScaleIndex = -1;
    cachedPitchMode = -1;

    // Spatial audio — size to actual block size, not arbitrary formula
    hoaBus.setSize (kHOA3Channels, samplesPerBlock, false, true, false);
    binauralDecoder.prepare (sampleRate, samplesPerBlock);
    binauralL.resize (static_cast<size_t> (samplesPerBlock), 0.0f);
    binauralR.resize (static_cast<size_t> (samplesPerBlock), 0.0f);
    grainPool.clearVoices();   // IN-05: no stale grains after a rate/block-size change
    grainPool.prepareSpatial (static_cast<float> (sampleRate));
    distanceLpfState[0] = 0.0f;
    distanceLpfState[1] = 0.0f;

    // WR-08: prime the distance-LPF coefficient smoother to the current param state so
    // the first spatial block doesn't ramp from a cold value.
    {
        float d0    = distanceParam->load() / 100.0f;
        float amt0  = distLpfParam->load() / 100.0f;
        float cut0  = 20000.0f - d0 * 15000.0f * amt0;
        float coeff0 = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cut0
                                        / static_cast<float> (sampleRate));
        lpfCoeffSmoothed.reset (sampleRate, 0.02);
        lpfCoeffSmoothed.setCurrentAndTargetValue (coeff0);
    }
}

void GrainScatterProcessor::releaseResources() {}

void GrainScatterProcessor::reset()
{
    // Kill all active grains (IN-05: shared clearVoices() path)
    grainPool.clearVoices();

    // Clear delay buffer contents (CR-02: alloc-free — reset() may run on the RT thread)
    delayBuffer.clear();

    // Reset feedback accumulators
    feedbackL = 0.0f;
    feedbackR = 0.0f;

    // Snap SmoothedValues to current targets (avoid ramp after jump)
    dryWetSmoothed.setCurrentAndTargetValue (dryWetSmoothed.getTargetValue());
    feedbackSmoothed.setCurrentAndTargetValue (feedbackSmoothed.getTargetValue());
    lpfCoeffSmoothed.setCurrentAndTargetValue (lpfCoeffSmoothed.getTargetValue());

    // Reset freeze state (CR-02: clear() zeroes the already-sized buffer, no setSize)
    freezeManager.clear();
    wasFrozen = false;

    // Reset scheduler timing
    scheduler.prepare (currentSampleRate);

    // IN-11: reset the standalone tempo counter so manualPpq doesn't keep advancing
    // across a transport stop/seek (completeness gap in "clear all DSP state").
    tempoTracker.prepare (currentSampleRate);

    // Clear distance LPF state
    distanceLpfState[0] = 0.0f;
    distanceLpfState[1] = 0.0f;

    // Clear HOA bus
    hoaBus.clear();
    std::fill (binauralL.begin(), binauralL.end(), 0.0f);
    std::fill (binauralR.begin(), binauralR.end(), 0.0f);
}

bool GrainScatterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void GrainScatterProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    int numSamples = buffer.getNumSamples();

    // WR-07: hoaBus / binauralL / binauralR are sized to samplesPerBlock. If a host ever
    // hands us a block larger than the declared maximum, every i < numSamples write into
    // those buffers would run past the allocation (this is the exact class v2.0.2 fixed).
    // Clamp defensively — the JUCE contract says this can't happen, but not all hosts honor it.
    jassert (numSamples <= hoaBus.getNumSamples());
    numSamples = juce::jmin (numSamples, hoaBus.getNumSamples());

    // Read parameters atomically
    float grainSizeMs    = grainSizeParam->load();
    float density        = densityParam->load();
    float pitchRandom    = pitchRandomParam->load();
    float panRandom      = panRandomParam->load();
    int   scaleIndex     = static_cast<int>(scaleParam->load());
    int   rootNote       = static_cast<int>(rootNoteParam->load());
    float reverseProb    = reverseParam->load();
    float feedbackAmt    = feedbackParam->load() / 100.0f;
    float dryWetMix      = dryWetParam->load() / 100.0f;
    int   syncMode       = static_cast<int>(syncModeParam->load());
    float probability    = probabilityParam->load();
    int   repeats        = static_cast<int>(repeatsParam->load());
    float spread         = spreadParam->load();
    int   pitchMode      = static_cast<int>(pitchModeParam->load());
    bool  freeze         = freezeParam->load() > 0.5f;
    int   eucSteps       = static_cast<int>(euclideanStepsParam->load());
    int   eucPulses      = static_cast<int>(euclideanPulsesParam->load());
    int   eucRotation    = static_cast<int>(euclideanRotationParam->load());
    float eucSwing       = euclideanSwingParam->load();
    bool  stutterGate    = stutterGateParam->load() > 0.5f;
    float sizeRandom     = sizeRandomParam->load() / 100.0f;
    float ampRandom      = ampRandomParam->load() / 100.0f;
    float scanPosition   = scanPositionParam->load() / 100.0f;
    int   grainShapeIdx  = static_cast<int>(grainShapeParam->load());

    // Spatial parameters
    int   spatialMode    = static_cast<int>(spatialModeParam->load());
    float azimuthDeg     = azimuthParam->load();
    float elevationDeg   = elevationParam->load();
    float azSpreadDeg    = azSpreadParam->load();
    float elSpreadDeg    = elSpreadParam->load();
    float distanceNorm   = distanceParam->load() / 100.0f;
    float spatialWidth   = spatialWidthParam->load() / 100.0f;
    int   trajectoryType = static_cast<int>(trajectoryParam->load());
    float trajSpeed      = trajSpeedParam->load() / 100.0f;   // 0-4.0 multiplier
    float distLpfAmt     = distLpfParam->load() / 100.0f;     // 0-1.0
    float dopplerAmt     = dopplerParam->load() / 100.0f;     // 0-1.0
    float spatialSmooth  = spatialSmoothParam->load();         // 1-200 ms

    // Convert degrees to radians for ambisonics math
    float azimuthRad     = juce::degreesToRadians (azimuthDeg);
    float elevationRad   = juce::degreesToRadians (elevationDeg);
    float azSpreadRad    = juce::degreesToRadians (azSpreadDeg);
    float elSpreadRad    = juce::degreesToRadians (elSpreadDeg);

    // Feedback shaping constants:
    // Drive pushes signal into tanh saturation curve for warm distortion
    // tanhCompensation corrects tanh's gain reduction near zero (tanh(x)/x ≈ 0.9950 at x=0.3)
    // stabilityMargin ensures feedback decays to silence (gain < 1.0)
    static constexpr float kFeedbackDrive = 3.0f;
    static constexpr float kTanhCompensation = 1.00497f;
    static constexpr float kStabilityMargin = 0.95f;

    // WR-06: recompute the 16 SH target coefficients only every N samples (control rate).
    // The one-pole SH smoothing interpolates between updates, so per-active-voice encodeSH16
    // trig no longer runs every sample. Trajectory position + Doppler stay per-sample.
    static constexpr int kTrajUpdateInterval = 16;

    int grainSizeSamples = juce::jmax(1, static_cast<int>(grainSizeMs * currentSampleRate / 1000.0));

    // Update Euclidean pattern on parameter change
    if (eucSteps != cachedEuclideanSteps.load (std::memory_order_relaxed)
        || eucPulses != cachedEuclideanPulses.load (std::memory_order_relaxed))
    {
        euclideanPattern = EuclideanGenerator::generate(eucSteps, eucPulses);
        cachedEuclideanSteps.store (eucSteps, std::memory_order_relaxed);
        cachedEuclideanPulses.store (eucPulses, std::memory_order_relaxed);
        scheduler.resetEuclideanStep(eucSteps);
    }

    // Detect scale/pitch mode change → reset ladder
    if (scaleIndex != cachedScaleIndex || pitchMode != cachedPitchMode)
    {
        scaleQuantizer.resetLadder();
        cachedScaleIndex = scaleIndex;
        cachedPitchMode = pitchMode;
    }

    // Freeze engage/release (with crossfade on both transitions)
    // Capture full delay buffer so scan_position can sweep through it
    if (freeze && !wasFrozen)
        freezeManager.engage(delayBuffer, static_cast<int>(currentSampleRate * 2.0));
    else if (!freeze && wasFrozen)
        freezeManager.release();  // Starts release crossfade; active stays true until crossfade completes
    wasFrozen = freeze;

    // Set SmoothedValue targets (never reset() here!)
    dryWetSmoothed.setTargetValue(dryWetMix);
    feedbackSmoothed.setTargetValue(feedbackAmt);

    // Get spawn requests
    spawnRequests.clear();
    int stutterGateStart = -1;
    int stutterGateEnd = -1;

    SyncInfo syncInfo = tempoTracker.update(getPlayHead(), numSamples);

    if (syncMode == 0)  // Free mode
    {
        scheduler.processBlockFree(numSamples, density, probability, spawnRequests);
    }
    else  // Sync mode
    {
        scheduler.processBlockSync(numSamples, syncInfo, syncMode, probability,
                                   repeats, euclideanPattern, eucSteps, eucRotation,
                                   eucSwing, stutterGate,
                                   spawnRequests, stutterGateStart, stutterGateEnd);
    }

    // Sort spawn requests by sample offset for correct ordering
    std::sort(spawnRequests.begin(), spawnRequests.end(),
              [](const SpawnRequest& a, const SpawnRequest& b) {
                  return a.sampleOffset < b.sampleOffset;
              });

    // Set grain envelope shape
    grainPool.setGrainShape(grainShapeIdx);

    // Update spatial smoothing time once per block
    if (spatialMode > 0)
        grainPool.setSpatialSmoothTime(spatialSmooth);

    auto* inL = buffer.getReadPointer(0);
    auto* inR = buffer.getReadPointer(1);
    auto* outBufL = buffer.getWritePointer(0);
    auto* outBufR = buffer.getWritePointer(1);

    // IN-09: cache the HOA write pointers once per block (spatial mode writes every
    // channel every sample) — avoids a getWritePointer()/bounds-check per setSample call.
    float* hoaWrite[kHOA3Channels];
    for (int ch = 0; ch < kHOA3Channels; ++ch)
        hoaWrite[ch] = hoaBus.getWritePointer (ch);

    size_t spawnIdx = 0;

    for (int i = 0; i < numSamples; ++i)
    {
        float inputL = inL[i];
        float inputR = inR[i];

        // Push input + feedback to delay buffer
        delayBuffer.pushSample(inputL + feedbackL, inputR + feedbackR);

        // Freeze crossfade
        freezeManager.advanceCrossfade();

        // Check for grain spawns at this sample
        while (spawnIdx < spawnRequests.size() && spawnRequests[spawnIdx].sampleOffset == i)
        {
            GrainParams gp;

            // Grain size with optional randomization
            int actualGrainSize = grainSizeSamples;
            if (sizeRandom > 0.0f)
                actualGrainSize = juce::jmax(1, static_cast<int>(grainSizeSamples
                    * (1.0f + grainRng.nextFloat() * sizeRandom)));
            gp.grainLengthSamples = actualGrainSize;

            // Per-grain amplitude randomization
            if (ampRandom > 0.0f)
                gp.amplitude = 1.0f - grainRng.nextFloat() * ampRandom;

            // Position offset: scan position + spread scatter
            float scanPositionSamples = scanPosition * static_cast<float>(currentSampleRate * 2.0);
            float spreadOffset = (spread / 100.0f) * (grainRng.nextFloat() * 2.0f - 1.0f)
                                 * static_cast<float>(grainSizeSamples);
            gp.positionInSamples = scanPositionSamples + spreadOffset;

            // Pitch
            gp.playbackRate = scaleQuantizer.getNextPitch(pitchMode, scaleIndex, rootNote,
                                                          pitchRandom, grainRng);

            // Pan randomization (used in stereo mode; ignored in spatial mode)
            gp.panPosition = 0.5f + (panRandom / 100.0f) * (grainRng.nextFloat() - 0.5f);

            // Reverse probability
            gp.reverse = grainRng.nextFloat() < (reverseProb / 100.0f);

            // Freeze: new grains read from frozen buffer (but not during release crossfade)
            gp.readFromFrozen = freezeManager.isActive() && !freezeManager.isReleasing();

            // Spatial parameters for scatter cloud
            if (spatialMode > 0)
            {
                gp.azimuth = azimuthRad + azSpreadRad * (grainRng.nextFloat() - 0.5f) * spatialWidth;
                gp.elevation = elevationRad + elSpreadRad * (grainRng.nextFloat() - 0.5f) * spatialWidth;
                gp.distance = distanceNorm;
                gp.trajectoryType = (spatialMode == 2) ? trajectoryType : 0;  // Trajectory only in mode 2
            }

            grainPool.spawnGrain(gp);
            ++spawnIdx;
        }

        float wetL = 0.0f, wetR = 0.0f;

        if (spatialMode == 0)
        {
            // === STEREO PATH (original, backwards compatible) ===
            grainPool.processSample(delayBuffer, freezeManager, wetL, wetR);
        }
        else
        {

            // Update grain trajectories before processing
            auto& poolVoices = grainPool.getVoices();
            const bool updateShTarget = (i % kTrajUpdateInterval == 0);  // WR-06 control rate
            for (auto& v : poolVoices)
            {
                if (!v.active)
                    continue;

                if (v.trajectoryType != GrainTrajectory::Static)
                {
                    float t = 1.0f - static_cast<float>(v.samplesRemaining) / static_cast<float>(v.grainLengthSamples);
                    auto [az, el] = GrainTrajectory::update(v.trajectoryType, v.azimuth, v.elevation,
                                                             t, spatialWidth, trajSpeed, v.rngState);

                    // Doppler: pitch shift from angular velocity
                    if (dopplerAmt > 0.001f)
                    {
                        float prevAz = v.shCoeffs.current[1];  // Y-component tracks sin(az)*cos(el)
                        float newY = std::sin(az) * std::cos(el);
                        float dY = newY - prevAz;
                        // Scale: 50x gives ~1 semitone at moderate movement, clamped to ±1 octave
                        v.dopplerFactor = juce::jlimit(0.5f, 2.0f, 1.0f + dopplerAmt * dY * 50.0f);
                    }
                    else
                    {
                        v.dopplerFactor = 1.0f;
                    }

                    // WR-06: encodeSH16 (16-coeff trig) only at control rate; the SH one-pole
                    // smoother interpolates the current[] coeffs toward this target every sample.
                    if (updateShTarget)
                        v.shCoeffs.setTarget(az, el);
                }
                else
                {
                    v.dopplerFactor = 1.0f;
                }
            }

            // Process into HOA bus (single-sample slice)
            float hoaSample[kHOA3Channels] = {};
            grainPool.processSampleSpatial(delayBuffer, freezeManager, hoaSample);

            // Store into HOA bus for this sample (IN-09: cached write pointers)
            for (int ch = 0; ch < kHOA3Channels; ++ch)
                hoaWrite[ch][i] = hoaSample[ch];

            // WR-01 + WR-05: per-sample feedback recursion in spatial mode. The delay write
            // at the top of this loop consumes feedbackL/R every sample; derive them HERE
            // from the per-sample HOA omni (W) channel — the mono grain sum available this
            // sample — so the recursion is genuinely per-sample. (Previously feedbackL/R were
            // updated only in the post-decode loop, one block late, so every input sample of
            // the block was fed the SAME held constant → DC offset + block-rate buzz.) The
            // feedback is mono; the grains re-spatialize it on the next pass. The isfinite
            // flush stops a single non-finite sample from latching permanent silence/noise.
            {
                float fbAmount = feedbackSmoothed.getNextValue();
                float fb = std::tanh (hoaSample[0] * fbAmount * kFeedbackDrive)
                           * kTanhCompensation * kStabilityMargin;
                if (! std::isfinite (fb)) fb = 0.0f;
                feedbackL = fb;
                feedbackR = fb;
            }
        }

        if (spatialMode == 0)
        {
            // Soft-clip wet output
            wetL = std::tanh(wetL);
            wetR = std::tanh(wetR);

            // Feedback
            float fbAmount = feedbackSmoothed.getNextValue();
            float rawFbL = wetL * fbAmount;
            float rawFbR = wetR * fbAmount;
            feedbackL = std::tanh(rawFbL * kFeedbackDrive) * kTanhCompensation * kStabilityMargin;
            feedbackR = std::tanh(rawFbR * kFeedbackDrive) * kTanhCompensation * kStabilityMargin;
            if (! std::isfinite (feedbackL)) feedbackL = 0.0f;   // WR-05: no sticky NaN in the loop
            if (! std::isfinite (feedbackR)) feedbackR = 0.0f;

            // Dry/wet mix
            float mix = dryWetSmoothed.getNextValue();
            float dryL = inputL;
            float dryR = inputR;

            if (stutterGate && stutterGateStart >= 0 && i >= stutterGateStart && i < stutterGateEnd)
            {
                dryL = 0.0f;
                dryR = 0.0f;
            }

            outBufL[i] = dryL * (1.0f - mix) + wetL * mix;
            outBufR[i] = dryR * (1.0f - mix) + wetR * mix;
        }
        // (Spatial mode: SmoothedValues consumed in post-processing loop below)
    }

    // === SPATIAL POST-PROCESSING: binaural decode HOA bus to stereo ===
    if (spatialMode > 0)
    {
        // Binaural decode: 16ch HOA3 -> stereo
        constexpr int numHoaChannels = kHOA3Channels;
        const float* hoaChannels[numHoaChannels];
        for (int ch = 0; ch < numHoaChannels; ++ch)
            hoaChannels[ch] = hoaBus.getReadPointer(ch);

        binauralDecoder.process(hoaChannels, numSamples, binauralL.data(), binauralR.data());

        // Distance LPF (1-pole low-pass for air absorption)
        // distLpfAmt scales how much distance affects the cutoff (0=no darkening, 1=full)
        // WR-08: smooth the coefficient per sample so automating Distance / Distance LPF
        // ramps the cutoff instead of stepping it discontinuously (zipper).
        float lpfCutoff = 20000.0f - distanceNorm * 15000.0f * distLpfAmt;
        float lpfCoeffTarget = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * lpfCutoff
                                               / static_cast<float>(currentSampleRate));
        lpfCoeffSmoothed.setTargetValue(lpfCoeffTarget);

        // Apply dry/wet mix with binaural output
        for (int i = 0; i < numSamples; ++i)
        {
            // LPF (WR-08: per-sample smoothed coefficient)
            float lpfCoeff = lpfCoeffSmoothed.getNextValue();
            distanceLpfState[0] += lpfCoeff * (binauralL[i] - distanceLpfState[0]);
            distanceLpfState[1] += lpfCoeff * (binauralR[i] - distanceLpfState[1]);
            if (! std::isfinite (distanceLpfState[0])) distanceLpfState[0] = 0.0f;   // WR-05
            if (! std::isfinite (distanceLpfState[1])) distanceLpfState[1] = 0.0f;

            float wetBinL = std::tanh(distanceLpfState[0]);
            float wetBinR = std::tanh(distanceLpfState[1]);

            // WR-01: spatial feedback is now derived per-sample in the main loop (from the
            // HOA omni channel), not here — the post-decode value was one block late.

            float mix = dryWetSmoothed.getNextValue();
            float dryL = inL[i];
            float dryR = inR[i];

            if (stutterGate && stutterGateStart >= 0 && i >= stutterGateStart && i < stutterGateEnd)
            {
                dryL = 0.0f;
                dryR = 0.0f;
            }

            outBufL[i] = dryL * (1.0f - mix) + wetBinL * mix;
            outBufR[i] = dryR * (1.0f - mix) + wetBinR * mix;
        }
    }

    // Populate visualization snapshot (lock-free triple buffer)
    {
        auto& snap = vizBuffer.getWriteBuffer();
        const auto& poolVoices = grainPool.getVoices();
        int activeCount = 0;

        float maxDelaySamples = static_cast<float> (currentSampleRate * 2.0);  // 2s buffer

        for (int v = 0; v < GrainPool::MaxVoices; ++v)
        {
            const auto& gv = poolVoices[static_cast<size_t> (v)];
            auto& sv = snap.voices[static_cast<size_t> (v)];

            sv.active = gv.active;
            if (gv.active)
            {
                sv.positionNorm = (maxDelaySamples > 0.0f)
                    ? gv.positionOffset / maxDelaySamples
                    : 0.0f;
                sv.pitchSemitones = 12.0f * std::log2 (std::max (gv.playbackRate, 0.001f));
                sv.pan = gv.panPosition;
                float vizPhase = (gv.grainLengthSamples > 0)
                    ? 1.0f - static_cast<float> (gv.samplesRemaining) / static_cast<float> (gv.grainLengthSamples)
                    : 0.0f;
                sv.envelope = GrainPool::computeEnvelope (vizPhase, grainShapeIdx);
                sv.reverse = gv.reverse;
                sv.frozen = gv.readFromFrozen;
                sv.azimuth = gv.azimuth;
                sv.elevation = gv.elevation;
                sv.distance = gv.distance;
                ++activeCount;
            }
        }

        snap.activeCount = activeCount;

        // Copy euclidean state into snapshot (avoids cross-thread reference)
        snap.euclideanPattern = euclideanPattern;
        snap.euclideanSteps = cachedEuclideanSteps.load (std::memory_order_relaxed);
        snap.euclideanStep = scheduler.getEuclideanStep();
        snap.euclideanRotation = eucRotation;

        vizBuffer.publish();
    }
}

juce::AudioProcessorEditor* GrainScatterProcessor::createEditor()
{
    return new GrainScatterEditor(*this);
}

bool GrainScatterProcessor::hasEditor() const { return true; }

const juce::String GrainScatterProcessor::getName() const { return JucePlugin_Name; }
bool GrainScatterProcessor::acceptsMidi() const { return false; }
bool GrainScatterProcessor::producesMidi() const { return false; }
bool GrainScatterProcessor::isMidiEffect() const { return false; }
double GrainScatterProcessor::getTailLengthSeconds() const { return 2.0; }

int GrainScatterProcessor::getNumPrograms() { return 1; }
int GrainScatterProcessor::getCurrentProgram() { return 0; }
void GrainScatterProcessor::setCurrentProgram(int) {}
const juce::String GrainScatterProcessor::getProgramName(int) { return {}; }
void GrainScatterProcessor::changeProgramName(int, const juce::String&) {}

void GrainScatterProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GrainScatterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrainScatterProcessor();
}
