# Verification: Reverb engine rewrite

**Plugin:** O-SimpleReverb
**Milestone:** reverb-engine-rewrite
**Created:** 2026-10-01
**Phase:** Verify

---

## Verification Summary

**Status:** AUTOMATED CHECKS PASSED — one human gate open: the listening pass. The milestone is not
closed until it is done. (The translations were read and approved in this phase.)
**Version:** 1.14.0 → 2.0.0 (already applied in execute, Task 14; nothing tagged, nothing pushed)

Every number SUMMARY.md reports was re-produced in this phase from a clean rebuild, not copied.
On top of that, the **installed** `O-SimpleReverb-dev.vst3` was hosted in a different host
(pedalboard) and measured with separate numpy / scipy code that shares nothing with
`tests/render-check/measure.h`. The two agree.

| What was re-run | Result |
|---|---|
| Clean rebuild of `O-SimpleReverb-render-check` and `-param-dump` (91 steps, `cmake-build-tests/`) | 0 warnings |
| `render-check` | **126 PASS, 0 FAIL, 0 PEND** |
| `render-check --mutants` | **30 of 30 caught** |
| `render-check --baseline` | identical to `BASELINE-v2.0.0.md` line for line, CPU table aside (within 0.03 points) |
| `render-check --levels` | identical to SUMMARY.md's law-on table |
| `param-dump` against `.planning/params.tsv` | byte-identical; the committed file has not changed since `02bac73f` (v1.14.0) |
| pluginval, strictness 10, in process, GUI tests skipped, installed VST3 | SUCCESS |
| `auval -v aufx OuSr OuDv` | AU VALIDATION SUCCEEDED |
| `check-i18n.js`, `i18n-fr-lint.js`, `i18n-zh-lint.js`, `check-ui-labels.js`, `tests/ui_tip_render_check.js` | all pass; after the flags were flipped: 0 of 32 fr entries unreviewed, the three zh-Hans bodies at `'bt'` |
| Independent measurement of the installed VST3 (below) | worst RT60 error 3.0 % over 30 points |
| Read of `Source/dsp/*.h` and the slot host in `PluginProcessor.cpp` | no defect found (notes below) |

Not re-run: the negative control against v1.14.0 out of tree (51 PASS, 42 FAIL of 93 — SUMMARY.md, Task 13).
`BASELINE.md` is the same tool's output on v1.14.0 and is compared row by row below, which covers the
same ground for the engine gates.

---

## Requirements Verification

From CONTEXT.md success criteria (1–13) and PLAN.md's added criterion 14.

### Criterion 1: `juce::dsp::Reverb` is gone; three engines

**Expected:** Booth / Room / Hall / Ambient on an FDN, Plate on a Dattorro tank, Spring on a dispersive spring.
**Actual:** `Source/dsp/FdnEngine.h` (16 lines, Hadamard), `PlateEngine.h`, `SpringEngine.h`, hosted by two
ring-out slots. CONTEXT asked for 8 lines; 16 was decided at plan time.
**Status:** ✓ PASS

**Evidence:**
- `grep -rn "dsp::Reverb" Source/` is empty.
- `typePresets[]` maps types 0, 1, 2, 5 to `Engine::Fdn`, 3 to `Spring`, 4 to `Plate`; `renderSlot` dispatches on it.
- Tail spectra of any two types correlate at −0.06..+0.05 (v1.14.0: +0.72..+0.96) — they are not one tank.

### Criterion 2: RT60 = base × DECAY for all six types, holding across SIZE

**Expected:** within ±10 % of the table at DECAY 0.5 / 1.0 / 2.0×, and at SIZE 0 / 100.
**Actual:** render-check worst +2.8 % through `processBlock` (Plate, SIZE 0); +5.3 % driven directly over
three sample rates (Booth, 96 kHz, 0.5×, size ×0.5).
**Status:** ✓ PASS

**Evidence:**
- 12 `processBlock` gates and 6 engine-header gates (27 points each) pass; `decayDead` and `sizeIsDecay`
  mutants fail them on all three engines.
- **Independent:** installed VST3 in pedalboard, own T30 code (4th-order Butterworth 354–1414 Hz, Schroeder
  integral, fit −5..−35 dB), 30 points: −1.6..+3.0 %, worst Hall at SIZE 0. Ambient at 2.0× reads 14.06 s.
- `getTailLengthSeconds()` returns 15.0 (`PluginProcessor.h:165`).

### Criterion 3: Early reflections at the output, different in L and R

**Expected:** discrete taps before the late tail, L ≠ R, still there with the tail at its shortest.
**Actual:** 8 taps a side, none within 0.1 ms of the other side's; 4–5 a side stand within 12 dB of the
largest at DECAY 1.0× and at 0.5×.
**Status:** ✓ PASS

**Evidence:**
- Four gates (Booth, Room, Hall, Ambient) plus the tap-table gate on the header; `earlyMuted` mutant fails them.
- First-eight arrival lists below: v1.14.0's L and R are the same list offset by 0.52 ms; v2.0.0's share no time.

### Criterion 4: Spring chirps; Plate shimmer builds in the tail and never runs away

**Expected:** frequency-dependent delay per echo, repeating at the round-trip time; octave energy that grows
over the tail; loop gain under 1 everywhere.
**Actual:** first echo 3 kHz 9.71 ms behind 1 kHz, second echo 14.36 ms, echoes 32.96 ms apart; 600 / 300 Hz
goes −35.2 → −13.3 dB; the plate asked for an endless tail still decays (−67.7 / −93.9 / −118.3 dB at
10 / 20 / 30 s with the shifter share forced to 1.0).
**Status:** ✓ PASS (measured). How the chirp and the shimmer *sound* is criterion 13.

**Evidence:**
- Chirp, band-limit, bloom and no-runaway gates pass; `springNoChirp`, `springOpenBand`, `noShimmer`,
  `decayStuck` mutants fail them.
- **Independent:** Spring's envelope autocorrelation peaks at 32.75 ms (0.89); Room, Hall, Plate show no
  period (0.06–0.12). Band envelope peak in the first echo, zero-phase filters: 1 kHz 0.96 ms, 2 kHz 2.58 ms,
  3 kHz 8.21 ms on Spring; under 0.6 ms spread on Room and Hall. Energy above 6 kHz: Spring −34.0 dB, Room +10.3 dB.
  Plate 600 / 300 Hz: −44.2 dB during a burst, −18.6 dB after (+25.6); Room −135 / −107 dB (nothing there).
  All six types at DECAY 2.0×, SIZE 100 under 10 s of noise: finite, and falling 5 s and 19 s on.
- The baseline tool's "band first arrival" column reads Spring +1.7 ms, not 9.7: that is the first-arrival
  metric stage 3 replaced (SUMMARY.md, stage 3, departure 1). It still separates Spring from the rest (0.0).

### Criterion 5: TYPE, SIZE, DECAY, CHARACTER, LOW CUT moves are click-free

**Expected:** no worse than the existing excess-step gates; a pitch bend on SIZE is not a failure.
**Actual:** all section 2 and 5 gates pass. SIZE 0↔100: −63.0..−67.5 dB; DECAY 0.5↔2.0: −56.6..−68.5 dB;
TYPE 1.80× (limit 2); TYPE every 16 ms −68.7 / −73.1 dB; all 48 presets 64 ms apart −60.1 dB above 8 kHz.
**Status:** ✓ PASS

**Evidence:** `noGlide` and `hardSteal` mutants fail the SIZE / DECAY / rapid-switch / preset-sweep gates.
CHARACTER and LOW CUT gates are v1.12.0's, unchanged, and pass.

### Criterion 6: `params.tsv` unchanged; a v1.14.0 state loads with equal values

**Status:** ✓ PASS

**Evidence:** fresh `param-dump` output `cmp`-identical to the committed file, which `git diff 02bac73f HEAD`
shows untouched. `tests/fixtures/state-v1.14.0.bin` loads with 8 parameter values and the UI language equal
(9 checked, 0 wrong). No change under `Source/PluginEditor.*`, `index.html` or CSS in the five commits.

### Criterion 7: 48 presets re-voiced; inserts ≤ +5 dB; types level-matched

**Status:** ✓ PASS (by level and decay time). Chosen by number, not by ear — criterion 13.

**Evidence:** 8 per type; six Sends at 100 / 0; inserts +0.2 (Booth - Voiceover) .. +2.9 dB (Spring - Surf
Guitar); type spread 0.07 dB stereo and 0.07 dB mono (target 0.07). Installed bank: `.factory-version` 2.0.0,
48 files.

### Criterion 8: v1.14.0 baseline captured and compared row by row

**Status:** ✓ PASS — the comparison is the next section.

### Criterion 9: No allocation in `processBlock`; stable at 44.1 / 48 / 96 kHz and with oversized blocks

**Status:** ✓ PASS

**Evidence:** 0 allocations over 600 blocks (256 / 100 / 1024, every control moving; v1.14.0: 4). A 1024
block on a processor prepared for 256 renders what 256-blocks do, difference 0. All types finite and
non-silent at 44.1 / 96 / 192 kHz, DECAY 2.0×, SIZE 100. NaN into the playing slot: recovers within a chunk.
`noChunking` and `noNanGuard` mutants fail these. Reading the code: every buffer is sized in
`prepareToPlay` from the running rate; `processChunk`'s `setSize` calls only shrink.

### Criterion 10: render-check passes; v1.14.0 fails the new gates

**Status:** ✓ PASS

**Evidence:** 126 PASS here. On v1.14.0 out of tree, 42 of 93 fail (execute, not re-run). 30 of 30 mutants
caught, re-run here — each new gate has been seen to fail.

### Criterion 11: Build succeeds without warnings

**Status:** ✓ PASS

**Evidence:** clean rebuild of the test targets (which compile `Source/`): 0 warnings. Standalone rebuilt in
`build/` in this phase: 0 warnings. The installed VST3 / AU are newer than every file under `Source/`.

### Criterion 12: pluginval (Level 5+) and `auval -v`

**Status:** ✓ PASS — strictness 10 SUCCESS; `auval -v aufx OuSr OuDv` AU VALIDATION SUCCEEDED. Both bundles report 2.0.0.

### Criterion 13: Listening pass in a DAW on the re-voiced bank before any tag

**Status:** ☐ OPEN — Taylor. Nothing measured stands in for this. The list is in "Human gates" below.

### Criterion 14: `tip.type` / `tip.decay` / `tip.size` in en / fr / zh-Hans; fr and zh-Hans read by Taylor

**Status:** ✓ PASS — Taylor read `I18N-REVIEW.md` and approved it on 2026-10-01. The three fr entries are
`reviewed: true` and the three zh-Hans entries `'bt'` (`Source/ui/public/js/i18n.js`).

**Evidence:** after the flip, `check-i18n.js` ALL CHECKS PASS with 0 of 32 French entries unreviewed;
`i18n-fr-lint.js` and `i18n-zh-lint.js` 0 findings for this plugin; `tests/ui_tip_render_check.js` ALL CHECKS
PASSED. `i18n.js` is embedded in the binary, so the plugin was rebuilt and reinstalled afterwards (below).

---

## Baseline comparison: v1.14.0 → v2.0.0, row by row

`BASELINE.md` (commit `02bac73f`) against `BASELINE-v2.0.0.md`, both from `render-check --baseline`:
48 kHz, block 512, WET 100 / DRY 0, CHARACTER 0, LOW CUT off, unit impulse in both channels. The v2.0.0
file was regenerated in this phase and matches the committed one.

What the rows say, in order:

- **Decay.** v1.14.0 is off target by more than 5 % on all 30 rows (−82 %..+1200 %) and lands every type
  on 10.4–11.2 s at DECAY 2.0×. v2.0.0 is within 3 % on all 30.
- **SIZE.** v1.14.0's RT60 moves with SIZE (Hall 1.14 → 3.49 s) and its first arrival does not move at all.
  v2.0.0's RT60 holds (Hall 2.995 → 3.017 s) and the first arrival moves 1.15–19.2 ms.
- **Stereo.** Booth +0.54 → −0.10. Spring stays near +0.33, by design (one spring is in both outputs).
- **High and low bands.** The 8 kHz RT60 is now a steady share of mid per type (Hall 55–58 %, Ambient 62–68 %,
  Booth 75–77 %) instead of 28–89 % depending on DECAY. Spring's 125 Hz RT60 is shorter than its mid
  (1.91 s against 2.50 s): the loop's 100 Hz high-pass.
- **Early structure.** About 100 arrivals a side within 20 dB in the first 60 ms → 20–75 on the room types at
  SIZE 50 (4–5 within 12 dB, which is what the gate reads). The first arrival is 21–25 ms earlier on every room type (Booth 28.3 → 3.75 ms): v1.14.0's was
  the comb's first return, v2.0.0's is the first tap after the pre-delay.
- **Peak.** The impulse response's peak is 9–13 dB higher on the room types (Booth −13.4 → −0.3 dBFS),
  with nearly the same total energy (7.2 → 7.5 dB): the energy that was spread over a click train is now in
  a few taps. See note 1 under "Non-critical".
- **Density.** Room types reach 0.94–1.05 at 100 ms at SIZE 50 (v1.14.0: 0.81–0.97). Under 0.9: Plate from
  SIZE 50 up (0.76–0.81 at SIZE 50, 0.70 at 100 — the known one), Ambient at SIZE 100 (0.76) and Hall at
  SIZE 100 (0.89), where the lines are longest. Spring reads 0.82–0.83 at SIZE 50, as discrete echoes should.
- **Resonances.** Every pair +0.72..+0.96 → −0.06..+0.05.
- **CPU.** FDN types 1.8–2.0×, Spring 2.7×, Plate 0.9–1.1×. A ring-out runs two slots, so the worst moment
  is about double the figure. Reported only (CONTEXT: no ceiling).

### Decay time and stereo

| Type | DECAY | SIZE | Target (s) | Mid RT60 v1.14.0 → v2.0.0 (s) | vs target | 8 kHz RT60 (s) | 125 Hz RT60 (s) | L/R correlation | Verdict |
|---|---|---|---|---|---|---|---|---|---|
| Booth | 0.5x | 50 | 0.20 | 0.668 → 0.197 | +234 % → -2 % | 0.350 → 0.151 | 0.681 → 0.210 | +0.522 → -0.086 | on target |
| Booth | 1.0x | 50 | 0.40 | 0.719 → 0.397 | +80 % → -1 % | 0.405 → 0.302 | 0.707 → 0.413 | +0.537 → -0.098 | on target |
| Booth | 2.0x | 50 | 0.80 | 10.400 → 0.803 | +1200 % → +0 % | 2.934 → 0.603 | 11.237 → 0.812 | +0.448 → -0.091 | on target |
| Booth | 1.0x | 0 | 0.40 | 0.687 → 0.405 | +72 % → +1 % | 0.395 → 0.302 | 0.687 → 0.402 | +0.525 → -0.067 | on target |
| Booth | 1.0x | 100 | 0.40 | 0.754 → 0.393 | +88 % → -2 % | 0.416 → 0.304 | 0.739 → 0.378 | +0.509 → -0.034 | on target |
| Room | 0.5x | 50 | 0.55 | 0.777 → 0.556 | +41 % → +1 % | 0.386 → 0.381 | 0.799 → 0.574 | +0.052 → -0.013 | on target |
| Room | 1.0x | 50 | 1.10 | 1.038 → 1.102 | -6 % → +0 % | 0.679 → 0.714 | 1.029 → 1.072 | +0.011 → -0.004 | on target |
| Room | 2.0x | 50 | 2.20 | 10.773 → 2.181 | +390 % → -1 % | 4.240 → 1.387 | 11.281 → 2.265 | -0.010 → -0.023 | on target |
| Room | 1.0x | 0 | 1.10 | 0.866 → 1.074 | -21 % → -2 % | 0.591 → 0.705 | 0.860 → 1.122 | +0.030 → -0.003 | on target |
| Room | 1.0x | 100 | 1.10 | 1.278 → 1.093 | +16 % → -1 % | 0.795 → 0.746 | 1.275 → 1.134 | +0.009 → -0.047 | on target |
| Hall | 0.5x | 50 | 1.50 | 0.967 → 1.530 | -36 % → +2 % | 0.646 → 0.884 | 0.980 → 1.645 | +0.010 → -0.004 | on target |
| Hall | 1.0x | 50 | 3.00 | 1.734 → 2.995 | -42 % → -0 % | 1.272 → 1.712 | 1.769 → 3.146 | +0.005 → -0.010 | on target |
| Hall | 2.0x | 50 | 6.00 | 11.049 → 6.027 | +84 % → +0 % | 5.996 → 3.332 | 11.352 → 6.304 | -0.016 → +0.003 | on target |
| Hall | 1.0x | 0 | 3.00 | 1.137 → 2.995 | -62 % → -0 % | 0.897 → 1.670 | 1.135 → 3.317 | +0.022 → +0.006 | on target |
| Hall | 1.0x | 100 | 3.00 | 3.494 → 3.017 | +16 % → +1 % | 2.137 → 1.771 | 3.630 → 3.084 | +0.018 → +0.001 | on target |
| Spring | 0.5x | 50 | 1.25 | 0.750 → 1.251 | -40 % → +0 % | 0.445 → 1.202 | 0.759 → 1.008 | +0.424 → +0.337 | on target |
| Spring | 1.0x | 50 | 2.50 | 0.904 → 2.505 | -64 % → +0 % | 0.669 → 2.273 | 0.877 → 1.908 | +0.351 → +0.300 | on target |
| Spring | 2.0x | 50 | 5.00 | 10.848 → 4.988 | +117 % → -0 % | 5.048 → 4.417 | 11.282 → 3.460 | +0.327 → +0.330 | on target |
| Spring | 1.0x | 0 | 2.50 | 0.804 → 2.497 | -68 % → -0 % | 0.606 → 2.331 | 0.797 → 1.837 | +0.366 → +0.338 | on target |
| Spring | 1.0x | 100 | 2.50 | 1.025 → 2.505 | -59 % → +0 % | 0.743 → 2.262 | 0.989 → 1.964 | +0.323 → +0.325 | on target |
| Plate | 0.5x | 50 | 1.25 | 0.867 → 1.266 | -31 % → +1 % | 0.540 → 1.060 | 0.840 → 1.449 | +0.022 → +0.022 | on target |
| Plate | 1.0x | 50 | 2.50 | 1.279 → 2.514 | -49 % → +1 % | 0.925 → 1.846 | 1.227 → 2.745 | +0.010 → +0.023 | on target |
| Plate | 2.0x | 50 | 5.00 | 10.935 → 4.940 | +119 % → -1 % | 5.343 → 3.043 | 11.258 → 5.514 | -0.013 → +0.004 | on target |
| Plate | 1.0x | 0 | 2.50 | 0.984 → 2.571 | -61 % → +3 % | 0.747 → 1.688 | 0.945 → 2.801 | -0.008 → +0.010 | on target |
| Plate | 1.0x | 100 | 2.50 | 1.803 → 2.515 | -28 % → +1 % | 1.216 → 1.997 | 1.799 → 2.698 | +0.022 → +0.006 | on target |
| Ambient | 0.5x | 50 | 3.50 | 1.026 → 3.505 | -71 % → +0 % | 0.849 → 2.382 | 1.006 → 3.683 | +0.000 → +0.004 | on target |
| Ambient | 1.0x | 50 | 7.00 | 2.165 → 6.983 | -69 % → -0 % | 1.795 → 4.561 | 2.126 → 7.263 | +0.038 → +0.002 | on target |
| Ambient | 2.0x | 50 | 14.00 | 11.166 → 14.165 | -20 % → +1 % | 7.974 → 8.845 | 11.300 → 14.715 | -0.014 → +0.001 | on target |
| Ambient | 1.0x | 0 | 7.00 | 1.252 → 7.067 | -82 % → +1 % | 1.114 → 4.439 | 1.223 → 7.210 | +0.006 → -0.020 | on target |
| Ambient | 1.0x | 100 | 7.00 | 6.510 → 7.044 | -7 % → +1 % | 4.435 → 4.757 | 6.673 → 7.246 | +0.015 → +0.011 | on target |

### Impulse response structure

| Type | DECAY | SIZE | First arrival L / R (ms) | Early arrivals L / R (within 20 dB) | L without R | Echo density @ 100 ms | Mixing time (ms) | Peak (dBFS) | Energy (dB) | −60 dB at (s) |
|---|---|---|---|---|---|---|---|---|---|---|
| Booth | 0.5x | 50 | 28.29 / 28.29 → 3.75 / 3.98 | 111 / 125 → 30 / 31 | 52 → 26 | 0.92 → 0.97 | 62 → 2 | -13.4 → 0.8 | 7.0 → 7.2 | 0.62 → 0.18 |
| Booth | 1.0x | 50 | 28.29 / 28.29 → 3.75 / 3.98 | 110 / 128 → 39 / 48 | 53 → 33 | 0.97 → 1.03 | 58 → 2 | -13.4 → -0.3 | 7.2 → 7.5 | 0.66 → 0.35 |
| Booth | 2.0x | 50 | 28.29 / 28.29 → 3.75 / 3.98 | 108 / 124 → 60 / 75 | 51 → 50 | 0.83 → 1.05 | 178 → 5 | -13.4 → -1.4 | 13.4 → 8.4 | 9.63 → 0.69 |
| Booth | 1.0x | 0 | 28.29 / 28.29 → 3.38 / 3.50 | 110 / 129 → 66 / 68 | 53 → 49 | 0.97 → 0.97 | 62 → 2 | -13.4 → -2.5 | 7.2 → 7.2 | 0.64 → 0.35 |
| Booth | 1.0x | 100 | 28.29 / 28.29 → 4.52 / 4.98 | 110 / 128 → 13 / 14 | 53 → 12 | 0.97 → 0.95 | 58 → 5 | -13.4 → 2.0 | 7.3 → 8.2 | 0.69 → 0.35 |
| Room | 0.5x | 50 | 40.29 / 40.81 → 17.52 / 18.29 | 99 / 119 → 20 / 45 | 60 → 14 | 0.92 → 0.96 | 85 → 22 | -16.3 → -3.8 | 4.2 → 4.0 | 0.73 → 0.53 |
| Room | 1.0x | 50 | 40.29 / 40.81 → 17.52 / 18.29 | 99 / 113 → 24 / 45 | 65 → 18 | 0.88 → 0.98 | 95 → 22 | -16.4 → -4.9 | 5.5 → 4.4 | 0.97 → 1.00 |
| Room | 2.0x | 50 | 40.29 / 40.81 → 17.52 / 18.29 | 98 / 111 → 26 / 47 | 64 → 20 | 0.81 → 0.96 | 318 → 22 | -16.5 → -6.0 | 11.8 → 5.2 | 9.88 → 1.97 |
| Room | 1.0x | 0 | 40.29 / 40.81 → 16.27 / 16.65 | 100 / 113 → 64 / 72 | 66 → 42 | 0.89 → 1.00 | 92 → 15 | -16.5 → -7.1 | 5.2 → 4.1 | 0.82 → 1.01 |
| Room | 1.0x | 100 | 40.29 / 40.81 → 20.06 / 21.58 | 98 / 112 → 19 / 20 | 64 → 11 | 0.89 → 0.93 | 95 → 50 | -16.4 → -2.6 | 6.0 → 5.2 | 1.19 → 1.00 |
| Hall | 0.5x | 50 | 78.06 / 78.58 → 55.06 / 56.58 | 101 / 111 → 25 / 33 | 66 → 15 | 0.92 → 0.97 | 48 → 58 | -21.8 → -7.5 | 0.8 → 2.6 | 0.98 → 1.41 |
| Hall | 1.0x | 50 | 78.06 / 78.58 → 55.06 / 56.58 | 98 / 111 → 25 / 33 | 63 → 15 | 0.90 → 0.98 | 65 → 55 | -21.4 → -8.6 | 3.0 → 3.2 | 1.72 → 2.76 |
| Hall | 2.0x | 50 | 78.06 / 78.58 → 55.06 / 56.58 | 96 / 111 → 26 / 33 | 64 → 16 | 0.89 → 0.98 | 70 → 55 | -21.1 → -9.7 | 9.3 → 4.2 | 10.34 → 5.49 |
| Hall | 1.0x | 0 | 78.06 / 78.58 → 52.52 / 53.29 | 98 / 109 → 57 / 67 | 65 → 36 | 0.89 → 1.02 | 42 → 35 | -21.6 → -10.8 | 1.8 → 2.9 | 1.16 → 2.79 |
| Hall | 1.0x | 100 | 78.06 / 78.58 → 60.12 / 63.15 | 97 / 111 → 9 / 11 | 63 → 7 | 0.90 → 0.89 | 62 → 135 | -21.3 → -6.3 | 5.0 → 3.8 | 3.34 → 2.74 |
| Spring | 0.5x | 50 | 45.52 / 45.54 → 32.19 / 36.15 | 102 / 111 → 93 / 70 | 56 → 70 | 0.99 → 0.83 | 12 → 10 | -23.6 → -23.0 | -0.3 → -0.4 | 0.76 → 1.31 |
| Spring | 1.0x | 50 | 45.52 / 45.54 → 32.19 / 36.15 | 102 / 111 → 93 / 70 | 57 → 67 | 0.97 → 0.83 | 12 → 10 | -23.6 → -24.1 | 0.6 → 0.9 | 0.90 → 2.56 |
| Spring | 2.0x | 50 | 45.52 / 45.54 → 32.19 / 36.15 | 102 / 108 → 94 / 70 | 61 → 68 | 1.05 → 0.82 | 12 → 10 | -23.6 → -25.2 | 8.7 → 2.3 | 10.46 → 5.02 |
| Spring | 1.0x | 0 | 45.52 / 45.54 → 23.94 / 26.90 | 102 / 110 → 99 / 72 | 57 → 70 | 0.97 → 0.94 | 12 → 2 | -23.6 → -25.0 | 0.3 → 0.9 | 0.81 → 2.57 |
| Spring | 1.0x | 100 | 45.52 / 45.54 → 43.17 / 48.48 | 102 / 111 → 97 / 48 | 58 → 82 | 0.99 → 1.14 | 12 → 12 | -23.6 → -23.0 | 0.9 → 1.0 | 1.01 → 2.54 |
| Plate | 0.5x | 50 | 33.29 / 33.81 → 13.06 / 14.71 | 97 / 118 → 94 / 74 | 60 → 72 | 0.85 → 0.81 | 130 → 188 | -16.6 → -13.1 | 5.5 → 6.1 | 0.78 → 1.18 |
| Plate | 1.0x | 50 | 33.29 / 33.81 → 13.06 / 14.71 | 97 / 115 → 94 / 74 | 63 → 72 | 0.82 → 0.77 | 162 → 210 | -16.8 → -14.2 | 7.1 → 6.3 | 1.14 → 2.24 |
| Plate | 2.0x | 50 | 33.29 / 33.81 → 13.06 / 14.71 | 98 / 113 → 94 / 74 | 66 → 72 | 0.79 → 0.76 | 185 → 210 | -15.9 → -15.3 | 13.0 → 6.6 | 9.56 → 4.40 |
| Plate | 1.0x | 0 | 33.29 / 33.81 → 11.58 / 12.75 | 97 / 115 → 105 / 88 | 62 → 72 | 0.82 → 0.92 | 172 → 92 | -16.8 → -17.8 | 6.5 → 5.9 | 0.89 → 2.24 |
| Plate | 1.0x | 100 | 33.29 / 33.81 → 15.15 / 17.48 | 97 / 114 → 104 / 78 | 63 → 82 | 0.82 → 0.70 | 170 → 255 | -16.8 → -15.2 | 7.9 → 6.7 | 1.60 → 2.25 |
| Ambient | 0.5x | 50 | 62.12 / 62.65 → 41.33 / 43.23 | 102 / 108 → 24 / 30 | 65 → 14 | 0.91 → 0.95 | 68 → 62 | -22.5 → -11.7 | -0.0 → 1.3 | 1.04 → 3.21 |
| Ambient | 1.0x | 50 | 62.12 / 62.65 → 41.33 / 43.23 | 100 / 110 → 24 / 30 | 60 → 14 | 0.89 → 0.94 | 162 → 88 | -22.3 → -12.8 | 2.7 → 2.3 | 2.10 → 6.40 |
| Ambient | 2.0x | 50 | 62.12 / 62.65 → 41.33 / 43.23 | 99 / 110 → 24 / 30 | 60 → 14 | 0.87 → 0.94 | 178 → 88 | -22.1 → -13.9 | 8.8 → 3.4 | 10.51 → 12.91 |
| Ambient | 1.0x | 0 | 62.12 / 62.65 → 38.17 / 39.10 | 102 / 110 → 48 / 64 | 61 → 31 | 0.91 → 0.98 | 85 → 35 | -22.4 → -15.1 | 0.9 → 2.3 | 1.27 → 6.47 |
| Ambient | 1.0x | 100 | 62.12 / 62.65 → 47.65 / 51.44 | 99 / 111 → 6 / 8 | 59 → 6 | 0.89 → 0.76 | 278 → 128 | -22.1 → -10.6 | 6.3 → 2.6 | 6.15 → 6.35 |

### Early arrivals at DECAY 1.0x, SIZE 50 (ms after the left channel's first arrival, first eight)

| Type | Side | v1.14.0 | v2.0.0 |
|---|---|---|---|
| Booth | L | 0.00+, 0.52+, 1.65+, 2.17+, 3.65+, 4.17+, 5.08-, 5.44+, ... | 0.00+, 0.67-, 1.44+, 2.23-, 3.12+, 4.04-, 4.98+, 5.38+, ... |
| Booth | R | 0.00+, 0.52+, 1.65+, 2.17+, 3.65+, 4.17+, 5.08-, 5.44+, ... | 0.23+, 0.96-, 2.50-, 2.85+, 4.35-, 5.27+, 5.73-, 7.48+, ... |
| Room | L | 0.00+, 1.65+, 3.65+, 5.08-, 5.44+, 6.94+, 7.73-, 8.50+, ... | 0.00+, 2.19-, 4.79+, 6.00+, 7.44-, 8.58+, 9.50+, 10.40+, ... |
| Room | R | 0.52+, 2.17+, 4.17+, 6.12-, 7.46+, 7.77-, 9.02+, 9.77-, ... | 0.77+, 3.17-, 3.90+, 6.00+, 8.31-, 8.58+, 9.50+, 14.52-, ... |
| Hall | L | 0.02+, 1.67+, 3.67+, 5.10-, 5.46+, 6.96+, 7.75-, 8.52+, ... | 0.00+, 4.38-, 9.56+, 14.85-, 16.50+, 20.79+, 24.10+, 25.15+, ... |
| Hall | R | 0.54+, 2.19+, 4.19+, 6.15-, 7.48+, 7.79-, 9.04+, 9.79-, ... | 1.52+, 6.29-, 7.77+, 16.60-, 19.00+, 24.10+, 25.15+, 29.02-, ... |
| Spring | L | 0.04-, 1.69-, 3.19+, 3.69-, 5.12+, 5.48-, 6.08+, 6.77+, ... | 0.71+, 1.23-, 1.88-, 2.42-, 2.90-, 3.35-, 3.79-, 4.42+, ... |
| Spring | R | 0.56-, 2.21-, 2.96+, 3.71+, 4.21-, 4.62+, 5.38+, 6.00-, ... | 4.60+, 5.04-, 5.58-, 6.06-, 6.48-, 7.23-, 7.58-, 7.94-, ... |
| Plate | L | 0.00+, 1.65+, 3.65+, 5.08-, 5.44+, 6.94+, 7.73-, 8.15+, ... | 0.00+, 3.79-, 5.08-, 7.58-, 8.88+, 9.50-, 10.17-, 11.38-, ... |
| Plate | R | 0.52+, 2.17+, 4.17+, 6.12-, 7.46+, 7.77-, 9.02+, 9.77-, ... | 1.65+, 5.25-, 6.42-, 8.85-, 10.02+, 10.96-, 12.46-, 13.62+, ... |
| Ambient | L | 0.02+, 1.67+, 3.67+, 5.10-, 5.46+, 6.96+, 7.75-, 8.52+, ... | 0.00+, 5.46-, 11.96+, 18.56-, 23.79+, 25.98+, 33.12+, 33.56-, ... |
| Ambient | R | 0.54+, 2.19+, 4.19+, 6.15-, 7.48+, 7.79-, 9.04+, 9.79-, ... | 1.90+, 7.88-, 9.71+, 20.75-, 23.73+, 33.10-, 33.71-, 36.27-, ... |

### Do two types share resonances? (tail-spectrum correlation, 200–2000 Hz)

| Pair | v1.14.0 | v2.0.0 |
|---|---|---|
| Booth / Room | +0.75 | -0.06 |
| Booth / Hall | +0.72 | -0.01 |
| Booth / Spring | +0.78 | -0.06 |
| Booth / Plate | +0.73 | -0.04 |
| Booth / Ambient | +0.72 | -0.04 |
| Room / Hall | +0.91 | -0.01 |
| Room / Spring | +0.79 | +0.04 |
| Room / Plate | +0.92 | -0.05 |
| Room / Ambient | +0.91 | +0.00 |
| Hall / Spring | +0.76 | +0.01 |
| Hall / Plate | +0.93 | +0.04 |
| Hall / Ambient | +0.96 | -0.03 |
| Spring / Plate | +0.76 | +0.05 |
| Spring / Ambient | +0.76 | +0.03 |
| Plate / Ambient | +0.94 | +0.00 |

### When each band first arrives (ms; DECAY 1.0x, SIZE 50)

| Type | 300 Hz | 600 Hz | 1 kHz | 2 kHz | 3 kHz | 3.8 kHz | 3 kHz − 1 kHz |
|---|---|---|---|---|---|---|---|
| Booth | 7.7 → -0.1 | 2.1 → 0.1 | 0.8 → 0.0 | 0.4 → 0.0 | 0.1 → 0.0 | 0.1 → 0.0 | -0.6 → +0.0 |
| Room | 17.7 → 0.0 | 1.5 → 0.0 | 0.8 → 0.0 | 0.4 → 0.0 | 0.1 → 0.0 | 0.1 → 0.0 | -0.6 → +0.0 |
| Hall | 17.8 → 0.2 | 1.6 → 0.0 | 0.9 → 0.0 | 0.4 → 0.0 | 0.2 → 0.0 | 0.1 → 0.0 | -0.7 → +0.0 |
| Spring | 14.1 → 0.5 | 6.9 → 0.8 | 4.1 → 1.0 | 5.8 → 1.5 | 2.0 → 2.8 | 2.0 → 4.1 | -2.1 → +1.7 |
| Plate | 6.6 → 1.3 | 2.1 → 0.7 | 0.8 → 0.4 | 0.4 → 0.2 | 0.1 → 0.1 | 0.1 → 0.1 | -0.7 → -0.3 |
| Ambient | 17.8 → 0.1 | 2.2 → 0.0 | 0.9 → 0.0 | 0.4 → 0.0 | 0.2 → 0.0 | 0.1 → 0.0 | -0.7 → +0.0 |

### CPU (one core, block 512; reported, never a verdict)

| Type | 48 kHz v1.14.0 → v2.0.0 | × | 96 kHz v1.14.0 → v2.0.0 | × |
|---|---|---|---|---|
| Booth | 0.33 % → 0.62 % | 1.9 | 0.65 % → 1.32 % | 2.0 |
| Room | 0.32 % → 0.65 % | 2.0 | 0.66 % → 1.40 % | 2.1 |
| Hall | 0.36 % → 0.66 % | 1.8 | 0.75 % → 1.38 % | 1.8 |
| Spring | 0.43 % → 1.17 % | 2.7 | 0.90 % → 2.34 % | 2.6 |
| Plate | 0.36 % → 0.34 % | 0.9 | 0.74 % → 0.81 % | 1.1 |
| Ambient | 0.36 % → 0.65 % | 1.8 | 0.75 % → 1.39 % | 1.9 |

---

## Independent measurement of the installed plugin

`O-SimpleReverb-dev.vst3` 2.0.0 from `~/Library/Audio/Plug-Ins/VST3`, loaded with pedalboard (Python 3.12
through `uv`, nothing installed into the repo), parameters set by normalised value, 48 kHz, block 512.
Mid RT60 by this phase's own code; first arrival = first sample within 40 dB of the peak; L/R correlation
over 0.25–0.75 × RT60.

| Type | 0.5× | 1.0× | 2.0× | SIZE 0 | SIZE 100 | Worst vs target | render-check's worst | First arrival, SIZE 0 → 100 (ms) |
|---|---|---|---|---|---|---|---|---|
| Booth (0.40 s) | 0.198 | 0.397 | 0.804 | 0.407 | 0.394 | +1.7 % | −1.8 % | 3.38 → 4.52 |
| Room (1.1 s) | 0.548 | 1.102 | 2.232 | 1.126 | 1.117 | +2.4 % | −2.3 % | 16.27 → 20.06 |
| Hall (3.0 s) | 1.497 | 3.023 | 6.148 | 3.091 | 3.046 | +3.0 % | +2.0 % | 52.52 → 60.12 |
| Spring (2.5 s) | 1.264 | 2.493 | 4.982 | 2.460 | 2.487 | −1.6 % | −0.2 % | 23.67 → 42.92 |
| Plate (2.5 s) | 1.255 | 2.499 | 5.012 | 2.532 | 2.513 | +1.3 % | +2.8 % | 11.58 → 15.15 |
| Ambient (7.0 s) | 3.533 | 7.081 | 14.055 | 7.069 | 7.050 | +1.2 % | +1.2 % | 38.17 → 47.65 |

First arrivals match the baseline tool to the sample on five types and to 0.27 ms on Spring (a different
threshold on a chirped onset). The two measurers differ by up to 3.2 points on a single row (Hall, SIZE 0), which is the
scatter BASELINE.md's own notes give for one realisation of a noise-like tail.

One false start, recorded because it looked like a plugin fault for a moment: the second script raised
"array must not contain infs or NaNs" on Room. The NaN was the script's (the log of a zero-phase-filtered
envelope that had rung below zero). With the plugin output asserted finite first, it runs clean.

---

## Build Verification

### Release Build

```bash
ninja -C cmake-build-tests -t clean O-SimpleReverb-render-check O-SimpleReverb-param-dump
ninja -C cmake-build-tests O-SimpleReverb-render-check O-SimpleReverb-param-dump   # 91 steps
ninja -C build O-SimpleReverb_Standalone                                           # 13 steps
```

**Result:** Success
**Warnings:** None

The VST3 and AU that were measured above are stage 4's (07:33 on 2026-10-01; nothing under `Source/` was
newer). After the six review flags were flipped in `i18n.js` — metadata no code reads, but the file is
embedded — `./scripts/build-and-install.sh O-SimpleReverb` ran again (0 warnings), pluginval strictness 10
and `auval -v` were repeated on that build (both pass), and the Standalone was relinked. No DSP source
changed between the two builds.

### Installation

**VST3 installed:** ✓ `O-SimpleReverb-dev.vst3`, 2.0.0
**AU installed:** ✓ `O-SimpleReverb-dev.component`, 2.0.0
**Alternate variant on disk:** none (the unsuffixed release build was swept in stage 1 — same AU triple)
**Standalone:** rebuilt in this phase for the listening pass —
`build/plugins/O-SimpleReverb/O-SimpleReverb_artefacts/Release/Standalone/O-SimpleReverb-dev.app`

---

## Pluginval Results

```bash
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --skip-gui-tests \
  --validate-in-process --validate ~/Library/Audio/Plug-Ins/VST3/O-SimpleReverb-dev.vst3
auval -v aufx OuSr OuDv
```

**Level 10 result:** PASS (SUCCESS)
**auval:** AU VALIDATION SUCCEEDED

---

## Functional Testing

### Parameter Verification

| Parameter | Expected Behavior | Result |
|-----------|-------------------|--------|
| TYPE | picks the engine and voicing; old tail rings out on a change | ✓ (ring-out gate: −3.7 dB at 200 ms, decays at its own 3.03 s) |
| DECAY | multiplies the type's decay time | ✓ (30 rows, two measurers) |
| SIZE | moves the structure, not the tail time; glides | ✓ |
| CHARACTER | unchanged from v1.12.0 | ✓ (7 gates) |
| WET / DRY | unchanged; smoothed | ✓ (reference burst −58.4 dB) |
| LOW CUT freq / on | unchanged | ✓ (2 gates) |

### Audio Processing

| Test | Expected | Result |
|------|----------|--------|
| Signal in → processed out | finite, non-silent at 44.1 / 96 / 192 kHz | ✓ |
| No clicks/pops | section 2 and 5 gates | ✓ |
| No runaway | every type, DECAY 2.0×, SIZE 100 | ✓ |
| No allocation | 600 blocks, oversized included | ✓ |
| CPU usage | reported, no ceiling | 0.33–1.19 % of a core at 48 kHz |

### UI Verification

| Test | Expected | Result |
|------|----------|--------|
| UI source | untouched by this milestone, `i18n.js` aside | ✓ (`git diff --stat 02bac73f HEAD`) |
| Tooltips fit at 500 × 350 in en / fr / zh-Hans | inside the frame | ✓ (French `tip.type` is 9.1 px from the bottom edge) |
| UI in a host | loads, controls respond | not exercised here (pluginval ran with GUI tests skipped) — part of the listening pass |

---

## Regression Check

**Baseline version:** 1.14.0
**Backup location:** `backups/O-SimpleReverb/v1.14.0/`

### Existing Features Still Work

| Feature | Status |
|---------|--------|
| Click-free CHARACTER, LOW CUT, WET / DRY | ✓ |
| VU peak hold | ✓ (2 gates) |
| Mono / stereo bus constraint | ✓ (code unchanged; mono run gated) |
| Preset manager, factory sentinel, stale-file sweep | ✓ |
| i18n en / fr / zh-Hans | ✓ (gates pass; three bodies await reading) |

### Preset Compatibility

**Old presets load:** ✓
**Values preserved:** ✓ — and they sound different, which is the MAJOR bump.

---

## Issues Found

### Critical Issues (blocking)

None found by measurement or by reading the code.

### Non-Critical Issues (noted)

1. **The early taps are loud against the dry signal on Booth.**
   - The impulse response's peak at WET 100 is −0.3 dBFS on Booth, 3.75 ms after the input (v1.14.0: −13.4 dBFS
     at 28 ms). At an insert setting of WET 25 / DRY 100 that is a copy about 12 dB under the dry signal 3.75 ms
     late, which is a comb of roughly ±2 dB with a 267 Hz spacing. Arithmetic from the baseline table, not rendered.
   - Severity: Low. Impact: possible colouration of Booth inserts on close, dry sources.
   - Recommendation: listen for it first (Booth - Vocal Booth, Booth - Voiceover on speech). The knob is
     `typePresets[0].earlyLevel` (0.6).

2. **The level law scales the early taps with the tank.** It is an input gain, so the taps follow it: the
   IR peak moves 4.5 dB across SIZE on Booth (−2.5 → +2.0 dBFS) and 2.2 dB across DECAY, while steady
   loudness holds within 0.6 dB. On sustained material that is what was wanted; on a single hit, SIZE up
   reads as "taps up".
   - Severity: Low. Recommendation: accept unless the listening pass objects.

3. **Plate echo density is under the plan's target from SIZE 50 up** (0.77 / 0.70 against 0.80). Known since
   stage 2; reported, not gated. Listening item. Knobs: `typePresets[4].sizeHi`, or more input diffusion.

4. **Spring's chirp differs by sample rate** (3 kHz lag 6.2 ms at 44.1 kHz, 9.8 ms at 48 kHz): the stretch
   factor is a whole number of samples. A session moved between rates gets a slightly different drip. Accept.

5. **A mono bus is about 6 dB wetter than v1.14.0's at the same WET.** Deliberate (mono now matches stereo)
   and in the CHANGELOG. A v1.x mono session will need WET turned down.

6. **`tip.character` and `tip.lowCutOn` still say the filter is bypassed** when neutral / off; both have run
   continuously since v1.12.0. Out of this milestone's scope. Follow-up: a `/improve` text fix in three languages.

7. **"render-check 32/32" in the v1.14.0 CHANGELOG entry and NOTES line** is 26 (stage 0 found it). History
   entries; left as written.

8. **`.claude/scripts/inject-context.py` fails on this plugin** (`'stages'`): `.planning/STATUS.md` is the
   January Stage-1 file with no stage table. Workflow tooling, not the plugin. Clearing `activeMilestone`
   after verify leaves that file stale in every other respect.

9. **A TYPE change clears the incoming slot's delay lines on the audio thread** (about 1 MB at 48 kHz, 4 MB
   at 192 kHz for the FDN). No allocation and no gate fails on it; it is a one-off cost at the switch.
   Noted from reading `startSlot` → `clearSlot`, not measured.

---

## Human gates

### 1. Listening pass (criterion 13)

On the installed 2.0.0 build, or the Standalone rebuilt above. A change to any of these constants needs
render-check re-run (and `--levels` if it is a level), not a new stage.

| Listen for | Where | Constant |
|---|---|---|
| Booth colouration against the dry signal (note 1) | Booth inserts on speech / snare | `typePresets[0].earlyLevel` |
| Early-reflection levels, spans, pre-delays, type EQs — carried over from v1.14.0 | each room type on a transient | `typePresets` |
| How much level DECAY should keep (half now: +2 dB from 0.5× to 2.0×) | any type, sweep DECAY on a pad | `kDecayLevelDbPerOctave` |
| Plate graininess at SIZE 50 and up | Plate - Long Plate, Plate - Shimmer Plate on a snare | `typePresets[4].sizeHi` |
| Shimmer amount and brightness | Plate presets, long tails | `PlateEngine::kShimmerPerLoopSecond`, `kDampingHz` |
| Spring drip: length, darkness, wobble | Spring - Amp Spring, Surf Guitar on a muted guitar | `SpringEngine::kSpring`, `kAllpassCoefficient`, `kWobbleMs` |
| Metallic ringing in long Hall / Ambient tails (CONTEXT requirement 1) | Hall - Cathedral, Ambient - Infinite Drone | FDN modulation, `typePresets[2]`, `[5]` |
| The 48 presets, chosen by decay time and level | the whole bank | `bank[]` |
| SIZE sweep on a ringing tail: a bend, no click | any | — |
| UI in the host: opens, knobs track, presets recall | Logic / Ableton | — |

If a DAW shows v1.14.0 behaviour, quit it fully and reopen (Logic caches per version; this is a version bump).

### 2. Read `I18N-REVIEW.md` (criterion 14) — done 2026-10-01

Read and approved; `tip.type`, `tip.decay`, `tip.size` are `reviewed: true` (fr) and `'bt'` (zh-Hans).

### 3. Commit shape — decided 2026-10-01

This phase goes on top of the five local `wip(O-SimpleReverb)` commits (`65b6672e`..`bf748c18`) as a sixth,
path-scoped, unpushed. Whether to squash the six waits for the listening pass, since a constant may still move.

---

## Verification Checklist

- [x] All measurable requirements from CONTEXT.md verified
- [x] Build succeeds without errors or warnings
- [x] Pluginval passes Level 10; `auval -v` passes
- [x] Audio processes correctly (two independent measurers)
- [x] No regressions in existing features
- [x] Preset compatibility confirmed (values load; sound changes by design)
- [x] Baseline compared row by row
- [ ] Listening pass in a DAW — Taylor
- [x] French and zh-Hans back-translation read; flags flipped — Taylor, 2026-10-01
- [ ] UI exercised in a host — with the listening pass

---

## Final Status

### PASSED WITH NOTES ⚠ — pending the listening pass

Every measurable criterion (1–12) passes, reproduced from a clean build and cross-checked on the installed
binary with independent code. Criterion 14 is closed. Criterion 13, the listening pass, needs Taylor.

**When it is done and nothing needs changing:**
1. Set `phases.verify.status: complete` and remove `activeMilestone` from `.planning/STATUS.md`.
2. Squash the six `wip` commits or leave them.
3. **No git tag here.** The tag is `/publish`'s, as `O-SimpleReverb-v2.0.0`.

**If the listening pass changes a constant:** edit it, re-run `render-check` (and `--mutants`, `--levels`),
rebuild with `./scripts/build-and-install.sh O-SimpleReverb`, and re-run this phase.

---

*Generated by improve-milestone verify phase*
