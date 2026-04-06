/*
  ==============================================================================

    InstrumentPresets.h
    O-Wind - Instrument Preset Parameter Sets
    Ouaricon Audio
    Developer: Taylor Brook

    Defines internal DSP coefficients for each instrument type.
    Presets tune the physics model; user APVTS params sit on top.
    These are NOT exposed as APVTS parameters -- they configure the
    internal jet gain, noise spectrum, bore characteristics, etc.

  ==============================================================================
*/

#pragma once
#include <array>

struct InstrumentPreset
{
    const char* name;
    float jetGain;            // tanh saturation gain (1.0-3.0)
    float noiseLevel;         // default noise level (0-1)
    float noiseCutoffBase;    // noise filter base cutoff (Hz)
    float radiationCutoff;    // radiation highpass cutoff (Hz)
    float boreLossCutoff;     // bore loss lowpass base cutoff (Hz)
    float boreLossQ;          // bore loss filter Q
    float endReflCutoff;      // end reflection lowpass cutoff (Hz)
    float embouchureMin;      // embouchure range min (jet ratio)
    float embouchureMax;      // embouchure range max (jet ratio)
    float defaultBreath;      // default breath pressure (0-1)
    float attackTimeMs;       // base attack time in ms
    float jetAmplification;   // mu factor: jet spatial amplification (Verge 1995)
    float overblowEase;       // how easily instrument overblows (0=hard, 1=easy)
    float jetDiameter;        // jet opening diameter in meters (for Strouhal noise)
};

namespace InstrumentPresets
{
    // Core presets (ship at launch)
    //                                    jetGain noise noiseCut radCut boreCut boreQ endRefl embMin embMax breath atk   mu   overblow jetDiam
    static constexpr InstrumentPreset concertFlute {
        "Concert Flute",
        2.8f, 0.15f, 4000.0f, 150.0f, 8000.0f, 0.707f, 3000.0f, 0.35f, 0.55f, 0.5f, 5.0f, 25.0f, 0.6f, 0.010f
    };

    static constexpr InstrumentPreset shakuhachi {
        "Shakuhachi",
        2.1f, 0.35f, 3000.0f, 100.0f, 5000.0f, 0.500f, 2000.0f, 0.30f, 0.60f, 0.4f, 15.0f, 15.0f, 0.3f, 0.015f
    };

    static constexpr InstrumentPreset bansuri {
        "Bansuri",
        2.5f, 0.25f, 3500.0f, 120.0f, 6000.0f, 0.600f, 2500.0f, 0.35f, 0.55f, 0.5f, 10.0f, 20.0f, 0.5f, 0.012f
    };

    static constexpr InstrumentPreset nativeAmericanFlute {
        "Native Am. Flute",
        1.8f, 0.20f, 2500.0f, 90.0f, 4000.0f, 0.450f, 1500.0f, 0.40f, 0.50f, 0.3f, 20.0f, 12.0f, 0.4f, 0.014f
    };

    // Expansion presets (Phase 3.4)
    static constexpr InstrumentPreset recorder {
        "Recorder",
        3.2f, 0.08f, 5000.0f, 200.0f, 10000.0f, 0.707f, 4000.0f, 0.42f, 0.48f, 0.6f, 3.0f, 35.0f, 1.0f, 0.008f
    };

    static constexpr InstrumentPreset panFlute {
        "Pan Flute",
        1.5f, 0.40f, 2000.0f, 175.0f, 6000.0f, 0.500f, 2500.0f, 0.38f, 0.52f, 0.4f, 12.0f, 18.0f, 0.4f, 0.013f
    };

    static constexpr InstrumentPreset piccolo {
        "Piccolo",
        3.0f, 0.12f, 5000.0f, 250.0f, 12000.0f, 0.707f, 5000.0f, 0.32f, 0.50f, 0.5f, 4.0f, 30.0f, 0.7f, 0.007f
    };

    static constexpr InstrumentPreset ocarina {
        "Ocarina",
        2.2f, 0.05f, 3000.0f, 120.0f, 5000.0f, 0.600f, 2000.0f, 0.45f, 0.50f, 0.5f, 8.0f, 18.0f, 0.2f, 0.010f
    };

    // All presets in order
    static constexpr std::array<InstrumentPreset, 8> allPresets {{
        concertFlute,
        shakuhachi,
        bansuri,
        nativeAmericanFlute,
        recorder,
        panFlute,
        piccolo,
        ocarina
    }};

    static constexpr int numCorePresets = 4;
    static constexpr int numTotalPresets = 8;

    inline const InstrumentPreset& getPreset (int index)
    {
        index = (index < 0 || index >= numTotalPresets) ? 0 : index;
        return allPresets[static_cast<size_t> (index)];
    }
}
