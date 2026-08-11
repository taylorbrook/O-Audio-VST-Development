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

//==============================================================================
/**
    O-Octagon — Stage 1 foundation shell.

    An 8-channel DBAP spatialiser for an irregular, non-flat concert array. At Stage 1 this is the
    transport shell only: the bus declaration and negotiation predicate, the 17 APVTS parameters,
    and the session-state round-trip. There is no DBAP here — no ChannelMap, no VENUE tree, no
    smoothers, no control grid, no WebView. The shell exists to prove the 8-channel transport
    before any geometry depends on it.

    Deliberately NOT an AsyncUpdater: there is nothing to defer yet, and adding one now would make
    Phase 2.1 inherit a cancelPendingUpdate() obligation it did not ask for
    (pattern_asyncupdater_guard_flag_needs_cancel).
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

private:
    //==============================================================================
    // ─────────────────────────────────────────────────────────────────────────────
    // VENUE STORE SLOT — Phase 2.1 declares the venue member HERE, above apvts.
    //
    // Member declaration order is fixed at Stage 1 and is annoying to change once Phase 2.1
    // depends on it: a venue-aware construct would need the member to already exist by the time
    // apvts is initialised. The position is claimed now; at Stage 1 there is no member to declare,
    // so this costs one comment. See RESEARCH §3.3 / PLAN P2.
    // ─────────────────────────────────────────────────────────────────────────────

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
    std::atomic<float>* weightParam[8]  { nullptr, nullptr, nullptr, nullptr,
                                          nullptr, nullptr, nullptr, nullptr };
    std::atomic<float>* hullAttenParam  { nullptr };
    std::atomic<float>* airAmountParam  { nullptr };
    std::atomic<float>* outputGainParam { nullptr };

    //==============================================================================
    // Explicitly absent at Stage 1, by plan: ChannelMap / rebuildChannelMap(), VenueSnapshot,
    // SmoothedValue, FirstOrderTPTFilter, absoluteSampleCounter, and any Source/DSP or
    // Source/Data include. See PLAN.md §Non-goals.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OOctagonProcessor)
};
