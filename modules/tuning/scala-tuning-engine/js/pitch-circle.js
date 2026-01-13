/**
 * Ouaricon Pitch Circle - JavaScript UI Module
 *
 * Circular visualization of scale intervals.
 * Shows scale degrees as lines radiating from center.
 *
 * Usage:
 *   import { PitchCircle } from './modules/pitch-circle.js';
 *
 *   const circle = new PitchCircle({
 *     container: document.getElementById('pitch-circle'),
 *     size: 150,
 *     showLabels: true
 *   });
 *
 *   circle.setIntervals([0, 100, 200, 300, ...]);
 */

const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

export class PitchCircle {
    constructor(options = {}) {
        this.container = options.container;
        this.size = options.size || 150;
        this.showLabels = options.showLabels !== false;
        this.lineColor = options.lineColor || '#6B8E4E';
        this.activeColor = options.activeColor || '#DC0000';
        this.labelColor = options.labelColor || '#3C2F2F';

        this.intervals = [0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100];
        this.tonic = 0;
        this.activeNotes = new Map();  // scaleIndex -> { count, velocity }

        this.svg = null;
        this.linesGroup = null;
        this.labelsGroup = null;

        this._buildSVG();
    }

    /**
     * Build SVG structure.
     */
    _buildSVG() {
        if (!this.container) return;

        const size = this.size;
        const cx = size / 2;
        const cy = size / 2;

        this.container.innerHTML = `
            <svg viewBox="0 0 ${size} ${size}" class="pitch-circle-svg">
                <!-- Outer circle -->
                <circle cx="${cx}" cy="${cy}" r="${cx - 5}" fill="none" stroke="#8B7355" stroke-width="1.5"/>
                <!-- Inner circle -->
                <circle cx="${cx}" cy="${cy}" r="${cx - 17}" fill="rgba(235, 217, 199, 0.4)" stroke="#8B7355" stroke-width="0.5"/>
                <!-- Center dot -->
                <circle cx="${cx}" cy="${cy}" r="3" fill="#3C2F2F"/>
                <!-- Interval lines -->
                <g class="interval-lines"></g>
                <!-- Degree labels -->
                <g class="degree-labels"></g>
            </svg>
        `;

        this.svg = this.container.querySelector('svg');
        this.linesGroup = this.svg.querySelector('.interval-lines');
        this.labelsGroup = this.svg.querySelector('.degree-labels');

        this._draw();
    }

    /**
     * Set intervals and redraw.
     */
    setIntervals(intervals, tonic = 0) {
        this.intervals = intervals;
        this.tonic = tonic;
        this._draw();
    }

    /**
     * Activate a note (highlight its line).
     */
    setNoteActive(midiNote, velocity = 0.8) {
        const scaleIndex = this._midiToScaleIndex(midiNote);

        if (this.activeNotes.has(scaleIndex)) {
            const entry = this.activeNotes.get(scaleIndex);
            entry.count++;
            entry.velocity = Math.max(entry.velocity, velocity);
        } else {
            this.activeNotes.set(scaleIndex, { count: 1, velocity });
        }

        this._updateLineVisual(scaleIndex);
    }

    /**
     * Deactivate a note.
     */
    setNoteInactive(midiNote) {
        const scaleIndex = this._midiToScaleIndex(midiNote);

        if (this.activeNotes.has(scaleIndex)) {
            const entry = this.activeNotes.get(scaleIndex);
            entry.count--;
            if (entry.count <= 0) {
                this.activeNotes.delete(scaleIndex);
            }
        }

        this._updateLineVisual(scaleIndex);
    }

    /**
     * Flash a note briefly (for non-sustaining triggers).
     */
    flashNote(midiNote, velocity = 0.8, durationMs = 150) {
        this.setNoteActive(midiNote, velocity);
        setTimeout(() => this.setNoteInactive(midiNote), durationMs);
    }

    /**
     * Map MIDI note to scale index.
     */
    _midiToScaleIndex(midiNote) {
        const noteInOctave = midiNote % 12;
        const scaleSize = this.intervals.length;

        if (scaleSize === 12) {
            return noteInOctave;
        }

        return Math.round((noteInOctave / 12) * scaleSize) % scaleSize;
    }

    /**
     * Get degree label.
     */
    _getDegreeLabel(index, total) {
        if (total === 12) {
            const noteIndex = (index + this.tonic) % 12;
            return NOTE_NAMES[noteIndex];
        }
        return String(index + 1);
    }

    /**
     * Draw the pitch circle.
     */
    _draw() {
        if (!this.linesGroup || !this.labelsGroup) return;

        this.linesGroup.innerHTML = '';
        this.labelsGroup.innerHTML = '';

        const cx = this.size / 2;
        const cy = this.size / 2;
        const innerR = 8;
        const outerR = this.size / 2 - 20;
        const labelR = this.size / 2 - 10;
        const total = this.intervals.length;

        // Adjust styling for large scales
        const strokeWidth = total > 19 ? 1 : 2;
        const fontSize = total > 12 ? 5 : 7;

        this.intervals.forEach((cents, i) => {
            // Convert cents to angle (0 cents = top, clockwise)
            const angle = (cents / 1200) * 360 - 90;
            const rad = angle * (Math.PI / 180);

            // Line endpoints
            const x1 = cx + innerR * Math.cos(rad);
            const y1 = cy + innerR * Math.sin(rad);
            const x2 = cx + outerR * Math.cos(rad);
            const y2 = cy + outerR * Math.sin(rad);

            // Create line
            const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
            line.setAttribute('x1', x1);
            line.setAttribute('y1', y1);
            line.setAttribute('x2', x2);
            line.setAttribute('y2', y2);
            line.setAttribute('stroke', this.lineColor);
            line.setAttribute('stroke-width', strokeWidth);
            line.setAttribute('stroke-linecap', 'round');
            line.setAttribute('data-index', i);
            this.linesGroup.appendChild(line);

            // Label (skip some for large scales)
            if (this.showLabels && (total <= 24 || i % 2 === 0)) {
                const labelX = cx + labelR * Math.cos(rad);
                const labelY = cy + labelR * Math.sin(rad);

                const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
                text.setAttribute('x', labelX);
                text.setAttribute('y', labelY + 2);
                text.setAttribute('text-anchor', 'middle');
                text.setAttribute('font-size', fontSize);
                text.setAttribute('fill', this.labelColor);
                text.textContent = this._getDegreeLabel(i, total);
                this.labelsGroup.appendChild(text);
            }
        });

        // Restore active note highlighting
        this.activeNotes.forEach((_, scaleIndex) => {
            this._updateLineVisual(scaleIndex);
        });
    }

    /**
     * Update visual appearance of a line.
     */
    _updateLineVisual(scaleIndex) {
        const lines = this.linesGroup.querySelectorAll('line');
        if (scaleIndex >= 0 && scaleIndex < lines.length) {
            const line = lines[scaleIndex];

            if (this.activeNotes.has(scaleIndex)) {
                const entry = this.activeNotes.get(scaleIndex);
                line.setAttribute('stroke', this._velocityToColor(entry.velocity));
                line.setAttribute('stroke-width', '4');
            } else {
                const strokeWidth = this.intervals.length > 19 ? 1 : 2;
                line.setAttribute('stroke', this.lineColor);
                line.setAttribute('stroke-width', strokeWidth);
            }
        }
    }

    /**
     * Convert velocity to color (darker red to bright red).
     */
    _velocityToColor(velocity) {
        const minR = 120, maxR = 220;
        const minG = 40, maxG = 0;
        const minB = 40, maxB = 0;

        const r = Math.round(minR + velocity * (maxR - minR));
        const g = Math.round(minG + velocity * (maxG - minG));
        const b = Math.round(minB + velocity * (maxB - minB));

        return `rgb(${r}, ${g}, ${b})`;
    }
}

// Global export for non-module usage
if (typeof window !== 'undefined') {
    window.OuariconPitchCircle = PitchCircle;
}
