# Stage 3 — GUI · Phase 3.2 (Buffer display + live grains) — Verification

## Verification Date

2026-11-06

Every check below was re-run in this verify session at `7cd3137a`. `git diff f7d3cd01 HEAD` on `plugins/O-Granulator/{Source,tests,CMakeLists.txt}`, `modules` and `scripts` is empty: the only commit since execute T7 is docs. All six contract checksums match the STATUS frontmatter (brief `f9c748a8`, requirements `36eafa9b`, draft `d0b1457e`, param-spec `f5ce3595`, architecture `f0cf753b`, roadmap `1060dae0`). `Source/ui/public/index.html` and `js/i18n.js` are byte-identical to `mockups/v2-ui.html` and `v2-i18n.js`. The harness and param-dump were touch-rebuilt first (0 warnings). No result here is copied from SUMMARY-3.2.md. Logs are in the session scratchpad `v32/`, and one script (`v32/run.sh`) ran every gate in sequence.

**Machine load:** 2.4–6.0, much quieter than 3.1 verify (≈ 20). The harness wall time was 7 min 37 s.

## Goal-Backward Analysis

### Original Goals (CONTEXT-3.2 D30–D33 / PLAN-3.2 success criteria)

1. A live dB-mapped waveform (D31), anchored to the snapshot head and scrolling in whole-bin steps.
2. Freeze: the held span is `min(span, heldCaptured)`, and the wave is bitwise stable.
3. Grain dots: delay in the wave's time domain; 1-based streams; exact pan / gain / pitch / env recovery, incl. stream pan.
4. Bands follow modulated position / spray, and sit at the base values when unrouted (D30).
5. Counts never exceed the limit and match the census (D32).
6. The triple buffer never tears; `readTelemetry` is false only when unprepared; the torn path is removed.
7. 0 allocations from block 0, and audio unchanged.
8. All UI gates green; the page diff is the band line only.
9. HANDS-ON-3.2.md run (UI-02 completes there).

### Goal Achievement

| Goal | Status | Evidence (re-run) |
|------|--------|-------------------|
| 1 Waveform | ✅ Achieved | WV1: 0 misses at 44.1 / 48 / 96 / 192 k, 6 bins lit each. WV1n: binning at the live head misses 6/6 (lag 512). WV2: silence 0, −30 dBFS 0.3750, 0 dBFS 1.0000, −60 dBFS 0. WV4 live arm: 50/50 whole-bin scrolls. WV4n: 0/50 |
| 2 Freeze | ✅ Achieved | WV3: early freeze span 2.0067 s (8 s buffer, held 2.0067 s); late freeze 4.0000 s (held 12.01 s). WV4 frozen arm: 50/50 bitwise identical, 464 bins lit |
| 3 Dots | ✅ Achieved | GD1: 32/32 checked against the spawn log, worst 5.87e-8 s; 8 stream-1 dots. GD1n: 0-based drops all 8. GD2: spread 0 → pan 0 / gain 1 / pitch 7 exact, Hann env worst 2.0e-7; panGains inverse worst 6.2e-8; bank env exact; 8/8 stream-1 dots at pan −1 |
| 4 Bands | ✅ Achieved | BD1: 94 frames, gp mismatches 0; s1 off base in 93 frames; streams 2–4 bitwise equal to the unrouted render |
| 5 Counts | ✅ Achieved | CN1: max sums 32 / 64 / 96 / 128, lowered 128 → 32 holds at 32. CN1n: counting killing slots gives 40 > 32 |
| 6 Transport | ✅ Achieved | TB1: 71.2 M reads, 0 torn, 0 backwards. TB1n: 10 365 torn. TL32u: false before prepare, has* false. Static read: `telemetryToVar` has no `torn` parameter, and the editor always sends the full var |
| 7 Alloc / audio | ✅ Achieved | M8: 0 allocations (var, and fresh 8192 after a 512 prepare, with freeze edges). M8L == 1. Carried diff vs B = **18 added rows, 0 changed or removed** (TL31 → TL32 attributed; TL31 is outside the carried set). N3 / N2 / N1 / N0 `--strict` + R12 rc 0. HA 79/79. Codec bytes 9/9 == B |
| 8 UI gates | ✅ Achieved | All green (table below). Page vs 3.1 = the band line (1 − / 2 +) |
| 9 Hands-on | ⏳ Pending | HANDS-ON-3.2.md: Taylor, Standalone |

## Code Review (static, `a368b4ae..f7d3cd01`)

- **Neutrality.** `Engine::process` gains two blocks:
  - The peak readback, after `CaptureWriter::write` and gated on the head having moved. It uses `copyLevel` into a stack `float[kCell]`.
  - `publishUi()` at the end of `process()`.

  Both blocks read from `pool`, `gp`, `ring`, `fz` and `spawner`, and write only `uiSnap`, `peaks`, `lastPublish` and `publishFirst`. `panNow[s] = pn` in `gridTick` is a store of the already-computed pan; no DSP statement reads it. `CaptureWriter.h` is untouched.
- **Peak cells.** `PeakRing::write` detects a cell's first write as `w % P == 0`, which needs piece starts aligned to 32 in the write-count domain. That holds: pieces are cut on the absolute 32-sample grid (`cellPos = absCount % kCell`), and freeze edges happen only at ticks (`Engine.h:743`), so frozen spans are whole cells. P = 32 at every rate: 41 s × 192 k = 7.87 M samples < 2^18 × 32 = 8.39 M.
- **Triple buffer.** `publish` / `read` each do one `acq_rel` exchange. The reader's `front` stays put until its next `read()`, so the `snap` reference in `readTelemetry` is stable under `telemetryLock`. The audio thread never takes the lock.
- **prepare.** It zero-fills the peaks but does not reset `uiSnap`. At most one stale frame (old head over cleared peaks) can be drawn after a re-prepare, before the first new publish. It is visual only and self-corrects within 1/60 s. NOTE, no action.
- **Page.** `drawBuffer` reads `t.bands[n − 1]` when it is a well-formed array, and otherwise falls back to `scaled(…) / 100`. Both are 0…1 fractions.

## Deviation Review (SUMMARY-3.2 deviations 1–12)

- **1 — `getSpawnerForTest()`** (test hooks only, `OGRAN_TEST_HOOKS`). **Accepted.**
- **2 — CN1 caps 32 / 64 / 96 / 128 + lowered.** They match the real `grain_limit` mapping. **Accepted.**
- **3 — CN1 / BD1 whole 1024-sample blocks.** This is throttle behaviour, not a defect. **Accepted.**
- **4 — Equal-gain pan → exactly 0.** Display-only; GD2 requires it. **Accepted.**
- **5 — WV4 frozen + live arms; WV4n on the live arm.** A frozen-arm control would be vacuous. **Accepted.**
- **6 — PL1 measures `readTelemetry`, not publish.** The C7 row covers the audio side. **Accepted.**
- **7 — Override counts 128 × 4.** This is the widest label case. **Accepted.**
- **8 — No final full harness after the dump change.** Closed by this verify: a fresh full run gives 491/491, and the overrides regenerate byte-identical to the committed file. **Accepted.**
- **9 — T0 N-suites not re-run.** Superseded: they were re-run here, all rc 0. **Accepted.**
- **10 — Extract strips `[PresetManager]` lines.** No verdict involved. **Accepted.**
- **11 — Muted Standalone screenshot.** Live input is covered by the hands-on. **Accepted.**
- **12 — default + 9 states.** Matches 3.1. **Accepted.**

## Requirements Verification

**Stage:** stage-3, Phase 3.2 of 3.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| UI-01: All parameters reachable | must | ✅ Complete (3.1) | Probe 220 / 220 ×2 re-run; no regression |
| UI-02: Buffer waveform, write head, per-stream regions, live grains | should | ⚠️ Partial → complete on hands-on | Every automated layer passes; the real-WebView look is pending |
| UI-03: Mod matrix view + mod amount on targets | should | ⏸️ Deferred | Phase 3.3 |

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Entry (checksums, tree, page cmp) | ✅ Pass | See the header |
| Harness build (touch rebuild) | ✅ Pass | 0 warnings |
| Render harness (default) | ✅ Pass | **491 PASS, 0 FAIL**, rc 0 |
| Full extract vs execute T4 | ✅ Pass | Only TB1 / TB1n read and torn tallies differ (a threaded stress, not deterministic) |
| Carried extract vs B | ✅ Pass | +18 new rows (TB1/n, WV1–4 + 1n/4n, GD1/1n/2, CN1/1n, BD1, M8/M8L, TL32/TL32u); 0 rows changed or removed. W2o **+0.483 dB**, unchanged |
| C7 CPU | ✅ Pass | median **17.32 %** (17.26–17.39, spread 0.13, 0 contended); execute 17.02 % |
| INFO FB1 (R124) | ℹ️ | Frozen band [0.300, 0.500] vs spawns [0.382, 0.577]: offset +0.08 (accepted at plan) |
| INFO PL1 | ℹ️ | 16.0 KB @ 48 k / 192 k (< 32 KB, no quantisation); `readTelemetry` 8 / 13 µs |
| N3 / N2 / N1 / N0 `--strict`, R12 | ✅ Pass | rc 0 ×5 |
| param-dump | ✅ Pass | == `.planning/params.tsv` |
| `generic-overrides.json` | ✅ Pass | 59 / 140 / 199; regenerated ×2 byte-identical; == committed |
| `build-and-install.sh O-Granulator` | ✅ Pass | rc 0; up to date with the T6 build of the identical tree (the shared code at 20:51); `-dev` VST3 + AU reinstalled, caches cleared. The Standalone was **relinked in verify** (it lagged the shared code) |
| auval `aufx OuGn OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED; Tail Time |
| pluginval 10 VST3 / AU (in-process) | ✅ Pass | SUCCESS ×2 |
| HA host-cache (installed VST3) | ✅ Pass | HA1 79/79, HA2 142 untouched, HA3 79/79 |
| Session-codec bytes | ✅ Pass | `diff -r` vs `codec-B` clean (9 fixtures) |
| `ui_frontend_check.js` + NCs | ✅ Pass | ALL PASS; NC1 exit 3, NC2 exit 2 |
| `ui_binding_probe.js` + NC | ✅ Pass | 220 / 220 ×2; NC 219 / 220, rc 2 |
| check-i18n · fr · zh | ✅ Pass | 0 · 0 · 0 |
| check-ui-labels | ✅ Pass | rc 0; `seed=param-dump (220 rows) + overrides`; `states: default + 9` |
| boot-all-uis `--strict-tips` | ✅ Pass | 0 warn, 0 failed, 0 DEAD, 0 late |
| Working tree after gates | ✅ Pass | `git status plugins/O-Granulator` clean |

## Human Verification — HANDS-ON-3.2.md

**Pending:** Taylor, Standalone (`build/plugins/O-Granulator/OuariconGranulator_artefacts/Release/Standalone/O-Granulator-dev.app`, relinked 21:13), with live input.

| Section | Verdict |
|---|---|
| A Waveform (A1–A4) | ⏳ |
| B Freeze (B1–B3) | ⏳ |
| C Grain dots (C1–C6) | ⏳ |
| D Bands (D1–D3) | ⏳ |
| E Counts (E1–E2) | ⏳ |
| F Editor lifecycle (F1–F3) | ⏳ |
| G Watch item (G1, INFO) | ⏳ |

## Issues Found

- **None blocking** in the automated set.
- **NOTE:** one possible stale frame after a re-prepare (`uiSnap` is not reset in `prepare`). Visual only, ≤ 1/60 s. No action.
- **Process note:** the Standalone binary lagged the shared code until this verify relinked it. `build-and-install.sh` builds VST3 + AU only. Re-run the Standalone target after an install before any hands-on.
- **Carried:** 3.3 (meters, LFO phase, onset, loop gain, mod-matrix view + rings, source meters; s2–s4-enabled i18n state from 3.1 deviation 10; DAW pass D27). Stage 4: Env 1e30 pin. Harness: C7 calibration, Z6a. W2o rule.

## Phase Verdict

**Status:** ⚠️ AUTOMATED VERIFIED — hands-on pending. It becomes ✅ VERIFIED (Phase 3.2) when HANDS-ON-3.2 A–F pass; G is INFO only.

**Ready for next phase:** after the hands-on, Phase 3.3 (meters, LFO phase, onset, loop gain, mod matrix view, UI-03).

**Blockers:** none in the automated set.
