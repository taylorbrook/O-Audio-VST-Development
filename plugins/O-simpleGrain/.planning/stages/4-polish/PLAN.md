# Stage 4 (Validation / Polish) — PLAN

**Plugin:** O-simpleGrain
**Stage:** 4 of 4 (Validation / Polish)
**Date:** 2026-06-25
**Inputs:** `4-polish/CONTEXT.md` (3 locked decisions), `4-polish/RESEARCH.md` (verified recipe + execution order).

---

## Goal

Close out O-simpleGrain as a stable, correct, distribution-ready 1.0.0 release. Stage 4 is a
**validation gate, not a feature stage** — the only product-file edits are the version bump
(0.1.0 → 1.0.0, confirmed by user) and a new CHANGELOG.md. No other code unless a validation
run surfaces a defect (D2); any fix is in-scope but minimal and re-gated. The stage ends by
handing the user a single consolidated DAW-listen checklist (the 7 deferred Stage-3 runtime
criteria) and recording verify as `human_needed`.

**Locked decisions carried in:** D1 automated-first/human-listen-last · D2 baseline-only/no-new-code · D3 Windows deferred-to-CI.
**Confirmed this plan:** version → **bump to 1.0.0 now**; CHANGELOG → **single `1.0.0` initial-release entry**; pluginval Tier B → **best-effort, gate on Tier A**.

---

## Tasks

1. [ ] **Fresh build + install (cache clear + dual-variant sweep)**
   - Run: `./scripts/build-and-install.sh O-simpleGrain` — Phase 4 sweeps BOTH `O-simpleGrain.{vst3,component}` and `O-simpleGrain-dev.{…}`, kills `AudioComponentRegistrar`, clears `~/Library/Caches/AudioUnitCache`.
   - Confirm installed bundles present: `~/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3`, `~/Library/Audio/Plug-Ins/Components/O-simpleGrain-dev.component`.
   - Files: none (prep step; no commit)
   - Depends on: none
   - Gate: build succeeds (VST3+AU+Standalone); both bundles installed; no `⚠ ALTERNATE-variant` orphan left behind.

2. [ ] **auval — AU regression re-check**
   - Run: `auval -v aumu OsGr OuDv` → expect `AU VALIDATION SUCCEEDED`. Then `auval -a | grep -i simplegrain` confirms registration.
   - Files: none
   - Depends on: Task 1 (must validate the freshly-installed, cache-cleared bundle)
   - Gate: SUCCEEDED.

3. [ ] **pluginval Tier A — VST3 strictness gate (primary)**
   - Locate `pluginval` (`/Applications/pluginval.app/Contents/MacOS/pluginval` || PATH).
   - Run on the INSTALLED bundle: `"$PLUGINVAL" --validate "$HOME/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3" --skip-gui-tests --strictness-level 10 --timeout-ms 180000`.
   - Tier B (best-effort, NOT a blocker): same without `--skip-gui-tests`, `--timeout-ms 600000`; record result but do not gate on it (WebView GUI-open folds into the human listen).
   - Files: none
   - Depends on: Task 1
   - Gate: Tier A → `All tests PASSED`. An allocation/lock flag in `processBlock` at strictness 10 = real DEF → D2 fix path.

4. [ ] **Offline DSP harness — 8-gate regression re-run**
   - Build + run:
     `cmake -S . -B build -G Ninja -DOUARICON_BUILD_TESTS=ON` →
     `cmake --build build --target O-simpleGrain-render-test` →
     `./build/plugins/O-simpleGrain/tests/render-harness/O-simpleGrain-render-test; echo exit=$?`
   - Re-runs as-is (no new harness code, D2). Proves Stage-3 editor rewrite caused no engine regression.
   - Files: none
   - Depends on: none (independent of install; can run in parallel with 2–3)
   - Gate: 8/8 PASS, exit 0 (gates: makes-sound, density→continuity, pitch-tracks-MIDI, window-rect-clicks, freeze-sustains, scatter-async, stress-bounded ≤192, uptranspose-stable).

5. [ ] **Factory-preset state desk-check (8 snapshots)**
   - Read the 8 `applyFactoryPreset` branches in `PluginProcessor.cpp` (~line 788+) against `parameter-spec.md` ranges. Confirm each branch touches only valid param IDs with literals inside declared `NormalisableRange` → in-range, finite, denormal-free by construction.
   - Presets: `Single Grain` · `Pitched Buzz` · `Fragments` · `Smooth Cloud` · `Frozen Pad` · `Asynchronous Cloud` · `Granular Fire` · `Rect Click`.
   - Files: none (read-only audit). Audible distinctness → human checklist (Task 8 item 6).
   - Depends on: none
   - Gate: 8/8 write in-range/finite APVTS. Suspect literal → targeted render check (only then, per D2).

6. [ ] **Version bump 0.1.0 → 1.0.0 (both files)**
   - `plugins/O-simpleGrain/CMakeLists.txt:17` → `VERSION "1.0.0"`.
   - `plugins/O-simpleGrain/tests/render-harness/CMakeLists.txt:52-53` → `JucePlugin_VersionString="1.0.0"`, `JucePlugin_VersionCode=0x010000`.
   - Files: `CMakeLists.txt`, `tests/render-harness/CMakeLists.txt`
   - Depends on: Tasks 2–5 green (don't stamp 1.0.0 on a build that failed a gate)
   - Gate: both files read 1.0.0; no other product file changed.

7. [ ] **Author CHANGELOG.md (single 1.0.0 initial-release entry)**
   - Follow sibling pattern (`plugins/O-simpleFM/CHANGELOG.md`): title `# Changelog — O-simpleGrain`, "Format loosely follows Keep a Changelog", `## [1.0.0] — 2026-06-25` block with **Added / Changed / Fixed / Validation** subsections.
   - **Added:** granular engine (preallocated grains, 192 global cap, atomic source hot-swap), 18 APVTS params, WebView field-guide UI (4 live visualizations, readouts, per-control tooltips), 8 factory presets, drag-drop + Load… source loading.
   - **Validation:** cite Stage-4 gates — auval SUCCEEDED, pluginval Tier A PASSED, offline harness 8/8, 8 presets in-range. Note Windows VST3 deferred to publish/CI.
   - Files: `CHANGELOG.md` (new)
   - Depends on: Tasks 2–5 (Validation section quotes their results)
   - Gate: file exists, mirrors sibling structure, Validation section reflects actual run results.
   - **Commit boundary:** Tasks 6+7 are the only product-file diff — commit together.

8. [ ] **Hand over consolidated DAW-listen checklist + mark deferrals**
   - Present the 7 deferred Stage-3 runtime criteria as one batched human checklist (load `O-simpleGrain-dev`, `aumu OsGr OuDv`, in DAW/Standalone with MIDI):
     1. Grain cloud accumulates — density thickens, spray widens (UI-01).
     2. Spectrum: discrete sidebands at scatter=0 → noise at high scatter (UI-04/DSP-05).
     3. Scope moves with output (UI-04).
     4. Grain/overlap/CPU readout counts `N/192` live (UI-05).
     5. Freeze pins playhead — ❄ pin + shaded spray band; freeze/unfreeze click-free (FUNC-03/QUAL-01).
     6. Window inset redraws on combo change (UI-03); 8 presets each snap knobs/combos/toggle + caption/active state AND sound audibly distinct (FUNC-06); every control shows hover tooltip (FUNC-07).
     7. Drag-drop a .wav AND Load… both granulate a user source (FUNC-05); host-automation → UI round-trip.
   - Record Stage-4 verify as `human_needed` until user confirms. Explicitly mark Windows **deferred-to-CI** (not a blocker).
   - Files: STATUS.md (verify phase updates), `4-polish/VERIFICATION.md` (created in verify, not here)
   - Depends on: Tasks 1–7 (checklist is handed over only after automated gates pass + fresh build installed)
   - Gate: checklist delivered; deferrals recorded.

---

## Execution Order & Parallelism

```
Task 1 (build+install) ──┬──> Task 2 (auval) ───┐
                         └──> Task 3 (pluginval)─┤
Task 4 (harness) ────────────────────────────────┼──> Task 6 (version) ─┐
Task 5 (preset desk-check) ──────────────────────┘                       ├──> Task 8 (handover)
                                                   Task 7 (CHANGELOG) ────┘
```
- Tasks 4 and 5 are independent of the install and may run alongside 1–3.
- Tasks 6+7 only after all gates (2–5) are green; they form the single commit.
- **A defect at any gate → minimal in-scope fix → re-run that gate (D2), then proceed.**

---

## Success Criteria (Stage 4 complete when)

- [ ] Fresh build installed (`O-simpleGrain-dev`, `aumu OsGr OuDv`); AU cache cleared; both variants swept (no orphan).
- [ ] `auval -v aumu OsGr OuDv` → SUCCEEDED.
- [ ] pluginval Tier A (`--skip-gui-tests --strictness-level 10`) → All tests PASSED on the installed VST3. (Tier B best-effort, non-blocking.)
- [ ] Offline DSP harness 8/8 PASS, exit 0 (no regression since Stage 2).
- [ ] 8 factory presets desk-checked → in-range, finite, denormal-free APVTS.
- [ ] Version bumped 0.1.0 → 1.0.0 in BOTH `CMakeLists.txt` and harness `CMakeLists.txt`.
- [ ] CHANGELOG.md authored (single 1.0.0 initial-release entry, sibling structure, Validation section cites real results).
- [ ] Consolidated 7-item DAW-listen checklist handed to user; verify recorded `human_needed`.
- [ ] Windows explicitly marked deferred-to-CI (not a blocker).

---

## Risks / Gotchas (must hold)

| Risk | Mitigation |
|------|-----------|
| Stale AU cache → `auval` validates old bits | Clear cache + kill `AudioComponentRegistrar` BEFORE auval (Task 1 wrapper does this). Order: build→install→validate. |
| Dev/release variant shadowing (project memory) | Sweep BOTH `-dev` and unsuffixed bundles before install; wrapper warns `⚠ ALTERNATE-variant`. |
| pluginval GUI-open flaky/slow for WebView editor | Gate on Tier A only; Tier B best-effort → fold live UI into human listen. |
| Version drift across 2 CMake files | Bump both in Task 6; harness else reports stale version (cosmetic). |
| Writing speculative new test code | D2 forbids unless a gate fails. Re-run existing harness; desk-check presets. |
| Windows scope creep | D3 — out of scope; CMake flags already static-verified. Record "deferred to CI" only. |
