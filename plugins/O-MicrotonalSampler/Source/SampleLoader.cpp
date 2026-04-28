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
#include "LoopDetector.h"

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
                               CompletionCallback onComplete,
                               FailureCallback    onFailure)
{
    // Drain any in-flight load defensively before mutating shared state.
    stopThread (500);

    pendingFolder      = folder;
    targetSampleRate   = sr;
    completionCallback = std::move (onComplete);
    failureCallback    = std::move (onFailure);
    skippedFiles.clear();

    startThread();
}

void SampleLoader::cancelLoad()
{
    stopThread (500);
}

void SampleLoader::run()
{
    // Local-only AudioFormatManager (RESEARCH pitfall #9 — never a member).
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

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

        // 1. Parse filename → (midi, velLayer).
        auto parsed = FilenameParser::parse (file.getFileNameWithoutExtension());
        if (! parsed.has_value())
        {
            skippedFiles.add (displayName);
            DBG ("SampleLoader: skipped (unparseable filename) " << displayName);
            continue;
        }

        // 2. Open reader (RESEARCH pitfall #14 — unique_ptr exclusively).
        std::unique_ptr<juce::AudioFormatReader> reader (
            formatManager.createReaderFor (file));
        if (reader == nullptr)
        {
            skippedFiles.add (displayName);
            DBG ("SampleLoader: skipped (unreadable file) " << displayName);
            continue;
        }

        const int    srcChannels = (int) reader->numChannels;
        const int    srcSamples  = (int) reader->lengthInSamples;
        const double srcSR       = reader->sampleRate;

        if (srcChannels <= 0 || srcSamples <= 0 || srcSR <= 0.0)
        {
            skippedFiles.add (displayName);
            DBG ("SampleLoader: skipped (invalid header) " << displayName);
            continue;
        }

        // 3. Allocate temp buffer (loader thread; not RT-critical) and read.
        juce::AudioBuffer<float> sourceBuf (srcChannels, srcSamples);
        if (! reader->read (&sourceBuf, 0, srcSamples, 0, true, true))
        {
            skippedFiles.add (displayName);
            DBG ("SampleLoader: skipped (read failure) " << displayName);
            continue;
        }

        // 4. SR conversion (D2-9, RQ-6). srcRatio = srcSR / targetSR.
        //    > 1.0 → source is faster than target → output has fewer samples.
        const bool needsResample = std::abs (srcSR - targetSampleRate) > 0.1;
        juce::AudioBuffer<float> workBuf;
        int outNumSamples = srcSamples;

        if (needsResample)
        {
            const double srcRatio = srcSR / targetSampleRate;
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

            if (srcRatio >= 0.25 && srcRatio <= 4.0)
            {
                DBG ("SampleLoader: INFO resampled " << displayName
                     << " from " << srcSR << " Hz to " << targetSampleRate
                     << " Hz (ratio " << srcRatio << ")");
            }
            else
            {
                DBG ("SampleLoader: WARN extreme SR ratio for " << displayName
                     << " (src=" << srcSR << " Hz, tgt=" << targetSampleRate
                     << " Hz, ratio=" << srcRatio
                     << ") — quality may suffer");
            }
        }
        else
        {
            // No resample — move source into work buffer.
            workBuf = std::move (sourceBuf);
        }

        // 5. Mono → stereo promotion (D2-10). All slots stored as stereo.
        // Phase 3.1: audio is now held via shared_ptr<AudioBuffer<float>> so
        // future SampleMap deep-copies (per-cell replace, loop override) only
        // copy a pointer per slot, not the audio bytes (RQ3-3).
        SampleSlot slot;
        slot.audio = std::make_shared<juce::AudioBuffer<float>> (2, outNumSamples);
        slot.audio->clear();

        if (srcChannels == 1)
        {
            // Duplicate mono into both channels at unity gain.
            slot.audio->copyFrom (0, 0, workBuf, 0, 0, outNumSamples);
            slot.audio->copyFrom (1, 0, workBuf, 0, 0, outNumSamples);
        }
        else
        {
            // Stereo+ → take first two channels.
            slot.audio->copyFrom (0, 0, workBuf, 0, 0, outNumSamples);
            slot.audio->copyFrom (1, 0, workBuf, 1, 0, outNumSamples);
        }

        slot.sourceSampleRate = targetSampleRate;   // host-SR after resample
        slot.midiNote         = parsed->midiNote;
        slot.velocityLayer    = parsed->velLayer;
        slot.filename         = displayName;        // Phase 3.1: basename for UI

        // 6. Phase 2.5: detect sustain loop region. Invalid → one-shot fallback.
        const auto region = LoopDetector::detectLoop (*slot.audio, targetSampleRate);
        if (region.valid)
        {
            slot.loopStart = region.loopStart;
            slot.loopEnd   = region.loopEnd;
            slot.loopMode  = LoopMode::Auto;        // Phase 3.1
            DBG ("SampleLoader: loop detected for " << displayName
                 << " [" << region.loopStart << ", " << region.loopEnd
                 << "] (" << (region.loopEnd - region.loopStart) << " samples)");
        }
        else
        {
            slot.loopStart = 0;
            slot.loopEnd   = 0;                     // one-shot fallback (EC-7)
            slot.loopMode  = LoopMode::OneShot;     // Phase 3.1
            DBG ("SampleLoader: no loop region for " << displayName << " (one-shot)");
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
