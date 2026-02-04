// ===== Footer Keyboard - PluginEditor native function =====
// Add this to your WebBrowserComponent options in PluginEditor.cpp

// In PluginEditor constructor, add to your WebView options chain:

.withNativeFunction("sendMidiNote", [this](const juce::Array<juce::var>& args,
                                            std::function<void(juce::var)> complete) {
    if (args.size() >= 3) {
        int midiNote = static_cast<int>(args[0]);
        float velocity = static_cast<float>(args[1]);
        bool isNoteOn = static_cast<bool>(args[2]);

        if (isNoteOn)
            processorRef.triggerNoteOn(midiNote, velocity);
        else
            processorRef.triggerNoteOff(midiNote);
    }
    complete({});
})


// ----- Full example context -----
// Your WebView initialization might look like:

webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options()
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withResourceProvider([this](const auto& url) { return getResource(url); },
                               juce::URL("http://localhost/index.html"))
        .withUserAgent("JUCE WebBrowserComponent")

        // ... your existing native functions ...

        // Add the sendMidiNote function:
        .withNativeFunction("sendMidiNote", [this](const juce::Array<juce::var>& args,
                                                    std::function<void(juce::var)> complete) {
            if (args.size() >= 3) {
                int midiNote = static_cast<int>(args[0]);
                float velocity = static_cast<float>(args[1]);
                bool isNoteOn = static_cast<bool>(args[2]);

                if (isNoteOn)
                    processorRef.triggerNoteOn(midiNote, velocity);
                else
                    processorRef.triggerNoteOff(midiNote);
            }
            complete({});
        })
);
