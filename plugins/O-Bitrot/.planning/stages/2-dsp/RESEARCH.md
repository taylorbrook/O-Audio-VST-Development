# Stage 2: DSP — Research

**Date:** 2026-08-15
**Scope:** The three open questions from CONTEXT.md (libgsm CMake integration, latency delay-trim
bookkeeping, suite reuse points) plus the render-harness template and pitfall map for the plan phase.
ARCHITECTURE.md remains the binding contract; nothing below contradicts it — this fills in the
implementation-level specifics it left open.

---

## 1. Render harness template (Phase 2.1, day one)

**No unit-test framework exists in this repo — the harness is a `juce_add_console_app` with a bare
`int main()`** (confirmed: `plugins/O-Octagon/tests/unit/CMakeLists.txt:4-7` states Catch2 in
TESTING.md was never implemented). Lineage: O-simpleGrain → O-ReverseDelay → O-Octagon. **Copy
O-Octagon for CMake + bit-identity helpers, O-ReverseDelay for the effect render loop + baseline
discipline.**

### CMake (`plugins/O-Bitrot/tests/render-harness/CMakeLists.txt`)

Template: `plugins/O-Octagon/tests/render-harness/CMakeLists.txt` (newest, 2026-08-14). Load-bearing
points:

- `juce_add_console_app(O-Bitrot-render-test)`; **compiles `Source/PluginProcessor.cpp` directly**
  ("share the engine, not a static lib"). Never list the editor TU.
- `JUCE_WEB_BROWSER=0` in the harness defines, and `createEditor()` restructured with the guard
  (include `PluginEditor.h` **inside** the guard, not top-of-file — O-ReverseDelay
  `PluginProcessor.cpp:2389-2400` shape; `pattern_render_harness_breaks_on_webview_editor`).
  Adopt the guard in Phase 2.1 even though Stage 1's editor is plain — Stage 3 swaps in WebView and
  would otherwise break the harness.
- Includes borrowed from the plugin target:
  `target_include_directories(... $<TARGET_PROPERTY:OBitrot,INCLUDE_DIRECTORIES>)` +
  `add_dependencies(... OBitrot)`. Target name is `OBitrot` (juce_add_plugin target, not folder
  name — `build_script_target_name_vs_folder`).
- **Version DERIVED, never mirrored:** `get_target_property(... OBitrot JUCE_VERSION)` +
  `FATAL_ERROR` if empty (O-ReverseDelay `CMakeLists.txt:52-67`; the literal drifted twice across
  five releases there — `pattern_test_fixture_mirrors_drift_silently`).
- Hand-define the `JucePlugin_*` macros the processor references + `JUCE_STANDALONE_APPLICATION=1`,
  `JUCE_USE_CURL=0` (O-Octagon `:112-126`).
- Gate: `option(OUARICON_BUILD_TESTS ... OFF)` → `add_subdirectory(tests/render-harness)` in
  O-Bitrot's CMakeLists. Invocation: `cmake -DOUARICON_BUILD_TESTS=ON` regen, `ninja
  O-Bitrot-render-test`, run binary, exit code 0/1 is the contract.
- Harness-only seed pinning flag if needed: `OUARICON_RENDER_HARNESS=1` precedent (O-ReverseDelay
  `:117`) — likely unnecessary here since SEED is a real parameter.

### Probe conventions (non-negotiable, all have shipped-bug war stories)

- `check(name, ok, detail)` helper printing `[PASS]/[FAIL]` + **the measured number and the bound**
  (O-Octagon `main.cpp:249-257`); count failures; `return failures == 0 ? 0 : 1`.
- **Position-deterministic noise, never a sequential RNG** for input signals:
  `noiseAt(t) = hash(t)` (O-ReverseDelay `main.cpp:1709-1752` `noiseAt`; rationale restated at
  O-Octagon `main.cpp:311-327`). A sequential generator makes 512-vs-4096 compare two different
  experiments.
- **`setBaseline(apvts)` at the top of every probe** resetting ALL 31 params to shipped defaults
  (O-ReverseDelay `main.cpp:697-805` — four releases of "DSP regressions" that were harness state
  leaking forward). Traps: the neutral value is often not the range minimum (CRUSH_BITS neutral =
  16, CRUSH_RATE neutral = 20 kHz, MIX neutral = 100); latching params (the six `*_ENABLE`s,
  HARD_EDGES) matter most.
- **Param writes via `setValueNotifyingHost(convertTo0to1(engineering))`** — fully synchronous,
  updates the cached atomics; bare `setValue` leaves atomics stale and the harness "tests" defaults
  (O-Octagon `main.cpp:329-343`). Read-back via the raw atomic so derived quantities use the
  processor's exact skew.
- `bitIdentical()` = per-channel `std::memcmp`; `firstDifference()` reports `chN @M (a vs b)`
  (O-Octagon `main.cpp:424-449`).
- Every probe that could be vacuous carries a **liveness control** (`getMagnitude > 1e-4`)
  (`pattern_zipper_sweep_probe_needs_liveness_gate`). No wall-clock inside any verdict
  (`pattern_wallclock_inside_a_stability_verdict`).

### Probe recipes for the ROADMAP criteria

| Criterion | Recipe (template ref) |
|---|---|
| 512-vs-4096 bit-identity (QUAL-02) | O-Octagon probes AL/AM/AN (`main.cpp:1569-1685`): memcmp, plus ragged `{1,7,64,333,4096}` variant and events at non-aligned offsets. Run with families enabled + fixed seed. |
| Seeded determinism (FUNC-04) | O-ReverseDelay probe T (`main.cpp:2039-2063`): render twice **on fresh processor instances**, `maxAbsDiff == 0.0` + liveness. Add: different SEED ⇒ outputs differ. |
| Bit-transparent minus latency (FUNC-02) | No existing delay-compensated null probe in the suite — write the missing one, and position-deterministic input makes it trivial: all-off, assert `out[n]` bit-equals `noiseAt(n − kCompLatency)` after the first block (O-Octagon Q′ shape, `main.cpp:1010-1048`, but bit-exact — the all-off path is a pure integer delay, see §3). |
| Tape instantaneous-frequency ramps (DSP-01) | Sine input; short-window autocorrelation pitch trace over time — copy `autocorrPitchHz()` from O-simpleGrain `main.cpp:144-168` (normalised autocorr, minCorr 0.3). **Window ≪ ramp time**: TAPE_RAMP default 150 ms → window ≤ 1024 @ 48 kHz, hop 256 (O-Contrabass learned this at `main.cpp:4142-4153`: a window spanning 58 % of the modulation period read 7.4¢ for a true 12¢ — `pattern_metric_window_vs_modulation_period`). Constrain τ search ±20 % around expected f to avoid the half-period octave latch (`:4157-4171`). |
| Vinyl no-pitch-change (DSP-03) | Same `autocorrPitchHz` before/after jumps; assert jump distances are integer multiples of the revolution quantum by locating discontinuities in a ramp-marker input. |
| GE burst statistics (DSP-04) | Count consecutive-lost-packet runs over a long render (≥ 60 s equivalent); compare run-length histogram to geometric with p = P(B→G); chi-square-lite (ratio bounds), fixed seed. |
| GSM/μ-law alignment (DSP-05) | Cross-correlate codec path against the plain-delay path; assert peak lag within ±fs/8000 samples (one 8 kHz grid period — see §3). |
| Sticky NaN (QUAL-01) | Pathological inputs (DC, full-scale square, NaN injection) then clean input; assert recovery (`pattern_envelope_follower_state_sticky_nan`). |

Runner script: none needed for v1.0 (Octagon/ReverseDelay are "build, run, exit code"). If WAV
goldens are ever wanted, copy O-Contrabass `reproduce-goldens.sh` (bash-3.2-safe, sha256 as
truth-bar; note `pattern_golden_tracked_as_checksum_only`).

---

## 2. Suite reuse points — what ports, what must change

### 2.1 CaptureBuffer (O-ReverseDelay `Source/dsp/CaptureBuffer.h`, header-only, 182 lines)

Port the ring wholesale; it is channel-agnostic (`readAbs(ch, absIndex)`) so per-channel read
cursors need no structural change. Key idioms to keep:

- Absolute-index scheme: write side advances `writePosition` mod size + `++totalWritten`; read side
  double-mod wrap tolerant of negative indices (`:160-164`):
  `idx = ((absIndex % bufferSize) + bufferSize) % bufferSize` — pre-history reads hit cleared
  buffer → zeros.
- `prepare(sr, maxSeconds)` is the only allocation; `clear()` is alloc-free (deliberate — documented
  WR fix).

**Required deltas for O-Bitrot (the ring has NO fractional read — v1.0 grains read at exactly ±1):**

1. **Fractional read:** read heads hold `double readPosAbs`; `i0 = floor(pos)`, `frac = pos − i0`,
   lerp `readAbs(ch,i0)`/`readAbs(ch,i0+1)`. The `i0+1` neighbour moves *toward* the write head —
   keep a ≥ 1-sample guard band below `writeAbs`.
2. **Runtime lag clamp, not a static one:** O-ReverseDelay's `static_assert(gD_max + 2·G_max)`
   bound doesn't apply to a head that can fall behind indefinitely (0.5× bends, tape stop). Keep
   the compile-time span assert (`kRingSeconds = 2.5 ≥ 1.8 + 0.5 + 0.1`) **and** clamp
   `writeAbs − readPosAbs` into `[minLag, ringSpan − margin]` inside `clampAndScheduleJump()` every
   time a position or rate target changes.
3. Write-then-read per sample (already in ARCHITECTURE) means NORMAL-state lag can be 0 host
   samples — the just-written sample is readable.

### 2.2 Varispeed read + crossfades (O-Polystutter `Source/DSP/RepeatLane.cpp`; actual lines
~198-232 / ~309-318, offset from the audit doc's 183-217 / 284-297)

- The varispeed loop **reads a frozen snapshot with rate applied at read time**
  (`readOffset = position · pitchRatio`, position advances +1.0/sample). O-Bitrot reads the **live
  ring** with `readPosAbs += rate` — take the 2-point linear interp + neighbour clamp idiom
  (`:217-222`), not the snapshot/offset structure.
- Crossfade law: **linear equal-gain everywhere, 5 ms (`sampleRate · 0.005`), computed once in
  `prepare()`** (`getCrossfadeGain` `:501-511`). Matches ARCHITECTURE's 1–5 ms spec.
- ⚠ **The retrigger crossfade is a DC-held blend, not a true tail:** it fades from a *single held
  sample value* (`lastOutputLeft/Right`) into the new signal (`:309-318`). Removes the click but
  freezes the old material. **O-Bitrot's jump crossfades need a genuine two-head fade** — on
  `clampAndScheduleJump()`, the old head keeps reading at its current position/rate for the fade
  length while the new head ramps in; blend two live signals. (One extra fractional read per channel
  during fades — negligible.)
- Worth copying: run any post-filter every sample including silent stretches so IIR state stays
  continuous (`:284-288`); reset ALL fade state in `reset()`.

### 2.3 updateBeatSync (O-Polystutter `PluginProcessor.cpp:1707-1840`) — **port the edges, NOT the granularity**

Reusable as-is:
- Playhead acquisition + no-playhead/offline fallback (`:1710-1722`): no `posInfo` ⇒ BPM 120
  fallback behavior (O-Bitrot: fall back to free-run per ARCHITECTURE).
- BPM clamp `jlimit(20.0, 999.0, bpm)`, default 120 (`:1727-1735`); `getIsPlaying()` is a plain
  bool in JUCE 8, not Optional.
- `wasPlaying` guard so the first block after transport start doesn't fire; commit
  `lastPPQPosition/wasPlaying` unconditionally at block end — including on early-outs (mode-bypass
  branch `:1757-1765` shows why: stale edges burst on mode exit).

**⚠ O-Polystutter's boundary detection is BLOCK-GRANULAR** — `trigger()` takes no sample offset;
jitter is host-block-size dependent (would fail QUAL-02 outright). The sample-accurate tick logic is
**new code**:

```
ppqPerSample = bpm / (60.0 · fs)
ppqEnd = ppqStart + numSamples · ppqPerSample
for each k·subdivPPQ in (ppqStart, ppqEnd]:
    offset = clamp(int((k·subdivPPQ − ppqStart) / ppqPerSample), 0, numSamples−1)
```

- Use `std::floor`, **not `static_cast<int>`** — int-cast truncates toward zero, so negative PPQ
  (count-in/pre-roll) breaks the edge test around 0 (confirmed latent bug shape in O-Polystutter).
- Then split the block at tick offsets and render sub-blocks — O-ReverseDelay's pass-splitting
  (`PluginProcessor.cpp:1737`, `passLen = jmax(1, jmin(numSamples, passBound))`) is the model for
  block-size-invariant sub-block rendering.
- Free mode: sample-counting phase accumulator; same split-block application.
- Subdivision table: O-Polystutter's PPQ map (`:1774-1786`) extends to the 7 divisions in
  parameter-spec (1/16=0.25, 1/8T=1/3, 1/8=0.5, 1/4T=2/3, 1/4=1.0, 1/2=2.0, 1 bar=4.0 — 1 bar
  assumes 4/4; use `getTimeSignature()` if available, else 4/4 fallback, matching suite precedent).

---

## 3. Latency scheme — exact delay-trim bookkeeping (the "off-by-frames" answer)

**Recommendation: the alignment delay lives entirely inside CodecStage; everything else in the wet
path is zero-latency.**

- `kCompLatency = (int) std::ceil(0.020 · fs)`. Note `0.020·fs` is an **exact integer at every
  standard rate** (44.1k→882, 48k→960, 88.2k→1764, 96k→1920, 176.4k→3528, 192k→3840) — ceil is a
  no-op there and only matters at pathological rates.
- **CodecStage always presents exactly `kCompLatency` samples of delay, in all three of its
  states:**
  - *Disabled* and *μ-law*: route through a plain integer delay line of `kCompLatency` samples
    (μ-law round trip itself is zero-latency).
  - *GSM*: the latch → 160-frame accumulate → encode+decode → hold-out chain **replaces** the delay
    line. Its structural delay is exactly 160 grid periods = 0.020 s = `0.020·fs` host samples (see
    frame bookkeeping below).
- Read head NORMAL state: `readPosAbs = writeAbs` at rate 1.0 (write-then-read makes lag 0 legal).
  MediaPlayer, Packet, Crush, Quant: all zero structural latency.
- Report once in `prepareToPlay()`: `setLatencySamples(kCompLatency)`;
  `dryWet.setWetLatency((float) kCompLatency)`. Never renegotiate on CODEC_MODE.
- **FUNC-02 becomes trivially bit-exact:** with all families off and codec disabled, the only
  element in the wet path is the ring at rate 1.0 + the plain integer delay line ⇒
  `out[n] == in[n − kCompLatency]` bitwise. No tolerance needed.

**GSM frame bookkeeping (the actual off-by-frames trap):**

- RSBrokenMedia's proven pattern (patterns-only reference): accumulate latch outputs into a
  160-slot frame; when the counter wraps, encode+decode the completed frame; **output the decoded
  frame while the next frame accumulates**. Grid sample fed in at grid tick `t` emerges at grid
  tick `t + 160` ⇒ constant one-frame delay. The trap is emitting the just-completed frame on the
  same tick that completed it — that gives a delay that *varies 0…159 grid samples across the
  frame* instead of a constant 160.
- Prime the output frame with zeros at `prepareToPlay` (first 20 ms of GSM output is silence — the
  CODEC_ENABLE 10 ms fade covers engagement; document in NOTES).
- Sub-sample alignment: the 8 kHz latch phase is fractional against host samples, so the GSM path's
  effective group delay carries ±1 grid period (±fs/8000 host samples ≈ ±0.125 ms) of phase offset
  versus the plain delay line. **Do not chase sample-exactness here** — μ-law↔GSM mode crossfade is
  10 ms, misalignment comb is transient, and GSM is lossy anyway. Harness bound: cross-correlation
  peak within ±fs/8000 samples of `kCompLatency`.
- Non-integer `0.020·fs` (weird host rates): the plain delay uses `kCompLatency` (ceiled); GSM path
  misaligns by < 1 sample. Accept, note in NOTES.

μ-law↔GSM mode switch: run both sub-paths during the 10 ms crossfade only (GSM state persists while
enabled; μ-law is stateless-cheap), or simpler per ARCHITECTURE: crossfade output taps. Both
sub-paths already delay-aligned by construction above, so the crossfade is phase-coherent to within
the grid jitter.

---

## 4. libgsm vendoring

**Canonical source:** [quut.com/gsm](https://quut.com/gsm/) (Jutta Degener & Carsten Bormann, TU
Berlin), current tarball gsm-1.0.22; GitHub mirrors exist
([timothytylee/libgsm](https://github.com/timothytylee/libgsm),
[MartinEesmaa/libgsm](https://github.com/MartinEesmaa/libgsm)). Vendor from the quut.com tarball
into `plugins/O-Bitrot/third_party/libgsm/{src,inc}` + `COPYRIGHT` verbatim.

**License (verified, 2009 revision):** *"Permission to use, copy, modify, and distribute this
software for any purpose with or without fee is hereby granted, provided that this notice is not
removed and that neither the authors nor the Technische Universitaet Berlin are deemed to have made
any representations as to the suitability of this software for any purpose nor are held responsible
for any defects of this software. THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE."* — ISC-style
permissive, AGPL-3.0-compatible. Record in plugin NOTES.md before the first vendored commit
(ARCHITECTURE gate). GPL firewall intact: libgsm comes from upstream, not RSBrokenMedia's tree.

**Library sources (18 .c, vendor all):** `add.c code.c debug.c decode.c long_term.c lpc.c
preprocess.c rpe.c gsm_create.c gsm_decode.c gsm_destroy.c gsm_encode.c gsm_explode.c gsm_implode.c
gsm_option.c gsm_print.c short_term.c table.c`. Headers in `inc/`: `gsm.h` (public) + `private.h
proto.h unproto.h config.h`. (Skip the toast/ utilities.)

**Compile definitions:** `SASR` (both clang and MSVC do signed arithmetic `>>` — required),
`NDEBUG` (stubs assertions/debug.c), `NeedFunctionPrototypes=1` (the `P()` macro in proto.h then
expands to ANSI prototypes — this is what makes 1992 C compile cleanly as C17 on MSVC). Do **not**
define `WAV49` (MS-GSM packing — we want standard 33-byte frames), `FAST`, `USE_FLOAT_MUL`,
`LTP_CUT`.

**CMake pattern** (no plugin in the suite hand-authors a vendored static lib yet — O-Orbit's SAF
uses upstream's own CMake; the transferable rule from it is that all warning suppressions stay
`PRIVATE` so nothing leaks into the plugin target):

```cmake
add_library(OBitrot_gsm STATIC
    third_party/libgsm/src/add.c ... )              # all 18
target_include_directories(OBitrot_gsm PUBLIC third_party/libgsm/inc)
target_compile_definitions(OBitrot_gsm PRIVATE SASR NDEBUG NeedFunctionPrototypes=1)
if(MSVC)
    target_compile_options(OBitrot_gsm PRIVATE /w)
else()
    target_compile_options(OBitrot_gsm PRIVATE -w)
endif()
set_target_properties(OBitrot_gsm PROPERTIES POSITION_INDEPENDENT_CODE ON)
# then: target_link_libraries(OBitrot PRIVATE OBitrot_gsm ...)
```

Root CMake globs `plugins/*` — no root-level edit needed. Do **not** link OBitrot_gsm into the
harness separately; the harness compiles PluginProcessor.cpp and links the same lib target.

**API usage (RT contract):**

- **Two separate `gsm` handles** — encoder and decoder state differ; one `gsm_create()` each in
  `prepareToPlay` (mallocs), `gsm_destroy()` in `releaseResources`/destructor. Codec chain is mono
  ⇒ one pair total.
- Per frame on the audio thread (allocation-free, fixed work):
  `gsm_encode(encState, gsm_signal[160], gsm_byte[33]);`
  `gsm_decode(decState, frame, out160)` — returns −1 on malformed frame (can't happen on our own
  frames; assert in debug).
- Scaling (deep-dive §3.3 / RSBrokenMedia-proven): in `(gsm_signal)(x·4096) << 3` masked to
  `0xFFF8`; out `(sample >> 3) / 4096.0f`.
- **Phase 2.5 gate order:** (1) vendor + license file commit, (2) harness probe that round-trips a
  160-sample sine frame standalone (non-NaN, correlated, bounded), (3) only then wire into
  CodecStage. Fallback (μ-law + extra decimation labeled "GSM") decided inside Phase 2.5 if the
  gate fails; CODEC_MODE keeps 2 choices either way (`critical_choice_param_needs_two_choices`).

**MSVC:** compiles as C automatically (.c extension); with `NeedFunctionPrototypes=1` there are no
K&R definitions left to trip C17; `/w` silences the C89 conversion warnings; pluginval Windows CI
run will exercise it (`pattern_ci_pluginval10_catches_latent_nan` — run strictness 10 locally 2–3×
before publishing).

---

## 5. Pitfall map for the plan phase (repo memory → where it bites in Stage 2)

| Pattern | Bites in | Discipline |
|---|---|---|
| `pattern_grain_read_before_capture_write_blocksize` | 2.1 | Write ring then read, per sample; lag-0 NORMAL head is only legal with this order |
| `pattern_rng_stream_interleave_blocksize` | 2.1 | 8 streams, consume only at ticks/packets; fixed roll order tape→cd→vinyl |
| `pattern_metric_window_vs_modulation_period` | 2.1 probes | Pitch-trace window ≤ 1024 @ 48k vs 150 ms ramps |
| `pattern_zipper_sweep_probe_needs_liveness_gate` | 2.4 probes | Prove CRUSH_BITS/RATE move the DSP before asserting no zipper |
| `pattern_block_rate_envelope_breaks_blocksize_invariance` | 2.4 | Follower one-pole per SAMPLE |
| `pattern_envelope_follower_state_sticky_nan` | 2.4 | Sanitize follower input; probe pathological input |
| `pattern_arraycoefficients_rt_safe_iir` | 2.5 | `ArrayCoefficients` only; coefficients in prepareToPlay (fixed cutoffs) |
| `critical_delayline_push_without_pop_shifts_delay` | 2.5 trim delay | If using `juce::dsp::DelayLine` for the μ-law/disabled alignment delay, push+pop every sample; a bare hand-rolled integer ring is simpler and bit-exact |
| `pattern_test_fixture_mirrors_drift_silently` | harness | Derive version + constants from target properties / parsed source, never literals |
| Harness param leak (`setBaseline`) | all probes | §1 discipline; neutral ≠ minimum for CRUSH_BITS/RATE/MIX |
| `pattern_stale_host_instance_vs_offline_repro` | DAW checks | Quit/reopen Logic before chasing "host-only" bugs |
| JUCE 8 `getLatencySamples` non-virtual | 2.1 | Report via `setLatencySamples` in prepareToPlay only |

---

## 6. Key decisions handed to the plan phase

1. **Harness scaffolding is Phase 2.1 Task 1** (before any DSP): CMake target + `check`/`noiseAt`/
   `bitIdentical`/`setBaseline` helpers + FUNC-02 null probe (it will fail until the latency scheme
   lands — that's the point).
2. **CodecStage owns all latency** (plain `kCompLatency` delay when disabled/μ-law; GSM chain
   replaces it) — but the delay line itself must exist from **Phase 2.1** so FUNC-02 and the
   latency report are provable before the codec exists. Suggest: a `LatencyTrim` element at the
   chain tail in 2.1, folded into CodecStage in 2.5 (or equivalently, CodecStage skeleton with
   bypass-delay lands in 2.1).
3. **Ring + fractional read is new code borrowing CaptureBuffer's idioms** (double-mod wrap,
   absolute indices, prepare-only allocation) — not a file copy; O-ReverseDelay's ring is
   integer-read.
4. **Sample-accurate tick math is new code** — O-Polystutter's updateBeatSync contributes fallbacks
   /clamps/edge-guard only; its block-granular trigger would fail QUAL-02.
5. **Jump crossfades are true two-head fades** (old head keeps reading during the fade), not
   O-Polystutter's held-sample blend.
6. **libgsm vendored per §4**; round-trip harness gate before integration; fallback decision inside
   Phase 2.5.

## Sources

- Repo: O-Octagon / O-ReverseDelay / O-simpleGrain / O-Contrabass render harnesses; O-ReverseDelay
  `CaptureBuffer.h`; O-Polystutter `RepeatLane.cpp`, `PluginProcessor.cpp`; O-Orbit CMakeLists
  (vendoring precedent); `research/glitch-effects/degradation-dsp-deep-dive.md` §3.3/§3.4/§4.
- Web: [quut.com/gsm](https://quut.com/gsm/) (canonical libgsm),
  [timothytylee/libgsm](https://github.com/timothytylee/libgsm) (Makefile/COPYRIGHT verified),
  [RSBrokenMedia](https://github.com/reillypascal/RSBrokenMedia) (GSMProcessor pattern read,
  patterns only — GPL firewall).
