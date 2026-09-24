/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    edit_rotation_check.cpp — O-Prism v1.27.0 regression gate for the three
    findings that the CR-01/CR-02/WR-07 fixes introduced and that have a
    deterministic single-threaded observable.

    [A] REG-01 — the copy-on-write publish is BOUNDED.

        The first shape of the CR-01 fix cloned the whole table per operation.
        The harmonic editor drives operations from requestAnimationFrame, so a
        drag ran that at 60 Hz: 20 MiB per frame against a retire queue that
        only the 500 ms timer swept. Nothing capped it, and the reaper's
        +2-generation rule keys off processBlock — so with the audio thread
        idle (exactly the case when a user opens the editor on a stopped
        transport) NOTHING ever expired.

        The rotation replaces it: two buffers alternate, at most one is ever
        cooling, and a returned buffer is repaired on only the frames that
        diverged. The signature is that a long burst of edits publishes from a
        SMALL, FIXED set of addresses.

        [A1] MEMORY — after 200 sequential edits with no processBlock at all,
             the processor holds at most 3 tables: live, shadow, and the one
             cooling buffer, with nothing stranded in the retire queue. Pre-fix
             this was 201, none of them freeable, because the timer never swept
             here (this console driver pumps no message loop) and its expiry
             rule keyed on a generation counter that never advanced.

        [A2] ALLOCATION — those 200 edits cost at most 2 shadow allocations.
             This is the half [A1] cannot see: freeing each clone instead of
             recycling it keeps the memory bound and still pays 200 x 20 MiB of
             allocation. Deliberately NOT gated on buffer addresses — an early
             draft was, and it passed with recycling disabled, because the
             allocator handed the same freed block straight back.

    [B] REG-01 — the rotation does not LOSE data.

        The saving that makes the rotation cheap is that a returned buffer is
        resynced on only the dirty frames instead of whole-table copied. If
        copyFramesInto missed a frame, or missed a mipmap level, the recycled
        buffer would carry stale content and an earlier edit would silently
        revert two publishes later. Nothing about that is visible in [A].

        [B1] Edit N DISTINCT frames in sequence, then assert every one of them
             still carries its edit in the live table. An edit that was
             resynced incorrectly reverts to the factory content.

        [B2] Every mipmap level of every frame agrees with a full regeneration
             from level 0. A resync that repaired level 0 but not levels 1..9
             passes [B1] and still detunes the top two octaves of the keyboard.

    [C] REG-02 — a host state restore closes the editing session.

        editingOscIndex and the editor's buffers are processor members that
        replaceState does not reset. Left open, the oscillator kept pointing at
        the working table (so the restored selection was inaudible) and the
        next editor operation republished the working table straight over it.

    [D] REG-03 — the routing predicate the per-sample hoist relies on.

        WR-07 removed a per-sample dead band that latched the last modulated
        LFO rate when the modulator crossed zero. Removing it left four
        std::exp2 running unconditionally in the voice loop. The hoist is safe
        only if "not routed" really does imply "offset is exactly 0 for every
        sample of the block" — that is, only if isDestinationRouted mirrors
        evaluate()'s skip conditions exactly. This gates the implication
        directly, over a sweep of slot configurations.

    [E] REG-01 — the edit is AUDIBLE.

        Everything above proves the POINTER moves. None of it proves a voice
        rendering audio reads the new buffer, and that gap is exactly how a
        copy-on-write fix regresses: in-place mutation was audible to a held
        voice for free (that was the CR-01 race), whereas a swap is audible
        only if updateWavetableAssignments repoints the voices. Three edits
        with real blocks rendered in between — the DAW interleave — each must
        change the rendered output.

        Runs on a FRESH processor. Sections [A]-[D] hammer the table with
        hundreds of synthetic spectra and leave the mod matrix randomised;
        rendering that would confound "the edit is inaudible" with "the state
        those sections left behind is degenerate".

    [F] REG-01 — repeated edits must not compound in gain.

        setFrameHarmonics rescales the user's 0..1 magnitudes by the frame's
        OWN current peak, so 250 successive edits must leave the level where
        they found it. Measured: 1.0 -> 0.540 on the first edit, then flat.

    NOT covered here:
      - REG-04, moving vst3Extensions.drainAndUpdate() above the WR-09
        zero-channel return. The drain dispatches through a slot that only the
        VST3 translation unit populates, so in a console build it is a
        pass-through and the correlation has no observable. It is verified
        statically — the call must precede the `getNumChannels() == 0` return
        in processBlock — by the grep recorded in the v1.27.0 CHANGELOG entry.
        The behavioural half (a zero-channel block, then a normal block still
        rendering at pitch) is already gated in bend_state_check [D].

    Usage:  O-Prism-edit-rotation-check
    Exit code = number of failed checks.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "dsp/ModulationMatrix.h"
#include "dsp/WavetableData.h"
#include "dsp/WavetableEditor.h"
#include "dsp/WavetableGenerator.h"

#include <cmath>
#include <iostream>
#include <set>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{

int failures = 0;

void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  ok    " : "  FAIL  ") << what << "\n";
    if (! ok)
        ++failures;
}

constexpr double kSampleRate = 48000.0;
constexpr int    kBlockSize  = 512;

/** A harmonic spectrum that is unmistakably not the factory content: one loud
    bin whose index is derived from `seed`, everything else silent. */
std::vector<float> spikeSpectrum (int numBins, int seed)
{
    std::vector<float> mags (static_cast<size_t> (numBins), 0.0f);
    mags[static_cast<size_t> (1 + (seed * 7) % (numBins - 1))] = 1.0f;
    return mags;
}

/** Peak magnitude of the live frame, as the editor reports it. */
int loudestBin (OPrismAudioProcessor& prism, int frame, int numBins)
{
    const auto mags = prism.getWavetableEditor().getFrameHarmonics (frame, numBins);
    if (mags.empty())
        return -1;

    int best = 0;
    for (size_t i = 1; i < mags.size(); ++i)
        if (mags[i] > mags[static_cast<size_t> (best)])
            best = static_cast<int> (i);

    return best;
}

} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::unique_ptr<juce::AudioProcessor> proc (createPluginFilter());
    auto* prism = dynamic_cast<OPrismAudioProcessor*> (proc.get());

    if (prism == nullptr)
    {
        std::cout << "  FAIL  createPluginFilter() did not return an OPrismAudioProcessor\n";
        return 1;
    }

    // setPlayConfigDetails BEFORE prepareToPlay, not instead of it: it is what
    // sets AudioProcessor::getSampleRate(), which prepareToPlay alone leaves
    // at 0. processBlock divides by that rate in advanceGlobalLfoPhases, so a
    // gate that skips this renders every block under a configuration no host
    // ever produces — and this one did, until v1.27.1.
    prism->setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);
    prism->prepareToPlay (kSampleRate, kBlockSize);

    // Park osc A on a multi-frame factory table, same walk as the CR-01 gate:
    // a single-frame table cannot show a per-frame resync failure at all.
    if (auto* tableParam = prism->getAPVTS().getParameter ("oscATable"))
    {
        const int numChoices = tableParam->getNumSteps();
        for (int i = 0; i < numChoices; ++i)
        {
            tableParam->setValueNotifyingHost (tableParam->convertTo0to1 (static_cast<float> (i)));
            const auto* candidate = prism->getActiveOscTable (0);
            if (candidate != nullptr && candidate->numFrames >= 4)
                break;
        }
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[A] REG-01 — the publish rotation is bounded\n";

    prism->startEditing (0);
    check (prism->getEditingOscIndex() == 0, "startEditing(0) opened a session");

    const int numFrames = prism->getWavetableEditor().getNumFrames();
    check (numFrames >= 4, juce::String ("working table has frames (") + juce::String (numFrames) + ")");

    constexpr int kNumBins = 64;
    constexpr int kBurst   = 200;

    // No processBlock is called anywhere in this section. That is the point:
    // it is the frozen-generation case that used to make the queue unbounded.
    //
    // Both checks below are COUNTERS, not buffer addresses. An earlier draft
    // gated on the set of distinct published pointers and it was unsound: with
    // recycling disabled the allocator handed the same 20 MiB block straight
    // back, so 200 real allocations still looked like 2 addresses. Addresses
    // cannot distinguish "reused" from "freed and reallocated".
    const int allocationsBefore = prism->getWavetableEditor().getShadowAllocationCount();
    int distinctPublished = 0;
    const WavetableData* lastPublished = nullptr;

    for (int i = 0; i < kBurst; ++i)
    {
        const auto mags = spikeSpectrum (kNumBins, i);
        prism->editWavetable ([&] (WavetableEditor& ed) { ed.setFrameHarmonics (0, mags); });

        const WavetableData* now = prism->getActiveOscTable (0);
        if (now != lastPublished)
            ++distinctPublished;
        lastPublished = now;
    }

    // [A1] MEMORY: live + shadow + cooling, and nothing stranded in the retire
    // queue. Pre-fix this was kBurst + 1 tables, none of them freeable.
    const int held = prism->getHeldTableCount();
    check (held <= 3,
           juce::String ("[A1] after ") + juce::String (kBurst)
           + " edits with the audio thread idle the processor holds "
           + juce::String (held) + " tables (bound is 3; pre-fix would be "
           + juce::String (kBurst + 1) + ")");

    // [A2] ALLOCATION: the rotation recycles rather than reallocates. One
    // allocation opens the rotation; every edit after that reuses the buffer
    // the sweep handed back.
    const int allocations = prism->getWavetableEditor().getShadowAllocationCount()
                          - allocationsBefore;
    check (allocations <= 2,
           juce::String ("[A2] ") + juce::String (kBurst) + " edits cost "
           + juce::String (allocations)
           + " shadow allocations (bound is 2; one per rotation slot)");

    check (distinctPublished >= 2,
           juce::String ("[A2] the rotation really does publish (")
           + juce::String (distinctPublished) + " pointer moves) — not a vacuous pass");

    check (lastPublished != nullptr,
           "[A1] the final publish resolved to a live table");

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[B] REG-01 — the dirty-frame resync loses nothing\n";

    // Give each frame its own unmistakable spectrum, one publish apart, so a
    // frame that is not repaired into the recycled buffer reverts visibly.
    const int framesToEdit = juce::jmin (numFrames, 8);
    std::vector<int> expectedBin (static_cast<size_t> (framesToEdit), -1);

    for (int f = 0; f < framesToEdit; ++f)
    {
        const int seed = f + 3;
        const auto mags = spikeSpectrum (kNumBins, seed);
        expectedBin[static_cast<size_t> (f)] = 1 + (seed * 7) % (kNumBins - 1);
        prism->editWavetable ([&] (WavetableEditor& ed) { ed.setFrameHarmonics (f, mags); });
    }

    int revertedFrames = 0;
    for (int f = 0; f < framesToEdit; ++f)
        if (loudestBin (*prism, f, kNumBins) != expectedBin[static_cast<size_t> (f)])
            ++revertedFrames;

    check (revertedFrames == 0,
           juce::String ("[B1] all ") + juce::String (framesToEdit)
           + " independently-edited frames survive the rotation ("
           + juce::String (revertedFrames) + " reverted)");

    // A resync that repaired level 0 but skipped levels 1..9 passes [B1] and
    // still plays the wrong table above the first mipmap crossover.
    {
        const WavetableData* live = prism->getActiveOscTable (0);
        check (live != nullptr, "[B2] a live table to inspect");

        if (live != nullptr)
        {
            WavetableData regenerated;
            regenerated.allocate (live->numFrames);

            for (int f = 0; f < live->numFrames; ++f)
            {
                const float* src = live->getFrameData (0, f);
                float* dst = regenerated.getFrameData (0, f);
                std::copy (src, src + WavetableData::kTableSize, dst);
            }

            WavetableGenerator::generateMipmaps (regenerated);

            int mismatchedLevels = 0;
            for (int level = 1; level < WavetableData::kNumMipmapLevels; ++level)
            {
                for (int f = 0; f < live->numFrames; ++f)
                {
                    const float* a = live->getFrameData (level, f);
                    const float* b = regenerated.getFrameData (level, f);
                    for (int i = 0; i < WavetableData::kFrameSize; ++i)
                    {
                        if (std::abs (a[i] - b[i]) > 1.0e-4f)
                        {
                            ++mismatchedLevels;
                            f = live->numFrames;   // one report per level is enough
                            break;
                        }
                    }
                }
            }

            check (mismatchedLevels == 0,
                   juce::String ("[B2] all 9 mipmap levels agree with a regeneration from level 0 (")
                   + juce::String (mismatchedLevels) + " diverged)");
        }
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[C] REG-02 — a host state restore closes the editing session\n";

    {
        // A blob captured while NOT editing is what a host hands back.
        prism->stopEditing (0);
        juce::MemoryBlock cleanState;
        prism->getStateInformation (cleanState);

        const WavetableData* restoredSource = prism->getActiveOscTable (0);

        // Now open a session and edit, which publishes the working table.
        prism->startEditing (0);
        prism->editWavetable ([&] (WavetableEditor& ed) {
            ed.setFrameHarmonics (0, spikeSpectrum (kNumBins, 11));
        });

        const WavetableData* previewTable = prism->getActiveOscTable (0);
        check (previewTable != restoredSource,
               "[C] the open session is previewing its working table (stimulus is real)");

        prism->setStateInformation (cleanState.getData(),
                                    static_cast<int> (cleanState.getSize()));

        check (prism->getEditingOscIndex() < 0,
               "[C] setStateInformation closed the editing session");

        check (prism->getActiveOscTable (0) == restoredSource,
               "[C] osc A resolves to the RESTORED table, not the working copy");

        // The UI panel may still think it is open. The next operation it sends
        // must not republish over the restored state.
        const WavetableData* afterRestore = prism->getActiveOscTable (0);
        prism->editWavetable ([&] (WavetableEditor& ed) {
            ed.setFrameHarmonics (0, spikeSpectrum (kNumBins, 19));
        });

        check (prism->getActiveOscTable (0) == afterRestore,
               "[C] a stale editor op after the restore publishes NOTHING");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[D] REG-03 — unrouted destination implies a zero offset\n";

    {
        // The hoist reads isDestinationRouted once per block and skips the
        // per-sample std::exp2 when it is false. That is sound only if an
        // unrouted destination can never accumulate a non-zero offset.
        ModulationMatrix matrix;
        matrix.setAPVTS (&prism->getAPVTS());

        auto& apvts = prism->getAPVTS();
        juce::Random rng (0xBEEF);

        int violations = 0;
        int routedSeen  = 0;

        for (int trial = 0; trial < 200; ++trial)
        {
            for (int slot = 0; slot < ModulationMatrix::kNumSlots; ++slot)
            {
                const auto prefix = "modSlot" + juce::String (slot);
                auto setP = [&] (const juce::String& id, float norm)
                {
                    if (auto* p = apvts.getParameter (id))
                        p->setValueNotifyingHost (norm);
                };

                setP (prefix + "Src", rng.nextFloat());
                setP (prefix + "Dst", rng.nextFloat());
                setP (prefix + "Amt", rng.nextFloat());
                setP (prefix + "On",  rng.nextBool() ? 1.0f : 0.0f);
            }

            matrix.updateFromAPVTS();

            // Every source hot, so any live route produces a non-zero offset.
            for (int src = 1; src < ModulationMatrix::kNumSources; ++src)
                matrix.setSourceValue (static_cast<ModSource> (src), 1.0f);

            matrix.evaluate();

            for (int d = 1; d < ModulationMatrix::kNumDests; ++d)
            {
                const auto dest = static_cast<ModDest> (d);
                const bool routed = matrix.isDestinationRouted (dest);

                if (routed)
                    ++routedSeen;
                else if (matrix.getModOffset (dest) != 0.0f)
                    ++violations;
            }
        }

        check (violations == 0,
               juce::String ("[D] no unrouted destination ever carried an offset (")
               + juce::String (violations) + " violations over 200 configurations)");

        check (routedSeen > 0,
               juce::String ("[D] the sweep actually produced routed destinations (")
               + juce::String (routedSeen) + ") — not a vacuous pass");
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[E] The edit is AUDIBLE — rendered output actually changes\n";

    {
        // Everything above proves the POINTER moves. None of it proves a voice
        // rendering audio reads the new buffer. That gap is exactly how a
        // copy-on-write fix regresses: in-place mutation was audible to a held
        // voice for free (that was the CR-01 race), whereas a swap is audible
        // only if updateWavetableAssignments repoints the voices.
        // A FRESH instance. Sections [A]-[D] hammered this table with hundreds
        // of synthetic spectra; rendering the result would confound "the edit
        // is inaudible" with "the table those sections left behind is
        // degenerate". This section must answer one question only.
        std::unique_ptr<juce::AudioProcessor> proc2 (createPluginFilter());
        auto* prism = dynamic_cast<OPrismAudioProcessor*> (proc2.get());
        check (prism != nullptr, "[E] fresh processor instance");
        if (prism == nullptr)
            return failures;

        // setPlayConfigDetails BEFORE prepareToPlay, not instead of it: it is what
        // sets AudioProcessor::getSampleRate(), which prepareToPlay alone leaves
        // at 0. processBlock divides by that rate in advanceGlobalLfoPhases, so a
        // gate that skips this renders every block under a configuration no host
        // ever produces — and this one did, until v1.27.1.
        prism->setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);
        prism->prepareToPlay (kSampleRate, kBlockSize);

        auto setParam = [&] (const juce::String& id, float scaled)
        {
            if (auto* p = prism->getAPVTS().getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 (scaled));
        };

        // Park osc A at frame 0 and make it the only thing we hear.
        setParam ("oscAPos", 0.0f);
        setParam ("oscMix", 0.0f);

        juce::AudioBuffer<float> buf (2, kBlockSize);

        auto renderBlocks = [&] (int n, juce::MidiBuffer& midi)
        {
            for (int i = 0; i < n; ++i)
            {
                buf.clear();
                prism->processBlock (buf, midi);
                midi.clear();
            }
        };

        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
        renderBlocks (6, midi);   // settle the envelope and the assignments

        auto capture = [&] ()
        {
            juce::MidiBuffer none;
            buf.clear();
            prism->processBlock (buf, none);
            return std::vector<float> (buf.getReadPointer (0),
                                       buf.getReadPointer (0) + kBlockSize);
        };

        auto rms = [] (const std::vector<float>& v)
        {
            double sum = 0.0;
            for (float x : v) sum += static_cast<double> (x) * x;
            return std::sqrt (sum / static_cast<double> (v.size()));
        };

        auto differs = [&] (const std::vector<float>& a, const std::vector<float>& b)
        {
            double sum = 0.0;
            for (size_t i = 0; i < a.size(); ++i)
            {
                const double d = static_cast<double> (a[i]) - b[i];
                sum += d * d;
            }
            return std::sqrt (sum / static_cast<double> (a.size()));
        };

        auto allFin = [] (const std::vector<float>& v)
        {
            for (float x : v) if (! std::isfinite (x)) return false;
            return true;
        };

        const auto reference = capture();
        check (allFin (reference), "[E] pre-edit output is finite (no NaN)");
        check (rms (reference) > 1.0e-4,
               juce::String ("[E] the held note is audible before editing (rms ")
               + juce::String (rms (reference), 6) + ")");

        prism->startEditing (0);
        check (prism->getEditingOscIndex() == 0, "[E] editing session open");

        // Three edits in sequence, each rendering blocks in between — the real
        // DAW interleave, and the path where a publish can be coalesced.
        std::vector<float> previous = reference;

        for (int step = 0; step < 3; ++step)
        {
            std::vector<float> mags (kNumBins, 0.0f);
            mags[static_cast<size_t> (1 + step * 5)] = 1.0f;   // a different partial each time

            prism->editWavetable ([&] (WavetableEditor& ed) { ed.setFrameHarmonics (0, mags); });

            juce::MidiBuffer none;
            renderBlocks (4, none);
            const auto after = capture();

            check (allFin (after),
                   juce::String ("[E] edit ") + juce::String (step + 1) + " output is finite");

            const double delta = differs (previous, after);
            check (delta > 1.0e-3,
                   juce::String ("[E] edit ") + juce::String (step + 1)
                   + " changed the rendered audio (rms delta " + juce::String (delta, 6)
                   + ", held=" + juce::String (prism->getHeldTableCount())
                   + ", osc=" + juce::String (prism->getEditingOscIndex()) + ")");

            previous = after;
        }

        prism->stopEditing (0);
    }

    // ───────────────────────────────────────────────────────────────
    std::cout << "\n[F] Repeated harmonic edits must not compound in gain\n";

    {
        std::unique_ptr<juce::AudioProcessor> proc3 (createPluginFilter());
        auto* prism = dynamic_cast<OPrismAudioProcessor*> (proc3.get());
        check (prism != nullptr, "[F] fresh processor instance");
        if (prism == nullptr)
            return failures;

        // setPlayConfigDetails BEFORE prepareToPlay, not instead of it: it is what
        // sets AudioProcessor::getSampleRate(), which prepareToPlay alone leaves
        // at 0. processBlock divides by that rate in advanceGlobalLfoPhases, so a
        // gate that skips this renders every block under a configuration no host
        // ever produces — and this one did, until v1.27.1.
        prism->setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);
        prism->prepareToPlay (kSampleRate, kBlockSize);
        prism->startEditing (0);

        auto framePeak = [&] () -> float
        {
            const auto wf = prism->getWavetableEditor().getFrameWaveform (0);
            float pk = 0.0f;
            for (float v : wf)
                pk = std::max (pk, std::abs (v));
            return pk;
        };

        const float peak0 = framePeak();
        std::cout << "        peak after   0 edits: " << peak0 << "\n";

        float peakN = peak0;
        for (int i = 1; i <= 250; ++i)
        {
            prism->editWavetable ([&] (WavetableEditor& ed) {
                ed.setFrameHarmonics (0, spikeSpectrum (kNumBins, i));
            });
            peakN = framePeak();
            if (i % 50 == 0)
                std::cout << "        peak after " << i << " edits: " << peakN << "\n";
        }

        check (std::isfinite (peakN),
               juce::String ("[F] frame is still finite after 250 harmonic edits (peak ")
               + juce::String (peakN) + ")");

        // setFrameHarmonics rescales the user's 0..1 magnitudes by the frame's
        // OWN current peak, so an edit whose input peaks at 1.0 should leave
        // the frame's level where it was. Any systematic drift compounds.
        check (peakN < peak0 * 8.0f && peakN > peak0 / 8.0f,
               juce::String ("[F] level is stable across edits (") + juce::String (peak0)
               + " -> " + juce::String (peakN) + ", bound is 8x either way)");

        prism->stopEditing (0);
    }

    std::cout << "\n" << (failures == 0 ? "ALL CHECKS PASSED" : "FAILURES: " + std::to_string (failures))
              << "\n\n";
    return failures;
}
