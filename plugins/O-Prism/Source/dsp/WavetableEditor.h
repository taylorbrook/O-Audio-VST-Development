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

    WavetableEditor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

    Working copy manager for wavetable harmonic editing.
    Provides FFT analysis, harmonic editing, frame operations, and save.

  ==============================================================================
*/

#pragma once
#include "WavetableData.h"
#include "WavetableGenerator.h"
#include "UserWavetableManager.h"
#include <JuceHeader.h>
#include <memory>
#include <vector>

class WavetableEditor
{
public:
    WavetableEditor();

    /** Deep-copy source table into working copy and generate mipmaps. */
    void loadTable (const WavetableData* sourceTable);

    /** Release working copy. */
    void clearWorkingTable();

    /** Hand ownership of the working table to the caller (for safe retirement
        — the audio thread may still be reading it). Leaves no working table. */
    std::unique_ptr<WavetableData> releaseWorkingTable() { return std::move (workingTable); }

    /** Get harmonic magnitudes for a frame (bins 1..numBins). */
    std::vector<float> getFrameHarmonics (int frameIndex, int numBins) const;

    /** Set harmonic magnitudes for a frame (triggers iFFT + mipmap regen). */
    void setFrameHarmonics (int frameIndex, const std::vector<float>& magnitudes);

    /** Get time-domain waveform for a frame (2048 samples). */
    std::vector<float> getFrameWaveform (int frameIndex) const;

    /** Get downsampled waveforms for all frames (for strip display). */
    std::vector<std::vector<float>> getAllFrameWaveforms (int samplesPerFrame) const;

    /** Normalize selected frames to peak ±1.0 (per-frame or global). */
    void normalizeFrames (const std::vector<int>& frames, bool perFrame);

    /** Apply linear fade-in/out at frame boundaries. */
    void fadeEdges (const std::vector<int>& frames, float fadePercent);

    /** Reverse audio within each selected frame. */
    void reverseFrames (const std::vector<int>& frames);

    /** Reverse the ordering of selected frames. */
    void reverseOrder (const std::vector<int>& frames);

    /** Spectral smoothing: 6dB/oct rolloff above cutoff harmonic. */
    void smoothFrames (const std::vector<int>& frames, float strength);

    /** Save working table as user wavetable. Returns true on success. On
        success `replacedOut` receives the previously-registered table for this
        name (nullptr if new) so the caller can retire it RT-safely. */
    bool saveAsUserWavetable (const juce::String& name, UserWavetableManager& manager,
                              std::unique_ptr<WavetableData>& replacedOut);

    /** Get mutable pointer to working table (for oscillator preview). */
    WavetableData* getWorkingTable() { return workingTable.get(); }

    /** Get const pointer to working table. */
    const WavetableData* getWorkingTable() const { return workingTable.get(); }

    // ─── Publish rotation (CR-01 / REG-01) ───
    //
    // Two buffers alternate. `workingTable` is LIVE: startEditing() publishes
    // it to userTablePtrA/B, so the audio thread reads it every block.
    // `shadowTable` is PRIVATE and is where every mutating operation above
    // writes — mutating it cannot race anything.
    //
    // A publish is a swap, not a copy: shadow becomes live, and the buffer it
    // displaces goes to the caller to hold until the audio thread can no
    // longer reach it, then comes back through returnCooledTable() as the next
    // shadow. Exactly one buffer is ever in that cooling state, which is what
    // bounds the whole path to three allocations for an editing session.
    //
    // Edits arriving while the rotation is cooling are NOT dropped and do not
    // allocate: they accumulate in the shadow and ride out on the next
    // publish. Publishing faster than the audio thread can observe is waste,
    // so the coalescing costs nothing — a 60 Hz harmonic drag used to clone
    // 20 MiB per rAF frame.

    /** True when the shadow holds edits that have not been published. */
    bool hasPendingEdits() const { return ! dirtyFrames.empty(); }

    /** Swap the shadow in as the live table and hand back the buffer it
        displaced. Returns nullptr when nothing is pending. The caller MUST
        keep the returned buffer alive until the audio thread cannot reach it
        and then return it through returnCooledTable(). */
    std::unique_ptr<WavetableData> commitPendingEdits();

    /** Take back a buffer handed out by commitPendingEdits(), now unreachable
        by the audio thread. It becomes the next shadow, repaired from the live
        table on only the frames that diverged — one frame for a harmonic drag,
        ~82 KB, rather than a 20 MiB whole-table copy. */
    void returnCooledTable (std::unique_ptr<WavetableData> cooled);

    /** Release the private shadow buffer (session teardown). */
    std::unique_ptr<WavetableData> releaseShadowTable();

    int getNumFrames() const;
    bool hasWorkingTable() const { return workingTable != nullptr; }
    bool hasShadowTable() const { return shadowTable != nullptr; }

    /** Diagnostic: how many times acquireShadow() had to ALLOCATE a buffer
        instead of reusing the one the rotation handed back. The whole point of
        the rotation is that this stays flat while edits stream in — it is the
        counter that separates "recycled" from "reallocated", which buffer
        ADDRESSES cannot do, because a freed 20 MiB block is usually handed
        straight back by the allocator. */
    int getShadowAllocationCount() const { return shadowAllocations; }

private:
    /** The buffer every read accessor and every save must consult: the shadow
        while it carries unpublished edits, otherwise the live table. Without
        this the UI would redraw the last PUBLISHED table and a save would
        write it to disk, silently dropping the tail of the user's gesture. */
    const WavetableData* latest() const
    {
        return (shadowTable != nullptr && ! dirtyFrames.empty())
            ? shadowTable.get() : workingTable.get();
    }

    /** The private buffer mutating operations write into, created on demand
        and synced to the live table plus any edits made since the last
        publish. Returns nullptr when there is no working table.

        The allocating branch only runs when an edit arrives while the rotation
        is still cooling — once per publish cycle at large buffer sizes, never
        at small ones, where the cooled buffer is always back first. */
    WavetableData* acquireShadow();

    /** Record the frames an operation changed, so the next returned buffer
        knows what to repair. */
    void markDirty (int frame);
    void markDirty (const std::vector<int>& frames);

    /** Copy `frames` — all 10 mipmap levels and guard samples — from the live
        table into `dst`. */
    void copyFramesInto (WavetableData& dst, const std::vector<int>& frames) const;

    std::unique_ptr<WavetableData> workingTable;   // live, published
    std::unique_ptr<WavetableData> shadowTable;    // private, receives edits
    std::vector<int> dirtyFrames;     // edited into the shadow, not yet published
    std::vector<int> coolingResync;   // frames the outstanding cooled buffer must repair
    int shadowAllocations = 0;        // diagnostic, see getShadowAllocationCount()
    juce::dsp::FFT fft { 11 }; // 2048-point

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableEditor)
};
