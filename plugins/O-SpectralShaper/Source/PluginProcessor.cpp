/*
   This file is part of O-SpectralShaper, an Ouaricon Audio plugin.
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

    O-SpectralShaper - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// ============================================================================
// Parameter Layout Creation (JUCE 8 format)
// ============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout
OSpectralShaperAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // MIX (0-100%, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // ATTACK_TIME (0.1-50ms, logarithmic skew)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ATTACK_TIME", 1 },
        "Attack Time",
        juce::NormalisableRange<float>(0.1f, 50.0f, 0.1f, 0.3f),
        10.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // SUSTAIN_TIME (10-500ms, logarithmic skew)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SUSTAIN_TIME", 1 },
        "Sustain Time",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.3f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // SENSITIVITY (0-100%, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SENSITIVITY", 1 },
        "Sensitivity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // LOOKAHEAD_ENABLED (toggle, default OFF)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "LOOKAHEAD_ENABLED", 1 },
        "Lookahead Enabled",
        false
    ));

    // LOOKAHEAD_TIME (0.1-10ms, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "LOOKAHEAD_TIME", 1 },
        "Lookahead Time",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f),
        2.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // OUTPUT_GAIN (-12 to +12 dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OUTPUT_GAIN", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    return layout;
}

// ============================================================================
// Constructor/Destructor
// ============================================================================

OSpectralShaperAudioProcessor::OSpectralShaperAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-SpectralShaper")
{
    // Initialize curves to neutral (no shaping)
    std::fill(attackCurve.begin(), attackCurve.end(), 0.0f);
    std::fill(sustainCurve.begin(), sustainCurve.end(), 0.0f);

    // Custom state callbacks for curve data (not in APVTS)
    presetManager.setCustomStateCallbacks(
        // Save callback
        [this]() -> juce::var {
            auto* obj = new juce::DynamicObject();
            // Encode curves as arrays
            juce::Array<juce::var> attackArr, sustainArr;
            for (int i = 0; i < 32; ++i)
            {
                attackArr.add(static_cast<double>(attackCurve[i]));
                sustainArr.add(static_cast<double>(sustainCurve[i]));
            }
            obj->setProperty("attackCurve", juce::var(attackArr));
            obj->setProperty("sustainCurve", juce::var(sustainArr));
            return juce::var(obj);
        },
        // Load callback
        [this](const juce::var& data) {
            if (auto* obj = data.getDynamicObject())
            {
                if (obj->hasProperty("attackCurve"))
                {
                    auto* arr = obj->getProperty("attackCurve").getArray();
                    if (arr != nullptr && arr->size() == 32)
                    {
                        std::array<float, 32> curve;
                        for (int i = 0; i < 32; ++i)
                            curve[i] = static_cast<float>((*arr)[i]);
                        setAttackCurve(curve);
                    }
                }
                if (obj->hasProperty("sustainCurve"))
                {
                    auto* arr = obj->getProperty("sustainCurve").getArray();
                    if (arr != nullptr && arr->size() == 32)
                    {
                        std::array<float, 32> curve;
                        for (int i = 0; i < 32; ++i)
                            curve[i] = static_cast<float>((*arr)[i]);
                        setSustainCurve(curve);
                    }
                }
            }
        }
    );

    // Helper to build customState var from curve arrays
    auto makeCurveState = [](const std::array<float, 32>& attack,
                             const std::array<float, 32>& sustain) -> juce::var
    {
        auto* obj = new juce::DynamicObject();
        juce::Array<juce::var> attackArr, sustainArr;
        for (int i = 0; i < 32; ++i)
        {
            attackArr.add(static_cast<double>(attack[i]));
            sustainArr.add(static_cast<double>(sustain[i]));
        }
        obj->setProperty("attackCurve", juce::var(attackArr));
        obj->setProperty("sustainCurve", juce::var(sustainArr));
        return juce::var(obj);
    };

    // Curve definitions for factory presets
    // 32 logarithmic bands @ 44.1kHz: band 0=20Hz, 6=68Hz, 9=126Hz, 11=189Hz,
    // 12=232Hz, 15=428Hz, 18=789Hz, 22=1786Hz, 25=3293Hz, 26=4039Hz,
    // 27=4954Hz, 28=6076Hz, 29=7452Hz, 30=9139Hz, 31=11210Hz..Nyquist
    //
    // Curve values are -1..+1 (full deflection = ±12 dB). Author curves as
    // CONTIGUOUS band runs — a gap between listed bands is a 0 dB notch in the
    // middle of the shape, not an interpolation.

    // Sparse authoring helper for the v1.6.0 bank additions: unlisted bands
    // stay 0 (flat). The pre-1.6.0 curves below keep their original
    // per-index-assignment form untouched so a diff shows their values did
    // not change.
    auto curve = [](std::initializer_list<std::pair<int, float>> points)
    {
        std::array<float, 32> c {};
        for (const auto& [band, v] : points)
            c[(size_t) juce::jlimit(0, 31, band)] = v;
        return c;
    };

    // Punch Enhancer: boost attack 60-200Hz, cut sustain 200-800Hz
    std::array<float, 32> punchAtk {};
    punchAtk[6] = 0.3f;  punchAtk[7] = 0.6f;  punchAtk[8] = 0.8f;
    punchAtk[9] = 0.8f;  punchAtk[10] = 0.6f;  punchAtk[11] = 0.3f;
    std::array<float, 32> punchSus {};
    punchSus[12] = -0.3f;  punchSus[13] = -0.5f;  punchSus[14] = -0.6f;
    punchSus[15] = -0.7f;  punchSus[16] = -0.6f;  punchSus[17] = -0.5f;
    punchSus[18] = -0.3f;

    // Transient Tamer: cut attack in upper-mids (2-8kHz)
    std::array<float, 32> tamerAtk {};
    tamerAtk[22] = -0.2f;  tamerAtk[23] = -0.5f;  tamerAtk[24] = -0.7f;
    tamerAtk[25] = -0.8f;  tamerAtk[26] = -0.8f;  tamerAtk[27] = -0.6f;
    tamerAtk[28] = -0.4f;  tamerAtk[29] = -0.2f;
    std::array<float, 32> tamerSus {};

    // De-Esser: target 4-8kHz sustain reduction
    std::array<float, 32> deessAtk {};
    std::array<float, 32> deessSus {};
    deessSus[25] = -0.2f;  deessSus[26] = -0.6f;  deessSus[27] = -0.8f;
    deessSus[28] = -0.8f;  deessSus[29] = -0.5f;  deessSus[30] = -0.2f;

    // Cymbal Control: cut attack 6-16kHz
    std::array<float, 32> cymbAtk {};
    cymbAtk[27] = -0.2f;  cymbAtk[28] = -0.6f;  cymbAtk[29] = -0.8f;
    cymbAtk[30] = -0.8f;  cymbAtk[31] = -0.7f;
    std::array<float, 32> cymbSus {};

    // Warm Sustain: boost sustain 200-800Hz
    std::array<float, 32> warmAtk {};
    std::array<float, 32> warmSus {};
    warmSus[12] = 0.3f;  warmSus[13] = 0.5f;  warmSus[14] = 0.7f;
    warmSus[15] = 0.7f;  warmSus[16] = 0.6f;  warmSus[17] = 0.4f;
    warmSus[18] = 0.2f;

    // Flat (neutral) curve state — WR-02: curve-less presets must reset the curves to
    // flat, otherwise the previously-loaded preset's curves persist. Giving these
    // presets an explicit all-zero customState makes "no shaping" the applied result.
    std::array<float, 32> flatCurve {};

    // CR-02: factory preset parameter values below are authored in ENGINEERING UNITS
    // (ATTACK_TIME/SUSTAIN_TIME/LOOKAHEAD_TIME in ms, OUTPUT_GAIN in dB, MIX/SENSITIVITY
    // as 0-1 fractions, LOOKAHEAD_ENABLED as 0/1). They are converted to normalised
    // (0-1) via each parameter's own NormalisableRange in the loop below, so the 0.3
    // skew on ATTACK_TIME/SUSTAIN_TIME is applied correctly. (Previously these were
    // authored as linear fractions that ignored the skew, so every preset recalled a
    // time ~10-30x too short and "Default" did not match the plugin's power-on state.)
    // v1.6.0 bank additions — authored with the sparse `curve` helper above.
    // Every run of listed bands is contiguous (see the authoring note).

    // Extra Snap: broad gentle attack lift through the mids and highs
    std::array<float, 32> snapAtk = curve({
        {10, 0.15f}, {11, 0.20f}, {12, 0.22f}, {13, 0.25f}, {14, 0.28f},
        {15, 0.30f}, {16, 0.32f}, {17, 0.34f}, {18, 0.35f}, {19, 0.35f},
        {20, 0.35f}, {21, 0.35f}, {22, 0.34f}, {23, 0.32f}, {24, 0.30f},
        {25, 0.28f}, {26, 0.25f}, {27, 0.22f}, {28, 0.18f}});

    // Kick Tightener: boost attack 40-90Hz, cut sustain 50-160Hz
    std::array<float, 32> kickAtk = curve({
        {3, 0.3f}, {4, 0.6f}, {5, 0.8f}, {6, 0.8f}, {7, 0.5f}, {8, 0.25f}});
    std::array<float, 32> kickSus = curve({
        {4, -0.2f}, {5, -0.4f}, {6, -0.5f}, {7, -0.6f}, {8, -0.6f},
        {9, -0.5f}, {10, -0.3f}});

    // Snare Crack: boost attack at the 150-300Hz knock and the 1.5-5kHz crack,
    // clear the 300-650Hz ring out of the sustain
    std::array<float, 32> snareAtk = curve({
        {10, 0.4f}, {11, 0.55f}, {12, 0.6f}, {13, 0.45f}, {14, 0.2f},
        {21, 0.2f}, {22, 0.35f}, {23, 0.5f}, {24, 0.6f}, {25, 0.55f},
        {26, 0.4f}, {27, 0.2f}});
    std::array<float, 32> snareSus = curve({
        {13, -0.2f}, {14, -0.35f}, {15, -0.4f}, {16, -0.35f}, {17, -0.2f}});

    // Tom Focus: boost attack 80-250Hz, tighten the 100-350Hz ring
    std::array<float, 32> tomAtk = curve({
        {7, 0.3f}, {8, 0.55f}, {9, 0.7f}, {10, 0.7f}, {11, 0.5f}, {12, 0.3f}});
    std::array<float, 32> tomSus = curve({
        {8, -0.25f}, {9, -0.4f}, {10, -0.5f}, {11, -0.5f}, {12, -0.45f},
        {13, -0.35f}, {14, -0.2f}});

    // Room Tamer: pull the sustained midrange wash out of a live drum room
    std::array<float, 32> roomSus = curve({
        {9, -0.25f}, {10, -0.4f}, {11, -0.5f}, {12, -0.55f}, {13, -0.6f},
        {14, -0.6f}, {15, -0.6f}, {16, -0.6f}, {17, -0.6f}, {18, -0.55f},
        {19, -0.5f}, {20, -0.45f}, {21, -0.4f}, {22, -0.3f}, {23, -0.2f}});

    // Percussion Sparkle: attack lift 3-16kHz for shakers, tambourines, congas
    std::array<float, 32> sparkAtk = curve({
        {25, 0.2f}, {26, 0.35f}, {27, 0.5f}, {28, 0.6f}, {29, 0.6f},
        {30, 0.5f}, {31, 0.4f}});

    // Hat De-Harsh: soften the 2.5-7.5kHz attack spike without dulling the top
    std::array<float, 32> hatAtk = curve({
        {24, -0.2f}, {25, -0.4f}, {26, -0.55f}, {27, -0.55f}, {28, -0.4f},
        {29, -0.2f}});
    std::array<float, 32> hatSus = curve({
        {26, -0.15f}, {27, -0.25f}, {28, -0.25f}, {29, -0.15f}});

    // Shimmer Sustain: lift the 5-16kHz+ wash cymbal tails ride on
    std::array<float, 32> shimmerSus = curve({
        {27, 0.2f}, {28, 0.4f}, {29, 0.55f}, {30, 0.6f}, {31, 0.6f}});

    // Plosive Guard: kill the sub-100Hz thump a P/B pop puts into a vocal take
    std::array<float, 32> plosiveAtk = curve({
        {0, -0.5f}, {1, -0.6f}, {2, -0.7f}, {3, -0.7f}, {4, -0.6f},
        {5, -0.45f}, {6, -0.3f}, {7, -0.15f}});
    std::array<float, 32> plosiveSus = curve({
        {0, -0.25f}, {1, -0.3f}, {2, -0.3f}, {3, -0.25f}, {4, -0.15f}});

    // Vocal Presence: sharpen consonant onsets around 1-3kHz
    std::array<float, 32> presenceAtk = curve({
        {19, 0.25f}, {20, 0.4f}, {21, 0.5f}, {22, 0.5f}, {23, 0.4f},
        {24, 0.25f}});

    // Breath & Air: lift the breathy 6-16kHz sustain, soften spitty onsets there
    std::array<float, 32> breathAtk = curve({
        {29, -0.2f}, {30, -0.25f}, {31, -0.25f}});
    std::array<float, 32> breathSus = curve({
        {28, 0.3f}, {29, 0.45f}, {30, 0.55f}, {31, 0.55f}});

    // Strum Snap: pick transient 500Hz-2kHz up, 230-520Hz body mud down
    std::array<float, 32> strumAtk = curve({
        {16, 0.25f}, {17, 0.4f}, {18, 0.5f}, {19, 0.5f}, {20, 0.4f},
        {21, 0.3f}, {22, 0.2f}});
    std::array<float, 32> strumSus = curve({
        {12, -0.2f}, {13, -0.3f}, {14, -0.3f}, {15, -0.25f}, {16, -0.15f}});

    // Piano Hammer: hammer strike from the 280Hz-800Hz knock up through 3kHz
    std::array<float, 32> pianoAtk = curve({
        {13, 0.25f}, {14, 0.4f}, {15, 0.5f}, {16, 0.5f}, {17, 0.45f},
        {18, 0.4f}, {19, 0.35f}, {20, 0.3f}, {21, 0.3f}, {22, 0.3f},
        {23, 0.3f}, {24, 0.25f}, {25, 0.2f}});

    // Bass Definition: 55-160Hz fundamental punch plus 650Hz-1.5kHz string
    // click, with the 40-90Hz boom tightened out of the sustain
    std::array<float, 32> bassAtk = curve({
        {5, 0.3f}, {6, 0.5f}, {7, 0.6f}, {8, 0.6f}, {9, 0.5f}, {10, 0.3f},
        {16, 0.1f}, {17, 0.2f}, {18, 0.3f}, {19, 0.3f}, {20, 0.2f}, {21, 0.1f}});
    std::array<float, 32> bassSus = curve({
        {3, -0.2f}, {4, -0.3f}, {5, -0.35f}, {6, -0.3f}, {7, -0.2f}});

    // String Swell: erase bow/pick onsets through the mids, lift the sustain
    std::array<float, 32> swellAtk = curve({
        {10, -0.2f}, {11, -0.3f}, {12, -0.4f}, {13, -0.45f}, {14, -0.5f},
        {15, -0.5f}, {16, -0.5f}, {17, -0.5f}, {18, -0.5f}, {19, -0.5f},
        {20, -0.5f}, {21, -0.5f}, {22, -0.45f}, {23, -0.4f}, {24, -0.35f},
        {25, -0.3f}, {26, -0.2f}});
    std::array<float, 32> swellSus = curve({
        {12, 0.2f}, {13, 0.25f}, {14, 0.3f}, {15, 0.3f}, {16, 0.3f},
        {17, 0.3f}, {18, 0.3f}, {19, 0.3f}, {20, 0.3f}, {21, 0.3f},
        {22, 0.25f}, {23, 0.2f}, {24, 0.15f}});

    // Pick Bite: electric guitar pick definition 1.2-3.3kHz
    std::array<float, 32> pickAtk = curve({
        {20, 0.3f}, {21, 0.45f}, {22, 0.55f}, {23, 0.55f}, {24, 0.45f},
        {25, 0.3f}});
    std::array<float, 32> pickSus = curve({
        {19, -0.15f}, {20, -0.2f}, {21, -0.2f}, {22, -0.2f}, {23, -0.15f}});

    // Low-End Tightener: bus-level low-frequency sustain control
    std::array<float, 32> tightAtk = curve({
        {5, 0.1f}, {6, 0.15f}, {7, 0.15f}, {8, 0.1f}});
    std::array<float, 32> tightSus = curve({
        {2, -0.15f}, {3, -0.2f}, {4, -0.25f}, {5, -0.25f}, {6, -0.25f},
        {7, -0.2f}, {8, -0.15f}, {9, -0.1f}});

    // Master Polish: a whisper of attack definition and top-octave air
    std::array<float, 32> polishAtk = curve({
        {21, 0.1f}, {22, 0.15f}, {23, 0.15f}, {24, 0.15f}, {25, 0.15f},
        {26, 0.15f}, {27, 0.1f}});
    std::array<float, 32> polishSus = curve({
        {28, 0.1f}, {29, 0.15f}, {30, 0.15f}, {31, 0.15f}});

    // Attack Eraser: flatten every onset, let the sustain carry the note
    std::array<float, 32> eraserAtk;  eraserAtk.fill(-0.85f);
    std::array<float, 32> eraserSus;  eraserSus.fill(0.2f);

    // Infinite Bloom: swallow attacks whole, maximum sustain everywhere
    std::array<float, 32> bloomAtk;   bloomAtk.fill(-0.3f);
    std::array<float, 32> bloomSus;   bloomSus.fill(0.6f);

    // The bank is DECLARED in narrative category order — the categorySpans
    // table below indexes into this vector, so a preset added here must also
    // extend (or add) a span, and the tiling assert catches a miss.
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        // ── Essentials ──────────────────────────────────────────────────────
        {
            "Default",
            {{"MIX", 1.0f}, {"ATTACK_TIME", 10.0f}, {"SUSTAIN_TIME", 100.0f},
             {"SENSITIVITY", 0.5f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(flatCurve, flatCurve)
        },
        {
            "Gentle Shaping",
            {{"MIX", 0.50f}, {"ATTACK_TIME", 12.5f}, {"SUSTAIN_TIME", 157.0f},
             {"SENSITIVITY", 0.35f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(flatCurve, flatCurve)
        },
        {
            "Extra Snap",
            {{"MIX", 0.60f}, {"ATTACK_TIME", 8.0f}, {"SUSTAIN_TIME", 120.0f},
             {"SENSITIVITY", 0.5f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(snapAtk, flatCurve)
        },
        {
            "Transient Tamer",
            {{"MIX", 0.75f}, {"ATTACK_TIME", 2.5f}, {"SUSTAIN_TIME", 59.0f},
             {"SENSITIVITY", 0.7f}, {"LOOKAHEAD_ENABLED", 1.0f},
             {"LOOKAHEAD_TIME", 3.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(tamerAtk, tamerSus)
        },
        // ── Drums & Percussion ──────────────────────────────────────────────
        {
            "Punch Enhancer",
            {{"MIX", 0.85f}, {"ATTACK_TIME", 15.0f}, {"SUSTAIN_TIME", 35.0f},
             {"SENSITIVITY", 0.6f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 1.0f}},
            makeCurveState(punchAtk, punchSus)
        },
        {
            "Kick Tightener",
            {{"MIX", 0.85f}, {"ATTACK_TIME", 6.0f}, {"SUSTAIN_TIME", 80.0f},
             {"SENSITIVITY", 0.6f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.5f}},
            makeCurveState(kickAtk, kickSus)
        },
        {
            "Snare Crack",
            {{"MIX", 0.80f}, {"ATTACK_TIME", 4.0f}, {"SUSTAIN_TIME", 60.0f},
             {"SENSITIVITY", 0.65f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(snareAtk, snareSus)
        },
        {
            "Tom Focus",
            {{"MIX", 0.80f}, {"ATTACK_TIME", 8.0f}, {"SUSTAIN_TIME", 90.0f},
             {"SENSITIVITY", 0.55f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(tomAtk, tomSus)
        },
        {
            "Room Tamer",
            {{"MIX", 0.75f}, {"ATTACK_TIME", 10.0f}, {"SUSTAIN_TIME", 220.0f},
             {"SENSITIVITY", 0.5f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.5f}},
            makeCurveState(flatCurve, roomSus)
        },
        {
            "Percussion Sparkle",
            {{"MIX", 0.70f}, {"ATTACK_TIME", 3.0f}, {"SUSTAIN_TIME", 50.0f},
             {"SENSITIVITY", 0.6f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(sparkAtk, flatCurve)
        },
        // ── Cymbals & Air ───────────────────────────────────────────────────
        {
            "Cymbal Control",
            {{"MIX", 0.75f}, {"ATTACK_TIME", 4.0f}, {"SUSTAIN_TIME", 49.0f},
             {"SENSITIVITY", 0.70f}, {"LOOKAHEAD_ENABLED", 1.0f},
             {"LOOKAHEAD_TIME", 4.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(cymbAtk, cymbSus)
        },
        {
            "Hat De-Harsh",
            {{"MIX", 0.75f}, {"ATTACK_TIME", 3.0f}, {"SUSTAIN_TIME", 70.0f},
             {"SENSITIVITY", 0.65f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(hatAtk, hatSus)
        },
        {
            "Shimmer Sustain",
            {{"MIX", 0.65f}, {"ATTACK_TIME", 12.0f}, {"SUSTAIN_TIME", 300.0f},
             {"SENSITIVITY", 0.4f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(flatCurve, shimmerSus)
        },
        // ── Vocals & Speech ─────────────────────────────────────────────────
        {
            "De-Esser",
            {{"MIX", 0.80f}, {"ATTACK_TIME", 12.5f}, {"SUSTAIN_TIME", 108.0f},
             {"SENSITIVITY", 0.60f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(deessAtk, deessSus)
        },
        {
            "Plosive Guard",
            {{"MIX", 0.85f}, {"ATTACK_TIME", 5.0f}, {"SUSTAIN_TIME", 60.0f},
             {"SENSITIVITY", 0.6f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(plosiveAtk, plosiveSus)
        },
        {
            "Vocal Presence",
            {{"MIX", 0.70f}, {"ATTACK_TIME", 12.0f}, {"SUSTAIN_TIME", 120.0f},
             {"SENSITIVITY", 0.45f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(presenceAtk, flatCurve)
        },
        {
            "Breath & Air",
            {{"MIX", 0.60f}, {"ATTACK_TIME", 15.0f}, {"SUSTAIN_TIME", 250.0f},
             {"SENSITIVITY", 0.4f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(breathAtk, breathSus)
        },
        // ── Instruments ─────────────────────────────────────────────────────
        {
            "Strum Snap",
            {{"MIX", 0.75f}, {"ATTACK_TIME", 6.0f}, {"SUSTAIN_TIME", 100.0f},
             {"SENSITIVITY", 0.55f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(strumAtk, strumSus)
        },
        {
            "Piano Hammer",
            {{"MIX", 0.70f}, {"ATTACK_TIME", 8.0f}, {"SUSTAIN_TIME", 150.0f},
             {"SENSITIVITY", 0.5f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(pianoAtk, flatCurve)
        },
        {
            "Bass Definition",
            {{"MIX", 0.80f}, {"ATTACK_TIME", 10.0f}, {"SUSTAIN_TIME", 130.0f},
             {"SENSITIVITY", 0.5f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(bassAtk, bassSus)
        },
        {
            "String Swell",
            {{"MIX", 0.70f}, {"ATTACK_TIME", 20.0f}, {"SUSTAIN_TIME", 350.0f},
             {"SENSITIVITY", 0.4f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.5f}},
            makeCurveState(swellAtk, swellSus)
        },
        {
            "Pick Bite",
            {{"MIX", 0.75f}, {"ATTACK_TIME", 5.0f}, {"SUSTAIN_TIME", 90.0f},
             {"SENSITIVITY", 0.55f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(pickAtk, pickSus)
        },
        // ── Mix & Master ────────────────────────────────────────────────────
        {
            "Warm Sustain",
            {{"MIX", 0.65f}, {"ATTACK_TIME", 15.0f}, {"SUSTAIN_TIME", 230.0f},
             {"SENSITIVITY", 0.40f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.5f}},
            makeCurveState(warmAtk, warmSus)
        },
        {
            "Sustain Lift",
            {{"MIX", 0.70f}, {"ATTACK_TIME", 20.0f}, {"SUSTAIN_TIME", 304.0f},
             {"SENSITIVITY", 0.45f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.5f}},
            makeCurveState(flatCurve, flatCurve)
        },
        {
            "Low-End Tightener",
            {{"MIX", 0.50f}, {"ATTACK_TIME", 15.0f}, {"SUSTAIN_TIME", 180.0f},
             {"SENSITIVITY", 0.45f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(tightAtk, tightSus)
        },
        {
            "Master Polish",
            {{"MIX", 0.35f}, {"ATTACK_TIME", 12.0f}, {"SUSTAIN_TIME", 150.0f},
             {"SENSITIVITY", 0.45f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 0.0f}},
            makeCurveState(polishAtk, polishSus)
        },
        // ── Creative ────────────────────────────────────────────────────────
        {
            "Aggressive Bite",
            {{"MIX", 1.0f}, {"ATTACK_TIME", 5.0f}, {"SUSTAIN_TIME", 20.0f},
             {"SENSITIVITY", 0.85f}, {"LOOKAHEAD_ENABLED", 1.0f},
             {"LOOKAHEAD_TIME", 5.0f}, {"OUTPUT_GAIN", -1.0f}},
            makeCurveState(flatCurve, flatCurve)
        },
        {
            "Attack Eraser",
            {{"MIX", 1.0f}, {"ATTACK_TIME", 2.0f}, {"SUSTAIN_TIME", 300.0f},
             {"SENSITIVITY", 0.7f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", 1.0f}},
            makeCurveState(eraserAtk, eraserSus)
        },
        {
            "Infinite Bloom",
            {{"MIX", 0.80f}, {"ATTACK_TIME", 25.0f}, {"SUSTAIN_TIME", 500.0f},
             {"SENSITIVITY", 0.35f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 2.0f}, {"OUTPUT_GAIN", -1.0f}},
            makeCurveState(bloomAtk, bloomSus)
        }
    };

    // ── Narrative categories for the preset menu (v1.6.0) ───────────────────
    // Expressed as index SPANS over factoryPresets, never as a second list of
    // names: a repeated name literal goes stale the first time a preset is
    // renamed and fails SILENTLY — the preset just falls into "User" beside
    // the user's own saves (pattern_test_fixture_mirrors_drift_silently).
    // Spans are inclusive [first, last] and must tile [0, size) exactly; the
    // loop below asserts it.
    struct CategorySpan { const char* label; int first, last; };
    static constexpr CategorySpan categorySpans[] {
        { "Essentials",          0,  3 },
        { "Drums & Percussion",  4,  9 },
        { "Cymbals & Air",      10, 12 },
        { "Vocals & Speech",    13, 16 },
        { "Instruments",        17, 21 },
        { "Mix & Master",       22, 25 },
        { "Creative",           26, 28 },
    };

    {
        int expectedFirst = 0;
        for (const auto& span : categorySpans)
        {
            jassert (span.first == expectedFirst);              // spans must tile,
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

    // CR-02: convert engineering-unit values to normalised (0-1) through each
    // parameter's NormalisableRange. This handles the ATTACK_TIME/SUSTAIN_TIME skew
    // once, correctly — initializeFactoryPresets stores these verbatim as the preset's
    // normalised value, and applyPresetJson feeds them back through convertFrom0to1.
    for (auto& preset : factoryPresets)
        for (auto& [paramId, value] : preset.parameters)
            if (auto* p = parameters.getParameter(paramId))
                value = p->convertTo0to1(value);

    presetManager.initializeFactoryPresets(factoryPresets);

    // Cache parameter pointers (avoids string hash lookup 7x per processBlock)
    cachedMix = parameters.getRawParameterValue("MIX");
    cachedSensitivity = parameters.getRawParameterValue("SENSITIVITY");
    cachedAttackTime = parameters.getRawParameterValue("ATTACK_TIME");
    cachedSustainTime = parameters.getRawParameterValue("SUSTAIN_TIME");
    cachedLookaheadEnabled = parameters.getRawParameterValue("LOOKAHEAD_ENABLED");
    cachedLookaheadTime = parameters.getRawParameterValue("LOOKAHEAD_TIME");
    cachedOutputGain = parameters.getRawParameterValue("OUTPUT_GAIN");
}

OSpectralShaperAudioProcessor::~OSpectralShaperAudioProcessor()
{
}

// ============================================================================
// Audio Processing
// ============================================================================

void OSpectralShaperAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Report base latency (FFT size); re-signalled only when lookahead changes it (WR-03)
    setLatencySamples(STFTProcessor::FFT_SIZE);
    lastReportedLatency = STFTProcessor::FFT_SIZE;

    // Prepare STFT processors (one per channel)
    for (int ch = 0; ch < 2; ++ch)
    {
        stftProcessor[ch].prepare(sampleRate);
        stftProcessor[ch].reset();
    }

    // Preallocate dry delay buffer (FFT_SIZE samples for latency matching)
    dryDelayBuffer.setSize(2, STFTProcessor::FFT_SIZE);
    dryDelayBuffer.clear();
    dryDelayWritePosition = 0;

    // Preallocate lookahead buffer (max 10ms @ highest sample rate)
    int maxLookaheadSamples = static_cast<int>(sampleRate * 0.010);  // 10ms
    lookaheadBuffer.setSize(2, maxLookaheadSamples);
    lookaheadBuffer.clear();
    lookaheadWritePosition = 0;
    lookaheadDelayLength = 0;

    juce::ignoreUnused(samplesPerBlock);
}

void OSpectralShaperAudioProcessor::releaseResources()
{
    // Optional: Release buffers to save memory
    dryDelayBuffer.setSize(0, 0);
    lookaheadBuffer.setSize(0, 0);
}

void OSpectralShaperAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Edge case: zero-length buffer
    if (buffer.getNumSamples() == 0)
        return;

    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);  // Stereo only
    const int numSamples = buffer.getNumSamples();

    // Read parameters (atomic, real-time safe — pointers cached in constructor)
    float mixValue = cachedMix->load();
    float sensitivity = cachedSensitivity->load();
    float attackTime = cachedAttackTime->load();
    float sustainTime = cachedSustainTime->load();
    lookaheadEnabled = cachedLookaheadEnabled->load() > 0.5f;
    float lookaheadTimeMs = cachedLookaheadTime->load();
    float outputGainDB = cachedOutputGain->load();
    float outputGain = juce::Decibels::decibelsToGain(outputGainDB);

    // Calculate lookahead delay length and update reported latency
    if (lookaheadEnabled)
    {
        lookaheadDelayLength = static_cast<int>(currentSampleRate * lookaheadTimeMs / 1000.0);
        lookaheadDelayLength = juce::jmin(lookaheadDelayLength, lookaheadBuffer.getNumSamples());
    }
    else
    {
        lookaheadDelayLength = 0;
    }
    // WR-03: only re-signal latency when it actually changes, not every block. Reporting a
    // changed latency from the audio thread every block makes hosts glitch or ignore it.
    // (The Lookahead control itself remains a deferred no-op — see CHANGELOG v1.3.2.)
    const int desiredLatency = STFTProcessor::FFT_SIZE + lookaheadDelayLength;
    if (desiredLatency != lastReportedLatency)
    {
        setLatencySamples(desiredLatency);
        lastReportedLatency = desiredLatency;
    }

    // Update STFT parameters
    for (int ch = 0; ch < numChannels; ++ch)
    {
        stftProcessor[ch].setSensitivity(sensitivity);
        stftProcessor[ch].setAttackTime(attackTime);
        stftProcessor[ch].setSustainTime(sustainTime);
    }

    // Process sample-by-sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float input = buffer.getSample(ch, sample);

            // Optional lookahead delay (reduces pre-ringing on sharp transients)
            float lookaheadInput = getLookaheadDelayedSample(ch, input);

            // Dry path (latency-matched to STFT processing)
            float dry = getDryDelayedSample(ch, lookaheadInput);

            // Wet path (STFT processing)
            float wet = stftProcessor[ch].processSample(lookaheadInput);

            // Mix and output gain
            float output = (dry * (1.0f - mixValue) + wet * mixValue) * outputGain;
            buffer.setSample(ch, sample, output);
        }

        advanceDryDelay();
        advanceLookahead();

        // Phase 3.3: Push visualization data once per FFT hop
        if (++hopCounter >= STFTProcessor::HOP_SIZE)
        {
            hopCounter = 0;

            // Build visualization frame from left channel (mono visualization)
            VisualizationFrame frame;
            frame.fftMagnitudes = stftProcessor[0].getLastMagnitudes();
            frame.transientActivity = stftProcessor[0].getTransientActivity();

            writeVisualizationFrame(frame);
        }
    }

    // Clear unused channels
    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);
}

// ============================================================================
// Editor
// ============================================================================

juce::AudioProcessorEditor* OSpectralShaperAudioProcessor::createEditor()
{
    return new OSpectralShaperAudioProcessorEditor(*this);
}

// ============================================================================
// State Management
// ============================================================================

void OSpectralShaperAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = presetManager.getStateAsXml())
    {
        // v1.5.0: the tooltip preference is stamped onto the session XML *after*
        // the preset manager has built it, so it rides along with the host
        // session but never enters a saved preset file.
        xml->setAttribute("tooltipsEnabled", tooltipsEnabled.load(std::memory_order_acquire));
        copyXmlToBinary(*xml, destData);
    }
}

void OSpectralShaperAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        presetManager.setStateFromXml(xml.get());

        // v1.5.0: absent in sessions saved before 1.5.0 — default to off.
        tooltipsEnabled.store(xml->getBoolAttribute("tooltipsEnabled", false),
                              std::memory_order_release);
    }
}

// ============================================================================
// Curve Access Methods
// ============================================================================

void OSpectralShaperAudioProcessor::setAttackCurve(const std::array<float, 32>& curve)
{
    attackCurve = curve;
    // Update STFT processors
    for (int ch = 0; ch < 2; ++ch)
        stftProcessor[ch].setAttackCurve(curve);
}

void OSpectralShaperAudioProcessor::setSustainCurve(const std::array<float, 32>& curve)
{
    sustainCurve = curve;
    // Update STFT processors
    for (int ch = 0; ch < 2; ++ch)
        stftProcessor[ch].setSustainCurve(curve);
}

// ============================================================================
// Dry Delay Buffer Helpers
// ============================================================================

float OSpectralShaperAudioProcessor::getDryDelayedSample(int channel, float input)
{
    // Write current input to delay buffer
    dryDelayBuffer.setSample(channel, dryDelayWritePosition, input);

    // Read delayed sample (FFT_SIZE samples ago for latency matching)
    int readPosition = (dryDelayWritePosition + 1) % STFTProcessor::FFT_SIZE;
    return dryDelayBuffer.getSample(channel, readPosition);
}

void OSpectralShaperAudioProcessor::advanceDryDelay()
{
    dryDelayWritePosition = (dryDelayWritePosition + 1) % STFTProcessor::FFT_SIZE;
}

float OSpectralShaperAudioProcessor::getLookaheadDelayedSample(int channel, float input)
{
    if (lookaheadDelayLength == 0)
        return input;

    lookaheadBuffer.setSample(channel, lookaheadWritePosition, input);

    int readPosition = (lookaheadWritePosition - lookaheadDelayLength + lookaheadBuffer.getNumSamples())
                       % lookaheadBuffer.getNumSamples();
    return lookaheadBuffer.getSample(channel, readPosition);
}

void OSpectralShaperAudioProcessor::advanceLookahead()
{
    if (lookaheadDelayLength > 0)
        lookaheadWritePosition = (lookaheadWritePosition + 1) % lookaheadBuffer.getNumSamples();
}

void OSpectralShaperAudioProcessor::writeVisualizationFrame(const VisualizationFrame& frame)
{
    // Write to FIFO if space available (audio thread, lock-free)
    if (visualizationFifo.getFreeSpace() > 0)
    {
        int start1, size1, start2, size2;
        visualizationFifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0)
        {
            visualizationBuffer[static_cast<size_t>(start1)] = frame;
        }

        visualizationFifo.finishedWrite(size1);
    }
}

// ============================================================================
// Factory Function
// ============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSpectralShaperAudioProcessor();
}
