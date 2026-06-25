---
plugin: O-simpleBeatmaker
stage: ideation
status: creative_brief_complete
last_updated: 2026-06-25 12:30:00
---

# Resume Point

## Current State: Creative Brief Complete

Creative brief has been finalized for O-simpleBeatmaker. Ready to proceed to Stage 0 planning (DSP research + architecture).

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined (pedagogical TR-808/909-style step-sequencer drum machine for MUSC319 wk09)
- Four architectural forks resolved (see Key Decisions)
- Parameters specified (sequencer/timing-feel core + minimal per-voice + master)
- UI vision captured (step grid + timing lane + MIDI readout + tooltips, single projector-readable page)
- Use cases identified
- Requirements extracted with acceptance criteria (24 reqs)

## Next Steps

1. Start planning / DSP research (`/plan O-simpleBeatmaker`) — recommended
2. Create UI mockup first (`/start O-simpleBeatmaker` → option 3)
3. Research similar plugins for inspiration

## Context to Preserve

**Key Decisions (the four resolved forks):**
- **Sound source:** Synthesized 808/909-lineage voices (no samples, transparent/see-inside).
- **Tracks:** Drums only (no bass/melodic lane — A3 bassline done in the DAW).
- **Play model:** Internal step sequencer synced to host transport **+** MIDI-playable voices (teaches "step grid & piano roll = two views of one MIDI stream").
- **Timing feel:** Timing/groove lane + quantize-strength (swing/humanize push hits off-grid, quantize-strength pulls them back — full quantize-vs-feel tradeoff, made visible).

**Pedagogical framing:** Sibling to O-simpleFM / O-simpleAdditive / O-simpleGrain / O-simpleSubtractive — same DNA (gesture→visible consequence, tooltips, concept-isolating presets, single page), but subject is MIDI sequencing & groove, not a synthesis engine. Class source: `/Users/taylorbrook/Documents/UBC/Courses/MUSC319/2026 term 1/out/wk09-mon-midi-beatmaking.html`.

**Proposed drum roster (confirm in research):** Kick, Snare, Clap, Closed Hat, Open Hat, Tom (~6 voices).

**Open questions for Stage 0 research:** 808-vs-909 flavor per voice; exact swing curve + 8th vs 16th swing; humanize distribution (timing vs velocity split) and how quantize-strength composes with swing; sample-accurate sub-step trigger scheduling; GM drum note map; per-voice polyphony/tail handling; whether to expose an internal free-run tempo for standalone.

**Files Created:**
- plugins/O-simpleBeatmaker/.planning/BRIEF.md
- plugins/O-simpleBeatmaker/.planning/REQUIREMENTS.md
- plugins/O-simpleBeatmaker/.planning/STATUS.md
