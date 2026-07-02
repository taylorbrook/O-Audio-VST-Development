#include "PluginEditor.h"
#include "BinaryData.h"

DrumRouletteAudioProcessorEditor::DrumRouletteAudioProcessorEditor(DrumRouletteAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ========================================================================
    // STEP 1: Create relays FIRST (Pattern #11)
    // ========================================================================

    // Slot 1 relays (8 parameters)
    relay_decay1 = std::make_unique<juce::WebSliderRelay>("DECAY_1");
    relay_attack1 = std::make_unique<juce::WebSliderRelay>("ATTACK_1");
    relay_tiltFilter1 = std::make_unique<juce::WebSliderRelay>("TILT_FILTER_1");
    relay_pitch1 = std::make_unique<juce::WebSliderRelay>("PITCH_1");
    relay_volume1 = std::make_unique<juce::WebSliderRelay>("VOLUME_1");
    relay_solo1 = std::make_unique<juce::WebToggleButtonRelay>("SOLO_1");  // Pattern #19
    relay_mute1 = std::make_unique<juce::WebToggleButtonRelay>("MUTE_1");  // Pattern #19
    relay_lock1 = std::make_unique<juce::WebToggleButtonRelay>("LOCK_1");  // Pattern #19

    // Slot 2 relays (8 parameters)
    relay_decay2 = std::make_unique<juce::WebSliderRelay>("DECAY_2");
    relay_attack2 = std::make_unique<juce::WebSliderRelay>("ATTACK_2");
    relay_tiltFilter2 = std::make_unique<juce::WebSliderRelay>("TILT_FILTER_2");
    relay_pitch2 = std::make_unique<juce::WebSliderRelay>("PITCH_2");
    relay_volume2 = std::make_unique<juce::WebSliderRelay>("VOLUME_2");
    relay_solo2 = std::make_unique<juce::WebToggleButtonRelay>("SOLO_2");  // Pattern #19
    relay_mute2 = std::make_unique<juce::WebToggleButtonRelay>("MUTE_2");  // Pattern #19
    relay_lock2 = std::make_unique<juce::WebToggleButtonRelay>("LOCK_2");  // Pattern #19

    // Slot 3 relays (8 parameters)
    relay_decay3 = std::make_unique<juce::WebSliderRelay>("DECAY_3");
    relay_attack3 = std::make_unique<juce::WebSliderRelay>("ATTACK_3");
    relay_tiltFilter3 = std::make_unique<juce::WebSliderRelay>("TILT_FILTER_3");
    relay_pitch3 = std::make_unique<juce::WebSliderRelay>("PITCH_3");
    relay_volume3 = std::make_unique<juce::WebSliderRelay>("VOLUME_3");
    relay_solo3 = std::make_unique<juce::WebToggleButtonRelay>("SOLO_3");
    relay_mute3 = std::make_unique<juce::WebToggleButtonRelay>("MUTE_3");
    relay_lock3 = std::make_unique<juce::WebToggleButtonRelay>("LOCK_3");

    // Slot 4 relays (8 parameters)
    relay_decay4 = std::make_unique<juce::WebSliderRelay>("DECAY_4");
    relay_attack4 = std::make_unique<juce::WebSliderRelay>("ATTACK_4");
    relay_tiltFilter4 = std::make_unique<juce::WebSliderRelay>("TILT_FILTER_4");
    relay_pitch4 = std::make_unique<juce::WebSliderRelay>("PITCH_4");
    relay_volume4 = std::make_unique<juce::WebSliderRelay>("VOLUME_4");
    relay_solo4 = std::make_unique<juce::WebToggleButtonRelay>("SOLO_4");
    relay_mute4 = std::make_unique<juce::WebToggleButtonRelay>("MUTE_4");
    relay_lock4 = std::make_unique<juce::WebToggleButtonRelay>("LOCK_4");

    // Slot 5 relays (8 parameters)
    relay_decay5 = std::make_unique<juce::WebSliderRelay>("DECAY_5");
    relay_attack5 = std::make_unique<juce::WebSliderRelay>("ATTACK_5");
    relay_tiltFilter5 = std::make_unique<juce::WebSliderRelay>("TILT_FILTER_5");
    relay_pitch5 = std::make_unique<juce::WebSliderRelay>("PITCH_5");
    relay_volume5 = std::make_unique<juce::WebSliderRelay>("VOLUME_5");
    relay_solo5 = std::make_unique<juce::WebToggleButtonRelay>("SOLO_5");
    relay_mute5 = std::make_unique<juce::WebToggleButtonRelay>("MUTE_5");
    relay_lock5 = std::make_unique<juce::WebToggleButtonRelay>("LOCK_5");

    // Slot 6 relays (8 parameters)
    relay_decay6 = std::make_unique<juce::WebSliderRelay>("DECAY_6");
    relay_attack6 = std::make_unique<juce::WebSliderRelay>("ATTACK_6");
    relay_tiltFilter6 = std::make_unique<juce::WebSliderRelay>("TILT_FILTER_6");
    relay_pitch6 = std::make_unique<juce::WebSliderRelay>("PITCH_6");
    relay_volume6 = std::make_unique<juce::WebSliderRelay>("VOLUME_6");
    relay_solo6 = std::make_unique<juce::WebToggleButtonRelay>("SOLO_6");
    relay_mute6 = std::make_unique<juce::WebToggleButtonRelay>("MUTE_6");
    relay_lock6 = std::make_unique<juce::WebToggleButtonRelay>("LOCK_6");

    // Slot 7 relays (8 parameters)
    relay_decay7 = std::make_unique<juce::WebSliderRelay>("DECAY_7");
    relay_attack7 = std::make_unique<juce::WebSliderRelay>("ATTACK_7");
    relay_tiltFilter7 = std::make_unique<juce::WebSliderRelay>("TILT_FILTER_7");
    relay_pitch7 = std::make_unique<juce::WebSliderRelay>("PITCH_7");
    relay_volume7 = std::make_unique<juce::WebSliderRelay>("VOLUME_7");
    relay_solo7 = std::make_unique<juce::WebToggleButtonRelay>("SOLO_7");
    relay_mute7 = std::make_unique<juce::WebToggleButtonRelay>("MUTE_7");
    relay_lock7 = std::make_unique<juce::WebToggleButtonRelay>("LOCK_7");

    // Slot 8 relays (8 parameters)
    relay_decay8 = std::make_unique<juce::WebSliderRelay>("DECAY_8");
    relay_attack8 = std::make_unique<juce::WebSliderRelay>("ATTACK_8");
    relay_tiltFilter8 = std::make_unique<juce::WebSliderRelay>("TILT_FILTER_8");
    relay_pitch8 = std::make_unique<juce::WebSliderRelay>("PITCH_8");
    relay_volume8 = std::make_unique<juce::WebSliderRelay>("VOLUME_8");
    relay_solo8 = std::make_unique<juce::WebToggleButtonRelay>("SOLO_8");
    relay_mute8 = std::make_unique<juce::WebToggleButtonRelay>("MUTE_8");
    relay_lock8 = std::make_unique<juce::WebToggleButtonRelay>("LOCK_8");

    // ========================================================================
    // STEP 2: Create WebView SECOND with relay options (Pattern #11)
    // ========================================================================
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()  // CRITICAL: Enables JUCE JavaScript library
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withResourceProvider([this](const juce::String& url) { return getResource(url); })
            // Register all relays (slot 1)
            .withOptionsFrom(*relay_decay1)
            .withOptionsFrom(*relay_attack1)
            .withOptionsFrom(*relay_tiltFilter1)
            .withOptionsFrom(*relay_pitch1)
            .withOptionsFrom(*relay_volume1)
            .withOptionsFrom(*relay_solo1)
            .withOptionsFrom(*relay_mute1)
            .withOptionsFrom(*relay_lock1)
            // Register all relays (slot 2)
            .withOptionsFrom(*relay_decay2)
            .withOptionsFrom(*relay_attack2)
            .withOptionsFrom(*relay_tiltFilter2)
            .withOptionsFrom(*relay_pitch2)
            .withOptionsFrom(*relay_volume2)
            .withOptionsFrom(*relay_solo2)
            .withOptionsFrom(*relay_mute2)
            .withOptionsFrom(*relay_lock2)
            // Register all relays (slot 3)
            .withOptionsFrom(*relay_decay3)
            .withOptionsFrom(*relay_attack3)
            .withOptionsFrom(*relay_tiltFilter3)
            .withOptionsFrom(*relay_pitch3)
            .withOptionsFrom(*relay_volume3)
            .withOptionsFrom(*relay_solo3)
            .withOptionsFrom(*relay_mute3)
            .withOptionsFrom(*relay_lock3)
            // Register all relays (slot 4)
            .withOptionsFrom(*relay_decay4)
            .withOptionsFrom(*relay_attack4)
            .withOptionsFrom(*relay_tiltFilter4)
            .withOptionsFrom(*relay_pitch4)
            .withOptionsFrom(*relay_volume4)
            .withOptionsFrom(*relay_solo4)
            .withOptionsFrom(*relay_mute4)
            .withOptionsFrom(*relay_lock4)
            // Register all relays (slot 5)
            .withOptionsFrom(*relay_decay5)
            .withOptionsFrom(*relay_attack5)
            .withOptionsFrom(*relay_tiltFilter5)
            .withOptionsFrom(*relay_pitch5)
            .withOptionsFrom(*relay_volume5)
            .withOptionsFrom(*relay_solo5)
            .withOptionsFrom(*relay_mute5)
            .withOptionsFrom(*relay_lock5)
            // Register all relays (slot 6)
            .withOptionsFrom(*relay_decay6)
            .withOptionsFrom(*relay_attack6)
            .withOptionsFrom(*relay_tiltFilter6)
            .withOptionsFrom(*relay_pitch6)
            .withOptionsFrom(*relay_volume6)
            .withOptionsFrom(*relay_solo6)
            .withOptionsFrom(*relay_mute6)
            .withOptionsFrom(*relay_lock6)
            // Register all relays (slot 7)
            .withOptionsFrom(*relay_decay7)
            .withOptionsFrom(*relay_attack7)
            .withOptionsFrom(*relay_tiltFilter7)
            .withOptionsFrom(*relay_pitch7)
            .withOptionsFrom(*relay_volume7)
            .withOptionsFrom(*relay_solo7)
            .withOptionsFrom(*relay_mute7)
            .withOptionsFrom(*relay_lock7)
            // Register all relays (slot 8)
            .withOptionsFrom(*relay_decay8)
            .withOptionsFrom(*relay_attack8)
            .withOptionsFrom(*relay_tiltFilter8)
            .withOptionsFrom(*relay_pitch8)
            .withOptionsFrom(*relay_volume8)
            .withOptionsFrom(*relay_solo8)
            .withOptionsFrom(*relay_mute8)
            .withOptionsFrom(*relay_lock8)
    );

    // ========================================================================
    // STEP 3: Create attachments LAST (Pattern #11, #12)
    // ========================================================================

    // Slot 1 attachments (Pattern #12: 3 parameters - parameter, relay, nullptr)
    attach_decay1 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DECAY_1"), *relay_decay1, nullptr);
    attach_attack1 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("ATTACK_1"), *relay_attack1, nullptr);
    attach_tiltFilter1 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("TILT_FILTER_1"), *relay_tiltFilter1, nullptr);
    attach_pitch1 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("PITCH_1"), *relay_pitch1, nullptr);
    attach_volume1 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("VOLUME_1"), *relay_volume1, nullptr);
    attach_solo1 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("SOLO_1"), *relay_solo1, nullptr);
    attach_mute1 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("MUTE_1"), *relay_mute1, nullptr);
    attach_lock1 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("LOCK_1"), *relay_lock1, nullptr);

    // Slot 2 attachments
    attach_decay2 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DECAY_2"), *relay_decay2, nullptr);
    attach_attack2 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("ATTACK_2"), *relay_attack2, nullptr);
    attach_tiltFilter2 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("TILT_FILTER_2"), *relay_tiltFilter2, nullptr);
    attach_pitch2 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("PITCH_2"), *relay_pitch2, nullptr);
    attach_volume2 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("VOLUME_2"), *relay_volume2, nullptr);
    attach_solo2 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("SOLO_2"), *relay_solo2, nullptr);
    attach_mute2 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("MUTE_2"), *relay_mute2, nullptr);
    attach_lock2 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("LOCK_2"), *relay_lock2, nullptr);

    // Slot 3 attachments
    attach_decay3 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DECAY_3"), *relay_decay3, nullptr);
    attach_attack3 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("ATTACK_3"), *relay_attack3, nullptr);
    attach_tiltFilter3 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("TILT_FILTER_3"), *relay_tiltFilter3, nullptr);
    attach_pitch3 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("PITCH_3"), *relay_pitch3, nullptr);
    attach_volume3 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("VOLUME_3"), *relay_volume3, nullptr);
    attach_solo3 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("SOLO_3"), *relay_solo3, nullptr);
    attach_mute3 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("MUTE_3"), *relay_mute3, nullptr);
    attach_lock3 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("LOCK_3"), *relay_lock3, nullptr);

    // Slot 4 attachments
    attach_decay4 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DECAY_4"), *relay_decay4, nullptr);
    attach_attack4 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("ATTACK_4"), *relay_attack4, nullptr);
    attach_tiltFilter4 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("TILT_FILTER_4"), *relay_tiltFilter4, nullptr);
    attach_pitch4 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("PITCH_4"), *relay_pitch4, nullptr);
    attach_volume4 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("VOLUME_4"), *relay_volume4, nullptr);
    attach_solo4 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("SOLO_4"), *relay_solo4, nullptr);
    attach_mute4 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("MUTE_4"), *relay_mute4, nullptr);
    attach_lock4 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("LOCK_4"), *relay_lock4, nullptr);

    // Slot 5 attachments
    attach_decay5 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DECAY_5"), *relay_decay5, nullptr);
    attach_attack5 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("ATTACK_5"), *relay_attack5, nullptr);
    attach_tiltFilter5 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("TILT_FILTER_5"), *relay_tiltFilter5, nullptr);
    attach_pitch5 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("PITCH_5"), *relay_pitch5, nullptr);
    attach_volume5 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("VOLUME_5"), *relay_volume5, nullptr);
    attach_solo5 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("SOLO_5"), *relay_solo5, nullptr);
    attach_mute5 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("MUTE_5"), *relay_mute5, nullptr);
    attach_lock5 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("LOCK_5"), *relay_lock5, nullptr);

    // Slot 6 attachments
    attach_decay6 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DECAY_6"), *relay_decay6, nullptr);
    attach_attack6 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("ATTACK_6"), *relay_attack6, nullptr);
    attach_tiltFilter6 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("TILT_FILTER_6"), *relay_tiltFilter6, nullptr);
    attach_pitch6 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("PITCH_6"), *relay_pitch6, nullptr);
    attach_volume6 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("VOLUME_6"), *relay_volume6, nullptr);
    attach_solo6 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("SOLO_6"), *relay_solo6, nullptr);
    attach_mute6 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("MUTE_6"), *relay_mute6, nullptr);
    attach_lock6 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("LOCK_6"), *relay_lock6, nullptr);

    // Slot 7 attachments
    attach_decay7 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DECAY_7"), *relay_decay7, nullptr);
    attach_attack7 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("ATTACK_7"), *relay_attack7, nullptr);
    attach_tiltFilter7 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("TILT_FILTER_7"), *relay_tiltFilter7, nullptr);
    attach_pitch7 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("PITCH_7"), *relay_pitch7, nullptr);
    attach_volume7 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("VOLUME_7"), *relay_volume7, nullptr);
    attach_solo7 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("SOLO_7"), *relay_solo7, nullptr);
    attach_mute7 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("MUTE_7"), *relay_mute7, nullptr);
    attach_lock7 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("LOCK_7"), *relay_lock7, nullptr);

    // Slot 8 attachments
    attach_decay8 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DECAY_8"), *relay_decay8, nullptr);
    attach_attack8 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("ATTACK_8"), *relay_attack8, nullptr);
    attach_tiltFilter8 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("TILT_FILTER_8"), *relay_tiltFilter8, nullptr);
    attach_pitch8 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("PITCH_8"), *relay_pitch8, nullptr);
    attach_volume8 = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("VOLUME_8"), *relay_volume8, nullptr);
    attach_solo8 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("SOLO_8"), *relay_solo8, nullptr);
    attach_mute8 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("MUTE_8"), *relay_mute8, nullptr);
    attach_lock8 = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("LOCK_8"), *relay_lock8, nullptr);

    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    setSize(1400, 950);  // From v4-ui.yaml
}

DrumRouletteAudioProcessorEditor::~DrumRouletteAudioProcessorEditor() = default;

void DrumRouletteAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void DrumRouletteAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
DrumRouletteAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Pattern #8: Explicit URL mapping (NOT loop over BinaryData)
    // Root "/" → index.html
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // JUCE JavaScript library
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    // Pattern #13: check_native_interop.js (REQUIRED for WebView)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("DrumRoulette: Resource not found: " + url);
    return std::nullopt;
}
