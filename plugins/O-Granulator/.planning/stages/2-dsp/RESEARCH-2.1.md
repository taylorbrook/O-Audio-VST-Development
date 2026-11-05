# Stage 2 — DSP · Phase 2.1 (Ring + pass engine + one stream) — Research

**Plugin:** O-Granulator
**Stage / phase:** 2-dsp / 2.1 of 6
**GSD phase:** research
**Date:** 2026-11-04
**Inputs:** `CONTEXT-2.1.md` (D1–D3 locked), `research/ARCHITECTURE.md` §1–7, §10, §14, §16, Processing Chain, grain-engine 1.1.0 headers (read in full), Stage 1 `PluginProcessor.cpp` and `tests/render-harness/main.cpp`.
**Scratch experiments:** `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/c2a2bb64-8e1e-48a0-9b5e-69635a0454f7/scratchpad/rt/` (`t.cpp` and `q.cpp` delay round-trip, `b.cpp` ring cost, `w.cpp` wrap boundary, `r.cpp` dB round-trip).

The architecture is closed, so this research answers the three open questions in CONTEXT-2.1 and adds **four findings the plan must carry**. Two of them (R1 and R2) would fail Phase 2.1's own gates if missed.

---

## Summary for the planner

| # | Finding | Severity | Plan action |
|---|---|---|---|
| **R1** | `W − readAbs` delay round-trip is block-size-dependent: 32 kernel-weight mismatches per 50 M reads, which is several bit-diffs per second at 128 grains | **Blocks the PERF-03 null** | Quantise the read fraction to 2⁻²⁸ before forming the delay (0 mismatches in 50 M reads @ 48 k and 192 k) |
| **R2** | `output_gain` 0 dB after a host normalised round trip is 7.15e-7 dB, so the gain is **1.00000012f** | **Blocks the FUNC-07 bitwise null** (and the rescoped B2) | Snap \|dB\| < 1e-3 to exactly 0 for `output_gain`, `input_gain` and `s1_level`. Bypass the wet sum entirely at mix == 0 |
| **R3** | `GrainPool` steals only when all 128 live slots are busy, so `grain_limit` 32/64/96 **cannot be enforced through `obtain()`** | Functional gap (PERF-02 `grain_limit`) | Plugin-side cap: kill the furthest-along grain (D1 `killGain`, 5 ms) and call `tryObtain()`, with an `obtain(progress)` fallback (§3) |
| **R4** | Re-enabling a stream resets `GrainScheduler`, so onset indices restart at 0 and **the same random grains replay** on every re-enable | Musical defect | Key draws on a per-stream monotonic counter `epoch + onset.index`, never reset after prepare |
| Q1 | `MipRing::push` costs 38.8 ns/sample (0.19 % of a core @ 48 k). `getWriteCount()` is `int64_t` | Closed | Per-sample `push`. No block-write API is needed or exists |
| Q2 | A `buffer_length` shrink with spawn-latched D is sufficient. In-flight grains stay valid because the ring holds 41 s | Closed | Define "stale/invalid" precisely (§2). The reach floor may exceed span, and that is valid audio |
| Q3 | Output law at the cap: closed form for Hann. ARCHITECTURE §10 double-counts μ₂ | Closed (latent for 2.2/2.3) | Use the form in §4. It is identical in 2.1 because Hann's ratio is 1 |

---

## 1. Q1 — `MipRing<2>` write path and counter width

From `BandlimitedReader.h`:

- **Only a per-sample `push(inL, inR)` exists.** It runs the whole half-band cascade per input sample (level 1 on odd level-0 indices, level 2 on odd level-1 indices). There is no block-write API, and none is needed.
- **Measured** (`b.cpp`, `-O3`, 41 s @ 48 k): push **38.8 ns/sample**, which is 0.19 % of one core @ 48 k. 128 random `readStereo` calls across levels 0–2 cost **1351 ns/output sample (6.5 % of a core)**. The read side dominates, as expected. This is a useful pre-2.3 CPU datum against the ≤ 30 % budget.
- `getWriteCount()` returns **`std::int64_t`** (`count[0]`). At 192 k it overflows after ~1.5 M years. Per-level `cap` is `int`: 41 s × 192 k = 7.87 M fits.
- Capacity (measured): `prepare(41·fs)` at 48 k gives `getCapacity(0..2)` = 1 968 064 / 984 064 / 492 064, so **27.6 MB**. This matches D2.
- **Wrap boundary (measured, `w.cpp`):** after the ring has wrapped, every level reads correctly up to a delay of **at least `capacitySamples + 61`**, where `capacitySamples` is the `prepare()` argument. So the reverse reach limit "`D + (r+1)·len ≤ capacity − 64`" is exact when *capacity* means `ring.getCapacity(0)` (which is `capacitySamples + 64`). **Plan should write it as `≤ kRingSamples`**, where `kRingSamples = (int) std::ceil (kRingSeconds · fs)` is the value passed to `prepare()`. That is clearer and still has 61+ samples of slack.
- **The read clamp has no upper side.** `locateStart` clamps only `delay ≥ margin(level)`. A delay past capacity silently wraps into the oldest (or zero) region. The spawner's reach clamp is the only protection, so add a `jassert (D_end ≤ kRingSamples)` in debug and a harness row (§6).
- **Under D2 the reverse clamp never binds:** D ≤ margin + 30 s and (r+1)·len ≤ 5 × 2 s = 10 s, so D_end ≤ 40 s + 222 < 41 s. Keep it as a backstop, and put the `static_assert (kMaxSpanS + kMaxReachS + kGuardS <= kRingSeconds)` on the derivation inputs (memory: ring invariant needs a static_assert).
- `prepare()` re-runs `vector::assign` with the same size on a repeat `prepareToPlay` at the same fs, so it does not reallocate. Still, `clear()` zeroes 27.6 MB, so **never call `ring.clear()` on the audio thread** (for example from a `reset()` override or a transport-stop path). Clear only in `prepareToPlay`.
- `sincBank()` is a function-local static built on first call. `MipRing::prepare` calls it, so the first build happens on the message thread. ✓

## 2. Q2 — `buffer_length` change: is spawn-latched D enough?

**Yes, with a precise definition of "stale/invalid".**

- `span` (and so D) is read only at spawn. A sounding grain keeps its latched `readAbs` anchor and rate. Its reads stay at absolute positions that the ring still holds, because D_end ≤ 40 s < 41 s (§1). So a **shrink cannot invalidate an in-flight grain**. It reads real recorded audio older than the new span and finishes.
- **No click from the span itself.** The span never multiplies the signal. It only places future grains, and each grain is windowed. `buffer_length` is in the spawn-latched destination class (§16), so it needs no base ramp. The grid value at the spawn sample is enough.
- **New grains after a shrink:** `D = margin(level) + reflect(pos·span + spray·span·(u − ½), [0, span])`, then the reach floor. So D ≤ margin + span_new **unless the forward reach floor is higher**.
- **The forward reach floor can exceed the span.** At +24 st (r = 4), size 2000 ms, `buffer_length` 1 s: the floor is `margin + 3·len = 6 s` > span. This is physics, not a bug. A forward grain at rate 4 consumes 8 s of source in 2 s of output, so it *must* start ≥ 6 s back. The ring holds it (41 s), so the audio is valid. **Decision for the plan (recommended):** the reach floor wins over span. Span bounds the *distribution* of positions. The floor bounds *validity*.
- **Gate definition** (FUNC-01 criterion 2). For every grain spawned after a `buffer_length` change at sample c:
  1. `D_spawn ≤ max(margin + span_new, floor(r, len))`
  2. `D_end ≤ kRingSamples` at every read (debug-assert counter == 0)
  3. a click probe across the change (tonal source, spray 0, jitter 0; a step verdict against a same-motion reference with no `buffer_length` change; memory: step click gate needs a same-motion control)

  "Stale" means only (a) wrapped past capacity, or (b) the newest `margin` samples that are not yet settled. Neither can happen.
- **Before the ring fills** (first `buffer_length` seconds after prepare), large positions read the zeros left by `clear()`, which is silence. Recommended: **do not clamp the live span to `writeCount`**, so the position knob keeps its meaning. Silence at startup is honest, and it is block-size invariant either way. (The freeze `heldSpan` latch in 2.4 *does* clamp to captured. That is a different case.)

## 3. R3 — `grain_limit` < 128 is not enforceable through the pool API

`GrainPool<Grain,128,16>::obtain(progress)` steals only when `findFreeLive()` returns −1, which means all **128** live slots are busy. With `grain_limit = 32`, the 33rd concurrent grain simply gets a free slot. The pool has no "steal a specific slot into a fade slot" call (`moveToFadeSlot` is private), and `release()` is an instant cut (a click).

**Plugin-side cap (D1-compatible; grain-engine unmodified):**

```
spawn(onset):
    draws = drawAll(seed, stream, counter)        // draw BEFORE obtain (always)
    nAlive = count of live-slot grains with killStep == 0   (per-sample exact pool state)
    if nAlive >= grainLimit:
        if a free live slot exists:
            victim = argmax over live, non-killing grains of progress(g)   // age·invLen
            victim.killStep = 1 / (5 ms · fs)      // D1 local fade, × fade.next()
            g = pool.tryObtain()                   // guaranteed non-null here
        else:
            g = &pool.obtain(progress')            // native fade-on-steal; it performs the steal itself, so no separate kill
    else:
        g = pool.tryObtain() ?: &pool.obtain(progress')   // kills may fill every slot
    progress'(g) = (g.killStep > 0 ? 3 : 0) + progress(g)  // prefer a grain that is already dying
```

- At `grain_limit = 128` this reduces to the pool's native behaviour. A steal-free render stays bit-identical because `Fader` is exactly 1.0f.
- The `nAlive` count reads pool state, which advances per sample identically at every block size. That makes it a deterministic, occupancy-based *decision*, which is allowed. Occupancy never enters a *gain law* (§4).
- **Steady state is the same as a native steal:** with constant `len` the victim is the oldest, so each grain dies at phase `q/N`. The N_eff law in §4 applies unchanged.
- 2.2's stream-fair `progress` (over-quota +2, same stream +1) slots into the same `progress` term. It needs no restructure.
- Add to the alloc and steal gates: `grain_limit` 32 with N ≫ 32 must give `max nAlive ≤ 32` (counted per sample) and a non-zero kill count. That second count is the liveness check, so the gate can't pass because nothing was ever killed.

## 4. Q3 — Output law for one stream at the cap (pinned)

Definitions:
- `N = density · len̄ / fs`, where `len̄` is the size after the [5, 2000] ms clamp. `size_rand` is symmetric (`(2u−1)·0.5`), so its mean is exactly `size` except where the clamp truncates. For 2.1 use `size` clamped, because the law is parameter-derived.
- `q = grain_limit` (with one stream, the quota is the cap). `x = q / N`.
- `M(x) = (1/x)·∫₀ˣ w²(φ) dφ`, the mean square over the first fraction `x` of the window. Steal-oldest kills each grain at phase `x` (memory: density norm over a cap truncates the window).

**The law:**

```
P      = (N <= q) ? N · μ₂            // uncapped
                  : q · M(q/N)         // capped: q grains alive, each truncated at phase q/N
g_out  = 1 / sqrt( max(1, P / μ₂_Hann) )
```

It is continuous at N = q, because M(1) = μ₂.

**Hann closed form** (2.1 is Hann-only, so no table is needed). For w = ½(1 − cos 2πφ), μ₂_Hann = 3/8 and

```
∫₀ˣ w² dφ = 0.375·x − sin(2πx)/(4π) + sin(4πx)/(32π)
```

Compute it in double at the control grid from latched parameters only. Check: x = 1 gives 0.375 ✓. At the worst case (200 g/s × 2 s = 400, q = 128 → x = 0.32), M is ≈ 0.18, so P/μ₂ ≈ 61 rather than 128: **−3.2 dB** versus a naive `min(N, q)`.

- **ARCHITECTURE §10 double-counts μ₂.** It defines `N_eff = q·M/μ₂_Hann` and then applies `1/√(N_eff·μ₂(m)/μ₂_Hann)`, which divides by μ₂_Hann twice and multiplies by μ₂(m) on top of M (already m's own moment). In 2.1 this is harmless because the Hann ratio is 1 either way. **For 2.2/2.3, use the single-normalisation form above**, with `M_m(x)` from the WindowMorphBank's cumulative-w² table (the "half-window moments" in ROADMAP 2.2). Recommend a one-line erratum note in the 2.1 SUMMARY so 2.2 doesn't inherit the formula.
- Force the Hann ratio to exactly 1.0f (memory), so a Hann render below the cap has g_out = 1/√N and nothing else.
- The 5 ms fade tail of stolen grains is ignored (< 0.1 dB at these overlaps). The 2.2 ±1.5 dB flatness gate measures it.
- **The incoherent law alone overshoots at spray 0 + jitter 0 + pitch 0 (forward, r = 1).** Those grains add in amplitude (memory: forward grain reads are coherent): ≈ +7 dB at overlap 8. The κ coherence blend is scheduled for **2.2**, so 2.1 knowingly ships that loudness at those settings. The 2.1 D3 listening checkpoint should judge level only at the defaults (spray 10 %, jitter 20 %), and the plan should say so.

## 5. R1 — Bitwise block-size null: quantise the read fraction

`MipRing::locateStart` turns the caller's delay back into an absolute position with `t = W − floor(delay)`, `phi = 1 − frac(delay)`, where W is the ring head **at the time of the call**. The per-pass order writes the whole piece `[a, b)` before rendering it, so W = b. **b depends on the host block**: a 441-sample host block cuts pieces at host boundaries, and a 32 host block never does. `delay = (double) W − readAbs` rounds differently when W changes exponent relative to `readAbs`, so `phi` can differ by 1 ulp.

**Measured** (`t.cpp`, 50 M random grain reads, grid-aligned W vs host-cut W): **70 (t, phi) mismatches, 32 of which change the float kernel weights.** At 128 grains × 48 k reads/s that is several bit-diffs per second, so the PERF-03 seeded null **would fail**, and intermittently, which is the worst way to fail.

**Fix (pinned):** split `readAbs` and quantise only the fraction, at read time, from the double accumulator:

```cpp
const double ri = std::floor (g.readAbs);                                   // exact
const double fq = std::round ((g.readAbs - ri) * 0x1p28) * 0x1p-28;         // 2^-28-sample grid
const double delay = static_cast<double> (W - static_cast<std::int64_t> (ri)) - fq;   // exact for W - ri < 2^24
ring.readStereo (g.level, delay, g.set, l, r);
```

- `W − ri` is an exact integer below 2²⁴ (41 s @ 192 k = 7.9 M), and `fq` has ≤ 28 fractional bits, so `delay` is exact. `locateStart` then recovers the same `(t, phi)` for every W. **Measured: 0 mismatches in 50 M reads at 48 k and at 192 k** (`q.cpp`).
- Error ≤ 1.9e-9 samples, which is inaudible and far below the long-buffer DSP-01 gate's resolution. The accumulator stays full double, so there is no drift (the double reference at 25 s still applies).
- `fq` may round to 1.0. `K − 1.0` is still exact. ✓
- Alternative rejected: deferring the ring write to grid points only. It adds up to 31 samples of latency, against "reported latency 0".

## 6. R2 — FUNC-07 bitwise dry null: snap 0 dB, bypass the wet sum

**Measured** (`r.cpp`): `NormalisableRange<float>(-24, 12)` with the 0 dB default has normalised value 0.666666687. Converting back gives **7.15e-7 dB**, and `pow(10, v/20)` gives **1.00000012f** ≠ 1.0f. Any host or pluginval write of the normalised default (and the Stage 1 harness B2 rescope, which sets params through `setValueNotifyingHost`) breaks the bitwise mix-0 null. `mix` is fine: `convertFrom0to1(0) == 0.0f` exactly.

**Pinned:**
- For `output_gain`, `input_gain` and `s1_level`, snap \|dB\| < 1e-3 to exactly 0, then `Decibels::decibelsToGain` (carry-in: never derive unity from a skewed `get()`). For `s1_level` and `input_gain` this is hygiene, because they are not on the null path.
- **The wet path must not touch the output at mix == 0.** `dry·1.0f + wet·0.0f` is not bitwise. `−0.0f + 0.0f = +0.0f` flips signed zeros, and a NaN/Inf wet sample poisons it. When the mix ramp sits at exactly 0 **and** the output-gain ramp sits at exactly 1.0f, copy dry → out and skip the add. Build ramps so that arrival assigns `current = target` exactly (custom ramps, not `SmoothedValue`; carry-in).
- The equal-power law has snapped ends (`m == 0 → (1, 0)`, `m == 1 → (0, 1)`). In between, `dry = cos(½πm)`, `wet = sin(½πm)`, computed at the grid and lerped within the cell.
- Mono→mono: take the dry for the mix-0 bypass straight from the input channel, not from `(L+R)/2` of the engine's duplicated L=R. `(x + x)/2 == x` happens to be exact in float, but the direct copy holds by construction.

## 7. R4 — Onset counter must survive a re-enable

`GrainScheduler::reset()` sets `nextIndex = 0`. ARCHITECTURE §4 says "Enabling resets the clock so the first onset fires on the next sample", and CounterRng draws are keyed on `onset.index`. So every disable→enable **replays the identical grain sequence** (same positions, pitches and reverse flags). That is audible on a toggled stream.

**Fix:** a per-stream `uint64_t epoch` that persists across re-enables (zeroed only in `prepareToPlay`). At re-enable: `epoch += scheduler.getOnsetCount(); scheduler.reset();`. Key every draw (scheduler jitter and grain) on `epoch + onset.index`. It is still a pure function of absolute stream history, so it is block-size invariant. Harness row: enable→disable→enable gives a second-burst grain set ≠ the first (positions differ), and the block-size null across the toggle is bit-identical.

---

## 8. Component pins (API facts the plan can rely on)

**Scheduler — telescoping jitter, simplified.** With one nominal interval I evaluated in the callback for both d_k and d_{k+1}:

```
interval(k) = I · (1 + J · (u_{k+1} − u_k)),    u_k = Draw(seed, kSchedStream + s, epoch + k).nextFloat(kSlotJitter)
```

- Range is I·(1 ± J). The mean is exact and the drift is bounded (it telescopes). Define u_0 := ½, so onset 0 is on the grid. It fires immediately, and it cannot be displaced backwards.
- At J = 1 the gap can approach 0. `GrainScheduler` clamps it to `kMinInterval = 1`, which is acceptable (at most one onset per sample).
- J = 0 must draw nothing (skip the call). `GrainScheduler::jittered` is **not** used (ARCHITECTURE §4).
- The callback reads I from the control-grid value at the onset sample, which is deterministic.

**Onset fraction.** `Onset::frac ∈ [0, 1)` is "samples elapsed since the exact onset". Apply it to both the window and the read:

```
phase   = (age + frac) · invLen
readAbs = anchor + dir · rate · (age + frac)
```

Initialise `readAbs` at spawn and then `+= dir·rate` per sample.

**Pool.** `GrainPool<Grain,128,16>`. `prepare(fs, 5.0)`. `activate()` value-initialises the slot, so declare the Grain POD at its final layout (CONTEXT default). `render(fn)` visits live and fade slots, and fn returns false to retire. For the D1 kill, multiply `killGain` (`killGain = max(0, killGain − killStep)`) by `fade.next()`, and return false when killGain hits 0. `liveWeight()` and `countActive()` are for telemetry only.

**Reader.** `latchLevel(rate, 2)` and `SincBank::setFor(rate / 2^level)` are latched per grain. ±24 st means r ≤ 4, so level ≤ 2 and r′ ∈ [1, 2], giving sets S0–S6. S7 is never reached. `readStereo` is 2 × 16 MAC. Exactly +24 st gives r = 4.0 → level 2, r′ = 1 → S0.

**CounterRng.** `Draw(seed, stream, counter).nextFloat(slot)` gives 24-bit uniforms. Fixed slot enum per quantity: position-spray, size, pitch, reverse, and reserve FX slots now. Gate a probability at the ends (`reverseProb ∈ (0,1)` before drawing). A randomisation at 0 draws nothing.

**WindowLut** depends on JUCE (`juce::jlimit`), which is fine inside the plugin. 2.1 does not need it (closed-form Hann), but 2.2's bank does.

## 9. Gate recipes (reuse, don't reinvent)

| Gate | Recipe / source |
|---|---|
| PERF-03 block-size null | Host blocks 32 / 441 / 512 / 4096 / variable. Every randomisation ON. Seeded via `setSeed`. Position-deterministic stimulus (a hash of t, not a sequential RNG; memory: grain read before capture write). `max\|Δ\| == 0`. Plus a different-seed control that must differ. Include a param change at a shared boundary and an s1 enable toggle (R4). R1 is the fix this gate exists to catch. |
| Oversized block | `prepareToPlay(fs, 512)`, then `processBlock(8192)`. Every per-pass array is a fixed 32, so nothing scales with the block. Assert no overrun under ASan if available, otherwise use the poison guard (memory: Contrabass oversampler). Host block ≥ min grain delay: 4096 at position 0. |
| PERF-01 alloc | O-Bells `--alloc-check` block: `malloc_logger`, **`volatile`** flag and counter, a liveness malloc that must count 1, scoped to the audio `pthread` (memory: malloc_logger two traps). Cover spawn, steal (`grain_limit` 32, N ≫ 32), the R3 kill path and enable toggles. |
| DSP-02 alias | Port `O-GrainScatter/tests/alias-gate` row 1e (processor end-to-end, +24 st via the param, out-of-band tones vs a 1 kHz-out tone, 44.1 kHz). Negative control: the same run with `latchLevel` forced to 0 must FAIL (non-vacuous). |
| DSP-01 long-buffer | O-TextureForge `longcorpus.cpp` pattern: an exactly periodic corpus (`i % P`), one grain near the start and one a multiple of P later (null), **plus** a double reference `start + n·rate` at 25 s (catches accumulator drift; memory: float grain read position degrades early). |
| FUNC-07 | Rescoped B1–B3 (CONTEXT constraint): mix 0 % bitwise across 3 layouts × 6 block patterns, **with params set through the host normalised path** (exercises R2). The defaults row becomes "wet present" (RMS(out − dry) > −40 dBFS). Mix 100 %: correlation with dry ≈ 0 at position ≥ 1 s. |
| FUNC-01 | Audible: first onset at position 0, output energy within `interval + len` samples. `buffer_length`: the §2 three-part gate. |
| CPU baseline (non-blocking) | 128 grains, no FX, 48 k/128, as a ratio vs engine-bypass. The datum to beat: 128 reads ≈ 6.5 % of a core in isolation (`b.cpp`). |

## 10. Pitfalls checked against the knowledge base

- Write-before-read on an absolute 32 grid avoids the "read before capture write" class at any host block, and the R1 fix closes the residual float-rounding leak.
- Draw before obtain, and probabilities gated at the ends.
- Laws come from latched params at the grid, never from `liveWeight()` (block-size leak).
- Custom ramps instead of `SmoothedValue::reset()` (NaN copy). `ScopedNoDenormals` is already in the Stage 1 `processBlock`.
- Channel counts come from the bus (`monoToStereo` is already decided from the layout in Stage 1).
- No APVTS writes from the audio thread or `parameterChanged`.
- The seed is a per-instance clock seed. The harness injects it via `setSeed()`, and null pairs are seeded.

---

## Next

Ready for `/plugin-plan O-Granulator 2-dsp` (Phase 2.1). The plan must include R1–R4 as explicit tasks or acceptance rows, the rescoped B1–B3 note, and the §4 erratum for 2.2.
