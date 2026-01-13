/**
 * Ouaricon Playable Keyboard - JavaScript UI Module
 *
 * Interactive piano keyboard that sends MIDI notes to the plugin.
 * Supports mouse, touch, and optional computer keyboard input.
 *
 * Usage:
 *   import { PlayableKeyboard } from './modules/playable-keyboard.js';
 *
 *   const keyboard = new PlayableKeyboard({
 *     container: document.getElementById('keyboard'),
 *     octaves: 1,
 *     startNote: 60,  // C4
 *     onNoteOn: (note, velocity) => console.log('Note on:', note),
 *     onNoteOff: (note) => console.log('Note off:', note)
 *   });
 *
 *   keyboard.initialize();
 */

// Note names for labels
const NOTE_NAMES = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
const ALL_NOTES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

// Black key positions within an octave (relative to white key indices)
// C#=0, D#=1, F#=3, G#=4, A#=5 (skipping E# and B#)
const BLACK_KEY_OFFSETS = [0, 1, 3, 4, 5];
const BLACK_KEY_SEMITONES = [1, 3, 6, 8, 10];  // Relative to C

export class PlayableKeyboard {
    constructor(options = {}) {
        this.container = options.container;
        this.octaves = options.octaves || 1;
        this.startNote = options.startNote || 60;  // C4
        this.showLabels = options.showLabels !== false;
        this.velocity = options.velocity || 0.8;
        this.highlightColor = options.highlightColor || '#8BC34A';

        // Callbacks
        this.onNoteOn = options.onNoteOn || (() => {});
        this.onNoteOff = options.onNoteOff || (() => {});

        // Optional integrations
        this.pitchCircle = options.pitchCircle;

        // Native function (set after JUCE initialization)
        this.sendMidiNote = null;

        // State
        this.playingKeys = new Set();
        this.keyElements = new Map();

        this.isInitialized = false;
    }

    /**
     * Initialize the keyboard.
     */
    async initialize() {
        if (this.isInitialized) return;

        await this._waitForNative();
        this._bindNativeFunction();
        this._buildKeyboard();
        this._attachEventListeners();

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
        // Try different ways to get the native function
        if (window.Juce && window.Juce.getNativeFunction) {
            this.sendMidiNote = window.Juce.getNativeFunction('sendMidiNote');
        } else if (window.__JUCE__ && window.__JUCE__.backend) {
            this.sendMidiNote = (...args) => window.__JUCE__.backend.sendMidiNote(...args);
        }
    }

    /**
     * Build keyboard DOM structure.
     */
    _buildKeyboard() {
        if (!this.container) return;

        this.container.innerHTML = '';
        this.container.classList.add('playable-keyboard');

        // Calculate dimensions
        const whiteKeysPerOctave = 7;
        const totalWhiteKeys = this.octaves * whiteKeysPerOctave;
        const keyWidth = 100 / totalWhiteKeys;  // Percentage

        // Create white keys first
        for (let octave = 0; octave < this.octaves; octave++) {
            for (let i = 0; i < whiteKeysPerOctave; i++) {
                const whiteKeyIndex = octave * whiteKeysPerOctave + i;
                const semitone = [0, 2, 4, 5, 7, 9, 11][i];  // C, D, E, F, G, A, B
                const midiNote = this.startNote + octave * 12 + semitone;

                const key = document.createElement('div');
                key.className = 'white-key';
                key.dataset.note = midiNote;
                key.style.width = `${keyWidth}%`;

                if (this.showLabels) {
                    const label = document.createElement('span');
                    label.textContent = NOTE_NAMES[i];
                    key.appendChild(label);
                }

                this.container.appendChild(key);
                this.keyElements.set(midiNote, key);
            }
        }

        // Create black keys (positioned absolutely)
        for (let octave = 0; octave < this.octaves; octave++) {
            for (let i = 0; i < BLACK_KEY_OFFSETS.length; i++) {
                const whiteKeyIndex = octave * whiteKeysPerOctave + BLACK_KEY_OFFSETS[i];
                const midiNote = this.startNote + octave * 12 + BLACK_KEY_SEMITONES[i];

                const key = document.createElement('div');
                key.className = 'black-key';
                key.dataset.note = midiNote;

                // Position between white keys
                const leftPos = (whiteKeyIndex + 1) * keyWidth - (keyWidth * 0.35 / 2);
                key.style.left = `${leftPos}%`;
                key.style.width = `${keyWidth * 0.6}%`;

                this.container.appendChild(key);
                this.keyElements.set(midiNote, key);
            }
        }

        // Apply default styles if not already styled
        this._applyDefaultStyles();
    }

    /**
     * Apply default CSS styles.
     */
    _applyDefaultStyles() {
        // Only apply if no existing styles
        if (this.container.querySelector('.white-key').offsetHeight > 0) return;

        const style = document.createElement('style');
        style.textContent = `
            .playable-keyboard {
                position: relative;
                display: flex;
                height: 70px;
                background: #FAF0E6;
                border: 1px solid #8B7355;
                border-radius: 4px;
                overflow: visible;
            }

            .playable-keyboard .white-key {
                flex: 1;
                background: #FAF0E6;
                border-right: 1px solid #8B7355;
                display: flex;
                flex-direction: column;
                justify-content: flex-end;
                align-items: center;
                padding-bottom: 4px;
                cursor: pointer;
                z-index: 1;
                transition: background 0.1s, transform 0.1s;
            }

            .playable-keyboard .white-key:last-of-type {
                border-right: none;
            }

            .playable-keyboard .white-key:hover:not(.playing) {
                background: #FFF8DC;
            }

            .playable-keyboard .white-key.playing {
                background: ${this.highlightColor} !important;
                transform: translateY(2px);
            }

            .playable-keyboard .white-key span {
                font-size: 8px;
                color: #3C2F2F;
                pointer-events: none;
            }

            .playable-keyboard .black-key {
                position: absolute;
                height: 60%;
                background: #3C2F2F;
                border-radius: 0 0 3px 3px;
                top: 0;
                z-index: 10;
                cursor: pointer;
                box-shadow: 0 2px 4px rgba(0,0,0,0.3);
                transition: background 0.1s, transform 0.1s;
            }

            .playable-keyboard .black-key:hover:not(.playing) {
                background: #5A3A3A;
            }

            .playable-keyboard .black-key.playing {
                background: ${this.highlightColor} !important;
                transform: translateY(2px);
                box-shadow: 0 1px 2px rgba(0,0,0,0.3);
            }
        `;

        document.head.appendChild(style);
    }

    /**
     * Attach event listeners.
     */
    _attachEventListeners() {
        this.keyElements.forEach((element, midiNote) => {
            // Mouse events
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

            // Touch events
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
    }

    /**
     * Handle note on.
     */
    _noteOn(midiNote) {
        if (this.playingKeys.has(midiNote)) return;

        this.playingKeys.add(midiNote);

        // Update visual
        const element = this.keyElements.get(midiNote);
        if (element) element.classList.add('playing');

        // Update pitch circle if connected
        if (this.pitchCircle) {
            this.pitchCircle.setNoteActive(midiNote, this.velocity);
        }

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
        if (element) element.classList.remove('playing');

        // Update pitch circle if connected
        if (this.pitchCircle) {
            this.pitchCircle.setNoteInactive(midiNote);
        }

        // Send MIDI to plugin
        if (this.sendMidiNote) {
            this.sendMidiNote(midiNote, 0, false);
        }

        // Callback
        this.onNoteOff(midiNote);
    }

    /**
     * Programmatically trigger a note (e.g., from computer keyboard).
     */
    triggerNote(midiNote, velocity = null, durationMs = 0) {
        const vel = velocity !== null ? velocity : this.velocity;
        this._noteOn(midiNote);

        if (durationMs > 0) {
            setTimeout(() => this._noteOff(midiNote), durationMs);
        }
    }

    /**
     * Release a programmatically triggered note.
     */
    releaseNote(midiNote) {
        this._noteOff(midiNote);
    }

    /**
     * Release all playing notes.
     */
    allNotesOff() {
        this.playingKeys.forEach(note => this._noteOff(note));
    }

    /**
     * Set velocity for subsequent notes.
     */
    setVelocity(velocity) {
        this.velocity = Math.max(0, Math.min(1, velocity));
    }

    /**
     * Connect to a pitch circle for note highlighting.
     */
    setPitchCircle(pitchCircle) {
        this.pitchCircle = pitchCircle;
    }

    /**
     * Get currently playing notes.
     */
    getPlayingNotes() {
        return [...this.playingKeys];
    }
}


// ============================================================================
// Computer Keyboard Mapping (Optional)
// ============================================================================

/**
 * Map computer keyboard to MIDI notes.
 * Bottom row: Z-M = C4-B4
 * Top row: Q-P = C5-E6 (extended)
 */
const QWERTY_MAP = {
    // Bottom row (C4-B4)
    'z': 60, 's': 61, 'x': 62, 'd': 63, 'c': 64,
    'v': 65, 'g': 66, 'b': 67, 'h': 68, 'n': 69, 'j': 70, 'm': 71,
    // Top row (C5+)
    'q': 72, '2': 73, 'w': 74, '3': 75, 'e': 76,
    'r': 77, '5': 78, 't': 79, '6': 80, 'y': 81, '7': 82, 'u': 83,
    'i': 84, '9': 85, 'o': 86, '0': 87, 'p': 88
};

/**
 * Enable computer keyboard input for a PlayableKeyboard.
 */
export function enableQwertyInput(keyboard) {
    const pressedKeys = new Set();

    document.addEventListener('keydown', (e) => {
        if (e.repeat) return;

        const note = QWERTY_MAP[e.key.toLowerCase()];
        if (note !== undefined && !pressedKeys.has(e.key)) {
            pressedKeys.add(e.key);
            keyboard.triggerNote(note);
        }
    });

    document.addEventListener('keyup', (e) => {
        const note = QWERTY_MAP[e.key.toLowerCase()];
        if (note !== undefined) {
            pressedKeys.delete(e.key);
            keyboard.releaseNote(note);
        }
    });

    return () => {
        // Return cleanup function
        pressedKeys.clear();
        keyboard.allNotesOff();
    };
}


// ============================================================================
// Factory Functions
// ============================================================================

/**
 * Create a simple one-octave keyboard.
 */
export function createSimpleKeyboard(containerId, options = {}) {
    const container = document.getElementById(containerId);
    if (!container) return null;

    const keyboard = new PlayableKeyboard({
        container,
        octaves: 1,
        startNote: 60,
        ...options
    });

    keyboard.initialize();
    return keyboard;
}

/**
 * Create a keyboard with pitch circle integration.
 */
export function createKeyboardWithCircle(keyboardId, circleId, options = {}) {
    const { PitchCircle } = window.OuariconPitchCircle ? { PitchCircle: window.OuariconPitchCircle } : {};

    const keyboardContainer = document.getElementById(keyboardId);
    const circleContainer = document.getElementById(circleId);

    if (!keyboardContainer) return null;

    let pitchCircle = null;
    if (circleContainer && PitchCircle) {
        pitchCircle = new PitchCircle({
            container: circleContainer,
            size: options.circleSize || 150
        });
    }

    const keyboard = new PlayableKeyboard({
        container: keyboardContainer,
        pitchCircle,
        ...options
    });

    keyboard.initialize();
    return { keyboard, pitchCircle };
}


// ============================================================================
// Global Exports
// ============================================================================

if (typeof window !== 'undefined') {
    window.OuariconPlayableKeyboard = PlayableKeyboard;
    window.createSimpleKeyboard = createSimpleKeyboard;
    window.enableQwertyInput = enableQwertyInput;
}
