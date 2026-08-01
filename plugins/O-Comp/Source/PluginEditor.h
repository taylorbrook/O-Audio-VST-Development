/*
   This file is part of O-Comp, an Ouaricon Audio plugin.
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

    O-Comp - Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 3 (GUI) - WebView UI Implementation

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OCompAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit OCompAudioProcessorEditor(OCompAudioProcessor&);
    ~OCompAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Timer callback for metering updates
    void timerCallback() override;

    OCompAudioProcessor& processorRef;

    // File chooser for preset dialogs (must persist during async operation)
    std::unique_ptr<juce::FileChooser> fileChooser;

    //==========================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    // Order: Relays → WebView → Attachments
    // Members destroyed in REVERSE order
    // Attachments call evaluateJavascript() during destruction
    // Therefore attachments MUST be destroyed BEFORE webView
    //==========================================================================

    // 1️⃣ RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> thresholdRelay;
    std::unique_ptr<juce::WebSliderRelay> ratioRelay;
    std::unique_ptr<juce::WebSliderRelay> attackTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> releaseTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> kneeRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> autoGainRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> thresholdAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> ratioAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> kneeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> autoGainAttachment;

    //==========================================================================
    // Resource provider for WebView
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OCompAudioProcessorEditor)
};
