# Stage 2: DSP — Execution Plan

**Date:** 2026-08-15
**Inputs:** CONTEXT.md, RESEARCH.md (this stage), `research/ARCHITECTURE.md` (BINDING),
ROADMAP.md Stage-2 phases, `parameter-spec.md` (BINDING, 31 params live in APVTS from Stage 1).

---

## Goal

Implement all 9 DSP components of O-Bitrot across ROADMAP's 5 phases (2.1 Engine Core + Tape →
2.2 CD Skip + Vinyl → 2.3 Packet Loss → 2.4 Crush + Quant → 2.5 Codec). Each phase ends with a git
commit and its render-harness probes green. Requirements in scope: FUNC-01..06, DSP-01..08,
PERF-01, QUAL-01/02.

Binding decisions carried in (do not re-litigate at execute):

- Constant 20 ms latency all modes; **CodecStage owns all latency** — a plain `kCompLatency`
  integer delay when disabled/μ-law, replaced by the GSM 160-frame chain in GSM mode. A
  `LatencyTrim` element lands at the chain tail in Phase 2.1 and is folded into CodecStage in 2.5.
- Real vendored libgsm, harness round-trip gate BEFORE integration; μ-law+extra-decimation
  fallback decided inside Phase 2.5 if the gate fails (CODEC_MODE keeps 2 choices either way).
- Ring + fractional read is NEW code borrowing CaptureBuffer idioms; sample-accurate tick math is
  NEW code (O-Polystutter contributes edges/fallbacks only); jump crossfades are TRUE two-head
  fades (RESEARCH.md §6).
- 8 RNG streams (`arbitration, tape, cd, vinyl, packet, jitter, dither, artifactSynth`), seeded
  `splitmix64(SEED · 0x9E3779B97F4A7C15 + k)`, reseeded in prepareToPlay + on SEED change; RNG
  consumed only at ticks/packets in fixed roll order tape→cd→vinyl.

---

## File Layout (new code)

```
plugins/O-Bitrot/
├── Source/
│   ├── PluginProcessor.h/.cpp        (modify: chain wiring, prepareToPlay, processBlock)
│   └── dsp/                          (new, header-only where practical — suite convention)
│       ├── CaptureRing.h             (2.1)
│       ├── ReadHead.h                (2.1)  clampAndScheduleJump() choke point + two-head fades
│       ├── MediaClock.h              (2.1)  sync + free, sample-accurate tick offsets
│       ├── RngBank.h                 (2.1)  splitmix64-derived juce::Random streams
│       ├── TapeTransport.h           (2.1)
│       ├── Arbitration.h             (2.1 skeleton, completed 2.2)
│       ├── CDSkip.h                  (2.2)
│       ├── VinylTransport.h          (2.2)
│       ├── ArtifactSynth.h           (2.2)  pops / ticks / chirps
│       ├── PacketLossStage.h         (2.3)
│       ├── CrushStage.h              (2.4)  fractional-hold latch (shared primitive)
│       ├── QuantStage.h              (2.4)
│       └── CodecStage.h              (2.1 skeleton w/ bypass delay → full in 2.5)
├── tests/render-harness/
│   ├── CMakeLists.txt                (2.1, O-Octagon template)
│   └── main.cpp                      (grows each phase)
├── third_party/libgsm/               (2.5: src/ 18 .c, inc/ 5 headers, COPYRIGHT)
├── CMakeLists.txt                    (modify: OUARICON_BUILD_TESTS gate 2.1; OBitrot_gsm 2.5)
└── NOTES.md                          (modify: latency scheme, libgsm license record)
```

---

## Tasks

### Phase 2.1 — Engine Core + Tape (Tasks 1–6)

1. [ ] **Render harness scaffolding — FIRST, before any DSP**
   - Files: `tests/render-harness/CMakeLists.txt`, `tests/render-harness/main.cpp`,
     `CMakeLists.txt` (OUARICON_BUILD_TESTS option), `Source/PluginProcessor.cpp` +
     `PluginEditor` include restructure.
   - Copy O-Octagon harness CMake (`juce_add_console_app`, compiles `PluginProcessor.cpp`
     directly, includes via `$<TARGET_PROPERTY:OBitrot,INCLUDE_DIRECTORIES>`, version DERIVED via
     `get_target_property` + FATAL_ERROR, hand-defined `JucePlugin_*` macros,
     `JUCE_WEB_BROWSER=0`). Restructure `createEditor()` with the `#if JUCE_WEB_BROWSER` guard
     NOW (include `PluginEditor.h` inside the guard) so Stage 3's WebView cannot break the
     harness.
   - Helpers in main.cpp: `check(name, ok, detail)` printing measured value + bound;
     position-deterministic `noiseAt(t)`; `bitIdentical()`/`firstDifference()`;
     `setBaseline(apvts)` resetting all 31 params to spec defaults (traps: CRUSH_BITS 16,
     CRUSH_RATE 20 kHz, MIX 100, the six `*_ENABLE`s per spec, HARD_EDGES off); param writes only
     via `setValueNotifyingHost(convertTo0to1(engineering))`.
   - First probe: FUNC-02 delay-compensated null — all-off, assert `out[n]` bit-equals
     `noiseAt(n − kCompLatency)` after the first block. **Expected to FAIL until Task 3 lands —
     that is the point.**
   - Depends on: none.

2. [ ] **Engine infrastructure: CaptureRing, RngBank, MediaClock, ReadHead**
   - Files: `Source/dsp/CaptureRing.h`, `RngBank.h`, `MediaClock.h`, `ReadHead.h`.
   - CaptureRing: absolute-index scheme + double-mod wrap from O-ReverseDelay `CaptureBuffer.h`;
     NEW fractional read (`floor`/lerp, ≥1-sample guard band below `writeAbs`); `prepare()` is the
     only allocation; `static_assert(kRingSeconds (2.5) >= 1.8 + 0.5 + 0.1)`.
   - RngBank: 8 named `juce::Random` streams, splitmix64-derived seeds; `reseed(int seed)` called
     from prepareToPlay and on per-block seed-change detection.
   - MediaClock: sync mode ports updateBeatSync EDGES only (BPM clamp 20–999, no-playhead ⇒
     free-run fallback, `wasPlaying` guard, unconditional commit of `lastPPQ/wasPlaying` at block
     end including early-outs); NEW sample-accurate tick math with `std::floor` (never int-cast —
     negative PPQ pre-roll), emitting tick offsets for split-block rendering. Free mode:
     sample-counting phase accumulator. 7-division PPQ table, 1 bar via time-sig (4/4 fallback).
   - ReadHead: fractional `readPosAbs`, per-channel interp state, ALL position/rate changes route
     through `clampAndScheduleJump()` (runtime lag clamp `[minLag, ringSpan − margin]`); TRUE
     two-head jump crossfade (old head keeps reading at its rate for the fade), linear equal-gain,
     1–5 ms computed in prepare, skipped under HARD_EDGES; NORMAL-state gentle re-approach trim
     ≤ ±2 % (ramped).
   - Depends on: Task 1 (harness exists to exercise them).

3. [ ] **Latency scheme + chain skeleton + dry/wet**
   - Files: `Source/dsp/CodecStage.h` (skeleton: bypass integer delay of `kCompLatency`),
     `PluginProcessor.h/.cpp`.
   - `kCompLatency = (int)std::ceil(0.020·fs)`; `setLatencySamples(kCompLatency)` once in
     prepareToPlay; `DryWetMixer::setWetLatency(kCompLatency)`. Hand-rolled integer delay ring
     (NOT `juce::dsp::DelayLine` — push-without-pop trap; bit-exactness required).
   - processBlock order (ARCHITECTURE Processing Order): push dry → write ring → advance clock /
     split at ticks → read heads → (stages, unity for now) → CodecStage bypass delay →
     mixWetSamples. `ScopedNoDenormals`. Write-then-read per sample.
   - FUNC-02 null probe from Task 1 goes green here.
   - Depends on: Task 2.

4. [ ] **TapeTransport + arbitration skeleton + enable fades**
   - Files: `Source/dsp/TapeTransport.h`, `Arbitration.h`, `PluginProcessor.cpp`.
   - Interval table `{1.0, 0.67, 1.5, 0.5, 2.0}`; stop = ramp to 0, hold, ramp back; linear rate
     ramp over TAPE_RAMP ms (the ramp IS the sound — no crossfade on rate changes, only on
     position jumps); phase accumulator never reset.
   - Arbitration at ticks: fixed roll order tape→cd→vinyl (cd/vinyl stubs roll but never fire in
     2.1), collision pick via `arbitration` stream; disabled family never wins; mid-event
     disable releases gracefully. ~10 ms enable fades on all `*_ENABLE`.
   - Seed-change detection per block start → RngBank reseed.
   - Depends on: Tasks 2, 3.

5. [ ] **Phase 2.1 probe suite**
   - File: `tests/render-harness/main.cpp`.
   - DSP-01: sine input, `autocorrPitchHz()` from O-simpleGrain (window ≤ 1024 @ 48 kHz, hop 256,
     τ constrained ±20 % around expected f); assert continuous instantaneous-frequency ramps on
     bends and ramp-to-silence with no click on stops.
   - FUNC-04: two renders on FRESH processor instances, same seed ⇒ `maxAbsDiff == 0.0` +
     liveness; different SEED ⇒ outputs differ.
   - QUAL-02: 512-vs-4096 memcmp bit-identity + ragged `{1,7,64,333,4096}` variant, families on,
     fixed seed.
   - FUNC-03: sync grid follows a BPM change; free mode event rate matches Hz setting.
   - Depends on: Task 4.

6. [ ] **Phase 2.1 gate: build + probes green + commit**
   - `cmake -DOUARICON_BUILD_TESTS=ON` regen, `ninja OBitrot_VST3 OBitrot_AU O-Bitrot-render-test`,
     run harness, exit 0. Commit `feat(o-bitrot): stage 2 phase 2.1 — engine core + tape`.
   - Depends on: Task 5.

### Phase 2.2 — CD Skip + Vinyl (Tasks 7–9)

7. [ ] **CDSkip ladder + ArtifactSynth**
   - Files: `Source/dsp/CDSkip.h`, `ArtifactSynth.h`.
   - CD_SEVERITY-weighted rung pick (cd stream): rung 1 = 30–80 ms `FirstOrderTPTFilter` dip
     (20 kHz→~2 kHz→back); rung 2 = 2–20 ms mute + filtered tick; rung 3 = CD_SEGMENT loop at
     exact intervals with restart chirp (~3→8 kHz over ~4 ms, τ≈1.5 ms), recovery jump forward
     toward `writeAbs − minLag` via `clampAndScheduleJump()`.
   - ArtifactSynth (artifactSynth stream): pops (±impulse pair → 1–3 kHz TPT LPF, ±3 dB
     variation), ticks (impulse → ~4 kHz BPF), chirps — all ~−18 dBFS pre-mix. Run post-filters
     every sample so IIR state stays continuous.
   - Depends on: Task 6.

8. [ ] **VinylTransport + full arbitration**
   - Files: `Source/dsp/VinylTransport.h`, `Arbitration.h`, `PluginProcessor.cpp`.
   - Revolution quantum 1.8 s / 1.333 s from VINYL_RPM; jumps = integer revolution multiples,
     rate stays 1.0; locked groove = exact one-revolution re-jump + pop per pass; all jumps
     through the choke point. Complete three-family arbitration.
   - Depends on: Task 7.

9. [ ] **Phase 2.2 probes + gate + commit**
   - DSP-02: loop repeats at exact CD_SEGMENT intervals with chirp at restarts, jumps forward on
     recovery. DSP-03: ramp-marker input locates discontinuities ⇒ jump distances are integer
     revolution multiples; `autocorrPitchHz` shows no pitch change across jumps. FUNC-01: event
     onsets quantized to the clock grid. Collision determinism: same seed ⇒ same arbitration
     winners. Re-run full 2.1 suite (QUAL-02 must still hold).
   - Commit `feat(o-bitrot): stage 2 phase 2.2 — cd skip + vinyl`.
   - Depends on: Task 8.

### Phase 2.3 — Packet Loss (Tasks 10–11)

10. [ ] **PacketLossStage**
    - Files: `Source/dsp/PacketLossStage.h`, `PluginProcessor.cpp`.
    - Own 20 ms sample-counting packet grid (independent of MediaClock). GE Markov advanced once
      per packet (packet stream): `p_BG = 1/E[B]` (PACKET_BURST → E[B] ∈ [1,8]),
      `p_GB = clamp(π_B·p_BG/(1−π_B), 0, 1)` (PACKET_LOSS → π_B scaled 0–0.6); lose at 0.5 in
      Bad, 0.01 in Good. Concealments: Silence / Repeat / Decay (−3 dB per repetition) /
      Substitute (AMDF over 2–15 ms lags on last good packet, −1 dB per cycle, auto-degrade to
      Decay when AMDF min ≥ 0.5× mean). Packet-boundary crossfades 1–5 ms unless HARD_EDGES.
      2 × 20 ms history per channel preallocated at max fs.
    - Depends on: Task 9.

11. [ ] **Phase 2.3 probes + gate + commit**
    - DSP-04: ≥60 s-equivalent render, consecutive-loss run-length histogram vs geometric
      expectation (ratio bounds), fixed seed. Four concealment modes measurably distinct in
      harness renders (Substitute-vs-Decay distinctness measured on a periodic input; if aliased,
      FLAG for verify re-scope — do not silently pass). QUAL-02 re-run with packet grid active.
    - Commit `feat(o-bitrot): stage 2 phase 2.3 — packet loss`.
    - Depends on: Task 10.

### Phase 2.4 — Crush + Quant (Tasks 12–14)

12. [ ] **CrushStage (fractional-hold SRR + jitter)**
    - File: `Source/dsp/CrushStage.h`, `PluginProcessor.cpp`.
    - Fractional-crossing interpolated hold (DeRez fix): `phase += rate`,
      `held = last·pos + input·(1−pos)` at crossing; CRUSH_RATE target smoothed per-sample
      (`juce::SmoothedValue` on the target), runtime clamp fs/2, phase accumulator never reset;
      CRUSH_JITTER: `phase += rate·(1 + amt·noise())` (jitter stream). Latch written as the
      shared primitive CodecStage's 8 kHz grid reuses in 2.5 (separate instances/state).
    - Depends on: Task 11.

13. [ ] **QuantStage (fractional bits + TPDF + env-driven depth)**
    - File: `Source/dsp/QuantStage.h`, `PluginProcessor.cpp`.
    - Mid-tread `delta = 2·exp2(−bits)`, `out = delta·floor(x/delta + 0.5)`, bits target
      per-sample smoothed. TPDF `(r1−r2)·delta·(CRUSH_DITHER/2)` pre-quantize (dither stream).
      PER-SAMPLE one-pole follower (attack ~5 ms, release ~120 ms), env→dB→t,
      `bitsNow` between CRUSH_BITS and floor 1.0 scaled by |CRUSH_ENV_AMT|, sign = duck/pump.
      Follower input sanitized (`std::isfinite` → 0); state never holds NaN.
    - Depends on: Task 12.

14. [ ] **Phase 2.4 probes + gate + commit**
    - DSP-06: full-range CRUSH_BITS/CRUSH_RATE sweeps behind a LIVENESS gate (prove the param
      moves the DSP before asserting no zipper); fractional rates/bits render without
      warble/periodicity error. DSP-07: duck vs pump polarity behaves; offline-vs-realtime
      block-size invariance (QUAL-02 re-run — per-sample follower is the load-bearing piece).
      QUAL-01: DC, silence, full-scale square, NaN injection then clean input ⇒ recovery, never
      sticky NaN/Inf.
    - Commit `feat(o-bitrot): stage 2 phase 2.4 — crush + quant`.
    - Depends on: Task 13.

### Phase 2.5 — Codec (Tasks 15–18)

15. [ ] **Vendor libgsm (license-first)**
    - Files: `third_party/libgsm/{src,inc}` (18 .c per RESEARCH §4, 5 headers, `COPYRIGHT`
      verbatim from quut.com gsm-1.0.22), `NOTES.md` (record license BEFORE vendored code
      commit), `CMakeLists.txt` (`OBitrot_gsm` STATIC, defines `SASR NDEBUG
      NeedFunctionPrototypes=1` — never WAV49/FAST —, `/w` / `-w` PRIVATE, PIC ON; link
      PRIVATE into OBitrot; harness gets it transitively — no separate link).
    - Commit vendoring + license record as its own commit.
    - Depends on: Task 14.

16. [ ] **GSM round-trip harness gate — BEFORE integration**
    - File: `tests/render-harness/main.cpp`.
    - Standalone probe: two handles (`gsm_create` enc + dec), 160-sample sine frame,
      scale in `(gsm_signal)(x·4096)<<3` masked `0xFFF8`, out `(s>>3)/4096.0f`; assert non-NaN,
      bounded, correlated with input. **Gate:** pass ⇒ proceed with real GSM; fail ⇒ decide
      μ-law+extra-decimation fallback NOW (inside this phase), CODEC_MODE keeps 2 choices.
    - Depends on: Task 15.

17. [ ] **CodecStage full implementation**
    - Files: `Source/dsp/CodecStage.h`, `PluginProcessor.cpp`.
    - Mono sum → HPF 300 Hz (2× cascaded 2-pole, `IIR::ArrayCoefficients` only, coefficients in
      prepareToPlay) → LPF 3400 Hz (4-pole) → 8 kHz fractional-hold latch (`rate = 8000/fs`) →
      μ-law round trip (continuous formula + 8-bit mid-tread) OR GSM: 160-slot accumulate,
      encode+decode on frame completion, **output the previously-decoded frame while the next
      accumulates** (constant one-frame delay — never emit the just-completed frame same-tick),
      output frame primed with zeros in prepareToPlay → hold-upsample → post LPF ~3400 Hz →
      equal-power CODEC_MIX blend with pre-codec stereo.
    - Fold LatencyTrim: disabled/μ-law route through the plain `kCompLatency` delay; GSM chain
      replaces it (structural delay = exactly `0.020·fs`). Mode switch crossfaded 10 ms, both
      sub-paths delay-aligned by construction. `gsm_create` in prepareToPlay, `gsm_destroy` in
      releaseResources/destructor.
    - Depends on: Task 16.

18. [ ] **Phase 2.5 probes + Stage-2 exit gate + commit**
    - DSP-05: μ-law bandwidth ~300–3400 Hz, level-dependent quantization noise; codec-path vs
      plain-delay cross-correlation peak within ±fs/8000 samples of `kCompLatency` (GSM);
      FUNC-02 null re-run in every CODEC_MODE/disabled state (latency = reported 20 ms in all
      modes).
    - PERF-01: harness render-time measurement ≤ 15 % single core @ 48 kHz worst case (all
      families + GSM) — measurement printed, not wall-clock inside a pass/fail race; assert the
      ratio bound only.
    - Full-suite re-run (all 2.1–2.4 probes). Build plugins; local pluginval strictness 10 run
      2–3× (VST3 + AU); `./scripts/build-and-install.sh O-Bitrot`; brief DAW smoke check.
    - Commit `feat(o-bitrot): stage 2 phase 2.5 — codec (mu-law + gsm)`.
    - Depends on: Task 17.

---

## Pitfall Checklist (execute agent MUST hold these — repo memory)

| Trap | Where | Discipline |
|---|---|---|
| Read latched before capture write | 2.1 | Write ring then read, per sample |
| Shared RNG breaks block invariance | 2.1 | 8 streams, consume only at ticks/packets, fixed order |
| `static_cast<int>` on PPQ | 2.1 | `std::floor` — negative pre-roll PPQ |
| Pitch-probe window vs ramp period | 2.1/2.2 probes | Window ≤ 1024 @ 48 kHz vs 150 ms ramps; τ ±20 % |
| DelayLine push-without-pop | 2.1/2.5 | Hand-rolled integer delay ring |
| Held-sample "crossfade" | 2.1 | True two-head fades |
| Harness param leak / wrong neutrals | all probes | `setBaseline` every probe; CRUSH_BITS 16, CRUSH_RATE 20 kHz, MIX 100 |
| Vacuous probes | 2.4 | Liveness gate before every no-zipper/no-change assertion |
| Block-rate follower | 2.4 | One-pole per SAMPLE |
| Sticky NaN in follower state | 2.4 | Sanitize input; pathological-input probe |
| `Coefficients::makeXXX` on audio path | 2.5 | `ArrayCoefficients` only, computed in prepareToPlay |
| GSM same-tick frame emit | 2.5 | Output previous frame; constant 160-grid-sample delay |
| Choice param with 1 entry | 2.5 fallback | CODEC_MODE keeps 2 choices in every scenario |
| Fixture mirrors constants | harness | Derive version/constants from target properties |
| Wall-clock in verdicts | all probes | Never; deterministic bounds only |
| Host-only weirdness | DAW checks | Quit/reopen DAW before chasing DSP |

---

## Success Criteria (Stage-2 exit — maps to VERIFICATION at /plugin-verify)

- [ ] FUNC-02: all-off passthrough bit-transparent minus reported latency, in every codec mode
- [ ] DSP-01: tape bends/stops show continuous instantaneous-frequency ramps, no clicks
- [ ] DSP-02: CD loop repeats at exact CD_SEGMENT intervals with restart chirp; recovery jumps forward
- [ ] DSP-03: vinyl jumps are integer revolution multiples; no pitch change (autocorr probe)
- [ ] DSP-04: burst lengths match geometric expectation; 4 concealment modes distinct (or Substitute≡Decay FLAGGED)
- [ ] DSP-05: μ-law bandwidth ~300–3400 Hz; GSM round trip verified pre-integration; latency = 20 ms all modes
- [ ] DSP-06: liveness-gated zipper probes pass full-range; fractional rates/bits clean
- [ ] DSP-07: env-driven depth per polarity; DSP-08: dither + jitter functional
- [ ] FUNC-01/03: events quantized to clock grid; sync follows tempo, free matches Hz
- [ ] FUNC-04: same seed ⇒ bit-identical fresh-instance renders; different seed differs; seed persists
- [ ] QUAL-01: pathological input never yields sticky NaN/Inf; jumps crossfaded unless HARD_EDGES
- [ ] QUAL-02: 512-vs-4096 (+ ragged) bit-identity green at every phase gate
- [ ] PERF-01: no allocations/locks/logging in processBlock; ≤ 15 % single core @ 48 kHz worst case
- [ ] libgsm license vendored + recorded in NOTES.md before vendored-code commit; zero GPL code
- [ ] pluginval strictness 10 local (VST3 + AU) clean, run 2–3×
- [ ] 5 phase commits, each with its probe suite green
