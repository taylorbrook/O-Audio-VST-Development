# Stage 3 — GUI · Phase 3.3 — Hands-on checklist (real WebView)

**Plugin:** O-Granulator · **Phase:** 3.3 (Mod matrix view + mod rings; meters, LFO phase, onset, loop gain)
**Run at:** 3.3 **verify**, by Taylor (PLAN-3.3 P139, CONTEXT-3.3 D41). Not run in execute.
**Why a human:** the harness proves every 3.3 field's value (MT1–MT5, ON1–ON2, LP1–LP2, LG1–LG3, TL33, M9) and that route edits are click-free in the trajectory and in audio (UX1). The stub gates prove the page reads every key. Only the real WebView at ~30 Hz shows whether the meters, the LFO dot, the onset pills, the loop-gain readout and the matrix *look and feel* right, and only an ear confirms the route edits on a sustained tone.

**Host:** the Standalone (`O-Granulator-dev.app`) unless an item says otherwise. Install first: `./scripts/build-and-install.sh O-Granulator`. **Unmute the Standalone input** (JUCE mutes it by default: "Audio input is muted to avoid feedback loop"; use headphones). Use a live input (mic or loopback): a sustained tone for B / F, material with clear transients for C. The Logic Pro pass is the separate **DAW-3.3.md**.

Mark each item PASS / FAIL / NOTE. A FAIL gets a one-line note (what you did, what you saw).

---

## A. Meters (D38; R128–R130)

| # | Item | Expect | Verdict |
|---|---|---|---|
| A1 | A steady −6 dBFS sine in, `mix` 0 % | The out meter sits about 6 dB below the top of its range and holds steady (no flicker between frames) | |
| A2 | Same, turn **Input Gain** up and down | With `mix` 0 the **out** meter does not move; the input **level lane** does (inPeak is post input gain, pre mix) | |
| A3 | `mix` 100 %, `feedback` 80 % | The input lane does **not** rise with the feedback (it shows the input, not the ring) | |
| A4 | `freeze` on, then stop the input | The input lane drops to zero while the out meter keeps moving with the frozen tail | |
| A5 | Play a loud passage, close the editor, wait 5 s in silence, reopen | No phantom peak on the first frame after the reopen (R130 gap rule); the meters show the current (silent) level | |

## B. Loop gain (D34; R133)

| # | Item | Expect | Verdict |
|---|---|---|---|
| B1 | `feedback` 50 %, quiet input | The readout shows about −6.5 dB | |
| B2 | `feedback` 0 % | "Open" | |
| B3 | `feedback` 50 %, `freeze` on | "Open" once the head stops (a short moment after the button); `freeze` off → a dB value again | |
| B4 | `feedback` 0 %, route **LFO 1 → Feedback** at 50 %, LFO 1 at 1 Hz | The readout moves with the LFO: a dB value (never above −0.4 dB) on the positive half, "Open" on the negative half | |
| B5 | `feedback` 100 %, **`fb_saturation` 0 %**, `fb_diffusion` 0 %, a hot input (Input Gain +12 dB) | The value drops well below −0.4 dB (the AGC engages; the harness measured ≈ −3.0 dB). **Note:** at the default `fb_saturation` 20 % the AGC cannot engage at all (the saturator caps its input at 1 / 2.4 ≈ 0.42 < the 0.7 ceiling), so the readout stays at the knob value, −0.45 dB at 100 % | |
| B6 | Saturation note (D34, by design): `feedback` 80 %, `fb_saturation` 100 % | The readout stays at the knob × AGC value (about −2.4 dB) while the audible return is visibly softer. This is the documented approximation, not a FAIL; note how misleading it feels | |

## C. Onset pills (D40 as amended; R131)

| # | Item | Expect | Verdict |
|---|---|---|---|
| C1 | Claps or a drum loop | The pills flash once per hit and fade within about 0.2 s | |
| C2 | Stop the input mid-flash (the Standalone keeps running) | The pills go dark within 0.2 s | |
| C3 | Close the editor during a busy passage, reopen in silence | No phantom onset tick on reopen | |

## D. LFO dot (D39; R132)

| # | Item | Expect | Verdict |
|---|---|---|---|
| D1 | LFO 1 sine, triangle, saw, square at 0.5 Hz | The dot rides on the drawn curve for each shape | |
| D2 | LFO 1 Phase 90° | The dot still rides on the drawn curve, and its height matches the LFO 1 source chip (the harness proved value == sin (2π (phase + 0.25)), LP1) | |
| D3 | LFO 1 S&H and Smooth Random | The dot moves along x but can sit off the stylised curve (accepted, D39). Note how it looks | |
| D4 | `sync_mode` Tempo (Standalone: transport off) | The dot still moves (the Free fallback) with no stutter | |

## E. Mod matrix + rings (UI-03)

| # | Item | Expect | Verdict |
|---|---|---|---|
| E1 | Drag a source chip (LFO 1) onto the S1 Position knob | A route appears in the matrix (source, dest, depth 50 %); the knob shows an arc and a live dot | |
| E2 | Change a route's source and dest with the selects | The matrix row, the knob ring and the slot count (`#matrix-count`) all follow | |
| E3 | Click a slot number (clear) | The route disappears; the ring is removed; the slot count drops by 1 | |
| E4 | Alt-drag on a routed knob | The depth changes; the arc follows the drag; the value matches the matrix row | |
| E5 | Route all 16 slots | The count shows 16 / 16; no layout overflow | |
| E6 | Source chips while LFOs / Env / Pitch / Onset run | Each chip's tint follows its source's live value | |

## F. Route edits by ear (D36; UX1)

On a **sustained tone** (a held synth note or a sung vowel), `mix` 50 %, `feedback` 30 %, LFO 1 2 Hz.

| # | Item | Expect | Verdict |
|---|---|---|---|
| F1 | Route LFO 1 → S1 Level, then clear the slot mid-swing | No click; the modulation fades out over ~20 ms | |
| F2 | Drop a chip onto Mix, Output Gain and Feedback in turn | No click as each route lands | |
| F3 | Swap a live route's dest (Mix → Output Gain) and source (LFO 1 → LFO 2) | No click | |
| F4 | Alt-drag a live route's depth from +60 to −60 quickly | Smooth, no zipper or step | |

## G. Editor lifecycle and two windows (P117 lock; destructive reads)

| # | Item | Expect | Verdict |
|---|---|---|---|
| G1 | Close and reopen the editor 10 times while audio plays | Every reopen shows live meters / pills / dot at once; no frozen frame | |
| G2 | Two editors at once (the Standalone plus the AU in a host, or two plugin instances in one host) | Both windows' meters move. **Note (accepted):** with two editors on ONE instance the max-since-read peaks are split between readers, so each shows part of the peaks. Record how visible it is; this is NOT a FAIL | |

---

## Verdict

PASS when A–F have no FAIL. B6, D3 and G2 are recorded notes, not gates. A FAIL in A–F is fixed under D28 (a page or telemetry defect) and re-gated before 3.3 closes. Stage 3 closes at 3.3 verify only when this checklist **and DAW-3.3.md** both PASS (PLAN-3.3 P142).
