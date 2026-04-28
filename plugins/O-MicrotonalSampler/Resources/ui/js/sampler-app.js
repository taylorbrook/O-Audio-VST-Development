/* ==============================================================================
 * sampler-app.js
 * O-MicrotonalSampler — Phase 3.1 entry point.
 *
 * Wires:
 *   - 7 APVTS sliders ↔ DOM range inputs via Juce.getSliderState() (relay
 *     identifiers must match the C++ WebSliderRelay names exactly).
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

// ============================================================================
// Slider relay binding
// ============================================================================
//
// Each entry maps the DOM element id to its WebSliderRelay/APVTS parameter
// id. The relay handle (Juce.getSliderState) is bidirectional: setting
// .setNormalisedValue() pushes to the C++ APVTS; valueChangedEvent fires
// when automation / preset / DAW changes the parameter so we update the
// DOM control to match.
const SLIDER_BINDINGS = [
    { domId: 'ctrl-attack',              relayId: 'attack' },
    { domId: 'ctrl-decay',               relayId: 'decay' },
    { domId: 'ctrl-sustain',             relayId: 'sustain' },
    { domId: 'ctrl-release',             relayId: 'release' },
    { domId: 'ctrl-polyphony',           relayId: 'polyphony' },
    { domId: 'ctrl-velocity-crossfade',  relayId: 'velocity_crossfade' },
    { domId: 'ctrl-output-gain',         relayId: 'output_gain' }
];

function bindSliders() {
    if (!window.__JUCE__) {
        console.warn('[sampler-app] __JUCE__ not available — running outside plugin host');
        return;
    }

    SLIDER_BINDINGS.forEach(({ domId, relayId }) => {
        const el = document.getElementById(domId);
        if (!el) {
            console.warn(`[sampler-app] DOM element #${domId} not found`);
            return;
        }

        try {
            const state = Juce.getSliderState(relayId);

            // Initial pull
            const init = state.getNormalisedValue();
            if (typeof init === 'number') el.value = init;

            // DOM → C++
            el.addEventListener('input', () => {
                const v = parseFloat(el.value);
                if (!Number.isNaN(v)) {
                    state.setNormalisedValue(v);
                    state.sliderDragStarted();
                    state.sliderDragEnded();
                }
            });

            // C++ → DOM (automation, preset load, DAW change)
            state.valueChangedEvent.addListener(() => {
                const v = state.getNormalisedValue();
                if (typeof v === 'number') el.value = v;
            });
        } catch (e) {
            console.error(`[sampler-app] Failed to bind slider ${relayId}:`, e);
        }
    });
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

    if (tabName === 'tuning') {
        ensureTuningPanelMounted();
        refreshTuningReadout();
    }
}

function bindTabs() {
    tabButtons().forEach(btn => {
        btn.addEventListener('click', () => activateTab(btn.dataset.tab));
    });
}

// ============================================================================
// TuningPanel — lazy mount + readonly span swap (RESEARCH §RQ3-1)
// ============================================================================
let tuningPanelMounted = false;
let tuningPanelInstance = null;

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

        tuningPanelInstance = new TuningPanel(container, window.__JUCE__);
        await tuningPanelInstance.init();

        // Readonly span-swap shim: walk every .interval-input and replace
        // with a <span class="interval-display"> showing the cents value.
        // The CSS overlay (tuning-panel-readonly.css) hides the inputs as
        // a belt-and-suspenders measure but the shim ensures the value is
        // still visible to the user as a static label.
        applyIntervalReadonlyShim(container);

        // Re-apply the shim if the panel re-renders its interval list.
        // Most TuningPanel implementations re-render on intervals change;
        // observe the DOM as a defensive net.
        const obs = new MutationObserver(() => applyIntervalReadonlyShim(container));
        obs.observe(container, { childList: true, subtree: true });
    } catch (e) {
        console.error('[sampler-app] TuningPanel mount failed:', e);
        container.innerHTML = '<div style="color:var(--text-muted); padding:16px; font-style:italic;">Tuning panel unavailable.</div>';
    }
}

function applyIntervalReadonlyShim(container) {
    container.querySelectorAll('.tuning-panel .interval-input').forEach(input => {
        // Skip if already swapped (shim re-runs on mutations).
        if (input.dataset.swapped === '1') return;
        input.dataset.swapped = '1';

        const cents = input.value || input.placeholder || '';
        const span = document.createElement('span');
        span.className = 'interval-display';
        span.textContent = cents;
        // Keep the input in DOM (CSS hides it) so the panel's logic that
        // reads .value still works for any internal book-keeping. We
        // simply place a visible read-only label next to it.
        if (input.parentNode) {
            input.parentNode.insertBefore(span, input);
        }
    });
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

function renderGrid(snap) {
    const container = document.getElementById('sample-map-grid');
    if (!container) return;

    // Build fast — clear via innerHTML then assemble in a fragment.
    container.innerHTML = '';
    const inner = document.createElement('div');
    inner.id = 'sample-grid-inner';

    // Slot lookup map: key = `${midi}_${layer}` → slot.
    const slotMap = new Map();
    if (Array.isArray(snap?.slots)) {
        for (const s of snap.slots) {
            slotMap.set(`${s.midiNote}_${s.velocityLayer}`, s);
        }
    }

    const frag = document.createDocumentFragment();

    // Rows are layer 3 (top) → layer 0 (bottom) so loudest-on-top.
    for (let row = 0; row < NUM_LAYERS; ++row) {
        const layer = (NUM_LAYERS - 1) - row;
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

            const slot = slotMap.get(`${midi}_${layer}`);
            if (slot) {
                cell.classList.add('cell-loaded');
                cell.title = slot.filename
                    ? `${slot.filename} — MIDI ${midi}, layer ${layer}`
                    : `MIDI ${midi}, layer ${layer}`;
            } else {
                cell.classList.add('cell-empty');
                cell.title = `Empty — MIDI ${midi}, layer ${layer}`;
            }

            frag.appendChild(cell);
        }
    }

    inner.appendChild(frag);
    container.appendChild(inner);

    // Wire interactions on the inner container (single delegated listener).
    bindGridInteractions(inner);
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

    cell.classList.add('cell-loading');
    const t0 = performance.now();
    lastReplaceTimestamp = t0;

    try {
        const fn = Juce.getNativeFunction('loadSingleSampleDialog');
        const ok = await fn(midi, layer);
        if (!ok) {
            // User cancelled or selection invalid — drop the loading shimmer.
            cell.classList.remove('cell-loading');
            return;
        }
        // The sampleMapUpdated push event will trigger renderGrid which
        // rebuilds the cell. Log timing for the gate metric.
        const t1 = performance.now();
        console.log(`[sampler-app] FileChooser close → load dispatch: ${(t1 - t0).toFixed(1)} ms`);
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

function bindResizeObserver() {
    if (typeof ResizeObserver !== 'function') {
        // Fallback to window resize listener.
        window.addEventListener('resize', () => publishCellLayout());
        return;
    }
    const obs = new ResizeObserver(() => publishCellLayout());
    obs.observe(document.body);
}

// ============================================================================
// Folder drop-zone — button fallback + drag-hover visuals (Phase 3.3 Task 20)
// ============================================================================
//
// File-system drop is consumed by the host editor (juce::FileDragAndDropTarget
// on OMicrotonalSamplerAudioProcessorEditor). The host emits hostFileDragMove
// {x,y} and hostFileDragExit events as the cursor moves — we toggle the
// .drag-over class on #folder-drop-zone based on whether (x,y) is inside the
// zone's client-rect.
//
// The HTML5 dragover handler on the zone calls preventDefault() purely as a
// belt-and-suspenders measure — without it, some browsers show a "no drop"
// cursor even though the host editor will receive the actual drop. macOS
// AU/VST3 hosts always route through juce::FileDragAndDropTarget; the JS
// dragover never sees real file paths (sandboxing).

function bindFolderDropZone() {
    const zone = document.getElementById('folder-drop-zone');
    const button = document.getElementById('load-folder-btn');

    if (button) {
        button.addEventListener('click', async () => {
            if (!window.__JUCE__) return;
            try {
                const fn = Juce.getNativeFunction('loadSampleFolderDialog');
                await fn();
                // sampleMapUpdated push event drives the rest. Cancel resolves
                // false — silent.
            } catch (e) {
                console.error('[sampler-app] loadSampleFolderDialog failed:', e);
            }
        });
    }

    if (zone) {
        // Prevent the "no drop" cursor without consuming the drop (host
        // editor still receives it). Only effective inside the WebView; the
        // real drop handling is in C++.
        zone.addEventListener('dragover', (e) => e.preventDefault());
    }
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
};

function isOneShot(snap) {
    if (!snap || typeof snap.loopMode !== 'string') return false;
    const m = snap.loopMode.toLowerCase();
    return m === 'one-shot' || m === 'oneshot';
}

async function openLoopEditor(midi, vel) {
    if (!window.__JUCE__) return;

    try {
        const fn = Juce.getNativeFunction('getWaveformPeaks');
        const json = await fn(midi, vel, LOOP_EDITOR_BINS);
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

        populateLoopEditorHeader(snap);
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

function showLoopEditorPanel() {
    const panel = document.getElementById('loop-editor-panel');
    if (!panel) return;
    panel.hidden = false;
    document.body.classList.add('le-open');
    // Republish layout shadow — grid width changed.
    requestAnimationFrame(() => publishCellLayout());
}

function closeLoopEditor() {
    const panel = document.getElementById('loop-editor-panel');
    if (panel) panel.hidden = true;
    document.body.classList.remove('le-open');
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
                await fn(editorState.midi, editorState.vel);
                // Re-fetch peaks to get fresh marker positions + mode.
                const get = Juce.getNativeFunction('getWaveformPeaks');
                const json = await get(editorState.midi, editorState.vel, LOOP_EDITOR_BINS);
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
                         editorState.loopStart, editorState.loopEnd, 8);
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
// Boot
// ============================================================================
document.addEventListener('DOMContentLoaded', () => {
    bindTabs();
    bindSliders();
    subscribeSampleMapUpdates();
    bindHostDragEvents();
    bindToastEventListener();
    bindFolderDropZone();
    pullInitialSampleMap();
    refreshTuningReadout();
    bindResizeObserver();
    bindLoopEditorEvents();
    bindLoopEditorResize();
});
