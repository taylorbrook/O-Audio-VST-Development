# Stage 2: DSP — Research

**Date:** 2026-08-15
**Scope:** ARCHITECTURE.md is the binding contract and Stage-2 CONTEXT.md resolved the three open
DSP questions (debt clamp, retrigger-everywhere, aliasing-as-character). This document fills in the
implementation-level specifics: the exact substrate port map (what copies, what trims, what is new
code), the render-harness template with recipes for probes P1–P6, the concrete mechanics of the
three CONTEXT decisions, and the pitfall map. Nothing below contradicts either contract.

Sibling precedent: O-Bitrot's Stage-2 RESEARCH.md (same day, same repo state) already surveyed the
harness lineage and O-Polystutter's tempo-sync code; its verified line references are reused here
and were spot-re-checked where load-bearing.

---

## 1. Render harness template (Phase 2.1, Task 1)

No unit-test framework exists in this repo — the harness is a `juce_add_console_app` with a bare
`int main()` (`project_no_unit_test_framework_ci_never_runs_tests`). **Copy O-Octagon
`tests/render-harness/CMakeLists.txt` for CMake + bit-identity helpers, O-ReverseDelay
`tests/render-harness/main.cpp` for the effect render loop + baseline discipline.**

### CMake — Stage 1 already did half the work

O-Tapestop's `CMakeLists.txt:74-76` already carries the `OUARICON_BUILD_TESTS` option with the
`EXISTS` guard on `tests/render-harness/CMakeLists.txt` — the plan only adds the harness directory,
no plugin-CMake edit. `PluginProcessor.cpp:25` already has the `#if JUCE_WEB_BROWSER` editor guard
with the include inside the guard (`pattern_render_harness_breaks_on_webview_editor` — done in
Stage 1, nothing to retrofit).

Load-bearing points for the new `tests/render-harness/CMakeLists.txt`:

- `juce_add_console_app(O-Tapestop-render-test)`; compiles `Source/PluginProcessor.cpp` directly
  with `JUCE_WEB_BROWSER=0` — never list the editor TU.
- Target name for property borrowing is **`OuariconTapestop`** (`CMakeLists.txt:5`), not the folder
  name (`build_script_target_name_vs_folder`):
  `$<TARGET_PROPERTY:OuariconTapestop,INCLUDE_DIRECTORIES>` + `add_dependencies`.
- Version DERIVED via `get_target_property(... OuariconTapestop JUCE_VERSION)` + `FATAL_ERROR` if
  empty (`pattern_test_fixture_mirrors_drift_silently`; O-ReverseDelay `CMakeLists.txt:52-67`).
- Hand-define the `JucePlugin_*` macros + `JUCE_STANDALONE_APPLICATION=1`, `JUCE_USE_CURL=0`
  (O-Octagon `:112-126`).
- Invocation: `cmake -DOUARICON_BUILD_TESTS=ON` regen → `ninja O-Tapestop-render-test` → run,
  exit code 0/1 is the gate.
- A debt-inspection accessor for probe P4 needs a harness-only hook: gate it behind
  `OUARICON_RENDER_HARNESS=1` (O-ReverseDelay precedent, `CMakeLists.txt:117`) or expose a plain
  `getDebtSamplesForTest()` — decide in plan; the flag precedent exists.

### Probe conventions (non-negotiable)

- `check(name, ok, detail)` printing `[PASS]/[FAIL]` + the measured number and the bound
  (O-Octagon `main.cpp:249-257`); `return failures == 0 ? 0 : 1`.
- **Position-deterministic noise** `noiseAt(t) = hash(t)`, never a sequential RNG (O-ReverseDelay
  `main.cpp:1709-1752`) — a sequential generator makes 512-vs-4096 two different experiments.
  (O-Tapestop's engine has NO RNG at all, so this applies only to harness input synthesis.)
- **`setBaseline(apvts)` at the top of every probe** resetting all 14 params to shipped defaults
  (O-ReverseDelay `main.cpp:697-805` — harness state leaking forward masqueraded as DSP
  regressions for four releases). Traps here: MIX neutral = 100 (not 0), ENGAGE must be
  explicitly forced OFF and one block rendered so the transport actually returns to Bypassed
  before the next probe (a probe ending mid-SpinUp leaks transport state, not just param state).
- **Param writes via `param->setValueNotifyingHost(range.convertTo0to1(engineering))`** —
  synchronous, updates the cached atomics; bare `setValue` leaves atomics stale (O-Octagon
  `main.cpp:329-343`). ENGAGE edges are exactly this call made **between** processBlock calls.
- `bitIdentical()` = per-channel `std::memcmp`; `firstDifference()` reports `chN @M (a vs b)`
  (O-Octagon `main.cpp:424-449`).
- Liveness control on every probe that could be vacuous (`getMagnitude > 1e-4`;
  `pattern_zipper_sweep_probe_needs_liveness_gate`) — especially P3/P6, where a broken engage
  edge would leave pure dry passing "no clicks" trivially. No wall-clock inside any verdict
  (`pattern_wallclock_inside_a_stability_verdict`).

### Recipes for probes P1–P6 (ARCHITECTURE Notes → ROADMAP test criteria)

| Probe | Recipe |
|---|---|
| P1 — 512-vs-4096 bit-identity (QUAL-01) | O-Octagon probes AL/AM/AN shape (`main.cpp:1569-1685`): render same timeline at both sizes, memcmp. **ENGAGE/release edges scheduled only at multiples of 4096** (CONTEXT constraint — edges are block-header detected, so identical timelines require boundaries common to both partitions). Include a full gesture cycle: engage → stopped hold → release → resync complete. Ragged-block variant {1, 7, 64, 333, 4096} only for edge-free spans (steady engaged hold), since ragged partitions cannot share edge positions. |
| P2 — post-resync null vs dry (DSP-03) | Render dry reference (ENGAGE never touched) and gesture pass on fresh instances, same `noiseAt` input. Null window starts **one full crossfade (50 ms) after Catchup ends**; assert bitwise memcmp from there. This is the probe that proves the integer-offset live read + per-sample write-then-read chain end to end. |
| P3 — ratio trace, curve law (DSP-02) | Sine input; short-window autocorrelation pitch trace — copy `autocorrPitchHz()` from O-simpleGrain `main.cpp:144-168`. **Window ≪ ramp time** (`pattern_metric_window_vs_modulation_period`): at the 500 ms default stop, window ≤ 1024 @ 48 kHz, hop 256; constrain τ search ±20 % around expected f to avoid the half-period octave latch (O-Contrabass `main.cpp:4157-4171`). At curve = 50 %, assert f(t)/f0 tracks (1−u)² within tolerance at several u sample points; at 0 %/100 % assert measured trajectories differ from x² in the expected direction (audibly distinct, DSP-02). |
| P4 — reverse-envelope debt bound | Commit a worst-case full-reverse envelope (all y = −1) via the same JSON path the UI will use, ENV_FREE_MS = 8000, engage, run the pass; sample the debt accessor per block; assert `maxDebt ≤ ringSpan − kInterpGuard` and no wrap artifacts (output stays correlated with 24-s-old input, not garbage). Also assert the kCaptureSeconds derivation constant matches the parsed source constant, not a harness literal. |
| P5 — pathological input / sticky state (QUAL-01) | Silence, DC, full-scale impulse train, then clean sine; full gesture cycle on each; assert no NaN/Inf anywhere and that output returns to bitwise dry after resync (recovery proves no sticky transport/filter state; `pattern_envelope_follower_state_sticky_nan` applies to the TPT filter state). |
| P6 — discontinuity scan (DSP-01) | Engage/release sweeps at stop times {50 ms, 500 ms, 8 s}, curves {0, 50, 100} %. Metric: max per-sample first difference of the wet output, normalised against the max first difference the dry input itself produces at the lowest instantaneous ratio reached — a continuous-position engine adds no step beyond the source material's own. Flag any sample where the wet diff exceeds k× the local dry bound. Liveness: assert the gesture actually ran (wet ≠ dry during the ramp). |

Determinism bonus probe (cheap, catches regressions early): render the same gesture twice on fresh
instances → bitwise identical (no RNG, no wall-clock → must hold exactly).

---

## 2. Substrate port map — what copies, what trims, what is new

### 2.1 CaptureBuffer (O-ReverseDelay `Source/dsp/CaptureBuffer.h`, 182 lines) — port TRIMMED

Verified against source. Keep: `prepare(sr, maxSeconds)` (the only allocation), alloc-free
`clear()`, `pushSample`, `readAbs(ch, absIndex)` with the double-mod negative-tolerant wrap
(`:160-164`), `getTotalWritten`. **Drop:** `pushLooped`, `pushCrossfaded` (freeze machinery,
~80 lines of the file), `monoSum` (O-Tapestop is full-stereo varispeed — ARCHITECTURE forbids
mono-summing). Call `prepare(sampleRate, kCaptureSeconds)` with the derivation comment above the
constant. Pre-history reads return the cleared buffer's zeros — this makes the debt clamp's
worst case (playing the oldest valid material) safe even in the first 26 s after load.

One adaptation: `readAbs` per-sample via `buffer.getSample` is fine at 2 voices × 4 taps × 2 ch
(the profiler said so for O-ReverseDelay at 64 grains); no need for raw-pointer hoisting in v1.0.

### 2.2 WindowLut (O-ReverseDelay `Source/dsp/WindowLut.h`, 825 lines) — take ~40 lines only

The file has grown five shapes, tilt, taper α grids, and four normalisation laws — **all of it
exists to serve overlapping GRAIN populations and none of it applies to a 2-voice crossfade.**
Take exactly what O-ReverseDelay v1.0.0 itself took from O-simpleGrain: the Hann table build
(one line, `build()` at `:591`) + `readAt` (clamp, lookup, lerp — `:229-239`). Crossfade gains per
ARCHITECTURE: `fadeOut = hann(0.5 + φ/2)`, `fadeIn = hann(φ/2)` — both land on the table's
monotonic halves; equal-power by construction (sin²+cos²=1). No normalisation constants needed:
exactly two voices, deterministic gains, no population statistics.

The Phase 2.2 A/B (equal-power vs linear skip splice) is then one branch: linear is
`fadeIn = φ, fadeOut = 1−φ` — no table at all. Keep both paths compiled and A/B via a harness
flag, record the decision in NOTES per CONTEXT.

### 2.3 ReverseGrain (O-ReverseDelay `Source/dsp/ReverseGrain.h`) — CONTRACT only, not code

What transfers is the discipline, verified at source: POD voice struct (`:78`), `active` flag with
find-inactive spawn that REFUSES on exhaustion (`:193-215`), direction latched at spawn (`:138-144`),
everything the voice needs copied into it at spawn (latch-at-spawn). O-Tapestop's `VarispeedVoice`
is new code ~30 lines: `{ bool active; double readAbsFrac; float gain; }` — speed `r` lives in the
transport, not the voice, because exactly one voice is "driven" at a time (see §4.2).

### 2.4 O-Polystutter tempo sync (`PluginProcessor.cpp:1707-1740`) — port the fallbacks, not the plumbing

Verified at source. Reusable as-is: no-playhead/offline → 120 BPM fallback (`:1710-1714`),
`getBpm()` missing → 120, clamp `jlimit(20.0, 999.0, bpm)` (`:1727-1735`). JUCE 8 API confirmed:
`getPlayHead()->getPosition()` → `Optional<PositionInfo>`.

**What O-Tapestop does NOT need from it:** the per-block `updateTempo` fan-out and any boundary/PPQ
logic. Durations are edge-latched (ARCHITECTURE latch contract) — the processor reads BPM into a
member each block (cheap), and the ONLY consumer is the gesture-edge conversion
`durationSamples = beats · (60/bpm) · fs`. No sample-accurate tick math exists in this plugin at
all (unlike O-Bitrot): gestures start at ENGAGE edges, not at beat boundaries. This kills the
whole class of block-granular-trigger invariance bugs by construction. Division table
{1/16, 1/8, 1/4, 1/2, 1, 2, 4 bars} → beats {0.25, 0.5, 1, 2, 4, 8, 16} (assumes 4/4, suite
precedent; note in NOTES).

### 2.5 Path C reference (`research/stutter-effects/path-c-playhead-modulator.md` §2.1) — bake source

Verified: the `Point {x, y, curve}` model and `getValueAt()` (`:313-368`) with the per-segment
curve law `t' = pow(t, 1 + curve·2)` for curve > 0, mirrored `1 − pow(1−t, 1 − curve·2)` for
curve < 0. Port `getValueAt` into the message-thread bake (evaluate at 2048 uniform φ, y made
bipolar, `r = 2y`), NOT onto the audio thread. Sanitize on parse: clamp x/y/curve ranges, sort by
x, pin endpoints x=0/1, clamp count 2–64, reject-to-default on any JSON type mismatch. `"v":1`
version field from day one.

---

## 3. New code specifics

### 3.1 Catmull-Rom kernel — hand-rolled, Horner form

`juce::Interpolators::CatmullRom` (local 8.0.14, `juce_Interpolators.h:160`) is a
`GenericInterpolator` **stream resampler** — `process()` over consecutive input, own state, one
instance per channel. Wrong shape for random-access ring reads at a per-sample-varying rate.
Hand-roll the standard kernel over `readAbs(ch, i−1 … i+2)`:

```
f2 = f*f;  a0 = -0.5f*y0 + 1.5f*y1 - 1.5f*y2 + 0.5f*y3;
a1 = y0 - 2.5f*y1 + 2.0f*y2 - 0.5f*y3;  a2 = 0.5f*(y2 - y0);
out = ((a0*f + a1)*f + a2)*f + y1;
```

At `f = 0` this returns **bitwise `y1`** (every term multiplies by 0.0f) — but do not rely on that
for the null test: the live-riding / post-resync path reads the **integer index directly**
(ARCHITECTURE: `d == 0, r == 1` → integer read), which is both bitwise-exact by construction and
avoids the i+1/i+2 taps landing on not-yet-written future samples. The Catmull-Rom path is only
ever taken with debt `d ≥ kInterpGuard = 4`, which keeps all four taps behind the write head.

Slow-speed quality (risk register): probe with a sine sweep at r = 0.02; if stair-stepping is
audible, the upgrade path is 6-point Lagrange — do not pre-build it.

### 3.2 Transport / voice topology — the retrigger-everywhere mechanics (CONTEXT decision 2)

Finding: **2 voices suffice for retrigger in every state — no third voice, no steal policy.** The
key is that a new gesture never needs a new voice; it needs a voice *at the right position*, and
one of the two live voices is always already there:

- **Retrigger during SpinDown/SpinUp** (1 voice live): mid-ramp reversal per ARCHITECTURE —
  same voice, new ramp seeded `u0 = clamp(r0,0,1)^(1/p_new)`. Speed-continuous, no crossfade.
- **Retrigger during Catchup** (1 voice live at r = 1.25): spawn voice B **at A's current
  position** with the new gesture's ramp starting from r matched to 1.0 (u0 = 0 for spin-down);
  50 ms crossfade A→B per CONTEXT. (A at 1.25× and B at ≤1× from the same origin diverge slowly —
  50 ms of ≤0.25× rate difference = ≤12 ms of content skew across the fade, benign.)
- **Retrigger during ResyncXfade** (2 voices live: A fading out at old position, B fading in at
  the live head, r = 1): **drive the new gesture on B.** B's spin-down starts at u0 = 0 exactly
  where r = 1 already is — speed-continuous on B; A's fade-out completes as scheduled. No third
  voice ever needed; the crossfade engine never restarts mid-fade. Scratch re-engage mid-resync:
  same — the new pass's LUT starts driving B from its current position (r jumps from 1 to
  lut[0] — a slope change, not a position jump; legal per the staircase-envelope precedent).
- **Retrigger during Stopped**: the frozen voice re-seeds per the mid-ramp reversal rule (u0 from
  r0 ≈ 0 → restarts the ramp).

Consequence for the state machine: `r` is a property of the *driven* voice; the resync controller
drives whichever voice is currently the gesture carrier. The 10 Hz toggling stress test
(Phase 2.2 criterion) exercises exactly the ResyncXfade-retrigger path — edges every 50 ms against
50 ms fades — and the B-carries-the-gesture rule keeps it click-free with the fixed 2-voice pool
ARCHITECTURE specifies.

### 3.3 Stopped-hold debt clamp (CONTEXT decision 1)

Mechanics: while Stopped, the ring keeps writing (+1 debt/sample) and the voice is silent (10 ms
fade landed on exact 0.0f). On SpinUp entry, clamp once:
`readAbsFrac = jmax(readAbsFrac, double(totalWritten) − (ringSpan − kInterpGuard))`. Additionally
clamp inside the voice read as the release-build backstop (ARCHITECTURE's `[kInterpGuard,
ringSpan − kInterpGuard]` debt clamp), with the debug `jassert` firing only outside Stopped-resume
paths — a hold-forever session must not assert on resume, that clamp is *specified* behavior now.
The clamped resume plays the oldest valid material; crossfade-skip resync absorbs the jump
(CONTEXT rationale). Probe P4's long-hold variant: engage → hold Stopped > 26 s → release →
assert in-bounds reads + clean resync.

### 3.4 toneTrack — absolute-grid updates

`FirstOrderTPTFilter<float>` verified (setCutoffFrequency `:80`, `snapToZero()` `:141`). Update
condition is `capture.getTotalWritten() % 16 == 0` checked per sample inside the engaged loop —
NOT a per-block counter (block starts differ between 512/4096 partitions; `totalWritten` doesn't).
Cutoff law per ARCHITECTURE; compute the `pow` only when the update fires (one pow + one tan per
16 samples). `reset()` + first cutoff at the engage edge (starts wide open). `snapToZero()` per
block while engaged. a = 0 pins fc = fMax with no conditional path.

### 3.5 Curve evaluation

One `pow` per sample is within budget (ARCHITECTURE allows it; ~15 ns for a slowly-varying base).
Recommendation to plan: per-sample `pow` in Phase 2.1 for exactness of the P3 probe; the
16-sample-eval + lerp optimisation only if the CPU probe demands it (it won't at 2 voices).
Track ramp phase `u` in **double** (`u += 1.0/durationSamples`; an 8 s ramp at 192 kHz is 1.5 M
steps — float accumulation drifts ~0.1 % there, audible as a duration error and a bit-identity
hazard between block sizes if intermediate rounding differs; double makes accumulation exact to
the probe's needs and costs nothing).

### 3.6 Scratch LUT handoff — double-buffer discipline

Two pre-allocated `std::array<float, 2048>` members + `std::atomic<const ScratchLut*>`; message
thread writes ONLY the unpublished buffer, then `store(release)`; audio thread `load(acquire)`
**once at the engage edge** into the transport's latched pointer. Because the audio thread never
re-reads the atomic mid-pass, the message thread can safely alternate buffers as fast as edits
commit (a second commit before the next engage just overwrites the inactive buffer again). Bake
triggers: `setStateInformation`, editor commit (Stage 3), preset load. Default gentle-wobble baked
in the constructor so the pointer is never null.

---

## 4. Pitfall map (repo memory → where it bites in Stage 2)

| Pattern | Bites in | Discipline |
|---|---|---|
| `pattern_grain_read_before_capture_write_blocksize` | 2.1 | Per-sample write-then-read; the integer live-read at d=0 is only legal with this order |
| `pattern_metric_window_vs_modulation_period` | 2.1 P3 | Pitch window ≤ 1024 @ 48 kHz vs 500 ms ramps; ±20 % τ search |
| `pattern_zipper_sweep_probe_needs_liveness_gate` | all probes | Assert the gesture ran before asserting its cleanliness |
| `pattern_wallclock_inside_a_stability_verdict` | all probes | Fixed sample counts everywhere |
| Harness param leak (`setBaseline`) | all probes | Reset 14 params + force ENGAGE off + settle one block (transport state leaks too) |
| `pattern_test_fixture_mirrors_drift_silently` | harness | Version + kCaptureSeconds derived/parsed, never harness literals |
| `pattern_envelope_follower_state_sticky_nan` | 2.3 P5 | TPT state recovery probe after pathological input |
| `pattern_block_rate_envelope_breaks_blocksize_invariance` | 2.3 | toneTrack updates on the absolute 16-sample grid (§3.4), not block starts |
| `critical_delayline_push_without_pop_shifts_delay` | 2.1 | Not using DelayLine at all — CaptureBuffer push/readAbs; the pattern's lesson (paired ops) is moot by design |
| `pattern_stale_host_instance_vs_offline_repro` | DAW checks | Quit/reopen Logic before chasing "host-only" bugs |
| `pattern_rng_stream_interleave_blocksize` | — | No RNG in this engine; harness input uses positional `noiseAt` |
| JUCE 8 `getLatencySamples` non-virtual | — | Zero latency; call nothing |
| `pattern_offline_dsp_render_harness` | gates | Harness is the Stage-2 correctness gate, Phase 2.1 Task 1 |

---

## 5. Key decisions handed to the plan phase

1. **Harness scaffolding is Phase 2.1 Task 1** — CMake target (`OuariconTapestop` property
   borrowing, derived version) + `check`/`noiseAt`/`bitIdentical`/`setBaseline` helpers + P1/P6
   skeletons. The Stage-1 CMake hook and editor guard already exist; only `tests/render-harness/`
   is new.
2. **CaptureBuffer ports trimmed** (drop freeze paths + monoSum); **WindowLut shrinks to
   Hann-table + readAt** (~40 lines) — two-voice crossfades need no normalisation machinery.
3. **VarispeedVoice is new ~30-line POD** (double `readAbsFrac`, latch-at-spawn); Catmull-Rom
   hand-rolled in Horner form; integer-read fast path at d=0/r=1 is what carries the null test —
   never route the live-riding read through the interpolator.
4. **Retrigger-everywhere costs no third voice**: the gesture carrier moves to voice B during
   ResyncXfade (B is at r=1, exactly where every new ramp starts); mid-ramp reversal covers the
   single-voice states (§3.2). State machine drives "the carrier voice", not "voice A".
5. **Debt clamp at SpinUp entry + per-read release-build clamp**; the >26 s Stopped hold is
   specified behavior, not an assert (§3.3); P4 gets a long-hold variant.
6. **Ramp phase `u` and `readAbsFrac` in double**; per-sample `pow` in 2.1 (optimise only on
   measured need).
7. **Tempo sync is fallback/clamp only** (verified `:1707-1740`) — read BPM per block, convert at
   gesture edges; no tick math, no per-block retargeting.
8. **Skip-splice A/B**: both gain laws compiled behind a harness flag; decision recorded in
   Phase 2.2 per CONTEXT (equal-power = Hann-half reads; linear = φ/1−φ, no table).
9. **Debt accessor for P4** behind `OUARICON_RENDER_HARNESS=1` (O-ReverseDelay `:117` precedent).

## Sources

- Repo (verified this session): O-Tapestop `CMakeLists.txt:5,58-76`, `PluginProcessor.cpp:23-75,236-247`;
  O-ReverseDelay `CaptureBuffer.h` (full read), `WindowLut.h` (full read), `ReverseGrain.h:78-229`;
  O-Polystutter `PluginProcessor.cpp:1707-1740`; `research/stutter-effects/path-c-playhead-modulator.md:304-395`;
  JUCE 8.0.14 local: `juce_Interpolators.h:113,160,217`, `juce_FirstOrderTPTFilter.h:80,129,141`.
- Sibling survey (same-day, line refs carried): O-Bitrot `.planning/stages/2-dsp/RESEARCH.md` §1
  (O-Octagon/O-ReverseDelay harness helper line numbers).
- Contracts: `research/ARCHITECTURE.md` (binding), `stages/2-dsp/CONTEXT.md` (three decisions),
  ROADMAP.md Stage-2 phase breakdown.
