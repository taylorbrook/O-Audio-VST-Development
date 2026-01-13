/**
 * Ouaricon Tuning Panel - JavaScript UI Module
 *
 * Complete tuning interface with mode selection, interval editing,
 * reference pitch control, and tonic transposition.
 *
 * Usage:
 *   import { TuningPanel } from './modules/tuning-panel.js';
 *
 *   const panel = new TuningPanel({
 *     container: document.getElementById('tuning-container'),
 *     pitchCircle: pitchCircleInstance,  // Optional PitchCircle component
 *     onTuningChanged: (intervals, name) => console.log('Tuning:', name)
 *   });
 *
 *   panel.initialize();
 */

// Note names for display
const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

// Built-in scale presets
const SCALE_PRESETS = {
    '12tet': {
        name: '12-TET Standard',
        intervals: [0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100]
    },
    'just': {
        name: 'Just Intonation',
        intervals: [0, 112, 204, 316, 386, 498, 590, 702, 814, 884, 996, 1088]
    },
    'pythagorean': {
        name: 'Pythagorean',
        intervals: [0, 90, 204, 294, 408, 498, 612, 702, 792, 906, 996, 1110]
    },
    'meantone': {
        name: 'Quarter-Comma Meantone',
        intervals: [0, 76, 193, 310, 386, 503, 579, 697, 773, 890, 1007, 1083]
    }
};

export class TuningPanel {
    constructor(options = {}) {
        this.container = options.container;
        this.pitchCircle = options.pitchCircle;
        this.onTuningChanged = options.onTuningChanged || (() => {});
        this.onModeChanged = options.onModeChanged || (() => {});

        // Current state
        this.currentMode = 0;  // 0 = 12-TET, 1 = Custom, 2 = MTS-ESP
        this.currentIntervals = [...SCALE_PRESETS['12tet'].intervals];
        this.currentScaleName = '12-TET Standard';
        this.currentTonic = 0;
        this.referencePitch = 440.0;

        // Native functions (set after JUCE initialization)
        this.nativeFunctions = {};

        this.isInitialized = false;
    }

    /**
     * Initialize the tuning panel.
     */
    async initialize() {
        if (this.isInitialized) return;

        await this._waitForNative();
        this._bindNativeFunctions();
        this._buildUI();
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
     * Bind native functions from JUCE.
     */
    _bindNativeFunctions() {
        const Juce = window.Juce || { getNativeFunction: (name) => window.__JUCE__.backend[name] };

        this.nativeFunctions = {
            loadScalaFile: Juce.getNativeFunction?.('loadScalaFile'),
            loadKBMFile: Juce.getNativeFunction?.('loadKBMFile'),
            saveScalaFile: Juce.getNativeFunction?.('saveScalaFile'),
            saveKBMFile: Juce.getNativeFunction?.('saveKBMFile'),
            setTuningIntervals: Juce.getNativeFunction?.('setTuningIntervals'),
            getTuningIntervals: Juce.getNativeFunction?.('getTuningIntervals'),
            setTonicNote: Juce.getNativeFunction?.('setTonicNote')
        };
    }

    /**
     * Build the panel UI.
     */
    _buildUI() {
        if (!this.container) return;

        this.container.innerHTML = `
            <div class="tuning-panel">
                <!-- Mode Buttons -->
                <div class="tuning-mode-section">
                    <div class="tuning-mode-buttons">
                        <button class="btn active" data-mode="0">12-TET</button>
                        <button class="btn" data-mode="1">CUSTOM</button>
                        <button class="btn" data-mode="2">MTS-ESP</button>
                    </div>
                </div>

                <!-- Scale Name Display -->
                <div class="scale-name-display">${this.currentScaleName}</div>

                <!-- Interval List -->
                <div class="interval-list"></div>

                <!-- File Buttons (Custom mode only) -->
                <div class="file-buttons" style="display: none;">
                    <button class="btn btn-small" data-action="load-scl">LOAD .SCL</button>
                    <button class="btn btn-small" data-action="load-kbm">LOAD .KBM</button>
                    <button class="btn btn-small" data-action="save-scl">SAVE .SCL</button>
                    <button class="btn btn-small" data-action="save-kbm">SAVE .KBM</button>
                </div>

                <!-- MTS Status (MTS mode only) -->
                <div class="mts-status" style="display: none;">
                    <div class="mts-indicator"></div>
                    <span>MTS-ESP: <span class="mts-status-text">Disconnected</span></span>
                </div>
            </div>
        `;

        this._updateIntervalList();
    }

    /**
     * Attach event listeners.
     */
    _attachEventListeners() {
        // Mode buttons
        this.container.querySelectorAll('[data-mode]').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const mode = parseInt(e.target.dataset.mode);
                this.setMode(mode);
            });
        });

        // File buttons
        this.container.querySelectorAll('[data-action]').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const action = e.target.dataset.action;
                this._handleFileAction(action);
            });
        });
    }

    /**
     * Set tuning mode.
     */
    setMode(mode) {
        this.currentMode = mode;

        // Update button states
        this.container.querySelectorAll('[data-mode]').forEach(btn => {
            btn.classList.toggle('active', parseInt(btn.dataset.mode) === mode);
        });

        // Show/hide mode-specific controls
        const fileButtons = this.container.querySelector('.file-buttons');
        const mtsStatus = this.container.querySelector('.mts-status');

        if (fileButtons) fileButtons.style.display = mode === 1 ? 'flex' : 'none';
        if (mtsStatus) mtsStatus.style.display = mode === 2 ? 'flex' : 'none';

        // Load appropriate preset
        if (mode === 0) {
            this.loadPreset('12tet');
        } else if (mode === 1) {
            this.loadPreset('just');
        }

        this._updateIntervalList();
        this.onModeChanged(mode);
    }

    /**
     * Load a built-in scale preset.
     */
    loadPreset(presetKey) {
        const preset = SCALE_PRESETS[presetKey];
        if (!preset) return;

        this.currentScaleName = preset.name;
        this.currentIntervals = [...preset.intervals];

        this._updateScaleNameDisplay();
        this._updateIntervalList();
        this._updatePitchCircle();
        this._applyTuning();
    }

    /**
     * Set intervals directly.
     */
    setIntervals(intervals, name = 'Custom') {
        this.currentIntervals = [0, ...intervals];  // Add unison
        this.currentScaleName = name;

        this._updateScaleNameDisplay();
        this._updateIntervalList();
        this._updatePitchCircle();
        this._applyTuning();
    }

    /**
     * Set tonic (transposition).
     */
    setTonic(tonicIndex) {
        this.currentTonic = tonicIndex % 12;

        // Update tonic display
        const tonicDisplay = this.container.querySelector('.tonic-value');
        if (tonicDisplay) {
            tonicDisplay.textContent = NOTE_NAMES[this.currentTonic];
        }

        this._updateIntervalList();
        this._updatePitchCircle();

        // Send to backend
        if (this.nativeFunctions.setTonicNote) {
            this.nativeFunctions.setTonicNote(this.currentTonic);
        }
    }

    /**
     * Update interval list UI.
     */
    _updateIntervalList() {
        const list = this.container.querySelector('.interval-list');
        if (!list) return;

        const total = this.currentIntervals.length;
        const isEditable = this.currentMode === 1;

        // Tonic selector (12-note scales only)
        let tonicHtml = '';
        if (total === 12) {
            tonicHtml = `
                <div class="tonic-selector">
                    <span class="tonic-arrow" data-dir="-1">◀</span>
                    <span class="tonic-label">Tonic:</span>
                    <span class="tonic-value">${NOTE_NAMES[this.currentTonic]}</span>
                    <span class="tonic-arrow" data-dir="1">▶</span>
                </div>
            `;
        }

        let html = `<div class="interval-list-header">Intervals (${total} notes)</div>${tonicHtml}`;

        for (let i = 0; i < total; i++) {
            const cents = this.currentIntervals[i] || 0;
            const isUnison = i === 0;
            const displayValue = isUnison ? '0' : cents.toFixed(1);
            const label = this._getDegreeLabel(i, total);
            const isDisabled = isUnison || !isEditable;

            html += `
                <div class="interval-item">
                    <span class="interval-degree">${label}</span>
                    <input type="text" class="interval-input" data-index="${i}"
                           value="${displayValue}" ${isDisabled ? 'disabled' : ''}>
                </div>
            `;
        }

        list.innerHTML = html;

        // Attach interval input handlers
        list.querySelectorAll('.interval-input:not([disabled])').forEach(input => {
            input.addEventListener('change', (e) => this._handleIntervalChange(e));
            input.addEventListener('keydown', (e) => {
                if (e.key === 'Enter') e.target.blur();
            });
        });

        // Attach tonic arrow handlers
        list.querySelectorAll('.tonic-arrow').forEach(arrow => {
            arrow.addEventListener('click', (e) => {
                const dir = parseInt(e.target.dataset.dir);
                this.setTonic((this.currentTonic + dir + 12) % 12);
            });
        });
    }

    /**
     * Get degree label for display.
     */
    _getDegreeLabel(index, total) {
        if (total === 12) {
            const noteIndex = (index + this.currentTonic) % 12;
            return NOTE_NAMES[noteIndex];
        }
        return String(index + 1);
    }

    /**
     * Handle interval input change.
     */
    _handleIntervalChange(e) {
        const index = parseInt(e.target.dataset.index);
        const parsed = this._parseIntervalInput(e.target.value);

        if (parsed !== null) {
            this.currentIntervals[index] = parsed;
            e.target.value = parsed.toFixed(1);
            this.currentScaleName = 'Custom';
            this._updateScaleNameDisplay();
            this._updatePitchCircle();
            this._applyTuning();
        } else {
            e.target.value = (this.currentIntervals[index] || 0).toFixed(1);
        }
    }

    /**
     * Parse interval input (cents or ratio).
     */
    _parseIntervalInput(value) {
        value = value.trim();
        if (!value) return null;

        if (value.includes('/')) {
            const parts = value.split('/');
            if (parts.length === 2) {
                const num = parseFloat(parts[0]);
                const den = parseFloat(parts[1]);
                if (!isNaN(num) && !isNaN(den) && den !== 0) {
                    return 1200 * Math.log2(num / den);
                }
            }
            return null;
        }

        const cents = parseFloat(value);
        return isNaN(cents) ? null : cents;
    }

    /**
     * Update scale name display.
     */
    _updateScaleNameDisplay() {
        const display = this.container.querySelector('.scale-name-display');
        if (display) {
            display.textContent = this.currentScaleName;
        }
    }

    /**
     * Update pitch circle visualization.
     */
    _updatePitchCircle() {
        if (this.pitchCircle) {
            this.pitchCircle.setIntervals(this.currentIntervals, this.currentTonic);
        }
    }

    /**
     * Apply tuning to backend.
     */
    _applyTuning() {
        const intervalsToSend = this.currentIntervals.slice(1);  // Exclude unison

        if (this.nativeFunctions.setTuningIntervals) {
            this.nativeFunctions.setTuningIntervals(intervalsToSend, this.currentScaleName);
        }

        this.onTuningChanged(this.currentIntervals, this.currentScaleName);
    }

    /**
     * Handle file button actions.
     */
    _handleFileAction(action) {
        switch (action) {
            case 'load-scl':
                this.nativeFunctions.loadScalaFile?.();
                break;
            case 'load-kbm':
                this.nativeFunctions.loadKBMFile?.();
                break;
            case 'save-scl':
                this.nativeFunctions.saveScalaFile?.();
                break;
            case 'save-kbm':
                this.nativeFunctions.saveKBMFile?.();
                break;
        }
    }

    /**
     * Callback when Scala file is loaded (called from C++).
     */
    onScalaLoaded(scaleName, intervals) {
        this.currentScaleName = scaleName;
        this.currentIntervals = intervals;

        this._updateScaleNameDisplay();
        this._updateIntervalList();
        this._updatePitchCircle();
    }

    /**
     * Get current state for preset saving.
     */
    getState() {
        return {
            mode: this.currentMode,
            intervals: [...this.currentIntervals],
            scaleName: this.currentScaleName,
            tonic: this.currentTonic,
            referencePitch: this.referencePitch
        };
    }

    /**
     * Restore state from preset.
     */
    setState(state) {
        if (state.mode !== undefined) this.setMode(state.mode);
        if (state.intervals) this.currentIntervals = [...state.intervals];
        if (state.scaleName) this.currentScaleName = state.scaleName;
        if (state.tonic !== undefined) this.setTonic(state.tonic);
        if (state.referencePitch) this.referencePitch = state.referencePitch;

        this._updateScaleNameDisplay();
        this._updateIntervalList();
        this._updatePitchCircle();
    }
}

// Export presets for external use
export { SCALE_PRESETS, NOTE_NAMES };

// Global export for non-module usage
if (typeof window !== 'undefined') {
    window.OuariconTuningPanel = TuningPanel;
    window.SCALE_PRESETS = SCALE_PRESETS;
}
