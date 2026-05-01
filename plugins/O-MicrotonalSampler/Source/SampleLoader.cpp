/*
  ==============================================================================

    SampleLoader.cpp
    Microtonal Sample Engine - Background sample loader
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "SampleLoader.h"
#include "FilenameParser.h"

#include <algorithm>
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
    stopThread (500);

    mode               = Mode::Folder;
    pendingFolder      = folder;
    targetSampleRate   = sr;
    folderOptions      = options;
    completionCallback = std::move (onComplete);
    failureCallback    = std::move (onFailure);
    skippedFiles.clear();
    singleVariantCallback = nullptr;

    startThread();
}

void SampleLoader::loadSingleVariant (const juce::File&     file,
                                      int                   midiPitch,
                                      int                   velocityLayer,
                                      double                sr,
                                      SingleVariantCallback onComplete)
{
    stopThread (500);

    mode                  = Mode::SingleVariant;
    singleFile            = file;
    singleMidi            = midiPitch;
    singleVelLayer        = velocityLayer;
    targetSampleRate      = sr;
    singleVariantCallback = std::move (onComplete);

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
// Per-file processing helper. Loads a file from disk, SR-converts, mono→stereo
// promotes, and populates a SampleVariant (audio + filename + loop fields).
// midiPitch/velocityLayer/rrIndex are resolved by the caller — this helper
// produces the variant only; cell-level addressing is the caller's concern.
// ----------------------------------------------------------------------
namespace
{
    bool processOneFile (const juce::File& file,
                         double            targetSR,
                         juce::AudioFormatManager& formatManager,
                         SampleVariant&    outVariant,
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

        juce::AudioBuffer<float> sourceBuf (srcChannels, srcSamples);
        if (! reader->read (&sourceBuf, 0, srcSamples, 0, true, true))
        {
            outSkipReason = "read failure: " + displayName;
            return false;
        }

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

        outVariant.audio = std::make_shared<juce::AudioBuffer<float>> (2, outNumSamples);
        outVariant.audio->clear();

        if (srcChannels == 1)
        {
            outVariant.audio->copyFrom (0, 0, workBuf, 0, 0, outNumSamples);
            outVariant.audio->copyFrom (1, 0, workBuf, 0, 0, outNumSamples);
        }
        else
        {
            outVariant.audio->copyFrom (0, 0, workBuf, 0, 0, outNumSamples);
            outVariant.audio->copyFrom (1, 0, workBuf, 1, 0, outNumSamples);
        }

        outVariant.sourceSampleRate = targetSR;
        outVariant.filename         = displayName;

        const int N = outVariant.audio->getNumSamples();
        if (N >= 18)
        {
            outVariant.loopStart = 0;
            outVariant.loopEnd   = N - 2;
            outVariant.loopMode  = LoopMode::Auto;
        }
        else
        {
            outVariant.loopStart = 0;
            outVariant.loopEnd   = 0;
            outVariant.loopMode  = LoopMode::OneShot;
        }

        outSkipReason.clear();
        return true;
    }

    // v1.8.0: per-file scratch struct used during folder enumeration. Holds
    // the variant payload + addressing key + RR token (or -1 sentinel).
    struct LoadedFile
    {
        int           midiNote      = -1;
        int           velocityLayer = 0;
        int           rrIndex       = -1;
        SampleVariant variant;
    };
}

void SampleLoader::run()
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    // ------------------------------------------------------------------
    // Single-variant mode (per-cell load)
    // ------------------------------------------------------------------
    if (mode == Mode::SingleVariant)
    {
        SampleVariant variant;
        juce::String  skipReason;

        if (! processOneFile (singleFile, targetSampleRate, formatManager,
                              variant, skipReason))
        {
            auto cb     = singleVariantCallback;
            auto reason = skipReason;
            const int midi = singleMidi;
            const int vel  = singleVelLayer;
            juce::MessageManager::callAsync ([cb, midi, vel, reason]()
            {
                if (cb) cb (midi, vel, SampleVariant{}, reason);
            });
            return;
        }

        if (threadShouldExit())
            return;

        auto cb       = singleVariantCallback;
        auto loaded   = std::move (variant);
        const int midi = singleMidi;
        const int vel  = singleVelLayer;
        juce::MessageManager::callAsync ([cb, midi, vel, loaded]() mutable
        {
            if (cb) cb (midi, vel, std::move (loaded), {});
        });
        return;
    }

    // ------------------------------------------------------------------
    // Folder mode
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

    std::vector<LoadedFile> loaded;
    loaded.reserve (128);

    const juce::String wildcards = "*.wav;*.aif;*.aiff;*.flac";

    for (const auto& entry : juce::RangedDirectoryIterator (pendingFolder,
                                                            /*recursive*/ false,
                                                            wildcards,
                                                            juce::File::findFiles))
    {
        if (threadShouldExit())
            return;

        const juce::File   file        = entry.getFile();
        const juce::String displayName = file.getFileName();

        auto parsed = FilenameParser::parse (file.getFileNameWithoutExtension());
        if (! parsed.has_value())
        {
            skippedFiles.add (displayName);
            DBG ("SampleLoader: skipped (unparseable filename) " << displayName);
            continue;
        }

        const int effectiveVelLayer = folderOptions.overrideTokens
            ? juce::jlimit (0, 3, folderOptions.targetLayer)
            : parsed->velLayer;

        SampleVariant variant;
        juce::String  skipReason;
        if (! processOneFile (file, targetSampleRate, formatManager,
                              variant, skipReason))
        {
            skippedFiles.add (displayName);
            DBG ("SampleLoader: skipped (" << skipReason << ") " << displayName);
            continue;
        }

        loaded.push_back ({ parsed->midiNote,
                            effectiveVelLayer,
                            parsed->rrIndex,
                            std::move (variant) });
    }

    if (threadShouldExit())
        return;

    if (loaded.empty())
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

    // ----------------------------------------------------------------
    // v1.8.0: group by (midiNote, velocityLayer) — each group becomes one
    // cell. Detect ambiguity: groups with > 1 file AND zero rr/take/tk
    // tokens surface as AmbiguousDuplicate entries.
    // ----------------------------------------------------------------
    auto encodeKey = [] (int midi, int layer) -> int
    {
        return (juce::jlimit (0, 127, midi) << 4) | juce::jlimit (0, 3, layer);
    };

    std::vector<int> groupKeys;
    groupKeys.reserve (loaded.size());
    for (const auto& lf : loaded)
        groupKeys.push_back (encodeKey (lf.midiNote, lf.velocityLayer));

    std::vector<int> uniqueKeys = groupKeys;
    std::sort (uniqueKeys.begin(), uniqueKeys.end());
    uniqueKeys.erase (std::unique (uniqueKeys.begin(), uniqueKeys.end()),
                      uniqueKeys.end());

    SampleMap built;
    built.lowestNote        = 127;
    built.highestNote       = 0;
    int maxLayer            = 0;

    std::vector<AmbiguousDuplicate> ambiguous;

    for (int key : uniqueKeys)
    {
        // Collect indices of all loaded files that match this key.
        std::vector<int> idxs;
        for (int i = 0; i < (int) loaded.size(); ++i)
            if (groupKeys[(size_t) i] == key)
                idxs.push_back (i);

        if (idxs.empty())
            continue;

        const int midi  = loaded[(size_t) idxs[0]].midiNote;
        const int layer = loaded[(size_t) idxs[0]].velocityLayer;

        // Detect explicit RR vs ambiguous.
        bool anyExplicitRr = false;
        for (int i : idxs)
            if (loaded[(size_t) i].rrIndex >= 0)
            {
                anyExplicitRr = true;
                break;
            }

        if (idxs.size() > 1 && ! anyExplicitRr)
        {
            // Ambiguous duplicate group — record for modal confirmation.
            // The cell is still BUILT with all variants; the processor stages
            // it as `pendingDuplicateMap` until the user confirms.
            AmbiguousDuplicate dup;
            dup.midiNote      = midi;
            dup.velocityLayer = layer;
            for (int i : idxs)
                dup.filenames.add (loaded[(size_t) i].variant.filename);
            ambiguous.push_back (std::move (dup));
        }

        // Sort: explicit-RR entries first by rrIndex; -1 entries after, in
        // load order. Stable sort preserves enumeration order for ties.
        std::stable_sort (idxs.begin(), idxs.end(),
                          [&loaded] (int a, int b)
                          {
                              const int ra = loaded[(size_t) a].rrIndex;
                              const int rb = loaded[(size_t) b].rrIndex;
                              const int ka = (ra < 0) ? 1000 + a : ra;
                              const int kb = (rb < 0) ? 1000 + b : rb;
                              return ka < kb;
                          });

        SampleCell cell;
        cell.midiNote      = midi;
        cell.velocityLayer = layer;
        cell.variants.reserve (idxs.size());
        for (int i : idxs)
            cell.variants.push_back (std::move (loaded[(size_t) i].variant));

        built.lowestNote  = juce::jmin (built.lowestNote,  midi);
        built.highestNote = juce::jmax (built.highestNote, midi);
        maxLayer          = juce::jmax (maxLayer,          layer);

        built.cells.push_back (std::move (cell));
    }

    built.numVelocityLayers = juce::jlimit (1, 4, maxLayer + 1);

    auto map = std::make_shared<SampleMap> (std::move (built));

    auto cb        = completionCallback;
    auto skipped   = skippedFiles;
    auto ambigCopy = ambiguous;
    juce::MessageManager::callAsync ([cb, map, skipped, ambigCopy]() mutable
    {
        if (cb)
            cb (map, skipped, std::move (ambigCopy));
    });
}
