# O-Granulator — Stage 0 Context (Discuss/Research Findings)

**Date:** 2026-11-04
**Phase:** Stage 0 Research & Planning (research-planning-agent)
**Tier / depth:** 6 / DEEP · **Complexity:** 5.0 (raw 22.0) · **Strategy:** phased (Stage 2 × 6, Stage 3 × 3)

## What was decided and why

### 1. Engine pass = absolute 32-sample grid; feedback returns after 32 samples
Each host block is cut at absolute multiples of 32. Per piece: control tick → analysis → **write input + fb[n−32]** → schedule → render (reads ≤ head − margin, all written) → mixer → feedback chain. Feedback from earlier pieces is all that's needed, so write-before-read holds at any block size. This is bit-identical across block sizes and uses fixed 32-sample arrays, so oversized host blocks can't overrun anything. Implements the suite rule "bound each pass so reads never outrun the capture write."

### 2. Freeze × feedback: freeze wins, head-stop, no overdub in v1.0
- Frozen = no ring writes at all, so contents stay byte-identical (FUNC-02). The feedback return is computed (chain warm) and discarded.
- No overdub: MipRing builds its octave levels causally at the head, so it can't overdub the past. Overdub is a v1.1 candidate via a virtual head.
- Head-stop (Clouds/Beads semantics) diverges from O-GrainScatter/O-Freeze, which use an advancing virtual head. The known traps are handled: micro-spray floor while frozen (anti-buzz), heldSpan latched to what was actually captured (anti-silence), 5 ms forced release for grains that would cross the stopped head, 10 ms content crossfade on release.
- Fallback: the O-GrainScatter virtual-head model. Decide after the QUAL-04 hands-on A/B.

### 3. Loop gain: four independent bounds
1. Coherent feedback-tap law `1/max(1, N_eff·μ₁)`, divided by Σ stream level, then 0.95·feedback. This gives per-generation gain < 1.
2. `tanh(d·x)/d` with d ≥ 1 always, so the guard is present at saturation 0.
3. Loop AGC holds the return peak ≤ 0.7.
4. Wet safety `1.9953·tanh(x/1.9953)` caps the peak at +6 dBFS (DSP-06).

The output uses the incoherent law, blended toward the forward/coherent norm when spray, pitchRand and jitter are all 0. Using one law for both would either duck the output or let the loop self-oscillate (O-ReverseDelay overlap finding).

### 4. Pitch follower = MPM on decimated input, on the audio thread; onset = dual-envelope ratio
- MPM: D = round(fs/12 k), W 1024, FFT order 11, hop 128 on an absolute counter. ≈ 21 MFLOP/s, deterministic. YIN was rejected because O-Bass's YIN overran the audio thread. A background thread was rejected because it is non-deterministic.
- Neither detector touches the dry or wet path. The pitch source lags about 45 ms; the onset detector has zero latency and fires at the exact sample.
- `pitch_follow_sN` = absolute in-key quantisation, which needs a new `scale_root` param. When the pitch is unconfident it falls back to interval quantisation.

### 5. CPU budget and grain cap
- Budget: worst case ≤ 30 % of one core @ 48 k/128 (128 grains, all FX, feedback, reverb); typical ≤ 10 %. Measured as a ratio against an engine-bypass baseline.
- Pool `GrainPool<Grain,128,16>` shared by the streams. Stealing is stream-fair, with quotas derived from parameters, using the existing `obtain(progress)` API.
- Degradation is deterministic only (cap, quotas, proposed `grain_limit`). There is no wall-clock adaptation, because that would break the rule that offline renders match live.

### 6. Modulation matrix
- 113 destinations, **append-only** enumeration. Evaluation is in normalised space (skew respected) at the 32-grid; audible destinations interpolate linearly inside each cell, so they are per-sample smooth. Grain and per-grain-FX destinations are latched at spawn (Beads rule).
- 20 ms slot-depth ramps. A route change cross-fades a shadow of the old route, so removing a route causes no step.
- The engine never writes APVTS for modulation. Listeners never write other params (VST3 nested-write trap).

### 7. Tempo sync
O-GrainScatter `TempoTracker` port (ppqAt = integer samples × ppq/sample). Grid lock re-times on any discontinuity > 0.5 sample. Jitter is a telescoping displacement around each grid point, so it never drifts. In Tempo mode, density modulation is power-of-two only.

### 8. Latency
Reports 0. Dry is untouched, and mix 0 % is a bitwise dry null.

## Constraints carried forward (from suite memory)
Draw-before-obtain · separate CounterRng streams per consumer · no `offset < numSamples` scheduling · per-sample followers · double read positions · Nyquist-clamped filters · no `IIR::Filter` default ctor · density norm over cap uses N_eff · steal = move to spare slot · dB/generation for diffused loops · int64 seed in state · no nested param writes.

## Module gaps (optional, non-breaking)
G1 WindowMorphBank → grain-engine 1.2.0 · G2 `GrainPool::fadeOut` · G3 `GrainScheduler::rescheduleIn` · G4 `ReverbUnit` raw-atomic bind (the only one on the critical path; fallback is a local copy).

## Open items for the mockup / parameter-spec.md (blocking Stage 1)
- Confirm the additions `scale_root`, `sN_onset_trigger` ×4, `grain_limit`, which bring the count to 220.
- Freeze the 113-entry destination order.
- UI: stream selector, buffer waveform + grain dots, matrix view.
