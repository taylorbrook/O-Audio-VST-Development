/**
 * Analog EQ Unit - Compact WebView UI Component with Dual-Ring Knobs
 * Ouaricon Module System v1.0.0
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

        // Knob geometry
        this.KNOB_RADIUS = 30;
        this.INNER_THRESHOLD = 0.60;

        // Default normalized values (center position)
        this.DEFAULT_VALUES = {
            lf_freq: 0.5, lf_gain: 0.5,
            lmf_freq: 0.5, lmf_gain: 0.5,
            hmf_freq: 0.5, hmf_gain: 0.5,
            hf_freq: 0.5, hf_gain: 0.5,
            output_gain: 0.5
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
            hf_gain: { format: v => ((-12 + v * 24).toFixed(1)) + ' dB' },
            output_gain: { format: v => ((-12 + v * 24).toFixed(1)) + ' dB' }
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
        this.container.innerHTML = `
            <div class="eq-unit">
                <div class="eq-header">
                    <span class="eq-title">ANALOG EQ</span>
                    <button class="eq-toggle analog-toggle" data-param="analog">ANALOG</button>
                </div>
                <div class="eq-bands">
                    ${this.renderBand('lf', 'LF SHELF', false)}
                    ${this.renderBand('lmf', 'LMF', true)}
                    ${this.renderBand('hmf', 'HMF', true)}
                    ${this.renderBand('hf', 'HF SHELF', false)}
                </div>
                <div class="eq-footer">
                    <div class="eq-output">
                        <label>OUTPUT</label>
                        <div class="output-knob-wrapper">
                            <div class="output-knob" data-param="output_gain">
                                <div class="output-knob-indicator"></div>
                            </div>
                        </div>
                        <span class="eq-value" data-display="output_gain">0 dB</span>
                    </div>
                    ${this.showMeter ? this.renderMeter() : ''}
                </div>
            </div>
        `;
    }

    renderBand(id, label, hasQ) {
        return `
            <div class="eq-band" data-band="${id}">
                <button class="band-label" data-param="${id}_on">${label}</button>
                <div class="dual-knob-wrapper">
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
                <div class="three-way-toggle" data-param="${id}_q">
                    <div class="three-way-option" data-value="0">W</div>
                    <div class="three-way-option active" data-value="1">M</div>
                    <div class="three-way-option" data-value="2">T</div>
                </div>
                ` : '<div class="q-spacer"></div>'}
            </div>
        `;
    }

    renderFreqNotches() {
        return `
            <svg class="freq-notches" viewBox="0 0 70 70">
                <g stroke="#5C4033" stroke-width="1.2" fill="none">
                    <line x1="35" y1="4" x2="35" y2="8" transform="rotate(-135, 35, 35)"/>
                    <line x1="35" y1="4" x2="35" y2="7" transform="rotate(-108, 35, 35)"/>
                    <line x1="35" y1="4" x2="35" y2="7" transform="rotate(-81, 35, 35)"/>
                    <line x1="35" y1="4" x2="35" y2="7" transform="rotate(-54, 35, 35)"/>
                    <line x1="35" y1="4" x2="35" y2="7" transform="rotate(-27, 35, 35)"/>
                    <line x1="35" y1="4" x2="35" y2="8" transform="rotate(0, 35, 35)"/>
                    <line x1="35" y1="4" x2="35" y2="7" transform="rotate(27, 35, 35)"/>
                    <line x1="35" y1="4" x2="35" y2="7" transform="rotate(54, 35, 35)"/>
                    <line x1="35" y1="4" x2="35" y2="7" transform="rotate(81, 35, 35)"/>
                    <line x1="35" y1="4" x2="35" y2="7" transform="rotate(108, 35, 35)"/>
                    <line x1="35" y1="4" x2="35" y2="8" transform="rotate(135, 35, 35)"/>
                </g>
            </svg>
        `;
    }

    renderMeter() {
        return `
            <div class="eq-meter">
                <div class="vu-meter-needle" id="eq_vu_needle"></div>
                <div class="vu-meter-pivot"></div>
                <div class="vu-meter-label">LEVEL</div>
            </div>
        `;
    }

    injectStyles() {
        if (document.getElementById('eq-unit-styles')) return;

        const style = document.createElement('style');
        style.id = 'eq-unit-styles';
        style.textContent = `
            .eq-unit {
                background: linear-gradient(135deg, #2a2318 0%, #1a1510 100%);
                border: 1px solid #5C4033;
                border-radius: 6px;
                padding: 10px;
                font-family: Garamond, 'Times New Roman', serif;
                color: #E8D5B7;
                user-select: none;
                min-width: 420px;
            }

            .eq-header {
                display: flex;
                justify-content: space-between;
                align-items: center;
                margin-bottom: 8px;
                padding-bottom: 6px;
                border-bottom: 1px solid #5C4033;
            }

            .eq-title {
                font-size: 11px;
                font-weight: 600;
                letter-spacing: 2px;
                color: #8B7355;
            }

            .analog-toggle {
                background: #6B8E4E;
                border: 1px solid #3C5C1A;
                border-radius: 3px;
                padding: 3px 10px;
                font-family: Garamond, serif;
                font-size: 9px;
                font-weight: 600;
                letter-spacing: 1px;
                color: #FFF;
                cursor: pointer;
                transition: all 0.1s ease;
            }

            .analog-toggle:not(.active) {
                background: #8B7355;
                border-color: #5C4033;
                color: #DDD;
            }

            .eq-bands {
                display: flex;
                gap: 6px;
                margin-bottom: 10px;
            }

            .eq-band {
                flex: 1;
                display: flex;
                flex-direction: column;
                align-items: center;
                gap: 4px;
            }

            .eq-band.disabled {
                opacity: 0.4;
            }

            .band-label {
                background: #6B8E4E;
                border: 1px solid #3C5C1A;
                border-radius: 3px;
                padding: 2px 6px;
                font-family: Garamond, serif;
                font-size: 9px;
                font-weight: 600;
                letter-spacing: 1px;
                color: #FFF;
                cursor: pointer;
                transition: all 0.1s ease;
                white-space: nowrap;
            }

            .band-label.inactive {
                background: #8B7355;
                border-color: #5C4033;
                color: #DDD;
            }

            /* Dual-ring knob wrapper */
            .dual-knob-wrapper {
                position: relative;
                width: 70px;
                height: 70px;
                display: flex;
                align-items: center;
                justify-content: center;
            }

            .freq-notches {
                position: absolute;
                width: 70px;
                height: 70px;
                pointer-events: none;
            }

            .dual-knob-container {
                position: absolute;
                width: 54px;
                height: 54px;
                cursor: pointer;
            }

            /* Outer ring - frequency */
            .knob-outer {
                position: absolute;
                width: 54px;
                height: 54px;
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
                height: 8px;
                background: #3C5C1A;
                border-radius: 1.5px;
                pointer-events: none;
            }

            /* Inner dial - gain (seed pattern) */
            .knob-inner {
                position: absolute;
                top: 10px;
                left: 10px;
                width: 34px;
                height: 34px;
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
                top: 3px;
                left: 50%;
                transform: translateX(-50%);
                width: 2px;
                height: 8px;
                background: #3C2F2F;
                pointer-events: none;
            }

            /* Value tooltip */
            .value-tooltip {
                position: absolute;
                bottom: -18px;
                left: 50%;
                transform: translateX(-50%);
                background: rgba(60, 47, 47, 0.95);
                color: #FFF;
                padding: 2px 5px;
                border-radius: 2px;
                font-size: 8px;
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

            /* Q selector */
            .three-way-toggle {
                display: flex;
                gap: 1px;
                background: #5C4033;
                border: 1px solid #5C4033;
                border-radius: 2px;
                overflow: hidden;
            }

            .three-way-option {
                padding: 2px 6px;
                font-size: 8px;
                font-weight: 600;
                letter-spacing: 0.5px;
                color: #DDD;
                cursor: pointer;
                background: #8B7355;
                transition: background 0.1s;
            }

            .three-way-option.active {
                background: #6B8E4E;
                color: #FFF;
            }

            .q-spacer {
                height: 18px;
            }

            /* Footer */
            .eq-footer {
                display: flex;
                justify-content: center;
                align-items: center;
                gap: 20px;
                padding-top: 8px;
                border-top: 1px solid #5C4033;
            }

            .eq-output {
                display: flex;
                flex-direction: column;
                align-items: center;
                gap: 2px;
            }

            .eq-output label {
                font-size: 8px;
                letter-spacing: 1px;
                color: #8B7355;
            }

            .output-knob-wrapper {
                width: 36px;
                height: 36px;
                display: flex;
                align-items: center;
                justify-content: center;
            }

            .output-knob {
                width: 28px;
                height: 28px;
                border-radius: 50%;
                background: radial-gradient(circle at 30% 30%, #5a5040, #3a3028);
                border: 2px solid #6B8E4E;
                position: relative;
                cursor: pointer;
            }

            .output-knob-indicator {
                position: absolute;
                top: 2px;
                left: 50%;
                transform: translateX(-50%);
                width: 3px;
                height: 6px;
                background: #6B8E4E;
                border-radius: 1.5px;
            }

            .eq-value {
                font-size: 9px;
                color: #E8D5B7;
                font-variant-numeric: tabular-nums;
            }

            /* VU Meter */
            .eq-meter {
                width: 60px;
                height: 60px;
                background: rgba(139, 168, 112, 0.15);
                border: 2px solid #3C5C1A;
                border-radius: 50%;
                position: relative;
                overflow: hidden;
            }

            .vu-meter-needle {
                position: absolute;
                width: 2px;
                height: 24px;
                background: #3C5C1A;
                bottom: 50%;
                left: 50%;
                transform-origin: bottom center;
                transform: translateX(-50%) rotate(-45deg);
                border-radius: 1px;
                transition: transform 0.12s ease-out;
            }

            .vu-meter-pivot {
                position: absolute;
                width: 8px;
                height: 8px;
                background: #3C2F2F;
                border-radius: 50%;
                bottom: calc(50% - 4px);
                left: calc(50% - 4px);
            }

            .vu-meter-label {
                position: absolute;
                bottom: 6px;
                width: 100%;
                text-align: center;
                font-size: 7px;
                color: #3C2F2F;
                letter-spacing: 0.5px;
            }
        `;
        document.head.appendChild(style);
    }

    bindParameters() {
        const prefix = this.paramPrefix;

        // Slider parameters
        const sliderParams = [
            'lf_freq', 'lf_gain',
            'lmf_freq', 'lmf_gain',
            'hmf_freq', 'hmf_gain',
            'hf_freq', 'hf_gain',
            'output_gain'
        ];

        sliderParams.forEach(param => {
            if (this.getSliderState) {
                const state = this.getSliderState(prefix + param);
                if (state) {
                    this.sliders[param] = state;
                }
            }
        });

        // Toggle parameters
        const toggleParams = ['lf_on', 'lmf_on', 'hmf_on', 'hf_on', 'analog'];

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

        // Setup output knob (simple single knob)
        this.setupOutputKnob();

        // Setup band toggles
        this.container.querySelectorAll('.band-label').forEach(btn => {
            const param = btn.dataset.param;
            btn.addEventListener('click', () => this.toggleBand(param));

            const state = this.toggles[param];
            if (state) {
                state.valueChangedEvent.addListener(() => this.updateBandToggle(param));
                this.updateBandToggle(param);
            }
        });

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
        this.container.querySelectorAll('.three-way-toggle').forEach(toggle => {
            this.setupQToggle(toggle);
        });

        // Global mouse events for drag
        document.addEventListener('mousemove', (e) => this.onDrag(e));
        document.addEventListener('mouseup', () => this.endDrag());
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

    setupOutputKnob() {
        const knob = this.container.querySelector('.output-knob');
        if (!knob) return;

        const state = this.sliders['output_gain'];
        if (!state) return;

        const indicator = knob.querySelector('.output-knob-indicator');
        const display = this.container.querySelector('[data-display="output_gain"]');

        const updateKnob = () => {
            const normalizedValue = state.getNormalisedValue();
            const degrees = -135 + (normalizedValue * 270);
            knob.style.transform = `rotate(${degrees}deg)`;

            if (display) {
                display.textContent = this.paramRanges['output_gain'].format(normalizedValue);
            }
        };

        knob.addEventListener('mousedown', (e) => {
            this.isDragging = true;
            this.currentState = state;
            this.currentParamName = 'output_gain';
            this.lastY = e.clientY;
            this.activeContainer = knob;

            if (state.sliderDragStarted) {
                state.sliderDragStarted();
            }

            e.preventDefault();
        });

        knob.addEventListener('dblclick', (e) => {
            state.setNormalisedValue(this.DEFAULT_VALUES['output_gain']);
            e.preventDefault();
        });

        state.valueChangedEvent.addListener(updateKnob);
        updateKnob();
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
            options.forEach((opt, i) => {
                opt.classList.toggle('active', parseInt(opt.dataset.value, 10) === index);
            });
        };

        state.valueChangedEvent.addListener(updateOptions);
        updateOptions();
    }

    /**
     * Update VU meter (call from timer callback)
     */
    updateMeter(levelDB) {
        if (!this.showMeter) return;

        const needle = this.container.querySelector('#eq_vu_needle');
        if (!needle) return;

        const minDB = -60;
        const maxDB = 3;
        const clampedDB = Math.max(minDB, Math.min(maxDB, levelDB));
        const normalized = (clampedDB - minDB) / (maxDB - minDB);
        const angle = -90 + (normalized * 180);

        needle.style.transform = `translateX(-50%) rotate(${angle}deg)`;

        // Color based on level
        const r = Math.round(60 + normalized * 195);
        const g = Math.round(92 - normalized * 42);
        const b = Math.round(26);
        needle.style.background = `rgb(${r}, ${g}, ${b})`;
    }

    destroy() {
        document.removeEventListener('mousemove', (e) => this.onDrag(e));
        document.removeEventListener('mouseup', () => this.endDrag());
    }
}

export default AnalogEQUnitUI;
