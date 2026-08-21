# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-08-21
**Participants:** User, Claude

## Requirements Confirmed

- **UI-02 — Factory presets, broad bank (15+):** per-console signature presets (2 per console: one clean signature + one aged/crushed variant = 10) **plus** cross-console utility presets (~5–8: lo-fi drums, tape-ish wash, extreme crush, subtle glue, reverb-forward, etc.). Delivered through the vendored **preset-manager module v1.0.6**; the Stage-3 header already reserves the preset-bar slot.
- **UI-01 completion — 6 inherited human gates, checked in Logic Pro** (primary; Standalone/Web Inspector may assist the console-error check since Logic can't attach the inspector to the release path):
  1. Visual layout pass at 620×430 (specimen placement/opacity, segment proportions, header spacing; final ±10px height call)
  2. Console switch audibly rides the 30 ms crossfade across all 5 consoles; accent + readout follow
  3. Knob feel: relative drag, shift-fine, wheel, double-click typed entry, Alt/Option-click reset
  4. Host automation of all 5 params updates the UI live
  5. Preset / session reload refreshes segments, accent, readout, and all knobs
  6. No WebView console errors (Safari Web Inspector on the dev build)
- **Final validation:** pluginval strictness 10 VST3+AU, auval, render-harness ALL PASS with digest anchors unchanged (9cf6baa8d3b61b14 / b23fe10b74526fab / dad157a01f7c393f), CHANGELOG, docs.
- **Release target: local install only** — `./scripts/build-and-install.sh O-Emulator`; publishing/packaging deferred to a later decision.

## Constraints Identified

- **Preset authoring must use denormalized (real) values** — factory presets authored as linear 0–1 fractions ignore NormalisableRange skew (house pattern).
- **Flat alphabetical preset list, no grouping** — grouped dropdowns desync the ◀/▶ walkers, which traverse the C++ flat list.
- **No "/" in preset names** — the name becomes the JSON filename; slash silently fails to save.
- `applyPresetJson` must reset all params to defaults first (partial presets otherwise inherit stale values); preset-manager v1.0.6 carries the migration hook for choice-param appends.
- **Bridge audit numbers will change:** Stage 3 verified 0↔0 native fns; preset-manager integration adds native fns. Re-run the getNativeFunction↔registration grep-diff and re-anchor the expected counts — a gap fails silently.
- Native-fn completions are dropped while the WebView is hidden — preset UI must not await a completion across an editor close.
- Harness must stay untouched and digest-identical: preset-manager is UI/state-layer only; any digest drift is a defect, not a re-anchor case.
- parameter-spec.md remains frozen (5 params); presets store existing params only.
- `build-and-install.sh` skips Standalone — for the console-error gate, rebuild the Standalone target explicitly or use the Stage-3 dev build.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Preset bank size | 15+ (10 per-console + ~5–8 utility) | User choice; consoles get signature coverage, utility presets give instant musical entry points |
| Preset delivery | Vendored preset-manager v1.0.6 | House standard; header slot reserved in Stage 3; migration hook current |
| Human-gate host | Logic Pro (AU) | Primary house DAW; covers automation refresh + session reload; inspector check via dev Standalone |
| Release target | Local install only | Publishing decided later; keeps Stage 4 scoped to UI-02 + gates + validation |
| List structure | Flat alphabetical | ◀/▶ walker parity with the C++ list |

## Open Questions

- Exact utility-preset list (names + values) — settle during plan phase by auditioning against the age/crush/reverb ranges.
- Whether preset-manager's completion-based native fns need the SafePointer/`complete(false)` pattern here — confirm against the v1.0.6 vendored source during research.

## Next Phase

Ready for: research phase
