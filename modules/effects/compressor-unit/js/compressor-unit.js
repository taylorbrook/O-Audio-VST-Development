/**
 * compressor-unit.js
 * Compact dynamics compressor module UI
 *
 * Ouaricon Audio Module System v1.1.0
 *
 * v1.1.0 Changes:
 * - Compact single-row layout matching EQ module
 * - COMP button as title/bypass toggle (left side)
 * - AUTOGAIN toggle (right side, before GR meter)
 * - Centered knobs aligned with EQ bands above
 *
 * Features:
 * - 4 seed-style knobs (Threshold, Ratio, Attack, Release)
 * - Clickable COMP button for bypass toggle
 * - AUTOGAIN button for automatic makeup gain
 * - Vertical GR LED meter
 * - Naturalist aesthetic
 */

export class CompressorUnit {
  constructor(options = {}) {
    this.container = options.container;
    this.paramPrefix = options.paramPrefix || 'comp_';
    this.getSliderState = options.getSliderState;
    this.getToggleState = options.getToggleState;
    this.onGainReductionUpdate = options.onGainReductionUpdate || null;

    // State
    this.enabled = true;
    this.autogain = false;
    this.grValue = 0;
    this.animationId = null;

    // Parameter definitions
    this.params = {
      threshold: { min: -60.0, max: 0.0, unit: ' dB', format: (v) => v.toFixed(1) },
      ratio: { min: 1.0, max: 20.0, unit: ':1', format: (v) => v.toFixed(1) },
      attack: { min: 0.1, max: 100.0, unit: ' ms', format: (v) => v.toFixed(1) },
      release: { min: 10.0, max: 1000.0, unit: ' ms', format: (v) => v.toFixed(0) }
    };

    // Default normalized values for double-click reset
    this.defaults = {
      threshold: ((-20.0) - (-60.0)) / (0.0 - (-60.0)),  // -20 dB
      ratio: (2.0 - 1.0) / (20.0 - 1.0),                  // 2:1
      attack: (10.0 - 0.1) / (100.0 - 0.1),               // 10 ms
      release: (100.0 - 10.0) / (1000.0 - 10.0)           // 100 ms
    };
  }

  /**
   * Generate the HTML for the compressor unit
   */
  render() {
    if (!this.container) return;

    // v1.1.0: Compact single-row layout matching EQ module
    // [COMP toggle] | [4 centered knobs] | [AUTOGAIN toggle] | [GR meter]
    this.container.innerHTML = `
      <div class="comp-unit-compact">
        <div class="comp-row">
          <button class="comp-bypass-toggle" data-param="enabled">COMP</button>

          <div class="comp-knobs-compact">
            ${this.renderKnob('threshold', 'Thresh')}
            ${this.renderKnob('ratio', 'Ratio')}
            ${this.renderKnob('attack', 'Attack')}
            ${this.renderKnob('release', 'Release')}
          </div>

          <button class="comp-toggle autogain-toggle" data-param="autogain">AUTO</button>

          <div class="comp-meter-compact">
            <div class="comp-gr-meter" id="${this.paramPrefix}gr_meter">
              <div class="comp-gr-segment"></div>
              <div class="comp-gr-segment"></div>
              <div class="comp-gr-segment"></div>
              <div class="comp-gr-segment"></div>
              <div class="comp-gr-segment"></div>
              <div class="comp-gr-segment"></div>
              <div class="comp-gr-segment"></div>
              <div class="comp-gr-segment"></div>
            </div>
            <div class="comp-gr-label">GR</div>
          </div>
        </div>
      </div>
    `;

    this.injectStyles();
  }

  renderKnob(paramName, label) {
    return `
      <div class="comp-knob-group" data-param="${paramName}">
        <div class="comp-knob-container" id="${this.paramPrefix}${paramName}_knob">
          <div class="comp-seed-knob">
            <div class="comp-knob-indicator" id="${this.paramPrefix}${paramName}_indicator"></div>
          </div>
        </div>
        <div class="comp-label">${label}</div>
        <div class="comp-value" id="${this.paramPrefix}${paramName}_value">--</div>
      </div>
    `;
  }

  /**
   * Inject scoped CSS styles
   */
  injectStyles() {
    const styleId = 'compressor-unit-styles';
    if (document.getElementById(styleId)) return;

    const style = document.createElement('style');
    style.id = styleId;
    style.textContent = `
      /* v1.1.0: Compact single-row layout matching EQ module */
      .comp-unit-compact {
        background: linear-gradient(135deg, #2a2318 0%, #1a1510 100%);
        border: 1px solid #5C4033;
        border-radius: 6px;
        padding: 8px 10px;
        font-family: Garamond, 'Times New Roman', serif;
        color: #E8D5B7;
        user-select: none;
      }

      .comp-row {
        display: flex;
        align-items: center;
        gap: 10px;
      }

      /* COMP bypass toggle (matches EQ toggle style) */
      .comp-bypass-toggle {
        background: #8B7355;
        border: 1px solid #5C4033;
        border-radius: 3px;
        padding: 4px 10px;
        font-family: Garamond, serif;
        font-size: 11px;
        font-weight: 600;
        letter-spacing: 1.5px;
        color: #DDD;
        cursor: pointer;
        transition: all 0.1s ease;
        min-width: 50px;
      }

      .comp-bypass-toggle.active {
        background: #6B8E4E;
        border-color: #3C5C1A;
        color: #FFF;
      }

      .comp-bypass-toggle:hover {
        filter: brightness(1.1);
      }

      /* Knobs container - centered */
      .comp-knobs-compact {
        display: flex;
        gap: 16px;
        flex: 1;
        justify-content: center;
      }

      .comp-knob-group {
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 2px;
      }

      .comp-knob-container {
        width: 36px;
        height: 36px;
        cursor: pointer;
        position: relative;
      }

      .comp-seed-knob {
        width: 100%;
        height: 100%;
        border-radius: 50%;
        background: radial-gradient(circle at 30% 30%, #C9A27B, #8B7355);
        border: 2px solid #6B5847;
        position: relative;
        box-shadow:
          inset 0 2px 3px rgba(255, 255, 255, 0.3),
          inset 0 -2px 3px rgba(0, 0, 0, 0.3),
          0 2px 4px rgba(0, 0, 0, 0.2);
      }

      .comp-seed-knob::before {
        content: '';
        position: absolute;
        top: 50%;
        left: 50%;
        width: 6px;
        height: 6px;
        background: #4A3728;
        border-radius: 50%;
        transform: translate(-50%, -50%);
        box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.5);
      }

      .comp-seed-knob::after {
        content: '';
        position: absolute;
        top: 50%;
        left: 50%;
        width: 90%;
        height: 90%;
        transform: translate(-50%, -50%);
        border-radius: 50%;
        background:
          repeating-conic-gradient(
            from 0deg,
            transparent 0deg,
            transparent 15deg,
            rgba(107, 88, 71, 0.1) 15deg,
            rgba(107, 88, 71, 0.1) 18deg
          );
      }

      .comp-knob-indicator {
        position: absolute;
        top: 8%;
        left: 50%;
        width: 2px;
        height: 35%;
        background: #3C2F2F;
        transform-origin: bottom center;
        transform: translateX(-50%);
        border-radius: 1px;
      }

      .comp-label {
        font-size: 9px;
        color: #A89080;
        text-align: center;
        letter-spacing: 0.3px;
      }

      .comp-value {
        font-size: 8px;
        color: #E8D5B7;
        text-align: center;
        font-weight: normal;
        min-width: 45px;
      }

      /* AUTOGAIN toggle (matches EQ ANALOG toggle style) */
      .comp-toggle {
        background: #5C4033;
        border: 1px solid #4A3728;
        border-radius: 3px;
        padding: 4px 8px;
        font-family: Garamond, serif;
        font-size: 9px;
        font-weight: 600;
        letter-spacing: 1px;
        color: #A89080;
        cursor: pointer;
        transition: all 0.1s ease;
      }

      .comp-toggle.active {
        background: #6B8E4E;
        border-color: #3C5C1A;
        color: #FFF;
      }

      .comp-toggle:hover {
        filter: brightness(1.1);
      }

      /* GR Meter */
      .comp-meter-compact {
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 2px;
      }

      .comp-gr-meter {
        display: flex;
        flex-direction: column-reverse;
        gap: 1px;
        background: rgba(60, 47, 47, 0.3);
        border: 1px solid #5C4033;
        padding: 2px;
        border-radius: 2px;
      }

      .comp-gr-segment {
        width: 10px;
        height: 5px;
        background: #4A3728;
        border-radius: 1px;
        transition: background 0.08s ease;
      }

      .comp-gr-segment.active {
        background: #C9A27B;
        box-shadow: 0 0 4px rgba(201, 162, 123, 0.6);
      }

      .comp-gr-segment.active-high {
        background: #D4A574;
        box-shadow: 0 0 6px rgba(212, 165, 116, 0.8);
      }

      .comp-gr-label {
        font-size: 7px;
        color: #A89080;
        letter-spacing: 0.5px;
      }

      /* Disabled state (bypassed) */
      .comp-unit-compact.bypassed .comp-knobs-compact,
      .comp-unit-compact.bypassed .autogain-toggle,
      .comp-unit-compact.bypassed .comp-meter-compact {
        opacity: 0.4;
        pointer-events: none;
      }
    `;

    document.head.appendChild(style);
  }

  /**
   * Initialize parameter bindings with JUCE
   */
  initialize() {
    this.render();
    this.setupBypassToggle();
    this.setupAutogainToggle();
    this.setupKnobs();
    this.startMeterAnimation();
  }

  /**
   * Setup bypass toggle on COMP button click
   */
  setupBypassToggle() {
    const bypassBtn = this.container.querySelector('.comp-bypass-toggle');
    const unit = this.container.querySelector('.comp-unit-compact');

    const toggleState = this.getToggleState(`${this.paramPrefix}enabled`);
    if (!toggleState) {
      console.warn('[CompressorUnit] Could not get toggle state for enabled');
      return;
    }

    // Listen to changes from C++
    toggleState.valueChangedEvent.addListener(() => {
      this.enabled = toggleState.getValue();
      this.updateBypassVisual();
    });

    // Click handler
    bypassBtn.addEventListener('click', () => {
      toggleState.setValue(!toggleState.getValue());
    });

    // Initialize
    this.enabled = toggleState.getValue();
    this.updateBypassVisual();
  }

  updateBypassVisual() {
    const bypassBtn = this.container.querySelector('.comp-bypass-toggle');
    const unit = this.container.querySelector('.comp-unit-compact');

    if (this.enabled) {
      bypassBtn.classList.add('active');
      unit.classList.remove('bypassed');
    } else {
      bypassBtn.classList.remove('active');
      unit.classList.add('bypassed');
    }
  }

  /**
   * Setup autogain toggle
   */
  setupAutogainToggle() {
    const autogainBtn = this.container.querySelector('.autogain-toggle');

    const toggleState = this.getToggleState(`${this.paramPrefix}autogain`);
    if (!toggleState) {
      console.warn('[CompressorUnit] Could not get toggle state for autogain');
      return;
    }

    // Listen to changes from C++
    toggleState.valueChangedEvent.addListener(() => {
      this.autogain = toggleState.getValue();
      this.updateAutogainVisual();
    });

    // Click handler
    autogainBtn.addEventListener('click', () => {
      toggleState.setValue(!toggleState.getValue());
    });

    // Initialize
    this.autogain = toggleState.getValue();
    this.updateAutogainVisual();
  }

  updateAutogainVisual() {
    const autogainBtn = this.container.querySelector('.autogain-toggle');

    if (this.autogain) {
      autogainBtn.classList.add('active');
    } else {
      autogainBtn.classList.remove('active');
    }
  }

  /**
   * Setup knob interactions
   */
  setupKnobs() {
    const knobParams = ['threshold', 'ratio', 'attack', 'release'];

    knobParams.forEach(paramName => {
      const knobId = `${this.paramPrefix}${paramName}_knob`;
      const knob = document.getElementById(knobId);
      const sliderState = this.getSliderState(`${this.paramPrefix}${paramName}`);

      if (!sliderState) {
        console.warn(`[CompressorUnit] Could not get slider state for ${paramName}`);
        return;
      }

      // Listen to changes from C++
      sliderState.valueChangedEvent.addListener(() => {
        const normalized = sliderState.getNormalisedValue();
        this.updateKnobVisual(paramName, normalized);
      });

      // Mouse drag handler
      let isDragging = false;
      let lastY = 0;

      knob.addEventListener('mousedown', (e) => {
        isDragging = true;
        lastY = e.clientY;
        e.preventDefault();
      });

      document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;

        const deltaY = lastY - e.clientY;
        const currentNormalized = sliderState.getNormalisedValue();
        let newNormalized = currentNormalized + (deltaY / 150);
        newNormalized = Math.max(0, Math.min(1, newNormalized));

        sliderState.setNormalisedValue(newNormalized);
        lastY = e.clientY;
      });

      document.addEventListener('mouseup', () => {
        isDragging = false;
      });

      // Double-click reset
      knob.addEventListener('dblclick', () => {
        const defaultNorm = this.defaults[paramName] || 0.5;
        sliderState.setNormalisedValue(defaultNorm);
      });

      // Initialize visual
      this.updateKnobVisual(paramName, sliderState.getNormalisedValue());
    });
  }

  /**
   * Update knob rotation and value display
   */
  updateKnobVisual(paramName, normalized) {
    const rotation = (normalized * 270) - 135;
    const indicator = document.getElementById(`${this.paramPrefix}${paramName}_indicator`);
    if (indicator) {
      indicator.style.transform = `translateX(-50%) rotate(${rotation}deg)`;
    }

    const valueDisplay = document.getElementById(`${this.paramPrefix}${paramName}_value`);
    if (valueDisplay) {
      valueDisplay.textContent = this.formatValue(paramName, normalized);
    }
  }

  /**
   * Format parameter value for display
   */
  formatValue(paramName, normalized) {
    const param = this.params[paramName];
    if (!param) return '';

    const value = param.min + normalized * (param.max - param.min);
    return param.format(value) + param.unit;
  }

  /**
   * Update GR meter from external source
   * Call this from host plugin's meter polling
   */
  updateGainReduction(grDB) {
    this.grValue = grDB;
  }

  /**
   * Start meter animation loop
   */
  startMeterAnimation() {
    const meter = document.getElementById(`${this.paramPrefix}gr_meter`);
    if (!meter) return;

    const segments = meter.querySelectorAll('.comp-gr-segment');
    const numSegments = segments.length;

    const update = () => {
      // Map 0-24 dB GR to 0-1 range
      const normalized = Math.min(1, this.grValue / 24);
      const activeCount = Math.round(normalized * numSegments);

      segments.forEach((segment, index) => {
        segment.classList.remove('active', 'active-high');

        if (index < activeCount) {
          if (index >= numSegments - 2) {
            segment.classList.add('active-high');
          } else {
            segment.classList.add('active');
          }
        }
      });

      this.animationId = requestAnimationFrame(update);
    };

    update();
  }

  /**
   * Cleanup
   */
  destroy() {
    if (this.animationId) {
      cancelAnimationFrame(this.animationId);
    }
  }
}

// Global function for C++ to call for meter updates
window.updateCompressorGR = function(grDB) {
  if (window.compressorUnitInstance) {
    window.compressorUnitInstance.updateGainReduction(grDB);
  }
};
