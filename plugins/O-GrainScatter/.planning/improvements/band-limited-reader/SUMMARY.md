---
milestone: band-limited-reader
phase: execute
status: complete
version: 3.2.0 -> 3.3.0
module: grain-engine 1.0.0 -> 1.1.0
completed: 2026-10-27
---

# Summary: Band-limited grain reader (mipmap + polyphase sinc)

## Implementation Summary

Grains that are pitched up in O-GrainScatter no longer alias. A new header-only `BandlimitedReader.h` (grain-engine 1.1.0) replaces the Lagrange-4 reader on the live ring and on freeze snapshots. It has two parts:
- a mirrored stereo mip ring, built by a causal 127-tap Kaiser half-band cascade with L = 3 and group-delay compensation;
- a 16-tap polyphase sinc bank, 8 sets × 257 phases, with every row at unity DC.

The new alias gate passes on 3.3.0 and fails on 3.2.0 (stereo, spatial and frozen rows), so the gate is not vacuous. Nothing is committed, tagged or staged.

## Tasks Completed

### Task 1: Offline design of the sinc bank and half-band constants ✓
- `research-scripts/emit_constants.py` generates the constants.
- Half-band: stopband −92.0 dB, centre exactly 0.5, even taps exactly 0. β was raised to 9.2 (Deviation 1).
- Sinc sets S0–S6 are designed to −60.5 dB. S7 is designed at r′ 2.83 (Deviation 4).
- At the research settings, the script reproduces RESEARCH §5 within 0.05 kHz.

### Task 2: `BandlimitedReader.h` in grain-engine ✓
- Compiles header-only with `-Wall -Wextra` and no warnings.
- `push`, `read`, `readMono` and `MipSnapshot::read` make no allocations.

### Task 3: Module tests + module release metadata ✓
- 38/38 pass.
  - Level alignment ≤ 0.003 sample.
  - Driven stopbands for S0–S6: −60.6 to −94.4 dB.
  - Block-split reads and snapshot-vs-live reads are bit-identical.
- Negative controls fail as expected:
  - Scaling one row by 1.001 fails the DC check.
  - Removing the `D(ℓ)` term fails alignment by 62 samples.
- `module.yaml` is at 1.1.0 and lists O-GrainScatter in `used_by`. `README.md` and the grain-engine entry in `modules/registry.yaml` are updated.

### Task 4: GrainScatter live ring on the reader ✓
- VST3 and AU build clean, with no new warnings.
- `LagrangeInterpolation.h` is deleted, and no references to it remain.
- One `margin(ℓ)` source drives the spawn guard, `floorS` and the read-time Doppler clamp.

### Task 5: Freeze snapshot carries the levels ✓ (offline)
- The snapshot holds every octave level, and capture ends at `head − margin(3)` (about 10 ms earlier).
- Positions use an exact integer + fraction split (Deviation 7).
- **Not yet checked:** freeze engage, release and re-engage in the Standalone. This moves to verify.

### Task 6: Gates — new alias gate + rework of two existing rows ✓

**Alias gate** (44.1 kHz; a row passes at ≤ −60 dB audible spurious):

| Row | v3.2.0 | v3.3.0 |
|---|---|---|
| +12 / +24 / +36 st stereo (worst of tones and sweep) | 0.0 dB | −95.8 / −95.9 / −80.8 dB |
| +24 st through `processBlock` | −0.2 dB | −117 dB |
| +36 st spatial, Doppler 1.0 / 1.4 | 0.0 dB | −80.8 / −63.5 dB |
| Frozen +36 st | 0.0 dB | −95.3 dB |
| Doppler 2 at +36 / +35 st (reported, not gated) | 0.0 dB | −67.3 / −38.5 dB |
| Octave boundary timing, rate 1.99 vs 2.01 | 0.000 samp | 0.000 samp |

The boundary timing row also passes on 3.2.0, because 3.2.0 has no octave levels. Its own negative control works: a deliberate 0.2-sample offset is detected as 0.197.

**blocksize-null:** all rows pass.
- The precision row is re-referenced to reader vs reader. It reads −400 dB.
- Casting the read position to float makes it fail at −47.9 dB.

**density-gain:** all rows pass.
- The sparse row is now checked against the build with density compensation off, and the two are bit-identical.
- Applying a 0.99 gain makes it fail at −40 dB.
- The unity-pitch level rows moved by 0.5–0.7 dB, because the sinc now cuts white noise above about 20.5 kHz. All of them are still within ±1.5 dB.

No thresholds were loosened.

### Task 7: Build, install, validate, document ✓
- Installed with `./scripts/build-and-install.sh O-GrainScatter`.
- pluginval at strictness 10: SUCCESS. auval: PASS, reports 3.3.0.
- `CMakeLists.txt` is at 3.3.0, and the CHANGELOG has a `## [3.3.0] - 2026-10-27` entry with the known limits. Only the O-GrainScatter row of PLUGINS.md was changed.

## Files Changed Summary

**Created**
- `modules/synthesis/grain-engine/cpp/BandlimitedReader.h`
- `plugins/O-GrainScatter/tests/alias-gate/main.cpp`
- `.planning/improvements/band-limited-reader/research-scripts/emit_constants.py`

**Modified**
- grain-engine: `tests/grain_engine_tests.cpp`, `tests/CMakeLists.txt`, `module.yaml`, `README.md`
- `modules/registry.yaml` (grain-engine entry only)
- O-GrainScatter: `CMakeLists.txt`, `CHANGELOG.md`, `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h` (test-only hook), `Source/dsp/DelayBuffer.h`, `Source/dsp/FreezeManager.h`, `Source/dsp/GrainPool.h`, `tests/blocksize-null/main.cpp`, `tests/density-gain/main.cpp`
- `PLUGINS.md` (O-GrainScatter row only)
- `PLAN.md` (checkboxes), `STATUS.yaml`

**Deleted**
- `plugins/O-GrainScatter/Source/dsp/LagrangeInterpolation.h`

**Generated (not tracked)**
- `build/gs-v320-9c6c22d0/`: the 3.2.0 source extracted for the negative control

## Implementation Notes

### Technical Decisions Made
- The octave level is latched when a grain spawns. The sinc set is picked from the live residual rate.
- 16 taps, per the plan. Going to 24 taps means changing one constant (`blr::kTaps`) and costs about +50 % reader CPU.

**Passband (output −1 dB point, 44.1 / 48 kHz)**

| Pitch | −1 dB point |
|---|---|
| 0 / +12 / +24 / +36 st | 16.2 / 17.7 kHz |
| +3 st | 13.5 / 14.7 kHz |
| +6 st | 13.0 / 14.2 kHz |
| +9 st | 9.6 / 10.4 kHz |
| +11.9 st | 8.3 / 9.1 kHz |
| −12 st | 8.1 / 8.8 kHz |

**CPU** (64 grains at +24 st, 48 kHz)

| Mode | 3.2.0 | 3.3.0 | Change | Share of a core |
|---|---|---|---|---|
| Stereo | 477 ns/sample | 871 ns/sample | +83 % | 4.2 % |
| Spatial | 765 ns/sample | 1234 ns/sample | +61 % | 5.9 % |

### Deviations from Plan
1. **Half-band β changed from 9 to 9.2.** With the centre pinned to exactly 0.5, β 9 reaches only −89.95 dB, which misses the −90 dB spec.
2. **The passband at the top of each octave is a little narrower than RESEARCH §5 predicted.** The cause is the 0.5 dB design margin, a finer tone grid, and S6 being designed exactly at 2.0. At r′ 1.99 the edge is 8.33 kHz instead of 9.1 kHz. At r′ 1 and 1.414 it is 0.34 and 0.43 kHz wider than predicted.
3. **Sets S1–S7 also cover 10–11 kHz input on the decimated levels.** That band is in the half-band transition. Without it, S1 leaves a −19 dB alias at 19.4–20 kHz from +12 st up. Covering it costs S1 2.3 kHz of passband.
4. **S7 is designed at r′ 2.83.** It holds −60 dB up to 2.83 and falls to about −20 dB by 3.99.
5. **The alias-gate engine rows drive GrainScatter's grain DSP classes directly.** That is the only way to force Doppler in both trees. One extra row runs end to end through `processBlock`.
6. **The frozen row avoids the loop seam.** The equal-power seam crossfade adds about +3 dB on correlated audio in both versions. This is an existing issue and out of scope.
7. **Snapshot positions use an exact integer + fraction split instead of per-level double origins.** This is what makes snapshot reads bit-identical to live reads.

### Issues Encountered
- **Known limits** (documented in the CHANGELOG and the module README):
  - At r′ 2.83–4 on a decimated level, rejection is best effort, about −38 dB. Reaching that range needs Doppler above 1.41 near the top of an octave.
  - Doppler below about 0.9 on a grain at +12 st or higher lets 10–11 kHz content image in at up to −30 dB.
  - Normal trajectory motion keeps Doppler within ±1 %, so both only happen on trajectory jumps.
- **Staging slip, already undone.** One `git rm --cached` was run on `LagrangeInterpolation.h`. It was undone right away with `git restore --staged`, and the file was then deleted with a plain `rm`. Nothing is staged.
- **SUMMARY.md author.** The execute agent could not write report files, so the orchestrator wrote this file from the agent's returned report.

## Build Status
- VST3 + AU: PASS, with no new warnings
- grain-engine module tests: 38/38
- pluginval at strictness 10: SUCCESS
- auval: PASS
- Installed `-dev` bundles

## Quick Test Results
- alias-gate: ALL PASS on 3.3.0. 3.2.0 fails the stereo, spatial and frozen rows.
- blocksize-null: ALL PASS, with its negative control confirmed.
- density-gain: ALL PASS, with its negative control confirmed.

## Ready for Verification
Human checks for the verify phase:
1. **Passband swing across the octave boundary.** Listen across +11 → +13 st (8.3 kHz at +11.9 st vs 16.2 kHz at +12 st). If the step is too audible, raise `blr::kTaps` to 24.
2. **Freeze in the Standalone.** Engage, release and re-engage at +0, +24 and +36 st. Expect no click, no silence and no stale read. Capture now ends about 10 ms before the engage.
3. **Factory presets** load unchanged in the Standalone.
4. **Unity pitch.** A/B against 3.2.0.
