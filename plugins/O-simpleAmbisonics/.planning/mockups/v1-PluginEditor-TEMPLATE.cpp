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

    O-simpleAmbisonics - Plugin Editor (implementation)  (TEMPLATE — mockup v1)
    Ouaricon Audio
    Developer: Taylor Brook

    Reference for gui-agent (Stage 3). Mirrors O-simpleWavetable (relays,
    natives, bare-path resource provider, 3-arg attachments, v1.0.1 gesture
    close, WebView2 user-data folder on Windows) and O-simpleFM /
    O-simpleAdditive (fixed-aspect resizable frame, editorScale, viz Timer).

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

    // ── meterFrame quantisation + hash ──────────────────────────────────────
    // The page animates ballistics on requestAnimationFrame, so a 1e-3 step on
    // gains / peaks and 0.01 deg on angles is below anything it can draw. The
    // emitted values ARE the quantised ones, so the hash and the payload agree.
    inline int q3  (float v) noexcept { return (int) std::lround ((double) v * 1000.0); }   // gains, peaks, mix
    inline int q2d (float v) noexcept { return (int) std::lround ((double) v * 100.0); }    // degrees -> 0.01 deg

    inline juce::uint64 fnv1a (juce::uint64 h, int v) noexcept
    {
        auto u = (juce::uint32) v;
        for (int i = 0; i < 4; ++i)
        {
            h ^= (u & 0xffu);
            h *= 1099511628211ULL;
            u >>= 8;
        }
        return h;
    }

    // Writes "k:v" numbers as fixed-point text without allocating per field.
    inline void appendFixed (juce::String& s, int scaled, int decimals)
    {
        const bool neg = scaled < 0;
        auto a = (juce::int64) (neg ? -(juce::int64) scaled : scaled);
        juce::int64 div = 1;
        for (int i = 0; i < decimals; ++i) div *= 10;
        if (neg) s << '-';
        s << (juce::int64) (a / div) << '.';
        auto frac = juce::String ((juce::int64) (a % div));
        for (int i = frac.length(); i < decimals; ++i) s << '0';
        s << frac;
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

    framePayload.preallocateBytes (1600);   // 32 floats + 6 scalars as text; grows once at most

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
    // again whenever it becomes visible. emitEventIfBrowserIsVisible drops
    // events while the browser is hidden, so the frame is re-sent on request
    // rather than assumed delivered. The language is PULLED by the canon, so
    // nothing is pushed here.
    options = options.withNativeFunction ("uiReady",
        [this] (auto&, auto complete)
        {
            forceFrameEmit = true;
            emitMeterFrame (true);
            complete (juce::var (true));
        });

    // Lesson presets by STABLE id (the button captions localize; the ids do not):
    // pointSource, orderStep, behindYou, overhead, rotateRoom, farNear,
    // slowOrbit, speakersHeadphones. The processor resets every parameter to its
    // default first, applies the recipe through begin/endChangeGesture +
    // setValueNotifyingHost, and NEVER touches outputLevel; the relays sync the
    // controls back. Message thread, APVTS only (CONTEXT.md constraints).
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
    const float storedScale = processorRef.editorScale;

    // The 1180 x 780 design frame — the page lays out at exactly this size and
    // scales itself (CSS transform on #plugin) to whatever the window is. The
    // headless UI gates parse this literal, so it stays the numeric setSize and
    // the LAST numeric setSize in the file.
    setSize (1180, 780);

    // Resizable at a fixed 1180:780 aspect, 0.6x..1.5x (UI-07: a 13" laptop
    // fits it unscaled, a projector blows it up). A fresh editor fits the
    // display it opens on; a size the user chose is kept for the session.
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (designW * minScale), juce::roundToInt (designH * minScale),
                     juce::roundToInt (designW * maxScale), juce::roundToInt (designH * maxScale));
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio ((double) designW / (double) designH);

    const float scale = storedScale > 0.0f ? storedScale : fitScaleForDisplay();
    if (! juce::approximatelyEqual (scale, 1.0f))
        setSize (juce::roundToInt (designW * scale), juce::roundToInt (designH * scale));

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
// Payload (MeterFrame.h field names, so the page and the header agree):
//   { shGain:[16], shPeak:[16], azEffDeg, elEffDeg, azRotDeg, elRotDeg,
//     orderActive, decoderMix, outPeakL, outPeakR, latencySamples }
// Sent as ONE JSON string (the page JSON.parse()s it): a juce::var tree of 32
// numbers would be ~40 small allocations per tick; the string is one, reused.
// The Stage-3 research item "var vs pre-formatted string" may overturn this.
void OSimpleAmbisonicsAudioProcessorEditor::emitMeterFrame (bool force)
{
    if (webView == nullptr)
        return;

    // read() every tick (whether or not we emit): the shPeak accumulate-then-
    // reset rule keys off hasNewData() having been consumed by the reader.
    const auto& f = processorRef.meterFrames.read();

    // Quantise, hashing as we go.
    int gain[16], peak[16];
    juce::uint64 h = 1469598103934665603ULL;
    for (int i = 0; i < 16; ++i) { gain[i] = q3 (f.shGain[i]); h = fnv1a (h, gain[i]); }
    for (int i = 0; i < 16; ++i) { peak[i] = q3 (f.shPeak[i]); h = fnv1a (h, peak[i]); }
    const int azEff = q2d (f.azEffDeg), elEff = q2d (f.elEffDeg);
    const int azRot = q2d (f.azRotDeg), elRot = q2d (f.elRotDeg);
    const int mix = q3 (f.decoderMix), outL = q3 (f.outPeakL), outR = q3 (f.outPeakR);
    for (int v : { azEff, elEff, azRot, elRot, f.orderActive, mix, outL, outR, f.latencySamples })
        h = fnv1a (h, v);

    if (! force && h == lastFrameHash)
        return;                                   // a static scene emits nothing
    lastFrameHash = h;

    auto& s = framePayload;
    s.clear();
    s << "{\"shGain\":[";
    for (int i = 0; i < 16; ++i) { if (i) s << ','; appendFixed (s, gain[i], 3); }
    s << "],\"shPeak\":[";
    for (int i = 0; i < 16; ++i) { if (i) s << ','; appendFixed (s, peak[i], 3); }
    s << "],\"azEffDeg\":";    appendFixed (s, azEff, 2);
    s << ",\"elEffDeg\":";     appendFixed (s, elEff, 2);
    s << ",\"azRotDeg\":";     appendFixed (s, azRot, 2);
    s << ",\"elRotDeg\":";     appendFixed (s, elRot, 2);
    s << ",\"orderActive\":"   << f.orderActive;
    s << ",\"decoderMix\":";   appendFixed (s, mix, 3);
    s << ",\"outPeakL\":";     appendFixed (s, outL, 3);
    s << ",\"outPeakR\":";     appendFixed (s, outR, 3);
    s << ",\"latencySamples\":" << f.latencySamples << '}';

    webView->emitEventIfBrowserIsVisible ("meterFrame", juce::var (s));
}

// ── Timer (30 Hz, message thread) ───────────────────────────────────────────
void OSimpleAmbisonicsAudioProcessorEditor::timerCallback()
{
    emitMeterFrame (std::exchange (forceFrameEmit, false));
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

    processorRef.editorScale = (float) getWidth() / (float) designW;
}

// Largest scale <= 1 whose window fits the usable height of the display under
// the mouse (where the host is about to open the editor), leaving room for the
// host's own plugin-window title bar and header.
float OSimpleAmbisonicsAudioProcessorEditor::fitScaleForDisplay()
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
