/*
   This file is part of O-Texture, an Ouaricon Audio plugin.
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

    O-Texture - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class TextureEditor : public juce::AudioProcessorEditor
{
public:
    explicit TextureEditor(TextureProcessor&);
    ~TextureEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    TextureProcessor& processorRef;

    // ========================================================================
    // CRITICAL MEMBER DECLARATION ORDER: Relays -> WebView -> Attachments
    // Members destroyed in REVERSE declaration order (C++ standard)
    // ========================================================================

    // 1. RELAYS (destroyed LAST)
    std::unique_ptr<juce::WebSliderRelay> xRelay;
    std::unique_ptr<juce::WebSliderRelay> yRelay;
    std::unique_ptr<juce::WebSliderRelay> characterARelay;
    std::unique_ptr<juce::WebSliderRelay> characterBRelay;
    std::unique_ptr<juce::WebSliderRelay> evolveRelay;
    std::unique_ptr<juce::WebSliderRelay> brightnessRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebComboBoxRelay> sourceRelay;
    std::unique_ptr<juce::WebComboBoxRelay> modeRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> freezeRelay;

    // 2. WEBVIEW (destroyed SECOND)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (destroyed FIRST)
    std::unique_ptr<juce::WebSliderParameterAttachment> xAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> yAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> characterAAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> characterBAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> evolveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> brightnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> sourceAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> modeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> freezeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureEditor)
};
