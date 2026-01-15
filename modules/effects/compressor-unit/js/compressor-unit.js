/**
 * compressor-unit.js - Compact dynamics compressor module UI
 * Ouaricon Audio Module System v1.2.0
 *
 * See module CHANGELOG for version history.
 */

// Constants
const DRAG_SENSITIVITY = 150;
const KNOB_ROTATION_RANGE = 270;
const KNOB_ROTATION_OFFSET = -135;
const GR_METER_MAX_DB = 24;
const GR_METER_SEGMENTS = 8;

export class CompressorUnit {
  constructor(options = {}) {
    this.container = options.container;
    this.paramPrefix = options.paramPrefix || 'comp_';
    this.getSliderState = options.getSliderState;
    this.getToggleState = options.getToggleState;

    this.enabled = true;
    this.autogain = false;
    this.grValue = 0;
    this.animationId = null;
    this.documentListeners = [];

    this.params = {
      threshold: { min: -60.0, max: 0.0, unit: ' dB', format: (v) => v.toFixed(1) },
      ratio: { min: 1.0, max: 20.0, unit: ':1', format: (v) => v.toFixed(1) },
      attack: { min: 0.1, max: 100.0, unit: ' ms', format: (v) => v.toFixed(1) },
      release: { min: 10.0, max: 1000.0, unit: ' ms', format: (v) => v.toFixed(0) }
    };

    this.defaults = {
      threshold: ((-20.0) - (-60.0)) / (0.0 - (-60.0)),  // -20 dB
      ratio: (2.0 - 1.0) / (20.0 - 1.0),                  // 2:1
      attack: (10.0 - 0.1) / (100.0 - 0.1),               // 10 ms
      release: (100.0 - 10.0) / (1000.0 - 10.0)           // 100 ms
    };
  }

  render() {
    if (!this.container) return;

    const segments = Array(GR_METER_SEGMENTS).fill('<div class="comp-gr-segment"></div>').join('');
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
            <div class="comp-gr-meter" id="${this.paramPrefix}gr_meter">${segments}</div>
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

  injectStyles() {
    const styleId = 'compressor-unit-styles';
    if (document.getElementById(styleId)) return;

    // Generate 10-segment conic gradient
    const segmentDeg = 36, dividerDeg = 1;
    const colors = ['#F5DEB3', '#E8D5B7'];
    const divider = '#8B7355';
    let conicStops = [];
    for (let i = 0; i < 10; i++) {
      const start = i * segmentDeg;
      const end = start + segmentDeg - dividerDeg;
      conicStops.push(`${colors[i % 2]} ${start}deg, ${colors[i % 2]} ${end}deg, ${divider} ${end}deg, ${divider} ${start + segmentDeg}deg`);
    }

    const style = document.createElement('style');
    style.id = styleId;
    style.textContent = `
      .comp-unit-compact {
        background: linear-gradient(135deg, #2a2318 0%, #1a1510 100%);
        border: 1px solid #5C4033;
        border-radius: 6px;
        padding: 3px 60px;
        font-family: Garamond, 'Times New Roman', serif;
        color: #E8D5B7;
        user-select: none;
      }
      .comp-row { display: flex; align-items: center; gap: 10px; }
      .comp-bypass-toggle {
        background: #8B7355; border: 1px solid #5C4033; border-radius: 3px;
        padding: 4px 10px; font-family: Garamond, serif; font-size: 11px;
        font-weight: 600; letter-spacing: 1.5px; color: #DDD;
        cursor: pointer; transition: all 0.1s ease; min-width: 50px;
      }
      .comp-bypass-toggle.active { background: #6B8E4E; border-color: #3C5C1A; color: #FFF; }
      .comp-bypass-toggle:hover { filter: brightness(1.1); }
      .comp-knobs-compact { display: flex; gap: 16px; flex: 1; justify-content: center; }
      .comp-knob-group { display: flex; flex-direction: column; align-items: center; gap: 2px; }
      .comp-knob-container { width: 40px; height: 40px; cursor: pointer; position: relative; }
      .comp-seed-knob {
        width: 100%; height: 100%; border-radius: 50%; border: 2px solid #8B7355;
        position: relative;
        background:
          radial-gradient(circle, transparent 88%, #C9A27B 88%, #C9A27B 92%, #8B7355 92%, #8B7355 94%, transparent 94%),
          conic-gradient(from 0deg, ${conicStops.join(', ')}),
          radial-gradient(circle, #FFF8DC 0%, #FFF8DC 20%, transparent 20%);
        box-shadow: inset 1px 1px 3px rgba(0,0,0,0.3), inset -1px -1px 2px rgba(255,248,220,0.5), 2px 2px 6px rgba(0,0,0,0.25);
      }
      .comp-seed-knob:hover { transform: scale(1.03); }
      .comp-knob-indicator {
        position: absolute; width: 5px; height: 5px; background: #3C2F2F;
        border-radius: 50%; top: 3px; left: 50%; transform: translateX(-50%) rotate(${KNOB_ROTATION_OFFSET}deg);
        transform-origin: center 17px;
      }
      .comp-label { font-size: 9px; color: #A89080; text-align: center; letter-spacing: 0.3px; }
      .comp-value { font-size: 8px; color: #E8D5B7; text-align: center; min-width: 45px; }
      .comp-toggle {
        background: #5C4033; border: 1px solid #4A3728; border-radius: 3px;
        padding: 4px 8px; font-family: Garamond, serif; font-size: 9px;
        font-weight: 600; letter-spacing: 1px; color: #A89080;
        cursor: pointer; transition: all 0.1s ease;
      }
      .comp-toggle.active { background: #6B8E4E; border-color: #3C5C1A; color: #FFF; }
      .comp-toggle:hover { filter: brightness(1.1); }
      .comp-meter-compact { display: flex; flex-direction: column; align-items: center; gap: 2px; }
      .comp-gr-meter {
        display: flex; flex-direction: column-reverse; gap: 1px;
        background: rgba(60, 47, 47, 0.3); border: 1px solid #5C4033;
        padding: 2px; border-radius: 2px;
      }
      .comp-gr-segment { width: 10px; height: 5px; background: #4A3728; border-radius: 1px; transition: background 0.08s ease; }
      .comp-gr-segment.active { background: #C9A27B; box-shadow: 0 0 4px rgba(201, 162, 123, 0.6); }
      .comp-gr-segment.active-high { background: #D4A574; box-shadow: 0 0 6px rgba(212, 165, 116, 0.8); }
      .comp-gr-label { font-size: 7px; color: #A89080; letter-spacing: 0.5px; }
      .comp-unit-compact.bypassed .comp-knobs-compact,
      .comp-unit-compact.bypassed .autogain-toggle,
      .comp-unit-compact.bypassed .comp-meter-compact { opacity: 0.4; pointer-events: none; }
    `;
    document.head.appendChild(style);
  }

  initialize() {
    this.render();
    this.setupBypassToggle();
    this.setupAutogainToggle();
    this.setupKnobs();
    this.startMeterAnimation();
  }

  setupToggle(selector, paramSuffix, stateKey, updateFn) {
    const btn = this.container.querySelector(selector);
    const toggleState = this.getToggleState(`${this.paramPrefix}${paramSuffix}`);

    if (!toggleState) {
      console.warn(`[CompressorUnit] Could not get toggle state for ${paramSuffix}`);
      return;
    }

    toggleState.valueChangedEvent.addListener(() => {
      this[stateKey] = toggleState.getValue();
      updateFn();
    });

    btn.addEventListener('click', () => {
      toggleState.setValue(!toggleState.getValue());
    });

    this[stateKey] = toggleState.getValue();
    updateFn();
  }

  setupBypassToggle() {
    this.setupToggle('.comp-bypass-toggle', 'enabled', 'enabled', () => this.updateBypassVisual());
  }

  updateBypassVisual() {
    const bypassBtn = this.container.querySelector('.comp-bypass-toggle');
    const unit = this.container.querySelector('.comp-unit-compact');
    bypassBtn.classList.toggle('active', this.enabled);
    unit.classList.toggle('bypassed', !this.enabled);
  }

  setupAutogainToggle() {
    this.setupToggle('.autogain-toggle', 'autogain', 'autogain', () => this.updateAutogainVisual());
  }

  updateAutogainVisual() {
    const autogainBtn = this.container.querySelector('.autogain-toggle');
    autogainBtn.classList.toggle('active', this.autogain);
  }

  setupKnobs() {
    const knobParams = ['threshold', 'ratio', 'attack', 'release'];
    let activeKnob = null;
    let lastY = 0;

    const onMouseMove = (e) => {
      if (!activeKnob) return;
      const deltaY = lastY - e.clientY;
      const currentNormalized = activeKnob.state.getNormalisedValue();
      const newNormalized = Math.max(0, Math.min(1, currentNormalized + deltaY / DRAG_SENSITIVITY));
      activeKnob.state.setNormalisedValue(newNormalized);
      lastY = e.clientY;
    };

    const onMouseUp = () => { activeKnob = null; };

    document.addEventListener('mousemove', onMouseMove);
    document.addEventListener('mouseup', onMouseUp);
    this.documentListeners.push({ event: 'mousemove', fn: onMouseMove });
    this.documentListeners.push({ event: 'mouseup', fn: onMouseUp });

    knobParams.forEach(paramName => {
      const knob = document.getElementById(`${this.paramPrefix}${paramName}_knob`);
      const sliderState = this.getSliderState(`${this.paramPrefix}${paramName}`);

      if (!sliderState) {
        console.warn(`[CompressorUnit] Could not get slider state for ${paramName}`);
        return;
      }

      sliderState.valueChangedEvent.addListener(() => {
        this.updateKnobVisual(paramName, sliderState.getNormalisedValue());
      });

      knob.addEventListener('mousedown', (e) => {
        activeKnob = { name: paramName, state: sliderState };
        lastY = e.clientY;
        e.preventDefault();
      });

      knob.addEventListener('dblclick', () => {
        sliderState.setNormalisedValue(this.defaults[paramName] || 0.5);
      });

      this.updateKnobVisual(paramName, sliderState.getNormalisedValue());
    });
  }

  updateKnobVisual(paramName, normalized) {
    const rotation = (normalized * KNOB_ROTATION_RANGE) + KNOB_ROTATION_OFFSET;
    const indicator = document.getElementById(`${this.paramPrefix}${paramName}_indicator`);
    if (indicator) indicator.style.transform = `translateX(-50%) rotate(${rotation}deg)`;

    const valueDisplay = document.getElementById(`${this.paramPrefix}${paramName}_value`);
    if (valueDisplay) valueDisplay.textContent = this.formatValue(paramName, normalized);
  }

  formatValue(paramName, normalized) {
    const param = this.params[paramName];
    if (!param) return '';

    const value = param.min + normalized * (param.max - param.min);
    return param.format(value) + param.unit;
  }

  updateGainReduction(grDB) {
    this.grValue = grDB;
  }

  startMeterAnimation() {
    const meter = document.getElementById(`${this.paramPrefix}gr_meter`);
    if (!meter) return;

    const segments = meter.querySelectorAll('.comp-gr-segment');

    const update = () => {
      const normalized = Math.min(1, this.grValue / GR_METER_MAX_DB);
      const activeCount = Math.round(normalized * GR_METER_SEGMENTS);

      segments.forEach((segment, index) => {
        segment.classList.remove('active', 'active-high');
        if (index < activeCount) {
          segment.classList.add(index >= GR_METER_SEGMENTS - 2 ? 'active-high' : 'active');
        }
      });
      this.animationId = requestAnimationFrame(update);
    };
    update();
  }

  destroy() {
    if (this.animationId) {
      cancelAnimationFrame(this.animationId);
    }
    this.documentListeners.forEach(({ event, fn }) => {
      document.removeEventListener(event, fn);
    });
    this.documentListeners = [];
  }
}

// Global function for C++ to call for meter updates
window.updateCompressorGR = function(grDB) {
  if (window.compressorUnitInstance) {
    window.compressorUnitInstance.updateGainReduction(grDB);
  }
};
