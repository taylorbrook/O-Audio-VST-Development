---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.2 Execution Plan (static engine, placement, transparency)"
created: 2026-11-13
juce_version: "8.0.15"
phase: "2.2"
summary: "Six waves, serial: (W0) pre-flight and baselines; (W1) the engine as std-only headers in Source/dsp (BandParams, SectionEngine, Band, Router) and its double-precision rows in the designer harness; (W2) the processor wired to the engine, version 0.2.0; (W3) render-harness rows (two Stage 1 rows amended, the Phase 2.2 rows added); (W4) build, both harnesses, negative controls, install, pluginval 10, auval; (W5) summary, STATUS, one path-scoped commit. No ramps, no crossfades, no sleep, no output stage. No tag."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-ParametricEQ — Stage 2 / Phase 2.2 Execution Plan (static engine, placement, transparency)

## Goal

Audio passes through up to 24 bands with fixed parameters. The engine realises the designer's closed form, each placement touches only its own component, untouched audio stays bit-identical, latency is 0, and `processBlock` never allocates. Version 0.2.0 is built, installed and validated.

Proved at two levels (RESEARCH-2.2 Q7): the engine in double by the designer harness (tight thresholds, no float confound), and the plugin's float output by the render harness.

**In scope:** `eq::SvfSection` / `eq::OnePole` (settled path), `eq::Band` with one slot, `eq::Router`, the shared raw-value → settings function, working buffers, fast path, hard global bypass, tail 2.0 s.

**Out of scope (and what 2.2 does instead, call C6):**

| Later phase | 2.2 behaviour |
|---|---|
| 2.3 ramps, control grid | A parameter change redesigns the band at the next block start; state kept. Audible step. |
| 2.4 crossfades | A structural change (type, slope, placement, activation) clears the band's state. Click. |
| 2.4 neutral sleep | A 0 dB band is an ordinary active band. G4 (d), (e) are not claimed. |
| 2.4 soft bypass | Global `bypass` is a hard switch: buffer untouched, engine state cleared. G4 (f) is not claimed. |
| 2.5 output stage | `output_gain` and `auto_gain` are read by nothing. |
| 2.6 | No taps, no display model, no listen. |

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/RESEARCH-2.2.md` | All of it. Q6 is the processor recipe; Q7 the row list; "Pitfalls carried". |
| `stages/2-dsp/research-2.2/engine_bench.cpp` | Working prototype: `toRt`, the recurrences, `modeIr` (single-bin DFT, tail rule), `modeMs`, `modeG8`, `modeBlk`, `modeCpu` (sample-major loop). Port from here. |
| `stages/2-dsp/CONTEXT.md` | D6, D9, D10, "Constraints carried". Binding. |
| `research/ARCHITECTURE.md` | § 3, 4, 5; "Processing Chain"; "SVF section and one-pole (realisation)"; "Float I/O, double engine"; gates G4, G6, G8 (as amended by this plan phase). |
| `Source/dsp/EqTypes.h`, `Designer.h`, `ResponseEval.h` | The interface the engine consumes (`BandSettings` → `BandDesign`), `bandMagDb`. |
| `Source/PluginProcessor.h/.cpp` | The Stage 1 shell this phase fills. |
| `tests/render-harness/main.cpp` | Row / tally conventions; `render`, `makeNoise`, `sameAudio`, `setRaw`, `rawOf`; rows P1 and L4 (amended here). |
| `tests/designer-harness/main.cpp` | Row conventions; A1 (`malloc_logger` window, lines ~2551–2670) is the allocation recipe to reuse. |
| Memory `index_dsp_gate_design.md`, `index_testing_render_harness_patterns.md` | Suite gate recipes (allocation gate scope and liveness probe, vacuity, click confounds). |

---

## Plan-phase decisions

User calls taken in this phase (2026-11-13), contracts amended and checksums refreshed:

- **U1 G4 (c)** as research C1: double engine ≤ -250 dB re input peak; float output ≤ -130 dB re the larger of input and output peak.
- **U2 G8** as research C2: latency 0; nothing before the impulse; predicted first-sample gain matched within 1e-6 relative where it is ≥ 1e-30; the double engine starts at the impulse index always.
- **U3 Version 0.2.0**, CHANGELOG entry, dev build installed, pluginval 10 and `auval` on the real engine.

The plan's calls:

- **P1 Files.** Four new std-only headers in `Source/dsp`, same rules as the designer (C++17, `noexcept`, no allocation, no framework, the T8.8 grep tokens absent even from comments):

  | File | Contents |
  |---|---|
  | `BandParams.h` | `RawBand` (the eight raw floats in registration order), `Placement`, `BandInput { BandSettings settings; Placement placement; bool active; }`, `makeBandInput (raw, gainScaleRaw, dynOffsetDb)`. The **one** function from raw parameter values to designer input: the processor, both harnesses and the 2.6 display model call it. |
  | `SectionEngine.h` | Runtime coefficients (`a1, a2, a3, k, mL, mB, mH`; `Gc, cL, cH`) converted once per design; the SVF and one-pole recurrences exactly as ARCHITECTURE writes them. |
  | `Band.h` | `eq::Band`: one slot, runtime sections, state for two lanes, `dynOffsetDb` (double, 0), `setDesign` (state kept), `reset` (state cleared), a sample-major lane loop. |
  | `Router.h` | Applies one band to the lanes by placement; difference injection for Mid / Side. |

  Working lanes belong to the processor and are passed in as pointers. No CMake change is needed for headers.
- **P2 `makeBandInput`, exactly.**
  1. Non-finite raw value → that parameter's default (type 0, freq 1000, gain 0, Q 1, slope index 1, placement 0, scale 100; toggles off).
  2. Clamp: freq 10–30 000 Hz, gain ±30 dB, Q 0.025–40, scale 0–200. Choices: round to nearest, clamp to the index range. Toggles: `raw >= 0.5`.
  3. `gEff = clamp (gain · scale / 100 + dynOffsetDb, −60, +60)`, in double. At scale 100 and offset 0 this is the float gain exactly [BENCH `scale`].
  4. `active = enabled && ! bypass`.
  5. Canonical form: `slopeIndex = 0` for Bell, Notch, Band Pass (D6); `q = 1.0` for a slope type at 6 dB/oct (D9, Q inert). The designer reads neither, so the design is unchanged; the canonical form is what makes those edits cost nothing.
  No test for an exact frequency or Q anywhere (skewed defaults are inexact).
- **P3 Change detection, two levels.** Per band, the eight raw floats plus raw `gain_scale` are compared with the last snapshot by bit pattern (`memcmp`). Only on a difference is `makeBandInput` run and its result compared, field by field, with the band's current canonical input:
  - identical → nothing (this is D6 / D9: no design, no state change);
  - `active`, type, `slopeIndex` or effective placement differs → **structural**: design, state cleared;
  - otherwise → **continuous**: design, state kept.
  A band that leaves the active list has its state cleared. Effective placement is Stereo on a mono layout, so a placement edit on mono is "identical".
- **P4 Lanes and conversion.** A lane is converted to double and written back only if an active band touches it this block (Stereo / Mid / Side: both; Left / Right: one). No active band: the buffer is not touched and no parameter beyond the snapshot compare is acted on.
- **P5 Inner loop (C5).** Sample-major per band and lane: every section of the band inside one sample loop. Lane-pair and SIMD stay deferred. Mid / Side in the form `d = F(m) − m; L += d; R ± d`; a scratch lane is optional (the sample-major loop has `m` and `F(m)` in hand) and, if used, is sized in `prepareToPlay`.
- **P6 Oversized host blocks.** Processing runs in chunks of at most the prepared block size. Only the sample range is split: parameters are read and designs made once per host block.
- **P7 Hard bypass.** `bypass` on, or `processBlockBypassed`: buffer untouched, every band's state cleared, snapshots left valid. The Stage 1 "clear outputs beyond the inputs" line stays first.
- **P8 `prepareToPlay`.** `designer.prepare (fs)` first; stereo flag from the negotiated output set; lanes sized to `samplesPerBlock`; all state cleared; every snapshot invalidated (first block designs every active band); `setLatencySamples (0)`. Safe to call repeatedly and at a new rate.
- **P9 Tail (C4).** `getTailLengthSeconds()` returns 2.0; render-harness row L4 changes in the same commit.
- **P10 Two Stage 1 rows are amended, by name.** CONTEXT says the 36 Stage 1 rows stay green; two of them assert shell behaviour that the engine ends:
  - **L4** "tail 0" → "tail 2.0".
  - **P1 `processBlock`, variant "all random"** → "all random, every `bNN_enabled` off, `bypass` off" (stimulus guard unchanged: more than 100 non-default values). The `processBlockBypassed` rows keep the full random draw. `output_gain` and `auto_gain` stay in the draw until 2.5 ends that.
  The other 34 rows are untouched and must print as before.
- **P11 FUNC-02 row (C3).** Scored where the closed form is above -80 dB; 160 log-spaced single-bin DFT points, 20 Hz to min (20 kHz, 0.499 fs); the render is extended in 1 s steps until the last 10 ms is at least 140 dB below the peak (200 dB in the double row), cap 20 s, and the row **fails** if the cap is reached. The reference is designed from `rawOf` values through `makeBandInput`. Eight binding cases, at 44.1 and 96 kHz in float and at all five rates in double:

  | Type | Setting |
  |---|---|
  | Bell | 1 kHz, +12 dB, Q 1 |
  | Low Shelf | 200 Hz, +9 dB, 24 dB/oct, Q 1 |
  | High Shelf | 8 kHz, −12 dB, 12 dB/oct, Q 1 |
  | Low Cut | 80 Hz, 48 dB/oct, Q 1 |
  | High Cut | 6 kHz, 96 dB/oct, Q 1 (the worst float case: 5.8e-4 dB) |
  | Notch | 1 kHz, Q 10 |
  | Band Pass | 500 Hz, Q 2 |
  | Tilt | 1 kHz, +6 dB, 12 dB/oct |

  The 10 Hz / Q 40 extremes belong to G7 and G9, not here.
- **P12 No float overflow in a 2.2 plugin row.** 24 stacked bands at ±30 dB and scale 200 % reach 1440 dB: finite in double, `inf` in float. Plugin rows therefore bound their gains (stated per row); the unbounded random draw runs only in the double engine row. Carried to 2.3 research: G5's "peak ≤ +72 dB re input" with "every parameter randomised" needs a statement about stacked gain.
- **P13 No audio golden.** Clang contracts `a * b + c` per build, so renders differ in the last bits across platforms. Every bit-identity row compares two renders of the same binary.
- **P14 Negative controls live inside the harness** (as P1n does): each is a second evaluation of the same row on a deliberately wrong configuration and prints as its own row `…n` that passes when the control fires. The designer harness gains one `--neg engine-alloc` (a planted allocation in the engine window), following `--neg alloc`.
- **P15 CPU is INFO in this phase.** Settled readings are printed; G10 binds in Stage 4.

---

## Tasks

Roles as in `PLAN-2.1.md`: **[agent]** = dsp-agent writes files (no Bash); **[run]** = the execute session. **BG** = `run_in_background`, log to `$S/<name>.log`; `$S` = the session scratchpad. Paths are relative to `plugins/O-ParametricEQ/` unless they start with `build/` or `scripts/`. `$H` = the designer harness binary, `$R` = the render harness binary (PLAN-2.1 T8).

### Wave 0 — pre-flight

**T1. Pre-flight and baselines** [run]
- Done when (any mismatch → STOP):
  - `shasum -a 256` of `parameter-spec.md`, `research/ARCHITECTURE.md`, `ROADMAP.md` equal STATUS `contract_checksums`.
  - `git branch --show-current` = `main`; `git status --short plugins/O-ParametricEQ` is empty. Save the full `git status --short` to `$S/git-status-T1.txt`.
  - (BG) `ninja -C build O-ParametricEQ-designer-harness O-ParametricEQ-render-harness` → rc 0. `$H > $S/designer-base.log` → **54 PASS, 0 FAIL, 12 INFO**. `$R > $S/render-base.log` → **36 PASS, 0 FAIL**. `python3 tests/golden/check_spec.py` → `OK:`.

### Wave 1 — the engine, in double

**T2. `Source/dsp/BandParams.h`** [agent]
- P1, P2. File header in the style of `EqTypes.h` (licence block, what the file is, who calls it).
- Depends on: T1.

**T3. `Source/dsp/SectionEngine.h`, `Band.h`, `Router.h`** [agent]
- P1, P5. Recurrences as ARCHITECTURE "SVF section and one-pole (realisation)"; a section runs only the halves its `hasSvf` / `hasOnePole` flags name; at most `maxSections`.
- `Band::setDesign` converts and keeps state; `Band::reset` zeroes both lanes' state. `Band::dynOffsetDb` is a public double initialised to 0 with a comment naming DSP-07; nothing in this phase writes it.
- Router: Stereo, Left, Right, Mid, Side per the Processing Chain; on a mono layout every placement runs lane 0. It never reads or writes a lane the placement does not touch.
- Keep a Stereo band's two lanes adjacent in the state layout (ARCHITECTURE "SIMD: deferred").
- Depends on: T2.

**T4. `tests/designer-harness/main.cpp`: engine rows** [agent]
- A new heading "2.2 / engine (double)". All rows drive `eq::Band` + `eq::Router` from `Designer::design`, no float anywhere unless stated.

  | Row | Check | Threshold |
  |---|---|---|
  | EN-ir | P11's eight cases at 44.1, 48, 88.2, 96, 192 kHz: impulse response vs `eq::bandMagDb` | ≤ 1e-6 dB above -80 dB; tail ≥ 200 dB down, asserted |
  | EN-irn | EN-ir's bell against the closed form of the same bell 1 % higher in frequency | must exceed 1e-6 dB (control fires) |
  | EN-ms | Stereo noise -12 dBFS, 48 kHz: 1 Mid bell ±30 dB, 24 Mid bells +6 and +12 dB; the same on Side. Untouched component vs the input's | ≤ -250 dB re input peak; touched component changes by more than -40 dB (vacuity) |
  | EN-msn | EN-ms with the band on Left | must fail |
  | EN-lr | Left bands only: right lane; and the converse | `memcmp` equal; the touched lane differs |
  | EN-g8 | 10 000 seeded random configurations (1–24 bands, all types, slopes, placements, full ranges, 44.1–192 kHz) plus the three forced cases of RESEARCH Q3 (High Cut 10 Hz 96 dB/oct at 44.1 and 192 kHz; 100 Hz at 192 kHz): unit impulse | first non-zero output at the impulse index in every one; `y[0]` equals the predicted first-sample gain within 1e-12 relative |
  | EN-blk | A 96 dB/oct High Shelf into a 36 dB/oct Low Cut, and one 24-band mixed-placement setting, at block sizes 1, 7, 16, 32, 64, 512, 2048, 4096, one block | bit-identical |
  | EN-fin | (i) every type at 20–30 kHz, 44.1 kHz; (ii) random full-range settings re-drawn every 64 samples at 22.05, 32, 44.1, 48, 88.2, 96, 176.4, 192 kHz, 10 s each, noise and silence segments, `Designer::prepare` per rate | every sample finite |
  | EN-alloc | 1e4 blocks of `setDesign` + Router processing inside the `malloc_logger` window on this thread | 0 allocations, 0 frees; SKIPPED line where the platform has no logger |
  | SN1 | `makeBandInput`: NaN, ±inf, out-of-range and off-grid values per field; toggles at 0.49 / 0.5; scale 0, 100, 200; clamp at ±60 | each result as P2 states, exact |
  | SN2 | 2 000 000 random float gains at scale 100, offset 0 | `gainDbEff` equals the float gain bit for bit |
  | SN3 | Canonical form: slope edits on Bell / Notch / Band Pass and Q edits at 6 dB/oct | `BandInput` field-identical |
  | EN-dyn | `dynOffsetDb` +6 on a 0 dB bell vs a bell at +6 dB: `gainDbEff`, and the rendered output | equal bit for bit; offset 0 leaves `gainDbEff` unchanged (DSP-07) |

- `--neg engine-alloc`: a planted allocation inside the EN-alloc window is counted; exit 0 only if it is.
- The 54 existing rows are not edited.
- Depends on: T3.

### Wave 2 — the processor

**T5. `Source/PluginProcessor.h/.cpp`: engine wired** [agent]
- Members: `eq::Designer`, 24 `eq::Band`, per-band raw snapshot + canonical `BandInput` + valid flag, the 196 raw-value pointers cached in the constructor (`getRawParameterValue`), working lanes, stereo flag, prepared block size.
- One test accessor: `double getBandDynOffsetDb (int band) const noexcept` (row V1). No other engine internals are exposed.
- `prepareToPlay` per P8. `processBlock` per RESEARCH Q6 "Per block" with P3, P4, P6, P7; `ScopedNoDenormals` first. `processBlockBypassed` per P7. `getTailLengthSeconds()` → 2.0.
- No lock, no allocation, no listener, no `AsyncUpdater`; none of `juce::dsp::IIR`, `StateVariableTPTFilter`, `juce::dsp::Gain`. Nothing sits between the float input and the float output except the bands.
- The header comment and the "Stage 1 audio path" comments are brought up to date (what 2.2 does, what 2.3–2.5 will replace).
- Depends on: T3.

**T6. Version 0.2.0** [agent]
- `CMakeLists.txt`: `OPEQ_VERSION 0.2.0`. Nothing else in the file changes.
- `CHANGELOG.md`: `## [0.2.0] - <execute date>` (bracket form): Added: the 24-band engine, eight types, 6–96 dB/oct, five placements, zero latency; bit-exact with no active band; 2 s tail. Known in this build: parameter moves step, type / slope / placement / enable changes click, global bypass is a hard switch, output gain and auto gain do nothing yet.
- Depends on: T5.

### Wave 3 — render-harness rows

**T7. `tests/render-harness/main.cpp`** [agent]
- Amend L4 and P1 per P10. Add the rows below under new section headings. `render` gains what the rows need (rate argument, a per-block callback for mid-render edits); existing callers keep their behaviour.
- Stimulus "noise+tail": white noise at -12 dBFS followed by 2048 samples scaled into 1e-41 … 1e-39 (denormal floats). Assert in the row that the tail holds denormal non-zero values.
- Gains are bounded as stated (P12).

  | Row | Check | Threshold |
  |---|---|---|
  | E1 | FUNC-01. 24 bells, 40 Hz–16 kHz log-spaced, Q 2, alternating ±3 dB, Stereo, 48 kHz. (i) rendered response vs the sum of the 24 closed forms; (ii) for each band, the render with it disabled | (i) ≤ 0.01 dB above -80 dB; (ii) each of the 24 differs from the full render and matches its own 23-band closed form ≤ 0.01 dB |
  | E1n | The full render against a 23-band closed form | must fail |
  | E2 | FUNC-02, P11, at 44.1 and 96 kHz, mono and stereo | ≤ 0.01 dB above -80 dB; tail asserted |
  | E2n | E2's bell against the same bell 1 % higher | must fail |
  | N1 | DSP-06 at 44.1 kHz: each of the 8 types at 25 kHz and at 30 kHz (gain ±12 dB, 12 dB/oct, Q 1; Notch and Band Pass Q 0.5) | finite; ≤ 0.01 dB vs the designer's closed form above -80 dB (the frequency policy itself is the designer's, proved in 2.1) |
  | N2 | 24 bands, mixed types, 11–30 kHz, ±6 dB, rendered at 22.05 and 44.1 kHz, noise then silence, 5 s | every sample finite; peak ≤ +40 dB re input peak |
  | T1a | G4 (a). No band enabled, every other band parameter random, `output_gain` 0, noise+tail; block sizes 1, 16, 64, 512, 1024, 4096 and a seeded random sequence; mono and stereo | `memcmp` equal |
  | T1b | G4 (b). 6 Left bands (mixed types), noise+tail, same block sizes: right channel; and the converse | untouched channel `memcmp` equal; touched channel differs |
  | T1bn | T1b with the bands on Stereo | must fail |
  | T1c | G4 (c). 24 Mid bells +12 dB, and 1 Mid bell +30 dB: side `0.5 (L − R)` vs the input's; the converse on Side | ≤ -130 dB re max (input peak, output peak); touched component changes by more than -40 dB |
  | T1cn | T1c with the bands on Left | must fail |
  | T1g | G4 (g). One 24-band mixed setting (all types, slopes, placements; gains ±6 dB), block sizes 32, 64, 512, 2048, plus 1 and the random sequence | all bit-identical |
  | T1m | Mono layout: one band per placement value | output differs from input, and is `memcmp` equal to the Stereo-placement render |
  | Z1 | G8. 20 seeded random configurations (1–24 bands, all types / slopes / placements, gains ±12 dB) at 44.1, 96, 192 kHz, impulse at index 37, mono and stereo; plus the three forced High Cut cases | `getLatencySamples() == 0`; every sample before 37 is exactly 0; output at 37 equals the predicted first-sample gain (router algebra applied to the per-band scalars) within 1e-6 relative where it is ≥ 1e-30; forced cases print their first non-zero index as INFO |
  | A1 | G6 static. `malloc_logger` window on the audio thread around `processBlock` only: the first block after `prepareToPlay`, 200 settled blocks, and the block after a parameter change of each kind (continuous, structural, enable, bypass on / off); 24 bands at 96 dB/oct. Parameter writes happen outside the window | 0 allocations, 0 frees; a liveness probe shows the logger was armed |
  | A1n | A planted allocation inside the window | counted (control fires) |
  | K1 | Oversized block: prepare 256, feed 1024-sample blocks, 24-band setting | bit-identical to the same input fed as 256-sample blocks |
  | H1 | Hard bypass with 24 bands enabled: (i) `bypass` on via `processBlock`, and `processBlockBypassed`; (ii) after bypass off, the output from that sample on | (i) `memcmp` equal to input; (ii) `memcmp` equal to a fresh instance fed the same remaining input (state was cleared) |
  | U1 | A gain edit between blocks on a 1 kHz bell, 1 kHz sine: steady-state gain 1 s later | equals the new closed form within 0.01 dB |
  | U2 | Band disabled for one block, then enabled: output from the re-enable on | `memcmp` equal to a fresh instance fed the same remaining input |
  | U3 | Inert edits mid-render: slope on a Bell / Notch / Band Pass; Q on a 6 dB/oct cut; placement on a mono layout | output bit-identical to the render without the edit; control: a slope edit on a Low Cut changes the output |
  | V1 | DSP-07 engine half: `Band::dynOffsetDb` exists, reads 0 for all 24 bands on a fresh instance and after a 1 s render (`getBandDynOffsetDb`) | exactly 0.0; the binding arithmetic row is EN-dyn |
  | X-cpu | INFO. 48 kHz stereo, 512-sample blocks, 20 s: 0 bands; 24 cuts × 96 dB/oct; 24 shelves × 96 dB/oct; 24 bells; 1 band | printed as % of one core beside the bench's 2.4 / 4.7 / 0.70 and the budgets 10 / 14; not gated |

- Depends on: T5.

### Wave 4 — build, gates, install, validators

**T8. Build and run** [run]
- Done when, in order:
  1. (BG) `ninja -C build O-ParametricEQ-designer-harness O-ParametricEQ-render-harness > $S/build.log 2>&1` → rc 0; `grep "O-ParametricEQ/.*warning:" $S/build.log` → no lines. A configure is needed only for the version bump: (BG) `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` first.
  2. (BG) `$H > $S/designer.log 2>&1` → **0 FAIL**. The 54 / 12 rows of `$S/designer-base.log` print the same figures; every T4 row ID appears.
  3. `$H --neg engine-alloc` → rc 0. The seven existing `--neg` controls → rc 0 each.
  4. (BG) `$R > $S/render.log 2>&1` → **0 FAIL**. The 34 untouched Stage 1 rows print as in `$S/render-base.log`; L4 and P1 print their amended names; every T7 row ID appears; every `…n` row reports that its control fired.
  5. `python3 tests/golden/check_spec.py` → `OK:`; `$R --dump-params | diff - tests/golden/params.tsv` → empty (layout frozen).
  6. Static gates, from the repo root:
     - `! grep -rnE "juce|Juce|std::vector|std::function|new |malloc|throw " plugins/O-ParametricEQ/Source/dsp`
     - `! grep -rnE "fast-math|Ofast|fp:fast|ffp-contract" plugins/O-ParametricEQ/CMakeLists.txt`
     - `! grep -rnE "IIR::|StateVariableTPT|dsp::Gain|AsyncUpdater|callAsync|CriticalSection|std::mutex|addParameterListener" plugins/O-ParametricEQ/Source/PluginProcessor.h plugins/O-ParametricEQ/Source/PluginProcessor.cpp`
     - `grep -rn "dynOffsetDb" plugins/O-ParametricEQ/Source` → the declaration and its read(s) only; no write outside the initialiser.
     - `grep -rn "makeBandInput" plugins/O-ParametricEQ/Source plugins/O-ParametricEQ/tests` → one definition; the processor and both harnesses call it (no second raw → settings path).
- **On a failure:**
  - An EN row red → the engine. Fix in `Source/dsp`, rebuild, rerun 2–6 in full.
  - EN rows green and a render row red → the processor glue or the row's float scope. Fix the glue. A threshold, case list or stimulus level is not changed in this session; if a row cannot pass as specified with the EN rows green, STOP and escalate with the figures.
  - A control that does not fire → the row is vacuous. Fix the row, not the control.
  - A Stage 1 row outside L4 / P1 changes → STOP.
- Depends on: T4, T6, T7.

**T9. Install and validate** [run]
- One validator at a time, each BG with its own log.
  1. `./scripts/build-and-install.sh O-ParametricEQ > $S/install.log 2>&1` → rc 0; exactly the two `-dev` bundles in the system folders.
  2. pluginval, strictness 10, `--timeout-ms 600000`, VST3: **2 runs** → `SUCCESS`. AU: 1 run → `SUCCESS`.
  3. `auval -v aufx OuPQ OuDv` → `AU VALIDATION SUCCEEDED`; channel capabilities still `[1, 1] [2, 2]`. Never `auval -a`.
  4. Standalone smoke as Stage 1 T7 #9 (rebuild the Standalone target first) → `ALIVE`.
- Any failure: fix the root cause in `Source/`, then rerun T8 and T9 in full. A pluginval NaN or `inf` report is a finding about P12 as well as about the engine: record the configuration.
- Depends on: T8.

### Wave 5 — bookkeeping + commit

**T10. Summary, STATUS, commit** [agent writes the summary; run does the rest]
- `.planning/stages/2-dsp/SUMMARY-2.2.md`: tallies of both harnesses; EN-ir and E2 worst error per type and rate; EN-ms / T1c figures; Z1 forced-case indices; A1 counts; X-cpu readings beside the bench's; controls; validator results; the two amended Stage 1 rows; deviations.
- Carry-forward in the summary: for 2.3: G5's peak bound against stacked gain (P12), the step and click behaviours listed under "Out of scope", ramp-path shelf cost not timed; for 2.4: G4 (d), (e), (f); for 2.5: P1's random draw includes `output_gain` and `auto_gain`; for verify: hands-on DAW listen (the Stage 1 smoke items are still open).
- `REQUIREMENTS.md`: no status edit (requirements close at verify).
- `PLUGINS.md`: version cell → 0.2.0. Check `git diff PLUGINS.md` first: other sessions' rows may be modified in the tree. If so, leave PLUGINS.md out of this commit (or use the temp-index recipe, memory `index_git_shared_checkout.md`) and say so in the summary.
- STATUS: phase rows, `status: stage_2_phase_2_2_executed`, `next_action: stage_2_verify_phase_2_2`, files list.
- Commit, path-scoped, on `main`. Immediately before: `git branch --show-current`, `git status --short`. `git add` the new files by name (`Source/dsp/BandParams.h`, `SectionEngine.h`, `Band.h`, `Router.h`, `.planning/stages/2-dsp/SUMMARY-2.2.md`), then `git commit -- plugins/O-ParametricEQ`. Never `-A` / `-a`. `git log -1 --stat` lists only this plugin's paths. No tag.
- Depends on: T9.

---

## Dependency graph

```
T1 → T2 → T3 ─┬→ T4 ───────────────┐
              └→ T5 → T6 ──┐        │
                   └→ T7 ──┴────────┴→ T8 (build, harnesses, controls) → T9 (install, validators) → T10
                                        ↑   │
                                        └───┘ fix → rebuild → rerun T8 in full (and T9 if Source changed)
```

T4 and T5 / T7 touch different files and can be written in either order; one build covers all of them.

## Success criteria

| Criterion (ROADMAP Phase 2.2 as amended; ARCHITECTURE G4, G6, G8) | Proven by |
|---|---|
| 24 bands enabled together, each changes the output independently (FUNC-01) | E1, E1n |
| Each of the 8 types renders the designer's closed form (FUNC-02) | EN-ir (1e-6 dB, five rates), E2 (0.01 dB, float); EN-irn, E2n |
| G4 (a) no band enabled: bit-identical, denormal samples included (QUAL-01) | T1a; P1 (amended) |
| G4 (b) Left / Right leave the other channel bit-identical (FUNC-03) | EN-lr, T1b, T1bn |
| G4 (c) Mid / Side leave the other component (FUNC-03) | EN-ms (-250 dB), T1c (-130 dB re larger peak); EN-msn, T1cn |
| G4 (g) block-size invariance | EN-blk, T1g, K1 |
| Mono: every placement processes the channel; placement inert | T1m, U3 |
| G8 latency 0, no pre-ringing, first-sample gain (DSP-02) | EN-g8, Z1 |
| G6 with static parameters (PERF-01) | EN-alloc, A1, A1n, `--neg engine-alloc` |
| Above Nyquist at 44.1 kHz: finite, follows the policy (DSP-06) | N1, N2, EN-fin |
| `dynOffsetDb` hook, 0 in v1 (DSP-07 engine half) | EN-dyn, V1, T8.6 grep |
| One raw → settings path; sanitised | SN1–SN3, T8.6 grep |
| 2.2 state rules (hard bypass, re-enable, inert edits) | H1, U1, U2, U3 |
| Stage 1 layout, state and bus rows unchanged; layout golden unchanged | T8.4, T8.5 |
| 0.2.0 validates | T9 (pluginval 10 ×3, `auval`, Standalone) |

## Risks

- **R1 The bench is not the plugin.** Every float figure in RESEARCH-2.2 came from a prototype engine. A render row that misses with the EN rows green is a finding: STOP (T8), do not move a threshold.
- **R2 P1's amended variant stops covering "random settings pass through".** That property no longer exists. T1a replaces it with the stronger statement (denormal tail, seven block-size patterns).
- **R3 pluginval fuzzes all 196 parameters with 24 bands live and no smoothing.** Stacked gain can overflow float (P12). Expected to pass (pluginval checks its own output for NaN / `inf` only in some tests; a random draw rarely stacks 24 broadband boosts), but a failure here is real behaviour, not a harness artefact: record it and escalate rather than adding a limiter. Nothing may be inserted in the signal path to make it pass.
- **R4 The allocation gate in a JUCE console.** The logger must be scoped to the audio thread and armed only around `processBlock`; parameter writes notify listeners and may allocate legitimately. A1's liveness probe and A1n guard against a dead logger.
- **R5 `render` is shared with the Stage 1 rows.** Its new arguments must default to the old behaviour; T8.4 compares the 34 untouched rows against the baseline log.
- **R6 Run time.** E1 is 25 renders plus DFTs; EN-g8 is 10 000 configurations; EN-fin is 80 s of audio at full-range settings. Run both harnesses BG. Do not cut counts; thread EN-g8 / EN-fin as NW2 is threaded if the designer harness passes about ten minutes.
- **R7 Gate tokens in prose.** T8.6 greps `Source/dsp` comments too: no "new ", "throw ", "malloc", "juce" in the four headers' comments.
- **R8 Shared checkout.** Other sessions have uncommitted files in the tree (PLUGINS.md among them). Path-scope the commit and re-check immediately before it (T10).
