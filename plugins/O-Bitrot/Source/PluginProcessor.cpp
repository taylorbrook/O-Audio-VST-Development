/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <cmath>

// NOTE: PluginEditor.h is deliberately NOT included here — the editor include
// lives inside the #if JUCE_WEB_BROWSER guard above createEditor() so the
// Stage-2 render harness (JUCE_WEB_BROWSER=0, no editor sources) can compile
// this TU (pattern_render_harness_breaks_on_webview_editor).

juce::AudioProcessorValueTreeState::ParameterLayout OBitrotAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ========================================================================
    // GLOBAL (6)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "CLOCK_MODE", 1 },
        "Clock Mode",
        juce::StringArray { "Sync", "Free" },
        0  // Default: Sync
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "CLOCK_SYNC_DIV", 1 },
        "Clock Division",
        juce::StringArray { "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 bar" },
        2  // Default: 1/8
    ));

    {
        auto range = juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f);
        range.setSkewForCentre(1.414f);  // sqrt(0.1 * 20) — exponential feel
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "CLOCK_FREE_RATE", 1 },
            "Clock Rate",
            range,
            2.0f,
            "Hz"
        ));
    }

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "SEED", 1 },
        "Seed",
        0, 9999,
        0
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "HARD_EDGES", 1 },
        "Hard Edges",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    // ========================================================================
    // TAPE (4 of 6 — TAPE_DROP and TAPE_WOW are appended at the END of this
    // layout in v1.4.0, to keep every later automation slot index stable)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "TAPE_ENABLE", 1 },
        "Tape Enable",
        true
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TAPE_PROB", 1 },
        "Tape Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TAPE_STOP_PROB", 1 },
        "Tape-Stop Share",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        10.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TAPE_RAMP", 1 },
        "Tape Ramp",
        juce::NormalisableRange<float>(20.0f, 500.0f, 0.1f),
        150.0f,
        "ms"
    ));

    // ========================================================================
    // CD SKIP (4)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "CD_ENABLE", 1 },
        "CD Enable",
        true
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CD_PROB", 1 },
        "CD Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CD_SEVERITY", 1 },
        "CD Severity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CD_SEGMENT", 1 },
        "CD Segment",
        juce::NormalisableRange<float>(10.0f, 400.0f, 0.1f),
        100.0f,
        "ms"
    ));

    // ========================================================================
    // VINYL (4)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "VINYL_ENABLE", 1 },
        "Vinyl Enable",
        true
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "VINYL_PROB", 1 },
        "Vinyl Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "VINYL_RPM", 1 },
        "Vinyl RPM",
        juce::StringArray { "33 1/3", "45" },  // ASCII-only; UI renders 33 1/3 glyph in Stage 3
        0  // Default: 33 1/3
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "VINYL_POP", 1 },
        "Vinyl Pop",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        "%"
    ));

    // ========================================================================
    // PACKET LOSS (4)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "PACKET_ENABLE", 1 },
        "Packet Enable",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "PACKET_LOSS", 1 },
        "Packet Loss",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        20.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "PACKET_BURST", 1 },
        "Packet Burstiness",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        30.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "PACKET_CONCEAL", 1 },
        "Concealment",
        juce::StringArray { "Silence", "Repeat", "Decay", "Substitute" },
        2  // Default: Decay
    ));

    // ========================================================================
    // CODEC (3)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "CODEC_ENABLE", 1 },
        "Codec Enable",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "CODEC_MODE", 1 },
        "Codec Mode",
        juce::StringArray { "Mu-law", "GSM" },  // ASCII-only; UI renders the mu glyph in Stage 3
        0  // Default: Mu-law
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CODEC_MIX", 1 },
        "Codec Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    // ========================================================================
    // CRUSH (6)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "CRUSH_ENABLE", 1 },
        "Crush Enable",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CRUSH_BITS", 1 },
        "Crush Bits",
        juce::NormalisableRange<float>(1.0f, 16.0f, 0.01f),
        16.0f,
        "bits"
    ));

    {
        auto range = juce::NormalisableRange<float>(500.0f, 20000.0f, 1.0f);
        range.setSkewForCentre(3162.0f);  // sqrt(500 * 20000) — exponential feel
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "CRUSH_RATE", 1 },
            "Crush Rate",
            range,
            20000.0f,
            "Hz"
        ));
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CRUSH_JITTER", 1 },
        "Crush Jitter",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CRUSH_ENV_AMT", 1 },
        "Crush Env Amount",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CRUSH_DITHER", 1 },
        "Crush Dither",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        0.0f,
        "LSB"
    ));

    // ========================================================================
    // TAPE — v1.4.0 additions (2)
    //
    // APPENDED here rather than inserted into the tape block above, on
    // purpose. Layout order is the automation-slot order a host presents, so
    // inserting two parameters after TAPE_RAMP would shift all 23 parameters
    // behind it by two slots and silently repoint every saved automation lane
    // in an existing session. APVTS state, the preset bank and the WebView
    // bindings are all keyed by parameter ID and are order-independent, so
    // nothing else cares where these sit.
    //
    // Both default to 0 — the value at which they are EXACTLY transparent, so
    // a v1.3.0 session or preset loaded into v1.4.0 renders bit-identically
    // (a preset that omits them is reset to default by the preset module's
    // WR-01 reset-to-defaults, which lands on the same 0).
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TAPE_DROP", 1 },
        "Tape Dropout Share",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TAPE_WOW", 1 },
        "Tape Wow/Flutter",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    return layout;
}

OBitrotAudioProcessor::OBitrotAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache raw parameter pointers (atomic, real-time safe).
    // Global
    clockModeParam     = apvts.getRawParameterValue("CLOCK_MODE");
    clockSyncDivParam  = apvts.getRawParameterValue("CLOCK_SYNC_DIV");
    clockFreeRateParam = apvts.getRawParameterValue("CLOCK_FREE_RATE");
    seedParam          = apvts.getRawParameterValue("SEED");
    hardEdgesParam     = apvts.getRawParameterValue("HARD_EDGES");
    mixParam           = apvts.getRawParameterValue("MIX");

    // Tape
    tapeEnableParam   = apvts.getRawParameterValue("TAPE_ENABLE");
    tapeProbParam     = apvts.getRawParameterValue("TAPE_PROB");
    tapeStopProbParam = apvts.getRawParameterValue("TAPE_STOP_PROB");
    tapeRampParam     = apvts.getRawParameterValue("TAPE_RAMP");
    tapeDropParam     = apvts.getRawParameterValue("TAPE_DROP");
    tapeWowParam      = apvts.getRawParameterValue("TAPE_WOW");

    // CD Skip
    cdEnableParam   = apvts.getRawParameterValue("CD_ENABLE");
    cdProbParam     = apvts.getRawParameterValue("CD_PROB");
    cdSeverityParam = apvts.getRawParameterValue("CD_SEVERITY");
    cdSegmentParam  = apvts.getRawParameterValue("CD_SEGMENT");

    // Vinyl
    vinylEnableParam = apvts.getRawParameterValue("VINYL_ENABLE");
    vinylProbParam   = apvts.getRawParameterValue("VINYL_PROB");
    vinylRpmParam    = apvts.getRawParameterValue("VINYL_RPM");
    vinylPopParam    = apvts.getRawParameterValue("VINYL_POP");

    // Packet Loss
    packetEnableParam  = apvts.getRawParameterValue("PACKET_ENABLE");
    packetLossParam    = apvts.getRawParameterValue("PACKET_LOSS");
    packetBurstParam   = apvts.getRawParameterValue("PACKET_BURST");
    packetConcealParam = apvts.getRawParameterValue("PACKET_CONCEAL");

    // Codec
    codecEnableParam = apvts.getRawParameterValue("CODEC_ENABLE");
    codecModeParam   = apvts.getRawParameterValue("CODEC_MODE");
    codecMixParam    = apvts.getRawParameterValue("CODEC_MIX");

    // Crush
    crushEnableParam = apvts.getRawParameterValue("CRUSH_ENABLE");
    crushBitsParam   = apvts.getRawParameterValue("CRUSH_BITS");
    crushRateParam   = apvts.getRawParameterValue("CRUSH_RATE");
    crushJitterParam = apvts.getRawParameterValue("CRUSH_JITTER");
    crushEnvAmtParam = apvts.getRawParameterValue("CRUSH_ENV_AMT");
    crushDitherParam = apvts.getRawParameterValue("CRUSH_DITHER");

    // ── Factory bank (Stage 4): 8 presets ───────────────────────────────────
    // Authored in ENGINEERING units, then batch-converted through each
    // parameter's own NormalisableRange below — raw-fraction authoring would
    // ignore the skew on CLOCK_FREE_RATE (centre 1.414 Hz) and CRUSH_RATE
    // (centre 3162 Hz). Choice params are authored as the INDEX; bools as
    // 0/1; SEED as the integer. Every preset lists all 33 param IDs (defense
    // in depth over the module's WR-01 reset-to-defaults). No customState —
    // SEED is an APVTS param and O-Bitrot has no non-parameter state.
    // Coverage: one showcase per family (1-6), sync (2,4,5,7) + free (1,3,8)
    // clocking, extreme (7) + subtle (8) combos, HARD_EDGES exercised (7),
    // both codec modes (5 GSM, 7 Mu-law).
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets {
        { "Worn Cassette",   // Tape showcase — wow, drag, occasional full stop
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 1.2f },
            { "SEED", 1111.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 45.0f }, { "TAPE_STOP_PROB", 12.0f }, { "TAPE_RAMP", 260.0f },
            { "TAPE_DROP", 35.0f }, { "TAPE_WOW", 55.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f } } },

        { "Skipping Disc",   // CD showcase — machine-gun buffer loops, restart chirps
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 0.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 2222.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 1.0f }, { "CD_PROB", 55.0f }, { "CD_SEVERITY", 0.8f }, { "CD_SEGMENT", 45.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f } } },

        { "Locked Groove",   // Vinyl showcase — revolution jumps, heavy pops, groove holds
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 0.4f },
            { "SEED", 3333.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 1.0f }, { "VINYL_PROB", 40.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 70.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f } } },

        { "Dropped Call",    // Packet showcase — bursty robotic loss
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 4444.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 1.0f }, { "PACKET_LOSS", 65.0f }, { "PACKET_BURST", 65.0f }, { "PACKET_CONCEAL", 1.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f } } },

        { "Cellphone 1998",  // Codec showcase — GSM crunch with a whiff of loss
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 4.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 5555.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 1.0f }, { "PACKET_LOSS", 12.0f }, { "PACKET_BURST", 40.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 1.0f }, { "CODEC_MODE", 1.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f } } },

        { "Eight-Bit Ruin",  // Crush showcase — quantize + SRR + jitter, ducking envelope
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 6666.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 1.0f }, { "CRUSH_BITS", 6.0f }, { "CRUSH_RATE", 11025.0f },
            { "CRUSH_JITTER", 15.0f }, { "CRUSH_ENV_AMT", -35.0f }, { "CRUSH_DITHER", 0.6f } } },

        { "Total Media Failure",  // Extreme combo — everything failing at once, hard splices
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 0.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 7777.0f }, { "HARD_EDGES", 1.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 60.0f }, { "TAPE_STOP_PROB", 25.0f }, { "TAPE_RAMP", 80.0f },
            { "TAPE_DROP", 45.0f }, { "TAPE_WOW", 80.0f },
            { "CD_ENABLE", 1.0f }, { "CD_PROB", 50.0f }, { "CD_SEVERITY", 0.9f }, { "CD_SEGMENT", 25.0f },
            { "VINYL_ENABLE", 1.0f }, { "VINYL_PROB", 50.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 85.0f },
            { "PACKET_ENABLE", 1.0f }, { "PACKET_LOSS", 90.0f }, { "PACKET_BURST", 70.0f }, { "PACKET_CONCEAL", 0.0f },
            { "CODEC_ENABLE", 1.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 1.0f }, { "CRUSH_BITS", 4.0f }, { "CRUSH_RATE", 6000.0f },
            { "CRUSH_JITTER", 40.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 1.0f } } },

        { "Gentle Rot",      // Subtle physical-media patina — mixable default-plus
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 0.7f },
            { "SEED", 8888.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 85.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 12.0f }, { "TAPE_STOP_PROB", 8.0f }, { "TAPE_RAMP", 300.0f },
            { "TAPE_DROP", 20.0f }, { "TAPE_WOW", 25.0f },
            { "CD_ENABLE", 1.0f }, { "CD_PROB", 10.0f }, { "CD_SEVERITY", 0.25f }, { "CD_SEGMENT", 120.0f },
            { "VINYL_ENABLE", 1.0f }, { "VINYL_PROB", 15.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 35.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f } } },
    };

    // Engineering units → normalized through each parameter's
    // NormalisableRange, once, here. initializeFactoryPresets stores the
    // values verbatim and applyPresetJson feeds them back through
    // setValueNotifyingHost (normalized domain).
    for (auto& preset : factoryPresets)
        for (auto& [paramId, value] : preset.parameters)
            if (auto* p = apvts.getParameter(paramId))
                value = p->convertTo0to1(value);

    // Sentinel-gated (v1.0.5): file writes happen only when
    // JucePlugin_VersionString changes — auval/pluginval scan-storm safe.
    presetManager.initializeFactoryPresets(factoryPresets);
}

OBitrotAudioProcessor::~OBitrotAudioProcessor()
{
}

void OBitrotAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Constant latency, all modes: kCompLatency = ceil(0.020 * fs) — an exact
    // integer at every standard rate. CodecStage presents exactly this delay
    // in every state; reported ONCE here, never renegotiated on CODEC_MODE.
    compLatencySamples = (int) std::ceil(0.020 * sampleRate);
    jassert(compLatencySamples <= kMaxWetLatencySamples);
    setLatencySamples(compLatencySamples);

    captureRing.prepare(sampleRate);
    readHead.prepare(sampleRate, captureRing.getSize());
    mediaClock.prepare(sampleRate, samplesPerBlock);
    tapeTransport.prepare(sampleRate);
    tapeDropout.prepare(sampleRate);
    tapeStopGain.prepare(sampleRate);
    wowFlutter.prepare(sampleRate);
    cdSkip.prepare(sampleRate);
    vinylTransport.prepare(sampleRate);
    artifactSynth.prepare(sampleRate);
    packetStage.prepare(sampleRate, packetEnableParam->load() > 0.5f);
    codecStage.prepare(sampleRate, compLatencySamples,
                       codecEnableParam->load() > 0.5f,
                       ((int) codecModeParam->load()) == 1,
                       codecMixParam->load() * 0.01f);
    crushStage.prepare(sampleRate, crushEnableParam->load() > 0.5f,
                       (double) crushRateParam->load());
    quantStage.prepare(sampleRate, crushEnableParam->load() > 0.5f,
                       (double) crushBitsParam->load());

    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32) juce::jmax(1, samplesPerBlock),
                                  2u };
    dryWetMixer.prepare(spec);
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::linear);
    dryWetMixer.setWetLatency((float) compLatencySamples);
    dryWetMixer.setWetMixProportion(juce::jlimit(0.0f, 1.0f, mixParam->load() * 0.01f));

    lastSeed = (int) seedParam->load();
    rngBank.reseed(lastSeed);
    lastAppliedRate = 1.0;

    stopRecoveryLagSamples = kStopRecoverySeconds * sampleRate;
}

void OBitrotAudioProcessor::releaseResources()
{
    // Ring / delay buffers stay allocated (< 5 MB at 192 kHz); prepareToPlay
    // re-sizes them on the next start. GSM handles are freed here (guarded;
    // prepareToPlay recreates them, and processBlock skips codec work on
    // null handles).
    codecStage.releaseHandles();
}

bool OBitrotAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // v1.1.0: mono->mono, mono->stereo, stereo->stereo. Stereo->mono stays
    // rejected — the engine has no downmix rule and hosts offer better ones.
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo())
        return false;
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return in == out
        || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void OBitrotAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples             = buffer.getNumSamples();

    // Clear any output channels that don't have corresponding input data.
    // Bound by buffer.getNumChannels() (Standalone canonical-channelset trap).
    for (int channel = totalNumInputChannels;
         channel < totalNumOutputChannels && channel < buffer.getNumChannels();
         ++channel)
    {
        buffer.clear(channel, 0, numSamples);
    }

    // Defensive: the engine needs prepareToPlay to have run (ring sized,
    // latency reported). Channel bounds come from the BUFFER, not the bus
    // layout (Standalone canonical-channelset trap).
    const int numChannels = buffer.getNumChannels();
    const bool stereoOut  = numChannels >= 2;

    if (numSamples == 0 || numChannels < 1 || captureRing.getSize() == 0)
        return;

    // Scrub non-finite INPUT at the boundary (QUAL-01). Downstream state that
    // is fed signal — the capture ring, packet history, and especially the
    // DryWetMixer's Thiran dry-delay (whose allpass state v computes
    // `alpha * (x - v)` and holds NaN FOREVER once poisoned, even at
    // alpha == 0, since 0 * NaN == NaN) — cannot recover from a NaN era on
    // its own. Finite samples are never written, so the all-off path stays
    // bit-exact (FUNC-02).
    for (int channel = 0; channel < juce::jmin(2, numChannels); ++channel)
    {
        auto* d = buffer.getWritePointer(channel);
        for (int n = 0; n < numSamples; ++n)
            if (! std::isfinite(d[n]))
                d[n] = 0.0f;
    }

    // Mono->stereo: ch1 was cleared above (no input behind it). Duplicate the
    // scrubbed mono input into it so the dry push and the L/R wet pipeline
    // below see dual-mono — the rest of the block runs unchanged.
    if (totalNumInputChannels == 1 && stereoOut)
        buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);

    // ── Block-start bookkeeping (never inside the sample loop) ──────────────

    // Seed-change detection: reseed all streams deterministically at the
    // block boundary (FUNC-04; dice writes SEED from the message thread).
    const int seedNow = (int) seedParam->load();
    if (seedNow != lastSeed)
    {
        lastSeed = seedNow;
        rngBank.reseed(seedNow);
    }

    const bool   hardEdges = hardEdgesParam->load() > 0.5f;
    const int    clockMode = (int) clockModeParam->load();      // 0 Sync, 1 Free
    const int    divIndex  = (int) clockSyncDivParam->load();
    const double freeRate  = (double) clockFreeRateParam->load();

    Arbitration::Params arbParams;
    arbParams.tapeEnabled   = tapeEnableParam->load() > 0.5f;
    arbParams.cdEnabled     = cdEnableParam->load() > 0.5f;
    arbParams.vinylEnabled  = vinylEnableParam->load() > 0.5f;
    arbParams.tapeProb      = (double) tapeProbParam->load() * 0.01;
    arbParams.cdProb        = (double) cdProbParam->load() * 0.01;
    arbParams.vinylProb     = (double) vinylProbParam->load() * 0.01;
    arbParams.tapeStopShare = (double) tapeStopProbParam->load() * 0.01;
    arbParams.tapeDropShare = (double) tapeDropParam->load() * 0.01;
    arbParams.tapeRampMs    = (double) tapeRampParam->load();
    arbParams.cdSeverity    = (double) cdSeverityParam->load();
    arbParams.cdSegmentMs   = (double) cdSegmentParam->load();
    arbParams.vinylRpmIndex = (int) vinylRpmParam->load();
    arbParams.vinylPop01    = vinylPopParam->load() * 0.01f;

    // Packet stage per-block snapshot. The grid + GE chain run
    // unconditionally (documented determinism convention in
    // PacketLossStage.h); PACKET_ENABLE only gates audibility via a ~10 ms
    // fade — off is bit-transparent.
    packetStage.setParams(packetLossParam->load() * 0.01f,
                          packetBurstParam->load() * 0.01f,
                          (int) packetConcealParam->load(),
                          packetEnableParam->load() > 0.5f,
                          hardEdges);

    // Crush + Quant per-block snapshot (one section enable gates both).
    // Determinism convention as PacketLossStage: latch/follower + jitter and
    // dither draws run unconditionally; CRUSH_ENABLE gates audibility.
    const bool crushEnabled = crushEnableParam->load() > 0.5f;
    crushStage.setParams(crushRateParam->load(),
                         crushJitterParam->load() * 0.01f,
                         crushEnabled);
    quantStage.setParams(crushBitsParam->load(),
                         crushDitherParam->load(),
                         crushEnvAmtParam->load() * 0.01f,
                         crushEnabled);

    // Codec per-block snapshot. No RNG in the codec path; the 8 kHz latch
    // phase and alignment rings run unconditionally (pure functions of the
    // sample count); CODEC_ENABLE rides the EnableFade rails.
    codecStage.setParams(codecEnableParam->load() > 0.5f,
                         ((int) codecModeParam->load()) == 1,
                         codecMixParam->load() * 0.01f);

    // Wow/flutter bed depth (v1.4.0). A tape artifact, so it follows
    // TAPE_ENABLE — but it fades over WowFlutter's own multi-second depth ramp
    // rather than switching, because the ramp's slope IS pitch. A running
    // dropout is bounded (<= 150 ms) and finishes on its own, exactly like the
    // CD conceal/mute rungs.
    wowFlutter.setDepth(arbParams.tapeEnabled
                            ? (double) tapeWowParam->load() * 0.01
                            : 0.0);

    // Mid-event disable releases gracefully (ramp back / recovery jump /
    // stop re-jumping), never teleports.
    if (! arbParams.tapeEnabled)
        tapeTransport.release(arbParams.tapeRampMs);
    if (! arbParams.cdEnabled)
        cdSkip.release(readHead, captureRing, hardEdges);
    if (! arbParams.vinylEnabled)
        vinylTransport.release();

    // ── Dry path (before any mutation) ──────────────────────────────────────
    dryWetMixer.setWetMixProportion(juce::jlimit(0.0f, 1.0f, mixParam->load() * 0.01f));
    juce::dsp::AudioBlock<float> block(buffer);
    dryWetMixer.pushDrySamples(block);

    // ── Clock: sample-accurate tick offsets for this block ──────────────────
    mediaClock.processBlock(getPlayHead(), numSamples, clockMode, divIndex, freeRate);

    // ── Wet path: per-sample write-then-read (blockSize-invariance trap) ────
    // Mono buffer: inR aliases inL (the ring captures dual-mono) and outR is
    // absent — wetR is computed and discarded. Aliasing is safe: each
    // iteration reads in*[n] (ring push, step 1) before writing out*[n].
    const float* inL  = buffer.getReadPointer(0);
    const float* inR  = stereoOut ? buffer.getReadPointer(1) : inL;
    float*       outL = buffer.getWritePointer(0);
    float*       outR = stereoOut ? buffer.getWritePointer(1) : nullptr;

    int tickIndex = 0;

    // UI telemetry: per-sample OR so ms-scale events (CD mutes, pops) are
    // never missed at large block sizes. Read-only const accessors — no RNG,
    // no DSP state touched (harness bit-identity preserved).
    uint32_t activity = 0;

    for (int n = 0; n < numSamples; ++n)
    {
        // 1. Write the ring FIRST — the NORMAL read head sits at lag 0.
        captureRing.push(inL[n], inR[n]);

        // 2. Ticks land at exact sample offsets (split-block equivalent).
        //    RNG is consumed ONLY here, in fixed roll order tape->cd->vinyl.
        while (tickIndex < mediaClock.getNumTicks()
               && mediaClock.getTickOffset(tickIndex) == n)
        {
            Arbitration::TickContext ctx { tapeTransport, tapeDropout,
                                           cdSkip, vinylTransport,
                                           readHead, captureRing, artifactSynth,
                                           lastAppliedRate, hardEdges };
            arbitration.onTick(rngBank, arbParams, ctx);
            ++tickIndex;
        }

        // 3. Rate for this sample: tape state machine while a tape event is
        //    in flight; EXACTLY 1.0 while a CD loop or locked groove runs
        //    (exact repeat intervals; pitch never changes); gentle
        //    re-approach trim (<= +2%, ramped) only when fully NORMAL.
        const double lag = readHead.getLag(captureRing.getTotalWritten());

        // A CD loop / locked groove OWNS the read rate only while no tape
        // event is in flight: a CD or vinyl win calls tape.release(), and that
        // ramp keeps running underneath the loop for up to TAPE_RAMP ms. Only
        // when tape is fully idle is the rate exactly 1.0 — which is the
        // premise that makes ReadHead's lag-overflow suppression safe.
        const bool loopOwnsRate = tapeTransport.isIdle()
                                  && (cdSkip.isLooping() || vinylTransport.isLocked());
        double rate;
        if (! tapeTransport.isIdle())
        {
            readHead.clearTrim();               // NORMAL trim restarts from 0
            rate = tapeTransport.nextRate(lag);

            // A deep stop (or a long down-bend) strands the head seconds
            // behind the write head. The +2% re-approach trim would need ~50x
            // the stall duration to recover, and the ReadHead lag-overflow
            // clamp would eventually teleport mid-normal-playback with no
            // family to attribute it to. Instead: when the release ramp lands
            // back on NORMAL still deep in the hole, take ONE intentional
            // crossfaded jump to live through the choke point — "content lost
            // while the transport was stalled" (the CD-recovery pattern).
            if (tapeTransport.consumeReleaseComplete() && lag > stopRecoveryLagSamples)
                readHead.clampAndScheduleJump(
                    static_cast<double>(captureRing.getTotalWritten() - 1),
                    captureRing.getTotalWritten(), hardEdges);
        }
        else if (loopOwnsRate)
        {
            readHead.clearTrim();
            rate = 1.0;
        }
        else
        {
            rate = readHead.reapproachRate(lag);
        }
        lastAppliedRate = rate;

        // 3b. Wow/flutter bed (v1.4.0): a NON-NEGATIVE read offset, not a rate
        //     multiplier. Modulating the rate would falsify the loopOwnsRate
        //     premise above and, at the lag-0 steady state, drive the head
        //     into ReadHead's write-slot pin on every positive excursion.
        //     Offsetting the read is the same physics (pitch deviation is the
        //     derivative of delay) and leaves `rate`, `pos` and the lag budget
        //     untouched — which is why loopOwnsRate above needs no amendment.
        //     Exactly 0.0 while the bed is transparent, so the passthrough
        //     stays on CaptureRing's bit-exact integer path (FUNC-02).
        const double wowOffset = wowFlutter.nextOffsetSamples(rngBank.get(RngBank::wow));

        // 4. Read heads render the transport output.
        float wetL = 0.0f, wetR = 0.0f;
        readHead.renderSample(captureRing, rate, hardEdges, loopOwnsRate, wowOffset, wetL, wetR);

        // 4b. Tape amplitude domain (v1.4.0), before the other families see the
        //     signal. A tape head is a dPhi/dt transducer, so a stop dies with
        //     speed instead of freezing a held sample at full level into a DC
        //     step; then the oxide-shed dropout dip. Both are exact no-ops
        //     while idle. `rate` here is the transport rate — the wow offset
        //     above is deliberately not part of it.
        tapeStopGain.processSample(tapeTransport.consumeStopInstalled(),
                                   tapeTransport.isIdle(), rate, wetL, wetR);
        tapeDropout.processSample(wetL, wetR);

        // 5. CD ladder (conceal dip / mute / loop wrap) then vinyl locked-
        //    groove wrap — both operate on the head/rendered signal.
        cdSkip.processSample(readHead, captureRing, hardEdges, artifactSynth, wetL, wetR);
        vinylTransport.processSample(readHead, captureRing, hardEdges, artifactSynth, rngBank);

        // 6. Artifact bus (pops / ticks / chirps): mono, both channels, runs
        //    every sample so the IIR state stays continuous. Exact 0.0f when
        //    nothing was ever triggered (FUNC-02 preserved).
        const float artifact = artifactSynth.renderSample();
        wetL += artifact;
        wetR += artifact;

        // 7. Packet loss (own 20 ms grid, GE Markov, concealment). RNG
        //    (packet stream) consumed only at packet boundaries.
        packetStage.processSample(rngBank, wetL, wetR);

        // 8. CodecStage: phone chain (mono -> BP -> 8 kHz latch -> mu-law |
        //    GSM -> post-LPF -> equal-power blend), presenting exactly
        //    kCompLatency delay in every state — bit-transparent alignment
        //    delay when disabled. ARCHITECTURE chain order:
        //    Packet -> Codec -> Crush -> Quant.
        codecStage.processSample(wetL, wetR);

        // 9. Crush (fractional-hold SRR + jitter) then Quant (fractional
        //    bits + TPDF + env-driven depth) — the "output converter"
        //    position. Jitter/dither streams draw unconditionally per sample.
        crushStage.processSample(rngBank, wetL, wetR);
        quantStage.processSample(rngBank, wetL, wetR);

        outL[n] = wetL;
        if (stereoOut)
            outR[n] = wetR;

        if (! tapeTransport.isIdle()
            || tapeDropout.isActive())       activity |= kTapeActive;
        if (cdSkip.isActive())               activity |= kCdActive;
        if (vinylTransport.isLocked()
            || artifactSynth.popActive())    activity |= kVinylActive;
        if (packetStage.isConcealing())      activity |= kPacketActive;
    }

    uiActivityMask.store(activity, std::memory_order_relaxed);

    // ── Dry/wet blend (dry is delayed inside the mixer by setWetLatency) ────
    dryWetMixer.mixWetSamples(block);
}

// The editor include lives INSIDE the guard: the Stage-2 render harness
// compiles this file with JUCE_WEB_BROWSER=0 and no editor sources, so a
// top-of-file include would break the harness the moment the editor gains
// WebView types (pattern_render_harness_breaks_on_webview_editor).
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* OBitrotAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OBitrotAudioProcessorEditor(*this);
#else
    return new juce::GenericAudioProcessorEditor(*this);   // harness build
#endif
}

void OBitrotAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void OBitrotAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBitrotAudioProcessor();
}
