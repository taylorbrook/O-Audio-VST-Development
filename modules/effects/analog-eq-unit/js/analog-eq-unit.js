/**
 * Analog EQ Unit - Compact WebView UI Component with Dual-Ring Knobs
 * Ouaricon Module System v1.2.0
 *
 * v1.2.0 Changes:
 * - Module dimensions: 10px shorter vertically, 100px wider
 * - Padding adjusted from 8px 10px to 3px 60px
 *
 * v1.1.0 Changes:
 * - Compact single-row layout (half height)
 * - EQ button as title/bypass toggle (left side)
 * - ANALOG toggle inline with bands (right side)
 * - Removed output knob and footer
 *
 * Features the signature dual-ring knob design:
 * - Outer ring: Frequency control with notches
 * - Inner dial: Gain control with seed pattern
 *
 * Usage:
 *   import { AnalogEQUnitUI } from './modules/analog-eq-unit.js';
 *
 *   const eq = new AnalogEQUnitUI({
 *     container: document.getElementById('eq-panel'),
 *     paramPrefix: 'eq_',
 *     getSliderState: getSliderState,
 *     getToggleState: getToggleState,
 *     getComboBoxState: getComboBoxState
 *   });
 *   eq.initialize();
 */

export class AnalogEQUnitUI {
    constructor(options = {}) {
        this.container = options.container;
        this.paramPrefix = options.paramPrefix || 'eq_';
        this.getSliderState = options.getSliderState;
        this.getToggleState = options.getToggleState;
        this.getComboBoxState = options.getComboBoxState;
        this.showMeter = options.showMeter ?? false;
        this.onReady = options.onReady || (() => {});

        // Internal state
        this.sliders = {};
        this.toggles = {};
        this.combos = {};

        // Drag state
        this.isDragging = false;
        this.currentState = null;
        this.currentParamName = null;
        this.lastY = 0;

        // Bound event handlers for cleanup
        this.boundOnDrag = (e) => this.onDrag(e);
        this.boundEndDrag = () => this.endDrag();

        // Knob geometry
        this.KNOB_RADIUS = 30;
        this.INNER_THRESHOLD = 0.60;

        // Default normalized values (center position)
        this.DEFAULT_VALUES = {
            lf_freq: 0.5, lf_gain: 0.5,
            lmf_freq: 0.5, lmf_gain: 0.5,
            hmf_freq: 0.5, hmf_gain: 0.5,
            hf_freq: 0.5, hf_gain: 0.5
        };

        // Parameter ranges for display
        this.paramRanges = {
            lf_freq: { format: v => Math.round(30 + v * 470) + ' Hz' },
            lf_gain: { format: v => ((-12 + v * 24).toFixed(1)) + ' dB' },
            lmf_freq: { format: v => Math.round(100 + v * 1900) + ' Hz' },
            lmf_gain: { format: v => ((-12 + v * 24).toFixed(1)) + ' dB' },
            hmf_freq: { format: v => Math.round(500 + v * 7500) + ' Hz' },
            hmf_gain: { format: v => ((-12 + v * 24).toFixed(1)) + ' dB' },
            hf_freq: { format: v => {
                const hz = 2000 + v * 18000;
                return hz >= 10000 ? (hz/1000).toFixed(1) + 'k' : Math.round(hz) + ' Hz';
            }},
            hf_gain: { format: v => ((-12 + v * 24).toFixed(1)) + ' dB' }
        };
    }

    initialize() {
        if (!this.container) {
            console.error('AnalogEQUnitUI: No container element provided');
            return;
        }

        this.render();
        this.injectStyles();
        this.bindParameters();
        this.attachEventListeners();
        this.onReady();
    }

    render() {
        // v1.1.0: Compact single-row layout
        // [EQ toggle] | [LF] [LMF] [HMF] [HF] | [ANALOG toggle]
        this.container.innerHTML = `
            <div class="eq-unit-compact">
                <div class="eq-row">
                    <button class="eq-bypass-toggle" data-param="enabled">EQ</button>
                    <div class="eq-bands-compact">
                        ${this.renderBand('lf', 'LF', false)}
                        ${this.renderBand('lmf', 'LMF', true)}
                        ${this.renderBand('hmf', 'HMF', true)}
                        ${this.renderBand('hf', 'HF', false)}
                    </div>
                    <button class="eq-toggle analog-toggle" data-param="analog">ANALOG</button>
                </div>
            </div>
        `;
    }

    renderBand(id, label, hasQ) {
        return `
            <div class="eq-band-compact" data-band="${id}">
                <button class="band-label-compact" data-param="${id}_on">${label}</button>
                <div class="dual-knob-wrapper-compact">
                    ${this.renderFreqNotches()}
                    <div class="dual-knob-container" data-param-outer="${id}_freq" data-param-inner="${id}_gain">
                        <div class="knob-outer" id="${id}_freq_knob">
                            <div class="knob-outer-indicator"></div>
                        </div>
                        <div class="knob-inner" id="${id}_gain_knob">
                            <div class="knob-pointer"></div>
                        </div>
                        <div class="value-tooltip" id="${id}_tooltip">-- / --</div>
                    </div>
                </div>
                ${hasQ ? `
                <div class="three-way-toggle-compact" data-param="${id}_q">
                    <div class="three-way-option" data-value="0">W</div>
                    <div class="three-way-option active" data-value="1">M</div>
                    <div class="three-way-option" data-value="2">T</div>
                </div>
                ` : '<div class="q-spacer-compact"></div>'}
            </div>
        `;
    }

    renderFreqNotches() {
        return `
            <svg class="freq-notches-compact" viewBox="0 0 60 60">
                <g stroke="#5C4033" stroke-width="1" fill="none">
                    <line x1="30" y1="4" x2="30" y2="7" transform="rotate(-135, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(-108, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(-81, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(-54, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(-27, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="7" transform="rotate(0, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(27, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(54, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(81, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(108, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="7" transform="rotate(135, 30, 30)"/>
                </g>
            </svg>
        `;
    }

    injectStyles() {
        if (document.getElementById('eq-unit-styles')) return;

        const style = document.createElement('style');
        style.id = 'eq-unit-styles';
        style.textContent = `
            /* v1.2.0: Compact single-row layout - wider, shorter */
            .eq-unit-compact {
                background: linear-gradient(135deg, #2a2318 0%, #1a1510 100%);
                border: 1px solid #5C4033;
                border-radius: 6px;
                padding: 3px 60px;
                font-family: Garamond, 'Times New Roman', serif;
                color: #E8D5B7;
                user-select: none;
            }

            .eq-row {
                display: flex;
                align-items: center;
                gap: 10px;
            }

            /* EQ bypass toggle (title + on/off) */
            .eq-bypass-toggle {
                background: #8B7355;
                border: 1px solid #5C4033;
                border-radius: 3px;
                padding: 4px 12px;
                font-family: Garamond, serif;
                font-size: 11px;
                font-weight: 600;
                letter-spacing: 2px;
                color: #DDD;
                cursor: pointer;
                transition: all 0.1s ease;
                min-width: 40px;
            }

            .eq-bypass-toggle.active {
                background: #6B8E4E;
                border-color: #3C5C1A;
                color: #FFF;
            }

            /* Analog toggle (inline with bands) */
            .analog-toggle {
                background: #8B7355;
                border: 1px solid #5C4033;
                border-radius: 3px;
                padding: 4px 8px;
                font-family: Garamond, serif;
                font-size: 9px;
                font-weight: 600;
                letter-spacing: 1px;
                color: #DDD;
                cursor: pointer;
                transition: all 0.1s ease;
            }

            .analog-toggle.active {
                background: #6B8E4E;
                border-color: #3C5C1A;
                color: #FFF;
            }

            /* Bands container */
            .eq-bands-compact {
                display: flex;
                gap: 4px;
                flex: 1;
                justify-content: center;
            }

            .eq-band-compact {
                display: flex;
                flex-direction: column;
                align-items: center;
                gap: 2px;
            }

            .eq-band-compact.disabled {
                opacity: 0.4;
            }

            .band-label-compact {
                background: #6B8E4E;
                border: 1px solid #3C5C1A;
                border-radius: 2px;
                padding: 1px 5px;
                font-family: Garamond, serif;
                font-size: 8px;
                font-weight: 600;
                letter-spacing: 0.5px;
                color: #FFF;
                cursor: pointer;
                transition: all 0.1s ease;
                white-space: nowrap;
            }

            .band-label-compact.inactive {
                background: #8B7355;
                border-color: #5C4033;
                color: #DDD;
            }

            /* Compact dual-ring knob wrapper */
            .dual-knob-wrapper-compact {
                position: relative;
                width: 60px;
                height: 60px;
                display: flex;
                align-items: center;
                justify-content: center;
            }

            .freq-notches-compact {
                position: absolute;
                width: 60px;
                height: 60px;
                pointer-events: none;
            }

            .dual-knob-container {
                position: absolute;
                width: 46px;
                height: 46px;
                cursor: pointer;
            }

            /* Outer ring - frequency */
            .knob-outer {
                position: absolute;
                width: 46px;
                height: 46px;
                border-radius: 50%;
                background: radial-gradient(circle,
                    transparent 62%,
                    rgba(107, 142, 78, 0.3) 62%,
                    rgba(107, 142, 78, 0.5) 72%,
                    rgba(139, 115, 85, 0.8) 82%,
                    #8B7355 88%
                );
                border: 2px solid #6B8E4E;
                transform-origin: center center;
                pointer-events: none;
            }

            .knob-outer-indicator {
                position: absolute;
                top: 2px;
                left: 50%;
                transform: translateX(-50%);
                width: 3px;
                height: 7px;
                background: #3C5C1A;
                border-radius: 1.5px;
                pointer-events: none;
            }

            /* Inner dial - gain (seed pattern) */
            .knob-inner {
                position: absolute;
                top: 9px;
                left: 9px;
                width: 28px;
                height: 28px;
                border-radius: 50%;
                background:
                    radial-gradient(circle, #FFF8DC 0%, #FFF8DC 18%, transparent 18%),
                    conic-gradient(from 0deg,
                        #F5DEB3 0deg, #F5DEB3 35deg, #8B7355 35deg, #8B7355 36deg,
                        #E8D5B7 36deg, #E8D5B7 71deg, #8B7355 71deg, #8B7355 72deg,
                        #F5DEB3 72deg, #F5DEB3 107deg, #8B7355 107deg, #8B7355 108deg,
                        #E8D5B7 108deg, #E8D5B7 143deg, #8B7355 143deg, #8B7355 144deg,
                        #F5DEB3 144deg, #F5DEB3 179deg, #8B7355 179deg, #8B7355 180deg,
                        #E8D5B7 180deg, #E8D5B7 215deg, #8B7355 215deg, #8B7355 216deg,
                        #F5DEB3 216deg, #F5DEB3 251deg, #8B7355 251deg, #8B7355 252deg,
                        #E8D5B7 252deg, #E8D5B7 287deg, #8B7355 287deg, #8B7355 288deg,
                        #F5DEB3 288deg, #F5DEB3 323deg, #8B7355 323deg, #8B7355 324deg,
                        #E8D5B7 324deg, #E8D5B7 359deg, #8B7355 359deg, #8B7355 360deg
                    );
                border: 2px solid #8B7355;
                transform-origin: center center;
                pointer-events: none;
            }

            .knob-pointer {
                position: absolute;
                top: 2px;
                left: 50%;
                transform: translateX(-50%);
                width: 2px;
                height: 7px;
                background: #3C2F2F;
                pointer-events: none;
            }

            /* Value tooltip */
            .value-tooltip {
                position: absolute;
                bottom: -16px;
                left: 50%;
                transform: translateX(-50%);
                background: rgba(60, 47, 47, 0.95);
                color: #FFF;
                padding: 2px 4px;
                border-radius: 2px;
                font-size: 7px;
                white-space: nowrap;
                pointer-events: none;
                opacity: 0;
                transition: opacity 0.15s;
                z-index: 10;
            }

            .dual-knob-container:hover .value-tooltip,
            .dual-knob-container.dragging .value-tooltip {
                opacity: 1;
            }

            /* Compact Q selector */
            .three-way-toggle-compact {
                display: flex;
                gap: 1px;
                background: #5C4033;
                border: 1px solid #5C4033;
                border-radius: 2px;
                overflow: hidden;
            }

            .three-way-toggle-compact .three-way-option {
                padding: 1px 4px;
                font-size: 7px;
                font-weight: 600;
                letter-spacing: 0.3px;
                color: #DDD;
                cursor: pointer;
                background: #8B7355;
                transition: background 0.1s;
            }

            .three-way-toggle-compact .three-way-option.active {
                background: #6B8E4E;
                color: #FFF;
            }

            .q-spacer-compact {
                height: 14px;
            }

            /* Bypassed state - dim entire module */
            .eq-unit-compact.bypassed .eq-bands-compact,
            .eq-unit-compact.bypassed .analog-toggle {
                opacity: 0.35;
                pointer-events: none;
            }
        `;
        document.head.appendChild(style);
    }

    bindParameters() {
        const prefix = this.paramPrefix;

        // Slider parameters (removed output_gain)
        const sliderParams = [
            'lf_freq', 'lf_gain',
            'lmf_freq', 'lmf_gain',
            'hmf_freq', 'hmf_gain',
            'hf_freq', 'hf_gain'
        ];

        sliderParams.forEach(param => {
            if (this.getSliderState) {
                const state = this.getSliderState(prefix + param);
                if (state) {
                    this.sliders[param] = state;
                }
            }
        });

        // Toggle parameters (added 'enabled')
        const toggleParams = ['lf_on', 'lmf_on', 'hmf_on', 'hf_on', 'analog', 'enabled'];

        toggleParams.forEach(param => {
            if (this.getToggleState) {
                const state = this.getToggleState(prefix + param);
                if (state) {
                    this.toggles[param] = state;
                }
            }
        });

        // ComboBox parameters
        const comboParams = ['lmf_q', 'hmf_q'];

        comboParams.forEach(param => {
            if (this.getComboBoxState) {
                const state = this.getComboBoxState(prefix + param);
                if (state) {
                    this.combos[param] = state;
                }
            }
        });
    }

    attachEventListeners() {
        // Setup dual-ring knobs for each band
        ['lf', 'lmf', 'hmf', 'hf'].forEach(band => {
            this.setupDualKnob(band);
        });

        // Setup band toggles
        this.container.querySelectorAll('.band-label-compact').forEach(btn => {
            const param = btn.dataset.param;
            btn.addEventListener('click', () => this.toggleBand(param));

            const state = this.toggles[param];
            if (state) {
                state.valueChangedEvent.addListener(() => this.updateBandToggle(param));
                this.updateBandToggle(param);
            }
        });

        // Setup EQ bypass toggle (new in v1.1.0)
        const eqBypassBtn = this.container.querySelector('.eq-bypass-toggle');
        if (eqBypassBtn) {
            eqBypassBtn.addEventListener('click', () => this.toggleEnabled());
            const state = this.toggles['enabled'];
            if (state) {
                state.valueChangedEvent.addListener(() => this.updateEnabledToggle());
                this.updateEnabledToggle();
            }
        }

        // Setup analog toggle
        const analogBtn = this.container.querySelector('.analog-toggle');
        if (analogBtn) {
            analogBtn.addEventListener('click', () => this.toggleAnalog());
            const state = this.toggles['analog'];
            if (state) {
                state.valueChangedEvent.addListener(() => this.updateAnalogToggle());
                this.updateAnalogToggle();
            }
        }

        // Setup Q selectors
        this.container.querySelectorAll('.three-way-toggle-compact').forEach(toggle => {
            this.setupQToggle(toggle);
        });

        // Global mouse events for drag
        document.addEventListener('mousemove', this.boundOnDrag);
        document.addEventListener('mouseup', this.boundEndDrag);
    }

    setupDualKnob(band) {
        const container = this.container.querySelector(`[data-param-outer="${band}_freq"]`);
        if (!container) return;

        const outerKnob = container.querySelector('.knob-outer');
        const innerKnob = container.querySelector('.knob-inner');
        const tooltip = this.container.querySelector(`#${band}_tooltip`);

        const freqState = this.sliders[`${band}_freq`];
        const gainState = this.sliders[`${band}_gain`];

        if (!freqState || !gainState) return;

        const updateTooltip = () => {
            const freqValue = this.paramRanges[`${band}_freq`].format(freqState.getNormalisedValue());
            const gainValue = this.paramRanges[`${band}_gain`].format(gainState.getNormalisedValue());
            tooltip.textContent = `${freqValue} / ${gainValue}`;
        };

        const updateKnobRotation = (element, normalizedValue) => {
            const degrees = -135 + (normalizedValue * 270);
            element.style.transform = `rotate(${degrees}deg)`;
        };

        // Mouse down - detect which layer
        container.addEventListener('mousedown', (e) => {
            const rect = container.getBoundingClientRect();
            const centerX = rect.left + rect.width / 2;
            const centerY = rect.top + rect.height / 2;

            const dx = e.clientX - centerX;
            const dy = e.clientY - centerY;
            const distance = Math.sqrt(dx * dx + dy * dy);
            const normalizedDistance = distance / (rect.width / 2);

            // Outer ring (freq) if > 60% from center, otherwise inner (gain)
            if (normalizedDistance > this.INNER_THRESHOLD) {
                this.currentState = freqState;
                this.currentParamName = `${band}_freq`;
            } else {
                this.currentState = gainState;
                this.currentParamName = `${band}_gain`;
            }

            this.isDragging = true;
            this.lastY = e.clientY;
            this.activeContainer = container;
            container.classList.add('dragging');

            if (this.currentState.sliderDragStarted) {
                this.currentState.sliderDragStarted();
            }

            e.preventDefault();
        });

        // Double-click to reset
        container.addEventListener('dblclick', (e) => {
            const rect = container.getBoundingClientRect();
            const centerX = rect.left + rect.width / 2;
            const centerY = rect.top + rect.height / 2;

            const dx = e.clientX - centerX;
            const dy = e.clientY - centerY;
            const distance = Math.sqrt(dx * dx + dy * dy);
            const normalizedDistance = distance / (rect.width / 2);

            if (normalizedDistance > this.INNER_THRESHOLD) {
                freqState.setNormalisedValue(this.DEFAULT_VALUES[`${band}_freq`]);
            } else {
                gainState.setNormalisedValue(this.DEFAULT_VALUES[`${band}_gain`]);
            }
            e.preventDefault();
        });

        // Listen for value changes
        freqState.valueChangedEvent.addListener(() => {
            updateKnobRotation(outerKnob, freqState.getNormalisedValue());
            updateTooltip();
        });

        gainState.valueChangedEvent.addListener(() => {
            updateKnobRotation(innerKnob, gainState.getNormalisedValue());
            updateTooltip();
        });

        // Initial state
        updateKnobRotation(outerKnob, freqState.getNormalisedValue());
        updateKnobRotation(innerKnob, gainState.getNormalisedValue());
        updateTooltip();
    }

    onDrag(e) {
        if (!this.isDragging || !this.currentState) return;

        const deltaY = this.lastY - e.clientY;
        const currentValue = this.currentState.getNormalisedValue();
        const newValue = Math.max(0, Math.min(1, currentValue + (deltaY * 0.005)));
        this.currentState.setNormalisedValue(newValue);
        this.lastY = e.clientY;
    }

    endDrag() {
        if (this.isDragging && this.currentState) {
            if (this.currentState.sliderDragEnded) {
                this.currentState.sliderDragEnded();
            }
            if (this.activeContainer) {
                this.activeContainer.classList.remove('dragging');
            }
        }
        this.isDragging = false;
        this.currentState = null;
        this.currentParamName = null;
        this.activeContainer = null;
    }

    toggleBand(param) {
        const state = this.toggles[param];
        if (state) {
            state.setValue(!state.getValue());
        }
    }

    updateBandToggle(param) {
        const state = this.toggles[param];
        if (!state) return;

        const isOn = state.getValue();
        const btn = this.container.querySelector(`[data-param="${param}"]`);
        const bandId = param.replace('_on', '');
        const band = this.container.querySelector(`[data-band="${bandId}"]`);

        if (btn) {
            btn.classList.toggle('inactive', !isOn);
        }
        if (band) {
            band.classList.toggle('disabled', !isOn);
        }
    }

    // v1.1.0: EQ bypass toggle
    toggleEnabled() {
        const state = this.toggles['enabled'];
        if (state) {
            state.setValue(!state.getValue());
        }
    }

    updateEnabledToggle() {
        const state = this.toggles['enabled'];
        if (!state) return;

        const isEnabled = state.getValue();
        const btn = this.container.querySelector('.eq-bypass-toggle');
        const unit = this.container.querySelector('.eq-unit-compact');

        if (btn) {
            btn.classList.toggle('active', isEnabled);
        }
        if (unit) {
            unit.classList.toggle('bypassed', !isEnabled);
        }
    }

    toggleAnalog() {
        const state = this.toggles['analog'];
        if (state) {
            state.setValue(!state.getValue());
        }
    }

    updateAnalogToggle() {
        const state = this.toggles['analog'];
        if (!state) return;

        const btn = this.container.querySelector('.analog-toggle');
        if (btn) {
            btn.classList.toggle('active', state.getValue());
        }
    }

    setupQToggle(container) {
        const param = container.dataset.param;
        const state = this.combos[param];
        if (!state) return;

        const options = container.querySelectorAll('.three-way-option');

        options.forEach((option) => {
            option.addEventListener('click', () => {
                const value = parseInt(option.dataset.value, 10);
                state.setChoiceIndex(value);
            });
        });

        const updateOptions = () => {
            const index = state.getChoiceIndex();
            options.forEach((opt) => {
                opt.classList.toggle('active', parseInt(opt.dataset.value, 10) === index);
            });
        };

        state.valueChangedEvent.addListener(updateOptions);
        updateOptions();
    }

    /**
     * Update VU meter (call from timer callback)
     * Note: VU meter removed in v1.1.0 compact layout
     */
    updateMeter(levelDB) {
        // No-op in compact layout (meter removed)
    }

    destroy() {
        document.removeEventListener('mousemove', this.boundOnDrag);
        document.removeEventListener('mouseup', this.boundEndDrag);
    }
}

export default AnalogEQUnitUI;
