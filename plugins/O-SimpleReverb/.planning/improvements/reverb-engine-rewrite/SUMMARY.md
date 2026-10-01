# Summary: Reverb engine rewrite

**Plugin:** O-SimpleReverb
**Milestone:** reverb-engine-rewrite
**Phase:** Execute — complete 2026-10-01 (stages 0–4). Next: verify, which includes the listening pass.

Written stage by stage. The result is first; each stage's section follows, oldest first, with stage 4 last.

---

## Result

O-SimpleReverb **2.0.0** is built and installed (`O-SimpleReverb-dev`, VST3 + AU). `juce::dsp::Reverb` is
gone: Booth / Room / Hall / Ambient run the 16-line FDN with early reflections at the output, Plate
Dattorro's tank with the shimmer in the loop, Spring three dispersive springs. Nothing is tagged or pushed.

| Success criterion (PLAN.md) | State |
|---|---|
| 1 `juce::dsp::Reverb` gone; three engines | done — `grep -rn "dsp::Reverb" Source/` is empty |
| 2 RT60 = base x DECAY within 10 %, holding across SIZE | done — worst +2.8 % through `processBlock`, +5.3 % driven directly over three rates |
| 3 Early reflections at the output, different in L and R | done — 4–5 arrivals a side, none shared |
| 4 Spring chirp; Plate shimmer builds and never runs away | done — 9.7 ms at 3 kHz, echoes 33 ms apart; +21.9 dB bloom; decays even asked for an endless tail |
| 5 TYPE, SIZE, DECAY, CHARACTER, LOW CUT click-free | done — all section 5 gates |
| 6 `params.tsv` unchanged; v1.14.0 state loads | done — byte-identical; 9 of 9 values |
| 7 48 presets re-voiced; inserts <= +5 dB; types re-trimmed | done — +0.2..+2.9 dB; type spread 0.07 dB stereo and mono |
| 8 v1.14.0 baseline captured and compared | captured: `BASELINE.md` and `BASELINE-v2.0.0.md` (same tool); the row-by-row comparison is verify's |
| 9 No allocation; stable at 44.1 / 48 / 96 kHz, oversized blocks | done — 0 allocations; 192 kHz too |
| 10 render-check passes; v1.14.0 fails the new gates | done — 126 PASS here, 42 FAIL of 93 there |
| 11 Builds without warnings | done — 0 in the plugin and test builds |
| 12 pluginval and `auval -v` | done — pluginval strictness 10 SUCCESS; `auval -v aufx OuSr OuDv` AU VALIDATION SUCCEEDED |
| 13 Listening pass before any tag | **open — Taylor, at verify** |
| 14 Three tooltips rewritten in en / fr / zh-Hans; fr and zh-Hans read | written and gated; **reading is open — `I18N-REVIEW.md`** |

**Decisions made in execute that the plan did not make** (each is in its stage's "Departures"):
the level law (SIZE fully compensated, DECAY half — stage 4); a mono bus now as loud as a stereo one,
about 6 dB wetter than v1.14.0's mono (stages 1 and 4); no pre-delay on Spring and a chirp that differs
by sample rate (stage 3); Plate's echo density under target from SIZE 50 up (stage 2).

---

## Stage 0 — Baseline (Tasks 1–2) — complete 2026-09-30

`Source/` is untouched. Everything in this stage is under `tests/` and the milestone directory.

### What exists now

| Item | Where |
|---|---|
| v1.14.0 backup, verified | `backups/O-SimpleReverb/v1.14.0/` (`scripts/verify-backup.sh` passes) |
| Tests build | `cmake-build-tests/` — Release, `OUARICON_BUILD_TESTS=ON`, only O-SimpleReverb configured |
| Measurement library | `tests/render-check/measure.h` |
| v1.14.0 state fixture | `tests/fixtures/state-v1.14.0.bin` (380 bytes) + `.tsv` |
| Baseline | `BASELINE.md` (tool output + hand-written reading notes) |

render-check has two new modes, `--baseline` and `--write-fixture`, and a new section 10 (18
checks: the measurer on known signals, and the state fixture). **44 PASS, 0 FAIL**; the 26
pre-existing checks print the same lines as on the unmodified tree. `params.tsv` from param-dump
is byte-identical.

To rebuild and run:

```bash
ninja -C cmake-build-tests O-SimpleReverb-render-check O-SimpleReverb-param-dump
RC=cmake-build-tests/plugins/O-SimpleReverb/O-SimpleReverb-render-check_artefacts/Release/O-SimpleReverb-render-check
$RC 2>/dev/null            # gates (the preset manager logs to stderr)
$RC --baseline 2>/dev/null # the BASELINE.md table, for the row-by-row comparison at verify
```

### `measure.h`

Works on plain vectors below one line and through `prepareToPlay`/`processBlock` above it, so
stage 1 can point the same functions at an engine header driven directly.

| Function | Reads |
|---|---|
| `impulseResponse` | stereo IR after a 0.5 s silent settle, unit impulse in both channels |
| `midRt60` / `hfRt60` / `lfRt60` | T30 in 354–1414 Hz, the 8 kHz octave, the 125 Hz octave |
| `correlation` | zero-lag L/R over a window |
| `tailSpectrumDb` + `pearson` | whether two tails share resonances (200–2000 Hz, 0.5 Hz, detrended) |
| `onsetIndex`, `earlyTaps`, `tapsWithoutPartner` | first arrival, discrete arrivals, L taps with no R partner |
| `echoDensityAt`, `mixingTimeMs` | Abel & Huang echo density |
| `bandArrivalMs` | when a narrow band first arrives (the spring chirp) |
| `cpuPercent` | wall time in `processBlock`, reported only |

### Departures from PLAN.md

1. **Build directory is `cmake-build-tests/`, not `build-tests/`.** `build-tests/` is not in
   `.gitignore` and would have shown as untracked in every session; `cmake-build-*/` already is.
   It is configured with `SKIP_PLUGINS` set to every other plugin, so configure takes 5 s.
2. **"render-check 32/32" is 26.** The unmodified v1.14.0 render-check prints 26 PASS lines and
   `ALL PASS`. CHANGELOG and NOTES say 32/32. Nothing fails; the count in the docs is off.
3. **The ±2 % measurer check is on the mean over seeds.** One realisation of decaying noise
   scatters 2.8 % at 0.2 s and 2.3 % at 0.4 s (measured over 24 seeds) — that is the signal, not
   the measurer, which reads a decaying tone within 0.01 %. Single-seed first draft: −6.2 %.
4. **Band arrival is the first arrival, not the envelope peak, through a doubled band filter.**
   The plan's envelope-peak metric read Spring +12.5 ms and Room −88.6 ms between 1 and 3 kHz on
   v1.14.0, which has no dispersion: in a tail that is still building, a band's largest sample
   falls anywhere. A v1.14.0 Spring would have passed the chirp gate. The first-arrival reading
   gives −0.6..−2.1 ms on all six types. The band filter runs twice because one pass let a burst
   an octave away through at −9 dB (the self-check caught it: 0.02 ms where 5 ms was built in).
5. **`inject-context.py` failed** (`'stages'` — the stale `.planning/STATUS.md` has no stage
   table). No research context was injected; PLAN.md's own file list was used instead.

### Gates shown to fail

| Gate | Broken how | Result |
|---|---|---|
| state fixture loads | `.tsv` SIZE edited 83 → 84 | FAIL (`SIZE: fixture 84.0000, loaded 83.0000`), restored → PASS |
| measurer, decaying noise within 2 % | single seed instead of the mean | FAIL at 0.2 s (−4.0 %) and 0.4 s (−6.2 %) |
| measurer, chirp of 5 ms | single-pass band filter | FAIL (0.02 ms) |

### For stage 1 (from BASELINE.md)

- "SIZE moves the structure" must read first-arrival or tap times. Mixing time moves with SIZE in
  v1.14.0 (Ambient 85 → 278 ms) with no delay changing, so that reading would pass the old engine.
- Late-tail L/R correlation: v1.14.0 already passes `|r| < 0.3` on four types; only Booth and
  Spring fail. The negative control in Task 13 should not list it as an across-the-board failure.
- Booth at DECAY 0.5× (0.20 s target) sits about 3 standard deviations inside ±10 % on a
  noise-like tail. If the FDN reads near the edge there, the fix is not a wider tolerance.
- v1.14.0 at DECAY 2.0× reads 10.4–11.2 s on every type, Booth included.
- render-check writes the preset manager's log to stderr; redirect it when capturing output.

---

## Stage 1 — FDN, early reflections, slot host (Tasks 3–7) — complete 2026-09-30

`juce::dsp::Reverb` is gone. Booth, Room, Hall and Ambient run the 16-line FDN with stereo early
reflections at the output; Plate and Spring run the same FDN on a stand-in delay set until their
engines land (stages 2 and 3). Version is still 1.14.0.

### What exists now

| Item | Where |
|---|---|
| Ring delay, allpass, one-pole, RT60 ↔ gain, mid-band shelf solve, glide | `Source/dsp/ReverbPrimitives.h` |
| 16-line Hadamard FDN | `Source/dsp/FdnEngine.h` |
| Stereo output tap line | `Source/dsp/EarlyReflections.h` |
| Two ring-out slots, chunked `processBlock`, NaN recovery, reshaped `TypePreset` | `Source/PluginProcessor.{h,cpp}` |
| Gates: sections 3, 5, 6, 7 reworked; 11 (engine headers) and 12 (reverb through `processBlock`) new | `tests/render-check/main.cpp` |
| `--mutants`, `--levels` modes | same |
| Test hooks, render-check target only | `OSIMPLEREVERB_TEST_HOOKS` in `CMakeLists.txt` |

### Results

render-check: **84 PASS, 0 FAIL, 6 PEND**. `--mutants`: **14 of 14 caught**. `params.tsv` byte-identical.
VST3 + AU + Standalone build with no compiler warning. pluginval strictness 10: SUCCESS.
`auval -v aufx OuSr OuDv`: AU VALIDATION SUCCEEDED.

Mid RT60 through `processBlock`, 48 kHz (target = base × DECAY, tolerance ±10 %):

| Type | 0.5× | 1.0× | 2.0× | SIZE 0 | SIZE 100 | worst |
|---|---|---|---|---|---|---|
| Booth (0.40 s) | 0.197 | 0.397 | 0.803 | 0.405 | 0.393 | −1.8 % |
| Room (1.1 s) | 0.556 | 1.102 | 2.181 | 1.074 | 1.093 | −2.3 % |
| Hall (3.0 s) | 1.530 | 2.995 | 6.027 | 2.995 | 3.017 | +2.0 % |
| Ambient (7.0 s) | 3.505 | 6.983 | 14.165 | 7.067 | 7.044 | +1.2 % |

Driven directly at 44.1 / 48 / 96 kHz × DECAY 0.5 / 1 / 2 × size ×0.5 / ×1 / ×2 (27 points a type), worst
errors are Booth +5.3 %, Room −2.4 %, Hall +3.1 %, Ambient +2.2 %. Ambient at 192 kHz, DECAY 2.0×, size ×2
reads 14.10 s.

| Other reading | v1.14.0 | now |
|---|---|---|
| Tail-spectrum correlation between two FDN types, worst pair | 0.96 | −0.06 |
| Late-tail L/R correlation, worst type | +0.54 (Booth) | −0.10 |
| First arrival, SIZE 0 → 100 | does not move | 1.15 ms (Booth) … 9.48 ms (Ambient) later |
| Arrivals within 12 dB of the largest, first 60 ms, per side | about 100 | 4–5, none shared between L and R |
| Old tail 200 ms after Hall → Booth | −17.4 dB, then decays at 0.73 s | −3.7 dB, decays at 3.03 s |
| Allocations in `processBlock`, 600 blocks incl. oversized | 4 | 0 |
| CPU per type, 48 / 96 kHz (one slot; a ring-out runs two) | 0.32–0.43 % / 0.65–0.90 % | 0.57–0.66 % / 1.24–1.42 % |

The v1.14.0 column is the new `tests/render-check` built against `git archive 02bac73f` out of tree
(a first run of Task 13): **45 PASS, 22 FAIL, 6 PEND**. Every section-12 gate fails there, plus the
oversized-block and preset-sweep cases of sections 3 and 5; the state blob, `params.tsv` and sections
1, 2, 4, 6, 7, 10 pass on both. Task 13 repeats it on the finished gates.

### Mutants (`render-check --mutants`)

Each is a one-line break compiled into the render-check target only (`OSR_MUTANT(...)` is `false` in
the plugin). The gate named must fail on it.

| Mutant | Gate | Reading on the broken build |
|---|---|---|
| DECAY does not reach the engine | RT60 = base × DECAY | Room 1.102 s at all three (+100 %) |
| SIZE scales the decay time | RT60 holds across SIZE | 0.552 s / 2.212 s (+101 %) |
| SIZE does not reach the engine | first arrival moves with SIZE | 0.00 ms |
| every type on Hall's delay set | tail spectra unrelated | 0.44 (limit 0.3) |
| early reflections muted | discrete early arrivals | 108 / 111 arrivals, 53 unshared |
| right = left | L/R correlation | 1.000 |
| old slot cleared on TYPE change | ring-out | tail gone |
| no NaN guard | NaN recovery | output not finite |
| oversized block processed whole | no allocation | 5 allocations |
| oversized block processed whole | oversized = 4 × prepared | max difference 0.53 |
| SIZE lands at once | SIZE HF burst | Room −3.8 dB, Hall +11.7 dB |
| DECAY lands at once | DECAY HF burst | −30.3 / −29.4 dB |
| sounding slot taken back with no fade | rapid 3-type switching | −3.1 dB |
| sounding slot taken back with no fade | 48-preset sweep | −23.2 dB above 8 kHz |

The tail-spectrum gate has the thinnest margin (0.44 against 0.3). The state-fixture gate was broken by
hand in stage 0.

### Departures from PLAN.md

1. **The SIZE glide is two poles in series (125 ms each), not one pole of 250 ms.** One pole starts
   moving at full speed, so the pitch jumps at the move: −40 dB above 3 kHz on Hall, 20 dB over a
   smoothed WET move. RESEARCH's "−63.4 dB" for it was re the wet peak and its WET reference re the
   input, so the two were never comparable. Through two poles the length's velocity starts at zero:
   Room −68.4 dB, Hall −66.0 dB against a reference of −59.3 dB.
2. **The TYPE duck is two poles (2.5 ms each, 90 % at 10 ms), not a 10 ms ramp; the steal fade is a
   smoothstep.** A ramp that turns round mid-way has a corner: −48.9 dB with TYPE switching every
   16 ms. Now −74.2 dB.
3. **DECAY slews over 60 ms and the per-line gains ramp linearly between recomputes** (every 16
   samples while anything moves). A stepped gain is the −29 dB of the mutant above.
4. **Pre-delay is not scaled by SIZE.** CONTEXT requirement 6 says "where the type uses it"; no type
   does. The first arrival still moves, because the tap times scale. Pre-delays are v1.14.0's.
5. **Mutants are a permanent mode** rather than broken builds made and thrown away, so stages 2–4
   re-run them for free.
6. **The early-taps gate through `processBlock` counts arrivals**, so it needs nothing from the engine
   headers and runs on v1.14.0. Exact tap times, gains and signs are gated on the header (section 11).
7. **The preset sweep is measured above 8 kHz and starts a second in.** Consecutive presets of one
   type move SIZE, and the bend takes a 300 Hz tail to 1.2 kHz, which a 3 kHz filter's skirt passes.
   The first draft read −32.6 dB, all of it the test sine's own start through the dry path.
8. **Rapid-switching cycles start and end on Booth.** A modulated type's level under a steady sine
   depends on its LFO phase; a restarted Room slot read −0.81 dB against one that never switched.
9. **Section 9's +5 dB ceiling prints PEND** until the bank is re-voiced (Task 11). It reads −2.0 ..
   +2.3 dB on the old bank today.
10. **`wetTrimDb` is set for all six types now**, provisionally, to +6.4 dB re input — v1.14.0's own
    stereo figure (6.3..6.5), read from the negative-control build. Task 10 re-measures.
11. **`TypePreset` and `typePresets` are public**, so render-check drives the engine with each type's
    own delay set. `PluginProcessor.h` no longer includes `ModulationFx.h`; `FlutterDelay` has no
    caller left and goes in Task 9 as planned.
12. **The 192 kHz stability check reads RT60 on the impulse response.** A first draft compared
    full-band noise energy 6 s apart and "failed" at −33.7 dB against 25.7: most of a 96 kHz band is
    above the crossover and decays on the HF time.
13. **`inject-context.py` failed again** (`'stages'`); PLAN.md's file list was used.

### Voicing constants that are placeholders

Set by measurement where there was one, otherwise carried from v1.14.0; all are for the listening pass.

| Type | pre-delay | early span | early level | wet trim |
|---|---|---|---|---|
| Booth | 3 ms | 6.9 ms | 0.6 | +6.6 dB |
| Room | 15 ms | 23 ms | 0.4 | +5.4 dB |
| Hall | 50 ms | 46 ms | 0.3 | +5.9 dB |
| Ambient | 35 ms | 57.5 ms | 0.25 | +4.1 dB |
| Spring (stand-in FDN) | 20 ms | — | — | +3.7 dB |
| Plate (stand-in FDN) | 8 ms | — | — | +2.1 dB |

Early spans are v1.14.0's last tap (23 ms × its per-type scale). Early levels put the taps' energy
about 4–9 dB under the tail's.

### For the stages ahead

- **Level against DECAY and SIZE (Task 10).** `render-check --levels`, wet re input, K-weighted pink:

  | Type | default | DECAY 0.5× | DECAY 2.0× | SIZE 0 | SIZE 100 |
  |---|---|---|---|---|---|
  | Booth | +6.40 | +4.54 | +8.62 | +9.00 | +4.67 |
  | Room | +6.38 | +4.32 | +8.77 | +8.89 | +4.21 |
  | Hall | +6.43 | +4.20 | +8.97 | +9.04 | +4.41 |
  | Ambient | +6.36 | +3.93 | +8.73 | +9.09 | +4.13 |

  The spread across DECAY is 4.1–4.8 dB, over Task 10's 3 dB threshold, so the `1/sqrt(T60)` law is
  due. **SIZE spreads it as far (4.3–5.0 dB, small is louder) and PLAN's Task 10 does not mention
  it** — same energy in less delay. Decide there whether SIZE gets the same treatment.
- **Mono is now as loud as stereo.** The engines run stereo and fold `0.7071 (L + R)`: mono wet reads
  +6.1..+7.0 dB. v1.14.0's mono wet read −0.1..+0.9 dB. A mono-bus session comes up about 6 dB wetter.
  The mono spread across types is 0.80 dB on stereo-derived trims (gate: 1 dB).
- **Plate and Spring rows** print PEND whatever the stand-in FDN reads. Stage 2 replaces Plate's
  PENDs (RT60, bloom, no-runaway) with gates; stage 3 Spring's (RT60, chirp).
- **`kFdnTypes` in render-check** lists the types the stereo and tail-spectrum gates cover. Plate and
  Spring join, or get their own thresholds, when their engines land — a spring is a mono device and
  may not meet `|r| < 0.3`.
- The TYPE ratio gate reads 1.83× against a limit of 2 (v1.14.0: 1.26×). The new engines' steady
  output above 3 kHz is lower, so the same burst is a larger ratio. In dB it is −56.7.

### Build and install

`./scripts/build-and-install.sh O-SimpleReverb` ran at the end of the stage. **Its Phase 4 swept the
release-branded `O-SimpleReverb.vst3` / `.component` off the machine** (same AU triple as the dev
build) and installed `O-SimpleReverb-dev`. What is installed is this stage's build: version 1.14.0,
new FDN on four types, stand-in FDN on Plate and Spring, v1.14.0's preset values.

---

## Stage 2 — Plate (Task 8) — complete 2026-09-30

Plate runs Dattorro's figure-8 tank with the octave shifter inside the loop. Spring is still the
stand-in FDN. Version is still 1.14.0.

### What exists now

| Item | Where |
|---|---|
| Dattorro tank: stereo in, 14 output taps, gliding lengths, decay in seconds, in-loop shimmer | `Source/dsp/PlateEngine.h` |
| Slot runs the plate or the FDN by type (`runsPlate`); only the active engine is cleared | `Source/PluginProcessor.{h,cpp}` |
| Gates: bloom (section 7), plate driven directly (11), Plate rows and no-runaway (12) | `tests/render-check/main.cpp` |
| Mutants `noShimmer`, `decayStuck`; the three RT60 / SIZE mutants repeated on Plate | same |

### Results

render-check: **96 PASS, 0 FAIL, 3 PEND** (Spring RT60, Spring chirp, the bank's +5 dB ceiling).
`--mutants`: **19 of 19 caught**. `params.tsv` byte-identical. VST3 + AU + Standalone build with no compiler
warning. pluginval strictness 10: SUCCESS. `auval -v aufx OuSr OuDv`: AU VALIDATION SUCCEEDED.
Installed: `O-SimpleReverb-dev` 1.14.0, this stage's build.

Mid RT60, Plate (2.5 s base), 48 kHz through `processBlock`:

| 0.5× | 1.0× | 2.0× | SIZE 0 | SIZE 100 | worst |
|---|---|---|---|---|---|
| 1.266 | 2.514 | 4.940 | 2.571 | 2.515 | +2.8 % |

Driven directly at 44.1 / 48 / 96 kHz × DECAY 0.5 / 1 / 2 × size ×0.40 / ×0.57 / ×0.80: −1.7..+2.5 % with
the shifter in the loop, −0.3..+3.6 % with it out. At 192 kHz, DECAY 2.0×, size ×1: 4.91 s.
RESEARCH's 1.055 loop correction is unchanged.

| Other reading | Value |
|---|---|
| 600 / 300 Hz while a 300 Hz burst sounds → over the 2 s after it | −35.2 → −13.3 dB (+21.9) |
| Room's tail, same reading | −74.9 dB |
| 8 kHz / 125 Hz RT60 at DECAY 1.0×, SIZE 50 | 1.85 s / 2.75 s |
| Late-tail L/R correlation, worst of 27 points | 0.022 |
| First arrival, SIZE 0 → 100 | 11.58 → 15.15 ms |
| DECAY 2.0×, 10 s of noise: level 5–6 s after it stops | −69.8 dB (SIZE 0), −71.8 dB (SIZE 100) |
| Asked for an endless tail: energy 10 / 20 / 30 s on, shifter share 1.0 | −67.7 / −93.9 / −118.3 dB |
| SIZE 0↔100 / DECAY 0.5↔2.0 HF burst | −62.0 / −58.3 dB (WET reference −59.3, limit +6) |
| TYPE ratio gate, six pairs incl. Hall↔Plate and Spring↔Plate | 1.80× (limit 2) |
| CPU, 48 / 96 kHz | 0.32 % / 0.76 % (the FDN types: 0.63 % / 1.3 %) |

**Echo density is under the plan's target from SIZE 50 up.** 100 ms after the first arrival
(1.0 = Gaussian; Hall reads 0.99):

| SIZE | scale | density | mixing time |
|---|---|---|---|
| 0 | ×0.40 | 0.92 | 92 ms |
| 50 | ×0.57 | 0.77 | 210 ms |
| 100 | ×0.80 | 0.70 | 255 ms |

Target was ≥ 0.80; the prototype read 0.84 at ×0.55. Reported, not gated, as planned. If Plate
sounds grainy at the listening pass, the options are a lower `sizeHi` (one number in `typePresets`)
or more input diffusion, measured the same way.

### Mutants added

| Mutant | Gate | Reading on the broken build |
|---|---|---|
| DECAY does not reach the plate | Plate RT60 = base × DECAY | 2.51 s at all three (+101 %) |
| SIZE scales the plate's decay time | Plate RT60 holds across SIZE | 1.80 s / 3.53 s (+41 %) |
| SIZE does not reach the plate | Plate first arrival moves | 0.00 ms |
| shifter bypassed | bloom | tail ratio −79.2 dB (must be −30..−6) |
| endless tail asked for (coefficient at its 0.97 ceiling) | no runaway | −37.3 / −28.0 dB after 5 s (must be < −45) |

### Departures from PLAN.md

1. **The bloom gate also bounds the tail ratio (−30..−6 dB), not only its rise.** With the shifter
   bypassed the "rise" still read +28.5 dB: both readings were leakage (−108 and −79 dB). The
   `noShimmer` mutant caught it. The upper bound keeps the octave under the fundamental.
2. **Damping is a cutoff in Hz (10 kHz) and is part of the decay solve.** The prototype used a raw
   one-pole coefficient (0.25), which is a different filter at every sample rate. Its gain at 1 kHz is
   now divided out of `decay`, as the FDN's shelf is.
3. **No input bandwidth filter.** Dattorro's 0.9995 passes everything; CHARACTER and the type EQ follow.
4. **Shimmer share is a rate, `kShimmerPerLoopSecond` = 0.121**, which is the planned 0.05 at SIZE 50
   (0.035 at SIZE 0, 0.070 at SIZE 100). Its low-pass is 4.5 kHz in Hz, not a raw coefficient.
5. **The "no runaway" mutant is a tail that does not end, not a tank that grows.** Nothing reachable
   from the host makes this loop grow: the coefficient is clamped and the shimmer mix is convex. The
   growth case is gated on the header instead (endless tail, shifter share 0 / 0.04 / 1.0).
6. **Plate joined the SIZE and DECAY glide gates and the stereo gate**; it is not in the
   tail-spectrum gate (FDN delay sets only) and has no early-taps gate (a plate has none).
7. **`clearSlot` clears only the engine the slot's type runs.** The other one is cleared when a type
   that runs it starts.
8. **`sizeIsDecay` now scales by size relative to SIZE 50**, so it means the same on the plate
   (×0.57 there) as on the FDN (×1, unchanged).
9. **`inject-context.py` was not run** — it has failed twice on the stale `.planning/STATUS.md`.

### Voicing constants that are placeholders (Plate)

| Constant | Value | Where |
|---|---|---|
| Shimmer share per loop second | 0.121 | `PlateEngine::kShimmerPerLoopSecond` |
| In-loop damping | 10 kHz | `PlateEngine::kDampingHz` |
| SIZE range | ×0.40..×0.80 | `typePresets[4]` |
| Pre-delay / type EQ | 8 ms / +3 dB shelf at 5 kHz | `typePresets[4]` (v1.14.0's) |
| Wet trim | +1.9 dB (stereo spread across types 0.07 dB) | `typePresets[4]`, Task 10 re-measures |

### For the stages ahead

- **Task 10.** Plate's level moves less with SIZE than the FDN's (+7.56 / +5.48 dB at SIZE 0 / 100,
  against about +9.0 / +4.4), and as much with DECAY (+4.69 / +8.80 dB). One law for all engines will
  not fit SIZE.
- **Task 13.** The bloom gate has not been run on v1.14.0 yet. It should fail there (the one-shot
  shimmer is at the same ratio from the first sample).
- **Stage 3.** `PlateEngine.h` includes `ModulationFx.h` for `OctaveUpShifter`; removing
  `FlutterDelay` from that header must leave the shifter and its `juce_dsp` include.
- Plate's first arrival through the plugin is 13.06 ms at SIZE 50: 8 ms pre-delay plus the tank's
  first tap (266 samples at 29761 Hz × 0.57).

---

## Stage 3 — Spring (Task 9) — complete 2026-09-30

Spring runs three dispersive springs. All six types are on their own engines; nothing of the v1.x
pre-reverb chain is left (`FlutterDelay` is deleted). Version is still 1.14.0.

### What exists now

| Item | Where |
|---|---|
| Three springs: delay, 72 / 80 / 88 stretched allpasses, 6th-order low-pass, 100 Hz high-pass, inverting reflection; decay in seconds, gliding echo time | `Source/dsp/SpringEngine.h` |
| Slot runs FDN, plate or spring by type (`engineOf`); per-slot mono fold | `Source/PluginProcessor.{h,cpp}` |
| `ModulationFx.h` is the octave shifter only | `Source/ModulationFx.h` |
| Measurers: band centre of gravity, echo period, energy above a frequency (each self-checked in section 10) | `tests/render-check/measure.h` |
| Gates: spring driven directly (11), Spring rows, chirp and band limit (12) | `tests/render-check/main.cpp` |
| Mutants `springNoChirp`, `springOpenBand`; the three RT60 / SIZE mutants repeated on Spring | same |

### Results

render-check: **113 PASS, 0 FAIL, 1 PEND** (the bank's +5 dB ceiling, Task 11). `--mutants`: **24 of 24 caught**.
`params.tsv` byte-identical. VST3 + AU + Standalone build with no compiler warning. pluginval strictness 10
(GUI tests skipped): SUCCESS. `auval -v aufx OuSr OuDv`: AU VALIDATION SUCCEEDED.
Installed: `O-SimpleReverb-dev` 1.14.0, this stage's build.

Mid RT60, Spring (2.5 s base), 48 kHz through `processBlock`:

| 0.5× | 1.0× | 2.0× | SIZE 0 | SIZE 100 | worst |
|---|---|---|---|---|---|
| 1.251 | 2.505 | 4.988 | 2.497 | 2.505 | −0.2 % |

Driven directly at 44.1 / 48 / 96 kHz × DECAY 0.5 / 1 / 2 × size ×0.75 / ×1 / ×1.33, both outputs:
−2.3..+1.1 %. At 192 kHz, DECAY 2.0×, size ×1.33: 4.96 s.

| Other reading | v1.14.0 | now |
|---|---|---|
| First echo, 3 kHz behind 1 kHz (SIZE 50, left spring) | none (−2.1 ms by first arrival) | 9.71 ms; 2 kHz 1.79 ms |
| Second echo, the same | — | 14.36 ms; 2 kHz 3.68 ms |
| Echo spacing (envelope match) | no echoes | 32.96 ms (0.81) |
| The same at 44.1 / 48 / 96 kHz × 3 sizes × 2 springs, driven directly | — | 4.47..10.48 ms; second echo ≥ 1.32×; spacing within 0.10 ms of the engine's figure |
| Energy above 6 kHz re the 1 kHz octave | not measured | −30.8 dB (Room: +11.0) |
| First arrival, SIZE 0 / 50 / 100 | 45.5 ms, does not move | 23.94 / 32.19 / 43.17 ms |
| Late-tail L/R correlation, mono input | +0.35 | +0.30 (driven directly: +0.34) |
| 125 Hz RT60 at DECAY 1.0× | 0.88 s | 1.91 s (the loop's 100 Hz high-pass) |
| Echo density 100 ms after the first arrival | 0.97 | 0.83 |
| Asked for an endless tail: energy 10 / 20 / 30 s on | — | −57.2 / −114.3 / −170.4 dB |
| SIZE 0↔100 / DECAY 0.5↔2.0 HF burst | — | −63.8 / −62.5 dB (WET reference −59.3, limit +6) |
| CPU, 48 / 96 kHz | 0.43 % / 0.90 % | 1.16 % / 2.31 % |

Spring is now the most expensive type (the FDN types: 0.63 % / 1.34 %; a ring-out runs two slots).
240 allpass stages a sample is where it goes. Nothing was optimised.

### Mutants added

| Mutant | Gate | Reading on the broken build |
|---|---|---|
| DECAY does not reach the spring | Spring RT60 = base × DECAY | 2.50 s at all three (+100 %) |
| SIZE scales the spring's decay time | Spring RT60 holds across SIZE | 1.87 s / 3.35 s (+34 %) |
| SIZE does not reach the spring | Spring first arrival moves | 0.00 ms |
| allpass cascade out of the loop | chirp | 3 kHz −0.07 ms behind 1 kHz; echoes still 33.04 ms apart |
| low-pass out of the loop | band limit | +7.7 dB above 6 kHz |

### Departures from PLAN.md

1. **The chirp is read by each band's centre of gravity over one echo, not by its first arrival.** Stage 0's
   first-arrival metric read 1.23 ms on a spring whose 3 kHz is 9.7 ms behind its 1 kHz, and −8 ms on the
   second echo: a dispersed echo's leading edge in a band is what the band filter's skirts pass of the faster
   frequencies below. The gate asks for all of: 3 kHz ≥ 2 ms behind 1 kHz with 2 kHz in between; the second
   echo ≥ 1.25× the first; echoes 33 ± 1 ms apart with an envelope match ≥ 0.5. It is measured on an impulse
   of +1 in L and −1 in R, which silences the shared spring and leaves one spring alone in each output.
   **Not yet run on v1.14.0** (Task 13). A centre of gravity over a window of dense tail is noise, so the
   echo-spacing condition is what should fail there; Freeverb's combs are 25–37 ms long, so check that it does.
2. **"Energy above 6 kHz" is read off the spectrum (FFT), not through `measure::band`.** A 4th-order high-pass
   at 6 kHz passes 4 kHz 14 dB down and read −15..−19 dB on a spring whose spectrum says −24..−30. I added an
   output low-pass on that reading, then took it out again when the reading turned out to be the filter's.
   The engine is as planned: no filter outside the loop.
3. **The transition frequency is the nearest `fs / (2K)`, and differs by sample rate**: asked for 4300 / 4550 /
   4800 Hz, the springs get 4410 / 4410 / 4410 at 44.1 kHz, 4000 / 4800 / 4800 at 48 kHz, 4364 / 4364 / 4800
   at 96 kHz. Decay time is unaffected (the echo time is computed from the K in use). The chirp is not: the
   left spring's 3 kHz lag is 6.2 ms at 44.1 kHz and 9.8 ms at 48 kHz. **A session moved between rates will
   have a slightly different drip.** A fractional K would need an interpolator in each of 240 stages.
4. **No pre-delay on Spring** (v1.14.0: 20 ms). Nothing leaves a spring before its first echo, which is
   already 24–43 ms after the input; with 20 ms more the first sound was at 52 ms.
5. **Spring is not decorrelated, and a mono bus folds it by 0.612, not 0.7071.** L is spring 1 plus half of
   spring 3, R is spring 2 plus half of spring 3: correlation +0.34 for a mono input, v1.14.0's width. At
   0.7071 a mono bus would come out 1.25 dB hot (the arithmetic, 10 log(1 + r); not rendered), enough to put
   the mono type spread over its 1 dB gate. With the fold the engine reads 0.00 dB and the plugin −0.14 dB.
   Spring stays out of the stereo gate, as planned.
6. **The echo time counts the filters' group delay at 1 kHz as well as the cascade's** (0.14 ms). The prototype
   left it out and read +0.3..+0.7 %.
7. **Reflection gain is capped at 0.98**, the plan's "|g| < 1" made a number. The shortest spring at DECAY 2.0×
   needs 0.966.
8. **SIZE moves the delay line only.** The cascade is the same at every SIZE, so the chirp's length does not
   scale with the echo time.
9. **Stage counts are 72 / 80 / 88**, wider apart than "slightly different", because at 44.1 kHz all three
   springs share one K.
10. **Spring joined the SIZE and DECAY glide gates.**
11. **`inject-context.py` was not run** (see stages 0–2).

### Voicing constants that are placeholders (Spring)

| Constant | Value | Where |
|---|---|---|
| Echo times / stages / transition | 33, 37, 41 ms / 72, 80, 88 / 4300, 4550, 4800 Hz | `SpringEngine::kSpring` |
| Allpass coefficient | 0.62 | `SpringEngine::kAllpassCoefficient` |
| Delay wobble | 0.15 ms at 0.71 / 0.93 / 1.19 Hz (about 1.5 cents) | `kWobbleMs`, `kSpring` |
| Shared spring's level | 0.5 | `SpringEngine::kSharedLevel` (`kMonoFold` follows from it) |
| SIZE range | ×0.75..×1.33 | `typePresets[3]` |
| Type EQ | +4 dB peak at 800 Hz, Q 2.5 (v1.14.0's) | `typePresets[3]` |
| Wet trim | −0.5 dB (stereo spread across types 0.07 dB, mono 0.71 dB) | `typePresets[3]`, Task 10 re-measures |

### For stage 4

- **Task 10.** Spring's row of `--levels`: +6.38 stereo, +6.24 mono, +4.06 / +8.84 at DECAY 0.5× / 2.0×,
  +7.58 / +5.39 at SIZE 0 / 100. With DECAY it moves as the others do (4.8 dB); with SIZE as the plate does
  (2.2 dB), not as the FDN does (4.6 dB).
- **Task 11.** The old bank on the new Spring reads −1.3..+2.9 dB; "Spring - Dub Spring" is the loudest insert
  preset in the bank. Spring's SIZE now means echo time 25–44 ms.
- **Task 12.** `tip.size` has three different SIZE ranges to describe (FDN ×0.5–×2, plate ×0.40–×0.80, spring
  ×0.75–×1.33).
- **Task 13.** Gates not yet run on v1.14.0: bloom, Spring chirp, Spring band limit, and the three new
  measurer self-checks (which must pass on both).
- **Task 14.** NOTES.md and CHANGELOG still describe the Freeverb path; "32/32" there is wrong (stage 0).

---

## Stage 4 — Voicing and release prep (Tasks 10–14) — complete 2026-10-01

The wet level follows a level law, the 48 presets are re-voiced, the three tooltips are rewritten, the
finished gates have been run on v1.14.0, and 2.0.0 is built and installed.

### What exists now

| Item | Where |
|---|---|
| Level law at the slot input; per-type mono trim | `Source/PluginProcessor.{h,cpp}` (`levelLawDb`, `kSizeLevelDbPerOctave`, `kDecayLevelDbPerOctave`, `TypePreset::monoTrimDb`) |
| Re-voiced bank, each row's tail length in its comment | `PluginProcessor.cpp`, `initializeFactoryPresets` |
| Gates: wet level across SIZE and across DECAY per type (section 6); the +5 dB ceiling is a gate again (9) | `tests/render-check/main.cpp` |
| Mutant `noLevelLaw`; `--levels raw` | same |
| `tip.type`, `tip.decay`, `tip.size` in en / fr / zh-Hans | `Source/ui/public/js/i18n.js` |
| zh-Hans sweep in the tip render check | `tests/ui_tip_render_check.js` |
| The three bodies and the blind back-translation, to read | `I18N-REVIEW.md` (milestone directory) |
| v2.0.0's measurement table, same tool as `BASELINE.md` | `BASELINE-v2.0.0.md` (milestone directory) |
| Version 2.0.0, CHANGELOG, NOTES (DSP Architecture rewritten), PLUGINS.md row | `CMakeLists.txt`, `CHANGELOG.md`, `NOTES.md`, `PLUGINS.md` |

### Results

render-check: **126 PASS, 0 FAIL, 0 PEND**. `--mutants`: **30 of 30 caught**. `params.tsv` byte-identical.
The same gates on v1.14.0 out of tree: **51 PASS, 42 FAIL**.

### Task 10 — level

`render-check --levels raw` (law off) and `--levels` (law on): wet loudness re input, K-weighted pink,
WET 100 / DRY 0, dB.

| Type | default | DECAY 0.5x / 0.71x / 1.41x / 2.0x, law off | law on | SIZE 0 / 25 / 75 / 100, law off | law on |
|---|---|---|---|---|---|
| Booth | +6.40 | +4.54 / +5.43 / +7.46 / +8.62 | +5.64 / +5.97 / +6.92 / +7.52 | +9.00 / +7.73 / +5.49 / +4.67 | +6.75 / +6.61 / +6.61 / +6.92 |
| Room | +6.38 | +4.32 / +5.31 / +7.54 / +8.77 | +5.42 / +5.85 / +6.99 / +7.67 | +8.89 / +7.48 / +5.10 / +4.21 | +6.64 / +6.35 / +6.22 / +6.46 |
| Hall | +6.43 | +4.20 / +5.26 / +7.67 / +8.97 | +5.30 / +5.81 / +7.13 / +7.87 | +9.04 / +7.67 / +5.34 / +4.41 | +6.79 / +6.55 / +6.46 / +6.66 |
| Spring | +6.38 | +4.06 / +5.21 / +7.59 / +8.84 | +5.16 / +5.75 / +7.05 / +7.74 | +7.58 / +6.90 / +5.80 / +5.39 | +6.50 / +6.36 / +6.34 / +6.47 |
| Plate | +6.37 | +4.49 / +5.38 / +7.45 / +8.60 | +5.59 / +5.93 / +6.91 / +7.50 | +7.36 / +6.94 / +6.04 / +5.28 | +6.34 / +6.43 / +6.55 / +6.30 |
| Ambient | +6.36 | +3.93 / +5.14 / +7.57 / +8.73 | +5.03 / +5.68 / +7.02 / +7.63 | +9.09 / +7.70 / +5.15 / +4.13 | +6.84 / +6.57 / +6.28 / +6.38 |

**Decision.** The spread across DECAY was 4.1–4.9 dB (over the plan's 3 dB), and across SIZE 2.1–4.9 dB,
which the plan did not cover. The slot's input gain now follows both, in dB per octave of each control:

- **SIZE: all of it** — 2.25 dB per octave on the FDN, 2.05 on the plate, 2.6 on the spring. The engines
  differ per SIZE percent because their ranges differ (two octaves, one, 0.83); per octave of length they
  are close. With the tail time fixed, taking back the steady level also takes back the level the tail
  starts at, so nothing is traded. Left: 0.16–0.57 dB across the knob.
- **DECAY: half of it** — 1.1 of the 2.2 dB per octave measured. Here there is a trade: taking all of it
  back would start a long tail 4.4 dB under a short one. Left: 1.9–2.6 dB across the knob, rising.
  **This half is a choice, not a measurement** (`kDecayLevelDbPerOctave`); it is on the listening list.

The gain moves through two poles of 30 ms, as the duck does. The SIZE and DECAY glide gates moved by
less than 1.1 dB with it (DECAY 0.5↔2.0 on Hall: −57.7 → −56.6 dB against a limit of −52.4).

**Type trims.** Stereo spread 0.07 dB (6.36..6.43) on the trims already in place; nothing to re-measure.
**Mono** was 0.71 dB on the stereo trims, because the fold assumes L and R share nothing and each type
shares a little. `monoTrimDb` (−0.52..+0.14 dB per type) brings mono to the same 0.07 dB.
Mono and stereo are now equally loud; v1.14.0's mono wet read −0.1..+0.9 dB, so a mono session is about
6 dB wetter at the same WET. It is in the CHANGELOG.

### Task 11 — the bank

Names, types, 8 per type and one Send per type are unchanged, so the stale-file sweep had nothing to
remove. Tail = the type's decay time x DECAY (the gates hold that within 10 %). Level is the whole
output re input, K-weighted pink.

| Preset | Tail (s) | Level re input (dB) | CHAR | WET | DRY | DECAY | SIZE | LOW CUT |
|---|---|---|---|---|---|---|---|---|
| Booth - Dark Closet | 0.22 | +1.2 | -65 | 30 | 100 | 0.55x | 5 | off |
| Booth - Drum Close | 0.25 | +0.3 | -30 | 15 | 100 | 0.62x | 20 | 119 Hz |
| Booth - Send | 0.40 | +5.8 | +0 | 100 | 0 | 1.00x | 50 | 150 Hz |
| Booth - Snare Ambience | 0.50 | +0.9 | +25 | 22 | 100 | 1.25x | 55 | 160 Hz |
| Booth - Tight Room | 0.40 | +1.1 | +0 | 25 | 100 | 1.00x | 60 | off |
| Booth - Vocal Booth | 0.24 | +0.6 | +0 | 20 | 100 | 0.60x | 30 | off |
| Booth - Voiceover | 0.20 | +0.2 | -20 | 12 | 100 | 0.50x | 20 | 120 Hz |
| Booth - Whisper | 0.28 | +1.4 | +40 | 30 | 100 | 0.70x | 15 | off |
| Room - Bright Chamber | 1.65 | +1.3 | +55 | 32 | 100 | 1.50x | 70 | 150 Hz |
| Room - Drum Room | 0.71 | +0.9 | +15 | 30 | 100 | 0.65x | 40 | 90 Hz |
| Room - Jazz Club | 1.21 | +1.3 | -20 | 35 | 100 | 1.10x | 60 | 119 Hz |
| Room - Live Room | 0.99 | +1.7 | +0 | 35 | 100 | 0.90x | 55 | off |
| Room - Send | 1.10 | +4.7 | +0 | 100 | 0 | 1.00x | 50 | 120 Hz |
| Room - Small Room | 0.61 | +0.8 | +0 | 25 | 100 | 0.55x | 25 | off |
| Room - Studio A | 0.79 | +1.3 | +10 | 30 | 100 | 0.72x | 45 | off |
| Room - Wood Room | 0.90 | +1.3 | -45 | 30 | 100 | 0.82x | 50 | off |
| Hall - Ballroom | 2.61 | +1.9 | +20 | 35 | 100 | 0.87x | 70 | off |
| Hall - Cathedral | 5.40 | +1.5 | -10 | 40 | 85 | 1.80x | 95 | 100 Hz |
| Hall - Choir Loft | 3.90 | +1.4 | +35 | 38 | 95 | 1.30x | 80 | 140 Hz |
| Hall - Concert Hall | 2.01 | +1.4 | +0 | 30 | 100 | 0.67x | 60 | off |
| Hall - Dark Hall | 3.30 | +1.5 | -65 | 40 | 95 | 1.10x | 75 | 120 Hz |
| Hall - Send | 3.00 | +4.9 | +0 | 100 | 0 | 1.00x | 60 | 120 Hz |
| Hall - Strings Hall | 2.40 | +1.4 | -25 | 35 | 100 | 0.80x | 65 | 100 Hz |
| Hall - Theater | 1.50 | +1.2 | +10 | 30 | 100 | 0.50x | 35 | off |
| Spring - Amp Spring | 2.00 | +0.9 | +0 | 25 | 100 | 0.80x | 40 | off |
| Spring - Bright Tank | 3.25 | +2.1 | +65 | 40 | 95 | 1.30x | 70 | 160 Hz |
| Spring - Dark Tank | 2.75 | +2.0 | -55 | 38 | 100 | 1.10x | 60 | 140 Hz |
| Spring - Dub Spring | 3.50 | +2.5 | -30 | 45 | 90 | 1.40x | 80 | 142 Hz |
| Spring - Send | 2.50 | +6.0 | +0 | 100 | 0 | 1.00x | 50 | 150 Hz |
| Spring - Surf Guitar | 3.00 | +2.9 | +20 | 45 | 100 | 1.20x | 65 | off |
| Spring - Twang | 1.75 | +1.7 | +40 | 35 | 100 | 0.70x | 30 | off |
| Spring - Vintage Spring | 2.50 | +1.4 | -10 | 30 | 100 | 1.00x | 50 | off |
| Plate - Dark Plate | 2.75 | +1.4 | -45 | 35 | 100 | 1.10x | 60 | 120 Hz |
| Plate - Long Plate | 4.50 | +1.8 | +15 | 38 | 95 | 1.80x | 90 | 120 Hz |
| Plate - Lush Plate | 3.25 | +1.6 | -10 | 40 | 95 | 1.30x | 70 | 119 Hz |
| Plate - Send | 2.50 | +4.8 | +0 | 100 | 0 | 1.00x | 50 | 150 Hz |
| Plate - Shimmer Plate | 3.50 | +2.2 | +40 | 35 | 100 | 1.40x | 85 | off |
| Plate - Snare Plate | 1.25 | +0.9 | +30 | 30 | 100 | 0.50x | 10 | 180 Hz |
| Plate - Studio Plate | 2.00 | +1.5 | +10 | 30 | 100 | 0.80x | 40 | off |
| Plate - Vocal Plate | 1.75 | +1.0 | +0 | 25 | 100 | 0.70x | 30 | off |
| Ambient - Cloud Nine | 10.50 | +1.3 | +0 | 55 | 65 | 1.50x | 90 | 142 Hz |
| Ambient - Ethereal | 9.10 | +2.3 | +10 | 50 | 70 | 1.30x | 85 | off |
| Ambient - Frozen Lake | 12.60 | +0.7 | -55 | 50 | 60 | 1.80x | 100 | 140 Hz |
| Ambient - Glass Haze | 7.00 | +0.4 | +50 | 40 | 80 | 1.00x | 70 | 160 Hz |
| Ambient - Infinite Drone | 14.00 | +1.8 | -30 | 55 | 55 | 2.00x | 100 | 100 Hz |
| Ambient - Pad Wash | 8.40 | +1.0 | -20 | 45 | 80 | 1.20x | 75 | 119 Hz |
| Ambient - Send | 7.00 | +4.7 | +0 | 100 | 0 | 1.00x | 60 | 120 Hz |
| Ambient - Soft Halo | 4.90 | +0.5 | -10 | 35 | 90 | 0.70x | 55 | 100 Hz |

What changed in kind, not only in number:
- **Tail lengths follow the names.** v1.14.0's Concert Hall would be 3.5 s on the new Hall and its
  Vocal Booth 0.30 s; they are now 2.0 s and 0.24 s. Cathedral is 5.4 s, Theater 1.5 s, Amp Spring 2.0 s,
  Studio Plate 2.0 s, Snare Plate 1.25 s.
- **SIZE is spread for what it now does.** Plate presets run SIZE 10–90 because its range is one octave;
  Spring's SIZE is the echo time (25–44 ms), so Twang is short and Dub Spring long.
- **Infinite Drone, Ethereal and Cloud Nine** carry round numbers again (55/55, 50/70, 55/65). v1.14.0 had
  scaled them down to hold +4.5 dB under a tank that ran hot at long DECAY; the level law does that now.
- Inserts span +0.2..+2.9 dB; Sends +4.7..+6.0 dB wet-only.
- The loudness reading is 4 s after 1 s of settling. Ambient at 14 s has not finished building by then, so
  the three longest Ambient presets read a little low (about 0.5 dB by the arithmetic of the build-up; not
  rendered longer).

### Task 12 — tooltips

Three bodies rewritten in each language; titles, keys, bindings, `index.html` and CSS untouched.

| Gate | Result |
|---|---|
| `scripts/check-i18n.js` | ALL CHECKS PASS; 3 fr entries now unreviewed (these three) |
| `scripts/i18n-fr-lint.js` | O-SimpleReverb 0 findings (the script exits 2 on O-DigiDelay's own finding, not this plugin's) |
| `scripts/i18n-zh-lint.js` | 0 findings, exit 0; 3 entries at `'mt'` |
| `tests/ui_tip_render_check.js` | ALL CHECKS PASSED, 251 assertions, en + fr + zh-Hans |
| `scripts/check-ui-labels.js --plugin O-SimpleReverb` | ALL CHECKS PASSED, unchanged |

Tip heights at 500 x 350, en / fr / zh-Hans: type 202.3 / **233.1** / 202.3 px, decay 110.0 / 125.3 / 110.0,
size 186.9 / 202.3 / 156.1. **The French `tip.type` is 9.1 px from the bottom edge** (it had 39 px). It is
inside the frame and gated; one more line of French would not be.

Back-translation: `i18n-zh-backtranslate.js --emit` (blinded ids, zh only) → a fresh Sonnet subagent that
was told to read that one file → `--ingest`. All three read back with the meaning intact; the triples are in
`I18N-REVIEW.md`. Not done: the flags stay `false` / `'mt'` until Taylor has read them.

### Task 13 — the finished gates on v1.14.0

`git archive 02bac73f` into the scratchpad, this stage's `tests/render-check` and `tests/fixtures` dropped
over it, built out of tree. 93 gates run there (section 11 and the NaN gate need the engine headers and the
test hooks). **51 PASS, 42 FAIL.**

| Gate | v1.14.0 | v2.0.0 |
|---|---|---|
| RT60 = base x DECAY, six types | FAIL x6 (worst +350 %; 2.0x reads 3.6–11.2 s) | PASS x6 (worst +2.0 %) |
| RT60 holds across SIZE, six types | FAIL x6 (worst −82 %) | PASS x6 (worst +2.8 %) |
| First arrival moves with SIZE, six types | FAIL x6 (0.00 ms) | PASS x6 (1.15–19.2 ms) |
| Early arrivals discrete and unshared, four room types | FAIL x4 (93–101 a side, about 60 unshared) | PASS x4 (4–5, all unshared) |
| Tail spectra of the room types unrelated | FAIL (0.96) | PASS (−0.06) |
| Late-tail L/R correlation | FAIL (+0.54, Booth) | PASS (−0.10) |
| Old tail rings out on a TYPE change | FAIL (−17.4 dB, 0.73 s) | PASS (−3.7 dB, 3.03 s) |
| No allocation in `processBlock` | FAIL (4) | PASS (0) |
| Oversized host block = prepared-size blocks | FAIL (differs by 0.08) | PASS (identical) |
| All 48 presets 64 ms apart | FAIL (−33.5 dB above 8 kHz) | PASS (−60.1 dB) |
| Plate shimmer blooms | FAIL (−11.6 → −12.1 dB: no rise) | PASS (−35.2 → −13.3 dB) |
| Plate at DECAY 2.0x ends | FAIL (−37.5 dB 5–6 s on) | PASS (−69.8 / −71.8 dB) |
| Spring chirp | FAIL (0.4 ms; echoes 48.9 ms apart, match 0.13) | PASS (9.7 ms; 33.0 ms, 0.81) |
| Spring band limit | FAIL (+5.8 dB above 6 kHz) | PASS (−30.8 dB) |
| Wet level across DECAY, six types | FAIL x6 (9.5–10.5 dB) | PASS x6 (1.9–2.6 dB) |
| Wet level across SIZE, six types | FAIL x4, PASS x2 (Booth 0.30, Spring 0.66 dB) | PASS x6 (0.16–0.57 dB) |
| `params.tsv` identity | same file | byte-identical |
| v1.14.0 state blob loads | PASS | PASS |
| Sections 1, 2, 4, the other section 5 cases, 7's shifter, 9, 10 | PASS | PASS |
| Type level spread, stereo / mono | PASS (0.12 / 0.98 dB) | PASS (0.07 / 0.07 dB) |

Read with care:
- **The chirp gate fails on v1.14.0 on every condition, not only echo spacing** — stage 3 asked for this to
  be checked. Freeverb's combs gave a best period of 48.9 ms at a match of 0.13.
- **Two v1.14.0 types pass the SIZE level gate.** Its SIZE moved Booth and Spring by under 0.7 dB anyway.
  That gate is shown to work by the `noLevelLaw` mutant, not by the negative control.
- v1.14.0's mono spread passes at 0.98 against a limit of 1.
- The rapid-TYPE and SIZE/DECAY glide gates pass on v1.14.0 as they should: it did not click either.
- The negative control rewrote the installed factory bank with v1.14.0's values while it ran. The 2.0.0
  gates and then the 2.0.0 plugin ran after it, and each rewrites the bank from its own table.

### Mutants added

| Mutant | Gate | Reading on the broken build |
|---|---|---|
| input gain ignores SIZE | wet level across SIZE (Room / Spring / Plate) | 4.69 / 2.19 / 2.09 dB across the knob (limit 1) |
| input gain ignores DECAY | wet level across DECAY (Room / Spring / Plate) | 4.45 / 4.78 / 4.12 dB (limit 3) |

### Departures from PLAN.md

1. **SIZE has a level law as well as DECAY**, and the two are not the same strength (above).
2. **The level law is in dB per octave of the control, not `1/sqrt(T60)`.** `1/sqrt` is 3 dB per octave;
   the engines measure 2.2, and half of that is 1.1.
3. **The SIZE level gate reads the spread over five sizes**, not the worst departure from SIZE 50. On the
   first draft the plate's mutant failed by 1.10 against a limit of 1.0; by spread it is 2.09.
4. **`monoTrimDb` per type** — the plan asked for the mono spread to be measured; it was fixable in one
   field.
5. **`tests/ui_tip_render_check.js` sweeps zh-Hans too.** The plan's "in all three languages" was not
   something the gate did: it drove English and French only.
6. **The French and Chinese bodies are new text, not edits**, so the fr flags went from `true` to `false`.
7. **`tip.character` and `tip.lowCutOn` still say the filter is bypassed** when neutral / off. That has not
   been true since v1.12.0 (both filters always run). Out of this milestone's scope; not touched.
8. **`pend()` is unused** now that nothing is pending; it is kept (`[[maybe_unused]]`) for the next stage
   that needs it.
9. **`inject-context.py` was not run** (see stages 0–3).

### Still placeholders — the listening list

One constant each; none is closed by a gate.

| What | Constant | Now |
|---|---|---|
| DECAY's share of the level law | `kDecayLevelDbPerOctave` | 1.1 dB per octave (half) |
| Early-reflection levels and spans, pre-delays, type EQs | `typePresets` | v1.14.0's |
| Plate shimmer amount / damping / SIZE range | `PlateEngine::kShimmerPerLoopSecond`, `kDampingHz`, `typePresets[4]` | 0.121 / 10 kHz / x0.40–0.80 |
| Plate echo density at SIZE 50 and up | `typePresets[4].sizeHi`, or more input diffusion | 0.77 / 0.70 against a target of 0.80 |
| Spring chirp length, band limit, wobble | `SpringEngine::kSpring`, `kAllpassCoefficient`, `kWobbleMs` | stage 3's table |
| The 48 presets | `bank[]` | chosen by decay time and level, not by ear |

### Build and install

`./scripts/build-and-install.sh O-SimpleReverb`: VST3 + AU, 0 compiler warnings (0 in the test build too).

| Check | Result |
|---|---|
| Installed bundles | `O-SimpleReverb-dev.vst3`, `O-SimpleReverb-dev.component` — both report 2.0.0; no unsuffixed variant on disk |
| pluginval, strictness 10, GUI tests skipped, in process (VST3) | SUCCESS |
| `auval -v aufx OuSr OuDv` | AU VALIDATION SUCCEEDED |
| Installed factory bank | `.factory-version` 2.0.0, 48 files, new values (Concert Hall: SIZE 60, WET 30, DECAY 0.67x) |

Not built: the Standalone. `build-and-install.sh` does not rebuild it, so
`build/plugins/O-SimpleReverb/.../Standalone` is stale; rebuild it (`ninja -C build O-SimpleReverb_Standalone`)
before using it for the listening pass.

No tag, no push. This stage is a fifth path-scoped `wip(O-SimpleReverb)` commit (the plugin's directory and
its one `PLUGINS.md` row); whether to squash the five is Taylor's call at verify.

### For verify

- **Listening pass** on the installed build, with the list above. A change to any of those constants means
  re-running render-check (and `--levels` if it is a level or trim), not a new stage.
- **Read `I18N-REVIEW.md`**, then flip the three fr flags to `true` and the three zh-Hans flags to `'bt'`.
- **Row-by-row comparison** of `BASELINE.md` against `BASELINE-v2.0.0.md` into VERIFICATION.md. Their CPU
  tables are from different runs of the same machine.
- **Logic caches an AU's I/O per version**; this is a version bump, so nothing to clear by hand. If a DAW
  shows v1.14.0 behaviour, quit it fully and reopen.
- `.planning/STATUS.md` is still the stale January file plus `activeMilestone`; clearing that field is the
  post-verify step.
