/*
  ==============================================================================

    SampleLoader.cpp
    Microtonal Sample Engine - Background sample loader (Phase 2.2)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "SampleLoader.h"
#include "FilenameParser.h"

#include <cmath>
#include <utility>
#include <vector>

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
                               LoadOptions        options,
                               CompletionCallback onComplete,
                               FailureCallback    onFailure)
{
    // Drain any in-flight load defensively before mutating shared state.
    stopThread (500);

    mode               = Mode::Folder;
    pendingFolder      = folder;
    targetSampleRate   = sr;
    folderOptions      = options;
    completionCallback = std::move (onComplete);
    failureCallback    = std::move (onFailure);
    skippedFiles.clear();
    singleSlotCallback = nullptr;

    startThread();
}

void SampleLoader::loadSingleSlot (const juce::File&    file,
                                   int                  midiPitch,
                                   int                  velocityLayer,
                                   double               sr,
                                   SingleSlotCallback   onComplete)
{
    // Drain any in-flight load defensively before mutating shared state.
    stopThread (500);

    mode               = Mode::SingleSlot;
    singleFile         = file;
    singleMidi         = midiPitch;
    singleVelLayer     = velocityLayer;
    targetSampleRate   = sr;
    singleSlotCallback = std::move (onComplete);

    // Folder-mode state cleared so a stale callback can't fire.
    completionCallback = nullptr;
    failureCallback    = nullptr;
    skippedFiles.clear();

    startThread();
}

void SampleLoader::cancelLoad()
{
    stopThread (500);
}

// ----------------------------------------------------------------------
// Per-file processing helper. Extracted from run() so the single-slot path
// shares the same SR-convert + mono→stereo + loop-detect pipeline. Returns
// true on success (slot populated); false on parse/read failure (skipReason
// populated). midiPitch/velocityLayer are explicit overrides — the folder
// path supplies them from the FilenameParser; the single-slot path supplies
// the user-provided cell coordinates.
//
// Local-only juce::AudioFormatManager (RESEARCH pitfall #9 — never a member).
// Caller is responsible for cancellation polling.
// ----------------------------------------------------------------------
namespace
{
    bool processOneFile (const juce::File& file,
                         int               midiPitch,
                         int               velocityLayer,
                         double            targetSR,
                         juce::AudioFormatManager& formatManager,
                         SampleSlot&       outSlot,
                         juce::String&     outSkipReason)
    {
        const juce::String displayName = file.getFileName();

        if (! file.existsAsFile())
        {
            outSkipReason = "file does not exist: " + displayName;
            return false;
        }

        std::unique_ptr<juce::AudioFormatReader> reader (
            formatManager.createReaderFor (file));
        if (reader == nullptr)
        {
            outSkipReason = "unreadable file: " + displayName;
            return false;
        }

        const int    srcChannels = (int) reader->numChannels;
        const int    srcSamples  = (int) reader->lengthInSamples;
        const double srcSR       = reader->sampleRate;

        if (srcChannels <= 0 || srcSamples <= 0 || srcSR <= 0.0)
        {
            outSkipReason = "invalid header: " + displayName;
            return false;
        }

        // Allocate temp buffer (loader thread; not RT-critical) and read.
        juce::AudioBuffer<float> sourceBuf (srcChannels, srcSamples);
        if (! reader->read (&sourceBuf, 0, srcSamples, 0, true, true))
        {
            outSkipReason = "read failure: " + displayName;
            return false;
        }

        // SR conversion (D2-9, RQ-6). srcRatio = srcSR / targetSR.
        // > 1.0 → source is faster than target → output has fewer samples.
        const bool needsResample = std::abs (srcSR - targetSR) > 0.1;
        juce::AudioBuffer<float> workBuf;
        int outNumSamples = srcSamples;

        if (needsResample)
        {
            const double srcRatio = srcSR / targetSR;
            outNumSamples = (int) std::ceil ((double) srcSamples / srcRatio);
            if (outNumSamples < 1)
                outNumSamples = 1;

            workBuf.setSize (srcChannels, outNumSamples);
            workBuf.clear();

            // Per-channel separate LagrangeInterpolator (RESEARCH pitfall #10).
            for (int ch = 0; ch < srcChannels; ++ch)
            {
                juce::LagrangeInterpolator interp;
                interp.reset();
                interp.process (srcRatio,
                                sourceBuf.getReadPointer (ch),
                                workBuf.getWritePointer (ch),
                                outNumSamples);
            }

            DBG ("SampleLoader: resampled " << displayName
                 << " from " << srcSR << " Hz to " << targetSR
                 << " Hz (ratio " << srcRatio << ")");
        }
        else
        {
            workBuf = std::move (sourceBuf);
        }

        // Mono → stereo promotion (D2-10).
        outSlot.audio = std::make_shared<juce::AudioBuffer<float>> (2, outNumSamples);
        outSlot.audio->clear();

        if (srcChannels == 1)
        {
            outSlot.audio->copyFrom (0, 0, workBuf, 0, 0, outNumSamples);
            outSlot.audio->copyFrom (1, 0, workBuf, 0, 0, outNumSamples);
        }
        else
        {
            outSlot.audio->copyFrom (0, 0, workBuf, 0, 0, outNumSamples);
            outSlot.audio->copyFrom (1, 0, workBuf, 1, 0, outNumSamples);
        }

        outSlot.sourceSampleRate = targetSR;     // host-SR after resample
        outSlot.midiNote         = midiPitch;
        outSlot.velocityLayer    = velocityLayer;
        outSlot.filename         = displayName;

        // v1.4.0: default to whole-file loop. loopEnd = N - 2 leaves headroom
        // for the cubic 4-tap interpolator context (renderer reads up to
        // pos + 2). The 8-sample equal-power crossfade at the wrap handles
        // click prevention. Closes V11-LOOP-FALLBACK.
        const int N = outSlot.audio->getNumSamples();
        if (N >= 18)   // need >= kMinLoopLength (16) + 2 headroom
        {
            outSlot.loopStart = 0;
            outSlot.loopEnd   = N - 2;
            outSlot.loopMode  = LoopMode::Auto;
        }
        else
        {
            outSlot.loopStart = 0;
            outSlot.loopEnd   = 0;
            outSlot.loopMode  = LoopMode::OneShot;
        }

        outSkipReason.clear();
        return true;
    }
}

void SampleLoader::run()
{
    // Local-only AudioFormatManager (RESEARCH pitfall #9 — never a member).
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    // ------------------------------------------------------------------
    // Phase 3.2: single-slot mode — process one file with explicit
    // (midiPitch, velocityLayer) provided by the caller. Skip the
    // FilenameParser path entirely.
    // ------------------------------------------------------------------
    if (mode == Mode::SingleSlot)
    {
        SampleSlot slot;
        juce::String skipReason;

        if (! processOneFile (singleFile,
                              singleMidi,
                              singleVelLayer,
                              targetSampleRate,
                              formatManager,
                              slot,
                              skipReason))
        {
            // Failure — empty slot (midiNote == -1 by default ctor) + reason.
            SampleSlot empty;
            auto cb     = singleSlotCallback;
            auto reason = skipReason;
            juce::MessageManager::callAsync ([cb, empty, reason]()
            {
                if (cb) cb (empty, reason);
            });
            return;
        }

        if (threadShouldExit())
            return;

        // Success — slot populated. Empty skipReason signals success.
        auto cb       = singleSlotCallback;
        auto loadedSlot = std::move (slot);
        juce::MessageManager::callAsync ([cb, loadedSlot]() mutable
        {
            if (cb) cb (std::move (loadedSlot), {});
        });
        return;
    }

    // ------------------------------------------------------------------
    // Folder mode (Phase 2.2 path — unchanged behaviour, refactored to
    // share the per-file pipeline with single-slot mode).
    // ------------------------------------------------------------------
    if (! pendingFolder.isDirectory())
    {
        auto fcb = failureCallback;
        const juce::String msg = "SampleLoader: \"" + pendingFolder.getFullPathName()
                                  + "\" is not a directory";
        juce::MessageManager::callAsync ([fcb, msg]()
        {
            if (fcb) fcb (msg);
        });
        return;
    }

    std::vector<SampleSlot> builtSlots;
    builtSlots.reserve (128);

    const juce::String wildcards = "*.wav;*.aif;*.aiff;*.flac";

    for (const auto& entry : juce::RangedDirectoryIterator (pendingFolder,
                                                            /*recursive*/ false,
                                                            wildcards,
                                                            juce::File::findFiles))
    {
        if (threadShouldExit())   // RESEARCH pitfall #11 — cancellation point
            return;

        const juce::File file = entry.getFile();
        const juce::String displayName = file.getFileName();

        // 1. Parse filename → (midi, velLayer). Folder-mode only.
        auto parsed = FilenameParser::parse (file.getFileNameWithoutExtension());
        if (! parsed.has_value())
        {
            skippedFiles.add (displayName);
            DBG ("SampleLoader: skipped (unparseable filename) " << displayName);
            continue;
        }

        // v1.6.0: with overrideTokens, the parser's velLayer is discarded
        // and every slot lands on the caller-supplied targetLayer. Without
        // override, the filename token wins (legacy v1.5.x behaviour).
        const int effectiveVelLayer = folderOptions.overrideTokens
            ? juce::jlimit (0, 3, folderOptions.targetLayer)
            : parsed->velLayer;

        // 2. Run the shared per-file pipeline (open → SR-convert → mono→stereo
        //    → loop-detect). Failures are recorded in skippedFiles.
        SampleSlot slot;
        juce::String skipReason;
        if (! processOneFile (file,
                              parsed->midiNote,
                              effectiveVelLayer,
                              targetSampleRate,
                              formatManager,
                              slot,
                              skipReason))
        {
            skippedFiles.add (displayName);
            DBG ("SampleLoader: skipped (" << skipReason << ") " << displayName);
            continue;
        }

        builtSlots.push_back (std::move (slot));
    }

    if (threadShouldExit())
        return;

    // No usable samples → report failure rather than empty completion.
    if (builtSlots.empty())
    {
        auto fcb = failureCallback;
        const int  skipCount = skippedFiles.size();
        const juce::String msg = "SampleLoader: no usable samples in \""
            + pendingFolder.getFullPathName()
            + "\" (skipped " + juce::String (skipCount) + " file(s))";
        juce::MessageManager::callAsync ([fcb, msg]()
        {
            if (fcb) fcb (msg);
        });
        return;
    }

    // Derive map metadata.
    SampleMap built;
    built.lowestNote        = 127;
    built.highestNote       = 0;
    int maxLayer            = 0;

    for (const auto& s : builtSlots)
    {
        built.lowestNote  = juce::jmin (built.lowestNote,  s.midiNote);
        built.highestNote = juce::jmax (built.highestNote, s.midiNote);
        maxLayer          = juce::jmax (maxLayer,          s.velocityLayer);
    }
    built.numVelocityLayers = juce::jlimit (1, 4, maxLayer + 1);
    built.slots             = std::move (builtSlots);

    // Wrap in shared_ptr — std::make_shared exactly once at the end (PLAN spec).
    auto map = std::make_shared<SampleMap> (std::move (built));

    // Dispatch via message thread (RESEARCH pitfall #12).
    auto cb      = completionCallback;
    auto skipped = skippedFiles;
    juce::MessageManager::callAsync ([cb, map, skipped]()
    {
        if (cb)
            cb (map, skipped);
    });
}
