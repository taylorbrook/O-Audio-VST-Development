/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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
#include "PluginProcessor.h"

class OEmulatorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OEmulatorAudioProcessorEditor(OEmulatorAudioProcessor&);
    ~OEmulatorAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    OEmulatorAudioProcessor& audioProcessor;

    // ========================================================================
    // NATIVE-FUNCTION SURFACE — exactly 10 (Stage 4, preset-manager v1.0.6):
    // the ten names modules/preset-manager.js resolves. All five parameters
    // still ride relays; the info readout is a static JS table. The bridge
    // audit runs at 10<->10: withNativeFunction in PluginEditor.cpp (10) <->
    // getNativeFunction in the generated modules/preset-manager.js (10) +
    // index.html (0).
    // ========================================================================

    // ========================================================================
    // CRITICAL MEMBER DECLARATION ORDER: Relays -> WebView -> Attachments
    // Members destroyed in REVERSE declaration order (C++ standard).
    // Attachments call into the WebView on destruction, so they MUST die
    // first; the relays outlive both (juce8-critical-patterns #11, and the
    // proven O-Bitrot ordering).
    // ========================================================================

    // 1. RELAYS (no dependencies) — 5 total: 1 combo / 4 slider
    std::unique_ptr<juce::WebComboBoxRelay> consoleRelay;
    std::unique_ptr<juce::WebSliderRelay>   crushRelay;
    std::unique_ptr<juce::WebSliderRelay>   ageRelay;
    std::unique_ptr<juce::WebSliderRelay>   reverbRelay;
    std::unique_ptr<juce::WebSliderRelay>   mixRelay;

    // 2. WEBVIEW (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (depend on relays + webView)
    std::unique_ptr<juce::WebComboBoxParameterAttachment> consoleAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>   crushAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>   ageAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>   reverbAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>   mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OEmulatorAudioProcessorEditor)
};
