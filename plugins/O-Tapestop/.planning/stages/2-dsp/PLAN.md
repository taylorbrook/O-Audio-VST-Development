# Stage 2: DSP — Execution Plan

**Date:** 2026-08-15
**Contracts:** `research/ARCHITECTURE.md` (binding), `stages/2-dsp/CONTEXT.md` (debt clamp, retrigger-everywhere, aliasing-as-character), `stages/2-dsp/RESEARCH.md` (port map, probe recipes, §5 decisions 1–9)
**Structure:** Three phases per ROADMAP (2.1 core stop/start → 2.2 resync + tempo sync → 2.3 scratch + toneTrack). Git commit per phase; render-harness probes are the phase gates.

---

## Goal

Implement the complete O-Tapestop DSP contract: single interpolated playhead over a 26 s capture ring with curve-morph stop/start ramps, Signalsmith resync (fall-behind → 1.25× catchup → 50 ms crossfade-skip) with bitwise post-resync null, drawn-envelope scratch mode, and speed-tracking toneTrack LPF — covering FUNC-01..04, DSP-01..05, PERF-01, QUAL-01.

---

## Phase 2.1 — Core Varispeed + Stop/Start

**Goal:** Ring + single voice + transport (Stop mode, Free timing only); bitwise-bypass proven; harness scaffold live from day one.

### Task 1 — Render-harness scaffold
- **Files:** `tests/render-harness/CMakeLists.txt`, `tests/render-harness/main.cpp` (new)
- **Depends on:** none
- CMake: copy O-Octagon `tests/render-harness/CMakeLists.txt` shape — `juce_add_console_app(O-Tapestop-render-test)`, compile `Source/PluginProcessor.cpp` directly with `JUCE_WEB_BROWSER=0` (never list the editor TU); borrow properties from target **`OuariconTapestop`** (not the folder name); version DERIVED via `get_target_property(... JUCE_VERSION)` + `FATAL_ERROR` if empty; hand-define `JucePlugin_*` macros + `JUCE_STANDALONE_APPLICATION=1`, `JUCE_USE_CURL=0`. Stage 1 already carries the `OUARICON_BUILD_TESTS` hook and the `#if JUCE_WEB_BROWSER` editor guard — no plugin-CMake or processor edits needed for wiring.
- Helpers (O-Octagon/O-ReverseDelay lineage): `check(name, ok, detail)` printing `[PASS]/[FAIL]` + measured vs bound, exit `failures == 0 ? 0 : 1`; positional `noiseAt(t) = hash(t)` (never a sequential RNG); `bitIdentical()` = per-channel memcmp + `firstDifference()`; `setBaseline(apvts)` resetting **all 14 params to shipped defaults (MIX neutral = 100, not 0), forcing ENGAGE off, then rendering one settle block so the transport returns to Bypassed** — transport state leaks between probes, not just param state. All param writes via `setValueNotifyingHost(range.convertTo0to1(engineering))`; ENGAGE edges are exactly this call made between processBlock calls.
- Add `getDebtSamplesForTest()` accessor on the processor behind `OUARICON_RENDER_HARNESS=1` (O-ReverseDelay precedent) — defined this phase so the harness API is stable; consumed by P4 in Phase 2.3.
- Invocation: `cmake -DOUARICON_BUILD_TESTS=ON` regen → `ninja O-Tapestop-render-test` → run; exit code is the gate.

### Task 2 — Port CaptureBuffer (trimmed)
- **Files:** `Source/dsp/CaptureBuffer.h` (new, from O-ReverseDelay)
- **Depends on:** none
- Keep: `prepare(sr, maxSeconds)` (the only allocation), alloc-free `clear()`, `pushSample`, `readAbs(ch, absIndex)` with the double-mod negative-tolerant wrap, `getTotalWritten`. **Drop:** `pushLooped`, `pushCrossfaded` (freeze machinery), `monoSum` (full-stereo varispeed — mono-summing forbidden).
- `kCaptureSeconds = 26.0` with the full derivation comment (scratch full-reverse: r = −2 ⇒ 3 s debt/s × 8 s = 24 s, + 2 s margin). Pre-history reads return cleared zeros — makes the debt clamp's worst case safe in the first 26 s after load.
- Per-sample `getSample` reads are fine at 2 voices × 4 taps × 2 ch (profiled precedent at 64 grains); no raw-pointer hoisting in v1.0.

### Task 3 — WindowLut (Hann-only, ~40 lines)
- **Files:** `Source/dsp/WindowLut.h` (new)
- **Depends on:** none
- Take exactly the Hann table `build()` + `readAt` (clamp, lookup, lerp) from the O-ReverseDelay file; none of the shapes/tilt/taper/normalisation machinery (it serves grain populations, not a 2-voice crossfade). Crossfade gains per ARCHITECTURE: `fadeOut = hann(0.5 + φ/2)`, `fadeIn = hann(φ/2)` — equal-power by construction.

### Task 4 — VarispeedVoice + Catmull-Rom kernel
- **Files:** `Source/dsp/VarispeedVoice.h` (new, ~30-line POD)
- **Depends on:** Task 2
- POD per ReverseGrain discipline: `{ bool active; double readAbsFrac; float gain; }` — everything latched at spawn; speed `r` lives in the transport (exactly one voice is driven at a time), not the voice.
- Hand-rolled Catmull-Rom in Horner form over `readAbs(ch, i−1 … i+2)` (JUCE's `Interpolators::CatmullRom` is a stream resampler — wrong shape for random-access varying-rate reads).
- **Integer-read fast path at d = 0, r = 1** (live riding / post-resync) — this carries the DSP-03 null; never route it through the interpolator (bitwise-exact by construction, and keeps i+1/i+2 taps off not-yet-written samples). The interpolated path is only taken with debt d ≥ `kInterpGuard = 4`.
- Per-read release-build debt clamp `[kInterpGuard, ringSpan − kInterpGuard]`; debug `jassert` gated so Stopped-hold resume paths (specified clamp behavior per CONTEXT decision 1) don't assert.

### Task 5 — TapestopTransport (Stop-mode subset)
- **Files:** `Source/dsp/TapestopTransport.h` (new)
- **Depends on:** Task 4
- States this phase: `Bypassed → SpinDown → Stopped → SpinUp → Bypassed` (spin-up ramps simply back onto the lagging playhead — no resync yet; Catchup/ResyncXfade arrive in 2.2).
- Curve morph: `r(u) = (1−u)^p` down / `u^p` up, `p = 2^(2c)`; **per-sample `pow`** in this phase (P3 exactness; optimise only on measured need); ramp phase `u` and `readAbsFrac` in **double** (`u += 1.0/durationSamples` — float drifts ~0.1 % on an 8 s ramp at 192 kHz).
- Mid-ramp reversal (FUNC-01): engage edge mid-ramp seeds `u0 = clamp(r0,0,1)^(1/p_new)` — speed-continuous, same voice.
- Stopped (FUNC-04): entered at `r < kStopEps = 0.001`; 10 ms linear fade landing on **exact 0.0f**; ring keeps recording while Stopped (CONTEXT decision 1 — debt clamp applied at SpinUp entry: `readAbsFrac = jmax(readAbsFrac, totalWritten − (ringSpan − kInterpGuard))`).
- Durations latched in samples at the gesture edge (Free ms only this phase).

### Task 6 — processBlock integration
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp` (modify)
- **Depends on:** Tasks 2–5
- Processing order per ARCHITECTURE: block header (read atomics, detect ENGAGE edge, transport update) → per sample: **write capture first, then read** → advance transport → voice read → stopped-fade → MIX blend (smoothed 20 ms) → OUTPUT_GAIN (SmoothedValue, dB→linear).
- **Bypassed = hard pass-through** (`out = in`, skip blend entirely; ring still written every sample in every state).
- `prepareToPlay`: ring `prepare(fs, kCaptureSeconds)` (the only allocation), reset voices/transport→Bypassed/smoothers; restored ENGAGE=on at prepare treated as an edge. `ScopedNoDenormals` in processBlock. Zero latency — call nothing.
- Wire STOP_FREE_MS / START_FREE_MS / STOP_CURVE / START_CURVE / MIX / OUTPUT_GAIN.

### Task 7 — Phase 2.1 probes + gate
- **Files:** `tests/render-harness/main.cpp` (extend)
- **Depends on:** Tasks 1, 6
- **P0 (determinism):** same gesture twice on fresh instances → bitwise identical (no RNG, no wall-clock — must hold exactly).
- **P1 (QUAL-01):** 512-vs-4096 bit-identity, full gesture cycle (engage → hold → release), **edges scheduled only at multiples of 4096**; ragged-block variant {1, 7, 64, 333, 4096} for edge-free steady spans only.
- **Bypass null:** disengaged output bitwise dry (memcmp vs input).
- **P3 (DSP-02):** sine input, autocorrelation pitch trace (copy `autocorrPitchHz()` from O-simpleGrain); window ≤ 1024 @ 48 kHz, hop 256, τ search constrained ±20 % around expected f; at curve 50 % assert f(t)/f0 tracks (1−u)² within tolerance at several u points; at 0 %/100 % assert trajectories differ from x² in the expected directions.
- **P6 (DSP-01):** engage/release sweeps at stop times {50 ms, 500 ms, 8 s} × curves {0, 50, 100} %; metric: max per-sample first difference of wet, normalised against the dry input's own first-difference bound at the lowest instantaneous ratio; liveness gate (wet ≠ dry during the ramp; `getMagnitude > 1e-4`).
- Mid-ramp reversal probe: engage → release mid-spin-down → assert no ratio discontinuity in the pitch trace (FUNC-01 partial).
- Slow-speed quality spot-check (risk register): sine sweep at r ≈ 0.02 rendered and eyeballed for stair-stepping; upgrade path is 6-point Lagrange — do not pre-build.
- **Gate:** `ninja OuariconTapestop_VST3 OuariconTapestop_AU O-Tapestop-render-test` clean; harness exit 0; **git commit** `phase: O-Tapestop 2.1 core varispeed + stop/start`.

---

## Phase 2.2 — Resync + Tempo Sync

**Goal:** DSP-03 complete (fall-behind → 1.25× catchup ≤ 250 ms → 50 ms crossfade-skip, bitwise post-resync null); FUNC-03 complete; retrigger honored in every state.

### Task 8 — Resync controller + voice B crossfade
- **Files:** `Source/dsp/TapestopTransport.h`, `Source/PluginProcessor.cpp` (modify)
- **Depends on:** Task 7 (gated 2.1)
- Add `Catchup` (`r = kCatchupRatio = 1.25`, ≤ `kMaxCatchupMs = 250`) and `ResyncXfade` states: spawn voice B at the live head (d = 0, r = 1, **integer-offset read**), 50 ms equal-power Hann crossfade A→B (Task 3 gains), discard A → Bypassed. Post-crossfade output = integer live read = bitwise dry by construction.
- Both skip-splice gain laws compiled: equal-power (Hann halves) and linear (`fadeIn = φ, fadeOut = 1−φ`), selected by a harness/dev flag for the A/B (Task 10).

### Task 9 — Retrigger-everywhere (CONTEXT decision 2)
- **Files:** `Source/dsp/TapestopTransport.h` (modify)
- **Depends on:** Task 8
- State machine drives **the carrier voice**, not "voice A". Per RESEARCH §3.2, two voices suffice in every state:
  - SpinDown/SpinUp retrigger: mid-ramp reversal, same voice (already in 2.1).
  - Catchup retrigger: spawn B at A's current position, new ramp from r matched to 1.0; 50 ms crossfade A→B.
  - ResyncXfade retrigger: **drive the new gesture on B** (B is at r = 1, exactly where every new ramp starts); A's fade-out completes as scheduled; the crossfade engine never restarts mid-fade.
  - Stopped retrigger: frozen voice re-seeds via the reversal rule.
- No third voice, no steal policy — spawn REFUSES on exhaustion per ReverseGrain contract (should be unreachable; jassert).

### Task 10 — Tempo sync
- **Files:** `Source/PluginProcessor.cpp` (modify)
- **Depends on:** Task 8
- Port O-Polystutter fallbacks only: no playhead/offline → 120 BPM, missing `getBpm()` → 120, `jlimit(20.0, 999.0, bpm)`. Read BPM into a member per block; the ONLY consumer is the gesture-edge conversion `durationSamples = beats · (60/bpm) · fs`. No PPQ/tick math anywhere.
- Division table {1/16, 1/8, 1/4, 1/2, 1, 2, 4 bars} → beats {0.25, 0.5, 1, 2, 4, 8, 16} (assumes 4/4 — note in NOTES).
- Wire SYNC_MODE routing + STOP_SYNC_DIV / START_SYNC_DIV; SYNC_MODE gates which of each pair the edge-latch reads (mid-gesture flips inert).

### Task 11 — Phase 2.2 probes + A/B + gate
- **Files:** `tests/render-harness/main.cpp` (extend)
- **Depends on:** Tasks 8–10
- **P2 (DSP-03):** dry reference vs gesture pass on fresh instances, same `noiseAt` input; null window starts **one full crossfade (50 ms) after Catchup ends**; assert bitwise memcmp from there. This proves the integer-offset live read + per-sample write-then-read chain end to end.
- **10 Hz toggling stress:** ENGAGE edges every 50 ms against 50 ms fades (exercises the ResyncXfade-retrigger path); assert click-free (P6 metric) and NaN-free.
- **Sync timing:** divisions track a host-tempo change (next gesture uses new BPM; live ramp does NOT retarget — latch contract); Free times match expected sample counts within one block.
- **P1 rerun** with sync active (block-size invariance holds).
- **Skip-splice A/B (CONTEXT open question):** render both gain laws over sustained pad-like material (low-passed noise + sine bed), compare splice-region level bump (equal-power over-sums correlated material) and audibility; **record the decision + evidence in NOTES.md**; fallback if both are judged too abrupt: repeated small skips (documented, not built).
- **Gate:** builds clean; harness exit 0; **git commit** `phase: O-Tapestop 2.2 resync + tempo sync`.

---

## Phase 2.3 — Scratch Mode + toneTrack

**Goal:** FUNC-02, DSP-04, DSP-05 complete; full DSP contract met.

### Task 12 — ScratchEnvelope (bake + persistence)
- **Files:** `Source/dsp/ScratchEnvelope.h/.cpp` (new), `Source/PluginProcessor.cpp` (modify)
- **Depends on:** Task 11 (gated 2.2)
- Port Path C `getValueAt()` (per-segment `t' = pow(t, 1 + curve·2)` / mirrored) into a **message-thread** bake: evaluate at 2048 uniform φ, y bipolar, `r = 2y`.
- Sanitize on parse: clamp x/y/curve ranges, sort by x, pin endpoints x = 0/1, clamp count 2–64, reject-to-default on any JSON type mismatch; `"v":1` version field from day one.
- Handoff: two pre-allocated `std::array<float, 2048>` + `std::atomic<const ScratchLut*>`; message thread writes ONLY the unpublished buffer, `store(release)`; audio thread `load(acquire)` **once at the engage edge**. Default gentle wobble `[{0,.5},{.25,.65},{.5,.35},{.75,.6},{1,.5}]` baked in the constructor — pointer never null.
- Persistence: JSON string property `scratchEnvelopeJson` on the APVTS tree in `get/setStateInformation`; missing/invalid → default, no error. Bake triggers: `setStateInformation`, preset load (editor commit lands in Stage 3).

### Task 13 — Scratch playback
- **Files:** `Source/dsp/TapestopTransport.h`, `Source/PluginProcessor.cpp` (modify)
- **Depends on:** Task 12
- `ScratchPass` state: `φ = samplesElapsed / envLengthSamples` (latched at edge), `r = lut[φ·2047]` linear-interp, r ∈ [−2, +2]; reverse via sign (position stays continuous — palindrome corner, not a click); φ = 1 → ResyncXfade; disengage mid-pass → abort straight to ResyncXfade; scratch re-engage mid-resync starts the new pass on the carrier voice immediately (r slope change, not position jump).
- Wire MODE routing + ENV_SYNC_DIV / ENV_FREE_MS. Disengaged mode switch touches nothing (Bypassed path).
- Aliasing at |r| > 1 accepted as character (CONTEXT decision 3) — no anti-alias filter.

### Task 14 — toneTrack filter
- **Files:** `Source/PluginProcessor.cpp` (modify)
- **Depends on:** Task 13
- `FirstOrderTPTFilter<float>` lowpass, wet-path only, engaged-only; cutoff law `fc = 20000 · (150/20000)^(a·(1−min(|r|,1)))`; a = 0 pins fc = fMax with no conditional path; |r| > 1 clamps open.
- Update condition: `capture.getTotalWritten() % 16 == 0` checked **per sample** inside the engaged loop (absolute grid — never a per-block counter); one `pow` + `setCutoffFrequency` only when the update fires. `reset()` + first cutoff at the engage edge; `snapToZero()` per block while engaged. Cutoff follows the carrier voice's r.
- Wire TONE_TRACK.

### Task 15 — Phase 2.3 probes + final gate
- **Files:** `tests/render-harness/main.cpp` (extend)
- **Depends on:** Tasks 12–14
- **P4 (debt bound):** commit worst-case full-reverse envelope (all y = −1) via the same JSON path the UI will use, ENV_FREE_MS = 8000, engage; sample `getDebtSamplesForTest()` per block; assert `maxDebt ≤ ringSpan − kInterpGuard`, output correlated with old input (not garbage); **assert kCaptureSeconds parsed from source, never a harness literal**. Long-hold variant: engage → Stopped hold > 26 s → release → in-bounds reads + clean resync (specified clamp behavior, no assert).
- **P5 (QUAL-01):** silence, DC, full-scale impulse train, then clean sine; full gesture cycle on each; no NaN/Inf anywhere; output returns to **bitwise** dry after resync (no sticky transport/TPT state).
- **toneTrack probes (DSP-05):** spectral centroid falls with |r| during spin-down at a = 60; a = 0 sonically transparent (band-energy delta under threshold — need not be bitwise, wet path); no zipper on the cutoff glide (P6 metric over the sweep); **P1 rerun with toneTrack active** (16-sample absolute grid is the invariance exposure).
- **Scratch probes (FUNC-02/DSP-04):** drawn LUT plays once per engage (pitch trace follows the envelope); direction flips artifact-free (P6 metric across a y sign change); disengaged mode switch bitwise silent.
- **PERF-01 audit:** grep/review processBlock call tree for allocation/locks/JSON; confirm memory allocated in prepareToPlay only. CPU spot-check vs < 5 % target.
- **Full suite green** (P0–P6 + all phase probes) on fresh build.
- **Gate:** builds clean; harness exit 0; **git commit** `phase: O-Tapestop 2.3 scratch + toneTrack`.

---

## Files Summary

**New:**
- `tests/render-harness/CMakeLists.txt`, `tests/render-harness/main.cpp`
- `Source/dsp/CaptureBuffer.h` (trimmed port), `Source/dsp/WindowLut.h` (Hann-only), `Source/dsp/VarispeedVoice.h`, `Source/dsp/TapestopTransport.h`, `Source/dsp/ScratchEnvelope.h/.cpp`
- `stages/2-dsp/NOTES.md` (A/B decision + evidence, division-table 4/4 assumption)

**Modified:**
- `Source/PluginProcessor.h/.cpp` (DSP integration; APVTS/editor guard untouched from Stage 1)

**Not touched:** `CMakeLists.txt` (Stage-1 `OUARICON_BUILD_TESTS` hook already present), `Source/PluginEditor.*`.

---

## Success Criteria (Stage 2 complete)

- [ ] Disengaged output BITWISE dry; 512-vs-4096 bit-identity with 4096-aligned edges (QUAL-01)
- [ ] Ratio trace: curve 50 % matches x² within tolerance; 0/100 % audibly distinct (DSP-02)
- [ ] Discontinuity scan clean across stop times {50 ms, 500 ms, 8 s} × curves {0, 50, 100} % (DSP-01)
- [ ] Mid-ramp reversal speed-continuous; engage honored in EVERY state; 10 Hz toggling click/NaN-free (FUNC-01)
- [ ] Post-resync null vs dry bitwise from one crossfade after Catchup (DSP-03); splice A/B decision recorded
- [ ] Sync divisions track tempo; Free times within one block; latch contract holds (FUNC-03)
- [ ] Scratch LUT plays once per engage; reverse artifact-free; full-reverse 8 s debt in bounds; > 26 s hold resumes clean (FUNC-02, DSP-04)
- [ ] toneTrack darkens with falling |r|; a = 0 transparent; no zipper; invariance holds with it active (DSP-05)
- [ ] Pathological input → no NaN/Inf, no sticky state, bitwise recovery (QUAL-01)
- [ ] processBlock allocation/lock audit clean; < 5 % single core @ 48 kHz (PERF-01)
- [ ] Three phase commits; harness exit 0 at each gate
