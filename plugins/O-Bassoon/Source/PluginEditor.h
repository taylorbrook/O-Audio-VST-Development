/*
   This file is part of O-Bassoon, an Ouaricon Audio plugin.
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

    O-Bassoon - Editor (Stage 3 WebView UI)
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 3: replaces Stage 1 GenericAudioProcessorEditor placeholder with a
    JUCE 8 WebBrowserComponent that hosts the Ouaricon-botanical HTML UI
    (Resources/ui/index.html). Binds 10 APVTS parameters via WebSliderRelay +
    WebSliderParameterAttachment (3-arg ctor, nullptr undoManager). Embeds
    shared tuning-panel.{js,css} via Pattern A (CMake direct reference, not
    per-plugin copy). 30 Hz Timer pushes 3 live-feedback channels to JS:
    activeVoiceCount, effectiveBreath, vibratoEnvelope.

    CRITICAL member declaration order: Relays -> WebView -> Attachments.
    Members destroyed in REVERSE order; attachments may call evaluateJavascript()
    during destruction, so WebView must outlive them.

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

class OBassoonAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit OBassoonAudioProcessorEditor (OBassoonAudioProcessor& p);
    ~OBassoonAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    OBassoonAudioProcessor& processorRef;

    // ===================================================================
    // CRITICAL ORDER: Relays -> WebView -> Attachments
    // (See header comment for reasoning. Memory-pinned regression sentinel.)
    // ===================================================================

    // 1. RELAYS FIRST (no dependencies) — 10 slider relays for 10 APVTS params
    std::unique_ptr<juce::WebSliderRelay> vibratoRateRelay;
    std::unique_ptr<juce::WebSliderRelay> vibratoDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> vibratoOnsetRelay;
    std::unique_ptr<juce::WebSliderRelay> breathRelay;
    std::unique_ptr<juce::WebSliderRelay> toneRelay;
    std::unique_ptr<juce::WebSliderRelay> attackCharacterRelay;
    std::unique_ptr<juce::WebSliderRelay> attackTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> releaseTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> voiceCountRelay;     // AudioParameterInt — still slider relay (#19)
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;

    // 2. WEBVIEW SECOND (depends on relays via .withOptionsFrom())
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on both relays and webView; 3-arg ctor #12)
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoOnsetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> breathAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> toneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackCharacterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> voiceCountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;

    // File chooser for tuning .scl/.kbm/.html dialogs (must be shared_ptr per JUCE 8 async API)
    std::shared_ptr<juce::FileChooser> fileChooser;

    // Push-channel diff suppression (Phase 3.2 — sentinels force first-tick emit)
    int   lastEmittedActive  { -1 };
    float lastEmittedBreath  { -1.0f };
    float lastEmittedVibEnv  { -1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OBassoonAudioProcessorEditor)
};
