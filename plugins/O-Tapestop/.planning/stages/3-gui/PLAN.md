# Stage 3: GUI — Plan

**Created:** 2026-08-15
**Inputs:** stages/3-gui/CONTEXT.md, stages/3-gui/RESEARCH.md, ROADMAP.md Phases 3.1–3.3
**Design path:** direct authoring (no mockup phase) — iterate in Standalone via `/show-standalone`

## Goal

Ship the O-Tapestop WebView UI: 860×580 ouaricon-naturalist-001 frame with a mode-switched
center panel (Stop controls ↔ drawable Scratch envelope canvas), all 14 params two-way bound,
ENGAGE as a prominent performance control, live playback-ratio indicator, and envelope
commit/readback bridge — with the Stage-2 DSP verified frozen (harness re-run after any
processor edit).

## Locked Decisions (do not relitigate)

- Frame: `setSize(860, 580)` == `.frame { width: 860px; height: 580px; }`. **Never resize after Phase 3.1.**
- Row layout: TRIGGER 180 / mode-switched CENTER 430 / OUTPUT 180, 10 px gaps; header ~73 + preset band 44 (disabled markup now, wired Stage 4) + footer.
- Ratio indicator: DOM horizontal bipolar bar (−2×…+2×, zero mark at 40 %, distinct reverse fill) + 2-decimal ×-readout, bottom of TRIGGER panel.
- One editor `juce::Timer` at 30 Hz → single `transportFrame` event `{state, ratio, phase}` via `emitEventIfBrowserIsVisible`; same timer polls `uiEnvGeneration` and emits `envelopeState` on change.
- Mode switch: instant `.hidden` toggle of two absolutely-positioned panes in a fixed center box.
- ENGAGE ships **latching** (toggle attachment); momentary-hold is optional later polish.
- JS envelope module named `envelope_editor.js` (underscores — binary-data hyphen strip).

---

## Tasks

### Phase 3.1 — Layout, scaffolding, WebView boot

1. [x] **Copy ORD UI scaffolding + author index.html/styles.css**
   - Files: `Source/ui/public/index.html`, `Source/ui/public/css/styles.css`,
     `Source/ui/public/js/juce/index.js`, `Source/ui/public/js/juce/check_native_interop.js` (copied from O-ReverseDelay), `Source/ui/public/img/<specimen>.png`
   - 860×580 fixed-px frame, no viewport units; header, disabled preset band
     (IDs `preset-prev/next/name/save/load/delete` exactly as preset-manager.js binds them, controls `disabled`), 3-column panel row, footer
   - CENTER box fixed-dimension with `.mode-stop` / `.mode-scratch` absolutely-positioned panes;
     three fixed time-slots (stop/start/env) each holding select-wrap + knob-wrap for the SYNC_MODE swap
   - All segment/button copy HTML-authored (`data-label`); readout elements born with `—`
   - Canvas element in Scratch pane with explicit width/height attributes (~400×240 CSS px)
   - Depends on: none

2. [x] **CMake binary-data target**
   - Files: `plugins/O-Tapestop/CMakeLists.txt` (replace the Stage-3 placeholder comment)
   - `juce_add_binary_data(OuariconTapestop_UIResources NAMESPACE UIBinaryData HEADER_NAME UIBinaryData.h SOURCES …)`; link PRIVATE to plugin target only (harness untouched)
   - Depends on: Task 1 (file list)

3. [x] **PluginEditor skeleton: WebView boot + resource provider**
   - Files: `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
   - `setSize(860, 580)`; explicit bare-path resource map with `charset=utf-8` on text MIME;
     `goToURL(getResourceProviderRoot())`; `Options::WinWebView2{}.withUserDataFolder` block copied from ORD
   - Member order: relays → webView → attachments (declared now, relays filled in 3.2)
   - Depends on: Task 2
   - Gate: build + `/show-standalone` — WebView opens at 860×580, layout correct, console clean (ES-module TDZ smoke)

### Phase 3.2 — Parameter binding + engage

4. [x] **Relays + attachments for all 14 params**
   - Files: `Source/PluginEditor.h/.cpp`
   - 8 WebSliderRelay (3-arg attachments), 5 WebComboBoxRelay, 1 WebToggleButtonRelay (ENGAGE);
     `Options{}.withOptionsFrom(*relay)` per relay before webView construction
   - `getParameterDefaults` native fn (engineering units via `convertFrom0to1`)
   - Depends on: Task 3

5. [x] **app.js control bindings**
   - Files: `Source/ui/public/js/app.js`
   - `import * as Juce from "./juce/index.js"`; all init inside `init()` (no top-level state touching later let/const)
   - Knobs (house 56×56), division selects built at runtime from `properties.choices`;
     readouts exclusively via `SliderState.getScaledValue()`; double-click reset via defaults fn
   - ENGAGE large latching toggle bound to `getToggleState("ENGAGE")`
   - MODE segments → center-pane `.hidden` toggle; SYNC_MODE segments → triple time-slot swap
     (env slot visibility = two independent nested class toggles, never one computed flag)
   - Depends on: Task 4
   - Gate: two-way binding sweep in Standalone — UI→param and automation→UI for all 14; skew-correct readouts on the three FREE_MS knobs

### Phase 3.3 — Envelope editor + live readback

6. [x] **Processor readback surface (DSP-frozen-safe additions only)**
   - Files: `Source/PluginProcessor.h/.cpp` (+ tiny getters in `Source/dsp/TapestopTransport.h` if needed)
   - `std::atomic<float> uiRatio, uiScratchPhase; std::atomic<int> uiState;` — relaxed stores once per block at end of processBlock (audio thread publishes only)
   - `std::atomic<uint32_t> uiEnvGeneration` — incremented in `commitScratchEnvelopeJson` and after envelope restore in `setStateInformation`
   - `juce::String getScratchEnvelopeJson() const` public accessor
   - Depends on: none (parallel-safe with 3.1/3.2)
   - Gate: **re-run the 47-probe render harness — mandatory** (regression guard on frozen DSP)

7. [x] **`envelope_editor.js` canvas module**
   - Files: `Source/ui/public/js/envelope_editor.js` (+ CMake SOURCES entry)
   - Path C §2.2 adapted: point model `{x, y, curve}`, drag / dblclick-add / rightclick+alt-click delete, endpoint pinning, 0.01 min x-separation, 2–64 points
   - DPR backing store via ORD `envResize()` pattern (resize only when changed, `setTransform(dpr…)`, draw in CSS px, mouse via `getBoundingClientRect()`)
   - Bipolar axis: `canvasY = (1 − (y+1)/2)·h`; labelled 1× line at y=+0.5, 0-line, tinted reverse zone, gridline labels +2×/1×/0/−1×/−2×; palette from CSS custom properties
   - Curve preview copied character-for-character from ScratchEnvelope.h:242-247 with a binding comment
   - Midpoint diamond curve handles: vertical drag adjusts curve ∈ [−1,+1], dblclick resets to 0
   - Pointer events + `setPointerCapture`; exports a class/factory only (no top-level init)
   - Depends on: Task 1 (canvas box exists); testable standalone against a Juce-bridge stub
   - Gate: TDZ smoke — module loads against bridge stub, console clean

8. [x] **Envelope bridge: commit / request / echo**
   - Files: `Source/PluginEditor.cpp`, `Source/ui/public/js/app.js`, `envelope_editor.js`
   - `withNativeFunction("commitEnvelope")` → `commitScratchEnvelopeJson(json)`, completes synchronously with sanitized `toJson()`; JS redraws from the echoed truth
   - `withNativeFunction("requestEnvelope")` → `getScratchEnvelopeJson()` at page init
   - Commit cadence: local draw during drag; commit on mouse-up + 50 ms debounce
   - Synchronous lambdas capturing `this` are safe here (no deferred completions); note Stage-4 SafePointer requirement in handoff
   - Depends on: Tasks 6, 7

9. [x] **30 Hz editor timer: transportFrame + envelopeState + ratio bar**
   - Files: `Source/PluginEditor.h/.cpp`, `Source/ui/public/js/app.js`
   - `startTimerHz(30)`; read atomics relaxed; `emitEventIfBrowserIsVisible("transportFrame", {state, ratio, phase})`; compare `uiEnvGeneration`, emit `envelopeState` with sanitized JSON on change
   - JS single listener via `window.__JUCE__.backend.addEventListener`: ratio bar always (change-gated DOM writes), canvas playhead only when MODE=Scratch && state==ScratchPass && phase changed
   - Ratio bar: zero-anchored fill, reverse fill styled distinctly, numeric readout `%.2f×`
   - Depends on: Tasks 6, 8

10. [x] **Verification pass + install**
    - Grep-diff bridge check: every JS `getNativeFunction("X")` ↔ C++ `withNativeFunction("X")`, both directions
    - Re-run render harness (if any processor file touched since last run); confirm 47/47
    - `./scripts/build-and-install.sh O-Tapestop` (cache clear + dual-variant sweep); `auval -a | grep -i tapestop`
    - Standalone + DAW smoke: engage gesture, mode switch, scratch draw + playhead, host-automation echo, editor close/reopen with envelope intact (session restore via `envelopeState`)
    - Depends on: Tasks 1–9

---

## Success Criteria

- [ ] WebView opens at 860×580; layout matches the locked column plan; zero console errors (Phase 3.1 gate)
- [ ] All 14 params two-way bind: UI edits reach the host, host automation/preset loads update every control (Phase 3.2 gate)
- [ ] ENGAGE from UI is indistinguishable from host automation (FUNC-01 acceptance path, UI-02)
- [ ] FREE_MS readouts are skew-correct (getScaledValue; spot-check 3 positions against the generic editor)
- [ ] SYNC_MODE and MODE swaps produce zero layout shift (instant `.hidden` toggles)
- [ ] Envelope editor: add/drag/delete points, curve handles, endpoint pinning, bipolar axis with labelled 1× line and reverse zone; redraw always converges to the C++-sanitized echo (UI-01)
- [ ] Envelope survives editor close/reopen and host session reload while editor open (generation-counter event)
- [ ] Pass playhead tracks the scratch pass at 30 Hz; ratio bar shows direction + magnitude incl. reverse
- [ ] Native-fn grep-diff clean both directions
- [ ] Render harness 47/47 after processor readback additions (DSP still frozen)
- [ ] Build + install via build-and-install.sh; auval PASS; Standalone and one DAW host verified

## Out of Scope (Stage 4)

Preset-manager wiring (band ships disabled), factory presets, FileChooser dialogs (SafePointer
pattern applies there), pluginval strictness-10 gate, changelog.
