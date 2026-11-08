# O-GranulatorInstrument — HANDS-ON-2.3 (human gate)

**Build:** 0.1.0 dev (`O-GranulatorInstrument-dev`), Phase 2.3 engine (per-grain FX) with the `DevLoaderEditor` (four Load buttons above the generic editor).
**Run in:** the Standalone **and** one DAW (Logic preferred; AU `aumu OuGI OuDv`).
**Not self-certified.** Mark each row ✅ / ❌ and add a note for every ❌.

> **Deferred with HANDS-ON-2.1 / 2.2.** On 2026-11-08 the user deferred all hands-on testing until the Stage 3 UI is in place. Run this sheet with HANDS-ON-2.1 and HANDS-ON-2.2 then.

Defaults to know before you start:
- Every per-grain FX parameter is **latched when a grain starts**. Moving a knob changes the next grains, never a sounding one. Expect a short lag of up to one grain length.
- Each stream has its own FX settings. They are in the generic editor as `S1 · Filter Type`, `Filter Cutoff`, `Filter Random`, `Filter Res`, `Drive`, `Crush`, `Crush Chance`, `Ring Freq`, `Ring Random` and `Ring Mix`.
- Filter Type defaults to **Off**. Drive, Crush and Ring Mix default to 0, so the patch sounds exactly as it did in 2.2 until you turn one on.
- There is still no feedback, freeze, reverb, modulation, glide or MPE. These arrive in 2.4 and 2.5.

| # | Row | How | Expected | Standalone | DAW | Notes |
|---|---|---|---|---|---|---|
| 1 | Filter LP / BP / HP | Hold a note. Set Filter Type LP, then BP, then HP; sweep Cutoff and Res on each | Each type is audible and distinct. Res 100 % rings without blowing up | | | |
| 2 | Filter Random | Filter Random 100 %, then 0 % | At 100 % the colour varies from grain to grain; at 0 % it is static | | | |
| 3 | Drive | Sweep Drive 0 → 100 % | Saturation grows progressively. Drive 0 sounds the same as FX off | | | |
| 4 | Crush + Crush Chance | Sweep Crush 0 → 100 % with Crush Chance at 100, 50 and 0 % | You hear sample-and-hold plus bit reduction. At 50 % only some grains crush. **Listening note:** write down where quiet material disappears at high Crush (R22 silence threshold) | | | |
| 5 | Ring | Sweep Ring Freq 20 → 5000 Hz at several Ring Mix values; then Ring Random 100 % | The ring modulation follows the frequency and mix. At Random 100 % each grain has its own carrier | | | |
| 6 | Extreme | Res 100 % + Drive 100 % + Crush + Ring on all 4 streams, a chord, Grain Limit 256 | No dropouts, no stuck or silent output. **Listening note:** judge the LP / HP resonance level against the wet safety (R19) | | | |
| 7 | FX off | Turn every FX off on every stream | The patch sounds as it did in 2.2 | | | |
| 8 | Worst-case CPU | In Logic at 48 k / 128, play the D19 worst-case patch (16 notes × 4 streams, BP res 100 + Drive 100 + Crush 100 + Ring Mix 100) | No overloads. This is informational; the verdict is C23w | | | |
| 9 | Session reopen | Set different FX values per stream, save the project, close and reopen it | Every FX setting comes back | | | |

Listening notes — Crush silence threshold (row 4): ______ · LP / HP resonance vs wet safety (row 6): ______

Result: ______ (all ✅ → Phase 2.3 hands-on PASS; any ❌ → note it and re-run `/plugin-execute O-GranulatorInstrument 2-dsp` for the fix)
