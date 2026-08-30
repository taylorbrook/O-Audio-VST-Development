/*
   This file is part of O-Bells, an Ouaricon Audio plugin.
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
 * tuning-panel.js
 * scala-tuning-engine module v2.0.0 — O-Bells' PLUGIN-OWNED copy
 *
 * LOCALIZED (O-Bells v4.2.0, canon v2). This is not the module file: it is a
 * plugin-owned copy already 279 lines diverged from
 * modules/tuning/scala-tuning-engine/js/tuning-panel.js, and O-Bells carries no
 * dependencies.json listing the module, so /module-upgrade will not revert the
 * data-i18n keys below. Localizing it deliberately WIDENS that divergence: the
 * alternative was a page whose Tuning tab stayed English while every other tab
 * spoke French.
 *
 * Its keys live in ../js/i18n.js. Every subtree this file REBUILDS is followed
 * by window.__reapplyI18n(), which re-runs the sweep at the current language —
 * a node built after the last sweep is a node the sweep never saw. It cannot be
 * window.__setLanguage, which needs a language argument this file does not have.
 *
 * Complete tuning panel UI component for WebView-based JUCE plugins.
 * Includes:
 * - Editable intervals table with tonic selector
 * - 5 visualization modes (Circle, Polar, Matrix, TrueKeys, Rotation)
 * - Tuning library browser (factory presets)
 * - Scale generator (EDO, Harmonic Series, Rank-2)
 * - Reference pitch (A4) control
 * - Octave stretch control
 * - Scala file I/O (.scl, .kbm)
 * - HTML export
 *
 * Usage:
 *   import { TuningPanel } from './tuning-panel.js';
 *   import * as Juce from './js/juce/index.js';
 *   // Pass the ES-module `Juce` namespace, NOT window.__JUCE__ — only the module
 *   // namespace exposes getNativeFunction() (see critical_juce_webview_namespace_vs_postmessage).
 *   const panel = new TuningPanel(document.getElementById('tuning-container'), Juce);
 *   panel.init();
 */

export class TuningPanel {
    constructor(containerElement, juceApi) {
        this.container = containerElement;
        this.juce = juceApi;

        // State
        this.intervals = [];
        this.scaleName = '12-TET Standard';
        this.tonic = 0;
        this.currentVizMode = 'circle';
        this.embeddedTunings = [];
        this.libraryFilter = 'all';
        this.generatorType = 'edo';
        this.heldNotes = new Set();
        this.heldNotesMidi = [];   // v3.1.0: MIDI note numbers from C++ backend
        this.heldNotesFreqs = [];  // v3.1.0: Actual frequencies from TuningEngine
        this.activeScaleDegrees = new Set(); // Track which scale degrees are currently sounding

        // Note names for display
        this.noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
    }

    async init() {
        this.render();
        this.attachEventListeners();
        await this.loadInitialState();
    }

    // ═══════════════════════════════════════════════════════════════════
    // RENDERING
    // ═══════════════════════════════════════════════════════════════════

    render() {
        this.container.innerHTML = `
            <div class="tuning-panel">
                <!-- LEFT: Interval List -->
                <div class="tuning-intervals-column">
                    <div class="interval-list" id="interval-list">
                        <div class="interval-list-header" id="interval-list-header"
                             data-i18n="label.intervalsHeader" data-i18n-vars='{"n":12}'>Intervals: 12</div>
                    </div>
                </div>

                <!-- CENTER: Viz Toggle + Visualization -->
                <div class="tuning-center-column">
                    <div class="viz-mode-toggle">
                        <button class="viz-btn active" data-mode="circle" data-i18n="label.vizCircle">Circle</button>
                        <button class="viz-btn" data-mode="polar" data-i18n="label.vizPolar">Polar</button>
                        <button class="viz-btn" data-mode="matrix" data-i18n="label.vizMatrix">Matrix</button>
                        <button class="viz-btn" data-mode="truekeys" data-i18n="label.vizTrueKeys">True Keys</button>
                        <button class="viz-btn" data-mode="rotation" data-i18n="label.vizRotation">Rotation</button>
                    </div>
                    <div class="viz-container" id="viz-container">
                        <div class="viz-view active" id="circle-view">
                            <div class="pitch-circle">
                                <svg viewBox="0 0 320 320" id="pitch-circle-svg">
                                    <circle cx="160" cy="160" r="150" fill="none" stroke="#8B7355" stroke-width="1.5"/>
                                    <circle cx="160" cy="160" r="125" fill="rgba(235, 217, 199, 0.3)" stroke="#8B7355" stroke-width="0.5"/>
                                    <circle cx="160" cy="160" r="5" fill="#3C2F2F"/>
                                    <g id="interval-lines"></g>
                                    <g id="degree-labels" font-size="11" fill="#5C4033"></g>
                                </svg>
                                <div class="pitch-circle-label" data-i18n="label.scaleIntervals">Scale Intervals</div>
                            </div>
                        </div>
                        <div class="viz-view" id="polar-view">
                            <canvas id="polar-canvas" width="300" height="300"></canvas>
                        </div>
                        <div class="viz-view matrix-view" id="matrix-view"></div>
                        <div class="viz-view truekeys-view" id="truekeys-view">
                            <div class="tk-hint" data-i18n="label.tkHint">Hold 2+ notes to see intervals</div>
                        </div>
                        <div class="viz-view rotation-view" id="rotation-view"></div>
                    </div>
                </div>

                <!-- RIGHT: Controls Panel -->
                <div class="tuning-controls-panel">
                    <!-- Tuning Library -->
                    <div class="library-section" id="library-section">
                        <div class="library-header">
                            <span class="library-header-text" data-i18n="label.tuningLibrary">Tuning Library</span>
                            <span class="library-toggle" id="library-toggle">▼</span>
                        </div>
                        <div class="library-content" id="library-content">
                            <div class="library-filter">
                                <select id="library-filter" class="library-filter-select">
                                    <option value="all" data-i18n="label.catAllCategories">All Categories</option>
                                    <option value="Historical" data-i18n="label.catHistorical">Historical</option>
                                    <option value="Just Intonation" data-i18n="label.catJustIntonation">Just Intonation</option>
                                    <option value="Equal Divisions" data-i18n="label.catEqualDivisions">Equal Divisions</option>
                                    <option value="Non-Octave" data-i18n="label.catNonOctave">Non-Octave</option>
                                    <option value="World" data-i18n="label.catWorld">World</option>
                                </select>
                            </div>
                            <div class="library-list" id="library-list"></div>
                        </div>
                    </div>

                    <!-- Reference Pitch -->
                    <div class="tuning-ref-section">
                        <div class="ref-knob-container">
                            <div class="ref-knob" id="ref-pitch-knob">
                                <div class="ref-knob-indicator" id="ref-pitch-indicator"></div>
                            </div>
                        </div>
                        <div class="ref-knob-label" data-i18n="label.a4Ref">A4 REF</div>
                        <div class="ref-knob-value" id="ref-pitch-value">440.0 Hz</div>
                    </div>

                    <!-- Scale Name Display -->
                    <div class="scale-name-display" id="scale-name-display">12-TET Standard</div>

                    <!-- Octave Stretch -->
                    <div class="octave-stretch-section">
                        <div class="octave-stretch-row">
                            <span class="octave-stretch-label" data-i18n="label.stretch">Stretch</span>
                            <input type="range" id="octave-stretch" class="octave-stretch-slider"
                                   min="0.95" max="1.25" step="0.01" value="1.0">
                            <span class="octave-stretch-value" id="octave-stretch-value">1.00</span>
                        </div>
                    </div>

                    <!-- File Operations -->
                    <div class="tuning-file-section">
                        <div class="tuning-file-buttons">
                            <button class="tuning-file-btn" id="btn-load-scl" data-i18n="label.loadScl">Load .SCL</button>
                            <button class="tuning-file-btn" id="btn-load-kbm" data-i18n="label.loadKbm">Load .KBM</button>
                            <button class="tuning-file-btn" id="btn-save-scl" data-i18n="label.saveScl">Save .SCL</button>
                            <button class="tuning-file-btn" id="btn-save-kbm" data-i18n="label.saveKbm">Save .KBM</button>
                            <button class="tuning-file-btn tuning-export-btn" id="btn-export-html" data-i18n="label.exportHtml">Export HTML</button>
                        </div>
                    </div>

                    <!-- Scale Generator -->
                    <div class="generator-section" id="generator-section">
                        <div class="generator-header">
                            <span class="generator-header-text" data-i18n="label.generateScale">Generate Scale</span>
                            <span class="generator-toggle" id="generator-toggle">▼</span>
                        </div>
                        <div class="generator-content" id="generator-content">
                            <div class="generator-type-row">
                                <select id="generator-type" class="generator-type-select">
                                    <option value="edo" data-i18n="label.genEdo">EDO (Equal Division)</option>
                                    <option value="harmonic" data-i18n="label.genHarmonic">Harmonic Series</option>
                                    <option value="rank2" data-i18n="label.genRank2">Rank-2 Temperament</option>
                                </select>
                            </div>
                            <div class="generator-inputs" id="generator-inputs">
                                <div class="gen-row">
                                    <label data-i18n="label.genDivisions">Divisions</label>
                                    <input type="number" id="gen-divisions" value="19" min="5" max="53">
                                </div>
                                <div class="gen-row">
                                    <label data-i18n="label.genPeriod">Period (c)</label>
                                    <input type="number" id="gen-period" value="1200" min="100" max="2400" step="1">
                                </div>
                            </div>
                            <button class="generator-btn" id="btn-generate" data-i18n="label.generate">Generate</button>
                        </div>
                    </div>
                </div>
            </div>
        `;
    }

    attachEventListeners() {
        // Visualization mode buttons
        this.container.querySelectorAll('.viz-btn').forEach(btn => {
            btn.addEventListener('click', () => this.setVizMode(btn.dataset.mode));
        });

        // Library toggle
        this.container.querySelector('.library-header').addEventListener('click', () => this.toggleLibrary());
        this.container.querySelector('#library-filter').addEventListener('change', (e) => this.filterLibrary(e.target.value));

        // Generator toggle
        this.container.querySelector('.generator-header').addEventListener('click', () => this.toggleGenerator());
        this.container.querySelector('#generator-type').addEventListener('change', (e) => this.setGeneratorType(e.target.value));
        this.container.querySelector('#btn-generate').addEventListener('click', () => this.generate());

        // File operations
        this.container.querySelector('#btn-load-scl').addEventListener('click', () => this.loadSCL());
        this.container.querySelector('#btn-load-kbm').addEventListener('click', () => this.loadKBM());
        this.container.querySelector('#btn-save-scl').addEventListener('click', () => this.saveSCL());
        this.container.querySelector('#btn-save-kbm').addEventListener('click', () => this.saveKBM());
        this.container.querySelector('#btn-export-html').addEventListener('click', () => this.exportHTML());

        // Octave stretch
        this.container.querySelector('#octave-stretch').addEventListener('input', (e) => this.setOctaveStretch(e.target.value));

        // Reference pitch knob (drag)
        this.setupRefPitchKnob();
    }

    // ═══════════════════════════════════════════════════════════════════
    // STATE MANAGEMENT
    // ═══════════════════════════════════════════════════════════════════

    async loadInitialState() {
        if (!this.juce) return;

        try {
            // Get intervals
            const intervalsJson = await this.juce.getNativeFunction('getTuningIntervals')();
            this.intervals = JSON.parse(intervalsJson);

            // Get scale name
            this.scaleName = await this.juce.getNativeFunction('getTuningName')();

            // Get tonic
            this.tonic = await this.juce.getNativeFunction('getTonicNote')();

            // Get octave stretch
            const stretch = await this.juce.getNativeFunction('getOctaveStretch')();
            this.container.querySelector('#octave-stretch').value = stretch;
            this.container.querySelector('#octave-stretch-value').textContent = stretch.toFixed(2);

            // Update UI
            this.updateIntervalList();
            this.updateVisualization();
            this.updateScaleNameDisplay();

        } catch (e) {
            console.error('[TuningPanel] Failed to load initial state:', e);
        }
    }

    async refreshState() {
        await this.loadInitialState();
    }

    // ═══════════════════════════════════════════════════════════════════
    // INTERVAL LIST
    // ═══════════════════════════════════════════════════════════════════

    updateIntervalList() {
        const listEl = this.container.querySelector('#interval-list');

        if (!listEl) return;

        const count = this.intervals.length - 1; // Exclude period

        // v4.2.0: the count rides a {n} var on the KEY instead of being written
        // into a #interval-count span, and the noun moved in front of the number.
        // Contract §6: the inflection is authored AROUND, not engineered —
        // English pluralizes zero as "0 notes" and French as "0 note", so
        // "Intervals (N notes)" needed a plural engine and "Intervals: N" does
        // not. The old span was written and then destroyed two statements later
        // by the innerHTML swap below, so nothing depended on it.
        let html = `<div class="interval-list-header" id="interval-list-header"
                         data-i18n="label.intervalsHeader" data-i18n-vars='{"n":${count}}'>Intervals: ${count}</div>`;

        // Tonic selector
        html += `
            <div class="tonic-selector">
                <span class="tonic-label" data-i18n="label.tonic">Tonic</span>
                <button class="tonic-arrow" id="tonic-down">◄</button>
                <span class="tonic-value" id="tonic-value">${this.noteNames[this.tonic]}</span>
                <button class="tonic-arrow" id="tonic-up">►</button>
            </div>
        `;

        // Interval rows
        for (let i = 0; i < this.intervals.length; i++) {
            const cents = this.intervals[i];
            const isOctave = i === this.intervals.length - 1;
            const degreeLabel = this.getNoteLabel(i, count);
            html += `
                <div class="interval-item ${isOctave ? 'octave' : ''}">
                    <span class="interval-degree">${degreeLabel}</span>
                    <input type="text" class="interval-input" data-index="${i}"
                           value="${cents.toFixed(isOctave ? 1 : 1)}" ${isOctave ? 'readonly' : ''}>
                    <span class="interval-unit">c</span>
                </div>
            `;
        }

        listEl.innerHTML = html;
        if (window.__reapplyI18n) window.__reapplyI18n();

        // Attach event listeners for intervals
        listEl.querySelectorAll('.interval-input:not([readonly])').forEach(input => {
            input.addEventListener('change', (e) => this.handleIntervalChange(e));
            input.addEventListener('keydown', (e) => {
                if (e.key === 'Enter') e.target.blur();
            });
        });

        // Tonic buttons
        listEl.querySelector('#tonic-down')?.addEventListener('click', () => this.decrementTonic());
        listEl.querySelector('#tonic-up')?.addEventListener('click', () => this.incrementTonic());
    }

    async handleIntervalChange(e) {
        const index = parseInt(e.target.dataset.index);
        const cents = parseFloat(e.target.value);

        if (isNaN(cents)) {
            e.target.value = this.intervals[index].toFixed(2);
            return;
        }

        try {
            await this.juce.getNativeFunction('setSingleInterval')(index, cents);
            this.intervals[index] = cents;
            this.updateVisualization();
        } catch (err) {
            console.error('[TuningPanel] Failed to set interval:', err);
        }
    }

    async incrementTonic() {
        this.tonic = (this.tonic + 1) % 12;
        await this.setTonic(this.tonic);
    }

    async decrementTonic() {
        this.tonic = (this.tonic - 1 + 12) % 12;
        await this.setTonic(this.tonic);
    }

    async setTonic(tonic) {
        try {
            await this.juce.getNativeFunction('setTonicNote')(tonic);
            this.updateIntervalList();
            this.updateVisualization();
        } catch (err) {
            console.error('[TuningPanel] Failed to set tonic:', err);
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    // VISUALIZATION
    // ═══════════════════════════════════════════════════════════════════

    setVizMode(mode) {
        this.currentVizMode = mode;

        // Update button states
        this.container.querySelectorAll('.viz-btn').forEach(btn => {
            btn.classList.toggle('active', btn.dataset.mode === mode);
        });

        // Show/hide views
        this.container.querySelectorAll('.viz-view').forEach(view => {
            view.classList.toggle('active', view.id === `${mode}-view`);
        });

        this.updateVisualization();
    }

    updateVisualization() {
        switch (this.currentVizMode) {
            case 'circle':
                this.drawPitchCircle();
                break;
            case 'polar':
                this.drawPolarPlot();
                break;
            case 'matrix':
                this.drawIntervalMatrix();
                break;
            case 'truekeys':
                this.drawTrueKeys();
                break;
            case 'rotation':
                this.drawRotationTable();
                break;
        }
    }

    drawPitchCircle() {
        const linesGroup = this.container.querySelector('#interval-lines');
        const labelsGroup = this.container.querySelector('#degree-labels');
        if (!linesGroup || !labelsGroup) return;

        linesGroup.innerHTML = '';
        labelsGroup.innerHTML = '';

        // Store spoke elements for note highlighting
        this.spokeElements = [];

        const cx = 160, cy = 160, radius = 125;
        const count = this.intervals.length - 1; // Exclude period
        const period = this.intervals[this.intervals.length - 1] || 1200;

        for (let i = 0; i < count; i++) {
            const cents = this.intervals[i];
            const angle = (cents / period) * 2 * Math.PI - Math.PI / 2;

            const x = cx + Math.cos(angle) * radius;
            const y = cy + Math.sin(angle) * radius;

            const isActive = this.activeScaleDegrees.has(i);
            const spokeColor = isActive ? '#C0392B' : '#5C4033';
            const dotColor = isActive ? '#C0392B' : '#8B7355';
            const dotRadius = isActive ? 6 : 5;
            const strokeWidth = isActive ? 2.5 : 1.5;

            // Line from center (spoke)
            const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
            line.setAttribute('x1', cx);
            line.setAttribute('y1', cy);
            line.setAttribute('x2', x);
            line.setAttribute('y2', y);
            line.setAttribute('stroke', spokeColor);
            line.setAttribute('stroke-width', strokeWidth);
            line.dataset.degree = i;
            linesGroup.appendChild(line);

            // Dot at interval position
            const dot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
            dot.setAttribute('cx', x);
            dot.setAttribute('cy', y);
            dot.setAttribute('r', dotRadius);
            dot.setAttribute('fill', dotColor);
            dot.dataset.degree = i;
            linesGroup.appendChild(dot);

            // Store references for fast updates
            this.spokeElements.push({ line, dot, degree: i });

            // Degree label (note name for 12-note scales, number otherwise)
            const labelRadius = radius + 16;
            const lx = cx + Math.cos(angle) * labelRadius;
            const ly = cy + Math.sin(angle) * labelRadius;

            const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            label.setAttribute('x', lx);
            label.setAttribute('y', ly);
            label.setAttribute('text-anchor', 'middle');
            label.setAttribute('dominant-baseline', 'middle');
            label.textContent = this.getNoteLabel(i, count);
            labelsGroup.appendChild(label);
        }
    }

    drawPolarPlot() {
        const canvas = this.container.querySelector('#polar-canvas');
        if (!canvas) return;

        const ctx = canvas.getContext('2d');
        const w = canvas.width, h = canvas.height;
        const cx = w / 2, cy = h / 2;
        const radius = Math.min(cx, cy) - 10;

        ctx.clearRect(0, 0, w, h);

        // Background circles
        ctx.strokeStyle = '#d4c9b5';
        ctx.lineWidth = 0.5;
        for (let r = radius / 4; r <= radius; r += radius / 4) {
            ctx.beginPath();
            ctx.arc(cx, cy, r, 0, Math.PI * 2);
            ctx.stroke();
        }

        // Plot intervals
        const count = this.intervals.length - 1;
        const period = this.intervals[this.intervals.length - 1] || 1200;

        ctx.fillStyle = '#8B7355';
        ctx.strokeStyle = '#5C4033';
        ctx.lineWidth = 1;

        for (let i = 0; i < count; i++) {
            const cents = this.intervals[i];
            const angle = (cents / period) * 2 * Math.PI - Math.PI / 2;
            const r = (cents / period) * radius;

            const x = cx + Math.cos(angle) * r;
            const y = cy + Math.sin(angle) * r;

            ctx.beginPath();
            ctx.arc(x, y, 4, 0, Math.PI * 2);
            ctx.fill();
        }
    }

    drawIntervalMatrix() {
        const container = this.container.querySelector('#matrix-view');
        if (!container) return;

        const count = this.intervals.length - 1;
        const period = this.intervals[this.intervals.length - 1] || 1200;

        let html = '<table class="matrix-table"><tr><th></th>';

        // Header row
        for (let i = 0; i < count; i++) {
            html += `<th>${this.getNoteLabel(i, count)}</th>`;
        }
        html += '</tr>';

        // Data rows
        for (let i = 0; i < count; i++) {
            html += `<tr><th>${this.getNoteLabel(i, count)}</th>`;
            for (let j = 0; j < count; j++) {
                let diff = this.intervals[j] - this.intervals[i];
                if (diff < 0) diff += period;
                html += `<td>${diff.toFixed(1)}</td>`;
            }
            html += '</tr>';
        }

        html += '</table>';
        container.innerHTML = html;
        if (window.__reapplyI18n) window.__reapplyI18n();
    }

    drawTrueKeys() {
        const container = this.container.querySelector('#truekeys-view');
        if (!container) return;

        if (!this.heldNotesMidi || this.heldNotesMidi.length < 2) {
            container.innerHTML = '<div class="tk-hint" data-i18n="label.tkHint">Hold 2+ notes to see intervals</div>';
            if (window.__reapplyI18n) window.__reapplyI18n();
            return;
        }

        // Sort by pitch for consistent display
        const sorted = this.heldNotesMidi
            .map((note, i) => ({ note, freq: this.heldNotesFreqs[i] }))
            .sort((a, b) => a.note - b.note);

        let html = '<div class="tk-grid">';

        // Calculate interval between each pair of adjacent notes
        for (let i = 0; i < sorted.length - 1; i++) {
            const lower = sorted[i];
            const upper = sorted[i + 1];
            const cents = 1200 * Math.log2(upper.freq / lower.freq);

            const intervalName = this.identifyInterval(cents);

            html += `
                <div class="tk-interval">
                    <span>${this.midiToNoteName(lower.note)} → ${this.midiToNoteName(upper.note)} ${intervalName}</span>
                    <span class="tk-cents">${cents.toFixed(1)}¢</span>
                </div>
            `;
        }

        // Total span for 3+ notes
        if (sorted.length > 2) {
            const lowest = sorted[0];
            const highest = sorted[sorted.length - 1];
            const totalCents = 1200 * Math.log2(highest.freq / lowest.freq);
            html += `
                <div class="tk-interval tk-total">
                    <span class="tk-total-label" data-i18n="label.totalSpan">Total span</span>
                    <span class="tk-cents">${totalCents.toFixed(1)}¢</span>
                </div>
            `;
        }

        html += '</div>';
        container.innerHTML = html;
        if (window.__reapplyI18n) window.__reapplyI18n();
    }

    /**
     * Convert MIDI note number to readable note name (e.g., 60 → "C4")
     */
    midiToNoteName(midi) {
        return this.noteNames[midi % 12] + (Math.floor(midi / 12) - 1);
    }

    /**
     * Identify common interval name from cents value (±15c tolerance)
     */
    identifyInterval(cents) {
        const intervals = [
            [100, 'm2'], [200, 'M2'], [300, 'm3'], [400, 'M3'],
            [500, 'P4'], [600, 'TT'], [700, 'P5'], [800, 'm6'],
            [900, 'M6'], [1000, 'm7'], [1100, 'M7'], [1200, 'P8']
        ];
        for (const [target, name] of intervals) {
            if (Math.abs(cents - target) < 15) return `(${name})`;
        }
        return '';
    }

    drawRotationTable() {
        const container = this.container.querySelector('#rotation-view');
        if (!container) return;

        const count = this.intervals.length - 1;
        const period = this.intervals[this.intervals.length - 1] || 1200;

        let html = '<table class="rotation-table"><tr><th data-i18n="label.mode">Mode</th>';

        for (let i = 0; i < count; i++) {
            html += `<th>${this.getNoteLabel(i, count)}</th>`;
        }
        html += '</tr>';

        for (let mode = 0; mode < count; mode++) {
            const isCurrentTonic = mode === this.tonic;
            const modeLabel = count === 12 ? this.noteNames[mode % 12] : mode.toString();
            html += `<tr class="${isCurrentTonic ? 'current-tonic' : ''}"><th>${modeLabel}</th>`;

            for (let i = 0; i < count; i++) {
                const srcIdx = (mode + i) % count;
                let cents = this.intervals[srcIdx] - this.intervals[mode];
                if (cents < 0) cents += period;
                html += `<td>${cents.toFixed(1)}</td>`;
            }
            html += '</tr>';
        }

        html += '</table>';
        container.innerHTML = html;
        if (window.__reapplyI18n) window.__reapplyI18n();
    }

    /**
     * v3.1.0: Called by host via window.updateHeldNotes with MIDI notes + actual frequencies.
     * Enables accurate interval reporting using real tuning engine frequencies.
     */
    updateHeldNotes(notes, freqs) {
        this.heldNotesMidi = notes || [];
        this.heldNotesFreqs = freqs || [];
        this.heldNotes = new Set(notes || []);
        if (this.currentVizMode === 'truekeys') {
            this.drawTrueKeys();
        }
    }

    /**
     * Called when a MIDI note starts sounding.
     * Maps the MIDI note to a scale degree and highlights the corresponding spoke.
     */
    noteOn(midiNote) {
        const scaleSize = this.intervals.length - 1;
        if (scaleSize <= 0) return;
        // Map MIDI note to scale degree (relative to tonic)
        const degree = ((midiNote - this.tonic) % scaleSize + scaleSize) % scaleSize;
        this.activeScaleDegrees.add(degree);
        this.updateSpokeHighlights();
    }

    /**
     * Called when a MIDI note stops sounding.
     */
    noteOff(midiNote) {
        const scaleSize = this.intervals.length - 1;
        if (scaleSize <= 0) return;
        const degree = ((midiNote - this.tonic) % scaleSize + scaleSize) % scaleSize;
        this.activeScaleDegrees.delete(degree);
        this.updateSpokeHighlights();
    }

    /**
     * Fast in-place spoke color update without full redraw.
     */
    updateSpokeHighlights() {
        if (!this.spokeElements || this.currentVizMode !== 'circle') return;

        for (const { line, dot, degree } of this.spokeElements) {
            const isActive = this.activeScaleDegrees.has(degree);
            line.setAttribute('stroke', isActive ? '#C0392B' : '#5C4033');
            line.setAttribute('stroke-width', isActive ? '2.5' : '1.5');
            dot.setAttribute('fill', isActive ? '#C0392B' : '#8B7355');
            dot.setAttribute('r', isActive ? '6' : '5');
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    // TUNING LIBRARY
    // ═══════════════════════════════════════════════════════════════════

    toggleLibrary() {
        const content = this.container.querySelector('#library-content');
        const toggle = this.container.querySelector('#library-toggle');
        if (content && toggle) {
            content.classList.toggle('expanded');
            toggle.classList.toggle('expanded');

            if (content.classList.contains('expanded') && this.embeddedTunings.length === 0) {
                this.loadEmbeddedTunings();
            }
        }
    }

    async loadEmbeddedTunings() {
        if (!this.juce) return;

        try {
            const jsonStr = await this.juce.getNativeFunction('getEmbeddedTuningList')();
            this.embeddedTunings = JSON.parse(jsonStr);
            this.renderLibraryList();
        } catch (e) {
            console.error('[TuningPanel] Failed to load embedded tunings:', e);
        }
    }

    filterLibrary(category) {
        this.libraryFilter = category;
        this.renderLibraryList();
    }

    renderLibraryList() {
        const listEl = this.container.querySelector('#library-list');
        if (!listEl) return;

        const filtered = this.libraryFilter === 'all'
            ? this.embeddedTunings
            : this.embeddedTunings.filter(t => t.category === this.libraryFilter);

        let html = '';
        for (const tuning of filtered) {
            html += `
                <div class="library-item" data-id="${tuning.id}">
                    <div class="library-item-name">${tuning.name}</div>
                    <div class="library-item-desc" data-i18n="label.noteCount"
                         data-i18n-vars='{"n":${tuning.noteCount}}'>Notes: ${tuning.noteCount}</div>
                </div>
            `;
        }

        listEl.innerHTML = html;
        if (window.__reapplyI18n) window.__reapplyI18n();

        // Attach click handlers
        listEl.querySelectorAll('.library-item').forEach(item => {
            item.addEventListener('click', () => this.loadEmbeddedTuning(item.dataset.id));
        });
    }

    async loadEmbeddedTuning(tuningId) {
        if (!this.juce) return;

        try {
            const success = await this.juce.getNativeFunction('loadEmbeddedTuning')(tuningId);
            if (success) {
                await this.refreshState();
            }
        } catch (e) {
            console.error('[TuningPanel] Failed to load embedded tuning:', e);
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    // SCALE GENERATOR
    // ═══════════════════════════════════════════════════════════════════

    toggleGenerator() {
        const content = this.container.querySelector('#generator-content');
        const toggle = this.container.querySelector('#generator-toggle');
        if (content && toggle) {
            content.classList.toggle('expanded');
            toggle.classList.toggle('expanded');
        }
    }

    setGeneratorType(type) {
        this.generatorType = type;
        const inputsDiv = this.container.querySelector('#generator-inputs');
        if (!inputsDiv) return;

        switch (type) {
            case 'edo':
                inputsDiv.innerHTML = `
                    <div class="gen-row">
                        <label data-i18n="label.genDivisions">Divisions</label>
                        <input type="number" id="gen-divisions" value="19" min="5" max="53">
                    </div>
                    <div class="gen-row">
                        <label data-i18n="label.genPeriod">Period (c)</label>
                        <input type="number" id="gen-period" value="1200" min="100" max="2400" step="1">
                    </div>
                `;
                break;
            case 'harmonic':
                inputsDiv.innerHTML = `
                    <div class="gen-row">
                        <label data-i18n="label.genStart">Start Harmonic</label>
                        <input type="number" id="gen-start" value="8" min="1" max="32">
                    </div>
                    <div class="gen-row">
                        <label data-i18n="label.genEnd">End Harmonic</label>
                        <input type="number" id="gen-end" value="16" min="2" max="64">
                    </div>
                `;
                break;
            case 'rank2':
                inputsDiv.innerHTML = `
                    <div class="gen-row">
                        <label data-i18n="label.genGenerator">Generator (c)</label>
                        <input type="number" id="gen-generator" value="696.6" min="1" max="1199" step="0.1">
                    </div>
                    <div class="gen-row">
                        <label data-i18n="label.genPeriod">Period (c)</label>
                        <input type="number" id="gen-r2-period" value="1200" min="100" max="2400">
                    </div>
                    <div class="gen-row">
                        <label data-i18n="label.genCount">Notes</label>
                        <input type="number" id="gen-count" value="12" min="3" max="31">
                    </div>
                `;
                break;
        }

        // The form was just REBUILT, so its two or three [data-i18n] labels are
        // brand-new nodes the last sweep never saw.
        if (window.__reapplyI18n) window.__reapplyI18n();
    }

    // The scale names built below are NOT localized. They are passed to
    // applyGeneratedScale() on the C++ side, become the tuning's stored name,
    // and are written into .scl files and the exported HTML — localizing them
    // would put French inside a Scala file. D-02: the name IS the data.
    async generate() {
        if (!this.juce) return;

        try {
            let intervalsJson, scaleName;

            switch (this.generatorType) {
                case 'edo': {
                    const divisions = parseInt(this.container.querySelector('#gen-divisions').value);
                    const period = parseFloat(this.container.querySelector('#gen-period').value);
                    intervalsJson = await this.juce.getNativeFunction('generateEDO')(divisions, period);
                    scaleName = `${divisions}-EDO`;
                    if (Math.abs(period - 1200) > 0.1) {
                        scaleName += ` (${period.toFixed(0)}c)`;
                    }
                    break;
                }
                case 'harmonic': {
                    const start = parseInt(this.container.querySelector('#gen-start').value);
                    const end = parseInt(this.container.querySelector('#gen-end').value);
                    intervalsJson = await this.juce.getNativeFunction('generateHarmonicSeries')(start, end);
                    scaleName = `Harmonics ${start}-${end}`;
                    break;
                }
                case 'rank2': {
                    const generator = parseFloat(this.container.querySelector('#gen-generator').value);
                    const period = parseFloat(this.container.querySelector('#gen-r2-period').value);
                    const count = parseInt(this.container.querySelector('#gen-count').value);
                    intervalsJson = await this.juce.getNativeFunction('generateRank2')(generator, period, count);
                    scaleName = `Rank-2 (${generator.toFixed(1)}c, ${count} notes)`;
                    break;
                }
            }

            const success = await this.juce.getNativeFunction('applyGeneratedScale')(intervalsJson, scaleName);
            if (success) {
                await this.refreshState();
            }
        } catch (e) {
            console.error('[TuningPanel] Generate failed:', e);
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    // FILE OPERATIONS
    // ═══════════════════════════════════════════════════════════════════

    async loadSCL() {
        if (!this.juce) return;
        try {
            const name = await this.juce.getNativeFunction('loadScalaFile')();
            if (name) {
                await this.refreshState();
            }
        } catch (e) {
            console.error('[TuningPanel] Load SCL failed:', e);
        }
    }

    async loadKBM() {
        if (!this.juce) return;
        try {
            await this.juce.getNativeFunction('loadKBMFile')();
        } catch (e) {
            console.error('[TuningPanel] Load KBM failed:', e);
        }
    }

    async saveSCL() {
        if (!this.juce) return;
        try {
            await this.juce.getNativeFunction('saveScalaFile')();
        } catch (e) {
            console.error('[TuningPanel] Save SCL failed:', e);
        }
    }

    async saveKBM() {
        if (!this.juce) return;
        try {
            await this.juce.getNativeFunction('saveKBMFile')();
        } catch (e) {
            console.error('[TuningPanel] Save KBM failed:', e);
        }
    }

    async exportHTML() {
        if (!this.juce) return;
        try {
            await this.juce.getNativeFunction('exportTuningHTML')();
        } catch (e) {
            console.error('[TuningPanel] Export HTML failed:', e);
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    // OCTAVE STRETCH
    // ═══════════════════════════════════════════════════════════════════

    async setOctaveStretch(value) {
        const stretch = parseFloat(value);
        this.container.querySelector('#octave-stretch-value').textContent = stretch.toFixed(2);

        if (!this.juce) return;
        try {
            await this.juce.getNativeFunction('setOctaveStretch')(stretch);
        } catch (e) {
            console.error('[TuningPanel] Set octave stretch failed:', e);
        }
    }

    // ═══════════════════════════════════════════════════════════════════
    // REFERENCE PITCH KNOB
    // ═══════════════════════════════════════════════════════════════════

    setupRefPitchKnob() {
        const knob = this.container.querySelector('#ref-pitch-knob');
        if (!knob) return;

        let isDragging = false;
        let startY = 0;
        let startValue = 440;
        let currentHz = 440;

        const updateKnob = (hz) => {
            currentHz = hz;
            const indicator = this.container.querySelector('#ref-pitch-indicator');
            const valueEl = this.container.querySelector('#ref-pitch-value');
            if (indicator) {
                const angle = ((hz - 400) / 80) * 270 - 135;
                indicator.style.transform = `rotate(${angle}deg)`;
            }
            if (valueEl) {
                valueEl.textContent = `${hz.toFixed(1)} Hz`;
            }
        };

        // WR-08/IN-03: initialize the knob from the backend (APVTS-backed master
        // tune) so it reflects the persisted/recalled value instead of a hardcoded
        // 440 Hz. Without this the A4 knob mis-displays after a state recall.
        if (this.juce) {
            this.juce.getNativeFunction('getMasterTune')().then(hz => {
                const v = parseFloat(hz);
                if (!isNaN(v) && v > 0) updateKnob(v);
            }).catch(() => {});
        }

        knob.addEventListener('mousedown', (e) => {
            isDragging = true;
            startY = e.clientY;
            // Accumulate from the current value (fixes drags always restarting at 440).
            startValue = currentHz;
            document.body.style.cursor = 'ns-resize';
        });

        document.addEventListener('mousemove', async (e) => {
            if (!isDragging) return;

            const delta = (startY - e.clientY) * 0.5;
            const newHz = Math.max(400, Math.min(480, startValue + delta));
            updateKnob(newHz);

            if (this.juce) {
                try {
                    await this.juce.getNativeFunction('setMasterTune')(newHz);
                } catch (err) {
                    // Throttled errors expected
                }
            }
        });

        document.addEventListener('mouseup', () => {
            if (isDragging) {
                isDragging = false;
                document.body.style.cursor = '';
            }
        });
    }

    // ═══════════════════════════════════════════════════════════════════
    // HELPERS
    // ═══════════════════════════════════════════════════════════════════

    updateScaleNameDisplay() {
        const el = this.container.querySelector('#scale-name-display');
        if (el) {
            el.textContent = this.scaleName;
        }
    }

    /**
     * Get note name label for a scale degree.
     * For 12-note scales: returns chromatic note names rotated by tonic (C, C#, D, ...).
     * For other scales: returns the degree number.
     */
    getNoteLabel(index, scaleSize) {
        if (scaleSize === 12) {
            return this.noteNames[(this.tonic + index) % 12];
        }
        return index.toString();
    }
}

// Default export for convenience
export default TuningPanel;
