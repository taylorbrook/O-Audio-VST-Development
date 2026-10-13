/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    O-simpleAmbisonics - Plugin Editor  (TEMPLATE — mockup v1 finalization)
    Ouaricon Audio
    Developer: Taylor Brook

    This is a REFERENCE for gui-agent (Stage 3, 3.1 bindings), not a copy-paste
    file: adapt it to the processor as it stands when Stage 3 starts. It is an
    EFFECT editor — no keyboard, no file import, no preset-manager natives yet
    (the FUNC-08 bank arrives in Stage 4).

    Single-page WebView UI on a 1180 x 780 design frame, resizable at a fixed
    aspect (0.6x..1.5x) through the O-simpleFM scale pattern: the page lays out
    at the design size and scales itself by CSS transform; the editor persists
    `editorScale` on the processor for the session.

    Binds all 12 APVTS parameters two-way (ids FROZEN, camelCase, exactly
    OSimpleAmbisonics::ParamIDs):
       8 WebSliderRelay        azimuth, elevation, distance, yaw, pitch, roll,
                               orbitRate, outputLevel
       3 WebComboBoxRelay      order, decoder, inputMode
       1 WebToggleButtonRelay  orbit
    and on the 30 Hz message-thread Timer pushes ONE event:
      meterFrame   TripleBuffer<MeterFrame>::read(), quantised (shGain / peaks
                   1e-3, angles 0.01 deg) and hash-gated so a static scene emits
                   nothing; emitEventIfBrowserIsVisible; forced by uiReady.
    Natives (every name the page calls — an unregistered one never settles its
    promise and its control is silently dead while build / auval / pluginval
    all pass): getParameterDefaults, getUiLanguage, setUiLanguage, uiReady,
    applyFactoryPreset.

    PROCESSOR MEMBERS THIS EDITOR EXPECTS (none exist at Stage 2 — the
    integration checklist §2 lists them; CONTEXT.md allows exactly these
    processor edits):
      float editorScale = 0.0f;                  // message thread; 0 = fit the display
      std::atomic<int> uiLanguage { 0 };         // 0 en, 1 fr, 2 zh-Hans
      static juce::String languageCode (int);    // 1 -> "fr", 2 -> "zh-Hans", else "en"
      static int languageIndex (const juce::String&);   // unknown -> 0
      bool applyFactoryPreset (const juce::String& lessonId);   // message thread, APVTS only
    plus the `uiLanguage` property persisted on apvts.state in get/setStateInformation.

  ==============================================================================
*/

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

#include <array>
#include <atomic>
#include <memory>
#include <optional>

class OSimpleAmbisonicsAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              private juce::Timer,
                                              private juce::AudioProcessorParameter::Listener
{
public:
    explicit OSimpleAmbisonicsAudioProcessorEditor (OSimpleAmbisonicsAudioProcessor&);
    ~OSimpleAmbisonicsAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Design frame the page lays out at, and the resize range (ui-design-rules
    // Rule 4, resizable pattern; the O-simpleFM / O-simpleAdditive shape).
    static constexpr int   designW  = 1180;
    static constexpr int   designH  = 780;
    static constexpr float minScale = 0.6f;
    static constexpr float maxScale = 1.5f;
    static float fitScaleForDisplay();

    // O-simpleWavetable v1.0.1: a page drag opens a host gesture through the
    // slider attachment (sliderDragStarted -> beginChangeGesture); if the host
    // closes the editor mid-drag no sliderDragEnded ever arrives and
    // ~WebSliderParameterAttachment does not end it. Every begin / end on every
    // parameter is tracked here and the destructor ends whatever is still open
    // (pattern_web_slider_attachment_dtor_leaves_gesture_open).
    static constexpr int kNumParams = 12;
    void parameterValueChanged (int, float) override {}
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;
    void closeOpenGestures();
    std::array<std::atomic<bool>, kNumParams> gestureOpen {};

    // Resource provider — serves the embedded UI files (BARE-PATH matching).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    // C++ -> page push. read()s the triple buffer, quantises, hash-gates, emits.
    // `force` bypasses the hash (uiReady, visibilitychange -> visible).
    void emitMeterFrame (bool force);

    OSimpleAmbisonicsAudioProcessor& processorRef;

    // Frame change detection + the reused payload (message thread only).
    juce::uint64 lastFrameHash  = 0;
    bool         forceFrameEmit = true;
    juce::String framePayload;              // pre-reserved; the one per-tick allocation is its growth, once

    // =====================================================================
    // CRITICAL MEMBER DECLARATION ORDER
    // Order: Relays -> WebView -> Attachments
    // Members are destroyed in REVERSE order:
    //   attachments first (WebView still alive), WebView second, relays last.
    // Wrong order = release-build crash / DAW freeze on plugin reload.
    // =====================================================================

    // 1) RELAYS FIRST (no dependencies) — 12, one per parameter, APVTS order
    std::unique_ptr<juce::WebSliderRelay>       azimuthRelay;
    std::unique_ptr<juce::WebSliderRelay>       elevationRelay;
    std::unique_ptr<juce::WebSliderRelay>       distanceRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     orderRelay;
    std::unique_ptr<juce::WebSliderRelay>       yawRelay;
    std::unique_ptr<juce::WebSliderRelay>       pitchRelay;
    std::unique_ptr<juce::WebSliderRelay>       rollRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     decoderRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> orbitRelay;
    std::unique_ptr<juce::WebSliderRelay>       orbitRateRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     inputModeRelay;
    std::unique_ptr<juce::WebSliderRelay>       outputLevelRelay;

    // 2) WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3) ATTACHMENTS LAST (depend on relays AND webView) — 12, same order
    std::unique_ptr<juce::WebSliderParameterAttachment>       azimuthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       elevationAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       distanceAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     orderAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       yawAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       pitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       rollAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     decoderAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> orbitAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       orbitRateAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     inputModeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       outputLevelAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleAmbisonicsAudioProcessorEditor)
};
