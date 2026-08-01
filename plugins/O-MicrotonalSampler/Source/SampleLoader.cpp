/*
   This file is part of O-MicrotonalSampler, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
#include "LoaderSupport.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <functional>
#include <utility>
#include <vector>

SampleLoader::SampleLoader()
    : juce::Thread ("SampleLoader")
{
}

SampleLoader::~SampleLoader()
{
    // v1.23.1 (CR-01): cooperative cancellation (see processOneFile) lets a large
    // in-flight load unwind promptly, so this join now completes well before the
    // timeout instead of escalating to a mid-decode pthread_cancel on shutdown.
    signalThreadShouldExit();
    stopThread (5000);
}

void SampleLoader::loadFolder (const juce::File& folder,
                               double             sr,
                               LoadOptions        options,
                               CompletionCallback onComplete,
                               FailureCallback    onFailure)
{
    // v1.23.1 (CR-01): signal + non-killing join. With the cooperative checkpoints
    // in processOneFile the previous worker unwinds within one read block, so this
    // returns fast; the generous timeout only bites in a genuinely stuck syscall,
    // where we prefer a rare assert over a pthread_cancel that leaks the reader and
    // hangs the UI (no completion callback ever fires after a force-kill).
    signalThreadShouldExit();
    if (! stopThread (5000))
        jassertfalse;

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
    // v1.23.1 (CR-01): signal + non-killing join (see loadFolder).
    signalThreadShouldExit();
    if (! stopThread (5000))
        jassertfalse;

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
    // v1.23.1 (CR-01): signal + non-killing join (see loadFolder).
    signalThreadShouldExit();
    if (! stopThread (5000))
        jassertfalse;
}

// ----------------------------------------------------------------------
// Per-file processing helper. Loads a file from disk, SR-converts, mono→stereo
// promotes, and populates a SampleVariant (audio + filename + loop fields).
// midiPitch/velocityLayer/rrIndex are resolved by the caller — this helper
// produces the variant only; cell-level addressing is the caller's concern.
// ----------------------------------------------------------------------
namespace
{
    // v1.23.1 (CR-01): `shouldExit` is polled between read blocks (and before the
    // resample) so a large-file decode is cooperatively cancellable. Empty predicate
    // = never cancel (kept optional so unit/harness callers need not supply one).
    bool processOneFile (const juce::File& file,
                         double            targetSR,
                         juce::AudioFormatManager& formatManager,
                         SampleVariant&    outVariant,
                         juce::String&     outSkipReason,
                         const std::function<bool()>& shouldExit = {})
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

        const int          srcChannels = (int) reader->numChannels;
        const juce::int64  srcLength   = reader->lengthInSamples;   // v1.23.6 (WR-02): keep int64
        const double       srcSR       = reader->sampleRate;

        if (srcChannels <= 0 || srcLength <= 0 || srcSR <= 0.0)
        {
            outSkipReason = "invalid header: " + displayName;
            return false;
        }

        // v1.23.6 (WR-01/WR-02): reject an absurd/corrupt length BEFORE narrowing
        // to int and BEFORE the proportional allocation. A bare (int) cast wraps a
        // >2^31 length to a small positive (silent under-read) or negative (false
        // reject), and an unbounded AudioBuffer would std::bad_alloc — which JUCE's
        // Release threadEntryPoint catch(...) swallows silently, killing the whole
        // folder worker with no callback. Here it becomes a recorded skip instead.
        if (! oms::isAcceptableSampleLength (srcLength))
        {
            outSkipReason = "file too large / bad length ("
                          + juce::String (srcLength) + " samples): " + displayName;
            return false;
        }

        const int srcSamples = (int) srcLength;   // safe: 1 .. kMaxSamplesPerFile

        // v1.23.6 (WR-01): from here the work allocates proportionally to the file
        // and calls into JUCE decode/resample paths that can throw (std::bad_alloc
        // on a genuinely large file, or a format-reader exception). Wrap the whole
        // body so a throw becomes a recorded skip + continue (folder mode) / a
        // reported failure (single-variant mode) instead of an uncaught throw that
        // unwinds run() past every already-decoded sample with no callback.
        try
        {

        // v1.23.6 (WR-03): pad ONE cleared guard sample. outNumSamples keeps the
        // ceil (below), so the LagrangeInterpolator's look-ahead can read up to
        // ~1 sample past srcSamples on odd-length SR conversions. The guard makes
        // that read in-bounds and silent (previously a real heap over-read).
        juce::AudioBuffer<float> sourceBuf (srcChannels, srcSamples + 1);
        sourceBuf.clear();

        // v1.23.1 (CR-01): read in blocks with a cancellation checkpoint between them.
        // A single monolithic reader->read() of a multi-hundred-MB file could not observe
        // threadShouldExit(), so a second load / project-close forced the 500 ms stopThread()
        // to escalate to pthread_cancel mid-read — leaking the reader, half-mutating state,
        // and never firing a callback (UI spinner hangs forever). Block reads are position-
        // addressed, so `sourceBuf` is byte-identical to the pre-v1.23.1 single read.
        constexpr int kReadBlockSamples = 1 << 16;   // 65 536 samples/chan
        for (int pos = 0; pos < srcSamples; pos += kReadBlockSamples)
        {
            if (shouldExit && shouldExit())
            {
                outSkipReason = "cancelled: " + displayName;
                return false;
            }

            const int blockLen = juce::jmin (kReadBlockSamples, srcSamples - pos);
            if (! reader->read (&sourceBuf, pos, blockLen, (juce::int64) pos, true, true))
            {
                outSkipReason = "read failure: " + displayName;
                return false;
            }
        }

        const bool needsResample = std::abs (srcSR - targetSR) > 0.1;
        double srcRatio    = 1.0;
        int outNumSamples  = srcSamples;

        if (needsResample)
        {
            srcRatio = srcSR / targetSR;
            // v1.23.6 (WR-03 / IN-05): ceil via the shared helper; the guard sample
            // above covers the interpolator's 1-sample over-read. srcSamples >= 1 and
            // srcRatio > 0, so the result is always >= 1 — the old `outNumSamples < 1`
            // clamp here was dead code and has been removed.
            outNumSamples = oms::resampleOutLength (srcSamples, srcRatio);
        }

        // v1.17.1 (LOAD-E3): build the 2-channel variant directly. Only the
        // first two source channels survive (mono is duplicated), so we never
        // resample/copy channels that would be discarded, and the previous
        // intermediate workBuf is gone — resampling writes straight into the
        // output. Decoded output is bit-identical to the pre-v1.17.1 path.
        const int srcCh1 = (srcChannels >= 2) ? 1 : 0;   // mono → reuse ch0

        // v1.23.1 (CR-01): bail before the (CPU-bound) resample of a cancelled load.
        if (shouldExit && shouldExit())
        {
            outSkipReason = "cancelled: " + displayName;
            return false;
        }

        outVariant.audio = std::make_shared<juce::AudioBuffer<float>> (2, outNumSamples);
        outVariant.audio->clear();

        if (needsResample)
        {
            // A fresh interpolator per channel (matches the pre-v1.17.1 loop).
            juce::LagrangeInterpolator interpL;
            interpL.reset();
            interpL.process (srcRatio,
                             sourceBuf.getReadPointer (0),
                             outVariant.audio->getWritePointer (0),
                             outNumSamples);

            if (srcChannels >= 2)
            {
                juce::LagrangeInterpolator interpR;
                interpR.reset();
                interpR.process (srcRatio,
                                 sourceBuf.getReadPointer (srcCh1),
                                 outVariant.audio->getWritePointer (1),
                                 outNumSamples);
            }
            else
            {
                // Mono: duplicate the resampled channel.
                outVariant.audio->copyFrom (1, 0, *outVariant.audio, 0, 0, outNumSamples);
            }

            DBG ("SampleLoader: resampled " << displayName
                 << " from " << srcSR << " Hz to " << targetSR
                 << " Hz (ratio " << srcRatio << ")");
        }
        else
        {
            outVariant.audio->copyFrom (0, 0, sourceBuf, 0,      0, outNumSamples);
            outVariant.audio->copyFrom (1, 0, sourceBuf, srcCh1, 0, outNumSamples);
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

        }   // end try (v1.23.6 WR-01)
        catch (const std::exception& e)
        {
            outSkipReason = juce::String ("decode error: ") + e.what()
                          + " (" + displayName + ")";
            return false;
        }
        catch (...)
        {
            outSkipReason = "decode error (unknown): " + displayName;
            return false;
        }
    }

    // v1.8.0: per-file scratch struct used during folder enumeration. Holds
    // the variant payload + addressing key + RR token (or -1 sentinel).
    // v1.14.0: technique field added (resolved by the loader from the parser
    // output + LoadOptions::overrideTechnique).
    struct LoadedFile
    {
        int           midiNote      = -1;
        int           velocityLayer = 0;
        int           technique     = 0;
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
                              variant, skipReason,
                              [this] { return threadShouldExit(); }))
        {
            // v1.23.1 (CR-01): a superseding load cancelled us mid-read — return
            // silently (a fresh worker is already starting) rather than firing a
            // spurious "cancelled" failure toast. Genuine decode failures still
            // report because threadShouldExit() is false for those.
            if (threadShouldExit())
                return;

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

        // v1.14.0: technique resolution. overrideTechnique forces every slot
        // onto LoadOptions::targetTechnique (used by the "assign folder to
        // technique" UI). Otherwise the parser token wins; missing token
        // (techniqueIndex == -1) defaults to slot 0 ("ord") so pre-1.14.0
        // libraries land on the back-compat default.
        const int parsedTech = (parsed->techniqueIndex < 0) ? 0 : parsed->techniqueIndex;
        const int effectiveTechnique = folderOptions.overrideTechnique
            ? juce::jlimit (0, kMaxTechniques - 1, folderOptions.targetTechnique)
            : juce::jlimit (0, kMaxTechniques - 1, parsedTech);

        SampleVariant variant;
        juce::String  skipReason;
        if (! processOneFile (file, targetSampleRate, formatManager,
                              variant, skipReason,
                              [this] { return threadShouldExit(); }))
        {
            skippedFiles.add (displayName);
            DBG ("SampleLoader: skipped (" << skipReason << ") " << displayName);
            continue;
        }

        loaded.push_back ({ parsed->midiNote,
                            effectiveVelLayer,
                            effectiveTechnique,
                            parsed->rrIndex,
                            std::move (variant) });
    }

    if (threadShouldExit())
        return;

    // v1.23.6 (IN-01): the enumeration above is FLAT (recursive=false), so samples
    // in nested subfolders are neither loaded nor reported — a user who dropped a
    // library organised into per-articulation subfolders would get a partial/empty
    // map with no explanation. Surface a single summary line in the skipped list.
    // (We keep the scan flat: recursing would silently ingest unrelated audio in
    // sibling/bounce/render subfolders, a worse surprise than an under-load.)
    {
        int ignoredSubfolders = 0;
        for (const auto& sub : juce::RangedDirectoryIterator (pendingFolder,
                                                              /*recursive*/ false,
                                                              "*",
                                                              juce::File::findDirectories))
        {
            juce::ignoreUnused (sub);
            ++ignoredSubfolders;
        }

        if (ignoredSubfolders > 0)
            skippedFiles.add (juce::String (ignoredSubfolders)
                + " subfolder(s) ignored (flat load — nested folders are not scanned)");
    }

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
    //
    // v1.14.0: key extended to (midi, layer, technique). Two files at the
    // same (midi, layer) with DIFFERENT techniques no longer collide — they
    // land in two distinct cells, one per technique slot. The encoded key
    // packs `midi[7] | layer[2] | technique[3]` into 12 bits — well within
    // a signed int.
    // ----------------------------------------------------------------
    auto encodeKey = [] (int midi, int layer, int tech) -> int
    {
        return  (juce::jlimit (0, 127, midi)             << 5)
              | (juce::jlimit (0, 3,   layer)            << 3)
              |  juce::jlimit (0, kMaxTechniques - 1, tech);
    };

    // v1.17.1 (LOAD-E4): group loaded files into cells with a single sort
    // instead of an O(n^2) per-unique-key full scan. We sort an index array by
    // (encodeKey, rr-sort-token, load-order). This reproduces BOTH the previous
    // ascending-key cell order AND the previous within-cell variant order — the
    // old code collected each group's indices in load order, then stable_sorted
    // them by the same token, which is equivalent to a single total order with a
    // load-order tiebreak. We then walk one linear pass, emitting a cell per
    // contiguous key run. encodeKey/token semantics are unchanged, so the built
    // map is identical to the pre-v1.17.1 output.
    const int numLoaded = (int) loaded.size();

    std::vector<int> groupKeys ((size_t) numLoaded);
    for (int i = 0; i < numLoaded; ++i)
        groupKeys[(size_t) i] = encodeKey (loaded[(size_t) i].midiNote,
                                           loaded[(size_t) i].velocityLayer,
                                           loaded[(size_t) i].technique);

    // rr-sort token: explicit rrIndex sorts first (ascending); -1 entries sort
    // after all explicit ones, in load order (matches the old `1000 + index`).
    auto rrToken = [&loaded] (int i) -> int
    {
        const int rr = loaded[(size_t) i].rrIndex;
        return (rr < 0) ? 1000 + i : rr;
    };

    std::vector<int> order ((size_t) numLoaded);
    for (int i = 0; i < numLoaded; ++i)
        order[(size_t) i] = i;

    std::sort (order.begin(), order.end(),
               [&] (int a, int b)
               {
                   if (groupKeys[(size_t) a] != groupKeys[(size_t) b])
                       return groupKeys[(size_t) a] < groupKeys[(size_t) b];
                   const int ta = rrToken (a);
                   const int tb = rrToken (b);
                   if (ta != tb)
                       return ta < tb;
                   return a < b;   // stable tiebreak = enumeration (load) order
               });

    SampleMap built;
    built.lowestNote        = 127;
    built.highestNote       = 0;
    int maxLayer            = 0;

    std::vector<AmbiguousDuplicate> ambiguous;

    for (int runStart = 0; runStart < numLoaded; )
    {
        const int key   = groupKeys[(size_t) order[(size_t) runStart]];
        int       runEnd = runStart + 1;
        while (runEnd < numLoaded && groupKeys[(size_t) order[(size_t) runEnd]] == key)
            ++runEnd;

        const int count     = runEnd - runStart;
        const int first     = order[(size_t) runStart];
        const int midi      = loaded[(size_t) first].midiNote;
        const int layer     = loaded[(size_t) first].velocityLayer;
        const int technique = loaded[(size_t) first].technique;     // v1.14.0

        // v1.23.6 (IN-02): collect the group's resolved RR indices and delegate
        // the ambiguity decision to oms::isAmbiguousRrGroup. Previously a group was
        // ambiguous ONLY when NO file carried an explicit RR token, so a mix like
        // {C3_v1_rr1.wav, C3_v1.wav} (a likely accidental duplicate next to an RR
        // set) — and two files sharing the same resolved RR index — merged silently
        // without the confirmation modal. The map is still BUILT with all variants
        // either way; this only governs whether the user is asked to confirm.
        std::vector<int> rrIndices;
        rrIndices.reserve ((size_t) count);
        for (int k = runStart; k < runEnd; ++k)
            rrIndices.push_back (loaded[(size_t) order[(size_t) k]].rrIndex);

        if (oms::isAmbiguousRrGroup (rrIndices))
        {
            // Ambiguous duplicate group — record for modal confirmation.
            // The cell is still BUILT with all variants; the processor stages
            // it as `pendingDuplicateMap` until the user confirms.
            AmbiguousDuplicate dup;
            dup.midiNote      = midi;
            dup.velocityLayer = layer;
            dup.technique     = technique;     // v1.14.0
            for (int k = runStart; k < runEnd; ++k)
                dup.filenames.add (loaded[(size_t) order[(size_t) k]].variant.filename);
            ambiguous.push_back (std::move (dup));
        }

        SampleCell cell;
        cell.midiNote      = midi;
        cell.velocityLayer = layer;
        cell.technique     = technique;     // v1.14.0
        cell.variants.reserve ((size_t) count);
        for (int k = runStart; k < runEnd; ++k)
            cell.variants.push_back (std::move (loaded[(size_t) order[(size_t) k]].variant));

        built.lowestNote  = juce::jmin (built.lowestNote,  midi);
        built.highestNote = juce::jmax (built.highestNote, midi);
        maxLayer          = juce::jmax (maxLayer,          layer);

        built.cells.push_back (std::move (cell));

        runStart = runEnd;
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
