/*
   This file is part of O-Bass, an Ouaricon Audio plugin.
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

    O-Bass - WebView Editor
    Ouaricon Audio
    Developer: Taylor Brook

    WebView-based editor with botanical aesthetic and JUCE 8 parameter binding.

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OBassAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit OBassAudioProcessorEditor(OBassAudioProcessor&);
    ~OBassAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    void timerCallback() override;
    OBassAudioProcessor& processorRef;

    // CRITICAL: Member declaration order (Pattern #11 from RESEARCH.md)
    // 1. Relays FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> frequencyRelay;
    std::unique_ptr<juce::WebSliderRelay> enhanceRelay;
    std::unique_ptr<juce::WebSliderRelay> outputRelay;

    // 2. WebView SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Navigation state (per-instance, not static)
    bool hasNavigated = false;

    // 3. Attachments LAST (depend on both relays and parameters)
    std::unique_ptr<juce::WebSliderParameterAttachment> frequencyAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> enhanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputAttachment;

    // Resource provider helper
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // File chooser for preset load/save dialogs
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBassAudioProcessorEditor)
};
