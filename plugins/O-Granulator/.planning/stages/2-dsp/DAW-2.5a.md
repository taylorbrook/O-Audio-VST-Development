# O-Granulator — Phase 2.5a DAW tempo checklist (D16)

**Stage:** 2-dsp / 2.5a · **Run at:** verify · **Written:** execute T13 (PLAN-2.5a P59), 2026-11-05
**Build:** the installed `O-Granulator-dev` VST3 / AU from the 2.5a execute commit (generic editor — the WebView UI is Stage 3).

These items cover what the harness cannot: real host transports report a position only at block starts, wrap loops mid-block, ramp tempo per block and may or may not report `isPlaying` during a count-in. The harness gates (TT, TS, LS, N8) prove the engine against a block-split mock; this sheet checks it against Logic and Reaper.

**General setup (every item unless it says otherwise)**
- Logic Pro (AU) and/or Reaper (VST3), 48 kHz, buffer 256 (Reaper: also try 1024 once), monitors at normal listening level.
- One stereo audio track with a sustained, pitched source (a pad, a held vocal or a sustained synth chord — something whose onsets you will NOT confuse with grain onsets). Put O-Granulator on it, open the generic editor.
- Start from defaults, then: `mix` 100 %, `s1_jitter` 0 %, `s1_spray` 0 %, `s1_size_rand` 0 %, `s1_window` 0 (Hann), `sync_mode` **Tempo**. Only stream 1 is on by default; leave it that way unless an item says otherwise.
- Enable the host metronome (click) for items 1–3 and 5–6.

Record PASS / FAIL and one line of what you heard per item.

| # | Item | Session recipe | Listen / look for | PASS if |
|---|---|---|---|---|
| 1 | Synced grains vs the click | 120 bpm, 4/4. `s1_density_sync` 1/16, `s1_size_sync` 1/32. Play 8 bars. Then `s1_density_sync` 1/8 T, play 8 bars. Repeat at 133 bpm. | Each grain onset against the click (and its subdivisions). With jitter 0, 1/16 grains land exactly 4 per click; 1/8 T lands 3 per click. | No audible flam / drift against the click over 8 bars, at both tempi, at both divisions. |
| 2 | Re-timing on start / loop / locate | 120 bpm, 1/16 density, `s1_size_sync` 1/64. (a) Stop, then start from bar 3. (b) Cycle bars 2–4 (Logic cycle / Reaper loop) and let it wrap 4×. (c) While playing, click the ruler to jump to bar 9, then to bar 5 beat 3. | The first grains after each start, wrap and jump. | Grains re-lock to the grid immediately after every start, wrap and jump: no burst of extra grains, no gap longer than one division, no off-grid grain after the wrap. |
| 3 | Host tempo ramp (60 → 180 bpm) | Tempo track: a linear ramp from 60 bpm at bar 1 to 180 bpm at bar 9, 1/16 density, `s1_size_sync` 1/32. Play bars 1–10. | Grains against the click through the ramp. | Grains follow the click through the whole ramp (no flam that grows over the ramp, no stutter at block boundaries). A ramp is block-size dependent by nature: the harness cannot gate it (R56). |
| 4 | LFO sweeps for zipper | `sync_mode` Free. LFO 1 sine 2 Hz. Mod 1: LFO 1 → S1 · Position, depth 60 %. Listen 10 s, then re-route Mod 1 dest in turn to S1 · Pitch (depth 30 %), S1 · Filter Cutoff (`s1_filter_type` LP, res 40 %, depth 40 %) and Mix (depth 50 %, `mix` 50 %). Then LFO 1 rate 20 Hz on Mix. | Zipper / stepping / clicks on the modulated parameter. Position / pitch / cutoff are latched per grain: each grain carries one value (a grain-by-grain "staircase" at low density is expected — that is the sampled source, not zipper). | No zipper noise or clicks on Mix (ramped) at 2 Hz or 20 Hz; no clicks inside grains on the latched destinations. |
| 5 | Synced LFO across a loop point | 120 bpm, `sync_mode` Tempo, LFO 1 square, `lfo1_rate_sync` 1 bar, Mod 1: LFO 1 → S1 · Level, depth 50 %. Cycle bars 2–3 (a 2-bar loop starting off bar 1) and let it wrap 4×. | Where the level steps happen relative to the bar line, before and after each wrap. | The square flips on the bar lines every pass (the LFO re-locks at the wrap, R42 / D15); no extra flip or half-bar offset after a wrap. |
| 6 | 3/4 project (R44, signed off) | Project in 3/4 at 120 bpm. `s1_density_sync` 1 bar, `s1_size_sync` 1/16; LFO 1 square `lfo1_rate_sync` 1 bar → S1 · Pan, depth 80 %. Play 8 bars. Then add a 4/4 bar at bar 5 (time-signature change) and play across it. | Grain onsets on the downbeats; the pan flip on each bar line. | One grain per 3/4 bar on the downbeat, pan flips on every bar line, and both follow the 4/4 bar after the change. |
| 7 | LFO → reverb_mix across 0, fast LFO → reverb_size (R54) | `sync_mode` Free, `reverb_mix` 0 %, `reverb_size` 60 %. Mod 1: LFO 1 sine 0.5 Hz → Reverb Mix, depth 60 %. Listen 20 s. Then Mod 1 → Reverb Size, depth 50 %, `reverb_mix` 40 %, LFO 1 rate 8 Hz. | (a) The reverb tail each time the LFO takes the mix through 0 (the unit clears its tank when the mix sits ≤ 0.1 % longer than its 50 ms fade). (b) Size modulation depth at 8 Hz. | (a) A gated-reverb effect at the bottom of each LFO cycle is ACCEPTED behaviour (R54) — FAIL only on a click or a hard dropout at the gate. (b) Size modulation is audibly smoothed (the unit's 100 ms smoothing) and pitch-warbles the tank, without zipper. Record what you hear for both. |
| 8 | Logic count-in / pre-roll (R42) | Logic only. 120 bpm, 1/16 density, Count-in 1 bar (and, separately, Pre-roll 1 bar). Start from bar 3. | Whether grains play during the count-in and land on the click there; the first bar after the count-in. | Either grains run on the grid during the count-in (Logic reports `isPlaying`) or they run on the virtual grid and re-lock at the downbeat — in both cases the first grain at bar 3 is on the downbeat with no burst. Record which behaviour Logic shows. |

## Notes for the verifier
- Every 2.5a parameter is reachable in the generic editor: `Mod N · Source / Dest / Depth`, `LFO N · Shape / Rate / Rate Sync / Phase`, `Env Attack / Release / Gain`, `Macro N`, `Sync Mode`, `SN · Size Sync / Density Sync`.
- Tempo size clamps at 2 s (R45, signed off): "1 bar" size below 120 bpm and "2 bars" below 240 bpm play at 2 s. Not a defect.
- Pitch and Onset sources output 0 until 2.5b; routing them does nothing (MF6).
