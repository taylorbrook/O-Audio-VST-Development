/*
  ==============================================================================

    O-Contrabass — WebView Editor (BOILERPLATE from finalized mockup v1)

    Stage 3 Phase 3.x: this replaces Source/PluginEditor.cpp. Adapt during
    Stage 3 execute — initialization order MUST match the header's declaration
    order (relays → webView → attachments).

    Native functions registered here (grep-diff gate vs JS getNativeFunction):
      - getParameterDefaults   (WR-11: skew-correct normalized defaults for
                                dblclick reset — JS must NEVER hardcode ranges)
      - openTuningFilePicker   (async FileChooser, SafePointer pattern)

    C++ → JS event feeds (emitEventIfBrowserIsVisible, timer-driven):
      - vuLevel  {"db": <rmsDb>}  @ ~16-30 Hz
        TODO(Stage 3 research): wire to actual post-limiter output RMS —
        processor publishes an atomic<float> from processBlock.
      - (optional, Stage 3 research) bowState / bodyModes feeds if the
        Schelleng dot / spectrum should reflect post-modulation DSP truth
        instead of raw parameter values.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OContrabassAudioProcessorEditor::OContrabassAudioProcessorEditor(OContrabassAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ------------------------------------------------------------------
    // 1️⃣ Create relays FIRST (IDs must match APVTS param IDs EXACTLY —
    //    case-sensitive, parameter-spec.md authoritative)
    // ------------------------------------------------------------------
    // I · Bow
    bowSpeedRelay        = std::make_unique<juce::WebSliderRelay>("BOW_SPEED");
    bowPressureRelay     = std::make_unique<juce::WebSliderRelay>("BOW_PRESSURE");
    bowPositionRelay     = std::make_unique<juce::WebSliderRelay>("BOW_POSITION");
    rosinRelay           = std::make_unique<juce::WebSliderRelay>("ROSIN");
    bowNoiseRelay        = std::make_unique<juce::WebSliderRelay>("BOW_NOISE");
    // II · Body
    bodySizeRelay        = std::make_unique<juce::WebSliderRelay>("BODY_SIZE");
    bodyDampingRelay     = std::make_unique<juce::WebSliderRelay>("BODY_DAMPING");
    bodyMixRelay         = std::make_unique<juce::WebSliderRelay>("BODY_MIX");
    brightnessRelay      = std::make_unique<juce::WebSliderRelay>("BRIGHTNESS");
    // III · Strings
    stringTensionRelay   = std::make_unique<juce::WebSliderRelay>("STRING_TENSION");
    stringStiffnessRelay = std::make_unique<juce::WebSliderRelay>("STRING_STIFFNESS");
    activeStringsRelay   = std::make_unique<juce::WebSliderRelay>("ACTIVE_STRINGS");
    detuneERelay         = std::make_unique<juce::WebSliderRelay>("DETUNE_E");
    detuneARelay         = std::make_unique<juce::WebSliderRelay>("DETUNE_A");
    detuneDRelay         = std::make_unique<juce::WebSliderRelay>("DETUNE_D");
    detuneGRelay         = std::make_unique<juce::WebSliderRelay>("DETUNE_G");
    // IV · Expression
    expressionMacroRelay = std::make_unique<juce::WebSliderRelay>("EXPRESSION_MACRO");
    vibratoRateRelay     = std::make_unique<juce::WebSliderRelay>("VIBRATO_RATE");
    vibratoDepthRelay    = std::make_unique<juce::WebSliderRelay>("VIBRATO_DEPTH");
    vibratoOnsetRelay    = std::make_unique<juce::WebSliderRelay>("VIBRATO_ONSET");
    slowLfoRateRelay     = std::make_unique<juce::WebSliderRelay>("SLOW_LFO_RATE");
    slowLfoDepthRelay    = std::make_unique<juce::WebSliderRelay>("SLOW_LFO_DEPTH");
    // V · Drone
    infiniteSustainRelay = std::make_unique<juce::WebSliderRelay>("INFINITE_SUSTAIN");
    subHarmonicsRelay    = std::make_unique<juce::WebSliderRelay>("SUB_HARMONICS");
    // VI · Output
    outputGainRelay      = std::make_unique<juce::WebSliderRelay>("OUTPUT_GAIN");
    widthRelay           = std::make_unique<juce::WebSliderRelay>("WIDTH");
    masterSatRelay       = std::make_unique<juce::WebSliderRelay>("MASTER_SAT_AMOUNT");
    limiterCeilingRelay  = std::make_unique<juce::WebSliderRelay>("LIMITER_CEILING_DB");
    // VII · Microtonal
    referencePitchRelay  = std::make_unique<juce::WebSliderRelay>("REFERENCE_PITCH");
    tuningSystemRelay    = std::make_unique<juce::WebComboBoxRelay>("TUNING_SYSTEM");
    noteExpressionRelay  = std::make_unique<juce::WebToggleButtonRelay>("NOTE_EXPRESSION");

    // ------------------------------------------------------------------
    // 2️⃣ Create WebView SECOND (with relay options)
    // ------------------------------------------------------------------
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            // Windows: WebView2 may be denied its default user-data location
            // inside DAW hosts — always point it at a temp dir. Requires
            // NEEDS_WEBVIEW2 TRUE + JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
            // (both already present in O-Contrabass CMakeLists.txt).
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile("OContrabass_WebView")))
            .withNativeIntegrationEnabled()
            // WR-11 pattern (O-GrainScatter): expose each parameter's TRUE
            // (skew-correct) normalized default so the UI's double-click reset
            // uses the C++ NormalisableRange, never hand-coded JS constants.
            .withNativeFunction(
                juce::Identifier("getParameterDefaults"),
                [this](const juce::Array<juce::var>& /*args*/,
                       juce::WebBrowserComponent::NativeFunctionCompletion completion)
                {
                    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
                    for (auto* param : processorRef.getParameters())
                        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param))
                            obj->setProperty(ranged->getParameterID(), ranged->getDefaultValue());
                    completion(juce::var(obj.get()));
                })
            // Scala/TUN file picker — async FileChooser handled in a member fn
            // so the SafePointer/UAF contract lives in one place.
            .withNativeFunction(
                juce::Identifier("openTuningFilePicker"),
                [this](const juce::Array<juce::var>& /*args*/,
                       juce::WebBrowserComponent::NativeFunctionCompletion completion)
                {
                    launchTuningFilePicker(std::move(completion));
                })
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
            .withOptionsFrom(*bowSpeedRelay)
            .withOptionsFrom(*bowPressureRelay)
            .withOptionsFrom(*bowPositionRelay)
            .withOptionsFrom(*rosinRelay)
            .withOptionsFrom(*bowNoiseRelay)
            .withOptionsFrom(*bodySizeRelay)
            .withOptionsFrom(*bodyDampingRelay)
            .withOptionsFrom(*bodyMixRelay)
            .withOptionsFrom(*brightnessRelay)
            .withOptionsFrom(*stringTensionRelay)
            .withOptionsFrom(*stringStiffnessRelay)
            .withOptionsFrom(*activeStringsRelay)
            .withOptionsFrom(*detuneERelay)
            .withOptionsFrom(*detuneARelay)
            .withOptionsFrom(*detuneDRelay)
            .withOptionsFrom(*detuneGRelay)
            .withOptionsFrom(*expressionMacroRelay)
            .withOptionsFrom(*vibratoRateRelay)
            .withOptionsFrom(*vibratoDepthRelay)
            .withOptionsFrom(*vibratoOnsetRelay)
            .withOptionsFrom(*slowLfoRateRelay)
            .withOptionsFrom(*slowLfoDepthRelay)
            .withOptionsFrom(*infiniteSustainRelay)
            .withOptionsFrom(*subHarmonicsRelay)
            .withOptionsFrom(*outputGainRelay)
            .withOptionsFrom(*widthRelay)
            .withOptionsFrom(*masterSatRelay)
            .withOptionsFrom(*limiterCeilingRelay)
            .withOptionsFrom(*referencePitchRelay)
            .withOptionsFrom(*tuningSystemRelay)
            .withOptionsFrom(*noteExpressionRelay)
    );

    addAndMakeVisible(*webView);

    // ------------------------------------------------------------------
    // 3️⃣ Create attachments LAST (relay + APVTS param, IDs exact)
    // ------------------------------------------------------------------
    auto& apvts = processorRef.parameters;

    bowSpeedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BOW_SPEED"), *bowSpeedRelay, nullptr);
    bowPressureAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BOW_PRESSURE"), *bowPressureRelay, nullptr);
    bowPositionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BOW_POSITION"), *bowPositionRelay, nullptr);
    rosinAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("ROSIN"), *rosinRelay, nullptr);
    bowNoiseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BOW_NOISE"), *bowNoiseRelay, nullptr);

    bodySizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BODY_SIZE"), *bodySizeRelay, nullptr);
    bodyDampingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BODY_DAMPING"), *bodyDampingRelay, nullptr);
    bodyMixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BODY_MIX"), *bodyMixRelay, nullptr);
    brightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BRIGHTNESS"), *brightnessRelay, nullptr);

    stringTensionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("STRING_TENSION"), *stringTensionRelay, nullptr);
    stringStiffnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("STRING_STIFFNESS"), *stringStiffnessRelay, nullptr);
    activeStringsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("ACTIVE_STRINGS"), *activeStringsRelay, nullptr);
    detuneEAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("DETUNE_E"), *detuneERelay, nullptr);
    detuneAAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("DETUNE_A"), *detuneARelay, nullptr);
    detuneDAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("DETUNE_D"), *detuneDRelay, nullptr);
    detuneGAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("DETUNE_G"), *detuneGRelay, nullptr);

    expressionMacroAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("EXPRESSION_MACRO"), *expressionMacroRelay, nullptr);
    vibratoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("VIBRATO_RATE"), *vibratoRateRelay, nullptr);
    vibratoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("VIBRATO_DEPTH"), *vibratoDepthRelay, nullptr);
    vibratoOnsetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("VIBRATO_ONSET"), *vibratoOnsetRelay, nullptr);
    slowLfoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("SLOW_LFO_RATE"), *slowLfoRateRelay, nullptr);
    slowLfoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("SLOW_LFO_DEPTH"), *slowLfoDepthRelay, nullptr);

    infiniteSustainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("INFINITE_SUSTAIN"), *infiniteSustainRelay, nullptr);
    subHarmonicsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("SUB_HARMONICS"), *subHarmonicsRelay, nullptr);

    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("OUTPUT_GAIN"), *outputGainRelay, nullptr);
    widthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("WIDTH"), *widthRelay, nullptr);
    masterSatAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("MASTER_SAT_AMOUNT"), *masterSatRelay, nullptr);
    limiterCeilingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("LIMITER_CEILING_DB"), *limiterCeilingRelay, nullptr);

    referencePitchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("REFERENCE_PITCH"), *referencePitchRelay, nullptr);
    tuningSystemAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("TUNING_SYSTEM"), *tuningSystemRelay, nullptr);
    noteExpressionAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("NOTE_EXPRESSION"), *noteExpressionRelay, nullptr);

    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Viz feed timer — 30 Hz is the budget ceiling; VU spec says 16 Hz is
    // sufficient. If C++ ends up feeding ONLY the VU, drop to startTimerHz(16).
    startTimerHz(30);

    // Fixed 1000x650 per finalized mockup v1 — non-resizable.
    setResizable(false, false);
    setSize(1000, 650);
}

OContrabassAudioProcessorEditor::~OContrabassAudioProcessorEditor()
{
    stopTimer();
}

void OContrabassAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xffEDD9BE));   // bg-paper — avoids white flash before WebView paints
}

void OContrabassAudioProcessorEditor::resized()
{
    webView->setBounds(getLocalBounds());
}

void OContrabassAudioProcessorEditor::timerCallback()
{
    // IN-15 pattern (O-GrainScatter): the emit below is visibility-gated, but
    // building per-tick JSON is pure churn while the UI is hidden. Skip early.
    if (! webView->isShowing())
        return;

    // ------------------------------------------------------------------
    // vuLevel feed — VU meter needle (JS listens on window.__JUCE__.backend).
    // TODO(Stage 3 research): replace placeholder with real post-limiter RMS.
    // Plan: processor publishes std::atomic<float> outputRmsDb from
    // processBlock (RT-safe store); editor reads it here:
    //     const float rmsDb = processorRef.getOutputRmsDb();
    // ------------------------------------------------------------------
    const float rmsDb = -20.0f;   // TODO placeholder — silent-floor reading
    webView->emitEventIfBrowserIsVisible("vuLevel",
        juce::var("{\"db\":" + juce::String(rmsDb, 2) + "}"));

    // OPTIONAL (Stage 3 research): "bowState" feed for the Schelleng dot so it
    // reflects post-macro/LFO modulated bow speed/pressure instead of raw
    // parameter values. Same emit pattern, ≤30 Hz:
    //     webView->emitEventIfBrowserIsVisible("bowState", juce::var(json));
}

/*  RESOURCE PROVIDER — receives BARE PATHS, not full URLs (MEMORY-critical).
    Compare with direct equality; never strip scheme/host. Missing entries
    here = silent 404 → blank UI / "Frame load interrupted".                  */
std::optional<juce::WebBrowserComponent::Resource>
OContrabassAudioProcessorEditor::getResource(const juce::String& url)
{
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

    // MIME must be application/javascript or the module import silently fails.
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

    juce::Logger::writeToLog("O-Contrabass resource not found: " + url);
    return std::nullopt;
}

/*  ASYNC FILE PICKER — SafePointer + bare-return-on-null contract
    (MEMORY: pattern_webview_launchasync_safepointer_no_complete).
    The NativeFunctionCompletion is OWNED by the WebView Impl. If the editor
    is torn down while the chooser is open, that Impl is dead — calling
    completion (even completion(false)) is a use-after-free. Bail with a
    bare `return` on the null-SafePointer path. Cancel-with-live-editor is
    safe to complete normally.                                                */
void OContrabassAudioProcessorEditor::launchTuningFilePicker(
    juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    tuningFileChooser = std::make_unique<juce::FileChooser>(
        "Load Scala / TUN tuning file",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.scl;*.tun");

    auto safeThis = juce::Component::SafePointer<OContrabassAudioProcessorEditor>(this);

    tuningFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis, completion = std::move(completion)](const juce::FileChooser& fc) mutable
        {
            if (safeThis == nullptr)
                return;   // editor gone — completion is dangling; DO NOT call it

            const auto file = fc.getResult();
            if (file == juce::File{})
            {
                completion(juce::var(false));   // user cancelled — editor alive, safe
                return;
            }

            // TuningEngine handles .scl via processor entry point (Phase 2.6b).
            // TODO(Stage 3 research): .tun route — extend loadScalaFile or add
            // a loadTunFile sibling in the processor / scala-tuning-engine.
            const bool ok = safeThis->processorRef.loadScalaFile(file);
            completion(juce::var(ok));
        });
}
