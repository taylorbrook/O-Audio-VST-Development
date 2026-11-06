/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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
#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include "GranulatorParamIds.h"
#include "dsp/Engine.h"

// ============================================================================
// O-Granulator.
//
// 220 APVTS parameters (GranulatorParamIds.h), three bus layouts (mono→mono,
// mono→stereo, stereo→stereo), latency 0, and the full non-parameter state
// codec: seed, shared custom scale, per-stream × per-collection degree masks,
// uiLanguage. Stage 2 (Phases 2.1–2.6): ogran::Engine — 41 s MipRing<2>,
// absolute 32-sample pass grid, four streams on one pool (water-filled quotas,
// fair stealing), scale quantiser, window morph, per-grain FX, feedback chain,
// freeze, reverb, modulation matrix, onset / pitch analysis, R5 output law,
// dry/wet; infinite tail (D21). The editor is juce::GenericAudioProcessorEditor
// until Stage 3.
//
// Threading: everything in the "scale state" and "seed" sections is message-
// thread API (Stage 2 hands the audio thread its own copies). No listener and
// no audio-thread code writes a parameter.
// ============================================================================
class GranulatorProcessor : public juce::AudioProcessor
{
public:
    GranulatorProcessor();
    ~GranulatorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    // D21 (Phase 2.6): the tail is infinite. Freeze holds the ring content (and
    // the grains reading it) for as long as it stays on, and feedback 100 % only
    // reaches an exact 0 after 295.6 s under FTZ (RESEARCH-2.6 R81). JUCE maps
    // +inf to VST3 kInfiniteTail (R79) and AU kAudioUnitProperty_TailTime +inf
    // (R80), so hosts keep calling processBlock after the input stops.
    double getTailLengthSeconds() const override { return std::numeric_limits<double>::infinity(); }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState parameters;

    // ── Seed (deterministic per-grain randomness; persisted as int64) ────────
    uint64_t getSeed() const noexcept    { return seed.load (std::memory_order_relaxed); }
    void setSeed (uint64_t s) noexcept   { seed.store (s, std::memory_order_relaxed); }

    // ── UI language: a state-tree PROPERTY, not a parameter (0 en, 1 fr, 2 zh-Hans)
    std::atomic<int> uiLanguage { 0 };
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }

    // ── Scale state (session state, NOT parameters) ─────────────────────────
    struct CustomScale
    {
        juce::String name, text;
        float period = 1200.0f;
        uint32_t version = 0;
    };
    static constexpr int kMaxCustomNameChars = 40;
    static const juce::String& defaultCustomText();

    CustomScale getCustomScale() const;
    PitchCollections::Table getCustomTable() const;

    /** Parses `text` (cents, ratios or edo N [period]) into the ONE custom
        scale shared by all streams. Returns an error KEY (empty = applied);
        badToken names the offending token for badPitch. On success every
        stream's Custom mask resets to all-on (the degrees re-numbered). */
    juce::String setCustomScale (const juce::String& name, const juce::String& text, juce::String& badToken);

    /** sN_scale choice → table: nullptr for 0 (Off) / out of range; 1…30 the
        built-ins (via scaleChoiceToId), 31 the custom table. Message thread. */
    const PitchCollections::Table* getCollectionTable (int choice) const;

    /** Degree mask for stream 1…4 × collection choice 1…31. Bit d = degree d
        on. Stored canonical (bits at and above the degree count set), so
        "all on" is always ~0. Out-of-range arguments read ~0 / are ignored. */
    uint64_t getDegreeMask (int stream, int collection) const noexcept;
    void setDegreeMask (int stream, int collection, uint64_t mask) noexcept;

    /** Bumped on every scale-state change and on state restore (the editor re-pulls). */
    uint32_t getScaleStateRevision() const noexcept { return scaleStateRevision.load (std::memory_order_acquire); }

    static constexpr uint64_t kAllOn = ~uint64_t (0);
    static uint64_t canonicalMask (uint64_t mask, int degreeCount) noexcept
    {
        return degreeCount >= 64 ? mask : (mask | ~((uint64_t (1) << degreeCount) - 1));
    }

    /** 2.5b telemetry (ARCH Telemetry, P68): the live pitch-follower note (MIDI,
        float) and its confidence (0 / 1). Relaxed atomics written per cell on the
        audio thread; read by the UI (Stage 3). */
    float getPitchNote() const noexcept      { return engine.getPitchNote(); }
    bool  getPitchConfident() const noexcept { return engine.getPitchConfident(); }

   #if OGRAN_TEST_HOOKS
    /** Render-harness only: hooks (forceLevel0, disableFracQuant) and Diag. */
    ogran::Engine& getEngineForTest() noexcept { return engine; }
   #endif

private:
    static constexpr int maskIndex (int stream, int collection) noexcept
    {
        return (stream - 1) * GranulatorParamIds::kNumCollections + collection;
    }
    int degreeCountFor (int collection) const;
    void resetNonParameterState();
    void bumpScaleRevision() noexcept { scaleStateRevision.fetch_add (1, std::memory_order_acq_rel); }

    std::atomic<uint64_t> seed { 0 };

    mutable juce::SpinLock customLock;
    PitchCollections::Table customTable;
    juce::String customText, customName;
    std::atomic<uint32_t> customVersion { 0 };

    std::array<std::atomic<uint64_t>, GranulatorParamIds::kStreams * GranulatorParamIds::kNumCollections> degreeMasks;
    std::atomic<uint32_t> scaleStateRevision { 1 };

    bool monoToStereo = false;   // negotiated layout, decided in prepareToPlay
    int  busInChannels  = 2;     // main-bus channel counts from the NEGOTIATED layout
    int  busOutChannels = 2;     // (never from buffer width), decided in prepareToPlay

    // DSP engine. Parameter pointers are bound in the constructor; prepare()
    // allocates on the message thread; process() on the audio thread only.
    ogran::Engine engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranulatorProcessor)
};
