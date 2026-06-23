# Stage 4 (Validation / Polish) — Context

**Date:** 2026-06-22
**Mode:** manual
**Goal:** Final-stage validation and release prep for O-simpleAdditive v1.0.0. No new DSP, no new UI.

## Scope decision

Stage 3 deferred one item: a **persistent user preset save/load bar** (`OuariconPresetManager`,
the sibling element O-simpleFM ships). Decision (user, 2026-06-22): **defer to v1.1.** The 6
curated factory lesson presets already carry the pedagogical goal; a user-savable bar is a clean
additive v1.1 feature and avoids a new WebView element + module-wiring + re-validation pass at the
final release gate.

**Stage 4 is therefore validation-only.**

## Tasks

1. Rebuild VST3 + AU fresh.
2. pluginval (VST3 + AU) at strictness 8 — render, automation, parameter fuzz, state, threading.
3. Factory-preset sweep — all 6 lessons apply finite, in-range snapshots; default patch unchanged.
4. Aliasing / artifact audit — confirm the exact per-note band-limit (`Kmax = floor(0.5·fs/f0)`,
   raised-cosine taper on top 2 harmonics, headroom divide) holds across the keyboard range.
5. Version bump 0.1.0 → 1.0.0; CHANGELOG.md; flip PLUGINS.md → ✅ Working / 1.0.0; STATUS.md.

## Out of scope (→ v1.1)

- Persistent user preset save/load bar (`OuariconPresetManager` + `preset-manager.js`).
- Per-partial mod-env decay routing (already deferred at Stage 0).
- Windows VST3 build (Windows CI gate; cross-platform flags already wired in CMake).

## Non-regression contract

Default patch (H1=100%, all else 0; scan=0, spectralDecay=0, bitDepth=Off) must stay a pure sine,
bit-identical to the Stage 2.3 render. No change to the audio render path in Stage 4.
