/*
   This file is part of O-SpectralShaper, an Ouaricon Audio plugin.
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

    O-SpectralShaper - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "STFTProcessor.h"
#include "OuariconPresetManager.h"
#include <array>

class OSpectralShaperAudioProcessor : public juce::AudioProcessor
{
public:
    OSpectralShaperAudioProcessor();
    ~OSpectralShaperAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-SpectralShaper"; }
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

    // Public APVTS access for editor
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Preset manager (public for editor native function access)
    OuariconPresetManager presetManager;

    // Narrative category order for the preset menu (v1.6.0). Built once in the
    // constructor from the categorySpans table over the factory bank's
    // DECLARATION order; consumed by the editor's getPresetListGrouped native
    // function. Pairs of { preset name, category label }.
    std::vector<std::pair<juce::String, juce::String>> factoryCategoryOrder;

    // Curve access for UI (thread-safe via STFTProcessor)
    const std::array<float, 32>& getAttackCurve() const { return attackCurve; }
    const std::array<float, 32>& getSustainCurve() const { return sustainCurve; }
    void setAttackCurve(const std::array<float, 32>& curve);
    void setSustainCurve(const std::array<float, 32>& curve);

    // Bumped only when the curves are replaced from STATE (preset load /
    // session restore), never by the setters above — the UI's own drag-edits
    // must not echo back into the curve editors mid-drag. The editor polls
    // this from its timer and re-sends both curves to the WebView on change.
    juce::uint32 getCurvesRevision() const { return curvesRevision.load(std::memory_order_acquire); }

    // Visualization data structures (Phase 3.3)
    struct VisualizationFrame
    {
        std::array<float, STFTProcessor::NUM_BINS> fftMagnitudes {};
        std::array<float, STFTProcessor::NUM_BANDS> transientActivity {};
    };

    // Visualization FIFO access (read from GUI thread)
    juce::AbstractFifo& getVisualizationFifo() { return visualizationFifo; }
    const std::vector<VisualizationFrame>& getVisualizationBuffer() const { return visualizationBuffer; }

    // Tooltip preference (v1.5.0). A UI-only setting, not an APVTS parameter:
    // it must not be automatable, and it is deliberately kept out of preset
    // files so loading a preset never changes the user's help preference.
    bool getTooltipsEnabled() const { return tooltipsEnabled.load(std::memory_order_acquire); }
    void setTooltipsEnabled(bool enabled) { tooltipsEnabled.store(enabled, std::memory_order_release); }

private:
    // DSP Components (declared BEFORE parameters for correct initialization order)
    STFTProcessor stftProcessor[2];  // L/R stereo
    juce::AudioBuffer<float> dryDelayBuffer;  // 512-sample latency matching
    int dryDelayWritePosition = 0;

    // Lookahead buffer (optional, toggleable via parameter)
    juce::AudioBuffer<float> lookaheadBuffer;
    int lookaheadWritePosition = 0;
    int lookaheadDelayLength = 0;  // In samples
    bool lookaheadEnabled = false;

    // WR-03: last latency value signalled to the host. Guards setLatencySamples() so it
    // is only called when the reported latency actually changes, instead of every block
    // from the audio thread (which many hosts glitch on or ignore).
    int lastReportedLatency = -1;

    // Curve arrays (saved/loaded with plugin state)
    std::array<float, 32> attackCurve {};
    std::array<float, 32> sustainCurve {};
    std::atomic<juce::uint32> curvesRevision { 0 };

    // Sample rate (cached for lookahead calculation)
    double currentSampleRate = 44100.0;

    // Visualization FIFO (Phase 3.3)
    juce::AbstractFifo visualizationFifo { 60 };  // 60 frames buffered (~1 second at 60fps)
    std::vector<VisualizationFrame> visualizationBuffer { 60 };
    int hopCounter = 0;  // Track when to push frames (once per FFT hop)

    // APVTS (declared AFTER DSP components)
    juce::AudioProcessorValueTreeState parameters;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Cached parameter pointers (initialized in constructor, avoids string lookup per processBlock)
    std::atomic<float>* cachedMix = nullptr;
    std::atomic<float>* cachedSensitivity = nullptr;
    std::atomic<float>* cachedAttackTime = nullptr;
    std::atomic<float>* cachedSustainTime = nullptr;
    std::atomic<float>* cachedLookaheadEnabled = nullptr;
    std::atomic<float>* cachedLookaheadTime = nullptr;
    std::atomic<float>* cachedOutputGain = nullptr;

    // Tooltip preference (v1.5.0). Written from the message thread via the
    // WebView native function, read back when the editor reopens.
    std::atomic<bool> tooltipsEnabled { false };

    // Helper methods
    float getDryDelayedSample(int channel, float input);
    void advanceDryDelay();
    float getLookaheadDelayedSample(int channel, float input);
    void advanceLookahead();
    void writeVisualizationFrame(const VisualizationFrame& frame);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSpectralShaperAudioProcessor)
};
