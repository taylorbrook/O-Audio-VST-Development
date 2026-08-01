/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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
 * scala-tuning-engine module v3.0.0
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
 * - Real-time note highlighting (noteOn/noteOff)
 *
 * Usage:
 *   import { TuningPanel, initTuningPanel } from './tuning-panel.js';
 *   const panel = new TuningPanel(document.getElementById('tuning-container'), window.__JUCE__);
 *   panel.init();
 *   // or:
 *   const panel = await initTuningPanel(document.getElementById('tuning-container'), window.__JUCE__);
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
        this.heldNotesMidi = [];
        this.heldNotesFreqs = [];
        this.activeScaleDegrees = new Set();

        // Note names for display
        this.noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
    }

    async init() {
        this.render();
        this.attachEventListeners();
        await this.loadInitialState();
    }

    // ===================================================================
    // RENDERING
    // ===================================================================

    render() {
        this.container.innerHTML = `
            <div class="tuning-panel-root">
            <div class="tuning-panel">
                <!-- LEFT: Interval List -->
                <div class="tuning-viz-container">
                    <div class="interval-list" id="interval-list">
                        <div class="interval-list-header">Intervals (<span id="interval-count">12</span> notes)</div>
                    </div>
                </div>

                <!-- Visualization Mode Toggle -->
                <div class="viz-mode-toggle">
                    <button class="viz-btn active" data-mode="circle">Circle</button>
                    <button class="viz-btn" data-mode="polar">Polar</button>
                    <button class="viz-btn" data-mode="matrix">Matrix</button>
                    <button class="viz-btn" data-mode="truekeys">True Keys</button>
                    <button class="viz-btn" data-mode="rotation">Rotation</button>
                </div>

                <!-- CENTER: Visualization Container -->
                <div class="viz-container" id="viz-container">
                    <div class="viz-view active" id="circle-view">
                        <div class="pitch-circle">
                            <svg viewBox="0 0 320 320" id="pitch-circle-svg">
                                <circle cx="160" cy="160" r="150" fill="none" stroke="#8B7355" stroke-width="1.5"/>
                                <circle cx="160" cy="160" r="125" fill="rgba(235, 217, 199, 0.4)" stroke="#8B7355" stroke-width="0.5"/>
                                <circle cx="160" cy="160" r="4" fill="#3C2F2F"/>
                                <g id="interval-lines"></g>
                                <g id="degree-labels" font-size="10" fill="#5C4033"></g>
                            </svg>
                            <div class="pitch-circle-label">Scale Intervals</div>
                        </div>
                    </div>
                    <div class="viz-view" id="polar-view">
                        <canvas id="polar-canvas"></canvas>
                    </div>
                    <div class="viz-view matrix-view" id="matrix-view"></div>
                    <div class="viz-view truekeys-view" id="truekeys-view">
                        <div class="tk-hint">Hold 2+ notes to see intervals</div>
                    </div>
                    <div class="viz-view rotation-view" id="rotation-view"></div>
                </div>

                <!-- RIGHT: Controls Panel -->
                <div class="tuning-controls-panel">
                    <!-- Tuning Library -->
                    <div class="library-section" id="library-section">
                        <div class="library-header">
                            <span class="library-header-text">Tuning Library</span>
                            <span class="library-toggle" id="library-toggle">&#9660;</span>
                        </div>
                        <div class="library-content" id="library-content">
                            <div class="library-filter">
                                <select id="library-filter" class="library-filter-select">
                                    <option value="all">All Categories</option>
                                    <option value="Historical">Historical</option>
                                    <option value="Just Intonation">Just Intonation</option>
                                    <option value="Equal Divisions">Equal Divisions</option>
                                    <option value="Non-Octave">Non-Octave</option>
                                    <option value="World">World</option>
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
                        <div class="ref-knob-label">A4 REF</div>
                        <div class="ref-knob-value" id="ref-pitch-value">440.0 Hz</div>
                    </div>

                    <!-- Scale Name Display -->
                    <div class="scale-name-display" id="scale-name-display">12-TET Standard</div>

                    <!-- Octave Stretch -->
                    <div class="octave-stretch-section">
                        <div class="octave-stretch-row">
                            <span class="octave-stretch-label">Stretch</span>
                            <input type="range" id="octave-stretch" class="octave-stretch-slider"
                                   min="0.95" max="1.25" step="0.01" value="1.0">
                            <span class="octave-stretch-value" id="octave-stretch-value">1.00</span>
                        </div>
                    </div>

                    <!-- File Operations -->
                    <div class="tuning-file-section">
                        <div class="tuning-file-buttons">
                            <button class="tuning-file-btn" id="btn-load-scl">Load .SCL</button>
                            <button class="tuning-file-btn" id="btn-load-kbm">Load .KBM</button>
                            <button class="tuning-file-btn" id="btn-save-scl">Save .SCL</button>
                            <button class="tuning-file-btn" id="btn-save-kbm">Save .KBM</button>
                            <button class="tuning-file-btn tuning-export-btn" id="btn-export-html">Export HTML</button>
                        </div>
                    </div>

                    <!-- Scale Generator -->
                    <div class="generator-section" id="generator-section">
                        <div class="generator-header">
                            <span class="generator-header-text">Generate Scale</span>
                            <span class="generator-toggle" id="generator-toggle">&#9660;</span>
                        </div>
                        <div class="generator-content" id="generator-content">
                            <div class="generator-type-row">
                                <select id="generator-type" class="generator-type-select">
                                    <option value="edo">EDO (Equal Division)</option>
                                    <option value="harmonic">Harmonic Series</option>
                                    <option value="rank2">Rank-2 Temperament</option>
                                </select>
                            </div>
                            <div class="generator-inputs" id="generator-inputs">
                                <div class="gen-row">
                                    <label>Divisions</label>
                                    <input type="number" id="gen-divisions" value="19" min="5" max="53">
                                </div>
                                <div class="gen-row">
                                    <label>Period (c)</label>
                                    <input type="number" id="gen-period" value="1200" min="100" max="2400" step="1">
                                </div>
                            </div>
                            <button class="generator-btn" id="btn-generate">Generate</button>
                        </div>
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

    // ===================================================================
    // STATE MANAGEMENT
    // ===================================================================

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

    // ===================================================================
    // INTERVAL LIST
    // ===================================================================

    updateIntervalList() {
        const listEl = this.container.querySelector('#interval-list');
        const countEl = this.container.querySelector('#interval-count');

        if (!listEl) return;

        const count = this.intervals.length - 1; // Exclude period
        if (countEl) countEl.textContent = count;

        let html = `<div class="interval-list-header">Intervals (${count} notes)</div>`;

        // Tonic selector
        html += `
            <div class="tonic-selector">
                <span class="tonic-label">Tonic</span>
                <button class="tonic-arrow" id="tonic-down">&#9668;</button>
                <span class="tonic-value" id="tonic-value">${this.noteNames[this.tonic]}</span>
                <button class="tonic-arrow" id="tonic-up">&#9658;</button>
            </div>
        `;

        // Interval rows
        for (let i = 0; i < this.intervals.length; i++) {
            const cents = this.intervals[i];
            const isOctave = i === this.intervals.length - 1;
            html += `
                <div class="interval-item ${isOctave ? 'octave' : ''}">
                    <span class="interval-degree">${this.getNoteLabel(i, count)}</span>
                    <input type="text" class="interval-input" data-index="${i}"
                           value="${cents.toFixed(2)}" ${isOctave ? 'readonly' : ''}>
                    <span class="interval-unit">c</span>
                </div>
            `;
        }

        listEl.innerHTML = html;

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
            this.container.querySelector('#tonic-value').textContent = this.noteNames[tonic];
            this.updateVisualization();
        } catch (err) {
            console.error('[TuningPanel] Failed to set tonic:', err);
        }
    }

    // ===================================================================
    // VISUALIZATION
    // ===================================================================

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

        // Dynamic sizing based on container
        const rect = canvas.parentElement.getBoundingClientRect();
        const size = Math.max(Math.min(rect.width, rect.height) - 20, 100);
        const dpr = window.devicePixelRatio || 1;
        canvas.width = size * dpr;
        canvas.height = size * dpr;
        canvas.style.width = size + 'px';
        canvas.style.height = size + 'px';
        ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

        const w = size, h = size;
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
            html += `<th>${i}</th>`;
        }
        html += '</tr>';

        // Data rows
        for (let i = 0; i < count; i++) {
            html += `<tr><th>${i}</th>`;
            for (let j = 0; j < count; j++) {
                let diff = this.intervals[j] - this.intervals[i];
                if (diff < 0) diff += period;
                html += `<td>${diff.toFixed(1)}</td>`;
            }
            html += '</tr>';
        }

        html += '</table>';
        container.innerHTML = html;
    }

    drawTrueKeys() {
        const container = this.container.querySelector('#truekeys-view');
        if (!container) return;

        if (this.heldNotes.size < 2) {
            container.innerHTML = '<div class="tk-hint">Hold 2+ notes to see intervals</div>';
            return;
        }

        const notes = Array.from(this.heldNotes).sort((a, b) => a - b);
        let html = '<div class="tk-intervals">';

        for (let i = 1; i < notes.length; i++) {
            const interval = notes[i] - notes[0];
            const scaleDegree = interval % (this.intervals.length - 1);
            const cents = this.intervals[scaleDegree] || (interval * 100);
            html += `<div class="tk-interval">${notes[0]}->${notes[i]}: ${cents.toFixed(1)}c</div>`;
        }

        html += '</div>';
        container.innerHTML = html;
    }

    drawRotationTable() {
        const container = this.container.querySelector('#rotation-view');
        if (!container) return;

        const count = this.intervals.length - 1;
        const period = this.intervals[this.intervals.length - 1] || 1200;

        let html = '<table class="rotation-table"><tr><th>Mode</th>';

        for (let i = 0; i < count; i++) {
            html += `<th>${i}</th>`;
        }
        html += '</tr>';

        for (let mode = 0; mode < count; mode++) {
            const isCurrentTonic = mode === this.tonic;
            html += `<tr class="${isCurrentTonic ? 'current-tonic' : ''}"><th>${this.noteNames[mode % 12]}</th>`;

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
    }

    // ===================================================================
    // NOTE HIGHLIGHTING
    // ===================================================================

    /**
     * v3.0.0: Called by host via window.updateHeldNotes with MIDI notes + actual frequencies.
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
     * Backward-compatible alias for updateHeldNotes.
     */
    setHeldNotes(notes) {
        this.updateHeldNotes(notes, []);
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

    // ===================================================================
    // TUNING LIBRARY
    // ===================================================================

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
                    <div class="library-item-desc">${tuning.noteCount} notes</div>
                </div>
            `;
        }

        listEl.innerHTML = html;

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

    // ===================================================================
    // SCALE GENERATOR
    // ===================================================================

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
                        <label>Divisions</label>
                        <input type="number" id="gen-divisions" value="19" min="5" max="53">
                    </div>
                    <div class="gen-row">
                        <label>Period (c)</label>
                        <input type="number" id="gen-period" value="1200" min="100" max="2400" step="1">
                    </div>
                `;
                break;
            case 'harmonic':
                inputsDiv.innerHTML = `
                    <div class="gen-row">
                        <label>Start Harmonic</label>
                        <input type="number" id="gen-start" value="8" min="1" max="32">
                    </div>
                    <div class="gen-row">
                        <label>End Harmonic</label>
                        <input type="number" id="gen-end" value="16" min="2" max="64">
                    </div>
                `;
                break;
            case 'rank2':
                inputsDiv.innerHTML = `
                    <div class="gen-row">
                        <label>Generator (c)</label>
                        <input type="number" id="gen-generator" value="696.6" min="1" max="1199" step="0.1">
                    </div>
                    <div class="gen-row">
                        <label>Period (c)</label>
                        <input type="number" id="gen-r2-period" value="1200" min="100" max="2400">
                    </div>
                    <div class="gen-row">
                        <label>Notes</label>
                        <input type="number" id="gen-count" value="12" min="3" max="31">
                    </div>
                `;
                break;
        }
    }

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

    // ===================================================================
    // FILE OPERATIONS
    // ===================================================================

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

    // ===================================================================
    // OCTAVE STRETCH
    // ===================================================================

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

    // ===================================================================
    // REFERENCE PITCH KNOB
    // ===================================================================

    setupRefPitchKnob() {
        const knob = this.container.querySelector('#ref-pitch-knob');
        if (!knob) return;

        let isDragging = false;
        let startY = 0;
        let startValue = 440;

        const updateKnob = (hz) => {
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

        knob.addEventListener('mousedown', (e) => {
            isDragging = true;
            startY = e.clientY;
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

    // ===================================================================
    // HELPERS
    // ===================================================================

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

/**
 * Convenience function for plugins that expect a function-style initialization.
 * Usage: const panel = await initTuningPanel(container, Juce);
 */
export async function initTuningPanel(container, juceApi) {
    const panel = new TuningPanel(container, juceApi);
    await panel.init();
    return panel;
}
