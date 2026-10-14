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

    O-simpleAmbisonics - Plugin Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Single-page WebView UI on a 1180 x 780 design frame, resizable at a fixed
    aspect (0.6x..1.5x, the O-simpleAdditive scale pattern): the page lays out
    at the design size and scales itself by CSS transform. The window scale is
    PERSISTED on the processor (editorScale, in the state XML) and clamped to
    the display the editor opens on.

    Binds all 12 APVTS parameters two-way (ids FROZEN, camelCase, exactly
    OSimpleAmbisonics::ParamIDs):
       8 WebSliderRelay        azimuth, elevation, distance, yaw, pitch, roll,
                               orbitRate, outputLevel
       3 WebComboBoxRelay      order, decoder, inputMode
       1 WebToggleButtonRelay  orbit
    and on the 30 Hz message-thread Timer pushes ONE event:
      meterFrame   ui::sampleFrame() (MeterFramePayload.h — the same code the
                   render harness uses for its fixtures): quantised, hashed with
                   the `live` flag, emitted only on change or when forced;
                   emitEventIfBrowserIsVisible. A static scene emits nothing.
    Natives (every name the page calls — an unregistered one never settles its
    promise and its control is silently dead while build / auval / pluginval
    all pass): getParameterDefaults, getUiLanguage, setUiLanguage, uiReady,
    applyFactoryPreset.

  ==============================================================================
*/

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "MeterFramePayload.h"

#include <array>
#include <atomic>
#include <cstdint>
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
    static float fitScaleForDisplay();   // largest scale <= 1 that fits (fresh editor)
    static float fitMaxForDisplay();     // largest scale <= maxScale that fits (stored scale clamp)

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

    // C++ -> page push: emits `current` if its hash changed (or `force`).
    // Never reads the triple buffer — timerCallback is the only sampler, so a
    // uiReady handshake can never count as a stale tick.
    void emitFrame (bool force);

    OSimpleAmbisonicsAudioProcessor& processorRef;

    // meterFrame state (message thread only). payloadBuf is a member: the
    // text is formatted without allocation; the one juce::String per emission
    // is the fromUTF8 copy handed to emitEventIfBrowserIsVisible.
    OSimpleAmbisonics::ui::LiveTracker    liveTracker;
    OSimpleAmbisonics::ui::QuantisedFrame current;
    std::uint64_t lastFrameHash  = 0;
    bool          forceFrameEmit = true;
    char          payloadBuf[1024] {};

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
