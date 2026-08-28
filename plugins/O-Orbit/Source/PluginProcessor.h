/*
   This file is part of O-Orbit, an Ouaricon Audio plugin.
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
#include <OuariconPresetManager.h>
#include "DSP/MotionEngine.h"
#include "DSP/VBAPRenderer.h"
#include "DSP/DistanceModel.h"
#include "DSP/VBAPDataExchange.h"
#include "DSP/DownmixEngine.h"
#include "Data/SpeakerLayout.h"
#include "Data/SpeakerPresets.h"

class OOrbitProcessor : public juce::AudioProcessor,
                        public juce::AsyncUpdater
{
public:
    OOrbitProcessor();
    ~OOrbitProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void handleAsyncUpdate() override;

    juce::AudioProcessorValueTreeState parameters;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Preset persistence (B1, v1.1.0): shared preset-manager module. The 12
    // factory presets from getFactoryPresets() are registered (skew-aware,
    // via convertTo0to1) in the constructor; the legacy Programs API is
    // collapsed to a single program like the rest of the suite.
    OuariconPresetManager presetManager { parameters, "Ouaricon Orbit" };

    // Factory preset name → category label, in authored (narrative) order.
    // Sole data source for the editor's getPresetListGrouped native fn.
    std::vector<std::pair<juce::String, juce::String>> factoryCategoryOrder;

    // Hover-help preference (B2, v1.1.0). Not a parameter: not automatable,
    // not part of the sound. Rides the APVTS tree as a plain property in
    // get/setStateInformation; the editor PULLS it at page init.
    std::atomic<bool> tooltipsEnabled { false };

    // ------------------------------------------------------------------------
    // Interface language (v1.2.0). 0 = en, 1 = fr.
    //
    // An INDEX rather than a string because std::atomic<juce::String> does not
    // compile — juce::String is not trivially copyable — so the audio-safe form
    // is an index behind the two-function codec below while the PERSISTED form
    // stays a language code.
    //
    // Deliberately NOT an AudioParameterChoice, for the same two reasons
    // tooltipsEnabled is not one: it must not appear in a DAW automation lane,
    // and a preset must not be able to change which language somebody reads
    // their interface in. It rides the same APVTS tree as a plain property
    // beside the toggle it belongs with, and the JSON preset path never touches
    // it. The editor PULLS it at page init; nothing pushes.
    // ------------------------------------------------------------------------
    std::atomic<int> uiLanguage { 0 };

    /** The codec. languageIndex() maps anything that is not "fr" to 0, so a
        hand-edited session or an unexpected argument from the page degrades to
        English rather than being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

    // UI motion snapshot (written by audio thread, read by UI timer)
    std::atomic<float> uiAzimuthL  { 0.0f };
    std::atomic<float> uiElevationL { 0.0f };
    std::atomic<float> uiAzimuthR  { 0.0f };
    std::atomic<float> uiElevationR { 0.0f };
    std::atomic<float> uiDistance   { 1.0f };

    float getUIAzimuthL() const   { return uiAzimuthL.load (std::memory_order_relaxed); }
    float getUIElevationL() const { return uiElevationL.load (std::memory_order_relaxed); }
    float getUIAzimuthR() const   { return uiAzimuthR.load (std::memory_order_relaxed); }
    float getUIElevationR() const { return uiElevationR.load (std::memory_order_relaxed); }
    float getUIDistance() const   { return uiDistance.load (std::memory_order_relaxed); }

    const SpeakerLayout& getCurrentLayout() const { return currentLayout; }
    const DownmixEngine& getDownmixEngine() const { return downmixEngine; }

    // Speaker layout modification (called from message thread via native functions)
    void setCustomSpeakerLayout (const SpeakerLayout& layout);
    void addSpeakerToLayout (float azimuth, float elevation, float distance, const juce::String& label);
    void removeSpeakerFromLayout (int index);
    void moveSpeakerInLayout (int index, float azimuth, float elevation, float distance);
    bool isUsingCustomLayout() const { return useCustomLayout; }

    // Named custom-layout library (D2, v1.1.0): JSON files (same schema as
    // export/import) in ~/Library/Ouaricon Orbit/Layouts/.
    juce::File getLayoutsDirectory() const;

    struct FactoryPreset
    {
        juce::String name;
        std::vector<std::pair<juce::String, float>> values;
    };

    static const std::vector<FactoryPreset>& getFactoryPresets();

private:
    static float shortestArc (float from, float to);
    static float wrapAngle (float angle);

    // Cached parameter pointers
    std::atomic<float>* pathParam        = nullptr;
    std::atomic<float>* speedParam       = nullptr;
    std::atomic<float>* widthParam       = nullptr;
    std::atomic<float>* depthParam       = nullptr;
    std::atomic<float>* tiltParam        = nullptr;
    std::atomic<float>* phaseParam       = nullptr;
    std::atomic<float>* elevEnableParam  = nullptr;
    std::atomic<float>* elevRangeParam   = nullptr;
    std::atomic<float>* tempoSyncParam   = nullptr;

    std::atomic<float>* speakerLayoutParam = nullptr;
    std::atomic<float>* distanceParam      = nullptr;
    std::atomic<float>* airAbsorptionParam = nullptr;
    std::atomic<float>* attenCurveParam    = nullptr;
    std::atomic<float>* centerDivergeParam = nullptr;

    std::atomic<float>* sourceModeParam  = nullptr;
    std::atomic<float>* lrOffsetParam    = nullptr;
    std::atomic<float>* mixParam         = nullptr;

    // DSP components
    MotionEngine  motionEngine;
    VBAPRenderer  vbapRenderer;
    DistanceModel distanceModel;
    DistanceModel distanceModelR;  // For L+R Split mode, R channel

    // Thread-safe VBAP gain table exchange
    VBAPDataExchange vbapExchange;
    VBAPComputeThread vbapThread { vbapExchange };

    // Auto-downmix engine
    DownmixEngine downmixEngine;

    // Buffers
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> spatialBuffer;  // Multi-channel intermediate

    // Per-speaker gain smoothing (max 24 speakers)
    std::array<float, 24> previousGains {};
    std::array<float, 24> currentGains {};
    std::array<float, 24> previousGainsR {};  // For L+R Split R source
    std::array<float, 24> currentGainsR {};

    // Layout change detection
    int lastSpeakerLayoutIndex = -1;
    SpeakerLayout currentLayout;
    int layoutNumSpeakers = 2;

    // Custom layout support
    bool useCustomLayout = false;
    SpeakerLayout customLayout;
    void applyLayout (const SpeakerLayout& layout);
    void applyLayoutOnAudioThread (const SpeakerLayout& layout);

    // Thread-safe pending layout (message thread writes, audio thread reads)
    juce::SpinLock pendingLayoutLock;
    SpeakerLayout pendingLayout;
    std::atomic<bool> layoutPending { false };

    // Smoothed values (mix is the only per-sample smoothed parameter; motion
    // params are phase-integrated and VBAP gains interpolate per block already)
    juce::SmoothedValue<float> mixSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OOrbitProcessor)
};
