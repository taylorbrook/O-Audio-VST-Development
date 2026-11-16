---
title: "O-ParametricEQ Stage 3 (GUI) — Phase 3.3 Summary (analyzer, meter, fills, listen)"
date: 2026-11-16
phase: "3.3"
plan: PLAN-3.3.md
version: 0.9.0
status: executed
---

# O-ParametricEQ — Phase 3.3 Summary

The display now shows what is playing:

- **Analyzer:** Off / Pre / Post / Pre+Post traces, drawn at the span centres, with nothing above Nyquist.
- **Meter:** a page-drawn two-bar output meter with dB-per-second ballistics. It stays steady at 2048-sample host buffers.
- **Fills:** each band is washed in its pigment, and bypassed bands are not.
- **Listen:** follows a node drag, returns to the latch on release, and ends when the band is disabled from any source.
- **NaN fix:** a NaN inside listen's input ramp is caught at its own block.

Version 0.9.0 is built, installed and validated. With listen off, the audio is bit-identical to 0.5.0. Executing the bench found three page bugs and one analyzer bug, and all four are fixed (see "Deviations").

## Results at a glance

| Check | Result |
|---|---|
| T1 baseline (HEAD `452a0cd3`) | designer 150 / 0 / 18; render 166 / 0; XR-lid 16 / 16 SHA-equal to 0.5.0; probe `probe33 n` on HEAD **N1e 12 / 16**, hash `6274ff9b5b0e488a` |
| Designer harness | **150 PASS / 0 FAIL / 18 INFO** |
| Render harness, default / `--long` | **171 PASS / 0 FAIL** both (166 + N1e, N1e-u, AO1, AO2, M1) |
| Audio unchanged | XR-lid sessions from 0.9.0 are SHA-256-equal to the 0.5.0 hashes 16 / 16 (`execute-3.3/harness/sessions.sha`) |
| Probe `probe33 n` on the edited tree | **N1e 16 / 16**, hash **`6274ff9b5b0e488a`** (= HEAD's; no-NaN renders bit-identical) |
| `git diff HEAD -- Source/dsp Source/analysis Source/PluginProcessor.*` | `Listen.h` (P1), step 8 (P1), the `vu-meter` include (P7), and `SpectrumAnalyzer.{h,cpp}` (deviation 1, message thread only) |
| G9 (b) `tests/g9b/run.sh` | PASS (controls fire; 5b 59.5 dB as in 3.2) |
| Meter ballistics `tests/meter-ballistics/run.sh` (new) | PASS; per-frame-decay control fires (−16 vs −26 dB at 0.5 s) |
| UI gates (repo root, final page) | `check-i18n` 0, `i18n-fr-lint` 0, `i18n-zh-lint` 0, `check-ui-labels --plugin O-ParametricEQ` 0, `boot-all-uis --strict-tips` 0: 49 / 49 clean, **0 DEAD** (2 late bindings are O-Bells, informational) |
| Native functions | 19 / 19; page + `preset-manager.js` vs `PluginEditor.cpp` grep diff empty |
| `vu-meter` / `VUMeter` / `vumeter` in `CMakeLists.txt`, `Source/` | none; `Source/ui/public/modules/vu-meter.js` deleted, reconfigured, not re-copied |
| `OPEQ_EDITOR_BENCH` in the plugin's compile commands | 0 |
| Page greps | one `setListenNative(` call site (in `syncListen`); no `i / (n - 1)`; no stretched gradient in the meter; `meterStep` defined once |
| Build warnings in the plugin's TUs | none |
| Install | `build-and-install.sh` rc 0, `O-ParametricEQ-dev.vst3` / `.component` at **0.9.0**, no alternate-variant orphan |
| pluginval strictness 10, `--timeout-ms 600000` | VST3 ×2 SUCCESS, AU ×1 SUCCESS (final tree) |
| auval `aufx OuPQ OuDv` | AU VALIDATION SUCCEEDED |
| **Editor bench** | **5 Release processes + 1 Debug `--live`: ALL-PASS** (`verdict33.py`); Debug 10 cycles, 0 assertions, 0 leak lines |
| **Mutant control** | **13 / 13 caught**; bench, harness and node controls all pass |

## Render-harness rows (T3)

| Row | Result |
|---|---|
| **N1e** | 16 / 16. A NaN at t1 + 700 inside listen's 30 ms ramp (engage / change), band 3 on / off, mono / stereo, blocks 64 / 512: no non-finite sample after the NaN's own block. Before P1: 12 / 16. |
| **N1e-u** | 4 / 4. The voice flag is taken on its own line. **Changed from the plan (deviation 5):** a Left-placed listen voice, with a NaN on L (voice + lane non-finite) vs on R (lane only); the two renders are bit-equal from the next block. Engage only, band 3 on / off × blocks 64 / 512. |
| **AO1** | Mode 0 drains 0 / 0; Post drains pre 0, post 48 000. |
| **AO2** (new, deviation 1) | Post with noise, 3 s Off, then Post with silence: every Post point after the switch is ≥ 55 dB below the noise's top (no frame of pre-Off audio). |
| **M1** | 6 / 6 with the shipping `PeakHold.h` (44.1 k / 1024 H 3, 2048 H 4, 4096 H 7; 48 k / 1024, 4096; 192 k / 2048): worst −6.0 dB on both channels. The H = 1 control shows empty reads. |

`--rows33` (new harness flag) runs only these five rows; the mutant runner uses it.

## Editor bench (`execute-3.3/editor-bench/`)

Five fresh Release processes and one Debug `--live` process (cycles ≥ 1 with noise, Pre+Post and listen on at close). The page is the real `index.html`. A bench thread feeds `processBlock` in real time for the audio rows, at user-interactive QoS, spin-waiting between blocks (deviation 6). Every process waited for an unlocked screen and finished unlocked.

### 3.1 and 3.2 rows

All re-run and all pass in all 6 processes:

- **E-open:** cold open 251 – 268 ms (Release).
- **E-idle:** 0 events, 0 posts.
- **E-preset and E-eq-init:** all pass.
- **E-lag, E-drag, E-wheel, E-ends, E-mono and the rest:** all pass.
- **E-res:** now expects 10 resources (P7 removed `/modules/vu-meter.js`).
- **E-listen (3.1):** its "on at close → −1" pair is now carried by E-cycle33.

### 3.3 rows (figures: the five Release processes)

| Row | Result |
|---|---|
| **E-an-modes** (UI-03) | 5 / 5 checks ×5. Modes 0 / 1 / 2 / 3 emit exactly {} / {Pre} / {Post} / {Pre, Post}. In mode 0 the processor writes 0 tap samples (write-index delta). A change that adds Post emits it 7.5 – 12.8 ms later (gate 25 ms). |
| **E-an-stale** | First Post frame after Post → Off (3.5 s silence) → Post: top −1200 tenths, every frame in 0.6 s. The noise reached −332 tenths (stimulus guard > −900). |
| **E-an-nyq** | 256 points: 7 null from span 249 (the first span ≥ 24 kHz), 0 mismatched. 0 analyzer ink pixels right of fx(24 kHz) + 2 in all 5 processes. |
| **E-an-peak** | Sine at the centre of span 240 (18.48 kHz), −6 dBFS, 1600 × 1000: highest trace pixel −0.37 px from fx(f). **Changed from the plan's 1 kHz (deviation 5).** 1 kHz is recorded: +1.0 px, inside its span's ±1.4 px. |
| **E-meter** (UI-06) | 48 k / 512: peak line and level fill at my(−6) (57.5 vs 57.73 px). 44.1 k / 2048 blocks: **level-fill top range 0 px over 2 s** (57.5 – 57.5), peak line range 0. Mono: two equal bars. |
| **E-meter-rest** | 0 meter frames in 2 s at rest; > 5 frames per 300 ms while playing. |
| **E-idle33** (E-idle extended) | After audio stops and the displays decay (8 s): 10 s with **0** eqSections, 0 analyzer, 0 outputLevel, 0 page events, 0 page posts. |
| **E-fills** (UI-07) | Bands 1 – 3 (150 Hz +9, 1.5 kHz −9, 6.5 kHz +9 dB): the wash pixel's hue is within 15° of each pigment. Band 2 bypassed → alpha < 6. |
| **E-listen33** (FUNC-08) | 18 / 18 ×5, sub-cases (a) – (j) of the plan. Drag → band during, −1 after; click without movement → 0 `setListenBand` writes; latch 3 + drag 5 → 4 during, 2 after; host disable → −1; preset disabling → −1; preset keeping → 2; bypass → 2; Delete → −1; strip twice → 2 then −1; cancel / lost capture → −1. |
| **E-fps** (criterion 5, page) | Pre+Post with noise, 192-section session, 1600 × 1000, 5 s scripted node drag at 60 Hz: **rAF median 17 ms, p99 20 – 22 ms** (gate 17.5 / 25). `draw()` 8 / 9 ms. |
| **E-rate** (V-3.2 Issue 1) | Median interval between eqSections emits during the drag: 16.6 – 16.7 ms (gate 20). |
| **E-audio** (criterion 5, audio) | `processBlock`, 24 × 96 dB/oct, 48 k / 512, 10 s each. Closed 315 – 320 µs; open, Off, automated 329 – 336 µs; open, Pre+Post, page drag 342 – 348 µs (**+8 – 10 %**, gate 10 %); p99 401 – 415 vs 350 – 358 µs (gate 25 %). rel-5's first attempt was +10.1 % on the median and passed the allowed re-run (+7.4 %). The first full run's rel-4 did the same on p99. **The margin is thin.** |
| **E-create-seeded** (V-3.2 Issue 2) | Band 1 seeded High Shelf / Side / bypassed / disabled → plot click → Bell / Stereo / not bypassed. |
| **E-gbypass** (V-3.2 Issue 4, V7) | Global bypass: every sum ink run on gy(0) ± 1 px. Before bypass, ≥ 1 column was off 0 dB. |
| **E-sumview** (V-3.2 Issue 4, V8) | Mid + Side bands, Side selected: the ink matches the SS composition at every valid probe column (≥ 3), and the view is SS. SS and MM differ by > 6 px at ≥ 1 column. |
| **E-cycle33 / E-cycle-dbg** | Every close in all 6 processes: listenBand −1, no analyzer consumer. Debug `--live`, cycles 1 – 9: listen 4, consumer on, mode 3, audio running before each close; then −1 / off. 0 assertions, 0 leak lines. |

### Mutant control (`execute-3.3/mutants/run-mutants33.py`, `mutants33-results.json`)

Each mutant is one textual edit to a scratch copy of `Source/`. Bench mutants rebuild the bench (`-DPEQ_SRC`). Harness mutants build the render harness from the repo build's commands with the scratch `Source/` (`build-harness.sh`) and run `--rows33`. The node mutant runs `ballistics.mjs` on the scratch page.

| Mutant | Must fail | Failed |
|---|---|---|
| Listen flag never set | N1e | N1e ✓ |
| Flag taken inside `! finite \|\|` | N1e-u | N1e-u ✓ |
| `analyzer->process` skipped while mode 0 | E-an-stale | E-an-stale ✓ |
| Emit every tick (no change test) | E-idle | E-idle, E-eq-idle, E-idle33 ✓ |
| Page maps `null` to 0 | E-an-nyq | E-an-nyq, E-an-peak ✓ |
| Span centre reverted to `i / (n − 1)` | E-an-peak | E-an-peak ✓ |
| `PeakHold` depth fixed at 1 | E-meter (2048) and M1 | E-meter ✓, M1 ✓ (after the gate fix below) |
| Listen set at pointerdown | E-listen (b) | E-listen33 (b) ✓ |
| No return to the latch on release | E-listen (c) | E-listen33 (c) ✓ |
| Disable hook only in `deleteBand` | E-listen (d), (e) | E-listen33 (d), (e) ✓ (after the gate fix below) |
| Timer at 30 Hz | E-rate | E-rate ✓ |
| Meter decay per frame | meter-ballistics | meter-ballistics ✓ |
| (added) Analyzer Off rule removed | E-an-stale, AO2 | E-an-stale ✓, AO2 ✓ |

The first mutant run left **two survivors**. Both were gate gaps, fixed in the bench and then re-run (13 / 13):

- **`PeakHold` depth 1 passed E-meter.** The probe read the first opaque pixel, which is the peak line, and the peak line holds −6 dB for 1 s over the level's sawtooth. The probe now also reads the level fill's top (zone colours), and the 2048 row gates that.
- **The disable-hook mutant failed (d) but passed (e).** (d) left the latch on, so (e)'s "re-latch" click toggled it *off*, and the preset check passed vacuously. `latch()` now presses strip Listen only when it is not already pressed (`latchOn`).

The full bench was re-run on the fixed gates. The figures above are from that run; the earlier run is kept in `editor-bench/results-run1/`.

## Deviations (every edit beyond P1 – P7)

1. **`SpectrumAnalyzer` Off rule** (`Source/analysis/SpectrumAnalyzer.{h,cpp}`, message thread only, no audio change).
   - **Problem:** after Off → On, the analyzer's history still held pre-Off samples, so the first frames redrew audio from before the switch. The plan's E-an-stale would fail on the planned code.
   - **Rule:** while `analyzerMode` has a tap's bit off, that tap's valid count is cleared every call. Switched back on, the tap computes no frame until N new samples arrive.
   - **What was rejected:** a first, time-based version (no samples for 0.25 s) also changed host-stop re-entry. It broke XR-anrel and XR-gen (Stage 2 rows), so it was replaced.
   - **Gates:** AO2 (new) and the "Off rule removed" mutant.
2. **Listen hook settle (page).**
   - **Problem:** a preset load resets every parameter to its default before applying the preset, so a band the preset keeps enabled passes through `enabled = false`, and listen ended (E-listen (f) failed).
   - **Fix:** the enabled hook re-checks after `LISTEN_DISABLE_SETTLE_MS` = 80 ms.
3. **One `draw()` per animation frame (page).**
   - **Problem:** with listen on during a drag, `listenPulse()`'s loop and `requestRender()` each drew, giving two 7 – 8 ms draws per frame and 22 ms frames (E-fps failed).
   - **Fix:** both now go through `drawOncePerFrame(ts)`.
4. **Strip Listen button shows the latch.** `aria-pressed` follows `ui.latch`, not the effective band, so a drag does not flip the button it never pressed.
5. **Two plan rows changed (ill-posed as written).**
   - **N1e-u:** a render with the NaN zeroed never clears state, so it can't be bit-equal even to correct code. It now compares NaN-on-L with NaN-on-R. "Change" cases are dropped: with a Left listen settled, a NaN on R never reaches a lane.
   - **E-an-peak:** a point's peak may sit anywhere in its span (±1.4 px at 1 kHz on a 1406 px plot), so 1 kHz cannot meet 1 px. It now uses the centre frequency of span 240, where the reverted mapping is 2.4 px off.
6. **E-audio method.**
   - **(a) Same motion in every run.** Runs 1 and 2 move band 12 by host automation along the drag's path at 60 Hz. A moving band redesigns every block, so a static run 1 would charge the engine's own motion cost to the editor.
   - **(b) The feeder spin-waits between blocks.** A core that idles between blocks reads 1.4 – 2 ms per block, and how often it idles depends on the message thread's load. That made "editor open" read *faster* than closed. A Mach time-constraint policy did not help; spinning with it got the thread demoted.
   - **Evidence:** `--diag-audio` runs in `editor-bench/results-diag/`. Adopted method, run 4: static 254 – 257 µs, automated 324 – 333 µs, open vs closed within 2.5 %.
7. **Bench additions:**
   - `--live` (Debug) and `--diag-audio` flags;
   - `verdict33.py`;
   - the render harness's `--rows33` flag.
8. **T1 bench baseline on the unmodified editor not re-run.** The source edits had started before the bench copy was built. VERIFICATION-3.2's committed bench JSON (6 / 6 ALL-PASS at `47d767d3`; no editor change since) stands as the baseline.
9. **T10.4 Standalone screenshot with live input not taken.** It would raise a microphone permission prompt with nobody at the machine. The bench covers the real editor, page and processor under real-time audio. Left for the hands-on.
10. **E-an-nyq, one stray result.** In the first full run, rel-5 read 1416 changed pixels right of fx(24 kHz), while its nulls were exact. It did not recur in 4 other processes of that run, 11 mutant runs, or 6 processes of the final run. The probe now reports a bounding box for any recurrence. Recorded, not explained.

## Carry-forward for verify 3.3 (hands-on, Taylor; Logic AU and one VST3 host)

- Analyzer modes on program material: Off / Pre / Post / Pre+Post, and nothing past Nyquist.
- Meter against Logic's own channel meter on a −6 dBFS sine. A 2048 buffer shows a steady meter.
- Drag listen and latch by ear: engage / return clean, no click.
- Disabling the listened band by automation ends it. Loading a preset that keeps it enabled does not.
- Standalone with live input: the analyzer and meter move, and the console is clean (deviation 9).
- E-audio's margin is thin (+8 – 10 % against 10 %). Verify should re-run it on a quiet machine before reading a fail as a regression.

## Files

- **Source:**
  - `Source/dsp/Listen.h`, `Source/PluginProcessor.{h,cpp}` (P1, P7);
  - `Source/analysis/SpectrumAnalyzer.{h,cpp}` (deviation 1);
  - `Source/editor/PeakHold.h` (new: `PeakHold`, `gainToTenthsDb`, `encodeAnalyzerPoints`);
  - `Source/PluginEditor.{h,cpp}`;
  - `Source/ui/public/index.html`.
- **Build and docs:** `CMakeLists.txt` (0.9.0, `vu-meter` removed), `CHANGELOG.md` `## [0.9.0]`.
- **Tests:** `tests/render-harness/main.cpp` (five rows, `--rows33`), `tests/meter-ballistics/` (new).
- **Execute artefacts in `execute-3.3/`:**
  - `editor-bench/` (results, results-run1, results-diag);
  - `mutants/`;
  - `harness/` (tallies, sessions SHA, probe outputs before and after);
  - `gates/` and `validators/`.
- **Elsewhere:** `modules/registry.yaml`, where `vu-meter` now has `used_by: []`.
