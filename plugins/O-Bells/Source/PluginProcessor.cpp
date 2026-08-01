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
#include "PluginEditor.h"

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
juce::AudioProcessorEditor* OBellsAudioProcessor::createEditor()
{
    return new OBellsAudioProcessorEditor(*this);
}

//==============================================================================
void OBellsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void OBellsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());
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
    const juce::String factoryVersion = "4.1.1";
    auto versionFile = presetManager.getFactoryPresetsDirectory().getChildFile(".factory_version");

    if (presetManager.factoryPresetsExist()
        && versionFile.existsAsFile()
        && versionFile.loadFileAsString().trim() == factoryVersion)
        return;

    std::vector<OuariconPresetManager::FactoryPresetDef> presets;

    // ==========================================================================
    // v2.2.1 FACTORY PRESETS - Research-Informed Bell Sounds
    // ==========================================================================
    // Designed using acoustic research on real bells (modal-synthesis-bells-academic-research.md):
    // - Church bell partial ratios: Hum(0.25), Prime(0.5), Tierce(0.6), Quint(0.75), Nominal(1.0)
    // - Frequency-dependent damping: R_k = b_1 + b_3 * f_k^2 (higher partials decay faster)
    // - Multi-stage envelope: Strike (1-5ms) → Body (0.5-3s) → Hum tail (5-15s)
    // - Risset bell inharmonicity ratios for beating effects
    //
    // Material Choice: 0=Bronze, 1=Brass, 2=Steel, 3=Aluminum, 4=Cast Iron
    // StrikeNoiseChar Choice: 0=Click, 1=Thud, 2=Ping
    // VelocityCurve Choice: 0=Linear, 1=Exponential, 2=Logarithmic
    //
    // v2.2.0+ parameters utilized: airAbsorption, airAbsorptionTime, acousticBrightness
    // v3.2.0+ parameters utilized: humanize, lpFilterEnabled, lpFilterCutoff,
    //   bloomFineEnabled, bloomSpeedLow/Mid/High, bloomAmountLow/Mid/High,
    //   nonlinearEffects, partialTuning, pitchEnvelope, pitchEnvTime (expanded usage)
    // ==========================================================================

    // ========== LARGE BELLS (5 presets) ==========
    // Deep, long-sustaining bell tones with extended decay

    // Large tower bell - deep bronze with rich hum tail
    // Research: Church bells have T60 of 8-15 seconds, hum partial persists longest
    presets.push_back({ "Large Bells", "Deep Bronze Tower", {
        {"strikePosition", 0.25f}, {"malletHardness", 0.55f}, {"damping", 0.95f},
        {"overtoneBrightness", 0.42f}, {"acousticBrightness", 0.55f}, {"material", 0.0f}, {"inharmonicity", 0.55f},
        {"airAbsorption", 0.35f}, {"airAbsorptionTime", 4.0f},
        {"bloomSpeed", 0.75f}, {"bloomAmount", 0.32f}, {"shimmer", 0.22f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 0.85f}, {"bloomSpeedMid", 0.7f}, {"bloomSpeedHigh", 0.55f},
        {"bloomAmountLow", 0.4f}, {"bloomAmountMid", 0.3f}, {"bloomAmountHigh", 0.18f},
        {"unisonCount", 2.0f}, {"unisonDetune", 12.0f},
        {"octaveBlendSub", 0.45f}, {"octaveBlendOct", 0.12f}, {"stereoSpread", 0.85f},
        {"strikeTime", 45.0f}, {"brilliance", 38.0f}, {"bodyTime", 3200.0f}, {"humSustain", 82.0f},
        {"attackLevel", 0.52f}, {"reverbMix", 0.55f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.35f}, {"pitchEnvelope", 0.03f}, {"pitchEnvTime", 80.0f}
    }, {} });

    // Heavy cast iron bell - dark, industrial quality
    // Research: Cast iron bells have stronger low partials, longer decay than bronze
    presets.push_back({ "Large Bells", "Massive Iron Bell", {
        {"strikePosition", 0.2f}, {"malletHardness", 0.48f}, {"damping", 1.0f},
        {"overtoneBrightness", 0.28f}, {"acousticBrightness", 0.45f}, {"material", 4.0f}, {"inharmonicity", 0.65f},
        {"airAbsorption", 0.42f}, {"airAbsorptionTime", 5.5f},
        {"bloomSpeed", 0.88f}, {"bloomAmount", 0.45f}, {"shimmer", 0.18f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 0.95f}, {"bloomSpeedMid", 0.82f}, {"bloomSpeedHigh", 0.65f},
        {"bloomAmountLow", 0.55f}, {"bloomAmountMid", 0.4f}, {"bloomAmountHigh", 0.22f},
        {"unisonCount", 3.0f}, {"unisonDetune", 18.0f},
        {"octaveBlendSub", 0.62f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.92f},
        {"strikeTime", 55.0f}, {"brilliance", 22.0f}, {"bodyTime", 4000.0f}, {"humSustain", 90.0f},
        {"attackLevel", 0.45f}, {"reverbMix", 0.6f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 6500.0f},
        {"nonlinearEffects", 0.08f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.3f}, {"pitchEnvelope", 0.04f}, {"pitchEnvTime", 100.0f}
    }, {} });

    // Warm brass bell - resonant, musical tone
    // Research: Brass bells have shorter decay but warmer midrange
    presets.push_back({ "Large Bells", "Cavernous Brass", {
        {"strikePosition", 0.32f}, {"malletHardness", 0.52f}, {"damping", 0.88f},
        {"overtoneBrightness", 0.55f}, {"acousticBrightness", 0.62f}, {"material", 1.0f}, {"inharmonicity", 0.48f},
        {"airAbsorption", 0.28f}, {"airAbsorptionTime", 3.5f},
        {"bloomSpeed", 0.65f}, {"bloomAmount", 0.28f}, {"shimmer", 0.25f},
        {"unisonCount", 2.0f}, {"unisonDetune", 10.0f},
        {"octaveBlendSub", 0.38f}, {"octaveBlendOct", 0.15f}, {"stereoSpread", 0.78f},
        {"strikeTime", 38.0f}, {"brilliance", 48.0f}, {"bodyTime", 2800.0f}, {"humSustain", 72.0f},
        {"attackLevel", 0.55f}, {"reverbMix", 0.52f},
        {"partialTuning", -5.0f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.3f}
    }, {} });

    // Classic European church bell - balanced, traditional
    // Research: Well-tuned bells have Tierce at minor third (2.4x hum frequency)
    presets.push_back({ "Large Bells", "Grand Cathedral Bell", {
        {"strikePosition", 0.28f}, {"malletHardness", 0.58f}, {"damping", 0.92f},
        {"overtoneBrightness", 0.5f}, {"acousticBrightness", 0.58f}, {"material", 0.0f}, {"inharmonicity", 0.52f},
        {"airAbsorption", 0.32f}, {"airAbsorptionTime", 4.5f},
        {"bloomSpeed", 0.72f}, {"bloomAmount", 0.35f}, {"shimmer", 0.2f},
        {"unisonCount", 2.0f}, {"unisonDetune", 8.0f},
        {"octaveBlendSub", 0.42f}, {"octaveBlendOct", 0.18f}, {"stereoSpread", 0.88f},
        {"strikeTime", 42.0f}, {"brilliance", 45.0f}, {"bodyTime", 3500.0f}, {"humSustain", 78.0f},
        {"attackLevel", 0.58f}, {"reverbMix", 0.58f},
        {"partialTuning", -8.0f}, {"pitchEnvelope", 0.02f}, {"pitchEnvTime", 65.0f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.4f}
    }, {} });

    // Very slow, meditative bell - extended decay
    // Research: Maximum T60 for contemplative quality
    presets.push_back({ "Large Bells", "Slow Tolling Bell", {
        {"strikePosition", 0.22f}, {"malletHardness", 0.42f}, {"damping", 1.0f},
        {"overtoneBrightness", 0.38f}, {"acousticBrightness", 0.5f}, {"material", 0.0f}, {"inharmonicity", 0.58f},
        {"airAbsorption", 0.48f}, {"airAbsorptionTime", 6.0f},
        {"bloomSpeed", 0.92f}, {"bloomAmount", 0.52f}, {"shimmer", 0.28f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 0.98f}, {"bloomSpeedMid", 0.88f}, {"bloomSpeedHigh", 0.72f},
        {"bloomAmountLow", 0.6f}, {"bloomAmountMid", 0.48f}, {"bloomAmountHigh", 0.3f},
        {"unisonCount", 3.0f}, {"unisonDetune", 15.0f},
        {"octaveBlendSub", 0.55f}, {"octaveBlendOct", 0.08f}, {"stereoSpread", 0.95f},
        {"strikeTime", 60.0f}, {"brilliance", 32.0f}, {"bodyTime", 4500.0f}, {"humSustain", 95.0f},
        {"attackLevel", 0.38f}, {"reverbMix", 0.65f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 8000.0f},
        {"pitchEnvelope", 0.05f}, {"pitchEnvTime", 120.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.45f}
    }, {} });

    // ========== BRIGHT BELLS (5 presets) ==========
    // Clear, articulate tones with pronounced upper partials

    // Small bronze disc - pure, sustaining tone
    // Research: Crotales have nearly harmonic partials, extremely long sustain
    presets.push_back({ "Bright Bells", "Bright Clear Crotale", {
        {"strikePosition", 0.58f}, {"malletHardness", 0.82f}, {"damping", 0.88f},
        {"overtoneBrightness", 0.85f}, {"acousticBrightness", 0.82f}, {"material", 0.0f}, {"inharmonicity", 0.15f},
        {"airAbsorption", 0.12f}, {"airAbsorptionTime", 2.0f},
        {"bloomSpeed", 0.28f}, {"bloomAmount", 0.1f}, {"shimmer", 0.22f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.2f}, {"stereoSpread", 0.55f},
        {"strikeTime", 12.0f}, {"brilliance", 88.0f}, {"bodyTime", 1800.0f}, {"humSustain", 35.0f},
        {"attackLevel", 0.65f}, {"reverbMix", 0.38f},
        {"partialTuning", 3.0f},
        {"strikeNoiseChar", 2.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.15f}
    }, {} });

    // Steel tube - bright, tubular bell character
    // Research: Tubular bells have strike pitch from 4th/5th/6th partials in 2:3:4 ratio
    presets.push_back({ "Bright Bells", "Crystalline Steel Chime", {
        {"strikePosition", 0.52f}, {"malletHardness", 0.78f}, {"damping", 0.75f},
        {"overtoneBrightness", 0.78f}, {"acousticBrightness", 0.75f}, {"material", 2.0f}, {"inharmonicity", 0.42f},
        {"airAbsorption", 0.15f}, {"airAbsorptionTime", 1.8f},
        {"bloomSpeed", 0.32f}, {"bloomAmount", 0.12f}, {"shimmer", 0.18f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.1f}, {"stereoSpread", 0.48f},
        {"strikeTime", 18.0f}, {"brilliance", 82.0f}, {"bodyTime", 1400.0f}, {"humSustain", 28.0f},
        {"attackLevel", 0.62f}, {"reverbMix", 0.32f},
        {"pitchEnvelope", 0.04f}, {"pitchEnvTime", 25.0f},
        {"strikeNoiseChar", 2.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.2f}
    }, {} });

    // Light aluminum - shimmery, delicate
    // Research: Aluminum has high brightness, short decay
    presets.push_back({ "Bright Bells", "Sparkling Aluminum", {
        {"strikePosition", 0.62f}, {"malletHardness", 0.75f}, {"damping", 0.65f},
        {"overtoneBrightness", 0.88f}, {"acousticBrightness", 0.88f}, {"material", 3.0f}, {"inharmonicity", 0.28f},
        {"airAbsorption", 0.08f}, {"airAbsorptionTime", 1.2f},
        {"bloomSpeed", 0.22f}, {"bloomAmount", 0.08f}, {"shimmer", 0.35f},
        {"unisonCount", 2.0f}, {"unisonDetune", 8.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.28f}, {"stereoSpread", 0.65f},
        {"strikeTime", 10.0f}, {"brilliance", 92.0f}, {"bodyTime", 900.0f}, {"humSustain", 18.0f},
        {"attackLevel", 0.68f}, {"reverbMix", 0.35f},
        {"nonlinearEffects", 0.05f},
        {"strikeNoiseChar", 2.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.12f}
    }, {} });

    // Bright bronze plate - clear attack, singing sustain
    presets.push_back({ "Bright Bells", "Brilliant Bronze Plate", {
        {"strikePosition", 0.55f}, {"malletHardness", 0.72f}, {"damping", 0.78f},
        {"overtoneBrightness", 0.75f}, {"acousticBrightness", 0.72f}, {"material", 0.0f}, {"inharmonicity", 0.22f},
        {"airAbsorption", 0.18f}, {"airAbsorptionTime", 1.5f},
        {"bloomSpeed", 0.35f}, {"bloomAmount", 0.15f}, {"shimmer", 0.25f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.05f}, {"octaveBlendOct", 0.18f}, {"stereoSpread", 0.52f},
        {"strikeTime", 15.0f}, {"brilliance", 78.0f}, {"bodyTime", 1600.0f}, {"humSustain", 32.0f},
        {"attackLevel", 0.6f}, {"reverbMix", 0.4f},
        {"partialTuning", 5.0f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.18f}
    }, {} });

    // Steel bar - glockenspiel-like, pure fundamental
    // Research: Short bars suppress higher partials
    presets.push_back({ "Bright Bells", "Crisp Steel Bar", {
        {"strikePosition", 0.65f}, {"malletHardness", 0.88f}, {"damping", 0.55f},
        {"overtoneBrightness", 0.82f}, {"acousticBrightness", 0.78f}, {"material", 2.0f}, {"inharmonicity", 0.18f},
        {"airAbsorption", 0.05f}, {"airAbsorptionTime", 0.8f},
        {"bloomSpeed", 0.15f}, {"bloomAmount", 0.05f}, {"shimmer", 0.12f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.12f}, {"stereoSpread", 0.45f},
        {"strikeTime", 8.0f}, {"brilliance", 85.0f}, {"bodyTime", 800.0f}, {"humSustain", 15.0f},
        {"attackLevel", 0.75f}, {"reverbMix", 0.28f},
        {"pitchEnvelope", 0.06f}, {"pitchEnvTime", 12.0f},
        {"nonlinearEffects", 0.03f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.1f}
    }, {} });

    // ========== WARM BELLS (5 presets) ==========
    // Mellow tones with soft attack and rounded character

    // Soft felt mallet on bronze - warm, organ-like
    presets.push_back({ "Warm Bells", "Soft Mallet Bronze", {
        {"strikePosition", 0.35f}, {"malletHardness", 0.25f}, {"damping", 0.82f},
        {"overtoneBrightness", 0.42f}, {"acousticBrightness", 0.48f}, {"material", 0.0f}, {"inharmonicity", 0.35f},
        {"airAbsorption", 0.25f}, {"airAbsorptionTime", 3.0f},
        {"bloomSpeed", 0.58f}, {"bloomAmount", 0.35f}, {"shimmer", 0.15f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 0.65f}, {"bloomSpeedMid", 0.55f}, {"bloomSpeedHigh", 0.4f},
        {"bloomAmountLow", 0.42f}, {"bloomAmountMid", 0.32f}, {"bloomAmountHigh", 0.15f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.22f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.55f},
        {"strikeTime", 35.0f}, {"brilliance", 35.0f}, {"bodyTime", 2200.0f}, {"humSustain", 65.0f},
        {"attackLevel", 0.28f}, {"reverbMix", 0.45f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 7500.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 2.0f},
        {"humanize", 0.4f}
    }, {} });

    // Singing bowl character - meditative, beating
    // Research: Asymmetric bowls create 2-3Hz monaural beats
    presets.push_back({ "Warm Bells", "Mellow Brass Bowl", {
        {"strikePosition", 0.28f}, {"malletHardness", 0.3f}, {"damping", 0.9f},
        {"overtoneBrightness", 0.45f}, {"acousticBrightness", 0.52f}, {"material", 1.0f}, {"inharmonicity", 0.28f},
        {"airAbsorption", 0.2f}, {"airAbsorptionTime", 2.5f},
        {"bloomSpeed", 0.65f}, {"bloomAmount", 0.42f}, {"shimmer", 0.38f},
        {"unisonCount", 2.0f}, {"unisonDetune", 5.0f},
        {"octaveBlendSub", 0.18f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.6f},
        {"strikeTime", 40.0f}, {"brilliance", 42.0f}, {"bodyTime", 2800.0f}, {"humSustain", 72.0f},
        {"attackLevel", 0.32f}, {"reverbMix", 0.48f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 9000.0f},
        {"partialTuning", -12.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 2.0f},
        {"humanize", 0.35f}
    }, {} });

    // Vibraphone-like - warm aluminum with gentle attack
    // Research: Resonator tubes amplify fundamental, modal ratios 1:2.76:5.4
    presets.push_back({ "Warm Bells", "Warm Aluminum Bars", {
        {"strikePosition", 0.4f}, {"malletHardness", 0.38f}, {"damping", 0.72f},
        {"overtoneBrightness", 0.52f}, {"acousticBrightness", 0.58f}, {"material", 3.0f}, {"inharmonicity", 0.32f},
        {"airAbsorption", 0.12f}, {"airAbsorptionTime", 1.5f},
        {"bloomSpeed", 0.45f}, {"bloomAmount", 0.22f}, {"shimmer", 0.28f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.1f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.58f},
        {"strikeTime", 25.0f}, {"brilliance", 48.0f}, {"bodyTime", 1400.0f}, {"humSustain", 52.0f},
        {"attackLevel", 0.4f}, {"reverbMix", 0.35f},
        {"pitchEnvelope", 0.02f}, {"pitchEnvTime", 18.0f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 2.0f},
        {"humanize", 0.3f}
    }, {} });

    // Soft hand bell - gentle, intimate
    presets.push_back({ "Warm Bells", "Gentle Hand Bell", {
        {"strikePosition", 0.42f}, {"malletHardness", 0.35f}, {"damping", 0.68f},
        {"overtoneBrightness", 0.55f}, {"acousticBrightness", 0.55f}, {"material", 1.0f}, {"inharmonicity", 0.38f},
        {"airAbsorption", 0.15f}, {"airAbsorptionTime", 1.8f},
        {"bloomSpeed", 0.35f}, {"bloomAmount", 0.18f}, {"shimmer", 0.18f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.15f}, {"stereoSpread", 0.48f},
        {"strikeTime", 22.0f}, {"brilliance", 52.0f}, {"bodyTime", 1200.0f}, {"humSustain", 45.0f},
        {"attackLevel", 0.35f}, {"reverbMix", 0.38f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 12000.0f},
        {"pitchEnvelope", 0.015f}, {"pitchEnvTime", 30.0f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 2.0f},
        {"humanize", 0.35f}
    }, {} });

    // Dark, smooth bronze - velvet character
    presets.push_back({ "Warm Bells", "Velvet Bronze Tone", {
        {"strikePosition", 0.3f}, {"malletHardness", 0.22f}, {"damping", 0.85f},
        {"overtoneBrightness", 0.35f}, {"acousticBrightness", 0.42f}, {"material", 0.0f}, {"inharmonicity", 0.4f},
        {"airAbsorption", 0.32f}, {"airAbsorptionTime", 3.5f},
        {"bloomSpeed", 0.7f}, {"bloomAmount", 0.4f}, {"shimmer", 0.12f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 0.78f}, {"bloomSpeedMid", 0.65f}, {"bloomSpeedHigh", 0.45f},
        {"bloomAmountLow", 0.5f}, {"bloomAmountMid", 0.38f}, {"bloomAmountHigh", 0.12f},
        {"unisonCount", 2.0f}, {"unisonDetune", 6.0f},
        {"octaveBlendSub", 0.28f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.62f},
        {"strikeTime", 45.0f}, {"brilliance", 28.0f}, {"bodyTime", 2500.0f}, {"humSustain", 68.0f},
        {"attackLevel", 0.25f}, {"reverbMix", 0.5f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 5500.0f},
        {"partialTuning", -6.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 2.0f},
        {"humanize", 0.45f}
    }, {} });

    // ========== METALLIC (5 presets) ==========
    // Complex spectra with pronounced inharmonicity

    // Gamelan-inspired - extreme inharmonicity
    // Research: Sléndro tuning derived from inharmonic bonang spectrum
    presets.push_back({ "Metallic", "Dense Bronze Gamelan", {
        {"strikePosition", 0.55f}, {"malletHardness", 0.62f}, {"damping", 0.58f},
        {"overtoneBrightness", 0.58f}, {"acousticBrightness", 0.65f}, {"material", 0.0f}, {"inharmonicity", 0.88f},
        {"airAbsorption", 0.1f}, {"airAbsorptionTime", 1.2f},
        {"bloomSpeed", 0.18f}, {"bloomAmount", 0.08f}, {"shimmer", 0.1f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.5f},
        {"strikeTime", 12.0f}, {"brilliance", 62.0f}, {"bodyTime", 600.0f}, {"humSustain", 22.0f},
        {"attackLevel", 0.72f}, {"reverbMix", 0.25f},
        {"nonlinearEffects", 0.12f}, {"partialTuning", 15.0f},
        {"pitchEnvelope", 0.03f}, {"pitchEnvTime", 15.0f},
        {"strikeNoiseChar", 2.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.2f}
    }, {} });

    // Industrial steel plate - clanging, harsh
    presets.push_back({ "Metallic", "Clanging Steel Plate", {
        {"strikePosition", 0.7f}, {"malletHardness", 0.85f}, {"damping", 0.52f},
        {"overtoneBrightness", 0.78f}, {"acousticBrightness", 0.72f}, {"material", 2.0f}, {"inharmonicity", 0.72f},
        {"airAbsorption", 0.08f}, {"airAbsorptionTime", 0.8f},
        {"bloomSpeed", 0.15f}, {"bloomAmount", 0.05f}, {"shimmer", 0.08f},
        {"unisonCount", 2.0f}, {"unisonDetune", 15.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.15f}, {"stereoSpread", 0.72f},
        {"strikeTime", 8.0f}, {"brilliance", 75.0f}, {"bodyTime", 500.0f}, {"humSustain", 15.0f},
        {"attackLevel", 0.85f}, {"reverbMix", 0.2f},
        {"nonlinearEffects", 0.18f}, {"partialTuning", 22.0f},
        {"pitchEnvelope", 0.08f}, {"pitchEnvTime", 10.0f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 1.0f},
        {"humanize", 0.15f}
    }, {} });

    // Complex gong - beating partials, evolving texture
    // Research: Gongs have strongly inharmonic partials with slow beating
    presets.push_back({ "Metallic", "Beating Bronze Gong", {
        {"strikePosition", 0.22f}, {"malletHardness", 0.48f}, {"damping", 0.95f},
        {"overtoneBrightness", 0.42f}, {"acousticBrightness", 0.52f}, {"material", 0.0f}, {"inharmonicity", 0.78f},
        {"airAbsorption", 0.35f}, {"airAbsorptionTime", 4.0f},
        {"bloomSpeed", 0.82f}, {"bloomAmount", 0.55f}, {"shimmer", 0.32f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 0.9f}, {"bloomSpeedMid", 0.78f}, {"bloomSpeedHigh", 0.6f},
        {"bloomAmountLow", 0.65f}, {"bloomAmountMid", 0.5f}, {"bloomAmountHigh", 0.35f},
        {"unisonCount", 3.0f}, {"unisonDetune", 22.0f},
        {"octaveBlendSub", 0.45f}, {"octaveBlendOct", 0.12f}, {"stereoSpread", 0.92f},
        {"strikeTime", 55.0f}, {"brilliance", 35.0f}, {"bodyTime", 3800.0f}, {"humSustain", 85.0f},
        {"attackLevel", 0.42f}, {"reverbMix", 0.55f},
        {"nonlinearEffects", 0.15f}, {"partialTuning", -18.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.25f}
    }, {} });

    // Bell tree - multiple resonances, cascading
    presets.push_back({ "Metallic", "Shimmering Bell Tree", {
        {"strikePosition", 0.6f}, {"malletHardness", 0.68f}, {"damping", 0.72f},
        {"overtoneBrightness", 0.72f}, {"acousticBrightness", 0.7f}, {"material", 1.0f}, {"inharmonicity", 0.55f},
        {"airAbsorption", 0.12f}, {"airAbsorptionTime", 1.5f},
        {"bloomSpeed", 0.28f}, {"bloomAmount", 0.15f}, {"shimmer", 0.45f},
        {"unisonCount", 4.0f}, {"unisonDetune", 25.0f},
        {"octaveBlendSub", 0.08f}, {"octaveBlendOct", 0.25f}, {"stereoSpread", 0.88f},
        {"strikeTime", 15.0f}, {"brilliance", 72.0f}, {"bodyTime", 1100.0f}, {"humSustain", 38.0f},
        {"attackLevel", 0.58f}, {"reverbMix", 0.42f},
        {"nonlinearEffects", 0.06f},
        {"pitchEnvelope", 0.05f}, {"pitchEnvTime", 20.0f},
        {"strikeNoiseChar", 2.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.28f}
    }, {} });

    // Heavy iron - dark, complex
    presets.push_back({ "Metallic", "Dark Iron Resonance", {
        {"strikePosition", 0.25f}, {"malletHardness", 0.52f}, {"damping", 0.88f},
        {"overtoneBrightness", 0.32f}, {"acousticBrightness", 0.45f}, {"material", 4.0f}, {"inharmonicity", 0.75f},
        {"airAbsorption", 0.4f}, {"airAbsorptionTime", 4.5f},
        {"bloomSpeed", 0.75f}, {"bloomAmount", 0.42f}, {"shimmer", 0.22f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 0.85f}, {"bloomSpeedMid", 0.7f}, {"bloomSpeedHigh", 0.5f},
        {"bloomAmountLow", 0.52f}, {"bloomAmountMid", 0.38f}, {"bloomAmountHigh", 0.2f},
        {"unisonCount", 2.0f}, {"unisonDetune", 12.0f},
        {"octaveBlendSub", 0.52f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.82f},
        {"strikeTime", 48.0f}, {"brilliance", 25.0f}, {"bodyTime", 3200.0f}, {"humSustain", 78.0f},
        {"attackLevel", 0.48f}, {"reverbMix", 0.52f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 5000.0f},
        {"nonlinearEffects", 0.1f}, {"partialTuning", -10.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.22f}
    }, {} });

    // ========== AMBIENT (5 presets) ==========
    // Atmospheric, evolving textures

    // Large bell heard from distance - air absorption effect
    presets.push_back({ "Ambient", "Distant Cathedral", {
        {"strikePosition", 0.3f}, {"malletHardness", 0.45f}, {"damping", 0.95f},
        {"overtoneBrightness", 0.38f}, {"acousticBrightness", 0.42f}, {"material", 0.0f}, {"inharmonicity", 0.52f},
        {"airAbsorption", 0.72f}, {"airAbsorptionTime", 6.0f},
        {"bloomSpeed", 0.78f}, {"bloomAmount", 0.45f}, {"shimmer", 0.28f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 0.88f}, {"bloomSpeedMid", 0.72f}, {"bloomSpeedHigh", 0.55f},
        {"bloomAmountLow", 0.55f}, {"bloomAmountMid", 0.4f}, {"bloomAmountHigh", 0.2f},
        {"unisonCount", 3.0f}, {"unisonDetune", 18.0f},
        {"octaveBlendSub", 0.48f}, {"octaveBlendOct", 0.1f}, {"stereoSpread", 0.95f},
        {"strikeTime", 50.0f}, {"brilliance", 28.0f}, {"bodyTime", 4200.0f}, {"humSustain", 88.0f},
        {"attackLevel", 0.32f}, {"reverbMix", 0.75f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 4500.0f},
        {"pitchEnvelope", 0.02f}, {"pitchEnvTime", 90.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.42f}
    }, {} });

    // Heavy filtering - muffled, submerged
    presets.push_back({ "Ambient", "Underwater Bell", {
        {"strikePosition", 0.28f}, {"malletHardness", 0.32f}, {"damping", 0.92f},
        {"overtoneBrightness", 0.25f}, {"acousticBrightness", 0.28f}, {"material", 0.0f}, {"inharmonicity", 0.58f},
        {"airAbsorption", 0.88f}, {"airAbsorptionTime", 3.5f},
        {"bloomSpeed", 0.85f}, {"bloomAmount", 0.55f}, {"shimmer", 0.35f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 0.92f}, {"bloomSpeedMid", 0.8f}, {"bloomSpeedHigh", 0.6f},
        {"bloomAmountLow", 0.65f}, {"bloomAmountMid", 0.5f}, {"bloomAmountHigh", 0.25f},
        {"unisonCount", 4.0f}, {"unisonDetune", 20.0f},
        {"octaveBlendSub", 0.58f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.88f},
        {"strikeTime", 55.0f}, {"brilliance", 18.0f}, {"bodyTime", 3500.0f}, {"humSustain", 82.0f},
        {"attackLevel", 0.25f}, {"reverbMix", 0.72f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 3000.0f},
        {"partialTuning", -15.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.38f}
    }, {} });

    // Maximum bloom - slowly evolving pad
    presets.push_back({ "Ambient", "Evolving Bronze Wash", {
        {"strikePosition", 0.38f}, {"malletHardness", 0.28f}, {"damping", 1.0f},
        {"overtoneBrightness", 0.48f}, {"acousticBrightness", 0.52f}, {"material", 0.0f}, {"inharmonicity", 0.42f},
        {"airAbsorption", 0.35f}, {"airAbsorptionTime", 5.0f},
        {"bloomSpeed", 0.95f}, {"bloomAmount", 0.78f}, {"shimmer", 0.42f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 0.98f}, {"bloomSpeedMid", 0.92f}, {"bloomSpeedHigh", 0.8f},
        {"bloomAmountLow", 0.85f}, {"bloomAmountMid", 0.72f}, {"bloomAmountHigh", 0.55f},
        {"unisonCount", 4.0f}, {"unisonDetune", 15.0f},
        {"octaveBlendSub", 0.35f}, {"octaveBlendOct", 0.22f}, {"stereoSpread", 0.98f},
        {"strikeTime", 65.0f}, {"brilliance", 42.0f}, {"bodyTime", 4800.0f}, {"humSustain", 92.0f},
        {"attackLevel", 0.2f}, {"reverbMix", 0.68f},
        {"nonlinearEffects", 0.05f},
        {"pitchEnvelope", 0.01f}, {"pitchEnvTime", 150.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.5f}
    }, {} });

    // Maximum shimmer - icy, crystalline texture
    presets.push_back({ "Ambient", "Frozen Steel Shimmer", {
        {"strikePosition", 0.55f}, {"malletHardness", 0.58f}, {"damping", 0.85f},
        {"overtoneBrightness", 0.72f}, {"acousticBrightness", 0.68f}, {"material", 2.0f}, {"inharmonicity", 0.32f},
        {"airAbsorption", 0.18f}, {"airAbsorptionTime", 2.5f},
        {"bloomSpeed", 0.45f}, {"bloomAmount", 0.28f}, {"shimmer", 0.75f},
        {"unisonCount", 3.0f}, {"unisonDetune", 12.0f},
        {"octaveBlendSub", 0.12f}, {"octaveBlendOct", 0.35f}, {"stereoSpread", 0.92f},
        {"strikeTime", 18.0f}, {"brilliance", 78.0f}, {"bodyTime", 2200.0f}, {"humSustain", 55.0f},
        {"attackLevel", 0.45f}, {"reverbMix", 0.58f},
        {"nonlinearEffects", 0.04f}, {"partialTuning", 8.0f},
        {"strikeNoiseChar", 2.0f}, {"velocityCurve", 0.0f},
        {"humanize", 0.2f}
    }, {} });

    // Soft, ethereal pad texture
    presets.push_back({ "Ambient", "Ethereal Chime Pad", {
        {"strikePosition", 0.45f}, {"malletHardness", 0.22f}, {"damping", 0.98f},
        {"overtoneBrightness", 0.55f}, {"acousticBrightness", 0.5f}, {"material", 3.0f}, {"inharmonicity", 0.35f},
        {"airAbsorption", 0.45f}, {"airAbsorptionTime", 4.5f},
        {"bloomSpeed", 0.88f}, {"bloomAmount", 0.65f}, {"shimmer", 0.48f},
        {"bloomFineEnabled", 1.0f},
        {"bloomSpeedLow", 0.95f}, {"bloomSpeedMid", 0.85f}, {"bloomSpeedHigh", 0.7f},
        {"bloomAmountLow", 0.72f}, {"bloomAmountMid", 0.6f}, {"bloomAmountHigh", 0.42f},
        {"unisonCount", 4.0f}, {"unisonDetune", 18.0f},
        {"octaveBlendSub", 0.25f}, {"octaveBlendOct", 0.38f}, {"stereoSpread", 1.0f},
        {"strikeTime", 70.0f}, {"brilliance", 52.0f}, {"bodyTime", 4500.0f}, {"humSustain", 85.0f},
        {"attackLevel", 0.18f}, {"reverbMix", 0.72f},
        {"lpFilterEnabled", 1.0f}, {"lpFilterCutoff", 10000.0f},
        {"pitchEnvelope", 0.01f}, {"pitchEnvTime", 180.0f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 2.0f},
        {"humanize", 0.48f}
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
