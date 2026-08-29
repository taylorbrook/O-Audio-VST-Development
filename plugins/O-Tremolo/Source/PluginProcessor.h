/*
   This file is part of O-Tremolo, an Ouaricon Audio plugin.
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

    OuariconTremolo - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "OuariconPresetManager.h"

class OuariconTremoloAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconTremoloAudioProcessor();
    ~OuariconTremoloAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Tremolo"; }
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

    // Public access to parameters for editor
    juce::AudioProcessorValueTreeState parameters;

    // Preset management
    OuariconPresetManager presetManager;

    // Current host tempo (BPM), cached from the audio thread for the UI (IN-04).
    // Returns the last-seen host BPM, or the 120.0 default if the host never reported one.
    double getHostBpm() const { return hostBpm.load(); }

    // ------------------------------------------------------------------------
    // v1.7.0: THE UI LANGUAGE.
    //
    // Held as an atomic int because the editor's native functions read and write
    // it from the message thread while getStateInformation may run from another;
    // the PERSISTED form stays a readable language code ("en"/"fr") through the
    // two-function codec below.
    //
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a preset must not be able to change which language
    // somebody reads their plugin in. It rides the APVTS state tree as a
    // non-parameter property instead.
    // ------------------------------------------------------------------------
    std::atomic<int> uiLanguage { 0 };

    /** The codec. languageIndex() maps anything that is not "fr" to 0, so a
        hand-edited session or an unexpected argument from the page degrades to
        English rather than being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components (declared BEFORE parameters for initialization order)
    // LFO state
    float lfoPhase = 0.0f;
    float lfoPhaseIncrement = 0.0f;
    double currentSampleRate = 44100.0;

    // Smoothing filter state (separate for L/R when Pan Sync enabled)
    float smoothedLFO_L = 0.0f;
    float smoothedLFO_R = 0.0f;

    // Depth parameter smoothing (WR-01): depth scales the per-sample output gain,
    // so an un-smoothed step (automation / preset recall / UI drag) causes an audible
    // zipper/click. Ramped per-sample to eliminate it.
    juce::SmoothedValue<float> depthSmoothed;

    // Host tempo cached from the audio thread for the UI's tempo-sync readout (IN-04).
    // The WebView has no direct host access; getHostBpm() lets JS show the real division.
    std::atomic<double> hostBpm { 120.0 };

    // Random number generator for noise waveform
    juce::Random random;

    // Sample-and-hold state for noise waveform
    float noiseHeldValue = 0.0f;
    float noisePrevHeldValue = 0.0f;  // Previous value for smooth transitions
    int noiseLastQuarter = -1;

    // Cached parameter pointers (valid for processor lifetime)
    std::atomic<float>* speedParam = nullptr;
    std::atomic<float>* depthParam = nullptr;
    std::atomic<float>* waveformParam = nullptr;
    std::atomic<float>* smoothingParam = nullptr;
    std::atomic<float>* panSyncParam = nullptr;
    std::atomic<float>* tempoSyncParam = nullptr;
    std::atomic<float>* syncDivisionParam = nullptr;  // discrete musical division for synced rate (v1.6.0)

    // Helper methods
    float generateWaveform(float phase, int waveformType, float mainLfoPhase);
    float applySmoothingFilter(float rawLFO, float& prevSmoothed, float coefficient);
    float smoothTransition(float t);  // Polynomial smoothing for sharp transitions

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconTremoloAudioProcessor)
};
