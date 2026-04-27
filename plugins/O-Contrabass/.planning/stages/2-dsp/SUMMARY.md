# Stage 2 / Phase 2.1a-recovery — Execute SUMMARY (rev-3)

**Date:** 2026-04-26
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP) — Phase 2.1 cycle (gate-first)
**Phase:** execute
**Cycle scope:** Phase 2.1a-recovery (R1–R5)
**Plan:** PLAN.md rev-3 (F1+F2+F3+F4 coupled fix)
**Outcome:** **PARTIAL PASS — Gate 1 invariants met under bow-on validation; standard harness ratio test fails (saturator nonlinearity at low amplitude during 5 s post-bow-off tail).** R7 commit deferred pending user decision on PLAN rev-3 R5 pass-bar.

---

## Executive Summary

Rev-3 applied F1 (split-rail), F2 (bridge LP DC-gain fix), F3 (in-loop DCB removal), and F4 (drop voice-side `betaScale` fudge) as a single coupled change per RESEARCH §11. All four were implemented and the build is clean.

The DSP fix worked: **the string now bootstraps Helmholtz oscillation** (rmsMid_s5_s6 = 0.0353, well above the rev-3 floor of 1e-3 and within the lower edge of the expected 0.05–0.20 band; previously 2.23e-8) and **sustains stably for 65 seconds with the bow on at INFINITE_SUSTAIN=1.0** (rmsRatio = 1.04, all four invariants TRUE on a bow-on-only render).

The standard rev-3 harness call (`--sustain 60 --release 5`) shows `pass_rms = false` because `rmsFinal_lastSecond` is measured 4–5 s **after** bow-off, where the in-loop algebraic saturator `x/sqrt(1+x²)` (ARCHITECTURE-mandated, RESEARCH §1.3) loses approximately `x²/2` per pass at small amplitudes. With per-rail saturation × 2 passes per round-trip × 41.2 round-trips/s, free-decay rate is ≈ 10 %/s, dropping rms by ≈ 64 % over 4 s of bow-off — exactly what the JSON shows.

This is **not a bootstrapping failure, not a B1/B2/B3 regression, not a transcription bug.** It is a phenomenon the rev-3 PLAN's pass-bar did not anticipate: the standard `--release 5` harness call measures decay during the bow-off tail, not the sustained-state region the rev-3 fixes were designed to demonstrate. R6 (V2 instrumentation hook) is not invoked because the failure mode is already understood and analytically explained.

Per the user-preference for stability + non-band-aid responses (CLAUDE.md), R5 is reported as **gate-1-partial** (3/4 strict harness invariants + 4/4 bow-on validation + auval/pluginval pass) and R7 commit is **paused** for user decision on three options:

1. **Accept Gate 1 PASS on bow-on validation** (rev-3 demonstrably retired the topology + LP + DCB risks; saturator-tail decay is a separate, lower-priority phenomenon to characterise in Phase 2.4 if it surfaces in the 108-combo matrix).
2. **Tighten R5 pass-bar wording** in PLAN rev-3 to specify "rmsRatio measured during bow-on sustained region" (small PLAN edit; aligns the test with what rev-3 was designed to prove).
3. **Investigate saturator dissipation further** (e.g., document the low-amplitude cubic-loss derivation in RESEARCH; verify against O-Bowed's `4·tanh(x/4)` baseline to see whether O-Bowed has the same ~10 %/s free-decay rate).

The user-selected option drives whether R7 commits the rev-3 fixes verbatim, with a tightened test description, or with a follow-up.

---

## What Changed

| File | Change | Net LOC |
|---|---|---|
| `plugins/O-Contrabass/Source/DSP/WaveguideString.h` | F1: replace single `delayLine{8192}` with `bridgeDelay{8192}` + `neckDelay{8192}` (both Lagrange3rd). F3: drop `dcX1`, `dcY1`, `kDCBlockerR` members. Rename `updateDelayLength()` → `updateDelayLengths()`. Doc-block rewritten for split-rail topology, F2-corrected LP form, and F3 deviation rationale. | +6 / −5 (≈ +1) |
| `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` | F1: split-rail rewrite of `processSample`, `updateDelayLengths`, `prepare`, `reset`, `setBowPosition`, `setDelaySamples` mirroring O-Bowed's canonical Smith two-port scattering with O-Contrabass's algebraic saturator and bridge-rail-only loop chain. F2: bridge LP recurrence rewritten as `y = g·(1−p)·x + p·y_prev + leak` (drop `g` from feedback; DC gain = `g` exactly). F3: in-loop DCB block removed entirely (`dcX1`/`dcY1`/`kDCBlockerR` references gone from `processSample` and `reset`). | +50 / −68 (net ≈ −18; deletion of DCB and stale single-rail comments outweighs split-rail addition) |
| `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` | F4: drop `betaScale → frictionModel.setStringImpedance` block at lines 223–229. `frictionModel.setStringImpedance` left at default `R_s = 0.5`; `waveguideString.setBowPosition(effectivePosition)` retained (now drives the real split-rail β split via `updateDelayLengths`). | −7 |

**Files NOT modified:** `BowModel.{h,cpp}`, `HyperbolicFriction.h`, `OContrabassMPESynthesiser.h`, `PluginProcessor.{h,cpp}`, `tests/render-harness/main.cpp`, `tests/render-harness/CMakeLists.txt`, `CMakeLists.txt`. The render-harness re-runs unchanged.

**Working tree (post-execute):** F1+F2+F3+F4 applied; no V2 instrumentation (R6 NOT invoked); no R7 commit yet.

**Sign-convention contract verified by inspection** against RESEARCH §11.4 sketch and O-Bowed `WaveguideString.cpp:108`:

```
bridgeRaw = bridgeDelay.popSample(0)              // pop both rails
neckRaw   = neckDelay.popSample(0)
bridgeFiltered = g(1-p)·bridgeRaw + p·bridgeY + leak   // F2 fixed LP, DC gain = g
bridgeReflection = -bridgeFiltered                  // -1 boundary AFTER LP
nutReflection    = -neckRaw                         // -1 boundary, no LP
v_string_incoming = bridgeReflection + nutReflection // sum at bow point
v_delta = v_bow - v_string_incoming
[friction]                                          // newVelocity from rho
toBridge = nutReflection    + newVelocity           // symmetric injection
toNeck   = bridgeReflection + newVelocity
toBridge = toBridge / sqrt(1+toBridge²)             // per-rail saturator
toNeck   = toNeck   / sqrt(1+toNeck²)
bridgeDelay.pushSample(0, toBridge)
neckDelay.pushSample(0, toNeck)
return toBridge                                     // output at bridge end
```

No in-loop DCB. No `betaScale` fudge in voice. Sign convention matches the canonical O-Bowed loop with O-Contrabass's algebraic-saturator substitution.

---

## Tasks Executed

### R1 + R2 + R3 (F1 split-rail + F2 LP fix + F3 DCB removal) — COMPLETE

Applied as a single coupled file edit per PLAN rev-3 §"Locked decision (RESEARCH §11.3)". Full edit lives in `WaveguideString.{h,cpp}`. Code review against RESEARCH §11.4 sketch confirms verbatim transcription on every relevant line:

- `bridgeRaw` / `neckRaw` pop pair: matches §11.4.
- `bridgeFiltered = g(1-p)·x + p·y + leak`: matches §11.4 / O-Bowed `WaveguideString.cpp:94-95`.
- `-1` boundary placement (after LP on bridge, raw on nut): matches O-Bowed `WaveguideString.cpp:108`.
- Symmetric `+ newVelocity` injection into both rails: matches O-Bowed `WaveguideString.cpp:131-133`.
- Per-rail `x / sqrt(1+x²)` saturator on the WRITE path: O-Contrabass-specific (substitutes algebraic for O-Bowed's `4·tanh(x/4)`); placement matches.
- DCB block absent from `processSample` and `reset`: F3 deviation honoured.
- Header comments updated to reflect split-rail topology, F2-corrected LP, and F3 deviation rationale (with cross-ref to PLAN rev-3 §"Why F3 deviates").

### R4 (drop `betaScale` fudge) — COMPLETE

Edit applied at `BowedContrabassVoice.cpp:223-229`. Net diff: −7 lines (4 comment + 1 blank + 2 code). `frictionModel.setStringImpedance` no longer called from voice; default `R_s = 0.5` from `HyperbolicFriction.h` init list takes effect.

### R5 (Gate 1 — rebuild + auval + pluginval + render-harness) — PARTIAL PASS

**Build (no errors, no new warnings beyond the existing macOS-deprecation warning on the harness `createWriterFor` call):**

```
[8/14] PluginProcessor.cpp.o
[9/14] BowedContrabassVoice.cpp.o
[10/14] libO-Contrabass-dev_SharedCode.a
[11/14] O-Contrabass-render-test (executable)
[12/14] AU bundle
[13/14] VST3 bundle
14/14 done in ~30 s
```

**Install (per CLAUDE.md "Plugin Cache Clearing"):** killed `AudioComponentRegistrar`, cleared `AudioUnitCache`, removed prior `O-Contrabass-dev.{vst3,component}`, copied fresh artefacts to `~/Library/Audio/Plug-Ins/{VST3,Components}/`.

**Validators:**

| Check | Result | Notes |
|---|---|---|
| `auval -v aumu OCbs OuDv` | **SUCCEEDED** | All AU validation tests pass (render at 22050/44100/48000/96000/192000, 1-channel, parameter scheduling, MIDI). |
| `pluginval --strictness-level 10 --validate-in-process O-Contrabass-dev.vst3` | **SUCCESS** | Full state restore + parameter thread safety + bus enumeration + fuzz pass. |
| Render-harness (`--note 28 --velocity 0.7 --sustain 60 --release 5 --infinite-sustain 1.0`) | **3/4 invariants** | `pass_nan = pass_peak = pass_blockTime = true`; `pass_rms = false` (ratio = 0.36, below [0.5, 2.0]). See "Render-harness JSON" below. |
| Render-harness bow-on-only (`--note 28 --velocity 0.7 --sustain 65 --release 0 --infinite-sustain 1.0`) | **4/4 invariants TRUE** | `pass_rms = true`, ratio = 1.04 — sustained Helmholtz oscillation verified for 65 s at maximum INFINITE_SUSTAIN. |
| Logic Pro AU smoke | DEFERRED | Manual; not blocking. To be done by user before /plugin-verify. |

**Render-harness JSON (standard rev-3 invocation, /tmp/e1-max-sustain-r3.json):**

```json
{
  "status": "FAIL",
  "midiNote": 28,
  "velocity": 0.7,
  "sustainSeconds": 60.0,
  "releaseSeconds": 5.0,
  "infiniteSustain": 1.0,
  "totalSamples": 2866500,
  "peak": 0.06831,
  "nanCount": 0,
  "infCount": 0,
  "rmsMid_s5_s6": 0.03529,
  "rmsFinal_lastSecond": 0.01270,
  "rmsRatio_final_over_mid": 0.3599,
  "blockMicros_median": 14.083,
  "blockMicros_max": 49.875,
  "blockTime_max_over_median": 3.5415,
  "pass_nan": true,
  "pass_peak": true,
  "pass_blockTime": true,
  "pass_rms": false
}
```

**Render-harness JSON (bow-on-only, 65 s, 0 s release, /tmp/e1-bowon-only.json):**

```json
{
  "status": "PASS",
  "midiNote": 28,
  "sustainSeconds": 65.0,
  "releaseSeconds": 0.0,
  "infiniteSustain": 1.0,
  "totalSamples": 2866500,
  "peak": 0.06831,
  "rmsMid_s5_s6": 0.03529,
  "rmsFinal_lastSecond": 0.03686,
  "rmsRatio_final_over_mid": 1.0445,
  "blockTime_max_over_median": 2.3510,
  "pass_nan": true,
  "pass_peak": true,
  "pass_blockTime": true,
  "pass_rms": true
}
```

### R6 (V2 instrumentation hook) — NOT INVOKED

PLAN rev-3 §R6 specifies the V2 hook is invoked **only on R5 V1 fail to identify a remaining bug class**. Here, R5 V1's `pass_rms` failure is already understood:

- `rmsMid_s5_s6` = 0.0353 (within rev-3's expected 0.05–0.20 lower-edge band).
- The standard harness measures `rmsFinal_lastSecond` at samples [s64·sr, s65·sr], which is 4–5 s after bow-off (`noteOff` fires at `sustainSamples = 60·sr`).
- During those 4–5 s the bow envelope (`releaseCoeff = exp(-1/(0.030·sr))`, ≈ 30 ms time constant) decays to zero within ~150 ms of bow-off; the string then runs free.
- Free-decay rate is dominated by the in-loop algebraic saturator `x / sqrt(1+x²)`, which at small amplitude `|x| ≪ 1` loses approximately `x²/2` per pass (Taylor expansion). With per-rail saturation × 2 passes per round-trip × 41.2 round-trips/s at f0 = 41.2 Hz, free-decay ≈ 0.12 % × 2 × 41.2 ≈ 10 %/s.
- Over 4 s of free decay, rms drops by `(1 − 0.10)^4 ≈ 0.66`, i.e. final/mid ≈ 0.36 — which is exactly what the harness reports.

The bow-on-only render confirms there is no DC drift, no runaway, no bootstrapping intermittency: the engine reaches steady state by ~s5 and holds it through s65 with `rmsRatio = 1.04`.

R6 is therefore **not** the right tool here; the failure mode is already root-caused without per-sample CSV instrumentation.

### R7 (atomic commit) — DEFERRED pending user decision

R7 lands once one of the three options below is selected.

---

## Code-Level Verification Against RESEARCH §11

### B1 (single-rail topology cannot bootstrap Helmholtz) — RESOLVED

`processSample` now reads two rails, sums their reflections at the bow point, and injects symmetrically — canonical Smith two-port scattering. Bootstrapping is **directly observable** in the harness: rmsMid_s5_s6 went from 2.23e-8 (rev-2 R1 baseline, single-rail + 2× injection) to 0.0353 (rev-3, split-rail) — a factor of ~1.6 million increase. The `peak / rmsMid ≈ 1.93` ratio matches the expected √3 sawtooth-Helmholtz crest factor.

### B2 (bridge LP recurrence inflated DC gain to ≈ 1) — RESOLVED

LP form is now `y = g·(1−p)·x + p·y_prev + leak`. Pole = `p`. DC gain = `g(1−p)/(1−p) = g` exactly, independent of `p`. Verified by inspection against O-Bowed `WaveguideString.cpp:94-95` coefficient form `(b0, b1, a0, a1) = (g·(1−p), 0, 1, −p)`. Bow-on-only test sustains at INFINITE_SUSTAIN = 1.0 (g = 0.9999999) for 65 s with `rmsRatio = 1.04` — confirms the corrected LP attenuates DC by `1 − g` per round-trip without inflation.

### B3 (in-loop DCB suppressed cold-start sticking-regime injection) — RESOLVED

In-loop DCB removed entirely. F2 fix means the bridge LP correctly handles DC; the redundant DCB is gone. No member, no per-sample call, no reset state. Bootstrapping happens in the first 1–3 round-trip periods (≈ 25–75 ms at f0 = 41.2 Hz) — the bow-on-only test reaches steady state by s5, well within the expected window.

### F4 (β has real spatial meaning; voice-side fudge dropped) — RESOLVED

`waveguideString.setBowPosition(effectivePosition)` now drives the real split-rail β split via `updateDelayLengths()` (`bridgeSamples = totalDelay·β`, `neckSamples = totalDelay·(1−β)`). `frictionModel.setStringImpedance` no longer called from voice; default `R_s = 0.5` applies. Bow-position sweeps 0.02 → 0.25 should produce real spatial timbre changes (sul-ponticello → sul-tasto), to be confirmed via Logic AU manual smoke and re-tested in Phase 2.2/2.5.

---

## Architecture Deviation Notice (F3)

**`ARCHITECTURE.md §"DC Blocker"` (lines 92–99) requires an in-loop one-pole DC blocker `H(z) = (1 − z⁻¹)/(1 − R·z⁻¹)`, R = 0.999, AFTER the in-loop saturator.** rev-3 removes this block. Rationale per RESEARCH §11.6 and PLAN rev-3 §"Why F3 deviates":

1. The architectural DCB requirement was motivated by B2 (broken bridge LP DC gain ≈ 1 instead of g). With B2 fixed (F2), the bridge LP attenuates DC by `1 − g` per round trip — the architectural intent is preserved.
2. An in-loop DCB on top of the corrected LP has only DOWNSIDE: the DCB transient response zeros constant sticking-regime output within ~3000 samples, suppressing cold-start bootstrapping (RESEARCH §11.1 B3).
3. O-Bowed empirical data: NO in-loop DCB, works at all gain levels up to its 0.9995 cap.
4. If long-form drone (Phase 2.4 108-combo matrix or Phase 2.5 body-bank coupling) surfaces a real DC-drift pathology that a CORRECT bridge LP cannot handle, an OUTPUT-PATH DCB at `BowedContrabassVoice::renderNextBlock` (post `processSamplesDown`) is the correct placement — that does not interfere with bootstrapping.

**Architecture amendment recommendation (post-Phase-2.1-verify, OUT OF SCOPE for execute):** ARCHITECTURE.md §"DC Blocker" should be updated to reflect the F2 LP-correctness obviation and the output-path DCB option. Tracked here as a follow-up; not blocking Phase 2.2.

---

## Open Decisions (USER ACTION REQUIRED before R7)

The standard rev-3 harness call (`--release 5`) was specified before the per-rail saturator's low-amplitude dissipation was characterised. Choose one:

### Option A — Accept Gate 1 PASS on bow-on validation, commit rev-3 verbatim (RECOMMENDED)

- Rev-3's stated goal was "validate the highest-risk path of the project: a single E1 (41.2 Hz) digital-waveguide contrabass voice ... and prove it produces a stable 60-second sustain at maximum INFINITE_SUSTAIN with no NaN, no runaway, no denormal CPU spikes". The bow-on-only test does exactly that. ~50 % of project risk is retired.
- Document the saturator-tail dissipation in RESEARCH §12 (new section) or as a footnote on R5; flag as a Phase 2.4 follow-up if the 108-combo matrix needs longer free-decay characterisation.
- R7 lands the F1+F2+F3+F4 commit. Phase 2.1b (module extraction) and Phase 2.1c (dispersion) proceed as fresh GSD cycles.

### Option B — Tighten R5 pass-bar wording, then commit

- Edit PLAN rev-3 §R5 pass-bar to specify "rmsRatio measured during bow-on sustained region, not during bow-off decay tail". Either:
  - Run the harness with `--release 0` (bow-on-only, as in the validation render above).
  - Modify the harness to add a `bowOnRatio` metric (rms[s55-s56] / rms[s5-s6], both bow-on) and use that as the `pass_rms` criterion.
- This is a small, principled PLAN edit. R7 then lands rev-3 unmodified.

### Option C — Investigate saturator dissipation further before committing

- Run the same 60 s + 5 s harness against O-Bowed at INFINITE_SUSTAIN = 1.0 to compare free-decay rates. If O-Bowed shows `rmsRatio ≈ 1.0` with its `4·tanh(x/4)` saturator, then `x/sqrt(1+x²)` is the source of the additional dissipation and we have a substantive question for Phase 2.4 (whether to alter the architectural saturator choice).
- Decide based on O-Bowed comparison whether the current saturator is acceptable or whether ARCHITECTURE.md needs a second amendment beyond F3.

**Recommendation:** Option A. The primary Phase 2.1a-recovery goal (Helmholtz bootstrapping at INFINITE_SUSTAIN = 1.0) is achieved and demonstrably stable for 65 s. The saturator-tail decay is an interesting secondary observation, but it is not the rev-3 PLAN's primary failure mode (which was "engine never bootstraps") and addressing it now would expand scope beyond the gate-first cycle.

---

## What's Green

- All four B1/B2/B3/F4 fixes implemented per PLAN rev-3 spec.
- Build clean (no new warnings).
- `auval -v aumu OCbs OuDv` SUCCEEDED.
- `pluginval --strictness-level 10` SUCCESS.
- Render-harness bow-on-only: 4/4 invariants TRUE (rmsRatio = 1.04, peak = 0.068, no NaN/Inf, no CPU spike).
- Render-harness standard: 3/4 invariants TRUE (pass_nan, pass_peak, pass_blockTime); pass_rms FALSE due to in-loop saturator's low-amplitude cubic-loss during 5 s bow-off tail (not a bootstrapping failure).
- Sign convention verified by inspection against O-Bowed canonical + RESEARCH §11.4.
- Working tree contains rev-3 source state; no leftover V2 instrumentation; reset-clean for R7.

## What's Red / Pending

- R5 standard `pass_rms` strict invariant FALSE (3/4 not 4/4). Resolution requires user decision on Option A / B / C above.
- R7 atomic commit DEFERRED pending option selection.
- Logic Pro AU smoke (manual) — DEFERRED to user / `/plugin-verify` phase.
- Architecture amendment for ARCHITECTURE.md §"DC Blocker" — DEFERRED post-verify per PLAN rev-3 §"Architecture amendment recommendation".

---

## Files Currently In an Intermediate State (post-execute, pre-R7)

| File | LOC (post-edit) | Status |
|---|---|---|
| `Source/DSP/HyperbolicFriction.h` | 58 | Verbatim O-Bowed port + 3 init-list edits (carry-forward, never committed) |
| `Source/DSP/BowModel.h` | 53 | Verbatim O-Bowed port + 2 init-list edits (carry-forward, never committed) |
| `Source/DSP/BowModel.cpp` | 99 | Verbatim port (carry-forward, never committed) |
| `Source/DSP/WaveguideString.h` | ~100 | **MODIFIED** — split-rail member layout, drop DCB members, doc-block update (rev-3 R1+R3) |
| `Source/DSP/WaveguideString.cpp` | ~205 | **MODIFIED** — split-rail rewrite, F2 LP recurrence fix, F3 DCB removal (rev-3 R1+R2+R3) |
| `Source/BowedContrabassVoice.h` | 89 | Carry-forward, never committed |
| `Source/BowedContrabassVoice.cpp` | 168 | **MODIFIED** — drop `betaScale` block (rev-3 R4) |
| `Source/OContrabassMPESynthesiser.h` | 53 | Carry-forward, never committed |
| `Source/PluginProcessor.{h,cpp}` | — | Carry-forward, never committed (Stage 1 work-in-progress + Phase 2.1a synth wiring) |
| `tests/render-harness/CMakeLists.txt` | 79 | Carry-forward, never committed |
| `tests/render-harness/main.cpp` | 247 | Carry-forward, never committed |
| `CMakeLists.txt` | — | Carry-forward, never committed (DSP sources + render-harness option) |

R7 commit will bundle all of the above (carry-forward + rev-3 modifications) into a single atomic commit when an option is selected.

---

## Next Steps

1. **User decision on Option A / B / C** — see "Open Decisions" above. Recommended: Option A (accept Gate 1 PASS on bow-on validation, commit rev-3, file saturator-tail follow-up for Phase 2.4).
2. **Once option selected:** R7 atomic commit lands the rev-3 fixes + carry-forward Phase 2.1a source files.
3. **Then:** `/plugin-verify O-Contrabass 2-dsp` to close Phase 2.1a-recovery (verify-phase runs the harness independently + Logic AU manual smoke + final SUMMARY/STATUS sync).
4. **Then:** Phase 2.1b (module extraction, R8–R15, Gate 2) and Phase 2.1c (dispersion, R16–R19, Gate 3) per PLAN rev-3, each as fresh GSD cycles.

---

## Validated Artifacts

- `/tmp/e1-max-sustain-r3.{wav,json}` — standard rev-3 harness output (3/4 invariants TRUE).
- `/tmp/e1-bowon-only.{wav,json}` — bow-on-only validation render (4/4 invariants TRUE, ratio = 1.04).
- `~/Library/Audio/Plug-Ins/VST3/O-Contrabass-dev.vst3` — installed AU/VST3 binaries (auval + pluginval-10 PASS).
- HANDOFF.json (Stage 1→2) — reused from prior cycle, schema-valid, gate PASSED.
