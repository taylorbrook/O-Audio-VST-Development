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

    // ---- Issues disclosure ----
    const issues = document.getElementById('issues-disclosure');
    const issuesList = document.getElementById('issues-list');
    if (issues && issuesList) {
        const skipped = Array.isArray(snap.skippedFiles) ? snap.skippedFiles : [];
        if (skipped.length > 0) {
            issuesList.innerHTML = '';
            skipped.forEach(s => {
                const li = document.createElement('li');
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

    // ---- Grid render (Phase 3.2) ----
    renderGrid(snap);

    // Re-publish the cell-layout shadow now that DOM has settled. Defer one
    // frame so the browser has computed final geometry.
    requestAnimationFrame(() => publishCellLayout());
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
        // 3.2 placeholder — full loop editor lands in 3.4.
        console.log(`[sampler-app] openLoopEditor placeholder: midi=${midi} vel=${layer}`);
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
                    console.log(`[sampler-app] context: openLoopEditor midi=${midi} vel=${layer} (Phase 3.4)`);
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
// Boot
// ============================================================================
document.addEventListener('DOMContentLoaded', () => {
    bindTabs();
    bindSliders();
    subscribeSampleMapUpdates();
    pullInitialSampleMap();
    refreshTuningReadout();
    bindResizeObserver();
});
