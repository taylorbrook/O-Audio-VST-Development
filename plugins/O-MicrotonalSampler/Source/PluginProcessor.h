/*
   This file is part of O-MicrotonalSampler, an Ouaricon Audio plugin.
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

    O-MicrotonalSampler - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Microtonal multi-sample instrument: APVTS parameters, TuningEngine
    (Scala/EDO), VST3 Note Expression for Dorico, a background SampleLoader, a
    round-robin / velocity-layer / technique sample map, and a WebView UI.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include "MicrotonalSamplerSound.h"
#include "MicrotonalSamplerVoice.h"
#include "SampleMap.h"
#include "RetiredMapReaper.h"      // v1.23.2 (W10) — off-audio-thread map free
#include "SampleLoader.h"
#include "TriggerMapping.h"        // v1.15.0 — CC + PC trigger tables
#include "TechniqueDefaults.h"     // v1.23.3 — canonical technique-axis defaults
#include "TuningEngine.h"          // global namespace (D-4)
#include "NoteExpression.h"        // modules/tuning/note-expression (via ouaricon_add_module)

// v1.6.0: explicit velocity-layer assignment for folder loads.
//
// LoadMode controls how a freshly loaded folder merges into the current map:
//   ReplaceAll   - wipe the existing map and load only this folder's slots.
//                  Truncates loadOpHistory to a single op. Reproduces v1.5.x
//                  behaviour and is the only path used by legacy state
//                  (<SampleFolder path>) restores.
//   ReplaceLayer - drop every existing slot whose velocityLayer equals the
//                  op's targetLayer, then merge the new slots. Other layers
//                  untouched.
//   Append       - merge the new slots into the existing map. (midi, layer)
//                  collisions are overwritten by the new slot.
//   MergeRR      - v1.9.0. Like Append, but on (midi, layer) collisions the
//                  new cell's variants are appended to the existing cell's
//                  variants vector instead of replacing it. Used to layer
//                  multiple recordings as round-robin alternates on the same
//                  notes. Per-cell variant count is capped at kMaxVariants
//                  (64); excess variants are skipped. RR counter for every
//                  affected cell is reset to the sentinel.
//
// Each successful load appends one LoadOp to loadOpHistory; clearSampleMap
// truncates the history. getStateInformation persists the history so a
// project reopen replays the same sequence of folder loads (subject to the
// folders still existing on disk; first missing folder triggers the
// existing missing-folder modal flow).
enum class LoadMode
{
    ReplaceAll   = 0,
    ReplaceLayer = 1,
    Append       = 2,
    MergeRR      = 3
};

struct LoadOp
{
    juce::String path;            // Absolute folder path (UTF-8). For drag-drop
                                  // ops this points at a session temp dir (used
                                  // for in-session reload only); never persisted
                                  // to state for drag-drop kind.
    int          targetLayer    = 0;     // 0..3.
    LoadMode     mode           = LoadMode::ReplaceAll;
    bool         overrideTokens = false; // true = ignore filename velocity tokens.

    // v1.12.0 — drag-drop persistence + audio embedding.
    //
    // origin classifies the load source:
    //   "filesystem" — Load Folder dialog (path is stable across sessions).
    //   "drag-drop"  — WebView drag-drop content streaming. The temp path is
    //                  short-lived; only the displayName is persisted unless
    //                  embedAudio is true.
    //
    // displayName is the user-visible folder name (e.g. "MyDrumKit"). For
    // filesystem ops it's File::getFileName(path); for drag-drop ops it comes
    // from JS (FileSystemEntry::name) since macOS WKWebView sandboxes the
    // original path.
    //
    // embedAudio=true causes captureStateValueTree to serialise the loaded
    // audio data inline (per-variant base64 WAV) so the project survives folder
    // moves / cross-machine transfer / drag-drop temp-dir cleanup. Default OFF
    // (path-reference behaviour, project-state stays small).
    juce::String origin       { "filesystem" };
    juce::String displayName;
    bool         embedAudio   { false };

    // v1.14.0 — technique-axis options (mirror SampleMap.h::LoadOptions).
    // overrideTechnique=true forces every loaded slot onto targetTechnique
    // (used by the "assign folder to technique" load modal). overrideTechnique
    // =false (default) lets filename tokens decide; missing tokens land on
    // slot 0 ("ord") for back-compat with v1.13.0 single-technique libraries.
    int          targetTechnique   { 0 };
    bool         overrideTechnique { false };

    // In-memory only — never serialised directly into the LoadOp XML attrs.
    // When embedAudio=true, this snapshot of the slots produced by this op is
    // walked at capture time to emit inline audio blobs. Populated by
    // applyFolderLoad on the live load path, and by restoreStateValueTree
    // when reading back an <Audio> child. Holds shared_ptr<AudioBuffer>s that
    // are also held by currentSampleMap, so memory cost is metadata only.
    std::shared_ptr<SampleMap> embeddedSlots;
};

// Polyphony-cap-enforcing Synthesiser. Pre-allocated voice pool stays at the
// max (16) for PERF-01 (no RT alloc when the user raises the cap), but the
// runtime cap from the `polyphony` APVTS parameter is enforced here by
// pre-stealing on every noteOn so the active count never exceeds the cap.
//
// FUNC-03 ("up to 16-voice polyphony") was structurally complete in Stage 2
// (16 voices pre-allocated) but functionally incomplete: the parameter was
// wired through APVTS + WebSlider but never read by the audio engine, so
// lowering the cap had no effect. This subclass closes that gap.
class CappedSynthesiser : public juce::Synthesiser
{
public:
    void setVoiceCap (int cap) noexcept
    {
        voiceCap.store (juce::jlimit (1, 16, cap), std::memory_order_relaxed);
    }

    // v1.7.1: lock-free snapshot of currently held MIDI notes (0..127) as a
    // pair of 64-bit bitmasks. Bit n in `low` = note n; bit n in `high` =
    // note n+64. Read on the message thread by the editor's 30 Hz timer to
    // diff against the previous snapshot and emit per-note events to the
    // tuning panel. Writes happen on the audio thread inside noteOn/noteOff.
    void getActiveNotes (juce::uint64& low, juce::uint64& high) const noexcept
    {
        low  = activeNotesLow .load (std::memory_order_relaxed);
        high = activeNotesHigh.load (std::memory_order_relaxed);
    }

protected:
    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override
    {
        // Called from inside renderNextBlock under the synth's recursive lock.
        // Counting + steal here is safe (lock is reentrant) and keeps the cap
        // enforced before the base implementation picks a voice via
        // findFreeVoice.
        const int cap = voiceCap.load (std::memory_order_relaxed);

        int active = 0;
        for (int i = 0; i < getNumVoices(); ++i)
            if (getVoice (i)->isVoiceActive())
                ++active;

        if (active >= cap && getNumSounds() > 0)
        {
            if (auto sound = getSound (0))
            {
                if (auto* steal = findVoiceToSteal (sound.get(), midiChannel, midiNoteNumber))
                    stopVoice (steal, /*velocity=*/0.0f, /*allowTailOff=*/false);
            }
        }

        // v1.7.1: track held note (audio-thread safe — atomic OR).
        if (juce::isPositiveAndBelow (midiNoteNumber, 128))
            setHeldBit (midiNoteNumber, true);

        juce::Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
    }

    void noteOff (int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override
    {
        // v1.7.1: clear held bit. We treat note-off as the user releasing the
        // key (matches O-Bells convention) — the tuning panel highlights are
        // about which keys are *being played*, not whether the voice is still
        // in its release tail. ADSR release continues independently below.
        if (juce::isPositiveAndBelow (midiNoteNumber, 128))
            setHeldBit (midiNoteNumber, false);

        juce::Synthesiser::noteOff (midiChannel, midiNoteNumber, velocity, allowTailOff);
    }

private:
    void setHeldBit (int midi, bool on) noexcept
    {
        const juce::uint64 mask = (juce::uint64) 1 << (midi & 63);
        auto& target = (midi < 64) ? activeNotesLow : activeNotesHigh;
        if (on)
            target.fetch_or  (mask, std::memory_order_relaxed);
        else
            target.fetch_and (~mask, std::memory_order_relaxed);
    }

    std::atomic<int>          voiceCap         { 16 };
    std::atomic<juce::uint64> activeNotesLow   { 0 };  // MIDI 0..63
    std::atomic<juce::uint64> activeNotesHigh  { 0 };  // MIDI 64..127
};

class OMicrotonalSamplerAudioProcessor : public juce::AudioProcessor,
                                         private juce::AsyncUpdater
{
public:
    OMicrotonalSamplerAudioProcessor();
    ~OMicrotonalSamplerAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // v1.12.1 — CR-01 fix. processBlock stages the latest CC 11 byte into
    // pendingCC11Value (audio thread, lock-free) and calls
    // triggerAsyncUpdate(); handleAsyncUpdate runs on the message thread and
    // forwards to the host via setValueNotifyingHost. Last-value-wins within a
    // block matches the previous behaviour; the difference is that the host
    // notification is no longer issued from the audio thread.
    void handleAsyncUpdate() override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-MicrotonalSampler"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }  // No fixed tail reported to the host

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Public access to APVTS for editor
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Public access to tuning engine (forward-compat for Phase 2.1+)
    TuningEngine* getTuningEngine() { return &tuningEngine; }

    // v1.23.4 (WR-01): the same persistenceLock that guards loadOpHistory /
    // lastSkippedFiles against Reaper's off-message-thread getStateInformation
    // also serialises techniqueNames and the TuningEngine capture. The editor's
    // tuning-WRITE native functions take this lock so a concurrent off-thread
    // save can't tear-read the engine mid-mutation. Recursive (JUCE
    // CriticalSection), never acquired from the audio thread.
    juce::CriticalSection& getPersistenceLock() noexcept { return persistenceLock; }

    // VST3 Note Expression (kTuningTypeID) - Dorico microtonal playback.
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

    // Drag-drop folder load entry point (called from PluginEditor).
    // Spawns the background SampleLoader; on completion (message thread) the
    // new slots are merged into currentSampleMap per the supplied LoadMode,
    // any skipped files are recorded in lastSkippedFiles, and the op is
    // appended to loadOpHistory for state persistence.
    //
    // v1.6.0: targetLayer/mode/overrideTokens replace the old single-folder
    // behaviour. Call with (folder, 0, LoadMode::ReplaceAll, false) to get
    // exact v1.5.x semantics — used by the missing-folder relocate flow and
    // by legacy state restore.
    void loadSampleFolder (const juce::File& folder,
                           int               targetLayer    = 0,
                           LoadMode          mode           = LoadMode::ReplaceAll,
                           bool              overrideTokens = false);

    // v1.12.0: full-fidelity overload — adds origin classification, a
    // user-visible displayName for the missing-folder modal, and the
    // embedAudio opt-in for inline audio serialisation. Used by the WebView
    // drag-drop streaming path (origin="drag-drop") and by the embed-aware
    // dialog path. The legacy 4-arg overload above forwards here with
    // origin="filesystem", displayName=folder.getFileName(), embedAudio=false.
    //
    // v1.17.0: targetTechnique / overrideTechnique appended as defaulted
    // params — surfaces the LoadOp::overrideTechnique path that has
    // existed since v1.14.0 but was previously only reachable through
    // direct LoadOp construction. Defaults preserve pre-v1.17.0 callers
    // (technique=0, overrideTechnique=false → identical SampleLoader
    // effectiveTechnique decision).
    void loadSampleFolder (const juce::File& folder,
                           int               targetLayer,
                           LoadMode          mode,
                           bool              overrideTokens,
                           const juce::String& origin,
                           const juce::String& displayName,
                           bool              embedAudio,
                           int               targetTechnique   = 0,
                           bool              overrideTechnique = false);

    // Read-only accessor for Stage-3 UI: list of files the loader skipped
    // (unparseable filenames, unreadable files, etc.). Refreshed on each
    // loadSampleFolder completion; cleared on failure.
    const juce::StringArray& getLastSkippedFiles() const noexcept { return lastSkippedFiles; }

    // Phase 3.1: per-cell load (full implementation in 3.2). Skeleton logs
    // and returns. UI calls this when the user single-clicks an empty cell or
    // double-clicks a loaded cell to replace.
    //
    // v1.8.0 semantics: replaces the cell with a single-variant cell. Existing
    // multi-variant cells (built via folder load with rr/take/tk tokens) are
    // collapsed to a single variant on user replace — explicit override of any
    // RR setup at that coordinate. To build a multi-variant cell, use folder
    // load.
    //
    // v1.9.0: when `mergeAsRr` is true and the target cell already exists, the
    // new variant is appended to the existing variants vector (capped at 64)
    // instead of replacing the cell. The UI surfaces a confirm prompt for
    // non-empty cells; default value preserves v1.8.0 behaviour for legacy
    // callers.
    void loadSingleSample (int midiPitch, int velocityLayer, const juce::File& file,
                           bool mergeAsRr = false,
                           int technique = 0);   // v1.14.0

    // v1.8.0: confirm or reject a folder load that surfaced ambiguous
    // duplicate (midi, layer) groups (no explicit rr/take/tk tokens).
    // accept=true commits the staged map (treats duplicates as round-robin
    // variants); accept=false discards the staged map without applying.
    // No-op when no map is staged. Message-thread only.
    void confirmRoundRobinLoad (bool accept);

    // v1.8.0: read-only accessor — true while a folder-load result is staged
    // pending user confirmation. The editor can use this on attach to re-emit
    // the modal if it missed the initial event.
    bool hasPendingRoundRobinConfirmation() const noexcept
    {
        return pendingDuplicateMap != nullptr;
    }

    // v1.8.0: snapshot of staged ambiguous duplicates for the editor's modal.
    // Returns empty when no confirmation is pending.
    const std::vector<SampleLoader::AmbiguousDuplicate>& getPendingAmbiguousDuplicates() const noexcept
    {
        return pendingAmbiguousDuplicates;
    }

    // v1.0.2: destructive — replaces currentSampleMap with a fresh empty
    // SampleMap (version bumped). Active voices keep their previously
    // snapshotted map alive transitively until release (Stage 2 EC-3); new
    // note-ons after the clear find an empty map and produce silence. Caller
    // (UI) is responsible for confirmation dialog. Message-thread only.
    void clearSampleMap();

    // Phase 3.4: loop-point override (full implementation). Atomically
    // deep-copies the current SampleMap, mutates the (midi, vel) cell's
    // selected variant, bumps version, atomic-stores, fires callback.
    //
    // v1.8.0: variantIndex selects which variant to override within the cell.
    // -1 (default) = primary variant (index 0) for back-compat with single-
    // variant cells. resetToAutoDetect=true snaps the variant back to
    // whole-file loop default. crossfadeLen recorded for v1.1.
    void overrideLoopPoints (int midiPitch, int velocityLayer,
                             int loopStart, int loopEnd,
                             int crossfadeLen,
                             bool resetToAutoDetect = false,
                             int variantIndex = -1,
                             int technique = 0);   // v1.14.0

    // Phase 3.4: convenience wrapper — calls overrideLoopPoints with the
    // resetToAutoDetect flag set. variantIndex defaults to primary (0).
    void resetLoopToAutoDetect (int midiPitch, int velocityLayer,
                                int variantIndex = -1,
                                int technique = 0);   // v1.14.0

    // v1.18.0: batch loop-point override — applies one loop region to EVERY
    // variant of EVERY cell in the current map at once (the per-cell editor
    // generalised across the whole library). Deep-copies the map, walks all
    // cells/variants, sets loopMode = Manual, bumps version, atomic-stores,
    // fires the change callback. Returns the number of variants updated.
    //
    //   mode 0 (Proportional): startVal/endVal are fractions in [0, 1] of each
    //          variant's own length. ls = round(startVal * N),
    //          le = round(endVal * N).
    //   mode 1 (Milliseconds): startVal/endVal are milliseconds from the file
    //          start. ls = round(startVal/1000 * sourceSampleRate),
    //          le = round(endVal/1000 * sourceSampleRate). Falls back to
    //          48 kHz when a variant's sourceSampleRate is unknown (0).
    //
    // Both modes clamp per-variant exactly like overrideLoopPoints and SKIP
    // one-shot buffers (< 18 samples), leaving them untouched. crossfadeLen is
    // recorded for v1.1 parity (currently ignored). Message-thread only.
    //
    // NB (v1.18.0 known limitation): like overrideLoopPoints, this mutates the
    // live map only — it is NOT recorded as a load-op, so the override does not
    // survive a project reload that replays the op history. See CHANGELOG.
    int applyLoopPointsToAll (int mode, double startVal, double endVal,
                              int crossfadeLen = 8);

    // v1.18.0: delete a single (midi, vel, technique) cell from the current
    // map — the granular counterpart to clearSampleMap(). Deep-copies the map,
    // erases the matching cell, recomputes note bounds, resets that cell's RR
    // counter, bumps version, atomic-stores, fires the change callback. No-op
    // (returns false) when the cell is absent. Active voices keep their
    // previously snapshotted map (Stage 2 EC-3). Message-thread only.
    //
    // NB (v1.18.0 known limitation): not recorded as a load-op — a deleted cell
    // reappears on project reload (folder re-load / op replay). See CHANGELOG.
    bool removeCell (int midiPitch, int velocityLayer, int technique = 0);

    // v1.18.0: clear an entire velocity layer — erases every cell whose
    // velocityLayer matches, across ALL techniques (matches the existing
    // LoadMode::ReplaceLayer semantics). Recomputes note bounds, resets RR
    // counters for the wiped layer, bumps version, atomic-stores, fires the
    // change callback. Returns the number of cells removed. Message-thread
    // only. Same reload caveat as removeCell.
    int clearVelocityLayer (int velocityLayer);

    // Phase 3.1: snapshot the current sample map as a JSON string for the
    // Stage 3 WebView UI (RESEARCH §RQ3-2 schema). Walks `currentSampleMap`
    // (atomic_load) + `lastSkippedFiles`. Read-only — message thread safe.
    juce::String snapshotSampleMapJson() const;

    // Phase 3.1: snapshot waveform peaks for a single variant as JSON
    // (RESEARCH §RQ3-5). v1.8.0: variantIndex selects which variant; defaults
    // to primary (0) for single-variant cells.
    juce::String snapshotWaveformPeaks (int midiPitch, int velocityLayer,
                                        int targetBins = 512,
                                        int variantIndex = 0,
                                        int technique = 0) const;   // v1.14.0

    // Phase 3.1: editor subscribes via this setter to receive notifications
    // after every atomic-store of `currentSampleMap` (folder load, per-cell
    // replace, loop override). Callback runs on the message thread.
    void setSampleMapChangedCallback (std::function<void()> cb)
    {
        sampleMapChangedCallback = std::move (cb);
    }

    // ------------------------------------------------------------------
    // v1.3.0 — state persistence (project reopen + Save/Load presets)
    // ------------------------------------------------------------------

    // Returns the absolute path of the most recently loaded sample folder,
    // or an empty File if none has been loaded this session. Persisted via
    // getStateInformation so projects reopen with the folder restored.
    juce::File getCurrentSampleFolder() const noexcept { return currentSampleFolder; }

    // Path that setStateInformation tried to restore but no longer exists
    // on disk. Empty when no missing-folder restore is outstanding. Editor
    // queries this on attach (covers cases where the project was loaded
    // before the WebView was ready to receive the folderMissing event).
    juce::String getPendingMissingFolderPath() const noexcept { return pendingMissingFolderPath; }

    // v1.12.0: kind ("filesystem" or "drag-drop") and human-friendly name for
    // the pending missing folder. drag-drop missings have an empty path (the
    // session temp dir is gone and never user-meaningful) but a name lifted
    // from FileSystemEntry::name at drop time.
    juce::String getPendingMissingFolderKind() const noexcept { return pendingMissingFolderKind; }
    juce::String getPendingMissingFolderName() const noexcept { return pendingMissingFolderName; }

    void clearPendingMissingFolder() noexcept
    {
        pendingMissingFolderPath.clear();
        pendingMissingFolderKind.clear();
        pendingMissingFolderName.clear();
    }

    // Editor subscribes to surface a "Locate folder?" modal in the WebView
    // when setStateInformation discovers a saved folder that no longer
    // exists (filesystem kind) or a drag-drop op that was never embedded
    // (drag-drop kind — no path, only displayName).
    //
    // Callback args: (path, kind, name). Fires on the message thread.
    //
    // v1.12.0: signature widened from (path) to (path, kind, name). drag-drop
    // payloads have empty path; filesystem payloads carry the saved absolute
    // path. The webview event payload is constructed by the editor as a
    // {path, kind, name} JSON object so the JS modal can branch UX.
    void setMissingFolderCallback (std::function<void (const juce::String& path,
                                                       const juce::String& kind,
                                                       const juce::String& name)> cb)
    {
        missingFolderCallback = std::move (cb);
    }

    // .omspreset save/load — captures and restores everything that
    // getStateInformation does, but as standalone XML text so users can
    // share preset bundles across projects on the same machine. (Per Q1=A:
    // sample data is referenced by path, not embedded.)
    juce::String capturePresetXml();
    bool restorePresetXml (const juce::String& xmlText);

    // v1.7.1: read the synth's lock-free active-notes bitmask. Used by the
    // editor's 30 Hz timer to diff and emit tuningNoteOn / tuningNoteOff
    // events to the WebView. Message-thread accessor (the load itself is
    // lock-free atomic, so the call is safe from any thread, but the
    // intended caller is the message thread).
    void getActiveNotesAtomic (juce::uint64& low, juce::uint64& high) const noexcept
    {
        synthesiser.getActiveNotes (low, high);
    }

    // v1.7.1: snapshot held notes + their tuned frequencies for the
    // TrueKeys interval display. Walks the bitmask and queries
    // TuningEngine::getFrequency per held note. Cleared output, then
    // appended in MIDI order (low → high). Message-thread only.
    void getHeldNotesData (std::vector<int>& notes,
                           std::vector<double>& freqs);

    // ------------------------------------------------------------------
    // v1.14.0 — Playing Techniques (engine + KS slice)
    // ------------------------------------------------------------------

    // Read-only snapshot of the current technique vocabulary (8 slots). Index
    // 0 = "ord", 1..7 follow the curated default vocabulary. The user can
    // rename slots via setTechniqueName; the names live in the state
    // ValueTree (not APVTS — APVTS strings are clumsy). Message-thread only.
    juce::StringArray getTechniqueNames() const;

    // Mutate slot `index`'s display name. No-op if `index` is out of range
    // or if `name` is empty (callers should default-fill in the UI). Bumps
    // the technique-state callback so the WebView refreshes. Message-thread
    // only.
    void setTechniqueName (int index, const juce::String& name);

    // Reset all 8 technique slots to the curated default names
    // ("ord", "sp", "st", "stacc", "cs", "pizz", "harm", "trem" — the single
    // source is OMtsTechnique::defaultTechniqueVocabulary in
    // TechniqueDefaults.h). Used by the UI's "reset" button and the ctor.
    void resetTechniqueNames();

    // Editor subscribes to receive a notification whenever the technique
    // vocabulary OR the active technique cursor changes (rename, KS toggle,
    // user click, MIDI KS event). Fires on the message thread. Coalesced
    // via AsyncUpdater so a flurry of changes (e.g. user dragging across
    // KS notes) only delivers one final state.
    void setTechniqueStateChangedCallback (std::function<void()> cb)
    {
        techniqueStateChangedCallback = std::move (cb);
    }

    // Read-only — current technique index. Snapshots the same atomic the
    // audio thread reads at startNote; the result is exact.
    int getActiveTechnique() const noexcept
    {
        return pendingTechniqueIndex.load (std::memory_order_acquire);
    }

    // UI / preset / native-fn entry point — set the active technique cursor.
    // Updates the audio-thread atomic AND the APVTS host-facing parameter so
    // automation + state round-trip work. Fires the technique callback.
    // Message-thread only.
    void setActiveTechnique (int technique);

    // ------------------------------------------------------------------
    // v1.23.0 — per-technique / per-(technique,layer) loudness trims (dB)
    // ------------------------------------------------------------------
    //
    // Library-balancing values ("make staccato quieter", "pull down the mf
    // layer of ord"), NOT performance automation — so they live OUTSIDE the
    // APVTS in the processor-owned `cellTrims` TrimTable and round-trip through
    // the state ValueTree like techniqueNames. The audio thread reads them at
    // note-on (RT-safe atomic loads, folded into layer weights / DynLayer
    // gains). All getters/setters are message-thread only.
    //
    // Range is clamped to ±kTrimMaxDb (12 dB); 0 dB = unity. Setters mark the
    // technique-state dirty flag so the editor re-pulls getTechniqueState (which
    // now carries the trims) on the coalesced techniqueStateUpdated event.

    // Per-technique master trim (applies to every layer of `technique`).
    float getTechniqueTrim (int technique) const;
    void  setTechniqueTrim (int technique, float db);

    // Per-(technique, layer) trim (one dynamic layer within one technique).
    float getLayerTrim (int technique, int layer) const;
    void  setLayerTrim (int technique, int layer, float db);

    // Reset every technique + layer trim back to 0 dB.
    void  resetTrims();

    // ------------------------------------------------------------------
    // v1.15.0 — CC + PC trigger mappings
    // ------------------------------------------------------------------

    // Read-only snapshot of the current CC mapping table (8 slots). Each
    // slot defines an inclusive [rangeLow..rangeHigh] sub-band over the
    // 0..127 controller-value range and the technique index that band
    // should select. Message-thread only.
    OMtsTrigger::CcMapping getCcMapping() const;

    // Read-only snapshot of the current PC mapping table (8 slots).
    // Message-thread only.
    OMtsTrigger::PcMapping getPcMapping() const;

    // Replace one CC mapping slot. `slot` is 0..7. Range edges are clamped
    // to 0..127 with `low` ≤ `high` enforced; technique is clamped to 0..7.
    // Performs a copy-on-write atomic_store under persistenceLock so the
    // audio thread observes a consistent table. Fires triggerStateChanged
    // callback. Message-thread only.
    void setCcMappingSlot (int slot, int rangeLow, int rangeHigh, int technique);

    // Replace one PC mapping slot. `slot` is 0..7, pcNumber is clamped to
    // 0..127, technique to 0..7. Same COW + callback semantics as
    // setCcMappingSlot. Message-thread only.
    void setPcMappingSlot (int slot, int pcNumber, int technique);

    // Reset both tables to their plan-defined defaults: CC equally splits
    // 0..127 across the current `technique_count`; PC#i → tech i.
    // Fires the trigger-state callback. Message-thread only.
    void resetTriggerMappings();

    // Editor subscribes to receive notifications whenever the CC/PC
    // mapping tables OR the cc_select_enabled / cc_number / pc_enabled
    // APVTS params change via the native-fn path. Coalesced via
    // AsyncUpdater. Message-thread only.
    void setTriggerStateChangedCallback (std::function<void()> cb)
    {
        triggerStateChangedCallback = std::move (cb);
    }

private:
    juce::AudioProcessorValueTreeState        parameters;

    // v1.17.1 (EF-1): cached raw APVTS parameter pointers for the audio thread.
    // getRawParameterValue returns a std::atomic<float>* that is stable for the
    // lifetime of the APVTS, so the processBlock parameters are resolved ONCE in
    // the constructor (cacheAudioParamPointers) instead of hashing the string IDs
    // on every block. Audio thread reads via ->load(); never written after ctor.
    void cacheAudioParamPointers() noexcept;
    std::atomic<float>* pPolyphony       = nullptr;
    std::atomic<float>* pKsEnabled       = nullptr;
    std::atomic<float>* pKsLowNote       = nullptr;
    std::atomic<float>* pKsHighNote      = nullptr;
    std::atomic<float>* pTechniqueCount  = nullptr;
    std::atomic<float>* pCcSelectEnabled = nullptr;
    std::atomic<float>* pCcNumber        = nullptr;
    std::atomic<float>* pPcEnabled       = nullptr;
    std::atomic<float>* pExpression      = nullptr;
    std::atomic<float>* pDynamicsMode    = nullptr;   // v1.21.0: 0=Velocity, 1=CC Crossfade
    std::atomic<float>* pOutputGain      = nullptr;

    // v1.23.2 (W10): declared BEFORE `synthesiser` so it is destroyed AFTER the
    // voices (members tear down in reverse declaration order) — a voice can only
    // hand off a retired map while the reaper is still alive. Each voice gets a
    // pointer to this in the constructor.
    RetiredMapReaper                          retiredMapReaper;

    CappedSynthesiser                         synthesiser;
    TuningEngine                              tuningEngine;       // D-4: global namespace
    Ouaricon::NoteExpression::VST3Extensions  vst3Extensions;

    // Sample-map storage (atomic-swap target — Stage 2.2 background loader writes here)
    std::shared_ptr<SampleMap>                currentSampleMap;

    // v1.8.0: per-cell round-robin counter array. Indexed by `midi * 4 + layer`.
    // Atomic uint8 per cell so the audio thread can advance counters at
    // startNote without locks. Sentinel 0xFF = "no last variant" (cleared on
    // ReplaceAll). Wired to every voice in the constructor.
    //
    // v1.14.0: index expanded to `midi * 4 * 8 + layer * 8 + technique`.
    // Layout matches MicrotonalSamplerVoice::kRrCounterSize (4096).
    MicrotonalSamplerVoice::RrCounterArray rrCounters;

    // v1.16.9 (LOW-01): resets every RR counter to the 0xFF "no-last"
    // sentinel. RT-safe relaxed atomic stores; safe to call from any thread
    // when no voices are running. Used by ctor, ReplaceAll folder loads,
    // and clearSampleMap.
    void resetAllRrCounters() noexcept;

    // v1.14.0: active technique cursor. Audio thread loads at startNote
    // (memory_order_acquire). KS / CC / PC handlers in processBlock store
    // (memory_order_release). UI clicks store via the message thread.
    std::atomic<int> pendingTechniqueIndex { 0 };

    // v1.14.0: per-slot user-facing names. Lives outside APVTS (strings) and
    // round-trips through state ValueTree only. 8 slots, default vocabulary
    // applied in ctor from OMtsTechnique::defaultTechniqueVocabulary()
    // (TechniqueDefaults.h): "ord", "sp", "st", "stacc", "cs", "pizz", "harm", "trem".
    juce::StringArray techniqueNames;

    // v1.23.0: per-technique master + per-(technique,layer) loudness trims (dB).
    // Atomic scalars the audio thread reads at note-on; written by the message-
    // thread trim setters. Round-trips through the state ValueTree (<CellTrims>),
    // NOT the APVTS. Each voice gets a `const TrimTable*` to this in the ctor.
    TrimTable cellTrims;

    // v1.14.0: editor subscribes for vocab + cursor change notifications.
    // Coalesced via the AsyncUpdater base class (the existing CC11 path
    // already uses handleAsyncUpdate, so we add an in-flight flag).
    std::function<void()> techniqueStateChangedCallback;
    std::atomic<bool>     techniqueStateDirty { false };

    // v1.15.0: CC + PC trigger mapping tables. Copy-on-write via shared_ptr
    // (matches the currentSampleMap pattern). The audio thread reads via
    // std::atomic_load at the top of every processBlock; the message thread
    // builds a new table, swaps it in via atomic_store, and the old table
    // dies when the last audio-thread snapshot releases its reference.
    //
    // Both tables ship with sane defaults seeded by the constructor —
    // resetTriggerMappings() rebuilds them from the current technique_count
    // and PC# = tech# convention.
    std::shared_ptr<OMtsTrigger::CcMapping> currentCcMapping;
    std::shared_ptr<OMtsTrigger::PcMapping> currentPcMapping;

    // v1.15.0: editor subscribes for trigger-table change notifications.
    // Coalesced via the same handleAsyncUpdate the technique-state path
    // already uses.
    std::function<void()> triggerStateChangedCallback;
    std::atomic<bool>     triggerStateDirty { false };

    // v1.8.0: a folder load that surfaced ambiguous (midi, layer) duplicates
    // (no rr/take/tk tokens) is staged here pending user confirmation via
    // confirmRoundRobinLoad. Non-null iff a confirmation modal is in flight.
    std::shared_ptr<SampleMap>                pendingDuplicateMap;
    std::vector<SampleLoader::AmbiguousDuplicate> pendingAmbiguousDuplicates;
    juce::StringArray                         pendingDuplicateSkippedFiles;
    LoadOp                                    pendingDuplicateOp;
    std::function<void()>                     pendingDuplicateChainContinuation;
    std::function<void(const std::vector<SampleLoader::AmbiguousDuplicate>&)>
                                              ambiguousDuplicateCallback;
public:
    // v1.8.0: editor subscribes to surface a confirmation modal when a
    // folder load surfaces ambiguous duplicates. Fires on the message thread
    // with the list of conflicting (midi, layer) groups.
    void setAmbiguousDuplicateCallback (std::function<void(const std::vector<SampleLoader::AmbiguousDuplicate>&)> cb)
    {
        ambiguousDuplicateCallback = std::move (cb);
    }
private:

    // Background sample loader (owns juce::Thread)
    std::unique_ptr<SampleLoader>             sampleLoader;

    // Output gain smoothing (RESEARCH R7, pitfall #8 — 10 ms ramp prevents
    // zipper noise on parameter changes). Initialized in prepareToPlay.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoother;

    // v1.7.0: expression (dynamics) smoothing. Squared curve applied at the
    // smoother target (target = expression²) so the smoother itself ramps the
    // final linear gain — no double-smoothing of the curve. CC 11 writes the
    // raw 0..1 expression parameter; the squaring happens here in processBlock.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> expressionSmoother;

    // Phase 2.2: list of filenames the most recent loader pass could not
    // parse / read. Populated on completion callback (message thread); read
    // by Stage-3 UI. Cleared on failure.
    //
    // v1.12.1 (HG-08): also read by getStateInformation, which Reaper can call
    // off the message thread. Mutations and the read path in
    // captureStateValueTree are guarded by persistenceLock below.
    juce::StringArray lastSkippedFiles;

    // v1.12.1 — CR-01. Last CC 11 (Expression) value seen on the audio thread
    // pending forward to the host via handleAsyncUpdate. -1 = no pending
    // value; 0..127 otherwise. Drained atomically on the message thread.
    std::atomic<int> pendingCC11Value { -1 };

    // v1.23.8: audio-thread-direct dynamics source. processBlock writes the
    // latest CC 11 value (0..1) here BEFORE staging the async host forward;
    // the voices and the Velocity-mode post-mix gain read this atom instead
    // of the "expression" APVTS atom. The APVTS value only updates via
    // handleAsyncUpdate on the message thread — during offline export the
    // render thread outruns the wall-clock-paced AsyncUpdater, whole CC 11
    // ramps coalesce into one late value, and dynamics jumped audibly in the
    // exported audio (Dorico export bug). lastForwardedExpression is
    // audio-thread-only state used to tell our own echoed
    // setValueNotifyingHost write apart from a genuine UI-knob /
    // host-automation move of "expression".
    std::atomic<float> liveExpression { 1.0f };
    float              lastForwardedExpression = 1.0f;

    // v1.12.1 — HG-08. Guards loadOpHistory and lastSkippedFiles across
    // message-thread mutations and the off-thread getStateInformation read.
    // Held only briefly; never acquired from the audio thread.
    mutable juce::CriticalSection persistenceLock;

    // v1.14.0: pre-allocated MIDI buffer used to strip absorbed keyswitch
    // notes from the host's MidiBuffer before forwarding to the
    // Synthesiser. ensureSize() in prepareToPlay reserves room so
    // processBlock's addEvent calls do not allocate. Cleared at the top of
    // every processBlock invocation.
    juce::MidiBuffer ksFilteredBuffer;

    // Phase 3.1: editor-side callback fired on the message thread after every
    // atomic-store of `currentSampleMap`. Editor sets this in its constructor
    // to forward as a `sampleMapUpdated` WebView event.
    std::function<void()> sampleMapChangedCallback;

    // v1.3.0: last folder passed to loadSampleFolder. Tracks the most
    // recent successful load — surfaced to UI for "current folder" displays
    // and used for legacy single-path persistence on save when no v1.6.0
    // op history is present (defensive only; new saves always write the
    // <SampleFolders> op list).
    juce::File currentSampleFolder;

    // v1.6.0: ordered list of successful folder-load operations since the
    // last clearSampleMap() or ReplaceAll load. Persisted by
    // captureStateValueTree as <SampleFolders><Op …/></SampleFolders> and
    // replayed sequentially by setStateInformation so a reopened project
    // reconstructs the same multi-layer sample bank (subject to folders
    // still existing on disk).
    //
    // Append: every successful applyFolderLoad pushes one entry.
    // ReplaceAll: clears the vector before pushing the new single entry.
    // clearSampleMap: clears the vector entirely.
    std::vector<LoadOp> loadOpHistory;

    // v1.6.0: replay queue for setStateInformation. Each entry is dispatched
    // sequentially via kickNextReplayOp() — completion of op N triggers op
    // N+1. Missing folders are skipped (first missing path is captured in
    // pendingMissingFolderPath and surfaced via missingFolderCallback).
    std::vector<LoadOp> pendingReplayOps;
    void kickNextReplayOp();

    // v1.16.10 (MEDIUM-08): "first-missing-only" publish helper. Used by
    // kickNextReplayOp to surface the first missing folder of a replay queue
    // and silently skip subsequent ones (so the user gets a single modal
    // they can resolve, not one per cell). No-op once any of the pending
    // fields is non-empty.
    void publishMissingFolderIfNew (const juce::String& kind,
                                    const juce::String& path,
                                    const juce::String& displayName);

    // v1.12.3 (HG-01) — replay-queue corruption guards.
    //
    // Cascaded callbacks can re-enter the replay path through two distinct
    // doors and each one can corrupt the chain in a different way:
    //
    // 1. Synchronous re-entry into kickNextReplayOp via cascading
    //    sampleMapChangedCallback → editor → loadSampleFolder. The outer kick
    //    is mid-loop (its iterator into pendingReplayOps is live); a re-entry
    //    that pops more ops from under that cursor corrupts the sequence.
    //    `replayKickReentryGuard` rejects the inner call so the outer kick
    //    completes before any inner kick runs.
    //
    // 2. A staged round-robin confirmation produces a chain continuation
    //    (`pendingDuplicateChainContinuation`) that captures `kickNextReplayOp`.
    //    Between staging and user confirmation, an unrelated state restore or
    //    clearSampleMap may rebuild / wipe pendingReplayOps. The deferred
    //    chain would then dispatch ops from a previous generation against a
    //    new queue. Each external mutation of pendingReplayOps bumps
    //    `replayQueueGeneration`; chain continuations capture the generation
    //    at staging time and abort if it has moved on.
    //
    // Both fields are message-thread only; the atomic on the generation just
    // gives us a clean memory model for the (rare) audit-thread reader.
    bool                  replayKickReentryGuard { false };
    std::atomic<uint64_t> replayQueueGeneration  { 0 };

    // v1.6.0: shared completion logic for both user-triggered and replay
    // folder loads. Performs the merge into currentSampleMap, recomputes
    // map metadata, appends to loadOpHistory, and fires
    // sampleMapChangedCallback. Runs on the message thread.
    void applyFolderLoad (std::shared_ptr<SampleMap> newSlotsMap,
                          const juce::StringArray&   skipped,
                          const LoadOp&              op);

    // v1.3.0: setStateInformation parked here when the saved folder no
    // longer exists on disk. Editor reads it on attach (or via callback)
    // to surface a "Locate folder?" modal. Cleared by clearPendingMissingFolder
    // once the user dismisses or relocates.
    juce::String pendingMissingFolderPath;

    // v1.12.0: kind classifies the modal flavour ("filesystem" or "drag-drop").
    // name is a human-friendly folder name surfaced in the modal copy. Both
    // populated alongside pendingMissingFolderPath whenever a replay op is
    // skipped because its source is unreachable.
    juce::String pendingMissingFolderKind;
    juce::String pendingMissingFolderName;

    // v1.3.0: editor surfaces a missing-folder modal via this callback.
    // v1.12.0: signature widened to (path, kind, name).
    std::function<void (const juce::String&,
                        const juce::String&,
                        const juce::String&)> missingFolderCallback;

    // v1.3.0: build a complete root ValueTree (APVTS state + SampleFolder
    // + TuningState children) for save/persist. Used by both
    // getStateInformation and the .omspreset path.
    juce::ValueTree captureStateValueTree();

    // v1.3.0: inverse of captureStateValueTree — restore APVTS, queue async
    // folder reload (or surface missing-folder), restore tuning. Used by
    // both setStateInformation and the .omspreset path.
    void restoreStateValueTree (const juce::ValueTree& root);

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // v1.12.3 (HG-05) — weak-reference master so message-thread callbacks
    // posted by SampleLoader can detect post-destructor invocation. Folder /
    // single-sample loads capture juce::WeakReference<...>(this); each
    // callback null-checks on entry and bails if the processor is gone (e.g.
    // the user closed the project mid-load). The master must be cleared in
    // the destructor before any other member runs down — see ~ctor body.
    JUCE_DECLARE_WEAK_REFERENCEABLE (OMicrotonalSamplerAudioProcessor)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OMicrotonalSamplerAudioProcessor)
};
