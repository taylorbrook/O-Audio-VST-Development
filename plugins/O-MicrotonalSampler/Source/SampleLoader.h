/*
  ==============================================================================

    SampleLoader.h
    Microtonal Sample Engine - Background sample loader (Phase 2.2)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.2: full implementation. loadFolder spawns the worker thread, run()
    enumerates the folder, parses filenames via FilenameParser, loads each
    via juce::AudioFormatReader, SR-converts via juce::LagrangeInterpolator,
    duplicates mono → stereo (D2-10), assembles a SampleMap, and dispatches
    the completion callback on the message thread (RESEARCH pitfall #12).

    AudioFormatManager is intentionally NOT a member — must only be constructed
    inside run() (RESEARCH pitfall #9).

    Completion-callback signature carries a juce::StringArray of files that
    failed to parse / load — Stage 3 UI surfaces this; Phase 2.2 stores it.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <functional>
#include <memory>
#include "SampleMap.h"

class SampleLoader : public juce::Thread
{
public:
    using CompletionCallback = std::function<void(std::shared_ptr<SampleMap>,
                                                  juce::StringArray skippedFiles)>;
    using FailureCallback    = std::function<void(const juce::String&)>;

    // Phase 3.2 — per-cell load completion. `slot.midiNote == -1` signals
    // failure; `skipReason` is human-readable when non-empty.
    using SingleSlotCallback = std::function<void(SampleSlot, juce::String /*skipReason*/)>;

    SampleLoader();
    ~SampleLoader() override;

    // v1.6.0: per-load options control whether filename velocity tokens are
    // honoured or forced to a caller-supplied target layer. See
    // SampleMap.h::LoadOptions for field semantics. Default-constructed
    // options ({targetLayer=0, overrideTokens=false}) reproduce v1.5.x
    // behaviour exactly — filename token wins, targetLayer ignored.
    void loadFolder (const juce::File& folder,
                     double             targetSampleRate,
                     LoadOptions        options,
                     CompletionCallback onComplete,
                     FailureCallback    onFailure = nullptr);

    // Phase 3.2 — single-slot async load (mirror of run()'s per-file pipeline:
    // open reader → SR-convert via LagrangeInterpolator → mono→stereo → loop
    // detect → assemble SampleSlot). On error, dispatches completion with an
    // empty slot (midiNote = -1) and a non-empty `skipReason`. Always invokes
    // the completion on the message thread via juce::MessageManager::callAsync.
    void loadSingleSlot (const juce::File&    file,
                         int                  midiPitch,
                         int                  velocityLayer,
                         double               targetSampleRate,
                         SingleSlotCallback   onComplete);

    void cancelLoad();

private:
    void run() override;

    // Mode discriminator — `run()` switches on this to decide between
    // folder enumeration and single-slot processing.
    enum class Mode { Folder, SingleSlot };
    Mode               mode                = Mode::Folder;

    juce::File         pendingFolder;
    double             targetSampleRate    = 48000.0;
    LoadOptions        folderOptions       {};   // v1.6.0 — see SampleMap.h
    CompletionCallback completionCallback;
    FailureCallback    failureCallback;
    juce::StringArray  skippedFiles;     // touched only by run() then captured
                                         // by message-thread callback

    // Phase 3.2 single-slot state — touched only between loadSingleSlot()
    // and the run() worker; the worker captures + dispatches via callAsync.
    juce::File           singleFile;
    int                  singleMidi          = 0;
    int                  singleVelLayer      = 0;
    SingleSlotCallback   singleSlotCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLoader)
};
