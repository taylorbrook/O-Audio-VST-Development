# Stage 4 (Validation / Polish) — Execute SUMMARY

**Plugin:** O-simpleGrain
**Stage:** 4 of 4 (Validation / Polish)
**Date:** 2026-06-25
**Result:** ✅ All automated gates PASS · version bumped 1.0.0 · CHANGELOG authored · human DAW-listen checklist handed over (verify = `human_needed`).

---

## What was done

Stage 4 was a **validation gate, not a feature stage** (CONTEXT D2: no new product code unless a
defect surfaces). All eight plan tasks completed. One defect surfaced and was fixed minimally
(test-harness link); no product DSP/parameter/UI code changed.

### Automated validation results

| Task | Gate | Command | Result |
|------|------|---------|--------|
| 1 | Fresh build + install | `./scripts/build-and-install.sh O-simpleGrain` | ✅ VST3+AU built; both `-dev` bundles installed (7.4M); AU cache cleared; `AudioComponentRegistrar` killed; **no ALTERNATE-variant orphan** (only `-dev` present). |
| 2 | auval AU regression | `auval -v aumu OsGr OuDv` | ✅ **AU VALIDATION SUCCEEDED**; registered `aumu OsGr OuDv — Ouaricon Audio Development: O-simpleGrain-dev`. |
| 3 | pluginval Tier A | `pluginval --validate <installed VST3> --skip-gui-tests --strictness-level 10 --timeout-ms 180000` | ✅ **SUCCESS** (exit 0) — automatable params, parameter thread-safety, bus layouts, fuzz. Tier B (GUI-open) folded into human listen per plan. |
| 4 | Offline DSP harness | `cmake -DOUARICON_BUILD_TESTS=ON` → `O-simpleGrain-render-test` | ✅ **8/8 PASS**, exit 0 (after harness-link fix below). |
| 5 | Factory-preset desk-check | read 8 `applyFactoryPreset` branches vs `parameter-spec.md` | ✅ **8/8** write in-range / finite / denormal-free APVTS. |
| 6 | Version bump | `CMakeLists.txt:17` + harness `JucePlugin_Version*` | ✅ `0.1.0 → 1.0.0` (both files; code `0x100 → 0x010000`). |
| 7 | CHANGELOG | new `CHANGELOG.md` (sibling pattern) | ✅ single `[1.0.0] — 2026-06-25` entry; Validation section cites real run results. |
| 8 | Handover | 7-item DAW-listen checklist | ✅ delivered; verify = `human_needed`; Windows deferred-to-CI. |

### Harness 8-gate detail (Task 4)

```
[PASS] makes-sound            rms=0.0121 peakGrains=7
[PASS] density->continuity    contLow=0.157 contHigh=0.900
[PASS] pitch-tracks-MIDI      C2=65.3 C3=130.9 C4=260.9 Hz (within tol of 2^((N-60)/12))
[PASS] window-rect-clicks     hfRect/hfHann ratio=905.14
[PASS] freeze-sustains        rms=0.0139 peak=0.161
[PASS] scatter-async-flatter  flatSync=0.0029 flatAsync=0.3777
[PASS] stress-bounded         peakGrains=157 (cap=192), peak=0.377
[PASS] uptranspose-stable     peak=1.569 rms=0.2100
ALL PASS — 0 failure(s)
```

Confirms the Stage-3 editor rewrite caused **no engine regression**.

### Defect found + fixed (D2 path)

- **Harness-link regression** — the Stage-3 `PluginEditor.cpp` `getResource()` references
  `UIBinaryData::*` (the WebView UI resources binary-data target added in Stage 3.1), but the
  render-harness `CMakeLists.txt` (written in Stage 2) only linked `O-simpleGrain_Samples`. The
  harness compiles `PluginEditor.cpp` directly → link failed on `UIBinaryData::index_html` etc.
- **Fix (test-only, minimal):** added `O-simpleGrain_UIResources` to the harness's
  `add_dependencies` and `target_link_libraries`. Re-gated → 8/8 PASS. **No product code touched**
  (the shipping VST3/AU already link `UIBinaryData` correctly — proven by auval + pluginval).

---

## Files changed (Stage 4)

- `plugins/O-simpleGrain/CMakeLists.txt` — `VERSION "1.0.0"`.
- `plugins/O-simpleGrain/tests/render-harness/CMakeLists.txt` — `JucePlugin_Version*` → 1.0.0;
  link/depend `O-simpleGrain_UIResources` (defect fix).
- `plugins/O-simpleGrain/CHANGELOG.md` — **new**, single 1.0.0 initial-release entry.

No `plugins/O-simpleGrain/Source/*` change (D2 held — the only fix was in test CMake).

---

## Deferred to human DAW listen (verify = `human_needed`)

The 7 Stage-3 runtime criteria that cannot be driven headlessly — handed over as one batched
checklist (see handoff). Load `O-simpleGrain-dev` (`aumu OsGr OuDv`) in a DAW/Standalone with MIDI.

## Windows

Explicitly **deferred to publish/CI** (CONTEXT D3). Cross-platform CMake flags
(`NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `withUserDataFolder`)
verified static in Stage 1/3; not a Stage-4 blocker.
