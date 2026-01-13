# playable-keyboard

Interactive MIDI keyboard for WebView-based Ouaricon plugins.

## Features

- **Mouse and Touch Input**: Click or tap keys to play notes
- **Configurable Size**: 1-5 octaves, any starting note
- **Visual Feedback**: Keys highlight when pressed
- **MIDI Output**: Sends note on/off via native function
- **Pitch Circle Integration**: Optional visual note highlighting
- **Computer Keyboard**: Optional QWERTY mapping

## Installation

```cmake
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
ouaricon_add_module(MyPlugin playable-keyboard)
```

## C++ Setup

### Register Native Function

```cpp
// In PluginEditor constructor:
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()

        .withNativeFunction("sendMidiNote", [this](auto& args, auto complete) {
            if (args.size() >= 3) {
                int midiNote = static_cast<int>(args[0]);
                float velocity = static_cast<float>(args[1]);
                bool isNoteOn = static_cast<bool>(args[2]);

                // Create MIDI message
                auto message = isNoteOn
                    ? juce::MidiMessage::noteOn(1, midiNote, velocity)
                    : juce::MidiMessage::noteOff(1, midiNote);

                // Send to processor
                processorRef.addMidiMessage(message);
                complete(true);
            } else {
                complete(false);
            }
        })

        // ... other options
);
```

### Processor MIDI Handling

```cpp
class MyProcessor : public juce::AudioProcessor
{
public:
    // Thread-safe MIDI message queue
    void addMidiMessage(const juce::MidiMessage& message)
    {
        const juce::ScopedLock sl(midiLock);
        pendingMidi.addEvent(message, 0);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
    {
        // Merge pending MIDI from UI
        {
            const juce::ScopedLock sl(midiLock);
            midiMessages.addEvents(pendingMidi, 0, buffer.getNumSamples(), 0);
            pendingMidi.clear();
        }

        // ... synthesize audio
    }

private:
    juce::CriticalSection midiLock;
    juce::MidiBuffer pendingMidi;
};
```

## JavaScript Usage

### Basic Keyboard

```html
<div id="keyboard" style="width: 300px; height: 80px;"></div>

<script type="module">
    import { PlayableKeyboard } from './modules/playable-keyboard.js';

    const keyboard = new PlayableKeyboard({
        container: document.getElementById('keyboard'),
        octaves: 1,
        startNote: 60,  // C4
        onNoteOn: (note, velocity) => {
            console.log('Note on:', note, velocity);
        },
        onNoteOff: (note) => {
            console.log('Note off:', note);
        }
    });

    keyboard.initialize();
</script>
```

### With Pitch Circle

```html
<div id="keyboard"></div>
<div id="circle"></div>

<script type="module">
    import { PlayableKeyboard } from './modules/playable-keyboard.js';
    import { PitchCircle } from './modules/pitch-circle.js';

    const circle = new PitchCircle({
        container: document.getElementById('circle'),
        size: 150
    });

    const keyboard = new PlayableKeyboard({
        container: document.getElementById('keyboard'),
        pitchCircle: circle,
        octaves: 2,
        startNote: 48  // C3
    });

    keyboard.initialize();
</script>
```

### With Computer Keyboard

```javascript
import { PlayableKeyboard, enableQwertyInput } from './modules/playable-keyboard.js';

const keyboard = new PlayableKeyboard({
    container: document.getElementById('keyboard')
});

keyboard.initialize();

// Enable QWERTY keyboard input
enableQwertyInput(keyboard);

// Bottom row: Z-M = C4-B4
// Top row: Q-P = C5+
```

### Factory Functions

```javascript
import { createSimpleKeyboard, createKeyboardWithCircle } from './modules/playable-keyboard.js';

// Simple one-octave keyboard
const keyboard = createSimpleKeyboard('keyboard-container');

// Keyboard with pitch circle
const { keyboard, pitchCircle } = createKeyboardWithCircle(
    'keyboard-container',
    'circle-container',
    { octaves: 2 }
);
```

## Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `container` | HTMLElement | required | Container element |
| `octaves` | number | 1 | Number of octaves (1-5) |
| `startNote` | number | 60 | MIDI note for first key (60 = C4) |
| `showLabels` | boolean | true | Show note names on white keys |
| `velocity` | number | 0.8 | Default velocity (0-1) |
| `highlightColor` | string | '#8BC34A' | Color for playing keys |
| `pitchCircle` | PitchCircle | null | Optional pitch circle integration |

## QWERTY Keyboard Mapping

```
  2 3   5 6 7   9 0
 Q W E R T Y U I O P
 ─┬─┬───┬─┬─┬───┬─┬───  C5+

  S D   G H J
 Z X C V B N M
 ─┬─┬───┬─┬─┬───┬─  C4-B4
```

## Programmatic Control

```javascript
// Trigger a note
keyboard.triggerNote(60);  // C4

// Trigger with velocity
keyboard.triggerNote(64, 0.5);  // E4 at 50% velocity

// Trigger for duration
keyboard.triggerNote(67, 0.8, 500);  // G4 for 500ms

// Release a note
keyboard.releaseNote(60);

// Release all notes
keyboard.allNotesOff();

// Change velocity
keyboard.setVelocity(0.6);

// Get playing notes
const playing = keyboard.getPlayingNotes();
```

## CSS Customization

```css
/* Container */
.playable-keyboard {
    height: 100px;
    border-radius: 8px;
}

/* White keys */
.playable-keyboard .white-key {
    background: #FFFFFF;
}

.playable-keyboard .white-key.playing {
    background: #4CAF50 !important;
}

/* Black keys */
.playable-keyboard .black-key {
    background: #1A1A1A;
}

.playable-keyboard .black-key.playing {
    background: #4CAF50 !important;
}
```

## Version History

### 1.0.0 (2026-01-12)
- Initial extraction from OuariconMarimba
- Configurable octave count and start note
- Mouse and touch support
- Computer keyboard (QWERTY) mapping
- Pitch circle integration
