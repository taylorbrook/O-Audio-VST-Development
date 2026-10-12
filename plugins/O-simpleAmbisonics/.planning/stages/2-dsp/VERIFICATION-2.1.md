# Stage 2: DSP — Phase 2.1 (Source → Encoder → Stereo decode) - Verification

## Verification Date

2026-10-12

## Method

Goal-backward against `CONTEXT-2.1.md` §Phase goal and §Success criteria, `PLAN-2.1.md` and ROADMAP §Phase 2.1. Every automated claim in `SUMMARY-2.1.md` was **re-run independently** in this verify session; nothing was taken on SUMMARY's word.

- Plugin tree clean at HEAD `b60516c4` (the 2.1 execute commit). `ninja O-simpleAmbisonics_{VST3,AU} O-simpleAmbisonics-render-test`: no work to do → the build is current with HEAD.
- The installed bundles `~/Library/Audio/Plug-Ins/{VST3,Components}/O-simpleAmbisonics-dev.*` are **byte-identical** (`cmp`) to the build artefacts; still identical after the forced rebuild below.
- `PluginProcessor.cpp` touched and rebuilt under `juce_recommended_warning_flags`: **0 warnings**.
- Project harness (`O-simpleAmbisonics-render-test`): **118 checks, 0 failures**, digest `9189d911a1fe98d5` reproduced.
- Host checks on the **installed** binaries: targeted `auval -v aufx OSAm OuDv`; pluginval VST3 + AU at strictness 10.
- Independent measurer: the installed VST3 loaded in pedalboard (`uv run --python 3.12 --with pedalboard --with numpy`), script kept at `tests/pedalboard/verify_2_1.py`. It shares no code with the harness or `ReferenceModel.h`; its anchors are the closed-form numbers from RESEARCH-2.1 (R5) and the cardioid matrix evaluated by hand. **25 checks, ALL PASS.**

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.1.md §Phase goal)

1. Replace the Stage-1 pass-through with the **audible ambisonic chain minus rotation and HRTF**: conditioned mono → distance model → SN3D encoder with smoothed order gating → identity rotation slot → ±60° cardioid stereo decode → 64-sample delay → decoder mixer → seeded output gain.
2. The source **audibly pans with Azimuth** in Stereo mode (+90° louder left).
3. Orders light **4 / 9 / 16** channels in the published `MeterFrame`; unused bands fade to exactly 0.
4. **Distance** darkens and attenuates monotonically (explainable law, exact anchors).
5. **Orbit** moves the source on its own; every angle change takes the **short way round**; no zipper on a full-circle sweep.
6. **Latency stays exactly 64**; no fade-in; no allocation in `processBlock`; no NaN at the 2.1 extremes.

### Deliverables (SUMMARY-2.1.md + code inspection)

1. `Source/dsp/{Smoothers,ShEncoder,DistanceModel,OrbitLfo,StereoDecoder,DecoderMixer,MeterFrame,TripleBuffer}.h` (JUCE-free) composed in `SpatialEngine.h` on the persistent 64-sample control grid (D5 amendment / A1); `busTap[16]` instead of a 16 × maxBlock bus; the A14 binaural placeholder explicit at the engine call site. Processor: 12 cached atomics, `fillParams()` snapping `distance` / `orbitRate` through their ranges, slice loop through `monoScratch`, seeded per-sample gain, one `MeterFrame` publish per host block.
2. `StereoDecoder.h` with the exact literals `0.5f / 0.25f / 0.4330127f` on (W, Y, X).
3. `LinearRamp` order gains with exact arrival + `orderActive` / `shGain` / `shPeak` in the frame; engine-owned peak accumulation gated on `hasNewData()`.
4. `DistanceModel.h`: `g = 1.25/(d + 0.25)`, TPT air LP clamped to 0.45·fs before `tan`, D7 wet mix exactly 0 at d ≤ 1 m.
5. `OrbitLfo.h` (phase reset on Off→On, `wrapTo180`) + `WrapSmoother` ([−180, 180), τ 20 ms, k from 64 samples).
6. `setLatencySamples(kPartition)` untouched (single Stage-1 call site); all vectors sized in `prepare` only; `ReferenceModel.h` + the full 2.1 probe set in `tests/render-harness/main.cpp`.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1. Audible chain, bit-identical across host slicing | ✅ Achieved | Harness P1 null vs the double `ReferenceModel` (fresh instance) < 1e-5; P1 anchor front / 1 m / order 1 = `0.75f·s[n−64]` `memcmp`-equal in Binaural, Stereo and engine-direct; P2 / P2b / P3 / P3b bit-equal across ragged and oversized blocks **with orbit On at 2 Hz / order 3**. pedalboard V3: impulse back at index 100 after host latency compensation with amplitude exactly 0.7500000 on both channels, no smear (max other sample 0), in both decoder modes. |
| 2. Pans with Azimuth, +90° louder LEFT | ✅ Achieved | Harness DSP-04: front L == R (bit), +90° separation 22.88 dB with L > R, in-phase over 72 directions, processor-level rms ratio ≥ 20 dB. pedalboard V2 on the installed VST3: +90° L/R = **22.88 dB**, −90° R/L = 22.88 dB, front and rear L == R to 1e-4, front output correlates 1.00000 with the source. |
| 3. Orders light 4 / 9 / 16; unused bands exactly 0 | ✅ Achieved | Harness meter frame: `orderActive` 4 / 9 / 16 and `shGain[active..15] == 0`; order fade (a) `shGain[4]` / `[9]` arrive EXACTLY at Y and EXACTLY at 0 within 16 control steps, monotonic; (c) `shPeak[4..15] == 0` after the fade back; bus-tap click ratios 0.9999 / 1.0000 (negative control 66.9 fails as expected). pedalboard V5: Stereo output bit-equal for orders 1 / 2 / 3 — the documented "orders change nothing audible in Stereo mode" (decoder reads W/Y/X only). |
| 4. Distance monotonic, darkens, exact anchors | ✅ Achieved | Harness DSP-05: level strictly decreasing over 26 distances, HF/LF strictly decreasing on (1, 20], anchors +8.87 / 0.00 / −24.19 dB and −10.788 dB HF/LF at 20 m, d ≤ 1 m bit-equal to `0.75f·g(d)·x[n−64]`, clamp target 19845 Hz at 44.1 k, fs 22050 render finite. pedalboard V4 independently: +8.895 / 0.000 / −24.178 dB (0.1 dB gate), HF/LF flat on [0.2, 1] (≤ 0.002 dB) then −1.95 / −5.15 / −7.91 / **−10.788 dB** at 2 / 5 / 10 / 20 m. |
| 5. Orbit moves; short way round; no zipper | ✅ Achieved | Harness DSP-06a (+170 → −170 through 180°, never \|az\| < 90, converged by 0.2 s; Off→On departs from the puck; On→Off glides through 180), DSP-06b click ratios 1.0003 / 1.0000 at 64-sample blocks and at 64 vs 4096 orbit renders (bit-equal), negative control 24.8, liveness 8.35 dB; meter `azEff` spans 327° over 10 reads. pedalboard V6: orbit On 1 Hz varies L energy 17.6 dB across quarter-cycles, Off is static (< 0.01 dB); V7: +170 → −170 keeps the normalised \|L−R\| at ≤ 0.299 vs the static 0.296 at 170° — the long way through 90° would have hit 0.866. |
| 6. Latency 64, no fade-in, RT-safe, finite extremes | ✅ Achieved | P0 `getLatencySamples() == 64`; P4 measured == reported, impulse exactly 0.75; P1 `out[64]` exact (no fade-in); PERF-01 counter 0 allocations across 200 `processBlock` calls (orders 1→3→2, decoders B→S→B, orbit On, d 0.2→20, one 4096 block) with liveness 1; extremes 16 corners finite at 48 k and 96 k. pedalboard V8: 32 corners × 4096 samples finite and not silenced. `TripleBuffer.h` inspected: two atomics (`middle`, `newData`) with acq_rel exchanges, no locks; `processBlock` and `SpatialEngine::process` contain no `new` / `resize` / lock (grep). auval Latency PASS. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.1 of 3)
**Requirements for this phase:** 10 of the 17 stage-2 requirements (7 must, 3 should); the rest are 2.2 / 2.3

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-01: Complete chain inside one stereo plugin | must | ⚠️ Partial | Buses: auval caps `[1, 2] [2, 2]`. +90° / order 1 louder left: 22.88 dB (Binaural == Stereo bit-equal under the A14 placeholder). Rotation criterion (Yaw = −Azimuth brings the source to front) → **2.2**; real binaural leg → **2.3**. |
| FUNC-02: Source positioned by Azimuth / Elevation / Distance | must | ✅ Complete | pedalboard lists the three as host params with ranges −180…180 / −90…90 / 0.2…20; DSP-01 grid covers el ±90 (X, Y = 0 there, `encodeSH16` vs the generic table 2.07e-07); DSP-04 e = ±90 → L == R == 0.5f bit. |
| FUNC-03: Order 1 / 2 / 3 switchable while running | must | ⚠️ Partial | Encoder side complete: 4 / 9 / 16 active, bands fade click-free (bus-tap ratio ≤ 2.0, exact 0 arrival). The decoder-side filter-bank swap is a **2.3** criterion (DSP-07). |
| FUNC-06: Orbit on/off + rate | should | ✅ Complete | DSP-06a Off→On departs from the puck, On→Off glides back the short way; `azEff` tracks the orbit; pedalboard V6. |
| DSP-01: SN3D/ACN encoder < 1e-5, W ≡ 1 | must | ✅ Complete | 13 × 7 grid vs the independent double table (factorials + Legendre): max 2.07e-07; W == 1.0f everywhere; AmbiX sign anchor and the Appendix-B front/left values to 1e-4. |
| DSP-04: Stereo decode without HRTF | should | ✅ Complete | ±60° cardioid pair (ARCHITECTURE's "equivalent" to the ±30° wording): front L == R bit, +90° 22.88 dB, in-phase over 72 directions, normalised correlation ≥ −1e-6 over 36 directions. pedalboard V2 confirms on the installed binary. |
| DSP-05: Distance model monotonic, explainable | should | ✅ Complete | Harness DSP-05 + pedalboard V4 (above). Anchors exact within 0.1 dB at 0.2 / 1 / 20 m; LP wet mix exactly 0 at d ≤ 1 m; Nyquist clamp gated on the clamp target. |
| DSP-06: Shortest-path angle smoothing, no zipper | must | ✅ Complete | +170 → −170 through 180° (harness DSP-06a, pedalboard V7); 1 s full-circle click ratios 1.0003 / 1.0000 vs a negative control at 24.8. |
| PERF-01: RT-safe; lock-free meter handoff | must | ✅ Complete | Allocation counter 0 at orders 1 / 2 / 3 and both decoders, during order and decoder fades (liveness 1); `TripleBuffer` atomics-only by inspection. The 2.3 convolver re-runs this gate before the Stage-2 roll-up. |
| QUAL-01: No artifacts at normal ranges | must | ⚠️ Partial | NaN / denormal: pluginval s10 ×2 + 16-corner extremes at 48 k / 96 k + pedalboard 32 corners, all finite; order-switch and sweep clicks gated. Decoder-switch click and the full parameter sweep → **2.3**; "no clicks on preset load" → Stage 4. |

**Requirements Summary:**
- ✅ Complete: 7 (FUNC-02, FUNC-06, DSP-01, DSP-04, DSP-05, DSP-06, PERF-01)
- ⚠️ Partial: 3 (FUNC-01, FUNC-03, QUAL-01 — remainder owned by 2.2 / 2.3 / Stage 4)
- ⏸️ Deferred (later phase/stage): 7 stage-2 (FUNC-04, FUNC-05, DSP-02, DSP-03, DSP-07, PERF-02 at 2.2 / 2.3) + 12 stage-3/4 (UI-01…08, FUNC-08, FUNC-09, COMPAT-02, QUAL-02)
- ❌ Failed: 0
- Already complete from Stage 1 and re-confirmed here: COMPAT-01 (pluginval s10 ×2), FUNC-07 (harness P1 / P1b, `[1, 2]` caps)

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build VST3 / AU / render-test | ✅ Pass | `ninja`: no work to do at HEAD `b60516c4`; forced rebuild of `PluginProcessor.cpp`: 0 warnings |
| Installed == build | ✅ Pass | `cmp` on both Mach-O binaries: identical (before and after the forced rebuild) |
| Harness (P0–P5, P2b/P3b, DSP-01/04/05/06, order fade, meter frame, extremes, PERF-01) | ✅ Pass | 118 PASS, 0 FAIL; digest `9189d911a1fe98d5` reproduced; negative controls fail as expected (24.84 / 24.81, 66.9) |
| `auval -v aufx OSAm OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED; caps `[1, 2] [2, 2]`; Latency PASS; 0 WARN |
| pluginval VST3 strictness 10 (installed) | ✅ Pass | exit 0, SUCCESS, 25 test groups, 0 FAILED |
| pluginval AU strictness 10 (installed) | ✅ Pass | exit 0, SUCCESS, 25 test groups, 0 FAILED; the standard `Current program is -1` warning (JUCE AU wrapper, `getNumPrograms() == 1`) |
| pedalboard on the installed VST3 (`tests/pedalboard/verify_2_1.py`) | ✅ Pass | V1 default not silent · V2 pan law · V3 latency + 0.75 anchor both modes · V4 distance law + air LP · V5 orders bit-equal in Stereo · V6 orbit · V7 short way · V8 32 corners finite · V9 state round-trip — 25 checks ALL PASS |

## Human Verification (non-blocking for Phase 2.2)

The installed-binary measurements above cover the substance of each item (pan sign, level and tone vs distance, orbit motion, short-way glide, default not silent); the DAW pass confirms them by ear. COMPAT-02 / QUAL-02 (Stage 4) repeat them formally in Logic and Live.

- [ ] Logic or Live, Stereo mode: the source pans with Azimuth; +90° is louder **left**.
- [ ] Order 1 / 2 / 3 changes nothing audible in Stereo mode (documented) — the meter frame shows 4 / 9 / 16 once Stage 3 draws it.
- [ ] Distance 1 → 20 m darkens and attenuates; Orbit On moves the source, Off glides back the short way.
- [ ] Default settings (Binaural) are **not** silent (the A14 placeholder plays the stereo leg until 2.3).

## Issues Found

- **None blocking.**
- SUMMARY's Incident 1 (unsnapped APVTS raw default of the skewed + stepped `distance` range → `fillParams()` snaps `distance` / `orbitRate`) is confirmed fixed on the installed binary: a fresh instance reads `distance_m` 1.0 (raw 0.395518) and the impulse anchor is exactly 0.7500000 — a 1-ulp-low gain would have shown as 0.7499999.
- Measurer lessons (not plugin defects), recorded so the 2.2 / 2.3 verifies do not repeat them: (1) a pedalboard **set** of `distance_m = 1.0` lands on its own quantised grid at ≈ 1.004 m, which engages the air LP (impulse 0.7477 with a 7.5e-4 tail) — leave the parameter at the APVTS default or set `raw_value`; (2) the ±60° cardioid is already asymmetric at 170° (normalised \|L−R\| = 0.296), so a short-way gate must be relative to the static value at the start angle, not an absolute "small" threshold; (3) `orbit` reads back as a numpy bool (`is True` fails).
- Deferred items from SUMMARY-2.1 stand: A14 placeholder and real `kStereoLevelMatch` → 2.3; `ShRotator` slot → 2.2; TripleBuffer → `core/triple-buffer` module candidate after Stage 2; −180 / 180 display → Stage 3.

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 2.1 of 3; the Stage-2 roll-up `VERIFICATION.md` is written at the 2.3 verify)

**Ready for next phase:** Yes. Next is Stage 2 / Phase 2.2 — SH rotation (yaw / pitch / roll) via Ivanic–Ruedenberg, rotated meter tap, DSP-02 gate, yaw-sweep click gate. The identity slot in `SpatialEngine::renderSample` and the `ReferenceModel` rotation extension are the drop-in points; the P1 null, the 0.75 anchor, the digest `9189d911a1fe98d5` (expected to change only if the identity pass is not bit-transparent) and the full 2.1 probe set carry forward.

**Blockers:** None
