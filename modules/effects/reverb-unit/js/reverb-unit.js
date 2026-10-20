/*
   This file is part of the Ouaricon Audio reverb-unit module.
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
 * reverb-unit.js - Compact FDN plate reverb module UI
 * Ouaricon Audio Module System v1.1.0
 *
 * Same strip language as compressor-unit (dark rack panel, clickable bypass
 * title, 10-segment seed knobs), sized to sit under it: 505 x 71 px.
 *
 * Differences from compressor-unit, on purpose:
 *  - Text inherits the host page's face: `var(--font-serif, ...)`.
 *  - No text below 9px.
 *  - Captions are stable hooks for host localization: the title is
 *    `.rev-bypass-toggle`, each caption `[data-param="<name>"] .rev-label`.
 *    A host keys them with literal `el.dataset.i18n = '...'` assignments, the
 *    shape its i18n gate can read; English is rendered as the fallback.
 *
 * Gestures (v1.1.0): drag a knob vertically; double-click a knob or its readout
 * to type a value (Enter or clicking away applies, Esc cancels); Alt/Option-click
 * resets a knob to its default inside one host gesture.
 */

const DRAG_SENSITIVITY = 150;
const KNOB_ROTATION_RANGE = 270;
const KNOB_ROTATION_OFFSET = -135;

const pct = (v) => `${Math.round(v * 100)}%`;

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
  input.className = 'rev-value-input';
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
// Percent readouts on 0..1 params: "30" or "30%" -> 0.30.
const parsePercent = (t) => {
  const v = numOf(t);
  return v === null ? null : v / 100;
};
// Millisecond readouts: "50" or "50 ms" -> 50; an explicit seconds unit
// ("0.05 s") multiplies by 1000.
const parseMs = (t) => {
  const v = numOf(t);
  if (v === null) return null;
  const s = String(t);
  if (/ms/i.test(s)) return v;            // before /s/ - "ms" contains "s"
  return /s/i.test(s) ? v * 1000 : v;
};

export class ReverbUnit {
  constructor(options = {}) {
    this.container = options.container;
    this.paramPrefix = options.paramPrefix || 'rev_';
    this.getSliderState = options.getSliderState;
    this.getToggleState = options.getToggleState;

    this.enabled = false;
    this.documentListeners = [];

    // Ranges mirror ReverbUnit::addParameters (all linear).
    this.params = {
      size:     { min: 0, max: 1,   label: 'Size',    format: pct, parse: parsePercent },
      damp:     { min: 0, max: 1,   label: 'Damp',    format: pct, parse: parsePercent },
      predelay: { min: 0, max: 200, label: 'Pre-dly', format: (v) => `${Math.round(v)} ms`, parse: parseMs },
      mod:      { min: 0, max: 1,   label: 'Mod',     format: pct, parse: parsePercent },
      shimmer:  { min: 0, max: 1,   label: 'Shimmer', format: pct, parse: parsePercent },
      mix:      { min: 0, max: 1,   label: 'Mix',     format: pct, parse: parsePercent }
    };

    this.defaults = { size: 0.5, damp: 0.5, predelay: 20 / 200, mod: 0.2, shimmer: 0, mix: 0.25 };
  }

  render() {
    if (!this.container) return;

    const knobs = Object.keys(this.params).map((p) => this.renderKnob(p)).join('');
    this.container.innerHTML = `
      <div class="rev-unit-compact">
        <div class="rev-row">
          <button class="rev-bypass-toggle" data-param="enabled">REVERB</button>
          <div class="rev-knobs-compact">${knobs}</div>
        </div>
      </div>
    `;
    this.injectStyles();
  }

  renderKnob(paramName) {
    return `
      <div class="rev-knob-group" data-param="${paramName}">
        <div class="rev-knob-container" id="${this.paramPrefix}${paramName}_knob">
          <div class="rev-seed-knob">
            <div class="rev-knob-indicator" id="${this.paramPrefix}${paramName}_indicator"></div>
          </div>
        </div>
        <div class="rev-label">${this.params[paramName].label}</div>
        <div class="rev-value" id="${this.paramPrefix}${paramName}_value">--</div>
      </div>
    `;
  }

  injectStyles() {
    const styleId = 'reverb-unit-styles';
    if (document.getElementById(styleId)) return;

    // 10-segment seed-knob face, identical to compressor-unit's.
    const segmentDeg = 36, dividerDeg = 1;
    const colors = ['#F5DEB3', '#E8D5B7'];
    const divider = '#8B7355';
    const conicStops = [];
    for (let i = 0; i < 10; i++) {
      const start = i * segmentDeg;
      const end = start + segmentDeg - dividerDeg;
      conicStops.push(`${colors[i % 2]} ${start}deg, ${colors[i % 2]} ${end}deg, ${divider} ${end}deg, ${divider} ${start + segmentDeg}deg`);
    }

    const serif = "var(--font-serif, 'EB Garamond', Garamond, 'Times New Roman', serif)";
    const style = document.createElement('style');
    style.id = styleId;
    style.textContent = `
      .rev-unit-compact {
        box-sizing: border-box; width: 505px; height: 71px;
        display: flex; align-items: center;
        background: #221C14 linear-gradient(135deg, #2a2318 0%, #1a1510 100%);
        border: 1px solid #5C4033; border-radius: 6px;
        padding: 2px 21px;
        font-family: ${serif}; color: #E8D5B7; user-select: none;
      }
      .rev-row { display: flex; align-items: center; gap: 10px; width: 100%; }
      .rev-bypass-toggle {
        background: #6B5040; border: 1px solid #4A3728; border-radius: 3px;
        padding: 4px 0; font-family: inherit; font-size: 11px; line-height: 13px;
        font-weight: 600; letter-spacing: 1.5px; color: #E8D5B7;
        cursor: pointer; transition: all 0.1s ease; white-space: nowrap;
        /* Fixed, not min-width: a caption that changes width with the language
           re-centres the whole knob row. 76px holds REVERB / RÉVERB (53px) at 11px. */
        box-sizing: border-box; width: 76px; flex: none;
      }
      .rev-bypass-toggle.active { background: #4E6839; border-color: #3C5C1A; color: #FFF; }
      .rev-bypass-toggle:hover { filter: brightness(1.1); }
      .rev-knobs-compact { display: flex; gap: 16px; padding-left: 6px; justify-content: flex-start; }
      .rev-knob-group { display: flex; flex-direction: column; align-items: center; gap: 2px; width: 48px; }
      .rev-knob-container { width: 40px; height: 40px; cursor: pointer; position: relative; }
      .rev-seed-knob {
        width: 100%; height: 100%; border-radius: 50%; border: 2px solid #8B7355;
        position: relative;
        background:
          radial-gradient(circle, transparent 88%, #C9A27B 88%, #C9A27B 92%, #8B7355 92%, #8B7355 94%, transparent 94%),
          conic-gradient(from 0deg, ${conicStops.join(', ')}),
          radial-gradient(circle, #FFF8DC 0%, #FFF8DC 20%, transparent 20%);
        box-shadow: inset 1px 1px 3px rgba(0,0,0,0.3), inset -1px -1px 2px rgba(255,248,220,0.5), 2px 2px 6px rgba(0,0,0,0.25);
      }
      .rev-seed-knob:hover { transform: scale(1.03); }
      .rev-knob-indicator {
        position: absolute; width: 5px; height: 5px; background: #3C2F2F;
        border-radius: 50%; top: 3px; left: 50%; transform: translateX(-50%) rotate(${KNOB_ROTATION_OFFSET}deg);
        transform-origin: center 17px;
      }
      .rev-label { font-size: 9px; line-height: 10px; color: #B8A08C; text-align: center; letter-spacing: 0.3px; white-space: nowrap; }
      .rev-value { font-size: 9px; line-height: 10px; color: #E8D5B7; text-align: center; white-space: nowrap; }
      /* Bypassed: knob graphics fade; captions and readouts stay AA. Knobs stay
         live, so settings can be dialled in before switching the reverb on. */
      .rev-unit-compact.bypassed .rev-knob-container { opacity: 0.4; }
      /* Value-entry field, overlaid on the readout (which keeps its text,
         transparent), so nothing shifts. Form controls do not inherit fonts on
         their own, hence the two inherits. */
      .rev-value-input {
        position: absolute; left: 0; top: 0; width: 100%; height: 100%;
        box-sizing: border-box; padding: 0; margin: 0;
        border: none; border-bottom: 1px solid currentColor; border-radius: 0;
        background: transparent;
        font-family: inherit; font-size: inherit; line-height: normal;
        text-align: inherit; outline: none; -moz-appearance: textfield;
      }
      .rev-value-input::-webkit-outer-spin-button,
      .rev-value-input::-webkit-inner-spin-button { -webkit-appearance: none; margin: 0; }
    `;
    document.head.appendChild(style);
  }

  initialize() {
    this.render();
    this.setupBypassToggle();
    this.setupKnobs();
  }

  setupBypassToggle() {
    const btn = this.container.querySelector('.rev-bypass-toggle');
    const toggleState = this.getToggleState(`${this.paramPrefix}enabled`);
    if (!toggleState) {
      console.warn('[ReverbUnit] Could not get toggle state for enabled');
      return;
    }
    toggleState.valueChangedEvent.addListener(() => {
      this.enabled = toggleState.getValue();
      this.updateBypassVisual();
    });
    btn.addEventListener('click', () => toggleState.setValue(!toggleState.getValue()));
    this.enabled = toggleState.getValue();
    this.updateBypassVisual();
  }

  updateBypassVisual() {
    this.container.querySelector('.rev-bypass-toggle').classList.toggle('active', this.enabled);
    this.container.querySelector('.rev-unit-compact').classList.toggle('bypassed', !this.enabled);
  }

  setupKnobs() {
    let activeKnob = null;
    let lastY = 0;

    const onMouseMove = (e) => {
      if (!activeKnob) return;
      const deltaY = lastY - e.clientY;
      const next = Math.max(0, Math.min(1, activeKnob.getNormalisedValue() + deltaY / DRAG_SENSITIVITY));
      activeKnob.setNormalisedValue(next);
      lastY = e.clientY;
    };
    const onMouseUp = () => {
      if (activeKnob && activeKnob.sliderDragEnded) activeKnob.sliderDragEnded();
      activeKnob = null;
    };

    document.addEventListener('mousemove', onMouseMove);
    document.addEventListener('mouseup', onMouseUp);
    this.documentListeners.push({ event: 'mousemove', fn: onMouseMove });
    this.documentListeners.push({ event: 'mouseup', fn: onMouseUp });

    Object.keys(this.params).forEach((paramName) => {
      const knob = document.getElementById(`${this.paramPrefix}${paramName}_knob`);
      const sliderState = this.getSliderState(`${this.paramPrefix}${paramName}`);
      if (!sliderState) {
        console.warn(`[ReverbUnit] Could not get slider state for ${paramName}`);
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
        if (sliderState.sliderDragStarted) sliderState.sliderDragStarted();
        activeKnob = sliderState;
        lastY = e.clientY;
        e.preventDefault();
      });

      // Double-click (knob or readout) opens inline value entry. All reverb
      // ranges are linear, so the readout format is a linear map of norm.
      const p = this.params[paramName];
      const readout = document.getElementById(`${this.paramPrefix}${paramName}_value`);
      const formatFn = (norm) => p.format(p.min + norm * (p.max - p.min));
      formatFn.parse = p.parse;
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
      const p = this.params[paramName];
      valueDisplay.textContent = p.format(p.min + normalized * (p.max - p.min));
    }
  }

  destroy() {
    this.documentListeners.forEach(({ event, fn }) => document.removeEventListener(event, fn));
    this.documentListeners = [];
  }
}
