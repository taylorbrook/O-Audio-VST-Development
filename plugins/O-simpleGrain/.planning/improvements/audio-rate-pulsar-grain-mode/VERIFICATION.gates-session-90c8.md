# Verification: Audio-Rate Pulsar Grain Mode

**Plugin:** O-simpleGrain · **Milestone:** audio-rate-pulsar-grain-mode · 1.9.0 → 2.0.0
**Verified:** 2026-10-29 · **Verdict:** PASS (automated). Some hands-on DAW items are still open; they are listed at the end.

## Method

Every gate was re-run in a fresh session against the working tree:
- The tree was rebuilt with `build-and-install.sh`, which installed `O-simpleGrain-dev` after clearing the cache and sweeping the dual variants.
- The render harness was rebuilt from the tree.
- The Pulsar gate ran against the executor's out-of-tree control builds (`c1_gate.sh`). Each control must fail its bound.

The executor's numbers in SUMMARY.md were not reused; every result below comes from this session's run. Logs are in the verify session's scratchpad, `v/`.

The source was also read against the requirements:
- **Parameters:** 7 are appended with version hint 2, and the first 23 are unchanged.
- **Old sessions:** the `setStateInformation` migration list covers all 7.
- **Old presets:** preset-manager WR-01 resets every parameter to its default before applying a preset, so a pre-2.0.0 preset loads as Cloud.
- **Level:** `modeGain` leaves Cloud's `normGain` untouched.
- **Real-time safety:** the BH table is built in the `WindowLuts` constructor. The added voice code has no allocating calls.

## Requirements → evidence

| # | Requirement | Result | Evidence |
|---|---|---|---|
| 1 | Cloud and pre-2.0.0 state bit-exact vs 1.9.0 | ✅ | `null_v190.py`, installed build vs `ref190`: GATE PASS |
| 2 | f_p = key ± 0.1 cent | ✅ | pulsar gate `fp` PASS at 44.1 / 48 / 96 kHz; the +1 cent control fails |
| 3 | Formant moves the centroid, not f_p | ✅ | `formant` PASS |
| 4 | Nyquist clamp, alias ≤ −60 dB | ✅ | `alias` PASS: Sine, Sample, FM + Glisson sweep and Pitch Spray band; the ceiling-0.45 control fails |
| 5 | Sine / Sample content, Sine never reads the source | ✅ | `sine-no-src` PASS; harness P2 / P4 |
| 6 | BH auto crossfade under ~1 ms | ✅ | `bh-band` PASS (no level step); `bh-sidelobe` PASS; the no-BH control fails |
| 7 | Fractional onsets, block-size null | ✅ | `frac-onset` PASS; the integer-onset control fails. `blocksize-null/gate.py` GATE PASS, Pulsar configs included |
| 8 | Mask f_p / N, Skip per pulse | ✅ | `mask` and `skip` PASS; the code-absent control fails |
| 9 | Rate FM sidebands; depth 0 neutral | ✅ | `fm-line` and `fm-inert` PASS (integer ratios inert by design, so the default is 0.5) |
| 10 | Glisson glide | ✅ | `glisson` PASS |
| — | Click-free mode switch | ✅ | `mode-switch` 3/3 PASS (harness P6); the snap-grain and snap-gain controls fail |

## Success criteria

| # | Criterion | Result |
|---|---|---|
| 1 | Cloud bit-exact vs 1.9.0 | ✅ |
| 2 | f_p within 0.1 cent; Formant independent of f_p | ✅ |
| 3 | No formant above the ceiling; alias ≤ −60 dB | ✅ |
| 4 | BH crossfade with no level step; sidelobes improved | ✅ |
| 5 | Pulsar block-size null | ✅ (bit-exact) |
| 6 | Mask, Skip, FM, Glisson behave as specified | ✅ |
| 7 | 0 allocations in Pulsar; CPU measured | ✅ (harness P5; CPU per SUMMARY). The voice-steal allocation predates this milestone and is documented. |
| 8 | UI i18n gates | ✅ `check-i18n` ALL PASS · fr lint `--strict` CLEAN · zh lint 0 findings (54 entries at `mt`) · `check-ui-labels` ALL PASSED |
| 9 | Build, pluginval, auval, state persistence | ✅ 0 new warnings · pluginval strictness 10 SUCCESS · auval SUCCEEDED · persist 34 PASS / 0 FAIL, including case 8 (all 7 Pulsar params) |

**Re-check after the version bump.** CMakeLists is now 2.0.0 / 0x020000. The bundle was rebuilt and reinstalled, and auval reports Component Version 2.0.0 and VALIDATION SUCCEEDED. pluginval at strictness 10 reports SUCCESS.

## Open: needs a person in a DAW
- Host automation round-trip of the 7 new parameters (Logic / Live).
- Dimming state after a preset load and after reopening a session.
- Real knob angles in the Standalone or a DAW. The headless stub could not show these; for example, FM Ratio 0.5× should sit at about 11 o'clock.
- A listen to the Pulsar Vowel and Subharmonic Mask lessons.
- The existing 7-item 1.x DAW-listen checklist and the v1.9.0 preset/library hands-on are still pending. They are independent of this milestone.
