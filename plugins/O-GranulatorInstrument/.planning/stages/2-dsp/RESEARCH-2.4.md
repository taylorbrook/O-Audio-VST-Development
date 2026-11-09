# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.4 — RESEARCH

**Researched:** 2026-11-08
**Scope:** Phase 2.4 only: the feedback tap, the 4 forked `FeedbackChain`s, the record heads (convex write, write-before-read, run condition, λ ramps), the spawner head clamp, Freeze, Reset, the SR-rebuild park, `ReverbUnit` → wet knee → `output_gain`, the D22 proof arms, the D23 ceiling, the D24 click gates and the D25 CPU verdict.
**Confidence:** HIGH on code facts. MEDIUM on the CPU projection (§9) and the AGC-lag bound (§4.3), which are derived, not run. Every claim is tagged **VERIFIED** (read this session: `path:line`, or `82406fce:<parent path>:line` from `git show` blobs, never the parent working tree) or **INFERRED** (reasoning, not run).
**Inputs read:**
- `CONTEXT-2.4.md` (D22–D25, Defaults, criteria, 6 open questions), `RESEARCH-2.3.md`, `research/ARCHITECTURE.md` §5–§8, §16–§18, Processing Chain, Algorithm §1/§3/§4, Risks, Gate Catalogue; `ROADMAP.md` § Phase 2.4
- Plugin: `Source/dsp/{Engine.cpp,Engine.h,MipTape.h,SlotTape.h,SlotHandoff.h,SlotLoader.{h,cpp},GiRenderer.h,GiConstants.h}`, `forked/{FeedbackChain.h,OutputLaw.h,ControlGrid.h,EngineConstants.h,FORK-MANIFEST.md}`, `tests/parity/parent_render.cpp`, `tests/golden/params.tsv`, `tests/render-harness/main.cpp`
- Parent blobs at `82406fce` (`plugins/O-Granulator/`): `Source/dsp/{Engine.cpp,Engine.h,CaptureWriter.h,EngineConstants.h,OutputLaw.h}`, `tests/render-harness/{engine_gates_24.h,engine_gates_24b.h}`, `.planning/stages/2-dsp/SUMMARY-2.4.md`, `tests/golden/params.tsv`
- Modules: `modules/effects/reverb-unit/cpp/ReverbUnit.h`, `modules/synthesis/grain-engine/cpp/BandlimitedReader.h`

## User Constraints (CONTEXT-2.4 + this session; binding)
- **D22:** leaf nulls vs the parent at −90 dB with a gain band and failing controls; the ARCH Algorithm §3 law arm for the tape loop (60 s stress, 1-vs-16 runaway guard, freeze / reset / fb-0 hashes, write-before-read block null); head-clamp telemetry rows.
- **D23:** the ceiling is the parent D9 knee, measured before `output_gain`: ≤ +6 dBFS on the 60 s stress and on the carried FZ1 stimulus at 44.1 / 48 / 96 k.
- **D24:** λ ramps over 20 ms on freeze and feedback; click gates vs same-motion controls with a hard-stop control; the freeze hash is taken from the first sample after the ramp.
- **D25:** worst case + feedback 100 % on 4 chains + reverb mix 100 % ≤ **40 %** (C23 method + calibration witness, C23w re-run in the same session); typical proxy ≤ 12 % regression; Reset dual-read fade INFO.
- **NEW 2026-11-08 (user, this session) — wet knee:** adopt the parent's `ogi::wetKnee`. It replaces our pure `kWetCeil·tanh`. The render hashes are re-baselined once, with attribution (§6.2). This supersedes the CONTEXT default "renders-2.2 / FR1 byte-identical" in one respect: byte-identity is now proven under a `legacyWetTanh` hook, not on the shipping path.
- **Defaults:** forked leaves stay byte-identical (drift 0/13; the `ControlGrid.h` ADAPTED edit is re-hashed). Reset and the SR rebuild run off-thread through `SlotHandoff`. 0 audio-thread allocations. No new params or `<Engine>` keys, and param-dump stays identical. HANDS-ON-2.4 is written and deferred. The Standalone is rebuilt explicitly.

## Project Constraints
- Trunk on `main`. Commits are path-scoped: `git commit -- plugins/O-GranulatorInstrument`. Re-check `git branch --show-current && git status --short` right before each commit. Foreign dirty paths at research time (**VERIFIED**): `.claude/agent-memory/{dsp,gui}-agent.md`, `modules/registry.yaml`, untracked `.claude/compaction-snapshot.md`, `plugins/O-GrainScatter/.planning/improvements/adaptive-grain-length/nielsen-dafx26.txt`. Never sweep them in. Never tag.
- Build with `./scripts/build-and-install.sh O-GranulatorInstrument`. Run long commands in the background (600 s watchdog). The parity reference is always built from `git archive 82406fce` (`tests/parity/run.sh`).

---

## Summary (what the plan must absorb)

1. **The wet knee was never the parent's** (resolved this session: adopt). Our wet safety is `kWetCeil·tanh(y/kWetCeil)` (`Engine.cpp:1523-1530`, `GiConstants.h:66`). The parent D9 knee is `wetKnee`: identity for |x| ≤ `kKneeT` = 0.501, then `T + (A−T)·tanh((|x|−T)/(A−T))` (`82406fce:…/OutputLaw.h:182-188`). It is already in our VERBATIM fork (`forked/OutputLaw.h:182-188`). CONTEXT D23 and ARCH §18 both wrote the tanh formula while naming the parent knee. Adopting it re-baselines every non-silent render (§6.2) and should collapse the 2.2 PN residuals toward the float floor. **VERIFIED.**
2. **Q1: no new parent hooks are needed.**
   - **Reverb + knee:** `parent_render.cpp` already sets `mix 100` (`:110`), so the parent output is `knee(reverb(wet))·og`, and the 2.4 wet guard runs before the reverb (`82406fce:…/Engine.cpp:1612-1663`). Engine-level rows suffice.
   - **FeedbackChain:** the parity geometry is frozen at `kTf` (`parent_render.cpp:170-171`). A frozen parent does not push (`82406fce:…/CaptureWriter.h:95-96`), so the parent's output is unchanged at feedback > 0 while its chain runs on the live tap. The tap comes from the existing `hooks.tapProbe` (`82406fce:…/Engine.h:255-256`, written `Engine.cpp:544-551`). The return comes from `getFbDelayForTest()` (`Engine.h:317`) read after each block, with the parent rendered in 32-sample blocks (block-size invariant, its N7 row).
   - Only our files change: `parent_render.cpp` columns/modes and `ParityRows.h`. **VERIFIED** (hooks) / **INFERRED** (design).
3. **Our chain knob must be unity, with a parity hook.** The leaf multiplies its input by the knob ramp (`FeedbackChain.h:218-219`). The parent drives it at `0.95·feedback` (`82406fce:…/Engine.cpp:193, 764`) because its write is additive. Ours is convex with λ = 0.95·feedback (ARCH §17), so the chain must run at knob = 1 (`l·1.0f` is bitwise `l`), or the loop gain is applied twice. For the null, a hook `chainKnobParity` drives our chain with the parent's `0.95·feedback` ramp. **VERIFIED** (code) / **INFERRED** (consequence).
4. **The ahead-of-head guard must be ≥ 40, not 16** (ARCH Algorithm §4 says `g_a = 16`). Write-before-read writes the whole piece's frames `[h_a, h_a + len)` before any grain reads (ARCH Per-piece order step 4). So at sample n the splice sits up to 31 frames ahead of `h_n`, and a level-0 kernel reaches 8 frames ahead (`kTapsAhead`). Set `g_a = kCell + 8 = 40`. **INFERRED** **[correction]**.
5. **The region end is a permanent unsettled margin unless the mips are settled at each wrap and park.** `writeRun` recomputes level-l children only for parent indices it writes (`MipTape.h:129-162`). The head never writes past `hi`, so the children whose window reaches `hi` keep the previous content. Pitched reads (level ≥ 1) within `D(l)` of `hi` then play the old version of frames level 0 already shows as new. The same holds at a parked head. **Fix:** add `MipTape::settleRun(pos, n)`, the child update with level 0 untouched, run over `[hi, hi + margin(4))` at each wrap and `[h, h + margin(4))` at each park. It is ≤ 1074 frames, deterministic, and has no effect on pristine builds. **INFERRED** **[new piece]**.
6. **Head start/stop needs the λ ramp too, not only freeze.** With λ > 0 and r ≈ 0 the write is `(1−λ)·wc`, an amplitude step against the unwritten `wc` ahead. That splice becomes audible as soon as the parked head stops constraining reads. Use `λ_eff = 0.95·fbRamp · runRamp_s`, with `runRamp_s` a 20 ms ExactRamp on the run condition, and park only when `λ_eff` has settled at exactly 0. With this, ARCH's "parked head means no constraint" is true. **INFERRED.**
7. **Swap rule:** a same-geometry take (Reset / SR rebuild) needs a parked head. `takeTapes` already waits while a crossfade runs (`Engine.cpp:572-573`), and `pending` holds ≤ 1 tape by `exchange`. Add: if the head is not parked, force the run target false and take at the first tick after `runRamp` lands. **VERIFIED** (existing) / **INFERRED** (rule).
8. **Reset reuses the 2.1 SR-rebuild crossfade.** `takeTapes` already dual-reads when `p->rebuild && same generation` (`Engine.cpp:589-600`). A Reset tape is a rebuild at the current rate, ratio 1. The loader needs a `requestReset(slot)` that skips the "already at this rate" early-out (`SlotLoader.cpp:402`) and dedupes on an in-flight reset. **VERIFIED.**
9. **The 0.7 tape bound is structural only for `fb_saturation` ≥ 6.2 %.** The saturator bounds `|x| ≤ 1/d`, but the AGC peak follower has a 1 ms attack (`FeedbackChain.h:184-197`; `kAgcAttackS` 0.001), so a transient can pass at gain 1. At d = 2.4 (default sat 20 %) `|r| ≤ 0.417`; at sat 0, `|r| ≤ 1.0`. Recommend a Gi-side `|r| ≤ kAgcCeil` clamp in the writer (leaf untouched, counted), which makes the ARCH §3 bound true at every setting (§4.3). **INFERRED** **[decision for plan]**.
10. **reverb-unit is unchanged since the pin** (`git diff --stat 82406fce HEAD -- modules/effects/reverb-unit` empty) and already linked (`CMakeLists.txt:86-87`). It returns before touching the buffer while not running, so mix 0 is a bitwise bypass (`ReverbUnit.h:176-197`). Mix → 0 fades the wet path over 50 ms, then stops. **VERIFIED.**
11. **The parent's 2.4 rows port** (`82406fce:…/engine_gates_24b.h`): `kappa` = max|Δ²x| / RMS (`:137-138`); FZ2a/b ≤ 2.0× vs a static control with > 2.0× hook controls (`:216-244`; measured 1.19× / 1.15× vs 23.9× / 112.7×, SUMMARY-2.4); RV1–RV3 (`:470-499`). RV4 (outer mix 0) has no analogue here. **VERIFIED.**
12. **CPU projection for D25: about 28–30 % against 40 %** (§9). Parent chain + reverb + tap measured +0.77 % (C4 18.57 vs C3w 17.80, SUMMARY-2.4). Ours has 4 chains, 4 writers and a per-pair tap. **INFERRED.**

---

## 1. Render order and where 2.4 slots in (Open Q4)

### 1.1 Today — **VERIFIED**
- `Engine::process` (`Engine.cpp:420-468`): pieces cut at cell multiples (`cellOff = t & 31`); `gridTick` at `cellOff == 0`; MIDI splits the piece into segments, each running `runSegment` (envs → schedule → render); `mixPiece` once per piece (`:466`).
- `mixPiece` (`:1442-1536`): `acc_s = Σ_k (env·lawGain)·bus` (bus cleared after reading); `y = Σ_s acc_s·level·aud·vgain·pan`; then `kWetCeil·tanh`; then `·og`.
- `fbReturn` is a `std::vector<float>` (`Engine.h:496`), zeroed in `prepare` (`Engine.cpp:105`), never written. `kFbDelay == kCell == 32` (`forked/EngineConstants.h:44-45`).

### 1.2 2.4 order (ARCH Per-piece order, adapted) — **INFERRED**
1. `gridTick`: latch the 9 new globals; λ / run / fb ramps; chain cutoffs (tan only on change, parent `updateChainCutoffs` shape `82406fce:…/Engine.cpp:240-251`); the per-pair `g_fb` targets; reverb atomics; freeze edges; take tapes under the swap rule (§3.4).
2. **Record-head writes for `[a, b)`**, before any `runSegment`: per slot with `λ_eff` not settled at 0, frame `h + i ← (1−λ_i)·wc + λ_i·r[cellOff + i]`, then wrap/park handling (§3).
3. Segments as today. The spawner clamp (§2) reads the head state of step 2.
4. `mixPiece`: unchanged mix statements, plus a separate tap accumulation (§1.3) → `y` into `wet[2][kCell]` → wet guard → `reverb.process` → `wetKnee` → `·og` (§6).
5. 4 × `chain[s].process(tap_s, …)` → `fbReturn[s][cellOff + i]`. This runs every piece, frozen or not (parent rule).

- `fbReturn` becomes a fixed member `float fbRet[kGiStreams][2][kFbDelay]` (no vector). It is still zeroed in `prepare`.
- `kCell ≤ kFbDelay` is already static_asserted, so the slot index of `r[n − 32]` is `cellOff + i` (parent `CaptureWriter.h:94`).

### 1.3 Tap (ARCH §16) — **INFERRED**
- `tap_s[c] = (Σ_k a_k[c]·gFb_kS(c)·bus_kS[c]) · min(level_s(c), 1) · aud_s(c) / max(1, Σ_k a_k[c])`.
  - Accumulate it in the existing k-loop **as separate statements** (before `bl[c] = 0.0f`), gated on `a_k > 0`, with its own accumulators `tapL/R[s][c]`.
  - The mix statements `aL[c] += a * bl[c]` stay untouched, so true bypass holds.
- `gFb_kS`: a new `CellLerp gFbLerp[k][s]`, target `fbTapGain(fbTapS1(nEff, nEff, win, morphBank))`. It is computed where `lawTarget` is set (`Engine.cpp:827, 938`) from the same `nEff`. `fbTapS1` / `fbTapGain` are already in the fork (`forked/OutputLaw.h:199-212`).
- Hook `sumTap` (÷1 instead of ÷max(1, Σa)) is the runaway-guard control. Hook `fbArchLaw` (`1/max(1, S1)`) is the parent U8n analogue.
- Muted or un-soloed streams contribute 0 through `aud`. Pan and `g_out` are not in the tap (parent R31, `82406fce:…/Engine.cpp:1603`).

## 2. Record-head clamp (Open Q2)

### 2.1 margin(level) — **VERIFIED**
`MipRing<L>::margin(l) = D(l) + 8·2^l + 1` with `D(l) = 63·(2^l − 1)`, giving **9 / 80 / 222 / 506 / 1074** for l = 0…4 (`BandlimitedReader.h:270-273`; comment `:56`). This is exactly ARCH's `g_b`. Ahead of the head, level-l children are untouched old content (`writeRun` only recomputes children whose parent index it wrote), so reads at d ≥ 0 see consistent old content at every level. **INFERRED** from `MipTape.h:129-162`.

### 2.2 Forbidden set — **INFERRED**
- Grain position `p(t) = p0 + dir·r·t`. While running, the head is `h(t) = h0 + t`. Relative `d(t) = d0 + (dir·r − 1)·t` and `Δ = (dir·r − 1)·len` (ARCH §4 ✓).
- Forbidden `d0 ∈ [min(0, −Δ) − g_b, max(0, −Δ) + g_a]` mod W, with **`g_b = margin(level)`, `g_a = kCell + 8 = 40`** (Summary 4).
- The head path over the grain's life can wrap once (latched `hi_h → lo_h`), so compute the forbidden set in absolute tape coordinates as ≤ 2 arcs. If the head is outside `[lo, hi)` (the region moved since its last wrap), use plain absolute distance.
- Intersect with the existing region fit (`Engine.cpp:1253-1268`: forward `[lo, hi − reach]`, reverse `[lo + reach, hi]`). Take the feasible p0 nearest the drawn one; ties go behind the head.
  - No feasible point → keep the drawn p0 and count `forcedCrossings`.
  - `reach ≥ W` (the wrap-read branch, `:1262-1267`) is always infeasible: count it.
- The clamp consumes **no draws** and is skipped when the head is parked. So every derandomised parity row and every fb-0 render is untouched.
- The head state the spawner sees is the post-write state of step 2. Onsets inside the piece use `h_a + offset`.

### 2.3 Gates
- **HC1:** 4 streams, feedback 80 %, density 200, pitch ±24 (levels 0–3), reverse 50 %, region 2 s. Telemetry `forcedCrossings == 0`. Every live grain, every sample (test hook `headReadScan`): relative distance ∉ `[−margin(level), 40]` while the head writes.
- **HC1n (control):** the hook `headClampOff` must report scan violations > 0.
- **HC2 (forced):** region 30 ms, size 100 ms. The reach ≥ W branch must be counted with `forcedCrossings > 0`, so the telemetry is live.
- **HC3:** reverse grains at rate 2 (Δ = −3·len) in a region of exactly `|Δ| + 40 + margin + 1`: feasible, 0 crossings. One frame less: forced, counted.
- The ARCH §7 "loop-wrap sliver at lo" is **not a defect.** The children near lo filter real positional content (old frames < lo still exist). No gate exemption is needed there; the real problem is at `hi` (§3.2).

## 3. Record heads, settle, run condition, swap

### 3.1 Write — **INFERRED**
- Per frame: `w = (1 − λ)·wc + λ·r`. If `w` is non-finite, keep `wc` and count `writeScrubs` (ARCH §17).
- Optional `|r| ≤ 0.7` clamp before the write (§4.3).
- Level 0 and the children go through `MipTape::writeRun(h, n, L, R)` on a ≤ 32-frame scratch built from `levelSample(0, …)` reads. `writeRun` takes `pos` and `n` and is allocation-free (`MipTape.h:129-162`). It is not marked audio-safe today: update the header comment.
- A test accessor `MipTape::hashForTest()` hashes all levels (the data are private).

### 3.2 Settle — **[new piece]**
- `settleRun(pos, n)` is `writeRun`'s child loop with level 0 skipped.
- Call it over `[hi, hi + margin(4))` when the head wraps, and over `[h, h + margin(4))` when it parks (`λ_eff` lands at 0).
- Cost: ≤ 1074 parent frames → about 1000 children × 33 double MACs × 2 ch ≈ 70 k MAC, one burst per lap or park.
- It makes the level-l content equal to the filtered level-0 content everywhere except within margin of a running head.
- It never runs on a pristine build, so `flushTail` and the pristine hash are unchanged.
- **Gate MS1:**
  - After 10 laps at feedback 80 %, park, then settle.
  - A pitched read at levels 1–4 across `[hi − 1100, hi)` and across the park point equals a `MipSnapshot` of the level-0 content (≤ −120 dB, the ARCH MipTape gate's tolerance).
  - **MS1n (control):** `settleOff` must fail.

### 3.3 Run condition and λ — **INFERRED**
- `runTarget_s = ! freeze && tape.rate == fs && ! xfade[s].active && ! swapWaiting_s && (any counted voice || returnPk_s > −100 dBFS)`.
  - `returnPk_s` is a peak follower on `r_s`.
- `λ_eff = 0.95·fbRamp·runRamp_s`. Both are ExactRamps over 20 ms (`rampCells20`). Read them per sample at the cell position, as the chain does.
- The writer is skipped when `λ_eff` settles at exactly 0 for the piece (`ExactRamp::settledAt`, `forked/ControlGrid.h:129`). It never writes `1·wc + 0·r`, which would flip a −0.
- Freeze is the same ramp (D24): `runTarget` goes false. The chains keep running and the return is discarded (the writer is skipped).
- Hooks:
  - `lambdaRampZero` (the step control for feedback),
  - `runRampZero` (the hard head start/stop control),
  - `freezeHardStop` (λ → 0 at the edge).
- `bindFreeze`: a `freeze` bool already saved as on engages at the first tick (parent `Engine.cpp:69-73`).

### 3.4 Reset and SR rebuild (Open Q6)
- **Exists** (**VERIFIED**):
  - `SlotLoader::setSessionRate` queues rebuilds via `callAsync`, deduped by (slot, rate) (`SlotLoader.cpp:356-407`);
  - the rebuilt tape has `rebuild = true` and the same generation (`:96-97`);
  - `takeTapes` starts the 20 ms dual-read crossfade (`Engine.cpp:589-600`);
  - `voiceRate` already multiplies by `tape.rate / fs` (`:181-187`), so the pitch is right while parked;
  - `takeTapes` skips a slot with a running crossfade (`:572-573`).
- **2.4 adds** (**INFERRED**):
  - `SlotLoader::requestReset(slot)`: a rebuild from the pristine at `sessionRate`, bypassing `:402`, deduped on an in-flight reset of the same slot. "Reset all" = 4 requests. The processor exposes it (a Stage 3 native fn later; harness-callable now).
  - **Swap rule:** for a same-geometry pending tape, if `runRamp_s` has not settled at 0, set `swapWaiting_s` (run target false) and leave the tape pending. Take it at the first tick after the ramp lands. Rebuilt tapes stay in `pending` (≤ 1 by `exchange`).
  - Heads: keep the position on Reset (same frames). Reset to `lo` on a new file or SR rebuild (the frame count changes).
  - The record head never writes during the crossfade (`! xfade.active` in `runTarget`).
- **Gates:**
  - **RS1:** after 5 s at feedback 100 %, freeze on, then Reset → the hash equals a fresh loader build of the same pristine; still frozen.
  - **RS2:** Reset with feedback on but no voices (parked) → the hash equals the pristine build.
  - **RS3:** 100 `requestReset` with no `processBlock` → `pending` ≤ 1 per slot (reuse the 2.1 H2 harness).
  - **RS4:** Reset requested mid-crossfade → it queues (`xfadesStarted` increments only after the first fade ends).
  - **SR1:** 44.1 k pristine in a 96 k session with notes held at feedback 100 % → 0 writes until the new tape lands (hash unchanged), pitch ±1 cent (the 2.1 autocorrelation probe), and the head resumes after the land.

## 4. Feedback law arm (D22, ARCH Algorithm §3)

### 4.1 Rows — **INFERRED**

| row | stimulus | pass | control |
|---|---|---|---|
| FL1 stress | 60 s, 16 voices held, fb 100 %, density 200, size 500 ms, pitch +12, all streams +6 dB, res 100 %, full-scale noise tape | every tape peak ≤ max(pristine peak, 0.7); 0 NaN/Inf; 0 subnormal (all levels, end scan); knee out ≤ float(A) (D23); pre-knee peak INFO | — |
| FL1s | FL1 at **fb_saturation 0** | same bound | (§4.3) |
| FL2 runaway | same patch, 1 vs 16 voices (unison note; grains decorrelate) | tap RMS₁₆ ≤ tap RMS₁ + 1 dB | `sumTap`: RMS₁₆ − RMS₁ ≥ +6 dB (incoherent ≈ +12) |
| FL3 freeze | fb 100 %, 8 voices, freeze at a tick | hash at (ramp end + settle) == hash 10 s later | `freezeHashLate`: hash taken at the edge → differs |
| FL4 fb 0 | 60 s play, fb 0, all else FL1 | hash == pristine build (bitwise) | fb 0.1 % → differs |
| FL5 block null | scripted chord, steals, all randomisation, fb 80 %, freeze toggles ×4, 2 resets | 441 / 512 / 4096 / var vs 32: 0 diffs (output and final tape hashes) | seed + 1 differs |
| FL6 NaN | `injectFbNan` into chain s2 (the leaf `testInjectAt` path, `FeedbackChain.h:220-223`) | scrubs == 1, tape finite, the other slots' hashes equal to a no-inject reference | `writeScrubOff` + an injected NaN on `r` → a non-finite tape sample |

- FL1 peak bookkeeping must scan **every level** (the children inherit the bound: the half-band has a DC gain of 1 but can overshoot). Report the per-level max as INFO; gate on level 0 only. **[flag]**

### 4.2 Leaf nulls vs the parent (D22) — **INFERRED**
- **PB (chain):** the frozen parity geometry, 1 voice, s1, parent `feedback` f > 0, and our `chainKnobParity`.
  - Dumps: our tap + return probes vs the parent's `tapProbe` + per-32-block `getFbDelayForTest()`.
  - Precondition **PB0:** tap null ≤ −130 dB (it rests on the 2.3 PF0 read identity, −149.3 dB).
  - Rows: fb {25, 100} × low_cut {20, 80, 2000} × high_cut {1000, 12000, 20000} corners (6), sat {0, 20, 100} (3), diffusion {0, 50, 100} (3), all at matPeak 0.25. Sat 100 rows need the AGC engaged: assert `env > 0.7` reached on both sides.
  - Return null ≤ −90 dB after gain alignment, with `|g/g₀ − 1| ≤ 1e-4`.
  - The 32-sample delay row: the cross-correlation peak of the return vs our tap-driven chain sits at lag 0 on the return arrays and at lag 32 against the tap.
  - Controls (ours only):
    - low_cut × 2^(1/12);
    - `chainKnobParity` off (unity knob ≠ 0.95·f);
    - diffusion + 1 %;
    - `agcLegacy`-style AGC on the un-normalised peak;
    - return read at lag 31.
- **PR (reverb + knee):** the parity geometry with the parent `reverb_mix` > 0. Our output vs the parent output directly (no hooks, no `wetSafetyOff`, since the knees now match).
  - Rows: mix {25, 100} × size {0, 50, 100} × damp {0, 100}, plus a knee row at matPeak 0.4 with mix 100 / size 100 so that the post-reverb peak exceeds `kKneeT` (assert the parent peak > 0.6).
  - Null ≤ −90 dB, gain band 1e-4.
  - Controls: size + 1 %; `legacyWetTanh` on the knee row (must fail).
- Per-row parent render ≈ 1–2 s, about 30 rows. Run in the background with `--parity`.

### 4.3 AGC lag vs the 0.7 bound — **[decision for plan]**
- ARCH §3 assumes `|r| ≤ 0.7`. The leaf gives `|tanh(dx)/d| ≤ 1/d`, and the AGC `g = min(1, 0.7/env)` lags by a 1 ms attack (`FeedbackChain.h:192-194`), so the true bound is `|r| ≤ 1/d`.
  - At the default sat 20 % (d = 2.4), that is 0.417 < 0.7: structural.
  - At sat < 6.2 %, a sub-millisecond transient can write up to `1/d` ≤ 1.0.
- **Recommend (a):** a Gi-side writer clamp `r := clamp(r, −0.7, 0.7)`, counted (`rClamps`, INFO). The leaf stays verbatim, the PB nulls are on the return before the clamp, and the bound is structural. FL1s gates `rClamps` as INFO and the bound as PASS/FAIL.
- **(b):** restate the law as `max(pristine, 1/d)` and gate FL1s on that. This is weaker than DSP-04's wording.

## 5. Click gates (D24, QUAL-01) — parent recipe **VERIFIED** (`engine_gates_24b.h:137-244`)
- **Metric:** κ = max|Δ²x| / RMS over a window, max over L/R. Pass ≤ **2.0×** the same-motion control. Every hard-stop control must read > 2.0×, recorded.
- **Material:** a 440 Hz sine tape, spray 0, scan 0 (parent FZ2 shape), so κ is well-conditioned (on noise it isn't).

| row | event | control (static) | hard control |
|---|---|---|---|
| CK1 freeze | freeze on at a tick, fb 100 %, grains spanning the head | same render, no freeze | `freezeHardStop` |
| CK2 fb step | feedback 0 → 100 % at a tick | fb held 100 % | `lambdaRampZero` |
| CK3 head stop/start | notes off → return decays → park; then a new note | same notes, fb 0 | `runRampZero` (the splice appears once the parked head stops constraining reads) |
| CK4 reset | Reset after 5 s at fb 100 % | frozen, no reset | `resetHardSwap` (no crossfade, grains switch tape at the take) |
| CK5 reverb | mix 50 → 0 (RV2) and 0 → 50 cold (RV3) | static mix | — (the parent rows carry no hook) |

- CK3 needs grains to sweep the stop point after the park: a small region (300 ms), spray 100 %, 4 s after the park. **INFERRED.**

## 6. Reverb, wet guard, knee, output gain (Open Q5)

### 6.1 Wiring — **VERIFIED** (parent) / **INFERRED** (ours)
- **Bind** the reverb to 7 engine-owned atomics, as the parent does (`82406fce:…/Engine.h:114-116`):
  - fixed values: enabled 1, predelay 20 ms, mod 0.2, shimmer 0;
  - size, damp and mix written at grid ticks only (`writeReverbAtomics`, parent `Engine.cpp:222-235`, without the 2.5 matrix lines). That is what makes it block-size invariant (parent R35).
- **Prepare** the reverb in `Engine::prepare` (it allocates: host thread).
- **mixPiece:**
  - `y` goes into `wet[2][kCell]` (the same statements, now stored);
  - then the R96 wet guard (non-finite → 0, parent `:1612-1634`);
  - then `reverb.process(view)` over the piece;
  - then `wetKnee`, then `·og` as its own statement.
- The reverb's internal dry/wet is the instrument's `reverb_mix`. There is no outer dry path (ARCH §18).

### 6.2 Knee adoption and re-baseline (user decision) — **INFERRED**
- Hook `legacyWetTanh` restores the exact `kWetCeil·tanh` statements.
- **TB1 (true-bypass attribution):** with `legacyWetTanh` + fb 0 + reverb mix 0, renders-2.1 / renders-2.2 / FR1 / RB1 are **byte-identical** to the committed hashes. This is the parent's P33 `--legacy-24` precedent.
- Then re-pin `renders-2.4.sha256` on the shipping path, with the moved-row list in SUMMARY.
- **TB2:** with the shipping knee + fb 0 + mix 0, every render whose |y| ≤ `kKneeT` throughout (the PN parity rows) is bitwise equal to its `wetSafetyOff` render. This proves the knee is identity below −6 dBFS.
- **Expected:** the 2.2 PN residuals fall from −110.9…−120.2 dB to near PF0's −149 dB. Record the new values. That is not a gate change.
- **D23 rows:**
  - **CL1:** FL1 knee output ≤ float(A) = 1.99526227.
  - **CL2:** the carried FZ1 stimulus at 44.1 / 48 / 96 k (`engine_gates_23.h:914`), knee output ≤ float(A), pre-knee peak INFO (2.3: 1.53 / 1.71 / 1.73).
  - `output_gain` is excluded.

### 6.3 DSP-05 rows (parent RV1–RV3) — **VERIFIED** (`engine_gates_24b.h:470-499`)
- **RV1:** mix 50, size 0. Wet − (mix 0) > −40 dBFS during input; tail RT60 > 0.2 s.
- **RV2:** mix 50 → 0 at a tick. κ ≤ 2.0× static; wet − dry < −90 dBFS within 60 ms (the unit's 50 ms fade, `ReverbUnit.h:31, 161, 243`). After the fade, `running` is false and the output is bitwise equal to the mix-0 render.
- **RV3:** re-enable after 2 s off (cold tank, `clearTank`, `:186-189`). κ ≤ 2.0×.
- **RV4 (replaces the parent's):** reverb_mix 0 for the whole render → bitwise identical to the no-reverb build path (TB1 covers it).

## 7. Grid, readsets, fork
- `GiGridParams` gains `feedback, fbLowCutHz, fbHighCutHz, fbSat, fbDiff, freeze, reverbMix, reverbSize, reverbDamp` (raw Hz for the skewed cutoffs, the 2.3 rule).
  - Ranges, defaults and skews are identical to the parent (`params.tsv`, both sides). **VERIFIED.**
  - `ControlGrid.h` is ADAPTED: re-hash `FORK-MANIFEST.md` columns 5 and 7, then drift = 0.
- `FeedbackChain.h` gets **no edit.** Our engine drives `process(…, knob = unity ExactRamp, diff, satD)` with `satD = 1 + kSatSpan·sat` and cutoffs from the parent's `log2` ramps. 4 instances ≈ 133 KB each, fixed arrays (`kDiffuserMaxLen` 4167, `EngineConstants.h:142`). The Engine is heap-held (`PluginProcessor.h:233`). **VERIFIED.**
- Readset: the 9 IDs leave the unread set. Add an N2-24 row and retire N2-23 with a reason (the P16 precedent).
- Param-dump stays identical (no new params).

## 8. Allocation, NaN, memory
- **FA4** extends A2: 0 allocations across Reset per slot and all, freeze toggles, the fb 0 ↔ 100 step, an SR-change rebuild landing, steals with feedback on, a reverb enable (`clearTank`) and a settle burst. Liveness: each event's counter > 0.
- Fixed chains and fixed `fbRet`; the reverb is prepared on the host thread. `writeRun` / `settleRun` allocate nothing (`MipTape.h:129-162`). **VERIFIED.**
- **Host-idle:** RS3 (§3.4).

## 9. CPU (D25) — **INFERRED** from **VERIFIED** inputs

| term | estimate | basis |
|---|---|---|
| C23w (2.3 verify) | 26.01 % | VERIFICATION-2.3 |
| 4 chains (HP+LP+AP4+2 tanh+AGC, ×2 ch) | +0.6–1.0 % | parent chain + reverb + tap = +0.77 % (SUMMARY-2.4 C4) |
| reverb (8-ch FDN, mix 100) | +0.4–0.6 % | same |
| 4 writers (level 0 + 0.94 children/frame × 33 double MAC × 2 ch) | +0.3–0.5 % | ≈ 12 M MAC/s |
| tap accumulation (64 pairs × 2 MAC) | +0.2 % | 6 M MAC/s |
| clamp per spawn, settle bursts | < 0.2 % | — |
| **C25w** | **≈ 28–30 %** | vs **40 %** |

- Method: `--cpu25` runs C23w and C25w in one session, run alone, warm-up + 7 × 10 s, live ≥ 240, duty ≥ 0.9, calibration witness vs `tests/golden/cpu-calib-2.3.txt`. NOVERDICT = FAIL.
  - **The writers must actually run:** assert `writes > 0` on all 4 slots per measured window. Notes are held, so the run condition holds.
- **C25t:** the C23t proxy with reverb at its default (mix 0) → ≤ 12 % regression row.
- **C25r (INFO):** Reset every 200 ms on s1 during C25w (the dual-read fade = 2× reads on one stream for 20 ms) → Δ vs C25w.
- The D21 ladder and "fallback asks" carry over; not expected to fire.

## Pitfalls checklist (PLAN must-haves)

| # | pitfall | must-have |
|---|---|---|
| P1 | chain knob 0.95·fb + convex λ → loop gain applied twice | unity knob; `chainKnobParity` for the PB null only |
| P2 | `g_a = 16` misses the in-piece written-ahead span | `g_a = kCell + 8 = 40`; HC1 scan + HC1n |
| P3 | children past `hi` / past a parked head stay stale → pitched splice | `settleRun` at wrap and park; MS1 + `settleOff` control |
| P4 | hard head start/stop leaves a `(1−λ)wc` amplitude splice | `runRamp` 20 ms; park only at λ_eff == 0; CK3 + `runRampZero` |
| P5 | writer runs `1·wc + 0·r` at fb 0 → −0 flips | skip on `settledAt(0)`; FL4 bitwise |
| P6 | Reset swapped while the head writes | swap rule: wait for `runRamp` to land; `! xfade.active` in runTarget |
| P7 | knee re-baseline hides an unrelated hash move | `legacyWetTanh` TB1 byte-identity first, then re-pin with the moved-row list |
| P8 | AGC 1 ms attack → `|r|` up to 1/d | writer clamp at 0.7 (recommended) + FL1s at sat 0 |
| P9 | reverb atomics written mid-cell → block-dependent | write at grid ticks only; FL5 includes reverb on |
| P10 | NaN into the reverb tank (unscrubbable) | wet guard before `reverb.process` |
| P11 | tap statements merged into the mix → FX-off / fb-0 hashes move | separate accumulators; TB1 |
| P12 | `ControlGrid.h` edit not re-hashed | FORK-MANIFEST cols 5/7; drift 0 |
| P13 | CPU row with parked heads (writers idle) | assert writes > 0 on 4 slots in C25w |
| P14 | freeze hash taken before the settle burst | hash at the first sample after ramp end **and** settle |
| P15 | FL1 checks only level 0 | scan every level for NaN/subnormal; peak per level INFO |
| P16 | Standalone stale | build `OuariconGranulatorInstrument_Standalone` explicitly |

## Open items for the planner
1. **Decide P8:** writer clamp (a, recommended) vs the restated bound (b). This is a technical call; ask only if the plan wants the weaker DSP-04 wording.
2. Freeze the row thresholds above (κ 2.0×, FL2 +1 dB / +6 dB, PB/PR −90 dB + 1e-4 band, MS1 −120 dB) and the CK3 / FL1 stimuli.
3. **Expected counts:**
   - harness ≈ 150 + HC 4 + MS 2 + RS 4 + SR 1 + FL 8 + CK 9 + RV 3 + TB 2 + CL 2 + FA4 1 + N2-24 1 ≈ **187**;
   - parity ≈ 99 + PB0 + 12 PB + 5 PB controls + 13 PR + 2 PR controls ≈ **132**;
   - `--cpu25`: 2 verdict rows + 1 INFO.
4. The re-baseline is a separate commit: TB1 first, then the re-pin, so the attribution is reviewable.
5. HANDS-ON-2.4 items: feedback character at sat 0 / 20 / 100; freeze; reset smoothness; reverb tail on mix → 0; chords thinning in the copy (the ARCH §3 mean-tap consequence; the `1/sqrt(Σa)` fallback stays a hands-on call).
