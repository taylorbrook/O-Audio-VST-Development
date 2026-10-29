# Verification: Signal-adaptive grain length in freeze (extrema density)

**Plugin:** O-GrainScatter
**Milestone:** adaptive-grain-length
**Created:** 2026-10-28
**Phase:** Verify

---

## Verification Summary

**Status:** PASSED. The automated checks passed, and the user passed human checks H1–H4 on 2026-10-29. H1, the Scan 0 level rise, is accepted as a known limitation.
**Version:** 3.4.0 → 3.5.0. The installed bundle reports 3.5.0 (AU 0x30500).

Everything below was rebuilt and re-run in this verify session: plugin, Standalone and all 12 gate binaries, run from a fresh scratch dump directory. None of it was copied from SUMMARY.md.

---

## Requirements Verification

### R1: `grain_adapt` 0–100 %, default 0, Adapt 0 bit-exact vs v3.4.0 ✓ PASS
- **A rows:** adapt-gate against the v3.4.0 twin (144a73f3, git archive) is bit-exact on all 8 paths, with peak diff 0.0. The paths are free, sync, spatial, freeze, freeze + rev/SizeRnd/Tilt, freeze +24 st, freeze spatial, and freeze-sync-reengage.
- **Live paths at Adapt 100:** free, sync and spatial are also bit-exact, confirming that live grains are untouched.
- **Session load:** a v3.4.0 session loaded into an instance with Adapt pre-set to 50 reads Adapt 0 and renders bit-exact.
- **Code review:**
  - All new math sits behind `grainAdapt > 0.0f && adaptReady`.
  - The `tiltPeakFor` move only changes when it is called. At Adapt 0 it gets the same arguments (`gp.grainLengthSamples == actualGrainSize`).
  - The c re-solve is skipped at 0.
  - The parameter is `{ "grain_adapt", 5 }` with range 0–100 and step 0.1, so the 0 default is exact.

### R2: Snapshot extrema analysis, RT-safe ✓ PASS
- **U1:** M and K0 match the prototype within 1 %.

  | Stimulus | M | K0 |
  |---|---|---|
  | Sine 55 Hz | 220 | 11.0 |
  | White noise | 47031 (prototype 47030) | 2366.4 |
  | Silence | 0, fallback | — |

- **R1:** 0 allocations in processBlock across engage, analysis, re-engage mid-analysis, release and Adapt 0↔100. The liveness probe counts 1.
- **C1 (re-measured):** no block comes close to the 50 µs limit.

  | Rate | Worst-block median | Total | Spread over |
  |---|---|---|---|
  | 48 kHz | 12.6 µs | 401 µs | 77.3 ms |
  | 96 kHz | 14.6 µs | 893 µs | 69.7 ms |

- **Code review:**
  - `ExtremaMap` allocates only in `prepare()`.
  - `solveC` keeps 2 × 256 doubles (4 KB) on the stack. That is acceptable on the audio thread, and it runs only when (gen, GS, amount) changes.
  - `begin` bumps the generation, and so do `clear`, `cancel` and `release`.
  - The analysis quota counts from the absolute engage sample, so the work is independent of block size (D2).

### R3: Adaptive length, Grain Size as the mean ✓ PASS
- **L1/L2:** the sine-half / noise-half length ratio is 5.78 at Adapt 100 (needs ≥ 3.0) and 2.80 at Adapt 50 (needs ≥ 1.8). The Adapt 0 control is 1.0000.
- **M1, forward:** mean error ranges from −0.15 % to +1.97 % (12 rows, ≥ 2387 spawns each). The limit is ±5 %.
- **M2, reverse:** mean error ranges from −1.55 % to +1.32 %. The limit is ±5 %.
- **P1:**
  - GS 50 → 100 gives a ratio of 2.000000 on all 2387 grains.
  - All 2387 logged lengths match the map's lookup.
- **N1, negative control:** the fixed-K twin gives a mean of 237.2 ms (+137 %), so it FAILS as it must and the M rows are not vacuous.
- **F1 fallbacks:** each case renders bit-exact against Adapt 0 and adapts 0 grains.
  - silent snapshot;
  - −80 dB noise;
  - loop shorter than 0.2 s;
  - grains spawned before readyAbs (readyAbs is 101994, i.e. 76.9 ms).

### R4: Grains/sec unchanged, level flat ✓ PASS (with a known limitation, see H1)
- **V1:** at Scan 30, Adapt 50 vs 0 differs by −0.003 dB, against an old-vs-old control of 0.007 dB. The length ratio in the same renders is 2.83.
- **V2:** the dense noise cloud at Adapt 100 vs 0 differs by −0.000 dB.
- **V3 (info only):** at Scan 30, Adapt 100 is −0.16 dB. **At Scan 0 the level rises +1.29 dB (Adapt 50) and +1.70 dB (Adapt 100).**
  - The cause is grains pinned to the read-head floor that sum coherently. The single ρ term in the density compensation doesn't track them.
  - This is documented in the CHANGELOG under Known limitations.

### R5: UI ✓ PASS
- **Layout:** the Adapt knob sits after Grain Size, with `.core-group` cells at 58 px and a 3 px gap. The frame is unchanged.
- **Editor:** relay before attachment, `.withOptionsFrom`, and the `pctFormatter` readout.
- **Gates:**
  - `check-i18n`: ALL CHECKS PASS (46 plugins).
  - `i18n-fr-lint` and `i18n-zh-lint`: rc 0.
  - `check-ui-labels --plugin O-GrainScatter`: ALL CHECKS PASSED.
  - `ui_tip_render_check.js`: 1366 / 1366.
- **FR labels:** "Adapt." (`reviewed: false`), and "Inclin." with a `termNote`.

### No regressions ✓ PASS

| Gate | Result |
|---|---|
| blocksize-null | 21 / 21 ALL PASS |
| density-gain (+ compoff reference) | 24 / 24 ALL PASS |
| alias-gate | 10 / 10 ALL PASS |
| alias-gate-v320 negative control | rows 1 / 2 / 4 FAIL as required |
| skip-tilt vs v3.3.0 twin | 30 / 30 ALL PASS |
| adapt-gate | 47 / 47 ALL PASS |
| pluginval strictness 10, `O-GrainScatter-dev.vst3` | SUCCESS |
| auval `aufx OuGS OuDv` | AU VALIDATION SUCCEEDED, 3.5.0 |
| Install via build-and-install.sh | dev variant only, no alternate-variant orphan |

**Build warnings:** none new in plugin code.
- PitchEngine.h:395 and the binaural indexing warnings in PluginProcessor.cpp are also present in the v3.3.0 and v3.2.0 twins, so they predate this milestone.
- The remaining warnings are unused helpers in test code.

> The row counts differ from SUMMARY.md (22 / 25 / 31) only because SUMMARY counted the "ALL PASS" line as a row.

---

## Human Checks (passed 2026-10-29)

| # | Check | Notes |
|---|---|---|
| H1 | **Scan 0 level rise.** Freeze a mixed sparse/dense capture (bass + hats/snare) at Scan 0, then sweep Adapt 0 → 100. Is the +1.3 to +1.7 dB acceptable? | Accept as a known limitation (already in CHANGELOG), or open a follow-up: per-region coherence, or counting floor-pinned grains separately. |
| H2 | **Smear on sparse passages** with Scan up and Adapt 100 (RESEARCH risk 5). | Lever: scale the scatter draw by `max(1, len / GS)`. |
| H3 | **Tref 100 ms feel.** Is the contrast between bass and transient regions musical? | Alternative: 50 ms. |
| H4 | **DAW.** The readout shows %, Adapt automates, Freeze engage/release while adapting is click-free, and adaptation starts about 77 ms after engage. | Installed: `O-GrainScatter-dev` VST3 + AU. |

---

*Generated by improve-milestone verify phase*
