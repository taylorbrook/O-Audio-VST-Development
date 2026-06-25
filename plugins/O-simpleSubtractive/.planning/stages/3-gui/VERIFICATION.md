# Stage 3 (GUI) — VERIFICATION

**Plugin:** O-simpleSubtractive
**Stage:** 3 of 4 — GUI
**Date:** 2026-06-25
**Method:** goal-backward analysis against CONTEXT.md success criteria + critic review + technical validation (build / auval / pluginval).
**Verdict:** ✅ **PASS** (1 visual item deferred to manual UAT; 1 item — preset *content* — carried to Stage 4 as planned)

---

## Technical validation (objective gates)

| Gate | Result | Evidence |
|------|--------|----------|
| VST3 builds (macOS) | ✅ | `O-simpleSubtractive_VST3` clean; 4.3M bundle installed |
| AU builds (macOS) | ✅ | `O-simpleSubtractive_AU` clean; 4.3M bundle installed |
| AU registers | ✅ | `auval -a` → `aumu OSiS OuDv — O-simpleSubtractive-dev` |
| AU validation | ✅ | `auval -v aumu OSiS OuDv` → **AU VALIDATION SUCCEEDED** (render @ 22050–192000 Hz, 1-ch, bad-max-frames, parameter set/schedule/ramp, MIDI — all PASS) |
| pluginval s10 | ✅ | **SUCCESS** (background-thread state, parameter thread-safety, buses, fuzz parameters) |
| Install + cache clear | ✅ | build-and-install.sh Phase 4 dual-variant sweep + AU cache clear + AudioComponentRegistrar kill |
| Critic review | ✅ | UI + Foundation critics: **NO BLOCKERS**; parity 3 native-fns / 4 events / 20 params; DSP-untouched invariant holds |

## Goal-backward: success criteria (from CONTEXT.md)

1. **WebView opens; single-page left→right signal-path layout, projector-readable (UI-06)** — ✅ *built & wired*; 5-column flex layout (OSC | FILTER | FILTER ADSR | AMP ADSR | VOICE/OUT), editor 1180×820 resizable. Resource provider serves all 5 assets (bare-path equality, BinaryData parity verified). **Visual rendering = manual UAT** (see below).
2. **All 20 controls two-way bound** — ✅ *verified*. 16 `WebSliderRelay` + 4 `WebComboBoxRelay`, each with a 3-arg attachment; IDs are `ParamIDs` constants (drift = compile error). Combos repopulate on `propertiesChangedEvent` (no empty dropdowns).
3. **Cutoff/res/slope/type/filter-env move the filter curve over the spectrum; curve matches heard (QUAL-02); self-osc peak** — ✅ *data path verified; QUAL-02 by construction*. Headline overlays `getCurve()` over `getSpectrum()` on an identical 256-bin log-f axis; the curve is the closed-form of the running SVF (validated to 0.00 dB in the Stage-2 render-harness). dB window `[-90,+18]` keeps the resonance/self-osc peak on-canvas. **Live visual = manual UAT.**
4. **Oscilloscope + dual-ADSR independent** — ✅ *built & wired*. `drawScope()` (128 pts post-filter); two ADSR canvases driven by the 8 ADSR params + independent `envUpdate` playheads.
5. **Signal-path diagram + per-control tooltips + preset selectable** — ✅ *built & wired*. SVG `updateDiagram()` reflects osc/filter/envelope state; `TIPS` map (30 entries, all 20 params + visuals + lessons); 5 preset buttons wired to `applyFactoryPreset`. **Preset *content* deferred to Stage 4 (FUNC-06)** — bridge is live (W1).
6. **Builds VST3 + AU macOS; CMake Windows-configured; auval + pluginval pass** — ✅ *verified* (see gates table). Windows flags present: `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_CURL=0`; `withUserDataFolder` in `#if JUCE_WINDOWS`.
7. **No audio-thread FFT/alloc; UI smooth at 30 Hz (PERF-01)** — ✅ *verified*. No audio-thread code changed (DSP-untouched invariant); all FFT/curve/scope/env work on the editor Timer @ 30 Hz, gated on `webView != nullptr`.

## Requirements coverage

UI-01 ✅ · UI-02 ✅ · UI-03 ✅ · UI-04 ✅ · UI-05 ✅ · UI-06 ✅ · UI-07 ✅ (hook; content S4) · COMPAT-02 ✅ · QUAL-02 ✅ (by construction) · PERF-01 ✅

## Applied during verify

- **N1 fixed** — global QWERTY keydown now early-returns while a `<select>`/`<input>`/`<textarea>` is focused, so dropdown type-ahead/selection isn't hijacked into note triggers. (`app.js`; `node --check` clean; rebuilt + revalidated.)

## Carried forward

- **W1 → Stage 4 (FUNC-06):** preset-tour buttons fire the live bridge but are audibly inert until `applyFactoryPreset` snapshots are filled in. Three-way name parity (HTML `data-preset` ≡ JS `LESSONS` ≡ future C++ `name==`) is in place; Stage 4 only fills the C++ bodies.
- **N2/N3/N4 (cosmetic/intentional):** spectrum-floor dB mismatch (−100 vs −90), per-tick wheel gesture open/close, static amp-env arrow. Optional Stage-4 polish; none affect correctness.

## Manual UAT (recommended before Stage 4 / ship)

The wiring, math, and validation are objectively verified; the one thing automated checks cannot confirm is **how it looks and feels on screen**. Recommend opening the plugin (`/show-standalone O-simpleSubtractive` or in a DAW) to eyeball:
- The headline curve visibly tracks cutoff/resonance/slope sweeps and the filter envelope; harmonics above cutoff fall away; self-osc shows a peak.
- The oscilloscope morphs; the two ADSR displays move independently.
- The signal-path diagram highlights the active stage; tooltips read clearly on a projector.

## Conclusion

Stage 3 (GUI) **achieves its goal**: the validated Stage-2 synth now has a complete, cross-platform-configured WebView teaching UI with all 20 parameters bound and the headline filter-curve-over-spectrum + scope + dual-ADSR + diagram + tooltips + preset hook in place. Objective gates (build, auval, pluginval, critic) all green; the DSP contract is untouched. Visual UAT and preset content are the only open items, both expected (manual eyeball + Stage 4 respectively).
