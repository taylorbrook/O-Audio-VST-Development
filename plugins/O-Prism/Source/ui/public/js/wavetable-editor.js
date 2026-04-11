/* ═══════════════════════════════════════════════════════════════════
   WAVETABLE EDITOR — O-PRISM
   Per-frame harmonic editing, frame strip, operations, undo/redo
   ═══════════════════════════════════════════════════════════════════ */

const WavetableEditor = (() => {
    // ─── State ───
    let activeOsc = 0;
    let numFrames = 0;
    let selectedFrames = new Set();
    let activeFrame = 0;
    let currentHarmonics = [];
    let numBins = 128;
    let allFrameWaveforms = [];
    let isInitialized = false;
    let editorActive = false;
    let isDragging = false;
    let globalListenersBound = false;

    // ─── Stored global listener refs (for cleanup on tab deactivation) ───
    let keydownHandler = null;
    let resizeHandler = null;

    // ─── Native Function Bindings (lazy-init from JUCE module) ───
    let nativeFns = null;

    function getNative() {
        if (!nativeFns) {
            const gn = window.JuceGetNativeFunction;
            if (!gn) return null;
            nativeFns = {
                startWavetableEditor:     gn('startWavetableEditor'),
                stopWavetableEditor:      gn('stopWavetableEditor'),
                getFrameHarmonics:        gn('getFrameHarmonics'),
                setFrameHarmonics:        gn('setFrameHarmonics'),
                getEditorFrameWaveform:   gn('getEditorFrameWaveform'),
                getAllEditorFrameWaveforms: gn('getAllEditorFrameWaveforms'),
                applyFrameOperation:      gn('applyFrameOperation'),
                saveEditedWavetable:      gn('saveEditedWavetable'),
            };
        }
        return nativeFns;
    }

    // ─── Undo Stack ───
    const undoStack = [];
    const redoStack = [];
    const MAX_UNDO = 50;

    function pushUndo(frameIndex, harmonics) {
        undoStack.push({ frameIndex, harmonics: [...harmonics] });
        if (undoStack.length > MAX_UNDO) undoStack.shift();
        redoStack.length = 0;
        updateUndoButtons();
    }

    function performUndo() {
        if (undoStack.length === 0) return;
        const fn = getNative();
        if (!fn) return;
        const entry = undoStack.pop();
        redoStack.push({ frameIndex: entry.frameIndex, harmonics: [...currentHarmonics] });
        fn.setFrameHarmonics(entry.frameIndex, JSON.stringify(entry.harmonics))
            .then(waveformJson => {
                currentHarmonics = entry.harmonics;
                if (entry.frameIndex === activeFrame) {
                    drawHarmonicBars();
                    if (waveformJson) drawWaveformPreview(JSON.parse(waveformJson));
                }
                refreshFrameStrip(entry.frameIndex);
                updateUndoButtons();
            });
    }

    function performRedo() {
        if (redoStack.length === 0) return;
        const fn = getNative();
        if (!fn) return;
        const entry = redoStack.pop();
        undoStack.push({ frameIndex: entry.frameIndex, harmonics: [...currentHarmonics] });
        fn.setFrameHarmonics(entry.frameIndex, JSON.stringify(entry.harmonics))
            .then(waveformJson => {
                currentHarmonics = entry.harmonics;
                if (entry.frameIndex === activeFrame) {
                    drawHarmonicBars();
                    if (waveformJson) drawWaveformPreview(JSON.parse(waveformJson));
                }
                refreshFrameStrip(entry.frameIndex);
                updateUndoButtons();
            });
    }

    function updateUndoButtons() {
        const undoBtn = document.getElementById('wt-undo-btn');
        const redoBtn = document.getElementById('wt-redo-btn');
        if (undoBtn) undoBtn.disabled = undoStack.length === 0;
        if (redoBtn) redoBtn.disabled = redoStack.length === 0;
    }

    // ─── DPR-aware canvas setup ───
    function setupCanvas(canvas) {
        const dpr = window.devicePixelRatio || 1;
        const rect = canvas.getBoundingClientRect();
        canvas.width = rect.width * dpr;
        canvas.height = rect.height * dpr;
        const ctx = canvas.getContext('2d');
        ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
        return ctx;
    }

    // ─── Frame Strip ───
    const FRAME_W = 52;
    const FRAME_H = 60;
    const FRAME_PAD = 2;
    const STRIP_Y_PAD = 4;
    const LABEL_H = 14;

    function drawFrameStrip() {
        const canvas = document.getElementById('wt-frame-strip');
        if (!canvas) return;

        const totalW = numFrames * (FRAME_W + FRAME_PAD) + FRAME_PAD;
        canvas.style.width = Math.max(totalW, canvas.parentElement.clientWidth) + 'px';

        const dpr = window.devicePixelRatio || 1;
        const rect = canvas.getBoundingClientRect();
        canvas.width = rect.width * dpr;
        canvas.height = rect.height * dpr;
        const ctx = canvas.getContext('2d');
        ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

        ctx.clearRect(0, 0, rect.width, rect.height);

        for (let f = 0; f < numFrames; f++) {
            const x = FRAME_PAD + f * (FRAME_W + FRAME_PAD);
            const isSelected = selectedFrames.has(f);
            const isActive = f === activeFrame;

            // Background
            ctx.fillStyle = isActive ? '#4a6a5a' : isSelected ? '#3d5040' : '#2a1f1f';
            ctx.fillRect(x, STRIP_Y_PAD, FRAME_W, FRAME_H);

            // Border
            if (isActive) {
                ctx.strokeStyle = '#7aaa8a';
                ctx.lineWidth = 2;
            } else if (isSelected) {
                ctx.strokeStyle = '#5a7a6a';
                ctx.lineWidth = 1.5;
            } else {
                ctx.strokeStyle = '#5C4033';
                ctx.lineWidth = 0.5;
            }
            ctx.strokeRect(x, STRIP_Y_PAD, FRAME_W, FRAME_H);

            // Waveform
            if (allFrameWaveforms[f]) {
                const samples = allFrameWaveforms[f];
                ctx.beginPath();
                ctx.strokeStyle = isActive ? '#c0e8d0' : '#8B7355';
                ctx.lineWidth = 1;
                const midY = STRIP_Y_PAD + FRAME_H / 2;
                const halfH = (FRAME_H - 8) / 2;
                for (let i = 0; i < samples.length; i++) {
                    const sx = x + 2 + (i / samples.length) * (FRAME_W - 4);
                    const sy = midY - samples[i] * halfH;
                    if (i === 0) ctx.moveTo(sx, sy);
                    else ctx.lineTo(sx, sy);
                }
                ctx.stroke();
            }

            // Frame number
            ctx.fillStyle = '#8B7355';
            ctx.font = '9px Garamond, Georgia, serif';
            ctx.textAlign = 'center';
            ctx.fillText(String(f), x + FRAME_W / 2, STRIP_Y_PAD + FRAME_H + LABEL_H - 2);
        }
    }

    function refreshFrameStrip(changedFrame) {
        const fn = getNative();
        if (!fn) return;
        // Re-fetch waveform for the changed frame
        if (changedFrame !== undefined) {
            fn.getEditorFrameWaveform(changedFrame).then(json => {
                if (json) {
                    const fullWf = JSON.parse(json);
                    // Downsample to display size
                    const samples = [];
                    const step = Math.max(1, Math.floor(fullWf.length / 32));
                    for (let i = 0; i < fullWf.length; i += step) samples.push(fullWf[i]);
                    allFrameWaveforms[changedFrame] = samples;
                    drawFrameStrip();
                }
            });
        } else {
            drawFrameStrip();
        }
    }

    function handleFrameStripClick(e) {
        const canvas = document.getElementById('wt-frame-strip');
        if (!canvas) return;

        const rect = canvas.getBoundingClientRect();
        const x = e.clientX - rect.left + canvas.parentElement.scrollLeft;
        const clickedFrame = Math.floor((x - FRAME_PAD) / (FRAME_W + FRAME_PAD));

        if (clickedFrame < 0 || clickedFrame >= numFrames) return;

        if (e.shiftKey && selectedFrames.size > 0) {
            // Range select
            const min = Math.min(activeFrame, clickedFrame);
            const max = Math.max(activeFrame, clickedFrame);
            for (let i = min; i <= max; i++) selectedFrames.add(i);
        } else if (e.ctrlKey || e.metaKey) {
            // Toggle
            if (selectedFrames.has(clickedFrame)) selectedFrames.delete(clickedFrame);
            else selectedFrames.add(clickedFrame);
        } else {
            // Single select
            selectedFrames.clear();
            selectedFrames.add(clickedFrame);
        }

        activeFrame = clickedFrame;
        selectedFrames.add(activeFrame);

        drawFrameStrip();
        loadFrameHarmonics(activeFrame);
    }

    // ─── Harmonic Bar Editor ───
    function drawHarmonicBars() {
        const canvas = document.getElementById('wt-harmonic-canvas');
        if (!canvas) return;

        const ctx = setupCanvas(canvas);
        const rect = canvas.getBoundingClientRect();
        const w = rect.width;
        const h = rect.height;

        ctx.clearRect(0, 0, w, h);

        if (currentHarmonics.length === 0) return;

        const displayBins = Math.min(numBins, currentHarmonics.length);
        const barW = Math.max(1, (w - 4) / displayBins - 1);
        const gap = 1;
        const totalBarW = barW + gap;

        for (let i = 0; i < displayBins; i++) {
            const mag = currentHarmonics[i] || 0;
            const barH = mag * (h - 4);
            const x = 2 + i * totalBarW;
            const y = h - 2 - barH;

            // Gradient color: green at bottom, lighter at top
            const grad = ctx.createLinearGradient(x, y, x, h - 2);
            grad.addColorStop(0, '#8aba9a');
            grad.addColorStop(1, '#5a7a6a');
            ctx.fillStyle = grad;
            ctx.fillRect(x, y, barW, barH);
        }

        // Grid lines
        ctx.strokeStyle = 'rgba(139,115,85,0.15)';
        ctx.lineWidth = 0.5;
        for (let i = 1; i < 4; i++) {
            const y = h * i / 4;
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(w, y);
            ctx.stroke();
        }
    }

    function handleHarmonicMouseDown(e) {
        isDragging = true;
        pushUndo(activeFrame, [...currentHarmonics]);
        handleHarmonicDrag(e);
    }

    function handleHarmonicDrag(e) {
        if (!isDragging) return;

        const canvas = document.getElementById('wt-harmonic-canvas');
        if (!canvas) return;

        const rect = canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;
        const w = rect.width;
        const h = rect.height;

        const displayBins = Math.min(numBins, currentHarmonics.length);
        const totalBarW = (w - 4) / displayBins;
        const binIndex = Math.floor((x - 2) / totalBarW);

        if (binIndex < 0 || binIndex >= displayBins) return;

        const mag = Math.max(0, Math.min(1, 1 - (y - 2) / (h - 4)));
        currentHarmonics[binIndex] = mag;

        drawHarmonicBars();
        scheduleHarmonicUpdate();
    }

    function handleHarmonicMouseUp() {
        isDragging = false;
        flushHarmonicUpdate();
    }

    let harmonicUpdatePending = false;
    let harmonicUpdateRAF = null;

    function scheduleHarmonicUpdate() {
        harmonicUpdatePending = true;
        if (!harmonicUpdateRAF) {
            harmonicUpdateRAF = requestAnimationFrame(() => {
                harmonicUpdateRAF = null;
                if (harmonicUpdatePending) flushHarmonicUpdate();
            });
        }
    }

    function flushHarmonicUpdate() {
        harmonicUpdatePending = false;
        const fn = getNative();
        if (!fn) return;
        const displayBins = Math.min(numBins, currentHarmonics.length);
        const mags = currentHarmonics.slice(0, displayBins);
        fn.setFrameHarmonics(activeFrame, JSON.stringify(mags))
            .then(waveformJson => {
                if (waveformJson) {
                    drawWaveformPreview(JSON.parse(waveformJson));
                }
            });
    }

    // ─── Waveform Preview ───
    function drawWaveformPreview(samples) {
        const canvas = document.getElementById('wt-waveform-preview');
        if (!canvas) return;

        const ctx = setupCanvas(canvas);
        const rect = canvas.getBoundingClientRect();
        const w = rect.width;
        const h = rect.height;

        ctx.clearRect(0, 0, w, h);

        if (!samples || samples.length === 0) return;

        // Center line
        ctx.strokeStyle = 'rgba(139,115,85,0.2)';
        ctx.lineWidth = 0.5;
        ctx.beginPath();
        ctx.moveTo(0, h / 2);
        ctx.lineTo(w, h / 2);
        ctx.stroke();

        // Waveform
        ctx.beginPath();
        ctx.strokeStyle = '#8aba9a';
        ctx.lineWidth = 1.5;
        const pad = 8;
        for (let i = 0; i < samples.length; i++) {
            const x = pad + (i / (samples.length - 1)) * (w - 2 * pad);
            const y = h / 2 - samples[i] * (h / 2 - pad);
            if (i === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
        }
        ctx.stroke();
    }

    // ─── Frame Data Loading ───
    function loadFrameHarmonics(frameIndex) {
        const fn = getNative();
        if (!fn) return;
        fn.getFrameHarmonics(frameIndex, numBins).then(json => {
            if (json) {
                currentHarmonics = JSON.parse(json);
                drawHarmonicBars();
            }
        });
        fn.getEditorFrameWaveform(frameIndex).then(json => {
            if (json) drawWaveformPreview(JSON.parse(json));
        });
    }

    // ─── Operations ───
    function applyOperation(opType, extraParam) {
        const fn = getNative();
        if (!fn) return;
        const frames = [...selectedFrames];
        if (frames.length === 0) return;

        // Push undo for active frame
        pushUndo(activeFrame, [...currentHarmonics]);

        const args = [opType, JSON.stringify(frames)];
        if (extraParam !== undefined) args.push(extraParam);

        fn.applyFrameOperation(...args).then(() => {
            // Reload active frame harmonics and refresh strip
            loadFrameHarmonics(activeFrame);
            loadAllFrameWaveforms();
        });
    }

    function saveWavetable() {
        const overlay = document.getElementById('wt-save-modal-overlay');
        const input = document.getElementById('wt-save-name-input');
        if (overlay && input) {
            input.value = 'Edited Wavetable';
            overlay.classList.add('active');
            input.focus();
            input.select();
        }
    }

    function confirmSave() {
        const fn = getNative();
        if (!fn) return;
        const input = document.getElementById('wt-save-name-input');
        const name = input ? input.value.trim() : '';
        if (!name) return;

        fn.saveEditedWavetable(name).then(resultJson => {
            closeSaveModal();
            if (resultJson) {
                const result = JSON.parse(resultJson);
                if (result.success) {
                    // Could show success feedback
                }
            }
        });
    }

    function closeSaveModal() {
        const overlay = document.getElementById('wt-save-modal-overlay');
        if (overlay) overlay.classList.remove('active');
    }

    // ─── Frame Waveform Loading ───
    function loadAllFrameWaveforms() {
        const fn = getNative();
        if (!fn) return;
        fn.getAllEditorFrameWaveforms(32).then(json => {
            if (json) {
                allFrameWaveforms = JSON.parse(json);
                drawFrameStrip();
            }
        });
    }

    // ─── Bin Count Selection ───
    function setBinCount(count) {
        numBins = count;
        document.querySelectorAll('.wt-bin-btn').forEach(btn => {
            btn.classList.toggle('active', parseInt(btn.dataset.bins) === count);
        });
        loadFrameHarmonics(activeFrame);
    }

    // ─── Osc Toggle ───
    function switchOsc(oscIndex) {
        if (oscIndex === activeOsc && editorActive) return;

        const fn = getNative();
        if (!fn) return;

        const doStart = () => {
            activeOsc = oscIndex;
            document.querySelectorAll('.wt-osc-btn').forEach(btn => {
                btn.classList.toggle('active', parseInt(btn.dataset.osc) === oscIndex);
            });

            fn.startWavetableEditor(oscIndex).then(resultJson => {
                if (resultJson) {
                    const result = typeof resultJson === 'string' ? JSON.parse(resultJson) : resultJson;
                    if (result.numFrames) {
                        editorActive = true;
                        numFrames = result.numFrames;
                        selectedFrames.clear();
                        selectedFrames.add(0);
                        activeFrame = 0;
                        currentHarmonics = result.harmonics || [];
                        undoStack.length = 0;
                        redoStack.length = 0;
                        updateUndoButtons();

                        drawHarmonicBars();
                        loadAllFrameWaveforms();
                        loadFrameHarmonics(0);
                    }
                }
            });
        };

        if (editorActive) {
            fn.stopWavetableEditor().then(doStart);
        } else {
            doStart();
        }
    }

    // ─── Lifecycle ───
    function onTabActivated() {
        if (!isInitialized) {
            isInitialized = true;
            bindEvents();
        }
        bindGlobalListeners();
        switchOsc(activeOsc);
    }

    function onTabDeactivated() {
        if (editorActive) {
            const fn = getNative();
            if (fn) fn.stopWavetableEditor();
            editorActive = false;
        }
        unbindGlobalListeners();
        undoStack.length = 0;
        redoStack.length = 0;
    }

    function bindEvents() {
        // Frame strip click
        const stripCanvas = document.getElementById('wt-frame-strip');
        if (stripCanvas) {
            stripCanvas.addEventListener('click', handleFrameStripClick);
        }

        // Harmonic canvas (DOM-scoped — tied to element lifetime, not document)
        const harmCanvas = document.getElementById('wt-harmonic-canvas');
        if (harmCanvas) {
            harmCanvas.addEventListener('mousedown', handleHarmonicMouseDown);
            harmCanvas.addEventListener('mousemove', handleHarmonicDrag);
        }

        // Bin count buttons
        document.querySelectorAll('.wt-bin-btn').forEach(btn => {
            btn.addEventListener('click', () => setBinCount(parseInt(btn.dataset.bins)));
        });

        // Osc toggle
        document.querySelectorAll('.wt-osc-btn').forEach(btn => {
            btn.addEventListener('click', () => switchOsc(parseInt(btn.dataset.osc)));
        });

        // Operations
        document.getElementById('wt-op-normalize')?.addEventListener('click', () => applyOperation('normalize'));
        document.getElementById('wt-op-normalizeGlobal')?.addEventListener('click', () => applyOperation('normalizeGlobal'));
        document.getElementById('wt-op-fade')?.addEventListener('click', () => applyOperation('fade', 10));
        document.getElementById('wt-op-reverse')?.addEventListener('click', () => applyOperation('reverse'));
        document.getElementById('wt-op-reverseOrder')?.addEventListener('click', () => applyOperation('reverseOrder'));
        document.getElementById('wt-op-smooth')?.addEventListener('click', () => applyOperation('smooth', 0.5));

        // Save
        document.getElementById('wt-op-save')?.addEventListener('click', saveWavetable);
        document.getElementById('wt-save-confirm')?.addEventListener('click', confirmSave);
        document.getElementById('wt-save-cancel')?.addEventListener('click', closeSaveModal);
        document.getElementById('wt-save-name-input')?.addEventListener('keydown', e => {
            if (e.key === 'Enter') confirmSave();
            if (e.key === 'Escape') closeSaveModal();
        });

        // Undo/Redo
        document.getElementById('wt-undo-btn')?.addEventListener('click', performUndo);
        document.getElementById('wt-redo-btn')?.addEventListener('click', performRedo);

        // Keyboard shortcuts + window resize + harmonic window-level mouseup
        // are now bound/unbound in bindGlobalListeners/unbindGlobalListeners,
        // tied to tab activation lifecycle to prevent listener accumulation.
    }

    // ─── Global listeners (window/document) — bound per activation ───
    // These are stored as module-level refs so they can be removed in
    // onTabDeactivated. Previously they were anonymous and leaked for the
    // lifetime of the page.
    function bindGlobalListeners() {
        if (globalListenersBound) return;

        keydownHandler = (e) => {
            const wtTab = document.getElementById('wavetable-tab');
            if (!wtTab || !wtTab.classList.contains('active')) return;

            if ((e.ctrlKey || e.metaKey) && e.key === 'z' && !e.shiftKey) {
                e.preventDefault();
                performUndo();
            } else if ((e.ctrlKey || e.metaKey) && e.key === 'z' && e.shiftKey) {
                e.preventDefault();
                performRedo();
            }
        };
        document.addEventListener('keydown', keydownHandler);

        resizeHandler = () => {
            const wtTab = document.getElementById('wavetable-tab');
            if (wtTab && wtTab.classList.contains('active')) {
                drawHarmonicBars();
                drawFrameStrip();
            }
        };
        window.addEventListener('resize', resizeHandler);

        // handleHarmonicMouseUp is already a named function declared elsewhere;
        // we reference it directly so we can remove it by identity later.
        window.addEventListener('mouseup', handleHarmonicMouseUp);

        globalListenersBound = true;
    }

    function unbindGlobalListeners() {
        if (!globalListenersBound) return;

        if (keydownHandler) {
            document.removeEventListener('keydown', keydownHandler);
            keydownHandler = null;
        }
        if (resizeHandler) {
            window.removeEventListener('resize', resizeHandler);
            resizeHandler = null;
        }
        window.removeEventListener('mouseup', handleHarmonicMouseUp);

        globalListenersBound = false;
    }

    return { onTabActivated, onTabDeactivated };
})();

// Export to global scope
window.WavetableEditor = WavetableEditor;
