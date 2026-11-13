# Stage 4: Polish / Validation - Context, Rework R2 (feedback holds the lane level)

## Discussion Summary

**Date:** 2026-11-12
**Participants:** User, Claude

Third discuss pass for Stage 4. R1 (`CONTEXT-R1.md`, D60–D69) was executed and verified PARTIAL; the D51 hands-on session on the R1 build came back 242 / 243 with one finding (`HANDS-ON-SESSION-4.md` triage F3, `VERIFICATION-R1.md` Human Verification). This document scopes the fix. D51–D69 stand unless amended below.

**F3:** with a stream's Feedback on, its lane is overwritten lap by lap and fades toward silence. It is the designed write law, not a regression: `GiRecordHead.h` writes `w = (1 − λ)·wc + λ·rw`, λ = 0.95 · Amount · run, and every stage of the return has gain ≤ 1 (tap gain `1 / (S1 + 1)`, amp-weighted mean over voices, HP / LP, diffuser mix, `tanh`, chain AGC and writer clamp at 0.7). The automated rows bound the loop from above only (FL1 / FL1p, CL1).

Already decided before this pass (Taylor, 2026-11-12): route (a), fix before 1.0.0; nothing is published, so no migration.

## Requirements Confirmed

- **A lane under feedback holds its level.** The lane keeps evolving (grain smear, filters, diffusion, saturation) but its loudness stays about where it started, for as long as feedback runs. Reset still restores the pristine copy.
- **Automatic, no control.** The makeup is internal. Parameter count stays 293, destinations stay 150, the FX · Feedback page does not change.
- **No runaway, as today.** The ±0.7 writer clamp and the convex write stay; FL1 / FL1p / CL1 keep passing unchanged.
- **A gate bounds the lane level from below**, with a negative control (makeup off → the row fails).
- **Factory bank:** the 15 presets with a Feedback Amount above 0 keep their values; each is rendered before / after, outliers are re-voiced, then all 15 are re-auditioned.
- VERSION stays 1.0.0; the CHANGELOG `## [1.0.0]` entry is amended.

## Constraints Identified

- Feedback 0 and the toggle Off remain a bitwise no-op on the working copy. Render goldens with feedback 0 stay byte-identical (all 42 golden lines are stream 1, fb 0, reverb 0).
- `forked/FeedbackChain.h` is a VERBATIM leaf (drift 0 / 13). The makeup should live in non-forked code (`Engine`, `GiRecordHead.h`); editing a forked leaf means a manifest re-pin and a recorded reason.
- The chain already has an AGC (ceiling 0.7, 1 ms attack / 400 ms release). A second level control in the same loop must not pump against it.
- Block-size invariance and determinism: every per-sample control is an `ExactRamp` formed at grid ticks; the makeup has to follow that rule (parity 168 / 0, PT rows).
- No allocation on the audio thread; CPU budgets unchanged (C25m ≤ 40 %, typical preset ≤ 12 %, now C25m 29.99 % / C25mPre 31.21 % / bank T max 9.13 %).
- The makeup gain is capped, so a silent or near-silent lane (empty Record-in tape, noise floor) is never amplified into audible noise.
- A gate whose stimulus sits below a threshold is vacuous (memory): the lower-bound row needs a stimulus that fades visibly on the R1 build.
- Contract checksums: `requirements` re-pins at verify (DSP-04, FUNC-08, PERF-02 evidence lines). `parameter_spec`, `params.tsv`, param-dump and destination goldens must not move.
- Trunk-based: all work on `main`, path-scoped commits (`plugins/O-GranulatorInstrument`, `PLUGINS.md`).

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D70 | Long-run behaviour | **Hold level.** Automatic makeup on the feedback return; the lane's loudness stays about where it started, indefinitely. | User choice over a long decay or a new per-stream Sustain control. No parameter, page or spec change. |
| D71 | Makeup scope | **Hold loudness after the whole chain, capped.** The return is matched to the lane's level after filters, diffusion and saturation, so the lane gets darker / narrower but not quieter. The makeup gain has a cap (research picks it, about +12 dB); filtering to nothing still fades. | User choice over compensating the grain / tap loss only. |
| D72 | Factory presets | **Re-audition, fix outliers.** Values kept; before / after renders of the 15 feedback presets; only those whose level or character moves a lot are re-voiced; Taylor re-auditions the 15 in Pass 2. | User choice over re-voicing all 15 or leaving the bank untouched. |
| D73 | Amount semantics (default, not asked) | Unchanged: λ = 0.95 · Amount is the write blend (how fast the lane is replaced). Amount 0 / toggle Off = no writes, bitwise. | Keeps every fb-0 golden and the meaning of existing preset values. |
| D74 | Reverb position (default, not asked) | The hold applies in both Post and Pre. In Pre the level is matched after tank B. | One behaviour per stream; D62 unchanged. |
| D75 | Lower-bound gate (default, not asked) | New harness rows: Amount 100 %, Saturate 0 / 20 / 100 %, default filters, 60 s, Post and Pre — lane RMS stays within a band of its starting level (proposed ±3 dB, research confirms). Negative control with the makeup off. FL1 / FL1p / CL1 unchanged. | F3 passed every gate because none looked downward. |
| D76 | Surface (default, not asked) | No new parameter, destination, UI element or string. No mockup, no i18n work. Freeze and Reset All unchanged. | Follows D70. |
| D77 | Release bookkeeping (default, not asked) | VERSION 1.0.0, CHANGELOG `[1.0.0]` amended, no tag (D56). Bank hash re-pins only if a preset is re-voiced. | As R1. |

## Open Questions (for research)

1. **Where the level goes.** Measure the per-lap loss on the R1 build and split it: tap gain `1 / (S1 + 1)`, mean over voices, incoherent grain averaging (spray, pitch, density, window), chain filters, saturation, AGC. How much does it vary across the 15 feedback presets?
2. **Level reference.** What the return is matched to: the content being overwritten at the head (running level of `wc`), a lap-integrated lane level, or the slot's pristine level. Which one holds without a random-walk drift, and which behaves on sparse material (silence gaps between hits)?
3. **Detector and placement.** Time constants, stereo link, where the makeup sits relative to the chain AGC and the writer clamp so the two do not pump; whether it is one-sided (gain ≥ 1 only) or also pulls a hot return down.
4. **Cap and silence.** The cap value; behaviour on an empty tape, on a noise floor, at the start of a Record-in take; gain wind-up and release when the lane goes quiet and comes back.
5. **Boundedness proof.** With makeup > 1 inside the loop, the writer clamp + convex write still give `tape peak ≤ max (pristine, 0.7)` exactly. Does the lane creep up toward the ceiling at Saturate 100 %?
6. **Invariance.** Block-size invariance, determinism, and which rows move: PT2 (16 at tolerance), FP rows, parity PB (`chainKnobParity`), `--cpu4`.
7. **Gate design.** Stimulus, duration, band and control for the D75 rows; confirm the same rows fail on the R1 tree.
8. **Pre mode.** Tank B adds energy and time smear in the loop; is "matched after tank B" stable at reverb Mix 100 % / Size 100 %?
9. **Outlier metric (D72).** A before / after number per preset (level over time, spectral centroid) that defines "moves a lot".
10. **CPU.** Cost of four makeup detectors against C25m / C25mPre and the bank's typical max.

## Out of Scope (R2)

- A user control for sustain / decay, or keeping the fading loop as an option.
- Any change to parameters, destinations, the FX page, strings or mockups.
- Windows hands-on, Reaper / Dorico / MPE passes (D54, D57 unchanged).
- Version bump or tag (D56).

## Retest after R2 (from F3 triage)

Hands-on rows 2.4-1 – 2.4-7, 3.2-10 / 3.2-11, 3.2-24, section 1d, and every Pass 2 preset with a Feedback Amount above 0 (the 10 showcase presets included).

## Next Phase

Ready for: research phase (`RESEARCH-R2.md`).
