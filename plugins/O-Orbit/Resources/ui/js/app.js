/*
   This file is part of O-Orbit, an Ouaricon Audio plugin.
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
// O-Orbit App.js - Parameter Binding + Orbital Visualizer + Speaker Editor
// Phase 3.1: WebView UI + 17 Parameter Controls
// Phase 3.2: Canvas Orbital Visualizer
// Phase 3.3: Speaker Layout Editor + File I/O

import { getSliderState, getComboBoxState, getToggleState, getNativeFunction } from './juce/index.js';

// ─── Motion State ───────────────────────────────────────────────

let motionState = { azL: 0, elL: 0, azR: 0, elR: 0, dist: 1, split: false };
let speakers = [];  // Current speaker positions from backend
let trailL = [];    // Circular buffer of L source positions
let trailR = [];    // Circular buffer of R source positions
const TRAIL_LENGTH = 120; // 2 seconds at 60fps

// ─── View Mode & Editor State ───────────────────────────────────

let viewMode = 'motion'; // 'motion' or 'editor'
let hoveredSpeakerIndex = -1;
let draggingSpeakerIndex = -1;
let canvasRef = null;
let canvasCtxRef = null;

// ─── Initialization ─────────────────────────────────────────────

document.addEventListener('DOMContentLoaded', () => {
    initializeParameters();
    initializeVisualizer();
});

function initializeParameters() {
    // Slider parameters (11 knobs)
    bindKnob('speed', 'Hz');
    bindKnob('width', '\u00B0');
    bindKnob('depth', '%');
    bindKnob('tilt', '\u00B0');
    bindKnob('phase', '\u00B0');
    bindKnob('elevation_range', '\u00B0');
    bindKnob('distance', 'm');
    bindKnob('air_absorption', '%');
    bindKnob('center_diverge', '%');
    bindKnob('lr_offset', '\u00B0');
    bindKnob('mix', '%');

    // Choice parameters (5 dropdowns)
    bindDropdown('path');
    bindDropdown('tempo_sync');
    bindDropdown('speaker_layout');
    bindDropdown('attenuation_curve');
    bindDropdown('source_mode');

    // Bool parameter (1 toggle)
    bindToggle('elevation_enable');
}

// ─── Knob Binding ───────────────────────────────────────────────

function bindKnob(paramId, unit) {
    const knobElement = document.getElementById(paramId);
    const valueDisplay = document.getElementById(paramId + '-value');

    if (!knobElement) return;

    const state = getSliderState(paramId);

    function updateFromState() {
        const norm = state.getNormalisedValue();
        updateKnobRotation(knobElement, norm);
        if (valueDisplay) {
            const scaled = state.getScaledValue();
            valueDisplay.textContent = formatValue(scaled, unit);
        }
    }

    updateFromState();
    state.valueChangedEvent.addListener(updateFromState);
    state.propertiesChangedEvent.addListener(updateFromState);

    let isDragging = false;
    let lastY = 0;

    knobElement.addEventListener('mousedown', (e) => {
        e.preventDefault();
        isDragging = true;
        lastY = e.clientY;
        knobElement.style.cursor = 'grabbing';
        state.sliderDragStarted();
    });

    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const delta = lastY - e.clientY;
        lastY = e.clientY;
        const normalizedDelta = delta / 150.0;
        let newNorm = Math.max(0, Math.min(1, state.getNormalisedValue() + normalizedDelta));
        state.setNormalisedValue(newNorm);
    });

    document.addEventListener('mouseup', () => {
        if (isDragging) {
            isDragging = false;
            knobElement.style.cursor = 'pointer';
            state.sliderDragEnded();
        }
    });

    knobElement.addEventListener('dblclick', () => {
        state.sliderDragStarted();
        state.setNormalisedValue(0.5);
        state.sliderDragEnded();
    });
}

function updateKnobRotation(knobElement, normalized) {
    const rotation = -135 + (normalized * 270);
    knobElement.style.transform = 'rotate(' + rotation + 'deg)';
}

function formatValue(scaled, unit) {
    let formatted;
    const abs = Math.abs(scaled);
    if (abs < 10) formatted = scaled.toFixed(2);
    else if (abs < 100) formatted = scaled.toFixed(1);
    else formatted = Math.round(scaled).toString();
    return formatted + ' ' + unit;
}

// ─── Dropdown Binding ───────────────────────────────────────────

function bindDropdown(paramId) {
    const dropdown = document.getElementById(paramId);
    if (!dropdown) return;

    const state = getComboBoxState(paramId);

    function updateFromState() {
        dropdown.selectedIndex = state.getChoiceIndex();
    }

    updateFromState();
    state.valueChangedEvent.addListener(updateFromState);
    state.propertiesChangedEvent.addListener(updateFromState);

    dropdown.addEventListener('change', () => {
        state.setChoiceIndex(dropdown.selectedIndex);
    });
}

// ─── Toggle Binding ─────────────────────────────────────────────

function bindToggle(paramId) {
    const checkbox = document.getElementById(paramId);
    if (!checkbox) return;

    const label = checkbox.nextElementSibling;
    const state = getToggleState(paramId);

    function updateFromState() {
        const val = state.getValue();
        checkbox.checked = val;
        if (label) label.textContent = val ? 'On' : 'Off';
    }

    updateFromState();
    state.valueChangedEvent.addListener(updateFromState);

    checkbox.addEventListener('change', () => {
        state.setValue(checkbox.checked);
        if (label) label.textContent = checkbox.checked ? 'On' : 'Off';
    });
}

// ─── Orbital Visualizer (Phase 3.2) ────────────────────────────

function initializeVisualizer() {
    const canvas = document.getElementById('visualizer-canvas');
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    canvasRef = canvas;
    canvasCtxRef = ctx;

    // High-DPI setup
    function resizeCanvas() {
        const dpr = window.devicePixelRatio || 1;
        const rect = canvas.parentElement.getBoundingClientRect();
        canvas.width = rect.width * dpr;
        canvas.height = rect.height * dpr;
        ctx.scale(dpr, dpr);
    }
    resizeCanvas();

    // Listen for motion updates from C++ timer (30Hz)
    window.__JUCE__.backend.addEventListener('motionUpdate', (data) => {
        if (typeof data === 'string') {
            try { motionState = JSON.parse(data); } catch (e) {}
        } else if (typeof data === 'object') {
            motionState = data;
        }
    });

    // Fetch speaker layout (and refresh when speaker_layout param changes)
    const getSpeakerLayout = getNativeFunction('getSpeakerLayout');

    window._refreshSpeakers = function() {
        getSpeakerLayout().then((result) => {
            if (Array.isArray(result)) {
                speakers = result;
            }
        });
    };

    window._refreshSpeakers();

    // Re-fetch speakers when layout parameter changes
    const layoutState = getComboBoxState('speaker_layout');
    layoutState.valueChangedEvent.addListener(() => {
        setTimeout(window._refreshSpeakers, 100);
    });

    // ─── View Toggle ────────────────────────────────────────────
    initializeViewToggle();

    // ─── Speaker Editor Interactions ────────────────────────────
    initializeEditorInteractions(canvas);

    // ─── Preset & File Buttons ──────────────────────────────────
    initializeEditorButtons();

    // ─── Downmix Badge Polling ──────────────────────────────────
    initializeDownmixBadge();

    // 60fps render loop
    function renderLoop() {
        if (viewMode === 'motion') {
            drawMotionFrame(canvas, ctx);
        } else {
            drawEditorFrame(canvas, ctx);
        }
        requestAnimationFrame(renderLoop);
    }
    requestAnimationFrame(renderLoop);
}

function drawMotionFrame(canvas, ctx) {
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.width / dpr;
    const h = canvas.height / dpr;
    const centerX = w / 2;
    const centerY = h / 2;
    const radius = Math.min(w, h) * 0.42;

    // Clear
    ctx.clearRect(0, 0, w, h);

    // Background plate
    ctx.fillStyle = 'rgba(139, 115, 85, 0.06)';
    ctx.fillRect(0, 0, w, h);

    // Draw grid circles
    ctx.strokeStyle = 'rgba(139, 115, 85, 0.15)';
    ctx.lineWidth = 0.5;
    for (let i = 1; i <= 3; i++) {
        ctx.beginPath();
        ctx.arc(centerX, centerY, radius * (i / 3), 0, Math.PI * 2);
        ctx.stroke();
    }

    // Draw crosshairs
    ctx.beginPath();
    ctx.moveTo(centerX, centerY - radius);
    ctx.lineTo(centerX, centerY + radius);
    ctx.moveTo(centerX - radius, centerY);
    ctx.lineTo(centerX + radius, centerY);
    ctx.stroke();

    // "Front" label
    ctx.fillStyle = 'rgba(60, 47, 47, 0.4)';
    ctx.font = '9px Garamond, serif';
    ctx.textAlign = 'center';
    ctx.fillText('FRONT', centerX, centerY - radius - 5);

    // Draw speaker icons
    drawSpeakers(ctx, centerX, centerY, radius);

    // Convert azimuth to canvas coordinates
    // Convention: azimuth 0=front (top), positive=clockwise on screen
    // SpeakerLayout.h: 0=front, +90=left (counter-clockwise)
    // So we negate azimuth for canvas (screen clockwise = positive angle)
    function azToCanvas(azDeg, dist) {
        const azRad = (-azDeg) * Math.PI / 180;
        const r = Math.min(dist / 3, 1) * radius; // Normalize distance to radius
        return {
            x: centerX + r * Math.sin(azRad),
            y: centerY - r * Math.cos(azRad)
        };
    }

    // Update trail buffers
    const posL = azToCanvas(motionState.azL, motionState.dist);
    trailL.push({ x: posL.x, y: posL.y });
    if (trailL.length > TRAIL_LENGTH) trailL.shift();

    if (motionState.split) {
        const posR = azToCanvas(motionState.azR, motionState.dist);
        trailR.push({ x: posR.x, y: posR.y });
        if (trailR.length > TRAIL_LENGTH) trailR.shift();
    } else {
        trailR = [];
    }

    // Draw trails
    drawTrail(ctx, trailL, '#8B7355', 0.6);  // Warm brown trail for L
    if (motionState.split) {
        drawTrail(ctx, trailR, '#C9A27B', 0.5);  // Amber trail for R
    }

    // Draw source dots
    drawSourceDot(ctx, posL.x, posL.y, '#8BA870', 'L');  // Green for L

    if (motionState.split) {
        const posR = azToCanvas(motionState.azR, motionState.dist);
        drawSourceDot(ctx, posR.x, posR.y, '#C9A27B', 'R');  // Amber for R
    }
}

function drawSpeakers(ctx, cx, cy, radius) {
    for (const spk of speakers) {
        if (spk.isLFE) continue; // Don't draw LFE speakers

        const azRad = (-spk.azimuth) * Math.PI / 180;
        const r = radius * 0.95;
        const x = cx + r * Math.sin(azRad);
        const y = cy - r * Math.cos(azRad);

        // Speaker icon: small circle
        ctx.beginPath();
        ctx.arc(x, y, 6, 0, Math.PI * 2);
        ctx.fillStyle = '#EBD9C7';
        ctx.fill();
        ctx.strokeStyle = '#8B7355';
        ctx.lineWidth = 1.5;
        ctx.stroke();

        // Label
        ctx.fillStyle = '#5C4033';
        ctx.font = '8px Garamond, serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(spk.label, x, y);
    }
}

function drawTrail(ctx, trail, color, maxOpacity) {
    if (trail.length < 2) return;

    for (let i = 1; i < trail.length; i++) {
        const opacity = (i / trail.length) * maxOpacity;
        const lineWidth = (i / trail.length) * 2.5 + 0.5;

        ctx.beginPath();
        ctx.moveTo(trail[i - 1].x, trail[i - 1].y);
        ctx.lineTo(trail[i].x, trail[i].y);
        ctx.strokeStyle = color;
        ctx.globalAlpha = opacity;
        ctx.lineWidth = lineWidth;
        ctx.lineCap = 'round';
        ctx.stroke();
    }
    ctx.globalAlpha = 1.0;
}

function drawSourceDot(ctx, x, y, color, label) {
    // Radial gradient glow
    const gradient = ctx.createRadialGradient(x, y, 0, x, y, 12);
    gradient.addColorStop(0, color);
    gradient.addColorStop(0.5, color + '80');
    gradient.addColorStop(1, color + '00');

    ctx.beginPath();
    ctx.arc(x, y, 12, 0, Math.PI * 2);
    ctx.fillStyle = gradient;
    ctx.fill();

    // Core dot
    ctx.beginPath();
    ctx.arc(x, y, 4, 0, Math.PI * 2);
    ctx.fillStyle = color;
    ctx.fill();
    ctx.strokeStyle = '#FFF8DC';
    ctx.lineWidth = 1;
    ctx.stroke();

    // Label below dot (only in split mode)
    if (label) {
        ctx.fillStyle = color;
        ctx.font = 'bold 8px Garamond, serif';
        ctx.textAlign = 'center';
        ctx.fillText(label, x, y + 18);
    }
}

// ─── View Toggle (Phase 3.3) ────────────────────────────────────

function initializeViewToggle() {
    const toggleBtn = document.getElementById('view-toggle');
    const toolbar = document.getElementById('editor-toolbar');
    if (!toggleBtn) return;

    toggleBtn.addEventListener('click', () => {
        if (viewMode === 'motion') {
            viewMode = 'editor';
            toggleBtn.textContent = 'Speaker Editor';
            if (toolbar) toolbar.classList.add('visible');
            // Clear motion trails when switching
            trailL = [];
            trailR = [];
        } else {
            viewMode = 'motion';
            toggleBtn.textContent = 'Motion View';
            if (toolbar) toolbar.classList.remove('visible');
        }
    });
}

// ─── Speaker Editor Drawing ─────────────────────────────────────

function drawEditorFrame(canvas, ctx) {
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.width / dpr;
    const h = canvas.height / dpr;
    const centerX = w / 2;
    const centerY = (h - 32) / 2; // Account for toolbar
    const radius = Math.min(w, h - 32) * 0.40;

    ctx.clearRect(0, 0, w, h);

    // Background plate
    ctx.fillStyle = 'rgba(139, 115, 85, 0.06)';
    ctx.fillRect(0, 0, w, h);

    // Draw grid circles
    ctx.strokeStyle = 'rgba(139, 115, 85, 0.15)';
    ctx.lineWidth = 0.5;
    for (let i = 1; i <= 3; i++) {
        ctx.beginPath();
        ctx.arc(centerX, centerY, radius * (i / 3), 0, Math.PI * 2);
        ctx.stroke();
    }

    // Draw crosshairs
    ctx.beginPath();
    ctx.moveTo(centerX, centerY - radius);
    ctx.lineTo(centerX, centerY + radius);
    ctx.moveTo(centerX - radius, centerY);
    ctx.lineTo(centerX + radius, centerY);
    ctx.stroke();

    // Labels
    ctx.fillStyle = 'rgba(60, 47, 47, 0.4)';
    ctx.font = '9px Garamond, serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'alphabetic';
    ctx.fillText('FRONT', centerX, centerY - radius - 5);
    ctx.fillText('REAR', centerX, centerY + radius + 12);

    // Draw interactive speaker icons
    for (let i = 0; i < speakers.length; i++) {
        const spk = speakers[i];
        if (spk.isLFE) continue;

        const azRad = (-spk.azimuth) * Math.PI / 180;
        const r = radius * 0.85;
        const x = centerX + r * Math.sin(azRad);
        const y = centerY - r * Math.cos(azRad);

        const isHovered = (i === hoveredSpeakerIndex);
        const isDragging = (i === draggingSpeakerIndex);
        const spkRadius = isHovered || isDragging ? 10 : 8;

        // Speaker icon
        ctx.beginPath();
        ctx.arc(x, y, spkRadius, 0, Math.PI * 2);
        ctx.fillStyle = isDragging ? '#8BA870' : (isHovered ? '#D4C5A9' : '#EBD9C7');
        ctx.fill();
        ctx.strokeStyle = isDragging ? '#5C8A3E' : '#8B7355';
        ctx.lineWidth = isDragging ? 2.5 : (isHovered ? 2 : 1.5);
        ctx.stroke();

        // Label
        ctx.fillStyle = '#5C4033';
        ctx.font = (isHovered ? 'bold ' : '') + '9px Garamond, serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(spk.label, x, y);
    }

    // Instructions
    ctx.fillStyle = 'rgba(60, 47, 47, 0.3)';
    ctx.font = '9px Garamond, serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'alphabetic';
    ctx.fillText('Drag to move \u2022 Click to add \u2022 Right-click to remove', centerX, h - 38);
}

// ─── Speaker Editor Interactions ────────────────────────────────

function initializeEditorInteractions(canvas) {
    const addSpeaker = getNativeFunction('addSpeaker');
    const removeSpeaker = getNativeFunction('removeSpeaker');
    const moveSpeaker = getNativeFunction('moveSpeaker');

    function getCanvasGeometry() {
        const dpr = window.devicePixelRatio || 1;
        const w = canvas.width / dpr;
        const h = canvas.height / dpr;
        const centerX = w / 2;
        const centerY = (h - 32) / 2;
        const radius = Math.min(w, h - 32) * 0.40;
        return { centerX, centerY, radius };
    }

    function canvasToAzimuth(mouseX, mouseY) {
        const { centerX, centerY } = getCanvasGeometry();
        const dx = mouseX - centerX;
        const dy = centerY - mouseY;
        // atan2 gives angle from positive Y axis, negate for our convention
        let az = -Math.atan2(dx, dy) * 180 / Math.PI;
        return az;
    }

    function findSpeakerAt(mouseX, mouseY) {
        const { centerX, centerY, radius } = getCanvasGeometry();
        let closest = -1;
        let closestDist = 15; // Max pixel distance to register hit

        for (let i = 0; i < speakers.length; i++) {
            if (speakers[i].isLFE) continue;
            const azRad = (-speakers[i].azimuth) * Math.PI / 180;
            const r = radius * 0.85;
            const x = centerX + r * Math.sin(azRad);
            const y = centerY - r * Math.cos(azRad);
            const dist = Math.sqrt((mouseX - x) ** 2 + (mouseY - y) ** 2);
            if (dist < closestDist) {
                closestDist = dist;
                closest = i;
            }
        }
        return closest;
    }

    function getMousePos(e) {
        const rect = canvas.getBoundingClientRect();
        return { x: e.clientX - rect.left, y: e.clientY - rect.top };
    }

    canvas.addEventListener('mousemove', (e) => {
        if (viewMode !== 'editor') return;
        const pos = getMousePos(e);

        if (draggingSpeakerIndex >= 0) {
            const az = canvasToAzimuth(pos.x, pos.y);
            const el = speakers[draggingSpeakerIndex].elevation;
            moveSpeaker(draggingSpeakerIndex, az, el).then(() => {
                setTimeout(window._refreshSpeakers, 50);
            });
            return;
        }

        hoveredSpeakerIndex = findSpeakerAt(pos.x, pos.y);
        canvas.style.cursor = hoveredSpeakerIndex >= 0 ? 'grab' : 'crosshair';
    });

    canvas.addEventListener('mousedown', (e) => {
        if (viewMode !== 'editor') return;
        if (e.button !== 0) return; // Left click only
        const pos = getMousePos(e);
        const hit = findSpeakerAt(pos.x, pos.y);

        if (hit >= 0) {
            draggingSpeakerIndex = hit;
            canvas.style.cursor = 'grabbing';
            e.preventDefault();
        }
    });

    canvas.addEventListener('mouseup', (e) => {
        if (viewMode !== 'editor') return;
        if (e.button !== 0) return;

        if (draggingSpeakerIndex >= 0) {
            draggingSpeakerIndex = -1;
            canvas.style.cursor = hoveredSpeakerIndex >= 0 ? 'grab' : 'crosshair';
            return;
        }

        // Click on empty space: add speaker
        const pos = getMousePos(e);
        const hit = findSpeakerAt(pos.x, pos.y);
        if (hit < 0) {
            const az = canvasToAzimuth(pos.x, pos.y);
            const label = 'S' + (speakers.length + 1);
            addSpeaker(az, 0, 1, label).then(() => {
                setTimeout(window._refreshSpeakers, 100);
            });
        }
    });

    canvas.addEventListener('contextmenu', (e) => {
        if (viewMode !== 'editor') return;
        e.preventDefault();
        const pos = getMousePos(e);
        const hit = findSpeakerAt(pos.x, pos.y);
        if (hit >= 0) {
            removeSpeaker(hit).then(() => {
                setTimeout(window._refreshSpeakers, 100);
            });
        }
    });

    canvas.addEventListener('mouseleave', () => {
        hoveredSpeakerIndex = -1;
        draggingSpeakerIndex = -1;
    });
}

// ─── Preset & File Buttons ──────────────────────────────────────

function initializeEditorButtons() {
    const layoutState = getComboBoxState('speaker_layout');
    const exportLayout = getNativeFunction('exportLayout');
    const importLayout = getNativeFunction('importLayout');

    // Preset buttons
    document.querySelectorAll('.preset-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const preset = parseInt(btn.dataset.preset, 10);
            layoutState.setChoiceIndex(preset);
            setTimeout(window._refreshSpeakers, 150);
        });
    });

    // Export button
    const exportBtn = document.getElementById('export-btn');
    if (exportBtn) {
        exportBtn.addEventListener('click', () => {
            exportLayout();
        });
    }

    // Import button
    const importBtn = document.getElementById('import-btn');
    if (importBtn) {
        importBtn.addEventListener('click', () => {
            importLayout().then((result) => {
                if (Array.isArray(result)) {
                    speakers = result;
                } else {
                    setTimeout(window._refreshSpeakers, 100);
                }
            });
        });
    }
}

// ─── Downmix Badge ──────────────────────────────────────────────

function initializeDownmixBadge() {
    const getDownmixStatus = getNativeFunction('getDownmixStatus');
    const badge = document.getElementById('downmix-badge');
    if (!badge) return;

    const layoutNames = ['Stereo', 'Quad', '5.1', '7.1', '5.1.4', '7.1.4', 'Hex', 'Oct'];

    function updateBadge() {
        getDownmixStatus().then((status) => {
            if (status && status.active) {
                badge.textContent = status.sourceChannels + 'ch \u2192 ' + status.targetChannels + 'ch';
                badge.classList.add('active');
            } else {
                badge.classList.remove('active');
            }
        });
    }

    // Poll every 2 seconds
    updateBadge();
    setInterval(updateBadge, 2000);
}
