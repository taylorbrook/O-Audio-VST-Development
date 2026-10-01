# Summary: Reverb engine rewrite

**Plugin:** O-SimpleReverb
**Milestone:** reverb-engine-rewrite
**Phase:** Execute (in progress — stages 0 and 1 of 0–4 complete)

Written stage by stage. Each stage adds a section; the final summary is assembled in stage 4.

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

