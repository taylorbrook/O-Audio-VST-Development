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

    // Phase 3.1: minimal handler. Update the issues disclosure if there are
    // skipped files; the grid is a placeholder until 3.2.
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

    // Stub grid update — confirms the data flow is alive.
    const grid = document.getElementById('sample-map-grid');
    if (grid) {
        const slotCount = Array.isArray(snap.slots) ? snap.slots.length : 0;
        const txt = grid.querySelector('.grid-placeholder-text');
        if (txt) {
            txt.textContent = (slotCount > 0)
                ? `${slotCount} slot${slotCount === 1 ? '' : 's'} loaded · grid renders in Phase 3.2`
                : `Sample-mapping grid arrives in Phase 3.2 (snapshot v${snap.version || 0})`;
        }
    }
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
});
