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

    O-simpleAmbisonics - Plugin Editor (implementation)
    Ouaricon Audio
    Developer: Taylor Brook

    Mirrors O-simpleWavetable v1.0.1 (relays, natives, bare-path resource
    provider, 3-arg attachments, gesture close on destruction, WebView2
    user-data folder on Windows) and O-simpleAdditive (fixed-aspect resizable
    frame). The meterFrame text comes from MeterFramePayload.h, shared with
    the render harness so the UI fixtures are the editor's exact bytes.

    BinaryData.h is included in THIS file only: tests/render-harness compiles
    PluginProcessor.cpp with JUCE_WEB_BROWSER=0 and without the UI binary-data
    library, so nothing outside the editor TU may include it. createEditor()
    in PluginProcessor.cpp includes PluginEditor.h under #if JUCE_WEB_BROWSER
    and returns the Generic editor in the #else.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace
{
    constexpr int kTimerHz = 30;

    // Usable height of the display under the mouse (where the host is about to
    // open the editor), minus room for the host's own plugin-window title bar
    // and header. 0 when no display is known.
    float availableDisplayHeight()
    {
        constexpr float hostChromeAllowance = 90.0f;

        const auto& displays = juce::Desktop::getInstance().getDisplays();
        const auto* display = displays.getDisplayForPoint (juce::Desktop::getMousePosition().toFloat());
        if (display == nullptr)
            display = displays.getPrimaryDisplay();
        if (display == nullptr)
            return 0.0f;

        return display->userBounds.getHeight() - hostChromeAllowance;
    }

    auto makeBinaryResource (const char* data, int size, const char* mimeType)
        -> std::optional<juce::WebBrowserComponent::Resource>
    {
        auto* bytes = reinterpret_cast<const std::byte*> (data);
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte> (bytes, bytes + size),
            juce::String (mimeType)
        };
    }

    // 3-arg attachment (nullptr UndoManager), or nullptr + a debug assert on ID
    // drift — a missing parameter is a silently dead control, never a crash.
    template <typename Attachment, typename Relay>
    std::unique_ptr<Attachment> attach (juce::AudioProcessorValueTreeState& apvts,
                                        const char* paramId, Relay& relay)
    {
        auto* p = apvts.getParameter (paramId);
        jassert (p != nullptr);
        return p != nullptr ? std::make_unique<Attachment> (*p, relay, nullptr) : nullptr;
    }
}

// ── Resource provider ───────────────────────────────────────────────────────
// WKWebView / WebView2 hand the callback a BARE PATH ("/", "/index.html",
// "/js/i18n.js"). Compare by string equality — never strip a scheme/host.
// charset=utf-8 on every text resource (the page uses non-ASCII glyphs).
// Every file listed in juce_add_binary_data(O-simpleAmbisonics_UIResources)
// has a branch here; a missing one is a silent 404 (the font falls back to
// Times, the i18n import fails and the page is BLANK).
std::optional<juce::WebBrowserComponent::Resource>
OSimpleAmbisonicsAudioProcessorEditor::getResource (const juce::String& url)
{
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (BinaryData::index_html, BinaryData::index_htmlSize, "text/html; charset=utf-8");

    // The copy table the inline controller imports (check-i18n [8]): a missing
    // branch is a BLANK page, because module evaluation never starts.
    if (url == "/js/i18n.js")
        return makeBinaryResource (BinaryData::i18n_js, BinaryData::i18n_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (BinaryData::index_js, BinaryData::index_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (BinaryData::check_native_interop_js,
                                   BinaryData::check_native_interop_jsSize, "application/javascript; charset=utf-8");

    if (url == "/img/insects.png")
        return makeBinaryResource (BinaryData::insects_png, BinaryData::insects_pngSize, "image/png");

    // Shared EB Garamond face (modules/ui/eb-garamond): stylesheet under /css/,
    // faces under /fonts/ where its relative url('../fonts/...') lands. Hyphens
    // are STRIPPED from the BinaryData symbols (critical_binary_data_strips_hyphens).
    if (url == "/css/eb-garamond.css")
        return makeBinaryResource (BinaryData::ebgaramond_css, BinaryData::ebgaramond_cssSize, "text/css; charset=utf-8");
    if (url == "/fonts/EBGaramond-Regular.woff2")
        return makeBinaryResource (BinaryData::EBGaramondRegular_woff2, BinaryData::EBGaramondRegular_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Italic.woff2")
        return makeBinaryResource (BinaryData::EBGaramondItalic_woff2, BinaryData::EBGaramondItalic_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Bold.woff2")
        return makeBinaryResource (BinaryData::EBGaramondBold_woff2, BinaryData::EBGaramondBold_woff2Size, "font/woff2");

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
OSimpleAmbisonicsAudioProcessorEditor::OSimpleAmbisonicsAudioProcessorEditor (OSimpleAmbisonicsAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      // 1) RELAYS — initialised in DECLARATION order, before the WebView exists.
      //    The ids are the frozen ParamIDs; never a retyped string.
      azimuthRelay     (std::make_unique<juce::WebSliderRelay>       (OSimpleAmbisonics::ParamIDs::azimuth)),
      elevationRelay   (std::make_unique<juce::WebSliderRelay>       (OSimpleAmbisonics::ParamIDs::elevation)),
      distanceRelay    (std::make_unique<juce::WebSliderRelay>       (OSimpleAmbisonics::ParamIDs::distance)),
      orderRelay       (std::make_unique<juce::WebComboBoxRelay>     (OSimpleAmbisonics::ParamIDs::order)),
      yawRelay         (std::make_unique<juce::WebSliderRelay>       (OSimpleAmbisonics::ParamIDs::yaw)),
      pitchRelay       (std::make_unique<juce::WebSliderRelay>       (OSimpleAmbisonics::ParamIDs::pitch)),
      rollRelay        (std::make_unique<juce::WebSliderRelay>       (OSimpleAmbisonics::ParamIDs::roll)),
      decoderRelay     (std::make_unique<juce::WebComboBoxRelay>     (OSimpleAmbisonics::ParamIDs::decoder)),
      orbitRelay       (std::make_unique<juce::WebToggleButtonRelay> (OSimpleAmbisonics::ParamIDs::orbit)),
      orbitRateRelay   (std::make_unique<juce::WebSliderRelay>       (OSimpleAmbisonics::ParamIDs::orbitRate)),
      inputModeRelay   (std::make_unique<juce::WebComboBoxRelay>     (OSimpleAmbisonics::ParamIDs::inputMode)),
      outputLevelRelay (std::make_unique<juce::WebSliderRelay>       (OSimpleAmbisonics::ParamIDs::outputLevel))
{
    namespace ids = OSimpleAmbisonics::ParamIDs;

    // 2) WEBVIEW options -------------------------------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); })
        // Every relay — slider, combo AND toggle. A toggle relay left out here
        // makes getToggleState() on the page bind to a state nothing updates.
        .withOptionsFrom (*azimuthRelay)
        .withOptionsFrom (*elevationRelay)
        .withOptionsFrom (*distanceRelay)
        .withOptionsFrom (*orderRelay)
        .withOptionsFrom (*yawRelay)
        .withOptionsFrom (*pitchRelay)
        .withOptionsFrom (*rollRelay)
        .withOptionsFrom (*decoderRelay)
        .withOptionsFrom (*orbitRelay)
        .withOptionsFrom (*orbitRateRelay)
        .withOptionsFrom (*inputModeRelay)
        .withOptionsFrom (*outputLevelRelay);

    // -- Native functions ------------------------------------------------------
    // EVERY name the page calls through getNativeFunction() is registered here.
    // 5 names: getParameterDefaults, getUiLanguage, setUiLanguage, uiReady,
    // applyFactoryPreset. (The preset-manager natives arrive with the FUNC-08
    // bank in Stage 4.)

    // dblclick reset: the relay's properties payload carries start / end / skew
    // but no default, and a JS default table would drift from C++. Engineering
    // units for every parameter; the page converts back through scaledToNorm().
    options = options.withNativeFunction ("getParameterDefaults",
        [this] (auto&, auto complete)
        {
            auto* obj = new juce::DynamicObject();
            static constexpr const char* all[] = {
                ids::azimuth, ids::elevation, ids::distance, ids::order, ids::yaw, ids::pitch,
                ids::roll, ids::decoder, ids::orbit, ids::orbitRate, ids::inputMode, ids::outputLevel };
            for (const auto* paramId : all)
                if (auto* param = processorRef.apvts.getParameter (paramId))
                    obj->setProperty (paramId, param->convertFrom0to1 (param->getDefaultValue()));
            complete (juce::var (obj));
        });

    // Interface language: plain natives, no relay — the language is not a
    // parameter and must not reach an automation lane. The page PULLS it once at
    // init (the canon's initI18n). The processor side (uiLanguage + the codec +
    // the persisted state property) is listed in the integration checklist §2.
    options = options.withNativeFunction ("getUiLanguage",
        [this] (auto&, auto complete)
        {
            complete (juce::var (OSimpleAmbisonicsAudioProcessor::languageCode (
                processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

    options = options.withNativeFunction ("setUiLanguage",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            // languageIndex() maps anything that is not "fr" or "zh-Hans" to en,
            // so an unexpected argument degrades to English, never stored raw.
            if (args.size() > 0)
                processorRef.uiLanguage.store (
                    OSimpleAmbisonicsAudioProcessor::languageIndex (args[0].toString()),
                    std::memory_order_release);
            complete (juce::var());
        });

    // The page's handshake: called once its meterFrame listener exists, and
    // again whenever it becomes visible, so the frame is re-sent on request
    // rather than assumed delivered. The language is PULLED by the canon, so
    // nothing is pushed here.
    options = options.withNativeFunction ("uiReady",
        [this] (auto&, auto complete)
        {
            // Emits the stored `current` — no read(), no tracker tick (the
            // timer is the only sampler; a handshake is never a stale tick).
            emitFrame (true);
            complete (juce::var (true));
        });

    // Lesson presets by STABLE id (the button captions localize; the ids do not):
    // pointSource, orderStep, behindYou, overhead, rotateRoom, farNear,
    // slowOrbit, speakersHeadphones. One pass in the processor: recipe value or
    // default for every parameter EXCEPT outputLevel (never reset, never set),
    // skip-if-equal, begin / setValueNotifyingHost / end per changed parameter;
    // the relays sync the controls back. Message thread, APVTS only.
    options = options.withNativeFunction ("applyFactoryPreset",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            const bool ok = args.size() > 0 && args[0].isString()
                         && processorRef.applyFactoryPreset (args[0].toString());
            forceFrameEmit = true;           // the puck / bars jump: send the next frame unconditionally
            complete (juce::var (ok));
        });

   #if JUCE_WINDOWS
    // WebView2 user-data folder ALWAYS set to a writable temp dir: hosts deny
    // the default location -> silent fallback -> blank page. Static linking is
    // JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 in CMakeLists (already set).
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                                     .getChildFile ("OsimpleAmbisonics_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3) ATTACHMENTS — after the WebView, same order as the declarations.
    //    3-arg constructor (nullptr UndoManager) through attach(), which asserts
    //    on ID drift in debug and leaves the control unbound in release.
    auto& apvts = processorRef.apvts;
    azimuthAttachment     = attach<juce::WebSliderParameterAttachment>       (apvts, ids::azimuth,     *azimuthRelay);
    elevationAttachment   = attach<juce::WebSliderParameterAttachment>       (apvts, ids::elevation,   *elevationRelay);
    distanceAttachment    = attach<juce::WebSliderParameterAttachment>       (apvts, ids::distance,    *distanceRelay);
    orderAttachment       = attach<juce::WebComboBoxParameterAttachment>     (apvts, ids::order,       *orderRelay);
    yawAttachment         = attach<juce::WebSliderParameterAttachment>       (apvts, ids::yaw,         *yawRelay);
    pitchAttachment       = attach<juce::WebSliderParameterAttachment>       (apvts, ids::pitch,       *pitchRelay);
    rollAttachment        = attach<juce::WebSliderParameterAttachment>       (apvts, ids::roll,        *rollRelay);
    decoderAttachment     = attach<juce::WebComboBoxParameterAttachment>     (apvts, ids::decoder,     *decoderRelay);
    orbitAttachment       = attach<juce::WebToggleButtonParameterAttachment> (apvts, ids::orbit,       *orbitRelay);
    orbitRateAttachment   = attach<juce::WebSliderParameterAttachment>       (apvts, ids::orbitRate,   *orbitRateRelay);
    inputModeAttachment   = attach<juce::WebComboBoxParameterAttachment>     (apvts, ids::inputMode,   *inputModeRelay);
    outputLevelAttachment = attach<juce::WebSliderParameterAttachment>       (apvts, ids::outputLevel, *outputLevelRelay);

    // v1.0.1 gesture tracking for the destructor (see PluginEditor.h).
    jassert (processorRef.getParameters().size() == (int) gestureOpen.size());
    for (auto* param : processorRef.getParameters())
        param->addListener (this);

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Read BEFORE the first setSize: resized() records the live scale.
    const float storedScale = processorRef.editorScale.load (std::memory_order_relaxed);

    // The 1180 x 780 design frame — the page lays out at exactly this size and
    // scales itself (CSS transform on #plugin) to whatever the window is. The
    // headless UI gates parse this literal, so it stays the numeric setSize and
    // the LAST numeric setSize in the file.
    setSize (1180, 780);

    // Resizable at a fixed 1180:780 aspect, 0.6x..1.5x (UI-07: a 13" laptop
    // fits it unscaled, a projector blows it up). A fresh editor fits the
    // display it opens on; a size the user chose persists in the session file,
    // clamped to what THIS display fits (a 1.5x projector size reopened on a
    // laptop must not open off-screen).
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (designW * minScale), juce::roundToInt (designH * minScale),
                     juce::roundToInt (designW * maxScale), juce::roundToInt (designH * maxScale));
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio ((double) designW / (double) designH);

    const float scale = storedScale > 0.0f ? juce::jmin (storedScale, fitMaxForDisplay())
                                           : fitScaleForDisplay();
    if (! juce::approximatelyEqual (scale, 1.0f))
        setSize (juce::roundToInt (designW * scale), juce::roundToInt (designH * scale));

    // Drain one read() before the timer starts: with no editor open nothing
    // consumes hasNewData(), so the audio side has been max-accumulating the
    // peaks since the last close (PluginProcessor.cpp reset rule). Then start
    // from an idle frame; the first tick emits unconditionally.
    processorRef.meterFrames.read();
    current        = OSimpleAmbisonics::ui::quantise (OSimpleAmbisonics::dsp::MeterFrame {}, false);
    lastFrameHash  = 0;
    forceFrameEmit = true;

    startTimerHz (kTimerHz);
}

OSimpleAmbisonicsAudioProcessorEditor::~OSimpleAmbisonicsAudioProcessorEditor()
{
    closeOpenGestures();                           // a slider drag cut short by the close
    for (auto* param : processorRef.getParameters())
        param->removeListener (this);
    stopTimer();
}

// Any thread per the JUCE contract; in this plugin every gesture source is the
// message thread (the attachments, applyFactoryPreset).
void OSimpleAmbisonicsAudioProcessorEditor::parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
{
    if (parameterIndex >= 0 && parameterIndex < (int) gestureOpen.size())
        gestureOpen[(size_t) parameterIndex].store (gestureIsStarting, std::memory_order_relaxed);
}

void OSimpleAmbisonicsAudioProcessorEditor::closeOpenGestures()
{
    const auto& params = processorRef.getParameters();
    for (int i = 0; i < params.size() && i < (int) gestureOpen.size(); ++i)
        if (gestureOpen[(size_t) i].exchange (false, std::memory_order_relaxed))
            params[i]->endChangeGesture();
}

// ── meterFrame: the one C++ -> page push ─────────────────────────────────────
// Payload (MeterFrame.h field names + `live`), formatted by MeterFramePayload.h:
//   { shGain:[16], shPeak:[16], azEffDeg, elEffDeg, azRotDeg, elRotDeg,
//     orderActive, decoderMix, outPeakL, outPeakR, latencySamples, live }
// Sent as ONE JSON string (the page JSON.parse()s it).
void OSimpleAmbisonicsAudioProcessorEditor::emitFrame (bool force)
{
    if (webView == nullptr)
        return;

    const auto h = OSimpleAmbisonics::ui::hash (current);
    if (! force && h == lastFrameHash)
        return;                                   // a static scene emits nothing

    const int len = OSimpleAmbisonics::ui::format (current, payloadBuf, (int) sizeof (payloadBuf));
    if (len <= 0)
        return;                                   // cannot happen within the clamp (harness PL1)

    lastFrameHash = h;
    webView->emitEventIfBrowserIsVisible ("meterFrame",
                                          juce::var (juce::String::fromUTF8 (payloadBuf, len)));
}

// ── Timer (30 Hz, message thread) — the only sampler ────────────────────────
// sampleFrame() polls hasNewData() BEFORE read() (the read consumes it; the
// audio side's peak reset keys off that), applies the U1 live rule and
// quantises. Read every tick whether or not anything is emitted.
void OSimpleAmbisonicsAudioProcessorEditor::timerCallback()
{
    current = OSimpleAmbisonics::ui::sampleFrame (processorRef.meterFrames, liveTracker);
    emitFrame (std::exchange (forceFrameEmit, false));
}

// ── Layout ──────────────────────────────────────────────────────────────────
void OSimpleAmbisonicsAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills the editor — nothing to paint.
}

void OSimpleAmbisonicsAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());

    processorRef.editorScale.store ((float) getWidth() / (float) designW, std::memory_order_relaxed);
}

// Largest scale <= 1 whose window fits the usable height of the display under
// the mouse (where the host is about to open the editor), leaving room for the
// host's own plugin-window title bar and header.
float OSimpleAmbisonicsAudioProcessorEditor::fitScaleForDisplay()
{
    const float available = availableDisplayHeight();
    if (available <= 0.0f)
        return 1.0f;
    return juce::jlimit (minScale, 1.0f, available / (float) designH);
}

// Largest scale <= maxScale that fits the same display: the ceiling for a
// stored (persisted) scale at open.
float OSimpleAmbisonicsAudioProcessorEditor::fitMaxForDisplay()
{
    const float available = availableDisplayHeight();
    if (available <= 0.0f)
        return 1.0f;
    return juce::jlimit (minScale, maxScale, available / (float) designH);
}
