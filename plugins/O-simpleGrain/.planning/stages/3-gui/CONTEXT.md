# Stage 3 (GUI) — CONTEXT

**Plugin:** O-simpleGrain · **Stage:** 3 of 4 (GUI) · **Phase:** discuss · **Date:** 2026-06-25
**Mode:** manual (checkpoint at each phase; full `/clear` handoff at the Stage 3→4 boundary)
**Source:** interactive discuss over ROADMAP.md § Stage 3 Phases (3.1/3.2/3.3) + the
research-locked parameter-spec.md (18 params) + the shipped Stage-2 viz taps & native fns.

> Stage 3 is the **UI mockup + implementation** stage (the mockup was deferred here from
> the pre-Stage-1 gate per the 2026-06-24 decision). The ROADMAP already pins the 3-phase
> breakdown, the control grouping, the four visualizations, and the cross-platform wiring.
> The discuss phase resolved the **four genuinely-open design decisions** the contract left
> to the mockup, listed below.

---

## Decisions made this discuss (the four open design items)

### D1 — Aesthetic: **continue the Ouaricon Audio Naturalist / "Field Guide" theme**
Match the sibling "simple" series (O-simpleFM, O-simpleAdditive): aged-paper background,
Garamond serif, seed-cross-section knobs, green botanical toggles, brown frame. Subtitle
**"Granular Synthesizer · A Field Guide."** Reuse `O-simpleFM/Source/ui/public/css/styles.css`
as the CSS base (palette, knob, toggle, header, preset-bar components are all proven).
- **Rationale:** series consistency, lowest visual risk, large CSS reuse.
- **Granular-specific additions (within the same palette):** the grain-cloud scatter, the
  source-waveform-with-playheads, the window inset, and the scope/spectrum are new canvases
  styled to the sepia/brown palette (sepia dots on aged paper, brown playhead lines).

### D2 — UI production workflow: **adapt the proven layout straight to implementation**
**No separate ui-mockup HTML loop.** Build directly on the O-simpleFM/Additive WebView
structure + CSS and review in the **live plugin build** (`/show-standalone` / DAW). The
layout is proven and the developer signs off on the running plugin, not a throwaway mockup.
- **Rationale:** faster, lower iteration risk; the layout grammar is already validated by
  two shipped siblings.
- **Implication:** Phase 3.1 produces the production `index.html` + `css/` + `js/` directly
  (no `MOCKUP.html` intermediate); first visual review is at the end of 3.1.

### D3 — Layout: **balanced 2×2 visualization grid + side control rail**
The four live visualizations sit in an equal-weight 2×2 grid (cloud / source-waveform /
scope / spectrum), with the window-envelope inset tucked into the cloud or waveform corner,
and all controls in a side rail (Source · Grain · Window · Spray&Scatter · Amp · Output).
- **Rationale:** even teaching emphasis across the four views; clean projector-readable grid.
- **Contrast w/ rejected "grain-cloud hero":** balanced grid keeps scope/spectrum (the
  sync↔async sideband lesson, UI-04) as prominent as the cloud.

### D4 — Preset tour: **ship all 8 concept presets in Phase 3.3**
Single Grain · Pitched Buzz · Fragments · Smooth Cloud · Frozen Pad · Asynchronous Cloud ·
Granular Fire · Rect Click. Each isolates exactly one concept (FUNC-06). Via the Ouaricon
`preset-manager` module / APVTS snapshots, selectable from the header preset bar.
- **Rationale:** the full teaching tour is the point of a pedagogical instrument; deferring
  half would gut the "learn granular in 5 minutes" goal.

---

## Locked contract carried into GUI (from ROADMAP.md + Stage 2 — immutable)

**The 18 APVTS params** (parameter-spec.md, FINAL): `sourceSample` (choice 4),
`grainSize`, `density`, `position`, `scan`, `freeze` (bool), `windowShape` (choice 5),
`pitchSpray`, `positionSpray`, `scatter`, `grainPitch`, `panSpray`, `velToDensity`,
`ampAttack`/`Decay`/`Sustain`/`Release`, `outputLevel`. Plus the **non-APVTS "Load…"**
action (drag-drop + picker) backed by custom state.

**Stage-2 viz taps to consume (already exposed on the processor):**
- `getVizRing()` → copy samples to a local block, run FFT on the **message thread** (via
  `VizAnalyzer`, 4096/Blackman-Harris) → output **scope** (UI-04) + **spectrum** (UI-04).
- `getGrainCloudBuffer()` → `TripleBuffer<GrainCloudFrame>::read()` grain events →
  **grain-cloud scatter** (UI-01) + **source-waveform live playheads / freeze pin / shaded
  position-spray range** (UI-02).
- `getActiveGrainCount()` → **grain-count / overlap / CPU readout** (UI-05):
  `Grains: N/192`, `Overlap: (grainSizeSec×density)×`, coarse CPU bar.
- **Window inset (UI-03):** draw the selected window LUT for one grain; redraw on
  `windowShape` change.

**Drag-drop / picker native fns to wire (already implemented on the processor — names FIXED
by the shared module, do NOT rename):** `dropSessionStart`, `dropSessionAddFile`,
`dropSessionCommitFile`, `dropSessionCommitFolder`, `loadSourceFromFileChooser`. Returns
bool "ok"; JS awaits + toasts on false. Decode is `juce::Base64::convertFromBase64`.

**Cross-platform WebView wiring (must hold — see project memory + critical-patterns):**
- PluginEditor member order: **relays → WebView → attachments**.
- `WebSliderRelay` (all float knobs), `WebComboBoxRelay` (`sourceSample`, `windowShape`),
  `WebToggleButtonRelay` (`freeze`); 3-arg attach + `nullptr` undoManager (#11/#12).
- Resource provider returns **bare paths** (`url == "/" || url == "/index.html"`), NOT
  stripped URLs. `type="module"` scripts; `import * as Juce`; pass the **`Juce` namespace**
  (not `window.__JUCE__`) to any shared panel; `getSliderState`/`getComboBoxState`/
  `getToggleState` per type (#19); relative-drag knobs (#16).
- CMake: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`,
  `NEEDS_WEBVIEW2 TRUE`; defs `JUCE_WEB_BROWSER=1`,
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`; Windows
  `withUserDataFolder(tempDir)`.
- Canvas DPR-aware backing store: `width: calc(100% - Npx)` (NOT `right/bottom`),
  `canvas.width = clientWidth·dpr`, `ctx.setTransform(dpr,…)` (project memory: replaced-element).
- JS viz subscriptions via `window.__JUCE__.backend.addEventListener` for
  `grainCloudUpdate` / `scopeUpdate` / `spectrumUpdate` / `windowInsetUpdate` / `grainMeterUpdate`.

---

## Phase plan for Stage 3 (from ROADMAP, confirmed)

- **3.1 — Layout + controls + cross-platform wiring + load-your-own UI:** production
  `index.html` + `css/` (Naturalist base) + `js/app.js` + `js/juce/…`; all 18 params
  two-way bound; drag-drop + picker wired; CMake WebView flags; renders macOS VST3+AU +
  Windows VST3. **Review in live build.**
- **3.2 — The four live visualizations + overlap/CPU readout:** 30 Hz editor `Timer`,
  consume the three taps; 2×2 grid + window inset + grain/overlap/CPU readout. No
  audio-thread FFT/alloc; scope copied before FFT.
- **3.3 — Pedagogical layer:** on-hover plain-language tooltips on **every** control;
  the 8 concept presets via `preset-manager`; optional cloud/waveform annotations.

---

## Out of scope for Stage 3 (→ Stage 4)
pluginval (VST3+AU) sweep, preset audit, artifact/aliasing/freeze listen audit, drag-drop
smoke test on Windows, changelog. (Stage 3 verify only checks the GUI goal + a clean build +
auval; the full validation battery is Stage 4.)

---

## Open risks / watch-items
- **Windows WebView2 blank-UI class of bugs** — the memory's full checklist must hold
  (`NEEDS_WEBVIEW2` + static-linking def + `withUserDataFolder`); silent IE fallback gives
  no error. macOS is the primary dev target; Windows verified by config-parity with the
  proven siblings (no Windows build run locally this stage).
- **`Juce` namespace vs `window.__JUCE__`** — any shared panel (preset-manager UI) must
  receive `Juce`, not `window.__JUCE__` (RECURRING regression — see memory).
- **parameter-spec reconciliation** — the spec was frozen pre-mockup; if the live layout
  surfaces a naming/range delta, reconcile APVTS/state here (accepted minor-rework risk).
