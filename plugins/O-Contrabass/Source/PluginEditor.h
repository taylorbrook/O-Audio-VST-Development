/*
   This file is part of O-Contrabass, an Ouaricon Audio plugin.
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

    O-Contrabass — WebView Editor (BOILERPLATE from finalized mockup v1)

    Stage 3 Phase 3.x: this replaces Source/PluginEditor.h. Adapt as needed
    during Stage 3 execute — but the MEMBER ORDER contract below is LOCKED.

    31 parameters (parameter-spec.md authoritative):
      29 × WebSliderRelay        (all Float/Int params incl. ACTIVE_STRINGS)
       1 × WebComboBoxRelay      (TUNING_SYSTEM)
       1 × WebToggleButtonRelay  (NOTE_EXPRESSION)

    RENDER-HARNESS NOTE: this header uses WebView types. The Stage-2 render
    harness compiles under JUCE_WEB_BROWSER=0 — PluginEditor.cpp must be
    DROPPED from tests/render-harness/CMakeLists.txt sources and
    createEditor() in PluginProcessor.cpp guarded with #if JUCE_WEB_BROWSER
    (see v1-integration-checklist.md §0).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OContrabassAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit OContrabassAudioProcessorEditor(OContrabassAudioProcessor&);
    ~OContrabassAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Resource provider — receives BARE PATHS ("/", "/index.html",
    // "/js/juce/index.js"), NEVER full URLs. Compare with direct equality;
    // do NOT strip scheme/host (fromFirstOccurrenceOf("://") on a bare path
    // returns "" and every lookup fails → "Frame load interrupted").
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // Async Scala file picker (native fn "openTuningFilePicker"; D1: .scl only —
    // TuningEngine 2.1.0 has no TUN parser). SafePointer + bare-return-on-null
    // pattern — see .cpp for the UAF contract.
    void launchTuningFilePicker(juce::WebBrowserComponent::NativeFunctionCompletion completion);

    OContrabassAudioProcessor& processorRef;

    // Owns the in-flight async chooser (must outlive launchAsync). Shared by
    // the footer Scala picker, tuning-panel file fns and preset dialogs.
    // fileDialogOpen guards re-entry: launching a second chooser would replace
    // (destroy) the first mid-flight (O-Wind pattern).
    std::shared_ptr<juce::FileChooser> fileChooser;
    bool fileDialogOpen = false;

    // ========================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER: Relays → WebView → Attachments ⚠️
    // Members are destroyed in REVERSE declaration order. Attachments call
    // evaluateJavascript() during destruction, so they MUST die before the
    // WebView. Wrong order = release-build crash on plugin reload.
    // ========================================================================

    // 1️⃣ RELAYS FIRST (no dependencies) — 31 total
    // ---- I · Bow (5) ----
    std::unique_ptr<juce::WebSliderRelay> bowSpeedRelay;          // BOW_SPEED
    std::unique_ptr<juce::WebSliderRelay> bowPressureRelay;       // BOW_PRESSURE
    std::unique_ptr<juce::WebSliderRelay> bowPositionRelay;       // BOW_POSITION
    std::unique_ptr<juce::WebSliderRelay> rosinRelay;             // ROSIN
    std::unique_ptr<juce::WebSliderRelay> bowNoiseRelay;          // BOW_NOISE
    std::unique_ptr<juce::WebSliderRelay> releaseRelay;           // RELEASE (v1.3)
    // ---- II · Body (4) ----
    std::unique_ptr<juce::WebSliderRelay> bodySizeRelay;          // BODY_SIZE
    std::unique_ptr<juce::WebSliderRelay> bodyDampingRelay;       // BODY_DAMPING
    std::unique_ptr<juce::WebSliderRelay> bodyMixRelay;           // BODY_MIX
    std::unique_ptr<juce::WebSliderRelay> brightnessRelay;        // BRIGHTNESS
    // ---- III · Strings (7) ----
    std::unique_ptr<juce::WebSliderRelay> stringTensionRelay;     // STRING_TENSION
    std::unique_ptr<juce::WebSliderRelay> stringStiffnessRelay;   // STRING_STIFFNESS
    std::unique_ptr<juce::WebSliderRelay> activeStringsRelay;     // ACTIVE_STRINGS (Int 1-4)
    std::unique_ptr<juce::WebSliderRelay> detuneERelay;           // DETUNE_E
    std::unique_ptr<juce::WebSliderRelay> detuneARelay;           // DETUNE_A
    std::unique_ptr<juce::WebSliderRelay> detuneDRelay;           // DETUNE_D
    std::unique_ptr<juce::WebSliderRelay> detuneGRelay;           // DETUNE_G
    // ---- IV · Expression (6) ----
    std::unique_ptr<juce::WebSliderRelay> expressionMacroRelay;   // EXPRESSION_MACRO
    std::unique_ptr<juce::WebSliderRelay> vibratoRateRelay;       // VIBRATO_RATE
    std::unique_ptr<juce::WebSliderRelay> vibratoDepthRelay;      // VIBRATO_DEPTH
    std::unique_ptr<juce::WebSliderRelay> vibratoOnsetRelay;      // VIBRATO_ONSET
    std::unique_ptr<juce::WebSliderRelay> slowLfoRateRelay;       // SLOW_LFO_RATE
    std::unique_ptr<juce::WebSliderRelay> slowLfoDepthRelay;      // SLOW_LFO_DEPTH
    // ---- V · Drone (2) ----
    std::unique_ptr<juce::WebSliderRelay> infiniteSustainRelay;   // INFINITE_SUSTAIN
    std::unique_ptr<juce::WebSliderRelay> subHarmonicsRelay;      // SUB_HARMONICS
    // ---- VI · Output (4) ----
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;        // OUTPUT_GAIN
    std::unique_ptr<juce::WebSliderRelay> widthRelay;             // WIDTH
    std::unique_ptr<juce::WebSliderRelay> masterSatRelay;         // MASTER_SAT_AMOUNT
    std::unique_ptr<juce::WebSliderRelay> limiterCeilingRelay;    // LIMITER_CEILING_DB
    // ---- VII · Microtonal (3) ----
    std::unique_ptr<juce::WebSliderRelay> referencePitchRelay;        // REFERENCE_PITCH
    std::unique_ptr<juce::WebComboBoxRelay> tuningSystemRelay;        // TUNING_SYSTEM
    std::unique_ptr<juce::WebToggleButtonRelay> noteExpressionRelay;  // NOTE_EXPRESSION

    // 2️⃣ WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on relays AND webView) — 31 total
    // ---- I · Bow (5) ----
    std::unique_ptr<juce::WebSliderParameterAttachment> bowSpeedAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bowPressureAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bowPositionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> rosinAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bowNoiseAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseAttachment;
    // ---- II · Body (4) ----
    std::unique_ptr<juce::WebSliderParameterAttachment> bodySizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bodyDampingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bodyMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> brightnessAttachment;
    // ---- III · Strings (7) ----
    std::unique_ptr<juce::WebSliderParameterAttachment> stringTensionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> stringStiffnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> activeStringsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> detuneEAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> detuneAAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> detuneDAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> detuneGAttachment;
    // ---- IV · Expression (6) ----
    std::unique_ptr<juce::WebSliderParameterAttachment> expressionMacroAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoOnsetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> slowLfoRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> slowLfoDepthAttachment;
    // ---- V · Drone (2) ----
    std::unique_ptr<juce::WebSliderParameterAttachment> infiniteSustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> subHarmonicsAttachment;
    // ---- VI · Output (4) ----
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> widthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> masterSatAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> limiterCeilingAttachment;
    // ---- VII · Microtonal (3) ----
    std::unique_ptr<juce::WebSliderParameterAttachment> referencePitchAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> tuningSystemAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> noteExpressionAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OContrabassAudioProcessorEditor)
};
