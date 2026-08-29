/*
   This file is part of O-Gain, an Ouaricon Audio plugin.
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

    O-Gain - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class OGainAudioProcessor : public juce::AudioProcessor,
                            private juce::AsyncUpdater,
                            private juce::AudioProcessorValueTreeState::Listener
{
public:
    OGainAudioProcessor();
    ~OGainAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Gain"; }
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

    // =========================================================================
    // Metering atomics (audio thread writes, UI thread reads)
    // =========================================================================

    // Input metering (post-utilities, pre-gain)
    std::atomic<float> inputPeakL  { 0.0f };
    std::atomic<float> inputPeakR  { 0.0f };
    std::atomic<float> inputRmsL   { 0.0f };
    std::atomic<float> inputRmsR   { 0.0f };

    // Output metering (post-gain)
    std::atomic<float> outputPeakL { 0.0f };
    std::atomic<float> outputPeakR { 0.0f };
    std::atomic<float> outputRmsL  { 0.0f };
    std::atomic<float> outputRmsR  { 0.0f };

    // VU metering (ballistics-filtered, post-utilities pre-gain)
    std::atomic<float> vuLevelL    { 0.0f };
    std::atomic<float> vuLevelR    { 0.0f };

    // =========================================================================
    // Learn mode state (UI thread writes flags, audio thread reads/accumulates)
    // =========================================================================

    std::atomic<bool>  learnActive        { false };

    // -------------------------------------------------------------------------
    // Learn-panel snapshot (WR-05)
    // -------------------------------------------------------------------------
    // All Learn-panel readouts are published as ONE coherent unit via a seqlock
    // (learnSnapshotSeq) so the editor timer can never send JS a mix of fields
    // from different processBlock iterations (e.g. state="DONE" with a stale
    // integrated value). Plain peak/RMS/VU meters remain independent atomics —
    // they need no cross-field coherence.
    struct LearnSnapshot
    {
        int   state          = 0;         // 0=idle, 1=learning, 2=complete
        int   confidence     = 0;         // 0=none, 1=low, 2=medium, 3=high
        float momentaryLUFS  = -100.0f;
        float shortTermLUFS  = -100.0f;
        float integratedLUFS = -100.0f;
        float samplePeakDBFS = -100.0f;   // WR-03: digital sample peak (dBFS), NOT oversampled dBTP
        float elapsedSeconds = 0.0f;
    };

    // Read a coherent copy of the Learn panel (message thread / editor timer).
    LearnSnapshot readLearnSnapshot() const noexcept;

    juce::AudioProcessorValueTreeState parameters;

    //==========================================================================
    // v1.3.0: the WebView UI language.
    //
    // 0 = en, 1 = fr. Held as an int index rather than the string it persists
    // as because std::atomic<juce::String> does not compile — juce::String is
    // not trivially copyable — so the audio-safe form is an index behind the
    // two-function codec below while the PERSISTED form stays a language code.
    //
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a preset must not be able to change which language
    // somebody reads their interface in. It rides the APVTS state tree as a
    // non-parameter property.
    //==========================================================================
    int  getUiLanguageIndex() const           { return uiLanguage.load(std::memory_order_acquire); }
    void setUiLanguageIndex(int i)            { uiLanguage.store(i, std::memory_order_release); }

    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

private:
    // v1.3.0: UI language index (0 = en, 1 = fr), saved with plugin state as
    // the language CODE string. See the codec above.
    std::atomic<int> uiLanguage { 0 };

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // =========================================================================
    // DSP Components
    // =========================================================================

    // Gain stage with built-in smoothing
    juce::dsp::Gain<float> gainStage;

    // VU meter ballistics (300ms attack/release, RMS mode)
    juce::dsp::BallisticsFilter<float> vuBallisticsL;
    juce::dsp::BallisticsFilter<float> vuBallisticsR;

    // =========================================================================
    // K-Weighting Filters (double precision, per channel)
    // =========================================================================

    // BS.1770 Stage 1: Pre-filter (high-shelf ~+4dB above 2kHz)
    juce::dsp::IIR::Filter<double> kWeightPreFilterL;
    juce::dsp::IIR::Filter<double> kWeightPreFilterR;

    // BS.1770 Stage 2: RLB high-pass (~100Hz rolloff)
    juce::dsp::IIR::Filter<double> kWeightRlbFilterL;
    juce::dsp::IIR::Filter<double> kWeightRlbFilterR;

    // =========================================================================
    // LUFS Gating Block Accumulator
    // =========================================================================

    int lufsBlockSize       = 0;     // 400ms in samples
    int lufsHopSize         = 0;     // 100ms in samples
    int lufsHopCounter      = 0;     // counts samples within current hop

    // Per-channel squared accumulation for current 400ms window
    // Using circular buffer approach: accumulate power in overlapping windows
    // We keep 4 sub-accumulators (each 100ms), rotate them to form 400ms blocks
    static constexpr int kNumSubBlocks = 4;
    double subBlockPowerL[kNumSubBlocks] = {};
    double subBlockPowerR[kNumSubBlocks] = {};
    int    subBlockSampleCount[kNumSubBlocks] = {};
    int    currentSubBlock  = 0;

    // All gating block loudness values (for integrated LUFS dual-gate)
    std::vector<double> gatingBlockPowers;  // mean power per 400ms block (pre-allocated)
    int gatingBlockCount    = 0;

    // Short-term buffer: last 30 blocks (3s at 100ms hop)
    static constexpr int kShortTermBlocks = 30;
    double shortTermPowers[kShortTermBlocks] = {};
    int    shortTermWritePos = 0;
    int    shortTermBlockCount = 0;

    // =========================================================================
    // Sample Peak Detection (WR-03: un-oversampled digital peak, dBFS not dBTP)
    // =========================================================================

    double samplePeakMax    = 0.0;   // running maximum absolute sample value

    // =========================================================================
    // RMS Accumulation (for learn mode RMS measurement)
    // =========================================================================

    double rmsAccumL        = 0.0;
    double rmsAccumR        = 0.0;
    long long rmsSampleCount = 0;

    // =========================================================================
    // Learn Mode Internal State
    // =========================================================================

    bool   prevLearnActive  = false;  // for edge detection
    int    learnMeasurementModeAtStart = 0;  // snapshot of measurement_mode at learn start
    int    learnChannelsAtStart = 2;  // snapshot of active channel count at learn start (1=mono, 2=stereo)
    double learnSampleCount = 0.0;    // total samples accumulated during learn

    // Set on the audio thread at the learn-stop edge; consumed on the message
    // thread via AsyncUpdater so the gain write / host notification never runs
    // on the real-time thread (see CR-01).
    std::atomic<bool> learnFinalizePending { false };

    // WR-04: throttle the (O(n)) running integrated-LUFS recompute to ~1 Hz so
    // the per-hop audio-thread cost stays bounded regardless of learn duration.
    int integratedHopCounter = 0;    // counts qualifying 400ms blocks during a learn

    // -------------------------------------------------------------------------
    // Learn-panel seqlock (WR-05)
    // -------------------------------------------------------------------------
    // Single-writer-at-a-time seqlock. Writers never truly overlap: the audio
    // thread only publishes while a learn is active; the message-thread paths
    // (finalizeLearn, parameterChanged/IN-02) only publish once a learn is idle
    // or complete — serialized by learnActive / learnDisplayState.
    std::atomic<uint32_t> learnSnapshotSeq  { 0 };   // even=stable, odd=write-in-progress
    LearnSnapshot         learnSnapshotData;          // guarded by learnSnapshotSeq
    std::atomic<int>      learnDisplayState { 0 };    // cheap mirror of snapshot.state (IN-02 gate)
    LearnSnapshot         liveLearn;                   // audio-thread working copy accumulated per hop

    // IN-02: guards finalizeLearn's own gain_offset write so its setValueNotifyingHost
    // does not immediately clear the just-set "DONE" state via parameterChanged.
    std::atomic<bool>     ignoreLearnGainWrite { false };

    void publishLearnSnapshot(const LearnSnapshot& s) noexcept;

    // Peak meter decay state
    float  inputPeakDecayL  = 0.0f;
    float  inputPeakDecayR  = 0.0f;
    float  outputPeakDecayL = 0.0f;
    float  outputPeakDecayR = 0.0f;

    // =========================================================================
    // Cached sample rate
    // =========================================================================

    double currentSampleRate = 44100.0;

    // =========================================================================
    // Helper Methods
    // =========================================================================

    // Initialize K-weight filter coefficients for a given sample rate
    void setupKWeightFilters(double sampleRate);

    // Reset all learn accumulators
    void resetLearnAccumulators();

    // Calculate integrated LUFS from accumulated gating blocks (with dual-gate)
    double calculateIntegratedLUFS() const;

    // Finalize learn: compute gain, write to APVTS. Runs on the MESSAGE THREAD only.
    void finalizeLearn();

    // AsyncUpdater callback: message-thread half of the learn-stop handoff (CR-01).
    void handleAsyncUpdate() override;

    // APVTS listener: clear the completed-Learn display when the user edits
    // gain_offset / trim (IN-02). Fires on whichever thread changed the param.
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OGainAudioProcessor)
};
