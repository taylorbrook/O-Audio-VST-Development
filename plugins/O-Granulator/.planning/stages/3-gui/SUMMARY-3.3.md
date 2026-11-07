# Stage 3 — GUI · Phase 3.3 (Mod matrix view + mod rings) — Summary

**Plugin:** O-Granulator · **Stage / phase:** 3-gui / 3.3 · **GSD phase:** execute · **Date:** 2026-11-07
**Commits:** 00e462b8 (T1–T5) · d3f14f7f (T6–T7) · T8 docs (this commit) · **Logs:** `${TMPDIR}/ogran-33/` (t0, t5, t67, t1-ux1.log, t4-only33.log)

## Result
**D36 is answered with zero audio change.** UX1 replays the page's real route-edit write sequences on the **unmodified** engine (T1, before any 3.3 code): clear, drop (source last), dest swap, source swap and a depth drag, multi-write sequences spread over 0 / 1 / 7 cells, on s1_level / mix / output_gain / feedback, LFO 1 and Onset. Every trajectory is ≤ 1.000× the analytic bound and every curvature ≤ 1.30× the held pre / post references. Both NCs fire at 4.1–22.7×. `ModMatrix` is untouched.

The 3.3 telemetry is in, and it is audio-neutral:
- **Meters:** the input peak is measured per piece in the engine, post input gain and before `mixOut` (in-place safe). The output peak comes from a `findMinAndMax` scan in `processBlock`. Both are max-since-read: a relaxed CAS fetch-max (`ogran::pushMax`) once per block, and the reader exchanges the value to 0.
- **Onset:** an audio-side event counter plus a reader wall clock, with a 200 ms linear pulse.
- **LFO phase:** offset-free, `position() − φ/360` in `tickSources`.
- **Loop gain:** `20·log10 (fbKnobNow · min (1, 0.7/env))`. It is absent at k == 0 or frozen.
- **Reader:** `readTelemetry (t, nowMs)` applies the 250 ms gap rule under `telemetryLock`. The 1-argument form uses `Time::getMillisecondCounterHiRes()`.

The page is not edited: every consumer existed (D35), and `telemetryToVar` already emitted each key behind its flag (P132). UI-03 → **partial**. HANDS-ON-3.3.md and DAW-3.3.md run at verify.

## Gate table
| Gate | Result |
|---|---|
| Entry check | PASS: 6 contract hashes match, `git diff 00a04a89 HEAD` on Source/tests/CMake/modules/scripts is empty, page + i18n `cmp` clean |
| Baseline C (T0, unmodified tree) | 491 PASS, rc 0, 7:36. The normalised extract == 3.2 verify's 491-row extract exactly (see deviation 1). N3–N0 `--strict` + R12 rc 0, HA 79/79, codec 9/9 == B, param-dump clean, overrides ×2 == committed, ufc / probe ×2 / check-i18n / labels / boot clean |
| UX1 (T1, unmodified engine) | 9 rows, 8/8 each. Worst 0.833× of B (clear / drop / swaps), 1.000× (depth drag); curvature 1.028–1.301×; cross-talk 0; \|source\| at the edit LFO1 1.000, Onset 0.664 (non-vacuous) |
| UX1nR / UX1nD | s1_level 4.1×, mix 22.7×, output_gain 15.1×, feedback 22.7× (each > 2×) |
| Harness default (T5) | **520 PASS / 0 FAIL**, rc 0, 7:42 (491 − TL32 − TL32u + 31 new rows) |
| Carried diff vs C | **TL32 / TL32u → TL33 / TL33u only** (R135, attributed) + the new rows. No carried row moved: every audio row, N9 / N10 block-size null and W2o is unchanged |
| MT1–MT5 | out 0.501187 L/R; ×1.99526 at +6 dB; mono R == L · inPeak 0.501187, fb 80 % identical · spike 0.900000 then 0 · frozen tail: inPeak 0, out 0.68 · stale first read and the 300 ms gap read 0 / no onset (onsets 5 → 11), next read normal |
| MT2n / MT4n | ring peak 0.501 → 0.733 with fb 80 % · the post-process in-place buffer reads 0.68 (≠ 0) |
| ON1 / ON2 / ON2n | 90 reads, 12 event-bearing, 12 fired, 0 mismatches, pulse \|err\| 2.1e-8 · idle: 1.000 / 0.750 / 0.500 / 0.250 / 0.000 / 0.000 · sample-clock pulse still 1.000 at +200 ms |
| LP1 / LP2 | Free 1 Hz: worst phase 2.8e-7, φ 90° offset 0, sin 4.1e-8 · Tempo 1/4 @ 120: worst 1.4e-8 |
| LG1–LG3 | −6.4661 dB (want −6.4661); hot (sat 0) −3.03 dB · absent at fb 0 / frozen, present while pending / after unfreeze · LFO → Feedback: 29 / 60 reads present, max −6.468 dB, 0 above −0.4455 · LG3n ramp k = 0 → 0 present |
| M9 / M9L | 0 allocations (var: 222 reads; 8192 after 512: 36 reads; every 3.3 field seen live) · liveness 1 |
| TL33 / TL33u | 28/28 (out 0.447, in 0.305, phase 0.066, loop −10.90 dB) · every has* false before prepare |
| C7 · C8 (INFO) | C7 17.22 % (C 17.19 %) · C8 17.27 %, Δ vs C +0.08 (noise) |
| N3 / N2 / N1 / N0 `--strict`, R12 | rc 0 ×5 (the new side was rebuilt from the 3.3 tree at 08:21) |
| param-dump · overrides | diff-clean · regenerated ×2, byte-identical to the committed file (no shape change; stub not regenerated) |
| Build | VST3 + AU + Standalone, 0 warnings (the harness is 0 warnings too). The Standalone was rebuilt after the install |
| auval · pluginval 10 VST3 + AU | PASS · SUCCESS ×2 |
| HA · codec bytes | 79/79, 142 untouched · 9/9 == B (installed T6 build) |
| ui_frontend_check.js | ALL PASS; NC1 / NC2 fail as they must |
| ui_binding_probe.js | 220/220 ×2; NC rc 2 (219/220) |
| check-i18n · fr · zh | 0 · 0 · 0 |
| check-ui-labels | rc 0; seed = param-dump (220) + overrides; **states default + 10** (`streams-all-enabled` measured in en / fr / zh-Hans) |
| boot-all-uis `--strict-tips` | 0 warn, 0 failed, 0 DEAD, 0 late |
| Page | `index.html` == `v2-ui.html`; unchanged vs 3.2 (`git diff 00a04a89` empty) |

## D36 answer (UX1, the ROADMAP test)
Route edits from every page write order are click-free on the 2.6 matrix as shipped. No `ModMatrix::tick` change was needed, and nothing needed a sign-off.

## Findings
- **The AGC cannot engage at the default `fb_saturation` (20 %).** The saturator `tanh (d·x)/d` with d = 1 + 7·sat caps the AGC input at 1/d. At 20 % that is 1/2.4 = 0.4166, below the 0.7 ceiling. Any fb_saturation above ≈ 6.1 % makes g_agc ≡ 1. The 4-stream, +12 dB, dense configurations all plateaued at env 0.4166. At saturation 0, a hot loop reads g_agc 0.75 (−3.03 dB at fb 100 %).
  - So at default settings the loop-gain readout is simply the knob value, at most −0.45 dB. That sits behind the D34 saturation note.
  - LG1's hot arm and HANDS-ON B5 use saturation 0. No DSP change; recorded for Taylor (it affects how meaningful "AGC × knob" is in practice).
- **TL33 / M9 telemetry cost.** C7 / C8 show no measurable CPU change (+0.03 / +0.08 percentage points vs C, inside the 0.29 spread).

## Deviations
1. **Extract normalisation.** The carried comparison uses `extract33.sh`: the 3.2 `extract.sh` plus PresetManager log-line stripping (3.2 deviation 10) and masking of the TB1 / TB1n read and torn **counts**. Those are wall-clock stress counts (73.0 M vs 72.0 M reads, 9 999 vs 10 571 torn); the verdicts are unchanged. With it, C == 3.2 verify exactly. The raw diff was only those two rows.
2. **LP1 / LP2 formula.** The plan's "phase = frac (N·32/fs) after N cells" is one cell off. After N cells the newest tick is cell N − 1 (it runs at absCount = (N − 1)·32), so the gated value is frac ((N − 1)·32/fs) (Tempo: the ppq at that cell's start). The value ↔ phase identity (lfo == sin (2π (phase + 0.25)) at φ 90°) holds to 4e-8, so the dot sits on the analytic phase of the value it shows. This is a harness-expectation correction, not code.
3. **LG1 hot arm** uses fb_saturation 0 and fb_diffusion 0 with a +12 dB 100 Hz sine (see Findings). With the default saturation the predicate is unreachable by design.
4. **onsetCount is not reset in `prepare`** (the plan said "all prepare-reset"). A reset would make `c != lastOnsetCount` fire a phantom onset on a read within 250 ms of a re-prepare. The counter is monotonic and wraps; the reader detects events by inequality. Every other 3.3 member is reset in `prepare`.
5. **LG3n** reads the knob (0.95 × the `feedback` parameter) from the APVTS, because the engine's `fbKnobRamp` is private. It is the same value the ramp holds once settled (prepare snaps it).
6. **ON1 stimulus.** 10 ms 1 kHz bursts at 0.5 every 250 ms, not single-sample clicks (a single sample does not reliably clear the detector). 12 bursts → 12 events → 12 fired reads.
7. **UX1 at block 32.** UX1 renders at block 32 (one trace entry per cell for the trajectory bound). The MF8 base is otherwise identical, with LFO 2 at 3 Hz added as the source-swap partner. The engine is block-size invariant (N9 / N10).
8. **Screenshot.** `ref-3.3/standalone-t6.jpg` shows the Streams view with a live LFO 1 → S1 Position ring (arc + dot) and the lit LFO 1 chip. The state was injected through the Standalone's `filterState` (feedback 50 %, mod1 LFO 1 → dest 7 at 60 %, LFO 1 1 Hz); Taylor's settings file was backed up and restored. The loop-gain readout and the LFO dot live on the FX / MOD tabs, which a non-interactive capture cannot switch to, and the input is muted (no meters). HANDS-ON-3.3 A, B and D cover them.
9. **C8** is an N 3 × 10 s INFO run at the C7 config, compared against C's C7 (17.19 %), not 3.2's T4 C7 (17.02 %).
10. **M8 is unchanged.** It still uses the 1-argument `readTelemetry` (its verdict does not read any 3.3 field). M9 is the injected-clock extension.

No module, scripts, page or contract edit. REQUIREMENTS UI-03 → partial.

## Carried
- **3.3 verify:** HANDS-ON-3.3.md + DAW-3.3.md (Logic AU; DAW-2.5a 1–8, DAW-2.5b 1–5, DAW-2.6 1–3 + the four 3.3 items). UI-03 completes, and Stage 3 closes, only if both PASS.
- **Stage 4:** the Env 1e30 pin; the harness items C7 calibration and Z6a; a VST3-host DAW pass if COMPAT-03 calls for it; the W2o rule; `uiSnap` not reset in `prepare` (3.2 note).
- **For Taylor (decision, not a defect):** whether the AGC-at-default-saturation finding warrants a readout note on the page or a Stage 4 look at the saturator / AGC staging.
