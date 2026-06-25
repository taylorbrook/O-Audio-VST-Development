---
plugin: O-simpleSubtractive
stage: ideation
status: creative_brief_complete
last_updated: 2026-06-25 00:00:00
---

# Resume Point

## Current State: Creative Brief Complete

Creative brief has been finalized for O-simpleSubtractive. Ready to proceed to Stage 0 planning (DSP research + architecture) or a UI mockup.

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined (pedagogical subtractive synth: osc → filter → amp, two ADSRs)
- Architecture decided via 4 ideation questions
- Parameters specified
- UI vision captured (mirrors O-simpleFM / O-simpleAdditive)
- Use cases identified
- Requirements extracted with acceptance criteria

## Key Architecture Decisions (from ideation)

- **Voices:** 16-voice poly + Mono/Legato switch (teaches Minimoog mono AND Juno poly; "a polysynth is several subtractive voices in parallel")
- **Filter slope:** Full 6/12/24 dB/oct (1/2/4-pole) selectable + self-oscillation
- **Filter modes:** LP + HP + BP + Notch (state-variable)
- **Oscillator:** 1 waveform-selectable osc (saw/square/triangle/sine) + octave-down sub + white noise
- **Headline visual:** live filter-response curve overlaid on the oscillator's harmonic spectrum (the class's "before/after filter" figure, made live)
- **Pedagogical context:** MUSC319 wk06-wed-subtractive session; A2 activity = build + save a bass and a lead

## Next Steps

1. Start Stage 0 planning / DSP research (recommended): `/plan O-simpleSubtractive`
2. Create UI mockup to visualize design: `/start O-simpleSubtractive` → option 3
3. Research filter topology (Moog ladder vs TPT/SVF) before planning

## Context to Preserve

**Key Decisions:**
- Plugin type: Synth (Pedagogical Subtractive / Oscillator→Filter→Amp)
- Core concept: Strip subtractive synthesis to osc + filter + amp with one envelope each; gesture→visible-consequence; teaching instrument first

**Open research questions (Stage 0):**
- Filter topology: Moog ladder vs zero-delay TPT/SVF (which gives stable self-oscillation + all four modes + correct magnitude curves)
- Cutoff key-tracking, velocity routing, PWM, anti-aliasing strategy, self-osc gain compensation

**Sibling references:** O-simpleFM (1.2.1, installed), O-simpleAdditive (1.0.0, working), O-simpleGrain (Stage 2) — same pedagogical template.

**Files Created:**
- plugins/O-simpleSubtractive/.planning/BRIEF.md
- plugins/O-simpleSubtractive/.planning/REQUIREMENTS.md
- plugins/O-simpleSubtractive/.planning/STATUS.md
