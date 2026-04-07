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
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f),
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
            {"consonantLevel", 0.6f}, {"consonantTone", 0.95f}, {"sibilance", 0.1f}, {"autoConsonant", 1.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", -12.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.6f}, {"sourceFilterCoupling", 0.5f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Cinematic", "Alien Whisper", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 1.5f},
            {"glottalRd", 2.5f}, {"breathiness", 0.7f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.2f}, {"shimmer", 0.15f}, {"rdModDepth", 0.2f}, {"spectralTilt", 4.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.67f}, {"sibilance", 0.85f}, {"autoConsonant", 0.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 12.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.7f}, {"sourceFilterCoupling", 0.2f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Cinematic", "Sci-Fi Choir", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 1.0f}, {"breathiness", 0.1f},
            {"vibratoRate", 4.0f}, {"vibratoDepth", 20.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.05f}, {"shimmer", 0.05f}, {"rdModDepth", 0.4f}, {"spectralTilt", 0.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.1f}, {"sibilance", 0.7f}, {"autoConsonant", 0.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 1.0f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.5f}, {"sourceFilterCoupling", 0.3f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.9f}
        }, juce::var() },
        { "Cinematic", "Spectral Voice", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 4.0f},
            {"glottalRd", 1.8f}, {"breathiness", 0.2f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.1f}, {"shimmer", 0.08f}, {"rdModDepth", 0.3f}, {"spectralTilt", 2.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.33f}, {"sibilance", 0.4f}, {"autoConsonant", 0.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 3.0f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.8f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.5f}, {"sourceFilterCoupling", 0.4f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },

        // ── Electronic ──
        { "Electronic", "Formant Bass", {
            {"vowelX", 0.8f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 0.3f}, {"breathiness", 0.1f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.08f}, {"shimmer", 0.05f}, {"rdModDepth", 0.2f}, {"spectralTilt", -2.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.0f}, {"sibilance", 0.05f}, {"autoConsonant", 1.0f},
            {"attack", 0.001f}, {"decay", 0.5f}, {"sustain", 0.4f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", -18.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.3f}, {"sourceFilterCoupling", 0.6f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Electronic", "Vowel Pad", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 1.2f}, {"breathiness", 0.1f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 8.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.05f}, {"shimmer", 0.03f}, {"rdModDepth", 0.3f}, {"spectralTilt", 1.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.1f}, {"sibilance", 0.9f}, {"autoConsonant", 0.0f},
            {"attack", 0.8f}, {"decay", 0.3f}, {"sustain", 1.0f}, {"release", 2.0f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.6f}, {"sourceFilterCoupling", 0.3f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.7f}
        }, juce::var() },
        { "Electronic", "Glitch Vocal", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 0.5f}, {"breathiness", 0.1f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.4f}, {"shimmer", 0.35f}, {"rdModDepth", 0.1f}, {"spectralTilt", -4.0f},
            {"consonantLevel", 0.8f}, {"consonantTone", 0.33f}, {"sibilance", 0.15f}, {"autoConsonant", 1.0f},
            {"attack", 0.001f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.1f}, {"sourceFilterCoupling", 0.1f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Electronic", "Robotic Speech", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 5.5f},
            {"glottalRd", 1.0f}, {"breathiness", 0.0f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 0.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.0f}, {"shimmer", 0.0f}, {"rdModDepth", 0.0f}, {"spectralTilt", 0.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.33f}, {"sibilance", 0.85f}, {"autoConsonant", 1.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 0.8f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.0f}, {"sourceFilterCoupling", 0.0f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },

        // ── Ambient ──
        { "Ambient", "Ethereal Drone", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 2.2f}, {"breathiness", 0.5f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.05f}, {"shimmer", 0.03f}, {"rdModDepth", 0.3f}, {"spectralTilt", 3.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.67f}, {"sibilance", 0.9f}, {"autoConsonant", 0.0f},
            {"attack", 2.0f}, {"decay", 0.3f}, {"sustain", 1.0f}, {"release", 5.0f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.8f}, {"sourceFilterCoupling", 0.2f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.8f}
        }, juce::var() },
        { "Ambient", "Breath Texture", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 1.0f},
            {"glottalRd", 2.7f}, {"breathiness", 0.9f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.25f}, {"shimmer", 0.2f}, {"rdModDepth", 0.2f}, {"spectralTilt", 5.0f},
            {"consonantLevel", 0.4f}, {"consonantTone", 0.1f}, {"sibilance", 0.95f}, {"autoConsonant", 0.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.5f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.5f}, {"sourceFilterCoupling", 0.1f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Ambient", "Overtone Chant", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 5.0f},
            {"glottalRd", 0.8f}, {"breathiness", 0.1f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 500.0f},
            {"jitter", 0.1f}, {"shimmer", 0.08f}, {"rdModDepth", 0.4f}, {"spectralTilt", -2.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.9f}, {"sibilance", 0.3f}, {"autoConsonant", 1.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 0.7f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.4f}, {"sourceFilterCoupling", 0.7f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Ambient", "Wind Voice", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 2.0f}, {"breathiness", 0.6f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.2f}, {"shimmer", 0.15f}, {"rdModDepth", 0.3f}, {"spectralTilt", 2.0f},
            {"consonantLevel", 0.5f}, {"consonantTone", 0.5f}, {"sibilance", 0.8f}, {"autoConsonant", 0.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 4.0f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.7f}, {"sourceFilterCoupling", 0.2f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },

        // ── Speech ──
        { "Speech", "Natural Tenor", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 1.0f}, {"breathiness", 0.1f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 15.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.15f}, {"shimmer", 0.1f}, {"rdModDepth", 0.6f}, {"spectralTilt", 0.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.33f}, {"sibilance", 0.5f}, {"autoConsonant", 1.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 0.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.4f}, {"sourceFilterCoupling", 0.4f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Speech", "Breathy Soprano", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 2.0f}, {"breathiness", 0.4f},
            {"vibratoRate", 6.0f}, {"vibratoDepth", 12.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.2f}, {"shimmer", 0.12f}, {"rdModDepth", 0.5f}, {"spectralTilt", 3.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.5f}, {"sibilance", 0.7f}, {"autoConsonant", 1.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 8.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.5f}, {"sourceFilterCoupling", 0.3f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Speech", "Pressed Baritone", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 3.0f},
            {"glottalRd", 0.4f}, {"breathiness", 0.0f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 10.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.1f}, {"shimmer", 0.08f}, {"rdModDepth", 0.7f}, {"spectralTilt", -3.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.9f}, {"sibilance", 0.1f}, {"autoConsonant", 1.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", -8.0f}, {"formantSpread", 1.0f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.3f}, {"sourceFilterCoupling", 0.5f},
            {"outputGain", 0.0f}, {"stereoWidth", 0.5f}
        }, juce::var() },
        { "Speech", "Child Voice", {
            {"vowelX", 0.5f}, {"vowelY", 0.5f}, {"vowelFocus", 2.5f},
            {"glottalRd", 1.5f}, {"breathiness", 0.2f},
            {"vibratoRate", 5.5f}, {"vibratoDepth", 5.0f}, {"vibratoDelay", 300.0f},
            {"jitter", 0.18f}, {"shimmer", 0.12f}, {"rdModDepth", 0.5f}, {"spectralTilt", 1.0f},
            {"consonantLevel", 0.3f}, {"consonantTone", 0.4f}, {"sibilance", 0.6f}, {"autoConsonant", 1.0f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.8f}, {"release", 0.5f},
            {"formantTopology", 0.0f}, {"formantShift", 18.0f}, {"formantSpread", 1.3f}, {"pitchGlide", 0.0f},
            {"transitionTime", 0.4f}, {"sourceFilterCoupling", 0.3f},
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
        synthesiser.addVoice (voice);
    }

    // Enable legacy mode for standard MIDI (channel range 1-16, pitchbend +/-2 semitones)
    synthesiser.enableLegacyMode (2, juce::Range<int> (1, 17));
}

OFormantAudioProcessor::~OFormantAudioProcessor()
{
}

//==============================================================================
void OFormantAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    synthesiser.setCurrentPlaybackSampleRate (sampleRate);
    setLatencySamples (0);
    outputGainSmoothed.reset (sampleRate, 0.050);

    // Prepare all voices with current sample rate
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<FormantVoice*> (synthesiser.getVoice (i)))
            voice->prepare (sampleRate);
    }

    lastSampleRate = sampleRate;
}

void OFormantAudioProcessor::releaseResources()
{
}

void OFormantAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synthesiser.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    // Post-synth output gain (dB -> linear, smoothed 50ms)
    float targetGain = juce::Decibels::decibelsToGain (
        parameters.getRawParameterValue ("outputGain")->load());
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
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("currentPreset", presetManager.getCurrentPresetName());
    copyXmlToBinary (*xml, destData);
}

void OFormantAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
    {
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
        presetManager.setCurrentPresetName (
            xmlState->getStringAttribute ("currentPreset", "Default"));
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OFormantAudioProcessor();
}
