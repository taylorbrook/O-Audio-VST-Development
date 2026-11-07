# Stage 3 — GUI · Phase 3.2 — Hands-on checklist (real WebView)

**Plugin:** O-Granulator · **Phase:** 3.2 (Buffer display + live grains)
**Run at:** 3.2 **verify**, by Taylor (PLAN-3.2 P126, CONTEXT-3.2 D33). Not run in execute.
**Why a human:** the automated rows (TB1, WV1–WV4, GD1–GD2, CN1, BD1, M8, TL32) prove the snapshot, the wave binning and every dot field in the harness, and the stub gates prove the page reads them. Only the real WebView shows whether the strip *looks* right at 30 Hz: alignment by eye, scrolling, colour and freeze styling.

**Host:** the Standalone (`O-Granulator-dev.app`). Install first: `./scripts/build-and-install.sh O-Granulator`. Use a live input (mic or a loopback) with clear transients — speech, claps or a drum loop. The DAW pass is not part of 3.2 (D27: 3.3 verify).

Mark each item PASS / FAIL / NOTE. A FAIL gets a one-line note (what you did, what you saw).

---

## Expected 3.3 stubs — not defects

These are drawn from telemetry fields that 3.3 fills. Do not flag them:

- The input / output **level lane** is flat.
- The **LFO phase dot** sits at the left of each LFO figure.
- There is **no onset pulse**.
- **Loop gain** reads "Open".
- Mod-matrix rings and the source meters are 3.3.

---

## A. Waveform (D31, R121, R122)

| # | Item | Expect | Verdict |
|---|---|---|---|
| A1 | Feed audio, freeze off | A waveform scrolls right → left; the newest audio is at the write head (right edge) | |
| A2 | Quiet material (about −30 dBFS, or turn Input Gain to −24 dB) | Still visible as a waveform about ⅓ of full height (D31 −48 dB floor), not a flat line | |
| A3 | Watch a sustained sound | It scrolls in clean steps; individual bins do not shimmer or flicker (absolute-cell anchoring) | |
| A4 | Buffer Length 1 s, then 30 s | The time ruler and the waveform rescale together; no gap at the right edge | |

## B. Freeze (R124)

| # | Item | Expect | Verdict |
|---|---|---|---|
| B1 | Freeze on | The write head turns dashed, the wave turns amber and **stops scrolling**, and the caption reads "held for N s" with N = Buffer Length | |
| B2 | Relaunch the Standalone, then freeze within 2–3 s | The caption shows the short held span (only that much was captured), and the wave fills only that span | |
| B3 | Freeze off | Scrolling resumes from the head; no jump or blank frame | |

## C. Grain dots (R119, R121, R125)

| # | Item | Expect | Verdict |
|---|---|---|---|
| C1 | Buffer Length 1 s, S1 Position 0, Spray 0, Density 20 | Dots sit on the transients they are reading (a dot appears at the right edge and drifts left with its transient, or stays put for pitch 0 forward) | |
| C2 | The same at Buffer Length 30 s | Dots still line up with the wave | |
| C3 | Enable all four streams | Dots in all four stream colours — **stream 1 (green) included** | |
| C4 | S1 Pitch +12, then −12 | S1 dots move up, then down | |
| C5 | S1 Stereo Spread 100 | S1 dots spread vertically (±6 px pan offset); S1 Pan hard left shifts them all to one side | |
| C6 | S1 Window: rectangular-ish vs Hann | The dot glow fades in/out differently with the window | |

## D. Bands (D30)

| # | Item | Expect | Verdict |
|---|---|---|---|
| D1 | No modulation | Each stream's band sits exactly at its Position / Spray knob values | |
| D2 | MOD: LFO 1 → S1 · Position, depth 60 %, rate 1 Hz | The S1 band and its marker move back and forth with the LFO | |
| D3 | Remove the route | The band returns to the knob value | |

## E. Counts (D32)

| # | Item | Expect | Verdict |
|---|---|---|---|
| E1 | Grain limit 32, four streams at high density (≥ 100 g/s) | The header reads at most "32 / 32"; the four per-stream "N sounding" values add up to the header count | |
| E2 | Grain limit 128 → 32 while it plays | The count falls to ≤ 32 within a fraction of a second and never shows more than 32 afterwards | |

## F. Editor lifecycle (P117 reader lock)

| # | Item | Expect | Verdict |
|---|---|---|---|
| F1 | Close and reopen the editor 10 times while audio plays | Every reopen shows the live strip at once; no stale frozen frame, no blank strip | |
| F2 | Canvas time labels after a reopen | EB Garamond, not the fallback serif (font-load repaint) | |
| F3 | Two editors at once: the Standalone plus the AU in a host, or two plugin instances | Both strips are live; neither freezes or flickers | |

## G. Watch item (R124 INFO — accepted approximation)

| # | Item | Expect | Verdict |
|---|---|---|---|
| G1 | Freeze on, Buffer Length 30 s, S1 Size 2 s, Pitch +12 | The S1 band can be offset from where the grains actually spawn by up to ≈ 8 % of the strip: the harness (FB1) measured spawn delays 0.08 of the span **older** than the band, so the dots appear left of (older than) the band. Note how visible it is; this is NOT a FAIL | |
