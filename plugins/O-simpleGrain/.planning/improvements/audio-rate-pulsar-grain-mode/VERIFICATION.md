# Verification: Audio-Rate Pulsar Grain Mode

**Plugin:** O-simpleGrain
**Milestone:** audio-rate-pulsar-grain-mode
**Created:** 2026-10-29
**Phase:** Verify

> `VERIFICATION.gates-session-90c8.md`, beside this file, comes from a parallel verify session. It was written **before** the I-1 and I-2 fixes below. Its "snap-gain control" and its mode-switch design predate the fix. This file supersedes it.

---

## Verification Summary

**Status:** PASSED, after one fix round. The first pass re-ran every gate on a fresh build, and all of them passed. An independent code review then found two defects that the gates could not see, I-1 and I-2. Both are now fixed, and every gate has been re-run with all controls rebuilt on the final tree.
**Version:** 1.9.0 → 2.0.0, bumped in CMakeLists.txt with VERSION_CODE 0x020000. The installed bundle reports 2.0.0.

---

## Requirements Verification (CONTEXT.md success criteria)

| # | Criterion | Status | Evidence (final tree) |
|---|---|---|---|
| 1 | Cloud and pre-2.0.0 sessions bit-exact vs 1.9.0 | ✓ PASS | `null_v190.py` against the installed build: GATE PASS, run again after the version bump |
| 2 | f_p within 0.1 cent; formant moves independently | ✓ PASS | pulsar `gate.py`: fp 5/5, formant 3/3 |
| 3 | No formant above the ceiling; alias ≤ −60 dB | ✓ PASS | alias 13/13; the ceiling-0.45 control fails |
| 4 | BH crossfade with no level step; sidelobes better than Hann | ✓ PASS | bh-band 2/2, bh-sidelobe 2/2 |
| 5 | Pulsar block-size null at 32 / 441 / 7 vs 512 | ✓ PASS (static params) | blocksize-null GATE PASS, run again after the bump. See I-3 for the automated case |
| 6 | Mask f_p/N, Skip, FM sidebands, Glisson | ✓ PASS | mask, skip, fm-line, fm-inert, glisson and the new fm-crossing all PASS, against the rebuilt absent control |
| 7 | 0 allocations in processBlock; CPU at full polyphony | ✓ PASS | Harness P5: 0 allocations. CPU measured at 8 voices, which is `kMaxVoices`. Voice steal still allocates in both modes; this predates the milestone and is documented. |
| 8 | UI with EN/FR/zh; i18n and label gates | ✓ PASS | check-i18n ALL PASS · fr-lint `--strict` CLEAN · zh-lint 0 findings · check-ui-labels ALL PASSED. No UI edits since. |
| 9 | Build with no new warnings; pluginval 10; auval; persistence | ✓ PASS | 10 `-Wfloat-equal` warnings, all on lines from earlier commits, so 0 new · pluginval strictness 10 SUCCESS · auval SUCCEEDED (run again after the bump) · persist_test 0 FAIL (run again after the bump) |
| — | Mid-note mode switch | ✓ PASS (after I-1) | P6 is exact against a model where each kind keeps its own bus gain. A shared-gain control fails it. |

---

## Issues found by the first pass, both fixed

### I-1 (medium-high): a Cloud→Pulsar switch swelled the draining Cloud grains — FIXED
- **Cause:** `modeGain = Pulsar ? 1 : normGain` was applied to the whole bus through `outputGain`. On a switch it ramped up to 1 over 20 ms underneath the Cloud grains still sounding.
  - P6 measured a 6 dB swell at overlap 4. A dense cloud would swell by about 14 dB or more.
  - The original P6 model included the same ramp, so it did not catch this.
- **Fix:** voices write pulsarets into a processor-owned `pulsarBus`.
  - The bus is sized 2 × max(block, 8192) in prepareToPlay. A larger host block sends the excess to the main bus.
  - Pulsarets take only the output trim, through `pulsarGain`. Its 20 ms ramp advances per sample in every block, used or not.
  - The main bus keeps `out × normGain` in both modes, so the Cloud path is unchanged.

### I-2 (medium): the FM crossing solver drifted at high modulator rates — FIXED
- **Cause:** `fmCrossingDelta` ran 3 plain Newton steps with a clamp. Errors reached 0.07 sample at Δm 0.4 and 0.7 sample at Δm 0.8, for example FM Ratio 8 above f_p ≈ 1.2 kHz at 48 kHz.
- **Fix:** a bracketed Newton that falls back to bisection when a step leaves the bracket.
  - Tolerance is 1e-9 sample. It takes at most 50 steps, about 8 on average.
  - It is now public static so the harness can test it.

### Noted, not fixed
- **I-3 (low):** while Formant or FM Depth is automated, their targets are set once per block, so the ramp shape depends on the host block size. This matches Cloud. With static parameters the output is bit-exact.
- **I-4 (info):** the sine δ phase offset ignores glisson inside the sub-sample. Negligible.
- **fm-inert at top keys:** ratio 1 at MIDI 100 measures −72 dB against no-FM, versus −121 dB at MIDI 57. Old and new solvers give identical numbers. The cause is a constant onset shift under the analysis window, not jitter. Inaudible.

---

## Test changes

### P6 mode-switch
- The reference is now old-mode tail + new-mode note, each on its own bus gain, with no ramp.
- The snap-gain control is replaced by `ctlSharedGain.py`, which re-applies the pre-fix single bus gain to the current tree, so the control runs the new P6.

### P7 fm-crossing (new, `pulsar_checks.cpp`)
- `GrainVoice::fmCrossingDelta` is checked against a 200-step bisection over seeded cases with Δm 0.005–1, depths 0.5 and 0.95, and ratios 0.25–8. Bound: 1e-7 sample.
- An in-harness copy of the pre-fix 3-step Newton is the control, and it must fail.
- In `gate.py` this check is `fm-crossing`.
- A spectral version (`fm-highdm`, ratio 8 at MIDI 100) was tried first and dropped. Old and new builds both read −67.5 dB: with an integer ratio, every onset sits at the same modulator phase, so a solver error is one constant shift and the spectrum cannot see it.

### ctlAbsent.py
- The patch anchor is updated for the new solver signature.

---

## Final gate run (fresh install, all controls rebuilt on the final tree)

| Gate | Result |
|---|---|
| `tests/pulsar-gate/null_v190.py` | GATE PASS (also re-run after the bump) |
| `tests/blocksize-null/gate.py` | GATE PASS, including 3 Pulsar configs (also re-run after the bump) |
| `tests/pulsar-gate/gate.py` | PULSAR GATE PASS, 14 groups |
| ↳ mode-switch | −600 dB (exact) / −159.7 dB. Shared-gain control: −8.6 / −30.1 dB, fails. Snap-grain control: −17.2 / +16.3 dB, fails. |
| ↳ fm-crossing | Worst 1.0e-9 sample over 52 246 cases. Pre-fix Newton control: 0.70 sample, fails. |
| Render harness | Pulsar checks P1–P7 ALL PASS. The 2 known failures (`window-rect-clicks`, `tukey-between-rect-hann`) also fail on the 1.9.0 harness. |
| `tests/state-persistence/persist_test.py` | 0 FAIL (also re-run after the bump) |
| pluginval strictness 10 (`--skip-gui-tests`) | SUCCESS |
| `auval -v aumu OsGr OuDv` | AU VALIDATION SUCCEEDED (also re-run after the bump) |

## Still needs a person in a DAW
- Host automation round-trip of the 7 new parameters.
- Dimming state after a preset load and after reopening a session.
- Real knob angles; FM Ratio 0.5× should sit at about 11 o'clock.
- A listen to a mid-note Cloud↔Pulsar switch on a dense cloud: the I-1 case, now with no swell expected.
- A listen to the Pulsar lessons.
- The existing 1.x DAW checklist and the v1.9.0 preset/library hands-on are still pending. They are independent of this milestone.

---

*Generated by improve-milestone verify phase*
