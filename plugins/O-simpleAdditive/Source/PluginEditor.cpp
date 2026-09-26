/*
   This file is part of O-simpleAdditive, an Ouaricon Audio plugin.
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

    O-simpleAdditive - Plugin Editor (implementation)

    Stage 3 (GUI): Ouaricon "Additive Field Guide" WebView UI. 31 slider relays
    (16 drawbars + scan/spectral/ADSR/output) + 2 combo relays (Frame B, bit depth)
    bound two-way to the APVTS. On the 30 Hz Timer the scope and the live drawbar
    spectrum are pushed to the page; the on-screen keyboard and lesson presets are
    wired through native functions.

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
OSimpleAdditiveAudioProcessorEditor::getResource (const juce::String& url)
{
    // charset=utf-8 on text resources — the page relies on UTF-8 entities
    // (♪, fleurons, en-dashes); a missing charset can mojibake them on some hosts.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (BinaryData::index_html, BinaryData::index_htmlSize, "text/html; charset=utf-8");

    if (url == "/css/styles.css")
        return makeBinaryResource (BinaryData::styles_css, BinaryData::styles_cssSize, "text/css; charset=utf-8");

    if (url == "/js/app.js")
        return makeBinaryResource (BinaryData::app_js, BinaryData::app_jsSize, "application/javascript; charset=utf-8");

    // v1.1.0 — the interface copy table (English + French). Served from the same
    // provider as app.js, which imports it; a branch missing here is a blank
    // page rather than an English one, because the import fails to resolve.
    if (url == "/js/i18n.js")
        return makeBinaryResource (BinaryData::i18n_js, BinaryData::i18n_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (BinaryData::index_js, BinaryData::index_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (BinaryData::check_native_interop_js,
                                   BinaryData::check_native_interop_jsSize, "application/javascript; charset=utf-8");

    if (url == "/img/insects.png")
        return makeBinaryResource (BinaryData::insects_png, BinaryData::insects_pngSize, "image/png");

    // v1.4.0 (R5): shared EB Garamond face (modules/ui/eb-garamond) —
    // stylesheet under /css/, the three woff2 faces under /fonts/ where its
    // relative font URLs land.
    if (url == "/css/eb-garamond.css")
        return makeBinaryResource (BinaryData::ebgaramond_css, BinaryData::ebgaramond_cssSize,
                                   "text/css; charset=utf-8");
    if (url == "/fonts/EBGaramond-Regular.woff2")
        return makeBinaryResource (BinaryData::EBGaramondRegular_woff2,
                                   BinaryData::EBGaramondRegular_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Italic.woff2")
        return makeBinaryResource (BinaryData::EBGaramondItalic_woff2,
                                   BinaryData::EBGaramondItalic_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Bold.woff2")
        return makeBinaryResource (BinaryData::EBGaramondBold_woff2,
                                   BinaryData::EBGaramondBold_woff2Size, "font/woff2");

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
OSimpleAdditiveAudioProcessorEditor::OSimpleAdditiveAudioProcessorEditor (OSimpleAdditiveAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    using namespace OSimpleAdditive::ParamIDs;

    // 31 float sliders (16 drawbars + scan/spectral/ADSR/output) + 2 combo boxes.
    // The 16 drawbar IDs come from the shared partialIds array (PluginProcessor.h).
    juce::StringArray sliderIds;
    for (const auto* id : partialIds)
        sliderIds.add (id);
    sliderIds.addArray (juce::StringArray {
        scanPosition, scanLfoRate, scanLfoDepth, scanEnvAmount,
        spectralDecay, velToDecay,
        ampAttack, ampDecay, ampSustain, ampRelease,
        modAttack, modDecay, modSustain, modRelease,
        outputLevel
    });
    const juce::StringArray comboIds { frameBSource, bitDepth };

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

    options = options
        // On-screen keyboard → synth. args: [noteNumber, isNoteOn, velocity?].
        .withNativeFunction ("uiMidi", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
                processorRef.handleUiMidi ((int) args[0], (bool) args[1],
                                           args.size() >= 3 ? (float) args[2] : 0.8f);
            complete (juce::var());
        })
        // Lesson preset tour — applies a full APVTS snapshot; relays sync the UI.
        .withNativeFunction ("applyFactoryPreset", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0)
                processorRef.applyFactoryPreset (args[0].toString());
            complete (juce::var());
        })
        // ── Interface-language pair (v1.1.0) ───────────────────────────────
        // Plain withNativeFunction, no relay: the language is not a parameter
        // and must not reach a DAW automation lane. PULLED once by the page at
        // init; nothing pushes from here or from the 30 Hz timer, and
        // applyFactoryPreset sets parameters only, so no preset path can change
        // it behind the page's back.
        //
        // The WebView is never hidden in this editor (no setVisible anywhere in
        // Source/), so these completions always settle
        // (critical_webview_completion_gated_on_isvisible).
        .withNativeFunction ("getUiLanguage", [this] (auto&, auto complete) {
            complete (juce::var (OSimpleAdditiveAudioProcessor::languageCode (
                                     processorRef.uiLanguage.load (std::memory_order_acquire))));
        })
        .withNativeFunction ("setUiLanguage", [this] (const juce::Array<juce::var>& args, auto complete) {
            // languageIndex() maps anything that is not "fr" to 0, so an
            // unexpected argument from the page degrades to English rather than
            // being stored unvalidated.
            if (args.size() > 0)
                processorRef.uiLanguage.store (
                    OSimpleAdditiveAudioProcessor::languageIndex (args[0].toString()),
                    std::memory_order_release);

            complete (juce::var (OSimpleAdditiveAudioProcessor::languageCode (
                                     processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

   #if JUCE_WINDOWS
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OsimpleAdditive_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS (after the WebView) ------------------------------------
    for (int i = 0; i < sliderIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (sliderIds[i]);
        jassert (param != nullptr);   // ID drift → silently dead control; catch in debug
        if (param != nullptr)
            sliderAttachments.push_back (
                std::make_unique<juce::WebSliderParameterAttachment> (
                    *param, *sliderRelays[(size_t) i], nullptr));
    }

    for (int i = 0; i < comboIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (comboIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            comboAttachments.push_back (
                std::make_unique<juce::WebComboBoxParameterAttachment> (
                    *param, *comboRelays[(size_t) i], nullptr));
    }

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // 860 wide fits the 16-drawbar bay. 930 tall fits the whole page with room to
    // spare: the v1.0.6 layout measures 892px of content in 924px of usable frame
    // height, so the on-screen keyboard sits above the fold with ~32px of margin —
    // far more than the <=2px the layout shifts across serif fallbacks. The frame
    // still scrolls if a host gives us less height than we asked for.
    //
    // Read BEFORE the first setSize: resized() records the live scale.
    const float storedScale = processorRef.editorScale;

    // The 860x930 design frame — the page lays out at exactly this size and
    // scales itself (CSS transform on .frame) to whatever the window is. The
    // headless UI gates parse this literal, so it stays the numeric setSize.
    setSize (860, 930);

    // v1.4.0 (R7): resizable at a fixed 860:930 aspect, 0.6x..1.5x. 930px is
    // off-screen on a 13" laptop, so a fresh editor fits the display it opens
    // on; a size the user chose is kept for the rest of the session.
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (designW * minScale), juce::roundToInt (designH * minScale),
                     juce::roundToInt (designW * maxScale), juce::roundToInt (designH * maxScale));
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio ((double) designW / (double) designH);

    const float scale = storedScale > 0.0f ? storedScale : fitScaleForDisplay();
    if (! juce::approximatelyEqual (scale, 1.0f))
        setSize (juce::roundToInt (designW * scale), juce::roundToInt (designH * scale));

    startTimerHz (30);
}

OSimpleAdditiveAudioProcessorEditor::~OSimpleAdditiveAudioProcessorEditor()
{
    stopTimer();
}

// ── Timer: scope + live drawbar spectrum → page ─────────────────────────────
void OSimpleAdditiveAudioProcessorEditor::timerCallback()
{
    // Scope downsample (message thread; the audio thread only copies into VizRing).
    vizAnalyzer.process (processorRef.getVizRing(), processorRef.getCurrentSampleRate());

    if (webView == nullptr)
        return;

    // Live drawbar spectrum: the 16 morphed + decayed amplitudes of the newest
    // sounding voice. Paired with a "sounding" flag so the page can snap the glow
    // to each drawbar's set level when nothing plays (avoids a stale snapshot).
    float active[AdditiveVoice::kNumPartials];
    processorRef.readActiveSpectrum (active);

    juce::Array<juce::var> levels;
    levels.ensureStorageAllocated (AdditiveVoice::kNumPartials);
    for (float v : active) levels.add (v);

    auto* spectrumObj = new juce::DynamicObject();
    spectrumObj->setProperty ("sounding", processorRef.isSounding());
    spectrumObj->setProperty ("levels",  juce::var (std::move (levels)));

    const auto& scope = vizAnalyzer.getScope();   // 128 pts, [-1, 1]
    juce::Array<juce::var> scopeArr;
    scopeArr.ensureStorageAllocated ((int) scope.size());
    for (float v : scope) scopeArr.add (v);

    webView->emitEventIfBrowserIsVisible ("drawbarSpectrumUpdate", juce::var (spectrumObj));
    webView->emitEventIfBrowserIsVisible ("scopeUpdate",           juce::var (std::move (scopeArr)));
}

// ── Layout ──────────────────────────────────────────────────────────────────
void OSimpleAdditiveAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills the editor — nothing to paint.
}

void OSimpleAdditiveAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());

    processorRef.editorScale = (float) getWidth() / (float) designW;
}

// Largest scale <= 1 whose window fits the usable height of the display under
// the mouse (where the host is about to open the editor), leaving room for the
// host's own plugin-window title bar and header.
float OSimpleAdditiveAudioProcessorEditor::fitScaleForDisplay()
{
    constexpr int hostChromeAllowance = 90;

    const auto& displays = juce::Desktop::getInstance().getDisplays();
    const auto* display = displays.getDisplayForPoint (juce::Desktop::getMousePosition());
    if (display == nullptr)
        display = displays.getPrimaryDisplay();
    if (display == nullptr)
        return 1.0f;

    const int available = display->userArea.getHeight() - hostChromeAllowance;
    return juce::jlimit (minScale, 1.0f, (float) available / (float) designH);
}
