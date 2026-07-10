/*
  ==============================================================================

    O-Wind - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter Layout (MUST be defined before constructor)
juce::AudioProcessorValueTreeState::ParameterLayout OWindAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ========== Excitation Controls (3) ==========

    // BREATH_PRESSURE - Primary excitation amplitude
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "breathPressure", 1 },
        "Breath Pressure",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // EMBOUCHURE - Jet-to-bore ratio / lip opening shape
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "embouchure", 1 },
        "Embouchure",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // BREATH_NOISE - Turbulence / noise in air jet
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "breathNoise", 1 },
        "Breath Noise",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.15f
    ));

    // ========== Resonator Controls (4) ==========

    // TONE_COLOR - Spectral tilt / harmonic brightness
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "toneColor", 1 },
        "Tone Color",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // AIR_COLUMN - Bore length / resonance character
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "airColumn", 1 },
        "Air Column",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // JET_REFLECTION - Reflection coefficient at jet/bore junction
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "jetReflection", 1 },
        "Jet Reflection",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
        0.5f
    ));

    // END_REFLECTION - Reflection coefficient at open end
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "endReflection", 1 },
        "End Reflection",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
        0.5f
    ));

    // ========== Attack Transient (2) ==========

    // ATTACK_CHIFF - Chiff transient intensity (noise burst + pitch overshoot at note onset)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attackChiff", 1 },
        "Attack Chiff",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // HUMANIZE - Master scale for per-note randomization amounts
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "humanize", 1 },
        "Humanize",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f
    ));

    // ========== Articulation (3) ==========

    // FLUTTER_TONGUE - Flutter tongue AM depth (0 = off, 1 = full)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "flutterTongue", 1 },
        "Flutter Tongue",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // FLUTTER_RATE - Flutter tongue oscillation rate
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "flutterRate", 1 },
        "Flutter Rate",
        juce::NormalisableRange<float>(15.0f, 30.0f, 0.1f),
        22.0f,
        "Hz"
    ));

    // GROWL - Secondary LFO modulating bore feedback (vocal-fold coupling roughness)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "growl", 1 },
        "Growl",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // ========== Modulation (3) ==========

    // VIBRATO_RATE - LFO frequency for pitch vibrato
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "vibratoRate", 1 },
        "Vibrato Rate",
        juce::NormalisableRange<float>(2.0f, 8.0f, 0.01f),
        5.0f,
        "Hz"
    ));

    // VIBRATO_DEPTH - Vibrato pitch modulation depth
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "vibratoDepth", 1 },
        "Vibrato Pitch",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f
    ));

    // VIBRATO_TREMOLO - Amplitude modulation depth locked to vibrato phase
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "vibratoTremolo", 1 },
        "Vibrato Tremolo",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // VIBRATO_ONSET - Delay before vibrato ramp-in after note-on
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "vibratoOnset", 1 },
        "Vibrato Onset",
        juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f),
        300.0f,
        "ms"
    ));

    // VIBRATO_DRIFT_DEPTH - How much the vibrato rate/depth wander organically
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "vibratoDriftDepth", 1 },
        "Vibrato Drift Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // VIBRATO_DRIFT_SPEED - How fast the vibrato character evolves
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "vibratoDriftSpeed", 1 },
        "Vibrato Drift Speed",
        juce::NormalisableRange<float>(0.1f, 2.0f, 0.01f),
        0.4f,
        "Hz"
    ));

    // ========== Output (4) ==========

    // MATERIAL - Timbral macro: 0=dark wood/bamboo, 1=bright metal
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "material", 1 },
        "Material",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // WIDTH - Stereo spread
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "width", 1 },
        "Width",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        1.0f
    ));

    // FORMANT - Headjoint formant resonance prominence
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "formant", 1 },
        "Formant",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // OUTPUT_LEVEL - Master output gain
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outputLevel", 1 },
        "Output Level",
        juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // ========== Resonator — Inharmonicity (1) ==========

    // INHARMONICITY - Allpass partial detuning for conical bore approximation
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "inharmonicity", 1 },
        "Inharmonicity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.2f
    ));

    // ========== Impossible Physics (3) ==========

    // INFINITE_SUSTAIN - Reduces damping toward zero
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "infiniteSustain", 1 },
        "Infinite Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // REVERSED_JET - Inverts jet delay direction
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reversedJet", 1 },
        "Reversed Jet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // SUB_HARMONICS - Sub-octave content via nonlinear feedback
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "subHarmonics", 1 },
        "Sub-Harmonics",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // ========== ADSR Envelope (5) ==========

    // ADSR_ENABLED - Toggle ADSR amplitude envelope on/off
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "adsrEnabled", 1 },
        "ADSR Enabled",
        false
    ));

    // ADSR_ATTACK - Attack time
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "adsrAttack", 1 },
        "ADSR Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f),
        0.01f,
        "s"
    ));

    // ADSR_DECAY - Decay time
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "adsrDecay", 1 },
        "ADSR Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f),
        0.1f,
        "s"
    ));

    // ADSR_SUSTAIN - Sustain level
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "adsrSustain", 1 },
        "ADSR Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.8f
    ));

    // ADSR_RELEASE - Release time
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "adsrRelease", 1 },
        "ADSR Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f),
        0.2f,
        "s"
    ));

    // ========== Tone Hole Toggle (1) ==========

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "toneHoleToggle", 1 },
        "Tone Hole",
        false
    ));

    // ========== Instrument Preset (1) ==========

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "instrumentPreset", 1 },
        "Instrument Preset",
        0, 7, 0
    ));

    // ========== Tuning (2) ==========

    // REFERENCE_PITCH - A4 reference frequency
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "referencePitch", 1 },
        "Reference Pitch",
        juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f),
        440.0f,
        "Hz"
    ));

    // TUNING_SYSTEM - Tuning mode selection
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "tuningSystem", 1 },
        "Tuning System",
        juce::StringArray { "Scala/TUN", "MTS-ESP", "12-TET" },
        2  // Default: 12-TET (index 2)
    ));

    // ========== Effects Chain (21 params) ==========

    // Chorus
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "chorusBypass", 1 }, "Chorus Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chorusRate", 1 }, "Chorus Rate",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f), 1.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chorusDepth", 1 }, "Chorus Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chorusMix", 1 }, "Chorus Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // Delay
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "delayBypass", 1 }, "Delay Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayTime", 1 }, "Delay Time",
        juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f), 0.375f, "s"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayFeedback", 1 }, "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "delayMode", 1 }, "Delay Mode",
        juce::StringArray { "Normal", "PingPong" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayMix", 1 }, "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // EQ (3-band)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "eqBypass", 1 }, "EQ Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eqLowGain", 1 }, "EQ Low Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eqMidGain", 1 }, "EQ Mid Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eqMidFreq", 1 }, "EQ Mid Freq",
        juce::NormalisableRange<float>(200.0f, 8000.0f, 1.0f), 1000.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eqHighGain", 1 }, "EQ High Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));

    // Reverb (FDN Plate)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "reverbBypass", 1 }, "Reverb Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbSize", 1 }, "Reverb Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbDamp", 1 }, "Reverb Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbPredelay", 1 }, "Reverb Pre-delay",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 20.0f, "ms"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbMix", 1 }, "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbMod", 1 }, "Reverb Mod",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbShimmer", 1 }, "Reverb Shimmer",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    return layout;
}

//==============================================================================
OWindAudioProcessor::OWindAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-Wind")
{
    // Add placeholder sound (all notes, all channels)
    synthesiser.addSound(new FluteSynthSound());

    // Initialize tuning engine from default parameters
    tuningEngine.setMasterTune (440.0);

    // Listen for tuning parameter changes (message thread callback)
    parameters.addParameterListener ("referencePitch", this);
    parameters.addParameterListener ("tuningSystem", this);
    parameters.addParameterListener ("instrumentPreset", this);
    parameters.addParameterListener ("toneHoleToggle", this);

    // v1.14.0: Cache effects parameter pointers for real-time access
    fxCache.chorusBypass    = parameters.getRawParameterValue("chorusBypass");
    fxCache.chorusRate      = parameters.getRawParameterValue("chorusRate");
    fxCache.chorusDepth     = parameters.getRawParameterValue("chorusDepth");
    fxCache.chorusMix       = parameters.getRawParameterValue("chorusMix");
    fxCache.delayBypass     = parameters.getRawParameterValue("delayBypass");
    fxCache.delayTime       = parameters.getRawParameterValue("delayTime");
    fxCache.delayFeedback   = parameters.getRawParameterValue("delayFeedback");
    fxCache.delayMode       = parameters.getRawParameterValue("delayMode");
    fxCache.delayMix        = parameters.getRawParameterValue("delayMix");
    fxCache.eqBypass        = parameters.getRawParameterValue("eqBypass");
    fxCache.eqLowGain       = parameters.getRawParameterValue("eqLowGain");
    fxCache.eqMidGain       = parameters.getRawParameterValue("eqMidGain");
    fxCache.eqMidFreq       = parameters.getRawParameterValue("eqMidFreq");
    fxCache.eqHighGain      = parameters.getRawParameterValue("eqHighGain");
    fxCache.reverbBypass    = parameters.getRawParameterValue("reverbBypass");
    fxCache.reverbSize      = parameters.getRawParameterValue("reverbSize");
    fxCache.reverbDamp      = parameters.getRawParameterValue("reverbDamp");
    fxCache.reverbPredelay  = parameters.getRawParameterValue("reverbPredelay");
    fxCache.reverbMix       = parameters.getRawParameterValue("reverbMix");
    fxCache.reverbMod       = parameters.getRawParameterValue("reverbMod");
    fxCache.reverbShimmer   = parameters.getRawParameterValue("reverbShimmer");

    // Persist tuning-engine state (intervals, scale name, tonic, built-in
    // preset, octave stretch, mode) through DAW session state and user presets.
    // Without this the APVTS restores tuningSystem=Scala on reload but the
    // engine silently reverts to 12-TET intervals. Same pattern as O-Lyrica.
    presetManager.setCustomStateCallbacks(
        // Save callback - returns tuning state as JSON
        [this]() -> juce::var {
            auto* obj = new juce::DynamicObject();

            auto intervals = tuningEngine.getIntervals();
            juce::Array<juce::var> intervalsArray;
            for (double cents : intervals)
                intervalsArray.add(cents);
            obj->setProperty("intervals", intervalsArray);

            obj->setProperty("scaleName", tuningEngine.getActiveTuningName());
            obj->setProperty("tonic", juce::var(tuningEngine.getTonicNote()));
            obj->setProperty("presetIndex", static_cast<int>(tuningEngine.getBuiltInPreset()));
            obj->setProperty("octaveStretch", tuningEngine.getOctaveStretch());
            obj->setProperty("tuningMode", static_cast<int>(tuningEngine.getMode()));

            return juce::var(obj);
        },
        // Load callback - restores tuning state from JSON
        [this](const juce::var& customState) {
            if (!customState.isObject())
                return;

            auto* obj = customState.getDynamicObject();
            if (obj == nullptr)
                return;

            // Restore preset index first (sets intervals for built-in presets)
            if (obj->hasProperty("presetIndex"))
                tuningEngine.setBuiltInPreset(static_cast<TuningEngine::BuiltInPreset>(
                    static_cast<int>(obj->getProperty("presetIndex"))));

            // If custom intervals were saved, restore them (overrides preset)
            if (obj->hasProperty("intervals"))
            {
                auto intervalsVar = obj->getProperty("intervals");
                if (intervalsVar.isArray())
                {
                    std::vector<double> intervals;
                    for (int i = 0; i < intervalsVar.size(); ++i)
                        intervals.push_back(static_cast<double>(intervalsVar[i]));

                    juce::String name = obj->getProperty("scaleName").toString();
                    if (name.isEmpty()) name = "Custom";

                    tuningEngine.setCustomIntervals(intervals, name);
                }
            }

            // Restore tuning mode and keep the APVTS choice param in sync
            if (obj->hasProperty("tuningMode"))
            {
                int mode = static_cast<int>(obj->getProperty("tuningMode"));
                tuningEngine.setMode(static_cast<TuningEngine::Mode>(mode));

                if (auto* tuningSystemParam = parameters.getParameter("tuningSystem"))
                    tuningSystemParam->setValueNotifyingHost(static_cast<float>(mode) / 2.0f);
            }

            // Restore tonic AFTER intervals so scale rotation applies correctly
            if (obj->hasProperty("tonic"))
                tuningEngine.setTonicNote(static_cast<int>(obj->getProperty("tonic")));

            if (obj->hasProperty("octaveStretch"))
                tuningEngine.setOctaveStretch(static_cast<float>(obj->getProperty("octaveStretch")));
        });

    // Initialize factory presets
    initializeFactoryPresets();
}

OWindAudioProcessor::~OWindAudioProcessor()
{
    parameters.removeParameterListener ("referencePitch", this);
    parameters.removeParameterListener ("tuningSystem", this);
    parameters.removeParameterListener ("instrumentPreset", this);
    parameters.removeParameterListener ("toneHoleToggle", this);
}

void OWindAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "referencePitch")
    {
        tuningEngine.setMasterTune (static_cast<double> (newValue));
    }
    else if (parameterID == "tuningSystem")
    {
        int modeIndex = static_cast<int> (newValue);
        switch (modeIndex)
        {
            case 0:  tuningEngine.setMode (TuningEngine::Mode::Scala); break;
            case 1:  tuningEngine.setMode (TuningEngine::Mode::MTSESP); break;
            case 2:
            default: tuningEngine.setMode (TuningEngine::Mode::TwelveTET); break;
        }
    }
    else if (parameterID == "instrumentPreset")
    {
        // No-op: voice reads instrumentPreset directly from APVTS each block
    }
    else if (parameterID == "toneHoleToggle")
    {
        // No-op: voice reads toneHoleToggle directly from APVTS each block
    }
}

void OWindAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Clear and recreate voices (8 max polyphony)
    synthesiser.clearVoices();
    for (int i = 0; i < 8; ++i)
    {
        auto* voice = new FluteSynthVoice (&parameters, &tuningEngine);
        voice->prepareToPlay (sampleRate, samplesPerBlock);
        voice->setPendingTuningSource (&vst3Extensions.getPendingTable()); // Phase 24: NE
        synthesiser.addVoice (voice);
    }
    synthesiser.setCurrentPlaybackSampleRate (sampleRate);

    // Report oversampling latency to host (use first voice's oversampling instance)
    if (auto* firstVoice = dynamic_cast<FluteSynthVoice*> (synthesiser.getVoice (0)))
        setLatencySamples (static_cast<int> (std::ceil (firstVoice->getOversamplingLatency())));

    // Prepare stereo width processor
    stereoWidth.prepare (sampleRate, samplesPerBlock);

    // Prepare formant resonance filter (flat initially, updated in processBlock)
    formantFilterL.reset();
    formantFilterR.reset();
    lastFormantPresetIndex = -1;  // force coefficient update on first block

    // v1.14.0: Prepare effects chain
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    chorus.prepare(spec);
    chorus.reset();
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(0.0f);

    delay.prepare(spec);
    eq.prepare(spec);
    reverbProcessor.prepare(spec);
}

void OWindAudioProcessor::releaseResources()
{
    stereoWidth.reset();
    formantFilterL.reset();
    formantFilterR.reset();
    chorus.reset();
    delay.reset();
    eq.reset();
    reverbProcessor.reset();
}

void OWindAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear all channels (instrument: no input, voices add to buffer)
    buffer.clear();

    // VST3 Note Expression: drain the JUCE wrapper's raw-event queue and
    // correlate tuning deltas to their NoteOn's MIDI pitch.
    vst3Extensions.drainAndUpdate();

    // Zero-length buffer early exit
    if (buffer.getNumSamples() == 0)
        return;

    // Render all voices via synthesiser (handles MIDI routing + voice allocation)
    // Output level is applied per-voice in FluteSynthVoice::renderNextBlock
    synthesiser.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // Post-voice stereo width processing
    auto* widthParam = parameters.getRawParameterValue ("width");
    stereoWidth.processBlock (buffer, widthParam->load());

    // Post-width headjoint formant resonance filter
    {
        auto formantVal = parameters.getRawParameterValue ("formant")->load();
        int presetIdx = static_cast<int> (parameters.getRawParameterValue ("instrumentPreset")->load());
        const auto& preset = InstrumentPresets::getPreset (presetIdx);
        float centerHz = preset.formantCenterHz;
        float gainDb = formantVal * 6.0f;  // 0dB at formant=0, +6dB at formant=1

        // Update coefficients only when parameters change
        if (presetIdx != lastFormantPresetIndex || std::abs (gainDb - lastFormantGainDb) > 0.01f)
        {
            lastFormantPresetIndex = presetIdx;
            lastFormantGainDb = gainDb;
            lastFormantCenterHz = centerHz;

            // ArrayCoefficients assigns in place — Coefficients::makePeakFilter
            // heap-allocates on the audio thread
            const auto coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (
                getSampleRate(), centerHz, 1.5f,
                juce::Decibels::decibelsToGain (gainDb));
            *formantFilterL.coefficients = coeffs;
            *formantFilterR.coefficients = coeffs;
        }

        // Skip processing when gain is negligible (formant near 0)
        if (gainDb > 0.05f)
        {
            auto numSamples = buffer.getNumSamples();
            auto* left  = buffer.getWritePointer (0);
            auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

            for (int i = 0; i < numSamples; ++i)
                left[i] = formantFilterL.processSample (left[i]);

            if (right != nullptr)
                for (int i = 0; i < numSamples; ++i)
                    right[i] = formantFilterR.processSample (right[i]);
        }
    }

    // v1.14.0: Effects chain (Chorus -> Delay -> Reverb -> EQ)
    juce::dsp::AudioBlock<float> block(buffer);

    // 1. Chorus
    bool chorusBypassed = fxCache.chorusBypass->load(std::memory_order_relaxed) >= 0.5f;
    if (!chorusBypassed)
    {
        float chorusRate = fxCache.chorusRate->load(std::memory_order_relaxed);
        float chorusDepth = fxCache.chorusDepth->load(std::memory_order_relaxed);
        float chorusMix = fxCache.chorusMix->load(std::memory_order_relaxed);

        chorus.setRate(chorusRate);
        chorus.setDepth(chorusDepth);
        chorus.setMix(chorusMix);

        if (chorusMix > 0.001f)
        {
            juce::dsp::ProcessContextReplacing<float> chorusCtx(block);
            chorus.process(chorusCtx);
        }
    }

    // 2. Delay
    bool delayBypassed = fxCache.delayBypass->load(std::memory_order_relaxed) >= 0.5f;
    if (!delayBypassed)
    {
        float delayTimeSec = fxCache.delayTime->load(std::memory_order_relaxed);
        float delayFb = fxCache.delayFeedback->load(std::memory_order_relaxed);
        int delayModeVal = static_cast<int>(fxCache.delayMode->load(std::memory_order_relaxed));
        float delayMixVal = fxCache.delayMix->load(std::memory_order_relaxed);

        delay.setTime(delayTimeSec);
        delay.setFeedback(delayFb);
        delay.setMode(delayModeVal);
        delay.setMix(delayMixVal);

        if (delayMixVal > 0.001f)
            delay.process(block);
    }

    // 3. Reverb
    bool reverbBypassed = fxCache.reverbBypass->load(std::memory_order_relaxed) >= 0.5f;
    if (!reverbBypassed)
    {
        float reverbMixVal = fxCache.reverbMix->load(std::memory_order_relaxed);
        reverbProcessor.setSize(fxCache.reverbSize->load(std::memory_order_relaxed));
        reverbProcessor.setDamping(fxCache.reverbDamp->load(std::memory_order_relaxed));
        reverbProcessor.setPredelay(fxCache.reverbPredelay->load(std::memory_order_relaxed));
        reverbProcessor.setMix(reverbMixVal);
        reverbProcessor.setMod(fxCache.reverbMod->load(std::memory_order_relaxed));
        reverbProcessor.setShimmer(fxCache.reverbShimmer->load(std::memory_order_relaxed));

        if (reverbMixVal > 0.001f)
            reverbProcessor.process(block);
    }

    // 4. EQ
    bool eqBypassed = fxCache.eqBypass->load(std::memory_order_relaxed) >= 0.5f;
    if (!eqBypassed)
    {
        eq.setLowGain(fxCache.eqLowGain->load(std::memory_order_relaxed));
        eq.setMidGain(fxCache.eqMidGain->load(std::memory_order_relaxed));
        eq.setMidFreq(fxCache.eqMidFreq->load(std::memory_order_relaxed));
        eq.setHighGain(fxCache.eqHighGain->load(std::memory_order_relaxed));
        eq.process(block);
    }
}

juce::AudioProcessorEditor* OWindAudioProcessor::createEditor()
{
    return new OWindAudioProcessorEditor(*this);
}

void OWindAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void OWindAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());
}

//==============================================================================
// Factory Preset Initialization
//==============================================================================
void OWindAudioProcessor::initializeFactoryPresets()
{
    // Presets use normalized values (0-1) as stored by OuariconPresetManager
    // For parameters with non-0-1 ranges, we use the normalized form
    // e.g., outputLevel -60..12 dB: 0.0 = -60, ~0.833 = 0, 1.0 = 12

    // Helper: convert raw value to normalized for a given parameter
    auto normalize = [this](const juce::String& paramId, float rawValue) -> float {
        if (auto* param = parameters.getParameter(paramId))
            return param->convertTo0to1(rawValue);
        return rawValue;
    };

    std::vector<OuariconPresetManager::FactoryPresetDef> presets;

    // 1. Concert Flute (default, balanced)
    presets.push_back({ "Concert Flute", {
        { "breathPressure",  normalize("breathPressure", 0.5f) },
        { "embouchure",      normalize("embouchure", 0.5f) },
        { "breathNoise",     normalize("breathNoise", 0.15f) },
        { "attackChiff",     normalize("attackChiff", 0.5f) },
        { "toneColor",       normalize("toneColor", 0.5f) },
        { "airColumn",       normalize("airColumn", 0.5f) },
        { "jetReflection",   normalize("jetReflection", 0.45f) },
        { "endReflection",   normalize("endReflection", 0.55f) },
        { "vibratoRate",     normalize("vibratoRate", 5.0f) },
        { "vibratoDepth",    normalize("vibratoDepth", 0.15f) },
        { "vibratoTremolo",  normalize("vibratoTremolo", 0.15f) },
        { "vibratoOnset",    normalize("vibratoOnset", 300.0f) },
        { "vibratoDriftDepth", normalize("vibratoDriftDepth", 0.5f) },
        { "vibratoDriftSpeed", normalize("vibratoDriftSpeed", 0.4f) },
        { "width",           normalize("width", 1.0f) },
        { "formant",         normalize("formant", 0.5f) },
        { "material",        normalize("material", 0.5f) },
        { "outputLevel",     normalize("outputLevel", 0.0f) },
        { "infiniteSustain", normalize("infiniteSustain", 0.0f) },
        { "reversedJet",     normalize("reversedJet", 0.0f) },
        { "subHarmonics",    normalize("subHarmonics", 0.0f) },
        { "flutterTongue",   normalize("flutterTongue", 0.0f) },
        { "flutterRate",     normalize("flutterRate", 22.0f) },
        { "growl",           normalize("growl", 0.0f) },
        { "toneHoleToggle",  0.0f },
        { "humanize",        normalize("humanize", 0.3f) },
        { "inharmonicity",   normalize("inharmonicity", 0.2f) },
        { "referencePitch",  normalize("referencePitch", 440.0f) },
        { "tuningSystem",    normalize("tuningSystem", 2.0f) },
        { "instrumentPreset", normalize("instrumentPreset", 0.0f) },
    }, {} });

    // 2. Shakuhachi
    presets.push_back({ "Shakuhachi", {
        { "breathPressure",  normalize("breathPressure", 0.4f) },
        { "embouchure",      normalize("embouchure", 0.45f) },
        { "breathNoise",     normalize("breathNoise", 0.35f) },
        { "attackChiff",     normalize("attackChiff", 0.7f) },
        { "toneColor",       normalize("toneColor", 0.35f) },
        { "airColumn",       normalize("airColumn", 0.6f) },
        { "jetReflection",   normalize("jetReflection", 0.35f) },
        { "endReflection",   normalize("endReflection", 0.4f) },
        { "vibratoRate",     normalize("vibratoRate", 4.0f) },
        { "vibratoDepth",    normalize("vibratoDepth", 0.25f) },
        { "vibratoTremolo",  normalize("vibratoTremolo", 0.2f) },
        { "vibratoOnset",    normalize("vibratoOnset", 500.0f) },
        { "vibratoDriftDepth", normalize("vibratoDriftDepth", 0.7f) },
        { "vibratoDriftSpeed", normalize("vibratoDriftSpeed", 0.3f) },
        { "width",           normalize("width", 0.8f) },
        { "formant",         normalize("formant", 0.5f) },
        { "material",        normalize("material", 0.5f) },
        { "outputLevel",     normalize("outputLevel", 0.0f) },
        { "infiniteSustain", normalize("infiniteSustain", 0.0f) },
        { "reversedJet",     normalize("reversedJet", 0.0f) },
        { "subHarmonics",    normalize("subHarmonics", 0.1f) },
        { "flutterTongue",   normalize("flutterTongue", 0.0f) },
        { "flutterRate",     normalize("flutterRate", 22.0f) },
        { "growl",           normalize("growl", 0.0f) },
        { "toneHoleToggle",  0.0f },
        { "humanize",        normalize("humanize", 0.5f) },
        { "inharmonicity",   normalize("inharmonicity", 0.35f) },
        { "referencePitch",  normalize("referencePitch", 440.0f) },
        { "tuningSystem",    normalize("tuningSystem", 2.0f) },
        { "instrumentPreset", normalize("instrumentPreset", 1.0f) },
    }, {} });

    // 3. Bansuri (gentle tremolo)
    presets.push_back({ "Bansuri", {
        { "breathPressure",  normalize("breathPressure", 0.5f) },
        { "embouchure",      normalize("embouchure", 0.5f) },
        { "breathNoise",     normalize("breathNoise", 0.25f) },
        { "attackChiff",     normalize("attackChiff", 0.55f) },
        { "toneColor",       normalize("toneColor", 0.55f) },
        { "airColumn",       normalize("airColumn", 0.55f) },
        { "jetReflection",   normalize("jetReflection", 0.4f) },
        { "endReflection",   normalize("endReflection", 0.5f) },
        { "vibratoRate",     normalize("vibratoRate", 5.5f) },
        { "vibratoDepth",    normalize("vibratoDepth", 0.18f) },
        { "vibratoTremolo",  normalize("vibratoTremolo", 0.15f) },
        { "vibratoOnset",    normalize("vibratoOnset", 350.0f) },
        { "vibratoDriftDepth", normalize("vibratoDriftDepth", 0.6f) },
        { "vibratoDriftSpeed", normalize("vibratoDriftSpeed", 0.35f) },
        { "width",           normalize("width", 1.2f) },
        { "formant",         normalize("formant", 0.5f) },
        { "material",        normalize("material", 0.5f) },
        { "outputLevel",     normalize("outputLevel", 0.0f) },
        { "infiniteSustain", normalize("infiniteSustain", 0.0f) },
        { "reversedJet",     normalize("reversedJet", 0.0f) },
        { "subHarmonics",    normalize("subHarmonics", 0.0f) },
        { "flutterTongue",   normalize("flutterTongue", 0.0f) },
        { "flutterRate",     normalize("flutterRate", 22.0f) },
        { "growl",           normalize("growl", 0.0f) },
        { "toneHoleToggle",  1.0f },
        { "humanize",        normalize("humanize", 0.4f) },
        { "inharmonicity",   normalize("inharmonicity", 0.3f) },
        { "referencePitch",  normalize("referencePitch", 440.0f) },
        { "tuningSystem",    normalize("tuningSystem", 2.0f) },
        { "instrumentPreset", normalize("instrumentPreset", 2.0f) },
    }, {} });

    // 4. Native American Flute
    presets.push_back({ "Native Am. Flute", {
        { "breathPressure",  normalize("breathPressure", 0.3f) },
        { "embouchure",      normalize("embouchure", 0.45f) },
        { "breathNoise",     normalize("breathNoise", 0.2f) },
        { "attackChiff",     normalize("attackChiff", 0.4f) },
        { "toneColor",       normalize("toneColor", 0.3f) },
        { "airColumn",       normalize("airColumn", 0.65f) },
        { "jetReflection",   normalize("jetReflection", 0.3f) },
        { "endReflection",   normalize("endReflection", 0.35f) },
        { "vibratoRate",     normalize("vibratoRate", 3.5f) },
        { "vibratoDepth",    normalize("vibratoDepth", 0.3f) },
        { "vibratoTremolo",  normalize("vibratoTremolo", 0.25f) },
        { "vibratoOnset",    normalize("vibratoOnset", 400.0f) },
        { "vibratoDriftDepth", normalize("vibratoDriftDepth", 0.8f) },
        { "vibratoDriftSpeed", normalize("vibratoDriftSpeed", 0.25f) },
        { "width",           normalize("width", 1.4f) },
        { "formant",         normalize("formant", 0.5f) },
        { "material",        normalize("material", 0.5f) },
        { "outputLevel",     normalize("outputLevel", 0.0f) },
        { "infiniteSustain", normalize("infiniteSustain", 0.0f) },
        { "reversedJet",     normalize("reversedJet", 0.0f) },
        { "subHarmonics",    normalize("subHarmonics", 0.05f) },
        { "flutterTongue",   normalize("flutterTongue", 0.0f) },
        { "flutterRate",     normalize("flutterRate", 22.0f) },
        { "growl",           normalize("growl", 0.0f) },
        { "toneHoleToggle",  0.0f },
        { "humanize",        normalize("humanize", 0.5f) },
        { "inharmonicity",   normalize("inharmonicity", 0.35f) },
        { "referencePitch",  normalize("referencePitch", 440.0f) },
        { "tuningSystem",    normalize("tuningSystem", 2.0f) },
        { "instrumentPreset", normalize("instrumentPreset", 3.0f) },
    }, {} });

    // 5. Recorder
    presets.push_back({ "Recorder", {
        { "breathPressure",  normalize("breathPressure", 0.6f) },
        { "embouchure",      normalize("embouchure", 0.45f) },
        { "breathNoise",     normalize("breathNoise", 0.08f) },
        { "attackChiff",     normalize("attackChiff", 0.25f) },
        { "toneColor",       normalize("toneColor", 0.65f) },
        { "airColumn",       normalize("airColumn", 0.4f) },
        { "jetReflection",   normalize("jetReflection", 0.5f) },
        { "endReflection",   normalize("endReflection", 0.6f) },
        { "vibratoRate",     normalize("vibratoRate", 5.0f) },
        { "vibratoDepth",    normalize("vibratoDepth", 0.08f) },
        { "vibratoTremolo",  normalize("vibratoTremolo", 0.05f) },
        { "vibratoOnset",    normalize("vibratoOnset", 200.0f) },
        { "vibratoDriftDepth", normalize("vibratoDriftDepth", 0.2f) },
        { "vibratoDriftSpeed", normalize("vibratoDriftSpeed", 0.5f) },
        { "width",           normalize("width", 0.6f) },
        { "formant",         normalize("formant", 0.5f) },
        { "material",        normalize("material", 0.5f) },
        { "outputLevel",     normalize("outputLevel", 0.0f) },
        { "infiniteSustain", normalize("infiniteSustain", 0.0f) },
        { "reversedJet",     normalize("reversedJet", 0.0f) },
        { "subHarmonics",    normalize("subHarmonics", 0.0f) },
        { "flutterTongue",   normalize("flutterTongue", 0.0f) },
        { "flutterRate",     normalize("flutterRate", 22.0f) },
        { "growl",           normalize("growl", 0.0f) },
        { "toneHoleToggle",  1.0f },
        { "humanize",        normalize("humanize", 0.15f) },
        { "inharmonicity",   normalize("inharmonicity", 0.1f) },
        { "referencePitch",  normalize("referencePitch", 440.0f) },
        { "tuningSystem",    normalize("tuningSystem", 2.0f) },
        { "instrumentPreset", normalize("instrumentPreset", 4.0f) },
    }, {} });

    // 6. Pan Flute
    presets.push_back({ "Pan Flute", {
        { "breathPressure",  normalize("breathPressure", 0.4f) },
        { "embouchure",      normalize("embouchure", 0.48f) },
        { "breathNoise",     normalize("breathNoise", 0.4f) },
        { "attackChiff",     normalize("attackChiff", 0.65f) },
        { "toneColor",       normalize("toneColor", 0.4f) },
        { "airColumn",       normalize("airColumn", 0.5f) },
        { "jetReflection",   normalize("jetReflection", 0.35f) },
        { "endReflection",   normalize("endReflection", 0.3f) },
        { "vibratoRate",     normalize("vibratoRate", 4.5f) },
        { "vibratoDepth",    normalize("vibratoDepth", 0.12f) },
        { "vibratoTremolo",  normalize("vibratoTremolo", 0.1f) },
        { "vibratoOnset",    normalize("vibratoOnset", 450.0f) },
        { "vibratoDriftDepth", normalize("vibratoDriftDepth", 0.5f) },
        { "vibratoDriftSpeed", normalize("vibratoDriftSpeed", 0.3f) },
        { "width",           normalize("width", 1.5f) },
        { "formant",         normalize("formant", 0.5f) },
        { "material",        normalize("material", 0.5f) },
        { "outputLevel",     normalize("outputLevel", 0.0f) },
        { "infiniteSustain", normalize("infiniteSustain", 0.0f) },
        { "reversedJet",     normalize("reversedJet", 0.0f) },
        { "subHarmonics",    normalize("subHarmonics", 0.0f) },
        { "flutterTongue",   normalize("flutterTongue", 0.0f) },
        { "flutterRate",     normalize("flutterRate", 22.0f) },
        { "growl",           normalize("growl", 0.0f) },
        { "toneHoleToggle",  0.0f },
        { "humanize",        normalize("humanize", 0.35f) },
        { "inharmonicity",   normalize("inharmonicity", 0.25f) },
        { "referencePitch",  normalize("referencePitch", 440.0f) },
        { "tuningSystem",    normalize("tuningSystem", 2.0f) },
        { "instrumentPreset", normalize("instrumentPreset", 5.0f) },
    }, {} });

    // 7. Piccolo
    presets.push_back({ "Piccolo", {
        { "breathPressure",  normalize("breathPressure", 0.55f) },
        { "embouchure",      normalize("embouchure", 0.42f) },
        { "breathNoise",     normalize("breathNoise", 0.12f) },
        { "attackChiff",     normalize("attackChiff", 0.45f) },
        { "toneColor",       normalize("toneColor", 0.7f) },
        { "airColumn",       normalize("airColumn", 0.3f) },
        { "jetReflection",   normalize("jetReflection", 0.5f) },
        { "endReflection",   normalize("endReflection", 0.65f) },
        { "vibratoRate",     normalize("vibratoRate", 5.5f) },
        { "vibratoDepth",    normalize("vibratoDepth", 0.12f) },
        { "vibratoTremolo",  normalize("vibratoTremolo", 0.1f) },
        { "vibratoOnset",    normalize("vibratoOnset", 250.0f) },
        { "vibratoDriftDepth", normalize("vibratoDriftDepth", 0.3f) },
        { "vibratoDriftSpeed", normalize("vibratoDriftSpeed", 0.5f) },
        { "width",           normalize("width", 0.5f) },
        { "formant",         normalize("formant", 0.5f) },
        { "material",        normalize("material", 0.5f) },
        { "outputLevel",     normalize("outputLevel", -3.0f) },
        { "infiniteSustain", normalize("infiniteSustain", 0.0f) },
        { "reversedJet",     normalize("reversedJet", 0.0f) },
        { "subHarmonics",    normalize("subHarmonics", 0.0f) },
        { "flutterTongue",   normalize("flutterTongue", 0.0f) },
        { "flutterRate",     normalize("flutterRate", 22.0f) },
        { "growl",           normalize("growl", 0.0f) },
        { "toneHoleToggle",  1.0f },
        { "humanize",        normalize("humanize", 0.2f) },
        { "inharmonicity",   normalize("inharmonicity", 0.1f) },
        { "referencePitch",  normalize("referencePitch", 440.0f) },
        { "tuningSystem",    normalize("tuningSystem", 2.0f) },
        { "instrumentPreset", normalize("instrumentPreset", 6.0f) },
    }, {} });

    // 8. Ocarina
    presets.push_back({ "Ocarina", {
        { "breathPressure",  normalize("breathPressure", 0.5f) },
        { "embouchure",      normalize("embouchure", 0.47f) },
        { "breathNoise",     normalize("breathNoise", 0.05f) },
        { "attackChiff",     normalize("attackChiff", 0.15f) },
        { "toneColor",       normalize("toneColor", 0.25f) },
        { "airColumn",       normalize("airColumn", 0.7f) },
        { "jetReflection",   normalize("jetReflection", 0.4f) },
        { "endReflection",   normalize("endReflection", 0.35f) },
        { "vibratoRate",     normalize("vibratoRate", 4.5f) },
        { "vibratoDepth",    normalize("vibratoDepth", 0.2f) },
        { "vibratoTremolo",  normalize("vibratoTremolo", 0.15f) },
        { "vibratoOnset",    normalize("vibratoOnset", 350.0f) },
        { "vibratoDriftDepth", normalize("vibratoDriftDepth", 0.5f) },
        { "vibratoDriftSpeed", normalize("vibratoDriftSpeed", 0.35f) },
        { "width",           normalize("width", 0.7f) },
        { "formant",         normalize("formant", 0.5f) },
        { "material",        normalize("material", 0.5f) },
        { "outputLevel",     normalize("outputLevel", 0.0f) },
        { "infiniteSustain", normalize("infiniteSustain", 0.0f) },
        { "reversedJet",     normalize("reversedJet", 0.0f) },
        { "subHarmonics",    normalize("subHarmonics", 0.0f) },
        { "flutterTongue",   normalize("flutterTongue", 0.0f) },
        { "flutterRate",     normalize("flutterRate", 22.0f) },
        { "growl",           normalize("growl", 0.0f) },
        { "toneHoleToggle",  0.0f },
        { "humanize",        normalize("humanize", 0.25f) },
        { "inharmonicity",   normalize("inharmonicity", 0.3f) },
        { "referencePitch",  normalize("referencePitch", 440.0f) },
        { "tuningSystem",    normalize("tuningSystem", 2.0f) },
        { "instrumentPreset", normalize("instrumentPreset", 7.0f) },
    }, {} });

    presetManager.initializeFactoryPresets(presets);
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OWindAudioProcessor();
}
