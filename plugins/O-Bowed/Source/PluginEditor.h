/*
   This file is part of O-Bowed, an Ouaricon Audio plugin.
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

    O-Bowed - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <JuceHeader.h>

class OBowedAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OBowedAudioProcessorEditor(OBowedAudioProcessor&);
    ~OBowedAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OBowedAudioProcessor& processorRef;

    // ===================================================================
    // CRITICAL ORDER: Relays -> WebView -> Attachments
    // Members destroyed in REVERSE order. Attachments call evaluateJavascript()
    // during destruction, so WebView must still exist when attachments die.
    // ===================================================================

    // 1. RELAYS FIRST (no dependencies)
    // Bow Controls
    std::unique_ptr<juce::WebSliderRelay> bowSpeedRelay;
    std::unique_ptr<juce::WebSliderRelay> bowPressureRelay;
    std::unique_ptr<juce::WebSliderRelay> bowPositionRelay;
    std::unique_ptr<juce::WebSliderRelay> rosinRelay;
    std::unique_ptr<juce::WebSliderRelay> bowNoiseRelay;
    // Body Controls
    std::unique_ptr<juce::WebSliderRelay> bodyMaterialRelay;
    std::unique_ptr<juce::WebSliderRelay> bodySizeRelay;
    std::unique_ptr<juce::WebSliderRelay> brightnessRelay;
    // String Configuration
    std::unique_ptr<juce::WebSliderRelay> sympatheticAmountRelay;
    std::unique_ptr<juce::WebSliderRelay> sympatheticCountRelay;
    // Advanced Physics (CR-04: real params that previously had no UI binding)
    std::unique_ptr<juce::WebSliderRelay> sympatheticDecayRelay;
    std::unique_ptr<juce::WebSliderRelay> bodyAmountRelay;
    std::unique_ptr<juce::WebSliderRelay> stringGaugeRelay;
    std::unique_ptr<juce::WebSliderRelay> bowHairStiffnessRelay;
    // Output
    std::unique_ptr<juce::WebSliderRelay> widthRelay;
    std::unique_ptr<juce::WebSliderRelay> outputLevelRelay;
    // Impossible Physics
    std::unique_ptr<juce::WebSliderRelay> infiniteSustainRelay;
    std::unique_ptr<juce::WebSliderRelay> reversedFrictionRelay;
    std::unique_ptr<juce::WebSliderRelay> subHarmonicsRelay;
    // Tuning
    std::unique_ptr<juce::WebSliderRelay> referencePitchRelay;
    std::unique_ptr<juce::WebComboBoxRelay> tuningSystemRelay;
    // Humanize (range + rate per bow parameter)
    std::unique_ptr<juce::WebSliderRelay> humanizeSpeedRangeRelay;
    std::unique_ptr<juce::WebSliderRelay> humanizeSpeedRateRelay;
    std::unique_ptr<juce::WebSliderRelay> humanizePressureRangeRelay;
    std::unique_ptr<juce::WebSliderRelay> humanizePressureRateRelay;
    std::unique_ptr<juce::WebSliderRelay> humanizePositionRangeRelay;
    std::unique_ptr<juce::WebSliderRelay> humanizePositionRateRelay;
    std::unique_ptr<juce::WebSliderRelay> humanizeRosinRangeRelay;
    std::unique_ptr<juce::WebSliderRelay> humanizeRosinRateRelay;

    // 2. WEBVIEW SECOND (depends on relays via .withOptionsFrom())
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on both relays and webView)
    // Bow Controls
    std::unique_ptr<juce::WebSliderParameterAttachment> bowSpeedAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bowPressureAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bowPositionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> rosinAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bowNoiseAttachment;
    // Body Controls
    std::unique_ptr<juce::WebSliderParameterAttachment> bodyMaterialAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bodySizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> brightnessAttachment;
    // String Configuration
    std::unique_ptr<juce::WebSliderParameterAttachment> sympatheticAmountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sympatheticCountAttachment;
    // Advanced Physics (CR-04)
    std::unique_ptr<juce::WebSliderParameterAttachment> sympatheticDecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bodyAmountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> stringGaugeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bowHairStiffnessAttachment;
    // Output
    std::unique_ptr<juce::WebSliderParameterAttachment> widthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputLevelAttachment;
    // Impossible Physics
    std::unique_ptr<juce::WebSliderParameterAttachment> infiniteSustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reversedFrictionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> subHarmonicsAttachment;
    // Tuning
    std::unique_ptr<juce::WebSliderParameterAttachment> referencePitchAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> tuningSystemAttachment;
    // Humanize
    std::unique_ptr<juce::WebSliderParameterAttachment> humanizeSpeedRangeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> humanizeSpeedRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> humanizePressureRangeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> humanizePressureRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> humanizePositionRangeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> humanizePositionRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> humanizeRosinRangeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> humanizeRosinRateAttachment;

    // Helper for serving UI resources from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // File chooser for preset save/load dialogs (must be shared_ptr)
    std::shared_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBowedAudioProcessorEditor)
};
