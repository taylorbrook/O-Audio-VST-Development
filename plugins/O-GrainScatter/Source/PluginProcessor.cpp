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
// PluginEditor.h is deliberately NOT included at the top of this TU — the
// include lives inside the #if JUCE_WEB_BROWSER guard directly above
// createEditor(), so a console target that compiles this TU with
// JUCE_WEB_BROWSER=0 and no editor sources (scripts/param-dump) links.

GrainScatterProcessor::GrainScatterProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // v3.1.0: a fresh instance gets a random seed; a loaded session restores its own.
    rngSeed.store (static_cast<uint64_t> (juce::Random::getSystemRandom().nextInt64()),
                   std::memory_order_relaxed);

    grainSizeParam       = parameters.getRawParameterValue("grain_size");
    densityParam         = parameters.getRawParameterValue("density");
    pitchRandomParam     = parameters.getRawParameterValue("pitch_random");
    panRandomParam       = parameters.getRawParameterValue("pan_random");
    scaleParam           = parameters.getRawParameterValue("scale");
    rootNoteParam        = parameters.getRawParameterValue("root_note");
    reverseParam         = parameters.getRawParameterValue("reverse");
    feedbackParam        = parameters.getRawParameterValue("feedback");
    dryLevelParam        = parameters.getRawParameterValue("dry_level");
    wetLevelParam        = parameters.getRawParameterValue("wet_level");
    pitchRangeParam      = parameters.getRawParameterValue("pitch_range");
    pitchTransposeParam  = parameters.getRawParameterValue("pitch_transpose");
    pitchFineParam       = parameters.getRawParameterValue("pitch_fine");
    pitchDirectionParam  = parameters.getRawParameterValue("pitch_direction");
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

    // v3.0.0: build the built-in collection tables HERE, on the constructing
    // thread — their function-local static must never first-run on the audio
    // thread (std::log2 + a guarded static init).
    (void) PitchCollections::builtIns();
    resetPitchCollectionState();
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

    // v3.0.0: 5 -> 32 collections. Indices 0-4 are unchanged, so a saved
    // session's choice index still names the same scale; the list only grows.
    coreGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "scale", 1 },
        "Scale",
        PitchCollections::names(),
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

    // v3.0.0: separate Dry and Wet levels replace the single dry_wet crossfade.
    // -60 dB is the floor and means silence (-inf). Interval 0 so the 0 dB
    // default is exact (a stepped range with a negative start snaps it to a
    // few e-06 off). Skew 3 puts 0 dB at ~75 % of the knob's travel.
    const auto levelText = [] (float v, int) {
        return v <= -59.95f ? juce::String ("-inf dB") : juce::String (v, 1) + " dB";
    };
    coreGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "dry_level", 3 },
        "Dry Level",
        juce::NormalisableRange<float>(-60.0f, 6.0f, 0.0f, 3.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (levelText)));

    coreGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wet_level", 3 },
        "Wet Level",
        juce::NormalisableRange<float>(-60.0f, 6.0f, 0.0f, 3.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (levelText)));

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

    // v3.0.0 pitch controls. Range is the span Pitch Rnd scales (12 st = the
    // v2 behaviour); Transpose and Fine shift every grain, Pitch Rnd or not.
    spreadGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitch_range", 3 },
        "Pitch Range",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.0f),
        12.0f,
        juce::AudioParameterFloatAttributes().withLabel ("st")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2); })));

    spreadGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "pitch_transpose", 3 },
        "Transpose",
        -24, 24, 0,
        juce::AudioParameterIntAttributes().withLabel ("st")));

    spreadGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitch_fine", 3 },
        "Fine Tune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ct")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); })));

    spreadGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "pitch_direction", 3 },
        "Pitch Direction",
        juce::StringArray { "Up + Down", "Up", "Down" },
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

    // v3.0.0: 2 s -> 12 s. Scan still sweeps the newest 2 s; the rest is
    // headroom for the spawn pre-roll a pitched-up forward grain needs
    // ((rate-1) x length, up to 7 s at +36 st on a 1 s grain) and for a
    // reverse grain's reach (length x rate). See the spawn block.
    delayBuffer.prepare(sampleRate, 12.0f);
    scheduler.prepare(sampleRate);
    tempoTracker.prepare(sampleRate);

    int maxFreezeSamples = static_cast<int>(sampleRate * 2.0) + 1;
    freezeManager.prepare(sampleRate, maxFreezeSamples);

    dryGainSmoothed.reset(sampleRate, 0.02);
    wetGainSmoothed.reset(sampleRate, 0.02);
    dryGainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (dryLevelParam->load(), -60.0f));
    wetGainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (wetLevelParam->load(), -60.0f));
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
    pitchEngine.invalidate();

    // Spatial audio — size to actual block size, not arbitrary formula
    hoaBus.setSize (kHOA3Channels, samplesPerBlock, false, true, false);
    binauralDecoder.prepare (sampleRate, samplesPerBlock);
    binauralL.resize (static_cast<size_t> (samplesPerBlock), 0.0f);
    binauralR.resize (static_cast<size_t> (samplesPerBlock), 0.0f);
    dryGateMask.resize (static_cast<size_t> (samplesPerBlock), 1.0f);
    grainCounter = 0;
    samplesProcessed = 0;
    stutterGateEndAbs = -1;
    grainPool.clearVoices();   // IN-05: no stale grains after a rate/block-size change
    grainPool.prepareSpatial (static_cast<float> (sampleRate));
    grainPool.setSampleRate (sampleRate);   // v3.0.5 steal fade length
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
    dryGainSmoothed.setCurrentAndTargetValue (dryGainSmoothed.getTargetValue());
    wetGainSmoothed.setCurrentAndTargetValue (wetGainSmoothed.getTargetValue());
    feedbackSmoothed.setCurrentAndTargetValue (feedbackSmoothed.getTargetValue());
    lpfCoeffSmoothed.setCurrentAndTargetValue (lpfCoeffSmoothed.getTargetValue());

    // Reset freeze state (CR-02: clear() zeroes the already-sized buffer, no setSize)
    freezeManager.clear();
    wasFrozen = false;

    // Reset scheduler timing (and, v3.1.0, the RNG event counters + sample clock)
    scheduler.prepare (currentSampleRate);
    grainCounter = 0;
    samplesProcessed = 0;
    stutterGateEndAbs = -1;

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
    float dryGain        = juce::Decibels::decibelsToGain (dryLevelParam->load(), -60.0f);
    float wetGain        = juce::Decibels::decibelsToGain (wetLevelParam->load(), -60.0f);
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

    // v3.0.0 pitch settings for every grain spawned this block. The engine
    // rebuilds its active-pitch table only when one of these moves, and
    // restarts a ladder when the collection or mode changes.
    if (customVersion.load (std::memory_order_acquire) != audioCustomVersion)
    {
        const juce::SpinLock::ScopedTryLockType tl (customLock);
        if (tl.isLocked())   // contended: keep the previous table one more block
        {
            audioCustomTable   = customTable;
            audioCustomVersion = customVersion.load (std::memory_order_relaxed);
        }
    }

    PitchEngine::Settings pitchSettings;
    pitchSettings.collection    = juce::jlimit (0, PitchCollections::kCount - 1, scaleIndex);
    pitchSettings.rootNote      = rootNote;
    pitchSettings.mode          = pitchMode;
    pitchSettings.direction     = static_cast<int> (pitchDirectionParam->load());
    pitchSettings.amount        = pitchRandom / 100.0f;
    pitchSettings.rangeSt       = pitchRangeParam->load();
    pitchSettings.transposeSt   = pitchTransposeParam->load();
    pitchSettings.fineCents     = pitchFineParam->load();
    pitchSettings.mask          = pitchMasks[static_cast<size_t> (pitchSettings.collection)]
                                      .load (std::memory_order_relaxed);
    pitchSettings.customVersion = audioCustomVersion;

    // Freeze engage/release. Capture up to 2 s of the delay line so scan_position can
    // sweep through it. v3.0.4: capture into the slot no sounding grain still reads
    // (grains from the last hold finish on their own snapshot), and release is immediate
    // for new grains only — the grain envelopes are the crossfade both ways.
    if (freeze && !wasFrozen)
    {
        const int other = 1 - freezeManager.getCurrentSlot();
        const int slot  = (grainPool.isFrozenSlotInUse (other)
                           && ! grainPool.isFrozenSlotInUse (freezeManager.getCurrentSlot()))
                              ? freezeManager.getCurrentSlot() : other;
        freezeManager.engage (delayBuffer, slot);
    }
    else if (!freeze && wasFrozen)
        freezeManager.release();
    wasFrozen = freeze;

    // Set SmoothedValue targets (never reset() here!)
    dryGainSmoothed.setTargetValue(dryGain);
    wetGainSmoothed.setTargetValue(wetGain);
    feedbackSmoothed.setTargetValue(feedbackAmt);

    // Get spawn requests
    spawnRequests.clear();

    const uint64_t seed = rngSeed.load (std::memory_order_relaxed);
    scheduler.setSeed (seed);

    SyncInfo syncInfo = tempoTracker.update(getPlayHead(), numSamples);

    if (syncMode == 0)  // Free mode
    {
        scheduler.processBlockFree(numSamples, density, probability, spawnRequests);
    }
    else  // Sync mode
    {
        scheduler.processBlockSync(numSamples, syncInfo, syncMode, probability,
                                   repeats, euclideanPattern, eucSteps, eucRotation,
                                   eucSwing, stutterGate, spawnRequests);
    }

    // Sort spawn requests by sample offset for correct ordering. v3.1.0: ties break on
    // gateSamples, so two requests at one sample always spawn in the same order (they get
    // consecutive grain RNG counters); requests equal on both keys are identical.
    std::sort(spawnRequests.begin(), spawnRequests.end(),
              [](const SpawnRequest& a, const SpawnRequest& b) {
                  if (a.sampleOffset != b.sampleOffset)
                      return a.sampleOffset < b.sampleOffset;
                  return a.gateSamples > b.gateSamples;
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

        // Freeze: the held snapshot's virtual write head keeps moving
        freezeManager.advance();

        // Check for grain spawns at this sample
        const juce::int64 absSample = samplesProcessed + i;

        while (spawnIdx < spawnRequests.size() && spawnRequests[spawnIdx].sampleOffset == i)
        {
            GrainParams gp;

            // v3.1.0: a primary sync grain opens the stutter gate, which may run into later blocks
            if (spawnRequests[spawnIdx].gateSamples > 0)
                stutterGateEndAbs = absSample + spawnRequests[spawnIdx].gateSamples;

            // v3.1.0: all of this grain's draws, keyed by its spawn index + a slot per quantity
            const CounterRng::Draw draw (seed, CounterRng::Grain, grainCounter++);

            // Grain size with optional randomization
            int actualGrainSize = grainSizeSamples;
            if (sizeRandom > 0.0f)
                actualGrainSize = juce::jmax(1, static_cast<int>(grainSizeSamples
                    * (1.0f + draw.nextFloat (CounterRng::SlotSize) * sizeRandom)));
            gp.grainLengthSamples = actualGrainSize;

            // Per-grain amplitude randomization
            if (ampRandom > 0.0f)
                gp.amplitude = 1.0f - draw.nextFloat (CounterRng::SlotAmp) * ampRandom;

            // Position offset: scan position + spread scatter
            double scanPositionSamples = static_cast<double> (scanPosition) * currentSampleRate * 2.0;
            double spreadOffset = static_cast<double> ((spread / 100.0f) * (draw.nextFloat (CounterRng::SlotSpread) * 2.0f - 1.0f))
                                  * static_cast<double> (grainSizeSamples);
            gp.positionInSamples = scanPositionSamples + spreadOffset;

            // Pitch
            gp.playbackRate = pitchEngine.nextRate (pitchSettings, audioCustomTable, draw);

            // Pan randomization (used in stereo mode; ignored in spatial mode)
            gp.panPosition = 0.5f + (panRandom / 100.0f) * (draw.nextFloat (CounterRng::SlotPan) - 0.5f);

            // Reverse probability
            gp.reverse = draw.nextFloat (CounterRng::SlotReverse) < (reverseProb / 100.0f);

            // v3.0.0 read-head guard. A grain's delay tap is
            //   forward: pos + t(1 - rate)      reverse: pos - len + t(1 + rate)
            // so a forward grain above unity rate, a reverse grain, or a
            // negative spread offset all reached a tap below 0 — AHEAD of the
            // write head — which wrapped round the ring to its oldest audio.
            // (That is also why Scan 0 % played a 2 s echo: delay 0 is the
            // oldest slot.) Pre-roll each grain so its tap never drops below
            // kReadMargin, the Lagrange kernel's 3-sample look-ahead plus one.
            {
                constexpr double kReadMargin = 4.0;
                const double len  = static_cast<double> (gp.grainLengthSamples);
                const double rate = static_cast<double> (gp.playbackRate);
                const double minPos = gp.reverse ? (len - 1.0 + kReadMargin)
                                                 : (len * juce::jmax (0.0, rate - 1.0) + kReadMargin);
                gp.positionInSamples = juce::jmax (gp.positionInSamples, minPos);
            }

            // Freeze: new grains read the held snapshot, from where the live tap would be
            gp.readFromFrozen = freezeManager.isActive();
            if (gp.readFromFrozen)
            {
                gp.frozenSlot = freezeManager.getCurrentSlot();
                gp.frozenBase = freezeManager.getSpawnBase (gp.positionInSamples);
            }

            // Spatial parameters for scatter cloud
            if (spatialMode > 0)
            {
                gp.azimuth = azimuthRad + azSpreadRad * (draw.nextFloat (CounterRng::SlotAzimuth) - 0.5f) * spatialWidth;
                gp.elevation = elevationRad + elSpreadRad * (draw.nextFloat (CounterRng::SlotElevation) - 0.5f) * spatialWidth;
                gp.distance = distanceNorm;
                gp.trajectoryType = (spatialMode == 2) ? trajectoryType : 0;  // Trajectory only in mode 2
            }

            // v3.0.4: frozen grains get a small random detune so a long hold drifts
            // instead of locking into fixed beating (v3.1.0: its own RNG slot).
            if (gp.readFromFrozen)
            {
                static constexpr float kFreezeDetuneCents = 6.0f;
                gp.playbackRate *= std::exp2 ((draw.nextFloat (CounterRng::SlotDetune) * 2.0f - 1.0f)
                                              * kFreezeDetuneCents / 1200.0f);
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
            // WR-06 control rate. v3.1.0: on the absolute sample clock, not the block index.
            const bool updateShTarget = (absSample % kTrajUpdateInterval == 0);
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

            // Dry / wet levels (v3.0.0: independent gains)
            float dryG = dryGainSmoothed.getNextValue();
            float wetG = wetGainSmoothed.getNextValue();
            float dryL = inputL;
            float dryR = inputR;

            if (stutterGate && absSample < stutterGateEndAbs)
            {
                dryL = 0.0f;
                dryR = 0.0f;
            }

            outBufL[i] = dryL * dryG + wetL * wetG;
            outBufR[i] = dryR * dryG + wetR * wetG;
        }
        else
        {
            // Spatial mode: SmoothedValues consumed in post-processing loop below
            dryGateMask[static_cast<size_t> (i)] = (stutterGate && absSample < stutterGateEndAbs) ? 0.0f : 1.0f;
        }
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

        // Apply dry / wet levels with binaural output
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

            float dryG = dryGainSmoothed.getNextValue();
            float wetG = wetGainSmoothed.getNextValue();
            float dryL = inL[i];
            float dryR = inR[i];

            if (dryGateMask[static_cast<size_t> (i)] == 0.0f)
            {
                dryL = 0.0f;
                dryR = 0.0f;
            }

            outBufL[i] = dryL * dryG + wetBinL * wetG;
            outBufR[i] = dryR * dryG + wetBinR * wetG;
        }
    }

    samplesProcessed += numSamples;

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
                // v3.0.0: a pre-rolled (pitched-up / reverse) grain can sit past
                // the 2 s scan window; pin it to the plot's right edge.
                sv.positionNorm = (maxDelaySamples > 0.0f)
                    ? juce::jmin (1.0f, static_cast<float> (gv.positionOffset) / maxDelaySamples)
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

#if JUCE_WEB_BROWSER
#include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* GrainScatterProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new GrainScatterEditor(*this);
#else
    // The param-dump console target builds with JUCE_WEB_BROWSER=0 and no
    // editor sources. It never opens an editor; this keeps the TU linkable.
    return new juce::GenericAudioProcessorEditor(*this);
#endif
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
    // v2.5.0: the UI language rides the same tree as one more plain property.
    // Written BEFORE copyState(), which takes the snapshot that is serialised
    // and would otherwise not carry it.
    //
    // Written as a STRING ("en"/"fr") rather than the atomic's int index, so a
    // hand-inspected session file says what it means.
    parameters.state.setProperty("uiLanguage",
                                 languageCode(uiLanguage.load(std::memory_order_acquire)),
                                 nullptr);

    // v3.0.0: pitch-collection state, same mechanism. Masks are one '0'/'1'
    // string per collection joined by ',', in collection-index order.
    {
        juce::StringArray masks;
        for (int c = 0; c < PitchCollections::kCount; ++c)
            masks.add (getPitchMaskString (c));
        parameters.state.setProperty ("pitchMasks", masks.joinIntoString (","), nullptr);
        parameters.state.setProperty ("customPitches", getCustomText(), nullptr);
        parameters.state.setProperty ("customPitchesName", getCustomName(), nullptr);
    }

    // v3.1.0: the RNG seed, so a reloaded session renders the same grains. Stored as
    // an int64 (getLargeIntValue clamps anything above INT64_MAX) and read back bit-cast.
    parameters.state.setProperty ("rngSeed",
                                  static_cast<juce::int64> (rngSeed.load (std::memory_order_relaxed)),
                                  nullptr);

    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GrainScatterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        migrateLegacyState (tree, getParameters());
        parameters.replaceState(tree);
    }

    // v3.0.0: pitch-collection state. Absent (a v2 session) = every degree on
    // and the 12-EDO custom placeholder — reset FIRST so a reload into a live
    // instance cannot keep the previous session's masks.
    resetPitchCollectionState();
    {
        const juce::var masks = parameters.state.getProperty ("pitchMasks");
        if (! masks.isVoid())
        {
            juce::StringArray parts;
            parts.addTokens (masks.toString(), ",", "");
            for (int c = 0; c < juce::jmin (parts.size(), (int) PitchCollections::kCount); ++c)
                setPitchMaskString (c, parts[c]);
        }

        const juce::var custom = parameters.state.getProperty ("customPitches");
        if (! custom.isVoid() && custom.toString().isNotEmpty())
            setCustomPitchesFromText (custom.toString(),
                                      parameters.state.getProperty ("customPitchesName").toString());
    }
    pitchStateRevision.fetch_add (1, std::memory_order_release);

    // v3.1.0: RNG seed. Absent (a pre-3.1.0 session) = keep this instance's seed.
    // The XML round-trip hands it back as a string var, so parse it as text.
    {
        const juce::var seedVar = parameters.state.getProperty ("rngSeed");
        if (! seedVar.isVoid() && seedVar.toString().containsOnly ("-0123456789"))
            rngSeed.store (static_cast<uint64_t> (seedVar.toString().getLargeIntValue()),
                           std::memory_order_relaxed);
    }

    // v2.5.0: the UI language. Read AFTER replaceState(), which rebuilds the
    // whole tree.
    //
    // isVoid() is the ONLY correct guard and toString() the only correct read.
    // getStateInformation writes a STRING var, but even a bool or int written
    // there would not survive: the XML round-trip does not preserve the type,
    // because NamedValueSet::setFromXmlAttributes rebuilds every property as
    // `var (value)` over the attribute STRING
    // (critical_valuetree_xml_roundtrip_loses_type). A pre-2.5.0 session has no
    // such property at all and the default (English) stands. languageIndex()
    // clamps anything that is not "fr" to 0, so a hand-edited value degrades to
    // English rather than to a bad index.
    //
    // The editor PULLS this through the getUiLanguage native fn at page init
    // rather than being pushed from here - a push would race the WebView's load.
    const juce::var lang = parameters.state.getProperty("uiLanguage");

    if (! lang.isVoid())
        uiLanguage.store(languageIndex(lang.toString()), std::memory_order_release);
}

// ============================================================================
// v3.0.0 — legacy state migration
//
// 1. dry_wet (0-100 %, a crossfade dry*(1-m) + wet*m) becomes dry_level and
//    wet_level in dB: 20*log10(1-m) and 20*log10(m), floored at -60 (= -inf).
//    m = 50 % gives -6.02 dB each, i.e. gain 0.5 each — the same output.
// 2. ANY parameter the tree does not carry is written at its default.
//    replaceState() leaves a parameter with no PARAM child at its CURRENT
//    value, so without this a v2 session loaded into a live v3 instance would
//    inherit whatever Range / Transpose / Fine the instance already had.
// ============================================================================
void GrainScatterProcessor::migrateLegacyState (juce::ValueTree& tree,
                                                const juce::Array<juce::AudioProcessorParameter*>& params)
{
    auto findParam = [&tree] (const juce::String& id)
    {
        for (auto child : tree)
            if (child.hasType ("PARAM") && child.getProperty ("id").toString() == id)
                return child;
        return juce::ValueTree();
    };

    auto addParam = [&tree] (const juce::String& id, float value)
    {
        juce::ValueTree child ("PARAM");
        child.setProperty ("id", id, nullptr);
        child.setProperty ("value", value, nullptr);
        tree.appendChild (child, nullptr);
    };

    auto legacyMix = findParam ("dry_wet");
    if (legacyMix.isValid())
    {
        if (! findParam ("dry_level").isValid() && ! findParam ("wet_level").isValid())
        {
            const float m = juce::jlimit (0.0f, 1.0f,
                                          static_cast<float> (legacyMix.getProperty ("value")) / 100.0f);
            addParam ("dry_level", juce::Decibels::gainToDecibels (1.0f - m, -60.0f));
            addParam ("wet_level", juce::Decibels::gainToDecibels (m, -60.0f));
        }
        tree.removeChild (legacyMix, nullptr);
    }

    for (auto* p : params)
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! findParam (ranged->getParameterID()).isValid())
                addParam (ranged->getParameterID(),
                          ranged->convertFrom0to1 (ranged->getDefaultValue()));
}

// ============================================================================
// v3.0.0 — pitch-collection state (message thread)
// ============================================================================
void GrainScatterProcessor::resetPitchCollectionState()
{
    for (auto& m : pitchMasks)
        m.store (~uint64_t (0), std::memory_order_relaxed);

    {
        const juce::SpinLock::ScopedLockType sl (customLock);
        customTable = PitchCollections::equalDivision (12);
        customText  = "100 200 300 400 500 600 700 800 900 1000 1100 1200";
        customName  = {};
    }
    customVersion.fetch_add (1, std::memory_order_release);
    pitchStateRevision.fetch_add (1, std::memory_order_release);
}

juce::String GrainScatterProcessor::getPitchMaskString (int collection) const
{
    if (collection < 0 || collection >= PitchCollections::kCount)
        return {};

    const int count = (collection == PitchCollections::Custom)
                          ? getCustomTable().count
                          : PitchCollections::builtIns()[static_cast<size_t> (collection)].count;
    const uint64_t mask = pitchMasks[static_cast<size_t> (collection)].load (std::memory_order_relaxed);

    juce::String bits;
    for (int d = 0; d < count; ++d)
        bits << (((mask >> d) & 1u) ? "1" : "0");
    return bits;
}

void GrainScatterProcessor::setPitchMaskString (int collection, const juce::String& bits)
{
    if (collection < 0 || collection >= PitchCollections::kCount)
        return;

    // Degrees beyond the string (or beyond 64) stay ON, so a mask saved
    // against a shorter custom table never silently mutes new degrees.
    uint64_t mask = ~uint64_t (0);
    for (int d = 0; d < juce::jmin (bits.length(), PitchCollections::kMaxPitches); ++d)
        if (bits[d] == '0')
            mask &= ~(uint64_t (1) << d);

    pitchMasks[static_cast<size_t> (collection)].store (mask, std::memory_order_relaxed);
    pitchStateRevision.fetch_add (1, std::memory_order_release);
}

juce::String GrainScatterProcessor::setCustomPitchesFromText (const juce::String& text,
                                                               const juce::String& name)
{
    PitchCollections::Table t;
    juce::String error;
    if (! PitchCollections::tableFromText (text, t, error))
        return error;

    {
        const juce::SpinLock::ScopedLockType sl (customLock);
        customTable = t;
        customText  = text.trim();
        customName  = name;
    }
    // A new table re-numbers its degrees: start from all-on.
    pitchMasks[PitchCollections::Custom].store (~uint64_t (0), std::memory_order_relaxed);
    customVersion.fetch_add (1, std::memory_order_release);
    pitchStateRevision.fetch_add (1, std::memory_order_release);
    return {};
}

juce::String GrainScatterProcessor::setCustomPitchesFromScala (const juce::String& sclBody,
                                                                const juce::String& fileName)
{
    PitchCollections::Table t;
    juce::String asText, description, error;
    if (! PitchCollections::tableFromScala (sclBody, t, asText, description, error))
        return error;

    return setCustomPitchesFromText (asText, description.isNotEmpty() ? description : fileName);
}

PitchCollections::Table GrainScatterProcessor::getCustomTable() const
{
    const juce::SpinLock::ScopedLockType sl (customLock);
    return customTable;
}

juce::String GrainScatterProcessor::getCustomText() const
{
    const juce::SpinLock::ScopedLockType sl (customLock);
    return customText;
}

juce::String GrainScatterProcessor::getCustomName() const
{
    const juce::SpinLock::ScopedLockType sl (customLock);
    return customName;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrainScatterProcessor();
}
