# O-GranulatorInstrument — HANDS-ON-2.4 (human gate)

**Build:** 0.1.0 dev (`O-GranulatorInstrument-dev`), Phase 2.4 engine (working-copy feedback, Freeze, Reset, reverb, the parent D9 wet knee) with the `DevLoaderEditor` (four Load buttons above the generic editor).
**Run in:** the Standalone **and** one DAW (Logic preferred; AU `aumu OuGI OuDv`).
**Not self-certified.** Mark each row ✅ / ❌ and add a note for every ❌.

> **Deferred with HANDS-ON-2.1 / 2.2 / 2.3.** On 2026-11-08 the user deferred all hands-on testing until the Stage 3 UI is in place. Run this sheet together with HANDS-ON-2.1, 2.2 and 2.3 then.

Defaults to know before you start:
- **Feedback defaults to 0 %, Freeze to off and Reverb Mix to 0 %.** With those three at their defaults the patch sounds as it did in 2.3, except for very hot peaks (row 11).
- Each slot's **working copy** is what the grains read. With feedback above 0, a record head walks through the slot's loop region at exactly 1× and writes the grains' processed output back into it, so the copy evolves while you play.
- **Freeze** stops the record heads (over 20 ms): the evolved copy holds. The feedback chains keep running, but nothing is written.
- **Reset** restores a slot (or all four) to its pristine source through a 20 ms crossfade. In 2.4 it is not a parameter and has no control in the generic editor; Stage 3 adds the Reset button (a native function). Until then, row 5 needs the Stage 3 UI, or a harness build.
- **The working copy is not saved.** Reopening a session reloads every slot pristine (by design).
- `fb_low_cut` / `fb_high_cut` / `fb_saturation` / `fb_diffusion` shape only what is written back. `reverb_mix` / `reverb_size` / `reverb_damp` act on the output.
- The output ceiling is the parent's knee: identity at or below −6 dBFS, then a soft limit that never exceeds +6 dBFS before `output_gain`.

| # | Row | How | Expected | Standalone | DAW | Notes |
|---|---|---|---|---|---|---|
| 1 | Feedback character | Hold a chord. Raise Feedback 0 → 100 % at Saturation 0, then 20, then 100 % | The copy evolves audibly. Each Saturation setting has its own character. No runaway and no level blow-up | | | |
| 2 | Feedback filters + diffusion | At Feedback 80 %, sweep FB Low Cut and FB High Cut, then FB Diffusion 0 → 100 % | The written layer thins and darkens with the cuts; diffusion smears it | | | |
| 3 | Freeze | Toggle Freeze on and off while playing at Feedback 80 % | The heads stop with no click and the evolved copy holds; unfreeze resumes smoothly | | | |
| 4 | Head stop / start | At Feedback 100 %, release every note, wait 2 s, play again | No splice or tick as grains sweep the point where the head stopped | | | |
| 5 | Reset | (Stage 3 UI.) Reset one slot, then all, while playing and while frozen | Back to the pristine sound through a smooth 20 ms crossfade. Frozen stays frozen | | | |
| 6 | Chords vs unison | Feedback 80 %: a single note, then a wide chord | Chords thin in the copy (the ARCH §3 mean tap). **Listening call:** is the `1/sqrt(Σa)` chord fallback needed? (Any change is a later decision.) | | | |
| 7 | Reverb | Sweep Reverb Mix, Size and Damp; then pull Mix to 0 while a tail rings; then raise it again after a pause | Audible and smooth. The tail fades out with no step when Mix goes to 0. A cold re-enable is clean | | | |
| 8 | Extreme patch | Feedback 100 % + Reverb Mix 100 % + Filter Res 100 % on all streams + 16 voices | The output never exceeds +6 dBFS before Output Gain (watch the DAW meter with Output Gain at 0 dB). **Listening note:** judge the knee's limiting character | | | |
| 9 | Sample-rate change | In the DAW, switch 44.1 ↔ 96 kHz with Feedback on and a note held | The pitch stays correct. The heads resume after the slot rebuild lands (a short pause in the copy's evolution is expected) | | | |
| 10 | Session reopen | Set Feedback, the FB controls, Freeze and the Reverb controls; save, close and reopen | Every parameter comes back. The working copy reloads pristine (by design) | | | |
| 11 | Knee adoption | Load a 2.1–2.3 patch with the defaults above | It sounds unchanged at normal levels (the knee is identity at or below −6 dBFS). Only hot peaks differ | | | |

Listening notes — saturation character (row 1): ______ · chord fallback needed? (row 6): ______ · knee limiting character (row 8): ______

Result: ______ (all ✅ → Phase 2.4 hands-on PASS; any ❌ → note it and re-run `/plugin-execute O-GranulatorInstrument 2-dsp` for the fix)
