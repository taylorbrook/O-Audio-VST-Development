# Stage 3: GUI — Phase 3.1 (Layout + controls + bindings) — Context

## Discussion Summary

**Date:** 2026-11-06
**Participants:** Taylor, Claude
**Inputs:** ROADMAP.md Stage 3; `mockups/v2-ui.yaml` (finalized) + `v2-integration-checklist.md`; VERIFICATION-2.6.md (Stage 2 ✓); DAW-2.5a / 2.5b / 2.6.md

## Starting state (observed)

- `Source/ui/public/index.html` is already byte-identical to `mockups/v2-ui.html`; `js/i18n.js` + `js/juce/` present.
- `Source/PluginEditor.*` is the Stage 1 placeholder: resource map (`granulatorUiResource`) only, no editor class.
- Processor already has: `uiLanguage`, scale STATE (`getCustomScale`, `setCustomScale`, `getCollectionTable`, `get/setDegreeMask`, `getScaleStateRevision`), pitch telemetry (`getPitchNote` / `getPitchConfident`), engine `getSrcTelemetry` (11 sources) + `getDestTelemetry` (113 dests).
- Missing (later phases): peak/wave ring, grain seqlock snapshot, counts/peaks/lfoPhase/onset/loopGainDb fields, `getWindowCurve`.

## Requirements Confirmed

- **UI-01** (must): all 220 parameters reachable and bound both ways; host automation + preset recall reflected; per-stream controls by stream selector.
- Integration follows `v2-integration-checklist.md` sections 1–8 as the contract (member order, 140/59/21 relay tables equal `createParameterLayout()` by TYPE and ID, 21 native fns, resource provider bare-path map, Windows WebView2 block, fixed 1200 × 800 non-resizable).
- Readouts via `getScaledValue()`; Tempo/Free swap shows the division knobs without writing params.

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| D26 | 3.1 native-function scope | **Register all 21 in 3.1.** `getTelemetry` returns what exists now — `scaleRev`, `sources[11]`, `modNorm/modRouted[113]`, `pitchMidi/pitchClarity` — and well-formed empties for the rest (`wave` / `grains` count 0, peaks 0, `loopGainDb` non-finite). `getWindowCurve` may return a flat stub until 3.2. 3.2 fills wave ring + grain seqlock + window curve; 3.3 the remaining matrix/meter fields. | No promise ever hangs (pattern_webview_native_fn_bridge_gap); native-fn parity 21 = 21 gated from the first commit; each later phase only changes payloads. |
| D27 | Deferred DAW pass (DAW-2.5a 1–8, DAW-2.5b 1–5, DAW-2.6 1–3 = 16 items) | **Run once at 3.3 verify** with the finished UI. | Mod matrix + source meters + onset/pitch readouts make the tempo / LFO / onset items observable. FAIL routing unchanged: 2.5a → FUNC-04 / QUAL-03; 2.5b → FUNC-08 / DSP-08; DAW-2.6 item 1 → D21 fallback 600 s. |
| D28 | Design latitude | **v2 locked, defects only.** Ship as designed; change only for breakage (clipping, dead bindings, contrast/fit gate failures). Settings popover (`#gear-btn` at (1124, 62), opens down-left) **accepted as placed** — closes the checklist §1 designer item. | Mockup finalized and pre-verified; avoid regressions from taste edits mid-integration. |
| D29 | Interaction-state coverage gate | **`tests/i18n-states.json` lands in 3.1**, driving: FX tab, MOD tab, Sync = Tempo, scale list open, custom-scale editor open, settings popover open. | check-ui-labels otherwise measures only the STREAMS page (339 of 472 `[data-i18n]` nodes unmeasured). |

## Constraints Identified

- Telemetry is a PULL at ~30 Hz from the page; no `juce::Timer`/`emitEvent` push; editor never touches pool/scheduler; torn read → page keeps its last frame.
- No parameter writes from `parameterChanged` or the audio thread (VST3 nested-write swallow).
- Scale revision bumped only by `setStateInformation` / preset apply; page re-pulls `getScaleState` on `scaleRev` change. Two-reopen persistence check (stale `<CustomState>` child).
- Windows: `withBackend(webview2)` + `withUserDataFolder()`; every `webView->` null-guarded; never `setVisible(false)` on the WebView.
- `preset-manager.js` / EB Garamond served from modules via `getResource`, not copied into `Source/ui/public/` (gitignored `modules/` copy never committed).
- Stage 2 audio must not move: harness default set + N3/N2/N1/N0 `--strict` + R12 stay green (the editor is excluded from headless builds; any processor accessor added must be copy-only).
- All five UI gates (check-i18n, fr-lint, zh-lint, check-ui-labels, boot-all-uis `--strict-tips`) pass before the 3.1 commit; exit 77 is not a pass.

## Open Questions (for research)

- Exact `getTelemetry` JSON shape the page expects for the stub fields (read the controller's consumer, not the checklist prose) — confirm empty arrays don't throw in the 30 Hz loop.
- Whether `getParameterDefaults` must special-case the 12 division Choice knobs (index) — verify against `bindChoiceKnob` reset.
- How to drive Sync = Tempo and the custom editor in `i18n-states.json` (selectors / clicks available to check-ui-labels' state runner).
- Host-automation reflection test method for 220 params (param-dump diff via Standalone + Playwright, or pedalboard set → editor read).

## Carried items (not 3.1 scope)

- Env 1e30 pin → Stage 4. Harness: C7 machine-speed calibration, event-aware `renderPool` (Z6a). W2o — widen seeds, never the threshold.

## Next Phase

Ready for: research phase (`/plugin-research O-Granulator 3-gui`)
