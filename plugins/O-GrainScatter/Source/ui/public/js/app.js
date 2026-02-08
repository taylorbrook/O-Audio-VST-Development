// O-GrainScatter — Parameter binding, grain visualization, Euclidean circle
// JUCE 8 WebView integration

import * as Juce from './juce/index.js';

    // ════════════════════════════════════════════════════════════════════
    // Knob drag system
    // ════════════════════════════════════════════════════════════════════

    const SENSITIVITY = 0.005;
    const ANGLE_MIN = -140;
    const ANGLE_MAX = 140;
    const ANGLE_RANGE = ANGLE_MAX - ANGLE_MIN;

    function setupKnob(paramId, state, formatter, defaultNorm) {
        const knobEl = document.querySelector(`.knob[data-param="${paramId}"]`);
        const indicator = knobEl ? knobEl.querySelector('.knob-indicator') : null;
        const valueEl = document.querySelector(`[data-value="${paramId}"]`);
        if (!knobEl || !indicator) return;

        let isDragging = false;
        let lastY = 0;

        function updateDisplay(norm) {
            const angle = ANGLE_MIN + norm * ANGLE_RANGE;
            indicator.style.transform = `translateX(-50%) rotate(${angle}deg)`;
            if (valueEl && formatter) valueEl.textContent = formatter(norm);
        }

        // Sync from JUCE
        state.valueChangedEvent.addListener(() => {
            updateDisplay(state.getNormalisedValue());
        });

        // Initial sync
        updateDisplay(state.getNormalisedValue());

        knobEl.addEventListener('mousedown', (e) => {
            isDragging = true;
            lastY = e.clientY;
            state.sliderDragStarted();
            e.preventDefault();
        });

        document.addEventListener('mousemove', (e) => {
            if (!isDragging) return;
            const deltaY = lastY - e.clientY;
            const norm = state.getNormalisedValue();
            const newNorm = Math.max(0, Math.min(1, norm + deltaY * SENSITIVITY));
            state.setNormalisedValue(newNorm);
            lastY = e.clientY;
        });

        document.addEventListener('mouseup', () => {
            if (isDragging) {
                state.sliderDragEnded();
                isDragging = false;
            }
        });

        // Double-click to reset to default
        if (defaultNorm !== undefined) {
            knobEl.addEventListener('dblclick', (e) => {
                e.preventDefault();
                state.sliderDragStarted();
                state.setNormalisedValue(defaultNorm);
                state.sliderDragEnded();
            });
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // ComboBox binding
    // ════════════════════════════════════════════════════════════════════

    function setupComboBox(paramId, state) {
        const selectEl = document.querySelector(`select[data-param="${paramId}"]`);
        if (!selectEl) return;

        state.valueChangedEvent.addListener(() => {
            selectEl.selectedIndex = state.getChoiceIndex();
        });

        selectEl.selectedIndex = state.getChoiceIndex();

        selectEl.addEventListener('change', () => {
            state.setChoiceIndex(selectEl.selectedIndex);
        });
    }

    // ════════════════════════════════════════════════════════════════════
    // Toggle binding
    // ════════════════════════════════════════════════════════════════════

    function setupToggle(paramId, state) {
        const btnEl = document.querySelector(`[data-param="${paramId}"].toggle`);
        if (!btnEl) return;

        state.valueChangedEvent.addListener(() => {
            btnEl.classList.toggle('active', state.getValue());
        });

        btnEl.classList.toggle('active', state.getValue());

        btnEl.addEventListener('click', () => {
            state.setValue(!state.getValue());
        });
    }

    // ════════════════════════════════════════════════════════════════════
    // Value formatters
    // ════════════════════════════════════════════════════════════════════

    function pctFormatter(norm) {
        return Math.round(norm * 100) + '%';
    }

    function grainSizeFormatter(norm) {
        // NormalisableRange: 10-500ms, skew 0.5
        // Approximate: denormalize with skew
        const skew = 0.5;
        const proportion = Math.pow(norm, 1.0 / skew);
        const val = 10 + proportion * (500 - 10);
        return Math.round(val) + ' ms';
    }

    function densityFormatter(norm) {
        const val = 1 + norm * (100 - 1);
        return Math.round(val) + '%';
    }

    function repeatsFormatter(norm) {
        const val = Math.round(1 + norm * 15);
        return String(val);
    }

    function eucPulsesFormatter(norm) {
        const val = Math.round(1 + norm * 15);
        return String(val);
    }

    function eucStepsFormatter(norm) {
        const val = Math.round(2 + norm * 14);
        return String(val);
    }

    function probabilityFormatter(norm) {
        return Math.round(norm * 100) + '%';
    }

    // ════════════════════════════════════════════════════════════════════
    // Initialize all parameter bindings
    // ════════════════════════════════════════════════════════════════════

    function initParams() {
        // Sliders (12) — last arg is normalized default for double-click reset
        setupKnob('grain_size',       Juce.getSliderState('grain_size'),       grainSizeFormatter, 0.4286);
        setupKnob('density',          Juce.getSliderState('density'),          densityFormatter,   0.4949);
        setupKnob('spread',           Juce.getSliderState('spread'),           pctFormatter,       0.0);
        setupKnob('reverse',          Juce.getSliderState('reverse'),          pctFormatter,       0.0);
        setupKnob('feedback',         Juce.getSliderState('feedback'),         pctFormatter,       0.0);
        setupKnob('dry_wet',          Juce.getSliderState('dry_wet'),          pctFormatter,       0.5);
        setupKnob('pitch_random',     Juce.getSliderState('pitch_random'),     pctFormatter,       0.0);
        setupKnob('pan_random',       Juce.getSliderState('pan_random'),       pctFormatter,       0.0);
        setupKnob('probability',      Juce.getSliderState('probability'),      probabilityFormatter, 1.0);
        setupKnob('repeats',          Juce.getSliderState('repeats'),          repeatsFormatter,   0.2);
        setupKnob('euclidean_pulses', Juce.getSliderState('euclidean_pulses'), eucPulsesFormatter, 0.2);
        setupKnob('euclidean_steps',  Juce.getSliderState('euclidean_steps'),  eucStepsFormatter,  0.4286);

        // ComboBoxes (4)
        setupComboBox('scale',      Juce.getComboBoxState('scale'));
        setupComboBox('root_note',  Juce.getComboBoxState('root_note'));
        setupComboBox('pitch_mode', Juce.getComboBoxState('pitch_mode'));
        setupComboBox('sync_mode',  Juce.getComboBoxState('sync_mode'));

        // Toggles (2)
        setupToggle('freeze',       Juce.getToggleState('freeze'));
        setupToggle('stutter_gate', Juce.getToggleState('stutter_gate'));
    }

    // ════════════════════════════════════════════════════════════════════
    // Pitch gate: dim Scale / Root Note / Pitch Mode when Pitch Rnd = 0
    // ════════════════════════════════════════════════════════════════════

    function setupPitchGate() {
        const pitchRandomState = Juce.getSliderState('pitch_random');
        const scaleContainer = document.querySelector('select[data-param="scale"]')?.closest('.dropdown-container');
        const rootNoteContainer = document.querySelector('select[data-param="root_note"]')?.closest('.dropdown-container');
        const pitchModeContainer = document.querySelector('select[data-param="pitch_mode"]')?.closest('.dropdown-container');
        const pitchHint = document.getElementById('pitch-hint');

        if (!scaleContainer || !rootNoteContainer || !pitchModeContainer) return;

        function update() {
            const isDimmed = pitchRandomState.getNormalisedValue() < 0.01;
            scaleContainer.classList.toggle('dimmed', isDimmed);
            rootNoteContainer.classList.toggle('dimmed', isDimmed);
            pitchModeContainer.classList.toggle('dimmed', isDimmed);
            if (pitchHint) pitchHint.classList.toggle('visible', isDimmed);
        }

        pitchRandomState.valueChangedEvent.addListener(update);
        update();
    }

    // ════════════════════════════════════════════════════════════════════
    // Grain Scatter Visualization (Canvas 2D)
    // ════════════════════════════════════════════════════════════════════

    class GrainScatterViz {
        constructor(canvas) {
            this.canvas = canvas;
            this.ctx = canvas.getContext('2d');
            this.grains = [];
            this.activeCount = 0;
            this.resize();
        }

        resize() {
            const dpr = window.devicePixelRatio || 1;
            const rect = this.canvas.parentElement.getBoundingClientRect();
            this.w = rect.width;
            this.h = rect.height;
            this.canvas.width = this.w * dpr;
            this.canvas.height = this.h * dpr;
            this.canvas.style.width = this.w + 'px';
            this.canvas.style.height = this.h + 'px';
            this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
        }

        update(data) {
            this.grains = data.g || [];
            this.activeCount = data.ac || 0;
        }

        draw() {
            const ctx = this.ctx;
            const w = this.w;
            const h = this.h;
            ctx.clearRect(0, 0, w, h);

            // Grid lines
            ctx.strokeStyle = 'rgba(139,115,85,0.12)';
            ctx.lineWidth = 0.5;
            // Horizontal (pitch axis)
            for (let i = 1; i < 5; i++) {
                const y = (h / 5) * i;
                ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
            }
            // Vertical (time axis)
            for (let i = 1; i < 5; i++) {
                const x = (w / 5) * i;
                ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
            }

            // Axis labels
            ctx.fillStyle = 'rgba(139,115,85,0.3)';
            ctx.font = '8px Georgia';
            ctx.fillText('0s', 4, h - 4);
            ctx.fillText('2s', w - 16, h - 4);
            ctx.fillText('+24st', 4, 14);
            ctx.fillText('-24st', 4, h - 14);

            // Center line (0 semitones)
            ctx.strokeStyle = 'rgba(139,115,85,0.2)';
            ctx.lineWidth = 0.5;
            ctx.setLineDash([3, 3]);
            ctx.beginPath(); ctx.moveTo(0, h / 2); ctx.lineTo(w, h / 2); ctx.stroke();
            ctx.setLineDash([]);

            // Draw grains
            for (const g of this.grains) {
                const x = g.p * w;
                // Map pitch: -24 to +24 semitones → bottom to top
                const y = h / 2 - (g.s / 24) * (h / 2);
                const radius = 3 + g.e * 6;
                const alpha = g.e * 0.85 + 0.15;

                if (g.f) {
                    // Frozen: green tint
                    ctx.fillStyle = `rgba(107,142,78,${alpha})`;
                } else {
                    // Live: warm brown
                    ctx.fillStyle = `rgba(139,115,85,${alpha})`;
                }

                ctx.beginPath();
                ctx.arc(x, y, radius, 0, Math.PI * 2);
                ctx.fill();

                // Reverse indicator: small arrow
                if (g.r) {
                    ctx.strokeStyle = `rgba(92,64,51,${alpha * 0.6})`;
                    ctx.lineWidth = 1;
                    ctx.beginPath();
                    ctx.moveTo(x - 3, y);
                    ctx.lineTo(x + 2, y - 3);
                    ctx.moveTo(x - 3, y);
                    ctx.lineTo(x + 2, y + 3);
                    ctx.stroke();
                }
            }

            // Active count
            ctx.fillStyle = 'rgba(139,115,85,0.4)';
            ctx.font = '9px Georgia';
            ctx.textAlign = 'right';
            ctx.fillText(this.activeCount + ' grains', w - 8, 14);
            ctx.textAlign = 'left';
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // Euclidean Circle Visualization (Canvas 2D)
    // ════════════════════════════════════════════════════════════════════

    class EuclideanCircleViz {
        constructor(canvas) {
            this.canvas = canvas;
            this.ctx = canvas.getContext('2d');
            this.pattern = [];
            this.currentStep = 0;
            this.steps = 8;
            this.resize();
        }

        resize() {
            const dpr = window.devicePixelRatio || 1;
            const rect = this.canvas.parentElement.getBoundingClientRect();
            this.w = rect.width;
            this.h = rect.height;
            this.canvas.width = this.w * dpr;
            this.canvas.height = this.h * dpr;
            this.canvas.style.width = this.w + 'px';
            this.canvas.style.height = this.h + 'px';
            this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
        }

        update(data) {
            this.pattern = data.p || [];
            this.currentStep = data.s || 0;
            this.steps = data.n || 8;
        }

        draw() {
            const ctx = this.ctx;
            const w = this.w;
            const h = this.h;
            const cx = w / 2;
            const cy = h / 2;
            const radius = Math.min(cx, cy) - 20;

            ctx.clearRect(0, 0, w, h);

            // Outer circle
            ctx.strokeStyle = 'rgba(139,115,85,0.25)';
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.arc(cx, cy, radius, 0, Math.PI * 2);
            ctx.stroke();

            // Inner circle
            ctx.strokeStyle = 'rgba(139,115,85,0.1)';
            ctx.beginPath();
            ctx.arc(cx, cy, radius * 0.4, 0, Math.PI * 2);
            ctx.stroke();

            if (this.steps === 0) return;

            // Connect active steps with arcs
            const activePositions = [];
            for (let i = 0; i < this.steps; i++) {
                if (this.pattern[i]) {
                    const angle = (i / this.steps) * Math.PI * 2 - Math.PI / 2;
                    activePositions.push({
                        x: cx + radius * Math.cos(angle),
                        y: cy + radius * Math.sin(angle)
                    });
                }
            }
            if (activePositions.length > 1) {
                ctx.strokeStyle = 'rgba(107,142,78,0.2)';
                ctx.lineWidth = 1;
                ctx.beginPath();
                ctx.moveTo(activePositions[0].x, activePositions[0].y);
                for (let i = 1; i < activePositions.length; i++) {
                    ctx.lineTo(activePositions[i].x, activePositions[i].y);
                }
                ctx.closePath();
                ctx.stroke();
            }

            // Draw step dots
            for (let i = 0; i < this.steps; i++) {
                const angle = (i / this.steps) * Math.PI * 2 - Math.PI / 2;
                const x = cx + radius * Math.cos(angle);
                const y = cy + radius * Math.sin(angle);

                const isCurrent = i === this.currentStep;
                const isActive = !!this.pattern[i];

                const dotRadius = isCurrent ? 8 : 5;

                ctx.beginPath();
                ctx.arc(x, y, dotRadius, 0, Math.PI * 2);

                if (isCurrent && isActive) {
                    ctx.fillStyle = '#6B8E4E';
                } else if (isCurrent) {
                    ctx.fillStyle = 'rgba(107,142,78,0.5)';
                } else if (isActive) {
                    ctx.fillStyle = '#8B7355';
                } else {
                    ctx.fillStyle = 'rgba(139,115,85,0.15)';
                }
                ctx.fill();

                // Border on current step
                if (isCurrent) {
                    ctx.strokeStyle = '#3C5C1A';
                    ctx.lineWidth = 2;
                    ctx.stroke();
                }
            }

            // Step counter in center
            ctx.fillStyle = 'rgba(139,115,85,0.4)';
            ctx.font = '10px Georgia';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            const activeCount = this.pattern.filter(Boolean).length;
            ctx.fillText(activeCount + '/' + this.steps, cx, cy);
            ctx.textBaseline = 'alphabetic';
            ctx.textAlign = 'left';
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // Initialization
    // ════════════════════════════════════════════════════════════════════

    let grainViz, euclideanViz;

    function init() {
        initParams();
        setupPitchGate();

        // Visualizations
        const grainCanvas = document.getElementById('grain-canvas');
        const eucCanvas = document.getElementById('euclidean-canvas');

        if (grainCanvas) grainViz = new GrainScatterViz(grainCanvas);
        if (eucCanvas) euclideanViz = new EuclideanCircleViz(eucCanvas);

        // Event listeners for C++ data push
        window.__JUCE__.backend.addEventListener('grainUpdate', (event) => {
            try {
                const data = JSON.parse(event);
                if (grainViz) grainViz.update(data);
            } catch (e) { /* ignore parse errors */ }
        });

        window.__JUCE__.backend.addEventListener('euclideanUpdate', (event) => {
            try {
                const data = JSON.parse(event);
                if (euclideanViz) euclideanViz.update(data);
            } catch (e) { /* ignore parse errors */ }
        });

        // Render loop
        function renderLoop() {
            if (grainViz) grainViz.draw();
            if (euclideanViz) euclideanViz.draw();
            requestAnimationFrame(renderLoop);
        }
        requestAnimationFrame(renderLoop);

        // Handle resize
        window.addEventListener('resize', () => {
            if (grainViz) grainViz.resize();
            if (euclideanViz) euclideanViz.resize();
        });
    }

// ES modules are deferred — DOM is ready when this runs
init();
