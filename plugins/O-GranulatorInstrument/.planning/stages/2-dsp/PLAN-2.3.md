---
title: "O-GranulatorInstrument Stage 2 (DSP) — Phase 2.3 Execution Plan"
created: 2026-11-08
juce_version: "8.0.15"
phase: "2.3"
summary: "Seven waves: (W0) pre-flight with baselines (harness 117/0, renders-2.2 re-dump, fresh parent build + parity 41/0 + PN residual table), then the calibration-loop reference frozen and C22 re-measured on the UNMODIFIED 2.2 engine; (W1) parity side: PF / PFc / PS5–7 rows with per-row material peak and level, parent FX log columns, rendered out of tree from git archive 82406fce; (W2) engine written header-first: ControlGrid FX rows + manifest re-hash, GiFxLatch.h (parent planFx / assignFxLatch re-derived, svfMake latched) wired into spawn, gather → FX → scatter kernel with the NaN kill before a single-statement scatter; (W3) harness gates (FU port, FV census + audio, FB no-bleed, FZ stress / NaN, FA3, FN block / unread, FR1 goldens), ONE console build, parity null with the PF0 precondition and the gain band; (W4) --cpu23 verdicts D19 ≤ 36 % / D20 ≤ 12 % with the calibration witness, golden-safe levers only on FAIL; (W5) install, explicit Standalone rebuild, auval, pluginval 10, X2, HANDS-ON-2.3 (deferred); (W6) SUMMARY, STATUS, path-scoped commit. No tag; version 0.1.0; CMakeLists.txt not edited; GrainFx.h byte-identical."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-GranulatorInstrument — Stage 2 / Phase 2.3 Execution Plan (Per-grain FX + CPU budget)

## Goal

DSP-03 and the PERF-02 CPU verdict at 16 voices × 4 streams. The already-forked `Source/dsp/forked/GrainFx.h` (TPT SVF with the D10 make-down, drive, crush, ring) is latched at spawn and run per sample in the ARCHITECTURE §13 order **read → SVF → drive → crush → ring → window·gain·pan**, with latched `fxActive` stage flags, the SVF NaN kill before the scatter, and FX state reset on pool-slot reuse by value-initialisation. FX at their defaults are a true bypass: every renders-2.2 hash stays byte-identical. DSP-03 is proven against O-Granulator at `82406fce` (null ≤ −90 dB after gain alignment with a fitted-gain band, ported law rows, a statistical arm, no-bleed rows). D19 is a PASS/FAIL ≤ 36 % worst case and D20 a PASS/FAIL ≤ 12 % frozen typical proxy, both behind a duty witness and a machine-speed calibration witness.

**Out of scope** (CONTEXT-2.3 § Out of scope): feedback chains, Freeze, Reset, reverb and the final 40 % worst-case verdict (2.4). Modulation of the FX destinations b+17…b+25 (2.5). The factory-preset typical verdict (Stage 4). FX UI (Stage 3). Planner exclusions: the parent K2 cutoff-step click row (PD3); lever 1 (Hermite) without the user's consent (PD9).

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/CONTEXT-2.3.md` | D18–D21, the Defaults block, the gate list. **Binding, not reopened.** D11–D17 (CONTEXT-2.1 / 2.2) still bind. |
| `stages/2-dsp/RESEARCH-2.3.md` | §1–§9 (cited **R§x**), Pitfalls **P1–P14**. Authoritative over ARCHITECTURE where they differ; this plan's PD5 / PD10 / PD14 correct three research details (see PD text). |
| `stages/2-dsp/{SUMMARY,VERIFICATION}-2.2.md`, `PLAN-2.2.md` | What exists: harness 117/0, `--parity` 41/0 + GV 2/0, hook names, row IDs, the parity plumbing. **Use VERIFICATION-2.2's HEAD PN residuals (−110.9 … −120.2 dB, fitted gain ≈ 0.998); SUMMARY-2.2's −123…−132 dB / 1.995 are pre-T12 and stale.** |
| `research/ARCHITECTURE.md` | §13 render order, §14 per-grain FX, Implementation Risks → CPU (budget, levers, calibration witness), Stage-2 Gate Catalogue. |
| `parameter-spec.md` (LOCKED) + `.planning/params.tsv` | FX rows `sN_filter_type/cutoff/rand/res`, `sN_drive`, `sN_crush`, `sN_crush_prob`, `sN_ring_freq/rand/mix` (params.tsv rows 50–59 for s1; identical ranges, skews and defaults to the parent, R Summary 3). **Not edited.** |
| Parent at the pin: `git show "${PIN}:plugins/O-Granulator/<path>"`, `PIN=82406fce` (brace every variable) | `Source/dsp/Spawner.h:330-415` (`planFx`, `assignFxLatch`), `Source/dsp/Engine.cpp:1422-1578` (gather / FX / scatter kernel, P25 NaN kill), `Source/dsp/Engine.h:218-242` (hook shapes `fxSharedLatch`, `fxNoReset`, `snapshotSteals`, `injectNan`, `forceRetire`), `:350-356` (`SpawnRecord` FX fields), `:385-389` (diag counters), `tests/render-harness/engine_gates_23.h:72-562` (U rows + helpers), `:600-1001` (V1a–d, V2–V4, H1–H3), `.planning/stages/2-dsp/SUMMARY-2.3.md:90-102` (C3w / C3t), `SUMMARY-2.6.md:82` (slowed-core finding). **Never read the parent working tree.** |
| This plugin | `Source/dsp/{Engine.h,Engine.cpp,GiRenderer.h,GiSpawner.h,VoiceGrain.h,GiConstants.h}`, `Source/dsp/forked/{GrainFx.h,Grain.h,EngineConstants.h,ControlGrid.h,FORK-MANIFEST.md}`, `tests/parity/{run.sh,parent_render.cpp,ParityRows.h}`, `tests/render-harness/{main.cpp,engine_gates_21.h,engine_gates_22.h,parity_gates_22.h,render_hashes.h}`, `tests/golden/renders-2.{1,2}.sha256`, `tests/fork-drift-check.sh`; read only: `modules/synthesis/grain-engine/cpp/GrainPool.h:236-241` (`activate` = `Grain {}`). |

---

## Plan-phase decisions (resolve RESEARCH open items)

- **PD1 [research §7, frozen here] D20 typical proxy C23t.** 6-note chord (48, 52, 55, 59, 62, 64), velocity 100, held from sample 0; s1 + s2 enabled (slots 1, 2 loaded with `loadNoise4 (…, 0.1f)` as C22), s3 / s4 off; s1 and s2 at **density 40 g/s, size 150 ms**; spray / jitter / spread at their defaults (10 / 20 / 30 %); `grain_limit` default (256); polyphony default (8); **s1 filter LP (type 1) cutoff 1200 Hz res 40 % + drive 30 %**, every other FX parameter at its default; s2 FX off; 48 k / 128; the C22 method (5 s warm-up + 7 × 10 s, median) plus the PD2 witness. **Assert mean live ∈ [60, 90]** (projection ≈ 72; P10) else the row is INVALID = FAIL. Verdict ≤ **12 %**. It stays a regression row; Stage 4 re-checks real presets.
- **PD2 [planner] Machine-speed witness (P9, R§6.2, ARCHITECTURE CPU risk).** `tests/render-harness/cpu_calib.h`: `calibOnceNs()` = 5·10⁷ iterations of 4 independent `double` chains `x_i = std::fma (x_i, 0.999999999, 1.0e-9)` (2·10⁸ FMAs; seeds 1, 2, 3, 4; the sum stored to a `volatile double` sink), timed with `CLOCK_THREAD_CPUTIME_ID`; `calibNs()` = median of 5 calls. **Frozen once, in T2,** on the unmodified 2.2 tree: three `--calib` processes run alone, accepted when max/min ≤ 1.05; `ref_ns` = median of the three, written to `tests/golden/cpu-calib-2.3.txt` (`ref_ns`, the 3 samples, date, `sysctl -n machdep.cpu.brand_string`, harness git rev) and copied into SUMMARY-2.3. Every timed config in `--cpu23` is bracketed by `calibNs()` before / after: `[NOVERDICT — slowed core]` if before > 1.15 · ref_ns or after / before > 1.10; `[NOVERDICT — contended]` if duty < 0.9. **NOVERDICT counts as FAIL** (D19). `cpu_calib.h` is not edited after T2 (the reference would be meaningless); 2.4 reads the same file.
- **PD3 [planner, R§5.3 open item] Audio census rows IN.** Parent V2 / V3 → **FV2 / FV3** (the only detectors for "latch right, renderer reads something else"), plus V4 → **FV4** (crush per-grain split: PS7 proves only the latched Bernoulli, and the PFC rows only prob 0 / 100). All three use the parent `fxSharedLatch` shape (re-plan with filter_rand = ring_rand = 0 and crush_prob = 1, assign only the FX latch; the log keeps the real draws). The parent K2 cutoff-step click row (R§9 FK2, optional) is **OUT**: the latch is spawn-only, so a cutoff move never touches a sounding grain; QUAL-01's 2.3 arm is FZ1 / FZ2 (CONTEXT criteria) plus HANDS-ON-2.3.
- **PD4 [research P6, adopted] Row IDs and targets.** `FU` (ported units U*), `FV` (census / audio V*), `FB` (no-bleed H*), `FZ` (stress / NaN), `FA3` (allocations), `FN` (block-size + unread), `FR1` (FX-off goldens), `PF` / `PFc` (parity null + controls), `PS5–PS7` (stats), `C22r` / `C23w` / `C23t` / `C23s` / `C23w96` (CPU). The only existing F-prefixed IDs are FS1 / FS2 (`engine_gates_22.h:948, :976`); `FS` is not reused. **Targets:** default harness **117 → 150** (FU 15 + FV 7 + FB 4 + FZ 2 + FA 1 + FN 3 + FR 1; FU1i / FU2i / FU3i are INFO lines, not rows); `--parity` **41 → 99** (PF0 + 48 PF + PFc 6 + PS 3), `--rows GV` 2/0 unchanged; `--cpu23` 2 verdict rows + 3 INFO lines. RESEARCH estimated ~151 / ~102; SUMMARY records the exact counts.
- **PD5 [research §2.2 adopted + planner correction] Material, level, wet safety (P2).** PF rows only (PN / PS1–4 / GV keep the 2.2 setup): our side sets the test hook `wetSafetyOff` (`Engine.cpp:1398-1399` → identity). Per-row `matPeak`: SVF rows **0.1**; PF0, drive, crush, ring and chained rows **0.25**; **PFC8 (crush 100 % / prob 100 %) 0.4 with stream `level` −6 dB on both sides.** Correction: at 0.25 peak crush 100 % is silent on both sides (scale = 2^(15 − 14) = 2, `GrainFx.h:132-133`; `nearbyint (2x)` = 0 for |x| ≤ 0.25 under ties-to-even, `:140`; `hashNoise` peak < amp, `ParityRows.h:63-72`), so R§2.3's crush-100 row would be dead (ref < −70 dBFS). At 0.4 about 37 % of samples quantise to ±0.5, and −6 dB keeps the parent below 0.45. Every PF row asserts **max |parent out| ≤ 0.45** (the parent knee is identity ≤ 0.501), else **INVALID = FAIL**. One stimulus fix is allowed per INVALID row: stream `level` −6 dB on both sides (post-FX, so the FX operating point is unchanged; the band ratio is unaffected because both sides apply the same `levelDbToGain`), recorded as a deviation. Still INVALID → STOP. Never lower a threshold.
- **PD6 [research §2.4–§2.5, adopted] PF0 precondition and gain band (P1, P3).** PF0 (FX off, `wetSafetyOff`, 0.25) is evaluated first: residual **≤ −130 dB**, and its fitted gain is **g₀**. If PF0 > −130 dB every PFC* / PFX* row reports `BLOCKED (PF0)` = FAIL and **S1** fires. Every non-control PF row also requires **|g_row / g₀ − 1| ≤ 1e-4**; a miss → **S4**. Make-down control **PFc6** = PFL6 + hook `legacySvfMake` (`svfMake := 1`): it fires only if it **passes** the −90 dB null **and fails** the band, which proves the alignment alone cannot see a latched scalar.
- **PD7 [research §2.3 / Summary 5, adopted; refines D18's row list] Clamp coverage (P4).** At 48 k, 0.45·fs = 21 600 Hz lies above both maxima (cutoff 20 kHz, ring 5 kHz), so no derandomised 48 k null row reaches a clamp. The clamp is gated by FU1a (fc = 0.45·fs at 44.1 / 48 / 96 / 192 k), FU1c, FU4a, and FV1a / FV1b at **44.1 k** (draws ≥ 19 845 Hz log exactly `float (0.45·fs)`). The clamp code sits inside the byte-identical `svfLatch` / `ringLatch` (`GrainFx.h:65`, `:152`), so our only exposure is the `fs` passed in, which FV1a / b check. SUMMARY records the refinement.
- **PD8 [research §1.2, adopted] One kernel, unchanged statements (P5, P12).** The existing loop body becomes the gather (`L[n] = l; R[n] = r; GK[n] = w * g.killGain * fader.next();` — the `GiRenderer.h:187` statement verbatim, at the same position); then the FX block with `fxa = g.fxActive` hoisted per segment; then the scatter `bl[c0 + j] += L[j] * GK[j] * g.gainL;` / `br[…] += R[j] * GK[j] * g.gainR;` (same single statements and operand order as `:188-189`). NaN kill after `fx::svf`, before the scatter. **Any renders-2.2 line moving → S3.** The P18 dual kernel is offered there, never applied automatically; nothing is ever re-baselined.
- **PD9 [planner: D21 × CONTEXT Defaults × P13] Lever ladder.** Lever 1 (Hermite, rate ≤ 1 at level 0) necessarily changes every rate ≤ 1 / level-0 render — renders-2.2 and the PN nulls — which the Defaults lock byte-identical, so it is **never auto-applied**. The automatic ladder keeps D21's relative order over the golden-safe levers: **lever 2** (4-lane SIMD over same-(k,s) grains), kept only if renders-2.2 and every PN / PF residual are unchanged (a hash move → S3), then **lever 3** (verify the hoisted `fxActive` skip; tighten with the P18 dual kernel for `fxa == 0`, byte-identical by construction), re-measuring `--cpu23` after each. Still FAIL → **S2**: lever 1 (with its R§8 timbre gate and a re-pin) and the `grain_limit` 256 → 128 fallback both ask.
- **PD10 [planner; extends research P7] Unread-parameter rows.** Two carried rows randomise every ID outside `readSet22()` and assert 113 bound / 148 touched: eg21 **N2** (`engine_gates_21.h:2056-2088`) as well as eg22 **N2s** (`engine_gates_22.h:1440-1474`); RESEARCH names only N2s. Binding the FX rows makes both fail. The 2.2 precedent (SUMMARY-2.2 deviation 5) moves the read set forward and keeps the 0-differ threshold. So: add `readSet23()` (= `readSet22()` + the 10 FX suffixes × 4 = **153**; unread **108**) to `engine_gates_21.h`; switch both rows to it with literals 153 / 108 (row IDs, stimuli and thresholds unchanged, title text updated); add **FN3** (4 streams, FX on, FX-ID controls).
- **PD11 [planner] Parity plumbing.** New `Kind::fxNull` / `Kind::fxStats` so `pg22::runNulls` / `runStats` (they filter on `null_` / `stats`, `parity_gates_22.h:220`, `:339`) never see PF / PS5–7; `isNullKind` / `isStatsKind` helpers replace the kind tests in `parent_render.cpp` and around the solver; `material (n, ch, peak = kMatPeak)`; FX columns are appended at the **end** of each `grains.tsv` / `manifest.tsv` line (the 2.2 readers take lower bounds, `:73` `< 15`, `:100` `< 14`). `parity_gates_22.h` is not edited; `pg23` (new `parity_gates_23.h`) reuses `pg22` helpers in the same TU. The 2.2 rows' parent `.f32` files stay byte-identical (T3 `cmp`).
- **PD12 [planner; parent `Engine.h:218-242` shapes] No-bleed and NaN hooks** (all under `OGI_TEST_HOOKS`; the shipping build compiles none):
  - `fxShadow[288]` (FX **state** only) written at every retire and by `poisonFreeSlots`;
  - `poisonFreeSlots`: at each `gridTick`, every inactive slot's FX state := poison (`svfIc1/2 = 0.7`, `crushHeld = 0.4`, `crushHoldPhase = 0.3`, `ringRe/Im = (0, 1)`), written through `const_cast<VoiceGrain*> (pool.getGrains().data())` (the pool's array is non-const storage; `modules/` is not edited);
  - `fxNoReset`: after `assignFxLatch`, copy `fxShadow[slot]`'s FX state into the new grain;
  - `snapshotSteals` + `stealSnapPerturb` (1-ulp flip of the snapshot's `svfIc1[0]`); counters `nanKills`, `fxStateNonDefaultAtSpawn`, `stealStateChecked`, `stealStateMismatches`;
  - `injectNan {slot, atSample, field, value, done}`, `forceRetire {slot, atSample, done}`, `nanKillOff`, `fxOrderSwap` (crush before drive), `legacySvfMake`, `fxSharedLatch`, `wetSafetyOff`.
- **PD13 [planner] CPU mode.** `--cpu23` (in `engine_gates_23.h`), run alone, its own exit code (= FAIL count), never part of the default harness rc (2.2 precedent). One process, in order: bypass (streams disabled) → **C22r** (the C22 recipe, FX off: the same-session reference, D19) → **C23w** → **C23t** → **C23w96** (INFO). Each timed config is bracketed by `calibNs()`. It prints Δ = C23w − C22r and the per-grain FX cost Δ / live (parent reference 0.064 %/grain). **C23s** (INFO): C22r vs T2's C22@2.2, each scaled by `ref_ns / calibBefore`; flag > +0.6 points (restructure cost; lever-3 / P18 candidate, R§8).
- **PD14 [research §3.1, adopted + completed] FU port.** Copy `82406fce:plugins/O-Granulator/tests/render-harness/engine_gates_23.h:72-562` into `fx_units_23.h` with the fork rename (`ogran` → `ogi`). R§3.1 cites `:140-562`, but the U rows call the helpers at `:74-137` (`dB20`, `sineAmp`, `svfGrain`, `svfGain`, `oracle`), so those come too. IDs `U*` → `FU*`; `eg::hashNoise` → `parity::hashNoise`; `eg::rmsDb` → a local helper; criteria verbatim; FU3a under its own `juce::ScopedNoDenormals`. The FU1i INFO text carries pre-D10 expectations (`:203`, +20.01 dB at res 100); with the make-down the LP / HP peak reads ≈ +10 dB, so the INFO line prints both, with no gate. The oracle rows keep the parent's control rows (FU3n, FU4n). FU1b adds one inline control (the oracle without × make → worst > 0.05 dB), the unit-level P1 detector.
- **PD15 Housekeeping.** Version stays **0.1.0** (I/O unchanged; the Logic AU I/O cache is unaffected); **no tag**; `CMakeLists.txt` **not edited** (`GiFxLatch.h` and every test file are headers; the parity build is out of tree). No new parameters or `<Engine>` keys; param-dump byte-identical. `GrainFx.h`, `Grain.h`, `VoiceGrain.h`, `EngineConstants.h`, `render_hashes.h` and `parity_gates_22.h` are not edited. HANDS-ON-2.3 is written but deferred post-Stage-3 with 2.1 / 2.2 (user override 2026-11-08, STATUS). The Standalone is rebuilt explicitly with `OuariconGranulatorInstrument_Standalone` (the exact phony target in `build/build.ninja`, verified at plan time) before any Standalone check (P14).

---

## Tasks

**Roles.**
- **[agent]** = dsp-agent writes files (no Bash).
- **[run]** = build/gate runner: commands, builds, validators, commit.
- **BG** = `run_in_background`, log to `$S/<name>.log`. `$S` = the executing session's scratchpad.
- `$H` = `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness`.
- Paths are relative to `plugins/O-GranulatorInstrument/` unless they start with `build/`, `scripts/`, `modules/` or `~`.

### Wave 0 — pre-flight, baselines, calibration reference

**T1. Pre-flight + baselines** [run]
- Files: none. Output to `$S/preflight.txt`.
- Depends on: none.
- Done when (any mismatch → STOP and ask):
  - **Late changes:** `git log --oneline e90ce25a..HEAD -- plugins/O-GranulatorInstrument/Source plugins/O-GranulatorInstrument/tests plugins/O-GranulatorInstrument/CMakeLists.txt` is empty (empty at plan time). If not, STOP for a re-check of the mapped tasks: `Engine.{h,cpp}` → T5, T6 · `GiRenderer.h` → T6 · `forked/ControlGrid.h`, `FORK-MANIFEST.md` → T4 · `tests/parity/*` → T3 · `tests/render-harness/*` → T2, T7.
  - **Git state:** `git branch --show-current` = `main`; record `git status --short`. Foreign paths are **never staged**: `modules/registry.yaml`, `.claude/agent-memory/*`, `.claude/compaction-snapshot.md`, `plugins/O-GrainScatter/.planning/improvements/adaptive-grain-length/nielsen-dafx26.txt`, any other session's files.
  - **Contracts and pin:** `parameter-spec.md` sha256 (file + body) = STATUS `contract_checksums`; `git cat-file -e 82406fce`; `git diff --quiet 82406fce HEAD -- plugins/O-Granulator/Source/dsp modules/synthesis/grain-engine modules/tuning/pitch-collections modules/effects/reverb-unit scripts/param-dump` (empty at plan time); `shasum -a 256 Source/dsp/forked/{GrainFx,ControlGrid}.h` = FORK-MANIFEST column 5 (`d92940e5…`, `8eb18919…`).
  - **Hands-on:** HANDS-ON-2.1 / 2.2 are deferred by the user (D14 override, SUMMARY-2.2); log it, not a gate.
  - **Baseline (BG):** `ninja -C build O-GranulatorInstrument-render-harness O-GranulatorInstrument-param-dump`; `$H` = **117 PASS / 0 FAIL** → `$S/h-22.log`; param-dump → `$S/pd-22.txt`, byte-identical to `.planning/params.tsv`; `$H --dump-render-hashes "$S/r22-base.sha256"` byte-identical to `tests/golden/renders-2.2.sha256`; `tests/fork-drift-check.sh` 0 drift.
  - **Parity baseline (BG, after the build):** `tests/parity/run.sh "$S/parity"` rc 0; `$H --parity "$S/parity/out"` **41/0** → `$S/par-22.log`; `$H --parity "$S/parity/out" --rows GV` **2/0**; the PN residual lines → `$S/pn-base.txt` (expect −110.9 … −120.2 dB); `cp -R "$S/parity/out" "$S/parity-base-out"`.
  - **Standalone target:** `ninja -C build -t targets all | grep -c '^OuariconGranulatorInstrument_Standalone: phony'` = 1.

**T2. Calibration witness, frozen reference, C22 on the unmodified 2.2 engine (P9, PD2)** [agent writes, run executes]
- Files: `tests/render-harness/cpu_calib.h` (new), `tests/render-harness/main.cpp` (include + `--calib`), `tests/golden/cpu-calib-2.3.txt` (new).
- Depends on: T1, with **every T1 background job finished**. **No `Source/` file is edited before this task is done**, so C22@2.2 measures the 2.2 engine.
- `cpu_calib.h` (namespace `cal`): `calibOnceNs()`, `calibNs()` per PD2, and `readRef (const juce::File& goldenDir)` → `ref_ns` or NaN. `--calib` prints `calib median <ns> samples <5 values>` and returns 0.
- Run **alone** (no ninja, parity, pluginval or Logic): build the harness (BG); then `$H --calib` in **three separate processes**; then `$H --cpu22` (BG); then `$H --calib` once more.
- Done when:
  - the three medians satisfy max / min ≤ 1.05 (else wait and re-run; a third failure → STOP and report a noisy machine);
  - `tests/golden/cpu-calib-2.3.txt` holds `ref_ns` (median of the three), the samples, date, CPU brand, harness rev, plus `c22_22_pct`, `c22_22_live`, `c22_22_duty` and the calib values bracketing `--cpu22`;
  - C22@2.2 has mean live ≥ 240 and duty ≥ 0.9; harness still 117/0; `git diff --stat -- Source` is empty.

### Wave 1 — parity side (runs while W2 is written)

**T3. FX parity rows + parent FX columns** [agent writes, run executes]
- Files: `tests/parity/ParityRows.h`, `tests/parity/parent_render.cpp`.
- Depends on: T1. The agent writes in parallel with W2; **the run starts only after T2's measurements** (quiet machine).
- **`ParityRows.h`** (PD5, PD11): `Row` gains `float matPeak = kMatPeak` (0.01, so every 2.2 row is unchanged) and `std::vector<std::string> oursHooks` (`legacySvfMake`, `fxOrderSwap`); `Kind` gains `fxNull`, `fxStats`; add `isNullKind` / `isStatsKind`; `material (n, ch, peak = kMatPeak)`. Rows (all on PN01 geometry: `nullBase()`, size 100, density 10, window 0, pitch 0, position 0.25; the existing `solve()` unchanged):

| Group | Rows (stream-suffix settings on top of `nullBase()`) | matPeak |
|---|---|---|
| precondition | **PF0** FX off | 0.25 |
| SVF (27) | **PFL1–9 / PFB1–9 / PFH1–9**: `filter_type` 1 / 2 / 3; index = 3·c + r + 1 over cutoff c ∈ {200, 2000, 12000} Hz × res r ∈ {0, 50, 100} % (PFL6 = LP 2 kHz res 100; PFB5 = BP 2 kHz res 50) | 0.1 |
| drive (4) | **PFD1–4**: drive 1 / 25 / 50 / 100 | 0.25 |
| crush (8) | **PFC1–4**: crush 10 / 40 / 70 / 100, prob 0 · **PFC5–8**: same, prob 100 (PFC6 = 40 / 100) | 0.25; **PFC8 0.4 + `level` −6 dB** |
| ring (6) | **PFR1–6**: ring_freq {20, 200, 5000} Hz × mix {25, 100} %, ordered (20/25, 20/100, 200/25, 200/100, 5000/25, 5000/100) | 0.25 |
| chained (3) | **PFX1–3**: LP / BP / HP 2 kHz res 80 + drive 50 + crush 40 prob 100 + ring 200 Hz mix 50 | 0.25 |
| controls (ours only; `parentOf` reuses the parent render) | **PFc1** PFB5, our cutoff × 2^(1/12) · **PFc2** PFD3, our drive 51 · **PFc3** PFC6, our crush 41 · **PFc4** PFR4, our ring_freq × 2^(1/12) · **PFc5** PFX1 + `fxOrderSwap` · **PFc6** PFL6 + `legacySvfMake` | as parent |
| statistics (`fxStats`, the `ps()` geometry: 4 streams × density 200 × size 20 × position 50 %, 5 s) | **PS5** LP 1 kHz, filter_rand 70 · **PS6** ring 200 Hz, mix 50, ring_rand 70 · **PS7** crush 50, crush_prob 30 | 0.01 |

- **`parent_render.cpp`:** the kind tests go through `isNullKind` / `isStatsKind`; material uses `row.matPeak`; every row's KV list (FX included) is set by exact raw value and counted in landed / total; `grains.tsv` appends `fxActive svfMode fcHz ringHz crushed driveD` (from `rec`, parent `Engine.h:350-356`) after the existing 16 columns; `manifest.tsv` appends the landed raws `filterType cutoff filterRand res drive crush crushProb ringFreq ringRand ringMix` and `peak` = max |out| over the row.
- Run (BG): `tests/parity/run.sh "$S/parity"`.
- Done when:
  - rc 0, every parent self-check passes (anchor == A, writeHead == W0, landed == total);
  - **every 2.2 row's `.f32` is byte-identical** to `$S/parity-base-out` (`cmp`);
  - PFC1–4 `.f32` are byte-identical to `PF0.f32` (prob 0 = no draw, inactive);
  - every PF row's parent `peak` ≤ 0.45 (else the PD5 level step once, recorded; still > 0.45 → STOP) and its `fxActive` bits equal the row's expected set on every grain;
  - PS5 / PS6 have the SVF / ring bit on ≥ 99 % of grains; PS7 has the crush bit equal to `crushed` on every grain, with 0 < p̂ < 1 (P11).

### Wave 2 — engine (agent writes; nothing builds until T8)

Audio-path rules are unchanged: no locks (except the PD3-2.2 scale try-lock), no allocation, `shared_ptr` or vector growth; fixed arrays only; `ScopedNoDenormals` per block; no listener writes a parameter. Latch math (tan / pow / exp2 / cos / sin / sqrt) runs **only at spawn**; stages advance one sample at a time.

**T4. ControlGrid FX rows + manifest (P8)** [agent; runner re-hashes in T8]
- Files: `Source/dsp/forked/ControlGrid.h`, `Source/dsp/forked/FORK-MANIFEST.md`.
- Depends on: T2.
- **ControlGrid.h**, ADAPTED region only (`:170-423`; `:38-165` stays byte-identical):
  - `GiParamPointers::Stream` (`:204-230`) gains `filterType, filterCutoff, filterRand, filterRes, drive, crush, crushProb, ringFreq, ringRand, ringMix`, bound in `bind` (`:237`) via `GiParamIds::stream (n, "filter_type")` … `"ring_mix"` for n = 1..4.
  - `GiGridParams::Stream` (`:304-333`) gains, with the spec defaults: `int filterType = 0` (Off), `float filterCutoffHz = 20000.0f`, `filterRand = 0.0f`, `filterRes = 0.2f`, `drive = 0.0f`, `crush = 0.0f`, `crushProb = 1.0f`, `ringFreqHz = 200.0f`, `ringRand = 0.0f`, `ringMix = 0.0f` (the parent `planFx` field names).
  - `latch` (`:336`): `readInt (filterType, 0, 3)`; `read (filterCutoff, 20, 20000)` (raw Hz from the skewed range, never inverted by hand); `readPct` for filter_rand, filter_res, drive, crush, crush_prob, ring_rand, ring_mix; `read (ringFreq, 20, 5000)`.
  - Update the header comment (`:170-186`): 2.3 adds the 10 per-grain FX rows; never bound: `sN_size_sync`, `sN_density_sync` (2.5), every other global row.
- **FORK-MANIFEST.md:** ControlGrid column 7 += "; 2.3: 10 per-grain FX stream rows (filter_type, filter_cutoff, filter_rand, filter_res, drive, crush, crush_prob, ring_freq, ring_rand, ring_mix)". The runner refreshes column 5 with `shasum -a 256` in T8. The GrainFx.h row is unchanged (VERBATIM, first-used 2.3).
- Done when: no VERBATIM leaf is edited (`GrainFx.h` byte-identical); drift check 2 holds for all 13 leaves (T8).

**T5. FX latch at spawn + spawn-side hooks + log (P1, P11)** [agent]
- Files: `Source/dsp/GiFxLatch.h` (new, header-only), `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on: T4.
- **`GiFxLatch.h`** (`ogi`): `struct FxPlan` (`fxActive`, `svfMode`, `svfK/A1/A2/A3`, `svfMake = 1`, `fcHz`, `driveD = 1`, `driveNorm = 1`, `crushed`, `crushHoldInc`, `crushScale`, `ringRotRe = 1`, `ringRotIm = 0`, `ringMix`, `ringHz`; defaults = the `Grain {}` defaults); `template <typename Uni> void planFx (FxPlan&, const GiGridParams::Stream&, double fs, Uni&& uni)` and `void assignFxLatch (Grain&, const FxPlan&)` — the parent statements (`Spawner.h:334-392`, `:398-414`) re-derived with names only changed. **`assignFxLatch` writes `svfMake`** (P1) and writes the latch only, never state. Draws: `Slot::FxCutoff` only when type ≠ Off && filter_rand > 0; `Slot::FxCrushProb` only when crush > 0 && 0 < prob < 1; `Slot::FxRing` only when mix > 0 && ring_rand > 0 (`forked/EngineConstants.h:258-263`, keyed by slot, so no 2.2 draw moves).
- **`Engine::spawn`** (`Engine.cpp:1121`):
  - `FxPlan fxp; planFx (fxp, sp, fs, uni);` after the window latch (`:1224-1225`) and before `stolenFlags` / `acquire` (`:1227-1235`), so a refused spawn consumes the same draws.
  - `legacySvfMake` (PFc6): `fxp.svfMake = 1.0f` when the SVF bit is set.
  - After `g.pitchCents` (`:1262`): `assignFxLatch (g, fxp);`. Replace the `:1263` comment: "killGain 1, killStep 0 and every FX **state** field from value-initialisation; the FX **latch** from assignFxLatch".
  - `fxSharedLatch`: re-plan with `filterRand = ringRand = 0`, `crushProb = 1` on a copy of `sp` and `assignFxLatch` again; the log keeps `fxp`.
  - `fxNoReset`: copy `fxShadow[a.slot]`'s FX state into `g`. Then count `fxStateNonDefaultAtSpawn` (FX state ≠ `Grain {}` defaults, compared bitwise).
  - `snapshotSteals`: before `acquire`, copy live slots `[0, 256)` into `stealSnap` (test-only `std::array<VoiceGrain, kPoolLive>` member). On `a.stolen`, the fade copy is the active slot in `[256, 288)` whose `(serial, g.age, g.readAbs)` equal `stealSnap[a.slot]`'s; compare the FX latch + state fields bitwise → `stealStateChecked`, `stealStateMismatches`. `stealSnapPerturb` flips 1 ulp of the snapshot's `svfIc1[0]` before comparing.
- **`Engine::gridTick`** (`:471`): `poisonFreeSlots` per PD12.
- **Log** (`GrainLogEntry`, `Engine.h:92-115`): add `fxActive svfMode fcHz ringHz crushed driveD` (parent names) from `fxp` (the real latch). The 2.1 / 2.2 fields keep their meaning and position (the V3 render-hash job hashes only the 2.1 fields).
- **Hooks** (`EngineTestHooks`, `Engine.h:120`): the PD12 set; `resetCounters` zeroes the new counters.

**T6. Kernel: gather → FX → scatter, NaN kill, `wetSafetyOff` (P5, P12)** [agent]
- Files: `Source/dsp/GiRenderer.h`, `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on: T5.
- **`GiRenderer::renderGrain`** (`GiRenderer.h:127-214`), per PD8:
  - Locals `float L[kCell], R[kCell], GK[kCell]; int n = 0;`.
  - The `:160-212` loop body is unchanged except that `:187-189` becomes `GK[n] = w * g.killGain * fader.next(); L[n] = l; R[n] = r; ++n;`. The xfade blend stays before it (FX run on the blended sample; one FX state per grain).
  - The `done` / break conditions are unchanged.
  - FX: `const std::uint32_t fxa = g.fxActive; if (fxa != 0) { … }`, in this order: SVF (`fx::svf (g, L, R, n)`, then `finite = fx::svfStateFinite (g) && ! forceKill`; when not finite → set the out-flag `nanKilled` and `return false`, **before the scatter**); drive; crush; ring. `fxOrderSwap` runs crush before drive. `nanKillOff` skips the finite check.
  - Scatter: `for (int j = 0; j < n; ++j) { bl[c0 + j] += L[j] * GK[j] * g.gainL; br[c0 + j] += R[j] * GK[j] * g.gainR; }`, then `return ! done;`.
  - Hook plumbing is a small `GiFxRenderFlags` (`forceKill`, `nanKillOff`, `fxOrderSwap`, out `nanKilled`) passed by reference; its fields exist only under `OGI_TEST_HOOKS` (the parent `#if` pattern), so the shipping kernel has no extra branches.
- **`Engine::renderGrains`** (`Engine.cpp:1104-1119`): the lambda computes `slot = &vg − base`; applies `injectNan` (writes `value` into field 0 `svfIc1[0]`, 1 `svfIc1[1]`, 2 `svfIc2[0]`, 3 `svfIc2[1]` of `slot` in the first segment with `t0 ≥ atSample`, once) and `forceRetire` (same trigger → `forceKill`); counts `nanKills`; writes `fxShadow[slot]` when the grain retires. Fade slots go through the same lambda.
- **`Engine::mixPiece`** (`:1397-1399`): under `OGI_TEST_HOOKS`, `wetSafetyOff` skips the two `kWetCeil · tanh` statements; the shipping statements are unchanged.
- Done when the agent reports the before / after text of `GiRenderer.h:187-189` (the shape argument for RB1 / FR1).

### Wave 3 — gates, build, parity

**T7. Harness gates** [agent]
- Files: `tests/render-harness/fx_units_23.h` (new, `fu23::runAll()`), `tests/render-harness/engine_gates_23.h` (new, `eg23::runAll()` + `eg23::cpu23()`), `tests/render-harness/parity_gates_23.h` (new, `pg23::run (dir, prefix)`), `engine_gates_21.h` (`readSet23()` + N2 switch, PD10), `engine_gates_22.h` (N2s switch only, PD10), `main.cpp` (includes, `fu23::runAll(); eg23::runAll();` after `eg22::runAll()`, `--parity` also calls `pg23::run`, `--cpu23`, banner and usage).
- Depends on: T6, T3 (`ParityRows.h`).
- Each row prints its metric, threshold, margin and control. A row without a control that can fail is not accepted, except the ported oracle rows (PD14), which keep the parent's control rows. Events in deterministic rows land at absolute multiples of 32 with event-split blocks.

| ID | Row | Stimulus | Pass | Negative control |
|---|---|---|---|---|
| FU1a–FU1d | SVF units (PD14) | parent `:140-262` | BP k·v1 at fc \|dB\| ≤ 0.1 (incl. fc = 0.45·fs × 44.1/48/96/192 k); LP/BP/HP vs oracle × make ±0.05 dB (n > 100); k 0.1 at 0.45·fs finite, \|y\| ≤ 100; `svfStateFinite` 12 false, finite true | FU1b inline: oracle without × make → worst > 0.05 dB |
| FU2a–FU2c | Drive units | parent `:265-356` | y(±r) == ±r (1 ulp); −12 dBFS sine peak 0.25 ±0.1 dB; guard d = 1e-4 rel ≤ 2e-7, below the guard inactive | (oracle rows) |
| FU3a, FU3b, FU3c, FU3n | Crush units | parent `:360-467` | silence → exactly 0; mid-tread lattice; hold runs floor/ceil(1/inc), first == q(x₀), 100 % @ 48 k = 48 | FU3n row: mid-rise gives \|q(0)\| = LSB/2 (100/100) |
| FU4a–FU4c, FU4n | Ring units | parent `:470-562` | drift ≤ −135 dB (5 f incl. 0.45·fs × 4 fs); carrier within 0.01 Hz; mix law | FU4n row: per-32 renorm > −135 dB |
| FN1 | Stage chunk invariance (D18) | each stage at an extreme (LP/BP/HP res 100 at 1 kHz and 0.45·fs, drive 100, crush 100 and 10, ring 5 Hz and 0.45·fs), 2 s noise peak 0.5 | chunks {1, 3, 17, 32} vs 32 bitwise, every case | U4n per-32-renorm ring under chunks 17 vs 32 → differs |
| FN2 | Engine block-size null, all FX (D18) | NB1 stimulus (`engine_gates_22.h:1353`) + s1 LP / s2 BP / s3 HP / s4 Off, cutoff 1.5 kHz, res 80, filter_rand 50, drive 40, crush 30 / prob 50, ring 300 Hz mix 50 rand 50 on all; s2 type BP→HP at a multiple of 32 | blocks 32/441/512/4096/8192/var bit-identical | seed + 1 → differs |
| FN3 | Unread params (PD10) | FN2 settings, 4 streams; every ID outside `readSet23()` (108) randomised | bit-identical; 153/153 read IDs bound | `s1_filter_cutoff` and `s4_ring_freq` → differ |
| FR1 | FX-off ≡ renders-2.2 (P5) | `rh::computeAll()` (`render_hashes.h:202`), 14 jobs | 14/14 lines = `renders-2.2.sha256` | N1-default with `s1_drive` 1 % → hash differs |
| FV1a | Cutoff census (port V1a, `:736`) | LP 1 kHz rand 100 %, ≥ 200 grains; rand 0; base 20 kHz rand 100 @ **44.1 k** | log2(fc/base) ∈ [−2, 2], span ≥ 3.6; rand 0 → fc == base bitwise; draws ≥ 0.45·fs log exactly `float (0.45·fs)` | rand 50 % → span < 3.6 |
| FV1b | Ring census (V1b) | 200 Hz rand 100 %; base 5 kHz @ 44.1 k | as FV1a | rand 50 % → span < 3.6 |
| FV1c | Crush Bernoulli (V1c, `:759`) | crush 50 %; prob 0 / 100 / 50 %, ≥ 2000 spawns | 0 none, 100 all, 50 % \|z\| ≤ 3.29 | the 50 % log tested against p = 0.3 → \|z\| > 3.29 |
| FV1d | `fxActive` gate table (V1d, `:808`) | every stage neutral / active combination | bits == expected; clear-bit stages keep `Grain {}` defaults (live pool scan) | checker fed a wrong table (drive 0 → `kFxDrive`) → mismatches > 0 |
| FV2 | BP centroid (V2, `:677`) | white noise, BP res 80 % 2 kHz, filter_rand 100 %, isolated grains (100 ms, 5 g/s, jitter 0, spray 10 %), ≥ 40 grains | Spearman ρ (log2 fcHz, log2 centroid) ≥ 0.9 | `fxSharedLatch` → \|ρ\| ≤ 0.5 |
| FV3 | Ring carrier (V3, `:847`) | sine 1 kHz, ring 200 Hz rand 100 % | ≥ 95 % of grains: USB carrier within 2 Hz of the latched `ringHz` | `fxSharedLatch` → < 20 % |
| FV4 | Crush split (V4, `:887`) | sine 440 Hz @ 0.5, crush 80 %, prob 50 % | out-of-band (−30 dB) split agrees with `crushed` ≥ 99 %, ≥ 10 per class | `fxSharedLatch` → ≤ 60 % |
| FB1 | No bleed vs a fresh pool (D18) | 2 s, 4 streams × 4 slots, LP/BP/HP/Off res 90, drive 50, crush 60 / prob 50, ring 400 Hz mix 70 rand 50; 8 notes, poly 8, d 200, size 300, cap 32 | `poisonFreeSlots` vs clean: max\|Δ\| == 0; spawns ≥ 1000 with R3 kills > 0 and native steals > 0 | poison + `fxNoReset` → differs |
| FB2 | Slot reuse after a resonant grain (H1, `:947`) | A: BP res 100 % @ 1 kHz + drive + crush + ring; B in the same pool slot on a silent region | B output == 0.0f exactly; same `poolSlot`; A rang (shadow \|ic\| > 1e-3) | `fxNoReset` → B ≠ 0 |
| FB3 | Steal copy (H2, `:978`) | steal-heavy all-FX render + `snapshotSteals` | ≥ 50 checked, 0 mismatches; the copy's SVF state moves on | `stealSnapPerturb` → mismatches == checked |
| FB4 | FX state == `Grain {}` at spawn (H3, `:999`) | a kills render and a steals render, ≥ 1000 spawns each | `fxStateNonDefaultAtSpawn` == 0 | `poisonFreeSlots` + `fxNoReset` → > 0 |
| FZ1 | Stress, 44.1 / 48 / 96 k (QUAL-01) | LP/BP/HP res 100 across streams, drive 100, crush 100 / prob 100, ring 5 kHz mix 100 rand 100; 16 notes × 4 streams, cap 256, s1 pitch +24 + notes 36 st above root (st → +48 clamp) and s2 −24; full-scale noise and a sine at fc | every output sample finite; 0 subnormal FX state in a per-tick pool scan; `nanKills` == 0; peak INFO | `injectNan` + `nanKillOff` → non-finite detected; scanner fed `denorm_min` → 1 |
| FZ2 | NaN kill (P12) | NaN / +Inf / −Inf × 4 SVF fields of a live SVF grain (12) + 1 fade-slot case | each: `nanKills` == 1, slot inactive after its segment, output bitwise ≡ the `forceRetire` render | `nanKillOff` → non-finite / ≠ reference |
| FA3 | 0 allocations, all FX (PERF-01) | A2 stimulus (`engine_gates_22.h:1259`) + all FX on (LP/BP/HP/Off, filter_rand / ring_rand 50 %, crush_prob 50 %), `filter_type` switches mid-render, note on/off, steals, cap 256 → 32, one injected NaN kill | 0 audio-thread allocations; `nanKills` == 1 | liveness `malloc(64)` → 1 |
| N2 / N2s | Carried, read set moved (PD10) | unchanged stimuli | 0 differ, 153 bound / 108 touched | their own |
| R0 / R21 / eg22 | Regression | 55 Stage 1 + 33 Phase 2.1 + 29 Phase 2.2 rows | **117 PASS**, thresholds unchanged; RB1 pinned == renders-2.1 | their own |
| PF0 | Precondition (PD6) | `--parity`, ours `wetSafetyOff`, 0.25 | residual ≤ **−130 dB**; sets g₀ | (gates PFC / PFX) |
| PF rows (48) | DSP-03 null (D18) | T3 rows; ours: 1 voice, root, kt 100, scan 0, `wetSafetyOff`, same Δ-shifted material | residual ≤ −90 dB after LS alignment over n ≥ 256; \|g/g₀ − 1\| ≤ 1e-4; parent peak ≤ 0.45; ref ≥ −70 dBFS; equal grain counts; FX raws landed identical; `fxActive` equal per grain; PFC1–4 our render bitwise ≡ PF0's | PFc1–PFc5 each > −90 dB |
| PFc6 | Make-down control (P1) | PFL6 + `legacySvfMake` | — | fires iff residual ≤ −90 **and** \|g/g₀ − 1\| > 1e-4 |
| PS5 | filter_rand (D18 stats) | T3 PS5, n ≥ 4000 per side | two-sample KS on log2(fc/1000) D ≤ 0.060; one-sample vs U(−1.4, 1.4) ≤ 0.043; \|Δmean\| ≤ 0.134σ; var ratio ∈ [0.887, 1.127]; range inside ±1.4 + 1e-6 and ≥ 98 % width; SVF bit ≥ 99 % both sides | ours filter_rand 35 % → fails |
| PS6 | ring_rand | T3 PS6 | as PS5 on log2(ringHz/200); ring bit ≥ 99 % | ours ring_rand 35 % → fails |
| PS7 | crush_prob | T3 PS7 | two-proportion \|Δp\| ≤ 0.050; each side \|p̂ − 0.3\| ≤ 0.0354; crush bit == `crushed` on every grain | ours prob 15 % → fails |
| PN / PS1–4 / SC / GV | Carried DSP-01 | unchanged | 41/0 + GV 2/0; PN residuals == `$S/pn-base.txt` to 0.1 dB | their own |
| C22r / C23w / C23t (`--cpu23`) | D19 / D20 (PD1, PD2, PD13) | C22 recipe (`engine_gates_22.h:1535-1615`); C23w = C22 + all 4 streams BP (type 2) 1 kHz res 100, drive 100, crush 100 / prob 100, ring mix 100 (freq 200, rands 0); C23t = PD1 | **C23w ≤ 36 %** with live ≥ 240; **C23t ≤ 12 %** with live 60–90; witness valid (else NOVERDICT = FAIL) | the witness rows are the control: before / after calib + duty printed per config |
| C23s / C23w96 | INFO | PD13; C23w at 96 k / 128 | printed, flag C23s > +0.6 points | — |

**T8. Build + default harness** [run]
- Files: none (the runner refreshes FORK-MANIFEST column 5 for ControlGrid.h).
- Depends on: T4–T7.
- Run (BG): `ninja -C build O-GranulatorInstrument-render-harness O-GranulatorInstrument-param-dump` (consoles only; plugin targets are built AND installed together in T12), then `$H`, then param-dump; `shasum -a 256 Source/dsp/forked/ControlGrid.h` → column 5; `tests/fork-drift-check.sh`.
- Done when:
  - rc 0, no new warnings in plugin sources;
  - harness **0 FAIL**: 117 carried (N2 / N2s on readSet23) + every FU / FV / FB / FZ / FA / FN / FR row, every control firing (`ctrl FIRED`, 0 `NOT FIRED`);
  - **FR1 14/14 and RB1 7/7** — otherwise **S3**;
  - param-dump byte-identical to `$S/pd-22.txt` and `.planning/params.tsv`;
  - drift **0 / 13** with `GrainFx.h` VERBATIM byte-identical, and NC-a / NC-b (re-made in `$S`) fire;
  - **X1:** `ninja -C build -t compdb` filtered to entries whose output contains `O-GranulatorInstrument` has 0 matches of `O-Granulator/Source|ogi-parity|82406fce` (control: run.sh's positive count).
- On a failure: fix it in the owning task (T1 map) and rebuild. **Never loosen a threshold or re-record renders-2.1 / renders-2.2.**

**T9. Parity run (DSP-03 null + stats; DSP-01 carried)** [run; agent fixes]
- Depends on: T3, T8.
- Run (BG): `tests/parity/run.sh "$S/parity"` (incremental; the parent renders now include PF / PS5–7); then `$H --parity "$S/parity/out" --rows PF0` → **PF0 ≤ −130 dB** (else **S1**, before any other PF row counts); then the full `$H --parity "$S/parity/out"` → `$S/par-23.log`; then `--rows GV`.
- Done when: `--parity` **0 FAIL** at about 99 rows (PD4): PF0 plus the other 48 PF rows with their band, PFc1–5 fire, PFc6 fires, PS5–7 pass with controls firing; the carried 41 rows pass with PN residuals equal to `$S/pn-base.txt` to 0.1 dB (FX off is a true bypass on the parity path); GV 2/0. SUMMARY gets the g₀ / g_row band table.
- A PF miss is diagnosed against the latch statements (`Spawner.h:334-392`), the `fs` passed in, stage order, the `svfMake` assignment, or the material / level row fields. **It is never tuned:** no offset, gain or threshold change. A crush / chained miss with PF0 passing → **S1**. A band miss → **S4**.

### Wave 4 — CPU verdicts

**T10. `--cpu23`: D19 + D20 (PD1, PD2, PD13)** [run]
- Depends on: T9 (no parity or build running; Logic closed).
- Run (BG, **alone**): `$H --golden-dir tests/golden --cpu23` → `$S/cpu23.log`.
- Done when: the C22r, C23w, C23t lines each carry calib before / after vs `ref_ns`, duty, live and median / min / max; **C23w ≤ 36 % (PASS)** with live ≥ 240; **C23t ≤ 12 % (PASS)** with live in [60, 90]; C23s and C23w96 printed. Δ = C23w − C22r and the per-grain FX cost go to SUMMARY (projection ≈ 30–31 %, R§6.1).
- A NOVERDICT is a FAIL: re-run alone up to 3 times; a third NOVERDICT → STOP and report the environment (not a design question). A valid-witness FAIL → T11.

**T11. Lever ladder — ONLY if T10 has a valid-witness FAIL (D21, PD9)** [agent + run]
- Files (if fired): `Source/dsp/GiRenderer.h`, `Source/dsp/Engine.cpp`.
- Depends on: T10.
- **Lever 2** (4-lane SIMD over same-(k,s) grains): kept only if FR1 14/14, RB1 7/7, FN2 0 differ and every PN / PF residual is unchanged; otherwise revert it (`git restore` of its files) and record it. A golden move inside the null tolerance is still **S3** (D21 allows it; the Defaults require consent). Rebuild consoles (BG), full harness, `--parity`, then `--cpu23` alone.
- **Lever 3**: confirm `fxa == 0` grains run no stage work (C23s); tighten with the P18 dual kernel (the verbatim 2.2 loop when `fxa == 0`). Same gates, then `--cpu23`.
- Still FAIL → **S2**. Lever 1 is never applied here.
- Done when: `--cpu23` passes with a valid witness, or S2 is raised with the measured numbers after each lever.

### Wave 5 — install, validators, hands-on sheet

**T12. Install + Standalone + validators + X2** [run]
- Depends on: T10 (T11 if fired).
- `./scripts/build-and-install.sh O-GranulatorInstrument` (BG): clears the AU cache and sweeps both the `O-GranulatorInstrument` and `-dev` variants; afterwards only `O-GranulatorInstrument-dev.{vst3,component}` are on disk.
- **Standalone (P14):** `ninja -C build OuariconGranulatorInstrument_Standalone` (BG); `build/plugins/O-GranulatorInstrument/OuariconGranulatorInstrument_artefacts/Release/Standalone/O-GranulatorInstrument-dev.app` must be newer than T8's harness binary; alive after 6 s.
- `auval -v aumu OuGI OuDv` PASS + the `ZZZZ` control. pluginval 10: VST3 ×2 + AU, BG, one at a time, `--timeout-ms 600000`, `Main bus num input channels: 2`, Logic closed.
- **X2:** `nm -C` on the installed VST3 + AU binaries and `$H` → 0 `ogran::`; control: the parity binary → > 0.
- Done when: all PASS.

**T13. `HANDS-ON-2.3.md` (human gate, deferred)** [agent writes; user runs after Stage 3]
- Files: `stages/2-dsp/HANDS-ON-2.3.md` (HANDS-ON-2.2 layout: build line, "Deferred with HANDS-ON-2.1 / 2.2" banner, defaults-to-know, table with Standalone / DAW / Notes columns, Result line).
- Depends on: T12.
- Rows (Standalone **and** one DAW, Logic AU):
  1. Filter LP / BP / HP per grain: sweep cutoff and res on a held note → each type audible; res 100 rings with no blow-up.
  2. Filter Random 100 % → grain-to-grain colour varies; 0 % → static.
  3. Drive 0 → 100 → progressive saturation; 0 sounds like FX off.
  4. Crush 0 → 100 with Crush Chance 100 / 50 / 0 → hold + bit reduction; at 50 only some grains crush; note where quiet material vanishes at high crush (R22 silence threshold, listening note).
  5. Ring 20 → 5000 Hz × Ring Mix; Ring Random 100 % → per-grain carriers differ.
  6. Extreme: res 100 + drive 100 + crush + ring on 4 streams × a chord at Grain Limit 256 → no dropouts, no stuck or silent output; judge the LP / HP resonance level against the wet safety (R19 listening note).
  7. Every FX off on every stream → the patch sounds as it did in 2.2.
  8. The D19 worst-case patch in Logic at 48 k / 128 → no overloads (informational; the verdict is C23w).
  9. Session reopen → every FX setting restores (APVTS).
- Done when the sheet exists. **Not self-certified**; the run is deferred to after Stage 3 with HANDS-ON-2.1 / 2.2 (user override 2026-11-08).

### Wave 6 — bookkeeping + commit

**T14. SUMMARY, STATUS, commit** [agent writes, run commits]
- Files: `stages/2-dsp/SUMMARY-2.3.md`, `.planning/STATUS.md`.
- Depends on: T12, T13. STATUS reads `stage_2_phase_2_3_executed_hands_on_pending` (HANDS-ON-2.1 / 2.2 / 2.3 deferred).
- SUMMARY covers:
  - every row with margin and control;
  - the exact row counts vs the PD4 targets;
  - PF0 residual, g₀ and the band table; PFc6;
  - PS5–7 statistics;
  - C22r / C23w / C23t / C23s / C23w96 with `ref_ns` and calib before / after, Δ and per-grain FX cost vs the parent 0.064 %/grain;
  - the lever outcome (not fired, or which lever and its numbers);
  - PD1–PD15, with the PD5 / PD7 / PD10 / PD14 corrections and refinements stated;
  - every deviation from this plan with its reason.
- Commit:
  1. `git branch --show-current` (= `main`) and `git status --short` **immediately before**. `git status --short -- plugins/O-GranulatorInstrument` must list only this plan's files. Anything else under the plugin → STOP and report before committing.
  2. `git add` the exact new paths: `Source/dsp/GiFxLatch.h`, `tests/render-harness/{cpu_calib,fx_units_23,engine_gates_23,parity_gates_23}.h`, `tests/golden/cpu-calib-2.3.txt`, `stages/2-dsp/{SUMMARY,HANDS-ON}-2.3.md`. Then `git commit -- plugins/O-GranulatorInstrument`. Never `-A` / `-a`; foreign paths (`modules/registry.yaml`, `.claude/*`, `plugins/O-GrainScatter/*`) are never staged.
  3. `git log -1 --stat` shows only `plugins/O-GranulatorInstrument/` paths. **No tag.**

---

## Dependency graph

```
T1 ─→ T2 ─→ T4 ─→ T5 ─→ T6 ─→ T7 ─→ T8 ─→ T9 ─→ T10 ─→ [T11 only on FAIL] ─→ T12 ─→ T13 ─→ T14
 │     │                       ↑            ↑
 │     └─ (quiet window ends) ─┼─ T3 run ───┘   (parent renders → T9)
 └─→ T3 write (parallel with W2) ─┘             (ParityRows.h → T7)
```

T2 must finish before any `Source/` edit (C22@2.2 and the reference come from the unmodified engine) and before T3's parent build (quiet machine). T4 → T5 → T6 run strictly in sequence (T5 and T6 both edit `Engine.{h,cpp}`). T10 runs alone after every build and parity job. CMake is not edited (PD15).

## Execution notes

- **Agent split.** dsp-agent writes T2's `cpu_calib.h`, T3's sources, T4–T7, T11's code (if fired), T13 and T14's SUMMARY. The runner does every build, gate, validator, hash refresh and the commit. Long commands (ninja, `run.sh`, `build-and-install.sh`, the Standalone link, pluginval, cold auval, `--cpu22`, `--cpu23`, the full harness) run BG with logs in `$S` (600 s watchdog).
- **Writing order inside W2:** T4, then T5, then T6. Nothing is configured or built between them.
- **Parent reads** go through `git show "${PIN}:…"` (braced) or the run.sh archive, never the working tree. The parity reference is only ever built by `tests/parity/run.sh` from `git archive 82406fce`.
- **CPU runs** (T2, T10, T11) happen with nothing else running: no ninja, parity, pluginval, Logic or other heavy session.
- **Build and install** only via `./scripts/build-and-install.sh O-GranulatorInstrument`, plus the explicit Standalone target.
- **Handoff.** After execute: `/clear` → `/plugin-verify O-GranulatorInstrument 2-dsp`.

## Success criteria (CONTEXT-2.3 gate)

| Criterion | Proven by |
|---|---|
| FX null vs parent ≤ −90 dB, each FX alone + chained, crush_prob 0 / 100; per-family controls fail (D18) | PF0, PFL / PFB / PFH, PFD, PFC, PFR, PFX, PFc1–5 |
| Gain alignment cannot hide a gain-law error (CONTEXT risk) | PF band (1e-4 vs g₀), PFc6, FU1b control |
| Parent GrainFx law rows ported and green; block-size invariance per stage (D18) | FU1a–FU4n, FN1, FN2 |
| Statistical arm filter_rand / ring_rand / crush_prob within CI, wrong-distribution controls fail (D18) | PS5–PS7, FV1a–FV1c |
| The renderer honours the per-grain latch | FV2, FV3, FV4 |
| No state bleed on slot reuse (bitwise vs a fresh pool), skip-reset control fails (DSP-03) | FB1–FB4 |
| 0 allocations with all FX on across note on/off, steals, cap change (PERF-01 arm) | FA3 |
| Stress at 44.1 / 48 / 96 k: no NaN / Inf / denormal; NaN kill fires on injection (QUAL-01 arm) | FZ1, FZ2 |
| 0.45·fs clamps at 44.1 / 48 / 96 k (PD7) | FU1a, FU1c, FU4a, FV1a, FV1b |
| FX-off renders byte-identical to renders-2.2; carried rows; param-dump identical; drift 0 | FR1, RB1, 117 carried rows, T8 param-dump, drift + NC-a / NC-b |
| DSP-01 unchanged | PN / PS1–4 / SC / GV re-run, PN residuals == T1 baseline |
| CPU worst case ≤ 36 % with witness; FX-off C22 in the same session (D19) | C23w, C22r, PD2 witness |
| Typical proxy ≤ 12 % (D20) | C23t |
| auval PASS; pluginval 10 VST3 + AU; parent code excluded | T12, X1, X2 |
| HANDS-ON-2.3 written, deferred post-Stage-3 | T13 |

## Pitfalls → task + gate (RESEARCH P1–P14)

| # | Pitfall | Task | Gate |
|---|---|---|---|
| P1 | `svfMake` not latched → LP/HP null still passes | T5 (`assignFxLatch` writes it) | PF band, PFc6 (`legacySvfMake`), FU1b control |
| P2 | −40 dBFS material → crush vacuous, BP dead | T3 (matPeak, PFC8 level), T6 (`wetSafetyOff`) | parent peak ≤ 0.45 (INVALID = FAIL), ref ≥ −70 dBFS per PF row |
| P3 | Crush null breaks on 1-ulp read drift | T9 | PF0 ≤ −130 dB precondition → S1 |
| P4 | Clamp rows not constructible at 48 k | T7 (PD7) | FU1a, FU1c, FU4a, FV1a / b @ 44.1 k |
| P5 | Scatter statement split → FX-off hashes move | T6 (PD8) | FR1 14/14, RB1 7/7 → S3 |
| P6 | Row-ID collision (V3, H3, A2) | T7 (PD4 prefixes) | no duplicate ID in `$S/h-23.log` (`sort \| uniq -d` empty) |
| P7 | N2s counts 148/113 hard-coded (and eg21 N2) | T7 (PD10) | N2, N2s at 153/108; FN3 |
| P8 | `ControlGrid.h` edit not re-hashed | T4, T8 | FORK-MANIFEST col 5/7, drift 0/13, GrainFx.h byte-identical |
| P9 | Duty witness blind to a slowed core | T2, T10 (PD2) | calib before/after vs `ref_ns`; NOVERDICT = FAIL |
| P10 | Typical proxy vacuous at defaults | PD1 | C23t live ∈ [60, 90] asserted |
| P11 | Stats rows with the gate closed draw nothing | T3, T7 | PS5 / PS6 bit ≥ 99 %; PS7 bit == `crushed`, 0 < p̂ < 1 |
| P12 | NaN reaches the bus | T6 (kill before scatter) | FZ2 vs `forceRetire`, FZ1 |
| P13 | Hermite lever moves goldens | T11 (PD9) | never auto-applied; S2 |
| P14 | Standalone stale | T12 | explicit `OuariconGranulatorInstrument_Standalone` build, `.app` newer than T8 |

## Stop-and-ask conditions (execute; never pre-decided)

- **S1 — crush null precondition.** PF0 > −130 dB, or a PFC* / PFX* row fails while PF0 and its SVF / drive / ring siblings pass. STOP and offer: (a) find the pre-FX read difference; (b) re-specify the crush rows as a flip-count metric (amends D18).
- **S2 — CPU over budget.** C23w > 36 % or C23t > 12 % (valid witness) after the automatic levers 2 → 3. STOP and offer: lever 1 (Hermite, rate ≤ 1 at level 0; timbre gate ≤ 0.6 dB to 10 kHz and ≤ 3 dB to 15 kHz vs the band-limited read, +24 / +48 alias rows green, forced-Hermite-at-rate > 1 control; it moves renders-2.2 and the PN nulls, so the hashes are re-pinned only with consent, P13); and/or the `grain_limit` 256 → 128 fallback + "heavy" UI note (a locked-spec change; it always asks, D21).
- **S3 — FX-off bypass broken.** Any renders-2.2 line differs (FR1), RB1 < 7/7, or a lever moves a golden. STOP and offer: (a) find the statement-shape change (P5); (b) the P18 dual kernel (the verbatim 2.2 loop for `fxa == 0`). Never re-baseline.
- **S4 — gain band.** \|g_row / g₀ − 1\| > 1e-4 on any non-control PF row. STOP: a latched scalar (`svfMake`, `driveNorm`, level) differs from the parent. Diagnose; never widen the band.
- **Inherited stops:** T1 contract / pin / late-change mismatch; a PF row still INVALID after the one PD5 level step; three NOVERDICTs in a row (environment); unexpected paths under the plugin at commit time.

## Risks

- **R1 Crush null fragility** (R§2.4). One quantiser flip per second is about −87 dB. PF0 is the precondition and S1 is the decision.
- **R2 Parent-side INVALID / dead rows** (PD5). The peak assert and the liveness floor turn a bad stimulus into a FAIL, never a silent PASS. The one allowed fix is post-FX level.
- **R3 FP statement shape** (PD8). The gather/scatter split can change contraction. FR1 / RB1 catch it; fix the shape, never the golden.
- **R4 CPU margin** (R§6.1, about 30–31 % vs 36 %, a 4–7 point margin). Measured, not assumed; a slowed core reads NOVERDICT = FAIL, never PASS.
- **R5 Lever 1 vs goldens** (PD9). It cannot be automatic; S2 owns it.
- **R6 Vacuous statistics** (P11). Every PS row asserts its draw is alive.
- **R7 Test hooks in shipping builds.** Every hook is under `OGI_TEST_HOOKS`. The shipping kernel is the PD8 statements plus `if (fxa != 0)`. X2 and the installed-binary validators run on the shipping build.
- **R8 Parity build cost.** The archived parent rebuild takes minutes; it runs BG, incrementally, and never from the parent working tree.
- **R9 Stale expectations.** The FU1i INFO and SUMMARY-2.2's PN figures predate D10 / T12. Compare against VERIFICATION-2.2 HEAD figures and the D10-adjusted INFO text.
- **R10 Concurrent sessions.** Foreign dirty paths exist. Path-scope the commit and re-check branch and status right before it.
