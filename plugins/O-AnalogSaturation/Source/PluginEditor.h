/*
   This file is part of O-AnalogSaturation, an Ouaricon Audio plugin.
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

    O-AnalogSaturation - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class OAnalogSaturationAudioProcessorEditor : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    explicit OAnalogSaturationAudioProcessorEditor(OAnalogSaturationAudioProcessor&);
    ~OAnalogSaturationAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    OAnalogSaturationAudioProcessor& processorRef;

    // Member declaration order matters for destruction safety:
    // Relays first, then WebView, then Attachments (destroyed in reverse)
    std::unique_ptr<juce::WebSliderRelay> intensityRelay;
    std::unique_ptr<juce::WebSliderRelay> modelRelay;
    std::unique_ptr<juce::WebSliderRelay> qualityRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> autogainRelay;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebSliderParameterAttachment> intensityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> modelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> qualityAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> autogainAttachment;

    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OAnalogSaturationAudioProcessorEditor)
};
