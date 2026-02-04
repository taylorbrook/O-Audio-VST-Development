# Instrument Footer Panel

Reusable sticky footer component for WebView-based VST instruments.

## Overview

This module provides a complete bottom panel containing:

| Left | Center | Right |
|------|--------|-------|
| Master Volume | Playable Keyboard | Branding |

The panel is fully responsive and adapts to different plugin widths.

## Quick Start

### 1. Include the CSS

```html
<link rel="stylesheet" href="modules/instrument-footer-panel.css">
```

Or include inline in your plugin's HTML:

```html
<style>
  /* Contents of instrument-footer-panel.css */
</style>
```

### 2. Add the container

```html
<div id="instrument-footer"></div>
```

### 3. Initialize JavaScript

```javascript
import { InstrumentFooterPanel } from './modules/instrument-footer-panel.js';

const footer = new InstrumentFooterPanel({
    container: document.getElementById('instrument-footer'),
    octaves: 2,
    startNote: 48,  // C3
    enableQwerty: true,
    onNoteOn: (note, velocity) => {
        console.log(`Note on: ${note} @ ${velocity}`);
    },
    onNoteOff: (note) => {
        console.log(`Note off: ${note}`);
    }
});

await footer.initialize();
```

## Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `container` | Element | required | DOM element to render into |
| `octaves` | number | 2 | Number of octaves (1-3) |
| `startNote` | number | 48 | First MIDI note (48 = C3) |
| `showKeyLabels` | boolean | true | Show note names on white keys |
| `enableQwerty` | boolean | true | Enable computer keyboard input |
| `showHelpText` | boolean | true | Show help text below keyboard |
| `velocity` | number | 0.8 | Default velocity (0-1) |
| `highlightColor` | string | "#8BC34A" | Active key color |
| `masterVolumeParamId` | string | "masterVolume" | Parameter ID for slider |
| `brandText` | string | "Ouaricon Audio" | Branding text (empty to hide) |
| `height` | number | 55 | Footer height in pixels |

## QWERTY Keyboard Mapping

When `enableQwerty` is true, the computer keyboard works as a piano:

```
Bottom Row: Z S X D C V G B H N J M
            C3  D3  E3 F3  G3  A3  B3

Top Row:    Q 2 W 3 E R 5 T 6 Y 7 U I 9 O 0 P
            C4  D4  E4F4  G4  A4  B4C5  D5  E5
```

## Theming

### CSS Variables

Override these to customize appearance:

```css
.instrument-footer {
    --footer-bg: rgba(245, 230, 211, 0.9);
    --footer-border: rgba(139, 115, 85, 0.5);
    --footer-text: #3C2F2F;
    --footer-text-muted: #8B7355;
    --key-white-bg: #FAF0E6;
    --key-black-bg: #3C2F2F;
    --key-active-bg: #8BC34A;
    --key-border: #8B7355;
}
```

### Dark Theme

A built-in dark theme is available:

```javascript
footer.setTheme('dark');
```

Or define custom themes:

```css
.instrument-footer.my-theme {
    --footer-bg: #1a1a2e;
    --key-white-bg: #16213e;
    --key-black-bg: #0f0f1a;
    --key-active-bg: #e94560;
}
```

## API Reference

### Methods

```javascript
// Set master volume (0-1)
footer.setMasterVolume(0.75);

// Highlight a key externally (from MIDI input)
footer.setKeyActive(60, true);   // C4 on
footer.setKeyActive(60, false);  // C4 off

// Release all notes
footer.allNotesOff();

// Set velocity for subsequent notes
footer.setVelocity(0.5);

// Get currently playing notes
const notes = footer.getPlayingNotes();  // [60, 64, 67]

// Apply theme
footer.setTheme('dark');
```

### Events/Callbacks

```javascript
const footer = new InstrumentFooterPanel({
    onNoteOn: (note, velocity) => {
        // Called when any key is pressed
    },
    onNoteOff: (note) => {
        // Called when any key is released
    },
    onMasterVolumeChange: (value) => {
        // Called when master slider moves
    }
});
```

## C++ Integration

The footer sends MIDI notes via the `sendMidiNote` native function. Register it in your PluginEditor:

```cpp
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options()
        .withNativeFunction("sendMidiNote",
            [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 3) {
                    int note = static_cast<int>(args[0]);
                    float velocity = static_cast<float>(args[1]);
                    bool isNoteOn = static_cast<bool>(args[2]);

                    if (isNoteOn) {
                        processorRef.noteOn(note, velocity);
                    } else {
                        processorRef.noteOff(note);
                    }
                }
                complete({});
            })
);
```

## Layout Tips

The footer positions itself absolutely at the bottom. Your main content area should account for it:

```css
.tab-content {
    position: absolute;
    top: 70px;           /* Below header/tabs */
    left: 0;
    width: 100%;
    height: calc(100% - 125px);  /* 70px top + 55px footer */
    overflow-y: auto;
}
```

## Origin

Extracted from O-Lyrica v1.18.1 (February 2026).
