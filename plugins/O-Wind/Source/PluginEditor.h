/*
   This file is part of O-Wind, an Ouaricon Audio plugin.
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

    O-Wind - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

class OWindAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OWindAudioProcessorEditor(OWindAudioProcessor&);
    ~OWindAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OWindAudioProcessor& processorRef;

    // ===================================================================
    // CRITICAL ORDER: Relays -> WebView -> Attachments
    // Members destroyed in REVERSE order. Attachments call evaluateJavascript()
    // during destruction, so WebView must still exist when attachments die.
    // ===================================================================

    // 1. RELAYS FIRST (no dependencies) - 14 slider relays + 1 toggle + 1 slider (instrument)
    // Excitation Controls
    std::unique_ptr<juce::WebSliderRelay> breathPressureRelay;
    std::unique_ptr<juce::WebSliderRelay> embouchureRelay;
    std::unique_ptr<juce::WebSliderRelay> breathNoiseRelay;
    // Resonator Controls
    std::unique_ptr<juce::WebSliderRelay> materialRelay;
    std::unique_ptr<juce::WebSliderRelay> toneColorRelay;
    std::unique_ptr<juce::WebSliderRelay> airColumnRelay;
    std::unique_ptr<juce::WebSliderRelay> jetReflectionRelay;
    std::unique_ptr<juce::WebSliderRelay> endReflectionRelay;
    // Articulation
    std::unique_ptr<juce::WebSliderRelay> flutterTongueRelay;
    std::unique_ptr<juce::WebSliderRelay> flutterRateRelay;
    // Modulation
    std::unique_ptr<juce::WebSliderRelay> vibratoRateRelay;
    std::unique_ptr<juce::WebSliderRelay> vibratoDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> vibratoTremoloRelay;
    // Output
    std::unique_ptr<juce::WebSliderRelay> widthRelay;
    std::unique_ptr<juce::WebSliderRelay> outputLevelRelay;
    // Impossible Physics
    std::unique_ptr<juce::WebSliderRelay> infiniteSustainRelay;
    std::unique_ptr<juce::WebSliderRelay> reversedJetRelay;
    std::unique_ptr<juce::WebSliderRelay> subHarmonicsRelay;
    // ADSR Envelope
    std::unique_ptr<juce::WebToggleButtonRelay> adsrEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> adsrAttackRelay;
    std::unique_ptr<juce::WebSliderRelay> adsrDecayRelay;
    std::unique_ptr<juce::WebSliderRelay> adsrSustainRelay;
    std::unique_ptr<juce::WebSliderRelay> adsrReleaseRelay;
    // Tone Hole Toggle
    std::unique_ptr<juce::WebToggleButtonRelay> toneHoleToggleRelay;
    // Instrument Preset (int param, bound as slider relay)
    std::unique_ptr<juce::WebSliderRelay> instrumentPresetRelay;
    // v1.16.1 (CR-02): Sound-tab params shipped without relays in v1.12.0
    std::unique_ptr<juce::WebSliderRelay> growlRelay;
    std::unique_ptr<juce::WebSliderRelay> formantRelay;
    std::unique_ptr<juce::WebSliderRelay> vibratoDriftDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> vibratoDriftSpeedRelay;
    // v1.16.1 (CR-01): Effects tab shipped without any relays in v1.14.0
    std::unique_ptr<juce::WebSliderRelay> chorusRateRelay;
    std::unique_ptr<juce::WebSliderRelay> chorusDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> chorusMixRelay;
    std::unique_ptr<juce::WebSliderRelay> delayTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> delayFeedbackRelay;
    std::unique_ptr<juce::WebSliderRelay> delayMixRelay;
    std::unique_ptr<juce::WebSliderRelay> eqLowGainRelay;
    std::unique_ptr<juce::WebSliderRelay> eqMidGainRelay;
    std::unique_ptr<juce::WebSliderRelay> eqMidFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> eqHighGainRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbSizeRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbDampRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbPredelayRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbMixRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbModRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbShimmerRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> chorusBypassRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> delayBypassRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> eqBypassRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> reverbBypassRelay;
    std::unique_ptr<juce::WebComboBoxRelay> delayModeRelay;

    // 2. WEBVIEW SECOND (depends on relays via .withOptionsFrom())
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on both relays and webView) - 14 slider + 1 toggle + 1 slider
    // Excitation Controls
    std::unique_ptr<juce::WebSliderParameterAttachment> breathPressureAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> embouchureAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> breathNoiseAttachment;
    // Resonator Controls
    std::unique_ptr<juce::WebSliderParameterAttachment> materialAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> toneColorAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> airColumnAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> jetReflectionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> endReflectionAttachment;
    // Articulation
    std::unique_ptr<juce::WebSliderParameterAttachment> flutterTongueAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> flutterRateAttachment;
    // Modulation
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoTremoloAttachment;
    // Output
    std::unique_ptr<juce::WebSliderParameterAttachment> widthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputLevelAttachment;
    // Impossible Physics
    std::unique_ptr<juce::WebSliderParameterAttachment> infiniteSustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reversedJetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> subHarmonicsAttachment;
    // ADSR Envelope
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> adsrEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> adsrAttackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> adsrDecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> adsrSustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> adsrReleaseAttachment;
    // Tone Hole Toggle
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> toneHoleToggleAttachment;
    // Instrument Preset
    std::unique_ptr<juce::WebSliderParameterAttachment> instrumentPresetAttachment;
    // v1.16.1 (CR-02): Sound-tab attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> growlAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> formantAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoDriftDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoDriftSpeedAttachment;
    // v1.16.1 (CR-01): Effects-tab attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> chorusRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> chorusDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> chorusMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> delayTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> delayFeedbackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> delayMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqLowGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqMidGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqMidFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqHighGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbDampAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbPredelayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbModAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbShimmerAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> chorusBypassAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> delayBypassAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> eqBypassAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> reverbBypassAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> delayModeAttachment;

    // Helper for serving UI resources from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // File chooser for preset save/load dialogs (must be shared_ptr).
    // fileDialogOpen guards re-entry: launching a second chooser would replace
    // the shared_ptr, so the first completion never fires and its JS `await`
    // hangs forever. The flag is cleared in each completion (destroying the
    // FileChooser inside its own callback is not safe, so the guard is a bool).
    std::shared_ptr<juce::FileChooser> fileChooser;
    bool fileDialogOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OWindAudioProcessorEditor)
};
