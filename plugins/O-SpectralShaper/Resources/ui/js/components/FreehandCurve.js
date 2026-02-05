/**
 * FreehandCurve - Freehand drawing mode with Catmull-Rom smoothing
 *
 * User drags to draw a freehand curve, which is smoothed using
 * Catmull-Rom splines and sampled at 32 logarithmic band centers.
 *
 * Supports partial drawing: releasing the mouse and re-clicking
 * only updates the frequency range covered by the new stroke.
 */

import { CurveEditor } from './CurveEditor.js';

export class FreehandCurve extends CurveEditor {
    constructor(canvasId, config = {}) {
        super(canvasId, config);

        // Freehand-specific state
        this.isDrawing = false;
        this.rawPoints = []; // Raw mouse coordinates for current stroke
        this.smoothedPoints = []; // Catmull-Rom smoothed points for current stroke

        // Bind event handlers
        this.onMouseDown = this.onMouseDown.bind(this);
        this.onMouseMove = this.onMouseMove.bind(this);
        this.onMouseUp = this.onMouseUp.bind(this);

        // Attach listeners
        this.canvas.addEventListener('mousedown', this.onMouseDown);

        // Throttle updates to 30fps during drag
        this.lastUpdateTime = 0;
        this.updateThrottle = 1000 / 30; // 33ms
    }

    onMouseDown(e) {
        e.preventDefault();
        this.isDrawing = true;

        // Start new stroke (curveData is preserved from previous strokes)
        this.rawPoints = [];
        this.smoothedPoints = [];
        const rect = this.canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;
        this.rawPoints.push({ x, y });

        // Attach global listeners
        document.addEventListener('mousemove', this.onMouseMove);
        document.addEventListener('mouseup', this.onMouseUp);
    }

    onMouseMove(e) {
        if (!this.isDrawing) return;

        const rect = this.canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;

        // Add point
        this.rawPoints.push({ x, y });

        // Throttle updates
        const now = performance.now();
        if (now - this.lastUpdateTime < this.updateThrottle) return;
        this.lastUpdateTime = now;

        // Smooth and sample (only within stroke range)
        this.smoothAndSample();

        // Render
        this.render();

        // Notify (throttled)
        this.notifyCurveChange();
    }

    onMouseUp(e) {
        if (!this.isDrawing) return;

        this.isDrawing = false;

        // Final smooth and sample
        this.smoothAndSample();
        this.render();

        // Final notification
        this.notifyCurveChange();

        // Clear stroke data (curveData is preserved)
        this.rawPoints = [];
        this.smoothedPoints = [];

        // Remove global listeners
        document.removeEventListener('mousemove', this.onMouseMove);
        document.removeEventListener('mouseup', this.onMouseUp);
    }

    /**
     * Catmull-Rom spline smoothing
     */
    smoothAndSample() {
        if (this.rawPoints.length < 2) return;

        // Sort points by X coordinate
        this.rawPoints.sort((a, b) => a.x - b.x);

        // Generate smoothed points using Catmull-Rom
        this.smoothedPoints = [];

        for (let i = 0; i < this.rawPoints.length - 1; i++) {
            const p0 = this.rawPoints[Math.max(0, i - 1)];
            const p1 = this.rawPoints[i];
            const p2 = this.rawPoints[i + 1];
            const p3 = this.rawPoints[Math.min(this.rawPoints.length - 1, i + 2)];

            // Interpolate between p1 and p2
            const steps = 10;
            for (let t = 0; t < steps; t++) {
                const tt = t / steps;
                const tt2 = tt * tt;
                const tt3 = tt2 * tt;

                // Standard Catmull-Rom formula (centripetal)
                // P(t) = 0.5 * [(2*P1) + (-P0+P2)*t + (2*P0-5*P1+4*P2-P3)*t² + (-P0+3*P1-3*P2+P3)*t³]
                const x = 0.5 * (
                    (2 * p1.x) +
                    (-p0.x + p2.x) * tt +
                    (2 * p0.x - 5 * p1.x + 4 * p2.x - p3.x) * tt2 +
                    (-p0.x + 3 * p1.x - 3 * p2.x + p3.x) * tt3
                );

                const y = 0.5 * (
                    (2 * p1.y) +
                    (-p0.y + p2.y) * tt +
                    (2 * p0.y - 5 * p1.y + 4 * p2.y - p3.y) * tt2 +
                    (-p0.y + 3 * p1.y - 3 * p2.y + p3.y) * tt3
                );

                this.smoothedPoints.push({ x, y });
            }
        }

        // Sample at 32 band centers (only within stroke range)
        this.sampleAtBands();
    }

    /**
     * Sample smoothed curve at 32 logarithmic band centers.
     * Only updates bands within the X range of the current stroke,
     * preserving all other band values from previous strokes.
     */
    sampleAtBands() {
        if (this.smoothedPoints.length === 0) return;

        const bandFreqs = this.getBandFrequencies();

        // Determine the X range of the current stroke
        let strokeMinX = Infinity;
        let strokeMaxX = -Infinity;
        for (const point of this.smoothedPoints) {
            if (point.x < strokeMinX) strokeMinX = point.x;
            if (point.x > strokeMaxX) strokeMaxX = point.x;
        }

        for (let i = 0; i < this.numBands; i++) {
            const freq = bandFreqs[i];
            const x = this.freqToX(freq);

            // Only update bands within the stroke's X range
            if (x < strokeMinX || x > strokeMaxX) continue;

            // Find closest smoothed point
            let closestY = this.gainToY(this.curveData[i]);
            let minDist = Infinity;

            for (const point of this.smoothedPoints) {
                const dist = Math.abs(point.x - x);
                if (dist < minDist) {
                    minDist = dist;
                    closestY = point.y;
                }
            }

            // Convert Y to gain (-1 to +1)
            this.curveData[i] = this.yToGain(closestY);
        }
    }

    /**
     * Draw the freehand curve - always renders from curveData
     * so partial strokes are visually consistent
     */
    drawCurve() {
        this.drawDataCurve();
    }

    /**
     * Draw curve from band data
     */
    drawDataCurve() {
        const bandFreqs = this.getBandFrequencies();

        this.ctx.strokeStyle = this.accentColor;
        this.ctx.lineWidth = 2;
        this.ctx.beginPath();

        bandFreqs.forEach((freq, i) => {
            const x = this.freqToX(freq);
            const y = this.gainToY(this.curveData[i]);

            if (i === 0) {
                this.ctx.moveTo(x, y);
            } else {
                this.ctx.lineTo(x, y);
            }
        });

        this.ctx.stroke();

        // Draw sample points
        this.drawSamplePoints();
    }

    /**
     * Draw sample points at band centers
     */
    drawSamplePoints() {
        const bandFreqs = this.getBandFrequencies();

        this.ctx.fillStyle = this.accentColor;
        bandFreqs.forEach((freq, i) => {
            const x = this.freqToX(freq);
            const y = this.gainToY(this.curveData[i]);

            this.ctx.beginPath();
            this.ctx.arc(x, y, 2, 0, Math.PI * 2);
            this.ctx.fill();
        });
    }

    /**
     * Reset curve to flat, clearing all stroke data
     */
    resetCurve() {
        this.rawPoints = [];
        this.smoothedPoints = [];
        super.resetCurve();
    }
}
