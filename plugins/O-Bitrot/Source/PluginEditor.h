/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

class OBitrotAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit OBitrotAudioProcessorEditor(OBitrotAudioProcessor&);
    ~OBitrotAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    OBitrotAudioProcessor& audioProcessor;

    // ========================================================================
    // NATIVE-FUNCTION SURFACE — exactly TWELVE, and it must match the names
    // requested by the inline module in Source/ui/public/index.html AND by
    // modules/preset-manager.js:
    //   - setTooltipsEnabled  (v1.12.0: "?" toggle -> processor state property)
    //   - getTooltipsEnabled  (v1.12.0: page init PULLS the saved preference)
    //   - the 10 preset fns (Stage 4): savePreset, savePresetWithDialog,
    //     loadPreset, loadPresetFromFile, getPresetList, getCurrentPreset,
    //     selectNextPreset, selectPreviousPreset, deletePreset, isFactoryPreset
    // An unregistered fn leaves its control silently dead while build, auval
    // and pluginval all pass (pattern_webview_native_fn_bridge_gap), so the
    // gate grep-diffs both directions at 12<->12.
    // ========================================================================

    // ========================================================================
    // CRITICAL MEMBER DECLARATION ORDER: Relays -> WebView -> Attachments
    // Members destroyed in REVERSE declaration order (C++ standard).
    // Attachments call into the WebView on destruction, so they MUST die first.
    // ========================================================================

    // 1. RELAYS (no dependencies) — 38 total: 25 slider / 7 toggle / 6 combo
    // Tape
    std::unique_ptr<juce::WebToggleButtonRelay> tapeEnableRelay;
    std::unique_ptr<juce::WebSliderRelay>       tapeProbRelay;
    std::unique_ptr<juce::WebSliderRelay>       tapeStopProbRelay;
    std::unique_ptr<juce::WebSliderRelay>       tapeRampRelay;
    std::unique_ptr<juce::WebSliderRelay>       tapeDropRelay;
    std::unique_ptr<juce::WebSliderRelay>       tapeWowRelay;
    std::unique_ptr<juce::WebSliderRelay>       tapeHissRelay;
    // CD Skip
    std::unique_ptr<juce::WebToggleButtonRelay> cdEnableRelay;
    std::unique_ptr<juce::WebSliderRelay>       cdProbRelay;
    std::unique_ptr<juce::WebSliderRelay>       cdSeverityRelay;
    std::unique_ptr<juce::WebSliderRelay>       cdSegmentRelay;
    // Vinyl
    std::unique_ptr<juce::WebToggleButtonRelay> vinylEnableRelay;
    std::unique_ptr<juce::WebSliderRelay>       vinylProbRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     vinylRpmRelay;
    std::unique_ptr<juce::WebSliderRelay>       vinylPopRelay;
    std::unique_ptr<juce::WebSliderRelay>       vinylWearRelay;
    std::unique_ptr<juce::WebSliderRelay>       vinylWarpRelay;
    // Packet
    std::unique_ptr<juce::WebToggleButtonRelay> packetEnableRelay;
    std::unique_ptr<juce::WebSliderRelay>       packetLossRelay;
    std::unique_ptr<juce::WebSliderRelay>       packetBurstRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     packetConcealRelay;
    std::unique_ptr<juce::WebSliderRelay>       packetComfortRelay;
    // Codec
    std::unique_ptr<juce::WebToggleButtonRelay> codecEnableRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     codecModeRelay;
    std::unique_ptr<juce::WebSliderRelay>       codecMixRelay;
    std::unique_ptr<juce::WebSliderRelay>       codecNoiseRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     codecMainsRelay;
    std::unique_ptr<juce::WebSliderRelay>       codecAgcRelay;
    // Crush
    std::unique_ptr<juce::WebToggleButtonRelay> crushEnableRelay;
    std::unique_ptr<juce::WebSliderRelay>       crushBitsRelay;
    std::unique_ptr<juce::WebSliderRelay>       crushRateRelay;
    std::unique_ptr<juce::WebSliderRelay>       crushJitterRelay;
    std::unique_ptr<juce::WebSliderRelay>       crushEnvAmtRelay;
    std::unique_ptr<juce::WebSliderRelay>       crushDitherRelay;
    // Rot (v1.10.0)
    std::unique_ptr<juce::WebToggleButtonRelay> rotEnableRelay;
    std::unique_ptr<juce::WebSliderRelay>       rotProbRelay;
    std::unique_ptr<juce::WebSliderRelay>       rotDepthRelay;
    std::unique_ptr<juce::WebSliderRelay>       rotStickRelay;
    std::unique_ptr<juce::WebSliderRelay>       rotGarbleRelay;
    // Global
    std::unique_ptr<juce::WebComboBoxRelay>     clockModeRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     clockSyncDivRelay;
    std::unique_ptr<juce::WebSliderRelay>       clockFreeRateRelay;
    std::unique_ptr<juce::WebSliderRelay>       seedRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> hardEdgesRelay;
    std::unique_ptr<juce::WebSliderRelay>       mixRelay;

    // 2. WEBVIEW (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (depend on relays + webView)
    // Tape
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> tapeEnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       tapeProbAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       tapeStopProbAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       tapeRampAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       tapeDropAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       tapeWowAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       tapeHissAttachment;
    // CD Skip
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> cdEnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       cdProbAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       cdSeverityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       cdSegmentAttachment;
    // Vinyl
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> vinylEnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       vinylProbAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     vinylRpmAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       vinylPopAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       vinylWearAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       vinylWarpAttachment;
    // Packet
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> packetEnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       packetLossAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       packetBurstAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     packetConcealAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       packetComfortAttachment;
    // Codec
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> codecEnableAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     codecModeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       codecMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       codecNoiseAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     codecMainsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       codecAgcAttachment;
    // Crush
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> crushEnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       crushBitsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       crushRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       crushJitterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       crushEnvAmtAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       crushDitherAttachment;
    // Rot (v1.10.0)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> rotEnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       rotProbAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       rotDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       rotStickAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       rotGarbleAttachment;
    // Global
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     clockModeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     clockSyncDivAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       clockFreeRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       seedAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> hardEdgesAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBitrotAudioProcessorEditor)
};
