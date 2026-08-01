/*
   This file is part of O-simpleSubtractive, an Ouaricon Audio plugin.
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

    O-simpleSubtractive - Plugin Editor (implementation)

    Stage 3 (GUI): Ouaricon-Naturalist WebView UI. 16 slider relays + 4 combo
    relays bound two-way to the APVTS; the filter curve, spectrum, scope, and
    dual-envelope levels are pushed at 30 Hz from the message-thread Timer.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

// ── Resource provider ───────────────────────────────────────────────────────
// The WebView2/WKWebView resource callback receives a BARE PATH ("/", "/index.html",
// "/js/app.js", ...). Compare by direct string equality — never strip a scheme/host
// (a bare path has none, which would collapse every lookup to an empty string).
namespace
{
    auto makeBinaryResource (const char* data, int size, const char* mimeType)
        -> std::optional<juce::WebBrowserComponent::Resource>
    {
        auto* bytes = reinterpret_cast<const std::byte*> (data);
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte> (bytes, bytes + size),
            juce::String (mimeType)
        };
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OSimpleSubtractiveAudioProcessorEditor::getResource (const juce::String& url)
{
    // charset=utf-8 on text resources — the page relies on UTF-8 entities
    // (♪, en-dashes, arrows); a missing charset can mojibake them on some hosts.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (BinaryData::index_html, BinaryData::index_htmlSize, "text/html; charset=utf-8");

    if (url == "/css/styles.css")
        return makeBinaryResource (BinaryData::styles_css, BinaryData::styles_cssSize, "text/css; charset=utf-8");

    if (url == "/js/app.js")
        return makeBinaryResource (BinaryData::app_js, BinaryData::app_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (BinaryData::index_js, BinaryData::index_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (BinaryData::check_native_interop_js,
                                   BinaryData::check_native_interop_jsSize, "application/javascript; charset=utf-8");

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
OSimpleSubtractiveAudioProcessorEditor::OSimpleSubtractiveAudioProcessorEditor (
        OSimpleSubtractiveAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    using namespace OSimpleSubtractive::ParamIDs;

    // 20 APVTS params = 16 float sliders + 4 combos (oscWave / filterType /
    // filterSlope / voiceMode are choices). 0 toggles.
    const juce::StringArray sliderIds {
        subLevel, noiseLevel,
        cutoff, resonance, filterEnvAmount, keyTrack,
        filterAttack, filterDecay, filterSustain, filterRelease,
        ampAttack, ampDecay, ampSustain, ampRelease,
        glide, outputLevel
    };
    const juce::StringArray comboIds { oscWave, filterType, filterSlope, voiceMode };

    // 1. RELAYS (before the WebView) ----------------------------------------
    for (const auto& id : sliderIds)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));
    for (const auto& id : comboIds)
        comboRelays.push_back (std::make_unique<juce::WebComboBoxRelay> (id));

    // 2. WEBVIEW options + relay registration -------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);
    for (const auto& relay : comboRelays)
        options = options.withOptionsFrom (*relay);

    // ── Native functions — exactly three (grep-diff against app.js) ─────────
    options = options
        // On-screen keyboard → synth. args: [noteNumber, isNoteOn, velocity?].
        .withNativeFunction ("uiMidi", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
                processorRef.handleUiMidi ((int) args[0], (bool) args[1],
                                           args.size() >= 3 ? (float) args[2] : 0.8f);
            complete (juce::var());
        })
        // Sample rate for the headline's frequency-axis labels (log-Hz mapping).
        .withNativeFunction ("getSampleRate", [this] (auto&, auto complete) {
            complete (processorRef.getCurrentSampleRate());
        })
        // Concept-preset tour hook (UI-07). Forwards the button label to the
        // processor's full-APVTS snapshot; the relays/attachments sync every
        // knob/combo back to the page. The C++ snapshot bodies land in Stage 4
        // (FUNC-06) — the Stage-3 stub keeps the bridge live.
        .withNativeFunction ("applyFactoryPreset", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0)
                processorRef.applyFactoryPreset (args[0].toString());
            complete (juce::var (true));
        });

   #if JUCE_WINDOWS
    // WebView2 user-data folder MUST be a writable temp dir (DAW hosts deny the
    // default location → silent IE fallback → blank page).
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OsimpleSubtractive_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS (after the WebView; 3-arg ctor + nullptr undoManager) ----
    for (int i = 0; i < sliderIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (sliderIds[i]);
        jassert (param != nullptr);   // ID drift → silently dead knob; catch in debug
        if (param != nullptr)
            sliderAttachments.push_back (
                std::make_unique<juce::WebSliderParameterAttachment> (
                    *param, *sliderRelays[(size_t) i], nullptr));
    }

    for (int i = 0; i < comboIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (comboIds[i]);
        jassert (param != nullptr);   // ID drift → silently dead combo; catch in debug
        if (param != nullptr)
            comboAttachments.push_back (
                std::make_unique<juce::WebComboBoxParameterAttachment> (
                    *param, *comboRelays[(size_t) i], nullptr));
    }

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (1180, 820);   // five signal-path columns + headline + keyboard; the frame scrolls on shorter screens
    setResizable (true, true);
    setResizeLimits (820, 560, 1700, 1200);
    startTimerHz (30);
}

OSimpleSubtractiveAudioProcessorEditor::~OSimpleSubtractiveAudioProcessorEditor()
{
    stopTimer();
}

// ── Timer (30 Hz): analyzer + curve, then push the four viz events ──────────
// Everything here runs on the MESSAGE thread. The audio thread only copies into
// the VizRing (lock-free); the FFT + scope downsample + closed-form curve happen
// here (PERF-01). The analyzer copies the scope window BEFORE its in-place FFT
// (SubVizAnalyzer::process). Do NOT add audio-thread work; do NOT reorder.
void OSimpleSubtractiveAudioProcessorEditor::timerCallback()
{
    vizAnalyzer.process (processorRef.getVizRing(), processorRef.getCurrentSampleRate());
    vizAnalyzer.updateCurve (processorRef.getDisplayCutoffHz(), processorRef.getDisplayK(),
                             processorRef.getDisplayType(),    processorRef.getDisplaySlope(),
                             processorRef.getCurrentSampleRate());   // closed-form curve == running filter (QUAL-02)

    if (webView == nullptr)
        return;

    const auto& curve = vizAnalyzer.getCurve();      // 256 dB bins (can exceed 0 at resonance)
    const auto& spec  = vizAnalyzer.getSpectrum();   // 256 dB bins, ~[-100, 0]
    const auto& scope = vizAnalyzer.getScope();      // 128 pts, [-1, 1]

    juce::Array<juce::var> curveArr, specArr, scopeArr;
    curveArr.ensureStorageAllocated ((int) curve.size());
    specArr.ensureStorageAllocated  ((int) spec.size());
    scopeArr.ensureStorageAllocated ((int) scope.size());
    for (float v : curve) curveArr.add (v);
    for (float v : spec)  specArr.add (v);
    for (float v : scope) scopeArr.add (v);

    // Live dual-envelope levels (0..1) for the ADSR playheads + diagram pulse.
    auto* envObj = new juce::DynamicObject();
    envObj->setProperty ("filterEnv", processorRef.getFilterEnvValue());
    envObj->setProperty ("ampEnv",    processorRef.getAmpEnvValue());

    // Curve BEFORE spectrum — the curve annotates the bars it sits over.
    webView->emitEventIfBrowserIsVisible ("filterCurveUpdate", juce::var (std::move (curveArr)));
    webView->emitEventIfBrowserIsVisible ("spectrumUpdate",    juce::var (std::move (specArr)));
    webView->emitEventIfBrowserIsVisible ("scopeUpdate",       juce::var (std::move (scopeArr)));
    webView->emitEventIfBrowserIsVisible ("envUpdate",         juce::var (envObj));
}

// ── Layout ──────────────────────────────────────────────────────────────────
void OSimpleSubtractiveAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills the editor — nothing to paint.
}

void OSimpleSubtractiveAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
