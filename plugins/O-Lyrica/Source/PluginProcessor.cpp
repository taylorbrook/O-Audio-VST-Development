/*
  ==============================================================================

    O-Lyrica - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OLyricaAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Core Sound Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "masterVolume", 1 },
        "Master Volume",
        juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f),
        0.0f,
        "dB"
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "stringMaterial", 1 },
        "String Material",
        juce::StringArray { "Gut", "Nylon", "Wire", "Carbon", "Metal Alloy", "Glass", "Crystal", "Energy" },
        1  // Default: Nylon
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "brightness", 1 },
        "Brightness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "timbre", 1 },
        "Timbre",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "decayTime", 1 },
        "Decay Time",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.1f, 0.4f),  // Skewed for finer control at low values
        5.0f,
        "s"
    ));

    // Body Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodySize", 1 },
        "Body Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodyResonance", 1 },
        "Body Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.6f
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "woodType", 1 },
        "Wood Type",
        juce::StringArray { "Spruce", "Maple", "Exotic", "Synthetic" },
        0  // Default: Spruce
    ));

    // Sympathetic Resonance
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sympatheticAmount", 1 },
        "Sympathetic Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f
    ));

    // Pluck Mechanics
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pluckPosition", 1 },
        "Pluck Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "fingerHardness", 1 },
        "Finger Hardness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // Expression
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "technique", 1 },
        "Technique",
        juce::StringArray { "Normal", "Harmonic", "Muted", "Près de la table" },
        0  // Default: Normal
    ));

    // v1.30.0: Glissando toggle params (replace old glissandoMode dropdown)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "freeToggle", 1 },
        "Free Glissando",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "scaleToggle", 1 },
        "Scale-Locked Glissando",
        false
    ));

    // v1.30.0: Keyswitch note assignments (MIDI 0-47 = C-1 through B2)
    {
        juce::StringArray keyswitchNoteNames;
        for (int i = 0; i < 48; ++i)
            keyswitchNoteNames.add(juce::MidiMessage::getMidiNoteName(i, true, true, 4));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { "freeKeyswitchNote", 1 },
            "Free Keyswitch Note",
            keyswitchNoteNames,
            12  // Default: C0
        ));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { "scaleKeyswitchNote", 1 },
            "Scale Keyswitch Note",
            keyswitchNoteNames,
            14  // Default: D0
        ));
    }

    // v1.30.0: Free mode's own shape/interval/direction/custom semitones
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "freeShape", 1 },
        "Free Gliss Shape",
        juce::StringArray { "Linear", "Accelerate", "Decelerate", "S-Curve" },
        3  // Default: S-Curve
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "freeInterval", 1 },
        "Free Gliss Interval",
        juce::StringArray { "Minor 2nd", "Major 2nd", "Minor 3rd", "Major 3rd",
                            "Perfect 4th", "Tritone", "Perfect 5th", "Minor 6th",
                            "Major 6th", "Minor 7th", "Major 7th", "Octave",
                            "Octave + 5th", "2 Octaves", "2.5 Octaves", "3 Octaves",
                            "Custom" },
        11  // Default: Octave
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "freeDirection", 1 },
        "Free Gliss Direction",
        juce::StringArray { "Up to Note", "Down to Note" },
        0  // Default: Up to Note
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "freeCustomSemitones", 1 },
        "Free Custom Semitones",
        juce::NormalisableRange<float>(1.0f, 48.0f, 1.0f),
        12.0f,
        "st"
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "glissandoScale", 1 },
        "Glissando Scale",
        juce::StringArray { "Major", "Minor", "Pentatonic", "Custom" },
        0  // Default: Major
    ));

    // v1.30.0: Independent glissando tonic (not tied to tuning tab tonic)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "glissandoTonic", 1 },
        "Glissando Tonic",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" },
        0  // Default: C
    ));

    // v1.21.0: Glissando speed for scale-locked mode (notes per second)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "glissandoSpeed", 1 },
        "Glissando Speed",
        juce::NormalisableRange<float>(4.0f, 30.0f, 0.1f, 0.5f),
        12.0f,
        "n/s"
    ));

    // v1.22.0: Glissando shape — acceleration curve for scale-locked note spacing
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "glissandoShape", 1 },
        "Glissando Shape",
        juce::StringArray { "Linear", "Accelerate", "Decelerate", "S-Curve" },
        3  // Default: S-Curve (most natural harp feel)
    ));

    // v1.23.0: Glissando interval — fixed sweep distance (replaces unpredictable previousFrequency)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "glissandoInterval", 1 },
        "Glissando Interval",
        juce::StringArray { "Minor 2nd", "Major 2nd", "Minor 3rd", "Major 3rd",
                            "Perfect 4th", "Tritone", "Perfect 5th", "Minor 6th",
                            "Major 6th", "Minor 7th", "Major 7th", "Octave",
                            "Octave + 5th", "2 Octaves", "2.5 Octaves", "3 Octaves",
                            "Custom" },
        11  // Default: Octave
    ));

    // v1.23.0: Custom semitone count (active when glissandoInterval = Custom)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "glissandoCustomSemitones", 1 },
        "Glissando Custom Semitones",
        juce::NormalisableRange<float>(1.0f, 48.0f, 1.0f),
        12.0f,
        "st"
    ));

    // v1.23.0: Glissando direction — sweep up to note or down to note
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "glissandoDirection", 1 },
        "Glissando Direction",
        juce::StringArray { "Up to Note", "Down to Note" },
        0  // Default: Up to Note
    ));

    // v1.24.0: Glissando humanize — per-step timing jitter for natural feel
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "glissandoHumanize", 1 },
        "Glissando Humanize",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f  // Default: Subtle jitter (±3-8ms depending on speed)
    ));

    // v1.25.0: Glissando time — configurable ramp duration for Free mode
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "glissandoTime", 1 },
        "Glissando Time",
        juce::NormalisableRange<float>(0.01f, 0.5f, 0.001f, 0.5f),
        0.05f,
        "s"
    ));

    // v1.31.0: Tempo sync for Free mode glissando
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "freeTempoSync", 1 },
        "Free Tempo Sync",
        juce::StringArray { "Off", "1/32", "1/16", "1/16D", "1/8T", "1/8",
                            "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/2D",
                            "1 Bar", "2 Bars", "4 Bars" },
        0  // Default: Off
    ));

    // v1.31.0: Tempo sync for Scale-Locked mode glissando
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "scaleTempoSync", 1 },
        "Scale Tempo Sync",
        juce::StringArray { "Off", "1/32", "1/16", "1/16D", "1/8T", "1/8",
                            "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/2D",
                            "1 Bar", "2 Bars", "4 Bars" },
        0  // Default: Off
    ));

    // v1.26.0: Glissando excitation softness — brush vs deliberate pluck
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "glissandoExcitation", 1 },
        "Gliss Softness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.6f
    ));

    // v1.27.0: Glissando velocity profile — dynamic contour across Scale-Locked sweep
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "glissandoVelStart", 1 },
        "Gliss Vel Start",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "glissandoVelEnd", 1 },
        "Gliss Vel End",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f
    ));

    // Tuning
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "masterTune", 1 },
        "Master Tune",
        juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f),
        440.0f,
        "Hz"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchBendRange", 1 },
        "Pitch Bend Range",
        juce::NormalisableRange<float>(1.0f, 48.0f, 1.0f),
        2.0f,
        "st"
    ));

    // v1.6.0: Tuning Mode (12-TET, Custom/Scala, or MTS-ESP)
    // v1.7.2: Added MTS-ESP option to match UI buttons
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "tuningMode", 1 },
        "Tuning Mode",
        juce::StringArray { "12-TET", "Custom", "MTS-ESP" },
        0  // Default: 12-TET
    ));

    // v1.9.0: Octave Stretch (0.95-1.25, physical modeling enhancement)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "octaveStretch", 1 },
        "Octave Stretch",
        juce::NormalisableRange<float>(0.95f, 1.25f, 0.01f),
        1.0f
    ));

    // v1.9.0: Temperament Preset Selection
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "temperamentPreset", 1 },
        "Temperament Preset",
        juce::StringArray {
            "Equal 12-TET", "Pythagorean", "Zarlino", "Meantone (1/4)",
            "Werckmeister III", "Kirnberger III", "Vallotti",
            "Well Tempered", "Just Intonation", "Bohlen-Pierce", "Custom"
        },
        0  // Default: Equal 12-TET
    ));

    // Advanced String Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringTension", 1 },
        "String Tension",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringGauge", 1 },
        "String Gauge",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringLength", 1 },
        "String Length",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringStiffness", 1 },
        "String Stiffness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.2f
    ));

    // v1.3.0: Advanced Physical Modeling Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attackNoise", 1 },
        "Attack Noise",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f  // Default: Material-typical noise amount
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sympatheticQ", 1 },
        "Sympathetic Sharpness",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.1f, 0.5f),  // Skewed for finer control at low Q
        5.0f  // Default: Moderate resonance sharpness
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodyModeSpread", 1 },
        "Body Mode Spread",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
        0.0f  // Default: No spread (original tuning)
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bridgeBrightness", 1 },
        "Bridge Brightness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f  // Default: Neutral bridge reflection
    ));

    // v1.19.0: Humanize - per-note randomization for realistic variation
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "humanize", 1 },
        "Humanize",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f  // Default: No humanization (deterministic behavior)
    ));

    // v1.32.0: Effects chain parameters (Chorus -> Delay -> EQ -> Reverb)

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

    // v1.35.0: String Crosstalk (soundboard coupling between adjacent strings)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringCrosstalk", 1 },
        "String Crosstalk",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.2f
    ));

    // Reverb
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

OLyricaAudioProcessor::OLyricaAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-Lyrica")
{
    // Initialize synthesiser with 32 voices (v1.18.4: increased from 16)
    for (int i = 0; i < 32; ++i)
    {
        auto* voice = new HarpSynthVoice();
        voice->setAPVTS(&parameters);
        voice->setSympatheticEngine(&sympatheticEngine); // Phase 2.7: Connect sympathetic engine
        voice->setTuningEngine(&tuningEngine); // Phase 2.8: Connect tuning engine
        voice->setActiveGlissandoMode(&activeGlissandoMode); // v1.30.0: Glissando mode atomic
        voice->setCustomDegreeMask(&glissCustomDegrees); // v1.30.0: Custom degree bitmask
        voice->setHostBpm(&currentBpm); // v1.31.0: BPM for tempo sync
        voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // D-09: module-owned table
        synthesiser.addVoice(voice);
    }

    // Add sound that accepts all MIDI notes
    synthesiser.addSound(new HarpSynthSound());

    // v1.32.0: Cache effects parameter pointers for real-time access
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

    // v1.35.0: Cache crosstalk parameter pointer
    crosstalkParam = parameters.getRawParameterValue("stringCrosstalk");

    // v1.30.0: Register APVTS listeners for toggle mutual exclusion
    parameters.addParameterListener("freeToggle", this);
    parameters.addParameterListener("scaleToggle", this);

    // v1.12.0: Set up custom state callbacks for tuning persistence
    // v1.18.3: Removed verbose DBG logging (was development diagnostics)
    presetManager.setCustomStateCallbacks(
        // Save callback - returns tuning state as JSON
        [this]() -> juce::var {
            auto* obj = new juce::DynamicObject();

            // Save intervals
            auto intervals = tuningEngine.getIntervals();
            juce::Array<juce::var> intervalsArray;
            for (double cents : intervals)
                intervalsArray.add(cents);
            obj->setProperty("intervals", intervalsArray);

            // Save scale name
            obj->setProperty("scaleName", tuningEngine.getActiveTuningName());

            // Save tonic (note: also saved directly to XML as workaround for CustomState bug)
            obj->setProperty("tonic", juce::var(tuningEngine.getTonicNote()));

            // Save built-in preset index
            obj->setProperty("presetIndex", static_cast<int>(tuningEngine.getBuiltInPreset()));

            // Save octave stretch
            obj->setProperty("octaveStretch", tuningEngine.getOctaveStretch());

            // Save tuning mode explicitly
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

            // Restore preset index first (this sets intervals for built-in presets)
            if (obj->hasProperty("presetIndex"))
            {
                int presetIdx = static_cast<int>(obj->getProperty("presetIndex"));
                tuningEngine.setBuiltInPreset(
                    static_cast<TuningEngine::BuiltInPreset>(presetIdx));
            }

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

            // Restore tuning mode
            if (obj->hasProperty("tuningMode"))
            {
                int mode = static_cast<int>(obj->getProperty("tuningMode"));
                tuningEngine.setMode(static_cast<TuningEngine::Mode>(mode));

                // v1.13.3: Also update APVTS parameter to prevent processBlock from resetting
                if (auto* tuningModeParam = parameters.getParameter("tuningMode"))
                    tuningModeParam->setValueNotifyingHost(static_cast<float>(mode) / 2.0f);
            }

            // Restore tonic (do this AFTER setting intervals so rotation works)
            if (obj->hasProperty("tonic"))
            {
                int tonic = static_cast<int>(obj->getProperty("tonic"));
                tuningEngine.setTonicNote(tonic);
            }

            // Restore octave stretch
            if (obj->hasProperty("octaveStretch"))
            {
                float stretch = static_cast<float>(obj->getProperty("octaveStretch"));
                tuningEngine.setOctaveStretch(stretch);
            }
        }
    );

#if OUARICON_LICENSING_ENABLED
    licenseManager = std::make_unique<OuariconLicense>(
        "ouaricon-lyrica", OUARICON_SUPABASE_URL, OUARICON_SUPABASE_ANON_KEY);
#endif
}

OLyricaAudioProcessor::~OLyricaAudioProcessor()
{
    // v1.30.0: Remove APVTS listeners
    parameters.removeParameterListener("freeToggle", this);
    parameters.removeParameterListener("scaleToggle", this);
}

void OLyricaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);

    // v1.5.0: Initialize factory presets on first run (only creates if not exist)
    initializeFactoryPresets();

    // Phase 2.7: Prepare sympathetic resonance engine
    sympatheticEngine.prepare(sampleRate, samplesPerBlock);

    // v1.33.1: Prepare shared body resonance (post-mix processing)
    bodyResonance.prepare(sampleRate, samplesPerBlock);

    // Prepare all voices
    // v1.3.2: Use static_cast - all voices are HarpSynthVoice (we control voice creation)
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        auto* voice = static_cast<HarpSynthVoice*>(synthesiser.getVoice(i));
        voice->prepare(sampleRate, samplesPerBlock);
    }

    // v1.35.0: Compute one-pole lowpass coefficient for ~2kHz crosstalk filter
    crosstalkLPCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 2000.0f / static_cast<float>(sampleRate));

    // v2.1.4: Reset persistent per-voice-pair crosstalk filter state on prepare
    for (int i = 0; i < kNumCrosstalkVoices; ++i)
        for (int j = 0; j < kNumCrosstalkVoices; ++j)
            crosstalkStateA[i][j] = crosstalkStateB[i][j] = 0.0f;

    // v1.32.0: Prepare effects chain
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

void OLyricaAudioProcessor::releaseResources()
{
    // Release any resources when playback stops
}

void OLyricaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer
    buffer.clear();

    // VST3 Note Expression: drain the JUCE wrapper's raw-event queue and
    // correlate tuning deltas to their NoteOn's MIDI pitch. Pending table
    // is consumed atomically by each voice in startNote().
    vst3Extensions.drainAndUpdate();

    // v1.18.3: Removed redundant null checks - APVTS guarantees non-null for registered params
    // (Pattern matches HarpSynthVoice cleanup from v1.3.2)

    // Phase 2.7: Update sympathetic resonance parameters
    sympatheticEngine.setIntensity(parameters.getRawParameterValue("sympatheticAmount")->load());
    sympatheticEngine.setResonatorQ(parameters.getRawParameterValue("sympatheticQ")->load());

    // Phase 2.8: Update tuning engine parameters
    tuningEngine.setMasterTune(static_cast<double>(parameters.getRawParameterValue("masterTune")->load()));
    tuningEngine.setPitchBendRange(parameters.getRawParameterValue("pitchBendRange")->load());

    // v1.6.0: Update tuning mode
    // v1.13.3: Skip mode sync during state restoration to prevent race condition
    if (!isRestoringState.load(std::memory_order_acquire))
    {
        int modeInt = static_cast<int>(parameters.getRawParameterValue("tuningMode")->load());
        tuningEngine.setMode(static_cast<TuningEngine::Mode>(modeInt));
    }

    // v1.9.0: Update octave stretch
    tuningEngine.setOctaveStretch(parameters.getRawParameterValue("octaveStretch")->load());

    // v1.31.0: Read host BPM for tempo sync
    double hostBpm = 120.0;
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (auto bpm = pos->getBpm())
                hostBpm = *bpm;
        }
    }
    currentBpm.store(hostBpm, std::memory_order_release);

    // v1.3.2: Sync sympathetic coupling matrix at block boundary (thread-safe)
    sympatheticEngine.syncBeforeBlock();

    // v1.30.0: Keyswitch filtering — intercept keyswitch notes before passing to synthesiser
    int freeKSNote = static_cast<int>(parameters.getRawParameterValue("freeKeyswitchNote")->load());
    int scaleKSNote = static_cast<int>(parameters.getRawParameterValue("scaleKeyswitchNote")->load());

    juce::MidiBuffer filteredMidi;

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        int noteNum = msg.getNoteNumber();

        bool isKeyswitch = false;

        if (msg.isNoteOn())
        {
            if (noteNum == freeKSNote)
            {
                freeKeyswitchHeld.store(true, std::memory_order_release);
                scaleKeyswitchHeld.store(false, std::memory_order_release); // Exclusive
                isKeyswitch = true;
            }
            else if (noteNum == scaleKSNote)
            {
                scaleKeyswitchHeld.store(true, std::memory_order_release);
                freeKeyswitchHeld.store(false, std::memory_order_release); // Exclusive
                isKeyswitch = true;
            }
        }
        else if (msg.isNoteOff())
        {
            if (noteNum == freeKSNote)
            {
                freeKeyswitchHeld.store(false, std::memory_order_release);
                isKeyswitch = true;
            }
            else if (noteNum == scaleKSNote)
            {
                scaleKeyswitchHeld.store(false, std::memory_order_release);
                isKeyswitch = true;
            }
        }

        if (!isKeyswitch)
        {
            filteredMidi.addEvent(msg, metadata.samplePosition);

            // v1.7.9: Push MIDI note events to queue for UI visualization (tuning circle flash)
            if (msg.isNoteOn())
                midiEventQueue.push({ noteNum, msg.getFloatVelocity() });
            else if (msg.isNoteOff())
                midiEventQueue.push({ noteNum, 0.0f });
        }
    }

    // v1.30.0: Update active glissando mode from keyswitches + toggles
    updateActiveGlissandoMode();

    // v1.35.0: Clear per-voice output buffers for crosstalk capture
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
        static_cast<HarpSynthVoice*>(synthesiser.getVoice(i))->clearVoiceOutputBuffer();

    // Render MIDI to audio via synthesiser
    synthesiser.renderNextBlock(buffer, filteredMidi, 0, buffer.getNumSamples());

    // v1.35.0: String crosstalk via soundboard coupling
    // Adjacent strings (±1-2 semitones) transfer energy through the soundboard
    // regardless of harmonic relationship — filtered through LP at ~2kHz
    {
        float crosstalkAmount = crosstalkParam->load(std::memory_order_relaxed);
        if (crosstalkAmount > 0.0001f)
        {
            const int numVoices = synthesiser.getNumVoices();
            const int numSamples = buffer.getNumSamples();

            struct VoiceInfo { int index; int note; };
            VoiceInfo active[32];
            int numActive = 0;

            for (int i = 0; i < numVoices && numActive < 32; ++i)
            {
                auto* v = static_cast<HarpSynthVoice*>(synthesiser.getVoice(i));
                if (v->isVoiceActive())
                    active[numActive++] = { i, v->getCurrentlyPlayingNote() };
            }

            float* ch0 = buffer.getWritePointer(0);

            for (int a = 0; a < numActive; ++a)
            {
                for (int b = a + 1; b < numActive; ++b)
                {
                    int diff = std::abs(active[a].note - active[b].note);
                    if (diff < 1 || diff > 2) continue;

                    float distFactor = (diff == 1) ? 1.0f : 0.5f;
                    float gain = crosstalkAmount * distFactor * 0.05f;

                    auto* vA = static_cast<HarpSynthVoice*>(synthesiser.getVoice(active[a].index));
                    auto* vB = static_cast<HarpSynthVoice*>(synthesiser.getVoice(active[b].index));

                    const float* dataA = vA->getVoiceOutputBuffer().getReadPointer(0);
                    const float* dataB = vB->getVoiceOutputBuffer().getReadPointer(0);

                    // v2.1.4: Use persistent per-voice-pair filter state so the one-pole LP
                    // is continuous across processBlock boundaries (previously reset to 0
                    // every block because stateA/stateB were inner-scope locals).
                    const int idxA = active[a].index;
                    const int idxB = active[b].index;
                    float stateA = crosstalkStateA[idxA][idxB];
                    float stateB = crosstalkStateB[idxA][idxB];

                    for (int s = 0; s < numSamples; ++s)
                    {
                        float filtA = crosstalkLPCoeff * dataA[s] + (1.0f - crosstalkLPCoeff) * stateA;
                        stateA = filtA;
                        float filtB = crosstalkLPCoeff * dataB[s] + (1.0f - crosstalkLPCoeff) * stateB;
                        stateB = filtB;
                        ch0[s] += (filtA + filtB) * gain;
                    }

                    crosstalkStateA[idxA][idxB] = stateA;
                    crosstalkStateB[idxA][idxB] = stateB;
                }
            }
        }
    }

    // v1.33.1: Shared body resonance (post-mix, single instance for all voices)
    {
        float bodySize = parameters.getRawParameterValue("bodySize")->load();
        float bodyAmount = parameters.getRawParameterValue("bodyResonance")->load();
        int woodTypeIndex = static_cast<int>(parameters.getRawParameterValue("woodType")->load());
        bodyResonance.setBodyParameters(bodySize, woodTypeFromIndex(woodTypeIndex), bodyAmount);
        bodyResonance.setModeSpread(parameters.getRawParameterValue("bodyModeSpread")->load());

        // Process mono synth output through shared body resonance, then copy to all channels
        float* ch0 = buffer.getWritePointer(0);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            ch0[i] = bodyResonance.process(ch0[i]);

        for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom(ch, 0, ch0, buffer.getNumSamples());
    }

    // v2.1.2: Effects chain (Chorus -> Delay -> Reverb -> EQ)
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
        // v2.1.6: Load reverbMix once and reuse — previously loaded twice (setMix + gate check).
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

    // Apply master volume
    float volumeDb = parameters.getRawParameterValue("masterVolume")->load();
    buffer.applyGain(juce::Decibels::decibelsToGain(volumeDb));
}

// v1.30.0: Toggle mutual exclusion — turning one ON turns the other OFF
void OLyricaAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (isUpdatingToggles)
        return;

    isUpdatingToggles = true;

    if (parameterID == "freeToggle" && newValue >= 0.5f)
    {
        if (auto* scaleParam = parameters.getParameter("scaleToggle"))
            scaleParam->setValueNotifyingHost(0.0f);
    }
    else if (parameterID == "scaleToggle" && newValue >= 0.5f)
    {
        if (auto* freeParam = parameters.getParameter("freeToggle"))
            freeParam->setValueNotifyingHost(0.0f);
    }

    isUpdatingToggles = false;

    updateActiveGlissandoMode();
}

// v1.30.0: Priority logic for active glissando mode
void OLyricaAudioProcessor::updateActiveGlissandoMode()
{
    if (scaleKeyswitchHeld.load(std::memory_order_acquire))
        activeGlissandoMode.store(2, std::memory_order_release);
    else if (freeKeyswitchHeld.load(std::memory_order_acquire))
        activeGlissandoMode.store(1, std::memory_order_release);
    else if (parameters.getRawParameterValue("scaleToggle")->load() >= 0.5f)
        activeGlissandoMode.store(2, std::memory_order_release);
    else if (parameters.getRawParameterValue("freeToggle")->load() >= 0.5f)
        activeGlissandoMode.store(1, std::memory_order_release);
    else
        activeGlissandoMode.store(0, std::memory_order_release);
}

int OLyricaAudioProcessor::getActiveVoiceCount() const
{
    // Phase 2.11: Count actively playing voices
    int activeCount = 0;
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = synthesiser.getVoice(i))
        {
            if (voice->isVoiceActive())
                ++activeCount;
        }
    }
    return activeCount;
}

// v1.7.4: Trigger note from WebView keyboard visualization
void OLyricaAudioProcessor::triggerNoteOn(int midiNote, float velocity)
{
    // Clamp values to valid ranges
    midiNote = juce::jlimit(0, 127, midiNote);
    velocity = juce::jlimit(0.0f, 1.0f, velocity);

    // Use channel 1 (index 0) for UI-triggered notes
    synthesiser.noteOn(1, midiNote, velocity);

    // v1.7.9: Push to event queue for tuning circle visualization
    midiEventQueue.push({ midiNote, velocity });
}

// v1.7.4: Release note from WebView keyboard visualization
void OLyricaAudioProcessor::triggerNoteOff(int midiNote)
{
    midiNote = juce::jlimit(0, 127, midiNote);

    // allowTailOff = true for natural release
    synthesiser.noteOff(1, midiNote, 0.0f, true);

    // v1.7.9: Push to event queue for tuning circle visualization
    midiEventQueue.push({ midiNote, 0.0f });
}

// v1.10.0: Get held notes and their frequencies for True Keys visualization
void OLyricaAudioProcessor::getHeldNotesData(std::vector<int>& notes, std::vector<double>& frequencies)
{
    notes.clear();
    frequencies.clear();

    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = synthesiser.getVoice(i))
        {
            if (voice->isVoiceActive())
            {
                int midiNote = voice->getCurrentlyPlayingNote();
                double freq = tuningEngine.getFrequency(midiNote);
                notes.push_back(midiNote);
                frequencies.push_back(freq);
            }
        }
    }
}

juce::AudioProcessorEditor* OLyricaAudioProcessor::createEditor()
{
    return new OLyricaAudioProcessorEditor(*this);
}

void OLyricaAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // v1.5.0: Use preset manager for state serialization
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
    {
        // v1.13.1: Save tonic directly to XML (workaround for CustomState serialization bug)
        xml->setAttribute("directTonic", tuningEngine.getTonicNote());
        // v1.18.0: Save tooltip enabled state
        xml->setAttribute("tooltipsEnabled", tooltipsEnabled.load(std::memory_order_acquire));
        // v1.30.0: Save glissando custom degree bitmask
        xml->setAttribute("glissCustomDegrees", juce::String(static_cast<juce::int64>(glissCustomDegrees.load(std::memory_order_acquire))));
        copyXmlToBinary(*xml, destData);
    }
}

void OLyricaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // v1.13.3: Set flag to prevent processBlock from interfering during restoration
    isRestoringState.store(true, std::memory_order_release);

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
    {
        // v1.5.0: Use preset manager for state deserialization
        presetManager.setStateFromXml(xmlState.get());

        // v1.13.1: Restore tonic directly from XML (workaround for CustomState serialization bug)
        if (xmlState->hasAttribute("directTonic"))
        {
            int savedTonic = xmlState->getIntAttribute("directTonic", 0);
            tuningEngine.setTonicNote(savedTonic);
        }

        // v1.18.0: Restore tooltip enabled state
        if (xmlState->hasAttribute("tooltipsEnabled"))
        {
            bool enabled = xmlState->getBoolAttribute("tooltipsEnabled", false);
            tooltipsEnabled.store(enabled, std::memory_order_release);
        }

        // v1.30.0: Restore glissando custom degree bitmask
        if (xmlState->hasAttribute("glissCustomDegrees"))
        {
            auto saved = xmlState->getStringAttribute("glissCustomDegrees").getLargeIntValue();
            glissCustomDegrees.store(static_cast<uint64_t>(saved), std::memory_order_release);
        }
    }

    // v1.13.3: Clear restoration flag
    isRestoringState.store(false, std::memory_order_release);
}

// v1.5.0: Initialize factory presets (48 presets organized by string material)
void OLyricaAudioProcessor::initializeFactoryPresets()
{
    // v2.0.0: Version-checked factory preset regeneration
    static const juce::String kFactoryPresetVersion = "2.0.1";
    auto factoryDir = presetManager.getFactoryPresetsDirectory();
    auto versionFile = factoryDir.getChildFile(".version");

    if (versionFile.existsAsFile() && versionFile.loadFileAsString().trim() == kFactoryPresetVersion)
    {
        DBG("[O-Lyrica] Factory presets v" + kFactoryPresetVersion + " already up to date");
        return;
    }

    // Clear old factory presets for regeneration
    if (factoryDir.isDirectory())
        factoryDir.deleteRecursively();

    std::vector<OuariconPresetManager::FactoryPresetDef> presets;

    auto choiceNorm = [](int index, int numOptions) { return static_cast<float>(index) / static_cast<float>(numOptions - 1); };

    // ========================================================================
    // GUT STRINGS (Material 0) - Warm, historical, organic character
    // ========================================================================
    presets.push_back({ "Ancient Lyre", {
        { "stringMaterial", choiceNorm(0, 8) }, { "brightness", 0.35f }, { "timbre", 0.6f },
        { "decayTime", 0.4f }, { "bodySize", 0.6f }, { "bodyResonance", 0.7f },
        { "woodType", choiceNorm(2, 4) }, { "sympatheticAmount", 0.4f },
        { "pluckPosition", 0.4f }, { "fingerHardness", 0.3f }, { "attackNoise", 0.6f },
        { "stringTension", 0.4f }, { "stringStiffness", 0.15f }, { "bridgeBrightness", 0.4f },
        { "humanize", 0.35f }, { "stringCrosstalk", 0.4f }, { "sympatheticQ", 0.04f },
        { "bodyModeSpread", 0.55f }, { "stringGauge", 0.55f }, { "stringLength", 0.45f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.3f }, { "reverbDamp", 0.6f },
        { "reverbPredelay", 0.05f }, { "reverbMix", 0.2f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Fireside Tales", {
        { "stringMaterial", choiceNorm(0, 8) }, { "brightness", 0.4f }, { "timbre", 0.65f },
        { "decayTime", 0.35f }, { "bodySize", 0.55f }, { "bodyResonance", 0.65f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.35f },
        { "pluckPosition", 0.45f }, { "fingerHardness", 0.35f }, { "attackNoise", 0.5f },
        { "stringTension", 0.45f }, { "stringStiffness", 0.2f }, { "bridgeBrightness", 0.45f },
        { "humanize", 0.3f }, { "stringCrosstalk", 0.35f }, { "sympatheticQ", 0.05f },
        { "bodyModeSpread", 0.53f }, { "stringGauge", 0.5f }, { "stringLength", 0.48f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Medieval Court", {
        { "stringMaterial", choiceNorm(0, 8) }, { "brightness", 0.45f }, { "timbre", 0.7f },
        { "decayTime", 0.45f }, { "bodySize", 0.65f }, { "bodyResonance", 0.6f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.5f },
        { "pluckPosition", 0.5f }, { "fingerHardness", 0.4f }, { "attackNoise", 0.55f },
        { "stringTension", 0.5f }, { "stringStiffness", 0.18f }, { "bridgeBrightness", 0.5f },
        { "humanize", 0.25f }, { "stringCrosstalk", 0.38f }, { "sympatheticQ", 0.06f },
        { "bodyModeSpread", 0.56f }, { "stringGauge", 0.52f }, { "stringLength", 0.5f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.5f }, { "reverbDamp", 0.4f },
        { "reverbPredelay", 0.15f }, { "reverbMix", 0.25f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Warm Classical", {
        { "stringMaterial", choiceNorm(0, 8) }, { "brightness", 0.5f }, { "timbre", 0.75f },
        { "decayTime", 0.65f }, { "bodySize", 0.78f }, { "bodyResonance", 0.82f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.62f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.45f }, { "attackNoise", 0.45f },
        { "stringTension", 0.55f }, { "stringStiffness", 0.22f }, { "bridgeBrightness", 0.55f },
        { "humanize", 0.2f }, { "stringCrosstalk", 0.42f }, { "sympatheticQ", 0.06f },
        { "bodyModeSpread", 0.60f }, { "stringGauge", 0.55f }, { "stringLength", 0.55f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.72f }, { "reverbDamp", 0.38f },
        { "reverbPredelay", 0.18f }, { "reverbMix", 0.32f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Bardic Song", {
        { "stringMaterial", choiceNorm(0, 8) }, { "brightness", 0.42f }, { "timbre", 0.68f },
        { "decayTime", 0.38f }, { "bodySize", 0.58f }, { "bodyResonance", 0.68f },
        { "woodType", choiceNorm(2, 4) }, { "sympatheticAmount", 0.55f },
        { "pluckPosition", 0.42f }, { "fingerHardness", 0.38f }, { "attackNoise", 0.58f },
        { "stringTension", 0.42f }, { "stringStiffness", 0.16f }, { "bridgeBrightness", 0.42f },
        { "humanize", 0.4f }, { "stringCrosstalk", 0.42f }, { "sympatheticQ", 0.04f },
        { "bodyModeSpread", 0.57f }, { "stringGauge", 0.52f }, { "stringLength", 0.47f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Nostalgic Whisper", {
        { "stringMaterial", choiceNorm(0, 8) }, { "brightness", 0.3f }, { "timbre", 0.55f },
        { "decayTime", 0.55f }, { "bodySize", 0.5f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.3f },
        { "pluckPosition", 0.35f }, { "fingerHardness", 0.25f }, { "attackNoise", 0.4f },
        { "stringTension", 0.35f }, { "stringStiffness", 0.12f }, { "bridgeBrightness", 0.35f },
        { "humanize", 0.3f }, { "stringCrosstalk", 0.3f }, { "sympatheticQ", 0.03f },
        { "bodyModeSpread", 0.52f }, { "stringGauge", 0.48f }, { "stringLength", 0.42f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.65f }, { "reverbDamp", 0.55f },
        { "reverbPredelay", 0.2f }, { "reverbMix", 0.3f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    // ========================================================================
    // NYLON STRINGS (Material 1) - Soft, mellow, folk/classical character
    // ========================================================================
    presets.push_back({ "Celtic Dawn", {
        { "stringMaterial", choiceNorm(1, 8) }, { "brightness", 0.45f }, { "timbre", 0.7f },
        { "decayTime", 0.45f }, { "bodySize", 0.6f }, { "bodyResonance", 0.65f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.5f },
        { "pluckPosition", 0.5f }, { "fingerHardness", 0.4f }, { "attackNoise", 0.35f },
        { "stringTension", 0.5f }, { "stringStiffness", 0.15f }, { "bridgeBrightness", 0.5f },
        { "humanize", 0.2f }, { "stringCrosstalk", 0.3f }, { "sympatheticQ", 0.06f },
        { "bodyModeSpread", 0.52f }, { "stringGauge", 0.45f }, { "stringLength", 0.5f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Folk Ballad", {
        { "stringMaterial", choiceNorm(1, 8) }, { "brightness", 0.5f }, { "timbre", 0.72f },
        { "decayTime", 0.4f }, { "bodySize", 0.55f }, { "bodyResonance", 0.6f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.4f },
        { "pluckPosition", 0.48f }, { "fingerHardness", 0.45f }, { "attackNoise", 0.4f },
        { "stringTension", 0.52f }, { "stringStiffness", 0.18f }, { "bridgeBrightness", 0.52f },
        { "humanize", 0.25f }, { "stringCrosstalk", 0.28f }, { "sympatheticQ", 0.05f },
        { "bodyModeSpread", 0.5f }, { "stringGauge", 0.48f }, { "stringLength", 0.48f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Gentle Stream", {
        { "stringMaterial", choiceNorm(1, 8) }, { "brightness", 0.4f }, { "timbre", 0.65f },
        { "decayTime", 0.55f }, { "bodySize", 0.65f }, { "bodyResonance", 0.7f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.55f },
        { "pluckPosition", 0.45f }, { "fingerHardness", 0.35f }, { "attackNoise", 0.3f },
        { "stringTension", 0.45f }, { "stringStiffness", 0.12f }, { "bridgeBrightness", 0.45f },
        { "humanize", 0.2f }, { "stringCrosstalk", 0.32f }, { "sympatheticQ", 0.07f },
        { "bodyModeSpread", 0.53f }, { "stringGauge", 0.44f }, { "stringLength", 0.52f },
        { "chorusBypass", 0.0f }, { "chorusRate", 0.05f }, { "chorusDepth", 0.3f }, { "chorusMix", 0.15f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.5f }, { "reverbDamp", 0.5f },
        { "reverbPredelay", 0.1f }, { "reverbMix", 0.2f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Morning Dew", {
        { "stringMaterial", choiceNorm(1, 8) }, { "brightness", 0.55f }, { "timbre", 0.68f },
        { "decayTime", 0.5f }, { "bodySize", 0.58f }, { "bodyResonance", 0.62f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.45f },
        { "pluckPosition", 0.52f }, { "fingerHardness", 0.42f }, { "attackNoise", 0.38f },
        { "stringTension", 0.48f }, { "stringStiffness", 0.16f }, { "bridgeBrightness", 0.48f },
        { "humanize", 0.15f }, { "stringCrosstalk", 0.3f }, { "sympatheticQ", 0.06f },
        { "bodyModeSpread", 0.51f }, { "stringGauge", 0.46f }, { "stringLength", 0.5f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Pastoral Scene", {
        { "stringMaterial", choiceNorm(1, 8) }, { "brightness", 0.48f }, { "timbre", 0.74f },
        { "decayTime", 0.48f }, { "bodySize", 0.62f }, { "bodyResonance", 0.68f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.52f },
        { "pluckPosition", 0.5f }, { "fingerHardness", 0.38f }, { "attackNoise", 0.32f },
        { "stringTension", 0.5f }, { "stringStiffness", 0.14f }, { "bridgeBrightness", 0.5f },
        { "humanize", 0.2f }, { "stringCrosstalk", 0.33f }, { "sympatheticQ", 0.07f },
        { "bodyModeSpread", 0.54f }, { "stringGauge", 0.48f }, { "stringLength", 0.53f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.45f }, { "reverbDamp", 0.35f },
        { "reverbPredelay", 0.15f }, { "reverbMix", 0.2f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Harmonic Dreams", {
        { "stringMaterial", choiceNorm(1, 8) }, { "brightness", 0.52f }, { "timbre", 0.76f },
        { "decayTime", 0.6f }, { "bodySize", 0.68f }, { "bodyResonance", 0.72f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.6f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.3f }, { "attackNoise", 0.25f },
        { "stringTension", 0.46f }, { "stringStiffness", 0.1f }, { "bridgeBrightness", 0.46f },
        { "technique", choiceNorm(1, 4) },
        { "humanize", 0.15f }, { "stringCrosstalk", 0.35f }, { "sympatheticQ", 0.08f },
        { "bodyModeSpread", 0.55f }, { "stringGauge", 0.44f }, { "stringLength", 0.55f },
        { "chorusBypass", 0.0f }, { "chorusRate", 0.04f }, { "chorusDepth", 0.35f }, { "chorusMix", 0.2f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.6f }, { "reverbDamp", 0.5f },
        { "reverbPredelay", 0.1f }, { "reverbMix", 0.25f },
        { "reverbMod", 0.3f }, { "reverbShimmer", 0.1f }
    }, {} });

    // ========================================================================
    // WIRE STRINGS (Material 2) - Bright, articulate, concert harp character
    // ========================================================================
    presets.push_back({ "Bright Cascade", {
        { "stringMaterial", choiceNorm(2, 8) }, { "brightness", 0.7f }, { "timbre", 0.65f },
        { "decayTime", 0.5f }, { "bodySize", 0.55f }, { "bodyResonance", 0.6f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.45f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.55f }, { "attackNoise", 0.5f },
        { "stringTension", 0.6f }, { "stringStiffness", 0.25f }, { "bridgeBrightness", 0.6f },
        { "humanize", 0.12f }, { "stringCrosstalk", 0.25f }, { "sympatheticQ", 0.1f },
        { "bodyModeSpread", 0.5f }, { "stringGauge", 0.52f }, { "stringLength", 0.52f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Articulate Pluck", {
        { "stringMaterial", choiceNorm(2, 8) }, { "brightness", 0.65f }, { "timbre", 0.6f },
        { "decayTime", 0.35f }, { "bodySize", 0.5f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.35f },
        { "pluckPosition", 0.6f }, { "fingerHardness", 0.6f }, { "attackNoise", 0.55f },
        { "stringTension", 0.65f }, { "stringStiffness", 0.28f }, { "bridgeBrightness", 0.65f },
        { "humanize", 0.1f }, { "stringCrosstalk", 0.2f }, { "sympatheticQ", 0.12f },
        { "bodyModeSpread", 0.48f }, { "stringGauge", 0.55f }, { "stringLength", 0.5f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Concert Grand", {
        { "stringMaterial", choiceNorm(2, 8) }, { "brightness", 0.6f }, { "timbre", 0.7f },
        { "decayTime", 0.55f }, { "bodySize", 0.7f }, { "bodyResonance", 0.7f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.55f },
        { "pluckPosition", 0.52f }, { "fingerHardness", 0.5f }, { "attackNoise", 0.45f },
        { "stringTension", 0.55f }, { "stringStiffness", 0.22f }, { "bridgeBrightness", 0.55f },
        { "humanize", 0.12f }, { "stringCrosstalk", 0.3f }, { "sympatheticQ", 0.1f },
        { "bodyModeSpread", 0.52f }, { "stringGauge", 0.54f }, { "stringLength", 0.56f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.54f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.55f }, { "reverbDamp", 0.4f },
        { "reverbPredelay", 0.12f }, { "reverbMix", 0.22f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Modern Classic", {
        { "stringMaterial", choiceNorm(2, 8) }, { "brightness", 0.62f }, { "timbre", 0.68f },
        { "decayTime", 0.48f }, { "bodySize", 0.6f }, { "bodyResonance", 0.65f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.5f },
        { "pluckPosition", 0.5f }, { "fingerHardness", 0.52f }, { "attackNoise", 0.48f },
        { "stringTension", 0.58f }, { "stringStiffness", 0.24f }, { "bridgeBrightness", 0.58f },
        { "humanize", 0.15f }, { "stringCrosstalk", 0.28f }, { "sympatheticQ", 0.09f },
        { "bodyModeSpread", 0.5f }, { "stringGauge", 0.52f }, { "stringLength", 0.54f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Silver Strings", {
        { "stringMaterial", choiceNorm(2, 8) }, { "brightness", 0.72f }, { "timbre", 0.62f },
        { "decayTime", 0.52f }, { "bodySize", 0.52f }, { "bodyResonance", 0.58f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.42f },
        { "pluckPosition", 0.58f }, { "fingerHardness", 0.58f }, { "attackNoise", 0.52f },
        { "stringTension", 0.62f }, { "stringStiffness", 0.26f }, { "bridgeBrightness", 0.62f },
        { "humanize", 0.1f }, { "stringCrosstalk", 0.22f }, { "sympatheticQ", 0.11f },
        { "bodyModeSpread", 0.49f }, { "stringGauge", 0.55f }, { "stringLength", 0.52f },
        { "chorusBypass", 0.0f }, { "chorusRate", 0.06f }, { "chorusDepth", 0.25f }, { "chorusMix", 0.12f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Pedal Technique", {
        { "stringMaterial", choiceNorm(2, 8) }, { "brightness", 0.58f }, { "timbre", 0.72f },
        { "decayTime", 0.6f }, { "bodySize", 0.68f }, { "bodyResonance", 0.72f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.58f },
        { "pluckPosition", 0.48f }, { "fingerHardness", 0.48f }, { "attackNoise", 0.42f },
        { "stringTension", 0.52f }, { "stringStiffness", 0.2f }, { "bridgeBrightness", 0.52f },
        { "humanize", 0.15f }, { "stringCrosstalk", 0.32f }, { "sympatheticQ", 0.09f },
        { "bodyModeSpread", 0.53f }, { "stringGauge", 0.5f }, { "stringLength", 0.56f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.5f }, { "reverbDamp", 0.45f },
        { "reverbPredelay", 0.1f }, { "reverbMix", 0.18f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    // ========================================================================
    // CARBON STRINGS (Material 3) - Clean, precise, extended range
    // ========================================================================
    presets.push_back({ "Crystal Clear", {
        { "stringMaterial", choiceNorm(3, 8) }, { "brightness", 0.75f }, { "timbre", 0.55f },
        { "decayTime", 0.45f }, { "bodySize", 0.45f }, { "bodyResonance", 0.5f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.35f },
        { "pluckPosition", 0.6f }, { "fingerHardness", 0.65f }, { "attackNoise", 0.4f },
        { "stringTension", 0.7f }, { "stringStiffness", 0.3f }, { "bridgeBrightness", 0.7f },
        { "humanize", 0.05f }, { "stringCrosstalk", 0.12f }, { "sympatheticQ", 0.15f },
        { "bodyModeSpread", 0.5f }, { "stringGauge", 0.4f }, { "stringLength", 0.52f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Precision Touch", {
        { "stringMaterial", choiceNorm(3, 8) }, { "brightness", 0.7f }, { "timbre", 0.58f },
        { "decayTime", 0.4f }, { "bodySize", 0.48f }, { "bodyResonance", 0.52f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.3f },
        { "pluckPosition", 0.58f }, { "fingerHardness", 0.62f }, { "attackNoise", 0.45f },
        { "stringTension", 0.68f }, { "stringStiffness", 0.32f }, { "bridgeBrightness", 0.68f },
        { "humanize", 0.05f }, { "stringCrosstalk", 0.1f }, { "sympatheticQ", 0.16f },
        { "bodyModeSpread", 0.49f }, { "stringGauge", 0.42f }, { "stringLength", 0.5f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Extended Range", {
        { "stringMaterial", choiceNorm(3, 8) }, { "brightness", 0.68f }, { "timbre", 0.6f },
        { "decayTime", 0.5f }, { "bodySize", 0.55f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.4f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.6f }, { "attackNoise", 0.42f },
        { "stringTension", 0.65f }, { "stringStiffness", 0.28f }, { "bridgeBrightness", 0.65f },
        { "humanize", 0.08f }, { "stringCrosstalk", 0.15f }, { "sympatheticQ", 0.13f },
        { "bodyModeSpread", 0.51f }, { "stringGauge", 0.38f }, { "stringLength", 0.55f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Studio Session", {
        { "stringMaterial", choiceNorm(3, 8) }, { "brightness", 0.65f }, { "timbre", 0.62f },
        { "decayTime", 0.42f }, { "bodySize", 0.5f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.38f },
        { "pluckPosition", 0.52f }, { "fingerHardness", 0.58f }, { "attackNoise", 0.48f },
        { "stringTension", 0.62f }, { "stringStiffness", 0.26f }, { "bridgeBrightness", 0.62f },
        { "humanize", 0.1f }, { "stringCrosstalk", 0.14f }, { "sympatheticQ", 0.14f },
        { "bodyModeSpread", 0.5f }, { "stringGauge", 0.44f }, { "stringLength", 0.52f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.54f }, { "eqHighGain", 0.56f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Clean Articulation", {
        { "stringMaterial", choiceNorm(3, 8) }, { "brightness", 0.72f }, { "timbre", 0.56f },
        { "decayTime", 0.35f }, { "bodySize", 0.42f }, { "bodyResonance", 0.48f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.32f },
        { "pluckPosition", 0.62f }, { "fingerHardness", 0.68f }, { "attackNoise", 0.5f },
        { "stringTension", 0.72f }, { "stringStiffness", 0.34f }, { "bridgeBrightness", 0.72f },
        { "humanize", 0.05f }, { "stringCrosstalk", 0.1f }, { "sympatheticQ", 0.18f },
        { "bodyModeSpread", 0.48f }, { "stringGauge", 0.4f }, { "stringLength", 0.5f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Harmonic Purity", {
        { "stringMaterial", choiceNorm(3, 8) }, { "brightness", 0.78f }, { "timbre", 0.52f },
        { "decayTime", 0.55f }, { "bodySize", 0.4f }, { "bodyResonance", 0.45f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.25f },
        { "pluckPosition", 0.65f }, { "fingerHardness", 0.7f }, { "attackNoise", 0.35f },
        { "stringTension", 0.75f }, { "stringStiffness", 0.35f }, { "bridgeBrightness", 0.75f },
        { "technique", choiceNorm(1, 4) },
        { "humanize", 0.05f }, { "stringCrosstalk", 0.12f }, { "sympatheticQ", 0.14f },
        { "bodyModeSpread", 0.5f }, { "stringGauge", 0.38f }, { "stringLength", 0.54f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.4f }, { "reverbDamp", 0.3f },
        { "reverbPredelay", 0.05f }, { "reverbMix", 0.18f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    // ========================================================================
    // METAL ALLOY STRINGS (Material 4) - Brilliant, bell-like, sustaining
    // ========================================================================
    presets.push_back({ "Brilliant Sustain", {
        { "stringMaterial", choiceNorm(4, 8) }, { "brightness", 0.75f }, { "timbre", 0.6f },
        { "decayTime", 0.7f }, { "bodySize", 0.6f }, { "bodyResonance", 0.65f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.55f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.6f }, { "attackNoise", 0.55f },
        { "stringTension", 0.65f }, { "stringStiffness", 0.35f }, { "bridgeBrightness", 0.65f },
        { "humanize", 0.12f }, { "stringCrosstalk", 0.3f }, { "sympatheticQ", 0.1f },
        { "bodyModeSpread", 0.52f }, { "stringGauge", 0.55f }, { "stringLength", 0.54f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Bell Tones", {
        { "stringMaterial", choiceNorm(4, 8) }, { "brightness", 0.8f }, { "timbre", 0.55f },
        { "decayTime", 0.65f }, { "bodySize", 0.5f }, { "bodyResonance", 0.6f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.5f },
        { "pluckPosition", 0.6f }, { "fingerHardness", 0.65f }, { "attackNoise", 0.6f },
        { "stringTension", 0.7f }, { "stringStiffness", 0.38f }, { "bridgeBrightness", 0.7f },
        { "humanize", 0.1f }, { "stringCrosstalk", 0.28f }, { "sympatheticQ", 0.12f },
        { "bodyModeSpread", 0.5f }, { "stringGauge", 0.58f }, { "stringLength", 0.52f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.58f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Orchestral Ring", {
        { "stringMaterial", choiceNorm(4, 8) }, { "brightness", 0.7f }, { "timbre", 0.65f },
        { "decayTime", 0.6f }, { "bodySize", 0.65f }, { "bodyResonance", 0.7f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.6f },
        { "pluckPosition", 0.52f }, { "fingerHardness", 0.55f }, { "attackNoise", 0.5f },
        { "stringTension", 0.6f }, { "stringStiffness", 0.32f }, { "bridgeBrightness", 0.6f },
        { "humanize", 0.12f }, { "stringCrosstalk", 0.35f }, { "sympatheticQ", 0.1f },
        { "bodyModeSpread", 0.53f }, { "stringGauge", 0.55f }, { "stringLength", 0.56f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.6f }, { "reverbDamp", 0.4f },
        { "reverbPredelay", 0.15f }, { "reverbMix", 0.22f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Shimmering Heights", {
        { "stringMaterial", choiceNorm(4, 8) }, { "brightness", 0.82f }, { "timbre", 0.52f },
        { "decayTime", 0.68f }, { "bodySize", 0.45f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.45f },
        { "pluckPosition", 0.65f }, { "fingerHardness", 0.68f }, { "attackNoise", 0.58f },
        { "stringTension", 0.72f }, { "stringStiffness", 0.4f }, { "bridgeBrightness", 0.72f },
        { "humanize", 0.1f }, { "stringCrosstalk", 0.25f }, { "sympatheticQ", 0.13f },
        { "bodyModeSpread", 0.5f }, { "stringGauge", 0.56f }, { "stringLength", 0.5f },
        { "chorusBypass", 0.0f }, { "chorusRate", 0.04f }, { "chorusDepth", 0.3f }, { "chorusMix", 0.15f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Warm Metallic", {
        { "stringMaterial", choiceNorm(4, 8) }, { "brightness", 0.6f }, { "timbre", 0.7f },
        { "decayTime", 0.62f }, { "bodySize", 0.68f }, { "bodyResonance", 0.72f },
        { "woodType", choiceNorm(0, 4) }, { "sympatheticAmount", 0.58f },
        { "pluckPosition", 0.48f }, { "fingerHardness", 0.52f }, { "attackNoise", 0.48f },
        { "stringTension", 0.58f }, { "stringStiffness", 0.3f }, { "bridgeBrightness", 0.58f },
        { "humanize", 0.15f }, { "stringCrosstalk", 0.35f }, { "sympatheticQ", 0.09f },
        { "bodyModeSpread", 0.54f }, { "stringGauge", 0.54f }, { "stringLength", 0.55f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.7f }, { "reverbDamp", 0.35f },
        { "reverbPredelay", 0.15f }, { "reverbMix", 0.3f },
        { "reverbMod", 0.35f }, { "reverbShimmer", 0.15f }
    }, {} });

    presets.push_back({ "Ethereal Chime", {
        { "stringMaterial", choiceNorm(4, 8) }, { "brightness", 0.78f }, { "timbre", 0.58f },
        { "decayTime", 0.75f }, { "bodySize", 0.55f }, { "bodyResonance", 0.62f },
        { "woodType", choiceNorm(1, 4) }, { "sympatheticAmount", 0.65f },
        { "pluckPosition", 0.58f }, { "fingerHardness", 0.62f }, { "attackNoise", 0.52f },
        { "stringTension", 0.68f }, { "stringStiffness", 0.36f }, { "bridgeBrightness", 0.68f },
        { "humanize", 0.1f }, { "stringCrosstalk", 0.32f }, { "sympatheticQ", 0.11f },
        { "bodyModeSpread", 0.55f }, { "stringGauge", 0.55f }, { "stringLength", 0.54f },
        { "chorusBypass", 0.0f }, { "chorusRate", 0.03f }, { "chorusDepth", 0.35f }, { "chorusMix", 0.18f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.55f }, { "reverbDamp", 0.45f },
        { "reverbPredelay", 0.1f }, { "reverbMix", 0.25f },
        { "reverbMod", 0.3f }, { "reverbShimmer", 0.12f }
    }, {} });

    // ========================================================================
    // GLASS STRINGS (Material 5) - Crystalline, fragile, delicate
    // ========================================================================
    presets.push_back({ "Crystalline Voice", {
        { "stringMaterial", choiceNorm(5, 8) }, { "brightness", 0.85f }, { "timbre", 0.48f },
        { "decayTime", 0.55f }, { "bodySize", 0.4f }, { "bodyResonance", 0.5f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.4f },
        { "pluckPosition", 0.65f }, { "fingerHardness", 0.72f }, { "attackNoise", 0.45f },
        { "stringTension", 0.75f }, { "stringStiffness", 0.42f }, { "bridgeBrightness", 0.75f },
        { "humanize", 0.08f }, { "stringCrosstalk", 0.15f }, { "sympatheticQ", 0.18f },
        { "bodyModeSpread", 0.5f }, { "stringGauge", 0.4f }, { "stringLength", 0.52f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.45f }, { "reverbDamp", 0.3f },
        { "reverbPredelay", 0.05f }, { "reverbMix", 0.2f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Fragile Beauty", {
        { "stringMaterial", choiceNorm(5, 8) }, { "brightness", 0.8f }, { "timbre", 0.52f },
        { "decayTime", 0.5f }, { "bodySize", 0.42f }, { "bodyResonance", 0.52f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.38f },
        { "pluckPosition", 0.62f }, { "fingerHardness", 0.68f }, { "attackNoise", 0.4f },
        { "stringTension", 0.72f }, { "stringStiffness", 0.4f }, { "bridgeBrightness", 0.72f },
        { "humanize", 0.1f }, { "stringCrosstalk", 0.14f }, { "sympatheticQ", 0.16f },
        { "bodyModeSpread", 0.49f }, { "stringGauge", 0.42f }, { "stringLength", 0.5f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Ice Palace", {
        { "stringMaterial", choiceNorm(5, 8) }, { "brightness", 0.88f }, { "timbre", 0.45f },
        { "decayTime", 0.6f }, { "bodySize", 0.35f }, { "bodyResonance", 0.45f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.35f },
        { "pluckPosition", 0.7f }, { "fingerHardness", 0.75f }, { "attackNoise", 0.38f },
        { "stringTension", 0.78f }, { "stringStiffness", 0.45f }, { "bridgeBrightness", 0.78f },
        { "humanize", 0.05f }, { "stringCrosstalk", 0.12f }, { "sympatheticQ", 0.2f },
        { "bodyModeSpread", 0.48f }, { "stringGauge", 0.38f }, { "stringLength", 0.55f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.75f }, { "reverbDamp", 0.25f },
        { "reverbPredelay", 0.2f }, { "reverbMix", 0.3f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Winter Bells", {
        { "stringMaterial", choiceNorm(5, 8) }, { "brightness", 0.82f }, { "timbre", 0.5f },
        { "decayTime", 0.58f }, { "bodySize", 0.38f }, { "bodyResonance", 0.48f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.42f },
        { "pluckPosition", 0.68f }, { "fingerHardness", 0.7f }, { "attackNoise", 0.42f },
        { "stringTension", 0.76f }, { "stringStiffness", 0.43f }, { "bridgeBrightness", 0.76f },
        { "humanize", 0.08f }, { "stringCrosstalk", 0.15f }, { "sympatheticQ", 0.18f },
        { "bodyModeSpread", 0.5f }, { "stringGauge", 0.4f }, { "stringLength", 0.52f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.5f }, { "reverbDamp", 0.35f },
        { "reverbPredelay", 0.1f }, { "reverbMix", 0.22f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Delicate Touch", {
        { "stringMaterial", choiceNorm(5, 8) }, { "brightness", 0.78f }, { "timbre", 0.55f },
        { "decayTime", 0.52f }, { "bodySize", 0.44f }, { "bodyResonance", 0.54f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.36f },
        { "pluckPosition", 0.6f }, { "fingerHardness", 0.65f }, { "attackNoise", 0.35f },
        { "stringTension", 0.7f }, { "stringStiffness", 0.38f }, { "bridgeBrightness", 0.7f },
        { "humanize", 0.12f }, { "stringCrosstalk", 0.13f }, { "sympatheticQ", 0.15f },
        { "bodyModeSpread", 0.51f }, { "stringGauge", 0.44f }, { "stringLength", 0.5f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Harmonic Prism", {
        { "stringMaterial", choiceNorm(5, 8) }, { "brightness", 0.9f }, { "timbre", 0.42f },
        { "decayTime", 0.62f }, { "bodySize", 0.32f }, { "bodyResonance", 0.42f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.3f },
        { "pluckPosition", 0.72f }, { "fingerHardness", 0.78f }, { "attackNoise", 0.32f },
        { "stringTension", 0.8f }, { "stringStiffness", 0.48f }, { "bridgeBrightness", 0.8f },
        { "technique", choiceNorm(1, 4) },
        { "humanize", 0.06f }, { "stringCrosstalk", 0.1f }, { "sympatheticQ", 0.22f },
        { "bodyModeSpread", 0.48f }, { "stringGauge", 0.38f }, { "stringLength", 0.54f },
        { "chorusBypass", 0.0f }, { "chorusRate", 0.05f }, { "chorusDepth", 0.3f }, { "chorusMix", 0.15f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    // ========================================================================
    // CRYSTAL STRINGS (Material 6) - Pure, mystical, resonant
    // ========================================================================
    presets.push_back({ "Pure Resonance", {
        { "stringMaterial", choiceNorm(6, 8) }, { "brightness", 0.8f }, { "timbre", 0.5f },
        { "decayTime", 0.7f }, { "bodySize", 0.45f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.55f },
        { "pluckPosition", 0.6f }, { "fingerHardness", 0.65f }, { "attackNoise", 0.35f },
        { "stringTension", 0.7f }, { "stringStiffness", 0.4f }, { "bridgeBrightness", 0.7f },
        { "humanize", 0.15f }, { "stringCrosstalk", 0.2f }, { "sympatheticQ", 0.12f },
        { "bodyModeSpread", 0.54f }, { "stringGauge", 0.45f }, { "stringLength", 0.52f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Mystical Glow", {
        { "stringMaterial", choiceNorm(6, 8) }, { "brightness", 0.75f }, { "timbre", 0.55f },
        { "decayTime", 0.75f }, { "bodySize", 0.5f }, { "bodyResonance", 0.6f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.6f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.6f }, { "attackNoise", 0.3f },
        { "stringTension", 0.65f }, { "stringStiffness", 0.38f }, { "bridgeBrightness", 0.65f },
        { "humanize", 0.2f }, { "stringCrosstalk", 0.25f }, { "sympatheticQ", 0.13f },
        { "bodyModeSpread", 0.56f }, { "stringGauge", 0.48f }, { "stringLength", 0.55f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.65f }, { "reverbDamp", 0.5f },
        { "reverbPredelay", 0.15f }, { "reverbMix", 0.28f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Sacred Space", {
        { "stringMaterial", choiceNorm(6, 8) }, { "brightness", 0.72f }, { "timbre", 0.58f },
        { "decayTime", 0.8f }, { "bodySize", 0.55f }, { "bodyResonance", 0.65f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.65f },
        { "pluckPosition", 0.52f }, { "fingerHardness", 0.55f }, { "attackNoise", 0.28f },
        { "stringTension", 0.62f }, { "stringStiffness", 0.35f }, { "bridgeBrightness", 0.62f },
        { "humanize", 0.2f }, { "stringCrosstalk", 0.22f }, { "sympatheticQ", 0.14f },
        { "bodyModeSpread", 0.57f }, { "stringGauge", 0.46f }, { "stringLength", 0.56f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.8f }, { "reverbDamp", 0.45f },
        { "reverbPredelay", 0.25f }, { "reverbMix", 0.35f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Singing Bowls", {
        { "stringMaterial", choiceNorm(6, 8) }, { "brightness", 0.68f }, { "timbre", 0.62f },
        { "decayTime", 0.85f }, { "bodySize", 0.6f }, { "bodyResonance", 0.7f },
        { "woodType", choiceNorm(2, 4) }, { "sympatheticAmount", 0.7f },
        { "pluckPosition", 0.48f }, { "fingerHardness", 0.5f }, { "attackNoise", 0.25f },
        { "stringTension", 0.58f }, { "stringStiffness", 0.32f }, { "bridgeBrightness", 0.58f },
        { "humanize", 0.25f }, { "stringCrosstalk", 0.28f }, { "sympatheticQ", 0.12f },
        { "bodyModeSpread", 0.55f }, { "stringGauge", 0.5f }, { "stringLength", 0.53f },
        { "chorusBypass", 0.0f }, { "chorusRate", 0.02f }, { "chorusDepth", 0.25f }, { "chorusMix", 0.12f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.6f }, { "reverbDamp", 0.55f },
        { "reverbPredelay", 0.1f }, { "reverbMix", 0.25f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Meditation", {
        { "stringMaterial", choiceNorm(6, 8) }, { "brightness", 0.65f }, { "timbre", 0.65f },
        { "decayTime", 0.9f }, { "bodySize", 0.65f }, { "bodyResonance", 0.72f },
        { "woodType", choiceNorm(2, 4) }, { "sympatheticAmount", 0.72f },
        { "pluckPosition", 0.45f }, { "fingerHardness", 0.45f }, { "attackNoise", 0.2f },
        { "stringTension", 0.55f }, { "stringStiffness", 0.28f }, { "bridgeBrightness", 0.55f },
        { "humanize", 0.25f }, { "stringCrosstalk", 0.25f }, { "sympatheticQ", 0.1f },
        { "bodyModeSpread", 0.58f }, { "stringGauge", 0.48f }, { "stringLength", 0.58f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.7f }, { "reverbDamp", 0.6f },
        { "reverbPredelay", 0.15f }, { "reverbMix", 0.3f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Angelic Choir", {
        { "stringMaterial", choiceNorm(6, 8) }, { "brightness", 0.78f }, { "timbre", 0.52f },
        { "decayTime", 0.82f }, { "bodySize", 0.48f }, { "bodyResonance", 0.58f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.62f },
        { "pluckPosition", 0.58f }, { "fingerHardness", 0.62f }, { "attackNoise", 0.22f },
        { "stringTension", 0.68f }, { "stringStiffness", 0.36f }, { "bridgeBrightness", 0.68f },
        { "technique", choiceNorm(1, 4) },
        { "humanize", 0.2f }, { "stringCrosstalk", 0.2f }, { "sympatheticQ", 0.15f },
        { "bodyModeSpread", 0.55f }, { "stringGauge", 0.44f }, { "stringLength", 0.55f },
        { "chorusBypass", 0.0f }, { "chorusRate", 0.04f }, { "chorusDepth", 0.35f }, { "chorusMix", 0.18f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.65f }, { "reverbDamp", 0.4f },
        { "reverbPredelay", 0.15f }, { "reverbMix", 0.28f },
        { "reverbMod", 0.3f }, { "reverbShimmer", 0.1f }
    }, {} });

    // ========================================================================
    // ENERGY STRINGS (Material 7) - Futuristic, synthetic, experimental
    // ========================================================================
    presets.push_back({ "Quantum Strings", {
        { "stringMaterial", choiceNorm(7, 8) }, { "brightness", 0.85f }, { "timbre", 0.4f },
        { "decayTime", 0.6f }, { "bodySize", 0.35f }, { "bodyResonance", 0.45f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.35f },
        { "pluckPosition", 0.7f }, { "fingerHardness", 0.8f }, { "attackNoise", 0.6f },
        { "stringTension", 0.8f }, { "stringStiffness", 0.5f }, { "bridgeBrightness", 0.8f },
        { "humanize", 0.25f }, { "stringCrosstalk", 0.1f }, { "sympatheticQ", 0.1f },
        { "bodyModeSpread", 0.48f }, { "stringGauge", 0.4f }, { "stringLength", 0.45f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayTime", 0.08f }, { "delayFeedback", 0.35f }, { "delayMix", 0.2f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Plasma Resonance", {
        { "stringMaterial", choiceNorm(7, 8) }, { "brightness", 0.9f }, { "timbre", 0.35f },
        { "decayTime", 0.65f }, { "bodySize", 0.3f }, { "bodyResonance", 0.4f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.4f },
        { "pluckPosition", 0.75f }, { "fingerHardness", 0.85f }, { "attackNoise", 0.65f },
        { "stringTension", 0.85f }, { "stringStiffness", 0.55f }, { "bridgeBrightness", 0.85f },
        { "humanize", 0.3f }, { "stringCrosstalk", 0.08f }, { "sympatheticQ", 0.12f },
        { "bodyModeSpread", 0.46f }, { "stringGauge", 0.38f }, { "stringLength", 0.42f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayTime", 0.1f }, { "delayFeedback", 0.4f }, { "delayMix", 0.18f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.4f }, { "reverbDamp", 0.3f },
        { "reverbPredelay", 0.05f }, { "reverbMix", 0.15f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Electric Dreams", {
        { "stringMaterial", choiceNorm(7, 8) }, { "brightness", 0.82f }, { "timbre", 0.45f },
        { "decayTime", 0.55f }, { "bodySize", 0.4f }, { "bodyResonance", 0.5f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.45f },
        { "pluckPosition", 0.65f }, { "fingerHardness", 0.75f }, { "attackNoise", 0.55f },
        { "stringTension", 0.75f }, { "stringStiffness", 0.45f }, { "bridgeBrightness", 0.75f },
        { "humanize", 0.3f }, { "stringCrosstalk", 0.12f }, { "sympatheticQ", 0.09f },
        { "bodyModeSpread", 0.5f }, { "stringGauge", 0.42f }, { "stringLength", 0.48f },
        { "chorusBypass", 0.0f }, { "chorusRate", 0.08f }, { "chorusDepth", 0.4f }, { "chorusMix", 0.2f },
        { "delayBypass", 0.0f }, { "delayTime", 0.15f }, { "delayFeedback", 0.35f }, { "delayMix", 0.18f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Cosmic Harp", {
        { "stringMaterial", choiceNorm(7, 8) }, { "brightness", 0.78f }, { "timbre", 0.5f },
        { "decayTime", 0.7f }, { "bodySize", 0.45f }, { "bodyResonance", 0.55f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.55f },
        { "pluckPosition", 0.6f }, { "fingerHardness", 0.7f }, { "attackNoise", 0.5f },
        { "stringTension", 0.7f }, { "stringStiffness", 0.42f }, { "bridgeBrightness", 0.7f },
        { "humanize", 0.25f }, { "stringCrosstalk", 0.15f }, { "sympatheticQ", 0.1f },
        { "bodyModeSpread", 0.52f }, { "stringGauge", 0.45f }, { "stringLength", 0.5f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayTime", 0.2f }, { "delayFeedback", 0.4f },
        { "delayMode", 1.0f }, { "delayMix", 0.2f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbSize", 0.6f }, { "reverbDamp", 0.45f },
        { "reverbPredelay", 0.1f }, { "reverbMix", 0.25f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Neon Glow", {
        { "stringMaterial", choiceNorm(7, 8) }, { "brightness", 0.88f }, { "timbre", 0.38f },
        { "decayTime", 0.58f }, { "bodySize", 0.32f }, { "bodyResonance", 0.42f },
        { "woodType", choiceNorm(3, 4) }, { "sympatheticAmount", 0.38f },
        { "pluckPosition", 0.72f }, { "fingerHardness", 0.82f }, { "attackNoise", 0.58f },
        { "stringTension", 0.82f }, { "stringStiffness", 0.52f }, { "bridgeBrightness", 0.82f },
        { "humanize", 0.3f }, { "stringCrosstalk", 0.08f }, { "sympatheticQ", 0.12f },
        { "bodyModeSpread", 0.47f }, { "stringGauge", 0.38f }, { "stringLength", 0.44f },
        { "chorusBypass", 0.0f }, { "chorusRate", 0.1f }, { "chorusDepth", 0.35f }, { "chorusMix", 0.18f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    presets.push_back({ "Future Primitive", {
        { "stringMaterial", choiceNorm(7, 8) }, { "brightness", 0.75f }, { "timbre", 0.55f },
        { "decayTime", 0.62f }, { "bodySize", 0.5f }, { "bodyResonance", 0.58f },
        { "woodType", choiceNorm(2, 4) }, { "sympatheticAmount", 0.5f },
        { "pluckPosition", 0.55f }, { "fingerHardness", 0.68f }, { "attackNoise", 0.52f },
        { "stringTension", 0.68f }, { "stringStiffness", 0.4f }, { "bridgeBrightness", 0.68f },
        { "humanize", 0.35f }, { "stringCrosstalk", 0.18f }, { "sympatheticQ", 0.08f },
        { "bodyModeSpread", 0.53f }, { "stringGauge", 0.5f }, { "stringLength", 0.48f },
        { "chorusBypass", 0.0f }, { "chorusMix", 0.0f },
        { "delayBypass", 0.0f }, { "delayMix", 0.0f },
        { "eqBypass", 0.0f }, { "eqLowGain", 0.5f }, { "eqMidGain", 0.5f }, { "eqHighGain", 0.5f },
        { "reverbBypass", 0.0f }, { "reverbMix", 0.0f },
        { "reverbMod", 0.2f }, { "reverbShimmer", 0.0f }
    }, {} });

    // Initialize all presets and write version marker
    presetManager.initializeFactoryPresets(presets);
    factoryDir.getChildFile(".version").replaceWithText(kFactoryPresetVersion);
    DBG("[O-Lyrica] Initialized " + juce::String(presets.size()) + " factory presets (v" + kFactoryPresetVersion + ")");
}


// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OLyricaAudioProcessor();
}
