/*
   This file is part of O-Freeze, an Ouaricon Audio plugin.
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

    O-Freeze - Editor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OFreezeAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OFreezeAudioProcessorEditor(OFreezeAudioProcessor&);
    ~OFreezeAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    OFreezeAudioProcessor& processorRef;

    // CRITICAL MEMBER ORDER: relays → webView → attachments
    // Members destroyed in REVERSE order - wrong order causes release build crashes

    // 1. RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> thresholdRelay;
    std::unique_ptr<juce::WebSliderRelay> driftRelay;
    std::unique_ptr<juce::WebSliderRelay> grainSizeRelay;
    std::unique_ptr<juce::WebSliderRelay> grainCountRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebSliderRelay> detuneRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> freezeRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> reverseRelay;
    std::unique_ptr<juce::WebComboBoxRelay> modeRelay;
    std::unique_ptr<juce::WebSliderRelay> lfoRateRelay;
    std::unique_ptr<juce::WebSliderRelay> lfoDepthRelay;
    std::unique_ptr<juce::WebComboBoxRelay> lfoShapeRelay;

    // 2. WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. NAVIGATION FLAG
    bool hasNavigated = false;

    // 4. ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> thresholdAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> driftAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> grainSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> grainCountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> detuneAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> freezeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> reverseAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> modeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfoRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfoDepthAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> lfoShapeAttachment;

    // Helper for resource serving
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OFreezeAudioProcessorEditor)
};
