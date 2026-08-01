/*
   This file is part of O-Reed, an Ouaricon Audio plugin.
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

    O-Reed - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <JuceHeader.h>

class OReedAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OReedAudioProcessorEditor(OReedAudioProcessor&);
    ~OReedAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OReedAudioProcessor& processorRef;

    // ===================================================================
    // CRITICAL ORDER: Relays -> WebView -> Attachments
    // Members destroyed in REVERSE order. Attachments call evaluateJavascript()
    // during destruction, so WebView must still exist when attachments die.
    // ===================================================================

    // 1. RELAYS FIRST (no dependencies)

    // Primary Controls
    std::unique_ptr<juce::WebSliderRelay>     breathPressureRelay;
    std::unique_ptr<juce::WebSliderRelay>     embouchureRelay;
    std::unique_ptr<juce::WebSliderRelay>     reedHardnessRelay;
    std::unique_ptr<juce::WebSliderRelay>     boreCharacterRelay;
    std::unique_ptr<juce::WebComboBoxRelay>   instrumentPresetRelay;

    // Secondary Controls
    std::unique_ptr<juce::WebSliderRelay>     reedOpeningRelay;
    std::unique_ptr<juce::WebSliderRelay>     bellSizeRelay;
    std::unique_ptr<juce::WebSliderRelay>     airNoiseRelay;
    std::unique_ptr<juce::WebSliderRelay>     doubleReedRelay;
    std::unique_ptr<juce::WebSliderRelay>     boreDiameterRelay;

    // Advanced / Sound Design
    std::unique_ptr<juce::WebSliderRelay>     reedMassRelay;
    std::unique_ptr<juce::WebSliderRelay>     reedDampingRelay;
    std::unique_ptr<juce::WebSliderRelay>     mouthpieceVolRelay;
    std::unique_ptr<juce::WebSliderRelay>     toneHoleCutoffRelay;
    std::unique_ptr<juce::WebSliderRelay>     registerHoleRelay;
    std::unique_ptr<juce::WebSliderRelay>     boreLengthRelay;
    std::unique_ptr<juce::WebComboBoxRelay>   boreProfileRelay;

    // Expressive Controls
    std::unique_ptr<juce::WebSliderRelay>     vibratoDepthRelay;
    std::unique_ptr<juce::WebSliderRelay>     vibratoRateRelay;
    std::unique_ptr<juce::WebComboBoxRelay>   vibratoSourceRelay;
    std::unique_ptr<juce::WebSliderRelay>     growlAmountRelay;
    std::unique_ptr<juce::WebSliderRelay>     flutterTongueRelay;
    std::unique_ptr<juce::WebSliderRelay>     subtoneRelay;
    std::unique_ptr<juce::WebSliderRelay>     attackChiffRelay;

    // Impossible Physics
    std::unique_ptr<juce::WebSliderRelay>     infiniteSustainRelay;
    std::unique_ptr<juce::WebSliderRelay>     reverseBoreRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> dualBoreRelay;
    std::unique_ptr<juce::WebSliderRelay>     dronePitchRelay;
    std::unique_ptr<juce::WebSliderRelay>     feedbackPathRelay;

    // Tuning
    std::unique_ptr<juce::WebSliderRelay>     referencePitchRelay;
    std::unique_ptr<juce::WebComboBoxRelay>   tuningSystemRelay;

    // Voice Configuration
    std::unique_ptr<juce::WebComboBoxRelay>   polyModeRelay;
    std::unique_ptr<juce::WebSliderRelay>     maxVoicesRelay;
    std::unique_ptr<juce::WebComboBoxRelay>   oversamplingRelay;

    // Output
    std::unique_ptr<juce::WebSliderRelay>     outputGainRelay;

    // 2. WEBVIEW SECOND (depends on relays via .withOptionsFrom())
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on both relays and webView)

    // Primary Controls
    std::unique_ptr<juce::WebSliderParameterAttachment>     breathPressureAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     embouchureAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     reedHardnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     boreCharacterAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>   instrumentPresetAttachment;

    // Secondary Controls
    std::unique_ptr<juce::WebSliderParameterAttachment>     reedOpeningAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     bellSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     airNoiseAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     doubleReedAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     boreDiameterAttachment;

    // Advanced / Sound Design
    std::unique_ptr<juce::WebSliderParameterAttachment>     reedMassAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     reedDampingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     mouthpieceVolAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     toneHoleCutoffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     registerHoleAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     boreLengthAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>   boreProfileAttachment;

    // Expressive Controls
    std::unique_ptr<juce::WebSliderParameterAttachment>     vibratoDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     vibratoRateAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>   vibratoSourceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     growlAmountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     flutterTongueAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     subtoneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     attackChiffAttachment;

    // Impossible Physics
    std::unique_ptr<juce::WebSliderParameterAttachment>     infiniteSustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     reverseBoreAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> dualBoreAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     dronePitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     feedbackPathAttachment;

    // Tuning
    std::unique_ptr<juce::WebSliderParameterAttachment>     referencePitchAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>   tuningSystemAttachment;

    // Voice Configuration
    std::unique_ptr<juce::WebComboBoxParameterAttachment>   polyModeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>     maxVoicesAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>   oversamplingAttachment;

    // Output
    std::unique_ptr<juce::WebSliderParameterAttachment>     outputGainAttachment;

    // File chooser (must outlive async callback)
    std::shared_ptr<juce::FileChooser> fileChooser;

    // Helper for serving UI resources from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OReedAudioProcessorEditor)
};
