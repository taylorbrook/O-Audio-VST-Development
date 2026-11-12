# O-GranulatorInstrument — HANDS-ON-2.5b (human gate)

> **Superseded by [HANDS-ON-SESSION-4](../4-polish/HANDS-ON-SESSION-4.md) (2026-11-11).** Run that sheet; this one is kept for history.

**Build:** 0.1.0 dev (`O-GranulatorInstrument-dev`), Phase 2.5b engine (Poly / Mono / Legato with glide, sustain pedal, the MPE lower zone, VST3 note expression), with the `DevLoaderEditor` (four Load buttons above the generic editor).
**Run in:** the Standalone, Logic (AU `aumu OuGI OuDv`) **and** Dorico (VST3, for rows 9 only).
**Not self-certified.** Mark each row ✅ / ❌ and add a note for every ❌.

> **Deferred with HANDS-ON-2.1 / 2.2 / 2.3 / 2.4 / 2.5a.** On 2026-11-08 the user deferred all hands-on testing until the Stage 3 UI is in place. Run this sheet together with the earlier ones then.

Defaults to know before you start:
- **Voice Mode** Poly / Mono / Legato. **Mono** restarts the envelopes from where they are on every new note (no drop to zero), restarts the scan and the per-voice LFOs, and redraws Note Random. **Legato** only moves the pitch while a key is held. Releasing the top key while a lower key is held glides back to it in both modes, without restarting anything.
- **Glide** (0–2000 ms) is constant-time: any interval takes the same time. In Poly with Glide > 0, each new note starts at the previous note's pitch.
- **Sustain (CC64)** holds the notes you release. Re-striking a held note in Poly releases the old one with its normal tail. CC121 (Reset All Controllers) also lifts the pedal; a transport stop (CC123) releases everything, pedal or not.
- **MPE** (lower zone): ch 1 = master, ch 2–16 = one note each. Member bend, pressure and slide move only their own note; the master bend adds to every note. Master aftertouch / CC74 and poly aftertouch are ignored with MPE on. The member bend range comes from the controller's RPN 0 (default 48).
- **Note expression** (VST3 only): per-note tuning by note ID, live while the note sounds. AU and Standalone receive none.
- With Voice Mode Poly, Glide 0, MPE off and no pedal, a patch sounds exactly as it did in 2.5a (proven bitwise: TB5).

| # | Row | How | Expected | Standalone | DAW | Notes |
|---|---|---|---|---|---|---|
| 1 | Mono retrigger | Voice Mode Mono, Amp Attack 200 ms, Sustain 50 %. Play overlapping phrases at mixed velocities | Each new note re-attacks from the current level. No click, no dip to silence; a velocity change swells over a few ms | | | |
| 2 | Legato | Voice Mode Legato, Glide 150 ms. Play overlapping phrases | Only the pitch moves; the envelope, scan and LFOs keep running. Smooth | | | |
| 3 | Fallback | Both modes: hold C, press E, release E | Glides back to C without a re-attack | | | |
| 4 | Glide time | Glide 0 / 50 / 300 / 2000 ms in Mono, small and large intervals; then Poly with Glide 300 ms | Constant time for any interval; Glide 0 jumps cleanly. In Poly each new note starts at the last note's pitch | | | |
| 5 | Mode switch | Hold a chord in Poly, switch to Mono (and back) while holding | One voice (the newest) keeps sounding, the rest fade in 5 ms. No click, no stuck note | | | |
| 6 | Sustain pedal | Pedal down, play and release notes, re-strike a held note, pedal up. Then a transport stop with the pedal down | Held notes ring until pedal-up; a re-struck note releases the old one with its tail. Transport stop silences everything | | | |
| 7 | MPE controller | MPE on; Seaboard / LinnStrument / Osmose (whichever is available) with per-note bend, pressure and slide routed (Pressure → S1 Level, Slide → S1 Filter Cutoff) | Each note bends, swells and brightens on its own. The master bend moves everything | | | |
| 8 | MPE bend range | Set the controller's pitch-bend range (it sends RPN 0, e.g. 24 or 48) | The per-note bend covers exactly that range | | | |
| 9 | Dorico NE | Dorico + VST3: quarter-tone accidentals in a melody; playback and note-input preview. Then Legato with glide on the same passage | Quarter-tones sound at the right pitch in preview and playback; Legato glides land on the tuned pitch | | | |
| 10 | No-hard-control click rows | Legato glide (CK5b), fallback glide (CK5c), MPE member bend sweep (CK5f) on a sine source | No clicks or zipper (pitch is latched per grain, so a step makes no click by construction; listening check) | | | |
| 11 | Old patches | N/A *(rewritten at Stage 4)*: no pre-1.0 sessions exist | Harness-covered (TB7 / state rows) | | | |

Listening notes: Mono re-attack feel (row 1): ______ · Legato smoothness (row 2): ______ · glide curve (row 4): ______ · MPE expression (row 7): ______ · Dorico tuning (row 9): ______

Result: ______ (all ✅ → Phase 2.5b hands-on PASS; any ❌ → note it and re-run `/plugin-execute O-GranulatorInstrument 2-dsp` for the fix)
