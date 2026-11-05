# Stage 2 — DSP · Phase 2.4 (Freeze, feedback network, output reverb) — Research

**Plugin:** O-Granulator
**Stage / phase:** 2-dsp / 2.4 of 6
**GSD phase:** research
**Date:** 2026-11-05
**Inputs:**
- `CONTEXT-2.4.md` (D9–D12 locked; six open questions)
- `RESEARCH-2.3.md`, `VERIFICATION-2.3.md` (deviation 4, B2)
- ARCHITECTURE §2, §3, §10–§14, Algorithm §1, §3, §4
- the 2.3 engine in `Source/dsp/`: `Engine.cpp` (`process`, `spawn`, `renderSegment`, `mixOut`), `Spawner.h` (`plan`, `fill`), `CaptureWriter.h`, `GrainFx.h` (`svf`, `ring`), `OutputLaw.h` (`streamPower`), `ControlGrid.h` (`panGains`)
- grain-engine 1.1.0 `BandlimitedReader.h` (`MipRing::locateStart`, `margin`, `levelSample`)
- reverb-unit 1.1.0 `ReverbUnit.h`; JUCE 8.0.15 `juce_SmoothedValue.h`, `juce_AudioSampleBuffer.h`
- render harness `main.cpp` (B2 rows)

**Scratch experiments:** `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/6d3802f8-717f-402c-8e8a-05c34156d29e/scratchpad/r24/`

| Script | What it does |
|---|---|
| `ripple.py` | peak of the periodic window sum Σw against the coherent tap law, for the four bank window families; the unimodal-bound law |
| `svfpeak.py` | D10: LP/HP peak of the bilinear TPT SVF across k and fc (20 Hz … 0.45·fs), analytic and float32 time domain |
| `knee.cpp` | D9: exhaustive float32 check of the knee (identity, monotone, y ≤ x, ceiling), branch vs branch-free, cost |
| `freeze.py` | engage geometry: grains that cross the stopped head, grains that cross even inside a 5 ms release, frozen spawns piled on the reach floor; release-splice step |
| `crest.py` | frozen-cloud crest factor and autocorrelation at the spawn period, for several start-position supports |

All six open questions are answered below. There are **sixteen findings** (R23–R38). Six of them would make 2.4 fail, or a 2.4 gate pass vacuously, if missed: R23, R24, R26, R28, R29 and R30.

---

## Summary for the planner

| # | Finding | Severity | Plan action |
|---|---|---|---|
| **R23** | `Spawner::fill` anchors every grain at `onsetSample + 1`, which is **absolute time**. While frozen the ring head stops but `absCount` keeps going, so every grain spawned during or after a freeze is anchored in the wrong domain, off by the frozen duration. `process()` also asserts `writeCount == absCount + len` | **Blocks FUNC-02** | Anchor in the **ring domain**: `W_on = W_pieceStart + offset + 1` when live, and `W_frozen` when frozen. The ring lags `absCount` by a whole number of cells after any freeze. Drop or rewrite the assert |
| **R24** | The frozen reach floor is `margin + r·len + 1` (the head no longer advances), not `(r − 1)·len`. Under the plain ARCH clamp at **position 0 (the default)**, 25 % of frozen spawns pile onto one D at defaults, and 100 % at spray 0 or pitch +24. They are identical grains, so they buzz, and **kFreezeMinSpray cannot help** | **Defeats mitigation 1** | While frozen, measure position **from the floor**: `D = floor_frz + reflect (x, A)`, where A is the span left after the floor. Reverse grains are capped at `D + r·len ≤ heldCaptured` |
| R25 | Forced release (Q1): a forward grain crosses iff `δ − r·m < margin(level)`, with δ its current delay and m its remaining length. Reverse grains never cross. That is 12.5 % of live grains at defaults and 100 % at position 0 / spray 0. **The 5 ms fade does not fit inside the margin**: it needs r·240 samples of travel, and the margin is 9–222. The ring then clamps the delay and the grain reads one stuck sample | Q1 closed | Release the crossers, and **reflect at the head** in `readGrain`: `delay < margin → 2·margin − delay`. This is value-continuous and exact under R1. The `reachViolations` diag must not count these reads |
| **R26** | The release crossfade `ring[W − heldSpan]` splices the newest held sample onto the oldest one. That writes a step into the ring: median −18 dBFS and p99 −7 dBFS on −18 dBFS noise. Every grain that later reads across it clicks | **QUAL-01 gate fails** | Use the **mirror** `ring[2·W0 − 1 − W]` as the held continuation during the 10 ms fade. The step is 0 by construction |
| R27 | kFreezeMinSpray × tiny heldSpan (Q2). With the R24 origin, a ±½-grain support already decorrelates: autocorrelation at the spawn period is 0.00, against 0.998 at spray 0. When the span left after the floor is below r·len/2 (freeze 0.5 s after load with 500 ms grains), grains must be allowed into the never-captured (zero) region, or they collapse onto one start | Q2 closed | `w = max (spray·span, r·len)`; reflect into `[0, max (A, r·len/2)]`. Latch `heldCaptured` on the rising edge and take `span = min (spanSamples, heldCaptured)` at every grid point |
| **R28** | **The crest-factor anti-buzz gate is vacuous on noise.** A perfect buzz (every grain identical, autocorrelation 0.998) gives a crest ratio of **0.77–0.93** against the live wash, which passes "≤ 1.5×" | **Gate passes vacuously** | Gate anti-buzz on **normalised autocorrelation at lag H** (the spawn period): ≤ 0.2 on the subject, ≥ 0.9 on a negative control with the min-spray and origin shift disabled. Keep crest as the ROADMAP row, but it is not the detector |
| **R29** | The coherent tap law normalises the **mean** Σw. The **peak** of a periodic (jitter 0) Σw is higher: +0.42 dB for Hann, **+4.8 dB** for Tukey/Trapezoid, +3.7 dB for Expo. Position 0 / spray 0 gives a 42-sample loop that is near phase-locked to the spawn grid, so the linear regime grows by up to +4.3 dB per generation | **Decay gate fails** | Use the unimodal bound **`g_fb = 1 / (S1 + 1)`**. It is provable for every unimodal window with peak 1, tilted or not. Worst peak gain is −0.17 dB, so −0.62 dB per generation with the 0.95. The cost is −0.3 to −6 dB of loop level. **ARCH §10 erratum; needs sign-off** |
| **R30** | Per-grain FX add small-signal gain inside the loop: drive up to **+12 dB** (R15), LP/HP up to **+10 dB** after D10, and crush up to ×2 around ½ LSB. "dB/generation < 0 at every corner in the linear regime" is **unattainable** with these on | **Decay gate fails** | Scope the linear-decay gate to corners whose FX small-signal gain is ≤ 1 (Off, BP, LP/HP at res ≤ 30.8 %, drive 0, crush 0, any ring). Gate drive/resonance/crush corners on **boundedness**: they sustain at the guard level by design. Add this to the listening checkpoint |
| R31 | Tap details: stream pan (balance) reaches √2 per channel, so leave it out of the tap. Use S1 **without** the spread factor (conservative). The fb HP/LP must be Butterworth, k = √2. float(√2) sits 1.7e-8 **below** √2, so use the next float up | Plan detail | `tap_s = g_fb_s·level_s·aud_s / max (1, Σ level·aud)`, ramped. `kFbK = 1.41421366f` |
| R32 | Diffuser credit: the group delay of each allpass section spans **0.18 M – 5.67 M** (g = 0.7). The 48.6 ms sum is the frequency **average**, not the maximum. dB/generation has the **same sign** as dB/s, so the `< 0` gate does not depend on the credit at all | Gate wording | Gate on dB/s < −margin, and report dB/gen (sum credit) as INFO. Rounded lengths: 226/398/667/1042 at 48 k (48.60 ms) and 207/366/613/957 at 44.1 k (48.59 ms) |
| R33 | D10 (Q6): **P(k) = 1/(k·√(1 − k²/4)) for k < √2, else 1. It depends on k only.** The bilinear map preserves the analog peak, which was confirmed at 20 Hz … 0.45·fs to ±0.001 dB and in the float32 time domain. m == 1 exactly for res ≤ 30.83 %. Res 100 % gives **+10.005 dB** | Q6 closed, 1-D latch | `m = 1/√P` in double at spawn, cast to float, as a new `Grain::svfMake` (default 1.0f). LP `m·v2`, HP `m·(x − k·v1 − v2)`. LP/HP rows with res > 30.83 % re-baseline |
| R34 | D9 (Q4), checked exhaustively: identity is **bitwise on all 2.11 × 10⁹ floats** with \|x\| ≤ T, and the output is monotone with y ≤ x. **max y == float(A)**, reached by 136 M floats, so the gate is `≤ A`. The discrete slope reads 2.0 from ulp steps. Cost is 0.006–0.05 % of a core (2 channels) | Q4 closed | Branch form, `std::tanh`. No SIMD need, because the knee runs on the 2-channel wet bus and not per grain. Gate the DSP-06 peak on the wet path (mix 100 %, output 0 dB) |
| R35 | ReverbUnit (Q5): at reverb_mix 0 it **never runs** (`mix > 0.001` gate), so the output is bitwise without any skip logic. Smoothing is per sample; same-target `setTargetValue` is a no-op; and targets move only at cell starts, so it is block-invariant. Re-enable is a **cold tank** with a 0 → 1 ramp, which gives no click. The idle hand-off can differ by **−0 vs +0** across splits | Q5 closed | Value-compare (max\|Δ\|), not memcmp, in PERF-03. Run the reverb even on `mixOut`'s mix-0 fast path. Mix → 0 fades the tail out in 50 ms (no step); the module has no "let it ring" mode |
| R36 | Feedback 0 neutral null: the chain on zero input yields ±0, and `x + (−0)` keeps x while `−0 + (+0)` flips it. Store `y + 0.0f` (canonical +0) into `fbDelay`. AGC at env 0 gives `min (1, 0.7/0) = 1`, which is finite | Null hygiene | `fbDelay[ch][c] = y + 0.0f;`. The neutral null then holds bitwise with the chain always running |
| R37 | Hot fbDelay on release (Q3): the loop's per-generation gain belongs to the law (R29/R30), not to the freeze history. The frozen period only sets the initial condition. The return is bounded by the AGC and faded in by the crossfade weight w | Q3 closed | One decay row: freeze 5 s at feedback 100 %, release, remove input. The decay must match the steady-loop dB/gen within ±0.2 dB |
| R38 | B2 seed and the CPU estimate. B2: `q->setSeed (…)` after `make()`, before `prepareToPlay` (`main.cpp:374`). CPU: feedback chain ≈ 0.25 % and reverb ≈ 0.4 % → **C4 ≈ 18 %** | Carry-in, INFO | Seed B2. The C4 row reports the measured value |

---

## 1. Freeze — the ring-domain head (R23)

`Engine::spawn` calls `Spawner::fill (…, onsetSample + 1)`, with `onsetSample = absCount + offset`. `fill` sets `readAbs = (W_on − D) + step·frac`, and `readGrain` forms the delay from `ring.getWriteCount()`. The design relies on `ring.getWriteCount() == absCount` at every piece start, and `process()` asserts it.

A head-stop freeze breaks that identity on the first frozen cell:
- **While frozen,** `absCount` advances and `writeCount` does not. A grain spawned t samples into the hold gets `readAbs` t samples **ahead** of the stopped head. The ring clamps the delay to `margin`, so the grain reads a stuck sample.
- **After release,** the lag is permanent. Every later grain is anchored `lag` samples too recent: the reach floor is violated, and the position knob is offset.

**Fix: anchor in the ring domain.**

```
W_piece = ring.getWriteCount()   // read BEFORE step 3 writes the piece (live), or the stopped head (frozen)
live   : W_on = W_piece + offset + 1        // == onsetSample + 1 while no freeze has happened
frozen : W_on = W_frozen                    // every frozen spawn sees the same head
```

- Pass `W_on` into `fill`. The live path is bitwise unchanged before any freeze, because `W_piece == absCount`.
- The spawn log's `writeHead` becomes `W_on`. `anchor = W_on − D` keeps its meaning.
- Replace the assert with `writeCount == W_piece + (frozen ? 0 : len)`.
- **Block-size invariance.** Freeze edges are sampled at grid ticks, so every frozen run is a whole number of cells, and `absCount − writeCount` is always a multiple of 32. Every per-sample quantity indexed from the ring head is therefore a pure function of the input and the tick sequence.
- The R1 read stays exact: `delay = W − ⌊readAbs⌋ − fq` uses only integers below 2^53 and a dyadic fraction.

## 2. Freeze — reach floor and frozen origin (R24, R27)

**The head no longer advances, so the forward floor changes.** A live forward grain's delay falls by (r − 1) per sample. A frozen one's falls by **r** per sample. The frozen floor is therefore:

```
forward : D ≥ margin + r·len + 1                     (live: margin + max (0, (r − 1)·len) + 1)
reverse : D ≥ margin + 1  and  D + r·len ≤ heldCaptured   (don't read never-captured silence)
```

**The ARCH clamp piles grains up at position 0** (`freeze.py`). Position 0 is the default. Plain `D = max (D0, floor)` maps every draw with `D0 < floor` to the **same** D. With the head stopped, the same D means the same `readAbs`, so those grains are identical and the result is the head-stop buzz (memory: freeze needs loopback write, case 1).

| position | spray | size | pitch | frozen spawns piled on the floor |
|---|---|---|---|---|
| 0 | 10 % of 8 s | 100 ms | 0 | **25 %** |
| 0 | 10 % | 100 ms | +12 | 50 % |
| 0 | 10 % | 100 ms | +24 | **100 %** |
| 0 | 0 (min-spray ±½ grain) | any | any | **100 %**: min-spray is folded entirely below the floor |
| 0.5 | any | any | any | 0 % |

**Fix (recommended): measure position from the floor while frozen.**

```
floorOff = r·len + 1                            (forward; 0 for reverse)
A        = max (span_frz − floorOff, r·len / 2) (span left after the floor; R27 lower bound)
w        = max (spray·span_frz, r·len)          (kFreezeMinSpray = ±½ grain of source)
x        = position·A + w·(u − ½)               (PosSpray drawn even at user spray 0 while frozen)
D        = margin + floorOff + reflectInto (x, A)
```

- `span_frz = min (spanSamples, heldCaptured)`, re-taken at every grid point. `heldCaptured = W_stop − margin(2) − 1` is latched on the rising edge, so `buffer_length` still works while frozen without any loop length moving under a read.
- **The draw.** At user spray 0, the frozen path draws `Slot::PosSpray`. CounterRng is keyed by slot, so no other draw moves. Live renders never take this path, so the neutral null is untouched.
- **`reflectInto` needs |w/2| ≤ A/2 for its single fold.** `w ≤ A` holds whenever `spray·span_frz ≤ A`. Otherwise clamp `w = min (w, A)` before the fold.
- **Tiny heldSpan (R27, Q2).** Freeze 0.5 s after load with 500 ms grains leaves A ≈ 0. Without the `r·len/2` floor on A, every grain lands on the floor (a buzz). With it, the grains spread across half a grain of source, and part of each grain reads the zero region the ring had before the first push. The result is decorrelated and not silent; its level is lower until enough has been captured. That is the honest behaviour of a freeze pressed right after load. The "0.5 s after load is not silent" row passes at default and at 500 ms sizes. Gate R28's autocorrelation **only** where `A ≥ r·len`.

**What decorrelates (`crest.py`, noise source, rate 1, Hann, N = 1 and 5.6):**

| frozen start support | autocorrelation at the spawn period H | crest vs live |
|---|---|---|
| 0 (spray 0, pile) | **+0.998** (buzz) | 0.77–0.93 |
| r·len/2 (±½ grain, folded at position 0) | −0.001 | 0.92 |
| r·len | +0.002 | 0.99 |
| 2·r·len | 0.000 | 1.02 |

±½ grain is enough once the origin sits on the floor. CONTEXT's kFreezeMinSpray is kept as written.

## 3. Freeze — engage: forced release (R25, Q1)

At the rising-edge tick, for every live grain (non-killing, live slots), with `δ = W_frozen − readAbs` and `m = len − age − frac`:

```
forward (dir +1): crosses  ⇔  δ − r·m < margin(level)      → set killStep (the 2.2 kill path, 5 ms)
reverse (dir −1): never crosses the head while frozen (δ grows by r per sample)
```

- Fade-slot grains are already fading (5 ms). Apply the same test to them; they just keep fading.
- Measured share of crossers among live grains (`freeze.py`): **12.5 %** at defaults, 37.5 % at +12 st, 87.6 % at +24 st, **100 %** at position 0 / spray 0, and 0 % at position 50 %.

**The 5 ms release does not fit inside the read guard.** While frozen, a releasing grain still travels r·240 samples (at 48 k) toward the head, but the margin is only 9 / 80 / 222 samples (levels 0 / 1 / 2). At position 0 / spray 0, 100 % of the crossers cross **during** the release itself. `MipRing::locateStart` clamps `delay` to `margin`, so the grain then reads one stuck sample for the rest of the fade. That is a DC hold, heard as a thump.

**Fix: reflect at the head.** Inside `readGrain`, after the R1 delay is formed:

```
if (delay < margin(level))  delay = 2·margin(level) − delay;   // ping-pong: plays back out of the head
```

- **Value-continuous.** The read turns around at the head instead of sticking. The level-0 margin is an integer, so the R1 exactness holds.
- **Pure function of the delay.** It is block-invariant and costs one compare per read.
- **Unreachable when live.** The live floor guarantees `delay ≥ margin`, so the live path is bitwise unchanged. The N0 neutral null proves it.
- **Diag.** `reachViolations` currently counts `delay < margin`. Count **pre-reflection** crossings by **non-killing** grains only: a killing grain is allowed to reflect, while any other crossing is a spawner bug. Add a separate `headReflections` counter as INFO, with an expected value > 0 at position 0 / spray 0 engages.
- Without the R24 frozen floor, frozen **spawns** would reflect too. With it, they never reach the head, which the `headReflections == 0` row checks for grains spawned while frozen.

## 4. Freeze — release crossfade (R26)

CONTEXT and ARCH write the held continuation as `ring.levelSample (0, ch, writeCount − heldSpan)`. At the first released sample (W = W0), that pushes `ring[W0 − heldSpan]`, **the oldest held sample**, directly after `ring[W0 − 1]`, the newest. The two are unrelated:
- On −18 dBFS noise the step at the splice has a median of **−18.4 dBFS** and a p99 of **−6.8 dBFS** (`freeze.py`).
- That step is **written into the ring**. Every grain that later reads across W0 hears it, for up to 41 s.
- The 10 ms fade only blends *after* the splice; it does not remove it.

The O-ReverseDelay loopback design did not have this problem: its copy source is continuous with what it writes. The head-stop design needs a different continuation:

```
mirror (recommended):  held[W] = ring.levelSample (0, ch, 2·W0 − 1 − W)     for W ∈ [W0, W0 + 10 ms)
push = w·(in·g + fb) + (1 − w)·held[W]        w = raised-cosine 0 → 1 over 10 ms, per sample from the release tick
```

- **Step 0 by construction.** The first pushed sample repeats `ring[W0 − 1]`, so only the slope reverses, inside a 10 ms fade.
- The mirror reads at most `fs·10 ms` back (480 samples @ 48 k), so it is always inside captured material. Clamp at `heldCaptured` for a freeze released within the first 10 ms after load.
- **Indices are ring-domain** (R23), and w is indexed from the release tick, so the fade is block-invariant.
- The feedback return enters through `w`. The fbDelay is hot after a frozen run (R37), but it fades in with the input.
- **Gate (QUAL-01).** The FUNC-02 click row compares freeze on → off against a same-motion control (freeze held, position automation identical). The **negative control** is a hook selecting the `W − heldSpan` continuation, which must exceed the κ threshold. That is the splice this finding removes.

## 5. Anti-buzz gate (R28)

The memory pattern says a periodic pulse train "measures several times" the live crest. That holds for sparse transient grains. On a **noise source** at N = 1 and N = 5.6, the pure head-stop buzz has a **lower** crest than the live wash (0.77–0.93). "Frozen crest ≤ 1.5× live" therefore passes with the buzz fully present (memory: stimulus below threshold is vacuous).

The quantity that separates them is periodicity at the spawn period H = fs / density:

```
ρ_H = Σ y[n]·y[n + H] / Σ y[n]²     (mean removed, frozen render, ≥ 10 s)
```

| | ρ_H |
|---|---|
| buzz (spray 0, no fix) | +0.997 / +0.998 |
| ±½ grain support with the R24 origin | ≈ 0.00 |

- **Gate:** ρ_H ≤ 0.2 on the subject (spray 0, position 0, the R24 + R27 path), at jitter 0 so that H is exact.
- **Negative control:** a `freezeNoMinSpray` hook that removes both the min-spray and the floor origin must give ρ_H ≥ 0.9.
- The crest ≤ 1.5× row stays, because it is the ROADMAP wording. Record it as passing but **non-diagnostic** on noise.

## 6. Feedback tap law — window-sum ripple (R29)

The ARCH tap law is `g_fb = 1/max (1, N_eff·μ₁)`, which is `1/max (1, S1)` in `streamPower`'s terms. S1 is the **mean** of Σ_k w(t − kH). At jitter 0 the sum is periodic, and its **peak** is what a phase-locked loop recirculates.

Phase lock is easy to reach. At spray 0 and rate 1 every grain reads at the same D, so the loop is the multiplicative recurrence `s(n) = 0.95·g·Σw(n − 32)·s(n − 32 − D) + …`. Its period is T = D + 32, which is 42 samples at position 0. T ≪ H, so successive generations see almost the same phase of Σw.

`ripple.py`: the peak Σw × g_fb, in dB, as a function of overlap N.

| window | μ₁ | N = 1.25 | 1.5 | 2.0 | 2.5 | 3.0 | 4.0 | 6.0 | worst, N ∈ [0.05, 40] |
|---|---|---|---|---|---|---|---|---|---|
| Hann | 0.500 | 0.00 | 0.00 | 0.00 | +0.40 | 0.00 | 0.00 | 0.00 | **+0.42** |
| Tukey 0.25 | 0.875 | +4.37 | +3.66 | +1.16 | +2.17 | +1.16 | +1.16 | +0.40 | **+4.78** |
| Trapezoid | 0.875 | +3.30 | +3.66 | +1.16 | +1.50 | +1.16 | +1.16 | +0.14 | **+4.66** |
| Expo-Decay | 0.205 | +0.15 | +0.28 | +0.65 | +1.19 | +1.70 | +2.79 | +3.01 | **+3.71** |

- With the 0.95 (−0.45 dB), Hann ends at −0.03 dB per generation, which barely decays. Every other window family **grows** by up to **+4.3 dB per generation** in the linear regime.
- The tanh and the AGC still bound it (DSP-06 holds), but the CONTEXT row "dB/generation < 0 at every corner, at −52 dBFS" **fails** at window ≥ ⅓ / spray 0 / jitter 0 corners.

**The unimodal bound.** For any unimodal w with max 1, `Σ_k w(t − kH) ≤ (1/H)∫w + max w = S1 + 1`. This covers every bank window and every tilt, because the warp preserves unimodality.

**Recommended law (ARCH §10 erratum):**

```
g_fb_s = 1 / (S1_s + 1)       S1_s = streamPower's s1 BEFORE the spread factor (capped form when N > q)
```

| | ARCH law | bound law |
|---|---|---|
| worst peak gain (all windows, N ∈ [0.05, 40]) | +4.78 dB | **−0.17 to −0.24 dB** |
| per generation, with the 0.95 | up to +4.3 dB | ≤ −0.62 dB |

- **Cost** (mean loop gain vs the ARCH law, Hann): −1.9 dB (N = 0.5), −3.5 (N = 1, the default), **−6.0 (N = 2)**, −4.4 (N = 3), −1.6 (N = 10), −0.3 (N = 50).
- Context for N = 1: the ARCH law already loses 6.5 dB per generation there, because S1 = 0.5 means the grains cover only half the time.
- **Needs sign-off at plan.** This makes feedback tails shorter at mid overlap.
- **Refinement (if the listening checkpoint finds feedback too short):** a per-step ripple allowance table `r(step) = sup_N (peak − S1)`, used as `1/max (1, S1 + r)`, built with the bank in the constructor (Hann r ≈ 0.06). Tilted windows keep r = 1. This is tighter but adds a table and a proof per step, so it is not recommended for 2.4.
- **Jitter > 0 is not the case to worry about.** Random onsets break phase lock (E[log a] < log E[a]), and both laws decay there. The bound law matters at jitter 0, which is the case the gate must cover.
- The capped law (N > q) stays: kills truncate windows, and a truncated window is still bounded by its peak 1.

## 7. FX gain in the loop (R30)

The bound in §6 assumes per-grain FX gain ≤ 1. 2.3 ships FX that exceed it on purpose:

| stage | worst small-signal gain | source |
|---|---|---|
| drive | **+12.05 dB** (d·norm = 4 at drive 100 %) | R15 |
| LP / HP res 100 % | **+10.0 dB** after D10 | R33 |
| crush | up to ×2 for samples just above ½ LSB (rounding) | D7 |
| BP (k·v1), ring, gain_rand, spread | ≤ 1 (ring \|1 − m + m·cos\| ≤ 1) | D8, 2.3 |

At drive 100 % and feedback 100 %, a −52 dBFS signal comes back about +11.6 dB louder every generation until the D12 tanh and the AGC catch it. The loop **self-sustains** at the guard level. That is bounded, so DSP-06 is met, but "dB/generation < 0 at every corner" is unattainable for these corners.

**Recommended: scope the gate; don't compensate.**
- **Linear-decay gate:** every corner whose FX small-signal gain is ≤ 1: filter Off, BP, LP/HP at res ≤ 30.83 %, drive 0, crush 0, any ring, any window, spray, jitter or pitch, plus diffusion and tone at their extremes.
- **FX-gain corners** (drive > 0, LP/HP res > 30.83 %, crush > 0): gate on **boundedness**. Peak ≤ +6 dBFS on the wet path, every sample finite, no denormal state, and the return envelope converging to ≤ the AGC ceiling (0.7 + the 1 ms attack overshoot, which is bounded by 1/d).
- **Listening checkpoint, new item 7:** drive + feedback sustain. Is a self-sustaining driven loop a feature (drone) or a fault?
- **Alternative (not recommended):** divide the tap by the stream's latched FX gain, `G = (drive ? d·norm : 1)·(LP/HP ? √P(k)·m : 1)`. That restores the linear guarantee, but it makes loud driven loops 12 dB quieter, which is the "saturation = quieter" effect D12 is trying to avoid.

## 8. Tap, chain and constants (R31, R32, R36)

**Tap** (pre-g_out, ×g_fb, at step 6):

```
tap[ch][i] = Σ_s bus[s][ch][i] · fbTap_s.at (c)
fbTap_s    = g_fb_s · level_s · aud_s / max (1, Σ_s' level_s' · aud_s')   (CellLerp; g_fb ramped 20 ms like g_out,
                                                                           level/aud the 10 ms mixer ramps' currents)
```

- **No stream pan in the tap.** `panGains` is a √2 constant-power balance, so a hard-panned stream would feed one channel of the loop at +3 dB.
- **S1 without the spread factor.** Per-grain spread gains reach √2 per channel, but their pan is random per grain, so they cannot phase-lock. The unscaled S1 is the conservative mean.
- **Mute / solo.** A muted or un-soloed stream does not feed the loop.

**Chain** (per sample, float, per channel; ARCH §12 order):

```
x  = tap·g_knob                g_knob = 0.95·feedback (ExactRamp, cell-lerped)
x  = HP_tpt (x)                k = kFbK; g = tan (π·fc/fs) at ticks, fc ramped at the grid
x  = LP_tpt (x)                fc ≤ 0.45·fs
x  = (1 − m)·x + m·AP4 (x)     m = fb_diffusion; allpasses 226/398/667/1042 @ 48 k, g = 0.7
x  = tanh (d·x)/d              d = 1 + 7·sat (D12)
env = max (|xL|, |xR|) > env ? a_att·env + (1−a_att)·peak : a_rel·env + (1−a_rel)·peak   (stereo-linked)
x  *= min (1, 0.7/env)         env == 0 → 0.7/0 = +inf → gain 1 (finite; no special case)
if (! isfinite (xL + xR))  reset chain, x = 0
fbDelay[ch][c] = x + 0.0f      (R36: canonical +0)
```

- **`kFbK = 1.41421366f`.** That is the next float **above** √2. float(√2) = 1.41421354 is 1.7e-8 below √2, which leaves a 1e-15 peak (harmless, but not ≤ 1 by construction). D10's math shows k ≥ √2 ⇔ no peak.
- **HP and LP share the 2.3 SVF free function** (a CONTEXT default). The fb filters latch at ticks, not per grain.
- **Diffuser lengths** round at prepare: 226/398/667/1042 at 48 k (Σ 2333 = 48.60 ms) and 207/366/613/957 at 44.1 k (Σ 2143 = 48.59 ms). Size the buffers at prepare: 21.7 ms × 192 k = 4167 samples, × 4 sections × 2 channels.
- **AGC coefficients:** `a = exp (−1/(τ·fs))` with τ = 1 ms / 400 ms, at prepare.
- **R36, the neutral null.** With feedback 0, the chain sees `tap·0 = ±0`, and every stage maps ±0 to ±0. Storing `x + 0.0f` turns −0 into +0, and changes no other value (no `-ffast-math`, so clang keeps it). The writer then computes `in·g + (+0)`, exactly as 2.3 does, so the null holds **with the chain running**.
- **The NaN-scrub reset** zeroes the diffuser lines: about 19 KB at 48 k and 75 KB at 192 k, written with `std::fill` on the audio thread. That runs only on the fault path and allocates nothing.

**Decay measurement (R32).**
- The group delay of a Schroeder section `(−g + z^−M)/(1 − g·z^−M)` is `M(1 − g²)/(1 − 2g·cos Mω + g²)`. That ranges from M(1 − g)/(1 + g) = 0.18 M to **M(1 + g)/(1 − g) = 5.67 M**, and its frequency average is M.
- So 48.6 ms is the **average** credit, not the "maximum the chain can produce". The memory pattern says maximum; it is corrected in this session.
- dB/generation = dB/s × T_gen with T_gen > 0, so **its sign equals the sign of dB/s whatever the credit**.
- **Gate:** the decay slope from a linear fit over 2 s at −52 dBFS, after the input is removed, must be ≤ −0.05 dB/s. Report dB/gen = dB/s × (D̄/fs + 32/fs + 48.6 ms·[diffusion > 0]) as INFO, using the rounded lengths from the plugin constant (never re-typed).

## 9. D10 — closed form (R33, Q6)

- The analog prototype is H_LP = 1/(s² + k·s + 1) and H_HP = s²/(…).
- For k < √2: |H|² peaks at ω² = 1 − k²/2, with **P(k) = 1/(k·√(1 − k²/4))**, for both LP and HP.
- For k ≥ √2 there is no peak, so P = 1.
- The TPT SVF **is** the bilinear transform of that prototype, prewarped at fc. The bilinear map sends [0, ∞) one-to-one onto [0, π), so the digital response takes exactly the analog values, and its maximum is the analog maximum **at any fc**. **P depends on k only: a 1-D latch.**

`svfpeak.py`, peak over fc ∈ {20 Hz, 1 kHz, 10 kHz, 0.45·fs}:

| res | k | P (analytic) | LP peak, all four fc | m = 1/√P | LP·m peak |
|---|---|---|---|---|---|
| 0 – 30.83 % | ≥ √2 | 0 dB | 0.0000 dB | **1 (exact)** | 0 dB |
| 31 % | 1.411 | +0.0001 dB | +0.0001 | 0.999995 | +0.0000 |
| 50 % | 1.05 | +0.977 | +0.977 | 0.9453 | +0.488 |
| 80 % | 0.48 | +6.633 | +6.633 | 0.6826 | +3.316 |
| 90 % | 0.29 | +10.844 | +10.844 | 0.5357 | +5.422 |
| 100 % | 0.10 | +20.011 | +20.010 … +20.011 | 0.3160 | **+10.005** |

- The float32 time domain (res 100 %, a sine at the mapped peak, fc = 1 kHz and 0.45·fs) gives +20.011 dB, matching the analytic value.
- The threshold is res ≤ (2 − √2)/1.9 = **30.83 %**. m is continuous across it (m → 1 as k → √2).

**Implementation.**
- `svfLatch` computes `m = k < √2 ? 1/√(1/(k·√(1 − k²/4))) : 1.0` in double from the **latched float k**, then casts to float.
- Store m in a new `Grain::svfMake` (default 1.0f). Grain stays trivially copyable. H2's steal compare and H3's state check add the field.
- LP becomes `x[i] = m * v2`, and HP becomes `x[i] = m * (in − k*v1 − v2)`. Leave the `v1`/`v2` statements untouched (FP contraction is per statement). `m * 1.0f` is bitwise, so res ≤ 30.83 % renders are identical to 2.3.
- BP is unchanged, and the D8 gate re-runs as is.
- **Gate:** m == 1.0f bitwise for every res in [0, 0.3083]. The LP/HP steady-sine peak at res 100 % is +10.0 dB ± 0.1 at fc = 1 kHz and 0.45·fs. The carried LP/HP rows with res > 30.83 % are the only D10 re-baselines; list them explicitly.

## 10. D9 — the knee (R34, Q4)

`knee.cpp` checks every float32 value, with T = 0.501187234f and A = 1.99526231f (which rounds to 1.99526227f):

| check | result |
|---|---|
| identity, \|x\| ≤ T, both signs (2 113 969 054 floats) | **0 non-identical** (branch form and branch-free form) |
| monotone over (T, 2^20] (176 M floats) | 0 violations |
| y ≤ x above the knee | 0 violations |
| max y | **1.99526227 = float(A)**, reached by 136 M floats (+6.000000 dBFS) |
| slope just above T | 0.999987 (C¹) |
| max one-ulp finite-difference slope | 2.0: ulp quantisation where y ≈ x, not a defect |
| NaN / ±inf | NaN passes through; ±inf → ±A |
| branch-free `x + copysign (AT·tanh (u/AT) − u, x)` vs branch form above T | differs on 99.9 % of samples (rounding) |
| cost, stereo @ 48 k | 0.006 % (below the knee) to 0.049 % (all above) of a core |

- **Use the branch form with `std::tanh`.** The knee runs on the 2-channel wet bus, once per sample, not per grain, so a SIMD form is never needed. If 2.6 ever switches forms, that switch is a numeric change, so record it as one.
- **Gate wording (CONTEXT correction).** The ceiling is "y ≤ A", not "< A": the float output reaches A. Check slope ≤ 1 analytically, or with finite differences ≥ 1e-3. Never use one-ulp steps.
- **Gate placement for DSP-06.** Measure the peak on the wet path: mix 100 % (dry exactly 0, wet exactly 1, snapped) and output 0 dB (exactly 1.0f). At mix 50 % the equal-power dry + wet can legitimately reach +6.5 dBFS, and output gain adds up to +12 dB.
- **The neutral null's expected-diff list.** Before D9 lands, run an INFO pass on the 2.3 tree that logs max|wet| per carried row. Rows above T, plus the D10 rows, are the only rows allowed to differ. Every other carried row must be bitwise. The W2o rows sit far below the knee, so W2o stays untouched (CONTEXT carry-in).
- **Structure.** `mixOut` forms `wl` once into a per-cell `float wet[2][kCell]` (the same `wl += bus·gain` statement), then reverb → knee → `(xl·dg + wl·wg)·og` (the same statement). Keep both statements verbatim (P18 rule).

## 11. ReverbUnit 1.2.0 (R35, Q5, D11)

**Mix 0 is bitwise for free.** `process()` computes `wantActive = enabled ≥ 0.5 && mix > 0.001`. From prepare with reverb_mix 0, `running` stays false and every call returns before touching the buffer. No skip logic is needed for the neutral null.
- The reverb_mix 1 % negative control is 0.01 > 0.001, so it is active.
- reverb_mix ≤ 0.1 % is silently off: a module threshold, recorded here and not changed.

**Block-size invariance on ≤ 32-sample calls.**
- Every smoother advances per sample inside the loop.
- `setTargetValue` returns early when the target is `approximatelyEqual` to the current one, so repeated calls with an unchanged atomic never restart a ramp (checked in JUCE 8.0.15 `juce_SmoothedValue.h:284`).
- The plugin writes the seven atomics **only at grid ticks**. The piece that starts a cell always starts at the cell boundary, so a target change is always seen first by the call that starts at that cell's first sample, whatever the host block.
- The damp coefficient (`pow`) is recomputed per call. It is stateless, so that is fine.

**Two hand-offs happen at call starts:**
1. **Enable** (`wantActive && ! running`): `clearTank()`, then a 0 → 1 activeGain ramp. This happens only at a cell start (the atomics change only at ticks), so it is deterministic. The CONTEXT question "warm or cold tank on re-enable" is decided by the module: **cold**. The ramp from 0 means no click.
   - `clearTank()` memsets about 0.36 MB at 48 k (1.4 MB at 192 k) on the audio thread. That is not an allocation (PERF-01 passes), but it is a ~50–100 µs spike at the enable edge. INFO only.
2. **Idle** (`! wantActive && ! isSmoothing && current ≤ 0`): this is decided at the next call's start after the fade lands. The samples between the landing and that call compute `dry·1 + out·0`. That equals dry in **value**, but turns −0 into +0. The tank is clamped to ±2, so it is never Inf/NaN. **PERF-03 with reverb toggles must compare values (max|Δ| == 0), not memcmp or hashes.**

**Mix → 0 semantics.** `wet = mixSmoothed·activeGain`, and both ramp over 50 ms. The tail is faded out over about 50 ms, with no step, and the module has no "let the tail ring out" mode. The DSP-07 row is therefore:
- the κ step ratio vs a static control ≤ 2.0 (no step);
- the wet-minus-input energy falling to < −90 dBFS within 60 ms;
- and, on mix > 0, a tail present (RT60 > 0.2 s at size 0).

"Not cut" means no step. It does not mean the tail persists.

**Run the reverb on the mix-0 fast path too.** `mixOut`'s fast path skips the wet entirely. Calling the reverb (and the knee) before the fast path keeps its state independent of the outer mix, so automating mix up from 0 never reveals a stale or cold tank. The output stays bitwise dry, and the cost is negligible.

**Binding (D11).**
- Add `void bind (std::atomic<float>* enabled, size, damp, predelay, mix, mod, shimmer) noexcept`, which only assigns the seven pointers. The APVTS `bind` is unchanged, so 1.2.0 is a non-breaking minor version.
- The plugin owns `std::atomic<float> revEnabled {1}, revPredelay {20}, revMod {0.2f}, revShimmer {0}`, plus size/damp/mix, written at ticks from `gp` (reverb_mix/size/damp ÷ 100).
- Wrap the wet with `AudioBuffer<float>::setDataToReferTo (ptrs, 2, len)`. Two channels sit inside `preallocatedChannelSpace`, so there is no malloc (checked in `juce_AudioSampleBuffer.h:1214`).
- **Version bookkeeping:** bump `module.yaml` to 1.2.0, add a changelog entry, and register O-Granulator as a consumer at 1.2.0.

**Cost.** One tank per sample: 4 diffusion stages × 8 channels plus Householder, 8 interpolated reads and 4 `std::sin` LFOs. That is about 80 ns per sample, or **≈ 0.4 % of a core**. C4 measures it.

## 12. Freeze × feedback on release (R37, Q3)

- The loop is linear and time-varying, and its per-generation gain is set by the law (R29) and the FX scope (R30). The frozen run only supplies the **initial condition**: fbDelay holds the chain's output for the last frozen cell.
- The frozen wash is maximally self-similar, but the tap law is already the coherent (worst-case) law, and R29 bounds its peak. After release, spray returns to the user value, and the min-spray and floor origin stop applying.
- The first released generation is faded in by `w` (§4), and the chain's AGC envelope has stayed warm, so the return is ≤ 0.7.
- **Row (FUNC-05 / decay):**
  1. Freeze 5 s with feedback 100 % (input noise at −52 dBFS, linear regime), then release.
  2. At release + 10 ms, remove the input.
  3. The decay slope must equal the steady-loop slope of the same settings within ±0.2 dB/s, and be < 0.
  4. **The "chain warm" proof:** while frozen, the chain's HP/LP integrators and AGC env are non-zero and change between ticks (hook snapshot), and the ring hash is unchanged.

## 13. Block-size, allocations, carry-ins (R38)

**PERF-03 (N7).** The N6 recipe plus:
- feedback 80 %, fb_diffusion 50 %, saturation 50 %;
- freeze toggles at three ticks;
- reverb_mix 0 → 30 % → 0;
- host blocks 32 / 441 / 512 / 4096 / variable;
- max|Δ| == 0 (value compare, R35);
- a different-seed control that must differ.

Every new stage is per sample. The edges are tick-sampled, and ring lag is a multiple of 32 (R23).

**PERF-01 (M4).** The M3 coverage plus freeze toggles, feedback 0 ↔ 100 %, diffusion sweeps, a NaN-scrub trigger (hook) and reverb enable/disable must give 0 allocations. The reverb's `clearTank` and the scrub's `std::fill` are writes, not allocations.

**B2 (VERIFICATION-2.3 deviation 4).**
- In the B2 defaults loop (`tests/render-harness/main.cpp:374`), call `q->setSeed (0x5EED'B2B2'0000'0001ull)` right after `make()`, before `prepareToPlay`. The processor constructor seeds from `getSystemRandom()`, and `setSeed` overrides it before the engine latches at prepare.
- The row detail string carries the measured dB, so it then becomes deterministic, and the three B2 rows rejoin the carried diff.

**`Engine.h` banner.** Step 8 currently reads "(reverb, wet safety — 2.6)". Change it to 2.4 and add steps 6 (tap) and 7 (chain).

**CPU (C4, INFO).**

| | % of a core |
|---|---|
| C3w (measured, 2.3 verify) | 17.08 |
| feedback chain, stereo (2 SVF, 4 allpasses, tanh, AGC) | + ≈ 0.25 |
| ReverbUnit | + ≈ 0.4 |
| knee | + < 0.05 |
| **C4 (projected)** | **≈ 18** of the 30 budget |

No lever is needed. 2.5 (matrix, MPM) has about 12 % of headroom.

## 14. Pitfalls checked

- `juce::dsp::IIR` and `StateVariableTPTFilter` are not used (memory: IIR default ctor). The fb filters reuse the 2.3 SVF function.
- `fb_low_cut` (skew 0.28906) and `fb_high_cut` (skew 0.40781) are read as **denormalised Hz** from the latched `GridParams`. A unity gain is never derived from `get()` on a skewed range.
- The Nyquist clamp is 0.45·fs on the fb LP (memory: SC filter Nyquist clamp). The HP needs none (2 kHz max).
- Diffusion is a wet/dry **mix** over the chain, never a coefficient scale (memory). Each section's coefficient is fixed at 0.7.
- There are no parameter writes from listeners. `freeze` is read at ticks.
- The ring is never cleared on the audio thread. Freeze never ramps the input gain; it uses only the content crossfade (§4).
- Every per-pass array is fixed at kCell. The new `wet[2][kCell]` and `tap[2][kCell]` arrays are bracketed by canaries in test builds (the existing guard pattern).
- FTZ does not catch NaN. The chain scrub and P25 cover both entry points.

---

## Next

Ready for `/plugin-plan O-Granulator 2-dsp` (Phase 2.4). The plan must:
- land **R23 (ring-domain head) first**, then the **neutral null vs `6aed8017`**. The expected-diff list is the rows with wet above the D9 knee plus the LP/HP rows with res > 30.83 %; everything else stays bitwise. Feedback 1 %, reverb_mix 1 %, a freeze toggle and a hot wet are the negative controls;
- carry R24–R38 as tasks or acceptance rows;
- record the corrections:
  - CONTEXT: the release continuation is the mirror, not `W − heldSpan` (R26);
  - CONTEXT: the D9 ceiling is `≤ A` (R34);
  - CONTEXT: the anti-buzz detector is ρ_H, not crest (R28);
  - ARCH §10: the tap law is `1/(S1 + 1)` (R29, **sign-off**);
  - ROADMAP: the decay gate is scoped to FX gain ≤ 1 (R30, **sign-off**);
  - memory: the diffuser sum is the average, not the maximum, group delay (R32);
- add listening item 7: drive + feedback sustain (R30).
