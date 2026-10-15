# O-SimpleReverb — reverb engine rewrite (brief)

**Date:** 2026-09-30
**Baseline:** v1.14.0
**Target:** v2.0.0 (parameter IDs, ranges and state format unchanged; every session sounds different)
**Source:** `/improve` Phase 0.5 investigation. Findings are from reading the plugin and JUCE source; no impulse responses were rendered or measured.

## Problem

"The reverb doesn't sound very good." There are no per-type models: all six types run through one
stock `juce::dsp::Reverb` (Freeverb). TYPE only changes pre/post processing around it
(`Source/PluginProcessor.h:144`, `Source/PluginProcessor.cpp:648-660`).

## Findings

1. **Identical resonances in every type.** Freeverb = 8 parallel combs with fixed tunings
   1116..1617 samples @ 44.1 kHz (25–37 ms) + 4 fixed allpasses (556/441/341/225)
   (`juce_Reverb.h:104-105`). Booth and Ambient ring at the same frequencies.
2. **SIZE does not change size.** `roomSize` only sets comb feedback, `roomSize * 0.28 + 0.7`
   (0.70..0.98, `juce_Reverb.h:214-222`). SIZE and DECAY are the same control; no delay length moves.
3. **Booth cannot be short.** Feedback floors at 0.70 on ~30 ms loops → shortest tail roughly
   0.5–0.7 s (computed, not measured), with 30 ms flutter.
4. **Mono tank.** Input is `(left + right) * gain` (`juce_Reverb.h:152`); the right channel is the
   same network offset by 23 samples. Weak stereo; also why the v1.13.0 L/R flutter offset flanged.
5. **Early reflections are not reflections.** The 4 taps (7/11/17/23 ms × scale) are summed into the
   reverb INPUT (`PluginProcessor.cpp:593-606`) and never reach the output — a comb EQ on the tank
   feed. Their 7 % L/R offset is lost in the mono sum.
6. **Static tank.** Flutter sits before the reverb (`PluginProcessor.cpp:614-627`); nothing inside
   the loop moves, so long tails (Hall, Ambient, feedback ~0.98) ring metallically.
7. **Crude decay control.** One one-pole damper per comb, no separate low/high decay time, DECAY not
   calibrated in seconds.
8. **Spring is not a spring.** Three allpasses of 1.5–3.7 ms (`PluginProcessor.h:173`) cannot make a
   dispersive chirp.
9. **Plate is not a plate.** Freeverb + high shelf; shimmer is a one-shot octave-up on the input,
   not in the feedback path.

## Approved direction

Replace the Freeverb core with three engines behind the same 8 parameters and the same UI.

| Types | Engine |
|---|---|
| Booth, Room, Hall, Ambient | 8-line FDN: input allpass diffusion, per-type delay set scaled by SIZE, modulated lines, two-band decay, true stereo in/out, real stereo early-reflection tap line mixed to the OUTPUT |
| Plate | Dattorro figure-8 plate tank; shimmer optionally inside the loop |
| Spring | Dispersion allpass cascade inside a feedback delay, 2–3 springs slightly detuned |

- DECAY becomes a calibrated tail-length multiplier on each type's base decay time.
- Keep: TYPE duck-and-swap, type EQ crossfade, CHARACTER, LOW CUT, wet trim (re-measured), metering.
- CPU: expected same class as now (Freeverb already runs 24 delay lines) — confirm by measurement.

## Suggested staging

1. FDN engine (Booth/Room/Hall/Ambient) + real early reflections
2. Plate engine
3. Spring engine
4. Re-voice the 48 factory presets, re-measure wet trims, re-anchor `tests/render-check` gates

## Constraints / risks

- Sessions and presets must still load (no ID, range or state-format change).
- All 48 factory presets need re-voicing; insert presets stay at or below +5 dB re input.
- RT safety: no allocation in `processBlock`; delay capacities sized from the running sample rate.
- Engine switch on TYPE change must stay click-free (existing duck-and-swap).
- Baseline measurements (IR, tail length, stereo correlation, CPU) of v1.14.0 should be captured
  first so the improvement is demonstrated, not asserted.
- Reference: `research/reverb-comprehensive-research.md` (§2.4 FDN, §2.5 Dattorro).

## Rejected alternative

Patch around Freeverb (ERs to output, front allpass diffusion, decorrelate R). Modest gain; fixed
combs, fake SIZE, fake Spring and Plate all remain.
