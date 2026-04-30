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

        juce::Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
    }

private:
    std::atomic<int> voiceCap { 16 };
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

    // Phase 2.2: drag-drop folder load entry point (called from PluginEditor).
    // Spawns the background SampleLoader; on completion (message thread) the
    // new SampleMap is atomic-stored into currentSampleMap and any skipped
    // files are recorded in lastSkippedFiles for Stage-3 UI surfacing.
    void loadSampleFolder (const juce::File& folder);

    // Read-only accessor for Stage-3 UI: list of files the loader skipped
    // (unparseable filenames, unreadable files, etc.). Refreshed on each
    // loadSampleFolder completion; cleared on failure.
    const juce::StringArray& getLastSkippedFiles() const noexcept { return lastSkippedFiles; }

    // Phase 3.1: per-cell load (full implementation in 3.2). Skeleton logs
    // and returns. UI calls this when the user single-clicks an empty cell or
    // double-clicks a loaded cell to replace.
    void loadSingleSample (int midiPitch, int velocityLayer, const juce::File& file);

    // v1.0.2: destructive — replaces currentSampleMap with a fresh empty
    // SampleMap (version bumped). Active voices keep their previously
    // snapshotted map alive transitively until release (Stage 2 EC-3); new
    // note-ons after the clear find an empty map and produce silence. Caller
    // (UI) is responsible for confirmation dialog. Message-thread only.
    void clearSampleMap();

    // Phase 3.4: loop-point override (full implementation). Atomically
    // deep-copies the current SampleMap, mutates the (midi, vel) slot's
    // loop fields, bumps version, atomic-stores, fires callback.
    // resetToAutoDetect=true re-runs LoopDetector and overrides loopMode.
    // crossfadeLen is recorded for v1.1 (per RP3-2 — global xfade in v1.0).
    void overrideLoopPoints (int midiPitch, int velocityLayer,
                             int loopStart, int loopEnd,
                             int crossfadeLen,
                             bool resetToAutoDetect = false);

    // Phase 3.4: convenience wrapper — calls overrideLoopPoints with the
    // resetToAutoDetect flag set. Re-runs LoopDetector::detectLoop on the
    // slot's audio; valid → Auto, invalid → OneShot.
    void resetLoopToAutoDetect (int midiPitch, int velocityLayer);

    // Phase 3.1: snapshot the current sample map as a JSON string for the
    // Stage 3 WebView UI (RESEARCH §RQ3-2 schema). Walks `currentSampleMap`
    // (atomic_load) + `lastSkippedFiles`. Read-only — message thread safe.
    juce::String snapshotSampleMapJson() const;

    // Phase 3.1: snapshot waveform peaks for a single slot as JSON (RESEARCH
    // §RQ3-5). Skeleton in 3.1 — full impl in 3.4. Returns empty JSON {} for
    // now so JS callers don't crash.
    juce::String snapshotWaveformPeaks (int midiPitch, int velocityLayer,
                                        int targetBins = 512) const;

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

private:
    juce::AudioProcessorValueTreeState        parameters;
    CappedSynthesiser                         synthesiser;
    TuningEngine                              tuningEngine;       // D-4: global namespace
    Ouaricon::NoteExpression::VST3Extensions  vst3Extensions;

    // Sample-map storage (atomic-swap target — Stage 2.2 background loader writes here)
    std::shared_ptr<SampleMap>                currentSampleMap;

    // Background sample loader (owns juce::Thread)
    std::unique_ptr<SampleLoader>             sampleLoader;

    // Output gain smoothing (RESEARCH R7, pitfall #8 — 10 ms ramp prevents
    // zipper noise on parameter changes). Initialized in prepareToPlay.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoother;

    // Phase 2.2: list of filenames the most recent loader pass could not
    // parse / read. Populated on completion callback (message thread); read
    // by Stage-3 UI. Cleared on failure.
    juce::StringArray lastSkippedFiles;

    // Phase 3.1: editor-side callback fired on the message thread after every
    // atomic-store of `currentSampleMap`. Editor sets this in its constructor
    // to forward as a `sampleMapUpdated` WebView event.
    std::function<void()> sampleMapChangedCallback;

    // v1.3.0: last folder passed to loadSampleFolder. Persisted across DAW
    // sessions (via getStateInformation) so reopening a project restores
    // the same sample bank.
    juce::File currentSampleFolder;

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
