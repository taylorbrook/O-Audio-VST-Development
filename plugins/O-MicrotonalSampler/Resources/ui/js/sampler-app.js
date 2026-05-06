/* ==============================================================================
 * sampler-app.js
 * O-MicrotonalSampler — Phase 3.1 entry point.
 *
 * Wires:
 *   - 8 APVTS sliders ↔ DOM range inputs via Juce.getSliderState() (relay
 *     identifiers must match the C++ WebSliderRelay names exactly).
 *     (v1.7.0 added 'expression' — CC 11 dynamics.)
 *   - Tab activation (Sample Map / Tuning / About).
 *   - Lazy mount of the TuningPanel on first Tuning-tab activation, plus a
 *     read-only interval-input → span swap shim per RESEARCH §RQ3-1.
 *   - sampleMapUpdated event listener (initial pull on load + push events
 *     from C++ on every map atomic-store).
 *   - Tuning-state readout poll on editor open + Tuning-tab activation
 *     (RP3-3 — no background interval).
 *
 * Phases 3.2–3.5 will extend this with grid rendering, drag-drop, loop
 * editor, knob styling. The 3.1 surface is intentionally minimal.
 * ============================================================================== */

// The check_native_interop.js script (carried verbatim from O-Bells, loaded
// in <head>) populates window.__JUCE__ before this module runs.
import * as Juce from './juce/index.js';

// v1.13.0 (ARCH-02): WKWebView drag-drop content-streaming — shared module.
// PluginEditor::getResource serves this at /js/modules/webview-drop-streaming.js.
import { bindWebViewFileDrop } from './modules/webview-drop-streaming.js';

// ============================================================================
// Slider relay binding
// ============================================================================
//
// Each entry maps the DOM element id to its WebSliderRelay/APVTS parameter
// id. The relay handle (Juce.getSliderState) is bidirectional: setting
// .setNormalisedValue() pushes to the C++ APVTS; valueChangedEvent fires
// when automation / preset / DAW changes the parameter so we update the
// DOM control to match.
// v1.16.8 (HIGH-06): SLIDER_BINDINGS is the single source of truth for the
// 8 control-strip knobs — order, label, and optional tooltip. Knob DOM is
// rendered from this array by renderControlStrip() before bindOneKnob runs.
const SLIDER_BINDINGS = [
    { domId: 'ctrl-attack',              relayId: 'attack',              label: 'Attack' },
    { domId: 'ctrl-decay',               relayId: 'decay',               label: 'Decay' },
    { domId: 'ctrl-sustain',             relayId: 'sustain',             label: 'Sustain' },
    { domId: 'ctrl-release',             relayId: 'release',             label: 'Release' },
    { domId: 'ctrl-polyphony',           relayId: 'polyphony',           label: 'Poly' },
    { domId: 'ctrl-velocity-crossfade',  relayId: 'velocity_crossfade',  label: 'Vel-XF' },
    { domId: 'ctrl-expression',          relayId: 'expression',          label: 'Expr',
      tooltip: 'Expression (MIDI CC 11) — dynamics control, independent of velocity layer' },  // v1.7.0
    { domId: 'ctrl-output-gain',         relayId: 'output_gain',         label: 'Out Gain' }
];

// Phase 3.5 — display formatting per parameter. Maps relayId → {min, max,
// suffix, format(value)}. Values shown next to each knob mirror the C++
// NormalisableRange + AudioParameterFloat conventions in PluginProcessor.
//
// All sliders are 0..1 normalised on the JS side; we map to display ranges
// here purely for UI text. The actual APVTS value is what the host sees.
const KNOB_FORMATS = {
    'attack':              { min: 0.001, max: 5.0, suffix: ' s',
                             format: v => v < 0.1 ? v.toFixed(3) : v.toFixed(2) },
    'decay':               { min: 0.001, max: 5.0, suffix: ' s',
                             format: v => v < 0.1 ? v.toFixed(3) : v.toFixed(2) },
    'sustain':             { min: 0.0,   max: 1.0, suffix: '',
                             format: v => v.toFixed(2) },
    'release':             { min: 0.001, max: 5.0, suffix: ' s',
                             format: v => v < 0.1 ? v.toFixed(3) : v.toFixed(2) },
    'polyphony':           { min: 1,     max: 32, suffix: '',
                             format: v => Math.round(v).toString() },
    'velocity_crossfade':  { min: 0.0,   max: 1.0, suffix: '',
                             format: v => v.toFixed(2) },
    // v1.7.0: expression as 0-100% (squared curve handled C++ side).
    'expression':          { min: 0.0,   max: 100.0, suffix: ' %',
                             format: v => Math.round(v).toString() },
    'output_gain':         { min: -24.0, max: 12.0, suffix: ' dB',
                             format: v => (v >= 0 ? '+' : '') + v.toFixed(1) },
};

// SVG arc geometry (matches O-Bells: 270 degree sweep over r=18). The arc
// is drawn via stroke-dasharray + stroke-dashoffset on a circle; the
// transform: rotate(-135deg) on the SVG places the gap at the bottom.
const KNOB_RADIUS = 18;
const KNOB_ARC_LENGTH = 2 * Math.PI * KNOB_RADIUS * 0.75;

// Global drag state for the SVG knobs (shared across all 7 — only one drag
// at a time, matches O-Bells convention so we don't fight pointer-capture).
const knobDrag = {
    active: false,
    knob: null,            // .ouaricon-knob root element
    state: null,           // Juce slider state
    input: null,           // hidden <input type="range">
    fmt: null,             // KNOB_FORMATS entry
    valueEl: null,         // .ouaricon-knob-value span
    vineEl: null,          // .knob-vine SVG circle
    startY: 0,
    startNorm: 0,
    pointerId: -1,
};

function knobUpdateVisual(vineEl, valueEl, fmt, norm) {
    // Arc sweep
    if (vineEl) {
        const offset = KNOB_ARC_LENGTH * (1 - Math.max(0, Math.min(1, norm)));
        vineEl.style.strokeDasharray  = KNOB_ARC_LENGTH.toFixed(2);
        vineEl.style.strokeDashoffset = offset.toFixed(2);
    }
    // Numeric readout
    if (valueEl && fmt) {
        const real = fmt.min + norm * (fmt.max - fmt.min);
        valueEl.textContent = fmt.format(real) + fmt.suffix;
    }
}

function bindOneKnob({ domId, relayId }) {
    const input = document.getElementById(domId);
    if (!input) {
        console.warn(`[sampler-app] DOM element #${domId} not found`);
        return;
    }

    const knob = input.closest('.ouaricon-knob');
    if (!knob) {
        console.warn(`[sampler-app] no .ouaricon-knob wrapper for #${domId}`);
        return;
    }

    const vineEl  = knob.querySelector('.knob-vine');
    const valueEl = knob.querySelector('.ouaricon-knob-value');
    const fmt = KNOB_FORMATS[relayId] || null;

    let state = null;
    try {
        state = Juce.getSliderState(relayId);
    } catch (e) {
        console.error(`[sampler-app] Failed to bind slider ${relayId}:`, e);
        return;
    }

    // Initial pull
    const init = state.getNormalisedValue();
    if (typeof init === 'number') {
        input.value = init;
        knobUpdateVisual(vineEl, valueEl, fmt, init);
    }

    // C++ -> DOM (automation, preset load, DAW change)
    state.valueChangedEvent.addListener(() => {
        const v = state.getNormalisedValue();
        if (typeof v === 'number') {
            input.value = v;
            knobUpdateVisual(vineEl, valueEl, fmt, v);
        }
    });

    // DOM -> C++ (defensive — input only changes if external code sets
    // .value and dispatches event; the SVG drag updates state directly).
    input.addEventListener('input', () => {
        const v = parseFloat(input.value);
        if (!Number.isNaN(v)) {
            state.setNormalisedValue(v);
            knobUpdateVisual(vineEl, valueEl, fmt, v);
        }
    });

    // Pointer drag — relative-vertical 200 px = full sweep (matches the
    // O-Reed sensitivity from agent memory; feels natural for 44 px knobs).
    knob.addEventListener('pointerdown', (e) => {
        if (knobDrag.active) return;
        knobDrag.active     = true;
        knobDrag.knob       = knob;
        knobDrag.state      = state;
        knobDrag.input      = input;
        knobDrag.fmt        = fmt;
        knobDrag.valueEl    = valueEl;
        knobDrag.vineEl     = vineEl;
        knobDrag.startY     = e.clientY;
        knobDrag.startNorm  = state.getNormalisedValue();
        knobDrag.pointerId  = e.pointerId;
        knob.classList.add('dragging');
        try { knob.setPointerCapture(e.pointerId); } catch (_) {}
        state.sliderDragStarted();
        e.preventDefault();
    });

    // Wheel — 2 % per tick (matches O-Bells fxKnob convention).
    knob.addEventListener('wheel', (e) => {
        e.preventDefault();
        const cur = state.getNormalisedValue();
        const delta = e.deltaY < 0 ? 0.02 : -0.02;
        const next = Math.max(0, Math.min(1, cur + delta));
        state.setNormalisedValue(next);
        input.value = next;
        knobUpdateVisual(vineEl, valueEl, fmt, next);
    }, { passive: false });

    // Double-click resets to default (per APVTS default — pull from C++).
    knob.addEventListener('dblclick', (e) => {
        e.preventDefault();
        // Most natural reset: snap to mid-range. Stage 4 polish will plumb
        // the parameter's default explicitly via a native function.
        const mid = 0.5;
        state.sliderDragStarted();
        state.setNormalisedValue(mid);
        state.sliderDragEnded();
        input.value = mid;
        knobUpdateVisual(vineEl, valueEl, fmt, mid);
    });
}

function bindKnobGlobalDrag() {
    document.addEventListener('pointermove', (e) => {
        if (!knobDrag.active || e.pointerId !== knobDrag.pointerId) return;
        const deltaY = knobDrag.startY - e.clientY;          // up = increase
        const sensitivity = 1 / 200;                          // 200 px = full sweep
        const next = Math.max(0, Math.min(1, knobDrag.startNorm + deltaY * sensitivity));
        knobDrag.state.setNormalisedValue(next);
        if (knobDrag.input) knobDrag.input.value = next;
        knobUpdateVisual(knobDrag.vineEl, knobDrag.valueEl, knobDrag.fmt, next);
    });

    const endDrag = (e) => {
        if (!knobDrag.active) return;
        if (e && e.pointerId !== knobDrag.pointerId) return;
        try {
            if (knobDrag.knob && knobDrag.knob.hasPointerCapture && knobDrag.knob.hasPointerCapture(knobDrag.pointerId)) {
                knobDrag.knob.releasePointerCapture(knobDrag.pointerId);
            }
        } catch (_) {}
        if (knobDrag.state) knobDrag.state.sliderDragEnded();
        if (knobDrag.knob) knobDrag.knob.classList.remove('dragging');
        knobDrag.active = false;
        knobDrag.knob = null;
        knobDrag.state = null;
        knobDrag.input = null;
        knobDrag.fmt = null;
        knobDrag.valueEl = null;
        knobDrag.vineEl = null;
        knobDrag.pointerId = -1;
    };
    document.addEventListener('pointerup',     endDrag);
    document.addEventListener('pointercancel', endDrag);
}

// v1.16.8 (HIGH-06): render the 8 knob blocks from SLIDER_BINDINGS into
// <footer id="control-strip">. Replaces 8 nearly-identical static HTML
// blocks. Run unconditionally at boot — the DOM should exist even when
// the plugin host isn't available (matches pre-refactor behaviour where
// the static knobs always rendered).
function renderControlStrip() {
    const strip = document.getElementById('control-strip');
    if (!strip) {
        console.warn('[sampler-app] #control-strip not found — knobs cannot render');
        return;
    }
    strip.innerHTML = SLIDER_BINDINGS.map(b => {
        const titleAttr = b.tooltip ? ` title="${b.tooltip.replace(/"/g, '&quot;')}"` : '';
        return `
      <div class="ouaricon-knob" data-knob-id="${b.domId}"${titleAttr}>
        <div class="ouaricon-knob-visual">
          <svg viewBox="0 0 44 44">
            <circle class="knob-track" cx="22" cy="22" r="18"/>
            <circle class="knob-vine"  cx="22" cy="22" r="18"/>
          </svg>
        </div>
        <label class="ouaricon-knob-label" for="${b.domId}">${b.label}</label>
        <span class="ouaricon-knob-value"></span>
        <input type="range" id="${b.domId}" min="0" max="1" step="0.001" />
      </div>`;
    }).join('');
}

function bindSliders() {
    renderControlStrip();
    if (!window.__JUCE__) {
        console.warn('[sampler-app] __JUCE__ not available — running outside plugin host');
        return;
    }
    SLIDER_BINDINGS.forEach(bindOneKnob);
    bindKnobGlobalDrag();
}

// ============================================================================
// Tab activation
// ============================================================================
const tabButtons = () => document.querySelectorAll('.tab-btn');
const tabBodies  = () => document.querySelectorAll('.tab-body');

function activateTab(tabName) {
    tabButtons().forEach(btn => {
        btn.classList.toggle('active', btn.dataset.tab === tabName);
    });
    tabBodies().forEach(body => {
        body.classList.toggle('active', body.id === `tab-${tabName}`);
    });

    // v1.10.0: anatomy overlay parallax — swap position class per tab so the
    // engraving slides subtly between tabs (samplemap = peeking right edge,
    // tuning = retreats further right, about = swings into view as a feature).
    const overlay = document.getElementById('anatomyOverlay');
    if (overlay) {
        overlay.classList.remove('samplemap-position', 'tuning-position', 'about-position');
        overlay.classList.add(`${tabName}-position`);
    }

    if (tabName === 'tuning') {
        ensureTuningPanelMounted();
        refreshTuningReadout();
    }
    // v1.1.0: re-sync vel-labels height when returning to Sample Map
    // (getBoundingClientRect returns 0 on hidden tab; values may be stale).
    if (tabName === 'samplemap') {
        requestAnimationFrame(syncVelLabelsHeight);
    }
}

function bindTabs() {
    tabButtons().forEach(btn => {
        btn.addEventListener('click', () => activateTab(btn.dataset.tab));
    });
}

// ============================================================================
// TuningPanel — lazy mount (v1.2.0: editable authoring surface)
//
// v1.0–v1.1 mounted the panel in display-only mode (per Stage 3 RESEARCH
// §RQ3-1) by layering tuning-panel-readonly.css on top of the editable
// HTML and span-swapping each interval-input into a read-only label.
// v1.2.0 reverses that decision: the panel is now fully editable —
// users can select factory tunings from the library, load .scl/.kbm
// files, generate scales, and edit individual intervals. The shared
// scala-tuning-engine remains the single source of truth that VST3
// Note Expression overrides at note-on time, so Dorico microtonal
// playback is preserved.
// ============================================================================
let tuningPanelMounted = false;
let tuningPanelInstance = null;

// v1.7.1: subscribe to C++ tuning-note events ONCE, at module init time, NOT
// inside ensureTuningPanelMounted (which is gated on first Tuning-tab click).
// Events that arrive before the panel is mounted are simply dropped; the
// catch-up pull inside ensureTuningPanelMounted brings the panel up to date
// with whatever is held at that moment, and subsequent events flow live.
function bindTuningNoteEvents() {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;

    const backend = window.__JUCE__.backend;

    backend.addEventListener('tuningNoteOn', (midiVar) => {
        if (!tuningPanelInstance) return;
        const midi = typeof midiVar === 'number' ? midiVar : parseInt(midiVar, 10);
        if (Number.isFinite(midi)) {
            try { tuningPanelInstance.noteOn(midi); } catch (_) { /* ignore */ }
        }
    });

    backend.addEventListener('tuningNoteOff', (midiVar) => {
        if (!tuningPanelInstance) return;
        const midi = typeof midiVar === 'number' ? midiVar : parseInt(midiVar, 10);
        if (Number.isFinite(midi)) {
            try { tuningPanelInstance.noteOff(midi); } catch (_) { /* ignore */ }
        }
    });

    backend.addEventListener('tuningHeldNotes', (payloadVar) => {
        if (!tuningPanelInstance) return;
        try {
            const payload = typeof payloadVar === 'string'
                ? JSON.parse(payloadVar) : payloadVar;
            if (payload && Array.isArray(payload.notes) && Array.isArray(payload.freqs)) {
                tuningPanelInstance.updateHeldNotes(payload.notes, payload.freqs);
            }
        } catch (_) { /* ignore — malformed payload is non-fatal */ }
    });
}
// Run once on module load.
bindTuningNoteEvents();

async function ensureTuningPanelMounted() {
    if (tuningPanelMounted) return;
    tuningPanelMounted = true;

    const container = document.getElementById('tuning-container');
    if (!container) {
        console.error('[sampler-app] #tuning-container missing');
        return;
    }

    try {
        const mod = await import('./tuning-panel.js');
        const TuningPanel = mod.TuningPanel || mod.default;
        if (!TuningPanel) {
            console.error('[sampler-app] tuning-panel.js exports unrecognized');
            return;
        }

        // tuning-panel.js calls juceApi.getNativeFunction(name) — that method
        // lives on the ES-module namespace `Juce`, NOT on window.__JUCE__
        // (which is the low-level postMessage handler). Passing the wrong
        // object is a pre-existing v1.0–v1.1 latent bug that silently
        // swallowed every backend call inside tuning-panel.js's try/catch
        // blocks (intervals never loaded, library never populated, etc.).
        tuningPanelInstance = new TuningPanel(container, Juce);
        await tuningPanelInstance.init();

        // v1.7.1: catch-up pull. The 30 Hz timer in C++ only emits
        // tuningHeldNotes on bitmask change, so a panel that mounts while
        // notes are already held would otherwise sit empty until the next
        // change. Pulling once at mount fixes that.
        try {
            const json = await Juce.getNativeFunction('getHeldNotesJson')();
            const payload = JSON.parse(json);
            if (Array.isArray(payload.notes) && Array.isArray(payload.freqs)) {
                tuningPanelInstance.updateHeldNotes(payload.notes, payload.freqs);
                // Replay note-ons so Circle / Polar highlights catch up too
                // (updateHeldNotes only feeds TrueKeys; activeScaleDegrees is
                // populated through noteOn).
                for (const midi of payload.notes) {
                    try { tuningPanelInstance.noteOn(midi); } catch (_) { /* ignore */ }
                }
            }
        } catch (_) { /* ignore — empty held set or backend unavailable */ }

        // v1.2.0: auto-expand the Tuning Library and pull the factory
        // preset list so the right column shows selectable tunings on
        // first open (instead of just the category dropdown). Same goes
        // for the Scale Generator section, which the panel renders
        // collapsed by default.
        const libContent = container.querySelector('#library-content');
        const libToggle  = container.querySelector('#library-toggle');
        libContent?.classList.add('expanded');
        libToggle?.classList.add('expanded');
        await tuningPanelInstance.loadEmbeddedTunings();

        const genContent = container.querySelector('#generator-content');
        const genToggle  = container.querySelector('#generator-toggle');
        genContent?.classList.add('expanded');
        genToggle?.classList.add('expanded');
    } catch (e) {
        console.error('[sampler-app] TuningPanel mount failed:', e);
        container.innerHTML = '<div style="color:var(--text-muted); padding:16px; font-style:italic;">Tuning panel unavailable.</div>';
    }
}

// ============================================================================
// Tuning-state readout (RP3-3 — poll on Tuning-tab activate + editor open)
// ============================================================================
async function refreshTuningReadout() {
    const el = document.getElementById('tuning-readout');
    if (!el || !window.__JUCE__) return;

    try {
        const getName = Juce.getNativeFunction('getTuningName');
        const name = await getName();
        if (typeof name === 'string') el.textContent = name;
    } catch (e) {
        // Silent — fail-safe if native function not registered yet.
    }
}

async function refreshAboutVersion() {
    const el = document.getElementById('about-version');
    if (!el || !window.__JUCE__) return;

    try {
        const getVersion = Juce.getNativeFunction('getPluginVersion');
        const value = await getVersion();
        if (typeof value === 'string' && value.length > 0) {
            el.textContent = 'v' + value;
        }
    } catch (e) {
        // Silent — leave pill empty if native function unavailable.
    }
}

// ============================================================================
// Sample-map snapshot — initial pull + push subscription
// ============================================================================
async function pullInitialSampleMap() {
    if (!window.__JUCE__) return;
    try {
        const getMap = Juce.getNativeFunction('getSampleMap');
        const json = await getMap();
        handleSampleMapSnapshot(json);
    } catch (e) {
        console.warn('[sampler-app] getSampleMap failed:', e);
    }
}

function subscribeSampleMapUpdates() {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;
    try {
        window.__JUCE__.backend.addEventListener('sampleMapUpdated', (payload) => {
            handleSampleMapSnapshot(payload);
        });
    } catch (e) {
        console.warn('[sampler-app] sampleMapUpdated subscription failed:', e);
    }
}

// Track previous skipped-files count so we only toast on transitions
// (e.g. 0 → N or M → N where the set changed). Toasts are emitted on every
// fresh load that produces skips; not on idle re-renders.
let lastSkippedSignature = '';

// v1.14.0: cache the most recent snapshot so technique-tab switches can
// re-render the grid without a fresh pull from C++.
let lastSampleMapSnapshot = null;

function handleSampleMapSnapshot(payloadOrJson) {
    let snap;
    try {
        // Push events arrive parsed; pull returns a JSON string.
        snap = (typeof payloadOrJson === 'string')
            ? JSON.parse(payloadOrJson)
            : payloadOrJson;
    } catch (e) {
        console.warn('[sampler-app] sampleMap snapshot parse failed:', e);
        return;
    }

    if (!snap) return;
    lastSampleMapSnapshot = snap;     // v1.14.0

    // v1.14.0: refresh the technique tab strip's per-slot cell counts and
    // empty-slot dimming whenever the map changes (folder load, single-cell
    // load, clear). Render is cheap — one DOM rebuild for at most 8 buttons.
    renderTechniqueBar();

    // ---- Issues disclosure + toast (Phase 3.3 Task 21) ----
    //
    // The disclosure is always reflected (open or closed depending on the
    // current map's skip set). The toast is emitted only on a *transition* —
    // i.e. the new skip-set differs from the last one we showed — to avoid
    // re-toasting on every push event when the set is stable.
    const issues = document.getElementById('issues-disclosure');
    const issuesList = document.getElementById('issues-list');
    const skipped = Array.isArray(snap.skippedFiles) ? snap.skippedFiles : [];
    const sig = skipped.join('');  // 0x01 separator — never appears in filenames

    if (issues && issuesList) {
        if (skipped.length > 0) {
            issuesList.innerHTML = '';
            skipped.forEach(s => {
                const li = document.createElement('li');
                // Each entry is "filename: reason" or just "filename" — render
                // the raw string; the processor formats consistently.
                li.textContent = s;
                issuesList.appendChild(li);
            });
            issues.querySelector('summary').textContent =
                `Issues (${skipped.length} file${skipped.length === 1 ? '' : 's'} skipped)`;
            issues.hidden = false;
        } else {
            issues.hidden = true;
        }
    }

    // Only toast on transition. (Initial pull on editor open: lastSkippedSignature
    // is '' — if there were skips from a prior session they will toast once.
    // Subsequent renders with the same set are silent.)
    if (sig !== lastSkippedSignature) {
        lastSkippedSignature = sig;
        if (skipped.length > 0) {
            showToast(`${skipped.length} file${skipped.length === 1 ? '' : 's'} skipped`);
        }
    }

    // ---- Grid render (Phase 3.2) ----
    renderGrid(snap);

    // ---- Clear-samples button enable/disable (v1.0.2) ----
    // Only meaningful when the map has at least one loaded slot — otherwise
    // there is nothing to clear and the button stays disabled to avoid
    // accidental clicks (and a no-op confirmation roundtrip).
    const clearBtn = document.getElementById('clear-samples-btn');
    if (clearBtn) {
        const hasSlots = Array.isArray(snap?.slots) && snap.slots.length > 0;
        clearBtn.disabled = !hasSlots;
    }

    // Re-publish the cell-layout shadow now that DOM has settled. Defer one
    // frame so the browser has computed final geometry.
    requestAnimationFrame(() => publishCellLayout());

    // ---- Loop editor sync (Phase 3.4) ----
    // If the editor is open and the active cell's loop fields changed (e.g.
    // user clicked Apply, or external automation altered the map), refresh
    // the editor's snapshot so the loop-mode label / reset-button enabled
    // state stay consistent. Marker positions are NOT clobbered if the user
    // is mid-drag (dragMarker !== null).
    if (editorState.open && editorState.midi >= 0) {
        const cellSlot = Array.isArray(snap?.slots)
            ? snap.slots.find(s => s.midiNote === editorState.midi
                                && s.velocityLayer === editorState.vel)
            : null;
        if (cellSlot && editorState.snap && editorState.dragMarker === null) {
            editorState.snap.loopMode = cellSlot.loopMode;
            editorState.snap.loopStart = cellSlot.loopStart;
            editorState.snap.loopEnd   = cellSlot.loopEnd;
            editorState.snap.filename  = cellSlot.filename;
            editorState.loopStart = cellSlot.loopStart;
            editorState.loopEnd   = cellSlot.loopEnd;
            populateLoopEditorHeader(editorState.snap);
            redrawLoopEditor();
        }
    }
}

// ============================================================================
// Sample-mapping grid (Phase 3.2 — Tasks 15–17)
// ============================================================================
//
// Layout per PLAN Task 15:
//   - 88 columns (MIDI 21..108), 4 rows (velocity layers).
//   - Rows top→bottom = layer 3..0 so the loudest layer reads at the top.
//   - data-note + data-layer attributes for hit-testing.
//   - cell-loaded / cell-empty / cell-loading / cell-active classes.
//   - title attribute = filename for loaded cells (tooltip).
//   - octave-separator marker (data-octave-start) every 12 cells.
//
// Mid-session replace metric (PLAN Task 18 gate): the time from FileChooser
// close → DOM update is logged via performance.now() in handleCellAction.
//
// The grid is rebuilt on every snapshot. With 352 cells (88 × 4), full
// rebuild is < 5 ms and avoids stateful diff bookkeeping.

const MIDI_LOW  = 21;   // A0
const MIDI_HIGH = 108;  // C8
const NUM_LAYERS = 4;

// v1.1.0: Velocity-layer ranges (matches MicrotonalSamplerVoice.cpp:
// layerWidth = 128/4 = 32, layerIdx = (vel-1)/32 with vel ∈ [1,127]).
//   L0 → 1..32, L1 → 33..64, L2 → 65..96, L3 → 97..127
function velocityLayerToRange(layer) {
    const lo = layer === 0 ? 1 : layer * 32 + 1;
    const hi = layer === 3 ? 127 : (layer + 1) * 32;
    return { lo, hi, label: `${lo}–${hi}` };
}

const NOTE_NAMES = ['C', 'C♯', 'D', 'D♯', 'E', 'F', 'F♯', 'G', 'G♯', 'A', 'A♯', 'B'];

// MIDI 0 = C-2, MIDI 12 = C-1, MIDI 60 = C3 (middle C), … This is the
// Ableton/Cubase/FL/Logic/Pro Tools/Reaper convention. v1.11.1 switched
// from C4=60 (Yamaha/JUCE-native) to C3=60 to match the FilenameParser
// (Source/FilenameParser.cpp) and the dominant DAW labelling. Keep these
// two formulas in lockstep — drift causes parsed cell midiNotes and UI
// labels to diverge by 12 semitones.
function midiToNoteName(midi) {
    const pitchClass = ((midi % 12) + 12) % 12;
    const octave = Math.floor(midi / 12) - 2;
    return `${NOTE_NAMES[pitchClass]}${octave}`;
}

function renderGrid(snap) {
    const container = document.getElementById('sample-map-grid');
    if (!container) return;

    // v1.12.2 (FE-03): cancel any in-flight single-click defer before we
    // tear down the cell DOM. The 250 ms double-click discriminator (see
    // bindGridInteractions) closes over a `cell` reference; if a folder
    // load / sample-map snapshot fires a renderGrid mid-defer, the timer
    // would later read `dataset.note`/`dataset.layer` off a detached node
    // and silently no-op (or worse, fire on a re-bound stale cell at the
    // same grid position with different MIDI/layer values). Clearing the
    // pending timer is the simplest fix and matches the cleanup the
    // dblclick branch already performs.
    if (pendingClickTimer) {
        clearTimeout(pendingClickTimer);
        pendingClickTimer = null;
    }

    // Build fast — clear via innerHTML then assemble in a fragment.
    container.innerHTML = '';
    const inner = document.createElement('div');
    inner.id = 'sample-grid-inner';

    // v1.8.0: prefer cells (multi-variant aware) but accept legacy slots
    // for back-compat in case any consumer is still feeding the old shape.
    // Cells emit `{midiNote, velocityLayer, technique, variants: [...]}`.
    // Slots emit `{midiNote, velocityLayer, filename, variantCount?, ...}`
    // with `variantCount` carrying the cell's variant count (v1.8.0+).
    //
    // v1.14.0: filter cells by the active technique tab. Cells in other
    // technique slots exist in the snapshot but are not visible in the
    // grid until the user switches tabs. Falls back gracefully when the
    // snapshot has no `technique` field (v1.13.0 in-memory contract).
    const activeTech = techniqueState.active;
    const cellMap = new Map();
    if (Array.isArray(snap?.cells)) {
        for (const c of snap.cells) {
            const t = Number.isFinite(c.technique) ? c.technique : 0;
            if (t !== activeTech) continue;
            cellMap.set(`${c.midiNote}_${c.velocityLayer}`, c);
        }
    }
    // The legacy `slots` array isn't technique-aware. We only fall through
    // to it when the cells map is empty AND the active technique is 0 —
    // that way single-technique back-compat libraries still render via the
    // legacy path while multi-technique sessions never see ghost slots
    // bleeding from another technique.
    const slotMap = new Map();
    if (cellMap.size === 0 && activeTech === 0 && Array.isArray(snap?.slots)) {
        for (const s of snap.slots) {
            slotMap.set(`${s.midiNote}_${s.velocityLayer}`, s);
        }
    }

    const frag = document.createDocumentFragment();

    // Rows are layer 3 (top) → layer 0 (bottom) so loudest-on-top.
    for (let row = 0; row < NUM_LAYERS; ++row) {
        const layer = (NUM_LAYERS - 1) - row;
        const velRange = velocityLayerToRange(layer);
        for (let midi = MIDI_LOW; midi <= MIDI_HIGH; ++midi) {
            const cell = document.createElement('div');
            cell.className = 'grid-cell';
            cell.dataset.note = String(midi);
            cell.dataset.layer = String(layer);

            // Mark octave starts (C notes — midi % 12 === 0) for the octave
            // separator border. Always also include MIDI 21 (lowest in range).
            if (midi % 12 === 0 || midi === MIDI_LOW) {
                cell.dataset.octaveStart = '1';
            }

            // v1.1.0: Tooltip = "<filename | Empty> · <NoteName>(<midi>) · Vel <lo>–<hi>"
            // v1.8.0: multi-variant cells list every variant filename.
            // v1.14.0: tooltip includes the active technique slot name when
            // the user has expanded beyond the default single slot — that's
            // the visible signal that a load auto-routed to a non-`ord` slot.
            const noteLabel = `${midiToNoteName(midi)} (${midi})`;
            const velLabel  = `Vel ${velRange.label}`;
            const techName  = techniqueState.names[activeTech]
                              || `slot ${activeTech + 1}`;
            const techLabel = (techniqueState.count > 1)
                                ? ` · tech: ${techName}`
                                : '';
            const cellEntry = cellMap.get(`${midi}_${layer}`);
            const slot = slotMap.get(`${midi}_${layer}`);

            if (cellEntry && Array.isArray(cellEntry.variants) && cellEntry.variants.length > 0) {
                cell.classList.add('cell-loaded');
                const variantCount = cellEntry.variants.length;
                if (variantCount > 1) {
                    cell.classList.add('cell-multivariant');
                    const head = `${variantCount} variants:`;
                    const list = cellEntry.variants
                        .map((v, i) => `  ${i + 1}. ${v.filename || '(unnamed)'}`)
                        .join('\n');
                    cell.title = `${head}\n${list}\n${noteLabel} · ${velLabel}${techLabel}`;
                    cell.dataset.variantCount = String(variantCount);
                } else {
                    const head = cellEntry.variants[0].filename || 'Loaded';
                    cell.title = `${head} · ${noteLabel} · ${velLabel}${techLabel}`;
                }
            } else if (slot) {
                cell.classList.add('cell-loaded');
                const head = slot.filename ? slot.filename : 'Loaded';
                if (slot.variantCount && slot.variantCount > 1) {
                    cell.classList.add('cell-multivariant');
                    cell.title = `${head} (+${slot.variantCount - 1} more) · ${noteLabel} · ${velLabel}`;
                    cell.dataset.variantCount = String(slot.variantCount);
                } else {
                    cell.title = `${head} · ${noteLabel} · ${velLabel}`;
                }
            } else {
                cell.classList.add('cell-empty');
                cell.title = `Empty · ${noteLabel} · ${velLabel}`;
            }

            frag.appendChild(cell);
        }
    }

    inner.appendChild(frag);
    container.appendChild(inner);

    // v1.1.0: Render C-note column labels below the grid (inside scroll
    // container, so they pan horizontally with it).
    const colLabels = document.createElement('div');
    colLabels.id = 'sample-grid-col-labels';
    const colFrag = document.createDocumentFragment();
    for (let midi = MIDI_LOW; midi <= MIDI_HIGH; ++midi) {
        const lbl = document.createElement('span');
        lbl.className = 'col-label';
        if (midi % 12 === 0) {
            lbl.dataset.c = '1';
            lbl.textContent = midiToNoteName(midi);   // "C1", "C2", … "C8"
        }
        colFrag.appendChild(lbl);
    }
    colLabels.appendChild(colFrag);
    container.appendChild(colLabels);

    // v1.1.0: Render velocity row labels in the sidebar wrapper (outside
    // scroll container so they stay visible while user scrolls horizontally).
    const velLabels = document.getElementById('sample-grid-vel-labels');
    if (velLabels) {
        velLabels.innerHTML = '';
        const vFrag = document.createDocumentFragment();
        for (let row = 0; row < NUM_LAYERS; ++row) {
            const layer = (NUM_LAYERS - 1) - row;
            const v = velocityLayerToRange(layer);
            const el = document.createElement('div');
            el.className = 'vel-label';
            el.dataset.layer = String(layer);
            el.textContent = v.label;
            el.title = `Velocity layer ${layer}: MIDI velocity ${v.label}`;
            vFrag.appendChild(el);
        }
        velLabels.appendChild(vFrag);
    }

    // Wire interactions on the inner container (single delegated listener).
    bindGridInteractions(inner);

    // v1.1.0: Sync vel-labels' top-padding + height to the inner grid's
    // actual rendered position. Run on next frame so layout is settled.
    requestAnimationFrame(syncVelLabelsHeight);
}

// v1.1.0: Align vel-labels rows to the rendered inner grid.
//   topOffset = inner.top - wrapper.top  → matches scroll container's
//                                          top-padding offset.
//   height    = topOffset + inner.height → bottom edge meets inner-grid bottom.
// The vel-labels' grid-template-rows: repeat(4, 1fr) then fills the area
// between (topOffset, topOffset+innerHeight), aligning row-for-row with
// the cells.
function syncVelLabelsHeight() {
    const inner   = document.getElementById('sample-grid-inner');
    const vel     = document.getElementById('sample-grid-vel-labels');
    const wrapper = document.getElementById('sample-map-grid-wrapper');
    if (!inner || !vel || !wrapper) return;

    const innerRect   = inner.getBoundingClientRect();
    const wrapperRect = wrapper.getBoundingClientRect();
    const topOffset   = Math.max(0, innerRect.top - wrapperRect.top);

    vel.style.paddingTop    = topOffset + 'px';
    vel.style.paddingBottom = '0px';
    vel.style.height        = (topOffset + innerRect.height) + 'px';
    vel.style.boxSizing     = 'border-box';
}

// ============================================================================
// Cell interactions (Phase 3.2 Task 16 — RP3-1 resolution)
// ============================================================================
//
//   - Single-click EMPTY  → loadSingleSampleDialog(midi, vel)
//   - Single-click LOADED → openLoopEditor(midi, vel)  (3.4 placeholder; logs)
//   - Double-click LOADED → loadSingleSampleDialog(midi, vel)  (replace)
//   - Right-click any     → context menu (Replace / Open Loop Editor / Clear)
//
// The 250 ms double-click discrimination is implemented via setTimeout — when
// a single-click fires we delay the action by 250 ms; if a second click
// arrives in that window we cancel the timer and fire the dblclick action
// instead. The browser's native `dblclick` event fires on the second click
// so we use it directly and rely on the timer to defer the single-click.

let pendingClickTimer = null;
let lastReplaceTimestamp = 0;

function bindGridInteractions(innerEl) {
    if (!innerEl || innerEl.dataset.bound === '1') return;
    innerEl.dataset.bound = '1';

    innerEl.addEventListener('click', (e) => {
        const cell = e.target.closest('.grid-cell');
        if (!cell) return;
        const midi  = parseInt(cell.dataset.note, 10);
        const layer = parseInt(cell.dataset.layer, 10);
        if (!Number.isFinite(midi) || !Number.isFinite(layer)) return;

        // Defer single-click so a follow-up dblclick can cancel it.
        if (pendingClickTimer) clearTimeout(pendingClickTimer);
        pendingClickTimer = setTimeout(() => {
            pendingClickTimer = null;
            handleCellSingleClick(cell, midi, layer);
        }, 250);
    });

    innerEl.addEventListener('dblclick', (e) => {
        const cell = e.target.closest('.grid-cell');
        if (!cell) return;
        // Cancel the pending single-click action.
        if (pendingClickTimer) {
            clearTimeout(pendingClickTimer);
            pendingClickTimer = null;
        }
        const midi  = parseInt(cell.dataset.note, 10);
        const layer = parseInt(cell.dataset.layer, 10);
        if (!Number.isFinite(midi) || !Number.isFinite(layer)) return;

        // Double-click on a loaded cell → replace path. Empty cells route
        // to the same FileChooser as single-click; behaviour is consistent.
        replaceCellSample(cell, midi, layer);
    });

    innerEl.addEventListener('contextmenu', (e) => {
        const cell = e.target.closest('.grid-cell');
        if (!cell) return;
        e.preventDefault();
        showContextMenu(cell, e.clientX, e.clientY);
    });
}

function handleCellSingleClick(cell, midi, layer) {
    const isLoaded = cell.classList.contains('cell-loaded');
    if (isLoaded) {
        // Phase 3.4 — open loop editor side panel.
        openLoopEditor(midi, layer);
    } else {
        replaceCellSample(cell, midi, layer);
    }
}

async function replaceCellSample(cell, midi, layer) {
    if (!window.__JUCE__) return;

    // v1.9.0: when the target cell is non-empty, prompt the user to choose
    // between merging the new sample as a round-robin variant or replacing
    // the cell. Empty cells skip straight to the file picker (v1.8.0 path).
    let mergeAsRr = false;
    const isLoaded = cell.classList.contains('cell-loaded');
    if (isLoaded) {
        const existingCount = parseInt(cell.dataset.variantCount, 10);
        const count = Number.isFinite(existingCount) && existingCount > 0
            ? existingCount : 1;
        const choice = await showPerCellMergeDialog(count, midi, layer);
        if (choice === null) return;
        mergeAsRr = (choice === 'merge');
    }

    cell.classList.add('cell-loading');
    const t0 = performance.now();
    lastReplaceTimestamp = t0;

    try {
        const fn = Juce.getNativeFunction('loadSingleSampleDialog');
        const ok = await fn(midi, layer, mergeAsRr);
        if (!ok) {
            // User cancelled or selection invalid — drop the loading shimmer.
            cell.classList.remove('cell-loading');
            return;
        }
        // The sampleMapUpdated push event will trigger renderGrid which
        // rebuilds the cell. Log timing for the gate metric.
        const t1 = performance.now();
        console.log(`[sampler-app] FileChooser close → load dispatch: ${(t1 - t0).toFixed(1)} ms (mergeAsRr=${mergeAsRr})`);
    } catch (e) {
        console.error('[sampler-app] replaceCellSample failed:', e);
        cell.classList.remove('cell-loading');
    }
}

// ---- Context menu ----
let contextMenuCell = null;

function showContextMenu(cell, clientX, clientY) {
    const menu = document.getElementById('cell-context-menu');
    if (!menu) return;

    const isLoaded = cell.classList.contains('cell-loaded');
    contextMenuCell = cell;

    // Disable "Open Loop Editor" on empty cells; "Clear" is always disabled in v1.0.
    const openBtn = menu.querySelector('button[data-action="open-loop-editor"]');
    if (openBtn) openBtn.disabled = !isLoaded;

    menu.style.left = `${clientX}px`;
    menu.style.top  = `${clientY}px`;
    menu.hidden = false;

    // Wire actions once.
    if (menu.dataset.bound !== '1') {
        menu.dataset.bound = '1';
        menu.addEventListener('click', (e) => {
            const btn = e.target.closest('button');
            if (!btn || btn.disabled) return;
            const action = btn.dataset.action;
            const cellEl = contextMenuCell;
            if (cellEl) {
                const midi  = parseInt(cellEl.dataset.note, 10);
                const layer = parseInt(cellEl.dataset.layer, 10);
                if (action === 'replace') {
                    replaceCellSample(cellEl, midi, layer);
                } else if (action === 'open-loop-editor') {
                    openLoopEditor(midi, layer);
                } else if (action === 'clear') {
                    /* disabled in v1.0 */
                }
            }
            hideContextMenu();
        });
    }
}

function hideContextMenu() {
    const menu = document.getElementById('cell-context-menu');
    if (menu) menu.hidden = true;
    contextMenuCell = null;
}

document.addEventListener('click', (e) => {
    const menu = document.getElementById('cell-context-menu');
    if (!menu || menu.hidden) return;
    if (!menu.contains(e.target)) hideContextMenu();
});

// ============================================================================
// Layout shadow publish (Phase 3.2 Task 17 — reportCellLayout)
// ============================================================================
//
// The C++ side hit-tests file drops against a JS-published cell rectangle map.
// We re-publish on:
//   - every sampleMapUpdated event (after the grid renders),
//   - ResizeObserver callbacks on document.body (rAF-throttled — RESEARCH §9
//     risk register: too-frequent calls cause WebView main-thread thrash).
//
// The shadow is JSON: {cells: [{midiNote, velocityLayer, x, y, w, h}], folderZone: {...}}.
// Coordinates are in WebView client space (page-relative), which is what the
// C++ FileDragAndDropTarget expects.

let layoutPublishScheduled = false;

function publishCellLayout() {
    if (layoutPublishScheduled || !window.__JUCE__) return;
    layoutPublishScheduled = true;

    requestAnimationFrame(() => {
        layoutPublishScheduled = false;

        try {
            const cells = [];
            document.querySelectorAll('.grid-cell').forEach(el => {
                const r = el.getBoundingClientRect();
                cells.push({
                    midiNote:      parseInt(el.dataset.note, 10),
                    velocityLayer: parseInt(el.dataset.layer, 10),
                    x: Math.round(r.left),
                    y: Math.round(r.top),
                    w: Math.round(r.width),
                    h: Math.round(r.height)
                });
            });

            let folderZone = { x: 0, y: 0, w: 0, h: 0 };
            const fz = document.getElementById('folder-drop-zone');
            if (fz) {
                const r = fz.getBoundingClientRect();
                folderZone = {
                    x: Math.round(r.left),
                    y: Math.round(r.top),
                    w: Math.round(r.width),
                    h: Math.round(r.height)
                };
            }

            const payload = JSON.stringify({ cells, folderZone });
            const fn = Juce.getNativeFunction('reportCellLayout');
            fn(payload);
        } catch (e) {
            console.warn('[sampler-app] publishCellLayout failed:', e);
        }
    });
}

// Phase 3.5 Task 33 — auto-close loop editor + toast when window narrows
// below 900 px while it's open (panel + grid would otherwise overlap).
const NARROW_BREAKPOINT_PX = 900;
const NARROW_TOAST = 'Resize wider to use the loop editor.';
let lastIsNarrow = null;  // null = not yet measured; otherwise boolean

function checkNarrowWindowGuard() {
    const w = window.innerWidth || document.documentElement.clientWidth || 0;
    const isNarrow = w < NARROW_BREAKPOINT_PX;
    if (isNarrow === lastIsNarrow) return;
    lastIsNarrow = isNarrow;

    if (isNarrow && editorState.open) {
        closeLoopEditor();
        showToast(NARROW_TOAST);
    }
}

function bindResizeObserver() {
    const onResize = () => {
        publishCellLayout();
        checkNarrowWindowGuard();
        syncVelLabelsHeight();   // v1.1.0: keep vel-labels aligned on resize
    };
    if (typeof ResizeObserver !== 'function') {
        // Fallback to window resize listener.
        window.addEventListener('resize', onResize);
        return;
    }
    const obs = new ResizeObserver(onResize);
    obs.observe(document.body);
    // Seed initial bucket so the very first transition fires correctly.
    checkNarrowWindowGuard();
}

// ============================================================================
// Folder drop-zone — Load Folder… button (Phase 3.3 Task 20)
// ============================================================================

function bindFolderDropZone() {
    const button = document.getElementById('load-folder-btn');
    if (!button) return;

    button.addEventListener('click', async () => {
        if (!window.__JUCE__) return;
        try {
            // v1.6.0: ask for layer / mode / override BEFORE the native file
            // picker so the user has the controls in front of them while the
            // OS modal is still latent. Cancel here = silent return.
            //
            // v1.12.0: dialog flow doesn't know the folder size at modal
            // time, so we pass null for sizeBytes — the embed checkbox
            // surfaces a "size confirmed after selection" hint, and a
            // post-pick confirmation modal shows the real size before
            // the load commits.
            const opts = await showFolderLoadOptionsModal(null);
            if (!opts) return;

            // v1.12.0: pickSampleFolder returns just the folder path
            // (replaces v1.6.0's combined loadSampleFolderDialog). This
            // separation lets us interject the embed-size confirmation
            // between selection and load.
            const pick = await Juce.getNativeFunction('pickSampleFolder')();
            if (!pick || pick.cancelled || !pick.path) {
                return;
            }

            // v1.12.0: when embed is on, surface the actual byte cost as a
            // confirmation gate. Cancel = abort the entire load (no half-
            // committed state on the C++ side, since we haven't called load
            // yet).
            if (opts.embedAudio) {
                let sizeBytes = 0;
                try {
                    sizeBytes = await Juce.getNativeFunction(
                        'estimateFolderAudioSize')(pick.path);
                } catch (sizeErr) {
                    console.warn('[sampler-app] estimateFolderAudioSize failed:', sizeErr);
                }
                const proceed = await showEmbedSizeConfirmModal(
                    Number(sizeBytes) || 0, pick.name || '');
                if (!proceed) return;
            }

            const ok = await Juce.getNativeFunction('loadSampleFolderByPath')(
                pick.path,
                opts.layer,
                opts.mode,
                opts.override   ? 1 : 0,
                opts.embedAudio ? 1 : 0);
            if (!ok) {
                showToast('Folder load failed');
            }
            // sampleMapUpdated push event drives the rest of the UI.
        } catch (e) {
            console.error('[sampler-app] folder load failed:', e);
        }
    });
}

// ============================================================================
// WebView file drag-drop — delegated to shared module (v1.13.0, ARCH-02)
// ============================================================================
//
// The full pattern lives in modules/core/webview-drop-streaming. The
// historical context (why C++ FileDragAndDropTarget overrides do not fire,
// why -unregisterDraggedTypes failed in v1.0.1, why the JUCE Component
// overlay failed in v1.0.2, and the v1.0.4 base64 content-streaming
// workaround) is documented in that module's README and source.
//
// We bind the document-level drop listener via the module API in the
// DOMContentLoaded handler at the bottom of this file. The plugin only
// owns the glue: drop-zone selectors, modal/toast/hover callbacks, and
// the cell midi/vel extractor.

// Plugin-specific UI helper passed to the module as opts.setDropZoneHover.
// Toggles the .drag-over class on the folder-drop-zone element so the CSS
// hover treatment fires while the cursor is over the zone during a drag.
// Also called via bindHostDragEvents (host C++→JS channel) for hosts that
// expose drag move/exit events — defence in depth for non-WKWebView paths.
function setFolderDropZoneHover (over) {
    const z = document.getElementById('folder-drop-zone');
    if (z) z.classList.toggle('drag-over', !!over);
}

// ============================================================================
// Clear-samples button + in-WebView confirmation modal (v1.0.2)
// ============================================================================
//
// JUCE does not wire the JavaScript window.confirm() through WKWebView's
// UIDelegate, so we render our own dialog (CSS in sampler-shell.css, markup
// at the end of #tab-samplemap). showConfirmDialog returns nothing — it
// invokes onConfirm on the Confirm button click, hides the dialog on Cancel
// or Escape, and supports Enter as a confirm shortcut. One-shot listeners
// are detached on every dialog close so subsequent opens do not double-fire.

// v1.12.0: format byte counts for the embed size label.
//   formatBytes(0)        → '0 B'
//   formatBytes(1024)     → '1.0 KB'
//   formatBytes(1500000)  → '1.4 MB'
//   formatBytes(2.5e9)    → '2.3 GB'
function formatBytes (n) {
    if (!Number.isFinite(n) || n <= 0) return '0 B';
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    let i = 0;
    let v = n;
    while (v >= 1024 && i < units.length - 1) {
        v /= 1024;
        i++;
    }
    const decimals = (i === 0 || v >= 100) ? 0 : 1;
    return `${v.toFixed(decimals)} ${units[i]}`;
}

// v1.6.0: folder-load options modal. Resolves to {layer, mode, override,
// embedAudio} on Load, or null on Cancel. Surfaced before BOTH the file
// picker (button) and macOS drag-drop streaming (so cancel doesn't waste 5s
// of base64 work).
//
//   layer:      0..3 — target velocity layer
//   mode:       'append' | 'replace_layer' | 'replace_all' | 'merge_rr' (v1.9.0)
//   override:   boolean — true = ignore filename velocity tokens (v1, ff, …)
//   embedAudio: boolean — v1.12.0; true = serialise audio inline into the
//               saved project state so reload doesn't depend on the source
//               folder existing on disk
//
// v1.9.0: 'merge_rr' adds the new samples as round-robin variants on top of
// any existing cells they collide with (per (note, layer)) — useful for
// layering multiple takes/recordings as RR alternates.
//
// v1.12.0: optional sizeBytes — when known at modal time (drag-drop has
// already walked the entry tree for size), the embed-size label updates
// live as the user toggles the embed checkbox. For dialog flows (folder
// not picked yet) pass null; the size is shown in a follow-up confirmation
// modal after the file picker resolves.
function showFolderLoadOptionsModal (sizeBytes) {
    return new Promise((resolve) => {
        const dialog     = document.getElementById('folder-load-options-dialog');
        if (!dialog) { resolve(null); return; }

        const segBtns    = dialog.querySelectorAll('.flo-seg');
        const modeRadios = dialog.querySelectorAll('input[name="flo-mode"]');
        const overrideEl = document.getElementById('flo-override-checkbox');
        const embedEl    = document.getElementById('flo-embed-checkbox');
        const sizeEl     = document.getElementById('flo-embed-size');
        const explainEl  = document.getElementById('flo-explain');
        const confirmBtn = document.getElementById('flo-confirm-btn');
        const cancelBtn  = document.getElementById('flo-cancel-btn');
        if (!segBtns.length || !modeRadios.length || !overrideEl
                || !embedEl || !sizeEl
                || !explainEl || !confirmBtn || !cancelBtn) {
            resolve(null);
            return;
        }

        // Reset to defaults each invocation.
        let layer = 0;
        let mode  = 'append';
        segBtns.forEach((b, i) => b.classList.toggle('active', i === 0));
        modeRadios.forEach((r) => { r.checked = (r.value === 'append'); });
        overrideEl.checked = false;
        embedEl.checked    = false;

        const updateSizeLabel = () => {
            // Always-on embed-size telemetry per the v1.12.0 spec — the label
            // is visible whenever the embed checkbox is on, regardless of
            // estimated size, so the user always sees the size impact.
            if (!embedEl.checked) {
                sizeEl.textContent = '';
                sizeEl.classList.remove('has-warning');
                return;
            }
            if (typeof sizeBytes === 'number' && sizeBytes > 0) {
                sizeEl.textContent =
                    `Project state will grow by ~${formatBytes(sizeBytes)}.`;
                sizeEl.classList.add('has-warning');
            } else {
                // Dialog flow — folder not selected yet. The post-pick
                // confirmation modal will surface the actual size before
                // the load commits.
                sizeEl.textContent =
                    'Size will be confirmed after folder selection.';
                sizeEl.classList.remove('has-warning');
            }
        };

        const updateExplain = () => {
            const overrideOn = overrideEl.checked;
            const layerStr   = `L${layer}`;
            let txt = '';
            if (mode === 'append') {
                txt = overrideOn
                    ? `Add samples to ${layerStr}, ignoring filename velocity tokens.`
                    : `Add samples; filename tokens (v1–v4, p/mp/mf/f) decide layer.`;
            } else if (mode === 'replace_layer') {
                txt = overrideOn
                    ? `Clear ${layerStr} and add the new samples there.`
                    : `Clear ${layerStr}; filename tokens decide where new samples land.`;
            } else if (mode === 'replace_all') {
                txt = overrideOn
                    ? `Replace existing samples; new ones land on ${layerStr}.`
                    : `Replace existing samples; filename tokens decide layer.`;
            } else if (mode === 'merge_rr') {
                txt = overrideOn
                    ? `Layer onto ${layerStr}: collisions become round-robin variants (cap 64 per cell).`
                    : `Layer existing notes: collisions become round-robin variants. Filename tokens decide layer.`;
            }
            explainEl.textContent = txt;
        };
        updateExplain();
        updateSizeLabel();

        const segHandler = (e) => {
            const b = e.target.closest('.flo-seg');
            if (!b) return;
            const v = parseInt(b.dataset.layer, 10);
            if (!Number.isFinite(v)) return;
            layer = v;
            segBtns.forEach((x) => x.classList.toggle('active', x === b));
            updateExplain();
        };
        const modeHandler = (e) => {
            mode = e.target.value;
            updateExplain();
        };
        const overrideHandler = () => updateExplain();
        const embedHandler    = () => updateSizeLabel();

        const cleanup = () => {
            dialog.hidden = true;
            segBtns.forEach((b) => b.removeEventListener('click', segHandler));
            modeRadios.forEach((r) => r.removeEventListener('change', modeHandler));
            overrideEl.removeEventListener('change', overrideHandler);
            embedEl.removeEventListener('change', embedHandler);
            confirmBtn.removeEventListener('click', onYes);
            cancelBtn.removeEventListener('click', onNo);
            document.removeEventListener('keydown', onKey, true);
        };
        const onYes = () => {
            cleanup();
            resolve({
                layer,
                mode,
                override:   overrideEl.checked,
                embedAudio: embedEl.checked,
            });
        };
        const onNo  = () => { cleanup(); resolve(null); };
        const onKey = (e) => {
            if (e.key === 'Escape')      { e.preventDefault(); onNo(); }
            else if (e.key === 'Enter')  { e.preventDefault(); onYes(); }
        };

        segBtns.forEach((b) => b.addEventListener('click', segHandler));
        modeRadios.forEach((r) => r.addEventListener('change', modeHandler));
        overrideEl.addEventListener('change', overrideHandler);
        embedEl.addEventListener('change', embedHandler);
        confirmBtn.addEventListener('click', onYes);
        cancelBtn.addEventListener('click', onNo);
        document.addEventListener('keydown', onKey, true);

        dialog.hidden = false;
        confirmBtn.focus();
    });
}

// v1.12.0: post-pick embed-size confirmation modal (dialog flow only). Used
// when the user opted in to embed in the options modal, and the folder
// they then picked is large enough that they may want to back out. Resolves
// true on Confirm, false on Cancel/Escape.
function showEmbedSizeConfirmModal (sizeBytes, displayName) {
    return new Promise((resolve) => {
        const dialog     = document.getElementById('embed-size-confirm-dialog');
        const messageEl  = document.getElementById('embed-size-confirm-message');
        const confirmBtn = document.getElementById('embed-size-confirm-btn');
        const cancelBtn  = document.getElementById('embed-size-cancel-btn');
        if (!dialog || !messageEl || !confirmBtn || !cancelBtn) {
            // Fallback to a plain confirm so the load never silently
            // commits without the user seeing the size.
            const summary = `Embed will add ~${formatBytes(sizeBytes)} `
                          + `to your project state. Continue?`;
            resolve(window.confirm(summary));
            return;
        }

        const folderLabel = displayName ? `"${displayName}" ` : '';
        messageEl.textContent =
            `Embedding folder ${folderLabel}will add `
            + `~${formatBytes(sizeBytes)} to your project state.`;

        const cleanup = () => {
            dialog.hidden = true;
            confirmBtn.removeEventListener('click', onYes);
            cancelBtn.removeEventListener('click', onNo);
            document.removeEventListener('keydown', onKey, true);
        };
        const onYes = () => { cleanup(); resolve(true); };
        const onNo  = () => { cleanup(); resolve(false); };
        const onKey = (e) => {
            if (e.key === 'Escape')     { e.preventDefault(); onNo(); }
            else if (e.key === 'Enter') { e.preventDefault(); onYes(); }
        };

        confirmBtn.addEventListener('click', onYes);
        cancelBtn.addEventListener('click', onNo);
        document.addEventListener('keydown', onKey, true);

        dialog.hidden = false;
        confirmBtn.focus();
    });
}

// v1.9.0: per-cell merge prompt. Surfaced when the user attempts a per-cell
// single-file load on a non-empty cell. Resolves to:
//   'merge'   — append the new sample as a round-robin variant.
//   'replace' — drop the cell's variants and replace with this single sample
//               (v1.8.0 behaviour, current default for explicit replace).
//   null      — user cancelled.
function showPerCellMergeDialog (existingCount, midi, layer) {
    return new Promise((resolve) => {
        const dialog    = document.getElementById('per-cell-merge-dialog');
        const messageEl = document.getElementById('per-cell-merge-message');
        const mergeBtn  = document.getElementById('per-cell-merge-merge-btn');
        const replBtn   = document.getElementById('per-cell-merge-replace-btn');
        const cancelBtn = document.getElementById('per-cell-merge-cancel-btn');
        if (!dialog || !messageEl || !mergeBtn || !replBtn || !cancelBtn) {
            resolve(null);
            return;
        }

        const noteName = (typeof midiToNoteName === 'function')
            ? midiToNoteName(midi) : `MIDI ${midi}`;
        const variantWord = existingCount === 1 ? '1 variant' : `${existingCount} variants`;
        const capHit = existingCount >= 64;
        messageEl.textContent = capHit
            ? `${noteName} layer L${layer} already holds the maximum ${variantWord}. Replace the cell, or cancel.`
            : `${noteName} layer L${layer} already holds ${variantWord}. Add this sample as round-robin variant ${existingCount + 1}, or replace the cell?`;
        mergeBtn.disabled = capHit;
        mergeBtn.style.opacity = capHit ? '0.4' : '';
        mergeBtn.style.cursor  = capHit ? 'not-allowed' : '';

        const cleanup = () => {
            dialog.hidden = true;
            mergeBtn.removeEventListener('click', onMerge);
            replBtn.removeEventListener('click', onReplace);
            cancelBtn.removeEventListener('click', onCancel);
            document.removeEventListener('keydown', onKey, true);
        };
        const onMerge   = () => { if (capHit) return; cleanup(); resolve('merge'); };
        const onReplace = () => { cleanup(); resolve('replace'); };
        const onCancel  = () => { cleanup(); resolve(null); };
        const onKey = (e) => {
            if (e.key === 'Escape')      { e.preventDefault(); onCancel(); }
            else if (e.key === 'Enter')  { e.preventDefault(); capHit ? onReplace() : onMerge(); }
        };

        mergeBtn.addEventListener('click', onMerge);
        replBtn.addEventListener('click', onReplace);
        cancelBtn.addEventListener('click', onCancel);
        document.addEventListener('keydown', onKey, true);

        dialog.hidden = false;
        (capHit ? replBtn : mergeBtn).focus();
    });
}

function showConfirmDialog ({ title, message, confirmLabel, destructive, onConfirm }) {
    const dialog    = document.getElementById('confirm-dialog');
    const titleEl   = document.getElementById('confirm-dialog-title');
    const messageEl = document.getElementById('confirm-dialog-message');
    const confirmEl = document.getElementById('confirm-confirm-btn');
    const cancelEl  = document.getElementById('confirm-cancel-btn');
    if (!dialog || !titleEl || !messageEl || !confirmEl || !cancelEl) return;

    titleEl.textContent   = title   || 'Are you sure?';
    messageEl.textContent = message || '';
    confirmEl.textContent = confirmLabel || 'Confirm';
    confirmEl.classList.toggle('destructive', !!destructive);

    const cleanup = () => {
        dialog.hidden = true;
        confirmEl.removeEventListener('click', onYes);
        cancelEl.removeEventListener('click', onNo);
        document.removeEventListener('keydown', onKey, true);
    };
    const onYes = async () => { cleanup(); if (onConfirm) await onConfirm(); };
    const onNo  = () => cleanup();
    const onKey = (e) => {
        if (e.key === 'Escape') { e.preventDefault(); onNo(); }
        else if (e.key === 'Enter') { e.preventDefault(); onYes(); }
    };

    confirmEl.addEventListener('click', onYes);
    cancelEl.addEventListener('click', onNo);
    document.addEventListener('keydown', onKey, true);

    dialog.hidden = false;
    cancelEl.focus();
}

// v1.0.3: diagnostic dialog used by the failed-drop path. Auto-copies the
// dump to the clipboard on open so the user doesn't have to chase a toast,
// and exposes a selectable <pre> fallback for hosts where the clipboard API
// is gated.
async function showDiagnosticDialog (title, text) {
    const dialog   = document.getElementById('diagnostic-dialog');
    const titleEl  = document.getElementById('diagnostic-dialog-title');
    const hintEl   = document.getElementById('diagnostic-dialog-hint');
    const textEl   = document.getElementById('diagnostic-dialog-text');
    const copyBtn  = document.getElementById('diagnostic-copy-btn');
    const closeBtn = document.getElementById('diagnostic-close-btn');
    if (!dialog || !titleEl || !textEl || !copyBtn || !closeBtn) return;

    titleEl.textContent = title || 'Diagnostic';
    textEl.textContent  = text  || '';
    copyBtn.textContent = 'Copy again';

    const writeClipboard = async () => {
        // Primary: async Clipboard API (requires user activation, which the
        // drop event provides).
        if (navigator?.clipboard?.writeText) {
            try { await navigator.clipboard.writeText(text); return true; }
            catch (_) { /* fall through to execCommand fallback */ }
        }
        // Fallback: select the <pre> contents and execCommand('copy').
        try {
            const range = document.createRange();
            range.selectNodeContents(textEl);
            const sel = window.getSelection();
            sel.removeAllRanges();
            sel.addRange(range);
            const ok = document.execCommand('copy');
            sel.removeAllRanges();
            return ok;
        } catch (_) { return false; }
    };

    const autoCopied = await writeClipboard();
    if (hintEl) {
        hintEl.textContent = autoCopied
            ? 'Auto-copied to clipboard. (Select below + ⌘C if you need it again.)'
            : 'Clipboard write blocked — select the text below and ⌘C to copy.';
    }

    const cleanup = () => {
        dialog.hidden = true;
        copyBtn.removeEventListener('click', onCopy);
        closeBtn.removeEventListener('click', onClose);
        document.removeEventListener('keydown', onKey, true);
    };
    const onCopy = async () => {
        const ok = await writeClipboard();
        copyBtn.textContent = ok ? 'Copied ✓' : 'Copy failed';
        setTimeout(() => { copyBtn.textContent = 'Copy again'; }, 1400);
    };
    const onClose = () => cleanup();
    const onKey = (e) => {
        if (e.key === 'Escape') { e.preventDefault(); onClose(); }
    };

    copyBtn.addEventListener('click', onCopy);
    closeBtn.addEventListener('click', onClose);
    document.addEventListener('keydown', onKey, true);

    dialog.hidden = false;
    closeBtn.focus();
}

function bindClearSamplesButton() {
    const btn = document.getElementById('clear-samples-btn');
    if (!btn) return;

    btn.addEventListener('click', () => {
        if (btn.disabled) return;
        showConfirmDialog({
            title:        'Clear all samples?',
            message:      'All loaded samples will be removed from the sample map. '
                        + 'Active notes will finish playing, but new note-ons will '
                        + 'produce silence until samples are loaded again. This '
                        + 'cannot be undone.',
            confirmLabel: 'Clear',
            destructive:  true,
            onConfirm:    async () => {
                if (!window.__JUCE__) return;
                try {
                    const fn = Juce.getNativeFunction('clearSampleMap');
                    await fn();
                    // sampleMapUpdated push event drives grid + button state
                    // refresh — no further work needed here.
                } catch (e) {
                    console.error('[sampler-app] clearSampleMap failed:', e);
                }
            },
        });
    });
}

function bindHostDragEvents() {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;

    const zone = () => document.getElementById('folder-drop-zone');

    const setHover = (over) => {
        const z = zone();
        if (!z) return;
        z.classList.toggle('drag-over', !!over);
    };

    try {
        window.__JUCE__.backend.addEventListener('hostFileDragMove', (payload) => {
            // Payload arrives parsed: {x, y}.
            const z = zone();
            if (!z) return;
            const r = z.getBoundingClientRect();
            const inside = payload && typeof payload.x === 'number' && typeof payload.y === 'number'
                ? (payload.x >= r.left && payload.x < r.right
                   && payload.y >= r.top && payload.y < r.bottom)
                : false;
            setHover(inside);
        });

        window.__JUCE__.backend.addEventListener('hostFileDragExit', () => {
            setHover(false);
        });
    } catch (e) {
        console.warn('[sampler-app] host drag event subscription failed:', e);
    }
}

// ============================================================================
// Toast queue (Phase 3.3 Task 21)
// ============================================================================
//
// Single-element queue: each toast displays for 3 s, fades in/out via CSS.
// New toasts replace the previous one (re-arming the timer). Subscribed to
// the C++ "toast" event so file-drop routing (filesDropped) can surface
// invalid-target messages without a custom JS round-trip.

let toastTimer = null;

function showToast(message) {
    const region = document.getElementById('toast-region');
    if (!region) return;

    // Replace any existing toast element to keep the queue single-element.
    region.innerHTML = '';

    const el = document.createElement('div');
    el.className = 'toast';
    el.textContent = message;
    region.appendChild(el);

    // Trigger entrance transition on next frame.
    requestAnimationFrame(() => el.classList.add('show'));

    if (toastTimer) clearTimeout(toastTimer);
    toastTimer = setTimeout(() => {
        el.classList.remove('show');
        // Remove from DOM after the fade-out completes (250 ms — matches
        // the CSS transition duration plus a small safety margin).
        setTimeout(() => {
            if (el.parentNode === region) region.removeChild(el);
        }, 300);
        toastTimer = null;
    }, 3000);
}

function bindToastEventListener() {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;
    try {
        window.__JUCE__.backend.addEventListener('toast', (payload) => {
            // Payload may be a parsed string (single arg) or an object; for
            // file-drop emit we send a bare String.
            const msg = (typeof payload === 'string') ? payload
                       : (payload && typeof payload === 'object' && 'message' in payload)
                            ? payload.message
                            : String(payload || '');
            if (msg) showToast(msg);
        });
    } catch (e) {
        console.warn('[sampler-app] toast event subscription failed:', e);
    }
}

// ============================================================================
// Loop-point editor (Phase 3.4 — DSP-06 + UI-02)
// ============================================================================
//
// On openLoopEditor(midi, vel):
//   - await Juce.getNativeFunction('getWaveformPeaks')(midi, vel, 512)
//   - parse JSON snapshot per RESEARCH §RQ3-5 schema
//   - render min/max envelope on DPR-aware canvas
//   - draw two draggable vertical markers (start, end) at sample positions
//   - Apply → Juce.getNativeFunction('overrideLoopPoints')(midi, vel, start, end, 8)
//   - Reset → Juce.getNativeFunction('resetLoopToAutoDetect')(midi, vel) → re-fetch
//   - Cancel / X / Esc → close panel, no writeback
//   - Reset disabled when loopMode === "one-shot" (EC3-7)
//
// Markers are pure JS overlay — drag does NOT re-call getWaveformPeaks; the
// peak data is cached in `editorState.snap` for the duration of the panel
// session. Apply persists; Cancel discards.

const LOOP_EDITOR_BINS = 512;
const MARKER_HIT_PX = 8;       // Within this many CSS px of marker → grab handle
const APPLY_TOAST = 'New loop points apply to next note-on.';
const ONE_SHOT_TOOLTIP = 'Sample is one-shot — no loop region detected.';

const editorState = {
    open: false,
    midi: -1,
    vel: -1,
    snap: null,           // Last fetched JSON snapshot (peaks + meta).
    loopStart: 0,         // Current marker positions (samples).
    loopEnd: 0,
    dragMarker: null,     // 'start' | 'end' | null
    pointerId: -1,
    variantIndex: 0,      // v1.8.0: which variant we're currently editing.
    variantCount: 1,      // v1.8.0: total variants in the cell.
};

function isOneShot(snap) {
    if (!snap || typeof snap.loopMode !== 'string') return false;
    const m = snap.loopMode.toLowerCase();
    return m === 'one-shot' || m === 'oneshot';
}

async function openLoopEditor(midi, vel, variantIndex = 0) {
    if (!window.__JUCE__) return;

    try {
        const fn = Juce.getNativeFunction('getWaveformPeaks');
        const json = await fn(midi, vel, LOOP_EDITOR_BINS, variantIndex);
        const snap = (typeof json === 'string') ? JSON.parse(json) : json;
        if (!snap || !Array.isArray(snap.peaks) || snap.peaks.length === 0) {
            console.warn('[sampler-app] openLoopEditor: empty peaks snapshot', snap);
            showToast('Unable to load waveform for this cell.');
            return;
        }
        editorState.open = true;
        editorState.midi = midi;
        editorState.vel = vel;
        editorState.snap = snap;
        editorState.loopStart = Number.isFinite(snap.loopStart) ? snap.loopStart : 0;
        editorState.loopEnd   = Number.isFinite(snap.loopEnd)   ? snap.loopEnd   : 0;

        // v1.8.0: variant tab strip surfaces when the cell has > 1 variant.
        editorState.variantIndex = Number.isFinite(snap.variantIndex) ? snap.variantIndex : variantIndex;
        editorState.variantCount = Number.isFinite(snap.variantCount) ? snap.variantCount : 1;

        populateLoopEditorHeader(snap);
        renderVariantTabStrip();
        showLoopEditorPanel();

        // Defer canvas draw one frame so the panel transition has applied
        // and clientWidth/clientHeight reflect the open state.
        requestAnimationFrame(() => {
            redrawLoopEditor();
        });
    } catch (e) {
        console.error('[sampler-app] openLoopEditor failed:', e);
    }
}

// v1.8.0: variant tab strip — one tab per variant. Shown only when cell
// has > 1 variant. Click switches the active variant; the snapshot reload
// happens via openLoopEditor with the selected index. Loop points are
// per-variant in v1.8.0, so each tab carries its own state on the C++ side.
function renderVariantTabStrip() {
    const wrap = document.getElementById('le-variant-tabs');
    if (!wrap) return;

    wrap.innerHTML = '';
    if (editorState.variantCount <= 1) {
        wrap.style.display = 'none';
        return;
    }
    wrap.style.display = '';

    const label = document.createElement('span');
    label.className = 'le-variant-label';
    label.textContent = `Variant ${editorState.variantIndex + 1} of ${editorState.variantCount}`;
    wrap.appendChild(label);

    for (let i = 0; i < editorState.variantCount; ++i) {
        const tab = document.createElement('button');
        tab.type = 'button';
        tab.className = 'le-variant-tab';
        if (i === editorState.variantIndex) tab.classList.add('active');
        tab.textContent = String(i + 1);
        tab.title = `Switch to variant ${i + 1}`;
        tab.addEventListener('click', () => {
            if (i === editorState.variantIndex) return;
            // Re-open with the chosen variant — fresh peaks + loop points.
            openLoopEditor(editorState.midi, editorState.vel, i);
        });
        wrap.appendChild(tab);
    }
}

function populateLoopEditorHeader(snap) {
    const fnEl    = document.getElementById('le-filename');
    const midiEl  = document.getElementById('le-midi');
    const velEl   = document.getElementById('le-vel');
    const modeEl  = document.getElementById('le-loop-mode');
    if (fnEl)   fnEl.textContent   = snap.filename || '(unknown)';
    if (midiEl) midiEl.textContent = `MIDI ${snap.midiNote}`;
    if (velEl)  velEl.textContent  = String(snap.velocityLayer);
    if (modeEl) modeEl.textContent = snap.loopMode || '—';
    updateLoopMetaLabels();
    updateResetButtonState(snap);
}

function updateLoopMetaLabels() {
    const snap = editorState.snap;
    if (!snap) return;
    const sr = snap.sourceSampleRate > 0 ? snap.sourceSampleRate : 48000;
    const startMs = (editorState.loopStart / sr) * 1000.0;
    const endMs   = (editorState.loopEnd   / sr) * 1000.0;
    const sEl = document.getElementById('le-loop-start-ms');
    const eEl = document.getElementById('le-loop-end-ms');
    if (sEl) sEl.textContent = startMs.toFixed(1);
    if (eEl) eEl.textContent = endMs.toFixed(1);
}

function updateResetButtonState(snap) {
    const btn = document.getElementById('loop-reset');
    if (!btn) return;
    if (isOneShot(snap)) {
        btn.disabled = true;
        btn.title = ONE_SHOT_TOOLTIP;
    } else {
        btn.disabled = false;
        btn.title = '';
    }
}

// v1.5.0 — panel is inline + always visible. show/close just toggle the
// .no-selection class which swaps between placeholder and full editor.
function showLoopEditorPanel() {
    const panel = document.getElementById('loop-editor-panel');
    if (!panel) return;
    panel.classList.remove('no-selection');
    // No grid-width change anymore (panel doesn't overlap the grid), but
    // republish layout in case the panel's appearance shifts cell rects.
    requestAnimationFrame(() => publishCellLayout());
}

function closeLoopEditor() {
    const panel = document.getElementById('loop-editor-panel');
    if (panel) panel.classList.add('no-selection');
    editorState.open = false;
    editorState.snap = null;
    editorState.dragMarker = null;
    editorState.pointerId = -1;
    requestAnimationFrame(() => publishCellLayout());
}

function redrawLoopEditor() {
    const canvas = document.getElementById('waveform-canvas');
    if (!canvas || !editorState.snap) return;

    // DPR-aware backing store (memory pitfall #6).
    const dpr = window.devicePixelRatio || 1;
    const cssW = Math.max(1, Math.floor(canvas.clientWidth));
    const cssH = Math.max(1, Math.floor(canvas.clientHeight));
    const targetW = Math.round(cssW * dpr);
    const targetH = Math.round(cssH * dpr);
    if (canvas.width !== targetW)  canvas.width  = targetW;
    if (canvas.height !== targetH) canvas.height = targetH;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

    // Background
    ctx.clearRect(0, 0, cssW, cssH);
    const bgGrad = ctx.createLinearGradient(0, 0, 0, cssH);
    bgGrad.addColorStop(0, 'rgba(245, 230, 211, 0.3)');
    bgGrad.addColorStop(1, 'rgba(235, 217, 199, 0.5)');
    ctx.fillStyle = bgGrad;
    ctx.fillRect(0, 0, cssW, cssH);

    // Centerline
    ctx.strokeStyle = 'rgba(139, 115, 85, 0.35)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, cssH / 2);
    ctx.lineTo(cssW, cssH / 2);
    ctx.stroke();

    // Min/max envelope
    const peaks = editorState.snap.peaks;
    const bins = peaks.length;
    if (bins === 0) return;
    const binW = cssW / bins;

    // Filled area
    ctx.fillStyle = 'rgba(184, 134, 11, 0.35)';  // antique-gold @ 35 %
    ctx.beginPath();
    for (let i = 0; i < bins; ++i) {
        const px = i * binW;
        const max = peaks[i][1];
        const y = cssH * 0.5 - max * (cssH * 0.48);
        if (i === 0) ctx.moveTo(px, y);
        else ctx.lineTo(px, y);
    }
    for (let i = bins - 1; i >= 0; --i) {
        const px = i * binW;
        const min = peaks[i][0];
        const y = cssH * 0.5 - min * (cssH * 0.48);
        ctx.lineTo(px, y);
    }
    ctx.closePath();
    ctx.fill();

    // Stroke outline (warm-brown)
    ctx.strokeStyle = '#5C4033';
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let i = 0; i < bins; ++i) {
        const px = i * binW;
        const max = peaks[i][1];
        const y = cssH * 0.5 - max * (cssH * 0.48);
        if (i === 0) ctx.moveTo(px, y);
        else ctx.lineTo(px, y);
    }
    ctx.stroke();
    ctx.beginPath();
    for (let i = 0; i < bins; ++i) {
        const px = i * binW;
        const min = peaks[i][0];
        const y = cssH * 0.5 - min * (cssH * 0.48);
        if (i === 0) ctx.moveTo(px, y);
        else ctx.lineTo(px, y);
    }
    ctx.stroke();

    // Markers — only draw if not one-shot (zero-region markers would be at x=0).
    drawMarker(ctx, cssW, cssH, sampleToX(editorState.loopStart, cssW), 'start');
    drawMarker(ctx, cssW, cssH, sampleToX(editorState.loopEnd,   cssW), 'end');
}

function drawMarker(ctx, cssW, cssH, x, which) {
    if (!Number.isFinite(x)) return;
    const colour = which === 'start' ? '#B8860B' : '#C0392B';
    ctx.strokeStyle = colour;
    ctx.fillStyle = colour;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, cssH);
    ctx.stroke();

    // Handle triangle at top
    const handleSize = 6;
    ctx.beginPath();
    ctx.moveTo(x - handleSize, 0);
    ctx.lineTo(x + handleSize, 0);
    ctx.lineTo(x, handleSize);
    ctx.closePath();
    ctx.fill();
}

function sampleToX(sample, cssW) {
    const snap = editorState.snap;
    if (!snap || !snap.lengthSamples) return 0;
    return (sample / snap.lengthSamples) * cssW;
}

function xToSample(x, cssW) {
    const snap = editorState.snap;
    if (!snap || !snap.lengthSamples) return 0;
    const s = Math.round((x / cssW) * snap.lengthSamples);
    return Math.max(0, Math.min(snap.lengthSamples - 1, s));
}

function bindLoopEditorEvents() {
    const closeBtn  = document.getElementById('loop-close');
    const resetBtn  = document.getElementById('loop-reset');
    const cancelBtn = document.getElementById('loop-cancel');
    const applyBtn  = document.getElementById('loop-apply');
    const canvas    = document.getElementById('waveform-canvas');

    if (closeBtn)  closeBtn.addEventListener('click', closeLoopEditor);
    if (cancelBtn) cancelBtn.addEventListener('click', closeLoopEditor);

    if (resetBtn) {
        resetBtn.addEventListener('click', async () => {
            if (resetBtn.disabled || !editorState.open) return;
            try {
                const fn = Juce.getNativeFunction('resetLoopToAutoDetect');
                await fn(editorState.midi, editorState.vel, editorState.variantIndex);
                // Re-fetch peaks to get fresh marker positions + mode.
                const get = Juce.getNativeFunction('getWaveformPeaks');
                const json = await get(editorState.midi, editorState.vel,
                                       LOOP_EDITOR_BINS, editorState.variantIndex);
                const snap = (typeof json === 'string') ? JSON.parse(json) : json;
                if (snap && Array.isArray(snap.peaks)) {
                    editorState.snap = snap;
                    editorState.loopStart = Number.isFinite(snap.loopStart) ? snap.loopStart : 0;
                    editorState.loopEnd   = Number.isFinite(snap.loopEnd)   ? snap.loopEnd   : 0;
                    populateLoopEditorHeader(snap);
                    redrawLoopEditor();
                }
            } catch (e) {
                console.error('[sampler-app] resetLoopToAutoDetect failed:', e);
            }
        });
    }

    if (applyBtn) {
        applyBtn.addEventListener('click', async () => {
            if (!editorState.open) return;
            try {
                const fn = Juce.getNativeFunction('overrideLoopPoints');
                await fn(editorState.midi, editorState.vel,
                         editorState.loopStart, editorState.loopEnd, 8,
                         editorState.variantIndex);
                showToast(APPLY_TOAST);
                // Don't auto-close — user may want to keep iterating.
                // The sampleMapUpdated push event will refresh the grid;
                // editor stays open with current values reflected.
            } catch (e) {
                console.error('[sampler-app] overrideLoopPoints failed:', e);
            }
        });
    }

    if (canvas) {
        canvas.addEventListener('pointerdown', onCanvasPointerDown);
        canvas.addEventListener('pointermove', onCanvasPointerMove);
        canvas.addEventListener('pointerup',   onCanvasPointerUp);
        canvas.addEventListener('pointercancel', onCanvasPointerUp);
    }

    // Esc closes the editor.
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape' && editorState.open) {
            e.preventDefault();
            closeLoopEditor();
        }
    });
}

function onCanvasPointerDown(e) {
    if (!editorState.open || !editorState.snap) return;
    const canvas = e.currentTarget;
    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const cssW = rect.width;

    // Hit-test against the two markers; prefer the closer one.
    const startX = sampleToX(editorState.loopStart, cssW);
    const endX   = sampleToX(editorState.loopEnd,   cssW);
    const dStart = Math.abs(x - startX);
    const dEnd   = Math.abs(x - endX);
    const HIT = MARKER_HIT_PX;

    let target = null;
    if (dStart <= HIT && dEnd <= HIT) {
        target = (dStart <= dEnd) ? 'start' : 'end';
    } else if (dStart <= HIT) {
        target = 'start';
    } else if (dEnd <= HIT) {
        target = 'end';
    }

    if (target) {
        editorState.dragMarker = target;
        editorState.pointerId = e.pointerId;
        canvas.setPointerCapture(e.pointerId);
        e.preventDefault();
    }
}

function onCanvasPointerMove(e) {
    if (!editorState.dragMarker || e.pointerId !== editorState.pointerId) return;
    const canvas = e.currentTarget;
    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const cssW = rect.width;
    let s = xToSample(x, cssW);

    // Maintain ordering: start < end with a 16-sample minimum gap (matches
    // the LoopDetector's defensive guard so we don't allow degenerate loops).
    const MIN_GAP = 16;
    const lengthSamples = editorState.snap.lengthSamples;
    if (editorState.dragMarker === 'start') {
        s = Math.max(0, Math.min(s, editorState.loopEnd - MIN_GAP));
        editorState.loopStart = s;
    } else {
        s = Math.max(editorState.loopStart + MIN_GAP, Math.min(s, lengthSamples));
        editorState.loopEnd = s;
    }

    updateLoopMetaLabels();
    redrawLoopEditor();
}

function onCanvasPointerUp(e) {
    if (e.pointerId !== editorState.pointerId) return;
    const canvas = e.currentTarget;
    if (canvas.hasPointerCapture && canvas.hasPointerCapture(e.pointerId)) {
        try { canvas.releasePointerCapture(e.pointerId); } catch (_) { /* ignore */ }
    }
    editorState.dragMarker = null;
    editorState.pointerId = -1;
}

// Re-publish cell layout AND redraw the loop editor on resize. The canvas
// CSS sizing is fluid (calc(100% - 0px)) so its clientWidth changes with
// the panel; we need to update the backing store + rerender the envelope.
function bindLoopEditorResize() {
    if (typeof ResizeObserver !== 'function') return;
    const canvas = document.getElementById('waveform-canvas');
    if (!canvas) return;
    const obs = new ResizeObserver(() => {
        if (editorState.open) requestAnimationFrame(() => redrawLoopEditor());
    });
    obs.observe(canvas);
}

// ============================================================================
// v1.3.0 — Save/Load preset buttons + missing-folder modal
// ============================================================================
//
// The plugin already round-trips full state (params + sample folder + tuning)
// through DAW project save/load via PluginProcessor::get/setStateInformation.
// The Save/Load preset buttons surface that same ValueTree as a portable
// .omspreset file so users can share configurations across projects on the
// same machine (path-only — sample data is referenced, not embedded).
//
// Missing-folder modal: when DAW project reopen tries to restore a folder
// path that no longer exists, the C++ side fires a `folderMissing` WebView
// event AND parks the path in the processor for the boot-time race case
// (state restore happens before the WebView attaches its listener). On boot
// we register the listener first, then pull any pending state — covers both.

function bindPresetButtons () {
    const saveBtn = document.getElementById('save-preset-btn');
    const loadBtn = document.getElementById('load-preset-btn');

    if (saveBtn) {
        saveBtn.addEventListener('click', async () => {
            if (!window.__JUCE__) return;
            try {
                const fn = Juce.getNativeFunction('saveCurrentPreset');
                const ok = await fn();
                if (ok === true) showToast('Preset saved');
                // Cancel resolves false — silent (user dismissed the picker).
            } catch (e) {
                console.error('[sampler-app] saveCurrentPreset failed:', e);
                showToast('Save preset failed');
            }
        });
    }

    if (loadBtn) {
        loadBtn.addEventListener('click', async () => {
            if (!window.__JUCE__) return;
            try {
                const fn = Juce.getNativeFunction('loadPreset');
                const ok = await fn();
                if (ok === true) {
                    showToast('Preset loaded');
                    // The processor's sampleMapUpdated push covers the grid;
                    // refresh the tuning readout (tab-tuning lazy-mounts the
                    // panel, but the header readout polls on demand).
                    refreshTuningReadout();
                    if (tuningPanelInstance && typeof tuningPanelInstance.refresh === 'function') {
                        try { await tuningPanelInstance.refresh(); } catch (_) { /* ignore */ }
                    }
                } else if (ok === false) {
                    // false from a non-cancel path means parse/restore failed;
                    // cancel also resolves false, so don't toast on that.
                }
            } catch (e) {
                console.error('[sampler-app] loadPreset failed:', e);
                showToast('Load preset failed');
            }
        });
    }
}

// v1.12.0: payload is now an object {path, kind, name} (was a bare string
// in v1.3.0–v1.11.x). String-form payloads are normalised in
// `subscribeFolderMissingEvent` and `pullPendingMissingFolder` before
// reaching this function.
//
//   kind="filesystem" — original behaviour: saved disk path is gone. The
//                       modal shows the path and offers a Locate folder
//                       picker that delegates to the existing reload flow.
//   kind="drag-drop"  — v1.12.0: the load was a WebView drag-drop with no
//                       embed. The temp dir is reaped at next-session-
//                       start, so we never had a stable path to surface.
//                       Show the original folder name (lifted from
//                       FileSystemEntry::name at drop time) and direct the
//                       user to re-drag or browse.
function showMissingFolderDialog (info) {
    const dialog    = document.getElementById('missing-folder-dialog');
    const titleEl   = document.getElementById('missing-folder-dialog-title');
    const messageEl = document.getElementById('missing-folder-dialog-message');
    const pathEl    = document.getElementById('missing-folder-saved-path');
    const skipBtn   = document.getElementById('missing-folder-skip-btn');
    const locateBtn = document.getElementById('missing-folder-locate-btn');
    if (!dialog || !titleEl || !messageEl || !pathEl || !skipBtn || !locateBtn) return;

    const safe      = (info && typeof info === 'object') ? info : {};
    const path      = typeof safe.path === 'string' ? safe.path : '';
    const kind      = typeof safe.kind === 'string' ? safe.kind : 'filesystem';
    const givenName = typeof safe.name === 'string' ? safe.name : '';

    if (kind === 'drag-drop') {
        titleEl.textContent = 'Drag-dropped samples not embedded';
        const friendly = givenName || 'this folder';
        messageEl.textContent =
            `Samples were drag-dropped from "${friendly}" without "Embed audio" `
            + `enabled, so they could not be re-loaded automatically. `
            + `Re-drag the folder onto the plugin, or browse to its current location.`;
        pathEl.textContent = '';   // no path to show; clear the legacy slot
        pathEl.style.display = 'none';
        locateBtn.textContent = 'Browse for folder…';
    } else {
        titleEl.textContent = 'Sample folder not found';
        // Derive a friendly folder name from the saved path for the message
        // (or use the explicit name when provided).
        let folderName = givenName;
        if (!folderName && path) {
            const sep = path.lastIndexOf('/') >= 0 ? '/' : '\\';
            const idx = path.lastIndexOf(sep);
            folderName = idx >= 0 ? path.substring(idx + 1) : path;
        }
        messageEl.textContent = folderName
            ? `The sample folder "${folderName}" was not found at its saved location. Locate it now, or skip and load samples manually.`
            : 'The saved sample folder was not found. Locate it now, or skip and load samples manually.';
        pathEl.textContent = path || '(empty path)';
        pathEl.style.display = '';
        locateBtn.textContent = 'Locate folder…';
    }

    const cleanup = () => {
        dialog.hidden = true;
        skipBtn.removeEventListener('click', onSkip);
        locateBtn.removeEventListener('click', onLocate);
        document.removeEventListener('keydown', onKey, true);
    };
    const onSkip = async () => {
        cleanup();
        if (!window.__JUCE__) return;
        try {
            const fn = Juce.getNativeFunction('dismissMissingFolder');
            await fn();
        } catch (e) {
            console.warn('[sampler-app] dismissMissingFolder failed:', e);
        }
    };
    const onLocate = async () => {
        cleanup();
        if (!window.__JUCE__) return;
        try {
            const fn = Juce.getNativeFunction('locateMissingFolder');
            const ok = await fn();
            // Cancel resolves false — leave the pending state intact so the
            // user can try again from the next setStateInformation event or
            // by manually using "Load Folder…".
            if (ok === true) showToast('Folder located — loading…');
        } catch (e) {
            console.error('[sampler-app] locateMissingFolder failed:', e);
            showToast('Locate folder failed');
        }
    };
    const onKey = (e) => {
        if (e.key === 'Escape') { e.preventDefault(); onSkip(); }
        else if (e.key === 'Enter') { e.preventDefault(); onLocate(); }
    };

    skipBtn.addEventListener('click', onSkip);
    locateBtn.addEventListener('click', onLocate);
    document.addEventListener('keydown', onKey, true);
    dialog.hidden = false;
    locateBtn.focus();
}

function subscribeFolderMissingEvent () {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;
    try {
        window.__JUCE__.backend.addEventListener('folderMissing', (payload) => {
            // v1.12.0: payload is now an object {path, kind, name}. Older
            // string-form payloads are no longer emitted by the plugin, but
            // we accept them defensively (host could replay an old event).
            let info;
            if (payload && typeof payload === 'object') {
                info = payload;
            } else if (typeof payload === 'string') {
                info = { path: payload, kind: 'filesystem', name: '' };
            } else {
                info = { path: '', kind: 'filesystem', name: '' };
            }
            showMissingFolderDialog(info);
        });
    } catch (e) {
        console.warn('[sampler-app] folderMissing subscription failed:', e);
    }
}

// v1.8.0: ambiguous-duplicates modal. Fired when a folder load contained
// multiple files for the same (midi, layer) without rr/take/tk tokens.
// User decides: treat as round-robin variants OR cancel the load.
function showAmbiguousDuplicatesDialog (dups) {
    const dialog  = document.getElementById('rr-confirm-dialog');
    const listEl  = document.getElementById('rr-confirm-list');
    const okBtn   = document.getElementById('rr-confirm-accept');
    const noBtn   = document.getElementById('rr-confirm-cancel');
    if (!dialog || !listEl || !okBtn || !noBtn) {
        // Fallback: send a quick confirm via window.confirm so the user is
        // never blocked by a missing modal element.
        const summary = dups.map(d => `MIDI ${d.midiNote}/L${d.velocityLayer}: ${(d.filenames || []).join(', ')}`).join('\n');
        const accept = window.confirm(`Multiple files share the same note/layer:\n\n${summary}\n\nTreat as round-robin variants?`);
        sendRrConfirmation(accept);
        return;
    }

    listEl.innerHTML = '';
    for (const d of dups) {
        const item = document.createElement('li');
        const head = document.createElement('div');
        head.className = 'rr-confirm-cell-head';
        head.textContent = `MIDI ${d.midiNote} · Layer ${d.velocityLayer + 1}`;
        item.appendChild(head);
        const fns = document.createElement('ul');
        fns.className = 'rr-confirm-filename-list';
        for (const fn of (d.filenames || [])) {
            const li = document.createElement('li');
            li.textContent = fn;
            fns.appendChild(li);
        }
        item.appendChild(fns);
        listEl.appendChild(item);
    }

    const cleanup = () => {
        dialog.hidden = true;
        okBtn.removeEventListener('click', onAccept);
        noBtn.removeEventListener('click', onCancel);
        document.removeEventListener('keydown', onKey, true);
    };
    const onAccept = () => { cleanup(); sendRrConfirmation(true); };
    const onCancel = () => { cleanup(); sendRrConfirmation(false); };
    const onKey = (e) => {
        if (e.key === 'Escape') { e.preventDefault(); onCancel(); }
        if (e.key === 'Enter')  { e.preventDefault(); onAccept(); }
    };

    okBtn.addEventListener('click', onAccept);
    noBtn.addEventListener('click', onCancel);
    document.addEventListener('keydown', onKey, true);
    dialog.hidden = false;
    okBtn.focus();
}

async function sendRrConfirmation (accept) {
    if (!window.__JUCE__) return;
    try {
        const fn = Juce.getNativeFunction('confirmRoundRobinLoad');
        await fn(!!accept);
    } catch (e) {
        console.warn('[sampler-app] confirmRoundRobinLoad failed:', e);
    }
}

function subscribeAmbiguousDuplicatesEvent () {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;
    try {
        window.__JUCE__.backend.addEventListener('ambiguousDuplicates', (payload) => {
            let dups = [];
            if (typeof payload === 'string') {
                try { dups = JSON.parse(payload); } catch (_) { dups = []; }
            } else if (Array.isArray(payload)) {
                dups = payload;
            }
            if (Array.isArray(dups) && dups.length > 0) {
                showAmbiguousDuplicatesDialog(dups);
            }
        });
    } catch (e) {
        console.warn('[sampler-app] ambiguousDuplicates subscription failed:', e);
    }
}

async function pullPendingMissingFolder () {
    // Boot-time race: setStateInformation may have run before this listener
    // was registered. Pull the parked path (if any) and surface the modal.
    //
    // v1.12.0: returns an object {path, kind, name}. Old v1.11.x C++ would
    // return a bare string — accept both forms defensively in case a stale
    // host bundle ever round-trips the value.
    if (!window.__JUCE__) return;
    try {
        const fn = Juce.getNativeFunction('getPendingMissingFolder');
        const result = await fn();
        let info = null;
        if (result && typeof result === 'object') {
            const hasContent = (typeof result.path === 'string' && result.path.length > 0)
                            || (typeof result.name === 'string' && result.name.length > 0);
            if (hasContent) info = result;
        } else if (typeof result === 'string' && result.length > 0) {
            info = { path: result, kind: 'filesystem', name: '' };
        }
        if (info) showMissingFolderDialog(info);
    } catch (e) {
        // Silent — older builds may lack the function (defence-in-depth).
    }
}

// ============================================================================
// Boot
// ============================================================================
// ============================================================================
// v1.14.0 — Playing Techniques tab strip + KS picker
// ============================================================================
//
// Single-file feature scope: rendering, native-fn bridging, KS edit, rename
// modal. State is mirrored from the C++ side — every render reads the
// authoritative `getTechniqueState()` snapshot and never trusts a stale local
// copy. The panel hides itself when the user has not opted in (count==1 AND
// ks_enabled==false) so the v1.13.0 visual contract survives untouched.

const techniqueState = {
    names: [],
    active: 0,
    count: 1,
    ksEnabled: false,
    ksLow: 0,
    ksHigh: 9,
};

async function pullTechniqueState() {
    if (!window.__JUCE__) return;
    try {
        const fn = Juce.getNativeFunction('getTechniqueState');
        const result = await fn();
        const obj = (typeof result === 'string') ? JSON.parse(result) : result;
        if (!obj) return;

        const prevActive = techniqueState.active;

        techniqueState.names     = Array.isArray(obj.names) ? obj.names.slice() : [];
        techniqueState.active    = Number.isFinite(obj.active)    ? obj.active    : 0;
        techniqueState.count     = Number.isFinite(obj.count)     ? obj.count     : 1;
        techniqueState.ksEnabled = !!obj.ksEnabled;
        techniqueState.ksLow     = Number.isFinite(obj.ksLow)     ? obj.ksLow     : 0;
        techniqueState.ksHigh    = Number.isFinite(obj.ksHigh)    ? obj.ksHigh    : 9;

        renderTechniqueBar();

        // v1.15.0: technique count drives the trigger-panel visibility
        // (single-technique libraries hide it) and slot dimming. Refresh
        // the panel against the cached trigger snapshot whenever the
        // technique state changes — cheap, no native fn call.
        renderTriggerPanel();

        // Active-technique change → re-render the grid against the cached
        // snapshot so cells from the new technique slot appear immediately.
        if (prevActive !== techniqueState.active && lastSampleMapSnapshot) {
            renderGrid(lastSampleMapSnapshot);
        }
    } catch (err) {
        console.warn('[sampler-app] pullTechniqueState failed', err);
    }
}

function subscribeTechniqueStateUpdates() {
    if (!window.__JUCE__) return;
    window.__JUCE__.backend.addEventListener('techniqueStateUpdated', () => {
        pullTechniqueState();
    });
}

function renderTechniqueBar() {
    const bar = document.getElementById('technique-bar');
    if (!bar) return;

    // Visibility: stay collapsed until the user has either grown beyond a
    // single technique slot OR enabled keyswitches. Matches the back-compat
    // contract.
    const expanded = techniqueState.count > 1 || techniqueState.ksEnabled;
    bar.classList.toggle('hidden', !expanded);

    // Per-slot cell counts derived from the cached snapshot. Used to
    // surface "where did my files go?" feedback on the tab strip and to
    // dim empty slots.
    const cellCountsByTech = new Array(8).fill(0);
    if (lastSampleMapSnapshot && Array.isArray(lastSampleMapSnapshot.cells)) {
        for (const c of lastSampleMapSnapshot.cells) {
            const t = Number.isFinite(c.technique) ? c.technique : 0;
            if (t >= 0 && t < 8) cellCountsByTech[t]++;
        }
    }

    // Tabs
    const tabs = document.getElementById('technique-tabs');
    if (tabs) {
        tabs.innerHTML = '';
        for (let i = 0; i < techniqueState.count; ++i) {
            const name  = techniqueState.names[i] || `slot ${i + 1}`;
            const count = cellCountsByTech[i];
            const btn = document.createElement('button');
            btn.type = 'button';
            btn.className = 'tech-tab';
            if (i === techniqueState.active) btn.classList.add('active');
            if (count === 0) btn.classList.add('empty');
            btn.dataset.tech = String(i);
            btn.setAttribute('role', 'tab');
            btn.setAttribute('aria-selected', i === techniqueState.active ? 'true' : 'false');
            btn.title = count > 0
                ? `Technique ${i + 1}: ${name} — ${count} cell${count === 1 ? '' : 's'} loaded  (right-click to rename)`
                : `Technique ${i + 1}: ${name} — empty  (right-click to rename)`;
            btn.textContent = count > 0 ? `${name} (${count})` : name;
            tabs.appendChild(btn);
        }
    }

    // Add / remove buttons
    const addBtn = document.getElementById('technique-add');
    const remBtn = document.getElementById('technique-remove');
    if (addBtn) addBtn.disabled = techniqueState.count >= 8;
    if (remBtn) remBtn.disabled = techniqueState.count <= 1;

    // KS controls
    const ksToggle = document.getElementById('technique-ks-enabled');
    const ksLow    = document.getElementById('technique-ks-low');
    const ksHigh   = document.getElementById('technique-ks-high');
    const ksWrap   = document.getElementById('technique-ks-controls');
    if (ksToggle) ksToggle.checked = techniqueState.ksEnabled;
    if (ksLow)    ksLow.value      = String(techniqueState.ksLow);
    if (ksHigh)   ksHigh.value     = String(techniqueState.ksHigh);
    if (ksWrap)   ksWrap.classList.toggle('disabled', !techniqueState.ksEnabled);
}

let techniqueRenameTargetIndex = -1;

function openTechniqueRenameDialog(index) {
    if (index < 0 || index >= techniqueState.count) return;
    techniqueRenameTargetIndex = index;
    const dialog = document.getElementById('technique-rename-dialog');
    const idxEl  = document.getElementById('technique-rename-index');
    const input  = document.getElementById('technique-rename-input');
    if (!dialog || !input) return;
    if (idxEl) idxEl.textContent = String(index + 1);
    input.value = techniqueState.names[index] || '';
    dialog.hidden = false;
    setTimeout(() => input.focus(), 0);
}

function closeTechniqueRenameDialog() {
    const dialog = document.getElementById('technique-rename-dialog');
    if (dialog) dialog.hidden = true;
    techniqueRenameTargetIndex = -1;
}

async function commitTechniqueRename() {
    const input = document.getElementById('technique-rename-input');
    if (!input || techniqueRenameTargetIndex < 0) return;
    const trimmed = input.value.trim();
    if (!trimmed) {
        closeTechniqueRenameDialog();
        return;
    }
    if (window.__JUCE__) {
        try {
            const fn = Juce.getNativeFunction('setTechniqueName');
            await fn(techniqueRenameTargetIndex, trimmed);
        } catch (err) {
            console.warn('[sampler-app] setTechniqueName failed', err);
        }
    }
    closeTechniqueRenameDialog();
}

function bindTechniqueBar() {
    const tabs = document.getElementById('technique-tabs');
    if (tabs) {
        tabs.addEventListener('click', async (e) => {
            const btn = e.target.closest('.tech-tab');
            if (!btn) return;
            const idx = parseInt(btn.dataset.tech, 10);
            if (!Number.isFinite(idx)) return;
            if (window.__JUCE__) {
                const fn = Juce.getNativeFunction('setActiveTechnique');
                await fn(idx);
            } else {
                techniqueState.active = idx;
                renderTechniqueBar();
            }
        });
        tabs.addEventListener('contextmenu', (e) => {
            const btn = e.target.closest('.tech-tab');
            if (!btn) return;
            e.preventDefault();
            const idx = parseInt(btn.dataset.tech, 10);
            if (Number.isFinite(idx)) openTechniqueRenameDialog(idx);
        });
    }

    const addBtn = document.getElementById('technique-add');
    if (addBtn) {
        addBtn.addEventListener('click', async () => {
            if (window.__JUCE__) {
                const fn = Juce.getNativeFunction('addTechniqueSlot');
                await fn();
            }
        });
    }

    const remBtn = document.getElementById('technique-remove');
    if (remBtn) {
        remBtn.addEventListener('click', async () => {
            if (window.__JUCE__ && techniqueState.count > 1) {
                const fn = Juce.getNativeFunction('removeTechniqueSlot');
                await fn(techniqueState.count - 1);
            }
        });
    }

    const ksToggle = document.getElementById('technique-ks-enabled');
    if (ksToggle) {
        ksToggle.addEventListener('change', async () => {
            if (!window.__JUCE__) return;
            const fn = Juce.getNativeFunction('setKeyswitchEnabled');
            await fn(!!ksToggle.checked);
        });
    }

    const commitKsRange = async () => {
        if (!window.__JUCE__) return;
        const ksLow  = document.getElementById('technique-ks-low');
        const ksHigh = document.getElementById('technique-ks-high');
        if (!ksLow || !ksHigh) return;
        let lo = parseInt(ksLow.value,  10);
        let hi = parseInt(ksHigh.value, 10);
        if (!Number.isFinite(lo)) lo = 0;
        if (!Number.isFinite(hi)) hi = 9;
        lo = Math.max(0, Math.min(127, lo));
        hi = Math.max(0, Math.min(127, hi));
        if (hi < lo) hi = lo;
        const fn = Juce.getNativeFunction('setKeyswitchRange');
        await fn(lo, hi);
    };

    const ksLow  = document.getElementById('technique-ks-low');
    const ksHigh = document.getElementById('technique-ks-high');
    if (ksLow)  ksLow.addEventListener('change',  commitKsRange);
    if (ksHigh) ksHigh.addEventListener('change', commitKsRange);

    // Rename modal — Save / Cancel + Enter / Escape
    const saveBtn   = document.getElementById('technique-rename-save');
    const cancelBtn = document.getElementById('technique-rename-cancel');
    const input     = document.getElementById('technique-rename-input');
    if (saveBtn)   saveBtn.addEventListener('click',   commitTechniqueRename);
    if (cancelBtn) cancelBtn.addEventListener('click', closeTechniqueRenameDialog);
    if (input) {
        input.addEventListener('keydown', (e) => {
            if (e.key === 'Enter')      { e.preventDefault(); commitTechniqueRename(); }
            else if (e.key === 'Escape'){ e.preventDefault(); closeTechniqueRenameDialog(); }
        });
    }
}

// ============================================================================
// v1.15.0 — CC + PC trigger panel
// ============================================================================
//
// The C++ side owns two 8-slot tables (CC value-range → tech, PC# → tech)
// plus three APVTS gates (cc_select_enabled, cc_number, pc_enabled). The
// audio thread reads these to drive technique routing in processBlock with
// KS > CC > PC > history precedence. The JS layer holds a cached snapshot
// for rendering — single source of truth always lives in C++.

const triggerState = {
    ccEnabled: false,
    ccNumber:  32,
    ccMapping: [],   // 8 entries: { rangeLow, rangeHigh, tech }
    pcEnabled: false,
    pcMapping: [],   // 8 entries: { pc, tech }
};

async function pullTriggerState() {
    if (!window.__JUCE__) return;
    try {
        const fn = Juce.getNativeFunction('getTriggerState');
        const raw = await fn();
        // The native fn returns a juce::DynamicObject which arrives as a
        // plain JS object (matching the getTechniqueState pattern).
        const obj = (typeof raw === 'string') ? JSON.parse(raw) : raw;

        triggerState.ccEnabled = !!obj.ccEnabled;
        triggerState.ccNumber  = Number.isFinite(obj.ccNumber) ? obj.ccNumber : 32;
        triggerState.pcEnabled = !!obj.pcEnabled;

        triggerState.ccMapping = Array.isArray(obj.ccMapping)
            ? obj.ccMapping.slice(0, 8).map(s => ({
                rangeLow:  Number.isFinite(s.rangeLow)  ? s.rangeLow  : 0,
                rangeHigh: Number.isFinite(s.rangeHigh) ? s.rangeHigh : 127,
                tech:      Number.isFinite(s.tech)      ? s.tech      : 0,
            }))
            : [];
        triggerState.pcMapping = Array.isArray(obj.pcMapping)
            ? obj.pcMapping.slice(0, 8).map(s => ({
                pc:   Number.isFinite(s.pc)   ? s.pc   : 0,
                tech: Number.isFinite(s.tech) ? s.tech : 0,
            }))
            : [];

        renderTriggerPanel();
    } catch (err) {
        console.warn('[sampler-app] pullTriggerState failed', err);
    }
}

function subscribeTriggerStateUpdates() {
    if (!window.__JUCE__) return;
    window.__JUCE__.backend.addEventListener('triggerStateUpdated', () => {
        pullTriggerState();
    });
}

function renderTriggerPanel() {
    const panel = document.getElementById('trigger-panel');
    if (!panel) return;

    // Mirror the technique-bar visibility contract: stay hidden until the
    // user has more than one technique slot, since CC/PC routing is
    // pointless on a single-technique library.
    const expanded = techniqueState.count > 1;
    panel.hidden = !expanded;
    if (!expanded) return;

    // Top-level fields
    const ccToggle = document.getElementById('cc-trigger-enabled');
    const ccNumber = document.getElementById('cc-trigger-number');
    const pcToggle = document.getElementById('pc-trigger-enabled');
    if (ccToggle) ccToggle.checked = triggerState.ccEnabled;
    if (ccNumber) ccNumber.value   = String(triggerState.ccNumber);
    if (pcToggle) pcToggle.checked = triggerState.pcEnabled;

    const techCount = Math.max(1, Math.min(8, techniqueState.count || 1));

    // CC table — 8 rows. Slots beyond techCount are dimmed (kept editable
    // but greyed so the user can still pre-stage values for a future grow).
    const ccTbody = document.querySelector('#cc-trigger-table tbody');
    if (ccTbody) {
        ccTbody.innerHTML = '';
        for (let i = 0; i < 8; ++i) {
            const slot = triggerState.ccMapping[i] || { rangeLow: 0, rangeHigh: 127, tech: 0 };
            const tr = document.createElement('tr');
            tr.dataset.slot = String(i);
            if (i >= techCount) tr.style.opacity = '0.45';

            const idTd = document.createElement('td');
            idTd.textContent = String(i + 1);
            tr.appendChild(idTd);

            const loTd = document.createElement('td');
            const loInput = document.createElement('input');
            loInput.type = 'number';
            loInput.min = '0'; loInput.max = '127'; loInput.step = '1';
            loInput.value = String(slot.rangeLow);
            loInput.dataset.field = 'lo';
            loTd.appendChild(loInput);
            tr.appendChild(loTd);

            const hiTd = document.createElement('td');
            const hiInput = document.createElement('input');
            hiInput.type = 'number';
            hiInput.min = '0'; hiInput.max = '127'; hiInput.step = '1';
            hiInput.value = String(slot.rangeHigh);
            hiInput.dataset.field = 'hi';
            hiTd.appendChild(hiInput);
            tr.appendChild(hiTd);

            const techTd = document.createElement('td');
            const techInput = document.createElement('input');
            techInput.type = 'number';
            techInput.min = '1'; techInput.max = '8'; techInput.step = '1';
            techInput.value = String(slot.tech + 1);   // user-facing 1..8
            techInput.dataset.field = 'tech';
            techTd.appendChild(techInput);
            tr.appendChild(techTd);

            ccTbody.appendChild(tr);
        }
    }

    // PC table
    const pcTbody = document.querySelector('#pc-trigger-table tbody');
    if (pcTbody) {
        pcTbody.innerHTML = '';
        for (let i = 0; i < 8; ++i) {
            const slot = triggerState.pcMapping[i] || { pc: i, tech: i };
            const tr = document.createElement('tr');
            tr.dataset.slot = String(i);
            if (i >= techCount) tr.style.opacity = '0.45';

            const idTd = document.createElement('td');
            idTd.textContent = String(i + 1);
            tr.appendChild(idTd);

            const pcTd = document.createElement('td');
            const pcInput = document.createElement('input');
            pcInput.type = 'number';
            pcInput.min = '0'; pcInput.max = '127'; pcInput.step = '1';
            pcInput.value = String(slot.pc);
            pcInput.dataset.field = 'pc';
            pcTd.appendChild(pcInput);
            tr.appendChild(pcTd);

            const techTd = document.createElement('td');
            const techInput = document.createElement('input');
            techInput.type = 'number';
            techInput.min = '1'; techInput.max = '8'; techInput.step = '1';
            techInput.value = String(slot.tech + 1);
            techInput.dataset.field = 'tech';
            techTd.appendChild(techInput);
            tr.appendChild(techTd);

            pcTbody.appendChild(tr);
        }
    }

    // Sub-panel disabled visuals (mirror the KS-controls disabled pattern)
    const ccSubpanel = document.querySelector('#cc-trigger-table')?.closest('.trigger-subpanel');
    const pcSubpanel = document.querySelector('#pc-trigger-table')?.closest('.trigger-subpanel');
    if (ccSubpanel) ccSubpanel.classList.toggle('disabled', !triggerState.ccEnabled);
    if (pcSubpanel) pcSubpanel.classList.toggle('disabled', !triggerState.pcEnabled);
}

function bindTriggerPanel() {
    const ccToggle = document.getElementById('cc-trigger-enabled');
    if (ccToggle) {
        ccToggle.addEventListener('change', async () => {
            if (!window.__JUCE__) return;
            try {
                const fn = Juce.getNativeFunction('setCcEnabled');
                await fn(!!ccToggle.checked);
            } catch (err) { console.warn('[sampler-app] setCcEnabled failed', err); }
        });
    }

    const ccNumber = document.getElementById('cc-trigger-number');
    if (ccNumber) {
        ccNumber.addEventListener('change', async () => {
            if (!window.__JUCE__) return;
            let v = parseInt(ccNumber.value, 10);
            if (!Number.isFinite(v)) v = 32;
            v = Math.max(0, Math.min(119, v));
            try {
                const fn = Juce.getNativeFunction('setCcNumber');
                await fn(v);
            } catch (err) { console.warn('[sampler-app] setCcNumber failed', err); }
        });
    }

    const pcToggle = document.getElementById('pc-trigger-enabled');
    if (pcToggle) {
        pcToggle.addEventListener('change', async () => {
            if (!window.__JUCE__) return;
            try {
                const fn = Juce.getNativeFunction('setPcEnabled');
                await fn(!!pcToggle.checked);
            } catch (err) { console.warn('[sampler-app] setPcEnabled failed', err); }
        });
    }

    const ccTable = document.getElementById('cc-trigger-table');
    if (ccTable) {
        ccTable.addEventListener('change', async (e) => {
            const input = e.target;
            if (!input || input.tagName !== 'INPUT') return;
            const tr = input.closest('tr');
            if (!tr) return;
            const slot = parseInt(tr.dataset.slot, 10);
            if (!Number.isFinite(slot)) return;

            const loInput   = tr.querySelector('input[data-field="lo"]');
            const hiInput   = tr.querySelector('input[data-field="hi"]');
            const techInput = tr.querySelector('input[data-field="tech"]');
            if (!loInput || !hiInput || !techInput) return;

            let lo   = Math.max(0, Math.min(127, parseInt(loInput.value,   10) || 0));
            let hi   = Math.max(0, Math.min(127, parseInt(hiInput.value,   10) || 0));
            let tech = Math.max(1, Math.min(8,   parseInt(techInput.value, 10) || 1)) - 1;
            if (hi < lo) hi = lo;

            if (!window.__JUCE__) return;
            try {
                const fn = Juce.getNativeFunction('setCcMapping');
                await fn(slot, lo, hi, tech);
            } catch (err) { console.warn('[sampler-app] setCcMapping failed', err); }
        });
    }

    const pcTable = document.getElementById('pc-trigger-table');
    if (pcTable) {
        pcTable.addEventListener('change', async (e) => {
            const input = e.target;
            if (!input || input.tagName !== 'INPUT') return;
            const tr = input.closest('tr');
            if (!tr) return;
            const slot = parseInt(tr.dataset.slot, 10);
            if (!Number.isFinite(slot)) return;

            const pcInput   = tr.querySelector('input[data-field="pc"]');
            const techInput = tr.querySelector('input[data-field="tech"]');
            if (!pcInput || !techInput) return;

            const pc   = Math.max(0, Math.min(127, parseInt(pcInput.value,   10) || 0));
            const tech = Math.max(1, Math.min(8,   parseInt(techInput.value, 10) || 1)) - 1;

            if (!window.__JUCE__) return;
            try {
                const fn = Juce.getNativeFunction('setPcMapping');
                await fn(slot, pc, tech);
            } catch (err) { console.warn('[sampler-app] setPcMapping failed', err); }
        });
    }

    const resetBtn = document.getElementById('trigger-reset');
    if (resetBtn) {
        resetBtn.addEventListener('click', async () => {
            if (!window.__JUCE__) return;
            try {
                const fn = Juce.getNativeFunction('resetTriggerMappings');
                await fn();
            } catch (err) { console.warn('[sampler-app] resetTriggerMappings failed', err); }
        });
    }
}

document.addEventListener('DOMContentLoaded', () => {
    bindTabs();
    bindSliders();
    subscribeSampleMapUpdates();
    subscribeFolderMissingEvent();   // v1.3.0 — register before pull
    subscribeAmbiguousDuplicatesEvent();   // v1.8.0
    subscribeTechniqueStateUpdates();      // v1.14.0
    subscribeTriggerStateUpdates();        // v1.15.0
    bindHostDragEvents();
    bindToastEventListener();
    bindFolderDropZone();
    // v1.13.0 (ARCH-02): document-level drop listener now lives in the
    // shared webview-drop-streaming module. We supply the plugin-specific
    // glue (selectors + UI callbacks + cell midi/vel extraction).
    bindWebViewFileDrop({
        juce: Juce,                              // Juce ES-module namespace, NOT window.__JUCE__
        dropZoneSelector: '#folder-drop-zone',
        cellSelector: '[data-note]',
        setDropZoneHover: setFolderDropZoneHover,
        showFolderLoadOptionsModal,              // (totalBytes) -> {layer,mode,override,embedAudio} | null
        cellMidiVelExtractor: (cellEl) => ({
            midi: parseInt(cellEl.dataset.note,  10),
            vel:  parseInt(cellEl.dataset.layer, 10),
        }),
        showToast,
        showDiagnosticDialog,
        // Fast-path for hosts that DO expose absolute paths (Linux/Win/some
        // WebKit). Forward to the existing handleWebViewFileDrop native fn
        // so cell hit-test + folder-zone hit-test routing run unchanged.
        onPathFastPath: async (paths, x, y) => {
            const fn = Juce.getNativeFunction('handleWebViewFileDrop');
            await fn(paths, x, y);
        },
    });
    bindClearSamplesButton();
    bindPresetButtons();             // v1.3.0
    pullInitialSampleMap();
    pullPendingMissingFolder();      // v1.3.0 — covers boot-time race
    refreshTuningReadout();
    refreshAboutVersion();
    bindResizeObserver();
    bindLoopEditorEvents();
    bindLoopEditorResize();
    bindTechniqueBar();              // v1.14.0
    pullTechniqueState();            // v1.14.0
    bindTriggerPanel();              // v1.15.0
    pullTriggerState();              // v1.15.0
});
