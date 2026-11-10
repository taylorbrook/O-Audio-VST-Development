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
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include "GiParamIds.h"
#include "NoteExpression.h"   // modules/tuning/note-expression 1.1.2 (ouaricon_add_module)
#include "OuariconPresetManager.h"   // modules/persistence/preset-manager 1.1.1 (preset FILES only, PLAN-3.1 PD2)
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
// PLAN-3.2 PD9: the Record-in `run` value the page reads, from the recorder's
// packed state (state | reason << 8 | heldArm << 16) and an INJECTED clock.
// Pure: no engine, no time source.
//
//   Armed / Recording / Committing            → 1 / 2 / 3 (and any hold ends)
//   an OBSERVED change into Idle + Committed  → 4, held kHoldMs, then 0
//   an OBSERVED change into Idle + NoSignal
//                        or Idle + TooShort   → 5, held kHoldMs, then 0
//   Idle otherwise                            → 0
//
// The recorder's reason is sticky (cleared only at the next arm), so the hold
// is edge-detected here: a FIRST observation that is already Idle starts no
// hold, and an expired hold never restarts on the same word. A change between
// two Idle words into a reason that holds nothing (WavFailed after Committed)
// leaves a running hold alone; only a non-Idle state ends one. The heldArm bit
// is not part of the comparison.
// ============================================================================
struct GiRecRunHold
{
    static constexpr double kHoldMs = 4000.0;

    int update (std::uint32_t packed, double nowMs) noexcept
    {
        const auto state  = static_cast<ogi::RecState>  (packed & 0xffu);
        const auto reason = static_cast<ogi::RecReason> ((packed >> 8) & 0xffu);
        const std::uint32_t word = packed & 0xffffu;

        if (state != ogi::RecState::Idle)
        {
            seen = true;
            last = word;
            held = 0;
            return static_cast<int> (state);   // 1 / 2 / 3
        }

        if (seen && word != last)
        {
            if (reason == ogi::RecReason::Committed)
            {
                held = 4;
                until = nowMs + kHoldMs;
            }
            else if (reason == ogi::RecReason::NoSignal || reason == ogi::RecReason::TooShort)
            {
                held = 5;
                until = nowMs + kHoldMs;
            }
        }
        seen = true;
        last = word;

        if (held != 0 && ! (nowMs < until))
            held = 0;
        return held;
    }

    void reset() noexcept { seen = false; last = 0; held = 0; until = 0.0; }

private:
    bool          seen = false;
    std::uint32_t last = 0;
    int           held = 0;
    double        until = 0.0;
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
// slot API (requestSlotLoad / getSlotInfo) is message-thread; the WebView
// editor and the Phase 3.2 drop / browser use the same calls.
//
// Stage 3 / Phase 3.1 (PLAN-3.1 PD1 – PD10), all message-thread, none of it
// read by the engine: OuariconPresetManager for preset FILES (the session
// stays on the codec below; the manager's XML session calls are never used),
// the scale / slot revisions the page polls, the pristine lane overview and
// the window figure. No WebView type appears in this header (the consoles
// build with JUCE_WEB_BROWSER=0); the JSON the page reads is built in
// GiUiJson.h.
//
// Stage 3 / Phase 3.2 (PLAN-3.2 PD2 – PD7, PD9 – PD16, PD19): buildUiTelemetry
// (the one reader of the grain frame and of the sidechain peak), the live
// overview + `dirty`, clearSlot, importDroppedFile, saveRecordingTo (a worker
// thread + the engine's GiTapeSnapshot). The only audio-thread addition is two
// relaxed atomic stores per processBlock (the tempo for `rec.capped`).
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
    void releaseResources() override;
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

    // ── Stage 3 / 3.1: presets (message thread; PLAN-3.1 PD1 / PD2) ───────────
    /** Preset FILES only. `output_gain` (the user's trim) and `rec_arm` (a
        preset must never arm a capture) are excluded: never written, never
        reset, never applied. No directory I/O happens until a preset call. */
    OuariconPresetManager& getPresetManager() noexcept { return presetManager; }

    /** The preset's customState: { seed (int64 decimal STRING), custom { name,
        text }, masks [ { stream, collection, bits (16 hex) } ] (non-default
        only), slots [ { index, source, name, fileSize (decimal string),
        contentHash (16 hex, when known), embed, trimmed } ] (non-empty slots
        only; NEVER the embedded blob, PD1) }. */
    juce::var presetCustomState() const;

    /** Applies a preset's customState through applyNonParameterState (reset
        to defaults first; a seed that is not a decimal string counts as absent
        and keeps the live seed). Then the slots: a `slots` ARRAY → the loader
        restores the listed slots (an identical source is a no-op, the resolve
        runs on the loader worker — nothing decodes here) and leaves every
        slot the list has no source for ALONE (PLAN-3.2 PD15, D45: tape,
        source and working copy stay); no `slots` array → all four slots are
        left as they are. */
    void applyPresetCustomState (const juce::var& customState);

    // ── Stage 3 / 3.1: what the page polls (PLAN-3.1 PD3 – PD6, PD9) ──────────
    static constexpr int kOverviewPeaks = 656;   // lane canvas width (v1-ui.yaml lanes.canvas)

    /** Moves whenever the custom scale or a degree mask may have changed
        (any thread reads; the engine never does). */
    std::uint32_t getScaleStateRevision() const noexcept { return scaleRevision.load (std::memory_order_acquire); }

    /** MESSAGE THREAD (non-const: it owns the last fingerprint). Copies the
        four SlotInfo under the loader lock, hashes what the page shows, and
        advances the revision when the hash moved. The first call returns 1. */
    std::uint32_t getSlotStateRevision();

    /** Per-slot overview revision for slot 0…3: moves on every event that can
        change the slot's tape (request, publish, clear, missing) and, from
        3.2 (PD6), every time the record head folds a cell into the live
        overview. See the definition for the formula. */
    std::uint32_t getOverviewRev (int slot0) const;

    /** `dirty[i]` scale (PLAN-3.2 PD7): d = max over the peak's bins of
        |liveLo − pristineLo| + |liveHi − pristineHi|, sent as
        min (255, round (255 · d / kDirtyFullScale)). The page lights a tick
        above 5 % (index.html drawLane: `d = D[k] / 255; if (d > 0.05)`), i.e.
        from byte 13 = a summed edge move of ≈ 0.1 (each edge 5 % of full
        scale). */
    static constexpr float kDirtyFullScale = 2.0f;

    /** The slot's overview folded to `n` peaks in 0…1 (max (|min|, |max|) per
        bin): the loader's PRISTINE bins, each replaced by the engine's live
        bin where the record head rewrote it on the current tape (PD7).
        `dirty` (may be nullptr): kDirtyFullScale above; 0 where no live bin
        differs. Always sets `rev` (= getOverviewRev, read BEFORE the copy, so
        a stale answer is re-fetched, never kept). False + zeros when the slot
        holds no tape. Message thread; takes only the loader lock (the live
        bins are relaxed atomics). */
    bool getSlotOverview (int slot0, float* peaks, std::uint8_t* dirty, int n, std::uint32_t& rev) const;

    /** `slots[i].dirty` (PD7): some live bin of the slot's current tape differs
        from its pristine bin. The CACHED flag buildUiTelemetry maintains (per
        slot only when the head wrote or the tape changed, at most 10 Hz); it
        is part of the slot fingerprint, so slotRev moves when it flips.
        Message thread. */
    bool getSlotDirty (int slot0) const noexcept
    {
        return slot0 >= 0 && slot0 < ogi::kTapeSlots && slotDirty[static_cast<std::size_t> (slot0)];
    }

    // ── Stage 3 / 3.2: telemetry (PLAN-3.2 PD2 – PD5, PD9 – PD13) ─────────────
    /** What getTelemetry sends, as plain values (GiUiJson::telemetryToVar
        encodes it). Slots / streams are 0-based array indices here; the
        numbers the page reads 1-based (grain stream, rec.slot) are already
        1-based in the fields. */
    struct UiTelemetry
    {
        static constexpr int kVoices    = ogi::kVoiceSlots;              // 20 rows
        static constexpr int kMaxGrains = ogi::GiUiFrame::kMaxGrains;    // 288

        struct Lane
        {
            std::uint32_t ovRev = 0;
            bool  hasFb = false;    // false → `fb: null`
            float fb = 0.0f;        // feedback head, 0…1
            bool  hasRec = false;   // false → `rec: null`
            float rec = 0.0f;       // record progress, 0…1 (only while run == 2, target lane)
        };
        struct Voice   // row [state, note, amp, scan1..4, 0]
        {
            int   state = 0;        // 1 = counted voice, 0 = free (no releasing state until 3.3)
            int   note = -1;
            float amp = 0.0f;
            std::array<float, ogi::kGiStreams> scan { { -1.0f, -1.0f, -1.0f, -1.0f } };   // −1 = no playhead
        };
        struct Grain   // row [stream, normPos, pan, env, gain, lengthSec, voice, flags]
        {
            int   stream = 1;       // 1…4
            float normPos = 0.0f, pan = 0.0f, env = 0.0f, gain = 0.0f, lengthSec = 0.0f;
            int   voice = 0;
            int   flags = 0;        // bit 0 = reversed, nothing else (the engine's killing bit is never sent)
        };
        struct Rec
        {
            int   bus = 0;          // 0 | 1
            bool  signal = false;   // a post-gain peak ≥ −90 dBFS within the last 2000 ms
            int   run = 0;          // GiRecRunHold
            int   slot = 0;         // 1…4 (0 before the first take)
            float pos = 0.0f, len = 0.0f;   // frames / target; target / fs (seconds)
            bool  capped = false;   // Tempo mode, not recording, bars × bar > F_max
            float peak = 0.0f;      // sent as [peak, peak]
        };

        std::uint32_t scaleRev = 0, slotRev = 0;
        std::array<Lane, ogi::kTapeSlots> lanes {};
        std::array<Voice, kVoices> voices {};
        std::array<Grain, kMaxGrains> grains {};
        int  grainCount = 0;
        std::array<int, ogi::kGiStreams> counts {};   // neither killing nor in a fade slot
        int  limit = 0;             // 0 → the page parses the grain_limit choice (3.3)
        Rec  rec;
        bool stale = false;         // PD4: the frame did not move for ≥ kStaleMs (voices / grains / counts emptied)
    };

    static constexpr double kStaleMs      = 250.0;    // PD4
    static constexpr double kSignalHoldMs = 2000.0;   // PD10
    static constexpr double kDirtyMinMs   = 100.0;    // PD7: ≤ 10 Hz per slot

    /** MESSAGE THREAD, under telemetryLock (two editors). THE ONLY caller of
        the grain frame (frames.read()) and of the recorder's takeScPeak();
        owns the stale-frame, run-hold, signal and dirty state. `nowMs` is the
        caller's clock (the editor: Time::getMillisecondCounterHiRes(); the
        harness: a scripted one) — never read inside. */
    UiTelemetry buildUiTelemetry (double nowMs);

    // ── Stage 3 / 3.2: slot actions (PLAN-3.2 PD14, PD16, PD19) ───────────────
    /** Empties slot 0…3 through the loader / handoff. Returns an error KEY:
        empty = ok (also when the slot was already empty), "recording" when
        the recorder is Recording or Committing into this slot (Armed is
        allowed), "badSlot". Message thread. */
    juce::String clearSlot (int slot0);

    /** D44: hands a dropped temp file to the loader's import job (hash →
        Imports/<hash>.<ext> → an ordinary file: load named `displayName`).
        The loader owns `tempFile` from here. False = not queued. A failure
        later sets the slot's lastError (the slot keeps its tape). Message
        thread. */
    bool importDroppedFile (int slot0, const juce::File& tempFile, const juce::String& displayName);

    /** The answer of saveRecordingTo. ok → `path` is the written file; else
        `error` is one of cancelled | busy | notRecording | empty | inUse |
        changed | timeout | writeFailed. */
    struct SaveResult
    {
        bool         ok = false;
        juce::String path, error;
    };
    using SaveCompletion = std::function<void (const SaveResult&)>;

    /** D46 (PLAN-3.2 PD19): writes slot 0…3's WORKING COPY (feedback writes
        included) as a 24-bit stereo WAV at the tape rate to `file` (".wav"
        forced; samples beyond ±1.0 clip), through a ".part" sibling.
        Guards, in order: notRecording (the slot's source is not rec:), empty,
        busy (a save in flight, or the buffer of a timed-out save is still
        lent to the audio thread), inUse (the target is this slot's own take
        or a file another slot references). Then a worker allocates the
        buffer, the AUDIO thread copies the tape into it (GiTapeSnapshot), and
        the worker writes the file. No audio callback for 2000 ms → `timeout`.
        MESSAGE THREAD. `completion` runs exactly once, on the message thread:
        inline for a guard refusal, else asynchronously (without a
        MessageManager: on the worker thread). */
    void saveRecordingTo (int slot0, const juce::File& file, SaveCompletion completion);

    static constexpr double kSaveNoProgressMs = 2000.0;   // PD19: keyed on the snapshot's progress counter

    /** The window figure of stream 0…3: `n` points of the engine's own bank
        curve at the BASE sN_window / sN_window_skew (no modulation). */
    void getWindowCurve (int stream0, float* out, int n) const noexcept;

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

    /** PLAN-3.1 SR1n control: the slot fingerprint WITHOUT its tape-identity
        term (SlotInfo::generation and the handoff publish count folded with
        it), so a same-source reload is missed. getOverviewRev is unaffected. */
    bool testSlotRevNoGeneration = false;

    /** PLAN-3.2 UOV2n-b control: the slot fingerprint WITHOUT the dirty flag
        (slotRev then stays put when a lane turns dirty). */
    bool testSlotRevNoDirty = false;
    /** PLAN-3.2 UOV2n control: getOverviewRev WITHOUT the live write count
        (the 3.1 formula: ovRev stays put while the head writes). */
    bool testOvRevNoWrites = false;

    /** PLAN-3.2 PD19 (USV rows): saveRecordingTo on the CALLING thread's
        terms. Blocks until the completion ran; while waiting it calls `pump`
        (the harness's processBlock driver; empty → it sleeps, i.e. "audio
        stopped") and delivers the completion on the calling thread. The
        2000 ms no-progress limit is the real one. */
    SaveResult saveRecordingSync (int slot0, const juce::File& file, const std::function<void()>& pump = {});
    /** USV2: true while a timed-out save's buffer is still allocated. */
    bool saveBufferHeldForTest() const;
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

    void bumpScaleRevision() noexcept { scaleRevision.fetch_add (1, std::memory_order_acq_rel); }

    // 3.2: the revision bodies over values the caller already copied (one
    // SlotInfo copy per slot and telemetry build). `published` = the handoff's
    // publish count, read BEFORE the infos.
    std::uint32_t overviewRevFor (int slot0, const ogi::SlotInfo& info, std::uint64_t published) const noexcept;
    std::uint32_t slotRevisionFor (const std::array<ogi::SlotInfo, ogi::kTapeSlots>& infos, std::uint64_t published);
    /** PD7: re-evaluates slotDirty[slot0] when its key moved (≤ 10 Hz). */
    void updateSlotDirty (int slot0, const ogi::SlotInfo& info, std::uint64_t published, double nowMs);

    // 3.2 save flow (PD19); see the comment at saveWorkerRun in the .cpp.
    struct SaveJob
    {
        int            slot0 = -1;
        juce::File     file;        // ".wav" already forced
        int            frames = 0;
        double         rate = 0.0;
        SaveCompletion completion;
    };
    void saveWorkerRun();
    SaveResult runSaveJob (const SaveJob& job);
    juce::String writeSaveWav (const SaveJob& job);   // empty = written; else writeFailed | cancelled
    void finishSave (SaveCompletion completion, const SaveResult& result);
    void deliverSaveCompletions();
    bool reclaimSaveBufferLocked() noexcept;         // release() → free; true = nothing is lent any more
    void reclaimSaveBufferAudioStopped() noexcept;   // prepareToPlay / releaseResources (rule (b))

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

    // Stage 3 / 3.1 (PLAN-3.1 PD2 – PD4). The manager holds a reference to
    // `parameters` (declared above) and two callbacks into this object; it
    // touches no directory until a preset call. scaleRevision is a UI-only
    // counter. The slot fingerprint members are message-thread only.
    OuariconPresetManager presetManager { parameters, "O-GranulatorInstrument" };
    std::atomic<std::uint32_t> scaleRevision { 0 };
    std::uint64_t slotFingerprint = 0;
    std::uint32_t slotRevision = 0;
    bool slotFingerprintValid = false;

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

    // ── 3.2 telemetry state (message thread, under telemetryLock) ────────────
    juce::CriticalSection telemetryLock;       // never taken on the audio thread
    bool          frameSeqSeen = false;        // PD4
    std::uint64_t frameSeqLast = 0;
    double        frameSeqMovedMs = 0.0;
    GiRecRunHold  recRunHold;                  // PD9
    bool          signalSeen = false;          // PD10
    double        signalLastMs = 0.0;

    // PD7 (message thread): the cached dirty flags and the key each was
    // evaluated at.
    struct DirtyKey
    {
        std::uint32_t writes = 0, epoch = 0;
        std::uint64_t tape = 0;     // generation + publish count
        bool          loaded = false;
        bool operator== (const DirtyKey& o) const noexcept
        {
            return writes == o.writes && epoch == o.epoch && tape == o.tape && loaded == o.loaded;
        }
    };
    std::array<bool, ogi::kTapeSlots>     slotDirty {};
    std::array<DirtyKey, ogi::kTapeSlots> slotDirtyKey {};
    std::array<bool, ogi::kTapeSlots>     slotDirtyKeyValid {};
    std::array<double, ogi::kTapeSlots>   slotDirtyEvalMs {};

    // PD11: the transport as the tempo tracker reads it, stored once per
    // processBlock (relaxed; the audio thread never reads them back). bpm 0 =
    // nothing valid reported (the reader falls back to 120, as the tracker).
    std::atomic<double> uiBpm { 0.0 };
    std::atomic<double> uiBarQuarters { 4.0 };

    // ── 3.2 save flow (PD19). saveLock guards every member below AND every
    // owner call into the engine's GiTapeSnapshot (request / cancel / release
    // / resetAudioStopped), so rule (b) of its lifetime invariant holds.
    struct SaveWorker final : juce::Thread
    {
        explicit SaveWorker (GranulatorInstrumentProcessor& o) : juce::Thread ("OGI save"), owner (o) {}
        void run() override { owner.saveWorkerRun(); }
        GranulatorInstrumentProcessor& owner;
    };
    mutable juce::CriticalSection saveLock;
    std::unique_ptr<SaveJob> saveJob;          // queued for the worker (at most one)
    bool saveInFlight = false;                 // a job is queued or running
    bool saveLent = false;                     // saveBufL / R are still lent to the audio thread (after a timeout / exit)
    juce::HeapBlock<float> saveBufL, saveBufR; // allocated by the worker; freed only per the invariant
    std::vector<std::pair<SaveCompletion, SaveResult>> saveDone;   // finished, not yet delivered
    std::shared_ptr<std::atomic<bool>> saveAlive { std::make_shared<std::atomic<bool>> (true) };   // guards callAsync
    std::unique_ptr<SaveWorker> saveWorker;    // started at the first save; joined first in the destructor

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranulatorInstrumentProcessor)
};
