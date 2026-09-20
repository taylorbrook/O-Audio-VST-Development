/*
   This file is part of O-Bells, an Ouaricon Audio plugin.
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

    O-Bells - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
// PluginEditor.h is deliberately NOT included at the top of this TU — the
// include lives inside the #if JUCE_WEB_BROWSER guard directly above
// createEditor(), so a console target that compiles this TU with
// JUCE_WEB_BROWSER=0 and no editor sources (scripts/param-dump) links.

//==============================================================================
// Parameter Layout (MUST be defined before constructor)
juce::AudioProcessorValueTreeState::ParameterLayout OBellsAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ========== Main Panel Parameters (7) ==========

    // STRIKE_POSITION - Center to edge strike
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "strikePosition", 1 },
        "Strike",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // MALLET_HARDNESS - Soft to hard striker
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "malletHardness", 1 },
        "Mallet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // DAMPING - Hand-damped to free-ring
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "damping", 1 },
        "Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f,
        "%"
    ));

    // OVERTONE BRIGHTNESS - Dark to brilliant (initial partial amplitudes)
    // v2.0.0: Renamed from "brightness" - BREAKING CHANGE
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "overtoneBrightness", 1 },
        "Overtone",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // ACOUSTIC BRIGHTNESS - Controls high-frequency decay rate (v2.0.0)
    // 0% = dark (higher partials decay 4x faster), 100% = bright (normal decay)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "acousticBrightness", 1 },
        "Acoustic",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f,  // Default: slightly natural/warm
        "%"
    ));

    // AIR ABSORPTION - Time-varying lowpass filter simulating air absorption (v2.1.0)
    // 0% = no filtering (transparent), 100% = progressive HF rolloff over decay
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "airAbsorption", 1 },
        "Air",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,  // Default: off (preserves existing sound)
        "%"
    ));

    // AIR ABSORPTION TIME - Independent time control for filter sweep (v2.2.0)
    // 0.1s to 10s - decoupled from note decay
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "airAbsorptionTime", 1 },
        "Air Time",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f, 0.5f),  // Skewed toward shorter times
        2.0f,  // Default: 2 seconds
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                if (value < 1.0f)
                    return juce::String(juce::roundToInt(value * 1000)) + " ms";
                return juce::String(value, 1) + " s";
            })
    ));

    // MATERIAL - Discrete choice parameter (v1.3.0)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "material", 1 },
        "Material",
        juce::StringArray { "Bronze", "Brass", "Steel", "Aluminum", "Cast Iron" },
        0  // Default: Bronze
    ));

    // INHARMONICITY - Pure harmonic to gamelan
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "inharmonicity", 1 },
        "Inharm",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // BLOOM SPEED - How fast partials swell (v1.4.0: split from bloom)
    // v1.5.1: Display as milliseconds (uses mid-partial range 25-400ms as representative)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomSpeed", 1 },
        "Bloom Speed",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,  // Default: medium speed
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                float ms = juce::jmap(value, 25.0f, 400.0f);
                return juce::String(juce::roundToInt(ms)) + " ms";
            })
    ));

    // BLOOM AMOUNT - Intensity of spectral swelling (v1.4.0: split from bloom)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomAmount", 1 },
        "Bloom Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,  // Default: off
        "%"
    ));

    // ========== Bloom Fine Controls (v1.5.0) ==========
    // Per-band bloom control when fine controls are enabled

    // BLOOM_FINE_ENABLED - Toggle for fine controls (0=off, 1=on)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "bloomFineEnabled", 1 },
        "Bloom Fine Controls",
        false  // Default: off (use main sliders)
    ));

    // Per-band Speed controls (v1.5.1: Display as milliseconds)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomSpeedLow", 1 },
        "Bloom Speed Low",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                float ms = juce::jmap(value, 15.0f, 250.0f);
                return juce::String(juce::roundToInt(ms)) + " ms";
            })
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomSpeedMid", 1 },
        "Bloom Speed Mid",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                float ms = juce::jmap(value, 25.0f, 400.0f);
                return juce::String(juce::roundToInt(ms)) + " ms";
            })
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomSpeedHigh", 1 },
        "Bloom Speed High",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                float ms = juce::jmap(value, 50.0f, 800.0f);
                return juce::String(juce::roundToInt(ms)) + " ms";
            })
    ));

    // Per-band Amount controls
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomAmountLow", 1 },
        "Bloom Amount Low",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomAmountMid", 1 },
        "Bloom Amount Mid",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomAmountHigh", 1 },
        "Bloom Amount High",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // SHIMMER - Frequency modulation that increases during decay (v1.2.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "shimmer", 1 },
        "Shimmer",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.2f,
        "%"
    ));

    // ========== Ensemble Section Parameters (5) ==========

    // UNISON_COUNT - Number of detuned bell copies (1-4)
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "unisonCount", 1 },
        "Unison",
        1,
        4,
        1
    ));

    // UNISON_DETUNE - Detune spread (0-50 cents)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "unisonDetune", 1 },
        "Detune",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f),
        10.0f,
        "cents"
    ));

    // OCTAVE_BLEND_SUB - Sub-octave layer mix
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "octaveBlendSub", 1 },
        "Sub",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // OCTAVE_BLEND_OCT - Upper-octave layer mix
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "octaveBlendOct", 1 },
        "Oct",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // STEREO_SPREAD - Ensemble panning width
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stereoSpread", 1 },
        "Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // ========== Advanced Panel Parameters (10) ==========

    // PARTIAL_TUNING - Fine-tune minor-third partial
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "partialTuning", 1 },
        "Partial Tune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f,
        "cents"
    ));

    // NONLINEAR_EFFECTS - Bell warping/distortion
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "nonlinearEffects", 1 },
        "Nonlinear",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // STRIKE_NOISE_CHARACTER - Transient filter type
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "strikeNoiseChar", 1 },
        "Noise",
        juce::StringArray { "Click", "Thud", "Ping" },
        0
    ));

    // ATTACK_LEVEL - Transient volume control (v1.3.0, renamed v1.5.2)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attackLevel", 1 },
        "Attack Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,  // Default 50% (natural level)
        "%"
    ));

    // VELOCITY_CURVE - Velocity response shaping
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "velocityCurve", 1 },
        "Velocity",
        juce::StringArray { "Linear", "Exponential", "Logarithmic" },
        0
    ));

    // PITCH_ENVELOPE - Initial pitch drop amount
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchEnvelope", 1 },
        "Pitch Env",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // PITCH_ENV_TIME - Pitch envelope return time
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchEnvTime", 1 },
        "P.Env Time",
        juce::NormalisableRange<float>(5.0f, 200.0f, 1.0f, 0.5f),
        50.0f,
        "ms"
    ));

    // ========== Multi-Stage Envelope Parameters (4) ==========
    // Only active when decayShape == 2 (Multi-stage)

    // STRIKE_TIME - Duration of bright metallic transient
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "strikeTime", 1 },
        "Strike Time",
        juce::NormalisableRange<float>(5.0f, 100.0f, 0.1f),
        30.0f,
        "ms"
    ));

    // BRILLIANCE - High-frequency sustain (0=warm/woody, 100=bright/glassy)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "brilliance", 1 },
        "Brilliance",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        "%"
    ));

    // BODY_TIME - Duration of main tonal decay phase
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodyTime", 1 },
        "Body Time",
        juce::NormalisableRange<float>(100.0f, 5000.0f, 1.0f),
        1500.0f,
        "ms"
    ));

    // HUM_SUSTAIN - Extension of low partial sustain
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "humSustain", 1 },
        "Hum Sustain",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        "%"
    ));

    // ========== Realism Parameters (v2.4.0) ==========

    // HUMANIZE - Per-note variation for organic realism
    // Applies subtle random variation to: strike position, mallet hardness, decay, attack, inharmonicity
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "humanize", 1 },
        "Humanize",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f,  // Default: moderate humanization
        "%"
    ));

    // ========== Lowpass Filter (v2.6.0) ==========

    // LP_FILTER_ENABLED - Toggle lowpass filter on/off
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lpFilterEnabled", 1 },
        "LP Filter",
        false  // Default: off
    ));

    // LP_FILTER_CUTOFF - One-pole lowpass cutoff frequency
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lpFilterCutoff", 1 },
        "LP Cutoff",
        juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f),  // Skewed toward low frequencies
        20000.0f,  // Default: wide open
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                if (value >= 1000.0f)
                    return juce::String(value / 1000.0f, 1) + " kHz";
                return juce::String(juce::roundToInt(value)) + " Hz";
            })
    ));

    // ========== Tuning Parameters (v3.0.0) ==========

    // MASTER TUNE - A4 reference frequency (400-480 Hz, default 440)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tuning_masterTune", 1 },
        "Master Tune",
        juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f),
        440.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));

    // OCTAVE STRETCH - Physical modeling octave stretch (0.95-1.25, default 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tuning_octaveStretch", 1 },
        "Octave Stretch",
        juce::NormalisableRange<float>(0.95f, 1.25f, 0.001f),
        1.0f
    ));

    // PITCH BEND RANGE - Pitch bend range in semitones (1-48, default 2)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tuning_pitchBendRange", 1 },
        "Pitch Bend Range",
        juce::NormalisableRange<float>(1.0f, 48.0f, 1.0f),
        2.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")
    ));

    // TEMPERAMENT PRESET - Built-in temperament selection
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "tuning_temperamentPreset", 1 },
        "Temperament",
        juce::StringArray {
            "Equal 12-TET",
            "Pythagorean",
            "Zarlino",
            "Meantone (1/4)",
            "Werckmeister III",
            "Kirnberger III",
            "Vallotti",
            "Well Tempered",
            "Just Intonation",
            "Bohlen-Pierce",
            "Custom"
        },
        0  // Default: Equal 12-TET
    ));

    // ========== Performance (v3.1.2) ==========

    // HIGH_FIDELITY - Disables voice culling for maximum sustain fidelity
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "highFidelity", 1 },
        "High Fidelity",
        false  // Default: off (voice culling active for CPU safety)
    ));

    // ========== Effects Chain Parameters (v4.0.0) ==========

    // --- Chorus ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "chorusBypass", 1 }, "Chorus Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chorusRate", 1 }, "Chorus Rate",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f), 1.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chorusDepth", 1 }, "Chorus Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chorusMix", 1 }, "Chorus Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f, "%"));

    // --- Delay ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "delayBypass", 1 }, "Delay Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayTime", 1 }, "Delay Time",
        juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f), 0.375f, "s"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayFeedback", 1 }, "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.3f, "%"));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "delayMode", 1 }, "Delay Mode",
        juce::StringArray { "Normal", "PingPong" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayMix", 1 }, "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f, "%"));

    // --- EQ ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "eqBypass", 1 }, "EQ Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eqLowGain", 1 }, "EQ Low",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eqMidGain", 1 }, "EQ Mid",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eqMidFreq", 1 }, "EQ Mid Freq",
        juce::NormalisableRange<float>(200.0f, 8000.0f, 1.0f, 0.5f), 1000.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eqHighGain", 1 }, "EQ High",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));

    // --- Reverb (FDN) ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "reverbBypass", 1 }, "Reverb Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbSize", 1 }, "Reverb Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbDamp", 1 }, "Reverb Damp",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbPredelay", 1 }, "Reverb Pre-delay",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 20.0f, "ms"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbMix", 1 }, "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbMod", 1 }, "Reverb Mod",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbShimmer", 1 }, "Reverb Shimmer",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f, "%"));

    // OUTPUT_GAIN - Master output level
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outputGain", 1 },
        "Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // ========== Timbral range (v4.7.0) ==========
    // APPEND-ONLY, version hint 2: AU orders parameters by hint, so these land
    // after every v1 parameter and no existing automation index moves. Defaults
    // (Classic, 0 dB, 0 dB, 0) take the v4.6.0 code paths — bit-identical render.
    // Tables: research/idiophone-partial-models.md.

    // PARTIAL_MODEL - which body the partial ratio + amplitude tables describe
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "partialModel", 2 },
        "Partial Model",
        juce::StringArray { "Classic", "Tubular", "Plate", "Bowl", "Glass" },
        0  // Classic: the harmonic -> bell -> gamelan interpolation
    ));

    // HUM_LEVEL / PRIME_LEVEL - level of partial 0 (hum) and partial 1 (prime)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "humLevel", 2 },
        "Hum Level",
        juce::NormalisableRange<float>(-24.0f, 6.0f, 0.1f),
        0.0f,
        "dB"
    ));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "primeLevel", 2 },
        "Prime Level",
        juce::NormalisableRange<float>(-24.0f, 6.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // HUM_FOLLOW - 0 = hum-stage time constant independent of Body Time (the
    // v4.6.0 law), 1 = it tracks Body Time, so a short body gets a short tail
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "humFollow", 2 },
        "Hum Follow",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    return layout;
}

//==============================================================================
OBellsAudioProcessor::OBellsAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-Bells")
{
    // Add 16 bell voices
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new BellVoice();
        voice->setTuningEngine(&tuningEngine);
        voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24: NE
        synthesiser.addVoice(voice);
    }

    // Add one sound (all notes trigger bell sounds)
    synthesiser.addSound(new BellSound());

    // Register tuning parameter listeners (v3.0.0)
    parameters.addParameterListener("tuning_masterTune", this);
    parameters.addParameterListener("tuning_octaveStretch", this);
    parameters.addParameterListener("tuning_pitchBendRange", this);
    parameters.addParameterListener("tuning_temperamentPreset", this);

    // Register custom state callbacks for tuning persistence
    presetManager.setCustomStateCallbacks(
        // Save callback
        [this]() -> juce::var
        {
            auto* obj = new juce::DynamicObject();
            auto intervals = tuningEngine.getIntervals();
            juce::Array<juce::var> intervalsArr;
            for (auto val : intervals)
                intervalsArr.add(val);
            obj->setProperty("intervals", juce::var(intervalsArr));
            obj->setProperty("scaleName", tuningEngine.getActiveTuningName());
            obj->setProperty("tonic", tuningEngine.getTonicNote());
            return juce::var(obj);
        },
        // Load callback
        [this](const juce::var& state)
        {
            if (auto* obj = state.getDynamicObject())
            {
                if (auto* intervalsArr = obj->getProperty("intervals").getArray())
                {
                    std::vector<double> intervals;
                    for (const auto& val : *intervalsArr)
                        intervals.push_back(static_cast<double>(val));
                    juce::String scaleName = obj->getProperty("scaleName").toString();
                    if (!intervals.empty())
                        tuningEngine.setCustomIntervals(intervals, scaleName.isEmpty() ? "Custom" : scaleName);
                }
                int tonic = obj->getProperty("tonic");
                tuningEngine.setTonicNote(tonic);
            }
        }
    );

    // Initialize factory presets (only on first run)
    initializeFactoryPresets();
}

OBellsAudioProcessor::~OBellsAudioProcessor()
{
    // Remove tuning parameter listeners (v3.0.0)
    parameters.removeParameterListener("tuning_masterTune", this);
    parameters.removeParameterListener("tuning_octaveStretch", this);
    parameters.removeParameterListener("tuning_pitchBendRange", this);
    parameters.removeParameterListener("tuning_temperamentPreset", this);
}

//==============================================================================
void OBellsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Reset lowpass filter state
    lpFilterStateL = 0.0f;
    lpFilterStateR = 0.0f;

    // Prepare synthesiser with sample rate
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);

    // Prepare all voices
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<BellVoice*>(synthesiser.getVoice(i)))
        {
            voice->prepare(sampleRate, samplesPerBlock);
        }
    }

    // Prepare effects chain DSP (v4.0.0)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    chorus.prepare(spec);
    chorus.reset();
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(0.0f);

    delayProcessor.prepare(spec);
    eqProcessor.prepare(spec);
    reverbFDN.prepare(spec);

    // Cache parameter pointers (atomic reads in processBlock)
    // Main Panel
    strikePositionParam = parameters.getRawParameterValue("strikePosition");
    malletHardnessParam = parameters.getRawParameterValue("malletHardness");
    dampingParam = parameters.getRawParameterValue("damping");
    overtoneBrightnessParam = parameters.getRawParameterValue("overtoneBrightness");
    acousticBrightnessParam = parameters.getRawParameterValue("acousticBrightness");
    airAbsorptionParam = parameters.getRawParameterValue("airAbsorption");
    airAbsorptionTimeParam = parameters.getRawParameterValue("airAbsorptionTime");
    materialParam = parameters.getRawParameterValue("material");
    inharmonicityParam = parameters.getRawParameterValue("inharmonicity");
    bloomSpeedParam = parameters.getRawParameterValue("bloomSpeed");
    bloomAmountParam = parameters.getRawParameterValue("bloomAmount");
    // v1.5.0: Bloom fine controls
    bloomFineEnabledParam = parameters.getRawParameterValue("bloomFineEnabled");
    bloomSpeedLowParam = parameters.getRawParameterValue("bloomSpeedLow");
    bloomSpeedMidParam = parameters.getRawParameterValue("bloomSpeedMid");
    bloomSpeedHighParam = parameters.getRawParameterValue("bloomSpeedHigh");
    bloomAmountLowParam = parameters.getRawParameterValue("bloomAmountLow");
    bloomAmountMidParam = parameters.getRawParameterValue("bloomAmountMid");
    bloomAmountHighParam = parameters.getRawParameterValue("bloomAmountHigh");
    shimmerParam = parameters.getRawParameterValue("shimmer");
    // Ensemble
    unisonCountParam = parameters.getRawParameterValue("unisonCount");
    unisonDetuneParam = parameters.getRawParameterValue("unisonDetune");
    octaveBlendSubParam = parameters.getRawParameterValue("octaveBlendSub");
    octaveBlendOctParam = parameters.getRawParameterValue("octaveBlendOct");
    stereoSpreadParam = parameters.getRawParameterValue("stereoSpread");
    // Advanced
    partialTuningParam = parameters.getRawParameterValue("partialTuning");
    nonlinearEffectsParam = parameters.getRawParameterValue("nonlinearEffects");
    strikeNoiseCharParam = parameters.getRawParameterValue("strikeNoiseChar");
    attackLevelParam = parameters.getRawParameterValue("attackLevel");
    velocityCurveParam = parameters.getRawParameterValue("velocityCurve");
    pitchEnvelopeParam = parameters.getRawParameterValue("pitchEnvelope");
    pitchEnvTimeParam = parameters.getRawParameterValue("pitchEnvTime");
    // Multi-stage envelope
    strikeTimeParam = parameters.getRawParameterValue("strikeTime");
    brillianceParam = parameters.getRawParameterValue("brilliance");
    bodyTimeParam = parameters.getRawParameterValue("bodyTime");
    humSustainParam = parameters.getRawParameterValue("humSustain");
    // Realism (v2.4.0)
    humanizeParam = parameters.getRawParameterValue("humanize");
    // Timbral range (v4.7.0)
    partialModelParam = parameters.getRawParameterValue("partialModel");
    humLevelParam = parameters.getRawParameterValue("humLevel");
    primeLevelParam = parameters.getRawParameterValue("primeLevel");
    humFollowParam = parameters.getRawParameterValue("humFollow");
    // Lowpass Filter (v2.6.0)
    lpFilterEnabledParam = parameters.getRawParameterValue("lpFilterEnabled");
    lpFilterCutoffParam = parameters.getRawParameterValue("lpFilterCutoff");
    // High Fidelity (v3.1.2)
    highFidelityParam = parameters.getRawParameterValue("highFidelity");
    // Tuning (v3.0.0)
    tuningMasterTuneParam = parameters.getRawParameterValue("tuning_masterTune");
    tuningOctaveStretchParam = parameters.getRawParameterValue("tuning_octaveStretch");
    tuningPitchBendRangeParam = parameters.getRawParameterValue("tuning_pitchBendRange");
    tuningTemperamentPresetParam = parameters.getRawParameterValue("tuning_temperamentPreset");
    // Output
    outputGainParam = parameters.getRawParameterValue("outputGain");

    // v4.0.0: Effects chain parameter cache
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
}

void OBellsAudioProcessor::releaseResources()
{
    // Release effects chain resources
    chorus.reset();
    delayProcessor.reset();
    eqProcessor.reset();
    reverbFDN.reset();
}

void OBellsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer
    buffer.clear();

    // VST3 Note Expression: drain raw event queue and correlate tuning deltas to NoteOn pitches.
    vst3Extensions.drainAndUpdate();

    // Read parameters (atomic, real-time safe)
    float inharmonicity = inharmonicityParam->load();
    float damping = dampingParam->load();
    float overtoneBrightness = overtoneBrightnessParam->load();
    float acousticBrightness = acousticBrightnessParam->load();
    float airAbsorption = airAbsorptionParam->load();
    float airAbsorptionTime = airAbsorptionTimeParam->load();
    float strikePosition = strikePositionParam->load();
    float malletHardness = malletHardnessParam->load();
    float material = materialParam->load();
    float bloomSpeed = bloomSpeedParam->load();
    float bloomAmount = bloomAmountParam->load();
    // v1.5.0: Bloom fine controls
    bool bloomFineEnabled = bloomFineEnabledParam->load() > 0.5f;
    float bloomSpeedLow = bloomSpeedLowParam->load();
    float bloomSpeedMid = bloomSpeedMidParam->load();
    float bloomSpeedHigh = bloomSpeedHighParam->load();
    float bloomAmountLow = bloomAmountLowParam->load();
    float bloomAmountMid = bloomAmountMidParam->load();
    float bloomAmountHigh = bloomAmountHighParam->load();
    float shimmer = shimmerParam->load();
    int unisonCount = static_cast<int>(unisonCountParam->load());
    float unisonDetune = unisonDetuneParam->load();
    float octaveBlendSub = octaveBlendSubParam->load();
    float octaveBlendOct = octaveBlendOctParam->load();
    float stereoSpread = stereoSpreadParam->load();
    float partialTuning = partialTuningParam->load();
    float pitchEnvelope = pitchEnvelopeParam->load();
    float pitchEnvTime = pitchEnvTimeParam->load();
    // decayShape removed - always use multi-stage in v1.2.0
    int velocityCurve = static_cast<int>(velocityCurveParam->load());
    float nonlinearEffects = nonlinearEffectsParam->load();
    int strikeNoiseChar = static_cast<int>(strikeNoiseCharParam->load());
    float attackLevel = attackLevelParam->load();
    // Multi-stage envelope params (always active in v1.2.0)
    float strikeTime = strikeTimeParam->load();
    float brilliance = brillianceParam->load();
    float bodyTime = bodyTimeParam->load();
    float humSustain = humSustainParam->load();
    // Realism (v2.4.0)
    float humanize = humanizeParam->load();
    // Timbral range (v4.7.0)
    int partialModel = static_cast<int>(partialModelParam->load());
    float humLevel = humLevelParam->load();
    float primeLevel = primeLevelParam->load();
    float humFollow = humFollowParam->load();
    float outputGain = outputGainParam->load();

    // v3.1.2: Read high fidelity toggle
    bool highFidelity = highFidelityParam->load() > 0.5f;

    // Update all voice parameters
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<BellVoice*>(synthesiser.getVoice(i)))
        {
            voice->setHighFidelity(highFidelity);
            voice->updateParameters(
                inharmonicity, damping, overtoneBrightness, acousticBrightness,
                airAbsorption, airAbsorptionTime,
                strikePosition, malletHardness, material, bloomSpeed, bloomAmount,
                bloomFineEnabled, bloomSpeedLow, bloomSpeedMid, bloomSpeedHigh,
                bloomAmountLow, bloomAmountMid, bloomAmountHigh,
                shimmer,
                unisonCount, unisonDetune,
                octaveBlendSub, octaveBlendOct, stereoSpread,
                partialTuning, pitchEnvelope, pitchEnvTime,
                velocityCurve, nonlinearEffects,
                strikeNoiseChar, attackLevel, outputGain,
                strikeTime, brilliance, bodyTime, humSustain,
                humanize
            );
            voice->updateTimbreParameters(partialModel, humLevel, primeLevel, humFollow);
        }
    }

    // v2.7.0: Track MIDI notes for UI spoke highlighting
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            int note = msg.getNoteNumber();
            if (note < 64)
                activeNotesLow.fetch_or(uint64_t(1) << note);
            else
                activeNotesHigh.fetch_or(uint64_t(1) << (note - 64));
        }
        else if (msg.isNoteOff())
        {
            int note = msg.getNoteNumber();
            if (note < 64)
                activeNotesLow.fetch_and(~(uint64_t(1) << note));
            else
                activeNotesHigh.fetch_and(~(uint64_t(1) << (note - 64)));
        }
    }

    // Process MIDI and render audio
    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // v3.1.2 / WR-09: Soft limiter. Transparent below threshold, gentle tanh
    // compression above. Applied BOTH pre-FX (tames overlapping voice tails, the
    // original v3.1.2 behaviour) AND again post-EQ below — the EQ shelves boost
    // up to +12 dB *after* this first stage, so a dense chord limited to ~0.9 and
    // then boosted could otherwise clip hard at the output.
    auto applySoftLimiter = [](juce::AudioBuffer<float>& buf)
    {
        constexpr float limiterThreshold = 0.9f;
        constexpr float limiterCeiling = 1.0f - limiterThreshold;

        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            float* channelData = buf.getWritePointer(ch);
            for (int s = 0; s < buf.getNumSamples(); ++s)
            {
                float sample = channelData[s];
                // Final NaN/Inf net: a pathological param combo must never emit a
                // non-finite sample to the host (pluginval strictness-10 gate). The
                // voice-level guards below are the real fixes; this backstops the
                // whole FX chain. (pattern_biquad_nan_guard_sticky_silence)
                if (! std::isfinite (sample))
                {
                    channelData[s] = 0.0f;
                    continue;
                }
                float absVal = std::abs(sample);
                if (absVal > limiterThreshold)
                {
                    float sign = sample > 0.0f ? 1.0f : -1.0f;
                    float over = absVal - limiterThreshold;
                    channelData[s] = sign * (limiterThreshold + limiterCeiling * std::tanh(over / limiterCeiling));
                }
            }
        }
    };

    applySoftLimiter(buffer);

    // Apply one-pole lowpass filter (v2.6.0) - post-synth, pre-reverb
    bool lpEnabled = lpFilterEnabledParam->load() > 0.5f;
    if (lpEnabled)
    {
        float cutoff = lpFilterCutoffParam->load();
        float coeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * cutoff / static_cast<float>(currentSampleRate));

        const int numSamplesLP = buffer.getNumSamples();
        float* leftChannel = buffer.getWritePointer(0);
        float* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        for (int i = 0; i < numSamplesLP; ++i)
        {
            lpFilterStateL += coeff * (leftChannel[i] - lpFilterStateL);
            leftChannel[i] = lpFilterStateL;

            if (rightChannel != nullptr)
            {
                lpFilterStateR += coeff * (rightChannel[i] - lpFilterStateR);
                rightChannel[i] = lpFilterStateR;
            }
        }
    }

    // v4.0.0: Effects chain (Chorus -> Delay -> Reverb -> EQ)
    juce::dsp::AudioBlock<float> block(buffer);

    // 1. Chorus
    bool chorusBypassed = fxCache.chorusBypass->load(std::memory_order_relaxed) >= 0.5f;
    if (!chorusBypassed)
    {
        float chorusRate = fxCache.chorusRate->load(std::memory_order_relaxed);
        float chorusDepth = fxCache.chorusDepth->load(std::memory_order_relaxed);
        float chorusMixVal = fxCache.chorusMix->load(std::memory_order_relaxed);

        chorus.setRate(chorusRate);
        chorus.setDepth(chorusDepth);
        chorus.setMix(chorusMixVal);

        if (chorusMixVal > 0.001f)
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

        delayProcessor.setTime(delayTimeSec);
        delayProcessor.setFeedback(delayFb);
        delayProcessor.setMode(delayModeVal);
        delayProcessor.setMix(delayMixVal);

        if (delayMixVal > 0.001f)
            delayProcessor.process(block);
    }

    // 3. Reverb (FDN)
    bool reverbBypassed = fxCache.reverbBypass->load(std::memory_order_relaxed) >= 0.5f;
    if (!reverbBypassed)
    {
        float reverbMixVal = fxCache.reverbMix->load(std::memory_order_relaxed);
        reverbFDN.setSize(fxCache.reverbSize->load(std::memory_order_relaxed));
        reverbFDN.setDamping(fxCache.reverbDamp->load(std::memory_order_relaxed));
        reverbFDN.setPredelay(fxCache.reverbPredelay->load(std::memory_order_relaxed));
        reverbFDN.setMix(reverbMixVal);
        reverbFDN.setMod(fxCache.reverbMod->load(std::memory_order_relaxed));
        reverbFDN.setShimmer(fxCache.reverbShimmer->load(std::memory_order_relaxed));

        if (reverbMixVal > 0.001f)
            reverbFDN.process(block);
    }

    // 4. EQ
    bool eqBypassed = fxCache.eqBypass->load(std::memory_order_relaxed) >= 0.5f;
    if (!eqBypassed)
    {
        eqProcessor.setLowGain(fxCache.eqLowGain->load(std::memory_order_relaxed));
        eqProcessor.setMidGain(fxCache.eqMidGain->load(std::memory_order_relaxed));
        eqProcessor.setMidFreq(fxCache.eqMidFreq->load(std::memory_order_relaxed));
        eqProcessor.setHighGain(fxCache.eqHighGain->load(std::memory_order_relaxed));
        eqProcessor.process(block);
    }

    // WR-09: final safety limiter after the FX chain (esp. the EQ's +12 dB
    // shelves) so post-EQ boost can't clip the output.
    applySoftLimiter(buffer);

    // Calculate output levels for metering (peak detection)
    const int numSamples = buffer.getNumSamples();
    float peakLeft = 0.0f;
    float peakRight = 0.0f;

    if (buffer.getNumChannels() >= 1)
        peakLeft = buffer.getMagnitude(0, 0, numSamples);
    if (buffer.getNumChannels() >= 2)
        peakRight = buffer.getMagnitude(1, 0, numSamples);

    // Store with ballistics (slight hold for visual smoothness)
    const float decay = 0.85f;  // Meter decay rate
    outputLevelLeft.store(std::max(peakLeft, outputLevelLeft.load() * decay));
    outputLevelRight.store(std::max(peakRight, outputLevelRight.load() * decay));
}

//==============================================================================
#if JUCE_WEB_BROWSER
#include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* OBellsAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OBellsAudioProcessorEditor(*this);
#else
    // The param-dump console target builds with JUCE_WEB_BROWSER=0 and no
    // editor sources. It never opens an editor; this keeps the TU linkable.
    return new juce::GenericAudioProcessorEditor(*this);
#endif
}

//==============================================================================
void OBellsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // v4.2.0: the UI language rides the same tree as one more plain property.
    // Not a parameter (see PluginProcessor.h), so it is written HERE, onto the
    // live state tree, immediately before the preset manager copies that tree —
    // getStateAsXml() starts from parameters.copyState(), so a property set on
    // parameters.state is carried into the XML with everything else.
    //
    // Written as a STRING ("en"/"fr") rather than the atomic's int index, so a
    // hand-inspected session file says what it means.
    parameters.state.setProperty("uiLanguage",
                                 languageCode(uiLanguage.load(std::memory_order_acquire)),
                                 nullptr);

    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void OBellsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());

    // v4.2.0: the UI language, read back AFTER the preset manager has restored
    // the tree — setStateFromXml() calls parameters.replaceState(), so reading
    // before it would read the property off the tree that was just discarded.
    //
    // isVoid() is the ONLY correct guard and toString() the only correct read.
    // getStateInformation writes a STRING var, but even a bool or an int written
    // there would not survive: the XML round-trip does not preserve the type,
    // because NamedValueSet::setFromXmlAttributes rebuilds every property as
    // `var (value)` over the attribute STRING
    // (critical_valuetree_xml_roundtrip_loses_type). A pre-4.2.0 session has no
    // such property at all and the default (English) stands. languageIndex()
    // clamps anything that is not "fr" to 0, so a hand-edited value degrades to
    // English rather than to a bad index.
    //
    // The editor PULLS this through the getUiLanguage native fn at page init
    // rather than being pushed from here — a push would race the WebView's load.
    const juce::var lang = parameters.state.getProperty("uiLanguage");

    if (! lang.isVoid())
        uiLanguage.store(languageIndex(lang.toString()), std::memory_order_release);
}

//==============================================================================
// v2.2.0: GUI keyboard note triggering
void OBellsAudioProcessor::triggerNoteOn(int midiNote, float velocity)
{
    // Clamp values to valid MIDI ranges
    midiNote = juce::jlimit(0, 127, midiNote);
    velocity = juce::jlimit(0.0f, 1.0f, velocity);

    // Track active note for UI spoke highlighting
    if (midiNote < 64)
        activeNotesLow.fetch_or(uint64_t(1) << midiNote);
    else
        activeNotesHigh.fetch_or(uint64_t(1) << (midiNote - 64));

    // Use channel 1 for UI-triggered notes
    synthesiser.noteOn(1, midiNote, velocity);
}

void OBellsAudioProcessor::triggerNoteOff(int midiNote)
{
    midiNote = juce::jlimit(0, 127, midiNote);

    // Clear active note for UI spoke highlighting
    if (midiNote < 64)
        activeNotesLow.fetch_and(~(uint64_t(1) << midiNote));
    else
        activeNotesHigh.fetch_and(~(uint64_t(1) << (midiNote - 64)));

    // allowTailOff = true for natural release
    synthesiser.noteOff(1, midiNote, 0.0f, true);
}

// v3.1.0: Get held notes and their actual frequencies for TrueKeys visualization
void OBellsAudioProcessor::getHeldNotesData(std::vector<int>& notes, std::vector<double>& frequencies)
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

//==============================================================================
void OBellsAudioProcessor::initializeFactoryPresets()
{
    // CR-01: force regeneration of on-disk factory JSON when the factory schema
    // version changes. Pre-v4.1.1 installs cached presets that stored raw
    // engineering units and recalled slammed to a rail; the sentinel guarantees
    // the corrected, normalized presets are (re)written on upgrade rather than
    // being skipped by the plain "already exists" guard.
    // v4.8.0: the whole bank was re-voiced (same 25 names, so every file is
    // overwritten in place and nothing is orphaned).
    const juce::String factoryVersion = "4.8.0";
    auto versionFile = presetManager.getFactoryPresetsDirectory().getChildFile(".factory_version");

    if (presetManager.factoryPresetsExist()
        && versionFile.existsAsFile()
        && versionFile.loadFileAsString().trim() == factoryVersion)
        return;

    std::vector<OuariconPresetManager::FactoryPresetDef> presets;

    // ==========================================================================
    // v4.8.0 FACTORY PRESETS - re-voiced against the v4.7 engine
    // (improvements/preset-differentiation-v4.6-v4.8.md, Step 5)
    // ==========================================================================
    // RC-3: Damping is the LIVE law - higher = SHORTER (note-off release 3 s -> 0.5 s,
    //   and the hum stage of partials 0-1). Big bells sit at 0-0.15, bars and plates
    //   at 0.8-1.0. The v2.2.1 bank had this inverted (Large Bells at 0.88-1.0).
    // RC-4: every preset picks a partialModel and sets humLevel / primeLevel; short
    //   presets pair Hum Follow with HIGH damping (Hum Follow alone is inaudible on a tap).
    // RC-5: every preset carries its own FX signature. Preset apply resets unnamed
    //   parameters to default first, so a block that is absent here is at default.
    // outputGain balances the bank to about -24 dBFS RMS (C3 / C4 / C5, vel 1, FX on).
    //
    // Gate: tests/render-harness/report.py - every preset's nearest neighbour >= 8 dB,
    //   tap and held, and the tap-T40 category ordering.
    //
    // Choice indices (tables are in ENGINEERING units; the manager normalises):
    //   material:        0 Bronze, 1 Brass, 2 Steel, 3 Aluminum, 4 Cast Iron
    //   partialModel:    0 Classic, 1 Tubular, 2 Plate, 3 Bowl, 4 Glass
    //   strikeNoiseChar: 0 Click, 1 Thud, 2 Ping
    //   velocityCurve:   0 Linear, 1 Exponential, 2 Logarithmic
    //   delayMode:       0 Normal, 1 PingPong
    //   bloomSpeed*:     0-1 (not ms)
    // ==========================================================================

    // ========== AMBIENT (5 presets) ==========
    // Heavily processed, bloom-led textures

    // A tower bell heard across a valley: no sub, no strike, air absorption at full, everything above 1.8 kHz gone.
    // FX: the room IS the sound - 85 % wet, 150 ms pre-delay, largest size.
    presets.push_back({ "Ambient", "Distant Cathedral", {
        {"partialModel", 0.0f}, {"material", 0.0f}, {"inharmonicity", 0.5f},
        {"strikePosition", 0.5f}, {"malletHardness", 0.3f}, {"damping", 0.1f},
        {"overtoneBrightness", 0.5f}, {"acousticBrightness", 0.35f}, {"airAbsorption", 1.0f}, {"airAbsorptionTime", 1.5f},
        {"humLevel", -12.0f}, {"primeLevel", 0.0f}, {"humFollow", 0.0f},
        {"bloomSpeed", 1.0f}, {"bloomAmount", 0.9f}, {"shimmer", 0.15f},
        {"unisonCount", 3.0f}, {"unisonDetune", 15.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 1.0f},
        {"strikeTime", 60.0f}, {"brilliance", 20.0f}, {"bodyTime", 4000.0f}, {"humSustain", 90.0f},
        {"attackLevel", 0.1f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 1800.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 2.0f}, {"humanize", 0.35f},
        {"eqLowGain", -3.0f}, {"eqHighGain", -6.0f},
        {"reverbSize", 1.0f}, {"reverbDamp", 0.7f}, {"reverbPredelay", 150.0f}, {"reverbMix", 0.85f}, {"reverbMod", 0.25f},
        {"outputGain", -5.5f}
    }, {} });

    // Glass model, full bloom on all three bands and no strike - a swell, not a hit.
    // FX: slow deep chorus into a shimmer reverb.
    presets.push_back({ "Ambient", "Ethereal Chime Pad", {
        {"partialModel", 4.0f}, {"material", 3.0f}, {"inharmonicity", 0.4f},
        {"strikePosition", 0.5f}, {"malletHardness", 0.0f}, {"damping", 0.15f},
        {"overtoneBrightness", 0.6f}, {"acousticBrightness", 0.8f}, {"airAbsorption", 0.2f}, {"airAbsorptionTime", 6.0f},
        {"humLevel", -12.0f}, {"primeLevel", 0.0f}, {"humFollow", 0.0f},
        {"bloomSpeed", 1.0f}, {"bloomAmount", 1.0f}, {"shimmer", 0.6f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 1.0f}, {"bloomSpeedMid", 1.0f}, {"bloomSpeedHigh", 1.0f},
        {"bloomAmountLow", 1.0f}, {"bloomAmountMid", 1.0f}, {"bloomAmountHigh", 0.9f},
        {"unisonCount", 4.0f}, {"unisonDetune", 20.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.5f}, {"stereoSpread", 1.0f},
        {"strikeTime", 100.0f}, {"brilliance", 70.0f}, {"bodyTime", 5000.0f}, {"humSustain", 100.0f},
        {"attackLevel", 0.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 2.0f}, {"humanize", 0.5f},
        {"chorusRate", 0.4f}, {"chorusDepth", 0.7f}, {"chorusMix", 0.5f},
        {"reverbSize", 0.9f}, {"reverbDamp", 0.3f}, {"reverbPredelay", 40.0f}, {"reverbMix", 0.65f}, {"reverbMod", 0.6f}, {"reverbShimmer", 0.6f},
        {"outputGain", -3.0f}
    }, {} });

    // Bowl model. Fine bloom staggers the bands (low at once, high after 800 ms) so the spectrum opens over time.
    // FX: long ping-pong delay at 70 % feedback, heavily modulated reverb.
    presets.push_back({ "Ambient", "Evolving Bronze Wash", {
        {"partialModel", 3.0f}, {"material", 0.0f}, {"inharmonicity", 0.7f},
        {"strikePosition", 0.65f}, {"malletHardness", 0.2f}, {"damping", 0.05f},
        {"overtoneBrightness", 0.8f}, {"acousticBrightness", 0.9f}, {"airAbsorption", 0.1f}, {"airAbsorptionTime", 8.0f},
        {"humLevel", 0.0f}, {"primeLevel", -8.0f}, {"humFollow", 0.0f},
        {"bloomSpeed", 1.0f}, {"bloomAmount", 0.9f}, {"shimmer", 0.8f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 0.0f}, {"bloomSpeedMid", 0.47f}, {"bloomSpeedHigh", 1.0f},
        {"bloomAmountLow", 0.2f}, {"bloomAmountMid", 0.8f}, {"bloomAmountHigh", 1.0f},
        {"unisonCount", 3.0f}, {"unisonDetune", 28.0f},
        {"octaveBlendSub", 0.4f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 1.0f},
        {"strikeTime", 80.0f}, {"brilliance", 85.0f}, {"bodyTime", 5000.0f}, {"humSustain", 100.0f},
        {"attackLevel", 0.05f}, {"nonlinearEffects", 0.4f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.45f},
        {"delayTime", 0.75f}, {"delayFeedback", 0.7f}, {"delayMode", 1.0f}, {"delayMix", 0.35f},
        {"reverbSize", 0.85f}, {"reverbDamp", 0.4f}, {"reverbPredelay", 30.0f}, {"reverbMix", 0.55f}, {"reverbMod", 0.8f},
        {"outputGain", 5.0f}
    }, {} });

    // Plate model in steel, damping 0, hum removed, octave layer high - a bright sheet that never settles.
    // FX: undamped shimmer reverb, EQ tilted up. Output trimmed: the shimmer builds on held notes.
    presets.push_back({ "Ambient", "Frozen Steel Shimmer", {
        {"partialModel", 2.0f}, {"material", 2.0f}, {"inharmonicity", 0.35f},
        {"strikePosition", 0.85f}, {"malletHardness", 0.6f}, {"damping", 0.0f},
        {"overtoneBrightness", 0.95f}, {"acousticBrightness", 1.0f}, {"airAbsorption", 0.0f},
        {"humLevel", -24.0f}, {"primeLevel", -6.0f}, {"humFollow", 0.0f},
        {"bloomSpeed", 0.33f}, {"bloomAmount", 0.4f}, {"shimmer", 0.9f},
        {"unisonCount", 2.0f}, {"unisonDetune", 9.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.8f}, {"stereoSpread", 0.9f},
        {"strikeTime", 20.0f}, {"brilliance", 100.0f}, {"bodyTime", 5000.0f}, {"humSustain", 100.0f},
        {"attackLevel", 0.3f},
        {"strikeNoiseChar", 2.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.2f},
        {"eqLowGain", -6.0f}, {"eqHighGain", 5.0f},
        {"reverbSize", 1.0f}, {"reverbDamp", 0.0f}, {"reverbPredelay", 20.0f}, {"reverbMix", 0.5f}, {"reverbMod", 0.35f}, {"reverbShimmer", 0.6f},
        {"outputGain", -7.5f}
    }, {} });

    // Cast iron under a 900 Hz low-pass with a deep 200 ms pitch dip.
    // FX: full-depth chorus wobble, fully damped reverb, high shelf at -12 dB.
    presets.push_back({ "Ambient", "Underwater Bell", {
        {"partialModel", 0.0f}, {"material", 4.0f}, {"inharmonicity", 0.4f},
        {"strikePosition", 0.05f}, {"malletHardness", 0.1f}, {"damping", 0.3f},
        {"overtoneBrightness", 0.05f}, {"acousticBrightness", 0.1f}, {"airAbsorption", 0.8f}, {"airAbsorptionTime", 0.6f},
        {"humLevel", 6.0f}, {"primeLevel", 0.0f}, {"humFollow", 0.0f},
        {"bloomSpeed", 0.6f}, {"bloomAmount", 0.5f}, {"shimmer", 0.4f},
        {"unisonCount", 2.0f}, {"unisonDetune", 16.0f},
        {"octaveBlendSub", 0.6f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.7f},
        {"strikeTime", 70.0f}, {"brilliance", 0.0f}, {"bodyTime", 3000.0f}, {"humSustain", 70.0f},
        {"attackLevel", 0.05f}, {"pitchEnvelope", 0.6f}, {"pitchEnvTime", 200.0f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 900.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 2.0f}, {"humanize", 0.4f},
        {"eqMidGain", 3.0f}, {"eqMidFreq", 350.0f}, {"eqHighGain", -12.0f},
        {"chorusRate", 0.8f}, {"chorusDepth", 1.0f}, {"chorusMix", 0.6f},
        {"reverbSize", 0.7f}, {"reverbDamp", 1.0f}, {"reverbPredelay", 0.0f}, {"reverbMix", 0.5f}, {"reverbMod", 0.7f},
        {"outputGain", -10.0f}
    }, {} });

    // ========== BRIGHT BELLS (5 presets) ==========
    // Hard mallets, high partials, moderate damping

    // Near-harmonic Classic (inharmonicity 0.08), hardest mallet, ping, hum removed.
    // Oct stays near its v2.2.1 value: Classic has no Nyquist guard (MODEL_NYQUIST_GUARD) and crotales are played high.
    // FX: small bright room, a touch of shimmer, high shelf up.
    presets.push_back({ "Bright Bells", "Bright Clear Crotale", {
        {"partialModel", 0.0f}, {"material", 0.0f}, {"inharmonicity", 0.08f},
        {"strikePosition", 0.8f}, {"malletHardness", 0.95f}, {"damping", 0.55f},
        {"overtoneBrightness", 0.85f}, {"acousticBrightness", 0.95f},
        {"humLevel", -24.0f}, {"primeLevel", 3.0f}, {"humFollow", 0.4f},
        {"bloomSpeed", 0.0f}, {"bloomAmount", 0.05f}, {"shimmer", 0.05f},
        {"unisonCount", 1.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.25f}, {"stereoSpread", 0.4f},
        {"strikeTime", 8.0f}, {"brilliance", 95.0f}, {"bodyTime", 2500.0f}, {"humSustain", 60.0f},
        {"attackLevel", 0.8f},
        {"strikeNoiseChar", 2.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.15f},
        {"eqHighGain", 4.0f},
        {"reverbSize", 0.5f}, {"reverbDamp", 0.1f}, {"reverbPredelay", 15.0f}, {"reverbMix", 0.25f}, {"reverbMod", 0.1f}, {"reverbShimmer", 0.15f},
        {"outputGain", 3.5f}
    }, {} });

    // Plate model stretched wide (inharmonicity 0.9), sub layer, some nonlinear clang. Hum Follow keeps the ring short.
    // FX: mid push at 3 kHz, medium bright room.
    presets.push_back({ "Bright Bells", "Brilliant Bronze Plate", {
        {"partialModel", 2.0f}, {"material", 0.0f}, {"inharmonicity", 0.9f},
        {"strikePosition", 0.45f}, {"malletHardness", 0.7f}, {"damping", 0.65f},
        {"overtoneBrightness", 0.7f}, {"acousticBrightness", 0.8f},
        {"humLevel", -12.0f}, {"primeLevel", 0.0f}, {"humFollow", 0.5f},
        {"bloomSpeed", 0.04f}, {"bloomAmount", 0.1f}, {"shimmer", 0.3f},
        {"unisonCount", 1.0f},
        {"octaveBlendSub", 0.3f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.75f},
        {"strikeTime", 15.0f}, {"brilliance", 80.0f}, {"bodyTime", 1200.0f}, {"humSustain", 30.0f},
        {"attackLevel", 0.7f}, {"nonlinearEffects", 0.3f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.25f},
        {"eqMidGain", 3.0f}, {"eqMidFreq", 3000.0f},
        {"reverbSize", 0.65f}, {"reverbDamp", 0.25f}, {"reverbPredelay", 35.0f}, {"reverbMix", 0.35f}, {"reverbMod", 0.2f},
        {"outputGain", 4.5f}
    }, {} });

    // Tubular model, hardest mallet, click. RC-3: a bar is SHORT - damping 0.8, body 600 ms, Hum Follow 1.
    // FX: tight ping-pong slap, nearly dry.
    presets.push_back({ "Bright Bells", "Crisp Steel Bar", {
        {"partialModel", 1.0f}, {"material", 2.0f}, {"inharmonicity", 0.75f},
        {"strikePosition", 0.7f}, {"malletHardness", 1.0f}, {"damping", 0.8f},
        {"overtoneBrightness", 0.8f}, {"acousticBrightness", 0.6f},
        {"humLevel", -24.0f}, {"primeLevel", -3.0f}, {"humFollow", 1.0f},
        {"bloomSpeed", 0.0f}, {"bloomAmount", 0.02f}, {"shimmer", 0.0f},
        {"unisonCount", 1.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.3f},
        {"strikeTime", 5.0f}, {"brilliance", 60.0f}, {"bodyTime", 600.0f}, {"humSustain", 10.0f},
        {"attackLevel", 0.9f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 1.0f}, {"humanize", 0.2f},
        {"delayTime", 0.18f}, {"delayFeedback", 0.25f}, {"delayMode", 1.0f}, {"delayMix", 0.2f},
        {"reverbSize", 0.25f}, {"reverbDamp", 0.4f}, {"reverbPredelay", 5.0f}, {"reverbMix", 0.15f}, {"reverbMod", 0.0f},
        {"outputGain", 1.5f}
    }, {} });

    // Glass model in steel with the octave layer and full brilliance.
    // FX: dotted ping-pong delay, shimmer reverb.
    presets.push_back({ "Bright Bells", "Crystalline Steel Chime", {
        {"partialModel", 4.0f}, {"material", 2.0f}, {"inharmonicity", 0.6f},
        {"strikePosition", 0.75f}, {"malletHardness", 0.85f}, {"damping", 0.6f},
        {"overtoneBrightness", 0.9f}, {"acousticBrightness", 1.0f},
        {"humLevel", -18.0f}, {"primeLevel", 0.0f}, {"humFollow", 0.3f},
        {"bloomSpeed", 0.01f}, {"bloomAmount", 0.08f}, {"shimmer", 0.4f},
        {"unisonCount", 2.0f}, {"unisonDetune", 4.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.5f}, {"stereoSpread", 0.85f},
        {"strikeTime", 10.0f}, {"brilliance", 100.0f}, {"bodyTime", 2000.0f}, {"humSustain", 45.0f},
        {"attackLevel", 0.6f},
        {"strikeNoiseChar", 2.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.2f},
        {"delayTime", 0.33f}, {"delayFeedback", 0.4f}, {"delayMode", 1.0f}, {"delayMix", 0.18f},
        {"reverbSize", 0.7f}, {"reverbDamp", 0.15f}, {"reverbPredelay", 25.0f}, {"reverbMix", 0.4f}, {"reverbMod", 0.3f}, {"reverbShimmer", 0.35f},
        {"outputGain", 0.5f}
    }, {} });

    // Tubular model stretched to the limit, struck at the rim, wide unison. A model (not Classic) so the octave layer is Nyquist-guarded.
    // FX: fast chorus, EQ low cut / high lift.
    presets.push_back({ "Bright Bells", "Sparkling Aluminum", {
        {"partialModel", 1.0f}, {"material", 3.0f}, {"inharmonicity", 1.0f},
        {"strikePosition", 1.0f}, {"malletHardness", 0.9f}, {"damping", 0.7f},
        {"overtoneBrightness", 1.0f}, {"acousticBrightness", 0.9f},
        {"humLevel", -18.0f}, {"primeLevel", -3.0f}, {"humFollow", 0.6f},
        {"bloomSpeed", 0.0f}, {"bloomAmount", 0.05f}, {"shimmer", 0.5f},
        {"unisonCount", 2.0f}, {"unisonDetune", 25.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.7f}, {"stereoSpread", 1.0f},
        {"strikeTime", 6.0f}, {"brilliance", 90.0f}, {"bodyTime", 900.0f}, {"humSustain", 20.0f},
        {"attackLevel", 0.75f},
        {"strikeNoiseChar", 2.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.3f},
        {"eqLowGain", -4.0f}, {"eqHighGain", 6.0f},
        {"chorusRate", 2.5f}, {"chorusDepth", 0.4f}, {"chorusMix", 0.3f},
        {"reverbSize", 0.35f}, {"reverbDamp", 0.2f}, {"reverbPredelay", 10.0f}, {"reverbMix", 0.3f}, {"reverbMod", 0.15f},
        {"outputGain", 5.5f}
    }, {} });

    // ========== LARGE BELLS (5 presets) ==========
    // RC-3: big bells ring - damping 0-0.15, longest body

    // Tubular model in brass - the orchestral tubular bell. RC-3: damping 0.05.
    // FX: 120 ms pre-delay and a half-second echo: the cavern.
    presets.push_back({ "Large Bells", "Cavernous Brass", {
        {"partialModel", 1.0f}, {"material", 1.0f}, {"inharmonicity", 0.5f},
        {"strikePosition", 0.6f}, {"malletHardness", 0.55f}, {"damping", 0.05f},
        {"overtoneBrightness", 0.7f}, {"acousticBrightness", 0.7f}, {"airAbsorption", 0.15f}, {"airAbsorptionTime", 5.0f},
        {"humLevel", -6.0f}, {"primeLevel", 0.0f}, {"humFollow", 0.0f},
        {"bloomSpeed", 0.25f}, {"bloomAmount", 0.2f}, {"shimmer", 0.25f},
        {"unisonCount", 2.0f}, {"unisonDetune", 10.0f},
        {"octaveBlendSub", 0.35f}, {"octaveBlendOct", 0.2f}, {"stereoSpread", 0.8f},
        {"strikeTime", 35.0f}, {"brilliance", 55.0f}, {"bodyTime", 5000.0f}, {"humSustain", 100.0f},
        {"attackLevel", 0.55f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.3f},
        {"delayTime", 0.5f}, {"delayFeedback", 0.35f}, {"delayMode", 0.0f}, {"delayMix", 0.15f},
        {"reverbSize", 0.95f}, {"reverbDamp", 0.45f}, {"reverbPredelay", 120.0f}, {"reverbMix", 0.5f}, {"reverbMod", 0.2f},
        {"outputGain", -4.0f}
    }, {} });

    // The reference church bell: Classic at 0.5, struck low, hum lifted, heavy sub layer. RC-3: damping 0.05 (was 0.95).
    // FX: large damped hall, low shelf up.
    presets.push_back({ "Large Bells", "Deep Bronze Tower", {
        {"partialModel", 0.0f}, {"material", 0.0f}, {"inharmonicity", 0.5f},
        {"strikePosition", 0.15f}, {"malletHardness", 0.5f}, {"damping", 0.05f},
        {"overtoneBrightness", 0.3f}, {"acousticBrightness", 0.5f}, {"airAbsorption", 0.3f}, {"airAbsorptionTime", 5.0f},
        {"humLevel", 2.0f}, {"primeLevel", 0.0f}, {"humFollow", 0.0f},
        {"bloomSpeed", 0.47f}, {"bloomAmount", 0.3f}, {"shimmer", 0.2f},
        {"unisonCount", 2.0f}, {"unisonDetune", 12.0f},
        {"octaveBlendSub", 0.6f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.85f},
        {"strikeTime", 45.0f}, {"brilliance", 30.0f}, {"bodyTime", 4500.0f}, {"humSustain", 100.0f},
        {"attackLevel", 0.5f}, {"pitchEnvelope", 0.03f}, {"pitchEnvTime", 80.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.35f},
        {"eqLowGain", 3.0f},
        {"reverbSize", 0.85f}, {"reverbDamp", 0.6f}, {"reverbPredelay", 40.0f}, {"reverbMix", 0.4f}, {"reverbMod", 0.15f},
        {"outputGain", -6.0f}
    }, {} });

    // Same bell family as the Tower but struck hard and bright: prime lifted, octave layer, minor-third tierce (-8 c).
    // FX: bright hall with long pre-delay.
    presets.push_back({ "Large Bells", "Grand Cathedral Bell", {
        {"partialModel", 0.0f}, {"material", 0.0f}, {"inharmonicity", 0.5f}, {"partialTuning", -8.0f},
        {"strikePosition", 0.55f}, {"malletHardness", 0.8f}, {"damping", 0.1f},
        {"overtoneBrightness", 0.75f}, {"acousticBrightness", 0.85f}, {"airAbsorption", 0.1f}, {"airAbsorptionTime", 6.0f},
        {"humLevel", 3.0f}, {"primeLevel", 4.0f}, {"humFollow", 0.0f},
        {"bloomSpeed", 0.15f}, {"bloomAmount", 0.15f}, {"shimmer", 0.2f},
        {"unisonCount", 2.0f}, {"unisonDetune", 6.0f},
        {"octaveBlendSub", 0.25f}, {"octaveBlendOct", 0.3f}, {"stereoSpread", 0.9f},
        {"strikeTime", 25.0f}, {"brilliance", 75.0f}, {"bodyTime", 4000.0f}, {"humSustain", 85.0f},
        {"attackLevel", 0.7f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.4f},
        {"reverbSize", 0.9f}, {"reverbDamp", 0.3f}, {"reverbPredelay", 80.0f}, {"reverbMix", 0.5f}, {"reverbMod", 0.3f},
        {"outputGain", -3.0f}
    }, {} });

    // Cast iron toward gamelan ratios, sub at 0.85, 3.5 kHz low-pass, wide 3-voice unison. RC-3: damping 0 (was 1.0).
    // FX: largest, darkest hall; EQ low up, high down.
    presets.push_back({ "Large Bells", "Massive Iron Bell", {
        {"partialModel", 0.0f}, {"material", 4.0f}, {"inharmonicity", 0.72f},
        {"strikePosition", 0.3f}, {"malletHardness", 0.45f}, {"damping", 0.0f},
        {"overtoneBrightness", 0.12f}, {"acousticBrightness", 0.3f}, {"airAbsorption", 0.4f}, {"airAbsorptionTime", 5.5f},
        {"humLevel", 3.0f}, {"primeLevel", 0.0f}, {"humFollow", 0.0f},
        {"bloomSpeed", 0.87f}, {"bloomAmount", 0.45f}, {"shimmer", 0.15f},
        {"unisonCount", 3.0f}, {"unisonDetune", 22.0f},
        {"octaveBlendSub", 0.85f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.9f},
        {"strikeTime", 55.0f}, {"brilliance", 15.0f}, {"bodyTime", 5000.0f}, {"humSustain", 100.0f},
        {"attackLevel", 0.45f}, {"nonlinearEffects", 0.25f}, {"pitchEnvelope", 0.05f}, {"pitchEnvTime", 100.0f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 3500.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.3f},
        {"eqLowGain", 4.0f}, {"eqHighGain", -6.0f},
        {"reverbSize", 1.0f}, {"reverbDamp", 0.85f}, {"reverbPredelay", 60.0f}, {"reverbMix", 0.45f}, {"reverbMod", 0.1f},
        {"outputGain", -8.0f}
    }, {} });

    // Softest mallet, slowest bloom, prime pulled down 12 dB so the hum carries the note.
    // FX: 1.2 s echo - the toll repeats.
    presets.push_back({ "Large Bells", "Slow Tolling Bell", {
        {"partialModel", 0.0f}, {"material", 0.0f}, {"inharmonicity", 0.35f},
        {"strikePosition", 0.6f}, {"malletHardness", 0.15f}, {"damping", 0.0f},
        {"overtoneBrightness", 0.4f}, {"acousticBrightness", 0.6f}, {"airAbsorption", 0.25f}, {"airAbsorptionTime", 7.0f},
        {"humLevel", 6.0f}, {"primeLevel", -12.0f}, {"humFollow", 0.0f},
        {"bloomSpeed", 1.0f}, {"bloomAmount", 0.8f}, {"shimmer", 0.3f},
        {"unisonCount", 1.0f},
        {"octaveBlendSub", 0.2f}, {"octaveBlendOct", 0.35f}, {"stereoSpread", 0.7f},
        {"strikeTime", 90.0f}, {"brilliance", 45.0f}, {"bodyTime", 5000.0f}, {"humSustain", 100.0f},
        {"attackLevel", 0.1f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 2.0f}, {"humanize", 0.45f},
        {"eqMidGain", -3.0f}, {"eqMidFreq", 800.0f},
        {"delayTime", 1.2f}, {"delayFeedback", 0.5f}, {"delayMode", 0.0f}, {"delayMix", 0.2f},
        {"reverbSize", 0.8f}, {"reverbDamp", 0.5f}, {"reverbPredelay", 50.0f}, {"reverbMix", 0.45f}, {"reverbMod", 0.2f},
        {"outputGain", -2.0f}
    }, {} });

    // ========== METALLIC (5 presets) ==========
    // Plates, gongs, gamelan: the inharmonic end

    // Plate model - a gong IS a plate. 4-voice unison at 35 c, heavy nonlinear, slow bloom, pitch dip.
    // FX: modulated hall, mid push at 400 Hz.
    presets.push_back({ "Metallic", "Beating Bronze Gong", {
        {"partialModel", 2.0f}, {"material", 0.0f}, {"inharmonicity", 0.7f},
        {"strikePosition", 0.4f}, {"malletHardness", 0.25f}, {"damping", 0.2f},
        {"overtoneBrightness", 0.55f}, {"acousticBrightness", 0.7f},
        {"humLevel", 0.0f}, {"primeLevel", 0.0f}, {"humFollow", 0.0f},
        {"bloomSpeed", 1.0f}, {"bloomAmount", 0.7f}, {"shimmer", 0.7f},
        {"unisonCount", 4.0f}, {"unisonDetune", 35.0f},
        {"octaveBlendSub", 0.5f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 1.0f},
        {"strikeTime", 70.0f}, {"brilliance", 60.0f}, {"bodyTime", 4500.0f}, {"humSustain", 80.0f},
        {"attackLevel", 0.3f}, {"nonlinearEffects", 0.7f}, {"pitchEnvelope", 0.15f}, {"pitchEnvTime", 200.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 1.0f}, {"humanize", 0.4f},
        {"eqMidGain", 3.0f}, {"eqMidFreq", 400.0f},
        {"reverbSize", 0.75f}, {"reverbDamp", 0.5f}, {"reverbPredelay", 30.0f}, {"reverbMix", 0.4f}, {"reverbMod", 0.5f},
        {"outputGain", 5.5f}
    }, {} });

    // Plate model at full stretch. RC-3: the shortest preset - damping 1, body 350 ms, Hum Follow 1, hum removed.
    // FX: 90 ms slap, tiny bright room.
    presets.push_back({ "Metallic", "Clanging Steel Plate", {
        {"partialModel", 2.0f}, {"material", 2.0f}, {"inharmonicity", 1.0f},
        {"strikePosition", 0.9f}, {"malletHardness", 1.0f}, {"damping", 1.0f},
        {"overtoneBrightness", 0.9f}, {"acousticBrightness", 0.5f},
        {"humLevel", -24.0f}, {"primeLevel", -6.0f}, {"humFollow", 1.0f},
        {"bloomSpeed", 0.0f}, {"bloomAmount", 0.02f}, {"shimmer", 0.1f},
        {"unisonCount", 1.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.6f},
        {"strikeTime", 5.0f}, {"brilliance", 40.0f}, {"bodyTime", 350.0f}, {"humSustain", 0.0f},
        {"attackLevel", 1.0f}, {"nonlinearEffects", 0.5f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 1.0f}, {"humanize", 0.35f},
        {"eqMidGain", 4.0f}, {"eqMidFreq", 2500.0f},
        {"delayTime", 0.09f}, {"delayFeedback", 0.2f}, {"delayMode", 0.0f}, {"delayMix", 0.25f},
        {"reverbSize", 0.2f}, {"reverbDamp", 0.2f}, {"reverbPredelay", 0.0f}, {"reverbMix", 0.15f}, {"reverbMod", 0.0f},
        {"outputGain", 5.0f}
    }, {} });

    // Bowl model in cast iron under a 2.5 kHz low-pass, hum lifted, heavy sub.
    // FX: huge fully damped hall, high shelf at -8 dB.
    presets.push_back({ "Metallic", "Dark Iron Resonance", {
        {"partialModel", 3.0f}, {"material", 4.0f}, {"inharmonicity", 0.3f},
        {"strikePosition", 0.25f}, {"malletHardness", 0.3f}, {"damping", 0.25f},
        {"overtoneBrightness", 0.1f}, {"acousticBrightness", 0.2f}, {"airAbsorption", 0.3f}, {"airAbsorptionTime", 3.0f},
        {"humLevel", 6.0f}, {"primeLevel", 0.0f}, {"humFollow", 0.0f},
        {"bloomSpeed", 0.73f}, {"bloomAmount", 0.4f}, {"shimmer", 0.35f},
        {"unisonCount", 2.0f}, {"unisonDetune", 18.0f},
        {"octaveBlendSub", 0.7f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.8f},
        {"strikeTime", 60.0f}, {"brilliance", 10.0f}, {"bodyTime", 4000.0f}, {"humSustain", 90.0f},
        {"attackLevel", 0.25f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 2500.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.3f},
        {"eqHighGain", -8.0f},
        {"reverbSize", 0.95f}, {"reverbDamp", 0.9f}, {"reverbPredelay", 100.0f}, {"reverbMix", 0.5f}, {"reverbMod", 0.3f},
        {"outputGain", 0.5f}
    }, {} });

    // Classic at full gamelan ratios, 3-voice unison at 30 c for the ombak beating, tierce pushed +35 c.
    // No octave layer (as in v2.2.1): Classic has no Nyquist guard. FX: ping-pong delay.
    presets.push_back({ "Metallic", "Dense Bronze Gamelan", {
        {"partialModel", 0.0f}, {"material", 0.0f}, {"inharmonicity", 1.0f}, {"partialTuning", 35.0f},
        {"strikePosition", 0.7f}, {"malletHardness", 0.75f}, {"damping", 0.75f},
        {"overtoneBrightness", 0.65f}, {"acousticBrightness", 0.75f},
        {"humLevel", -6.0f}, {"primeLevel", 0.0f}, {"humFollow", 0.3f},
        {"bloomSpeed", 0.01f}, {"bloomAmount", 0.1f}, {"shimmer", 0.45f},
        {"unisonCount", 3.0f}, {"unisonDetune", 30.0f},
        {"octaveBlendSub", 0.35f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.95f},
        {"strikeTime", 12.0f}, {"brilliance", 65.0f}, {"bodyTime", 1600.0f}, {"humSustain", 35.0f},
        {"attackLevel", 0.65f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.5f},
        {"delayTime", 0.25f}, {"delayFeedback", 0.45f}, {"delayMode", 1.0f}, {"delayMix", 0.22f},
        {"reverbSize", 0.45f}, {"reverbDamp", 0.35f}, {"reverbPredelay", 20.0f}, {"reverbMix", 0.3f}, {"reverbMod", 0.2f},
        {"outputGain", 8.0f}
    }, {} });

    // Glass model, octave layer at full, 4-voice unison at 50 c, shimmer at full, hum and prime pulled down.
    // FX: fast chorus, short high-feedback ping-pong, shimmer reverb.
    presets.push_back({ "Metallic", "Shimmering Bell Tree", {
        {"partialModel", 4.0f}, {"material", 1.0f}, {"inharmonicity", 0.85f},
        {"strikePosition", 0.95f}, {"malletHardness", 0.9f}, {"damping", 0.75f},
        {"overtoneBrightness", 1.0f}, {"acousticBrightness", 1.0f},
        {"humLevel", -24.0f}, {"primeLevel", -10.0f}, {"humFollow", 0.5f},
        {"bloomSpeed", 0.0f}, {"bloomAmount", 0.05f}, {"shimmer", 1.0f},
        {"unisonCount", 4.0f}, {"unisonDetune", 50.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 1.0f}, {"stereoSpread", 1.0f},
        {"strikeTime", 5.0f}, {"brilliance", 100.0f}, {"bodyTime", 1100.0f}, {"humSustain", 25.0f},
        {"attackLevel", 0.7f},
        {"strikeNoiseChar", 2.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.6f},
        {"chorusRate", 6.0f}, {"chorusDepth", 0.5f}, {"chorusMix", 0.4f},
        {"delayTime", 0.12f}, {"delayFeedback", 0.55f}, {"delayMode", 1.0f}, {"delayMix", 0.25f},
        {"reverbSize", 0.6f}, {"reverbDamp", 0.1f}, {"reverbPredelay", 10.0f}, {"reverbMix", 0.4f}, {"reverbMod", 0.4f}, {"reverbShimmer", 0.5f},
        {"outputGain", 9.0f}
    }, {} });

    // ========== WARM BELLS (5 presets) ==========
    // Soft mallets, prime-led spectra, mid damping

    // Small brass bell: near-harmonic, hum removed, short body.
    // FX: small dry room only.
    presets.push_back({ "Warm Bells", "Gentle Hand Bell", {
        {"partialModel", 0.0f}, {"material", 1.0f}, {"inharmonicity", 0.2f},
        {"strikePosition", 0.4f}, {"malletHardness", 0.45f}, {"damping", 0.45f},
        {"overtoneBrightness", 0.45f}, {"acousticBrightness", 0.65f},
        {"humLevel", -24.0f}, {"primeLevel", 2.0f}, {"humFollow", 0.2f},
        {"bloomSpeed", 0.07f}, {"bloomAmount", 0.1f}, {"shimmer", 0.15f},
        {"unisonCount", 1.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.3f}, {"stereoSpread", 0.5f},
        {"strikeTime", 20.0f}, {"brilliance", 55.0f}, {"bodyTime", 1200.0f}, {"humSustain", 40.0f},
        {"attackLevel", 0.45f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.35f},
        {"reverbSize", 0.3f}, {"reverbDamp", 0.5f}, {"reverbPredelay", 10.0f}, {"reverbMix", 0.2f}, {"reverbMod", 0.1f},
        {"outputGain", 8.0f}
    }, {} });

    // Bowl model, soft mallet, slow beating shimmer.
    // FX: slow shallow chorus, damped medium room.
    presets.push_back({ "Warm Bells", "Mellow Brass Bowl", {
        {"partialModel", 3.0f}, {"material", 1.0f}, {"inharmonicity", 0.5f},
        {"strikePosition", 0.35f}, {"malletHardness", 0.2f}, {"damping", 0.35f},
        {"overtoneBrightness", 0.4f}, {"acousticBrightness", 0.6f},
        {"humLevel", -18.0f}, {"primeLevel", 3.0f}, {"humFollow", 0.3f},
        {"bloomSpeed", 0.41f}, {"bloomAmount", 0.3f}, {"shimmer", 0.5f},
        {"unisonCount", 2.0f}, {"unisonDetune", 5.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.7f},
        {"strikeTime", 40.0f}, {"brilliance", 50.0f}, {"bodyTime", 3000.0f}, {"humSustain", 70.0f},
        {"attackLevel", 0.2f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.3f},
        {"chorusRate", 0.3f}, {"chorusDepth", 0.3f}, {"chorusMix", 0.25f},
        {"reverbSize", 0.6f}, {"reverbDamp", 0.7f}, {"reverbPredelay", 25.0f}, {"reverbMix", 0.35f}, {"reverbMod", 0.25f},
        {"outputGain", -1.5f}
    }, {} });

    // A true bell (0.55) hit with the softest mallet at the centre, 5 kHz low-pass.
    // FX: damped room, EQ tilted down.
    presets.push_back({ "Warm Bells", "Soft Mallet Bronze", {
        {"partialModel", 0.0f}, {"material", 0.0f}, {"inharmonicity", 0.55f},
        {"strikePosition", 0.1f}, {"malletHardness", 0.0f}, {"damping", 0.5f},
        {"overtoneBrightness", 0.2f}, {"acousticBrightness", 0.4f},
        {"humLevel", 2.0f}, {"primeLevel", 3.0f}, {"humFollow", 0.2f},
        {"bloomSpeed", 0.2f}, {"bloomAmount", 0.2f}, {"shimmer", 0.1f},
        {"unisonCount", 1.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.45f},
        {"strikeTime", 50.0f}, {"brilliance", 25.0f}, {"bodyTime", 2200.0f}, {"humSustain", 50.0f},
        {"attackLevel", 0.05f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 5000.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 2.0f}, {"humanize", 0.35f},
        {"eqLowGain", 2.0f}, {"eqHighGain", -4.0f},
        {"reverbSize", 0.45f}, {"reverbDamp", 0.8f}, {"reverbPredelay", 15.0f}, {"reverbMix", 0.25f}, {"reverbMod", 0.1f},
        {"outputGain", -4.5f}
    }, {} });

    // Fully harmonic Classic, hum removed, prime at +6 dB, 4-voice unison, sub + octave layers.
    // FX: deep slow chorus - the velvet.
    presets.push_back({ "Warm Bells", "Velvet Bronze Tone", {
        {"partialModel", 0.0f}, {"material", 0.0f}, {"inharmonicity", 0.0f},
        {"strikePosition", 0.5f}, {"malletHardness", 0.3f}, {"damping", 0.4f},
        {"overtoneBrightness", 0.5f}, {"acousticBrightness", 0.55f},
        {"humLevel", -24.0f}, {"primeLevel", 6.0f}, {"humFollow", 0.1f},
        {"bloomSpeed", 0.6f}, {"bloomAmount", 0.5f}, {"shimmer", 0.3f},
        {"unisonCount", 4.0f}, {"unisonDetune", 14.0f},
        {"octaveBlendSub", 0.5f}, {"octaveBlendOct", 0.3f}, {"stereoSpread", 0.9f},
        {"strikeTime", 60.0f}, {"brilliance", 40.0f}, {"bodyTime", 2600.0f}, {"humSustain", 60.0f},
        {"attackLevel", 0.15f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.3f},
        {"chorusRate", 0.6f}, {"chorusDepth", 0.6f}, {"chorusMix", 0.45f},
        {"reverbSize", 0.55f}, {"reverbDamp", 0.6f}, {"reverbPredelay", 20.0f}, {"reverbMix", 0.35f}, {"reverbMod", 0.3f},
        {"outputGain", 7.5f}
    }, {} });

    // Tubular model compressed (0.2), soft mallet, prime only - vibraphone territory.
    // FX: 4.5 Hz chorus as the motor, small room.
    presets.push_back({ "Warm Bells", "Warm Aluminum Bars", {
        {"partialModel", 1.0f}, {"material", 3.0f}, {"inharmonicity", 0.2f},
        {"strikePosition", 0.3f}, {"malletHardness", 0.35f}, {"damping", 0.55f},
        {"overtoneBrightness", 0.25f}, {"acousticBrightness", 0.5f},
        {"humLevel", -24.0f}, {"primeLevel", 6.0f}, {"humFollow", 0.6f},
        {"bloomSpeed", 0.04f}, {"bloomAmount", 0.1f}, {"shimmer", 0.1f},
        {"unisonCount", 1.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.6f},
        {"strikeTime", 25.0f}, {"brilliance", 35.0f}, {"bodyTime", 1500.0f}, {"humSustain", 30.0f},
        {"attackLevel", 0.35f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"humanize", 0.25f},
        {"chorusRate", 4.5f}, {"chorusDepth", 0.35f}, {"chorusMix", 0.35f},
        {"reverbSize", 0.4f}, {"reverbDamp", 0.55f}, {"reverbPredelay", 15.0f}, {"reverbMix", 0.25f}, {"reverbMod", 0.15f},
        {"outputGain", 7.5f}
    }, {} });

    presetManager.initializeFactoryPresets(presets);

    // Stamp the schema version so the corrected presets aren't regenerated again
    // until the next factory-format change.
    presetManager.getFactoryPresetsDirectory().createDirectory();
    versionFile.replaceWithText(factoryVersion);
}

//==============================================================================
// v3.0.0: Tuning parameter change callback
void OBellsAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "tuning_masterTune")
    {
        tuningEngine.setMasterTune(static_cast<double>(newValue));
    }
    else if (parameterID == "tuning_octaveStretch")
    {
        tuningEngine.setOctaveStretch(newValue);
    }
    else if (parameterID == "tuning_pitchBendRange")
    {
        tuningEngine.setPitchBendRange(newValue);
    }
    else if (parameterID == "tuning_temperamentPreset")
    {
        int preset = static_cast<int>(newValue);
        tuningEngine.setBuiltInPreset(static_cast<TuningEngine::BuiltInPreset>(preset));
    }
}

//==============================================================================
// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBellsAudioProcessor();
}
