# Stage 2: DSP — Phase 2.4 (structural crossfades and bypass) - Verification

## Verification Date

2026-11-14 (verify session). Everything below was run against `75566f48`.

- **Independent re-run.**
  - The three harness objects were deleted and recompiled: the designer `main.cpp`, the render `main.cpp` and the render harness's `PluginProcessor.cpp`.
  - Both harnesses ran in default and `--long` form, with all ten negative controls.
  - The figures were compared with SUMMARY-2.4.
- **Code read.** A full read of `Fade.h`, `SlotBand.h` and the processor's `prepareToPlay` / `processAudio`, with `classifyBandChange` read for the request table.
- **Independent probe.** Written in this session (`verify-2.4/probe.cpp`, 57 gated rows + 11 INFO). It drives the real processor through its public API and uses nothing from `Source/dsp` or the harness code. The oracle is two plain processor instances (old settings, and new settings started at the edit sample) mixed with the raised-cosine weights.
- **Mutation run.** 56 mutants of `Fade.h`, `SlotBand.h` and the processor's bypass / fade path, run through both harnesses. The 21 that survived or were thinly gated were then run through the probe.
- **Validators.** pluginval strictness 10 (VST3, AU) and `auval` on the installed 0.4.0 bundles.

The probe, its build script, the mutation driver and their outputs are in `stages/2-dsp/verify-2.4/`. Nothing in the repo source or in `build/` was modified; mutants and the probe were compiled in the session scratchpad.

Scope: Phase 2.4 only. Stage 2 has two more phases (2.5, 2.6), so this file does not close the stage.

## Goal-Backward Analysis

### Original Goals (ROADMAP Phase 2.4 as amended 2026-11-14; PLAN-2.4)

1. Type, slope, placement, enable, band bypass and global bypass are click-free: G3 step rows as amended — second difference ≤ 2.0x the louder held render (both tones), local κ ≤ 2.5x at 1 kHz, hard control ≥ 10x with void rows named (DSP-04, FUNC-01, FUNC-04).
2. The engine does the specified fade: incoming slot from rest, raised cosine, `a + w (b − a)`, requests latched during a fade, latest wins, the bypass fade never reverses.
3. G2 live: slope changes on a running band (DSP-03).
4. G4 (d), (e), (f): 0 dB bells, gain scale 0 % and global bypass are bit-transparent within their times (QUAL-01, FUNC-04, FUNC-05).
5. A parameter changed while bypassed is in effect after un-bypass; `processBlockBypassed` takes the same path.
6. G5 re-run with structural parameters and bypass randomised; G6 with fades; block-size independence with structural events.
7. VERIFICATION-2.3 Issues 1 – 5 closed; existing rows unchanged outside P18; 0.4.0 builds, installs and validates.

### Deliverables (from SUMMARY-2.4.md, confirmed in the tree)

1. `Source/dsp/Fade.h` (FadeTable, BypassFade, lengths) and `Source/dsp/SlotBand.h` (two-slot band, latch, sleep, wake). The diff of `Source/dsp` against `07d044d8` is these two new files only.
2. `PluginProcessor.h/.cpp`: `std::array<eq::SlotBand, 24>`, one `processAudio (buffer, viaBypassed)` for both entry points, P8 steps 1 – 9.
3. Designer harness: 26 rows + 2 INFO, `--neg fade-alloc`. Render harness: 19 rows + X-cpu3. P18 amendments to existing rows.
4. `CMakeLists.txt` 0.4.0, `CHANGELOG.md` `## [0.4.0] - 2026-11-14`, `-dev` bundles installed.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 G3 step rows | ✅ Achieved (gate as specified); see Issue 1 | XF-g3 worst 1.525x (50 Hz) / 1.021x (1 kHz); XR-g3 1.522x; XF-g3k 2.20x, XR-g3k 2.197x; XF-g3h 482 / 482 ≥ 10x, XR-g3h weakest 17.92x; XR-g3b 1.334x / 1.648x. All equal to the summary. The probe agrees on its own step rows (C1, C3 – C6: 1.00 – 1.24x; hard-switch control ≥ 10.1x) **except C2**: a Low Cut slope change with the tone below the corner reads 3.89x (Issue 1). |
| 2 Specified fade | ✅ Achieved | The probe measures the output against `A + w (B − A)` built from two plain processor instances on type, slope, placement (Mid, Left), enable on / off and band bypass on / off at 44.1 / 48 / 96 / 192 kHz. Error 5.6e-8 – 8.2e-8 of the peak (limit 1e-6). The output is bit-equal to the new instance from exactly edit + N (20.000 ms), and linear weights miss by at least 2.2e-2 (control). Latch: a second edit at e+320 waits for e+N and then fades from rest (L1); three edits in one fade apply only the last (L2); B → C → B starts nothing more (L3); a gain edit mid-fade reaches the incoming slot at once (L5). Bypass: B1 – B4, B3n (the reversing envelope misses by 0.51). XF-ref / XF-refn / XR-ref / XF-rest / XR-rest as in the summary. |
| 3 G2 live | ✅ Achieved | XF-g2 336 cases, corner worst 0.0194 dB, slope worst 0.545 dB/oct; XR-g2 16 / 16. |
| 4 G4 (d), (e), (f) | ✅ Achieved | XF-g4: (d1) 85.83 – 88.18 ms, (e) 86.94 – 88.92 ms, (f) exactly N; XF-g4n fires 60 / 60; XR-g4d / e / f / n pass. Probe, seen from outside through the denormal flush: a 0 dB bell is untouched from sample 0 (S1); +6 → 0 dB and gain_scale → 0 are untouched from edit + 90.00 ms (S2, S3); 0.0001 dB never sleeps (S4); a wake reads 1.000x and lands on the static design (S6, S7). |
| 5 Changed while bypassed, one path | ✅ Achieved | XR-byp 4 / 4 by both routes, XR-bypn fires. Probe B6s / B6c / B6f / B6r: structural, continuous, during the fade to dry and with a ramp in flight at bypass, by both routes — bit-equal to a new instance from un-bypass + N. A session that opens bypassed is untouched from sample 0 (B5). |
| 6 G5, G6, block identity | ✅ Achieved | `--long`: XF-g5 600 s × 8 rates, 0 non-finite, 0 of 56 582 249 checks above stack + 12 dB, worst −8.4 dB; S5 16 / 16 (seed attempt 1 at 176.4 and 192 kHz, INFO as before). XF-alloc, A3 0 / 0, A3n and `--neg fade-alloc` fire. XF-blk / XR-blk bit-identical. Probe K1 – K3: events every 2048 samples at blocks 1 / 32 / 64 / 512 / 2048 at 48 and 192 kHz, and a host block of 4096 on prepared 512 / 100, all bit-identical. |
| 7 2.3 issues, existing rows, 0.4.0 | ✅ Achieved | XF-ms (Issue 1), XF-1p (Issue 2), XR-byp / XR-big / XR-prep (Issue 3), XF-rest / XR-rest (Issue 4), XF-g5 / S5 (Issue 5). Probe MS1 / MS2 read a difference of 0.0. `params.tsv` unchanged, `check_spec.py` OK. Installed bundles identical to the artefacts, version 0.4.0. pluginval 10 and `auval` pass. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.4 of 7)
**Requirements touched by this phase:** 8 (all must)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-03: slopes | must | ✅ Complete | Criterion 2 met: live slope changes (XF-g2, XR-g2; G3 slope rows). The stop-band burst (Issue 1) is recorded against the listening item, not against this criterion's gate. |
| DSP-04: click-free modulation | must | ✅ Complete | Criterion 2 met (G3 step rows as amended, and the probe's own step rows). Criterion 3 met with structural fades and bypass (XF-g5, S5, `--long`). Issue 1 stays open as a listening item. |
| FUNC-04: bypass | must | ✅ Complete | Criterion 2 met: band and global bypass fade (XF-g3 band-bypass rows, XR-g3b, XF-byp, probe B1 – B4); fully bypassed is bit-equal to the input. |
| FUNC-01: 24 on-demand bands | must | ⚠️ Partial | Criterion 2 met (enable / remove fades; probe F2e1 / F2e0, M1 / M2 with 24 bands). Criterion 3 (state survives save / reload and preset recall) waits for presets (Stage 4). |
| FUNC-05: output gain and gain scale | must | ⚠️ Partial | G4 (e) met through the plugin. Output gain is 2.5. |
| QUAL-01: transparency | must | ⚠️ Partial | G4 (d), (e), (f) met. Listening across factory presets is Stage 4. |
| PERF-01: real-time safety | must | ⚠️ Partial | G6 with fades met (XF-alloc, A3, `--neg fade-alloc`); read: no lock, no allocation, stack buffers only. G6 re-runs with the analyzer on in 2.6. |
| DSP-06: ranges | must | ⚠️ Partial | Unchanged: range ends are in every G5 draw; reachability from UI and typed entry is Stage 3. |

**Requirements Summary (all 27):**
- ✅ Complete: 9 (COMPAT-01, FUNC-03, FUNC-04, DSP-01, DSP-02, DSP-03, DSP-04, DSP-05, DSP-07)
- ⚠️ Partial: 6 (FUNC-01, FUNC-02, FUNC-05, DSP-06, PERF-01, QUAL-01)
- ⏸️ Pending / later phase: 12
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Contract checksums | ✅ Pass | BRIEF, parameter-spec, ARCHITECTURE and ROADMAP equal STATUS |
| Commit hygiene | ✅ Pass | `75566f48` lists this plugin's paths and `PLUGINS.md` only; plugin tree clean; no tag |
| Build (three objects deleted, recompiled) | ✅ Pass | rc 0; no warning from this plugin's files (the SDK's libc++ `availability.h` notice only) |
| Designer harness, default and `--long` | ✅ Pass | `114 PASS, 0 FAIL, 16 INFO` both times; every 2.4 figure read equals SUMMARY-2.4 |
| Render harness, default and `--long` | ✅ Pass | `100 PASS, 0 FAIL` both times |
| Negative controls | ✅ Pass | `rbj`, `f1`, `alloc`, `seed`, `rbj-b`, `binary-gate`, `first-build`, `engine-alloc`, `ramp-alloc`, `fade-alloc` rc 0; an unknown name rc 255 |
| Static gates (PLAN-2.4 T8.7) | ✅ Pass | The three greps print nothing; `Source/dsp` diff is `Fade.h` and `SlotBand.h` only; `dynOffsetDb` declaration and reads only; no complementary-form mix |
| Layout golden, spec | ✅ Pass | `--dump-params` equals `params.tsv`; `OK: 196 params` |
| Installed bundles | ✅ Pass | Exactly the two `-dev` bundles, 0.4.0, `diff -rq` identical to the build artefacts, newer than every source file |
| pluginval 10, `auval` | ✅ Pass | VST3 `SUCCESS`, AU `SUCCESS`; `AU VALIDATION SUCCEEDED`, `[1, 1] [2, 2]` |
| Independent probe | ⚠️ 56 / 57 | C2 fails (Issue 1); every other row passes (`verify-2.4/probe-output.txt`) |

### Mutation run

56 mutants plus an unmodified control (which reproduces 114 / 0 / 16 and 100 / 0). The designer harness does not compile `PluginProcessor.cpp`, so the processor mutants (PX) are gated by the render harness alone. Full table: `verify-2.4/mutation_results.txt`.

**Caught by both harnesses or by several rows:**
- **Fade table:** F1 linear weights, F2 10 ms, F3 40 ms, F4 / F5 / F7 off-by-one ends, F6 a 2 ms pre-roll.
- **BypassFade:** BF2 latched opposite dropped, BF4 idle never reached.
- **SlotBand:** SB1b warm start, SB2 outgoing cleared, SB3c mix reversed, SB4 / SB5 latch, SB8 zero test with a tolerance, SB12 lanes, SB18 clearState.
- **Processor:** PX1 hard bypass path, PX2 / PX3 un-bypass, PX5 / PX6 / PX6b write-back and mix order, PX7 re-prepare, PX10 `processBlockBypassed`, PX13 non-finite.

**Thinly gated (one row each):**
- XF-byp only: BF1 reversal.
- TK only: BF3, PX8, PX8b, PX11 (the clock and the jump).
- XF-lane only: SB12b.
- XF-cont only: SB14, SB15.
- XF-blk only: SB6b.
- XF-wake only: F9, SB6.
- One row in each harness: SB11b.
- The render harness sees none of SB6, SB6b, SB10, SB13, F9 or SB12b.

The probe also kills BF1 (B3, B4), SB6b (F1) and SB15 (L5).

**Survive both harnesses:**

| Mutant | Kind | Probe |
|---|---|---|
| **PX14** the step-7 restart (un-bypass latched during the fade to bypass) uses the realised input instead of the latest request: every band restarts off and the plugin stays dry until a parameter changes | **real** | **caught (B3)** |
| PX9 the segment not cut at the bypass fade's end: a latched opposite fade starts up to 63 samples late | real, small | not caught |
| BF5, SB3, SB3b complementary mix form | rounding only; the P2 rule is held by the static grep alone | not caught |
| SB9 sleep ignores ramp arrival on a 0 dB band | near-equivalent (output differs by rounding) | not caught |
| F8, SB1, SB16, PX2b, PX4, PX5b, PX12 | equivalent (reasons in the results file) | — |

The shipped code takes the correct branch in every surviving case (read: step 7 calls `startSettled (band.getDesired())`; the segment is cut at `samplesLeft`; every mix is `a + w (b − a)`).

## Human Verification

Not blocking. Carried to the hands-on listen:

- [ ] A 48 dB/oct or steeper Low Cut switched in under sustained bass (PLAN-2.4 U2).
- [ ] **A slope change on a steep Low Cut whose corner sits above sustained bass** (Issue 1): for example a 50 Hz bass line, a Low Cut at 60 – 120 Hz, slope 48 ↔ 96 dB/oct. Listen for a thump at the change.
- [ ] Whether 20 ms feels immediate on enable and on bypass.
- [ ] Several bands changing type at once, as in a preset change.
- [ ] The VERIFICATION-2.3 items: frequency, gain and Q drags; an extreme throw on a low tone.

Expected in this build and not defects: output gain and auto gain do nothing (2.5); no analyzer (2.6).

## Issues Found

None is a defect in the shipped code against its specification. Issue 1 is a behaviour finding that the gate does not cover. Issues 2 – 5 are gate gaps or notes for the 2.5 plan.

1. **Stop-band burst on a structural change of a steep cut (open; Taylor's call this session: pass, decide after the listen).**
   - **What happens.** The incoming slot starts from rest (C2 / U2, no pre-roll). When the tone sits below a cut's corner, the new filter's start-up ringing is louder than either settled output.
   - **Probe C2.** 48 kHz, block 512, 50 Hz tone at 0.5, Low Cut 60 Hz, Q 0.7, 48 → 96 dB/oct. It reads **3.89x** the louder held render, against the 2.0x limit. Held peaks are 0.091 / 0.024; the rendered peak after the edit is 0.107; the incoming-from-rest peak is 0.416.
   - **Not an engine error.** The blend error is 1.7e-8, the output is bit-equal to the new instance from e + N, and a hard switch reads about 1.2e5x.
   - **Corner sweep, 50 Hz tone, 48 → 96 dB/oct:**

     | Corner | Reading |
     |---|---|
     | 30 Hz | 1.25x |
     | 40 Hz | 1.17x |
     | 50 Hz | 1.70x |
     | 60 Hz | 3.89x (2.36x at block 32) |
     | 80 Hz | 29x |
     | 120 Hz | 259x |

     At 120 Hz both held outputs are near silence and the burst peaks at 0.046 (about −21 dB re the input). There the ratio to the held render degenerates, but the absolute level is audible.
   - **Slope pairs, 60 Hz corner:**

     | Change | Reading |
     |---|---|
     | 96 → 48 | 2.66x |
     | 24 → 48 | 1.95x |
     | 12 → 24 | 1.50x |
     | enable 96 dB/oct | 1.37x |
   - **Why the gate misses it.** The 242 G3 step rows never put the tone in a cut's stop band.
   - **How it differs from the research.** RESEARCH-2.4 C2 characterised start-from-rest as a level dip when a cut is switched in. This case is a burst on a slope change of a cut that is already removing the tone.
   - **Options, if the listen confirms it.** The pre-roll (`fadePrerollSeconds`, a zero-length option already wired through `FadeTable`; RESEARCH-2.4 measured 40 ms as bringing slope rows within 3.1x of a warm crossfade). Or a warm start for slope-only changes on a cut. Either way, add a stop-band G3 row.
2. **The un-bypass latched during the fade to bypass is gated by no harness row.** PX14 survives both harnesses; the probe's B3 catches it. When 2.5 touches the bypass path (its stages go in front of the bypass fade), add a render row: bypass on, un-bypass inside the 20 ms fade, compare against a fresh instance from the end of the fade back. PX9's timing of a latched opposite fade fits in the same row.
3. **The P2 mix form rests on the static grep.** BF5 / SB3 / SB3b are invisible to every row (ulp-level). That is acceptable: T8's grep is the gate and it is in the plan's static list.
4. **Multi-band structural changes settle in a chain.** After 24 bands change type at once, the output equals one fresh instance only after about 237 ms. Each band's incoming slot starts from rest on the crossfaded output of the band before it, so state propagates down the chain. The fade itself ends at 20 ms: against a series chain of 24 single-band instances, the probe reads 1.8e-7 / 3.5e-7 (M1 / M2). Any later row that compares a multi-band structural change with a single fresh instance must allow for it.
5. **Revert during a fade costs 40 ms (L4).** A → B → A within one fade runs the full fade to B, then a second fade back to A from rest. This is P4 as specified (the latch compares with what the band realises). Noted for the preset-change listen.
6. **CPU** (INFO; G10 binds in Stage 4). X-cpu3 shows 24 shelves × 96 dB/oct always fading at 11.2 %, and settled 24 cuts at 3.06 % (up from 2.74 %, the slot bookkeeping).
7. **`getBandDynOffsetDb` reads slot 0** whatever the current slot is. In v1 the offset is 0 on both slots; wiring a non-zero offset (v1.x) must read the current slot.

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 2.4)

**Ready for next phase:** Yes. Phase 2.5 (output stage and auto gain) has no research or plan yet; next is research for 2.5.

**Blockers (if any):** none. Carried forward, non-blocking:
- Issue 1, the stop-band burst: hands-on item; pre-roll or warm start if confirmed; a stop-band G3 row.
- Issue 2, a render row for the un-bypass latched during the fade to bypass (PX14, PX9), in 2.5's plan.
- Issue 4, the multi-band chain settle, for any multi-band compare row.
- The hands-on listen.
