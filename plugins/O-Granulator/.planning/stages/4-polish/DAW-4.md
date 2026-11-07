# O-Granulator — Stage 4 DAW pass (Logic Pro AU + Reaper VST3)

**Stage:** 4-polish · **Run at:** Stage 4 **verify**, by Taylor (CONTEXT-4 D46, PLAN-4 P161). Not run in execute. · **Written:** execute T7, 2026-11-07
**Build:** the installed `O-Granulator-dev` 1.0.0 (AU + VST3) from the Stage 4 execute commits. Install: `./scripts/build-and-install.sh O-Granulator` (it sweeps the AU cache and the `-dev` / unsuffixed alternate bundles). Quit and relaunch both hosts after an install.

**Pre-step (both hosts, R143):** confirm the installed factory bank is the current one:
`ls ~/Library/O-Granulator/Presets/Factory` → 16 `.json` files (`Ambient · …` ×4, `Cascade · …` ×4, `Harmoniser · …` ×4, `Rhythmic · …` ×4) and `cat ~/Library/O-Granulator/Presets/Factory/.factory-version` → `1.0.0`. If either is wrong: quit the hosts, `rm -rf ~/Library/O-Granulator/Presets/Factory`, reopen one instance (it rewrites the bank). Never touch `User/`.

**General setup:** 48 kHz, buffer 256, monitors at normal listening level, editor open. Record PASS / FAIL and one line of what you heard / saw per item.

---

## Part 1 — Logic Pro (AU)

| # | Item | Session recipe | PASS if | FAIL consequence |
|---|---|---|---|---|
| L1 | All 16 presets by intent | A stereo track with a melodic loop (pad / keys / voice) and a drum loop on a second track sent to a bus with a second instance. Step through all 16 with ◀ ▶ on the matching source: Ambient + Harmoniser + Cascade on the melodic loop, Rhythmic on the drums. | Each preset does what its category says (Ambient: a sustained cloud with a tail; Harmoniser: audible intervals that track the notes and sound in key; Rhythmic: grains locked to the host tempo; Cascade: a self-evolving feedback texture). No preset is silent, harsh-clipped or wildly louder than the rest (the harness spread is 5.9 dB RMS). | FUNC-09 reopens; re-voice the named preset (a verify finding, not a plan change) |
| L2 | AGC action on a Cascade preset | `Cascade · Rust Spiral` (sat 60 %) on the melodic loop. Watch the loop-gain readout. | The readout moves below the knob value (about −1.4 dB at fb 90 %) during loud passages; the AGC is engaging at a non-zero saturation (D43). At sat 0 (`Cascade · Infinite`) it may stay at the knob value. | D43 regresses; check AG1 / AG2 against the installed build |
| L3 | No pumping at fb 50 % | Defaults, `feedback` 50 %, on the drum loop at a hot level (peaks near 0 dBFS). | No audible gain breathing on the tail between hits (the harness measured 0.0 dB p-p of AGC modulation; AG4). | D43 pumping (AG4 by ear); stop and decide `kAgcCeil` with Taylor |
| L4 | Freeze re-listen (confirmation only) | Any Ambient preset, Freeze on for 10 s, off. | The frozen texture holds and the release is click-free, as at 2.4 (head-stop accepted, VERIFICATION-2.4). | Note only; head-stop was closed at 2.4 |
| L5 | Preset A → B while playing | Switch between four presets of different categories while the transport plays. | No crash, no stuck notes / runaway, no loud click beyond the change itself; each lands as itself (PR4: A → B == fresh B). | FUNC-09 / COMPAT-02 reopen |
| L6 | Session save / quit / reopen ×2 | Instance on `Harmoniser · 19-EDO Haze`, change the scale root and one mod route; save, quit Logic, reopen; change the seed-bearing texture (step to another preset and back); save, quit, reopen. | Both reopens restore every edit, including the 19-EDO custom scale and its mask (ST1–ST4). | COMPAT-02 reopens |
| L7 | Mono and stereo insert | (a) a mono track (mono → stereo insert); (b) a stereo track. Default preset + `Ambient · Cloud Drift`. | Both work; (a) has a stereo output with width. | COMPAT-03 reopens |

## Part 2 — Reaper (VST3) — D46

| # | Item | Session recipe | PASS if | FAIL consequence |
|---|---|---|---|---|
| R1 | Load | Insert `O-Granulator-dev` (VST3) on a stereo track. | The UI opens; audio passes; the preset menu lists 16 factory presets grouped by prefix. | COMPAT-03 (VST3) reopens |
| R2 | The four categories | One preset from each category on suitable material, at 120 bpm project tempo (and once at 93 bpm for the Rhythmic one). | Same intent as L1; the Rhythmic preset follows the project tempo at 93 bpm too. | FUNC-09 reopens |
| R3 | Automation of 3 params | Write automation for `feedback`, `s1_position` and `mix`, play back. | Smooth, click-free playback following the lanes; the UI knobs follow. | QUAL / COMPAT-03 reopen (VST3 automation path) |
| R4 | Save / reopen | Save the project, close Reaper, reopen. | Every edit and the automation are restored. | COMPAT-02 reopens |
| R5 | Mono / stereo track | Repeat R1 on a mono item track. | Works; stereo output. | COMPAT-03 reopens |

---

## Verdict

QUAL-04 completes at Stage 4 verify only when **this pass and HANDS-ON-4.md** both PASS (PLAN-4 P162). A FAIL reopens the named requirement. Windows VST3 is proven by release CI at `/publish` (sign-off 4), not here.

## Results (2026-11-07, Taylor)

| Part | Items | Verdict |
|---|---|---|
| Logic Pro (AU) | L1 · L2 · L3 · L4 · L5 · L6 · L7 | PASS (7 / 7) |
| Reaper (VST3) | R1 · R2 · R3 · R4 · R5 | PASS (5 / 5) |

**DAW-4 PASS.** With HANDS-ON-4 PASS (H1–H6), QUAL-04 is complete. No requirement reopens.
