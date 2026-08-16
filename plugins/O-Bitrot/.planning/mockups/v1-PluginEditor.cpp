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
#include "PluginEditor.h"
#include "BinaryData.h"

OBitrotAudioProcessorEditor::OBitrotAudioProcessorEditor(OBitrotAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // 1. Create relays FIRST (IDs verbatim from parameter-spec.md — BINDING)
    // Tape
    tapeEnableRelay    = std::make_unique<juce::WebToggleButtonRelay>("TAPE_ENABLE");
    tapeProbRelay      = std::make_unique<juce::WebSliderRelay>("TAPE_PROB");
    tapeStopProbRelay  = std::make_unique<juce::WebSliderRelay>("TAPE_STOP_PROB");
    tapeRampRelay      = std::make_unique<juce::WebSliderRelay>("TAPE_RAMP");
    // CD Skip
    cdEnableRelay      = std::make_unique<juce::WebToggleButtonRelay>("CD_ENABLE");
    cdProbRelay        = std::make_unique<juce::WebSliderRelay>("CD_PROB");
    cdSeverityRelay    = std::make_unique<juce::WebSliderRelay>("CD_SEVERITY");
    cdSegmentRelay     = std::make_unique<juce::WebSliderRelay>("CD_SEGMENT");
    // Vinyl
    vinylEnableRelay   = std::make_unique<juce::WebToggleButtonRelay>("VINYL_ENABLE");
    vinylProbRelay     = std::make_unique<juce::WebSliderRelay>("VINYL_PROB");
    vinylRpmRelay      = std::make_unique<juce::WebComboBoxRelay>("VINYL_RPM");
    vinylPopRelay      = std::make_unique<juce::WebSliderRelay>("VINYL_POP");
    // Packet
    packetEnableRelay  = std::make_unique<juce::WebToggleButtonRelay>("PACKET_ENABLE");
    packetLossRelay    = std::make_unique<juce::WebSliderRelay>("PACKET_LOSS");
    packetBurstRelay   = std::make_unique<juce::WebSliderRelay>("PACKET_BURST");
    packetConcealRelay = std::make_unique<juce::WebComboBoxRelay>("PACKET_CONCEAL");
    // Codec
    codecEnableRelay   = std::make_unique<juce::WebToggleButtonRelay>("CODEC_ENABLE");
    codecModeRelay     = std::make_unique<juce::WebComboBoxRelay>("CODEC_MODE");
    codecMixRelay      = std::make_unique<juce::WebSliderRelay>("CODEC_MIX");
    // Crush
    crushEnableRelay   = std::make_unique<juce::WebToggleButtonRelay>("CRUSH_ENABLE");
    crushBitsRelay     = std::make_unique<juce::WebSliderRelay>("CRUSH_BITS");
    crushRateRelay     = std::make_unique<juce::WebSliderRelay>("CRUSH_RATE");
    crushJitterRelay   = std::make_unique<juce::WebSliderRelay>("CRUSH_JITTER");
    crushEnvAmtRelay   = std::make_unique<juce::WebSliderRelay>("CRUSH_ENV_AMT");
    crushDitherRelay   = std::make_unique<juce::WebSliderRelay>("CRUSH_DITHER");
    // Global
    clockModeRelay     = std::make_unique<juce::WebComboBoxRelay>("CLOCK_MODE");
    clockSyncDivRelay  = std::make_unique<juce::WebComboBoxRelay>("CLOCK_SYNC_DIV");
    clockFreeRateRelay = std::make_unique<juce::WebSliderRelay>("CLOCK_FREE_RATE");
    seedRelay          = std::make_unique<juce::WebSliderRelay>("SEED");
    hardEdgesRelay     = std::make_unique<juce::WebToggleButtonRelay>("HARD_EDGES");
    mixRelay           = std::make_unique<juce::WebSliderRelay>("MIX");

    // 2. Create WebView with relay options (NO native functions in this plugin)
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile("OBitrot_WebView")))
            .withNativeIntegrationEnabled()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
            .withOptionsFrom(*tapeEnableRelay)
            .withOptionsFrom(*tapeProbRelay)
            .withOptionsFrom(*tapeStopProbRelay)
            .withOptionsFrom(*tapeRampRelay)
            .withOptionsFrom(*cdEnableRelay)
            .withOptionsFrom(*cdProbRelay)
            .withOptionsFrom(*cdSeverityRelay)
            .withOptionsFrom(*cdSegmentRelay)
            .withOptionsFrom(*vinylEnableRelay)
            .withOptionsFrom(*vinylProbRelay)
            .withOptionsFrom(*vinylRpmRelay)
            .withOptionsFrom(*vinylPopRelay)
            .withOptionsFrom(*packetEnableRelay)
            .withOptionsFrom(*packetLossRelay)
            .withOptionsFrom(*packetBurstRelay)
            .withOptionsFrom(*packetConcealRelay)
            .withOptionsFrom(*codecEnableRelay)
            .withOptionsFrom(*codecModeRelay)
            .withOptionsFrom(*codecMixRelay)
            .withOptionsFrom(*crushEnableRelay)
            .withOptionsFrom(*crushBitsRelay)
            .withOptionsFrom(*crushRateRelay)
            .withOptionsFrom(*crushJitterRelay)
            .withOptionsFrom(*crushEnvAmtRelay)
            .withOptionsFrom(*crushDitherRelay)
            .withOptionsFrom(*clockModeRelay)
            .withOptionsFrom(*clockSyncDivRelay)
            .withOptionsFrom(*clockFreeRateRelay)
            .withOptionsFrom(*seedRelay)
            .withOptionsFrom(*hardEdgesRelay)
            .withOptionsFrom(*mixRelay)
    );

    addAndMakeVisible(*webView);

    // 3. Create attachments LAST (JUCE 8.0.14 three-arg form: param, relay, nullptr)
    // Tape
    tapeEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("TAPE_ENABLE"), *tapeEnableRelay, nullptr);
    tapeProbAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("TAPE_PROB"), *tapeProbRelay, nullptr);
    tapeStopProbAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("TAPE_STOP_PROB"), *tapeStopProbRelay, nullptr);
    tapeRampAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("TAPE_RAMP"), *tapeRampRelay, nullptr);
    // CD Skip
    cdEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("CD_ENABLE"), *cdEnableRelay, nullptr);
    cdProbAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CD_PROB"), *cdProbRelay, nullptr);
    cdSeverityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CD_SEVERITY"), *cdSeverityRelay, nullptr);
    cdSegmentAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CD_SEGMENT"), *cdSegmentRelay, nullptr);
    // Vinyl
    vinylEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("VINYL_ENABLE"), *vinylEnableRelay, nullptr);
    vinylProbAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("VINYL_PROB"), *vinylProbRelay, nullptr);
    vinylRpmAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("VINYL_RPM"), *vinylRpmRelay, nullptr);
    vinylPopAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("VINYL_POP"), *vinylPopRelay, nullptr);
    // Packet
    packetEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("PACKET_ENABLE"), *packetEnableRelay, nullptr);
    packetLossAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("PACKET_LOSS"), *packetLossRelay, nullptr);
    packetBurstAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("PACKET_BURST"), *packetBurstRelay, nullptr);
    packetConcealAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("PACKET_CONCEAL"), *packetConcealRelay, nullptr);
    // Codec
    codecEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("CODEC_ENABLE"), *codecEnableRelay, nullptr);
    codecModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("CODEC_MODE"), *codecModeRelay, nullptr);
    codecMixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CODEC_MIX"), *codecMixRelay, nullptr);
    // Crush
    crushEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("CRUSH_ENABLE"), *crushEnableRelay, nullptr);
    crushBitsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CRUSH_BITS"), *crushBitsRelay, nullptr);
    crushRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CRUSH_RATE"), *crushRateRelay, nullptr);
    crushJitterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CRUSH_JITTER"), *crushJitterRelay, nullptr);
    crushEnvAmtAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CRUSH_ENV_AMT"), *crushEnvAmtRelay, nullptr);
    crushDitherAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CRUSH_DITHER"), *crushDitherRelay, nullptr);
    // Global
    clockModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("CLOCK_MODE"), *clockModeRelay, nullptr);
    clockSyncDivAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("CLOCK_SYNC_DIV"), *clockSyncDivRelay, nullptr);
    clockFreeRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CLOCK_FREE_RATE"), *clockFreeRateRelay, nullptr);
    seedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("SEED"), *seedRelay, nullptr);
    hardEdgesAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("HARD_EDGES"), *hardEdgesRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("MIX"), *mixRelay, nullptr);

    // Load UI from resource provider
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    startTimerHz(30);
    setSize(900, 620);
}

OBitrotAudioProcessorEditor::~OBitrotAudioProcessorEditor()
{
    stopTimer();
}

void OBitrotAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Aged-paper base tone behind the WebView while it loads
    g.fillAll(juce::Colour(0xfff5e6d3));
}

void OBitrotAudioProcessorEditor::resized()
{
    webView->setBounds(getLocalBounds());
}

void OBitrotAudioProcessorEditor::timerCallback()
{
    // Event LED bridge — fire-and-forget at 30 Hz. Payload bits 0-3 =
    // tape, cd, vinyl, packet. Codec/Crush LEDs are UI-side (enable state).
    if (! webView->isShowing())
        return;

    const auto mask = audioProcessor.uiActivityMask.load(std::memory_order_relaxed);
    // NOTE: juce::String("...") << x does not compile (repo pattern) — use +
    const juce::String payload = juce::String("{\"mask\":")
                               + juce::String(static_cast<int>(mask)) + "}";
    webView->emitEventIfBrowserIsVisible("ledUpdate", juce::var(payload));
}

std::optional<juce::WebBrowserComponent::Resource>
OBitrotAudioProcessorEditor::getResource(const juce::String& url)
{
    // Resource provider receives BARE paths ("/index.html"), never a scheme
    // (repo pattern: URL schemes differ per platform).
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size);
    };

    if (url == "/" || url == "/index.html")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")};
    }

    if (url == "/js/juce/index.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript")};
    }

    if (url == "/js/juce/check_native_interop.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")};
    }

    if (url == "/img/paper.jpg")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")};
    }

    if (url == "/img/specimen.webp")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::specimen_webp, BinaryData::specimen_webpSize),
            juce::String("image/webp")};
    }

    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
