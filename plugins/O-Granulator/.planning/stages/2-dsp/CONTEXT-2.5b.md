# Stage 2 — DSP · Phase 2.5b (Onset detector + burst, MPM pitch follower, absolute pitch follow) — Context

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.5b of 7
**GSD phase:** discuss
**Date:** 2026-11-05
**Branch:** `main` (trunk-based) @ `2cf30120`
**Participants:** Taylor Brook, Claude

---

## Entry Check — contract checksums

All six were re-hashed this session and match the STATUS.md frontmatter. No contract was amended at this discuss. D17–D19 are recorded below as ARCHITECTURE errata and are not edited into ARCHITECTURE.md.

| Contract | SHA-256 (prefix) | Result |
|---|---|---|
| `BRIEF.md` | `f9c748a8…` | ✅ |
| `REQUIREMENTS.md` | `f0b15388…` | ✅ (2.5a-verify revision) |
| `parameter-spec-draft.md` | `d0b1457e…` | ✅ |
| `parameter-spec.md` | `f5ce3595…` | ✅ |
| `research/ARCHITECTURE.md` | `f0cf753b…` | ✅ |
| `ROADMAP.md` | `1060dae0…` | ✅ |

---

## Discussion Summary

ARCHITECTURE already specifies most of 2.5b:
- the onset detector (§15, Algorithm §8): HP 150 Hz, fast/slow envelopes, +6 dB ratio, 50 ms refractory, and a 150 ms pulse source
- the MPM follower (§15, Algorithm §7): decimation D = max(1, round(fs/12000)), a 64-tap sinc FIR, W 1024, FFT order 11, hop 128 on an absolute counter, k 0.9, clarity ≥ 0.8, a −50 dBFS gate, 250 ms hold, 40–1500 Hz, a 20 ms slewed source
- absolute pitch follow against `scale_root` (Algorithm §5)
- onset burst semantics (§4, locked as D14 in 2.5a: re-phase, and the clock keeps running)

Discuss reopened four points:
1. Which f0 a grain harmonises against.
2. The onset trigger in Tempo mode.
3. `sN_scale` = Off with follow on.
4. MPM octave-error robustness.

Hooks in the code at `2cf30120`:
- `ModSources.h:64`: Pitch (6) and Onset (7) output exactly 0 ("2.5b")
- `GranulatorParamIds.h`: `scale_root` (:122), `sN_onset_trigger` (:154), `pitch_follow_sN` and `onset_threshold` are registered and read nowhere

---

## Approach Decisions (locked this session)

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D17 | Pitch-follow reference note | **f0 history ring (ARCH §5 erratum).** Each MPM hop result (f0 note as a float + a confident flag) is stored in a small ring indexed by **capture-ring write position**. There is one entry per hop; at 30 s max that is about 2.8 k entries, sized in `prepareToPlay`. The entry is time-aligned to the centre of the analysed window: the hop's write position minus the MPM group delay. At spawn, an absolute-follow grain looks up the f0 of the content at its **read start** (D behind the head; frozen: inside `heldSpan`). It uses that f0 for `t = quantise_abs(f0 + st)` and interval `t − f0`. If the entry at that position is unconfident or missing (never written, or older than the live span), the grain uses interval mode. While frozen the ring is not written (head-stop), so the held span keeps its f0. The **Pitch matrix source** still reads the **live** tracker (latest hop, slewed). | ARCH used the live f0, which is wrong whenever position > 0 or freeze is on: the grain plays content whose note is not the current one, and the harmony goes off-key. With the ring, the follow works at any position and while frozen ("harmonise this held chord"). It costs one lookup per spawn and needs no allocation. |
| D18 | Onset trigger in Tempo mode | **Free mode: D14 unchanged** (re-phase: one grain at the detected sample, and the clock re-phases to it). **Tempo mode: one extra grain at the detected sample, and the grid clock does NOT move.** The extra grain uses its own onset index space, so the grid's telescoping-jitter draws and onset k indices are not displaced. | D14's re-phase in Tempo would move the stream off the host grid, and the Tempo clock only re-locks at transport discontinuities, so the stream would stay off. Keeping the grid keeps FUNC-04 intact while the trigger is on. |
| D19 | `sN_scale` = Off with `pitch_follow_sN` On | **Chromatic from `scale_root`.** For the absolute target only, Off is treated as 12-TET chromatic, so each grain lands on the nearest semitone (a tune-to-semitone harmoniser). With follow off, Off stays unquantised, as now (D4 unchanged). Degree masks do not apply to the implicit chromatic set. | If follow were a no-op under Off it would read as a broken switch. Chromatic is the least surprising default for "follow the tracked note". |
| D20 | MPM octave-error guard | **Built in from the start and gated.** Median-of-3 over hop results, plus octave-jump hysteresis: a jump of ≈ ±12 st (±50 ct) must persist for 2 consecutive hops before it is accepted. The guard adds about 1 hop of source latency (≈ 10.7 ms at 12 kHz decimated). Gate rows use octave-error stimuli: a strong 2nd harmonic, and a breathy tone (sine + noise). | ARCH listed the guard as the fallback for octave flips on breathy or polyphonic input. Those flips would otherwise first show up at the Stage 3 DAW check, which is the most expensive place to find them. |

Unchanged: D1–D16. D14 is narrowed to Free mode by D18. D16's 2.5b DAW items (onset bursts on drums, pitch-follow harmonisation on a monophonic line) join the deferred Stage 3 DAW pass with DAW-2.5a.md items 1–8 (VERIFICATION-2.5a). A 2.5b DAW FAIL reopens FUNC-08 / DSP-08.

---

## Requirements Confirmed (Phase 2.5b scope)

**Goal:** FUNC-08 and DSP-08. It completes FUNC-06 (Pitch and Onset sources). It also closes the 2.5b remainders of QUAL-01 (onset bursts), PERF-01 (onset/MPM) and PERF-03 (2.5b sources) carried from VERIFICATION-2.5a.

**Components:**

- **Onset detector (ARCH §15, Algorithm §8):**
  - Runs per sample on the conditioned input: post `input_gain`, mono sum, pre-ring. Never the feedback return or the dry path (2.5a execution default).
  - HP 150 Hz feeds a fast envelope (0.5 ms / 10 ms) and a slow envelope (80 ms / 80 ms).
  - Fires when fast_dB > `onset_threshold` AND fast/slow > +6 dB.
  - A 50 ms per-sample refractory follows each detection.
  - The event offset is the first sample where the condition holds.
  - Coefficients are computed in `prepareToPlay`; `onset_threshold` is read at grid ticks.
  - NaN-sticky reset.
  - Outputs: (a) an event at the exact sample, (b) the Onset source: instant attack, 150 ms exponential decay, unipolar.
  - The detector always runs. It is cheap, and it keeps the Onset source live regardless of the triggers.
- **Burst trigger (`sN_onset_trigger`):**
  - Free mode: D14 (`process(i)` → re-phase so one grain fires at i).
  - Tempo mode: D18 (one extra grain at i, grid untouched).
  - Disabled streams ignore events.
  - A frozen stream still fires: a live transient retriggers the held buffer.
  - Spawn draws follow the existing draw-before-obtain rule. Onset grains need a reserved CounterRng stream/slot so the 2.2–2.5a draw indices never move (the N3 null depends on it).
- **Decimator + MPM (ARCH §15, Algorithm §7):**
  - Same contract as ARCH: D = max(1, round(fs/12000)), a 64-tap windowed-sinc FIR (cutoff 0.4·fs/D, designed in `prepareToPlay`), W 1024, `juce::dsp::FFT` order 11 (real-only forward + inverse, buffers preallocated), hop 128 on an **absolute** decimated counter, key-max k 0.9, parabolic refinement, clarity ≥ 0.8 and level ≥ −50 dBFS to accept, else hold 250 ms then unconfident, range 40–1500 Hz.
  - It runs on the audio thread at hop boundaries.
  - D20 guard on the hop results.
  - Outputs:
    - the live f0 note plus confidence, feeding the Pitch source: MIDI 24..108 → 0..1, 20 ms slew, holds its last value when unconfident
    - the D17 f0 history ring entry
- **Absolute pitch follow (`pitch_follow_sN`, `scale_root`):**
  - At spawn: `f0 = ring lookup at read start (D17)`. If confident, `t = quantise_abs(f0 + st)` against the stream's scale and degree mask from `scale_root` (D19: Off → chromatic). Then interval = `t − f0`, then the ±24 st clamp.
  - Otherwise interval mode (the D4 path).
  - `scale_root` is read at the spawn sample (spawn-latched; it is a Choice, and 2.5a R55 says Choice params are not matrix destinations).
- **Matrix sources 6 (Pitch) and 7 (Onset)** go live. The 2.5a sparse eval, residuals and telemetry need no change beyond the source values. The source telemetry atomics carry them.

**Test criteria (gate):**

- [ ] **Neutral null first (N3):** follow off ×4, trigger off ×4, no slot on sources 6/7. The render must be bit-identical to the 2.5a tree (out-of-tree vs `2cf30120`, N0–N2 style), with the 299 carried rows unchanged.
  - This must hold before any 2.5b row counts.
  - The detector and MPM run in the neutral case, so the N3 null also proves the analysis tap never reaches the audio path.
  - Negative controls, each of which must DIFFER: `s1_onset_trigger` on with a percussive stimulus; `pitch_follow_s1` on with a tonal stimulus; a slot Onset → level at 1 %; a slot Pitch → level at 1 %.
  - Positive control, which must be IDENTICAL: `onset_threshold` swept with every trigger off and no Onset routes.
- [ ] **FUNC-08:**
  - A burst fires at the detected sample, within 0 samples of the detector's reported offset, on every enabled stream with the trigger on.
  - Nothing fires on disabled or trigger-off streams.
  - The refractory holds for 50 ms.
  - Free mode re-phases (D14). Tempo mode keeps every grid onset bit-identical to the trigger-off render (D18).
  - Detection is placed by a synthetic click train with known offsets: hit rate, no double fires, and no fires below threshold.
- [ ] **DSP-08:**
  - Sines from 55 to 1000 Hz track within 10 cents at 44.1 / 48 / 96 k.
  - Clarity holds through silence and white noise (no confident output), and the 250 ms hold then goes unconfident.
  - D20: the strong-2nd-harmonic and breathy stimuli show no octave flip that persists more than 1 hop.
- [ ] **D17:** a stepped-note stimulus (note A, then note B), with a grain reading at position > 0 over the A region. The interval is computed from A's f0, not the live B. Frozen over A while the live input plays B: still A. An unconfident region gives interval mode.
- [ ] **D19:** follow on, scale Off → every grain lands within 1 cent of a 12-TET note relative to `scale_root`. Follow off, scale Off → unchanged from 2.5a.
- [ ] **Absolute follow quantisation:** at least one 12-EDO collection, one non-12 EDO (e.g. 19-EDO), Bohlen-Pierce (period 1901.955 ct), and Custom. Each with a degree mask. The target lands on an active degree from `scale_root`, and the ±24 st clamp holds.
- [ ] **FUNC-06 completion:** Pitch and Onset → a sample of destinations across every class (ramped, spawn-latched, skewed, reverb atomics), both depth signs. Removing a route returns to base without a step (same-motion control).
- [ ] **QUAL-01:** onset bursts on a drum loop at high trigger rates show no clicks beyond the same-density reference (tonal source for the click probe).
- [ ] **PERF-03:** block-size null at 32/441/512/4096/variable with LFO + env + onset + pitch routings active, triggers on in Free AND Tempo, follow on, feedback 80 % + freeze toggle, seeded, plus a different-seed control. The MPM hop and detector events must be identical across block sizes, because they run on absolute counters.
- [ ] **PERF-01:** an allocation gate over detector events, MPM hops (FFT), follow lookups and source routings: 0 allocations.
- [ ] **Telemetry:** the Pitch and Onset source atomics are in range and equal the engine values at the cell.
- [ ] **CPU (INFO):** C6 = C5 + the detector + MPM + 4 follow streams, against the 30 % budget. The ≈ 50 µs-per-hop spike is reported separately (worst single block at 32 samples).
- [ ] **D16 2.5b DAW items:** deferred to the Stage 3 DAW pass (with DAW-2.5a.md); a `DAW-2.5b.md` is written at execute.

---

## Carry-ins

| Item | Handling |
|---|---|
| D16 DAW check deferred to Stage 3 (VERIFICATION-2.5a) | The 2.5b items join it. DAW-2.5b.md is written at execute. |
| QUAL-01 / PERF-01 / PERF-03 2.5b remainders | Gate rows above. |
| W2o margin (0.017 dB) | The N3 neutral null keeps it untouched. If a row flips, widen the seeds. Never loosen the threshold. |
| Item 6 watch (bar-origin re-anchor keeps the slot, VERIFICATION-2.5a) | D18 must not touch the Tempo slot. The Tempo-mode trigger row re-checks TS1 with triggers on. |
| `Engine.h` "analysis — 2.5" / `ModSources.h:64` stage comments | Update them when this code is opened. |

## Execution Defaults (Claude's call, recorded for plan)

- The detector, decimator, MPM and f0 ring are owned by one `Analysis` unit fed per sample from the conditioned input. All of it is sized in `prepareToPlay`; nothing grows on the audio thread.
- The f0 ring stores `float note` plus a confidence byte (or NaN for unconfident). The write index is `writeAbs / (hop·D)`, so the lookup is a pure function of the absolute write position. It is block-size invariant by construction.
- MPM's group delay (half window + FIR delay + D20 median delay) is a constant computed in `prepareToPlay`. It is applied when an entry is written.
- Onset events are queued as absolute sample indices (memory: block-relative scheduling drops cross-block events), consumed by the stream pass that contains them.
- `onset_threshold` is read at grid ticks only.
- The FFT is preallocated in `prepareToPlay`. `juce::dsp::FFT::performRealOnlyForwardTransform` does not allocate. Verify this in the M-gate, not by reading the code.

## Constraints Identified

- The analysis tap never reaches the dry or wet audio path (N3 positive proof).
- Reported latency stays 0. MPM latency (≈ 45 ms + D20 hop) affects only the Pitch source and the f0 ring timestamps.
- No background threads (determinism: offline == live).
- No allocation on the audio thread.
- No APVTS write-back.
- No 2.2–2.5a CounterRng draw moves. Onset grains get a reserved stream/slot.
- Per-pass arrays stay at 32 samples. Analysis is per sample, and hops run on absolute counters.

## Open Questions (for research)

- **f0 ring alignment under feedback:** the ring stores the input's f0. Feedback-written content in the capture ring carries shifted pitches, so grains that read a feedback-heavy region get the input f0. Decide whether that is acceptable (follow tracks the "source note"), or whether the analysis should tap the ring write (input + feedback) instead. That alternative breaks the "conditioned input only" default and makes MPM see its own grains.
- **MPM group delay and the hop → write-position mapping:** the exact constant (W/2·D + FIR (63/2)·D + median-of-3 hop), and how lookup should treat a grain whose read span straddles two notes: start-only, or the majority over the grain length.
- **Onset grain index space (D18 / D14):** whether a separate onset index (its own draw stream) is needed in Free mode as well, or whether re-phase can keep using the stream's onset k. Check this against N3 and the telescoping-jitter contract.
- **`quantise_abs` for non-1200 periods:** Bohlen-Pierce (1901.955 ct) is relative to `scale_root` in which octave register? Pick a reference (e.g. root at MIDI 60 + k·period) and confirm GrainScatter precedent.
- **D20 hysteresis threshold:** whether ±12 st ±50 ct catches octave-down (sub-harmonic) errors as well as octave-up errors, and whether a fifth (+19 st, 3rd-harmonic lock) needs a guard too.
- **Onset detector at 96/192 k:** whether the HP 150 Hz and the 0.5 ms attack are stable at every rate (TPT form).
- **CPU spike:** the hop-boundary FFT on a 32-sample block. Should it be spread across passes (e.g. forward FFT on one pass, inverse + peak pick on the next)? Spreading is still deterministic because it is keyed on the absolute counter.

## Next Phase

Ready for: research (`/plugin-research O-Granulator 2-dsp`, Phase 2.5b).
