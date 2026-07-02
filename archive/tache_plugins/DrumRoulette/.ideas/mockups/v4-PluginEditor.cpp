#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrumRouletteAudioProcessorEditor::DrumRouletteAudioProcessorEditor (DrumRouletteAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      // Initialize relays (Pattern #15: Relay creation before WebView)
      // Slot 1
      relay_decay1 (*audioProcessor.apvts.getParameter ("DECAY_1")),
      relay_attack1 (*audioProcessor.apvts.getParameter ("ATTACK_1")),
      relay_tilt1 (*audioProcessor.apvts.getParameter ("TILT_1")),
      relay_pitch1 (*audioProcessor.apvts.getParameter ("PITCH_1")),
      relay_volume1 (*audioProcessor.apvts.getParameter ("VOLUME_1")),
      relay_solo1 (*audioProcessor.apvts.getParameter ("SOLO_1")),
      relay_mute1 (*audioProcessor.apvts.getParameter ("MUTE_1")),
      // Slot 2
      relay_decay2 (*audioProcessor.apvts.getParameter ("DECAY_2")),
      relay_attack2 (*audioProcessor.apvts.getParameter ("ATTACK_2")),
      relay_tilt2 (*audioProcessor.apvts.getParameter ("TILT_2")),
      relay_pitch2 (*audioProcessor.apvts.getParameter ("PITCH_2")),
      relay_volume2 (*audioProcessor.apvts.getParameter ("VOLUME_2")),
      relay_solo2 (*audioProcessor.apvts.getParameter ("SOLO_2")),
      relay_mute2 (*audioProcessor.apvts.getParameter ("MUTE_2")),
      // Slot 3
      relay_decay3 (*audioProcessor.apvts.getParameter ("DECAY_3")),
      relay_attack3 (*audioProcessor.apvts.getParameter ("ATTACK_3")),
      relay_tilt3 (*audioProcessor.apvts.getParameter ("TILT_3")),
      relay_pitch3 (*audioProcessor.apvts.getParameter ("PITCH_3")),
      relay_volume3 (*audioProcessor.apvts.getParameter ("VOLUME_3")),
      relay_solo3 (*audioProcessor.apvts.getParameter ("SOLO_3")),
      relay_mute3 (*audioProcessor.apvts.getParameter ("MUTE_3")),
      // Slot 4
      relay_decay4 (*audioProcessor.apvts.getParameter ("DECAY_4")),
      relay_attack4 (*audioProcessor.apvts.getParameter ("ATTACK_4")),
      relay_tilt4 (*audioProcessor.apvts.getParameter ("TILT_4")),
      relay_pitch4 (*audioProcessor.apvts.getParameter ("PITCH_4")),
      relay_volume4 (*audioProcessor.apvts.getParameter ("VOLUME_4")),
      relay_solo4 (*audioProcessor.apvts.getParameter ("SOLO_4")),
      relay_mute4 (*audioProcessor.apvts.getParameter ("MUTE_4")),
      // Slot 5
      relay_decay5 (*audioProcessor.apvts.getParameter ("DECAY_5")),
      relay_attack5 (*audioProcessor.apvts.getParameter ("ATTACK_5")),
      relay_tilt5 (*audioProcessor.apvts.getParameter ("TILT_5")),
      relay_pitch5 (*audioProcessor.apvts.getParameter ("PITCH_5")),
      relay_volume5 (*audioProcessor.apvts.getParameter ("VOLUME_5")),
      relay_solo5 (*audioProcessor.apvts.getParameter ("SOLO_5")),
      relay_mute5 (*audioProcessor.apvts.getParameter ("MUTE_5")),
      // Slot 6
      relay_decay6 (*audioProcessor.apvts.getParameter ("DECAY_6")),
      relay_attack6 (*audioProcessor.apvts.getParameter ("ATTACK_6")),
      relay_tilt6 (*audioProcessor.apvts.getParameter ("TILT_6")),
      relay_pitch6 (*audioProcessor.apvts.getParameter ("PITCH_6")),
      relay_volume6 (*audioProcessor.apvts.getParameter ("VOLUME_6")),
      relay_solo6 (*audioProcessor.apvts.getParameter ("SOLO_6")),
      relay_mute6 (*audioProcessor.apvts.getParameter ("MUTE_6")),
      // Slot 7
      relay_decay7 (*audioProcessor.apvts.getParameter ("DECAY_7")),
      relay_attack7 (*audioProcessor.apvts.getParameter ("ATTACK_7")),
      relay_tilt7 (*audioProcessor.apvts.getParameter ("TILT_7")),
      relay_pitch7 (*audioProcessor.apvts.getParameter ("PITCH_7")),
      relay_volume7 (*audioProcessor.apvts.getParameter ("VOLUME_7")),
      relay_solo7 (*audioProcessor.apvts.getParameter ("SOLO_7")),
      relay_mute7 (*audioProcessor.apvts.getParameter ("MUTE_7")),
      // Slot 8
      relay_decay8 (*audioProcessor.apvts.getParameter ("DECAY_8")),
      relay_attack8 (*audioProcessor.apvts.getParameter ("ATTACK_8")),
      relay_tilt8 (*audioProcessor.apvts.getParameter ("TILT_8")),
      relay_pitch8 (*audioProcessor.apvts.getParameter ("PITCH_8")),
      relay_volume8 (*audioProcessor.apvts.getParameter ("VOLUME_8")),
      relay_solo8 (*audioProcessor.apvts.getParameter ("SOLO_8")),
      relay_mute8 (*audioProcessor.apvts.getParameter ("MUTE_8")),
      // Master
      relay_masterVolume (*audioProcessor.apvts.getParameter ("MASTER_VOLUME")),
      // Initialize WebView with resource provider (Pattern #16)
      webView (juce::WebBrowserComponent::Options()
          .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
          .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2::backgroundColour (juce::Colour (0xff1a1a1a)))
          .withResourceProvider ([](const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
          {
              if (url == "/")
              {
                  return juce::WebBrowserComponent::Resource {
                      BinaryData::index_html, BinaryData::index_htmlSize, "text/html"
                  };
              }
              if (url == "/js/juce/index.js")
              {
                  return juce::WebBrowserComponent::Resource {
                      BinaryData::index_js, BinaryData::index_jsSize, "text/javascript"
                  };
              }
              return std::nullopt;
          }, juce::URL {"http://drumroulette.local/"})),
      // Initialize attachments AFTER webView (Pattern #14)
      // Slot 1
      attach_decay1 (audioProcessor.apvts, "DECAY_1", relay_decay1),
      attach_attack1 (audioProcessor.apvts, "ATTACK_1", relay_attack1),
      attach_tilt1 (audioProcessor.apvts, "TILT_1", relay_tilt1),
      attach_pitch1 (audioProcessor.apvts, "PITCH_1", relay_pitch1),
      attach_volume1 (audioProcessor.apvts, "VOLUME_1", relay_volume1),
      attach_solo1 (audioProcessor.apvts, "SOLO_1", relay_solo1),
      attach_mute1 (audioProcessor.apvts, "MUTE_1", relay_mute1),
      // Slot 2
      attach_decay2 (audioProcessor.apvts, "DECAY_2", relay_decay2),
      attach_attack2 (audioProcessor.apvts, "ATTACK_2", relay_attack2),
      attach_tilt2 (audioProcessor.apvts, "TILT_2", relay_tilt2),
      attach_pitch2 (audioProcessor.apvts, "PITCH_2", relay_pitch2),
      attach_volume2 (audioProcessor.apvts, "VOLUME_2", relay_volume2),
      attach_solo2 (audioProcessor.apvts, "SOLO_2", relay_solo2),
      attach_mute2 (audioProcessor.apvts, "MUTE_2", relay_mute2),
      // Slot 3
      attach_decay3 (audioProcessor.apvts, "DECAY_3", relay_decay3),
      attach_attack3 (audioProcessor.apvts, "ATTACK_3", relay_attack3),
      attach_tilt3 (audioProcessor.apvts, "TILT_3", relay_tilt3),
      attach_pitch3 (audioProcessor.apvts, "PITCH_3", relay_pitch3),
      attach_volume3 (audioProcessor.apvts, "VOLUME_3", relay_volume3),
      attach_solo3 (audioProcessor.apvts, "SOLO_3", relay_solo3),
      attach_mute3 (audioProcessor.apvts, "MUTE_3", relay_mute3),
      // Slot 4
      attach_decay4 (audioProcessor.apvts, "DECAY_4", relay_decay4),
      attach_attack4 (audioProcessor.apvts, "ATTACK_4", relay_attack4),
      attach_tilt4 (audioProcessor.apvts, "TILT_4", relay_tilt4),
      attach_pitch4 (audioProcessor.apvts, "PITCH_4", relay_pitch4),
      attach_volume4 (audioProcessor.apvts, "VOLUME_4", relay_volume4),
      attach_solo4 (audioProcessor.apvts, "SOLO_4", relay_solo4),
      attach_mute4 (audioProcessor.apvts, "MUTE_4", relay_mute4),
      // Slot 5
      attach_decay5 (audioProcessor.apvts, "DECAY_5", relay_decay5),
      attach_attack5 (audioProcessor.apvts, "ATTACK_5", relay_attack5),
      attach_tilt5 (audioProcessor.apvts, "TILT_5", relay_tilt5),
      attach_pitch5 (audioProcessor.apvts, "PITCH_5", relay_pitch5),
      attach_volume5 (audioProcessor.apvts, "VOLUME_5", relay_volume5),
      attach_solo5 (audioProcessor.apvts, "SOLO_5", relay_solo5),
      attach_mute5 (audioProcessor.apvts, "MUTE_5", relay_mute5),
      // Slot 6
      attach_decay6 (audioProcessor.apvts, "DECAY_6", relay_decay6),
      attach_attack6 (audioProcessor.apvts, "ATTACK_6", relay_attack6),
      attach_tilt6 (audioProcessor.apvts, "TILT_6", relay_tilt6),
      attach_pitch6 (audioProcessor.apvts, "PITCH_6", relay_pitch6),
      attach_volume6 (audioProcessor.apvts, "VOLUME_6", relay_volume6),
      attach_solo6 (audioProcessor.apvts, "SOLO_6", relay_solo6),
      attach_mute6 (audioProcessor.apvts, "MUTE_6", relay_mute6),
      // Slot 7
      attach_decay7 (audioProcessor.apvts, "DECAY_7", relay_decay7),
      attach_attack7 (audioProcessor.apvts, "ATTACK_7", relay_attack7),
      attach_tilt7 (audioProcessor.apvts, "TILT_7", relay_tilt7),
      attach_pitch7 (audioProcessor.apvts, "PITCH_7", relay_pitch7),
      attach_volume7 (audioProcessor.apvts, "VOLUME_7", relay_volume7),
      attach_solo7 (audioProcessor.apvts, "SOLO_7", relay_solo7),
      attach_mute7 (audioProcessor.apvts, "MUTE_7", relay_mute7),
      // Slot 8
      attach_decay8 (audioProcessor.apvts, "DECAY_8", relay_decay8),
      attach_attack8 (audioProcessor.apvts, "ATTACK_8", relay_attack8),
      attach_tilt8 (audioProcessor.apvts, "TILT_8", relay_tilt8),
      attach_pitch8 (audioProcessor.apvts, "PITCH_8", relay_pitch8),
      attach_volume8 (audioProcessor.apvts, "VOLUME_8", relay_volume8),
      attach_solo8 (audioProcessor.apvts, "SOLO_8", relay_solo8),
      attach_mute8 (audioProcessor.apvts, "MUTE_8", relay_mute8),
      // Master
      attach_masterVolume (audioProcessor.apvts, "MASTER_VOLUME", relay_masterVolume)
{
    // Set plugin window size (from mockup v4: 1400 x 950)
    setSize (1400, 950);

    // Add and display WebView
    addAndMakeVisible (webView);

    // Navigate to root URL (loads index.html)
    webView.goToURL ("/");
}

DrumRouletteAudioProcessorEditor::~DrumRouletteAudioProcessorEditor()
{
}

//==============================================================================
void DrumRouletteAudioProcessorEditor::paint (juce::Graphics& g)
{
    // WebView fills entire window - no need to paint background
    g.fillAll (juce::Colour (0xff1a1a1a));
}

void DrumRouletteAudioProcessorEditor::resized()
{
    // WebView fills entire window
    webView.setBounds (getLocalBounds());
}
