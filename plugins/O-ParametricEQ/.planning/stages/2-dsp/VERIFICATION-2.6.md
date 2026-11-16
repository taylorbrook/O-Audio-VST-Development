# Stage 2: DSP — Phase 2.6 (analyzer taps, display model, listen) - Verification

## Verification Date

2026-11-15 (verify session). Everything below was run against `52257b89`.

- **Independent re-run.**
  - The five harness objects were deleted and recompiled: designer `main.cpp`, render `main.cpp`, and the render harness's `PluginProcessor.cpp`, `SpectrumAnalyzer.cpp` and `DisplayModel.cpp`.
  - Both harnesses ran in default and `--long` form, and all 12 designer `--neg` controls ran.
- **Independent 0.5.0 dump for XR-lid (SUMMARY-2.6 deviation 8).**
  - A scratch single-plugin tree was built at `e9d6b560` (0.5.0 `Source/`, 0.5.0 harness), and the 0.6.0 harness's `--dump-sessions` block was spliced in (`verify-2.6/dump050-harness.patch`).
  - The helpers that block calls (`StressDraw24`, `Xorshift`, `makeNoiseAt`, `makeNoiseTail`, `handlesOf`, `keepsDenormals`, `makeBands`) were diffed between the two harness versions and are identical.
  - 16 sessions were written (`verify-2.6/sessions-050.sha`, `dump050-output.txt`). XR-lid ran against this dump, not the execute session's.
- **Code read.**
  - In full: `Listen.h`, `SpectrumTap.h`, `DisplayModel.cpp` and `SpectrumAnalyzer.cpp`.
  - The processor diff `e9d6b560..52257b89`: taps, step 4, seeding, `pathIdleForBlock`, the segment loop and steps 7 and 8.
- **Independent probe** (`verify-2.6/probe.cpp`, 12 gated rows + 1 INFO).
  - It drives the real processor through its public API only: parameters, `listenBand`, `analyzerMode`, the tap read side, `getListenState` and `SpectrumAnalyzer`.
  - Its oracles are restated from PLAN-2.6 P4 – P6, P10 and P11: R = round (0.030 fs), N = round (0.020 fs), the grid K, the tick clock, the solo router, and C3.
  - Rows N1, N1e, E1, X1 and B2 were added after the first mutation pass, aimed at its survivors.
- **Mutation run.** 69 mutants plus an unmodified control (`verify-2.6/mutation_run.py`, `results-all.txt`), covering:
  - `Listen.h` (33) and `SpectrumTap.h` (7);
  - the processor's listen / tap glue and `pathIdleForBlock` (22);
  - the display model (3) and the analyzer (4).

  Every mutant ran through both harnesses (the designer only where a header it includes changed) and the probe. The 20 survivors were re-run on the final probe (`results-survivors-probe2.txt`).
- **Side check of deviation 1.** XG-exact re-scored with a symmetric floor (`xg-symmetric-floor.patch`, `-output.txt`).
- **Validators.** pluginval strictness 10 (VST3, AU) and `auval` on the installed 0.6.0 bundles.

Nothing in the repo source or in `build/` was modified. Mutants, the probe and the 0.5.0 tree were compiled in the session scratchpad.

Scope: Phase 2.6 and the close of Stage 2.

## Goal-Backward Analysis

### Original Goals (ROADMAP Phase 2.6 as amended 2026-11-15; PLAN-2.6)

1. **G9 (a).** The closed form from the processor's packets equals the measured response on the damped circle, at five rates, for LL / RR / MM / SS. A unit-circle row confirms it; the packets equal the realised designs bit for bit; the four controls fire (UI-02 C++ half).
2. **Analyzer.** A −12 dBFS 1 kHz sine reads −12 ± 0.5 dB at 44.1 / 96 / 192 kHz. Overflow never blocks, and a prepare between drains reaches no transform.
3. **G6** with the analyzer on, the rings saturated and listen switching (PERF-01).
4. **Listen.**
   - On / off and band changes pass G3, engaging over a 30 ms ramp plus the 20 ms fade (C2).
   - Listen sits after auto gain (C3).
   - With listen off, the output is bit-identical to 0.5.0 (FUNC-08 engine half).
5. **The five rows owed by VERIFICATION-2.5:** PX17, PX11, PX4, AG3, OS18.
6. **Housekeeping.** Existing rows unchanged outside S5; 0.6.0 builds, installs and validates.

### Deliverables (from SUMMARY-2.6.md, confirmed in the tree)

1. **New std-only headers:** `Source/dsp/SpectrumTap.h` and `Source/dsp/Listen.h`.
   - The edits to the verified headers are additions only: `OutputStage.h` +43, `BandParams.h` +16, `ResponseEval.h` +49, `Band.h` +31.
   - `git diff … | grep "^-[^-]"` prints nothing for those and for `Fade.h`, `Designer.h`, `AutoGain.h` and `SlotBand.h`.
2. **Message-thread classes:** `Source/analysis/SpectrumAnalyzer.h/.cpp` and `DisplayModel.h/.cpp`. They poll, with no listener and no `AsyncUpdater`.
3. **Processor:**
   - taps on both entry points;
   - step 4 reads `listenBand`;
   - `seedListen()` at both restarts;
   - `pathIdleForBlock()`, one definition and two calls;
   - the split gain form when engaged;
   - `listen.goIdle()` at step 7 and `listen.clearState()` at step 8.
4. **Harness rows:** 20 designer rows plus `--neg tap-alloc`; 33 render rows plus the S5 amendment.
5. **Build and release files:** 0.6.0 in `CMakeLists.txt` and `CHANGELOG.md`; the `-dev` bundles installed.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 G9 (a) | ✅ Achieved | See below the table. |
| 2 Analyzer | ✅ Achieved | XR-an −12.039 / −12.081 dB at every rate; XR-answ −12.087 … −11.956; XR-ann (1x) −12.627, fires; XR-ovf, XR-gen, XT-ring, XT-thr, XT-gen pass. Probe A1: mono layout −12.04 dB, L-only stereo −18.06 dB (the 0.5 mono sum), L = −R at the −200 dB floor, +6 dB output reads post − pre 6.00 dB, at 48 / 96 / 192 kHz. Probe G1: mode bits gate both entry points exactly; the ring holds the oldest-fitting run of the mono sum, 32 768 samples bit-equal. |
| 3 G6 | ✅ Achieved | A5 0 / 0 in 222 834 windows (rings saturated, 927 232 drops per tap, 200 listen switches), A5n fires; XT-alloc 0 / 0; `--neg tap-alloc` fires. Read: no lock or allocation; the analyzer allocates only in its own `prepare` on the message thread. |
| 4 Listen | ✅ Achieved | See below the table. |
| 5 Owed rows | ✅ Achieved | XR-px17 −150.5 dB (control −23 dB); XR-px11 12 / 12; XR-px4 −150.5 dB (one refresh late −43 dB); XR-ag3 3.7e-5 dB (0.5 fs top fires); XO-os18 bit-exact, control 14 / 400. |
| 6 Existing rows, build | ✅ Achieved | Designer 150 / 0 / 18 and render 155 / 0, default and `--long` (S5 600 s per rate, 16 / 16, largest peak 252.5 dB, slowest return 220.6 ms). `params.tsv` unchanged; `check_spec.py` OK. Static gates clean. Installed bundles `diff -rq` identical to the artefacts, version 0.6.0. pluginval 10 VST3 and AU `SUCCESS`; `auval` `AU VALIDATION SUCCEEDED`, `[1, 1] [2, 2]`. |

**Goal 1 evidence (G9 (a)):**
- XR-g9 253 / 253 scored, worst 2.03e-3 dB (96 kHz stereo) against 0.05.
- XR-g9u 129 / 129, worst 1.61e-3.
- XR-g9n: all four controls ≥ 21.6 dB.
- XR-g9id 262 / 262, packets `memcmp` equal to `getBandRealisedDesign`.
- XR-g9j 1.37e-8 dB.
- XG-exact worst 2.06e-5 dB at ≤ 1e-4.
- Deviation 1 (the 646 / 4000 XG-exact voids) was checked with a symmetric floor and **holds**: see Issue 3.

**Goal 4 evidence (listen):**
- **G3:**
  - XL-on 1.213x, κ 2.38x;
  - XL-chg 1.350x, 0 / 3040 over 2.0x;
  - XR-lon / XR-lchg 1.333x, κ 2.347x;
  - the hard controls ≥ 10x.
- **Exactness and identity:**
  - XL-ref 0 and XR-lref −139.7 dB;
  - XR-lid 16 / 16 `memcmp` equal to the independently regenerated 0.5.0 dump, plain and with the analyzer drained on a second thread; XR-lidn fires;
  - XR-lag `memcmp` equal with auto gain on vs off.
- **Probe:**
  - T1: the state machine is sample-exact against the restated P6 at 48 / 96 / 192 kHz stereo and 44.1 kHz mono. It covers engage, a latched change, a latched release, a latched engage, an f edit (no move), a gain edit (nothing), a type-family change (change, same target) and a latched change.
  - O1: the output equals the input bit for bit until exactly tick + R, and again from exactly the release end, at 5 rate / block configurations.
  - I1: 448 / 448 `memcmp` equal: listen output is a function of the dry input and the listened band's f / Q / family / placement only. It is independent of the other 23 bands, of enable, bypass, gain, gain scale, slope and auto gain, and of random block sizes. The controls fire 56 / 56 and 21 / 21.
  - I2: the output gain scales listen.
  - E1: f / Q edits during a move reach the incoming voice.
  - X1: a latched un-bypass with listen engaged fades in.
  - B1 / B2: 36 / 36 bit-identical across block sizes, with bypass on both routes and both gains away from unity.

## Requirements Verification

**Stage:** stage-2 (Phase 2.6 of 7, the last; Stage 2 closes)
**Requirements touched by this phase:** PERF-01 (must); the engine halves of FUNC-08, UI-02 and UI-03 (Stage 3 requirements).

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| PERF-01: real-time safety | must | ✅ Complete | G6 with the analyzer on, the rings saturated and listen switching (A5, XT-alloc, `--neg tap-alloc`). Read: no lock, no allocation, no message-thread call on the audio thread; the taps are wait-free SPSC with fixed storage. |
| FUNC-08: band solo / listen | should | ⏸️ Pending (stage-3) | Engine half met (Goal 4). The UI gesture is Stage 3. |
| UI-02: true curve | must | ⏸️ Pending (stage-3) | C++ half met: G9 (a) ≤ 2.1e-3 dB, inside the 0.1 dB criterion; packets bit-exact. G9 (b) (the JS evaluator) is 3.2. |
| UI-03: analyzer | should | ⏸️ Pending (stage-3) | Taps and analyzer engine met (Goal 2). Drawing is Stage 3. |

Stage 2 requirements as the stage closes:

| Requirement | Status | What remains |
|---|---|---|
| FUNC-01 | ⚠️ Partial | State survives save / reload and preset recall: Stage 4 (presets). |
| FUNC-02 | ⚠️ Partial | Live type change "no click beyond the response change": the gate is met (2.4). Held for the hands-on listen of VERIFICATION-2.4 Issue 1. |
| DSP-06 | ⚠️ Partial | Ranges reachable from the UI and typed entry: Stage 3. |
| QUAL-01 | ⚠️ Partial | Listening across factory presets: Stage 4. |
| FUNC-03 – 06, DSP-01 – 05, DSP-07, COMPAT-01, PERF-01 | ✅ Complete | — |

**Requirements Summary (all 27):**
- ✅ Complete: 12 (COMPAT-01, FUNC-03, FUNC-04, FUNC-05, FUNC-06, DSP-01, DSP-02, DSP-03, DSP-04, DSP-05, DSP-07, PERF-01)
- ⚠️ Partial: 4 (FUNC-01, FUNC-02, DSP-06, QUAL-01). Each one's remaining criterion belongs to Stage 3 or 4, or to the listen.
- ⏸️ Pending / later stage: 11
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Contract checksums | ✅ Pass | BRIEF, parameter-spec, ARCHITECTURE and ROADMAP equal STATUS |
| Commit hygiene | ✅ Pass | `52257b89` lists this plugin's paths and `PLUGINS.md` only; plugin tree clean; no tag |
| Build (five objects deleted, recompiled) | ✅ Pass | rc 0; no warning from this plugin's files |
| Designer harness, default and `--long` | ✅ Pass | `150 PASS, 0 FAIL, 18 INFO` both times; spot-checked figures equal SUMMARY-2.6 (XG-exact 2.06e-5, XL-onh 33 635x, XL-sweep 1.110x, XL-seed 4.71e-15) |
| Render harness, default and `--long` | ✅ Pass | `155 PASS, 0 FAIL` both times (`--lid-dir` = the independent 0.5.0 dump). CPU INFO is inflated by the concurrent mutation run and is not a figure |
| Negative controls | ✅ Pass | All 12 (`rbj`, `f1`, `alloc`, `seed`, `rbj-b`, `binary-gate`, `first-build`, `engine-alloc`, `ramp-alloc`, `fade-alloc`, `ag-alloc`, `tap-alloc`) rc 0; an unknown name rc 255 |
| Static gates (PLAN-2.6 T10.7) | ✅ Pass | `Source/dsp` framework / allocation tokens: none. Processor: no `IIR::`, `SmoothedValue`, `AsyncUpdater`, `AbstractFifo`, lock or listener. `Source/analysis`: no listener or `AsyncUpdater`. Header edits are additions only. `pathIdleForBlock`: one definition, two calls; `stageIdleForBlock()` used only inside it |
| Layout golden, spec | ✅ Pass | `--dump-params` equals `params.tsv`; `OK: 196 params` |
| Installed bundles | ✅ Pass | Exactly the two `-dev` bundles, 0.6.0, identical to the artefacts, newer than every source file |
| pluginval 10, `auval` | ✅ Pass | VST3 `SUCCESS`, AU `SUCCESS`; `AU VALIDATION SUCCEEDED`, `[1, 1] [2, 2]`; no NaN / inf in the logs |
| Independent probe | ✅ 12 / 12 | `verify-2.6/probe-output.txt` (+ N1e INFO, Issue 1) |

### Mutation run

The unmodified control (M0) reproduces 150 / 0 / 18, 155 / 0 and 12 / 0. Full table: `verify-2.6/results-all.txt`; survivors on the final probe: `results-survivors-probe2.txt`.

**Caught by the harnesses (49 of 69):**
- **Both harnesses (29):**
  - listen: LS1 – LS7, LS9, LS14, LS18 – LS29, LS31, LS33, LS34;
  - tap: ST2, ST3, ST5, ST6, ST7.
- **Designer only (3):** LS8 (XL-latch), ST1 (XT-ring), ST4 (XT-ring, XT-thr).
- **Render only (17):** LS16, PX21, PX23, PX24, PX29, PX31 – PX37, PX40, DM1, DM3, AN3, AN4.

**Thinly gated (one row in one harness):**
- LS8 (XL-latch);
- ST1 (XT-ring);
- PX24 (XR-lseed);
- PX31 (XR-lag);
- PX32 / PX33 (XR-gate);
- PX34 (XR-ovf);
- PX36 (XR-anpp);
- PX37 (XR-gen);
- DM1 (XR-g9id);
- AN4 (XR-anrel);
- the router and map mutants LS19 – LS27 (XL-map or XL-route, plus one render row each).

**Survive both harnesses (20):**

| Mutant | Kind | Probe |
|---|---|---|
| **LS10** an f / Q edit during an engage / change is not passed to the incoming voice (deferred to the move's end) | **real** | **caught (E1)** |
| **LS11** a settled f / Q edit runs a 50 ms change move instead of a ramp | **real** | **caught (T1)** |
| **LS12** a structural edit of the listened band (type family) is ignored | **real** | **caught (T1)** |
| **LS15 / PX26** a non-finite input leaves the listen voices non-finite for good (clearState not run) | **real** | **caught (N1)** |
| **PX27** the listen voices get no control tick (f / Q ramps never move) | **real** | **caught (I1, E1)** |
| **PX38** `listenBand` 24 accepted (listens to a default-constructed input) | **real** | **caught (R1)** |
| **PX39** a latched un-bypass jumps although listen is engaged (no fade back, with no band running and a unity stage) | **real** | **caught (X1)** |
| PX22 the listen input without the effective placement | real by read, mono layout only: a placement edit of the listened band while listening starts a change move between identical filters | not caught |
| DM2 the display model does not re-design on a layout-only change at the same rate | real by read: placements stay stale until a band edit (XR-dm changes the rate, never the layout alone) | not caught (no display row in the probe) |
| AN1 the analyzer ignores the drop counter | real by read: one frame straddles an overflow gap (display only, transient) | not caught |
| PX42 rate published before the layout | real by read, concurrent only (a reader can see the new rate with the old layout for one poll; self-heals at the next poll); not observable single-threaded | — |
| PX28 segment not ended at a listen move end; PX30 gain form chosen by `isIdle` | real in double (the fused `x·(ag·og)` and split `(x·ag)·og` forms differ by one double ulp) and invisible in float output: probe B2 16 / 16 and B1 20 / 20 bit-identical with both gains away from unity | not caught (effectively equivalent) |
| AN2 the drain after a drop is not skipped | equivalent: the analyzer drains everything every call, so the drop count cannot lead the gap (SUMMARY deviation 4); the extra skip is a stricter guard | — |
| LS13 `goIdle` drops the target; PX25 step 7 does not idle listen | equivalent: every path out of step 7 either stops processing until a restart, or seeds at once, and the restart's `seedListen()` retires both voices and sets the target from the wish | — |
| LS17 `hasPending` ignores an input change of the same band | equivalent: with the wish's index ≥ 0 equal to the request's, listen is engaged, so `isIdle()` is false either way | — |
| LS32 `configure` does not reset the voice | equivalent: every `configure` targets a voice already retired (reset) | — |
| PX41 the segment rule uses `isEngaged` | equivalent: a wish that is not yet taken needs no dry copy (`applyPending` runs at the tick before the segment) | — |

The shipped code takes the correct branch in every surviving case. By read:
- `applyPending` passes continuous edits to the incoming voice;
- `settle` ramps continuous changes and runs a change move on structural ones;
- step 8 calls `listen.clearState`;
- step 6 calls `listen.controlTick`;
- step 4 bounds `listenBand` to 0 … 23;
- step 7's jump requires `listen.isIdle()`;
- `makeInput` uses `effectiveBandInput`;
- `DisplayModel::poll` re-designs on a layout change;
- the analyzer discards on a drop;
- `prepareToPlay` stores the layout before the rate.

## Human Verification

Not blocking. Carried to the hands-on listen (Stage 3, once the editor exists, or in a host with a generic editor):

- [ ] Listen on a narrow band-pass under bass: no burst; the 50 ms engage feels immediate enough.
- [ ] Listen on a Mid band of a stereo mix; listen while auto gain is on (the level should not move).
- [ ] Listen on a Low Shelf at 20 Hz with a 1 kHz-heavy source (the κ-void case of deviation 3): no audible click at engage / release.
- [ ] VERIFICATION-2.4 Issue 1 (the stop-band burst on a steep cut's slope change) and the earlier hands-on items (2.3 – 2.5).

## Issues Found

None is a defect against the specification that blocks the stage.

1. **A NaN inside listen's input ramp surfaces late, as a short burst, when no band is running (minor, probe N1e).**
   - Step 8 detects non-finite state from the last sample of each chunk.
   - With no band running, the EQ path is the input itself, so a single NaN sample does not reach the chunk end. A listen voice still inside its input ramp (engage or change) holds the NaN in its state while it is not yet heard.
   - When the voice becomes audible at move sample R, the output is non-finite until the end of that chunk: 25 samples at block 64, 89 at block 512 (48 kHz), up to 30 ms after the NaN. The voice is then cleared.
   - With a band running, or with listen settled, the NaN is caught at its own chunk and nothing surfaces later (N1 8 / 8, N1e "band on": none).
   - G5's "non-finite input does not stick" holds.
   - Suggested fix for a later `/improve`: have `Listen::process` report a non-finite voice output (it renders every voice anyway) and treat it like a non-finite lane at step 8.
2. **Gate gaps held by the probe only.** Each is a real mutant that survives both harnesses:
   - LS10 (E1); LS11 / LS12 (T1); LS15 / PX26 (N1); PX27 (I1, E1); PX38 (R1); PX39 (X1).
   - Port the probe rows into the render harness at the next harness edit (a Stage 3 phase or an `/improve`). The probe in `verify-2.6/` is the template.

   Also ungated, real by read:
   - DM2: an XR-dm case for a layout change at the same rate. Stage 3 relies on it when a host renegotiates mono / stereo.
   - AN1: an XR-ovf case asserting that no frame straddles a drop.
   - PX22: a mono-layout placement edit while listening starts nothing.
3. **SUMMARY-2.6 deviation 1 judged: accept.**
   - XG-exact chooses its scored points from the closed form alone, so a loud engine response under a quiet closed form would have been voided rather than failed.
   - Re-scored with a symmetric floor (a point counts if either the closed form or the measurement is above −150 dB and within 80 dB of the larger maximum): the same 646 voids, 0 points that only the symmetric rule scores, worst 2.06e-5 dB either way.
   - The voided chains' measured responses also peak at −150.5 dB or lower. (The summary's "−158 dB or lower" is a misstatement: the closed-form maximum among voids is −150.5 dB.)
   - The render row's 7 voids sit on packets that XR-g9id proves bit-equal to the engine's designs.
   - Recommendation (hardening, not required): make the XG-exact and XR-g9 floors symmetric at the next harness edit.
4. **SUMMARY-2.6 deviation 3 judged: accept.**
   - κ is voided only where the destination's held level at the tone is more than 40 dB under the source's (Low Shelf 20 Hz listened through a 20 Hz low-pass: about 68 dB).
   - There κ divides by a near-silent reference. The (a) second-difference gate still binds every one of those rows.
   - The hands-on item above covers the audible side.
5. **ROADMAP wording drift.**
   - The 2.6 criterion still reads "ring overflow keeps the newest samples". Per P2 and Taylor's XR-ovf call, the ring never overwrites unread data: a push keeps its newest samples that fit, and the processor offers each block's last 32 768.
   - The behaviour is as decided; the ROADMAP sentence is stale. Amend it at Stage 3 discuss (with a checksum refresh) rather than in a verify.
6. **VERIFICATION-2.4 Issue 1 is still open**, waiting on the listen. The input-ramp infrastructure it would need now exists in `Listen.h` (PLAN-2.6, out-of-scope table).
7. **CPU** (INFO; G10 binds in Stage 4). X-cpu5 per SUMMARY-2.6: taps + listen add +0.1 – 0.4 %. This session's CPU INFO ran beside the mutation run and is not a figure.

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 2.6; Stage 2 complete)

**Ready for next stage:** Yes. Stage 3 (GUI) starts with discuss.

**Blockers (if any):** none. Carried forward, non-blocking:
- Issue 1, the late NaN burst: a small `/improve` item.
- Issue 2, the probe-only gates to port; DM2 matters for Stage 3.
- Issue 5, the ROADMAP wording, at Stage 3 discuss.
- The hands-on listen, including VERIFICATION-2.4 Issue 1.
- For Stage 3, from SUMMARY-2.6:
  - the editor registers the analyzer consumer and drives `SpectrumAnalyzer::process (measured dt)` and `DisplayModel::poll()` from its timer;
  - `eqSections` / `analyzerPre` / `analyzerPost` and the `init` pull;
  - `listenBand` from the drag gesture;
  - the meter reader;
  - G9 (b) on packets dumped by the render harness.
