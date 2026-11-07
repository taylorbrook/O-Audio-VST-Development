# Stage 3 — GUI · Phase 3.3 (Mod matrix view + mod rings; meters, LFO phase, onset, loop gain) — Verification

## Verification Date

2026-11-07

Every automated check below was re-run in this verify session at `fbeb5878`. `git diff d3f14f7f HEAD` on `plugins/O-Granulator/{Source,tests,CMakeLists.txt}`, `modules` and `scripts` is empty: the only commit since execute T7 is docs. All six contract checksums match the STATUS frontmatter (brief `f9c748a8`, requirements `d83ee152`, draft `d0b1457e`, param-spec `f5ce3595`, architecture `f0cf753b`, roadmap `1060dae0`). `Source/ui/public/index.html` and `js/i18n.js` are byte-identical to `mockups/v2-ui.html` and `v2-i18n.js`, and the page is unchanged vs 3.2 (`git diff 00a04a89 HEAD` empty). The harness and param-dump were touch-rebuilt first (0 warnings). No result here is copied from SUMMARY-3.3.md. One script (session scratchpad `v33/run.sh`) ran every gate in sequence; logs sit beside it.

**Machine load:** 2.2–4.6. The harness wall time was 7 min 38 s.

## Goal-Backward Analysis

### Original Goals (CONTEXT-3.3 D34–D41 / PLAN-3.3 success criteria)

1. D36: route edits from every page write order are click-free on the 2.6 matrix (UX1, on the unmodified engine).
2. Meters (D38): in = post input gain, pre-mix; out = final output; max-since-read, destructive reads, 250 ms gap rule (R128–R130).
3. Onset pills (D40 amended): an audio-side counter plus a 200 ms reader-clock pulse (R131).
4. LFO dot (D39): offset-free phase that sits on the drawn curve (R132).
5. Loop gain (D34): `20·log10 (k · g_agc)`, absent at k == 0 or frozen (R133).
6. 0 allocations; audio unchanged (no carried row moves); CPU unchanged.
7. All UI gates green, with the `streams-all-enabled` i18n state.
8. HANDS-ON-3.3.md + DAW-3.3.md (UI-03 completes and Stage 3 closes there).

### Goal Achievement

| Goal | Status | Evidence (re-run) |
|------|--------|-------------------|
| 1 D36 / UX1 | ✅ Achieved | UX1 rows 8/8 each, cross-talk 0, \|source\| at the edit LFO1 1.000, Onset 0.664; UX1nR / UX1nD fire. The extract is identical to execute T5 |
| 2 Meters | ✅ Achieved | MT1 0.501187 L/R, ×1.99526, mono R == L. MT2 / MT3 / MT5 PASS. MT2n and MT4 / MT4n PASS (values vary by run, see INFO) |
| 3 Onset | ✅ Achieved | ON1: 90 reads, 12 event-bearing, 12 fired, 0 mismatches, pulse \|err\| 2.1e-8. ON2 idle decay PASS; ON2n fires |
| 4 LFO phase | ✅ Achieved | LP1 / LP2 PASS (identical to T5) |
| 5 Loop gain | ✅ Achieved | LG1: −6.4661 dB (want −6.4661); hot arm −3.03 dB (< −0.4455). LG2: absent at fb 0 / frozen, present while pending / after unfreeze. LG3 / LG3n PASS |
| 6 Alloc / audio / CPU | ✅ Achieved | M9: 0 allocations (var 222 reads; 8192 after 512: 36 reads), M9L 1. **Carried rows identical to execute** (diff-vs-C == T5's diff-vs-C outside the 3.3-only rows). N3 / N2 / N1 / N0 `--strict` + R12 rc 0. HA 79/79. Codec 9/9 == B. C7 17.26 % (spread 0.10), C8 17.32 % (Δ vs C +0.13, noise) |
| 7 UI gates | ✅ Achieved | All green (table below) |
| 8 Hands-on + DAW | ✅ Achieved | HANDS-ON-3.3.md A–G all PASS (Standalone) and DAW-3.3.md all PASS (Logic AU), Taylor, 2026-11-07 |

## Code Review (static, `3043b72a..d3f14f7f`, Source only: 5 files, +143 / −1)

- **Neutrality.** Every new engine write goes to a telemetry member: `inPk`, `onsetCount`, `lfoPhaseTel`, `loopKTel`, `loopAgcTel` and `frozenTel`. No DSP statement reads them. The input-peak loop only reads `inL` / `inR` / `inGain`, and it reads them before `mixOut` overwrites the aliased buffer. A mono input aliases `inR = inL` (`Engine.cpp:421`).
- **NaN safety.** In `std::max (inPkBlock, x)` and in `pushMax`'s `v > cur`, a NaN is a no-op. The reader also applies `finite0`.
- **Out meter.** The `findMinAndMax` scan over `[0, numOut)` runs after `engine.process`. Mono out → R = L. Bypass never reaches it.
- **Loop gain.** `loopKTel` is stored on every grid tick, in the same statement that commits `fbKnobNow`. `prepare` seeds it from the snapped `fbKnobNow`.
- **Reader state.** `lastReadMs`, `pulseStartMs` and `lastOnsetCount` are touched only under `telemetryLock`, and the audio thread never takes that lock. The stale path consumes everything and fires no onset. `pulseStartMs ≤` the previous read, so a stale read can never leave a live pulse behind.
- **onsetCount not reset in `prepare`** (deviation 4). Correct: a reset would make the inequality detector fire a phantom onset.
- `static_assert` that the three atomic types are lock-free. ✅

## Deviation Review (SUMMARY-3.3 deviations 1–10)

- **1 — extract33.sh** (PresetManager lines stripped; TB1 / TB1n wall-clock counts masked). Verdicts are untouched. **Accepted.**
- **2 — LP formula (N − 1 cells).** This corrects the harness expectation, and the value ↔ phase identity is checked independently. **Accepted.**
- **3 — LG1 hot arm at saturation 0.** At the default saturation the predicate cannot be reached (see Findings). **Accepted.**
- **4 — onsetCount monotonic across prepare.** Confirmed in the static review. **Accepted.**
- **5 — LG3n reads the APVTS knob.** This is the settled ramp value. **Accepted.**
- **6 — ON1 10 ms bursts.** The detector needs energy. **Accepted.**
- **7 — UX1 at block 32.** The engine is block-size invariant (N9 / N10 unchanged). **Accepted.**
- **8 — Muted Standalone screenshot.** The FX / MOD tabs and live meters are covered by HANDS-ON A, B and D. **Accepted.**
- **9 — C8 vs C's C7.** **Accepted.**
- **10 — M8 unchanged; M9 is the injected-clock extension.** **Accepted.**

## Requirements Verification

**Stage:** stage-3, Phase 3.3 of 3.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| UI-01: All parameters reachable | must | ✅ Complete (3.1) | Probe 220 / 220 ×2 re-run; no regression |
| UI-02: Buffer waveform, write head, regions, live grains | should | ✅ Complete (3.2) | No regression (TB1, WV, GD, CN, BD rows unchanged) |
| UI-03: Mod matrix view + mod amount on targets | should | ✅ Complete (partial → complete) | Every automated layer passes; HANDS-ON-3.3 E / F + DAW-3.3 PASS |

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Entry (checksums, tree, page cmp, page vs 3.2) | ✅ Pass | See the header |
| Harness build (touch rebuild) | ✅ Pass | 0 warnings |
| Render harness (default) | ✅ Pass | **520 PASS, 0 FAIL**, rc 0 |
| Full extract vs execute T5 | ✅ Pass | Only 5 rows differ in measured values, all still PASS: LG1 hot arm (−3.03 vs −2.99 dB), MT2n fb-80 ring peak, MT4 / MT4n frozen-tail out, and TL33 out. See INFO |
| Carried extract vs C | ✅ Pass | Identical to execute's diff-vs-C: TL32 / TL32u → TL33 / TL33u + new rows. No carried row moved. W2o unchanged |
| C7 CPU | ✅ Pass | median **17.26 %** (17.17–17.27, spread 0.10, 0 contended) |
| INFO C8 | ℹ️ | 17.32 %, Δ vs C +0.13 (noise) |
| N3 / N2 / N1 / N0 `--strict`, R12 | ✅ Pass | rc 0 ×5 |
| param-dump | ✅ Pass | == `.planning/params.tsv` |
| `generic-overrides.json` | ✅ Pass | regenerated ×2 byte-identical; == committed |
| `build-and-install.sh O-Granulator` | ✅ Pass | rc 0, 0 warnings; `-dev` VST3 + AU reinstalled, caches cleared; **Standalone rebuilt after the install** (3.2 process note) |
| auval `aufx OuGn OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| pluginval 10 VST3 / AU (in-process) | ✅ Pass | SUCCESS ×2 |
| HA host-cache (installed VST3) | ✅ Pass | HA1 79/79, HA2 142 untouched, HA3 79/79 |
| Session-codec bytes | ✅ Pass | `diff -r` vs `codec-B` clean (9 fixtures) |
| `ui_frontend_check.js` + NCs | ✅ Pass | ALL PASS; the NCs fail as they must |
| `ui_binding_probe.js` ×2 + NC | ✅ Pass | 220 / 220 ×2; NC 219 / 220, rc 2 |
| check-i18n · fr · zh | ✅ Pass | rc 0 ×3 |
| check-ui-labels | ✅ Pass | ALL CHECKS PASSED (default + 10 states) |
| boot-all-uis `--strict-tips` | ✅ Pass | rc 0 |
| Working tree after gates | ✅ Pass | `git status plugins/O-Granulator` clean |

**INFO — run-to-run variance in 5 new rows.** The processor's grain seed comes from `Random::getSystemRandom()` at construction (`PluginProcessor.cpp:36`). The processor-level 3.3 rows that read wet-grain levels therefore vary between runs: MT2n, MT4 / MT4n, the TL33 out value and the LG1 hot arm. Every one of those verdicts is a threshold (rises > 5 %, inPeak == 0 with out > 0.01, < −0.4455 dB, live), and every run clears it with wide margin. The engine-level exact rows (MT1, ON1, LP1 / LP2, the LG1 quiet arm, UX1) are bitwise identical to execute. No action is needed. If a byte-stable extract is wanted later, seed these processors explicitly.

## Human Verification

### HANDS-ON-3.3.md (Standalone, real WebView)

Run by Taylor, 2026-11-07, in the Standalone (`O-Granulator-dev.app`, rebuilt in this verify after the install), with live input: **all items PASS.** The recorded notes (B6, D3, G2) raised no issue.

| Section | Verdict |
|---|---|
| A Meters (A1–A5) | ✅ PASS |
| B Loop gain (B1–B5; B6 note) | ✅ PASS |
| C Onset pills (C1–C3) | ✅ PASS |
| D LFO dot (D1, D2, D4; D3 note) | ✅ PASS |
| E Mod matrix + rings (E1–E6) | ✅ PASS |
| F Route edits by ear (F1–F4) | ✅ PASS |
| G Lifecycle / two windows (G1; G2 note) | ✅ PASS |

### DAW-3.3.md (Logic Pro AU)

Run by Taylor, 2026-11-07, Logic Pro AU: **all items PASS.** DAW-2.6 item 1 PASS → the ∞ tail stands (D21 fallback (b) not needed). FUNC-04 / QUAL-03 / FUNC-08 / DSP-08 stay complete. The recorded items (DAW-2.6 item 2 R85, DAW-2.5a item 8 count-in) raised no issue.

| Part | Verdict |
|---|---|
| DAW-2.5a 1–8 | ✅ PASS |
| DAW-2.5b 1–5 | ✅ PASS |
| DAW-2.6 1–3 | ✅ PASS |
| 3.3 items 1–4 | ✅ PASS |

## Issues Found

- **None blocking** in the automated set.
- **Decision for Taylor (SUMMARY-3.3 Findings):** the AGC cannot engage at the default `fb_saturation` (20 %). The saturator caps the AGC input at 1 / 2.4 ≈ 0.42, which is below the 0.7 ceiling, and any setting above ≈ 6.1 % makes g_agc ≡ 1. At defaults, the loop-gain readout is therefore the knob value alone. Options were (a) accept and record, (b) add a page note, or (c) a Stage 4 look at the saturator / AGC staging. **Carried to Stage 4 as (c)**, the recommended default; Taylor named no option at verify.
- **Carried to Stage 4:** the AGC / saturator staging (above); the Env 1e30 pin; C7 calibration; Z6a; the W2o rule; `uiSnap` not reset in `prepare`; a VST3-host DAW pass if COMPAT-03 calls for it.

## Phase Verdict

**Status:** ✅ VERIFIED (Phase 3.3, and Stage 3 closes). Every automated gate passes on an independent re-run, deviations 1–10 are accepted, HANDS-ON-3.3 A–G pass, and DAW-3.3 passes (PLAN-3.3 P142).

**Ready for next stage:** Yes. Stage 4 (Polish / Validation).

**Blockers:** none.
