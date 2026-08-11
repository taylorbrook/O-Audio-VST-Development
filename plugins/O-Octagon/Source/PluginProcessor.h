/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>

#include "Data/VenueModel.h"
#include "Data/VenueSnapshot.h"
#include "DSP/ChannelMap.h"
#include "DSP/ConvexHull2D.h"

//==============================================================================
/**
    O-Octagon — Stage 2 Phase 2.1 (Geometry Core).

    An 8-channel DBAP spatialiser for an irregular, non-flat concert array.

    At Phase 2.1 the plugin owns the ROOM: the 42-value VENUE tree, the derived geometry, the convex
    hull, and the speaker→buffer channel map. It still writes the same mono sum to every output —
    but it writes it THROUGH the channel map. Independent per-speaker signal is FUNC-01 and arrives
    with the DBAP solver at Phase 2.2.

    Still absent, by plan: DbapSolver, GainStage, the 64-sample control grid, SmoothedValue,
    SourceShaper, HullProcessor, VerifyPing, any WebView editor.

    Deliberately NOT an AsyncUpdater. The one thing that could have needed deferral —
    setStateInformation() arriving before prepareToPlay() — is handled by a plain `preparedYet` flag
    instead, so there is no queued apply that could stomp restored state and therefore no
    cancelPendingUpdate() obligation (pattern_asyncupdater_guard_flag_needs_cancel).
*/
class OOctagonProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    OOctagonProcessor();
    ~OOctagonProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    /** Read-only access to the room, for the editor and the tests. Message thread. */
    const oo::VenueModel&   getVenue() const noexcept  { return venue; }
    const oo::ConvexHull2D& getHull()  const noexcept  { return hull; }

    /** True when the last map build failed and the previous valid map is still in force. Drives the
        persistent FUNC-03 UI warning; never a reason to route audio anyway. */
    bool isChannelMapInvalid() const noexcept { return mapInvalid.load (std::memory_order_acquire); }

    /** Applies a venue edit: re-derives geometry, rebuilds the hull and the map, publishes a new
        snapshot, and writes the venue back into apvts.state. Message thread only. */
    void applyVenueEdit (const oo::VenueModel& newVenue);

private:
    //==============================================================================
    // ─────────────────────────────────────────────────────────────────────────────
    // VENUE STORE — the slot claimed at Stage 1 (PLAN P2), now occupied.
    //
    // Declaration order matters and is why the slot was reserved above apvts rather than below it.
    // ─────────────────────────────────────────────────────────────────────────────

    oo::VenueModel   venue;
    oo::ConvexHull2D hull;

    juce::AudioProcessorValueTreeState apvts;

    //==============================================================================
    // Cached raw parameter pointers. Stage 1 does not read these in processBlock — they are the
    // Phase 2.2 control-grid snapshot source, and caching them here is what makes the constructor
    // complete. Matches the O-Orbit idiom.
    std::atomic<float>* srcXParam       { nullptr };
    std::atomic<float>* srcYParam       { nullptr };
    std::atomic<float>* srcZParam       { nullptr };
    std::atomic<float>* widthParam      { nullptr };
    std::atomic<float>* rolloffParam    { nullptr };
    std::atomic<float>* blurParam       { nullptr };
    std::atomic<float>* weightParam[ochan::kNumSpeakers] { nullptr, nullptr, nullptr, nullptr,
                                                           nullptr, nullptr, nullptr, nullptr };
    std::atomic<float>* hullAttenParam  { nullptr };
    std::atomic<float>* airAmountParam  { nullptr };
    std::atomic<float>* outputGainParam { nullptr };

    //==============================================================================
    // ── Channel map state (R1) ───────────────────────────────────────────────────────────────

    /** Speaker n → output buffer index. The LAST VALID map: on a failed rebuild this is retained
        unchanged and mapInvalid is raised, so the plugin never silently routes to a half-applied
        assignment. Initialised to identity so a pre-prepareToPlay read is defined. */
    std::array<int, ochan::kNumSpeakers> speakerToBuffer { 0, 1, 2, 3, 4, 5, 6, 7 };

    std::atomic<bool> mapInvalid { false };

    oo::VenueSnapshotPublisher venuePublisher;

    /** setStateInformation() can arrive before prepareToPlay(). The map must be built from the
        NEGOTIATED layout, so in that window the rebuild is deferred to prepareToPlay() rather than
        adding a second construction site. A plain flag, not an AsyncUpdater — there is no queued
        apply to cancel (pattern_asyncupdater_guard_flag_needs_cancel). */
    bool preparedYet { false };

    //==============================================================================
    /** THE SINGLE CHANNEL-MAP CONSTRUCTION SITE.

        The only caller of ochan::buildSpeakerToBuffer() in the plugin, the only writer of
        mapInvalid, and the only publisher of speakerToBuffer into the snapshot. Called from
        prepareToPlay() and on a label-map edit — nowhere else.
    */
    void rebuildChannelMap();

    /** Reads the VENUE child of apvts.state (defaults if missing or partial), re-derives geometry,
        rebuilds the hull, and publishes a new snapshot. Message thread only. */
    void readVenueFromState();

    /** Copies the current venue + hull + map into a snapshot and publishes it. */
    void publishSnapshot();

    /** True iff the channel map may be used to index THIS block's buffer.

        BOTH conditions are load-bearing and neither implies the other (RESEARCH-2.1 G1):

          - a VALID map is not evidence of an 8-channel BUFFER. Under the F3 hazard — Standalone on
            a 3-7 output device — canonicalChannelSet(n) is rejected, Release KEEPS the 7.1 layout,
            and the buffer arrives with n < 8 channels. mapInvalid stays false the whole time while
            speakerToBuffer still holds indices up to 7. The map is derived from
            getTotalNumOutputChannels(), which is the accessor that lies in exactly this state;
          - an 8-channel buffer is not evidence of a valid map.

        Phase 2.2's GainStage inner loop CALLS THIS. It does not re-derive it.

        @param numOutputChannels  the count the caller already read from buffer.getNumChannels()
    */
    bool mappedOutputAvailable (int numOutputChannels) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OOctagonProcessor)
};
