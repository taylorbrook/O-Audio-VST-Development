/*
   This file is part of O-simpleReverb, an Ouaricon Audio plugin.
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

    O-simpleReverb - Editor (v3.0.0, the O-simple family shape)

    Single-page WebView UI on a 1180 x 780 design frame, resizable at a fixed
    aspect (0.6x..1.5x): the page lays out at the design size and scales
    itself by CSS transform. The window scale is PERSISTED on the processor
    (editorScale, in the state) and clamped to the display the editor opens on.

    Binds all 8 APVTS parameters two-way (ids FROZEN since v1):
       1 WebComboBoxRelay      TYPE
       6 WebSliderRelay        CHARACTER, WET, DRY, DECAY, SIZE, LPFREQ
       1 WebToggleButtonRelay  LPON
    and on the 30 Hz message-thread Timer pushes TWO events:
      meterFrame   osr::ui::sampleFrame() (MeterFramePayload.h - the same code
                   render-check uses for its fixtures): quantised, hashed with
                   the `live` flag, emitted only on change or when forced;
                   emitEventIfBrowserIsVisible. A steady scene emits nothing.
      presetState  { name, id, factory, modified } - when the processor's
                   preset revision changes or `modified` flips.
    Natives (every name the page or preset-manager.js calls - an unregistered
    one never settles its promise and its control is silently dead while
    build / auval / pluginval all pass), 20: see the registry comment in
    PluginEditor.cpp.

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

class OSimpleReverbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer,
                                          private juce::AudioProcessorParameter::Listener
{
public:
    explicit OSimpleReverbAudioProcessorEditor(OSimpleReverbAudioProcessor&);
    ~OSimpleReverbAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Design frame the page lays out at, and the resize range.
    static constexpr int   designW  = 1180;
    static constexpr int   designH  = 780;
    static constexpr float minScale = 0.6f;
    static constexpr float maxScale = 1.5f;
    static float fitScaleForDisplay();   // largest scale <= 1 that fits (fresh editor)
    static float fitMaxForDisplay();     // largest scale <= maxScale that fits (stored scale clamp)

    // A page drag opens a host gesture through the slider attachment; if the
    // host closes the editor mid-drag no end ever arrives and the attachment's
    // destructor does not send one. Every begin / end is tracked here and the
    // destructor ends whatever is still open.
    void parameterValueChanged(int, float) override {}
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;
    void closeOpenGestures();
    std::array<std::atomic<bool>, OSimpleReverbAudioProcessor::kNumParams> gestureOpen {};

    // Resource provider - serves the embedded UI files (BARE-PATH matching).
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // The well models (message thread; see PluginProcessor.h).
    juce::var reverbModel() const;
    juce::var earlyReflectionTaps(int typeIndex, float sizePercent);
    juce::var toneCurve(int typeIndex, float characterValue, float lowCutHz, bool lowCutOn) const;
    double modelRate() const;

    // C++ -> page pushes.
    void emitFrame(bool force);
    void emitPresetState(bool force);

    OSimpleReverbAudioProcessor& processorRef;

    // The early-reflection model: the processor's own class, prepared once at
    // the running rate, read through its public tap accessors. The audio
    // thread's slots are never touched from here.
    osr::EarlyReflections erModel;
    double erModelRate = 0.0;

    // meterFrame state (message thread only). payloadBuf is a member: the
    // text is formatted without allocation; the one juce::String per emission
    // is the copy handed to emitEventIfBrowserIsVisible.
    osr::ui::LiveTracker    liveTracker;
    osr::ui::QuantisedFrame current;
    std::uint64_t lastFrameHash  = 0;
    bool          forceFrameEmit = true;
    char          payloadBuf[256] {};

    // presetState change detection (message thread only).
    juce::uint32 lastPresetRevision = 0;
    bool         lastPresetModified = false;

    // =====================================================================
    // CRITICAL MEMBER DECLARATION ORDER
    // Order: Relays -> WebView -> Attachments
    // Members are destroyed in REVERSE order:
    //   attachments first (WebView still alive), WebView second, relays last.
    // =====================================================================

    // 1) RELAYS FIRST - one per parameter, kParamIds order
    std::unique_ptr<juce::WebComboBoxRelay>     typeRelay;
    std::unique_ptr<juce::WebSliderRelay>       characterRelay;
    std::unique_ptr<juce::WebSliderRelay>       wetRelay;
    std::unique_ptr<juce::WebSliderRelay>       dryRelay;
    std::unique_ptr<juce::WebSliderRelay>       decayRelay;
    std::unique_ptr<juce::WebSliderRelay>       sizeRelay;
    std::unique_ptr<juce::WebSliderRelay>       lpFreqRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lpOnRelay;

    // 2) WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3) ATTACHMENTS LAST (depend on relays AND webView) - same order
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     typeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       characterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       wetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       dryAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       sizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       lpFreqAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lpOnAttachment;

    // The gear menu's "Load file..." chooser (destroyed first).
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSimpleReverbAudioProcessorEditor)
};
