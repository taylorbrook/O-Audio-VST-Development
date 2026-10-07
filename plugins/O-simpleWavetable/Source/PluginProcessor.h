/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): the complete 21-parameter APVTS, a
    MidiMessageCollector for the Stage 3 on-screen keyboard, and APVTS-XML
    state carrying the uiLanguage property and a strip-on-load
    IMPORTED_BANK stub.

    Stage 2.2 (Voice + Oscillator): 16-voice Poly / true-legato Mono
    wavetable engine over the shared BuiltInBanks, per-block BlockContext
    (D-H), oversized-block chunk loop with sliced MIDI, seeded output stage,
    lead-voice display atomics. Imported (bank 5) renders silence until 2.4
    publishes into importedForAudio.

    Stage 2.3 (Modulation -> Position): global per-sample PositionLfo (free
    or PPQ-locked tempo, deterministic S&H), 20 ms smoothed lfo_depth and
    env_amount (D-I), per-voice mod env + 2 ms smoother (WtVoice), display
    atomics dispLfo / dispMenv / dispAmp, strict 1-sample Synthesiser
    subdivision (D-L) so renders are block-size invariant.

    Stage 2.4 (Bank switch / Import / Persistence):
      - frozen-cycle crossfader + D-K level hysteresis live in WtVoice.
      - Imported bank: built off the audio thread (import worker or the
        thread calling setStateInformation), published lock-free through
        importedForAudio (seq_cst), owned by importedOwner under
        bankStateLock, retired into retiredBanks and freed by the REG-01
        reaper (O-Prism 0f7d65ce) with the D-C amendment: the audio thread
        publishes audioHeldBank (this block's resolved pointer = every active
        voice's cfg.bank, the only bank the next block may capture a frozen
        cycle from) and the sweep never frees it. seq_cst on every reaper
        atomic (documented hardening deviation from Prism's acq/rel).
      - import API (D-E): importFromFile / importFromMemory on a
        single-thread pool, importGen supersedes in-flight jobs, status is
        POLLED through getImportStatusVersion(), publish then auto-select
        Imported through an AsyncUpdater (D-M).
      - IMPORTED_BANK state child (version 1, flac16 | pcm16gz, standard
        base64), synchronous restore through the same builder as import.

    Stage 4 (Polish): presets (FUNC-08) from ONE recipe table
    (PresetRecipes.h) through ONE apply core (applyNormalisedTargets), the
    preset-manager module for folders / user save / delete only, a
    currentPreset root property in the plugin's own state; Stage 3 critic
    fixes W4 (import error cleared on a bank change), W5 (UI-held notes),
    N5 (a preset apply cancels a queued Imported auto-select), N13 (one
    gesture per stepped-knob drag).

  ==============================================================================
*/

#pragma once

#include "TestHooks.h"   // first: the OSIW_TEST_HOOKS default (Stage 2 note 8)

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>   // MidiMessageCollector (juce_audio_devices)
#include <array>
#include <atomic>
#include <cstdint>
#include <map>        // before the preset-manager header: its FactoryPresetDef uses std::map
#include <memory>
#include <vector>
#include "OuariconPresetManager.h"
#include "BuiltInBanks.h"
#include "CycleView.h"
#include "MonoStack.h"
#include "PositionLfo.h"
#include "PresetRecipes.h"
#include "WavetableBank.h"
#include "WavetableImporter.h"
#include "WtSynthesiser.h"
#include "WtVoice.h"

//==============================================================================
// Parameter IDs - the exact snake_case strings of parameter-spec.md (LOCKED).
// C++ identifiers are camelCase so none can shadow a juce:: free function.
namespace OSimpleWavetable::ParamIDs
{
    inline constexpr auto bank        = "bank";
    inline constexpr auto position    = "position";
    inline constexpr auto interp      = "interp";
    inline constexpr auto bandlimit   = "bandlimit";
    inline constexpr auto bitDepth    = "bit_depth";
    inline constexpr auto lfoRate     = "lfo_rate";
    inline constexpr auto lfoSync     = "lfo_sync";
    inline constexpr auto lfoDiv      = "lfo_div";
    inline constexpr auto lfoShape    = "lfo_shape";
    inline constexpr auto lfoDepth    = "lfo_depth";
    inline constexpr auto menvAttack  = "menv_attack";
    inline constexpr auto menvDecay   = "menv_decay";
    inline constexpr auto menvSustain = "menv_sustain";
    inline constexpr auto menvRelease = "menv_release";
    inline constexpr auto envAmount   = "env_amount";
    inline constexpr auto ampAttack   = "amp_attack";
    inline constexpr auto ampDecay    = "amp_decay";
    inline constexpr auto ampSustain  = "amp_sustain";
    inline constexpr auto ampRelease  = "amp_release";
    inline constexpr auto voiceMode   = "voice_mode";
    inline constexpr auto outputLevel = "output_level";

    // Spec order - single source of truth for the count/order gate (and Stage 3 loops).
    inline constexpr std::array<const char*, 21> all {
        bank, position, interp, bandlimit, bitDepth, lfoRate, lfoSync, lfoDiv, lfoShape,
        lfoDepth, menvAttack, menvDecay, menvSustain, menvRelease, envAmount,
        ampAttack, ampDecay, ampSustain, ampRelease, voiceMode, outputLevel };
}

//==============================================================================
class OSimpleWavetableAudioProcessor : public juce::AudioProcessor,
                                       private juce::AsyncUpdater,
                                       private juce::Timer
{
public:
    // Stage 3 viz types (D-W). Declared FIRST in the class so the template's
    // qualified names (OSimpleWavetableAudioProcessor::CycleView / ::BankThumbs)
    // compile and no earlier use of the plain names changes meaning.
    using CycleView  = ::CycleView;
    using BankThumbs = ::BankThumbs;

    OSimpleWavetableAudioProcessor();
    ~OSimpleWavetableAudioProcessor() override;

    // The console harnesses have no message loop: they apply the pending
    // auto-select (D-M) by calling this on the message (main) thread.
    using juce::AsyncUpdater::handleUpdateNowIfNeeded;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "O-simpleWavetable"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }   // amp_release max

    //==========================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // Mockup contract (mockups/v1-PluginEditor.cpp calls exactly these names).
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // On-screen keyboard: the editor injects note on/off from the WebView (any
    // thread). Queued via the MidiMessageCollector and merged into processBlock.
    void handleUiMidi (int noteNumber, bool noteOn, float velocity);

    // Interface language of the WebView UI. NOT a parameter (no automation
    // lane, no preset can change it). Runtime form = index, persisted form =
    // the ASCII language code (XML round trip rebuilds properties as strings).
    std::atomic<int> uiLanguage { 0 };

    // Codec. languageIndex() maps anything that is neither "fr" nor "zh-Hans"
    // to 0, so a hand-edited session degrades to English (allow-list).
    static juce::String languageCode  (int i)                 { return i == 2 ? "zh-Hans" : i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "zh-Hans" ? 2 : s == "fr" ? 1 : 0; }

    //==========================================================================
    // Lead-voice display snapshot (Stage 3 viz seam). Written by the audio
    // thread once per block (newest note among sounding voices), relaxed.
    float getDisplayPosition() const noexcept { return dispPos.load (std::memory_order_relaxed); }
    int   getDisplayLevel() const noexcept    { return dispLevel.load (std::memory_order_relaxed); }
    int   getDisplayFrame() const noexcept    { return dispFrame.load (std::memory_order_relaxed); }
    bool  isDisplaySounding() const noexcept  { return dispSounding.load (std::memory_order_relaxed); }
    float getDisplayLfo() const noexcept      { return dispLfo.load (std::memory_order_relaxed); }    // -1..1, every block
    float getDisplayModEnv() const noexcept   { return dispMenv.load (std::memory_order_relaxed); }   // lead voice, 0..1
    float getDisplayAmpEnv() const noexcept   { return dispAmp.load (std::memory_order_relaxed); }    // lead voice, 0..1
    int   getDisplayNote() const noexcept     { return dispNote.load (std::memory_order_relaxed); }   // lead MIDI note, -1 silent (D-X)
    float getDisplayHz() const noexcept       { return dispHz.load (std::memory_order_relaxed); }     // lead Hz incl. bend, 0 silent
    double getDisplaySampleRate() const noexcept { return displayFs.load (std::memory_order_relaxed); } // prepareToPlay rate

    //==========================================================================
    // Stage 3 visualization API (D-R, D-S, D-U, D-W). MESSAGE THREAD ONLY and
    // non-reentrant (one shared renderer; documented, not asserted - the
    // console harness calls it from main). Never call from the audio thread.
    //
    // getSelectedBankIndex: the APVTS bank index 0..5, resolved exactly as
    //   processBlock does (the editor's bank watch, P2).
    // buildCycleView: the cycle the lead voice reads now - same bank, mip
    //   level, position / latched frame and quantizer as the voice, rendered
    //   through wt::readSample + BitQuantizer, then FFT'd (CycleRenderer).
    //   Silent: knob position, level 0, frame = Interp ? -1 : latched knob
    //   frame, note -1, f0 = nyquistH = 0 (D-R). lfo = 0 when lfo_depth is 0
    //   (D-P). Imported is read through getImportedBankSnapshot(), held for
    //   THIS call only. No allocation.
    // getBankThumbnails: level-0 thumbnails of the selected bank. Imported:
    //   the owner and cachedBlob.filename are copied in ONE bankStateLock
    //   scope (D-S), the points outside it. points.assign() reuses capacity.
    int  getSelectedBankIndex() const noexcept;
    void buildCycleView (CycleView& v);
    void getBankThumbnails (BankThumbs& t) const;

    // Built-in banks (immutable; shared by all instances). Message thread
    // readers (Stage 3 cycle view / thumbnails) may use this freely.
    const BuiltInBanks& getBuiltInBanks() const noexcept { return *builtIns; }

    static constexpr int kNumVoices   = 16;
    static constexpr int kNumBanks    = 6;    // 0..4 built-in, 5 Imported
    static constexpr int kImportedIdx = 5;

    //==========================================================================
    // Import API (D-E; the Stage 3 template names). Any non-audio thread.
    struct ImportStatus
    {
        enum class State { idle, busy, done, error };
        State state = State::idle;
        juce::String filename;
        int frames = 0;
        juce::String error;        // "tooShort" | "unreadable" | "tooLarge" | "unsupported"
    };

    // false = refused to start (status says why). A rejected or failed import
    // leaves the current Imported bank untouched.
    bool importFromFile   (const juce::File& file);
    bool importFromMemory (const juce::String& name, juce::MemoryBlock&& bytes);

    // Drop path (D-Z). The WebView sends the file as STANDARD base64 text
    // (the page's arrayBufferToBase64 / btoa alphabet). The length is capped
    // BEFORE decoding (kMaxMemoryBytes / 3 * 4 + 4 chars -> "tooLarge"), the
    // text is decoded with juce::Base64::convertFromBase64 (failure ->
    // "unreadable"), then the bytes go through importFromMemory. Every status
    // it sets carries WavetableImporter::sanitiseName (name). Any non-audio
    // thread.
    bool importFromBase64 (const juce::String& name, const juce::String& base64);

    // Factory presets by STABLE table id (Stage 3 D-Y, Stage 4 D-AD): the 9 ids
    // of PresetRecipes.h (the 5 lesson ids are a subset). Table -> recipeTargets
    // -> applyNormalisedTargets, then the preset name becomes the recipe's file
    // name (unmodified). An unknown id returns false and changes nothing. The
    // Imported bank is untouched. Message thread (documented, not asserted).
    bool applyFactoryPreset (const juce::String& id);

    //==========================================================================
    // Stage 4 presets (FUNC-08; D-AA .. D-AH). MESSAGE THREAD unless noted.
    using Targets = std::array<float, 21>;   // normalised, ParamIDs::all order; NaN = "not in the preset"

    // D-AB: the ONE apply core. (1) pendingAutoSelect = false under
    // bankStateLock (Stage 3 N5); (2) one pass over ParamIDs::all, output_level
    // skipped; (3) target = the clamped, parameter-quantised value if finite,
    // else the parameter's default; (4) skipped when within 1e-6; (5) begin ->
    // setValueNotifyingHost -> end per changed parameter. Never touches the
    // imported bank or the import status.
    bool applyNormalisedTargets (const Targets& targets);

    // Recipe -> targets: convertTo0to1 (raw) for each listed entry, NaN for the
    // rest (and for output_level). Pure.
    Targets recipeTargets (const wtpresets::Recipe& recipe) const;

    // Preset identity (D-AF / D-AG). Any non-audio thread (presetLock).
    juce::String getPresetName() const;
    juce::String getPresetId() const;                 // table id, or "" (user / unnamed)
    bool isPresetFactory() const;
    juce::uint32 getPresetRevision() const noexcept { return presetRevision.load(); }   // bumps on every name / target change
    // Any parameter except output_level differs from the cached targets by
    // more than 1e-4. Unnamed, or targets not (yet) known -> false.
    bool isPresetModified() const;

    // D-AE name cleaning (save and restore): trim -> strip C0, DEL, C1 and the
    // bidi controls -> juce::File::createLegalFileName -> strip leading dots
    // -> cap 64 characters. "" = refused. Pure.
    static juce::String sanitisePresetName (const juce::String& raw);
    static bool isFactoryPresetName (const juce::String& presetName) { return wtpresets::findByName (presetName) != nullptr; }

    // The module-format factory definitions: 9 defs in table order, each a full
    // 20-parameter normalised map (recipe targets, else defaults; output_level
    // omitted). Pure (no file I/O).
    std::vector<OuariconPresetManager::FactoryPresetDef> buildFactoryPresetDefs() const;

    // Lazily materialises the factory bank on disk (once per instance, through
    // the module and its version sentinel). Called by the preset natives only:
    // never by the ctor, so auval, pluginval and the console gates do no I/O.
    void ensureFactoryBankOnDisk();

    // D-AE walk order: the 9 factory names in table order, then the user stems
    // sorted case-insensitively, minus any that match a factory name. The list,
    // prev / next and the page select all use this ONE order.
    juce::StringArray getPresetWalkOrder() const;                       // reads the User folder (no create)
    static juce::StringArray buildWalkOrder (const juce::StringArray& userStems);   // pure
    // Neighbour of `current` in `order` (wraps). An unnamed or unknown current
    // enters at the top going forward and at the bottom going back. Pure.
    static juce::String neighbourInOrder (const juce::StringArray& order, const juce::String& current, int dir);
    juce::String getNeighbourPreset (int dir) const;                    // no load

    // D-AB caller 2 (untrusted file content, ASVS V5): the object's
    // "parameters" object; known ids only; numeric / bool values only, any
    // other value -> that parameter's default; output_level and unknown keys
    // ignored. A non-object, or "parameters" absent / not an object -> false,
    // nothing changed. Does not rename the preset.
    bool applyUserPresetJson (const juce::var& presetJson);

    // Factory name -> applyFactoryPreset (the table, never the disk copy).
    // User name -> must be in the walk order (blocks traversal) -> file ->
    // applyUserPresetJson -> name = the stem.
    bool loadPresetByName (const juce::String& presetName);
    bool saveUserPreset (const juce::String& raw);                      // refuses "" and factory names
    bool deleteUserPreset (const juce::String& presetName);             // refuses factory names
    juce::var getPresetCatalog() const;                                 // { factory: [{ name, id }], user: [names] }

    // D-AF: a restored user name re-reads its file ONCE (the first editor tick).
    // A missing or unreadable file leaves the targets unknown (unmodified).
    // Creates nothing.
    void refreshPresetTargetsIfNeeded();

    //==========================================================================
    // Stage 3 W4 (D-AL): an import error lives until the next bank change. The
    // processor's 250 ms timer calls this; public for the console gate.
    void pollBankForImportStatus();

    // Stage 3 W5 (D-AM): notes the on-screen keyboard holds. Message thread.
    // Queues a note-off for every held note and clears them; returns how many.
    int releaseUiHeldNotes();

    // Stage 3 N13 (D-AP): one host gesture per stepped-knob DRAG. paramId must
    // be bit_depth or lfo_div; phase 0 begin (opens once), 1 move (sets
    // jlimit (0, n - 1, index) if it differs; needs an open gesture), 2 end
    // (closes once). false = refused / nothing done. Message thread.
    bool stepKnobGesture (const juce::String& paramId, int phase, int index);
    void closeStepKnobGestures();

    ImportStatus getImportStatus() const;
    juce::uint32 getImportStatusVersion() const noexcept   { return importStatusVersion.load(); }   // bumps on every transition
    juce::uint32 getBankDisplayGeneration() const noexcept { return bankDisplayGen.load (std::memory_order_relaxed); }

    // The current Imported bank (nullptr = empty). A shared_ptr copy taken
    // under bankStateLock: safe to read on the message thread (Stage 3
    // thumbnails / cycle view). NEVER call from the audio thread.
    std::shared_ptr<const WavetableBank> getImportedBankSnapshot() const;

   #if OSIW_TEST_HOOKS
    // Test-only accessors (console targets). Call from the thread that runs
    // processBlock (the harness's main thread).
    int getSoundingVoiceCountForTesting() const noexcept
    {
        int n = 0;
        for (const auto* v : wtVoices)
            if (v != nullptr && v->isSounding())
                ++n;
        return n;
    }

    const WtVoice* getVoiceForTesting (int i) const noexcept
    {
        return (i >= 0 && i < kNumVoices) ? wtVoices[(size_t) i] : nullptr;
    }

    // 2.3 hooks.
    void setLfoSeedForTesting (std::uint64_t s) noexcept { lfo.setSeed (s); }        // lfoSeed: one per render job
    const PositionLfo& getLfoForTesting() const noexcept { return lfo; }             // data() = last chunk's LFO
    void setSmootherBypassForTesting (bool b) noexcept
    {
        for (auto* v : wtVoices)
            v->smootherBypass = b;
    }
    // Negative control for the zipper gate: the knob jumps to its target
    // every block (no 20 ms ramp).
    void setKnobRampOffForTesting (bool b) noexcept { testKnobRampOff = b; }

    // 2.4 hooks (crossfader).
    void setXfadeLenOverrideForTesting (int len) noexcept      // xfadeLenOverride (0 = hard switch)
    {
        for (auto* v : wtVoices)
            v->xfadeLenOverride = len;
    }
    // Stage 4 (W4) negative control: false = the frozen cycle is read at the
    // live phase (the pre-v1.0.0 behaviour).
    void setXfadeKeepRateForTesting (bool on) noexcept
    {
        for (auto* v : wtVoices)
            v->xfKeepRate = on;
    }
    void setForceLevelForTesting (int level) noexcept          // forceLevel (-1 = off)
    {
        for (auto* v : wtVoices)
            v->forceLevel = level;
    }

    // Gap-closure hooks (W1 / W2 negative controls).
    void setTailFadeForTesting (bool on) noexcept              // false = hard stop to 0
    {
        for (auto* v : wtVoices)
            v->tailFadeEnabled = on;
    }
    void setVelRampForTesting (bool on) noexcept               // false = instant velGain
    {
        for (auto* v : wtVoices)
            v->velRampEnabled = on;
    }

    // 2.4 hooks (reaper). Lock-taking ones: never from inside processBlock
    // except through the midBlockCallback (test only).
    void setDisableHeldExclusionForTesting (bool b) noexcept { testDisableHeldExclusion.store (b); }   // D-C negative control
    void setGraveyardModeForTesting (bool b) noexcept        { testGraveyard.store (b); }              // park + testReaped
    static int  getDerefAfterReapForTesting() noexcept       { return WavetableBank::testDerefAfterReap.load(); }
    static void resetDerefAfterReapForTesting() noexcept     { WavetableBank::testDerefAfterReap.store (0); }
    int  getHeldBankCountForTesting() const;                 // imported banks the processor keeps: owner + retired
    int  getRetiredBankCountForTesting() const;
    int  getGraveyardCountForTesting() const;
    void clearGraveyardForTesting();                         // frees parked banks (no render in flight!)
    void sweepNowForTesting();                               // sweepNow
    // Publishes a harness-built bank synchronously (exactly the worker's
    // publish path, without the worker): deterministic reaper scenarios.
    void publishImportedBankForTesting (std::shared_ptr<const WavetableBank> b);
    // midBlockCallback: invoked inside processBlock between the entry and the
    // exit increment (a block "in flight"). Plain function pointer: no
    // std::function on the audio path, even in test builds.
    using MidBlockFn = void (*) (void*);
    void setMidBlockCallbackForTesting (MidBlockFn fn, void* context) noexcept { midBlockFn = fn; midBlockCtx = context; }
    const WavetableBank* getAudioHeldBankForTesting() const noexcept    { return audioHeldBank.load(); }
    const WavetableBank* getImportedForAudioForTesting() const noexcept { return importedForAudio.load(); }
    std::uint64_t getBlockEntriesForTesting() const noexcept { return blockEntries.load(); }
    std::uint64_t getBlockExitsForTesting() const noexcept   { return blockGeneration.load(); }

    // Stage 4 hooks (negative controls; every default = the shipped fix).
    void setPreparedGuardForTesting (bool on) noexcept        { testPreparedGuard = on; }   // note 4: false = render unprepared
    void setMidiCapacityGuardForTesting (bool on) noexcept    { testMidiGuard = on; }       // note 3: false = uncapped chunk copy
    void setMonoWheelSeedForTesting (bool on) noexcept        { testMonoWheelSeed = on; }   // W3: false = no seed at Poly -> Mono
    void setReleaseClearsDisplayForTesting (bool on) noexcept { testReleaseClears = on; }   // N4: false = release keeps the display
   #endif

private:
    //==========================================================================
    juce::AudioProcessorValueTreeState parameters;   // declared BEFORE synth/collector (ctor init order)
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Stage 4 (D-AA): used ONLY for the Factory / User folders, the user save
    // (factory guard + sanitize), the user delete (factory guard) and the
    // factory-bank sentinel. Never its state, load, dialog or prev / next
    // paths. Message thread only. Declared after `parameters` (it holds a ref).
    OuariconPresetManager presetManager { parameters, "O-simpleWavetable" };

    // Preset identity (D-AF). Never taken on the audio thread, never held
    // across setValueNotifyingHost.
    mutable juce::CriticalSection presetLock;
    juce::String curPresetName;                    // "" = unnamed
    Targets presetTargets {};                      // resolved (clamped, quantised) normalised targets
    bool presetTargetsValid = false;               // false = unknown -> never "modified"
    bool presetTargetsStale = false;               // a restored user name: re-read its file once
    std::atomic<juce::uint32> presetRevision { 0 };
    bool factoryBankEnsured = false;               // message thread

    // W4 (D-AL): the bank index the import status last saw. Seeded in the ctor
    // and at the end of setStateInformation.
    std::atomic<int> lastStatusBank { 0 };

    // W5 (D-AM): notes held by the on-screen keyboard. Message thread.
    std::array<bool, 128> uiHeld {};

    // N13 (D-AP): open stepped-knob drag gestures [bit_depth, lfo_div]. Message thread.
    std::array<bool, 2> stepGestureOpen {};

    Targets resolveTargets (const Targets& targets) const;   // NaN -> default; clamp; quantise; output_level NaN
    bool userJsonTargets (const juce::var& presetJson, Targets& out) const;
    void setCurrentPreset (const juce::String& newName, const Targets& resolved, bool valid, bool stale);
    Targets currentNormalisedValues() const;

    // Declared before anything that reads it. Built on the message thread
    // (first instance) inside JUCE's SpinLock; never on the audio thread.
    juce::SharedResourcePointer<BuiltInBanks> builtIns;

    WtSynthesiser synth;   // alloc-free findVoiceToSteal (see WtSynthesiser.h)
    juce::MidiMessageCollector midiCollector;

    //==========================================================================
    // Stage 2.2 engine state (audio thread unless noted).

    // The imported bank as the audio thread sees it (seq_cst publish by
    // publishImportedBank; loaded ONCE per block). nullptr = empty = silence.
    std::atomic<const WavetableBank*> importedForAudio { nullptr };

    //==========================================================================
    // REG-01 reaper (+ D-C amendment). seq_cst throughout.
    std::atomic<std::uint64_t> blockGeneration { 0 };   // blocks FINISHED (exit increment, every exit)
    std::atomic<std::uint64_t> blockEntries    { 0 };   // blocks STARTED  (first statement of processBlock)
    // D-C: the bank every ACTIVE voice holds as cfg.bank after the last
    // finished block = that block's resolved pointer. The next block may
    // dereference it once (frozen-cycle capture), so it is never freed.
    std::atomic<const WavetableBank*> audioHeldBank { nullptr };

    struct RetiredBank
    {
        std::shared_ptr<const WavetableBank> bank;
        std::uint64_t retiredAt = 0;                    // blockGeneration at retire (after the publish store)
    };

    // Everything below up to the import status is guarded by bankStateLock.
    // The audio thread NEVER takes it (it only loads importedForAudio).
    mutable juce::CriticalSection bankStateLock;
    std::shared_ptr<const WavetableBank> importedOwner;
    std::vector<RetiredBank> retiredBanks;

    // Persisted form of the current import (no work in getStateInformation).
    struct CachedBlob
    {
        juce::String filename, encoding, data;
        int numFrames = 0;
        juce::ValueTree passthrough;                    // unknown version / encoding: re-emitted verbatim
    };
    CachedBlob cachedBlob;
    ImportStatus importStatus;
    bool pendingAutoSelect = false;                     // set with the publish; consumed by handleAsyncUpdate

    std::atomic<juce::uint32> importGen { 0 };          // bumped by every import request and every restore
    std::atomic<juce::uint32> importStatusVersion { 0 };
    std::atomic<juce::uint32> bankDisplayGen { 0 };
    int lastBankIndex = -1;                             // audio thread: bank-param change -> bankDisplayGen

   #if OSIW_TEST_HOOKS
    std::atomic<bool> testDisableHeldExclusion { false };
    std::atomic<bool> testGraveyard { false };
    std::vector<std::shared_ptr<const WavetableBank>> graveyard;   // under bankStateLock
    MidBlockFn midBlockFn = nullptr;
    void* midBlockCtx = nullptr;
   #endif

    // Raw parameter pointers (cached in the ctor; atomic loads only).
    std::atomic<float>* pBank       = nullptr;
    std::atomic<float>* pPosition   = nullptr;
    std::atomic<float>* pInterp     = nullptr;
    std::atomic<float>* pBandlimit  = nullptr;
    std::atomic<float>* pBitDepth   = nullptr;
    std::atomic<float>* pAmpAttack  = nullptr;
    std::atomic<float>* pAmpDecay   = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;
    std::atomic<float>* pVoiceMode  = nullptr;
    std::atomic<float>* pOutput     = nullptr;
    std::atomic<float>* pLfoRate     = nullptr;
    std::atomic<float>* pLfoSync     = nullptr;
    std::atomic<float>* pLfoDiv      = nullptr;
    std::atomic<float>* pLfoShape    = nullptr;
    std::atomic<float>* pLfoDepth    = nullptr;
    std::atomic<float>* pMenvAttack  = nullptr;
    std::atomic<float>* pMenvDecay   = nullptr;
    std::atomic<float>* pMenvSustain = nullptr;
    std::atomic<float>* pMenvRelease = nullptr;
    std::atomic<float>* pEnvAmount   = nullptr;

    std::array<WtVoice*, (size_t) kNumVoices> wtVoices {};   // owned by synth

    BlockContext blockCtx;                  // pointer handed to every voice (stable)
    std::vector<float> knobBuf, depthBuf, amtBuf;   // [preparedBlock], sized in prepareToPlay
    PositionLfo lfo;                        // global LFO; its buffer is blockCtx.lfo
    juce::MidiBuffer chunkMidi, wheelMidi;  // ensureSize()d in prepareToPlay

    juce::SmoothedValue<float> outputGain  { 1.0f };
    juce::SmoothedValue<float> knobSmooth  { 0.0f };   // 20 ms position knob (D-D)
    juce::SmoothedValue<float> depthSmooth { 0.0f };   // 20 ms lfo_depth  (D-I)
    juce::SmoothedValue<float> amtSmooth   { 0.0f };   // 20 ms env_amount (D-I)

    MonoStack monoStack;
    int lastVoiceMode = 0;
    int preparedBlock = 512;
    int wheelNow = 8192;                    // W3: last pitch-wheel value on ANY channel (Mono is omni)
    // Note 4: set as the LAST statement of prepareToPlay; processBlock renders
    // only once it is true (silence before). Never cleared in releaseResources
    // (the buffers stay valid; a host that processes after release keeps sound).
    std::atomic<bool> prepared { false };

    std::atomic<float> dispPos      { 0.0f };
    std::atomic<int>   dispLevel    { 0 };
    std::atomic<int>   dispFrame    { 0 };
    std::atomic<bool>  dispSounding { false };
    std::atomic<float> dispLfo      { 0.0f };
    std::atomic<float> dispMenv     { 0.0f };
    std::atomic<float> dispAmp      { 0.0f };
    std::atomic<int>    dispNote    { -1 };         // D-X: lead voice note (-1 silent)
    std::atomic<float>  dispHz      { 0.0f };       // D-X: lead voice Hz incl. bend (0 silent)
    std::atomic<double> displayFs   { 44100.0 };    // D-X: prepareToPlay rate (getSampleRate() is a plain double)
    static_assert (std::atomic<double>::is_always_lock_free, "displayFs must be lock-free");

   #if OSIW_TEST_HOOKS
    bool testKnobRampOff = false;
    bool testPreparedGuard = true;
    bool testMidiGuard = true;
    bool testMonoWheelSeed = true;
    bool testReleaseClears = true;
   #endif

    //==========================================================================
    // Returns this block's resolved bank (published as audioHeldBank).
    const WavetableBank* renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples);
    void renderMono (juce::AudioBuffer<float>& view, const juce::MidiBuffer& chunk, int numSamples);
    void updateDisplayFromLeadVoice() noexcept;
    void resetDisplayState (double fs) noexcept;   // N4: prepareToPlay + releaseResources
    const WavetableBank* resolveBank (int bankIndex) const noexcept;
    juce::ADSR::Parameters currentAmpParams() const noexcept;
    juce::ADSR::Parameters currentModEnvParams() const noexcept;
    float currentKnob() const noexcept;
    float currentLfoDepth() const noexcept;
    float currentEnvAmount() const noexcept;
    float currentOutputGain() const noexcept;

    //==========================================================================
    // State helpers. The IMPORTED_BANK child is written only from
    // processor-owned data (Stage 2.4) and is never kept in the live APVTS
    // tree: it is stripped before replaceState and from every copyState().
    static constexpr const char* kImportedBankTag = "IMPORTED_BANK";
    static constexpr const char* kUiLanguageProp  = "uiLanguage";
    static constexpr const char* kCurrentPresetProp = "currentPreset";   // D-AF: root property, "" = unnamed

    void restoreImportedBank (const juce::ValueTree& childOrInvalid);   // synchronous; any non-audio thread
    void writeImportedBank   (juce::ValueTree& state) const;            // exactly one child from the cache, or none

    static void stripImportedBank (juce::ValueTree& tree)
    {
        // ALL copies: a hand-edited or legacy blob may hold two.
        for (auto c = tree.getChildWithName (kImportedBankTag); c.isValid();
                  c = tree.getChildWithName (kImportedBankTag))
            tree.removeChild (c, nullptr);
    }

    //==========================================================================
    // Publish / retire / sweep. The caller holds bankStateLock (any thread
    // except the audio thread).
    void publishImportedBank (std::shared_ptr<const WavetableBank> nb);
    void retireBank (std::shared_ptr<const WavetableBank> b);
    void sweepRetiredBanks();

    // Import worker.
    void runImportJob (std::unique_ptr<juce::AudioFormatReader> reader, const juce::String& name, juce::uint32 gen);
    void setImportStatus (ImportStatus::State state, const juce::String& filename, int frames, const juce::String& error);

    void handleAsyncUpdate() override;   // D-M: auto-select Imported after a publish (message thread)
    void timerCallback() override;       // 250 ms reaper sweep (message thread)

    //==========================================================================
    // Stage 3 cycle renderer (D-W): message thread only, non-reentrant. Its
    // FFT and scratch are sized here, in the processor ctor (message thread).
    // Declared BEFORE importPool, which stays last.
    CycleRenderer vizRenderer;

    //==========================================================================
    // DECLARED LAST so it is destroyed FIRST (jobs capture this); the
    // destructor also supersedes and drains it (removeAllJobs (true, 10000)).
    juce::ThreadPool importPool { juce::ThreadPoolOptions{}.withThreadName ("OSiW import").withNumberOfThreads (1) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleWavetableAudioProcessor)
};
