/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    PluginEditor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PrismParamIds.h"

class OPrismAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit OPrismAudioProcessorEditor (OPrismAudioProcessor&);
    ~OPrismAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OPrismAudioProcessor& processorRef;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: Member declaration order (C++ destroys in reverse)
    // 1. Relays destroyed LAST
    // 2. WebView destroyed SECOND
    // 3. Attachments destroyed FIRST (WebView still alive — safe)
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS (destroyed last)
    std::vector<std::unique_ptr<juce::WebSliderRelay>> sliderRelays;
    std::unique_ptr<juce::WebToggleButtonRelay> delaySyncRelay;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> lfoSyncRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> lfoFreeRunRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> bypassRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> modSlotToggleRelays;

    // 2. WEBVIEW (destroyed second)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (destroyed first — WebView still alive)
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> sliderAttachments;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> delaySyncAttachment;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> lfoSyncAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> lfoFreeRunAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> bypassAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> modSlotToggleAttachments;

    // Resource provider for WebView
    std::optional<juce::WebBrowserComponent::Resource>
        getResource (const juce::String& url);

    // Native function registration
    juce::WebBrowserComponent::Options addNativeFunctions (
        juce::WebBrowserComponent::Options options);

    // Timer callback to push active notes to WebView for TrueKeys
    void timerCallback() override;
    std::vector<std::pair<int, double>> lastSentNotes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OPrismAudioProcessorEditor)
};
