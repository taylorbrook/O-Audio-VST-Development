# Context: Reverb engine rewrite

**Plugin:** O-SimpleReverb
**Milestone:** reverb-engine-rewrite
**Created:** 2026-09-30
**Phase:** Discuss

---

## Original Request

> Replace the Freeverb core with purpose-built engines (FDN for Booth/Room/Hall/Ambient, Dattorro
> plate, dispersive spring) per plugins/O-SimpleReverb/improvements/reverb-engine-rewrite.md —
> target v2.0.0

The brief (`plugins/O-SimpleReverb/improvements/reverb-engine-rewrite.md`) is the source of the
nine findings and the approved direction. This document adds the decisions the brief left open.
Where the two disagree, this document wins.

---

## Requirements Gathered

### Core Requirements

1. **FDN engine for Booth, Room, Hall, Ambient**
   - Description: 8-line feedback delay network replacing `juce::dsp::Reverb`. Input allpass
     diffusion, a per-type delay set scaled by SIZE, modulated lines inside the loop, two-band
     decay (separate low/high decay time), true stereo in and out.
   - Acceptance criteria: the four types have different modal structure (delay sets differ, so
     their IRs do not share resonant frequencies); L/R tail is decorrelated without a fixed
     sample offset; no metallic ringing in Hall/Ambient long tails.

2. **Real early reflections at the output**
   - Description: a stereo early-reflection tap line mixed to the OUTPUT, not summed into the
     tank feed. Tap times scale with SIZE.
   - Acceptance criteria: discrete L/R-different taps are visible in the IR before the late
     tail, per type; removing the late tail leaves the taps audible.

3. **Dattorro plate engine for Plate**
   - Description: figure-8 plate tank. Shimmer moves INSIDE the loop as an octave-up voice at a
     fixed low level, capped so it blooms gently and never cascades into an obvious
     shimmer-reverb effect.
   - Acceptance criteria: fast echo-density build-up with no discrete early taps; octave energy
     grows over the tail rather than appearing once at the onset; loop gain including the
     shifter stays below 1 at every DECAY/SIZE setting (no runaway).

4. **Dispersive spring engine for Spring**
   - Description: dispersion allpass cascade inside a feedback delay, 2–3 springs slightly
     detuned. Voicing target is a **guitar-amp tank**: audible chirp/drip on transients,
     band-limited (roughly 100 Hz – 5 kHz).
   - Acceptance criteria: an impulse produces a visible downward-or-upward frequency chirp per
     echo in a spectrogram (frequency-dependent group delay), repeating at the spring's
     round-trip time; this is absent in v1.14.0.

5. **DECAY calibrated in seconds**
   - Description: DECAY (0.5x–2.0x, range and skew unchanged) multiplies a per-type base RT60.
     Bases at DECAY 1.0x, SIZE 50:

     | Type | Base | 0.5x | 2.0x |
     |---|---|---|---|
     | Booth | 0.40 s | 0.20 s | 0.80 s |
     | Room | 1.1 s | 0.55 s | 2.2 s |
     | Hall | 3.0 s | 1.5 s | 6.0 s |
     | Spring | 2.5 s | 1.25 s | 5.0 s |
     | Plate | 2.5 s | 1.25 s | 5.0 s |
     | Ambient | 7.0 s | 3.5 s | 14 s |

   - Acceptance criteria: measured mid-band RT60 at DECAY 0.5/1.0/2.0x lands within a stated
     tolerance of the table for every type (tolerance set in research; ±10 % suggested).
     `getTailLengthSeconds()` is raised from 10.0 to cover Ambient at 2.0x (14 s).

6. **SIZE changes size, and only size**
   - Description: SIZE scales delay lengths, early-reflection spacing and (where the type uses
     it) pre-delay. It does not change tail time — RT60 at a given DECAY is held as SIZE moves,
     which means the per-line feedback gain is recomputed from the line length.
   - Acceptance criteria: sweeping SIZE 0→100 at fixed DECAY moves the modal density / first
     reflection time measurably while RT60 stays within the same tolerance as requirement 5.

7. **SIZE moves glide**
   - Description: when SIZE changes while audio rings, delay lengths slew smoothly, so the tail
     bends in pitch briefly. No crossfade engine, no second read-tap set.
   - Acceptance criteria: a SIZE sweep on a ringing tail is click-free (same excess-step gate
     style as the existing CHARACTER/LOW CUT checks); a pitch bend is expected and is not a
     failure.

8. **Compatibility**
   - Description: no parameter ID, range, skew, default or state-format change. The 8 parameters
     and the UI stay as they are. v1.x sessions and user presets load and recall the same
     parameter values; they sound different, which is the point of the MAJOR bump.
   - Acceptance criteria: `params.tsv` is byte-identical before and after; a v1.14.0 state blob
     loads into v2.0.0 with every parameter equal.

9. **Factory bank re-voiced**
   - Description: all 48 presets re-voiced for the new engines, keeping names, TYPE, the
     8-per-type structure and one WET 100 / DRY 0 Send per type. Per-type wet trims re-measured.
   - Acceptance criteria: insert presets at or below +5 dB re input (K-weighted pink noise, as
     now); type-to-type level spread at defaults back within the v1.12.0 figure (0.07 dB target,
     tolerance set in research); render-check section 9 passes on the new bank.

10. **Baseline first**
    - Description: before any engine code changes, capture v1.14.0 measurements — IR, RT60,
      stereo correlation, CPU — per type, so the improvement is demonstrated, not asserted.
    - Acceptance criteria: a baseline table exists in the milestone directory and
      VERIFICATION.md compares v2.0.0 against it row by row.

### Scope Boundaries

**In scope:**
- Three new engines and the removal of `juce::dsp::Reverb` from the plugin.
- Early reflections rebuilt as an output-mixed stereo tap line.
- DECAY calibration, SIZE semantics, `getTailLengthSeconds()`.
- Re-voicing the 48 factory presets and re-measuring wet trims.
- Re-anchoring `tests/render-check` gates and adding gates for the new acceptance criteria.
- CHANGELOG, NOTES.md, PLUGINS.md row, version 2.0.0.

**Kept as they are (must not regress):**
- TYPE duck-and-swap and type-EQ crossfade (click-free engine switch).
- CHARACTER (TPT SVF warm + always-on bright shelf, smoothed).
- LOW CUT (always-running SVF, crossfaded on/off).
- Wet/dry smoothing, VU metering, mono/stereo bus constraint.
- Preset manager behaviour, factory sentinel, i18n (en / fr / zh-Hans).

**Out of scope:**
- New parameters, new types, UI or layout changes, a seconds readout for DECAY.
- A legacy/Freeverb compatibility mode for v1.x sessions.
- Surround layouts.
- Extracting the engines into a shared module (possible follow-up; not this milestone).
- Staged intermediate releases — nothing is tagged before all three engines and the re-voiced
  bank are done.

---

## User Preferences

### Interaction Style
- [x] Real-time parameter response
- [x] Preset-based switching
- [x] Automation-friendly (SIZE glides; TYPE ducks and swaps)

### Quality Priorities
1. **Sound quality over CPU.** No CPU ceiling: "as much as needed for high quality". This
   supersedes the original creative brief's "CPU efficiency over complexity". CPU is measured
   and reported against the v1.14.0 baseline, but it is not a pass/fail gate.
2. Click-free behaviour on every control that is click-free today.
3. Level consistency across types and presets.

### Breaking Changes
- [ ] OK to break existing presets
- [x] Must maintain preset compatibility (values load; sound changes by design)
- [x] Must maintain automation compatibility (IDs and ranges unchanged)

---

## Plugin Context

### Current State
- **Version:** 1.14.0
- **Stage:** 📦 Installed
- **Last change:** v1.14.0 — factory bank reviewed, 24 → 48 presets, insert presets level-capped.

### Relevant Existing Features
- `Source/PluginProcessor.h:144` — the single `juce::dsp::Reverb` every type runs through.
- `Source/PluginProcessor.cpp:593-660` — early reflections into the tank feed, flutter before
  the reverb, the per-type pre/post chain.
- `TypePreset` table (`PluginProcessor.cpp`, `typePresets[6]`) — per-type pre-delay, ER scale,
  modulation, EQ and `wetTrimDb`. Most fields are Freeverb-shaped and will be replaced.
- `Source/ModulationFx.h` — `FlutterDelay` and `OctaveUpShifter` (two-grain). The shifter is the
  candidate for the in-loop plate shimmer; the pre-reverb flutter becomes redundant once lines
  are modulated in the loop.
- `tests/render-check/main.cpp` — 32/32 at v1.14.0. Many gates are anchored to Freeverb
  behaviour (tail lengths, level trims, preset levels) and need re-anchoring, not re-recording
  blind.
- `research/reverb-comprehensive-research.md` — §2.4 FDN, §2.5 Dattorro.

### Known Limitations
- No impulse responses were rendered for the brief; findings 3 and the CPU expectation are
  computed, not measured. Requirement 10 closes that.
- The v1.14.0 voicings were chosen by parameter range and level, not by ear, and have not been
  checked in a DAW. The re-voiced bank needs a listening pass before the tag.
- `OctaveUpShifter` was written for a one-shot feed. Inside a feedback loop its latency, grain
  artefacts and gain all recirculate — research must confirm it is stable and clean there.
- `.planning/STATUS.md` is stale (it still describes Stage 1 from 2026-01-13). Only
  `activeMilestone` was added; the rest was left alone.

### Project patterns that apply (from memory)
- Delay capacity must be sized from the running sample rate, and a host block larger than the
  prepared block must not overrun (chunk the core).
- A bare `pushSample` shifts a `DelayLine`'s delay; pop-then-push is exact.
- `SmoothedValue::reset()` does not clear a NaN; a NaN guard that latches produces sticky
  silence. Feedback loops here need a guard that recovers.
- Random start phase seeded from `this`, or a clock-seeded `Random`, breaks render-diff
  determinism — line modulation phases must be deterministic.
- Clamped float ramp rails and noise levels are sample-rate dependent — check 44.1/48/96 kHz.
- Diffusion slows decay per second, not per generation — matters for hitting the RT60 table.
- Presets never set output gain; factory preset gates can read a stale installed bank unless
  the sentinel is cleared first (render-check already does this).

---

## Questions Answered

**Q1: How should v2.0.0 ship?**
> One release. All three engines and the re-voiced bank land together; execute runs in the
> brief's four stages, each gated by render-check, with no tag until the end.

**Q2: Base tail length per type at DECAY 1.0x, SIZE 50?**
> "Longer / lusher": Booth 0.40 s, Room 1.1 s, Hall 3.0 s, Spring 2.5 s, Plate 2.5 s,
> Ambient 7.0 s. SIZE moves delay lengths and ER spacing only, not tail time.

**Q3: Plate shimmer?**
> In the loop, subtle — fixed low level, capped so it never cascades.

**Q4: Spring voicing target?**
> Amp tank — audible chirp/drip, band-limited, 2–3 detuned springs.

**Q5: CPU ceiling against v1.14.0?**
> "doesn't matter - as much as needed for high quality"

**Q6: What should the tail do when SIZE moves while audio rings?**
> Glide — delay lengths slew, the tail bends in pitch briefly.

---

## Open for Research

- FDN delay sets per type (mutually prime, Booth…Ambient), feedback matrix (Householder vs
  Hadamard), modulation depth/rate per line, and how two-band decay is realised per line.
- How to hold RT60 constant while SIZE scales line lengths, including during a glide.
- Dattorro tank constants at arbitrary sample rate, and where the shifter sits in the figure-8.
- Spring: dispersion stage count and coefficient for an amp-tank chirp, detune between springs,
  band-limiting; cost at 96 kHz.
- In-loop `OctaveUpShifter` stability and artefacts; level cap that keeps loop gain < 1.
- RT60 measurement method and tolerance for the render-check gates (Schroeder integration,
  band, fit range).
- Which render-check gates re-anchor, which are replaced, and which new ones are added.
- Engine-switch behaviour: whether inactive engines keep running, are cleared at the duck's
  silent point, or ring out.
- Mono-in/mono-out path for each engine.

---

## Success Criteria

The improvement is successful when:

1. [ ] `juce::dsp::Reverb` is gone; Booth/Room/Hall/Ambient run the FDN, Plate the Dattorro
       tank, Spring the dispersive spring.
2. [ ] Measured RT60 matches the base table × DECAY for all six types, and holds across SIZE.
3. [ ] Early reflections appear at the output, different in L and R.
4. [ ] Spring shows a dispersive chirp; Plate shimmer builds inside the tail and never runs away.
5. [ ] TYPE, SIZE, DECAY, CHARACTER and LOW CUT moves are click-free.
6. [ ] `params.tsv` unchanged; a v1.14.0 state loads with equal parameter values.
7. [ ] 48 presets re-voiced; insert presets ≤ +5 dB re input; type level spread re-trimmed.
8. [ ] v1.14.0 baseline captured and compared row by row (IR, RT60, stereo correlation, CPU).
9. [ ] No allocation in `processBlock`; stable at 44.1/48/96 kHz and with oversized host blocks.
10. [ ] render-check passes with re-anchored and new gates; the v1.14.0 build fails the new ones.
11. [ ] Build succeeds without warnings.
12. [ ] Pluginval passes (Level 5+); `auval -v` passes.
13. [ ] Listening pass in a DAW on the re-voiced bank before the tag.

---

## Next Phase

This context document feeds into the **Research** phase, which will:
- Investigate implementation approaches
- Find relevant patterns in codebase
- Assess complexity and domain
- Identify affected files

---

*Generated by improve-milestone discuss phase*
