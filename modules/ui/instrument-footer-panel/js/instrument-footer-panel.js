/*
   This file is part of the Ouaricon Audio instrument-footer-panel module.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/**
 * Ouaricon Instrument Footer Panel - JavaScript Module
 *
 * Complete sticky footer for VST instruments with:
 * - Master volume slider (left)
 * - Playable keyboard with QWERTY support (center)
 * - Branding text (right)
 *
 * Usage:
 *   import { InstrumentFooterPanel } from './modules/instrument-footer-panel.js';
 *
 *   const footer = new InstrumentFooterPanel({
 *     container: document.getElementById('footer'),
 *     octaves: 2,
 *     startNote: 48,  // C3
 *     masterVolumeParamId: 'masterVolume',
 *     onNoteOn: (note, velocity) => console.log('Note on:', note),
 *     onNoteOff: (note) => console.log('Note off:', note)
 *   });
 *
 *   await footer.initialize();
 */

// Note names for key labels
const NOTE_NAMES = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];

// QWERTY keyboard mapping
const QWERTY_MAP = {
    // Bottom row (C3-B3)
    'z': 48, 's': 49, 'x': 50, 'd': 51, 'c': 52,
    'v': 53, 'g': 54, 'b': 55, 'h': 56, 'n': 57, 'j': 58, 'm': 59,
    // Top row (C4-B4)
    'q': 60, '2': 61, 'w': 62, '3': 63, 'e': 64,
    'r': 65, '5': 66, 't': 67, '6': 68, 'y': 69, '7': 70, 'u': 71,
    // Extended (C5+)
    'i': 72, '9': 73, 'o': 74, '0': 75, 'p': 76
};

// Semitone offsets for white keys (C, D, E, F, G, A, B)
const WHITE_KEY_SEMITONES = [0, 2, 4, 5, 7, 9, 11];

// Black key positions (which white key they follow)
const BLACK_KEY_POSITIONS = [0, 1, 3, 4, 5];  // C#, D#, F#, G#, A#
const BLACK_KEY_SEMITONES = [1, 3, 6, 8, 10];


export class InstrumentFooterPanel {
    constructor(options = {}) {
        // Container
        this.container = options.container;

        // Keyboard config
        this.octaves = options.octaves || 2;
        this.startNote = options.startNote || 48;  // C3
        this.showKeyLabels = options.showKeyLabels !== false;
        this.enableQwerty = options.enableQwerty !== false;
        this.showHelpText = options.showHelpText !== false;
        this.velocity = options.velocity || 0.8;
        this.highlightColor = options.highlightColor || '#8BC34A';

        // Master volume config
        this.masterVolumeParamId = options.masterVolumeParamId || 'masterVolume';

        // Branding
        this.brandText = options.brandText !== undefined ? options.brandText : 'Ouaricon Audio';

        // Height
        this.height = options.height || 55;

        // Callbacks
        this.onNoteOn = options.onNoteOn || (() => {});
        this.onNoteOff = options.onNoteOff || (() => {});
        this.onMasterVolumeChange = options.onMasterVolumeChange || (() => {});

        // State
        this.playingKeys = new Set();
        this.keyElements = new Map();
        this.qwertyPressedKeys = new Set();

        // Native function (set after JUCE initialization)
        this.sendMidiNote = null;

        // DOM references
        this.footerEl = null;
        this.keyboardVizEl = null;
        this.masterSliderEl = null;
        this.masterValueEl = null;

        this.isInitialized = false;
    }

    /**
     * Initialize the footer panel.
     */
    async initialize() {
        if (this.isInitialized) return;

        await this._waitForNative();
        this._bindNativeFunction();
        this._buildDOM();
        this._attachEventListeners();

        if (this.enableQwerty) {
            this._setupQwertyInput();
        }

        this.isInitialized = true;
    }

    /**
     * Wait for JUCE native integration.
     */
    async _waitForNative() {
        return new Promise((resolve) => {
            const check = () => {
                if (window.__JUCE__ && window.__JUCE__.backend) {
                    resolve();
                } else {
                    setTimeout(check, 50);
                }
            };
            check();
        });
    }

    /**
     * Bind native MIDI function.
     */
    _bindNativeFunction() {
        if (window.Juce && window.Juce.getNativeFunction) {
            this.sendMidiNote = window.Juce.getNativeFunction('sendMidiNote');
        } else if (window.__JUCE__ && window.__JUCE__.backend) {
            this.sendMidiNote = (...args) => window.__JUCE__.backend.sendMidiNote(...args);
        }
    }

    /**
     * Build the footer DOM structure.
     */
    _buildDOM() {
        if (!this.container) return;

        this.container.innerHTML = '';

        // Create footer container
        this.footerEl = document.createElement('div');
        this.footerEl.className = 'instrument-footer';
        this.footerEl.style.height = `${this.height}px`;

        // === Master Volume (Left) ===
        const masterVolumeDiv = document.createElement('div');
        masterVolumeDiv.className = 'footer-master-volume';

        const masterLabel = document.createElement('span');
        masterLabel.className = 'footer-master-label';
        masterLabel.textContent = 'Master';

        this.masterSliderEl = document.createElement('input');
        this.masterSliderEl.type = 'range';
        this.masterSliderEl.id = this.masterVolumeParamId;
        this.masterSliderEl.className = 'footer-master-slider';
        this.masterSliderEl.min = '0';
        this.masterSliderEl.max = '1';
        this.masterSliderEl.step = '0.001';
        this.masterSliderEl.value = '0.909';  // ~0 dB

        this.masterValueEl = document.createElement('span');
        this.masterValueEl.className = 'footer-master-value';
        this.masterValueEl.id = `${this.masterVolumeParamId}Value`;
        this.masterValueEl.textContent = '0.0 dB';

        masterVolumeDiv.appendChild(masterLabel);
        masterVolumeDiv.appendChild(this.masterSliderEl);
        masterVolumeDiv.appendChild(this.masterValueEl);

        // === Keyboard (Center) ===
        const keyboardDiv = document.createElement('div');
        keyboardDiv.className = 'footer-keyboard';
        keyboardDiv.id = 'footer-keyboard';

        this.keyboardVizEl = document.createElement('div');
        this.keyboardVizEl.className = 'footer-keyboard-viz';
        this.keyboardVizEl.id = 'keyboard-viz';

        this._buildKeyboard();

        keyboardDiv.appendChild(this.keyboardVizEl);

        if (this.showHelpText) {
            const helpText = document.createElement('div');
            helpText.className = 'footer-keyboard-help';
            helpText.textContent = this.enableQwerty ? 'Click or use keyboard (Z-M, Q-P)' : 'Click to play';
            keyboardDiv.appendChild(helpText);
        }

        // === Branding (Right) ===
        let brandSpan = null;
        if (this.brandText) {
            brandSpan = document.createElement('span');
            brandSpan.className = 'footer-brand';
            brandSpan.textContent = this.brandText;
        }

        // Assemble footer
        this.footerEl.appendChild(masterVolumeDiv);
        this.footerEl.appendChild(keyboardDiv);
        if (brandSpan) {
            this.footerEl.appendChild(brandSpan);
        }

        this.container.appendChild(this.footerEl);

        // Apply highlight color CSS variable
        this.footerEl.style.setProperty('--key-active-bg', this.highlightColor);
    }

    /**
     * Build keyboard keys.
     */
    _buildKeyboard() {
        const whiteKeysPerOctave = 7;
        const totalWhiteKeys = this.octaves * whiteKeysPerOctave;

        // Build white keys
        for (let octave = 0; octave < this.octaves; octave++) {
            for (let i = 0; i < whiteKeysPerOctave; i++) {
                const semitone = WHITE_KEY_SEMITONES[i];
                const midiNote = this.startNote + octave * 12 + semitone;

                const key = document.createElement('div');
                key.className = 'white-key mapped';
                key.dataset.note = midiNote;

                if (this.showKeyLabels) {
                    const label = document.createElement('span');
                    // Show octave number on C keys
                    if (i === 0) {
                        const octaveNum = Math.floor(midiNote / 12) - 1;
                        label.textContent = `C${octaveNum}`;
                    } else {
                        label.textContent = NOTE_NAMES[i];
                    }
                    key.appendChild(label);
                }

                this.keyboardVizEl.appendChild(key);
                this.keyElements.set(midiNote, key);
            }
        }

        // Build black keys
        for (let octave = 0; octave < this.octaves; octave++) {
            for (let i = 0; i < BLACK_KEY_POSITIONS.length; i++) {
                const whiteKeyIndex = octave * whiteKeysPerOctave + BLACK_KEY_POSITIONS[i];
                const midiNote = this.startNote + octave * 12 + BLACK_KEY_SEMITONES[i];

                const key = document.createElement('div');
                key.className = 'black-key mapped';
                key.dataset.note = midiNote;

                // Position between white keys
                const leftPos = `calc(100% / ${totalWhiteKeys} * ${whiteKeyIndex + 1} - 7px)`;
                key.style.left = leftPos;

                this.keyboardVizEl.appendChild(key);
                this.keyElements.set(midiNote, key);
            }
        }
    }

    /**
     * Attach event listeners.
     */
    _attachEventListeners() {
        // Keyboard events
        this.keyElements.forEach((element, midiNote) => {
            element.addEventListener('mousedown', (e) => {
                e.preventDefault();
                this._noteOn(midiNote);
            });

            element.addEventListener('mouseup', () => {
                this._noteOff(midiNote);
            });

            element.addEventListener('mouseleave', () => {
                if (this.playingKeys.has(midiNote)) {
                    this._noteOff(midiNote);
                }
            });

            // Touch support
            element.addEventListener('touchstart', (e) => {
                e.preventDefault();
                this._noteOn(midiNote);
            });

            element.addEventListener('touchend', (e) => {
                e.preventDefault();
                this._noteOff(midiNote);
            });
        });

        // Global mouseup to catch releases outside keyboard
        document.addEventListener('mouseup', () => {
            this.playingKeys.forEach(note => this._noteOff(note));
        });

        // Master volume slider
        if (this.masterSliderEl) {
            this.masterSliderEl.addEventListener('input', (e) => {
                const value = parseFloat(e.target.value);
                this._updateMasterVolumeDisplay(value);
                this.onMasterVolumeChange(value);
            });
        }
    }

    /**
     * Setup QWERTY keyboard input.
     */
    _setupQwertyInput() {
        document.addEventListener('keydown', (e) => {
            if (e.repeat) return;
            if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') return;

            const note = this._qwertyToNote(e.key.toLowerCase());
            if (note !== null && !this.qwertyPressedKeys.has(e.key)) {
                this.qwertyPressedKeys.add(e.key);
                this._noteOn(note);
            }
        });

        document.addEventListener('keyup', (e) => {
            const note = this._qwertyToNote(e.key.toLowerCase());
            if (note !== null) {
                this.qwertyPressedKeys.delete(e.key);
                this._noteOff(note);
            }
        });
    }

    /**
     * Map QWERTY key to MIDI note, adjusted for startNote.
     */
    _qwertyToNote(key) {
        const baseNote = QWERTY_MAP[key];
        if (baseNote === undefined) return null;

        // Adjust mapping based on startNote offset from C3 (48)
        const offset = this.startNote - 48;
        const adjustedNote = baseNote + offset;

        // Check if note is within our keyboard range
        const maxNote = this.startNote + (this.octaves * 12) - 1;
        if (adjustedNote >= this.startNote && adjustedNote <= maxNote) {
            return adjustedNote;
        }

        return null;
    }

    /**
     * Handle note on.
     */
    _noteOn(midiNote) {
        if (this.playingKeys.has(midiNote)) return;

        this.playingKeys.add(midiNote);

        // Update visual
        const element = this.keyElements.get(midiNote);
        if (element) element.classList.add('pressed');

        // Send MIDI to plugin
        if (this.sendMidiNote) {
            this.sendMidiNote(midiNote, this.velocity, true);
        }

        // Callback
        this.onNoteOn(midiNote, this.velocity);
    }

    /**
     * Handle note off.
     */
    _noteOff(midiNote) {
        if (!this.playingKeys.has(midiNote)) return;

        this.playingKeys.delete(midiNote);

        // Update visual
        const element = this.keyElements.get(midiNote);
        if (element) element.classList.remove('pressed');

        // Send MIDI to plugin
        if (this.sendMidiNote) {
            this.sendMidiNote(midiNote, 0, false);
        }

        // Callback
        this.onNoteOff(midiNote);
    }

    /**
     * Update master volume display.
     */
    _updateMasterVolumeDisplay(value) {
        if (!this.masterValueEl) return;

        // Convert 0-1 to dB (assuming -60dB to +6dB range)
        let db;
        if (value <= 0) {
            db = -Infinity;
            this.masterValueEl.textContent = '-\u221EdB';
        } else {
            // Common master volume curve: value^2 gives natural feel
            // Map to -60dB to +6dB range (typical DAW master)
            db = 20 * Math.log10(value);
            this.masterValueEl.textContent = `${db.toFixed(1)} dB`;
        }
    }

    /**
     * Set master volume programmatically.
     */
    setMasterVolume(value) {
        if (this.masterSliderEl) {
            this.masterSliderEl.value = value;
            this._updateMasterVolumeDisplay(value);
        }
    }

    /**
     * Highlight a key externally (e.g., from MIDI input).
     */
    setKeyActive(midiNote, active) {
        const element = this.keyElements.get(midiNote);
        if (element) {
            if (active) {
                element.classList.add('pressed');
            } else {
                element.classList.remove('pressed');
            }
        }
    }

    /**
     * Release all notes.
     */
    allNotesOff() {
        this.playingKeys.forEach(note => this._noteOff(note));
        this.qwertyPressedKeys.clear();
    }

    /**
     * Set velocity for subsequent notes.
     */
    setVelocity(velocity) {
        this.velocity = Math.max(0, Math.min(1, velocity));
    }

    /**
     * Get currently playing notes.
     */
    getPlayingNotes() {
        return [...this.playingKeys];
    }

    /**
     * Apply a theme class.
     */
    setTheme(themeName) {
        if (this.footerEl) {
            // Remove existing theme classes
            this.footerEl.classList.remove('dark-theme', 'light-theme');

            if (themeName) {
                this.footerEl.classList.add(`${themeName}-theme`);
            }
        }
    }
}


// ============================================================================
// Factory Functions
// ============================================================================

/**
 * Create a standard 2-octave footer panel.
 */
export function createInstrumentFooter(containerId, options = {}) {
    const container = document.getElementById(containerId);
    if (!container) return null;

    const footer = new InstrumentFooterPanel({
        container,
        octaves: 2,
        startNote: 48,
        ...options
    });

    footer.initialize();
    return footer;
}


// ============================================================================
// Global Exports
// ============================================================================

if (typeof window !== 'undefined') {
    window.InstrumentFooterPanel = InstrumentFooterPanel;
    window.createInstrumentFooter = createInstrumentFooter;
}
