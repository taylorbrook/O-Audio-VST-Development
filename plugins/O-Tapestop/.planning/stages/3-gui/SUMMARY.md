# Stage 3: GUI — Execution Summary

**Executed:** 2026-08-15
**Result:** All 10 PLAN tasks complete. Clean build (VST3 + AU + Standalone), render
harness **47/47 after processor edits**, native-fn grep-diff clean both directions,
`build-and-install.sh` complete, **auval PASS**, Standalone boots crash-free.

## What was built, per task

### Phase 3.1 — Layout, scaffolding, WebView boot

1. **UI scaffolding + index.html/styles.css** — 860×580 fixed-px Naturalist frame
   (ouaricon-naturalist-001), header, 44 px preset band (IDs
   `preset-prev/next/name/save/load/delete` exactly as preset-manager.js binds them,
   all controls `disabled` — Stage 4 wires them without a frame resize), one panel
   row TRIGGER 180 / CENTER 430 / OUTPUT 180 with 10 px gaps, footer. Copied
   `js/juce/index.js` + `js/juce/check_native_interop.js` from O-ReverseDelay.
   Specimen: `img/shell.png` (spiral shell — the reel), pinned px height.
   All segment/button copy HTML-authored (`data-label`); readouts born with `—`.
   Canvas has explicit `width="380" height="224"` attributes + CSS px dimensions.
2. **CMake binary-data target** — `OuariconTapestop_UIResources` with
   `NAMESPACE UIBinaryData`, `HEADER_NAME UIBinaryData.h`, linked PRIVATE to the
   plugin target only (harness untouched). JS module named `envelope_editor.js`
   (underscores — hyphen-strip trap dodged).
3. **PluginEditor skeleton** — WebView boot with explicit bare-path resource map
   (`charset=utf-8` on all text MIME), `goToURL(getResourceProviderRoot())`,
   `withKeepPageLoadedWhenBrowserIsHidden()`, `#if JUCE_WINDOWS` WinWebView2 block
   (`withUserDataFolder` temp dir + status bar/error page disabled — ORD verbatim).
   `setSize(860, 580)` mirrors `.frame`.

### Phase 3.2 — Parameter binding + engage

4. **Relays + attachments, all 14 params** — 8 WebSliderRelay (3-arg attachments,
   nullptr undoManager), 5 WebComboBoxRelay (MODE, SYNC_MODE, 3 divisions),
   1 WebToggleButtonRelay (ENGAGE). Member order relays → webView → attachments;
   every relay registered via `withOptionsFrom(*relay)` before webView construction.
   `getParameterDefaults` native fn returns engineering-unit defaults via
   `convertFrom0to1` (skew-exact dblclick reset for the three 0.35-skew FREE_MS).
5. **app.js control bindings** — `import * as Juce from "./juce/index.js"`; all init
   inside `init()` at file bottom (TDZ-safe); house 56 px knobs with pointer-capture
   drag (ORD WR-05 four-path termination), wheel/arrow nudge, dblclick reset;
   division selects built at runtime from `properties.choices`; readouts exclusively
   `SliderState.getScaledValue()`; ENGAGE large latching toggle via
   `getToggleState("ENGAGE")`; MODE → instant `.hidden` pane toggle; SYNC_MODE →
   triple time-slot swap (env slot visibility = two independent nested toggles).

### Phase 3.3 — Envelope editor + live readback

6. **Processor readback surface** (only processor edits this stage) —
   `std::atomic<float> uiRatio, uiScratchPhase; std::atomic<int> uiState;` relaxed
   stores once at the END of processBlock (audio thread publishes only; Bypassed
   reads 1.0×); `std::atomic<uint32_t> uiEnvGeneration` bumped in
   `commitScratchEnvelopeJson` and after envelope restore in `setStateInformation`;
   `getScratchEnvelopeJson()` public accessor; tiny observer getter
   `TapestopTransport::getScratchPhase()` (pure read, ScratchPass only).
   **Gate: harness re-run → 47/47, exit 0** (run immediately after these edits and
   again at the end).
7. **envelope_editor.js** — class-only ES module (no top-level side effects, no Juce
   dependency — stub-renderable). Point model `{x, y, curve}`; drag / dblclick-add /
   right-click + alt-click delete; endpoints pinned; 0.01 min separation; 2–64
   points. DPR backing store via the ORD `envResize()` pattern (write width/height
   only on change, `setTransform(dpr…)`, draw in CSS px, mouse via
   `getBoundingClientRect()`). Bipolar axis `canvasY = (1 − (y+1)/2)·h`; prominent
   labelled 1× line at y=+0.5, 0-line, tinted red-brown reverse zone, labels
   +2×/1×/0/−1×/−2×; palette from CSS custom properties. Curve law copied
   character-for-character from ScratchEnvelope.h:242-247 with a binding comment.
   Midpoint diamond curve handles (vertical drag ∈ [−1,+1], direction-aware sign,
   dblclick resets to 0). Pointer events + `setPointerCapture`. Default model
   mirrors `defaultPoints()` (gentle wobble).
8. **Envelope bridge** — `commitEnvelope` (arg JSON → `commitScratchEnvelopeJson`,
   completes SYNCHRONOUSLY with the sanitized `toJson()`; JS redraws from the echo;
   the lambda also re-seeds `lastEnvGeneration` so the timer doesn't push a
   redundant event) and `requestEnvelope` (page init). Commit cadence: local draw
   during drag, commit on pointer-up + 50 ms debounce. Synchronous `this`-capturing
   lambdas — safe here; **Stage-4 handoff note: SafePointer is mandatory the moment
   any completion defers (preset FileChoosers).**
9. **30 Hz timer + ratio bar** — one `juce::Timer` at `startTimerHz(30)`; relaxed
   atomic reads; single `transportFrame` event `{state, ratio, phase}` via
   `emitEventIfBrowserIsVisible`; same tick compares `uiEnvGeneration` and emits
   `envelopeState` with sanitized JSON on change. JS single listener via
   `window.__JUCE__.backend.addEventListener` (the sanctioned `__JUCE__` use):
   ratio bar always (change-gated DOM writes), canvas playhead only when
   MODE=Scratch && state==ScratchPass(6) (setPlayhead skips unchanged phases).
   Ratio bar: DOM bipolar bar, zero mark at 40 % (−2..0 into 40 %, 0..+2 over
   60 %, mirrored in `ratioToPct()`), labelled 1× mark at 70 %, distinct red-brown
   reverse fill, 2-decimal ×-readout.
10. **Verification pass + install** — results below.

## Files created

- `Source/ui/public/index.html`
- `Source/ui/public/css/styles.css`
- `Source/ui/public/js/app.js`
- `Source/ui/public/js/envelope_editor.js`
- `Source/ui/public/js/juce/index.js` (copied from O-ReverseDelay)
- `Source/ui/public/js/juce/check_native_interop.js` (copied from O-ReverseDelay)
- `Source/ui/public/img/shell.png` (copied from O-IntonationPad)

## Files modified

- `Source/PluginEditor.h` / `Source/PluginEditor.cpp` — GenericAudioProcessorEditor
  placeholder replaced with the WebView editor (relays → webView → attachments,
  resource provider, 3 native fns, 30 Hz timer)
- `Source/PluginProcessor.h` — UI readback atomics, uiEnvGeneration,
  `getScratchEnvelopeJson()`; generation bump inside `commitScratchEnvelopeJson`
- `Source/PluginProcessor.cpp` — `uiLastR` bookkeeping + end-of-block relaxed
  publishes; generation bump after envelope restore in `setStateInformation`
- `Source/dsp/TapestopTransport.h` — `getScratchPhase()` observer getter
- `CMakeLists.txt` — `OuariconTapestop_UIResources` binary-data target + link

## Gate results

| Gate | Result |
|---|---|
| Render harness after Task-6 processor edits | **47/47, exit 0** |
| Render harness final re-run | **47/47, exit 0** |
| Clean build `OuariconTapestop_VST3` + `_AU` + `_Standalone` | exit 0, no warnings |
| Native-fn grep-diff (JS `getNativeFunction` ↔ C++ `withNativeFunction`, both directions) | 3 ↔ 3, exact match (`getParameterDefaults`, `commitEnvelope`, `requestEnvelope`) |
| Event-name parity (`transportFrame`, `envelopeState`) | emit ↔ listen, both present |
| Relay coverage vs APVTS | 14/14 exact set match (diff clean) |
| JS-referenced element IDs present in HTML | 29/29, none missing |
| `node --check` on app.js + envelope_editor.js | pass |
| `./scripts/build-and-install.sh O-Tapestop` | exit 0 (cache clear + dual-variant sweep, no alternate-variant warning) |
| `auval -a \| grep -i tapestop` | `aufx OTsp OuDv — O-Tapestop-dev` |
| `auval -v aufx OTsp OuDv` | **AU VALIDATION SUCCEEDED** |
| Standalone smoke (launched 6 s) | alive, no crash on WebView boot |

## Deviations from PLAN.md

- **Canvas 380×224 CSS px** (plan said "~400×240"): the 430 px panel's content box
  is 402 wide / 348 tall; 380×224 + plate chrome + the env row lands at 346 of 348
  with 2 px spare. Within the plan's "~" tolerance.
- **Redundant-event suppression added to commitEnvelope** (not in plan text): the
  commit bumps `uiEnvGeneration`, so the lambda re-seeds `lastEnvGeneration` to
  keep the timer from pushing a duplicate `envelopeState` ~33 ms after the
  synchronous echo. Message-thread-only state; no race.
- **TDZ smoke against a Juce-bridge stub** (Task 7 gate) executed as static
  checks instead (class-only module, `node --check`, element-ID cross-check,
  no top-level init) — no browser stub harness exists in this repo for this
  plugin; the interactive Standalone console check is deferred below.

## Manual checks deferred to /plugin-verify

1. **Phase 3.1 visual gate:** open Standalone (`/show-standalone`) — layout matches
   the locked column plan at 860×580, zero console errors in the WebView inspector.
2. **Two-way binding sweep (Phase 3.2 gate):** all 14 controls UI→param and host
   automation/generic-editor→UI; spot-check the three FREE_MS readouts at 3
   positions against the generic editor (skew correctness).
3. **ENGAGE gesture:** UI click vs host automation indistinguishable (FUNC-01 /
   UI-02); rapid engage/release mid-ramp.
4. **Mode/timing swaps:** MODE Stop↔Scratch and SYNC_MODE Sync↔Free produce zero
   layout shift.
5. **Envelope editor interaction:** add/drag/delete points, curve diamonds,
   endpoint pinning, reverse-zone drawing; commit echo convergence (draw a point
   past a neighbour — the redraw must snap to the C++-sanitized order).
6. **Envelope persistence:** editor close/reopen and host session save/reload with
   a custom envelope intact (incl. reload while the editor is open —
   `envelopeState` push).
7. **Live readback:** pass playhead tracks the scratch pass at 30 Hz; ratio bar
   shows direction + magnitude including reverse fill during a bipolar scratch.
8. **DAW smoke:** one host (Logic/Live) — load, engage, automate ENGAGE, close/
   reopen editor, no stale-cache behavior.
