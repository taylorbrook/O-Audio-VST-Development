---
title: "O-GranulatorInstrument Stage 2 (DSP) — Phase 2.4 Execution Plan"
created: 2026-11-08
juce_version: "8.0.15"
phase: "2.4"
summary: "Eight waves: (W0) pre-flight with baselines (harness 150/0, renders-2.2 re-dump, fresh parent build + parity 99/0 + GV 2/0 + PN / PF tables) and C23w re-measured on the UNMODIFIED 2.3 engine; (W1) the user's knee adoption ALONE: ogi::wetKnee replaces kWetCeil·tanh, a legacyWetTanh hook proves renders-2.1 / 2.2 byte-identical (RB1 / FR1 under legacy), renders-2.4.sha256 is pinned once on the shipping path, committed separately (C1); (W2) engine in signal order with a bypass checkpoint after every task: ControlGrid 9 global rows → reverb + wet guard + knee chain → tap + 4 unity-knob FeedbackChains → record heads (whole cell written at the tick, 0.7 writer clamp + convex envelope, 20 ms fb / run ramps, settleRun at wrap / park, every take waits for park) → head clamp (g_a 40, g_b margin) → loader requestReset / resetSync; (W3) parity PB / PR rows on the frozen geometry, no parent edits; (W4) gates eg24 / eg24b / pg24, one build, harness 150 → 181, parity 99 → 133; (W5) --cpu25 D25 ≤ 40 % + typical ≤ 12 % with the frozen calibration witness, levers only on FAIL; (W6) install, Standalone, auval, pluginval 10, X2, HANDS-ON-2.4 (deferred); (W7) SUMMARY, STATUS, commit C2. No tag; version 0.1.0; CMakeLists.txt not edited; FeedbackChain.h / OutputLaw.h byte-identical."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-GranulatorInstrument — Stage 2 / Phase 2.4 Execution Plan (Working-copy feedback, Freeze, Reset, reverb)

## Goal

DSP-04 and DSP-05, plus the final PERF-02 worst-case verdict. Grain output feeds back into each stream's own working copy:
- an amp-weighted **mean** tap per stream with the per-(k,s) `g_fb` law;
- 4 forked `FeedbackChain`s (VERBATIM, driven at unity knob) with 32-sample returns;
- a record head per slot at exactly 1×, wrapping in the loop region, writing `wc ← (1−λ)·wc + λ·r[n−32]` with `λ = 0.95·feedback` ramped over 20 ms, the whole cell written before any read;
- a spawner head clamp in head-relative modular coordinates with forced-crossing telemetry;
- Freeze (heads stop, chains warm), Reset (off-thread rebuild from pristine + the 20 ms dual-read crossfade), and the SR-rebuild park.

The output is `ReverbUnit` (atomic bind) → wet guard → the parent's D9 knee `wetKnee` (adopted by the user, re-baselined once with attribution) → `output_gain`.

DSP-04 is proven by leaf nulls vs O-Granulator at `82406fce` (FeedbackChain, reverb + knee) and by the ARCHITECTURE Algorithm §3 law arm (bound, runaway guard, freeze / reset / feedback-0 hashes, block-size null), with head-clamp telemetry and click gates. D25 is a PASS/FAIL ≤ 40 % worst case with 4 chains + reverb behind the duty and calibration witnesses.

**Out of scope** (CONTEXT-2.4 § Out of scope): modulation of feedback / reverb / output destinations, voice modes, MPE, NE, tempo (2.5); Record-in, `<Slots>`, factory sources (2.6); the typical verdict on factory presets (Stage 4); feedback / freeze / reset UI and record-head visuals (Stage 3).

**Planner exclusions:**
- ARCH feedback fallbacks (a) raised-cosine write brush, (b) −1.5 dB/oct HF tilt, (c) Follow mode are never applied automatically (S6).
- The `1/sqrt(Σa)` chord fallback is a hands-on call (HANDS-ON-2.4 row 6).
- The parent's RV4 (outer mix 0) has no analogue here; TB1 covers the reverb-off path.

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/CONTEXT-2.4.md` | D22–D25, the Defaults block, the gate list. **Binding, not reopened.** D11–D21 still bind. |
| `stages/2-dsp/RESEARCH-2.4.md` | §1–§9 (cited **R§x**), Summary 1–12, Pitfalls **P1–P16**, the user's 2026-11-08 knee decision. Authoritative over ARCHITECTURE where they differ. This plan's PD2 / PD3 / PD4 / PD5 / PD12 / PD13 decide or correct research details (see PD text). |
| `stages/2-dsp/{SUMMARY,VERIFICATION}-2.3.md`, `PLAN-2.3.md` | What exists: harness 150/0, `--parity` 99/0 + GV 2/0, PF0 −149.3 dB / g₀ 0.9977, PN −110.9…−120.2 dB, hook names, row IDs, `--cpu23` (C23w 26.01 %, C23t 5.38 %), `ref_ns` 40 523 708, FZ1 pre-knee peaks 1.53 / 1.71 / 1.73. |
| `research/ARCHITECTURE.md` | D7, §6 (handoff, dual-read swap), §7 (MipTape), §16 (tap), §17 (feedback network), §18 (reverb, wet safety), Processing Chain + per-piece order, Algorithm §1 / §3 / §4, Implementation Risks → Feedback + MipTape, Stage-2 Gate Catalogue. |
| `parameter-spec.md` (LOCKED) + `.planning/params.tsv` | `freeze` (row 11), `output_gain` (13), `feedback`, `fb_low_cut`, `fb_high_cut`, `fb_saturation`, `fb_diffusion`, `reverb_mix`, `reverb_size`, `reverb_damp` (rows 186–193); identical ranges, skews and defaults to the parent (R§7). **Not edited.** |
| Parent at the pin: `git show "${PIN}:plugins/O-Granulator/<path>"`, `PIN=82406fce` (brace every variable) | `Source/dsp/Engine.cpp` `:69-73` (freeze bind), `:193`, `:764` (knob 0.95·fb), `:222-235` (`writeReverbAtomics`), `:240-251` (`updateChainCutoffs`), `:544-551` (tap probe), `:1603` (R31 tap excludes pan / g_out), `:1612-1663` (R96 wet guard → reverb → knee); `Source/dsp/Engine.h` `:114-116` (reverb atomics), `:250-256` (`injectFbNan`, `tapProbe`), `:317` (`getFbDelayForTest`); `Source/dsp/CaptureWriter.h:94-96`; `Source/dsp/OutputLaw.h:182-188`; `tests/render-harness/engine_gates_24b.h` `:137-138` (κ), `:190-244` (FZ2), `:470-499` (RV1–RV3); `.planning/stages/2-dsp/SUMMARY-2.4.md` (C4 18.57 vs C3w 17.80). **Never read the parent working tree.** |
| This plugin | `Source/dsp/{Engine.h,Engine.cpp,MipTape.h,SlotTape.h,SlotHandoff.h,SlotLoader.{h,cpp},GiConstants.h,GiRenderer.h,GiSpawner.h}`, `Source/PluginProcessor.{h,cpp}`, `Source/dsp/forked/{FeedbackChain.h,OutputLaw.h,ControlGrid.h,EngineConstants.h,FORK-MANIFEST.md}`, `tests/parity/{run.sh,parent_render.cpp,ParityRows.h}`, `tests/render-harness/{main.cpp,engine_gates_21.h,engine_gates_22.h,engine_gates_23.h,parity_gates_22.h,parity_gates_23.h,render_hashes.h,cpu_calib.h}`, `tests/golden/{renders-2.1,renders-2.2}.sha256`, `tests/golden/cpu-calib-2.3.txt`, `tests/fork-drift-check.sh`. Read only: `modules/effects/reverb-unit/cpp/ReverbUnit.h` (`:112-114` atomic bind, `:125` prepare, `:168` reset, `:176-197` process / running, `:428` clearTank), `modules/synthesis/grain-engine/cpp/BandlimitedReader.h:270-273` (`margin`). |

---

## Plan-phase decisions (resolve RESEARCH open items)

- **PD1 [user decision R§6.2; ordering is the planner's] Knee adoption ALONE in W1, attributed before any 2.4 engine code.**
  - T3 replaces the two `kWetCeil * std::tanh` statements in `Engine::mixPiece` (`Engine.cpp:1528-1529`) with `yL = wetKnee (yL); yR = wetKnee (yR);` (`forked/OutputLaw.h:182-188`, VERBATIM, = parent `OutputLaw.h:182-188`). Hook `legacyWetTanh` restores the exact two statements (same operands, same order). `wetSafetyOff` stays (identity; PF rows unchanged). `kWetCeil` stays in `GiConstants.h` (legacy hook only; comment updated).
  - **Attribution (parent P33 `--legacy-24` precedent).** RB1 (eg22) and FR1 (eg23) run their hash jobs under `legacyWetTanh`: renders-2.1 pinned 7/7 and renders-2.2 14/14 byte-identical. This is RESEARCH's "TB1" proof. The ID TB1 is re-used for the post-engine bypass row (PD15).
  - **Re-pin once.** `tests/golden/renders-2.4.sha256` is the shipping-path dump at T4, with the moved-line list in SUMMARY. All 14 are expected to move: kWetCeil·tanh is not identity anywhere, wetKnee is identity ≤ kKneeT.
  - From then on **TB0** (shipping == renders-2.4) is the true-bypass invariant. renders-2.1 / 2.2 are never re-recorded, and renders-2.4 is never re-pinned again in 2.4 (a move → S3).
  - **Separate commit C1** at T4 (knee + hooks + hash plumbing + TB0 / TB2 + renders-2.4 only), so the re-baseline is reviewable on its own (RESEARCH open item 4).
  - **Parity.** The PN residuals should fall from −110.9…−120.2 dB toward the PF0 floor (−149 dB). The new table `$S/pn-24.txt` becomes the 0.1 dB baseline for every later check; no threshold changes. GV rows must still pass. `kVoiceGainMeasuredDb` is **not** re-measured (that would be a constant change); GV0's new value is INFO.
- **PD2 [planner — the R§4.3 / P8 decision] Writer clamp: option (a), plus a convex envelope.**
  - Before the write, the writer clamps **its local copy** of the return: `rw = std::clamp (r, −kAgcCeil, kAgcCeil)` (`kAgcCeil` 0.7, `forked/EngineConstants.h:146`), counting `rClamps` (hook counter, INFO). `fbRet` itself is untouched, so the PB nulls compare the leaf's own return, and `FeedbackChain.h` stays VERBATIM.
  - Then `w = (1 − λ)·wc + λ·rw`. Non-finite → keep `wc`, `writeScrubs++` (ARCH §17). Else `w = std::clamp (w, std::min (wc, rw), std::max (wc, rw))`.
  - The envelope makes "a convex combination cannot grow" exact in float. Without it, rounding can land ~1 ulp above max(|wc|, |r|) and creep. So FL1 / FL1s gate at the exact bound `max (pristinePeak, 0.7f)` with **no tolerance**.
  - **Why (a), not (b).** The leaf bound is `|r| ≤ 1/d`. The AGC's 1 ms attack (`FeedbackChain.h:184-197`, `kAgcAttackS`) lets a sub-millisecond transient through at gain 1. So at `fb_saturation` < 6.2 % (d < 1.434) the ARCH §3 bound of 0.7 is false without a clamp. (b) restates DSP-04 as `max(pristine, 1/d)`, which is weaker than "never exceeds the limiter ceiling". The clamp is inert by construction at sat ≥ 6.2 % (default 20 %: `|r| ≤ 0.417`) and costs two min/max per sample.
  - Control: `rClampOff` + `injectReturn {slot 0, 10 s, 0.95}` on a 0.5-peak tape → the bound fails (FL1s).
- **PD3 [planner; refines R§1.2 step 2] Whole cell written at the grid tick.**
  - `kFbDelay == kCell == 32` (`forked/EngineConstants.h:43-45`). So at the tick at `t`, every return `r[n − 32]` for n ∈ [t, t + 32) is already in `fbRet`: the previous cell's pieces computed it.
  - The writer writes the **whole cell at the tick**, before any MIDI event or segment of that cell. For 32-aligned blocks this is identical to per-piece writes. For split cells (441, 4096, var) it removes the only order in which a read could see a partly written cell. So block-size invariance no longer depends on the clamp being feasible: forced crossings stay deterministic.
  - `g_a` stays 40 (PD9): up to 31 frames ahead of `h_n` are written before `n` is read, plus the 8-tap kernel reach.
  - `fbRet` becomes a fixed member `float fbRet[kGiStreams][2][kFbDelay]`, zeroed in `prepare`. The `std::vector<float> fbReturn` (`Engine.h:496`, `Engine.cpp:105`) is removed.
- **PD4 [R§3.3 adopted + planner refinement] Run condition, ramps, park.**
  - `runTarget_s = ! freeze && kaiser::ratesEqual (tape.rate, fs) && ! xfade[s].active && ! swapWaiting_s && (voiceTerm_s || returnPk_s > 1e-5)` (−100 dBFS). Latched **at the tick only**.
  - **Refinement:** `voiceTerm_s = gp.s[s].enable && audibleTarget (s, anySolo) && (any counted voice)`. ARCH §17 says "stream has a sounding voice". A disabled, muted or un-soloed stream has a zero tap, so a running head would erase its own copy toward 0 at rate λ while notes play on other streams. The return-decay term still writes the tail after notes end.
  - `returnPk_s` = per-cell peak of `|r_s|`, held with a 50 ms exponential release applied at the tick (the time constant is the planner's choice: it only decides when a silent loop parks).
  - `fbRamp` (target `feedback / 100`) and `runRamp_s` (target 1 / 0) are `ExactRamp`s over `rampCells20` (20 ms, D24), ticked at the tick and read per sample at the cell position (`value (c)`, as the chain reads its ramps). `λ_eff(c) = kFbKnob · fbRamp.value (c) · runRamp_s.value (c)` (`kFbKnob` 0.95, `EngineConstants.h:135`: the parent applies it to its knob, ours to λ, ARCH D7).
  - **Parked** ⇔ `fbRamp.settledAt (0.0f) || runRamp_s.settledAt (0.0f)` for the cell: no write, no advance, no clamp. Feedback 0 means parked, so the writer is **skipped**, never runs `1·wc + 0·r` (P5), and FL4 / TB0 hold bitwise.
  - The cell where the head becomes parked runs `settleRun` over `[h, h + margin(4))` (PD10), `parks++`.
  - **Freeze** = `runTarget` false (D24): λ_eff ramps to 0 in 20 ms and the heads stop. The chains keep running; their return is discarded (the writer is skipped). A `freeze` saved as on engages at the first tick (parent `Engine.cpp:69-73`): `prepare` resets every `runRamp_s` to 0 (heads start parked) and `fbRamp` to its target.
  - Hooks: `lambdaRampZero` (fbRamp steps in 0 cells), `runRampZero` (runRamp steps in 0 cells), `freezeHardStop` (on a freeze edge runRamp snaps to 0).
- **PD5 [R§3.4 adopted; widened by the planner] Every take waits for a parked head.**
  - At the tick, before `handoff.take (s)` (`Engine.cpp:568-612`): if `handoff.pendingDepth (s) > 0` and head s is not parked → `swapWaiting_s = true` (runTarget false) and the tape stays pending. It is taken at the first tick after the head parks (≤ 20 ms + 1 cell). `pending` holds ≤ 1 tape by `exchange`.
  - **Widened** from same-geometry takes to every take (Reset, SR rebuild, new file, recording). A head running on the old tape would otherwise write the new tape at a stale position. With feedback 0 the heads are always parked, so no 2.1 / 2.2 swap row and no TB0 job changes.
  - **Head positions.** A Reset take (`rebuild` && same generation && `ratesEqual (old->rate, p->rate)`) keeps `h`. Any other take (new file, SR rebuild) sets `h = region lo`, parked. The head never writes during the dual-read crossfade (`! xfade.active`).
  - The existing crossfade skip (`Engine.cpp:572-573`) stays, so a Reset requested mid-crossfade queues (RS4).
  - Hook `resetHardSwap` (CK4 control): a Reset take uses `x.len = 1` (the grains switch tape at the take, no crossfade).
- **PD6 [R§3.4 adopted] Reset API.**
  - **`SlotLoader::requestReset (int slot)`** (message thread; `slot == −1` = all four): a rebuild from the slot's pristine at `sessionRate`. It bypasses the "already at this rate" early-out (`SlotLoader.cpp:402`) and dedupes on an in-flight reset of the same slot (a new `resetInFlight` flag in `SlotRecord`, cleared in `publishResult` / `jobFailed`).
  - The job is the existing rebuild `Job` (`rebuild = true`, same generation, `SlotLoader.cpp:96-97`), so `takeTapes` starts the existing 20 ms dual-read crossfade with ratio 1 (`Engine.cpp:589-600`).
  - **`SlotLoader::resetSync (int slot)`** (`OGI_TEST_HOOKS`, the `loadSync` pattern `:534`): builds on the calling thread and publishes. Deterministic rows call it from `Session::at` events at absolute multiples of 32 (event-split blocks), so the take tick is block-size independent.
  - **Processor:** `void requestReset (int slot)` (message thread; Stage 3 binds the native function) and, under `OGI_TEST_HOOKS`, `bool resetSlotSync (int slot)`.
  - Reset is not a parameter, no listener writes a parameter, and there is no `<Engine>` key: the working copy is not saved; reload = pristine.
- **PD7 [R Summary 3 / R§7 adopted] Chain drive.**
  - `FeedbackChain chain[kGiStreams]` (≈ 133 KB each, fixed arrays; the Engine is heap-held, `PluginProcessor.h:233`), with `prepare (fs)` in `Engine::prepare`.
  - `chain[s].process (l, r, n, cellOff, knob, diffRamp, satDRamp)` runs every piece for every stream, frozen or not (parent rule), on the tap. It writes `fbRet[s][ch][cellOff + i]`.
  - **Knob = unity:** `ExactRamp unityKnob` is reset to 1.0f and never ticked, so `value (c)` is exactly 1.0f and `l[i] * 1.0f` is bitwise `l[i]` (`FeedbackChain.h:218-219`). The loop gain lives in λ only (P1).
  - Hook `chainKnobParity`: the chains read `parityKnob` (target `kFbKnob · feedback / 100`, 20 ms), the parent drive (`Engine.cpp:193, 764`). PB rows only.
  - Cutoffs use the parent `updateChainCutoffs` shape (`Engine.cpp:240-251`): log2 ramps of raw Hz (the 2.3 raw-Hz rule), `setCutoffs` (tan) only on change, LP at most `kFbNyquist · fs`. `diffRamp` = `fb_diffusion / 100`; `satDRamp` = `1 + kSatSpan · fb_saturation / 100`. All 20 ms, ticked at the tick.
  - The leaf is driven, never edited (drift 0 / 13). Hook `injectFbNan {slot, atSample}` sets the leaf's `testInjectAt` (`FeedbackChain.h:220-223`) in the first piece ≥ atSample.
- **PD8 [R§1.3 adopted] Tap.**
  - `tap_s[c] = (Σ_k a_k[c] · gFb_kS(c) · bus_kS[c]) · min (levelLin_s(c), 1) · aud_s(c) / max (1, Σ_k a_k[c])`, with `a_k = envBuf[k][c]` (the amp env). `g_out`, pan and voice gain are not in the tap (parent R31, `Engine.cpp:1603`).
  - It is accumulated in the existing k-loop of `mixPiece` (`Engine.cpp:1472-1488`) as **separate statements before `bl[c] = 0.0f`** (`:1485`), gated on `envBuf[k][c] > 0.0f`, into its own `tapAcc[s][2][kCell]` and `envSum[s][kCell]`. The mix statements (`:1479-1484`) are not touched (P11). An inaudible stream's bus clear (`:1454-1462`) gives tap 0, consistent with `aud = 0`.
  - `CellLerp gFbLerp[k][s]` with target `fbTapGain (fbTapS1 (…))` (`forked/OutputLaw.h:199-212`, the parent's arguments, R§1.3) is set at the same sites and from the same `nEff` as `lawTarget` (`Engine.cpp:827`, `:938`).
  - Hooks: `sumTap` (÷1; FL2 control), `fbArchLaw` (`fbTapGainArch`; FL8 control), `tapProbe` / `retProbe` (per-sample L / R of one slot into a harness buffer; PB rows).
- **PD9 [R§2 adopted; corrects ARCH Algorithm §4's g_a = 16] Head clamp.**
  - New pure header `Source/dsp/GiHeadClamp.h`: `clampToHead (p0Drawn, dir, rate, len, level, lo, hi, reach, head) → { p0, forced }`.
  - `g_b = MipRing<4>::margin (level)` (9 / 80 / 222 / 506 / 1074, `BandlimitedReader.h:270-273`). `g_a = kCell + 8 = 40`: ARCH's 16 misses the cell written ahead (PD3).
  - Relative sweep Δ = (dir·rate − 1)·len. Forbidden d₀ ∈ [min(0, −Δ) − g_b, max(0, −Δ) + g_a] mod W, computed in absolute tape coordinates as ≤ 2 arcs (the head wraps hi_h → lo at most once over a grain's life). A head outside [lo, hi) (the region moved since its last wrap) → plain absolute distance.
  - Intersect with the existing region fit (`Engine.cpp:1257-1270`). Take the feasible p0 nearest the drawn one; ties go behind the head. No feasible point, or `reach ≥ W` (the wrap-read branch) → keep the drawn p0 and `forcedCrossings++`.
  - The clamp consumes **no draws** and is **skipped while the head is parked**, so every derandomised parity row and every fb-0 render is untouched. It uses the post-write head state of the tick (PD3); an onset at offset o in the cell sees `h + o`.
  - Hooks:
    - `headClampOff` (HC1 control);
    - `headReadScan`: per render segment and live grain, the linear grain path vs the linear head path, as an exact interval test of relative distance ∈ [−margin(level), 40]. It counts `headScanViolations` for grains spawned with a running head and `parkedSpawnCrossings` (INFO) for grains spawned while parked.
- **PD10 [R§3.2 adopted] MipTape settle + hash.**
  - `MipTape::settleRun (pos, n)`: `writeRun`'s child loop (`MipTape.h:144-159`) with level 0 untouched. It runs over `[hi_h, hi_h + margin(4))` at every wrap and `[h, h + margin(4))` at every park, clipped to the stored span. That is ≤ 1074 parent frames per call, allocation-free. It never runs on a pristine build, so `flushTail` and the pristine hash are unchanged.
  - Writer path: `MipTape::writeRun (h, n, L, R)` on a ≤ 32-frame scratch; a wrap inside a cell splits it into two runs. The header comment (`:61-62`) is updated: `writeRun` / `settleRun` are audio-thread safe (no allocation; `written` stays a max).
  - `hashForTest()` (`OGI_TEST_HOOKS`): FNV-1a 64 over every level, both channels, the full stored span. Plus `levelPeak (level)`.
  - Hook `settleOff` (MS1 control).
- **PD11 [R§6.1 adopted] Reverb, wet guard, knee, output gain.**
  - `ReverbUnit reverb` (reverb-unit 1.2.0, unchanged since the pin, linked at `CMakeLists.txt:86-87`), bound once to 7 engine-owned `std::atomic<float>` (parent `Engine.h:114-116`):
    - fixed: enabled 1, predelay 20 ms, mod 0.2, shimmer 0 (new constants in `GiConstants.h`);
    - size / damp / mix written **only at the tick** by `writeReverbAtomics` (parent `Engine.cpp:222-235`, minus the 2.5 matrix lines; the mapping statements are the parent's).
  - `reverb.prepare (fs, kCell)` in `Engine::prepare` (it allocates: host thread).
  - `mixPiece`:
    1. the existing y statements, unchanged, stored into `wet[2][kCell]`;
    2. the R96 wet guard (non-finite → 0, parent `:1612-1634`);
    3. `reverb.process` over the piece through a non-owning `juce::AudioBuffer<float>` view of `wet` (the parent statement);
    4. the knee (PD1; `legacyWetTanh` / `wetSafetyOff` act here);
    5. `outL[c − c0] = y · og` as its own statement.
  - No outer dry path (ARCH §18). Mix 0 is a bitwise bypass: `ReverbUnit::process` returns before touching the buffer while not running (`ReverbUnit.h:176-197`). Mix → 0 fades over 50 ms, then stops.
  - Hooks: `reverbHardCut` (RV2 control: `reverb.reset()` at the mix-0 edge), `injectPreKnee {atSample, value}` (CL2 control); counters `preKneePeak`, `kneePeak` (max |y| before and after the knee, before og).
- **PD12 [R§4.2 adopted + planner corrections] Leaf nulls (PB / PR), no parent edits.**
  - **Plumbing.** The parent at `82406fce` is built out of tree by the unchanged `tests/parity/run.sh` from `git archive`. Our side changes:
    - `ParityRows.h`: new `Kind::fbNull`, `Kind::revNull`, handled only by pg24;
    - `parent_render.cpp`: columns appended at the **end** of each line (2.3 PD11);
    - new `parity_gates_24.h` (`pg24`), reusing pg22 / pg23 helpers in the same TU.
  - Every 2.2 / 2.3 parent `.f32` stays byte-identical to the T1 copy (`cmp`).
  - **PB (FeedbackChain).** PN01 frozen geometry (`kTf`, `parent_render.cpp:170-171`), 1 voice, s1, matPeak 0.25, parent `feedback` f > 0.
    - Parent: frozen, so no capture push (`CaptureWriter.h:95-96`) and its output is unchanged while its chain runs on the live tap. The tap comes from `hooks.tapProbe` (`Engine.h:255-256`), the return from `getFbDelayForTest()` (`Engine.h:317`) read after every block, with PB rows rendered in **32-sample blocks**.
    - Ours: freeze on from the first tick (no writes, chains warm), `chainKnobParity`, `tapProbe` / `retProbe`.
    - **PB0** precondition: tap null ≤ −130 dB (it rests on PF0's −149.3 dB read identity); fitted gain `g_PB`. PB0 > −130 dB → every PB row reports `BLOCKED (PB0)` = FAIL → S1.
    - **PB1–PB12:** return null ≤ −90 dB after LS alignment (n ≥ 256), `|g_row / g_PB − 1| ≤ 1e-4`, ref ≥ −70 dBFS.
    - **Row set (corrected:** R§4.2's sat 20 and diffusion 0 rows duplicate the default-filter row):
      - PB1–3 = f 25 × (low, high) ∈ {(20, 20000), (80, 12000), (2000, 1000)} Hz; PB4–6 = the same at f 100 (PB5 = f 100 at the default cutoffs). All at sat 20, diffusion 0.
      - PB7–9 = PB5 at sat {0, 50, 100}.
      - PB10–12 = PB5 at diffusion {25, 50, 100}.
      - PB9 (sat 100) asserts our chain `env > 0.7` was reached (AGC engaged).
    - **PBd:** the xcorr peak of our vs the parent's return arrays sits at lag 0, and our written layer vs our tap at lag 32 (the z^-32).
    - **Controls** (ours only; `parentOf` reuses the parent render):

      | Control | Row + change | Fires when |
      |---|---|---|
      | PBc1 | PB5, low_cut × 2^(1/12) | null > −90 dB |
      | PBc2 | PB1, `chainKnobParity` off | the band fails (the PFc6 analogue: alignment would absorb a scalar knob; the null result is recorded) |
      | PBc3 | PB11, diffusion 51 | null > −90 dB |
      | PBc4 | PB9 + the leaf's `testAgcLegacy` | null > −90 dB |
      | PBc5 | PB5, return read at lag 31 | null > −90 dB |
  - **PR (reverb + knee).** PN01 geometry with the parent's `reverb_mix` > 0. The parent output is `knee(reverb(wet))·og` with `mix 100` (`parent_render.cpp:110`).
    - Our output is compared to the parent output directly: no hooks, no `wetSafetyOff` (the knees match now). Null ≤ −90 dB, band `|g / g₀ − 1| ≤ 1e-4` vs PF0's g₀.
    - **PR1–12** = mix {25, 100} × size {0, 50, 100} × damp {0, 100} in that nesting order (PR9 = mix 100, size 50, damp 0), matPeak 0.25.
    - **PRk** (knee row): matPeak 0.4, mix 100, size 100, damp 50. The parent peak must exceed 0.6, else INVALID = FAIL; one stimulus step is allowed (stream level +6 dB on both sides, recorded).
    - **Controls:** PRc1 = PR9 with our size 51 → null > −90 dB. PRc2 = PRk + `legacyWetTanh` → the null or the band fails.
- **PD13 [R§5 adopted; planner fixes the CK4 control] Click gates (D24, QUAL-01).**
  - **Metric:** κ = max |Δ²x| / RMS over a window, max over L / R (port of parent `engine_gates_24b.h:137-138`). Pass ≤ **2.0×** the static same-motion control.
  - **Hard controls** must read **> 2.0×** (recorded). A hard control ≤ 2.0× means the stimulus cannot see the click: the row's control is NOT FIRED = FAIL. One stimulus fix is allowed, recorded; still not firing → S6.
  - **Material:** a 440 Hz sine tape, spray 0, scan 0 (parent FZ2 shape). CK3 uses region 300 ms, spray 100 %, and a window 4 s after the park.

  | Row | Event | Static control | Hard control |
  |---|---|---|---|
  | CK1 | freeze on at a tick, fb 100 %, grains spanning the head | same render, no freeze | `freezeHardStop` |
  | CK2 | feedback 0 → 100 % at a tick | fb 100 % held | `lambdaRampZero` |
  | CK3 | notes off → return decays → park; then a new note | same notes, fb 0 | `runRampZero` |
  | CK4 | Reset after 5 s at fb 100 % | **corrected:** a freeze pulse spanning the same park → take → fade → resume interval, with no Reset. R§5 said "frozen, no reset"; the pulse gives the same head motion, so only the tape switch differs. | `resetHardSwap` |

  - Reverb transitions are RV2 / RV3 (the parent recipe).
- **PD14 [D23 + R§6.2] Ceiling rows.**
  - Measured at the knee output before `output_gain` (`kneePeak`; `output_gain` excluded, D23): ≤ float(A) = **1.99526231** (`kKneeA`, `EngineConstants.h:148`). Pre-knee peak INFO.
  - **CL1** uses the FL1 render, with reverb mix 100 / size 100 added so the reverb's peak gain is inside the row. Control: `wetSafetyOff` → output peak > float(A). The pre-knee peak exceeding A is asserted live; otherwise INVALID = FAIL.
  - **CL2** uses the carried FZ1 stimulus (`eg23::stressParams()`, `engine_gates_23.h:855`) at 44.1 / 48 / 96 k. Its 2.3 pre-knee peaks (1.53 / 1.71 / 1.73) are below A, so a knee-off control cannot fire on it. Control: `injectPreKnee {0.5 s, 8.0}` + `wetSafetyOff` → > float(A); the same injection with the knee on stays ≤ float(A).
- **PD15 [planner; refines R open item 3] Row IDs and targets.**
  - **New prefixes:** TB, FL, CL, HC, MS, RS, SR, CK, RV, FA4 (harness); PB, PR (parity); C23w / C25w / C25t / C25r / C25s / C25w96 (`--cpu25`).
  - **Collision check:** against eg21 (Q, T, K, L, P, S, V, M, E, H, N, A), eg22 (RB1, W0*, QS*, LF1, CO*, MX1, V2s, FS*, SC2, ST*, A2, NB1, V3s, N2s), eg23 (F*), main.cpp (G*, D*, SRC1, B1, M1, R1, S1a, H1) and parity (PN, PS, SC, GV, PF*): no collision (`SR1` ≠ `SRC1`). The `sort | uniq -d` duplicate set must equal the 2.3 log's.
  - **Default harness: 150 → 152** after W1 (TB0, TB2), **→ 181** after W4. The 29 W4 rows are TB1; FL1, FL1s, FL2–FL8 (9); CL1–2; HC1–3; MS1; RS1–4; SR1; CK1–4; RV1–3; FA4. Controls are inline (`ctrl FIRED`), not rows.
  - **`--parity`: 99 → 133.** PB0 + PB1–12 + PBd + PBc1–5 (19) + PR1–12 + PRk + PRc1–2 (15). `--rows GV` 2 / 0 unchanged.
  - **`--cpu25`:** 2 verdict rows (C25w ≤ 40 %, C25t ≤ 12 %) + INFO lines.
  - RESEARCH estimated ≈ 187 / ≈ 132: it counted controls as rows and had no PBd. SUMMARY records the exact counts.
- **PD16 [R§7 + 2.3 PD10 precedent] Read set.**
  - `readSet24()` (`engine_gates_21.h`, after `readSet23()` at `:263`) = `readSet23()` + `feedback, fb_low_cut, fb_high_cut, fb_saturation, fb_diffusion, freeze, reverb_mix, reverb_size, reverb_damp` = **162** bound, **99** unread.
  - N2 (eg21 `:2071-2098`), N2s (eg22 `:1443-1470`) and FN3 (eg23 `:332-359`) switch to it with the literals 162 / 99. IDs, stimuli and thresholds are unchanged; the titles are updated.
  - New **FL7**: the same unread randomisation with the loop active. RESEARCH's "N2-24 / retire N2-23" is realised as this switch + FL7 (the 2.3 FN3 precedent); nothing is retired.
- **PD17 [D25, R§9; 2.3 PD2 / PD13 method] `--cpu25`.**
  - `eg24::cpu25()` in `engine_gates_24.h`, run alone, with its own exit code (= FAIL count); never part of the default rc. `cpu_calib.h` and `tests/golden/cpu-calib-2.3.txt` are **not edited** (`ref_ns` 40 523 708, frozen in 2.3 T2).
  - One process, in order. Each config is bracketed by `calibNs()`; a slowed-core or contended NOVERDICT = FAIL.
    1. bypass;
    2. **C23w**: the 2.3 worst case on the 2.4 engine, fb 0, mix 0 (the same-session reference);
    3. **C25w**;
    4. **C25t**;
    5. C25r;
    6. C25w96.
  - **C25w** = C23w + `feedback` 100, `fb_diffusion` 100, `fb_saturation` 20 (default), cutoffs default, `reverb_mix` 100, `reverb_size` 100, freeze off; 16 notes held, so the run condition holds.
    - Asserted per measured window: **writes > 0 on all 4 slots** (P13), live ≥ 240, duty ≥ 0.9.
    - Verdict **≤ 40 %** PASS/FAIL: the final PERF-02 worst case.
  - **C25t** = the 2.3 C23t proxy (PLAN-2.3 PD1) unchanged: feedback 0, reverb at its default mix 0. Verdict **≤ 12 %**, live ∈ [60, 90].
  - **INFO lines:**
    - **C25r:** C25w + async `requestReset (0)` every 200 ms of rendered audio. Prints `xfadesStarted` and Δ vs C25w: the dual-read fade cost (D25).
    - **C25w96:** C25w at 96 k / 128.
    - **C25s:** C23w@2.4 vs T2's C23w@2.3, each scaled by `ref_ns / calibBefore`; flag > +0.6 points (the fb-0 restructure cost).
    - **Δ = C25w − C23w** (projection +2–4 points, C25w ≈ 28–30 %, R§9).
- **PD18 [D21 + 2.3 PD9 carried] Lever ladder** (T18, only on a valid-witness FAIL).
  - Lever 1 (Hermite) is never applied automatically.
  - Levers 2 → 3 are automatic. Each is kept only if all of these hold; otherwise it is reverted (`git restore` of its files) and recorded:
    - TB0 14/14;
    - the legacy dump == renders-2.2;
    - FL5 0 differ;
    - every PN / PF / PB / PR residual unchanged.
  - Still FAIL → S2.
- **PD19 [planner] Test hooks** (all under `OGI_TEST_HOOKS`; the shipping build compiles none; `EngineTestHooks`, `Engine.h:196`):
  - **Switches:** `legacyWetTanh`, `chainKnobParity`, `sumTap`, `fbArchLaw`, `lambdaRampZero`, `runRampZero`, `freezeHardStop`, `resetHardSwap`, `writeScrubOff`, `rClampOff`, `settleOff`, `headClampOff`, `headReadScan`, `runIgnoreRate`, `reverbHardCut`.
  - **Injections:** `injectFbNan {slot, atSample, done}`, `injectReturn {slot, atSample, value, done}`, `injectPreKnee {atSample, value, done}`.
  - **Probes:** `tapProbe` / `retProbe` (`float*` + length + slot).
  - **Counters:** `writes[4]`, `maxWritten[4]`, `writeScrubs`, `rClamps`, `settles`, `parks`, `swapWaits`, `forcedCrossings`, `headScanViolations`, `parkedSpawnCrossings`, `preKneePeak`, `kneePeak`. `resetCounters()` zeroes the new counters.
  - **Accessors:** `Engine::getHeadForTest (s)`, `Engine::getChainForTest (s)`, `MipTape::hashForTest()` / `levelPeak()`.
  - `SlotLoader` gains `resetSync`; the processor gains `resetSlotSync`.
- **PD20 [planner; deviation from 2.3's "nothing builds until T8"] Bypass checkpoint after every W2 task.**
  - After each of T5–T10, the runner builds the consoles (BG) and runs `$H --dump-render-hashes "$S/ck-<T>.sha256"` and `$H --dump-render-hashes-legacy "$S/ck-<T>-legacy.sha256"`.
  - Both must be byte-identical to `renders-2.4.sha256` / `renders-2.2.sha256`. A move is attributed to the task that just landed → S3 before the next task starts. The full harness runs only at T15.
  - **Why:** W2 touches the bypass path five times (wet storage, tap statements, chain / return plumbing, writer skip, spawn clamp). In 2.3 there was one shape change, and a single FR1 sufficed.
- **PD21 Housekeeping.**
  - Version stays **0.1.0** (I/O unchanged, so the Logic AU I/O cache is unaffected); **no tag**.
  - `CMakeLists.txt` **not edited**: the new Source files `GiRecordHead.h` and `GiHeadClamp.h` are headers, every test file is a header, and `SlotLoader.cpp` is already compiled.
  - Not edited: `FeedbackChain.h`, `OutputLaw.h`, `GrainFx.h`, `Grain.h`, `EngineConstants.h`, `GiRenderer.h` (unless T18 fires), `cpu_calib.h`, `parity_gates_22.h`, `parity_gates_23.h`, `run.sh`. `ControlGrid.h` (ADAPTED) is re-hashed (columns 5 / 7).
  - No new parameters and no `<Engine>` keys; param-dump byte-identical.
  - **Two commits**, both path-scoped: C1 (T4) and C2 (T21).
  - HANDS-ON-2.4 is written and deferred post-Stage-3 with 2.1–2.3 (user override 2026-11-08).
  - The Standalone is rebuilt explicitly with `OuariconGranulatorInstrument_Standalone` (the exact phony target in `build/build.ninja`, verified at plan time) before any Standalone check (P16).

---

## Tasks

**Roles.**
- **[agent]** = dsp-agent writes files (no Bash).
- **[run]** = build/gate runner: commands, builds, validators, hash refreshes, commits.
- **BG** = `run_in_background`, log to `$S/<name>.log`. `$S` = the executing session's scratchpad.
- `$H` = `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness`.
- Paths are relative to `plugins/O-GranulatorInstrument/` unless they start with `build/`, `scripts/`, `modules/` or `~`.

**Audio-path rules (every engine task).**
- No locks (except the PD3-2.2 scale try-lock), no allocation, no `shared_ptr`, no vector growth; fixed arrays only.
- `ScopedNoDenormals` per block. No listener writes a parameter.
- Reset / SR rebuild run off-thread and land through `SlotHandoff` (`pending[s].exchange`, the SPSC retire ring).
- `tan` / `exp2` run only on a cutoff change at the tick. Every ramp is read per sample at its cell position.

### Wave 0 — pre-flight, baselines, CPU reference on the unmodified engine

**T1. Pre-flight + baselines** [run]
- Files: none. Output to `$S/preflight.txt`.
- Depends on: none.
- Done when (any mismatch → STOP and ask):
  - **Late changes:** `git log --oneline 9fc9289c..HEAD -- plugins/O-GranulatorInstrument/Source plugins/O-GranulatorInstrument/tests plugins/O-GranulatorInstrument/CMakeLists.txt` is empty (it was empty at plan time). If not, STOP and re-check the mapped tasks:

    | Changed path | Tasks |
    |---|---|
    | `Engine.{h,cpp}` | T3, T6–T9 |
    | `MipTape.h` | T8 |
    | `SlotLoader.*`, `PluginProcessor.*` | T10 |
    | `forked/ControlGrid.h`, `FORK-MANIFEST.md` | T5 |
    | `tests/parity/*` | T11 |
    | `tests/render-harness/*` | T3, T5, T12–T14 |
  - **Git state:** `git branch --show-current` = `main`; record `git status --short`. Foreign paths are **never staged**: `modules/registry.yaml`, `.claude/agent-memory/*`, `.claude/compaction-snapshot.md`, `plugins/O-GrainScatter/.planning/improvements/adaptive-grain-length/nielsen-dafx26.txt`, and any other session's files.
  - **Contracts and pin:**
    - `parameter-spec.md` sha256 (file + body) = STATUS `contract_checksums`;
    - `git cat-file -e 82406fce`;
    - `git diff --quiet 82406fce HEAD -- plugins/O-Granulator/Source/dsp modules/synthesis/grain-engine modules/tuning/pitch-collections modules/effects/reverb-unit scripts/param-dump` (empty at plan time);
    - `shasum -a 256 Source/dsp/forked/{FeedbackChain,OutputLaw,ControlGrid}.h` = FORK-MANIFEST column 5 (`49b29942…`, `a550ff9d…`, `ba86d4d3…`; verified at plan time).
  - **Hands-on:** HANDS-ON-2.1 / 2.2 / 2.3 are deferred by the user (D14 override); log it, not a gate.
  - **Baseline (BG):**
    - `ninja -C build O-GranulatorInstrument-render-harness O-GranulatorInstrument-param-dump`;
    - `$H` = **150 PASS / 0 FAIL** → `$S/h-23.log`; record its duplicate-ID set (`sort | uniq -d`) → `$S/dup-23.txt`;
    - param-dump → `$S/pd-23.txt`, byte-identical to `.planning/params.tsv`;
    - `$H --dump-render-hashes "$S/r23-base.sha256"` byte-identical to `tests/golden/renders-2.2.sha256`;
    - `tests/fork-drift-check.sh` 0 drift.
  - **Parity baseline (BG, after the build):**
    - `tests/parity/run.sh "$S/parity"` rc 0;
    - `$H --parity "$S/parity/out"` **99/0** → `$S/par-23.log`; `$H --parity "$S/parity/out" --rows GV` **2/0**;
    - PN residual lines → `$S/pn-base.txt` (expect −110.9 … −120.2 dB); PF / PFc residual lines → `$S/pf-base.txt`;
    - `cp -R "$S/parity/out" "$S/parity-base-out"`.
  - **Standalone target:** `ninja -C build -t targets all | grep -c '^OuariconGranulatorInstrument_Standalone: phony'` = 1.

**T2. C23w on the unmodified 2.3 engine (C25s reference, PD17)** [run]
- Files: none (log `$S/cpu23-base.log`).
- Depends on: T1, with **every T1 background job finished**. **No `Source/` file is edited before this task is done.**
- Run **alone** (no ninja, parity, pluginval or Logic): `$H --golden-dir tests/golden --cpu23` (BG).
- Done when:
  - the C23w line carries calib before / after vs `ref_ns`, duty ≥ 0.9 and live ≥ 240, and no NOVERDICT (re-run up to 3 times; a third NOVERDICT → STOP and report the environment);
  - `c23w_23_pct`, its calib before / after and the C23t line are copied into `$S/preflight.txt` for SUMMARY (VERIFICATION-2.3: C23w 26.01 %, C23t 5.38 %);
  - `git diff --stat -- Source` is empty.

### Wave 1 — knee adoption + re-pin, attributed (commit C1)

**T3. Knee swap + `legacyWetTanh` + hash plumbing + TB0 / TB2 (PD1)** [agent]
- Files: `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`, `Source/dsp/GiConstants.h`, `tests/render-harness/render_hashes.h`, `tests/render-harness/engine_gates_22.h` (RB1 only), `tests/render-harness/engine_gates_23.h` (FR1 only), `tests/render-harness/engine_gates_24.h` (new), `tests/render-harness/main.cpp`.
- Depends on: T2.
- **`Engine::mixPiece`** (`Engine.cpp:1522-1530`): the two `kWetCeil * std::tanh` statements become `yL = wetKnee (yL); yR = wetKnee (yR);`.
  - Under `OGI_TEST_HOOKS`: `wetSafetyOff` → identity (unchanged); `legacyWetTanh` → the exact former statements.
  - The shipping statements are the two `wetKnee` calls only. Nothing else in `mixPiece` changes in this task.
  - `EngineTestHooks` gains `bool legacyWetTanh = false;` with a comment.
  - `GiConstants.h:66`: the `kWetCeil` comment becomes "legacy wet safety (2.1–2.3); the `legacyWetTanh` hook only (PLAN-2.4 PD1)".
  - The Engine header comment (`Engine.h:76-80`) is updated: wet safety = the parent D9 knee `wetKnee`.
- **`render_hashes.h`** (the declared single extension point `prepareJob`, `:55`):
  - add `struct Overrides { bool legacy = false; P extra; std::vector<std::pair<juce::int64, P>> events; }` and a namespace-scope `rh::overrides`;
  - `prepareJob` applies `extra` after the pins, schedules `events` through `Session::at` (absolute multiples of 32), and sets `hk().legacyWetTanh = overrides.legacy`;
  - `dump (file, legacy)` renders under `overrides.legacy = legacy` and restores the previous overrides.
- **RB1** (`engine_gates_22.h:163-210`) and **FR1** (`engine_gates_23.h:366-395`): compute their hashes with `rh::overrides.legacy = true` (scoped). Row IDs, thresholds and controls are unchanged; titles gain "(legacyWetTanh, PLAN-2.4 PD1)".
- **`engine_gates_24.h`** (new, `eg24::runAll()`), header listing every 2.4 row (filled in by T12). Rows now:

  | ID | Row | Stimulus | Pass | Negative control |
  |---|---|---|---|---|
  | TB0 | Shipping renders == renders-2.4 (PD1) | `rh::computeAll()`, 14 jobs, no overrides | 14/14 lines == `renders-2.4.sha256` | N1-default + `legacyWetTanh` → hash differs |
  | TB2 | Knee is identity ≤ kKneeT (R§6.2) | **identity half:** N1-default and P2-default jobs; the `wetSafetyOff` render's peak ≤ kKneeT is asserted (else INVALID = FAIL; one step allowed: `s1_level` −6 dB on all renders, recorded). **Knee half:** `eg23::stressParams()` at 48 k, 2 s (pre-knee peak > 0.6 asserted) | identity half: shipping ≡ `wetSafetyOff` bitwise. Knee half: every sample with \|pre\| ≤ kKneeT is bitwise equal; every \|out\| ≤ float(A); \|out\| < \|pre\| above kKneeT | `legacyWetTanh` → the identity-half renders differ |
- **`main.cpp`:**
  - `#include "engine_gates_24.h"` after `engine_gates_23.h`;
  - `eg24::runAll();` after `eg23::runAll();`;
  - `--dump-render-hashes-legacy <file>` → `rh::dump (file, true)`; `--dump-render-hashes` stays the shipping path;
  - banner and usage updated.
- Done when the agent reports the before / after text of `Engine.cpp:1522-1530`.

**T4. Build, attribution proof, re-pin, parity re-baseline, commit C1 (PD1)** [run]
- Files: `tests/golden/renders-2.4.sha256` (new).
- Depends on: T3.
- Run (BG): `ninja -C build O-GranulatorInstrument-render-harness O-GranulatorInstrument-param-dump`. Then, in order:
  1. **Attribution:** `$H --golden-dir tests/golden --dump-render-hashes-legacy "$S/r24-legacy.sha256"` → `cmp` with `tests/golden/renders-2.2.sha256` must be **identical**, and its 7 `-pinned` lines must equal `renders-2.1.sha256`. Otherwise → **S3** (the knee hook does not restore the old statements). Nothing is pinned before this passes.
  2. **Re-pin:** `$H --dump-render-hashes "$S/r24.sha256"` → `cp` to `tests/golden/renders-2.4.sha256`. The moved-line list (`diff` vs renders-2.2, expected 14/14) → `$S/moved-24.txt`.
  3. **Harness:** `$H` → **152 PASS / 0 FAIL** (150 carried + TB0 + TB2), 0 `NOT FIRED`, RB1 7/7 and FR1 14/14 under legacy → `$S/h-24a.log`. Any carried row other than RB1 / FR1 failing → **S7**.
  4. param-dump byte-identical to `$S/pd-23.txt`; `tests/fork-drift-check.sh` 0 / 13 (no leaf touched).
  5. **Parity (BG):** `tests/parity/run.sh "$S/parity"` (incremental; parent unchanged); `$H --parity "$S/parity/out"` **99/0**; `--rows GV` **2/0**.
     - PN residual lines → `$S/pn-24.txt`, expected well below the T1 values (toward −149 dB). A PN row worse than its T1 value by > 0.1 dB → **S7**.
     - PF / PFc lines == `$S/pf-base.txt` to 0.1 dB (PF uses `wetSafetyOff`, so they are untouched).
     - GV0's new value is recorded (INFO).
  6. **Commit C1:**
     - `git branch --show-current` (= `main`) and `git status --short` **immediately before**. `git status --short -- plugins/O-GranulatorInstrument` must list exactly the T3 files + `renders-2.4.sha256` (plus planning files the user has not committed: STOP and ask if any).
     - `git add` the two new paths, then `git commit -- <the exact T3 paths> plugins/O-GranulatorInstrument/tests/golden/renders-2.4.sha256` with the message `phase: O-GranulatorInstrument 2-dsp/2.4 knee adoption (wetKnee, legacyWetTanh attribution: renders-2.1 7/7 + renders-2.2 14/14 byte-identical under legacy; renders-2.4.sha256 pinned, 14/14 moved; harness 152/0; parity 99/0, PN <new range>)`.
     - `git log -1 --stat` shows only those paths. **No tag.**
- Done when C1 exists and every check above passed.

### Wave 2 — engine (agent writes in signal order; bypass checkpoint after each task, PD20)

**Checkpoint (after each of T5–T10)** [run]: `ninja -C build O-GranulatorInstrument-render-harness` (BG). Then `$H --dump-render-hashes "$S/ck-<T>.sha256"` must be byte-identical to `tests/golden/renders-2.4.sha256`, and `$H --dump-render-hashes-legacy "$S/ck-<T>-legacy.sha256"` byte-identical to `renders-2.2.sha256`. rc 0, no new warnings in plugin sources. A move → **S3**, attributed to that task, before the next task starts.

**T5. ControlGrid 9 global rows + manifest + read-set switch (PD16, P12)** [agent; the runner re-hashes]
- Files: `Source/dsp/forked/ControlGrid.h`, `Source/dsp/forked/FORK-MANIFEST.md`, `tests/render-harness/engine_gates_21.h`, `tests/render-harness/engine_gates_22.h`, `tests/render-harness/engine_gates_23.h`.
- Depends on: T4.
- **`ControlGrid.h`**, ADAPTED region only (`:170-469`; `:38-165` stays byte-identical):
  - `GiParamPointers` (`:194-307`) gains `feedback, fbLowCut, fbHighCut, fbSaturation, fbDiffusion, freeze, reverbMix, reverbSize, reverbDamp`, bound in `bind` (`:251`) by their IDs.
  - `GiGridParams` (`:316-469`) gains, with the spec defaults: `float feedback = 0.0f` (0..1), `fbLowCutHz = 80.0f`, `fbHighCutHz = 12000.0f`, `fbSaturation = 0.2f`, `fbDiffusion = 0.0f`, `bool freeze = false`, `float reverbMix = 0.0f`, `reverbSize = 0.5f`, `reverbDamp = 0.5f`.
  - `latch` (`:372`): `readPct` for feedback / fb_saturation / fb_diffusion / reverb_*; `read (fbLowCut, 20, 2000)` and `read (fbHighCut, 1000, 20000)` as raw Hz from the skewed ranges (never inverted by hand); the bool read for `freeze` (the `enable` pattern).
  - Header comment (`:170-190`): 2.4 adds the 9 global rows; never bound: `sN_size_sync`, `sN_density_sync` (2.5), mod, voice mode, MPE, record.
- **FORK-MANIFEST.md:** ControlGrid column 7 += "; 2.4: 9 global rows (feedback, fb_low_cut, fb_high_cut, fb_saturation, fb_diffusion, freeze, reverb_mix, reverb_size, reverb_damp)". The runner refreshes column 5 at the checkpoint (`shasum -a 256`). FeedbackChain.h stays VERBATIM (first used 2.4).
- **Read set (PD16):**
  - `readSet24()` in `engine_gates_21.h` after `readSet23()`;
  - N2 (`:2071-2098`), N2s (`engine_gates_22.h:1443-1470`) and FN3 (`engine_gates_23.h:332-359`) use it with the literals **162 / 99**; titles say `readSet24`.
- Done when the checkpoint passes and drift is 0 / 13 after the column-5 refresh.

**T6. Reverb + wet guard + knee chain (PD11, P9, P10)** [agent]
- Files: `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`, `Source/dsp/GiConstants.h`.
- Depends on: T5.
- `Engine.h`:
  - `#include "ReverbUnit.h"`;
  - members `ReverbUnit reverb;`, `std::atomic<float> revEnabled, revSize, revDamp, revPredelay, revMix, revMod, revShimmer;` and `float wet[2][kCell] {};`;
  - `void writeReverbAtomics() noexcept;`.
- `GiConstants.h`: `kRevPredelayMs = 20.0f`, `kRevMod = 0.2f`, `kRevShimmer = 0.0f` (parent values, `Engine.h:114-116` / `Engine.cpp:222-235`).
- The constructor binds the 7 atomics once (`reverb.bind (&revEnabled, …)`, `ReverbUnit.h:112-114`). `prepare` calls `writeReverbAtomics()` then `reverb.prepare (fs, kCell)`. `gridTick` calls `writeReverbAtomics()` once per tick, after `gp.latch` (P9).
- `mixPiece` per PD11 steps 1–5:
  - The y statements (`:1493-1520`) are unchanged and store into `wet[0][c − c0]`, `wet[1][c − c0]`.
  - The guard, then `reverb.process` on a non-owning `juce::AudioBuffer<float>` over `wet` for `c1 − c0` samples (two channel pointers; no allocation; the parent statement).
  - The knee per sample (T3's hook branches unchanged), then `outL[c − c0] = wet[0][c − c0] * og;`.
- Hooks `reverbHardCut`, `injectPreKnee`, counters `preKneePeak` / `kneePeak` (PD19).
- Done when the checkpoint passes (mix 0 = the unit returns untouched).

**T7. Tap + 4 FeedbackChains + returns (PD7, PD8, P1, P11)** [agent]
- Files: `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on: T6.
- `Engine.h`:
  - `#include "forked/FeedbackChain.h"`;
  - members `FeedbackChain chain[kGiStreams];`, `float fbRet[kGiStreams][2][kFbDelay] {};` (replaces `std::vector<float> fbReturn`, `:496`), `ExactRamp unityKnob, parityKnob, fbLowRamp, fbHighRamp, diffRamp, satDRamp;`, `CellLerp gFbLerp[kVoiceSlots][kGiStreams];`, `float gFbTarget[kVoiceSlots][kGiStreams] {};`, `float tapAcc[kGiStreams][2][kCell] {};`, `float envSum[kGiStreams][kCell] {};`, `double prevLowHz, prevHighHz;`.
- `prepare`:
  - remove `fbReturn.assign` (`Engine.cpp:105`); zero `fbRet`;
  - `chain[s].prepare (fs)`;
  - `unityKnob.reset (1.0f)` (never ticked);
  - the remaining ramps reset to their targets; `chain[s].setCutoffs` once.
- `gridTick`: the parent `updateChainCutoffs` (PD7): log2 ramps, `setCutoffs` on change only; diff / satD / parity-knob ramps set and ticked; `gFbLerp` targets set beside `lawTarget` (`:827`, `:938`) from the same `nEff`.
- `mixPiece`:
  - In the existing k-loop (`:1472-1488`), after the two mix statements and before `bl[c] = 0.0f`, as separate statements gated on `envBuf[k][c] > 0.0f`: `tapAcc[s][ch][c] += envBuf[k][c] * g * bl[c]` (g = `gFbLerp[k][s].at (c)`, or 1 under `sumTap`; `fbArchLaw` swaps the target law) and `envSum[s][c] += envBuf[k][c]`. Both are zeroed per piece with the accumulators.
  - After the stream loop, per stream: `tap = tapAcc · min (levelLin, 1) · aud / max (1, envSum)` (÷1 under `sumTap`), then `chain[s].process (tapL, tapR, len, c0, chainKnobParity ? parityKnob : unityKnob, diffRamp, satDRamp)`, then copy into `fbRet[s][ch][c0 + i]`.
  - `levelLin` = `levelRamp[s].value (c)` before voice gain. Chains run for all 4 streams every piece, regardless of freeze or feedback.
- Hooks `chainKnobParity`, `sumTap`, `fbArchLaw`, `tapProbe` / `retProbe`, `injectFbNan` (PD19); accessor `getChainForTest (s)`.
- Done when the checkpoint passes (nothing reads `fbRet` yet) and the agent reports the before / after text of the k-loop (`:1472-1488`).

**T8. Record heads, MipTape settle / hash, ramps, freeze, swap rule (PD2–PD5, PD10, P2–P6, P14)** [agent]
- Files: `Source/dsp/MipTape.h`, `Source/dsp/GiRecordHead.h` (new, header-only), `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`, `Source/dsp/GiConstants.h`.
- Depends on: T7.
- **`MipTape.h`:** `settleRun (pos, n)` (PD10); `hashForTest()` / `levelPeak (level)` under `OGI_TEST_HOOKS`; header comment (`:61-62`) per PD10; the `settleOff` switch is honoured by the caller, not inside MipTape.
- **`GiRecordHead.h`** (`ogi::RecordHead`, one per slot):
  - State: `std::int64_t h`, latched `lo, hi` (re-latched at each wrap from `region[s]`), `ExactRamp run`, `float returnPk`, `bool parked = true`, `bool swapWaiting`.
  - **`writeCell (MipTape<4>&, const float (&r)[2][kFbDelay], const ExactRamp& fb, …)`**: for i ∈ [0, 32), `λ = kFbKnob · fb.value (i) · run.value (i)`; read `wc` at h + i (level 0); `rw` = clamp (PD2); `w` = convex; non-finite → `wc` + `writeScrubs`; else the envelope. Scratch L / R (≤ 32). `writeRun` once, or twice when the cell crosses `hi` (then `h = lo`, re-latch, `settleRun (hi_old, margin(4))`, `settles++`).
  - Returns the per-cell max |w| for `maxWritten` (hooks).
  - `park()`: `settleRun (h, margin(4))` unless `settleOff`; `parks++`.
- **Engine integration:**
  - `fbRamp` (ExactRamp, 20 ms), `RecordHead head[kTapeSlots]`.
  - `gridTick`:
    1. after `gp.latch`: `fbRamp.setTarget (lambdaRampZero ? step : 20 ms)`;
    2. freeze edges (`freezeHardStop` → `run.reset (0)`);
    3. `runTarget_s` per PD4 (`runIgnoreRate` drops the rate term);
    4. `run.setTarget`; tick both ramps;
    5. the PD5 swap rule **before** `takeTapes`;
    6. `takeTapes` repositions heads per PD5;
    7. then, **after** `takeTapes` / `finishXfades` / `updateRegion`, per slot: if not parked → `head[s].writeCell (current[s]->tape, fbRet[s], fbRamp)` and `h += 32` (wrap inside `writeCell`); if the head became parked this cell → `park()`.
  - The writes run at the tick, before any MIDI or segment of the cell (PD3).
  - `returnPk` is updated at the tick from the previous cell's `fbRet[s]` (PD4).
  - `prepare`: heads at `region lo`, parked, `run.reset (0)`, `fbRamp.reset (target)`, `swapWaiting = false`; a saved `freeze` on stays parked.
- Hooks `lambdaRampZero`, `runRampZero`, `freezeHardStop`, `resetHardSwap`, `writeScrubOff`, `rClampOff`, `settleOff`, `runIgnoreRate`, `injectReturn` and counters `writes`, `maxWritten`, `writeScrubs`, `rClamps`, `settles`, `parks`, `swapWaits` (PD19); accessor `getHeadForTest (s)`.
- Done when the checkpoint passes. With feedback 0, `fbRamp.settledAt (0)` → every head parked → 0 writes and 0 settles; the agent states where the skip is tested.

**T9. Spawner head clamp + scan (PD9, P2)** [agent]
- Files: `Source/dsp/GiHeadClamp.h` (new, header-only), `Source/dsp/Engine.h`, `Source/dsp/Engine.cpp`.
- Depends on: T8.
- **`GiHeadClamp.h`:** the pure function per PD9. No engine types except `MipRing<4>::margin`; `constexpr int kHeadAhead = kCell + 8;`.
- **`Engine::spawn`** (`Engine.cpp:1185`): after the region fit (`:1257-1270`) and before the stereo spread, when head `s` is not parked and `! headClampOff`, call `clampToHead` with the head at `h + onsetOffset`. It consumes no draw and does not reorder any draw. A forced result leaves `p0` and counts `forcedCrossings`.
- **`Engine::renderGrains`** (`:1123`): under `headReadScan`, per segment and live grain, the exact interval test of PD9, comparing against the grain's spawn-time head state (a test-only `bool spawnedRunning[kPoolLive + kPoolFade]` written at spawn).
- Done when the checkpoint passes (parked heads → clamp skipped).

**T10. Reset / SR path: loader + processor (PD6)** [agent]
- Files: `Source/dsp/SlotLoader.h`, `Source/dsp/SlotLoader.cpp`, `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`.
- Depends on: T4. It is file-disjoint from T5–T9, but is written after T9 (one agent) and checkpointed like them.
- `SlotLoader`:
  - `requestReset (int slot)` (`−1` = all; per slot: `ScopedLock`, `loaded && pristine && ! loading`, dedupe on `resetInFlight`, then `startRebuildLocked (slot, sessionRate)` with the early-out at `:402` bypassed);
  - `resetSync (int slot)` under `OGI_TEST_HOOKS` (the `loadSync` pattern, `:534`: build from `pristine` at `sessionRate` with `rebuild = true` and the same generation, then `handoff.publish`);
  - `resetInFlight` cleared in `publishResult` / `jobFailed`.
- `PluginProcessor`: `void requestReset (int slot)` (message thread) → `loader->requestReset`; under `OGI_TEST_HOOKS`, `bool resetSlotSync (int slot)`. No parameter, no listener, no `<Engine>` key.
- Done when the checkpoint passes and the agent confirms `requestRebuild`'s SR path (`:390-406`) is unchanged.

### Wave 3 — parity side (agent writes in parallel with W2; the run waits for a quiet window)

**T11. PB / PR rows + parent tap / return dumps (PD12)** [agent writes, run executes]
- Files: `tests/parity/ParityRows.h`, `tests/parity/parent_render.cpp`.
- Depends on: T4. The agent writes alongside W2. **The run happens after T4's parity run and outside any T2 / T17 CPU window.**
- **`ParityRows.h`:**
  - `Kind` gains `fbNull`, `revNull`; `isNullKind` is unchanged (pg22 / pg23 never see the new kinds); add `isFbKind` / `isRevKind`.
  - `Row` gains `int blockSize = 512` (PB rows 32) and `std::vector<std::string> oursHooks` entries `chainKnobParity`, `legacyWetTanh`, `testAgcLegacy`, `retLag31`.
  - Rows per PD12: PB0, PB1–PB12, PBd (it reuses PB5's parent render), PBc1–5, PR1–PR12, PRk, PRc1–2. Every row sets `feedback`, `fb_*`, `reverb_*` explicitly on both sides, the 2.3 `fxBase()` pattern.
- **`parent_render.cpp`:**
  - `row.blockSize` drives the render block.
  - For `fbNull` rows: the parent's `hooks.tapProbe` points at a buffer sized to the render, and `getFbDelayForTest()` is copied after every 32-sample block. Writes `tap_<row>.f32` and `ret_<row>.f32` beside the output.
  - `manifest.tsv` appends, at the end of each line, the landed `feedback fbLow fbHigh fbSat fbDiff revMix revSize revDamp` raws and `peak` (the 2.3 PD11 rule).
- Run (BG): `tests/parity/run.sh "$S/parity"`.
- Done when:
  - rc 0, and every parent self-check passes (anchor == A, writeHead == W0, landed == total);
  - **every 2.2 / 2.3 row's `.f32` is byte-identical** to `$S/parity-base-out` (`cmp`);
  - every PB row has non-empty tap / return dumps with a non-zero tap after `kTe`;
  - PRk's parent `peak` > 0.6 (else the one PD12 level step, recorded; still ≤ 0.6 → STOP).

### Wave 4 — gates, build, parity

Rules for every row:
- Each row prints its metric, threshold, margin and control. A row without a control that can fail is not accepted (a detector-liveness control counts, the 2.3 FZ1 precedent).
- Events in deterministic rows land at absolute multiples of 32 with event-split blocks. Resets in deterministic rows use `resetSlotSync`.
- The "pristine build" of a slot is `hashForTest()` of a fresh `SlotTape` built by a second processor's loader from the same PCM at the same rate.

**T12. `engine_gates_24.h`: bypass, law arm, ceiling, settle, reset, SR, allocations, `--cpu25`** [agent]
- Files: `tests/render-harness/engine_gates_24.h`.
- Depends on: T9, T10, T3.

| ID | Row | Stimulus | Pass | Negative control |
|---|---|---|---|---|
| TB0, TB2 | (T3) | — | — | — |
| TB1 | True bypass at moved 2.4 params (CONTEXT Defaults) | the 14 rh jobs with `rh::overrides.extra`: feedback 0, reverb_mix 0, fb_low_cut 2000, fb_high_cut 1000, fb_saturation 100, fb_diffusion 100, reverb_size 100, reverb_damp 0; `events`: freeze on at 0.5 s, off at 1.0 s | shipping 14/14 == renders-2.4 **and** legacy 14/14 == renders-2.2 | N1-default + feedback 0.1 % → differs; N1-default + reverb_mix 1 % → differs |
| FL1 | Algorithm §3 stress (DSP-04) | 60 s @ 48 k / 512; 16 notes held (polyphony 16); fb 100 %; s1–s4 on, density 200, size 500 ms, pitch +12, level +6 dB, filter LP / BP / HP / LP 1 kHz res 100 %; grain_limit 256; full-scale noise tape on all 4 slots; reverb mix 100 / size 100 (shared with CL1) | per slot max(`maxWritten`, final level-0 scan) ≤ max(pristine peak, 0.7f) **exactly** (PD2); 0 non-finite output samples; 0 non-finite / subnormal values in every tape level (end scan), the chain state (end scan) and the output; writes > 0 on 4 slots. INFO: per-level peaks, `rClamps`, `writeScrubs`, `forcedCrossings` | scanners fed `denorm_min` and NaN → 1 each; the bound checker fed bound + 1 ulp → 1 |
| FL1s | sat-0 bound (PD2, P8) | FL1 with `fb_saturation` 0 and 0.5-peak noise tapes (bound = 0.7f), 60 s | bound 0.7f exact on every slot; `rClamps` INFO | `rClampOff` + `injectReturn {s1, 10 s, 0.95}` → a written sample > 0.7f |
| FL2 | Runaway guard (DSP-04) | the FL1 patch on s1 only, `key_track` 0 (unison pitch), fb 100 %, 1 vs 16 distinct notes, 20 s; tap RMS (`tapProbe`, s1) over the last 10 s | RMS₁₆ ≤ RMS₁ + 1 dB | `sumTap` → RMS₁₆ − RMS₁ ≥ +6 dB (incoherent ≈ +12) |
| FL3 | Freeze hash (DSP-04, D24, P14) | fb 100 %, 8 notes, freeze on at 5 s | `hashForTest()` at the first tick after `run` lands at 0 **and** its settle == the hash 10 s later; `writes` frozen | the hash taken at the freeze edge (before the ramp) → differs |
| FL4 | Feedback 0 (DSP-04, P5) | the FL1 stimulus at feedback 0, 60 s | every slot's hash == its pristine build (bitwise); `writes` == 0; `settles` == 0 | feedback 0.1 % → differs |
| FL5 | Block-size null (ARCH Algorithm §1) | scripted chord with > 16 notes (steals), sustain, bend; `eg22::randomised()`; the FN2 FX set; fb 80 %; freeze toggles ×4; `resetSlotSync` s1 at 6 s and s3 at 12 s; reverb mix 50 → 0 → 50; 20 s | blocks 441 / 512 / 4096 / var vs 32: output max\|Δ\| == 0 and every final tape hash equal | seed + 1 → differs |
| FL6 | NaN scrub (ARCH §17) | fb 100 %, 4 notes; `injectFbNan {s2, 2 s}` | chain s2 `scrubs` == 1; every tape finite; s1 / s3 / s4 hashes == a no-inject reference; `writeScrubs` == 0 | `writeScrubOff` + `injectReturn {s2, 2 s, NaN}` → ≥ 1 non-finite tape sample |
| FL7 | Unread params, loop active (PD16) | the N2s stimulus + fb 80 %, reverb 50 %, freeze toggles ×2; every ID outside `readSet24()` (99) randomised | output and tape hashes bit-identical; 162/162 read IDs bound | `fb_diffusion` 51 → differs; `reverb_size` 51 → differs |
| FL8 | Tap law: one cloud ≤ tape peak (ARCH §3 premise, PD8) | 1 voice at root, s1, DC tape 0.5, level +6 dB (min(level, 1) caps), FX off, spread 0, gain_rand 0, jitter 0; density {10, 75, 200} × size {20, 500} ms × window {Hann, step 32}, 4 s each | tap peak ≤ 0.5 · (1 + 1e-6) in all 12 configs | `fbArchLaw` → tap peak > 0.5 in ≥ 1 config |
| CL1 | Ceiling, stress (D23, PD14) | the FL1 render | `kneePeak` ≤ float(A) over 60 s; pre-knee peak INFO | `wetSafetyOff` → output peak > float(A) (pre-knee > A asserted, else INVALID = FAIL) |
| CL2 | Ceiling, FZ1 carried (D23, PD14) | `eg23::stressParams()` at 44.1 / 48 / 96 k | `kneePeak` ≤ float(A); pre-knee INFO (2.3: 1.53 / 1.71 / 1.73) | `injectPreKnee {0.5 s, 8.0}` + `wetSafetyOff` → > float(A); with the knee on, the same injection stays ≤ float(A) |
| MS1 | Mip settle (PD10, P3) | fb 80 %, 4 notes, region 2 s, ≥ 10 laps; notes off → park | every level of every slot **bitwise** == a fresh offline `MipTape` built from the final level 0 (`writeRun` [0, F) + `flushTail`) over the whole stored span. INFO: pitched reads at levels 1–4 across [hi − 1100, hi) and across the park point vs `MipSnapshot` (≤ −120 dB, the ARCH MipTape gate) | `settleOff` → mismatching children > 0 within margin(4) of hi and of the park point |
| RS1 | Reset == pristine, frozen (DSP-04) | fb 100 %, 4 notes, 5 s; freeze on; `resetSlotSync` on all 4 slots; 2 s more | each slot hash == its pristine build after the take + crossfade; still frozen: hashes and `writes` unchanged over the 2 s | no Reset → hash ≠ pristine |
| RS2 | Reset while parked | fb 100 %, 4 notes 3 s; notes off; wait for park; `resetSlotSync (s1)` | s1 hash == pristine; s2–s4 hashes unchanged | no Reset → s1 hash ≠ pristine |
| RS3 | Host-idle bounded memory (PERF-03) | the eg21 H2 harness (`:1766-1815`): 100 `requestReset (−1)` with no processBlock, message loop pumped | `pendingDepth (s)` ≤ 1 per slot; `SlotTape::liveCount` ≤ baseline + 4 after the sweep; ≥ 1 reset tape pending per slot (liveness) | the H2 control `testNoEntryCounter` → superseded tapes not freed (as H2) |
| RS4 | Reset mid-crossfade queues (PD5) | fb 100 %, notes; Reset A; Reset B at A's take + 10 ms (inside the 20 ms fade) | `xfadesStarted` +1 at A's take and +1 again only after A's fade ends; `pendingDepth` ≤ 1 | B at A's take + 100 ms → taken at the next parked tick |
| SR1 | SR rebuild path (CONTEXT Defaults, DSP-07) | 44.1 k pristine; session prepared at 96 k; notes held, fb 100 %; the async rebuild lands later (polled) | before the land: `writes` == 0, hash unchanged, root pitch ±1 cent (the eg21 P-row autocorrelation probe); after the land: head at region lo, writes > 0 after the crossfade | `runIgnoreRate` → writes > 0 before the land |
| FA4 | 0 allocations (PERF-01) | the FA3 stimulus (`engine_gates_23.h:998`) + fb 100 %, reverb 50 %, and: Reset per slot + all (`resetSlotSync` outside processBlock), freeze toggles, fb 0 ↔ 100 step, an SR-rebuild take (a pre-published rebuilt tape), steals with feedback on, a reverb re-enable (`clearTank`), a wrap and a park (settle bursts) | 0 audio-thread allocations; each event's counter > 0 (`tapesTaken`, `xfadesStarted`, `parks`, `settles`, `voiceSteals`, `writes`) | liveness `malloc(64)` → 1 |

- **`cpu25()`** per PD17, reusing `eg23::measureCfg` / `c22Session` / `worstFx` (`engine_gates_23.h:1089-1240`) and `cal::readRef`. It prints a `C25w` line with `writes` per slot per window.
- `eg24::runAll()` order: TB0, TB1, TB2, FL1 (+CL1 from the same render), FL1s, FL2–FL8, CL2, MS1, RS1–RS4, SR1, FA4.

**T13. `engine_gates_24b.h`: head clamp, click gates, reverb (PD9, PD13)** [agent]
- Files: `tests/render-harness/engine_gates_24b.h` (new, `eg24b::runAll()`).
- Depends on: T12 (it uses eg24 helpers).
- κ helper: port of parent `engine_gates_24b.h:137-138` (`kappa (out, t0, t1)`), the rename only.

| ID | Row | Stimulus | Pass | Negative control |
|---|---|---|---|---|
| HC1 | Clamp telemetry + scan (D22) | 4 streams, fb 80 %, density 200, pitch ±24 (levels 0–3), reverse 50 %, region 2 s, 8 notes, 10 s; `headReadScan` | `forcedCrossings` == 0; `headScanViolations` == 0; writes > 0; INFO `parkedSpawnCrossings` | `headClampOff` → violations > 0 |
| HC2 | Forced crossings counted | region 30 ms, size 100 ms (the reach ≥ W branch), fb 80 % | `forcedCrossings` > 0 | the HC1 region (2 s) → 0 |
| HC3 | Clamp boundary (unit, `GiHeadClamp.h`) | levels 0–4 × fwd / rev × rate {0.5, 1, 2, 4}; head positions sweeping the region (every frame for W ≤ 4096, else 4096 samples) | W = \|Δ\| + 40 + margin + reach + 1 → feasible at every head position, with the result outside the forbidden set and inside the fit; W = \|Δ\| + 40 + margin → forced at every head position; INFO the feasible fraction between | the same sweep with g_a = 16 (ARCH Algorithm §4) → a returned p0 inside the PD9 forbidden set |
| CK1 | Freeze toggle (QUAL-01, D24) | PD13 table | κ ≤ 2.0× static | `freezeHardStop` > 2.0× |
| CK2 | Feedback step 0 → 100 % | PD13 | κ ≤ 2.0× static | `lambdaRampZero` > 2.0× |
| CK3 | Head stop / start | PD13 (region 300 ms, spray 100 %, 4 s after the park) | κ ≤ 2.0× static | `runRampZero` > 2.0× |
| CK4 | Reset swap | PD13 (same-motion freeze pulse) | κ ≤ 2.0× static | `resetHardSwap` > 2.0× |
| RV1 | Reverb audible (DSP-05; parent RV1) | noise tape, reverb_mix 50, size 0; 2 s input, then silence | wet − (mix 0) > −40 dBFS during input; tail RT60 > 0.2 s | reverb_mix 0 → fails the −40 dBFS criterion |
| RV2 | Mix → 0 (DSP-05; parent RV2) | mix 50 → 0 at a tick | κ ≤ 2.0× static; output − (mix-0 render) < −90 dBFS within 60 ms; after the 50 ms fade, output bitwise == the mix-0 render | `reverbHardCut` → κ > 2.0× |
| RV3 | Re-enable, cold tank (parent RV3) | mix 0 → 50 after 2 s off | κ ≤ 2.0× static-on | the κ detector fed the same render + a 1-sample +0.1 step → > 2.0× |

**T14. `parity_gates_24.h` + `main.cpp` wiring (PD12, PD15, PD17)** [agent]
- Files: `tests/render-harness/parity_gates_24.h` (new, `pg24::run (dir, prefix)`), `tests/render-harness/main.cpp`.
- Depends on: T13, T11 (`ParityRows.h`).
- `pg24` renders our side per PD12: PB rows with `chainKnobParity`, freeze from the first tick, `tapProbe` / `retProbe` on s1, the row's block size; PR rows plain. It reads the parent `out.f32`, `tap_*.f32`, `ret_*.f32` and the manifest. It reuses `pg22` (LS solver, dB helpers) and `pg23` (g₀ from PF0) in the same TU. PB0 is evaluated first; PB rows report `BLOCKED (PB0)` when it fails.

| ID | Pass | Control |
|---|---|---|
| PB0 | tap null ≤ −130 dB; sets `g_PB` | (gates PB1–12) |
| PB1–PB12 | return null ≤ −90 dB after LS alignment (n ≥ 256); \|g / g_PB − 1\| ≤ 1e-4; ref ≥ −70 dBFS; landed fb raws identical; PB9 `env > 0.7` reached | PBc1, PBc3, PBc4, PBc5 each > −90 dB; PBc2 band fails |
| PBd | xcorr(our ret, parent ret) peak at lag 0; xcorr(our written layer, our tap) peak at lag 32 | PBc5 |
| PR1–PR12, PRk | null ≤ −90 dB; \|g / g₀ − 1\| ≤ 1e-4 (g₀ from PF0); PRk parent peak > 0.6 | PRc1 > −90 dB; PRc2 null or band fails |

- **`main.cpp`:**
  - `#include "engine_gates_24b.h"` and `"parity_gates_24.h"`;
  - `eg24b::runAll();` after `eg24::runAll();`;
  - `--parity` also calls `pg24::run`; `--rows` prefixes `PB` / `PR` work;
  - `--cpu25` → `eg24::cpu25()` (its own rc);
  - banner and usage updated.

**T15. Build + default harness** [run]
- Files: none (ControlGrid column 5 was refreshed at the T5 checkpoint; re-check it here).
- Depends on: T12–T14.
- Run (BG): `ninja -C build O-GranulatorInstrument-render-harness O-GranulatorInstrument-param-dump` (consoles only; plugin targets are built AND installed together in T19); then `$H` → `$S/h-24.log`; then param-dump; `tests/fork-drift-check.sh`.
- Done when:
  - rc 0, no new warnings in plugin sources;
  - harness **181 PASS / 0 FAIL**: 150 carried (RB1 / FR1 under legacy, N2 / N2s / FN3 on readSet24) + TB0 / TB2 + the 29 W4 rows; every control fires (`ctrl FIRED`, **0 `NOT FIRED`**);
  - `sort | uniq -d` of the row IDs == `$S/dup-23.txt`;
  - **TB0 14/14, TB1 both halves, RB1 7/7, FR1 14/14** — otherwise **S3**;
  - param-dump byte-identical to `$S/pd-23.txt` and `.planning/params.tsv`;
  - drift **0 / 13** with `FeedbackChain.h` and `OutputLaw.h` VERBATIM byte-identical, and NC-a / NC-b (re-made in `$S`) fire;
  - **X1:** `ninja -C build -t compdb` filtered to entries whose output contains `O-GranulatorInstrument` has 0 matches of `O-Granulator/Source|ogi-parity|82406fce` (control: run.sh's positive count).
- On a failure: fix it in the owning task (T1 map) and rebuild. A law-arm failure → **S5**; HC1 / CK → **S6**. **Never loosen a threshold or re-record renders-2.1 / 2.2 / 2.4.**

**T16. Parity run (DSP-04 / DSP-05 leaf nulls; DSP-01 / DSP-03 carried)** [run; agent fixes]
- Depends on: T11, T15.
- Run (BG): `tests/parity/run.sh "$S/parity"` (incremental); `$H --parity "$S/parity/out" --rows PB0` → **≤ −130 dB** (else **S1** before any PB row counts); then the full `$H --parity "$S/parity/out"` → `$S/par-24.log`; then `--rows GV`.
- Done when:
  - `--parity` **133 PASS / 0 FAIL**: PB0, PB1–12 inside the band, PBd, PBc1–5 fire, PR1–12 + PRk inside the band, PRc1–2 fire;
  - the carried 99 pass, with PN residuals == `$S/pn-24.txt` and PF / PFc == `$S/pf-base.txt` to 0.1 dB;
  - GV 2/0;
  - SUMMARY gets the g_PB / g₀ band table and the PB / PR residual ranges.
- A PB / PR miss is diagnosed against the drive (knob, cutoff ramps, satD, diffusion mix), the tap law and statement order, or the reverb atomics mapping / tick timing. **It is never tuned.** A miss with PB0 passing → **S1**; a band miss → **S4**.

### Wave 5 — CPU verdict

**T17. `--cpu25`: the final PERF-02 worst case + typical (D25, PD17)** [run]
- Depends on: T16 (no parity or build running; Logic closed).
- Run (BG, **alone**): `$H --golden-dir tests/golden --cpu25` → `$S/cpu25.log`.
- Done when:
  - each config line carries calib before / after vs `ref_ns`, duty, live and median / min / max;
  - **C25w ≤ 40 % (PASS)** with live ≥ 240 and writes > 0 on all 4 slots in every window;
  - **C25t ≤ 12 % (PASS)** with live ∈ [60, 90];
  - C23w (same session), C25r, C25w96 and C25s printed; Δ = C25w − C23w and the C25s flag go to SUMMARY (projection C25w ≈ 28–30 %, R§9).
- A NOVERDICT is a FAIL: re-run alone up to 3 times; a third NOVERDICT → STOP and report the environment. An INVALID (writes = 0 on a slot) is a FAIL of the row construction: fix the stimulus once, recorded. A valid-witness FAIL → T18.

**T18. Lever ladder — ONLY if T17 has a valid-witness FAIL (D21, PD18)** [agent + run]
- Files (if fired): `Source/dsp/GiRenderer.h`, `Source/dsp/Engine.cpp`.
- Depends on: T17.
- **Lever 2** (4-lane SIMD over same-(k,s) grains), then **lever 3** (verify the hoisted `fxActive` skip; the P18 dual kernel). Each is kept only under the PD18 conditions. Rebuild consoles (BG), full harness, `--parity`, then `--cpu25` alone after each.
- Still FAIL → **S2**. Lever 1 is never applied here.
- Done when: `--cpu25` passes with a valid witness, or S2 is raised with the measured numbers after each lever.

### Wave 6 — install, validators, hands-on sheet

**T19. Install + Standalone + validators + X2** [run]
- Depends on: T17 (T18 if fired).
- `./scripts/build-and-install.sh O-GranulatorInstrument` (BG): it clears the AU cache and sweeps both the `O-GranulatorInstrument` and `-dev` variants; afterwards only `O-GranulatorInstrument-dev.{vst3,component}` are on disk.
- **Standalone (P16):** `ninja -C build OuariconGranulatorInstrument_Standalone` (BG). `build/plugins/O-GranulatorInstrument/OuariconGranulatorInstrument_artefacts/Release/Standalone/O-GranulatorInstrument-dev.app` must be newer than T15's harness binary and alive after 6 s.
- `auval -v aumu OuGI OuDv` PASS + the `ZZZZ` control. pluginval 10: VST3 ×2 + AU, BG, one at a time, `--strictness-level 10 --timeout-ms 600000`, `Main bus num input channels: 2`, Logic closed.
- **X2:** `nm -C` on the installed VST3 + AU binaries and `$H` → 0 `ogran::`; control: the parity binary → > 0.
- Done when: all PASS.

**T20. `HANDS-ON-2.4.md` (human gate, deferred)** [agent writes; the user runs it after Stage 3]
- Files: `stages/2-dsp/HANDS-ON-2.4.md`, in the HANDS-ON-2.3 layout: build line, a "Deferred with HANDS-ON-2.1 / 2.2 / 2.3" banner, defaults-to-know (feedback 0, freeze off, reverb mix 0; Reset is the Stage 3 native button; the working copy is not saved), a table with Standalone / DAW / Notes columns, and a Result line.
- Depends on: T19.
- Rows (Standalone **and** one DAW, Logic AU):
  1. Feedback 0 → 100 % on a held chord at `fb_saturation` 0 / 20 / 100 → the copy evolves; the character changes per saturation; no runaway, no level blow-up.
  2. `fb_low_cut` / `fb_high_cut` sweeps → the written layer thins / darkens; `fb_diffusion` 0 → 100 smears it.
  3. Freeze on / off while playing → the heads stop with no click; the evolved copy holds; unfreeze resumes smoothly.
  4. Head start / stop: release every note, wait, play again → no splice as grains sweep the stop point.
  5. Reset per slot and all, while playing and while frozen → back to pristine through a smooth 20 ms crossfade; frozen stays frozen.
  6. Chords vs unison: chords thin in the copy (the ARCH §3 mean-tap consequence). Judge whether the `1/sqrt(Σa)` fallback is needed (listening call; any change is a later decision).
  7. Reverb mix / size / damp → audible; tails fade on mix → 0 with no step; a cold re-enable is clean.
  8. Extreme patch (fb 100 + reverb 100 + res 100 + 16 voices) → never above +6 dBFS before `output_gain` (meter); judge the knee's limiting character.
  9. DAW sample-rate change 44.1 ↔ 96 k with feedback on → pitch stays correct; the heads resume after the rebuild.
  10. Session reopen → the feedback / freeze / reverb params restore; the working copy reloads pristine (by design).
  11. Knee adoption: the 2.1–2.3 patches sound unchanged (identity ≤ −6 dBFS; only hot peaks differ).
- Done when the sheet exists. **Not self-certified**; the run is deferred to after Stage 3 with HANDS-ON-2.1 / 2.2 / 2.3 (user override 2026-11-08).

### Wave 7 — bookkeeping + commit C2

**T21. SUMMARY, STATUS, commit** [agent writes, run commits]
- Files: `stages/2-dsp/SUMMARY-2.4.md`, `.planning/STATUS.md`.
- Depends on: T19, T20. STATUS reads `stage_2_phase_2_4_executed_hands_on_pending` (HANDS-ON-2.1 / 2.2 / 2.3 / 2.4 deferred).
- SUMMARY covers:
  - every row with its margin and control; the exact counts vs the PD15 targets;
  - the C1 attribution (legacy dumps, the moved-line list, the new PN range, GV0);
  - PB0, `g_PB`, g₀ and the PB / PR band table;
  - the law-arm numbers (FL1 / FL1s bounds and `rClamps`, FL2 dB, MS1, the RS / FL3 / FL4 hashes);
  - the HC1 telemetry and the CK κ ratios with each hard control;
  - C23w / C25w / C25t / C25r / C25s / C25w96 with `ref_ns` and calib before / after, and Δ;
  - the lever outcome;
  - PD1–PD21 with the PD2 / PD3 / PD4 / PD5 / PD12 / PD13 decisions and corrections stated;
  - every deviation from this plan, with its reason.
- **Commit C2:**
  1. `git branch --show-current` (= `main`) and `git status --short` **immediately before**. `git status --short -- plugins/O-GranulatorInstrument` must list only this plan's files; anything else under the plugin → STOP and report before committing.
  2. `git add` the exact new paths: `Source/dsp/GiRecordHead.h`, `Source/dsp/GiHeadClamp.h`, `tests/render-harness/{engine_gates_24b,parity_gates_24}.h`, `stages/2-dsp/{SUMMARY,HANDS-ON}-2.4.md`. Then `git commit -- plugins/O-GranulatorInstrument`. Never `-A` / `-a`; foreign paths (`modules/registry.yaml`, `.claude/*`, `plugins/O-GrainScatter/*`) are never staged.
  3. `git log -1 --stat` shows only `plugins/O-GranulatorInstrument/` paths. **No tag.**

---

## Dependency graph

```
T1 ─→ T2 ─→ T3 ─→ T4 ─→ T5 ─→ T6 ─→ T7 ─→ T8 ─→ T9 ─→ T10 ─→ T12 ─→ T13 ─→ T14 ─→ T15 ─→ T16 ─→ T17 ─→ [T18 only on FAIL] ─→ T19 ─→ T20 ─→ T21
                    │     (checkpoint after each of T5–T10)                        ↑             ↑
                    └─→ T11 write (alongside W2) ── T11 run (quiet window) ─────────┴─────────────┘
                                                    (ParityRows.h → T14; parent renders → T16)
```

- T2 must finish before any `Source/` edit, so C23w@2.3 measures the 2.3 engine.
- T3 → T4 (C1) must land before any 2.4 engine code, so the re-pin is attributed to the knee alone.
- T5 → T9 run strictly in sequence: they share `Engine.{h,cpp}`. T10 is file-disjoint but written after T9 by the same agent.
- T12 → T13 → T14 run in sequence (helpers, then `main.cpp`).
- T17 runs alone after every build and parity job. CMake is not edited (PD21).

## Execution notes

- **Agent split.** dsp-agent writes T3, T5–T10, T11's sources, T12–T14, T18's code (if fired), T20 and T21's SUMMARY. The runner does every build, checkpoint, gate, validator, hash refresh and both commits.
- **Background jobs.** Long commands run BG with logs in `$S` (600 s watchdog): ninja, `run.sh`, `build-and-install.sh`, the Standalone link, pluginval, cold auval, `--cpu23`, `--cpu25`, the full harness. FL1 / FL1s / CL1 render 60 s each, so the default harness is longer than in 2.3.
- **Writing order inside W2:** T5 → T6 → T7 → T8 → T9 → T10, each followed by its checkpoint. No CMake configure is needed (headers only).
- **Parent reads** go through `git show "${PIN}:…"` (braced) or the run.sh archive, never the working tree. The parity reference is only ever built by `tests/parity/run.sh` from `git archive 82406fce`.
- **CPU runs** (T2, T17, T18) happen with nothing else running: no ninja, parity, pluginval, Logic or other heavy session.
- **Build and install** only via `./scripts/build-and-install.sh O-GranulatorInstrument`, plus the explicit Standalone target.
- **Handoff.** After execute: `/clear` → `/plugin-verify O-GranulatorInstrument 2-dsp`.

## Success criteria (CONTEXT-2.4 gate)

| Criterion | Proven by | Target |
|---|---|---|
| FeedbackChain and ReverbUnit + knee nulls vs the parent ≤ −90 dB, gain band held, controls fail (D22) | PB0, PB1–12, PBd, PBc1–5, PR1–12, PRk, PRc1–2 | `--parity` **99 → 133 / 0**; PB0 ≤ −130 dB; band ≤ 1e-4 |
| 60 s feedback-100 % stress at 16 voices: tape peak ≤ max(pristine, 0.7), no NaN / Inf / denormal (DSP-04) | FL1, FL1s (writer clamp, PD2) | exact bound, 0 non-finite, 0 subnormal |
| Knee output ≤ +6 dBFS on the stress and on FZ1 at 44.1 / 48 / 96 k, before `output_gain` (D23) | CL1, CL2 | `kneePeak` ≤ 1.99526231 |
| Runaway guard: 1 vs 16 voices, the tap does not scale; sum-tap control fails (DSP-04) | FL2, FL8 | ≤ +1 dB; control ≥ +6 dB |
| Reset hash == pristine (incl. while frozen); Freeze hash unchanged 10 s after the ramp; feedback 0 hash == pristine after 60 s (DSP-04) | RS1, RS2, FL3, FL4 | bitwise |
| Click gates: freeze toggle, feedback step, head start / stop, Reset swap vs same-motion controls (QUAL-01, D24) | CK1–CK4 (+ RV2 / RV3) | κ ≤ 2.0×; hard controls > 2.0× |
| Head clamp: 0 forced crossings when feasible; forced control reports; no read inside the unsettled mip margin | HC1–HC3, MS1 | 0 / > 0 / bitwise mips |
| Reverb audible; tails decay on mix → 0 (DSP-05) | RV1, RV2, RV3 | per parent RV1–3 |
| Block-size null with feedback 80 % + freeze toggles; different-seed control | FL5 (+ FL7) | max\|Δ\| == 0 |
| Allocation gate across Reset / Freeze / SR change / steals with feedback; host-idle bounded memory (PERF-01, PERF-03) | FA4, RS3, RS4, SR1 | 0 allocations; pending ≤ 1 |
| True bypass with feedback 0 + reverb mix 0; knee attribution; param-dump identical; fork drift 0 | TB0, TB1, TB2, RB1 / FR1 (legacy), C1, T15 param-dump, drift + NC-a / NC-b | renders-2.1 7/7 + 2.2 14/14 (legacy), renders-2.4 14/14 |
| CPU worst case with chains + reverb ≤ 40 % with the witness; typical proxy ≤ 12 %; reset-fade INFO (D25) | C25w, C25t, C25r, C23w, PD17 witness | **C25w ≤ 40 %**, **C25t ≤ 12 %** |
| Carried gates | 150 carried harness rows, 99 carried parity rows, GV 2/0 | harness **150 → 152 (W1) → 181 / 0**, 0 NOT FIRED |
| auval PASS; pluginval 10 VST3 + AU; parent code excluded | T19, X1, X2 | all PASS |
| HANDS-ON-2.4 written, deferred post-Stage-3 | T20 | 11 rows |

**Requirements this phase moves** (verify sets the final status; REQUIREMENTS.md is not edited by execute):
- **DSP-04:** pending → automated criteria covered (limiter ceiling, NaN, Reset, Freeze).
- **DSP-05:** pending → automated arm (PR nulls, RV1–3); the listening row is HANDS-ON-2.4 row 7.
- **PERF-02:** the final 40 % worst case is in; the typical verdict on presets stays Stage 4.
- **PERF-01 / PERF-03:** the Reset / Freeze / SR-change arms are in; the matrix (2.5) and file swap (2.6) stay open.
- **QUAL-01:** the reset / freeze click gates are in; the hands-on rows stay open.

## Pitfalls → task + gate (RESEARCH P1–P16)

| # | Pitfall | Task | Gate |
|---|---|---|---|
| P1 | Chain knob 0.95·fb + convex λ → loop gain applied twice | T7 (unity knob, PD7) | PB rows with `chainKnobParity`; PBc2 (band); FL2 |
| P2 | `g_a = 16` misses the cell written ahead | T9 (PD9, g_a 40), T8 (PD3) | HC1 scan + `headClampOff`; HC3 (g_a 16 control) |
| P3 | Children past `hi` / a parked head stay stale | T8 (`settleRun`, PD10) | MS1 bitwise + `settleOff` |
| P4 | Hard head start / stop leaves a `(1−λ)·wc` splice | T8 (`runRamp`, park only at λ_eff 0) | CK3 + `runRampZero` |
| P5 | Writer runs `1·wc + 0·r` at fb 0 → −0 flips | T8 (skip on `settledAt (0)`) | FL4 bitwise, TB0, TB1 |
| P6 | Reset swapped while the head writes | T8 (PD5 swap rule), T10 | RS1, RS4, CK4 |
| P7 | Knee re-baseline hides an unrelated hash move | T3 / T4 (PD1, C1 first) | RB1 / FR1 under legacy, then TB0 at every checkpoint |
| P8 | AGC 1 ms attack → `\|r\|` up to 1/d | T8 (PD2 clamp + envelope) | FL1s + `rClampOff` control |
| P9 | Reverb atomics written mid-cell → block-dependent | T6 (tick-only writes) | FL5 (reverb mix changes inside) |
| P10 | NaN into the reverb tank | T6 (wet guard before `reverb.process`) | FL1 finite scan, FL6 |
| P11 | Tap statements merged into the mix → hashes move | T7 (separate statements) | T7 checkpoint, TB0, TB1 |
| P12 | `ControlGrid.h` edit not re-hashed | T5 | FORK-MANIFEST col 5 / 7, drift 0 / 13 |
| P13 | CPU row with parked heads (writers idle) | T12 (PD17) | C25w asserts writes > 0 on 4 slots |
| P14 | Freeze hash taken before the settle | T12 | FL3: hash after ramp end **and** settle; edge-hash control |
| P15 | FL1 checks only level 0 | T12 | non-finite / subnormal scan on every level; per-level peaks INFO |
| P16 | Standalone stale | T19 | explicit `OuariconGranulatorInstrument_Standalone`, `.app` newer than T15 |

## Stop-and-ask conditions (execute; never pre-decided)

- **S1 — leaf-null precondition.** PB0 > −130 dB, or a PB / PR row fails while PB0 (or PF0) passes. STOP and offer: (a) find the tap / drive / reverb-atomics difference (tap law, env weighting, knob, cutoff ramps, tick timing); (b) re-specify the failing family's metric (amends D22).
- **S2 — CPU over budget.** C25w > 40 % or C25t > 12 % (valid witness) after levers 2 → 3. STOP and offer:
  - lever 1 (Hermite; it moves renders-2.4 and the PN nulls, so a re-pin happens only with consent);
  - the `grain_limit` 256 → 128 fallback + a "heavy" UI note (a locked-spec change; it always asks, D21).
- **S3 — bypass broken.** The T4 legacy dump ≠ renders-2.2 / renders-2.1, or a checkpoint / TB0 / TB1 / RB1 / FR1 / lever moves a hash. STOP and offer: find the statement-shape change (tap statements, wet storage, the writer skip, the clamp guard). **Never re-baseline**; renders-2.4 is pinned once (PD1).
- **S4 — gain band.** \|g_row / g_ref − 1\| > 1e-4 on a non-control PB / PR row. STOP: a scalar differs from the parent (knob, satD, level, reverb mix mapping). Diagnose; never widen the band.
- **S5 — law arm.** FL1 / FL1s bound exceeded, FL2 > +1 dB, FL3 / FL4 / RS1 / RS2 hash mismatch, FL5 differs, or MS1 not bitwise. These are structural DSP-04 failures. STOP with the diagnosis (clamp / envelope order, the skip on `settledAt (0)`, the settle span, write timing, the swap rule). Never loosen.
- **S6 — splices / clicks.** HC1 violations > 0 with the clamp on, a CK row > 2.0×, or a hard control that still cannot fire after its one stimulus fix. STOP and offer: ARCH fallback (a), a 2 ms raised-cosine write brush, and / or a longer λ ramp (a D24 change). Both need consent.
- **S7 — carried-row regression from the knee.** At T4, a carried row other than RB1 / FR1 fails, a PN residual is worse than its T1 value by > 0.1 dB, or GV leaves its tolerance. STOP: the knee is the only change, so this is a real interaction (for example a row with an analytic expectation that included the tanh). Never loosen; the user decides between a row fix with a recorded reason and reverting the knee.
- **Inherited stops:**
  - a T1 contract / pin / late-change mismatch;
  - an INVALID row after its one allowed stimulus step (TB2, CL1, PRk, C25w);
  - three NOVERDICTs in a row (environment);
  - unexpected paths under the plugin at either commit.

## Risks

- **R1 Head clamp in modular coordinates** (ARCH Known Challenges). Short regions and reverse grains are the edge cases. HC3 sweeps the exact boundary on the pure function; HC1's scan measures the engine; S6 owns failures.
- **R2 Mip settle correctness** (R§3.2). MS1 is bitwise against an offline build, so a wrong settle span cannot hide behind a tolerance.
- **R3 Knee re-baseline masking** (P7). The knee lands alone, is proven under legacy, is pinned once and committed separately; TB0 then guards every later step.
- **R4 Bypass statement shape** (P11). Five W2 tasks touch the bypass path; PD20's checkpoints attribute any move to its task.
- **R5 Async loader timing in deterministic rows.** `resetSlotSync` at event-split multiples of 32 makes take ticks block-size independent; async `requestReset` is only used in RS3 and C25r (INFO).
- **R6 CPU margin** (R§9: ≈ 28–30 % vs 40 %). Measured, not assumed. C25w asserts the writers run; a slowed core reads NOVERDICT = FAIL.
- **R7 Vacuous click rows.** Every CK hard control must exceed 2.0×, or the row fails.
- **R8 Test hooks in shipping builds.** Every hook is under `OGI_TEST_HOOKS`. X2 and the validators run on the shipping build.
- **R9 Parity build cost.** The archived parent rebuild takes minutes; PB rows render in 32-sample blocks. It runs BG and incrementally, never from the parent working tree.
- **R10 Concurrent sessions.** Foreign dirty paths exist. Both commits are path-scoped, with branch and status re-checked right before each.
