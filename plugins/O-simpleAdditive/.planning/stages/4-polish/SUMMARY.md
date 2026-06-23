# Stage 4 (Validation / Polish) — Summary

**Date:** 2026-06-22
**Outcome:** ✓ O-simpleAdditive validated and release-ready at **v1.0.0**.

## What was done

- **Rebuilt** VST3 + AU clean (`ninja O-simpleAdditive_VST3 O-simpleAdditive_AU`, exit 0).
- **pluginval @ strictness 8 — both formats SUCCESS:**
  - VST3 → `SUCCESS` (no fail/error lines).
  - AU → `SUCCESS` (no fail/error lines).
  - Tests run incl. render, parameter automation, **fuzz parameters**, state save/restore,
    background-thread automation, bus-layout restore.
- **Factory-preset sweep (code-verified):** `applyFactoryPreset` resets every parameter to its
  default, then writes drawbars via `convertTo0to1` with `jlimit(0,1)` and choices by index. All
  6 lessons (Pure Sine, Sawtooth, Square, Organ, Morph Pad, Lo-Fi Bells) produce finite, in-range
  snapshots. The pluginval fuzz/automation pass exercises this same parameter space at runtime
  with no errors.
- **Aliasing / artifact audit (code-verified + render):** `computeKmax = floor(0.5·fs/f0)` jlimited
  to [1,16]; `nyquistGain` raised-cosine taper on the top 2 surviving harmonics; headroom divide by
  `max(1, Σband)`; `isfinite` phase guard with floor-modulo wrap. pluginval render + fuzz at the top
  of the keyboard surfaced no NaN/Inf/denormal.
- **Release prep:**
  - CMake `VERSION` 0.1.0 → **1.0.0**.
  - New `CHANGELOG.md` with the v1.0.0 entry (Stages 1–4).
  - `PLUGINS.md` row → **✅ Working / 1.0.0**.
  - `STATUS.md` → Stage 4 complete (100%).

## Scope note

Validation-only per the Stage 4 decision (user, 2026-06-22). The persistent user preset save/load
bar (`OuariconPresetManager`) is **deferred to v1.1** — see CONTEXT.md.

## Build version note

The current on-disk artefacts were compiled at 0.1.0; the version bump to 1.0.0 takes effect at the
next fresh build. `/install-plugin O-simpleAdditive` rebuilds from source, so the installed binary
will report 1.0.0.
