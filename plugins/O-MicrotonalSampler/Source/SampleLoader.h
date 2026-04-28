/*
  ==============================================================================

    SampleLoader.h
    Microtonal Sample Engine - Background sample loader (Stage 1 skeleton)
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1: API surface frozen. loadFolder dispatches a failure callback
    via MessageManager::callAsync; no startThread call, no I/O. run() is empty.
    Stage 2.2 fills in: AudioFormatManager construction inside run(), filename
    parsing, AudioBuffer load, SampleMap build, completion callback.

    AudioFormatManager is intentionally NOT a member — must only be constructed
    inside run() (RESEARCH.md pitfall #9).

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
    using CompletionCallback = std::function<void(std::shared_ptr<SampleMap>)>;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLoader)
};
