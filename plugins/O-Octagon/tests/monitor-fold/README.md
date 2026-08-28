# MonitorFold correctness harness

A standalone gate for `oo::MonitorFold` — the binaural / stereo monitoring fold-down.

## Why it is standalone

`MonitorFold` depends on nothing but `VenueSnapshot`, `VenueGeometry` and three JUCE modules. It
does **not** need `PluginProcessor.cpp`, the WebView editor, or a negotiated bus layout, so its
correctness gate does not need them either. Same reasoning as `tests/unit`, which deliberately links
only the three geometry TUs: a narrow link line compiles in seconds and is structurally immune to a
Stage-3 editor swap.

```sh
./build-standalone.sh && ./monitor-fold-test
```

Exit code 0 on success, 1 on any failure — the `check()`/exit-code idiom every other harness in this
repo uses. **There is no unit-test framework in this repository**; do not resolve a missing symbol by
adding one.

`juce_glue.cpp` supplies `juce_compilationDate` / `juce_compilationTime`, which CMake normally emits
into `JuceLibraryCode`. This harness has no such target.

## What it asserts, and what each check is protecting

| Check | Guards against |
|---|---|
| `woodworth-formula-*` (5) | **Documentation of the model, not class coverage.** These assert properties of a lambda transcribed in the test; `MonitorFold` is not constructed. v1.10.0 (WR-04) renamed them after a mutation pass showed θ-only, 0.5×, 2×, a 45° clamp and a naive 90° clamp all pass them. |
| `itd-class-45deg` / `-90deg-seam` / `-135deg-rear` / `-180deg-zero` | **The class's ITD magnitude**, measured as the far-ear onset lag of an impulse from a speaker at a known azimuth, against the *published* Woodworth formula (r/c · (θ + sin θ), c = 343) with ±2 samples of interpolation slack — never against `kMaxItdSeconds`. 90° is the maximum and the seam; 180° must be **zero** (a naive clamp reads 31.5); 135° exercises the (π − θ) branch. Every mutation that passed the formula checks fails at least one of these. |
| `itd-class-mirror-symmetric` | Hard-left and hard-right speakers produce equal and opposite lags through the class. |
| `bypass-disengaged-not-running` | **The bit-identity contract.** If `isRunning()` were true while disengaged, `GainStage` would clock the fold and pre-v1.6.0 sessions would stop rendering identically. |
| `fold-six-lanes-hard-zero` | The rig lanes must be *silent*, not merely quiet, or a monitor fold leaks into the hall. |
| `itd-right-source-reaches-right-ear-first` | The `atan2 (dx, -dy)` convention. A mirrored fold is inaudible without a reference. **This check caught the ILD-ceiling bug** — see below. |
| `ild-within-plausible-band` | Both rails. `>` alone passed at an ILD of 108 dB, which is the far ear being *silent*. |
| `fold-is-position-dependent` | Non-vacuity, and **mirror symmetry** — the two sides must be equal and opposite. |
| `fold-never-exceeds-source-peak` | The `kFoldTrim` ceiling argument, driven at Cauchy-Schwarz's equality case (eight fully coherent lanes at Σvᵢ² = 1). |
| `nan-guard-recovers` | The shadow filters are recursive and poison stickily, exactly as the air filter does. |
| `geometry-gated-on-generation` | The fold must re-derive on a venue publish and *not* on a source move. |

## The bug this harness caught

The first run failed `itd-right-source-reaches-right-ear-first` with the left ear reading **nothing
at all** (R/L energy ratio 8.8 × 10¹⁰). An uncompressed constant-power pan drives the far ear to
exactly 0 at ±90°, so the inter-aural delay had nothing to delay and the head shadow had nothing to
filter — both cues silently ceased to exist for precisely the lateral sources they matter most for,
and the fold degraded into a hard-panned stereo mix that happened to own eight delay lines.

The fix is `kPanDepth`, an ILD ceiling of ~17 dB (a real head is 15–20 dB at 90°, never infinite).

Note what made it visible: the check measures the **far** ear. Every assertion that measures the
near ear passes with the bug present. The two assertions that passed vacuously alongside it were
then tightened to plausible *bands* rather than `>` comparisons, so neither the rail nor its absence
can pass silently again.
