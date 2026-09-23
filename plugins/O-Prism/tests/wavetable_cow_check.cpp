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

    wavetable_cow_check.cpp — O-Prism v1.27.0 wavetable copy-on-write gate.

    Closes CR-01 of the v1.26.0 code review: the wavetable editor mutated the
    live, published table in place, racing the audio thread.

    startEditing() stores wavetableEditor.getWorkingTable() into userTablePtrA/B,
    so from that moment the voices read that exact buffer every block for live
    preview. Every editor operation then wrote straight into it from the message
    thread — level 0 via std::copy, then all 10 mipmap levels and the guard
    sample per frame — with no synchronisation of the CONTENTS between the two
    threads. (The v1.19.0 reaper made it safe to FREE a table the audio thread
    may hold; it says nothing about writing to one it is holding right now.)

    The fix is copy-on-write: each operation clones, mutates the clone, and
    installs it; OPrismAudioProcessor::publishEditedWorkingTable() then stores
    the new pointer with a release store and hands the displaced table to
    retireTable().

    What is gated, per operation (all six, plus the harmonic editor):

      [1] The edit publishes a NEW table — getActiveOscTable() must return a
          different pointer than before the call. This is what fails on
          pre-fix code, where the pointer never moved.

      [2] The DISPLACED buffer is byte-identical to the snapshot taken before
          the edit. This is the CR-01 assertion proper: it is the buffer the
          audio thread is still reading, and in-place mutation is exactly
          what makes it differ. Reading it is well-defined — retireTable()
          parks it in retiredTables (a live unique_ptr), and the reaper that
          would free it only runs from timerCallback(), which never fires
          here: this console driver pumps no message loop.

      [3] The NEW buffer DIFFERS from the snapshot — the operation actually
          did something. Without this, [1] and [2] would both pass for an
          op that cloned and then no-opped, which is a vacuous green
          (pattern_gate_stimulus_below_threshold_is_vacuous).

      [4] Frame count is preserved and every sample stays finite.

    [5] is a concurrency smoke, not a proof: a render thread runs processBlock
    with a note held while the main thread hammers edits, which is the exact
    reported failure ("hold a pad chord, open the editor, drag a harmonic").
    It asserts survival and finite output. A weak-ordering race does not
    reproduce on demand, so this is an opportunistic net over the retire path
    under contention — [1]..[4] are the deterministic verdict.

    NOT covered here:
      - CR-02, the release/acquire ordering on the userTablePtrA/B publish.
        A memory-ordering edge has no single-threaded observable, and the
        window it closes is an arm64 store-buffer visibility gap that no
        stress loop reproduces on demand. It is verified statically: no
        userTablePtr store or load may use memory_order_relaxed. The grep
        that enforces it is in the v1.27.0 CHANGELOG entry.

    Usage:  O-Prism-wavetable-cow-check
    Exit code = number of failed checks.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "dsp/WavetableData.h"
#include "dsp/WavetableEditor.h"

#include <atomic>
#include <cmath>
#include <iostream>
#include <memory>
#include <thread>
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

bool allFinite (const std::vector<float>& v)
{
    for (float s : v)
        if (! std::isfinite (s))
            return false;
    return true;
}

/** Run one mutating editor operation through the copy-on-write wrapper and
    assert the four invariants above. `label` names the op in the output. */
void gateOperation (OPrismAudioProcessor& prism,
                    const juce::String& label,
                    std::function<void (WavetableEditor&)> op)
{
    const WavetableData* before = prism.getActiveOscTable (0);
    if (before == nullptr)
    {
        check (false, label + ": no active table to edit");
        return;
    }

    // Snapshot the buffer the audio thread is currently reading.
    const std::vector<float> snapshot = before->data;
    const int framesBefore = before->numFrames;

    prism.editWavetable (op);

    const WavetableData* after = prism.getActiveOscTable (0);

    // [1] A fresh table was published.
    check (after != nullptr && after != before,
           label + ": published a NEW table (pointer moved)");

    if (after == nullptr)
        return;

    // [2] CR-01 proper — the displaced buffer was not written through.
    check (before->data == snapshot,
           label + ": the DISPLACED buffer is untouched (no in-place mutation)");

    // [3] Non-vacuity — the edit actually changed something.
    check (after->data != snapshot,
           label + ": the new buffer differs from the snapshot (edit applied)");

    // [4] Shape and finiteness preserved.
    check (after->numFrames == framesBefore && allFinite (after->data),
           label + ": frame count preserved and all samples finite");
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

    prism->prepareToPlay (kSampleRate, kBlockSize);

    std::cout << "\n[0] Editing session publishes the working table\n";

    // Park osc A on a MULTI-FRAME factory table. The default (index 0) has a
    // single frame, which silently skips reverseOrder and the global
    // normalize — both of them frame-swapping ops, i.e. exactly the two the
    // one-frame case cannot exercise. Walk the bank for the first table with
    // two or more frames rather than hard-coding an index, so a re-ordering
    // of the bank cannot quietly drop the coverage again.
    if (auto* tableParam = prism->getAPVTS().getParameter ("oscATable"))
    {
        const int numChoices = tableParam->getNumSteps();
        int chosen = -1;

        for (int i = 0; i < numChoices; ++i)
        {
            tableParam->setValueNotifyingHost (tableParam->convertTo0to1 (static_cast<float> (i)));
            const auto* candidate = prism->getActiveOscTable (0);
            if (candidate != nullptr && candidate->numFrames >= 2)
            {
                chosen = i;
                break;
            }
        }

        check (chosen >= 0,
               juce::String ("found a multi-frame factory table (index ") + juce::String (chosen) + ")");
    }
    else
    {
        check (false, "oscATable parameter exists");
    }

    const WavetableData* factoryTable = prism->getActiveOscTable (0);
    check (factoryTable != nullptr, "osc A resolves to a table before editing");
    check (! prism->isUserTableActive (0), "osc A starts on a factory table");

    prism->startEditing (0);
    check (prism->getEditingOscIndex() == 0, "startEditing(0) opened the session");

    const WavetableData* workingTable = prism->getActiveOscTable (0);
    check (workingTable != nullptr && workingTable != factoryTable,
           "osc A now reads the editor's working copy, not the factory table");
    check (workingTable == prism->getWavetableEditor().getWorkingTable(),
           "the published pointer IS the editor's working table");

    const int numFrames = prism->getWavetableEditor().getNumFrames();
    check (numFrames > 0, juce::String ("working table has frames (") + juce::String (numFrames) + ")");

    if (numFrames <= 0)
    {
        std::cout << "\nFAILURES: " << failures << "\n";
        return failures == 0 ? 1 : failures;
    }

    // Every op below targets frame 0; reverseOrder needs two distinct frames.
    const std::vector<int> frame0 { 0 };
    const std::vector<int> firstTwo = (numFrames >= 2) ? std::vector<int> { 0, 1 } : frame0;

    std::cout << "\n[1..4] Each editor operation is copy-on-write\n";

    // Harmonic editor. A descending ramp over 64 bins is guaranteed to differ
    // from any factory frame's own spectrum, so [3] cannot pass vacuously.
    {
        std::vector<float> magnitudes (64);
        for (size_t k = 0; k < magnitudes.size(); ++k)
            magnitudes[k] = 1.0f / static_cast<float> (k + 1);

        gateOperation (*prism, "setFrameHarmonics", [&] (WavetableEditor& e) {
            e.setFrameHarmonics (0, magnitudes);
        });
    }

    // Normalize to peak 1.0. The harmonic edit above leaves frame 0 well below
    // full scale, so this is a real change rather than a no-op.
    gateOperation (*prism, "normalizeFrames(perFrame)", [&] (WavetableEditor& e) {
        e.normalizeFrames (frame0, true);
    });

    gateOperation (*prism, "fadeEdges", [&] (WavetableEditor& e) {
        e.fadeEdges (frame0, 25.0f);
    });

    gateOperation (*prism, "reverseFrames", [&] (WavetableEditor& e) {
        e.reverseFrames (frame0);
    });

    gateOperation (*prism, "smoothFrames", [&] (WavetableEditor& e) {
        e.smoothFrames (frame0, 0.75f);
    });

    if (numFrames >= 2)
    {
        // Frame 0 has been edited heavily above and frame 1 has not, so
        // swapping them is guaranteed observable.
        gateOperation (*prism, "reverseOrder", [&] (WavetableEditor& e) {
            e.reverseOrder (firstTwo);
        });

        gateOperation (*prism, "normalizeFrames(global)", [&] (WavetableEditor& e) {
            e.normalizeFrames (firstTwo, false);
        });
    }
    else
    {
        std::cout << "  skip  reverseOrder / global normalize (table has one frame)\n";
    }

    std::cout << "\n[5] Render thread + concurrent edits (smoke)\n";
    {
        std::atomic<bool> stop { false };
        std::atomic<int>  blocksRendered { 0 };
        std::atomic<bool> sawNonFinite { false };
        std::atomic<bool> sawSignal { false };

        std::thread renderThread ([&] {
            juce::AudioBuffer<float> buffer (2, kBlockSize);
            juce::MidiBuffer midi;

            // Hold a chord for the whole run — this is the reported scenario.
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
            midi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.8f), 1);
            midi.addEvent (juce::MidiMessage::noteOn (1, 67, 0.8f), 2);

            while (! stop.load (std::memory_order_relaxed))
            {
                buffer.clear();
                prism->processBlock (buffer, midi);
                midi.clear();

                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                {
                    const float* d = buffer.getReadPointer (ch);
                    for (int i = 0; i < buffer.getNumSamples(); ++i)
                    {
                        if (! std::isfinite (d[i]))
                            sawNonFinite.store (true, std::memory_order_relaxed);
                        if (std::abs (d[i]) > 1.0e-5f)
                            sawSignal.store (true, std::memory_order_relaxed);
                    }
                }

                blocksRendered.fetch_add (1, std::memory_order_relaxed);
            }
        });

        // Hammer the editor from this (message) thread while the chord renders.
        std::vector<float> magnitudes (48);
        for (int pass = 0; pass < 60; ++pass)
        {
            for (size_t k = 0; k < magnitudes.size(); ++k)
                magnitudes[k] = ((pass + static_cast<int> (k)) % 7 == 0) ? 1.0f
                              : 1.0f / static_cast<float> (k + 2);

            prism->editWavetable ([&] (WavetableEditor& e) {
                e.setFrameHarmonics (0, magnitudes);
            });
            prism->editWavetable ([&] (WavetableEditor& e) {
                e.normalizeFrames (frame0, true);
            });
            prism->editWavetable ([&] (WavetableEditor& e) {
                e.reverseFrames (frame0);
            });
        }

        stop.store (true, std::memory_order_relaxed);
        renderThread.join();

        check (blocksRendered.load() > 0,
               juce::String ("render thread ran (") + juce::String (blocksRendered.load()) + " blocks)");
        check (! sawNonFinite.load(), "no non-finite sample rendered during concurrent edits");
        check (sawSignal.load(), "the held chord actually produced audio (stimulus was not silent)");
        check (prism->getActiveOscTable (0) != nullptr, "osc A still resolves after the hammer run");
    }

    std::cout << "\n[6] Teardown\n";
    prism->stopEditing (0);
    check (prism->getEditingOscIndex() < 0, "stopEditing closed the session");
    check (prism->getActiveOscTable (0) == factoryTable,
           "osc A reverted to the factory table it started on");

    proc->releaseResources();

    std::cout << "\n" << (failures == 0 ? "ALL CHECKS PASSED" : "FAILURES: " + std::to_string (failures))
              << "\n";
    return failures;
}
