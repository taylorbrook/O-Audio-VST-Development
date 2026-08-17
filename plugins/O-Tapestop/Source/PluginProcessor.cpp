/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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
#include "PluginProcessor.h"

#include <cmath>

// The editor is WebView-bound from Stage 3; the Stage-2 render harness builds
// this translation unit with JUCE_WEB_BROWSER=0 and no editor sources
// (pattern_render_harness_breaks_on_webview_editor).
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif

namespace
{
    // The single shared tempo-division list — deliberately triplet-free
    // (7 entries; NOT O-Bitrot's list). Indices: 0=1/16, 1=1/8, 2=1/4,
    // 3=1/2, 4=1 bar, 5=2 bars, 6=4 bars.
    juce::StringArray syncDivisionChoices()
    {
        return { "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars" };
    }

    // 10–8000 ms, skew 0.35 — shared by STOP_FREE_MS / START_FREE_MS /
    // ENV_FREE_MS (parameter-spec.md, BINDING). The 8000 ms ceiling is DERIVED
    // from kMaxGestureSeconds so the ring-sizing static_assert in the header
    // and this range can never drift apart silently
    // (pattern_test_fixture_mirrors_drift_silently).
    juce::NormalisableRange<float> freeMsRange()
    {
        return { 10.0f,
                 (float) (TapestopProcessor::kMaxGestureSeconds * 1000.0),
                 0.0f, 0.35f };
    }

    juce::NormalisableRange<float> percentRange()
    {
        return { 0.0f, 100.0f, 0.0f, 1.0f };
    }

    // ── Factory-preset envelope blobs (Stage 4) ─────────────────────────────
    // customState carriage: ONE property, "scratchEnvelope", holding the same
    // versioned JSON string ScratchEnvelope::toJson() persists — the preset
    // layer never parses it. EVERY factory preset carries a blob, including
    // Stop-mode ones: applyPresetJson invokes the load callback only when
    // customState exists, and a preset without it would silently inherit the
    // live envelope (no custom-state reset-to-default exists).
    juce::var envelopeCustomState (const char* envelopeJson)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("scratchEnvelope", juce::String (envelopeJson));
        return juce::var (obj);
    }

    // The ScratchEnvelope default gentle wobble, spelled out (schema v1).
    // Stop-mode presets carry this so switching them to Scratch behaves like
    // a fresh instance.
    constexpr const char* kDefaultWobbleEnv =
        R"({"v":1,"points":[{"x":0.0,"y":0.5,"curve":0.0},{"x":0.25,"y":0.65,"curve":0.0},{"x":0.5,"y":0.35,"curve":0.0},{"x":0.75,"y":0.6,"curve":0.0},{"x":1.0,"y":0.5,"curve":0.0}]})";

    // Baby scratch: symmetric forward/back rock around 1× (y=0.5 is 1×,
    // y=−0.5 is full reverse at 1×).
    constexpr const char* kBabyScratchEnv =
        R"({"v":1,"points":[{"x":0.0,"y":0.5,"curve":0.0},{"x":0.25,"y":-0.5,"curve":0.0},{"x":0.5,"y":0.5,"curve":0.0},{"x":0.75,"y":-0.5,"curve":0.0},{"x":1.0,"y":0.5,"curve":0.0}]})";

    // Chirp flare: sharp accelerating strokes with two reverse dips.
    constexpr const char* kChirpFlareEnv =
        R"({"v":1,"points":[{"x":0.0,"y":0.5,"curve":0.5},{"x":0.15,"y":0.9,"curve":0.0},{"x":0.3,"y":-0.7,"curve":-0.5},{"x":0.45,"y":0.6,"curve":0.0},{"x":0.6,"y":-0.4,"curve":0.5},{"x":0.8,"y":0.8,"curve":0.0},{"x":1.0,"y":0.5,"curve":0.0}]})";

    // Stutter scratch: square-ish alternation via near-vertical segments.
    constexpr const char* kStutterEnv =
        R"({"v":1,"points":[{"x":0.0,"y":0.5,"curve":0.0},{"x":0.12,"y":0.5,"curve":0.0},{"x":0.13,"y":-0.75,"curve":0.0},{"x":0.37,"y":-0.75,"curve":0.0},{"x":0.38,"y":0.75,"curve":0.0},{"x":0.62,"y":0.75,"curve":0.0},{"x":0.63,"y":-0.75,"curve":0.0},{"x":0.87,"y":-0.75,"curve":0.0},{"x":0.88,"y":0.5,"curve":0.0},{"x":1.0,"y":0.5,"curve":0.0}]})";

    // ── v1.3 scratch envelopes ──────────────────────────────────────────────
    // Transformer: rhythmic full-speed / dead-stop chop (y=0 is stopped).
    constexpr const char* kTransformerEnv =
        R"({"v":1,"points":[{"x":0.0,"y":0.5,"curve":0.0},{"x":0.24,"y":0.5,"curve":0.0},{"x":0.25,"y":0.0,"curve":0.0},{"x":0.49,"y":0.0,"curve":0.0},{"x":0.5,"y":0.5,"curve":0.0},{"x":0.74,"y":0.5,"curve":0.0},{"x":0.75,"y":0.0,"curve":0.0},{"x":0.99,"y":0.0,"curve":0.0},{"x":1.0,"y":0.5,"curve":0.0}]})";

    // Tape rewind: dive into a held fast-reverse drag, then snap back to 1×.
    constexpr const char* kRewindEnv =
        R"({"v":1,"points":[{"x":0.0,"y":0.5,"curve":-0.5},{"x":0.2,"y":-0.9,"curve":0.0},{"x":0.8,"y":-0.9,"curve":0.5},{"x":1.0,"y":0.5,"curve":0.0}]})";

    // Orbit: one smooth wide swing — overspeed crest into a shallow reverse
    // trough and back. Gentle curves keep it seasick-smooth, never stepped.
    constexpr const char* kOrbitEnv =
        R"({"v":1,"points":[{"x":0.0,"y":0.5,"curve":0.3},{"x":0.25,"y":0.95,"curve":-0.3},{"x":0.5,"y":-0.35,"curve":0.3},{"x":0.75,"y":0.7,"curve":-0.3},{"x":1.0,"y":0.5,"curve":0.0}]})";

    // Crab roll: four fast forward/reverse flicks per cycle.
    constexpr const char* kCrabRollEnv =
        R"({"v":1,"points":[{"x":0.0,"y":0.6,"curve":0.0},{"x":0.12,"y":-0.6,"curve":0.0},{"x":0.25,"y":0.6,"curve":0.0},{"x":0.37,"y":-0.6,"curve":0.0},{"x":0.5,"y":0.6,"curve":0.0},{"x":0.62,"y":-0.6,"curve":0.0},{"x":0.75,"y":0.6,"curve":0.0},{"x":0.87,"y":-0.6,"curve":0.0},{"x":1.0,"y":0.5,"curve":0.0}]})";

    // Division table {1/16, 1/8, 1/4, 1/2, 1 bar, 2 bars, 4 bars} → beats.
    // ASSUMES 4/4 (suite precedent — recorded in stages/2-dsp/NOTES.md).
    // Indices match syncDivisionChoices() above.
    constexpr double kDivisionBeats[7] = { 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0 };

    // CONT_DEPTH → peak fractional speed deviation, log-perceptual:
    // 0 → 0.1 % (±2 cents), 0.5 → 1.1 %, 1 → 12 % (≈ ±2 semitones). Keeps
    // r = 1 ± m deep inside the engine's [−2, +2] rail at every setting
    // (research/o-tapestop-continuous-mode.md, "Parameters").
    double continuousPeakDeviation(double depth01)
    {
        return 0.001 * std::pow(10.0, 2.08 * juce::jlimit(0.0, 1.0, depth01));
    }

    // "X.Y.Z" < "1.1.0"? Non-parsing / empty versions count as OLD — every
    // O-Tapestop preset before the MODE append carried a version string, so
    // an unknown one can only be pre-1.1 (or hand-mangled; remapping a 2-way
    // MODE fraction is still the safe read of it).
    bool presetVersionIsPre110(const juce::String& v)
    {
        auto tokens = juce::StringArray::fromTokens(v, ".", {});
        const int major = tokens.size() > 0 ? tokens[0].getIntValue() : 0;
        const int minor = tokens.size() > 1 ? tokens[1].getIntValue() : 0;
        return major < 1 || (major == 1 && minor < 1);
    }
} // namespace

TapestopProcessor::TapestopProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache raw parameter atomics once — read per block on the audio thread
    // (suite convention; research/ARCHITECTURE.md line 348).
    pEngage       = parameters.getRawParameterValue("ENGAGE");
    pMode         = parameters.getRawParameterValue("MODE");
    pSyncMode     = parameters.getRawParameterValue("SYNC_MODE");
    pStopSyncDiv  = parameters.getRawParameterValue("STOP_SYNC_DIV");
    pStopFreeMs   = parameters.getRawParameterValue("STOP_FREE_MS");
    pStopCurve    = parameters.getRawParameterValue("STOP_CURVE");
    pStartSyncDiv = parameters.getRawParameterValue("START_SYNC_DIV");
    pStartFreeMs  = parameters.getRawParameterValue("START_FREE_MS");
    pStartCurve   = parameters.getRawParameterValue("START_CURVE");
    pEnvSyncDiv   = parameters.getRawParameterValue("ENV_SYNC_DIV");
    pEnvFreeMs    = parameters.getRawParameterValue("ENV_FREE_MS");
    pCharacter    = parameters.getRawParameterValue("CHARACTER");
    pContRateDiv  = parameters.getRawParameterValue("CONT_RATE_SYNC_DIV");
    pContRateHz   = parameters.getRawParameterValue("CONT_RATE_HZ");
    pContDepth    = parameters.getRawParameterValue("CONT_DEPTH");
    pContChaos    = parameters.getRawParameterValue("CONT_CHAOS");
    pToneTrack    = parameters.getRawParameterValue("TONE_TRACK");
    pMix          = parameters.getRawParameterValue("MIX");
    pOutputGain   = parameters.getRawParameterValue("OUTPUT_GAIN");

    // getRawParameterValue returns nullptr only for an id missing from the
    // layout — a typo, never a runtime condition. Fail loudly here, in debug,
    // on first instantiation (O-ReverseDelay v1.7.3 IN-06 posture).
    jassert(pEngage != nullptr && pMode != nullptr && pSyncMode != nullptr
            && pStopSyncDiv != nullptr && pStopFreeMs != nullptr && pStopCurve != nullptr
            && pStartSyncDiv != nullptr && pStartFreeMs != nullptr && pStartCurve != nullptr
            && pEnvSyncDiv != nullptr && pEnvFreeMs != nullptr
            && pCharacter != nullptr && pContRateDiv != nullptr && pContRateHz != nullptr
            && pContDepth != nullptr && pContChaos != nullptr
            && pToneTrack != nullptr && pMix != nullptr && pOutputGain != nullptr);

    // ── Preset migration (v1.1.0): MODE gained "Continuous" ─────────────────
    // Presets store NORMALIZED values; a pre-1.1 MODE fraction n over 2
    // choices (end = 1) decodes as round(n·1); over 3 choices (end = 2) the
    // SAME fraction decodes as round(n·2) — Scratch (1.0) would load as
    // Continuous. Remap: index = round(n_old·1), n_new = index/2. Runs before
    // the WR-01 reset/apply passes; factory presets regenerate at the version
    // bump (WR-04) and never hit the pre-1.1 gate.
    presetManager.setMigrationCallback(
        [](juce::DynamicObject& params, const juce::String& presetVersion)
        {
            if (!presetVersionIsPre110(presetVersion))
                return;

            if (params.hasProperty("MODE"))
            {
                const double nOld = (double) params.getProperty("MODE");
                params.setProperty("MODE", std::round(juce::jlimit(0.0, 1.0, nOld)) * 0.5);
            }
        });

    // ── Preset custom state (Stage 4): the scratch-envelope blob ────────────
    // Save: the SAME string getStateInformation persists — opaque and
    // self-versioned; the preset layer never parses it. Load: defensive
    // ladder (null obj → leave the envelope alone; missing/non-string
    // property → leave it alone), then commitScratchEnvelopeJson(), which
    // does the message-thread bake + atomic publish AND bumps
    // uiEnvGeneration, so the 30 Hz editor timer pushes the sanitized echo
    // to the WebView with zero new plumbing. loadPreset() runs from a native
    // fn → message thread → thread contract holds.
    presetManager.setCustomStateCallbacks(
        [this]() -> juce::var
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty("scratchEnvelope", scratchEnvelope.toJson());
            return juce::var(obj);
        },
        [this](const juce::var& data)
        {
            if (auto* obj = data.getDynamicObject())
            {
                const juce::var blob = obj->getProperty("scratchEnvelope");
                if (blob.isString())
                    commitScratchEnvelopeJson(blob.toString());
            }
        });

    // ── Factory bank (Stage 4; +6 Continuous in v1.1; +14 in v1.3): 28 ──────
    // v1.3 groups the bank into four display themes — Tape Stops, Scratch,
    // Wobble & Warp, Glitch & Chaos. The grouping lives ONLY in js/app.js
    // (PRESET_THEMES): getPresetList() stays a flat alphabetical sort and the
    // preset JSON format is unchanged. Keep app.js's map in sync with the
    // names here — an unmapped factory preset falls into the "User" group.
    // Authored in ENGINEERING units, then batch-converted through each
    // parameter's NormalisableRange below (CR-02) — raw-fraction authoring
    // would ignore the 0.35 skew on the three FREE_MS ranges
    // (pattern_factory_preset_normalized_ignores_skew). Choice params are
    // authored as the INDEX. Every preset lists all 19 param IDs (defense in
    // depth over the WR-01 reset) and carries an explicit envelope blob.
    // Coverage: all three MODEs, both SYNC_MODEs, curve extremes, all three
    // Continuous characters. ENGAGE is 0 everywhere — loading a preset must
    // never fire a gesture by itself.
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets {
        { "Classic Half-Bar Stop",
          { { "ENGAGE", 0.0f }, { "MODE", 0.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 60.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Classic 1-Bar Stop",
          { { "ENGAGE", 0.0f }, { "MODE", 0.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 4.0f }, { "STOP_FREE_MS", 1500.0f }, { "STOP_CURVE", 85.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 400.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 70.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "DJ Spinup",
          { { "ENGAGE", 0.0f }, { "MODE", 0.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 180.0f }, { "START_CURVE", 15.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 55.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Baby Scratch",
          { { "ENGAGE", 0.0f }, { "MODE", 1.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 2.0f }, { "ENV_FREE_MS", 500.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 65.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kBabyScratchEnv) },

        { "Chirp Flare",
          { { "ENGAGE", 0.0f }, { "MODE", 1.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 3.0f }, { "ENV_FREE_MS", 800.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 50.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kChirpFlareEnv) },

        { "Tempo-Synced Short Stop",
          { { "ENGAGE", 0.0f }, { "MODE", 0.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 1.0f }, { "STOP_FREE_MS", 120.0f }, { "STOP_CURVE", 20.0f },
            { "START_SYNC_DIV", 1.0f }, { "START_FREE_MS", 120.0f }, { "START_CURVE", 20.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 40.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Slow-Tape Drag",
          { { "ENGAGE", 0.0f }, { "MODE", 0.0f }, { "SYNC_MODE", 1.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 4000.0f }, { "STOP_CURVE", 70.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 800.0f }, { "START_CURVE", 60.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 90.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        // ── v1.3 Tape Stops additions ───────────────────────────────────────
        { "Power Cut",
          { { "ENGAGE", 0.0f }, { "MODE", 0.0f }, { "SYNC_MODE", 1.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 180.0f }, { "STOP_CURVE", 15.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 300.0f }, { "START_CURVE", 40.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 85.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Cassette Eject",
          { { "ENGAGE", 0.0f }, { "MODE", 0.0f }, { "SYNC_MODE", 1.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 2500.0f }, { "STOP_CURVE", 65.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 500.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 95.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Two-Bar Dive",
          { { "ENGAGE", 0.0f }, { "MODE", 0.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 5.0f }, { "STOP_FREE_MS", 4000.0f }, { "STOP_CURVE", 80.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 75.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Snap Back",
          { { "ENGAGE", 0.0f }, { "MODE", 0.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 2.0f }, { "STOP_FREE_MS", 250.0f }, { "STOP_CURVE", 40.0f },
            { "START_SYNC_DIV", 0.0f }, { "START_FREE_MS", 60.0f }, { "START_CURVE", 10.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 50.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Half-Mix Stop",
          { { "ENGAGE", 0.0f }, { "MODE", 0.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 60.0f }, { "MIX", 50.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Stutter-Scratch",
          { { "ENGAGE", 0.0f }, { "MODE", 1.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 1.0f }, { "ENV_FREE_MS", 250.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 45.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kStutterEnv) },

        // ── v1.3 Scratch additions — each rides its own envelope shape ──────
        { "Transformer",
          { { "ENGAGE", 0.0f }, { "MODE", 1.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 2.0f }, { "ENV_FREE_MS", 500.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 45.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kTransformerEnv) },

        { "Tape Rewind",
          { { "ENGAGE", 0.0f }, { "MODE", 1.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1500.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 70.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kRewindEnv) },

        { "Orbit",
          { { "ENGAGE", 0.0f }, { "MODE", 1.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 3.0f }, { "ENV_FREE_MS", 800.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 55.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kOrbitEnv) },

        { "Crab Roll",
          { { "ENGAGE", 0.0f }, { "MODE", 1.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 1.0f }, { "ENV_FREE_MS", 200.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 20.0f },
            { "TONE_TRACK", 40.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kCrabRollEnv) },

        // ── Continuous mode (v1.1) — depth/chaos values from the research
        // synthesis (research/o-tapestop-continuous-mode.md, "Proposed
        // factory presets"); wobble/random run Free, glitch runs Synced.
        { "Subtle Wobble",
          { { "ENGAGE", 0.0f }, { "MODE", 2.0f }, { "SYNC_MODE", 1.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 1.2f },
            { "CONT_DEPTH", 35.0f }, { "CONT_CHAOS", 15.0f },
            { "TONE_TRACK", 30.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Warped Record",
          { { "ENGAGE", 0.0f }, { "MODE", 2.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 4.0f }, { "CONT_RATE_HZ", 0.55f },
            { "CONT_DEPTH", 60.0f }, { "CONT_CHAOS", 35.0f },
            { "TONE_TRACK", 55.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Drunk Tape",
          { { "ENGAGE", 0.0f }, { "MODE", 2.0f }, { "SYNC_MODE", 1.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 1.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 0.5f },
            { "CONT_DEPTH", 55.0f }, { "CONT_CHAOS", 50.0f },
            { "TONE_TRACK", 60.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Seasick",
          { { "ENGAGE", 0.0f }, { "MODE", 2.0f }, { "SYNC_MODE", 1.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 1.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 0.15f },
            { "CONT_DEPTH", 75.0f }, { "CONT_CHAOS", 70.0f },
            { "TONE_TRACK", 75.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        // ── v1.3 Wobble & Warp additions ────────────────────────────────────
        { "Tape Flutter",
          { { "ENGAGE", 0.0f }, { "MODE", 2.0f }, { "SYNC_MODE", 1.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 7.0f },
            { "CONT_DEPTH", 15.0f }, { "CONT_CHAOS", 25.0f },
            { "TONE_TRACK", 30.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Pitch Tide",
          { { "ENGAGE", 0.0f }, { "MODE", 2.0f }, { "SYNC_MODE", 1.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 0.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 0.08f },
            { "CONT_DEPTH", 55.0f }, { "CONT_CHAOS", 10.0f },
            { "TONE_TRACK", 50.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Loose Capstan",
          { { "ENGAGE", 0.0f }, { "MODE", 2.0f }, { "SYNC_MODE", 1.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 1.0f }, { "CONT_RATE_SYNC_DIV", 2.0f }, { "CONT_RATE_HZ", 0.9f },
            { "CONT_DEPTH", 45.0f }, { "CONT_CHAOS", 60.0f },
            { "TONE_TRACK", 55.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Glitch",
          { { "ENGAGE", 0.0f }, { "MODE", 2.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 2.0f }, { "CONT_RATE_SYNC_DIV", 1.0f }, { "CONT_RATE_HZ", 4.0f },
            { "CONT_DEPTH", 60.0f }, { "CONT_CHAOS", 55.0f },
            { "TONE_TRACK", 40.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Total Meltdown",
          { { "ENGAGE", 0.0f }, { "MODE", 2.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 2.0f }, { "CONT_RATE_SYNC_DIV", 0.0f }, { "CONT_RATE_HZ", 8.0f },
            { "CONT_DEPTH", 90.0f }, { "CONT_CHAOS", 95.0f },
            { "TONE_TRACK", 65.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        // ── v1.3 Glitch & Chaos additions ───────────────────────────────────
        { "Sputter",
          { { "ENGAGE", 0.0f }, { "MODE", 2.0f }, { "SYNC_MODE", 0.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 2.0f }, { "CONT_RATE_SYNC_DIV", 1.0f }, { "CONT_RATE_HZ", 2.5f },
            { "CONT_DEPTH", 40.0f }, { "CONT_CHAOS", 30.0f },
            { "TONE_TRACK", 35.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },

        { "Data Rot",
          { { "ENGAGE", 0.0f }, { "MODE", 2.0f }, { "SYNC_MODE", 1.0f },
            { "STOP_SYNC_DIV", 3.0f }, { "STOP_FREE_MS", 500.0f }, { "STOP_CURVE", 50.0f },
            { "START_SYNC_DIV", 2.0f }, { "START_FREE_MS", 250.0f }, { "START_CURVE", 50.0f },
            { "ENV_SYNC_DIV", 4.0f }, { "ENV_FREE_MS", 1000.0f },
            { "CHARACTER", 2.0f }, { "CONT_RATE_SYNC_DIV", 1.0f }, { "CONT_RATE_HZ", 13.0f },
            { "CONT_DEPTH", 75.0f }, { "CONT_CHAOS", 85.0f },
            { "TONE_TRACK", 50.0f }, { "MIX", 100.0f }, { "OUTPUT_GAIN", 0.0f } },
          envelopeCustomState(kDefaultWobbleEnv) },
    };

    // CR-02: engineering units → normalized through each parameter's
    // NormalisableRange, once, here. initializeFactoryPresets stores the
    // values verbatim and applyPresetJson feeds them back through
    // setValueNotifyingHost (normalized domain).
    for (auto& preset : factoryPresets)
        for (auto& [paramId, value] : preset.parameters)
            if (auto* p = parameters.getParameter(paramId))
                value = p->convertTo0to1(value);

    // Sentinel-gated (v1.0.5, WR-04): file writes happen only when
    // JucePlugin_VersionString changes — auval/pluginval scan-storm safe.
    presetManager.initializeFactoryPresets(factoryPresets);
}

TapestopProcessor::~TapestopProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout TapestopProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ── Trigger & Mode ──────────────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "ENGAGE", 1 },
        "Engage",
        false));

    // v1.1: "Continuous" APPENDED (existing indices keep their meaning).
    // Sessions are safe (APVTS stores the index), but presets store the
    // NORMALIZED value — the registered migration callback remaps pre-1.1.0
    // saves (old n over 2 choices → same index over 3). VST3 automation
    // lanes of MODE cannot be migrated (host-side normalized storage) —
    // release-noted caveat.
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MODE", 1 },
        "Mode",
        juce::StringArray { "Stop", "Scratch", "Continuous" },
        0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "SYNC_MODE", 1 },
        "Sync Mode",
        juce::StringArray { "Sync", "Free" },
        0));

    // ── Stop / Start (Stop mode) ────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "STOP_SYNC_DIV", 1 },
        "Stop Time",
        syncDivisionChoices(),
        3)); // 1/2

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "STOP_FREE_MS", 1 },
        "Stop Time (Free)",
        freeMsRange(),
        500.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "STOP_CURVE", 1 },
        "Stop Curve",
        percentRange(),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "START_SYNC_DIV", 1 },
        "Start Time",
        syncDivisionChoices(),
        2)); // 1/4

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "START_FREE_MS", 1 },
        "Start Time (Free)",
        freeMsRange(),
        250.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "START_CURVE", 1 },
        "Start Curve",
        percentRange(),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // ── Scratch (Scratch mode) ──────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "ENV_SYNC_DIV", 1 },
        "Env Length",
        syncDivisionChoices(),
        4)); // 1 bar

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ENV_FREE_MS", 1 },
        "Env Length (Free)",
        freeMsRange(),
        1000.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // ── Continuous (Continuous mode, v1.1) ──────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "CHARACTER", 1 },
        "Character",
        juce::StringArray { "Wobble", "Random", "Glitch" },
        0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "CONT_RATE_SYNC_DIV", 1 },
        "Motion Rate",
        syncDivisionChoices(),
        2)); // 1/4

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CONT_RATE_HZ", 1 },
        "Motion Rate (Free)",
        juce::NormalisableRange<float> { 0.05f, 20.0f, 0.0f, 0.3f },
        1.2f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CONT_DEPTH", 1 },
        "Motion Depth",
        percentRange(),
        35.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CONT_CHAOS", 1 },
        "Motion Chaos",
        percentRange(),
        20.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // ── Output ──────────────────────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TONE_TRACK", 1 },
        "Tone Track",
        percentRange(),
        60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        percentRange(),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OUTPUT_GAIN", 1 },
        "Output Gain",
        juce::NormalisableRange<float> { -24.0f, 12.0f, 0.0f, 1.0f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    return layout;
}

void TapestopProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentFs = sampleRate;

    // The ONLY allocation on any audio-adjacent path (PERF-01). processBlock
    // itself allocates nothing.
    capture.prepare(sampleRate, kCaptureSeconds);

    // Transport → Bypassed; no cached sample counts survive a rate change
    // (durations are converted from ms/beats at gesture edges using currentFs).
    transport.prepare(sampleRate);
    voices[0] = VarispeedVoice{};
    voices[1] = VarispeedVoice{};

    // toneTrack: TPT one-pole, wet-path only. Prepared stereo; reset + first
    // cutoff happen again at every engage edge.
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = sampleRate;
        spec.maximumBlockSize = (juce::uint32) juce::jmax(1, samplesPerBlock);
        spec.numChannels      = 2;
        toneFilter.prepare(spec);
        toneFilter.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
        toneFilter.setCutoffFrequency((float) juce::jmin(20000.0, 0.47 * sampleRate));
        toneFilter.reset();
    }

    mixSmoothed.reset(sampleRate, 0.02);
    mixSmoothed.setCurrentAndTargetValue(pMix->load() * 0.01f);
    gainSmoothed.reset(sampleRate, 0.02);
    gainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(pOutputGain->load()));

    // A session restored with ENGAGE held ON must re-trigger cleanly: leaving
    // the edge detector at `false` makes the next block header see the held-on
    // parameter as a fresh engage edge (research/ARCHITECTURE.md, Sample Rate
    // Handling).
    lastEngage = false;
    tonePrimePending = false;

    // Zero latency — no setLatencySamples call anywhere (Stage-0 constraint).
}

void TapestopProcessor::releaseResources()
{
}

bool TapestopProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Stereo or mono only; in == out; no side-chain, no multi-out
    // (research/ARCHITECTURE.md File I/O).
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != mainOut)
        return false;

    if (layouts.inputBuses.size() > 1 || layouts.outputBuses.size() > 1)
        return false;

    return true;
}

double TapestopProcessor::durationFromParams(const std::atomic<float>* divParam,
                                             const std::atomic<float>* msParam) const noexcept
{
    // SYNC_MODE choice index 0 = Sync, 1 = Free. Only the gesture-edge latch
    // reads this pair — mid-gesture flips are inert (latch contract).
    const bool sync = pSyncMode->load() < 0.5f;

    if (sync)
    {
        const int div = juce::jlimit(0, 6, (int) std::lround((double) divParam->load()));
        return juce::jmax(1.0, kDivisionBeats[div] * (60.0 / currentBpm) * currentFs);
    }

    return juce::jmax(1.0, (double) msParam->load() * 0.001 * currentFs);
}

double TapestopProcessor::gestureDurationSamples(bool isStopGesture) const noexcept
{
    return isStopGesture ? durationFromParams(pStopSyncDiv, pStopFreeMs)
                         : durationFromParams(pStartSyncDiv, pStartFreeMs);
}

double TapestopProcessor::continuousPeriodSamples() const noexcept
{
    const bool sync = pSyncMode->load() < 0.5f;

    if (sync)
    {
        const int div = juce::jlimit(0, 6, (int) std::lround((double) pContRateDiv->load()));
        return juce::jmax(2.0, kDivisionBeats[div] * (60.0 / currentBpm) * currentFs);
    }

    return juce::jmax(2.0, currentFs / juce::jlimit(0.05, 20.0, (double) pContRateHz->load()));
}

void TapestopProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    const int numInputChannels  = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear(channel, 0, numSamples);

    // ── Block header: read atomics + host BPM, detect the ENGAGE edge, update
    // transport (research/ARCHITECTURE.md Processing Order step 1). Durations
    // and curves are LATCHED at the edge — mid-gesture parameter/tempo moves
    // are inert (latch contract; a live ramp never retargets).
    //
    // BPM: O-Polystutter fallback+clamp pattern — no playhead / offline host /
    // missing getBpm() → 120; clamp 20–999. Read per block; the ONLY consumer
    // is the gesture-edge conversion in gestureDurationSamples().
    currentBpm = 120.0;
    if (auto* playHead = getPlayHead())
        if (auto posInfo = playHead->getPosition())
            if (auto bpm = posInfo->getBpm())
                currentBpm = juce::jlimit(20.0, 999.0, *bpm);

    const bool engagedNow = pEngage->load() > 0.5f;

    if (engagedNow != lastEngage)
    {
        if (engagedNow)
        {
            // toneTrack restarts wide open at every engage edge. The state is
            // NOT reset to zero here: a TPT one-pole near Nyquist RINGS from
            // zero state for a few samples (±0.1·x flutter — a P6 metric
            // violation at the edge). Instead the first engaged sample PRIMES
            // the state to the wet sample itself (aa-prime pattern), making
            // the first filtered sample a bitwise pass-through — and the full
            // state overwrite means nothing sticks across gestures (QUAL-01).
            toneFilter.setCutoffFrequency((float) juce::jmin(20000.0, 0.47 * currentFs));
            tonePrimePending = true;

            // MODE is latched HERE (mid-gesture flips inert; disengaged
            // switches touch nothing — the Bypassed path never looks at it).
            // Index decode, NOT a boolean: 3 choices since v1.1 — a > 0.5f
            // test would classify Continuous (index 2) as Scratch.
            const int mode = juce::jlimit(0, 2, (int) std::lround((double) pMode->load()));

            if (mode == 1)
            {
                // Scratch (FUNC-02): latch the envelope LUT pointer ONCE at
                // this edge (load-acquire) and the pass length.
                const auto* lut = scratchEnvelope.acquireLut();
                transport.engageScratch(durationFromParams(pEnvSyncDiv, pEnvFreeMs),
                                        lut->data(), voices, capture);
            }
            else if (mode == 2)
            {
                // Continuous (v1.1): CHARACTER + seeds latch here; DEPTH and
                // RATE stay live on the 16-sample grid below.
                ContinuousMotion::Params mp;
                mp.character     = (ContinuousMotion::Character)
                                       juce::jlimit(0, 2, (int) std::lround((double) pCharacter->load()));
                mp.depth01       = juce::jlimit(0.0, 1.0, (double) pContDepth->load() * 0.01);
                mp.mPeak         = continuousPeakDeviation(mp.depth01);
                mp.chaos         = juce::jlimit(0.0, 1.0, (double) pContChaos->load() * 0.01);
                mp.periodSamples = continuousPeriodSamples();
                transport.engageContinuous(mp, voices, capture);
            }
            else
            {
                const double curveP = std::exp2(2.0 * (double) pStopCurve->load() * 0.01);
                transport.engage(gestureDurationSamples(true), curveP, voices, capture);
            }
        }
        else
        {
            const double curveP = std::exp2(2.0 * (double) pStartCurve->load() * 0.01);
            transport.release(gestureDurationSamples(false), curveP, voices, capture);
        }

        lastEngage = engagedNow;
    }

    mixSmoothed.setTargetValue(pMix->load() * 0.01f);
    gainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(pOutputGain->load()));

    const double toneAmt = (double) pToneTrack->load() * 0.01;   // a ∈ [0, 1]
    const double fcCeil  = juce::jmin(20000.0, 0.47 * currentFs);
    bool engagedAny = false;

    // Continuous-mode live targets (v1.1): computed once per block from the
    // atomics, PUSHED on the absolute 16-sample grid below (the toneTrack
    // cadence — block-size invariant). Inert in every other transport state.
    const double contDepth01 = juce::jlimit(0.0, 1.0, (double) pContDepth->load() * 0.01);
    const double contMPeak   = continuousPeakDeviation(contDepth01);
    const double contPeriod  = continuousPeriodSamples();

    // Capture is stereo-fixed; mono buses write/read channel 0 twice
    // (bounded by the BUFFER's channel count, never the layout's —
    // pattern_standalone_canonical_channelset_oob).
    const int  chans = juce::jmin(buffer.getNumChannels(), 2);
    const int  chR   = chans > 1 ? 1 : 0;
    auto*      dL    = buffer.getWritePointer(0);
    auto*      dR    = buffer.getWritePointer(chR);

    // UI readback (Stage 3): the carrier's last per-sample ratio this block.
    // Publish-only bookkeeping — no DSP path reads it.
    double uiLastR = 1.0;

    for (int n = 0; n < numSamples; ++n)
    {
        const float inL = dL[n];
        const float inR = dR[n];

        // WRITE capture first, then read — per-sample write-then-read makes
        // 512-vs-4096 bit-identity structural and legalises the d = 0 integer
        // live-read (pattern_grain_read_before_capture_write_blocksize). The
        // ring records in EVERY state, including Bypassed.
        capture.pushSample(inL, inR);

        // Smoothers tick every sample in every state (block-size invariance:
        // consumption count is a function of the absolute timeline only).
        const float m = mixSmoothed.getNextValue();
        const float g = gainSmoothed.getNextValue();

        float outL = inL;
        float outR = inR;

        if (! transport.isBypassed())
        {
            // Transport advances the carrier (and any fading voice) and
            // returns per-sample gains. On the crossfade-complete tick the
            // state flips to Bypassed and this sample renders dry — the
            // previous sample's blend was already within ~1e-6 of dry
            // (fadeIn ≈ 1, fadeOut ≈ 0), so the handoff is continuous and
            // every sample from here is bitwise dry (DSP-03).
            const auto t = transport.tick(voices, capture);

            if (! transport.isBypassed())
            {
                engagedAny = true;
                uiLastR    = t.carrierR;   // UI readback only (Stage 3)

                float wetL = voices[t.carrierIdx].read(capture, 0)   * t.carrierWetGain;
                float wetR = voices[t.carrierIdx].read(capture, chR) * t.carrierWetGain;

                if (t.xfActive)
                {
                    // Skip-splice crossfade: the old voice fades out at its
                    // latched rate while the carrier fades in. During resync
                    // the carrier is the integer live-rider, so the fade
                    // lands on bitwise-dry content (DSP-03).
                    wetL = voices[t.fadingIdx].read(capture, 0)   * t.fadeOutGain + wetL * t.fadeInGain;
                    wetR = voices[t.fadingIdx].read(capture, chR) * t.fadeOutGain + wetR * t.fadeInGain;
                }

                // toneTrack (DSP-05): fc = 20000·(150/20000)^(a·(1−min(|r|,1))).
                // a = 0 pins fc = fMax with NO conditional path (pow(x,0)=1);
                // |r| > 1 clamps open. One pow + one tan only when the update
                // fires on the ABSOLUTE 16-sample grid (totalWritten % 16 —
                // never a per-block counter). Cutoff follows the CARRIER's r.
                if ((capture.getTotalWritten() & 15) == 0)
                {
                    const double speed = juce::jmin(std::abs(t.carrierR), 1.0);
                    const double fc    = 20000.0 * std::pow(150.0 / 20000.0,
                                                            toneAmt * (1.0 - speed));
                    toneFilter.setCutoffFrequency((float) juce::jmin(fc, fcCeil));

                    // v1.1: DEPTH/RATE ride the same absolute grid (no-op
                    // outside ContinuousMotionState).
                    transport.setContinuousTargets(contMPeak, contDepth01, contPeriod);
                }

                // Both voices pass through the same filter (wet-path only,
                // engaged-only). Mono buses (chR == 0) filter once — a second
                // processSample on channel 0 would double-advance its state.
                if (tonePrimePending)
                {
                    // Prime ALL channel states to the first wet sample: the
                    // TPT steady state for input x is s1 = x, so this sample
                    // passes bitwise and no zero-state ring ever fires.
                    // (Stereo caveat: R primes to L's value — a 2-sample
                    // ~0.2·|R−L| flutter on decorrelated material, far below
                    // the zero-state ring it replaces.)
                    toneFilter.reset(wetL);
                    tonePrimePending = false;
                }

                wetL = toneFilter.processSample(0, wetL);
                wetR = (chR == 1) ? toneFilter.processSample(1, wetR) : wetL;

                // MIX blend, then OUTPUT_GAIN last — ENGAGED CHAIN ONLY. The
                // trim rides the transport's engaged-trim blend: it releases
                // to trimAmount = 0 (gain EXACTLY 1.0) across the resync
                // fade, so a non-default trim cannot step at the Bypassed
                // handoff, and glides back in over 50 ms on engage.
                const float dry      = 1.0f - m;
                const float trimGain = 1.0f + (g - 1.0f) * t.trimAmount;

                outL = (inL * dry + wetL * m) * trimGain;
                outR = (inR * dry + wetR * m) * trimGain;
            }
        }
        // Bypassed: TRUE hard pass-through — out = in, NO arithmetic at all
        // (bitwise null regardless of MIX/OUTPUT_GAIN; Stage-0 decision #6).
        // The trim CANNOT ride the bypass path even nominally at 0 dB: the
        // APVTS normalized round-trip of the 0 dB default over −24..+12
        // returns ~1.2e-6 dB, so decibelsToGain gives ≈ 1.0000001f, not
        // exactly 1.0f — the multiply flips bits. This is also what makes
        // the Phase-2.2 post-resync null bitwise BY CONSTRUCTION.

        dL[n] = outL;
        dR[n] = outR;
    }

    // Denormal hygiene on the TPT state while engaged (per-block, cheap).
    if (engagedAny)
        toneFilter.snapToZero();

    // ── UI readback publish (Stage 3): relaxed stores once per block at the
    // END of processBlock. The audio thread only ever writes these; the
    // editor's 30 Hz timer reads them relaxed. Bypassed reads as 1.0× — the
    // transport is transparent, so unity is the honest ratio.
    uiState.store((int) transport.getState(), std::memory_order_relaxed);
    uiRatio.store(transport.isBypassed() ? 1.0f : (float) uiLastR,
                  std::memory_order_relaxed);
    uiScratchPhase.store((float) transport.getScratchPhase(),
                         std::memory_order_relaxed);
}

juce::AudioProcessorEditor* TapestopProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new TapestopEditor(*this);
#else
    return nullptr;
#endif
}

bool TapestopProcessor::hasEditor() const
{
#if JUCE_WEB_BROWSER
    return true;
#else
    return false;
#endif
}

const juce::String TapestopProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TapestopProcessor::acceptsMidi() const           { return false; }
bool TapestopProcessor::producesMidi() const          { return false; }
bool TapestopProcessor::isMidiEffect() const          { return false; }
double TapestopProcessor::getTailLengthSeconds() const { return 0.0; }

int TapestopProcessor::getNumPrograms()                                { return 1; }
int TapestopProcessor::getCurrentProgram()                             { return 0; }
void TapestopProcessor::setCurrentProgram(int index)                   { juce::ignoreUnused(index); }
const juce::String TapestopProcessor::getProgramName(int index)        { juce::ignoreUnused(index); return {}; }
void TapestopProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void TapestopProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Standard APVTS XML round-trip; the drawn scratch envelope rides the
    // same tree as ONE string property (NOT a parameter — not automatable).
    auto state = parameters.copyState();
    state.setProperty("scratchEnvelopeJson", scratchEnvelope.toJson(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void TapestopProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
    {
        if (xmlState->hasTagName(parameters.state.getType()))
        {
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));

            // Envelope restore + bake (MESSAGE THREAD). Missing or invalid
            // JSON falls back to the default wobble, silently — a Stage-1
            // session with no property restores cleanly. The audio thread
            // picks the LUT up at the next engage edge.
            const juce::var blob = parameters.state.getProperty("scratchEnvelopeJson");

            if (blob.isString())
                scratchEnvelope.setFromJson(blob.toString());
            else
                scratchEnvelope.resetToDefault();

            // Stage 3: tell an open editor the envelope changed under it —
            // the editor timer compares this counter and pushes the sanitized
            // JSON via emitEventIfBrowserIsVisible (push model; no JS promise
            // that could be dropped while hidden).
            uiEnvGeneration.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TapestopProcessor();
}
