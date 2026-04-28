/*
  ==============================================================================

    SampleLoader.cpp
    Microtonal Sample Engine - Background sample loader (Stage 1 skeleton)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "SampleLoader.h"

SampleLoader::SampleLoader()
    : juce::Thread ("SampleLoader")
{
}

SampleLoader::~SampleLoader()
{
    stopThread (2000);
}

void SampleLoader::loadFolder (const juce::File& folder,
                               double             sr,
                               CompletionCallback onComplete,
                               FailureCallback    onFailure)
{
    // Drain any previous run defensively (run() is empty at Stage 1, so this is
    // a no-op fast-path; preserved for Stage 2.2 forward-compat).
    stopThread (500);

    pendingFolder      = folder;
    targetSampleRate   = sr;
    completionCallback = std::move (onComplete);
    failureCallback    = std::move (onFailure);

    // Stage 1 explicit stub: dispatch failure callback to message thread so
    // callers observe a deterministic "no map produced" outcome rather than a
    // silent no-op. Do NOT call startThread() — keeps the failure path
    // deterministic and avoids a brief background thread that does nothing.
    auto onFailureCopy = failureCallback;
    juce::MessageManager::callAsync ([onFailureCopy]()
    {
        if (onFailureCopy)
            onFailureCopy ("SampleLoader stub - Stage 1; sample loading lands in Stage 2.2");
    });
}

void SampleLoader::cancelLoad()
{
    stopThread (500);
}

void SampleLoader::run()
{
    // Stage 1: empty. Stage 2.2 fills in:
    //   - juce::AudioFormatManager mgr; mgr.registerBasicFormats();
    //   - scan pendingFolder for .wav / .aif files
    //   - parse "<note>_v<layer>" filenames
    //   - load each into juce::AudioBuffer<float>
    //   - resample to targetSampleRate via juce::LagrangeInterpolator
    //   - build SampleMap, dispatch completionCallback via MessageManager::callAsync
}
