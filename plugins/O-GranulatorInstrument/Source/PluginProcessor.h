/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>
#include "GiParamIds.h"
#include "NoteExpression.h"   // modules/tuning/note-expression 1.1.2 (ouaricon_add_module)
#include "dsp/Engine.h"
#include "dsp/SlotHandoff.h"
#include "dsp/SlotLoader.h"

// Sidechain default-active flag. The shipping build leaves it at 1; the
// gate-only CMake option OGI_GATE_SC_DEFAULT_OFF compiles 0 into the PLUGIN
// target. Consoles never inherit the plugin's compile definitions, so the
// render harness always tests the shipping default (PLAN P7 / P8).
#ifndef OGI_SC_DEFAULT_ACTIVE
 #define OGI_SC_DEFAULT_ACTIVE 1
#endif

// ============================================================================
// D9: the only input bus is the Record-in "Sidechain". On VST3 the first input
// bus is typed kAux exactly when getPluginHasMainInput() returns false
// (juce_VST3ClientExtensions.h; juce_audio_plugin_client_VST3.cpp getBusInfo).
// The note-expression module class does not override it, so this plugin-local
// subclass does.
// ============================================================================
struct GiVst3Extensions final : Ouaricon::NoteExpression::VST3Extensions
{
    bool getPluginHasMainInput() const override { return false; }

    // PLAN-2.5b PD6: the stamped NE path (no module edit). The VST3 wrapper
    // calls this on the audio thread inside the same process() call, before
    // processBlock / processBlockBypassed (juce_audio_plugin_client_VST3.cpp
    // :3588-3640 → :3779, patched). The module path runs first (its table
    // stays intact); then a fixed 64-entry copy (drop beyond 64, counted).
    void onVst3RawEvent (const Vst3RawEvent& e) override
    {
        VST3Extensions::onVst3RawEvent (e);
        if (neCount < kNeCap)
            neBuf[(size_t) neCount++] = e;
        else
            ++neDropped;
    }

    static constexpr int kNeCap = 64;
    std::array<Vst3RawEvent, kNeCap> neBuf {};
    int neCount = 0;
    std::uint64_t neDropped = 0;
};

// ============================================================================
// O-GranulatorInstrument, Stage 2 / Phase 2.2.
//
// 261 APVTS parameters (GiParamIds.h), a stereo instrument output plus ONE
// optional stereo "Sidechain" input that never reaches the output, latency 0,
// and the state: root uiLanguage attribute, <Engine seed stateVersion
// modDestTableVersion/>, <CustomScale/>, <DegreeMasks/> and an empty <Slots/>
// (slot paths persist from 2.6).
//
// Phase 2.2: the ogi::Engine renders 4 grain streams per voice, stream N
// reading slot N (Source/dsp/Engine.h); the engine copies the in-place
// sidechain into a scratch and zeroes each output piece before rendering it
// (PLAN-2.6 PD19), so the sidechain never reaches the output. Slot files load off-thread
// through ogi::SlotLoader and are installed lock-free by ogi::SlotHandoff.
//
// Scale state (CONTEXT-2.2 D17, PLAN-2.2 T6 / PD3 / PD4): ONE shared custom
// scale (used by any sN_scale = Custom) and 4 × 32 degree masks (per stream
// and per collection), ported from O-Granulator 82406fce. The engine reads
// the masks as relaxed atomics and copies the custom table through a
// non-blocking try-lock when customVersion moves; nothing parses on the audio
// thread. Message-thread API only (no native functions / UI until Stage 3).
//
// Threading: no listener and no audio-thread code writes a parameter. The
// slot API (requestSlotLoad / getSlotInfo) is message-thread; the dev-loader
// editor and the Phase 3.2 drop / browser use the same calls.
// ============================================================================
class GranulatorInstrumentProcessor : public juce::AudioProcessor
{
public:
    GranulatorInstrumentProcessor();
    ~GranulatorInstrumentProcessor() override;

    /** D9 seam: the one place the bus set is declared. Sidechain input (active
        per OGI_SC_DEFAULT_ACTIVE) + stereo output. Any D9 fallback lands here. */
    static BusesProperties makeBuses();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    /** PLAN-2.5b PD6 / P6: a bypassed block drops this block's raw NE events
        (never replayed later at stale offsets). PLAN-2.6 PD20: then clears
        EVERY channel (the base would pass the sidechain through, RS1) and
        tells the engine (the recorder cancels / stops). */
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlockBypassed;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState parameters;

    // ── Seed (deterministic per-grain randomness; persisted as an int64 string)
    uint64_t getSeed() const noexcept    { return seed.load (std::memory_order_relaxed); }
    void setSeed (uint64_t s) noexcept   { seed.store (s, std::memory_order_relaxed); }

    // ── UI language: a state-tree ROOT attribute, not a parameter (0 en, 1 fr, 2 zh-Hans)
    std::atomic<int> uiLanguage { 0 };
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }

    /** Sidechain channels from the NEGOTIATED bus (0 = no bus / disabled),
        decided in prepareToPlay. Never derived from the buffer width. */
    int getSidechainChannelCount() const noexcept { return sidechainChannels.load (std::memory_order_relaxed); }

    // ── Slots (message thread; shared with Phase 3.2) ─────────────────────────
    /** Enqueues an off-thread load of `file` into slot 0...3 (a newer request
        for the same slot cancels the older one). */
    void requestSlotLoad (int slot, const juce::File& file);
    /** A copy of the slot's status, taken under the loader lock. */
    ogi::SlotInfo getSlotInfo (int slot) const;
    /** PLAN-2.4 PD6: Reset slot 0...3 (−1 = all) to its pristine source: an
        off-thread rebuild taken through the 20 ms dual-read crossfade. Not a
        parameter, never saved (the working copy reloads pristine). Stage 3
        binds the native function. */
    void requestReset (int slot);

    /** PLAN-2.6 (message thread; Stage 3.2 binds them): relink a missing /
        moved slot to `file` (PD14), and the D36 per-slot embed toggle. */
    void requestRelink (int slot, const juce::File& file);
    void setSlotEmbed (int slot, bool on);

    /** PLAN-2.6 PD15: Record-in telemetry (any thread, lock-free).
        recState = state | reason << 8 | heldArm << 16 (GiRecorder.h codes). */
    std::uint32_t getRecordState() const noexcept { return engine->getRecorder().getPackedState(); }

    // ── Scale state (message thread; D17) ─────────────────────────────────────
    struct CustomScale
    {
        juce::String name, text;
        float period = 1200.0f;
        uint32_t version = 0;
    };
    static constexpr int kMaxCustomNameChars = 40;
    static const juce::String& defaultCustomText();

    CustomScale getCustomScale() const;
    PitchCollections::Table getCustomTable() const;

    /** Parses `text` (cents, ratios or edo N [period]) into the ONE custom scale
        shared by all streams. Returns an error KEY (empty = applied); badToken
        names the offending token for badPitch. On success every stream's Custom
        mask resets to all-on (the degrees re-numbered), then customVersion
        advances (acq_rel) so the engine copies the table at its next tick. */
    juce::String setCustomScale (const juce::String& name, const juce::String& text, juce::String& badToken);

    /** sN_scale choice → table: nullptr for 0 (Off) / out of range; 1…30 the
        built-ins (via scaleChoiceToId), 31 the custom table. */
    const PitchCollections::Table* getCollectionTable (int choice) const;

    /** Degree mask for stream 1…4 × collection choice 1…31. Bit d = degree d
        on. Stored canonical (bits at and above the degree count set), so
        "all on" is always ~0. Out-of-range arguments read ~0 / are ignored. */
    uint64_t getDegreeMask (int stream, int collection) const noexcept;
    void setDegreeMask (int stream, int collection, uint64_t mask) noexcept;

    static constexpr uint64_t kAllOn = ~uint64_t (0);
    static uint64_t canonicalMask (uint64_t mask, int degreeCount) noexcept
    {
        return degreeCount >= 64 ? mask : (mask | ~((uint64_t (1) << degreeCount) - 1));
    }

    /** Mask array index for stream 1…4 (1-based) × collection 0…31. */
    static constexpr int maskIndex (int stream, int collection) noexcept
    {
        return (stream - 1) * GiParamIds::kNumCollections + collection;
    }

   #if OGI_TEST_HOOKS
    /** Render harness only. Decode / resample + build the tape for slot 0...3
        (= the rec_target index) on the CALLING thread and publish it through the
        same pending exchange as the loader worker. False + error on failure
        (missing / unreadable file, source shorter than 10 ms, bad rate). */
    bool loadSlotSync (int slot, const juce::File& file, juce::String& error);
    bool loadSlotSync (int slot, const juce::AudioBuffer<float>& pcm, double sourceRate,
                       const juce::String& name, juce::String& error);
    /** PLAN-2.4 PD6: Reset on the CALLING thread (deterministic rows). */
    bool resetSlotSync (int slot);

    ogi::Engine&      getEngine() noexcept  { return *engine; }
    ogi::SlotLoader&  getLoader() noexcept  { return *loader; }
    ogi::SlotHandoff& getHandoff() noexcept { return handoff; }

    /** ST6: the harness holds this to prove the engine's try-lock never blocks. */
    juce::SpinLock& getCustomLockForTest() noexcept { return customLock; }
    /** ST4 control: the codec without removeCustomChildren on either leg. */
    bool testNoStateStrip = false;
    /** ST3 control: setStateInformation skips resetNonParameterState(). */
    bool testNoStateReset = false;
    /** PLAN-2.6 ST5n control: <Slot> embed / trimmed parsed with var::isBool()
        (XML gives strings, so the defaults come back). */
    bool testParseIsBool = false;

    /** PLAN-2.6 PD7: the commit poll + job on the CALLING thread
        (deterministic rows). False when no take is ready. */
    bool commitSync();
   #endif

    /** PLAN-2.6 PD7: the off-audio poll (message-thread Timer, or a watcher
        thread when there is no MessageManager): copy a ready take out of the
        recorder, then hand it to the loader's worker. */
    void pollRecorder();

private:
    void updateSidechainChannelCount() noexcept;
    void setEngineDestRanges();   // PLAN-2.5a PD3 (message / host thread)

    int degreeCountFor (int collection) const;
    void resetNonParameterState();

    /** THE non-parameter apply path (parent P98): reset to defaults FIRST →
        seed (if present and well-formed; absent keeps the live seed) → custom
        scale (if `custom` has a non-empty text) → masks. Inputs are vars:
        `custom` an object { name, text }, `masks` an array of { stream,
        collection, bits } objects, `seed` an int64 decimal string. Void =
        absent. Message thread. */
    void applyNonParameterState (const juce::var& custom, const juce::var& masks, const juce::var& seed);

    /** PLAN-2.6 PD21: the <Slots> children, type-safely parsed (R§6.1). */
    std::vector<ogi::SavedSlot> parseSlots (const juce::ValueTree& slotsTree) const;

    GiVst3Extensions vst3Extensions;
    std::array<ogi::GiNeEvent, GiVst3Extensions::kNeCap> neEvents {};   // PD6: this block's converted raw events
    std::atomic<uint64_t> seed { 0 };
    std::atomic<int> sidechainChannels { 0 };

    // Scale state (D17 / PD3). customLock guards customTable / customText /
    // customName; the audio thread only ever TRY-locks it (at a grid tick, when
    // customVersion moved). degreeMasks: relaxed atomics, all-on by default.
    mutable juce::SpinLock customLock;
    PitchCollections::Table customTable;
    juce::String customText, customName;
    std::atomic<uint32_t> customVersion { 0 };
    std::array<std::atomic<uint64_t>, GiParamIds::kStreams * GiParamIds::kNumCollections> degreeMasks;

    // Declaration order = construction order; destruction runs in reverse, so
    // the loader (worker jobs that publish) goes first, then the engine (frees
    // the tapes it holds), then the handoff (pending / ring / deferred tapes),
    // then the format manager the loader's reader creation used.
    juce::AudioFormatManager formatManager;
    ogi::SlotHandoff handoff;
    std::unique_ptr<ogi::Engine> engine;
    std::unique_ptr<ogi::SlotLoader> loader;

    // PLAN-2.6 PD7: the commit poll. A 20 ms message-thread Timer; without a
    // MessageManager a small watcher thread runs the same poll. Destroyed
    // first (declared last), before the loader / engine it calls.
    struct RecPollTimer final : juce::Timer
    {
        explicit RecPollTimer (GranulatorInstrumentProcessor& o) : owner (o) {}
        void timerCallback() override { owner.pollRecorder(); }
        GranulatorInstrumentProcessor& owner;
    };
    struct RecPollThread final : juce::Thread
    {
        explicit RecPollThread (GranulatorInstrumentProcessor& o) : juce::Thread ("OGI rec poll"), owner (o) {}
        void run() override
        {
            while (! threadShouldExit())
            {
                owner.pollRecorder();
                wait (20);
            }
        }
        GranulatorInstrumentProcessor& owner;
    };
    std::unique_ptr<RecPollTimer>  recPollTimer;
    std::unique_ptr<RecPollThread> recPollThread;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranulatorInstrumentProcessor)
};
