/*
   This file is part of O-SpectralShaper, an Ouaricon Audio plugin.
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
 * CurveEditor Base Class
 *
 * Provides common functionality for drawable curve editors:
 * - Logarithmic X-axis (20Hz to Nyquist)
 * - Y-axis: -1.0 to +1.0 (displayed as ±12dB)
 * - Grid overlay with frequency labels
 * - Abstract draw() method for subclasses
 */

export class CurveEditor {
    constructor(canvasId, config = {}) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');

        // Configuration
        this.numBands = config.numBands || 32;
        this.minFreq = 20;
        this.maxFreq = 22050; // Will be set to Nyquist in init
        this.accentColor = config.accentColor || '#4A90D9';
        this.gridColor = 'rgba(232, 224, 212, 0.15)';
        this.textColor = '#A89888';

        // Cached log constants (used by freqToX/xToFreq every frame)
        this.logMinFreq = Math.log(this.minFreq);
        this.logMaxFreq = Math.log(this.maxFreq);
        this.logFreqRange = this.logMaxFreq - this.logMinFreq;

        // Cached band frequencies (immutable — depend only on minFreq/maxFreq/numBands)
        this.bandFrequencies = this._computeBandFrequencies();

        // Curve data (32 values, -1.0 to +1.0)
        this.curveData = new Array(this.numBands).fill(0.0);

        // Undo/redo stacks (curveData snapshots, max 30)
        this.undoStack = [];
        this.redoStack = [];
        this.maxUndoSteps = 30;

        // Transient activity data (32 values, 0.0-1.0) for glow animation
        this.transientActivity = new Float32Array(this.numBands);
        this.hasActiveTransients = false;

        // Spectrum overlay data (257 FFT bins, raw magnitudes)
        this.spectrumData = null;
        this.showSpectrum = false;
        this.spectrumColor = config.accentColor || '#4A90D9';

        // Callback for curve updates
        this.onCurveChange = null;

        // Callback for undo/redo state changes (for button enable/disable)
        this.onUndoStateChange = null;

        // Focus tracking for keyboard shortcuts
        this._boundFocusHandler = () => { CurveEditor._focusedEditor = this; };
        this.canvas.addEventListener('mousedown', this._boundFocusHandler);

        // Attach global undo/redo keyboard listener (once across all instances)
        if (!CurveEditor._keyboardListenerAttached) {
            CurveEditor._keyboardListenerAttached = true;
            document.addEventListener('keydown', (e) => {
                if (!CurveEditor._focusedEditor) return;
                CurveEditor._focusedEditor._handleUndoRedoKey(e);
            });
        }

        // Setup canvas size (may need to wait for layout)
        this.resizeCanvas();

        // Handle window resize
        this._boundHandleResize = () => this.handleResize();
        window.addEventListener('resize', this._boundHandleResize);

        // Initial render (deferred to ensure layout is complete)
        requestAnimationFrame(() => {
            this.resizeCanvas();
            this.render();
        });
    }

    handleResize() {
        this.resizeCanvas();
        this.render();
    }

    resizeCanvas() {
        const rect = this.canvas.getBoundingClientRect();

        // Ensure we have valid dimensions (fallback to parent or minimum)
        const width = rect.width > 0 ? rect.width : (this.canvas.parentElement?.clientWidth || 400);
        const height = rect.height > 0 ? rect.height : (this.canvas.parentElement?.clientHeight || 100);

        // Only resize if dimensions changed
        if (this.width === width && this.height === height) return;

        this.width = width;
        this.height = height;

        // Set canvas buffer size (high-DPI support)
        const dpr = window.devicePixelRatio || 1;
        this.canvas.width = width * dpr;
        this.canvas.height = height * dpr;

        // Reset and scale context
        this.ctx.setTransform(1, 0, 0, 1, 0, 0);
        this.ctx.scale(dpr, dpr);

        console.log(`CurveEditor resized: ${width}x${height} (canvas: ${this.canvas.width}x${this.canvas.height})`);
    }

    /**
     * Convert frequency to X pixel coordinate (logarithmic)
     */
    freqToX(freq) {
        const normalized = (Math.log(freq) - this.logMinFreq) / this.logFreqRange;
        return normalized * this.width;
    }

    /**
     * Convert X pixel coordinate to frequency (logarithmic)
     */
    xToFreq(x) {
        const normalized = x / this.width;
        return Math.exp(this.logMinFreq + normalized * this.logFreqRange);
    }

    /**
     * Convert gain value (-1 to +1) to Y pixel coordinate
     */
    gainToY(gain) {
        const normalized = (gain + 1.0) / 2.0; // Map -1..+1 to 0..1
        return this.height - (normalized * this.height); // Flip Y axis
    }

    /**
     * Convert Y pixel coordinate to gain value (-1 to +1)
     */
    yToGain(y) {
        const normalized = 1.0 - (y / this.height); // Flip Y axis
        return (normalized * 2.0) - 1.0; // Map 0..1 to -1..+1
    }

    /**
     * Get 32 logarithmically-spaced band center frequencies
     */
    getBandFrequencies() {
        return this.bandFrequencies;
    }

    /**
     * Compute band frequencies (called once in constructor)
     */
    _computeBandFrequencies() {
        const bands = [];
        for (let i = 0; i < this.numBands; i++) {
            const t = i / (this.numBands - 1);
            const logFreq = this.logMinFreq + t * this.logFreqRange;
            bands.push(Math.exp(logFreq));
        }
        return bands;
    }

    /**
     * Draw grid overlay with frequency labels
     */
    drawGrid() {
        this.ctx.strokeStyle = this.gridColor;
        this.ctx.lineWidth = 1;
        this.ctx.font = '9px Georgia';
        this.ctx.fillStyle = this.textColor;

        // Vertical grid lines (frequency markers)
        const freqMarkers = [50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000];
        freqMarkers.forEach(freq => {
            if (freq > this.maxFreq) return;
            const x = this.freqToX(freq);

            // Line
            this.ctx.beginPath();
            this.ctx.moveTo(x, 0);
            this.ctx.lineTo(x, this.height);
            this.ctx.stroke();

            // Label
            const label = freq >= 1000 ? `${freq / 1000}k` : `${freq}`;
            this.ctx.fillText(label, x + 2, this.height - 4);
        });

        // Horizontal grid lines (gain markers)
        const gainMarkers = [-1.0, -0.5, 0.0, 0.5, 1.0];
        gainMarkers.forEach(gain => {
            const y = this.gainToY(gain);

            // Line
            this.ctx.beginPath();
            this.ctx.moveTo(0, y);
            this.ctx.lineTo(this.width, y);
            this.ctx.stroke();

            // Label (dB)
            const db = gain * 12.0; // ±12dB range
            const label = `${db >= 0 ? '+' : ''}${db.toFixed(0)}dB`;
            this.ctx.fillText(label, 4, y - 2);
        });

        // Center line (0dB) emphasis
        const centerY = this.gainToY(0.0);
        this.ctx.strokeStyle = 'rgba(232, 224, 212, 0.3)';
        this.ctx.lineWidth = 1;
        this.ctx.beginPath();
        this.ctx.moveTo(0, centerY);
        this.ctx.lineTo(this.width, centerY);
        this.ctx.stroke();
    }

    /**
     * Draw the curve (implemented by subclasses)
     */
    drawCurve() {
        // Override in subclass (Freehand or Node)
    }

    /**
     * Set transient activity data from visualization pipeline
     * @param {number[]} data - 32 float values (0.0 to 1.0)
     */
    setTransientActivity(data) {
        let hasActive = false;
        for (let i = 0; i < this.numBands; i++) {
            this.transientActivity[i] = data[i] || 0;
            if (this.transientActivity[i] > 0.02) hasActive = true;
        }
        this.hasActiveTransients = hasActive;
    }

    /**
     * Draw transient glow overlay behind the curve.
     * Renders per-band vertical glow strips whose opacity tracks transient activity.
     * Color ramps from deep red (low activity) through orange to bright yellow (peak).
     */
    drawTransientGlow() {
        if (!this.hasActiveTransients) return;

        const bandFreqs = this.getBandFrequencies();
        const ctx = this.ctx;

        ctx.save();
        ctx.globalCompositeOperation = 'lighter'; // Additive blending for glow

        for (let i = 0; i < this.numBands; i++) {
            const activity = this.transientActivity[i];
            if (activity < 0.02) continue;

            // Band center X position
            const x = this.freqToX(bandFreqs[i]);

            // Band width: distance to neighbors (log-spaced, so wider at high freq)
            let bandWidth;
            if (i < this.numBands - 1) {
                bandWidth = this.freqToX(bandFreqs[i + 1]) - x;
            } else {
                bandWidth = this.width - x;
            }
            // Minimum visible width
            bandWidth = Math.max(bandWidth, 4);

            // Glow Y position centered on the curve value for this band
            const curveGain = this.curveData[i] || 0;
            const curveY = this.gainToY(curveGain);

            // Glow radius scales with activity
            const glowRadius = (this.height * 0.35) * activity;

            // Heat colormap: red → orange → yellow based on activity level
            const r = 255;
            const g = Math.round(60 + activity * 160); // 60 at low, 220 at peak
            const b = Math.round(activity * 40);        // Subtle warm tint at peak
            const alpha = activity * 0.4;                // Max 40% opacity

            // Radial glow centered on curve position
            const grad = ctx.createRadialGradient(
                x, curveY, 0,
                x, curveY, glowRadius
            );
            grad.addColorStop(0, `rgba(${r}, ${g}, ${b}, ${alpha})`);
            grad.addColorStop(0.5, `rgba(${r}, ${g}, ${b}, ${alpha * 0.4})`);
            grad.addColorStop(1, `rgba(${r}, ${g}, ${b}, 0)`);

            ctx.fillStyle = grad;
            ctx.fillRect(x - bandWidth * 0.6, curveY - glowRadius, bandWidth * 1.2, glowRadius * 2);
        }

        ctx.restore();
    }

    /**
     * Set FFT spectrum data for overlay rendering
     * @param {number[]} fftBins - 257 float values (raw magnitudes, 0.0+)
     */
    setSpectrumData(fftBins) {
        this.spectrumData = fftBins;
    }

    /**
     * Draw real-time input spectrum as a semi-transparent filled shape behind the curve.
     * Maps 257 linear FFT bins onto the logarithmic X-axis.
     */
    drawSpectrumOverlay() {
        if (!this.showSpectrum || !this.spectrumData) return;

        const ctx = this.ctx;
        const bins = this.spectrumData;
        const numBins = bins.length; // 257
        // Nyquist frequency — bins are linearly spaced 0..maxFreq
        const nyquist = this.maxFreq;

        ctx.save();

        // Build filled path from bin 1 (skip DC at bin 0)
        ctx.beginPath();
        ctx.moveTo(0, this.height); // Start at bottom-left

        let started = false;
        for (let i = 1; i < numBins; i++) {
            // Linear frequency for this bin
            const freq = (i / (numBins - 1)) * nyquist;
            if (freq < this.minFreq) continue;

            const x = this.freqToX(freq);

            // Convert magnitude to dB, normalize to 0..1 (60dB range)
            const mag = bins[i] || 0;
            const db = 20 * Math.log10(Math.max(mag, 0.000001));
            const normalized = Math.max(0, Math.min(1, (db + 60) / 60));

            // Map to canvas Y (fills from bottom)
            const y = this.height - (normalized * this.height);

            if (!started) {
                ctx.moveTo(x, this.height);
                ctx.lineTo(x, y);
                started = true;
            } else {
                ctx.lineTo(x, y);
            }
        }

        // Close path along bottom
        ctx.lineTo(this.width, this.height);
        ctx.closePath();

        // Semi-transparent fill using curve accent color
        ctx.fillStyle = this.spectrumColor + '25'; // ~15% opacity (hex alpha)
        ctx.fill();

        // Subtle top edge stroke
        ctx.strokeStyle = this.spectrumColor + '40'; // ~25% opacity
        ctx.lineWidth = 1;
        ctx.stroke();

        ctx.restore();
    }

    /**
     * Main render loop
     */
    render() {
        // Clear canvas
        this.ctx.clearRect(0, 0, this.width, this.height);

        // Draw grid
        this.drawGrid();

        // Draw spectrum overlay (behind everything else)
        this.drawSpectrumOverlay();

        // Draw transient glow (behind curve)
        this.drawTransientGlow();

        // Draw curve (subclass implementation)
        this.drawCurve();
    }

    /**
     * Set curve data from C++ (32 values, -1.0 to +1.0)
     */
    setCurveData(data) {
        if (data.length !== this.numBands) {
            console.error('Invalid curve data length:', data.length);
            return;
        }
        this.curveData = [...data];
        this.render();
    }

    /**
     * Get curve data for C++ (32 values, -1.0 to +1.0)
     */
    getCurveData() {
        return [...this.curveData];
    }

    /**
     * Reset curve to flat (0dB across all bands)
     */
    resetCurve() {
        this.pushUndoSnapshot();
        this.curveData = new Array(this.numBands).fill(0.0);
        this.render();
        this.notifyCurveChange();
    }

    // =====================================================================
    // UNDO / REDO
    // =====================================================================

    /**
     * Get a snapshot of the current editor state.
     * Subclasses override to include additional state (e.g. nodes).
     */
    getSnapshot() {
        return { curveData: [...this.curveData] };
    }

    /**
     * Restore editor state from a snapshot.
     * Subclasses override to restore additional state.
     */
    restoreSnapshot(snapshot) {
        this.curveData = [...snapshot.curveData];
        this.render();
    }

    /**
     * Push current state onto the undo stack. Call before making changes.
     * Clears the redo stack (new action invalidates redo history).
     */
    pushUndoSnapshot() {
        this.undoStack.push(this.getSnapshot());
        if (this.undoStack.length > this.maxUndoSteps) {
            this.undoStack.shift();
        }
        this.redoStack.length = 0;
        this._notifyUndoState();
    }

    /**
     * Undo: restore previous state, push current state onto redo stack.
     */
    undo() {
        if (this.undoStack.length === 0) return;
        this.redoStack.push(this.getSnapshot());
        const snapshot = this.undoStack.pop();
        this.restoreSnapshot(snapshot);
        this.notifyCurveChange();
        this._notifyUndoState();
    }

    /**
     * Redo: restore next state, push current state onto undo stack.
     */
    redo() {
        if (this.redoStack.length === 0) return;
        this.undoStack.push(this.getSnapshot());
        const snapshot = this.redoStack.pop();
        this.restoreSnapshot(snapshot);
        this.notifyCurveChange();
        this._notifyUndoState();
    }

    get canUndo() { return this.undoStack.length > 0; }
    get canRedo() { return this.redoStack.length > 0; }

    _notifyUndoState() {
        if (this.onUndoStateChange) {
            this.onUndoStateChange(this.canUndo, this.canRedo);
        }
    }

    /**
     * Handle Ctrl+Z / Ctrl+Shift+Z keyboard shortcuts
     */
    _handleUndoRedoKey(e) {
        const isCtrlOrMeta = e.ctrlKey || e.metaKey;
        if (!isCtrlOrMeta || e.key.toLowerCase() !== 'z') return;

        e.preventDefault();
        if (e.shiftKey) {
            this.redo();
        } else {
            this.undo();
        }
    }

    /**
     * Remove event listeners for cleanup
     */
    destroy() {
        window.removeEventListener('resize', this._boundHandleResize);
        this.canvas.removeEventListener('mousedown', this._boundFocusHandler);
        if (CurveEditor._focusedEditor === this) {
            CurveEditor._focusedEditor = null;
        }
    }

    /**
     * Notify listeners of curve change
     */
    notifyCurveChange() {
        if (this.onCurveChange) {
            this.onCurveChange(this.getCurveData());
        }
    }
}

// Static state for focus tracking and single keyboard listener
CurveEditor._focusedEditor = null;
CurveEditor._keyboardListenerAttached = false;
