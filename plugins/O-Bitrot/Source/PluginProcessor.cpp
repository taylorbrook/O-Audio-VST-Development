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

namespace
{
    /** True for a preset saved before v1.7.0 — i.e. before "78" was appended
        to VINYL_RPM. A malformed or absent version string parses to 0.0 and
        counts as pre-1.7.0, which is the safe direction: the migration below
        is a no-op for any value that already decodes to a valid two-choice
        index. */
    bool presetVersionIsPre170(const juce::String& v)
    {
        auto tokens = juce::StringArray::fromTokens(v, ".", {});
        const int major = tokens.size() > 0 ? tokens[0].getIntValue() : 0;
        const int minor = tokens.size() > 1 ? tokens[1].getIntValue() : 0;
        return major < 1 || (major == 1 && minor < 7);
    }
} // namespace

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

    // "78" APPENDED in v1.7.0 (brief item 27c). Appending is safe for APVTS
    // session state — that stores the choice INDEX, and 0/1 still mean what
    // they always did — but NOT for presets, which store the NORMALISED
    // fraction: over 2 choices "45" saved as 1.0, and over 3 choices 1.0
    // decodes as "78". The preset-manager v1.0.6 migration hook installed in
    // the constructor repoints pre-1.7.0 saves.
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "VINYL_RPM", 1 },
        "Vinyl RPM",
        juce::StringArray { "33 1/3", "45", "78" },  // ASCII-only; UI renders the 33 1/3 glyph
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

    // ========================================================================
    // MEDIA NOISE BEDS — v1.5.0 additions (5)
    //
    // Appended for the same reason the v1.4.0 pair was: layout order IS the
    // automation-slot order a host presents, so inserting these into their
    // family blocks would shift every parameter behind them and silently
    // repoint saved automation lanes. Everything else (APVTS state, the preset
    // bank, the WebView bindings) is keyed by parameter ID.
    //
    // All five default to their transparent value — 0 for the four levels,
    // and CODEC_MAINS is inert while CODEC_NOISE is 0 — so a v1.4.0 session or
    // preset renders bit-identically. CODEC_MAINS is a NEW choice parameter
    // rather than an appended choice on an existing one, so no saved
    // normalised value is repointed and no preset migration gate is needed.
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TAPE_HISS", 1 },
        "Tape Hiss",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "VINYL_WEAR", 1 },
        "Vinyl Wear",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CODEC_NOISE", 1 },
        "Line Noise",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "CODEC_MAINS", 1 },
        "Mains Frequency",
        juce::StringArray { "50 Hz", "60 Hz" },
        0  // Default: 50 Hz
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "PACKET_COMFORT", 1 },
        "Comfort Noise",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    // ========================================================================
    // VINYL WARP — v1.7.0 addition (1)
    //
    // Appended for the third time for the same reason the v1.4.0 and v1.5.0
    // additions were: layout order IS the automation-slot order a host
    // presents, so putting this in the vinyl block where it belongs visually
    // would shift every parameter behind it and silently repoint saved
    // automation lanes. Everything else (APVTS state, the preset bank, the
    // WebView bindings) is keyed by parameter ID, so the UI puts it in the
    // Vinyl panel regardless.
    //
    // Default 0, and at 0 VinylWarp returns EXACTLY 0.0 — a v1.6.0 session or
    // preset renders bit-identically as far as this parameter is concerned.
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "VINYL_WARP", 1 },
        "Vinyl Warp",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    // ========================================================================
    // CODEC AGC — v1.8.0 addition (1), improvement brief item 16
    //
    // Appended for the fourth time for the reason stated above: layout order
    // IS the automation-slot order, so slotting this into the CODEC block
    // where it belongs visually would repoint every saved automation lane
    // behind it. The UI puts it in the Codec panel regardless — the WebView
    // binds by parameter ID.
    //
    // UNLIKE the three previous appends, this one does NOT default to its
    // transparent value. The research chain has a fast AGC after the codec,
    // and it is a large part of why a phone sounds like a phone, so it is on
    // with the section at 100. Consequences, stated plainly:
    //
    //   * a v1.7.0 session or preset with CODEC_ENABLE OFF is bit-identical
    //     (the whole codec sub-path sits behind the enable rail);
    //   * a v1.7.0 session or preset with CODEC_ENABLE ON renders differently
    //     — as it also does from item 29's segmented mu-law, which has no
    //     opt-out at all. Preserving item 16 alone would have bought nothing.
    //
    // At CODEC_AGC 0 the gain is exp(0) = 1.0f exactly, so the knob's bottom
    // rail restores the v1.7.0 gain structure bit-for-bit.
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CODEC_AGC", 1 },
        "Codec AGC",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    // ========================================================================
    // ROT — v1.10.0 additions (5), improvement brief item 8
    //
    // The seventh family, and the fifth append, for the reason every previous
    // one gave: layout order IS the automation-slot order a host presents, so a
    // block of five inserted anywhere but the end would shift every parameter
    // behind it and silently repoint saved automation lanes. The UI puts these
    // in their own plate regardless — the WebView binds by parameter ID.
    //
    // ROT_ENABLE defaults OFF and, unlike CODEC_AGC in v1.8.0, that default is
    // load-bearing rather than merely polite: the rot gate short-circuits
    // before its RNG draw while disabled, so a v1.9.0 session or preset renders
    // BIT-IDENTICALLY under v1.10.0 no matter what the other four are set to.
    // A preset that omits all five is reset to these defaults by the preset
    // module's WR-01 pass, which lands in the same place.
    //
    // No migration gate is needed: these are new IDs, and nothing about an
    // existing parameter's range or choice list moved
    // (critical_apvts_denormalised_vs_preset_normalised).
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "ROT_ENABLE", 1 },
        "Rot Enable",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ROT_PROB", 1 },
        "Rot Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        "%"
    ));

    // Flip severity: sweeps the flip rate 25 Hz -> 4 kHz exponentially AND
    // opens the reachable bit field from bit 3 to bit 14. It is the FLIP kind's
    // severity only — sticky holds and wrong-decode stretches have no severity
    // axis, their duration is the whole shape.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ROT_DEPTH", 1 },
        "Rot Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        "%"
    ));

    // Kind ladder, same shape as the tape family's stop/dropout shares:
    // STICK is tested first, GARBLE takes a share of what is left, and
    // whatever survives both is a bit-flip window.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ROT_STICK", 1 },
        "Rot Sticky Share",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ROT_GARBLE", 1 },
        "Rot Garble Share",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
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
    tapeHissParam     = apvts.getRawParameterValue("TAPE_HISS");

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
    vinylWearParam   = apvts.getRawParameterValue("VINYL_WEAR");
    vinylWarpParam   = apvts.getRawParameterValue("VINYL_WARP");

    // Packet Loss
    packetEnableParam  = apvts.getRawParameterValue("PACKET_ENABLE");
    packetLossParam    = apvts.getRawParameterValue("PACKET_LOSS");
    packetBurstParam   = apvts.getRawParameterValue("PACKET_BURST");
    packetConcealParam = apvts.getRawParameterValue("PACKET_CONCEAL");
    packetComfortParam = apvts.getRawParameterValue("PACKET_COMFORT");

    // Codec
    codecEnableParam = apvts.getRawParameterValue("CODEC_ENABLE");
    codecModeParam   = apvts.getRawParameterValue("CODEC_MODE");
    codecMixParam    = apvts.getRawParameterValue("CODEC_MIX");
    codecNoiseParam  = apvts.getRawParameterValue("CODEC_NOISE");
    codecMainsParam  = apvts.getRawParameterValue("CODEC_MAINS");
    codecAgcParam    = apvts.getRawParameterValue("CODEC_AGC");

    // Crush
    crushEnableParam = apvts.getRawParameterValue("CRUSH_ENABLE");
    crushBitsParam   = apvts.getRawParameterValue("CRUSH_BITS");
    crushRateParam   = apvts.getRawParameterValue("CRUSH_RATE");
    crushJitterParam = apvts.getRawParameterValue("CRUSH_JITTER");
    crushEnvAmtParam = apvts.getRawParameterValue("CRUSH_ENV_AMT");
    crushDitherParam = apvts.getRawParameterValue("CRUSH_DITHER");

    // Rot
    rotEnableParam = apvts.getRawParameterValue("ROT_ENABLE");
    rotProbParam   = apvts.getRawParameterValue("ROT_PROB");
    rotDepthParam  = apvts.getRawParameterValue("ROT_DEPTH");
    rotStickParam  = apvts.getRawParameterValue("ROT_STICK");
    rotGarbleParam = apvts.getRawParameterValue("ROT_GARBLE");

    // ── Preset migration (v1.7.0): VINYL_RPM gained "78" ────────────────────
    // Presets store NORMALIZED values. AudioParameterChoice encodes index i
    // over N choices as i/(N-1), so a pre-1.7.0 VINYL_RPM fraction was decoded
    // against end = 1 and is now decoded against end = 2: "45", saved as 1.0,
    // would load as "78" — the one saved preset value that silently changes
    // meaning. Remap: index = round(n_old * 1), n_new = index / 2.
    //
    // Runs BEFORE the module's WR-01 reset/apply passes, so it sees the raw
    // saved fractions. Factory presets are regenerated at the version bump
    // (WR-04's version-stamped sentinel) and are written with the CURRENT
    // version string, so they never hit this gate.
    //
    // APVTS SESSION state needs no equivalent: it persists the choice INDEX,
    // not the fraction, and indices 0/1 still mean 33 1/3 and 45
    // (critical_apvts_denormalised_vs_preset_normalised).
    presetManager.setMigrationCallback(
        [](juce::DynamicObject& params, const juce::String& presetVersion)
        {
            if (!presetVersionIsPre170(presetVersion))
                return;

            if (params.hasProperty("VINYL_RPM"))
            {
                const double nOld = (double) params.getProperty("VINYL_RPM");
                params.setProperty("VINYL_RPM",
                                   std::round(juce::jlimit(0.0, 1.0, nOld)) * 0.5);
            }
        });

    // ── Factory bank: 28 presets ────────────────────────────────────────────
    // Authored in ENGINEERING units, then batch-converted through each
    // parameter's own NormalisableRange below — raw-fraction authoring would
    // ignore the skew on CLOCK_FREE_RATE (centre 1.414 Hz) and CRUSH_RATE
    // (centre 3162 Hz). Choice params are authored as the INDEX; bools as
    // 0/1; SEED as the integer. Every preset lists all 45 param IDs (defense
    // in depth over the module's WR-01 reset-to-defaults). No customState —
    // SEED is an APVTS param and O-Bitrot has no non-parameter state.
    //
    // v1.11.0 grows the bank 9 -> 28 (improvement brief item 31). Organised by
    // MEDIA NARRATIVE — nine family/character showcases (1-9), then tape (10-13),
    // vinyl (14-16), CD (17-18), phone-and-network (19-21), rot (22-24), and a
    // four-rung severity ladder (25-28). That grouping lives in the CONTENT, not
    // in this vector's order: OuariconPresetManager::getPresetList() ends with
    // presets.sort(true), so the browser presents all 28 alphabetically no matter
    // what order they are declared in here.
    //
    // Every name is ASCII-only and slash-free on purpose. juce::String's
    // const char* constructor is ASCII-only (a non-ASCII literal mangles), and
    // sanitizePresetName() rewrites '/' to '_' — a name is a FILENAME, so a
    // slash would silently change the file the preset round-trips through.
    //
    // The nine v1.10.0 names are carried forward VERBATIM. initializeFactoryPresets()
    // writes files and never prunes, so renaming an existing factory preset would
    // strand its old .json in the Factory directory permanently and the bank would
    // list both. Adding is safe; renaming is not.
    //
    // What the new nineteen are for, beyond content: they are the first factory
    // presets to reach values the original nine never did — VINYL_RPM "45"
    // (Dusty 45) and "78" (Shellac 78, the exact value item 27c's migration gate
    // exists for), PACKET_CONCEAL "Substitute" (Last Voicemail), CODEC_MAINS
    // 60 Hz (Last Voicemail), CODEC_AGC off its 100 rail (Answering Machine 45,
    // Transatlantic Line 80), CODEC_MIX below 100 (Transatlantic Line 75), and
    // the CLOCK_SYNC_DIV rungs 1/8T, 1/4T, 1/2 and 1 bar. All seven divisions and
    // both clock modes are now covered by the bank.
    //
    // TAPE_PROB 0 with TAPE_ENABLE 1 (Basement Reel) is deliberate, not an
    // oversight: the beds are gated by their family's ENABLE, not by its
    // probability, so that combination is the continuous wow/hiss layer with the
    // discrete tape events switched off — the v1.2 authenticity work in isolation.
    // Same shape for vinyl: VINYL_WEAR and VINYL_WARP follow VINYL_ENABLE, while
    // pops ride the VINYL_PROB roll.
    //
    // The Disintegration Loop ladder shares one SEED (6060) across all four rungs.
    // The four do not render the same pattern — their parameters differ, so their
    // RNG consumption diverges within the first tick — but they start from one
    // stream, which is the narrative: one tape, four stages of decay.
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets {
        { "Worn Cassette",   // Tape showcase — hiss, wow, drag, occasional full stop
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 1.2f },
            { "SEED", 1111.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 45.0f }, { "TAPE_STOP_PROB", 12.0f }, { "TAPE_RAMP", 260.0f },
            { "TAPE_DROP", 35.0f }, { "TAPE_WOW", 55.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 45.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

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
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        { "Locked Groove",   // Vinyl showcase — worn surface, revolution jumps, heavy pops
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 0.4f },
            { "SEED", 3333.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 1.0f }, { "VINYL_PROB", 40.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 70.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 55.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 45.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        { "Dropped Call",    // Packet showcase — bursty robotic loss over comfort noise
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 4444.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 1.0f }, { "PACKET_LOSS", 65.0f }, { "PACKET_BURST", 65.0f }, { "PACKET_CONCEAL", 1.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 45.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        { "Cellphone 1998",  // Codec showcase — GSM crunch, mains hum, a whiff of loss
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 4.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 5555.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 1.0f }, { "PACKET_LOSS", 12.0f }, { "PACKET_BURST", 40.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 1.0f }, { "CODEC_MODE", 1.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 40.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 35.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

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
            { "CRUSH_JITTER", 15.0f }, { "CRUSH_ENV_AMT", -35.0f }, { "CRUSH_DITHER", 0.6f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

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
            { "CRUSH_JITTER", 40.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 1.0f },
            { "TAPE_HISS", 60.0f }, { "VINYL_WEAR", 70.0f },
            { "CODEC_NOISE", 55.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 60.0f }, { "VINYL_WARP", 60.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 1.0f }, { "ROT_PROB", 70.0f }, { "ROT_DEPTH", 85.0f },
            { "ROT_STICK", 35.0f }, { "ROT_GARBLE", 40.0f } } },

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
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 22.0f }, { "VINYL_WEAR", 25.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 20.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 1.0f }, { "ROT_PROB", 15.0f }, { "ROT_DEPTH", 30.0f },
            { "ROT_STICK", 20.0f }, { "ROT_GARBLE", 20.0f } } },

        { "Corrupt Archive", // Rot showcase (v1.10.0) — a WAV with bad blocks:
                             // crackling flips, decoder hangs, garbled stretches
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 3.0f },
            { "SEED", 9999.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 1.0f }, { "ROT_PROB", 55.0f }, { "ROT_DEPTH", 65.0f },
            { "ROT_STICK", 30.0f }, { "ROT_GARBLE", 35.0f } } },

        // ── Tape (v1.11.0) ──────────────────────────────────────────────────

        { "Warped C-90",     // A mixtape left on a dashboard — heavy continuous
                             // warble, bare oxide patches, thick hiss
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 0.8f },
            { "SEED", 1210.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 40.0f }, { "TAPE_STOP_PROB", 8.0f }, { "TAPE_RAMP", 320.0f },
            { "TAPE_DROP", 55.0f }, { "TAPE_WOW", 75.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 50.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        { "Basement Reel",   // The bed with NO events — TAPE_PROB 0 leaves the
                             // wow and hiss layers running on their own
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 0.4f },
            { "SEED", 1120.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 0.0f }, { "TAPE_STOP_PROB", 0.0f }, { "TAPE_RAMP", 400.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 22.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 30.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        { "Chewed Tape",     // The machine ate it — stops, long bare stretches,
                             // extreme warble, hard splices
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 1.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 1330.0f }, { "HARD_EDGES", 1.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 75.0f }, { "TAPE_STOP_PROB", 35.0f }, { "TAPE_RAMP", 90.0f },
            { "TAPE_DROP", 60.0f }, { "TAPE_WOW", 95.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 65.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        { "Dictaphone Memo", // Microcassette: ~5 kHz of bandwidth, 10 bits, a lot
                             // of hiss and a lot of wow. Crush does the band
                             // limiting, NOT the codec — this is tape, not a phone
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 1.1f },
            { "SEED", 1440.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 20.0f }, { "TAPE_STOP_PROB", 6.0f }, { "TAPE_RAMP", 200.0f },
            { "TAPE_DROP", 30.0f }, { "TAPE_WOW", 45.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 1.0f }, { "CRUSH_BITS", 10.0f }, { "CRUSH_RATE", 5000.0f },
            { "CRUSH_JITTER", 8.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.4f },
            { "TAPE_HISS", 60.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        // ── Vinyl (v1.11.0) ─────────────────────────────────────────────────

        { "Thrift-Store Turntable",  // 33 1/3, warped disc, worn surface, pops
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 0.6f },
            { "SEED", 2140.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 1.0f }, { "VINYL_PROB", 30.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 65.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 60.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 40.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        { "Dusty 45",        // The bank's first "45" — a shorter revolution
                             // quantum, so groove jumps land closer together
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 4.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 2250.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 1.0f }, { "VINYL_PROB", 35.0f }, { "VINYL_RPM", 1.0f }, { "VINYL_POP", 55.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 40.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 25.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        { "Shellac 78",      // The bank's first "78", and the exact VINYL_RPM
                             // value item 27c's preset-migration gate exists for.
                             // Crush at 4.5 kHz / 14 bits stands in for shellac
                             // bandwidth; ENV_AMT +30 crushes harder on peaks,
                             // which is what groove distortion actually does
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 1.5f },
            { "SEED", 2360.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 1.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 2.0f }, { "VINYL_POP", 80.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 1.0f }, { "CRUSH_BITS", 14.0f }, { "CRUSH_RATE", 4500.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 30.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 75.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 30.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        // ── CD (v1.11.0) ────────────────────────────────────────────────────

        { "Scratched CD-R",  // Mid-severity skips on long, lazy loop windows
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 3.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 3170.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 1.0f }, { "CD_PROB", 40.0f }, { "CD_SEVERITY", 0.55f }, { "CD_SEGMENT", 180.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        { "Stuck Disc",      // Machine-gun: 25 ms windows at severity 0.95, past
                             // both the sector-lock (0.5) and servo-seek (0.85)
                             // thresholds, spliced hard on a 1/16 clock
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 0.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 3280.0f }, { "HARD_EDGES", 1.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 1.0f }, { "CD_PROB", 75.0f }, { "CD_SEVERITY", 0.95f }, { "CD_SEGMENT", 25.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        // ── Phone and network (v1.11.0) ─────────────────────────────────────

        { "Last Voicemail",  // Sparse loss on a 1-bar clock, concealed by
                             // SUBSTITUTE over comfort noise, under 60 Hz hum.
                             // The bank's first Substitute and first 60 Hz
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 6.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 4190.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 1.0f }, { "PACKET_LOSS", 18.0f }, { "PACKET_BURST", 55.0f }, { "PACKET_CONCEAL", 3.0f },
            { "CODEC_ENABLE", 1.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 45.0f }, { "CODEC_MAINS", 1.0f }, { "PACKET_COMFORT", 50.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        { "Answering Machine",  // Microcassette behind a mu-law line: tape hiss
                                // and wow INTO the phone chain, 50 Hz hum, and
                                // the bank's first CODEC_AGC off its 100 rail
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 0.9f },
            { "SEED", 4200.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 15.0f }, { "TAPE_STOP_PROB", 4.0f }, { "TAPE_RAMP", 260.0f },
            { "TAPE_DROP", 30.0f }, { "TAPE_WOW", 50.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 1.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 65.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 35.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 45.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        { "Transatlantic Line", // GSM at CODEC_MIX 75 — the bank's first partial
                                // codec blend — with heavy bursty loss decaying
                                // into comfort noise
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 5.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 4310.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 1.0f }, { "PACKET_LOSS", 35.0f }, { "PACKET_BURST", 70.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 1.0f }, { "CODEC_MODE", 1.0f }, { "CODEC_MIX", 75.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 55.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 40.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 80.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        // ── Rot (v1.11.0) — one preset per kind, the shares set to isolate it ─

        { "Bad Blocks",      // FLIP only (stick 0, garble 0): fast ticks, deep
                             // bit field — a PCM file crackling at the block rate
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 8.0f },
            { "SEED", 5220.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 1.0f }, { "ROT_PROB", 60.0f }, { "ROT_DEPTH", 80.0f },
            { "ROT_STICK", 0.0f }, { "ROT_GARBLE", 0.0f } } },

        { "Wrong Byte Offset",  // GARBLE dominant (stick 0, garble 95) at MIX 65
                                // — wrong-decode stretches blended under the dry
                                // programme, which is where they are usable
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 1.6f },
            { "SEED", 5330.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 65.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 1.0f }, { "ROT_PROB", 45.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 0.0f }, { "ROT_GARBLE", 95.0f } } },

        { "Frozen Decoder",  // STICK dominant (stick 90, garble 0) on a 1/16
                             // clock — the playback pointer hanging in DC
                             // plateaus, sixteen chances a bar
          { { "CLOCK_MODE", 0.0f }, { "CLOCK_SYNC_DIV", 0.0f }, { "CLOCK_FREE_RATE", 2.0f },
            { "SEED", 5440.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 0.0f }, { "TAPE_PROB", 25.0f }, { "TAPE_STOP_PROB", 10.0f }, { "TAPE_RAMP", 150.0f },
            { "TAPE_DROP", 0.0f }, { "TAPE_WOW", 0.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 0.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 1.0f }, { "ROT_PROB", 55.0f }, { "ROT_DEPTH", 30.0f },
            { "ROT_STICK", 90.0f }, { "ROT_GARBLE", 0.0f } } },

        // ── Disintegration ladder (v1.11.0) — one tape, four stages of decay,
        //    one shared SEED. Every axis climbs together: tick rate, event
        //    probability, dropout share, wow depth, hiss, and from rung II on,
        //    digital rot underneath the tape. ────────────────────────────────

        { "Disintegration Loop I",    // Old, not yet failing — bed and a rare drop
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 0.35f },
            { "SEED", 6060.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 8.0f }, { "TAPE_STOP_PROB", 0.0f }, { "TAPE_RAMP", 420.0f },
            { "TAPE_DROP", 15.0f }, { "TAPE_WOW", 18.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 28.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 0.0f }, { "ROT_PROB", 25.0f }, { "ROT_DEPTH", 50.0f },
            { "ROT_STICK", 25.0f }, { "ROT_GARBLE", 25.0f } } },

        { "Disintegration Loop II",   // Oxide shedding — rot joins underneath
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 0.5f },
            { "SEED", 6060.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 20.0f }, { "TAPE_STOP_PROB", 4.0f }, { "TAPE_RAMP", 380.0f },
            { "TAPE_DROP", 35.0f }, { "TAPE_WOW", 35.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 42.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 1.0f }, { "ROT_PROB", 12.0f }, { "ROT_DEPTH", 25.0f },
            { "ROT_STICK", 30.0f }, { "ROT_GARBLE", 10.0f } } },

        { "Disintegration Loop III",  // Bare patches, the loop losing its shape
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 0.7f },
            { "SEED", 6060.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 38.0f }, { "TAPE_STOP_PROB", 12.0f }, { "TAPE_RAMP", 300.0f },
            { "TAPE_DROP", 60.0f }, { "TAPE_WOW", 55.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 55.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 1.0f }, { "ROT_PROB", 30.0f }, { "ROT_DEPTH", 45.0f },
            { "ROT_STICK", 40.0f }, { "ROT_GARBLE", 20.0f } } },

        { "Disintegration Loop IV",   // Mostly gone — more absence than programme
          { { "CLOCK_MODE", 1.0f }, { "CLOCK_SYNC_DIV", 2.0f }, { "CLOCK_FREE_RATE", 1.0f },
            { "SEED", 6060.0f }, { "HARD_EDGES", 0.0f }, { "MIX", 100.0f },
            { "TAPE_ENABLE", 1.0f }, { "TAPE_PROB", 60.0f }, { "TAPE_STOP_PROB", 25.0f }, { "TAPE_RAMP", 240.0f },
            { "TAPE_DROP", 85.0f }, { "TAPE_WOW", 80.0f },
            { "CD_ENABLE", 0.0f }, { "CD_PROB", 25.0f }, { "CD_SEVERITY", 0.5f }, { "CD_SEGMENT", 100.0f },
            { "VINYL_ENABLE", 0.0f }, { "VINYL_PROB", 25.0f }, { "VINYL_RPM", 0.0f }, { "VINYL_POP", 50.0f },
            { "PACKET_ENABLE", 0.0f }, { "PACKET_LOSS", 20.0f }, { "PACKET_BURST", 30.0f }, { "PACKET_CONCEAL", 2.0f },
            { "CODEC_ENABLE", 0.0f }, { "CODEC_MODE", 0.0f }, { "CODEC_MIX", 100.0f },
            { "CRUSH_ENABLE", 0.0f }, { "CRUSH_BITS", 16.0f }, { "CRUSH_RATE", 20000.0f },
            { "CRUSH_JITTER", 0.0f }, { "CRUSH_ENV_AMT", 0.0f }, { "CRUSH_DITHER", 0.0f },
            { "TAPE_HISS", 70.0f }, { "VINYL_WEAR", 0.0f },
            { "CODEC_NOISE", 0.0f }, { "CODEC_MAINS", 0.0f }, { "PACKET_COMFORT", 0.0f }, { "VINYL_WARP", 0.0f },
            { "CODEC_AGC", 100.0f },
            { "ROT_ENABLE", 1.0f }, { "ROT_PROB", 50.0f }, { "ROT_DEPTH", 60.0f },
            { "ROT_STICK", 50.0f }, { "ROT_GARBLE", 30.0f } } },
    };

    // ── Narrative categories for the preset menu (v1.13.0) ──────────────────
    // The bank's grouping has always lived in the DECLARATION ORDER above and
    // in the comment at the head of it. getPresetList() ends in presets.sort(),
    // so until the menu shipped there was nowhere the grouping could be seen.
    //
    // Expressed as index SPANS over factoryPresets, never as a second list of
    // names: a name literal repeated here would go stale the first time a
    // preset is renamed and the mismatch would be silent — the preset would
    // just quietly fall into "User"
    // (pattern_test_fixture_mirrors_drift_silently). Spans are inclusive
    // [first, last] and must tile [0, size) exactly; the loop below asserts it.
    struct CategorySpan { const char* label; int first, last; };
    static constexpr CategorySpan categorySpans[] {
        { "Showcases",        0,  8 },   // 1-9   family + character showcases
        { "Tape",             9, 12 },   // 10-13
        { "Vinyl",           13, 15 },   // 14-16
        { "CD",              16, 17 },   // 17-18
        { "Phone & Network", 18, 20 },   // 19-21
        { "Rot",             21, 23 },   // 22-24
        { "Disintegration",  24, 27 },   // 25-28 the four-rung severity ladder
    };

    // Walked in authored order, so factoryCategoryOrder preserves the NARRATIVE
    // sequence within each category — which is load-bearing for the
    // Disintegration ladder (I → IV is a progression, not a set).
    {
        int expectedFirst = 0;
        for (const auto& span : categorySpans)
        {
            jassert (span.first == expectedFirst);          // spans must tile,
            jassert (span.last < (int) factoryPresets.size());  // and stay in range
            for (int i = span.first; i <= span.last; ++i)
                factoryCategoryOrder.push_back ({ factoryPresets[(size_t) i].name,
                                                  span.label });
            expectedFirst = span.last + 1;
        }
        // Every factory preset lands in exactly one category — a preset added
        // to the vector without extending a span would otherwise show up under
        // "User" beside the user's own saves.
        jassert (expectedFirst == (int) factoryPresets.size());
        juce::ignoreUnused (expectedFirst);
    }

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
    vinylWarp.prepare(sampleRate);
    tapeBed.prepare(sampleRate);
    vinylBed.prepare(sampleRate);
    codecBed.prepare(sampleRate);
    cdSkip.prepare(sampleRate);
    vinylTransport.prepare(sampleRate);
    artifactSynth.prepare(sampleRate);
    rotStage.prepare(sampleRate);
    packetStage.prepare(sampleRate, packetEnableParam->load() > 0.5f);
    codecStage.prepare(sampleRate, compLatencySamples,
                       codecEnableParam->load() > 0.5f,
                       ((int) codecModeParam->load()) == 1,
                       codecMixParam->load() * 0.01f,
                       codecAgcParam->load() * 0.01f);
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

    // Rot (v1.10.0). Unlike the trio there is no continuous bed and no state to
    // release: rotEnabled gates the ROLL only, so switching the family off
    // leaves a running event to finish on its own fade (bounded at 300 ms),
    // which is how the CD conceal/mute rungs and the tape dropout behave too.
    arbParams.rotEnabled     = rotEnableParam->load() > 0.5f;
    arbParams.rotProb        = (double) rotProbParam->load() * 0.01;
    arbParams.rotStickShare  = (double) rotStickParam->load() * 0.01;
    arbParams.rotGarbleShare = (double) rotGarbleParam->load() * 0.01;
    arbParams.rotDepth       = (double) rotDepthParam->load() * 0.01;

    // Packet stage per-block snapshot. The grid + GE chain run
    // unconditionally (documented determinism convention in
    // PacketLossStage.h); PACKET_ENABLE only gates audibility via a ~10 ms
    // fade — off is bit-transparent.
    const bool packetEnabled = packetEnableParam->load() > 0.5f;

    packetStage.setParams(packetLossParam->load() * 0.01f,
                          packetBurstParam->load() * 0.01f,
                          (int) packetConcealParam->load(),
                          packetEnabled,
                          hardEdges,
                          packetComfortParam->load() * 0.01f);

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
    const bool  codecEnabled = codecEnableParam->load() > 0.5f;
    const float codecMix01   = codecMixParam->load() * 0.01f;

    codecStage.setParams(codecEnabled,
                         ((int) codecModeParam->load()) == 1,
                         codecMix01,
                         codecAgcParam->load() * 0.01f);

    // Media-noise beds (v1.5.0, brief item 4). Each level arrives already
    // gated by its family's enable, so a family switched off fades its bed out
    // over the bed's own ~30 ms ramp rather than stepping it to zero. At level
    // exactly 0 every bed returns exactly 0.0f and the FUNC-02 null holds.
    tapeBed.setParams(arbParams.tapeEnabled
                          ? tapeHissParam->load() * 0.01f
                          : 0.0f);

    vinylBed.setParams(arbParams.vinylEnabled
                           ? vinylWearParam->load() * 0.01f
                           : 0.0f,
                       arbParams.vinylRpmIndex);

    codecBed.setParams(codecNoiseParam->load() * 0.01f,
                       codecMix01,
                       (int) codecMainsParam->load(),
                       codecEnabled);

    // Wow/flutter bed depth (v1.4.0). A tape artifact, so it follows
    // TAPE_ENABLE — but it fades over WowFlutter's own multi-second depth ramp
    // rather than switching, because the ramp's slope IS pitch. A running
    // dropout is bounded (<= 150 ms) and finishes on its own, exactly like the
    // CD conceal/mute rungs.
    wowFlutter.setDepth(arbParams.tapeEnabled
                            ? (double) tapeWowParam->load() * 0.01
                            : 0.0);

    // Vinyl warp (v1.7.0, brief item 27b). The vinyl family's counterpart to
    // wow/flutter, and gated the same way: it follows VINYL_ENABLE but fades
    // over VinylWarp's own multi-second depth ramp rather than switching,
    // because the ramp's slope IS pitch. VINYL_RPM is passed every block; the
    // LFO's phase accumulator is persistent across the change.
    vinylWarp.setParams(arbParams.vinylEnabled
                            ? (double) vinylWarpParam->load() * 0.01
                            : 0.0,
                        arbParams.vinylRpmIndex);

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
                                           rotStage,
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
        //
        //     v1.7.0 adds the vinyl warp on the SAME terms and for the same
        //     reasons, and the two simply sum: both are non-negative read lags,
        //     so their sum is one too, and the head still never crosses the
        //     write head. Worst case at both depths 1 is ~250 + ~165 samples,
        //     a rounding error against the 10 s ring. A tape deck and a warped
        //     LP are different machines, but nothing stops a patch from asking
        //     for both, and the physics of stacking two delay modulations is
        //     addition.
        const double wowOffset  = wowFlutter.nextOffsetSamples(rngBank.get(RngBank::wow));
        const double warpOffset = vinylWarp.nextOffsetSamples();
        const double readOffset = wowOffset + warpOffset;

        // 4. Read heads render the transport output.
        float wetL = 0.0f, wetR = 0.0f;
        readHead.renderSample(captureRing, rate, hardEdges, loopOwnsRate, readOffset, wetL, wetR);

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

        // 6. Artifact bus (pops / ticks / chirps) plus the v1.5.0 media-noise
        //    beds. All of it runs every sample so the IIR state stays
        //    continuous, and all of it is exactly 0.0f when nothing has been
        //    triggered and every bed level is 0 (FUNC-02 preserved).
        //
        //    Vinyl joins the mono bus for the same reason the pops do, only
        //    more literally: one platter bearing, one stylus. Tape hiss is the
        //    opposite case — two tracks carry two independent noise sources,
        //    so it is stereo, and it rides TapeStopGain's speed gain because
        //    hiss is recorded material that has to die with the transport.
        //
        //    Both are UPSTREAM of the packet stage on purpose: a lost packet
        //    has to conceal the media noise along with the programme, because
        //    on real media they are the same signal.
        const float artifact = artifactSynth.renderSample(rngBank.get(RngBank::scratch))
                             + vinylBed.renderSample(rngBank.get(RngBank::vinylBed));

        float hissL = 0.0f, hissR = 0.0f;
        tapeBed.renderSample(rngBank.get(RngBank::tapeBed),
                             tapeStopGain.currentGain(), hissL, hissR);

        wetL += artifact + hissL;
        wetR += artifact + hissR;

        // 6b. Rot (v1.10.0, brief item 8): XOR bit flips, a sticky held sample,
        //     or a wrong-decode white-noise stretch at the programme's own
        //     level. Placed AFTER the artifact bus and the beds and BEFORE the
        //     packet stage, on the beds' own argument: rot is damage to the
        //     STORED medium, and by the time the file was written the hiss, the
        //     rumble and the pops were part of the programme, so all of it rots
        //     together and all of it has to be concealed together when a packet
        //     goes missing.
        //
        //     Exact no-op on the signal while idle — the envelope follower
        //     inside runs every sample for state continuity but writes only to
        //     itself, so FUNC-02 survives with the ROT knobs at any setting.
        rotStage.processSample(rngBank.get(RngBank::rot), wetL, wetR);

        // 7. Packet loss (own 20 ms grid, GE Markov, concealment). RNG
        //    (packet stream) consumed only at packet boundaries.
        //
        //    The loss flag is read BEFORE the call on purpose (v1.8.0, brief
        //    item 7): processSample advances the packet grid at its END, so
        //    on a boundary sample the post-call flag already describes the
        //    NEXT packet. Read here, it is exactly this sample's state.
        const bool packetLostNow = packetEnabled && packetStage.isPacketLost();

        packetStage.processSample(rngBank, wetL, wetR);

        // 8. CodecStage: phone chain (mono -> BP -> 8 kHz latch -> mu-law |
        //    GSM -> AGC -> post-LPF -> equal-power blend), presenting exactly
        //    kCompLatency delay in every state — bit-transparent alignment
        //    delay when disabled. ARCHITECTURE chain order:
        //    Packet -> Codec -> Crush -> Quant.
        //
        //    packetLostNow is consumed ONLY on the sample that closes a GSM
        //    frame, which is the frame-indexed lookup item 7 calls for: the
        //    8 kHz latch is fractional, so the frame boundary drifts against
        //    the 20 ms packet grid and a shared counter would desynchronise.
        codecStage.processSample(wetL, wetR, packetLostNow);

        // 8b. Codec media bed (v1.5.0): mains hum + line crackle, mono because
        //     the line is mono. AFTER the codec, not on the artifact bus with
        //     the other two: CodecStage is a 300-3400 Hz phone chain, so 50 Hz
        //     injected in front of it is annihilated by the passband and the
        //     hum would be inaudible at every setting. It is also where the
        //     physics puts it — mains hum is induced on the line, not recorded
        //     at the source. Crush and Quant still see it, which is correct:
        //     they are the output converter, downstream of everything.
        const float lineNoise = codecBed.renderSample(rngBank.get(RngBank::codecBed));
        wetL += lineNoise;
        wetR += lineNoise;

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
        if (rotStage.isActive())             activity |= kRotActive;
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

    // v1.12.0: the hover-help preference rides the same tree as one more plain
    // property. Not a parameter (see PluginProcessor.h), so it is saved and
    // restored here rather than by the APVTS parameter round-trip.
    state.setProperty("tooltipsEnabled",
                      tooltipsEnabled.load(std::memory_order_acquire), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void OBitrotAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

        // v1.12.0: hover-help preference. A pre-1.12.0 session has no such
        // property; getProperty returns a VOID var and the default (OFF)
        // stands. The editor PULLS this via the getTooltipsEnabled native fn
        // at page init rather than being pushed — a push from here would race
        // the WebView's load (the O-FreqPulse WR-01 bug).
        //
        // isVoid() is the ONLY correct test, and the obvious isBool()/isInt()
        // one is wrong: getStateInformation writes a bool var, but the XML
        // round-trip does not preserve the type.
        // NamedValueSet::setFromXmlAttributes rebuilds every property as
        // `var (value)` over the attribute STRING, so what comes back is a var
        // holding "1" or "0" and a type test on bool or int is false for every
        // saved session — the preference would restore as OFF forever while
        // build, auval and pluginval all passed
        // (critical_valuetree_xml_roundtrip_loses_type). var's bool conversion
        // handles all three forms, so the cast is safe once the property is
        // known to exist. Gated by a round-trip probe in tests/render-harness.
        const juce::var tips = apvts.state.getProperty("tooltipsEnabled");

        if (! tips.isVoid())
            tooltipsEnabled.store((bool) tips, std::memory_order_release);
    }
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBitrotAudioProcessor();
}
