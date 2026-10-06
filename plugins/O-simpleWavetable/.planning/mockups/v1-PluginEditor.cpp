/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - Plugin Editor (implementation)  (TEMPLATE — mockup v1)

    gui-agent reference for Stage 3. Mirrors O-simpleAdditive / O-simpleGrain:
    bare-path resource provider, the `Juce` namespace on the page, 3-arg
    attachments, WebView2 user-data folder always set on Windows.

    ── PROCESSOR CONTRACT this template assumes (built in Stages 2.2–2.4/3.2) ──
    Names are proposals; keep the SHAPES. Everything is message-thread only
    unless stated.

      juce::AudioProcessorValueTreeState& getAPVTS();
      std::atomic<int> uiLanguage;                       // non-parameter state
      static juce::String languageCode  (int);           // 1 → "fr", 2 → "zh-Hans", else "en"
      static int          languageIndex (const juce::String&);  // unknown → 0 (en)

      void handleUiMidi (int note, bool isOn, float velocity);   // MidiMessageCollector
      bool applyFactoryPreset (const juce::String& lessonId);    // steppedSmooth, aliasDemo,
                                                                 // driveSweep, vowelPad, ppg8bit
      // Import (ARCHITECTURE §A8; worker thread → callAsync publish):
      bool importFromFile   (const juce::File&);                    // false = refused to start
      bool importFromMemory (const juce::String& name, juce::MemoryBlock&&);
      struct ImportStatus { enum class State { idle, busy, done, error };
                            State state; juce::String filename; int frames;
                            juce::String error; };                  // "tooShort" | "unreadable" | "tooLarge" | ...
      ImportStatus  getImportStatus() const;
      juce::uint32  getImportStatusVersion() const;   // bumps on every transition

      // Bank panel (bumps on bank-param change, import publish, state restore):
      juce::uint32 getBankDisplayGeneration() const;
      struct BankThumbs { int bank; bool imported; int numFrames;
                          juce::String filename; std::vector<float> points; };   // numFrames * 128
      void getBankThumbnails (BankThumbs&) const;

      // Cycle panel (ARCHITECTURE §Visualization Data Path — the message thread
      // renders the exact 2048-sample cycle with the voice's own read code,
      // quantizes, FFTs, point-samples to 256):
      struct CycleView { std::array<float, 256> cycle, preQ;
                         std::array<float, 32>  harmonicsDb, harmonicsRawDb;   // dB re heard max, -60 floor
                         float pos; int frame; int level; int kmax; float nyquistH;
                         int note; float f0; float lfo; float menv; float amp;
                         bool sounding; bool quantized; };
      void buildCycleView (CycleView&);

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    // ── Parameter IDs (ARCHITECTURE.md §Parameter Mapping / parameter-spec.md) ──
    // Every id that getParameterDefaults reports. Sliders report the engineering
    // default; choices and bools report convertFrom0to1(default) = the index / 0|1,
    // which is what the page's stepped knobs reset to.
    constexpr const char* kAllParamIds[] = {
        "bank", "position", "interp", "bandlimit", "bit_depth",
        "lfo_rate", "lfo_sync", "lfo_div", "lfo_shape", "lfo_depth",
        "menv_attack", "menv_decay", "menv_sustain", "menv_release", "env_amount",
        "amp_attack", "amp_decay", "amp_sustain", "amp_release",
        "voice_mode", "output_level",
    };

    constexpr int    kDesignW          = 1120;
    constexpr int    kDesignH          = 780;
    constexpr int    kTimerHz          = 30;
    constexpr int    kThumbPoints      = 128;
    constexpr size_t kDropMaxBytes     = 96u * 1024u * 1024u;   // matches DROP_MAX_BYTES in the page

    auto makeBinaryResource (const char* data, int size, const char* mimeType)
        -> std::optional<juce::WebBrowserComponent::Resource>
    {
        auto* bytes = reinterpret_cast<const std::byte*> (data);
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte> (bytes, bytes + size),
            juce::String (mimeType)
        };
    }

    juce::var toVarArray (const float* data, size_t n)
    {
        juce::Array<juce::var> arr;
        arr.ensureStorageAllocated ((int) n);
        for (size_t i = 0; i < n; ++i)
            arr.add (data[i]);
        return juce::var (std::move (arr));
    }

    // 3-arg attachment, or nullptr (and a debug assert) on ID drift — a missing
    // parameter is a silently dead control, never a crash.
    template <typename Attachment, typename Relay>
    std::unique_ptr<Attachment> attach (juce::AudioProcessorValueTreeState& apvts,
                                        const char* id, Relay& relay)
    {
        auto* p = apvts.getParameter (id);
        jassert (p != nullptr);
        return p != nullptr ? std::make_unique<Attachment> (*p, relay, nullptr) : nullptr;
    }

    // FNV-1a over the fields that reach the page — the cycleUpdate gate.
    struct Fnv
    {
        juce::uint64 h = 1469598103934665603ull;
        void add (const void* p, size_t n)
        {
            auto* b = static_cast<const unsigned char*> (p);
            for (size_t i = 0; i < n; ++i) { h ^= b[i]; h *= 1099511628211ull; }
        }
        template <typename T> void add (const T& v) { add (&v, sizeof (T)); }
    };

    const char* importStateName (OSimpleWavetableAudioProcessor::ImportStatus::State s)
    {
        using S = OSimpleWavetableAudioProcessor::ImportStatus::State;
        switch (s)
        {
            case S::busy:  return "busy";
            case S::done:  return "done";
            case S::error: return "error";
            case S::idle:
            default:       return "idle";
        }
    }
}

// ── Resource provider ───────────────────────────────────────────────────────
// WKWebView / WebView2 hand the callback a BARE PATH ("/", "/index.html",
// "/js/i18n.js"). Compare by string equality — never strip a scheme/host.
// charset=utf-8 on every text resource (the page uses →, ·, ◆, en dashes).
std::optional<juce::WebBrowserComponent::Resource>
OSimpleWavetableAudioProcessorEditor::getResource (const juce::String& url)
{
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (BinaryData::index_html, BinaryData::index_htmlSize, "text/html; charset=utf-8");

    // The copy table the inline controller imports. A missing branch is a BLANK
    // page (the module import fails, so init() never runs) — check-i18n [8].
    if (url == "/js/i18n.js")
        return makeBinaryResource (BinaryData::i18n_js, BinaryData::i18n_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (BinaryData::index_js, BinaryData::index_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (BinaryData::check_native_interop_js,
                                   BinaryData::check_native_interop_jsSize, "application/javascript; charset=utf-8");

    // webview-drop-streaming (copied to Source/ui/public/modules/ by
    // ouaricon_add_module). The page imports it LAZILY on a drop, so a miss here
    // costs the drop path only — but it is still a miss. Hyphens are stripped
    // from the BinaryData symbol (critical_binary_data_strips_hyphens).
    if (url == "/modules/webview-drop-streaming.js")
        return makeBinaryResource (BinaryData::webviewdropstreaming_js,
                                   BinaryData::webviewdropstreaming_jsSize, "application/javascript; charset=utf-8");

    if (url == "/img/insects.png")
        return makeBinaryResource (BinaryData::insects_png, BinaryData::insects_pngSize, "image/png");

    // Shared EB Garamond face (modules/ui/eb-garamond): stylesheet under /css/,
    // faces under /fonts/ where its relative URLs land (O-simpleAdditive R5).
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
OSimpleWavetableAudioProcessorEditor::OSimpleWavetableAudioProcessorEditor (OSimpleWavetableAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      // 1️⃣ RELAYS — initialised in DECLARATION order, before the WebView exists
      bankRelay        (std::make_unique<juce::WebComboBoxRelay>     ("bank")),
      positionRelay    (std::make_unique<juce::WebSliderRelay>       ("position")),
      interpRelay      (std::make_unique<juce::WebToggleButtonRelay> ("interp")),
      bandlimitRelay   (std::make_unique<juce::WebToggleButtonRelay> ("bandlimit")),
      bitDepthRelay    (std::make_unique<juce::WebComboBoxRelay>     ("bit_depth")),
      lfoRateRelay     (std::make_unique<juce::WebSliderRelay>       ("lfo_rate")),
      lfoSyncRelay     (std::make_unique<juce::WebComboBoxRelay>     ("lfo_sync")),
      lfoDivRelay      (std::make_unique<juce::WebComboBoxRelay>     ("lfo_div")),
      lfoShapeRelay    (std::make_unique<juce::WebComboBoxRelay>     ("lfo_shape")),
      lfoDepthRelay    (std::make_unique<juce::WebSliderRelay>       ("lfo_depth")),
      menvAttackRelay  (std::make_unique<juce::WebSliderRelay>       ("menv_attack")),
      menvDecayRelay   (std::make_unique<juce::WebSliderRelay>       ("menv_decay")),
      menvSustainRelay (std::make_unique<juce::WebSliderRelay>       ("menv_sustain")),
      menvReleaseRelay (std::make_unique<juce::WebSliderRelay>       ("menv_release")),
      envAmountRelay   (std::make_unique<juce::WebSliderRelay>       ("env_amount")),
      ampAttackRelay   (std::make_unique<juce::WebSliderRelay>       ("amp_attack")),
      ampDecayRelay    (std::make_unique<juce::WebSliderRelay>       ("amp_decay")),
      ampSustainRelay  (std::make_unique<juce::WebSliderRelay>       ("amp_sustain")),
      ampReleaseRelay  (std::make_unique<juce::WebSliderRelay>       ("amp_release")),
      voiceModeRelay   (std::make_unique<juce::WebComboBoxRelay>     ("voice_mode")),
      outputLevelRelay (std::make_unique<juce::WebSliderRelay>       ("output_level"))
{
    // 2️⃣ WEBVIEW options -----------------------------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); })
        // Every relay — slider, combo AND toggle. A toggle relay left out here
        // makes getToggleState() on the page bind to a state nothing updates.
        .withOptionsFrom (*bankRelay)
        .withOptionsFrom (*positionRelay)
        .withOptionsFrom (*interpRelay)
        .withOptionsFrom (*bandlimitRelay)
        .withOptionsFrom (*bitDepthRelay)
        .withOptionsFrom (*lfoRateRelay)
        .withOptionsFrom (*lfoSyncRelay)
        .withOptionsFrom (*lfoDivRelay)
        .withOptionsFrom (*lfoShapeRelay)
        .withOptionsFrom (*lfoDepthRelay)
        .withOptionsFrom (*menvAttackRelay)
        .withOptionsFrom (*menvDecayRelay)
        .withOptionsFrom (*menvSustainRelay)
        .withOptionsFrom (*menvReleaseRelay)
        .withOptionsFrom (*envAmountRelay)
        .withOptionsFrom (*ampAttackRelay)
        .withOptionsFrom (*ampDecayRelay)
        .withOptionsFrom (*ampSustainRelay)
        .withOptionsFrom (*ampReleaseRelay)
        .withOptionsFrom (*voiceModeRelay)
        .withOptionsFrom (*outputLevelRelay);

    // ── Native functions ─────────────────────────────────────────────────────
    // EVERY name the page calls through getNativeFunction() is registered here;
    // an unregistered one never settles its promise and its control is silently
    // dead while build, auval and pluginval all pass. The page calls:
    //   getParameterDefaults, getUiLanguage, setUiLanguage, uiReady,
    //   importAudio, importDroppedAudio, uiMidi, applyFactoryPreset.

    // dblclick reset: the relay's properties carry no default, and a JS default
    // table would drift from C++.
    options = options.withNativeFunction ("getParameterDefaults",
        [this] (auto&, auto complete)
        {
            auto* obj = new juce::DynamicObject();
            for (const auto* id : kAllParamIds)
                if (auto* param = processorRef.getAPVTS().getParameter (id))
                    obj->setProperty (id, param->convertFrom0to1 (param->getDefaultValue()));
            complete (juce::var (obj));
        });

    // Interface language: plain natives, no relay — the language is not a
    // parameter and must not reach an automation lane. The page PULLS it once.
    options = options.withNativeFunction ("getUiLanguage",
        [this] (auto&, auto complete)
        {
            complete (juce::var (OSimpleWavetableAudioProcessor::languageCode (
                processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

    options = options.withNativeFunction ("setUiLanguage",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            // languageIndex() maps anything that is not "fr" or "zh-Hans" to en,
            // so an unexpected argument degrades to English, never stored raw.
            if (args.size() > 0)
                processorRef.uiLanguage.store (
                    OSimpleWavetableAudioProcessor::languageIndex (args[0].toString()),
                    std::memory_order_release);
            complete (juce::var());
        });

    // The page's handshake: called once its event listeners exist, and again
    // whenever it becomes visible. emitEventIfBrowserIsVisible DROPS events while
    // the browser is hidden, so the bank, the import status and the next cycle
    // are re-sent on request rather than assumed delivered.
    options = options.withNativeFunction ("uiReady",
        [this] (auto&, auto complete)
        {
            emitBankUpdate();
            emitImportStatus();
            forceCycleEmit = true;
            complete (juce::var (true));
        });

    // Import button → native FileChooser. Completes IMMEDIATELY: progress and
    // the result reach the page as importStatus / bankUpdate events.
    options = options.withNativeFunction ("importAudio",
        [this] (auto&, auto complete)
        {
            complete (juce::var (true));

            auto chooser = std::make_shared<juce::FileChooser> (
                "Import audio", juce::File{}, "*.wav;*.wave;*.aif;*.aiff;*.flac;*.ogg");
            juce::Component::SafePointer<OSimpleWavetableAudioProcessorEditor> safeThis (this);

            chooser->launchAsync (juce::FileBrowserComponent::openMode
                                    | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser] (const juce::FileChooser& fc)
                {
                    // Editor gone while the dialog was open: bail WITHOUT touching
                    // the bridge (pattern_webview_launchasync_safepointer_no_complete).
                    if (safeThis == nullptr)
                        return;
                    const auto file = fc.getResult();
                    if (file.existsAsFile())
                        safeThis->processorRef.importFromFile (file);
                });
        });

    // Drop path: WKWebView strips file paths, so the page sends the bytes as
    // standard base64 (webview-drop-streaming's arrayBufferToBase64). Decode with
    // Base64::convertFromBase64 — NOT MemoryBlock::fromBase64Encoding, which is
    // JUCE's own format and rejects btoa() output.
    options = options.withNativeFunction ("importDroppedAudio",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() < 2)
            {
                complete (juce::var (false));
                return;
            }

            const auto name   = args[0].toString();
            const auto base64 = args[1].toString();
            if (name.isEmpty() || base64.isEmpty()
                || (size_t) base64.getNumBytesAsUTF8() > kDropMaxBytes / 3 * 4 + 4)
            {
                complete (juce::var (false));
                return;
            }

            juce::MemoryOutputStream decoded;
            if (! juce::Base64::convertFromBase64 (decoded, base64))
            {
                complete (juce::var (false));
                return;
            }

            complete (juce::var (processorRef.importFromMemory (name, decoded.getMemoryBlock())));
        });

    // On-screen + computer keyboard → synth. args: [note, isNoteOn, velocity?].
    options = options.withNativeFunction ("uiMidi",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() >= 2)
                processorRef.handleUiMidi ((int) args[0], (bool) args[1],
                                           args.size() >= 3 ? (float) args[2] : 0.8f);
            complete (juce::var());
        });

    // Lesson presets by STABLE id (the button captions localize; the ids do not).
    // Applies a full parameter snapshot (reset to defaults first, never sets
    // output_level); the relays sync every control back to the page.
    options = options.withNativeFunction ("applyFactoryPreset",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            const bool ok = args.size() > 0 && processorRef.applyFactoryPreset (args[0].toString());
            complete (juce::var (ok));
        });

   #if JUCE_WINDOWS
    // WebView2 user-data folder ALWAYS set to a writable temp dir: hosts deny the
    // default location → silent fallback → blank page.
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                                     .getChildFile ("OsimpleWavetable_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3️⃣ ATTACHMENTS — after the WebView, same order as the declarations.
    //    3-arg constructor (nullptr UndoManager) through attach(), which asserts
    //    on ID drift in debug and leaves the control unbound (never a null
    //    dereference) in release.
    auto& apvts = processorRef.getAPVTS();
    bankAttachment        = attach<juce::WebComboBoxParameterAttachment>      (apvts, "bank",          *bankRelay);
    positionAttachment    = attach<juce::WebSliderParameterAttachment>        (apvts, "position",      *positionRelay);
    interpAttachment      = attach<juce::WebToggleButtonParameterAttachment>  (apvts, "interp",        *interpRelay);
    bandlimitAttachment   = attach<juce::WebToggleButtonParameterAttachment>  (apvts, "bandlimit",     *bandlimitRelay);
    bitDepthAttachment    = attach<juce::WebComboBoxParameterAttachment>      (apvts, "bit_depth",     *bitDepthRelay);
    lfoRateAttachment     = attach<juce::WebSliderParameterAttachment>        (apvts, "lfo_rate",      *lfoRateRelay);
    lfoSyncAttachment     = attach<juce::WebComboBoxParameterAttachment>      (apvts, "lfo_sync",      *lfoSyncRelay);
    lfoDivAttachment      = attach<juce::WebComboBoxParameterAttachment>      (apvts, "lfo_div",       *lfoDivRelay);
    lfoShapeAttachment    = attach<juce::WebComboBoxParameterAttachment>      (apvts, "lfo_shape",     *lfoShapeRelay);
    lfoDepthAttachment    = attach<juce::WebSliderParameterAttachment>        (apvts, "lfo_depth",     *lfoDepthRelay);
    menvAttackAttachment  = attach<juce::WebSliderParameterAttachment>        (apvts, "menv_attack",   *menvAttackRelay);
    menvDecayAttachment   = attach<juce::WebSliderParameterAttachment>        (apvts, "menv_decay",    *menvDecayRelay);
    menvSustainAttachment = attach<juce::WebSliderParameterAttachment>        (apvts, "menv_sustain",  *menvSustainRelay);
    menvReleaseAttachment = attach<juce::WebSliderParameterAttachment>        (apvts, "menv_release",  *menvReleaseRelay);
    envAmountAttachment   = attach<juce::WebSliderParameterAttachment>        (apvts, "env_amount",    *envAmountRelay);
    ampAttackAttachment   = attach<juce::WebSliderParameterAttachment>        (apvts, "amp_attack",    *ampAttackRelay);
    ampDecayAttachment    = attach<juce::WebSliderParameterAttachment>        (apvts, "amp_decay",     *ampDecayRelay);
    ampSustainAttachment  = attach<juce::WebSliderParameterAttachment>        (apvts, "amp_sustain",   *ampSustainRelay);
    ampReleaseAttachment  = attach<juce::WebSliderParameterAttachment>        (apvts, "amp_release",   *ampReleaseRelay);
    voiceModeAttachment   = attach<juce::WebComboBoxParameterAttachment>      (apvts, "voice_mode",    *voiceModeRelay);
    outputLevelAttachment = attach<juce::WebSliderParameterAttachment>        (apvts, "output_level",  *outputLevelRelay);

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Snapshot the counters: the page's uiReady covers the current state; the
    // timer only signals changes that happen after this.
    lastBankGeneration = processorRef.getBankDisplayGeneration();
    lastImportVersion  = processorRef.getImportStatusVersion();

    // ui-design-rules.md Rule 4: fixed frame, <= 800 px tall. The headless UI
    // gates parse this literal, so it stays the numeric setSize.
    setSize (1120, 780);
    setResizable (false, false);

    startTimerHz (kTimerHz);
}

OSimpleWavetableAudioProcessorEditor::~OSimpleWavetableAudioProcessorEditor()
{
    stopTimer();
}

// ── C++ → page pushes ──────────────────────────────────────────────────────
// bankUpdate { bank, imported, numFrames, filename, frames: [[128 floats] x N] }
// Only on bank change / import / restore / uiReady — up to 256 x 128 floats.
void OSimpleWavetableAudioProcessorEditor::emitBankUpdate()
{
    if (webView == nullptr)
        return;

    OSimpleWavetableAudioProcessor::BankThumbs thumbs;
    processorRef.getBankThumbnails (thumbs);
    jassert ((int) thumbs.points.size() == thumbs.numFrames * kThumbPoints);

    juce::Array<juce::var> frames;
    frames.ensureStorageAllocated (thumbs.numFrames);
    for (int f = 0; f < thumbs.numFrames; ++f)
        frames.add (toVarArray (thumbs.points.data() + (size_t) f * kThumbPoints, (size_t) kThumbPoints));

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("bank",      thumbs.bank);
    obj->setProperty ("imported",  thumbs.imported);
    obj->setProperty ("numFrames", thumbs.numFrames);
    obj->setProperty ("filename",  thumbs.filename);
    obj->setProperty ("frames",    juce::var (std::move (frames)));

    webView->emitEventIfBrowserIsVisible ("bankUpdate", juce::var (obj));
}

// importStatus { state: "idle"|"busy"|"done"|"error", filename, frames, error }
void OSimpleWavetableAudioProcessorEditor::emitImportStatus()
{
    if (webView == nullptr)
        return;

    const auto st = processorRef.getImportStatus();

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("state",    juce::String (importStateName (st.state)));
    obj->setProperty ("filename", st.filename);
    obj->setProperty ("frames",   st.frames);
    obj->setProperty ("error",    st.error);

    webView->emitEventIfBrowserIsVisible ("importStatus", juce::var (obj));
}

// cycleUpdate { cycle[256], harmonics[32], pos, frame, level, sounding,
//               note, f0, kmax, nyquistH, harmonicsRaw[32], preQ[256]?, lfo, menv, amp }
// Hash-gated: emitted only when something the page draws has changed.
void OSimpleWavetableAudioProcessorEditor::emitCycleUpdate (bool force)
{
    if (webView == nullptr)
        return;

    OSimpleWavetableAudioProcessor::CycleView v;
    processorRef.buildCycleView (v);

    Fnv fnv;
    fnv.add (v.cycle.data(), sizeof (float) * v.cycle.size());
    fnv.add (v.harmonicsDb.data(), sizeof (float) * v.harmonicsDb.size());
    fnv.add (v.harmonicsRawDb.data(), sizeof (float) * v.harmonicsRawDb.size());
    fnv.add (v.pos);  fnv.add (v.frame);  fnv.add (v.level);  fnv.add (v.kmax);
    fnv.add (v.nyquistH);  fnv.add (v.note);  fnv.add (v.f0);
    fnv.add (v.lfo);  fnv.add (v.menv);  fnv.add (v.amp);
    fnv.add (v.sounding);  fnv.add (v.quantized);

    if (! force && fnv.h == lastCycleHash)
        return;
    lastCycleHash = fnv.h;

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("cycle",        toVarArray (v.cycle.data(), v.cycle.size()));
    obj->setProperty ("harmonics",    toVarArray (v.harmonicsDb.data(), v.harmonicsDb.size()));
    obj->setProperty ("harmonicsRaw", toVarArray (v.harmonicsRawDb.data(), v.harmonicsRawDb.size()));
    if (v.quantized)   // the unquantized ghost only matters when Bit Depth != Full
        obj->setProperty ("preQ", toVarArray (v.preQ.data(), v.preQ.size()));
    obj->setProperty ("pos",      v.pos);        // effective position 0..1 (lead voice, else knob)
    obj->setProperty ("frame",    v.frame);      // latched frame (Interpolation Off), else -1
    obj->setProperty ("level",    v.level);      // mip level actually read
    obj->setProperty ("kmax",     v.kmax);       // highest harmonic in that level
    obj->setProperty ("nyquistH", v.nyquistH);   // (fs / 2) / f0
    obj->setProperty ("note",     v.note);       // lead voice MIDI note, -1 when silent
    obj->setProperty ("f0",       v.f0);         // Hz, bend included
    obj->setProperty ("lfo",      v.lfo);        // -1..1, drives the LFO lamp
    obj->setProperty ("menv",     v.menv);       // 0..1 live mod-env level (lead voice)
    obj->setProperty ("amp",      v.amp);        // 0..1 live amp-env level (lead voice)
    obj->setProperty ("sounding", v.sounding);

    webView->emitEventIfBrowserIsVisible ("cycleUpdate", juce::var (obj));
}

// ── Timer (30 Hz, message thread) ───────────────────────────────────────────
void OSimpleWavetableAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    if (const auto g = processorRef.getBankDisplayGeneration(); g != lastBankGeneration)
    {
        lastBankGeneration = g;
        emitBankUpdate();
        forceCycleEmit = true;
    }

    if (const auto v = processorRef.getImportStatusVersion(); v != lastImportVersion)
    {
        lastImportVersion = v;
        emitImportStatus();
    }

    emitCycleUpdate (std::exchange (forceCycleEmit, false));
}

// ── Layout ──────────────────────────────────────────────────────────────────
void OSimpleWavetableAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills the editor — nothing to paint.
}

void OSimpleWavetableAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}

static_assert (kDesignW == 1120 && kDesignH == 780, "keep in step with v1-ui.yaml and the page's body size");
