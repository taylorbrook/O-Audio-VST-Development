/*
   This file is part of O-SpectralShaper, an Ouaricon Audio plugin.
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

    O-SpectralShaper - Editor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OSpectralShaperAudioProcessorEditor : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit OSpectralShaperAudioProcessorEditor(OSpectralShaperAudioProcessor&);
    ~OSpectralShaperAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

    // Timer callback for 60fps visualization updates
    void timerCallback() override;

private:
    OSpectralShaperAudioProcessor& processorRef;

    // CRITICAL MEMBER ORDER: relays → webView → attachments
    // Members destroyed in REVERSE order - wrong order causes release build crashes

    // 1. RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebSliderRelay> attackTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> sustainTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> sensitivityRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lookaheadEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> lookaheadTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;

    // 2. WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. NAVIGATION FLAG
    bool hasNavigated = false;

    // Last curves revision pushed to the WebView. Compared against
    // processorRef.getCurvesRevision() in timerCallback(): a mismatch means a
    // preset load or session restore replaced the curves and the curve editors
    // are showing stale shapes, so both curves are re-sent.
    juce::uint32 lastSentCurvesRevision = 0;

    // 4. ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sustainTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sensitivityAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lookaheadEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lookaheadTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;

    // File chooser for preset save/load dialogs (must persist during async operation)
    std::unique_ptr<juce::FileChooser> fileChooser;

    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);
    void handleCurveUpdate(const juce::Array<juce::var>& args,
        void (OSpectralShaperAudioProcessor::*setter)(const std::array<float, 32>&));
    void sendCurveToJS(const char* functionName, const std::array<float, 32>& curve);
    void sendAttackCurveToJS();
    void sendSustainCurveToJS();
    void emitVisualizationFrame(const OSpectralShaperAudioProcessor::VisualizationFrame& frame);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSpectralShaperAudioProcessorEditor)
};
