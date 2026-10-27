/*
   This file is part of O-Freeze, an Ouaricon Audio plugin.
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

    O-Freeze - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cstdint>
#include "CounterRng.h"

class OFreezeAudioProcessor : public juce::AudioProcessor
{
public:
    OFreezeAudioProcessor();
    ~OFreezeAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Freeze"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // ------------------------------------------------------------------------
    // v2.1.0 — the UI language. 0 = en, 1 = fr.
    //
    // An INDEX rather than a string because std::atomic<juce::String> does not
    // compile (juce::String is not trivially copyable), so the audio-safe form
    // is an index behind the two-function codec below while the PERSISTED form
    // stays a readable language code.
    //
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a preset must not be able to change which language
    // somebody reads their plugin in. It rides the APVTS state tree as a
    // non-parameter property instead.
    // ------------------------------------------------------------------------
    std::atomic<int> uiLanguage { 0 };

    /** The codec, three branches as of the zh-Hans release. languageIndex() maps
        anything the branches below do not name to 0, so a hand-edited session or
        an unexpected argument from the page degrades to English rather than
        being stored unvalidated.

        THE STORED FORM IS THE BCP-47 TAG, NOT THE INDEX, and the Simplified
        Chinese tag is spelled here exactly as the page's <option value> and
        i18n.js LANGUAGES spell it — one spelling crosses the whole boundary, so
        nothing has to translate between two of them. The two literals below are
        the ONLY occurrences of that tag in this file; the gate counts them,
        which is why this sentence does not spell it a third time.

        PURE ASCII, AND THAT IS THE CONTRACT. Not one Han character exists
        anywhere under Source/ — every Chinese string lives in the UI table, and
        the one in the markup is written as numeric character references. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }

    // v2.8.0: RNG seed (same scheme as O-TextureForge v1.6.0). Drawn at random per
    // instance and saved with the session as the rngSeed property, so a reopened
    // session, or a fresh instance given the same state, renders the same grains.
    std::atomic<uint64_t> rngSeed { static_cast<uint64_t>(juce::Random::getSystemRandom().nextInt64()) };

private:
    // Gate state machine
    enum class GateState { Idle, Frozen };

    // Grain structure for granular synthesis
    struct Grain
    {
        int startSample = 0;      // Position within grain (0 to length)
        // v2.8.0: fractional part of the grain's age, from its sub-sample onset.
        // The window runs at age startSample + onsetFrac; the read pointer stays on
        // whole samples (fractionalPosition), so a rate-1 grain is not interpolated.
        double onsetFrac = 0.0;
        // v2.7.0: the grain's own length, latched at onset. GRAIN_SIZE changes only
        // reach grains started after them, so a knob move never truncates or
        // re-windows a grain that is already sounding.
        int length = 1;
        double invLength = 1.0;   // 1 / length: window phase = startSample * invLength
        int position = 0;         // Base position in freeze buffer (set at activation, fixed)
        int jitterOffset = 0;     // Per-grain random position offset (decorrelates overlapping grains)
        float playbackRate = 1.0f;        // Pitch micro-detuning rate multiplier
        // Fractional read offset from base position. DOUBLE, not float (v2.3.0):
        // a float accumulator at 44100 samples (1000 ms grain) has an ulp of
        // 2^-8, so adding a rate of 1.004 per sample rounds the rate itself to a
        // multiple of 1/512 -- every grain's pitch snapped to a 3.38 ct grid,
        // measured. The knob claims 0.1 ct.
        double fractionalPosition = 0.0;
        // v2.7.0 fade-on-steal: 1.0f exactly unless the grain was stolen, then it
        // ramps to 0 by fadeStep per sample and the grain ends there.
        float fadeGain = 1.0f;
        float fadeStep = 0.0f;
        bool active = false;      // Grain active flag
    };

    // DSP Components (declared BEFORE parameters for initialization order)
    juce::AudioBuffer<float> freezeBuffer;
    int writePosition = 0;
    bool bufferFrozen = false;
    double currentSampleRate = 44100.0;
    juce::LinearSmoothedValue<float> freezeGain;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Threshold gate components. v2.8.0: evaluated per sample from a running sum
    // (was once per block), recomputed exactly each time the window wraps.
    GateState gateState = GateState::Idle;
    std::vector<float> rmsBuffer;
    double rmsSum = 0.0;
    int rmsWriteIndex = 0;
    int rmsSamplesPerWindow = 0;

    // Granular synthesis components
    // v2.7.0: a new grain takes any free slot of POOL_GRAINS (was round-robin over
    // GRAIN_COUNT slots, which restarted grains still sounding — IN-03). With the
    // +-30% trigger jitter, steady state peaks near GRAIN_COUNT / 0.7 + 1 = 47 live
    // grains, so the pool only overflows when a long GRAIN_SIZE is cut short while
    // its grains are still live. Then the oldest grain moves to one of the
    // STEAL_SLOTS spares and fades out over kStealFadeSeconds.
    static constexpr int POOL_GRAINS = 64;
    static constexpr int STEAL_SLOTS = 8;
    static constexpr int MAX_GRAINS = POOL_GRAINS + STEAL_SLOTS;
    static constexpr double kStealFadeSeconds = 0.005;
    int numGrains = 8;         // Target grain count (from GRAIN_COUNT parameter)
    int lastGrainCount = 8;    // Change detection
    std::array<Grain, MAX_GRAINS> grains;
    int grainSize = 0;         // Length given to the NEXT grain (latched per grain)
    int maxGrainSize = 0;
    float lastGrainSizeMs = 400.0f;
    // v2.8.0: onsets are scheduled in fractional samples. grainSize / numGrains is
    // no longer truncated, and a grain due between two samples starts on the later
    // one with its window already onsetFrac samples in.
    double grainTriggerInterval = 0.0;
    double nextTriggerInterval = 0.0;  // Jittered interval for next grain trigger
    double samplesSinceTrigger = 0.0;  // Age of the newest onset at the current sample
    bool stopTriggeringNewGrains = false;  // Soft release: let active grains complete

    // v2.8.0: counter-based RNG (was a time-seeded juce::Random shared with the LFO's
    // per-block S&H draws, so the block size reordered the grain draws). Each stream
    // has its own counter; a draw is hash(seed, counter, slot). The counters restart
    // in prepareToPlay and when the seed changes.
    enum RngSlot : uint32_t { kSlotJitter = 0, kSlotDetune, kSlotInterval, kSlotDrift, kSlotHold };
    uint64_t activeSeed = 0;
    uint64_t grainCounter = 0;      // one per started grain: jitter, detune, next interval
    uint64_t engageCounter = 0;     // one per fresh freeze: drift offset
    uint64_t lfoCycleCounter = 0;   // one per LFO cycle: S&H value
    float rand01(uint64_t counter, RngSlot slot) const noexcept { return CounterRng::uniform(activeSeed, counter, slot); }

    // v2.7.0: fixed Hann table read by phase (linear interpolation, +1 guard point).
    // Replaces the grainSize-long window that was rebuilt under live grains.
    static constexpr int WINDOW_LUT_SIZE = 2048;
    std::array<float, WINDOW_LUT_SIZE + 1> windowLut {};
    float windowAt(const Grain& g) const noexcept
    {
        const double x = (static_cast<double>(g.startSample) + g.onsetFrac) * g.invLength * WINDOW_LUT_SIZE;
        const int i = juce::jmin(static_cast<int>(x), WINDOW_LUT_SIZE - 1);
        const float f = static_cast<float>(x - static_cast<double>(i));
        return windowLut[static_cast<size_t>(i)] + f * (windowLut[static_cast<size_t>(i) + 1] - windowLut[static_cast<size_t>(i)]);
    }

    // v2.7.0 density compensation (research granular-2026-update §3.2; same form as
    // O-GrainScatter v3.2.0). Grains read different lags of the frozen audio, so they
    // sum incoherently: wet power ~ N * mu2 with N the LIVE grain count. A one-pole
    // estimate of that power over ~100 ms sets the gain 1 / sqrt(max(1, P)), which
    // never boosts. Replaces the per-sample divide by the window sum, which made
    // GRAIN_COUNT a level control (-12 dB from 2 to 32 grains) and divided a lone
    // grain's window out entirely.
    static constexpr float kHannMu2 = 0.375f;
    static constexpr double kDensitySmoothSeconds = 0.1;
    float densityPower = 0.0f;
    float densityCoeff = 0.0f;
    float nextDensityGain(float live) noexcept
    {
        densityPower += densityCoeff * (live * kHannMu2 - densityPower);
        return 1.0f / std::sqrt(juce::jmax(1.0f, densityPower));
    }

    void startGrain(Grain& g, int basePos, int jitterOffset, float playbackRate, double age) noexcept;
    int findFreeGrainSlot() noexcept;
    float randomDetuneRate(float detuneCents, uint64_t counter) const noexcept;
    void setFrozen(bool frozen, bool reverseActive, float driftValue, float detuneValue) noexcept;
    void controlTick(float lfoRate, float lfoDepth, int lfoShape, float driftValue) noexcept;

    // Per-sample grain scratch buffers (class members to avoid stack zero-init each sample)
    float windowValues[MAX_GRAINS] {};   // window * fadeGain
    int grainPos0[MAX_GRAINS] {};
    int grainPos1[MAX_GRAINS] {};
    float grainFrac[MAX_GRAINS] {};

    // WSOLA overlap matching (best-overlap grain positioning)
    static constexpr int WSOLA_TAIL_SIZE = 64;
    std::array<float, WSOLA_TAIL_SIZE> lastGrainTail{};
    bool hasLastGrainTail = false;

    // Drift offset (locked at freeze engage, shared by all grains)
    float frozenDriftOffset = 0.0f;    // Offset locked at freeze moment (0 to 1)
    juce::LinearSmoothedValue<float> smoothedDriftOffset; // Smoothed drift in samples (prevents clicks)

    // LFO state. v2.8.0: updated on a fixed 32-sample control tick counted on the
    // sample clock (was once per host block), together with the drift target.
    static constexpr int kControlInterval = 32;
    int controlCountdown = 0;
    float lfoPhase = 0.0f;            // 0 to 1 phase accumulator
    float lfoCurrentValue = 0.0f;     // Current LFO output (-1 to +1)
    float randomHoldValue = 0.0f;     // Sample-and-hold value for Random shape

    // APVTS comes AFTER DSP components
    juce::AudioProcessorValueTreeState parameters;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OFreezeAudioProcessor)
};
