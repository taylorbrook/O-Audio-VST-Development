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

    O-Gain - Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 3 (GUI) - WebView UI Implementation

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OGainAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit OGainAudioProcessorEditor(OGainAudioProcessor&);
    ~OGainAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Timer callback for metering updates
    void timerCallback() override;

    OGainAudioProcessor& processorRef;

    //==========================================================================
    // CRITICAL MEMBER DECLARATION ORDER
    // Order: Relays -> WebView -> Attachments
    // Members destroyed in REVERSE order
    // Attachments call evaluateJavascript() during destruction
    // Therefore attachments MUST be destroyed BEFORE webView
    //==========================================================================

    // 1. RELAYS FIRST (no dependencies)
    // Float parameters
    std::unique_ptr<juce::WebSliderRelay> gainOffsetRelay;
    std::unique_ptr<juce::WebSliderRelay> trimRelay;
    std::unique_ptr<juce::WebSliderRelay> targetLevelRelay;
    // Choice parameters
    std::unique_ptr<juce::WebComboBoxRelay> measurementModeRelay;
    std::unique_ptr<juce::WebComboBoxRelay> meterModeRelay;
    std::unique_ptr<juce::WebComboBoxRelay> msModeRelay;
    // Bool parameters
    std::unique_ptr<juce::WebToggleButtonRelay> phaseInvertLRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> phaseInvertRRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> channelSwapRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> monoSumRelay;

    // 2. WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on both relays and webView)
    // Float attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> gainOffsetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> trimAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> targetLevelAttachment;
    // Choice attachments
    std::unique_ptr<juce::WebComboBoxParameterAttachment> measurementModeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> meterModeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> msModeAttachment;
    // Bool attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> phaseInvertLAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> phaseInvertRAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> channelSwapAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> monoSumAttachment;

    //==========================================================================
    // Resource provider for WebView
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OGainAudioProcessorEditor)
};
