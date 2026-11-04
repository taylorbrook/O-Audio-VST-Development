# Summary: Audio-Rate Pulsar Grain Mode

**Plugin:** O-simpleGrain · **Milestone:** audio-rate-pulsar-grain-mode · 1.9.0 → 2.0.0

## Wave A — DSP (A0–A7): **A7 PASS**

Cloud is bit-exact vs 1.9.0, a 1-ULP-perturbed build fails that null, and Pulsar is bit-identical at blocks 32/441/7 vs 512. Nothing is committed, tagged or installed. `modules/` is untouched.

### Decisions taken after A7 (user, 2026-10-28) and applied
1. **FM Ratio default → 0.5.**
   - Whole-number ratios are inert by construction: onsets stay 1/f_p apart. The Wave B tooltip must say so.
   - The FM DSP, range and skew are unchanged.
   - Gate: ratio 0.5 / 1.5 → f_p/2 line at −20.8 / −30.9 dB. Ratio 1 / 2 is inert vs no-FM at −106.9 / −105.4 dB (bound ≤ −100). The PLAN check was replaced and ticked.
2. **Sine trim: `GrainVoice::kSineContentTrim = 0.0625f` (2⁻⁴, −24.1 dB).**
   - Applies to Sine content only, inside `renderPulsaret`.
   - Sample content and the Formant 32×-vs-1× drop are unchanged.
   - A Sine → Sample switch mid-note is now a +2.1 dB level step (RMS 0.01230 → 0.01566; fire source, Formant 1.25×, MIDI 57). The switch still passes its click check: curvature ratio 0.987.
   - The P2 Sine-without-source audibility floor was lowered from RMS 0.01 to 1e-3, because the trim scales the signal by exactly 1/16 (RMS 0.0070). Its NaN-source control still fails as it should.
3. **Re-run after both changes, all PASS:**
   - Cloud null vs 1.9.0: 19 renders, bit-exact.
   - Pulsar block-size null: 45 PASS.
   - BH-band level spread: 0.094 dB. Control: 3.34 dB.
   - A3 alias: −86.5 / −87.3 / −85.4 dB. Control: +0.4 dB.
   - P5: 0 allocations.
   - `wave_a.py`: WAVE-A PASS.
   - CPU: Pulsar 1.0–1.1 %, Cloud defaults 0.78 %.

The level table below is the post-trim measurement. It supersedes the Sine column in the earlier report.

### Deviations from PLAN
- **Sample reference pitch.** It is MIDI 60 (261.63 Hz), not 130.81 Hz. MIDI 60 is the plugin's root key, so Formant 1× reads at the Cloud grain rate on that key.
- **Content-aware level gains.** Sine uses a sine-weighted gain (BH 1.469). With the plain power gain (1.206), Sine sits 1.7 dB low across the 0.75–1.25 ms band. Sample keeps 1.206. Rect and Tukey use closed-form gains instead of a per-pulse `meanSquare()`.
- **Exact per-sample FM integration with crossing refinement.** The naive step jittered onsets: the FM alias sweep measured −38 dB with it and −85 dB now.
- **f_p capped at the ceiling after Glisson and Pitch Spray.** At Glisson +24, top keys stop rising above ~1.1 kHz at 44.1 kHz. The Wave B tooltips must say this.
- **Extras written with the clock in A3.** Mask, Skip, FM, Glisson and the viz countdown went in with the A3 clock code. Their neutral-setting check runs against a build with that code removed, which is stronger than the plan's check against the A4 build.
- **Checks measured differently from the plan:**
  - The BH band is measured at 13 fixed lengths. Pedalboard cannot automate within a render.
  - The Pitch Spray alias check measures the 0.47–0.5·fs band, because the output is non-periodic.
  - The content-switch, Sine-without-source, viz, allocation and CPU checks live in the C++ harness (`pulsar_checks.cpp`). Sine-without-source uses a NaN-filled source instead of a read counter.

### Pre-existing (not regressions)
- **Voice steal allocates.** A 9th-note steal allocates in `juce::Synthesiser::findVoiceToSteal`, in both modes. The 0-alloc check uses a no-steal scenario. Fixing it would change Cloud's voice choice, so it is out of scope.
- **Two harness checks already failed.** `window-rect-clicks` and `tukey-between-rect-hann` fail with identical numbers on the 1.9.0 harness.
- **Preset-library gate environment.** It needs `CFFIXED_USER_HOME` and `HOME` set to a throwaway folder.

### A0: BandlimitedReader contract (grain-engine 1.1.0)
- **Level latch.** `latchLevel(rate, max)` returns `clamp(floor(log2 rate), 0, max)`, computed exactly with `ilogb`. Any rate below 2, or NaN, gives level 0.
- **Sinc set selection.** `SincBank::setFor(r′)`:
  - r′ ≤ 1 → S0.
  - (1, 2] → S1–S6, with edges at 2^(k/6). These are the −60 dB sets.
  - (2, 4] → S7, best effort, about −20 dB at r′ = 3.99.

  An over-filtered residual in [1, 2) therefore lands in S0–S6. S7 is reached only when the level is capped at 6.
- **Read.** `MipSnapshot::read(level, ch, pos, set)` takes `pos` in level-0 source samples, as a double. Out-of-range positions are clamped. No allocation, no lock.
- **How Pulsar latches.** The level and sinc set are latched from rTop = r·γ·glideUp, with γ = 0.5 / max(0.05, 0.5 − 4/L). `readPos` advances by the true r, so the reader can only over-filter.
- **Numpy checks.**
  - BH μ₁ = 0.35875, μ₂ = 0.257963.
  - One-cycle BH −60 dB extent: 4.62× (RESEARCH: 4.61×). Hann: 6.63×.
- **Reference build.** 1.9.0 from `git archive` of HEAD a2c6d5fc, with the gitignored module JS copied in. The seed travels in the state, so no seed patch was needed.

### Files changed
- **`PluginProcessor.h`:** 7 parameter IDs, `kFormantCeilRatio = 0.10`, 7 parameter pointers.
- **`PluginProcessor.cpp`:**
  - 7 parameters appended with version hint 2. Their values are pushed to the voices each block.
  - The old-session migration list now covers the 7 IDs, so an old session loaded into an instance left in Pulsar comes back as Cloud.
  - `modeGain = Pulsar ? 1 : normGain`. The Cloud normGain block is untouched.
- **`GrainVoice.h`:**
  - A per-block mode branch. The Cloud scheduler is left verbatim inside `else` and not re-indented, so the diff shows no Cloud edits.
  - `if (g.pulsaret) { renderPulsaret; continue; }` sits ahead of the Cloud expressions. `spawnGrain` resets `g.pulsaret = false`.
  - New code: the pulse clock, exact FM integration with crossing refinement, pulsaret spawn and render, and sample content.
- **`dsp/WindowLuts.h`:** a separate internal BH table, `readBH`, the power and sine-weighted gains, and the BH moments. The 5 user tables and `build()` are unchanged.
- **`dsp/Grain.h`:** plain-data pulsaret fields.
- **New tests:**
  - `tests/pulsar-gate/null_v190.py` and `wave_a.py`.
  - `tests/render-harness/pulsar_checks.cpp`: checks P1–P5. `--pulsar` runs only these.
- **Modified tests:**
  - `tests/blocksize-null/gate.py`: 3 Pulsar configs with a liveness check each, skew-safe parameter setting, and `--cloud-only`.
  - `persist_test.py`: optional bundle-path argument.

### Gates (measured vs threshold, with the control that must fail)

**Cloud null and parameters**
- **Cloud null vs 1.9.0, 19 renders.** 3 blocksize-null configs at blocks 512 and 441, 4 v180 configs, 7 migrated states and 2 new-written Cloud states. Max diff 0 at A1, A2, A3, A4, A5 and A7.
  - Control: the +1 ULP build (on Cloud `phaseInc`) fails 19 checks (2.98e-8).
- **Parameters:** 23 → 30, with the first 22 identical.
- **New warnings:** 0.

**A2: BH table**
- Mean 0.358750, mean-square 0.257963 (limit ±1e-4).
- The 5 user tables are byte-identical (memcmp + FNV hashes).

**A3: clock, Sine pulsarets, ceiling, BH band**
- **f_p, MIDI 57:** −0.0001 cent at 44.1, 48 and 96 kHz (limit 0.1).
  - Control: +1 cent of Grain Pitch reads +0.9999 cent.
- **Formant 2/3/4/6/8×:** spectral centroid 518 < 776 < 1058 < 1914 < 2552 Hz, f_p unchanged.
- **Alias, Formant 32× at MIDI 108/96/93:** −86.5 / −87.3 / −85.4 dB (limit −60).
  - Control: ceiling at 0.45·fs gives +0.4 dB.
- **BH band level:** spread 0.094 dB (limit 0.5).
  - Control: with gains forced to 1, spread is 3.34 dB.

**A4: Sample content**
- **Alias, worst case per source:** fire −76.4, voice −85.1, piano −83.8 dB.
  - Control: ceiling at 0.45·fs gives −5.3 dB.
- **Sine over a NaN-filled source:** finite, RMS 0.112.
  - Control: Sample mode over the same source goes non-finite.
- **Content switch mid-pulsaret:** output matches the Sine render up to the next onset and the Sample render after it; curvature ratio 0.987 (limit ≤ 1).
  - Control: content read live instead of latched gives a mismatch, ratio 69.6.

**A5: Mask, Skip, FM, Glisson**
- **Mask 3:** line at 73.3333 Hz, −24.2 dB. With Mask Off: −147 dB.
- **Skip 25 / 50 / 75 %:** counts within 4σ of expected; every surviving pulse falls on a Skip-0 onset.
- **FM ratio 0.5 and 1.5:** f_p/2 line at −20.8 and −30.9 dB, at 110.0000 Hz.
- **FM ratio 1 and 2:** inert (open decision 1).
- **FM 100 % mean rate:** 221 vs 220 pulses in 4 s.
- **Glisson:** half-cycle ends at 0.4302 L for +12 st (theory 0.4293) and 0.6121 L for −12 st (theory 0.6139).
- **Alias sweep with FM and Glisson:** Sine −85.4 dB, Sample −88.6 dB.
- **Pitch Spray band:** Sine −91.3 dB, Sample −107.5 dB.
  - Control: ceiling at 0.45·fs gives −8.7 dB.
- **Neutral settings vs the code-absent build:** bit-exact, Sine and Sample. Each feature moved off neutral differs from both builds.

**A6: viz**
- At most 16 events per block (limit 16; buffer holds 256).
- Viz frame attached vs detached: bit-identical audio.

**A7: hard gate**
- **Pulsar block-size null:** 3 configs × 2 seeds × 3 block sizes, all bit-exact. Each config differs from its own state rendered in Cloud.
- **Cloud block-size null:** bit-exact. Reaper memory +1.6 MiB.
- **Allocations:** 0 in processBlock, with all features on, release and re-strike. The malloc hook is confirmed live.
- **Existing gates:**
  - alias-gate: 18 PASS.
  - skip-tilt: 56 PASS, including its 1 ms control.
  - state-persistence: 24 PASS / 0 FAIL.
  - preset-library: 0 FAIL.
  - Full render harness: P1–P5 pass, plus the 2 pre-existing failures.

### CPU (8 voices, MIDI 101–108, 48 kHz, block 512)

| Mode | % of real time |
|---|---|
| Pulsar, Sample ×32, FM + Glisson + spray | 1.10–1.19 % |
| Pulsar, Sample ×32, plain | 0.86–0.94 % |
| Cloud, defaults | 0.77–0.88 % |
| Cloud, 500 ms × 200 g/s | 8.3–8.9 % |

### Steady-state level (Output −6 dB, 48 kHz, RMS of one held note, with the Sine trim applied)

| Note | Cloud (fire) | Pulsar Sine ×4 | Pulsar Sample (fire) ×4 | Sine ×1 | Sine ×32 (vs ×1) |
|---|---|---|---|---|---|
| MIDI 48 | −52.3 dBFS | −51.2 (+1.1 dB) | −49.1 (+3.2) | −45.2 | −60.2 (−15.0) |
| MIDI 60 | −61.1 | −51.3 (+9.9) | −50.6 (+10.5) | −45.2 | −57.8 (−12.6) |
| MIDI 72 | −63.5 | −51.2 (+12.3) | −53.1 (+10.3) | −45.2 | −54.8 (−9.6) |

Before the trim, Sine sat 25–36 dB above Cloud.

### Reproducing
- **Gate scripts:** `tests/pulsar-gate/`, `tests/blocksize-null/gate.py`, and `O-simpleGrain-render-test [--pulsar]`.
- **Control builds** are working-tree copies, each with one patch, built out of tree in the session scratchpad under `ctl/` (scratch only, not kept):
  - C: ceiling 0.45 and gains forced to 1.
  - absent: Mask/Skip/FM/Glisson code removed.
  - ulp: +1 ULP on Cloud `phaseInc`.
  - live: content read live instead of latched.

  The 1.9.0 reference build is in `ref190/`.

---

## Wave B: UI, i18n, lessons (B1–B3)

### Gates (final tree; Cloud null run on the installed build)

| Gate | Result |
|---|---|
| Cloud null vs 1.9.0 (`tests/pulsar-gate/null_v190.py`, installed VST3 vs `ref190`) | 21 PASS. 19 Cloud renders bit-exact. Pulsar-differs liveness checks at −17.6 / −18.6 dBFS. |
| `check-i18n` | ALL PASS |
| fr lint `--strict` / zh lint | 0 findings in this plugin, 0 / 46 repo-wide. zh: 166 entries, 54 at `'mt'`. |
| Glossary negative control | Wrong fr Mask and wrong zh Glisson each FAIL. File restored. |
| `check-ui-labels` (default + 8 states) | ALL PASS |
| nowrap fit, FR / zh | Lesson grid 5×2 at 560 px. Tightest fr button "Bourdon harmonique": 2.98 px spare. Worst fr lesson caption 819.4 / 846 px; zh 660.5. |
| Build | Clean. 10 float-equal warnings, all on Wave A or older lines. |

`check-ui-labels` found two real issues, both fixed:
- The second fr Pulsar lesson caption wrapped and pushed the rack down 13.2 px. Fixed by shortening both fr captions.
- The zh Mode cell shifted 9.8 px. Fixed with a 31 px caption floor.

### Changes
- **`index.html`:** Mode select on the Freeze row. New Pulsar group under Grain: Formant / FM Depth / FM Ratio / Glisson knobs, plus Pulsaret and Mask selects. Lesson buttons 9 and 10 in a 5th column.
- **`app.js`:**
  - 4 sliders and 3 selects bound. Readouts go through `getScaledValue()`.
  - Mode dimming is driven by the Mode param's change event, so it holds for click, automation, lesson, preset and restore. Pulsar dims Size, Density, Scatter, Vel→Density and Tilt. Cloud dims the Pulsar controls; the Pulsar title stays at full ink.
  - The envelope inset ignores Tilt in Pulsar. The grain cloud skips non-finite events.
- **`styles.css`:** dimming reuses the Taper/ADSR 0.38 opacity and locking.
- **`i18n.js`:** 9 tooltips, 12 labels and 2 lesson captions in EN/FR/zh. The five required statements are included: the Mode-ignored list, the 0.10·fs Formant ceiling, inert integer FM ratios, the Glisson top-key ceiling, and the Mask f_p/N subharmonic.
- **`PluginEditor`:** 7 relays declared before the WebView; attachments after.
- **`applyFactoryPreset()`:** "Pulsar Vowel" (Sine, Formant 5×) and "Subharmonic Mask" (Formant 3×, Mask 3). The existing reset-to-defaults loop returns Mode to Cloud for the Cloud lessons. No DSP touched.
- **`tests/i18n-states.json`:** +3 states.
- **Shared files outside the plugin (additive):** 11 pulsar terms each in `scripts/i18n-fr-glossary.js` and `scripts/i18n-zh-glossary.js`.

### Deviations
- **Choice menus stay English** (Cloud/Pulsar, Sine/Sample, Off/2…8). This follows the plugin's existing rule that menus match the host automation lane. The language tooltip lists all five English-only menus.
- **French lesson button label shortened** to "Sous-harmonique". The full "Masque sous-harmonique" stays as the tooltip title.
- **Pulsar Vowel moved** to sit 5th in source order, so the two Pulsar lessons form column 5.
- **Header subtitle:** NBSP before the middot plus balanced wrapping, so EN now wraps to 2 lines like FR. Header height is unchanged.

### Not verified (needs a human pass)
- No Standalone/DAW screenshot: `screencapture -l` refused the window. The shots are headless stub captures in the session scratchpad `shots/`.
- **Not checked by hand:**
  - host automation round-trip
  - dimming after preset load or session reopen
  - real knob angles. The stub uses a neutral 0–1 range for any slider without an override, so FM Ratio 0.5× draws at 12 o'clock. In the real build it should sit at about 11 o'clock (norm 0.36 under skew centre 1).

---

## Wave C: gates, build, validation, docs (C1–C2)

### C1: `tests/pulsar-gate/gate.py` — PULSAR GATE PASS (installed VST3)
Imports `wave_a.py` for its render and spectrum helpers. Every check prints its value and bound, then runs once more against a negative control that must fail the same bound. A missing control counts as a FAIL.

| Check | Measured (bound) | Control → result |
|---|---|---|
| fp | −0.0001 cent at 44.1 / 48 / 96 kHz (≤ 0.1); estimator −0.0000 | Grain Pitch +1 cent → +0.9999 cent, fails |
| formant | centroid 518 < 776 < 1058 < 1914 < 2552 Hz (×1.05 per step); f_p −0.0000 cent | Formant held at 4× → 1058 ×5, fails |
| alias | Sine C8/C7/A6 ×32 −86.5 / −87.3 / −85.4; Sample fire / voice / piano −76.4 / −85.1 / −83.8; FM+Glisson sweep Sine −85.4, Sample −88.6; Pitch Spray band −91.5 / −106.4 dB (≤ −60) | ceiling-0.45 build: +0.4 / −5.3 / −7.1 dB, all fail |
| bh-band | spread 0.094 dB (≤ 0.5) | gains forced to 1 → 3.34 dB, fails |
| bh-sidelobe | 0.3 ms pulsaret, 96 kHz: harmonics ≥ 5 f_form −85.2 dB (≤ −60) | no-BH build (Hann) → −51.9 dB, fails; 33.3 dB improvement |
| frac-onset | f_p 220 Hz, formant 5 kHz, 96 kHz: off-comb −92.9 dB (≤ −60) | integer-onset build → −20.8 dB, fails; 72.2 dB reduction |
| mask | Mask 3 line −24.2 dB (> −30), Off −147.2 (< −80), 73.3333 Hz | Mask code absent → −147.2, fails |
| skip | 124 / 86 / 40 of 165 at 25 / 50 / 75 % (within 4σ), all on Skip-0 onsets | Skip code absent → 165 of 165, fails |
| fm-line | ratio 0.5 / 1.5: −20.8 / −30.9 dB at 110.0000 Hz (> −40; no FM −142.8) | FM code absent → −142.8, fails |
| fm-inert | ratio 1 / 2: −106.9 / −105.4 dB vs no-FM (≤ −100) | ratio 1.01 → −0.7 dB, fails (stimulus control) |
| glisson | 0.4303 / 0.6119 L (0.4293 / 0.6139 ± 0.01) | Glisson code absent → 0.5000 / 0.5000, fails |
| sine-no-src | Sine over a silent dropped source = over Fire, bit-identical, −43.3 dBFS; harness P2 PASS | Sample content → differs (−29.1 dBFS), fails |
| mode-switch | harness P6: Cloud→Pulsar −137.6 dB, Pulsar→Cloud −141.4 dB vs the same-motion model (≤ −100), curvature S/R 1.0000 (≤ 1.01), gain step ±6.02 dB | cut-grains build −11.2 / +16.1 dB, curvature 25.7 / 300; snap-gain build −6.7 / −0.4 dB, 25.7 / 150; both fail |

**Mode switch method:** harness-side. Pedalboard cannot move a parameter inside one render. A two-call render parked in silence would switch with no grain in flight and prove nothing. New check P6 in `pulsar_checks.cpp`:
- The switch lands mid-note, at a block boundary inside a pulsaret, with the Cloud overlap at 4, so the gain moves 6 dB.
- The reference is the old-mode tail (a note-off at the switch, ADSR off) plus the new-mode note started at the switch, each on a fresh processor, multiplied by the 20 ms ramp rebuilt with `juce::SmoothedValue`.
- The vs-static curvature ratio is printed as INFO only: 1.38 and 1.18.

`--pulsar [P2,P6]` now selects a subset of the checks.

**Control builds.** All were rebuilt from the final tree in the session scratchpad `ctl/`:
- `C`: ceiling 0.45 and gains forced to 1.
- `absent`: the Mask/Skip/FM/Glisson code removed.
- `A3`: now holds **nobh**, `bhMix = 0`.
- `int`: δ = 0.
- `live`: now holds the **snapgrain** harness, which cuts the other kind's grains at a switch.
- `snapgain`: a harness whose output gain snaps on a mode change.

The patch scripts are `ctlA3.py`, `ctlAbsent.py`, `ctlNobh.py`, `ctlInt.py`, `ctlSnapGrain.py` and `ctlSnapGain.py`. The runner is `c1_gate.sh`.

### C2: build, install, validation
- **Build and install.** `./scripts/build-and-install.sh O-simpleGrain` was a no-op build: ninja had no work to do, because the Wave B tree was already built. It installed `O-simpleGrain-dev.{vst3,component}` with a cache clear and a dual-variant sweep. No new warnings: the harness build shows only the 10 known float-equal warnings and the old `octaveShift` unused warning.
- **pluginval:** strictness 10, `--validate-in-process --skip-gui-tests`, installed VST3: SUCCESS.
- **auval:** `auval -v aumu OsGr OuDv`: AU VALIDATION SUCCEEDED.
- **State persistence:** `tests/state-persistence/persist_test.py` 34 PASS / 0 FAIL. New case 8 sets all 7 Pulsar params off their defaults (Pulsar, Sample, Mask 5, Formant 12, FM 40 %, ratio 1.5, Glisson −7) and checks three things:
  - The state XML carries all 7.
  - The decoded values restore in a fresh instance.
  - The restored instance renders bit-identically. Control: a default instance differs.
- **Final Cloud null** (`null_v190.py`, installed vs `ref190`): GATE PASS, 21 PASS.
- **Final block-size null:** full `gate.py` on the installed VST3: GATE PASS, 45 PASS. Reaper +0.0 MiB.

### Docs
- `CHANGELOG.md`: a `## [2.0.0] — 2026-10-29` draft. The version in CMakeLists.txt is not bumped yet.
- `.planning/parameter-spec.md`: a Pulsar group and a 30-parameter note.
- `.planning/BRIEF.md`: a Pulsar Mode section.
- `.planning/STATUS.md`: a v2.0.0 note. The frontmatter is untouched.
- There is no README or USER_GUIDE for this plugin.
