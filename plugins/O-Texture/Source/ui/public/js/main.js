/*
  ==============================================================================
    O-Texture - Main UI Logic
    Parameter binding, XY pad, animations, controls
  ==============================================================================
*/

import { getSliderState, getToggleState, getComboBoxState, getBackendResourceAddress } from './juce/index.js';

// ============================================================================
// State Management
// ============================================================================
let xState, yState, characterAState, characterBState, evolveState;
let brightnessState, mixState, sourceState, modeState, freezeState;

// XY pad animation
const canvas = document.getElementById('xy-pad');
const ctx = canvas ? canvas.getContext('2d') : null;
const trailPoints = [];
const MAX_TRAIL_LENGTH = 60; // ~2 seconds at 30fps
let lastFrameTime = 0;
const FRAME_INTERVAL = 1000 / 30; // 30fps cap
let isDraggingXY = false;

// ============================================================================
// Initialize after DOM loads
// ============================================================================
document.addEventListener('DOMContentLoaded', () => {
    // Setup canvas
    if (canvas) {
        const container = canvas.parentElement;
        canvas.width = container.clientWidth - 4; // Account for border
        canvas.height = container.clientHeight - 4;
    }

    // Initialize all relay states
    initializeRelays();

    // Bind UI controls
    bindModeToggle();
    bindXYPad();
    bindVerticalSliders();
    bindSourceSelector();
    bindKnobs();
    bindFreezeToggle();

    // Start animation loop
    if (canvas) {
        requestAnimationFrame(animationLoop);
    }
});

// ============================================================================
// Relay Initialization
// ============================================================================
function initializeRelays() {
    // 7 WebSliderRelays
    xState = getSliderState('xSlider');
    yState = getSliderState('ySlider');
    characterAState = getSliderState('characterASlider');
    characterBState = getSliderState('characterBSlider');
    evolveState = getSliderState('evolveSlider');
    brightnessState = getSliderState('brightnessSlider');
    mixState = getSliderState('mixSlider');

    // 2 WebComboBoxRelays
    sourceState = getComboBoxState('sourceCombo');
    modeState = getComboBoxState('modeCombo');

    // 1 WebToggleButtonRelay
    freezeState = getToggleState('freezeToggle');

    // Listen for backend property updates (initial state)
    sourceState.propertiesChangedEvent.addListener(() => updateSourceButtons());
    modeState.propertiesChangedEvent.addListener(() => updateModeButtons());

    // Listen for backend value changes (automation, presets)
    xState.valueChangedEvent.addListener(() => updateXYPadVisual());
    yState.valueChangedEvent.addListener(() => updateXYPadVisual());
    characterAState.valueChangedEvent.addListener(() => updateVerticalSlider('charA'));
    characterBState.valueChangedEvent.addListener(() => updateVerticalSlider('charB'));
    evolveState.valueChangedEvent.addListener(() => updateVerticalSlider('evolve'));
    brightnessState.valueChangedEvent.addListener(() => updateKnob('brightness'));
    mixState.valueChangedEvent.addListener(() => updateKnob('mix'));
    sourceState.valueChangedEvent.addListener(() => updateSourceButtons());
    modeState.valueChangedEvent.addListener(() => updateModeButtons());
    freezeState.valueChangedEvent.addListener(() => updateFreezeVisual());
}

// ============================================================================
// Mode Toggle (Generate | Transform)
// ============================================================================
function bindModeToggle() {
    const buttons = document.querySelectorAll('.mode-toggle button');
    buttons.forEach((btn, index) => {
        btn.addEventListener('click', () => {
            modeState.setChoiceIndex(index);
        });
    });
}

function updateModeButtons() {
    const buttons = document.querySelectorAll('.mode-toggle button');
    const currentIndex = modeState.getChoiceIndex();
    buttons.forEach((btn, index) => {
        if (index === currentIndex) {
            btn.classList.add('active');
        } else {
            btn.classList.remove('active');
        }
    });
}

// ============================================================================
// XY Pad with Orbital Trails
// ============================================================================
function bindXYPad() {
    if (!canvas) return;

    canvas.addEventListener('pointerdown', (e) => {
        isDraggingXY = true;
        canvas.setPointerCapture(e.pointerId);
        xState.sliderDragStarted();
        yState.sliderDragStarted();
        updateXYFromPointer(e);
    });

    canvas.addEventListener('pointermove', (e) => {
        if (!isDraggingXY) return;
        updateXYFromPointer(e);
    });

    canvas.addEventListener('pointerup', (e) => {
        if (!isDraggingXY) return;
        isDraggingXY = false;
        xState.sliderDragEnded();
        yState.sliderDragEnded();
    });

    canvas.addEventListener('pointercancel', () => {
        if (isDraggingXY) {
            isDraggingXY = false;
            xState.sliderDragEnded();
            yState.sliderDragEnded();
        }
    });
}

function updateXYFromPointer(e) {
    const rect = canvas.getBoundingClientRect();
    const normX = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
    const normY = Math.max(0, Math.min(1, 1.0 - (e.clientY - rect.top) / rect.height)); // Y inverted

    xState.setNormalisedValue(normX);
    yState.setNormalisedValue(normY);
}

function updateXYPadVisual() {
    // Visual update happens in animation loop
}

function animationLoop(timestamp) {
    if (timestamp - lastFrameTime >= FRAME_INTERVAL) {
        lastFrameTime = timestamp;

        const normX = xState.getNormalisedValue();
        const normY = yState.getNormalisedValue();

        // Add trail point if not frozen
        if (!freezeState.getValue()) {
            trailPoints.push({ x: normX, y: normY, age: 0 });
            if (trailPoints.length > MAX_TRAIL_LENGTH) {
                trailPoints.shift();
            }

            // Age all points
            for (const p of trailPoints) {
                p.age++;
            }
        }

        drawXYPad(normX, normY);
    }
    requestAnimationFrame(animationLoop);
}

function drawXYPad(normX, normY) {
    if (!ctx) return;

    const w = canvas.width;
    const h = canvas.height;

    // Clear
    ctx.clearRect(0, 0, w, h);

    // Draw trail
    for (let i = 0; i < trailPoints.length; i++) {
        const p = trailPoints[i];
        const alpha = 1.0 - (p.age / MAX_TRAIL_LENGTH);
        const radius = 2 + alpha * 3;

        ctx.beginPath();
        ctx.arc(p.x * w, (1 - p.y) * h, radius, 0, Math.PI * 2);
        ctx.fillStyle = `rgba(107, 142, 78, ${alpha * 0.8})`; // botanical green
        ctx.fill();
    }

    // Draw current cursor
    ctx.beginPath();
    ctx.arc(normX * w, (1 - normY) * h, 7, 0, Math.PI * 2);
    ctx.fillStyle = '#6B8E4E';
    ctx.fill();
    ctx.strokeStyle = '#4A6B35';
    ctx.lineWidth = 2;
    ctx.stroke();
}

// ============================================================================
// Vertical Sliders (Character A, B, Evolve)
// ============================================================================
function bindVerticalSliders() {
    bindVerticalSlider('charA', characterAState);
    bindVerticalSlider('charB', characterBState);
    bindVerticalSlider('evolve', evolveState);
}

function bindVerticalSlider(id, sliderState) {
    const slider = document.getElementById(`slider-${id}`);
    if (!slider) return;

    const track = slider.querySelector('.slider-track');
    const thumb = slider.querySelector('.slider-thumb');
    let isDragging = false;
    let startY, startValue;

    track.addEventListener('pointerdown', (e) => {
        isDragging = true;
        startY = e.clientY;
        startValue = sliderState.getNormalisedValue();
        track.setPointerCapture(e.pointerId);
        sliderState.sliderDragStarted();
        e.preventDefault();
    });

    track.addEventListener('pointermove', (e) => {
        if (!isDragging) return;
        const deltaY = startY - e.clientY; // Up = positive
        const sensitivity = 1.0 / track.clientHeight;
        const newValue = Math.max(0, Math.min(1, startValue + deltaY * sensitivity));
        sliderState.setNormalisedValue(newValue);
    });

    track.addEventListener('pointerup', () => {
        if (!isDragging) return;
        isDragging = false;
        sliderState.sliderDragEnded();
    });

    track.addEventListener('pointercancel', () => {
        if (isDragging) {
            isDragging = false;
            sliderState.sliderDragEnded();
        }
    });

    // Double-click to reset
    track.addEventListener('dblclick', () => {
        sliderState.sliderDragStarted();
        sliderState.setNormalisedValue(0.5); // Default center
        sliderState.sliderDragEnded();
    });

    // Initial position
    updateVerticalSlider(id);
}

function updateVerticalSlider(id) {
    const slider = document.getElementById(`slider-${id}`);
    if (!slider) return;

    const thumb = slider.querySelector('.slider-thumb');
    const valueDisplay = slider.querySelector('.value');
    let sliderState;

    if (id === 'charA') sliderState = characterAState;
    else if (id === 'charB') sliderState = characterBState;
    else if (id === 'evolve') sliderState = evolveState;

    if (!sliderState) return;

    const normValue = sliderState.getNormalisedValue();
    const track = slider.querySelector('.slider-track');
    const thumbHeight = thumb.offsetHeight;
    const trackHeight = track.clientHeight;
    const thumbY = (1 - normValue) * (trackHeight - thumbHeight);

    thumb.style.top = `${thumbY}px`;

    if (valueDisplay) {
        const scaledValue = sliderState.getScaledValue();
        valueDisplay.textContent = scaledValue.toFixed(2);
    }
}

// ============================================================================
// Source Selector (6 icon buttons)
// ============================================================================
function bindSourceSelector() {
    const buttons = document.querySelectorAll('.source-button');
    buttons.forEach((btn, index) => {
        btn.addEventListener('click', () => {
            sourceState.setChoiceIndex(index);
        });
    });
}

function updateSourceButtons() {
    const buttons = document.querySelectorAll('.source-button');
    const currentIndex = sourceState.getChoiceIndex();
    buttons.forEach((btn, index) => {
        if (index === currentIndex) {
            btn.classList.add('active');
        } else {
            btn.classList.remove('active');
        }
    });
}

// ============================================================================
// Rotary Knobs (Brightness, Mix)
// ============================================================================
function bindKnobs() {
    bindKnob('brightness', brightnessState);
    bindKnob('mix', mixState);
}

function bindKnob(id, sliderState) {
    const knob = document.getElementById(`knob-${id}`);
    if (!knob) return;

    let isDragging = false;
    let startY, startValue;

    knob.addEventListener('pointerdown', (e) => {
        isDragging = true;
        startY = e.clientY;
        startValue = sliderState.getNormalisedValue();
        knob.setPointerCapture(e.pointerId);
        sliderState.sliderDragStarted();
        e.preventDefault();
    });

    knob.addEventListener('pointermove', (e) => {
        if (!isDragging) return;
        const deltaY = startY - e.clientY;
        const sensitivity = 0.005; // Slower rotation
        const newValue = Math.max(0, Math.min(1, startValue + deltaY * sensitivity));
        sliderState.setNormalisedValue(newValue);
    });

    knob.addEventListener('pointerup', () => {
        if (!isDragging) return;
        isDragging = false;
        sliderState.sliderDragEnded();
    });

    knob.addEventListener('pointercancel', () => {
        if (isDragging) {
            isDragging = false;
            sliderState.sliderDragEnded();
        }
    });

    // Double-click to reset
    knob.addEventListener('dblclick', () => {
        sliderState.sliderDragStarted();
        sliderState.setNormalisedValue(0.5); // Default center
        sliderState.sliderDragEnded();
    });

    // Initial rotation
    updateKnob(id);
}

function updateKnob(id) {
    const knob = document.getElementById(`knob-${id}`);
    if (!knob) return;

    const indicator = knob.querySelector('.knob-indicator');
    const valueDisplay = knob.parentElement.querySelector('.knob-value');
    let sliderState;

    if (id === 'brightness') sliderState = brightnessState;
    else if (id === 'mix') sliderState = mixState;

    if (!sliderState) return;

    const normValue = sliderState.getNormalisedValue();
    // Rotation: 0 = -135deg, 1 = +135deg
    const angle = -135 + normValue * 270;
    indicator.style.transform = `translate(-50%, -100%) rotate(${angle}deg)`;

    if (valueDisplay) {
        const scaledValue = sliderState.getScaledValue();
        valueDisplay.textContent = scaledValue.toFixed(2);
    }
}

// ============================================================================
// Freeze Toggle
// ============================================================================
function bindFreezeToggle() {
    const button = document.getElementById('freeze-button');
    if (!button) return;

    button.addEventListener('click', () => {
        freezeState.setValue(!freezeState.getValue());
    });
}

function updateFreezeVisual() {
    const button = document.getElementById('freeze-button');
    if (!button) return;

    const isActive = freezeState.getValue();

    if (isActive) {
        button.classList.add('active');
        canvas?.classList.add('frozen');
    } else {
        button.classList.remove('active');
        canvas?.classList.remove('frozen');
    }
}
