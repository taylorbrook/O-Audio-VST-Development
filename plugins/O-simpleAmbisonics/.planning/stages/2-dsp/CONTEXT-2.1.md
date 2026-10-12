# Stage 2 — DSP · Phase 2.1 (Source → Encoder → Stereo decode) — Context

**Plugin:** O-simpleAmbisonics
**Stage:** 2 of 4 — DSP
**Phase:** 2.1 of 3 — audible chain, no rotation, no HRTF
**GSD phase:** discuss
**Date:** 2026-10-12
**Branch:** `main` @ `3b7525b3` (Stage 1 verified)
**Participants:** Taylor Brook, Claude
**Inputs:** `research/ARCHITECTURE.md` (immutable contract), `ROADMAP.md` §Phase 2.1, `REQUIREMENTS.md`, `stages/1-foundation/{SUMMARY,VERIFICATION}.md`, `Source/dsp/{SpatialEngine,FixedDelay}.h`, `tests/render-harness/main.cpp`

---

## Entry check — what Stage 1 hands over

Re-read at this boundary, not taken on STATUS.md's word:

| Carry-forward | State |
|---|---|
| Harness P0–P5 on the installed bundles | ALL PASS, digest `6a1df555bc125ae3` (pass-through baseline — *expected* to change in 2.1) |
| Latency contract | `setLatencySamples(kPartition = 64)` once in `prepareToPlay`; P0 + P4 + pedalboard assert 64 reported == measured. **Never touched in Stage 2.** |
| Engine seam | `Source/dsp/SpatialEngine.h`: `prepare(fs, maxBlock)` / `reset()` / `setParams(const Params&)` / `process(const float* monoIn, float* outL, float* outR, int n)` — no-op, not yet called |
| Processor | conditioner → `FixedDelay<64>` → seeded `LinearSmoothedValue` gain, slice loop ≤ `maxBlock`, finite scrub, read-before-write in place; only `inputMode` / `outputLevel` are read by DSP |
| Degree parameters | continuous (`NormalisableRange` interval 0) — exact 0° default; the wrap smoother wants continuous angles |
| Namespace rule | `osa_dsp = OSimpleAmbisonics::dsp` alias in the processor TU (bare `dsp::` is ambiguous under `using namespace juce`) |
| Non-blocking human checks | Logic stereo + mono track, Generic editor 12 rows, project save/reopen — still open, repeated formally under COMPAT-02 (Stage 4) |

No contract drift: ARCHITECTURE.md, ROADMAP.md and the 12-parameter set are unchanged since Stage 0.

---

## Discussion summary

The architecture is closed at the algorithm level: every 2.1 component has its formula, its smoothing law and its gate written down (ARCHITECTURE §Core Components, §Algorithm Details). Discuss therefore did **not** re-open algorithm choice. It settled **how Stage 2 runs** (one question for Taylor) and **eight execution-level details** the architecture left implicit, two of which are small but real wrinkles (D6 silent-default, D7 air-LP transparency) that would otherwise surface as surprises at execute.

Two external risks for Phase 2.3 were also retired early so that research for 2.3 starts from facts, not hopes:

- **SADIE II D1 (KU100) HRIR SOFA is downloadable today:** Zenodo record 12092466, file `D1_HRIR_SOFA.zip`, **38.2 MB**, license field `apache2.0` (HTTP 200 on a HEAD probe, 2026-10-12). The york.ac.uk page only links to Zenodo — the earlier guessed york-hosted paths 404.
- **`sofar` 1.3.0 resolves via `uv run --python 3.12 --with sofar`** (not installed system-wide; numpy 2.4.0 / scipy 1.17.0 are). The fallback synthetic-head generator stays documented but is not expected to be needed.

---

## Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| **D1** | Stage 2 cycle structure | **Three GSD cycles, one per ROADMAP phase** (Taylor's call). Artifacts `stages/2-dsp/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}-2.N.md`; STATUS.md carries `stage_phase` / `stage_phase_name` / `stage_phase_total` / `artifact_suffix`; a Stage-2 roll-up `VERIFICATION.md` is written at the 2.3 verify and lists every Stage-2 requirement ID with its gate | O-Octagon precedent (`CONTEXT-2.1.md` …). The rotation recursion (2.2) carries known-errata risk and earns its own research pass; the binaural pipeline (2.3) is ~60 % of project risk and stays isolated |
| **D2** | Engine seam | **Keep the Stage-1 signature** — the processor keeps the input conditioner and calls `engine.process(mono, outL, outR, len)` per slice. ARCHITECTURE's multi-channel `process(const float* const*, …)` sketch is not adopted. Add `const MeterFrame& engine.lastFrame()`; the processor owns the `TripleBuffer<MeterFrame>` and publishes once per host block (the last slice's frame) | The seam is already compiled against by the harness; the conditioner is a bus concern, not a DSP one; publishing once per host block keeps the 30 Hz reader rate-independent |
| **D3** | JUCE in `Source/dsp/` | **Phase 2.1 headers stay JUCE-free** (`<cmath>`, `<array>`, `<algorithm>`, `<cstdint>` only). "Headless" is defined as *no `AudioProcessor` / APVTS dependency*; 2.3 may include `juce_dsp` (FFT) and `juce_audio_basics` (WindowedSinc) because the harness links JUCE anyway | Plain headers are unit-testable in double precision without the module tree and compile faster; the FFT is the only thing that genuinely needs JUCE |
| **D4** | Air LP implementation | **Own 6-line TPT one-pole** (`v = (x − s)·G; y = v + s; s = y + v`) with the coefficient `G = t/(1+t)`, `t = tan(π f_c/fs)`, computed **once per block** at the smoothed cutoff and **lerped per sample** across the block — not `juce::dsp::FirstOrderTPTFilter` | ARCHITECTURE asks for a per-sample linear ramp of the cutoff inside the block; `setCutoffFrequency` recomputes `tan` per call, so a per-sample ramp through it costs 48 k `tan`/s. Lerping `G` is the same topology as lerping the encoder vector, and the TPT pole decays to exact zero (denormal note) |
| **D5** | Block-rate smoothing with the slice loop | "Block" for every block-rate smoother = **one engine `process` call** (one slice ≤ `maxBlock`). `k = 1 − exp(−n/(τ·fs))` is evaluated from the slice length `n`, so the time constant is block-size-invariant (P2 ragged / P3 oversized gates stay meaningful for the engine). The per-sample lerp runs from the previous call's end vector to the new one | A smoother whose rate depends on the host block size would fail P2 against itself; computing `k` from `n` is exact for a one-pole |
| **D6** | Binaural leg in 2.1 | The `DecoderMixer`'s binaural input is fed by the **stereo decoder output as a placeholder** (both legs identical, both delayed 64) — **not silence**. 2.3 replaces the placeholder with the convolver. The harness gate DSP-04 drives `StereoDecoder` directly and the processor in Stereo mode | `decoder` defaults to **Binaural**; a silent binaural leg would make the installed plugin silent at default settings for the whole of 2.1 and 2.2 — bad for DAW hands-on and for the P1 engine render. Recorded as 2.1-only so 2.3's verify can assert the placeholder is gone |
| **D7** | Air LP transparency at d ≤ 1 m | ARCHITECTURE says the LP is "open at d ≤ 1 m (bypass-equivalent)" but a first-order pole at the 0.45·fs clamp (21.6 kHz at 48 k) is **−0.85 dB at 10 kHz** — audibly dark at the "off" position. Implement the clause literally with a **wet mix** `y = x + m·(lp(x) − x)`, `m(d) = clamp(d − 1, 0, 1)` (0 at ≤ 1 m, 1 at ≥ 2 m), `m` smoothed by the same one-pole as `d` and lerped per sample with `G` | 1 m must be *exactly* "no effect" for the before/after lesson and for the P1 reference render; `m` is continuous in `d`, so no knee click; monotonicity (DSP-05) is preserved because both `m` and `f_c` move the same way with `d`. The `f_c(d)` formula and the `g(d)` law are unchanged |
| **D8** | P1 replacement | P1 becomes an **engine-reference null**: a double-precision model of the 2.1 chain in the harness (gain law → air LP+mix → SN3D encode → ±60° cardioid → 64 delay) at a fixed static setting vs the processor render, **max |err| < 1e-5**, on a fresh instance. Front/1 m/order 1/Stereo must yield exactly `0.75·s[n−64]` on both channels (W = 1, X = 1 → 0.5 + 0.5·cos 60°) with the LP mix at 0 — a closed-form anchor the model and the gate share | Bit-equality was a pass-through property; a reference model is the Stage-2 form of the same gate and is independent of the ported code (`pattern_new_lint_zero_must_agree_with_independent_scan`) |
| **D9** | Gate discipline (all 2.1 probes) | From `index_dsp_gate_design`: verdict thresholds are **ratios**, not absolute slack; stimulus must exceed the threshold being tested (`pattern_gate_stimulus_below_threshold_is_vacuous`); gate the clamp *target*, not a downstream guard; **fresh engine/processor instance per render**; goldens are **analytic** (closed-form tables, reference model), never stored renders; the FNV digest is informational only; PERF-01 counts global `operator new`/`delete` inside `processBlock` (distinct addresses are not a bound — `pattern_distinct_buffer_addresses_are_not_an_allocation_bound`); no wall-clock verdicts in 2.1 (PERF-02 is a 2.3 gate and will be run on a quiet machine) | Every one of these rules has bitten a sibling; writing them into the context means the planner cannot omit them |

Two further execution notes, not decisions:

- **`kStereoLevelMatch`** is a `constexpr float = 1.0f` placeholder in `StereoDecoder.h` in 2.1; 2.3's generator emits the real value into `BinauralFilterData.h` and `StereoDecoder.h` reads it from there. The 2.3 decoder-switch level gate (< 1 dB) is where it is verified.
- **Orbit lag is constant, not a defect:** at `orbitRate` = 2 Hz with τ = 20 ms the smoothed azimuth trails the target by ≈ 14°, which is a fixed phase offset of the orbit, not a wobble. The DSP-06 trajectory gate is written on the smoother, not on the orbit.

---

## Phase goal

Replace the Stage-1 pass-through with the **audible ambisonic chain minus rotation and HRTF**: conditioned mono source → distance model → SN3D encoder with smoothed order gating → (identity rotation slot) → ±60° virtual-cardioid stereo decode → 64-sample delay → decoder mixer → seeded output gain. The source audibly pans with Azimuth in Stereo mode, orders light 4/9/16 channels in the published `MeterFrame`, distance darkens and attenuates monotonically, orbit moves the source on its own, and every angle change takes the short way round. Latency stays exactly 64.

## Scope (in)

New headers in `Source/dsp/` (all JUCE-free, `prepare / reset / process`-shaped, double-testable):

1. **`ShEncoder.h`** — `encodeSH16(azDeg, elDeg, float out[16])` lifted from O-GrainScatter `SpatialEncoder.h` (constants `kSqrt3_2`, `kSqrt15_2`, `kSqrt5_8`, `kSqrt3_8`; two trig pairs + identities), plus `OrderGate` (three block-rate one-pole band gains, targets 1 if `n ≤ order` else 0, folded into `c_eff[acn] = Y_acn · orderGain[n(acn)]`), and the per-sample lerp of the 16-vector from the previous call's `c_eff` to the current one.
2. **`DistanceModel.h`** — `g(d) = 1.25/(d + 0.25)`; `f_c(d) = 20000·min(1, d^−0.5)` clamped to `0.45·fs`; TPT one-pole per D4; wet mix per D7; `g`, `G`, `m` smoothed at block rate and lerped per sample; **gain seeded** from the parameter in `prepare` (no fade-in — `pattern_gain_ramp_without_seeding_fades_in_from_silence`).
3. **`Smoothers.h`** — `wrapTo180(x) = x − 360·floor((x + 180)/360)`; `WrapSmoother` (`d = wrapTo180(target − cur); cur = wrapTo180(cur + k·d)`); `OnePoleSmoother` (same without wrap); `k` from the slice length per D5; `snapTo(value)` for `prepare`.
4. **`OrbitLfo.h`** — phase accumulator advanced per call (`φ += rate·n/fs`, wrap 1); `azTarget = wrapTo180(azimuth + 360·φ)`; Off→On edge resets `φ = 0`; On→Off leaves the target at `azimuth` so the smoother glides back the short way; never touches host parameters.
5. **`StereoDecoder.h`** — constant 16→2 matrix on (W, Y, X): `L = k·(0.5·W + 0.5·(cos 60°·X + sin 60°·Y))`, `R = k·(0.5·W + 0.5·(cos 60°·X − sin 60°·Y))`, `k = kStereoLevelMatch = 1.0f` placeholder.
6. **`DecoderMixer.h`** — reuses `DecoderDelay` (= `FixedDelay<kPartition>`) ×2 on the stereo leg; `mix` target 1 (Binaural) / 0 (Stereo), 20 ms linear ramp per sample; `out = mix·binaural + (1 − mix)·stereoDelayed`; binaural leg = placeholder per D6.
7. **`MeterFrame.h`** + **`TripleBuffer.h`** (copied from O-GrainScatter, SPSC wait-free) — `shGain[16]` (= `c_eff` this phase; `R·c_eff` from 2.2), `shPeak[16]`, `azEff / elEff` (post-orbit, smoothed), `azRot / elRot` (= `azEff / elEff` this phase), `orderActive`, `decoderMix`, `outPeakL / R`, `latencySamples`.
8. **`SpatialEngine.h`** — composition per ARCHITECTURE §Processing Order (read `Params` → advance smoothers → build this call's `c_eff` → per sample: distance → encode (lerped) → [identity rotation] → 16-channel bus → meter peaks → stereo matrix + delay → mixer); 16 × `maxBlock` scratch bus and 2 × `maxBlock` leg buffers allocated in `prepare` only; `ShRotator` slot present as an identity pass so 2.2 drops in without re-plumbing.

Processor wiring:

9. Cache the remaining 10 raw-parameter atomics in the constructor; fill `Params` once per host block; `engine.prepare` in `prepareToPlay` (after `setLatencySamples`, which stays the one call site); replace `delay.process(s)` with a per-slice conditioned-mono scratch → `engine.process` → `outputGain.getNextValue()` per sample, same read-before-write discipline; finite scrub kept; `TripleBuffer<MeterFrame>` published once per host block.

Harness (`tests/render-harness/main.cpp`):

10. Keep P0 / P2 / P3 / P4 / P5 as written (P4 must still measure 64; P3 oversized-block bit-equality now exercises the engine's slice invariance). Replace P1 per D8. Add: **DSP-01** 13 × 7 (az × el) grid of all 16 coefficients vs a closed-form double-precision table written independently of the ported code, max |err| < 1e-5, W ≡ 1; **DSP-06** +170° → −170° trajectory sampled per 64-sample block: every point within 10° + ε of 180°, never near 0°, plus a 1 s full-circle sweep click gate on a 1 kHz tone (metric per open question R1); **DSP-05** level monotonic decreasing 0.2 → 20 m, HF/LF ratio decreasing, exact `g(d)` at 0.2 / 1 / 20 m within 0.1 dB, LP mix exactly 0 at d ≤ 1 m; **DSP-04** front L = R, +90° L/R ≥ 20 dB and L > R (AmbiX sign), L·s and R·s correlation ≥ 0 for a direction sweep; **PERF-01** global allocation counter = 0 across `processBlock` at orders 1/2/3, both decoder settings, during an order fade and a decoder fade; **order fade** 1 → 3 → 1 mid-tone click-free with the unused bands reaching exactly 0; **meter frame** publishes 4 / 9 / 16 active channels and `azEff` tracking the orbit.

## Scope (out — later phases)

- `ShRotator.h`, rotated meter tap, DSP-02 gate → **Phase 2.2**.
- `scripts/generate_binaural_filters.py`, `BinauralFilterData.h`, `FILTERS.md`, `BinauralConvolver.h`, real `kStereoLevelMatch`, DSP-03 / DSP-07 (decoder switch) / PERF-02 / QUAL-01 sweep, 44.1 / 96 kHz latency gates → **Phase 2.3**.
- WebView editor, viz Timer, relays → **Stage 3**. Presets, hands-on COMPAT-02, QUAL-02 listening → **Stage 4**.
- `PLUGINS.md` row stays at its Stage-1 text until the Stage-2 roll-up (shared file; one edit per stage).

## Requirements covered this phase (verify gates them)

| Requirement | Priority | Phase 2.1 delivers | Gate |
|---|---|---|---|
| FUNC-01 (partial) | must | chain without rotation / HRTF inside the stereo plugin | P1 reference null |
| FUNC-02 | must | azimuth / elevation / distance position the source | DSP-01, DSP-04, DSP-05 |
| FUNC-03 (encoder side) | must | order 1 / 2 / 3 switchable while running, bands fade | order-fade probe + meter frame 4 / 9 / 16 |
| FUNC-06 | should | orbit on/off + rate, departs from the puck, glides back | meter-frame `azEff` probe |
| DSP-01 | must | SN3D/ACN encoder, < 1e-5 vs reference, W ≡ 1 | DSP-01 |
| DSP-04 | should | ±60° cardioid pair, in-phase, ≥ 20 dB side separation | DSP-04 |
| DSP-05 | should | monotonic 1/d gain + air LP, exact at 0.2 / 1 / 20 m | DSP-05 |
| DSP-06 | must | shortest-path smoothing, no zipper on a full-circle sweep | DSP-06 |
| PERF-01 | must | zero allocations in `processBlock`, lock-free meter handoff | PERF-01 counter; `TripleBuffer` by inspection |
| QUAL-01 (partial) | must | no NaN at 2.1 parameter extremes (full sweep in 2.3) | P5 + extremes render |

FUNC-07 and COMPAT-01 stay complete; pluginval s10 VST3 + AU is re-run at verify because the engine now runs inside the host wrappers.

## Constraints honored

- JUCE 8.0.15; `setLatencySamples` untouched; `getLatencySamples` non-virtual.
- RT safety: all allocation in `prepare`; no locks, no transcendental per sample (trig only at block rate: 2 pairs encoder, none for distance beyond one `tan` per call); `ScopedNoDenormals`; TPT pole and smoothers decay to exact zero.
- Oversized host blocks: engine buffers sized to `maxBlock`, processor slice loop (`pattern_oversampler_overruns_on_oversized_host_block`).
- Nyquist clamp on the air LP (`pattern_sc_filter_needs_nyquist_clamp`); `SmoothedValue::reset()` does not clear a NaN — the engine's own smoothers `snapTo` finite values in `prepare` (`critical_smoothedvalue_reset_does_not_clear_a_nan`).
- Harness: no editor TU, `JUCE_WEB_BROWSER=0`; allocation gates are counters, not ASan (`critical_asan_runtime_hangs_at_init_on_macos26`).
- Trunk-based, path-scoped, **temp-index commit** (`index_git_shared_checkout`): `plugins/O-simpleAmbisonics` only; `PLUGINS.md` untouched this phase.
- After every plugin build: `./scripts/build-and-install.sh O-simpleAmbisonics` (AU cache clear + dual-variant sweep), then targeted `auval -v aufx OSAm OuDv` (never `auval -a` — `pattern_auval_a_sigabrt_native_instruments_tcc`).

## Open questions for research (2.1)

- **R1 — click-gate metric.** ARCHITECTURE's "no sample-to-sample jump > −60 dBFS relative" cannot apply to the raw first difference of a 1 kHz tone (its own first difference is ≈ −17.6 dB re peak). Define the metric as a ratio against the stimulus's own steady-state derivative (e.g. max |Δy| during the sweep / max |Δy| of the static render ≤ 1 + ε, or a high-passed residual against a static reference). Pick one, reuse it for the order-fade probe and for every 2.2 / 2.3 click gate.
- **R2 — allocation counter precedent.** Which sibling harness (O-simpleWavetable / O-Strata / O-Octagon) already overrides global `operator new` / `delete` with a thread-local counter, and how does it exclude the harness's own allocations around the `processBlock` call.
- **R3 — chord error of the per-sample lerp.** Quantify `|c_eff|` dip mid-block when the direction moves far in one call (worst case: 2 Hz orbit at 4096-sample blocks ≈ 61° per call at 48 kHz). Decide whether the orbit target should be advanced per sub-block inside long slices or whether the dip is below audibility (ARCHITECTURE accepts the chord path; put a number on it).
- **R4 — independent DSP-01 table.** Write the harness reference from the deep-dive §2.8 formulas (`N_n^|m| · P_n^|m|(sin e) · T_m(a)`) in double, not from `encodeSH16`; confirm the ACN index ↔ (n, m) assignment and the sign of the sin-terms for m < 0 match AmbiX.
- **R5 — DSP-05 "HF/LF ratio" definition.** Choose bands (e.g. 8–16 kHz vs 100–1 kHz on pink noise, or a two-tone 200 Hz + 12 kHz) so that the ratio is strictly decreasing with `d` and insensitive to the gain law.
- **R6 — `TripleBuffer.h` provenance.** Check whether it is a registered Ouaricon module (`/module-list`) before copying; if so, add it via the module system instead of a bare copy.
- **R7 — `MeterFrame` publish cadence under the slice loop.** Confirm one publish per host block (last slice) is what the 30 Hz Stage-3 reader wants, and that `shPeak` should be the max over the whole host block, not the last slice.

## Success criteria (verify phase checks)

- [ ] `ninja O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics-render-test` builds with 0 warnings; harness configured with `-DOUARICON_BUILD_TESTS=ON`.
- [ ] P0 / P2 / P3 / P4 / P5 still PASS; P4 impulse latency still 64; P0 `getLatencySamples() == 64`.
- [ ] P1 reference null < 1e-5 on a fresh instance; front / 1 m / order 1 / Stereo = exactly `0.75·s[n−64]` both channels.
- [ ] DSP-01, DSP-04, DSP-05, DSP-06, PERF-01, order-fade, meter-frame probes all PASS with the thresholds above (ratios, not slack).
- [ ] pluginval strictness 10 passes for the installed VST3 and AU; `auval -v aufx OSAm OuDv` passes.
- [ ] Installed plugin hands-on in a DAW (Stereo mode): source pans with Azimuth, +90° louder **left**; orders change nothing audible in Stereo mode (documented) but the meter frame shows 4 / 9 / 16; Distance 1 → 20 m darkens and attenuates; Orbit On moves, Off glides back the short way. Default settings (Binaural) are **not silent** (D6 placeholder).
- [ ] No allocation in `processBlock`; no NaN at parameter extremes; no fade-in at the first block after `prepareToPlay`.
- [ ] `SUMMARY-2.1.md` records the new digest and names the D6 placeholder as a 2.3 obligation.

## Sibling references (read at research / execute)

- **O-GrainScatter** — `Source/dsp/SpatialEncoder.h` (`encodeSH16`, trig cache), `Source/dsp/TripleBuffer.h`.
- **O-simpleWavetable** — latest harness + gate conventions (allocation counter, click metric candidates).
- **O-Octagon** — phased Stage-2 artifact naming and STATUS.md fields; `tests/render-harness` probe style.
- **O-simpleSubtractive / O-simpleFM** — harness scaffold lineage (already ported at Stage 1).
- `research/ambisonics-encoding-deep-dive.md` §2.8 (reference table), §3.7 (coefficient smoothing); `research/ambisonics-binaural-decoding-deep-dive.md` §5.6 (rotation smoothing — for 2.2).
