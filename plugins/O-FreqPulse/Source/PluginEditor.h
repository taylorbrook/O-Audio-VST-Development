/*
   This file is part of O-FreqPulse, an Ouaricon Audio plugin.
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

    O-FreqPulse - Editor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>

struct BandRelays
{
    std::unique_ptr<juce::WebToggleButtonRelay> enable;
    std::unique_ptr<juce::WebSliderRelay> depth;
    std::unique_ptr<juce::WebComboBoxRelay> rate;
    std::unique_ptr<juce::WebToggleButtonRelay> eucOn;
    std::unique_ptr<juce::WebSliderRelay> eucSteps;
    std::unique_ptr<juce::WebSliderRelay> eucPulses;
    std::unique_ptr<juce::WebSliderRelay> eucOffset;
    std::unique_ptr<juce::WebSliderRelay> phaseOffset;
    std::unique_ptr<juce::WebSliderRelay> bandSteps;
};

struct BandAttachments
{
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> enable;
    std::unique_ptr<juce::WebSliderParameterAttachment> depth;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> rate;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> eucOn;
    std::unique_ptr<juce::WebSliderParameterAttachment> eucSteps;
    std::unique_ptr<juce::WebSliderParameterAttachment> eucPulses;
    std::unique_ptr<juce::WebSliderParameterAttachment> eucOffset;
    std::unique_ptr<juce::WebSliderParameterAttachment> phaseOffset;
    std::unique_ptr<juce::WebSliderParameterAttachment> bandSteps;
};

class OFreqPulseAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit OFreqPulseAudioProcessorEditor(OFreqPulseAudioProcessor&);
    ~OFreqPulseAudioProcessorEditor() override;

    void resized() override;

private:
    // Timer callback for playhead updates
    void timerCallback() override;

    OFreqPulseAudioProcessor& processorRef;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL ORDER: Relays → WebView → Attachments
    // Members destroyed in REVERSE order. Attachments call evaluateJavascript()
    // during destruction, so WebView must still exist when attachments die.
    // ═══════════════════════════════════════════════════════════════════

    // 1️⃣ RELAYS FIRST (no dependencies)
    // Global parameter relays (5 total)
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebSliderRelay> stepsRelay;
    std::unique_ptr<juce::WebComboBoxRelay> rateRelay;
    std::unique_ptr<juce::WebSliderRelay> swingRelay;
    std::unique_ptr<juce::WebSliderRelay> attackRelay;
    std::unique_ptr<juce::WebSliderRelay> releaseRelay;

    // Crossover parameter relays (3 total)
    std::unique_ptr<juce::WebSliderRelay> crossover1Relay;
    std::unique_ptr<juce::WebSliderRelay> crossover2Relay;
    std::unique_ptr<juce::WebSliderRelay> crossover3Relay;

    // Frequency boundary relays (2 total)
    std::unique_ptr<juce::WebSliderRelay> freqLowRelay;
    std::unique_ptr<juce::WebSliderRelay> freqHighRelay;

    // Per-band parameter relays (24 total: 6 params x 4 bands)
    std::array<BandRelays, 4> bandRelays;

    // Step grid relays (128 total: 32 steps × 4 bands) — velocity sliders
    std::array<std::unique_ptr<juce::WebSliderRelay>, 128> stepRelays;

    // 2️⃣ WEBVIEW SECOND (depends on relays via .withOptionsFrom())
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on both relays and webView)
    // Global parameter attachments (5 total)
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> stepsAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> rateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> swingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseAttachment;

    // Crossover parameter attachments (3 total)
    std::unique_ptr<juce::WebSliderParameterAttachment> crossover1Attachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> crossover2Attachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> crossover3Attachment;

    // Frequency boundary attachments (2 total)
    std::unique_ptr<juce::WebSliderParameterAttachment> freqLowAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> freqHighAttachment;

    // Per-band parameter attachments (24 total)
    std::array<BandAttachments, 4> bandAttachments;

    // Step grid attachments (128 total) — velocity sliders
    std::array<std::unique_ptr<juce::WebSliderParameterAttachment>, 128> stepAttachments;

    // v1.6.0: File chooser for preset save/load dialogs (must persist across async callbacks)
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Helper for serving UI resources from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OFreqPulseAudioProcessorEditor)
};
