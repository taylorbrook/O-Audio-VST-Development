---
milestone: reverb-engine-rewrite
domain: mixed
execute_agent: general-purpose
version_bump: major
base_version: 1.14.0
target_version: 2.0.0
created: 2026-09-30
---

# Plan: Reverb engine rewrite

**Plugin:** O-SimpleReverb
**Milestone:** reverb-engine-rewrite
**Created:** 2026-09-30
**Phase:** Plan

---

## Summary

**Improvement:** Replace the single `juce::dsp::Reverb` with three purpose-built engines — a
16-line Hadamard FDN (Booth/Room/Hall/Ambient) with output-mixed stereo early reflections, a
Dattorro plate with in-loop shimmer, and a three-spring dispersive tank — behind the same 8
parameters and UI. DECAY becomes a calibrated multiplier on a per-type base RT60; SIZE moves
delay lengths only and glides.

**Approach:** Header-only per-sample engines in `Source/dsp/`, hosted by two ring-out slots in
`PluginProcessor`. Everything after the reverb (type EQ, CHARACTER, LOW CUT, wet/dry, meter)
stays. RT60 is hit by formula and gated by measurement; the v1.14.0 baseline is captured with
the same measurement code before any engine code changes.

**Version:** 1.14.0 → 2.0.0 (MAJOR — IDs, ranges and state format unchanged; every session
sounds different)

### Decisions closed at plan time (2026-09-30)

| Question (RESEARCH Open Questions) | Answer |
|---|---|
| 1. FDN line count | **16** (not the brief's 8) |
| 2. `tip.type` / `tip.decay` / `tip.size` | **Rewrite** in en, fr, zh-Hans — text only, no layout change |
| 3. TYPE change | **Two slots, ring-out**; a third change while both sound steals the older slot |

### Execute runs in five sessions

Each stage ends on a green render-check and a path-scoped `wip` commit. `/clear` between
stages; `/improve-milestone O-SimpleReverb` resumes at the next stage from `STATUS.yaml`
(`phases.execute.stage`).

| Stage | Tasks | Ends when |
|---|---|---|
| 0 Baseline | 1–2 | `BASELINE.md` exists; `Source/` untouched |
| 1 FDN + ER + host | 3–7 | FDN gates green; Plate/Spring gates print `PEND` |
| 2 Plate | 8 | Plate gates green |
| 3 Spring | 9 | Spring gates green; no `PEND` left on engine gates |
| 4 Voicing + release prep | 10–14 | render-check all green, v1.14.0 fails the new gates, 2.0.0 built and installed |

---

## Task Breakdown

### Task 1: Backup, fixtures, test build

**Outcome:** v1.14.0 is recoverable and its state is captured before anything changes.

- `backups/O-SimpleReverb/v1.14.0/` (latest existing backup is v1.13.1) — verify with
  `scripts/verify-backup.sh`. HALT if it fails.
- `tests/fixtures/state-v1.14.0.bin`: `getStateInformation()` blob from a v1.14.0 processor
  with every parameter set to a non-default value, plus `state-v1.14.0.tsv` listing the eight
  values it holds. Written by a one-shot mode of render-check (`--write-fixture`), run once
  on the unmodified tree.
- Find the build directory render-check has been built in. `build/` has
  `OUARICON_BUILD_TESTS=OFF`; `build-asan/` has it ON but ASan hangs at init on this machine.
  If no non-ASan tests build exists, configure `build-tests/` (Release, tests ON) and use it
  for the whole milestone. Record the directory in `STATUS.yaml`.
- Confirm render-check is 32/32 on the unmodified tree before touching it.

**Verification:**
- [x] `scripts/verify-backup.sh` passes for `backups/O-SimpleReverb/v1.14.0/`
- [x] Fixture blob and its `.tsv` exist; loading the blob into a fresh v1.14.0 processor
      returns the eight values in the `.tsv`
- [x] render-check all-pass on the unmodified tree, in the recorded build directory
      (`cmake-build-tests/`; it prints 26 checks, not the 32 the changelog states)

**Dependencies:** None

**Estimated effort:** Small

---

### Task 2: Measurement library and v1.14.0 baseline

**Outcome:** `BASELINE.md` holds v1.14.0's numbers, produced by the same code that will gate
v2.0.0.

New `tests/render-check/measure.h` (header-only, no engine dependency, works through
`processBlock` only so it runs unchanged against v1.14.0 and v2.0.0):

- impulse response per type (stereo, 48 kHz, WET 100 / DRY 0);
- RT60: 354–1414 Hz band-pass (4th order), Schroeder integration, least-squares fit over
  −5..−35 dB, extrapolated to 60 dB; render length `1.35 × T60 + 0.4 s`. Also the 8 kHz
  octave (reported, for the HF voicing);
- late-tail L/R correlation (mono input, window after the mixing time);
- tail-spectrum correlation between two types (200–2000 Hz);
- first-reflection time and early-tap detection in L and R;
- normalised echo density at 100 ms;
- per-band first-echo arrival time (for the spring chirp);
- CPU: wall time per second of audio at 48 kHz and 96 kHz, block 512, median of 5 — reported
  only, never a pass/fail (a wall-clock figure does not belong in a verdict).

render-check gets a `--baseline` mode that prints the table (6 types × DECAY 0.5/1.0/2.0 at
SIZE 50, and SIZE 0/100 at DECAY 1.0) as Markdown.

**Verification:**
- [x] `BASELINE.md` in the milestone directory: IR summary, mid and HF RT60, L/R correlation,
      pairwise tail-spectrum correlation, first-reflection time, echo density, CPU — per type
- [x] The RT60 measurer reads a synthetic exponentially decaying noise of known T60 within
      ±2 % (unit check on the measurer itself, so the gate is not trusted blind) — on the
      mean over seeds; one realisation scatters up to 2.8 %. A decaying tone reads within 0.01 %
- [x] `git status` shows nothing under `Source/` changed
- [x] The existing checks still pass (26, output identical line for line; 44 with the new ones)

**Dependencies:** Task 1

**Estimated effort:** Medium

---

### Task 3: `Source/dsp/ReverbPrimitives.h`

**Outcome:** The shared building blocks, ported from `prototype/common.h` to float.

- Power-of-two ring delay: integer read, 3rd-order Lagrange fractional read, capacity set in
  `prepare` from the running sample rate. Write-then-read order fixed and documented (the
  `pushSample`-shifts-the-delay trap does not exist here, but the off-by-one must be stated).
- Schroeder allpass, one-pole LP/HP, `rt60ToGain(lengthSamples, fs, t60)`, the mid-band shelf
  solve (`g_lo` from the target at 1 kHz, closed-form quadratic).
- A one-pole slew for the size scale (250 ms time constant, coefficient from the sample rate).

**Verification:**
- [x] Unit gates in render-check: allpass magnitude flat within 0.01 dB; Lagrange read of a
      sine at a fractional delay matches the analytic value; `rt60ToGain` round-trips
      (and a loop at that gain decays in that time; the shelf has its target gain at 1 kHz)
- [x] No `juce::` type in the header beyond what `ModulationFx.h` already uses (none at all)
- [x] Builds without warnings
- Departure: the size slew is two poles of 125 ms in series, not one of 250 ms — SUMMARY.md, stage 1, item 1

**Dependencies:** Task 2

**Estimated effort:** Small

---

### Task 4: `Source/dsp/FdnEngine.h`

**Outcome:** The 16-line FDN, measurable on its own.

Port `prototype/engines.h` `Fdn`. Per-type constants start from the prototype table
(`prototype/proto.cpp:25-28`):

| Type | minMs | maxMs | base T60 | hfRatio | crossover | mod ms | mod Hz |
|---|---|---|---|---|---|---|---|
| Booth | 2.9 | 13.7 | 0.40 s | 0.55 | 6 kHz | 0 | — |
| Room | 8.3 | 37.9 | 1.1 s | 0.50 | 5 kHz | 0.06 | 0.7 |
| Hall | 21.7 | 83.1 | 3.0 s | 0.45 | 4 kHz | 0.15 | 0.5 |
| Ambient | 30.7 | 121.3 | 7.0 s | 0.60 | 3.5 kHz | 0.25 | 0.3 |

- Hadamard by fast Walsh–Hadamard; L/R in through two rows, out through two others.
- Geometric delay ladder with the per-type ±3.5 % irregularity; SIZE scale ×0.5 / ×1.0 / ×2.0
  (geometric, 1.0 at SIZE 50), slewed.
- Per-line two-band absorption `y = g_hi·x + (g_lo − g_hi)·LP1(x)`, gains from the line's
  **current** length; recomputed on DECAY/SIZE change and every 16–32 samples during a glide.
- Odd lines modulated (sine, rate spread 0.6×–1.6×, deterministic start phases — no
  `Random`, nothing seeded from `this`); unmodulated lines settle on integer lengths.
- Four input allpasses per channel (0.75, 0.75, 0.625, 0.625), L/R lengths different, scaled
  per type (Booth ×0.25).
- Capacity: `maxMs × 2.0 × 1.035 + modulation` at the running rate.
- Interface: `prepare(fs) / reset() / setType(cfg) / setSize(scale) / setT60(seconds) /
  process(inL, inR, outL, outR)`.

**Verification:**
- [x] Engine-level RT60 (driving the header directly) within ±10 % of the table at DECAY
      0.5/1.0/2.0 and SIZE 0/50/100, at 44.1, 48 and 96 kHz (worst +5.3 %, Booth at 96 kHz)
- [x] Pairwise tail-spectrum correlation between the four types < 0.3 (worst 0.03)
- [x] Late-tail L/R correlation `|r| < 0.3` (worst 0.116 over 108 points)
- [x] 8 kHz RT60 < mid RT60 for every type
- [x] Stable (finite, decaying) at DECAY 2.0×, SIZE 100, 192 kHz (mid RT60 14.10 s)

**Dependencies:** Task 3

**Estimated effort:** Large

---

### Task 5: `Source/dsp/EarlyReflections.h`

**Outcome:** A stereo output tap line for the FDN types.

6–8 taps per side, different times L and R, alternating polarity, gains ∝ 1/t, times scaled
by SIZE (slewed with the same scale), fed from the pre-delayed input **before** diffusion,
mixed to the slot's **output** at a per-type level. Not used by Plate or Spring.

**Verification:**
- [x] Unit gate: the header's impulse response has the stated taps at the stated times, L ≠ R
- [x] Tap times move with SIZE (and a glide lands on the same taps as a reset)

**Dependencies:** Task 3

**Estimated effort:** Small

---

### Task 6: Slot host in `PluginProcessor`

**Outcome:** The processor runs the new wet path; `juce::dsp::Reverb` is no longer on it for
the FDN types.

- `struct Slot`: one engine of each kind (only the active kind is processed), early
  reflections, pre-delay, input gain (duck × `wetTrim`), type EQ, state (idle / playing /
  ringing-out / stolen), a −100 dB retire detector.
- TYPE change: playing slot's **input** ducks to 0 over `kTypeDuckMs` and it keeps running;
  the other slot is reset and its input rises. Third change while both sound: the older slot
  takes a fast output fade, then is reset and reused. `typeEq[2]` becomes one EQ per slot;
  `eqMix`/`eqFadeBuffer`/`switchChainTo` go.
- `TypePreset` reshaped: engine kind, base T60, size range, FDN config, ER table and level,
  pre-delay, EQ fields (kept), `wetTrimDb` (kept, re-measured in Task 10).
- DECAY → `T60 = base × decay`; SIZE → engine scale. Both read once per chunk.
- `processBlock` processes in chunks of at most the prepared block size; no `setSize` on a
  scratch buffer can allocate.
- Mono bus: feed L = R = x, write `0.7071 × (L + R)`.
- NaN: checked once per chunk on each slot's output; a non-finite value resets that slot's
  engine and silences the chunk, so the next chunk recovers. No latch.
- `getTailLengthSeconds()` → 15.0.
- **Plate and Spring during this stage:** they run the FDN on a provisional config so the
  plugin builds and passes pluginval at every stage. Their engine-specific gates print `PEND`
  (counted separately, not as PASS). Remove the pre-reverb flutter, the 3-allpass Spring
  chain, the one-shot shimmer and the input-summed early reflections now.
- Kept byte-for-byte in behaviour: CHARACTER, LOW CUT, wet/dry smoothing, VU peak hold,
  `isBusesLayoutSupported`, state code, parameter layout, factory-preset plumbing.

**Verification:**
- [x] `grep -n "dsp::Reverb" Source/` returns nothing
- [x] `params.tsv` regenerated by param-dump is byte-identical
- [x] Existing sections 1, 2, 4, 5 (LOW CUT / CHARACTER / TYPE cases) and 9's shape checks pass
- [x] Builds without warnings; VST3 + AU + Standalone link (pluginval strictness 10 and `auval -v` pass)
- Departures: the duck is two poles and the steal fade a smoothstep; pre-delay is not scaled by
  SIZE — SUMMARY.md, stage 1, items 2 and 4

**Dependencies:** Task 4, Task 5

**Estimated effort:** Large

---

### Task 7: Stage-1 gates

**Outcome:** render-check proves the FDN stage through the plugin, and is ready for the two
remaining engines.

Per RESEARCH's gate map:

| Section | Change |
|---|---|
| 3 | all six types at 44.1 / 96 / 192 kHz, plus a host block 4× the prepared size |
| 5 | add SIZE 0↔100 and DECAY 0.5↔2.0 HF-burst cases against the WET reference (not a second-difference metric — the glide's pitch bend raises it by design); add rapid A-B-A-B TYPE switching and a sweep through all 48 presets |
| 6 | keep; add a mono run |
| 7 | remove the three flutter cases; keep the shifter unit test; Plate octave gate → `PEND` |
| 8 | remove (superseded by the RT60 table) |
| new | RT60 table (FDN types now, Plate/Spring `PEND`), ±10 % |
| new | RT60 holds across SIZE 0/100, ±10 % |
| new | SIZE moves the structure (first-reflection or mixing time changes) |
| new | pairwise tail-spectrum correlation < 0.3 across the FDN types |
| new | early taps present in the output IR, different in L and R; still present with DECAY at minimum |
| new | late-tail L/R correlation `|r| < 0.3`, mono input |
| new | ring-out: after a TYPE change the old tail is still audible 200 ms later and decays on its own type's RT60 |
| new | NaN injected into a slot recovers within one chunk (test hook behind `OUARICON_BUILD_TESTS`) |
| new | no allocation in `processBlock`, including the oversized block — `malloc_logger`, volatile flag, audio-thread scope (model: O-Bells `tests/render-harness`) |
| new | `tests/fixtures/state-v1.14.0.bin` loads with all eight values equal |

Each new gate's stimulus must be shown to exceed its threshold on a deliberately broken
build (e.g. SIZE wired to nothing; ER level 0) before it is trusted.

**Verification:**
- [x] render-check: every non-`PEND` check passes; the summary line prints PASS / FAIL / PEND counts
      (84 PASS, 0 FAIL, 6 PEND)
- [x] Each new gate has failed at least once on a broken variant (noted in SUMMARY.md) —
      `render-check --mutants`, 14 of 14; and all of section 12 fails on v1.14.0 out of tree
- [x] `wip(O-SimpleReverb)` commit, path-scoped — **end of stage 1**

**Dependencies:** Task 6

**Estimated effort:** Large

---

### Task 8: `Source/dsp/PlateEngine.h` and wiring (stage 2)

**Outcome:** Plate runs a Dattorro tank with shimmer inside the loop.

- Dattorro constants as confirmed in RESEARCH §4, all lengths × `fs / 29761`; modulated
  allpasses read with Lagrange, not linear.
- SIZE scale ×0.40 / ×0.57 / ×0.80 of Dattorro's lengths, slewed.
- `decay = 10^(−3 × loopSeconds × 1.055 / (4 × T60))`, divided by `sqrt(1 − 0.9 s)`, clamped
  ≤ 0.97; `decay diffusion 2 = decay + 0.15`, floor 0.25, ceiling 0.50.
- Shimmer: `feedback = decay × ((1 − s)·x + s·shift(lowpass(x)))` on both cross-feeds, using
  `OctaveUpShifter` unchanged; `s` derived from loop time (constant transfer per second),
  starting value 0.05 at SIZE 50 — one named constant, final value set at the listening pass.
- No early-reflection taps on Plate.
- O-Strata's `ReverbProcessor` is a reference for the output-tap wiring only.

**Verification:**
- [x] RT60 table rows for Plate within ±10 % (DECAY 0.5/1.0/2.0; SIZE 0/100), 44.1/48/96 kHz
      (through `processBlock` +1.3 / +2.8 %; driven directly −1.7..+2.5 % over 27 points)
- [x] Bloom: 600/300 Hz ratio in the tail exceeds the ratio at onset by ≥ 10 dB; Room shows none
      (−35.2 → −13.3 dB; and the tail ratio must sit in −30..−6 dB — SUMMARY.md, stage 2, item 1)
- [x] No runaway: DECAY 2.0×, SIZE 0 and 100, 10 s of noise, then monotone decay
- [x] Echo density at 100 ms reported (target ≥ 0.80; reported, judged at the listening pass)
      — **0.92 / 0.77 / 0.70 at SIZE 0 / 50 / 100: under the target from SIZE 50 up**
- [x] TYPE HF-burst gate still < 2× with Plate pairs (1.80×); all earlier gates still pass
- [x] `wip` commit — **end of stage 2**
- Departures: damping is a cutoff in Hz and is folded into the decay solve; no input bandwidth
  filter; shimmer share is per second of loop time — SUMMARY.md, stage 2

**Dependencies:** Task 7

**Estimated effort:** Medium

---

### Task 9: `Source/dsp/SpringEngine.h` and wiring (stage 3)

**Outcome:** Spring is a dispersive tank; nothing of the v1.x pre-chain is left.

- Three springs, echo times 33 / 37 / 41 ms at SIZE 50, SIZE scale ×0.75 / ×1.0 / ×1.33.
- Per spring: delay, M ≈ 80 stretched allpasses `(a + z^−K)/(1 + a·z^−K)`, a = 0.62,
  `K = round(fs / (2 fC))`, fC ≈ 4.3–4.8 kHz and M slightly different per spring, 6th-order
  low-pass at fC, 100 Hz high-pass, inverting feedback.
- `g = 10^(−3 × echoSeconds(1 kHz) / T60)`, echo time including the cascade's group delay.
- Feed L, R, (L+R)/√2; mix to L/R with different weights. Deterministic slow wobble, no noise.
- Delete `FlutterDelay` from `ModulationFx.h` (last caller gone) and its remaining references.

**Verification:**
- [x] Chirp: first echo arrives later at 3 kHz than at 1 kHz by ≥ 2 ms, and the pattern
      repeats at the echo time (9.71 ms, the second echo 14.36 ms, echoes 32.96 ms apart;
      read by centre of gravity, not first arrival — SUMMARY.md, stage 3, item 1)
- [x] RT60 rows for Spring within ±10 %, 44.1/48/96 kHz
      (through `processBlock` −0.2 / +0.2 %; driven directly −2.3..+1.1 % over 27 points, L and R)
- [x] Output band-limited: energy above 6 kHz ≥ 20 dB below the 1 kHz octave
      (−30.8 dB; read off the spectrum — SUMMARY.md, stage 3, item 2)
- [x] No `PEND` left on any engine gate; all earlier gates still pass
      (113 PASS, 0 FAIL, 1 PEND — the bank's +5 dB ceiling, Task 11)
- [x] `wip` commit — **end of stage 3**
- Departures: no pre-delay on Spring; the transition frequency is the nearest `fs / (2K)`;
  a mono bus folds Spring by its own factor — SUMMARY.md, stage 3

**Dependencies:** Task 8

**Estimated effort:** Medium

---

### Task 10: Level — wet trims and the decay law (stage 4)

**Outcome:** Types are level-matched again and long tails do not run hot.

- Re-measure `wetTrimDb` per type at defaults (K-weighted pink, WET 100 / DRY 0), stereo and mono.
- Measure wet level against DECAY 0.5/1.0/2.0 and SIZE 0/100 per type. If the spread across
  DECAY exceeds 3 dB, scale the slot input by a partial `1/sqrt(T60)` law (exponent chosen
  from the measurement); if not, leave it alone and say so.

**Verification:**
- [x] Section 6: type loudness spread ≤ 1 dB (gate), target ≤ 0.1 dB (v1.12.0 measured 0.07) — 0.07 dB
- [x] Same in mono — 0.07 dB, with a per-type mono trim
- [x] The DECAY level table is in SUMMARY.md, with the decision (half taken back; SIZE, which this task
      did not mention, fully)
- Departures: a SIZE level law; dB per octave rather than `1/sqrt(T60)`; two new gates per type and a
  mutant — SUMMARY.md, stage 4

**Dependencies:** Task 9

**Estimated effort:** Small

---

### Task 11: Re-voice the 48 factory presets

**Outcome:** The bank suits the new engines.

Edit the `bank[]` table only. Names, TYPE, 8-per-type and one WET 100 / DRY 0 Send per type
are fixed, so the stale-file sweep is a no-op. Presets do not set output gain. Titles that
promise realism (Vocal Booth, Concert Hall, Studio Plate, Amp Spring…) get plausible decay
times: record each preset's resulting RT60 in seconds beside its row in SUMMARY.md, since
DECAY now means something. Plate SIZE spans a narrower audible range — spread presets
accordingly.

**Verification:**
- [x] Section 9: 8 per type, six Sends at 100/0, insert presets ≤ +5 dB re input (+0.2..+2.9)
- [x] Section 1: 48 presets recall their TYPE
- [x] SUMMARY.md lists all 48 with level and RT60

**Dependencies:** Task 10

**Estimated effort:** Medium

---

### Task 12: Tooltips — `tip.type`, `tip.decay`, `tip.size`

**Outcome:** The three tips describe what the controls now do.

`Source/ui/public/js/i18n.js` bodies and their source comments (which cite line numbers and
arithmetic of the Freeverb path). New facts to state: TYPE picks one of three engines; DECAY
is a multiplier on the type's own decay time and the tail length holds as SIZE moves; SIZE
scales the space (FDN ×0.5–×2.0) and bends pitch briefly while it moves. "Booth at 100 % is
still smaller than Hall at 0 %" must be re-checked against the new delay tables before it is
kept (Booth max 13.7 ms × 2 = 27.4 ms vs Hall min-scale 83.1 × 0.5 = 41.6 ms — holds). No
seconds readout, no layout change. fr goes through the glossary and lint; zh-Hans through its
glossary, lint and back-translation; new bodies are marked unreviewed until Taylor reads the
French and the zh-Hans back-translation.

**Verification:**
- [x] `scripts/check-i18n.js`, `i18n-fr-lint.js`, `i18n-zh-lint.js` clean for O-SimpleReverb
- [x] `tests/ui_tip_render_check.js` passes at the shipping viewport in all three languages
      (the longest new body still fits — French `tip.type`, 9.1 px from the bottom edge; the gate now
      sweeps zh-Hans, which it did not before)
- [x] `check-ui-labels.js` unchanged result; `index.html` and layout CSS untouched
- Open: fr `reviewed: false`, zh-Hans `'mt'` until Taylor reads `I18N-REVIEW.md`

**Dependencies:** Task 9 (text depends on final behaviour); independent of Tasks 10–11

**Estimated effort:** Medium

---

### Task 13: Negative control against v1.14.0

**Outcome:** The new gates are shown to fail on the old engine.

`git archive 02bac73f plugins/O-SimpleReverb modules cmake …` into the scratchpad, drop the
new `tests/render-check/` over it, build out of tree, run. Never a checkout in the working
tree. Copy any gitignored UI module files the build needs.

**Verification:**
- [x] On v1.14.0 these fail: RT60 table, RT60-holds-across-SIZE, SIZE-moves-structure,
      type resonance decorrelation, early taps at output, spring chirp (51 PASS, 42 FAIL of 93)
- [x] These pass on both: `params.tsv` identity, state-blob load
- [x] Result table in SUMMARY.md (gate × v1.14.0 × v2.0.0)

**Dependencies:** Task 11

**Estimated effort:** Small

---

### Task 14: Version, docs, build, install

**Outcome:** 2.0.0 is built, validated and installed for the listening pass.

The version bump happens here, not after verify: the factory bank is rewritten only when
`.factory-version` differs from `JucePlugin_VersionString`, so an install still at 1.14.0
would play the old preset values in the DAW.

- `CMakeLists.txt` VERSION 2.0.0; header comments in `PluginProcessor.h` / render-check.
- `CHANGELOG.md` — heading exactly `## [2.0.0] - <date>`; baseline-vs-new table; the
  "sounds different by design" note; what was measured and what is still by ear.
- `NOTES.md` — DSP Architecture section rewritten; lifecycle line added.
- `PLUGINS.md` row → 2.0.0. The file currently carries another session's staged change:
  patch this row only, and commit via a temp index on a fresh `HEAD`.
- `./scripts/build-and-install.sh O-SimpleReverb` (target name, not folder); Standalone
  rebuilt separately if the listening pass uses it.
- pluginval strictness ≥ 5 (10 if time allows — it catches latent NaNs); `auval -v aufx OuSr <mfr>`
  (targeted — `auval -a` aborts on this machine).
- **No git tag.** Tags are cut by `/publish` only, in the form `O-SimpleReverb-v2.0.0`.

**Verification:**
- [x] render-check all PASS, zero PEND, in the recorded build directory (126 PASS; mutants 30 of 30)
- [x] Build without warnings; pluginval passes (strictness 10); `auval -v` passes
- [x] Installed bundle reports 2.0.0; no alternate `-dev`/unsuffixed variant left on disk
- [x] SUMMARY.md written; `wip` commits left as they are — squashing is Taylor's call at verify
- Not done: the Standalone was not rebuilt (SUMMARY.md, stage 4, Build and install)

**Dependencies:** Task 12, Task 13

**Estimated effort:** Medium

---

## Dependency Graph

```
T1 ─> T2 ─> T3 ─┬─> T4 ─┐
                └─> T5 ─┴─> T6 ─> T7 ─> T8 ─> T9 ─┬─> T10 ─> T11 ─> T13 ─┐
                                                  └─> T12 ───────────────┴─> T14
   stage 0   |        stage 1          | st. 2 | st. 3 |        stage 4
```

**Parallelizable:** T4 ∥ T5 (separate headers). T12 ∥ T10–T11 (different files; `i18n.js`
vs `PluginProcessor.cpp`). Everything else is serial — each engine lands on a green gate.

---

## Execution Order

| Order | Task | Stage | Dependencies | Parallelizable With |
|-------|------|-------|--------------|---------------------|
| 1 | 1 Backup, fixtures, test build | 0 | None | - |
| 2 | 2 Measurement library + baseline | 0 | 1 | - |
| 3 | 3 ReverbPrimitives.h | 1 | 2 | - |
| 4 | 4 FdnEngine.h | 1 | 3 | 5 |
| 4 | 5 EarlyReflections.h | 1 | 3 | 4 |
| 5 | 6 Slot host | 1 | 4, 5 | - |
| 6 | 7 Stage-1 gates | 1 | 6 | - |
| 7 | 8 Plate | 2 | 7 | - |
| 8 | 9 Spring | 3 | 8 | - |
| 9 | 10 Level | 4 | 9 | 12 |
| 10 | 11 Presets | 4 | 10 | 12 |
| 9–10 | 12 Tooltips | 4 | 9 | 10, 11 |
| 11 | 13 Negative control | 4 | 11 | - |
| 12 | 14 Version, docs, build, install | 4 | 12, 13 | - |

---

## Risk Notes

1. **Plate echo density** (0.84 at 100 ms against the FDN's 0.99)
   - Impact: "fast echo-density build-up" reads as grainy at large SIZE / short DECAY.
   - Mitigation: SIZE range ×0.40–0.80 (Task 8); density reported; judged at the listening
     pass. Fallback: more input-diffusion stages, measured the same way.

2. **Level against DECAY and SIZE** (≈ 3 dB louder at 2.0×; tails to 14 s)
   - Impact: insert presets breach +5 dB; Ambient runs hot.
   - Mitigation: Task 10 measures before Task 11 re-voices; section 9 catches a breach.

3. **Shimmer level and HF voicings are listening decisions**
   - Impact: cannot be closed by a gate.
   - Mitigation: one named constant each; the bloom gate bounds shimmer from both sides;
     verify-phase listening pass before any tag.

4. **Ring-out slot logic** (two slots, retire threshold, steal)
   - Impact: a click or a stuck slot on rapid type/preset changes.
   - Mitigation: Task 7's A-B-A-B and 48-preset-sweep cases, plus the ring-out gate.

5. **Half-finished plugin on `main` between stages**
   - Impact: stages 1–2 leave Plate/Spring on a provisional FDN at version 1.14.0.
   - Mitigation: `wip` commits stay local (no push, no tag) until stage 4; CI releases only
     on a tag. If this session's work must not sit on `main` at all, say so at approval and
     the stages stay uncommitted with a backup snapshot per stage instead.

6. **Gates that pass for the wrong reason**
   - Impact: a green RT60 gate on a measurer that is wrong, or a click gate whose stimulus
     never reaches the threshold.
   - Mitigation: Task 2 unit-checks the measurer on a known decay; Task 7 breaks each new
     gate once; Task 13 runs them on v1.14.0.

7. **Shared checkout** (other plugins' work is uncommitted in this tree; `PLUGINS.md` is
   staged by another session)
   - Impact: a foreign path joins a commit, or a stale-base commit reverts another row.
   - Mitigation: every commit path-scoped to `plugins/O-SimpleReverb` (and, in Task 14 only,
     this plugin's `PLUGINS.md` row), via a temp index, with branch and status re-checked
     immediately before.

8. **Stale `.planning/STATUS.md`** — left alone; milestone state lives in `STATUS.yaml`.

---

## Domain Agent Instructions

**Execute Agent:** general-purpose (needs a shell: every stage ends on a build and a
render-check run; `dsp-agent` has none). Brief it with the DSP real-time rules below.

**Rules to follow:**
- No allocation, lock or system call in `processBlock`. Capacities set in `prepareToPlay`
  from the running sample rate; an oversized host block is chunked, never grown into.
- `juce::dsp::IIR` coefficient updates on the audio thread use `ArrayCoefficients`.
- Every feedback loop's gain bound is stated in a comment beside it (FDN `g_lo < 1`; plate
  `decay ≤ 0.97` with the convex shimmer mix; spring `|g| < 1`).
- NaN guards recover — reset the engine, never latch silence. `SmoothedValue::reset()` does
  not clear a NaN.
- Modulation phases are deterministic. No `juce::Random`, no clock, nothing seeded from `this`.
- Check anything with a time constant or a ramp at 44.1, 48 and 96 kHz.
- Do not touch: `PluginEditor.*`, `index.html`, CSS, parameter layout, state code,
  preset-manager module, any other plugin's directory.
- Long builds and renders run in the background (600 s watchdog).
- Commits: path-scoped, temp index, re-check `git branch --show-current` and
  `git status --short` immediately before. Never `git add -A`. No tags.
- Build the plugins with `./scripts/build-and-install.sh O-SimpleReverb` (cache clear and
  dual-variant sweep included).

**Files to read first:**
- `.planning/improvements/reverb-engine-rewrite/CONTEXT.md` — requirements and acceptance criteria
- `.planning/improvements/reverb-engine-rewrite/RESEARCH.md` — every constant and formula, the gate map
- `.planning/improvements/reverb-engine-rewrite/prototype/{common.h,engines.h,proto.cpp,results.txt}` — the code to port and the numbers to reproduce
- `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp:262-774` — what is replaced and what stays
- `Source/ModulationFx.h` — `OctaveUpShifter` (kept), `FlutterDelay` (removed in Task 9)
- `tests/render-check/main.cpp` — the gate style to extend
- `plugins/O-Bells/tests/render-harness/main.cpp` — the `malloc_logger` allocation gate
- `plugins/O-Strata/Source/dsp/ReverbProcessor.{h,cpp}` — Dattorro tap wiring reference only
- `research/reverb-comprehensive-research.md` §2.4, §2.5

---

## Success Criteria

From CONTEXT.md, the improvement is successful when:

1. [x] `juce::dsp::Reverb` is gone; Booth/Room/Hall/Ambient run the FDN, Plate the Dattorro
       tank, Spring the dispersive spring (Tasks 6, 8, 9)
2. [x] Measured RT60 matches base × DECAY for all six types within ±10 %, and holds across
       SIZE (Tasks 4, 7, 8, 9)
3. [x] Early reflections appear at the output, different in L and R (Tasks 5, 7)
4. [x] Spring shows a dispersive chirp; Plate shimmer builds in the tail and never runs away
       (Tasks 8, 9)
5. [x] TYPE, SIZE, DECAY, CHARACTER and LOW CUT moves are click-free (Task 7)
6. [x] `params.tsv` unchanged; the v1.14.0 state blob loads with equal values (Tasks 1, 7)
7. [x] 48 presets re-voiced; insert presets ≤ +5 dB; type spread re-trimmed (Tasks 10, 11)
8. [ ] v1.14.0 baseline captured and compared row by row (Task 2; comparison in VERIFICATION.md)
9. [x] No allocation in `processBlock`; stable at 44.1/48/96 kHz and with oversized blocks (Task 7)
10. [x] render-check passes; v1.14.0 fails the new gates (Tasks 13, 14)
11. [x] Build succeeds without warnings (Task 14)
12. [x] Pluginval passes (Level 5+); `auval -v` passes (Task 14)
13. [ ] Listening pass in a DAW on the re-voiced bank before any tag (verify phase — Taylor)
14. [ ] `tip.type` / `tip.decay` / `tip.size` describe the new behaviour in en, fr, zh-Hans;
        fr and zh-Hans read by Taylor (Task 12; review at verify)

---

## Approval

Approved by Taylor, 2026-09-30 (`STATUS.yaml` → `phases.plan.approvedAt`). Local per-stage `wip` commits approved; no push, no tag.

---

*Generated by improve-milestone plan phase*
