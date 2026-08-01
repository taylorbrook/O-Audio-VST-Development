/*
   This file is part of O-Tremolo, an Ouaricon Audio plugin.
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

    OuariconTremolo - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OuariconTremoloAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OuariconTremoloAudioProcessorEditor(OuariconTremoloAudioProcessor&);
    ~OuariconTremoloAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    OuariconTremoloAudioProcessor& processorRef;

    // CRITICAL: Member declaration order (Pattern #11)
    // 1. Relays FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> speedRelay;
    std::unique_ptr<juce::WebSliderRelay> depthRelay;
    std::unique_ptr<juce::WebSliderRelay> waveformRelay;
    std::unique_ptr<juce::WebSliderRelay> smoothingRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> panSyncRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> tempoSyncRelay;
    std::unique_ptr<juce::WebComboBoxRelay> syncDivisionRelay;

    // 2. WebView SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Navigation state (per-instance, not static)
    bool hasNavigated = false;

    // 3. Attachments LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> speedAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> depthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> waveformAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> smoothingAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> panSyncAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> tempoSyncAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> syncDivisionAttachment;

    // Resource provider helper
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // File chooser for loading presets (v1.3.0)
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconTremoloAudioProcessorEditor)
};
