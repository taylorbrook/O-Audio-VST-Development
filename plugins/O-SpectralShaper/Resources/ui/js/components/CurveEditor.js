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

        // Curve data (32 values, -1.0 to +1.0)
        this.curveData = new Array(this.numBands).fill(0.0);

        // Callback for curve updates
        this.onCurveChange = null;

        // Setup canvas size (may need to wait for layout)
        this.resizeCanvas();

        // Handle window resize
        window.addEventListener('resize', () => this.handleResize());

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
        const logMin = Math.log(this.minFreq);
        const logMax = Math.log(this.maxFreq);
        const logFreq = Math.log(freq);
        const normalized = (logFreq - logMin) / (logMax - logMin);
        return normalized * this.width;
    }

    /**
     * Convert X pixel coordinate to frequency (logarithmic)
     */
    xToFreq(x) {
        const normalized = x / this.width;
        const logMin = Math.log(this.minFreq);
        const logMax = Math.log(this.maxFreq);
        const logFreq = logMin + normalized * (logMax - logMin);
        return Math.exp(logFreq);
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
        const bands = [];
        const logMin = Math.log(this.minFreq);
        const logMax = Math.log(this.maxFreq);
        for (let i = 0; i < this.numBands; i++) {
            const t = i / (this.numBands - 1);
            const logFreq = logMin + t * (logMax - logMin);
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
     * Main render loop
     */
    render() {
        // Clear canvas
        this.ctx.clearRect(0, 0, this.width, this.height);

        // Draw grid
        this.drawGrid();

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
     * Notify listeners of curve change
     */
    notifyCurveChange() {
        if (this.onCurveChange) {
            this.onCurveChange(this.getCurveData());
        }
    }
}
