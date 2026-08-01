/*
   This file is part of O-MultiBandCompressor, an Ouaricon Audio plugin.
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

    O-MultiBandCompressor - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Parameter layout creation (BEFORE constructor)
juce::AudioProcessorValueTreeState::ParameterLayout OMultiBandCompressorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ===== GLOBAL PARAMETERS (8) =====

    // INPUT_GAIN: -24 to +24 dB, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "INPUT_GAIN", 1 },
        "Input Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // OUTPUT_GAIN: -24 to +24 dB, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OUTPUT_GAIN", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // MIX: 0-100%, default 100%
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    // AUTO_MAKEUP: bool, default false
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "AUTO_MAKEUP", 1 },
        "Auto-Makeup",
        false
    ));

    // MS_MODE: choice (Off/Mid/Side/Both), default Off (0)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MS_MODE", 1 },
        "M/S Mode",
        juce::StringArray { "Off", "Mid", "Side", "Both" },
        0
    ));

    // XOVER1: 20-500 Hz, default 200 Hz (logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "XOVER1", 1 },
        "Crossover 1",
        juce::NormalisableRange<float>(20.0f, 500.0f, 0.1f, 0.3f),
        200.0f,
        "Hz"
    ));

    // XOVER2: 200-5000 Hz, default 2000 Hz (logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "XOVER2", 1 },
        "Crossover 2",
        juce::NormalisableRange<float>(200.0f, 5000.0f, 0.1f, 0.3f),
        2000.0f,
        "Hz"
    ));

    // XOVER3: 2000-16000 Hz, default 8000 Hz (logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "XOVER3", 1 },
        "Crossover 3",
        juce::NormalisableRange<float>(2000.0f, 16000.0f, 0.1f, 0.3f),
        8000.0f,
        "Hz"
    ));

    // ===== PER-BAND PARAMETERS (12 × 4 = 48) =====

    juce::StringArray bandPrefixes = { "LOW", "LOMID", "HIMID", "HIGH" };
    juce::StringArray bandNames = { "Low", "Low-Mid", "High-Mid", "High" };

    for (int i = 0; i < 4; ++i)
    {
        const auto& prefix = bandPrefixes[i];
        const auto& name = bandNames[i];

        // THRESHOLD: -60 to 0 dB, default -20
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_THRESHOLD", 1 },
            name + " Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
            -20.0f,
            "dB"
        ));

        // RATIO: 1 to 20 (1:1 to 20:1), default 4
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_RATIO", 1 },
            name + " Ratio",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
            4.0f,
            ":1"
        ));

        // ATTACK: 0.1 to 200 ms, default 10
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_ATTACK", 1 },
            name + " Attack",
            juce::NormalisableRange<float>(0.1f, 200.0f, 0.1f, 0.3f),
            10.0f,
            "ms"
        ));

        // RELEASE: 10 to 2000 ms, default 100
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_RELEASE", 1 },
            name + " Release",
            juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f, 0.3f),
            100.0f,
            "ms"
        ));

        // KNEE: 0 to 24 dB, default 6
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_KNEE", 1 },
            name + " Knee",
            juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f),
            6.0f,
            "dB"
        ));

        // MAKEUP: -12 to +24 dB, default 0
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_MAKEUP", 1 },
            name + " Makeup",
            juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f),
            0.0f,
            "dB"
        ));

        // PEAK_RMS: 0-100%, default 50%
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_PEAK_RMS", 1 },
            name + " Peak/RMS",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            50.0f,
            "%"
        ));

        // SOLO: bool, default false
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "_SOLO", 1 },
            name + " Solo",
            false
        ));

        // BYPASS: bool, default false
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "_BYPASS", 1 },
            name + " Bypass",
            false
        ));

        // SC_HPF: 20-2000 Hz, default 0 (off)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_SC_HPF", 1 },
            name + " SC HPF",
            juce::NormalisableRange<float>(0.0f, 2000.0f, 0.1f, 0.3f),
            0.0f,
            "Hz"
        ));

        // SC_LPF: 500-20000 Hz, default 0 (off)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_SC_LPF", 1 },
            name + " SC LPF",
            juce::NormalisableRange<float>(0.0f, 20000.0f, 0.1f, 0.3f),
            0.0f,
            "Hz"
        ));

        // SC_LISTEN: bool, default false
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "_SC_LISTEN", 1 },
            name + " SC Listen",
            false
        ));
    }

    return layout;
}

//==============================================================================
// v1.5.0: FACTORY PRESETS
//
// Every value below is written in engineering units — dB, ms, Hz, ratio — and run
// through the owning parameter's own NormalisableRange in buildFactoryPresets().
// Hand-writing the normalised 0-1 fractions that FactoryPresetDef ultimately stores
// would silently ignore the skew of 0.3 carried by ATTACK, RELEASE, XOVER1/2/3 and
// SC_HPF/SC_LPF, and those values would recall 10-30x wrong.
//
// Thresholds follow a band-level staircase. Threshold is measured on the *band*
// signal, not the full-range input, and in a mix balanced to roughly -12 dBFS RMS the
// 8 kHz+ band typically sits 25-30 dB below the low band. A preset using one threshold
// across all four bands would squash the lows and never engage up top, so the values
// step down about -20 / -24 / -30 / -38 and shift from there per application.
//
// A band with `bypassed = true` is still summed into the output, just not compressed
// (Compressor::processStereo returns early), so single-band presets leave the rest of
// the spectrum untouched and the reason is visible on the BYPASS button.
//==============================================================================

namespace
{
    struct BandSpec
    {
        float thresholdDb;   // -60 .. 0
        float ratio;         // 1 .. 20 (1.0 = no gain reduction at any level)
        float attackMs;      // 0.1 .. 200
        float releaseMs;     // 10 .. 2000
        float kneeDb;        // 0 .. 24
        float makeupDb;      // -12 .. +24
        float peakRmsPct;    // 0 = pure peak, 100 = pure RMS (10 ms window)
        bool  bypassed;      // true = band passes through summed but uncompressed
        float scHpfHz;       // 0 = off, else 20 .. 2000
        float scLpfHz;       // 0 = off, else 500 .. 20000
    };

    struct PresetSpec
    {
        const char* name;    // never contains '/' — the manager uses it verbatim as a filename
        float inputGainDb;   // -24 .. +24
        float outputGainDb;  // -24 .. +24
        float mixPct;        // 0 .. 100
        bool  autoMakeup;    // applies 80% of each band's average gain reduction
        int   msMode;        // 0 Off, 1 Mid, 2 Side, 3 Both
        float xover1Hz;      // 20 .. 500
        float xover2Hz;      // 200 .. 5000
        float xover3Hz;      // 2000 .. 16000
        BandSpec bands[4];   // LOW, LOMID, HIMID, HIGH
    };

    const PresetSpec kFactoryPresets[] =
    {
        // --- Neutral starting point ------------------------------------------------
        // Every ratio at 1:1, so the crossover network runs but nothing is compressed.
        { "Init Flat", 0.0f, 0.0f, 100.0f, false, 0, 200.0f, 2000.0f, 8000.0f, {
            { -20.0f, 1.0f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, false,  0.0f,     0.0f },
            { -24.0f, 1.0f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, false,  0.0f,     0.0f },
            { -30.0f, 1.0f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, false,  0.0f,     0.0f },
            { -38.0f, 1.0f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, false,  0.0f,     0.0f } } },

        // --- Mastering / bus -------------------------------------------------------
        // Light ratios and long releases for 1-2 dB of gain reduction per band, the
        // standard "glue" target. RMS-leaning detection and a 12 dB knee keep it from
        // grabbing individual transients.
        { "Mastering Glue", 0.0f, 0.0f, 100.0f, false, 0, 120.0f, 1800.0f, 7000.0f, {
            { -22.0f, 1.8f, 25.0f, 300.0f, 12.0f, 0.5f, 75.0f, false, 25.0f,     0.0f },
            { -24.0f, 1.7f, 20.0f, 250.0f, 12.0f, 0.5f, 75.0f, false,  0.0f,     0.0f },
            { -30.0f, 1.6f, 15.0f, 200.0f, 12.0f, 0.5f, 70.0f, false,  0.0f,     0.0f },
            { -38.0f, 1.5f, 10.0f, 150.0f, 12.0f, 0.5f, 60.0f, false,  0.0f,     0.0f } } },

        // Lighter still: 1.3-1.4:1 with an 18 dB knee, for material that is already
        // balanced and only needs the spectrum held steady.
        { "Transparent Master", 0.0f, 0.0f, 100.0f, false, 0, 150.0f, 2000.0f, 8000.0f, {
            { -20.0f, 1.4f,  30.0f, 400.0f, 18.0f, 0.3f, 85.0f, false, 30.0f,    0.0f },
            { -22.0f, 1.35f, 25.0f, 350.0f, 18.0f, 0.3f, 85.0f, false,  0.0f,    0.0f },
            { -28.0f, 1.3f,  20.0f, 300.0f, 18.0f, 0.3f, 80.0f, false,  0.0f,    0.0f },
            { -36.0f, 1.3f,  15.0f, 250.0f, 18.0f, 0.3f, 75.0f, false,  0.0f,    0.0f } } },

        // Denser: real ratios, faster attacks up top, auto-makeup on so each band gives
        // back 80% of what it takes. Output trimmed +1 dB.
        { "Loud and Punchy", 0.0f, 1.0f, 100.0f, true, 0, 110.0f, 1600.0f, 6500.0f, {
            { -26.0f, 3.0f, 15.0f, 180.0f,  6.0f, 0.0f, 60.0f, false, 30.0f,     0.0f },
            { -26.0f, 2.5f, 12.0f, 160.0f,  6.0f, 0.0f, 60.0f, false,  0.0f,     0.0f },
            { -32.0f, 2.5f,  5.0f, 120.0f,  4.0f, 0.0f, 40.0f, false,  0.0f,     0.0f },
            { -40.0f, 2.2f,  2.0f,  90.0f,  4.0f, 0.0f, 30.0f, false,  0.0f,     0.0f } } },

        // Slow low-band attack lets the bass bloom before it is caught; the high band
        // uses a stronger ratio and slight cut to round the top the way tape does.
        { "Warm Tape Glue", 0.0f, 0.0f, 100.0f, false, 0, 130.0f, 1500.0f, 6000.0f, {
            { -24.0f, 2.2f, 40.0f, 350.0f, 14.0f,  1.0f, 85.0f, false, 25.0f,    0.0f },
            { -26.0f, 2.0f, 35.0f, 300.0f, 14.0f,  0.5f, 85.0f, false,  0.0f,    0.0f },
            { -30.0f, 1.8f, 25.0f, 250.0f, 14.0f,  0.0f, 80.0f, false,  0.0f,    0.0f },
            { -36.0f, 2.5f,  8.0f, 180.0f, 10.0f, -0.5f, 70.0f, false,  0.0f,    0.0f } } },

        // --- Corrective ------------------------------------------------------------
        // Low band only, crossed at 120 Hz. The sidechain high-pass at 30 Hz keeps
        // inaudible subsonic energy from triggering gain reduction.
        { "Low End Control", 0.0f, 0.0f, 100.0f, false, 0, 120.0f, 2000.0f, 8000.0f, {
            { -24.0f, 4.0f, 15.0f, 180.0f,  4.0f, 1.5f, 40.0f, false, 30.0f,     0.0f },
            { -24.0f, 1.5f, 20.0f, 200.0f,  8.0f, 0.0f, 60.0f, true,   0.0f,     0.0f },
            { -30.0f, 1.5f, 15.0f, 150.0f,  8.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -38.0f, 1.5f, 10.0f, 120.0f,  8.0f, 0.0f, 50.0f, true,   0.0f,     0.0f } } },

        // Crossovers pulled to 180 Hz and 600 Hz so the low-mid band sits exactly on the
        // boxy region where guitars, bass and low vocals pile up.
        { "Mud Tamer", 0.0f, 0.0f, 100.0f, false, 0, 180.0f, 600.0f, 8000.0f, {
            { -22.0f, 1.5f, 20.0f, 200.0f,  8.0f, 0.0f, 60.0f, true,   0.0f,     0.0f },
            { -26.0f, 3.5f, 12.0f, 200.0f,  6.0f, 0.0f, 60.0f, false,  0.0f,     0.0f },
            { -28.0f, 1.5f, 15.0f, 150.0f,  8.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -38.0f, 1.5f, 10.0f, 120.0f,  8.0f, 0.0f, 50.0f, true,   0.0f,     0.0f } } },

        // High band crossed at 6.5 kHz, hard knee, near-instant attack and a 40 ms
        // release so only the sibilant itself ducks. Pure peak detection — an RMS
        // window would average the "s" away. The sidechain low-pass at 12 kHz stops
        // cymbals and air from holding the de-esser down.
        { "De-Esser Vocal", 0.0f, 0.0f, 100.0f, false, 0, 200.0f, 2000.0f, 6500.0f, {
            { -20.0f, 1.5f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -24.0f, 1.5f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -30.0f, 1.5f,  5.0f,  90.0f,  6.0f, 0.0f, 40.0f, true,   0.0f,     0.0f },
            { -34.0f, 6.0f,  0.3f,  40.0f,  2.0f, 0.0f,  0.0f, false,  0.0f, 12000.0f } } },

        // The 2.5-7 kHz band is where cheap converters, bright amps and over-EQ'd
        // vocals get abrasive. 4:1 with a 3 ms attack catches the peaks without dulling.
        { "Harshness Tamer", 0.0f, 0.0f, 100.0f, false, 0, 200.0f, 2500.0f, 7000.0f, {
            { -20.0f, 1.5f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -24.0f, 1.5f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -30.0f, 4.0f,  3.0f,  80.0f,  4.0f, 0.0f, 20.0f, false,  0.0f,     0.0f },
            { -38.0f, 1.5f, 10.0f, 120.0f,  8.0f, 0.0f, 50.0f, true,   0.0f,     0.0f } } },

        // v1.6.0. Sizzle above 7 kHz — overheads, hats and ride wash. Peak detection
        // and a 1 ms attack so it rides the individual crashes rather than the bed.
        { "Cymbal Sizzle Control", 0.0f, 0.0f, 100.0f, false, 0, 200.0f, 2000.0f, 7000.0f, {
            { -20.0f, 1.5f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -24.0f, 1.5f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -30.0f, 1.5f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -34.0f, 4.0f,  1.0f,  60.0f,  3.0f, 0.0f, 10.0f, false,  0.0f,     0.0f } } },

        // v1.6.0. Crossovers at 800 Hz and 2 kHz put the high-mid band exactly on the
        // nasal/honky region — boxy snares, quacky guitars, midrange-forward vocals.
        // Slower than the other harshness presets: honk is a sustained resonance, not
        // a transient, so a fast attack would just chatter on it.
        { "Nasal Honk Control", 0.0f, 0.0f, 100.0f, false, 0, 200.0f, 800.0f, 2000.0f, {
            { -20.0f, 1.5f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -24.0f, 1.5f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -26.0f, 3.5f,  8.0f, 120.0f,  5.0f, 0.0f, 45.0f, false,  0.0f,     0.0f },
            { -38.0f, 1.5f, 10.0f, 120.0f,  8.0f, 0.0f, 50.0f, true,   0.0f,     0.0f } } },

        // v1.6.0. Amp-sim and distortion fizz sits from about 3.5 kHz up. The sidechain
        // low-pass at 10 kHz keeps genuine air out of the detector, so the band responds
        // to the fizz itself rather than being held down by everything above it.
        { "Amp Fizz Control", 0.0f, 0.0f, 100.0f, false, 0, 200.0f, 2000.0f, 3500.0f, {
            { -20.0f, 1.5f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -24.0f, 1.5f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -30.0f, 1.5f, 10.0f, 100.0f,  6.0f, 0.0f, 50.0f, true,   0.0f,     0.0f },
            { -32.0f, 4.0f,  2.0f,  70.0f,  3.0f, 0.0f, 15.0f, false,  0.0f, 10000.0f } } },

        // --- Instrument / bus ------------------------------------------------------
        // Four jobs at once: fast low band for plosives and mic rumble, gentle RMS band
        // for chest-voice level, presence control through the mids, de-esser on top.
        // The low crossover sits at 120 Hz so a male fundamental (roughly 85-180 Hz)
        // lands mostly in the RMS body band rather than under the fast plosive band —
        // at 150 Hz with a -30 dB threshold the harness measured 21 dB of gain
        // reduction on the low band, which thins the voice instead of de-popping it.
        { "Vocal Bus", 0.0f, 0.0f, 100.0f, false, 0, 120.0f, 1200.0f, 6500.0f, {
            { -26.0f, 4.0f,  3.0f, 120.0f,  4.0f, 0.0f, 20.0f, false,  0.0f,     0.0f },
            { -24.0f, 2.5f, 15.0f, 150.0f,  8.0f, 0.5f, 70.0f, false,  0.0f,     0.0f },
            { -28.0f, 3.0f,  5.0f,  90.0f,  6.0f, 0.0f, 30.0f, false,  0.0f,     0.0f },
            { -34.0f, 5.0f,  0.5f,  45.0f,  2.0f, 0.0f,  0.0f, false,  0.0f, 12000.0f } } },

        // The 30 ms low-band attack is the point: the kick transient passes untouched
        // and only the tail after it is compressed, which is what reads as punch. The
        // high-mid band tightens the snare crack, the high band holds cymbals down.
        { "Drum Bus Punch", 0.0f, 0.0f, 100.0f, false, 0, 120.0f, 1500.0f, 6000.0f, {
            { -20.0f, 3.0f, 30.0f, 120.0f,  4.0f, 1.0f, 30.0f, false, 25.0f,     0.0f },
            { -24.0f, 2.5f, 20.0f, 140.0f,  6.0f, 0.0f, 50.0f, false,  0.0f,     0.0f },
            { -28.0f, 3.0f,  8.0f, 100.0f,  4.0f, 0.5f, 25.0f, false,  0.0f,     0.0f },
            { -34.0f, 2.5f,  3.0f,  70.0f,  4.0f, 0.0f, 20.0f, false,  0.0f,     0.0f } } },

        // Crossed at 100 Hz and 800 Hz to split fundamental from growl. 5:1 on the
        // fundamental evens out note-to-note level; the sidechain high-pass keeps
        // subsonic energy out of the detector.
        { "Bass Tighten", 0.0f, 0.0f, 100.0f, false, 0, 100.0f, 800.0f, 4000.0f, {
            { -18.0f, 5.0f,  8.0f, 150.0f,  4.0f, 1.5f, 60.0f, false, 30.0f,     0.0f },
            { -22.0f, 3.0f, 12.0f, 180.0f,  6.0f, 0.5f, 55.0f, false,  0.0f,     0.0f },
            { -30.0f, 2.0f,  5.0f, 100.0f,  6.0f, 0.0f, 40.0f, false,  0.0f,     0.0f },
            { -38.0f, 1.5f, 10.0f, 120.0f,  8.0f, 0.0f, 50.0f, true,   0.0f,     0.0f } } },

        // Boom control below 180 Hz plus a fast high-mid band on the pick and fret
        // noise, leaving the body of the instrument mostly alone.
        // The low band is boom control, not a gate on the body of the instrument —
        // the low E is 82 Hz and the body resonance sits near 100 Hz, both inside this
        // band, so 4:1 at -26 dB (12 dB of reduction in the harness) hollowed it out.
        { "Acoustic Guitar", 0.0f, 0.0f, 100.0f, false, 0, 180.0f, 1500.0f, 7000.0f, {
            { -22.0f, 3.0f, 10.0f, 150.0f,  4.0f, 0.0f, 40.0f, false, 30.0f,     0.0f },
            { -26.0f, 2.0f, 20.0f, 180.0f,  8.0f, 0.5f, 70.0f, false,  0.0f,     0.0f },
            { -30.0f, 2.5f,  4.0f,  90.0f,  6.0f, 0.0f, 25.0f, false,  0.0f,     0.0f },
            { -36.0f, 2.0f,  2.0f,  60.0f,  4.0f, 0.0f, 20.0f, false,  0.0f,     0.0f } } },

        // Speech: heavy RMS levelling through the body band, a fast low band for
        // plosives and desk rumble, de-esser on top, auto-makeup and +2 dB out so the
        // result lands close to broadcast level without further gain staging.
        // Low crossover at 120 Hz for the same reason as Vocal Bus: the fast low band
        // is there to catch plosives and desk rumble, not to compress the voice itself.
        { "Podcast Voice", 0.0f, 2.0f, 100.0f, true, 0, 120.0f, 1200.0f, 6000.0f, {
            { -26.0f, 4.0f,  2.0f, 100.0f,  2.0f, 0.0f, 10.0f, false, 30.0f,     0.0f },
            { -28.0f, 3.5f, 12.0f, 140.0f,  8.0f, 0.0f, 80.0f, false,  0.0f,     0.0f },
            { -30.0f, 3.0f,  6.0f,  90.0f,  6.0f, 0.0f, 40.0f, false,  0.0f,     0.0f },
            { -34.0f, 5.0f,  0.5f,  40.0f,  2.0f, 0.0f,  0.0f, false,  0.0f, 11000.0f } } },

        // v1.6.0. Crossed at 150 / 900 / 4000: rumble, body, pick and mids, then fizz.
        // The high band is trimmed slightly negative because amp sims and bright
        // single-coils rarely need more top once the fizz is under control.
        { "Electric Guitar", 0.0f, 0.0f, 100.0f, false, 0, 150.0f, 900.0f, 4000.0f, {
            { -26.0f, 3.5f, 12.0f, 150.0f,  4.0f,  0.0f, 40.0f, false, 30.0f,     0.0f },
            { -24.0f, 2.5f, 18.0f, 180.0f,  8.0f,  0.5f, 65.0f, false,  0.0f,     0.0f },
            { -28.0f, 3.0f,  5.0f, 100.0f,  6.0f,  0.0f, 30.0f, false,  0.0f,     0.0f },
            { -34.0f, 3.0f,  2.0f,  70.0f,  4.0f, -0.5f, 20.0f, false,  0.0f, 12000.0f } } },

        // v1.6.0. Piano has the widest dynamic range of anything here, so every band is
        // deliberately gentle with a wide knee and RMS-leaning detection — the goal is
        // to stop the low register swamping the mix, not to level the performance.
        { "Piano", 0.0f, 0.0f, 100.0f, false, 0, 160.0f, 1200.0f, 5000.0f, {
            { -22.0f, 2.0f, 25.0f, 250.0f, 10.0f, 0.0f, 70.0f, false, 25.0f,     0.0f },
            { -24.0f, 1.8f, 20.0f, 220.0f, 10.0f, 0.0f, 75.0f, false,  0.0f,     0.0f },
            { -28.0f, 1.8f, 12.0f, 160.0f,  8.0f, 0.0f, 55.0f, false,  0.0f,     0.0f },
            { -36.0f, 2.0f,  5.0f, 110.0f,  6.0f, 0.0f, 40.0f, false,  0.0f,     0.0f } } },

        // v1.6.0. The slowest preset in the set: 40-60 ms attacks and 300-500 ms
        // releases, heavily RMS. A fast attack on strings clamps the bow articulation
        // and turns a section into a pad, so every band is set to ride the swell
        // underneath the attack rather than catch it.
        { "Strings Ensemble", 0.0f, 0.0f, 100.0f, false, 0, 200.0f, 1500.0f, 6000.0f, {
            { -24.0f, 1.6f, 60.0f, 500.0f, 14.0f, 0.0f, 85.0f, false, 30.0f,     0.0f },
            { -26.0f, 1.6f, 50.0f, 450.0f, 14.0f, 0.0f, 85.0f, false,  0.0f,     0.0f },
            { -30.0f, 1.5f, 40.0f, 400.0f, 14.0f, 0.0f, 80.0f, false,  0.0f,     0.0f },
            { -38.0f, 1.5f, 25.0f, 300.0f, 12.0f, 0.0f, 75.0f, false,  0.0f,     0.0f } } },

        // v1.6.0. Brass blare concentrates between 1 and 4 kHz and rises sharply with
        // playing intensity, so the high-mid band is the aggressive one (4:1, 4 ms)
        // while the rest stays moderate.
        { "Brass Section", 0.0f, 0.0f, 100.0f, false, 0, 200.0f, 1000.0f, 4000.0f, {
            { -22.0f, 2.0f, 25.0f, 200.0f,  8.0f, 0.0f, 60.0f, false, 30.0f,     0.0f },
            { -24.0f, 2.2f, 15.0f, 180.0f,  8.0f, 0.0f, 60.0f, false,  0.0f,     0.0f },
            { -28.0f, 4.0f,  4.0f,  90.0f,  4.0f, 0.0f, 25.0f, false,  0.0f,     0.0f },
            { -36.0f, 2.5f,  3.0f,  80.0f,  4.0f, 0.0f, 30.0f, false,  0.0f,     0.0f } } },

        // v1.6.0. Marimba, vibes, glockenspiel, tuned percussion. The 25-35 ms attacks
        // exist to let the mallet strike through untouched; what is being controlled is
        // the bloom and ring after it, which is why the releases are comparatively long.
        { "Mallet Percussion", 0.0f, 0.0f, 100.0f, false, 0, 250.0f, 2000.0f, 7000.0f, {
            { -22.0f, 2.5f, 35.0f, 200.0f,  6.0f, 0.5f, 45.0f, false, 30.0f,     0.0f },
            { -24.0f, 2.5f, 30.0f, 220.0f,  6.0f, 0.0f, 50.0f, false,  0.0f,     0.0f },
            { -30.0f, 2.2f, 25.0f, 180.0f,  6.0f, 0.0f, 40.0f, false,  0.0f,     0.0f },
            { -38.0f, 2.0f, 15.0f, 140.0f,  6.0f, 0.0f, 35.0f, false,  0.0f,     0.0f } } },

        // v1.6.0. Flute, clarinet, oboe, recorder. The body bands are gentle and RMS-led;
        // the work happens in the 5 kHz+ band, where breath and key noise live. Peak
        // detection with a 3 ms attack ducks the breath rather than the note, and the
        // sidechain low-pass keeps room air out of that decision.
        { "Woodwind Breath", 0.0f, 0.0f, 100.0f, false, 0, 200.0f, 1600.0f, 5000.0f, {
            { -24.0f, 2.0f, 30.0f, 250.0f, 10.0f,  0.0f, 75.0f, false, 30.0f,     0.0f },
            { -26.0f, 2.0f, 25.0f, 220.0f, 10.0f,  0.0f, 80.0f, false,  0.0f,     0.0f },
            { -30.0f, 2.0f, 15.0f, 150.0f,  8.0f,  0.0f, 55.0f, false,  0.0f,     0.0f },
            { -36.0f, 4.0f,  3.0f,  80.0f,  4.0f, -0.5f, 20.0f, false,  0.0f, 12000.0f } } },

        // --- Effect ----------------------------------------------------------------
        // Deliberately destructive, then blended back at 35% via Mix. Low thresholds
        // and 12-15:1 ratios mean every band is compressing almost all the time, which
        // is the point — the dry path keeps the transients.
        { "Parallel Crush", 0.0f, 0.0f, 35.0f, false, 0, 150.0f, 1800.0f, 7000.0f, {
            { -40.0f, 12.0f, 1.0f, 100.0f,  0.0f, 3.0f, 30.0f, false, 30.0f,     0.0f },
            { -40.0f, 14.0f, 0.5f,  90.0f,  0.0f, 3.0f, 25.0f, false,  0.0f,     0.0f },
            { -44.0f, 15.0f, 0.5f,  70.0f,  0.0f, 3.5f, 20.0f, false,  0.0f,     0.0f },
            { -48.0f, 12.0f, 0.3f,  60.0f,  0.0f, 3.5f, 15.0f, false,  0.0f,     0.0f } } },

        // M/S Side mode: the mid channel passes through untouched and only the stereo
        // difference signal is compressed. The 4:1 low band pulls wandering stereo bass
        // toward the centre; the upper bands hold the width steady.
        { "Wide and Controlled", 0.0f, 0.0f, 100.0f, false, 2, 150.0f, 2000.0f, 8000.0f, {
            { -30.0f, 4.0f, 12.0f, 200.0f,  6.0f, 0.0f, 50.0f, false, 30.0f,     0.0f },
            { -28.0f, 2.0f, 20.0f, 250.0f, 10.0f, 0.0f, 70.0f, false,  0.0f,     0.0f },
            { -32.0f, 1.8f, 15.0f, 200.0f, 10.0f, 0.0f, 65.0f, false,  0.0f,     0.0f },
            { -38.0f, 1.6f, 10.0f, 150.0f, 10.0f, 0.0f, 60.0f, false,  0.0f,     0.0f } } },
    };

    // Converts one engineering value through the parameter's own NormalisableRange.
    // jassertfalse on a missing ID catches a typo in the table at first construction
    // rather than letting the entry silently vanish from the preset.
    void addParam(std::map<juce::String, float>& dest,
                  const juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramID,
                  float engineeringValue)
    {
        if (auto* param = apvts.getParameter(paramID))
            dest[paramID] = param->convertTo0to1(engineeringValue);
        else
            jassertfalse;
    }
}

std::vector<OuariconPresetManager::FactoryPresetDef>
OMultiBandCompressorAudioProcessor::buildFactoryPresets() const
{
    static const char* const bandPrefixes[4] = { "LOW", "LOMID", "HIMID", "HIGH" };

    std::vector<OuariconPresetManager::FactoryPresetDef> defs;
    defs.reserve(std::size(kFactoryPresets));

    for (const auto& spec : kFactoryPresets)
    {
        std::map<juce::String, float> values;

        addParam(values, parameters, "INPUT_GAIN",  spec.inputGainDb);
        addParam(values, parameters, "OUTPUT_GAIN", spec.outputGainDb);
        addParam(values, parameters, "MIX",         spec.mixPct);
        addParam(values, parameters, "AUTO_MAKEUP", spec.autoMakeup ? 1.0f : 0.0f);
        addParam(values, parameters, "MS_MODE",     static_cast<float>(spec.msMode));
        addParam(values, parameters, "XOVER1",      spec.xover1Hz);
        addParam(values, parameters, "XOVER2",      spec.xover2Hz);
        addParam(values, parameters, "XOVER3",      spec.xover3Hz);

        for (int band = 0; band < 4; ++band)
        {
            const juce::String prefix { bandPrefixes[band] };
            const auto& b = spec.bands[band];

            addParam(values, parameters, prefix + "_THRESHOLD", b.thresholdDb);
            addParam(values, parameters, prefix + "_RATIO",     b.ratio);
            addParam(values, parameters, prefix + "_ATTACK",    b.attackMs);
            addParam(values, parameters, prefix + "_RELEASE",   b.releaseMs);
            addParam(values, parameters, prefix + "_KNEE",      b.kneeDb);
            addParam(values, parameters, prefix + "_MAKEUP",    b.makeupDb);
            addParam(values, parameters, prefix + "_PEAK_RMS",  b.peakRmsPct);
            addParam(values, parameters, prefix + "_BYPASS",    b.bypassed ? 1.0f : 0.0f);
            addParam(values, parameters, prefix + "_SC_HPF",    b.scHpfHz);
            addParam(values, parameters, prefix + "_SC_LPF",    b.scLpfHz);

            // Solo and SC Listen are audition states, not part of a sound. A preset
            // that shipped either one engaged would mute or replace the mix on load.
            addParam(values, parameters, prefix + "_SOLO",      0.0f);
            addParam(values, parameters, prefix + "_SC_LISTEN", 0.0f);
        }

        defs.push_back({ juce::String(spec.name), std::move(values), juce::var() });
    }

    return defs;
}

OMultiBandCompressorAudioProcessor::OMultiBandCompressorAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-MultiBandCompressor")
{
    // Writes the Factory/*.json files only when the plugin version changes — the
    // manager guards this with a version-stamped sentinel, so repeated construction
    // (auval scans, every new instance) does not touch the disk.
    presetManager.initializeFactoryPresets(buildFactoryPresets());
}

OMultiBandCompressorAudioProcessor::~OMultiBandCompressorAudioProcessor()
{
}

void OMultiBandCompressorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Prepare multiband processor (crossover + 4 compressors)
    multibandProcessor.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    // Prepare gain stages
    inputGain.prepare(spec);
    outputGain.prepare(spec);

    // IN-03: ~20 ms ramp so Input/Output Gain automation doesn't zipper.
    inputGain.setRampDurationSeconds(0.02);
    outputGain.setRampDurationSeconds(0.02);

    // Prepare dry/wet mixer
    dryWetMixer.prepare(spec);
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::linear);

    // CR-03: preallocate the mono M/S scratch buffer to the maximum block size so Mid/Side
    // modes never allocate on the audio thread.
    msScratchBuffer.setSize(1, samplesPerBlock, false, false, false);
    msScratchBuffer.clear();

    // CR-02: resolve all parameter pointers once (no per-block string building / map lookups).
    cacheParameterPointers();

    // IN-06: precompute log-spaced FFT bin edges (20 Hz – 20 kHz) so the spectrum's 64 UI
    // bins line up with the log frequency axis of the crossover overlay.
    {
        constexpr float minFreq = 20.0f;
        constexpr float maxFreq = 20000.0f;

        for (int k = 0; k <= SPECTRUM_BINS; ++k)
        {
            const float freq = minFreq * std::pow(maxFreq / minFreq,
                                                  static_cast<float>(k) / static_cast<float>(SPECTRUM_BINS));
            const int bin = static_cast<int>(std::lround(freq * static_cast<float>(FFT_SIZE)
                                                         / static_cast<float>(sampleRate)));
            spectrumBinEdges[static_cast<size_t>(k)] = juce::jlimit(1, FFT_SIZE / 2, bin);
        }
    }

    // Reset components to initial state
    multibandProcessor.reset();
    inputGain.reset();
    outputGain.reset();
    dryWetMixer.reset();
}

void OMultiBandCompressorAudioProcessor::cacheParameterPointers()
{
    static const char* const prefixes[4] = { "LOW", "LOMID", "HIMID", "HIGH" };
    static const char* const suffixes[numBandParams] = {
        "_THRESHOLD", "_RATIO", "_ATTACK", "_RELEASE", "_KNEE", "_MAKEUP",
        "_PEAK_RMS", "_BYPASS", "_SOLO", "_SC_HPF", "_SC_LPF", "_SC_LISTEN"
    };

    for (int band = 0; band < 4; ++band)
        for (int kind = 0; kind < numBandParams; ++kind)
            bandParamPtrs[band][kind] =
                parameters.getRawParameterValue(juce::String(prefixes[band]) + suffixes[kind]);

    pInputGain  = parameters.getRawParameterValue("INPUT_GAIN");
    pOutputGain = parameters.getRawParameterValue("OUTPUT_GAIN");
    pMix        = parameters.getRawParameterValue("MIX");
    pAutoMakeup = parameters.getRawParameterValue("AUTO_MAKEUP");
    pMsMode     = parameters.getRawParameterValue("MS_MODE");
    pXover1     = parameters.getRawParameterValue("XOVER1");
    pXover2     = parameters.getRawParameterValue("XOVER2");
    pXover3     = parameters.getRawParameterValue("XOVER3");
}

void OMultiBandCompressorAudioProcessor::releaseResources()
{
    // Release resources when plugin not in use
}

void OMultiBandCompressorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Early exit on empty buffer
    if (buffer.getNumSamples() == 0)
        return;

    // ===== Phase 5.3: Measure Input Levels =====
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels >= 1)
    {
        float inputMaxL = buffer.getMagnitude(0, 0, numSamples);
        inputLevelL.store(inputMaxL, std::memory_order_relaxed);
    }

    if (numChannels >= 2)
    {
        float inputMaxR = buffer.getMagnitude(1, 0, numSamples);
        inputLevelR.store(inputMaxR, std::memory_order_relaxed);
    }

    // ===== Read Parameters (atomic, real-time safe) =====
    // CR-02: all pointers were resolved once in prepareToPlay — no string building or map
    // lookups here.

    // Global parameters
    float inputGainDB = pInputGain->load();
    float outputGainDB = pOutputGain->load();
    float mixPercent = pMix->load();
    bool autoMakeupEnabled = pAutoMakeup->load() > 0.5f;
    int msMode = static_cast<int>(pMsMode->load());

    // Crossover frequencies
    float xover1 = pXover1->load();
    float xover2 = pXover2->load();
    float xover3 = pXover3->load();

    // Per-band parameters (arrays indexed by band: 0=LOW, 1=LOMID, 2=HIMID, 3=HIGH)
    float thresholds[4];
    float ratios[4];
    float attacks[4];
    float releases[4];
    float knees[4];
    float makeups[4];
    float peakRmsBlends[4];
    bool bypasses[4];
    bool solos[4];
    float scHPFs[4];
    float scLPFs[4];
    bool scListens[4];

    for (int band = 0; band < 4; ++band)
    {
        thresholds[band]    = bandParamPtrs[band][bpThreshold]->load();
        ratios[band]        = bandParamPtrs[band][bpRatio]->load();
        attacks[band]       = bandParamPtrs[band][bpAttack]->load();
        releases[band]      = bandParamPtrs[band][bpRelease]->load();
        knees[band]         = bandParamPtrs[band][bpKnee]->load();
        makeups[band]       = bandParamPtrs[band][bpMakeup]->load();
        peakRmsBlends[band] = bandParamPtrs[band][bpPeakRms]->load();
        bypasses[band]      = bandParamPtrs[band][bpBypass]->load() > 0.5f;
        solos[band]         = bandParamPtrs[band][bpSolo]->load() > 0.5f;
        scHPFs[band]        = bandParamPtrs[band][bpScHPF]->load();
        scLPFs[band]        = bandParamPtrs[band][bpScLPF]->load();
        scListens[band]     = bandParamPtrs[band][bpScListen]->load() > 0.5f;
    }

    // ===== Process Audio =====

    // Apply input gain
    inputGain.setGainDecibels(inputGainDB);
    juce::dsp::AudioBlock<float> inputBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> inputContext(inputBlock);
    inputGain.process(inputContext);

    // Set dry/wet mix ratio (0-100% to 0.0-1.0)
    dryWetMixer.setWetMixProportion(mixPercent / 100.0f);

    // Capture dry signal for parallel compression
    juce::dsp::AudioBlock<float> dryWetBlock(buffer);
    dryWetMixer.pushDrySamples(dryWetBlock);

    // M/S Encoding (if enabled)

    if (msMode > 0 && numChannels == 2)
    {
        // Encode L/R to M/S (power-preserving)
        const float sqrtHalf = 0.70710678f; // 1/sqrt(2)

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float left = buffer.getSample(0, sample);
            float right = buffer.getSample(1, sample);

            float mid = (left + right) * sqrtHalf;
            float side = (left - right) * sqrtHalf;

            buffer.setSample(0, sample, mid);
            buffer.setSample(1, sample, side);
        }
    }

    // Update crossover frequencies
    multibandProcessor.updateCrossoverFrequencies(xover1, xover2, xover3);

    // Update compressor attack/release times
    multibandProcessor.setAttackTimes(attacks);
    multibandProcessor.setReleaseTimes(releases);

    // Array of gain reduction meters
    std::atomic<float>* grMeters[4] = {
        &lowBandGainReduction,
        &loMidBandGainReduction,
        &hiMidBandGainReduction,
        &highBandGainReduction
    };

    // Process based on M/S mode
    if (msMode == 0 || numChannels != 2)
    {
        // Mode 0: Off - Process L/R independently (standard stereo)
        multibandProcessor.processMultiband(
            buffer,
            thresholds,
            ratios,
            knees,
            makeups,
            peakRmsBlends,
            bypasses,
            solos,
            scHPFs,
            scLPFs,
            scListens,
            autoMakeupEnabled,
            grMeters
        );
    }
    else if (msMode == 1)
    {
        // Mode 1: Mid - Compress only mid (channel 0), side passes through.
        // CR-03: reuse the preallocated scratch buffer (avoidReallocating — no RT alloc).
        msScratchBuffer.setSize(1, numSamples, false, false, true);
        msScratchBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);

        multibandProcessor.processMultiband(
            msScratchBuffer,
            thresholds,
            ratios,
            knees,
            makeups,
            peakRmsBlends,
            bypasses,
            solos,
            scHPFs,
            scLPFs,
            scListens,
            autoMakeupEnabled,
            grMeters
        );

        buffer.copyFrom(0, 0, msScratchBuffer, 0, 0, numSamples);
    }
    else if (msMode == 2)
    {
        // Mode 2: Side - Compress only side (channel 1), mid passes through.
        // CR-03: reuse the preallocated scratch buffer (avoidReallocating — no RT alloc).
        msScratchBuffer.setSize(1, numSamples, false, false, true);
        msScratchBuffer.copyFrom(0, 0, buffer, 1, 0, numSamples);

        multibandProcessor.processMultiband(
            msScratchBuffer,
            thresholds,
            ratios,
            knees,
            makeups,
            peakRmsBlends,
            bypasses,
            solos,
            scHPFs,
            scLPFs,
            scListens,
            autoMakeupEnabled,
            grMeters
        );

        buffer.copyFrom(1, 0, msScratchBuffer, 0, 0, numSamples);
    }
    else if (msMode == 3)
    {
        // Mode 3: Both - Independent compression for mid AND side
        multibandProcessor.processMultiband(
            buffer,
            thresholds,
            ratios,
            knees,
            makeups,
            peakRmsBlends,
            bypasses,
            solos,
            scHPFs,
            scLPFs,
            scListens,
            autoMakeupEnabled,
            grMeters
        );
    }

    // M/S Decoding (if enabled)
    if (msMode > 0 && numChannels == 2)
    {
        // Decode M/S back to L/R (power-preserving)
        const float sqrtHalf = 0.70710678f; // 1/sqrt(2)

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mid = buffer.getSample(0, sample);
            float side = buffer.getSample(1, sample);

            float left = (mid + side) * sqrtHalf;
            float right = (mid - side) * sqrtHalf;

            buffer.setSample(0, sample, left);
            buffer.setSample(1, sample, right);
        }
    }

    // Mix wet (processed) with dry signal
    dryWetMixer.mixWetSamples(dryWetBlock);

    // Apply output gain
    outputGain.setGainDecibels(outputGainDB);
    juce::dsp::AudioBlock<float> outputBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> outputContext(outputBlock);
    outputGain.process(outputContext);

    // ===== Phase 5.3: Measure Output Levels =====
    if (numChannels >= 1)
    {
        float outputMaxL = buffer.getMagnitude(0, 0, numSamples);
        outputLevelL.store(outputMaxL, std::memory_order_relaxed);
    }

    if (numChannels >= 2)
    {
        float outputMaxR = buffer.getMagnitude(1, 0, numSamples);
        outputLevelR.store(outputMaxR, std::memory_order_relaxed);
    }

    // ===== v1.2.0: FFT Spectrum Analysis =====
    // Accumulate samples for FFT (mono sum of output)
    const float* leftOut = buffer.getReadPointer(0);
    const float* rightOut = numChannels >= 2 ? buffer.getReadPointer(1) : leftOut;

    for (int i = 0; i < numSamples; ++i)
    {
        // Mono sum
        float sample = (leftOut[i] + rightOut[i]) * 0.5f;
        fftInputFifo[static_cast<size_t>(fftFifoWriteIndex)] = sample;
        ++fftFifoWriteIndex;

        // When FIFO is full, compute FFT
        if (fftFifoWriteIndex >= FFT_SIZE)
        {
            fftFifoWriteIndex = 0;

            // Copy to work buffer and apply window
            std::copy(fftInputFifo.begin(), fftInputFifo.end(), fftWorkBuffer.begin());
            fftWindow.multiplyWithWindowingTable(fftWorkBuffer.data(), FFT_SIZE);

            // Perform FFT
            fft.performFrequencyOnlyForwardTransform(fftWorkBuffer.data());

            // Downsample to SPECTRUM_BINS and convert to normalized dB.
            // IN-06: bin groups are log-spaced (edges precomputed in prepareToPlay) so the
            // UI's linear bin→X draw lands on the log frequency axis of the grid/overlay.
            // Peak (not average) within each group so narrowband energy isn't smeared.
            constexpr float minDb = -80.0f;
            constexpr float maxDb = 0.0f;

            std::array<float, SPECTRUM_BINS> tempSpectrum {};

            for (int bin = 0; bin < SPECTRUM_BINS; ++bin)
            {
                const int startIdx = spectrumBinEdges[static_cast<size_t>(bin)];
                const int endIdx = std::max(spectrumBinEdges[static_cast<size_t>(bin + 1)],
                                            startIdx + 1);

                float peakMag = 0.0f;
                for (int j = startIdx; j < endIdx; ++j)
                    peakMag = std::max(peakMag, fftWorkBuffer[static_cast<size_t>(j)]);

                float db = peakMag > 0.0f
                    ? juce::jlimit(minDb, maxDb, juce::Decibels::gainToDecibels(peakMag))
                    : minDb;

                // Normalize to 0-1
                tempSpectrum[static_cast<size_t>(bin)] = (db - minDb) / (maxDb - minDb);
            }

            // WR-01: publish lock-free. Fill the writer-private slot, then atomically hand it
            // off to the "ready" slot (receiving a free slot to write next time). No lock, no
            // wait — the audio thread cannot be blocked by the UI thread.
            spectrumBuffers[static_cast<size_t>(spectrumWriteSlot)] = tempSpectrum;
            spectrumWriteSlot = spectrumReadySlot.exchange(spectrumWriteSlot, std::memory_order_acq_rel);
            spectrumDataReady.store(true, std::memory_order_release);
        }
    }
}

void OMultiBandCompressorAudioProcessor::getSpectrumData(std::array<float, SPECTRUM_BINS>& dest) const
{
    // WR-01: atomically claim the most-recently published slot (handing back the slot we
    // previously held). Gated by hasNewSpectrumData()/clearSpectrumDataFlag() in the editor,
    // so it reads at most once per published frame.
    spectrumReadSlot = spectrumReadySlot.exchange(spectrumReadSlot, std::memory_order_acq_rel);
    dest = spectrumBuffers[static_cast<size_t>(spectrumReadSlot)];
}

juce::AudioProcessorEditor* OMultiBandCompressorAudioProcessor::createEditor()
{
    return new OMultiBandCompressorAudioProcessorEditor(*this);
}

// v1.5.0: routed through the preset manager so the session also remembers which
// preset is loaded. The XML is still the APVTS state under the same tag with one
// extra "currentPreset" attribute, so sessions saved by v1.4.x and earlier restore
// unchanged — the attribute is simply absent and the name falls back to "Default".
void OMultiBandCompressorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void OMultiBandCompressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
        presetManager.setStateFromXml(xmlState.get());
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OMultiBandCompressorAudioProcessor();
}
