# Stage 3 — GUI · Phase 3.3 (Mod matrix view + mod rings) — Research

**Plugin:** O-Granulator
**Stage / phase:** 3-gui / 3.3 of 3
**GSD phase:** research
**Date:** 2026-11-06
**Tree:** `main` @ `4f50e26f` (3.3 discuss). Nothing was edited, built or installed.
**Inputs:**
- `CONTEXT-3.3.md` (D34–D41 locked; six open questions)
- Source: `PluginProcessor.h/.cpp` (`Telemetry`, `readTelemetry`, `processBlock`), `ScaleStateJson.h` (`telemetryToVar`), `dsp/Engine.h/.cpp` (`process`, `gridTick` feedback block 720–745, `tickSources`, `publishUi`), `dsp/ModMatrix.h` (`tick`, residuals, depth ramps), `dsp/FeedbackChain.h` (`stepAgc`, `env`), `dsp/ModSources.h` (`Lfo::tick`, `position`, `getPhaseDeg`), `dsp/Analysis.h` (`processPiece` event path, `Stats`), `dsp/UiTelemetry.h`
- Page: `Source/ui/public/index.html`: `pollTelemetry` / `ingestTelemetry` (2810–2845), `drawMeter` (3103), `lfoShapeAt` / `drawLfos` (3119–3155), onset lane (3175–3180), pills (3192–3196), `updateReadouts` loop gain (3220–3229), `routesFor` / `renderModRing` / `bindAltDepthDrag` (1871–1955), `clearSlot` / `addRoute` (2173–2240)
- Gates: `tests/render-harness/engine_gates_26b.h` (MX3, MF8), `engine_gates_32.h` (TL32, M8), `main.cpp` `dumpUiOverrides`, `tests/i18n-states.json`
- 2.x research: RESEARCH-2.5a R46 / R49 / R57, VERIFICATION-2.6 (QUAL-01 complete)
- Memory: generation reaper freezes when the host idles the audio thread, WebView completion gated on isVisible, step click gate needs same-motion control, UI fixture state producible by C++, gate inherits the review finding's scope

There are **ten findings** (R127–R136). All six open questions are answered (§9).

Three of them change what the plan builds:
- **R127:** the D36 click fix is very likely not needed. 2.6 already gates every route-edit kind (MX3a–g on 11 sources × 112 destinations, MF8a–f in audio, with live negative controls). 3.3 adds rows that replay **the page's actual write sequences**, not a DSP change.
- **R128:** `inPeak` cannot be measured in `processBlock`. The buffers are in-place, so after `engine.process` the "input" is the output. It also cannot come from the 3.2 peak ring, which includes the feedback return.
- **R131:** D40's sample-count clock for the onset pulse **sticks the pills on** when the host idles the audio thread. Use an event counter and a **reader-side wall clock** instead.

Two refine a locked decision without reversing it:
- **R133 (D34):** use the *effective* `fbKnobNow`, not the knob ramp. A matrix route on Feedback makes the loop live while the knob reads 0.
- **R131 (D40):** a 100 ms decay keeps the pills above 0.5 for only 50 ms (1–2 pulls). Use 200 ms linear decay to get the "about 3 frames" D40 asks for.

---

## Summary for the planner

| # | Finding | Severity | Plan action |
|---|---|---|---|
| **R127** | **Route edits are already click-gated.** `ModMatrix::tick` ramps every depth over 20 ms and moves a removed or switched route's last contribution into a per-destination residual that ramps linearly to 0 (R49). 2.6 gated this as QUAL-01: MX3a–g (add, source → None, dest → None, depth → 0, depth step, dest swap, source swap) at worst 1.000× of the analytic bound, MF8a–f audio κ at worst 1.444× vs 2.0, NCs MX3nR / MX3nD / MF8n live. Every page edit maps onto those kinds (§1) | **D36 likely needs no DSP fix** | Add **UX1**: replay the page's exact write orders (`clearSlot`, `addRoute`, select changes, depth drag), with the writes landing in the same cell, 1 cell apart and 7 cells apart. Use the MX3 trajectory bound plus MF8 κ vs a held reference. NCs: `noResidual`, `noDepthRamp`. Touch DSP only if UX1 fails |
| **R128** | **`inPeak` must be measured inside the engine's piece loop, before `mixOut`.** `in` and `out` alias (JUCE in-place), so measuring in `processBlock` after `engine.process` reads the output. The 3.2 `PeakRing` is not usable either: the ring holds input·g **+ the feedback return** | **Wrong meter if missed** | Per piece, next to `env.process`: `pk = max(pk, max(|inL|, |inR|) · inGain.value(c))`. Out peak: in `processBlock` after `engine.process`, over channels `[0, numOut)`. Mono out → R = L |
| R129 | `std::atomic<float>` has no `fetch_max` before C++26. A plain load → max → store races with the reader's `exchange(0)` and can bring back a peak that was already consumed | Design | CAS loop once per **block** (not per sample) on a local block max. The only competing writer is the reader's exchange, so retries are bounded. `static_assert(std::atomic<float>::is_always_lock_free)` |
| **R130** | **Max-since-read goes stale when no one reads.** While the editor is closed, or the WebView is hidden and completions are dropped, peaks and the onset flag keep accumulating. The first read after a reopen shows the loudest peak in minutes and a phantom onset tick | **Visible on every reopen** | Reader-side gap rule: if `now − lastReadMs > 250 ms`, consume everything and report 0 / false for that frame. Reader state lives in the processor, `mutable`, under `telemetryLock` |
| **R131** | **Onset: event counter + reader wall clock, not a sample clock.** A pulse computed from `absCount − eventSample` stops decaying when the host stops calling `processBlock`, and Logic idles the audio thread when stopped. The pills then stay lit. Offline bounces also run faster than real time | **Stuck-lit pills if D40 ships as written** | Audio: `onsetCount` (single writer, relaxed increment when `eventOffset >= 0`, at most one per piece). Reader: count changed → `onsetFired = true`, `pulseStartMs = now`. `onsetPulse = max(0, 1 − (now − pulseStartMs)/200)`. That stays above 0.5 for 100 ms ≈ 3 pulls, so no hold is needed |
| R132 | **LFO phase needs no `Lfo` change.** After each `lfos[n].tick`: `p = position() − getPhaseDeg()/360`, `phase = p − floor(p)`. That is the pre-advance accumulator (or the Direct) fraction without the offset (D39). The page geometry matches exactly for shapes 0–3 (§4) | Design | `lfoPhaseTel[4]` relaxed atomics in `tickSources`. Clamp a float rounding to 1.0f → 0 |
| **R133** | **Loop gain: use `fbKnobNow`, not `fbKnobRamp`.** `gridTick` sets `fbKnobNow` on both branches (routed and unrouted, Engine.cpp:731–737). A route LFO → Feedback with the knob at 0 makes the loop live, and a ramp-based readout would say "Open". The AGC `env` is a plain float written per sample, so reading it from the UI is a data race | **D34 refinement** | Audio: store `k = fbKnobNow` per tick, `gAgc = min(1, kAgcCeil/env)` (env 0 → 1) and `frozen` at block end, all relaxed. Reader: absent iff `k == 0` or `frozen`, else `20·log10(k·gAgc)`. The bound ≤ −0.4455 dB holds by construction |
| R134 | `readTelemetry` is `const` and reads the engine through a const reference. Consuming reads (exchange) and reader state need `mutable`. Harness reads are destructive, and the onset pulse needs a clock | Design | Engine `consume*()` getters on `mutable` atomics. Add `readTelemetry (Telemetry&, double nowMs)`; the existing 1-arg form passes `Time::getMillisecondCounterHiRes()`. The harness injects time |
| R135 | **TL32 asserts that the 3.3 fields are zero / NaN** (`engine_gates_32.h:777–785`). It fails the moment 3.3 lands. This is a planned row change, as R126 was | Gate bookkeeping | Rewrite as **TL33**. Record it in the carried-diff attribution |
| R136 | **i18n state:** with s2–s4 enabled, `#count-s2…s4` switch from "Off" to `label.sounding {n}` (128 from the overrides), which is the widest text. `generic-overrides.json` needs no shape change: `dumpUiOverrides` already forces every 3.3 `has*` on with the widest values | Deviation 10 closes | Add `streams-all-enabled` to `tests/i18n-states.json` (§7). Regenerate the overrides as a check and expect a byte-identical file |

---

## 1. Route-edit clicks (R127, D36)

### What is already there

`ModMatrix::tick` (per 32-sample cell):
- **Depth edit:** `s.depth.setTarget(want.depth, rampCells)`, an ExactRamp over 20 ms. A continuous fader drag is a series of retargets from the current value, so it stays value-continuous.
- **Source or destination change:** the slot's last contribution is added to `residual[oldDest]`, which ramps linearly to exactly 0 over 20 ms. The slot restarts at depth 0 and ramps to its target. That holds for any change rate, with fixed memory (R49).
- The routed/unrouted switch (R46 round trip) has a non-identity of at most ~6.5e-7 relative. It is already inside the MX3 / MF8 measurements.

VERIFICATION-2.6 row 3: *"MX3a–g 77/77 each, worst 1.000× of the analytic bound, cross-talk 0. MX3nR / MX3nD > 2× for every source. MF8a–f 20/20 each, worst gated 1.444× … MF8n 2.09×."* QUAL-01 is complete.

### Page write sequences mapped onto the MX3 kinds

| Page action | Writes, in order | Equivalent MX3 kind | Note |
|---|---|---|---|
| Click slot number (`clearSlot`) | source → 0, dest → 0, depth → 0 (drag gesture) | **b** source → None | dest / depth on a dead slot are inert (`wasLive` false, `lastContrib` 0). Latched in the same tick as the source write → still one change, and the residual goes to the **old** dest |
| Drag chip onto knob (`addRoute`) | dest → d, depth → 0.5 default, **source last** | **a** add None → src | The slot is dead until the source lands, and the depth enters from 0 |
| Source select | source → S′ or 0 | **g** / **b** | — |
| Dest select | dest → d′ or 0 | **f** / **c** | — |
| Depth fader / Alt-drag on knob | depth retargets at UI rate | **e** (repeated) | The ramp retargets from its current value |
| Preset load / host automation | up to 16 slots at once | MX3 edits 16 slots in one cell | — |

**Conclusion:** no edit path falls outside the 2.6 gates. What MX3 does **not** cover is a multi-write sequence that **spans ticks**. Example: `clearSlot`'s source and dest writes land in different cells, so the evaluation sees two changes. By reading, that is harmless: the second change finds `wasLive = false`. UX1 proves it instead of assuming it.

### UX1 gate design (P-level)

- **Base:** reuse `mf8Job` (sine 440, 4 streams, mix 50 %, LFO1 2 Hz at its peak, and Onset). Destinations: `s1_level`, `mix`, `output_gain`, `feedback`. These are the per-sample ramped paths where a step is audible. Spawn-latched destinations only affect new grains, and MX3 covers their trajectory.
- **Sequences:** `clear` (3 writes), `drop` (3 writes, source last), `dest swap`, `source swap`, and `depth drag` (10 retargets ±60 over 300 ms). Spread each multi-write sequence across 0, 1 and 7 cells.
- **Verdicts:** (a) the trajectory, using `mx3Check` with the same 1.05× analytic bound; (b) audio κ ≤ 2.0× the held-route reference. That is the same-motion control (pattern_step_click_gate_needs_same_motion_control), on a tonal source.
- **NCs:** `noResidual` on `clear` and `noDepthRamp` on `drop`, each > 2× on at least one destination.
- **If UX1 passes:** D36 closes with **zero audio change**. The carried diff = new rows only.
- **If it fails:** the fix lives in `ModMatrix::tick` only and fires only on change cells, so no-edit renders stay byte-identical by construction.

---

## 2. Meters (R128, R129, R130; D38)

### Where to measure

`processBlock` passes `in` and `out` built from the **same** `AudioBuffer` (`PluginProcessor.cpp:288–289`). The engine writes `outL + done` in `mixOut` at the end of each piece, after reading `inL + done`. Consequences:
- **inPeak:** measure inside `Engine::process`, per piece, before `mixOut`. The gain is `inGain.value(c)`, the routed/unrouted selector that `CaptureWriter` and `env.process` already use. That matches D38's "after input gain". The ring (3.2 `PeakRing`) is **not** the input: `CaptureWriter::write` adds `fbDelay`, so with feedback the ring peak rises above the input. Order NaN-safely: `pk = std::max(pk, v)` with `v` as the second argument drops a NaN, and the reader clamps non-finite values to 0.
- **out:** measure in `processBlock` after `engine.process`, `FloatVectorOperations::findMinAndMax` over `[0, n)` per channel (no allocation). If `numOut == 1`, report R = L. That covers output gain, the wet safety and anything mixed after the engine. Bypass (JUCE's default `processBlockBypassed`) doesn't call `processBlock`, so the meters read 0 there. That's correct.

### Transport (R129)

```
// audio thread, once per block
void pushMax (std::atomic<float>& a, float pk) noexcept {
    float cur = a.load (std::memory_order_relaxed);
    while (pk > cur && ! a.compare_exchange_weak (cur, pk, std::memory_order_relaxed)) {}
}
// reader
float v = a.exchange (0.0f, std::memory_order_relaxed);
```
Three atomics: `outPkL`, `outPkR`, `inPk`. A plain `load / max / store` would let a reader exchange that lands between the load and the store resurrect the consumed peak. That is only a cosmetic stale frame, but the CAS loop costs the same and has no race.

### Staleness (R130)

The editor polls at 33 ms. When nothing polls (editor closed, Logic window hidden: critical_webview_completion_gated_on_isvisible), the maxima accumulate. Rule in the reader:

```
const bool stale = now − lastReadMs > 250.0;   // also true on the first read
lastReadMs = now;
consume every accumulator (exchange 0, count snapshot);
if (stale) report 0 / onsetFired false / pulse 0 for this frame;
```
The page's own decay (0.88 / 0.94) makes a one-frame 0 invisible.

---

## 3. Onset (R131; D40)

**Where the event is:** `Analysis::processPiece` returns `eventOffset >= 0` (at most one per piece) into `Engine::process` (line ~479). Analysis always runs in release (`analysisOn()` is `true` outside test hooks), frozen included. `Analysis::Stats::onsetEvents` exists but is a plain `int64`, so don't read it across threads.

**Why not a sample clock:** a "time since the last event" clock in samples needs a "now" in samples. That "now" only advances while `processBlock` runs. Logic idles the audio thread for a stopped, unarmed track (pattern_generation_reaper_freezes_when_host_idles_audio_thread), so the pulse would freeze at its last value. If that value was > 0.5, the trigger pills stay lit until playback resumes. An offline bounce runs faster than real time and compresses the decay.

**Design:**
- Audio: `onsetCount.store(onsetCount.load(relaxed) + 1, relaxed)` when `eventOffset >= 0`. There is a single writer, so no RMW is needed.
- Reader (under `telemetryLock`): `c = onsetCount.load()`. If `c != lastOnsetCount`: `onsetFired = true`, `pulseStartMs = now`, `lastOnsetCount = c`. `onsetPulse = clamp(1 − (now − pulseStartMs)/200, 0, 1)`. `hasOnset = true` once prepared.
- Several events between two pulls coalesce into one tick. At the lane's scale (4 s across ~300 px) 33 ms is under 3 px, so nothing visible is lost.

**Pill threshold (Q6):** `pulse > 0.5` holds for 100 ms → 3 pulls at 33 ms. D40's "~100 ms decay" taken literally (1 → 0 over 100 ms) gives 50 ms (1–2 pulls, sometimes one). No separate hold is needed with the 200 ms ramp.

---

## 4. LFO phase (R132; D39)

`Lfo::tick` computes `ph = frac + φ_ramped/360`, `x = ph − floor(ph)`, `posOut = cycle + floor(ph) + x`, **then** advances `frac` for the next cell. The offset-free phase of the current cell is therefore:

```
const double p = lfos[n].position() - lfos[n].getPhaseDeg() / 360.0;   // cycle + frac_pre (+ ulp)
float ph = (float) (p - std::floor (p));  if (! (ph < 1.0f)) ph = 0.0f;
lfoPhaseTel[n].store (ph, relaxed);                                       // in tickSources, after the tick
```
There is no change to `ModSources.h`, so N-suites stay bitwise by construction.

**Geometry check (page vs engine):**
- The curve at x-fraction `q` is `lfoShapeAt(shape, (2q + ph0) % 1)`. The dot sits at `q = phase/2`, so the curve there is `shape(phase + ph0)`, which is the engine's output. Only during the 20 ms `lfoN_phase` ramp do the page's `ph0` (the target) and the engine's ramped φ differ.
- Shapes 0–3 are the same formulas: sine; triangle `1 − 4|x − ½|` ≡ the page's piecewise; saw `2x − 1`; square `x < ½`. Shapes 4 (S&H) and 5 (smooth random) are stylised on the page, so the dot can leave the curve (D39 accepted, hands-on note).
- The dot always sits in the first cycle of the two-cycle figure (`phase % 1 / 2`). That is by design.

**Tempo / onset (Q3):** Tempo uses the Direct form `frac((ppq − barOrigin)/divBeats)`, so the dot is bar-locked and **jumps** at a re-lock (transport start, loop, locate). That is a hands-on item. Onset triggers act on streams only: nothing re-phases an LFO, so there is no onset effect on the dot.

---

## 5. Loop gain (R133; D34)

`Engine::gridTick` (Engine.cpp:720–745):

```
fbKnobRamp → 0.95·feedback over 20 ms
k = routed(Feedback) ? fbKnobFromRaw (apply (…, offset)) : fbKnobRamp.current();
fbKnobNow = k;                       // ← the chain's effective g_knob at this tick
```
- **Use `fbKnobNow`.** With a route on Feedback, the ramp is the unmodulated base. At knob 0 with LFO → Feedback, the loop is live, but a ramp-based readout says "Open", which contradicts the constraint "Open exactly when the loop contributes nothing".
- **"Open" iff `k == 0` (exact) or `frozen`.** ExactRamp settles to exactly 0. `fbKnobFromRaw` clamps the raw value to [0, 100], so a negative offset at 0 stays 0. While frozen the chain still runs ("Runs every pass, frozen included"), but `CaptureWriter` pushes nothing, so the return is discarded. Use `frozen`, the stopped head, not `freezePending`: during the pending fade the ring is still written.
- **AGC:** `FeedbackChain::env` is one stereo-linked peak follower (Q2: a single value), a plain `float` written per sample. Publish `gAgc = env > 0 ? min(1, kAgcCeil / env) : 1` at the end of `process()`, as a relaxed atomic. Store `k` per tick and `frozen` per block.
- **Reader:** `hasLoopGain = !(k == 0 || frozen)`, `loopGainDb = 20·log10(k · gAgc)`. There is no `log10` on the audio thread.
- **Bound:** k ≤ 0.95f and gAgc ≤ 1, so the value is ≤ 20·log10(0.95f) = −0.4455 dB. The page prints "−0.4 dB" and the bar reads 100 %.
- **What the number means:** it is the gain the loop applies (knob × AGC). It does not include tanh compression or filter loss, so at heavy saturation the real return is lower than the readout. That is consistent with D34's definition. Add a hands-on note.

---

## 6. Threading and API shape (R134)

- New engine members: `std::atomic<float> outPkL, outPkR, inPk, loopK, loopAgc`; `std::atomic<bool> frozenTel`; `std::atomic<uint32_t> onsetCount`; `std::array<std::atomic<float>, 4> lfoPhaseTel`. The ones the reader exchanges are `mutable`, behind `consume*()` const getters. Reset all of them in `prepare` (message thread), as `srcTelemetry` is (Engine.cpp:110).
- Reader state in the processor: `mutable double lastReadMs, pulseStartMs; mutable uint32_t lastOnsetCount;` guarded by the existing `telemetryLock`. The audio thread never takes it.
- `bool readTelemetry (Telemetry&, double nowMs) const`, plus the existing signature forwarding `Time::getMillisecondCounterHiRes()`. The harness drives `nowMs`.
- **Destructive reads in the harness:** a TL row that calls `readTelemetry` twice gets 0 the second time. Design the rows for this, and treat it as a gate (MT3). `dumpUiOverrides` overwrites every 3.3 value after its read, so it stays deterministic.
- Audio-thread cost: one `abs·mul·max` per input sample, one `findMinAndMax` per output channel per block, a few relaxed stores. Negligible against C7.

---

## 7. Page, i18n and stubs (R136)

- The page needs **no edit**. All consumers exist (`ingestTelemetry`, `drawMeter`, `drawLfos`, the lane, the pills, `updateReadouts`), and the JSON keys and types match: `out[2]`, `inPeak`, `lfoPhase[4]`, `onset`, `onsetFired`, `loopGainDb` (finite only).
- **New i18n state** (`tests/i18n-states.json`):
  ```json
  { "name": "streams-all-enabled",
    "eval": "document.getElementById('tab-streams').click(); ['s2','s3','s4'].forEach((s) => document.getElementById('toggle-' + s + '_enable').click());" }
  ```
  With the overrides' counts at 128, `#count-s2…s4` show `label.sounding {n: "128"}`, the widest. Run check-ui-labels in every language.
- `generic-overrides.json`: no shape change. Regenerate with `--dump-ui-overrides` and expect a byte-identical file. A diff means the telemetry shape moved, so regenerate the binding-probe stub too.

---

## 8. Gate table (proposal)

| Row | Checks | NC |
|---|---|---|
| **MT1** | Out peak: −6 dBFS sine, mix 0 → `out` = 0.5012 ± 1e-5 both channels. +6 dB output gain → ×1.995. Mono out → R = L | — |
| **MT2** | inPeak: −12 dBFS sine, input_gain +6 → 0.5012 ± 1e-5. **Feedback 80 % does not raise it** | **MT2n:** the ring-peak measure rises with feedback |
| **MT3** | Max since read: a 1-sample 0.9 spike in block 3 of 10 → the read returns 0.9. An immediate second read → 0 | — |
| **MT4** | In-place: silent input with a loud wet tail (captured earlier, frozen) → inPeak 0 | **MT4n:** measuring after `engine.process` reads ≠ 0 |
| **MT5** | Staleness: a read gap > 250 ms → 0 / `onsetFired` false. Next read normal | — |
| **ON1** | 4 Hz click train, reads every 33 ms (injected clock): fired once per event-bearing read. Pulse 1 → 0.5 at +100 ms → 0 at +200 ms | — |
| **ON2** | Idle host: no `process` calls, clock advances → pulse reaches 0 | **ON2n:** a sample-domain pulse stays > 0.5 |
| **LP1** | Free 1 Hz: phase = frac(N·32/fs) ± 1e-6 after N cells. With `lfo1_phase` 90°: phase unchanged, `lfo` = sin(2π(phase + 0.25)) ± 1e-6 | — |
| **LP2** | Tempo 1/4 at 120 bpm: phase = frac((ppq − bar)/1) ± 1e-6 | — |
| **LG1** | fb 50 %, quiet input (AGC idle) → −6.465 dB ± 0.01. fb 100 %, hot input → < 20·log10(0.95) and strictly below knob-only | — |
| **LG2** | fb 0 → absent. Freeze on (after the edge) → absent. Unfreeze → present | — |
| **LG3** | Knob 0 + LFO1 → Feedback 50 % → present, finite, ≤ −0.4455 | **LG3n:** the ramp-based k reads absent |
| **UX1** | §1: page write sequences × spread {0, 1, 7 cells} × {s1_level, mix, output_gain, feedback}: trajectory ≤ 1.05× bound, κ ≤ 2.0× held ref | `noResidual` clear, `noDepthRamp` drop > 2× |
| **TL33** | Replaces TL32: 3.1 + 3.2 fields == engine; 3.3 fields live (`hasMeters`, `hasOnset`, `hasLoopGain` per the state, `lfoPhase` ≠ 0 after 1 s at 1 Hz) | — |
| **M8** | Extended: 0 allocations from block 0 with every 3.3 write active (CAS, counter, phase, loop) | M8L liveness |
| Carried | Default harness carried diff vs the 3.2 baseline = TL32 → TL33 + new rows only. N3 / N2 / N1 / N0 `--strict`, R12 rc 0, block-size null | — |
| UI | 5 UI gates + binding probe green, with the new i18n state | — |

---

## 9. Open questions → answers

| Q (CONTEXT) | Answer |
|---|---|
| Click gate design; is the matrix ramped; is a dest switch a jump; smallest fix? | Ramped and residual-faded (R127). A dest switch is not a jump: the old dest's contribution fades through its residual and the new one ramps in from depth 0. 2.6 MX3 / MF8 already cover every edit kind. 3.3 adds UX1 for the page's multi-write orders. Expected fix: none |
| Where are `g_knob` and AGC `env`; readable as relaxed atomics? | `fbKnobNow` (per tick, effective, modulated) and `chain.env` (per sample, plain float). Publish both as relaxed atomics (R133). The AGC is stereo-linked, one value |
| LFO phase with or without offset; Tempo; onset re-phase? | The accumulator is offset-free. φ is added outside it, so subtract it from `position()` (R132). Tempo is bar-locked, with a jump at a re-lock. No onset re-phase of LFOs exists |
| Onset event visibility and timing without a clock dependency? | `eventOffset >= 0` in `Engine::process`. Count on the audio side and time on the reader (R131). A sample clock freezes when the host idles |
| Harness rows | §8 |
| `pulse > 0.5` vs a 30 Hz pull: hold needed? | No. A linear 200 ms decay from the first read that sees the event keeps it > 0.5 for 100 ms ≈ 3 pulls |

---

## 10. Hands-on additions (HANDS-ON-3.3)

- Meter: dry sine at −6 dBFS reads about 6 dB below the top tick region. Input gain moves the level lane, not the out meter, when mix is 0.
- Reopen the editor after a loud passage with the editor closed: no false peak or onset tick (R130).
- Stop transport in Logic with a track not armed: the pills go dark within 200 ms (R131).
- LFO dot on the curve for sine / triangle / saw / square. S&H / random dot off the stylised curve (accepted). Tempo: the dot jumps on transport start / locate.
- Loop gain: fb 50 % reads −6.5 dB. Push fb 100 % with hot input: the value drops below −0.4 dB as the AGC engages. Freeze on → "Open". Knob 0 + LFO → Feedback → a moving value.
- Saturation note: at heavy `fb_saturation` the readout stays near the knob × AGC value while the real return is softer (by design, D34).
- Route edits by ear (D36): clear a slot, drop a chip, swap source / dest, Alt-drag depth, all mid-stream on a sustained tone.
