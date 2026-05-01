/*
  ==============================================================================

    SampleLoader.h
    Microtonal Sample Engine - Background sample loader
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

    v1.8.0 — Round-Robin Samples:
      - The loader now groups variants by (midiNote, velocityLayer). Groups
        with explicit rr/take/tk tokens flow silently into a multi-variant
        SampleCell. Groups without explicit RR tokens but with > 1 file
        surface as `AmbiguousDuplicate` entries in the completion payload —
        the processor stages the SampleMap and emits a modal-confirm event
        to the WebView. User decision is forwarded via
        OMicrotonalSamplerAudioProcessor::confirmRoundRobinLoad.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <functional>
#include <memory>
#include <vector>
#include "SampleMap.h"

class SampleLoader : public juce::Thread
{
public:
    // v1.8.0: a (midiNote, velocityLayer) cell where the loader detected > 1
    // file but NO explicit rr/take/tk tokens. The map is built with all
    // variants included, but the processor must surface a confirmation modal
    // before atomic-storing it.
    struct AmbiguousDuplicate
    {
        int                midiNote      = -1;
        int                velocityLayer = 0;
        juce::StringArray  filenames;
    };

    using CompletionCallback = std::function<void(std::shared_ptr<SampleMap>,
                                                  juce::StringArray skippedFiles,
                                                  std::vector<AmbiguousDuplicate>)>;
    using FailureCallback    = std::function<void(const juce::String&)>;

    // Per-cell single-load completion. Empty `skipReason` = success; non-empty
    // = failure (variant.audio will be null). Single-variant always.
    using SingleVariantCallback = std::function<void(int midiNote,
                                                     int velocityLayer,
                                                     SampleVariant variant,
                                                     juce::String skipReason)>;

    SampleLoader();
    ~SampleLoader() override;

    void loadFolder (const juce::File& folder,
                     double             targetSampleRate,
                     LoadOptions        options,
                     CompletionCallback onComplete,
                     FailureCallback    onFailure = nullptr);

    void loadSingleVariant (const juce::File&     file,
                            int                   midiPitch,
                            int                   velocityLayer,
                            double                targetSampleRate,
                            SingleVariantCallback onComplete);

    void cancelLoad();

private:
    void run() override;

    enum class Mode { Folder, SingleVariant };
    Mode               mode                = Mode::Folder;

    juce::File         pendingFolder;
    double             targetSampleRate    = 48000.0;
    LoadOptions        folderOptions       {};
    CompletionCallback completionCallback;
    FailureCallback    failureCallback;
    juce::StringArray  skippedFiles;

    juce::File              singleFile;
    int                     singleMidi      = 0;
    int                     singleVelLayer  = 0;
    SingleVariantCallback   singleVariantCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLoader)
};
