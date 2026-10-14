/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    O-simpleAmbisonics - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include <atomic>
#include <vector>

#include "dsp/SpatialEngine.h"
#include "dsp/MeterFrame.h"
#include "dsp/TripleBuffer.h"

namespace OSimpleAmbisonics::ParamIDs
{
    // The 12-parameter v1.0 contract (research/ARCHITECTURE.md §Parameter Mapping).
    // IDs are frozen: a range change after 1.0.0 needs a preset-migration gate.
    inline constexpr auto azimuth     = "azimuth";
    inline constexpr auto elevation   = "elevation";
    inline constexpr auto distance    = "distance";
    inline constexpr auto order       = "order";
    inline constexpr auto yaw         = "yaw";
    inline constexpr auto pitch       = "pitch";
    inline constexpr auto roll        = "roll";
    inline constexpr auto decoder     = "decoder";
    inline constexpr auto orbit       = "orbit";
    inline constexpr auto orbitRate   = "orbitRate";
    inline constexpr auto inputMode   = "inputMode";
    inline constexpr auto outputLevel = "outputLevel";
}

class OSimpleAmbisonicsAudioProcessor : public juce::AudioProcessor
{
public:
    OSimpleAmbisonicsAudioProcessor();
    ~OSimpleAmbisonicsAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    // Public APVTS member for direct UI / harness access (house pattern).
    juce::AudioProcessorValueTreeState apvts;

    // Visualisation tap (ARCHITECTURE §Visualization Tap). Public like apvts
    // so the Stage-3 editor Timer and the harness read() it directly. One
    // publish() per host block from the audio thread; read() from exactly
    // one thread (the message thread / the harness thread).
    OSimpleAmbisonics::dsp::TripleBuffer<OSimpleAmbisonics::dsp::MeterFrame> meterFrames;

    //==========================================================================
    // INTERFACE LANGUAGE (Stage 3) — the WebView page's own language.
    //
    // Deliberately NOT a parameter: it must not reach an automation lane, and a
    // lesson must not change which language somebody reads. It rides the state
    // XML as a plain PROPERTY written as the ASCII code ("en" / "fr" /
    // "zh-Hans"), restored only when present (isVoid() — the XML round-trip
    // rebuilds every property as a string var,
    // critical_valuetree_xml_roundtrip_loses_type). The editor PULLS it once at
    // page init (getUiLanguage native); nothing pushes. O-simpleAdditive shape.
    std::atomic<int> uiLanguage { 0 };   // 0 en, 1 fr, 2 zh-Hans

    /** languageIndex() maps anything that is neither "fr" nor "zh-Hans" to 0,
        so a hand-edited session or an unexpected page argument degrades to
        English instead of being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 2 ? "zh-Hans" : i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "zh-Hans" ? 2 : s == "fr" ? 1 : 0; }

    //==========================================================================
    // EDITOR SCALE (Stage 3, UI-07) — window width / 1180, PERSISTED in the
    // state XML (CONTEXT locked: "save / reopen keeps the window size"; this
    // overrides the siblings' session-only member and ARCHITECTURE "Custom
    // state: none"). 0 = never sized → the editor fits the display at open.
    // Written as "0.850" only when > 0 (removed from the saved copy otherwise),
    // restored only when finite and within 0.6 .. 1.5. Atomic: hosts may call
    // getStateInformation off the message thread.
    std::atomic<float> editorScale { 0.0f };

    //==========================================================================
    // LESSON TOUR (Stage 3, UI-08) — apply a lesson by STABLE id (pointSource,
    // orderStep, behindYou, overhead, rotateRoom, farNear, slowOrbit,
    // speakersHeadphones). One pass over the 12 parameters: recipe value or the
    // default; outputLevel is NEVER reset and NEVER set; a parameter already at
    // its target is skipped (no host edit, no gesture); every other one gets
    // begin / setValueNotifyingHost / end. Unknown id → false, nothing touched.
    // Message thread, APVTS only; the relays sync the page back.
    bool applyFactoryPreset (const juce::String& lessonId);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Engine Params from the cached atomics — once per host block, and once
    // in prepareToPlay BEFORE engine.prepare (A7).
    OSimpleAmbisonics::dsp::Params fillParams() const noexcept;

    // Cached raw-value atomics for all 12 parameters.
    std::atomic<float>* azimuthParam     = nullptr;
    std::atomic<float>* elevationParam   = nullptr;
    std::atomic<float>* distanceParam    = nullptr;
    std::atomic<float>* orderParam       = nullptr;
    std::atomic<float>* yawParam         = nullptr;
    std::atomic<float>* pitchParam       = nullptr;
    std::atomic<float>* rollParam        = nullptr;
    std::atomic<float>* decoderParam     = nullptr;
    std::atomic<float>* orbitParam       = nullptr;
    std::atomic<float>* orbitRateParam   = nullptr;
    std::atomic<float>* inputModeParam   = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;

    // The two stepped+skewed ranges, for snapping the raw value in fillParams
    // (see the JUCE legacy note there). Owned by the parameters; never null
    // after construction.
    const juce::NormalisableRange<float>* distanceRange  = nullptr;
    const juce::NormalisableRange<float>* orbitRateRange = nullptr;

    // Chain: conditioner (processor) → SpatialEngine (per slice) → seeded gain.
    OSimpleAmbisonics::dsp::SpatialEngine engine;
    juce::LinearSmoothedValue<float> outputGain;

    // Prepared-size scratch (sized in prepareToPlay only): the conditioned
    // mono slice, and a right-output sink for a 1-channel buffer.
    std::vector<float> monoScratch;
    std::vector<float> rightScratch;

    // Post-gain output peaks, peak-since-the-UI-last-read (A8 / R7).
    float outPeakAccL = 0.0f;
    float outPeakAccR = 0.0f;

    int numInputLegs = 2;   // negotiated input bus width (1 or 2), NOT the buffer width
    int maxBlock     = 0;   // samplesPerBlock from prepareToPlay; slice bound

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleAmbisonicsAudioProcessor)
};
