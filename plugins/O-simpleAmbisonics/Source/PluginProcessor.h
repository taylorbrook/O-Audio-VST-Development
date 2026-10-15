/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    O-simpleAmbisonics - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include <array>
#include <atomic>
#include <map>        // before the preset-manager header: its FactoryPresetDef uses std::map
#include <vector>

#include "OuariconPresetManager.h"

#include "dsp/SpatialEngine.h"
#include "dsp/MeterFrame.h"
#include "dsp/TripleBuffer.h"
#include "PresetRecipes.h"

namespace OSimpleAmbisonics::ParamIDs
{
    // The 12-parameter v1.0 contract (research/ARCHITECTURE.md §Parameter Mapping).
    // IDs are frozen: a range change after 1.0.0 needs a preset-migration gate.
    inline constexpr auto azimuth     = "azimuth";
    inline constexpr auto elevation   = "elevation";
    inline constexpr auto distance    = "distance";
    inline constexpr auto order       = "order";
    inline constexpr auto yaw         = "yaw";
    inline constexpr auto pitch       = "pitch";
    inline constexpr auto roll        = "roll";
    inline constexpr auto decoder     = "decoder";
    inline constexpr auto orbit       = "orbit";
    inline constexpr auto orbitRate   = "orbitRate";
    inline constexpr auto inputMode   = "inputMode";
    inline constexpr auto outputLevel = "outputLevel";

    // All 12 in APVTS (layout) order — the ONE list. Preset Targets are
    // indexed by it; the editor's defaults native walks it.
    inline constexpr const char* kAll[] = {
        azimuth, elevation, distance, order, yaw, pitch,
        roll, decoder, orbit, orbitRate, inputMode, outputLevel };
    inline constexpr int kNumParams = (int) (sizeof (kAll) / sizeof (kAll[0]));
}

class OSimpleAmbisonicsAudioProcessor : public juce::AudioProcessor
{
public:
    OSimpleAmbisonicsAudioProcessor();
    ~OSimpleAmbisonicsAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    // Stage 4 WR-03: the host's transport stop / AU Reset clears the engine
    // state (convolver FDL, delays, air LP) and re-seeds every ramp, exactly
    // as prepareToPlay does but without allocating. A no-op before prepare.
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    // Stage 4 WR-05: the structural latency plus the Binaural FIR at the
    // prepared rate ring on after the input stops ((64 + L) / fs, set in
    // prepareToPlay; the 48 k value before the first prepare).
    double getTailLengthSeconds() const override { return tailSeconds.load (std::memory_order_relaxed); }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    // Public APVTS member for direct UI / harness access (house pattern).
    juce::AudioProcessorValueTreeState apvts;

    // Visualisation tap (ARCHITECTURE §Visualization Tap). Public like apvts
    // so the Stage-3 editor Timer and the harness read() it directly. One
    // publish() per host block from the audio thread; read() from exactly
    // one thread (the message thread / the harness thread).
    OSimpleAmbisonics::dsp::TripleBuffer<OSimpleAmbisonics::dsp::MeterFrame> meterFrames;

    //==========================================================================
    // INTERFACE LANGUAGE (Stage 3) — the WebView page's own language.
    //
    // Deliberately NOT a parameter: it must not reach an automation lane, and a
    // lesson must not change which language somebody reads. It rides the state
    // XML as a plain PROPERTY written as the ASCII code ("en" / "fr" /
    // "zh-Hans"), restored only when present (isVoid() — the XML round-trip
    // rebuilds every property as a string var,
    // critical_valuetree_xml_roundtrip_loses_type). The editor PULLS it once at
    // page init (getUiLanguage native); nothing pushes. O-simpleAdditive shape.
    std::atomic<int> uiLanguage { 0 };   // 0 en, 1 fr, 2 zh-Hans

    /** languageIndex() maps anything that is neither "fr" nor "zh-Hans" to 0,
        so a hand-edited session or an unexpected page argument degrades to
        English instead of being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 2 ? "zh-Hans" : i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "zh-Hans" ? 2 : s == "fr" ? 1 : 0; }

    //==========================================================================
    // EDITOR SCALE (Stage 3, UI-07) — window width / 1180, PERSISTED in the
    // state XML (CONTEXT locked: "save / reopen keeps the window size"; this
    // overrides the siblings' session-only member and ARCHITECTURE "Custom
    // state: none"). 0 = never sized → the editor fits the display at open.
    // Written as "0.850" only when > 0 (removed from the saved copy otherwise),
    // restored only when finite and within 0.6 .. 1.5. Atomic: hosts may call
    // getStateInformation off the message thread.
    std::atomic<float> editorScale { 0.0f };

    //==========================================================================
    // PRESETS + LESSONS (Stage 4, FUNC-08; PLAN P1 .. P9). The 9 factory
    // recipes (Init + the 8 lessons) live in ONE table, PresetRecipes.h; the
    // lesson buttons, the browser's factory loads, the factory defs, the page
    // catalog and the modified check all read it. ONE apply core. Message
    // thread unless noted; presetLock guards the identity and is never taken
    // on the audio thread nor held across setValueNotifyingHost.
    //
    // User-owned (R-D3): outputLevel and inputMode are never reset, never set
    // and never counted in "modified" by a lesson, a factory load or a user
    // preset (isUserOwnedParam — the one list).
    using Targets = std::array<float, (size_t) OSimpleAmbisonics::ParamIDs::kNumParams>;   // normalised, APVTS order; NaN = "not in the preset" -> default

    static bool isUserOwnedParam (const juce::String& paramId);

    // Recipe -> targets through each parameter's OWN convertTo0to1 (skew-
    // correct). orderIndex >= 0 overrides the recipe's order (the orderStep
    // walk, R-D4). Unlisted -> NaN.
    Targets recipeTargets (const ambpresets::Recipe& recipe, int orderIndex = -1) const;
    // NaN -> default; clamp 0..1; snapped to the value the parameter holds.
    // User-owned slots -> NaN.
    Targets resolveTargets (const Targets& targets) const;
    Targets currentNormalisedValues() const;

    // The ONE apply core (P2): one pass in APVTS order, user-owned skipped,
    // target = t[i] if finite else the default, clamped and snapped, skipped
    // within 1e-6, else begin / setValueNotifyingHost / end. No
    // JUCE_ASSERT_MESSAGE_THREAD: the harness calls it.
    bool applyNormalisedTargets (const Targets& targets);

    // The LESSON entry (P3 / P4): any table id; unknown / "" -> false, nothing
    // touched. "orderStep" walks the order 1 -> 2 -> 3 -> 1 while the scene is
    // still the unmodified orderStep scene, else starts at 1 (R-D4). Each step
    // is a full reset + recipe, so distance returns to the 1 m default.
    bool applyFactoryPreset (const juce::String& id);

    // The BROWSER entry (P3): a factory file name -> the table recipe with NO
    // walk (orderStep -> order 1); a user name -> must be a walk-order member
    // (blocks traversal) -> its file -> applyUserPresetJson. False = nothing
    // changed.
    bool loadPresetByName (const juce::String& presetName);

    // Strict user-JSON apply (T-4-01): the "parameters" object, known ids,
    // number / bool values only (anything else -> that parameter's default),
    // clamped 0..1, user-owned ignored even if present; a non-object root or
    // a missing / non-object "parameters" -> false, nothing changed.
    bool applyUserPresetJson (const juce::var& presetJson);

    // Identity (P5). Any non-audio thread (presetLock).
    juce::String getPresetName() const;
    juce::String getPresetId() const;                     // the table id, or "" (user / unnamed)
    bool isPresetFactory() const;
    juce::uint32 getPresetRevision() const noexcept { return presetRevision.load(); }
    // Any non-owned parameter > 1e-4 away from the preset's targets. An
    // unnamed preset, or one whose targets are not known yet, is unmodified.
    bool isPresetModified() const;
    // A restored USER name re-reads its file once (editor timer, message
    // thread) — never inside setStateInformation.
    void refreshPresetTargetsIfNeeded();

    // Names (P9). "" = refused: trim; C0 / DEL / C1 / bidi controls
    // stripped; File::createLegalFileName; leading dots stripped; 64 chars.
    static juce::String sanitisePresetName (const juce::String& raw);
    static bool isFactoryPresetName (const juce::String& name) { return ambpresets::findByName (name) != nullptr; }

    // Walk order (P9): the 9 factory file names in table order, then the user
    // stems sorted case-insensitively, minus any equal (ignoring case) to a
    // factory name. The list, ◀ / ▶ and the dropdown all use this ONE order.
    static juce::StringArray buildWalkOrder (const juce::StringArray& userStems);   // pure
    // Neighbour of `current` in `order`, wrapping at both ends; an unnamed or
    // unknown current enters at the top going forward, the bottom going back.
    static juce::String neighbourIn (const juce::StringArray& order, const juce::String& current, int dir);
    juce::StringArray getPresetWalkOrder() const;     // reads the User folder (creates nothing)
    juce::String getNeighbourPreset (int dir) const;  // no load, no parameter touched

    // The 9 factory defs for the module's disk copies (pure): table order,
    // exactly 10 keys each (every id but outputLevel / inputMode), values =
    // the resolved recipe targets. The disk copies are never read back.
    std::vector<OuariconPresetManager::FactoryPresetDef> buildFactoryPresetDefs() const;
    // Lazy (P7): once per instance, message thread, sentinel-gated inside the
    // module. Called by the editor's preset natives ONLY — never the ctor,
    // prepareToPlay, the state calls or the harness.
    void ensureFactoryBankOnDisk();

    // User presets (P9 / T-4-02): sanitise -> refuse "" and any factory file
    // name (ignoring case) BEFORE any module call or disk access -> the
    // module's savePreset -> identity = the saved stem, targets = now.
    bool saveUserPreset (const juce::String& rawName);
    bool deleteUserPreset (const juce::String& name);   // refuses factory names; deleting the current -> unnamed
    juce::var getPresetCatalog() const;                  // { factory: [{ name, id } x 9], user: [names] }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // The module, used narrowly (P7): folders, savePreset, deletePreset and
    // the factory sentinel. Declared after apvts (it holds a reference). The
    // literal name puts both dev and release builds on
    // ~/Library/O-simpleAmbisonics/Presets/{Factory,User}.
    OuariconPresetManager presetManager { apvts, "O-simpleAmbisonics" };
    bool factoryBankEnsured = false;

    // Preset identity (P5): name ("" = unnamed), resolved targets, validity
    // (false = unknown -> never "modified"), and the stale flag of a restored
    // user name. presetRevision bumps on every identity change.
    mutable juce::CriticalSection presetLock;
    juce::String presetName;
    Targets presetTargets {};
    bool presetTargetsValid = false;
    bool presetTargetsStale = false;
    std::atomic<juce::uint32> presetRevision { 0 };

    void setCurrentPreset (const juce::String& newName, const Targets& resolved, bool valid, bool stale);
    // Strict parse of a user preset's JSON into targets (false = rejected).
    bool userJsonTargets (const juce::var& presetJson, Targets& out) const;
    // The orderStep targets for an order index (clamped 0..2).
    Targets orderStepTargets (int orderIndex) const;

    // Engine Params from the cached atomics — once per host block, and once
    // in prepareToPlay BEFORE engine.prepare (A7).
    OSimpleAmbisonics::dsp::Params fillParams() const noexcept;

    // Cached raw-value atomics for all 12 parameters.
    std::atomic<float>* azimuthParam     = nullptr;
    std::atomic<float>* elevationParam   = nullptr;
    std::atomic<float>* distanceParam    = nullptr;
    std::atomic<float>* orderParam       = nullptr;
    std::atomic<float>* yawParam         = nullptr;
    std::atomic<float>* pitchParam       = nullptr;
    std::atomic<float>* rollParam        = nullptr;
    std::atomic<float>* decoderParam     = nullptr;
    std::atomic<float>* orbitParam       = nullptr;
    std::atomic<float>* orbitRateParam   = nullptr;
    std::atomic<float>* inputModeParam   = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;

    // The two stepped+skewed ranges, for snapping the raw value in fillParams
    // (see the JUCE legacy note there). Owned by the parameters; never null
    // after construction.
    const juce::NormalisableRange<float>* distanceRange  = nullptr;
    const juce::NormalisableRange<float>* orbitRateRange = nullptr;

    // Chain: conditioner (processor) → SpatialEngine (per slice) → seeded gain.
    OSimpleAmbisonics::dsp::SpatialEngine engine;
    juce::LinearSmoothedValue<float> outputGain;

    // Stage 4 WR-04: the Left-only weight of the input conditioner, ramped
    // over 20 ms (seeded in prepareToPlay / reset). While it is NOT ramping
    // the conditioner takes the exact branch (0.5 (L + R) or L), so both
    // endpoints stay bit-identical to Stage 2.
    juce::LinearSmoothedValue<float> leftOnlyWeight;

    std::atomic<double> tailSeconds { (double) (OSimpleAmbisonics::dsp::kPartition + OSimpleAmbisonics::dsp::kBinauralFilterTaps) / 48000.0 };

    // Prepared-size scratch (sized in prepareToPlay only): the conditioned
    // mono slice, and a right-output sink for a 1-channel buffer.
    std::vector<float> monoScratch;
    std::vector<float> rightScratch;

    // Post-gain output peaks, peak-since-the-UI-last-read (A8 / R7).
    float outPeakAccL = 0.0f;
    float outPeakAccR = 0.0f;

    int numInputLegs = 2;   // negotiated input bus width (1 or 2), NOT the buffer width
    int maxBlock     = 0;   // samplesPerBlock from prepareToPlay; slice bound

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleAmbisonicsAudioProcessor)
};
