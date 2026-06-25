# Stage 4 (Validation / Polish) — CONTEXT

**Plugin:** O-simpleGrain
**Stage:** 4 of 4 (Validation / Polish)
**Date:** 2026-06-25
**Mode:** manual (interactive discuss)
**Source:** Interactive discuss session (3 decisions locked) + ROADMAP.md Stage 4 baseline + Stage 3 VERIFICATION deferrals.

---

## Goal

Close out O-simpleGrain: prove the shipped granular instrument is stable, correct, and distribution-ready. Stage 4 is a **validation gate**, not a feature stage — no new code unless validation surfaces a defect.

---

## Locked Decisions (this discuss)

1. **Sequencing — Automated first, then human listen.**
   Run all automatable validation (pluginval VST3+AU, preset audit, artifact/aliasing/freeze audit via offline harness, drag-drop config smoke test, changelog), install a **fresh build**, then hand the user a single consolidated DAW-listen checklist covering the 7 deferred Stage-3 runtime criteria. Human listen is the *last* step, batched.

2. **Scope — Baseline validation only.**
   Stick to the ROADMAP Stage 4 baseline. No CPU/optimization pass, no new edge-case test code. If validation finds a defect, fix it (that's in-scope); otherwise no new code.

3. **Windows — Defer entirely to publish/CI.**
   No local Windows work. The publish stage's GitHub Actions CI builds + validates Windows VST3. Stage 4 does NOT block on Windows. (The cross-platform CMake config — `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `withUserDataFolder` — is already in place from Stage 1/3 and verified statically; no action needed here.)

---

## The 7 Deferred Stage-3 Runtime Criteria (fold into the listen checklist)

These could not be driven headlessly and were deferred from Stage 3 verify. They become the human DAW-listen checklist at the END of Stage 4:

1. Grain **cloud accumulates** — density thickens it, spray widens it (UI-01).
2. **Spectrum** shows discrete sidebands at scatter=0, smears to noise at high scatter (UI-04, DSP-05 visual).
3. **Scope** moves with output (UI-04).
4. **Grain/overlap/CPU readout** counts `N/192` live (UI-05).
5. **Freeze pins the playhead** — ❄ pin + shaded spray band; freeze/unfreeze click-free (FUNC-03/QUAL-01).
6. **Window inset** redraws on Window combo change (UI-03); **8 presets** each snap knobs/combos/toggle + caption/active state (FUNC-06); **every control** shows its hover tooltip (FUNC-07).
7. **Drag-drop a .wav AND Load…** both granulate a user source (FUNC-05); **host-automation → UI** round-trip.

---

## Validation Baseline (ROADMAP Stage 4)

| Item | Method | Automatable? |
|------|--------|--------------|
| pluginval strictness sweep (VST3) | `pluginval --strictness-level 10` on the VST3 | ✅ yes |
| pluginval strictness sweep (AU) | AU via `auval` (pluginval AU support is limited) + `auval -v aumu OsGr OuDv` | ✅ yes |
| Preset audit | Re-confirm the 8 `applyFactoryPreset` snapshots write APVTS without NaN/denormal; offline harness can render each | ✅ partial (audible char = human) |
| Artifact / aliasing / freeze audit | Offline render harness (`-DOUARICON_BUILD_TESTS=ON`) re-run — 8 DSP gates | ✅ yes |
| Drag-drop smoke test (macOS) | Config/static check of the base64 + native-fn path; live drag = human | ✅ partial (live = human) |
| Changelog | Author CHANGELOG / version bump 0.1.0 → release candidate | ✅ yes |

---

## Constraints / Gotchas (must hold)

- **Cache-clear discipline:** every fresh build MUST clear the AU cache and sweep BOTH `-dev` and unsuffixed variant bundles before install (project memory: dev/release variant shadowing). Use `./scripts/build-and-install.sh O-simpleGrain` (its Phase 4 sweeps both variants).
- **Offline harness is the DSP gate**, not a substitute for the human listen on the viz/UI criteria.
- **No new code unless defect found** — keep the diff minimal.
- Version currently `0.1.0` in CMakeLists.txt.

---

## Success Criteria (Stage 4 complete when)

1. pluginval passes (VST3) at the project's standard strictness; `auval` SUCCEEDED (AU).
2. Offline DSP harness re-run: 8/8 PASS (no regression since Stage 2).
3. 8 preset snapshots verified to write valid APVTS state (no NaN/denormal/out-of-range).
4. Fresh build installed (`O-simpleGrain-dev`, `aumu OsGr OuDv`), AU cache cleared, both variants swept.
5. CHANGELOG authored.
6. Consolidated DAW-listen checklist (7 criteria) handed to the user — Stage 4 verify records these as `human_needed` until the user confirms.
7. Windows explicitly marked deferred-to-CI (not a blocker).
