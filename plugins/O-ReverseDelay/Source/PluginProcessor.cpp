/*
   This file is part of O-ReverseDelay, an Ouaricon Audio plugin.
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

ReverseDelayProcessor::ReverseDelayProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache raw parameter atomics once — read per block on the audio thread.
    pDelayTime    = parameters.getRawParameterValue("delayTime");
    pSyncMode     = parameters.getRawParameterValue("syncMode");
    pNoteDivision = parameters.getRawParameterValue("noteDivision");
    pGrainSize    = parameters.getRawParameterValue("grainSize");
    pGrainLink    = parameters.getRawParameterValue("grainLink");       // v1.17.0
    pGrainDivision = parameters.getRawParameterValue("grainDivision");  // v1.17.0
    pDensity      = parameters.getRawParameterValue("density");
    pFeedback     = parameters.getRawParameterValue("feedback");
    pLowCut       = parameters.getRawParameterValue("lowCut");
    pHighCut      = parameters.getRawParameterValue("highCut");
    pWidth        = parameters.getRawParameterValue("width");
    pMix          = parameters.getRawParameterValue("mix");

    // v1.1.0 grain randomisation (B3).
    pJitter       = parameters.getRawParameterValue("jitter");
    pDelayScatter = parameters.getRawParameterValue("delayScatter");
    pSizeRandom   = parameters.getRawParameterValue("sizeRandom");
    pGainRandom   = parameters.getRawParameterValue("gainRandom");

    // v1.2.0 grain window (B1).
    pGrainTilt    = parameters.getRawParameterValue("grainTilt");
    pGrainShape   = parameters.getRawParameterValue("grainShape");

    // v1.3.0 grain count (B2).
    pGrainCount   = parameters.getRawParameterValue("grainCount");

    // v1.4.0 Tukey taper.
    pTukeyTaper   = parameters.getRawParameterValue("tukeyTaper");

    // v1.6.0 MOTION (B4 #1-#3).
    pFreeze       = parameters.getRawParameterValue("freeze");
    pDirection    = parameters.getRawParameterValue("direction");
    pRegenMakeup  = parameters.getRawParameterValue("regenMakeup");
    pFreezeLength = parameters.getRawParameterValue("freezeLength");   // v1.15.0

    // v1.7.0 SOURCE / DUCK / DRIFT (B4 #4-#6).
    pSourceMode   = parameters.getRawParameterValue("sourceMode");
    pDuck         = parameters.getRawParameterValue("duck");
    pDriftRate    = parameters.getRawParameterValue("driftRate");
    pDriftDepth   = parameters.getRawParameterValue("driftDepth");

    pDiffusion    = parameters.getRawParameterValue("diffusion");
    pDrive        = parameters.getRawParameterValue("drive");

    // ── v1.7.3 (IN-06): one guard posture, asserted once, here ──────────────
    // getRawParameterValue returns nullptr for an id that is not in the layout,
    // which is only ever a TYPO in createParameterLayout() or in the string
    // above — never a runtime condition. Asserting it once at construction is
    // therefore strictly better than guarding at every use site:
    //
    //   - a typo fails LOUDLY here, in debug, on the first instantiation;
    //   - reset() and prepareToPlay() can then dereference identically.
    //
    // Before this, they disagreed — reset() guarded every pointer and
    // prepareToPlay dereferenced the same ones bare. That disagreement picked
    // the WRONG direction: a typo'd id made reset() a silent no-op (a smoother
    // stuck at its default forever, with no diagnostic) and prepareToPlay a
    // null dereference (a release crash on the host's first prepare). One
    // symptom hides the bug, the other reports it far from its cause.
    //
    // jassert compiles out of release, so this costs nothing shipped. The
    // release posture is deliberate: if an id ever did go missing past a debug
    // run, crashing at prepare is more discoverable than a silently dead
    // control, and ui_frontend_check.js already diffs the relay lists against
    // the APVTS in both directions on every build.
    for (auto* p : { pDelayTime, pSyncMode, pNoteDivision, pGrainSize, pDensity,
                     pFeedback, pLowCut, pHighCut, pWidth, pMix,
                     pJitter, pDelayScatter, pSizeRandom, pGainRandom,
                     pGrainTilt, pGrainShape, pGrainCount, pTukeyTaper,
                     pFreeze, pDirection, pRegenMakeup, pFreezeLength,
                     pSourceMode, pDuck, pDriftRate, pDriftDepth,
                     pGrainLink, pGrainDivision })
    {
        jassert (p != nullptr);   // id typo in createParameterLayout() or above
        juce::ignoreUnused (p);
    }

    // ── Stage 4 (D16): 8 factory presets ────────────────────────────────────
    // Authored in ENGINEERING UNITS (ms, %, Hz, choice index) and converted once
    // through each parameter's own NormalisableRange below. Four params are
    // skewed (delayTime centre 316 ms, grainSize 158 ms, lowCut 200 Hz, highCut
    // 3162 Hz); a hand-written normalised fraction on any of them recalls 10–30×
    // wrong (pattern_factory_preset_normalized_ignores_skew).
    //
    // All thirty keys are explicit in every preset. Omitted keys would
    // revert to the APVTS default (applyPresetJson resets everything first),
    // which is safe but makes the table's intent unreadable.
    //
    // ── v1.7.0 (B4 #4-#6): sourceMode / duck / driftRate / driftDepth ────────
    // Three of the four are pinned at 0 and the fourth — driftRate — is pinned
    // at its own default of 0.30 Hz, which is inert while driftDepth is 0. That
    // asymmetry is deliberate and is the trap this block carries: a rate written
    // as 0 would be CLAMPED up to kDriftRateMinHz by the NormalisableRange, so
    // the preset would recall 0.02 Hz rather than the default and probe N's
    // round-trip comparison would fail against a table that looked correct.
    //
    // sourceMode carries the grainShape trap rather than the freeze one: its
    // no-op is index 0 (Mono Sum), and 0 is also what an absent key resolves to,
    // so the ORDER of the choice list is load-bearing. Putting Stereo first
    // would re-voice every existing session the moment v1.7.0 is installed.
    //
    // ── v1.8.0 (B4 #7-#8): diffusion / drive both pinned at 0 ───────────────
    // Sixth release running that adds keys here and pins every one to the no-op.
    // Both are plain zero, so this block carries neither the grainTilt trap nor
    // the driftRate one — but pinning them is doing real work even so, because
    // these are the two keys most obviously "free" to author into the presets.
    //
    // "Reverse Bloom" at diffusion 40 would be a better patch. It would also be
    // a DIFFERENT patch than the one shipped since v1.0.0, and re-voicing a
    // sound people have built work on is the thing this table has refused five
    // times now (pattern_activating_dead_param_default_timbre). A v1.8.0 preset
    // that wants diffusion is a NEW entry, not an edit to an existing one.
    //
    // ── v1.6.0 (B4): freeze / direction / regenMakeup all pinned at 0 ────────
    // Fifth release running that adds keys here and pins every one of them to
    // the no-op, and the FIRST where all three no-ops are plain zero — so this
    // block is the one that does NOT carry the trap the four above it do.
    //
    // regenMakeup deserves a sentence of its own, because it is the one an
    // improvement pass is most tempted to move. The review's motivation for
    // exposing it is precisely that "Near-Infinite" cannot self-sustain at
    // feedback 100 — the topology loses ≈7.3 dB per generation — so the obvious
    // reflex is to author that preset at +7 dB and finally make its name true.
    // Deliberately NOT done: "Near-Infinite" is a shipped sound that people have
    // used since v1.0.0, and quietly turning it into a self-oscillator would
    // re-voice their work (pattern_activating_dead_param_default_timbre). The
    // preset stays exactly what it was; true sustain is now one knob away, which
    // is what the review actually asked for.
    //
    // ── v1.3.0 (B2): grainCount pinned at 8 ─────────────────────────────────
    // 8 is v1.2.0's hard-coded overlap ceiling, so every preset below keeps the
    // exact overlap it was authored for. Same trap as grainTilt one paragraph
    // down and worth naming separately, because the wrong reflex here is the
    // OTHER one: "new ceiling, use the new maximum" would push all eight presets
    // to overlap 16 and make every factory sound roughly twice as dense.
    //
    // Note this is also why the v1.0.1 density re-authoring survives untouched:
    // those values encode overlaps against a ceiling of 8, and the ceiling is
    // now written down next to them rather than implied by the engine.
    //
    // ── v1.2.0 (B1): grainTilt pinned at 0.5, grainShape at 0 (Hann) ────────
    // Same reasoning as the v1.1 block below, with one extra trap: the no-op
    // value for grainTilt is 0.5, NOT 0. A reflex "new key, write 0" here would
    // hard-tilt all eight factory presets to a peak-early window and change what
    // "Reverse Bloom" sounds like for everyone already using it.
    //
    // ── v1.1.0 (B3): the four randomisation keys are pinned at 0 ────────────
    // Not an oversight. These presets are the shipped v1.0 sound, and a factory
    // preset that quietly switched on grain randomisation would change what
    // "Reverse Bloom" means for everyone who already uses it
    // (pattern_activating_dead_param_default_timbre). They are written
    // explicitly rather than left to the default so that intent is on the page:
    // v1.1 presets are deliberately unchanged, and a future release that DOES
    // want a randomised preset edits a visible number here.
    //
    // No "/" in any name — OuariconPresetManager sanitises it to "_", so
    // "Reverse 1/8" would round-trip as "Reverse 1_8"
    // (critical_preset_name_slash_path_separator). Hence "Rhythmic Reverse".
    //
    // syncMode: 0 = Free, 1 = Sync. noteDivision: 4 = 1/8D, 6 = 1/4 (the default).
    //
    // ── v1.0.1 (A3): density values re-authored ─────────────────────────────
    // density no longer means what it meant at v1.0.0. The old map was
    // overlap = 1 + d·7; the new one is overlap = 2 + d·6. Every preset's
    // density is therefore rewritten to the value that reproduces its SHIPPED
    // overlap exactly:
    //     d_new = (7·d_old − 100) / 6
    // so all eight presets render bit-identically to v1.0.0 (the 0.1 % density
    // step resolves each of these exactly). Only the knob's *scale* moved; the
    // presets did not. Old -> new: 60->53.3, 55->47.5, 70->65, 30->18.3,
    // 90->88.3, 65->59.2, 80->76.7.
    //
    // ── v1.12.4: the no-op tail is written ONCE ─────────────────────────────
    // Every key added since v1.1.0 is pinned to its no-op in every preset, so
    // the pins live in kShippedNoOpTail below rather than as eight copies. The
    // rows keep only the ten v1.0 keys that genuinely differ per preset. The
    // tail is merged with map::insert, which never overwrites — so a FUTURE
    // preset that deliberately authors one of these keys writes it in its own
    // row and wins. Every value above still applies; it is simply stated once.
    const std::map<juce::String, float> kShippedNoOpTail = {
        {"jitter", 0.0f}, {"delayScatter", 0.0f},
        {"sizeRandom", 0.0f}, {"gainRandom", 0.0f},
        {"grainTilt", 0.5f}, {"grainShape", 0.0f},     // tilt: 0.5, NOT 0
        {"grainCount", 8.0f}, {"tukeyTaper", 0.5f},    // v1.2.0's ceiling; taper no-op 0.5
        {"freeze", 0.0f}, {"direction", 0.0f},
        {"regenMakeup", 0.0f},
        {"sourceMode", 0.0f}, {"duck", 0.0f},
        {"driftRate", 0.30f}, {"driftDepth", 0.0f},    // rate: the DEFAULT, not 0
        {"diffusion", 0.0f}, {"drive", 0.0f},
        {"freezeLength", 0.0f},                         // v1.15.0: Ring, the shipped loop
        {"grainLink", 0.0f},                            // v1.17.0: Free, the Size knob
        {"grainDivision", static_cast<float> (kDefaultNoteDivision)}};   // inert while Free

    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        { "Reverse Bloom",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime",  500.0f},
           {"grainSize", 200.0f}, {"density", 53.3f}, {"feedback",  40.0f},
           {"lowCut",    100.0f}, {"highCut", 8000.0f},
           {"width",      60.0f}, {"mix",       40.0f}}, {} },

        { "Guitar Swell",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime",  700.0f},
           {"grainSize", 300.0f}, {"density", 47.5f}, {"feedback",  45.0f},
           {"lowCut",    120.0f}, {"highCut", 6500.0f},
           {"width",      55.0f}, {"mix",       55.0f}}, {} },

        { "Vocal Halo",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime",  380.0f},
           {"grainSize", 180.0f}, {"density", 65.0f}, {"feedback",  30.0f},
           {"lowCut",    300.0f}, {"highCut", 7000.0f},
           {"width",      70.0f}, {"mix",       25.0f}}, {} },

        { "Slow Wash",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime", 1400.0f},
           {"grainSize", 450.0f}, {"density", 18.3f}, {"feedback",  65.0f},
           {"lowCut",     80.0f}, {"highCut", 5000.0f},
           {"width",      85.0f}, {"mix",       50.0f}}, {} },

        { "Tight Smear",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime",  180.0f},
           {"grainSize",  70.0f}, {"density", 88.3f}, {"feedback",  35.0f},
           {"lowCut",    150.0f}, {"highCut", 11000.0f},
           {"width",      35.0f}, {"mix",       45.0f}}, {} },

        { "Dark Cavern",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime",  850.0f},
           {"grainSize", 320.0f}, {"density", 59.2f}, {"feedback",  70.0f},
           {"lowCut",    220.0f}, {"highCut", 1800.0f},
           {"width",      75.0f}, {"mix",       55.0f}}, {} },

        // feedback = 100 %: doubles as the preset-driven DSP-03 stability
        // statement (probe N renders this one for 30 s, not 10). Its density is
        // re-authored to hold overlap at 5.9, so the v1.0.1 loop duty cycle —
        // and with it the measured decay — is unchanged from v1.0.0.
        { "Near-Infinite",
          {{"syncMode", 0.0f}, {"noteDivision", 6.0f}, {"delayTime",  900.0f},
           {"grainSize", 350.0f}, {"density", 65.0f}, {"feedback", 100.0f},
           {"lowCut",    180.0f}, {"highCut", 2500.0f},
           {"width",      80.0f}, {"mix",       50.0f}}, {} },

        { "Rhythmic Reverse",
          {{"syncMode", 1.0f}, {"noteDivision", 4.0f}, {"delayTime",  500.0f},
           {"grainSize", 120.0f}, {"density", 76.7f}, {"feedback",  50.0f},
           {"lowCut",    140.0f}, {"highCut", 9000.0f},
           {"width",      50.0f}, {"mix",       45.0f}}, {} },
    };

    // C1: engineering units → normalised, through each param's own range. Handles
    // skew, step and choice-index uniformly (AudioParameterChoice's range is
    // 0…n-1 step 1, so convertTo0to1(6.0f) on the 13-entry division list = 0.5).
    // initializeFactoryPresets stores the normalised value verbatim;
    // applyPresetJson feeds it back through convertFrom0to1 on load.
    for (auto& preset : factoryPresets)
        preset.parameters.insert(kShippedNoOpTail.begin(), kShippedNoOpTail.end());

    for (auto& preset : factoryPresets)
        for (auto& [id, value] : preset.parameters)
            if (auto* p = parameters.getParameter(id))
                value = p->convertTo0to1(value);

    // Only re-seeds when JucePlugin_VersionString changes (.factory-version
    // sentinel). At a frozen 1.0.0 that means edits to the table above are a
    // SILENT no-op until ~/Library/O-ReverseDelay/Presets/Factory is removed.
    // The v1.0.0 -> v1.0.1 CMake VERSION bump is what makes the density and
    // range edits above actually reach disk.
    presetManager.initializeFactoryPresets(factoryPresets);

    // A1: user presets are stored as NORMALISED fractions and must be rescaled
    // whenever a parameter's range moves under them — delayTime at v1.0.1,
    // grainSize at v1.5.0. Runs immediately after the factory seed, on the
    // message thread, once per version.
    migrateUserPresets();
}

namespace
{
    /** "1.4.0" -> 10400, so preset-version gates read as plain comparisons.
        Two decimal digits per component is ample for a plugin whose components
        have never exceeded 5, and packed decimal keeps the constants legible in
        a debugger. Missing or non-numeric components parse as 0 via
        getIntValue(), which makes a truncated "1.5" compare as 1.5.0 — the
        conservative direction, since the only cost of treating a file as NEWER
        than it is would be skipping a rescale on a file that is not ours to
        begin with. */
    int packVersion (const juce::String& v)
    {
        auto parts = juce::StringArray::fromTokens (v.trim(), ".", "");

        const int major = parts.size() > 0 ? parts[0].getIntValue() : 0;
        const int minor = parts.size() > 1 ? parts[1].getIntValue() : 0;
        const int patch = parts.size() > 2 ? parts[2].getIntValue() : 0;

        return major * 10000 + minor * 100 + patch;
    }
}

//==============================================================================
// v1.0.1 (A1) — user-preset delayTime migration.
// v1.5.0      — user-preset grainSize migration, on the same machinery.
//
// Two DIFFERENT storage formats are in play, and they need opposite treatment:
//
//   * Session state (getStateInformation) is an APVTS ValueTree, and APVTS
//     stores each PARAM's DENORMALISED value — literal milliseconds. JUCE
//     restores it through ParameterAdapter::setDenormalisedValue(), which
//     re-normalises against whatever range is current. A v1.0.0 session saved
//     at 1400 ms therefore recalls 1400 ms under the 4000 ms range with no
//     migration at all — and rescaling it would actively corrupt it. Nothing to
//     do here; harness probe P asserts the round trip.
//
//   * Preset JSON (OuariconPresetManager::createPresetJson) stores
//     RangedAudioParameter::getValue(), i.e. the normalised 0..1 fraction, and
//     applyPresetJson feeds it straight back to setValueNotifyingHost(). Those
//     fractions DO shift meaning when the range max moves: a v1.0.0 preset saved
//     at 500 ms holds ~0.606, which under the 4000 ms range reads back as
//     ~1240 ms. Those files are what this function rewrites.
//
// Factory presets need no migration — the version bump re-seeds them from the
// engineering-unit table above.
//
// ── v1.5.0: the two arms have DIFFERENT version gates ─────────────────────────
// This is the part that is easy to get wrong, because through v1.4.0 there was
// only one arm and one gate, written as `if (version != "1.0.0") continue;`.
//
//   * delayTime's range moved once, at v1.0.1. Only v1.0.0 files carry the old
//     mapping, so its gate stays  version < 1.0.1.
//   * grainSize's range moved at v1.5.0, so EVERY file written by v1.0.0 through
//     v1.4.x carries the old mapping: its gate is  version < 1.5.0.
//
// Reusing the delayTime gate for grainSize would have migrated v1.0.0 presets
// and silently left every v1.1–v1.4 preset holding a fraction against the old
// curve — the majority of any real user library, and the failure is quiet: the
// preset still loads, it just recalls the wrong grain size.
//
// grainSize is also the harder rescale of the two. delayTime kept its skew
// centre (316 ms) and moved only its max, but grainSize moved BOTH — max
// 500 -> 4000 and centre 158 -> 316 — so the old and new curves differ in shape
// as well as extent. Reconstructing the old range exactly and round-tripping
// through milliseconds is the only correct transform; there is no scale factor
// that does it. Worked example: a v1.4.0 preset at the old default 200 ms holds
// ~0.573, which under the new curve would read back as ~1450 ms without this.
//
// Guarded by a version sentinel mirroring initializeFactoryPresets()'s: without
// it every processor construction (each auval/pluginval scan pass, each instance
// added to a session) would re-read every user preset on the message thread, and
// two instances constructing concurrently would race on the same files.
//
// v1.7.3 (IN-05): the sentinel is now STAMPED BEFORE THE WALK. It used to be
// written at the end, which made this a check-then-act that did not close the
// race it was documented to close: several instances constructing in parallel on
// the first launch after an upgrade all pass the existsAsFile() check, all walk,
// and all rewrite the same files. The outcome was benign — the transform is
// per-file version-gated so every writer produces identical content, and
// replaceWithText goes via a temp file — but it was still N× the intended
// message-thread file IO during construction, which is exactly where AU
// validation is timing-sensitive.
//
// Two trade-offs, both accepted deliberately:
//   - a v1.0.0 preset restored from a backup AFTER the sentinel is stamped will
//     not be migrated (unchanged from before);
//   - an INTERRUPTED pass is now never retried, because the claim is already on
//     disk. Recovery is to delete the sentinel — see NOTES.md "Known Issues".
//     This is the same trade the backup path already accepts.
void ReverseDelayProcessor::migrateUserPresets()
{
    auto userDir  = presetManager.getUserPresetsDirectory();
    auto sentinel = presetManager.getPresetsDirectory().getChildFile(".user-migration-version");

    if (sentinel.existsAsFile()
        && sentinel.loadFileAsString().trim() == JucePlugin_VersionString)
        return;

    // Stamp FIRST: a concurrent constructor must see the claim before the work
    // is visible, or the sentinel serialises nothing.
    sentinel.getParentDirectory().createDirectory();
    sentinel.replaceWithText(JucePlugin_VersionString);

    if (userDir.isDirectory())
    {
        // Reconstruct v1.0.0's delayTime range exactly: same min, same skew
        // centre, only the max differs. setSkewForCentre re-solves the exponent
        // against the max, so the two curves are genuinely different mappings —
        // this is not a linear rescale.
        juce::NormalisableRange<float> legacyDelayRange { kDelayTimeMinMs, kLegacyDelayTimeMaxMs, 0.01f };
        legacyDelayRange.setSkewForCentre(kDelayTimeSkewCentreMs);

        // v1.5.0: grainSize's pre-1.5.0 range — max AND centre both differ.
        juce::NormalisableRange<float> legacyGrainRange { kGrainSizeMinMs, kLegacyGrainSizeMaxMs, 0.01f };
        legacyGrainRange.setSkewForCentre(kLegacyGrainSizeSkewCentreMs);

        auto* delayParam = parameters.getParameter("delayTime");
        auto* grainParam = parameters.getParameter("grainSize");

        // Rescale one stored fraction from `from` onto `target`'s current range,
        // via the engineering value both agree on. Returns false when the key is
        // absent so the caller knows whether the file actually changed.
        auto rescale = [](juce::DynamicObject* params,
                          const juce::String& id,
                          const juce::NormalisableRange<float>& from,
                          const juce::RangedAudioParameter* target) -> bool
        {
            if (params == nullptr || target == nullptr || ! params->hasProperty(id))
                return false;

            const float oldNorm = static_cast<float>(
                static_cast<double>(params->getProperty(id)));
            const float value   = from.convertFrom0to1(juce::jlimit(0.0f, 1.0f, oldNorm));
            const float newNorm = target->getNormalisableRange().convertTo0to1(value);

            params->setProperty(id, newNorm);
            return true;
        };

        for (const auto& file : userDir.findChildFiles(juce::File::findFiles, false, "*.json"))
        {
            auto data = juce::JSON::parse(file.loadFileAsString());
            auto* obj = data.getDynamicObject();
            if (obj == nullptr)
                continue;

            // A missing "version" is treated as 1.0.0 — that field has been
            // written since the preset manager's first release, so absence means
            // hand-authored/ancient.
            const juce::String version = obj->hasProperty("version")
                                           ? obj->getProperty("version").toString()
                                           : juce::String("1.0.0");
            const int fileVersion = packVersion(version);

            auto paramsVar = obj->getProperty("parameters");
            auto* params   = paramsVar.getDynamicObject();

            bool changed = false;

            // delayTime: widened at v1.0.1, so only v1.0.0 files are stale.
            if (fileVersion < packVersion("1.0.1"))
                changed |= rescale(params, "delayTime", legacyDelayRange, delayParam);

            // grainSize: widened at v1.5.0, so everything through v1.4.x is stale.
            if (fileVersion < packVersion("1.5.0"))
                changed |= rescale(params, "grainSize", legacyGrainRange, grainParam);

            if (! changed)
                continue;

            obj->setProperty("version", JucePlugin_VersionString);
            file.replaceWithText(juce::JSON::toString(data, true));
        }
    }
}

//==============================================================================
void ReverseDelayProcessor::reset()
{
    // C: hosts call reset() to drop tail state between transport passes. v1.0.0
    // had no override, so the capture ring, in-flight grains and filter memory
    // survived and bled a stale reverse tail into the next pass. Everything here
    // is alloc-free (CaptureBuffer::clear never calls setSize).
    capture.clear();
    grainPool.clear();
    scheduler.clear();

    // v1.3.0: the meter must follow the engine it reports on — the pool is now
    // empty, so a stale non-zero count would sit on the UI until the next block.
    // The DROP/REFUSAL counters are deliberately NOT cleared here: they are
    // cumulative diagnostics over the processor's life, and a host that calls
    // reset() between transport passes would otherwise erase the evidence of a
    // drop that happened in the pass before.
    publishedActiveGrains.store(0, std::memory_order_relaxed);
    publishedFreezeEngaged.store(false, std::memory_order_relaxed);   // v1.13.0: latch cleared below
    peakInSinceRead.store(0.0f, std::memory_order_relaxed);           // v1.14.0
    peakOutSinceRead.store(0.0f, std::memory_order_relaxed);

    // v1.8.0 (B4 #7): the diffusion chain holds up to ~48 ms of the previous
    // pass's tail, so it belongs with the filter memory in reason C above — a
    // host reset that dropped the ring and the filters but left the allpasses
    // loaded would splice the old tail's smear into the new pass. Allpass
    // reset() only zeroes state; the buffers keep their prepareToPlay sizing.
    resetLoopState();

    // v1.7.2 (WR-03): the coefficient grid restarts with the filter state, so a
    // host reset does not leave a partial countdown straddling the boundary.
    // Note the filters' COEFFICIENTS are deliberately not touched — only their
    // state is — so a reset cannot produce sticky silence
    // (pattern_biquad_nan_guard_sticky_silence).
    coeffCountdown = 0;

    wetScratch.clear();
    loopScratch.clear();
    fbScratch.clear();

    // Same seeds as prepareToPlay — a reset must not desynchronise the sequences
    // the harness depends on. Per-instance since v1.1.0.
    rngState       = instanceSeed;
    jitterRngState = deriveJitterSeed (instanceSeed);
    panSign        = 1.0f;

    // Jump the smoothers to their current targets rather than ramping from
    // whatever the previous pass ended on.
    //
    // v1.7.3 (IN-06): the null guards these four carried are gone — every cached
    // atomic is jassert-ed once in the constructor, so this now reads exactly as
    // prepareToPlay's identical block does.
    feedbackSmoothed.setCurrentAndTargetValue(pFeedback->load() * 0.01f);
    mixSmoothed.setCurrentAndTargetValue(pMix->load() * 0.01f);
    lowCutSmoothed.setCurrentAndTargetValue(pLowCut->load());
    highCutSmoothed.setCurrentAndTargetValue(pHighCut->load());
    diffuseSmoothed.setCurrentAndTargetValue(currentDiffuseMix());   // v1.12.2
    driveSmoothed.setCurrentAndTargetValue(driveRatio (pDrive->load()));

    // v1.7.2 (CR-02): freeze starts at ZERO here, not at the parameter's value —
    // the same correction prepareToPlay carries, for the same reason and with one
    // extra consequence specific to reset().
    //
    // capture.clear() above has just emptied the ring, so there is nothing to
    // hold. Jumping the smoother to 1.0 while the latch (correctly) refuses to
    // arm against an empty ring would leave the two disagreeing: processBlock
    // would target 0.0f and spend ~20 ms ramping DOWN through pushCrossfaded
    // against the un-latched freezeLoopSamples == 1, writing the zeros the guard
    // exists to prevent. Starting at 0 keeps the smoother and the latch in
    // agreement — the hold re-arms itself once a grain of material exists.
    //
    // The material the user froze is genuinely gone, which is the honest reading
    // of "the host dropped the tail state"; inventing a loop out of a cleared
    // buffer would be worse than re-capturing one.
    freezeSmoothed.setCurrentAndTargetValue(0.0f);
    freezeEngaged     = false;
    freezeLoopSamples = 1;

    // v1.7.0: the duck follower. Zeroed rather than jumped to the current input
    // level, because there IS no current input level at a reset — the host has
    // just told us the signal is gone. Starting at 0 means the first block after
    // a reset is un-ducked and the envelope climbs into the duck over its attack
    // time, which is what a transport start sounds like anyway.
    duckEnv = 0.0f;
}

ReverseDelayProcessor::~ReverseDelayProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout ReverseDelayProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // delayTime: 50–4000 ms, default 500, skew centred on 316 ms.
    //
    // v1.0.1 (A1): max raised 2000 -> 4000 ms. At 2000 ms the sync clamp silently
    // collapsed 1/1 below 120 BPM, 1/2D below 90 and 1/2 below 60 — i.e. across the
    // whole 70–100 BPM band this plugin is written for, the UI named a division it
    // was not playing and two divisions landed on the same delay. 4000 ms covers
    // 1/1 down to 60 BPM.
    //
    // The skew centre deliberately stays at 316 ms: setSkewForCentre re-solves the
    // exponent against the new max, so short times keep their knob resolution
    // instead of being crushed into the bottom of a linear-ish 4 s sweep.
    {
        juce::NormalisableRange<float> range { kDelayTimeMinMs, kDelayTimeMaxMs, 0.01f };
        range.setSkewForCentre(kDelayTimeSkewCentreMs);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "delayTime", 1 }, "Delay Time", range, 500.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));
    }

    // syncMode: Free / Sync, default Sync
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "syncMode", 1 }, "Sync Mode",
        juce::StringArray { "Free", "Sync" }, 1));

    // noteDivision: the kNoteDivisions table, contract order, default 1/4.
    juce::StringArray divisionNames;
    for (const auto& d : kNoteDivisions)
        divisionNames.add (d.name);

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "noteDivision", 1 }, "Note Division",
        divisionNames, kDefaultNoteDivision));

    // grainSize: 50–4000 ms, default 200, skew centred on 316 ms (v1.5.0).
    //
    // Endpoints and skew centre both come from the named constants rather than
    // literals — the v1.0.0 A1 defect was exactly a literal drifting from the
    // parameter range, and migrateUserPresets() has to reconstruct the OLD curve
    // from kLegacyGrainSize* to rescale saved presets, so the two must not be
    // able to disagree. Default stays 200 ms: it is the shipped value and a new
    // instance must sound identical to v1.4.0.
    {
        juce::NormalisableRange<float> range { kGrainSizeMinMs, kGrainSizeMaxMs, 0.01f };
        range.setSkewForCentre(kGrainSizeSkewCentreMs);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "grainSize", 1 }, "Grain Size", range, 200.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));
    }

    // density: 0–100 %, default 60, linear
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "density", 1 }, "Density",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // feedback: 0–100 %, default 40, linear
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "feedback", 1 }, "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 40.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // lowCut: 20–2000 Hz, default 100, skew centred on 200 Hz
    {
        juce::NormalisableRange<float> range { 20.0f, 2000.0f, 0.01f };
        range.setSkewForCentre(200.0f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "lowCut", 1 }, "Low Cut", range, 100.0f,
            juce::AudioParameterFloatAttributes().withLabel("Hz")));
    }

    // highCut: 500–20000 Hz, default 8000, skew centred on 3162 Hz
    {
        juce::NormalisableRange<float> range { 500.0f, 20000.0f, 0.01f };
        range.setSkewForCentre(3162.0f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "highCut", 1 }, "High Cut", range, 8000.0f,
            juce::AudioParameterFloatAttributes().withLabel("Hz")));
    }

    // width: 0–100 %, default 60, linear
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "width", 1 }, "Width",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // mix: 0–100 %, default 35, linear
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 35.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // ── v1.1.0 (B3): grain randomisation ────────────────────────────────────
    //
    // EVERY ONE DEFAULTS TO 0, and that is load-bearing rather than cautious:
    //
    //   * A new parameter appended to the APVTS is absent from every v1.0
    //     session and every v1.0 preset. APVTS leaves absent params at their
    //     default, and OuariconPresetManager::applyPresetJson resets all params
    //     to defaults before applying, so a v1.0 session or preset reopened
    //     under v1.1 gets 0 for all four. A non-zero default would therefore
    //     silently re-voice existing work
    //     (pattern_activating_dead_param_default_timbre).
    //
    //   * 0 is the exact no-op in the engine, not merely a small value: each
    //     randomisation is gated on `amount > 0` and draws NOTHING from the
    //     shared xorshift when off, so the pan sequence — and with it the whole
    //     v1.0.1 render — is reproduced bit-for-bit. Render-harness probe T
    //     asserts that equality against a defaults render rather than trusting
    //     it, because "a randomiser that is off" is exactly the kind of claim
    //     that quietly stops being true.
    //
    // This is why v1.1.0 is a MINOR bump and not a MAJOR one: nothing is
    // renamed, removed, re-ranged or re-typed, and no existing session changes.

    // jitter: 0–100 %, default 0. Randomises the SPAWN INTERVAL; the mean
    // interval is unchanged, so density and the loop duty cycle do not move.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "jitter", 1 }, "Jitter",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // delayScatter: 0–500 ms, default 0. Randomises each grain's latched D by
    // ±this, so the smear thickens without the average rhythmic anchor moving.
    // The ring is sized for the POSITIVE half of this range (kCaptureSeconds).
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayScatter", 1 }, "Delay Scatter",
        juce::NormalisableRange<float>(0.0f, kDelayScatterMaxMs, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // sizeRandom: 0–100 %, default 0. Randomises each grain's latched G.
    // Jitter alone leaves a residual periodicity because every grain still
    // shares one envelope length; this is what removes it.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sizeRandom", 1 }, "Size Random",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // gainRandom: 0–100 %, default 0. Randomises per-grain gain AFTER the
    // feedback tap — see the wet/loop split in processBlock. Power-normalised,
    // so it adds shimmer without adding level.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "gainRandom", 1 }, "Gain Random",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // ── v1.2.0 (B1): grain window shape + tilt ──────────────────────────────
    //
    // The v1.1 block above turns on "defaults to 0". These two default to the
    // SHIPPED WINDOW instead, which is the same guarantee reached differently —
    // and the distinction is worth stating because it is the easy thing to get
    // wrong here:
    //
    //   * grainShape defaults to index 0 = Hann. Index 0 is also what an absent
    //     key resolves to in a v1.0/v1.1 preset or session, so the ordering in
    //     WindowLut::Shape is load-bearing: moving Hann off index 0 would
    //     re-voice every existing session silently.
    //
    //   * grainTilt defaults to 0.5, NOT 0. 0.5 is the symmetric window; 0 is a
    //     hard peak-early tilt. This is the one parameter in the plugin where
    //     "default to zero for safety" is exactly backwards.
    //
    // Both are the engine's exact no-op, not merely close to it: shape 0 reads
    // the same Hann table v1.0.0 built, and tilt 0.5 makes the phase warp the
    // bitwise identity while its power normalisation is exactly 1.0f (see
    // WindowLut.h). Probe Z1 asserts a defaults render is BIT-IDENTICAL to the
    // v1.1.0 one rather than taking that on trust.

    // grainTilt: 0–1, default 0.5, step 0.001. Linear and unskewed so the
    // midpoint of the knob IS the symmetric window.
    //
    // The step matters for the identity guarantee: NormalisableRange snaps to
    // the interval grid, and 0.5 sits exactly on a 0.001 grid, so the parameter
    // reads back as exactly 0.5f. It is displayed as a signed percentage
    // (−100 % early … +100 % late) by the UI — a display concern, kept out of
    // the range so the C++ NormalisableRange stays the single source of truth.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "grainTilt", 1 }, "Grain Tilt",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    // grainShape: 5 entries, WindowLut::Shape order, default index 0 (Hann).
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "grainShape", 1 }, "Grain Shape",
        juce::StringArray { "Hann", "Tukey", "Gaussian", "Triangular", "Expo-Decay" },
        0));

    // ── v1.3.0 (B2): the overlap ceiling ────────────────────────────────────
    //
    // grainCount: 2–16 grains, default 8, step 1, linear.
    //
    // The default is the load-bearing part, for the third release running, and
    // for a slightly different reason each time. v1.1's four defaulted to 0
    // because 0 was the no-op; v1.2's grainTilt defaulted to 0.5 because the
    // no-op was the MIDPOINT; here the no-op is v1.2.0's own hard-coded
    // constant, so the default is 8 and the parameter's job at that value is to
    // reproduce a number that used to be spelled out in processBlock.
    //
    // That reproduction is exact rather than approximate, which is the whole
    // reason this is a MINOR bump. Density is stored DENORMALISED in session
    // state — a session saved at 60 % recalls 60 % — so if the density knob's
    // span had been widened in place instead, every saved session would have
    // become ~2.3x denser at the same knob position with no migration possible
    // (critical_apvts_denormalised_vs_preset_normalised: sessions and preset
    // JSON need opposite treatment, and sessions cannot be rescaled without
    // corrupting the ones already correct). Making the ceiling its own
    // parameter sidesteps that entirely: absent from a v1.0–v1.2 session or
    // preset, it resolves to this default, and (8 − 2) is exactly 6.0f.
    //
    // Step 1 keeps the parameter on integers, so "Count" reads as a grain count
    // and the default lands exactly on 8.0f rather than near it. The reachable
    // overlap span is [2, ceiling]: density 0 always gives 2 whatever the
    // ceiling is, so nothing the knob could reach before is now unreachable.
    // At ceiling = 2 the two coincide and density goes inert — the honest
    // endpoint of "lock overlap to the Hann constant-overlap-add minimum", and
    // the reason the COUNT panel shows the effective overlap next to the knob.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "grainCount", 1 }, "Grain Count",
        juce::NormalisableRange<float>(kOverlapCeilingMin, kOverlapCeilingMax, 1.0f),
        kLegacyOverlapMax));

    // ── v1.4.0: Tukey taper α ────────────────────────────────────────────────
    //
    // tukeyTaper: 0.01–1.00, step 0.01, default 0.50 — the value v1.2.0 froze.
    // Fourth release running where the no-op default is a specific number and
    // not zero, and here 0 is not merely wrong but out of range: α = 0 is the
    // rectangular window, a hard step at both grain edges.
    //
    // The STEP is not cosmetic. α changes the window's power and amplitude duty
    // cycles, so the level and feedback normalisations have to track it, and
    // WindowLut integrates those from the real window rather than a closed form.
    // A 0.01 step over [0.01, 1.00] is exactly the 100-entry grid it precomputes,
    // so every reachable α hits an entry exactly — no interpolation, and the
    // α = 0.50 entry reproduces v1.3.0's constants bitwise. See
    // WindowLut::kNumTaperSteps.
    //
    // INERT unless grainShape is Tukey. That is a deliberate dead-control-shaped
    // thing, and the UI owns making it legible: the knob dims and its tooltip
    // says so when another shape is selected. The alternative — applying a taper
    // to windows that have no taper — would mean either redefining four shapes or
    // pretending the knob does something.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tukeyTaper", 1 }, "Tukey Taper",
        juce::NormalisableRange<float>(WindowLut::kTukeyTaperMin,
                                       WindowLut::kTukeyTaperMax,
                                       WindowLut::kTukeyTaperStep),
        WindowLut::kTukeyTaperDefault));

    // ── v1.6.0 (B4 #1-#3): the MOTION panel ─────────────────────────────────
    //
    // Three parameters whose no-op is 0 — the first release since v1.1.0 where
    // that is true of all of them, and worth naming because v1.2.0 (tilt 0.5),
    // v1.3.0 (count 8) and v1.4.0 (taper 0.5) each trapped exactly the reflex
    // that would be CORRECT here. The rule was never "default to zero"; it is
    // "default to whatever the engine already did", and here the engine already
    // did nothing.

    // freeze: bool, default false. The only bool parameter in the plugin, and
    // therefore the only one that reaches the page through a WebToggleButtonRelay
    // rather than a slider or combo relay (see PluginEditor's kToggleIds).
    //
    // A bool rather than a two-entry choice on purpose: hosts draw a bool as a
    // switch and automate it as a switch, which is what a performance control
    // wants. syncMode is a choice because it names two MODES; this names an
    // on/off state.
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "freeze", 1 }, "Freeze", false));

    // v1.15.0 — freezeLength: how much of the ring a hold loops. Latched on the
    // freeze rising edge with the loop length itself, so moving it mid-hold
    // changes the NEXT hold, never the running one (a loop whose length moves
    // under the read head skips).
    //
    // Ring is index 0 and the default: an absent key in any pre-v1.15.0 session
    // or preset resolves there, and Ring is the v1.6.0-v1.14.0 loop bitwise.
    // Bar modes read the host's tempo whether or not TIME is in Sync, and fall
    // back to Delay when there is none. See FreezeLength in the header.
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "freezeLength", 1 }, "Freeze Length",
        juce::StringArray { "Ring", "Delay", "1 Bar", "2 Bars" }, 0));

    // direction: 0–100 %, default 0 (every grain reverse). The probability that
    // a grain is latched FORWARD at spawn, not a crossfade between two renders —
    // at 50 % the cloud genuinely contains both read laws at once.
    //
    // Step 0.1 matches every other percentage knob here, and 0.0f is exactly on
    // that grid, so the parameter reads back as exactly zero and the spawn
    // handler's `> 0.0f` gate is exact rather than nearly exact.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "direction", 1 }, "Direction",
        juce::NormalisableRange<float>(0.0f, kDirectionMaxPct, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // regenMakeup: 0 dB … kRegenMakeupMaxDb, default 0 dB — the D11 constant
    // declined at v1.0, shipped as a control instead of a hidden number.
    //
    // Unipolar: attenuation is already available on the `feedback` knob, so a
    // negative half would be a second, worse way to spell a control that exists.
    // That also makes the no-op the range MINIMUM, which is the cleanest form of
    // the guarantee — a preset or session written before v1.6.0 has no key here,
    // resolves to 0 dB, and the loop gain it recalls is bitwise the loop gain it
    // was saved with (regenMakeupGain returns exactly 1.0f at 0 dB).
    //
    // The maximum is a MEASURED bound, not a taste: see kRegenMakeupMaxDb.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "regenMakeup", 1 }, "Regen Makeup",
        juce::NormalisableRange<float>(0.0f, kRegenMakeupMaxDb, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // ── v1.7.0 (B4 #4-#6): SOURCE / DUCK / DRIFT ────────────────────────────
    //
    // Four parameters, and their no-ops split two ways for the first time in one
    // release — three at zero and one that is NOT, which is exactly the mix that
    // has caught this plugin before. driftRate's neutral value is its DEFAULT
    // rate, because the control it is neutral through is driftDepth; writing
    // driftRate as 0 anywhere would be clamped to kDriftRateMinHz and would look
    // like it worked.

    // sourceMode: Mono Sum / Stereo, default index 0 (Mono Sum).
    //
    // Index 0 is load-bearing in the same way grainShape's Hann-at-0 is: a
    // v1.0-v1.6 session or preset has no key here, APVTS leaves an absent param
    // at its default and applyPresetJson resets to defaults before applying, so
    // index 0 is what every piece of existing work resolves to. It must
    // therefore be the shipped behaviour, which is the mono sum the grain engine
    // has read since Stage 2 (decision D4).
    //
    // A choice rather than a bool because it names two MODES of reading the
    // buffer, which is the distinction syncMode draws and freeze does not — and
    // because a third mode (mid/side) is a plausible later addition that a bool
    // could not grow into without a parameter-type change.
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "sourceMode", 1 }, "Source Mode",
        juce::StringArray { "Mono Sum", "Stereo" }, 0));

    // duck: 0–100 %, default 0. Attenuates the WET by the dry input's envelope.
    //
    // 0 is the exact no-op and not merely a small value: the gain is
    // 1 − depth·u, so at depth 0 it is exactly 1.0f whatever the envelope holds,
    // and multiplying the wet by exactly 1.0f is bitwise identity. The follower
    // still runs — it is two mul-adds per sample and keeping it warm means
    // raising the knob mid-session does not start from a stale envelope — but
    // nothing it computes can reach the output at 0.
    //
    // Applied to the OUTPUT path only, never to the feedback tap. A duck inside
    // the loop would attenuate what is written back, so the input's envelope
    // would modulate the DECAY RATE and the knob would control how long the tail
    // lasts rather than when it is heard. That is the same split gainRandom has
    // used since v1.1.0 and loopTrim since v1.2.0, applied to a third control.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "duck", 1 }, "Duck",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // driftRate: 0.02–5 Hz, default 0.30, skew centred on 0.30 Hz.
    //
    // Skewed for the same reason delayTime is: the useful settings are bunched at
    // the bottom (a slow wander under a wash) and the top of the range is a
    // special effect, so a linear knob would spend most of its travel on vibrato
    // and leave "slow" unreachable. Centring the skew on the DEFAULT puts the
    // shipped value at the knob's midpoint.
    //
    // Inert while driftDepth is 0, which is the shipped state — the same
    // deliberate dead-control shape tukeyTaper has when the window is not Tukey,
    // and handled the same way: the UI dims the cell, the relay stays bound, and
    // the knob is never actually dead.
    {
        juce::NormalisableRange<float> range { kDriftRateMinHz, kDriftRateMaxHz, 0.01f };
        range.setSkewForCentre(kDriftRateCentreHz);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "driftRate", 1 }, "Drift Rate", range, kDriftRateCentreHz,
            juce::AudioParameterFloatAttributes().withLabel("Hz")));
    }

    // driftDepth: 0–100 %, default 0. How far the LFO moves D, as a fraction of
    // kDriftMaxFraction — so 100 % is ±25 % of the current delay.
    //
    // 0 is the exact no-op: driftMul() early-returns 1.0f rather than evaluating
    // a sine that happens to be near zero, so `D * 1.0f` is bitwise D and no
    // std::sin is called at all. Probe AV asserts that as bit-equality against a
    // defaults render rather than trusting it.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "driftDepth", 1 }, "Drift Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // ── v1.8.0 (B4 #7-#8): COLOUR ───────────────────────────────────────────
    //
    // Both no-ops are the range minimum and both are EXACT rather than nearly
    // exact, which is the property the defaults-bit-identity probes rest on.
    // Neither needs the kind of measured cap kRegenMakeupMaxDb carries, and the
    // reasons are different for the two — diffusion because an allpass cannot
    // add loop gain, drive because its normalisation holds the small-signal gain
    // at 1. Both are argued where they are implemented rather than here.

    // diffusion: 0-100 %, default 0. A wet/dry mix over the four-section allpass
    // chain in the feedback return, NOT a scaling of the allpass coefficient.
    //
    // That distinction is the whole reason 0 is a true no-op. Scaling the
    // coefficient toward zero leaves each section as a pure N-sample DELAY, so
    // "off" would still be ~48 ms of latency spliced into the loop and the
    // knob's first movement would click. Mixing leaves the dry path at exactly
    // (1 - 0)·x = x, which is bitwise identity, and gives a continuous path from
    // there to fully diffused.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "diffusion", 1 }, "Diffusion",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // drive: 0-100 %, default 0. Percent rather than dB, deliberately.
    //
    // regenMakeup next door is in dB and this is not the same kind of quantity:
    // regenMakeup IS a gain and its dB reading is the honest one, whereas drive
    // is level-compensated (tanh(d·x)/d) and so has no gain to report — its dB
    // figure would name a pre-gain the user never hears as level. Percent says
    // "amount of character", which is what it is, and matches Duck, Direction
    // and Drift Depth. It also avoids two adjacent dB knobs in one panel that
    // do unrelated things, which is a misread waiting to happen.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "drive", 1 }, "Drive",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // ── v1.17.0: Grain Link ──────────────────────────────────────────────────
    //
    // grainLink: Free / = Delay / Division, default index 0 (Free). Free is the
    // Size knob through the v1.16.0 expression verbatim, and index 0 is what an
    // absent key in any older session or preset resolves to — the sourceMode /
    // freezeLength guarantee again. See resolveGrainMs() in the header.
    //
    // Appended at the END of the layout rather than beside grainSize, so no
    // existing parameter's index moves for a host that automates by index.
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "grainLink", 1 }, "Grain Link",
        juce::StringArray { "Free", "= Delay", "Division" }, 0));

    // grainDivision: the SAME kNoteDivisions table as noteDivision (one table,
    // v1.12.4 HIGH-03), default 1/4 so Division at the defaults lands on the
    // same note value as a synced delay. Read only while grainLink is Division.
    juce::StringArray grainDivisionNames;
    for (const auto& d : kNoteDivisions)
        grainDivisionNames.add (d.name);

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "grainDivision", 1 }, "Grain Division",
        grainDivisionNames, kDefaultNoteDivision));

    return layout;
}

//==============================================================================
// v1.1.0 — per-instance RNG seed.
//
// v1.0 seeded every instance with the same literal, so two instances on two
// tracks produced identical pan sequences and would, once v1.1's randomisations
// landed, produce identical grain jitter/scatter/size/gain too — i.e. two
// tracks that should decorrelate into a wide cloud would instead move together.
//
// The seed is fixed for the LIFETIME of the processor rather than re-rolled per
// prepareToPlay, which keeps two properties that both matter:
//   * one instance renders identically across prepare/reset cycles (probe O
//     compares a 512- and a 4096-sample render of the same instance and demands
//     bit equality; a per-prepare reseed would break that outright),
//   * two instances differ.
//
// Under the render harness it collapses to v1.0's literal so all 49 shipped
// probes keep their exact expected output.
juce::uint32 ReverseDelayProcessor::makeInstanceSeed (const void* self) noexcept
{
   #if OUARICON_RENDER_HARNESS
    juce::ignoreUnused (self);
    return 0x12345678u;
   #else
    // Address bits (ASLR + allocator placement) mixed with a per-process
    // counter, so instances differ even if an allocator reuses an address.
    static std::atomic<juce::uint32> counter { 0 };

    const auto addr = static_cast<juce::uint32> (
        reinterpret_cast<juce::pointer_sized_uint> (self) >> 4);

    const juce::uint32 s = 0x12345678u ^ addr
                         ^ (counter.fetch_add (1u, std::memory_order_relaxed) * 0x9E3779B9u);

    // xorshift32 is absorbing at zero — a zero state returns 0.0f forever, so
    // every grain would pan hard to one side and every randomisation would
    // collapse to its lower bound. One in 4 billion, silent, and permanent.
    return s != 0u ? s : 0x12345678u;
   #endif
}

void ReverseDelayProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int maxBlock = juce::jmax(1, samplesPerBlock);

    // ALL allocation happens here — processBlock touches only preallocated state.
    // v1.0.1 (A1): 3.5 -> 5.5 s (Dmax grew 2.0 -> 4.0 s).
    // v1.1.0 (B3): 5.5 -> 6.0 s. delayScatter can push a grain's LATCHED delay
    // 500 ms past the delayTime max, so the requirement became 4.5 + 2·0.5 =
    // 5.5 s — which 5.5 s met by a single sample.
    // v1.5.0:     6.0 -> 13.0 s. G_max went 0.5 -> 4.0 s and the requirement is
    // gD_max + 2·G_max, so 4.5 + 2·4.0 = 12.5 s. See kCaptureSeconds.
    // v1.7.0:    13.0 -> 14.0 s. Delay drift MULTIPLIES the latched delay by up
    // to 1 + kDriftMaxFraction, so gD_max became 4.0·1.25 + 0.5 = 5.5 s and the
    // requirement 13.5 s. The static_assert in the header is what enforces it.
    capture.prepare(sampleRate, kCaptureSeconds);
    scheduler.prepare(sampleRate);
    grainPool.clear();

    // v1.7.2 (WR-02): the spawn-request cap's bound is stated against
    // hostBlock <= 16384 and fs >= 44100. Both assumptions hold for every host
    // this ships into, but the bound is now written down in
    // GrainScheduler::kMaxSpawnsPerBlock's derivation rather than assumed, so the
    // one input the plugin does not control gets asserted where it arrives.
    // Exceeding it is not a fault — the scheduler drops requests it cannot report
    // and getDroppedSpawnCount() surfaces exactly that — but it invalidates the
    // harness's `dropped == 0` assertion, which is the thing worth knowing.
    jassert (maxBlock <= 16384);

    // Deterministic sequences per prepare — from the INSTANCE's seed, not a
    // shared literal, so this instance repeats and the next one differs.
    rngState       = instanceSeed;
    jitterRngState = deriveJitterSeed (instanceSeed);
    panSign        = 1.0f;

    wetScratch.setSize(2, maxBlock);
    loopScratch.setSize(2, maxBlock);
    fbScratch.setSize(2, maxBlock);
    wetScratch.clear();
    loopScratch.clear();
    fbScratch.clear();

    const double smoothingSeconds = 0.02;   // ~20 ms per contract
    feedbackSmoothed.reset(sampleRate, smoothingSeconds);
    mixSmoothed.reset(sampleRate, smoothingSeconds);
    lowCutSmoothed.reset(sampleRate, smoothingSeconds);
    highCutSmoothed.reset(sampleRate, smoothingSeconds);
    freezeSmoothed.reset(sampleRate, smoothingSeconds);   // v1.6.0
    diffuseSmoothed.reset(sampleRate, smoothingSeconds);  // v1.12.2
    driveSmoothed.reset(sampleRate, smoothingSeconds);

    feedbackSmoothed.setCurrentAndTargetValue(pFeedback->load() * 0.01f);
    mixSmoothed.setCurrentAndTargetValue(pMix->load() * 0.01f);
    lowCutSmoothed.setCurrentAndTargetValue(pLowCut->load());
    highCutSmoothed.setCurrentAndTargetValue(pHighCut->load());
    diffuseSmoothed.setCurrentAndTargetValue(currentDiffuseMix());   // v1.12.2
    driveSmoothed.setCurrentAndTargetValue(driveRatio (pDrive->load()));

    // v1.7.2 (CR-02): start UN-frozen, even when the parameter says frozen.
    //
    // This used to jump to the parameter's value, reasoning that "a session saved
    // with Freeze engaged must reopen frozen". It still does — but the RING is
    // not part of that saved state, and capture.prepare() below has just
    // allocated and cleared it. Jumping the smoother to 1.0 here made the first
    // processBlock hold a buffer with nothing in it: the latch floored at one
    // sample, pushLooped(1) copied the previous (zero) slot, and the wet path was
    // exactly zero for the life of the session with no way back except toggling
    // Freeze off and waiting for capture.
    //
    // Starting at 0 with freezeEngaged false leaves the edge detector ARMED, so
    // the hold arms itself on the first block where a grain of real material
    // exists (see the freeze latch in processBlock). The session still reopens
    // frozen; it just captures a grain first, which is the only reading under
    // which the control does anything at all.
    freezeSmoothed.setCurrentAndTargetValue(0.0f);
    freezeEngaged     = false;
    freezeLoopSamples = 1;

    // v1.7.0: the duck follower starts un-ducked — see reset().
    duckEnv = 0.0f;

    // In-loop damping filters. prepare() + reset() here; coefficients seeded now
    // so the cached-cutoff guards start valid (the guard only gates recompute).
    const juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(maxBlock), 1 };
    hpL.prepare(spec); hpR.prepare(spec);
    lpL.prepare(spec); lpR.prepare(spec);
    hpL.reset(); hpR.reset();
    lpL.reset(); lpR.reset();

    const float fsF = static_cast<float>(sampleRate);
    lastLowCut  = juce::jlimit(20.0f,  0.49f * fsF, lowCutSmoothed.getCurrentValue());
    lastHighCut = juce::jlimit(500.0f, 0.49f * fsF, highCutSmoothed.getCurrentValue());

    // v1.7.2 (WR-03): zeroed so the first block refreshes coefficients on its
    // first sub-block rather than inheriting a partial countdown from whatever
    // the previous configuration was mid-way through.
    coeffCountdown = 0;

    const auto hpCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(sampleRate, lastLowCut);
    const auto lpCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sampleRate, lastHighCut);
    *hpL.coefficients = hpCoeffs;  *hpR.coefficients = hpCoeffs;
    *lpL.coefficients = lpCoeffs;  *lpR.coefficients = lpCoeffs;

    // v1.8.0 (B4 #7) — the diffusion chain. Lengths are re-derived from ms here
    // rather than kept as samples across a rate change, so 4.7 ms stays 4.7 ms
    // at 192 kHz instead of becoming 1.2. This is the ONLY place the allpass
    // buffers are sized, and the buffer length IS the section's delay, so a
    // length/rate mismatch has nowhere to exist.
    for (size_t i = 0; i < kDiffusionAllpassMs.size(); ++i)
    {
        const int n = juce::jmax (1, juce::roundToInt (kDiffusionAllpassMs[i] * 0.001f * fsF));
        apL[i].prepare (n);
        apR[i].prepare (n);
    }
}

// The loop's recirculating filter memory: damping HP/LP and the diffusion
// chain. Shared by reset() and the non-finite guard so neither can drift into
// clearing only half of it. State only — coefficients are left alone
// (pattern_biquad_nan_guard_sticky_silence).
void ReverseDelayProcessor::resetLoopState() noexcept
{
    hpL.reset(); hpR.reset();
    lpL.reset(); lpR.reset();
    for (auto& ap : apL) ap.reset();
    for (auto& ap : apR) ap.reset();
}

void ReverseDelayProcessor::releaseResources()
{
    // Buffers are modest (capture ring kCaptureSeconds = 14 s stereo, ~5.4 MB at
    // 48 kHz); keep them allocated so a transport stop/start cycle never
    // reallocates. Nothing to do here.
}

bool ReverseDelayProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in.isDisabled() || out.isDisabled())
        return false;

    const bool inOk  = in  == juce::AudioChannelSet::mono() || in  == juce::AudioChannelSet::stereo();
    const bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();

    // Accept mono→mono, mono→stereo, stereo→stereo; reject stereo→mono (no down-mix path)
    return inOk && outOk && in.size() <= out.size();
}

// v1.7.2 (WR-03) — cutoff smoother advance + damping coefficient refresh.
//
// Extracted from processBlock's old once-per-block step 2 so it can be called on
// the kCoeffUpdateSamples grid instead. The body is unchanged apart from taking
// the advance length as an argument: skip() still returns the smoothed value
// AFTER the requested span, and both values are still clamped clear of the tan()
// prewarp region (highCut jlimit 500..0.49*fs per contract) before reaching
// makeLowPass/makeHighPass.
//
// Bitwise inert while the cutoffs are STATIC, which is every existing probe and
// every session that does not automate them: once a SmoothedValue has reached its
// target, skip(n) returns that target for any n, so 30 calls of skip(32) and one
// call of skip(960) land on the identical float and the exactlyEqual guards then
// suppress every recompute after the first.
void ReverseDelayProcessor::updateDampingCoefficients (int numSamples) noexcept
{
    const float fsF = static_cast<float>(currentSampleRate);
    const float lc  = juce::jlimit(20.0f,  0.49f * fsF, lowCutSmoothed.skip(numSamples));
    const float hc  = juce::jlimit(500.0f, 0.49f * fsF, highCutSmoothed.skip(numSamples));

    // Cached-cutoff guards gate ONLY the recompute (no enabled flag exists).
    // ArrayCoefficients returns a stack std::array; operator= assigns the
    // normalised values in place into the existing Coefficients — no allocation.
    if (! juce::exactlyEqual (lc, lastLowCut))
    {
        lastLowCut = lc;
        const auto a = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(currentSampleRate, lc);
        *hpL.coefficients = a;
        *hpR.coefficients = a;
    }
    if (! juce::exactlyEqual (hc, lastHighCut))
    {
        lastHighCut = hc;
        const auto a = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(currentSampleRate, hc);
        *lpL.coefficients = a;
        *lpR.coefficients = a;
    }
}

void ReverseDelayProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();

    // `<= 0` rather than `== 0`: getNumSamples() should never be negative, but
    // every loop and every division below keys off this value, so the guard costs
    // nothing and covers the case rather than assuming it away.
    if (numSamples <= 0 || buffer.getNumChannels() <= 0)
        return;

    const int numInputChannels  = juce::jmin(getTotalNumInputChannels(),  buffer.getNumChannels());
    const int numOutputChannels = juce::jmin(getTotalNumOutputChannels(), buffer.getNumChannels());

    // Defensive: prepareToPlay never ran. Bail without touching the wet path —
    // but pass DRY through rather than emitting whatever the extra output
    // channels happen to hold.
    //
    // (C: the v1.0.0 bare `return` did already leave channel 0 dry — the review's
    // "bails to total silence" reading is wrong. What it genuinely leaked is the
    // mono->stereo case, where output channel 1 is never written by this plugin
    // and would carry stale host memory. Duplicating dry closes that.)
    if (capture.getBufferSize() == 0 || wetScratch.getNumSamples() <= 0)
    {
        for (int ch = numInputChannels; ch < numOutputChannels; ++ch)
            buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);

        return;
    }

    // v1.12.2: a host block LARGER than prepared is processed in chunks of the
    // prepared size rather than bailed to dry. The old bail passed dry at unity
    // whatever Mix said, and stalled the ring, the scheduler and the feedback
    // loop for that block — so the tail jumped a block's worth on the next one.
    //
    // Each chunk is a full processBlock over a buffer that REFERS to a slice of
    // the host's channels: the pointer-array constructor with ≤ 32 channels uses
    // AudioBuffer's preallocated channel space, so nothing is allocated here.
    // Recursion depth is exactly one — every chunk fits wetScratch. Parameters
    // are re-read per chunk, which is what the host would have delivered had it
    // split the block itself; at a static setting a 2N block renders bitwise the
    // same as two N blocks (probe BG).
    if (numSamples > wetScratch.getNumSamples())
    {
        const int maxChunk = wetScratch.getNumSamples();

        for (int start = 0; start < numSamples; start += maxChunk)
        {
            juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(),
                                            buffer.getNumChannels(), start,
                                            juce::jmin (maxChunk, numSamples - start));
            processBlock (chunk, midiMessages);
        }

        return;
    }

    // ---- (0) once-per-block parameter reads (atomic) ------------------------
    // Latched-per-grain parameters — read raw, NEVER smoothed (latching at
    // spawn is the click-free mechanism per contract).
    const float delayTimeMs = pDelayTime->load();
    const float grainSizeMs = pGrainSize->load();
    const float densityPct  = pDensity->load();
    const float widthNorm   = pWidth->load() * 0.01f;
    const bool  syncMode    = pSyncMode->load() >= 0.5f;

    // v1.1.0 (B3) — also latched per grain, also never smoothed. Each is used
    // only through a `> 0` gate, so at the shipped defaults none of them touches
    // the shared xorshift and the render is bit-identical to v1.0.1.
    const float jitterNorm   = pJitter->load() * 0.01f;
    const float scatterMs    = pDelayScatter->load();
    const float sizeRandNorm = pSizeRandom->load() * 0.01f;
    const float gainRandNorm = pGainRandom->load() * 0.01f;

    // v1.2.0 (B1) — grain window, also latched per grain, also never smoothed.
    // Smoothing a WINDOW SHAPE is not merely unnecessary, it is meaningless:
    // two windows disagree at every phase, so a crossfade between them still
    // steps a live grain's envelope. Latching at spawn is the only click-free
    // mechanism available, and it is the one every other content parameter here
    // already uses.
    const int   shapeIdx    = juce::jlimit (0, WindowLut::kNumShapes - 1,
                                            static_cast<int> (pGrainShape->load()));
    const float tiltPeakPos = tiltToPeakPos (juce::jlimit (0.0f, 1.0f, pGrainTilt->load()));

    // v1.4.0 — Tukey taper α, latched per grain like the two above. Read
    // unconditionally rather than only when the shape is Tukey: makeTaper()
    // already returns the inactive geometry for the other four, and a
    // conditional read here would make the block's work depend on a parameter
    // in a way the harness would have to model.
    const float tukeyAlpha  = juce::jlimit (WindowLut::kTukeyTaperMin,
                                            WindowLut::kTukeyTaperMax,
                                            pTukeyTaper->load());

    // v1.6.0 (B4 #2) — the forward-grain probability, latched per grain like
    // every other content parameter. Read as a fraction; the draw that consumes
    // it is gated on `> 0`, so at the default it costs the shared xorshift
    // nothing (see the spawn handler).
    const float directionNorm = juce::jlimit (0.0f, 1.0f,
                                              pDirection->load() * 0.01f);

    // v1.6.0 (B4 #3) — the feedback-tap makeup, block rate. NOT smoothed and it
    // does not need to be: it multiplies the loop gain alongside feedbackSmoothed,
    // which IS smoothed, and a step in a gain that is already being ramped by its
    // neighbour would only be audible at a step size the 0.1 dB parameter grid
    // cannot produce. Exactly 1.0f at the 0 dB default.
    const float regenMakeup = regenMakeupGain (pRegenMakeup->load());

    // v1.7.0 (B4 #5) — the source read law, latched per grain like every other
    // content parameter. Resolved to a bool here so the spawn handler does one
    // compare rather than a float test, and so a stale/out-of-range choice index
    // can only ever mean "mono sum".
    const bool  stereoSource = pSourceMode->load() >= 0.5f;

    // v1.7.0 (B4 #6) — delay drift. Both read at block rate; the LFO itself is
    // sampled per SPAWN from the grain's own absolute position, so the value that
    // reaches a grain is latched exactly like gD and gG are. Depth is gated on
    // `> 0` inside driftMul(), so at the default no std::sin runs and the latched
    // delay is bitwise D.
    const float driftDepthNorm = juce::jlimit (0.0f, 1.0f, pDriftDepth->load() * 0.01f);
    const float driftRateHz    = juce::jlimit (kDriftRateMinHz, kDriftRateMaxHz,
                                               pDriftRate->load());

    // v1.7.0 (B4 #4) — ducking. The DEPTH is block rate; the envelope it scales
    // is per sample (see the mix loop in step 7 and kDuckKnee's note in the
    // header). Exactly 0.0f at the default, which makes the duck gain exactly
    // 1.0f and the wet multiply a bitwise no-op.
    const float duckNorm   = juce::jlimit (0.0f, 1.0f, pDuck->load() * 0.01f);
    const float duckAttack = duckCoeff (kDuckAttackSec,  currentSampleRate);
    const float duckRelease= duckCoeff (kDuckReleaseSec, currentSampleRate);

    // v1.8.0 (B4 #7-#8) — COLOUR. Read here, applied per sample in step 5.
    //
    // v1.12.2: both are now SMOOTHED (~20 ms, stepped per sample like
    // feedbackSmoothed). v1.8.0 read them at block rate on the argument that
    // neither moves quiet material — true, but loud material moves plenty, and
    // both sit INSIDE the feedback loop, so the step then recirculates:
    //
    //   * driveD: tanh(d·x)/d at x = 0.9 is 0.716 at 0 % and 0.125 at 100 %;
    //     even 40 -> 50 % steps that sample by 0.072 (about -23 dB).
    //   * diffuseMix: the output steps by Δm·(allpass − dry), which is as large
    //     as the signal itself wherever the chain has decorrelated it.
    //
    // Bitwise inert at a static setting: prepare/reset jump each smoother to
    // its target, setTargetValue() with an unchanged value starts no ramp, and a
    // SmoothedValue at rest returns its target exactly — the same float the
    // block-rate read produced. Both defaults are still exact: diffusion 0 gives
    // a dry term of exactly 1.0f and driveD 1.0f still early-outs to std::tanh.
    // Mid-ramp from Drive 0, d passes through values just above 1.0f; tanh(d·x)/d
    // is continuous in d there, so the early-out boundary is not a seam.
    diffuseSmoothed.setTargetValue (currentDiffuseMix());
    driveSmoothed.setTargetValue (driveRatio (pDrive->load()));

    // Smoothed (~20 ms) parameters — set targets once per block.
    feedbackSmoothed.setTargetValue(pFeedback->load() * 0.01f);
    mixSmoothed.setTargetValue(pMix->load() * 0.01f);
    lowCutSmoothed.setTargetValue(pLowCut->load());
    highCutSmoothed.setTargetValue(pHighCut->load());

    // v1.6.0 (B4 #1) — Freeze. The rising-edge latch and the smoother's target
    // both live after G is resolved (see "Freeze latch" below), because v1.7.2
    // floors the loop length at one grain and G is not known yet here.
    const bool frozen = pFreeze->load() >= 0.5f;

    // ---- (1) resolve grain anchor delay D -----------------------------------
    // Sync only changes the VALUE of D per block — never the spawn timing (no
    // conditional routing; the scheduler always runs the free countdown).
    // Fallback to the free delayTime when the playhead is null OR getBpm() is
    // empty (COMPAT-02). D is consumed only at spawn time, so a Sync<->Free
    // switch changes only next-spawn D — in-flight grains finish on latched
    // values (the click-free mechanism).
    float effectiveDelayMs = delayTimeMs;

    // v1.13.0: published with the meter so the UI can show the D in use.
    // Starts at `fallback` in Sync and is promoted only when a tempo arrives,
    // so every early-out of the playhead chain below reports honestly.
    auto delaySource = syncMode ? DelaySource::fallback : DelaySource::free;

    if (syncMode)
    {
        if (auto* playHead = getPlayHead())
        {
            if (const auto position = playHead->getPosition())
            {
                if (const auto bpm = position->getBpm())
                {
                    const int div = juce::jlimit(0, kNumNoteDivisions - 1,
                                                 static_cast<int>(pNoteDivision->load()));
                    const double ms = kNoteDivisions[div].beats * 60000.0 / juce::jmax(1.0, *bpm);
                    // A1: clamp tracks the delayTime range. It MUST stay in sync
                    // with kDelayTimeMaxMs — a stale literal here is exactly the
                    // v1.0.0 defect (division named but not played, silently).
                    effectiveDelayMs = static_cast<float>(
                        juce::jlimit(static_cast<double>(kDelayTimeMinMs),
                                     static_cast<double>(kDelayTimeMaxMs), ms));

                    // Either rail: 1/1 below 60 BPM hits the 4000 ms max, and
                    // 1/16T above 200 BPM hits the 50 ms floor.
                    delaySource = (ms < kDelayTimeMinMs || ms > kDelayTimeMaxMs)
                                      ? DelaySource::clamped : DelaySource::tempo;
                }
            }
        }
    }

    // ---- (1b) resolve grain length G (v1.17.0 Grain Link) ---------------------
    // Right after D, because = Delay reads it. Like D, only the VALUE moves per
    // block: G is latched at spawn, so a tempo change or a link switch reaches
    // the next grain and never a live one. Free passes the Size knob through
    // untouched, so the G expression below is bitwise v1.16.0's. The playhead is
    // only asked for a tempo in Division mode.
    const auto grainLink = static_cast<GrainLink>(
        juce::jlimit(0, 2, static_cast<int>(pGrainLink->load())));

    double grainBpm = 0.0;
    if (grainLink == GrainLink::division)
        if (auto* playHead = getPlayHead())
            if (const auto position = playHead->getPosition())
                if (const auto bpm = position->getBpm())
                    grainBpm = *bpm;

    auto grainSource = GrainSource::free;
    const float effectiveGrainMs = resolveGrainMs(grainLink, grainSizeMs, effectiveDelayMs,
                                                  static_cast<int>(pGrainDivision->load()),
                                                  grainBpm, grainSource);

    const int D = juce::jmax(1, static_cast<int>(effectiveDelayMs * 0.001 * currentSampleRate));
    const int G = juce::jmax(2, static_cast<int>(effectiveGrainMs * 0.001 * currentSampleRate));

    // ---- Freeze latch (v1.6.0 B4 #1; corrected v1.7.2 CR-02) ----------------
    // Latch the loop length on the RISING edge only. Recomputing it per block
    // would grow it as the hold proceeds (totalWritten keeps advancing while
    // frozen, because the ring is still being written — with copies), and a loop
    // whose length changes under the read head is a loop that skips.
    //
    // jmin against the capture that has actually happened is the whole point:
    // freeze 3 s after loading and the hold is a 3 s loop, not 13 s of which
    // 10 are the cleared buffer.
    //
    // v1.7.2 (CR-02): the latch now REFUSES to arm against a ring that has less
    // than one grain in it, and freezeEngaged tracks whether the latch actually
    // SUCCEEDED rather than what the parameter says. Two failures came out of the
    // old "freezeEngaged = frozen" unconditional assignment:
    //
    //  1. A session saved with Freeze engaged reopened permanently SILENT.
    //     prepareToPlay/reset clear the ring but the parameter is restored, so
    //     the first block latched jlimit(1, .., totalWritten == 0) == 1 and
    //     pushLooped(1) copies the previous slot — zero — forever. freezeEngaged
    //     was already true, so the latch never re-armed. The dry path passed
    //     through, which is why it reads as "the plugin stopped working".
    //  2. Engaging Freeze inside the first blocks after load produced a TONE:
    //     200 captured samples became a 240 Hz loop, not a wash.
    //
    // Because freezeEngaged now stays false until the latch succeeds, a Freeze
    // that arrives too early is not lost — it arms itself on the first block
    // where a grain's worth of material exists. The smoother's target follows
    // freezeEngaged (not `frozen`) for the same reason: pushCrossfaded would
    // otherwise ramp holdWeight to 1.0 against the un-latched
    // freezeLoopSamples == 1 and write the same zeros the latch guard just
    // refused to arm.
    const auto minLoopSamples = static_cast<juce::int64>(juce::jmax(2, G));

    if (frozen)
    {
        if (! freezeEngaged && capture.getTotalWritten() >= minLoopSamples)
        {
            // v1.15.0 — Freeze Length, latched here with everything else. Ring
            // reads nothing new and resolves to the v1.7.2 expression exactly.
            const auto mode = static_cast<FreezeLength>(
                juce::jlimit(0, 3, static_cast<int>(pFreezeLength->load())));

            juce::int64 modeSamples = 0;

            if (mode != FreezeLength::ring)
            {
                // Delay: the furthest back any grain spawned now could read —
                // the latched delay at its widest (drift up, scatter up) plus
                // two of the largest grain sizeRandom allows — plus the seam
                // margin. At the randomisation defaults this is D + 2G + margin.
                //
                // ms -> samples as ms·fs/1000, not ms·0.001·fs: 0.001 is inexact
                // in binary, so the product can land a hair above an integer,
                // which ceil() would round up a whole sample.
                const double reach =
                    static_cast<double>(D) * (1.0 + static_cast<double>(driftDepthNorm) * kDriftMaxFraction)
                    + static_cast<double>(juce::jmax(0.0f, scatterMs)) * currentSampleRate / 1000.0
                    + 2.0 * static_cast<double>(G) * (1.0 + static_cast<double>(juce::jlimit(0.0f, 1.0f, sizeRandNorm)))
                    + static_cast<double>(kFreezeLoopMarginMs) * currentSampleRate / 1000.0;

                modeSamples = static_cast<juce::int64>(std::ceil(reach));

                // Bars: the host's bar, Sync or not. No tempo -> stays Delay.
                if (mode == FreezeLength::oneBar || mode == FreezeLength::twoBars)
                {
                    if (auto* playHead = getPlayHead())
                    {
                        if (const auto position = playHead->getPosition())
                        {
                            if (const auto bpm = position->getBpm(); bpm && *bpm > 0.0)
                            {
                                double beatsPerBar = 4.0;
                                if (const auto sig = position->getTimeSignature();
                                    sig && sig->numerator > 0 && sig->denominator > 0)
                                    beatsPerBar = sig->numerator * 4.0 / sig->denominator;

                                const double bars = mode == FreezeLength::twoBars ? 2.0 : 1.0;
                                modeSamples = static_cast<juce::int64>(std::llround(
                                    bars * beatsPerBar * 60.0 / *bpm * currentSampleRate));
                            }
                        }
                    }
                }
            }

            freezeLoopSamples = resolveFreezeLoopSamples(mode, modeSamples,
                                                         capture.getTotalWritten(),
                                                         capture.getBufferSize(),
                                                         minLoopSamples);
            freezeEngaged = true;
        }
    }
    else
    {
        freezeEngaged = false;
    }

    freezeSmoothed.setTargetValue(freezeEngaged ? 1.0f : 0.0f);

    // A3 (v1.0.1): overlap floor raised 1 -> 2. At overlap = 1 the hop equals the
    // grain length, so Hann-windowed grains ABUT rather than overlap and the wet
    // output amplitude-modulates to full silence at every boundary — a 100 %-depth
    // 5 Hz tremolo at grainSize 200 ms, not the "denser wash" the control claims.
    // Hann reaches constant-overlap-add at hop = G/2, i.e. overlap >= 2, so the
    // bottom ~14 % of the knob was a gated-pulse region. 2 + d·6 made the whole
    // travel a smooth->dense sweep.
    //
    // B2 (v1.3.0): the 6 is no longer a literal. The span's top is grainCount,
    // so density scales into [kOverlapMin, ceiling] instead of a fixed [2, 8].
    //
    // Written as `min + d·(ceiling − min)` and NOT as the algebraically identical
    // `min·(1−d) + ceiling·d`, because only this form reproduces v1.2.0 bitwise
    // at the default ceiling: (8.0f − 2.0f) is exactly 6.0f, so the three
    // operations and their operands are the ones v1.0.1 shipped. The other form
    // is two multiplies and an add on different values and lands an ulp away —
    // which would cost the identity guarantee that makes this a MINOR bump, in a
    // way no listening test would ever surface. Same trap as kTiltTravel's map
    // at v1.2.0; probe AC asserts this one the same way probe Z1 asserted that.
    const float ceilingF        = juce::jlimit(kOverlapCeilingMin, kOverlapCeilingMax,
                                               pGrainCount->load());
    const float overlap         = kOverlapMin + (densityPct * 0.01f) * (ceilingF - kOverlapMin);
    const int   intervalSamples = juce::jmax(1, static_cast<int>(static_cast<float>(G) / overlap));

    // ── grain gain: overlap × window power compensation (v1.2.0 / B1) ────────
    // 1/sqrt(overlap) alone assumed HANN's power duty cycle — which was true
    // while Hann was the only window. It is the constant probe D freezes, and
    // the v1.0.0 CHANGELOG attributes the loop's −4.3 dB per generation to the
    // Hann² duty specifically.
    //
    // Tukey's mean square is 0.6875 against Hann's 0.375. Without compensation,
    // selecting Tukey at a fixed density would raise the wet level by 2.6 dB AND
    // slow the feedback decay by the same factor every generation — so a control
    // sold as "window shape" would be read by the user as a volume control that
    // also changes the tail length. Both norms below are exactly 1.0f at the
    // shipped (Hann, tilt 0.5), so this multiply is a bitwise no-op there.
    //
    // Two separate constants because they answer two separate questions: how
    // much power this SHAPE has relative to Hann, and how much the TILT warp
    // moved it. The second is exactly 1.0f for every symmetric shape at any
    // tilt — the warp is power-preserving by construction — so it only ever
    // does real work for Expo-Decay. Applied BEFORE the feedback tap, so the
    // loop sees the compensated level and the decay rate stays shape-independent
    // (probe Z4 measures that at feedback = 100 for all five).
    // v1.4.0: both norms are now α-aware. Tukey's power duty runs 0.994 at
    // α = 0.01 down to 0.375 at α = 1.0 — a 4.2 dB level swing — so without this
    // the taper knob would read as a volume knob, the same failure v1.2.0's
    // getShapeNorm exists to keep grainShape from having. α is ignored by the
    // other four shapes.
    const float windowNorm      = windowLuts.getShapeNorm (shapeIdx, tukeyAlpha)
                                * windowLuts.getTiltNorm  (shapeIdx, tiltPeakPos, tukeyAlpha);

    // B2 (v1.3.0): UNCHANGED, and that is a measured result rather than an
    // oversight. Raising the overlap ceiling to 16 was expected to need a
    // coherence correction here — it does not; 1/sqrt(overlap) holds the wet
    // level flat to 0.07 dB across the whole 2..16 range. The full derivation,
    // and the harness bug that first said otherwise, are in PluginProcessor.h
    // above kLegacyOverlapMax. Probe AA is the standing guard.
    const float grainGain       = (1.0f / std::sqrt(overlap)) * windowNorm;

    // ── feedback-tap trim (v1.2.0 / B1) ──────────────────────────────────────
    // windowNorm above equalises POWER, which is what the OUTPUT path needs: a
    // pass over broadband input has each grain reading a different stretch of
    // the ring, so the contributions are decorrelated and sum in power (probe
    // Z2 measures all five shapes inside 0.15 dB).
    //
    // The loop sums something else. What recirculates is the wash this engine
    // just made — self-similar material that overlapping grains read at nearby
    // offsets — so it sums closer to coherently, and a coherent sum follows the
    // window's MEAN rather than its mean square. Measured, the power-only
    // normalisation left the decay rate at feedback = 100 spanning 5.7 dB/s
    // across the five shapes, ranked exactly by amplitude duty: "window shape"
    // would have been audible as "how long the tail lasts".
    //
    // So the two paths take different constants, which the engine can express
    // because it has held separate output and feedback-tap gains since v1.1.0
    // (built for gainRandom, and the same split serves this). Exactly 1.0f at
    // the shipped (Hann, 0.5) — a value divided by itself — so the tap gains
    // stay bitwise what they were.
    //
    // B2 (v1.3.0): times the OVERLAP half of the same argument. v1.2.0 corrected
    // the loop for the window's amplitude duty but left overlap on the output's
    // power law, which is a sqrt(overlap) excess in a recirculating path — and at
    // the raised ceiling that excess turns feedback 100 into self-oscillation
    // that clipped the output (peak 1.28). Exactly 1.0f at overlap <= 8, so the
    // shipped decay is bitwise the shipped decay. See loopCountTrim().
    // v1.4.0: α-aware here too, and NOT by the same factor as the output. The
    // amplitude duty runs 0.995 -> 0.500 across α (6.0 dB) against the power
    // duty's 4.2 dB, so an α-aware output norm alone would have left 1.8 dB of
    // per-generation error in the loop — the same class of miss v1.2.0 found for
    // shape and v1.3.0 found for overlap. Probe AK measures it.
    const float loopTrim        = windowLuts.getLoopNorm (shapeIdx, tiltPeakPos, tukeyAlpha)
                                * loopCountTrim (overlap);

    // ── forward-grain output trim (v1.6.0 / B4 #2) ───────────────────────────
    // The third summing law, on the third path — and the reason it is needed is
    // exact rather than statistical: every forward grain in flight reads the
    // SAME source sample (a unit-rate read against a unit-rate write head has a
    // constant offset), so the forward subset adds in amplitude while the
    // reverse subset adds in power. Uncorrected that is +7.3 dB at overlap 8,
    // i.e. Direction would be heard as a volume control first and a character
    // control second — the same failure v1.2.0, v1.3.0 and v1.4.0 each closed
    // for shape, overlap and taper. Derivation in WindowLut::getForwardNorm.
    //
    // Applied to gLout/gRout ONLY. The feedback tap takes no direction trim at
    // all: getLoopNorm already models the loop as a coherent sum, so a forward
    // and a reverse grain contribute identically through it and the decay rate
    // does not move with the mix. Probe AN measures that instead of assuming it.
    //
    // Computed unconditionally rather than under `if (directionNorm > 0)`: it is
    // one sqrt and one divide at block rate, and a conditional would make the
    // block's work depend on a parameter in a way the harness would have to
    // model. It reaches no grain at direction 0 — nothing is latched forward —
    // so the shipped render is untouched whatever this holds.
    const float forwardTrim     = windowLuts.getForwardNorm (shapeIdx, tiltPeakPos,
                                                             tukeyAlpha, overlap);

    // Resolved once per block; both PODs are copied into each grain at spawn.
    const auto  blockTilt       = WindowLut::makeTilt (tiltPeakPos);
    const auto  blockTaper      = WindowLut::makeTaper (shapeIdx, tukeyAlpha);

    // ---- (1b) v1.1.0 randomisation amounts, resolved once per block ---------
    // Everything here is derived from block-rate parameter reads; the actual
    // draws happen per grain at spawn, so an amount that changes mid-flight
    // never re-randomises a live grain (the same click-free latching mechanism
    // that delayTime/grainSize/density/width already use).
    const int scatterSamples = static_cast<int>(scatterMs * 0.001 * currentSampleRate);

    // sizeRandom clamps into grainSize's OWN range: a randomised grain must
    // never exceed a grain the user could dial in, because kCaptureSeconds is
    // sized against kGrainSizeMaxMs. At the range endpoints the distribution is
    // therefore one-sided (at grainSize = 4000 ms the knob can only shorten),
    // which is the correct trade — the alternative is an unbounded read span.
    const int gMinSamples = juce::jmax(2,
        static_cast<int>(kGrainSizeMinMs * 0.001 * currentSampleRate));
    const int gMaxSamples = juce::jmax(gMinSamples,
        static_cast<int>(kGrainSizeMaxMs * 0.001 * currentSampleRate));

    // ── v1.7.0 (B4 #6): the ring-headroom ceiling on a latched delay ─────────
    //
    // A grain reaching gD back needs the ring to still hold gD + 2·G when its
    // LAST sample is read, so gD may never exceed bufferSize − 2·G_max − 1.
    // The static_assert in the header already guarantees the PARAMETERS cannot
    // ask for more than that — drift's +25 %, scatter's +500 ms and the 4 s
    // grain together land 500 ms inside the 14 s ring — so this clamp is a
    // guard rather than a shaper and never bites at any reachable setting.
    //
    // It exists anyway because the failure it prevents is silent. An
    // over-reaching read does not fault, does not produce a NaN and does not
    // click: it wraps onto material the writer has already overwritten, and the
    // only symptom is that long settings sound wrong. The compile-time assert
    // covers the ranges as declared; this covers the arithmetic that combines
    // them, which is where v1.5.0's near-miss actually lived.
    //
    // Derived from G_MAX rather than from this grain's own gG on purpose: a
    // bound that varied with the latched grain size would make the clamp point
    // depend on an RNG draw, and a guard that moves is a guard that cannot be
    // reasoned about. Conservative and constant is the right trade for something
    // that should never fire.
    const int maxLatchedDelay = juce::jmax(1, capture.getBufferSize() - 2 * gMaxSamples - 1);

    // gainRandom, power-normalised. For g = 1 + dev·u with u uniform on [−1,1),
    // E[g²] = 1 + dev²/3, so an un-normalised multiplier would raise wet RMS by
    // up to +0.75 dB at full travel — the knob would read as a loudness control
    // and would eat most of probe D's ±1 dB flatness budget. Dividing by the
    // root makes expected power exactly independent of the setting.
    // At gainRandNorm = 0: dev = 0, norm = 1.0f, and every grain's gLout is
    // BITWISE gL — which is what makes the defaults render bit-identical.
    const float gainRandDev  = juce::jmin(1.0f, gainRandNorm) * kMaxGainRandomDeviation;
    const float gainRandNorml = 1.0f / std::sqrt(1.0f + gainRandDev * gainRandDev / 3.0f);

    // ---- (2) smoother advance rates -----------------------------------------
    // mix advances per sample in the mix loop (step 7); feedback gain advances
    // per sample in the feedback fill (step 5).
    //
    // v1.7.2 (WR-03): the CUTOFFS no longer advance here. They used to run
    // `skip(numSamples)` once per block and hold one set of coefficients for the
    // whole block, which made the documented 20 ms smoothing contract sample
    // itself at the HOST's block rate. Two things followed, and neither was
    // measured because every invariance probe sets its parameters before
    // prepareToPlay and never moves them:
    //
    //   1. The 512-vs-4096 bit-identity was conditional on the cutoffs being
    //      static, while NOTES.md and the v1.7.0 CHANGELOG stated it flatly. A
    //      session that AUTOMATES highCut bounced differently offline than it
    //      monitored — the same class of defect the duck follower was deliberately
    //      built per-sample to avoid
    //      (pattern_block_rate_envelope_breaks_blocksize_invariance).
    //   2. At large buffers the smoother did nothing at all: a 20 ms ramp is 960
    //      samples at 48 kHz, so a 4096-sample block skipped past it in ONE step
    //      and a swept cutoff became a single large biquad coefficient jump per
    //      block instead of eight small ones — a zipper/click risk inside the
    //      feedback loop. Probe M's click detector runs at block 512, where the
    //      ramp resolves in eight steps and the artefact does not appear.
    //
    // They now advance on a fixed kCoeffUpdateSamples grid inside step 5, driven
    // by a countdown that PERSISTS ACROSS BLOCKS AND PASSES (coeffCountdown), so
    // the update grid is a function of the sample rate alone. Chunking relative to
    // each pass offset instead would have re-broken the very invariant this fixes:
    // grainDelayFloor is 2205 samples at 44.1 kHz, which is not a multiple of 32,
    // so a second pass starting there would shift the grid at 4096 while a
    // 512-sample block kept it aligned.

    // ---- (3)–(6) sub-blocked engine pass ------------------------------------
    // A2 (v1.0.1): a grain spawned at block offset i latches
    // readAbs = blockStart + i − D, but the capture write for this block only
    // happens in step 6. Every read is therefore already-written ONLY while
    // i < D. In v1.0.0 the whole host block ran as one pass, so at
    // blockSize >= D (2205 samples at 44.1 kHz / delayTime 50 ms — i.e. any
    // 2048- or 4096-sample buffer, common in offline bounce and high-latency
    // live rigs) the late grains read positions not yet written this pass:
    // silence early on, then a full ring lap of stale audio.
    //
    // Fix: bound each pass to D samples. i < D then holds by construction at
    // ANY host block size, with no change to the delay time and no partial
    // feedback (which is what "write input first, add feedback second" would
    // leave behind — the block's own regeneration is still missing there).
    // The engine becomes block-size invariant, which is what probe O asserts.
    //
    // Cost at the shipped block size is exactly zero: D >= 2400 at 48 kHz, so
    // a 512-sample block is one pass and the code path is bit-identical to
    // v1.0.0. Only oversized blocks (or very low sample rates) split.
    wetScratch.clear();
    loopScratch.clear();
    float* wetL  = wetScratch.getWritePointer(0);
    float* wetR  = wetScratch.getWritePointer(1);
    float* loopL = loopScratch.getWritePointer(0);   // v1.1.0: feedback tap, pre-gainRandom
    float* loopR = loopScratch.getWritePointer(1);
    float* fbLw  = fbScratch.getWritePointer(0);
    float* fbRw  = fbScratch.getWritePointer(1);

    // Mono input feeds both capture channels (L = R = in).
    const float* inL = buffer.getReadPointer(0);
    const float* inR = numInputChannels > 1 ? buffer.getReadPointer(1) : inL;

    // ---- pass bound (A2, extended for v1.1's delayScatter) ------------------
    // A2 bounds each pass so that a grain spawning at pass offset i reads
    // capture that is already written, which needs i < (that grain's latched
    // delay). Through v1.0.1 every grain's delay WAS D, so bounding the pass by
    // D was sufficient.
    //
    // delayScatter breaks that: a scattered grain's delay can fall below D. The
    // obvious repair — clamp the latched delay up to passLen — is wrong in a way
    // that is invisible without probe W2, because passLen is itself derived from
    // the host block size, so the clamp would latch DIFFERENT delays at 512 than
    // at 4096 samples and an offline bounce would stop matching what was
    // monitored. (It did; W2 caught it.)
    //
    // Both the bound and the clamp therefore key off grainDelayFloor, which is a
    // function of the PARAMETERS alone: the delayTime range's own minimum, i.e.
    // nothing can pull a grain below the shortest delay the plugin offers.
    // With no shortening parameter active the bound collapses to v1.0.1's exact
    // min(numSamples, D) and the render is bit-identical.
    //
    // v1.7.2 (CR-01): the test is "can ANY parameter pull a latched delay below
    // D", not "is scatter on". v1.7.0's drift multiplies D by as little as
    // (1 - kDriftMaxFraction) = 0.75, so `scatterSamples > 0` alone re-opened A2:
    // with drift at 100 %, scatter at 0 and a host block of 4096, a grain
    // spawning late in the pass on the LFO's negative half latched
    // readAbs AHEAD of the write head and read a full ring lap of stale
    // material — and rendered differently at 512 than at 4096. driftMul()
    // early-returns exactly 1.0f at depth 0, so `driftDepthNorm > 0.0f` is the
    // precise complement of "drift can shorten" and this stays bit-inert at the
    // shipped default, every factory preset and every pre-v1.7.0 session.
    const int minDelaySamples = juce::jmax(1,
        static_cast<int>(kDelayTimeMinMs * 0.001 * currentSampleRate));
    const int grainDelayFloor = juce::jmin(D, minDelaySamples);
    const bool delayMayShorten = (scatterSamples > 0) || (driftDepthNorm > 0.0f);
    const int passBound       = delayMayShorten ? grainDelayFloor : D;
    const int passLen         = juce::jmax(1, juce::jmin(numSamples, passBound));

    // v1.3.0 (B2): accumulated across the block's passes and published once at
    // the end, so the message thread sees one consistent snapshot per block
    // rather than a value that changes under it mid-block.
    int peakActiveThisBlock = 0;
    int droppedThisBlock    = 0;
    int refusedThisBlock    = 0;

    for (int off = 0; off < numSamples; )
    {
        const juce::int64 passStartAbs = capture.getTotalWritten();   // capture write happens in step 6

        // v1.12.1: the bound above covers every grain spawned from here on, but
        // not a FORWARD grain carried in from an earlier pass. Its latched gD is
        // whatever D (or scatter, or drift) was when it spawned, and passBound
        // only drops to grainDelayFloor while a shortening parameter is on NOW.
        // So with Direction > 0 and scatter/drift at 0, a delay that GROWS —
        // knob, automation, or a tempo drop in Sync — lifts the bound to the new
        // D while a live forward grain still reads `t − gD_old`; once the pass
        // outruns gD_old that read lands on a slot this pass has not written
        // yet, i.e. a full ring lap (~14 s) of stale audio, into the wet AND the
        // loop. The same hole opens when scatter or drift is switched off while
        // scattered forward grains are still sounding.
        //
        // Fix: cap the pass at the smallest remaining lag over live forward
        // grains. A carried forward grain's next read is g.readAbs, and it reads
        // strictly behind the write head for exactly `passStartAbs − g.readAbs`
        // (= its gD) more samples. Reverse grains move away from the write head
        // and never constrain the pass. The cap is recomputed every pass because
        // the live set changes between passes.
        //
        // Block-size invariance survives it: the pass partition was already free
        // to vary with the host block (512 vs 4096 differ in it by construction),
        // and every piece of state that crosses a pass boundary is sample-exact.
        // Bit-inert whenever no forward grain is live — Direction 0, every factory
        // preset at 0, and every pre-v1.6.0 session — and whenever every live
        // forward grain's gD >= passBound, which is the steady state at any
        // Direction: the cap only bites in the transient after a delay increase.
        int thisPassLen = passLen;
        for (const auto& g : grainPool.grains)
            if (g.active && g.step > 0)
                thisPassLen = juce::jmin(thisPassLen,
                                         static_cast<int>(juce::jmin<juce::int64>(
                                             passStartAbs - g.readAbs, passLen)));
        thisPassLen = juce::jmax(1, thisPassLen);

        const int len = juce::jmin(thisPassLen, numSamples - off);
        const int passEnd = off + len;

        // ---- (3) schedule spawns, latch per-grain state ---------------------
        // Offsets come back pass-relative; startOffset stays block-relative so
        // the render loop below indexes the shared scratch buffers directly.
        // jitterRngState, NOT rngState: the scheduler consumes its stream once
        // per spawn from inside a per-sample countdown, while the loop below
        // consumes the grain stream after the whole pass has been scheduled.
        // Sharing one stream would interleave the two differently at different
        // pass lengths — i.e. differently at 512 than at 4096 samples — and the
        // engine would stop being block-size invariant (probe W2).
        //
        // v1.3.0: the scheduler now also reports what its fixed request array
        // could not hold. Unreachable at these ranges by 8x (see
        // kMaxSpawnsPerBlock's derivation) — which is exactly why it is counted
        // rather than trusted, since "unreachable" was also true of the 32-cap it
        // replaced and nothing measured that either.
        const auto spawn = scheduler.processBlock(len, intervalSamples, jitterNorm,
                                                  spawnRequests,
                                                  [this] { return nextJitterRand01(); });
        droppedThisBlock += spawn.dropped;

        for (int s = 0; s < spawn.count; ++s)
        {
            const int passOffset = spawnRequests[static_cast<size_t>(s)].sampleOffset;

            // ── per-grain randomisation, drawn BEFORE the slot is requested ──
            // Draw order is fixed (scatter -> size -> gain -> pan) so a render
            // is reproducible from the seed. Each draw is gated on its own
            // amount, so enabling one does NOT shift the others' sequences and
            // all-zero consumes nothing at all.
            //
            // Drawn before obtain() ON PURPOSE: a refused spawn must consume
            // exactly what a granted one consumes, or RNG consumption would
            // depend on pool occupancy — and occupancy at a given spawn depends
            // on how many render passes have run, i.e. on the host block size.
            // The few wasted draws buy an unconditional invariant.

            // ── v1.7.0 (B4 #6): delay drift, sampled at this grain's own
            // absolute spawn position ─────────────────────────────────────────
            //
            // The LFO phase comes from `passStartAbs + passOffset`, which is the
            // capture ring's monotonic write position — i.e. the grain's
            // absolute sample time — rather than from a phase accumulated per
            // block. That is what makes drift block-size invariant: the same
            // spawn lands at the same absolute sample at 512 and at 4096, so it
            // sees the same phase exactly, where an accumulator advanced eight
            // times as often would land a few ulps apart and probe AV demands
            // bit equality (the property probes O and W2 established).
            //
            // Applied BEFORE scatter so the scatter stays symmetric about the
            // drifted delay rather than about the nominal one — the two controls
            // then compose as "wander, and smear around wherever you are"
            // instead of fighting over the same anchor. It consumes no RNG, so
            // every existing draw sequence is untouched (probe T's property).
            const juce::int64 spawnAbs = passStartAbs + static_cast<juce::int64>(passOffset);
            const float driftedD = static_cast<float>(D)
                                     * driftMul(driftDepthNorm, driftRateHz,
                                                spawnAbs, currentSampleRate);

            // delayScatter: ±scatterSamples around D. Symmetric, so the MEAN
            // latched delay stays D and the rhythmic anchor does not move —
            // only the thickness of the smear around it changes.
            int gD = static_cast<int>(driftedD);
            if (scatterSamples > 0)
                gD += static_cast<int>(static_cast<float>(scatterSamples)
                                       * (2.0f * nextRand01() - 1.0f));

            // See the pass-bound note above: clamped to grainDelayFloor, which
            // depends only on the parameters, so the latched value is identical
            // at every host block size. passLen <= grainDelayFloor, so this also
            // preserves A2's `i < gD` guarantee.
            //
            // v1.7.0 adds the ceiling: drift can only push gD UP, and the ring
            // has to still hold gD + 2·G when the grain's last sample is read.
            // Unreachable at every legal setting (see maxLatchedDelay) and kept
            // because the failure it guards is inaudible until it is not.
            gD = juce::jlimit(grainDelayFloor, juce::jmax(grainDelayFloor, maxLatchedDelay), gD);

            // sizeRandom: ±% on G, clamped into grainSize's own range.
            int gG = G;
            if (sizeRandNorm > 0.0f)
                gG = juce::jlimit(gMinSamples, gMaxSamples,
                                  static_cast<int>(static_cast<float>(G)
                                                   * randomMul(juce::jmin(1.0f, sizeRandNorm))));

            // gainRandom: bounded, power-normalised, applied ONLY to the output
            // pan gains below — never to gL/gR, which is what the feedback tap
            // reads. jmax(0) guards the (unreachable at dev <= 0.75) negative
            // multiplier that would invert a grain's polarity.
            const float gainRand = gainRandDev > 0.0f
                                     ? juce::jmax(0.0f, randomMul(gainRandDev)) * gainRandNorml
                                     : 1.0f;

            // Width spread: per-grain equal-power pan, latched at spawn, never
            // smoothed. Alternating-sign random bias — consecutive grains ping
            // left/right; magnitude in [kPanBias, 1] scaled by width. width=0
            // collapses to pan 0.5 -> gL = gR = 1/sqrt(2) (centered dual-mono).
            // panSign flips here rather than after the slot check for the same
            // reason the draws happen here: it is sequence state.
            panSign = -panSign;
            const float spread = panSign * (kPanBias + (1.0f - kPanBias) * nextRand01());
            const float pan    = 0.5f + widthNorm * 0.5f * spread;
            const float phase  = pan * juce::MathConstants<float>::halfPi;

            // v1.6.0 (B4 #2): the direction draw, LAST in the sequence.
            //
            // Its position is deliberate. Every draw here is gated on its own
            // amount, so an amount at zero consumes nothing — but the ones that
            // are on still consume in a fixed ORDER, which means enabling any
            // randomisation shifts the streams of every draw AFTER it. Appending
            // direction at the end is the only placement that leaves all four
            // v1.1 sequences and the pan sequence bit-for-bit where they were,
            // so a v1.5.0 session that turns Direction up gets its existing
            // scatter/size/gain/pan character back unchanged with forward grains
            // added to it, rather than a re-rolled cloud.
            //
            // Before obtain(), like everything above it: a refused spawn must
            // consume exactly what a granted one does, or RNG consumption would
            // depend on pool occupancy and the engine would stop being
            // block-size invariant (probe W2).
            const bool forward = directionNorm > 0.0f
                              && nextRand01() < directionNorm;

            // v1.1.0: REFUSE, never steal. A stolen slot's Hann envelope jumps
            // from mid-window to zero in one sample; a refused spawn just leaves
            // the overlap-add one contributor lighter for that window. See
            // GrainPool::obtain(). The scheduler countdown already advanced, so
            // dropping the grain does not shift the spawn grid either.
            auto* slot = grainPool.obtain();
            if (slot == nullptr)
            {
                ++refusedThisBlock;   // v1.3.0: counted, so probe AB can report it
                continue;
            }

            auto& g = *slot;

            g.active      = true;
            g.readAbs     = (passStartAbs + static_cast<juce::int64>(passOffset)) - static_cast<juce::int64>(gD);
            g.n           = 0;
            g.G           = gG;
            g.invG        = 1.0f / static_cast<float>(gG);
            g.gain        = grainGain;
            g.startOffset = off + passOffset;

            // B1: window shape + tilt, latched like everything else above. No
            // RNG is consumed here — the window is deterministic — so the two
            // xorshift streams' consumption per spawn is UNCHANGED from v1.1.0
            // and probes T (zero-determinism) and W2 (block-size invariance)
            // stay valid without re-tuning.
            g.shape       = shapeIdx;
            g.tiltT       = blockTilt.t;
            g.tiltA       = blockTilt.a;
            g.tiltB       = blockTilt.b;

            // v1.4.0: taper geometry, resolved at block rate and latched here.
            // Consumes no RNG, exactly as the window did at v1.2.0, so probes T
            // and W2 stay valid without re-tuning.
            g.taperActive = blockTaper.active;
            g.taperInv    = blockTaper.invTaperEnd;

            // The two paths diverge here, and BOTH divergences are latched:
            //   gL / gR       — feedback tap: pan × loopTrim, no gainRandom.
            //   gLout / gRout — output:       pan × gainRandom, no loopTrim.
            //
            // Neither trim may cross over. loopTrim in the output would undo the
            // power normalisation Z2 asserts; gainRandom in the loop would make
            // the decay rate stochastic (probe X). Both multipliers are exactly
            // 1.0f at their no-op — Hann/tilt-0.5 and gainRandom 0 — so at the
            // shipped defaults all four gains are BITWISE the raw pan values.
            //
            // v1.6.0 adds a THIRD multiplier and puts it on the output side
            // only. forwardTrim equalises the forward set's coherent sum against
            // the reverse set's incoherent one; loopTrim equalises the loop,
            // which is coherent for BOTH directions and therefore needs nothing
            // extra. Crossing them over would break each path in the way its own
            // normalisation exists to prevent.
            const float panL = std::cos(phase);
            const float panR = std::sin(phase);

            g.gL = panL * loopTrim;
            g.gR = panR * loopTrim;

            const float outTrim = forward ? gainRand * forwardTrim : gainRand;

            g.gLout = panL * outTrim;
            g.gRout = panR * outTrim;

            // −1 reverse, +1 forward. The whole of B4 #2 in the engine.
            g.step = forward ? 1 : -1;

            // v1.7.0 (B4 #5): the source channel, latched. Follows panSign — the
            // grain's pan SIDE — rather than the resolved pan position, because
            // at width 0 every pan is exactly 0.5 and a position test would send
            // every grain to the same channel (see ReverseGrain::srcCh).
            //
            // panSign was flipped above, before the pan was computed, so the sign
            // read here is this grain's: +1 pans right and reads R, −1 pans left
            // and reads L. That correspondence is the whole feature — it is what
            // makes Stereo preserve the image instead of just decorrelating it.
            //
            // Consumes no RNG, exactly as the window and taper latches do, so
            // probes T and W2 stay valid without re-tuning.
            g.srcCh = stereoSource ? (panSign > 0.0f ? 1 : 0) : -1;
        }

        // ---- (4) render active grains into wetScratch (overlap-add) ---------
        for (auto& g : grainPool.grains)
        {
            if (!g.active)
                continue;

            // Grains spawned in THIS pass carry a block-relative startOffset
            // inside [off, passEnd); grains carried over from an earlier pass
            // or block have it cleared to 0 and resume at the pass start.
            const int start = juce::jmax(off, g.startOffset);
            const int end   = juce::jmin(passEnd, start + (g.G - g.n));
            g.startOffset   = 0;

            if (end <= start)
                continue;

            juce::int64 readAbs = g.readAbs;
            int         n       = g.n;

            // v1.6.0 (B4 #2): the read direction, hoisted to a register like
            // every other per-grain constant. −1 is the shipped law and
            // `readAbs += (-1)` is the same integer operation `--readAbs` was, so
            // this costs the reverse path nothing measurable and nothing at all
            // in the result.
            const juce::int64 step = g.step;

           #if OUARICON_RENDER_HARNESS
            // v1.12.1: HARNESS ONLY — count every grain-pass whose furthest read
            // lands at or past this pass's write head, i.e. on a slot not yet
            // written. Evaluated from the grain's endpoints, outside the inner
            // loop, so the harness build's hot path is the shipped one.
            {
                const juce::int64 last = readAbs + step * static_cast<juce::int64>(end - start - 1);
                if (juce::jmax(readAbs, last) >= passStartAbs)
                    ++harnessUnwrittenReads;
            }
           #endif

            // v1.7.0 (B4 #5): the grain's latched source channel, hoisted for the
            // same reason `step` is. −1 selects the mono sum, which keeps the
            // shipped path bitwise what it was: monoSum() is called unchanged
            // rather than being re-expressed as a weighted read, and 0.5·(L+R) is
            // NOT bit-equal to 0.5·L + 0.5·R once denormals are in play.
            //
            // The per-sample branch is on a per-grain constant, so it is either
            // perfectly predicted or hoisted out of the loop entirely by loop
            // unswitching — the same argument readShaped's taper branch has made
            // since v1.4.0.
            const int srcCh = g.srcCh;

            const float invG = g.invG, gain = g.gain;
            const float gL = g.gL,    gR = g.gR;       // feedback tap — never randomised
            const float gLo = g.gLout, gRo = g.gRout;  // output — includes gainRandom

            // B1: the grain's OWN latched window, resolved once per pass. The
            // table pointer is hoisted out of the per-sample loop so a five-shape
            // bank costs the inner loop nothing over v1.1's single table, and
            // the three tilt coefficients become plain registers.
            // v1.4.0: a Tukey grain reads the HANN table, because its taper IS a
            // Hann half and a variable α is served by remapping the phase into it
            // rather than by a table per α (WindowLut header). Resolved here, out
            // of the per-sample loop, so the choice costs the loop nothing.
            const WindowLut::Taper taper { g.taperActive, g.taperInv };
            const float* const win = windowLuts.getTable (taper.active ? WindowLut::hann
                                                                       : g.shape);
            const float tT = g.tiltT, tA = g.tiltA, tB = g.tiltB;

            // Branch-free inner loop: LUT lerp + mul-adds, per-grain constants
            // precomputed at spawn. Integer reverse read: readAbs steps −1 while
            // the write head advances +1 → net offset growth D+2n.
            //
            // v1.1.0 accumulates TWO buffers so gainRandom can sit downstream of
            // the feedback tap (step 5 reads loop*, step 7 reads wet*). Kept
            // unconditional rather than branched on "is gainRandom on": a
            // block-rate branch would re-gain grains that were latched while it
            // was on, which is precisely the click the latching exists to avoid.
            // Cost is two extra mul-adds per grain-sample — at overlap <= 8 and
            // 48 kHz, a few Mflop/s.
            for (int i = start; i < end; ++i)
            {
                const float src = srcCh < 0 ? capture.monoSum(readAbs)
                                            : capture.readAbs(srcCh, readAbs);

                // B1 tilt: two branchless segments mapping [0,t] -> [0,0.5] and
                // [t,1] -> [0.5,1]. At the default t = 0.5 both coefficients are
                // exactly 1.0f and this reduces to `p` BITWISE — min/max, not a
                // near-identity multiply — which is what preserves the shipped
                // render exactly. See WindowLut.h for the proof.
                const float p   = static_cast<float>(n) * invG;
                const float q   = juce::jmin(p, tT) * tA + juce::jmax(p - tT, 0.0f) * tB;

                // B1 tilt warped the phase; v1.4.0's taper warps it AGAIN, for
                // Tukey only, and then both read one table. readShaped's branch
                // is on a per-grain constant, so it is perfectly predicted, and
                // it reduces to a plain readAt for the other four shapes.
                const float env = windowLuts.readShaped(win, taper, q);
                const float v   = src * env * gain;
                wetL[i]  += v * gLo;
                wetR[i]  += v * gRo;
                loopL[i] += v * gL;
                loopR[i] += v * gR;
                readAbs += step;
                ++n;
            }

            g.readAbs = readAbs;
            g.n       = n;

            if (g.n >= g.G)
                g.active = false;
        }

        // ---- (5) feedback return: loop → fbGain → HP → LP → tanh → guard ----
        // Tap is post grain-gain (overlap compensation already applied in step 4 —
        // loop gain stays density-independent), pre width/mix (they never enter
        // the loop). tanh bounds the loop to ±1 at any feedback setting.
        //
        // v1.1.0: reads loop*, NOT wet*. gainRandom must not reach the loop —
        // a randomised gain inside a recirculating path compounds every
        // generation, so the knob would control how fast the tail dies rather
        // than how it shimmers, and at feedback = 100 it would make the decay
        // rate itself stochastic. At gainRandom = 0 loop* holds bitwise the same
        // values as wet*, so this is a no-op against v1.0.1.
        {
            float acc = 0.0f;   // NaN detector: tanh output is bounded, so only NaN can escape

            // v1.6.0 (B4 #3): the makeup rides WITH the feedback gain, i.e. ahead
            // of the damping filters and ahead of the tanh. That ordering is the
            // safety argument — the limiter stays the last thing in the loop, so
            // no makeup setting can put an unbounded value into the capture ring
            // — and it is also the musical one, since driving the tanh harder is
            // what turns the ceiling from a safety net into saturation character.
            // Exactly 1.0f at the 0 dB default, so `g * regenMakeup` is bitwise g.
            //
            // v1.7.2 (WR-03): chunked on the cutoff control grid. The inner loop
            // is byte-for-byte what it was; the only change is that the damping
            // coefficients are refreshed every kCoeffUpdateSamples samples of
            // CONTINUOUS stream time rather than once per host block.
            for (int i = off; i < passEnd; )
            {
                if (coeffCountdown <= 0)
                {
                    updateDampingCoefficients (kCoeffUpdateSamples);
                    coeffCountdown = kCoeffUpdateSamples;
                }

                const int n = juce::jmin (coeffCountdown, passEnd - i);

                for (int e = i + n; i < e; ++i)
                {
                    const float g          = feedbackSmoothed.getNextValue() * regenMakeup;
                    const float diffuseMix = diffuseSmoothed.getNextValue();   // v1.12.2
                    const float driveD     = driveSmoothed.getNextValue();
                    float l = hpL.processSample(loopL[i] * g);
                    float r = hpR.processSample(loopR[i] * g);
                    l = lpL.processSample(l);
                    r = lpR.processSample(r);

                    // v1.8.0 (B4 #7) — diffusion, AFTER the damping filters and
                    // BEFORE the saturator. Both halves of that placement matter:
                    //
                    //   * After damping, because an allpass chain fed the raw loop
                    //     smears content the low-pass is about to remove anyway,
                    //     and the diffuser should blur what survives the loop, not
                    //     what is on its way out of it.
                    //   * Before the saturator, because the limiter must stay the
                    //     LAST thing in the loop — the invariant v1.6.0's makeup
                    //     was placed around, restated. It is also the physical
                    //     ordering: a chamber diffuses and the amplifier then
                    //     saturates, not the reverse.
                    //
                    // The mix is what makes this safe inside a recirculating path,
                    // and the bound is exact rather than empirical. An allpass has
                    // unit magnitude at every frequency, so the block's response is
                    // |(1-m) + m·e^{jφ}| <= (1-m) + m = 1 for every m in [0,1] and
                    // every φ. Diffusion is therefore NON-EXPANSIVE at every
                    // setting: it cannot raise the loop gain at any frequency and
                    // so cannot open a self-oscillation path that `feedback` alone
                    // would not. That is why it carries no measured cap where
                    // kRegenMakeupMaxDb needed one.
                    //
                    // The chain runs even at m = 0, on the same reasoning the duck
                    // follower does: two mul-adds per section keeps its memory
                    // warm, so raising the knob mid-session blurs the tail that is
                    // actually sounding rather than splicing in ~48 ms of silence.
                    // Nothing it computes reaches the loop at 0 — the dry term is
                    // exactly 1.0f·l, which is bitwise l.
                    {
                        float dl = l, dr = r;
                        for (size_t s = 0; s < apL.size(); ++s)
                        {
                            dl = apL[s].process (dl);
                            dr = apR[s].process (dr);
                        }

                        l += diffuseMix * (dl - l);
                        r += diffuseMix * (dr - r);
                    }

                    // v1.8.0 (B4 #8) — the saturator, now level-compensated.
                    // driveShape(x, 1.0f) is std::tanh(x), so at the default this
                    // is the line it replaced. See driveShape() for why dividing
                    // by d is what makes this a colour control and not a second
                    // regenMakeup.
                    l = driveShape (l, driveD);
                    r = driveShape (r, driveD);

                    fbLw[i] = l;
                    fbRw[i] = r;
                    acc += l + r;
                }

                coeffCountdown -= n;
            }

            // Non-finite guard at the loop write point: reset BOTH filter pairs AND
            // zero the feedback source for this pass; keep last-known-good
            // coefficients (sticky-silence pattern — never reset only the filters).
            //
            // v1.8.0: the allpass chain is reset here too, and it has to be. It is
            // the one v1.8.0 state that RECIRCULATES — each section feeds its own
            // output back through `g`, so a single non-finite sample written into
            // buf survives every subsequent block and reproduces itself forever,
            // exactly the way an envelope follower's state does
            // (pattern_envelope_follower_state_sticky_nan). Clearing the feedback
            // source without clearing the allpasses would leave the guard looking
            // like it worked while the chain kept re-poisoning the loop.
            if (! std::isfinite(acc))
            {
                resetLoopState();
                juce::FloatVectorOperations::clear(fbLw + off, len);
                juce::FloatVectorOperations::clear(fbRw + off, len);
            }
        }

        // ---- (6) capture write: input + feedback return ---------------------
        //
        // ── v1.6.0 (B4 #1): Freeze ────────────────────────────────────────────
        // The whole feature is here: while frozen the ring is not written, and
        // the grains — which are not told anything — go on reading it. Nothing
        // else in the engine changes.
        //
        // Three points worth stating, because each is a thing the obvious
        // implementation gets wrong:
        //
        //   * The ring keeps being WRITTEN, with a copy of itself
        //     freezeLoopSamples back (CaptureBuffer::pushLooped). Neither
        //     stopping the head nor advancing it without writing works — the
        //     first turns the hold into a buzz at the spawn interval, the second
        //     falls silent as soon as the read passes the last captured sample.
        //     Both failure modes are written up on pushLooped(); the second is
        //     the one probe AP found rather than the one that was anticipated.
        //
        //   * The feedback return is COMPUTED and then DISCARDED, not skipped.
        //     Step 5 above runs unconditionally, so the damping filters keep
        //     tracking the wash and are in the right state when writing resumes;
        //     skipping it would leave stale filter memory to be spliced in on
        //     release. Discarding rather than writing it is what makes a freeze
        //     genuinely infinite: writing the return would overwrite the held
        //     material with a decaying copy of itself, so a "freeze" would fade
        //     out at whatever rate `feedback` happened to be set to.
        //
        //   * The transition is a CONTENT crossfade, not a gain ramp. Writing
        //     `input · 0` would erase the ring, which is the opposite of holding
        //     it, so the ramp has to blend against the material being looped —
        //     which also makes the loop's own seam a crossfade.
        //
        // At freeze off and not smoothing this is the v1.5.0 loop, unchanged and
        // uncalled-through — the branch costs one predicted compare per pass.
        if (freezeSmoothed.isSmoothing() || freezeSmoothed.getCurrentValue() > 0.0f)
        {
            for (int i = off; i < passEnd; ++i)
                capture.pushCrossfaded(inL[i] + fbLw[i], inR[i] + fbRw[i],
                                       freezeLoopSamples,
                                       freezeSmoothed.getNextValue());
        }
        else
        {
            for (int i = off; i < passEnd; ++i)
                capture.pushSample(inL[i] + fbLw[i], inR[i] + fbRw[i]);
        }

        // Sampled at the END of each pass, after step 4 has retired the grains
        // that finished in it — so this is concurrency actually rendered, not a
        // count inflated by slots about to be freed. PEAK across the block rather
        // than the final value: at low density the block boundary lands between
        // spawns and the instantaneous count reads low, which would make the UI
        // meter flicker toward zero on material that is plainly still washing.
        peakActiveThisBlock = juce::jmax(peakActiveThisBlock, grainPool.countActive());

        off += len;
    }

    // ---- (6b) publish the meter + spawn accounting (v1.3.0 / B2) ------------
    // Relaxed stores, once per block, read by the editor's 15 Hz poll. Nothing in
    // the audio path reads these back, so there is no ordering to establish.
    publishedActiveGrains.store(peakActiveThisBlock, std::memory_order_relaxed);
    publishedOverlap.store(overlap, std::memory_order_relaxed);

    // v1.13.0: what the delay and the Freeze latch are actually doing, which is
    // not what their parameters say — see DelaySource and freezeEngaged.
    publishedDelayMs.store(effectiveDelayMs, std::memory_order_relaxed);
    publishedDelaySource.store(static_cast<int>(delaySource), std::memory_order_relaxed);
    publishedFreezeEngaged.store(freezeEngaged, std::memory_order_relaxed);
    publishedGrainMs.store(effectiveGrainMs, std::memory_order_relaxed);                   // v1.17.0
    publishedGrainSource.store(static_cast<int>(grainSource), std::memory_order_relaxed);

    // Cumulative, and only touched when non-zero: the common case is two loads
    // and no stores rather than an unconditional read-modify-write per block.
    if (droppedThisBlock > 0)
        droppedSpawns.fetch_add(static_cast<juce::uint32>(droppedThisBlock),
                                std::memory_order_relaxed);

    if (refusedThisBlock > 0)
        refusedSpawns.fetch_add(static_cast<juce::uint32>(refusedThisBlock),
                                std::memory_order_relaxed);

    // ---- (7) equal-power dry/wet mix + duck ---------------------------------
    // Dry comes from the untouched input buffer (wet never rendered in-place).
    //
    // ── v1.7.0 (B4 #4): the duck follower lives HERE ─────────────────────────
    // This loop is the last place in the block where the dry input is still
    // readable — everything upstream reads it, this is what overwrites it — so
    // running the follower inline costs no second pass over the buffer and no
    // copy of the dry signal.
    //
    // Per SAMPLE, with coefficients computed once per block. A block-rate
    // envelope would be cheaper and is what the obvious implementation does; it
    // would also make the duck a function of the host's buffer size, so an
    // offline bounce at 4096 samples would duck 75 ms later than the same
    // session monitored at 512, and probes O/W2/AV could no longer assert bit
    // equality. See kDuckKnee in the header for the full argument.
    //
    // The duck multiplies the WET ONLY. The feedback tap is upstream of this
    // loop and never sees it, which is what keeps the tail's decay rate a
    // property of `feedback` rather than of what the player happens to be doing
    // — the same output/loop split gainRandom (v1.1.0), loopTrim (v1.2.0) and
    // forwardTrim (v1.6.0) each rely on.
    constexpr float halfPi = juce::MathConstants<float>::halfPi;

    // Rectified stereo mean, one-pole with asymmetric attack/release, then the
    // compressive map. `duckNorm == 0` makes this exactly 1.0f whatever the
    // envelope holds, so the wet multiply below is a bitwise no-op at the
    // default — and the divide is skipped on a branch the compiler hoists,
    // because duckNorm is a block constant.
    auto advanceDuck = [&] (float dryL, float dryR) noexcept
    {
        const float rect = 0.5f * (std::abs (dryL) + std::abs (dryR));
        const float c    = rect > duckEnv ? duckAttack : duckRelease;

        duckEnv = rect + c * (duckEnv - rect);

        // ── The follower is the one piece of state here that cannot heal ──────
        //
        // duckEnv is the plugin's first PERSISTENT audio state a pathological
        // input can poison permanently, and that is a property of the follower
        // rather than of ducking. Once it holds a non-finite value the update
        // above reproduces one every sample for the life of the instance:
        // `rect + c*(NaN - rect)` is NaN for any finite rect, so a single bad
        // input sample is forever. An infinity gets there by a second route —
        // inf/(inf + knee) is NaN, not 1 — so both flavours have to be caught.
        //
        // Everything else in the engine already recovers on its own schedule and
        // that is exactly why this stood out: the capture ring ages a bad sample
        // out after one lap, and the feedback loop's isfinite guard resets the
        // damping filters within the pass. Measured, before this guard existed:
        // a 10 ms NaN/inf burst left duck 0 fully recovered by 20 s (rms 0.0638)
        // and duck 80 reading NaN for the rest of the render. Probe AY is the
        // standing check, and it measures duck 80 AGAINST duck 0 rather than
        // against an absolute — the shipped path's recovery is the standard.
        //
        // Reset to 0, i.e. un-ducked, which is the safe direction: garbage in the
        // envelope must not attenuate the wet. Costs one bit test per sample in a
        // loop that already evaluates cos and sin, and it cannot fire for finite
        // input, so the duck-0 identity and the block-size invariance probe AX
        // asserts are both untouched.
        if (! std::isfinite (duckEnv))
            duckEnv = 0.0f;

        if (duckNorm <= 0.0f)
            return 1.0f;

        return 1.0f - duckNorm * (duckEnv / (duckEnv + kDuckKnee));
    };

    // v1.14.0: the page's level meter. Tracked inside the mix loops because the
    // dry input is still readable here and nothing downstream touches the
    // buffer. `a > p` rather than jmax so a NaN sample never becomes the peak;
    // an infinity does, and should — it lights the clip lamp.
    float blockPeakIn  = 0.0f;
    float blockPeakOut = 0.0f;
    auto trackPeak = [] (float& p, float x) noexcept { const float a = std::abs (x); if (a > p) p = a; };

    if (numOutputChannels > 1)
    {
        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            const float m       = mixSmoothed.getNextValue();
            const float dryGain = std::cos(m * halfPi);
            const float wetGain = std::sin(m * halfPi);
            const float dryL    = outL[i];
            const float dryR    = numInputChannels > 1 ? outR[i] : dryL;   // mono→stereo: duplicate dry

            // (wetGain · duck) · wet, i.e. the duck folds into the existing wet
            // gain rather than adding a third multiply. At duck 0 that factor is
            // wetGain · 1.0f, which is bitwise wetGain.
            const float wg = wetGain * advanceDuck (dryL, dryR);

            outL[i] = dryGain * dryL + wg * wetL[i];
            outR[i] = dryGain * dryR + wg * wetR[i];

            trackPeak (blockPeakIn,  dryL);
            trackPeak (blockPeakIn,  dryR);
            trackPeak (blockPeakOut, outL[i]);
            trackPeak (blockPeakOut, outR[i]);
        }
    }
    else
    {
        float* outM = buffer.getWritePointer(0);

        for (int i = 0; i < numSamples; ++i)
        {
            const float m       = mixSmoothed.getNextValue();
            const float dryGain = std::cos(m * halfPi);
            const float wetGain = std::sin(m * halfPi);
            const float dryM    = outM[i];

            // Mono in: the follower sees the same sample twice, which is the
            // same rectified mean a centred stereo signal would give it, so a
            // mono and a dual-mono render duck identically.
            const float wg = wetGain * advanceDuck (dryM, dryM);

            // Mono out: equal-power fold of the wet pair.
            //
            // ── v1.7.0: why 0.7071 is still the right constant with a STEREO
            // source ──────────────────────────────────────────────────────────
            // The constant answers a question about the PAN, not about what the
            // grains read. At width 0 every grain pans centre, so panL = panR =
            // 1/√2 and each grain contributes v/√2 to both channels; the fold is
            // 0.7071·(wetL + wetR) = 0.7071·2·(v/√2) = v, i.e. exactly unity —
            // and that holds in Stereo mode too, because a Stereo-mode grain at
            // width 0 still pans centre and still writes both channels equally.
            // Only the value of v changes, never the geometry.
            //
            // Above width 0 the fold is the incoherent-sum constant for a pair
            // that is no longer identical, and Stereo mode makes wetL and wetR
            // MORE decorrelated rather than less — so 1/√2 is if anything a
            // better fit there than it was for the mono-sum case it was derived
            // for. Re-derived rather than assumed, and probe AT measures the
            // mono fold in both source modes rather than taking either claim on
            // trust.
            outM[i] = dryGain * dryM + wg * 0.70710677f * (wetL[i] + wetR[i]);

            trackPeak (blockPeakIn,  dryM);
            trackPeak (blockPeakOut, outM[i]);
        }
    }

    // Max-fold into the since-last-read accumulators (see takeLevelPeaks()).
    // The message thread only ever exchanges them to 0, so the CAS loop retries
    // at most once per poll and never spins against another writer.
    auto foldPeak = [] (std::atomic<float>& acc, float v) noexcept
    {
        float cur = acc.load (std::memory_order_relaxed);
        while (v > cur && ! acc.compare_exchange_weak (cur, v, std::memory_order_relaxed)) {}
    };

    foldPeak (peakInSinceRead,  blockPeakIn);
    foldPeak (peakOutSinceRead, blockPeakOut);
}

// The editor include lives INSIDE the guard: the Stage-2 render harness compiles
// this file with JUCE_WEB_BROWSER=0 and no editor sources, so a top-of-file
// include would make the harness un-buildable the moment the editor gained
// WebView types (pattern_render_harness_breaks_on_webview_editor). PluginProcessor.h
// stays editor-include-free for the same reason.
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* ReverseDelayProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new ReverseDelayEditor(*this);
#else
    return new juce::GenericAudioProcessorEditor(*this);   // harness build
#endif
}

bool ReverseDelayProcessor::hasEditor() const { return true; }

const juce::String ReverseDelayProcessor::getName() const { return JucePlugin_Name; }

bool ReverseDelayProcessor::acceptsMidi() const { return false; }
bool ReverseDelayProcessor::producesMidi() const { return false; }
bool ReverseDelayProcessor::isMidiEffect() const { return false; }
// Conservative real tail so hosts don't truncate the reverse tail on bounce
// (RESEARCH pitfall 11 — offline renders honour this).
//
// v1.7.2 (WR-04): DERIVED, not pinned. This sat at 10.0 from v1.0.0, when
// kDelayTimeMaxMs was 2000 and kGrainSizeMaxMs was 500 — i.e. ~4x the longest
// single-generation span. Every one of those numbers then moved: delayTime to
// 4000 ms (v1.0.1), grainSize to 4000 ms (v1.5.0), drift +25 % (v1.7.0), and
// regenMakeup can push the loop into sustain (v1.6.0). The static_assert on
// kCaptureSeconds puts the worst-case span for ONE generation at 13.5 s, so the
// declared tail had become SHORTER than a single generation and a long bounce
// ended mid-wash while the same settings monitored live decayed properly.
//
// Tied to kCaptureSeconds so a future range move carries this with it rather
// than leaving it behind again — that is the whole failure being fixed. The ring
// is sized for exactly one generation's read span, and feedback stacks more, so
// four of them is the honest figure at feedback 70 % with makeup.
//
// The asymmetry matters: an over-long tail costs offline render time and nothing
// else, while an under-long one truncates audio the user cannot get back.
double ReverseDelayProcessor::getTailLengthSeconds() const
{
    constexpr double kGenerations = 4.0;
    return kGenerations * static_cast<double> (kCaptureSeconds - 0.5f);   // ~54 s
}

int ReverseDelayProcessor::getNumPrograms() { return 1; }
int ReverseDelayProcessor::getCurrentProgram() { return 0; }
void ReverseDelayProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String ReverseDelayProcessor::getProgramName(int index) { juce::ignoreUnused(index); return {}; }
void ReverseDelayProcessor::changeProgramName(int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

// Stage 4: session state routed through OuariconPresetManager so the current
// preset NAME survives a save/reload alongside the parameters.
//
// Backward compatible in both directions: getStateAsXml() wraps the SAME APVTS
// root, adding only a `currentPreset` attribute, and setStateFromXml() accepts a
// plain pre-Stage-4 APVTS tree (the attribute simply defaults to "Default").
// Stage 1–3 sessions therefore still load. No setCustomStateCallbacks — this
// plugin holds no state outside the APVTS.
void ReverseDelayProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // v1.9.0: the UI language rides the state tree as a non-parameter property.
    //
    // Written BEFORE the delegation, not after: getStateAsXml() starts from its
    // own parameters.copyState(), so a setProperty on the live tree afterwards
    // would never reach the XML that gets serialised.
    parameters.state.setProperty ("uiLanguage",
                                  languageCode (uiLanguage.load (std::memory_order_acquire)),
                                  nullptr);

    // v1.16.0: Mix lock, written the same way and before the same delegation.
    // Written as a STRING ("1" / "0") so that what is saved is exactly what the
    // restore below reads back. The XML round-trip turns every property into a
    // string anyway (critical_valuetree_xml_roundtrip_loses_type).
    parameters.state.setProperty ("mixLock",
                                  mixLock.load (std::memory_order_acquire) ? "1" : "0",
                                  nullptr);

    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void ReverseDelayProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        // v1.12.2: a parameter the saved state does not mention restores to its
        // DEFAULT, not to whatever this instance currently holds — stated here
        // explicitly rather than inherited from JUCE.
        //
        // On the JUCE this builds against (8.0.15) that already happens, but only
        // incidentally: replaceState() re-binds the parameters, appends an id-only
        // PARAM child for each one the tree lacks, and the APVTS's own
        // valueTreeChildAdded listener then reads that child's missing "value"
        // with the parameter's default as fallback. Nothing documents that path,
        // and if it changed a v1.0–v1.7 session (no diffusion/drive; before
        // v1.6.0 no freeze/direction/regenMakeup either) opened into a slot at
        // Drive 60 or Freeze on would keep those values. Writing the defaults into
        // the incoming XML makes the guarantee ours, and matches the JSON preset
        // path, which resets to defaults before applying (WR-01 in
        // OuariconPresetManager::loadPreset). Probe BF pins the behaviour.
        //
        // On the XML rather than by resetting the live parameters first: that
        // would notify the host of a default for every parameter and then again
        // with the saved value. Only for the APVTS root — setStateFromXml ignores
        // anything else.
        if (xml->hasTagName (parameters.state.getType()))
        {
            for (auto* p : getParameters())
            {
                auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p);

                if (rp == nullptr
                    || xml->getChildByAttribute ("id", rp->paramID) != nullptr)
                    continue;

                auto* child = xml->createNewChildElement ("PARAM");
                child->setAttribute ("id", rp->paramID);
                child->setAttribute ("value",
                                     static_cast<double> (rp->convertFrom0to1 (rp->getDefaultValue())));
            }
        }

        presetManager.setStateFromXml(xml.get());

        // Read AFTER the restore: setStateFromXml replaces the whole tree, so
        // the property only exists on it once that has happened.
        //
        // isVoid() is the ONLY correct guard and toString() the only correct
        // read. NamedValueSet::setFromXmlAttributes rebuilds every property as
        // a var over the attribute STRING, so isBool()/isInt()/isString() type
        // predicates are false for every session ever saved
        // (critical_valuetree_xml_roundtrip_loses_type). A session written
        // before v1.9.0 has no such attribute at all and simply leaves the
        // language where it is — English on a fresh instance.
        const juce::var lang = parameters.state.getProperty ("uiLanguage");

        if (! lang.isVoid())
            uiLanguage.store (languageIndex (lang.toString()), std::memory_order_release);

        // v1.16.0: same guard, same read, but an ABSENT property turns the lock
        // OFF instead of leaving it alone — see mixLock's declaration. A
        // pre-v1.16 session opened into an instance whose lock is on therefore
        // comes up unlocked, like any setting it never saved. This path
        // restores the session's own Mix and never re-applies a held one.
        const juce::var lock = parameters.state.getProperty ("mixLock");
        mixLock.store (! lock.isVoid() && lock.toString() == "1", std::memory_order_release);
    }
}

//==============================================================================
// v1.16.0: Mix lock. The shared OuariconPresetManager is used exactly as is.
// Each wrapper captures Mix, performs the load, and then — only if the load
// SUCCEEDED and the lock is on — puts Mix back.
//
// Captured as the NORMALISED value and written back through
// setValueNotifyingHost, which is also the call applyPresetJson makes. The host
// sees the same kind of change it already sees for every other parameter, and
// no range conversion sits between capture and restore to round the value.
//
// Known limit, not worked around: applyPresetJson resets every parameter to
// its default before applying the preset, so during the load Mix passes through
// 35 % and the preset's value on the message thread. An audio block that starts
// inside that window reads one of those targets, and mixSmoothed starts toward
// it for that block before the restore takes effect. It happens only while the
// rest of the preset is changing at the same instant, and removing it would mean
// changing the module, which this feature was scoped not to do.
namespace
{
    template <typename Load>
    bool loadHoldingMix (juce::AudioProcessorValueTreeState& apvts,
                         const std::atomic<bool>& lock, Load&& load)
    {
        auto* mix = apvts.getParameter ("mix");
        jassert (mix != nullptr);

        const float held = mix->getValue();
        const bool  ok   = load();

        if (ok && lock.load (std::memory_order_acquire))
            mix->setValueNotifyingHost (held);

        return ok;
    }
}

bool ReverseDelayProcessor::loadPresetHoldingMix (const juce::String& name)
{
    return loadHoldingMix (parameters, mixLock, [&] { return presetManager.loadPreset (name); });
}

bool ReverseDelayProcessor::loadPresetFromFileHoldingMix (const juce::File& file)
{
    return loadHoldingMix (parameters, mixLock, [&] { return presetManager.loadPresetFromFile (file); });
}

//==============================================================================
// v1.18.0: A/B compare and Randomise. Message thread only — every path ends in
// setValueNotifyingHost, and the slots themselves are unsynchronised.
ReverseDelayProcessor::AbSnapshot ReverseDelayProcessor::captureAbSnapshot() const
{
    return { presetManager.capturePresetData(), presetManager.getCurrentPresetName() };
}

ReverseDelayProcessor::AbState ReverseDelayProcessor::getAbState() const noexcept
{
    AbState s;
    s.active    = abActive;
    s.filled[0] = ! abSlots[0].data.isVoid();
    s.filled[1] = ! abSlots[1].data.isVoid();
    return s;
}

ReverseDelayProcessor::AbState ReverseDelayProcessor::abSelect (int slot)
{
    if ((slot != 0 && slot != 1) || slot == abActive)
        return getAbState();

    abSlots[abActive] = captureAbSnapshot();

    // An empty target starts as a copy of the slot being left: identical state,
    // so there is nothing to recall and nothing audible happens.
    if (abSlots[slot].data.isVoid())
    {
        abSlots[slot] = abSlots[abActive];
    }
    else
    {
        const auto& target = abSlots[slot];

        // Same Mix-lock hold as a preset load: with the lock on, A/B compares
        // the sound at a fixed dry/wet balance.
        if (loadHoldingMix (parameters, mixLock, [&] { return presetManager.applyPresetData (target.data); }))
            presetManager.setCurrentPresetName (target.presetName);
    }

    abActive = slot;
    return getAbState();
}

ReverseDelayProcessor::AbState ReverseDelayProcessor::abCopyActiveToInactive()
{
    abSlots[1 - abActive] = captureAbSnapshot();
    return getAbState();
}

ReverseDelayProcessor::AbState ReverseDelayProcessor::randomiseCharacter()
{
    return randomiseCharacter (abRandom);
}

ReverseDelayProcessor::AbState ReverseDelayProcessor::randomiseCharacter (juce::Random& rng)
{
    // The way back: the pre-randomise state goes into the INACTIVE slot, so one
    // click on the other slot letter returns to it.
    abSlots[1 - abActive] = captureAbSnapshot();

    for (const auto* id : kRandomiseParamIds)
    {
        auto* p = parameters.getParameter (id);
        jassert (p != nullptr);

        if (p == nullptr)
            continue;

        float value;

        // A choice is drawn over its ENTRIES: a uniform normalised draw rounded
        // to an index would give the first and last entries half the odds.
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (p))
            value = choice->convertTo0to1 (static_cast<float> (rng.nextInt (choice->choices.size())));
        else
            value = rng.nextFloat();

        p->beginChangeGesture();
        p->setValueNotifyingHost (value);
        p->endChangeGesture();
    }

    return getAbState();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverseDelayProcessor();
}
