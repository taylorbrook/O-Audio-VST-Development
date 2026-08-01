/*
   This file is part of O-Texture, an Ouaricon Audio plugin.
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

    O-Texture - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <onnxruntime_cxx_api.h>

#include "DSP/OverlapAddProcessor.h"
#include "DSP/PerlinNoise1D.h"
#include "DSP/TiltFilter.h"

class TextureProcessor : public juce::AudioProcessor, private juce::Thread
{
public:
    TextureProcessor();
    ~TextureProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Texture"; }
    // MIDI is accepted but unused: an aumu MusicDevice MUST accept MIDI
    // (auval fails MusicDeviceMIDIEventList with NEEDS_MIDI_INPUT FALSE), so
    // the declaration is load-bearing for the AU type. See CODE_REVIEW IN-07.
    bool acceptsMidi() const override { return true; }
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

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // =========================================================================
    // Parameters (cached atomic pointers)
    // =========================================================================
    std::atomic<float>* sourceParam       = nullptr;
    std::atomic<float>* modeParam         = nullptr;
    std::atomic<float>* xParam            = nullptr;
    std::atomic<float>* yParam            = nullptr;
    std::atomic<float>* characterAParam   = nullptr;
    std::atomic<float>* characterBParam   = nullptr;
    std::atomic<float>* evolveParam       = nullptr;
    std::atomic<float>* freezeParam       = nullptr;
    std::atomic<float>* brightnessParam   = nullptr;
    std::atomic<float>* mixParam          = nullptr;

    // =========================================================================
    // Dimension map (loaded from BinaryData)
    // =========================================================================
    struct DimMap
    {
        int latentDim = 32;
        int xDim = 0;
        int yDim = 1;
        int charADim = 2;
        int charBDim = 3;
        std::vector<int> evolveDims;
        std::vector<int> inactiveDims;
    };

    DimMap dimMap;
    void loadDimMap();

    // =========================================================================
    // ONNX Runtime decoder inference (direct, synchronous)
    // =========================================================================
    static constexpr int kLatentDim = 32;
    static constexpr int kBlockSize = 4096;
    static constexpr int kHopSize   = 2048;

    Ort::Env ortEnv { ORT_LOGGING_LEVEL_WARNING, "OTexture" };
    Ort::MemoryInfo memoryInfo { nullptr }; // created once in initDecoderSession()
    std::unique_ptr<Ort::Session> decoderSession;

    void initDecoderSession();
    bool runDecoder(const float* latent, float* outputAudio);

    // =========================================================================
    // Background inference (audio thread never runs the decoder in realtime).
    // Protocol: audio thread writes reqLatentL/R then bumps reqSeq; the thread
    // decodes into asyncDecodedL/R and acknowledges via doneSeq. At most one
    // request is in flight; the audio thread only writes the request slots when
    // idle and only reads the response slots when doneSeq == reqSeq.
    // =========================================================================
    void run() override;

    std::array<float, 32> reqLatentL{};
    std::array<float, 32> reqLatentR{};
    std::vector<float> asyncDecodedL;
    std::vector<float> asyncDecodedR;
    std::atomic<uint32_t> reqSeq { 0 };
    std::atomic<uint32_t> doneSeq { 0 };
    uint32_t consumedSeq = 0;      // audio-thread local
    bool asyncDecodeOk = false;    // ordered by doneSeq release/acquire
    std::atomic<bool> decodeError { false };

    // =========================================================================
    // Latent vector state
    // =========================================================================
    std::array<float, 32> latentL{};
    std::array<float, 32> latentR{};

    // Full decoded block buffers (4096 samples each) — offline-render path only
    std::vector<float> decodedBufferL;
    std::vector<float> decodedBufferR;

    void constructLatentVectors(float x, float y, float charA, float charB,
                                float evolveRate, bool freeze);

    // =========================================================================
    // Evolve noise modulation
    // =========================================================================
    PerlinNoise1D<28> evolveNoise;

    // Host→audio staging for restored noise state: setStateInformation writes
    // under the lock, processBlock applies via try-lock (never blocks audio).
    juce::SpinLock noiseStagingLock;
    std::atomic<bool> noiseStagingPending { false };
    uint32_t stagedSeed = 0;
    bool stagedHasSeed = false;
    std::array<float, 28> stagedCursors{};
    bool stagedHasCursors = false;

    // Audio→host snapshot for getStateInformation, published per hop via
    // try-lock so the audio thread never blocks.
    juce::SpinLock noiseSnapshotLock;
    uint32_t snapshotSeed = 0;
    std::array<float, 28> snapshotCursors{};

    // Fixed per-seed values for inactive latent dims — drawn once per seed so
    // FREEZE holds still and a saved project reproduces its texture.
    std::array<float, 32> inactiveValues{};
    void regenerateInactiveValues(uint32_t seed);

    // =========================================================================
    // Overlap-add processor
    // =========================================================================
    OverlapAddProcessor olaProcessor;
    bool needsNewBlock = true;

    // =========================================================================
    // Tilt filter (brightness)
    // =========================================================================
    TiltFilter tiltFilter;

    // =========================================================================
    // Stereo decorrelation offset
    // =========================================================================
    static constexpr float kStereoOffset = 0.1f;

    // =========================================================================
    // State
    // =========================================================================
    double currentSampleRate = 48000.0;
    bool prepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureProcessor)
};
