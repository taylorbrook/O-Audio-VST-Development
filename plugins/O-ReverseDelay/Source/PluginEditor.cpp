/*
  ==============================================================================

    O-ReverseDelay — Plugin Editor (implementation)

    20 WebSliderRelay knobs (delayTime, grainSize, density, feedback, lowCut,
    highCut, width, mix + v1.1.0's jitter, delayScatter, sizeRandom, gainRandom
    + v1.2.0's grainTilt + v1.3.0's grainCount + v1.4.0's tukeyTaper + v1.6.0's
    direction, regenMakeup + v1.7.0's duck, driftRate, driftDepth) + 4
    WebComboBoxRelay controls (syncMode, noteDivision, grainShape + v1.7.0's
    sourceMode) + 1 WebToggleButtonRelay (v1.6.0's freeze) bound two-way to the
    APVTS. The UI-02 Sync/Free control swap is pure JS — both controls stay
    relay-bound at all times, so neither is ever dead.

    ── v1.6.0: the first toggle relay ────────────────────────────────────────
    `freeze` is the plugin's only AudioParameterBool, and a bool needs its own
    relay TYPE — the slider and combo relays will attach to it without
    complaint and produce a switch whose state never changes. Three lists now
    have to stay in step with createParameterLayout(): kSliderIds, kComboIds and
    kToggleIds. tests/ui_frontend_check.js diffs all three against the APVTS in
    both directions, which is the only thing that actually catches an id landing
    in the wrong one.

    ── v1.3.0: this editor now polls the processor ───────────────────────────
    Stage 3's decision D10 was "no visualization, no Timer, no C++->JS polling
    bridge", and the COUNT panel's live grain readout (B2) reverses it. Recorded
    plainly rather than quietly: the readout is the whole point of B2's third
    item — GrainPool::countActive() has existed since Stage 2 and was called by
    nothing, which made density an abstract percentage — and a value that
    changes every block cannot be delivered by the parameter relays, which only
    fire on parameter change.

    It is a PULL, not a push: JS asks via a native function on an interval
    (see app.js METER_POLL_MS), so there is no juce::Timer in this class and no
    emitEvent listener plumbing. That choice is what keeps the ui-stub able to
    render the page — the stub already models getNativeFunction, and modelling
    window.__JUCE__.backend.addEventListener would have been new test surface
    for no gain.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "UIBinaryData.h"   // distinct namespace — see CMakeLists juce_add_binary_data

namespace
{
    // Parameter IDs — must match createParameterLayout() exactly. An ID typo is
    // a silently dead control, so every lookup below is jassert'ed.
    const juce::StringArray kSliderIds {
        "delayTime",
        "grainSize", "density",
        "feedback", "lowCut", "highCut",
        "width", "mix",
        // v1.1.0 (B3): RANDOM panel. Order matches app.js's KNOB_IDS — not
        // required by the relay machinery, but a reviewer diffing the two
        // lists for a missing ID should not also have to reorder them.
        "jitter", "delayScatter", "sizeRandom", "gainRandom",
        // v1.2.0 (B1): WINDOW panel. grainTilt is the only NEW SLIDER — its
        // companion grainShape is a choice and belongs in kComboIds below, not
        // here. Putting a choice param in the slider list yields a relay that
        // attaches but a control whose index never updates.
        "grainTilt",
        // v1.3.0 (B2): COUNT panel. A float with step 1 rather than a choice,
        // so it belongs here — a 15-entry AudioParameterChoice would have read
        // as a dropdown of numbers and lost the drag gesture.
        "grainCount",
        // v1.4.0: WINDOW panel, beside Tilt. Inert unless grainShape is Tukey —
        // still relay-bound at all times (never conditionally attached), exactly
        // as the Sync/Free pair is, so it can never become a dead control. The UI
        // dims it; the binding stays live.
        "tukeyTaper",
        // v1.6.0 (B4 #2, #3): MOTION panel. `freeze` is the third control in
        // that panel and is NOT here — it is a bool and belongs in kToggleIds
        // below. That split is the same trap grainShape presented at v1.2.0 in a
        // different type: a relay whose type does not match the parameter's
        // attaches cleanly and then never updates.
        "direction", "regenMakeup",
        // v1.7.0 (B4 #4, #6): the DUCK and DRIFT panels in row 3. `sourceMode`
        // is the fourth new parameter and is NOT here — it is a CHOICE and
        // belongs in kComboIds below, the same split grainShape presented at
        // v1.2.0 and `freeze` presented again at v1.6.0 in a third type. A relay
        // whose type does not match the parameter attaches cleanly and then
        // never updates.
        "duck", "driftRate", "driftDepth"
    };

    // v1.7.0: sourceMode joins the three originals. It is rendered as a SEGMENT
    // PAIR rather than a select — like syncMode, and unlike noteDivision and
    // grainShape — because it names two modes rather than picking from a list;
    // the relay type is the same either way.
    const juce::StringArray kComboIds { "syncMode", "noteDivision", "grainShape", "sourceMode" };

    // v1.6.0: the plugin's first bool parameter, and therefore its first toggle
    // relay. Kept as a StringArray rather than a bare id so the frontend check's
    // layout-vs-editor diff can scan this block the same way it scans the other
    // two — an AudioParameterBool that appeared in the APVTS and in NO relay list
    // would otherwise be a silently dead switch.
    const juce::StringArray kToggleIds { "freeze" };

    // v1.4.0: how many points the window-shape graph is sampled at. 128 is well
    // above the ~160 CSS px the canvas is drawn into, so the curve is smooth at
    // 1x and 2x DPR alike without shipping a payload per pixel. The value is
    // mirrored in app.js only as an expectation to validate, never to size an
    // array — the C++ decides the length and JS reads what it is sent.
    constexpr int kWindowCurvePoints = 128;

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

// ── Resource provider ───────────────────────────────────────────────────────
// The WKWebView / WebView2 resource callback receives a BARE PATH ("/",
// "/js/app.js", ...) — there is no scheme or host to strip, and stripping one
// would collapse every lookup to an empty string. Match by direct equality and
// never hard-code juce:// vs https://juce.backend
// (critical_webview_resource_provider_and_schemes).
std::optional<juce::WebBrowserComponent::Resource>
ReverseDelayEditor::getResource (const juce::String& url)
{
    // charset=utf-8 on text resources — the page uses UTF-8 entities (fleurons,
    // en-dashes, hair spaces) that mojibake without it on some hosts.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (UIBinaryData::index_html, UIBinaryData::index_htmlSize,
                                   "text/html; charset=utf-8");

    if (url == "/css/styles.css")
        return makeBinaryResource (UIBinaryData::styles_css, UIBinaryData::styles_cssSize,
                                   "text/css; charset=utf-8");

    if (url == "/js/app.js")
        return makeBinaryResource (UIBinaryData::app_js, UIBinaryData::app_jsSize,
                                   "application/javascript; charset=utf-8");

    // Shared preset-manager module. Embedded from modules/… but SERVED under
    // /js/ so app.js can reach it with a plain relative `import("./preset-manager.js")`.
    // NOTE the symbol: juce_add_binary_data STRIPS the hyphen rather than
    // converting it to an underscore, so "preset-manager.js" becomes
    // `presetmanager_js` — not `preset_manager_js`.
    if (url == "/js/preset-manager.js")
        return makeBinaryResource (UIBinaryData::presetmanager_js,
                                   UIBinaryData::presetmanager_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (UIBinaryData::index_js, UIBinaryData::index_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (UIBinaryData::check_native_interop_js,
                                   UIBinaryData::check_native_interop_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/img/birds.png")
        return makeBinaryResource (UIBinaryData::birds_png, UIBinaryData::birds_pngSize,
                                   "image/png");

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
ReverseDelayEditor::ReverseDelayEditor (ReverseDelayProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    // 1. RELAYS (must exist before the WebView) ------------------------------
    for (const auto& id : kSliderIds)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));

    for (const auto& id : kComboIds)
        comboRelays.push_back (std::make_unique<juce::WebComboBoxRelay> (id));

    for (const auto& id : kToggleIds)
        toggleRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> (id));

    // 2. WEBVIEW options + relay registration --------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);

    for (const auto& relay : comboRelays)
        options = options.withOptionsFrom (*relay);

    // Registers "freeze" in initialisationData.__juce__toggles, which is the
    // list Juce.getToggleState() checks before it will build a ToggleState.
    // Miss this and the page throws on load rather than merely losing a control.
    for (const auto& relay : toggleRelays)
        options = options.withOptionsFrom (*relay);

    // ── NATIVE FUNCTIONS — exactly 13 ──────────────────────────────────────
    // 1 for dblclick-reset + 1 for the v1.3.0 grain meter + 1 for v1.4.0's
    // window-shape curve + the 10 that js/preset-manager.js fetches. The count is grep-diffed against app.js +
    // preset-manager.js at the Stage-4 gate: an unregistered fn leaves its
    // control silently dead while build, auval and pluginval all pass
    // (pattern_webview_native_fn_bridge_gap).

    // Dblclick-reset needs each parameter's default in ENGINEERING units: the
    // properties payload pushed to the page carries start/end/skew but no
    // default, and a hardcoded JS default table would drift from the C++
    // NormalisableRange (pattern_webview_knob_readout_scaled_value). The page
    // converts back through the same live properties, so the round-trip is exact
    // for the four skewed params.
    options = options.withNativeFunction ("getParameterDefaults",
        [this] (auto&, auto complete)
        {
            auto* obj = new juce::DynamicObject();

            for (const auto& id : kSliderIds)
            {
                if (auto* param = processorRef.parameters.getParameter (id))
                    obj->setProperty (id, param->convertFrom0to1 (param->getDefaultValue()));
            }

            complete (juce::var (obj));
        });

    // ── v1.3.0 (B2): the grain meter ────────────────────────────────────────
    // Reports what the engine is DOING, not what the parameters say — which is
    // the point. Density was an abstract percentage through v1.2.0 because the
    // overlap it produces depends on grain size too, and now on the ceiling as
    // well; a live count is the only honest readout of "how many grains".
    //
    // Reads two relaxed atomics the audio thread publishes once per block, never
    // the pool or scheduler directly. processorRef.getActiveGrainCount() would
    // have been the obvious call and is the wrong one here: it walks 32 bools
    // that the audio thread is writing, which is a data race from this thread.
    //
    // Returns a DynamicObject rather than a JSON string so the page needs no
    // parse step and a malformed payload cannot silently degrade to a dead
    // readout — same shape as getParameterDefaults above.
    options = options.withNativeFunction ("getGrainMeter",
        [this] (auto&, auto complete)
        {
            const auto meter = processorRef.getGrainMeter();

            auto* obj = new juce::DynamicObject();
            obj->setProperty ("active",  meter.active);
            obj->setProperty ("overlap", meter.overlap);

            complete (juce::var (obj));
        });

    // ── v1.4.0: the window-shape curve ──────────────────────────────────────
    // Returns the envelope the engine will actually apply — shape, tilt and Tukey
    // taper composed — sampled at kWindowCurvePoints.
    //
    // Computed by WindowLut::sampleWindow rather than replicated in JavaScript,
    // and that is the whole design of this feature. A JS reimplementation of the
    // window would be a SECOND definition free to drift from the first, and the
    // drift would be invisible: the graph would keep looking like a plausible
    // window while no longer describing the audio. That is the same failure mode
    // as a JS knob-range table drifting from the C++ NormalisableRange
    // (pattern_webview_knob_readout_scaled_value), except a graph has no units to
    // give it away.
    //
    // PULLED on change, not polled: the page refetches when grainShape, grainTilt
    // or tukeyTaper moves, which is a handful of calls per gesture rather than 15
    // a second for a curve that is static between edits.
    //
    // Reads the parameters here rather than taking them as arguments so the curve
    // can never disagree with the knobs — one source for both.
    options = options.withNativeFunction ("getWindowCurve",
        [this] (auto&, auto complete)
        {
            const auto& luts = processorRef.getWindowLuts();

            auto* shapeParam = processorRef.parameters.getParameter ("grainShape");
            auto* tiltParam  = processorRef.parameters.getParameter ("grainTilt");
            auto* taperParam = processorRef.parameters.getParameter ("tukeyTaper");

            const int   shape = shapeParam != nullptr
                                  ? juce::roundToInt (shapeParam->convertFrom0to1 (shapeParam->getValue()))
                                  : 0;
            const float tilt  = tiltParam  != nullptr
                                  ? tiltParam->convertFrom0to1 (tiltParam->getValue())
                                  : 0.5f;
            const float alpha = taperParam != nullptr
                                  ? taperParam->convertFrom0to1 (taperParam->getValue())
                                  : WindowLut::kTukeyTaperDefault;

            std::array<float, (size_t) kWindowCurvePoints> curve {};
            luts.sampleWindow (shape,
                               ReverseDelayProcessor::tiltToPeakPos (tilt),
                               alpha,
                               curve.data(), kWindowCurvePoints);

            juce::Array<juce::var> points;
            points.ensureStorageAllocated (kWindowCurvePoints);

            for (auto v : curve)
                points.add (juce::var (v));

            complete (juce::var (points));
        });

    // ── Preset bridge (OuariconPresetManager v1.0.5 contract) ──────────────
    options = options
        .withNativeFunction ("savePreset", [this] (const auto& args, auto complete)
        {
            if (args.size() < 1 || ! args[0].isString()) { complete (juce::var (false)); return; }
            complete (juce::var (processorRef.getPresetManager().savePreset (args[0].toString())));
        })

        .withNativeFunction ("loadPreset", [this] (const auto& args, auto complete)
        {
            if (args.size() < 1 || ! args[0].isString()) { complete (juce::var (false)); return; }
            complete (juce::var (processorRef.getPresetManager().loadPreset (args[0].toString())));
        })

        .withNativeFunction ("getPresetList", [this] (const auto&, auto complete)
        {
            juce::Array<juce::var> arr;
            for (const auto& name : processorRef.getPresetManager().getPresetList())
                arr.add (name);
            complete (juce::var (arr));
        })

        .withNativeFunction ("getCurrentPreset", [this] (const auto&, auto complete)
        {
            complete (juce::var (processorRef.getPresetManager().getCurrentPresetName()));
        })

        // Return the neighbour NAME only — preset-manager.js calls loadPreset()
        // on the result itself, so loading here as well would double-load.
        .withNativeFunction ("selectNextPreset", [this] (const auto&, auto complete)
        {
            complete (juce::var (processorRef.getPresetManager().getNextPreset()));
        })

        .withNativeFunction ("selectPreviousPreset", [this] (const auto&, auto complete)
        {
            complete (juce::var (processorRef.getPresetManager().getPreviousPreset()));
        })

        .withNativeFunction ("deletePreset", [this] (const auto& args, auto complete)
        {
            if (args.size() < 1 || ! args[0].isString()) { complete (juce::var (false)); return; }
            complete (juce::var (processorRef.getPresetManager().deletePreset (args[0].toString())));
        })

        .withNativeFunction ("isFactoryPreset", [this] (const auto& args, auto complete)
        {
            if (args.size() < 1 || ! args[0].isString()) { complete (juce::var (false)); return; }
            complete (juce::var (processorRef.getPresetManager().isFactoryPreset (args[0].toString())));
        })

        // Both dialog fns MUST resolve {success, name} — preset-manager.js checks
        // `result && result.success`, so a bare bool silently no-ops the bar.
        .withNativeFunction ("savePresetWithDialog", [this] (const auto&, auto complete)
        {
            // MSVC: the SafePointer is hoisted to a LOCAL here and captured by
            // copy below. Writing `[safeThis = SafePointer<...>(this), ...]`
            // directly in the nested launchAsync lambda (as O-Contrabass does)
            // resolves `this` to the enclosing closure on MSVC — a hard compile
            // error that Apple Clang accepts silently and that only surfaces on
            // the first Windows CI build
            // (critical_msvc_safepointer_init_capture_nested_lambda).
            auto safeThis = juce::Component::SafePointer<ReverseDelayEditor> (this);

            auto makeResult = [] (bool ok, const juce::String& name)
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty ("success", ok);
                obj->setProperty ("name", name);
                return juce::var (obj);
            };

            if (fileDialogOpen) { complete (makeResult (false, {})); return; }
            fileDialogOpen = true;

            fileChooser = std::make_shared<juce::FileChooser> (
                "Save Preset",
                processorRef.getPresetManager().getUserPresetsDirectory(),
                "*.json");

            fileChooser->launchAsync (
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete, makeResult] (const juce::FileChooser& fc)
                {
                    // Editor destroyed while the dialog was up: BARE return.
                    // `complete` is owned by the now-dead WebView Impl, so even
                    // calling complete(false) here is itself a use-after-free
                    // (pattern_webview_launchasync_safepointer_no_complete).
                    if (safeThis == nullptr)
                        return;

                    safeThis->fileDialogOpen = false;

                    auto result = fc.getResult();
                    if (result == juce::File{}) { complete (makeResult (false, {})); return; }

                    auto  name = result.getFileNameWithoutExtension();
                    auto& pm   = safeThis->processorRef.getPresetManager();

                    // Honour the directory the user picked — savePreset() always
                    // writes to the user-presets dir regardless (O-Wind WR-12).
                    const bool ok = result.isAChildOf (pm.getUserPresetsDirectory())
                                      ? pm.savePreset (name)
                                      : pm.savePresetToFile (result.withFileExtension ("json"));

                    complete (makeResult (ok, name));
                });
        })

        .withNativeFunction ("loadPresetFromFile", [this] (const auto&, auto complete)
        {
            auto safeThis = juce::Component::SafePointer<ReverseDelayEditor> (this);   // hoisted — see above

            auto makeResult = [] (bool ok, const juce::String& name)
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty ("success", ok);
                obj->setProperty ("name", name);
                return juce::var (obj);
            };

            if (fileDialogOpen) { complete (makeResult (false, {})); return; }
            fileDialogOpen = true;

            fileChooser = std::make_shared<juce::FileChooser> (
                "Load Preset",
                processorRef.getPresetManager().getUserPresetsDirectory(),
                "*.json");

            fileChooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete, makeResult] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;   // bare return — see savePresetWithDialog

                    safeThis->fileDialogOpen = false;

                    auto file = fc.getResult();
                    if (! file.existsAsFile()) { complete (makeResult (false, {})); return; }

                    const bool ok = safeThis->processorRef.getPresetManager().loadPresetFromFile (file);
                    complete (makeResult (ok, file.getFileNameWithoutExtension()));
                });
        });

   #if JUCE_WINDOWS
    // WebView2's default user-data folder is denied in most DAW hosts; a failed
    // construction falls back to the IE backend with no resource provider, which
    // presents as a blank page and no error
    // (critical_webview2_runtime_gotchas_windows).
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OReverseDelay_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS (after the WebView; 3-arg ctor, nullptr undoManager) -----
    for (int i = 0; i < kSliderIds.size(); ++i)
    {
        auto* param = processorRef.parameters.getParameter (kSliderIds[i]);
        jassert (param != nullptr);   // ID drift → silently dead control
        if (param != nullptr)
            sliderAttachments.push_back (
                std::make_unique<juce::WebSliderParameterAttachment> (
                    *param, *sliderRelays[(size_t) i], nullptr));
    }

    for (int i = 0; i < kComboIds.size(); ++i)
    {
        auto* param = processorRef.parameters.getParameter (kComboIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            comboAttachments.push_back (
                std::make_unique<juce::WebComboBoxParameterAttachment> (
                    *param, *comboRelays[(size_t) i], nullptr));
    }

    for (int i = 0; i < kToggleIds.size(); ++i)
    {
        auto* param = processorRef.parameters.getParameter (kToggleIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            toggleAttachments.push_back (
                std::make_unique<juce::WebToggleButtonParameterAttachment> (
                    *param, *toggleRelays[(size_t) i], nullptr));
    }

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Fixed 940 x 768.
    //
    // ── v1.7.1: 972 -> 768, all of it empty space ────────────────────────────
    // The v1.7.0 frame did not fit a 1080p screen once a DAW's plugin header and
    // the menu bar were above it. The 204 px removed here held no control: rows 1
    // and 3 were 215 px panels around 100 px and 94 px of content, and .groups —
    // which is flex:1, so it takes the frame's content box minus header, preset
    // band and footer — was centring 703 px of rows inside 796.5 px.
    //
    // That last 93.5 px survived five releases because every comment reasoned
    // about the ROW sum and called it "exactly zero slack" without ever
    // subtracting it from the frame height. It is not the kind of error a build,
    // auval or a static check can see; it was found by rendering the page at the
    // shipping viewport and measuring, which is now what styles.css documents.
    //
    // Row 2 stays 245. Its WINDOW panel lays 212 px of content into a 213 px
    // content box, so trimming row 2 means shrinking the v1.4.0 envelope display
    // — a feature change, not a layout one.
    //
    // v1.7.3 (IN-03): this read "214 px into a 213 px body" through v1.7.2, and
    // so did styles.css in two places and NOTES.md in one. All four were copies
    // of a figure derived from the CSS rules rather than measured; the rendered
    // knob-cell is 78 px, not 80, so the panel is 1 px UNDER, not 1 px over.
    // The itemised budget now lives in exactly one comment (the .group-window
    // block in styles.css) and is asserted against the real layout in
    // tests/ui_tooltip_clamp_check.js. Do not restate it here again.
    //
    // The WIDTH is deliberately unchanged at 940. The tooltip edge-clamp gate is
    // horizontal and viewport-sensitive — it only fires at the real shipping
    // width, so a width change would invalidate the verification outright
    // (pattern_tooltip_clamp_gate_viewport_sensitive). A height change moves only
    // the above/below flip, which is re-measured at 940 x 768 rather than assumed
    // — tests/ui_tooltip_clamp_check.js drives the real page at exactly this size.
    //
    // Must stay in sync with styles.css (html/body and .frame both read 768px).
    setSize (940, 768);
}

ReverseDelayEditor::~ReverseDelayEditor() = default;

// ── Layout ──────────────────────────────────────────────────────────────────
void ReverseDelayEditor::paint (juce::Graphics&)
{
    // The WebView fills the editor — nothing to paint.
}

void ReverseDelayEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
