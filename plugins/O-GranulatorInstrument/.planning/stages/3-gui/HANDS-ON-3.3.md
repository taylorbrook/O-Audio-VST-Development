# O-GranulatorInstrument — HANDS-ON-3.3 (human gate)

**Build:** 0.1.0 dev (`O-GranulatorInstrument-dev`), Phase 3.3: MOD page (17 sources, drag-to-modulate, rings, mono badges, loop-edge ticks), VOICE page (voice strip with releases and steals, env scopes, LFO views, `+48` clamp marker), output and sidechain meters L / R, grain caption and heavy hint.
**Run in:** the Standalone and Logic (AU `aumu OuGI OuDv`). Rebuild the Standalone first: `ninja -C build OuariconGranulatorInstrument_Standalone`.
**Run inside the one session** described in `HANDS-ON-SESSION-3.3.md` (D47), after 2.1–2.6, 3.1 and 3.2. A ❌ here routes to `/plugin-execute O-GranulatorInstrument 3-gui`.
**Not self-certified.** Mark each row ✅ / ❌ and add a note for every ❌.

## What automation already covers (do not repeat by hand)

Harness (processor, scripted clock): 16 rows keyed by voice serial (a voice in engine slot 16–19 is shown), `newest`, the 300 ms steal hold, release / pedal states, steal counting, the `wantSt` column and grain clamp bit, env and LFO phases, the 17 sources, routed-only `modNorm`, loop `lo / hi`, the mono rule for all 17 × 125 pairs, meters bit-exact (output) and exact at known sine levels (sidechain), max-since-read, `limit`, collision-proof take names, 0 audio-thread allocations. Headless page: voice cell classes and read-outs, chip glow, ring dot, loop shading and edge tick, LFO dot on the curve at 90°, mono badges, a real pointer drag from each of the 17 chips onto a knob and a loop handle, a drop on all 137 targets (125 destinations), meter bar heights, heavy hint, every i18n state held for ≥ 450 ms, label fit in en / fr / zh-Hans.
**Not covered:** the real WKWebView, pointer capture across pages, devicePixelRatio 2, Logic's side-chain routing, the 0.3 s steal hold seen by eye, anything you judge by eye or ear.

## Rows

| # | Where | Do | Expect | ✅ / ❌ |
|---|---|---|---|---|
| 1 | MOD dock → any page | Drag each of the 17 chips (LFO 1–4, Env 1–2, Vel, Key, Rand, Wheel, Press, Slide, Bend, M1–M4) onto a knob | A matrix row appears with that source and destination, depth +40 %; the knob gets a ring | |
| 2 | MOD dock → STREAMS | Drag a few chips onto a loop start / end handle | Route appears on the loop destination; the handle shows a tick | |
| 3 | MOD | Route a global LFO (Mode Global) to a knob and to a loop end; play no notes | Ring dot and the loop-edge tick move with the LFO; the loop shading follows the moving edge | |
| 4 | MOD | Route Env 1 to a per-stream knob (e.g. stream I Level); hold a note, release | Ring dot moves with the envelope, then rests at the knob value once the note has ended | |
| 5 | MOD | Route Env 2 → Output; route LFO 2 in Per-voice mode → Reverb mix | Both rows show the mono badge. **Expected:** after the last note ends these rings stay at the last value (a per-voice source on a global destination holds; see Known limits) | |
| 6 | MOD | Alt-drag a matrix row's depth | Depth changes smoothly; the ring's travel follows | |
| 7 | VOICE, Polyphony 8 | Play a chord of 10 notes | 8 cells lit; each steal shows a stolen cell for about 0.3 s, the new note in its own cell | |
| 8 | VOICE, Polyphony 16 | Play past 16 notes | Steals read as a note change in the same cell (accepted limit PD1) | |
| 9 | VOICE | Release notes; then hold the sustain pedal (CC64) and release keys | Released cells show the release state until the tail ends; with the pedal the cells stay held | |
| 10 | VOICE + MOD | Play several notes in turn | The key view and the newest marker follow the last note | |
| 11 | VOICE | Hold a note with slow Env 1 / Env 2 / amp settings | Env scope dots ride their curves stage by stage; read-outs show `newest`; with no note they read idle | |
| 12 | VOICE / MOD | LFO 1 Per-voice, Phase 90°, hold three notes at different times | One dot per voice on the LFO curve (newest in ochre), each on the drawn line | |
| 13 | VOICE + STREAMS | Play a note far above the stream root (more than 48 st above) | That voice's cell shows the `+48` clamp marker; that stream's grain dots draw in the warn colour; a stream with key tracking 0 does not | |
| 14 | Output meter | Load a tape, set stream I Pan fully left, Stereo 0, Reverb mix 0, hold a note | L bar moves, R bar stays at rest | |
| 15 | Record-in meter (Logic) | Route a side-chain from an audio track playing a left-only signal | SC L bar moves, R rests; with no side-chain routed the meter reads slashed / No signal | |
| 16 | Record-in meter | Close the editor while audio plays, reopen | The first frame shows the max since the last read, then normal ballistics (Known limits) | |
| 17 | STREAMS header | Grain Limit 128, then 256 | Caption `n / 128` and `n / 256`; heavy hint off at 128, on at 256 | |
| 18 | Gear → Français, then 简体中文 | Walk MOD and VOICE, incl. a clamp cell (row 13) and a stolen cell (row 7) | Text reads correctly and fits; zh-Hans clamp marker fits its cell (fixed at 3.3, check-ui-labels [4]) | |

## Known limits (expected, not ❌)

- **Mono routes do not return to base.** A per-voice source on a global destination reads the engine's held newest-voice value, so the ring stays where the last voice left it (row 5). Per-voice destinations rest at base (row 4).
- **Polyphony 16:** a steal shows as a note change in the same cell (row 8).
- **Meters accumulate while the editor is closed**; the first open shows that max (row 16).
- **`+48` marker excludes `pitch_rand` and scale picks** (the per-grain draw is not stored).
- **Sidechain meter on an idle instrument track** in Logic reads 0 (Logic does not feed the side-chain while the track is idle).
- A failed take's intended file name can be reused by a successful take on the same slot within the same second (the failed file never existed).
