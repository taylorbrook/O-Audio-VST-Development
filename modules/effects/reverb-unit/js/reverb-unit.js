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
 * Ouaricon Audio Module System v1.0.0
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
 */

const DRAG_SENSITIVITY = 150;
const KNOB_ROTATION_RANGE = 270;
const KNOB_ROTATION_OFFSET = -135;

const pct = (v) => `${Math.round(v * 100)}%`;

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
      size:     { min: 0, max: 1,   label: 'Size',    format: pct },
      damp:     { min: 0, max: 1,   label: 'Damp',    format: pct },
      predelay: { min: 0, max: 200, label: 'Pre-dly', format: (v) => `${Math.round(v)} ms` },
      mod:      { min: 0, max: 1,   label: 'Mod',     format: pct },
      shimmer:  { min: 0, max: 1,   label: 'Shimmer', format: pct },
      mix:      { min: 0, max: 1,   label: 'Mix',     format: pct }
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

      knob.addEventListener('mousedown', (e) => {
        if (sliderState.sliderDragStarted) sliderState.sliderDragStarted();
        activeKnob = sliderState;
        lastY = e.clientY;
        e.preventDefault();
      });

      knob.addEventListener('dblclick', () => {
        sliderState.setNormalisedValue(this.defaults[paramName] ?? 0.5);
      });

      this.updateKnobVisual(paramName, sliderState.getNormalisedValue());
    });
  }

  updateKnobVisual(paramName, normalized) {
    const rotation = (normalized * KNOB_ROTATION_RANGE) + KNOB_ROTATION_OFFSET;
    const indicator = document.getElementById(`${this.paramPrefix}${paramName}_indicator`);
    if (indicator) indicator.style.transform = `translateX(-50%) rotate(${rotation}deg)`;

    const valueDisplay = document.getElementById(`${this.paramPrefix}${paramName}_value`);
    if (valueDisplay) {
      const p = this.params[paramName];
      valueDisplay.textContent = p.format(p.min + normalized * (p.max - p.min));
    }
  }

  destroy() {
    this.documentListeners.forEach(({ event, fn }) => document.removeEventListener(event, fn));
    this.documentListeners = [];
  }
}
