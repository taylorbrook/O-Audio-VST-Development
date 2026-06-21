# Stage 4 (Polish) — CONTEXT

**Date:** 2026-06-20 · **Mode:** express (auto-derived; one user decision recorded below)

## Goal
Final refinement + validation for O-simpleFM v1.0.0: factory presets, aliasing/edge-case
validation, deferred Stage-3 critic notes, changelog. DSP frozen since Stage 2; the
Naturalist WebView is finalized from Stage 3 — touch it minimally and without regression.

## User decision (recorded this session)
**Factory presets → "Full preset manager."** Integrate the suite-canonical `preset-manager`
module (C++ `OuariconPresetManager` + on-disk JSON factory/user presets + WebView native
functions) AND add a preset **browser panel** (list / prev-next / save / delete) to the UI,
while **keeping** the existing pedagogical "Lesson Presets" tour. Suite norm confirmed via
O-AnalogEQ (shared module, `ouaricon_add_module`, unconditional `initializeFactoryPresets`,
serves `preset-manager.js` via resource provider). Presets stay in the WebView, NOT the DAW
host program menu (suite norm: `getNumPrograms()` stays `return 1`).

## In-scope (Stage 4)
1. **Preset manager (full):** module wiring (CMake), `FactoryPresets.{h,cpp}` (6 presets:
   Default + the 5 lessons, raw→normalized via `convertTo0to1`), processor member +
   `getStateAsXml/setStateFromXml` routing, 10 native functions, browser-panel UI, keep tour.
2. **Aliasing audit (QUAL-01):** high index + high feedback + high pitch render checks
   (extend offline render harness; the 2× polyphase-IIR + key-tracked ceiling is the v1.0 chain).
3. **Edge cases:** sample rates 44.1/48/88.2/96/192 k, buffer sizes 64–2048, state save/restore.
4. **Deferred Stage-3 critic notes:** (a) keyboard-accessible tooltips (focus/blur + Escape) and
   focusable controls; (b) Windows fleuron/♪ glyph font fallback; (c) MIME `charset=utf-8` on
   served text resources.
5. **Validation:** `auval` SUCCEEDED, `pluginval --strictness-level 10`, render harness green.
6. **Release:** CHANGELOG.md (v1.0.0), version confirmed 1.0.0.

## Out-of-scope
- DSP changes (frozen). v1.1 non-sine operators / 4× OS remain deferred.
- DAW host program-menu presets (suite deliberately does not use them).
- Windows build (macOS-only this session; cross-platform flags already set at Foundation).

## Constraints / regression guards
- Editor member order **relays → WebView → attachments** must stay intact.
- `getNativeFunction` for the preset UI comes from the `Juce` ES-module namespace, NOT
  `window.__JUCE__` (see suite memory: namespace-vs-postMessage).
- Loading a preset via C++ `setValueNotifyingHost` propagates through the relays → JS
  `valueChangedEvent` → knob visuals update automatically (no extra wiring needed).
- Render harness (`-DOUARICON_BUILD_TESTS=ON`) is the permanent DSP regression gate.
