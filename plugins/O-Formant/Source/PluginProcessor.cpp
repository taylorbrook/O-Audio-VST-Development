/*
  ==============================================================================

    PluginProcessor.cpp
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FormantVoice.h"
#include "dsp/GlottalTableGenerator.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OFormantAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- Vowel Morph (3) ---
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vowelX", 1 },
        "Vowel X",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vowelY", 1 },
        "Vowel Y",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vowelFocus", 1 },
        "Vowel Focus",
        juce::NormalisableRange<float> (1.0f, 6.0f, 0.0f),
        2.5f));

    // --- Glottal Source (5) ---
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "glottalRd", 1 },
        "Voice Quality",
        juce::NormalisableRange<float> (0.3f, 2.7f, 0.0f),
        1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "breathiness", 1 },
        "Breathiness",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.1f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vibratoRate", 1 },
        "Vibrato Rate",
        juce::NormalisableRange<float> (0.5f, 12.0f, 0.0f),
        5.5f,
        "Hz"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vibratoDepth", 1 },
        "Vibrato Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.0f),
        15.0f,
        "cents"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vibratoDelay", 1 },
        "Vibrato Delay",
        juce::NormalisableRange<float> (0.0f, 2000.0f, 0.0f, 0.4f),
        300.0f,
        "ms"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "jitter", 1 },
        "Jitter",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.15f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "shimmer", 1 },
        "Shimmer",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.1f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rdModDepth", 1 },
        "Rd Mod Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "spectralTilt", 1 },
        "Spectral Tilt",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f),
        0.0f,
        "dB"));

    // --- Consonant / Noise (4) ---
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "consonantLevel", 1 },
        "Consonant Level",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.0f),
        0.3f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "consonantTone", 1 },
        "Place",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sibilance", 1 },
        "Manner",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "consonantVoicing", 1 },
        "Voicing",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "autoConsonant", 1 },
        "Auto Consonant",
        false));

    // --- Consonant Envelope (3) ---
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "consonantAttack", 1 },
        "Cons Attack",
        juce::NormalisableRange<float> (1.0f, 100.0f, 0.1f, 0.5f),
        20.0f,
        "ms"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "consonantHold", 1 },
        "Cons Hold",
        juce::NormalisableRange<float> (0.0f, 200.0f, 0.1f),
        30.0f,
        "ms"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "consonantDecay", 1 },
        "Cons Decay",
        juce::NormalisableRange<float> (5.0f, 200.0f, 0.1f, 0.5f),
        40.0f,
        "ms"));

    // VOT scale for voiceless plosive aspiration (0 = no aspiration, 1 = 2x nominal)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "consonantVOT", 1 },
        "VOT",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    // F2/F3 locus transition amount (Delattre-Liberman-Cooper 1955;
    // Kewley-Port 1982 τ=15ms). 0 = legacy off, 1 = full DLC locus pull.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "consonantTransition", 1 },
        "Transition",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    // --- Envelope (4) ---
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 },
        "Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.3f),
        0.01f,
        "s"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "decay", 1 },
        "Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.3f),
        0.3f,
        "s"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sustain", 1 },
        "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.8f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 },
        "Release",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.0f, 0.3f),
        0.5f,
        "s"));

    // --- Voice Character (4) ---
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "formantTopology", 1 },
        "Formant Topology",
        juce::StringArray { "Cascade", "Parallel", "Hybrid" },
        0)); // default: Cascade (Klatt series resonators)

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "formantShift", 1 },
        "Formant Shift",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.0f),
        0.0f,
        "st"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "formantSpread", 1 },
        "Formant Spread",
        juce::NormalisableRange<float> (0.5f, 2.0f, 0.0f),
        1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pitchGlide", 1 },
        "Pitch Glide",
        juce::NormalisableRange<float> (0.0f, 1000.0f, 0.0f, 0.3f),
        0.0f,
        "ms"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "transitionTime", 1 },
        "Transition Time",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.4f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sourceFilterCoupling", 1 },
        "Source-Filter Coupling",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.3f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "singersFormant", 1 },
        "Singer's Formant",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.0f));

    // --- Nasal (2) ---
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "nasalCoupling", 1 },
        "Nasality",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "nasalPlace", 1 },
        "Nasal Place",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    // --- Output (2) ---
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.0f),
        0.0f,
        "dB"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "stereoWidth", 1 },
        "Stereo Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    // --- Tuning (5) ---
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tuning_masterTune", 1 },
        "Master Tune",
        juce::NormalisableRange<float> (400.0f, 480.0f, 0.1f),
        440.0f,
        "Hz"));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "tuning_tuningMode", 1 },
        "Tuning Mode",
        juce::StringArray { "12-TET", "Custom", "MTS-ESP" },
        0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tuning_octaveStretch", 1 },
        "Octave Stretch",
        juce::NormalisableRange<float> (0.95f, 1.25f, 0.001f),
        1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tuning_pitchBendRange", 1 },
        "Pitch Bend Range",
        juce::NormalisableRange<float> (1.0f, 48.0f, 1.0f),
        2.0f,
        "st"));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "tuning_temperamentPreset", 1 },
        "Temperament",
        juce::StringArray {
            "Equal 12-TET", "Pythagorean", "Zarlino", "Meantone (1/4)",
            "Werckmeister III", "Kirnberger III", "Vallotti",
            "Well Tempered", "Just Intonation", "Bohlen-Pierce", "Custom" },
        0));

    // --- Effects: Chorus (4) ---
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "chorusBypass", 1 },
        "Chorus Bypass",
        false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorusRate", 1 },
        "Chorus Rate",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.0f),
        1.0f,
        "Hz"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorusDepth", 1 },
        "Chorus Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorusMix", 1 },
        "Chorus Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.0f));

    // --- Effects: Delay (5) ---
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "delayBypass", 1 },
        "Delay Bypass",
        false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayTime", 1 },
        "Delay Time",
        juce::NormalisableRange<float> (0.001f, 2.0f, 0.0f),
        0.375f,
        "s"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayFeedback", 1 },
        "Delay Feedback",
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.0f),
        0.3f));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "delayMode", 1 },
        "Delay Mode",
        juce::StringArray { "Normal", "PingPong" },
        0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayMix", 1 },
        "Delay Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.0f));

    // --- Effects: Reverb (7) ---
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "reverbBypass", 1 },
        "Reverb Bypass",
        false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbSize", 1 },
        "Reverb Size",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbDamp", 1 },
        "Reverb Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbPredelay", 1 },
        "Reverb Pre-delay",
        juce::NormalisableRange<float> (0.0f, 200.0f, 0.0f),
        20.0f,
        "ms"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbMix", 1 },
        "Reverb Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbMod", 1 },
        "Reverb Modulation",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.2f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbShimmer", 1 },
        "Reverb Shimmer",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
        0.0f));

    // --- Effects: EQ (5) ---
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "eqBypass", 1 },
        "EQ Bypass",
        false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "eqLowGain", 1 },
        "EQ Low Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f),
        0.0f,
        "dB"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "eqMidGain", 1 },
        "EQ Mid Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f),
        0.0f,
        "dB"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "eqMidFreq", 1 },
        "EQ Mid Freq",
        juce::NormalisableRange<float> (200.0f, 8000.0f, 0.0f),
        1000.0f,
        "Hz"));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "eqHighGain", 1 },
        "EQ High Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.0f),
        0.0f,
        "dB"));

    // --- Lyrics (1) ---
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "lyricsEnabled", 1 },
        "Lyrics Enabled",
        false));

    return layout;
}

//==============================================================================
OFormantAudioProcessor::OFormantAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , parameters (*this, nullptr, "Parameters", createParameterLayout())
    , presetManager (parameters, "O-Formant")
{
    // Generate glottal wavetable at construction (before any audio processing)
    GlottalTableGenerator::generate (glottalWavetable);

    // Initialize 16 factory presets (4 categories x 4)
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        // ── Cinematic ──
        { "Cinematic", "Creature Growl", {
            {"vowelX", 0.8f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 0.4f}, {"breathiness", 0.3f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.3f}, {"shimmer", 0.3f}, {"rdModDepth", 0.3f}, {"spectralTilt", -3.0f},
            {"consonantLevel", 0.6f}, {"consonantTone", 0.95f}, {"sibilance", 0.1f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 1.0f},
            {"consonantAttack", 5.0f}, {"consonantHold", 15.0f}, {"consonantDecay", 30.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", -12.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.6f}, {"sourceFilterCoupling", 0.5f},
            {"singersFormant", 0.0f},
            {"nasalCoupling", 0.2f}, {"nasalPlace", 0.3f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Cinematic", "Alien Whisper", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 1.5f},
            {"glottalRd", 2.5f}, {"breathiness", 0.7f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.2f}, {"shimmer", 0.15f}, {"rdModDepth", 0.2f}, {"spectralTilt", 4.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.67f}, {"sibilance", 0.85f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 0.0f},
            {"consonantAttack", 40.0f}, {"consonantHold", 10.0f}, {"consonantDecay", 80.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 12.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.7f}, {"sourceFilterCoupling", 0.2f},
            {"singersFormant", 0.0f},
            {"nasalCoupling", 0.0f}, {"nasalPlace", 0.5f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Cinematic", "Sci-Fi Choir", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 1.0f}, {"breathiness", 0.1f},
            {"vibratoRate", 4.0f}, {"vibratoDepth", 20.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.05f}, {"shimmer", 0.05f}, {"rdModDepth", 0.4f}, {"spectralTilt", 0.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.1f}, {"sibilance", 0.7f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 0.0f},
            {"consonantAttack", 25.0f}, {"consonantHold", 20.0f}, {"consonantDecay", 50.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 1.0f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.5f}, {"sourceFilterCoupling", 0.3f},
            {"singersFormant", 0.4f},
            {"nasalCoupling", 0.15f}, {"nasalPlace", 0.5f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.9f}
        }, juce::var() },
        { "Cinematic", "Spectral Voice", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 4.0f},
            {"glottalRd", 1.8f}, {"breathiness", 0.2f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.1f}, {"shimmer", 0.08f}, {"rdModDepth", 0.3f}, {"spectralTilt", 2.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.33f}, {"sibilance", 0.4f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 0.0f},
            {"consonantAttack", 35.0f}, {"consonantHold", 15.0f}, {"consonantDecay", 70.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 3.0f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.8f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.5f}, {"sourceFilterCoupling", 0.4f},
            {"singersFormant", 0.0f},
            {"nasalCoupling", 0.0f}, {"nasalPlace", 0.5f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },

        // ── Electronic ──
        { "Electronic", "Formant Bass", {
            {"vowelX", 0.8f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 0.3f}, {"breathiness", 0.1f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.08f}, {"shimmer", 0.05f}, {"rdModDepth", 0.2f}, {"spectralTilt", -2.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.0f}, {"sibilance", 0.05f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 1.0f},
            {"consonantAttack", 3.0f}, {"consonantHold", 10.0f}, {"consonantDecay", 20.0f},
            {"attack", 0.001f}, {"decay", 0.5f}, {"sustain", 0.4f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", -18.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.3f}, {"sourceFilterCoupling", 0.6f},
            {"singersFormant", 0.0f},
            {"nasalCoupling", 0.0f}, {"nasalPlace", 0.5f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Electronic", "Vowel Pad", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 1.2f}, {"breathiness", 0.1f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 8.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.05f}, {"shimmer", 0.03f}, {"rdModDepth", 0.3f}, {"spectralTilt", 1.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.1f}, {"sibilance", 0.9f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 0.0f},
            {"consonantAttack", 50.0f}, {"consonantHold", 40.0f}, {"consonantDecay", 80.0f},
            {"attack", 0.8f}, {"decay", 0.3f}, {"sustain", 1.0f}, {"release", 2.0f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.6f}, {"sourceFilterCoupling", 0.3f},
            {"singersFormant", 0.0f},
            {"nasalCoupling", 0.1f}, {"nasalPlace", 0.7f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.7f}
        }, juce::var() },
        { "Electronic", "Glitch Vocal", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 0.5f}, {"breathiness", 0.1f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.4f}, {"shimmer", 0.35f}, {"rdModDepth", 0.1f}, {"spectralTilt", -4.0f},
            {"consonantLevel", 0.8f}, {"consonantTone", 0.33f}, {"sibilance", 0.15f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 1.0f},
            {"consonantAttack", 1.0f}, {"consonantHold", 5.0f}, {"consonantDecay", 15.0f},
            {"attack", 0.001f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.1f}, {"sourceFilterCoupling", 0.1f},
            {"singersFormant", 0.0f},
            {"nasalCoupling", 0.0f}, {"nasalPlace", 0.5f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Electronic", "Robotic Speech", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 5.5f},
            {"glottalRd", 1.0f}, {"breathiness", 0.0f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 0.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.0f}, {"shimmer", 0.0f}, {"rdModDepth", 0.0f}, {"spectralTilt", 0.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.33f}, {"sibilance", 0.85f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 1.0f},
            {"consonantAttack", 5.0f}, {"consonantHold", 25.0f}, {"consonantDecay", 25.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 0.8f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.0f}, {"sourceFilterCoupling", 0.0f},
            {"singersFormant", 0.0f},
            {"nasalCoupling", 0.0f}, {"nasalPlace", 0.5f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },

        // ── Ambient ──
        { "Ambient", "Ethereal Drone", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 2.2f}, {"breathiness", 0.5f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.05f}, {"shimmer", 0.03f}, {"rdModDepth", 0.3f}, {"spectralTilt", 3.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.67f}, {"sibilance", 0.9f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 0.0f},
            {"consonantAttack", 60.0f}, {"consonantHold", 50.0f}, {"consonantDecay", 100.0f},
            {"attack", 2.0f}, {"decay", 0.3f}, {"sustain", 1.0f}, {"release", 5.0f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.8f}, {"sourceFilterCoupling", 0.2f},
            {"singersFormant", 0.0f},
            {"nasalCoupling", 0.1f}, {"nasalPlace", 0.6f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.8f}
        }, juce::var() },
        { "Ambient", "Breath Texture", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 1.0f},
            {"glottalRd", 2.7f}, {"breathiness", 0.9f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.25f}, {"shimmer", 0.2f}, {"rdModDepth", 0.2f}, {"spectralTilt", 5.0f},
            {"consonantLevel", 0.4f}, {"consonantTone", 0.1f}, {"sibilance", 0.95f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 0.0f},
            {"consonantAttack", 40.0f}, {"consonantHold", 15.0f}, {"consonantDecay", 90.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.5f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.5f}, {"sourceFilterCoupling", 0.1f},
            {"singersFormant", 0.0f},
            {"nasalCoupling", 0.0f}, {"nasalPlace", 0.5f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Ambient", "Overtone Chant", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 5.0f},
            {"glottalRd", 0.8f}, {"breathiness", 0.1f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 500.0f},
            {"jitter", 0.1f}, {"shimmer", 0.08f}, {"rdModDepth", 0.4f}, {"spectralTilt", -2.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.9f}, {"sibilance", 0.3f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 1.0f},
            {"consonantAttack", 20.0f}, {"consonantHold", 30.0f}, {"consonantDecay", 40.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 0.7f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.4f}, {"sourceFilterCoupling", 0.7f},
            {"singersFormant", 0.7f},
            {"nasalCoupling", 0.35f}, {"nasalPlace", 0.4f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Ambient", "Wind Voice", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 2.0f}, {"breathiness", 0.6f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.2f}, {"shimmer", 0.15f}, {"rdModDepth", 0.3f}, {"spectralTilt", 2.0f},
            {"consonantLevel", 0.5f}, {"consonantTone", 0.5f}, {"sibilance", 0.8f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 0.0f},
            {"consonantAttack", 45.0f}, {"consonantHold", 20.0f}, {"consonantDecay", 80.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 4.0f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.7f}, {"sourceFilterCoupling", 0.2f},
            {"singersFormant", 0.0f},
            {"nasalCoupling", 0.1f}, {"nasalPlace", 0.5f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },

        // ── Speech ──
        { "Speech", "Natural Tenor", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 1.0f}, {"breathiness", 0.1f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.15f}, {"shimmer", 0.1f}, {"rdModDepth", 0.6f}, {"spectralTilt", 0.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.33f}, {"sibilance", 0.5f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 1.0f},
            {"consonantAttack", 15.0f}, {"consonantHold", 25.0f}, {"consonantDecay", 35.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.4f}, {"sourceFilterCoupling", 0.4f},
            {"singersFormant", 0.5f},
            {"nasalCoupling", 0.2f}, {"nasalPlace", 0.5f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Speech", "Breathy Soprano", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 2.0f}, {"breathiness", 0.4f},
            {"vibratoRate", 6.0f}, {"vibratoDepth", 12.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.2f}, {"shimmer", 0.12f}, {"rdModDepth", 0.5f}, {"spectralTilt", 3.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.5f}, {"sibilance", 0.7f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 1.0f},
            {"consonantAttack", 25.0f}, {"consonantHold", 20.0f}, {"consonantDecay", 45.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 8.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.5f}, {"sourceFilterCoupling", 0.3f},
            {"singersFormant", 0.3f},
            {"nasalCoupling", 0.15f}, {"nasalPlace", 0.6f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Speech", "Pressed Baritone", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 3.0f},
            {"glottalRd", 0.4f}, {"breathiness", 0.0f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 10.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.1f}, {"shimmer", 0.08f}, {"rdModDepth", 0.7f}, {"spectralTilt", -3.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.9f}, {"sibilance", 0.1f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 1.0f},
            {"consonantAttack", 8.0f}, {"consonantHold", 30.0f}, {"consonantDecay", 30.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", -8.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.3f}, {"sourceFilterCoupling", 0.5f},
            {"singersFormant", 0.7f},
            {"nasalCoupling", 0.25f}, {"nasalPlace", 0.4f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Speech", "Child Voice", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 1.5f}, {"breathiness", 0.2f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 5.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.18f}, {"shimmer", 0.12f}, {"rdModDepth", 0.5f}, {"spectralTilt", 1.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.4f}, {"sibilance", 0.6f}, {"consonantVoicing", 0.5f}, {"autoConsonant", 1.0f},
            {"consonantAttack", 18.0f}, {"consonantHold", 15.0f}, {"consonantDecay", 35.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 18.0f}, {"formantSpread", 1.3f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.4f}, {"sourceFilterCoupling", 0.3f},
            {"singersFormant", 0.0f},
            {"nasalCoupling", 0.1f}, {"nasalPlace", 0.6f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
    };
    presetManager.initializeFactoryPresets (factoryPresets);

    // Create 16 FormantVoice instances
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new FormantVoice (i);
        voice->setAPVTS (&parameters);
        voice->setWavetable (&glottalWavetable);
        voice->setTuningEngine (&tuningEngine);
        voice->setLyricsEngine (&lyricsEngine);
        voice->setPendingTuningSource (&vst3Extensions.getPendingTable()); // Phase 24: NE
        synthesiser.addVoice (voice);
    }

    // Enable legacy mode for standard MIDI (channel range 1-16, pitchbend +/-2 semitones)
    synthesiser.enableLegacyMode (2, juce::Range<int> (1, 17));
}

OFormantAudioProcessor::~OFormantAudioProcessor()
{
}

//==============================================================================
void OFormantAudioProcessor::cacheParamPointers()
{
    // Fetch the effects/output raw-parameter pointers once (IN-02). getRawParameterValue
    // returns a lifetime-stable std::atomic<float>*; processBlock then reads ->load()
    // directly instead of doing a string-keyed hash lookup per parameter per block.
    fxParams.chorusBypass  = parameters.getRawParameterValue ("chorusBypass");
    fxParams.chorusMix     = parameters.getRawParameterValue ("chorusMix");
    fxParams.chorusRate    = parameters.getRawParameterValue ("chorusRate");
    fxParams.chorusDepth   = parameters.getRawParameterValue ("chorusDepth");

    fxParams.delayBypass   = parameters.getRawParameterValue ("delayBypass");
    fxParams.delayMix      = parameters.getRawParameterValue ("delayMix");
    fxParams.delayTime     = parameters.getRawParameterValue ("delayTime");
    fxParams.delayFeedback = parameters.getRawParameterValue ("delayFeedback");
    fxParams.delayMode     = parameters.getRawParameterValue ("delayMode");

    fxParams.reverbBypass   = parameters.getRawParameterValue ("reverbBypass");
    fxParams.reverbMix      = parameters.getRawParameterValue ("reverbMix");
    fxParams.reverbSize     = parameters.getRawParameterValue ("reverbSize");
    fxParams.reverbDamp     = parameters.getRawParameterValue ("reverbDamp");
    fxParams.reverbPredelay = parameters.getRawParameterValue ("reverbPredelay");
    fxParams.reverbMod      = parameters.getRawParameterValue ("reverbMod");
    fxParams.reverbShimmer  = parameters.getRawParameterValue ("reverbShimmer");

    fxParams.eqBypass   = parameters.getRawParameterValue ("eqBypass");
    fxParams.eqLowGain  = parameters.getRawParameterValue ("eqLowGain");
    fxParams.eqMidGain  = parameters.getRawParameterValue ("eqMidGain");
    fxParams.eqMidFreq  = parameters.getRawParameterValue ("eqMidFreq");
    fxParams.eqHighGain = parameters.getRawParameterValue ("eqHighGain");

    fxParams.outputGain = parameters.getRawParameterValue ("outputGain");
}

void OFormantAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synthesiser.setCurrentPlaybackSampleRate (sampleRate);
    setLatencySamples (0);
    outputGainSmoothed.reset (sampleRate, 0.050);

    cacheParamPointers();

    // Prepare all voices with current sample rate
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<FormantVoice*> (synthesiser.getVoice (i)))
            voice->prepare (sampleRate);
    }

    // Prepare effects chain
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };

    chorus.prepare (spec);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.0f);

    delayProcessor.prepare (spec);
    reverbProcessor.prepare (spec);
    eqProcessor.prepare (spec);

    lastSampleRate = sampleRate;
}

void OFormantAudioProcessor::releaseResources()
{
}

void OFormantAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // VST3 Note Expression: drain the JUCE wrapper's raw-event queue and
    // correlate tuning deltas to their NoteOn's MIDI pitch.
    vst3Extensions.drainAndUpdate();

    synthesiser.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    // ── Effects chain: Chorus -> Delay -> Reverb -> EQ ──
    juce::dsp::AudioBlock<float> block (buffer);

    // Chorus
    {
        bool bypassed = fxParams.chorusBypass->load() >= 0.5f;
        float mix = fxParams.chorusMix->load();
        if (! bypassed && mix > 0.001f)
        {
            chorus.setRate (fxParams.chorusRate->load());
            chorus.setDepth (fxParams.chorusDepth->load());
            chorus.setMix (mix);
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            chorus.process (ctx);
        }
    }

    // Delay
    {
        bool bypassed = fxParams.delayBypass->load() >= 0.5f;
        float mix = fxParams.delayMix->load();
        if (! bypassed && mix > 0.001f)
        {
            delayProcessor.setTime (fxParams.delayTime->load());
            delayProcessor.setFeedback (fxParams.delayFeedback->load());
            delayProcessor.setMode (static_cast<int> (fxParams.delayMode->load()));
            delayProcessor.setMix (mix);
            delayProcessor.process (block);
        }
    }

    // Reverb
    {
        bool bypassed = fxParams.reverbBypass->load() >= 0.5f;
        float mix = fxParams.reverbMix->load();
        if (! bypassed && mix > 0.001f)
        {
            reverbProcessor.setSize (fxParams.reverbSize->load());
            reverbProcessor.setDamping (fxParams.reverbDamp->load());
            reverbProcessor.setPredelay (fxParams.reverbPredelay->load());
            reverbProcessor.setMix (mix);
            reverbProcessor.setMod (fxParams.reverbMod->load());
            reverbProcessor.setShimmer (fxParams.reverbShimmer->load());
            reverbProcessor.process (block);
        }
    }

    // EQ
    {
        bool bypassed = fxParams.eqBypass->load() >= 0.5f;
        if (! bypassed)
        {
            eqProcessor.setLowGain (fxParams.eqLowGain->load());
            eqProcessor.setMidGain (fxParams.eqMidGain->load());
            eqProcessor.setMidFreq (fxParams.eqMidFreq->load());
            eqProcessor.setHighGain (fxParams.eqHighGain->load());
            eqProcessor.process (block);
        }
    }

    // Post-synth output gain (dB -> linear, smoothed 50ms)
    float targetGain = juce::Decibels::decibelsToGain (fxParams.outputGain->load());
    outputGainSmoothed.setTargetValue (targetGain);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float gain = outputGainSmoothed.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.setSample (ch, i, buffer.getSample (ch, i) * gain);
    }

    // Brickwall safety limiter — hard clip at 0 dBFS
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* channelData = buffer.getWritePointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            channelData[i] = juce::jlimit (-1.0f, 1.0f, channelData[i]);
    }
}

//==============================================================================
bool OFormantAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Output must be stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // No input bus for synth
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;

    return true;
}

//==============================================================================
juce::AudioProcessorEditor* OFormantAudioProcessor::createEditor()
{
    return new OFormantEditor (*this);
}

//==============================================================================
void OFormantAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Save tuning engine state
    auto tuningState = state.getOrCreateChildWithName ("tuningEngine", nullptr);
    auto intervals = tuningEngine.getIntervals();
    juce::String intervalsStr;
    for (size_t i = 0; i < intervals.size(); ++i)
    {
        if (i > 0) intervalsStr += ",";
        intervalsStr += juce::String (intervals[i], 6);
    }
    tuningState.setProperty ("intervals", intervalsStr, nullptr);
    tuningState.setProperty ("scaleName", tuningEngine.getActiveTuningName(), nullptr);
    tuningState.setProperty ("tonic", tuningEngine.getTonicNote(), nullptr);
    tuningState.setProperty ("preset", static_cast<int> (tuningEngine.getBuiltInPreset()), nullptr);

    // Save lyrics engine state
    auto lyricsState = state.getOrCreateChildWithName ("lyricsEngine", nullptr);
    lyricsState.setProperty ("text", lyricsEngine.getLyricsText(), nullptr);
    lyricsState.setProperty ("looping", lyricsEngine.isLooping(), nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("currentPreset", presetManager.getCurrentPresetName());
    copyXmlToBinary (*xml, destData);
}

void OFormantAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
    {
        auto state = juce::ValueTree::fromXml (*xmlState);
        parameters.replaceState (state);
        presetManager.setCurrentPresetName (
            xmlState->getStringAttribute ("currentPreset", "Default"));

        // Restore tuning engine state
        auto tuningState = state.getChildWithName ("tuningEngine");
        if (tuningState.isValid())
        {
            juce::String intervalsStr = tuningState.getProperty ("intervals", "");
            if (intervalsStr.isNotEmpty())
            {
                std::vector<double> intervals;
                juce::StringArray tokens;
                tokens.addTokens (intervalsStr, ",", "");
                for (const auto& token : tokens)
                    intervals.push_back (token.getDoubleValue());

                juce::String scaleName = tuningState.getProperty ("scaleName", "Custom");
                tuningEngine.setCustomIntervals (intervals, scaleName);
            }

            int tonic = tuningState.getProperty ("tonic", 0);
            tuningEngine.setTonicNote (tonic);
        }

        // Restore lyrics engine state
        auto lyricsState = state.getChildWithName ("lyricsEngine");
        if (lyricsState.isValid())
        {
            juce::String text = lyricsState.getProperty ("text", "");
            lyricsEngine.setLyricsText (text);
            bool loop = lyricsState.getProperty ("looping", true);
            lyricsEngine.setLooping (loop);
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OFormantAudioProcessor();
}
