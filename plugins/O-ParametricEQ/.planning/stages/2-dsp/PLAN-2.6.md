---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.6 Execution Plan (analyzer taps, display model, listen)"
created: 2026-11-15
juce_version: "8.0.15"
phase: "2.6"
summary: "Seven waves, serial: (W0) pre-flight, baselines and the 0.5.0 session dump the listen-off identity row compares against; (W1) std-only eq::SpectrumTap and eq::Listen in Source/dsp, three additive edits to verified headers (OutputStage::nextGains, BandParams effectiveBandInput, ResponseEval complex-s evaluation), designer-harness rows in double (listen G3, routing, ring semantics, G9 engine exactness, OS18); (W2) SpectrumAnalyzer and DisplayModel (JUCE, message thread) as plugin TUs; (W3) the processor: taps, listen after auto gain (C3), the extended idle predicate, seeding, preparedRate / preparedStereo, test accessors, version 0.6.0; (W4) render-harness rows (G9 (a) damped + unit-circle + packet identity + controls, analyzer reading / overflow / generation, listen G3 / identity / seeding / block identity, G6 saturated, the four owed render rows) and the S5 amendment; (W5) build, both harnesses, controls, --long, install, pluginval 10, auval; (W6) summary, STATUS, one path-scoped commit. Stage 2 closes at verify 2.6. No WebView, no JS evaluator, no G9 (b). No tag."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-ParametricEQ — Stage 2 / Phase 2.6 Execution Plan (analyzer taps, display model, listen)

## Goal

Everything the editor needs from the processor exists and is proved headless:

- **Display model.** A message-thread `DisplayModel` designs every band with the processor's own designer code, rate and layout, and emits section packets. The curve drawn from those packets equals the plugin's measured response (G9 (a) as amended: damped circle, within 80 dB of each response's maximum, ≤ 0.05 dB) and the packets equal the realised designs bit for bit.
- **Analyzer.** Two read-only taps feed lock-free rings of fixed storage; a message-thread `SpectrumAnalyzer` (Hann, zero-pad 4x) reads a −12 dBFS 1 kHz sine at −12 ± 0.5 dB at every rate; overflow keeps the newest samples and never blocks.
- **Listen.** `eq::Listen` solos the listened band's region of the dry input, after auto gain and before output gain (C3). It engages over a 30 ms input ramp plus the 20 ms crossfade (C2), passes G3 on / off and on band changes, and with listen off the output is bit-identical to the 0.5.0 processor.
- **G6** holds with the rings saturated and listen switching. The five rows VERIFICATION-2.5 owes exist.

Version 0.6.0 is built, installed and validated. Proved at two levels, as in 2.2 – 2.5: the std-only parts in double by the designer harness, the plugin's float output and the JUCE classes by the render harness.

**In scope:** `eq::SpectrumTap`, `SpectrumAnalyzer`, `DisplayModel`, `eq::Listen`; the processor's taps, listen path, extended idle predicate, seeding, published rate and layout; gates G9 (a), G6 (analyzer on, listen), G3 for listen, the analyzer rows, the listen-off identity row; VERIFICATION-2.5 Issue 2 (PX17, PX11, PX4, AG3, OS18).

**Out of scope (and what 2.6 does instead):**

| Later | 2.6 behaviour |
|---|---|
| Stage 3 (3.1 – 3.3) | No editor, no WebView, no timer, no `emitEventIfBrowserIsVisible`. `DisplayModel::poll()` and `SpectrumAnalyzer::process (dt)` are driven by the render harness on its own thread. The JS evaluator and G9 (b) are 3.2. Whether the UI ends listen when a band is disabled is a Stage 3 call. |
| VERIFICATION-2.4 Issue 1 | Still a listening item. 2.6 lands the input-ramp infrastructure (a 30 ms raised-cosine table) inside `Listen.h` only; `SlotBand` does not take it. If the hands-on confirms the burst is audible, a later `/improve` can apply the same ramp to structural changes (RESEARCH-2.6 Q5: 20 + R ms). |
| Not planned | Blackman-Harris or flat-top windows; a binary display channel; dry pass-through of the untouched channel on Left / Right listen (one-line change of the solo router if Taylor asks after the hands-on); a pre-roll. |

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/RESEARCH-2.6.md` | All of it. Q1 (G9 closed form, damped measurement, controls), Q2 (taps, rings, analyzer calibration), Q3 (listen: mapping, solo router, input ramp, structure, C3), Q4 (display model, packets, precision), Q5 (G6, owed rows). |
| `stages/2-dsp/research-2.6/tap_bench.cpp`, `bench-output.txt` | Working prototype and its figures: `composeAtS`, `damped`, `g9`, `g9unit`, `g9diag`, `decay`, `analyzer`, `listen`, `listen2`, `json`, `cost`. |
| `stages/2-dsp/VERIFICATION-2.5.md` | Issue 2 (the five owed rows and the probe rows they template: S3 / S3b, T1 / T3, T2, E1, T3). |
| `stages/2-dsp/SUMMARY-2.5.md`, `PLAN-2.5.md` | P6 (the per-block order this plan extends), P8 (controls are harness renders), P12 (existing-row discipline), P13 (multi-band compares). |
| `stages/2-dsp/CONTEXT.md` | D11 and "Constraints carried". Binding. |
| `research/ARCHITECTURE.md` | § 8, 9, 10, "Processing Chain", "True-response display", "Analyzer", "Processing Order Requirements", "Thread Boundaries", G3, G6, G9 — **as amended by this plan phase**. |
| `Source/dsp/Fade.h`, `Band.h`, `SlotBand.h`, `BandParams.h`, `ResponseEval.h`, `OutputStage.h`, `AutoGain.h`, `Designer.h` | The verified engine. P1 lists the only edits allowed. |
| `Source/PluginProcessor.h/.cpp` | `processAudio` as 2.5 left it (steps 1 – 9, `stageIdleForBlock`, `seedOutputStage`, `makeInput`, `getBandDynOffsetDb`, `updateOutputPeak`, `analyzerMode` in the UI state). |
| `tests/designer-harness/main.cpp`, `tests/render-harness/main.cpp` | Row and tally conventions; `XfEngine`; `fadeRefRender`; `render` with its per-block callback; A1 – A4 windows; `maxD2`, local κ; the step-row harness of 2.4 / 2.5; S5. |
| `scripts/param-dump/ParamDump.cmake` (`ouaricon_add_processor_console`) | The render harness compiles the plugin target's own `SOURCES` (minus the editor TU), so a new `.cpp` added to `target_sources` reaches it. Confirm the filter does not drop the new TUs. |
| Memory `index_dsp_gate_design.md`, `index_testing_render_harness_patterns.md`, `pattern_gate_stimulus_below_threshold_is_vacuous.md`, `pattern_downstream_guard_masks_defect_gate_the_clamp_target.md`, `pattern_run_flag_past_dirty_check_latches.md`, `pattern_webview_one_shot_state_push_stale_on_preset_load.md`, `pattern_generation_reaper_freezes_when_host_idles_audio_thread.md`, `pattern_gate_inherits_the_review_findings_scope.md` | Suite gate recipes and traps this phase leans on. |

---

## Plan-phase decisions

User calls taken in this phase (2026-11-15), all as recommended; contracts amended and checksums refreshed:

- **C1 G9 (a) on the damped circle**, scored within 80 dB of each response's own maximum; a confirming unit-circle row on decaying draws; packet identity bit-exact; four negative controls. ARCHITECTURE G9 amended.
- **C2 Listen engages over 50 ms:** a 30 ms raised-cosine input ramp on the incoming listen filter, then the 20 ms crossfade. ARCHITECTURE § 10 amended (with the solo-router table).
- **C3 Listen after auto gain, before output gain.** ARCHITECTURE "Processing Chain" and "Processing Order Requirements" 3 – 5 amended.

Also amended by the plan (no call needed): ARCHITECTURE § 8 (std-only ring), "Analyzer" (zero-pad 4x, 30 kHz axis, interpolation rule, storage at construction, generation, newest-fit overflow, consumer flag, taps on every prepared path), "Thread Boundaries" (consumer flag replaces `editorOpen`; `preparedRate` / `preparedStereo`), G6 (what "analyzer on" means); ROADMAP 2.6 test criteria.

The plan's calls:

- **P1 Files.**

  | File | Status | Content |
  |---|---|---|
  | `Source/dsp/SpectrumTap.h` | new, std-only | `eq::SpectrumTap` (P2) |
  | `Source/dsp/Listen.h` | new, std-only | `eq::Listen`, `listenInputFor`, the solo router, the ramp table (P5 – P7) |
  | `Source/analysis/SpectrumAnalyzer.h/.cpp` | new, JUCE | message-thread analyzer (P3) |
  | `Source/analysis/DisplayModel.h/.cpp` | new, JUCE | message-thread display model (P8) |
  | `Source/dsp/OutputStage.h` | **additive edit** | `nextGains (double* ag, double* og, int n)` (P9). `process` and everything else byte-identical. |
  | `Source/dsp/BandParams.h` | **additive edit** | `effectiveBandInput (const RawBand&, float gainScaleRaw, bool stereoLayout)`: `makeBandInput (raw, gainScaleRaw, 0.0)` then `effectivePlacement`. The processor's `makeInput` and the display model both call it (one definition). |
  | `Source/dsp/ResponseEval.h` | **additive edit** | `sectionResponseAtZ (std::complex<double> z, const SectionParams&)`, `bandResponseAtZ (const BandDesign&, z)`: s = (z − 1) / (z + 1), the same rational forms. The unit-circle functions are untouched. |
  | `Source/dsp/Band.h` / `SlotBand.h` | additive only if needed | a `const` accessor for the current slot's `BandDesign` (and whether it is off / asleep), if none exists. |
  | `Source/PluginProcessor.h/.cpp` | edit | P10 |
  | `CMakeLists.txt` | edit | `OPEQ_VERSION 0.6.0`; the two new `.cpp` in `target_sources`. Nothing else. |

  Rules as before: `Source/dsp` is C++17, `noexcept`, no allocation, no framework, and the gate tokens are absent even from comments. Every edited verified header keeps its existing functions byte-identical (T10.7 diffs them); every existing row that touches those files prints as in the baseline. Any other edit of a verified `Source/dsp` file is a deviation recorded with its reason.

- **P2 `eq::SpectrumTap`, exactly (RESEARCH Q2).**
  - Storage: `float ring[32768]` (capacity C = 32768, a power of two), `std::atomic<uint32_t> writeIndex, readIndex` (free-running counts, masked on access), `std::atomic<uint32_t> generation, drops`. A member of the processor from construction (2 × 128 KB; the processor is heap-allocated).
  - Producer (audio thread): `void push (const float* x, int n) noexcept`. `free = C − (writeIndex − readIndex)`. If `n ≤ free`, copy all; else copy the newest `free` samples (`x + n − free`) and `drops += n − free`. Then publish `writeIndex` (release). Never blocks, never waits, never reads the reader's buffer.
  - Consumer (message thread): `int drain (float* dest, int maxN, uint32_t& genOut, uint32_t& dropsOut) noexcept`: reads `generation` before and after; copies `min (available, maxN)`; publishes `readIndex`. `genOut` / `dropsOut` returned so the reader can discard (P3).
  - `reset()` (called from `prepareToPlay` only): `readIndex = writeIndex` is **not** enough on a concurrent reader, so prepare stores `writeIndex = readIndex = 0` and bumps `generation` (release) last; the reader discards any drain whose generation moved. State the race argument in the header comment; it is single-producer / single-consumer, and prepare does not run concurrently with `processBlock` (JUCE contract).
  - `static constexpr int capacity = 32768`.
  - **Mono sum in the processor, not in the tap:** the processor fills a 256-sample stack piece with `0.5f * (L[i] + R[i])` (or the single channel) and pushes it piece by piece.

- **P3 `SpectrumAnalyzer`, exactly (RESEARCH P2, Q2 "message side").**
  - Constructed with the processor reference; registers as consumer in the constructor (`setAnalyzerConsumer (true)`) and unregisters in the destructor.
  - `prepare (fs)`: on first use and whenever `preparedRate` or the tap generation changes. FFT size N = 4096 / 8192 / 16384 for fs ≤ 48 / ≤ 96 / ≤ 192 kHz (above 192 kHz: 16384, stated); `juce::dsp::FFT` of order log2 (4N) (14 / 15 / 16); per-tap history of N floats; scratch of 2 × 4N; periodic Hann window `w[i] = 0.5 − 0.5 cos (2π i / N)` (as `tap_bench.cpp` `window`), computed in double, and `Σw`. Allocation happens here, on the message thread.
  - `process (double dtSeconds)` → fills `float pre[256], post[256]` (dB):
    1. Drain both rings into their histories (shift-in, newest last). A moved generation → discard the drained data and clear the history's "valid" count. A moved drop counter → clear the history's valid count (no window straddles a gap).
    2. Per tap with ≥ N valid samples accumulated since the last clear: the newest N, × w, zero-padded to 4N, `performFrequencyOnlyForwardTransform` on the scratch copy; per bin `20 log10 (|X| · 2 / Σw)`, floor −200 dB.
    3. Ballistics per bin: `display = max (new, held − 20 · dt)`; with no new frame the release still runs (the display falls when the host stops calling `processBlock`).
    4. Reduce to 256 log-spaced points over 10 Hz – 30 kHz: point p spans [f_p, f_{p+1}) with f_p = 10 · 3000^{p/256}; its value is the maximum bin in the span; a span holding no bin reads the dB interpolation (linear in dB, linear in frequency) of the two bins around its centre; a span above fs / 2 reads the floor (the page draws it empty).
  - Mode gating is the processor's (P10); the analyzer only reads.
  - Not std-only (needs `juce::dsp::FFT`), so the render harness runs it.

- **P4 Listen mapping, exactly (D11; RESEARCH Q3).** `BandInput listenInputFor (const BandInput& band)`:
  - Bell / Notch / Band Pass → Band Pass at the band's f, Q.
  - Low Shelf / Low Cut → High Cut (the 12 dB/oct low-pass), slope index 1, Q 1.0, at f.
  - High Shelf / High Cut → Low Cut (12 dB/oct high-pass), slope index 1, Q 1.0, at f.
  - Tilt → Band Pass, Q 0.7, at f.
  - `active = true` always (enable and bypass do not stop listen); gain and gain scale do not enter; placement = the band's **effective** placement.
  - So a gain edit, a slope edit, or a type change inside one family (Bell → Notch) maps to the same listen input and does nothing. `classifyBandChange (current, next)` on the mapped inputs decides the rest: identity → nothing; continuous (frequency, Q) → a ramp target on the active voice; structural (type family, placement) → the two-step move (P6).

- **P5 Solo router (RESEARCH Q3 table; ARCHITECTURE § 10 as amended).** Applied to the **dry** input (the converted lanes before any band runs), into a stack listen piece:

  | Placement | Listen output |
  |---|---|
  | Stereo | L = F(L), R = F(R) (`processPair`) |
  | Left | L = F(L), R = 0 |
  | Right | L = 0, R = F(R) |
  | Mid | m = 0.5 (L + R); L = R = F(m) |
  | Side | s = 0.5 (L − R); L = F(s), R = −F(s) |
  | Mono layout | F(x) |

- **P6 `eq::Listen`, exactly (RESEARCH P4, P5).**
  - **Voices:** two, each an `eq::Band` (Band.h, the verified section engine with its control ramp) plus its `Placement`, its mapped `BandInput`, and an input-ramp position `r`.
  - **Constants:** `listenRampSeconds = 0.030`; `maxListenRampSamples = 11520` (30 ms at 384 kHz). A ramp table `double ramp[11520]` built in `prepare (fs)` with `R = max (1, round (0.030 fs))` and the `FadeTable::weightOf (i, R)` formula (no edit of Fade.h). The 20 ms part is the processor's `FadeTable`.
  - **State machine.** `target` (−1 or a band index), `active` (the voice audible when no move runs), and one move at a time:

    | Move | What runs | Ends |
    |---|---|---|
    | **engage** (−1 → band) | incoming voice from rest; its input × `ramp[r]` for R samples (EQ path weight 1); then output `y = (1 − w) · eq + w · listen` over the fade table | R + N samples; listen fully on |
    | **change** (band A → band B, or the listened band's type family / placement) | incoming voice B from rest, input ramp R; voice A at weight 1 meanwhile; then `listen = (1 − w) · A + w · B` over N | R + N; B active, A reset |
    | **release** (band → −1) | `y = (1 − w) · listen + w · eq` over N; no ramp (the EQ path is warm) | N; listen idle, voices reset |
    | **ramp** (the listened band's f or Q edited) | `Band::setTarget` on the active voice; its `controlTick` on the grid | not a move: no latch |

  - **Latch (the SlotBand rule):** a request (a new target, or a structural change of the listened band) arriving during a move waits for the move's end and is then taken as one request (the latest wins). The voice that is not used stays at rest.
  - **Requests are read at the block start and handed over at the next control tick** (`setPending (index, mappedInput)` from step 4; `applyPending (designer)` on the tick). Like every other request, so listen renders identically at any block size.
  - **Seeding:** `startSettled (index, input, designer)`: no move, the voice snapped to its design and started from rest, weight 1 if listening; −1 → idle. Called at the first block and at both un-bypass restarts (P10).
  - **`goIdle()`** (the step-7 fade-to-bypass end): voices reset, no move; `target` kept so the restart seeds it.
  - **`clearState()`** (step 8, non-finite): both voices' state cleared, ramps snapped; moves carry on.
  - **`isIdle()`:** target −1, no move, nothing pending. **`samplesLeftInMove()`** for the segment rule (P10).
  - **`process (const double* dryL, const double* dryR, double* L, double* R, int n, bool stereo, const FadeTable&, Designer&)`:** in place on the EQ-path lanes (already × auto gain), per P5 and the table above. With listen idle it is never called.
  - Header comment: the bench readings (from-rest burst 29 672x; pre-roll 200 ms still 10 / 3040 over 2.0x; ramp 30 ms → 1.21x on, 1.35x band change), why the weight starts only after the ramp ends (R = 50, P = 30 reads 30.7x), why a band change is never a ramp (BP → BP ramps read 23 – 76x).

- **P7 Listen cost and storage.** Two `eq::Band`, the ramp table (90 KB), two stack pieces of `kDryPieceSize` per segment. No allocation. One or two SVF sections per lane, two voices only during a move.

- **P8 `DisplayModel`, exactly (RESEARCH Q4).**
  - Owns its own `eq::Designer`, prepared at the processor's `preparedRate` (48 kHz and stereo, `rateKnown = false`, before the first prepare).
  - `poll()` → the list of band indices whose packet changed. Reads the 196 parameter atomics (the processor's raw pointers, the same `RawBand` + raw `gain_scale` path), builds `effectiveBandInput` (P1), compares the 9-float raw snapshot per band by bit pattern with the last one, and re-designs only the changed bands. A change of `preparedRate` or `preparedStereo` marks all 24. No listener, no `AsyncUpdater` (a parameter listener would fire on the audio thread under automation and design there).
  - `packetFor (band)` → `juce::var`: `{ band, type, placement (effective), active, sections: [[g, k, mL, mB, mH] …], onePoles: [[G, cL, cH] …] }`; `headerVar()` → `{ sampleRate, stereo, rateKnown }`; `allPackets()` for the page's `init` pull.
  - **`active` mirrors the engine:** false when the input is inactive (disabled or bypassed) and false when the band sleeps (`SlotBand::gainIsZero` on the input: a gain-bearing type at exactly 0 dB). The display never draws a design the engine does not run.
  - A section with no SVF or one-pole half: send the neutral values of that half (state which in the header comment); never omit silently.
  - Designs are finite (DSP-06), so no NaN reaches JSON; assert it in the harness, do not guard it in the class.
  - The display shows the target. The page is told nothing about ramps.

- **P9 `OutputStage::nextGains` (C3).** `void nextGains (double* ag, double* og, int n) noexcept`: advances both smoothers exactly as `process` does (`ag.next()` then `og.next()`, per sample, the same order) and writes the per-sample gains; settled, it writes the targets. `process` is not touched. Invariant (row XO-split): for any history, `ag[i] * og[i]` equals the `g` `process` would have used, bit for bit, and both smoothers end in the same state.

- **P10 Processor, per host block (PLAN-2.5 P6 extended; RESEARCH Q2 – Q4).** Steps not named are as in 2.5.
  - **Members:** `eq::SpectrumTap preTap, postTap`; `std::atomic<bool> analyzerConsumer { false }`; `std::atomic<int> listenBand { -1 }` (not saved; out-of-range values read as −1); `eq::Listen listen`; `std::atomic<double> preparedRate { 0.0 }`; `std::atomic<bool> preparedStereo { true }`; public `setAnalyzerConsumer (bool)`, `getPreTap()`, `getPostTap()`, `getPreparedRate()`, `getPreparedStereo()`, raw parameter access for the display model (or `DisplayModel` is a friend; pick one, state it).
  - **`prepareToPlay`:** `listen.prepare (fs, grid, rampTicks)`; `preTap.reset()`, `postTap.reset()`; then `preparedRate.store (fs)`, `preparedStereo.store (stereoLayout)` (release, after the taps' generation bump).
  - **`processBlock` / `processBlockBypassed`:** prepared → pre tap (P11), `processAudio`, post tap, `updateOutputPeak`. The taps read the float buffer only.
  - **Step 4:** also read `listenBand` → `listenWanted` (−1 if out of range) and, when ≥ 0, that band's raw snapshot → `listen.setPending (listenWanted, listenInputFor (effectiveBandInput (…)))`; −1 → `listen.setPending (-1, {})`. Every processed block. `makeInput` now calls `effectiveBandInput`.
  - **Seeding (step 4 restart and step 7 restart):** after the bands and the output stage: `listen.startSettled (pending …)`.
  - **Steps 5 and 6: one predicate.** `stageIdleForBlock()` becomes `pathIdleForBlock()` = the 2.5 stage predicate **and** `listen.isIdle()` with nothing pending (computed at the block start after step 4, never latched; memory `pattern_run_flag_past_dirty_check_latches.md`). Step 5's untouched return and step 6's "every lane converted" both use it, as in 2.5. The grep gate counts one definition and two calls.
  - **Step 6, per segment:**
    - On a tick: band ticks, `applyOutputPending`, the auto-gain refresh (2.5), **then `listen.applyPending (designer)` and the active voices' `controlTick`**.
    - Segment length: also ends where a listen move ends (`listen.samplesLeftInMove()`), and is capped at `kDryPieceSize` while listen is not idle.
    - The dry copy is taken when a bypass fade is in flight **or listen is not idle** (one copy, shared).
    - Bands, in index order.
    - **Gains and listen:** listen idle at the segment start → `outputStage.process` (the 2.5 fused multiply, unchanged). Not idle → `outputStage.nextGains (ag, og, m)`; lanes × `ag[i]`; `listen.process (dry, lanes, …)`; lanes × `og[i]`.
    - The bypass mix, as before, against the dry copy.
  - **Step 7:** at the fade-to-bypass end, `listen.goIdle()` beside the bands.
  - **Step 8:** a non-finite lane sample also runs `listen.clearState()`.
  - **Test accessors** (public, harness-only use, like `getBandDynOffsetDb`): `getBandRealisedDesign (int band, eq::BandDesign&) const` (the current slot's design; false when off / asleep); `getListenState()` (target, move, positions) for the rows that need it.
  - The 2.5 per-block comment block above `processBlock` is rewritten for 2.6 (the order now names the taps and listen).
  - Still no lock, no allocation, no listener, no `AsyncUpdater`, none of `juce::SmoothedValue`, `juce::dsp::Gain`, `juce::dsp::IIR`, `StateVariableTPTFilter`, `juce::AbstractFifo` in the processor.

- **P11 Tap gating.** A tap writes only when `analyzerConsumer` is true **and** `analyzerMode` has its bit (1 Pre, 2 Post; 3 both; 0 Off writes nothing). The mode is read once per block. Not prepared (`preparedRate == 0` or the 2.5 "not prepared" return): no write.

- **P12 No test switch in the engine or the plugin** (2.3 P11 – 2.5 P8 stand). Controls are harness renders or harness-side closed forms:
  - `listenRefRender`: an independent per-sample restatement of P5 + P6 (harness code: its own `Band` instances, its own ramp weights from the formula, the fade weights from the formula; no `Listen`), scored sample by sample.
  - `listenHardRender`: the same with R = 0 and N = 1 (the hard control of the listen G3 rows), and with R = 0, N as specified (the "fade from rest" figure of the bench, printed).
  - The G9 controls perturb the harness's closed form, not the plugin.
- **P13 Listen-off identity against the 0.5.0 processor.** The roadmap criterion is "bit-identical to the output without the listen code path". The 0.5.0 processor is exactly that build. T1 adds a `--dump-sessions <dir>` mode to the render harness (harness-only change, before any Source change), builds it against the unchanged 0.5.0 Source, and writes 16 session renders to `$S/sessions-050/`. After the change, row XR-lid renders the same sessions and `memcmp`s. No audio golden is committed (2.2 P13 stands): the files live in the scratchpad for this session only.
- **P14 Negative controls live inside the harness** as rows `…n`; the designer harness gains `--neg tap-alloc` (a planted allocation inside the XT-alloc window).
- **P15 CPU is INFO.** G10 binds in Stage 4.
- **P16 Existing rows.** Baselines (VERIFICATION-2.5): designer harness **131 PASS / 0 FAIL / 17 INFO**, render harness **122 PASS / 0 FAIL**.
  - **Designer harness: no existing row is edited and all print as in the baseline.**
  - **Render harness: one amendment, by name.**

    | Row | 2.5 assertion | 2.6 form |
    |---|---|---|
    | S5 | P14 draw + gains (PLAN-2.5) | the same, plus `listenBand` (redrawn with p 1/200 per block: −1 with p 0.5, else a uniform band) and the analyzer (consumer registered, mode redrawn p 1/500, never drained, so the rings overflow); the end clause "all bands off, output 0 dB, auto off → `memcmp` equal within 300 ms" also sets listen −1 first |

    Every other existing row runs with listen −1 and no consumer registered; any existing row whose verdict or figures change → STOP with the row and both readings.
- **P17 Multi-band compares (VERIFICATION-2.4 Issue 4)** as PLAN-2.5 P13.

---

## Tasks

Roles as in `PLAN-2.2.md`: **[agent]** = dsp-agent writes files (no Bash); **[run]** = the execute session. **BG** = `run_in_background`, log to `$S/<name>.log`; `$S` = the session scratchpad. Paths are relative to `plugins/O-ParametricEQ/` unless they start with `build/` or `scripts/`. `$H` = the designer harness binary, `$R` = the render harness binary.

### Wave 0 — pre-flight

**T1. Pre-flight, baselines, the 0.5.0 session dump** [run, agent for the dump mode]
- Done when (any mismatch → STOP):
  - `shasum -a 256` of `parameter-spec.md`, `research/ARCHITECTURE.md`, `ROADMAP.md` equal STATUS `contract_checksums` (ARCHITECTURE and ROADMAP refreshed by this plan phase).
  - `git branch --show-current` = `main`; `git status --short plugins/O-ParametricEQ` is empty. Save the full `git status --short` to `$S/git-status-T1.txt`.
  - (BG) `ninja -C build O-ParametricEQ-designer-harness O-ParametricEQ-render-harness` → rc 0. `$H > $S/designer-base.log` → **131 PASS, 0 FAIL, 17 INFO**. `$R > $S/render-base.log` → **122 PASS, 0 FAIL**. `python3 tests/golden/check_spec.py` → `OK:`.
  - [agent] `tests/render-harness/main.cpp` gains `--dump-sessions <dir>` and the session list `lidSessions()` (P13): 16 sessions, seeded, each 4 s: random band automation (2.5 S5 draw without listen), output gain and auto gain toggles, bypass toggles by both routes, mono and stereo, 44.1 and 96 kHz, a random block-size sequence 1 – 4096, white noise −12 dBFS ending in the denormal tail. Writes one raw float file per session. Nothing else in the harness changes in T1.
  - [run] Rebuild the render harness against the **unchanged** Source; `$R --dump-sessions $S/sessions-050` → 16 files; `$R` again → still 122 / 0 (the new mode added no row). `shasum` of the 16 files to `$S/sessions-050.sha`.

### Wave 1 — std-only parts, in double

**T2. `Source/dsp/SpectrumTap.h`** [agent]
- P1, P2. Header comment: the SPSC argument, newest-fit overflow and why (an offline bounce with blocks > 32 768 still delivers its last 32 768), generation, why storage is fixed at construction (a prepare on another thread than the reader's is a use-after-free otherwise).
- Depends on: T1.

**T3. `Source/dsp/Listen.h`** [agent]
- P1, P4 – P7. Includes `Band.h`, `BandParams.h`, `Fade.h`, `Designer.h`; edits none of them.
- Depends on: T1.

**T4. Additive edits: `OutputStage.h`, `BandParams.h`, `ResponseEval.h` (and a `Band.h` / `SlotBand.h` const accessor if needed)** [agent]
- P1, P9. Each addition sits after the existing code with a comment naming this plan; no existing line changes (T10.7 checks with `git diff`, which must show additions only in these files).
- Depends on: T1.

**T5. `tests/designer-harness/main.cpp`: 2.6 rows** [agent]
- A new heading "2.6 / taps, listen, display truth (double)". Stimuli and scores as `tap_bench.cpp` (`listen`, `listen2`, `damped`, `g9diag`); heavy rows threaded. `listenRefRender`, `listenHardRender` (P12). An XfEngine-style driver: 24 `SlotBand` chain + `OutputStage` + `Listen` in the P10 order, in double.

  | Row | Check | Threshold |
  |---|---|---|
  | XL-map | `listenInputFor` on every type × placement × slope: the P4 table; gain / slope / gain-scale edits map to identical inputs; Bell → Notch identity; Bell → Low Shelf structural | exact |
  | XL-route | Solo router (P5): each placement, stereo and mono, against a harness render of the table with an independent `Band` | ≤ 1e-12 of peak, sample by sample; mono maps every placement to F(x) |
  | XL-on | Listen on and off: the bench set (20 listened bands incl. band-passes 150 Hz – 8 kHz, Q to 40; shelves, cuts, tilt), 50 Hz and 1 kHz at −6 dBFS, 4 phases, 48 and 96 and 192 kHz, events off the grid | (a) ≤ 2.0x the louder held render (bench 1.21x); 1 kHz κ ≤ 2.5x (bench 2.36x) |
  | XL-onh | `listenHardRender` (R = 0, N = 1) on the same | ≥ 10x on (a) for every listened band whose stop band holds the tone (the harness lists the void rows by name, as G3 does); the R = 0 / N = 20 ms figure printed (bench 29 672x) |
  | XL-chg | Band changes: all 20 × 19 ordered pairs at 48 kHz, ≥ 60 sampled pairs at 96 and 192 kHz, both tones, 4 phases | (a) ≤ 2.0x (bench 1.35x), (b) ≤ 2.5x (2.41x); count over 2.0x printed (bench 0 / 3040) |
  | XL-sweep | The listened bell's frequency 10 Hz → 30 kHz in 50 ms, Q 0.025 → 40 in 20 ms, against the 2.3 same-motion per-sample reference applied to the listen filter | ≤ 4.0x the reference |
  | XL-ref | `Listen` against `listenRefRender` over the XL-on and XL-chg renders | ≤ 1e-12 relative, sample by sample |
  | XL-refn | `listenRefRender` with the weight starting at R − 1 (one sample early), and with R = 20 ms | each must differ from `Listen` by > 1e-9 |
  | XL-latch | A second request during each move kind: taken at the move's end, latest wins; a request equal to the current target cancels nothing and starts nothing | as stated |
  | XL-seed | `startSettled` on each listened type: output from the first sample equals a listen rendered for 2 s and then measured (one band, P17) | ≤ 1e-12 after the voice's settle; exact weight 1 from sample 0 |
  | XL-ag | C3: auto gain on with a +12 dB Low Shelf (estimate −6.167 dB): listen output level equals the auto-gain-off listen output | within 1e-9 dB |
  | XO-split | `nextGains` vs `process` (P9): 2000 random gain histories incl. snaps | `ag · og` bit-equal to `process`'s g; final smoother states equal |
  | XO-os18 | **VERIFICATION-2.5 OS18.** `GainSmoother` against a restated four-stage cascade with the snap rule "all four stages within 1e-9", through the snap, 8 rates, every 2.5 step pair | bit-exact sample by sample; the restatement with "last stage only" must differ on at least one pair (the control) |
  | XT-ring | `SpectrumTap`: pushes of 1 – 65 536 against drains of 0 – 40 000 in random interleaving (single thread, deterministic): every drained sample equals the producer's sample at that position; overflow keeps exactly the newest `free`; drop count exact | exact |
  | XT-thr | Two threads, producer at random block sizes, consumer draining at random intervals, 20 s: every drained run is a contiguous run of the producer's sequence (counter-valued samples) or follows a drop-counter move | 0 torn runs |
  | XT-gen | `reset()` between drains: the next drain reports the moved generation; no pre-reset sample is returned after it | exact |
  | XG-exact | **G9 engine row.** 400 random 1 – 24-band configurations × 5 rates, stereo and mono: the double `SlotBand` chain's damped impulse response (N = 2^17, a^N = 1e-9, no float rounding) against `bandResponseAtZ` composed per ARCHITECTURE's 2x2 matrices in index order; LL / RR / MM / SS within 80 dB of each maximum on 192 log points | ≤ **1e-4 dB** (bench 1.1e-6) |
  | XG-ctl | The four controls on the XG-exact draws: k × 1.01; one-pole dropped; an L band and an M band composed in the wrong order (a draw is forced to contain the pair); Mid matrix for Side | each must exceed 0.05 dB on at least one draw per control |
  | XT-alloc | 1e4 blocks of the P10 driver with listen switching every 100 ms, voices ramping, `nextGains` path, and two taps pushed into rings that are never drained, inside the `malloc_logger` window | 0 allocations, 0 frees; SKIPPED line without a logger |
  | XL-cpu | INFO. Listen per-sample cost, one and two voices, stereo / Mid | printed |

- `--neg tap-alloc`: a planted allocation inside the XT-alloc window is counted; exit 0 only if it is.
- No existing row is edited (P16).
- Depends on: T2, T3, T4.

### Wave 2 — the message-thread classes

**T6. `Source/analysis/SpectrumAnalyzer.h/.cpp`** [agent]
- P3. Header comment: the calibration table (Hann 1x reads −12.63 at 96 / 192 kHz; 4x −12.04), why Hann and not Blackman-Harris (lobe width), the interpolation rule below about 90 Hz, `dt` passed for determinism.
- Depends on: T2.

**T7. `Source/analysis/DisplayModel.h/.cpp`** [agent]
- P8. Header comment: polling, not listeners, and why (audio-thread design under automation; one-shot pushes go stale on preset load); `active` mirrors sleep; `rateKnown`; the precision note (`serialiseDouble` ~16 significant digits, ≤ 1.5e-8 dB).
- Depends on: T4.

### Wave 3 — the processor

**T8. `Source/PluginProcessor.h/.cpp`, `CMakeLists.txt`, `CHANGELOG.md`** [agent]
- P10, P11. `CMakeLists.txt`: `OPEQ_VERSION 0.6.0`; `Source/analysis/SpectrumAnalyzer.cpp` and `Source/analysis/DisplayModel.cpp` in `target_sources`. Nothing else.
- `CHANGELOG.md`: `## [0.6.0] - <execute date>` (bracket form). Added: band listen engine (solo of the band's region, after auto gain; engages in 50 ms without a burst); analyzer engine (pre / post, calibrated); display model (the drawn curve is the plugin's exact response). Nothing audible changes with listen off. Known: the stop-band burst on a steep cut's slope change (VERIFICATION-2.4 Issue 1, under listening).
- Depends on: T2, T3, T4, T6, T7.

### Wave 4 — render-harness rows

**T9. `tests/render-harness/main.cpp`** [agent]
- New section heading for 2.6; the row index in the file header extended. The S5 amendment of P16, with a comment naming this plan.
- G9 helpers: `dampedResponse` (impulse through a fresh processor seeded on the configuration: first block = settled, so no ramp; N = 2^17; aⁿ weight; a direct sum on 192 log points with a recursive phasor, re-normalised every 4096 samples), `composeFromPackets` (a `DisplayModel` on the processor under test, `poll()`, parse each packet's `var` back to `SectionParams`, compose with `bandResponseAtZ`), `slowestPoleRadius` from the packet sections.

  | Row | Check | Threshold |
  |---|---|---|
  | XR-g9 | **G9 (a).** 50 random full-range configurations, 1 – 24 bands, at 44.1, 48, 88.2, 96, 192 kHz, stereo; plus 10 mono. Four impulses (L, R, mid (1, 1), side (1, −1)); closed form from the processor's own packets | LL / RR / MM / SS ≤ **0.05 dB** where the closed form is within 80 dB of that response's maximum over the grid (bench ≤ 1.2e-3) |
  | XR-g9u | Confirming unit-circle row: draws whose slowest pole (from the packets) falls 200 dB within 6 s, rendered 1.25x that time (at least 25 per rate; the harness draws until it has them, max 200) | ≤ 0.05 dB (bench ≤ 3.8e-2 at 48 kHz) |
  | XR-g9n | The four controls (P12) on the XR-g9 draws | each must exceed 0.05 dB |
  | XR-g9id | Packet identity: after the first block, for every band, the packet's sections (parsed back from the `var`, and also straight from the model's `BandDesign`) equal `getBandRealisedDesign`, `memcmp` of the `SectionParams` | bit-equal; inactive / sleeping bands send `active: false` (rows: disabled, bypassed, Bell at exactly 0 dB, gain scale 0 %) |
  | XR-g9j | JSON: each packet through `juce::JSON::toString` and `juce::JSON::parse`, composed again | ≤ 1e-7 dB from the unserialised composition (bench 1.5e-8) |
  | XR-dm | Polling: no edit → empty list; one band edited → exactly that band; a `setStateInformation` (preset load) changing 5 bands → those 5; `prepareToPlay` at a new rate → all 24 and the header's rate; before any prepare → `rateKnown: false`, 48 kHz | exact |
  | XR-an | Analyzer reading: −12 dBFS sine on both channels at 1 kHz, 44.1 / 48 / 88.2 / 96 / 192 kHz, 8 phases, pre and post (no band), drained once per 33 ms of audio | largest display value −12 ± 0.5 dB (bench −12.08 / −12.04 / −12.04) |
  | XR-answ | 50 log tones 20 Hz – 20 kHz, the five rates | within ±0.25 dB of −12 (bench −12.11 … −11.99) |
  | XR-ann | The XR-an stimulus through an analyzer configured with zero-pad 1x at 96 kHz (a harness-side restatement of P3 step 2, the plugin's analyzer untouched) | must read below −12.5 (bench −12.63) |
  | XR-anpp | +12 dB Bell at 1 kHz, Q 1: post − pre at 1 kHz | +12 ± 0.5 dB |
  | XR-anrel | Signal stops (no more `processBlock`): `process (dt)` keeps falling at 20 dB/s; instant attack on re-entry | as stated, ±1 dB over 2 s |
  | XR-gate | Mode 0, or no consumer: neither tap's write index moves; mode 1 / 2 move only their tap; mode 3 both; unprepared: nothing | exact |
  | XR-ovf | No drain for 2 s at blocks 32 / 4096 / 65 536 (stereo, 48 kHz, the 65 536 block through `processBlock`): every block returns; drop counter > 0; the ring holds exactly the newest 32 768 mono-sum samples (compared with the input tail); then one `process` gives the XR-an reading | bit-exact tail; reading ±0.5 dB |
  | XR-gen | `prepareToPlay` at a new rate between two drains (the analyzer thread draining concurrently in a second run): no sample of the old rate reaches a transform; the analyzer re-prepares at the new size | exact |
  | XR-lon | Listen on / off through `processBlock`: the XL-on set, 50 Hz and 1 kHz, blocks 32 / 512, 48 and 96 kHz, `listenBand` written between blocks off the grid | (a) ≤ 2.0x; 1 kHz κ ≤ 2.5x |
  | XR-lonh | The same with the listen engine's input ramp and fade replaced by `listenHardRender` on the gain-neutral EQ output and the dry input | ≥ 10x every non-void row at block 512 |
  | XR-lchg | Band changes, ≥ 60 sampled ordered pairs, blocks 32 / 512, 48 kHz | (a) ≤ 2.0x, (b) ≤ 2.5x |
  | XR-lref | Every XR-lon / XR-lchg render against `listenRefRender` fed the plugin's dry input, the EQ path and the gains restated (P12) | ≤ −120 dB re the output peak |
  | XR-lid | **Listen-off identity (P13).** The 16 sessions of T1 rendered by the 0.6.0 processor; and again with the analyzer consumer registered, mode 3, drained by a second thread | each file `memcmp` equal to `$S/sessions-050/` |
  | XR-lidn | A session with one `listenBand` write (band 1, 1 block) | must differ |
  | XR-loff | Listen on → off, no band, output 0 dB, auto off: white noise without the denormal tail | `memcmp` equal to the input from the end of the release fade; with the denormal tail, from the first block that starts after it |
  | XR-lroute | Solo routing through the plugin: each placement, stereo and mono, listen settled | within float rounding (≤ −140 dB re peak) of `listenRefRender` |
  | XR-lseed | Listen on at session open (set before the first block), and listen changed while bypassed then un-bypassed by both routes: listen output from the first processed sample equals a listen started settled (one band, P17) | ≤ −140 dB re peak from sample 0; no ramp |
  | XR-lseedn | The same against an unseeded engage (ramp + fade from the first sample) | must differ |
  | XR-lag | C3 through the plugin: auto gain on with a +12 dB Low Shelf, listen on that band: listen level vs auto gain off | within 1e-6 dB on float output (target 0); output gain +6 dB moves it +6 ± 1e-4 dB |
  | XR-blk6 | XR-blk5 extended: `listenBand` events off the tick grid (p 0.2), listened band f / Q edits in flight, the analyzer on; blocks 1, 32, 64, 512, 2048 and offline (one block of the whole render) at 44.1 / 96 / 192 kHz | bit-identical per rate |
  | XR-lbyp | Bypass toggled while listening and during each move kind, both routes | finite; ≤ 2.0x on (a); the bypass fade mixes the listen output with the dry input (≤ −120 dB vs the restated mix) |
  | XR-px17 | **VERIFICATION-2.5 PX17.** Latched un-bypass with no band and output gain +12 dB (fade to bypass, un-bypass inside it, by both routes, blocks 32 / 512): the gain is in effect from the first processed sample after the restart, no fade-in | ≤ −140 dB re peak against the constant-gain render; control: an unseeded stage must differ |
  | XR-px11 | **PX11.** Output-gain edits off the tick boundary (random offsets), blocks 1 – 2048 random and fixed 1 / 7 / 64 / 512 / 2048 | bit-identical |
  | XR-px4 | **PX4.** An auto-gain toggle in an otherwise idle block (empty band list, stage at unity, one Left +1 dB bell set in the same block as the toggle in one variant, none in the other): lands on the refresh tick | the onset sample equals the refresh tick's sample at blocks 32 / 512 / 2048; bit-identical across them |
  | XR-ag3 | **AG3.** G12 auto-gain row at 22.05 and 32 kHz (the 0.45 fs grid top active) against the pink truth restated with the 0.45 fs top | within 0.06 dB of truth; control: a 0.5 fs top must read outside 0.001 dB of the 0.45 fs estimate |
  | A5 | **G6** (ARCHITECTURE as amended): the `malloc_logger` window around `processBlock` / `processBlockBypassed`, 20 s at 48 kHz, first block included; consumer registered, mode 3, no reader (rings saturate after ~0.7 s, every later block overflows); listen on and switching every 100 ms; 24 bands moving; auto gain refreshing; bypass toggled | 0 allocations, 0 frees; liveness probe |
  | A5n | A planted allocation inside the window | counted |
  | S5 | Amended (P16) | as amended |
  | X-cpu5 | INFO. X-cpu4's cases with the taps on (mode 3) and listen on (one and two voices) | printed beside X-cpu4 |

- Depends on: T8.

### Wave 5 — build, gates, install, validators

**T10. Build and run** [run]
- Done when, in order:
  1. (BG) `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` (version bump, new TUs), then `ninja -C build O-ParametricEQ-designer-harness O-ParametricEQ-render-harness OuariconParametricEQ_VST3 OuariconParametricEQ_AU > $S/build.log 2>&1` → rc 0; `grep "O-ParametricEQ/.*warning:" $S/build.log` → no lines. The render harness link line includes `SpectrumAnalyzer.cpp.o` and `DisplayModel.cpp.o`.
  2. (BG) `$H > $S/designer.log 2>&1` → **0 FAIL**. The 131 / 17 lines of `$S/designer-base.log` print the same figures (P16); every T5 row ID appears.
  3. `$H --neg tap-alloc` → rc 0. The eleven existing `--neg` controls → rc 0 each.
  4. (BG) `$R > $S/render.log 2>&1` → **0 FAIL**. Against `$S/render-base.log`: all 122 rows present; rows other than S5 print as in the baseline; every T9 row ID appears; every `…n` row reports that its control fired.
  5. (BG, one at a time) `$H --long > $S/designer-long.log 2>&1` and `$R --long > $S/render-long.log 2>&1` → 0 FAIL each; S5 prints 600 s per rate.
  6. `python3 tests/golden/check_spec.py` → `OK:`; `$R --dump-params | diff - tests/golden/params.tsv` → empty (`listenBand` and the consumer flag are not parameters).
  7. Static gates, from the repo root:
     - `! grep -rnE "juce|Juce|std::vector|std::function|new |malloc|throw " plugins/O-ParametricEQ/Source/dsp`
     - `! grep -rnE "fast-math|Ofast|fp:fast|ffp-contract" plugins/O-ParametricEQ/CMakeLists.txt`
     - `! grep -rnE "IIR::|StateVariableTPT|dsp::Gain|SmoothedValue|AsyncUpdater|callAsync|CriticalSection|std::mutex|addParameterListener|AbstractFifo" plugins/O-ParametricEQ/Source/PluginProcessor.h plugins/O-ParametricEQ/Source/PluginProcessor.cpp`
     - `! grep -rnE "addParameterListener|AudioProcessorValueTreeState::Listener|AsyncUpdater" plugins/O-ParametricEQ/Source/analysis` (polling, P8)
     - `git diff HEAD -- plugins/O-ParametricEQ/Source/dsp/OutputStage.h plugins/O-ParametricEQ/Source/dsp/BandParams.h plugins/O-ParametricEQ/Source/dsp/ResponseEval.h plugins/O-ParametricEQ/Source/dsp/Band.h plugins/O-ParametricEQ/Source/dsp/SlotBand.h | grep "^-[^-]"` → no lines (additions only, P1).
     - `git status --short plugins/O-ParametricEQ/Source/dsp` lists `SpectrumTap.h`, `Listen.h` (new) and the edits of P1 only, or the summary records the deviation.
     - `grep -n "pathIdleForBlock" plugins/O-ParametricEQ/Source/PluginProcessor.cpp`: the definition and exactly two calls; `grep -c "stageIdleForBlock()" …` used only inside it.
     - Read the bodies of `listenRefRender`, `listenHardRender` and the G9 compose helpers in both harnesses: `Listen`, `OutputStage::process` / `nextGains` and `DisplayModel` design calls do not appear in them (the G9 compose reads the packets, which is the point; the closed form itself is harness code + `bandResponseAtZ`).
- **On a failure:**
  - A T5 row red → the std-only part. Fix it, rebuild, rerun 2 – 7 in full.
  - T5 rows green and a render row red → the processor glue, the analyzer or the model. Fix those. A threshold, row list or stimulus level is not changed in this session; if a row cannot pass as specified with the T5 rows green, STOP and escalate with the figures.
  - XR-lid red → the listen-idle or tap path touched the sample path. Do not loosen it; it is the roadmap criterion.
  - A control that does not fire → the row is vacuous. Fix the row, not the control.
  - An existing row changes outside P16 → STOP.
  - XR-g9 red with XG-exact and XR-g9id green → the measurement (render length, weight, float scope), not the display: report it with the figures before touching anything.
- Depends on: T5, T8, T9.

**T11. Install and validate** [run]
- One validator at a time, each BG with its own log.
  1. `./scripts/build-and-install.sh O-ParametricEQ > $S/install.log 2>&1` → rc 0; exactly the two `-dev` bundles in the system folders, version 0.6.0.
  2. pluginval, strictness 10, `--timeout-ms 600000`, VST3: **2 runs** → `SUCCESS`. AU: 1 run → `SUCCESS`.
  3. `auval -v aufx OuPQ OuDv` → `AU VALIDATION SUCCEEDED`; channel capabilities still `[1, 1] [2, 2]`. Never `auval -a`.
  4. Standalone smoke as in 2.2 T9 (rebuild the Standalone target first) → `ALIVE`.
- Any failure: fix the root cause in `Source/`, then rerun T10 and T11 in full. A pluginval NaN or `inf` report is a finding about stacked gain (2.2 R3; 2.5 R6): record the configuration, insert nothing in the signal path.
- Depends on: T10.

### Wave 6 — bookkeeping + commit

**T12. Summary, STATUS, commit** [agent writes the summary; run does the rest]
- `.planning/stages/2-dsp/SUMMARY-2.6.md`: tallies of both harnesses (default and `--long`); XG-exact / XR-g9 / XR-g9u per rate beside the bench; the four G9 controls; XR-g9id; XR-an / XR-answ per rate beside the bench and XR-ann; XR-ovf / XR-gen; XL-on / XL-chg / XR-lon / XR-lchg beside the bench (1.21x / 1.35x / 2.36x / 2.41x) and the hard-control figures; XL-ref / XR-lref; XR-lid (16 / 16 and with the analyzer on); XR-lseed / XR-lag / XR-blk6; the five owed rows; A5 and XT-alloc counts; S5 per rate; X-cpu5; controls; validator results; deviations (every verified-header edit listed with its diff stat).
- Carry-forward:
  - **For verify 2.6** (closes Stage 2): hands-on (listen on a narrow band-pass under bass: no burst, 50 ms engage feel; listen on a Mid band of a stereo mix; listen while auto gain is on; the analyzer pre / post can wait for 3.3); a mutation run over `Listen.h`, `SpectrumTap.h`, the processor's listen / tap glue and `pathIdleForBlock`; the `--long` runs once more; VERIFICATION-2.4 Issue 1 decided after the listen hands-on.
  - **For Stage 3:** the editor registers the analyzer consumer and drives `SpectrumAnalyzer::process (measured dt)` and `DisplayModel::poll()` from its timer; `eqSections` / `analyzerPre` / `analyzerPost` emits and the `init` pull; `listenBand` written by the drag gesture; the meter reader (2.5); G9 (b) under node on packets dumped by the render harness.
- `REQUIREMENTS.md`: no status edit (requirements close at verify).
- `PLUGINS.md`: version cell → 0.6.0. Check `git diff PLUGINS.md` first; if other sessions' rows are modified, use the temp-index recipe (memory `index_git_shared_checkout.md`) or leave it out and say so.
- STATUS: phase rows, `status: stage_2_phase_2_6_executed`, `next_action: stage_2_verify_phase_2_6`, files list.
- Commit, path-scoped, on `main`. Immediately before: `git branch --show-current`, `git status --short`. `git add` the new files by name (`Source/dsp/SpectrumTap.h`, `Source/dsp/Listen.h`, `Source/analysis/SpectrumAnalyzer.h`, `.cpp`, `Source/analysis/DisplayModel.h`, `.cpp`, `.planning/stages/2-dsp/SUMMARY-2.6.md`), then `git commit -- plugins/O-ParametricEQ`. Never `-A` / `-a`. `git log -1 --stat` lists only this plugin's paths. No tag.
- Depends on: T11.

---

## Dependency graph

```
T1 (baselines, 0.5.0 session dump) ─┬→ T2 ─┬──────────→ T6 ─┐
                                    ├→ T3 ─┤                 │
                                    └→ T4 ─┼→ T5 ───────────┐│
                                           └──────→ T7 ────┐││
                                                            ↓↓↓
                                                 T8 (processor, 0.6.0) → T9 (render rows)
                                                                              ↓
                              T10 (build, harnesses, controls, --long, gates) → T11 (install, validators) → T12
                                ↑   │
                                └───┘ fix → rebuild → rerun T10 in full (and T11 if Source changed)
```

T2, T3 and T4 are independent. T5 and T6 / T7 / T8 / T9 touch different files; one build covers all of them. T1's session dump must finish before any Source edit.

## Success criteria

| Criterion (ROADMAP Phase 2.6 as amended; ARCHITECTURE G3, G6, G9, § 8 – 10 as amended) | Proven by |
|---|---|
| G9 (a): closed form from the processor's packets = the measured response on the damped circle, five rates, LL / RR / MM / SS, within 80 dB of each maximum, ≤ 0.05 dB (UI-02) | XR-g9, XG-exact |
| G9 (a) confirming unit-circle row; controls fire | XR-g9u, XR-g9n, XG-ctl |
| The display draws exactly what the engine runs; JSON is lossless enough for G9 (b) | XR-g9id, XR-g9j |
| The display model follows edits, preset loads and rate changes without listeners | XR-dm |
| Analyzer: −12 dBFS 1 kHz reads −12 ± 0.5 dB at 44.1, 96, 192 kHz (UI-03 engine half) | XR-an, XR-answ, XR-ann, XR-anpp, XR-anrel |
| Ring overflow drops data without blocking; newest kept; a prepare between drains reaches no transform | XR-ovf, XR-gen, XT-ring, XT-thr, XT-gen |
| Mode Off and no consumer stop the audio-thread writes | XR-gate |
| G6 with the analyzer on, rings saturated, listen switching (PERF-01) | A5, A5n, XT-alloc, `--neg tap-alloc` |
| Listen on / off and band changes pass G3, 30 + 20 ms (C2) (FUNC-08 engine half) | XL-on, XL-onh, XL-chg, XR-lon, XR-lonh, XR-lchg, XL-ref, XL-refn, XR-lref |
| Listen plays the band's region (D11) | XL-map, XL-route, XR-lroute |
| Listen sits after auto gain (C3) | XL-ag, XR-lag, XO-split |
| With listen off the output is bit-identical to the 0.5.0 processor (no listen path) | XR-lid, XR-lidn, XR-loff |
| Listen is seeded and block-size independent | XL-seed, XR-lseed, XR-lseedn, XR-blk6, XL-latch |
| VERIFICATION-2.5 Issue 2 (PX17, PX11, PX4, AG3, OS18) | XR-px17, XR-px11, XR-px4, XR-ag3, XO-os18 |
| G5 with listen and the analyzer randomised | S5 (amended); `--long` once |
| Stage 1 – 2.5 rows, layout golden unchanged outside P16 | T10.2, T10.4, T10.6 |
| 0.6.0 validates | T11 (pluginval 10 ×3, `auval`, Standalone) |

## Risks

- **R1 The output form switch is a block-identity hazard.** Listen idle → the fused `ag · og` multiply; not idle → `· ag`, listen, `· og`. The two round differently. If the form were chosen per block, or a segment straddled the end of a release, renders at different block sizes would differ. P10: the form is chosen at the segment start and segments end where a move ends. XR-blk6 at block 1 vs offline is the row that sees it; XR-loff sees the return to fused form.
- **R2 The idle predicate now has three parts** (stage, listen, pending listen request). One function, used twice, computed at the block start after step 4. A pending listen request that is not counted leaves the lanes unconverted when listen starts mid-block (Left listen writes R = 0 into a lane that was never converted). XR-lroute (Left / Right) at block 2048 with the request mid-block, and XR-lid, are its rows.
- **R3 The tap is the one new audio-thread writer outside the sample path.** Its correctness is the SPSC protocol. XT-thr is the only concurrent row; run it long enough (20 s, random timings). If it ever reports a torn run, STOP: no retry-until-green.
- **R4 G9 render time.** 260 configurations × 4 impulses × 2^17 samples × up to 24 bands, plus 192-point direct sums: thread it across configurations, as XO-agref was. Do not cut counts or N. If the default run exceeds the 600 s watchdog, run it BG (memory `pattern_executor_watchdog_stall_run_long_commands_in_background.md`).
- **R5 The listen rows at 192 kHz have no bench figure** (the bench ran 48 kHz; RESEARCH Q3 asks for 96 / 192). R is 5760 samples there. A miss with XL-ref green is a finding: STOP, do not retune R.
- **R6 `getBandRealisedDesign` may need a `SlotBand` / `Band` accessor.** Additive and `const` only (P1). If the realised design is not stored (only the section states), the accessor returns the design the slot was configured with; say which in the summary.
- **R7 The session dump compares two binaries.** Same compiler, flags and machine within one session; if XR-lid fails on every session identically from sample 0, first rebuild the 0.5.0 dump to rule out a stale file before suspecting the code.
- **R8 Gate tokens in prose.** T10.7 greps `Source/dsp` comments: no "new ", "throw ", "malloc", "juce" in `SpectrumTap.h` / `Listen.h` or the additions. Write "no framework FIFO" for the ring's comment.
- **R9 `analyzerMode` is saved UI state (default 2, Post).** A host session restores it; with no consumer registered nothing is written. XR-gate covers the restored value.
- **R10 Shared checkout.** Other sessions have uncommitted files in the tree. Path-scope the commit and re-check immediately before it (T12).
