/**
 * Ouaricon VU Meter - JavaScript UI Module
 *
 * Provides smooth animated VU meter displays with configurable ballistics.
 * Works with the C++ VUMeterBridge via evaluateJavascript calls.
 *
 * Display Styles:
 *   - LED Bar: Segmented LED-style meter
 *   - Smooth Bar: Continuous gradient bar
 *   - Needle: Analog VU needle meter
 *
 * Usage:
 *   import { LEDMeter, SmoothMeter, NeedleMeter } from './modules/vu-meter.js';
 *
 *   const inputMeter = new LEDMeter({
 *     container: document.getElementById('input-meter'),
 *     segments: 14,
 *     minDB: -60,
 *     maxDB: 3
 *   });
 *
 *   // Called from C++ via evaluateJavascript
 *   window.updateMeters = function(inputDB, outputDB) {
 *     inputMeter.setLevel(inputDB);
 *     outputMeter.setLevel(outputDB);
 *   };
 */


// ============================================================================
// Configuration Defaults
// ============================================================================

const DEFAULT_CONFIG = {
    minDB: -60,
    maxDB: 3,
    attackSpeed: 0.4,      // Fast attack
    releaseSpeed: 0.15,    // Slow release
    peakHoldMs: 1000,      // 1 second peak hold
    animationFps: 60       // Animation frame rate
};


// ============================================================================
// Base Meter Class
// ============================================================================

class BaseMeter {
    constructor(options = {}) {
        this.config = { ...DEFAULT_CONFIG, ...options };
        this.container = options.container;
        this.currentLevel = this.config.minDB;
        this.targetLevel = this.config.minDB;
        this.peakLevel = this.config.minDB;
        this.peakHoldTime = 0;
        this.lastFrameTime = performance.now();
        this.isAnimating = false;
    }

    /**
     * Set target level from C++.
     * @param {number} levelDB - Level in decibels
     */
    setLevel(levelDB) {
        this.targetLevel = Math.max(this.config.minDB, Math.min(this.config.maxDB, levelDB));

        // Update peak hold
        if (this.targetLevel > this.peakLevel) {
            this.peakLevel = this.targetLevel;
            this.peakHoldTime = performance.now();
        }

        if (!this.isAnimating) {
            this.startAnimation();
        }
    }

    /**
     * Normalize level to 0-1 range.
     */
    normalize(levelDB) {
        const range = this.config.maxDB - this.config.minDB;
        return (levelDB - this.config.minDB) / range;
    }

    /**
     * Start animation loop.
     */
    startAnimation() {
        this.isAnimating = true;
        this.animate();
    }

    /**
     * Animation loop with attack/release ballistics.
     */
    animate() {
        const now = performance.now();
        const deltaTime = (now - this.lastFrameTime) / 1000; // seconds
        this.lastFrameTime = now;

        // Apply ballistics
        const speed = this.currentLevel < this.targetLevel
            ? this.config.attackSpeed
            : this.config.releaseSpeed;

        // Exponential smoothing with time-based adjustment
        const factor = 1 - Math.exp(-speed * deltaTime * 60);
        this.currentLevel += (this.targetLevel - this.currentLevel) * factor;

        // Peak hold decay
        if (this.config.peakHoldMs > 0) {
            const peakAge = now - this.peakHoldTime;
            if (peakAge > this.config.peakHoldMs) {
                // Decay peak towards current level
                this.peakLevel += (this.config.minDB - this.peakLevel) * 0.05;
            }
        }

        // Update display
        this.render();

        // Continue animation if level is changing
        const delta = Math.abs(this.currentLevel - this.targetLevel);
        const peakDelta = Math.abs(this.peakLevel - this.config.minDB);

        if (delta > 0.01 || peakDelta > 0.01) {
            requestAnimationFrame(() => this.animate());
        } else {
            this.isAnimating = false;
        }
    }

    /**
     * Override in subclasses to render the meter.
     */
    render() {
        // Subclass implementation
    }
}


// ============================================================================
// LED Bar Meter
// ============================================================================

export class LEDMeter extends BaseMeter {
    constructor(options = {}) {
        super(options);

        this.segments = options.segments || 14;
        this.colors = options.colors || {
            low: '#8BA870',     // Green
            mid: '#6B8E4E',     // Darker green
            high: '#C9A27B',    // Amber
            inactive: '#8B7355' // Brown (off)
        };
        this.thresholds = options.thresholds || {
            mid: 0.5,   // 50% = mid color
            high: 0.85  // 85% = high/warning color
        };

        this.segmentElements = [];
        this.peakElement = null;

        this.buildDOM();
    }

    /**
     * Build LED segment DOM structure.
     */
    buildDOM() {
        if (!this.container) return;

        this.container.innerHTML = '';
        this.container.style.display = 'flex';
        this.container.style.flexDirection = 'column-reverse';
        this.container.style.gap = '2px';

        for (let i = 0; i < this.segments; i++) {
            const segment = document.createElement('div');
            segment.className = 'led-segment';
            segment.style.cssText = `
                width: 100%;
                height: ${100 / this.segments}%;
                background: ${this.colors.inactive};
                border-radius: 1px;
                transition: background 0.05s ease;
            `;
            this.container.appendChild(segment);
            this.segmentElements.push(segment);
        }
    }

    /**
     * Render LED meter display.
     */
    render() {
        const normalized = this.normalize(this.currentLevel);
        const peakNormalized = this.normalize(this.peakLevel);

        this.segmentElements.forEach((segment, index) => {
            const segmentThreshold = index / this.segments;
            const segmentCenter = (index + 0.5) / this.segments;

            // Clear previous state
            segment.style.background = this.colors.inactive;
            segment.style.boxShadow = 'none';

            // Active segments
            if (normalized > segmentThreshold) {
                let color;
                if (segmentCenter < this.thresholds.mid) {
                    color = this.colors.low;
                } else if (segmentCenter < this.thresholds.high) {
                    color = this.colors.mid;
                } else {
                    color = this.colors.high;
                }
                segment.style.background = color;
                segment.style.boxShadow = `0 0 4px ${color}`;
            }
            // Peak indicator
            else if (this.config.peakHoldMs > 0 && peakNormalized > segmentThreshold && peakNormalized <= (index + 1) / this.segments) {
                segment.style.background = this.colors.high;
            }
        });
    }
}


// ============================================================================
// Smooth Bar Meter
// ============================================================================

export class SmoothMeter extends BaseMeter {
    constructor(options = {}) {
        super(options);

        this.direction = options.direction || 'vertical'; // 'vertical' or 'horizontal'
        this.gradient = options.gradient || ['#8BA870', '#6B8E4E', '#C9A27B'];
        this.backgroundColor = options.backgroundColor || 'rgba(139, 115, 85, 0.2)';

        this.fillElement = null;
        this.peakMarker = null;

        this.buildDOM();
    }

    /**
     * Build smooth bar DOM structure.
     */
    buildDOM() {
        if (!this.container) return;

        this.container.innerHTML = '';
        this.container.style.position = 'relative';
        this.container.style.background = this.backgroundColor;
        this.container.style.overflow = 'hidden';

        // Fill bar
        this.fillElement = document.createElement('div');
        this.fillElement.style.cssText = `
            position: absolute;
            bottom: 0;
            left: 0;
            width: 100%;
            height: 0%;
            background: linear-gradient(to top, ${this.gradient.join(', ')});
            transition: none;
        `;
        this.container.appendChild(this.fillElement);

        // Peak marker
        if (this.config.peakHoldMs > 0) {
            this.peakMarker = document.createElement('div');
            this.peakMarker.style.cssText = `
                position: absolute;
                left: 0;
                width: 100%;
                height: 2px;
                background: ${this.gradient[this.gradient.length - 1]};
                bottom: 0;
            `;
            this.container.appendChild(this.peakMarker);
        }
    }

    /**
     * Render smooth bar display.
     */
    render() {
        const normalized = this.normalize(this.currentLevel);
        const peakNormalized = this.normalize(this.peakLevel);

        if (this.fillElement) {
            this.fillElement.style.height = `${normalized * 100}%`;
        }

        if (this.peakMarker) {
            this.peakMarker.style.bottom = `${peakNormalized * 100}%`;
        }
    }
}


// ============================================================================
// Needle Meter (Analog VU Style)
// ============================================================================

export class NeedleMeter extends BaseMeter {
    constructor(options = {}) {
        super(options);

        this.minAngle = options.minAngle || -45;  // degrees
        this.maxAngle = options.maxAngle || 45;   // degrees
        this.needleColor = options.needleColor || '#3C2F2F';

        this.needleElement = null;
        this.buildDOM();
    }

    /**
     * Build needle meter DOM structure.
     */
    buildDOM() {
        if (!this.container) return;

        // Needle (centered at bottom)
        this.needleElement = document.createElement('div');
        this.needleElement.style.cssText = `
            position: absolute;
            bottom: 10%;
            left: 50%;
            width: 3px;
            height: 70%;
            background: ${this.needleColor};
            transform-origin: bottom center;
            transform: translateX(-50%) rotate(${this.minAngle}deg);
            border-radius: 2px;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.3);
        `;
        this.container.appendChild(this.needleElement);
    }

    /**
     * Render needle display.
     */
    render() {
        const normalized = this.normalize(this.currentLevel);
        const angleRange = this.maxAngle - this.minAngle;
        const angle = this.minAngle + (normalized * angleRange);

        if (this.needleElement) {
            this.needleElement.style.transform = `translateX(-50%) rotate(${angle}deg)`;
        }
    }
}


// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Create a standard input/output meter pair.
 */
export function createMeterPair(inputContainer, outputContainer, options = {}) {
    const inputMeter = new LEDMeter({
        container: inputContainer,
        ...options
    });

    const outputMeter = new LEDMeter({
        container: outputContainer,
        ...options
    });

    // Register global update function
    window.updateMeters = function(inputDB, outputDB, ...aux) {
        inputMeter.setLevel(inputDB);
        outputMeter.setLevel(outputDB);
    };

    return { inputMeter, outputMeter };
}


/**
 * Register a custom update function that includes auxiliary values.
 */
export function registerMeterUpdate(meters, auxHandlers = []) {
    window.updateMeters = function(inputDB, outputDB, ...auxValues) {
        if (meters.input) meters.input.setLevel(inputDB);
        if (meters.output) meters.output.setLevel(outputDB);

        auxValues.forEach((value, index) => {
            if (auxHandlers[index]) {
                auxHandlers[index](value);
            }
        });
    };
}


// ============================================================================
// Global Exports for Non-Module Usage
// ============================================================================

if (typeof window !== 'undefined') {
    window.OuariconMeter = {
        LED: LEDMeter,
        Smooth: SmoothMeter,
        Needle: NeedleMeter,
        createPair: createMeterPair,
        registerUpdate: registerMeterUpdate
    };
}
