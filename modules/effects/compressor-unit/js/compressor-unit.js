/*
   This file is part of the Ouaricon Audio compressor-unit module.
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
 * compressor-unit.js - Compact dynamics compressor module UI
 * Ouaricon Audio Module System v1.4.0
 *
 * v1.3.0: shared strip contract with analog-eq-unit 1.3.0 / reverb-unit 1.0.0 —
 * host face var(--font-serif); fixed 505px strip, 21px side padding, 76px
 * bypass; knob groups 48px + 16px gap (64px pitch) after a 6px lead-in, so
 * knob centres sit on the EQ band centres; AUTO + GR pinned right; 9px floor
 * (readouts and GR label were 7-8px); AA fills on #4E6839 / off #6B5040.
 * Bypassed: knob graphics and the GR strip fade (0.4); captions, readouts and
 * AUTO stay AA (AUTO takes the muted #3A2E24 / #B8A08C fill).
 *
 * v1.4.0: double-click a knob or its readout to type a value (Enter or
 * clicking away applies, Esc cancels); Alt/Option-click resets a knob to its
 * default inside one host gesture (was double-click, which wrote without one).
 *
 * See module CHANGELOG for version history.
 */

// Constants
const DRAG_SENSITIVITY = 150;
const KNOB_ROTATION_RANGE = 270;
const KNOB_ROTATION_OFFSET = -135;
const GR_METER_MAX_DB = 24;
const GR_METER_SEGMENTS = 8;

// ====================================================================
// Value entry (module-private; the suite's O-Prism attachValueEntry semantics).
// Self-contained on purpose: the unit imports nothing from its host page and
// uses its own input class, so the host's .knob-value-input rule never crosses.
// ====================================================================

// Scaled (C++ units) -> normalised [0,1], reading the live start/end/skew the
// WebSliderRelay pushes from C++ (never a JS range table). Clamps BEFORE the
// pow(): a value below `start` gives a negative base, and Math.pow(negative,
// fractional skew) is NaN, which would stick in the parameter.
function scaledToNorm(v, props) {
  const start = props.start, end = props.end;
  const skew = props.skew > 0 ? props.skew : 1;
  if (!(end > start)) return 0;
  const t = Math.min(1, Math.max(0, (v - start) / (end - start)));
  return skew === 1 ? t : Math.pow(t, skew);
}

// Opens an inline editor over a readout. formatFn(norm) renders the readout;
// formatFn.parse(text, currentText) returns a SCALED value or null to reject.
// Enter / blur / clicking away commits, Esc cancels, untouched text is never
// written back, and "-inf" / "off" land on the range start.
function attachValueEntry(valueEl, state, formatFn) {
  if (!valueEl || !state) return;
  if (typeof formatFn !== 'function' || typeof formatFn.parse !== 'function') return;
  if (valueEl.dataset.editing === '1') return;   // already open

  const currentText = valueEl.textContent;
  const prevPosition = valueEl.style.position;
  const prevColor = valueEl.style.color;
  // Read the ink before the readout goes transparent.
  const inkColor = getComputedStyle(valueEl).color;

  const input = document.createElement('input');
  input.type = 'text';
  input.className = 'comp-value-input';
  input.value = currentText;
  input.spellcheck = false;
  input.autocomplete = 'off';
  input.style.color = inkColor;

  // The readout keeps its text (transparent) so its box does not change size.
  valueEl.dataset.editing = '1';
  valueEl.style.position = 'relative';
  valueEl.style.color = 'transparent';
  valueEl.appendChild(input);
  input.focus();
  input.select();

  let closed = false;

  // Clicking away commits even where a dial's drag handler preventDefault()s
  // its mousedown: that suppresses the focus change that would blur this
  // field, so grabbing another dial used to leave it open with its readout
  // frozen. Capture phase on document, so it commits before any drag starts.
  const onOutsidePointer = (e) => { if (e.target !== input) input.blur(); };
  document.addEventListener('pointerdown', onOutsidePointer, true);

  const close = () => {
    if (closed) return;
    closed = true;
    document.removeEventListener('pointerdown', onOutsidePointer, true);
    delete valueEl.dataset.editing;
    input.remove();
    valueEl.style.position = prevPosition;
    valueEl.style.color = prevColor;
    // Re-render from live state (automation / preset may have moved it).
    valueEl.textContent = formatFn(state.getNormalisedValue());
  };

  const commit = () => {
    if (closed) return;
    if (input.value === currentText) { close(); return; }
    const props = state.properties || {};
    const scaled = /^\s*([-−]?\s*inf(inity)?|off)\s*(db)?\s*$/i.test(input.value)
      ? props.start
      : formatFn.parse(input.value, currentText);
    if (scaled !== null && scaled !== undefined && isFinite(scaled)) {
      // setNormalisedValue() snaps to the interval and clamps.
      state.sliderDragStarted();
      state.setNormalisedValue(scaledToNorm(scaled, props));
      state.sliderDragEnded();
    }
    close();
  };

  input.addEventListener('keydown', (e) => {
    e.stopPropagation();
    if (e.key === 'Enter') { e.preventDefault(); commit(); }
    else if (e.key === 'Escape') { e.preventDefault(); close(); }
  });
  input.addEventListener('blur', commit);
  // Never start a drag (or reach a document handler) from inside the field.
  input.addEventListener('pointerdown', (e) => e.stopPropagation());
  input.addEventListener('mousedown', (e) => e.stopPropagation());
  input.addEventListener('dblclick', (e) => e.stopPropagation());
  input.addEventListener('wheel', (e) => e.stopPropagation());
}

// First signed decimal in the text; U+2212 (typographic minus) counts as '-'.
const numOf = (t) => {
  const m = String(t).replace(/−/g, '-').match(/-?\d*\.?\d+/);
  if (!m) return null;
  const v = parseFloat(m[0]);
  return isFinite(v) ? v : null;
};
// Decibel / ratio readouts: the display already IS the scaled value
// ("-12", "-12 dB", "4", "4:1").
const parseDirect = (t) => numOf(t);
// Millisecond readouts: "10" or "10 ms" -> 10; an explicit seconds unit
// ("0.01 s") multiplies by 1000.
const parseMs = (t) => {
  const v = numOf(t);
  if (v === null) return null;
  const s = String(t);
  if (/ms/i.test(s)) return v;            // before /s/ - "ms" contains "s"
  return /s/i.test(s) ? v * 1000 : v;
};

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
      threshold: { min: -60.0, max: 0.0, unit: ' dB', format: (v) => v.toFixed(1), parse: parseDirect },
      ratio: { min: 1.0, max: 20.0, unit: ':1', format: (v) => v.toFixed(1), parse: parseDirect },
      attack: { min: 0.1, max: 100.0, unit: ' ms', format: (v) => v.toFixed(1), parse: parseMs },
      release: { min: 10.0, max: 1000.0, unit: ' ms', format: (v) => v.toFixed(0), parse: parseMs }
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
        box-sizing: border-box; width: 505px; height: 71px;
        display: flex; align-items: center;
        background: #221C14 linear-gradient(135deg, #2a2318 0%, #1a1510 100%);
        border: 1px solid #5C4033;
        border-radius: 6px;
        padding: 3px 21px;
        font-family: var(--font-serif, 'EB Garamond', Garamond, 'Times New Roman', serif);
        color: #E8D5B7;
        user-select: none;
      }
      .comp-row { display: flex; align-items: center; gap: 10px; width: 100%; }
      .comp-bypass-toggle {
        box-sizing: border-box; width: 76px; flex: none;
        background: #6B5040; border: 1px solid #4A3728; border-radius: 3px;
        padding: 4px 0; font-family: inherit; font-size: 11px; line-height: 13px;
        font-weight: 600; letter-spacing: 1.5px; color: #E8D5B7;
        cursor: pointer; transition: all 0.1s ease; white-space: nowrap;
      }
      .comp-bypass-toggle.active { background: #4E6839; border-color: #3C5C1A; color: #FFF; }
      .comp-bypass-toggle:hover { filter: brightness(1.1); }
      .comp-knobs-compact { display: flex; gap: 16px; padding-left: 6px; justify-content: flex-start; }
      .comp-knob-group { width: 48px; display: flex; flex-direction: column; align-items: center; gap: 2px; }
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
      .comp-label { font-size: 9px; line-height: 10px; color: #B8A08C; text-align: center; letter-spacing: 0.3px; white-space: nowrap; }
      .comp-value { font-size: 9px; line-height: 10px; color: #E8D5B7; text-align: center; white-space: nowrap; }
      .comp-toggle {
        flex: none; margin-left: auto;
        background: #6B5040; border: 1px solid #4A3728; border-radius: 3px;
        padding: 4px 8px; font-family: inherit; font-size: 9px; line-height: 11px;
        font-weight: 600; letter-spacing: 1px; color: #E8D5B7;
        cursor: pointer; transition: all 0.1s ease;
      }
      .comp-toggle.active { background: #4E6839; border-color: #3C5C1A; color: #FFF; }
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
      .comp-gr-label { font-size: 9px; line-height: 10px; color: #B8A08C; letter-spacing: 0.5px; }
      .comp-unit-compact.bypassed .comp-knobs-compact,
      .comp-unit-compact.bypassed .autogain-toggle { pointer-events: none; }
      .comp-unit-compact.bypassed .comp-knob-container,
      .comp-unit-compact.bypassed .comp-gr-meter { opacity: 0.4; }
      .comp-unit-compact.bypassed .autogain-toggle,
      .comp-unit-compact.bypassed .autogain-toggle.active { background: #3A2E24; border-color: #4A3728; color: #B8A08C; }
      /* Value-entry field, overlaid on the readout (which keeps its text,
         transparent), so nothing shifts. Form controls do not inherit fonts on
         their own, hence the two inherits. */
      .comp-value-input {
        position: absolute; left: 0; top: 0; width: 100%; height: 100%;
        box-sizing: border-box; padding: 0; margin: 0;
        border: none; border-bottom: 1px solid currentColor; border-radius: 0;
        background: transparent;
        font-family: inherit; font-size: inherit; line-height: normal;
        text-align: inherit; outline: none; -moz-appearance: textfield;
      }
      .comp-value-input::-webkit-outer-spin-button,
      .comp-value-input::-webkit-inner-spin-button { -webkit-appearance: none; margin: 0; }
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

      // Alt/Option-click resets to the default as one host gesture. CAPTURE
      // phase, registered before the drag listener, so it runs first and
      // cancels the drag outright (stopImmediatePropagation).
      knob.addEventListener('mousedown', (e) => {
        if (!e.altKey) return;
        e.preventDefault();
        e.stopImmediatePropagation();
        const def = this.defaults[paramName];
        if (typeof def !== 'number') return;
        sliderState.sliderDragStarted();
        sliderState.setNormalisedValue(def);
        sliderState.sliderDragEnded();
      }, true);

      knob.addEventListener('mousedown', (e) => {
        activeKnob = { name: paramName, state: sliderState };
        lastY = e.clientY;
        e.preventDefault();
      });

      // Double-click (knob or readout) opens inline value entry.
      const readout = document.getElementById(`${this.paramPrefix}${paramName}_value`);
      const formatFn = (norm) => this.formatValue(paramName, norm);
      formatFn.parse = this.params[paramName].parse;
      knob.addEventListener('dblclick', (e) => {
        if (e.altKey) return;
        e.preventDefault();
        attachValueEntry(readout, sliderState, formatFn);
      });
      if (readout) readout.addEventListener('dblclick', (e) => {
        if (e.altKey) return;
        e.preventDefault();
        attachValueEntry(readout, sliderState, formatFn);
      });

      this.updateKnobVisual(paramName, sliderState.getNormalisedValue());
    });
  }

  updateKnobVisual(paramName, normalized) {
    const rotation = (normalized * KNOB_ROTATION_RANGE) + KNOB_ROTATION_OFFSET;
    const indicator = document.getElementById(`${this.paramPrefix}${paramName}_indicator`);
    if (indicator) indicator.style.transform = `translateX(-50%) rotate(${rotation}deg)`;

    const valueDisplay = document.getElementById(`${this.paramPrefix}${paramName}_value`);
    // Skipped while a value-entry field is open over the readout; the field
    // re-renders it from live state when it closes.
    if (valueDisplay && valueDisplay.dataset.editing !== '1') {
      valueDisplay.textContent = this.formatValue(paramName, normalized);
    }
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
