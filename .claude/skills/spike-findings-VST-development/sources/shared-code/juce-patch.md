# JUCE Local-Fork Patch for VST3 Note Expression

**Applies to:** JUCE 8.0.4 at `/Users/taylorbrook/JUCE/`
**Purpose:** Surface `kNoteExpressionValueEvent` and noteId-tagged noteOn/noteOff events to the plugin, which upstream JUCE silently drops in `MidiEventList::toMidiBuffer`.

JUCE is NOT a git repo at that path, so no `.patch` file. Reapply by pasting these hunks back in after any JUCE upgrade. Both edits are commented `// JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22)` for grep discoverability.

## Patch 1/2 — `modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h`

Inside `struct VST3ClientExtensions`, right after the `~VST3ClientExtensions() = default;` line (around line 60 in the unpatched file), insert:

```cpp
//==============================================================================
// JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22):
// Surface raw VST3 input events — including kNoteExpressionValueEvent and the
// noteIds on kNoteOnEvent/kNoteOffEvent — to the plugin, which upstream JUCE
// drops when it converts events to a MidiBuffer. Called by the VST3 wrapper
// on the audio thread immediately before MidiEventList::toMidiBuffer, once
// per input event, in host-supplied order.
struct Vst3RawEvent
{
    enum class Kind : uint8_t
    {
        NoteOn,
        NoteOff,
        NoteExpressionValue,
    };

    Kind     kind{};
    int32_t  sampleOffset = 0;
    int32_t  noteId       = -1;
    int16_t  pitch        = -1;
    int16_t  channel      = -1;
    uint32_t typeId       = 0;
    double   value        = 0.0;
};

/** Called once per input VST3 event on the audio thread, immediately before
    events are converted to the plugin's MidiBuffer. Override to receive
    Note Expression events and note-ID-tagged NoteOn/NoteOff events. Default
    implementation is a no-op (events are simply dropped as in upstream JUCE).
*/
virtual void onVst3RawEvent (const Vst3RawEvent&) {}
```

## Patch 2/2 — `modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp`

Find the `JucePlugin_WantsMidiInput` block in the VST3 wrapper's process path (around line 3696):

**Original:**
```cpp
#if JucePlugin_WantsMidiInput
if (isMidiInputBusEnabled && data.inputEvents != nullptr)
    MidiEventList::toMidiBuffer (midiBuffer, *data.inputEvents);
#endif
```

**Replace with:**
```cpp
#if JucePlugin_WantsMidiInput
if (isMidiInputBusEnabled && data.inputEvents != nullptr)
{
    // JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22):
    // Forward raw VST3 events (kNoteExpressionValueEvent + noteId-carrying
    // kNoteOn/kNoteOff) to VST3ClientExtensions before MIDI conversion.
    if (auto* ext = pluginInstance->getVST3ClientExtensions())
    {
        const auto numRawEvents = data.inputEvents->getEventCount();
        for (Steinberg::int32 i = 0; i < numRawEvents; ++i)
        {
            Steinberg::Vst::Event e;
            if (data.inputEvents->getEvent (i, e) != Steinberg::kResultOk)
                continue;

            VST3ClientExtensions::Vst3RawEvent raw{};
            raw.sampleOffset = (int32_t) e.sampleOffset;

            using Kind = VST3ClientExtensions::Vst3RawEvent::Kind;

            if (e.type == Steinberg::Vst::Event::kNoteOnEvent)
            {
                raw.kind    = Kind::NoteOn;
                raw.noteId  = e.noteOn.noteId;
                raw.pitch   = e.noteOn.pitch;
                raw.channel = e.noteOn.channel;
            }
            else if (e.type == Steinberg::Vst::Event::kNoteOffEvent)
            {
                raw.kind    = Kind::NoteOff;
                raw.noteId  = e.noteOff.noteId;
                raw.pitch   = e.noteOff.pitch;
                raw.channel = e.noteOff.channel;
            }
            else if (e.type == Steinberg::Vst::Event::kNoteExpressionValueEvent)
            {
                raw.kind   = Kind::NoteExpressionValue;
                raw.noteId = e.noteExpressionValue.noteId;
                raw.typeId = e.noteExpressionValue.typeId;
                raw.value  = e.noteExpressionValue.value;
            }
            else
            {
                continue;
            }

            ext->onVst3RawEvent (raw);
        }
    }

    MidiEventList::toMidiBuffer (midiBuffer, *data.inputEvents);
}
#endif
```

## Verification after applying

```bash
grep -rn "JUCE-NE-PATCH" /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/ \
                        /Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/
```

Should return 4 hits (2 per file: one in header, one marker + the full block in the cpp).
