/*
   This file is part of O-MultiBandCompressor, an Ouaricon Audio plugin.
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

    O-MultiBandCompressor - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include <map>
#include <vector>
#include "DSP/MultiBandProcessor.h"
#include "DSP/PhaseMatchChain.h"
#include "OuariconPresetManager.h"

// FFT Configuration
static constexpr int FFT_ORDER = 11;                     // 2^11 = 2048 samples
static constexpr int FFT_SIZE = 1 << FFT_ORDER;          // 2048
static constexpr int SPECTRUM_BINS = 64;                 // Bins sent to UI (downsampled)

class OMultiBandCompressorAudioProcessor : public juce::AudioProcessor
{
public:
    OMultiBandCompressorAudioProcessor();
    ~OMultiBandCompressorAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-MultiBandCompressor"; }
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

    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

    // Access to gain reduction meters for UI (all 4 bands)
    float getLowBandGainReduction() const { return lowBandGainReduction.load(std::memory_order_relaxed); }
    float getLoMidBandGainReduction() const { return loMidBandGainReduction.load(std::memory_order_relaxed); }
    float getHiMidBandGainReduction() const { return hiMidBandGainReduction.load(std::memory_order_relaxed); }
    float getHighBandGainReduction() const { return highBandGainReduction.load(std::memory_order_relaxed); }

    // Access to input/output level meters for UI (Phase 5.3)
    float getInputLevelL() const { return inputLevelL.load(std::memory_order_relaxed); }
    float getInputLevelR() const { return inputLevelR.load(std::memory_order_relaxed); }
    float getOutputLevelL() const { return outputLevelL.load(std::memory_order_relaxed); }
    float getOutputLevelR() const { return outputLevelR.load(std::memory_order_relaxed); }

    // FFT Spectrum data access (v1.2.0)
    void getSpectrumData(std::array<float, SPECTRUM_BINS>& dest) const;
    bool hasNewSpectrumData() const { return spectrumDataReady.load(std::memory_order_acquire); }
    void clearSpectrumDataFlag() { spectrumDataReady.store(false, std::memory_order_release); }

private:
    // DSP Components (BEFORE parameters for initialization order)
    juce::dsp::ProcessSpec spec;

    // Phase 4.2: Multiband processor (4-band crossover + compressors)
    MultiBandProcessor multibandProcessor;

    // Gain stages
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> outputGain;

    // Phase 4.3: Dry/wet mixer for parallel compression
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Gain reduction metering (atomic for thread-safe UI access)
    std::atomic<float> lowBandGainReduction { 0.0f };
    std::atomic<float> loMidBandGainReduction { 0.0f };
    std::atomic<float> hiMidBandGainReduction { 0.0f };
    std::atomic<float> highBandGainReduction { 0.0f };

    // Input/output level metering (Phase 5.3)
    std::atomic<float> inputLevelL { 0.0f };
    std::atomic<float> inputLevelR { 0.0f };
    std::atomic<float> outputLevelL { 0.0f };
    std::atomic<float> outputLevelR { 0.0f };

    // FFT Spectrum analyzer (v1.2.0)
    juce::dsp::FFT fft { FFT_ORDER };
    juce::dsp::WindowingFunction<float> fftWindow { FFT_SIZE, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, FFT_SIZE * 2> fftWorkBuffer {};    // FFT work buffer
    std::array<float, FFT_SIZE> fftInputFifo {};         // Sample accumulation
    int fftFifoWriteIndex = 0;
    std::atomic<bool> spectrumDataReady { false };

    // WR-01: lock-free triple-buffer hand-off for the spectrum snapshot. The audio thread
    // never blocks — it fills its private write slot and atomically swaps it into the
    // "ready" slot; the UI thread atomically claims the ready slot to read. Each of the
    // three slots is owned by exactly one role at a time, so writer and reader never touch
    // the same buffer.
    std::array<std::array<float, SPECTRUM_BINS>, 3> spectrumBuffers {};
    int spectrumWriteSlot = 0;                            // audio-thread private
    mutable int spectrumReadSlot = 1;                     // UI-thread private
    mutable std::atomic<int> spectrumReadySlot { 2 };     // published-slot hand-off

    // IN-06: log-spaced FFT bin edges for the UI spectrum (20 Hz – 20 kHz, matching the
    // crossover overlay's log axis). Edge k is the first FFT bin of output bin k.
    // Sample-rate dependent, so precomputed in prepareToPlay — never on the audio thread.
    std::array<int, SPECTRUM_BINS + 1> spectrumBinEdges {};

    // M/S scratch buffer, preallocated in prepareToPlay (CR-03) — avoids per-block AudioBuffer
    // allocation in Mid/Side modes.
    juce::AudioBuffer<float> msScratchBuffer;

    // v1.6.1 phase matching: the crossover's band sum carries AP(f1)·AP(f2)·AP(f3), so
    // anything summed/decoded against it needs the same rotation (see PhaseMatchChain.h).
    PhaseMatchChain dryPhaseMatch;          // dry path of the Mix control (WR-03)
    PhaseMatchChain passthroughPhaseMatch;  // uncompressed channel in Mid/Side modes (WR-02)
    juce::AudioBuffer<float> dryScratchBuffer;  // preallocated dry copy for phase matching

    // APVTS comes AFTER DSP components
    juce::AudioProcessorValueTreeState parameters;

public:
    // v1.5.0: preset persistence (~/Library/O-MultiBandCompressor/Presets/).
    // Declared immediately after `parameters` because members are constructed in
    // declaration order regardless of access specifier, and the manager stores a
    // reference to the APVTS — it must not be built before the APVTS exists.
    // Public so the editor can bind the WebView preset native functions to it.
    OuariconPresetManager presetManager;

    //==========================================================================
    // v1.7.0: preset categories.
    //
    // Held in this plugin's own preset table rather than in the shared
    // OuariconPresetManager, so adding categories here costs the other plugins
    // vendoring that module nothing. Public for the same reason presetManager is:
    // the editor binds a WebView native function straight to these.
    //==========================================================================

    /** Reported for any preset name absent from the factory table. Always sorts last. */
    static constexpr const char* kUserPresetCategory = "User";

    /**
     * Presets in this category are deliberately inert — every band sits at 1:1 and
     * measures zero gain reduction. The verification harness keys its "should do
     * nothing" check off this name, so an Init preset added later is covered without
     * the harness needing to learn its name.
     */
    static constexpr const char* kInertPresetCategory = "Init";

    /** Category for one preset, factory or user. Never returns an empty string. */
    juce::String getPresetCategory(const juce::String& presetName) const;

    /** Every category in browser display order, with kUserPresetCategory appended. */
    juce::StringArray getPresetCategoryOrder() const;

private:
    // Builds the v1.5.0 factory preset table. Values are authored in engineering
    // units and converted through each parameter's own NormalisableRange, so the
    // skew on ATTACK / RELEASE / XOVER* / SC_* is honoured.
    std::vector<OuariconPresetManager::FactoryPresetDef> buildFactoryPresets() const;

    // CR-02: raw parameter pointers resolved once in prepareToPlay so processBlock never
    // builds juce::String IDs or hashes the parameter map on the audio thread.
    enum BandParam
    {
        bpThreshold = 0, bpRatio, bpAttack, bpRelease, bpKnee, bpMakeup,
        bpPeakRms, bpBypass, bpSolo, bpScHPF, bpScLPF, bpScListen, numBandParams
    };
    std::atomic<float>* bandParamPtrs[4][numBandParams] {};
    std::atomic<float>* pInputGain = nullptr;
    std::atomic<float>* pOutputGain = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pAutoMakeup = nullptr;
    std::atomic<float>* pMsMode = nullptr;
    std::atomic<float>* pXover1 = nullptr;
    std::atomic<float>* pXover2 = nullptr;
    std::atomic<float>* pXover3 = nullptr;
    void cacheParameterPointers();

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OMultiBandCompressorAudioProcessor)
};
