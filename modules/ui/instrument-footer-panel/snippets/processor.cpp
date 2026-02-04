// ===== Footer Keyboard - PluginProcessor additions =====
// Add these methods to enable WebView keyboard → synth communication

// ----- In PluginProcessor.h -----
// Add to public section:

    // Note trigger methods for WebView keyboard
    void triggerNoteOn(int midiNote, float velocity);
    void triggerNoteOff(int midiNote);


// ----- In PluginProcessor.cpp -----
// Add these implementations:

void YourProcessorName::triggerNoteOn(int midiNote, float velocity)
{
    midiNote = juce::jlimit(0, 127, midiNote);
    velocity = juce::jlimit(0.0f, 1.0f, velocity);

    // For synthesisers:
    synthesiser.noteOn(1, midiNote, velocity);

    // Alternative: If you use a different synth architecture, adjust accordingly:
    // mySynth.startNote(midiNote, velocity);
}

void YourProcessorName::triggerNoteOff(int midiNote)
{
    midiNote = juce::jlimit(0, 127, midiNote);

    // For synthesisers:
    synthesiser.noteOff(1, midiNote, 0.0f, true);

    // Alternative: If you use a different synth architecture:
    // mySynth.stopNote(midiNote);
}
