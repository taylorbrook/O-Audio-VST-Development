# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-07-10
**Participants:** User, Claude
**Revision:** rev-1 (fresh CONTEXT for Stage 3 per Stage 2 full-verify handoff)

## Requirements Confirmed

- **UI-01 (should):** 7 logical sections — Bow / Body / Strings (incl. per-string detune) / Expression / Drone / Output / Microtonal — all visible in a **single-view sectioned grid** (no tabs, no drawers). Everything reachable at all times; window sized accordingly (O-Bowed is 900×600; O-Contrabass may need ~1000×650 for 31 params + 3 visualizations — final size decided in mockup).
- **UI-02 (nice → in-scope):** All three Phase 3.3 visualizations are **confirmed for v1.0**, not deferred:
  1. **Schelleng wedge** — real-time bow operating point (X = bow speed, Y = bow pressure) inside the playable-wedge bounds.
  2. **Body spectrum display** — 8 body-resonator mode peaks reacting to Body Size / Damping.
  3. **Output level meter** — VU/RMS near the Output section (saturator + limiter chain from Phase 2.6a makes this genuinely useful).
- **Parameter count is 31** (not ROADMAP's stale "29"): 29 Stage-1 params + MASTER_SAT_AMOUNT + LIMITER_CEILING_DB added in Phase 2.6a. All 31 get WebView bindings in Phase 3.1/3.2.
- Full Ouaricon microtonal convention in the Microtonal section: Reference Pitch, Tuning System (Scala-TUN / MTS-ESP / 12-TET), Note Expression toggle, Scala/TUN file picker.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Mockup path | **ui-mockup workflow first** — run `/ui-mockup O-Contrabass` (ui-design-agent iteration) BEFORE Stage 3 research/plan | Matches ROADMAP Phase 3.1 assumption ("mockup created in separate workflow"); visual iteration happens in browser before any C++ work |
| Visual direction | **Match Ouaricon house style, reference = O-Bowed** | Sibling bowed-string plugin; closest domain match — bow/body/string section vocabulary and styling transfer directly |
| Style tokens (from O-Bowed `Resources/ui/index.html`) | Warm parchment `#F5E6D3` background, brown text `#3C2F2F` / frame `#5C4033`, sage-green accents (`#6B8E4E` family), Garamond/Georgia serif, 55px knobs (`--knob-size`), soft brown shadows | Concrete tokens for ui-design-agent to inherit; O-Contrabass may darken the palette slightly toward its bass identity, but must stay recognizably in-family |
| Layout | **Single-view sectioned grid**, all 7 UI-01 sections simultaneously visible | User preference; typical Ouaricon pattern; no controls hidden during performance |
| Visualization scope | All 3 (Schelleng wedge + body spectrum + level meter) in v1.0 | User confirmed full Phase 3.3 scope; ROADMAP's "defer to v1.1" escape hatch NOT taken |
| Editor base size | Start from O-Bowed's 900×600; grow only as mockup requires | Suite consistency; fixed-size (non-resizable) unless mockup phase decides otherwise |

## Constraints Identified

- **WebView critical patterns apply** (juce8-critical-patterns #3, #7, #8, #11–#16, #19, #20): resource provider gets bare **paths** not URLs; `Juce` ES-module namespace (NOT `window.__JUCE__`) passed to any shared panel taking a `juceApi` arg; Windows needs `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `withUserDataFolder()`.
- **Knob readouts must use `SliderState.getScaledValue()`** — never a JS min/max map (O-MicrotonalSampler shipped ~20 versions with 2× wrong readouts). Dblclick-reset needs a `getParameterDefaults` native fn. `ui_frontend_check.js` (O-MicrotonalSampler v1.23.7) is the reusable audit.
- **Native-fn bridge gaps fail silently** — grep-diff JS `getNativeFunction` names vs C++ `withNativeFunction` registrations as a Stage 3 gate.
- **Async FileChooser completions (Scala/TUN picker) need `SafePointer`** and must bail with bare `return` on the null path (no `complete()` call — UAF).
- **Shared tuning-panel module:** if the `scala-tuning-engine` tuning-panel.js is adopted for the Microtonal section, it must receive the `Juce` namespace object; O-Wind v1.16.1 is the corrected reference integration.
- **Render-harness breakage:** the Stage-2 render harness compiles `PluginEditor.cpp` under `JUCE_WEB_BROWSER=0` — when the editor gains WebView types, guard `createEditor` with `#if JUCE_WEB_BROWSER` / drop PluginEditor.cpp from harness sources, and **re-run the harness + reproduce-goldens.sh at the start of Stage 3 execute and again before verify** (19-entry byte-identical battery is the regression bar).
- **Visualization CPU budget:** 60 fps max, throttled updates (ROADMAP Phase 3.3 test criterion — no CPU spikes). Schelleng wedge and body spectrum need C++→JS data feeds (likely a timer-driven event emit, not per-sample).
- **Monophonic instrument** — no polyphony UI affordances needed; Active Strings (1–4) is an Int param, per-string detune is 4 float knobs/sliders.

## Sonic-Identity Notes for the Mockup

- Dual identity (UI-02): cinematic-orchestral **and** drone-experimental. Within O-Bowed's parchment house style this can read as: same layout vocabulary, slightly deeper/darker tonal balance, Drone section visually distinct (its params default to 0 and "wake up" the drone territory).
- The Schelleng wedge is the centerpiece differentiator — it belongs visually adjacent to the Bow section (Speed/Pressure/Position knobs).

## Open Questions (for research phase)

- How does O-Bowed feed its UI meters/visuals (if any) — reuse its C++→JS eventing pattern or build a new throttled emitter for wedge/spectrum/meter?
- Schelleng wedge math source: derive wedge bounds from HyperbolicFriction/BowModel params already in `Source/DSP/` (shared module with O-Bowed) — what's the cheapest correct bound computation for display?
- Body spectrum: read the 8 mode frequencies/Qs directly from the body resonator's current coefficients, or precompute per Size/Damping on the message thread?
- Preset browser: adopt `OuariconPresetManager` + shared preset-manager module (v1.0.2) now in Stage 3, or defer preset bank wiring to Stage 4? (ROADMAP Phase 3.2 lists "preset selection menu" — research should confirm the module version to pull.)
- Editor window final dimensions once mockup lays out 31 params + 3 visualizations in single view.

## Next Phase

Ready for: **ui-mockup workflow** (`/ui-mockup O-Contrabass`), then **research phase** (`/plugin-research O-Contrabass 3-gui`).
The mockup is a Stage-3 entry gate: Phase 3.1 copies its HTML into `Source/ui/public/index.html`.
