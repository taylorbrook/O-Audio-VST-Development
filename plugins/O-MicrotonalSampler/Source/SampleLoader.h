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

    SampleLoader();
    ~SampleLoader() override;

    void loadFolder (const juce::File& folder,
                     double             targetSampleRate,
                     CompletionCallback onComplete,
                     FailureCallback    onFailure = nullptr);

    void cancelLoad();

private:
    void run() override;

    juce::File         pendingFolder;
    double             targetSampleRate    = 48000.0;
    CompletionCallback completionCallback;
    FailureCallback    failureCallback;
    juce::StringArray  skippedFiles;     // touched only by run() then captured
                                         // by message-thread callback

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLoader)
};
