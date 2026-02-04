# Instrument Footer Panel

Reusable sticky footer component for WebView-based VST instruments.

## Overview

This module provides a complete bottom panel containing:

| Left | Center | Right |
|------|--------|-------|
| Master Volume | Playable Keyboard | Branding |

The panel is fully responsive and adapts to different plugin widths.

---

## Integration Approach

> **IMPORTANT:** Based on real-world integration experience (O-Bells v2.2.0), the **surgical integration** approach works better than importing the standalone JS module.

### Why Surgical Integration?

The standalone `InstrumentFooterPanel` class assumes a clean slate, but real plugins have:
- Existing parameter binding patterns (WebSliderRelay/Attachment)
- Footer CSS that may overlap with other UI elements
- Specific initialization order requirements

**Surgical integration** means:
1. Adding CSS styles inline (without replacing existing footer CSS)
2. Expanding existing footer HTML structure
3. Inlining keyboard JS after existing parameter bindings
4. Using the plugin's native function pattern

---

## Surgical Integration Guide

### Step 1: Update Footer Height

Change your `.footer` or `.tab-content` CSS to accommodate the larger footer:

```css
/* Before: 40px footer */
.tab-content {
    height: calc(100% - 110px);  /* header + old footer */
}

/* After: 55px footer */
.tab-content {
    height: calc(100% - 125px);  /* header + new 55px footer */
}

.footer {
    height: 55px;  /* Was 40px */
}
```

### Step 2: Add Keyboard CSS

**Add these styles to your existing `<style>` block** (don't replace footer CSS entirely):

```css
/* ===== Footer Keyboard Styles ===== */
.footer-keyboard-viz {
    display: flex;
    position: relative;
    height: 42px;
    background: var(--panel-bg, #FAF0E6);
    border: 1px solid rgba(139, 115, 85, 0.6);
    border-radius: 3px;
    overflow: visible;
    flex: 1;
    max-width: 420px;
    min-width: 200px;
    margin: 0 15px;
}

.footer-keyboard-viz .white-key {
    flex: 1;
    background: rgba(139, 168, 112, 0.35);
    border-right: 1px solid rgba(139, 115, 85, 0.6);
    display: flex;
    flex-direction: column;
    justify-content: flex-end;
    align-items: center;
    padding-bottom: 2px;
    transition: background 0.15s, transform 0.1s;
    cursor: pointer;
    z-index: 1;
    min-width: 0;
}

.footer-keyboard-viz .white-key:last-of-type {
    border-right: none;
}

.footer-keyboard-viz .white-key span {
    font-size: 6px;
    color: #5C4033;
    pointer-events: none;
    opacity: 0.7;
}

.footer-keyboard-viz .white-key:hover:not(.pressed) {
    background: rgba(139, 168, 112, 0.5);
}

.footer-keyboard-viz .white-key.pressed {
    background: #8BC34A !important;
    transform: translateY(1px);
    z-index: 2;
}

.footer-keyboard-viz .black-key {
    position: absolute;
    width: 14px;
    height: 28px;
    background: #4A6B2A;
    border-radius: 0 0 2px 2px;
    top: 0;
    z-index: 10;
    transition: background 0.15s, transform 0.1s;
    cursor: pointer;
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.3);
}

.footer-keyboard-viz .black-key:hover:not(.pressed) {
    background: #5A8B3A;
}

.footer-keyboard-viz .black-key.pressed {
    background: #8BC34A !important;
    transform: translateY(1px);
}

.footer-help-text {
    font-size: 7px;
    color: #8B7355;
    text-align: center;
    margin-top: 2px;
}
```

### Step 3: Update Footer HTML

Modify your existing `<div class="footer">` to include the keyboard:

```html
<div class="footer">
    <!-- Master/Gain Slider (left) -->
    <div class="footer-gain">
        <span class="footer-gain-label">Gain</span>
        <input type="range" class="footer-gain-slider" data-param="outputGain">
        <span class="footer-gain-value" id="outputGainValue">0.0 dB</span>
    </div>

    <!-- Keyboard (center) -->
    <div class="footer-keyboard-viz" id="keyboard-viz"></div>

    <!-- Branding (right) -->
    <span class="footer-brand">Ouaricon Audio</span>
</div>
```

### Step 4: Add Keyboard JavaScript

**Add this code AFTER your existing parameter binding code** (the keyboard needs `parameterStates` to be populated):

```javascript
// ===== Footer Keyboard =====
(function initFooterKeyboard() {
    const keyboardViz = document.getElementById('keyboard-viz');
    if (!keyboardViz) return;

    const octaves = 2;
    const startNote = 48;  // C3
    const velocity = 0.8;
    const playingKeys = new Set();
    const qwertyPressed = new Set();

    // QWERTY mapping
    const qwertyMap = {
        'z': 48, 's': 49, 'x': 50, 'd': 51, 'c': 52, 'v': 53, 'g': 54,
        'b': 55, 'h': 56, 'n': 57, 'j': 58, 'm': 59,
        'q': 60, '2': 61, 'w': 62, '3': 63, 'e': 64, 'r': 65, '5': 66,
        't': 67, '6': 68, 'y': 69, '7': 70, 'u': 71,
        'i': 72, '9': 73, 'o': 74, '0': 75, 'p': 76
    };

    const whiteSemitones = [0, 2, 4, 5, 7, 9, 11];
    const blackPositions = [0, 1, 3, 4, 5];
    const blackSemitones = [1, 3, 6, 8, 10];
    const noteNames = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];

    // Get native function
    let sendMidiNote = null;
    if (window.Juce && window.Juce.getNativeFunction) {
        sendMidiNote = window.Juce.getNativeFunction('sendMidiNote');
    }

    // Build white keys
    const totalWhiteKeys = octaves * 7;
    for (let oct = 0; oct < octaves; oct++) {
        for (let i = 0; i < 7; i++) {
            const midiNote = startNote + oct * 12 + whiteSemitones[i];
            const key = document.createElement('div');
            key.className = 'white-key';
            key.dataset.note = midiNote;

            const label = document.createElement('span');
            label.textContent = i === 0 ? `C${Math.floor(midiNote / 12) - 1}` : noteNames[i];
            key.appendChild(label);

            keyboardViz.appendChild(key);
        }
    }

    // Build black keys
    for (let oct = 0; oct < octaves; oct++) {
        for (let i = 0; i < blackPositions.length; i++) {
            const whiteIdx = oct * 7 + blackPositions[i];
            const midiNote = startNote + oct * 12 + blackSemitones[i];
            const key = document.createElement('div');
            key.className = 'black-key';
            key.dataset.note = midiNote;
            key.style.left = `calc(100% / ${totalWhiteKeys} * ${whiteIdx + 1} - 7px)`;
            keyboardViz.appendChild(key);
        }
    }

    // Note handlers
    function noteOn(note) {
        if (playingKeys.has(note)) return;
        playingKeys.add(note);
        const el = keyboardViz.querySelector(`[data-note="${note}"]`);
        if (el) el.classList.add('pressed');
        if (sendMidiNote) sendMidiNote(note, velocity, true);
    }

    function noteOff(note) {
        if (!playingKeys.has(note)) return;
        playingKeys.delete(note);
        const el = keyboardViz.querySelector(`[data-note="${note}"]`);
        if (el) el.classList.remove('pressed');
        if (sendMidiNote) sendMidiNote(note, 0, false);
    }

    // Mouse events
    keyboardViz.querySelectorAll('.white-key, .black-key').forEach(key => {
        const note = parseInt(key.dataset.note);
        key.addEventListener('mousedown', e => { e.preventDefault(); noteOn(note); });
        key.addEventListener('mouseup', () => noteOff(note));
        key.addEventListener('mouseleave', () => { if (playingKeys.has(note)) noteOff(note); });
        key.addEventListener('touchstart', e => { e.preventDefault(); noteOn(note); });
        key.addEventListener('touchend', e => { e.preventDefault(); noteOff(note); });
    });

    document.addEventListener('mouseup', () => {
        playingKeys.forEach(n => noteOff(n));
    });

    // QWERTY input
    document.addEventListener('keydown', e => {
        if (e.repeat || e.target.tagName === 'INPUT') return;
        const baseNote = qwertyMap[e.key.toLowerCase()];
        if (baseNote !== undefined && !qwertyPressed.has(e.key)) {
            const note = baseNote + (startNote - 48);
            const maxNote = startNote + octaves * 12 - 1;
            if (note >= startNote && note <= maxNote) {
                qwertyPressed.add(e.key);
                noteOn(note);
            }
        }
    });

    document.addEventListener('keyup', e => {
        const baseNote = qwertyMap[e.key.toLowerCase()];
        if (baseNote !== undefined) {
            qwertyPressed.delete(e.key);
            const note = baseNote + (startNote - 48);
            noteOff(note);
        }
    });
})();
```

### Step 5: Add C++ Native Function

**In PluginProcessor.h**, add these methods:

```cpp
// Note trigger methods for WebView keyboard
void triggerNoteOn(int midiNote, float velocity);
void triggerNoteOff(int midiNote);
```

**In PluginProcessor.cpp**, implement them:

```cpp
void YourProcessor::triggerNoteOn(int midiNote, float velocity)
{
    midiNote = juce::jlimit(0, 127, midiNote);
    velocity = juce::jlimit(0.0f, 1.0f, velocity);
    synthesiser.noteOn(1, midiNote, velocity);
}

void YourProcessor::triggerNoteOff(int midiNote)
{
    midiNote = juce::jlimit(0, 127, midiNote);
    synthesiser.noteOff(1, midiNote, 0.0f, true);
}
```

**In PluginEditor.cpp**, register the native function in WebView options:

```cpp
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
```

---

## Configuration

Customize the keyboard by editing these values in the JS:

| Variable | Default | Description |
|----------|---------|-------------|
| `octaves` | 2 | Number of octaves (1-3) |
| `startNote` | 48 | First MIDI note (48 = C3, 60 = C4) |
| `velocity` | 0.8 | Default note velocity (0-1) |

---

## QWERTY Keyboard Mapping

```
Bottom Row: Z S X D C V G B H N J M
            C3  D3  E3 F3  G3  A3  B3

Top Row:    Q 2 W 3 E R 5 T 6 Y 7 U I 9 O 0 P
            C4  D4  E4F4  G4  A4  B4C5  D5  E5
```

---

## Theming

Override CSS variables to match your plugin's aesthetic:

```css
.footer-keyboard-viz {
    background: #1a1a2e;  /* Dark background */
}

.footer-keyboard-viz .white-key {
    background: rgba(100, 100, 150, 0.35);
}

.footer-keyboard-viz .white-key.pressed,
.footer-keyboard-viz .black-key.pressed {
    background: #e94560 !important;  /* Custom active color */
}
```

---

## What NOT To Do

Based on O-Bells integration experience:

1. **Don't replace footer CSS entirely** - This can break styles for expandable sections, meters, etc.
2. **Don't use the standalone JS module** - It expects its own initialization pattern
3. **Don't reorder HTML sections** - Moving Output section or changing structure breaks meter bindings
4. **Don't add keyboard JS before parameter bindings** - The keyboard may need access to `parameterStates`

---

## Standalone Module (Advanced)

The `js/instrument-footer-panel.js` file contains a full ES6 class if you need programmatic control. However, for most plugins, the surgical approach above is recommended.

```javascript
import { InstrumentFooterPanel } from './modules/instrument-footer-panel.js';

const footer = new InstrumentFooterPanel({
    container: document.getElementById('footer'),
    octaves: 2,
    startNote: 48,
    onNoteOn: (note, velocity) => console.log('Note on:', note),
    onNoteOff: (note) => console.log('Note off:', note)
});

await footer.initialize();
```

---

## Origin

Extracted from O-Lyrica v1.18.1 (February 2026).
Integration lessons learned from O-Bells v2.2.0 (February 2026).
