# O-GranulatorInstrument — HANDS-ON-2.2 (human gate)

> **Superseded by [HANDS-ON-SESSION-4](../4-polish/HANDS-ON-SESSION-4.md) (2026-11-11).** Run that sheet; this one is kept for history.

**Build:** 0.1.0 dev (`O-GranulatorInstrument-dev`), Phase 2.2 engine with the `DevLoaderEditor` (four Load buttons above the generic editor).
**Run in:** the Standalone **and** one DAW (Logic preferred; AU `aumu OuGI OuDv`).
**Not self-certified.** Mark each row ✅ / ❌ and add a note for every ❌.

> **Deferred with HANDS-ON-2.1.** On 2026-11-08 the user deferred all hands-on testing until the Stage 3 UI is in place. Run this sheet and HANDS-ON-2.1 together then.

Defaults to know before you start:
- Stream N plays **slot N**. Only stream 1 is enabled by default. Enable s2–s4 in the generic editor.
- The defaults are spray 10 %, jitter 20 % and stereo spread 30 %. A note now sounds wider and less static than in 2.1.
- There are still no per-grain FX, feedback, freeze, reverb, modulation, glide or MPE. These arrive in 2.3–2.5.
- Voice gain is now measured against O-Granulator (mix 100 %). See row 6.
- Scale and degree-mask editing has no UI yet (Stage 3). Only the `sN_scale` choice is reachable.

| # | Row | How | Expected | Standalone | DAW | Notes |
|---|---|---|---|---|---|---|
| 1 | Four streams, four slots | Load 4 different files into slots 1–4, enable s2–s4 and hold a chord | Each stream plays its own slot; all four are audible | | | |
| 2 | Enable / mute / solo | Toggle each stream's Enable, Mute and Solo while a chord sounds | Click-free. Solo is additive across streams. Unmute is instant: the stream kept spawning while muted | | | |
| 3 | Randomisation | Per stream, sweep Jitter, Spray, Stereo Spread, Reverse and Gain Random | Each one is audible on its own stream | | | |
| 4 | Window + skew | Sweep Window 0 → 1 and Window Skew −100 → +100 while a note holds | Audible change in grain shape; click-free | | | |
| 5 | Scales | Pitch Random 12 st with Scale = Major, a pentatonic, then a non-12 collection (e.g. 19-EDO); then Off | With a scale on, pitches snap to it; Off = continuous random pitch | | | |
| 6 | **Voice gain (D16)** | One note at defaults vs O-Granulator at **mix 100 %** on the same file at default stream settings; then a 6-note chord | The single note matches in loudness. The chord has no harsh knee or pumping. **Confirm, or give a trim in dB** (recorded as `kVoiceGainEarTrimDb`, PD1) | | | |
| 7 | Grain Limit | 4 streams × a dense chord; lower Grain Limit 256 → 32 | Density thins quickly; no clicks or stuck grains | | | |
| 8 | Steals | Polyphony 4, play 6+ distinct notes legato with 4 streams enabled | Steals are inaudible | | | |
| 9 | Release | Release 500 ms+, 4 streams, release a chord | Every stream keeps spawning through the release and fades out | | | |
| 10 | Session reopen | Set different `sN_scale` choices, save the project, close and reopen it | The scale choices come back. Slots come back too (slots restore from the session) *(rewritten at Stage 4)* | | | |

Voice-gain trim (row 6): ______ dB (0.0 = confirmed as measured)

Result: ______ (all ✅ → Phase 2.2 hands-on PASS; any ❌ → note it and re-run `/plugin-execute O-GranulatorInstrument 2-dsp` for the fix)
