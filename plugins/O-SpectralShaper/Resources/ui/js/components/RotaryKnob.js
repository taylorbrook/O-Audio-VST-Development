/**
 * RotaryKnob Component
 *
 * Implements relative drag pattern for smooth knob control
 * Rotation range: -135° to +135° (270° total)
 */

export class RotaryKnob {
    constructor(containerId, valueElementId, config = {}) {
        this.container = document.getElementById(containerId);
        this.knob = this.container.querySelector('.knob');
        this.valueElement = document.getElementById(valueElementId);

        // Configuration
        this.minAngle = -135;
        this.maxAngle = 135;
        this.sensitivity = config.sensitivity || 0.5;
        this.formatValue = config.formatValue || ((v) => `${Math.round(v * 100)}%`);

        // State
        this.isDragging = false;
        this.lastY = 0;
        this.rotation = 0; // Current rotation in degrees (-135 to +135)
        this.value = 0; // Normalized value (0.0 to 1.0)

        // Bind event handlers
        this.onMouseDown = this.onMouseDown.bind(this);
        this.onMouseMove = this.onMouseMove.bind(this);
        this.onMouseUp = this.onMouseUp.bind(this);

        // Attach listeners
        this.container.addEventListener('mousedown', this.onMouseDown);

        // Callback for value changes
        this.onValueChange = null;
    }

    onMouseDown(e) {
        e.preventDefault();
        this.isDragging = true;
        this.lastY = e.clientY;

        // Attach global listeners
        document.addEventListener('mousemove', this.onMouseMove);
        document.addEventListener('mouseup', this.onMouseUp);

        // Visual feedback
        this.container.style.cursor = 'ns-resize';
    }

    onMouseMove(e) {
        if (!this.isDragging) return;

        // Calculate delta from LAST frame (relative drag)
        const deltaY = this.lastY - e.clientY;

        // Update rotation
        this.rotation += deltaY * this.sensitivity;
        this.rotation = Math.max(this.minAngle, Math.min(this.maxAngle, this.rotation));

        // Convert rotation to normalized value (0.0 to 1.0)
        const range = this.maxAngle - this.minAngle;
        this.value = (this.rotation - this.minAngle) / range;

        // Update visuals
        this.updateVisuals();

        // Notify listeners
        if (this.onValueChange) {
            this.onValueChange(this.value);
        }

        // Store for next frame
        this.lastY = e.clientY;
    }

    onMouseUp(e) {
        if (!this.isDragging) return;

        this.isDragging = false;

        // Remove global listeners
        document.removeEventListener('mousemove', this.onMouseMove);
        document.removeEventListener('mouseup', this.onMouseUp);

        // Reset cursor
        this.container.style.cursor = 'ns-resize';
    }

    updateVisuals() {
        // Rotate knob visual
        this.knob.style.transform = `rotate(${this.rotation}deg)`;

        // Update value display
        this.valueElement.textContent = this.formatValue(this.value);
    }

    /**
     * Set normalized value (0.0 to 1.0) from external source (e.g., JUCE automation)
     */
    setValue(normalizedValue) {
        this.value = Math.max(0, Math.min(1, normalizedValue));

        // Convert to rotation
        const range = this.maxAngle - this.minAngle;
        this.rotation = this.minAngle + (this.value * range);

        this.updateVisuals();
    }

    /**
     * Get current normalized value
     */
    getValue() {
        return this.value;
    }
}
