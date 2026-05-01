/*
  ==============================================================================

    O-MicrotonalSampler - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): silent shell. APVTS + headless TuningEngine + NE drain
    + sample-map shared_ptr surface + SampleLoader skeleton. First audio: Phase 2.1.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include "MicrotonalSamplerSound.h"
#include "MicrotonalSamplerVoice.h"
#include "SampleMap.h"
#include "SampleLoader.h"
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
    juce::String path;            // Absolute folder path (UTF-8).
    int          targetLayer    = 0;     // 0..3.
    LoadMode     mode           = LoadMode::ReplaceAll;
    bool         overrideTokens = false; // true = ignore filename velocity tokens.
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

class OMicrotonalSamplerAudioProcessor : public juce::AudioProcessor
{
public:
    OMicrotonalSamplerAudioProcessor();
    ~OMicrotonalSamplerAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-MicrotonalSampler"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }  // Stage 1: silent stub

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
                           bool mergeAsRr = false);

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
                             int variantIndex = -1);

    // Phase 3.4: convenience wrapper — calls overrideLoopPoints with the
    // resetToAutoDetect flag set. variantIndex defaults to primary (0).
    void resetLoopToAutoDetect (int midiPitch, int velocityLayer,
                                int variantIndex = -1);

    // Phase 3.1: snapshot the current sample map as a JSON string for the
    // Stage 3 WebView UI (RESEARCH §RQ3-2 schema). Walks `currentSampleMap`
    // (atomic_load) + `lastSkippedFiles`. Read-only — message thread safe.
    juce::String snapshotSampleMapJson() const;

    // Phase 3.1: snapshot waveform peaks for a single variant as JSON
    // (RESEARCH §RQ3-5). v1.8.0: variantIndex selects which variant; defaults
    // to primary (0) for single-variant cells.
    juce::String snapshotWaveformPeaks (int midiPitch, int velocityLayer,
                                        int targetBins = 512,
                                        int variantIndex = 0) const;

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
    void clearPendingMissingFolder() noexcept { pendingMissingFolderPath.clear(); }

    // Editor subscribes to surface a "Locate folder?" modal in the WebView
    // when setStateInformation discovers a saved folder that no longer
    // exists. Callback fires on the message thread; payload is the saved
    // absolute path.
    void setMissingFolderCallback (std::function<void(const juce::String&)> cb)
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

private:
    juce::AudioProcessorValueTreeState        parameters;
    CappedSynthesiser                         synthesiser;
    TuningEngine                              tuningEngine;       // D-4: global namespace
    Ouaricon::NoteExpression::VST3Extensions  vst3Extensions;

    // Sample-map storage (atomic-swap target — Stage 2.2 background loader writes here)
    std::shared_ptr<SampleMap>                currentSampleMap;

    // v1.8.0: per-cell round-robin counter array. Indexed by `midi * 4 + layer`.
    // Atomic uint8 per cell so the audio thread can advance counters at
    // startNote without locks. Sentinel 0xFF = "no last variant" (cleared on
    // ReplaceAll). Wired to every voice in the constructor.
    MicrotonalSamplerVoice::RrCounterArray rrCounters;

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
    juce::StringArray lastSkippedFiles;

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

    // v1.3.0: editor surfaces a missing-folder modal via this callback.
    // Fires on the message thread; payload is the saved absolute path.
    std::function<void(const juce::String&)> missingFolderCallback;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OMicrotonalSamplerAudioProcessor)
};
