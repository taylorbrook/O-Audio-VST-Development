# Stage 2: DSP — Phase 2.2 (SH rotation + rotated meter tap) - Verification

## Verification Date

2026-10-12

## Method

Goal-backward against `CONTEXT-2.2.md` §Phase goal and §Success criteria, `PLAN-2.2.md` §Success criteria and ROADMAP §Phase 2.2. Every automated claim in `SUMMARY-2.2.md` was **re-run independently** in this verify session; nothing was taken on SUMMARY's word.

- Plugin tree clean at HEAD `6ddb38be` (the 2.2 execute commit). The installed bundles `~/Library/Audio/Plug-Ins/{VST3,Components}/O-simpleAmbisonics-dev.*` are **byte-identical** (`cmp`) to the build artefacts — before and after the forced rebuild below.
- `SpatialEngine.h` touched and the four targets (`VST3`, `AU`, `Standalone`, `render-test`) rebuilt under `juce_recommended_warning_flags`: exit 0, **0 warnings** (`grep -c warning:` on the full log).
- Project harness (`O-simpleAmbisonics-render-test`) run **twice**: **160 checks, 0 failures** each, the two outputs byte-identical (`diff` empty); **digest-2.1 `9189d911a1fe98d5`** asserted as a check and reproduced; digest-2.2 `7f35bc9f91e321b9` reproduced.
- Host checks on the **installed** binaries: targeted `auval -v aufx OSAm OuDv`; pluginval VST3 + AU at strictness 10, twice each.
- Independent measurer: the installed VST3 loaded in pedalboard (`uv run --no-project --python 3.12 --with pedalboard --with numpy`), script kept at `tests/pedalboard/verify_2_2.py`. It shares no code with the harness, `ReferenceModel.h` or `ShRotator.h`; its anchors are the ±60° cardioid pair evaluated by hand on the rotated first-order bus (`L = 0.5·W + 0.4330127·Y + 0.25·X`, `R` with `−Y`) at 1 m / order 1. **35 checks, ALL PASS.**

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.2.md §Phase goal)

1. Drop `ShRotator` into the 2.1 engine so the **scene** rotates after encoding: yaw spins the decoded image while the puck (`azEff / elEff`) stays put.
2. `yaw = −azimuth` brings the source back to front.
3. Pitch raises the front and roll raises the left side with the ARCHITECTURE signs.
4. The meter frame publishes the rotated coefficient vector (`shGain = R_SH·c_eff`) and the rotated direction (`azRot / elRot`).
5. **Nothing 2.1 proved changes** — the 2.1 digest reproduces bit-for-bit at zero rotation.
6. Latency stays exactly 64.

### Deliverables (SUMMARY-2.2.md + code inspection)

1. `Source/dsp/ShRotator.h` — own Ivanic–Ruedenberg transcription in float (`p = (1, 2, 0)`, no transpose, `u ≠ 0` / `w ≠ 0` guards, three-case `P`), `u/v/w` tables filled once in `prepare()`, 83-element `rotIndex` layout with four `static_assert`s, `apply` and fused `applyLerped` with W implicit. `SpatialEngine.h`: three `WrapSmoother`s stepped after the mixer, `prevR ← curR` + rebuild per control step, `bus → tap` rotated per sample with the 2.1 `t`, peak scan and stereo decoder on the rotated `tap`. Inspected: no allocation, no `sqrt` outside `prepare()`, trig only at the control step, `clamp` before `asinf`, `wrapTo180` on `azRot`.
2. `fillFrameState`: `azEff / elEff` pre-rotation; `azRot / elRot = dir (R₃·d)` with the identity fast path when all three smoothed angles are exactly 0.
3. Signs by construction of `buildCartesian` (`Rz·Ry′·Rx` with the ARCHITECTURE matrices); pinned by the harness cardinal probes and, independently, by the pedalboard V4 / V5 anchors.
4. `shGain = ShRotator::apply (curR, curC)`; `shPeak` accumulated post-rotation.
5. The identity is exact in float (0 negative zeros observed); the entire 2.1 probe set runs unchanged and digest-2.1 is now a verdict.
6. `setLatencySamples` untouched; 12 parameters untouched; `ReferenceModel.h` rotates by re-encoding through its own double `buildR3` (never includes `ShRotator.h`).

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1. Scene rotates after encoding; puck stays put | ✅ Achieved | Harness: FUNC-01 probe `azEffDeg == 90.0f` bit-equal under yaw −90 with `azRot 0`; meter frame (2.2) `azEff / elEff` bitEqual to the raw parameters while `azRot / elRot` read 48.5942 / −7.6349 = reference; yaw hook 0 → 90 moves `azRot` by 90 over 10 reads with `azEff` fixed at 37. yaw ≡ azimuth cross-gate 8.9e-8 on all 16 bus channels. pedalboard V1: yaw ±90 / 180 / 37 on a front source puts the impulse at the compensated index 100 with L / R **exactly** the hand cardioid at that azimuth (0.9330127 / 0.0669873; 0.25 / 0.25); V2: yaw ψ (az 0) vs azimuth ψ (yaw 0) over 1 s of noise **bit-equal** (max diff 0) at 90 / 37 / −123; V3: the azimuth host parameter reads 90.0000 under yaw −90. |
| 2. `yaw = −azimuth` → front | ✅ Achieved | Harness FUNC-01: frame `shGain` vs `refSh16 (0, 0)` 2.2e-8 on all 16, output max\|L − R\| 0.000e+00 at order 3. pedalboard V3 on the installed VST3: az +90 / 37 / −150 with the opposite yaw gives \|L − R\| ≤ 1.5e-8 and matches the front render to 1.5e-8. |
| 3. Pitch + raises the front, roll + raises the left | ✅ Achieved | Harness cardinal (a) ψ +90 → azRot 90.00000; (b) θ +90 → elRot 90.00000; (c) φ +90 on a left source → elRot 90.00000. pedalboard V4 (signs pinned off the equator, where Stereo mode can see them): pitch +45 on (az 0, el 45) → overhead (L = R = 0.500000), pitch −45 → front (0.750000); roll +45 on (az 90, el 45) → overhead (0.5), roll −45 → hard left (0.933013 / 0.066987); pitch +90 on a front source → overhead; roll +90 on the front axis invariant, on a front-up source lands at (az −45, el 0) exactly. V5 pins the composition order: pitch +90 then yaw +90 → overhead (yaw-first would read 0.933 / 0.067). |
| 4. Frame publishes `R_SH·c_eff` and the rotated direction | ✅ Achieved | Harness meter frame (2.2): `shGain` vs `refSh16 (rotatedDirection)` 3.5e-7; `azRot / elRot` = reference to 4 decimals; `shPeak` scan is on `tap` (post-rotation) by inspection; round trip ψ 0 → 90 → 0 returns `azRot == azEff` bit-equal (fast path reachable) and the last 8192 samples memcmp-equal to a fresh static render. |
| 5. Nothing 2.1 proved changes | ✅ Achieved | 118 2.1 checks pass with bodies untouched; **digest-2.1 `9189d911a1fe98d5`** reproduced on both runs; identity-exact probe (83 elements exactly 0 / 1, diagonal bit-equal 1.0f, `R₃` bit-equal `I`, 0 negative zeros). pedalboard: the 2.1 anchors inside `verify_2_2.py` (impulse 0.75 at index 100 on the roll-axis invariant, +90° 22.88 dB) still hold on the installed binary. |
| 6. Latency exactly 64 | ✅ Achieved | Harness P0 `latency == 64`, P4 measured == reported (first nonzero 164 at impulse 100), amplitude exactly 0.75. pedalboard: every rotated impulse returns at index 100 after host compensation (residual lag 0) with zero tail. auval Latency PASS. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.2 of 3)
**Requirements for this phase:** FUNC-04, DSP-02 (new), FUNC-01 third box, DSP-06 / PERF-01 / QUAL-01 extended; the remaining stage-2 requirements are 2.3

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-04: Scene rotation via Yaw (must), Pitch and Roll (should), in the SH domain after encoding | must | ✅ Complete | Yaw rotates the decoded image while the puck stays fixed: harness FUNC-01 / meter-frame probes (`azEff` bit-equal to the parameter under yaw, `azRot` moves), pedalboard V1 / V2 / V3 (host azimuth unchanged). Rotated encode == direct encode at the rotated direction: DSP-02 2.6e-7 / 5.2e-7 / 7.2e-7 at orders 1 / 2 / 3 over 200 tuples (gate 1e-4). Pitch and roll: cardinal (b) / (c) + pedalboard V4 anchors exact to 1e-6. |
| DSP-02: SH rotation matrix per order equivalent to re-encoding at the rotated direction | must | ✅ Complete | `R_SH·refSh16 (az, el)` vs `refSh16 (dir (R₃·d))` over 200 random tuples × orders 1 / 2 / 3: max 7.157e-07 < 1e-4; orthogonality 5.6e-7 (blocks) / 1.9e-7 (`R₃`); transpose trap FAILs as expected (1.974); yaw-only closed form 2.5e-7; W exact. On the installed binary: yaw ≡ azimuth bit-equal (pedalboard V2), the Stereo output at (ψ 25, θ −40, φ 70) bit-equal across orders 1 / 2 / 3 (V6 — the order-1 block is self-contained). |
| FUNC-01: Complete chain inside one stereo plugin | must | ⚠️ Partial | Third box now ticked: `yaw = −azimuth` returns the source to front — harness `shGain == refSh16 (0, 0)` 2.2e-8 and \|L − R\| = 0, pedalboard V3 ≤ 1.5e-8 at three azimuths. The real binaural leg (A14 placeholder) → **2.3**. |
| DSP-06: Shortest-path angle smoothing, no zipper (extended to yaw / pitch / roll) | must | ✅ Complete | Yaw sweep 360°/s at order 3: bus-tap click ratio 1.127 ≤ 2.0 on all 16 channels vs 24 static parks (negative control 49.0–156.4 on the m ≠ 0 channels; zonal channels bit-identical); yaw step 0 → 180: 1.0003 vs the same-motion reference (negative control 98.3–544.4); liveness −8.20 dB. pedalboard V7 on the installed VST3: 360°/s sweep with 256-sample host steps, normalised second-difference ratio **1.000** vs the static tone (negative control 24.8), L energy swings 17 dB across quarter-turns; the 0 → 180 flick settles to the rear anchor (L = R, −9.54 dB re front) within 0.25 s. The three `WrapSmoother`s share the azimuth smoother's short-way rule by construction (same class, same k). |
| PERF-01: RT-safe; lock-free meter handoff (re-gated) | must | ✅ Complete | Allocation counter 0 across 200 `processBlock` calls with yaw / pitch / roll moving (yaw 90 @15, pitch −45 @25, roll 30 @35, yaw −180 @70, both 0 @90) on top of the 2.1 motion set; liveness 1. By inspection `ShRotator` has only `float[83]` members, `build` / `applyLerped` are arithmetic on members and stack locals, the only `std::sqrt` calls are in `prepare()`. The 2.3 convolver re-runs this gate. |
| QUAL-01: No artifacts at normal ranges (partial) | must | ⚠️ Partial | Finite at ±180° on all three angles at 48 k / 96 k (harness extremes 2.2, 8 corners each), gimbal θ = +90 under a yaw sweep finite and live; pedalboard V8 32 corners (angles × orders × decoders, pole source, orbit 2 Hz) finite and not silenced; pluginval s10 ×2 both formats. Decoder-switch click, the full sweep with the HRTF leg → **2.3**; preset load → Stage 4. |

**Requirements Summary:**
- ✅ Complete this phase: 2 new (FUNC-04, DSP-02) + DSP-06 and PERF-01 re-confirmed under rotation
- ⚠️ Partial: 3 (FUNC-01, FUNC-03, QUAL-01 — remainder owned by 2.3 / Stage 4)
- ⏸️ Deferred (later phase/stage): 4 stage-2 (FUNC-05, DSP-03, DSP-07, PERF-02 at 2.3) + 12 stage-3/4 (UI-01…08, FUNC-08, FUNC-09, COMPAT-02, QUAL-02)
- ❌ Failed: 0
- Unchanged and re-confirmed by the unchanged 2.1 probe set + digest-2.1: FUNC-02, FUNC-06, DSP-01, DSP-04, DSP-05; COMPAT-01 (pluginval s10 ×2), FUNC-07

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build VST3 / AU / Standalone / render-test | ✅ Pass | forced rebuild after touching `SpatialEngine.h`: exit 0, **0 warnings** |
| Installed == build | ✅ Pass | `cmp` on both Mach-O binaries: identical before and after the forced rebuild |
| Harness, run twice | ✅ Pass | **160 PASS, 0 FAIL** both runs, outputs byte-identical; digest-2.1 `9189d911a1fe98d5` (verdict) and digest-2.2 `7f35bc9f91e321b9` reproduced; negative controls fail as expected (24.84 / 24.81, 66.9, 1.974, 49.0–156.4, 98.3–544.4) |
| DSP-02 + orthogonality + identity + cardinal + FUNC-01 | ✅ Pass | 2.613e-07 / 5.248e-07 / 7.157e-07; 5.589e-07 / 1.946e-07; identity exact, 0 negative zeros; azRot 90.00000 / elRot 90.00000 / elRot 90.00000; 2.229e-08, \|L − R\| 0 |
| P1 null rotated / meter frame / round trip | ✅ Pass | 4.754e-08 (fresh instance); shGain 3.456e-07, azRot / elRot 48.5942 / −7.6349; memcmp-equal, fast path reachable |
| Click gates | ✅ Pass | yaw sweep 1.1269 at ACN 9; yaw step 1.0003 at ACN 6; cross-gate 8.941e-08; liveness −8.20 dB; vs-static step ratios 5.3 / 45.3 / 61.9× printed only (R2) |
| Slicing / PERF-01 / extremes | ✅ Pass | P2c / P3c / P2d bit-equal; 0 allocations with the three angles moving (liveness 1); 8 + 8 corners finite at 48 k / 96 k, gimbal finite (azRot 124.546 elRot 51.369) |
| `auval -v aufx OSAm OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| pluginval VST3 strictness 10 (installed) ×2 | ✅ Pass | exit 0, SUCCESS, 25 test groups, 0 FAILED, both runs |
| pluginval AU strictness 10 (installed) ×2 | ✅ Pass | exit 0, SUCCESS, 25 test groups, 0 FAILED, both runs; the standard `Current program is -1` warning (JUCE AU wrapper) |
| pedalboard on the installed VST3 (`tests/pedalboard/verify_2_2.py`) | ✅ Pass | P0 parameters · V1 yaw anchors exact (impulse at 100, L / R = hand cardioid to 1e-7) + 22.88 dB both ways · V2 yaw ≡ azimuth bit-equal ×3 · V3 yaw = −azimuth ≤ 1.5e-8 ×3 + host azimuth untouched · V4 pitch / roll signs ×7 · V5 composition order ×2 · V6 orders bit-equal under rotation + liveness · V7 sweep finite / 17 dB swing / zipper ratio 1.000 (negative 24.8) / flick settles to −9.54 dB · V8 32 corners finite · V9 state round-trip — **35 checks ALL PASS** |

## Human Verification (non-blocking for Phase 2.3)

The installed-binary measurements above cover the substance of each item (yaw moves the image by exactly the azimuth pan law while the host azimuth stays put; pitch / roll signs; the 0 → 180 flick settles cleanly with a same-motion click ratio of 1.0003 in the harness and 1.000 on the installed binary). The DAW pass confirms them by ear; the one item no measurer can settle is whether the one-pole step response (R2: 5–62× vs a *static* control on bands 1–3, 1.0003 vs the same-motion reference) is audible as a zip. COMPAT-02 / QUAL-02 (Stage 4) repeat the hands-on formally in Logic and Live.

- [ ] Logic or Live, Stereo mode: Yaw spins the image while the Generic editor's Azimuth value stays put.
- [ ] Pitch + raises the front (source at el ≈ 45° to hear it in Stereo); Roll + raises the left.
- [ ] **Flick Yaw 0 → 180 at order 3 on a low tone and listen for a zip** (RESEARCH-2.2 R2). If the ear objects: v1.1 candidates are a longer τ for jumps only, or quadratic (prev / cur / next) element interpolation.
- [ ] The 2.1 list: Stereo mode pans with Azimuth, +90° louder LEFT; Distance 1 → 20 m darkens and attenuates; Orbit On moves, Off glides back the short way; default Binaural setting is NOT silent.

## Issues Found

- **None blocking.** No engine, reference or harness defect surfaced; every SUMMARY-2.2 literal reproduced to the printed digit.
- Measurer lessons (not plugin defects), recorded so the 2.3 verify does not repeat them. The first pedalboard run reported 6 FAILs, all in the measurer: (1) a pedalboard `setattr` on a **linear** ±180° range quantises to its own grid too, not only on skewed ranges — 37 → 37.08, 123 → 123.12 — so the 37° anchor missed by 3e-4 and the round-trip read 123.12; angles are now set by `raw_value = (deg − lo) / (hi − lo)` and read back exact (37.000000); (2) roll is about the FRONT axis, so a front-UP source (az 0, el 45) is not invariant — `Rx(+90)` sends it to (az −45, el 0) and the plugin's 0.370591 / 0.982963 was exactly the hand cardioid there (the probe's expectation was wrong; the corrected probe asserts the az −45 value and the on-axis invariant separately); (3) a block-to-block envelope step in dB is meaningless through the cardioid null (a legitimate 20 dB step) — the zipper gate is now a normalised second-difference ratio vs the static tone, with a snapped-envelope negative control (24.8). The pedalboard set-quantisation applies to every parameter: always set numeric parameters by `raw_value` and compare read-back-vs-restored on round trips.
- Noted for Stage 3 (from SUMMARY-2.2 Incident 1, confirmed on the installed binary): the frame carries the host's float (`elEff` −11.999999 for a −12 parameter); the puck readout should format the parameter value, not the frame.
- Deferred items from SUMMARY-2.2 stand: A14 placeholder (replace with the UPOLS convolver on the ROTATED `tap`; the 2.3 verify must assert it is gone) and real `kStereoLevelMatch` → 2.3; TripleBuffer → `core/triple-buffer` module candidate after Stage 2; −180 / 180 display, gimbal-lock tooltip (compass holds within ≈ 0.01° of the pole), `azEff` readout formatting → Stage 3; D1 cost ≈ 16 MFLOPS; signed zero not observed (first suspect if digest-2.1 ever fails).

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 2.2 of 3; the Stage-2 roll-up `VERIFICATION.md` is written at the 2.3 verify)

**Ready for next phase:** Yes. Next is Stage 2 / Phase 2.3 — binaural decoder (MagLS, SADIE II KU100, 256 taps, three per-order sets in a generated `BinauralFilterData.h`, UPOLS P = 64 on `juce::dsp::FFT`) consuming the **rotated** `tap`; the A14 placeholder lines in `SpatialEngine::renderSample` and `ReferenceModel` are the drop-in points; `kStereoLevelMatch` gets its real value; DSP-03 / DSP-07 / FUNC-05 / PERF-02 gates; PERF-01 re-run with the convolver. digest-2.1 `9189d911a1fe98d5` and digest-2.2 `7f35bc9f91e321b9` carry forward (expected to change only through the binaural leg in Binaural mode; the Stereo-mode renders should stay bit-identical).

**Blockers:** None
