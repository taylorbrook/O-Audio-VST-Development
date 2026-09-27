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
 * RotaryKnob Component
 *
 * Implements relative drag pattern for smooth knob control
 * Rotation range: -135° to +135° (270° total)
 *
 * v1.10.0 (R7): pointer capture, keyboard and ARIA, ported from O-ReverseDelay.
 * The container is the focusable role="slider"; .knob keeps the rotation
 * transform and the container keeps the hover scale, as before.
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

        // Callbacks, wired by bindKnobToParameter():
        //   onValueChange(norm)  — every drag move
        //   onGestureStart/End() — bracket a drag, so the host sees ONE gesture
        //   onNudge(dir)         — an arrow key, dir = ±1 (a full gesture itself)
        this.onValueChange = null;
        this.onGestureStart = null;
        this.onGestureEnd = null;
        this.onNudge = null;

        // Bind event handlers
        this.onPointerDown = this.onPointerDown.bind(this);
        this.onPointerMove = this.onPointerMove.bind(this);
        this.onPointerUp = this.onPointerUp.bind(this);
        this.onKeyDown = this.onKeyDown.bind(this);

        // Accessibility: focusable slider, named by its visible caption.
        this.container.setAttribute('tabindex', '0');
        this.container.setAttribute('role', 'slider');
        const caption = this.container.closest('.knob-wrapper')?.querySelector('.knob-label');
        if (caption) {
            if (!caption.id) caption.id = `${containerId}-label`;
            this.container.setAttribute('aria-labelledby', caption.id);
        }

        // Attach listeners
        this.container.addEventListener('pointerdown', this.onPointerDown);
        this.container.addEventListener('keydown', this.onKeyDown);
        // Text-selection suppression stays on mousedown, NOT pointerdown:
        // preventDefault on pointerdown would cancel the compatibility mousedown,
        // and the settings popover and preset menu dismiss on document mousedown.
        this.container.addEventListener('mousedown', (e) => e.preventDefault());
    }

    // Capture on the CONTAINER rather than listening on document (O-ReverseDelay
    // v1.7.2 WR-05). With document listeners and only a mouseup to end the drag,
    // a release outside the WebView, a host modal grab or a focus loss left the
    // drag latched: the knob followed the cursor with no button held. Capture
    // guarantees a terminating pointerup / pointercancel / lostpointercapture.
    onPointerDown(e) {
        if (e.button !== 0 || this.isDragging) return;
        this.isDragging = true;
        this.lastY = e.clientY;
        if (this.onGestureStart) this.onGestureStart();

        try { this.container.setPointerCapture(e.pointerId); } catch (_) { /* older backends */ }
        this.container.addEventListener('pointermove', this.onPointerMove);
        this.container.addEventListener('pointerup', this.onPointerUp);
        this.container.addEventListener('pointercancel', this.onPointerUp);
        this.container.addEventListener('lostpointercapture', this.onPointerUp);
    }

    onPointerMove(e) {
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
        e.preventDefault();
    }

    // Idempotent (the !isDragging early-return): up, cancel and lost-capture can
    // all fire for one release.
    onPointerUp(e) {
        if (!this.isDragging) return;

        this.isDragging = false;

        this.container.removeEventListener('pointermove', this.onPointerMove);
        this.container.removeEventListener('pointerup', this.onPointerUp);
        this.container.removeEventListener('pointercancel', this.onPointerUp);
        this.container.removeEventListener('lostpointercapture', this.onPointerUp);
        if (e && e.pointerId !== undefined) {
            try { this.container.releasePointerCapture(e.pointerId); } catch (_) { /* already released */ }
        }

        if (this.onGestureEnd) this.onGestureEnd();
    }

    onKeyDown(e) {
        let dir = 0;
        if (e.key === 'ArrowUp' || e.key === 'ArrowRight') dir = 1;
        else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') dir = -1;
        else return;
        if (!this.isDragging && this.onNudge) this.onNudge(dir);
        e.preventDefault();
    }

    updateVisuals() {
        // Rotate knob visual
        this.knob.style.transform = `rotate(${this.rotation}deg)`;

        // Update value display — the readout and the slider's spoken value are
        // one string, so a screen reader hears exactly what is printed.
        const text = this.formatValue(this.value);
        this.valueElement.textContent = text;
        this.container.setAttribute('aria-valuetext', text);
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
