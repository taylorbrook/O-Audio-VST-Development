# Verification: Band-limited grain reader (mipmap + polyphase sinc)

**Plugin:** O-GrainScatter (+ grain-engine 1.1.0)
**Milestone:** band-limited-reader
**Created:** 2026-10-27
**Phase:** Verify

---

## Verification Summary

**Status:** PASSED. The automated checks passed, and the user passed human checks H1–H4 on 2026-10-27.
**Version:** O-GrainScatter 3.2.0 → 3.3.0. grain-engine 1.0.0 → 1.1.0.

Everything below was rebuilt and re-run in the verify session. None of it was copied from SUMMARY.md.

---

## Requirements Verification

### R1: `BandlimitedReader` in grain-engine ✓ PASS
The module tests were built fresh from `modules/synthesis/grain-engine/tests`. **Result: 38/38 PASS.**
- B1, unity DC: the worst row is off by 5.1e-08 and the worst lerped phase by 9.3e-08. The spec allows 1e-6.
- B2, level alignment: levels 1, 2 and 3 land within +0.0001, −0.0005 and +0.0031 samples of level 0. The spec allows 0.05.
- B3, stopbands:
  - The half-band reaches −92.0 dB.
  - S0–S6 range from −60.6 to −94.4 dB, all within the −60 dB spec.
  - S7 is report-only: −62.4 dB at r′ 2.83 and −25.6 dB at r′ 3.99.
- B4 and B6, determinism: block-split reads and snapshot reads are bit-identical to live reads.
- B5: zero allocations in push, read, capture, snapshot read and clear.
- B7: margin(ℓ) is 9 / 80 / 222 / 506 samples, and the test shows it is tight.
- Code review:
  - Reads stay inside the mirrored guard: the furthest tap is at `cap + 14`, inside a guard of `cap + 16`.
  - Snapshot storage covers the pre-roll and post-roll at every level.
  - The level position is split into integer and fraction before the division by 2^ℓ.

### R2: GrainScatter live ring on the reader ✓ PASS
This is `O-GrainScatter-alias-gate`, re-run at 44.1 kHz. The pass threshold is −60 dB audible spurious.

| Row | v3.2.0 control | v3.3.0 |
|---|---|---|
| +12 / +24 / +36 st stereo, tones | 0.0 dB FAIL | −95.8 / −95.9 / −95.4 dB |
| +12 / +24 / +36 st stereo, sweep | −0.3 / −0.4 / −0.5 dB | −108.2 / −99.1 / −80.8 dB |
| +24 st through `processBlock` | −0.2 dB FAIL | −117.0 dB |
| +36 st spatial, Doppler 1.0 / 1.4 | 0.0 dB FAIL | −95.4 / −63.5 dB |
| Octave boundary, rate 1.99 vs 2.01 | — | 0.0000 samples (positive control reads 0.197) |

- The control fails every alias row, so the gate is not vacuous.
- Code review: one `margin(ℓ)` source drives the spawn pre-roll, the density `floorS` and the reader's own clamp. The Doppler read-past-head hole is closed.

### R3: Freeze snapshot carries the levels ✓ PASS (offline)
- Frozen +36 st: −95.3 dB on v3.3.0, against 0.0 dB on the v3.2.0 control.
- blocksize-null freeze rows are bit-exact at block sizes 32, 512 and variable.
- Freeze rows in density-gain pass: loop correlation is 0.005 and level is −1.19 dB.
- Code review: capture ends at `head − 506`, and the seam crossfade logic is unchanged.
- Edge case: if freeze is engaged with fewer than 506 samples in the ring, the loop length is 0, so frozen grains are silent until the next engage. Before this release you got a loop of a few samples instead. This is acceptable.
- The hands-on Standalone check (H2) passed.

### R4: No regressions ✓ PASS
- **blocksize-null: ALL PASS.**
  - Every render row reaches −400 dBFS.
  - The seed negative control differs at −2.9 dBFS, as it should.
  - Ring precision with the head past 2^21 is −400 dB.
- **density-gain: ALL PASS.**
  - The sparse row nulls against the comp-off build at −400 dB.
  - THD is −160 dB.
- **pluginval:** strictness 10, VST3 (`O-GrainScatter-dev.vst3`), result SUCCESS.
- **auval:** `aufx OuGS OuDv`, result AU VALIDATION SUCCEEDED. The bundle reports version 0x030300, which is 3.3.0.
- **Build warnings:** no new ones in plugin code; the plugin warnings in the log match the v3.1.0 build. Two warnings are in test code only: an unused `next` in `alias-gate/main.cpp:254` and an unused `failures` in `density-gain/main.cpp:56`.

### R5: Module release ✓ PASS
- `module.yaml` is at 1.1.0 and lists O-GrainScatter 3.3.0 in `used_by`.
- The grain-engine entry in `registry.yaml` is updated.
- The README is updated.

---

## Performance

64 grains at +24 st, 48 kHz, re-measured:

| Path | v3.2.0 | v3.3.0 |
|---|---|---|
| Stereo | 477 ns per output sample | 876 ns per output sample |
| Spatial | 765 ns per output sample | 1224 ns per output sample |

That is about 4–6 % of one core. It matches the SUMMARY figures, so tap tiering is not needed.

---

## Commit scope note

`PLUGINS.md` also holds an uncommitted O-Prism row edit from another session. Stage only the O-GrainScatter hunk. Per the project rule, create no tag unless `/publish` is run.

---

## Human checks (all passed, user, 2026-10-27)

| # | Check | Pass when |
|---|---|---|
| H1 | Sweep transpose from +11 to +13 st on bright material. | The top-end step at +12 st is acceptable. If not, set `blr::kTaps` to 24, at about +50 % reader CPU. |
| H2 | In the Standalone, engage, release and re-engage freeze at +0, +24 and +36 st. | No click, no silence, no stale loop. |
| H3 | Load the factory presets. | They load and sound as before, apart from the documented top end. |
| H4 | A/B unity pitch against 3.2.0. | The timbre change is acceptable. |
