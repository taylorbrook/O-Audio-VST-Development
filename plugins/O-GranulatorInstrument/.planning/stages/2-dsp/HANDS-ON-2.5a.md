# O-GranulatorInstrument — HANDS-ON-2.5a (human gate)

> **Superseded by [HANDS-ON-SESSION-4](../4-polish/HANDS-ON-SESSION-4.md) (2026-11-11).** Run that sheet; this one is kept for history.

**Build:** 0.1.0 dev (`O-GranulatorInstrument-dev`), Phase 2.5a engine (mod sources, the 16-slot matrix with per-voice evaluation, the newest-voice rule, Tempo sync), with the `DevLoaderEditor` (four Load buttons above the generic editor).
**Run in:** the Standalone **and** one DAW (Logic preferred; AU `aumu OuGI OuDv`).
**Not self-certified.** Mark each row ✅ / ❌ and add a note for every ❌.

> **Deferred with HANDS-ON-2.1 / 2.2 / 2.3 / 2.4.** On 2026-11-08 the user deferred all hands-on testing until the Stage 3 UI is in place. Run this sheet together with the earlier ones then. The generic editor exposes every matrix slot (`Mod N · Source / Destination / Depth`), but the Stage 3 matrix view makes rows 1–8 far quicker.

Defaults to know before you start:
- **Every slot starts at None / 0.** With no live route a patch sounds exactly as it did in 2.4 (proven bitwise: TB3 / TB4).
- **Depth is bipolar and works in the destination's normalised (knob) space.** Routes on the same destination add up before one clamp at the knob's ends.
- **Per-voice sources** (Mod Env 1–2, Velocity, Key, Note Random, Pressure, Slide, Bend, and an LFO in Per-voice mode) modulate each voice separately when the destination is a stream parameter. Routed to a **global** destination (Output Gain, Feedback, Reverb, an LFO rate while that LFO is Global), they follow the **newest sounding voice** through a 20 ms ramp, and the value holds after the last note ends.
- **Tempo (Sync Mode = Tempo):** grain onsets lock to the host's bar grid. In Tempo mode a note-on fires **one grain immediately**, then the grid takes over (user decision 2026-11-08). Under heavy load the grid thins by whole powers of two, so it stays on the beat.
- Pressure, Slide and Bend are non-MPE in 2.5a: channel aftertouch, CC74 and the pitch wheel reach every voice. MPE arrives in 2.5b.

| # | Row | How | Expected | Standalone | DAW | Notes |
|---|---|---|---|---|---|---|
| 1 | LFO shapes | LFO 1 → S1 Position, then S1 Level, then S1 Pitch, at depth ±50. Step through all 6 shapes at 0.5–8 Hz | Each shape moves the destination as named. S&H steps are clean, Smooth Random glides. No zipper noise | | | |
| 2 | Per-voice LFO | LFO 1 Mode = Per-voice → S1 Pan (or Level) on a held chord. Then switch Global ↔ Per-voice while holding | Each chord note moves on its own (the LFO restarts at each note-on). The mode switch has no click | | | |
| 3 | Velocity / Key / Note Random | Velocity → S1 Size, Key → S1 Filter Cutoff (Filter LP), Note Random → S1 Pan; play chords at mixed velocities | Each note gets its own size / brightness / position, independent of the others | | | |
| 4 | Mod Envs | Mod Env 1 → S1 Size and Mod Env 2 → S1 Filter Cutoff at depth +60; sweep A / D / S / R | The shapes follow the knobs. A knob edited mid-segment takes effect at the next stage | | | |
| 5 | Controllers | Mod wheel, channel aftertouch, CC74 and pitch bend each routed to S1 Level from a controller; then send Reset All Controllers (CC121) | Each controller moves its route. CC121 returns wheel / pressure / slide / bend to rest | | | |
| 6 | Macros | Macro 1–4 → four destinations; sweep by hand and by DAW automation | Smooth, with no stepping on fast moves (20 ms ramp) | | | |
| 7 | Newest voice | Velocity → Reverb Mix (depth +60); play legato phrases at varied velocities, then release everything | Reverb Mix follows the newest note smoothly and holds after the release | | | |
| 8 | Live route edits | While a route sounds, change its source, destination, depth and sign, and set source / dest to None | No clicks; every change crossfades over 20 ms | | | |
| 9 | Tempo sync | Sync Mode Tempo; S1 Density Sync and Size Sync at 1/16, 1/8 T and 1 bar in a DAW loop at 3 tempi; change tempo and jump the loop while notes sound | Grains stay locked to the beat. The note-on grain sounds immediately. Heavy chords thin but stay on the grid | | | |
| 10 | LFO Tempo sync | Sync Mode Tempo, LFO 1 Rate Sync 1 bar → S1 Level; Global, then Per-voice | Global: locked to the bar, even across loop jumps. Per-voice: each note restarts the LFO at tempo rate | | | |
| 11 | LFO → Reverb Mix through 0 | LFO 1 → Reverb Mix at depth ±50 with Reverb Mix at 0 | The tank restarts cold each time the mix reaches 0 (parent R54). **Listening call:** is this acceptable? | | | |
| 12 | Old patches | N/A *(rewritten at Stage 4)*: no pre-1.0 sessions exist | Harness-covered (TB7 / state rows) | | | |

Listening notes: LFO feel (row 1): ______ · per-voice spread on chords (row 2): ______ · Tempo burst feel (row 9): ______ · reverb cold restarts (row 11): ______

Result: ______ (all ✅ → Phase 2.5a hands-on PASS; any ❌ → note it and re-run `/plugin-execute O-GranulatorInstrument 2-dsp` for the fix)
