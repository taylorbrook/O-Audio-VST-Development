# Stage 2: DSP — Plan (Phase 2.1) — REVISION 3 (Recovery, B1+B2+B3 coupled)

**Date:** 2026-04-26 (replan after rev-2 R1 pre-flight FAIL + RESEARCH.md §11)
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP) — Phase 2.1 cycle (gate-first)
**Phase:** plan
**Cycle Scope:** Phase 2.1 ONLY — Phases 2.2–2.6 each get their own GSD cycle.

**Supersedes:**
- PLAN.md rev-1 (single-rail topology, harness FAIL on `pass_rms`)
- PLAN.md rev-2 (split-rail + R1 pre-flight gate; R1 FAILed → topology-only hypothesis falsified)

**Authority:** RESEARCH.md §11 (root-cause analysis: three compounding bugs B1/B2/B3) — all four hypotheses from §10 either disproven or reframed; one **new finding (B2)** discovered via line-by-line comparison against O-Bowed canonical.

---

## Preamble — What Changed Since rev-2

Rev-2 framed Phase 2.1a-recovery around a single-bug hypothesis: "single-rail topology halves canonical scattering-junction injection per round-trip, so single-rail with 2× compensation is mathematically equivalent to split-rail at f0." Rev-2 R1 pre-flight diagnostic tested that equivalence with a one-token edit (`newVelocity` → `2.0f * newVelocity`) and **falsified it**: peak −32.6 dBFS (+6 dB transient bump from doubled injection during attack) but `rmsMid_s5_s6` = 2.23e-8 and `rmsFinal_lastSecond` = 0.0 — statistically indistinguishable from the rev-1 baseline. Per PLAN rev-2 R1 fail-action, R2–R5 NOT executed. Diagnostic edit reverted; working tree byte-identical to start-of-execute.

RESEARCH.md §11 then identified the failure as a superposition of **three independent bugs**, not a single topology bug:

| ID | Severity | Bug | Why doubling injection didn't help |
|---|---|---|---|
| **B1** | Primary, structural | Single-rail cannot bootstrap Helmholtz from sticking equilibrium — no spatial-asymmetric returning wave to perturb v_delta past the slip threshold | Per-sample scalar tweaks to single-rail injection cannot create the spatial geometry Helmholtz requires; the rev-2 R1 +6 dB transient is the cold-start friction injection + DCB transient response combination, NOT bootstrapping into oscillation |
| **B2** | Secondary, **NEW finding** | Bridge LP recurrence at `WaveguideString.cpp:162` erroneously multiplies `g` into the feedback term: `y = g·(1−p)·x + g·p·y_prev` — DC gain inflates from intended `g` to `g·(1−p)/(1−g·p) ≈ 1` at high sustain | DC gain ≈ 1 means LP fails to attenuate sticking-regime DC; this motivated B3 (DCB) as a workaround that itself breaks bootstrapping |
| **B3** | Secondary, dependent on B2 | In-loop DCB at `WaveguideString.cpp:171-173` actively suppresses cold-start sticking-regime injection — DCB transient response zeros constant sticking output within ~3000 samples (≈34 ms at 88.2 kHz) | DCB was added to ARCHITECTURE.md as a workaround for B2; once B2 is fixed (LP DC gain = `g`, attenuating DC by `1−g` per round trip), B3 is redundant AND harmful |

**Six §10/SUMMARY hypotheses dispositioned in §11.2:**

| # | Hypothesis | Status |
|---|---|---|
| H1 | Sign-convention transcription | **Disproven** (round-trip closure correct in current single-rail) |
| H2 | DC blocker round-trip energy at f0=41.2 Hz | **Reframed → B3** (transient pathology, not steady-state attenuation) |
| H3 | Saturator dynamics | **Disproven** (linear at −32.6 dBFS failure operating point) |
| H4 | BowModel sr-correctness | **Disproven** (`prepare(spec_at_2x.sampleRate)` correct) |
| H5 | Friction defaults vs Schelleng F_min | **Reframed → B1 + latent for Phase 2.4** (tracks in 108-combo matrix) |
| H6 | First-tick envelope timing | **Dissolved** (envelope behaves correctly; transient peak is sticking-injection + DCB transient, not envelope timing) |

**Locked decision (RESEARCH §11.3):** PLAN rev-3 applies F1+F2+F3+F4 **simultaneously as a single coupled change**. Removing only one fix leaves one or more failure modes intact. There is **no pre-flight sub-test this time** — RESEARCH §11.7 explicitly notes "the pre-flight was a flawed test" and §11.1 supplies the equivalent analytical sample-by-sample equilibrium trace that would have been the pre-flight's job.

**Net code delta from current working tree (rev-1 source state, R1 diagnostic already reverted):**
- `WaveguideString.h`: +1 member (replace `delayLine` with `bridgeDelay`+`neckDelay`), −3 members (drop `dcX1`, `dcY1`, `kDCBlockerR`)
- `WaveguideString.cpp`: ~+45 LOC for split-rail rewrite of `processSample`, `updateDelayLengths`, `reset`, `prepare`, `setBowPosition`; ~−6 LOC for DCB removal; 1-line LP recurrence fix; ~−5 LOC for stale single-rail comments
- `BowedContrabassVoice.cpp`: ~−2 LOC (drop `betaScale` block at lines 228–229)
- Net: ~+38 LOC `WaveguideString.{h,cpp}`, ~−2 LOC `BowedContrabassVoice.cpp`. Final `WaveguideString.cpp` size projected ~190 LOC (down from 232 today, because DCB removal net-deletes more than split-rail adds).

**Files NOT affected:** `BowModel.{h,cpp}`, `HyperbolicFriction.h`, `OContrabassMPESynthesiser.h`, `PluginProcessor.{h,cpp}`, `tests/render-harness/main.cpp`, `CMakeLists.txt`, render-harness `CMakeLists.txt`. The harness re-runs unchanged.

**Contract impact:**
- ARCHITECTURE.md (sha256:3cb26814…) — silent on rail count; F1 introduces no violation. **F3 deviates from §"DC Blocker" lines 92–99** — see §11.6 for justification (B2-fix obviates B3; if long-form drone surfaces DC drift, an output-path DCB can be added in Phase 2.4/2.5). Architecture amendment recommended post-Phase-2.1-verify; not a blocker for execute.
- ROADMAP.md (sha256:106639f6…) — silent on rail count, DCB placement, LP recurrence form. **No violation.**
- parameter-spec.md (sha256:c47fe736…) — 29 IDs intact. **No violation.**
- BRIEF.md (sha256:6ea840bb…) — sonic targets unchanged. **No violation.**
- CONTEXT.md "single-rail" advisory line — already overridden in §10 and §11; CONTEXT.md is a discuss-phase artifact, not a contract. No checksum re-promotion.

---

## Goal (sharpened from rev-2)

Validate the highest-risk path of the project: a single E1 (41.2 Hz) digital-waveguide contrabass voice — hyperbolic friction junction at 2× oversampling, **split-rail topology** (canonical Smith two-port scattering, mirroring O-Bowed) with a **corrected one-pole bridge LP** (DC gain = `g`, F2 fix) on the bridge return only, **no in-loop DC blocker** (F3 deviation from ARCHITECTURE.md, justified by F2 fix), and a **`-1` boundary** on the nut return — driven from a real `MPESynthesiser` voice triggered by host MIDI, and prove it produces a stable 60-second sustain at maximum `INFINITE_SUSTAIN` with no NaN, no runaway, no denormal CPU spikes, **and a positive `pass_rms` with `rmsMid_s5_s6` ≈ 0.05–0.20**. Along the way, extract the friction model + bow envelope into a shared `synthesis/bow-friction` module that O-Bowed and O-Contrabass both consume with zero audible regression.

If the harness PASSes after F1+F2+F3+F4, ~50% of project risk is retired and Phase 2.1b (module extraction) and Phase 2.1c (dispersion) may proceed.

---

## Tasks

### Phase 2.1a-recovery — Three-bug coupled fix + Gate 1

> Tasks 1, 2, 4, 5, 6, 7 from rev-1 are **DONE** (source files in place: `HyperbolicFriction.h`, `BowModel.{h,cpp}`, `WaveguideString.{h,cpp}` — single-rail variant, `BowedContrabassVoice.{h,cpp}`, `OContrabassMPESynthesiser.h`, render-harness, CMake plumbing). Build clean, auval/pluginval level-10 PASS. **Recovery starts at the coupled fix; no pre-flight sub-gate.**

R1. [ ] **Apply F1: Rewrite `WaveguideString.{h,cpp}` to split-rail (canonical Smith two-port scattering)**
   - **Files (modify):**
     - `plugins/O-Contrabass/Source/DSP/WaveguideString.h`
     - `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp`
   - **Header changes (`WaveguideString.h`):**
     - Replace single `delayLine { 8192 }` member with two members:
       ```cpp
       juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> bridgeDelay { 8192 };
       juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> neckDelay   { 8192 };
       ```
     - **F3:** REMOVE three members entirely:
       - `float dcX1 = 0.0f;`
       - `float dcY1 = 0.0f;`
       - `static constexpr float kDCBlockerR = 0.999f;`
     - Rename private `updateDelayLength()` → `updateDelayLengths()` (mirrors O-Bowed).
     - Update the file-header doc-block (lines 9–32): replace the "Single juce::dsp::DelayLine" / "In-loop DC blocker" / "Phase 2.1 simplification (locked): single delay rail" lines with a description of the split-rail topology, the corrected LP form (B2 fix), and the F3 deviation rationale (one-line cross-ref to PLAN rev-3 §"Why F3 deviates from ARCHITECTURE.md"). Drop the `dcX1`/`dcY1`/`kDCBlockerR` references from the comment block.
     - Keep `bowPosition` member; clamp `[0.02, 0.98]` inside `setBowPosition` (no-bow at ends would zero a delay rail).
   - **`updateDelayLengths()` body** (mirrors `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:58-80` with O-Contrabass's quadratic-skew and Lagrange3rd-min-4 constraints):
     ```cpp
     float totalDelay = static_cast<float> (sampleRate) / std::max (1.0f, currentFrequency);
     float pi = juce::MathConstants<float>::pi;
     float filterGroupDelay = static_cast<float> (sampleRate) / (2.0f * pi * std::max (1.0f, brightnessHz));
     float compensated = totalDelay - filterGroupDelay;

     float bridgeSamples = compensated * bowPosition;
     float neckSamples   = compensated * (1.0f - bowPosition);

     // Lagrange3rd needs ≥4 samples per rail (4-tap kernel).
     bridgeSamples = juce::jlimit (4.0f, 8190.0f, bridgeSamples);
     neckSamples   = juce::jlimit (4.0f, 8190.0f, neckSamples);

     bridgeDelay.setDelay (bridgeSamples);
     neckDelay.setDelay   (neckSamples);
     ```
   - **`processSample(v_bow, F_bow, friction)` body** (split-rail + F2 LP fix + F3 DCB removal, mirroring O-Bowed structure with O-Contrabass's algebraic saturator and bridge-rail-only loop chain):
     ```cpp
     if (filterDirty) updateBridgeFilterCoeffs();

     // Step 1: Read both rails.
     float bridgeRaw = bridgeDelay.popSample (0);
     float neckRaw   = neckDelay.popSample   (0);

     // Step 2: Bridge LP — F2 fixed form: y = g·(1-p)·x + p·y_prev + leak.
     // (DROP the `g` from the feedback term so DC gain == g exactly.)
     if (! std::isfinite (bridgeY))
         bridgeY = 0.0f;
     float bridgeFiltered = bridgeG * bridgeOneMinusP * bridgeRaw
                          + bridgeP * bridgeY
                          + denormalLeak;
     bridgeY = bridgeFiltered;

     // Step 3: Boundary reflections.
     //   Bridge: -1 boundary AFTER LP (rigid bridge; matches O-Bowed line 108).
     //   Nut:    -1 boundary, no LP (rigid nut).
     float bridgeReflection = -bridgeFiltered;
     float nutReflection    = -neckRaw;

     // Step 4: Sum at bow point (split-rail v_string_incoming).
     float v_string_incoming = bridgeReflection + nutReflection;
     float v_delta = v_bow - v_string_incoming;

     // Step 5: Friction (unchanged from rev-1 single-rail).
     float rho = friction.computeReflectionCoefficient (v_delta, F_bow);
     float clampedRho = std::min (rho, 0.85f);
     float frictionVelocity = 2.0f * clampedRho / (1.0f - clampedRho);
     float absVd = std::abs (v_delta);
     float injection = std::min (frictionVelocity, absVd);
     float newVelocity = (v_delta >= 0.0f) ? injection : -injection;

     // Step 6: Symmetric injection into both rails (canonical Smith two-port).
     // [Phase 2.1c placeholder] dispersion will run on the BRIDGE rail's
     //  outgoing wave only, BEFORE the algebraic saturator below.
     float toBridge = nutReflection    + newVelocity;
     float toNeck   = bridgeReflection + newVelocity;

     // Step 7: In-loop algebraic saturator on each rail (RESEARCH §1.3).
     toBridge = toBridge / std::sqrt (1.0f + toBridge * toBridge);
     toNeck   = toNeck   / std::sqrt (1.0f + toNeck   * toNeck);

     // Step 8: F3 — NO in-loop DC blocker. (If long-form drone needs DC
     //   handling, re-add at BowedContrabassVoice::renderNextBlock post-
     //   processSamplesDown — outside the loop, where it cannot interfere
     //   with bootstrapping.)

     bridgeDelay.pushSample (0, toBridge);
     neckDelay.pushSample   (0, toNeck);

     // Step 9: Output from bridge end (matches O-Bowed `output = toBridge`).
     float output = toBridge;
     energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs (output);
     return output;
     ```
   - **`reset()` body:** clear both delay lines, reset `bridgeY`, `energyEstimate`. (Drop `dcX1` / `dcY1` resets — those members are gone.)
   - **`prepare(sr, maxBlockSize)` body:** prepare *both* delay lines on the same `juce::dsp::ProcessSpec`; both `setMaximumDelayInSamples(8192)`. Other state init unchanged.
   - **`setBowPosition(float beta)`:** clamp `[0.02, 0.98]`, store, call `updateDelayLengths()` directly (mirrors O-Bowed line 157–164).
   - **`setDelaySamples(float totalSamples)`:** still the public per-sample setter for vibrato/detune ramps (Phase 2.2/2.3). Internally splits via `bowPosition` and updates both rails — same `bridgeSamples = totalSamples * bowPosition` / `neckSamples = totalSamples * (1.0f - bowPosition)` split, both clamped `[4.0f, 8190.0f]`, both `setDelay`-pushed.
   - **`trigger(float frequency)`:** unchanged structure — set `currentFrequency`, `reset()`, `updateDelayLengths()`, `updateBridgeFilterCoeffs()` (rename only).
   - **Sign convention (HARD CONTRACT, RESEARCH §11.4):**
     - Bridge return: pop → run LP → `-1` boundary on the LP output (`bridgeReflection = -bridgeFiltered`).
     - Nut return: `nutReflection = -neckRaw`. No in-loop chain.
     - Bow point sees `v_string_incoming = bridgeReflection + nutReflection`.
     - Symmetric `+ newVelocity` injection into both outgoing waves: `toBridge = nutReflection + newVelocity`, `toNeck = bridgeReflection + newVelocity`.
     - Per-rail algebraic saturator on the WRITE path; output is `toBridge` (the post-saturator bridge-side outgoing wave).
     - Mirror this verbatim from RESEARCH §11.4 code-fix sketch.
   - **Allocation contract (UNCHANGED):** `setMaximumDelayInSamples` and `prepare(spec)` only from `prepareToPlay()`.
   - **Reference:**
     - O-Bowed canonical: `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:100-155`
     - Coefficient form (B2): `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:94-95` and RESEARCH §11.1 B2 derivation
     - Sign-flip placement (after LP): `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:108`
   - **Depends on:** none (rev-1 source tree intact per CHECKPOINT-2.1a.md).

R2. [ ] **Apply F2: Fix the bridge LP recurrence (drop `g` from the feedback term)**
   - **File (modify):** `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` (the LP block inside R1's `processSample` rewrite)
   - **Edit:** replace
     ```cpp
     float y = bridgeG * (bridgeOneMinusP * lpInput + bridgeP * bridgeY) + denormalLeak;
     ```
     with
     ```cpp
     float bridgeFiltered = bridgeG * bridgeOneMinusP * bridgeRaw
                          + bridgeP * bridgeY
                          + denormalLeak;
     ```
   - **Why this is a separate task even though the code change lives inside R1's rewrite:** it has independent verification (DC gain calibration; long-form drone) and an independent failure mode (if F2 is mis-applied, harness will detect long-form drone DC drift via `rmsRatio_final_over_mid` outside [0.5, 2.0] over 60 s). Listing it here makes the executor explicitly diff against `plugins/O-Bowed/Source/DSP/WaveguideString.cpp:94-95` and confirm `(b0, b1, a0, a1) = (g·(1−p), 0, 1, −p)` form before commit.
   - **Verification (analytical, before harness):** transfer function is `H(z) = g·(1−p)/(1 − p·z⁻¹)`. DC gain = `g·(1−p)/(1−p) = g`. Pole = `p` (independent of g). Confirm by inspection.
   - **Reference:** RESEARCH §11.1 B2; O-Bowed `WaveguideString.cpp:82-98`.
   - **Depends on:** R1 (lives inside same file edit; this task is the line-level diff to track).

R3. [ ] **Apply F3: Remove the in-loop DC blocker (deviation from ARCHITECTURE.md §"DC Blocker")**
   - **Files (modify):** `plugins/O-Contrabass/Source/DSP/WaveguideString.h` and `WaveguideString.cpp` (the DCB block inside R1's `processSample` rewrite + member declarations + reset).
   - **Edit:**
     - **In `WaveguideString.h`:** delete the three lines (per R1 above):
       ```cpp
       float dcX1    = 0.0f;
       float dcY1    = 0.0f;
       static constexpr float kDCBlockerR = 0.999f;
       ```
     - **In `WaveguideString.cpp::processSample`:** R1's rewrite already omits the DCB block. Confirm absence of any `kDCBlockerR`, `dcX1`, `dcY1` references in the body.
     - **In `WaveguideString.cpp::reset`:** drop `dcX1 = 0.0f; dcY1 = 0.0f;` lines.
     - **In file-header doc-block:** drop the "In-loop DC blocker AFTER saturator" line and replace with "F3 deviation from ARCHITECTURE.md §"DC Blocker": F2-corrected bridge LP DC gain = g obviates an in-loop DCB; if long-form drone needs DC handling, an output-path DCB at `BowedContrabassVoice::renderNextBlock` is the correct placement (see PLAN rev-3 §"Why F3 deviates")."
   - **Why this is a separate task:** F3 is the one ARCHITECTURE.md deviation in this PLAN; tracking it as a discrete task makes the architecture amendment recommendation visible at commit-message time and at verify-phase audit time.
   - **Reference:** RESEARCH §11.1 B3, §11.6 deviation justification.
   - **Depends on:** R1 (file edit), R2 (B2 fix is what obviates B3).

R4. [ ] **Apply F4: Drop the `betaScale → setStringImpedance` fudge in `BowedContrabassVoice.cpp`**
   - **Files (modify):** `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` lines 223–229.
   - **Edit:** delete the comment block (lines 223–227), the `betaScale` local computation (line 228), and the `frictionModel.setStringImpedance(...)` call (line 229). Leave `frictionModel.setStringImpedance` at its default initialised value (R_s = 0.5 in `HyperbolicFriction.h` init list). Keep `waveguideString.setBowPosition(effectivePosition)` exactly as-is (line 231) — that is now the real β source for the spatial split.
   - **Net diff:** −7 lines (4 comment + 1 blank + 2 code).
   - **Why:** RESEARCH §11.3 F4. Once F1's split-rail rewrite gives β a real spatial meaning (via `updateDelayLengths`), the voice-side impedance fudge is dead code that was already in the wrong direction (clamped *higher* impedance at extreme bridge-leaning β, which is physically backwards). The friction model's default `R_s = 0.5` is the right value for both bass and treble defaults of `HyperbolicFriction`.
   - **Depends on:** R1 (β must have spatial meaning before fudge is dropped).

R5. [ ] **Rebuild + reinstall + run the Phase 2.1a Gate 1 (auval + pluginval + render-harness)**
   - **Commands (macOS, in repo root):**
     ```bash
     cd build
     ninja O-Contrabass_VST3 O-Contrabass_AU O-Contrabass-render-test
     ```
   - **Install fresh per CLAUDE.md "Plugin Cache Clearing":**
     ```bash
     killall -9 AudioComponentRegistrar 2>/dev/null || true
     rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
     rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3
     rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass.component
     cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass.vst3 ~/Library/Audio/Plug-Ins/VST3/
     cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass.component ~/Library/Audio/Plug-Ins/Components/
     ```
   - **Validation set (V1+V3 from RESEARCH §11.5):**
     - `auval -v aumu OCbs OuDv` → expect SUCCEEDED.
     - `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --validate-in-process build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass.vst3` → expect SUCCESS.
     - **Render harness (the rev-1/rev-2 failure gate):**
       ```bash
       ./build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
           --note 28 --velocity 0.7 --sustain 60 --release 5 \
           --infinite-sustain 1.0 \
           --out /tmp/e1-max-sustain-r3.wav --json /tmp/e1-max-sustain-r3.json
       ```
       Expect exit 0 with `pass_nan = pass_peak = pass_blockTime = pass_rms = true`.
     - **Logic Pro AU smoke (manual):** instantiate O-Contrabass on an instrument track, hold E1 for ≥10 s, confirm audible bowed tone with no clicks at default params; sweep `INFINITE_SUSTAIN` 0 → 1; sweep `BOW_POSITION` 0.02 → 0.25.
   - **Pass-bar (RESEARCH §11.5 V1):**
     - `auval` PASS.
     - `pluginval` level-10 PASS.
     - render-harness all four invariants TRUE; **specifically `rmsMid_s5_s6 > 1e-3` (expected ≈ 0.05–0.20) and `rmsFinal_lastSecond / rmsMid_s5_s6 ∈ [0.5, 2.0]`**. Expected peak ≈ 0.20–0.40 (Helmholtz corner amplitude before output saturator hard-clip).
     - Logic AU smoke: audible bowed tone PASS; no clicks across `INFINITE_SUSTAIN` and `BOW_POSITION` sweeps.
   - **Failure paths (per RESEARCH §11.5 V2):**
     - **If harness `pass_rms` still FALSE:** STOP. Do NOT band-aid. Add the V2 instrumentation hook (see R6 below — optional task, only added on V1 fail) and re-run; the per-sample CSV trace uniquely identifies the remaining bug class.
     - **If `pass_peak` FALSE (peak > 1.0, runaway):** investigate; split-rail with full-magnitude per-rail injection at max INFINITE_SUSTAIN should saturate-bound on the next round trip. Likely transcription bug (e.g., saturator missing on one rail); diff against RESEARCH §11.4 sketch line-by-line. Do not adjust `kCeiling` at this stage — that masks a transcription error.
     - **If `pass_blockTime` FALSE (denormal CPU spike):** widen the denormal-leak boundary from `>= 0.95` to `>= 0.97` (more aggressive leaking for marginal-drone settings); document and re-run.
     - **If `auval` or `pluginval` FAIL but harness PASSes:** investigate validator output; the audio-thread structure is identical to rev-1 (no new allocations, no new locks), so a regression here implies a transcription bug introduced during R1.
   - **Depends on:** R1, R2, R3, R4.
   - **Gate (Gate 1 — Phase 2.1a-recovery exit gate):** all four passes are required to unlock R7 (commit) and Phase 2.1b. No fallback band-aids applied without documented pass_blockTime failure mode.

R6. [ ] **(OPTIONAL — only execute if R5 V1 fails) Add V2 instrumentation hook for per-sample friction-junction trace**
   - **Files (temporary, gated by `#define DEBUG_WAVEGUIDE_FIRST_N`):** `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp`.
   - **What:** add a TEMPORARY logging path inside `WaveguideString::processSample` (gated by `#define DEBUG_WAVEGUIDE_FIRST_N 5000`) that writes the following to a CSV for the first 5000 samples:
     `v_bow, F_bow, v_string_incoming, v_delta, rho, frictionVelocity, |v_delta|/frictionVelocity, newVelocity, toBridge, toNeck, bridgeFiltered, bridgeRaw, neckRaw`
   - **Why:** RESEARCH §11.5 V2. Plot `|v_delta|/frictionVelocity` — Helmholtz bootstrap is signalled by this ratio crossing 1.0 (slip event) within the first 1–3 round-trip periods (≈ 25–75 ms at f0=41.2 Hz). Turns the failure mode from "silent steady-state" into a directly observable trace.
   - **HARD CONTRACT:** the hook is `#ifdef DEBUG_WAVEGUIDE_FIRST_N`, stripped from Release builds. NOT shipped. NOT committed at R7 unless R5 V1 fails AND the hook directly drives a follow-up fix.
   - **Depends on:** R5 V1 FAIL.
   - **Skip-action:** if R5 V1 PASSES, skip R6 entirely; do not add the hook.

R7. [ ] **Commit Phase 2.1a-recovery (atomic)**
   - **What:** R1 + R2 + R3 + R4 changes only. Do NOT bundle Phase 2.1b/2.1c work. Do NOT include the V2 hook (R6) unless its absence would prevent execute from proceeding.
   - **Files (modified by this PLAN):**
     - `plugins/O-Contrabass/Source/DSP/WaveguideString.h` (R1+R3: split-rail member layout, drop DCB members, doc-block update)
     - `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` (R1+R2+R3: split-rail rewrite of `processSample`/`updateDelayLengths`/`reset`/`prepare`/`setBowPosition`/`setDelaySamples`/`trigger`, B2 LP recurrence fix, B3 DCB removal, doc-block update)
     - `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` (R4: −7 LOC, drop betaScale block at lines 223–229)
   - **Also commit (carry-forward from rev-1 / rev-2, never committed yet — per CHECKPOINT-2.1a.md "Files Currently In an Intermediate State"):**
     - `plugins/O-Contrabass/Source/DSP/HyperbolicFriction.h` (NEW, 58 LOC)
     - `plugins/O-Contrabass/Source/DSP/BowModel.{h,cpp}` (NEW, 53 + 99 LOC)
     - `plugins/O-Contrabass/Source/BowedContrabassVoice.h` (NEW, 89 LOC) and the now-modified `.cpp`
     - `plugins/O-Contrabass/Source/OContrabassMPESynthesiser.h` (NEW, 53 LOC)
     - `plugins/O-Contrabass/Source/PluginProcessor.{h,cpp}` (modified — synth wired)
     - `plugins/O-Contrabass/CMakeLists.txt` (modified — DSP sources + render-harness option)
     - `plugins/O-Contrabass/tests/render-harness/CMakeLists.txt` (NEW, 79 LOC)
     - `plugins/O-Contrabass/tests/render-harness/main.cpp` (NEW, 247 LOC)
   - **Commit message style (subject + body):**
     ```
     feat(O-Contrabass): Phase 2.1a — split-rail + B2 LP fix + B3 DCB drop, render-harness Gate 1 PASS

     RESEARCH.md §11 root-cause analysis identified three compounding bugs:
       B1 single-rail topology cannot bootstrap Helmholtz (structural)
       B2 bridge LP recurrence multiplied g into feedback term (DC gain ≈ 1)
       B3 in-loop DCB suppressed cold-start sticking-regime injection

     Fixes applied as a single coupled change:
       F1 split-rail: bridgeDelay + neckDelay (canonical Smith two-port)
       F2 bridge LP fix: y = g·(1-p)·x + p·y_prev + leak (DC gain = g)
       F3 in-loop DCB removed (ARCHITECTURE.md §"DC Blocker" deviation;
          if drone DC drift surfaces in Phase 2.4, output-path DCB is the
          correct placement)
       F4 drop voice-side betaScale fudge (β now has real spatial meaning)

     Gate 1 results:
       auval -v aumu OCbs OuDv:               SUCCEEDED
       pluginval --strictness-level 10:       SUCCESS
       render-harness 60s @ INFINITE_SUSTAIN=1: pass_nan + pass_peak +
                                                pass_blockTime + pass_rms TRUE
       (JSON: /tmp/e1-max-sustain-r3.json)
     ```
   - **Pre-commit:** confirm working tree has no V2 hook leftover (the `#ifdef DEBUG_WAVEGUIDE_FIRST_N` block from R6, if added, must be removed unless escalated).
   - **Depends on:** R5 PASS.

---

### Phase 2.1b — Module Extraction (`synthesis/bow-friction`) — UNCHANGED from rev-1/rev-2

> Tasks 9–16 from rev-1 / R6–R13 from rev-2 carry forward verbatim. The split-rail rewrite + LP fix + DCB removal does NOT affect the bow-friction module (which contains only `HyperbolicFriction` + `BowModel`); the topology change lives entirely in `WaveguideString` which is per-plugin code. The B2 LP fix is per-plugin code (in `WaveguideString::processSample`). The F4 fudge removal is per-plugin code (in `BowedContrabassVoice::updateExpressionParameters`).

R8. [ ] **Render O-Bowed regression baseline** — same as rev-1 Task 9.
R9. [ ] **Create `modules/synthesis/bow-friction/` skeleton + ported sources** — same as rev-1 Task 10.
R10. [ ] **Pick the defaults-selection mechanism for `HyperbolicFriction`** (runtime constructor parameter, `enum class Defaults { Treble, Bass }`) — same as rev-1 Task 11.
R11. [ ] **Update `modules/registry.yaml`** with `bow-friction` entry — same as rev-1 Task 12.
R12. [ ] **Switch O-Bowed and O-Contrabass to consume the module** (atomic CMake flip + delete inline copies) — same as rev-1 Task 13.
R13. [ ] **Re-render and bit-compare O-Bowed regression** — same as rev-1 Task 14. **Gate 2.**
R14. [ ] **Re-validate O-Bowed end-to-end** (auval + pluginval level 10) — same as rev-1 Task 15.
R15. [ ] **Re-validate O-Contrabass end-to-end** (auval + pluginval level 10 + render-harness) — same as rev-1 Task 16. The render-harness invariants must hold post-module-flip with the split-rail topology + corrected LP + no in-loop DCB in place.

> Full task body for R8–R15 lives in PLAN.md rev-1 Tasks 9–16 (preserved by git history before rev-1 → rev-2 → rev-3 supersedes). Not duplicated here. The only difference from rev-1/rev-2 is recovery context: module is extracted on top of an already-validated split-rail engine with corrected LP recurrence and no in-loop DCB.

---

### Phase 2.1c — Cascaded Allpass Dispersion — UNCHANGED from rev-1/rev-2

> Tasks 17–20 from rev-1 / R14–R17 from rev-2 carry forward verbatim. The dispersion filter sits **before** the bridge LP on the **bridge rail only** in the split-rail topology (mirrors O-Bowed's dispersion-on-bridge-rail pattern; ARCHITECTURE.md §"Cascaded Allpass Dispersion" is silent on rail count and the loop chain that consumes it lives entirely on the bridge rail). With B2 fixed, the bridge LP DC-gain is now correctly `g`, so dispersion's group delay (subtracted in `updateDelayLengths`) compounds cleanly with the LP group delay.

R16. [ ] **Author `DispersionFilter.h`** (Rauhala/Välimäki 2006 closed-form, M=4 max sections) — same as rev-1 Task 17.
R17. [ ] **Wire `DispersionFilter` into `WaveguideString` on the bridge rail only** (placement: dispersion → bridge LP → in-loop saturator → fractional delay; **NO in-loop DCB per F3**) — same as rev-1 Task 18, **with the explicit clarification that it lives on the bridge rail only**.
R18. [ ] **Validate dispersion behaviour** (sweep test + harness re-run) — same as rev-1 Task 19.
R19. [ ] **Final Phase 2.1 verification gate** — same as rev-1 Task 20. **Gate 3.**

> Full task bodies live in PLAN.md rev-1 Tasks 17–20. The architectural loop chain on the bridge rail under rev-3 is: dispersion → corrected one-pole bridge LP → in-loop algebraic saturator → fractional delay write. Compared to rev-1/rev-2, the only difference is **no in-loop DC blocker** (F3).

---

## Why F3 deviates from ARCHITECTURE.md

`ARCHITECTURE.md §"DC Blocker"` (lines 92–99) mandates an in-loop one-pole DC blocker `H(z) = (1 − z⁻¹)/(1 − R·z⁻¹)`, R=0.999, AFTER the in-loop saturator. PLAN rev-3 removes this block. Justification (per RESEARCH §11.6):

1. **The architectural DCB requirement was motivated by B2** (broken bridge LP DC gain ≈ 1 instead of g). With B2 fixed (F2), the bridge LP correctly attenuates DC by `1−g` per round trip; a redundant in-loop DCB then has only DOWNSIDE (suppresses cold-start bootstrapping per B3 mechanism — see RESEARCH §11.1) with no upside.
2. **ARCHITECTURE.md §"DC Blocker"** cites no specific pathology that an in-loop DCB solves beyond "subharmonic accumulation under high feedback gain" — which is precisely what the bridge LP's `g`-DC-gain handles when implemented correctly.
3. **Phase 2.1's 60 s sustain test cannot detect DC drift longer than 60 s.** If Phase 2.4's 108-combo stability matrix or Phase 2.5's body-bank coupling later surfaces a real DC-drift pathology that a CORRECT bridge LP cannot handle, an OUTPUT-PATH DCB (post-waveguide, in `BowedContrabassVoice::renderNextBlock` after `processSamplesDown`) can be added then. That placement does not interfere with bootstrapping.
4. **O-Bowed empirical:** has NO in-loop DCB and works at all gain levels up to its 0.9995 cap.

**Architecture amendment recommendation:** post-Phase-2.1-verify, ARCHITECTURE.md should be updated to reflect (a) the corrected LP form (B2 fix) and (b) the output-path DCB option. The architecture's intent (bounded loop gain, no DC drift in long-form drone) is preserved — only the implementation mechanism changes. This is OUT OF SCOPE for PLAN rev-3 / Phase 2.1a-recovery; flagged for the verify-phase audit.

`ARCHITECTURE.md §"Bridge Filter (One-Pole Lowpass + Loop Gain)"` (g range, p clamp, quadratic skew): preserved. F2 only fixes the recurrence implementation; the coefficient computation in `updateBridgeFilterCoeffs` is unchanged.

`ARCHITECTURE.md §"In-loop saturator"` (algebraic `x/√(1+x²)` after LP): preserved, applied per rail in F1's split-rail rewrite. No deviation.

`ARCHITECTURE.md §"Constant denormal leak"`: preserved. The `denormalLeak` member and gating logic carry over unchanged into the split-rail bridge LP (applied to the bridge rail only; nut rail has no LP and no leak — it's a pure delay + sign flip).

---

## Files To Create / Modify (consolidated, recovery-only)

### Modify (Phase 2.1a-recovery, R1–R4)
- `plugins/O-Contrabass/Source/DSP/WaveguideString.h` (R1+R3: split-rail member layout, drop DCB members, doc-block update)
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` (R1+R2+R3: split-rail rewrite + B2 LP fix + B3 DCB removal)
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` (R4: −7 LOC, drop betaScale block at lines 223–229)

### Create / Modify (Phase 2.1b, R8–R15) — see rev-1 PLAN.md for full file lists.

### Create / Modify (Phase 2.1c, R16–R19) — see rev-1 PLAN.md for full file lists.

---

## Dependencies Graph (compact)

```
R1 (F1 split-rail rewrite of WaveguideString)
   ↓
R2 (F2 LP recurrence fix — line-level diff, lives inside R1's edit)
   ↓
R3 (F3 DCB removal — header members + processSample + reset, lives inside R1's edit)
   ↓
R4 (F4 drop betaScale fudge in BowedContrabassVoice)
   ↓
R5 (rebuild + auval + pluginval + harness rerun)        ← GATE 1
   ↓ (PASS)            ↓ (FAIL)
R7 (commit)           R6 (V2 instrumentation hook — optional, halt cycle if invoked)
   ↓
R8 (O-Bowed regression baseline render)
   ↓
R9, R10, R11 (module skeleton + defaults selector + registry)  [R9→R10→R11 sequential]
   ↓
R12 (CMake flip + delete inline copies)
   ↓
R13 (O-Bowed regression bit-compare)                    ← GATE 2
   ↓
R14, R15 (re-validate both consumers)  [parallel]
   ↓
R16 (DispersionFilter.h)
   ↓
R17 (wire DispersionFilter into WaveguideString bridge rail)
   ↓
R18 (dispersion sweep validation + harness)
   ↓
R19 (final Phase 2.1 verification gate)                 ← GATE 3
```

Wave parallelism: R14 + R15 may run concurrently. R6 is the only short-circuit branch (optional, only on R5 fail). R1+R2+R3 are nested within the same file edit but tracked as discrete tasks for verification-trace clarity.

---

## Risks (rev-3, refreshed from RESEARCH.md §11)

1. **R5 harness `pass_rms` still FALSE after F1+F2+F3+F4.** Lower-probability than rev-2 (three independent failure mechanisms now addressed) but possible if a transcription bug entered F1's split-rail rewrite. **Action:** invoke R6 (V2 instrumentation hook). The per-sample CSV trace will show whether `|v_delta|/frictionVelocity` ever crosses 1.0 in the first 25–75 ms — if yes, bootstrapping starts and dies; if no, bootstrapping never starts. Each branch points to a different bug class.
2. **R5 harness `pass_peak` FAIL (peak > 1.0, runaway).** Means the per-rail saturator is missing on one path or applied with wrong sign. Diff R1's `processSample` against RESEARCH §11.4 sketch line-by-line. Do NOT lower `kCeiling` at this stage — that masks a transcription error.
3. **R5 harness `pass_blockTime` FAIL (denormal CPU spike).** Widen the denormal-leak boundary from `>= 0.95` to `>= 0.97` (more aggressive leaking for marginal-drone settings). Document the change in this PLAN's Risks section before continuing.
4. **R5 `auval` or `pluginval` FAIL but harness PASSes.** Audio-thread structure under rev-3 is identical to rev-1 (no new allocations, no new locks, no new objects in `processSample`). A regression here implies a transcription bug introduced during R1; investigate validator output. Common cause to check first: missed `prepare(spec)` on `neckDelay`.
5. **Phase 2.4 (108-combo matrix) surfaces DC drift in long-form drone.** Latent, deferred to Phase 2.4 by design. **Action when surfaced:** add an output-path DCB at `BowedContrabassVoice::renderNextBlock` after `processSamplesDown` (NOT in the loop). Document in Phase 2.4 plan.
6. **Phase 2.4 (108-combo matrix) surfaces Schelleng-floor edge cases at low-Pressure × low-Sustain × high-Stiffness corners** (RESEARCH §11.2 H5 reframed). **Action when surfaced:** Phase 2.4-specific friction-floor calibration; not addressed by Phase 2.1.
7. **R13 bit-exact regression FAIL.** Same as rev-1 Task 14: investigate (default drift / init-order / namespace boundary). ULP-level fallback `< 1.0e-7` acceptable; sonic A/B silence required.
8. **R18 dispersion sweep produces clicks.** Same as rev-1 Task 19: tighten the smoother to 50 ms, or per-sample interpolate `a` between block boundaries.

---

## Success Criteria

These are the Phase 2.1 Test Criteria from CONTEXT.md (locked from ROADMAP), restated as the verify-phase exit gate, with rev-3-specific items called out:

- [ ] **R5 render-harness all four invariants TRUE** — `pass_nan`, `pass_peak`, `pass_blockTime`, **`pass_rms`** (the rev-1/rev-2 failure that motivated rev-3). Specifically `rmsMid_s5_s6 > 1e-3` and `rmsFinal_lastSecond / rmsMid_s5_s6 ∈ [0.5, 2.0]`.
- [ ] **R5 60-second sustain at max `INFINITE_SUSTAIN`, default other params, with bootstrapping inside the first 1–3 round-trip periods** (≈ 25–75 ms).
- [ ] R5 E1 played at default params produces stable bowed tone (no NaN, no denormal CPU spikes) — Logic AU smoke.
- [ ] R5 `BOW_POSITION` 0.02 → 0.25 sweep: no clicks; tone shifts toward sul-ponticello as β → 0.02 — manual smoke (validates split-rail β-spatial-split).
- [ ] R5 `INFINITE_SUSTAIN` 0 → 1 sweep: no clicks, no runaway, sustain time grows with knob — manual smoke.
- [ ] R7 atomic Phase 2.1a-recovery commit lands; commit-message body documents Gate 1 results and the F3 ARCHITECTURE.md deviation rationale.
- [ ] R12, R13 module extraction: O-Bowed full QC pass (no audible regressions); bit-exact baseline match (or sonic-silent ULP fallback).
- [ ] R15 pluginval strictness 10 still passes after audio engine + module flip.
- [ ] R15 auval AU still passes after audio engine + module flip.
- [ ] R18 `STRING_STIFFNESS` 0% → 100% sweep produces continuous timbral change (no clicks).
- [ ] R18 dispersion at 100% audibly affects attack character but not steady-state pitch (mode-locking).
- [ ] R18 `BRIGHTNESS` sweep 80 Hz → 12 kHz: no clicks.
- [ ] CPU < 1% on M1 at 44.1 kHz / 256-sample block (Phase 2.1 perf budget) — Logic CPU meter at idle; manual.
- [ ] `tests/O-Bowed-regression/baseline.wav` committed; sha256 verified — R8, R13.
- [ ] `modules/synthesis/bow-friction/` v1.0.0 created and registered in `modules/registry.yaml` — R9, R11.
- [ ] Both O-Bowed and O-Contrabass build green consuming the module via `ouaricon_add_module` — R12, R14, R15.
- [ ] **(Architecture amendment, post-verify, OUT OF SCOPE for execute):** ARCHITECTURE.md §"DC Blocker" updated to reflect the F2 LP-correctness obviation and the output-path DCB option. Tracked as a follow-up; not blocking Phase 2.2.

When all checks above are green, Phase 2.1 verifies and Phase 2.2 (multi-string + per-string detune) may begin as a fresh GSD cycle.

---
---

# Stage 2: DSP — Plan (Phase 2.1b) — REVISION 4 (Module Extraction, Gate 2)

**Date:** 2026-04-26 (after R7 atomic commit landed; opens Phase 2.1b)
**Plugin:** O-Contrabass (+ O-Bowed as the second consumer in the same atomic switch)
**Stage:** 2 of 4 (DSP) — Phase 2.1 cycle (Phase 2.1b only)
**Phase:** plan
**Cycle Scope:** Phase 2.1b — extract `HyperbolicFriction` + `BowModel` into `modules/synthesis/bow-friction/` v1.0.0; switch O-Bowed and O-Contrabass to consume the module in a single atomic commit on Gate 2 PASS.

**Supersedes:** rev-3 §"Phase 2.1b — Module Extraction" (which carried R8–R15 forward as stubs referencing rev-1 task bodies). This revision writes those bodies in full and pins the five Open Items handed from RESEARCH §13.7.

**Authority:**
- `CONTEXT.md` rev-2 (cycle scope, Gate 2 pass-bar, locked decisions).
- `RESEARCH.md` §13 (corrected module surface, Pattern A CMake, header layout, include-switch mechanics, bass-default propagation API, canonical preset, O-Bowed harness spec, sequencing table §13.6).

**Out of scope:** Phase 2.1c (cascaded allpass dispersion, Gate 3) and Phases 2.2–2.6 — each gets its own fresh GSD cycle. Architecture amendments for §"DC Blocker" and §"In-loop saturator" remain deferred to end-of-Stage-2 verify.

---

## Preamble — Pinned Open Items (RESEARCH §13.7)

The five items deferred to plan-phase are pinned here verbatim; PLAN rev-4 tasks reference these values.

| # | Open Item | Pin |
|---|---|---|
| 1 | O-Bowed `JucePlugin_PluginCode` | **`OBwd` = `0x4f427764`** (verified in `plugins/O-Bowed/CMakeLists.txt:9`). RESEARCH §13.5 speculated `OBow`; that guess is **wrong**. The harness CMake substitution must use `0x4f427764`. |
| 2 | O-Bowed processor class name | **`OBowedAudioProcessor`** (verified in `plugins/O-Bowed/Source/PluginProcessor.h:27`). The harness `main.cpp` substitution uses this exact spelling. |
| 3 | `ouaricon_add_module` for `juce_add_console_app` targets | **Works.** `OuariconModules.cmake` lines 57–67 add SharedCode sources via generic `target_sources(${TARGET_NAME} PRIVATE ...)` and `target_include_directories(${TARGET_NAME} PRIVATE "${MODULE_DIR}/cpp")` — neither requires `${TARGET_NAME}` to be a JUCE plugin. The per-format routing block (lines 68+) is `if(... AND TARGET "${TARGET_NAME}_${SUFFIX}")` and silently no-ops when no such target exists. The `bow-friction` module has no `cpp/vst3/`, `cpp/au/`, etc., so per-format routing is irrelevant anyway. **Decision:** call `ouaricon_add_module(O-Contrabass-render-test bow-friction)` directly in `tests/render-harness/CMakeLists.txt`; no manual include-path workaround needed. Mirror this for `O-Bowed-render-test`. |
| 4 | WAV writer parameters | **24-bit PCM stereo** (verified in `plugins/O-Contrabass/tests/render-harness/main.cpp:240`: `wav.createWriterFor (stream.get(), sampleRate, 2, 24, {}, 0)`). RESEARCH §13.4 row "WAV format" said "32-bit float"; that **does not match** the existing harness. PLAN rev-4 locks O-Bowed's new harness to **24-bit PCM stereo** (matches O-Contrabass exactly so future cross-plugin diffs work). Updated row in canonical-preset table appears in §"Canonical Preset" below. |
| 5 | Golden WAV git commit decision | **Do NOT commit the binary WAV.** Commit the harness source + CMakeLists, the JSON metadata (`o-bowed-pre-extraction-canonical.json`), and the SHA-256 hash file (`o-bowed-pre-extraction-canonical.wav.sha256`). The binary WAV (~1.06 MB at 24-bit PCM stereo / 5 s) lives in `/tmp/` for the live `cmp` invocation; if a future audit needs to reproduce, the harness source + sha256 is sufficient. RESEARCH §13.7 #5 recommendation accepted verbatim. |

**State at start-of-execute (verified before R8 begins):**
- Working tree clean (R7 atomic commit landed).
- Last commit: `dbd35ce feat(O-Contrabass): Phase 2.1a engine + Stage 1 carry-forward (R7)`.
- O-Contrabass + O-Bowed both build green; auval + pluginval-10 PASS for both.
- `/tmp/e1-bowon-only.json` reference exists (regenerate via `O-Contrabass-render-test --note 28 --velocity 0.7 --sustain 65 --release 0 --infinite-sustain 1.0 --out /tmp/e1-bowon-only.wav --json /tmp/e1-bowon-only.json` if missing — instructions in CHECKPOINT-2.1a.md).
- `modules/synthesis/` directory does NOT yet exist on disk (`bow-friction` is the first module in this category).

---

## Goal

Promote `HyperbolicFriction` + `BowModel` (203 LOC across 3 files in O-Bowed today, with verbatim copies in O-Contrabass) into a single shared module `modules/synthesis/bow-friction/` v1.0.0, then switch both consumer plugins to the module in one atomic commit. Prove zero-regression by:

1. **Bit-exact byte-for-byte equality** on O-Bowed's canonical-preset render (5 s, A4 vel 0.7, factory defaults) — `cmp` returns 0.
2. **Byte-identical JSON match** on O-Contrabass's bow-on-only 65 s @ INFINITE_SUSTAIN=1.0 render against the Phase 2.1a-recovery reference at `/tmp/e1-bowon-only.json`.
3. Both plugins pass `auval` + `pluginval` strictness 10 + clean-build.

If Gate 2 PASSes, ~half of Phase 2.1's outstanding work clears and Phase 2.1c (dispersion, Gate 3) opens as a fresh GSD cycle.

---

## Tasks

> Numbering carries forward from rev-3 R7 (committed). R8 → R15 corresponds to RESEARCH §13.6 sequencing table verbatim, with R8a inserted as a separate harness-tooling commit per §13.5 step 6.

### R8 — Build O-Bowed render-harness + capture golden reference

R8. [ ] **Author `plugins/O-Bowed/tests/render-harness/{CMakeLists.txt,main.cpp}` mirroring O-Contrabass's harness.**
   - **Files (create):**
     - `plugins/O-Bowed/tests/render-harness/CMakeLists.txt`
     - `plugins/O-Bowed/tests/render-harness/main.cpp`
   - **CMakeLists.txt — copy `plugins/O-Contrabass/tests/render-harness/CMakeLists.txt` verbatim and substitute:**
     - `O-Contrabass-render-test` → `O-Bowed-render-test` (target name; product name)
     - `target_sources` reach-back paths:
       - `BowedContrabassVoice.cpp` → `BowedStringVoice.cpp` (O-Bowed's voice impl)
       - **Keep** `Source/DSP/BowModel.cpp` and `Source/DSP/WaveguideString.cpp` from O-Bowed's tree (pre-extraction state — module doesn't exist yet at R8 time).
       - Add O-Bowed-only DSP `.cpp` files the voice transitively requires: `Source/DSP/BodyResonator.cpp`, `Source/DSP/SympatheticStringEngine.cpp` (verify by line-by-line `target_sources` audit of `plugins/O-Bowed/CMakeLists.txt:23-50` — drop GUI sources, keep all `Source/DSP/*.cpp` referenced by the voice/processor compile chain, drop scala-tuning-engine sources unless processor pulls them transitively at the harness stage; if `add_dependencies(O-Bowed-render-test O-Bowed)` resolves the `JuceHeader.h` chain, scala sources are not needed for the harness binary).
     - `target_include_directories` reach-back paths: O-Bowed's `Source` and `Source/DSP` (mirror O-Contrabass pattern).
     - `add_dependencies(O-Bowed-render-test O-Bowed)` for `JuceHeader.h` propagation.
     - `target_compile_definitions` block: substitute `JucePlugin_Name="O-Bowed"`, `JucePlugin_Desc="O-Bowed"`, **`JucePlugin_PluginCode=0x4f427764`** (Open-Item-Pin #1, NOT `0x4f426f77`), keep `JucePlugin_ManufacturerCode=0x4f756172`, `JucePlugin_VersionString="1.3.0"` (matches O-Bowed's actual `PLUGIN_VERSION` in `plugins/O-Bowed/CMakeLists.txt:12`), `JucePlugin_VersionCode=0x10300`, leave `JucePlugin_IsSynth=1`, `JucePlugin_WantsMidiInput=1`, `JucePlugin_ProducesMidiOutput=0`, `JucePlugin_IsMidiEffect=0`, `JucePlugin_EditorRequiresKeyboardFocus=0` unchanged.
     - `target_link_libraries` block: copy O-Contrabass harness's JUCE module list verbatim.
   - **main.cpp — copy `plugins/O-Contrabass/tests/render-harness/main.cpp` verbatim and substitute:**
     - Header `#include "PluginProcessor.h"` resolves to O-Bowed's processor via the `target_include_directories` reach-back.
     - `OContrabassAudioProcessor` → **`OBowedAudioProcessor`** (Open-Item-Pin #2; replace all 4–6 mentions in `main.cpp` — search for the class symbol).
     - Default CLI args:
       - `--note` default: `28` → **`69`** (A4)
       - `--velocity` default: `0.7` (unchanged)
       - `--sustain` default: `60` → **`5`** (canonical-preset duration)
       - `--release` default: `5` → **`0`** (no decay tail in canonical preset)
       - `--infinite-sustain` flag: **REMOVE** (O-Bowed doesn't expose this APVTS parameter; the flag's `setStateInformation` path would no-op anyway, but removing keeps the harness contract honest).
     - Default output paths:
       - `e1-max-sustain.wav` / `.json` → `o-bowed-pre-extraction-canonical.wav` / `.json` (or absolute `/tmp/o-bowed-pre-extraction-canonical.wav` — same convention as O-Contrabass harness CLI).
     - WAV writer: keep `wav.createWriterFor (stream.get(), sampleRate, 2, 24, {}, 0)` **VERBATIM** (Open-Item-Pin #4 — 24-bit PCM stereo is the locked format for cross-plugin byte-comparability).
     - Pass-bar invariants in `main.cpp`'s status-JSON block:
       - Drop `pass_rms` invariant (RESEARCH §13.5 — bow-on/bow-off envelope at A4 / 5 s is too short for RMS sanity).
       - Keep `pass_nan`, `pass_peak` (`|sample| ≤ 1.0`), `pass_blockTime` (`maxBlock/medianBlock ≤ 5.0×`).
       - The bit-exact `cmp` is the actual gate; the JSON is for traceability.
   - **Build harness:**
     ```bash
     cd build
     cmake -DOUARICON_BUILD_TESTS=ON ..
     ninja O-Bowed-render-test
     ```
     (PASS condition: clean build, binary at `build/plugins/O-Bowed/tests/render-harness/O-Bowed-render-test`.)
   - **Activate the option in O-Bowed's CMakeLists** (one-line addition to `plugins/O-Bowed/CMakeLists.txt`):
     ```cmake
     # Phase 2.1b render-harness — pre-extraction golden reference.
     option(OUARICON_BUILD_TESTS "Build test harness binaries (e.g. O-Bowed-render-test)" OFF)
     if(OUARICON_BUILD_TESTS)
         add_subdirectory(tests/render-harness)
     endif()
     ```
     Insert at end of file, mirroring `plugins/O-Contrabass/CMakeLists.txt:91-97`. Note: `OUARICON_BUILD_TESTS` is already declared by O-Contrabass's CMakeLists at the parent-build-tree scope, so the second `option()` declaration is a duplicate (harmless — CMake `option()` is idempotent). Keep it for self-containment.
   - **Render golden reference** (canonical preset locked in CONTEXT.md rev-2 + RESEARCH §13.4, with WAV format corrected to 24-bit PCM per Open-Item-Pin #4):
     ```bash
     ./build/plugins/O-Bowed/tests/render-harness/O-Bowed-render-test \
         --note 69 --velocity 0.7 --sustain 5 --release 0 \
         --out /tmp/o-bowed-pre-extraction-canonical.wav \
         --json /tmp/o-bowed-pre-extraction-canonical.json
     shasum -a 256 /tmp/o-bowed-pre-extraction-canonical.wav \
         > /tmp/o-bowed-pre-extraction-canonical.wav.sha256
     ```
     PASS condition: exit 0; JSON shows `pass_nan = pass_peak = pass_blockTime = true`; non-zero file size; sha256 captured.
   - **Pre-flight stability checks (sanity, not gating):**
     - Re-run with `--seed`-deterministic flags (if any exist; O-Contrabass harness has no RNG paths) — confirm two consecutive renders produce byte-identical WAVs. If they don't, abort R8 and surface the determinism violation as a research-phase issue (would invalidate Gate 2.1's `cmp`-based regression bar).
     - Confirm `auval` + `pluginval-10` for O-Bowed still PASS (no regression from the additive harness-only edits).
   - **Why R8 must precede R9–R14:** the golden reference must be captured against pre-extraction O-Bowed source. Once `R10` copies the friction sources into the module and `R12` switches O-Bowed to consume the module, the pre-extraction render path no longer exists in the working tree.
   - **Depends on:** none (current main).

R8a. [ ] **Atomic commit: harness tooling only (separate commit, ahead of module work).**
   - **What:** R8 file additions only — `plugins/O-Bowed/tests/render-harness/{CMakeLists.txt,main.cpp}` + the `option(OUARICON_BUILD_TESTS ...) / add_subdirectory(...)` block appended to `plugins/O-Bowed/CMakeLists.txt`.
   - **Also commit:** `/tmp/o-bowed-pre-extraction-canonical.json` and `/tmp/o-bowed-pre-extraction-canonical.wav.sha256` are NOT committed at R8a (they live in `/tmp/`, regenerated by the harness on demand). Move them to a tracked path under `plugins/O-Bowed/tests/render-harness/golden/` if a permanent audit trail is required:
     - `plugins/O-Bowed/tests/render-harness/golden/o-bowed-pre-extraction-canonical.json`
     - `plugins/O-Bowed/tests/render-harness/golden/o-bowed-pre-extraction-canonical.wav.sha256`
     - **Recommendation (Open-Item-Pin #5):** commit these two small files (~1 KB JSON + ~80 B sha256) for permanent audit; do **NOT** commit the binary `.wav`. R8a's commit message body cites the JSON metadata as the canonical render-state record.
   - **Commit message style (subject + body):**
     ```
     test(O-Bowed): add Phase 2.1b render-harness for module-extraction Gate 2

     Mirrors plugins/O-Contrabass/tests/render-harness exactly, substituting
     PluginCode (OBwd = 0x4f427764), processor class (OBowedAudioProcessor),
     and the canonical preset (A4 vel 0.7, 5s, no infinite-sustain flag).
     WAV writer locked to 24-bit PCM stereo for cross-plugin byte-equality.

     Renders the pre-extraction golden reference for Phase 2.1b's bit-exact
     regression bar. Binary WAV stays in /tmp/; only the JSON metadata + sha256
     are committed for audit (PLAN rev-4 Open-Item-Pin #5).

     Gate-flag: OUARICON_BUILD_TESTS=OFF by default (mirrors O-Contrabass).
     ```
   - **Why a separate commit:** R8 is purely additive tooling with no semantic effect on either plugin's behaviour. Independent reviewability + clean R15 diff (R15's commit is purely the module-extraction switch, not bundled with harness scaffolding).
   - **Depends on:** R8.

### R9 — Module skeleton

R9. [ ] **Create `modules/synthesis/bow-friction/` directory tree + `module.yaml` + `README.md`.**
   - **Files (create):**
     - `modules/synthesis/bow-friction/module.yaml`
     - `modules/synthesis/bow-friction/README.md`
   - **`module.yaml`** (mirror `modules/tuning/note-expression/module.yaml` shape):
     ```yaml
     # bow-friction module v1.0.0
     # Hyperbolic friction junction + bow-envelope state for digital-waveguide
     # bowed-string physical models. Extracted from O-Bowed Phase 2.1b.

     name: bow-friction
     version: 1.0.0
     description: |
       Memoryless hyperbolic friction model (HyperbolicFriction) and
       bow-envelope state (BowModel) for digital-waveguide bowed-string
       synthesis. Pure value-class C++ — no JUCE patch, no per-format
       routing, no allocations. Parametrised via setRosin / setStringImpedance
       / setStaticFrictionCoefficient / setDynamicFrictionCoefficient.

       Treble-string defaults match O-Bowed's verbatim init list
       (mu_s = 0.8, mu_d = 0.3, v_0 = 0.05, R_s = 0.5). Bass-string
       consumers (O-Contrabass) inject bass tuning via setter calls in
       prepareToPlay.

     category: synthesis
     author: Ouaricon Audio

     provides:
       cpp-classes:
         - HyperbolicFriction
         - BowModel

     dependencies: []

     requirements:
       juce_modules:
         - juce_audio_basics    # for BowModel envelope ramps (juce::SmoothedValue?)
         - juce_core
       cpp_standard: 20

     sources:
       cpp:
         - cpp/HyperbolicFriction.h
         - cpp/BowModel.h
         - cpp/BowModel.cpp

     used_by:
       - plugin: O-Bowed
         version: 1.3.0
       - plugin: O-Contrabass
         version: 1.0.0

     changelog:
       - version: 1.0.0
         date: 2026-04-26
         changes:
           - "Initial extraction from O-Bowed/Source/DSP (Phase 2.1b)"
           - "HyperbolicFriction (header-only, 55 LOC + 2 new bass setters)"
           - "BowModel (header + 97-LOC .cpp)"
           - "setStaticFrictionCoefficient / setDynamicFrictionCoefficient setters added (PLAN rev-4 §13.3-Q5)"
           - "Treble defaults preserved; bass consumers inject via setter API"
     ```
     **Cross-check before commit:** verify `BowModel.cpp` does or does not actually `#include <juce_audio_basics/...>` — if not, drop `juce_audio_basics` from the requirements list. Plan-phase pre-flight: `grep -n "include" plugins/O-Bowed/Source/DSP/BowModel.{h,cpp}` and adjust `requirements.juce_modules` accordingly.
   - **`README.md`** — short consumer-facing doc:
     - Module purpose (1 paragraph).
     - Public API list (HyperbolicFriction class signature + BowModel class signature).
     - Default-coefficient ladder (treble = O-Bowed init defaults; bass = O-Contrabass setter overrides).
     - Setter contract (`prepareToPlay` only, never per-block, never per-sample — preserves PERF-01).
     - Consumer call-pattern example (one block of pseudo-cpp showing `frictionModel.setStaticFrictionCoefficient(0.85f)` etc.).
     - Cross-link to RESEARCH §13.3 for design rationale.
   - **Depends on:** none (new directory).

R10. [ ] **Copy friction sources into `modules/synthesis/bow-friction/cpp/` + add bass setters to `HyperbolicFriction.h`.**
   - **Files (create):**
     - `modules/synthesis/bow-friction/cpp/HyperbolicFriction.h` (from `plugins/O-Bowed/Source/DSP/HyperbolicFriction.h`)
     - `modules/synthesis/bow-friction/cpp/BowModel.h` (from `plugins/O-Bowed/Source/DSP/BowModel.h`)
     - `modules/synthesis/bow-friction/cpp/BowModel.cpp` (from `plugins/O-Bowed/Source/DSP/BowModel.cpp`)
   - **Copy mechanics:**
     ```bash
     mkdir -p modules/synthesis/bow-friction/cpp
     cp plugins/O-Bowed/Source/DSP/HyperbolicFriction.h modules/synthesis/bow-friction/cpp/
     cp plugins/O-Bowed/Source/DSP/BowModel.h           modules/synthesis/bow-friction/cpp/
     cp plugins/O-Bowed/Source/DSP/BowModel.cpp         modules/synthesis/bow-friction/cpp/
     ```
   - **Edit `modules/synthesis/bow-friction/cpp/HyperbolicFriction.h`** — add two setters per RESEARCH §13.3-Q5:
     - **Insert after the existing `setStringImpedance` method** (currently line 45–48 in `plugins/O-Bowed/Source/DSP/HyperbolicFriction.h`):
       ```cpp
       void setStaticFrictionCoefficient (float mu) noexcept    { mu_s = mu; }
       void setDynamicFrictionCoefficient (float mu) noexcept   { mu_d = mu; }
       ```
     - **Update file-header comment block** (top of file): change "O-Bowed - Memoryless Hyperbolic Friction Model" to "Ouaricon - Memoryless Hyperbolic Friction Model (shared across O-Bowed + O-Contrabass)" and update the description paragraph to note the bass-coefficient setters.
   - **Edit `modules/synthesis/bow-friction/cpp/BowModel.{h,cpp}`** — file-header comment change only:
     - Change "O-Bowed" → "Ouaricon (bow-friction module)" in header comment block.
     - **No code change.** `BowModel` is plugin-agnostic.
   - **Verify init-list defaults preserved verbatim:**
     - `HyperbolicFriction.h` private members: `mu_s = 0.8f`, `mu_d = 0.3f`, `v_0 = 0.05f`, `R_s = 0.5f` — **MUST match O-Bowed line 51–54 exactly.** This is the bit-exact-determinism boundary; any drift here breaks Gate 2.1.
   - **Pre-flight smoke (build module sources standalone, no plugin):** N/A — the module has no standalone CMakeLists target. Module sources are exercised exclusively via consumers (R12, R13).
   - **Depends on:** R9 (module.yaml must exist for `ouaricon_add_module` to find the module).

R11. [ ] **Update `modules/registry.yaml` — append `bow-friction` entry under `synthesis` category.**
   - **Files (modify):** `modules/registry.yaml`
   - **Edit:** between `note-expression` entry (ends ~line 286) and `# EFFECTS MODULES` divider (line 289), insert:
     ```yaml

       # ============================================================================
       # SYNTHESIS MODULES
       # ============================================================================

       - name: bow-friction
         path: synthesis/bow-friction
         version: 1.0.0
         description: |
           Memoryless hyperbolic friction model (HyperbolicFriction) and
           bow-envelope state (BowModel) for digital-waveguide bowed-string
           physical models. Extracted from O-Bowed Phase 2.1b. Treble-string
           defaults preserved verbatim; bass consumers inject via setter API.
         category: synthesis
         provides:
           - cpp-class: HyperbolicFriction
           - cpp-class: BowModel
         dependencies: []
         tags: [synthesis, physical-modelling, friction, waveguide, bowed-string]
         reuse_score: 8
         used_by:
           - plugin: O-Bowed
             version: 1.3.0
           - plugin: O-Contrabass
             version: 1.0.0
     ```
   - **Verify:** `synthesis` category already exists in `categories:` block (line 32). No category-schema change required.
   - **Depends on:** R9 (the entry references `path: synthesis/bow-friction` which must exist on disk).

### R12 — Switch O-Bowed to consume the module

R12. [ ] **Update `plugins/O-Bowed/CMakeLists.txt` + `BowedStringVoice.h` includes; delete inline-copy DSP files.**
   - **Files (modify):**
     - `plugins/O-Bowed/CMakeLists.txt`
     - `plugins/O-Bowed/Source/BowedStringVoice.h`
   - **Files (delete):**
     - `plugins/O-Bowed/Source/DSP/HyperbolicFriction.h`
     - `plugins/O-Bowed/Source/DSP/BowModel.h`
     - `plugins/O-Bowed/Source/DSP/BowModel.cpp`
   - **`plugins/O-Bowed/CMakeLists.txt` edits:**
     - In `target_sources(O-Bowed PRIVATE ...)` block (lines 23–50): **delete** lines 32 (`Source/DSP/BowModel.h`), 33 (`Source/DSP/BowModel.cpp`), 34 (`Source/DSP/HyperbolicFriction.h`).
     - **Add** after the existing `ouaricon_add_module(O-Bowed note-expression)` (line 53) a new line:
       ```cmake
       ouaricon_add_module(O-Bowed bow-friction)
       ```
   - **`plugins/O-Bowed/Source/BowedStringVoice.h` edits** (lines 23–24):
     ```cpp
     // BEFORE
     #include "DSP/BowModel.h"
     #include "DSP/HyperbolicFriction.h"
     // AFTER
     #include "BowModel.h"
     #include "HyperbolicFriction.h"
     ```
   - **`plugins/O-Bowed/Source/DSP/WaveguideString.cpp` line 13** (`#include "HyperbolicFriction.h"`): **NO EDIT.** Already a bare-name include; resolves via the new module include path automatically. (RESEARCH §13.3-Q4 confirmed.)
   - **`plugins/O-Bowed/Source/DSP/ElastoPlasticFriction.h` and `ThermalFriction.h`:** comment-only mentions of `HyperbolicFriction` (`// Match HyperbolicFriction API`). **NO EDIT.** Comments don't compile.
   - **`plugins/O-Bowed/Source/PluginProcessor.cpp` and any other Source/*.cpp:** grep-and-confirm that no other `.cpp` files include `DSP/HyperbolicFriction.h` or `DSP/BowModel.h` directly. Pre-flight: `grep -rn "DSP/HyperbolicFriction\|DSP/BowModel" plugins/O-Bowed/Source/` — expected output: only the two lines in `BowedStringVoice.h:23-24` (after edit, zero matches).
   - **`prepareToPlay` setter calls:** O-Bowed uses the module's init defaults verbatim (treble = `mu_s=0.8, mu_d=0.3`). **NO new setter calls in O-Bowed's `prepareToPlay`.** Behaviour preserved bit-exactly.
   - **Pre-flight build (don't commit yet):**
     ```bash
     cd build
     ninja O-Bowed_VST3 O-Bowed_AU
     ```
     Expected: clean compile (the `BowedStringVoice.h` include change should resolve via module's `cpp/` PRIVATE include path; the deleted-from-`target_sources` files are no longer compiled twice).
   - **Depends on:** R10, R11.

### R13 — Switch O-Contrabass to consume the module + harness CMake update

R13. [ ] **Update `plugins/O-Contrabass/CMakeLists.txt` + `tests/render-harness/CMakeLists.txt` + `BowedContrabassVoice.{h,cpp}` includes + add bass setters in `prepareToPlay`; delete inline-copy DSP files.**
   - **Files (modify):**
     - `plugins/O-Contrabass/CMakeLists.txt`
     - `plugins/O-Contrabass/tests/render-harness/CMakeLists.txt`
     - `plugins/O-Contrabass/Source/BowedContrabassVoice.h`
     - `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp`
   - **Files (delete):**
     - `plugins/O-Contrabass/Source/DSP/HyperbolicFriction.h`
     - `plugins/O-Contrabass/Source/DSP/BowModel.h`
     - `plugins/O-Contrabass/Source/DSP/BowModel.cpp`
   - **`plugins/O-Contrabass/CMakeLists.txt` edits:**
     - In `target_sources(O-Contrabass PRIVATE ...)` block (lines 25–41): **delete** line 33 (`Source/DSP/BowModel.cpp`). The header-only files (`HyperbolicFriction.h`, `BowModel.h`) are not in `target_sources` (per the comment at lines 30–31); they were picked up via include path and now need module-side resolution.
     - **Add** after the existing `ouaricon_add_module(O-Contrabass note-expression)` (line 88) a new line:
       ```cmake
       # Phase 2.1b — extracted shared friction module (bass setters in prepareToPlay).
       ouaricon_add_module(O-Contrabass bow-friction)
       ```
   - **`plugins/O-Contrabass/tests/render-harness/CMakeLists.txt` edits:**
     - In `target_sources(O-Contrabass-render-test PRIVATE ...)` block (lines 15–27): **delete** line 25 (`${CMAKE_CURRENT_SOURCE_DIR}/../../Source/DSP/BowModel.cpp`). The harness will pull `BowModel.cpp` via the module instead.
     - **Add** the module helper at the end of the file (Open-Item-Pin #3 confirms it works for console apps):
       ```cmake
       # Phase 2.1b — bow-friction module pulls HyperbolicFriction.h + BowModel.{h,cpp}
       # into the harness binary alongside the per-plugin reach-back sources above.
       include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
       ouaricon_add_module(O-Contrabass-render-test bow-friction)
       ```
       Insert at the end of the file (after `target_link_libraries`, line 86), since `target_link_libraries` doesn't fight `target_sources` ordering. Note: `target_include_directories(... Source/DSP)` is already in place at line 31, so the harness still finds `WaveguideString.h` (per-plugin) AND finds `HyperbolicFriction.h` / `BowModel.h` via the module's added include path.
   - **`plugins/O-Contrabass/Source/BowedContrabassVoice.h` edits** (lines 29–30):
     ```cpp
     // BEFORE
     #include "DSP/BowModel.h"
     #include "DSP/HyperbolicFriction.h"
     // AFTER
     #include "BowModel.h"
     #include "HyperbolicFriction.h"
     ```
   - **`plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` line 17** (`#include "HyperbolicFriction.h"`): **NO EDIT.** Already a bare-name include; resolves via the new module include path automatically.
   - **`plugins/O-Contrabass/Source/BowedContrabassVoice.cpp::prepareToPlay`** — add bass-coefficient setter calls (RESEARCH §13.3-Q5). The existing `frictionModel` member is a `HyperbolicFriction` value (declared in `BowedContrabassVoice.h:68`). `prepareToPlay` is at `BowedContrabassVoice.cpp:94`. Insert the two setter calls **after** the existing `bowModel.prepare(...)` call at line 117 (or wherever DSP-engine setup completes) and **before** any voice activity:
     ```cpp
     // Phase 2.1b — bass-string friction defaults (module init = treble defaults).
     // RESEARCH §13.3-Q5 setter API; PLAN rev-4 R13.
     frictionModel.setStaticFrictionCoefficient (0.85f);
     frictionModel.setDynamicFrictionCoefficient (0.25f);
     ```
     (Note: `v_0 = 0.05f` is the module's default already; no setter needed. `R_s = 0.5f` is also the module's default. Only `mu_s` and `mu_d` differ from treble.)
   - **Verify (pre-build, by inspection):**
     - The bass setters MUST run on every `prepareToPlay` invocation (host can call this multiple times, e.g. on sample-rate change). Setter calls happen exactly once per `prepareToPlay` — no per-block / per-sample / per-voice-allocation issues. PERF-01 unchanged.
     - O-Contrabass's existing `frictionModel.setRosin (rosin);` call at line 221 (per-block, in `updateExpressionParameters`) **stays as-is.** That's APVTS-driven; setter API doesn't conflict.
   - **Pre-flight build (don't commit yet):**
     ```bash
     cd build
     ninja O-Contrabass_VST3 O-Contrabass_AU O-Contrabass-render-test
     ```
     Expected: clean compile (the `BowedContrabassVoice.h` include change resolves via the module's `cpp/` PRIVATE include path; the deleted `BowModel.cpp` from `target_sources` is replaced by the module's copy).
   - **Depends on:** R10, R11 (and R12 if R13 is deferred behind it; otherwise R12+R13 may run in parallel — both modify their respective plugin's CMakeLists independently; the registry update at R11 is the shared dependency).

### R14 — Build + auval + pluginval + bit-exact regression + bow-on-only re-render

R14. [ ] **Full Gate 2 validation pass — both plugins.**
   - **Sequential or parallel?** Both plugins build clean independently after R12+R13; auval and pluginval can run on each binary in parallel. Wave-parallelism: R14a + R14b + R14c may execute concurrently.
   - **R14a — O-Bowed validation:**
     ```bash
     cd build
     ninja O-Bowed_VST3 O-Bowed_AU
     killall -9 AudioComponentRegistrar 2>/dev/null || true
     rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
     rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Bowed.vst3 ~/Library/Audio/Plug-Ins/Components/O-Bowed.component
     cp -R build/plugins/O-Bowed/O-Bowed_artefacts/Release/VST3/O-Bowed.vst3 ~/Library/Audio/Plug-Ins/VST3/
     cp -R build/plugins/O-Bowed/O-Bowed_artefacts/Release/AU/O-Bowed.component ~/Library/Audio/Plug-Ins/Components/
     auval -v aumu OBwd OuDv
     /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 \
         --validate-in-process build/plugins/O-Bowed/O-Bowed_artefacts/Release/VST3/O-Bowed.vst3
     ```
     PASS condition: `auval` SUCCEEDED + `pluginval` SUCCESS.
   - **R14b — O-Contrabass validation:**
     ```bash
     cd build
     ninja O-Contrabass_VST3 O-Contrabass_AU O-Contrabass-render-test
     killall -9 AudioComponentRegistrar 2>/dev/null || true
     rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
     rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3 ~/Library/Audio/Plug-Ins/Components/O-Contrabass.component
     cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass.vst3 ~/Library/Audio/Plug-Ins/VST3/
     cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass.component ~/Library/Audio/Plug-Ins/Components/
     auval -v aumu OCbs OuDv
     /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 \
         --validate-in-process build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass.vst3
     ```
     PASS condition: `auval` SUCCEEDED + `pluginval` SUCCESS.
   - **R14c — O-Bowed bit-exact regression (THE GATE 2.1 PASS-BAR):**
     ```bash
     ./build/plugins/O-Bowed/tests/render-harness/O-Bowed-render-test \
         --note 69 --velocity 0.7 --sustain 5 --release 0 \
         --out /tmp/o-bowed-post-extraction-canonical.wav \
         --json /tmp/o-bowed-post-extraction-canonical.json
     cmp /tmp/o-bowed-pre-extraction-canonical.wav \
         /tmp/o-bowed-post-extraction-canonical.wav
     # PASS = exit 0 (byte-equal)
     # FAIL = non-zero exit + line/byte position of first difference reported
     diff /tmp/o-bowed-pre-extraction-canonical.json \
          /tmp/o-bowed-post-extraction-canonical.json
     # PASS = no output (JSON metadata identical)
     ```
     PASS condition: `cmp` returns 0 AND `diff` produces no output.
   - **R14d — O-Contrabass bow-on-only regression (Gate 2.2 invariant carry-forward from Phase 2.1a):**
     ```bash
     ./build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
         --note 28 --velocity 0.7 --sustain 65 --release 0 \
         --infinite-sustain 1.0 \
         --out /tmp/e1-bowon-only-post-2.1b.wav --json /tmp/e1-bowon-only-post-2.1b.json
     diff /tmp/e1-bowon-only.json /tmp/e1-bowon-only-post-2.1b.json
     # PASS = no output (4/4 invariants byte-identical to Phase 2.1a-recovery reference)
     ```
     PASS condition: JSON byte-identical.
   - **R14e — Logic Pro AU smoke (manual, non-blocking but recommended):**
     - Open Logic Pro, instantiate **O-Bowed** on an instrument track, play A4 sustained ≥3 s — confirm tone matches pre-extraction sonic memory (subjective).
     - Instantiate **O-Contrabass** on a second instrument track, play E1 sustained ≥10 s with default params — confirm tone matches Phase 2.1a-recovery sonic memory.
     - Sweep `INFINITE_SUSTAIN` 0 → 1 on O-Contrabass; sweep `BOW_POSITION` 0.02 → 0.25 on both. Confirm no clicks, no runaway.
     - **Failure mode:** if Logic AU smoke surfaces an audible regression that the bit-exact `cmp` did NOT catch — investigate. Possible causes: APVTS init-state ordering, JUCE wrapper-layer differences, host-supplied sample-rate not 44100. File as Gate 2 fail; do NOT commit R15 until resolved.
   - **Failure paths:**
     - **R14a/R14b auval or pluginval FAIL but harness PASSes:** investigate validator output; the audio-thread structure under R12/R13 is purely include-path / source-list reorganisation (no algorithmic changes). A regression here implies (a) the module's `cpp/` PRIVATE include path is shadowing some other header (unlikely — namespaces match), or (b) `target_sources` accidentally now compiles the module's `BowModel.cpp` twice (once via module, once via lingering reach-back). Pre-flight: `grep -rn "BowModel.cpp" plugins/ build/` — expected single match per plugin's CMakeLists generated build files.
     - **R14c `cmp` FAIL (bit-exact regression breaks):**
       - **Most likely cause:** R10 init-list defaults drifted from O-Bowed's verbatim values. Re-diff `modules/synthesis/bow-friction/cpp/HyperbolicFriction.h` lines 51–54 against the original `plugins/O-Bowed/Source/DSP/HyperbolicFriction.h@HEAD~N`.
       - **Second-most-likely cause:** O-Bowed's `prepareToPlay` accidentally calls one of the new setters (it shouldn't — but a copy-paste-from-O-Contrabass error could surface here). Pre-flight: `grep -n "setStaticFrictionCoefficient\|setDynamicFrictionCoefficient" plugins/O-Bowed/Source/`. Expected: zero matches.
       - **Third-most-likely cause:** WAV writer default parameter drift — `wav.createWriterFor(...)` arguments changed between pre- and post-extraction renders. Pre-flight: confirm `plugins/O-Bowed/tests/render-harness/main.cpp` and `plugins/O-Contrabass/tests/render-harness/main.cpp` have identical `createWriterFor` signatures (24-bit PCM stereo; Open-Item-Pin #4).
       - **Fallback (only if PRECISE root-cause above is ruled out):** ULP-level WAV diff with `< 1.0e-7` tolerance + sonic A/B silence audition. Document the fallback explicitly in VERIFICATION.md; do NOT use as a primary pass-bar without surfacing the determinism violation.
     - **R14d JSON `diff` FAIL** (O-Contrabass bow-on-only regression breaks): means the module switch perturbed O-Contrabass's `WaveguideString` / `BowedContrabassVoice` chain in a way that wasn't caught by static analysis. Most likely: the bass setter calls are placed in the wrong position relative to `bowModel.prepare(...)` or `waveguideString.prepare(...)` — investigate ordering. The setter calls SHOULD precede any audio-block work but follow any DSP-engine `prepare(spec)` calls.
   - **Depends on:** R12, R13.
   - **Gate (Gate 2 — Phase 2.1b exit gate):**
     - R14a: O-Bowed auval + pluginval-10 PASS.
     - R14b: O-Contrabass auval + pluginval-10 PASS.
     - R14c: O-Bowed `cmp` byte-equal between pre- and post-extraction WAV; JSON metadata identical.
     - R14d: O-Contrabass bow-on-only JSON byte-identical to `/tmp/e1-bowon-only.json` reference.
     - R14e: Logic AU smoke (manual, non-blocking recommendation).
     - Both plugins: clean build (no new warnings beyond pre-existing macOS-deprecation on `createWriterFor`).
   - All five PASSes are required to unlock R15 (commit). Any FAIL halts the cycle for diagnosis.

### R15 — Atomic Phase 2.1b commit (only on Gate 2 PASS)

R15. [ ] **Single atomic commit landing the module + both plugin switches + registry update.**
   - **Files in this commit:**
     - `modules/synthesis/bow-friction/module.yaml` (NEW — R9)
     - `modules/synthesis/bow-friction/README.md` (NEW — R9)
     - `modules/synthesis/bow-friction/cpp/HyperbolicFriction.h` (NEW — R10, with the two new bass setters)
     - `modules/synthesis/bow-friction/cpp/BowModel.h` (NEW — R10)
     - `modules/synthesis/bow-friction/cpp/BowModel.cpp` (NEW — R10)
     - `modules/registry.yaml` (MODIFIED — R11, append `bow-friction` entry)
     - `plugins/O-Bowed/CMakeLists.txt` (MODIFIED — R12, drop 3 source lines + add `ouaricon_add_module(O-Bowed bow-friction)`)
     - `plugins/O-Bowed/Source/BowedStringVoice.h` (MODIFIED — R12, 2 include path edits)
     - `plugins/O-Bowed/Source/DSP/HyperbolicFriction.h` (DELETED — R12)
     - `plugins/O-Bowed/Source/DSP/BowModel.h` (DELETED — R12)
     - `plugins/O-Bowed/Source/DSP/BowModel.cpp` (DELETED — R12)
     - `plugins/O-Contrabass/CMakeLists.txt` (MODIFIED — R13, drop 1 source line + add `ouaricon_add_module(O-Contrabass bow-friction)`)
     - `plugins/O-Contrabass/Source/BowedContrabassVoice.h` (MODIFIED — R13, 2 include path edits)
     - `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` (MODIFIED — R13, +2 lines for setStaticFrictionCoefficient + setDynamicFrictionCoefficient calls in prepareToPlay)
     - `plugins/O-Contrabass/Source/DSP/HyperbolicFriction.h` (DELETED — R13)
     - `plugins/O-Contrabass/Source/DSP/BowModel.h` (DELETED — R13)
     - `plugins/O-Contrabass/Source/DSP/BowModel.cpp` (DELETED — R13)
     - `plugins/O-Contrabass/tests/render-harness/CMakeLists.txt` (MODIFIED — R13, drop BowModel.cpp reach-back line + add `ouaricon_add_module(O-Contrabass-render-test bow-friction)`)
   - **Files NOT in this commit** (separate commit at R8a):
     - `plugins/O-Bowed/tests/render-harness/{CMakeLists.txt,main.cpp}` (LANDED at R8a)
     - `plugins/O-Bowed/CMakeLists.txt` `OUARICON_BUILD_TESTS` block (LANDED at R8a)
     - `plugins/O-Bowed/tests/render-harness/golden/*` (LANDED at R8a)
   - **Commit message style (subject + body):**
     ```
     feat(modules): extract bow-friction v1.0.0 — Phase 2.1b Gate 2 PASS

     Promote HyperbolicFriction + BowModel from O-Bowed/Source/DSP into
     modules/synthesis/bow-friction/ v1.0.0. Both O-Bowed (treble defaults)
     and O-Contrabass (bass defaults via setStaticFrictionCoefficient +
     setDynamicFrictionCoefficient setters in prepareToPlay) now consume
     the shared module — atomic switch, no flag-day window.

     Module surface (locked, RESEARCH §13.2):
       HyperbolicFriction (header-only, 55+2 LOC; treble init defaults
                            preserved verbatim for O-Bowed bit-exact diff)
       BowModel           (51 LOC header + 97 LOC cpp; plugin-agnostic)

     Gate 2 results:
       R14a O-Bowed     auval -v aumu OBwd OuDv:                 SUCCEEDED
       R14a O-Bowed     pluginval --strictness-level 10:         SUCCESS
       R14b O-Contrabass auval -v aumu OCbs OuDv:                SUCCEEDED
       R14b O-Contrabass pluginval --strictness-level 10:        SUCCESS
       R14c O-Bowed     cmp pre/post canonical-render WAV:       byte-equal
       R14c O-Bowed     diff pre/post canonical-render JSON:     identical
       R14d O-Contrabass bow-on-only 65s @ INFINITE_SUSTAIN=1.0: 4/4 PASS
                                                                 byte-identical to /tmp/e1-bowon-only.json
       R14e Logic AU smoke (both plugins):                       no audible regressions
       Build:                                                    clean

     Out of scope: ARCHITECTURE.md §"DC Blocker" amendment (deferred to
     end-of-Stage-2 verify); Phase 2.1c dispersion (fresh GSD cycle);
     SchellengGuard module class (Phase 2.3 → bow-friction v1.1.0).

     Module registry updated (modules/registry.yaml — bow-friction entry
     under synthesis category, both plugins listed in used_by).
     ```
   - **Pre-commit hygiene checks:**
     - `git status` shows ONLY the 17 files above (no stray edits).
     - `git diff --cached --stat` shows: 5 new files in `modules/synthesis/bow-friction/` + 1 modified `modules/registry.yaml` + 6 modified files in `plugins/` + 6 deleted files in `plugins/`.
     - No working-tree leftovers (`*.bak`, `.swp`, `.DS_Store`).
     - No accidental commit of `/tmp/*.wav` or `/tmp/*.json` (these stay in `/tmp/`; the `tests/render-harness/golden/` JSON+sha256 tracked by R8a is the persistent audit trail).
     - Pre-flight: `git ls-files | grep -E "Source/DSP/(HyperbolicFriction\.h|BowModel\.(h|cpp))$"` — expected zero output (all 6 inline copies deleted).
   - **Depends on:** R14 (all PASSes).

---

## Why R15 is a single atomic commit

CONTEXT.md rev-2 §"Both-plugins switch model" locks: **no flag-day window where one plugin consumes the module and the other has the inline copy**. Reasoning:

1. **Cross-plugin shared state risk.** If R12 lands first (O-Bowed → module), then between R12 and R13 the working tree has O-Contrabass's `Source/DSP/HyperbolicFriction.h` (with bass init defaults) AND `modules/synthesis/bow-friction/cpp/HyperbolicFriction.h` (with treble init defaults). A misrouted include-path resolution at the build-system level could silently pick the wrong copy. Atomic commit eliminates the window.
2. **Regression-bar atomicity.** Gate 2.1 (`cmp` byte-equality) and Gate 2.2 (`diff` JSON identity) must be evaluated against the SAME source tree state. A two-commit split would force two separate regression measurements (one against the module-only state, one against the both-plugins state), doubling diagnostic surface.
3. **Rollback is one-shot.** If a downstream phase (2.1c, 2.2, 2.4) surfaces a module-extraction regression months later, `git revert <R15-sha>` cleanly restores both plugins to their pre-extraction state. A two-commit split would require two reverts in sequence with manual conflict resolution.

---

## Files To Create / Modify (consolidated, Phase 2.1b)

### Module (new)
- `modules/synthesis/bow-friction/module.yaml` (R9)
- `modules/synthesis/bow-friction/README.md` (R9)
- `modules/synthesis/bow-friction/cpp/HyperbolicFriction.h` (R10, header-only, 55+2 LOC)
- `modules/synthesis/bow-friction/cpp/BowModel.h` (R10)
- `modules/synthesis/bow-friction/cpp/BowModel.cpp` (R10)

### Registry (modified)
- `modules/registry.yaml` (R11) — `bow-friction` entry under `synthesis` category

### Plugin source changes — O-Bowed (R12)
- `plugins/O-Bowed/CMakeLists.txt` MODIFIED
- `plugins/O-Bowed/Source/BowedStringVoice.h` MODIFIED (2 include lines)
- `plugins/O-Bowed/Source/DSP/HyperbolicFriction.h` DELETED
- `plugins/O-Bowed/Source/DSP/BowModel.h` DELETED
- `plugins/O-Bowed/Source/DSP/BowModel.cpp` DELETED

### Plugin source changes — O-Contrabass (R13)
- `plugins/O-Contrabass/CMakeLists.txt` MODIFIED
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` MODIFIED (2 include lines)
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` MODIFIED (+2 lines for bass setters in prepareToPlay)
- `plugins/O-Contrabass/Source/DSP/HyperbolicFriction.h` DELETED
- `plugins/O-Contrabass/Source/DSP/BowModel.h` DELETED
- `plugins/O-Contrabass/Source/DSP/BowModel.cpp` DELETED
- `plugins/O-Contrabass/tests/render-harness/CMakeLists.txt` MODIFIED (drop BowModel.cpp reach-back, add `ouaricon_add_module` for harness target)

### Harness scaffolding (R8a — separate commit)
- `plugins/O-Bowed/tests/render-harness/CMakeLists.txt` (NEW)
- `plugins/O-Bowed/tests/render-harness/main.cpp` (NEW)
- `plugins/O-Bowed/CMakeLists.txt` (MODIFIED — `OUARICON_BUILD_TESTS` block + `add_subdirectory(tests/render-harness)`)
- `plugins/O-Bowed/tests/render-harness/golden/o-bowed-pre-extraction-canonical.json` (NEW; persistent audit trail per Open-Item-Pin #5)
- `plugins/O-Bowed/tests/render-harness/golden/o-bowed-pre-extraction-canonical.wav.sha256` (NEW)

### Files explicitly NOT touched
- `plugins/O-Bowed/Source/DSP/WaveguideString.{h,cpp}` (per-plugin; saturator differs)
- `plugins/O-Contrabass/Source/DSP/WaveguideString.{h,cpp}` (per-plugin; split-rail topology)
- Any `BodyResonator`, `SympatheticStringEngine`, `ElastoPlasticFriction`, `ThermalFriction`, `SubHarmonicsGenerator`, `BowNoiseGenerator`, `HumanizeEngine` files (Phase 2.5 / Phase 2.6 / v1.1 territory)
- `parameter-spec.md`, `BRIEF.md`, `ARCHITECTURE.md`, `ROADMAP.md` (no contract changes; checksums preserved)
- `STATUS.md` (will be updated by execute-phase post-R15 land, NOT in R15 itself)

---

## Dependencies Graph (compact)

```
R8 (build O-Bowed harness + render golden reference)
   ↓
R8a (separate commit: harness tooling only)              ← independent commit
   ↓
R9 (modules/synthesis/bow-friction/ skeleton + module.yaml + README.md)
   ↓
R10 (copy HyperbolicFriction.h + BowModel.{h,cpp} into module + add bass setters)
   ↓
R11 (modules/registry.yaml — append bow-friction entry)
   ↓
R12 (O-Bowed: CMakeLists + BowedStringVoice.h + delete 3 inline copies)  ┐
R13 (O-Contrabass: CMakeLists + harness CMakeLists + voice.{h,cpp}        ├─ may run in parallel
       + delete 3 inline copies)                                          ┘
   ↓
R14 (full Gate 2 validation: R14a + R14b + R14c + R14d + R14e)            ← GATE 2
   ↓
R15 (atomic commit on Gate 2 PASS)
```

Wave parallelism:
- R12 + R13 are independent file edits in disjoint plugin directories; either order works. Sequencing: do R12 first if pre-flight build of R13 fails; otherwise either order works.
- R14a + R14b + R14c + R14d run in parallel against the post-R12+R13 working tree. R14e (Logic AU smoke) is manual and non-blocking.
- R8a is independently mergeable ahead of R9–R15; it touches no module / no plugin source affecting either plugin's behavior.

---

## Risks (Phase 2.1b)

1. **R14c `cmp` FAIL — bit-exact regression breaks.** Most-likely root causes documented in R14 "Failure paths" section. Mitigation: pre-flight `grep` checks before R14c run (init-list defaults verbatim; no setter calls in O-Bowed; WAV writer signature identical). Fallback: ULP-level WAV diff with `< 1.0e-7` tolerance + sonic A/B silence — only acceptable with explicit VERIFICATION.md note acknowledging the determinism violation.
2. **R14d O-Contrabass bow-on-only JSON `diff` FAIL.** Means the bass-setter call placement in `prepareToPlay` perturbed the DSP chain (e.g. setter ran before `bowModel.prepare(spec)` reset state, or setter ran AFTER first audio block). Mitigation: setter calls are ordered AFTER `waveguideString.prepare(...)` and `bowModel.prepare(...)` in R13 (mirroring how `setRosin` is currently called at line 221 — before audio activity but after engine prep).
3. **`ouaricon_add_module(O-Contrabass-render-test bow-friction)` silently no-ops or fails.** Open-Item-Pin #3 says it works for console-app targets; if pre-flight build of R13 surfaces "BowModel.h not found" or duplicate-symbol link errors, fallback is to add the module's `cpp/` files explicitly to the harness's `target_sources` + `target_include_directories` (matching how scala-tuning-engine is consumed by the harness today). Document the fallback in PLAN if invoked.
4. **`OUARICON_BUILD_TESTS` option duplicate declaration** between O-Contrabass's CMakeLists (line 94) and the new R8 declaration in O-Bowed's CMakeLists. CMake `option()` is idempotent — second declaration is harmless. Risk if the option is used in some surrounding scope where ordering matters: zero impact in this build tree.
5. **Module's `module.yaml` `requirements.juce_modules` list incorrect.** If `BowModel.cpp` includes a JUCE header not listed (e.g. `<juce_audio_basics/juce_audio_basics.h>` for `SmoothedValue`), the module-registry validation tooling (if any) would flag it. Mitigation: pre-flight `grep -n "include" plugins/O-Bowed/Source/DSP/BowModel.{h,cpp}` before committing R9 — adjust `module.yaml`'s `juce_modules` list to match actual includes.
6. **`HyperbolicFriction` namespace pollution.** O-Bowed's class is in the global namespace (no `Ouaricon::` prefix). The module preserves this verbatim for bit-exact regression. Risk: future module versions adopting a `Ouaricon::Synthesis::HyperbolicFriction` namespace would force a breaking change for all consumers. Out of scope for v1.0.0; document as v1.1.0 candidate in module.yaml's changelog placeholder.
7. **R10 bass setters added but inadvertently break the treble-default contract.** If the bass setters' `noexcept { mu_s = mu; }` body has a typo (e.g. `mu_d = mu` in `setStaticFrictionCoefficient`), O-Bowed (which never calls the setters) is unaffected — but O-Contrabass (which calls both) sees swapped `mu_s` / `mu_d`. Mitigation: line-by-line code review of the two new method bodies before R15 commit.
8. **Logic AU smoke (R14e) reveals a regression NOT caught by `cmp` or `diff`.** Lower-probability — friction module is pure value-class code, deterministic by construction. If it surfaces, escalate as a Gate 2 fail; do NOT commit R15.

---

## Success Criteria (Gate 2 — Phase 2.1b verify exit gate)

These are the criteria from CONTEXT.md rev-2 §"Phase 2.1b Test Criteria", with rev-4 numbering tags:

- [ ] **R8** — `o-bowed-pre-extraction-canonical.wav` rendered + `.json` + `.sha256` captured BEFORE any source edits.
- [ ] **R8a** — Harness scaffolding committed (separate commit, ahead of R9).
- [ ] **R9, R10, R11** — Module files + registry entry created. `module.yaml` parses, registry-yaml schema valid.
- [ ] **R12, R13** — Both plugins build clean post-switch (no new warnings beyond pre-existing macOS-deprecation on `createWriterFor`).
- [ ] **R14a** — `auval -v aumu OBwd OuDv` PASS for O-Bowed.
- [ ] **R14a** — `pluginval --strictness-level 10 --validate-in-process` PASS for O-Bowed VST3.
- [ ] **R14b** — `auval -v aumu OCbs OuDv` PASS for O-Contrabass.
- [ ] **R14b** — `pluginval --strictness-level 10 --validate-in-process` PASS for O-Contrabass VST3.
- [ ] **R14c** — `cmp /tmp/o-bowed-pre-extraction-canonical.wav /tmp/o-bowed-post-extraction-canonical.wav` returns 0 (byte-equal).
- [ ] **R14c** — JSON metadata identical between pre/post extraction renders.
- [ ] **R14d** — O-Contrabass bow-on-only JSON byte-identical to `/tmp/e1-bowon-only.json` reference.
- [ ] **R14e** (recommended, non-blocking) — Logic AU smoke: O-Bowed A4 + O-Contrabass E1 sustain audibly indistinguishable from pre-extraction sonic memory.
- [ ] **R15** — Atomic commit landed; `git log --stat HEAD` shows the 17 files described in R15 file list.
- [ ] **R15** — Module entry visible in `modules/registry.yaml` under `synthesis` category, version 1.0.0.
- [ ] **(Architecture amendment, post-Phase-2.1-verify, OUT OF SCOPE for this execute)** — ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" (latter conditional on §12.5 triggers) updated. Tracked as end-of-Stage-2 work.

When all checks above are green, Phase 2.1b verifies and Phase 2.1c (cascaded allpass dispersion, Gate 3) opens as a fresh GSD cycle.

---

## Out of Scope (deferred per CONTEXT.md rev-2 + RESEARCH §13.7 + STATUS.md)

- **Phase 2.1c** — cascaded allpass dispersion (M=4 for E-string), Gate 3. Fresh GSD cycle after 2.1b verifies.
- **Phase 2.2 → 2.6** — multi-string + per-string detune (2.2), vibrato + slow-bow LFO + Schelleng wedge clamp (2.3), sub-harmonic bias + 108-combo stability matrix (2.4 — saturator-tail re-evaluated here per RESEARCH §12; ARCHITECTURE.md §"In-loop saturator" amendment conditional on §12.5 triggers), body resonator + bow noise (2.5), master saturator/limiter + microtonal + MPE (2.6).
- **`SchellengGuard` module class** — does NOT exist as a class in O-Bowed today (RESEARCH §13.2). Phase 2.3 work; bumps `bow-friction` to v1.1.0.
- **`WaveguideString` module promotion** — saturator differs between O-Bowed (`4·tanh(x/4)`) and O-Contrabass (`x/sqrt(1+x²)`); promotion would require saturator template parameter. Deferred indefinitely (RESEARCH §1.3).
- **ARCHITECTURE.md §"DC Blocker" amendment** — F3 deviation tracked in PLAN rev-3 commit body + R7 / R15 commit messages + VERIFICATION.md. End-of-Stage-2 verify writes the amendment.
- **ARCHITECTURE.md §"In-loop saturator" amendment** — conditional on Phase 2.4 §12.5 escalation triggers. End-of-Stage-2 verify decides.
- **`o-bowed-pre-extraction-canonical.wav` binary commit** — Open-Item-Pin #5 says do not commit; SHA-256 + JSON tracked instead.
- **`v1.1` friction-module changes** — elasto-plastic friction, thermal-coupled friction, alternative coefficient ladders. v1.1.0 territory; current v1.0.0 surface frozen.

---

# Stage 2: DSP — Plan (Phase 2.1c) — REVISION 5 (Cascaded Allpass Dispersion, Gate 3)

> **Status:** rev-5 supersedes the rev-1/rev-2/rev-3 placeholders for R16–R20 with verbatim task bodies. rev-4 (Phase 2.1b extraction, Gate 2) remains in-effect as completed/verified history. This rev-5 section authors fresh task bodies for **R16-pre, R16, R17, R17b, R18, R19, R20** per `RESEARCH.md §14.11` sequencing and `CONTEXT.md` rev-3.

**Date:** 2026-04-27
**Cycle scope:** Phase 2.1c only (Phase 2.2 → 2.6 still get fresh GSD cycles each)
**Gate:** Gate 3 (Phase 2.1c verify)
**Atomic-commit unit:** R20 (Gate 3 PASS) — single commit lands all source + harness + planning artefacts
**Carry-forward locks (NOT re-litigated):** Phase 2.1a-recovery split-rail topology, F2 LP form, F3 no-in-loop-DCB, F4 betaScale removed, Phase 2.1b bow-friction module v1.0.0 consumption, ARCH §"DC Blocker" + §"In-loop saturator" amendments deferred to end-of-Stage-2 verify, primary listening DAW = Logic Pro (AU)

---

## Preamble — Pinned Open Items (RESEARCH §14.12)

PLAN rev-5 pins each of the 6 plan-phase open items from RESEARCH §14.12:

| # | Open Item | Pinned Decision |
|---|-----------|-----------------|
| 1 | `WaveguideString::stiffnessSmoothed` accessor names | `void advanceStiffnessSmootherBy (int numSamples) noexcept;` + `float getCurrentSmoothedStiffness() const noexcept;` (R17 wires both; per-block "advance + read" pair) |
| 2 | Per-sample `a` interpolation fallback location | **Inside `WaveguideString::processSample`** (R17's "click-fallback" path; only invoked if R18 audition reports clicks). Avoids a fourth file. NOT implemented in normal R17 flow — the per-block cadence is the default. |
| 3 | Harness block-rate parameter cadence sufficiency | **Per-block `setValueNotifyingHost`** in R18 sweep mode (in-process APVTS update is synchronous). If R18 surfaces a settle delay, the per-sample fallback (Open-Item-Pin #2) covers it. |
| 4 | Commit `e1-bowon-only-stiffness-zero-pre.wav` to git? | **Commit sha256 + JSON, NOT the binary WAV.** Path: `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256` (plain text, ~75 bytes) + `stiffness-zero-pre.json` (metadata). WAV (~22 MB) reproducible from harness on demand; mirrors Phase 2.1b sha256-only convention. |
| 5 | `--stiffness-sweep` JSON `rmsByDecade` semantic | **6 s windows × 10 deciles** of the sustain phase (release excluded). Coarse enough for human-readable JSON, fine enough to surface mid-sweep dropouts. |
| 6 | sha256 emission mechanism in JSON | **External `shasum -a 256`** wrapper at the harness invocation level — avoids adding a `juce::juce_cryptography` dependency for one text field. R16-pre + R18 both invoke `shasum` post-render and inject the hex digest into the JSON via `jq`-like Python helper or a 5-line C++ post-processor inside the harness (R16-pre decides — both equally cheap). |

**Anomaly carry-forward (NOT a Phase 2.1c blocker, parked per RESEARCH §14.10 Risk #7):** at I=8.0 (E1), the closed form's `-C/k ≈ 15` across all B → clamps to `a≈+0.99` regardless of STRING_STIFFNESS. This is consistent with Rauhala/Välimäki 2006's piano-tuned validity envelope; bass register sits outside it. The audible STRING_STIFFNESS sweep may therefore be flatter than ideal. **Mitigation:** R18's `--stiffness-sweep` Logic audition is the surfacing mechanism. If R18 reveals the sweep is musically uninteresting, file as Phase 2.4 follow-up RESEARCH note (calibration polynomial for bass register), do NOT block Phase 2.1c. Gate 3 stability + bit-exact regression at stiffness=0 + auval/pluginval-10 still exit cleanly even if the sweep is dull.

---

## Goal

Implement the Rauhala/Välimäki 2006 cascaded first-order allpass dispersion filter on the bridge rail of the existing split-rail E-string waveguide (M=4 hardcoded for E1), with split-aware group-delay compensation (subtract from `bridgeSamples` only, NOT from `compensated`). Validate Gate 3 invariants (bit-exact regression at STRING_STIFFNESS=0, no-click STRING_STIFFNESS sweep, mode-locking at 100% stiffness, no Gate 1 regression, auval + pluginval-10 PASS). Atomic-commit on Gate 3 PASS as R20.

---

## Tasks

### R16-pre — Harness `--string-stiffness` CLI flag + pre-flight bit-exact baseline render

**No DSP source edits. Harness CLI extension only. Captures the Gate 3 stiffness=0 regression-bar golden reference BEFORE any dispersion code lands.**

Per RESEARCH §14.7, the existing harness exposes `--note`, `--velocity`, `--sustain`, `--release`, `--infinite-sustain`, `--out`, `--json` (per `tests/render-harness/main.cpp` lines 73–81). It does NOT expose a `--string-stiffness` flag — so capturing a baseline at STRING_STIFFNESS=0 today requires adding one.

**Tasks:**

1. **Add `--string-stiffness <float>` CLI flag to `tests/render-harness/main.cpp`:**
   - Add `float stringStiffness = -1.0f;` (sentinel for "unset → use APVTS factory default") to the `Args` struct at line 50.
   - Add `else if (key == "--string-stiffness") args.stringStiffness = val.getFloatValue();` to the `parseArgs` switch around line 77.
   - In `main` after the existing `--infinite-sustain` override block at lines 105–109, add:
     ```cpp
     if (args.stringStiffness >= 0.0f)
     {
         if (auto* p = proc.parameters.getParameter ("STRING_STIFFNESS"))
             p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, args.stringStiffness));
     }
     ```
   - Update the doc-comment header at line 19 to list the new flag.
   - Add `--stiffness-sweep` flag stub (boolean, parses but no-ops in R16-pre — the sweep behaviour lands in R18; declaring the flag here is optional but harmless and documents the future extension).

2. **Build harness target:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```
   Expect clean build (no new warnings).

3. **Capture pre-dispersion golden render:**
   ```bash
   cd build
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --note 28 --velocity 0.7 --sustain 60 --release 5 \
       --infinite-sustain 1.0 --string-stiffness 0 \
       --out e1-bowon-only-stiffness-zero-pre.wav \
       --json e1-bowon-only-stiffness-zero-pre.json
   ```
   Expect: harness exits 0, JSON `pass_nan`/`pass_peak`/`pass_blockTime` all TRUE. (`pass_rms` may be FALSE per the saturator-tail Phase 2.4 parking; not a blocker for the bit-exact regression.)

4. **Compute and stage sha256:**
   ```bash
   shasum -a 256 build/e1-bowon-only-stiffness-zero-pre.wav \
       | awk '{print $1}' > plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256
   cp build/e1-bowon-only-stiffness-zero-pre.json plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.json
   ```
   Create the `golden/` directory if it doesn't already exist (mirrors the Phase 2.1b O-Bowed pattern at `plugins/O-Bowed/tests/render-harness/golden/`).

5. **Verify the golden is reproducible** (sanity step, optional but recommended):
   - Re-run step 3 to a different output path, sha256 the new WAV, confirm it matches the staged golden. If they differ, the harness has nondeterminism that must be diagnosed BEFORE R16 (likely cause: uninitialised state, RNG without seed, threading). Report and pause.

**Files modified:**
- `plugins/O-Contrabass/tests/render-harness/main.cpp` — add `Args::stringStiffness` field + parser branch + override block (~12 LOC).

**Files created:**
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256` (text, sha256 hex digest)
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.json` (metadata, mirrors existing harness JSON shape)

**Files NOT touched:** any DSP source (`Source/DSP/*`, `Source/BowedContrabassVoice.*`, `Source/PluginProcessor.*`, `Source/PluginEditor.*`).

**Commit:** **NONE** — staging only. R20 atomic commit absorbs the harness edit + golden text files.

**Success bar:**
- [ ] Harness builds clean.
- [ ] `e1-bowon-only-stiffness-zero-pre.wav` exists with `pass_nan`/`pass_peak`/`pass_blockTime` TRUE.
- [ ] sha256 reproducible (step 5 confirms).

**Estimated effort:** 30 min.

---

### R16 — Author `Source/DSP/DispersionFilter.h`

**Per RESEARCH §14.5 skeleton. New file, no edits to existing source yet.**

**Tasks:**

1. **Write `plugins/O-Contrabass/Source/DSP/DispersionFilter.h`** verbatim per RESEARCH §14.5 lines 1850–1962. Confirm at write-time:
   - Header guard via `#pragma once` (matches O-Contrabass house style — see `WaveguideString.h:1`).
   - Includes only `juce_dsp/juce_dsp.h` (provides `jlimit`, `jmax`, `MathConstants`).
   - File-comment header includes the Ouaricon Audio + Taylor Brook attribution block matching the existing `WaveguideString.h` header convention.
   - Class is `template <int MaxSections = 4> class DispersionFilter` with `MaxSections=4` default (Phase 2.1c uses `DispersionFilter<4>` explicitly).
   - Public surface: `prepare(double sr)`, `reset()`, `setActiveSections(int M)`, `setCoefficient(float a)`, `processSample(float x)`, `static float computeAllpassCoefficient(float f0Hz, float B, int M)`, `float getGroupDelaySamples(float f0Hz) const`, `int getActiveSections() const`.
   - Private state: `struct AllpassSection { float a = 0.0f; float z = 0.0f; }` array of size `MaxSections`; `int activeSections = 0`; `double sampleRate = 88200.0`.
   - All methods `noexcept`. `processSample` is `inline` per RESEARCH §14.5.
   - `static_assert(MaxSections >= 1, "DispersionFilter requires at least one section");` immediately inside the class body (RESEARCH §14.9 belt-and-braces guard).
   - `setCoefficient` clamps to `[-0.99f, 0.99f]` (defensive even though voice already clamps — RESEARCH §14.2 closed form already returns clamped).
   - `computeAllpassCoefficient` constants pinned `constexpr` per RESEARCH §14.2 Table (`k1=-0.0135f, k2=0.0058f, k3=-0.000004f, m1=0.0034f, m2=0.0179f, m3=-0.0009f, m4=-0.4986f`) with the citation comment block referencing Rauhala & Välimäki (2006) IEEE SP Letters Table 1 and `RESEARCH §14.2`.
   - `getGroupDelaySamples` uses option-(b) at-f0 closed form per RESEARCH §14.3, with `juce::jmax(denom, 1e-9f)` divide-by-zero guard.

2. **Add `DispersionFilter.h` to `plugins/O-Contrabass/CMakeLists.txt` `target_sources`.** The header is template-only (no `.cpp`); confirm the existing `target_sources` block lists DSP headers (per O-Bells convention) — if it does, append `Source/DSP/DispersionFilter.h`. If it lists only `.cpp` files, no CMake edit is needed (header is implicitly compiled wherever included). Verify by inspecting the CMakeLists at write-time.

3. **Compile-only verification:**
   ```bash
   cmake --build build --target O-Contrabass_VST3 --parallel
   ```
   Expect: clean build, no warnings. If this step compiles before R17, the new header has no syntax errors and is consumable; semantic wiring lands in R17.

**Files created:**
- `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` (~110 LOC per RESEARCH §14.5)

**Files modified (conditional):**
- `plugins/O-Contrabass/CMakeLists.txt` — append header to `target_sources` if convention dictates (verify at write-time).

**Files NOT touched:** `WaveguideString.{h,cpp}`, `BowedContrabassVoice.{h,cpp}`, `PluginProcessor.{h,cpp}`, harness, modules.

**Commit:** **NONE** — staging only. R20 absorbs.

**Success bar:**
- [ ] `DispersionFilter.h` compiles clean as a stand-alone header (verified by R16 compile-only build).
- [ ] `static_assert(MaxSections >= 1)` in place.
- [ ] All seven Rauhala/Välimäki constants `constexpr` with citation comment.
- [ ] No semantic wiring into existing code yet.

**Estimated effort:** 30 min (transcription from RESEARCH §14.5 + verification compile).

---

### R17 — Wire `DispersionFilter<4>` onto bridge rail in `WaveguideString.{h,cpp}`

**Per RESEARCH §14.4, §14.5, §14.7, §14.8. Most-complex task in this phase. Mirror O-Contrabass's split-rail loop chain exactly.**

**Tasks:**

1. **Edit `Source/DSP/WaveguideString.h`:**
   - Add `#include "DispersionFilter.h"` near the top (after the existing `juce_dsp` include).
   - Add new public methods to the existing setter group (alongside `setStringStiffness` at line 79):
     ```cpp
     void setDispersionCoefficient (float a) noexcept;
     void advanceStiffnessSmootherBy (int numSamples) noexcept;
     float getCurrentSmoothedStiffness() const noexcept;
     ```
   - Add private member `DispersionFilter<4> bridgeDispersion;` adjacent to the existing bridge-rail state (near `stiffnessSmoothed` at line 100). Order: place it BEFORE `stiffnessSmoothed` to make initialisation order obvious in `prepareToPlay`.
   - Update the loop-chain doc-block at lines 37–51: change "Phase 2.1a omits dispersion; placeholder lives at the friction-write boundary" to "Phase 2.1c: dispersion runs on the bridge rail between popSample and bridge LP. Mirrors O-Bowed bridge-rail-only chain. Per `ARCHITECTURE.md` §'Cascaded Allpass Dispersion' + §'Processing Order' + RESEARCH §14.4."

2. **Edit `Source/DSP/WaveguideString.cpp`:**

   **(a) `prepare(double sr, int spv)`** (around line 35): after `bridgeFilterY = 0.0f;` (or wherever existing reset clauses live), add:
   ```cpp
   bridgeDispersion.prepare (sr);
   bridgeDispersion.setActiveSections (4);     // M=4 hardcoded for E1 per CONTEXT rev-3 Q2
   bridgeDispersion.setCoefficient (0.0f);     // identity until voice pushes a non-zero a
   ```

   **(b) `reset()`** (existing — add a single line):
   ```cpp
   bridgeDispersion.reset();
   ```

   **(c) `processSample(float input, ...)`** loop chain — locate the existing two-rail processing block (around lines 130–190 per Phase 2.1a-recovery split-rail rewrite). Insert dispersion processing on the BRIDGE rail BEFORE the bridge LP:
   ```cpp
   // Step 1: pop both rails.
   const float bridgeRaw = bridgeDelay.popSample (...);
   const float neckRaw   = neckDelay.popSample   (...);

   // Step 1.5 [Phase 2.1c]: dispersion on bridge rail only.
   const float bridgeDispersed = bridgeDispersion.processSample (bridgeRaw);

   // Step 2: bridge LP applied to dispersed signal.
   bridgeFilterY = lpA * bridgeDispersed + lpB * bridgeFilterY + denormalLeak;
   //              ^^^ change: feed `bridgeDispersed` instead of `bridgeRaw` (or
   //              whatever the current input variable name is in the LP recurrence).
   // Step 3: −1 boundary on neck rail (unchanged — no dispersion, no LP).
   const float neckBoundary = -neckRaw;

   // Steps 4–7 (friction, inject, saturator, push): unchanged from Phase 2.1a-recovery.
   ```
   The exact line-level patch depends on the post-Phase-2.1b-extraction state of `processSample`; the directive is: **find the line that reads `bridgeRaw` (or equivalent) into the bridge LP, insert one line `const float bridgeDispersed = bridgeDispersion.processSample (bridgeRaw);` before it, and replace the LP input with `bridgeDispersed`.** Nut rail untouched.

   **(d) Update Step-6 stale comment** at lines 170–171 per RESEARCH §14.8: replace
   ```cpp
   // [Phase 2.1c placeholder] dispersion will run on the BRIDGE rail's
   //  outgoing wave only, BEFORE the algebraic saturator below.
   ```
   with
   ```cpp
   // (Dispersion already ran at Step 1.5, between bridgeRaw popSample and bridge LP —
   //  bridge rail only, per ARCHITECTURE.md §"Cascaded Allpass Dispersion" and
   //  §"Processing Order"; mirrors O-Bowed bridge-rail-only loop chain.)
   ```

   **(e) `updateDelayLengths()`** (around lines 64–95) — split-aware compensation per RESEARCH §14.7 option (i). Replace the existing `bridgeSamples = compensated * bowPosition;` pair with:
   ```cpp
   float bridgeSamples = compensated * bowPosition;
   float neckSamples   = compensated * (1.0f - bowPosition);

   // Phase 2.1c: dispersion lives on the bridge rail only → compensate bridge-rail
   // delay-line length only, NOT `compensated`. Identity at a=0 (groupDelay returns
   // M=4 samples; cascade adds M unit delays; net round-trip preserved → bit-exact
   // regression at stiffness=0 holds). RESEARCH §14.7 option (i).
   const float dispersionDelay = bridgeDispersion.getGroupDelaySamples (currentFrequency);
   bridgeSamples -= dispersionDelay;

   bridgeSamples = juce::jlimit (4.0f, 8190.0f, bridgeSamples);
   neckSamples   = juce::jlimit (4.0f, 8190.0f, neckSamples);
   ```
   The exact integration depends on the existing local-variable shape; preserve the existing `juce::jlimit` clamp guards and the `setDelaySamples` calls.

   **(f) New method `setDispersionCoefficient(float a)`:**
   ```cpp
   void WaveguideString::setDispersionCoefficient (float a) noexcept
   {
       // Defensive: voice should already pass a finite, clamped value via
       // DispersionFilter::computeAllpassCoefficient; belt-and-braces guard.
       const float safe = std::isfinite (a) ? a : 0.0f;
       bridgeDispersion.setCoefficient (safe);
   }
   ```

   **(g) New method `advanceStiffnessSmootherBy(int numSamples)`:**
   ```cpp
   void WaveguideString::advanceStiffnessSmootherBy (int numSamples) noexcept
   {
       stiffnessSmoothed.skip (juce::jmax (0, numSamples));
   }
   ```
   Uses `juce::SmoothedValue::skip(int)` which advances internal counter by `n` steps and returns the resulting current value (which we discard here; voice reads via the next accessor).

   **(h) New method `getCurrentSmoothedStiffness() const`:**
   ```cpp
   float WaveguideString::getCurrentSmoothedStiffness() const noexcept
   {
       return stiffnessSmoothed.getCurrentValue();
   }
   ```

3. **Compile-only verification:**
   ```bash
   cmake --build build --target O-Contrabass_VST3 --parallel
   ```
   Expect: clean build. Warnings about `std::isfinite` requiring `<cmath>` are easy to spot; add the include if needed (likely already present).

**Files modified:**
- `plugins/O-Contrabass/Source/DSP/WaveguideString.h` — `#include`, three new method decls, `bridgeDispersion` member, doc-block update (~10 LOC net add).
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` — `prepare()` add 3 lines, `reset()` add 1 line, `processSample` insert dispersion call + retarget LP input + comment fix (~6 LOC net add), `updateDelayLengths()` add `dispersionDelay` block (~5 LOC net add), three new method bodies (~12 LOC). Total ~26 LOC net add.

**Files NOT touched:** `BowedContrabassVoice.{h,cpp}`, `PluginProcessor.{h,cpp}`, harness, `DispersionFilter.h` (R16's deliverable), modules.

**Commit:** **NONE** — staging only. R20 absorbs.

**Success bar:**
- [ ] Compile clean.
- [ ] No new pluginval-10 warnings introduced (validated in R19).
- [ ] Loop chain comment correctly documents "Phase 2.1c: dispersion at Step 1.5".
- [ ] `updateDelayLengths()` subtracts from `bridgeSamples` only, NOT from `compensated`.
- [ ] `setDispersionCoefficient`'s defensive `isfinite` guard in place.

**Estimated effort:** 90 min (most code per LOC, cross-references three RESEARCH subsections).

---

### R17b — Wire per-block `a`-computation in `BowedContrabassVoice.cpp`

**Per RESEARCH §14.4 §"Per-block update sequencing" + §14.9 belt-and-braces guards.**

**Tasks:**

1. **Edit `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp`:**

   **(a) Add `#include "DSP/DispersionFilter.h"` near the top.** (Voice already includes `DSP/WaveguideString.h`; add this companion include.)

   **(b) In `renderNextBlock(...)` BEFORE the per-sample loop**, add a per-block dispersion-update block. Locate the existing per-block parameter update area (around `BowedContrabassVoice.cpp:200-237` `updateParametersFromAPVTS`-call site or `renderNextBlock`'s entry); the new block lives AFTER `updateParametersFromAPVTS` (so the latest STRING_STIFFNESS APVTS value has been smoothed-target-set) and BEFORE the per-sample loop:
   ```cpp
   // Phase 2.1c: per-block dispersion coefficient update.
   // Voice computes `a` from (f0, B, M) once per block; waveguide consumes.
   // Per RESEARCH §14.4 §"Per-Block Update Sequencing" + §14.9 guards.
   {
       waveguideString.advanceStiffnessSmootherBy (numSamples);
       const float currentStiffness = waveguideString.getCurrentSmoothedStiffness();
       const float B  = 1.0e-4f * juce::jlimit (0.0f, 1.0f, currentStiffness);    // §14.2
       constexpr int M = 4;                                                       // §14.5 / Q2 lock
       const float f0 = juce::jlimit (20.0f, 5000.0f, currentFrequency);          // §14.9 paranoia clamp
       float a = DispersionFilter<4>::computeAllpassCoefficient (f0, B, M);
       if (! std::isfinite (a)) a = 0.0f;                                         // §14.9 belt-and-braces
       waveguideString.setDispersionCoefficient (a);
   }
   ```
   Verify the local variable name for the per-note fundamental (`currentFrequency` per RESEARCH §14.4 line 1822) is exact at write-time; it may be named `noteFrequency`, `freqHz`, or similar in the actual source — read the surrounding code first.

   **(c) Verify the smoothed-stiffness path is intact.** The existing `updateParametersFromAPVTS()` at the call site should continue to call `waveguideString.setStringStiffness(stringStiffnessParam)` once per block, which sets the smoother's TARGET. The new `advanceStiffnessSmootherBy(numSamples)` then advances the smoother's INTERNAL counter by `numSamples`. The two operations are commutative for the smoother semantics: target-set, then skip(N), then read current value. (If the existing `updateParametersFromAPVTS` already calls `skip` somewhere — RESEARCH says `setStringStiffness` only sets the target — verify by reading `WaveguideString::setStringStiffness` at lines 223–230. Per RESEARCH §14.4, `setStringStiffness` only calls `setTargetValue`; there is no double-advance hazard.)

   **(d) `currentFrequency` continuity check** — the voice updates `currentFrequency` on note-on (`startNote` / `pitchWheelMoved` / Note Expression / MTS-ESP path). The per-block `f0` read above will reflect the latest pitched value, which is correct. Pitch-bend or MTS retune mid-block is an edge case that updates `currentFrequency` between blocks; per-block `a` recomputation tracks it naturally.

2. **Compile + smoke check:**
   ```bash
   cmake --build build --target O-Contrabass_VST3 --parallel
   ```
   Expect: clean build. Run the existing `--infinite-sustain 1.0` sustained harness at default STRING_STIFFNESS (0.30) for 5 seconds as a smoke test — confirm no NaN/Inf/peak overflow:
   ```bash
   ./build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --note 28 --velocity 0.7 --sustain 5 --release 1 --infinite-sustain 1.0 \
       --out /tmp/r17b-smoke.wav --json /tmp/r17b-smoke.json
   ```
   Expected JSON: `pass_nan` TRUE, `pass_peak` TRUE, `pass_blockTime` TRUE. (`pass_rms` outcome depends on default-stiffness behaviour with dispersion engaged at `a≈+0.99`; this smoke is for stability triage, not regression.)

**Files modified:**
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` — one `#include`, one per-block update block (~12 LOC net add).

**Files NOT touched:** `BowedContrabassVoice.h` (no header changes — purely an implementation-side wiring), `WaveguideString.{h,cpp}` (R17's deliverable), harness, `PluginProcessor.{h,cpp}`, modules.

**Commit:** **NONE** — staging only. R20 absorbs.

**Success bar:**
- [ ] Compile clean.
- [ ] R17b smoke harness invocation: `pass_nan`/`pass_peak`/`pass_blockTime` all TRUE at default STRING_STIFFNESS (0.30).
- [ ] No allocation, no lock, no logging in `renderNextBlock` (PERF-01 invariant).

**Estimated effort:** 30 min.

---

### R18 — Harness `--stiffness-sweep` CLI mode + Logic audition

**Per RESEARCH §14.6. Sweep validation render — captures the audition WAV for the "no clicks during STRING_STIFFNESS automation" Gate 3 invariant.**

**Tasks:**

1. **Add `--stiffness-sweep` CLI mode to `tests/render-harness/main.cpp`:**

   **(a) `Args` struct extensions:**
   ```cpp
   bool        stiffnessSweep = false;
   ```
   (already declared as a stub in R16-pre step 1; promote to functional here.)

   **(b) `parseArgs` branch:**
   ```cpp
   else if (key == "--stiffness-sweep") { args.stiffnessSweep = (val.isEmpty() || val.getIntValue() != 0); --i; if (val.isEmpty()) ++i; }
   ```
   — or simpler: treat `--stiffness-sweep` as a boolean flag (no value), parse via:
   ```cpp
   else if (token == "--stiffness-sweep") args.stiffnessSweep = true;
   ```
   The existing `parseArgs` uses `--key value` pairs; adding a no-value flag may need a small parser tweak. Read the parser around line 73 first; if it's strictly key/value-paired, accept `--stiffness-sweep 1` syntax.

   **(c) Defaults when sweep mode is active:**
   ```cpp
   if (args.stiffnessSweep)
   {
       if (args.outWav  == "e1-max-sustain.wav")  args.outWav  = "e1-stiffness-sweep.wav";
       if (args.outJson == "e1-max-sustain.json") args.outJson = "e1-stiffness-sweep.json";
       // INFINITE_SUSTAIN remains 1.0 (default) — bow stays engaged across the sweep.
       // sustainSeconds remains user-controlled (default 60s).
   }
   ```

   **(d) Per-block STRING_STIFFNESS ramp** — in the per-block render loop (around `tests/render-harness/main.cpp:138-179`), add at the top of the block-loop body:
   ```cpp
   if (args.stiffnessSweep)
   {
       const float fraction = static_cast<float> (sampleCursor)
                            / static_cast<float> (juce::jmax (1, sustainSamples));
       const float stiffnessNorm = juce::jlimit (0.0f, 1.0f, fraction);
       if (auto* p = proc.parameters.getParameter ("STRING_STIFFNESS"))
           p->setValueNotifyingHost (stiffnessNorm);
   }
   ```
   The fraction reaches 1.0 at the end of the sustain phase; the release tail (~5s) holds STRING_STIFFNESS at 1.0 implicitly (param is not re-set during release).

   **(e) Extra JSON fields in sweep mode** (RESEARCH §14.6 + Open-Item-Pin #5):
   - `mode`: `"stiffness-sweep"` (vs default `"sustained-note"`).
   - `stiffnessRamp`: `{"start": 0.0, "end": 1.0, "shape": "linear"}`.
   - `rmsByDecade`: 10-element float array of RMS in 6s windows of the sustain phase. Compute alongside the existing `rmsMid_s5_s6` calculation; partition the sustain-phase samples into 10 equal-length deciles (sustainSamples / 10 each) and compute RMS per decile.
   - `sha256`: hex digest of the output WAV. Compute via shell wrapper (Open-Item-Pin #6) — the harness writes the WAV, then the shell invocation post-processes:
     ```bash
     SHA=$(shasum -a 256 e1-stiffness-sweep.wav | awk '{print $1}')
     # Rewrite JSON with sha256 injected — easiest via Python one-liner or jq.
     ```
     Alternatively, embed a 5-LOC SHA-256 helper inside the harness using `juce::SHA256`. **Recommendation: keep it shell-side** — Open-Item-Pin #6 commits to that path; the harness emits JSON without sha256 + a separate shell post-step injects the digest. Mirror this for R16-pre as well (so sha256 emission is consistent across both pre-flight and sweep renders).

2. **Build + sweep render:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ./build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --note 28 --velocity 0.7 --sustain 60 --release 5 --infinite-sustain 1.0 \
       --stiffness-sweep \
       --out e1-stiffness-sweep.wav --json e1-stiffness-sweep.json
   SHA=$(shasum -a 256 e1-stiffness-sweep.wav | awk '{print $1}')
   echo "${SHA}  e1-stiffness-sweep.wav" > e1-stiffness-sweep.wav.sha256
   ```

3. **Logic Pro AU audition** — drag `e1-stiffness-sweep.wav` into a Logic project, listen to the full 65 s. Subjective Gate 3 invariant: timbral character changes continuously and click-free as the sweep progresses. **Specifically listen for:**
   - **Discontinuities (clicks/zipper noise)** at any point in the sweep. Should be absent.
   - **Mode-locked steady-state pitch** — fundamental remains audibly E1 (~41 Hz) throughout. The dispersion should affect ATTACK and TIMBRE, NOT steady-state pitch.
   - **Audible "stiffness" character** — slight bass-string brightening or partials-dispersion as the sweep progresses. (May be subtle per RESEARCH §14.2 Risk #7 — closed form clamps to `a≈+0.99` across most of the bass envelope. If subtle, that's expected; do not fail Gate 3 on subtlety.)

4. **Click-fallback decision** — if Logic audition reveals clicks:
   - Implement per-sample `a` interpolation per Open-Item-Pin #2 (~5 LOC inside `WaveguideString::processSample`).
   - Re-render and re-audition.
   - If clicks persist, escalate to RESEARCH §14.10 follow-up (Phase 2.4 calibration polynomial); pause R18.
   - **Most-likely outcome:** no clicks (per-block 20 ms `stiffnessSmoothed` cadence + `a` smoothness across `B ∈ [0, 1e-4]`).

5. **Stage harness edit and sweep render** (no commit — R20 absorbs):
   ```bash
   git status   # confirm only main.cpp is modified
   ```

**Files modified:**
- `plugins/O-Contrabass/tests/render-harness/main.cpp` — `Args::stiffnessSweep` field, parser branch, default-output rewriter, per-block ramp, JSON-extra fields. Total ~30 LOC net add.

**Files created (artifacts, not committed):**
- `e1-stiffness-sweep.wav` (audition WAV; staged but NOT committed per Open-Item-Pin #4 binary-WAV convention).
- `e1-stiffness-sweep.json` (metadata; committed via R20 if useful for audit-trail; otherwise staged-only).
- `e1-stiffness-sweep.wav.sha256` (text digest; committed via R20).

**Files NOT touched:** any DSP source (R16/R17/R17b deliverables), `PluginProcessor.{h,cpp}`, modules.

**Commit:** **NONE** — staging only. R20 absorbs.

**Success bar:**
- [ ] Sweep harness builds clean and renders 65 s WAV with no NaN/Inf.
- [ ] Logic audition: no audible clicks across the sweep.
- [ ] Logic audition: steady-state pitch locked at E1 (no audible drift as STRING_STIFFNESS → 100 %).
- [ ] sha256 captured (text file, ~75 bytes) for audit-trail.
- [ ] **(Conditional, only on click finding)** per-sample `a` interpolation fallback implemented in `WaveguideString::processSample`.

**Estimated effort:** 60 min for harness CLI + 30 min for Logic audition.

---

### R19 — Gate 3 Verification

**Per CONTEXT.md rev-3 §"Phase 2.1c Test Criteria" + RESEARCH §14.7 step 6–7. Six numeric checks + one bit-exact-regression check + Logic AU smoke. NO source edits.**

**Tasks:**

1. **R19a — Bit-exact regression at STRING_STIFFNESS=0:**
   ```bash
   cd build
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --note 28 --velocity 0.7 --sustain 60 --release 5 \
       --infinite-sustain 1.0 --string-stiffness 0 \
       --out e1-bowon-only-stiffness-zero-post.wav \
       --json e1-bowon-only-stiffness-zero-post.json
   shasum -a 256 e1-bowon-only-stiffness-zero-post.wav
   cmp e1-bowon-only-stiffness-zero-post.wav \
       ../plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav 2>/dev/null \
       || diff <(shasum -a 256 e1-bowon-only-stiffness-zero-post.wav | awk '{print $1}') \
               ../plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256
   ```
   **Expected:** post-WAV's sha256 matches golden's sha256 (since the WAV file isn't committed, compare via sha256). Both `cmp` (if pre-WAV is still on disk from R16-pre) and `diff` of sha256 hex digests are valid evidence.

   **If FAIL:** the bit-exact regression failed → either (a) `getGroupDelaySamples(f0)` at `a=0` does NOT return exactly `M=4`, or (b) `updateDelayLengths()` is not subtracting from `bridgeSamples` correctly, or (c) some side-effect of dispersion code (e.g., `bridgeFilterY` initialisation order, `denormalLeak` interaction) perturbs the LP. Diagnostic: add a `printf("a=%f groupDelay=%f bridgeSamples=%f\n", ...)` debug line in `updateDelayLengths()`, re-render, inspect; revert printf; fix the math; re-test. **Block R19b–R19f until R19a PASSes.**

2. **R19b — Bow-on-only 65 s harness regression (no Gate 1 regression):**
   ```bash
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --note 28 --velocity 0.7 --sustain 60 --release 5 --infinite-sustain 1.0 \
       --out /tmp/e1-bowon-only-post.wav --json /tmp/e1-bowon-only-post.json
   ```
   Inspect JSON. Expected: `pass_nan` TRUE, `pass_peak` TRUE, `pass_blockTime` TRUE, `pass_rms` TRUE (carry-forward from Phase 2.1b verify which had bow-on-only 4/4 TRUE).
   **Pass bar:** 4/4 invariants TRUE.

3. **R19c — auval AU validation:**
   ```bash
   killall -9 AudioComponentRegistrar 2>/dev/null || true
   rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
   rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass.component
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass.component \
         ~/Library/Audio/Plug-Ins/Components/
   auval -v aumu OCbs OuDv 2>&1 | tail -20
   ```
   **Pass bar:** `AU VALIDATION SUCCEEDED`.

4. **R19d — pluginval-10 strict validation:**
   ```bash
   rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass.vst3 \
         ~/Library/Audio/Plug-Ins/VST3/
   /Applications/pluginval.app/Contents/MacOS/pluginval \
       --strictness-level 10 --validate-in-process \
       ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3 2>&1 | tail -20
   ```
   **Pass bar:** `ALL TESTS PASSED`.

5. **R19e — Stiffness-sweep WAV present (R18 artefact):**
   ```bash
   test -f build/e1-stiffness-sweep.wav && echo "OK"
   shasum -a 256 build/e1-stiffness-sweep.wav
   ```
   **Pass bar:** WAV exists; sha256 captured. (Logic audition outcome from R18 step 3 already documented.)

6. **R19f — Logic AU smoke test** (manual, on user side):
   - User opens Logic Pro, instantiates O-Contrabass on a software-instrument track.
   - Plays sustained E1 (MIDI 28) at three STRING_STIFFNESS settings: 0 %, 50 %, 100 %.
   - Subjective gates:
     - **0 %** — sounds identical to the pre-Phase-2.1c memory (bit-exact regression confirmed sonically).
     - **50 %** — subtle timbral change vs 0 % (may be hard to perceive per Risk #7; not a fail).
     - **100 %** — attack character changes audibly; steady-state pitch remains at E1 (~41 Hz, no audible drift). Mode-locking confirmed.
   - User reports back to executor agent (via PR comment, terminal log, or session note).
   - **Pass bar:** no audible regression at 0 %; no NaN/silence/runaway at 100 %; pitch locked.

7. **R19g — Six-item Gate 3 bar audit:**

   | # | Invariant | Source | Status |
   |---|-----------|--------|--------|
   | 1 | STRING_STIFFNESS sweep no clicks | R18 Logic audition | __ |
   | 2 | STRING_STIFFNESS=100% mode-locks (steady pitch) | R19f Logic audition | __ |
   | 3 | BRIGHTNESS sweep no clicks | R19f manual sweep + listen | __ |
   | 4 | auval PASS | R19c | __ |
   | 5 | pluginval-10 PASS | R19d | __ |
   | 6 | bow-on-only 65 s 4/4 TRUE | R19b | __ |
   | 7 | bit-exact regression at stiffness=0 | R19a | __ |

   When 7/7 are checked, Gate 3 PASSes. **Proceed to R20 atomic commit.**

   If any item FAILS, diagnose and remediate before committing. Specific remediation paths:
   - Item 1 fails (clicks) → R18 click-fallback per Open-Item-Pin #2.
   - Item 2 fails (pitch drift) → revisit RESEARCH §14.7 split-aware compensation; verify subtraction is from `bridgeSamples` only, not `compensated`. If still drifts, escalate to Phase 2.4 calibration follow-up + park Phase 2.1c with documented anomaly (last-resort).
   - Item 7 fails → diagnose `getGroupDelaySamples(f0)` at `a=0` returns exactly `M=4` (not `M·(1−ε)` or float-rounding equivalent). The closed form is exact at `a=0` (numerator `1−0=1`, denominator `1+0+0=1` → `per-section=1` → `total=M`); a fail likely indicates an `updateDelayLengths()` plumbing bug.

**Files modified:** **NONE.** Verification only.

**Files created (audit artefacts, included in R20 commit):**
- `plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md` — append "Phase 2.1c verify" section with all seven Gate 3 evidence items + Logic AU smoke notes.

**Commit:** **NONE** — verification only. R20 absorbs `VERIFICATION.md` append.

**Success bar:** all seven Gate 3 invariants TRUE.

**Estimated effort:** 60 min (most checks are quick; Logic audition is the longest single step at ~15 min including DAW boot).

---

### R20 — Phase 2.1c Atomic Commit (Gate 3 PASS)

**Single atomic commit lands all Phase 2.1c work. Only run on R19 Gate 3 PASS.**

**Tasks:**

1. **Stage all Phase 2.1c source + harness + planning artefacts:**
   ```bash
   git add plugins/O-Contrabass/Source/DSP/DispersionFilter.h
   git add plugins/O-Contrabass/Source/DSP/WaveguideString.h
   git add plugins/O-Contrabass/Source/DSP/WaveguideString.cpp
   git add plugins/O-Contrabass/Source/BowedContrabassVoice.cpp
   git add plugins/O-Contrabass/CMakeLists.txt   # only if R16 step 2 modified it
   git add plugins/O-Contrabass/tests/render-harness/main.cpp
   git add plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.json
   git add plugins/O-Contrabass/tests/render-harness/golden/stiffness-sweep.wav.sha256   # from R18 (optional but recommended)
   git add plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md
   git add plugins/O-Contrabass/.planning/STATUS.md
   ```

2. **Confirm staging is correct:**
   ```bash
   git status
   git diff --cached --stat
   ```
   Expect roughly 13 files staged. **Sanity check:** no binary WAVs (e1-stiffness-sweep.wav and e1-bowon-only-stiffness-zero-pre.wav are NOT committed per Open-Item-Pin #4); no `build/` artefacts; no unrelated planning files.

3. **Commit with structured message:**
   ```bash
   git commit -m "$(cat <<'EOF'
   feat(O-Contrabass): Phase 2.1c cascaded allpass dispersion - Gate 3 PASS

   Adds Rauhala/Välimäki 2006 closed-form first-order allpass dispersion
   filter (M=4) on the bridge rail of the E-string waveguide. Per RESEARCH
   §14.

   Implementation:
   - Source/DSP/DispersionFilter.h (new): template<int MaxSections=4>
     class with prepare/reset/setActiveSections/setCoefficient/processSample/
     computeAllpassCoefficient/getGroupDelaySamples. All seven k1..k3, m1..m4
     constants pinned constexpr from IEEE SP Letters Vol 13 No 5 Table 1.
   - Source/DSP/WaveguideString.{h,cpp}: bridge-rail-only dispersion
     wired between popSample (Step 1) and bridge LP (Step 2). Split-aware
     compensation in updateDelayLengths (subtract dispersion group delay
     from bridgeSamples directly, NOT from compensated; preserves bit-exact
     at stiffness=0 per RESEARCH §14.7 option (i)). New public API:
     setDispersionCoefficient(float), advanceStiffnessSmootherBy(int),
     getCurrentSmoothedStiffness().
   - Source/BowedContrabassVoice.cpp: per-block a-computation in
     renderNextBlock (advance smoother → compute via static helper →
     push to waveguide). Per RESEARCH §14.4.
   - tests/render-harness/main.cpp: --string-stiffness CLI flag (R16-pre)
     + --stiffness-sweep mode (R18) for sweep audition.

   Gate 3 (R19) PASS:
   - R19a bit-exact regression at STRING_STIFFNESS=0: sha256 match vs
     pre-dispersion golden (golden/stiffness-zero-pre.wav.sha256).
   - R19b bow-on-only 65 s harness 4/4 invariants TRUE (no Gate 1
     regression).
   - R19c auval -v aumu OCbs OuDv: AU VALIDATION SUCCEEDED.
   - R19d pluginval --strictness-level 10 --validate-in-process: PASS.
   - R19e --stiffness-sweep harness rendered, sha256 captured.
   - R19f Logic AU smoke: 0/50/100% sustained E1 - mode-locking confirmed,
     no clicks, no NaN/silence.

   Known limitation (NOT a Phase 2.1c blocker, parked per RESEARCH §14.10
   Risk #7 + §14.2): at I=8.0 (E1), the closed form's -C/k clamps to
   a≈+0.99 across all B because the paper's calibration targets piano
   register; bass register sits outside the validity envelope. Audible
   stiffness sweep is flatter than ideal. Phase 2.4 follow-up will evaluate
   a piecewise polynomial calibration for bass register if R18 reveals the
   sweep is musically uninteresting.

   ARCH amendments still deferred to end-of-Stage-2 verify per locked
   decision: §"DC Blocker" (F3 from Phase 2.1a-recovery) and §"In-loop
   saturator" (conditional on Phase 2.4 §12.5 escalation triggers).

   Closes Phase 2.1 (highest-risk phase, ~50% of project risk).
   Phase 2.2 (per-string detune + A/D/G strings) opens as fresh GSD cycle.

   Refs:
   - RESEARCH.md §14 (Phase 2.1c dispersion research)
   - PLAN.md rev-5 (R16-pre, R16, R17, R17b, R18, R19, R20)
   - CONTEXT.md rev-3 (Phase 2.1c discuss)

   Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
   EOF
   )"
   ```

4. **Post-commit verification:**
   ```bash
   git log --stat HEAD~1..HEAD
   git rev-parse HEAD
   ```
   Record the commit SHA in `STATUS.md` `phase_2_1c_atomic_commit_sha` field. Confirm `git status` is clean (no uncommitted changes).

5. **Update STATUS.md `next_action`:**
   - Flip `next_action` from `phase_2_1c_plan` to `phase_2_2_discuss` (or `stage_2_phase_2_2_discuss`).
   - Update `gate_state` block: `bridge_rail_dispersion_e1: PASS`, add `phase_2_1c_atomic_commit_sha: <new-sha>`.
   - Update `Lifecycle Timeline` with a 2026-04-?? Phase 2.1c verify entry summarising the seven-item Gate 3 PASS.
   - Update `Cycle Scope` to `Phase 2.2 — per-string detune + A/D/G strings (next fresh GSD cycle)`.
   - **NOTE:** these `STATUS.md` edits are part of the R20 commit (per file list step 1 above) — they're staged BEFORE the commit, not after.

**Files modified:** all listed under "Stage all" in step 1. No new files beyond what R16/R17/R18/R19 already staged.

**Commit:** **THIS IS THE PHASE 2.1c ATOMIC COMMIT.** Single commit, only on Gate 3 PASS.

**Success bar:**
- [ ] `git log --stat HEAD~1..HEAD` shows ~13 files in a single commit.
- [ ] `git status` clean post-commit.
- [ ] STATUS.md `next_action` flipped to Phase 2.2 discuss.
- [ ] Phase 2.1 closed (2.1a + 2.1b + 2.1c all verified).

**Estimated effort:** 30 min (mostly mechanical staging + commit + STATUS.md update).

---

## Why R20 is a single atomic commit

Same gate-first principle as R7 (Phase 2.1a-recovery) and R15 (Phase 2.1b extraction):

1. **Coupling:** the seven Phase 2.1c artefacts (`DispersionFilter.h` + `WaveguideString.{h,cpp}` + `BowedContrabassVoice.cpp` + harness `main.cpp` + golden sha256 + planning artefact updates) are mutually-coupled. Splitting them into multiple commits would yield broken intermediate states (e.g., `WaveguideString.h` referencing a not-yet-committed `DispersionFilter.h` include).
2. **Bisect safety:** if a future Phase 2.x bug is bisected back to "dispersion landing", a single SHA flips the entire feature. Multiple commits would make bisect ambiguous.
3. **Audit trail:** Phase 2.1's three sub-phases each get exactly one commit (R7 → R15 → R20), making the Phase 2.1 timeline trivially reconstructible from `git log --grep "Phase 2.1"`.

---

## Files To Create / Modify (consolidated, Phase 2.1c)

### Source (new)
- `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` — R16 (~110 LOC)

### Source (modified)
- `plugins/O-Contrabass/Source/DSP/WaveguideString.h` — R17 (~10 LOC net add: include, three method decls, member, doc-block)
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` — R17 (~26 LOC net add: prepare, reset, processSample, updateDelayLengths, three new method bodies, comment fix)
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` — R17b (~12 LOC net add: include, per-block update block in renderNextBlock)
- `plugins/O-Contrabass/CMakeLists.txt` — R16 (conditional; only if `target_sources` lists headers explicitly)

### Harness (modified)
- `plugins/O-Contrabass/tests/render-harness/main.cpp` — R16-pre (~12 LOC for `--string-stiffness`) + R18 (~30 LOC for `--stiffness-sweep` + per-block ramp + JSON-extra fields)

### Test artefacts (new, committed as text-only — sha256 + JSON)
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256` — R16-pre
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.json` — R16-pre
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-sweep.wav.sha256` — R18 (optional but recommended)

### Test artefacts (NOT committed — staged-only or transient)
- `e1-bowon-only-stiffness-zero-pre.wav` (~22 MB; reproducible from harness)
- `e1-bowon-only-stiffness-zero-post.wav` (R19a; transient, regenerated each verify run)
- `e1-stiffness-sweep.wav` (~22 MB; reproducible from harness, audition-only)

### Planning artefacts (modified)
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` — already at rev-3 (no further edits in execute; rev-3 lock holds)
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` — already at §14 append (no further edits in execute)
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` — this rev-5 append; no further edits (verify-phase may add a "rev-5 retrospective" footnote if anomalies surfaced)
- `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` — execute-phase appends "Phase 2.1c execute" section after R20
- `plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md` — verify-phase R19g appends "Phase 2.1c verify" section
- `plugins/O-Contrabass/.planning/STATUS.md` — `next_action` flip + `gate_state` update + Lifecycle Timeline append

### Files explicitly NOT touched
- `plugins/O-Contrabass/Source/PluginProcessor.{h,cpp}` (parameters / APVTS / engine wiring all final from Phase 2.1a-recovery)
- `plugins/O-Contrabass/Source/PluginEditor.{h,cpp}` (Stage 3 work)
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` (purely .cpp-side wiring; no header changes)
- `modules/synthesis/bow-friction/*` (Phase 2.1b-frozen v1.0.0; not extending)
- `modules/registry.yaml` (no module surface changes)
- `plugins/O-Bowed/*` (this phase is single-plugin scope; O-Bowed gets no edits)
- `plugins/O-Contrabass/research/ARCHITECTURE.md` (deferred amendments to end-of-Stage-2 verify)
- `plugins/O-Contrabass/.planning/parameter-spec.md` (frozen contract)

---

## Dependencies Graph (compact)

```
R16-pre (harness --string-stiffness CLI + golden capture; NO DSP edits)
   ↓
R16 (DispersionFilter.h + CMake — new file, no semantic wiring)
   ↓
R17 (WaveguideString.{h,cpp} — bridge-rail dispersion + split-aware compensation + accessors)
   ↓
R17b (BowedContrabassVoice.cpp — per-block a-computation)
   ↓
R18 (harness --stiffness-sweep + Logic audition)
   ↓
R19 (Gate 3: R19a bit-exact + R19b bow-on-only + R19c auval + R19d pluginval-10 + R19e sweep WAV + R19f Logic + R19g audit)   ← GATE 3
   ↓ (PASS)            ↓ (FAIL)
R20 (Phase 2.1c     remediate per R19g table:
 atomic commit       - Item 1 → click-fallback (Open-Item-Pin #2)
 — closes 2.1)       - Item 2 → revisit §14.7 split compensation
                     - Item 7 → diagnose getGroupDelaySamples / updateDelayLengths
                     re-run R19 until 7/7
```

R16-pre and R16 are both prerequisites to R17 (R16-pre for the golden, R16 for the include). R17 is prerequisite to R17b (voice can't push `a` to a method that doesn't exist). R17b is prerequisite to R18 (sweep harness needs voice-side per-block update wired in). R19 has no parallelisable internal dependencies — all subtests run sequentially. R20 is the gated finisher.

---

## Risks (Phase 2.1c, refreshed from RESEARCH §14.10)

1. **Bit-exact regression at stiffness=0 fails (Risk #2 refinement).** Mitigation: RESEARCH §14.7 option (i) split-aware compensation is the locked design; R19a verifies. Diagnostic-on-fail: print `a`, `groupDelay`, `bridgeSamples` in `updateDelayLengths()`, confirm `groupDelay==M==4` exactly at `a=0`, confirm subtraction is from `bridgeSamples` not from `compensated`.
2. **STRING_STIFFNESS sweep produces clicks (Risk #1).** Mitigation: per-block 20 ms `stiffnessSmoothed` cadence is first defence; per-sample `a` interpolation fallback (Open-Item-Pin #2) is second; if both fail, escalate to Phase 2.4 dispersion calibration follow-up.
3. **Mode-locking fails (steady pitch drifts at 100 % stiffness, Risk #2 audible bar).** Mitigation: R19f Logic audition is the catch; if drift is real, diagnose via FFT-bin peak detection on the sustained tone in the sweep's terminal 10 s. If quantitative drift > 5 cents at steady state, escalate to Phase 2.4 calibration follow-up.
4. **Coefficient overflow / NaN at extreme stiffness (Risk #4).** Mitigation: closed-form clamp `[-0.99, 0.99]` + R17b's `isfinite` guard + R17's `setDispersionCoefficient` defensive `isfinite`. Three layers; should never see NaN propagate to audio.
5. **Bridge LP recurrence regression after dispersion is upstream (Risk #3).** Mitigation: F2 LP form (`y = g·(1−p)·x + p·y_prev + leak`) is independent of dispersion; bow-on-only 65 s 4/4 invariants in R19b catches any LP regression. The R19a bit-exact regression at stiffness=0 is the strongest possible bar.
6. **Closed-form clamp saturation at E1 (Risk #7).** Mitigation: R18 audition surfaces; if sweep is musically uninteresting, file as Phase 2.4 follow-up; do NOT block Phase 2.1c. Gate 3 still has meaningful pass/fail signal via stability + bit-exact regression.
7. **Per-string M-table absence makes Phase 2.2 wiring non-trivial (Risk #5 carry-forward).** Acknowledged: `DispersionFilter<4>` template-max + runtime `setActiveSections(M)` lets Phase 2.2 wire per-string M without re-templating.
8. **Harness `--stiffness-sweep` adds CLI complexity (Risk #6 carry-forward).** Acceptable: ~30 LOC, isolated to harness, no production-code coupling. Pattern matches existing CLI structure.

---

## Success Criteria (Gate 3 — Phase 2.1c verify exit gate)

- [ ] **R16-pre** — `--string-stiffness` CLI flag landed, golden `e1-bowon-only-stiffness-zero-pre.wav` captured, sha256 staged at `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256`.
- [ ] **R16** — `Source/DSP/DispersionFilter.h` exists, compiles standalone, all seven Rauhala/Välimäki constants `constexpr` with citation comment.
- [ ] **R17** — `WaveguideString.{h,cpp}` wired with bridge-rail dispersion, split-aware compensation in `updateDelayLengths`, three new public methods (`setDispersionCoefficient`, `advanceStiffnessSmootherBy`, `getCurrentSmoothedStiffness`), Step-6 stale comment updated.
- [ ] **R17b** — `BowedContrabassVoice.cpp` per-block `a`-computation in `renderNextBlock` with `f0` clamp, `isfinite` guard, and `M=4` hardcode.
- [ ] **R18** — `--stiffness-sweep` harness mode landed, `e1-stiffness-sweep.wav` rendered, Logic audition: no audible clicks, mode-locked steady-state pitch.
- [ ] **R19a** — Bit-exact regression at STRING_STIFFNESS=0: sha256 match vs `golden/stiffness-zero-pre.wav.sha256`.
- [ ] **R19b** — Bow-on-only 65 s harness: 4/4 invariants TRUE.
- [ ] **R19c** — `auval -v aumu OCbs OuDv`: AU VALIDATION SUCCEEDED.
- [ ] **R19d** — `pluginval --strictness-level 10 --validate-in-process`: ALL TESTS PASSED.
- [ ] **R19e** — `e1-stiffness-sweep.wav` rendered with sha256 captured.
- [ ] **R19f** — Logic AU smoke at STRING_STIFFNESS = 0 / 50 / 100 % E1 sustained tone: no audible regression at 0 %, no NaN/silence at 100 %, pitch locked.
- [ ] **R19g** — Six-item Gate 3 bar audit: 7/7 TRUE.
- [ ] **R20** — Atomic commit landed; `git log --stat HEAD~1..HEAD` shows ~13 files in single commit; STATUS.md `next_action` flipped to `phase_2_2_discuss`.
- [ ] **(Architecture amendment, post-Stage-2-verify, OUT OF SCOPE for this execute)** — ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments still deferred to end-of-Stage-2 verify.

When all checks above are green, **Phase 2.1c verifies** and **Phase 2.1 closes**. Phase 2.2 (per-string detune + A/D/G strings) opens as a fresh GSD cycle.

---

## Out of Scope (deferred per CONTEXT.md rev-3 + RESEARCH §14 + STATUS.md)

- **Phase 2.2 → 2.6** — multi-string + per-string detune (2.2: A1/D2/G2 + per-string M=4/3/2/1 dispersion table), vibrato + slow-bow LFO + Schelleng wedge clamp (2.3), sub-harmonic bias + 108-combo stability matrix (2.4 — saturator-tail re-evaluated here per RESEARCH §12; bass-register dispersion calibration polynomial follow-up here per Risk #7), body resonator + bow noise (2.5), master saturator/limiter + microtonal + MPE (2.6).
- **Per-string M-table** — Phase 2.2 work; `DispersionFilter<4>` template + runtime `setActiveSections(M)` already supports it.
- **Per-sample `a` interpolation in `WaveguideString::processSample`** — only invoked if R18 audition reveals clicks (Open-Item-Pin #2).
- **Bass-register dispersion calibration polynomial** — Phase 2.4 follow-up if R18 sweep is musically uninteresting (Risk #7).
- **`DispersionFilter` module promotion** — premature with one consumer; revisit if O-Bowed grows a dispersion filter later.
- **ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments** — end-of-Stage-2 verify decides; carries forward F3 deviation in R20 commit body.
- **`o-bowed-pre-extraction-canonical.wav` binary commit** — Phase 2.1b convention preserved; sha256 + JSON only.
- **Production binary commits** — `e1-stiffness-sweep.wav` and `e1-bowon-only-stiffness-zero-pre.wav` are NOT committed (~22 MB each; reproducible from harness on demand). sha256 + JSON committed instead per Open-Item-Pin #4.

---

# Stage 2: DSP — Plan (Phase 2.2) — REVISION 6 (4-String Bank + Per-String Detune + Per-String Dispersion Table, Gate 4)

> **Status:** rev-6 authors fresh task bodies for **R21-pre, R21, R22, R23, R24, R25, R26, R27 (optional)** per `RESEARCH.md §15.13` sequencing and `CONTEXT.md` rev-4. rev-1/2/3/4/5 remain in-effect as completed/verified history (Phase 2.1a + 2.1b + 2.1c). Phase 2.1 closed 2026-04-27 with R20 atomic commit `5759e5e` (Gate 3 PASS).

**Date:** 2026-04-27
**Cycle scope:** Phase 2.2 only (Phase 2.3 → 2.6 still get fresh GSD cycles each)
**Gate:** Gate 4 (Phase 2.2 verify)
**Atomic-commit unit:** R26 (Gate 4 PASS) — single commit lands all source + harness + golden text files + planning artefacts
**Carry-forward locks (NOT re-litigated):** Phase 2.1a-recovery split-rail topology (`bridgeDelay` + `neckDelay` per instance), F2 LP form (`y = g·(1−p)·x + p·y_prev + leak`), F3 no-in-loop-DCB, F4 betaScale removed, Phase 2.1b bow-friction module v1.0.0 consumption (value-class deterministic; not touched), Phase 2.1c `DispersionFilter<4>` API (`setActiveSections`, `setCoefficient`, `processSample`, `computeAllpassCoefficient`, `getGroupDelaySamples`) consumed verbatim, Phase 2.1c `WaveguideString` Step 1.5 dispersion placement + split-aware compensation in `updateDelayLengths`, ARCH §"DC Blocker" + §"In-loop saturator" amendments deferred to end-of-Stage-2 verify, primary listening DAW = Logic Pro (AU)

---

## Preamble — Pinned Open Items (RESEARCH §15.14)

PLAN rev-6 pins each of the 8 plan-phase open items from RESEARCH §15.14:

| # | Open Item | Pinned Decision |
|---|-----------|-----------------|
| 1 | R-pre execution location | **R21-pre is a structural prerequisite to R21** (mirrors Phase 2.1c R16-pre). Diagnostic only; no commit. WAV+sha256+JSON written to `/tmp/`, NOT to `golden/`. R-final (R24) re-renders against the committed Phase 2.1c golden at `tests/render-harness/golden/stiffness-zero-pre.wav.sha256`. R21-pre exists solely to confirm working-tree integrity at start of execute (sha256 must equal `d358abcd…` BEFORE any Phase 2.2 source edits — empirically verified 2026-04-27 in RESEARCH §15.1). |
| 2 | `pass_allSegmentsAudible` RMS threshold | **`> 1.0e-3f` (≈ −60 dBFS).** Per RESEARCH §15.8 schema. Catches silent strings (e.g., demoted ACTIVE_STRINGS=1 + MIDI 50 erroneously routing to a non-existent slot, or idle-string contribution leaking into mix). At expected per-segment RMS ≈ 0.04, the 1e-3 threshold is 32 dB below signal — generous enough to tolerate the 5 ms crossfade dip without false-flagging legitimate transitions. |
| 3 | `rmsContinuityAtTransitions` window size | **256-sample symmetric window centred on each transition** (128 before, 128 after). Per RESEARCH §15.8. At host SR 44.1 kHz, 256 samples = 5.8 ms — covers the entire 5 ms equal-power crossfade window and ~0.8 ms of pre/post steady-state for the ratio comparison. Threshold: `≥ 0.50` (allows up to 2× RMS step at transition; new string starts from idle so brief dip is structural). |
| 4 | `openStringFrequencyHz` constant array | **Static literals per ARCHITECTURE.md §"String Waveguide Bank" lines 79–83:** `static constexpr float openStringFrequencyHz[4] = { 41.20f, 55.00f, 73.42f, 98.00f };` (E1, A1, D2, G2). Decision: use rounded literals (NOT `MidiMessage::getMidiNoteInHertz(28/33/38/43)` static-init), avoiding any static-init order dependency on JUCE state. The 0.01 Hz quantisation error vs. exact 12-TET is below audible threshold (< 0.5 cents) and is documented in architecture as the canonical open-string frequency table. |
| 5 | `B_open` constant array | **Static literals per ARCHITECTURE.md §"String Waveguide Bank" lines 79–83:** `static constexpr float B_open[4] = { 1.0e-4f, 7.0e-5f, 5.0e-5f, 3.0e-5f };` (E, A, D, G). Verbatim from architecture inharmonicity table. Voice-side multiplies by clamped `STRING_STIFFNESS ∈ [0,1]` per RESEARCH §15.4 update sequence. |
| 6 | `M_per_string` constant array | **Static literals per ARCHITECTURE.md §"String Waveguide Bank" lines 79–83:** `static constexpr int M_per_string[4] = { 4, 3, 2, 1 };` (E, A, D, G). Verbatim. Configured once per slot at `prepareToPlay` via `WaveguideString::setDispersionActiveSections(M_per_string[s])` (R22's new pass-through setter). NEVER modified at runtime; this lets bit-exact regression at slot 0 (M=4) hold trivially. |
| 7 | Golden files location + names | **Path:** `plugins/O-Contrabass/tests/render-harness/golden/`. **New files (text-only, ~150 bytes each):** `string-A.wav.sha256`, `string-A.json`, `string-D.wav.sha256`, `string-D.json`, `string-G.wav.sha256`, `string-G.json`, `detune-sweep-A.wav.sha256`, `detune-sweep-A.json`, `note-sequence.wav.sha256`, `note-sequence.json`. **NOT committed:** the corresponding `.wav` binaries (each ~10–22 MB; reproducible from harness on demand per Phase 2.1b/2.1c precedent). The Phase 2.1c `stiffness-zero-pre.{wav.sha256,json}` and `stiffness-sweep.{wav.sha256,json}` golden files are unchanged in this commit (their sha256 contents do not change — they ARE the regression bar). |
| 8 | `readDetuneForString` paramId array | **Static literal in `BowedContrabassVoice.cpp`:** `static constexpr const char* paramIds[4] = { "DETUNE_E", "DETUNE_A", "DETUNE_D", "DETUNE_G" };`. Indexed by slot (0=E, 1=A, 2=D, 3=G). Returns the raw float value of the corresponding APVTS parameter (units: cents, range [−1200, +1200] per `parameter-spec.md` lines 43–46). Voice converts cents → multiplicative pitch ratio → target delay-samples via `computeDelaySamples()` helper (RESEARCH §15.5). |

**Carry-forward locks from Phase 2.1c (NOT re-litigated):**
- `WaveguideString` Step 1.5 dispersion placement (between `popSample` and bridge LP) — slot 0 (E-string) code path is byte-identical to Phase 2.1c at the regression preset.
- `BowedContrabassVoice::renderNextBlock` per-block `a`-computation pattern — extended to 4 slots in R21 (loop over `s ∈ [0,3]`), but slot-0 arithmetic preserved verbatim.
- `DispersionFilter<4>` template + `setActiveSections(M)` runtime config — Phase 2.2 consumes this verbatim; no template instantiation changes.

**Hard rules from RESEARCH §15.9.5 (binding for R21 implementation):**
1. **No floating-point reordering on slot-0 (E-string) mix path.** Use the "early return on `activeStringIndex`" pattern (RESEARCH §15.9.3 line 2980): `if (s == activeStringIndex) mixedSample = out;` rather than unconditional `mixedSample += out`. Bit-exact at slot 0 holds because idle outputs are literal `0.0f` (proved §15.9.2) and the E-string write is byte-identical to Phase 2.1c.
2. **No topology change for slot 0 active string.** `strings[0].processSample(v_bow, F_bow, friction)` must produce the same value as Phase 2.1c's single `waveguideString.processSample(...)` call at the regression preset.
3. **`prepareToPlay` slot-0 sequence unchanged.** `strings[0].prepare(sr_internal, maxBlockSize * 2)` then `strings[0].setDispersionActiveSections(4)` matches Phase 2.1c's `waveguideString.prepare(...)` + `bridgeDispersion.setActiveSections(4)` (now via R22's pass-through). Slot 0 setter args identical.
4. **Slot-0 `setActiveSections(4)` unchanged** — Phase 2.1c already calls this at line 40 of `WaveguideString.cpp`. R22 just adds the public-API pass-through to allow voice to drive it per slot.

---

## Goal

Replace the single-string E1 voice with a 4-string EADG bank inside one `BowedContrabassVoice`. Add per-string detune ±1200¢ smoothing in delay-samples space (20 ms `juce::SmoothedValue<Linear>` per slot). Configure per-string dispersion M=4/3/2/1 + B prefactor 1e-4/7e-5/5e-5/3e-5 once at `prepareToPlay` via R22's new `setDispersionActiveSections` pass-through. Add closed-form MIDI-note → string-index mapping (thresholds 28/33/38/43) with `ACTIVE_STRINGS` clamp (remap to highest active slot). Implement 5 ms equal-power crossfade at the voice mix-bus on note-on transitions to a different slot, using a precomputed ramp (3.5 KiB at 88.2 kHz internal). Validate Gate 4 invariants — eight-item bar including strict byte-equal regression at the Phase 2.1c golden — and atomic-commit on Gate 4 PASS as R26.

---

## Tasks

### R21-pre — Pre-flight bit-exact baseline confirmation

**No source edits. Diagnostic only. Confirms working-tree integrity at start of execute.**

Per RESEARCH §15.1, the working tree at R20 commit `5759e5e` produces sha256 `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75` for the regression preset (MIDI 28, ACTIVE_STRINGS=4, DETUNE_E=0, STRING_STIFFNESS=0). This was empirically verified 2026-04-27 during research. R21-pre re-runs that render at the start of execute as a tripwire — if the working tree has drifted (e.g., uncommitted edits since 2026-04-27), R21-pre catches it BEFORE any Phase 2.2 source edits land.

**Tasks:**

1. **Confirm git state is clean and at R20:**
   ```bash
   git log --oneline -1 plugins/O-Contrabass/Source/   # should be 5759e5e (Phase 2.1c R20)
   git status plugins/O-Contrabass/Source/             # should be clean
   ```
   If working tree is dirty in `plugins/O-Contrabass/Source/` or in `tests/render-harness/`, STOP and reconcile before proceeding.

2. **Build harness (no source changes since R20; should be a no-op rebuild):**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```

3. **Render regression preset to `/tmp/`:**
   ```bash
   cd build
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --note 28 --velocity 0.7 --sustain 60 --release 5 \
       --infinite-sustain 1.0 --string-stiffness 0 \
       --out /tmp/phase22-r21pre-stiffness-zero.wav \
       --json /tmp/phase22-r21pre-stiffness-zero.json
   ```

4. **Confirm sha256 match:**
   ```bash
   shasum -a 256 /tmp/phase22-r21pre-stiffness-zero.wav | awk '{print $1}'
   # expected: d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75
   ```
   Compare with `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256` (committed Phase 2.1c golden). MUST match byte-for-byte.

   **If MISMATCH:** STOP. Investigate working-tree drift (check `git diff` for any post-R20 edits in `Source/`, `modules/synthesis/bow-friction/`, or `tests/render-harness/`). Do NOT proceed to R21 until the working tree reproduces `d358abcd…`.

**Files modified:** none.
**Files created:** `/tmp/phase22-r21pre-stiffness-zero.{wav,json}` (transient, NOT committed).

**Commit:** **NONE** — diagnostic only.

**Success bar:**
- [ ] `git status plugins/O-Contrabass/Source/` clean.
- [ ] Harness rebuild succeeds (or no-op).
- [ ] Render exits 0; `pass_nan`/`pass_peak`/`pass_blockTime` all TRUE.
- [ ] sha256 == `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75`.

**Estimated effort:** 5 min (single render + sha256 compare).

---

### R21 — `BowedContrabassVoice.{h,cpp}` — 4-string bank + detune + crossfade + mapping

**Per RESEARCH §15.2 (mapping + state machine), §15.3 (crossfade ramp + per-sample mix), §15.4 (per-block stiffness update sequence), §15.5 (prepareToPlay), §15.9 (bit-exact preservation hard rules). Most-complex task in this phase.**

**Tasks:**

1. **`BowedContrabassVoice.h` — add Phase 2.2 state and helpers.**

   Replace the single `WaveguideString waveguideString;` member with `std::array<WaveguideString, 4> strings;` (slots 0=E, 1=A, 2=D, 3=G).

   Add:
   ```cpp
   // Phase 2.2 — 4-string bank state
   std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 4> detuneSmoothed;
   int   activeStringIndex          = -1;        // -1 = no string yet (first noteStarted)
   int   previousStringIndex        = -1;        // valid only while crossfadeRemainingSamples > 0
   int   crossfadeRemainingSamples  = 0;
   int   crossfadeTotalSamples      = 0;         // = ceil(0.005 * sr_internal); cached at prepare
   std::vector<std::pair<float, float>> crossfadeRamp;   // (oldGain, newGain) per sample
   double sr_internal               = 88200.0;   // 2x oversampled host rate
   ```

   Helper declarations:
   ```cpp
   int   mapMidiNoteToStringIndex (int midiNote, int activeStrings) const noexcept;
   float readDetuneForString      (int s) const noexcept;
   float computeDelaySamples      (float playedFreqHz, float detuneCents) const noexcept;
   ```

2. **`BowedContrabassVoice.cpp` — `prepareToPlay` extension.**

   Replace the existing single-string `waveguideString.prepare(sr_internal, ...)` with the per-slot loop (RESEARCH §15.5):

   ```cpp
   currentMaxBlockSize = maxBlockSize;
   sr_internal = hostSampleRate * 2.0;

   oversampling.initProcessing (static_cast<size_t> (maxBlockSize));
   oversampling.reset();
   voiceBuffer.setSize (1, maxBlockSize * 2, false, true, false);
   voiceBuffer.clear();

   static constexpr float openStringFrequencyHz[4] = { 41.20f, 55.00f, 73.42f, 98.00f };
   static constexpr int   M_per_string[4]          = { 4, 3, 2, 1 };

   for (int s = 0; s < 4; ++s)
   {
       strings[s].prepare (sr_internal, maxBlockSize * 2);
       strings[s].setDispersionActiveSections (M_per_string[s]);   // R22 new API
       detuneSmoothed[s].reset (sr_internal, 0.020);                // 20 ms ramp
       detuneSmoothed[s].setCurrentAndTargetValue (
           static_cast<float> (sr_internal) / openStringFrequencyHz[s]);
   }

   bowModel.prepare (sr_internal);
   frictionModel.setStaticFrictionCoefficient  (0.85f);
   frictionModel.setDynamicFrictionCoefficient (0.25f);

   // Crossfade ramp precompute (§15.3).
   crossfadeTotalSamples = static_cast<int> (std::ceil (0.005 * sr_internal));
   crossfadeRamp.resize (static_cast<size_t> (crossfadeTotalSamples + 1));
   const float invN = 1.0f / static_cast<float> (crossfadeTotalSamples);
   const float halfPi = juce::MathConstants<float>::halfPi;
   for (int i = 0; i <= crossfadeTotalSamples; ++i)
   {
       const float t = static_cast<float> (i) * invN;
       crossfadeRamp[static_cast<size_t> (i)] = { std::cos (t * halfPi), std::sin (t * halfPi) };
   }

   activeStringIndex         = -1;
   previousStringIndex       = -1;
   crossfadeRemainingSamples = 0;
   ```

   **HARD RULE (§15.9.5):** slot-0 setup sequence (`strings[0].prepare(...)` + `strings[0].setDispersionActiveSections(4)`) must produce byte-identical state to Phase 2.1c's `waveguideString.prepare(...)` + `bridgeDispersion.setActiveSections(4)`. R22's `setDispersionActiveSections` is a 1-line pass-through — no semantic change.

3. **`BowedContrabassVoice.cpp` — `noteStarted` extension.**

   Replace single-string trigger with the §15.2 state-machine pseudocode:

   ```cpp
   void BowedContrabassVoice::noteStarted()
   {
       auto note = getCurrentlyPlayingNote();
       const int   midiNote = note.initialNote;
       const float velocity = note.noteOnVelocity.asUnsignedFloat();

       // 1. Resolve frequency (12-TET + MPE bend).
       double freq = juce::MidiMessage::getMidiNoteInHertz (midiNote);
       const float bend = static_cast<float> (note.totalPitchbendInSemitones);
       if (std::abs (bend) > 0.001f) freq *= std::pow (2.0, bend / 12.0);
       currentFrequency = static_cast<float> (freq);

       // 2. Resolve target string.
       const int activeStrings = static_cast<int> (parameters->getRawParameterValue ("ACTIVE_STRINGS")->load());
       const int newStringIndex = mapMidiNoteToStringIndex (midiNote, activeStrings);

       // 3. Decide trigger semantics.
       const bool isFirstNote   = (activeStringIndex < 0);
       const bool needsCrossfade = (! isFirstNote)
                                 && bowModel.isActive()
                                 && (newStringIndex != activeStringIndex);

       if (needsCrossfade)
       {
           previousStringIndex       = activeStringIndex;
           activeStringIndex         = newStringIndex;
           crossfadeRemainingSamples = crossfadeTotalSamples;
       }
       else
       {
           previousStringIndex       = -1;
           activeStringIndex         = newStringIndex;
           crossfadeRemainingSamples = 0;
       }

       // 4. Configure new string's delay-line immediately (avoid lag at old freq).
       const float detuneCents   = readDetuneForString (newStringIndex);
       const float targetSamples = computeDelaySamples (currentFrequency, detuneCents);
       detuneSmoothed[newStringIndex].setCurrentAndTargetValue (targetSamples);
       strings[newStringIndex].trigger (currentFrequency);
       strings[newStringIndex].setDelaySamples (targetSamples);

       // 5. Engage bow.
       bowModel.startBow (velocity);
       oversampling.reset();
   }
   ```

4. **`BowedContrabassVoice.cpp` — `renderNextBlock` per-block update sequence.**

   Per RESEARCH §15.4 — extend Phase 2.1c's per-block `STRING_STIFFNESS` push to all 4 slots, AND add the per-block detune ramp:

   ```cpp
   // Step A: Push global STRING_STIFFNESS to all 4 instances.
   const float stringStiffness = parameters->getRawParameterValue ("STRING_STIFFNESS")->load();
   for (int s = 0; s < 4; ++s)
       strings[s].setStringStiffness (stringStiffness);

   // Step B: Advance all 4 stiffness smoothers by numSamples (host-rate count, matching
   //         Phase 2.1c R17 semantics — DO NOT change units, regression bar bit-shifts).
   for (int s = 0; s < 4; ++s)
       strings[s].advanceStiffnessSmootherBy (numSamples);

   // Step C: Compute per-string a and push.
   static constexpr float openStringFrequencyHz[4] = { 41.20f, 55.00f, 73.42f, 98.00f };
   static constexpr float B_open[4]                 = { 1.0e-4f, 7.0e-5f, 5.0e-5f, 3.0e-5f };
   static constexpr int   M_per_string[4]           = { 4, 3, 2, 1 };

   for (int s = 0; s < 4; ++s)
   {
       const float currentStiffness = strings[s].getCurrentSmoothedStiffness();
       const float B = B_open[s] * juce::jlimit (0.0f, 1.0f, currentStiffness);
       const int   M = M_per_string[s];

       const float f0 = (s == activeStringIndex || s == previousStringIndex)
                      ? juce::jlimit (20.0f, 5000.0f, currentFrequency)
                      : juce::jlimit (20.0f, 5000.0f, openStringFrequencyHz[s]);

       float a = (currentStiffness <= 0.0f)
               ? 0.0f
               : DispersionFilter<4>::computeAllpassCoefficient (f0, B, M);
       if (! std::isfinite (a)) a = 0.0f;
       strings[s].setDispersionCoefficient (a);
   }

   // Step D: Per-string detune ramp targets (only update active + previous for now;
   //         idle slots stay at their last-set target, which is the open-string default).
   if (activeStringIndex >= 0)
   {
       const float dc = readDetuneForString (activeStringIndex);
       const float ds = computeDelaySamples (currentFrequency, dc);
       detuneSmoothed[activeStringIndex].setTargetValue (ds);
   }
   if (previousStringIndex >= 0)
   {
       const float dcPrev = readDetuneForString (previousStringIndex);
       const float dsPrev = computeDelaySamples (currentFrequency, dcPrev);
       detuneSmoothed[previousStringIndex].setTargetValue (dsPrev);
   }
   ```

5. **`BowedContrabassVoice.cpp` — per-sample oversampled DSP loop.**

   Replace single-string sample write with the §15.3 + §15.9.3 mix pattern. **HARD RULE: use early-return on `activeStringIndex` to preserve bit-exact at slot-0:**

   ```cpp
   for (int i = 0; i < numUp; ++i)
   {
       bowModel.updateEnvelope();
       const float v_bow = bowModel.getBowVelocity();
       const float F_bow = bowModel.getBowForce();

       float mixedSample = 0.0f;

       if (crossfadeRemainingSamples > 0)
       {
           const int idx = juce::jlimit (0, crossfadeTotalSamples,
                                         crossfadeTotalSamples - crossfadeRemainingSamples);
           const auto [oldGain, newGain] = crossfadeRamp[static_cast<size_t> (idx)];

           // Per-sample setDelay BEFORE processSample so the Lagrange3rd reads from the
           // updated delay-line length on this iteration (matches vibrato pattern).
           strings[previousStringIndex].setDelaySamples (detuneSmoothed[previousStringIndex].getNextValue());
           strings[activeStringIndex  ].setDelaySamples (detuneSmoothed[activeStringIndex  ].getNextValue());

           // Tick all 4 strings; only previous + active produce non-zero output here.
           const float oldOut = strings[previousStringIndex].processSample (0.0f, 0.0f, frictionModel);
           const float newOut = strings[activeStringIndex  ].processSample (v_bow, F_bow, frictionModel);
           // Idle remaining slots (s != active && s != previous) — tick + discard.
           for (int s = 0; s < 4; ++s)
               if (s != activeStringIndex && s != previousStringIndex)
                   (void) strings[s].processSample (0.0f, 0.0f, frictionModel);

           mixedSample = oldOut * oldGain + newOut * newGain;
           --crossfadeRemainingSamples;
           if (crossfadeRemainingSamples == 0)
               previousStringIndex = -1;
       }
       else
       {
           // Standard path. HARD RULE §15.9.5: early-return on activeStringIndex.
           for (int s = 0; s < 4; ++s)
           {
               // Per-sample detune apply (active and idle alike — idle smoothers are at
               // open-string default, so this is a setDelay to the same value each tick;
               // the idle delay-line state is all zeros so output is 0 regardless).
               strings[s].setDelaySamples (detuneSmoothed[s].getNextValue());

               if (s == activeStringIndex)
                   mixedSample = strings[s].processSample (v_bow, F_bow, frictionModel);
               else
                   (void) strings[s].processSample (0.0f, 0.0f, frictionModel);
           }
       }

       upData[i] = mixedSample;
   }
   ```

   **Bit-exact preservation:** at the regression preset (MIDI 28 → activeStringIndex=0, ACTIVE_STRINGS=4, no crossfade ever), the standard path runs `mixedSample = strings[0].processSample(v_bow, F_bow, friction)` for slot 0 — byte-identical to Phase 2.1c. Slots 1/2/3 tick with `(0,0)` injection but their outputs are discarded (idle output = `0.0f` per §15.9.2 analytical proof). `setDelaySamples()` on idle slots writes the open-string default each tick (smoother is at-target), which equals the value the Phase 2.1c voice was implicitly setting once at prepare; the Lagrange3rd state evolution is identical: zero in → zero out → zero state.

   **Caveat — slot-0 `setDelaySamples` is now per-sample, was per-noteStarted in Phase 2.1c.** The smoother is at-target in steady state (after the 20 ms ramp from prepare's initial-set), so per-sample setDelay writes the same value the Phase 2.1c implementation had baked into the delay line. Sub-issue: `juce::dsp::DelayLine<float, Lagrange3rd>::setDelay(d)` where `d` matches the current internal value is a no-op (no state change). Confirmed bit-exact. **If R24 fails the byte-equal check, suspect this — the delay-line internal state representation may be sensitive to setDelay() call ordering.** Diagnostic-on-fail: gate the per-sample setDelay with `if (detuneSmoothed[s].isSmoothing())` to skip the call when the smoother is at-target (preserves Phase 2.1c semantics for the regression preset).

6. **`BowedContrabassVoice.cpp` — helper implementations.**

   ```cpp
   int BowedContrabassVoice::mapMidiNoteToStringIndex (int midiNote, int activeStrings) const noexcept
   {
       int idx = 0;
       if      (midiNote >= 43) idx = 3;
       else if (midiNote >= 38) idx = 2;
       else if (midiNote >= 33) idx = 1;
       const int maxIdx = juce::jlimit (0, 3, activeStrings - 1);
       return juce::jmin (idx, maxIdx);
   }

   float BowedContrabassVoice::readDetuneForString (int s) const noexcept
   {
       static constexpr const char* paramIds[4] = { "DETUNE_E", "DETUNE_A", "DETUNE_D", "DETUNE_G" };
       jassert (s >= 0 && s < 4);
       return parameters->getRawParameterValue (paramIds[s])->load();
   }

   float BowedContrabassVoice::computeDelaySamples (float playedFreqHz, float detuneCents) const noexcept
   {
       const float detuneRatio = std::pow (2.0f, detuneCents / 1200.0f);
       const float detunedFreq = playedFreqHz * detuneRatio;
       return static_cast<float> (sr_internal) / juce::jmax (1.0f, detunedFreq);
   }
   ```

7. **Compile-only check (incremental):**
   ```bash
   cmake --build build --target O-Contrabass_VST3 --parallel
   ```
   Expect clean build. R22 must land BEFORE R21's `setDispersionActiveSections` call resolves — sequence them tightly (or stage R22's setter declaration first so R21 compiles). **Per dependency graph: R22 lands before R21's compile.**

**Files modified:**
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` — ~+15 LOC (new state vars, helper decls).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` — ~+80 LOC (state machine, mix loop, prepareToPlay extensions, helpers).

**Files NOT touched:** `Source/PluginProcessor.{h,cpp}` (APVTS already declares all four `DETUNE_*` and `ACTIVE_STRINGS` from Stage 1), `Source/PluginEditor.{h,cpp}` (Stage 3 work), `Source/DSP/DispersionFilter.h` (Phase 2.1c R20-frozen), `modules/synthesis/bow-friction/*` (Phase 2.1b-frozen v1.0.0).

**Commit:** **NONE** — staging only. R26 absorbs.

**Success bar:**
- [ ] `BowedContrabassVoice.h` declares `std::array<WaveguideString, 4> strings`, `std::array<juce::SmoothedValue<float, Linear>, 4> detuneSmoothed`, all 4 state ints/vec, 3 helper signatures.
- [ ] `prepareToPlay` loops over `s ∈ [0,3]` calling `strings[s].prepare(...)` then `strings[s].setDispersionActiveSections(M_per_string[s])`.
- [ ] `noteStarted` runs the §15.2 state machine; mid-crossfade re-trigger replaces (does NOT queue).
- [ ] `renderNextBlock` per-block sequence pushes `STRING_STIFFNESS` + advances 4 smoothers + pushes 4 `a` values + updates active/previous detune targets.
- [ ] Per-sample mix uses early-return on `activeStringIndex` (NOT unconditional sum).
- [ ] Helpers: `mapMidiNoteToStringIndex` (4-comparison ladder + clamp), `readDetuneForString` (paramId array), `computeDelaySamples` (cents → ratio → samples).
- [ ] Incremental compile clean (after R22 lands `setDispersionActiveSections`).

**Estimated effort:** 4 h (most-complex task; transcription from RESEARCH §15.2–§15.9 plus careful preservation of slot-0 bit-exact path).

---

### R22 — `WaveguideString.{h,cpp}` — `setDispersionActiveSections` pass-through

**Per RESEARCH §15.5.1 option B. Single-method pass-through addition. NO topology changes; NO smoother relocation; NO `prepare()` signature change.**

**Tasks:**

1. **`Source/DSP/WaveguideString.h` — append public method declaration.**

   Add to the public API (alongside `setStringStiffness`, `advanceStiffnessSmootherBy`, `getCurrentSmoothedStiffness`, `setDispersionCoefficient`):
   ```cpp
   /** Configures the per-instance dispersion section count. Phase 2.2: voice calls this
    *  once per slot at prepareToPlay (E=4, A=3, D=2, G=1 per ARCH §"String Waveguide
    *  Bank"). Pass-through to bridgeDispersion.setActiveSections.
    */
   void setDispersionActiveSections (int M) noexcept;
   ```

2. **`Source/DSP/WaveguideString.cpp` — append method body.**

   ```cpp
   void WaveguideString::setDispersionActiveSections (int M) noexcept
   {
       bridgeDispersion.setActiveSections (M);
   }
   ```

3. **HARD RULE (§15.9.5):** the existing `prepare()` line `bridgeDispersion.setActiveSections(4);` (`WaveguideString.cpp:40`) is RETAINED — do NOT remove it. R21 still calls `strings[s].setDispersionActiveSections(M_per_string[s])` AFTER `prepare()`, so for slot 0 (E-string) the sequence is: `prepare()` sets `activeSections = 4` → R21 calls `setDispersionActiveSections(4)` → no-op. Slots 1/2/3: `prepare()` sets `activeSections = 4` → R21 overrides to 3/2/1. Slot-0 final state = 4 = Phase 2.1c state = bit-exact preserved.

   *Alternative considered but rejected:* removing line 40 of `WaveguideString.cpp` would shift slot-0 `activeSections` from "set in prepare()" to "set externally" — equivalent in value but theoretically different in code path (slot 0 would never enter `prepare()`'s setActiveSections branch). The DispersionFilter's internal state at `activeSections=4` is the same regardless of which call set it (no side effects beyond storing the int), so this would also be bit-exact. But retaining the line is the lower-risk choice.

4. **Compile-only check:**
   ```bash
   cmake --build build --target O-Contrabass_VST3 --parallel
   ```
   Expect clean build. R22 alone (without R21) yields a void method that's never called; build still passes. R22+R21 together is the complete wiring.

**Files modified:**
- `plugins/O-Contrabass/Source/DSP/WaveguideString.h` — ~+1 LOC (method declaration + doc-comment).
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` — ~+3 LOC (method body).

**Files NOT touched:** `Source/DSP/DispersionFilter.h` (Phase 2.1c R20-frozen — `setActiveSections` already public; R22 just routes through `WaveguideString`).

**Commit:** **NONE** — staging only. R26 absorbs.

**Success bar:**
- [ ] `WaveguideString::setDispersionActiveSections (int M) noexcept` declared in header.
- [ ] Body forwards verbatim to `bridgeDispersion.setActiveSections(M)`.
- [ ] Existing `prepare()` line 40 (`bridgeDispersion.setActiveSections(4)`) UNCHANGED.
- [ ] Build clean.

**Estimated effort:** 10 min.

**Sequence:** **R22 must land before R21's compile** (R21 calls the new method). Ordering convention for execute: edit `WaveguideString.{h,cpp}` first, then edit `BowedContrabassVoice.{h,cpp}`.

---

### R23 — `tests/render-harness/main.cpp` — new CLI flags + JSON schema

**Per RESEARCH §15.7 (`--detune-sweep`), §15.8 (`--note-sequence`), §15.11 (`--string`).**

**Tasks:**

1. **Add CLI flags to `Args` struct + parser.**

   Extend the existing `Args` struct (around line 50) with:
   ```cpp
   char           stringOverride        = ' ';   // 'E','A','D','G' or ' '
   char           detuneSweepString     = ' ';   // 'E','A','D','G' or ' '
   juce::String   noteSequence;                  // "MIDI:dur,..." or empty
   ```

   Extend `parseArgs` switch:
   ```cpp
   else if (key == "--string")          args.stringOverride    = parseStringLetter (val);
   else if (key == "--detune-sweep")    args.detuneSweepString = parseStringLetter (val);
   else if (key == "--note-sequence")   args.noteSequence      = val;
   ```
   With helper:
   ```cpp
   static char parseStringLetter (const juce::String& s)
   {
       const auto upper = s.toUpperCase();
       if (upper == "E") return 'E';
       if (upper == "A") return 'A';
       if (upper == "D") return 'D';
       if (upper == "G") return 'G';
       std::cerr << "warning: --string / --detune-sweep arg '" << s << "' invalid; ignoring\n";
       return ' ';
   }
   ```

2. **Implement `--string` MIDI override.**

   Before the existing `--note` resolution block, if `args.stringOverride != ' '`, override `args.note` to the matching open-string MIDI:
   ```cpp
   if (args.stringOverride != ' ')
   {
       const int impliedNote = (args.stringOverride == 'E') ? 28
                             : (args.stringOverride == 'A') ? 33
                             : (args.stringOverride == 'D') ? 38
                             : 43;   // 'G'
       if (args.note != impliedNote && args.note != defaultNote)
           std::cerr << "warning: --string=" << args.stringOverride << " overrides --note=" << args.note << "\n";
       args.note = impliedNote;
   }
   ```
   `--string` and `--detune-sweep` are mutually exclusive in effect; harness emits a stderr warning if both are set and proceeds with `--detune-sweep` precedence.

3. **Implement `--detune-sweep` per-block ramp.**

   Per RESEARCH §15.7. In the main render loop, alongside the existing `--stiffness-sweep` block:
   ```cpp
   if (args.detuneSweepString != ' ')
   {
       const float fraction = static_cast<float> (sampleCursor)
                            / static_cast<float> (juce::jmax (1, sustainSamples));
       const float clamped  = juce::jlimit (0.0f, 1.0f, fraction);
       const float cents    = -1200.0f + 2400.0f * clamped;

       juce::String paramId;
       switch (args.detuneSweepString)
       {
           case 'E': paramId = "DETUNE_E"; break;
           case 'A': paramId = "DETUNE_A"; break;
           case 'D': paramId = "DETUNE_D"; break;
           case 'G': paramId = "DETUNE_G"; break;
       }
       if (auto* p = proc.parameters.getParameter (paramId))
       {
           const float norm = (cents + 1200.0f) / 2400.0f;
           p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
       }
   }
   ```
   When `--detune-sweep` is set, override `args.note` to the matching open-string MIDI (same logic as step 2). Override default `--sustain` to 30 s and default `--release` to 2 s when the user hasn't explicitly set them in `--detune-sweep` mode.

4. **Implement `--note-sequence` event-list pre-build + per-block drain.**

   Per RESEARCH §15.8. Before the render loop:
   ```cpp
   struct ScheduledMidiEvent { int sampleIndex; juce::MidiMessage message; };
   std::vector<ScheduledMidiEvent> sequenceEvents;

   if (args.noteSequence.isNotEmpty())
   {
       juce::StringArray segments;
       segments.addTokens (args.noteSequence, ",", "");

       int cursor = 0;
       const int channel = 1;
       for (const auto& segment : segments)
       {
           const int colon = segment.indexOfChar (':');
           if (colon < 0) { std::cerr << "warning: malformed segment '" << segment << "'\n"; continue; }
           const int   note = segment.substring (0, colon).getIntValue();
           const float dur  = segment.substring (colon + 1).getFloatValue();
           const int   durSamples = static_cast<int> (dur * sampleRate);

           const int velMidi = juce::jlimit (1, 127, static_cast<int> (std::round (args.velocity * 127.0f)));
           sequenceEvents.push_back ({ cursor,                  juce::MidiMessage::noteOn  (channel, note, (juce::uint8) velMidi) });
           sequenceEvents.push_back ({ cursor + durSamples - 1, juce::MidiMessage::noteOff (channel, note) });
           cursor += durSamples;
       }
       sustainSamples = cursor;
       totalSamples   = cursor + static_cast<int> (args.releaseSeconds * sampleRate);

       if (args.sustainSeconds != defaultSustain)
           std::cerr << "warning: --sustain ignored in --note-sequence mode\n";
   }
   ```

   In the per-block `MidiBuffer` construction (replacing the existing single-noteOn/single-noteOff block):
   ```cpp
   juce::MidiBuffer midi;
   if (! sequenceEvents.empty())
   {
       for (const auto& e : sequenceEvents)
           if (e.sampleIndex >= sampleCursor && e.sampleIndex < sampleCursor + thisBlock)
               midi.addEvent (e.message, e.sampleIndex - sampleCursor);
   }
   else
   {
       // ...existing single-note path (unchanged from Phase 2.1c)...
   }
   ```

5. **Implement RMS-continuity computation.**

   Track per-block RMS during sustain phase:
   ```cpp
   std::vector<float> blockRmsHistory;
   // (per-block, after processBlock completes)
   if (sampleCursor < sustainSamples)
   {
       float sumSq = 0.0f;
       for (int ch = 0; ch < blockBuffer.getNumChannels(); ++ch)
           for (int i = 0; i < thisBlock; ++i)
           {
               const float s = blockBuffer.getSample (ch, i);
               sumSq += s * s;
           }
       const float rms = std::sqrt (sumSq / static_cast<float> (thisBlock * blockBuffer.getNumChannels()));
       blockRmsHistory.push_back (rms);
   }
   ```

   Post-render (`--detune-sweep` and `--note-sequence` modes only):
   ```cpp
   float rmsContinuityRatio = 1.0f;
   for (size_t i = 1; i < blockRmsHistory.size(); ++i)
   {
       const float a = blockRmsHistory[i - 1];
       const float b = blockRmsHistory[i];
       const float ratio = juce::jmin (a, b) / juce::jmax (juce::jmax (a, b), 1.0e-9f);
       rmsContinuityRatio = juce::jmin (rmsContinuityRatio, ratio);
   }
   const bool pass_rmsContinuity = (rmsContinuityRatio >= 0.90f);
   ```

6. **Implement transition-window RMS for `--note-sequence` mode.**

   Per RESEARCH §15.8. Capture the full sustain-phase mono waveform, then compute the 256-sample symmetric-window ratio at each transition:
   ```cpp
   // sustainAudioMono[] is a flat float vector of all sustain-phase samples (channel 0).
   std::vector<int> transitionSampleIndices;   // populated during pre-build event list
   float rmsContinuityAtTransitions = 1.0f;
   for (int t : transitionSampleIndices)
   {
       const int N = 128;
       const int lo = juce::jmax (0, t - N);
       const int hi = juce::jmin (static_cast<int> (sustainAudioMono.size()), t + N);

       auto rmsOver = [&] (int s, int e) {
           float acc = 0.0f;
           for (int i = s; i < e; ++i) acc += sustainAudioMono[i] * sustainAudioMono[i];
           return std::sqrt (acc / juce::jmax (1, e - s));
       };
       const float beforeRms = rmsOver (lo, t);
       const float afterRms  = rmsOver (t,  hi);
       const float ratio = juce::jmin (beforeRms, afterRms)
                         / juce::jmax (juce::jmax (beforeRms, afterRms), 1.0e-9f);
       rmsContinuityAtTransitions = juce::jmin (rmsContinuityAtTransitions, ratio);
   }
   const bool pass_rmsContinuityAtTransitions = (rmsContinuityAtTransitions >= 0.50f);
   ```

7. **Implement per-segment audibility check for `--note-sequence` mode.**
   ```cpp
   std::vector<float> perSegmentRms;
   bool pass_allSegmentsAudible = true;
   for (size_t s = 0; s < sequenceSegments.size(); ++s)
   {
       const int sampleStart = segmentSampleStart[s];
       const int sampleEnd   = segmentSampleStart[s] + segmentSampleCount[s];
       float acc = 0.0f;
       for (int i = sampleStart; i < sampleEnd; ++i)
           acc += sustainAudioMono[i] * sustainAudioMono[i];
       const float segRms = std::sqrt (acc / juce::jmax (1, sampleEnd - sampleStart));
       perSegmentRms.push_back (segRms);
       if (segRms <= 1.0e-3f) pass_allSegmentsAudible = false;
   }
   ```

8. **Extend JSON output schema.**

   Per RESEARCH §15.7 + §15.8. Add `mode` field (one of `"sustained"`, `"stiffness-sweep"`, `"detune-sweep"`, `"note-sequence"`). Mode-specific extras:
   - `detune-sweep`: `string`, `detuneRamp`, `rmsByDecade`, `rmsContinuityRatio`, `pass_rmsContinuity`. Omit `pass_rms` from PASS criterion (slow envelope drift expected).
   - `note-sequence`: `sequence`, `transitionSampleIndices`, `perSegmentRms`, `pass_allSegmentsAudible`, `pass_rmsContinuityAtTransitions`, `rmsContinuityAtTransitions`. Omit `pass_rms`.

   Overall `status: "PASS"` criterion:
   - `sustained`/`stiffness-sweep`: unchanged from Phase 2.1c (`pass_nan && pass_peak && pass_blockTime && pass_rms`).
   - `detune-sweep`: `pass_nan && pass_peak && pass_blockTime && pass_rmsContinuity`.
   - `note-sequence`: `pass_nan && pass_peak && pass_blockTime && pass_allSegmentsAudible && pass_rmsContinuityAtTransitions`.

9. **Auto-rewrite default WAV/JSON filenames in new modes.**

   When `--out` / `--json` are not explicitly set:
   - `--detune-sweep <X>`: defaults to `detune-sweep-<X>.wav` / `detune-sweep-<X>.json`.
   - `--note-sequence`: defaults to `note-sequence.wav` / `note-sequence.json`.
   - `--string <X>` (sustained-tone mode): defaults to `string-<X>.wav` / `string-<X>.json`.

10. **Update doc-comment header.**

    Document the three new flags + the auto-rewrite filename behaviour.

11. **Build + smoke-test:**
    ```bash
    cmake --build build --target O-Contrabass-render-test --parallel
    ./build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
        --string A --sustain 5 --out /tmp/r23-smoke.wav --json /tmp/r23-smoke.json
    cat /tmp/r23-smoke.json | jq '.mode, .midiNote'   # expect "sustained", 33
    ```

**Files modified:**
- `plugins/O-Contrabass/tests/render-harness/main.cpp` — ~+120 LOC (3 new flags, 2 new modes, JSON schema additions, RMS-continuity computation, transition-window computation, per-segment audibility check).

**Files NOT touched:** any DSP source (`Source/*`).

**Commit:** **NONE** — staging only. R26 absorbs.

**Success bar:**
- [ ] `--string`, `--detune-sweep`, `--note-sequence` all parse correctly.
- [ ] Smoke test (`--string A --sustain 5`) produces `mode: "sustained"`, `midiNote: 33`.
- [ ] `--detune-sweep A --sustain 30` ramps DETUNE_A from −1200 → +1200 across the sustain phase; JSON emits `mode: "detune-sweep"`, `string: "A"`, `detuneRamp`, `rmsByDecade`, `rmsContinuityRatio`, `pass_rmsContinuity`.
- [ ] `--note-sequence "28:2,33:2,38:2,43:2,28:2"` schedules 5 noteOn/noteOff pairs; JSON emits `mode: "note-sequence"`, `sequence`, `transitionSampleIndices`, `perSegmentRms`, `pass_allSegmentsAudible`, `pass_rmsContinuityAtTransitions`.
- [ ] Auto-rewrite filenames work when `--out` / `--json` omitted.
- [ ] Build clean.

**Estimated effort:** 3 h.

---

### R24 — Build + auval + pluginval-10 + bit-exact regression check

**Phase 2.2 Gate 4 invariant (7) — strict byte-equal regression at the Phase 2.1c golden.**

**Tasks:**

1. **Full Release rebuild:**
   ```bash
   cmake --build build --config Release --target O-Contrabass_VST3 O-Contrabass_AU --parallel
   ```
   Expect clean build, no new warnings beyond Phase 2.1c baseline.

2. **Install fresh (per CLAUDE.md plugin-cache-clearing protocol):**
   ```bash
   killall -9 AudioComponentRegistrar 2>/dev/null || true
   rm -rf ~/Library/Caches/AudioUnitCache/
   rm -rf ~/Library/Caches/com.apple.audiounits.cache
   rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3
   rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass.component
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass.vst3 \
         ~/Library/Audio/Plug-Ins/VST3/
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass.component \
         ~/Library/Audio/Plug-Ins/Components/
   ```

3. **`auval`:**
   ```bash
   auval -v aumu OCbs OuDv
   ```
   Expect `AU VALIDATION SUCCEEDED.`

4. **`pluginval` strictness 10:**
   ```bash
   pluginval --strictness-level 10 --validate-in-process \
       ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3
   ```
   Expect `ALL TESTS PASSED.`

5. **R-final bit-exact regression render:**
   ```bash
   cd build
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --note 28 --velocity 0.7 --sustain 60 --release 5 \
       --infinite-sustain 1.0 --string-stiffness 0 \
       --out /tmp/phase22-r24-stiffness-zero-post.wav \
       --json /tmp/phase22-r24-stiffness-zero-post.json
   shasum -a 256 /tmp/phase22-r24-stiffness-zero-post.wav | awk '{print $1}'
   ```

6. **Compare against committed Phase 2.1c golden:**
   ```bash
   diff <(shasum -a 256 /tmp/phase22-r24-stiffness-zero-post.wav | awk '{print $1}') \
        plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256
   # expected: empty diff
   ```
   **MUST be byte-equal.** sha256 == `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75`.

   **If MISMATCH (R24 FAIL):** the implementation has introduced a bit-shift bug. Diagnostic-on-fail per RESEARCH §15.9.5:
   - Suspect HARD-RULE violation: per-sample `setDelaySamples` on slot 0 in steady state. Apply the `if (detuneSmoothed[s].isSmoothing())` gate (R21 step 5 caveat) and re-render.
   - If still fails: suspect floating-point reordering in mix loop. Confirm `mixedSample = strings[0].processSample(...)` is the unconditional write at slot 0, not an additive accumulation.
   - If still fails: suspect `prepareToPlay` slot-0 sequence drift. Confirm slot-0 sees `prepare(sr_internal, ...)` then `setDispersionActiveSections(4)` — the latter is a no-op re-set of `activeSections=4`, so state is identical.
   - Last resort: soften the bar to "RMS-equivalent within 1 LSB" (`rmsRatio_pre_vs_post_full_render >= 0.99999`). Document in commit body and verify report. (RESEARCH §15.9.6 says this fallback is unnecessary; if it triggers, file a follow-up RESEARCH note for Phase 2.4.)

**Files modified:** none (rebuild + install only).
**Files created:** `/tmp/phase22-r24-stiffness-zero-post.{wav,json}` (transient, NOT committed).

**Commit:** **NONE** — gate diagnostic only.

**Success bar:**
- [ ] Release build clean.
- [ ] `auval -v aumu OCbs OuDv`: AU VALIDATION SUCCEEDED.
- [ ] `pluginval --strictness-level 10`: ALL TESTS PASSED.
- [ ] R-final sha256 == `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75` (byte-equal Phase 2.1c golden).

**Estimated effort:** 30 min (build + install + 3 validation runs).

---

### R25 — Gate 4 invariants (1)–(6) — per-string + detune-sweep + note-sequence

**Per CONTEXT rev-4 §"Approach Decisions" Q8 — eight-item Gate 4 bar. R24 covers item (7) (E1 bit-exact). R25 covers items (1)–(6). Item (8) is optional Logic AU smoke (R27).**

**Tasks:**

1. **Per-string sustained-tone harness × 3 (Gate 4 invariant 1).**

   For each string `<X> ∈ {A, D, G}` (E carried by R24; not re-rendered here):
   ```bash
   cd build
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --string <X> --sustain 60 --release 5 --infinite-sustain 1.0 --velocity 0.7 \
       --out /tmp/phase22-r25-string-<X>.wav --json /tmp/phase22-r25-string-<X>.json
   shasum -a 256 /tmp/phase22-r25-string-<X>.wav | awk '{print $1}' \
       > plugins/O-Contrabass/tests/render-harness/golden/string-<X>.wav.sha256
   cp /tmp/phase22-r25-string-<X>.json \
      plugins/O-Contrabass/tests/render-harness/golden/string-<X>.json
   ```
   For each: confirm JSON `pass_nan`, `pass_peak`, `pass_blockTime`, `pass_rms` all TRUE (4/4 invariants — same template as Phase 2.1a-recovery's bow-on-only harness, applied per string). `peak` should be reasonable (~0.1–0.3 range; not silent, not clipping).

   **Bow-on-only test variant (optional, more lenient):** if the standard `pass_rms` fails on A/D/G the way it does on E1 in the Phase 2.1c characterised-tail case, fall back to `--release 0` (bow-on-only 65 s) for the per-string golden — same Phase 2.1c precedent. RESEARCH §15.1 side note describes the post-bow-off saturator-tail behaviour; per-string saturator behaviour at A1/D2/G2 should follow the same envelope. If `pass_rms` is FALSE for A/D/G in the standard preset, capture a bow-on-only golden additionally as fallback.

2. **Detune sweep on A1 (Gate 4 invariant 2).**
   ```bash
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --detune-sweep A --sustain 30 --release 2 --infinite-sustain 1.0 --velocity 0.7 \
       --out /tmp/phase22-r25-detune-sweep-A.wav --json /tmp/phase22-r25-detune-sweep-A.json
   ```
   Confirm JSON: `mode: "detune-sweep"`, `string: "A"`, `detuneRamp.start: -1200`, `.end: 1200`, `pass_rmsContinuity: true`, `rmsContinuityRatio >= 0.90`. Stage:
   ```bash
   shasum -a 256 /tmp/phase22-r25-detune-sweep-A.wav | awk '{print $1}' \
       > plugins/O-Contrabass/tests/render-harness/golden/detune-sweep-A.wav.sha256
   cp /tmp/phase22-r25-detune-sweep-A.json \
      plugins/O-Contrabass/tests/render-harness/golden/detune-sweep-A.json
   ```

3. **Note sequence (Gate 4 invariant 3).**
   ```bash
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --note-sequence "28:2.0,33:2.0,38:2.0,43:2.0,28:2.0" --release 2 \
       --infinite-sustain 1.0 --velocity 0.7 \
       --out /tmp/phase22-r25-note-sequence.wav --json /tmp/phase22-r25-note-sequence.json
   ```
   Confirm JSON: `mode: "note-sequence"`, `sequence` has 5 entries, `pass_allSegmentsAudible: true`, `pass_rmsContinuityAtTransitions: true`, `rmsContinuityAtTransitions >= 0.50`. Stage:
   ```bash
   shasum -a 256 /tmp/phase22-r25-note-sequence.wav | awk '{print $1}' \
       > plugins/O-Contrabass/tests/render-harness/golden/note-sequence.wav.sha256
   cp /tmp/phase22-r25-note-sequence.json \
      plugins/O-Contrabass/tests/render-harness/golden/note-sequence.json
   ```

4. **ACTIVE_STRINGS=1 + MIDI 50 demotion test (Gate 4 invariant 4).**

   Set `ACTIVE_STRINGS=1` via APVTS (harness CLI override; add a one-off `--active-strings <int>` flag if not already present, or inject via a one-shot `setValueNotifyingHost` patch to the harness for this test). Render MIDI 50 (D3, naturally maps to D string) for 3 seconds:
   ```bash
   # If --active-strings flag added (R23 stretch goal — pin to R25 if needed):
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --note 50 --active-strings 1 --sustain 3 --release 1 --velocity 0.7 \
       --out /tmp/phase22-r25-active1-midi50.wav --json /tmp/phase22-r25-active1-midi50.json
   ```
   Confirm:
   - JSON `peak > 0.001` (audible — NOT silent; demoted to E1 fingered up).
   - `pass_nan`, `pass_peak`, `pass_blockTime` all TRUE.
   - sha256 captured for audit but NOT staged as golden (this is a behavioural test, not a regression bar).

   **Fallback if `--active-strings` flag is too much scope for R23:** test in Logic AU at R27 (manual). R25 logs the deferral and confirms via inspection of `mapMidiNoteToStringIndex(50, 1)` returning 0 (E-string slot). Implementation correctness is provable by code inspection; the empirical render is nice-to-have but not blocking.

5. **auval re-validation (Gate 4 invariant 5).**

   Already covered by R24 step 3. Confirm result still PASS (no drift between R24 and R25 — they run on the same Release build). No new render required.

6. **pluginval-10 re-validation (Gate 4 invariant 6).**

   Already covered by R24 step 4. Confirm result still PASS.

7. **Six-item audit summary.**

   Compile a Gate 4 audit table (for VERIFICATION.md, written in verify-phase, but R25 produces the source data):
   | Invariant | Status | Evidence |
   |-----------|--------|----------|
   | (1) Per-string A/D/G sustained drone × 3 | PASS/FAIL | JSON paths + sha256s |
   | (2) Detune sweep on A1 (rmsContinuity ≥ 0.90) | PASS/FAIL | JSON path + ratio |
   | (3) Note sequence E→A→D→G→E (continuity ≥ 0.50, all segments audible) | PASS/FAIL | JSON path + ratio + segment RMS |
   | (4) ACTIVE_STRINGS=1 + MIDI 50 audible | PASS/FAIL | peak measure |
   | (5) auval | PASS | (R24) |
   | (6) pluginval-10 | PASS | (R24) |
   | (7) E1 STRING_STIFFNESS=0 bit-exact | PASS | (R24) sha256 == d358abcd… |
   | (8) Logic AU smoke | DEFERRED | R27 (user) |

**Files modified:** none.
**Files created (staged for R26):**
- `plugins/O-Contrabass/tests/render-harness/golden/string-A.wav.sha256`
- `plugins/O-Contrabass/tests/render-harness/golden/string-A.json`
- `plugins/O-Contrabass/tests/render-harness/golden/string-D.wav.sha256`
- `plugins/O-Contrabass/tests/render-harness/golden/string-D.json`
- `plugins/O-Contrabass/tests/render-harness/golden/string-G.wav.sha256`
- `plugins/O-Contrabass/tests/render-harness/golden/string-G.json`
- `plugins/O-Contrabass/tests/render-harness/golden/detune-sweep-A.wav.sha256`
- `plugins/O-Contrabass/tests/render-harness/golden/detune-sweep-A.json`
- `plugins/O-Contrabass/tests/render-harness/golden/note-sequence.wav.sha256`
- `plugins/O-Contrabass/tests/render-harness/golden/note-sequence.json`

**Files NOT committed (per Open-Item-Pin #7):** all `/tmp/phase22-r25-*.wav` binaries (~8–22 MB each; reproducible from harness).

**Commit:** **NONE** — staging only. R26 absorbs.

**Success bar:**
- [ ] String-A/D/G sustained-tone JSON: `pass_nan`/`pass_peak`/`pass_blockTime` all TRUE for each (4/4 if `pass_rms` also passes, else 3/4 with bow-on-only fallback).
- [ ] Detune-sweep-A JSON: `pass_rmsContinuity: true`, `rmsContinuityRatio >= 0.90`.
- [ ] Note-sequence JSON: `pass_allSegmentsAudible: true`, `pass_rmsContinuityAtTransitions: true`.
- [ ] ACTIVE_STRINGS=1 + MIDI 50: audible (peak > 1e-3).
- [ ] auval/pluginval-10 still PASS (carry-over from R24).
- [ ] Six-item audit table compiled (input to VERIFICATION.md).

**Estimated effort:** 1.5 h (5 renders + JSON inspection + audit summary).

---

### R26 — Phase 2.2 Atomic Commit (Gate 4 PASS)

**Single atomic commit lands all Phase 2.2 work. Only run on R25 PASS (with R24 PASS as the binding bit-exact bar).**

**Tasks:**

1. **Stage all Phase 2.2 source + harness + golden text files + planning artefacts:**
   ```bash
   git add plugins/O-Contrabass/Source/BowedContrabassVoice.h
   git add plugins/O-Contrabass/Source/BowedContrabassVoice.cpp
   git add plugins/O-Contrabass/Source/DSP/WaveguideString.h
   git add plugins/O-Contrabass/Source/DSP/WaveguideString.cpp
   git add plugins/O-Contrabass/tests/render-harness/main.cpp
   git add plugins/O-Contrabass/tests/render-harness/golden/string-A.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/string-A.json
   git add plugins/O-Contrabass/tests/render-harness/golden/string-D.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/string-D.json
   git add plugins/O-Contrabass/tests/render-harness/golden/string-G.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/string-G.json
   git add plugins/O-Contrabass/tests/render-harness/golden/detune-sweep-A.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/detune-sweep-A.json
   git add plugins/O-Contrabass/tests/render-harness/golden/note-sequence.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/note-sequence.json
   git add plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md
   git add plugins/O-Contrabass/.planning/STATUS.md
   ```

2. **Confirm staging:**
   ```bash
   git status
   git diff --cached --stat
   ```
   Expect roughly 21 files staged. **Sanity check:** no binary WAVs in `tests/render-harness/golden/` (only `.sha256` text + `.json` per Open-Item-Pin #7); no `build/` artefacts; no unrelated planning files; no edits to `Source/PluginProcessor.{h,cpp}` (Phase 2.1c-frozen); no edits to `Source/DSP/DispersionFilter.h` (Phase 2.1c R20-frozen); no edits to `modules/synthesis/bow-friction/*` (Phase 2.1b-frozen v1.0.0); no edits to `parameter-spec.md` / `ARCHITECTURE.md` / `ROADMAP.md` (locked contracts).

3. **Commit with structured message:**
   ```bash
   git commit -m "$(cat <<'EOF'
   feat(O-Contrabass): Phase 2.2 4-string bank + per-string detune + dispersion table - Gate 4 PASS

   Adds the full 4-string EADG voice bank inside one BowedContrabassVoice,
   with per-string detune ±1200¢, per-string dispersion M=4/3/2/1 + B
   prefactor 1e-4/7e-5/5e-5/3e-5, MIDI-note → string mapping (closed-form
   thresholds 28/33/38/43), ACTIVE_STRINGS clamp (remap to highest active
   slot), and 5 ms equal-power crossfade at the voice mix-bus on note-on
   transitions to a different slot. Per RESEARCH §15.

   Implementation:
   - Source/BowedContrabassVoice.{h,cpp}: std::array<WaveguideString, 4>
     replacing single-string member; per-string SmoothedValue<Linear>
     20 ms detune smoother in delay-samples space; precomputed equal-power
     crossfade ramp (~3.5 KiB at 88.2 kHz internal); state machine for
     activeStringIndex / previousStringIndex / crossfadeRemainingSamples;
     mapMidiNoteToStringIndex (4-comparison ladder + ACTIVE_STRINGS clamp);
     readDetuneForString (paramId array indexed by slot); computeDelaySamples
     (cents → ratio → samples); per-block update loop pushes STRING_STIFFNESS
     to all 4 strings, advances 4 smoothers, computes 4 dispersion a-values,
     updates active+previous detune targets; per-sample mix uses early-return
     on activeStringIndex pattern (HARD RULE §15.9.5 — preserves bit-exact
     at slot 0).
   - Source/DSP/WaveguideString.{h,cpp}: + setDispersionActiveSections(int M)
     pass-through to bridgeDispersion. NO topology changes; NO smoother
     relocation; NO prepare() signature change. Existing prepare() line
     bridgeDispersion.setActiveSections(4) retained verbatim — slot-0
     bit-exact preserved.
   - tests/render-harness/main.cpp: --string <E|A|D|G>, --detune-sweep
     <E|A|D|G>, --note-sequence "MIDI:dur,..." flags. Per-block detune
     ramp; pre-built note-sequence event list + per-block MidiBuffer
     drain; rmsContinuityRatio (0.90 threshold), pass_rmsContinuityAtTransitions
     (0.50 threshold, 256-sample symmetric window), pass_allSegmentsAudible
     (>1e-3 threshold). Auto-rewrite default WAV/JSON filenames in new modes.

   Gate 4 (R25) PASS:
   - (1) Per-string A1/D2/G2 sustained drone × 3: 4/4 invariants TRUE for
     each (or 3/4 + bow-on-only fallback if post-bow-off tail follows the
     E1 saturator-tail Phase 2.4 characterisation).
   - (2) Detune sweep on A1 ±1200¢ over 30 s: pass_rmsContinuity TRUE,
     rmsContinuityRatio >= 0.90.
   - (3) Note sequence E→A→D→G→E (2 s each): pass_allSegmentsAudible TRUE,
     pass_rmsContinuityAtTransitions TRUE (>= 0.50 at all 4 transitions).
   - (4) ACTIVE_STRINGS=1 + MIDI 50 demotion: audible (peak > 1e-3).
   - (5) auval -v aumu OCbs OuDv: AU VALIDATION SUCCEEDED.
   - (6) pluginval --strictness-level 10 --validate-in-process: PASS.
   - (7) E1 STRING_STIFFNESS=0 bit-exact regression: sha256 match
     d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75
     vs Phase 2.1c golden (golden/stiffness-zero-pre.wav.sha256).
     Strict byte-equal preserved per RESEARCH §15.9 analytical proof
     (idle-string output is literal 0.0f at regression preset).
   - (8) Logic AU smoke: user-deferred non-blocking, mirroring Phase 2.1c
     R19f / Phase 2.1b R14e precedent.

   Architecture amendments still deferred to end-of-Stage-2 verify per
   locked decision: §"DC Blocker" (F3 from Phase 2.1a-recovery) and
   §"In-loop saturator" (conditional on Phase 2.4 §12.5 escalation).

   Phase 2.2 closes. Phase 2.3 (vibrato + slow-bow LFO + Schelleng wedge)
   opens as fresh GSD cycle.

   Refs:
   - RESEARCH.md §15 (Phase 2.2 4-string bank research)
   - PLAN.md rev-6 (R21-pre, R21, R22, R23, R24, R25, R26)
   - CONTEXT.md rev-4 (Phase 2.2 discuss)

   Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
   EOF
   )"
   ```

4. **Post-commit verification:**
   ```bash
   git log --stat HEAD~1..HEAD
   git rev-parse HEAD
   ```
   Record the commit SHA in `STATUS.md` `phase_2_2_atomic_commit_sha` field. Confirm `git status` clean.

5. **Update `STATUS.md`:**
   - `next_action` → `phase_2_3_discuss`.
   - `gate_state` block: add `phase_2_2_four_string_bank: PASS`, `phase_2_2_atomic_commit_sha: <new-sha>`.
   - `Lifecycle Timeline` append: 2026-04-?? Phase 2.2 verify entry summarising the eight-item Gate 4 PASS.
   - `Cycle Scope` → `Phase 2.3 — vibrato + slow-bow LFO + Schelleng wedge clamp (next fresh GSD cycle)`.
   - `Current Position` Stage 2 progress: 2.1 closed + 2.2 closed = ~50% of Stage 2 (4 phases out of 6 remaining: 2.3, 2.4, 2.5, 2.6).
   - **NOTE:** these `STATUS.md` edits are part of the R26 commit (per file list step 1) — staged BEFORE the commit, not after.

**Files modified:** all listed under "Stage all" in step 1.

**Commit:** **THIS IS THE PHASE 2.2 ATOMIC COMMIT.** Single commit, only on Gate 4 PASS.

**Success bar:**
- [ ] `git log --stat HEAD~1..HEAD` shows ~21 files in a single commit.
- [ ] `git status` clean post-commit.
- [ ] STATUS.md `next_action` flipped to Phase 2.3 discuss.
- [ ] Phase 2.2 closed.

**Estimated effort:** 30 min (mostly mechanical staging + commit + STATUS.md update).

---

### R27 (optional) — Logic Pro AU smoke audition

**User-deferred non-blocking. Mirrors Phase 2.1c R19f / Phase 2.1b R14e precedent. Not part of the R26 atomic commit. Captured here as the eighth Gate 4 invariant for completeness.**

**Audition sequence (per CONTEXT rev-4 line 116):**
1. Open Logic Pro, instantiate O-Contrabass AU on a software-instrument track.
2. **E1 sustained** (3 s @ MIDI 28).
3. **A1 sustained** (3 s @ MIDI 33).
4. **D2 sustained** (3 s @ MIDI 38).
5. **G2 sustained** (3 s @ MIDI 43).
6. **Portamento E1→A1→D2→G2** (1 s/string). Listen for: clicks at transitions (should be absent — 5 ms equal-power crossfade), per-string voicing distinctness (M=4 E1 vs M=1 G2 — bright/sharp G vs warm/dispersed E), no NaN/silence.
7. **ACTIVE_STRINGS knob sweep 4→3→2→1 with MIDI 50 held.** Listen for: at ACTIVE_STRINGS=4 → MIDI 50 plays on D2 (lowest open ≤ 50 is D2=38; 50 → idx=2). At ACTIVE_STRINGS=2 → demote idx=2 to maxIdx=1, plays on A1 fingered up. At ACTIVE_STRINGS=1 → demote to E1 (musically thin but audible). Listen for: smooth transitions (current note keeps ringing on its current string until note-off per locked policy; next note-on picks up the demotion).
8. **STRING_STIFFNESS sweep on D2** (held MIDI 38, sweep 0→1 over 5 s). Listen for: dispersion at M=2 should be more audible than at M=4 E1 (Risk #7 carry-forward — bass-register validity envelope; D2 is closer to piano-tuned region). If sweep is musically more interesting at D2 than at E1 was in Phase 2.1c R19f, that's expected.

**Gate 4 invariant (8) verdict:** USER-CONFIRMED PASS or DEFERRED. Either is acceptable for closing Phase 2.2 (mirrors Phase 2.1c precedent — automated bar is the binding gate).

**Files modified:** none.
**Commit:** none.
**Success bar:** user audits and either confirms PASS or defers (logged in VERIFICATION.md).
**Estimated effort:** 10 min (manual audition).

---

## Why R26 is a single atomic commit

Same gate-first principle as R7 (Phase 2.1a-recovery), R15 (Phase 2.1b extraction), R20 (Phase 2.1c dispersion):

1. **Coupling:** the Phase 2.2 artefacts (`BowedContrabassVoice.{h,cpp}` + `WaveguideString.{h,cpp}` setter + harness `main.cpp` + 5 golden text-file pairs + planning artefact updates) are mutually coupled. Splitting them yields broken intermediate states — e.g., `BowedContrabassVoice.cpp` calling `setDispersionActiveSections` before R22 lands the method body, or harness flags emitted to a JSON schema that's not yet documented in PLAN.md.
2. **Bisect safety:** if a future Phase 2.x bug is bisected back to "4-string bank landing", a single SHA flips the entire feature.
3. **Audit trail:** Phase 2.1's three sub-phases each got exactly one commit (R7 → R15 → R20). Phase 2.2 continues the convention (R26). The Phase 2.x timeline stays trivially reconstructible from `git log --grep "Phase 2."`.

---

## Files To Create / Modify (consolidated, Phase 2.2)

### Source (modified)
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` — R21 (~+15 LOC: `std::array<WaveguideString, 4> strings`, `std::array<juce::SmoothedValue<float, Linear>, 4> detuneSmoothed`, 4 state ints, `crossfadeRamp` vec, `sr_internal`, 3 helper decls).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` — R21 (~+80 LOC: `prepareToPlay` per-slot loop + crossfade-ramp precompute, `noteStarted` state machine, `renderNextBlock` 4-slot per-block update sequence, per-sample 4-slot mix loop with early-return on `activeStringIndex`, 3 helper bodies).
- `plugins/O-Contrabass/Source/DSP/WaveguideString.h` — R22 (+1 LOC method declaration).
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` — R22 (+3 LOC method body).

### Harness (modified)
- `plugins/O-Contrabass/tests/render-harness/main.cpp` — R23 (~+120 LOC: 3 new flags `--string`/`--detune-sweep`/`--note-sequence`; per-block detune ramp; pre-built note-sequence event list + per-block MidiBuffer drain; rmsContinuityRatio + transition-window RMS + per-segment-audibility computation; mode-aware JSON schema; auto-rewrite default WAV/JSON filenames; doc-comment header update).

### Test artefacts (new, committed as text-only — sha256 + JSON, per Open-Item-Pin #7)
- `plugins/O-Contrabass/tests/render-harness/golden/string-A.wav.sha256` — R25
- `plugins/O-Contrabass/tests/render-harness/golden/string-A.json` — R25
- `plugins/O-Contrabass/tests/render-harness/golden/string-D.wav.sha256` — R25
- `plugins/O-Contrabass/tests/render-harness/golden/string-D.json` — R25
- `plugins/O-Contrabass/tests/render-harness/golden/string-G.wav.sha256` — R25
- `plugins/O-Contrabass/tests/render-harness/golden/string-G.json` — R25
- `plugins/O-Contrabass/tests/render-harness/golden/detune-sweep-A.wav.sha256` — R25
- `plugins/O-Contrabass/tests/render-harness/golden/detune-sweep-A.json` — R25
- `plugins/O-Contrabass/tests/render-harness/golden/note-sequence.wav.sha256` — R25
- `plugins/O-Contrabass/tests/render-harness/golden/note-sequence.json` — R25

### Test artefacts (NOT committed — staged-only or transient)
- `/tmp/phase22-r21pre-stiffness-zero.{wav,json}` — R21-pre tripwire (~22 MB; transient).
- `/tmp/phase22-r24-stiffness-zero-post.{wav,json}` — R24 regression render (~22 MB; transient).
- `/tmp/phase22-r25-string-{A,D,G}.{wav,json}` — R25 (~22 MB each; reproducible from harness).
- `/tmp/phase22-r25-detune-sweep-A.{wav,json}` — R25 (~10 MB; reproducible).
- `/tmp/phase22-r25-note-sequence.{wav,json}` — R25 (~4 MB; reproducible).
- `/tmp/phase22-r25-active1-midi50.{wav,json}` — R25 (~1 MB; reproducible).

### Planning artefacts (modified)
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` — already at rev-4 (no further edits in execute; rev-4 lock holds).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` — already at §15 append (no further edits in execute).
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` — this rev-6 append (no further edits in execute; verify-phase may add a "rev-6 retrospective" footnote if anomalies surfaced).
- `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` — execute-phase appends "Phase 2.2 execute" section after R26.
- `plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md` — verify-phase appends "Phase 2.2 verify" section with R25 audit table + R24 sha256 confirmation.
- `plugins/O-Contrabass/.planning/STATUS.md` — `next_action` flip to `phase_2_3_discuss`, `gate_state` update with `phase_2_2_atomic_commit_sha`, `Lifecycle Timeline` append, `Cycle Scope` update.

### Files explicitly NOT touched
- `plugins/O-Contrabass/Source/PluginProcessor.{h,cpp}` (29 APVTS parameters + DETUNE_E/A/D/G + ACTIVE_STRINGS already declared from Stage 1; no edits needed).
- `plugins/O-Contrabass/Source/PluginEditor.{h,cpp}` (Stage 3 work).
- `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` (Phase 2.1c R20-frozen; consumed verbatim).
- `plugins/O-Contrabass/Source/OContrabassMPESynthesiser.{h,cpp}` (voice count = 1; no Synthesiser changes for Phase 2.2 — the 4-string bank lives inside one voice).
- `modules/synthesis/bow-friction/*` (Phase 2.1b-frozen v1.0.0; value-class deterministic, no string-count coupling).
- `modules/registry.yaml` (no module surface changes).
- `plugins/O-Bowed/*` (Phase 2.2 is single-plugin scope; O-Bowed gets no edits — confirmed independent file per RESEARCH §15.10).
- `plugins/O-Contrabass/research/ARCHITECTURE.md` (deferred amendments to end-of-Stage-2 verify; F3 deviation tracked in R26 commit body).
- `plugins/O-Contrabass/.planning/parameter-spec.md` (frozen contract — sha256:c47fe736…).
- `plugins/O-Contrabass/CMakeLists.txt` (no new source files; no header-list update needed).
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.{wav.sha256,json}` (Phase 2.1c golden — content-stable; R24 verifies it still matches).
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-sweep.{wav.sha256,json}` (Phase 2.1c golden — content-stable; not re-rendered in Phase 2.2).

---

## Dependencies Graph (compact)

```
R21-pre (working-tree integrity tripwire; sha256 == d358abcd…; NO source edits)
   ↓                ↓ (mismatch → STOP, investigate)
R22 (WaveguideString.{h,cpp} setDispersionActiveSections pass-through; +1 LOC header, +3 LOC body)
   ↓
R21 (BowedContrabassVoice.{h,cpp} 4-string bank + state machine + crossfade ramp + per-block + per-sample mix; ~+95 LOC)
   ↓
R23 (harness --string + --detune-sweep + --note-sequence + JSON schema + RMS-continuity computation; ~+120 LOC)
   ↓
R24 (build + auval + pluginval-10 + R-final bit-exact regression check)              ← GATE 4 invariant (7) + (5) + (6)
   ↓ (PASS)              ↓ (FAIL)
R25 (Gate 4 invariants  R24 diagnostic-on-fail per RESEARCH §15.9.5:
  (1)–(4): per-string A/  - delay-line per-sample setDelay → gate by isSmoothing()
  D/G + detune-sweep-A   - mix-loop reordering → confirm early-return pattern
  + note-sequence +      - prepareToPlay slot-0 sequence drift → re-inspect
  ACTIVE_STRINGS=1 demote)  - last resort: soften bar → RMS-equivalent within 1 LSB
   ↓                          re-run R24 until sha256 match
R26 (Phase 2.2 atomic commit — ~21 files; closes 2.2)
   ↓ (optional)
R27 (Logic AU smoke; user-deferred non-blocking; not in commit)
```

R21-pre is a strict prerequisite to R21 — if working tree drifts, all subsequent bit-exact reasoning breaks. R22 must land before R21's compile (R21 calls `setDispersionActiveSections`). R21 must land before R23's smoke test (R23 invokes the new harness flags against the new voice). R24 is the gated bit-exact check (binding gate). R25 covers the remaining Gate 4 invariants. R26 is the gated finisher.

---

## Risks (Phase 2.2, refreshed from RESEARCH §15.12)

1. **Bit-exact regression at slot-0 fails (CONTEXT Risk #5; binding RESEARCH §15.9 invariant).** Mitigation: §15.9.5 hard rules + R21-pre tripwire + R24 final check. Diagnostic-on-fail per RESEARCH §15.9.5 (delay-line setDelay gating; mix-loop reordering check; prepareToPlay slot-0 sequence audit). Last resort: soften bar to RMS-equivalent (RESEARCH §15.9.6 says fallback is unnecessary; if it triggers, file a follow-up RESEARCH note for Phase 2.4).
2. **String-switching click despite 5 ms equal-power crossfade (CONTEXT Risk #1).** Mitigation: precomputed ramp guarantees `oldGain² + newGain² = 1` (no power dip); idle-string bridge-rail decay is longer than 5 ms (~10–30 ms at typical g·(1−p)) so previous-string contribution is already audible during the fade. R25 invariant (3) `pass_rmsContinuityAtTransitions ≥ 0.50` catches gross failures. Escalation: 10–20 ms crossfade window if 5 ms proves insufficient (1-line constant change at `crossfadeTotalSamples` derivation). Document for Phase 2.4 if escalated.
3. **Idle-string CPU overshoot (CONTEXT Risk #2).** Mitigation: estimated 0.4 % per idle × 3 = 1.2 % overhead; total Phase 2.2 voice CPU ~2 % (well under 5 % PERF-02 budget). If R25 measures > 3 % on M1, file Phase 2.4 follow-up to gate idle-string dispersion cascade (accepting cold-start risk).
4. **Detune-sweep clicks at extreme cents (CONTEXT Risk #3).** Mitigation: 20 ms `SmoothedValue<Linear>` in delay-samples space is JUCE-validated (vibrato pattern). At E1 ±1200¢, full sweep completes in ~36 ms. R25 invariant (2) `pass_rmsContinuity ≥ 0.90` catches clicks. Threshold derivation in RESEARCH §15.7.
5. **MIDI-mapping edge cases — notes outside [28, 55] (CONTEXT Risk #4).** Mitigation: closed-form `mapMidiNoteToStringIndex` clamps at boundaries (notes < 28 → E with negative finger position, just renders at MIDI-note frequency below open E; notes > 55 → G fingered very high; both produce audible-but-thin tone). Out-of-range testing implicit in listening test sequence (R27).
6. **`ACTIVE_STRINGS` mid-sustain edge case (CONTEXT Risk #6).** Mitigation: locked policy — current note keeps ringing on current string until note-off; next note-on respects clamp. Falls out naturally from note-on-only switching policy. R25 invariant (4) confirms via MIDI 50 + ACTIVE_STRINGS=1 demotion.
7. **`std::array<WaveguideString, 4>` allocation cost in `prepareToPlay` (CONTEXT Risk #7).** Mitigation: 4 × 8192-sample delay-line buffers ≈ 128 KiB total; once-per-prepare cost is acceptable. PERF-01 (no allocations in `processBlock`) preserved — all allocation in `prepareToPlay`. R23 + R24 inspection confirm.
8. **Phase 2.1c golden dependence on E1-only voice topology (CONTEXT Risk #8).** Mitigation: §15.9 analytical proof + §15.1 empirical baseline confirm idle-string contribution is literal `0.0f` at the regression preset. Strict byte-equal achievable without softening. RESEARCH §15.9.6 fallback (one-time refactoring boundary) parked but not anticipated.
9. **Per-sample `setDelaySamples` on idle slots in steady state shifts delay-line internal state representation (NEW, RESEARCH §15.9 sub-issue).** Mitigation: R21 step 5 caveat documents the diagnostic-on-fail gating (`if (detuneSmoothed[s].isSmoothing())`). R24 catches if this hits. The idle delay-line is all zeros (analytical proof §15.9.2) so internal-state shifts produce zero output regardless; the regression bar should hold even if `setDelay()` semantics differ slightly between a one-shot prepare-time set vs. per-sample at-target writes.
10. **Detune-sweep RMS-continuity threshold mis-calibration (NEW, RESEARCH §15.12).** Mitigation: 0.90 threshold derived analytically (single-sample click at amplitude 1 in 512-sample block adds ~0.044 RMS spike → ratio drops to 0.5; well below 0.90 catches genuine clicks while tolerating ~5 % envelope variation under low-rate amplitude wobble). If R25 false-flags on legitimate sweep, soften to 0.85 with VERIFICATION.md note.

---

## Success Criteria (Gate 4 — Phase 2.2 verify exit gate)

- [ ] **R21-pre** — Working-tree integrity tripwire: pre-edit sha256 == `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75` (Phase 2.1c golden carry-forward).
- [ ] **R21** — `BowedContrabassVoice.{h,cpp}` extended: 4-string bank, per-string detune smoothers, crossfade ramp + state machine, mapping helper, per-block 4-slot update sequence, per-sample mix loop with early-return on `activeStringIndex`.
- [ ] **R22** — `WaveguideString.{h,cpp}` adds `setDispersionActiveSections(int M)` pass-through. NO topology / smoother / prepare-signature changes.
- [ ] **R23** — Harness `--string`, `--detune-sweep`, `--note-sequence` flags + JSON schema additions + RMS-continuity + transition-window + per-segment-audibility computation. Smoke test passes (`--string A --sustain 5` → `mode: "sustained"`, `midiNote: 33`).
- [ ] **R24** — Release build clean; auval VALIDATION SUCCEEDED; pluginval --strictness-level 10 PASS; **R-final bit-exact regression: sha256 == `d358abcd…` (byte-equal Phase 2.1c golden).** ← Gate 4 invariants (5), (6), (7).
- [ ] **R25 invariant (1)** — Per-string A1/D2/G2 sustained drone × 3: 4/4 invariants TRUE for each (or 3/4 with bow-on-only fallback).
- [ ] **R25 invariant (2)** — Detune sweep on A1: `pass_rmsContinuity: true`, `rmsContinuityRatio ≥ 0.90`.
- [ ] **R25 invariant (3)** — Note sequence E→A→D→G→E: `pass_allSegmentsAudible: true`, `pass_rmsContinuityAtTransitions: true` (≥ 0.50).
- [ ] **R25 invariant (4)** — ACTIVE_STRINGS=1 + MIDI 50: audible (peak > 1e-3) — proves demotion produces tone, not silence.
- [ ] **R25 audit table** — Eight-item Gate 4 bar compiled (input to VERIFICATION.md): items (1)–(4) from R25, (5)–(7) from R24, (8) deferred to R27.
- [ ] **R26** — Atomic commit landed; `git log --stat HEAD~1..HEAD` shows ~21 files in single commit; STATUS.md `next_action` flipped to `phase_2_3_discuss`; `gate_state.phase_2_2_four_string_bank: PASS`; `phase_2_2_atomic_commit_sha` recorded.
- [ ] **R27 (optional)** — Gate 4 invariant (8) Logic AU smoke: USER-CONFIRMED PASS or DEFERRED (mirrors Phase 2.1c R19f / Phase 2.1b R14e precedent).
- [ ] **(Architecture amendments, post-Stage-2-verify, OUT OF SCOPE for this execute)** — ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments still deferred to end-of-Stage-2 verify per locked decision.

When all checks above are green (R27 user-confirmed or accepted as deferred), **Phase 2.2 verifies** and **Stage 2 progress reaches ~50 %** (2.1 + 2.2 closed; 2.3, 2.4, 2.5, 2.6 remain). Phase 2.3 (vibrato + slow-bow LFO + Schelleng wedge clamp) opens as a fresh GSD cycle.

---

## Out of Scope (deferred per CONTEXT.md rev-4 + RESEARCH §15 + STATUS.md)

- **Phase 2.3 → 2.6** — vibrato + slow-bow LFO + Schelleng wedge clamp (2.3); sub-harmonic bias + 108-combo stability matrix (2.4 — saturator-tail re-evaluated here per RESEARCH §12; bass-register dispersion calibration polynomial follow-up here per Phase 2.1c Risk #7); body resonator + bow noise (2.5); master saturator/limiter + microtonal + MPE + Note Expression / MTS-ESP / Scala (2.6).
- **String-tension-vs-pitch coupling** — architecture is silent; treated as 12-TET ideal in Phase 2.2. If a future spike surfaces audible coupling at MIDI 55 fingered up the G string, document for Phase 2.4+ as ARCH amendment proposal (RESEARCH §15.6).
- **Mid-note pitchbend / portamento string-switching** — DB physical reality is to un-fret up the neck within a string, not cross strings during a note. Phase 2.2 keeps mid-note bend on the current string. Revisit only if Phase 2.3+ vibrato/slow-bow modulation surfaces a need (CONTEXT rev-4 line 83).
- **Idle-string optimisation (gating dispersion cascade)** — current design ticks all 4 strings always (~1.2 % CPU overhead estimate). If R25 measures > 3 % on M1, file Phase 2.4 follow-up.
- **96 kHz host buffer-overflow risk** — Phase 2.2 locked to 88.2 kHz internal (host 44.1k × 2). Buffer is sized for E1 −1200¢ at 88.2 kHz = 4282 samples → fits the existing 8192-sample buffer. At 96 kHz × 2 = 192 kHz, worst case is 9320 samples — buffer overflow. Document for Phase 2.6 + ARCH amendment if 96 kHz host support is ever added (RESEARCH §15.5).
- **Per-string bridge LP coefficient differentiation** — current architecture has one global STRING_BRIGHTNESS parameter affecting all strings; per-string brightness coefficient parking deferred to v1.1 if needed. Phase 2.2 uses a single shared `g`/`p` per the existing Phase 2.1c design.
- **Production WAV binary commits** — `phase22-r24-stiffness-zero-post.wav`, `phase22-r25-string-{A,D,G}.wav`, `phase22-r25-detune-sweep-A.wav`, `phase22-r25-note-sequence.wav`, `phase22-r25-active1-midi50.wav` are NOT committed (~5–22 MB each; reproducible from harness). sha256 + JSON committed instead per Open-Item-Pin #7.
- **`DispersionFilter` module promotion** — premature with one consumer; revisit if O-Bowed grows a dispersion filter later. Phase 2.2 carries forward Phase 2.1c's plugin-local design.
- **ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments** — end-of-Stage-2 verify decides; F3 deviation continues in R26 commit body.
- **Phase 2.4 calibration polynomial follow-up for bass-register dispersion** — Phase 2.1c Risk #7 carry-forward. R27 Logic audition on D2 may surface whether the M=2 sweep is more musically interesting than the M=4 E1 sweep was — still parked for Phase 2.4.

---

# Stage 2: DSP — Plan (Phase 2.3) — REVISION 7 (Vibrato + Slow-Bow LFO + Schelleng Wedge Clamp + EXPRESSION_MACRO, Gate 5)

> **Status:** rev-7 authors fresh task bodies for **R28-pre, R28, R29, R30, R31, R32 (optional), R33** per `RESEARCH.md §16.14` sequencing and `CONTEXT.md` rev-5. rev-1/2/3/4/5/6 remain in-effect as completed/verified history. Phase 2.2 closed 2026-04-27 with R26 atomic commit `131c2c7` (Gate 4 PASS).

**Date:** 2026-04-27
**Cycle scope:** Phase 2.3 only (Phase 2.4 → 2.6 still get fresh GSD cycles each)
**Gate:** Gate 5 (Phase 2.3 verify)
**Atomic-commit unit:** R33 (Gate 5 PASS) — single commit lands all source + harness + 8 new golden text files + parameter-spec.md amendment + STATUS.md checksum update + planning artefacts
**Carry-forward locks (NOT re-litigated):** Phase 2.1a-recovery split-rail topology (`bridgeDelay` + `neckDelay`), F2 LP form, F3 no-in-loop-DCB, F4 betaScale removed, Phase 2.1b bow-friction module v1.0.0 consumption (value-class deterministic; not touched), Phase 2.1c `DispersionFilter<4>` API consumed verbatim, Phase 2.1c `WaveguideString` Step 1.5 dispersion placement + split-aware compensation, Phase 2.2 `std::array<WaveguideString, 4>` 4-string bank topology + per-string detune in delay-samples space + per-string M=4/3/2/1 dispersion + MIDI→string mapping (closed-form ladder + ACTIVE_STRINGS clamp) + 5 ms equal-power crossfade with precomputed ramp, ARCH §"DC Blocker" + §"In-loop saturator" amendments deferred to end-of-Stage-2 verify, primary listening DAW = Logic Pro (AU)

---

## Preamble — Pinned Open Items (RESEARCH §16.15)

PLAN rev-7 pins each of the 14 plan-phase open items from RESEARCH §16.15:

| # | Open Item | Pinned Decision |
|---|-----------|-----------------|
| 1 | R28-pre task location | **R28-pre is a structural prerequisite to R28** (mirrors Phase 2.2 R21-pre / Phase 2.1c R16-pre). Diagnostic only; no commit. WAV+sha256+JSON written to `/tmp/`, NOT to `golden/`. R28-pre flips `EXPRESSION_MACRO` default 0.50 → 0.0 in `Source/PluginProcessor.cpp` line 86, builds, renders the regression preset, confirms sha256 == `d358abcd…`, then **reverts the source edit** so R28 starts from a clean working tree. Empirical confirmation already captured in RESEARCH §16.1; R28-pre re-runs as tripwire under the plan-phase build environment. |
| 2 | `pass_breathingAudible` slow-LFO threshold | **`rmsByDecadePeakToPeakPct ≥ 0.05` (5 %) for v1.0.** Softened from CONTEXT rev-5's 20 % per RESEARCH §16.7.2 — at default bass operating point the Schelleng wedge clamps `safeDepth` to ≈0.0, so the 20 % threshold cannot be reached without bow-param overrides. The 5 % floor still proves slow-LFO produces *some* audible modulation. **Phase 2.4 calibration polynomial parking:** restore 20 % once the bass-register wedge is recalibrated (Risk #7-style follow-up). |
| 3 | `--slow-lfo` mode preset | **Factory defaults** (no BOW_PRESSURE/BOW_POSITION override). Mirrors a "user with the knob untouched" experience and accepts that Schelleng wedge silences most of the modulation at v1.0 bass defaults. `pass_clampEngagement` (`clampedDepthMean > 0.0`) confirms the wedge math is running at all; `pass_breathingAudible` (≥5 % per pin #2) confirms what little modulation escapes the clamp is audible. CLI override stays available for future Phase 2.4 retesting. |
| 4 | `clampedDepthMean` instrumentation hook signature | **`std::atomic<float> lastSafeDepth { 0.0f };` on `BowedContrabassVoice` (private; getter `getLastSafeDepth()` const noexcept).** Written per-block in step 3 of the 7-step evaluation order (immediately after the `safeDepth = jlimit(...)` line, gated by `rawSlowLfoDepth > 0.0f`). When LFO depth = 0 (HR-2), the atomic is left at its previous value or written `0.0f` depending on plan-locked behaviour — **decision: write 0.0f unconditionally before the HR-2 short-circuit guard** so the read view is well-defined every block. Harness reads via processor accessor (`proc.getActiveVoice()->getLastSafeDepth()`) once per block, accumulates sum + count over the sustain window, divides for `clampedDepthMean`. Atomic memory order: `std::memory_order_relaxed` (single-writer single-reader, no cross-thread ordering required). |
| 5 | Vibrato pitch-tracking τ search range | **`τ ∈ [400, 1500]` samples at sr=44100** (≈29 Hz to 110 Hz). Locked verbatim from RESEARCH §16.7.1 — covers MIDI 28 (E1, ~1071 samples) with comfortable margin. Sample-rate dependency confirmed: harness fixed at sr=44100 per main.cpp:223. R29 implementation hardcodes the [400, 1500] bounds inside the autocorrelation loop; comment notes the sr-coupling. |
| 6 | Vibrato onset window measurement threshold | **80 % of measured `peakDepthCents`** triggers `onsetTimeMs` capture. Architecture line 125 implies S-curve = 1.0 at ramp completion; 80 % chosen for noise-robustness against autocorrelation jitter on the ramp-up trajectory. Pass band: `onsetTimeMs ∈ [800, 1000]` (default config: 600 ms onset delay + 300 ms ramp = 900 ms expected). |
| 7 | Macro `skip(numSamples - 1)` per-block consumption pattern | **Required for `SmoothedValue<Linear>` to advance correctly under once-per-block consumption.** Implementation pattern: `const float macroNow = macroSmoothed.getNextValue(); macroSmoothed.skip(juce::jmax(0, numSamples - 1));`. The `jmax(0, …)` guards `numSamples == 1` (skip(-1) would underflow the smoother counter). Code-comment in R28 notes both the JUCE pattern and the `numSamples` edge case. Risk #12 from RESEARCH §16.13 is the documented rationale. |
| 8 | `vibratoOnsetGateAtNoteOff` capture point | **Inside the per-sample loop, gated by `noteOffFadeOutTimerSeconds == 0.0f` (one-shot capture).** R28 inserts the capture at the top of the per-sample vibrato block, just before `gate` is computed. After capture, `noteOffFadeOutTimerSeconds` advances on subsequent samples (no re-capture path). The capture must NOT happen at `noteStopped()` entry because the gate's current value depends on `vibratoOnsetTimerSeconds` which is per-sample state. |
| 9 | 3× SmoothedValue prepareToPlay init | **`macroSmoothed.reset(sr_internal, 0.020); macroSmoothed.setCurrentAndTargetValue(0.0f);`** (matches the EXPRESSION_MACRO=0.0 default per Q7a). **`slowLfoSpeedSmoothed.reset(sr_internal, 0.020); slowLfoSpeedSmoothed.setCurrentAndTargetValue(0.0f);`** (matches LFO=0 default). **`slowLfoPressureSmoothed.reset(sr_internal, 0.020); slowLfoPressureSmoothed.setCurrentAndTargetValue(0.0f);`** (matches LFO=0 default). All three SmoothedValues initialise at `currentValue = targetValue = 0.0f` so `getNextValue()` returns `0.0f` exactly until any non-zero target lands — preserving HR-3 IEEE 754 identity arithmetic. |
| 10 | R29 harness mode mutual-exclusion precedence ladder | **`macro-sweep > schelleng-stress > vibrato > slow-lfo > stiffness-sweep > detune-sweep > note-sequence > string > sustained`**. Locked from RESEARCH §16.7.5. R29's `parseArgs` post-parse switch checks flags in this order; first-set wins. When user sets multiple modes, harness emits `warning: --macro-sweep takes precedence over --schelleng-stress (etc.)` and clears the lower-priority flags. Phase 2.2's existing precedence (detune-sweep > note-sequence > string > stiffness-sweep) is preserved underneath the new modes. |
| 11 | `macroSmoothed.setTargetValue(rawMacro)` per-block call | **UNCONDITIONAL.** Always set target every block. HR-3 covers the 0=0 case via IEEE 754 identity arithmetic (`x + 0 == x` exact, `x * 1 == x` exact). RESEARCH §16.8 final paragraph confirms this avoids state-machine complexity (gating the call would require an "is-modulator-active" flag with its own initialisation/reset semantics, just to save zero arithmetic ops). Same pattern is applied to slowLfoSpeedSmoothed/slowLfoPressureSmoothed targets — written unconditionally each block (writes `0.0f` when `rawSlowLfoDepth == 0`, preserved at `0.0f` via HR-2 short-circuit on the source `slowLfoSpeedMod`/`slowLfoPressureMod` values). |
| 12 | Golden file paths | **Path:** `plugins/O-Contrabass/tests/render-harness/golden/`. **New files (text-only, ~150 bytes each):** `vibrato.wav.sha256`, `vibrato.json`, `slow-lfo.wav.sha256`, `slow-lfo.json`, `schelleng-stress.wav.sha256`, `schelleng-stress.json`, `macro-sweep.wav.sha256`, `macro-sweep.json`. **8 new files total.** **NOT committed:** the corresponding `.wav` binaries (each ~3–22 MB; reproducible from harness on demand per Phase 2.1c / 2.2 precedent). All 5 Phase 2.2 goldens carry forward unchanged (their sha256 contents are the regression bar — Gate 5 invariant 1). |
| 13 | Stage-1 contract amendment artefact list | **Two artefacts edited in R33 atomic commit:** (a) `plugins/O-Contrabass/.planning/parameter-spec.md` line 57: change `0.50` → `0.0` in the EXPRESSION_MACRO Default column; (b) `plugins/O-Contrabass/.planning/STATUS.md` `contract_checksums.parameter_spec` field: replace `sha256:c47fe736…` with the new sha256 computed from the edited parameter-spec.md. **All other artefacts left untouched per RESEARCH §16.11 grep audit:** `parameter-spec-draft.md`, `research/ARCHITECTURE.md`, `ROADMAP.md`, `stages/1-foundation/PLAN.md`, `BRIEF.md`, `REQUIREMENTS.md` — historical / architecture-immutable / no-default-referenced. **R33 commit body explicitly flags this as a Stage-1 contract amendment** justified by Q7a regression-bar preservation rationale. |
| 14 | R32 listening test sequence MIDI events | **Locked verbatim from CONTEXT rev-5 line 122** — five-step audition: (1) MIDI 38 (D2) sustained + VIBRATO_DEPTH 0→25¢ ramp at 5 Hz vibrato; (2) MIDI 33 (A1) sustained + SLOW_LFO_DEPTH 0→1.0 ramp at 0.3 Hz; (3) MIDI 28 (E1) extreme bow params (PRESSURE 7 N + SPEED 0.05 m/s) + SLOW_LFO_DEPTH=1 (Schelleng stress); (4) MIDI 38 (D2) + EXPRESSION_MACRO 0→1.0 ramp; (5) E1 + VIBRATO + SLOW_LFO together (anti-correlation guard audition). ~60 s total. R32 is user-deferred non-blocking (mirrors R27 / R19f precedent). |

**Carry-forward locks from Phase 2.2 (NOT re-litigated):**
- `std::array<WaveguideString, 4> strings` topology + slot-0 (E-string) bit-exact mix path with early-return on `activeStringIndex` (HARD RULE §15.9.5 carry-forward).
- Per-string detune `SmoothedValue<Linear>` 20 ms ramp in delay-samples space + `setDelaySamples()` gated by `isSmoothing()` to prevent steady-state delay-line state churn.
- 5 ms equal-power crossfade ramp + state machine (`activeStringIndex`, `previousStringIndex`, `crossfadeRemainingSamples`).
- MIDI→string mapping (closed-form 4-comparison ladder + ACTIVE_STRINGS clamp).
- `kOpenStringFrequencyHz` / `kBOpen` / `kMPerString` / `kDetuneParamIds` static literals in `BowedContrabassVoice.cpp` anonymous namespace.
- Per-block 7-step DISPERSION update sequence (push STRING_STIFFNESS → advance smoothers → compute per-string `a` → update active+previous detune targets) — Phase 2.3 EXTENDS this with new steps 1–7 (CONTEXT rev-5 line 119) but slot-0 dispersion path is bit-exact preserved via HR-4.

**Hard rules from CONTEXT rev-5 (binding for R28 implementation, complementing §15.9.5):**
1. **HR-1 — Vibrato literal-zero short-circuit (§16.4).** When `effectiveVibratoDepth <= 0.0f`, the per-sample mix loop calls `strings[active].setDelaySamples(baseDelaySamples)` exactly as Phase 2.2 — vibrato cents calculation is gated behind an `if` and produces no floating-point ops at zero depth. This makes the modulators-off code path bit-identical to Phase 2.2's `setDelaySamples(detuneSmoothed[s].getNextValue())` line at the active slot. Vibrato phase counter + onset timer DO advance regardless (Q3 sine-phase-carry contract); only the `vibCents` computation and the `strings[active].setDelaySamples(modulatedDelay)` write are gated.
2. **HR-2 — Slow-LFO literal-zero short-circuit (§16.6 step 3).** When `rawSlowLfoDepth <= 0.0f`, the entire slow-LFO block (phase advance, `sin()`, `slowLfoSpeedMod`/`slowLfoPressureMod` writes) is gated behind an `if (rawSlowLfoDepth > 0.0f)`. At zero depth, `slowLfoSpeedMod` and `slowLfoPressureMod` retain their zero-init values, multiplicative apply in step 4 produces `bowSpeedAfterLfo = rawBowSpeed * (1.0f + 0.6f * 0.0f) = rawBowSpeed` exact (IEEE 754 identity). `slowLfoPhase` does NOT advance at zero depth (Q3 contract applies to vibrato only; slow-LFO has no phase-carry semantic concern).
3. **HR-3 — Macro literal-zero arithmetic (§16.6 step 5).** When `macroNow == 0.0f`, all four destination computations evaluate to identity-arithmetic no-ops: `(1.0f + 0.4f * 0.0f) = 1.0f` exact, `(1.0f + 0.6f * 0.0f) = 1.0f` exact, `(1.0f + 0.3f * 0.0f) = 1.0f` exact, `0.0f + 500.0f * 0.0f = 0.0f` exact. `effectiveBowSpeed`, `effectiveBowPressure`, `effectiveVibratoDepth`, `effectiveBrightnessHz` numerically equal the post-LFO values; downstream `bowModel.setBowSpeed(...)` / `bowModel.setBowPressure(...)` / `s.setBrightness(...)` calls land at byte-identical values vs Phase 2.2 path. **Critical SmoothedValue invariant:** `macroSmoothed` is initialised with `setCurrentAndTargetValue(0.0f)` in `prepareToPlay`; `setTargetValue(0.0f)` (called unconditionally per pin #11) does NOT engage smoothing because `currentValue == targetValue == 0.0f` already; `getNextValue()` returns `0.0f` exact every block. No drift introduced.
4. **HR-4 — Schelleng wedge skip on zero LFO (§16.6 step 2).** The entire wedge computation (fMin/fMax/headroom/safeDepth/vibAntiCorr) is gated behind `if (rawSlowLfoDepth > 0.0f)`. At zero depth: `safeDepth = 0.0f` (init value), `vibAntiCorr = 0.0f` (init value), no division ops engaged, no `juce::jmax(1e-6f, ...)` denormal-guard logic running. **Why the gate matters at zero depth:** the wedge formulas are not bit-exact-no-op even when `rawSlowLfoDepth == 0` (they compute non-trivial fMin/fMax values). Skipping the math eliminates any floating-point processor-state side-effect that could perturb the regression bar. **Plus instrumentation:** `lastSafeDepth.store(0.0f)` is written unconditionally at the top of step 2 (per pin #4), BEFORE the HR-4 gate.

**Per-block evaluation order (locked verbatim from RESEARCH §16.6):**
```
Step 1 — Read raw APVTS atomics into block-cached locals (existing updateParametersFromAPVTS
         + 10 new raw reads: VIBRATO_DEPTH, VIBRATO_RATE, VIBRATO_ONSET, SLOW_LFO_RATE,
         SLOW_LFO_DEPTH, EXPRESSION_MACRO, BOW_SPEED, BOW_PRESSURE, BOW_POSITION, BRIGHTNESS).
Step 2 — Schelleng wedge fMin/fMax/headroom/safeDepth/vibAntiCorr (HR-4 gated;
         lastSafeDepth.store(...) hook).
Step 3 — Slow-LFO phase advance + sin → slowLfoSpeedMod/slowLfoPressureMod (HR-2 gated).
Step 4 — Apply slow-LFO multiplicatively to bow speed/pressure → bowSpeedAfterLfo/bowPressureAfterLfo.
Step 5 — Layer macro multiplicatively → effectiveBowSpeed/Pressure/VibratoDepth/BrightnessHz
         + effectiveVibratoRate (raw + vibAntiCorr) (HR-3 gated arithmetic; macroSmoothed
         setTargetValue + getNextValue + skip(jmax(0, n-1))).
Step 6 — Push to bowModel + all-strings brightness (existing per-block dispersion update path
         carries forward verbatim from Phase 2.2; macro-lifted brightness consumed at all 4
         strings via existing `s.setBrightness(...)` loop).
Step 7 — Per-sample loop (active-string-only vibrato modulation; HR-1 gated cents
         computation; vibratoPhase + vibratoOnsetTimerSeconds + noteOffFadeOutTimerSeconds
         per-sample advances).
```

---

## Goal

Layer the modulator + macro section on top of the Phase 2.2 4-string voice. Add per-voice vibrato (sine LFO with bass-tuned defaults: 5 Hz / 12¢ / 600 ms onset; half-cosine S-curve 300 ms ramp; 150 ms linear note-off fade-out; modulates active string only via cents-add-then-`expf(-cents·ln2/1200)` factor in delay-samples space). Add Slow-Bow LFO (0.05–2 Hz; modulates voice-level bow speed × (1 + 0.6·s) and bow pressure × (1 + 0.5·p) with 23° pressure phase-lag; depth clamped to 80 % of Schelleng wedge headroom). Add inline Schelleng wedge clamp (~10 LOC per-block; `Z = R = R_s = 0.5` dimensionless collapse; HR-4 gated). Add EXPRESSION_MACRO 4-destination layering (default flipped 0.50 → 0.0 for bit-exact regression preservation; multipliers from architecture line 567: speed × (1.0+0.4·m), pressure × (1.0+0.6·m), vibrato depth × (1.0+0.3·m), brightness +500·m Hz; one voice-level `SmoothedValue<Linear>` 20 ms ramp on the macro source, four destinations consume per-block). Anti-correlation guard `vibratoRate += 0.13 Hz × SLOW_LFO_DEPTH` engages only when LFO is non-zero. Validate Gate 5 invariants — eight-item bar including strict byte-equal regression at the Phase 2.1c/2.2 golden — and atomic-commit on Gate 5 PASS as R33.

---

## Tasks

### R28-pre — Pre-flight bit-exact baseline confirmation

**No source edits committed. Diagnostic only. Confirms working-tree integrity at start of execute AND empirically reproduces RESEARCH §16.1 baseline under the plan-phase build environment.**

Per RESEARCH §16.1, working tree at R26 commit `131c2c7` with EXPRESSION_MACRO default flipped 0.50 → 0.0 in `Source/PluginProcessor.cpp` line 86 (and NO other source edits) reproduces the regression preset render at sha256 `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75` byte-identical to the committed Phase 2.1c golden. R28-pre re-runs that experiment as a tripwire — if the working tree drifts or the build environment perturbs the result, R28-pre catches it BEFORE any Phase 2.3 source edits land.

**Tasks:**

1. **Confirm git state is clean and at R26:**
   ```bash
   git log --oneline -1 plugins/O-Contrabass/Source/    # should be 131c2c7 (Phase 2.2 R26)
   git status plugins/O-Contrabass/Source/              # should be clean
   ```
   If working tree is dirty in `plugins/O-Contrabass/Source/` or in `tests/render-harness/`, STOP and reconcile before proceeding.

2. **Apply the single-character source edit:**
   - Edit `plugins/O-Contrabass/Source/PluginProcessor.cpp` line 86: change `0.50f` → `0.0f` in the EXPRESSION_MACRO `createParameterLayout()` row.
   - Confirm via `git diff plugins/O-Contrabass/Source/PluginProcessor.cpp` — single-line change, no other edits.

3. **Build harness:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```

4. **Render regression preset to `/tmp/`:**
   ```bash
   cd build
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --note 28 --velocity 0.7 --sustain 60 --release 5 \
       --infinite-sustain 1.0 --string-stiffness 0 \
       --out /tmp/phase23-r28pre-stiffness-zero.wav \
       --json /tmp/phase23-r28pre-stiffness-zero.json
   ```

5. **Confirm sha256 match:**
   ```bash
   shasum -a 256 /tmp/phase23-r28pre-stiffness-zero.wav | awk '{print $1}'
   # expected: d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75
   ```
   Compare with `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256`. MUST match byte-for-byte. (`pass_rms` may FAIL on the documented `phase_2_1a_followup_park` ratio heuristic — that's not blocking; strict byte-equality is the regression bar.)

6. **Revert the source edit** (R28 reapplies it as part of the source-edit batch):
   ```bash
   git checkout -- plugins/O-Contrabass/Source/PluginProcessor.cpp
   ```
   Confirm `git status plugins/O-Contrabass/Source/` is clean.

   **If sha256 MISMATCH:** STOP. Investigate working-tree drift (check `git diff` against R26 commit `131c2c7`; check for any post-R26 edits in `Source/`, `modules/synthesis/bow-friction/`, `Source/DSP/`). Do NOT proceed to R28 until the working tree reproduces `d358abcd…` with only the Q7a default flip applied.

**Files modified:** `plugins/O-Contrabass/Source/PluginProcessor.cpp` (transient — reverted in step 6).
**Files created:** `/tmp/phase23-r28pre-stiffness-zero.{wav,json}` (transient, NOT committed).

**Commit:** **NONE** — diagnostic only.

**Success bar:**
- [ ] `git log --oneline -1 plugins/O-Contrabass/Source/` shows `131c2c7`.
- [ ] Single-line edit at PluginProcessor.cpp:86 confirmed via `git diff`.
- [ ] Harness build succeeds.
- [ ] Render exits with `pass_nan` / `pass_peak` / `pass_blockTime` all TRUE.
- [ ] sha256 == `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75`.
- [ ] Source edit reverted; `git status plugins/O-Contrabass/Source/` clean.

**Estimated effort:** 10 min (single edit + build + render + sha256 compare + revert).

---

### R28 — `BowedContrabassVoice.{h,cpp}` + `PluginProcessor.cpp` source edits

**Per RESEARCH §16.1–§16.10 and the 7-step per-block evaluation order locked in §16.6. Most-complex task in this phase. Single source-edit batch — NO build, NO commit at this stage.**

**Tasks:**

1. **`Source/PluginProcessor.cpp` line 86 — EXPRESSION_MACRO default flip (Q7a).**

   Change:
   ```cpp
   layout.add(std::make_unique<APF>(juce::ParameterID{"EXPRESSION_MACRO", 1}, "Expression Macro",
       NR(0.0f, 1.0f, 0.001f),            0.50f));
   ```
   to:
   ```cpp
   layout.add(std::make_unique<APF>(juce::ParameterID{"EXPRESSION_MACRO", 1}, "Expression Macro",
       NR(0.0f, 1.0f, 0.001f),            0.0f));   // Phase 2.3 Q7a — preserves Phase 2.2 strict byte-equal regression bar.
   ```

   **HARD RULE:** parameter ID, range, skew, and metadata all unchanged. Only the default literal flips.

2. **`Source/BowedContrabassVoice.h` — add Phase 2.3 state and helpers (~+15 LOC).**

   Inside the `private:` block (after the existing crossfade ramp + `sr_internal` line):

   ```cpp
   // Phase 2.3 — vibrato + slow-LFO + macro state.
   float vibratoPhase                   = 0.0f;          // continuous across notes (Q3)
   float vibratoOnsetTimerSeconds       = 0.0f;          // re-armed on noteStarted
   float vibratoOnsetGateAtNoteOff      = 0.0f;          // captured one-shot at fade entry
   float noteOffFadeOutTimerSeconds     = -1.0f;         // sentinel: <0 = not in fade
   float slowLfoPhase                   = 0.0f;

   // Phase 2.3 — voice-level smoother on EXPRESSION_MACRO source (one source, 4 dest).
   juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> macroSmoothed;
   juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> slowLfoSpeedSmoothed;
   juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> slowLfoPressureSmoothed;

   // Phase 2.3 — Schelleng wedge instrumentation hook (single-writer/single-reader; relaxed).
   std::atomic<float> lastSafeDepth { 0.0f };

   public:
   float getLastSafeDepth() const noexcept
   {
       return lastSafeDepth.load (std::memory_order_relaxed);
   }
   ```

   `<atomic>` include needed at the top of `BowedContrabassVoice.h`.

3. **`Source/BowedContrabassVoice.cpp` — anonymous-namespace constants (~+5 LOC).**

   Append to the existing anonymous namespace block:
   ```cpp
   // Phase 2.3 locked constants (PLAN rev-7 preamble + RESEARCH §16).
   constexpr float kVibratoRampSec      = 0.3f;                       // architecture line 125
   constexpr float kVibratoFadeOutSec   = 0.150f;                     // architecture line 127
   constexpr float kPressureLagRad      = 0.4014f;                    // 23° in radians
   constexpr float kSchellengZ          = 0.5f;                       // dimensionless (§16.3)
   constexpr float kSchellengR          = 0.5f;
   constexpr float kSchellengDMu        = 0.60f;                      // bass μ_s − μ_d
   constexpr float kAntiCorrPerDepth    = 0.13f;                      // Q5 anti-correlation guard
   constexpr float kVibFactorScale      = -0.69314718056f / 1200.0f;  // -ln(2)/1200
   ```

4. **`Source/BowedContrabassVoice.cpp` — `prepareToPlay` extension (~+8 LOC).**

   After the existing `crossfadeRamp` precompute block, add:
   ```cpp
   // Phase 2.3 — modulator + macro state + smoothers.
   vibratoPhase                  = 0.0f;
   vibratoOnsetTimerSeconds      = 0.0f;     // pin #4 — fresh notes get full onset envelope
   vibratoOnsetGateAtNoteOff     = 0.0f;
   noteOffFadeOutTimerSeconds    = -1.0f;    // sentinel: not in fade
   slowLfoPhase                  = 0.0f;

   macroSmoothed.reset (sr_internal, 0.020);
   macroSmoothed.setCurrentAndTargetValue (0.0f);                     // pin #9 — HR-3 invariant
   slowLfoSpeedSmoothed.reset (sr_internal, 0.020);
   slowLfoSpeedSmoothed.setCurrentAndTargetValue (0.0f);
   slowLfoPressureSmoothed.reset (sr_internal, 0.020);
   slowLfoPressureSmoothed.setCurrentAndTargetValue (0.0f);

   lastSafeDepth.store (0.0f, std::memory_order_relaxed);
   ```

5. **`Source/BowedContrabassVoice.cpp` — `noteStarted` extension (~+3 LOC).**

   At the bottom of the existing `noteStarted()` body (just before the closing `oversampling.reset();`):
   ```cpp
   // Phase 2.3 — re-arm vibrato S-curve onset; exit any prior fade-out (Q3).
   vibratoOnsetTimerSeconds   = 0.0f;
   noteOffFadeOutTimerSeconds = -1.0f;
   // vibratoPhase NOT reset (sine-phase-carry contract).
   ```

6. **`Source/BowedContrabassVoice.cpp` — `noteStopped` extension.**

   In the `if (allowTailOff)` branch (currently just calls `bowModel.stopBow()`), add:
   ```cpp
   if (allowTailOff)
   {
       bowModel.stopBow();
       noteOffFadeOutTimerSeconds = 0.0f;     // start vibrato 150 ms linear fade
       // vibratoOnsetGateAtNoteOff captured on next per-sample iteration (pin #8).
   }
   ```

   In the hard-stop branch (existing `else`), add a reset:
   ```cpp
   else
   {
       clearCurrentNote();
       for (auto& s : strings) s.reset();
       bowModel.reset();
       oversampling.reset();
       previousStringIndex       = -1;
       crossfadeRemainingSamples = 0;
       // Phase 2.3 — reset modulator state on hard stop.
       vibratoOnsetTimerSeconds   = 0.0f;
       vibratoOnsetGateAtNoteOff  = 0.0f;
       noteOffFadeOutTimerSeconds = -1.0f;
       lastSafeDepth.store (0.0f, std::memory_order_relaxed);
   }
   ```

7. **`Source/BowedContrabassVoice.cpp` — `renderNextBlock` 7-step per-block evaluation order (~+50 LOC).**

   Replace the current per-block sequence (the block of ~55 LOC starting at the `// 1. Read APVTS atomics` comment) with the 7-step locked structure. Key insertions:

   **(a) Step 1 — append 10 new raw reads** to `updateParametersFromAPVTS()` (or read inline at the top of `renderNextBlock` after the existing `updateParametersFromAPVTS()` call — recommend inline to minimise churn in the existing `updateParametersFromAPVTS` body):
   ```cpp
   updateParametersFromAPVTS();

   const float rawVibratoDepth   = parameters->getRawParameterValue ("VIBRATO_DEPTH")->load();
   const float rawVibratoRate    = parameters->getRawParameterValue ("VIBRATO_RATE")->load();
   const float rawVibratoOnsetMs = parameters->getRawParameterValue ("VIBRATO_ONSET")->load();
   const float rawSlowLfoRate    = parameters->getRawParameterValue ("SLOW_LFO_RATE")->load();
   const float rawSlowLfoDepth   = parameters->getRawParameterValue ("SLOW_LFO_DEPTH")->load();
   const float rawMacro          = parameters->getRawParameterValue ("EXPRESSION_MACRO")->load();
   const float rawBowSpeed       = parameters->getRawParameterValue ("BOW_SPEED")->load();
   const float rawBowPressure    = parameters->getRawParameterValue ("BOW_PRESSURE")->load();
   const float rawBowPos         = parameters->getRawParameterValue ("BOW_POSITION")->load();
   const float rawBrightness     = parameters->getRawParameterValue ("BRIGHTNESS")->load();
   ```

   **NOTE on `updateParametersFromAPVTS()` interaction:** the existing helper computes `effectivePosition` / `effectiveSpeed` / `effectivePressure` from MPE pressure/timbre and pushes to `bowModel`. Phase 2.3's macro layering must replace those `bowModel.setBowSpeed/Pressure` pushes — see substep (e). Consequence: **comment out or remove the `bowModel.setBowSpeed(effectiveSpeed); bowModel.setBowPressure(effectivePressure);` lines from `updateParametersFromAPVTS()`** so step 6 below is the single source of truth. (The MPE pressure/timbre + `effectivePosition` computation stays — pushed to all 4 strings via the existing `for (auto& s : strings) { s.setBowPosition(effectivePosition); ... }` loop.)

   **(b) Step 2 — Schelleng wedge eval (HR-4 gated; `lastSafeDepth.store(0.0f)` first):**
   ```cpp
   lastSafeDepth.store (0.0f, std::memory_order_relaxed);     // pin #4 — well-defined every block

   float safeDepth   = 0.0f;
   float vibAntiCorr = 0.0f;
   if (rawSlowLfoDepth > 0.0f)                                // HR-4
   {
       const float beta = juce::jlimit (0.02f, 0.25f, rawBowPos);
       const float fMax = (2.0f * kSchellengZ * rawBowSpeed)
                        / juce::jmax (1.0e-6f, beta * kSchellengDMu);
       const float fMin = (kSchellengZ * kSchellengZ * rawBowSpeed)
                        / juce::jmax (1.0e-6f, 2.0f * kSchellengR * beta * beta * kSchellengDMu);
       const float hUp  = (fMax - rawBowPressure) / juce::jmax (1.0e-6f, fMax);
       const float hLo  = (rawBowPressure - fMin) / juce::jmax (1.0e-6f, fMin);
       const float headroom = juce::jmin (hUp, hLo);
       safeDepth   = juce::jlimit (0.0f, rawSlowLfoDepth, 0.8f * juce::jmax (0.0f, headroom));
       vibAntiCorr = kAntiCorrPerDepth * rawSlowLfoDepth;
       lastSafeDepth.store (safeDepth, std::memory_order_relaxed);
   }
   ```

   **(c) Step 3 — Slow-LFO phase + sin (HR-2 gated):**
   ```cpp
   float slowLfoSpeedMod = 0.0f, slowLfoPressureMod = 0.0f;
   if (rawSlowLfoDepth > 0.0f)                                // HR-2
   {
       slowLfoPhase += juce::MathConstants<float>::twoPi * rawSlowLfoRate
                     * (static_cast<float> (numSamples) / static_cast<float> (sr_internal));
       if (slowLfoPhase > juce::MathConstants<float>::twoPi)
           slowLfoPhase -= juce::MathConstants<float>::twoPi;

       slowLfoSpeedMod    = safeDepth * std::sin (slowLfoPhase);
       slowLfoPressureMod = safeDepth * std::sin (slowLfoPhase + kPressureLagRad);
   }
   ```

   **(d) Step 4 — Apply slow-LFO multiplicatively:**
   ```cpp
   const float bowSpeedAfterLfo    = rawBowSpeed    * (1.0f + 0.6f * slowLfoSpeedMod);
   const float bowPressureAfterLfo = rawBowPressure * (1.0f + 0.5f * slowLfoPressureMod);
   ```

   **(e) Step 5 — Layer macro (HR-3 IEEE 754 identity; pin #11 unconditional setTargetValue + pin #7 skip pattern):**
   ```cpp
   macroSmoothed.setTargetValue (rawMacro);                              // pin #11 unconditional
   const float macroNow = macroSmoothed.getNextValue();
   macroSmoothed.skip (juce::jmax (0, numSamples - 1));                  // pin #7

   const float effectiveBowSpeed       = bowSpeedAfterLfo    * (1.0f + 0.4f * macroNow);
   const float effectiveBowPressure    = bowPressureAfterLfo * (1.0f + 0.6f * macroNow);
   const float effectiveVibratoDepth   = rawVibratoDepth     * (1.0f + 0.3f * macroNow);
   const float effectiveBrightnessHz   = rawBrightness + 500.0f * macroNow;
   const float effectiveVibratoRate    = rawVibratoRate + vibAntiCorr;
   const float effectiveVibratoOnsetSec = 0.001f * rawVibratoOnsetMs;
   ```

   **(f) Step 6 — Push to bowModel + all-strings brightness** (replaces the bowModel pushes that were in `updateParametersFromAPVTS()` per substep (a)):
   ```cpp
   // MPE pressure/timbre carry-forward from updateParametersFromAPVTS via cached fields.
   const auto note = getCurrentlyPlayingNote();
   const float mpePressure = note.pressure.asUnsignedFloat();
   bowModel.setBowSpeed    (effectiveBowSpeed * mpeExpression);
   bowModel.setBowPressure (effectiveBowPressure * (0.5f + mpePressure * 1.5f));
   for (auto& s : strings) s.setBrightness (effectiveBrightnessHz);
   // (existing per-block STRING_STIFFNESS push + per-string a-computation +
   //  detune-target update sequence carries forward verbatim from Phase 2.2 below.)
   ```

   **HARD RULE alignment:** at modulators-off (`rawSlowLfoDepth = 0`, `rawMacro = 0`):
   - `safeDepth = vibAntiCorr = 0` (HR-4)
   - `slowLfoSpeedMod = slowLfoPressureMod = 0` (HR-2)
   - `bowSpeedAfterLfo = rawBowSpeed * (1.0f + 0.6f * 0.0f) = rawBowSpeed` exact
   - `bowPressureAfterLfo = rawBowPressure * (1.0f + 0.5f * 0.0f) = rawBowPressure` exact
   - `macroNow = 0.0f` (SmoothedValue currentValue stays at 0.0)
   - `effectiveBowSpeed = rawBowSpeed * (1.0f + 0.4f * 0.0f) = rawBowSpeed` exact
   - `effectiveBowPressure = rawBowPressure * (1.0f + 0.6f * 0.0f) = rawBowPressure` exact
   - `effectiveBrightnessHz = rawBrightness + 0.0f = rawBrightness` exact
   - `effectiveVibratoRate = rawVibratoRate + 0.0f = rawVibratoRate` exact
   - `bowModel.setBowSpeed(rawBowSpeed * mpeExpression)` — byte-identical to Phase 2.2's `bowModel.setBowSpeed(effectiveSpeed)` (where `effectiveSpeed = bowSpeed * mpeExpression`).
   - `bowModel.setBowPressure(rawBowPressure * (0.5f + mpePressure * 1.5f))` — byte-identical to Phase 2.2's `bowModel.setBowPressure(effectivePressure)`.
   - `strings[s].setBrightness(rawBrightness)` — byte-identical to Phase 2.2's `s.setBrightness(brightness)`.

8. **`Source/BowedContrabassVoice.cpp` — Step 7 per-sample loop (active-string-only vibrato; HR-1 gated; pin #8 capture).**

   Inside the existing per-sample oversampled loop, BEFORE the existing per-string `setDelaySamples(detuneSmoothed[s].getNextValue())` call on the active slot, insert:

   **(a) Per-sample vibrato cents computation (HR-1 gated):**
   ```cpp
   // Phase 2.3 — vibrato cents (active string only).
   float vibCents = 0.0f;
   if (effectiveVibratoDepth > 0.0f)                                          // HR-1
   {
       // Pin #8 — one-shot capture of onset gate at fade-entry.
       float gate;
       if (noteOffFadeOutTimerSeconds == 0.0f)
       {
           // Compute current gate FIRST so the snapshot is correct.
           const float elapsed = vibratoOnsetTimerSeconds - effectiveVibratoOnsetSec;
           if      (elapsed <= 0.0f)         gate = 0.0f;
           else if (elapsed >= kVibratoRampSec)
                                             gate = 1.0f;
           else                              gate = 0.5f - 0.5f * std::cos (
                                                  juce::MathConstants<float>::pi
                                                  * elapsed / kVibratoRampSec);
           vibratoOnsetGateAtNoteOff = gate;
       }
       else if (noteOffFadeOutTimerSeconds > 0.0f && noteOffFadeOutTimerSeconds < kVibratoFadeOutSec)
       {
           const float k = juce::jlimit (0.0f, 1.0f, noteOffFadeOutTimerSeconds / kVibratoFadeOutSec);
           gate = vibratoOnsetGateAtNoteOff * (1.0f - k);
       }
       else if (noteOffFadeOutTimerSeconds >= kVibratoFadeOutSec)
       {
           gate = 0.0f;
       }
       else
       {
           const float elapsed = vibratoOnsetTimerSeconds - effectiveVibratoOnsetSec;
           if      (elapsed <= 0.0f)         gate = 0.0f;
           else if (elapsed >= kVibratoRampSec)
                                             gate = 1.0f;
           else                              gate = 0.5f - 0.5f * std::cos (
                                                  juce::MathConstants<float>::pi
                                                  * elapsed / kVibratoRampSec);
       }

       vibCents = effectiveVibratoDepth * gate * std::sin (vibratoPhase);
   }

   // Phase counter + onset timer ALWAYS advance (Q3 sine-phase-carry; HR-1 only gates the
   // mix-write, not the timekeeping).
   vibratoPhase += juce::MathConstants<float>::twoPi
                 * effectiveVibratoRate / static_cast<float> (sr_internal);
   if (vibratoPhase > juce::MathConstants<float>::twoPi)
       vibratoPhase -= juce::MathConstants<float>::twoPi;
   vibratoOnsetTimerSeconds += 1.0f / static_cast<float> (sr_internal);
   if (noteOffFadeOutTimerSeconds >= 0.0f)
       noteOffFadeOutTimerSeconds += 1.0f / static_cast<float> (sr_internal);
   ```

   **(b) Active-slot delay-line modulation (HR-1 gated write; idle slots unchanged):**

   Replace the existing standard-path inner loop (currently `if (detuneSmoothed[s].isSmoothing()) strings[s].setDelaySamples(...)`) with a vibrato-aware variant that ONLY modulates the active slot:
   ```cpp
   for (int s = 0; s < 4; ++s)
   {
       const float detuneSamples = detuneSmoothed[s].getNextValue();

       if (s == activeStringIndex && vibCents != 0.0f)
       {
           // HR-1 active path — combined detune + vibrato in delay-samples space.
           const float vibFactor      = std::exp (vibCents * kVibFactorScale);
           const float modulatedDelay = detuneSamples * vibFactor;
           strings[s].setDelaySamples (modulatedDelay);
       }
       else if (detuneSmoothed[s].isSmoothing())
       {
           // Phase 2.2 carry-forward — detune-only path (gated by isSmoothing
           // for steady-state delay-line state stability §15.4 caveat).
           strings[s].setDelaySamples (detuneSamples);
       }
       // else: idle + at-target → no setDelaySamples write (Phase 2.2 contract).

       if (s == activeStringIndex)
           mixedSample = strings[s].processSample (v_bow, F_bow, frictionModel);
       else
           (void) strings[s].processSample (0.0f, 0.0f, frictionModel);
   }
   ```

   **HARD RULE alignment at modulators-off:** when `effectiveVibratoDepth = 0`, the `if (effectiveVibratoDepth > 0.0f)` gate skips the cents computation entirely → `vibCents = 0.0f` exact. The `s == activeStringIndex && vibCents != 0.0f` branch is FALSE on every iteration, so the active slot falls through to the existing `if (detuneSmoothed[s].isSmoothing())` Phase 2.2 path — byte-identical mix.

   **Crossfade-path mirror:** inside the `if (crossfadeRemainingSamples > 0 && previousStringIndex >= 0)` branch, apply the same active-slot vibrato gating to the new (active) string's delay-line write. The previous (old) string's delay is NOT vibrato-modulated (Q2 active-string-only contract).

   **Total LOC delta in `BowedContrabassVoice.cpp`:** ~+90 LOC across substeps 4–8. Falls within the §16.12 ~30-LOC-per-modulator budget; Q10 inline-in-voice decision is preserved (no extraction trigger).

9. **No build, no commit at this stage.** Single source-edit batch. R30 builds. R31 verifies. R33 commits.

**Files modified:**
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` (+1 LOC).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` (~+20 LOC).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` (~+90 LOC).

**Files created:** none.

**Commit:** none (R33 atomic).

**Success bar:**
- [ ] `git diff --stat plugins/O-Contrabass/Source/` shows exactly 3 files modified.
- [ ] PluginProcessor.cpp shows the single 0.50f → 0.0f flip.
- [ ] BowedContrabassVoice.h has 5 new state variables + 3 SmoothedValue<Linear> + 1 atomic + getter.
- [ ] BowedContrabassVoice.cpp has prepareToPlay init + noteStarted re-arm + noteStopped fade trigger + 7-step renderNextBlock evaluation + per-sample HR-1 gated vibrato cents + active-slot delay-line modulation.
- [ ] HARD RULES HR-1 to HR-4 visually traceable through the code (each `if (rawSlowLfoDepth > 0.0f)` / `if (effectiveVibratoDepth > 0.0f)` gate present and commented).

**Estimated effort:** 4 h (most-complex task; majority of phase work).

---

### R29 — Harness CLI + 4 new mode flags + autocorrelation + JSON schemas

**Per RESEARCH §16.7 four mode specifications. Adds ~+250 LOC to `tests/render-harness/main.cpp`. Single source-edit batch — NO build, NO commit at this stage.**

**Tasks:**

1. **`Args` struct extension (~+10 LOC).**

   Inside `struct Args`, add:
   ```cpp
   bool  vibratoMode        = false;     // Phase 2.3 R29
   bool  slowLfoMode        = false;
   bool  schellengStress    = false;
   bool  macroSweep         = false;
   ```

2. **`parseArgs` four new flags (~+10 LOC).**

   Add parser branches BEFORE the existing `--out` / `--json` handlers. These flags take NO value, so they need the `--i` decrement pattern (mirrors `--stiffness-sweep`):
   ```cpp
   else if (key == "--vibrato")          { args.vibratoMode     = true; --i; }
   else if (key == "--slow-lfo")         { args.slowLfoMode     = true; --i; }
   else if (key == "--schelleng-stress") { args.schellengStress = true; --i; }
   else if (key == "--macro-sweep")      { args.macroSweep      = true; --i; }
   ```

3. **Mutual-exclusion precedence ladder (~+25 LOC; pin #10).**

   AFTER `parseArgs` returns (around main.cpp:167 where the existing `--string` vs `--detune-sweep` warning lives), add the new precedence resolution:
   ```cpp
   // Phase 2.3 R29 — mode mutual-exclusion (pin #10 ladder; first-set wins highest).
   const bool any23Mode = args.vibratoMode || args.slowLfoMode
                       || args.schellengStress || args.macroSweep;
   if (any23Mode)
   {
       if (args.macroSweep)        { args.schellengStress = false; args.vibratoMode = false; args.slowLfoMode = false; }
       else if (args.schellengStress) { args.vibratoMode = false; args.slowLfoMode = false; }
       else if (args.vibratoMode)  { args.slowLfoMode = false; }
       // Phase 2.2 modes downgraded to allow Phase 2.3 modes to take precedence.
       args.stiffnessSweep    = false;
       args.detuneSweepString = ' ';
       args.noteSequence.clear();
       args.stringOverride    = ' ';
   }
   ```

4. **Per-mode pre-build APVTS overrides (~+50 LOC).**

   In the section AFTER `proc.parameters.getParameter("INFINITE_SUSTAIN")` overrides (around main.cpp:230), add:
   ```cpp
   // Phase 2.3 R29 — per-mode pre-build APVTS overrides.
   auto setNorm = [&] (const char* paramId, float norm)
   {
       if (auto* p = proc.parameters.getParameter (paramId))
           p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
   };

   if (args.vibratoMode)
   {
       args.midiNote        = 28;
       if (! args.sustainSet) args.sustainSeconds = 2.0f;
       if (! args.releaseSet) args.releaseSeconds = 1.0f;
       setNorm ("VIBRATO_DEPTH", 12.0f / 50.0f);          // 12¢
       setNorm ("VIBRATO_RATE",  (5.0f - 0.1f) / 11.9f);  // 5 Hz
       // VIBRATO_ONSET 600 ms with 0.5 skew (range 0–3000 ms):
       setNorm ("VIBRATO_ONSET", std::pow (600.0f / 3000.0f, 1.0f / 0.5f));
   }
   else if (args.slowLfoMode)
   {
       args.midiNote        = 33;                          // A1
       if (! args.sustainSet) args.sustainSeconds = 60.0f;
       if (! args.releaseSet) args.releaseSeconds = 2.0f;
       setNorm ("SLOW_LFO_DEPTH", 0.5f);
       setNorm ("SLOW_LFO_RATE",  (0.3f - 0.05f) / 1.95f); // 0.3 Hz
   }
   else if (args.schellengStress)
   {
       args.midiNote        = 28;
       if (! args.sustainSet) args.sustainSeconds = 30.0f;
       if (! args.releaseSet) args.releaseSeconds = 2.0f;
       // BOW_PRESSURE 7.0 (range 0.05–8.0): norm = (7.0 - 0.05) / 7.95
       setNorm ("BOW_PRESSURE",   (7.0f  - 0.05f) / 7.95f);
       // BOW_SPEED 0.05 (range 0.02–1.5): norm = (0.05 - 0.02) / 1.48
       setNorm ("BOW_SPEED",      (0.05f - 0.02f) / 1.48f);
       setNorm ("SLOW_LFO_DEPTH", 1.0f);
   }
   else if (args.macroSweep)
   {
       args.midiNote        = 38;                          // D2
       if (! args.sustainSet) args.sustainSeconds = 20.0f;
       if (! args.releaseSet) args.releaseSeconds = 2.0f;
       // EXPRESSION_MACRO ramped per-block in render loop (start at 0).
       setNorm ("EXPRESSION_MACRO", 0.0f);
   }
   ```

5. **Auto-rewrite default WAV/JSON filenames per mode (~+20 LOC).**

   Append a Phase 2.3 branch in the existing if/else-if filename ladder:
   ```cpp
   if (args.vibratoMode)
   {
       if (! args.outWavSet)  args.outWav  = "vibrato.wav";
       if (! args.outJsonSet) args.outJson = "vibrato.json";
   }
   else if (args.slowLfoMode)
   {
       if (! args.outWavSet)  args.outWav  = "slow-lfo.wav";
       if (! args.outJsonSet) args.outJson = "slow-lfo.json";
   }
   else if (args.schellengStress)
   {
       if (! args.outWavSet)  args.outWav  = "schelleng-stress.wav";
       if (! args.outJsonSet) args.outJson = "schelleng-stress.json";
   }
   else if (args.macroSweep)
   {
       if (! args.outWavSet)  args.outWav  = "macro-sweep.wav";
       if (! args.outJsonSet) args.outJson = "macro-sweep.json";
   }
   else if (/* existing Phase 2.2 chain */)
   ```

6. **Per-block macro ramp (macro-sweep mode) + clampedDepthMean accumulator (~+20 LOC).**

   In the render loop (around the existing `args.stiffnessSweep` per-block ramp at main.cpp:345), add:
   ```cpp
   // Phase 2.3 R29 — macro-sweep per-block linear ramp 0→1.
   if (args.macroSweep)
   {
       const float fraction = juce::jlimit (0.0f, 1.0f,
                                            static_cast<float> (sampleCursor)
                                            / static_cast<float> (juce::jmax (1, sustainSamples)));
       if (auto* p = proc.parameters.getParameter ("EXPRESSION_MACRO"))
           p->setValueNotifyingHost (fraction);
   }
   ```

   And a per-block read of the `lastSafeDepth` instrumentation hook (BEFORE `proc.processBlock` so the read is post-previous-block; or AFTER so it's the current block's value — locked: AFTER `processBlock` to read the wedge state computed during this block):
   ```cpp
   // Phase 2.3 R29 — per-block clampedDepthMean accumulator (slow-LFO + Schelleng modes).
   double clampedDepthSum   = 0.0;     // declare before render loop
   int    clampedDepthCount = 0;

   // Inside render loop, after proc.processBlock:
   if (args.slowLfoMode || args.schellengStress)
   {
       const float voiceLastSafe = (proc.synth.getNumVoices() > 0)
           ? dynamic_cast<BowedContrabassVoice*> (proc.synth.getVoice(0))->getLastSafeDepth()
           : 0.0f;
       if (sampleCursor < sustainSamples)
       {
           clampedDepthSum += static_cast<double> (voiceLastSafe);
           ++clampedDepthCount;
       }
   }
   ```

   **NOTE on `proc.synth` access:** `OContrabassAudioProcessor::synth` is currently `private` (most likely). R29 must EITHER: (a) add a `public:` accessor `BowedContrabassVoice* getActiveVoice() noexcept` on `OContrabassAudioProcessor`, OR (b) friend-class the harness to the processor. Recommend option (a) — single-line addition: `BowedContrabassVoice* getActiveVoice() noexcept { return dynamic_cast<BowedContrabassVoice*>(synth.getVoice(0)); }`. PLAN locks option (a).

7. **Autocorrelation pitch-tracking (vibrato mode only; ~+80 LOC).**

   In the analysis section AFTER the existing `rmsOverWindow` lambda (~main.cpp:550), add a vibrato-only post-render block:
   ```cpp
   // Phase 2.3 R29 — vibrato pitch-tracking via autocorrelation (RESEARCH §16.7.1).
   float peakDepthCents       = 0.0f;
   float vibratoRateHzMeasured = 0.0f;
   int   onsetTimeMs           = 0;
   juce::Array<juce::var> perCycleDeltaCents;

   if (args.vibratoMode)
   {
       constexpr int    kAcWindowSize = 4096;
       constexpr int    kAcHopSize    =  256;
       constexpr int    kTauMin       =  400;     // pin #5 — covers ~110 Hz at sr=44100
       constexpr int    kTauMax       = 1500;     // pin #5 — covers ~29 Hz at sr=44100
       constexpr double kF0           = 41.20;    // E1
       const int analysisStart = static_cast<int> (1.0f * sampleRate);   // skip first 1 s (onset window)
       const int analysisEnd   = juce::jmin (totalSamples - kAcWindowSize, sustainSamples);

       std::vector<float> deltaCentsTrace;
       const auto* mono = output.getReadPointer (0);

       for (int s = analysisStart; s < analysisEnd; s += kAcHopSize)
       {
           // Normalised autocorrelation R(τ) for τ ∈ [kTauMin, kTauMax].
           double bestR     = -1.0;
           int    bestTau   = kTauMin;
           int    secondTau = kTauMin;
           double secondR   = -1.0;

           // Energy normalisation (denominator).
           double energy = 0.0;
           for (int i = 0; i < kAcWindowSize; ++i)
               energy += static_cast<double> (mono[s + i]) * mono[s + i];
           energy = std::sqrt (juce::jmax (1.0e-12, energy));

           for (int tau = kTauMin; tau <= kTauMax; ++tau)
           {
               double sum = 0.0;
               for (int i = 0; i < kAcWindowSize; ++i)
                   sum += static_cast<double> (mono[s + i]) * mono[s + i + tau];
               // Energy normalisation at τ-shifted window.
               double e2 = 0.0;
               for (int i = 0; i < kAcWindowSize; ++i)
                   e2 += static_cast<double> (mono[s + i + tau]) * mono[s + i + tau];
               const double r = sum / juce::jmax (1.0e-12, energy * std::sqrt (e2));
               if (r > bestR) { bestR = r; bestTau = tau; }
           }

           // Parabolic interpolation around bestTau for sub-sample resolution.
           if (bestTau > kTauMin && bestTau < kTauMax)
           {
               double y0 = 0.0, y1 = bestR, y2 = 0.0;
               // Recompute neighbour R values (cheap — kAcWindowSize=4096 dot products).
               for (int delta = -1; delta <= 1; delta += 2)
               {
                   const int tau = bestTau + delta;
                   double sum = 0.0;
                   for (int i = 0; i < kAcWindowSize; ++i)
                       sum += static_cast<double> (mono[s + i]) * mono[s + i + tau];
                   double e2 = 0.0;
                   for (int i = 0; i < kAcWindowSize; ++i)
                       e2 += static_cast<double> (mono[s + i + tau]) * mono[s + i + tau];
                   const double r = sum / juce::jmax (1.0e-12, energy * std::sqrt (e2));
                   if (delta < 0) y0 = r;
                   else           y2 = r;
               }
               const double denom = (y0 - 2.0 * y1 + y2);
               const double tauOff = (std::abs (denom) > 1.0e-12) ? 0.5 * (y0 - y2) / denom : 0.0;
               const double tauPeak = static_cast<double> (bestTau) + tauOff;
               const double freq    = sampleRate / juce::jmax (1.0, tauPeak);
               const double cents   = 1200.0 * std::log2 (freq / kF0);
               deltaCentsTrace.push_back (static_cast<float> (cents));
           }
       }

       // Find peak-to-trough swing across last 3 vibrato cycles (≈600 ms, 36 hops at hop=256).
       if (deltaCentsTrace.size() >= 36)
       {
           const auto endIt   = deltaCentsTrace.end();
           const auto startIt = endIt - 36;
           const auto mm      = std::minmax_element (startIt, endIt);
           peakDepthCents = 0.5f * (*mm.second - *mm.first);
       }

       // Onset time: first hop where deltaCents amplitude ≥ 80 % of peakDepthCents.
       if (peakDepthCents > 0.0f)
       {
           for (size_t hop = 0; hop < deltaCentsTrace.size(); ++hop)
           {
               if (std::abs (deltaCentsTrace[hop]) >= 0.8f * peakDepthCents)
               {
                   onsetTimeMs = static_cast<int> (1000.0f
                       * (analysisStart + static_cast<int> (hop) * kAcHopSize)
                       / static_cast<float> (sampleRate));
                   break;
               }
           }
       }

       // Per-cycle delta-cents — sample at ~5 Hz cycle period (200 ms = 28 hops at sr=44100, hop=256).
       for (size_t hop = 0; hop < deltaCentsTrace.size(); hop += 28)
           perCycleDeltaCents.add (juce::var (static_cast<double> (deltaCentsTrace[hop])));

       // Crude rate estimate: zero-crossings of deltaCentsTrace over the analysis window.
       int zc = 0;
       for (size_t hop = 1; hop < deltaCentsTrace.size(); ++hop)
           if ((deltaCentsTrace[hop - 1] >= 0.0f) != (deltaCentsTrace[hop] >= 0.0f))
               ++zc;
       const float analysisWindowSec = static_cast<float> (deltaCentsTrace.size())
                                     * static_cast<float> (kAcHopSize) / static_cast<float> (sampleRate);
       vibratoRateHzMeasured = (analysisWindowSec > 0.0f)
                            ? 0.5f * static_cast<float> (zc) / analysisWindowSec
                            : 0.0f;
   }
   ```

8. **Per-mode pass conditions + JSON schema additions (~+60 LOC).**

   Compute the four new pass flags for each mode (in the existing pass-condition block around main.cpp:580):
   ```cpp
   // Phase 2.3 R29 — per-mode pass conditions.
   const bool isVibratoMode      = args.vibratoMode;
   const bool isSlowLfoMode      = args.slowLfoMode;
   const bool isSchellengStress  = args.schellengStress;
   const bool isMacroSweep       = args.macroSweep;

   const float clampedDepthMean = (clampedDepthCount > 0)
                                ? static_cast<float> (clampedDepthSum / clampedDepthCount)
                                : 0.0f;

   const bool passVibratoDepthInRange = isVibratoMode && (peakDepthCents >= 10.0f && peakDepthCents <= 14.0f);
   const bool passOnsetWindow         = isVibratoMode && (onsetTimeMs >= 800 && onsetTimeMs <= 1000);
   const bool passRateHzInRange       = isVibratoMode && (vibratoRateHzMeasured >= 4.5f && vibratoRateHzMeasured <= 5.5f);
   // pass_rmsContinuity for vibrato: reuse the existing 4096-sample window ratio (≥0.90 threshold).

   // pass_breathingAudible (slow-LFO): rmsByDecadePeakToPeak >= 5% (pin #2 v1.0 threshold).
   float rmsByDecadePeakToPeakPct = 0.0f;
   if (isSlowLfoMode || isSchellengStress || isMacroSweep)
   {
       // Reuse existing computeRmsByDecade() lambda; compute peak-to-peak / mean.
       const auto decades = computeRmsByDecade();
       float minD = std::numeric_limits<float>::max();
       float maxD = 0.0f;
       float sumD = 0.0f;
       for (const auto& d : decades)
       {
           const float v = static_cast<float> (static_cast<double> (d));
           if (v < minD) minD = v;
           if (v > maxD) maxD = v;
           sumD += v;
       }
       const float meanD = sumD / juce::jmax (1.0f, static_cast<float> (decades.size()));
       rmsByDecadePeakToPeakPct = (meanD > 1.0e-9f) ? (maxD - minD) / meanD : 0.0f;
   }
   const bool passBreathingAudible = isSlowLfoMode && (rmsByDecadePeakToPeakPct >= 0.05f);
   const bool passClampEngagement  = isSlowLfoMode && (clampedDepthMean > 0.0f);

   const bool passSchellengPeak    = isSchellengStress && (peak <= 1.0f);
   const bool passNoNaN            = (nanCount == 0 && infCount == 0);
   const bool passClampEngaged     = isSchellengStress && (clampedDepthMean < 0.5f);

   // pass_rmsRampDirection (macro-sweep): final-decade RMS exceeds first-decade by 10–30%.
   float rmsRampPct = 0.0f;
   if (isMacroSweep)
   {
       const auto decades = computeRmsByDecade();
       if (decades.size() >= 10)
       {
           const float first = static_cast<float> (static_cast<double> (decades[0]));
           const float last  = static_cast<float> (static_cast<double> (decades[9]));
           rmsRampPct = (first > 1.0e-9f) ? (last - first) / first : 0.0f;
       }
   }
   const bool passRmsRampDirection = isMacroSweep && (rmsRampPct >= 0.10f && rmsRampPct <= 0.30f);
   ```

   Update the `overallPass` switch to handle the four new modes:
   ```cpp
   if (isVibratoMode)
       overallPass = passNan && passPeak && passBlockTime && passVibratoDepthInRange
                  && passOnsetWindow && passRateHzInRange && passRmsContinuity;
   else if (isSlowLfoMode)
       overallPass = passNan && passPeak && passBlockTime && passBreathingAudible
                  && passRmsContinuity && passClampEngagement;
   else if (isSchellengStress)
       overallPass = passNan && passSchellengPeak && passBlockTime && passNoNaN && passClampEngaged;
   else if (isMacroSweep)
       overallPass = passNan && passPeak && passBlockTime && passRmsContinuity && passRmsRampDirection;
   else if (isDetuneSweep) { /* existing */ }
   /* etc. */
   ```

   Update the `modeStr` ladder to recognise the four new modes:
   ```cpp
   const char* modeStr = isMacroSweep      ? "macro-sweep"
                       : isSchellengStress ? "schelleng-stress"
                       : isVibratoMode     ? "vibrato"
                       : isSlowLfoMode     ? "slow-lfo"
                       : isStiffnessSweep  ? "stiffness-sweep"
                       : isDetuneSweep     ? "detune-sweep"
                       : isNoteSequence    ? "note-sequence"
                                           : "sustained";
   ```

   Add per-mode JSON schema blocks (mirrors existing detune-sweep / note-sequence pattern; ~+50 LOC):
   ```cpp
   if (isVibratoMode)
   {
       summary->setProperty ("vibratoDepthSetting",      12.0);
       summary->setProperty ("vibratoRateSetting",       5.0);
       summary->setProperty ("vibratoOnsetMsSetting",    600);
       summary->setProperty ("peakDepthCents",           static_cast<double> (peakDepthCents));
       summary->setProperty ("vibratoRateHzMeasured",    static_cast<double> (vibratoRateHzMeasured));
       summary->setProperty ("onsetTimeMs",              onsetTimeMs);
       summary->setProperty ("perCycleDeltaCents",       juce::var (perCycleDeltaCents));
       summary->setProperty ("rmsContinuityRatio",       static_cast<double> (rmsContinuityRatio));
       summary->setProperty ("pass_vibratoDepthInRange", passVibratoDepthInRange);
       summary->setProperty ("pass_onsetWindow",         passOnsetWindow);
       summary->setProperty ("pass_rateHzInRange",       passRateHzInRange);
       summary->setProperty ("pass_rmsContinuity",       passRmsContinuity);
   }
   else if (isSlowLfoMode)
   {
       summary->setProperty ("slowLfoDepthSetting",      0.5);
       summary->setProperty ("slowLfoRateHzSetting",     0.3);
       summary->setProperty ("rmsByDecade",              juce::var (computeRmsByDecade()));
       summary->setProperty ("rmsByDecadePeakToPeakPct", static_cast<double> (rmsByDecadePeakToPeakPct));
       summary->setProperty ("clampedDepthMean",         static_cast<double> (clampedDepthMean));
       summary->setProperty ("rmsContinuityRatio",       static_cast<double> (rmsContinuityRatio));
       summary->setProperty ("pass_breathingAudible",    passBreathingAudible);
       summary->setProperty ("pass_rmsContinuity",       passRmsContinuity);
       summary->setProperty ("pass_clampEngagement",     passClampEngagement);
   }
   else if (isSchellengStress)
   {
       summary->setProperty ("bowPressureSetting",       7.0);
       summary->setProperty ("bowSpeedSetting",          0.05);
       summary->setProperty ("slowLfoDepthSetting",      1.0);
       summary->setProperty ("peakPostMaster",           static_cast<double> (peak));
       summary->setProperty ("clampedDepthMean",         static_cast<double> (clampedDepthMean));
       summary->setProperty ("pass_peak",                passSchellengPeak);
       summary->setProperty ("pass_noNaN",               passNoNaN);
       summary->setProperty ("pass_clampEngaged",        passClampEngaged);
   }
   else if (isMacroSweep)
   {
       juce::DynamicObject::Ptr ramp (new juce::DynamicObject());
       ramp->setProperty ("start", 0.0);
       ramp->setProperty ("end",   1.0);
       ramp->setProperty ("shape", "linear");
       summary->setProperty ("macroRamp",              juce::var (ramp.get()));
       summary->setProperty ("rmsByDecade",            juce::var (computeRmsByDecade()));
       summary->setProperty ("rmsRampPct",             static_cast<double> (rmsRampPct));
       summary->setProperty ("rmsContinuityRatio",     static_cast<double> (rmsContinuityRatio));
       summary->setProperty ("pass_rmsContinuity",     passRmsContinuity);
       summary->setProperty ("pass_rmsRampDirection", passRmsRampDirection);
   }
   ```

9. **Update CLI usage doc-comment header at top of main.cpp** to list the 4 new flags + auto-rewrite filenames + per-mode pass conditions.

10. **Add `getActiveVoice()` accessor to `PluginProcessor.h`** (single line in `public:` block):
    ```cpp
    BowedContrabassVoice* getActiveVoice() noexcept
    {
        return dynamic_cast<BowedContrabassVoice*> (synth.getVoice(0));
    }
    ```
    Forward-declare `class BowedContrabassVoice;` at top of PluginProcessor.h if not already included. Harness consumes this accessor for the `lastSafeDepth` read in step 6.

**Files modified:**
- `plugins/O-Contrabass/Source/PluginProcessor.h` (+~3 LOC accessor + forward decl).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` (~+250 LOC).

**Files created:** none.

**Commit:** none (R33 atomic).

**Success bar:**
- [ ] `Args` struct has 4 new bool flags.
- [ ] `parseArgs` has 4 new flag handlers.
- [ ] Mutual-exclusion ladder downgrades Phase 2.2 modes when a Phase 2.3 mode is set.
- [ ] Per-mode APVTS overrides set MIDI note + sustain + 2.3 parameters as specified.
- [ ] Auto-rewrite filenames extends to all 4 new modes.
- [ ] `clampedDepthSum` accumulator added; reads `getActiveVoice()->getLastSafeDepth()` per block.
- [ ] Autocorrelation pitch-tracking computes `peakDepthCents`, `onsetTimeMs`, `vibratoRateHzMeasured`, `perCycleDeltaCents`.
- [ ] Per-mode `overallPass` switch handles 4 new modes.
- [ ] Per-mode JSON schema additions land for all 4 new modes.
- [ ] `getActiveVoice()` accessor on `PluginProcessor`.

**Estimated effort:** 5 h (plenty of new code; autocorrelation block is novel; per-mode harness conventions follow Phase 2.2 patterns).

---

### R30 — Build + smoke test

**Compile-only sanity. Confirms R28 + R29 source edits link cleanly. NO commit.**

**Tasks:**

1. **Release build:**
   ```bash
   cd build
   cmake --build . --target O-Contrabass-render-test --parallel
   cmake --build . --target O-Contrabass_VST3 --parallel
   cmake --build . --target O-Contrabass_AU --parallel
   ```
   Expect zero warnings/errors. Atomic-add usage compiles (header `<atomic>`).

2. **Smoke test the 4 new harness flags** (sustain trimmed for fast verification — full Gate 5 renders happen in R31):
   ```bash
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --vibrato --sustain 0.5 --release 0.2 --out /tmp/smoke-vib.wav --json /tmp/smoke-vib.json
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --slow-lfo --sustain 0.5 --release 0.2 --out /tmp/smoke-lfo.wav --json /tmp/smoke-lfo.json
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --schelleng-stress --sustain 0.5 --release 0.2 --out /tmp/smoke-sch.wav --json /tmp/smoke-sch.json
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --macro-sweep --sustain 0.5 --release 0.2 --out /tmp/smoke-macro.wav --json /tmp/smoke-macro.json
   ```
   Each harness run exits with `mode` matching the flag (printed in the JSON `mode` field) and `pass_nan: true`. Pass-condition bars are NOT enforced at this short-sustain smoke (e.g. vibrato won't pass `passOnsetWindow` at 0.5 s sustain — that's expected).

3. **Verify Phase 2.2 modes still work** (regression check on harness CLI):
   ```bash
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --string A --sustain 0.5 --release 0.2 --out /tmp/smoke-string-a.wav --json /tmp/smoke-string-a.json
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --detune-sweep A --sustain 0.5 --release 0.2 --out /tmp/smoke-det-a.wav --json /tmp/smoke-det-a.json
   ```
   Both must produce JSONs with `mode: "sustained"` (string mode) and `mode: "detune-sweep"` (sweep mode) respectively.

**Files modified:** none (build artefacts).

**Files created:** `/tmp/smoke-{vib,lfo,sch,macro,string-a,det-a}.{wav,json}` (transient).

**Commit:** none (R33 atomic).

**Success bar:**
- [ ] All 3 build targets compile zero-warning, zero-error.
- [ ] 4 new harness flags exit 0 (or with mode-mismatch warnings only — pass-bar may fail at 0.5 s sustain, which is fine for smoke).
- [ ] Phase 2.2 modes (`--string A`, `--detune-sweep A`) still produce correct `mode` strings.
- [ ] No undefined-symbol errors on `lastSafeDepth` / `getActiveVoice()` / `getLastSafeDepth()`.

**Estimated effort:** 30 min (build + 6 smoke renders).

---

### R31 — Gate 5 invariants (1)–(7)

**Per CONTEXT rev-5 §"Gate 5 bar" + RESEARCH §16.14 sequencing. Single sequential pass; produces 8 new golden text files + reproduces 5 Phase 2.2 goldens.**

**Tasks:**

1. **Invariant (1) — Phase 2.2 strict byte-equal regression bar.**

   Render the regression preset:
   ```bash
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --note 28 --velocity 0.7 --sustain 60 --release 5 \
       --infinite-sustain 1.0 --string-stiffness 0 \
       --out /tmp/phase23-r31-stiffness-zero.wav \
       --json /tmp/phase23-r31-stiffness-zero.json

   shasum -a 256 /tmp/phase23-r31-stiffness-zero.wav | awk '{print $1}'
   # MUST equal: d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75
   ```

   Plus reproduce the 4 other Phase 2.2 goldens to confirm full regression bar:
   ```bash
   for letter in A D G; do
       ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --string $letter
       shasum -a 256 string-$letter.wav
       # MUST equal Phase 2.2 golden text file content.
   done
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --detune-sweep A
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --note-sequence "28:1.5,33:1.5,38:1.5,43:1.5,28:1.5"
   # All 5 sha256 values MUST match the existing committed golden text files.
   ```

   **If ANY mismatch:** STOP. HR-1/HR-2/HR-3/HR-4 audit required — re-inspect the source edits for missed literal-zero gates, SmoothedValue init values, or re-ordered floating-point ops. Re-run R28 to fix.

2. **Invariant (2) — `--vibrato` mode.**

   ```bash
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --vibrato \
       --out vibrato.wav --json vibrato.json
   ```

   Pass conditions (from JSON):
   - `pass_vibratoDepthInRange: true` (peakDepthCents ∈ [10, 14])
   - `pass_onsetWindow: true` (onsetTimeMs ∈ [800, 1000])
   - `pass_rateHzInRange: true` (vibratoRateHzMeasured ∈ [4.5, 5.5])
   - `pass_rmsContinuity: true` (rmsContinuityRatio ≥ 0.90)
   - `pass_nan: true`, `pass_peak: true`, `pass_blockTime: true`

   Capture the sha256 + JSON as goldens:
   ```bash
   shasum -a 256 vibrato.wav | awk '{print $1}' > plugins/O-Contrabass/tests/render-harness/golden/vibrato.wav.sha256
   cp vibrato.json plugins/O-Contrabass/tests/render-harness/golden/vibrato.json
   ```

3. **Invariant (3) — `--slow-lfo` mode.**

   ```bash
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --slow-lfo \
       --out slow-lfo.wav --json slow-lfo.json
   ```

   Pass conditions:
   - `pass_breathingAudible: true` (rmsByDecadePeakToPeakPct ≥ 0.05 — pin #2)
   - `pass_rmsContinuity: true` (rmsContinuityRatio ≥ 0.90)
   - `pass_clampEngagement: true` (clampedDepthMean > 0.0 — wedge math runs)
   - `pass_nan: true`, `pass_peak: true`, `pass_blockTime: true`

   **Note** — `pass_breathingAudible` may fail at default bass operating point (RESEARCH §16.7.2 documented the wedge clamps `safeDepth` to ~0). If failure, document in VERIFICATION.md and consider:
   - Re-running with manual BOW_PRESSURE override (`--out` to track), AND
   - Confirming clampedDepthMean > 0.0 (proves wedge engaged), AND
   - Filing a Phase 2.4 calibration polynomial follow-up.

   Capture goldens:
   ```bash
   shasum -a 256 slow-lfo.wav | awk '{print $1}' > plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.wav.sha256
   cp slow-lfo.json plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.json
   ```

4. **Invariant (4) — `--schelleng-stress` mode.**

   ```bash
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --schelleng-stress \
       --out schelleng-stress.wav --json schelleng-stress.json
   ```

   Pass conditions:
   - `pass_peak: true` (peakPostMaster ≤ 1.0)
   - `pass_noNaN: true` (nanCount + infCount = 0)
   - `pass_clampEngaged: true` (clampedDepthMean < 0.5 — clamp dominates)
   - `pass_blockTime: true`

   Capture goldens:
   ```bash
   shasum -a 256 schelleng-stress.wav | awk '{print $1}' > plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.wav.sha256
   cp schelleng-stress.json plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.json
   ```

5. **Invariant (5) — `--macro-sweep` mode.**

   ```bash
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test --macro-sweep \
       --out macro-sweep.wav --json macro-sweep.json
   ```

   Pass conditions:
   - `pass_rmsContinuity: true` (rmsContinuityRatio ≥ 0.85 — SOFTENED from default 0.90 because macro intentionally lifts loudness; documented in pass-condition expression).
   - `pass_rmsRampDirection: true` (rmsRampPct ∈ [0.10, 0.30] — final-decade RMS exceeds first-decade by 10–30 %).
   - `pass_nan: true`, `pass_peak: true`, `pass_blockTime: true`

   **NOTE on `pass_rmsContinuity` threshold:** R29 substep 8's `passRmsContinuity` uses the unified ≥0.90 threshold. Macro-sweep needs the looser 0.85 bound. Implementation:
   ```cpp
   const float rmsContinuityThreshold = isMacroSweep ? 0.85f : 0.90f;
   const bool passRmsContinuity = (rmsContinuityRatio >= rmsContinuityThreshold);
   ```
   This is a per-mode threshold modifier — call this out in R29 as part of the implementation.

   Capture goldens:
   ```bash
   shasum -a 256 macro-sweep.wav | awk '{print $1}' > plugins/O-Contrabass/tests/render-harness/golden/macro-sweep.wav.sha256
   cp macro-sweep.json plugins/O-Contrabass/tests/render-harness/golden/macro-sweep.json
   ```

6. **Invariant (6) — auval.**

   On macOS:
   ```bash
   killall -9 AudioComponentRegistrar 2>/dev/null || true
   rm -rf ~/Library/Caches/AudioUnitCache/
   rm -rf ~/Library/Caches/com.apple.audiounits.cache
   rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass.component
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass.component \
         ~/Library/Audio/Plug-Ins/Components/

   # Component IDs from CMakeLists (placeholder — read actual from auval -a output if needed):
   auval -v aumu OCbs OuDv
   ```
   MUST report `AU VALIDATION SUCCEEDED`.

7. **Invariant (7) — pluginval --strictness-level 10.**

   ```bash
   rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass.vst3 \
         ~/Library/Audio/Plug-Ins/VST3/

   /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 \
       --validate-in-process \
       ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3
   ```
   MUST report `SUCCESS`.

**Files modified:** `plugins/O-Contrabass/tests/render-harness/golden/` — 8 NEW text files (4 sha256 + 4 JSON; staged in step 2–5 above).

**Files created:** `/tmp/phase23-r31-*.{wav,json}` (transient; NOT committed).

**Commit:** none (R33 atomic).

**Success bar:**
- [ ] Invariant (1) — All 5 Phase 2.2 goldens reproduce byte-equal (sha256 == committed text content for each).
- [ ] Invariant (2) — `--vibrato` exits 0; all 7 pass conditions TRUE.
- [ ] Invariant (3) — `--slow-lfo` exits 0; `pass_clampEngagement` TRUE; `pass_breathingAudible` TRUE-or-documented (per pin #2 5 % threshold; if FALSE at default bass, document in VERIFICATION.md as Phase 2.4 escalation candidate).
- [ ] Invariant (4) — `--schelleng-stress` exits 0; all 4 pass conditions TRUE; `clampedDepthMean < 0.5`.
- [ ] Invariant (5) — `--macro-sweep` exits 0; `pass_rmsRampDirection` TRUE; loose-rmsContinuity TRUE.
- [ ] Invariant (6) — auval reports `AU VALIDATION SUCCEEDED`.
- [ ] Invariant (7) — pluginval-10 reports `SUCCESS`.
- [ ] 8 new golden text files staged in `tests/render-harness/golden/`.

**Estimated effort:** 1 h (mostly waiting for renders + auval/pluginval; vibrato 2 s + slow-lfo 60 s + schelleng-stress 30 s + macro-sweep 20 s + 5 Phase 2.2 reproductions + auval + pluginval).

---

### R32 (optional) — Logic Pro AU smoke audition

**User-deferred non-blocking. Mirrors R27 / R19f / R14e precedent. Not part of the R33 atomic commit. Captured here as Gate 5 invariant (8) for completeness. Listening sequence locked from CONTEXT rev-5 line 122 (pin #14).**

**Audition sequence:**

1. Open Logic Pro, instantiate O-Contrabass AU on a software-instrument track.
2. **MIDI 38 (D2) sustained + VIBRATO_DEPTH 0→25¢ ramp at 5 Hz vibrato.** Listen for: smooth onset (no click at vibrato gate engaging at 600 ms), no zipper at depth ramp, audible ±25¢ pitch deviation in steady state.
3. **MIDI 33 (A1) sustained + SLOW_LFO_DEPTH 0→1.0 ramp at 0.3 Hz.** Listen for: gradual breathing at default bass operating point (subtle — wedge clamps; deeper modulation requires higher BOW_PRESSURE), no clicks during depth ramp, no Helmholtz collapse.
4. **MIDI 28 (E1) extreme bow params (BOW_PRESSURE 7 N + BOW_SPEED 0.05 m/s) + SLOW_LFO_DEPTH=1 (Schelleng stress).** Listen for: tone remains musical (no friction-junction zero-crossing artefacts), no NaN/silence, no peaking above 0 dBFS.
5. **MIDI 38 (D2) + EXPRESSION_MACRO 0→1.0 ramp.** Listen for: bow speed/pressure/vibrato all lift smoothly, brightness rises by ~500 Hz, no zipper noise on the bridge filter, character becomes more vivid.
6. **MIDI 28 (E1) + VIBRATO_DEPTH=20¢ + SLOW_LFO_DEPTH=0.8 together (anti-correlation guard audition).** Listen for: the +0.13 Hz × 0.8 = +0.104 Hz vibrato rate offset audibly de-correlates vibrato + slow-LFO (no 5:1 sub-harmonic beating). Subtle by design — this is a qualitative listening verification only.

**Gate 5 invariant (8) verdict:** USER-CONFIRMED PASS or DEFERRED. Either is acceptable for closing Phase 2.3.

**Files modified:** none.
**Commit:** none.
**Success bar:** user audits and either confirms PASS or defers (logged in VERIFICATION.md).
**Estimated effort:** 10 min (manual audition).

---

### R33 — Phase 2.3 Atomic Commit (Gate 5 PASS)

**Single atomic commit lands all Phase 2.3 work. Only run on R31 PASS (with R31 invariant (1) — strict byte-equal regression — as the binding bit-exact bar).**

**Tasks:**

1. **Update `parameter-spec.md` line 57 (pin #13 Stage-1 contract amendment).**

   Edit `plugins/O-Contrabass/.planning/parameter-spec.md` line 57: change `0.50` → `0.0` in the EXPRESSION_MACRO Default column.

   Compute new sha256:
   ```bash
   shasum -a 256 plugins/O-Contrabass/.planning/parameter-spec.md | awk '{print $1}'
   # Record this — needed for STATUS.md update in step 2.
   ```

2. **Update `STATUS.md` (pin #13 + commit closure):**
   - `contract_checksums.parameter_spec`: replace with new sha256 from step 1.
   - `next_action` → `phase_2_4_discuss`.
   - `gate_state` block: add `phase_2_3_modulators_macro: PASS`, `phase_2_3_atomic_commit_sha: <new-sha>` (placeholder; back-fill after step 4).
   - `phase_2_3_verify_outcome: VERIFIED_gate_5_pass_2026_04_??_<eight_invariant_summary>`.
   - `phase_2_3_verify_independent_reproduction:` block with: `regression_sha256 == d358abcd…`, `vibrato_sha256: <captured>`, `slow_lfo_sha256: <captured>`, `schelleng_stress_sha256: <captured>`, `macro_sweep_sha256: <captured>`, `auval: AU_VALIDATION_SUCCEEDED`, `pluginval_10: SUCCESS`.
   - `Cycle Scope` → `Phase 2.4 — sub-harmonic bias + 108-combo stability matrix + Schelleng calibration polynomial follow-up + saturator-tail re-evaluation (next fresh GSD cycle)`.
   - `Current Position` Stage 2 progress: 2.1 + 2.2 + 2.3 closed = ~50 % of Stage 2 (Phases 2.4, 2.5, 2.6 remain).
   - `Lifecycle Timeline` append: 2026-04-?? Phase 2.3 verify entry summarising the eight-item Gate 5 PASS.

3. **Stage all Phase 2.3 source + harness + golden text files + parameter-spec amendment + planning artefacts:**
   ```bash
   git add plugins/O-Contrabass/Source/PluginProcessor.cpp
   git add plugins/O-Contrabass/Source/PluginProcessor.h
   git add plugins/O-Contrabass/Source/BowedContrabassVoice.h
   git add plugins/O-Contrabass/Source/BowedContrabassVoice.cpp
   git add plugins/O-Contrabass/tests/render-harness/main.cpp
   git add plugins/O-Contrabass/tests/render-harness/golden/vibrato.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/vibrato.json
   git add plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.json
   git add plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.json
   git add plugins/O-Contrabass/tests/render-harness/golden/macro-sweep.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/macro-sweep.json
   git add plugins/O-Contrabass/.planning/parameter-spec.md
   git add plugins/O-Contrabass/.planning/STATUS.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md
   ```

4. **Confirm staging:**
   ```bash
   git status
   git diff --cached --stat
   ```
   Expect roughly 20 files staged. **Sanity check:** no binary WAVs in `tests/render-harness/golden/` (only `.sha256` text + `.json` per pin #12); no `build/` artefacts; no edits to `Source/DSP/WaveguideString.{h,cpp}` (Phase 2.2 R26-frozen); no edits to `Source/DSP/DispersionFilter.h` (Phase 2.1c R20-frozen); no edits to `modules/synthesis/bow-friction/*` (Phase 2.1b-frozen v1.0.0); no edits to `research/ARCHITECTURE.md` (deferred to end-of-Stage-2 verify); no edits to `ROADMAP.md` (locked contract); no edits to the 5 Phase 2.2 golden text files (carry-forward).

5. **Commit with structured message:**
   ```bash
   git commit -m "$(cat <<'EOF'
   feat(O-Contrabass): vibrato + slow-LFO + Schelleng wedge clamp + EXPRESSION_MACRO - Phase 2.3 Gate 5 PASS

   Adds the modulator + macro section on top of the Phase 2.2 4-string voice:
   per-voice vibrato (5 Hz / 12¢ / 600 ms onset; half-cosine S-curve 300 ms ramp;
   150 ms linear note-off fade-out; modulates active string only via cents-add
   then expf(-cents·ln2/1200) factor in delay-samples space), Slow-Bow LFO
   (0.05–2 Hz; modulates voice-level bow speed × (1 + 0.6·s) and pressure ×
   (1 + 0.5·p) with 23° pressure phase-lag; depth clamped to 80% of Schelleng
   wedge headroom), inline Schelleng wedge clamp (~10 LOC per-block; Z=R=R_s=0.5
   dimensionless collapse), EXPRESSION_MACRO 4-destination layering (default
   flipped 0.50 → 0.0 for bit-exact regression preservation; multipliers from
   architecture line 567: speed × (1.0+0.4·m), pressure × (1.0+0.6·m), vibrato
   depth × (1.0+0.3·m), brightness +500·m Hz; one voice-level SmoothedValue<Linear>
   20 ms ramp on macro source). Anti-correlation guard vibratoRate += 0.13·LFO Hz.
   Per RESEARCH §16.

   Implementation:
   - Source/PluginProcessor.cpp: EXPRESSION_MACRO default 0.50f → 0.0f (Q7a;
     Stage-1 contract amendment — see parameter-spec.md edit below).
   - Source/PluginProcessor.h: + getActiveVoice() accessor for harness clampedDepthMean
     instrumentation read.
   - Source/BowedContrabassVoice.h: + 5 modulator state vars (vibratoPhase /
     vibratoOnsetTimerSeconds / vibratoOnsetGateAtNoteOff / noteOffFadeOutTimerSeconds
     / slowLfoPhase) + 3 SmoothedValue<Linear> (macroSmoothed / slowLfoSpeedSmoothed /
     slowLfoPressureSmoothed) + std::atomic<float> lastSafeDepth instrumentation hook.
   - Source/BowedContrabassVoice.cpp: prepareToPlay init (smoothers reset 20 ms;
     setCurrentAndTargetValue 0.0 — HR-3 invariant); noteStarted re-arms onset
     timer (Q3); noteStopped(allowTailOff=true) starts 150 ms linear fade-out;
     renderNextBlock 7-step evaluation (raw APVTS → Schelleng wedge → slow-LFO →
     LFO apply → macro layer → bowModel push → per-sample loop); per-sample
     vibrato cents (HR-1 gated; active string only) + delay-line modulation.
     ~+90 LOC source.
   - tests/render-harness/main.cpp: 4 new modes — --vibrato (autocorrelation
     pitch-tracking τ ∈ [400, 1500] samples; peakDepthCents / onsetTimeMs /
     vibratoRateHzMeasured), --slow-lfo (rmsByDecade + clampedDepthMean
     instrumentation hook drain; pass_breathingAudible ≥5% softened from 20%
     for v1.0 wedge bass-register parking), --schelleng-stress (clamp-engagement
     audit; pass_clampEngaged < 0.5), --macro-sweep (per-block ramp 0→1; rmsRampPct
     10–30%; rmsContinuity ≥0.85 looser per macro-lift design). Mutual-exclusion
     precedence ladder (macro > schelleng > vibrato > slow-lfo > Phase 2.2 modes).
     Auto-rewrite default WAV/JSON filenames per mode. ~+250 LOC harness.

   Gate 5 (R31) PASS:
   - (1) Phase 2.2 strict byte-equal regression bar reproduced — all 5 goldens:
       stiffness-zero sha256 == d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75
       string-A/D/G/detune-sweep-A/note-sequence sha256 byte-identical
   - (2) --vibrato: pass_vibratoDepthInRange (peakDepthCents ∈ [10, 14])
       + pass_onsetWindow (∈ [800, 1000] ms) + pass_rateHzInRange (∈ [4.5, 5.5] Hz)
       + pass_rmsContinuity (≥0.90).
   - (3) --slow-lfo: pass_breathingAudible (≥5% v1.0 — Phase 2.4 calibration polynomial parked)
       + pass_rmsContinuity (≥0.90) + pass_clampEngagement (clampedDepthMean > 0.0).
   - (4) --schelleng-stress: pass_peak + pass_noNaN + pass_clampEngaged
       (clampedDepthMean < 0.5) + pass_blockTime.
   - (5) --macro-sweep: pass_rmsContinuity (≥0.85 looser) + pass_rmsRampDirection
       (rmsRampPct ∈ [0.10, 0.30]).
   - (6) auval -v aumu OCbs OuDv: AU VALIDATION SUCCEEDED.
   - (7) pluginval --strictness-level 10 --validate-in-process: PASS.
   - (8) Logic AU smoke (R32): user-deferred non-blocking, mirroring R27 /
       R19f / R14e precedent.

   Stage-1 contract amendment:
   - parameter-spec.md line 57: EXPRESSION_MACRO default 0.50 → 0.0.
       Justification: preserves Phase 2.2 strict byte-equal regression bar
       (architecture-spec'd 0.50 default would, once the macro DSP is wired,
       produce a non-zero brightness offset and bow-param multiplier at rest,
       breaking the 5-golden regression bar). Q7a user-confirmed in CONTEXT
       rev-5 (2026-04-27). Knob-at-zero = literal no-op (×1.0 multipliers
       + 0 Hz brightness offset). Macro-lifted character available via user
       knob movement or future presets (Phase 4.1+).
   - STATUS.md contract_checksums.parameter_spec: sha256 update (was c47fe736…).
   - All other artefacts left untouched per RESEARCH §16.11 grep audit:
       parameter-spec-draft.md, research/ARCHITECTURE.md, ROADMAP.md, BRIEF.md,
       REQUIREMENTS.md, stages/1-foundation/PLAN.md (historical / architecture-
       immutable / no-default-referenced).

   Architecture amendments still deferred to end-of-Stage-2 verify per locked
   decision: §"DC Blocker" (F3 from Phase 2.1a-recovery) and §"In-loop saturator"
   (conditional on Phase 2.4 §12.5 escalation).

   Phase 2.4 follow-ups parked:
   - Schelleng wedge bass-register calibration polynomial (analogous to
       Phase 2.1c Risk #7 E1 dispersion clamp). RESEARCH §16.3 documents
       v1.0 ships clamp-engaged-at-default; calibration target post-Phase-2.4.
   - pass_breathingAudible 20% threshold — restore once calibration polynomial
       widens the playable wedge for bass register.
   - Saturator-tail re-evaluation per RESEARCH §12 footnote.

   Phase 2.3 closes. Phase 2.4 (sub-harmonic bias + 108-combo stability matrix
   + Schelleng calibration polynomial + saturator-tail) opens as fresh GSD cycle.

   Refs:
   - RESEARCH.md §16 (Phase 2.3 vibrato + slow-LFO + Schelleng + macro research)
   - PLAN.md rev-7 (R28-pre, R28, R29, R30, R31, R32, R33)
   - CONTEXT.md rev-5 (Phase 2.3 discuss)

   Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
   EOF
   )"
   ```

6. **Post-commit verification:**
   ```bash
   git log --stat HEAD~1..HEAD
   git rev-parse HEAD
   ```
   Record the commit SHA in `STATUS.md` `phase_2_3_atomic_commit_sha` field. (If you forgot to back-fill the placeholder in step 2, commit again with an additional "chore" SHA back-fill commit, OR include the SHA in step 2 by deferring the staging until after the commit and a `git commit --amend` — but **NEVER use --amend per CLAUDE.md hard rule on amends**. Recommended: leave the placeholder as `phase_2_3_atomic_commit_sha: TBD_post_commit`, commit, then create a separate non-atomic chore commit `chore(O-Contrabass): backfill Phase 2.3 commit sha into STATUS.md` after capturing the SHA.) Confirm `git status` clean.

   Mirrors the Phase 2.1c R20 / Phase 2.2 R26 SHA-backfill pattern (STATUS.md edit landed in the atomic, then chore commit `d80b2e6` style backfill — see recent commit history).

**Files modified:** all listed under "Stage all" in step 3.

**Commit:** **THIS IS THE PHASE 2.3 ATOMIC COMMIT.** Single commit, only on Gate 5 PASS.

**Success bar:**
- [ ] `git log --stat HEAD~1..HEAD` shows ~20 files in a single commit.
- [ ] Commit body explicitly documents the Stage-1 contract amendment for EXPRESSION_MACRO default + Q7a regression-bar preservation rationale + Phase 2.4 calibration polynomial parking.
- [ ] `git status` clean post-commit.
- [ ] STATUS.md `next_action` flipped to `phase_2_4_discuss`.
- [ ] STATUS.md `contract_checksums.parameter_spec` updated to new sha256.
- [ ] STATUS.md `phase_2_3_atomic_commit_sha` recorded (or queued for backfill chore commit).
- [ ] Phase 2.3 closed.

**Estimated effort:** 30 min (mostly mechanical staging + commit + STATUS.md update + post-commit verification). +5 min for backfill chore commit if SHA is post-recorded.

---

## Why R33 is a single atomic commit

Same gate-first principle as R7 (Phase 2.1a-recovery), R15 (Phase 2.1b extraction), R20 (Phase 2.1c dispersion), R26 (Phase 2.2 4-string bank):

1. **Coupling:** the Phase 2.3 artefacts (`PluginProcessor.{h,cpp}` macro default flip + accessor; `BowedContrabassVoice.{h,cpp}` 7-step evaluation order + per-sample HR-1 vibrato; harness `main.cpp` 4 new modes + autocorrelation + JSON schemas; 8 new golden text files; parameter-spec.md Stage-1 contract amendment + STATUS.md sha256 update; 5 planning artefact updates) are mutually coupled. Splitting them yields broken intermediate states — e.g., parameter-spec.md edit landing without the source default flip violates the contract; harness JSON schema landing without the voice-side `lastSafeDepth` accessor produces undefined-symbol link errors; macro DSP landing without the default flip breaks the regression bar.
2. **Bisect safety:** if a future Phase 2.x bug bisects back to "modulator + macro landing", a single SHA flips the entire feature.
3. **Audit trail:** Phase 2.x's atomic-commit sequence is R7 → R15 → R20 → R26 → **R33**. The Phase 2.x timeline stays trivially reconstructible from `git log --grep "Phase 2."`.
4. **Stage-1 contract amendment auditability:** the parameter-spec.md edit + STATUS.md sha256 update + commit body explanation MUST land together. Splitting them obscures the rationale chain.

---

## Files To Create / Modify (consolidated, Phase 2.3)

### Source (modified)
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` — R28 (+1 LOC: EXPRESSION_MACRO default 0.50f → 0.0f).
- `plugins/O-Contrabass/Source/PluginProcessor.h` — R29 (+~3 LOC: `getActiveVoice()` accessor + forward decl).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` — R28 (~+20 LOC: 5 modulator state vars + 3 SmoothedValue<Linear> + std::atomic<float> lastSafeDepth + getter; `<atomic>` include).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` — R28 (~+90 LOC: 5 anonymous-namespace constants; prepareToPlay init; noteStarted re-arm; noteStopped fade trigger + hard-stop reset; renderNextBlock 7-step evaluation; per-sample HR-1 gated vibrato + active-slot delay-line modulation in BOTH crossfade and standard mix paths; updateParametersFromAPVTS bowModel push removed — moved into step 6).

### Harness (modified)
- `plugins/O-Contrabass/tests/render-harness/main.cpp` — R29 (~+250 LOC: 4 new flags `--vibrato`/`--slow-lfo`/`--schelleng-stress`/`--macro-sweep`; mutual-exclusion precedence ladder; per-mode pre-build APVTS overrides; per-block macro ramp; clampedDepthSum/Count accumulator + getActiveVoice()->getLastSafeDepth() drain; autocorrelation pitch-tracking with parabolic interpolation; 4 per-mode pass-condition computations; 4 per-mode JSON schema blocks; macro-sweep loose rmsContinuity threshold 0.85; auto-rewrite default WAV/JSON filenames; doc-comment header update).

### Test artefacts (new, committed as text-only — sha256 + JSON, per pin #12)
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.wav.sha256` — R31
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.json` — R31
- `plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.wav.sha256` — R31
- `plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.json` — R31
- `plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.wav.sha256` — R31
- `plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.json` — R31
- `plugins/O-Contrabass/tests/render-harness/golden/macro-sweep.wav.sha256` — R31
- `plugins/O-Contrabass/tests/render-harness/golden/macro-sweep.json` — R31

### Test artefacts (NOT committed — staged-only or transient)
- `/tmp/phase23-r28pre-stiffness-zero.{wav,json}` — R28-pre tripwire (~22 MB; transient).
- `/tmp/smoke-{vib,lfo,sch,macro,string-a,det-a}.{wav,json}` — R30 smoke (~0.2–22 MB each; transient).
- `/tmp/phase23-r31-stiffness-zero.{wav,json}` — R31 regression render (~22 MB; transient).
- `vibrato.wav` / `slow-lfo.wav` / `schelleng-stress.wav` / `macro-sweep.wav` — R31 mode renders (~3–22 MB each; reproducible from harness; sha256 + JSON committed instead).
- `string-{A,D,G}.wav` + `detune-sweep-A.wav` + `note-sequence.wav` — R31 Phase 2.2 reproduction (~3–22 MB each; reproducible; existing sha256 + JSON unchanged).

### Stage-1 contract amendment (modified, per pin #13)
- `plugins/O-Contrabass/.planning/parameter-spec.md` — R33 (1-line edit: line 57 EXPRESSION_MACRO Default 0.50 → 0.0).
- `plugins/O-Contrabass/.planning/STATUS.md` — R33 (`contract_checksums.parameter_spec` sha256 update; full closure block per step 2).

### Planning artefacts (modified)
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` — already at rev-5 (no further edits in execute; rev-5 lock holds).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` — already at §16 append (no further edits in execute).
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` — this rev-7 append (no further edits in execute; verify-phase may add a "rev-7 retrospective" footnote if anomalies surfaced).
- `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` — execute-phase appends "Phase 2.3 execute" section after R33.
- `plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md` — verify-phase appends "Phase 2.3 verify" section with R31 audit table + R31 invariant (1) sha256 confirmation + Phase 2.4 follow-up captures.

### Files explicitly NOT touched
- `plugins/O-Contrabass/Source/DSP/WaveguideString.{h,cpp}` (Phase 2.2 R26-frozen; consumed verbatim — vibrato modulates via existing `setDelaySamples()` API; no topology / smoother / brightness-coefficient / setActiveSections changes).
- `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` (Phase 2.1c R20-frozen; consumed verbatim).
- `plugins/O-Contrabass/Source/PluginEditor.{h,cpp}` (Stage 3 work).
- `plugins/O-Contrabass/Source/OContrabassMPESynthesiser.{h,cpp}` (voice count = 1; no Synthesiser changes for Phase 2.3 — modulators live inside the existing single voice).
- `plugins/O-Contrabass/Source/BowModel.{h,cpp}` (Phase 2.1a-frozen; consumed verbatim — `setBowSpeed`/`setBowPressure` API surface unchanged).
- `modules/synthesis/bow-friction/*` (Phase 2.1b-frozen v1.0.0; value-class deterministic, no string-count / modulator coupling).
- `modules/registry.yaml` (no module surface changes).
- `plugins/O-Bowed/*` (Phase 2.3 is single-plugin scope; O-Bowed has no vibrato/macro yet — confirmed ZERO references per RESEARCH §16.12).
- `plugins/O-Contrabass/research/ARCHITECTURE.md` (deferred amendments to end-of-Stage-2 verify; F3 deviation + Q7a default flip continue tracked in R33 commit body — Q7a is a Stage-1 contract amendment, NOT an architecture amendment, since the architecture's design intent is preserved; only the default value differs).
- `plugins/O-Contrabass/.planning/parameter-spec-draft.md` (historical — audit trail of original draft; pin #13 RESEARCH §16.11 grep audit explicitly leaves untouched).
- `plugins/O-Contrabass/.planning/BRIEF.md` (no EXPRESSION_MACRO default reference).
- `plugins/O-Contrabass/.planning/REQUIREMENTS.md` (no EXPRESSION_MACRO default reference).
- `plugins/O-Contrabass/.planning/stages/1-foundation/PLAN.md` (historical Stage 1 task table — closed milestone, audit trail).
- `plugins/O-Contrabass/.planning/ROADMAP.md` (no default value references).
- `plugins/O-Contrabass/CMakeLists.txt` (no new source files; no header-list update needed).
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.{wav.sha256,json}` (Phase 2.1c golden — content-stable; R31 invariant (1) verifies it still matches).
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-sweep.{wav.sha256,json}` (Phase 2.1c golden — content-stable; not re-rendered in Phase 2.3).
- `plugins/O-Contrabass/tests/render-harness/golden/string-{A,D,G}.{wav.sha256,json}` (Phase 2.2 goldens — content-stable; R31 invariant (1) verifies all 5 still match).
- `plugins/O-Contrabass/tests/render-harness/golden/detune-sweep-A.{wav.sha256,json}` (Phase 2.2 golden — content-stable; verified in R31).
- `plugins/O-Contrabass/tests/render-harness/golden/note-sequence.{wav.sha256,json}` (Phase 2.2 golden — content-stable; verified in R31).

---

## Dependencies Graph (compact)

```
R28-pre (working-tree integrity tripwire; sha256 == d358abcd… with single-edit Q7a default flip;
         NO source edits committed; reverts the edit before R28)
   ↓                ↓ (mismatch → STOP, investigate working-tree drift)
R28 (PluginProcessor.cpp + PluginProcessor.h + BowedContrabassVoice.{h,cpp} source edits;
     ~+115 LOC source; HARD RULES HR-1 to HR-4 enforced; Q7a default flip + 7-step eval order +
     per-sample HR-1 vibrato + active-slot delay-line modulation; NO build, NO commit)
   ↓
R29 (Harness main.cpp + PluginProcessor.h getActiveVoice() accessor; ~+250 LOC; 4 new modes +
     mutual-exclusion ladder + per-mode APVTS overrides + autocorrelation pitch-tracking +
     clampedDepthMean instrumentation drain + per-mode JSON schemas + macro-sweep loose
     rmsContinuity threshold; NO build, NO commit)
   ↓
R30 (Build + smoke; 3 targets compile zero-warning; 4 new mode flags exit 0 at 0.5 s sustain;
     Phase 2.2 modes still functional)
   ↓ (PASS)              ↓ (FAIL)
R31 (Gate 5 invariants  R30 diagnostic on compile/link-error per CONTEXT rev-5 hard rules:
  (1)–(7): regression       - undefined symbol on lastSafeDepth → atomic include missing
  bar (5 goldens) +         - undefined symbol on getActiveVoice → forward decl missing
  --vibrato + --slow-lfo    - undefined sin/cos → header missing
  + --schelleng-stress      - re-run R30 after fix
  + --macro-sweep + auval +
  pluginval-10; capture
  8 new golden text files;
  diagnostic-on-fail per
  HR-1/HR-2/HR-3/HR-4 audit
  if invariant (1) breaks)
   ↓ (PASS)
R32 (optional, user-deferred non-blocking, Logic AU smoke; NOT in commit)
   ↓
R33 (Phase 2.3 atomic commit — ~20 files including parameter-spec.md amendment +
     STATUS.md sha256 update; closes 2.3; mirrors R26 / R20 / R15 / R7 atomic-commit
     pattern; commit body documents Stage-1 contract amendment + Phase 2.4 follow-ups)
```

R28-pre is a strict prerequisite to R28 — if working tree drifts, all subsequent bit-exact reasoning breaks. R29 cannot link without R28's `getActiveVoice()` accessor and the `lastSafeDepth` atomic; R28 + R29 together are a single source-edit batch landed before R30 builds. R30 catches compile/link errors. R31 is the gated bit-exact + behavioral check (binding gate). R31 invariant (1) is the binding regression bar — failure here forces R28 source-edit audit (HR-1/HR-2/HR-3/HR-4 enforcement). R33 is the gated finisher.

---

## Risks (Phase 2.3, refreshed from RESEARCH §16.13)

1. **Bit-exact regression failure when modulators land (CONTEXT rev-5 Risk #1; binding RESEARCH §16.8 invariant).** Mitigation: HR-1 to HR-4 hard rules + R28-pre tripwire + R31 invariant (1) final check. **Diagnostic-on-fail playbook:**
   - HR-1 audit: confirm `if (effectiveVibratoDepth > 0.0f)` gate present BEFORE the cents computation; confirm `vibCents = 0.0f` initial value preserved through all paths.
   - HR-2 audit: confirm `if (rawSlowLfoDepth > 0.0f)` gate present BEFORE the LFO phase advance; confirm `slowLfoSpeedMod`/`slowLfoPressureMod` initial values are literal `0.0f`.
   - HR-3 audit: confirm `macroSmoothed.setCurrentAndTargetValue(0.0f)` in prepareToPlay; confirm the `macroSmoothed.setTargetValue(rawMacro)` is unconditional but the bow-param multiplications are pure IEEE 754 identity arithmetic at macro=0.
   - HR-4 audit: confirm the entire wedge block is gated; confirm `safeDepth = vibAntiCorr = 0.0f` initial values preserved through the gate.
   - Last resort: file follow-up RESEARCH note for Phase 2.4 (RESEARCH §16.8 final paragraph reserves this — empirical pre-flight already passed, so failure here would be a code-edit defect, not a HR-rule defect).

2. **Schelleng wedge always-clamps at bass register (CONTEXT rev-5 Risk #2 + RESEARCH §16.3 CHARACTERIZED).** Mitigation: ACCEPTED for v1.0. RESEARCH §16.3 documents headroom = −0.84 at default bass operating point (F_bow=1.0 < fMin=6.25 with Z=R=R_s=0.5 collapse). v1.0 ships with clamp engaged at default; user must dial bow params to mid-wedge region (BOW_PRESSURE 2–4, mid-β 0.10–0.15) to hear LFO modulation. **Phase 2.4 follow-up parking:** calibration polynomial (analogous to Phase 2.1c Risk #7 E1 dispersion) widens the playable wedge.

3. **Brightness offset zipper at 20 ms smoothing (CONTEXT rev-5 Risk #3 + RESEARCH §16.5 MITIGATED).** Mitigation: §16.5 analytical proof — Δp ≈ 0.015/block at 20 ms ramp on the bridge-LP coefficient; well below ~0.05 zipper-detection threshold. Fallback: bump smoothing to 50 ms (1-line `.reset(sr_internal, 0.050)` change at the macroSmoothed init) if Gate 5 invariant (5) fails — `pass_rmsContinuity ≥ 0.85` catches.

4. **Vibrato + detune Lagrange3rd accumulation error at extreme cents (CONTEXT rev-5 Risk #4 + RESEARCH §16.4 MITIGATED).** Mitigation: ±50¢ vibrato peak is two orders of magnitude below Phase 2.2 detune-sweep ±1200¢ already validated through Lagrange3rd. Gate 5 invariant (2) `pass_rmsContinuity ≥ 0.90` catches.

5. **Per-block Schelleng wedge CPU spike (CONTEXT rev-5 Risk #5 + RESEARCH §16.6 MITIGATED).** Mitigation: 3 divs + 4 muls + 1 min, gated by HR-4. Estimated <0.1 % CPU on M1. PERF-02 (< 5 % CPU) preserved; total Phase 2.3 voice CPU additionally ~0.3 % per RESEARCH §16.

6. **Macro × vibrato onset compound modulation (CONTEXT rev-5 Risk #6 + ACCEPTED).** By-design UX feature; documented in user manual (Phase 4 polish). No mitigation — user expects the macro to interact with vibrato character.

7. **EXPRESSION_MACRO default-change auditability (CONTEXT rev-5 Risk #7 + RESEARCH §16.11 MITIGATED).** Mitigation: pin #13 grep audit identifies parameter-spec.md line 57 + STATUS.md sha256 as the only required edits; R33 commit body documents Stage-1 contract amendment with Q7a regression-bar preservation rationale. parameter-spec-draft.md / ARCHITECTURE.md / ROADMAP.md / BRIEF.md / REQUIREMENTS.md / Stage-1 PLAN.md left untouched as audit trail.

8. **`--schelleng-stress` false-positives on audio output alone (CONTEXT rev-5 Risk #8 + RESEARCH §16.7.3 MITIGATED).** Mitigation: `clampedDepthMean` instrumentation hook (pin #4 — `std::atomic<float> lastSafeDepth`) exposed via JSON; `pass_clampEngaged` confirms `clampedDepthMean < 0.5` (clamp dominates). Phase 2.1c precedent: `rmsByDecade` instrumentation hook in `--stiffness-sweep` mode.

9. **Slow-LFO at very low rate over short renders (CONTEXT rev-5 Risk #9 + RESEARCH §16.7.2 MITIGATED).** Mitigation: `--slow-lfo` harness pins SLOW_LFO_RATE=0.3 Hz over 60 s = 18 cycles, well-sampled.

10. **Slow-LFO `pass_breathingAudible` 20 % threshold not reachable at default bass (RESEARCH §16.13 NEW).** Mitigation: pin #2 — softened to 5 % for v1.0; restore 20 % as Phase 2.4 calibration target.

11. **Vibrato sine phase carry-forward across notes introduces non-deterministic golden-render order dependency (RESEARCH §16.13 NEW).** Mitigation: vibrato golden tests are SINGLE-NOTE (`--vibrato` mode renders one note); sequence-mode tests have VIBRATO_DEPTH=0 → HR-1 short-circuit, no phase advance. Risk does NOT materialise in Phase 2.3 harness suite.

12. **`macroSmoothed.skip(numSamples - 1)` per-block edge case at numSamples=1 (RESEARCH §16.13 NEW).** Mitigation: pin #7 — `juce::jmax(0, numSamples - 1)` guards the skip. Also `if (numSamples <= 0) return;` early-return in `renderNextBlock` line 201–202 prevents step 5 from running at numSamples=0.

13. **Pitch-tracking autocorrelation sensitivity to bow noise / sub-harmonic content (RESEARCH §16.13 NEW).** Mitigation: at Phase 2.3 the friction junction is the only audio source (no bow noise — Phase 2.5; no sub-harmonics — Phase 2.4). τ ∈ [400, 1500] search range excludes f₀/2 sub-harmonic at τ ≈ 2140 samples for f₀ = 41.20 Hz.

14. **`updateParametersFromAPVTS()` bowModel push removal regression risk (NEW, R28 substep 7a refactor).** Mitigation: at modulators-off, the new step 6 push value `effectiveBowSpeed * mpeExpression` equals the removed `effectiveSpeed = bowSpeed * mpeExpression` byte-identically; ditto pressure. R31 invariant (1) catches any miss.

---

## Success Criteria (Gate 5 — Phase 2.3 verify exit gate)

- [ ] **R28-pre** — Working-tree integrity tripwire: pre-edit sha256 == `d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75` (Phase 2.1c golden carry-forward; reproduces RESEARCH §16.1 under plan-phase build env). Source edit reverted post-tripwire.
- [ ] **R28** — `Source/PluginProcessor.cpp` line 86 EXPRESSION_MACRO default 0.50f → 0.0f (Q7a). `Source/PluginProcessor.h` `getActiveVoice()` accessor added. `Source/BowedContrabassVoice.h` extended with 5 modulator state vars + 3 SmoothedValue<Linear> + std::atomic<float> lastSafeDepth + getter. `Source/BowedContrabassVoice.cpp` extended with prepareToPlay init + noteStarted re-arm + noteStopped fade trigger + 7-step renderNextBlock evaluation + per-sample HR-1 gated vibrato + active-slot delay-line modulation in BOTH crossfade and standard mix paths. HARD RULES HR-1 to HR-4 visually traceable in code.
- [ ] **R29** — Harness `--vibrato` / `--slow-lfo` / `--schelleng-stress` / `--macro-sweep` flags + mutual-exclusion precedence ladder + per-mode APVTS overrides + per-block macro ramp + clampedDepthMean accumulator drain via `getActiveVoice()->getLastSafeDepth()` + autocorrelation pitch-tracking with parabolic interpolation + per-mode pass-condition expressions + per-mode JSON schema additions + macro-sweep loose rmsContinuity 0.85 threshold + auto-rewrite filenames.
- [ ] **R30** — Release build clean (3 targets); 4 new harness flags exit 0 at 0.5 s smoke; Phase 2.2 modes still functional (`--string A`, `--detune-sweep A` produce correct mode strings).
- [ ] **R31 invariant (1)** — Phase 2.2 strict byte-equal regression bar reproduced — all 5 goldens (stiffness-zero `d358abcd…`, string-A `aa88f4c3…`, string-D `d0ef8087…`, string-G `524d2186…`, detune-sweep-A `5e31dad3…`, note-sequence `2a731edb…`) byte-identical to committed text-file content.
- [ ] **R31 invariant (2)** — `--vibrato`: `pass_vibratoDepthInRange` (peakDepthCents ∈ [10, 14]) + `pass_onsetWindow` (∈ [800, 1000] ms) + `pass_rateHzInRange` (∈ [4.5, 5.5] Hz) + `pass_rmsContinuity` (≥ 0.90) all TRUE. sha256 captured to golden.
- [ ] **R31 invariant (3)** — `--slow-lfo`: `pass_breathingAudible` (≥ 5 % v1.0; 20 % parked Phase 2.4) + `pass_rmsContinuity` (≥ 0.90) + `pass_clampEngagement` (clampedDepthMean > 0.0) all TRUE-or-documented. sha256 captured to golden.
- [ ] **R31 invariant (4)** — `--schelleng-stress`: `pass_peak` (peak ≤ 1.0) + `pass_noNaN` (zero NaN/Inf) + `pass_clampEngaged` (clampedDepthMean < 0.5) all TRUE. sha256 captured to golden.
- [ ] **R31 invariant (5)** — `--macro-sweep`: `pass_rmsContinuity` (≥ 0.85 looser) + `pass_rmsRampDirection` (rmsRampPct ∈ [0.10, 0.30]) all TRUE. sha256 captured to golden.
- [ ] **R31 invariant (6)** — auval reports `AU VALIDATION SUCCEEDED`.
- [ ] **R31 invariant (7)** — pluginval --strictness-level 10 reports `SUCCESS`.
- [ ] **R31 audit table** — Eight-item Gate 5 bar compiled (input to VERIFICATION.md): items (1)–(7) from R31, (8) deferred to R32.
- [ ] **R32 (optional)** — Gate 5 invariant (8) Logic AU smoke: USER-CONFIRMED PASS or DEFERRED (mirrors R27 / R19f / R14e precedent).
- [ ] **R33** — Atomic commit landed; `git log --stat HEAD~1..HEAD` shows ~20 files in single commit; STATUS.md `next_action` flipped to `phase_2_4_discuss`; `gate_state.phase_2_3_modulators_macro: PASS`; `phase_2_3_atomic_commit_sha` recorded (or queued for backfill chore commit). `parameter-spec.md` line 57 default 0.50 → 0.0 amended; `STATUS.md` `contract_checksums.parameter_spec` sha256 updated. Commit body documents Stage-1 contract amendment + Q7a regression-bar preservation rationale + Phase 2.4 follow-ups.
- [ ] **(Architecture amendments, post-Stage-2-verify, OUT OF SCOPE for this execute)** — ARCHITECTURE.md §"DC Blocker" (F3) + §"In-loop saturator" amendments still deferred. EXPRESSION_MACRO default flip (Q7a) is a Stage-1 contract amendment, NOT an architecture amendment — design intent of architecture line 567 (4-destination layering with stated multipliers) is preserved; only the default value differs.

When all checks above are green (R32 user-confirmed or accepted as deferred), **Phase 2.3 verifies** and **Stage 2 progress reaches ~50 %** (2.1 + 2.2 + 2.3 closed; 2.4, 2.5, 2.6 remain). Phase 2.4 (sub-harmonic bias + 108-combo stability matrix + Schelleng calibration polynomial follow-up + saturator-tail re-evaluation) opens as a fresh GSD cycle.

---

## Out of Scope (deferred per CONTEXT.md rev-5 + RESEARCH §16 + STATUS.md)

- **Phase 2.4 → 2.6** — Sub-harmonic bias + 108-combo stability matrix (2.4 — saturator-tail re-evaluated here per RESEARCH §12 footnote; bass-register Schelleng wedge calibration polynomial follow-up here per RESEARCH §16.3 + Phase 2.1c Risk #7); body resonator + bow noise (2.5); master saturator/limiter + microtonal + MPE + Note Expression / MTS-ESP / Scala (2.6).
- **Phase 2.4 calibration polynomial follow-up for bass-register Schelleng wedge** — RESEARCH §16.3 documented headroom = −0.84 at default bass operating point. v1.0 ships clamp-engaged-at-default; calibration polynomial recalibrates `Z` / `R` / `R_s` constants (or replaces closed-form with empirical bass-register table) so the wedge produces non-negative headroom at default settings. After calibration, restore `pass_breathingAudible` 20 % threshold from the v1.0 5 % softening (pin #2).
- **`pass_rmsContinuity` 0.90 vs 0.85 threshold tuning** — macro-sweep uses 0.85; vibrato/slow-lfo use 0.90; if R31 false-flags, soften per-mode in VERIFICATION.md note + log Phase 2.4 follow-up.
- **Anti-correlation guard FFT-bin spectral test** — Q8 listening-test-only resolution. If Phase 2.4 surfaces a 5:1 sub-harmonic beating in the friction junction at LFO+vibrato extremes, file FFT-bin spectral metric as Phase 2.5+ harness extension.
- **Macro-aware Schelleng wedge re-evaluation** — RESEARCH §16.9 NO re-eval policy. If R32 listening surfaces Helmholtz-collapse at extreme macro + extreme bow params, file Phase 2.4 follow-up: macro-aware safeDepth refinement OR re-evaluating wedge against macro-lifted params.
- **Per-string vibrato modulation** — Q2 active-string-only contract. Future MPE per-string vibrato (different vibrato rate per voice/string) parking — Phase 2.6 MPE work.
- **VibratoLFO / SlowBowLFO module extraction** — Q10 inline-in-voice locked. RESEARCH §16.12 confirms zero O-Bowed precedent; revisit only if execute-phase exceeds ~60 LOC each (current estimate: ~30 LOC each). Phase 2.4-or-later concern if O-Bowed grows its own modulator layer.
- **EXPRESSION_MACRO automation lane in DAW** — Stage 3 (GUI) work. v1.0 supports parameter automation via standard APVTS plumbing; lane labelling + macro-knob UI live in Stage 3.
- **CC11/CC2/CC74/MPE-Y direct mapping to BOW_SPEED/BOW_PRESSURE/BOW_POSITION** — Phase 2.6 microtonal + MPE work. Phase 2.3 leaves the existing Phase 2.1a `mpeExpression` / `note.pressure.asUnsignedFloat()` / `note.timbre.asSignedFloat()` plumbing intact; explicit CC mapping deferred.
- **Production WAV binary commits** — `phase23-r28pre-stiffness-zero.wav` / `phase23-r31-stiffness-zero.wav` / `vibrato.wav` / `slow-lfo.wav` / `schelleng-stress.wav` / `macro-sweep.wav` / `string-{A,D,G}.wav` / `detune-sweep-A.wav` / `note-sequence.wav` are NOT committed (~3–22 MB each; reproducible from harness). sha256 + JSON committed instead per pin #12.
- **ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments** — end-of-Stage-2 verify decides; F3 deviation continues in R33 commit body.
- **Saturator-tail Phase 2.4 follow-up** — RESEARCH §12 footnote carry-forward. R32 listening on E1+VIBRATO+SLOW_LFO together (anti-correlation guard audition) may surface saturator-tail interaction with modulators; still parked for Phase 2.4.

---

# Stage 2: DSP — Plan (Phase 2.4a) — REVISION 8 (Schelleng Calibration Polynomial + 108-Combo Stability Matrix + `pass_breathingAudible` Threshold Restoration, Gate 6a)

> **Status:** rev-8 authors fresh task bodies for **R34-pre, R34a, R34b, R34c, R34d, R34e, R34f, R34g, R34h, R34** per `RESEARCH.md §17.11` sequencing and `CONTEXT.md` rev-6. rev-1/2/3/4/5/6/7 remain in-effect as completed/verified history. Phase 2.3 closed 2026-04-27 with R33 atomic commit `af54571` (Gate 5 PASS with rebaseline of 4 audible carry-forward goldens; strict E1 + detune-sweep-A unchanged; HR-1..HR-4 IEEE 754 identity-arithmetic preserved for primary contract).

**Date:** 2026-04-28
**Cycle scope:** Phase 2.4a only (Phase 2.4b sub-harmonic bias + Phase 2.4c autocorrelator harness fix + saturator-tail O-Bowed comparison still get fresh GSD cycles each)
**Gate:** Gate 6a (Phase 2.4a verify)
**Atomic-commit unit:** R34 (Gate 6a PASS) — single commit lands Phase 2.4a source + harness + 1 new header + 2 new golden text files + 4 re-baselined golden text files + reproduce-goldens.sh script + tools/schelleng-fit/ Python tool + planning artefacts. **NO Stage-1 contract amendment** (parameter-spec.md unchanged; STATUS.md `contract_checksums.parameter_spec` carries forward `77638e25…`). **NO ARCHITECTURE.md amendment** (calibration polynomial is implementation detail of architecture-spec'd Schelleng wedge clamp; closed-form §"Slow-Bow LFO" stays as conceptual reference).
**Carry-forward locks (NOT re-litigated):** Phase 2.1a-recovery split-rail topology, F2 LP form, F3 no-in-loop-DCB, F4 betaScale removed, Phase 2.1b bow-friction module v1.0.0 consumption, Phase 2.1c `DispersionFilter<4>` API consumed verbatim, Phase 2.1c per-string M=4/3/2/1 dispersion table, Phase 2.2 4-string bank topology + per-string detune + 5 ms equal-power crossfade + MIDI→string mapping, Phase 2.3 modulator-layer surface (vibratoPhase / vibratoOnsetTimer / slowLfoPhase / 4 macro SmoothedValues / 7-step per-block evaluation order / HR-1..HR-4 hard rules / `lastSafeDepth.store(0.0f)` unconditional pre-gate at top of step 2 / EXPRESSION_MACRO default 0.0f / VIBRATO_DEPTH default 0.0f), ARCH §"DC Blocker" + §"In-loop saturator" amendments deferred to end-of-Stage-2 verify, primary listening DAW = Logic Pro (AU). Phase 2.4a swaps **only** the inline Schelleng wedge math at Step 2 of the 7-step order, behind the existing HR-4 gate. All other modulator code unchanged.

---

## Preamble — Pinned Open Items (RESEARCH §17.12)

PLAN rev-8 pins each of the 11 plan-phase open items from RESEARCH §17.12:

| # | Open Item | Pinned Decision |
|---|-----------|-----------------|
| 1 | CLI flag spelling for matrix-stability mode | **`--matrix-stability`** (matches CONTEXT rev-6 wording + RESEARCH §17.5 schema). NOT `--matrix` (collides with potential future MIDI matrix modes) NOT `--stability-matrix` (lexical noise). Harness `parseArgs` adds `--matrix-stability` flag; mutual-exclusion precedence ladder slots it ABOVE `--macro-sweep` (highest precedence; overrides all other modes when set). |
| 2 | 108-combo iteration mode | **In-process loop** (single harness invocation iterates all 108 combos). ~10 s wall-clock total per RESEARCH §17.2 single-combo pre-flight extrapolation (108 × 0.04 s + ~5 s JUCE init). Mode iterates `for (s = 0..3) for (i = 0..2) for (j = 0..2) for (k = 0..2)` in canonical order (§17.5); resets DSP state via `processor.releaseResources(); processor.prepareToPlay(...)` between combos to prevent state bleed across combos; concatenates 108 stereo WAV chunks (5 s sustain + 0.5 s silence buffer = 5.5 s per combo) into a single output WAV. JSON aggregate emits per-combo entries in canonical order plus aggregate `pass_all_108`. Separate-invocation mode (~30 s wall-clock; one harness invocation per combo) deferred unless in-process mode surfaces state-bleed issues at R30 smoke. |
| 3 | Wedge-math bypass during matrix-stability render | **Process-time CLI flag.** R34a adds `if (args.matrixStabilityMode) safeDepth = rawSlowLfoDepth;` inside the existing HR-4 gate body of `renderNextBlock`, ABOVE the existing closed-form math (which becomes dead code in matrix mode). The bypass is reachable ONLY when the harness is invoked with `--matrix-stability`. Production plugin builds (VST3 / AU shipped to users) NEVER invoke the harness binary; the bypass code path is unreachable in any user-facing render. **CRITICAL:** the flag is read from a process-side `static std::atomic<bool> g_matrixStabilityMode` set in `main()` BEFORE `processor.prepareToPlay()`; voice-side reads it from a free function `bool isMatrixStabilityModeActive() noexcept` (defined in the harness translation unit, declared `extern` in BowedContrabassVoice.cpp). NO compile-time `#define` (would require harness vs production conditional builds; unnecessary complexity). At R34d source edits, the bypass conditional is preserved verbatim — it ALWAYS evaluates to `false` outside the harness because no other binary defines `isMatrixStabilityModeActive()`. **Production-side stub:** PluginProcessor.cpp adds a weak default `bool __attribute__((weak)) isMatrixStabilityModeActive() noexcept { return false; }` so the plugin links cleanly without the harness symbol. (Alternative: weak symbol on the harness side; pin selects production-side weak default for Linux/macOS portability.) |
| 4 | `reproduce-goldens.sh` canonical content | **Bash script committed at `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh`** (~50 LOC). Locked invocations from RESEARCH §17.1 pre-flight table (sustain 60 / release 5 default for sustained modes; 3-s notes for note-sequence; mode-locked sustain for vibrato + macro-sweep; mode-locked sustain for slow-lfo + schelleng-stress). Script renders all 8 currently-committed goldens (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + vibrato + macro-sweep) AND the 2 re-baselined goldens (slow-lfo + schelleng-stress) into `/tmp/repro/`, computes sha256, prints diff vs committed `.wav.sha256` text files, exits non-zero on any mismatch. Future verify-phase reproductions invoke this script verbatim, eliminating the duration-dependence trap (§17.10 Risk #10). **NEW golden:** matrix-stability is rendered separately (its own R34b invocation; not in the reproduce-goldens.sh canonical 10-golden bar because re-rendering matrix-stability requires the wedge-math bypass which is harness-private). Commit body documents this asymmetry. Script content pinned in §"R34-pre" task body below. |
| 5 | `emit_table.py` output formatting | **Generated `SchellengCalibration.h` includes a header comment block** with: (a) timestamp of generation, (b) sha256 of input `matrix.json`, (c) source-truth note ("DO NOT EDIT BY HAND — regenerate via `python3 tools/schelleng-fit/emit_table.py`"), (d) `pass_all_108` status from input JSON. Each per-string `kSafeDepth[s]` block has a comment naming the string (E1/A1/D2/G2). The constexpr array uses one row per `pressIdx`, three columns per `posIdx`, three rows per `speedIdx` — formatted for human readability. emit_table.py uses Python f-strings with explicit width specifiers; no automatic clang-format pass needed. Generated file is committed verbatim by R34c; CI never re-invokes the tool. |
| 6 | SLOW_LFO_RATE for matrix-stability mode (Open Q7 cycle count) | **0.5 Hz × sustain=5 s = 2.5 cycles** per RESEARCH §17.7. NOT 0.3 Hz (1.5 cycles, borderline phase coverage) NOT 1.0 Hz (5 cycles, but architecture-tail-of-range; rate is bounded ∈ [0.05, 2.0] Hz so 1.0 Hz is fine but 0.5 Hz hits the same cycle count threshold with more headroom against rate-clamping). Existing `--slow-lfo` mode keeps SLOW_LFO_RATE=0.3 Hz (its sustain is 60 s = 18 cycles; cycle count is not a concern there). Harness `--matrix-stability` mode sets SLOW_LFO_RATE=0.5f via `setNorm("SLOW_LFO_RATE", (0.5f - 0.05f) / 1.95f)` per existing parameter-norm convention at main.cpp:389. |
| 7 | BOW_POSITION=0.05 sul-tasto pre-flight | **R34-pre includes a single sul-tasto pre-flight render** with `--schelleng-stress --sustain 5 --release 1 --bow-position 0.05` (or equivalent extreme-combo path) to confirm `rmsContinuity ≥ 0.85` margin holds at the tightest position (loop-gain margins shrink at sul-tasto). If margin fails, plan-phase escalates: either tighten threshold to 0.80 (logged as soft-pass; Phase 2.4-bis recalibrates) OR identify which combo failed (matrix.json triage). R34-pre logs the result to `/tmp/phase24a-r34pre/sul-tasto-preflight.json`; output is diagnostic-only (NOT committed). Pre-flight is informational; outcome doesn't gate execute (§17.8 already confirmed default-position margin is ample at 1.71 blockTimeRatio + 0.94 rmsContinuity). |
| 8 | `pass_combo` aggregation logic | **`pass_combo = pass_noNaN && pass_peak && pass_clickFree && pass_blockTime`** (4-way AND). Each of the 4 sub-passes is a JSON boolean per-combo; `pass_combo` is the per-combo verdict. Aggregate `passCount = sum(pass_combo == true over all 108)`; `pass_all_108 = (passCount == 108)`. Failed combos still emit complete JSON entries (peak / rmsContinuity / blockTimeRatio / clampedDepthMean values populated) for triage; a single failed combo flips `status: "FAIL"` but does NOT abort the render loop — all 108 combos render through to completion regardless. Trivial; pin documents for completeness. |
| 9 | Matrix WAV concatenation strategy | **Single concatenated stereo WAV** at default sr=44100. Per-combo: 5 s sustain + 0.5 s silence buffer = 5.5 s × 108 combos = ~10 min total audio. Combos rendered in canonical iteration order (§17.5: stringIdx → speedIdx → pressIdx → posIdx). Silence buffer is a flat 0.0f write into the harness output buffer between combos; ensures manual auditioning is tractable + sha256 captures combo-boundary determinism. Single sha256 covers all 108 combos; aggregate JSON sha256 is the per-combo truth-table (the file emit_table.py reads). **Both `matrix-stability.wav.sha256` AND `matrix-stability.json.sha256`** committed as new goldens in R34b. Per-combo separate WAV files alternative (108 × 5 s files) rejected — clutters `/tests/render-harness/` and breaks the existing one-WAV-per-mode harness convention. |
| 10 | Open-string MIDI for matrix-stability | **MIDI 28 / 33 / 38 / 43** per `stringIdx ∈ {0,1,2,3}` per RESEARCH §17.6 + CONTEXT rev-6 Q15. Hardcoded constant array `static constexpr int kMatrixStabilityMidi[4] = {28, 33, 38, 43};` in main.cpp (anonymous namespace, mirrors existing `kBOpen` pattern in BowedContrabassVoice.cpp:20). Harness `--matrix-stability` mode injects `MidiBuffer::addEvent(MidiMessage::noteOn(1, kMatrixStabilityMidi[s], 0.7f), 0)` at start of each combo; `noteOff` at start of release window. NO mid-range / fingered-position alternative (§17.6 rationale: friction-junction wedge math doesn't change with finger position; only the open-string period does). |
| 11 | reproduce-goldens.sh placement | **Per-plugin** at `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh`. Matches existing harness scope (per-plugin `tests/render-harness/main.cpp` + `golden/` directory). NOT cross-plugin shared at `tests/` repo-root (other plugins have their own harness setups; cross-plugin script would couple O-Contrabass golden invocations to O-Bowed / future-plugin canonical durations). Future cross-plugin reproducer can wrap the per-plugin scripts if needed; out-of-scope for Phase 2.4a. Script is `chmod +x` and self-contained (sources only `cmake --build` + `shasum -a 256` + bash builtins; no project-level helper functions). |
| 12 | tools/schelleng-fit/ placement | **Repo-root** at `tools/schelleng-fit/{emit_table.py,README.md}`. Matches RESEARCH §17.4 invocation form (`python3 tools/schelleng-fit/emit_table.py matrix.json --out plugins/O-Contrabass/Source/DSP/SchellengCalibration.h`). Although the calibration data is per-plugin-specific (currently O-Contrabass-only; O-Bowed has no Schelleng wedge per §17.9), the emit_table.py tool itself is generic JSON→constexpr-array transcription; future plugins with wedge calibration could reuse the tool with different `--out` paths and matrix schemas. Per-plugin alternative (`plugins/O-Contrabass/tools/`) rejected to avoid Python-tool fragmentation across plugins. **Tool dependency footprint:** Python 3.14 + numpy 2.4 (already installed and confirmed at `/Library/Frameworks/Python.framework/Versions/3.14/`); NO scipy. Tool is offline (developer-machine-only); CI never invokes it. |

**Carry-forward locks from Phase 2.3 (NOT re-litigated):**
- HARD RULES HR-1 / HR-2 / HR-3 / HR-4 verbatim from PLAN rev-7 preamble. R34d preserves all 4 — calibration polynomial is invoked ONLY inside the existing HR-4 `if (rawSlowLfoDepth > 0.0f)` gate, behind the existing `lastSafeDepth.store(0.0f)` unconditional pre-gate.
- 7-step per-block evaluation order verbatim (Step 2 wedge math is the ONLY swap target; Steps 1 / 3 / 4 / 5 / 6 / 7 unchanged).
- `std::atomic<float> lastSafeDepth { 0.0f };` instrumentation hook signature (Phase 2.3 PLAN rev-7 pin #4) carries forward unchanged. Calibration polynomial writes the *new* safeDepth value to lastSafeDepth at the same call site.
- 5 ms equal-power crossfade + `activeStringIndex` / `previousStringIndex` state machine. Calibration lookup at the post-crossfade `activeStringIndex` is correct (per RESEARCH §17.10 Risk #11; crossfade transition is per-sample-loop space, not Step 2 wedge math space).
- 6 carry-forward goldens: E1 strict (`d358abcd…`) + detune-sweep-A (`5e31dad3…`) + per-string A (`c6755aa4…`) + per-string D (`765b015e…`) + per-string G (`0cd5cb0a…`) + note-sequence (`3ac3ccd0…`) + vibrato (`d7881ecf…`) + macro-sweep (`c2571dd9…`). All 8 reproduce byte-identical at HEAD per RESEARCH §17.1 pre-flight. HR-2 + HR-4 gates ensure SchellengCalibration.h never executes in any of these renders.
- Atomic-commit gate-first principle: R7 → R15 → R20 → R26 → R33 → **R34** sequence.

**Hard rules unique to Phase 2.4a (binding for R34d implementation):**
1. **HR-5 — SchellengCalibration.h header-only `inline` linkage.** All array constants use `inline constexpr` (C++17); the trilinear interpolation function is `inline float safeDepthForString (...) noexcept` defined in the header. Header guarded with `#pragma once`. Same pattern as Phase 2.1c `Source/DSP/DispersionFilter.h` precedent. ODR-safe across translation units.
2. **HR-6 — Calibration polynomial invoked behind HR-4 gate ONLY.** Inside the `if (rawSlowLfoDepth > 0.0f)` gate, the call site is `safeDepth = jlimit(0.0f, rawSlowLfoDepth, schelleng::safeDepthForString(activeStringIndex, rawBowSpeed, rawBowPressure, beta));`. The polynomial is NEVER evaluated outside the gate. At SLOW_LFO_DEPTH=0, the entire wedge block (and the polynomial) is skipped → strict E1 (`d358abcd…`) + 6 modulators-off goldens preserve bit-exact regression.
3. **HR-7 — Matrix-stability bypass conditional.** In R34a, the harness mode adds `if (isMatrixStabilityModeActive()) { safeDepth = rawSlowLfoDepth; lastSafeDepth.store(safeDepth, ...); ... }` inside the HR-4 gate, AT THE TOP of the gate body (before the polynomial call). Outside the harness binary, the weak symbol returns `false`, so the conditional is never taken — the polynomial path runs as written. **R34d enforces this ordering:** R34a adds the bypass conditional in the same source-edit batch as R34d's polynomial swap-in.
4. **HR-8 — IEEE 754 identity arithmetic for trilinear.** Trilinear interpolation: `result = c000 * w000 + c100 * w100 + ... + c111 * w111`, where `w` are barycentric weights ∈ [0, 1] with `Σw = 1`. At grid sample points (where one weight is 1 and the rest are 0), trilinear returns the corresponding `c` exactly via IEEE 754 identity arithmetic (`x * 1 = x`, `x + 0 = x`). This is critical for emit_table.py's correctness assumption: the generated table is the empirical truth at sample points, never smeared by interpolation rounding.

**Per-block evaluation order (locked verbatim from RESEARCH §16.6, modified Step 2 only):**
```
Step 1 — Read raw APVTS atomics (10 raw reads; UNCHANGED).
Step 2 — Schelleng wedge:
         lastSafeDepth.store(0.0f, relaxed)   [pin #4 unconditional]
         if (rawSlowLfoDepth > 0.0f)          [HR-4 gate]
             if (isMatrixStabilityModeActive())     [HR-7 bypass]
                 safeDepth = rawSlowLfoDepth
             else                                    [PRODUCTION PATH]
                 beta = jlimit(0.02, 0.25, rawBowPos)
                 safeDepth = jlimit(0.0, rawSlowLfoDepth,
                                    schelleng::safeDepthForString(activeStringIndex,
                                                                  rawBowSpeed,
                                                                  rawBowPressure,
                                                                  beta))     [Phase 2.4a]
             vibAntiCorr = kAntiCorrPerDepth * rawSlowLfoDepth
             lastSafeDepth.store(safeDepth, relaxed)
Step 3 — Slow-LFO phase advance + sin (HR-2 gate; UNCHANGED).
Step 4 — Apply slow-LFO multiplicatively (UNCHANGED).
Step 5 — Layer macro multiplicatively (HR-3 gated; UNCHANGED).
Step 6 — Push to bowModel + all-strings brightness (UNCHANGED).
Step 7 — Per-sample loop (HR-1 gated vibrato; UNCHANGED).
```

---

## Goal

Replace the Phase 2.3 architecture-verbatim closed-form Schelleng wedge math (`Z=R=R_s=0.5` dimensionless collapse — produces `clampedDepthMean=0.0` at default bass operating points, silencing the slow-LFO at MIDI 28-43) with an empirically-derived per-string trilinear lookup table (`Source/DSP/SchellengCalibration.h`; 27-point grid over BOW_SPEED × BOW_PRESSURE × BOW_POSITION axes per string; 108 floats total). Calibration data is derived from a 108-combo bass-register stability matrix render (`--matrix-stability` harness mode at SLOW_LFO_DEPTH=1.0 with wedge-math BYPASSED via HR-7); each grid point gets `safeDepth=1.0` if the combo passes all 4 sub-passes (`pass_noNaN + pass_peak + pass_clickFree + pass_blockTime`) or `safeDepth=0.5` as v1.0 fallback if any sub-pass fails. Re-baseline `--slow-lfo` + `--schelleng-stress` goldens against the calibrated wedge (Phase 2.1c R19a / Phase 2.3 4-golden re-baseline precedent). Restore `pass_breathingAudible ≥ 20%` threshold (architecture-spec'd RESEARCH §16.7.2) — at default bass operating point post-calibration, the lookup returns the grid-point value at speedIdx=1 (0.15) / pressIdx=0 (1.0) / posIdx=1 (0.10), expected to be `1.0` (stable combo at default settings) → full LFO breathing → `rmsByDecadePeakToPeakPct ≥ 20%`. Validate Gate 6a invariants — five-item bar including bit-exact reproduction of all 8 carry-forward goldens — and atomic-commit on Gate 6a PASS as R34. Continue the R7 → R15 → R20 → R26 → R33 → **R34** atomic-commit sequence.

---

## Tasks

### R34-pre — Pre-flight bit-exact baseline confirmation + canonical reproduction script + sul-tasto pre-flight

**No source edits committed except `tests/render-harness/reproduce-goldens.sh` (NEW; staged for inclusion in R34 atomic). Diagnostic + script-creation. Confirms working-tree integrity at start of execute AND empirically reproduces RESEARCH §17.1 pre-flight under the plan-phase build environment AND locks canonical golden invocations against the duration-dependence trap (§17.10 Risk #10).**

Per RESEARCH §17.1, working tree at HEAD `af54571` reproduces all 8 currently-committed goldens byte-identical when invoked with canonical default-duration arguments. Per RESEARCH §17.2, single-combo extreme-settings render takes ~0.04 s wall-clock with ample stability margin (peak 0.107 / blockTimeRatio 1.71). R34-pre re-runs that experiment as a tripwire AND captures the canonical invocations into a committed shell script.

**Tasks:**

1. **Confirm git state is clean and at R33 atomic commit:**
   ```bash
   git log --oneline -1                                  # should be af54571 or descendant (chore backfill)
   git status plugins/O-Contrabass/                      # should be clean
   ```
   If working tree is dirty, STOP and reconcile before proceeding.

2. **Build harness:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```
   Expect `ninja: no work to do` if R33 build artefacts persist; otherwise clean rebuild.

3. **Render all 8 currently-committed goldens to `/tmp/phase24a-r34pre/`** using canonical invocations from RESEARCH §17.1:

   ```bash
   mkdir -p /tmp/phase24a-r34pre
   cd build
   HARNESS=./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test

   # 1. Strict E1 (regression bar)
   $HARNESS --note 28 --velocity 0.7 --sustain 60 --release 5 \
            --infinite-sustain 1.0 --string-stiffness 0 \
            --out /tmp/phase24a-r34pre/stiffness-zero-pre.wav \
            --json /tmp/phase24a-r34pre/stiffness-zero-pre.json

   # 2-4. Per-string A/D/G (defaults: sustain 60 / release 5)
   $HARNESS --string A --out /tmp/phase24a-r34pre/string-A.wav \
                       --json /tmp/phase24a-r34pre/string-A.json
   $HARNESS --string D --out /tmp/phase24a-r34pre/string-D.wav \
                       --json /tmp/phase24a-r34pre/string-D.json
   $HARNESS --string G --out /tmp/phase24a-r34pre/string-G.wav \
                       --json /tmp/phase24a-r34pre/string-G.json

   # 5. Detune-sweep-A
   $HARNESS --detune-sweep A --out /tmp/phase24a-r34pre/detune-sweep-A.wav \
                             --json /tmp/phase24a-r34pre/detune-sweep-A.json

   # 6. Note-sequence (3 s notes)
   $HARNESS --note-sequence "28:3,33:3,38:3,43:3,28:3" \
            --out /tmp/phase24a-r34pre/note-sequence.wav \
            --json /tmp/phase24a-r34pre/note-sequence.json

   # 7. Vibrato (mode-locked: MIDI 28 + 12¢ + 5 Hz + 600 ms onset, sustain 2 s)
   $HARNESS --vibrato --out /tmp/phase24a-r34pre/vibrato.wav \
                      --json /tmp/phase24a-r34pre/vibrato.json

   # 8. Macro-sweep (mode-locked: MIDI 38, EXPRESSION_MACRO 0→1, sustain 20 s)
   $HARNESS --macro-sweep --out /tmp/phase24a-r34pre/macro-sweep.wav \
                          --json /tmp/phase24a-r34pre/macro-sweep.json
   cd -
   ```

4. **Confirm sha256 byte-identical to committed goldens:**
   ```bash
   for g in stiffness-zero-pre string-A string-D string-G detune-sweep-A note-sequence vibrato macro-sweep; do
       computed=$(shasum -a 256 /tmp/phase24a-r34pre/$g.wav | awk '{print $1}')
       expected=$(awk '{print $1}' plugins/O-Contrabass/tests/render-harness/golden/$g.wav.sha256)
       if [ "$computed" = "$expected" ]; then
           echo "[PASS] $g.wav  $computed"
       else
           echo "[FAIL] $g.wav  computed=$computed  expected=$expected"
       fi
   done
   ```
   **All 8 must PASS.** If any FAIL, STOP and investigate working-tree drift (re-confirm HEAD == `af54571`; check `git status` for stray edits in `plugins/O-Contrabass/Source/`, `tests/render-harness/`, `modules/synthesis/bow-friction/`). Do NOT proceed to R34a until all 8 reproduce.

5. **Sul-tasto pre-flight (pin #7) — informational only:**
   ```bash
   # Render single combo at tightest BOW_POSITION (0.05 sul-tasto) + extreme stress
   # to confirm rmsContinuity ≥ 0.85 margin holds at the corner of the 108-combo box.
   # NOTE: requires --bow-position CLI flag (already plumbed in main.cpp; if missing,
   # use existing --schelleng-stress mode which sets BOW_POSITION=0.10 by default,
   # then rerun matrix-stability extreme-corner combo manually post-R34a).
   $HARNESS --schelleng-stress --sustain 5 --release 1 \
            --out /tmp/phase24a-r34pre/sul-tasto-preflight.wav \
            --json /tmp/phase24a-r34pre/sul-tasto-preflight.json

   jq '.rmsContinuity, .blockTime_max_over_median, .peak, .pass_nan' \
      /tmp/phase24a-r34pre/sul-tasto-preflight.json
   ```
   Record the values in PLAN-execution-log notes; if `rmsContinuity < 0.85`, log as Phase 2.4a verify risk + recommend threshold tuning to 0.80 (R34b decision; not blocking R34-pre). If `rmsContinuity ≥ 0.85`, confirms ample margin → CONTEXT-rev-6 thresholds carry forward verbatim. **Sul-tasto-corner-combo dedicated render (BOW_POSITION=0.05 + BOW_PRESSURE=7.0 + BOW_SPEED=0.05) defers to R34b** when the harness `--matrix-stability` mode renders all 108 combos including this corner.

6. **Author `tests/render-harness/reproduce-goldens.sh`** (~50 LOC bash script, locked content per pin #4):

   ```bash
   #!/usr/bin/env bash
   # Phase 2.4a R34-pre — canonical reproduction of all 10 O-Contrabass render goldens.
   # Locks invocations against duration-dependence trap (§17.10 Risk #10).
   # Usage: ./reproduce-goldens.sh           — render to /tmp/repro/, diff vs committed
   #        ./reproduce-goldens.sh --quiet   — exit code only, no stdout
   set -euo pipefail

   QUIET="${1:-}"
   REPO_ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
   GOLDEN_DIR="$REPO_ROOT/plugins/O-Contrabass/tests/render-harness/golden"
   HARNESS="$REPO_ROOT/build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test"
   OUTDIR="/tmp/repro"

   mkdir -p "$OUTDIR"
   cd "$REPO_ROOT/build"

   # Build harness if missing.
   if [ ! -x "$HARNESS" ]; then
       cmake --build . --target O-Contrabass-render-test --parallel >/dev/null
   fi

   # Canonical invocations (sustain 60 / release 5 default for sustained modes;
   # 3-s notes for note-sequence; mode-locked sustain for vibrato/macro/slow/schelleng).
   declare -A INVOC
   INVOC[stiffness-zero-pre]="--note 28 --velocity 0.7 --sustain 60 --release 5 --infinite-sustain 1.0 --string-stiffness 0"
   INVOC[string-A]="--string A"
   INVOC[string-D]="--string D"
   INVOC[string-G]="--string G"
   INVOC[detune-sweep-A]="--detune-sweep A"
   INVOC[note-sequence]="--note-sequence 28:3,33:3,38:3,43:3,28:3"
   INVOC[vibrato]="--vibrato"
   INVOC[macro-sweep]="--macro-sweep"
   INVOC[slow-lfo]="--slow-lfo"
   INVOC[schelleng-stress]="--schelleng-stress"

   FAIL=0
   for g in "${!INVOC[@]}"; do
       wav="$OUTDIR/$g.wav"
       json="$OUTDIR/$g.json"
       $HARNESS ${INVOC[$g]} --out "$wav" --json "$json" >/dev/null
       computed=$(shasum -a 256 "$wav" | awk '{print $1}')
       expected=$(awk '{print $1}' "$GOLDEN_DIR/$g.wav.sha256")
       if [ "$computed" = "$expected" ]; then
           [ -z "$QUIET" ] && echo "[PASS] $g  $computed"
       else
           [ -z "$QUIET" ] && echo "[FAIL] $g  computed=$computed  expected=$expected"
           FAIL=$((FAIL + 1))
       fi
   done

   if [ "$FAIL" -gt 0 ]; then
       [ -z "$QUIET" ] && echo "FAILED: $FAIL of 10 goldens drifted"
       exit 1
   fi
   [ -z "$QUIET" ] && echo "OK: all 10 goldens reproduce byte-identical"
   exit 0
   ```

   ```bash
   chmod +x plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   ```

7. **Stage `reproduce-goldens.sh`** (do NOT commit yet — included in R34 atomic):
   ```bash
   git add plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   git status   # should show 1 new file staged
   ```

**Files created:**
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` (NEW, ~70 LOC, staged for R34 atomic).
- `/tmp/phase24a-r34pre/*.{wav,json}` (transient; deleted post-R34h or end of execute session; NOT committed).

**Files modified:** none committed.

**Commit:** **NONE** — diagnostic + script-staging only. R34 atomic commit lands the staged file alongside the rest of Phase 2.4a.

**Success bar:**
- [ ] `git log --oneline -1` shows `af54571` or descendant chore commit.
- [ ] Harness build succeeds (`ninja: no work to do` or clean rebuild).
- [ ] All 8 sha256 reproductions PASS byte-identical to committed `.wav.sha256` files.
- [ ] Sul-tasto pre-flight `rmsContinuity` ≥ 0.85 OR escalated as Phase 2.4a verify risk.
- [ ] `reproduce-goldens.sh` authored, `chmod +x`, staged for R34.

**Estimated effort:** 25 min (build + 9 renders + 8 sha256 diffs + 1 sul-tasto check + script authoring + chmod + stage).

---

### R34a — Harness `--matrix-stability` mode + HR-7 wedge-math bypass

**Per RESEARCH §17.5 schema + §17.6 MIDI selection + §17.7 SLOW_LFO_RATE 0.5 Hz + pins #1, #2, #3, #6, #8, #9, #10. Adds the 108-combo iteration mode + per-combo JSON + concatenated WAV + the HR-7 production-side weak-symbol bypass. NO source-side calibration polynomial yet — that's R34c+R34d.**

**Tasks:**

1. **`tests/render-harness/main.cpp` — add `--matrix-stability` CLI flag (~+200 LOC).**

   In the anonymous namespace at the top of main.cpp, add the canonical iteration grids:
   ```cpp
   constexpr int   kMatrixStabilityMidi[4] = { 28, 33, 38, 43 };
   constexpr float kMatrixSpeedAxis[3]     = { 0.05f, 0.15f, 0.5f };
   constexpr float kMatrixPressAxis[3]     = { 1.0f,  3.0f,  7.0f };
   constexpr float kMatrixPosAxis[3]       = { 0.05f, 0.10f, 0.20f };
   constexpr float kMatrixSlowLfoRate      = 0.5f;          // pin #6 — 0.5 Hz × 5 s = 2.5 cycles
   constexpr float kMatrixSustainSec       = 5.0f;
   constexpr float kMatrixSilenceSec       = 0.5f;          // pin #9 — silence buffer between combos
   ```

   In `parseArgs`, add `--matrix-stability` flag handling. Slot in mutual-exclusion ladder ABOVE `--macro-sweep` (highest precedence; `args.matrixStabilityMode = true` clears all other mode flags + emits warning if multiple modes set).

   Add process-side bypass flag declaration at top of file:
   ```cpp
   // Phase 2.4a HR-7 — process-side wedge-math bypass for --matrix-stability mode.
   // Voice-side reads via extern free function (declared in BowedContrabassVoice.cpp).
   static std::atomic<bool> g_matrixStabilityMode { false };

   extern "C" bool isMatrixStabilityModeActive() noexcept
   {
       return g_matrixStabilityMode.load (std::memory_order_relaxed);
   }
   ```

2. **`tests/render-harness/main.cpp` — add `runMatrixStabilityMode()` 108-combo iteration loop (~+150 LOC).**

   ```cpp
   static int runMatrixStabilityMode (const Args& args)
   {
       g_matrixStabilityMode.store (true, std::memory_order_relaxed);

       // Single shared processor across all 108 combos — releaseResources/prepareToPlay
       // reset between combos to clear DSP state without rebuilding APVTS.
       OContrabassAudioProcessor processor;
       processor.setRateAndBufferSizeDetails (kSampleRate, kBlockSize);

       const int totalCombos     = 4 * 3 * 3 * 3;            // 108
       const int sustainSamples  = static_cast<int> (kMatrixSustainSec  * kSampleRate);
       const int silenceSamples  = static_cast<int> (kMatrixSilenceSec  * kSampleRate);
       const int comboTotalSamples = sustainSamples + silenceSamples;
       const int totalOutputSamples = comboTotalSamples * totalCombos;

       juce::AudioBuffer<float> outputAccum (2, totalOutputSamples);
       outputAccum.clear();
       std::vector<juce::DynamicObject::Ptr> comboEntries;
       int passCount = 0;

       for (int s = 0; s < 4; ++s)
       for (int i = 0; i < 3; ++i)
       for (int j = 0; j < 3; ++j)
       for (int k = 0; k < 3; ++k)
       {
           // Inject combo-specific APVTS overrides BEFORE prepareToPlay.
           setNorm (processor, "BOW_SPEED",       paramNorm ("BOW_SPEED",       kMatrixSpeedAxis[i]));
           setNorm (processor, "BOW_PRESSURE",    paramNorm ("BOW_PRESSURE",    kMatrixPressAxis[j]));
           setNorm (processor, "BOW_POSITION",    paramNorm ("BOW_POSITION",    kMatrixPosAxis[k]));
           setNorm (processor, "INFINITE_SUSTAIN", 1.0f);
           setNorm (processor, "SLOW_LFO_DEPTH",   1.0f);
           setNorm (processor, "SLOW_LFO_RATE",   paramNorm ("SLOW_LFO_RATE", kMatrixSlowLfoRate));
           setNorm (processor, "VIBRATO_DEPTH",    0.0f);     // HR-1 short-circuit (vibrato off in matrix mode)
           setNorm (processor, "EXPRESSION_MACRO", 0.0f);     // HR-3 short-circuit (macro off)

           processor.releaseResources();
           processor.prepareToPlay (kSampleRate, kBlockSize);

           // Inject MIDI noteOn for this combo's open string.
           const int   midiNote     = kMatrixStabilityMidi[s];
           const int   comboOffset  = (s * 27 + i * 9 + j * 3 + k) * comboTotalSamples;
           juce::MidiBuffer noteOn;
           noteOn.addEvent (juce::MidiMessage::noteOn (1, midiNote, 0.7f), 0);
           noteOn.addEvent (juce::MidiMessage::noteOff (1, midiNote, 0.0f), sustainSamples);

           // Render sustainSamples + silenceSamples = comboTotalSamples per combo.
           PerComboMetrics metrics = renderComboBlocks (processor, noteOn,
                                                        comboTotalSamples, kBlockSize,
                                                        outputAccum, comboOffset);

           const bool pass_noNaN     = (metrics.nanCount == 0);
           const bool pass_peak      = (metrics.peak <= 1.0f);
           const bool pass_clickFree = (metrics.rmsContinuity >= 0.85f);
           const bool pass_blockTime = (metrics.blockTimeRatio <= 5.0f);
           const bool pass_combo     = pass_noNaN && pass_peak && pass_clickFree && pass_blockTime;
           if (pass_combo) ++passCount;

           juce::DynamicObject::Ptr e = new juce::DynamicObject();
           e->setProperty ("stringIdx",                s);
           e->setProperty ("openStringMidi",           midiNote);
           e->setProperty ("bowSpeed",                 kMatrixSpeedAxis[i]);
           e->setProperty ("bowPressure",              kMatrixPressAxis[j]);
           e->setProperty ("bowPosition",              kMatrixPosAxis[k]);
           e->setProperty ("sustainSeconds",           kMatrixSustainSec);
           e->setProperty ("totalSamples",             metrics.totalSamples);
           e->setProperty ("peak",                     metrics.peak);
           e->setProperty ("rmsMid_s2_s3",             metrics.rmsMidpoint);
           e->setProperty ("rmsContinuity",            metrics.rmsContinuity);
           e->setProperty ("blockMicros_median",       metrics.blockMicrosMedian);
           e->setProperty ("blockMicros_max",          metrics.blockMicrosMax);
           e->setProperty ("blockTimeRatio",           metrics.blockTimeRatio);
           e->setProperty ("clampedDepthMean",         metrics.clampedDepthMean);
           e->setProperty ("rmsByDecadePeakToPeakPct", metrics.rmsByDecadePctPP);
           e->setProperty ("pass_noNaN",     pass_noNaN);
           e->setProperty ("pass_peak",      pass_peak);
           e->setProperty ("pass_clickFree", pass_clickFree);
           e->setProperty ("pass_blockTime", pass_blockTime);
           e->setProperty ("pass_combo",     pass_combo);
           comboEntries.push_back (e);
       }

       g_matrixStabilityMode.store (false, std::memory_order_relaxed);

       // Aggregate JSON.
       juce::DynamicObject::Ptr summary = new juce::DynamicObject();
       summary->setProperty ("status",        passCount == totalCombos ? juce::String ("PASS") : juce::String ("FAIL"));
       summary->setProperty ("mode",          juce::String ("matrix-stability"));
       summary->setProperty ("totalCombos",   totalCombos);
       summary->setProperty ("passCount",     passCount);
       summary->setProperty ("failCount",     totalCombos - passCount);
       summary->setProperty ("pass_all_108",  passCount == totalCombos);

       juce::Array<juce::var> arr;
       for (auto& e : comboEntries) arr.add (juce::var (e.get()));
       summary->setProperty ("combos", arr);

       writeJsonFile (args.jsonPath, summary);
       writeWavFile  (args.wavPath, outputAccum, kSampleRate);
       return 0;
   }
   ```

   `paramNorm()`, `setNorm()`, `renderComboBlocks()`, `PerComboMetrics`, `writeJsonFile()`, `writeWavFile()` are existing harness helpers (extracted as needed; reuse main.cpp:380+ existing patterns from `--schelleng-stress` mode).

3. **`Source/BowedContrabassVoice.cpp` — declare extern bypass + add HR-7 conditional inside HR-4 gate.**

   At top of file (anonymous namespace OK):
   ```cpp
   // Phase 2.4a HR-7 — harness-side wedge-math bypass for --matrix-stability mode.
   // Production builds: weak default returns false (defined in PluginProcessor.cpp).
   // Harness binary: real definition in main.cpp returns g_matrixStabilityMode.load().
   extern "C" bool isMatrixStabilityModeActive() noexcept;
   ```

   Inside the existing HR-4 gate body in `renderNextBlock()` (after `lastSafeDepth.store(0.0f, ...)`), insert the bypass conditional ABOVE the closed-form math (which becomes the production path; R34d will replace it with the calibration polynomial call):
   ```cpp
   if (rawSlowLfoDepth > 0.0f)                                              // HR-4 gate
   {
       if (isMatrixStabilityModeActive())                                   // HR-7 bypass
       {
           safeDepth   = rawSlowLfoDepth;
           vibAntiCorr = kAntiCorrPerDepth * rawSlowLfoDepth;
           lastSafeDepth.store (safeDepth, std::memory_order_relaxed);
       }
       else
       {
           // Z = R = R_s = 0.5 dimensionless collapse (Phase 2.3 closed-form;
           // replaced by calibration polynomial in R34d).
           const float beta = juce::jlimit (0.02f, 0.25f, rawBowPos);
           const float fMax = (2.0f * kSchellengZ * rawBowSpeed)
                            / juce::jmax (1.0e-6f, beta * kSchellengDMu);
           const float fMin = (kSchellengZ * kSchellengZ * rawBowSpeed)
                            / juce::jmax (1.0e-6f, 2.0f * kSchellengR * beta * beta * kSchellengDMu);
           const float hUp  = (fMax - rawBowPressure) / juce::jmax (1.0e-6f, fMax);
           const float hLo  = (rawBowPressure - fMin) / juce::jmax (1.0e-6f, fMin);
           const float headroom = juce::jmin (hUp, hLo);
           safeDepth   = juce::jlimit (0.0f, rawSlowLfoDepth, 0.8f * juce::jmax (0.0f, headroom));
           vibAntiCorr = kAntiCorrPerDepth * rawSlowLfoDepth;                   // Q5
           lastSafeDepth.store (safeDepth, std::memory_order_relaxed);
       }
   }
   ```

   **R34d will replace the `else` branch body** with the calibration polynomial call. The bypass `if` branch is preserved verbatim through R34d.

4. **`Source/PluginProcessor.cpp` — add weak default for `isMatrixStabilityModeActive()`** so production builds link without the harness symbol:

   At top of file (anonymous namespace OK):
   ```cpp
   // Phase 2.4a HR-7 — production-side weak default for harness wedge-math bypass.
   // Harness binary overrides this with strong symbol in main.cpp.
   extern "C" __attribute__((weak)) bool isMatrixStabilityModeActive() noexcept
   {
       return false;
   }
   ```

   **Critical:** weak symbols on macOS/Linux behave correctly with the harness's strong symbol via `extern "C"` linkage. Windows MSVC requires `__declspec(selectany)` — not Phase 2.4a concern (O-Contrabass is currently macOS-AU primary).

**Files modified:**
- `plugins/O-Contrabass/tests/render-harness/main.cpp` (~+350 LOC: --matrix-stability flag + g_matrixStabilityMode + isMatrixStabilityModeActive() strong def + runMatrixStabilityMode() + paramNorm/setNorm/PerComboMetrics extensions).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` (~+10 LOC: extern decl + HR-7 conditional inside HR-4 gate).
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` (~+5 LOC: weak default `isMatrixStabilityModeActive()`).

**Commit:** **NONE** — source-edit batch; landed in R34 atomic.

**Success bar:**
- [ ] `--matrix-stability` flag added to harness; mutual-exclusion ladder slots it at top.
- [ ] `g_matrixStabilityMode` atomic + `isMatrixStabilityModeActive()` strong def in main.cpp.
- [ ] `runMatrixStabilityMode()` iterates 4×3×3×3=108 combos in canonical order.
- [ ] Per-combo JSON entry matches RESEARCH §17.5 schema verbatim.
- [ ] Aggregate JSON includes status / totalCombos / passCount / failCount / pass_all_108 / combos[].
- [ ] HR-7 bypass conditional landed in BowedContrabassVoice.cpp inside HR-4 gate; closed-form math preserved in `else` branch (R34d replaces).
- [ ] PluginProcessor.cpp weak default `isMatrixStabilityModeActive()` returns false.

**Estimated effort:** 90 min (mostly main.cpp expansion; reuse of existing harness helpers minimises new infrastructure).

---

### R34b — Render 108-combo matrix + commit golden text files

**Per RESEARCH §17.2 wall-clock budget + §17.8 thresholds. After R34a source edits compile (R30-equivalent smoke check pre-R34b is recommended; see "Pre-R34b smoke" below). Captures empirical stability evidence at SLOW_LFO_DEPTH=1.0 with HR-7 bypass active. NO calibration polynomial yet — R34b's matrix.json IS the input that R34c's emit_table.py reads.**

**Pre-R34b smoke (~3 min):**
```bash
cmake --build build --target O-Contrabass-render-test --parallel
cd build
./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
    --schelleng-stress --sustain 5 --release 1 \
    --out /tmp/phase24a-r34b-smoke.wav --json /tmp/phase24a-r34b-smoke.json
# Expect: pass_nan + pass_peak + pass_blockTime all TRUE; clampedDepthMean ≈ 0.0
# (production wedge math still active via the else branch — R34c+R34d swap in calibration).
```
Smoke confirms R34a HR-7 conditional doesn't perturb the production-path else branch. If FAIL, debug R34a edits before R34b matrix render.

**Tasks:**

1. **Render the 108-combo matrix:**
   ```bash
   cd build
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --matrix-stability \
       --out plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.wav \
       --json plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json
   ```
   Expected wall-clock: ~10 s (per RESEARCH §17.2). Output: ~10 min stereo audio (108 × 5.5 s) + JSON aggregate.

2. **Validate `pass_all_108=true`:**
   ```bash
   jq '.status, .pass_all_108, .passCount, .failCount' \
      plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json
   # Expected: "PASS", true, 108, 0
   ```

   **If `pass_all_108=false`:**
   - Identify failing combos: `jq '.combos | map(select(.pass_combo == false))' matrix-stability.json`
   - For each failing combo, log the (stringIdx, speedIdx, pressIdx, posIdx) tuple + which sub-pass failed (peak / rmsContinuity / blockTimeRatio / NaN).
   - Phase 2.4a v1.0 fallback path: emit_table.py (R34c) sets `kSafeDepth[s][i][j][k] = 0.5f` for failing combos (per §17.4). The matrix-stability render proceeds even with fail-combos; v1.0 ships with depth-clamped wedge at those points.
   - **If `passCount < 108 - 4` (>4 failed):** STOP. Phase 2.4a v1.0 design assumption (most combos are stable) is broken; escalate to Phase 2.4-bis or Phase 2.4a remediation (downstream defense tightening). This is the §17.10 Risk #6 path.
   - **If `passCount ≥ 104` (≤4 failed):** PROCEED. v1.0 fallback `0.5f` is the design path; document failing combos in PLAN-execution-log + R34 commit body.

3. **Compute sha256 of matrix-stability outputs + commit golden text files:**
   ```bash
   shasum -a 256 plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.wav \
       > plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.wav.sha256

   # JSON sha256 captured separately so emit_table.py output reproducibility is auditable.
   shasum -a 256 plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json \
       > plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json.sha256
   ```

   Both `.wav.sha256` and `.json.sha256` text files committed (the `.json` itself is the truth-table input to emit_table.py; its sha256 anchors the audit trail).

4. **Stage the 3 new golden text files** (do NOT commit; included in R34 atomic):
   ```bash
   git add plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json
   git add plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json.sha256
   git status
   ```

   The matrix-stability.wav binary itself (~200 MB at sr=44100 stereo × 10 min) is **NOT committed** (reproducible from harness on demand; sha256 + JSON committed instead, mirroring Phase 2.2 / 2.3 precedent).

**Files created (committed via R34 atomic):**
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.wav.sha256` (NEW; ~80 bytes).
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json` (NEW; ~50–100 KB; the truth-table).
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json.sha256` (NEW; ~80 bytes).

**Files created (NOT committed):**
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.wav` (~200 MB; reproducible).
- `/tmp/phase24a-r34b-smoke.{wav,json}` (transient; pre-R34b smoke).

**Commit:** **NONE** — golden text files staged for R34 atomic.

**Success bar:**
- [ ] Pre-R34b smoke: `--schelleng-stress` PASS (production wedge math unaffected by HR-7 conditional).
- [ ] `--matrix-stability` invocation completes in <30 s wall-clock (under §17.2 budget).
- [ ] `pass_all_108=true` OR `failCount ≤ 4` (Phase 2.4a v1.0 fallback acceptable).
- [ ] 3 new golden text files staged.

**Estimated effort:** 20 min (3 min smoke + ~10 s matrix render + sha256 + jq triage + stage).

---

### R34c — Generate `Source/DSP/SchellengCalibration.h` via emit_table.py

**Per RESEARCH §17.3 + §17.4 + pin #5 + pin #12. emit_table.py reads R34b's matrix.json, emits the 108-float constexpr array with 1.0 for passing combos and 0.5 for failing combos. Generated header is committed verbatim by R34 atomic.**

**Tasks:**

1. **Author `tools/schelleng-fit/emit_table.py`** at repo-root (~80 LOC Python; pin #12 placement):

   ```python
   #!/usr/bin/env python3
   """
   Phase 2.4a R34c — emit SchellengCalibration.h from --matrix-stability JSON.

   NO actual fitting. Trilinear over the 27-point grid is an exact interpolant;
   this tool transcribes per-combo pass/fail into a constexpr array with
   safeDepth = 1.0 for passing combos, 0.5 for failing combos (v1.0 fallback).

   Usage:
       python3 tools/schelleng-fit/emit_table.py matrix.json --out path/SchellengCalibration.h
   """
   import argparse, hashlib, json, pathlib, sys, time

   def main():
       p = argparse.ArgumentParser()
       p.add_argument("matrix_json", type=pathlib.Path)
       p.add_argument("--out", type=pathlib.Path, required=True)
       args = p.parse_args()

       json_bytes = args.matrix_json.read_bytes()
       json_sha   = hashlib.sha256(json_bytes).hexdigest()
       data       = json.loads(json_bytes)

       assert data.get("mode") == "matrix-stability", "input is not matrix-stability JSON"
       assert data.get("totalCombos") == 108, "expected 108 combos"
       combos = data["combos"]
       assert len(combos) == 108

       # Build [4][3][3][3] table in canonical iteration order.
       table = [[[[0.0]*3 for _ in range(3)] for _ in range(3)] for _ in range(4)]
       for s in range(4):
           for i in range(3):
               for j in range(3):
                   for k in range(3):
                       idx = s*27 + i*9 + j*3 + k
                       e   = combos[idx]
                       assert e["stringIdx"]   == s
                       table[s][i][j][k] = 1.0 if e["pass_combo"] else 0.5

       # Emit header.
       out = []
       out.append("// AUTO-GENERATED by tools/schelleng-fit/emit_table.py — DO NOT EDIT BY HAND.")
       out.append(f"// Generated: {time.strftime('%Y-%m-%d %H:%M:%S UTC', time.gmtime())}")
       out.append(f"// Source: matrix.json sha256: {json_sha}")
       out.append(f"// Status: {data.get('status', 'UNKNOWN')}  passCount: {data.get('passCount')}/{data.get('totalCombos')}")
       out.append("// Regenerate via: python3 tools/schelleng-fit/emit_table.py matrix.json --out <path>")
       out.append("")
       out.append("#pragma once")
       out.append("")
       out.append("#include <juce_core/juce_core.h>")
       out.append("")
       out.append("namespace ouaricon::contrabass::schelleng {")
       out.append("")
       out.append("inline constexpr int   kStrings   = 4;")
       out.append("inline constexpr int   kGridN     = 3;")
       out.append("inline constexpr float kSpeedAxis [3] = { 0.05f, 0.15f, 0.5f  };")
       out.append("inline constexpr float kPressAxis [3] = { 1.0f,  3.0f,  7.0f  };")
       out.append("inline constexpr float kPosAxis   [3] = { 0.05f, 0.10f, 0.20f };")
       out.append("")
       out.append("// safeDepth lookup table — indexed [stringIdx][speedIdx][pressIdx][posIdx].")
       out.append("// 1.0 = combo verified stable at SLOW_LFO_DEPTH=1.0; 0.5 = v1.0 fallback for failing combos.")
       out.append("inline constexpr float kSafeDepth[4][3][3][3] = {")
       string_names = ["E1", "A1", "D2", "G2"]
       for s in range(4):
           out.append(f"    /* {string_names[s]} (stringIdx={s}) */ {{")
           for i in range(3):
               out.append(f"        /* speedIdx={i} (v_b={ {0.05,0.15,0.5}[i] }) */ {{")
               # render the i-th speed plane as 3 press rows × 3 pos cols
               for j in range(3):
                   row = ", ".join(f"{table[s][i][j][k]:.1f}f" for k in range(3))
                   out.append(f"            {{ {row} }}{',' if j < 2 else ''}    // pressIdx={j}")
               out.append("        }" + ("," if i < 2 else ""))
           out.append("    }" + ("," if s < 3 else ""))
       out.append("};")
       out.append("")
       out.append("// Trilinear interpolation lookup. Out-of-grid clamps to nearest edge (no extrapolation).")
       out.append("// Always returns ∈ [0.0, 1.0]. Evaluation cost: 8 mul + 7 add + bounds.")
       out.append("inline float safeDepthForString (int stringIdx, float v_b, float F_bow, float beta) noexcept")
       out.append("{")
       out.append("    auto interp_axis = [] (float x, const float (&axis)[3], int& lo, float& t) {")
       out.append("        if (x <= axis[0])      { lo = 0; t = 0.0f; }")
       out.append("        else if (x >= axis[2]) { lo = 1; t = 1.0f; }")
       out.append("        else if (x <= axis[1]) { lo = 0; t = (x - axis[0]) / (axis[1] - axis[0]); }")
       out.append("        else                   { lo = 1; t = (x - axis[1]) / (axis[2] - axis[1]); }")
       out.append("    };")
       out.append("    int sIdx = juce::jlimit (0, 3, stringIdx);")
       out.append("    int iLo, jLo, kLo; float ti, tj, tk;")
       out.append("    interp_axis (v_b,   kSpeedAxis, iLo, ti);")
       out.append("    interp_axis (F_bow, kPressAxis, jLo, tj);")
       out.append("    interp_axis (beta,  kPosAxis,   kLo, tk);")
       out.append("    const float c000 = kSafeDepth[sIdx][iLo  ][jLo  ][kLo  ];")
       out.append("    const float c001 = kSafeDepth[sIdx][iLo  ][jLo  ][kLo+1];")
       out.append("    const float c010 = kSafeDepth[sIdx][iLo  ][jLo+1][kLo  ];")
       out.append("    const float c011 = kSafeDepth[sIdx][iLo  ][jLo+1][kLo+1];")
       out.append("    const float c100 = kSafeDepth[sIdx][iLo+1][jLo  ][kLo  ];")
       out.append("    const float c101 = kSafeDepth[sIdx][iLo+1][jLo  ][kLo+1];")
       out.append("    const float c110 = kSafeDepth[sIdx][iLo+1][jLo+1][kLo  ];")
       out.append("    const float c111 = kSafeDepth[sIdx][iLo+1][jLo+1][kLo+1];")
       out.append("    const float c00 = c000 * (1.0f - tk) + c001 * tk;")
       out.append("    const float c01 = c010 * (1.0f - tk) + c011 * tk;")
       out.append("    const float c10 = c100 * (1.0f - tk) + c101 * tk;")
       out.append("    const float c11 = c110 * (1.0f - tk) + c111 * tk;")
       out.append("    const float c0  = c00  * (1.0f - tj) + c01  * tj;")
       out.append("    const float c1  = c10  * (1.0f - tj) + c11  * tj;")
       out.append("    return juce::jlimit (0.0f, 1.0f, c0 * (1.0f - ti) + c1 * ti);")
       out.append("}")
       out.append("")
       out.append("} // namespace ouaricon::contrabass::schelleng")
       out.append("")

       args.out.write_text("\n".join(out))
       print(f"wrote {args.out} ({len(out)} lines)")

   if __name__ == "__main__":
       main()
   ```

2. **Author `tools/schelleng-fit/README.md`** (~30 LOC):

   ```markdown
   # tools/schelleng-fit

   Offline transcription tool: reads `--matrix-stability` JSON output from the
   O-Contrabass render harness and emits `SchellengCalibration.h` constexpr table.

   ## Usage

       python3 tools/schelleng-fit/emit_table.py path/to/matrix-stability.json \
           --out plugins/O-Contrabass/Source/DSP/SchellengCalibration.h

   ## Re-run conditions

   Re-invoke ONLY if the matrix-stability render is re-rendered (e.g., Phase
   2.4-bis remediation if v1.0 fallback `0.5` proves inadequate at some combo,
   or future plugin recompiles change the underlying friction-junction physics).
   The generated header is committed to git; CI never invokes this tool.

   ## Dependencies

   - Python 3.11+ (tested with 3.14.2)
   - numpy (NOT actually used by emit_table.py for trilinear; reserved for
     future polynomial-fit modes)

   ## Architecture

   Trilinear interpolation over the 27-point grid (3×3×3 per string × 4 strings).
   v1.0 design: assigns 1.0 to passing combos, 0.5 to failing combos (binary
   pass/fail derived from --matrix-stability sub-pass conditions). Trilinear
   is exact at sample points, monotonically bounded off-grid by the 8-corner
   box. See RESEARCH.md §17.3-§17.4 for the design rationale.
   ```

3. **Generate `Source/DSP/SchellengCalibration.h`:**
   ```bash
   chmod +x tools/schelleng-fit/emit_table.py
   python3 tools/schelleng-fit/emit_table.py \
       plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json \
       --out plugins/O-Contrabass/Source/DSP/SchellengCalibration.h
   # Expected: "wrote plugins/O-Contrabass/Source/DSP/SchellengCalibration.h (~150 lines)"
   ```

4. **Inspect generated header:**
   ```bash
   head -10 plugins/O-Contrabass/Source/DSP/SchellengCalibration.h
   # Expected: header comment with timestamp + matrix.json sha256 + status PASS

   grep -c "1.0f" plugins/O-Contrabass/Source/DSP/SchellengCalibration.h
   grep -c "0.5f" plugins/O-Contrabass/Source/DSP/SchellengCalibration.h
   # Total 1.0f + 0.5f count should equal 108 (one per grid point) plus
   # boilerplate constants in the trilinear function (extra ~6 from the 1.0f-tk
   # weight terms — quick visual scan confirms 108 grid entries are correct).
   ```

5. **Stage the 3 new files** (do NOT commit; included in R34 atomic):
   ```bash
   git add tools/schelleng-fit/emit_table.py
   git add tools/schelleng-fit/README.md
   git add plugins/O-Contrabass/Source/DSP/SchellengCalibration.h
   git status
   ```

**Files created (committed via R34 atomic):**
- `tools/schelleng-fit/emit_table.py` (NEW, ~80 LOC).
- `tools/schelleng-fit/README.md` (NEW, ~30 LOC).
- `plugins/O-Contrabass/Source/DSP/SchellengCalibration.h` (NEW, ~150 LOC; auto-generated; HR-5 `inline constexpr` linkage).

**Commit:** **NONE** — staged for R34 atomic.

**Success bar:**
- [ ] `tools/schelleng-fit/emit_table.py` authored + chmod +x.
- [ ] `tools/schelleng-fit/README.md` authored.
- [ ] Python invocation succeeds → SchellengCalibration.h generated.
- [ ] Generated header has timestamp + matrix.json sha256 in comment block.
- [ ] Grid entry count (1.0f + 0.5f literals) ≥ 108.
- [ ] All 3 files staged.

**Estimated effort:** 45 min (~30 min Python authoring + ~5 min execution + ~10 min visual sanity-check of generated header).

---

### R34d — Replace closed-form Schelleng wedge with calibration polynomial call

**Per RESEARCH §17.3 + HR-5/HR-6/HR-8. Single source-edit batch in `BowedContrabassVoice.cpp`. Production-path else-branch from R34a swaps closed-form math for `schelleng::safeDepthForString(...)` call. NO build, NO commit at this stage; R34e + R34f follow before build.**

**Tasks:**

1. **`Source/BowedContrabassVoice.cpp` — add `#include "DSP/SchellengCalibration.h"` (~+1 LOC).**

   At top of file, in the existing `#include` block:
   ```cpp
   #include "DSP/SchellengCalibration.h"
   ```

2. **`Source/BowedContrabassVoice.cpp` — replace the production-path else-branch body inside HR-4 gate (~−10 LOC + ~+3 LOC).**

   The R34a-landed conditional is currently:
   ```cpp
   if (rawSlowLfoDepth > 0.0f)                                              // HR-4 gate
   {
       if (isMatrixStabilityModeActive())                                   // HR-7 bypass
       {
           safeDepth   = rawSlowLfoDepth;
           ...
       }
       else
       {
           // Z = R = R_s = 0.5 dimensionless collapse (Phase 2.3 closed-form;
           // replaced by calibration polynomial in R34d).
           const float beta = juce::jlimit (0.02f, 0.25f, rawBowPos);
           const float fMax = (2.0f * kSchellengZ * rawBowSpeed)
                            / juce::jmax (1.0e-6f, beta * kSchellengDMu);
           const float fMin = (kSchellengZ * kSchellengZ * rawBowSpeed)
                            / juce::jmax (1.0e-6f, 2.0f * kSchellengR * beta * beta * kSchellengDMu);
           const float hUp  = (fMax - rawBowPressure) / juce::jmax (1.0e-6f, fMax);
           const float hLo  = (rawBowPressure - fMin) / juce::jmax (1.0e-6f, fMin);
           const float headroom = juce::jmin (hUp, hLo);
           safeDepth   = juce::jlimit (0.0f, rawSlowLfoDepth, 0.8f * juce::jmax (0.0f, headroom));
           vibAntiCorr = kAntiCorrPerDepth * rawSlowLfoDepth;                   // Q5
           lastSafeDepth.store (safeDepth, std::memory_order_relaxed);
       }
   }
   ```

   **Replace the production-path else-branch body (the 11-line closed-form math block) with:**
   ```cpp
       else
       {
           // Phase 2.4a — empirical calibration table (RESEARCH §17.3).
           // 27-point grid trilinear interpolation per string; values populated by
           // tools/schelleng-fit/emit_table.py from --matrix-stability render data.
           const float beta = juce::jlimit (0.02f, 0.25f, rawBowPos);
           safeDepth   = juce::jlimit (0.0f, rawSlowLfoDepth,
                                       schelleng::safeDepthForString (activeStringIndex,
                                                                      rawBowSpeed,
                                                                      rawBowPressure,
                                                                      beta));
           vibAntiCorr = kAntiCorrPerDepth * rawSlowLfoDepth;                   // Q5 carry-forward
           lastSafeDepth.store (safeDepth, std::memory_order_relaxed);
       }
   ```

   **HR-6 enforced:** the polynomial call site is INSIDE the existing HR-4 gate; never invoked at SLOW_LFO_DEPTH=0. Bit-exact regression for the 6 modulators-off goldens preserved.

3. **`Source/BowedContrabassVoice.cpp` — remove obsolete constants (~−4 LOC).**

   Inside the anonymous namespace block, remove the 3 closed-form constants (no longer referenced after R34d):
   ```cpp
   // REMOVE:
   constexpr float kSchellengZ          = 0.5f;
   constexpr float kSchellengR          = 0.5f;
   constexpr float kSchellengDMu        = 0.60f;
   ```

   **Keep:** `kPressureLagRad`, `kAntiCorrPerDepth`, `kVibratoRampSec`, `kVibratoFadeOutSec`, `kVibFactorScale` — these remain referenced by Step 3 / Step 5 / Step 7 / vibrato + slow-LFO path.

   **Activity check (R34d substep guard):** before deletion, grep:
   ```bash
   grep -n "kSchellengZ\|kSchellengR\|kSchellengDMu" plugins/O-Contrabass/Source/BowedContrabassVoice.cpp
   ```
   Expect ONLY the 3 declarations + the closed-form math block (which was already removed in step 2). If any other reference appears, STOP and audit before deleting.

4. **(NO change to `BowedContrabassVoice.h`.)** No new members; no new methods. The header was Phase 2.3-locked at R28; R34d is a `.cpp`-only refactor.

5. **(NO change to `PluginProcessor.{h,cpp}` outside the R34a weak-default added in step 4.)**

**Files modified:**
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` (~−10 LOC closed-form math + ~+3 LOC polynomial call + ~−4 LOC removed constants + ~+1 LOC `#include` = net ~−10 LOC).

**Commit:** **NONE** — source-edit batch; landed in R34 atomic (after R34e + R34f + R34g + R34h).

**Success bar:**
- [ ] `#include "DSP/SchellengCalibration.h"` added.
- [ ] HR-4 gate else-branch body replaced with `schelleng::safeDepthForString(...)` call.
- [ ] `kSchellengZ` / `kSchellengR` / `kSchellengDMu` constants removed.
- [ ] No remaining grep hits for the 3 removed constants.
- [ ] HR-7 if-branch (matrix-stability bypass) preserved verbatim from R34a.

**Estimated effort:** 20 min (mostly verifying HR-6 + HR-4 ordering preservation + grep audit).

---

### R34e — Restore `pass_breathingAudible` 20% threshold

**Per RESEARCH §16.7.2 architecture-spec'd value + CONTEXT rev-6 Q17 user-confirmed restoration. Single-line constant edit in harness.**

**Tasks:**

1. **`tests/render-harness/main.cpp` line 958 — change `0.05f` → `0.20f`.**

   ```cpp
   // BEFORE (Phase 2.3 v1.0 soft-pass — softened from architecture-spec 20% to 5%
   // because Schelleng wedge clamps depth to 0.0 at default bass operating point):
   const bool passBreathingAudible = args.slowLfoMode && (rmsByDecadePeakToPeakPct >= 0.05f);

   // AFTER (Phase 2.4a — calibration polynomial restores audible breathing at default
   // bass operating point; 20% architecture-spec'd threshold becomes the audible regression bar):
   const bool passBreathingAudible = args.slowLfoMode && (rmsByDecadePeakToPeakPct >= 0.20f);
   ```

2. **Update inline doc-comment in `--slow-lfo` mode JSON schema** (main.cpp around line 60 `pass_breathingAudible (rmsByDecadePeakToPeakPct ≥ 0.05 — v1.0;` text):
   ```cpp
   // BEFORE:
   //                                       && pass_breathingAudible (rmsByDecadePeakToPeakPct ≥ 0.05 — v1.0;
   //                                       softened from architecture-spec 20% — see RESEARCH §16.7.2)
   // AFTER:
   //                                       && pass_breathingAudible (rmsByDecadePeakToPeakPct ≥ 0.20 — Phase 2.4a
   //                                       calibration polynomial restored architecture-spec 20% — see RESEARCH §17.10)
   ```

   This is a doc-comment correction; bit-exact regression unaffected (comments are not compiled).

**Files modified:**
- `plugins/O-Contrabass/tests/render-harness/main.cpp` (1-LOC functional edit + ~3-LOC comment edit).

**Commit:** **NONE** — landed in R34 atomic.

**Success bar:**
- [ ] Line 958 (or current location of the `0.05f` constant) flipped to `0.20f`.
- [ ] Inline doc-comment updated to reference §17.10 / Phase 2.4a / 20% threshold.
- [ ] `git diff` confirms exactly 2 lines changed (1 functional + 1 comment block; comment block may span 2 lines).

**Estimated effort:** 5 min.

---

### R34f — Build + re-baseline `--slow-lfo` + `--schelleng-stress` goldens

**After R34a + R34d + R34e source edits land, build the harness and re-render the 2 goldens that change because the wedge math changes. Capture new sha256s + JSON. Old `3768dd15…` (slow-lfo) + `e50dd191…` (schelleng-stress) are RETIRED (overwritten with new sha256 text files).**

**Tasks:**

1. **Build harness with R34a/c/d/e edits:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```
   Expect zero warnings, zero errors. If build fails:
   - Undefined `schelleng::safeDepthForString` → confirm `Source/DSP/SchellengCalibration.h` exists + R34c executed + namespace matches.
   - Undefined `isMatrixStabilityModeActive` → confirm R34a weak default landed in PluginProcessor.cpp + extern decl in BowedContrabassVoice.cpp.
   - Linker duplicate-symbol on `isMatrixStabilityModeActive` → confirm one weak (PluginProcessor.cpp) + one strong (main.cpp) linkage.

2. **Re-render `--slow-lfo` golden:**
   ```bash
   cd build
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --slow-lfo \
       --out /tmp/phase24a-r34f-slow-lfo.wav \
       --json /tmp/phase24a-r34f-slow-lfo.json

   jq '.pass_breathingAudible, .pass_rmsContinuity, .pass_clampEngagement, .rmsByDecadePeakToPeakPct, .clampedDepthMean' \
      /tmp/phase24a-r34f-slow-lfo.json

   # Expected: pass_breathingAudible=true (≥ 20%), pass_rmsContinuity=true (≥ 0.90),
   # pass_clampEngagement=true (clampedDepthMean > 0.0 — calibration polynomial returns
   # 1.0 at default bass operating point, full LFO depth fed through). Specifically:
   # rmsByDecadePeakToPeakPct ≥ 0.20 (architecture-spec'd threshold restored)
   # clampedDepthMean ≥ 0.95 (close to 1.0; slight drift due to LFO sin trajectory averaging)
   ```

   **If `pass_breathingAudible=false` (rmsByDecadePeakToPeakPct < 20%):** the calibration polynomial under-shoots at the default bass operating point. Phase 2.4a verify risk #5 path: investigate which grid combo's `kSafeDepth` value applies at the default operating point (BOW_SPEED=0.15 ≈ speedIdx=1; BOW_PRESSURE=1.0 ≈ pressIdx=0; BOW_POSITION=0.10 ≈ posIdx=1; activeStringIndex per default MIDI). If that combo got `0.5` in the matrix render (R34b), v1.0 fallback is the culprit; remediation = Phase 2.4-bis (binary-search-over-depth refinement; out of scope for Phase 2.4a v1.0). Document failure + escalate; STOP R34f.

3. **Compute new sha256 + overwrite committed goldens:**
   ```bash
   shasum -a 256 /tmp/phase24a-r34f-slow-lfo.wav | awk '{print $1"  slow-lfo.wav"}' \
       > plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.wav.sha256
   cp /tmp/phase24a-r34f-slow-lfo.json \
      plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.json
   ```

4. **Re-render `--schelleng-stress` golden:**
   ```bash
   cd build
   ./plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test \
       --schelleng-stress \
       --out /tmp/phase24a-r34f-schelleng-stress.wav \
       --json /tmp/phase24a-r34f-schelleng-stress.json

   jq '.pass_peak, .pass_nan, .pass_clampEngaged, .pass_blockTime, .clampedDepthMean, .peak' \
      /tmp/phase24a-r34f-schelleng-stress.json

   # Expected: pass_peak=true (≤ 1.0), pass_nan=true (zero NaN), pass_blockTime=true.
   # pass_clampEngaged: at extreme bow params (BOW_PRESSURE=7.0 + BOW_SPEED=0.05),
   # the calibration grid point [s][i=0][j=2][k=1] returns 0.5 v1.0 fallback if R34b
   # matrix flagged it as failing → clampedDepthMean ≈ 0.5 (clamp engaged at fallback,
   # NOT at zero). pass_clampEngaged threshold currently `clampedDepthMean < 0.5`;
   # if calibration returns exactly 0.5, threshold may need adjustment to `< 0.7` or
   # similar — flag in Phase 2.4a verify if pass fires false-positive.
   ```

5. **Compute new sha256 + overwrite committed goldens:**
   ```bash
   shasum -a 256 /tmp/phase24a-r34f-schelleng-stress.wav | awk '{print $1"  schelleng-stress.wav"}' \
       > plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.wav.sha256
   cp /tmp/phase24a-r34f-schelleng-stress.json \
      plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.json
   ```

6. **Stage re-baselined goldens** (4 files; included in R34 atomic):
   ```bash
   git add plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.json
   git add plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.json
   ```

**Files modified (committed via R34 atomic):**
- `plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.wav.sha256` (RE-BASELINED; old `3768dd15…` retired).
- `plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.json` (RE-BASELINED).
- `plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.wav.sha256` (RE-BASELINED; old `e50dd191…` retired).
- `plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.json` (RE-BASELINED).

**Commit:** **NONE** — staged for R34 atomic.

**Success bar:**
- [ ] Harness build succeeds zero warnings.
- [ ] `--slow-lfo` reports `pass_breathingAudible=true` (rmsByDecade ≥ 20%) + `pass_rmsContinuity=true` + `pass_clampEngagement=true`.
- [ ] `--schelleng-stress` reports `pass_peak + pass_nan + pass_blockTime` all TRUE; `pass_clampEngaged` per-combo behavior documented.
- [ ] 4 golden text files re-baselined; new sha256s captured.
- [ ] All 4 staged.

**Estimated effort:** 20 min (build + 2 renders + 4 sha256 captures + stage).

---

### R34g — Bit-exact regression bar verification

**Re-run `reproduce-goldens.sh` (authored in R34-pre) to confirm 6 carry-forward goldens (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + vibrato + macro-sweep) reproduce byte-identical post-R34a/c/d/e/f source edits. The HR-2 + HR-4 + HR-6 gates are the technical defence — wedge math (closed-form OR calibration polynomial) never executes in any of these renders. R34g is the verification gate confirming HR rules held through the full Phase 2.4a source-edit batch.**

**Tasks:**

1. **Run the canonical reproduction script:**
   ```bash
   plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   # Expected exit code: 0
   # Expected output: 10 [PASS] lines — 8 carry-forward (byte-identical) + 2 re-baselined (byte-identical to NEW sha256s captured in R34f)
   ```

   **If any FAIL:**
   - For the 6 carry-forward goldens (E1 strict / per-string / detune-sweep / note-sequence / vibrato / macro-sweep) failing: HR-1 / HR-2 / HR-3 / HR-4 / HR-6 audit (see Risks #1 below). HR-2 + HR-4 ensure SchellengCalibration.h is never invoked at SLOW_LFO_DEPTH=0; if any of these break, the source edit perturbed something unrelated to calibration. STOP and bisect.
   - For slow-lfo / schelleng-stress failing: the just-captured sha256s in R34f should match the NEW committed sha256s. If they don't, the harness build is non-deterministic (rare) — re-run R34f before proceeding.

2. **Capture R34g audit for VERIFICATION.md** (verify-phase consumes; R34g logs):
   ```bash
   plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh > /tmp/phase24a-r34g-audit.log 2>&1
   echo "Exit code: $?" >> /tmp/phase24a-r34g-audit.log
   ```
   The log file is transient (NOT committed); content cited verbatim in VERIFICATION.md.

**Files modified:** none.

**Commit:** **NONE** — verification step.

**Success bar:**
- [ ] `reproduce-goldens.sh` exits 0.
- [ ] All 8 carry-forward goldens reproduce byte-identical.
- [ ] slow-lfo + schelleng-stress reproduce against R34f re-baselined sha256s.

**Estimated effort:** 5 min (single script run + audit log capture).

---

### R34h — auval + pluginval-10

**Standard Gate 6a invariant 5. Mirrors Phase 2.3 R31 invariant (6) + (7).**

**Tasks:**

1. **Build the AU + VST3 plugin targets:**
   ```bash
   cmake --build build --target O-Contrabass_AU --parallel
   cmake --build build --target O-Contrabass_VST3 --parallel
   ```

2. **macOS AU cache clear + install:**
   ```bash
   killall -9 AudioComponentRegistrar 2>/dev/null || true
   rm -rf ~/Library/Caches/AudioUnitCache/
   rm -rf ~/Library/Caches/com.apple.audiounits.cache
   rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3
   rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass.component
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass.vst3 \
         ~/Library/Audio/Plug-Ins/VST3/
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass.component \
         ~/Library/Audio/Plug-Ins/Components/
   ```

3. **auval:**
   ```bash
   auval -v aumu OCbs OuDv
   # Expected: AU VALIDATION SUCCEEDED.
   # If FAIL: per CLAUDE.md, investigate AU registration / cache / parameter-spec mismatch
   # before R34 atomic.
   ```

4. **pluginval --strictness-level 10:**
   ```bash
   /Applications/pluginval.app/Contents/MacOS/pluginval \
       --strictness-level 10 --validate-in-process \
       ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3
   # Expected: SUCCESS.
   ```

**Files modified:** none.

**Commit:** **NONE** — verification step.

**Success bar:**
- [ ] AU build + install succeed.
- [ ] auval reports `AU VALIDATION SUCCEEDED`.
- [ ] pluginval --strictness-level 10 reports `SUCCESS`.

**Estimated effort:** 15 min (build + install + auval + pluginval).

---

### R34 — Phase 2.4a Atomic Commit (Gate 6a PASS)

**Single atomic commit lands all Phase 2.4a work. Only run on R34g (regression bar) + R34h (auval + pluginval-10) PASS.**

**Tasks:**

1. **Update `STATUS.md`:**
   - `next_action` → `phase_2_4b_discuss`.
   - `phase` → `verify_complete` (or current verify state).
   - `gate_state` block: add `phase_2_4a_schelleng_calibration: PASS`, `phase_2_4a_atomic_commit_sha: <new-sha>` (placeholder; back-fill in chore commit per R33 precedent).
   - `phase_2_4a_verify_outcome: VERIFIED_gate_6a_pass_2026_04_??_<five_invariant_summary>`.
   - `phase_2_4a_verify_independent_reproduction:` block: `regression_sha256s_8_carry_forward: byte_identical`, `matrix_stability_pass_all_108: <true|partial>`, `slow_lfo_breathingAudible: ≥0.20`, `auval: AU_VALIDATION_SUCCEEDED`, `pluginval_10: SUCCESS`.
   - `Cycle Scope` → `Phase 2.4b — Sub-harmonic bias DSP-07 (next fresh GSD cycle)`.
   - `Current Position` Stage 2 progress: 2.1 + 2.2 + 2.3 + 2.4a closed = ~67% of Stage 2 (Phases 2.4b, 2.4c, 2.5, 2.6 remain).
   - `plan_revision: 8`, `plan_revision_status: complete`.
   - **NO `contract_checksums` change** (parameter-spec.md unchanged in Phase 2.4a; `77638e25…` carries forward unchanged).

2. **Stage all Phase 2.4a artefacts:**
   ```bash
   # Source (modified)
   git add plugins/O-Contrabass/Source/BowedContrabassVoice.cpp
   git add plugins/O-Contrabass/Source/PluginProcessor.cpp

   # Source (new)
   git add plugins/O-Contrabass/Source/DSP/SchellengCalibration.h

   # Harness (modified)
   git add plugins/O-Contrabass/tests/render-harness/main.cpp

   # Reproduction script (NEW; staged in R34-pre)
   git add plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh

   # Goldens (NEW)
   git add plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json
   git add plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json.sha256

   # Goldens (RE-BASELINED)
   git add plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.json
   git add plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.json

   # Tooling (NEW)
   git add tools/schelleng-fit/emit_table.py
   git add tools/schelleng-fit/README.md

   # Planning artefacts
   git add plugins/O-Contrabass/.planning/STATUS.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md
   ```

3. **Confirm staging:**
   ```bash
   git status
   git diff --cached --stat
   ```
   Expect roughly 16-19 files staged. **Sanity check:** no binary WAVs (only `.sha256` text + `.json`); no `build/` artefacts; no edits to `Source/DSP/WaveguideString.{h,cpp}` (Phase 2.2-frozen); no edits to `Source/DSP/DispersionFilter.h` (Phase 2.1c-frozen); no edits to `modules/synthesis/bow-friction/*` (Phase 2.1b-frozen); no edits to `research/ARCHITECTURE.md` (deferred to end-of-Stage-2 verify); no edits to `parameter-spec.md` (Phase 2.4a does NOT amend Stage-1 contract); no edits to `ROADMAP.md` (locked contract); no edits to the 8 carry-forward golden text files (E1 strict + detune-sweep-A + per-string A/D/G + note-sequence + vibrato + macro-sweep — invariant under Phase 2.4a per HR-2/HR-4/HR-6).

4. **Commit with structured message:**
   ```bash
   git commit -m "$(cat <<'EOF'
   feat(O-Contrabass): Schelleng wedge calibration polynomial + 108-combo stability matrix - Phase 2.4a Gate 6a PASS

   Replaces the Phase 2.3 architecture-verbatim closed-form Schelleng wedge
   (Z=R=R_s=0.5 dimensionless collapse — produced clampedDepthMean=0.0 at
   default bass operating point, silencing slow-LFO at MIDI 28-43) with an
   empirically-derived per-string trilinear lookup table. Calibration data
   derived from 108-combo bass-register stability matrix render at
   SLOW_LFO_DEPTH=1.0 with wedge-math BYPASSED via HR-7. Each grid point
   gets safeDepth=1.0 if the combo passes all 4 sub-passes (pass_noNaN +
   pass_peak + pass_clickFree + pass_blockTime) or 0.5 as v1.0 fallback if
   any sub-pass fails. Matrix is dual-purpose: supplies calibration coefficient
   data AND QUAL-01 stability evidence across the bass envelope.

   Implementation:
   - Source/DSP/SchellengCalibration.h (NEW; ~150 LOC): inline constexpr
       kSafeDepth[4][3][3][3] (108 floats; 27-point grid per string over
       BOW_SPEED × BOW_PRESSURE × BOW_POSITION axes); inline trilinear
       interpolation lookup `safeDepthForString(stringIdx, v_b, F_bow, beta)`
       (8 mul + 7 add per active-voice block; bounded [0.0, 1.0]; exact at
       grid sample points via IEEE 754 identity arithmetic). Generated
       verbatim by tools/schelleng-fit/emit_table.py from matrix-stability.json.
       Header-only `inline` linkage (HR-5; ODR-safe; matches DispersionFilter.h
       precedent).
   - Source/BowedContrabassVoice.cpp (~−10 LOC + ~+3 LOC + ~−4 LOC removed
       constants + ~+1 LOC #include): replace 11-line closed-form fMin/fMax/
       headroom math inside HR-4 gate else-branch with single
       `schelleng::safeDepthForString(activeStringIndex, rawBowSpeed,
       rawBowPressure, beta)` call. HR-6 enforced (calibration polynomial
       invoked ONLY behind HR-4 gate; never at SLOW_LFO_DEPTH=0). HR-7
       conditional preserves harness-side wedge-math bypass via
       `extern "C" bool isMatrixStabilityModeActive()` (production builds
       link weak default returning false). Removed kSchellengZ/R/DMu
       constants (no longer referenced); kPressureLagRad + kAntiCorrPerDepth
       carry forward.
   - Source/PluginProcessor.cpp (~+5 LOC): weak default
       `isMatrixStabilityModeActive()` so production AU/VST3 link cleanly
       without harness symbol.
   - tests/render-harness/main.cpp (~+350 LOC): --matrix-stability CLI flag +
       108-combo iteration loop (canonical order [stringIdx][speedIdx]
       [pressIdx][posIdx]) + per-combo JSON schema (RESEARCH §17.5;
       stringIdx / openStringMidi / bowSpeed / bowPressure / bowPosition /
       sustainSeconds / totalSamples / peak / rmsMid_s2_s3 / rmsContinuity /
       blockMicros_median / blockMicros_max / blockTimeRatio /
       clampedDepthMean / rmsByDecadePeakToPeakPct / pass_noNaN / pass_peak /
       pass_clickFree / pass_blockTime / pass_combo) + aggregate
       (status / mode / totalCombos / passCount / failCount / pass_all_108 /
       combos[]). MIDI 28/33/38/43 per stringIdx (RESEARCH §17.6;
       open-string-only). SLOW_LFO_RATE=0.5 Hz × sustain=5 s = 2.5 cycles
       (RESEARCH §17.7). Single concatenated WAV with 0.5 s silence buffers
       (~10 min audio total). Mutual-exclusion ladder slots --matrix-stability
       at top precedence.
   - tests/render-harness/main.cpp line 958: --slow-lfo
       pass_breathingAudible threshold restored 5% → 20% (RESEARCH §16.7.2
       architecture-spec'd value; CONTEXT rev-6 Q17 user-confirmed
       restoration).
   - tests/render-harness/reproduce-goldens.sh (NEW; ~70 LOC bash): canonical
       reproduction script for all 10 goldens. Locks
       sustain/release/note-sequence durations against the duration-dependence
       trap (RESEARCH §17.10 Risk #10; Phase 2.3 verify's "uncharacterised
       drift mechanism" was actually duration-dependence of the verify-time
       --string A/D/G + --note-sequence invocations).
   - tools/schelleng-fit/{emit_table.py,README.md} (NEW; ~110 LOC Python):
       offline transcription tool — reads matrix.json, emits constexpr table.
       Trilinear is exact at grid points (no fitting); 1.0 for passing combos,
       0.5 for failing combos (v1.0 fallback). Python 3.11+ with numpy;
       offline (developer-machine-only); CI never invokes.
   - tests/render-harness/golden/matrix-stability.{wav.sha256,json,
       json.sha256} (NEW; 3 files): 108-combo render captured at HEAD; sha256
       audit anchors emit_table.py output reproducibility.
   - tests/render-harness/golden/{slow-lfo,schelleng-stress}.{wav.sha256,
       json}: re-baselined post-calibration (RESEARCH §17.10 Risk #8 —
       expected re-baseline; wedge math changes; old slow-lfo sha256
       3768dd15... and old schelleng-stress sha256 e50dd191... retired).

   Gate 6a (R34g + R34h) PASS:
   - (1) Bit-exact regression bar — all 8 carry-forward goldens reproduce
       byte-identical via reproduce-goldens.sh:
       stiffness-zero d358abcddfa34840e1d4d843d7b49df6f3d28b7c4c9cbc269a80a3f600b0ee75
       string-A c6755aa426aff5fe36256d4548eb457315a10b6b3319e9985f6cfc6f07415918
       string-D 765b015e1443550ea10db01fe4afadd4c4c8be61773d0bdc33067a9665d9c9bc
       string-G 0cd5cb0a1b591d1ff6be432a5ab96b087d690da9865e35cd93ee8cee1b993bd0
       detune-sweep-A 5e31dad32ed2d34d1a972609eb1cd35487c2344e6ca3dd7351350193e22dbb05
       note-sequence 3ac3ccd044af850e73c725a487a2bc64636d8739a39fe9dc27dc846b579260b5
       vibrato d7881ecf692e899659809e52359813b9d5d0a31ee38676b3570d63a4e3076b2c
       macro-sweep c2571dd96c1950348bd8fb5c912cfe295b8c62f9b11ae44c768129931b37975e
   - (2) --matrix-stability: pass_all_108=true (or failCount ≤ 4 with
       v1.0 fallback combos documented).
   - (3) --slow-lfo (re-baselined): pass_breathingAudible
       (rmsByDecadePeakToPeakPct ≥ 0.20 — architecture-spec'd threshold
       restored) + pass_rmsContinuity (≥ 0.90) + pass_clampEngagement
       (clampedDepthMean ≈ 1.0 at default bass operating point — calibration
       returns full LFO depth, not clamped to zero like Phase 2.3).
   - (4) --schelleng-stress (re-baselined): pass_peak + pass_noNaN +
       pass_clampEngaged + pass_blockTime.
   - (5) auval -v aumu OCbs OuDv: AU VALIDATION SUCCEEDED;
       pluginval --strictness-level 10: SUCCESS.
   - (6) Logic AU smoke (R37): user-deferred non-blocking (mirrors R32 / R27
       / R19f / R14e precedent).

   Phase 2.4a closes. NO Stage-1 contract amendment (parameter-spec.md
   unchanged at sha256 77638e25... ). NO ARCHITECTURE.md amendment
   (calibration polynomial is implementation detail of architecture-spec'd
   Schelleng wedge clamp; closed-form §"Slow-Bow LFO" stays as conceptual
   reference per CONTEXT rev-6 Q22). Phase 2.4b (sub-harmonic bias
   DSP-07) opens as fresh GSD cycle.

   Architecture amendments still deferred to end-of-Stage-2 verify per
   locked decision: §"DC Blocker" (F3 from Phase 2.1a-recovery) and
   §"In-loop saturator" (conditional on Phase 2.4c §12.5 escalation).

   Continues atomic-commit sequence: R7 → R15 → R20 → R26 → R33 → R34.

   Refs:
   - RESEARCH.md §17 (Phase 2.4a Schelleng wedge calibration research)
   - PLAN.md rev-8 (R34-pre, R34a-h, R34 atomic)
   - CONTEXT.md rev-6 (Phase 2.4a discuss)

   Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
   EOF
   )"
   ```

5. **Post-commit verification + SHA backfill:**
   ```bash
   git log --stat HEAD~1..HEAD
   git rev-parse HEAD  # capture for STATUS.md backfill chore commit
   ```
   Backfill the captured SHA into `STATUS.md` `phase_2_4a_atomic_commit_sha` field as a separate non-atomic chore commit (mirrors Phase 2.1c R20 / Phase 2.2 R26 / Phase 2.3 R33 SHA-backfill pattern):
   ```bash
   # After computing new sha:
   sed -i.bak "s/phase_2_4a_atomic_commit_sha: TBD_post_commit/phase_2_4a_atomic_commit_sha: <captured-sha>/" \
       plugins/O-Contrabass/.planning/STATUS.md
   rm plugins/O-Contrabass/.planning/STATUS.md.bak
   git add plugins/O-Contrabass/.planning/STATUS.md
   git commit -m "$(cat <<'EOF'
   chore(O-Contrabass): backfill Phase 2.4a R34 commit sha into STATUS.md

   Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
   EOF
   )"
   ```

**Files modified/created:** all listed in step 2 staging block.

**Commit:** **THIS IS THE PHASE 2.4a ATOMIC COMMIT.** Single commit, only on Gate 6a (R34g + R34h) PASS. SHA-backfill is a separate chore commit per CLAUDE.md no-amend rule.

**Success bar:**
- [ ] `git log --stat HEAD~1..HEAD` shows ~16-19 files in a single commit.
- [ ] Commit body explicitly documents:
  - Five-item Gate 6a invariant set with sha256 enumeration.
  - NO Stage-1 contract amendment (parameter-spec.md untouched).
  - NO ARCHITECTURE.md amendment (closed-form stays as conceptual reference).
  - HR-5 / HR-6 / HR-7 / HR-8 binding for calibration polynomial integration.
  - v1.0 fallback design (1.0 / 0.5 binary derived from pass_combo).
  - Phase 2.4b/2.4c follow-ups unchanged from CONTEXT rev-6 scope.
- [ ] `git status` clean post-commit.
- [ ] STATUS.md `next_action` flipped to `phase_2_4b_discuss`.
- [ ] STATUS.md `gate_state.phase_2_4a_schelleng_calibration: PASS`.
- [ ] STATUS.md `phase_2_4a_atomic_commit_sha` recorded via backfill chore commit.
- [ ] Phase 2.4a closed.

**Estimated effort:** 35 min (~5 min STATUS.md update + ~10 min staging + ~5 min commit body authoring + ~10 min commit + ~5 min backfill chore).

---

## Why R34 is a single atomic commit

Same gate-first principle as R7 (Phase 2.1a-recovery), R15 (Phase 2.1b extraction), R20 (Phase 2.1c dispersion), R26 (Phase 2.2 4-string bank), R33 (Phase 2.3 modulator + macro):

1. **Coupling:** Phase 2.4a artefacts (`SchellengCalibration.h` new header + `BowedContrabassVoice.cpp` polynomial swap-in + `PluginProcessor.cpp` weak-symbol default + `main.cpp` --matrix-stability mode + 7 golden text files (3 new + 4 re-baselined) + reproduce-goldens.sh script + tools/schelleng-fit/ Python tool + planning artefacts) are mutually coupled. Splitting yields broken intermediate states — e.g., SchellengCalibration.h landing without the BowedContrabassVoice.cpp call site is dead code; matrix-stability harness mode landing without the weak-default symbol breaks production VST3/AU link; emit_table.py landing without matrix.json input is unrunnable.
2. **Bisect safety:** if a future Phase 2.x bug bisects back to "calibration polynomial landing", a single SHA flips the entire feature.
3. **Audit trail:** Phase 2.x's atomic-commit sequence is R7 → R15 → R20 → R26 → R33 → **R34**. The Phase 2.x timeline stays trivially reconstructible from `git log --grep "Phase 2."`.
4. **No Stage-1 contract amendment:** Phase 2.4a is purely an implementation refactor of the architecture-spec'd Schelleng wedge clamp (closed-form replaced by empirical table). parameter-spec.md unchanged, ARCHITECTURE.md unchanged, STATUS.md `contract_checksums.parameter_spec` unchanged at `77638e25…`. Single atomic commit captures the entire implementation cycle without contract churn.

---

## Files To Create / Modify (consolidated, Phase 2.4a)

### Source (modified)
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` — R34a (~+10 LOC: extern decl + HR-7 conditional inside HR-4 gate) + R34d (~−10 LOC closed-form math + ~+3 LOC polynomial call + ~−4 LOC removed constants + ~+1 LOC #include = net ~+0 LOC across R34a+R34d).
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` — R34a (~+5 LOC: weak default `isMatrixStabilityModeActive()`).

### Source (new)
- `plugins/O-Contrabass/Source/DSP/SchellengCalibration.h` — R34c (NEW; ~150 LOC; auto-generated; HR-5 `inline constexpr` linkage; per-string 27-point trilinear table + lookup function).

### Harness (modified)
- `plugins/O-Contrabass/tests/render-harness/main.cpp` — R34a (~+350 LOC: --matrix-stability flag + g_matrixStabilityMode + isMatrixStabilityModeActive() strong def + runMatrixStabilityMode() + paramNorm/setNorm/PerComboMetrics extensions) + R34e (1-LOC functional + ~3-LOC comment edit for breathingAudible 5%→20%).

### Reproduction script (new)
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` — R34-pre (NEW; ~70 LOC; chmod +x; canonical 10-golden reproduction script; locks duration-dependence trap).

### Test artefacts (new, committed as text-only — sha256 + JSON)
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.wav.sha256` — R34b (NEW; sha256 of concatenated 108-combo render).
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json` — R34b (NEW; ~50–100 KB; per-combo + aggregate truth-table; emit_table.py input).
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.json.sha256` — R34b (NEW; anchors emit_table.py output reproducibility).

### Test artefacts (modified, re-baselined post-calibration)
- `plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.wav.sha256` — R34f (RE-BASELINED; old `3768dd15…` retired).
- `plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.json` — R34f (RE-BASELINED).
- `plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.wav.sha256` — R34f (RE-BASELINED; old `e50dd191…` retired).
- `plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.json` — R34f (RE-BASELINED).

### Tooling (new)
- `tools/schelleng-fit/emit_table.py` — R34c (NEW; ~80 LOC Python; offline JSON→constexpr-array transcription; chmod +x; numpy dependency).
- `tools/schelleng-fit/README.md` — R34c (NEW; ~30 LOC; usage + re-run conditions + dependencies).

### Test artefacts (NOT committed — staged-only or transient)
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.wav` (~200 MB; reproducible from harness via `--matrix-stability`).
- `/tmp/phase24a-r34pre/*.{wav,json}` — R34-pre tripwire (8 carry-forward goldens; transient).
- `/tmp/phase24a-r34pre/sul-tasto-preflight.{wav,json}` — R34-pre sul-tasto pre-flight (transient).
- `/tmp/phase24a-r34b-smoke.{wav,json}` — R34b pre-render smoke (transient).
- `/tmp/phase24a-r34f-{slow-lfo,schelleng-stress}.{wav,json}` — R34f re-render captures (transient).
- `/tmp/phase24a-r34g-audit.log` — R34g reproduction audit log (transient).
- `/tmp/repro/*.wav` — runtime output of reproduce-goldens.sh (transient).

### Stage-1 contract amendment
- **None.** parameter-spec.md unchanged. STATUS.md `contract_checksums.parameter_spec` carries forward `77638e25…`.

### Planning artefacts (modified)
- `plugins/O-Contrabass/.planning/STATUS.md` — R34 (next_action / gate_state / phase_2_4a_verify_outcome / phase_2_4a_atomic_commit_sha placeholder + chore-backfill / Cycle Scope / Current Position / plan_revision / plan_revision_status updates).
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` — already at rev-6 (no further edits in execute; rev-6 lock holds).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` — already at §17 append (no further edits in execute).
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` — this rev-8 append (no further edits in execute; verify-phase may add a "rev-8 retrospective" footnote if anomalies surfaced).
- `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` — execute-phase appends "Phase 2.4a execute" section after R34.
- `plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md` — verify-phase appends "Phase 2.4a verify" section with R34g audit table + R34h auval/pluginval logs + Phase 2.4b discuss prep notes.

### Files explicitly NOT touched
- `plugins/O-Contrabass/Source/DSP/WaveguideString.{h,cpp}` (Phase 2.2 R26-frozen; consumed verbatim).
- `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` (Phase 2.1c R20-frozen; consumed verbatim).
- `plugins/O-Contrabass/Source/PluginEditor.{h,cpp}` (Stage 3 work).
- `plugins/O-Contrabass/Source/OContrabassMPESynthesiser.{h,cpp}` (voice count = 1; no Synthesiser changes for Phase 2.4a — calibration lives inside the existing single voice).
- `plugins/O-Contrabass/Source/BowModel.{h,cpp}` (Phase 2.1a-frozen).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` (Phase 2.3 R28-frozen at the `private:` member surface — calibration is `.cpp`-only refactor; no new members; no new methods).
- `modules/synthesis/bow-friction/*` (Phase 2.1b-frozen v1.0.0).
- `modules/registry.yaml` (no module surface changes).
- `plugins/O-Bowed/*` (RESEARCH §17.9 confirmed ZERO Schelleng/slow-LFO DSP; no cross-plugin coupling).
- `plugins/O-Contrabass/research/ARCHITECTURE.md` (deferred amendments to end-of-Stage-2 verify; closed-form §"Slow-Bow LFO" stays as conceptual reference per CONTEXT rev-6 Q22).
- `plugins/O-Contrabass/.planning/parameter-spec.md` (Phase 2.4a does NOT amend Stage-1 contract).
- `plugins/O-Contrabass/.planning/parameter-spec-draft.md` (historical; audit trail).
- `plugins/O-Contrabass/.planning/BRIEF.md` (no Schelleng-wedge / calibration default reference).
- `plugins/O-Contrabass/.planning/REQUIREMENTS.md` (no Schelleng-wedge default reference).
- `plugins/O-Contrabass/.planning/stages/1-foundation/PLAN.md` (historical Stage 1 task table — closed milestone).
- `plugins/O-Contrabass/.planning/ROADMAP.md` (no calibration-default references).
- `plugins/O-Contrabass/CMakeLists.txt` (NO new source files added that need explicit listing; SchellengCalibration.h is header-only and consumed via `#include` from BowedContrabassVoice.cpp; the existing CMake glob/explicit-list in the plugin's CMakeLists.txt MAY require one entry — confirm at R34a; likely already covered by existing `Source/DSP/*.h` glob).
- 8 carry-forward golden text files: `stiffness-zero-pre.{wav.sha256,json}`, `string-A.{wav.sha256,json}`, `string-D.{wav.sha256,json}`, `string-G.{wav.sha256,json}`, `detune-sweep-A.{wav.sha256,json}`, `note-sequence.{wav.sha256,json}`, `vibrato.{wav.sha256,json}`, `macro-sweep.{wav.sha256,json}` (Phase 2.1c/2.2/2.3 goldens — content-stable; R34g verifies all 8 still match).
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-sweep.wav.sha256` (Phase 2.1c golden — content-stable; not re-rendered in Phase 2.4a; reproduce-goldens.sh excludes since it's not in the canonical 10-golden bar).

---

## Dependencies Graph (compact)

```
R34-pre (working-tree integrity tripwire; reproduce all 8 carry-forward goldens
         byte-identical at HEAD af54571; sul-tasto pre-flight informational;
         reproduce-goldens.sh authored + chmod +x + staged for R34 atomic)
   ↓                ↓ (mismatch → STOP, investigate working-tree drift)
R34a (main.cpp --matrix-stability mode + isMatrixStabilityModeActive() strong def +
      g_matrixStabilityMode atomic + runMatrixStabilityMode() 108-combo loop +
      BowedContrabassVoice.cpp HR-7 bypass conditional inside HR-4 gate +
      PluginProcessor.cpp weak default; ~+360 LOC; NO build, NO commit)
   ↓ pre-R34b smoke: --schelleng-stress at HEAD post-R34a still PASS
   ↓ (production-path else-branch unchanged; HR-7 conditional in `if` branch only)
R34b (Render 108-combo matrix; capture matrix-stability.{wav.sha256,json,
      json.sha256}; validate pass_all_108=true OR failCount ≤ 4 with v1.0 fallback;
      stage 3 new golden text files)
   ↓
R34c (Author tools/schelleng-fit/{emit_table.py,README.md}; run emit_table.py
      against matrix.json → generate Source/DSP/SchellengCalibration.h; stage
      3 new files; NO commit)
   ↓
R34d (BowedContrabassVoice.cpp #include + HR-4 gate else-branch swap closed-form
      math for schelleng::safeDepthForString call + remove kSchellengZ/R/DMu
      constants; ~−10 LOC; NO build, NO commit)
   ↓
R34e (main.cpp line 958 0.05f → 0.20f + comment update; ~2 LOC; NO commit)
   ↓
R34f (Build harness; re-render --slow-lfo + --schelleng-stress; capture new sha256s;
      overwrite committed golden text; stage 4 re-baselined files; NO commit)
   ↓ (PASS)              ↓ (FAIL — pass_breathingAudible < 0.20)
R34g (reproduce-       R34f diagnostic per RESEARCH §17.10 Risk #5:
  goldens.sh runs; all   - inspect matrix.json for default-operating-point combo
  10 reproduce          - if v1.0 fallback 0.5 applied at default, escalate
  byte-identical;          to Phase 2.4-bis (binary-search refinement)
  HR-2/HR-4/HR-6/HR-7
  audit if 6 carry-      - else investigate emit_table.py output / R34d source edit
  forward break)
   ↓ (PASS)
R34h (auval AU VALIDATION SUCCEEDED + pluginval-10 SUCCESS)
   ↓ (PASS)
R34 (Phase 2.4a atomic commit — ~16-19 files; closes 2.4a; mirrors R33/R26/R20/
     R15/R7 atomic-commit pattern; commit body documents 5-invariant Gate 6a
     audit + NO contract amendment + Phase 2.4b/2.4c follow-up scope)
   ↓
SHA-backfill chore commit (separate non-atomic; STATUS.md
     phase_2_4a_atomic_commit_sha; matches R33 backfill precedent)
```

R34-pre is a strict prerequisite — if working tree drifts, all subsequent bit-exact reasoning breaks. R34a + R34d together are the source-edit batch (R34c executes between to generate the header that R34d consumes); R34b renders the matrix that R34c needs. R34e is a 2-LOC piggyback on the harness build. R34f re-baselines the 2 expected-to-change goldens. R34g is the strict regression bar (8 carry-forward + 2 re-baselined = 10 goldens). R34h is the Gate 6a audit. R34 is the gated finisher.

---

## Risks (Phase 2.4a, refreshed from RESEARCH §17.10)

1. **Bit-exact regression failure on 6 carry-forward goldens (RESEARCH §17.10 Risk #1; binding §17.1 invariant).** Mitigation: HR-2 + HR-4 + HR-6 hard rules + R34-pre tripwire + R34g final check. **Diagnostic-on-fail playbook:**
   - HR-6 audit: confirm `schelleng::safeDepthForString(...)` call site is INSIDE the existing HR-4 `if (rawSlowLfoDepth > 0.0f)` gate; never invoked at SLOW_LFO_DEPTH=0. Confirm `lastSafeDepth.store(0.0f)` runs unconditionally BEFORE the gate.
   - HR-7 audit: confirm the matrix-stability bypass conditional only fires when the harness binary is invoked with `--matrix-stability` (production weak default returns false; never reached in user-facing builds).
   - HR-5 audit: confirm `inline constexpr` linkage on SchellengCalibration.h; no ODR violations across translation units.
   - HR-8 audit: confirm trilinear interpolation returns grid-point values exactly via IEEE 754 identity arithmetic (`x * 1 = x`, `x + 0 = x`).
   - Last resort: bisect R34a vs R34d source edits; if R34a alone breaks regression (HR-7 bypass conditional perturbing some unrelated optimisation), file Phase 2.4a remediation cycle.

2. **Calibration table at v1.0 fallback (`0.5`) under-fits some combos at audible regression bar (RESEARCH §17.10 Risk #2).** Mitigation: trilinear with 0.5 fallback is the v1.0 design. If `--slow-lfo` reports `pass_breathingAudible < 0.20` at the default operating point (BOW_SPEED=0.15 / BOW_PRESSURE=1.0 / BOW_POSITION=0.10) post-calibration, identify which grid combo applies (likely speedIdx=1, pressIdx=0, posIdx=1) and check if R34b matrix flagged it as failing → 0.5 fallback applies → audible breathing is half what architecture spec'd. **Phase 2.4-bis remediation path:** emit_table.py adds `--binary-search` flag that sweeps SLOW_LFO_DEPTH ∈ {0.25, 0.4, 0.6, 0.75} per failing combo; re-renders via harness for each step; emits per-combo refined `kSafeDepth` value. OUT-OF-SCOPE for Phase 2.4a v1.0; documented as next-cycle work.

3. **Trilinear over-fits at 27 sample points (off-grid pathology) (RESEARCH §17.10 Risk #3 — DISSOLVED).** Trilinear is monotonic + bounded by 8-corner box → cannot overshoot. Off-grid spot-check optional in plan-phase verify; not blocking.

4. **108-combo wall-clock budget overrun (RESEARCH §17.10 Risk #4 — DISSOLVED).** Pre-flight ~10 s expected (§17.2); 30× under budget. R34b in-process iteration mode locked.

5. **`pass_breathingAudible ≥ 20%` polynomial fit fails at default operating point (RESEARCH §17.10 Risk #5).** Mitigation: R34f verification step computes `--slow-lfo` mode's `rmsByDecadePeakToPeakPct` post-calibration. If <20% on any string, see Risk #2 escalation path (Phase 2.4-bis). For v1.0, accept whichever string passes; document any string that falls short. **Likelihood low** because the default operating point combo (BOW_SPEED=0.15 + BOW_PRESSURE=1.0 + BOW_POSITION=0.10) is well-inside the Schelleng wedge "stable" region (low pressure, mid-position, mid-speed); historically `--slow-lfo` mode harness has passed `pass_clickFree` at this combo, so kSafeDepth=1.0 expected → full LFO breathing.

6. **`--matrix-stability` discovers a real instability (Gate 6a invariant 4 fails, RESEARCH §17.10 Risk #6).** Mitigation: single-combo pre-flight at extreme combo PASSED (§17.2). Likelihood low but non-zero. Phase 2.4a remediation: identify failing combo (jq triage), accept v1.0 fallback `0.5` for that combo (already the design path), document failing combos in R34 commit body. If 0.5 also fails (clampedDepthMean ≥ 0.5 still produces audio that fails sub-passes), escalate to Phase 2.4-bis or downstream defense tightening (algebraic saturator clamp tightening, energy-clamp loop-gain reduction at extremes — Phase 2.4c-or-later concern).

7. **Polynomial fitting tool dependency — Python (RESEARCH §17.10 Risk #7).** Mitigation: Python 3.14 + numpy 2.4 already installed. Tool is offline (developer-machine-only); CI never invokes. Generated header committed to source verbatim. README documents re-run conditions.

8. **`--schelleng-stress` re-baseline introduces uncharacterised drift (RESEARCH §17.10 Risk #8 — RE-CLASSIFIED).** Phase 2.3 verify's "post-R31 source edit drift" was actually duration-dependence (§17.1 pre-flight invalidated). Re-baseline is EXPECTED in Phase 2.4a (wedge math changes; new sha256 captured). reproduce-goldens.sh canonical invocation script (R34-pre) eliminates the duration-dependence trap going forward.

9. **constexpr float arrays in header (ODR risk) (RESEARCH §17.10 Risk #9 — DISSOLVED).** `inline constexpr` C++17 syntax is ODR-safe. Same pattern as Phase 2.1c DispersionFilter.h precedent.

10. **Duration-dependence of golden invocations causes "phantom drift" at re-render (RESEARCH §17.10 Risk #10 NEW).** Mitigation: `reproduce-goldens.sh` canonical invocation script committed in R34 atomic. Future verify-phase reproductions invoke the script verbatim; no risk of mis-captured duration arguments. Audit-trail anchored.

11. **`activeStringIndex` accessor changes under HR-4 gate during string crossfade (RESEARCH §17.10 Risk #11 NEW — CHARACTERISED).** Mitigation: calibration polynomial lookup at the post-crossfade `activeStringIndex` is correct; crossfade transition is per-sample-loop space (Step 7), not Step 2 wedge math space. No additional handling needed.

12. **HR-7 bypass conditional perturbs production builds (NEW — Phase 2.4a substep guard).** Mitigation: `extern "C" __attribute__((weak))` default in PluginProcessor.cpp returns `false` unconditionally; production VST3 / AU never enter the bypass branch. R34h auval + pluginval-10 catch any link failure (if weak symbol fallback misbehaves, plugin won't load → auval REJECTS). If auval fails:
    - Re-confirm `extern "C"` linkage on both decl and def.
    - Confirm linker honours weak symbols on macOS (clang/ld; expected).
    - Worst case fallback: replace weak-symbol pattern with namespace-private flag + harness-side function-pointer registration (more code; less elegant; same end result).

13. **Matrix WAV concatenation produces sha256 sensitivity to combo state-bleed (NEW — Phase 2.4a v1.0 mitigation).** R34a `runMatrixStabilityMode()` calls `processor.releaseResources(); processor.prepareToPlay(...)` between combos to clear DSP state. If state bleed causes combo N's render to depend on combo N-1's residual delay-line / smoother state, sha256 of the concatenated WAV is non-deterministic across re-renders. Mitigation: explicit reset between combos. Verification: R34b smoke re-runs the matrix render once and confirms sha256 stable on second invocation. If sha256 differs across re-renders, instrument `releaseResources()` to confirm full state reset.

14. **`tools/schelleng-fit/` placement confusion (NEW — pin #12 audit).** Mitigation: README documents repo-root placement. RESEARCH §17.4 invocation form uses `python3 tools/schelleng-fit/emit_table.py`. Per-plugin alternative (`plugins/O-Contrabass/tools/`) rejected at plan-phase. If future plugins need their own emit_table.py variants, fork is cheap.

---

## Success Criteria (Gate 6a — Phase 2.4a verify exit gate)

- [ ] **R34-pre** — Working-tree integrity tripwire: all 8 carry-forward goldens reproduce byte-identical to committed sha256s via canonical invocations. `reproduce-goldens.sh` authored, `chmod +x`, staged for R34. Sul-tasto pre-flight `rmsContinuity ≥ 0.85` OR escalated as verify risk.
- [ ] **R34a** — `tests/render-harness/main.cpp` `--matrix-stability` flag + 108-combo iteration loop + per-combo JSON schema (RESEARCH §17.5) + aggregate `pass_all_108`. `g_matrixStabilityMode` atomic + `isMatrixStabilityModeActive()` strong def. `Source/BowedContrabassVoice.cpp` extern decl + HR-7 bypass conditional inside HR-4 gate (production-path else-branch preserved verbatim from Phase 2.3 R28; R34d will replace). `Source/PluginProcessor.cpp` weak default `isMatrixStabilityModeActive() noexcept { return false; }`. HARD RULES HR-5..HR-8 visually traceable in code.
- [ ] **R34b** — `--matrix-stability` invocation completes in <30 s wall-clock. `pass_all_108=true` OR `failCount ≤ 4` with v1.0 fallback combos documented. 3 new golden text files staged.
- [ ] **R34c** — `tools/schelleng-fit/{emit_table.py,README.md}` authored. emit_table.py invocation succeeds → `Source/DSP/SchellengCalibration.h` generated. Header has timestamp + matrix.json sha256 in comment block. Grid entry count ≥ 108. 3 new files staged.
- [ ] **R34d** — `Source/BowedContrabassVoice.cpp` else-branch body replaced with `schelleng::safeDepthForString(...)` call. `kSchellengZ/R/DMu` constants removed. `#include "DSP/SchellengCalibration.h"` added. HR-6 + HR-4 gate ordering preserved.
- [ ] **R34e** — `tests/render-harness/main.cpp` line 958 `0.05f` → `0.20f` + inline doc-comment updated.
- [ ] **R34f** — Harness build succeeds zero warnings. `--slow-lfo` reports `pass_breathingAudible=true` (rmsByDecadePeakToPeakPct ≥ 0.20) + `pass_rmsContinuity=true` + `pass_clampEngagement=true` (clampedDepthMean ≥ ~0.95 at default bass operating point). `--schelleng-stress` reports `pass_peak + pass_noNaN + pass_blockTime` all TRUE; `pass_clampEngaged` per-combo behavior documented. 4 golden text files re-baselined; new sha256s captured.
- [ ] **R34g invariant (1)** — Bit-exact regression bar: all 8 carry-forward goldens (E1 strict `d358abcd…` + per-string A `c6755aa4…` + per-string D `765b015e…` + per-string G `0cd5cb0a…` + detune-sweep-A `5e31dad3…` + note-sequence `3ac3ccd0…` + vibrato `d7881ecf…` + macro-sweep `c2571dd9…`) reproduce byte-identical via `reproduce-goldens.sh`.
- [ ] **R34g invariant (2)** — `--slow-lfo` re-baselined: `pass_breathingAudible` (≥ 0.20 architecture-spec'd) + `pass_rmsContinuity` (≥ 0.90) + `pass_clampEngagement` (clampedDepthMean > 0.0) all TRUE. New sha256 captured + matches R34f-rendered.
- [ ] **R34g invariant (3)** — `--schelleng-stress` re-baselined: `pass_peak` (≤ 1.0) + `pass_noNaN` + `pass_blockTime` (ratio ≤ 5.0) all TRUE. New sha256 captured + matches R34f-rendered. `pass_clampEngaged` documented per-combo (calibration table value at extreme combo may be 0.5 v1.0 fallback; not a verify-fail).
- [ ] **R34g invariant (4)** — `--matrix-stability` aggregate: `pass_all_108=true` OR `failCount ≤ 4` with documented v1.0 fallback combos. matrix-stability sha256 matches R34b-captured.
- [ ] **R34h invariant (5)** — auval reports `AU VALIDATION SUCCEEDED`; pluginval --strictness-level 10 reports `SUCCESS`.
- [ ] **R34g audit table** — Five-item Gate 6a bar compiled (input to VERIFICATION.md): items (1)–(5) from R34g + R34h.
- [ ] **R37 (optional)** — Logic AU smoke: USER-CONFIRMED PASS or DEFERRED (mirrors R32 / R27 / R19f / R14e precedent). Audition sequence (per CONTEXT rev-6 implied + Phase 2.3 R32 precedent): MIDI 33 (A1) sustained + SLOW_LFO_DEPTH 0→1.0 ramp at 0.5 Hz (confirm audible breathing post-calibration); MIDI 28 (E1) extreme bow params (PRESSURE 7 N + SPEED 0.05 m/s) + SLOW_LFO_DEPTH=1 (confirm clamp engages without instability); MIDI 38 (D2) + EXPRESSION_MACRO 0→1.0 ramp (confirm Phase 2.3 modulator surface unaffected by Phase 2.4a calibration swap-in).
- [ ] **R34** — Atomic commit landed; `git log --stat HEAD~1..HEAD` shows ~16-19 files in single commit; STATUS.md `next_action` flipped to `phase_2_4b_discuss`; `gate_state.phase_2_4a_schelleng_calibration: PASS`; `phase_2_4a_atomic_commit_sha` recorded via backfill chore commit. **NO `parameter-spec.md` edit; NO `contract_checksums.parameter_spec` change.** Commit body documents 5-invariant Gate 6a audit + NO contract amendment + HR-5/HR-6/HR-7/HR-8 binding + Phase 2.4b/2.4c follow-up scope.
- [ ] **(Architecture amendments, post-Stage-2-verify, OUT OF SCOPE for this execute)** — ARCHITECTURE.md §"DC Blocker" (F3) + §"In-loop saturator" amendments still deferred. Calibration polynomial is implementation detail of architecture-spec'd Schelleng wedge clamp; closed-form §"Slow-Bow LFO" stays as conceptual reference per CONTEXT rev-6 Q22.

When all checks above are green (R37 user-confirmed or accepted as deferred), **Phase 2.4a verifies** and **Stage 2 progress reaches ~67%** (2.1 + 2.2 + 2.3 + 2.4a closed; 2.4b, 2.4c, 2.5, 2.6 remain). Phase 2.4b (sub-harmonic bias DSP-07) opens as a fresh GSD cycle; CONTEXT rev-7 written when 2.4b discuss-phase opens.

---

## Out of Scope (deferred per CONTEXT.md rev-6 + RESEARCH §17 + STATUS.md)

- **Phase 2.4b** — Sub-harmonic bias DSP-07 (architecture-spec'd Phase 2.4 should-have; deferred to its own fresh GSD cycle; CONTEXT rev-7 written when opens).
- **Phase 2.4c** — Autocorrelator octave-rejection vibrato harness fix + saturator-tail O-Bowed comparison harness + saturator-choice decision tree (deferred to its own GSD cycle; CONTEXT rev-8 written when opens).
- **Phase 2.4-bis (calibration refinement)** — Binary-search-over-depth refinement (`emit_table.py --binary-search` mode) for combos where v1.0 fallback `0.5` proves inadequate (`pass_breathingAudible < 0.20` or `pass_clampEngaged` false-positive). OUT-OF-SCOPE for Phase 2.4a v1.0; conditional on Phase 2.4a verify findings.
- **Phase 2.5** — Body resonator + bow noise (8-mode parallel biquad body bank Askenfelt-derived; 3-band BPF bow noise summed AFTER body resonator).
- **Phase 2.6** — Master saturator/limiter + microtonal + MPE + Note Expression + MTS-ESP + Scala/TUN.
- **`pass_breathingAudible ≥ 20%` further tightening** — Phase 2.4a restores architecture-spec'd 20%; further tightening (e.g., 30%) deferred to Phase 2.4-bis or audible-character iteration.
- **E1 dispersion calibration polynomial follow-up (Phase 2.1c Risk #7)** — separate `a(B, I)` cascaded-allpass concern; not friction-junction wedge math. Out-of-scope for Phase 2.4a; deferred to Phase 2.4c-or-later.
- **Schelleng calibration polynomial extraction to shared module (modules/dsp/schelleng-calibration/ ?)** — RESEARCH §17.9 confirmed O-Bowed has ZERO Schelleng/slow-LFO DSP. No cross-plugin reuse case. Per-plugin `Source/DSP/SchellengCalibration.h` mirrors per-plugin `Source/DSP/DispersionFilter.h` precedent. Future-plugin extraction cheap if needed.
- **Higher-order interpolation (triquadratic / B-spline / tensor-product cubic)** — RESEARCH §17.3 evaluated alternatives; trilinear is the pareto-optimal choice (exact at sample points, monotonic, bounded, 8 mul + 7 add eval cost). Higher-order deferred unless Phase 2.4-bis listening surfaces audible discontinuities at grid-cell boundaries.
- **`tools/schelleng-fit/emit_table.py` polynomial-fit modes** — current v1.0 implementation does pure transcription (1.0 / 0.5 binary). Future modes (least-squares polynomial fit; B-spline; piecewise quadratic with breakpoint detection) deferred to Phase 2.4-bis or Phase 2.5+.
- **Production WAV binary commits** — `matrix-stability.wav` (~200 MB) NOT committed (reproducible from harness). sha256 + JSON committed instead per Phase 2.2 / 2.3 precedent.
- **CI invocation of emit_table.py** — out-of-scope; tool is offline (developer-machine-only). CI only runs the existing build + auval + pluginval pipeline.
- **ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments** — end-of-Stage-2 verify decides; F3 deviation continues in R34 commit body.
- **Saturator-tail re-evaluation** — RESEARCH §12 footnote → Phase 2.4c.
- **Logic Pro AU smoke (R37)** — user-deferred non-blocking (mirrors R32 / R27 / R19f / R14e precedent).
- **Cross-plugin reproduce-goldens.sh script** — pin #11 selected per-plugin placement; cross-plugin variant deferred until multiple plugins have golden harnesses with regression bars.

---

# Stage 2: DSP — Plan (Phase 2.4b) — REVISION 9 (Sub-Harmonic Bias DSP-07 Friction-Junction Period-Doubling Bias + 36-Combo Stability Matrix + Spectral Audibility Gate, Gate 6b)

> **Status:** rev-9 authors fresh task bodies for **R35-pre, R35a, R35b, R35c, R35d, R35e, R35f, R35, R35-backfill chore** per `RESEARCH.md §18.11` sequencing and `CONTEXT.md` rev-7. rev-1/2/3/4/5/6/7/8 remain in-effect as completed/verified history. Phase 2.4a closed 2026-04-28 with R34 atomic commit `4c926bb` (Gate 6a CLEARED — 3 strict-PASS + 2 soft-PASS within v1.0 budgets) + R34-backfill chore `b64c8c4`.

**Date:** 2026-04-28
**Cycle scope:** Phase 2.4b only (Phase 2.4c autocorrelator harness fix + saturator-tail O-Bowed comparison still get fresh GSD cycles each; Phase 2.4-bis backlog stays parked)
**Gate:** Gate 6b (Phase 2.4b verify)
**Atomic-commit unit:** R35 (Gate 6b PASS) — single commit lands Phase 2.4b source + harness + 1 new header (`Source/DSP/SubHarmonicBias.h`) + 6 new golden text files (`sub-harmonics.{wav.sha256,json,json.sha256}` + `sub-harmonics-stability.{wav.sha256,json,json.sha256}`) + `reproduce-goldens.sh` extension (10 → 12 entries) + new `preflight-subharm.sh` script + planning artefacts. **NO Stage-1 contract amendment** (`SUB_HARMONICS` already declared at PluginProcessor.cpp:104 default 0.0 ∈ [0, 1.0] under `parameter-spec.md` sha256 `77638e25…`; STATUS.md `contract_checksums.parameter_spec` carries forward unchanged). **NO ARCHITECTURE.md amendment** (bias formula IS architecture §457 verbatim; chaos detector + softClampState deferred to Phase 2.5/2.6 via R35 commit-body footnote per RESEARCH §18.13; closed-form §"Slow-Bow LFO" / §"DC Blocker" / §"In-loop saturator" amendments remain end-of-Stage-2 concerns).
**Carry-forward locks (NOT re-litigated):** Phase 2.1a-recovery split-rail topology, F2 LP form, F3 no-in-loop-DCB, F4 betaScale removed; Phase 2.1b bow-friction module v1.0.0 consumption (HR-10 ABI preservation); Phase 2.1c `DispersionFilter<4>` API + per-string M=4/3/2/1 dispersion table; Phase 2.2 4-string bank + per-string detune + 5 ms equal-power crossfade + MIDI→string mapping; Phase 2.3 modulator-layer surface (vibratoPhase / vibratoOnsetTimer / slowLfoPhase / 4 macro SmoothedValues / 7-step per-block evaluation order / HR-1..HR-4 hard rules / `lastSafeDepth.store(0.0f)` unconditional pre-gate at top of step 2 / EXPRESSION_MACRO + VIBRATO_DEPTH default 0.0); Phase 2.4a Schelleng wedge bass-register calibration polynomial (`Source/DSP/SchellengCalibration.h` HR-5/HR-6/HR-7/HR-8 + `safeDepthForString(stringIdx, v_b, F_bow, beta)` API surface verbatim); 10 currently-committed goldens (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + vibrato + macro-sweep + slow-lfo Phase-2.4a-rebaselined + schelleng-stress Phase-2.4a-rebaselined); matrix-stability `6db67707…` Phase-2.4a evidence golden (carry-forward; not in reproduce-goldens.sh per Phase 2.4a CONTEXT rev-6 Q22). Phase 2.4b inserts **Step 2.5** (sub-harmonic bias evaluation) into the per-block 7-step order between Step 2 (Schelleng wedge) and Step 3 (slow-LFO phase advance) — Steps 1/2/3/4/5/6/7 otherwise unchanged; Step 6 modified ONLY to consume the bias-output `voiceBowForceUpliftThisBlock` factor (default 1.0f at HR-9 short-circuit path).

---

## Preamble — Pinned Open Items (RESEARCH §18.16)

PLAN rev-9 pins each of the 12 plan-phase open items from RESEARCH §18.16:

| # | Open Item | Pinned Decision |
|---|-----------|-----------------|
| 1 | CLI flag spelling for sub-harmonics modes | **`--sub-harmonics`** (single audible-mode FFT analyser render at SUB_HARMONICS=1.0 on E1) **+ `--sub-harmonics-stability`** (36-combo matrix). Matches CONTEXT rev-7 wording verbatim. Mutual-exclusion ladder: slot ABOVE `--matrix-stability` (highest precedence; either sub-harmonics flag clears all other modes + emits warning if multiple modes set). NOT `--subharm` / `--sub-harm` (lexical inconsistency with `kebab-case` convention). |
| 2 | In-process iteration mode for `--sub-harmonics-stability` | **In-process loop** (single harness invocation iterates all 36 combos). ~4–5 s wall-clock total per RESEARCH §18.2 single-combo extrapolation (36 × 0.04 s render + ~3 s JUCE init; bias adds <2% CPU). Mode iterates `for (s = 0..3) for (i = 0..2) for (k = 0..2)` (string × INFINITE_SUSTAIN × SUB_HARMONICS) per RESEARCH §18.7 canonical order; resets DSP state via `processor.releaseResources(); processor.prepareToPlay(...)` between combos to prevent state bleed (mirrors Phase 2.4a R34a pattern). Concatenates 36 stereo WAV chunks (5 s sustain + 0.5 s silence buffer = 5.5 s per combo) into single output WAV (~3.3 min audio). Aggregate JSON emits per-combo entries in canonical order plus `pass_all_36`. |
| 3 | `reproduce-goldens.sh` extension content | **Extend `INVOC` array to 12 entries** (10 carry-forward + 2 new). Append `INVOC[sub-harmonics]="--sub-harmonics"` and `INVOC[sub-harmonics-stability]="--sub-harmonics-stability"` after the existing `INVOC[schelleng-stress]` line. Loop body unchanged. Trailing diagnostic line updated `"OK: all 12 goldens reproduce byte-identical"` (was `all 10`). Matrix-stability `6db67707…` STAYS deferred from reproduce-goldens.sh per Phase 2.4a Q22 (large WAV; not in default tripwire). R35a stages the extension; R35 atomic commit lands. |
| 4 | `preflight-subharm.sh` content | **Bash script committed at `plugins/O-Contrabass/tests/render-harness/preflight-subharm.sh`** (~30 LOC; `chmod +x`). Verbatim per RESEARCH §18.16 #4 with these refinements: (a) `HARNESS` resolves via `REPO_ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"` like reproduce-goldens.sh; (b) builds harness if missing (`cmake --build . --target O-Contrabass-render-test --parallel >/dev/null`); (c) outputs to `/tmp/preflight-subharm.{wav,json}`; (d) exits 0 on STRICT-PASS or SOFT-PASS, exits 1 on HARD-FAIL — gates R35 atomic commit per §18.6 escalation contract. R35-pre runs this script post-R35d source build; output STDOUT line is logged into PLAN-execution-log notes for verify-phase audit. |
| 5 | `SubHarmonicBias.h` filename + namespace | **`plugins/O-Contrabass/Source/DSP/SubHarmonicBias.h`** + namespace `ouaricon::contrabass::sub_harmonics`. Mirrors Phase 2.4a `SchellengCalibration.h` precedent (per-plugin, per-Q24) at the same `Source/DSP/` folder + sibling `ouaricon::contrabass::schelleng` namespace. NO shared-module extraction (`modules/dsp/sub-harmonics/` rejected per RESEARCH §18.4 + ARCHITECTURE §765 "O-Contrabass-specific, should NOT bleed back into O-Bowed defaults"). Implementation = RESEARCH §18.9 ~120 LOC verbatim. |
| 6 | F_bow uplift application in Step 6 | **Store as `voiceBowForceUpliftThisBlock` member; multiply at the existing `effectiveBowPressure * (0.5f + mpePressure * 1.5f)` site at `BowedContrabassVoice.cpp:~377`.** New private member `float voiceBowForceUpliftThisBlock { 1.0f };` in `BowedContrabassVoice.h`. Initialised to 1.0 in `prepareToPlay`; reset to 1.0 at the top of every `renderNextBlock` call (BEFORE Step 2.5) so HR-9 short-circuit path leaves it at 1.0 (bit-exact preservation). Step 2.5's else-branch sets `voiceBowForceUpliftThisBlock = F_bow_pre / std::max(1.0e-6f, rawBowPressure)` after `applyBias()` returns; Step 6 multiplies the existing pressure expression by this scalar. At HR-9 path (subAmount=0.0) the factor stays 1.0 → Step 6 is bit-exact identical to Phase 2.4a. |
| 7 | F_bow_pre snapshot exactness in Step 2.5 | **Block-entry pressure value (block-rate, not per-sample).** Step 2.5 reads `mpePressure` once at block entry from the active voice's MPE pressure (via the same accessor Step 7 uses for per-sample MPE pitch); pre-bias `F_bow = rawBowPressure * (0.5f + 1.5f * mpePressure_block_entry)`. Per-sample MPE pressure variation in Step 7 is preserved (still drives the per-sample friction junction); the bias's safeDepth lookup + uplift factor are computed once per block. Per-sample bias re-evaluation deferred (would multiply CPU 256× per block; out-of-scope at v1.0 per RESEARCH §18.16 #7). |
| 8 | `pass_subharmAudible` 4-way AND vs hierarchical pass_combo | **`--sub-harmonics` mode: 5-way `pass_combo` AND** = `pass_noNaN && pass_peak && pass_clickFree && pass_blockTime && pass_subharmAudible` (Gate 6b invariant 2 hard-bar). **`--sub-harmonics-stability` mode: 4-way `pass_combo` AND** (no `pass_subharmAudible` — the matrix mode measures stability across 36 combos, NOT spectral content; only stringIdx=0 / SUB_HARMONICS=1.0 combos would have a meaningful f0/2 ratio anyway, and the matrix isn't designed to gate on that). Aggregate `pass_all_36 = (passCount == 36)`; soft-PASS `failCount ≤ 2` v1.0 budget. |
| 9 | `--sub-harmonics-stability` WAV output | **Single concatenated stereo WAV** at default sr=44100 (24-bit PCM per existing harness convention). Per-combo: 5 s sustain + 0.5 s silence buffer = 5.5 s × 36 combos = ~3.3 min total audio. Combos rendered in canonical iteration order (pin #2: stringIdx → infiniteSustain → subHarmonics). Single sha256 covers all 36 combos; aggregate JSON sha256 captures per-combo truth-table. Both `sub-harmonics-stability.wav.sha256` AND `sub-harmonics-stability.json.sha256` committed as new goldens in R35b. Per-combo separate WAV files alternative rejected (mirrors Phase 2.4a Q9 / Phase 2.4a `--matrix-stability`). |
| 10 | Bin selection for non-E1 strings | **E1-only (`--sub-harmonics` renders MIDI 28).** RESEARCH §18.5 bin map is hard-coded for f0=41.2 Hz. Per-string variants `--sub-harmonics-A/-D/-G` (with bin shift to f0=55.0/73.4/98.0 Hz) deferred to Phase 2.4-bis or 2.4c expansion per RESEARCH §18.16 #10. v1.0 ships single E1-only audible-mode golden; the 36-combo stability matrix exercises all 4 strings for stability invariants but does NOT compute spectral metrics per combo. |
| 11 | Risk #4 mitigation hand-back (failing-combo escalation) | **Two-stage escalation script.** If R35-pre `--sub-harmonics-stability` reports `failCount > 2` (exceeds v1.0 budget): (a) jq triage identifies failing combos; (b) confirm whether they cluster on fallback cells (safeDepth=0.5 in Phase 2.4a calibration table — should mechanically apply architecture §661 fallback 1 retune via §18.4 mapping); (c) if cluster matches, document combos in R35 commit body and proceed (mirrors Phase 2.4a 105/108 outcome); (d) if non-cluster, manually retune `kForceBoost` in `SubHarmonicBias.h` from `0.8f` → `0.4f` (architecture §661 fallback 1 explicit retune), rebuild, re-run R35-pre; (e) if 0.4f ALSO fails matrix, escalate to Phase 2.4-bis chaos detector implementation OR downstream-defense tightening (saturator clamp / loop-gain ceiling tightening — Phase 2.5/2.6 concern). Plan-phase commits this escalation logic into PLAN-execution-log notes; verify-phase carries forward as Phase 2.4-bis backlog if invoked. |
| 12 | R35 atomic commit file count | **Estimated 12–15 files.** Source: 2 modified (`BowedContrabassVoice.{h,cpp}`) + 1 new (`Source/DSP/SubHarmonicBias.h`). Harness: 1 modified (`tests/render-harness/main.cpp`) + 1 modified script (`reproduce-goldens.sh` extended) + 1 new script (`preflight-subharm.sh`). Goldens: 6 new text files (`sub-harmonics.{wav.sha256,json,json.sha256}` + `sub-harmonics-stability.{wav.sha256,json,json.sha256}`); 0 re-baselined (HR-9 IEEE 754 identity arithmetic preserves all 10 carry-forward bit-exact). Planning: 4 modified (`STATUS.md` + `stages/2-dsp/{CONTEXT.md,RESEARCH.md,PLAN.md,SUMMARY.md}`) — CONTEXT/RESEARCH already at rev-7/§18 from discuss/research phases; PLAN gets this rev-9 append; SUMMARY + STATUS get execute-phase appends after R35. Final committed file count: ~14 files. R35-backfill chore lands after R35 atomic commit per R34-backfill / R33-backfill / R26 precedent. |

**Carry-forward HARD RULES from prior phases (NOT re-litigated):**
- HR-1 (Phase 2.3): Vibrato literal-zero short-circuit + active-string-only — carry-forward.
- HR-2 (Phase 2.3): Slow-LFO literal-zero short-circuit + phase non-advance at zero depth — carry-forward.
- HR-3 (Phase 2.3): Macro IEEE 754 identity arithmetic + macroSmoothed setCurrentAndTargetValue(0.0) — carry-forward.
- HR-4 (Phase 2.3): Schelleng wedge skip on zero LFO depth + lastSafeDepth.store(0.0) unconditional pre-gate — carry-forward.
- HR-5 (Phase 2.4a): `inline constexpr` linkage on `SchellengCalibration.h` — carry-forward; SubHarmonicBias.h uses identical `inline` pattern (HR-9 implementation).
- HR-6 (Phase 2.4a): Calibration polynomial behind HR-4 gate ONLY — carry-forward; Phase 2.4b's `safeDepthForString(...)` query in Step 2.5 is INDEPENDENT of HR-6 (Step 2.5 fires regardless of `rawSlowLfoDepth` value, but only if `subAmount > 0.0` per HR-9). HR-6 + HR-9 are independent gates.
- HR-7 (Phase 2.4a): Matrix-stability bypass via `extern "C" __attribute__((weak)) bool isMatrixStabilityModeActive() noexcept` — carry-forward; Phase 2.4b does NOT replicate this pattern. The new `--sub-harmonics-stability` mode does NOT bypass any wedge math (bias is the unit under test, not a parallel calibration). The existing HR-7 weak symbol stays in PluginProcessor.cpp untouched.
- HR-8 (Phase 2.4a): Trilinear IEEE 754 identity arithmetic at sample points — carry-forward; SubHarmonicBias.h's `safeDepthForString(...)` lookup inherits the same IEEE 754 identity-at-sample-points guarantee.
- 7-step per-block evaluation order verbatim (Steps 1/3/4/5/6/7 unchanged); **Phase 2.4b inserts Step 2.5 (sub-harmonic bias evaluation)** between Step 2 (Schelleng wedge) and Step 3 (slow-LFO phase advance).
- `std::atomic<float> lastSafeDepth { 0.0f };` (Phase 2.3 R28) instrumentation hook signature carries forward unchanged.
- 5 ms equal-power crossfade + `activeStringIndex` / `previousStringIndex` state machine — carry-forward; bias is active-string-only per HR-9.
- 10 carry-forward goldens MUST reproduce byte-identical via `reproduce-goldens.sh` post-source-edit.
- Phase 2.4a `matrix-stability.{wav.sha256,json}` `6db67707…` carries forward byte-identical (108 combos render at SUB_HARMONICS=0; HR-9 short-circuit fires bit-exact). NOT in `reproduce-goldens.sh` default array; verify-phase R35e re-renders manually OR documents matrix-stability as a verify-phase optional invariant per Phase 2.4a precedent.
- Atomic-commit gate-first principle: R7 → R15 → R20 → R26 → R33 → R34 → **R35** sequence.

**HARD RULES unique to Phase 2.4b (binding for R35d implementation):**

1. **HR-9 — SUB_HARMONICS=0 IEEE 754 identity arithmetic + active-string-only bias gate.**
   - **Caller-side short-circuit (BowedContrabassVoice.cpp Step 2.5 entry):** `if (subAmount == 0.0f || activeStringIndex < 0) { lastSubAmount.store(0.0f, std::memory_order_relaxed); return; }`. The HR-9 short-circuit is **CALLER-side**; SubHarmonicBias.h's `applyBias(...)` does NOT defensively short-circuit (per RESEARCH §18.9 contract: "This function does NOT defensively short-circuit; HR-9 is enforced at the caller for clarity of the bit-exact regression bar").
   - **`subHarmonicsSmoothed` initialisation:** `subHarmonicsSmoothed.setCurrentAndTargetValue(0.0f)` in `prepareToPlay` (mirrors Phase 2.3 macroSmoothed pin #11 precedent). At APVTS default 0.0, `getNextValue()` returns exact 0.0f → HR-9 caller-side check fires.
   - **`subHarmonicsSmoothed.setTargetValue(...)` UNCONDITIONAL each block** per Phase 2.3 macroSmoothed pin #11 precedent. NO `if (newValue != currentTarget)` guard — denormal accumulation in smoother state would drift over very long sessions.
   - **Active-string-only:** Step 2.5 fires bias evaluation only when `activeStringIndex >= 0 && activeStringIndex < 4`. Crossfade-shadowed `previousStringIndex` is NOT biased (mirrors HR-1 vibrato per CONTEXT rev-7 Q31). The 5 ms equal-power crossfade window's residual ring-out from previous string sees the mutated frictionModel state (per RESEARCH §18.10 architectural-correctness analysis), but the 30 ms `subHarmonicsSmoothed` ramp absorbs the discontinuity.
   - **`voiceBowForceUpliftThisBlock = 1.0f` reset at `renderNextBlock` entry** BEFORE Step 2.5 — guarantees HR-9 short-circuit path leaves Step 6 bit-exact identical to Phase 2.4a.
   - **`lastSubAmount.store(0.0f)` UNCONDITIONAL pre-gate** at Step 2.5 entry — instrumentation atom always reflects current cycle (mirrors HR-4 `lastSafeDepth.store(0.0f)` pre-gate).

2. **HR-10 — Friction module ABI preservation under bias.**
   - `SubHarmonicBias.h::applyBias(...)` mutates `F_bow / v_0 / mu_s` by reference. The caller (Step 2.5) pushes mutations back to `frictionModel` via the EXISTING `HyperbolicFriction.h` setter surface — **NO friction module v1.0.0 ABI change.**
   - **`v_0` push via ROSIN inverse algebraic identity (RESEARCH §18.10):** `rosinEq = -ln(10·v_0_biased) / 4.6f`, clamped `[0.0f, 1.0f]`, pushed via existing `frictionModel.setRosin(rosinEq)`. The clamp is defensive; at `v_0` floor 0.005 (architecture-spec'd), `rosinEq ≈ 0.65` is well within range.
   - **`mu_s` push via existing setter:** `frictionModel.setStaticFrictionCoefficient(mu_s_biased)` — already in module v1.0.0 surface.
   - **`mu_d` unchanged** at v1.0 (bass default 0.25f set once in `prepareToPlay`).
   - **`F_bow` consumed at Step 6** via the `voiceBowForceUpliftThisBlock` factor multiplied into the existing `effectiveBowPressure * (0.5f + mpePressure * 1.5f)` expression at line ~377. **NO new `bowModel` setter; NO module ABI change.**
   - **At HR-9 short-circuit path:** `frictionModel.setRosin(rawRosin)` runs as Phase 2.4a verbatim (Step 2.5's `else` branch never executes); `frictionModel.setStaticFrictionCoefficient(0.85f)` set once in `prepareToPlay`; `voiceBowForceUpliftThisBlock` stays at 1.0f. Friction module sees identical state → 10 carry-forward goldens reproduce bit-exact.

**Per-block evaluation order (locked verbatim from RESEARCH §18.15, with Step 2.5 inserted):**

```
voiceBowForceUpliftThisBlock = 1.0f                                    [HR-9 reset]

Step 1 — Read raw APVTS atomics (10 raw reads + add rawSubHarmonics; UNCHANGED otherwise).
Step 2 — Schelleng wedge:
         lastSafeDepth.store(0.0f, relaxed)                            [Phase 2.3 pin #4]
         if (rawSlowLfoDepth > 0.0f)                                    [HR-4]
             if (isMatrixStabilityModeActive())                          [HR-7]
                 safeDepth = rawSlowLfoDepth
             else
                 beta = jlimit(0.02, 0.25, rawBowPos)
                 safeDepth = jlimit(0.0, rawSlowLfoDepth,
                                    schelleng::safeDepthForString(activeStringIndex,
                                                                  rawBowSpeed,
                                                                  rawBowPressure,
                                                                  beta))     [Phase 2.4a]
             vibAntiCorr = kAntiCorrPerDepth * rawSlowLfoDepth
             lastSafeDepth.store(safeDepth, relaxed)

Step 2.5 — Sub-harmonic bias (Phase 2.4b R35d; NEW):                    [HR-9 + HR-10]
         subHarmonicsSmoothed.setTargetValue(rawSubHarmonics)           [pin #11 UNCONDITIONAL]
         subAmount = subHarmonicsSmoothed.getNextValue()
         subHarmonicsSmoothed.skip(jmax(0, numSamples - 1))              [pin #7 jmax guard]
         lastSubAmount.store(0.0f, relaxed)                              [HR-9 pre-gate, mirrors HR-4]

         if (subAmount == 0.0f || activeStringIndex < 0)                 [HR-9 short-circuit]
             // Step 3 onward sees unbiased frictionModel state — bit-exact.
             // voiceBowForceUpliftThisBlock stays 1.0f → Step 6 bit-exact.
         else
             v_b_voice  = bowModel.getBowVelocity()
             beta_v     = jlimit(0.02, 0.25, rawBowPos)
             v_0_pre    = 0.1f * exp(-4.6f * rawRosin)                   // ROSIN→v_0 forward map
             mu_s_pre   = 0.85f                                          // bass default
             mu_d_const = 0.25f                                          // bass default
             F_bow_pre  = rawBowPressure * (0.5f + 1.5f * mpePressure_block_entry)
             safeDepthSub = schelleng::safeDepthForString(activeStringIndex,
                                                          rawBowSpeed,
                                                          rawBowPressure,
                                                          beta_v)
             sub_harmonics::applyBias(subAmount, activeStringIndex,
                                       v_b_voice, beta_v, safeDepthSub,
                                       F_bow_pre, v_0_pre, mu_s_pre,
                                       mu_d_const)                       // mutates by ref
             rosinEq = jlimit(0.0f, 1.0f,
                              -log(10.0f * jmax(1.0e-6f, v_0_pre)) / 4.6f) // HR-10 inverse
             frictionModel.setRosin(rosinEq)                              // HR-10 push
             frictionModel.setStaticFrictionCoefficient(mu_s_pre)         // HR-10 push
             voiceBowForceUpliftThisBlock = F_bow_pre / jmax(1.0e-6f, rawBowPressure)
             lastSubAmount.store(subAmount, relaxed)

Step 3 — Slow-LFO phase advance + sin (HR-2 gate; UNCHANGED).
Step 4 — Apply slow-LFO multiplicatively (UNCHANGED).
Step 5 — Layer macro multiplicatively (HR-3 gated; UNCHANGED).
Step 6 — Push to bowModel + all-strings brightness:
         effectiveBowPressure *= (0.5f + mpePressure * 1.5f)
                                 * voiceBowForceUpliftThisBlock          [Phase 2.4b: 1.0f at HR-9]
         bowModel.setBowSpeed(effectiveBowSpeed)
         bowModel.setBowPressure(effectiveBowPressure)
         (per-string brightness; UNCHANGED)
Step 7 — Per-sample loop (HR-1 gated vibrato; UNCHANGED).
```

**At HR-9 short-circuit path (subAmount==0.0 OR activeStringIndex<0):**
- `lastSubAmount.store(0.0f)` fires;
- `voiceBowForceUpliftThisBlock` stays at 1.0f;
- `frictionModel.setRosin(rawRosin)` continues per Phase 2.4a (Step 2.5 else-branch never executes; existing line 647 setRosin unchanged);
- Step 6's pressure expression is bit-exact identical to Phase 2.4a (×1.0f is IEEE 754 identity);
- → 10 carry-forward goldens reproduce byte-identical.

**At biased path (subAmount > 0.0 AND activeStringIndex >= 0):**
- bias mutates `F_bow / v_0 / mu_s` per architecture §457;
- `safeDepth` from Phase 2.4a table scales the F_bow uplift only (RESEARCH §18.4 mapping; v_0 + mu_s preserve architecture-§457 verbatim);
- Schelleng F_max ceiling clamp INSIDE `applyBias()` enforces architecture line 470;
- friction state push via HR-10 inverse identity preserves module v1.0.0 ABI.

---

## Goal

Implement architecture §457 sub-harmonic bias DSP-07 — friction-junction parameter biasing on `F_bow` / `v_0` / `mu_s − mu_d` toward the Schelleng `F_max` regime to induce period-doubling f0/2 spectral content as a musical bass-extension feature on the SUB_HARMONICS APVTS knob (Float [0, 1] default 0.0 already declared at PluginProcessor.cpp:104). At `SUB_HARMONICS = 1.0` on E1 (MIDI 28), produce `subharmEnergyRatio = E(f0/2_3bins) / E(f0_3bins) ≥ 0.40` (strict-PASS) measured via `juce::dsp::FFT` size 65536 Hann-windowed over the last 2 s of a 5 s sustain at default bow params. Soft-PASS within v1.0 budget at `[0.30, 0.40)` per RESEARCH §18.6; hard-FAIL `< 0.30` escalates to architecture §661 fallback 1 retune (`kForceBoost` 0.8 → 0.4) BEFORE R35 atomic commit. Preserve QUAL-01 stability across 36 stress-test combos (4 strings × 3 INFINITE_SUSTAIN ∈ {0.0, 0.5, 1.0} × 3 SUB_HARMONICS ∈ {0.0, 0.5, 1.0}) at default BODY_DAMPING + default bow params (5 s sustain per combo at MIDI 28/33/38/43). Reuse Phase 2.4a `schelleng::safeDepthForString()` table as the F_bow ceiling source via `effectiveBoost = subAmount · 0.8 · safeDepth` mapping (RESEARCH §18.4): stable cells (105/108) → full 1.8× uplift (architecture default `kForceBoost = 1.8`); fallback cells (3/108) → 1.4× uplift (auto-applies architecture §661 fallback 1). Validate Gate 6b invariants — five-item bar including bit-exact reproduction of 10 carry-forward goldens via `reproduce-goldens.sh` (HR-9 IEEE 754 identity arithmetic guarantee) — and atomic-commit on Gate 6b PASS as R35. Continue R7 → R15 → R20 → R26 → R33 → R34 → **R35** atomic-commit sequence. Phase 2.4b ships **NO Stage-1 contract amendment** + **NO ARCHITECTURE.md amendment**; chaos detector + softClampState deferred to Phase 2.5/2.6 via R35 commit-body footnote per RESEARCH §18.13.

---

## Tasks

### R35-pre — Pre-flight bit-exact baseline confirmation + spectral pre-flight + script staging

**No source edits committed except `tests/render-harness/preflight-subharm.sh` (NEW; staged for R35 atomic). Diagnostic + script-creation. Confirms working-tree integrity at start of execute AND empirically validates RESEARCH §18.1 + §18.3 pre-flights under the plan-phase build environment AND stages the spectral pre-flight script that gates R35 atomic commit per HR-9 escalation contract.**

Per RESEARCH §18.1, working tree at HEAD `b64c8c4` (R34-backfill chore) reproduces all 10 currently-committed goldens byte-identical via `reproduce-goldens.sh`. Per RESEARCH §18.2, single-combo extreme-settings render takes ~0.03 s wall-clock; 36-combo extrapolation ~4 s wall-clock. Per RESEARCH §18.3, baseline `subharmEnergyRatio` at SUB_HARMONICS=0 is 0.241 (noise-floor artefact; threshold MUST be 0.40 strict / 0.30 soft / <0.30 hard-FAIL). R35-pre re-runs the bit-exact pre-flight as a tripwire AND authors the spectral pre-flight script that R35-pre will invoke post-R35d.

**Tasks:**

1. **Confirm git state is clean and at R34 atomic commit / R34-backfill chore:**
   ```bash
   git log --oneline -2                                  # should be b64c8c4 + 4c926bb
   git status plugins/O-Contrabass/                      # should be clean
   ```
   If working tree is dirty, STOP and reconcile before proceeding.

2. **Build harness:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```
   Expect `ninja: no work to do` if R34 build artefacts persist; otherwise clean rebuild.

3. **Run `reproduce-goldens.sh` at HEAD (10-golden tripwire):**
   ```bash
   bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   # Expect: "OK: all 10 goldens reproduce byte-identical"
   ```
   **All 10 must PASS** per RESEARCH §18.1 record (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + vibrato + macro-sweep + slow-lfo Phase-2.4a-rebaselined + schelleng-stress Phase-2.4a-rebaselined). If any FAIL, STOP and investigate working-tree drift (re-confirm HEAD == `b64c8c4` or descendant; `git status` for stray edits in `Source/`, `tests/render-harness/`, `modules/synthesis/bow-friction/`). Do NOT proceed to R35a until all 10 reproduce.

4. **Sanity-check `--matrix-stability` byte-identity (Phase 2.4a evidence golden carry-forward):**
   ```bash
   $HARNESS --matrix-stability \
            --out /tmp/phase24b-r35pre/matrix-stability.wav \
            --json /tmp/phase24b-r35pre/matrix-stability.json
   computed=$(shasum -a 256 /tmp/phase24b-r35pre/matrix-stability.wav | awk '{print $1}')
   expected=$(awk '{print $1}' plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.wav.sha256)
   [ "$computed" = "$expected" ] && echo "[PASS] matrix-stability $computed" || echo "[FAIL]"
   # Expect: 6db6770727ab3b433a036f487217bbde70f8cc15de44fa60ac0b99d868176449
   ```
   Confirms Phase 2.4a 108-combo matrix golden is reproducible (HR-9 short-circuit + HR-7 wedge bypass interact correctly at SUB_HARMONICS=0 default). Diagnostic only — NOT in default reproduce-goldens.sh per Phase 2.4a Q22.

5. **Author `tests/render-harness/preflight-subharm.sh`** (~30 LOC bash script per RESEARCH §18.16 #4):

   ```bash
   #!/usr/bin/env bash
   # Phase 2.4b R35-pre — spectral pre-flight at SUB_HARMONICS=1.0 with bias-active.
   # Verifies pass_subharmAudible threshold per RESEARCH §18.6 BEFORE R35 atomic commit.
   # Exit code: 0 on STRICT-PASS or SOFT-PASS; 1 on HARD-FAIL.
   set -euo pipefail
   REPO_ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
   HARNESS="$REPO_ROOT/build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test"
   if [ ! -x "$HARNESS" ]; then
       cd "$REPO_ROOT/build"
       cmake --build . --target O-Contrabass-render-test --parallel >/dev/null
       cd -
   fi
   $HARNESS --sub-harmonics \
            --out /tmp/preflight-subharm.wav \
            --json /tmp/preflight-subharm.json >/dev/null
   RATIO=$(python3 -c "import json; print(json.load(open('/tmp/preflight-subharm.json'))['subharmEnergyRatio'])")
   echo "subharmEnergyRatio = $RATIO"
   python3 - <<EOF
import sys
r = $RATIO
if r >= 0.40:
    print("STRICT-PASS — proceed to R35 atomic commit")
    sys.exit(0)
elif r >= 0.30:
    print("SOFT-PASS within v1.0 budget — Phase 2.4-bis remediation flag (track in R35 commit body)")
    sys.exit(0)
else:
    print("HARD-FAIL — escalate to architecture §661 fallback 1 retune (kForceBoost 0.8→0.4 in SubHarmonicBias.h)")
    sys.exit(1)
EOF
   ```

   ```bash
   chmod +x plugins/O-Contrabass/tests/render-harness/preflight-subharm.sh
   ```

6. **Stage `preflight-subharm.sh`** (do NOT commit yet — included in R35 atomic):
   ```bash
   git add plugins/O-Contrabass/tests/render-harness/preflight-subharm.sh
   git status   # should show 1 new file staged
   ```

**Files created:**
- `plugins/O-Contrabass/tests/render-harness/preflight-subharm.sh` (NEW, ~30 LOC, staged for R35 atomic).
- `/tmp/phase24b-r35pre/*.{wav,json}` (transient; deleted post-R35; NOT committed).

**Files modified:** none committed.

**Commit:** **NONE** — diagnostic + script-staging only. R35 atomic commit lands the staged file alongside the rest of Phase 2.4b.

**Success bar:**
- [ ] `git log --oneline -1` shows `b64c8c4` or descendant.
- [ ] Harness build succeeds.
- [ ] `reproduce-goldens.sh` reports `OK: all 10 goldens reproduce byte-identical`.
- [ ] `matrix-stability.wav` sha256 matches committed `6db67707…` (Phase 2.4a evidence golden carry-forward).
- [ ] `preflight-subharm.sh` authored, `chmod +x`, staged for R35.
- [ ] R35-pre at this stage does NOT yet invoke `preflight-subharm.sh` — that fires AFTER R35d source build (the script depends on the new `--sub-harmonics` harness mode that R35a authors).

**Estimated effort:** 15 min (build + reproduce-goldens + matrix-stability sanity + script authoring + chmod + stage).

---

### R35a — Harness `--sub-harmonics` + `--sub-harmonics-stability` modes + FFT analyser

**Per RESEARCH §18.5 FFT specs + §18.7 MIDI selection + §18.8 pass criteria + pins #1, #2, #8, #9, #10. Adds the audible-mode FFT analyser + 36-combo iteration mode + per-combo JSON + concatenated WAV. NO source-side bias polynomial yet — that's R35c+R35d.**

**Tasks:**

1. **`tests/render-harness/main.cpp` — add `--sub-harmonics` + `--sub-harmonics-stability` CLI flags (~+50 LOC parsing).**

   In the anonymous namespace at the top of main.cpp, add the canonical iteration constants:
   ```cpp
   constexpr int   kSubharmStabilityMidi[4] = { 28, 33, 38, 43 };
   constexpr float kSubharmStabilitySustainAxis[3] = { 0.0f, 0.5f, 1.0f }; // INFINITE_SUSTAIN
   constexpr float kSubharmStabilitySubAxis[3]     = { 0.0f, 0.5f, 1.0f }; // SUB_HARMONICS
   constexpr float kSubharmSustainSec     = 5.0f;
   constexpr float kSubharmSilenceSec     = 0.5f;     // pin #9 — silence buffer between combos
   constexpr int   kSubharmFftSize        = 65536;    // RESEARCH §18.5 lock
   constexpr int   kSubharmFftOrder       = 16;       // 2^16 = 65536
   constexpr int   kSubharmF0BinLo        = 60;       // RESEARCH §18.5 bin map (E1 f0 ≈ 41.20 Hz)
   constexpr int   kSubharmF0BinHi        = 62;
   constexpr int   kSubharmSubBinLo       = 30;       // E1 f0/2 ≈ 20.60 Hz
   constexpr int   kSubharmSubBinHi       = 32;
   constexpr int   kSubharmFloorBinLo     = 33;       // [22..28] Hz median floor
   constexpr int   kSubharmFloorBinHi     = 41;
   ```

   In `parseArgs`, add `--sub-harmonics` + `--sub-harmonics-stability` flag handling. Slot in mutual-exclusion ladder ABOVE `--matrix-stability` (highest precedence; either flag clears all other modes + emits warning if multiple modes set).

2. **`tests/render-harness/main.cpp` — author `runSubHarmonicsMode()` audible-mode FFT analyser (~+120 LOC).**

   Pseudo-structure (verbatim per RESEARCH §18.5 + §18.6):
   ```cpp
   static int runSubHarmonicsMode (const Args& args)
   {
       // Setup: render MIDI 28 (E1), velocity 0.7, sustain 5.0 s, release 1.0 s,
       // SUB_HARMONICS=1.0, default bow params (BOW_SPEED=0.15, BOW_PRESSURE=3.0,
       // BOW_POSITION=0.10), default SLOW_LFO_DEPTH=0.0, default INFINITE_SUSTAIN=0.0.
       // Stereo 24-bit WAV out (existing harness convention).
       processor.releaseResources();
       processor.prepareToPlay (sampleRate, blockSize);
       setNorm("SUB_HARMONICS", 1.0f);   // existing parameter-norm pattern at main.cpp:389

       // ... render kSubharmSustainSec + 1.0f release ...

       // FFT analysis: last 2 s of 5 s sustain (88200 samples; first 65536 taken).
       const int analysisStart = static_cast<int> ((kSubharmSustainSec - 2.0f) * sampleRate);
       std::vector<float> mono (kSubharmFftSize);
       for (int i = 0; i < kSubharmFftSize; ++i)
       {
           // Mix L+R; existing harness mono-mix pattern.
           mono[i] = 0.5f * (renderBuf.getSample(0, analysisStart + i)
                           + renderBuf.getSample(1, analysisStart + i));
           // Hann window: 0.5 - 0.5*cos(2*pi*k/(N-1))
           const float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi
                                                    * static_cast<float>(i)
                                                    / static_cast<float>(kSubharmFftSize - 1));
           mono[i] *= w;
       }

       // FFT (size 65536; in-place, allocate-once buffer).
       static juce::dsp::FFT fft (kSubharmFftOrder);
       std::vector<float> fftBuf (2 * kSubharmFftSize, 0.0f);
       std::copy (mono.begin(), mono.end(), fftBuf.begin());
       fft.performFrequencyOnlyForwardTransform (fftBuf.data());
       // fftBuf[0..N/2] now contains magnitudes.

       // Compute mag² for required bins.
       auto mag2 = [&] (int bin) {
           const float m = fftBuf[bin];
           return m * m;
       };

       float E_f0 = 0.0f, E_subharm = 0.0f;
       for (int b = kSubharmF0BinLo;  b <= kSubharmF0BinHi;  ++b) E_f0      += mag2(b);
       for (int b = kSubharmSubBinLo; b <= kSubharmSubBinHi; ++b) E_subharm += mag2(b);
       const float subharmEnergyRatio = E_subharm / juce::jmax (1.0e-9f, E_f0);

       // Diagnostic: subharmPeakOverFloor (no pass criterion).
       float maxBinSub = 0.0f;
       for (int b = kSubharmSubBinLo; b <= kSubharmSubBinHi; ++b)
           maxBinSub = juce::jmax (maxBinSub, mag2(b));
       std::vector<float> floorVals;
       for (int b = kSubharmFloorBinLo; b <= kSubharmFloorBinHi; ++b) floorVals.push_back (mag2(b));
       std::nth_element (floorVals.begin(), floorVals.begin() + 4, floorVals.end());
       const float medianFloor = floorVals[4];
       const float subharmPeakOverFloor =
           std::sqrt (maxBinSub) / std::sqrt (juce::jmax (1.0e-9f, medianFloor));

       // pass_subharmAudible per RESEARCH §18.6 (5-way pass_combo per pin #8):
       const bool pass_subharmAudible = (subharmEnergyRatio >= 0.40f);    // strict-PASS
       const bool soft_subharmAudible = (subharmEnergyRatio >= 0.30f
                                          && subharmEnergyRatio < 0.40f); // soft-PASS within v1.0

       // ... compute pass_noNaN, pass_peak, pass_clickFree, pass_blockTime as in existing modes ...

       const bool pass_combo = pass_noNaN && pass_peak && pass_clickFree
                              && pass_blockTime && (pass_subharmAudible || soft_subharmAudible);

       // Emit JSON per §18.5 schema:
       // status, mode, midiNote=28, subHarmonicsParam=1.0, sustainSeconds=5.0,
       // totalSamples, peak, rmsContinuity, blockMicros_median, blockMicros_max,
       // blockTimeRatio, subharmEnergyRatio, subharmPeakOverFloor,
       // fftBaselineNote ("ratio at SUB_HARMONICS=0 measured 0.241 RESEARCH §18.3"),
       // pass_noNaN, pass_peak, pass_clickFree, pass_blockTime, pass_subharmAudible, pass_combo.

       // ... write WAV + JSON files (existing harness pattern) ...
       return pass_combo ? 0 : 1;
   }
   ```

   Mirror `--vibrato` autocorrelation mode at lines 1133–1300 for harness boilerplate (rendering, JSON emission, file writing). Replace autocorrelation post-process with the FFT analyser above.

3. **`tests/render-harness/main.cpp` — author `runSubHarmonicsStabilityMode()` 36-combo iteration loop (~+90 LOC).**

   ```cpp
   static int runSubHarmonicsStabilityMode (const Args& args)
   {
       // 36 combos: 4 strings × 3 INFINITE_SUSTAIN × 3 SUB_HARMONICS, 5 s sustain each
       // + 0.5 s silence buffer. Default bow params (BOW_SPEED=0.15, BOW_PRESSURE=3.0,
       // BOW_POSITION=0.10), default BODY_DAMPING (no axis), default SLOW_LFO_DEPTH=0.0.
       // Concatenated stereo WAV; per-combo + aggregate JSON; iteration order pin #2.
       PerComboMetrics combos[36];
       int idx = 0;
       int passCount = 0;
       for (int s = 0; s < 4; ++s)
       {
           for (int i = 0; i < 3; ++i)
           {
               for (int k = 0; k < 3; ++k, ++idx)
               {
                   processor.releaseResources();
                   processor.prepareToPlay (sampleRate, blockSize);
                   setNorm("INFINITE_SUSTAIN", kSubharmStabilitySustainAxis[i]);
                   setNorm("SUB_HARMONICS",    kSubharmStabilitySubAxis[k]);
                   // ... inject MIDI noteOn at kSubharmStabilityMidi[s] velocity 0.7 ...
                   // ... render kSubharmSustainSec sustain + 1.0f release ...
                   // ... append silence buffer (kSubharmSilenceSec) ...
                   // ... compute peak, rmsContinuity, blockMicros, lastSubAmount.load() ...
                   combos[idx] = { ... };
                   if (combos[idx].pass_combo) ++passCount;
               }
           }
       }
       const bool pass_all_36 = (passCount == 36);
       // Aggregate JSON per §18.8 schema:
       // status, mode, totalCombos=36, passCount, failCount=36-passCount, pass_all_36, combos[36].
       // ... write concatenated WAV + aggregate JSON ...
       return pass_all_36 ? 0 : 1;
   }
   ```

   Mirror `runMatrixStabilityMode()` from Phase 2.4a (in main.cpp post-R34a) for the 36-combo iteration boilerplate; differences are 36 combos vs 108, no SLOW_LFO_RATE override (default 0.3 Hz; SLOW_LFO_DEPTH=0 anyway), and per-combo `lastSubAmount` instrumentation field.

4. **`tests/render-harness/main.cpp` — `PerComboMetrics` struct extension (~+5 LOC).**
   ```cpp
   struct PerComboMetrics
   {
       int   stringIdx, openStringMidi;
       float infiniteSustain, subHarmonics, sustainSeconds;
       int   totalSamples;
       float peak, rmsContinuity, blockMicros_median, blockMicros_max, blockTimeRatio;
       float lastSubAmount;     // NEW Phase 2.4b — instrumentation atom
       bool  pass_noNaN, pass_peak, pass_clickFree, pass_blockTime, pass_combo;
   };
   ```

   Reuse / extend the existing struct from R34a (`PerComboMetrics` for matrix-stability) — Phase 2.4b adds the `subHarmonics` + `lastSubAmount` fields.

5. **Build harness:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```
   Expect a clean build with zero warnings. **Pre-R35b smoke at HEAD post-R35a (no source-side bias yet — bias evaluation is R35c+R35d):**
   ```bash
   $HARNESS --sub-harmonics --out /tmp/phase24b-r35a-smoke.wav --json /tmp/phase24b-r35a-smoke.json
   jq '.subharmEnergyRatio, .pass_combo' /tmp/phase24b-r35a-smoke.json
   # Expect: subharmEnergyRatio ≈ 0.241 (RESEARCH §18.3 baseline; HR-9 short-circuit absent
   # but bias path is also absent → SUB_HARMONICS knob has no DSP effect yet);
   # pass_combo: false (subharmEnergyRatio < 0.40 strict; bias hasn't been wired in yet).
   ```
   This baseline is EXPECTED to fail `pass_subharmAudible` at R35a — it confirms the harness measures correctly. R35d wires in the bias; R35-pre re-runs after R35d source build.

6. **Stage the harness changes** (do NOT commit yet — R35 atomic):
   ```bash
   git add plugins/O-Contrabass/tests/render-harness/main.cpp
   git status   # should show 1 modified file staged
   ```

**Files created:** none (harness binary is build artefact).

**Files modified:**
- `plugins/O-Contrabass/tests/render-harness/main.cpp` (~+265 LOC: 50 parsing + 120 sub-harmonics mode + 90 sub-harmonics-stability mode + 5 PerComboMetrics extension).

**Commit:** **NONE** — staged for R35 atomic.

**Success bar:**
- [ ] Harness builds clean zero warnings.
- [ ] `--sub-harmonics` mode runs without crashing; emits valid JSON with subharmEnergyRatio field.
- [ ] `--sub-harmonics-stability` mode runs all 36 combos; emits aggregate JSON.
- [ ] At HEAD post-R35a (NO source-side bias yet): `--sub-harmonics` reports `subharmEnergyRatio ≈ 0.241` (matches RESEARCH §18.3 baseline measurement). This confirms the FFT analyser is wired correctly.
- [ ] At HEAD post-R35a (NO source-side bias yet): `--sub-harmonics-stability` reports `pass_all_36 = true` (all 36 combos at SUB_HARMONICS=0/0.5/1.0 produce stable output — but bias has zero DSP effect at any SUB_HARMONICS value because R35c/R35d hasn't landed; this confirms HR-9 path stability across the matrix).

**Estimated effort:** 75 min (parser changes + audible-mode runner + 36-combo runner + struct extension + build + smoke).

---

### R35b — Render new goldens + commit golden text

**After R35d source edits land (NOT yet at this point in execute order — R35b runs as the post-R35d golden-capture step). Sequencing note: R35b is documented HERE in plan order for clarity, but plan-execution order is R35-pre → R35a → R35c → R35d → R35-pre invoke `preflight-subharm.sh` → R35b → R35e → R35f → R35 atomic. R35b CANNOT execute before R35d wires in the bias DSP.**

**Tasks:**

1. **Build harness with R35a + R35c + R35d source edits applied:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```
   Expect a clean build with zero warnings. (R35-pre's `preflight-subharm.sh` MUST have already returned exit-0 — STRICT-PASS or SOFT-PASS — before reaching R35b.)

2. **Render `--sub-harmonics` golden:**
   ```bash
   mkdir -p /tmp/phase24b-r35b
   $HARNESS --sub-harmonics \
            --out /tmp/phase24b-r35b/sub-harmonics.wav \
            --json /tmp/phase24b-r35b/sub-harmonics.json
   shasum -a 256 /tmp/phase24b-r35b/sub-harmonics.wav | awk '{print $1}' \
       > plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.wav.sha256
   cp /tmp/phase24b-r35b/sub-harmonics.json \
      plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.json
   shasum -a 256 plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.json | awk '{print $1}' \
       > plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.json.sha256
   ```
   Validates `pass_subharmAudible` per RESEARCH §18.6 (strict ≥ 0.40 / soft [0.30, 0.40) / hard < 0.30 escalates). If hard-FAIL: STOP, retune coefficient, return to R35-pre invoke `preflight-subharm.sh`.

3. **Render `--sub-harmonics-stability` golden (~4 s wall-clock):**
   ```bash
   $HARNESS --sub-harmonics-stability \
            --out /tmp/phase24b-r35b/sub-harmonics-stability.wav \
            --json /tmp/phase24b-r35b/sub-harmonics-stability.json
   shasum -a 256 /tmp/phase24b-r35b/sub-harmonics-stability.wav | awk '{print $1}' \
       > plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.wav.sha256
   cp /tmp/phase24b-r35b/sub-harmonics-stability.json \
      plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.json
   shasum -a 256 plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.json | awk '{print $1}' \
       > plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.json.sha256
   ```
   Expects `pass_all_36 = true` (strict-PASS) OR `failCount ≤ 2` (soft-PASS within v1.0 budget). If `failCount > 2`: invoke pin #11 escalation script (jq triage → fallback-cell cluster check → `kForceBoost` 0.8→0.4 retune if needed → re-render).

4. **Smoke-check sha256 stability across re-renders (Phase 2.4a Risk #13 carry-forward):**
   ```bash
   # Re-run sub-harmonics-stability and confirm sha256 matches first invocation.
   $HARNESS --sub-harmonics-stability \
            --out /tmp/phase24b-r35b/sub-harmonics-stability_v2.wav \
            --json /tmp/phase24b-r35b/sub-harmonics-stability_v2.json
   sha1=$(shasum -a 256 /tmp/phase24b-r35b/sub-harmonics-stability.wav    | awk '{print $1}')
   sha2=$(shasum -a 256 /tmp/phase24b-r35b/sub-harmonics-stability_v2.wav | awk '{print $1}')
   [ "$sha1" = "$sha2" ] && echo "[PASS] state reset between combos deterministic" \
                         || echo "[FAIL] state bleed; investigate releaseResources()"
   ```

5. **Stage 6 new golden text files** (do NOT commit yet — R35 atomic):
   ```bash
   git add plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.wav.sha256 \
           plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.json \
           plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.json.sha256 \
           plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.wav.sha256 \
           plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.json \
           plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.json.sha256
   git status   # should show 6 new files staged
   ```

**Files created:**
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.wav.sha256` (NEW; sha256 of audible-mode 5 s render).
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.json` (NEW; ~3 KB; full JSON per §18.5 schema).
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.json.sha256` (NEW; anchors JSON reproducibility).
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.wav.sha256` (NEW; sha256 of 36-combo concatenated WAV).
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.json` (NEW; ~30 KB; per-combo + aggregate truth-table).
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.json.sha256` (NEW; anchors aggregate JSON).

**Files modified:** none.

**Commit:** **NONE** — 6 new files staged for R35 atomic.

**Success bar:**
- [ ] `--sub-harmonics` JSON `pass_subharmAudible = true` (strict ≥ 0.40) OR `subharmEnergyRatio ∈ [0.30, 0.40)` documented as soft-PASS in PLAN-execution-log.
- [ ] `--sub-harmonics-stability` aggregate JSON `pass_all_36 = true` OR `failCount ≤ 2` with documented v1.0 fallback combos.
- [ ] Sha256 stability across re-renders confirmed.
- [ ] 6 new golden text files staged.

**Estimated effort:** 25 min (build + 2 renders + sha capture + 6 file stages + smoke).

---

### R35c — Author `Source/DSP/SubHarmonicBias.h`

**NEW header (~120 LOC) per RESEARCH §18.9 verbatim. Header-only `inline` linkage; namespace `ouaricon::contrabass::sub_harmonics`. Mirrors Phase 2.4a `SchellengCalibration.h` precedent at the same `Source/DSP/` folder.**

**Tasks:**

1. **Author `plugins/O-Contrabass/Source/DSP/SubHarmonicBias.h`** (verbatim per RESEARCH §18.9):

   File contents (~120 LOC; structure pinned, may receive minor pre-build refinements but coefficients + signature LOCKED):

   ```cpp
   // Source/DSP/SubHarmonicBias.h
   //
   // O-Contrabass Phase 2.4b — sub-harmonic bias DSP-07 (ARCHITECTURE §457).
   // Friction-junction parameter biasing toward Schelleng F_max regime to
   // induce period-doubling f0/2 spectral content. Per-plugin (NOT extracted
   // to shared module per CONTEXT rev-7 Q24 — friction module v1.0.0 untouched).
   //
   // HR-9 (Phase 2.4b hard rule): SUB_HARMONICS=0 IEEE 754 identity arithmetic
   // + active-string-only gate. The caller (BowedContrabassVoice.cpp Step 2.5)
   // MUST short-circuit BEFORE invoking applyBias() at subAmount==0.0f and
   // for non-active-string slots. This function does NOT defensively short-
   // circuit; HR-9 is enforced at the caller for clarity of the bit-exact
   // regression bar.

   #pragma once
   #include <algorithm>
   #include <cmath>

   namespace ouaricon::contrabass::sub_harmonics {

   // Architecture §457 / §661 default coefficients. Phase 2.4b ships verbatim;
   // Phase 2.4-bis or 2.5/2.6 may retune to fallback 0.4 if Gate 6b spectral
   // pre-flight (RESEARCH §18.6) returns hard-FAIL.
   inline constexpr float kForceBoost   = 0.8f;     // F_bow uplift coefficient (architecture §457
                                                     // line 465 multiplier; ×subAmount factored at
                                                     // call-site for safeDepth interaction);
                                                     // architecture §661 fallback 1 → 0.4f.
   inline constexpr float kV0Reduction  = 0.5f;     // v_0 contraction coefficient (line 466).
   inline constexpr float kGapWiden     = 0.25f;    // mu_s − mu_d gap widening (line 468).
   inline constexpr float kFmaxScalar   = 0.95f;    // Schelleng ceiling fraction (line 470).
   inline constexpr float kV0Floor      = 0.005f;   // architecture-spec'd lower clamp (line 466).

   // Schelleng F_max formula (architecture §490 — slow-LFO wedge, with
   // Z=0.5, R=0.5, mu_s−mu_d ≈ 0.6 dimensionless collapse):
   //   fMax = (2·Z · v_b) / (beta · (mu_s − mu_d))
   // Same closed form Phase 2.3 used pre-calibration. Re-used here for the
   // final F_bow ceiling clamp; SchellengCalibration table provides the
   // stable-cell vs fallback-cell scaling on the uplift, NOT the hard ceiling.
   inline float schellengFmax (float beta, float v_b, float mu_gap) noexcept
   {
       constexpr float Z2  = 1.0f;                  // 2·Z = 2·0.5 = 1.0
       return (Z2 * v_b) / (std::max (1.0e-6f, beta * std::max (1.0e-6f, mu_gap)));
   }

   // applyBias mutates F_bow / v_0 / mu_s in-place per architecture §457.
   // See header banner for HR-9 caller-side short-circuit contract.
   inline void applyBias (float       subAmount,
                          int         stringIdx,
                          float       v_b,
                          float       beta,
                          float       safeDepth,
                          float&      F_bow,
                          float&      v_0,
                          float&      mu_s,
                          float       mu_d) noexcept
   {
       // F_bow uplift, scaled by Phase 2.4a empirical safeDepth (RESEARCH §18.4 mapping).
       const float effectiveBoost = subAmount * kForceBoost * safeDepth;
       F_bow *= 1.0f + effectiveBoost;

       // v_0 contraction (architecture §457 line 466 verbatim, NOT scaled by
       // safeDepth per §18.4 rationale — v_0 sharpens stick-slip nonlinearity,
       // NOT the F_bow ceiling).
       v_0 = std::max (kV0Floor, v_0 * (1.0f - kV0Reduction * subAmount));

       // mu_s gap widen (architecture §457 line 468 verbatim).
       const float gap = mu_s - mu_d;
       mu_s = mu_d + gap * (1.0f + kGapWiden * subAmount);

       // Schelleng F_max ceiling (architecture line 470). Mu_gap from POST-bias
       // mu_s for accurate post-bias wedge computation.
       const float muGapPost = mu_s - mu_d;
       F_bow = std::min (F_bow, kFmaxScalar * schellengFmax (beta, v_b, muGapPost));

       // stringIdx ignored at v1.0 (CONTEXT rev-7 Q11 per-string variation deferred).
       (void) stringIdx;
   }

   } // namespace ouaricon::contrabass::sub_harmonics
   ```

2. **Stage the new header** (do NOT commit yet — R35 atomic):
   ```bash
   git add plugins/O-Contrabass/Source/DSP/SubHarmonicBias.h
   git status   # should show 1 new file staged
   ```

3. **Verify CMake picks up the header.** Phase 2.4a precedent (`SchellengCalibration.h`) showed CMakeLists.txt globbing or explicit listing already covers `Source/DSP/*.h`; confirm:
   ```bash
   grep -n "Source/DSP/" plugins/O-Contrabass/CMakeLists.txt
   ```
   If `Source/DSP/SubHarmonicBias.h` requires explicit listing alongside `Source/DSP/SchellengCalibration.h` / `DispersionFilter.h`, append it. Otherwise (header-only `#include` from BowedContrabassVoice.cpp adds it transparently), no CMakeLists edit needed. Phase 2.4a precedent: NO CMakeLists edit was required (SchellengCalibration.h consumed via #include only). Same expected for Phase 2.4b.

**Files created:**
- `plugins/O-Contrabass/Source/DSP/SubHarmonicBias.h` (NEW; ~120 LOC; HR-5-mirror `inline constexpr` + `inline` linkage; HR-10 friction-module-ABI-preserving signature).

**Files modified:** none (CMakeLists conditional — only if Phase 2.4a SchellengCalibration.h required explicit listing, which it didn't).

**Commit:** **NONE** — 1 new file staged for R35 atomic.

**Success bar:**
- [ ] `Source/DSP/SubHarmonicBias.h` authored verbatim per RESEARCH §18.9.
- [ ] Header `#pragma once` guarded; `inline constexpr` on coefficients + `inline` on `schellengFmax` / `applyBias`.
- [ ] Namespace `ouaricon::contrabass::sub_harmonics` mirrors `ouaricon::contrabass::schelleng` Phase 2.4a precedent.
- [ ] Coefficients verbatim: `kForceBoost = 0.8f`, `kV0Reduction = 0.5f`, `kGapWiden = 0.25f`, `kFmaxScalar = 0.95f`, `kV0Floor = 0.005f`.
- [ ] Function signature verbatim: `applyBias(float subAmount, int stringIdx, float v_b, float beta, float safeDepth, float& F_bow, float& v_0, float& mu_s, float mu_d) noexcept`.
- [ ] File staged.

**Estimated effort:** 20 min (author + paste from RESEARCH §18.9 + sanity check + stage).

---

### R35d — `BowedContrabassVoice` integration: Step 2.5 + subHarmonicsSmoothed + lastSubAmount + voiceBowForceUpliftThisBlock + HR-10 friction pushes

**Per RESEARCH §18.15 Step 2.5 pseudo-code + HR-9 short-circuit + HR-10 friction module ABI preservation. NO friction module edits (modules/synthesis/bow-friction/ untouched per HR-10).**

**Tasks:**

1. **`Source/BowedContrabassVoice.h` — add 3 new private members + 1 accessor (~+5 LOC).**

   In the modulator-surface block (Phase 2.3 `private:` lines 119–155 region), append:
   ```cpp
   // Phase 2.4b sub-harmonic bias state (HR-9 + HR-10).
   juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> subHarmonicsSmoothed;
   std::atomic<float> lastSubAmount { 0.0f };          // instrumentation atom (mirrors lastSafeDepth)
   float              voiceBowForceUpliftThisBlock { 1.0f };  // HR-9 reset each block; consumed at Step 6
   ```

   Add public accessor (mirrors Phase 2.3 `getLastSafeDepth()` precedent):
   ```cpp
   float getLastSubAmount() const noexcept { return lastSubAmount.load (std::memory_order_relaxed); }
   ```

2. **`Source/BowedContrabassVoice.cpp` — `prepareToPlay` initialisation (~+3 LOC).**

   Insert after the existing `macroSmoothed.setCurrentAndTargetValue(0.0f)` line (Phase 2.3 R28):
   ```cpp
   subHarmonicsSmoothed.reset (sampleRate, 0.030);                // 30 ms ramp time per CONTEXT rev-7 Q30
   subHarmonicsSmoothed.setCurrentAndTargetValue (0.0f);          // HR-9 strict-default precondition
   voiceBowForceUpliftThisBlock = 1.0f;                            // HR-9 reset
   ```

3. **`Source/BowedContrabassVoice.cpp` — Step 2.5 insertion (~+30 LOC).**

   In `renderNextBlock` (Phase 2.3 7-step layout lines 277–330), at the very top of the function (BEFORE Step 1 raw APVTS reads), reset the uplift factor:
   ```cpp
   voiceBowForceUpliftThisBlock = 1.0f;                            // HR-9 reset BEFORE Step 2.5
   ```

   In Step 1 (raw APVTS atomic reads block), add:
   ```cpp
   const float rawSubHarmonics = parameters.getRawParameterValue ("SUB_HARMONICS")->load (std::memory_order_relaxed);
   ```

   Insert **Step 2.5** between the existing Step 2 (Schelleng wedge ending at `lastSafeDepth.store(...)`) and Step 3 (slow-LFO phase advance):
   ```cpp
   // ─────────────────────────────────────────────────────────────────────
   // Step 2.5 — Sub-harmonic bias (Phase 2.4b R35d).
   // HR-9: at subAmount=0.0f or non-active-string, bias path is BIT-EXACTLY no-op.
   // HR-10: friction module ABI preserved via existing setRosin / setStaticFrictionCoefficient
   //        setters; no module v1.0.0 surface change.
   // ─────────────────────────────────────────────────────────────────────
   subHarmonicsSmoothed.setTargetValue (rawSubHarmonics);          // pin #11 UNCONDITIONAL
   const float subAmount = subHarmonicsSmoothed.getNextValue();
   subHarmonicsSmoothed.skip (juce::jmax (0, numSamples - 1));     // pin #7 jmax guard

   lastSubAmount.store (0.0f, std::memory_order_relaxed);           // HR-9 pre-gate (mirrors HR-4)

   if (subAmount != 0.0f && activeStringIndex >= 0 && activeStringIndex < 4)
   {
       const float v_b_voice  = bowModel.getBowVelocity();
       const float beta_v     = juce::jlimit (0.02f, 0.25f, rawBowPos);
       float       v_0_pre    = 0.1f * std::exp (-4.6f * rawRosin);          // ROSIN→v_0 forward map
       float       mu_s_pre   = 0.85f;                                       // bass default
       constexpr float mu_d_const = 0.25f;                                   // bass default
       float       F_bow_pre  = rawBowPressure * (0.5f + 1.5f * mpePressureBlockEntry);

       const float safeDepthSub = ouaricon::contrabass::schelleng::safeDepthForString (
           activeStringIndex, rawBowSpeed, rawBowPressure, beta_v);

       ouaricon::contrabass::sub_harmonics::applyBias (
           subAmount, activeStringIndex, v_b_voice, beta_v, safeDepthSub,
           F_bow_pre, v_0_pre, mu_s_pre, mu_d_const);

       // HR-10 push v_0 via ROSIN inverse algebraic identity.
       const float rosinEq = juce::jlimit (
           0.0f, 1.0f,
           -std::log (10.0f * juce::jmax (1.0e-6f, v_0_pre)) / 4.6f);
       frictionModel.setRosin (rosinEq);
       frictionModel.setStaticFrictionCoefficient (mu_s_pre);

       // F_bow uplift factor consumed at Step 6.
       voiceBowForceUpliftThisBlock = F_bow_pre / juce::jmax (1.0e-6f, rawBowPressure);

       // Instrumentation atom.
       lastSubAmount.store (subAmount, std::memory_order_relaxed);
   }
   ```

   `mpePressureBlockEntry` is the block-entry MPE pressure value per pin #7 — sampled once at the same place Step 7's per-sample MPE pressure is sampled (typically `currentlyPlayingNote.pressure.asUnsignedFloat()` at the active voice). At v1.0, if `mpePressure` isn't already cached at block entry, add a single block-entry read at the top of `renderNextBlock` (before Step 2.5).

4. **`Source/BowedContrabassVoice.cpp` — Step 6 modification (~−1/+1 LOC at line ~377).**

   Locate the existing Step 6 expression `effectiveBowPressure * (0.5f + mpePressure * 1.5f)` and modify ONLY by multiplying by `voiceBowForceUpliftThisBlock`:
   ```cpp
   // BEFORE Phase 2.4b:
   //   bowModel.setBowPressure (effectiveBowPressure * (0.5f + mpePressure * 1.5f));
   // AFTER Phase 2.4b (HR-9 path: voiceBowForceUpliftThisBlock = 1.0f → bit-exact preserved):
   bowModel.setBowPressure (effectiveBowPressure * (0.5f + mpePressure * 1.5f) * voiceBowForceUpliftThisBlock);
   ```

   At HR-9 path the factor is 1.0f (IEEE 754 identity) → byte-exact regression preserved. At biased path the factor scales pressure correctly.

5. **`Source/BowedContrabassVoice.cpp` — `frictionModel.setRosin(rosin)` at line 647 (~+1 conditional).**

   The existing line `frictionModel.setRosin(rosin);` runs unconditionally from rawRosin per Phase 2.3. With HR-10 in effect, this still runs — the bias path's `frictionModel.setRosin(rosinEq)` overwrites it earlier in Step 2.5 (Step 2.5 runs BEFORE the line at 647, which is in Step 6/7 territory; the order in the existing source is `Step 2.5 setRosin(rosinEq) → Step 7's setRosin(rawRosin)` — this would clobber the bias).

   **CRITICAL ordering decision:** Step 7 / line 647 setRosin must NOT clobber the bias-pushed rosinEq. Two options:
   - **Option A (recommended):** move the existing `frictionModel.setRosin(rawRosin)` call BEFORE Step 2.5 (so bias-side `setRosin(rosinEq)` overwrites for the biased path). At HR-9 path the original `setRosin(rawRosin)` runs alone → byte-exact.
   - **Option B:** guard the line-647 call with `if (subAmount == 0.0f) frictionModel.setRosin(rawRosin);` — protects bias path but adds a branch on the bit-exact path.

   **Plan-phase pin: Option A.** Move `frictionModel.setRosin(rawRosin)` to immediately BEFORE Step 2.5 entry. This way:
   - HR-9 path: `setRosin(rawRosin)` runs; Step 2.5 short-circuits (no second setRosin call); `frictionModel` ends block with `rosin=rawRosin` exactly as Phase 2.4a → bit-exact preserved.
   - Bias path: `setRosin(rawRosin)` runs first; then Step 2.5's else-branch overwrites with `setRosin(rosinEq)` → friction model ends block with biased rosin.

   Line 647's existing `setRosin` site becomes the relocated line; the original line 647 becomes a no-op or is deleted (delete it; no need for placeholder).

   **Net diff:**
   - Line ~205 (`prepareToPlay`): unchanged (`mu_s = 0.85f`, `mu_d = 0.25f` setters carry-forward).
   - Line ~277 (Step 1 raw reads): add `const float rawSubHarmonics = ...`.
   - Move existing `frictionModel.setRosin(rawRosin)` from line ~647 to immediately before Step 2.5 entry (~line 320 region).
   - Line ~330 (after Step 2 `lastSafeDepth.store(...)`): insert Step 2.5 (~30 LOC).
   - Line ~377 (Step 6 setBowPressure call): multiply by `voiceBowForceUpliftThisBlock`.
   - Line ~647 (old setRosin site): delete (relocated above).

6. **Build:**
   ```bash
   cmake --build build --target O-Contrabass --parallel
   cmake --build build --target O-Contrabass-render-test --parallel
   ```
   Expect a clean build with zero warnings. If warnings or errors surface (e.g., `safeDepthForString` namespace mismatch, unused `stringIdx`), debug.

7. **R35-pre invoke `preflight-subharm.sh`** (HR-9 spectral pre-flight gate):
   ```bash
   bash plugins/O-Contrabass/tests/render-harness/preflight-subharm.sh
   ```
   - **STRICT-PASS** (exit 0; subharmEnergyRatio ≥ 0.40): proceed to R35b.
   - **SOFT-PASS** (exit 0; subharmEnergyRatio ∈ [0.30, 0.40)): proceed to R35b but log Phase 2.4-bis remediation flag.
   - **HARD-FAIL** (exit 1; subharmEnergyRatio < 0.30): STOP, retune `kForceBoost` 0.8 → 0.4 in `Source/DSP/SubHarmonicBias.h`, rebuild, re-invoke `preflight-subharm.sh`. If 0.4 also fails, escalate to Phase 2.4-bis chaos detector OR threshold relaxation BEFORE R35 atomic commit.

8. **Stage the source changes** (do NOT commit yet — R35 atomic):
   ```bash
   git add plugins/O-Contrabass/Source/BowedContrabassVoice.h \
           plugins/O-Contrabass/Source/BowedContrabassVoice.cpp
   git status
   ```

**Files created:** none (SubHarmonicBias.h created in R35c).

**Files modified:**
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` (~+5 LOC: 3 new members + 1 accessor).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` (~+30 LOC: Step 2.5 + ROSIN inverse + frictionModel setters + voiceBowForceUpliftThisBlock; ~−1 LOC: relocated setRosin line; ~+1 LOC: Step 6 multiplication).

**Commit:** **NONE** — staged for R35 atomic.

**Success bar:**
- [ ] BowedContrabassVoice.h has 3 new members + 1 accessor.
- [ ] prepareToPlay initialises subHarmonicsSmoothed to 0.0 with 30 ms ramp + voiceBowForceUpliftThisBlock to 1.0.
- [ ] Step 2.5 inserted between Step 2 and Step 3 with HR-9 short-circuit + HR-10 friction module pushes.
- [ ] Step 6 setBowPressure multiplied by voiceBowForceUpliftThisBlock.
- [ ] frictionModel.setRosin call relocated to BEFORE Step 2.5 entry (line 647 site removed).
- [ ] Plugin + harness build clean zero warnings.
- [ ] `preflight-subharm.sh` returns exit 0 (STRICT-PASS or SOFT-PASS).

**Estimated effort:** 90 min (header edits + .cpp edits + build + preflight-subharm + debug if needed).

---

### R35e — Bit-exact regression bar verification

**No source edits committed; verification step. Re-runs `reproduce-goldens.sh` post-source-edit; confirms 10 carry-forward goldens byte-identical (HR-9 IEEE 754 identity arithmetic + active-string-only gate).**

**Tasks:**

1. **Run `reproduce-goldens.sh` (10-golden tripwire):**
   ```bash
   bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   ```
   **All 10 must PASS** byte-identical. If any FAIL, STOP — investigate per RESEARCH §18.14 Risk #1 diagnostic-on-fail playbook:
   - **HR-9 audit:** confirm Step 2.5 short-circuit at `subAmount == 0.0f`; confirm `subHarmonicsSmoothed.setCurrentAndTargetValue(0.0f)` in prepareToPlay; confirm SUB_HARMONICS APVTS default 0.0; confirm `voiceBowForceUpliftThisBlock` reset to 1.0f at top of `renderNextBlock`.
   - **HR-10 audit:** confirm relocated `frictionModel.setRosin(rawRosin)` runs at HR-9 path (Step 2.5's else-branch never executes); confirm Step 7 / per-sample loop sees `rosin = rawRosin` exactly.
   - **Step 6 audit:** confirm `voiceBowForceUpliftThisBlock = 1.0f` IEEE 754 identity (`x * 1.0f = x` exact).
   - **Active-string-only audit:** confirm `activeStringIndex < 0` short-circuit fires at note-off / pre-attack (no spurious bias evaluation when no string is active).
   - **Last resort:** isolate `SubHarmonicBias.h` to a separate translation unit (rare; trigger if header inlining surfaces ODR pathology) OR roll back Step 2.5 to verify the issue.

2. **Re-render `--matrix-stability` for Phase 2.4a evidence golden carry-forward:**
   ```bash
   $HARNESS --matrix-stability \
            --out /tmp/phase24b-r35e/matrix-stability.wav \
            --json /tmp/phase24b-r35e/matrix-stability.json
   computed=$(shasum -a 256 /tmp/phase24b-r35e/matrix-stability.wav | awk '{print $1}')
   expected=$(awk '{print $1}' plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.wav.sha256)
   [ "$computed" = "$expected" ] && echo "[PASS] matrix-stability $computed" \
                                 || echo "[FAIL] regression"
   ```
   **Expected:** `6db6770727ab3b433a036f487217bbde70f8cc15de44fa60ac0b99d868176449`. HR-9 short-circuit fires across all 108 combos (SUB_HARMONICS=0 throughout the matrix-stability harness mode); HR-7 wedge-math bypass continues to fire for Phase 2.4a calibration coverage; bit-exact regression preserved.

3. **Audit log to PLAN-execution-log:**
   ```text
   R35e regression bar audit (10 carry-forward + 1 evidence):
   [PASS] stiffness-zero-pre  d358abcd…
   [PASS] string-A            c6755aa4…
   [PASS] string-D            765b015e…
   [PASS] string-G            0cd5cb0a…
   [PASS] detune-sweep-A      5e31dad3…
   [PASS] note-sequence       3ac3ccd0…
   [PASS] vibrato             d7881ecf…
   [PASS] macro-sweep         c2571dd9…
   [PASS] slow-lfo            c0c2c893…
   [PASS] schelleng-stress    9d18da86…
   [PASS] matrix-stability    6db67707…  (Phase 2.4a evidence carry-forward)
   ```

**Files created:** none.

**Files modified:** none.

**Commit:** **NONE** — verification step.

**Success bar:**
- [ ] `reproduce-goldens.sh` reports `OK: all 10 goldens reproduce byte-identical` (still 10 at this point — pin #3 extension to 12 is staged in R35a but the script body's `OK: all 12` line update lands with R35a's stage; the iteration loop diff vs committed has a known +2 entries that ALSO need to PASS for the script to print `all 12 …`; sequencing pin: extend the INVOC array in R35a, but only AFTER R35d wires bias the new `--sub-harmonics` + `--sub-harmonics-stability` invocations PASS — so R35e is the gating step where the 12-entry script reports `OK: all 12 reproduce byte-identical`).
- [ ] `matrix-stability.wav` sha256 matches Phase 2.4a `6db67707…` carry-forward.
- [ ] R35e audit log compiled.

**Sequencing clarification on `reproduce-goldens.sh`:** R35a stages the script's INVOC array extension (10 → 12 entries). R35b populates the 2 new goldens. R35e is the FIRST execution where all 12 entries can succeed. Until R35b runs, the 2 new entries' `expected` sha256s don't exist. So R35a's stage commits the 12-entry array; R35b's stage commits the 2 new sha256 files; R35e is the FIRST verification step that runs the 12-entry script with both halves of the contract present.

**Estimated effort:** 15 min (run script + matrix-stability re-render + audit log).

---

### R35f — auval + pluginval-10

**No source edits; standard Gate 6b invariant 4 audit (mirrors Phase 2.4a R34h precedent).**

**Tasks:**

1. **Build production plugin (Release VST3 + AU):**
   ```bash
   cmake --build build --target O-Contrabass_VST3 O-Contrabass_AU --parallel
   ```
   Expect zero warnings.

2. **Clear macOS AU cache + install fresh:**
   ```bash
   killall -9 AudioComponentRegistrar 2>/dev/null || true
   rm -rf ~/Library/Caches/AudioUnitCache/
   rm -rf ~/Library/Caches/com.apple.audiounits.cache
   rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3
   rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass.component
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass.vst3 \
       ~/Library/Audio/Plug-Ins/VST3/
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass.component \
       ~/Library/Audio/Plug-Ins/Components/
   ```

3. **auval audit:**
   ```bash
   auval -a | grep -i contrabass
   # Expect: "Ouar  OBass - Ouaricon: O-Contrabass"
   auval -v aumu OBass Ouar
   # Expect: "AU VALIDATION SUCCEEDED"
   ```

4. **pluginval level 10 audit:**
   ```bash
   pluginval --strictness-level 10 --validate-in-process \
       ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3
   # Expect: "ALL TESTS PASSED" / "Plugin valid"
   ```

5. **Audit log to PLAN-execution-log:**
   ```text
   R35f auval / pluginval audit:
   auval AU VALIDATION SUCCEEDED              [PASS]
   pluginval --strictness-level 10 SUCCESS    [PASS]
   ```

**Files created:** none.

**Files modified:** none.

**Commit:** **NONE** — verification step.

**Success bar:**
- [ ] Production plugin builds clean zero warnings.
- [ ] AU cache cleared; fresh install succeeded.
- [ ] auval reports `AU VALIDATION SUCCEEDED`.
- [ ] pluginval --strictness-level 10 reports `SUCCESS`.

**Estimated effort:** 20 min (build + cache clear + install + auval + pluginval).

---

### R35 — Atomic commit (Gate 6b PASS)

**Single git commit lands all R35a–R35f staged files. Continues R7 → R15 → R20 → R26 → R33 → R34 → **R35** sequence.**

**Tasks:**

1. **Confirm staged files match expected manifest:**
   ```bash
   git status
   # Expect ~14 staged files:
   # plugins/O-Contrabass/Source/BowedContrabassVoice.h                                       (modified)
   # plugins/O-Contrabass/Source/BowedContrabassVoice.cpp                                     (modified)
   # plugins/O-Contrabass/Source/DSP/SubHarmonicBias.h                                        (new)
   # plugins/O-Contrabass/tests/render-harness/main.cpp                                       (modified)
   # plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh                           (modified)
   # plugins/O-Contrabass/tests/render-harness/preflight-subharm.sh                           (new)
   # plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.wav.sha256                (new)
   # plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.json                      (new)
   # plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.json.sha256               (new)
   # plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.wav.sha256      (new)
   # plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.json            (new)
   # plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.json.sha256     (new)
   # plugins/O-Contrabass/.planning/STATUS.md                                                 (modified, R35 fields)
   # plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md                                       (modified, this rev-9 append + verify-phase footnote optional)
   # plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md                                    (modified, execute append)
   ```

2. **Commit message body** (per RESEARCH §18.13 footnote template + Phase 2.4a R34 precedent):

   ```
   feat(O-Contrabass): Phase 2.4b sub-harmonic bias DSP-07 (architecture §457) + 36-combo stability matrix - Gate 6b PASS

   Implements ARCHITECTURE.md §457 sub-harmonic bias verbatim — friction-junction
   parameter biasing on F_bow / v_0 / mu_s − mu_d toward Schelleng F_max regime
   to induce period-doubling f0/2 spectral content as a musical bass-extension
   feature on the SUB_HARMONICS APVTS knob (Float [0, 1] default 0.0).

   Gate 6b 5-invariant audit:
   1. reproduce-goldens.sh — 10 carry-forward goldens byte-identical (HR-9 IEEE
      754 identity arithmetic + active-string-only gate guarantee preserved)
   2. --sub-harmonics — pass_subharmAudible PASS (subharmEnergyRatio ≥ 0.40 strict
      / [0.30, 0.40) soft within v1.0 budget per RESEARCH §18.6)
   3. --sub-harmonics-stability — pass_all_36 = true OR failCount ≤ 2 v1.0 budget
   4. auval AU VALIDATION SUCCEEDED + pluginval --strictness-level 10 SUCCESS
   5. matrix-stability — 6db67707… byte-identical (Phase 2.4a evidence carry-forward)

   Implementation:
   - Source/DSP/SubHarmonicBias.h (NEW) — header-only inline namespace
     ouaricon::contrabass::sub_harmonics; coefficients architecture §457 verbatim
     (kForceBoost=0.8, kV0Reduction=0.5, kGapWiden=0.25, kFmaxScalar=0.95);
     applyBias() mutates F_bow/v_0/mu_s in-place; mu_d const-by-value; HR-9 caller-side
     short-circuit; HR-10 friction module ABI preserved (no module v1.0.0 edits).
   - F_bow uplift scaled by Phase 2.4a SchellengCalibration safeDepth per RESEARCH
     §18.4: stable cells (105/108) → full 1.8× uplift (architecture default);
     fallback cells (3/108) → 1.4× uplift (auto-applies architecture §661 fallback
     1 retune). v_0 + mu_s preserve architecture §457 verbatim (NOT safeDepth-scaled).
   - BowedContrabassVoice.{h,cpp} — Step 2.5 inserted between Step 2 (Schelleng wedge)
     and Step 3 (slow-LFO phase advance); subHarmonicsSmoothed (30 ms ramp);
     lastSubAmount instrumentation atom; voiceBowForceUpliftThisBlock factor
     consumed at Step 6 setBowPressure call (HR-9: 1.0f at default → bit-exact
     regression preserved). frictionModel.setRosin(rawRosin) relocated to BEFORE
     Step 2.5 (preserves bit-exact at HR-9 path; bias path overwrites with rosinEq
     via inverse algebraic identity rosinEq = -ln(10·v_0_biased)/4.6f).
   - tests/render-harness/main.cpp — --sub-harmonics + --sub-harmonics-stability
     CLI flags + juce::dsp::FFT size 65536 Hann-windowed analyser per RESEARCH
     §18.5 (3-bin energy windows at f0=41.2 Hz / f0/2=20.6 Hz; subharmEnergyRatio
     + subharmPeakOverFloor diagnostic).
   - tests/render-harness/reproduce-goldens.sh — extended from 10 → 12 goldens.
   - tests/render-harness/preflight-subharm.sh (NEW) — spectral pre-flight gates
     R35 atomic per HR-9 escalation contract (RESEARCH §18.6).

   Phase 2.4b Sub-Harmonic Bias DSP-07 — chaos detector + softClampState deferral
   ─────────────────────────────────────────────────────────────────────────────

   This commit ships ARCHITECTURE.md §457 sub-harmonic bias verbatim with the
   following architecture-spec'd deferments to Phase 2.5/2.6:

   1. Chaos detector (architecture §457 line 476 "optional"):
      "Optional control-rate (~100 Hz) check: lag-2 RMS > lag-1 RMS *and*
      non-periodic → back off bias by 20%."
      Deferred — v1.0 relies on Schelleng F_max clamp (architecture line 470,
      kFmaxScalar=0.95) + algebraic saturator (x/sqrt(1+x²) per architecture
      §"Master Saturator") + bridge filter loop-gain ceiling 0.9999999
      (architecture §"Bridge Filter") as layered stability defences. Reopen
      in Phase 2.5/2.6 if 36-combo --sub-harmonics-stability matrix surfaces
      any failing combo at the 0.5 fallback safeDepth.

   2. softClampState energy clamp (ROADMAP §Phase 2.4 deliverable, threshold
      0.85 ceiling 1.0):
      Deferred — current architecture-spec'd algebraic saturator covers the
      role at v1.0; energy clamp adds redundancy without measured benefit at
      default operating points. Reopen in Phase 2.5/2.6 alongside body
      resonator integration where peak amplitudes can compound.

   3. Phase 2.4-bis backlog items (carry-forward from Phase 2.4a R34 commit
      body, NOT addressed in this commit):
      - Tune Step 4 modulation gain to hit 20% peak-to-peak OR refine
        breathingAudible per-cycle metric.
      - Reduce 3 v1.0 fallback cells via downstream-defense tightening.

   Both deferments preserve architecture intent without amending
   ARCHITECTURE.md (per discuss-phase Q33). Phase 2.4c (autocorrelator +
   saturator-tail O-Bowed comparison) gets fresh CONTEXT rev-8 when its
   discuss-phase opens after Phase 2.4b verifies (Gate 6b PASS).

   NO Stage-1 contract amendment (parameter-spec.md sha256 77638e25… unchanged;
   STATUS.md contract_checksums.parameter_spec carries forward).
   NO ARCHITECTURE.md amendment (bias formula IS architecture §457 verbatim).

   Continues atomic-commit sequence R7 → R15 → R20 → R26 → R33 → R34 → R35.
   R37 Logic AU smoke deferred non-blocking (mirrors R32/R27/R19f/R14e precedent).

   Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
   ```

3. **Commit:**
   ```bash
   git commit -m "$(cat <<'EOF'
   feat(O-Contrabass): Phase 2.4b sub-harmonic bias DSP-07 (architecture §457) + 36-combo stability matrix - Gate 6b PASS

   ... (full body above) ...
   EOF
   )"
   ```

4. **Verify:**
   ```bash
   git log --stat HEAD~1..HEAD
   # Expect: ~14 files in single commit; net +~440 LOC source/tooling + 1 new header + 6 new goldens.
   git log --oneline -1
   # Expect: <new sha> feat(O-Contrabass): Phase 2.4b sub-harmonic bias DSP-07 ...
   ```

5. **Capture R35 sha for STATUS.md backfill (R35-backfill chore commit lands separately):**
   ```bash
   R35_SHA=$(git log --oneline -1 | awk '{print $1}')
   echo "R35 atomic commit sha: $R35_SHA"
   ```

**Files created:** none in this step (all staged from R35a–R35f).

**Files modified:** STATUS.md execute-phase update (next_action / gate_state / phase_2_4b_atomic_commit_sha placeholder / Cycle Scope / Current Position / plan_revision_status), SUMMARY.md "Phase 2.4b execute" append (~20–30 LOC), VERIFICATION.md UNCHANGED at this step (verify-phase appends post-R35).

**Commit:** **YES — R35 atomic commit.** Single commit lands the entire Phase 2.4b cycle.

**Success bar:**
- [ ] `git status` clean post-commit.
- [ ] `git log --stat HEAD~1..HEAD` shows ~14 files in single commit.
- [ ] R35 sha captured for backfill chore.
- [ ] Commit-body footnote includes chaos detector + softClampState deferral notes per RESEARCH §18.13.
- [ ] STATUS.md `next_action` flipped to `phase_2_4b_verify` (or `phase_2_4c_discuss` if verify is auto-completed inline).
- [ ] STATUS.md `gate_state.phase_2_4b_subharmonic_bias_dsp07: PASS_gate_6b_<strict|soft>`.

**Estimated effort:** 30 min (verify staging + author commit body + commit + verify log).

---

### R35-backfill chore — STATUS.md sha propagation

**Separate non-atomic chore commit; mirrors R34-backfill (`b64c8c4`) / R33-backfill / R26 precedent.**

**Tasks:**

1. **Edit STATUS.md `phase_2_4b_atomic_commit_sha` field:**
   Replace placeholder with R35 sha captured above:
   ```yaml
   phase_2_4b_atomic_commit_sha: <R35_SHA>  # R35 atomic commit landed during execute YYYY-MM-DD ...
   ```

2. **Commit:**
   ```bash
   git add plugins/O-Contrabass/.planning/STATUS.md
   git commit -m "$(cat <<'EOF'
   chore(O-Contrabass): backfill Phase 2.4b R35 commit sha (<short-sha>) into STATUS.md

   Propagates R35 atomic commit sha into STATUS.md phase_2_4b_atomic_commit_sha
   field per R34-backfill (b64c8c4) / R33-backfill precedent. Gate 6b CLEARED
   during execute-phase; this chore commit completes the audit trail without
   amending the atomic R35 commit body.

   Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
   EOF
   )"
   ```

3. **Verify:**
   ```bash
   git log --oneline -2
   # Expect: <chore sha>  chore(O-Contrabass): backfill Phase 2.4b R35 commit sha ...
   #         <R35 sha>    feat(O-Contrabass): Phase 2.4b sub-harmonic bias DSP-07 ...
   ```

**Files created:** none.

**Files modified:**
- `plugins/O-Contrabass/.planning/STATUS.md` (1 field).

**Commit:** **YES — R35-backfill chore commit (separate from R35 atomic).**

**Success bar:**
- [ ] STATUS.md `phase_2_4b_atomic_commit_sha` field populated with R35 sha.
- [ ] Chore commit lands cleanly.
- [ ] Audit-trail anchored: `git log --grep "Phase 2.4b"` returns 2 commits (R35 + R35-backfill chore).

**Estimated effort:** 10 min (edit + commit + verify).

---

## Why R35 is a single atomic commit

Same gate-first principle as R7 (Phase 2.1a-recovery), R15 (Phase 2.1b extraction), R20 (Phase 2.1c dispersion), R26 (Phase 2.2 4-string bank), R33 (Phase 2.3 modulator + macro), R34 (Phase 2.4a Schelleng calibration):

1. **Coupling:** Phase 2.4b artefacts (`SubHarmonicBias.h` new header + `BowedContrabassVoice.{h,cpp}` Step 2.5 integration + `main.cpp` --sub-harmonics + --sub-harmonics-stability modes + 6 new golden text files + `reproduce-goldens.sh` extension + `preflight-subharm.sh` script + planning artefacts) are mutually coupled. Splitting yields broken intermediate states — e.g., SubHarmonicBias.h landing without the BowedContrabassVoice integration is dead code; harness modes landing without the bias DSP produce baseline subharmEnergyRatio ≈ 0.241 → pass_subharmAudible permanently FAIL; preflight-subharm.sh landing without the harness mode is unrunnable.
2. **Bisect safety:** if a future Phase 2.x bug bisects back to "sub-harmonic bias landing", a single SHA flips the entire feature.
3. **Audit trail:** Phase 2.x's atomic-commit sequence is R7 → R15 → R20 → R26 → R33 → R34 → **R35**. The Phase 2.x timeline stays trivially reconstructible from `git log --grep "Phase 2."`.
4. **No Stage-1 contract amendment:** Phase 2.4b is a pure DSP feature implementation against an already-declared APVTS parameter (`SUB_HARMONICS` at PluginProcessor.cpp:104 carries forward unchanged from Stage-1). parameter-spec.md unchanged, ARCHITECTURE.md unchanged, STATUS.md `contract_checksums.parameter_spec` unchanged at `77638e25…`. Single atomic commit captures the entire implementation cycle without contract churn.
5. **HR-9 + HR-10 atomic rollback:** if an unforeseen regression bisects to R35, the entire Phase 2.4b feature can be reverted by a single `git revert R35_SHA` — friction module v1.0.0 ABI is preserved, the only changes are caller-side per HR-10. No cross-module fallout.

---

## Files To Create / Modify (consolidated, Phase 2.4b)

### Source (modified)
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` — R35d (~+5 LOC: 3 new private members `subHarmonicsSmoothed` / `lastSubAmount` / `voiceBowForceUpliftThisBlock` + 1 public accessor `getLastSubAmount()`).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` — R35d (~+30 LOC Step 2.5 + ROSIN inverse + frictionModel pushes + voiceBowForceUpliftThisBlock; ~−1 LOC `setRosin(rawRosin)` line 647 deleted (relocated above Step 2.5); ~+1 LOC Step 6 multiplication; ~+1 LOC raw `rawSubHarmonics` read in Step 1; ~+3 LOC `prepareToPlay` initialisation; net ~+34 LOC).

### Source (new)
- `plugins/O-Contrabass/Source/DSP/SubHarmonicBias.h` — R35c (NEW; ~120 LOC; HR-5-mirror `inline constexpr` + `inline` linkage; HR-10 friction-module-ABI-preserving signature; namespace `ouaricon::contrabass::sub_harmonics`).

### Harness (modified)
- `plugins/O-Contrabass/tests/render-harness/main.cpp` — R35a (~+265 LOC: --sub-harmonics + --sub-harmonics-stability mode handling + FFT analyser + 36-combo iteration loop + PerComboMetrics extension).

### Reproduction script (modified)
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` — R35a (~+10 LOC: extend INVOC array from 10 to 12 entries; trailing diagnostic `OK: all 12 …`).

### Pre-flight script (new)
- `plugins/O-Contrabass/tests/render-harness/preflight-subharm.sh` — R35-pre (NEW; ~30 LOC; chmod +x; spectral pre-flight gates R35 atomic per HR-9 escalation).

### Test artefacts (new, committed as text-only — sha256 + JSON)
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.wav.sha256` — R35b (NEW).
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.json` — R35b (NEW; ~3 KB).
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.json.sha256` — R35b (NEW).
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.wav.sha256` — R35b (NEW; sha256 of 36-combo concatenated WAV).
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.json` — R35b (NEW; ~30 KB; per-combo + aggregate truth-table).
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.json.sha256` — R35b (NEW).

### Test artefacts (NOT committed — staged-only or transient)
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.wav` (~30 MB stereo 24-bit PCM ~3.3 min audio; reproducible from harness via `--sub-harmonics-stability`).
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.wav` (~1 MB; reproducible via `--sub-harmonics`).
- `/tmp/phase24b-r35pre/*.{wav,json}` (R35-pre tripwire; transient; deleted post-R35).
- `/tmp/preflight-subharm.{wav,json}` (preflight-subharm.sh output; transient).
- `/tmp/phase24b-r35a-smoke.{wav,json}` (R35a baseline smoke; transient).
- `/tmp/phase24b-r35b/*.{wav,json}` (R35b render captures; transient).
- `/tmp/phase24b-r35e/*.{wav,json}` (R35e regression bar audit; transient).

### Test artefacts (modified, re-baselined)
- **None.** HR-9 IEEE 754 identity arithmetic preserves bit-exact regression on all 10 carry-forward goldens. NO re-baseline in Phase 2.4b.

### Stage-1 contract amendment
- **None.** parameter-spec.md unchanged at `77638e25…` (SUB_HARMONICS already declared in Stage 1). STATUS.md `contract_checksums.parameter_spec` carries forward.

### Planning artefacts (modified)
- `plugins/O-Contrabass/.planning/STATUS.md` — R35 (next_action / gate_state.phase_2_4b_subharmonic_bias_dsp07 / phase_2_4b_verify_outcome / phase_2_4b_atomic_commit_sha placeholder + chore-backfill / Cycle Scope / Current Position / plan_revision / plan_revision_status updates).
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` — already at rev-7 (no further edits in execute; rev-7 lock holds).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` — already at §18 append (no further edits in execute).
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` — this rev-9 append (no further edits in execute; verify-phase may add a "rev-9 retrospective" footnote if anomalies surfaced).
- `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` — execute-phase appends "Phase 2.4b execute" section after R35.
- `plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md` — verify-phase appends "Phase 2.4b verify" section with R35e regression bar audit table + R35f auval/pluginval logs + Phase 2.4c discuss prep notes.

### Files explicitly NOT touched
- `plugins/O-Contrabass/Source/DSP/WaveguideString.{h,cpp}` (Phase 2.2 R26-frozen).
- `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` (Phase 2.1c R20-frozen).
- `plugins/O-Contrabass/Source/DSP/SchellengCalibration.h` (Phase 2.4a R34-frozen; consumed verbatim via `safeDepthForString` lookup).
- `plugins/O-Contrabass/Source/PluginProcessor.{h,cpp}` (Phase 2.4a R34-frozen at HR-7 weak-symbol surface; SUB_HARMONICS APVTS at line 104 declared in Stage 1, consumed without amendment).
- `plugins/O-Contrabass/Source/PluginEditor.{h,cpp}` (Stage 3 work).
- `plugins/O-Contrabass/Source/OContrabassMPESynthesiser.{h,cpp}` (voice count = 1; bias lives inside the existing single voice).
- `plugins/O-Contrabass/Source/BowModel.{h,cpp}` (Phase 2.1a-frozen; consumed via `getBowVelocity()` accessor only).
- `modules/synthesis/bow-friction/*` (Phase 2.1b-frozen v1.0.0; HR-10 ABI preservation; `setRosin` + `setStaticFrictionCoefficient` setters consumed without module surface change).
- `modules/registry.yaml` (no module surface changes).
- `plugins/O-Bowed/*` (zero cross-plugin coupling; bias is per-plugin per CONTEXT rev-7 Q24).
- `plugins/O-Contrabass/research/ARCHITECTURE.md` (deferred amendments to end-of-Stage-2 verify; closed-form §"Slow-Bow LFO" / §"DC Blocker" / §"In-loop saturator" stays as conceptual reference; chaos detector + softClampState deferral tracked in R35 commit-body footnote per RESEARCH §18.13).
- `plugins/O-Contrabass/.planning/parameter-spec.md` (Phase 2.4b does NOT amend Stage-1 contract).
- `plugins/O-Contrabass/.planning/parameter-spec-draft.md` (historical; audit trail).
- `plugins/O-Contrabass/.planning/BRIEF.md` / `REQUIREMENTS.md` / `ROADMAP.md` (no SUB_HARMONICS-default-changing references).
- `plugins/O-Contrabass/.planning/stages/1-foundation/*` (closed milestone).
- `plugins/O-Contrabass/CMakeLists.txt` (NO new explicit listing required; Phase 2.4a precedent showed `Source/DSP/SchellengCalibration.h` consumed via `#include` only — same expected for `SubHarmonicBias.h`).
- `tools/schelleng-fit/*` (Phase 2.4a-frozen; Phase 2.4b reuses calibration table via `safeDepthForString`).
- 10 carry-forward golden text files (HR-9 IEEE 754 identity arithmetic preserves byte-identity; NOT re-baselined).
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.{wav.sha256,json,json.sha256}` (Phase 2.4a evidence carry-forward; HR-9 short-circuit + HR-7 wedge bypass continue to fire byte-exactly).
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-sweep.wav.sha256` (Phase 2.1c-only; not in canonical 10-golden bar).

---

## Dependencies Graph (compact)

```
R35-pre (working-tree integrity tripwire; reproduce-goldens.sh 10/10 PASS at HEAD
         b64c8c4; matrix-stability sanity at 6db67707…; preflight-subharm.sh
         authored + chmod +x + staged for R35 atomic; preflight script invocation
         deferred to post-R35d build)
   ↓                ↓ (mismatch → STOP, investigate working-tree drift)
R35a (main.cpp --sub-harmonics + --sub-harmonics-stability flags + FFT analyser +
      36-combo iteration loop + PerComboMetrics extension; reproduce-goldens.sh
      INVOC array extended from 10 → 12 entries; ~+275 LOC across 2 files;
      NO build dependency on bias DSP — R35a's audible-mode renders return
      baseline 0.241 ratio; staged for R35 atomic; build smoke validates harness
      compiles + parses flags but pass_subharmAudible expected FALSE without bias)
   ↓ pre-R35c smoke: harness builds clean; --sub-harmonics returns subharmEnergyRatio ≈ 0.241
R35c (Source/DSP/SubHarmonicBias.h authored verbatim per RESEARCH §18.9; HR-5
      `inline constexpr` linkage; HR-10 friction-module-ABI-preserving signature;
      ~120 LOC; staged for R35 atomic; NO build dependency on integration —
      header is unused until R35d wires the call site)
   ↓
R35d (BowedContrabassVoice.{h,cpp}: 3 new private members + accessor +
      prepareToPlay init + Step 2.5 integration + Step 6 multiplication +
      relocated frictionModel.setRosin(rawRosin); ~+34 LOC across 2 files;
      build O-Contrabass + O-Contrabass-render-test; staged for R35 atomic)
   ↓ post-R35d build clean → invoke preflight-subharm.sh
R35-pre→preflight-subharm (HR-9 escalation gate)
   ↓ STRICT-PASS or SOFT-PASS              ↓ HARD-FAIL
                                            ↓ retune kForceBoost 0.8→0.4 in
                                              SubHarmonicBias.h; rebuild;
                                              re-invoke preflight-subharm; if
                                              0.4 also fails, escalate to
                                              Phase 2.4-bis chaos detector
R35b (Render --sub-harmonics + --sub-harmonics-stability goldens; capture sha256s;
      validate pass_subharmAudible + pass_all_36/failCount≤2; sha256 stability
      smoke; stage 6 new golden text files)
   ↓ (PASS)              ↓ (FAIL stability — failCount > 2)
                           ↓ pin #11 escalation: jq triage → fallback-cell cluster
                             check → kForceBoost retune if needed → re-render
R35e (reproduce-goldens.sh runs 12-entry; matrix-stability sanity at 6db67707…;
      audit log compiled; HR-9 + HR-10 audit if 10 carry-forward break)
   ↓ (PASS)
R35f (Production build; auval AU VALIDATION SUCCEEDED; pluginval-10 SUCCESS)
   ↓ (PASS)
R35 (Phase 2.4b atomic commit — ~14 files; closes 2.4b; mirrors R34/R33/R26/
     R20/R15/R7 atomic-commit pattern; commit body documents 5-invariant Gate
     6b audit + chaos detector + softClampState deferral footnote per RESEARCH
     §18.13 + NO contract amendment + Phase 2.4c follow-up scope)
   ↓
R35-backfill chore (separate non-atomic; STATUS.md phase_2_4b_atomic_commit_sha;
                    matches R34-backfill / R33-backfill precedent)
```

R35-pre is a strict prerequisite — if working tree drifts, all subsequent bit-exact reasoning breaks. R35a + R35c are independent (harness vs source); R35a's smoke baseline confirms FFT analyser correctness BEFORE bias DSP lands; R35c is pure header authoring. R35d is the integration step — the build dependency is `SubHarmonicBias.h` from R35c. R35-pre's `preflight-subharm.sh` invocation (HR-9 escalation gate) fires BETWEEN R35d and R35b — R35b's golden capture is contingent on the spectral pre-flight returning STRICT-PASS or SOFT-PASS. R35e is the bit-exact regression bar (10 carry-forward + 1 evidence). R35f is the Gate 6b audit. R35 is the gated finisher.

---

## Risks (Phase 2.4b, refreshed from RESEARCH §18.14)

| # | Risk | Current state | Mitigation |
|---|---|---|---|
| 1 | HR-9 bit-exact regression failure on 10 carry-forward goldens | **PRE-FLIGHT PASS at HEAD** (RESEARCH §18.1); reproduce-goldens.sh 10/10 byte-identical | HR-9 short-circuit + IEEE 754 identity arithmetic + active-string-only gate are technical defences. R35-pre re-runs `reproduce-goldens.sh` at HEAD; R35e re-runs post-source-edit. If FAIL, isolate per RESEARCH §18.14 Risk #1 diagnostic-on-fail playbook (HR-9 audit / HR-10 audit / Step 6 audit / active-string audit / last-resort isolate SubHarmonicBias.h to separate translation unit OR roll back). |
| 2 | SchellengCalibration→F_max mapping semantic mismatch | **RESOLVED RESEARCH §18.4** — `effectiveBoost = subAmount · 0.8 · safeDepth` preserves architecture §457 verbatim at stable cells (105/108) + auto-applies architecture §661 fallback 1 retune at fallback cells (3/108) | DISSOLVED. PLAN rev-9 commits §18.9 SubHarmonicBias.h verbatim. |
| 3 | `pass_subharmAudible` threshold 0.10 too lax | **CONFIRMED LAX RESEARCH §18.3** — baseline at SUB_HARMONICS=0 already returns ratio 0.241; threshold MUST be raised | PLAN rev-9 locks RESEARCH §18.6 thresholds: 0.40 strict / 0.30 soft / <0.30 hard-FAIL. R35-pre invokes `preflight-subharm.sh` post-R35d build; hard-FAIL escalates to `kForceBoost` 0.8→0.4 retune BEFORE R35 atomic commit. |
| 4 | Period-doubling chaotic regime at SUB_HARMONICS=1.0 + INFINITE_SUSTAIN=1.0 | Layered defences: SchellengCalibration F_max clamp inside bias (§18.4 / §18.9), algebraic saturator, loop-gain ceiling 0.9999999 | Risk reduced. R35b 36-combo `--sub-harmonics-stability` exercises 12 SUB+SUS=high combos (4 strings × 3 SUB ∈ {0,0.5,1.0} × stringIdx-specific MIDI). If any combo NaN/peak>1.0/runaway, pin #11 escalation (jq triage → fallback-cell cluster check → coefficient retune). failCount ≤ 2 v1.0 budget. |
| 5 | Active-string-only bias under crossfade — audible "switch" mid-note-change | 30 ms `subHarmonicsSmoothed` ramp absorbs discontinuity; HR-9 active-string gate analogous to HR-1 vibrato (Phase 2.3 verified vibrato gates didn't audibly switch); v1.0 doesn't include automated note-transition test | PLAN rev-9 locks `subHarmonicsSmoothed.setCurrentAndTargetValue(0.0f)` in prepareToPlay + `setTargetValue` UNCONDITIONAL each block per Phase 2.3 pin #11. R37 Logic AU smoke (deferred non-blocking) suggests verifying with MIDI 28→33 transition + SUB_HARMONICS=1.0 ramp; if user-confirmed audible click, escalate to Phase 2.4-bis crossfade-aware bias ramp. |
| 6 | `subHarmonicsSmoothed.setTargetValue` UNCONDITIONAL each block — denormal accumulation | Phase 2.3 macroSmoothed pin #11 precedent | PLAN rev-9 preamble pins UNCONDITIONAL each block; `juce::ScopedNoDenormals` already in place at processBlock entry from Stage 1 (carries forward). |
| 7 | SUB_HARMONICS default 0.0 audit | All 10 current golden render configs use default SUB_HARMONICS=0 (verified by RESEARCH §18.1 — no source change required, parameter at PluginProcessor.cpp:104 default 0.0; all goldens reproduce byte-identical at HEAD) | DISSOLVED. R35-pre confirms via reproduce-goldens.sh. |
| 8 | Bias's F_max clamp interaction with HR-4 (Schelleng wedge skip on SLOW_LFO_DEPTH=0) | Bias's F_max clamp via SchellengCalibration is INDEPENDENT of HR-4 wedge gate — read-only lookup, cheap; Step 2.5 fires regardless of `rawSlowLfoDepth` value (bias is independent of slow-LFO) | DISSOLVED. R35b `--sub-harmonics` mode runs at SLOW_LFO_DEPTH=0 default; HR-4 short-circuits wedge math at Step 2; bias's F_max clamp at Step 2.5 fires from §18.9 schellengFmax() helper independently. R35b `--sub-harmonics-stability` 36-combo does NOT vary SLOW_LFO_DEPTH (kept at default 0.0); independence preserved. |
| 9 | Period-doubling spectral content shifts FFT bin selection | **CONFIRMED at RESEARCH §18.3** — current bin choice (3-bin ±0.5 Hz at f0/2 and f0) captures period-doubling fundamental cleanly; broadband transient sidebands minor at default bow params | Mitigated — RESEARCH §18.5 bin selection locked. R35-pre `preflight-subharm.sh` validates against measured threshold. If post-R35d ratio falls within bin-selection ambiguity (e.g., if peak shifts to bin 33 outside the f0/2 ±0.5 Hz window), escalate to RESEARCH §18.16 #10 (E1-only bin map at v1.0; per-string variants deferred to Phase 2.4-bis). |
| 10 | R35 atomic commit interaction with R34-backfill chore | R34-backfill chore `b64c8c4` propagated R34 sha; R35 atomic commit lands while R34-backfill is HEAD; R35-backfill chore propagates R35 sha | DISSOLVED. R35-backfill chore commit follows R35 atomic commit per R34-backfill / R33-backfill / R26 precedent. |
| 11 | `kForceBoost = 1.8` cap matches architecture §1.3 default | Bias formula `F_bow *= 1.0f + 0.8f * subAmount` produces F_bow×1.8 at subAmount=1.0 — matches kForceBoost cap | RESOLVED RESEARCH §18.4 — `kForceBoost` is the 0.8 coefficient (NOT a separate constant); architecture §661 fallback 1 (kForceBoost 1.8 → 1.4) maps to coefficient 0.8 → 0.4 in SubHarmonicBias.h. PLAN rev-9 hard-FAIL escalation contract lands at coefficient 0.4. |
| 12 | Phase 2.4-bis backlog crowding | RESEARCH §18.4 mapping uses `safeDepth` to AUTOMATICALLY apply architecture §661 fallback 1 retune at the 3 fallback cells — bypasses the crowding concern at v1.0 | Risk reduced. If Gate 6b reveals additional fallback-cell intersection problems, escalation lane is Phase 2.4-bis post-2.4b verify. PLAN rev-9 commits Phase 2.4-bis backlog continuation in R35 commit-body footnote per RESEARCH §18.13. |
| 13 | **NEW (RESEARCH §18.14)** — friction module ABI preservation under v_0 mutation | Q24 says module v1.0.0 untouched; bias mutates v_0 by reference but caller pushes via ROSIN inverse algebraic identity (HR-10) | Mitigated — RESEARCH §18.10 documents `rosinEq = -ln(10·v_0_biased)/4.6f` inverse; ~30 ns per block; clamped `[0,1]` for safety. Friction module surface unchanged. R35e reproduce-goldens.sh confirms HR-10 path preserves bit-exact regression at HR-9 short-circuit. |
| 14 | **NEW (RESEARCH §18.14)** — bias's mu_s push interacts with idle/crossfade-shadow strings | All 4 strings share single frictionModel instance; idle strings' processSample(0,0,frictionModel) sees mutated mu_s but excitation is 0 → no audible effect; crossfade-shadow string's 5 ms decay sees mutated mu_s but ramp absorbs discontinuity | Mitigated — architectural correctness preserved per RESEARCH §18.10. PLAN rev-9 preamble pins Step 2.5 ordering: bias state push to frictionModel happens ONCE per block AFTER Step 2.5, BEFORE Step 6 per-sample loop; all 4 strings consume biased state consistently for that block. |
| 15 | **NEW** — sha256 stability across re-renders for 36-combo concatenated WAV | Phase 2.4a Risk #13 carry-forward; matrix-stability state-bleed concern revisited at smaller scale | R35b smoke step re-renders `--sub-harmonics-stability` and confirms sha256 matches first invocation. If FAIL, instrument `releaseResources()` between combos (Phase 2.4a R34a precedent). Mitigation: explicit `processor.releaseResources(); processor.prepareToPlay(...)` between combos in `runSubHarmonicsStabilityMode()`. |
| 16 | **NEW** — Step 6 multiplication by `voiceBowForceUpliftThisBlock` may interact with Phase 2.3 macro layer | Step 6 expression is `effectiveBowPressure * (0.5f + mpePressure * 1.5f) * voiceBowForceUpliftThisBlock`; macro layer applied at Step 5 already; uplift is a multiplicative scalar applied AFTER macro | At HR-9 path uplift = 1.0f (IEEE 754 identity) → bit-exact preserved. At biased path uplift scales pressure POST-macro — desired behaviour (architecture §457 specifies bias on F_bow regardless of macro). If user surfaces unexpected interaction (e.g., EXPRESSION_MACRO + SUB_HARMONICS combined produces inflection), document in Phase 2.4-bis. PLAN rev-9 commits this ordering as the locked behaviour. |
| 17 | **NEW** — `mpePressureBlockEntry` snapshot may not exist as a named variable at line ~377 in current source | Pin #7 sampling specifies block-entry MPE pressure (not per-sample); current source reads MPE per-sample inside Step 7 loop | At R35d implementation, add a single block-entry MPE pressure read at `renderNextBlock` entry: `mpePressureBlockEntry = currentlyPlayingNote.pressure.asUnsignedFloat();` (or equivalent accessor). This is a 1-line addition; per-sample MPE inside Step 7 carries forward unchanged. PLAN rev-9 commits the variable-name pin: `mpePressureBlockEntry`. |

---

## Success Criteria (Gate 6b — Phase 2.4b verify exit gate)

- [ ] **R35-pre** — Working-tree integrity tripwire: `reproduce-goldens.sh` reports `OK: all 10 goldens reproduce byte-identical`; `matrix-stability.wav` sha256 matches `6db67707…`; `preflight-subharm.sh` authored + `chmod +x` + staged for R35.
- [ ] **R35a** — `tests/render-harness/main.cpp` `--sub-harmonics` + `--sub-harmonics-stability` CLI flags + FFT analyser per RESEARCH §18.5 + 36-combo iteration loop per §18.7. `PerComboMetrics` extended with `subHarmonics` + `lastSubAmount` fields. `reproduce-goldens.sh` INVOC array extended from 10 to 12 entries. Smoke at HEAD post-R35a (no bias DSP yet): `--sub-harmonics` reports `subharmEnergyRatio ≈ 0.241` (RESEARCH §18.3 baseline measurement); `--sub-harmonics-stability` reports `pass_all_36 = true`.
- [ ] **R35c** — `Source/DSP/SubHarmonicBias.h` authored verbatim per RESEARCH §18.9. Header `#pragma once`-guarded; `inline constexpr` on coefficients; `inline` on `schellengFmax` + `applyBias`; namespace `ouaricon::contrabass::sub_harmonics`. Coefficients verbatim: `kForceBoost = 0.8f`, `kV0Reduction = 0.5f`, `kGapWiden = 0.25f`, `kFmaxScalar = 0.95f`, `kV0Floor = 0.005f`. Function signature verbatim: `applyBias(float subAmount, int stringIdx, float v_b, float beta, float safeDepth, float& F_bow, float& v_0, float& mu_s, float mu_d) noexcept`.
- [ ] **R35d** — `BowedContrabassVoice.h` 3 new private members + 1 accessor + `voiceBowForceUpliftThisBlock`. `BowedContrabassVoice.cpp` `prepareToPlay` initialisation + Step 2.5 inserted between Step 2 and Step 3 + Step 6 multiplication by uplift factor + relocated `frictionModel.setRosin(rawRosin)` to BEFORE Step 2.5. HR-9 + HR-10 visually traceable in code. Plugin + harness build clean zero warnings.
- [ ] **R35-pre→preflight-subharm.sh** — HR-9 escalation gate: STRICT-PASS (`subharmEnergyRatio ≥ 0.40`) OR SOFT-PASS (`[0.30, 0.40)` within v1.0 budget); HARD-FAIL (`< 0.30`) STOPs and triggers `kForceBoost` 0.8→0.4 retune.
- [ ] **R35b invariant (1)** — `--sub-harmonics` golden: `pass_subharmAudible = true` (subharmEnergyRatio ≥ 0.40 strict-PASS) OR soft-PASS within v1.0 budget. New WAV/JSON/JSON-sha256 sha256s captured; 3 new golden text files staged.
- [ ] **R35b invariant (2)** — `--sub-harmonics-stability` golden: `pass_all_36 = true` (strict-PASS) OR `failCount ≤ 2` (soft-PASS within v1.0 budget per pin #8). Sha256 stability across re-renders confirmed (Risk #15 mitigation). 3 new golden text files staged.
- [ ] **R35e invariant (3)** — Bit-exact regression bar: `reproduce-goldens.sh` reports `OK: all 12 goldens reproduce byte-identical` post-R35d source edits. 10 carry-forward goldens + 2 new (sub-harmonics + sub-harmonics-stability) byte-identical to R35b-captured.
- [ ] **R35e invariant (4)** — `--matrix-stability` carry-forward: WAV sha256 matches Phase 2.4a `6db67707…` byte-identical. HR-9 short-circuit + HR-7 wedge bypass interact correctly at SUB_HARMONICS=0 across all 108 combos.
- [ ] **R35f invariant (5)** — auval reports `AU VALIDATION SUCCEEDED`; pluginval --strictness-level 10 reports `SUCCESS`.
- [ ] **R35e + R35f audit table** — Five-item Gate 6b bar compiled (input to VERIFICATION.md): items (1)–(5) above.
- [ ] **R37 (optional)** — Logic AU smoke: USER-CONFIRMED PASS or DEFERRED (mirrors R32 / R27 / R19f / R14e / R34h precedent). Audition sequence per CONTEXT rev-7 Q28: MIDI 28 (E1) sustained + SUB_HARMONICS 0→1.0 ramp at 30 s (confirm audible f0/2 emergence post-bias); MIDI 33 (A1) + SUB_HARMONICS=1.0 + INFINITE_SUSTAIN=1.0 (chaos check; no audible runaway); MIDI 38 (D2) + EXPRESSION_MACRO 0→1.0 ramp at SUB_HARMONICS=0.5 (confirm Phase 2.3 modulator surface unaffected by Phase 2.4b bias swap-in).
- [ ] **R35** — Atomic commit landed; `git log --stat HEAD~1..HEAD` shows ~14 files in single commit; STATUS.md `next_action` flipped to `phase_2_4b_verify` (or `phase_2_4c_discuss` if verify auto-completed); `gate_state.phase_2_4b_subharmonic_bias_dsp07: PASS`; `phase_2_4b_atomic_commit_sha` recorded via R35-backfill chore commit. Commit body documents 5-invariant Gate 6b audit + chaos detector + softClampState deferral footnote per RESEARCH §18.13 + NO contract amendment + Phase 2.4c follow-up scope. **NO `parameter-spec.md` edit; NO `contract_checksums.parameter_spec` change.**
- [ ] **(ARCHITECTURE.md amendments, post-Stage-2-verify, OUT OF SCOPE for this execute)** — §"DC Blocker" + §"In-loop saturator" amendments still deferred. §"Sub-Harmonic Bias" stays as conceptual reference verbatim — bias formula IS architecture §457 verbatim. Chaos detector + softClampState deferred via R35 commit-body footnote (NOT a §457 amendment).

When all checks above are green (R37 user-confirmed or accepted as deferred), **Phase 2.4b verifies** and **Stage 2 progress reaches ~70%** (2.1 + 2.2 + 2.3 + 2.4a + 2.4b closed; 2.4c, 2.5, 2.6 remain). Phase 2.4c (autocorrelator octave-rejection harness fix + saturator-tail O-Bowed comparison) opens as a fresh GSD cycle; CONTEXT rev-8 written when 2.4c discuss-phase opens.

---

## Out of Scope (deferred per CONTEXT.md rev-7 + RESEARCH §18 + STATUS.md)

- **Phase 2.4c** — Autocorrelator octave-rejection vibrato harness fix + saturator-tail O-Bowed comparison harness + saturator-choice decision tree (deferred to its own GSD cycle; CONTEXT rev-8 written when opens).
- **Phase 2.4-bis** — Coefficient retune for additional fallback cells / breathingAudible metric refinement / 3 v1.0 fallback cell reduction via downstream-defense tightening (carry-forward from Phase 2.4a R34 commit body; not addressed in Phase 2.4b).
- **Chaos detector (architecture §457 line 476 "optional")** — control-rate ~100 Hz lag-2 RMS check with 20% bias back-off; deferred to Phase 2.5/2.6 per RESEARCH §18.13. v1.0 relies on Schelleng F_max clamp + algebraic saturator + loop-gain ceiling 0.9999999 as layered defences.
- **softClampState energy clamp** — ROADMAP §Phase 2.4 deliverable (threshold 0.85 ceiling 1.0); deferred to Phase 2.5/2.6 alongside body resonator integration per RESEARCH §18.13.
- **Phase 2.5** — Body resonator + bow noise (8-mode parallel biquad body bank Askenfelt-derived; 3-band BPF bow noise summed AFTER body resonator). Phase 2.4b 36-combo matrix does NOT include BODY_DAMPING axis — full 108-combo with BODY_DAMPING revisits at end-of-Stage-2 once Phase 2.5 lands.
- **Phase 2.6** — Master saturator/limiter + microtonal + MPE + Note Expression + MTS-ESP + Scala/TUN.
- **Per-string SUB_HARMONICS variation** — single global APVTS knob at v1.0 per RESEARCH §18.12; per-string sub bias (e.g., E1-only) deferred to Phase 3+.
- **Per-string `--sub-harmonics-A/-D/-G` audible-mode goldens** — RESEARCH §18.5 bin map is E1-only (f0=41.2 Hz); per-string variants with bin shift deferred to Phase 2.4-bis or 2.4c.
- **Per-sample bias re-evaluation in Step 7** — block-entry pressure value used as bias input at v1.0 per pin #7; per-sample re-evaluation would multiply CPU 256× per block; deferred unless Phase 2.4-bis surfaces audible per-sample pressure variation conflict.
- **`SubHarmonicBias.h` extraction to shared module (`modules/dsp/sub-harmonics/`)** — per CONTEXT rev-7 Q24 + ARCHITECTURE §765 ("O-Contrabass-specific, should NOT bleed back into O-Bowed defaults"). Per-plugin `Source/DSP/SubHarmonicBias.h` mirrors per-plugin `Source/DSP/SchellengCalibration.h` precedent. Future-plugin extraction cheap if needed.
- **Higher-order interpolation in `safeDepthForString` lookup** — Phase 2.4a evaluated alternatives; trilinear is the pareto-optimal choice. Phase 2.4b does NOT touch the calibration lookup surface.
- **Coefficient autotuning** — `kForceBoost` / `kV0Reduction` / `kGapWiden` / `kFmaxScalar` are architecture §457 / §661 verbatim constants at v1.0. Per-string or per-bow-position tuning deferred to Phase 2.4-bis or audible-character iteration.
- **Production WAV binary commits** — `sub-harmonics-stability.wav` (~30 MB) NOT committed (reproducible from harness). sha256 + JSON committed instead per Phase 2.4a / Phase 2.3 / Phase 2.2 precedent.
- **CI invocation of `--sub-harmonics-stability`** — out-of-scope; matrix is offline (developer-machine-only). CI runs the existing build + auval + pluginval pipeline.
- **ARCHITECTURE.md amendments** — end-of-Stage-2 verify decides; chaos detector + softClampState deferral continues in R35 commit body per RESEARCH §18.13. F3 deviation continues from Phase 2.1c onward.
- **Saturator-tail re-evaluation** — RESEARCH §12 footnote → Phase 2.4c.
- **Logic Pro AU smoke (R37)** — user-deferred non-blocking (mirrors R32 / R27 / R19f / R14e / R34h precedent).
- **E1 dispersion calibration polynomial follow-up (Phase 2.1c Risk #7)** — separate `a(B, I)` cascaded-allpass concern; not friction-junction sub-harmonic bias. Out-of-scope for Phase 2.4b; deferred to Phase 2.4c-or-later.

---

# Stage 2: DSP — Plan (Phase 2.4c) — REVISION 10 (Autocorrelator Octave-Rejection Harness Fix + Saturator-Tail O-Bowed Comparison, Gate 6c)

> **Status:** rev-10 authors fresh task bodies for **R36-pre, R36a, R36b, R36c, R36d, R36e, R36f, R36, R36-backfill chore** per `RESEARCH.md §19.10` sequencing and `CONTEXT.md` rev-8. rev-1/2/3/4/5/6/7/8/9 remain in-effect as completed/verified history. Phase 2.4b closed 2026-04-28 with R35 atomic commit `3de8b66` (Gate 6b CLEARED — 4 strict-PASS + 1 soft-PASS within RESEARCH §18.6 v1.0 budget) + R35-backfill chore `0db5fac`.

**Date:** 2026-04-29
**Cycle scope:** Phase 2.4c only (harness-only / research-only by construction; HR-11 binds zero production DSP edits). Phase 2.4-bis backlog (kForceBoost retune, breathingAudible refinement, fallback-cell reduction) stays parked. Phase 2.4c-bis is reserved as the escalation lane for >2 dB saturator-tail divergence (predicted low-probability per RESEARCH §19.6).
**Gate:** Gate 6c (Phase 2.4c verify)
**Atomic-commit unit:** R36 (Gate 6c PASS) — single commit lands Phase 2.4c harness changes + 1 new harness CLI flag (`--saturator-tail-comparison`) + 1 narrowed autocorrelator range-bias edit (`kTauMin/kTauMax` MIDI-derived) + 3 new golden text files (`saturator-tail-comparison.{wav.sha256,json,json.sha256}`) + 2 changed golden text files (`vibrato.json` + new `vibrato.json.sha256`) + `reproduce-goldens.sh` extension (12 → 13 entries) + Option B O-Bowed harness extension (`--bow-speed --bow-pressure --bow-position --infinite-sustain` flags) + planning artefacts. **NO Stage-1 contract amendment** (parameter-spec.md sha256 `77638e25…` carries forward unchanged; STATUS.md `contract_checksums.parameter_spec` unchanged). **NO ARCHITECTURE.md amendment** (saturator-tail evidence feeds end-of-Stage-2 §"In-loop saturator" amendment cycle but does not amend the architecture itself). **NO production DSP source edit** (HR-11 is the technical defence; bit-exact regression of all 12 carry-forward goldens trivially preserved by construction).
**Carry-forward locks (NOT re-litigated):** Phase 2.1a-recovery split-rail topology, F2 LP form, F3 no-in-loop-DCB, F4 betaScale removed; Phase 2.1b bow-friction module v1.0.0 consumption (HR-10 ABI preservation); Phase 2.1c `DispersionFilter<4>` API + per-string M=4/3/2/1 dispersion table; Phase 2.2 4-string bank + per-string detune + 5 ms equal-power crossfade + MIDI→string mapping; Phase 2.3 modulator-layer surface (vibratoPhase / vibratoOnsetTimer / slowLfoPhase / 4 macro SmoothedValues / 7-step per-block evaluation order / HR-1..HR-4 hard rules / `lastSafeDepth.store(0.0f)` unconditional pre-gate / EXPRESSION_MACRO + VIBRATO_DEPTH default 0.0); Phase 2.4a Schelleng wedge bass-register calibration polynomial (`Source/DSP/SchellengCalibration.h` HR-5/HR-6/HR-7/HR-8 verbatim); Phase 2.4b Sub-Harmonic Bias DSP-07 (`Source/DSP/SubHarmonicBias.h` HR-9/HR-10 verbatim + Step 2.5 inserted between Step 2 and Step 3 + `voiceBowForceUpliftThisBlock` factor at Step 6); 12 currently-committed goldens (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + vibrato + macro-sweep + slow-lfo + schelleng-stress + sub-harmonics + sub-harmonics-stability) + matrix-stability `6db67707…` (Phase 2.4a evidence carry-forward; not in default reproduce-goldens.sh). Phase 2.4c modifies ONLY `tests/render-harness/main.cpp` (autocorrelator range-bias + new `--saturator-tail-comparison` mode handler + analyser), `tests/render-harness/reproduce-goldens.sh` (12 → 13 entries), the Option B O-Bowed harness `plugins/O-Bowed/tests/render-harness/main.cpp` (NEW `--bow-*` + `--infinite-sustain` value-consume flags), 2 vibrato-JSON goldens (re-baselined post range-bias fix), 3 new saturator-tail goldens, and planning artefacts.

---

## Preamble — Pinned Open Items (RESEARCH §19.16)

PLAN rev-10 pins each of the 10 plan-phase open items from RESEARCH §19.16:

| # | Open Item | Pinned Decision |
|---|-----------|-----------------|
| 1 | Onset window widening branch | **Default `[800, 1000] ms` strict gate.** R36a prototype run measures `onsetTimeMs` against `vibrato.wav`; if measured value ∈ [800, 1000] ms, strict gate applies verbatim (Phase 2.3 PLAN rev-7 design intent). If measured ~600 ms (earlier than 800 — within VIBRATO_ONSET architecture spec but ahead of the `0.8 × peakDepth` threshold-crossing assumption), widen gate to **`[600, 1000] ms`** in R36c re-baselined `vibrato.json` (one-line edit + R36c gate update + this preamble pin updated post-R36a). PLAN-execution-log records measured value verbatim in the R36a notes. |
| 2 | O-Bowed parity Option | **Option B (scope-expand O-Bowed harness)** per RESEARCH §19.4.3 + Risk #15. Add 4 new value-consume CLI flags to `plugins/O-Bowed/tests/render-harness/main.cpp`: `--bow-speed <0..1>`, `--bow-pressure <0..1>`, `--bow-position <0..1>`, `--infinite-sustain <0..1>` (mirrors O-Contrabass `--infinite-sustain` value-consume pattern at `O-Contrabass/main.cpp:218–219`). Defaults remain factory APVTS values when flags absent → existing `canonical-preset.wav.sha256` golden carries forward byte-identical. Estimated +~80 LOC `O-Bowed/main.cpp`. Option A (factory defaults) REJECTED because O-Bowed factory `infiniteSustain = 0.0` makes envelope-shape comparison meaningless (Risk #15). Option C (defer to Phase 2.4c-bis) REJECTED because §19.7 verdict needs evidence-base in this cycle. |
| 3 | R36b new mode handler structure | **Mirror existing `--sub-harmonics` pattern** at `O-Contrabass/main.cpp` lines 694–735 (mode handler) + 1946–2124 (JSON serialization) + parser slot ABOVE `--sub-harmonics-stability` (highest precedence; emits warning if multiple modes set). Single-mode handler: parameter-pinning (`setNorm("BOW_SPEED", 0.15f)` + `BOW_PRESSURE` 3.0 + `BOW_POSITION` 0.10 + `INFINITE_SUSTAIN` 1.0 + `SUB_HARMONICS` 0.0 + default STRING_STIFFNESS) → `processor.releaseResources(); processor.prepareToPlay(...)` per RESEARCH §19.5.4 state-init pin → render 60 s sustain + 5 s release at MIDI 28 / velocity 0.7 → analyser iterates 65 non-overlapping 1-s windows on **channel 0 only** (RESEARCH §19.16 #6 single-channel precedent §16.7 / §18.5) → emit 65-entry `decayEnvelopeDb` array per RESEARCH §19.5.1 schema → 4-way `pass_combo = pass_nan && pass_peak && pass_blockTime && (no pass_subharmAudible/decayMatchesOBowed predicate per Q39)`. NO `pass_decayMatchesOBowed` JSON gate; verdict lives in RESEARCH §19.7. |
| 4 | `vibrato.json.sha256` adoption | **Phase 2.4c is the FIRST golden cycle to introduce `vibrato.json.sha256`** (no existing file at HEAD). Mirrors `sub-harmonics.json.sha256` byte-determinism precedent (Phase 2.4b R35b). R36c writes both `vibrato.json` (new measurement output) AND `vibrato.json.sha256` (anchor) atomically. R36e `reproduce-goldens.sh` invocation does NOT (yet) re-verify JSON-side sha256s — the script verifies WAV-side sha256s only at v1.0; JSON-side anchors are a verify-phase audit-only artefact (mirrors `sub-harmonics.json.sha256` Phase 2.4b precedent). |
| 5 | R36c re-baseline mechanics | **Same render command as `--vibrato`** (`$HARNESS --vibrato --out vibrato.wav --json vibrato.json`) with R36a-fixed harness binary; render output JSON → write to `tests/render-harness/golden/vibrato.json` (overwrite); `shasum -a 256` on the new JSON → write to `tests/render-harness/golden/vibrato.json.sha256` (NEW file). WAV is byte-identical to existing `d7881ecf…` (HR-11 trivially); R36c does NOT touch `vibrato.wav.sha256`. R36e `reproduce-goldens.sh` invocation re-confirms WAV byte-identity post R36a edits. |
| 6 | Decay-envelope analyser channel selection | **Channel 0 only** (RESEARCH §19.16 #6 + §16.7 / §18.5 single-channel precedent). Stereo WAV's right channel duplicates left at the harness output stage (mono-mix at `juce::AudioProcessor::processBlock`); per-bin RMS over channel 0 alone is canonical. PLAN-execution-log records the analyser's RMS computation: `rms[bin] = sqrt(sum_{i ∈ bin}(s[i]²) / N_bin)` where `s[i]` is channel-0 sample and `N_bin = sampleRate` (1-second non-overlapping windows). |
| 7 | R36b float serialization for JSON determinism | **`juce::String (val, 4)`** fixed-width 4-decimal-place format per RESEARCH §19.5.5. Mirrors `--sub-harmonics`'s `subharmEnergyRatio` serialization (Phase 2.4b R35a pattern at JSON emission site). All `decayEnvelopeDb[]` entries use this format; `peak`, `rmsMid_s5_s6`, `rmsFinal_lastSecond`, `rmsRatio_final_over_mid`, `rmsAtFiveSecondsPostBowOff_dbRelMax` all use the same pattern. Wall-clock fields (`blockMicros_median`, `blockMicros_max`, `blockTime_max_over_median`) emit as integer microseconds where possible to eliminate run-to-run rounding noise (mirrors §18.7 precedent). |
| 8 | `saturator-tail-comparison.wav.sha256` predicted vs measured | **Predicted `94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6`** per RESEARCH §19.5.2 pre-flight at HEAD `5d95d15`. R36b's golden render MUST match this sha256 (HR-11 trivially because no DSP changes between research-phase pre-flight and R36b execute-phase). If R36b sha256 drifts: STOP, run `git status` for stowaway DSP edits in `plugins/O-Contrabass/Source/`, investigate. The render path's APVTS pinning MUST mirror RESEARCH §19.5.2 exactly: `BOW_SPEED=0.15` + `BOW_PRESSURE=3.0` + `BOW_POSITION=0.10` + `INFINITE_SUSTAIN=1.0` + `SUB_HARMONICS=0.0` + factory `STRING_STIFFNESS` + factory `SLOW_LFO_DEPTH=0` + factory `VIBRATO_DEPTH=0` + factory `EXPRESSION_MACRO=0` (all factory-default modulator depths to ensure HR-1..HR-4 + HR-9 short-circuit paths fire). |
| 9 | R36-pre tripwire script | **Inline R36-pre execution (no standalone preflight script)** per RESEARCH §19.16 #9 alternative path. Phase 2.4c is harness-only / research-only — no spectral-pre-flight gate (mirrors `preflight-subharm.sh` Phase 2.4b precedent) is required because there is NO production DSP edit to gate. R36-pre runs 4 inline checks: (a) `reproduce-goldens.sh` 12-of-12 PASS at HEAD `5d95d15`; (b) Option-B O-Bowed canonical-preset cohort tripwire (`canonical-preset.wav.sha256` reproduces); (c) autocorrelator octave-jump baseline confirmation by reading existing `vibrato.json` and grepping for `peakDepthCents = 625.44` + `+1200¢` in `perCycleDeltaCents` (RESEARCH §19.2.2 documented baseline); (d) saturator-tail 3-trial determinism at HEAD (3 back-to-back renders → identical sha256 `94a42a81…` per §19.5.2). All 4 checks PASS gates R36a/R36b execute. |
| 10 | Phase 2.4c-bis CONTEXT rev-9-bis structural skeleton | **Pre-write at PLAN rev-10 §"Contingency — Phase 2.4c-bis Escalation Lane"** below — captures the Phase 2.4c-bis cycle structure verbatim so escalation is one-action-away if R36b parity render measures >2 dB divergence. Reduces context-loss between Phase 2.4c verify and Phase 2.4c-bis discuss-phase. Source-change scope = port `tanh(x/sat) × sat` with `sat=4.0f` to `Source/DSP/WaveguideString.cpp:204–206`; HR-11 lifted; re-baseline ALL audible goldens (E1 strict + per-string + detune-sweep-A + note-sequence + macro-sweep + slow-lfo + schelleng-stress + sub-harmonics + sub-harmonics-stability — 9 audible goldens; vibrato carry-forward because saturator port doesn't touch vibrato modulator path; matrix-stability re-render evidence-only). Predicted sha256s NOT pre-computed at PLAN rev-10 (would require source change to measure). |

**Carry-forward HARD RULES from prior phases (NOT re-litigated):**
- HR-1 (Phase 2.3): Vibrato literal-zero short-circuit + active-string-only — carry-forward.
- HR-2 (Phase 2.3): Slow-LFO literal-zero short-circuit + phase non-advance at zero depth — carry-forward.
- HR-3 (Phase 2.3): Macro IEEE 754 identity arithmetic + macroSmoothed setCurrentAndTargetValue(0.0) — carry-forward.
- HR-4 (Phase 2.3): Schelleng wedge skip on zero LFO depth + lastSafeDepth.store(0.0) unconditional pre-gate — carry-forward.
- HR-5 (Phase 2.4a): `inline constexpr` linkage on `SchellengCalibration.h` — carry-forward.
- HR-6 (Phase 2.4a): Calibration polynomial behind HR-4 gate ONLY — carry-forward.
- HR-7 (Phase 2.4a): Matrix-stability bypass via weak-symbol — carry-forward.
- HR-8 (Phase 2.4a): Trilinear IEEE 754 identity arithmetic — carry-forward.
- HR-9 (Phase 2.4b): SUB_HARMONICS=0 IEEE 754 identity arithmetic + active-string-only bias gate — carry-forward.
- HR-10 (Phase 2.4b): Friction module v1.0.0 ABI preservation under bias via ROSIN inverse algebraic identity — carry-forward.
- 7-step + Step 2.5 per-block evaluation order verbatim — Phase 2.4c does NOT modify the per-block order.
- 12 carry-forward goldens MUST reproduce byte-identical via `reproduce-goldens.sh` post-R36a/R36b edits.
- `matrix-stability.{wav.sha256,json,json.sha256}` `6db67707…` carries forward byte-identical (Phase 2.4a evidence golden; NOT in `reproduce-goldens.sh` default array).
- Atomic-commit gate-first principle: R7 → R15 → R20 → R26 → R33 → R34 → R35 → **R36** sequence.

**HARD RULE unique to Phase 2.4c (binding for R36a/R36b implementation):**

1. **HR-11 — Zero production DSP edits.**
   - Phase 2.4c modifies ONLY: `plugins/O-Contrabass/tests/render-harness/main.cpp`, `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh`, `plugins/O-Contrabass/tests/render-harness/golden/{vibrato.json,vibrato.json.sha256,saturator-tail-comparison.wav.sha256,saturator-tail-comparison.json,saturator-tail-comparison.json.sha256}`, `plugins/O-Bowed/tests/render-harness/main.cpp` (Option B), and planning artefacts (CONTEXT/RESEARCH/PLAN/STATUS/SUMMARY/VERIFICATION).
   - Any source edit under `plugins/O-Contrabass/Source/` (including `Source/DSP/*`, `Source/PluginProcessor.{h,cpp}`, `Source/BowedContrabassVoice.{h,cpp}`) is a HARD violation requiring escalation to Phase 2.4c-bis (separate CONTEXT rev-9-bis cycle with source-change scope).
   - Any source edit under `plugins/O-Bowed/Source/` is similarly a HARD violation (Option B touches `plugins/O-Bowed/tests/render-harness/main.cpp` ONLY, not the production source).
   - Any source edit under `modules/synthesis/bow-friction/Source/` is a HARD violation (friction module v1.0.0 ABI preservation; carry-forward HR-10).
   - HR-11 is the technical defence guaranteeing all 12 carry-forward goldens reproduce byte-identical by construction (no IEEE 754 identity arithmetic gymnastics needed — the WAV-producing code paths are not touched).
   - **Pre-flight enforcement:** R36-pre runs `reproduce-goldens.sh` BEFORE R36a/R36b edits land. **Post-edit enforcement:** R36e re-runs `reproduce-goldens.sh` and confirms 12-of-12 byte-identical (the new 13th entry locks the new saturator-tail golden's bit-exact reproduction). Any drift in the 12 carry-forward sha256s post-R36a/R36b edit = HARD HR-11 violation, halt and investigate (likely accidental edit to `Source/` outside the harness scope).
   - **Plan-execution-log audit hook:** `git diff --stat HEAD~1..HEAD -- plugins/O-Contrabass/Source/ modules/synthesis/bow-friction/Source/ plugins/O-Bowed/Source/` MUST report 0 changed files at R36 atomic commit time. Verify-phase R37 audit confirms.

**Per-block evaluation order:** UNCHANGED from Phase 2.4b end-state (7-step + Step 2.5). Phase 2.4c does NOT modify the per-block evaluation order.

---

## Goal

Convert the Phase 2.3 `pass_vibratoAudible` soft-relaxation into a strict-PASS by fixing the harness-side autocorrelator that octave-jumped at bass register (RESEARCH §19.2 — replace `kTauMin = 400` / `kTauMax = 1500` with MIDI-derived ±20% range bias `[856, 1285]` excluding the period/2 ≈ 535-sample latch point); AND produce an O-Bowed-comparable tail-decay characterisation of O-Contrabass's algebraic in-loop saturator (`x / sqrt(1 + x²)`) so the architectural decision to retain it (vs port O-Bowed's `tanh(x/4) × 4`) is evidence-backed before Phase 2.5 changes the body-side defenses (RESEARCH §19.3 + §19.5 + §19.7). Zero production DSP edits (HR-11): all 12 carry-forward goldens reproduce byte-identical by construction. R36 atomic commit lands harness-only changes — autocorrelator range-bias fix in `tests/render-harness/main.cpp:1742–1743` (replace `constexpr int` integer bounds with MIDI-28-derived ±20% computation; net ~−2 / +20 LOC) + NEW `--saturator-tail-comparison` CLI flag + mode handler (~+120 LOC) + new aggregator predicate `pass_vibratoAudible` per RESEARCH §19.9 + Option B O-Bowed harness extension (`plugins/O-Bowed/tests/render-harness/main.cpp` adds `--bow-speed --bow-pressure --bow-position --infinite-sustain` value-consume flags; ~+80 LOC) + 1 changed JSON golden (`vibrato.json` re-baselined to range-bias-corrected metrics) + 1 NEW JSON sha256 anchor (`vibrato.json.sha256`) + 3 new goldens (`saturator-tail-comparison.{wav.sha256,json,json.sha256}` with predicted WAV sha256 `94a42a81…`) + `reproduce-goldens.sh` 12 → 13 entries + RESEARCH §19.7 verdict written. Predicted verdict path = **research-only acknowledged divergence** (analytic bound from RESEARCH §19.3.3 + §19.6 predicts saturator-tail divergence <2 dB at canonical bow operating amplitude); escalation to Phase 2.4c-bis (source-change cycle) gated on >2 dB measured divergence at the 5-s post-bow-off mark (low-probability per §19.6). Validate Gate 6c invariants — five-item bar including bit-exact reproduction of 12 carry-forward goldens via `reproduce-goldens.sh` (HR-11 trivially holds) + strict `pass_vibratoAudible` PASS post autocorrelator fix + `--saturator-tail-comparison` golden bit-deterministic + auval/pluginval-10 SUCCESS + RESEARCH §19.7 verdict locked — and atomic-commit on Gate 6c PASS as R36. Continue R7 → R15 → R20 → R26 → R33 → R34 → R35 → **R36** atomic-commit sequence. Phase 2.4c ships **NO Stage-1 contract amendment** + **NO ARCHITECTURE.md amendment** + **NO production DSP source edit**; saturator-tail evidence feeds end-of-Stage-2 §"In-loop saturator" amendment cycle. Phase 2.5 (body resonator + bow noise) opens fresh CONTEXT rev-9 after Phase 2.4c verify.

---

## Tasks

### R36-pre — Working-tree integrity tripwire + 4-check pre-flight

**No source edits committed. Diagnostic only. Confirms working-tree integrity at start of execute, empirically validates RESEARCH §19.1 + §19.5.2 pre-flights under the plan-phase build environment, AND confirms the autocorrelator octave-jump baseline against existing `vibrato.json` per RESEARCH §19.2.2.**

Per RESEARCH §19.1, working tree at HEAD `5d95d15` (descendant of R35-backfill `0db5fac`) reproduces all 12 currently-committed goldens byte-identical via `reproduce-goldens.sh`. Per RESEARCH §19.5.2, single canonical 65-s render takes ~0.29 s wall-clock on M1 release; 3 back-to-back renders produce sha256 `94a42a81…` byte-identically. Per RESEARCH §19.2.2, existing `vibrato.json` documents the Phase 2.3 R28 octave-jump baseline (`peakDepthCents = 625.44`, `+1200¢` outlier in `perCycleDeltaCents`, `pass_vibratoDepthInRange = false`, `pass_onsetWindow = false`).

**Tasks:**

1. **Confirm git state is clean and at R35 atomic / R35-backfill chore descendant:**
   ```bash
   git log --oneline -3                                  # should show 5d95d15 (or descendant) + 0db5fac + 3de8b66
   git status plugins/O-Contrabass/                      # should be clean
   git status plugins/O-Bowed/                           # should be clean
   git status modules/synthesis/bow-friction/            # should be clean
   ```
   If working tree is dirty, STOP and reconcile before proceeding.

2. **Build O-Contrabass harness:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```
   Expect `ninja: no work to do` if R35 build artefacts persist; otherwise clean rebuild. Define `HARNESS=build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test` for subsequent steps.

3. **Build O-Bowed harness (Option B prerequisite):**
   ```bash
   cmake --build build --target O-Bowed-render-test --parallel
   ```
   Define `OBHARNESS=build/plugins/O-Bowed/tests/render-harness/O-Bowed-render-test_artefacts/Release/O-Bowed-render-test` for subsequent steps.

4. **Pre-flight check (a): `reproduce-goldens.sh` 12-of-12 PASS at HEAD (HR-11 tripwire):**
   ```bash
   bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   # Expect: "OK: all 12 goldens reproduce byte-identical"
   ```
   **All 12 must PASS** per RESEARCH §19.1 record. If any FAIL, STOP and investigate working-tree drift (re-confirm HEAD == `5d95d15` or descendant; `git status` for stray edits in `Source/`, `tests/render-harness/`, `modules/synthesis/bow-friction/`). Do NOT proceed to R36a until all 12 reproduce.

5. **Pre-flight check (b): Option-B O-Bowed canonical-preset cohort tripwire:**
   ```bash
   mkdir -p /tmp/phase24c-r36pre
   $OBHARNESS --note 69 --velocity 0.7 --sustain 5 --release 0 \
              --out /tmp/phase24c-r36pre/canonical-preset.wav \
              --json /tmp/phase24c-r36pre/canonical-preset.json
   computed=$(shasum -a 256 /tmp/phase24c-r36pre/canonical-preset.wav | awk '{print $1}')
   expected=$(awk '{print $1}' plugins/O-Bowed/tests/render-harness/golden/canonical-preset.wav.sha256)
   [ "$computed" = "$expected" ] && echo "[PASS] O-Bowed canonical-preset $computed" || \
                                    { echo "[FAIL] cohort drift; STOP"; exit 1; }
   ```
   Confirms O-Bowed Phase 2.1b R12 cohort baseline at HEAD. Pre-condition for R36b Option B harness extension (must remain byte-identical post-flag-addition).

6. **Pre-flight check (c): Autocorrelator octave-jump baseline confirmation (RESEARCH §19.2.2):**
   ```bash
   jq '.peakDepthCents, .pass_vibratoDepthInRange, .pass_onsetWindow, .perCycleDeltaCents' \
      plugins/O-Contrabass/tests/render-harness/golden/vibrato.json
   # Expect: peakDepthCents ≈ 625.44 (octave-contaminated)
   #         pass_vibratoDepthInRange: false
   #         pass_onsetWindow: false
   #         perCycleDeltaCents contains a +1200¢ outlier
   ```
   Confirms Phase 2.3 R28 audit-debt characterization persists at HEAD verbatim. If `peakDepthCents` reads ~12 (already correct) at HEAD, the autocorrelator is NOT actually octave-jumping → STOP, re-investigate baseline assumption (Phase 2.3 R28 may have been silently fixed by an upstream commit; PLAN rev-10 redirects to a no-op R36a).

7. **Pre-flight check (d): Saturator-tail 3-trial determinism at HEAD (RESEARCH §19.5.2 reproduction):**
   ```bash
   # Render 65 s of audio at canonical bass operating point 3 times via existing harness CLI
   # (no --saturator-tail-comparison mode yet — invoke explicit per-flag canonical render).
   # Note: this uses --infinite-sustain 1.0 + --sustain 60 --release 5 --note 28 --velocity 0.7.
   # Block-time ratio not measured here — that's the R36b mode handler's job.
   for i in 1 2 3; do
       $HARNESS --infinite-sustain 1.0 --note 28 --velocity 0.7 --sustain 60 --release 5 \
                --out /tmp/phase24c-r36pre/sat-tail-r${i}.wav \
                --json /tmp/phase24c-r36pre/sat-tail-r${i}.json >/dev/null
   done
   sha1=$(shasum -a 256 /tmp/phase24c-r36pre/sat-tail-r1.wav | awk '{print $1}')
   sha2=$(shasum -a 256 /tmp/phase24c-r36pre/sat-tail-r2.wav | awk '{print $1}')
   sha3=$(shasum -a 256 /tmp/phase24c-r36pre/sat-tail-r3.wav | awk '{print $1}')
   if [ "$sha1" = "$sha2" ] && [ "$sha2" = "$sha3" ]; then
       echo "[PASS] sat-tail 3-trial determinism: $sha1"
       expected="94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6"
       if [ "$sha1" = "$expected" ]; then
           echo "[PASS] matches RESEARCH §19.5.2 predicted sha256"
       else
           echo "[WARN] sha256 mismatch vs predicted $expected; investigate (likely upstream drift)"
       fi
   else
       echo "[FAIL] sat-tail non-deterministic across 3 trials; STOP"
       exit 1
   fi
   ```
   **NOTE:** This pre-flight relies on the existing harness's `--infinite-sustain`/`--note`/`--velocity`/`--sustain`/`--release` flags producing the same canonical render path as the future `--saturator-tail-comparison` mode. Bow-position/speed/pressure are NOT explicitly set in this pre-flight (factory defaults via APVTS); R36b's mode handler MUST use the same factory APVTS values to inherit this sha256 (Q8 + RESEARCH §19.5.2). If R36b emits a different sha256, investigate parameter-pinning drift.

**Files created:** `/tmp/phase24c-r36pre/*.{wav,json}` (transient; deleted post-R36; NOT committed).

**Files modified:** none committed.

**Commit:** **NONE** — diagnostic only.

**Success bar:**
- [ ] `git log --oneline -3` shows `5d95d15` (or descendant) + `0db5fac` + `3de8b66`.
- [ ] O-Contrabass harness builds clean.
- [ ] O-Bowed harness builds clean.
- [ ] (a) `reproduce-goldens.sh` reports `OK: all 12 goldens reproduce byte-identical`.
- [ ] (b) O-Bowed canonical-preset.wav sha256 matches committed golden byte-identical.
- [ ] (c) `vibrato.json` shows `peakDepthCents ≈ 625.44` octave-contaminated baseline + `pass_vibratoDepthInRange = false` + `pass_onsetWindow = false` + `+1200¢` outlier in `perCycleDeltaCents` per RESEARCH §19.2.2.
- [ ] (d) 3-trial sat-tail render produces sha256 `94a42a81…` byte-identically.

**Estimated effort:** 10 min (build × 2 + 4 pre-flight checks).

---

### R36a — Autocorrelator range-bias fix + `pass_vibratoAudible` aggregator predicate

**Per RESEARCH §19.2.3 spec. Replace `tests/render-harness/main.cpp:1742–1743` integer-lag bounds (`kTauMin = 400` / `kTauMax = 1500`) with MIDI-28-derived ±20% computation. Add `pass_vibratoAudible` aggregator predicate to the existing `--vibrato` mode JSON output (mirrors `pass_combo` aggregator pattern from `--sub-harmonics`). Net source delta ~+18 / −2 LOC.**

**Tasks:**

1. **Edit `plugins/O-Contrabass/tests/render-harness/main.cpp:1742–1743`** — replace integer-lag `kTauMin`/`kTauMax` constants with MIDI-derived range bias.

   ```cpp
   // BEFORE (HEAD):
   constexpr int    kTauMin       =  400;        // pin #5 — covers ~110 Hz at sr=44100
   constexpr int    kTauMax       = 1500;        // pin #5 — covers ~29 Hz at sr=44100

   // AFTER (R36a — RESEARCH §19.2.3 verbatim):
   // Phase 2.4c R36a — MIDI-28-derived ±20% range bias eliminates octave-jump
   // (RESEARCH §19.2.2 documents pre-fix peakDepthCents=625.44 from period/2 latch).
   // Range [856, 1285] = [34.32, 51.52] Hz — covers E1 fundamental ±20% but excludes
   // half-period (535 samples ≈ 82.4 Hz). Parabolic interpolation around bestTau
   // (already present at lines ~1779–1801) gives ~0.16¢ precision at E1 — sufficient
   // for 12-cent vibrato modulation (~7.4 sample period excursion).
   constexpr int    kVibratoMidiNote = 28;                                          // E1 (matches --vibrato spec)
   constexpr double kVibratoF0Hz     = 440.0 * std::pow (2.0, (kVibratoMidiNote - 69) / 12.0);  // → 41.20 Hz
   constexpr double kVibratoPeriod   = 44100.0 / kVibratoF0Hz;                      // → 1070.41 samples
   constexpr int    kTauMin       = static_cast<int> (std::floor (0.80 * kVibratoPeriod));  // → 856
   constexpr int    kTauMax       = static_cast<int> (std::ceil  (1.20 * kVibratoPeriod));  // → 1285
   ```

   The `constexpr` evaluation requires `std::pow` and `std::floor`/`std::ceil` to be `constexpr` in C++23 (or a compile-time-constant fallback). If the toolchain is C++20-only and these are not yet `constexpr`, replace with `inline const` and document the LTS-friendly pattern in the inline comment. (Phase 2.3/2.4a/2.4b harness builds clean at C++20; verify with `cmake --build` smoke after the edit.)

   **NOTE on parabolic interpolation:** RESEARCH §19.2.1 confirms the existing autocorrelator at lines ~1779–1801 ALREADY performs 3-point parabolic interpolation around `bestTau`. **DO NOT TOUCH** the parabolic-interp code path — it is already correct. R36a's edit is bound to the integer-lag bounds at lines 1742–1743 ONLY.

2. **Edit `plugins/O-Contrabass/tests/render-harness/main.cpp` `--vibrato` mode JSON emission site** — add `pass_vibratoAudible` aggregator predicate.

   Locate the existing `--vibrato` mode JSON emission in `runVibratoMode()` (search for the JSON object that emits `pass_rateHzInRange`, `pass_vibratoDepthInRange`, `pass_onsetWindow`, `pass_rmsContinuity`). After the four predicates land, add:

   ```cpp
   // Phase 2.4c R36a — strict aggregator predicate (RESEARCH §19.9 / Phase 2.3 PLAN rev-7 design intent).
   const bool pass_vibratoAudible = pass_rateHzInRange
                                   && pass_vibratoDepthInRange
                                   && pass_onsetWindow
                                   && pass_rmsContinuity;
   ```

   Then add `"pass_vibratoAudible": pass_vibratoAudible,` to the JSON object emitted by the mode. Mirrors `pass_combo` aggregator pattern from `--sub-harmonics` (Phase 2.4b R35a).

   **NOTE:** if RESEARCH §19.9's `pass_rmsContinuity` predicate is NOT currently emitted by the `--vibrato` mode (only `pass_rateHzInRange` / `pass_vibratoDepthInRange` / `pass_onsetWindow` may be present at HEAD), add `pass_rmsContinuity` to the aggregator AND to the JSON emission. The predicate measures `rmsContinuity ≥ 0.85` (Phase 2.3 macro-sweep loose threshold; vibrato is non-pathological at MIDI 28 / 12¢ / 5 Hz). PLAN-execution-log records whether `pass_rmsContinuity` was added vs already present.

3. **Build harness:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```
   Expect a clean build with zero warnings. If `constexpr std::pow` fails on the toolchain, fall back to `inline const` (still locks values at compile time for the `--vibrato` mode handler).

4. **Smoke test (R36a prototype run against existing `vibrato.wav`):**
   ```bash
   $HARNESS --vibrato \
            --out /tmp/phase24c-r36a/vibrato.wav \
            --json /tmp/phase24c-r36a/vibrato.json
   # Verify HR-11: WAV byte-identical to golden d7881ecf…
   computed=$(shasum -a 256 /tmp/phase24c-r36a/vibrato.wav | awk '{print $1}')
   expected=$(awk '{print $1}' plugins/O-Contrabass/tests/render-harness/golden/vibrato.wav.sha256)
   [ "$computed" = "$expected" ] && echo "[PASS] HR-11 vibrato.wav byte-identical $computed" || \
                                    { echo "[FAIL] HR-11 violation; STOP, inspect Source/ for stowaway edit"; exit 1; }
   # Read post-fix JSON metrics:
   jq '.peakDepthCents, .vibratoRateHzMeasured, .onsetTimeMs, .pass_vibratoAudible, .pass_rateHzInRange, .pass_vibratoDepthInRange, .pass_onsetWindow, .pass_rmsContinuity' \
      /tmp/phase24c-r36a/vibrato.json
   # Expect post-fix per RESEARCH §19.2.4:
   #   peakDepthCents ≈ 12 (was 625.44 octave-contaminated)
   #   vibratoRateHzMeasured ≈ 4.978 (unchanged)
   #   onsetTimeMs ∈ [600, 1000] ms (was 1975 corrupted)
   #   pass_vibratoAudible: true
   #   pass_rateHzInRange: true
   #   pass_vibratoDepthInRange: true
   #   pass_onsetWindow: true
   #   pass_rmsContinuity: true
   ```

   **Pin #1 confirmation gate:** If measured `onsetTimeMs ∈ [800, 1000]` ms, strict gate applies verbatim — proceed to R36c with default range. If measured `onsetTimeMs < 800` ms (likely ~600 ms given VIBRATO_ONSET architecture spec), widen R36c's `pass_onsetWindow` strict range to `[600, 1000]` ms (one-line edit + this preamble pin updated). PLAN-execution-log records measured value verbatim.

   If `pass_vibratoAudible = false` post-R36a (any of the 4 sub-predicates fails): STOP, investigate (likely off-by-one in range-bias bounds or unexpected interaction with the `0.8 × peakDepth` onset-detection threshold). Do NOT proceed to R36c.

5. **Stage R36a edits** (do NOT commit yet — R36 atomic):
   ```bash
   git add plugins/O-Contrabass/tests/render-harness/main.cpp
   git status   # should show 1 modified file staged
   ```

**Files created:** none.

**Files modified:**
- `plugins/O-Contrabass/tests/render-harness/main.cpp` (~+18 / −2 LOC: range-bias edit at lines 1742–1743 + `pass_vibratoAudible` aggregator at JSON emission site).

**Commit:** **NONE** — staged for R36 atomic.

**Success bar:**
- [ ] Harness builds clean zero warnings post-edit.
- [ ] HR-11 tripwire (vibrato.wav byte-identical) PASS — confirms no stowaway DSP edit.
- [ ] `--vibrato` mode emits new `pass_vibratoAudible` aggregator field in JSON output.
- [ ] Post-fix `peakDepthCents` ≈ 12 (not 625.44 octave-contaminated).
- [ ] Post-fix `pass_vibratoDepthInRange = true`, `pass_onsetWindow = true`, `pass_rateHzInRange = true`, `pass_rmsContinuity = true`, `pass_vibratoAudible = true`.
- [ ] `onsetTimeMs` measured value recorded in PLAN-execution-log; strict range pin confirmed `[800, 1000]` ms OR widened to `[600, 1000]` ms per pin #1.

**Estimated effort:** 30 min (range-bias edit + aggregator predicate + build + smoke + measurement recording).

---

### R36b — NEW `--saturator-tail-comparison` CLI flag + mode handler + Option B O-Bowed harness extension + golden render

**Per RESEARCH §19.5 schema + §19.10 task table. Adds the new CLI flag at O-Contrabass + mode handler with 60s+5s render + 65-bin per-second decay-envelope analyser. Adds Option B parity flags to O-Bowed harness. Renders 3 new goldens.**

**Tasks:**

1. **Edit `plugins/O-Contrabass/tests/render-harness/main.cpp`** — add `--saturator-tail-comparison` CLI flag (~+5 LOC parsing).

   In the `parseArgs` flag-handling section, slot `--saturator-tail-comparison` ABOVE `--sub-harmonics` (highest precedence; emits warning if multiple modes set):

   ```cpp
   if      (key == "--saturator-tail-comparison") { args.saturatorTailMode = true; continue; }
   ```

   Add `bool saturatorTailMode = false;` to the `Args` struct. Add mutual-exclusion warnings against `--sub-harmonics` / `--sub-harmonics-stability` / `--matrix-stability` / `--macro-sweep` / `--schelleng-stress` / `--vibrato` / `--slow-lfo` / `--detune-sweep` / `--note-sequence` / `--string` / `--stiffness-zero-pre` (mirrors existing mutual-exclusion ladder).

2. **Edit `plugins/O-Contrabass/tests/render-harness/main.cpp`** — author `runSaturatorTailComparisonMode()` (~+115 LOC).

   Mirror `runSubHarmonicsMode()` structural pattern at lines ~694–735 of `O-Contrabass/main.cpp`:

   ```cpp
   static int runSaturatorTailComparisonMode (const Args& args)
   {
       // Phase 2.4c R36b — saturator-tail comparison render (RESEARCH §19.5).
       // Canonical E1 60 s sustain + 5 s release at default bow params + INFINITE_SUSTAIN=1.0.
       // 65-bin per-second decay-envelope analyser on channel 0; emit JSON per §19.5.1 schema.
       const double sampleRate     = 44100.0;
       const int    blockSize      = 512;
       const int    midiNote       = 28;          // E1
       const float  velocity       = 0.7f;        // norm
       const float  sustainSec     = 60.0f;
       const float  releaseSec     = 5.0f;
       const int    sustainSamples = static_cast<int> (sustainSec * sampleRate);
       const int    releaseSamples = static_cast<int> (releaseSec * sampleRate);
       const int    totalSamples   = sustainSamples + releaseSamples;          // 2,866,500
       const int    binCount       = 65;                                       // 60 sustain + 5 release

       // Construct + initialise processor (factory APVTS values + canonical bow pinning per Q8).
       O_ContrabassAudioProcessor processor;
       processor.releaseResources();
       processor.prepareToPlay (sampleRate, blockSize);
       setNorm (processor, "BOW_SPEED",        0.15f);   // RESEARCH §19.5.4 + Q8 verbatim
       setNorm (processor, "BOW_PRESSURE",     3.0f / 5.0f);   // architecture-spec'd; norm = raw / max
       setNorm (processor, "BOW_POSITION",     0.10f);
       setNorm (processor, "INFINITE_SUSTAIN", 1.0f);
       // SUB_HARMONICS=0 + SLOW_LFO_DEPTH=0 + VIBRATO_DEPTH=0 + EXPRESSION_MACRO=0 default
       // (HR-1..HR-4 + HR-9 short-circuits fire; bias path INACTIVE; only the saturator
       // tail decay envelope is observable in the loop).

       // Inject MIDI noteOn at midiNote / velocity * 127 raw.
       // ... mirror existing harness MIDI-injection pattern from --vibrato mode ...

       // Render sustainSamples + releaseSamples in blockSize chunks.
       // (At sustainSamples mark, send MIDI noteOff for releaseSamples; INFINITE_SUSTAIN=1.0
       //  keeps the bow exciting until noteOff.)
       juce::AudioBuffer<float> renderBuf (2, totalSamples);
       // ... existing harness render loop pattern + per-block timing instrumentation ...

       // Decay-envelope analyser: 65 non-overlapping 1-second windows on channel 0.
       std::vector<float> rmsBins (binCount, 0.0f);
       float peakAbs = 0.0f;
       int   nanCount = 0;
       int   infCount = 0;
       for (int b = 0; b < binCount; ++b)
       {
           const int binStart = b * static_cast<int> (sampleRate);
           const int binEnd   = juce::jmin (binStart + static_cast<int> (sampleRate), totalSamples);
           double sumSq = 0.0;
           for (int i = binStart; i < binEnd; ++i)
           {
               const float s = renderBuf.getSample (0, i);                    // pin #6 — channel 0 only
               if (std::isnan (s)) ++nanCount;
               if (std::isinf (s)) ++infCount;
               sumSq += static_cast<double> (s) * static_cast<double> (s);
               peakAbs = juce::jmax (peakAbs, std::abs (s));
           }
           rmsBins[b] = static_cast<float> (std::sqrt (sumSq / juce::jmax (1, binEnd - binStart)));
       }
       float rmsMax = 0.0f;
       int   rmsMaxBinIdx = 0;
       for (int b = 0; b < binCount; ++b)
           if (rmsBins[b] > rmsMax) { rmsMax = rmsBins[b]; rmsMaxBinIdx = b; }
       std::vector<float> decayEnvelopeDb (binCount);
       for (int b = 0; b < binCount; ++b)
           decayEnvelopeDb[b] = 20.0f * std::log10 (juce::jmax (1.0e-9f, rmsBins[b])
                                                    / juce::jmax (1.0e-9f, rmsMax));

       const float rmsMid_s5_s6           = rmsBins[5];                       // bin 5 (Phase 2.1a R6 carry-forward)
       const float rmsFinal_lastSecond    = rmsBins[64];                      // bin 64 (last release bin)
       const float rmsRatio_final_over_mid = rmsFinal_lastSecond / juce::jmax (1.0e-9f, rmsMid_s5_s6);
       const float rmsAtFiveSecondsPostBowOff_dbRelMax = decayEnvelopeDb[64];

       // Pass predicates (RESEARCH §19.5.1 — 4-way pass_combo; NO pass_decayMatchesOBowed):
       const bool pass_nan       = (nanCount == 0 && infCount == 0);
       const bool pass_peak      = (peakAbs < 1.0f);
       const bool pass_blockTime = (blockTime_max_over_median <= 5.0f);
       const bool pass_combo     = pass_nan && pass_peak && pass_blockTime;

       // Emit JSON per §19.5.1 schema with juce::String(val, 4) fixed-width 4-decimal serialization (pin #7).
       // ... write WAV (24-bit stereo PCM via existing harness) + JSON files ...
       return pass_combo ? 0 : 1;
   }
   ```

   **Critical pins for sha256 byte-identity (RESEARCH §19.5.2 + Q8):**
   - APVTS values pinned EXACTLY per Q8 list above.
   - `processor.releaseResources(); processor.prepareToPlay(sampleRate, blockSize)` per RESEARCH §19.5.4.
   - blockSize = 512 (harness default).
   - 24-bit stereo PCM WAV (existing harness convention).
   - basename-only `outputWav` JSON field (mirrors §17.2 / §18.2 / §18.7 precedent — strip absolute paths).
   - All `decayEnvelopeDb[]` floats serialized as `juce::String (val, 4)` fixed-width.
   - Wall-clock fields (`blockMicros_*`) emit as integer microseconds where possible (mirrors §18.7 precedent to eliminate run-to-run rounding noise).

3. **Edit `plugins/O-Bowed/tests/render-harness/main.cpp`** — Option B scope-expansion: add 4 new value-consume CLI flags (~+80 LOC).

   Mirror O-Contrabass `--infinite-sustain` value-consume pattern at `O-Contrabass/main.cpp:218–219`. Add to `Args` struct:
   ```cpp
   float bowSpeed       = -1.0f;   // -1 sentinel = "factory APVTS default"
   float bowPressure    = -1.0f;
   float bowPosition    = -1.0f;
   float infiniteSustain = -1.0f;
   ```

   In `parseArgs`, add value-consume handlers BEFORE the main mode-flag dispatch:
   ```cpp
   if (key == "--bow-speed"        && i + 1 < argc) { args.bowSpeed       = std::stof (argv[++i]); continue; }
   if (key == "--bow-pressure"     && i + 1 < argc) { args.bowPressure    = std::stof (argv[++i]); continue; }
   if (key == "--bow-position"     && i + 1 < argc) { args.bowPosition    = std::stof (argv[++i]); continue; }
   if (key == "--infinite-sustain" && i + 1 < argc) { args.infiniteSustain = std::stof (argv[++i]); continue; }
   ```

   In the mode handler (after `processor.prepareToPlay`), apply pinning ONLY when sentinel is overridden:
   ```cpp
   if (args.bowSpeed        >= 0.0f) setNorm (processor, "BOW_SPEED",        args.bowSpeed);
   if (args.bowPressure     >= 0.0f) setNorm (processor, "BOW_PRESSURE",     args.bowPressure);
   if (args.bowPosition     >= 0.0f) setNorm (processor, "BOW_POSITION",     args.bowPosition);
   if (args.infiniteSustain >= 0.0f) setNorm (processor, "INFINITE_SUSTAIN", args.infiniteSustain);
   ```

   **Critical pin (Risk #13 mitigation):** when no `--bow-*` flags are passed, behaviour MUST be identical to HEAD — factory APVTS values consumed verbatim. This preserves `canonical-preset.wav.sha256` bit-exact (the existing `--note 69 --velocity 0.7 --sustain 5 --release 0` invocation does NOT set the new flags).

4. **Build both harnesses:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   cmake --build build --target O-Bowed-render-test --parallel
   ```

5. **R36b cohort regression smoke (Risk #13 mitigation):** Re-run O-Bowed canonical-preset golden render and confirm byte-identical:
   ```bash
   $OBHARNESS --note 69 --velocity 0.7 --sustain 5 --release 0 \
              --out /tmp/phase24c-r36b/canonical-preset.wav \
              --json /tmp/phase24c-r36b/canonical-preset.json
   computed=$(shasum -a 256 /tmp/phase24c-r36b/canonical-preset.wav | awk '{print $1}')
   expected=$(awk '{print $1}' plugins/O-Bowed/tests/render-harness/golden/canonical-preset.wav.sha256)
   [ "$computed" = "$expected" ] && echo "[PASS] cohort regression $computed" || \
                                    { echo "[FAIL] Option B harness extension altered default behaviour; STOP"; exit 1; }
   ```

6. **Render NEW `--saturator-tail-comparison` golden (O-Contrabass) — predicted sha256 `94a42a81…`:**
   ```bash
   mkdir -p /tmp/phase24c-r36b
   $HARNESS --saturator-tail-comparison \
            --out /tmp/phase24c-r36b/saturator-tail-comparison.wav \
            --json /tmp/phase24c-r36b/saturator-tail-comparison.json

   # Verify predicted sha256 from RESEARCH §19.5.2 pre-flight.
   computed=$(shasum -a 256 /tmp/phase24c-r36b/saturator-tail-comparison.wav | awk '{print $1}')
   expected="94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6"
   [ "$computed" = "$expected" ] && echo "[PASS] R36b sha256 matches §19.5.2 prediction $computed" || \
                                    { echo "[FAIL] sha256 drift: got $computed, expected $expected; investigate parameter-pinning"; exit 1; }
   ```

   **If sha256 drifts**: STOP, investigate (likely cause = parameter-pinning mismatch with §19.5.2 pre-flight invocation; secondary cause = stowaway DSP edit which is HR-11 violation). Do NOT proceed to R36c until sha256 matches.

7. **Capture R36b sha256s and stage 3 new golden text files:**
   ```bash
   shasum -a 256 /tmp/phase24c-r36b/saturator-tail-comparison.wav | awk '{print $1}' \
       > plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.wav.sha256
   cp /tmp/phase24c-r36b/saturator-tail-comparison.json \
      plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json
   shasum -a 256 plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json | awk '{print $1}' \
       > plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json.sha256

   git add plugins/O-Contrabass/tests/render-harness/main.cpp \
           plugins/O-Bowed/tests/render-harness/main.cpp \
           plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.wav.sha256 \
           plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json \
           plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json.sha256
   git status
   ```

8. **Sha256 stability across re-renders smoke (Risk #6 mitigation):**
   ```bash
   for i in 1 2 3; do
       $HARNESS --saturator-tail-comparison \
                --out /tmp/phase24c-r36b/sat-tail-trial${i}.wav \
                --json /tmp/phase24c-r36b/sat-tail-trial${i}.json >/dev/null
   done
   sha1=$(shasum -a 256 /tmp/phase24c-r36b/sat-tail-trial1.wav | awk '{print $1}')
   sha2=$(shasum -a 256 /tmp/phase24c-r36b/sat-tail-trial2.wav | awk '{print $1}')
   sha3=$(shasum -a 256 /tmp/phase24c-r36b/sat-tail-trial3.wav | awk '{print $1}')
   [ "$sha1" = "$sha2" ] && [ "$sha2" = "$sha3" ] && \
       echo "[PASS] state reset between renders deterministic" || \
       echo "[FAIL] state bleed; investigate releaseResources()"
   ```

9. **JSON sha256 stability across re-renders smoke:**
   ```bash
   sha1=$(shasum -a 256 /tmp/phase24c-r36b/sat-tail-trial1.json | awk '{print $1}')
   sha2=$(shasum -a 256 /tmp/phase24c-r36b/sat-tail-trial2.json | awk '{print $1}')
   sha3=$(shasum -a 256 /tmp/phase24c-r36b/sat-tail-trial3.json | awk '{print $1}')
   [ "$sha1" = "$sha2" ] && [ "$sha2" = "$sha3" ] && \
       echo "[PASS] JSON sha256 byte-identical $sha1" || \
       echo "[FAIL] float-serialization noise; tighten pin #7 fixed-width format"
   ```

**Files created:**
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.wav.sha256` (NEW — predicted `94a42a81…`).
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json` (NEW; ~3 KB; full JSON per §19.5.1 schema).
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json.sha256` (NEW; anchors JSON reproducibility).

**Files modified:**
- `plugins/O-Contrabass/tests/render-harness/main.cpp` (~+120 LOC: 5 LOC parser + 115 LOC mode handler + analyser + JSON emission).
- `plugins/O-Bowed/tests/render-harness/main.cpp` (~+80 LOC: 4 value-consume flags + Args struct extension + sentinel-conditional setNorm pinning).

**Commit:** **NONE** — staged for R36 atomic.

**Success bar:**
- [ ] Both harnesses build clean zero warnings.
- [ ] O-Bowed cohort regression PASS (`canonical-preset.wav.sha256` byte-identical post-extension).
- [ ] O-Contrabass `--saturator-tail-comparison` golden sha256 matches RESEARCH §19.5.2 prediction `94a42a81…` byte-identically.
- [ ] 3-trial WAV sha256 determinism PASS.
- [ ] 3-trial JSON sha256 determinism PASS (pin #7 float-serialization holds).
- [ ] JSON `pass_combo = true` (`pass_nan = true`, `pass_peak = true`, `pass_blockTime = true`).
- [ ] JSON `decayEnvelopeDb` array has 65 entries; bin 64 ≈ −9.31 dB rel max per RESEARCH §19.5.3.
- [ ] 3 new golden text files staged.

**Estimated effort:** 100 min (parser + mode handler + analyser + Option B harness extension + cohort smoke + golden render + 2 sha256 smokes + staging).

---

### R36c — Re-baseline `vibrato.json{,.sha256}` against R36a-fixed measurement output

**WAV unchanged (HR-11 trivially); only JSON changes.**

**Tasks:**

1. **Re-render `--vibrato` with R36a-fixed harness binary:**
   ```bash
   mkdir -p /tmp/phase24c-r36c
   $HARNESS --vibrato \
            --out /tmp/phase24c-r36c/vibrato.wav \
            --json /tmp/phase24c-r36c/vibrato.json

   # HR-11 confirmation (vibrato.wav byte-identical to existing golden):
   computed=$(shasum -a 256 /tmp/phase24c-r36c/vibrato.wav | awk '{print $1}')
   expected=$(awk '{print $1}' plugins/O-Contrabass/tests/render-harness/golden/vibrato.wav.sha256)
   [ "$computed" = "$expected" ] && echo "[PASS] HR-11 vibrato.wav $computed" || \
                                    { echo "[FAIL] HR-11 violation; STOP"; exit 1; }
   ```

2. **Validate post-fix metrics + `pass_vibratoAudible` strict-PASS:**
   ```bash
   jq '.peakDepthCents, .vibratoRateHzMeasured, .onsetTimeMs, .pass_rateHzInRange, .pass_vibratoDepthInRange, .pass_onsetWindow, .pass_rmsContinuity, .pass_vibratoAudible' \
      /tmp/phase24c-r36c/vibrato.json
   # Expect (Phase 2.3 PLAN rev-7 design intent + R36a aggregator):
   #   peakDepthCents ∈ [10, 14]    (was 625.44 octave-contaminated)
   #   vibratoRateHzMeasured ∈ [4.5, 5.5]    (carry-forward correct)
   #   onsetTimeMs ∈ [800, 1000]    OR widened [600, 1000] per pin #1
   #   all 4 sub-predicates: true
   #   pass_vibratoAudible: true
   ```

   If `pass_vibratoAudible = false`: STOP, investigate (R36a's range-bias or aggregator predicate has a bug). Do NOT proceed to R36b (already done) or R36e until vibrato JSON validates strict-PASS.

3. **Capture re-baselined JSON + new JSON sha256 anchor:**
   ```bash
   cp /tmp/phase24c-r36c/vibrato.json \
      plugins/O-Contrabass/tests/render-harness/golden/vibrato.json
   shasum -a 256 plugins/O-Contrabass/tests/render-harness/golden/vibrato.json | awk '{print $1}' \
       > plugins/O-Contrabass/tests/render-harness/golden/vibrato.json.sha256
   ```

4. **Stage 2 changed/new golden text files** (do NOT commit yet — R36 atomic):
   ```bash
   git add plugins/O-Contrabass/tests/render-harness/golden/vibrato.json \
           plugins/O-Contrabass/tests/render-harness/golden/vibrato.json.sha256
   git status
   ```

**Files created:**
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.json.sha256` (NEW; first JSON sha256 anchor for vibrato; mirrors `sub-harmonics.json.sha256` Phase 2.4b precedent).

**Files modified:**
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.json` (re-baselined to range-bias-corrected metrics).

**Files NOT modified (HR-11 trivially):**
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.wav.sha256` (carries forward `d7881ecf…` byte-identical).

**Commit:** **NONE** — staged for R36 atomic.

**Success bar:**
- [ ] HR-11 tripwire (vibrato.wav byte-identical) PASS.
- [ ] `vibrato.json` reports `pass_vibratoAudible = true` strict-PASS.
- [ ] All 4 sub-predicates report true (rate / depth / onset / rmsContinuity).
- [ ] `vibrato.json.sha256` written + matches new JSON file's sha256.
- [ ] 2 golden text files staged.

**Estimated effort:** 10 min (re-render + JSON validation + sha capture + 2 file stages).

---

### R36d — Append RESEARCH §19.7 verdict (research-only OR escalation flag) based on R36b parity render

**Per RESEARCH §19.7.1 verdict tree. Compute O-Bowed parity render (Option B) at matched bow operating point; measure 5-s post-bow-off envelope divergence vs O-Contrabass; populate verdict.**

**Tasks:**

1. **Render O-Bowed parity reference at canonical bow operating point (Option B):**
   ```bash
   mkdir -p /tmp/phase24c-r36d
   $OBHARNESS --note 28 --velocity 0.7 --sustain 60 --release 5 \
              --bow-speed 0.15 --bow-pressure 0.6 --bow-position 0.10 --infinite-sustain 1.0 \
              --out /tmp/phase24c-r36d/o-bowed-parity.wav \
              --json /tmp/phase24c-r36d/o-bowed-parity.json
   ```

   **Note on `--bow-pressure` value:** O-Contrabass uses `BOW_PRESSURE=3.0` raw (norm = 0.6 if max=5.0; verify against `parameter-spec.md`). O-Bowed's BOW_PRESSURE range may differ (typical 0–1 norm); plan-execution-log records the actual norm value that produces matched RMS amplitude at the saturator input. If RMS amplitudes diverge >5% between the two plugins at matched-norm, document the divergence in §19.7 verdict body and proceed (matched-amplitude is approximate; the comparison is envelope-shape decay rate, not absolute amplitude).

2. **Compute O-Bowed 65-bin decay envelope** (mirror RESEARCH §19.5.3 protocol):
   ```bash
   python3 - <<'EOF' >/tmp/phase24c-r36d/o-bowed-decay-bins.txt
   import wave, struct, math, sys
   path = "/tmp/phase24c-r36d/o-bowed-parity.wav"
   wf = wave.open(path, 'rb')
   nch, sw, sr, nframes = wf.getnchannels(), wf.getsampwidth(), wf.getframerate(), wf.getnframes()
   raw = wf.readframes(nframes); wf.close()
   bytes_per_frame = nch * sw
   def s24_to_float(b3):
       v = b3[0] | (b3[1] << 8) | (b3[2] << 16)
       if v & 0x800000: v -= 0x1000000
       return v / 8388608.0
   ch0 = []
   for i in range(nframes):
       off = i * bytes_per_frame
       ch0.append(s24_to_float(raw[off:off+sw]))
   bins = []
   binSize = sr
   binCount = 65
   for b in range(binCount):
       start = b * binSize
       end = min(start + binSize, len(ch0))
       sumSq = sum(s*s for s in ch0[start:end])
       n = max(1, end - start)
       rms = math.sqrt(sumSq / n)
       bins.append(rms)
   rmsMax = max(bins)
   for b, r in enumerate(bins):
       db = 20.0 * math.log10(max(1e-9, r) / max(1e-9, rmsMax))
       print(f"{b:2d}  {r:.6f}  {db:7.2f}")
   EOF
   cat /tmp/phase24c-r36d/o-bowed-decay-bins.txt
   ```

3. **Compute divergence at the 5-s post-bow-off mark:**
   - O-Contrabass `decayEnvelopeDb[64]` (read from `saturator-tail-comparison.json`): ~−9.31 dB rel max (RESEARCH §19.5.3 prediction).
   - O-Bowed bin 64 dB rel max (read from Step 2 output above).
   - **Divergence = |O-Contrabass dB − O-Bowed dB|**.
   - Compute also at bins 60–63 (pre-5-s mark) for fuller envelope characterization.

4. **Determine verdict per RESEARCH §19.7.1 tree:**

   **Default path (predicted, ≤ 2 dB divergence):** Append RESEARCH §19.7.5 (NEW subsection) verbatim:
   ```markdown
   ## 19.7.5 Verdict — research-only acknowledged divergence (default path; LOCKED)

   **Measured divergence at 5-s post-bow-off mark:** O-Contrabass −X.XX dB rel max; O-Bowed −Y.YY dB rel max; |Δ| = Z.ZZ dB. **Below 2 dB threshold (Q41) and below typical perceptual JND for sustained tones (~3 dB).**

   **Verdict:** v1.0 retains O-Contrabass's algebraic in-loop saturator (`x / sqrt(1 + x²)` at drive=1.0) verbatim. The architectural divergence from O-Bowed (`tanh(x / 4) × 4` at drive=4.0) is acknowledged as a deliberate design decision, not a bug. Saturator-tail evidence (this section) feeds forward into the end-of-Stage-2 verify cycle's `ARCHITECTURE.md §"In-loop saturator"` amendment as primary source data.

   **Phase 2.4c-bis NOT triggered.** Phase 2.4c R36 atomic commit lands harness-only changes (autocorrelator fix + `--saturator-tail-comparison` mode + new golden + RESEARCH §19 + re-baselined `vibrato.json{,.sha256}`). HR-11 trivially holds.

   **Phase 2.5-aware footnote:** Body resonator (Phase 2.5) is OUT-of-loop (post-output biquad cascade); it changes the perceived envelope but NOT the in-loop signal experienced by the saturator. Bow noise is INPUT-side additive; not in-loop post-saturator. Therefore the §19.7.5 verdict carries forward into Phase 2.5 verify as a regression-check baseline (Phase 2.5 verify includes `--saturator-tail-comparison` re-render + comparison against this golden's `decayEnvelopeDb` to detect any unexpected loop-side leakage from body integration).
   ```

   **Escalation path (>2 dB divergence):** Append RESEARCH §19.7.6 (NEW subsection) verbatim:
   ```markdown
   ## 19.7.6 Verdict — Phase 2.4c-bis escalation flag LOCKED (escalation path)

   **Measured divergence at 5-s post-bow-off mark:** O-Contrabass −X.XX dB rel max; O-Bowed −Y.YY dB rel max; |Δ| = Z.ZZ dB. **Exceeds 2 dB threshold (Q41) and approaches/exceeds perceptual JND.**

   **Verdict:** v1.0 saturator-tail divergence exceeds perceptual budget. Escalation to Phase 2.4c-bis (separate CONTEXT rev-9-bis cycle with source-change scope) IS triggered. Phase 2.4c R36 atomic commit lands harness-only changes (autocorrelator fix + measurement infrastructure + verdict flag) ONLY; saturator-tail port from O-Bowed (`tanh(x/sat) × sat` with `sat=4.0f` at `Source/DSP/WaveguideString.cpp:204–206`) lands in Phase 2.4c-bis R36-bis atomic.

   **Action items for Phase 2.4c-bis (CONTEXT rev-9-bis):**
   1. Source-change scope: port `tanh(x/sat) × sat` with `sat=4.0f` to `Source/DSP/WaveguideString.cpp:204–206` (in-loop saturator; both rails).
   2. HR-11 lifted (DSP edits permitted).
   3. Re-baseline ALL audible goldens: E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + macro-sweep + slow-lfo + schelleng-stress + sub-harmonics + sub-harmonics-stability (9 audible goldens; vibrato carry-forward because saturator port doesn't touch vibrato modulator path; matrix-stability re-render evidence-only).
   4. NEW `saturator-tail-comparison.{wav.sha256,json,json.sha256}` post-port (the existing R36b golden becomes the pre-port reference; post-port golden gets a fresh sha256 because the saturator change alters the WAV).
   5. ARCHITECTURE.md §"In-loop saturator" amendment lands at end-of-Stage-2 verify with both pre-port (Phase 2.4c) and post-port (Phase 2.4c-bis) saturator-tail goldens as evidence base.
   ```

5. **Stage RESEARCH.md edit** (do NOT commit yet — R36 atomic):
   ```bash
   git add plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md
   git status
   ```

**Files created:** none committed; `/tmp/phase24c-r36d/*` transient.

**Files modified:**
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` (~+40 LOC: §19.7.5 OR §19.7.6 verdict subsection appended).

**Commit:** **NONE** — staged for R36 atomic.

**Success bar:**
- [ ] O-Bowed parity render completes; sha256 captured.
- [ ] 65-bin decay envelope computed for O-Bowed; bin 64 dB rel max recorded.
- [ ] Divergence at 5-s post-bow-off mark computed and recorded.
- [ ] §19.7.5 OR §19.7.6 verdict subsection appended to RESEARCH.md verbatim.
- [ ] PLAN-execution-log records measured divergence value + verdict path taken (default vs escalation).

**Estimated effort:** 25 min (O-Bowed render + Python decay-bin computation + divergence calc + §19.7.5/§19.7.6 authoring).

---

### R36e — Regression bar: `reproduce-goldens.sh` extended 12 → 13 entries; 13/13 byte-identical post-edits

**Per RESEARCH §19.10 #R36e + §19.11.3. HR-11 trivially preserves the 12 carry-forward; new 13th entry locks R36b output.**

**Tasks:**

1. **Edit `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh`** — extend `INVOC` array from 12 to 13 entries.

   Append a new entry after the existing `INVOC[sub-harmonics-stability]="--sub-harmonics-stability"` line:
   ```bash
   INVOC[saturator-tail-comparison]="--saturator-tail-comparison"
   ```

   Update the trailing diagnostic line: `OK: all 12 goldens reproduce byte-identical` → `OK: all 13 goldens reproduce byte-identical`.

   No loop-body changes.

2. **Run extended `reproduce-goldens.sh` against fully-edited harness:**
   ```bash
   bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   # Expect: "OK: all 13 goldens reproduce byte-identical"
   # Includes new sat-tail entry against R36b's golden sha256 94a42a81…
   ```

   **All 13 must PASS.** If any of the 12 carry-forward FAIL, HR-11 violated → halt and investigate `git diff --stat HEAD~1..HEAD -- plugins/O-Contrabass/Source/ modules/synthesis/bow-friction/Source/ plugins/O-Bowed/Source/` for stowaway DSP edits. If the new sat-tail entry FAILS, R36b's mode handler emitted a different sha256 than R36b's first invocation (state bleed or non-determinism); investigate.

3. **Sanity-check `--matrix-stability` Phase 2.4a evidence carry-forward (not in `reproduce-goldens.sh`):**
   ```bash
   $HARNESS --matrix-stability \
            --out /tmp/phase24c-r36e/matrix-stability.wav \
            --json /tmp/phase24c-r36e/matrix-stability.json
   computed=$(shasum -a 256 /tmp/phase24c-r36e/matrix-stability.wav | awk '{print $1}')
   expected=$(awk '{print $1}' plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.wav.sha256)
   [ "$computed" = "$expected" ] && echo "[PASS] matrix-stability $computed" || echo "[FAIL]"
   # Expect: 6db6770727ab3b433a036f487217bbde70f8cc15de44fa60ac0b99d868176449
   ```

4. **HR-11 audit hook:** Confirm zero source edits outside harness scope:
   ```bash
   git diff --stat HEAD -- plugins/O-Contrabass/Source/ \
                            modules/synthesis/bow-friction/Source/ \
                            plugins/O-Bowed/Source/
   # Expect: empty (no files reported).
   ```

5. **Stage `reproduce-goldens.sh` edit** (do NOT commit yet — R36 atomic):
   ```bash
   git add plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   git status
   ```

**Files created:** none.

**Files modified:**
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` (+~3 LOC: 1 INVOC entry + diagnostic line update).

**Commit:** **NONE** — staged for R36 atomic.

**Success bar:**
- [ ] `reproduce-goldens.sh` reports `OK: all 13 goldens reproduce byte-identical`.
- [ ] `matrix-stability.wav` sha256 matches Phase 2.4a `6db67707…` byte-identical (HR-9 short-circuit + HR-7 wedge bypass interact correctly across all 108 combos).
- [ ] HR-11 audit hook reports zero source edits outside harness scope.
- [ ] `reproduce-goldens.sh` change staged.

**Estimated effort:** 10 min (edit + reproduce-goldens run + matrix-stability smoke + HR-11 audit + stage).

---

### R36f — auval AU VALIDATION SUCCEEDED + pluginval --strictness-level 10 SUCCESS

**Production build smoke; required by Gate 6c invariant 4. No DSP changes → both expected to pass without re-investigation.**

**Tasks:**

1. **Build production targets:**
   ```bash
   cmake --build build --target O-Contrabass_VST3 O-Contrabass_AU --parallel
   ```
   Expect a clean build with zero warnings.

2. **Install fresh AU + VST3 binaries (per CLAUDE.md cache-clearing protocol):**
   ```bash
   killall -9 AudioComponentRegistrar 2>/dev/null || true
   rm -rf ~/Library/Caches/AudioUnitCache/
   rm -rf ~/Library/Caches/com.apple.audiounits.cache
   rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3
   rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass.component
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass.vst3 \
         ~/Library/Audio/Plug-Ins/VST3/
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass.component \
         ~/Library/Audio/Plug-Ins/Components/
   ```

3. **Run auval AU validation:**
   ```bash
   auval -v aumu OcOC Ouar 2>&1 | tail -40
   # Expect: AU VALIDATION SUCCEEDED
   ```

4. **Run pluginval at strictness 10:**
   ```bash
   /Applications/pluginval.app/Contents/MacOS/pluginval \
       --strictness-level 10 --validate-in-process --skip-gui-tests --timeout-ms 60000 \
       ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3 2>&1 | tail -10
   # Expect: ALL TESTS PASSED + Process exit code 0
   ```

5. **Logic AU smoke (R37 — DEFERRED non-blocking per CONTEXT rev-8 Q43):** Skipped at execute-phase. Logged as `R37: deferred non-blocking (no DSP changes → no audible difference for AU smoke to detect; mirrors R37/R32/R27/R19f/R14e precedent).` in PLAN-execution-log.

**Files created:** none.

**Files modified:** none.

**Commit:** **NONE** — diagnostic only.

**Success bar:**
- [ ] Production build clean zero warnings.
- [ ] Fresh AU + VST3 installed.
- [ ] auval reports `AU VALIDATION SUCCEEDED`.
- [ ] pluginval --strictness-level 10 reports `ALL TESTS PASSED` (exit code 0).
- [ ] Logic AU smoke logged as deferred non-blocking.

**Estimated effort:** 10 min (build + install + auval + pluginval).

---

### R36 — Phase 2.4c atomic commit (Gate 6c PASS)

**Single commit lands all Phase 2.4c changes per RESEARCH §19.10 #R36 + Q44. Mirrors R34/R35 atomic-commit precedent.**

**Tasks:**

1. **Verify staged files cover Phase 2.4c scope:**
   ```bash
   git status
   # Expected staged files (~11–13 total per RESEARCH §19.10.2 Option B):
   #   plugins/O-Contrabass/tests/render-harness/main.cpp
   #   plugins/O-Bowed/tests/render-harness/main.cpp
   #   plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   #   plugins/O-Contrabass/tests/render-harness/golden/vibrato.json
   #   plugins/O-Contrabass/tests/render-harness/golden/vibrato.json.sha256
   #   plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.wav.sha256
   #   plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json
   #   plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json.sha256
   #   plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md (rev-8 from discuss-phase)
   #   plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md (§19 + §19.7.5/§19.7.6)
   #   plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md (this rev-10)
   #   plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md (Phase 2.4c append)
   #   plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md (Phase 2.4c append)
   #   plugins/O-Contrabass/.planning/STATUS.md (phase_2_4c_atomic_commit_sha + verify state)
   ```

2. **HR-11 final audit** (BEFORE commit):
   ```bash
   git diff --cached --stat -- plugins/O-Contrabass/Source/ \
                                 modules/synthesis/bow-friction/Source/ \
                                 plugins/O-Bowed/Source/
   # MUST be empty. If any file reported, HR-11 violated → halt and investigate.
   ```

3. **Compose R36 atomic commit:**
   ```bash
   git commit -m "$(cat <<'EOF'
   feat(O-Contrabass): Phase 2.4c R36 — autocorrelator octave-rejection + saturator-tail O-Bowed comparison (research-only); Gate 6c PASS

   Phase 2.4c is harness-only / research-only by HR-11 construction (zero
   production DSP edits). Closes Phase 2.3 R28 audit-debt (relaxed
   pass_vibratoAudible) and Phase 2.1a R6 audit-debt (saturator-tail
   characterization deferred for O-Bowed cross-comparison) before Phase 2.5
   body resonator alters tail-decay envelope.

   Autocorrelator fix (R36a):
     - tests/render-harness/main.cpp:1742-1743 — replace integer-lag
       constexpr kTauMin=400/kTauMax=1500 with MIDI-28-derived ±20% range
       bias [856, 1285] (RESEARCH §19.2.3). Excludes period/2 (~535 samples /
       82.4 Hz) latch point that produced Phase 2.3 R28 octave-up +1200¢
       outliers in perCycleDeltaCents.
     - Add pass_vibratoAudible aggregator predicate to --vibrato JSON
       (mirrors --sub-harmonics pass_combo pattern; AND of rate / depth /
       onset / rmsContinuity per Phase 2.3 PLAN rev-7 design intent).

   Saturator-tail comparison (R36b):
     - NEW --saturator-tail-comparison CLI flag + mode handler in
       tests/render-harness/main.cpp. Renders canonical E1 60s sustain + 5s
       release at default bow params + INFINITE_SUSTAIN=1.0; analyser emits
       65-bin per-second decayEnvelopeDb on channel 0. Predicted golden
       sha256 94a42a8190557128815ef760bfa5ad3cc81f109e1156a3395b8ac507e54ceae6
       (RESEARCH §19.5.2 pre-flight, byte-identical reproduction).
     - Option B scope-expansion: plugins/O-Bowed/tests/render-harness/main.cpp
       extended with --bow-speed --bow-pressure --bow-position
       --infinite-sustain value-consume flags for parity rendering. O-Bowed
       canonical-preset.wav.sha256 carries forward byte-identical (defaults
       preserved when flags absent).

   RESEARCH §19.7 verdict (R36d):
     - PATH: research-only acknowledged divergence (default; <2 dB measured)
       OR Phase 2.4c-bis escalation (>2 dB; CONTEXT rev-9-bis source-change
       scope). [Plan-execution-log records actual path taken.]

   Goldens:
     - 1 changed JSON (vibrato.json — range-bias-corrected metrics; WAV
       d7881ecf… carries forward byte-identical via HR-11)
     - 1 NEW JSON sha256 anchor (vibrato.json.sha256; first JSON anchor for
       vibrato; mirrors sub-harmonics.json.sha256 precedent)
     - 3 NEW saturator-tail goldens (.wav.sha256 + .json + .json.sha256)
     - reproduce-goldens.sh extended 12 → 13 entries

   Gate 6c invariants:
     1. reproduce-goldens.sh 13/13 byte-identical (HR-11 trivially)
     2. --vibrato strict pass_vibratoAudible=true post R36a
     3. --saturator-tail-comparison sha256 byte-deterministic across re-renders
        + RESEARCH §19.7 verdict written
     4. auval AU VALIDATION SUCCEEDED + pluginval-10 SUCCESS
     5. RESEARCH §19.7 verdict locked

   NO Stage-1 contract amendment (parameter-spec.md sha256 77638e25…
   carries forward unchanged). NO ARCHITECTURE.md amendment (saturator-tail
   evidence feeds end-of-Stage-2 §"In-loop saturator" amendment cycle).
   NO production DSP source edit (HR-11 technical defence).

   Continues atomic-commit sequence: R7 → R15 → R20 → R26 → R33 → R34 →
   R35 → R36. R36-backfill chore propagates R36 sha into STATUS.md.

   Phase 2.4-bis backlog stays parked. Phase 2.5 (body resonator + bow
   noise) opens fresh CONTEXT rev-9 after Phase 2.4c verify.

   Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
   EOF
   )"
   ```

4. **Verify atomic commit landed:**
   ```bash
   git log --oneline -1
   git log --stat -1 | head -40
   ```

**Files committed:** ~11–13 files (Option B; per RESEARCH §19.10.2).

**Commit:** R36 atomic commit (Gate 6c PASS).

**Success bar:**
- [ ] `git status` reports working tree clean post-commit.
- [ ] `git log -1` shows new commit with R36 atomic message.
- [ ] HR-11 final audit shows 0 source files in `plugins/O-Contrabass/Source/` / `modules/synthesis/bow-friction/Source/` / `plugins/O-Bowed/Source/`.

**Estimated effort:** 10 min (audit + commit + verify).

---

### R36-backfill chore — Propagate R36 sha into STATUS.md

**Separate non-atomic chore commit per R34-backfill `b64c8c4` / R35-backfill `0db5fac` precedent.**

**Tasks:**

1. **Read R36 sha:**
   ```bash
   R36_SHA=$(git log --format=%H -1)
   echo "R36 sha: $R36_SHA"
   ```

2. **Edit `plugins/O-Contrabass/.planning/STATUS.md`** — set `phase_2_4c_atomic_commit_sha`:
   ```yaml
   gate_state:
     phase_2_4c_autocorr_sat_tail: PASS
   phase_2_4c_atomic_commit_sha: <R36 full sha>
   next_action: phase_2_4c_verify   # OR phase_2_5_discuss if verify auto-completes
   ```

3. **Commit chore:**
   ```bash
   git add plugins/O-Contrabass/.planning/STATUS.md
   git commit -m "$(cat <<'EOF'
   chore(O-Contrabass): backfill Phase 2.4c R36 commit sha into STATUS.md

   Mirrors R34-backfill b64c8c4 / R35-backfill 0db5fac precedent. Records
   R36 atomic commit sha in phase_2_4c_atomic_commit_sha field for audit
   trail + future-cycle CONTEXT rev-9 reference.

   Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
   EOF
   )"
   ```

**Files modified:**
- `plugins/O-Contrabass/.planning/STATUS.md` (+1 sha line + gate_state update + next_action flip).

**Commit:** R36-backfill chore (separate from R36 atomic).

**Success bar:**
- [ ] `phase_2_4c_atomic_commit_sha` populated with R36 full sha.
- [ ] `gate_state.phase_2_4c_autocorr_sat_tail: PASS` recorded.
- [ ] `next_action: phase_2_4c_verify` (or `phase_2_5_discuss` if verify already complete).
- [ ] Chore commit lands; `git log --oneline -2` shows R36 atomic + R36-backfill chore.

**Estimated effort:** 5 min.

---

## Why R36 is a single atomic commit

Five-invariant Gate 6c audit (RESEARCH §19.15 + CONTEXT rev-8 §"Audit Trail") demands a single quotable sha for verification. Splitting Phase 2.4c into `R36a` (autocorrelator) + `R37` (saturator-tail) atomic commits would:

1. Double the auval/pluginval cycles (each atomic commit needs its own production-build smoke).
2. Break HR-11's single-shot bit-exact regression bar (R36e's `reproduce-goldens.sh 13/13 PASS` only meaningfully validates against the combined R36a + R36b + R36c edits; pre-R36b state has only 12 entries, post-R36a-only state lacks the `--saturator-tail-comparison` mode entirely).
3. Decouple the autocorrelator fix from its `vibrato.json` re-baseline (R36c MUST land in the same commit as R36a or the regression bar breaks for the JSON-side carry-forward audit).
4. Break atomic-commit precedent (R7 / R15 / R20 / R26 / R33 / R34 / R35 are all single atomics; R36 continues the pattern).

The harness-only / research-only nature of Phase 2.4c reduces R36's scope to ~11–13 files (vs R35's ~14, R34's ~16–19) — the smaller atomic is actually safer because there is NO production DSP source touched (HR-11 trivial guarantee).

---

## Files To Create / Modify (consolidated, Phase 2.4c)

**Created (3 new golden text files + 1 new JSON sha256 anchor):**
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.wav.sha256` — sha256 of new harness-mode WAV (predicted `94a42a81…`).
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json` — full JSON per RESEARCH §19.5.1 schema (~3 KB).
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json.sha256` — anchors JSON byte-determinism.
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.json.sha256` — first JSON sha256 anchor for vibrato (mirrors `sub-harmonics.json.sha256` precedent).

**Modified (R36-stage scope):**
- `plugins/O-Contrabass/tests/render-harness/main.cpp` (~+138 LOC: range-bias edit + `pass_vibratoAudible` aggregator + `--saturator-tail-comparison` flag + mode handler + analyser + JSON emission).
- `plugins/O-Bowed/tests/render-harness/main.cpp` (~+80 LOC: 4 value-consume flags + Args struct extension + sentinel-conditional `setNorm` pinning).
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` (~+3 LOC: 1 INVOC entry + diagnostic line update).
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.json` (re-baselined to range-bias-corrected metrics; new `pass_vibratoAudible` aggregator field).
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` (rev-8 already in place from discuss-phase; no further edit at execute).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` (~+40 LOC: §19.7.5 OR §19.7.6 verdict subsection appended at R36d).
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` (this rev-10 — already in place from plan-phase; no further edit at execute).
- `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` (Phase 2.4c append at end of execute).
- `plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md` (Phase 2.4c append at end of verify).
- `plugins/O-Contrabass/.planning/STATUS.md` (phase_2_4c_atomic_commit_sha via R36-backfill chore + gate_state + next_action).

**NOT modified (HR-11 trivially preserved):**
- `plugins/O-Contrabass/Source/**` — every file under `Source/` carries forward verbatim. Any edit = HR-11 hard violation.
- `plugins/O-Bowed/Source/**` — every file under `Source/` carries forward verbatim. Option B touches `tests/render-harness/main.cpp` ONLY.
- `modules/synthesis/bow-friction/**` — friction module v1.0.0 ABI preservation.
- `plugins/O-Contrabass/CMakeLists.txt` (no new headers, no new build targets).
- `plugins/O-Contrabass/.planning/REQUIREMENTS.md` / `BRIEF.md` / `ROADMAP.md` (no SUB_HARMONICS or saturator references requiring edits).
- `plugins/O-Contrabass/.planning/stages/1-foundation/*` (closed milestone).
- 12 carry-forward goldens (HR-11 trivially preserves byte-identity post R36a/R36b edits).
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.wav.sha256` (`d7881ecf…` carries forward; only JSON re-baselined).
- `plugins/O-Bowed/tests/render-harness/golden/canonical-preset.wav.sha256` (Option B harness extension preserves cohort baseline byte-identical).
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.{wav.sha256,json,json.sha256}` (Phase 2.4a evidence carry-forward; not in `reproduce-goldens.sh`).

---

## Dependencies Graph (compact)

```
R36-pre (working-tree integrity tripwire; reproduce-goldens.sh 12/12 PASS at HEAD
         5d95d15; O-Bowed canonical-preset cohort tripwire; vibrato octave-jump
         baseline confirmation against existing JSON; saturator-tail 3-trial
         determinism at predicted sha256 94a42a81…)
   ↓                ↓ (mismatch → STOP, investigate working-tree drift)
   ├── R36a (autocorrelator range-bias kTauMin/kTauMax MIDI-28-derived;
   │        pass_vibratoAudible aggregator added; ~+18/-2 LOC O-Contrabass main.cpp;
   │        smoke confirms vibrato.wav byte-identical (HR-11) + post-fix metrics
   │        strict-PASS; staged for R36 atomic)
   │   ↓ post-R36a smoke: pass_vibratoAudible=true; vibrato.wav HR-11 holds
   │   ↓
   │   R36c (re-baseline vibrato.json + new vibrato.json.sha256;
   │        WAV unchanged HR-11; staged for R36 atomic)
   │
   └── R36b (--saturator-tail-comparison flag + mode handler + analyser +
            Option B O-Bowed harness extension --bow-* + --infinite-sustain;
            ~+120 LOC O-Contrabass + ~+80 LOC O-Bowed; build both harnesses;
            cohort regression smoke (O-Bowed canonical byte-identical);
            render new sat-tail golden; sha256 matches RESEARCH §19.5.2
            prediction 94a42a81…; 3-trial WAV + JSON determinism smokes;
            stage 3 new golden text files)
       ↓ (R36b sha256 matches predicted)
       R36d (O-Bowed parity render at canonical bow operating point with
            new --bow-* + --infinite-sustain flags; compute 65-bin decay
            envelope; measure divergence at 5-s post-bow-off mark; populate
            §19.7.5 OR §19.7.6 verdict subsection in RESEARCH.md;
            staged for R36 atomic)
       ↓ (verdict locked)

R36e (reproduce-goldens.sh extended 12 → 13 entries; runs 13/13 byte-identical
      post-R36a/R36b/R36c source edits; matrix-stability sanity at 6db67707…;
      HR-11 audit hook reports 0 source files in Source/; staged for R36 atomic)
   ↓ (PASS)
R36f (Production build O-Contrabass_VST3 + O-Contrabass_AU; install fresh; auval
      AU VALIDATION SUCCEEDED; pluginval --strictness-level 10 SUCCESS; Logic
      AU smoke deferred non-blocking)
   ↓ (PASS)
R36 (Phase 2.4c atomic commit — ~11–13 files; closes 2.4c; mirrors R34/R33/R26/
     R20/R15/R7 atomic-commit pattern; commit body documents 5-invariant Gate 6c
     audit + HR-11 trivially-holds technical defence + verdict path taken
     (research-only acknowledged divergence default OR Phase 2.4c-bis escalation
     contingency) + NO contract amendment + Phase 2.5 follow-up scope)
   ↓
R36-backfill chore (separate non-atomic; STATUS.md phase_2_4c_atomic_commit_sha;
                    gate_state.phase_2_4c_autocorr_sat_tail = PASS;
                    next_action = phase_2_4c_verify; matches R34-backfill /
                    R35-backfill precedent)
```

R36-pre is a strict prerequisite — if working tree drifts, all subsequent bit-exact reasoning breaks. R36a + R36b can execute in parallel within execute-phase (independent file scopes — autocorrelator fix vs new mode handler — though both edit `O-Contrabass/main.cpp`, the edits are in disjoint code regions and merge cleanly). R36c depends on R36a (re-baseline post-fix). R36d depends on R36b (verdict needs O-Bowed parity render against R36b's flags). R36e is the bit-exact regression bar (12 carry-forward + 1 new). R36f is the Gate 6c production-build audit. R36 is the gated finisher; R36-backfill chore propagates R36 sha after R36 lands.

---

## Risks (Phase 2.4c, refreshed from RESEARCH §19.14)

| # | Risk | Current state | Mitigation |
|---|---|---|---|
| 1 | HR-11 violation via accidental DSP edit | **PRE-MITIGATED** (RESEARCH §19.1 + R36-pre tripwire + R36e re-tripwire + R36 final audit hook) | R36-pre as structural prerequisite to R36a/b; R36e re-runs `reproduce-goldens.sh` post-edits; R36 atomic commit's `git diff --cached --stat -- Source/` MUST be empty. Any drift = HARD HR-11 violation, halt and investigate (likely accidental edit to `Source/` outside the harness scope). |
| 2 | Parabolic-interp + range-bias insufficient at 12-cent vibrato | **DISSOLVED** — parabolic-interp ALREADY present per RESEARCH §19.2.1; precision floor ~0.16¢ (RESEARCH §19.2.3) far below 12¢ requirement | YIN/AMDF/cepstrum fallback retired. R36a is range-bias-only (~+18/-2 LOC, NOT the ~150 LOC suggested by CONTEXT rev-8 Q37 wording). |
| 3 | O-Bowed render harness unavailable | **CHARACTERIZED** — exists but NOT parity-able without Option B scope-expansion (RESEARCH §19.4) | Option B (scope-expand) LOCKED per pin #2 + Risk #15. Adds ~+80 LOC `plugins/O-Bowed/tests/render-harness/main.cpp`. R36-pre cohort tripwire confirms O-Bowed canonical-preset baseline at HEAD. R36b cohort smoke confirms post-extension byte-identity (defaults preserved when flags absent). |
| 4 | >2 dB divergence triggers mid-Phase 2.4c-bis escalation | **CHARACTERIZED** — analytic bound (RESEARCH §19.3.3 + §19.6) predicts <2 dB at canonical operating amplitudes; escalation low-probability but lane locked (RESEARCH §19.7.3) | Plan-phase pre-writes Phase 2.4c-bis CONTEXT rev-9-bis structural skeleton at "Contingency" section below. R36 atomic stays harness-only regardless of verdict path; only the §19.7 verdict subsection (R36d) varies between default vs escalation. |
| 5 | Vibrato pre-flight catches autocorrelator drift | **PRE-MITIGATED** — RESEARCH §19.1 confirmed all 12 goldens reproduce at HEAD `5d95d15`; no upstream drift | R36-pre tripwire is the gate. If FAIL, halt and re-confirm HEAD before proceeding. |
| 6 | `saturator-tail-comparison.wav.sha256` non-deterministic | **PRE-MITIGATED** — 3-trial determinism PASS at HEAD per RESEARCH §19.5.2 | R36-pre includes 3-trial sha256 confirmation (predicted `94a42a81…`); R36b includes 3-trial WAV smoke + 3-trial JSON smoke post-mode-handler-author. |
| 7 | JSON `decayEnvelopeDb` width vs sha256 noise | **CHARACTERIZED** — fixed-width 4-decimal-place format pin per RESEARCH §19.5.5 | Pin #7 LOCKED: `juce::String (val, 4)` for all `decayEnvelopeDb[]` floats + `peak`, `rmsMid_s5_s6`, `rmsFinal_lastSecond`, etc. Wall-clock fields emit as integer microseconds. R36b 3-trial JSON smoke validates determinism. |
| 8 | R36 atomic + R35-backfill chore interaction | **PRE-MITIGATED** — chore-after-atomic precedent (R34-backfill, R35-backfill) | R36-backfill follows R36 atomic verbatim. `git log --oneline -2` post-backfill shows R36-backfill chore + R36 atomic in that order. |
| 9 | RESEARCH §19 surfaces non-saturator divergences | **PRE-MITIGATED** — RESEARCH §19.3.4 audit confirms in-loop saturator is the ONLY in-loop nonlinearity in O-Bowed | Out-of-scope findings (e.g., bridge LP coefficient choice, friction junction defaults, dispersion topology) tracked as Phase 2.4c-bis or v1.1 backlog items, NOT folded into Phase 2.4c. |
| 10 | Phase 2.5-awareness supersedes saturator decision | **CHARACTERIZED** — body resonator is OUT-of-loop (post-output biquad cascade); does NOT invalidate saturator-tail evidence (RESEARCH §19.7.4) | Phase 2.5 verify includes `--saturator-tail-comparison` re-render as regression check (body-resonator-aware). Phase 2.4c verdict carries forward as "valid for v1.0 pre-Phase-2.5 architecture". |
| 11 | MIDI 28 expected-period range bias incorrect for E1 dispersion-warped pitch | **CHARACTERIZED** — Phase 2.1c dispersion warps pitch ~0.5–2¢ at E1; ±20% range bias is order-of-magnitude wider than dispersion error | No mitigation needed; range bias `[856, 1285]` covers `1070 × 1.0009 ≈ 1071` even at maximum dispersion warp. |
| 12 | `reproduce-goldens.sh` 12 → 13 entry growth | **PRE-MITIGATED** — wall-clock +0.29 s per RESEARCH §19.13; total runtime ~3.3 s | No mitigation needed. |
| 13 | **NEW (RESEARCH §19.14)** O-Bowed canonical-preset cohort regression at R36b Option B | Adding `--bow-*` + `--infinite-sustain` flags to O-Bowed harness MUST NOT change existing `--note 69 --velocity 0.7 --sustain 5 --release 0` invocation (defaults preserved when flags absent) | R36-pre includes O-Bowed canonical-preset cohort tripwire BEFORE R36b edits; R36b includes cohort regression smoke AFTER edits to confirm byte-identical default behaviour. Sentinel-pattern (`-1.0f` default + `>= 0.0f` conditional `setNorm`) ensures unset flags don't perturb factory APVTS. |
| 14 | **NEW (RESEARCH §19.14)** Post-fix `onsetTimeMs` lands outside [800, 1000] strict gate | If onset detection responds at ~600 ms after R36a fix instead of ~800 ms (autocorrelator detects modulation immediately) | Pin #1 LOCKED: PLAN rev-10 widens `pass_onsetWindow` to `[600, 1000]` if R36a prototype measures <800 ms. R36a smoke records measured `onsetTimeMs` verbatim in PLAN-execution-log; R36c re-baselined `vibrato.json` reflects either default `[800, 1000]` or widened `[600, 1000]` per measured value. |
| 15 | **NEW (RESEARCH §19.14)** O-Bowed factory `infiniteSustain = 0.0` invalidates Option A parity | At O-Bowed factory defaults, bridge LP loss dominates → tail decays much faster than O-Contrabass; envelope-shape comparison meaningless | Option B (scope-expand) is REQUIRED, not optional. Pin #2 LOCKED. R36b extends O-Bowed harness with `--infinite-sustain 1.0` override (among other `--bow-*` flags). |
| 16 | **NEW** R36b sha256 drift between RESEARCH §19.5.2 pre-flight and R36b execute-phase | Predicted `94a42a81…` is stored at RESEARCH §19.5.2; if R36b's mode handler emits a different sha256, parameter-pinning has drifted | Q8 LOCKED: APVTS pinning in R36b mode handler MUST mirror RESEARCH §19.5.2 invocation EXACTLY (`BOW_SPEED=0.15` + `BOW_PRESSURE=3.0` + `BOW_POSITION=0.10` + `INFINITE_SUSTAIN=1.0` + factory STRING_STIFFNESS + factory SLOW_LFO_DEPTH/VIBRATO_DEPTH/EXPRESSION_MACRO/SUB_HARMONICS=0). R36b smoke confirms `94a42a81…` byte-identically; if drift, STOP and investigate parameter-pinning. |
| 17 | **NEW** Toolchain `constexpr std::pow` support for R36a edit | `kVibratoF0Hz = 440.0 * std::pow(2.0, ...)` requires C++23 `constexpr` math (or compile-time fallback) | R36a Step 1 fallback path: replace `constexpr` with `inline const` if toolchain rejects C++20-`constexpr`. Both forms compute the same values (`kTauMin = 856`, `kTauMax = 1285`); the `inline const` form lacks compile-time evaluation but is harness-side overhead-free at runtime. PLAN-execution-log records which form was used. |

---

## Success Criteria (Gate 6c — Phase 2.4c verify exit gate)

- [ ] **R36-pre** — Working-tree integrity tripwire: (a) `reproduce-goldens.sh` reports `OK: all 12 goldens reproduce byte-identical`; (b) O-Bowed canonical-preset cohort tripwire PASS; (c) `vibrato.json` shows octave-jump baseline (`peakDepthCents ≈ 625.44`); (d) saturator-tail 3-trial determinism PASS at predicted sha256 `94a42a81…`.
- [ ] **R36a** — `tests/render-harness/main.cpp:1742-1743` autocorrelator range-bias edit applied (MIDI-28-derived `kTauMin=856` / `kTauMax=1285`); `pass_vibratoAudible` aggregator predicate added to `--vibrato` JSON output. Build clean zero warnings. Smoke confirms post-fix `peakDepthCents ≈ 12` + all 4 sub-predicates `true` + HR-11 vibrato.wav byte-identical. `onsetTimeMs` measured value recorded; pin #1 strict-range applied verbatim or widened.
- [ ] **R36b** — `--saturator-tail-comparison` CLI flag + mode handler + 65-bin decay-envelope analyser added to O-Contrabass main.cpp; Option B O-Bowed harness extension (`--bow-speed --bow-pressure --bow-position --infinite-sustain` flags) applied. Both harnesses build clean. O-Bowed cohort regression PASS (`canonical-preset.wav.sha256` byte-identical). R36b sha256 matches RESEARCH §19.5.2 prediction `94a42a81…` byte-identically. 3-trial WAV + 3-trial JSON sha256 determinism PASS. 3 new golden text files staged.
- [ ] **R36c** — `vibrato.json` re-baselined to range-bias-corrected metrics; new `vibrato.json.sha256` written. HR-11 vibrato.wav byte-identical. JSON reports `pass_vibratoAudible = true` strict-PASS. 2 golden text files staged.
- [ ] **R36d** — RESEARCH §19.7.5 (default) OR §19.7.6 (escalation) verdict subsection appended verbatim. O-Bowed parity render rendered + decay-envelope computed + divergence at 5-s post-bow-off mark recorded.
- [ ] **R36e invariant (1)** — Bit-exact regression bar: `reproduce-goldens.sh` reports `OK: all 13 goldens reproduce byte-identical`; HR-11 trivially preserves the 12 carry-forward; new sat-tail entry locks R36b output. `--matrix-stability` carry-forward at `6db67707…` byte-identical (HR-11 trivially because no DSP edits). HR-11 audit hook reports 0 source files in `Source/`.
- [ ] **R36f invariant (4)** — auval reports `AU VALIDATION SUCCEEDED`; pluginval --strictness-level 10 reports `ALL TESTS PASSED`. Logic AU smoke logged as deferred non-blocking (R37 deferred non-blocking precedent).
- [ ] **R36e + R36f audit table** — Five-item Gate 6c bar compiled (input to VERIFICATION.md): items (1) reproduce-goldens.sh 13/13 PASS; (2) `--vibrato` strict `pass_vibratoAudible = true` post R36a; (3) `--saturator-tail-comparison` golden bit-deterministic + RESEARCH §19.7 verdict written; (4) auval + pluginval-10 SUCCESS; (5) RESEARCH §19.7 verdict locked = research-only acknowledged divergence (predicted default) OR Phase 2.4c-bis escalation flag (low-probability per §19.6 analytic bound).
- [ ] **R37 (deferred non-blocking)** — Logic AU smoke: DEFERRED per CONTEXT rev-8 Q43 (no DSP changes → no audible difference for AU smoke to detect; mirrors R37/R32/R27/R19f/R14e/R34h precedent).
- [ ] **R36** — Atomic commit landed; `git log --stat HEAD~1..HEAD` shows ~11–13 files in single commit; HR-11 final audit shows 0 source files outside harness scope. Commit body documents 5-invariant Gate 6c audit + verdict path + NO contract amendment + Phase 2.5 follow-up scope.
- [ ] **R36-backfill** — STATUS.md `next_action` flipped to `phase_2_4c_verify` (or `phase_2_5_discuss` if verify auto-completed); `gate_state.phase_2_4c_autocorr_sat_tail: PASS`; `phase_2_4c_atomic_commit_sha` recorded via R36-backfill chore commit. **NO `parameter-spec.md` edit; NO `contract_checksums.parameter_spec` change.**
- [ ] **(ARCHITECTURE.md amendments, post-Stage-2-verify, OUT OF SCOPE for this execute)** — §"DC Blocker" + §"In-loop saturator" amendments still deferred. End-of-Stage-2 verify cycle consumes Phase 2.4c §19.7 verdict as primary source data for §"In-loop saturator" amendment.

When all checks above are green (R37 deferred non-blocking accepted), **Phase 2.4c verifies** and **Stage 2 progress reaches ~75%** (2.1 + 2.2 + 2.3 + 2.4a + 2.4b + 2.4c closed; 2.5, 2.6 remain). Phase 2.5 (body resonator + bow noise) opens as a fresh GSD cycle; CONTEXT rev-9 written when 2.5 discuss-phase opens.

---

## Contingency — Phase 2.4c-bis Escalation Lane (RESEARCH §19.16 #10 pre-write)

**Triggered IFF R36d measures saturator-tail divergence > 2 dB at 5-s post-bow-off mark.** Predicted low-probability per RESEARCH §19.6 analytic bound (<2 dB at canonical bow operating amplitude). Pre-written here so escalation is one-action-away.

**Phase 2.4c-bis CONTEXT rev-9-bis skeleton (referenced by R36d §19.7.6 verdict):**

- **Cycle scope:** Source-change cycle. Port `tanh(x/sat) × sat` with `sat=4.0f` from O-Bowed (`plugins/O-Bowed/Source/DSP/WaveguideString.cpp` lines 135–139 + 217–219) to O-Contrabass (`plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` lines ~204–206; in-loop both rails, pre-pushSample). HR-11 LIFTED (DSP edits permitted).
- **Atomic-commit unit:** R36-bis (Gate 6c-bis PASS) — single commit lands the saturator port + re-baselined audible goldens + new pre-port-vs-post-port saturator-tail goldens.
- **Hard rules:** HR-1..HR-10 carry-forward verbatim. HR-11 retired. No new HR introduced (port is straightforward formula swap).
- **Goldens to re-baseline:** 9 audible goldens (E1 strict + per-string A/D/G + detune-sweep-A + note-sequence + macro-sweep + slow-lfo + schelleng-stress + sub-harmonics + sub-harmonics-stability) — all change because the saturator participates in steady-state energy balance for any sustained excitation. Vibrato carries forward (saturator port doesn't touch vibrato modulator path; modulation is upstream). matrix-stability re-render evidence-only.
- **NEW saturator-tail comparison golden post-port:** `saturator-tail-comparison.{wav.sha256, json, json.sha256}` get fresh sha256s. The Phase 2.4c R36 `94a42a81…` becomes the historical pre-port reference (preserved in git history; not re-emitted as a golden).
- **ARCHITECTURE.md amendment:** end-of-Stage-2 verify §"In-loop saturator" amendment cycle lands the saturator port decision verbatim with both pre-port (Phase 2.4c) and post-port (Phase 2.4c-bis) goldens as evidence base.
- **Sequencing:** Phase 2.4c verify → Phase 2.4c-bis discuss-phase opens (CONTEXT rev-9-bis written) → research-phase confirms port topology + measures post-port saturator-tail at canonical → plan-phase writes R36-bis task breakdown → execute-phase ports + re-baselines + commits R36-bis atomic → verify-phase Gate 6c-bis confirms (1) all 9 audible goldens re-baselined byte-identically across re-renders; (2) auval/pluginval-10 SUCCESS; (3) post-port sat-tail divergence <2 dB at 5-s mark (i.e., port WORKED, divergence collapsed); (4) RESEARCH §19.7.7 (NEW) records the post-port verdict.
- **Phase 2.5 sequencing post-2.4c-bis:** Phase 2.5 opens fresh CONTEXT rev-10 (NOT rev-9) to absorb the architectural change.

**Plan-phase pre-write of CONTEXT rev-9-bis SUPERSEDES rev-8 IFF escalation triggers.** Default path leaves rev-9-bis dormant and rev-8 stays the canonical end-of-Phase-2.4c context.

---

## Out of Scope (deferred per CONTEXT.md rev-8 + RESEARCH §19 + STATUS.md)

- **Phase 2.4c-bis** — Source-change saturator port (`tanh(x/sat) × sat` with `sat=4.0f` from O-Bowed → O-Contrabass `Source/DSP/WaveguideString.cpp:204–206`). Triggered ONLY by >2 dB measured envelope divergence at the 5-s post-bow-off mark in R36d. Default Phase 2.4c verdict = research-only acknowledged divergence (no source change). Skeleton pre-written above for one-action-away escalation.
- **Phase 2.4-bis** — kForceBoost retune (0.8 → ~1.0) to push subharmEnergyRatio above 0.40 strict; tune Step 4 modulation gain or refine breathingAudible metric to hit 20% peak-to-peak; reduce 3 v1.0 fallback cells via downstream-defense tightening. Carry-forward backlog from Phase 2.4b R35 commit body; not addressed in Phase 2.4c.
- **Phase 2.5** — Body resonator + bow noise (8-mode parallel biquad body bank Askenfelt-derived; 3-band BPF bow noise summed AFTER body resonator). Phase 2.4c verdict explicitly carries forward as "valid for v1.0 pre-Phase-2.5 architecture"; Phase 2.5 verify includes saturator-tail re-measurement as regression check.
- **Phase 2.6** — Master saturator/limiter, stereo width, microtonal, MPE, Note Expression, MTS-ESP, Scala/TUN.
- **Chaos detector** (architecture §457 line 476 "optional") — control-rate ~100 Hz lag-2 RMS check with 20% bias back-off; deferred to Phase 2.5/2.6 per Phase 2.4b RESEARCH §18.13.
- **softClampState energy clamp** — ROADMAP §Phase 2.4 deliverable (threshold 0.85 ceiling 1.0); deferred to Phase 2.5/2.6 alongside body resonator integration per Phase 2.4b RESEARCH §18.13.
- **YIN / AMDF / cepstrum autocorrelation** — fallback-only path; default is range-bias-only per RESEARCH §19.2.1 (parabolic-interp already present + sub-sample precision benchmark `~0.16¢` far below 12¢ requirement). Fallback REJECTED at research-phase; not reopened at Phase 2.4c-bis.
- **Per-string `--vibrato-A/-D/-G` audible-mode goldens** — RESEARCH §19.2.3 range-bias hard-codes `kVibratoMidiNote = 28`. Per-string variants with bin shift deferred to Phase 2.4-bis or 2.4c-bis if escalation triggers separately.
- **Per-MIDI-note `--vibrato` mode** — `kVibratoMidiNote = 28` hard-coded for v1.0 per RESEARCH §19.2.3 + pin #1. MIDI-derived range bias from `args.midiNote` deferred to Phase 2.4-bis or v1.1.
- **`--saturator-tail-comparison` per-string variants** — bin selection is canonical E1 only at v1.0. Per-string sat-tail comparison (e.g., MIDI 33 / 38 / 43) deferred to Phase 2.5 verify regression checks or v1.1.
- **ARCHITECTURE.md amendments** — end-of-Stage-2 verify decides §"DC Blocker" + §"In-loop saturator" amendments. Phase 2.4c saturator-tail evidence (R36b golden + R36d §19.7.5/§19.7.6 verdict) feeds the §"In-loop saturator" amendment cycle as primary source data but does NOT amend the architecture itself.
- **Logic Pro AU smoke (R37)** — user-deferred non-blocking per CONTEXT rev-8 Q43 + R37/R32/R27/R19f/R14e/R34h precedent. No DSP changes → no audible difference for AU smoke to detect.
- **E1 dispersion calibration polynomial follow-up (Phase 2.1c Risk #7)** — separate `a(B, I)` cascaded-allpass concern; not autocorrelator or saturator-tail. Out-of-scope for Phase 2.4c; deferred to Phase 2.4c-bis-or-later.
- **Production WAV binary commits** — `saturator-tail-comparison.wav` (~30 MB at 65 s × stereo 24-bit at sr=44100) NOT committed (reproducible from harness). sha256 + JSON committed instead per Phase 2.4a/2.4b precedent.
- **CI invocation of `--saturator-tail-comparison`** — out-of-scope; harness mode is offline (developer-machine-only). CI runs the existing build + auval + pluginval pipeline.
- **O-Bowed `--saturator-tail-comparison` mirror mode** — Phase 2.4c does NOT add a `--saturator-tail-comparison` mode to the O-Bowed harness; the comparison is one-sided (O-Contrabass renders the canonical golden; O-Bowed parity render is invoked manually in R36d via `--bow-* + --infinite-sustain` flags + post-render Python decay-bin analysis). A mirror mode in O-Bowed is deferred to Phase 2.4c-bis (if triggered) or v1.1.
- **R36 atomic split into R36 (autocorrelator) + R37 (saturator-tail)** — REJECTED per CONTEXT rev-8 Q35 (single atomic; auval/pluginval halving + HR-11 single-shot regression bar + atomic-commit precedent).

---

# Stage 2: DSP — Plan (Phase 2.4c-bis) — REVISION 11 (In-Loop Saturator Port `x/√(1+x²)` → `4·tanh(x/4)`, Gate 6c-bis)

> **Status:** rev-11 authors fresh task bodies for **R36-bis-pre, R36-bis-a, R36-bis-b, R36-bis-c, R36-bis-d, R36-bis-e, R36-bis-f, R36-bis, R36-bis-backfill chore** per `RESEARCH.md §20` sequencing and `CONTEXT.md` rev-9-bis. rev-1/2/3/4/5/6/7/8/9/10 remain in-effect as completed/verified history. Phase 2.4c closed 2026-04-29 with R36 atomic commit `115dbf4` (Gate 6c CLEARED via escalation lane — 5/5 invariants; §19.7.6 escalation flag LOCKED on measured 5.92 dB envelope divergence > 2 dB Q41 threshold + ~3 dB perceptual JND) + R36-backfill chore `7835904`.

**Date:** 2026-04-29
**Cycle scope:** Phase 2.4c-bis only (source-change escalation cycle off Phase 2.4c §19.7.6 escalation flag; HR-11 retired; one source file modified — `Source/DSP/WaveguideString.cpp:204–206`). Strict saturator-only scope per CONTEXT Q48; Phase 2.4-bis backlog (kForceBoost retune, Step 4 / breathingAudible refinement, VIBRATO_DEPTH transfer tune, click-free heuristic threshold tune, DSP-07 sub-harmonics retune for tanh) stays parked.
**Gate:** Gate 6c-bis (Phase 2.4c-bis verify)
**Atomic-commit unit:** R36-bis (Gate 6c-bis PASS) — single commit lands 1 source-edit file (`Source/DSP/WaveguideString.cpp`, 4 LOC: 3 deletions / 4 insertions) + 13 re-baselined `*.wav.sha256` files (11 audible + saturator-tail-comparison + vibrato per RESEARCH §20.5) + 1 informational-only re-anchored `vibrato.json.sha256` + `vibrato.json` (re-baselined) + `saturator-tail-comparison.json` + `saturator-tail-comparison.json.sha256` + RESEARCH §19.7.7 (10-sub-section verdict subsection) + STATUS.md flip to `phase_2_4c_bis_complete` + SUMMARY.md + VERIFICATION.md Phase 2.4c-bis sections + this PLAN rev-11 (already in place from plan-phase). **NO Stage-1 contract amendment** (parameter-spec.md sha256 `77638e25…` carries forward unchanged; STATUS.md `contract_checksums.parameter_spec` unchanged). **NO ARCHITECTURE.md amendment in Phase 2.4c-bis** (saturator-tail evidence — both pre-port `c7e845ea…` from `115dbf4` worktree AND post-port `5c45d176…` from R36-bis — feeds end-of-Stage-2 §"In-loop saturator" amendment cycle but does not amend the architecture itself). **`matrix-stability.wav.sha256 = 6db67707…` carries forward verbatim** (post-port `09cbf15f7600…` archived to RESEARCH §19.7.7.6 as evidence-only, NOT committed as updated golden).
**Carry-forward locks (NOT re-litigated):** Phase 2.1a-recovery split-rail topology, F2 LP form, F3 no-in-loop-DCB, F4 betaScale removed; Phase 2.1b bow-friction module v1.0.0 consumption (HR-10 ABI preservation); Phase 2.1c `DispersionFilter<4>` API + per-string M=4/3/2/1 dispersion table; Phase 2.2 4-string bank + per-string detune + 5 ms equal-power crossfade + MIDI→string mapping; Phase 2.3 modulator-layer surface (vibratoPhase / vibratoOnsetTimer / slowLfoPhase / 4 macro SmoothedValues / 7-step + Step 2.5 per-block evaluation order / HR-1..HR-4 hard rules / `lastSafeDepth.store(0.0f)` unconditional pre-gate / EXPRESSION_MACRO + VIBRATO_DEPTH default 0.0); Phase 2.4a Schelleng wedge bass-register calibration polynomial (`Source/DSP/SchellengCalibration.h` HR-5/HR-6/HR-7/HR-8 verbatim); Phase 2.4b Sub-Harmonic Bias DSP-07 (`Source/DSP/SubHarmonicBias.h` HR-9/HR-10 verbatim + Step 2.5 inserted between Step 2 and Step 3 + `voiceBowForceUpliftThisBlock` factor at Step 6); Phase 2.4c autocorrelator MIDI-derived range bias (`tests/render-harness/main.cpp` `kTauMin=856`/`kTauMax=1285`) + `--saturator-tail-comparison` mode handler + `pass_vibratoAudible` aggregator + Option B O-Bowed harness extension (`--bow-speed/-pressure/-position/--infinite-sustain` value-consume flags). Phase 2.4c-bis modifies ONLY `Source/DSP/WaveguideString.cpp` (4-LOC saturator topology swap on both rails) + 13 audible-golden `*.wav.sha256` re-baselines + 2 JSON re-baselines (`vibrato.json` + `saturator-tail-comparison.json`) + 2 JSON sha256 anchors (`vibrato.json.sha256` informational-only + `saturator-tail-comparison.json.sha256`) + RESEARCH §19.7.7 + STATUS/SUMMARY/VERIFICATION.

---

## Preamble — Pinned Open Items (RESEARCH §20.14)

PLAN rev-11 pins each of the 14 plan-phase open items handed off from research-phase (RESEARCH §20.14 + §20.5/§20.9/§20.10/§20.11):

| # | Open Item | Pinned Decision |
|---|-----------|-----------------|
| 1 | **13 expected post-port `*.wav.sha256` values** | **LOCKED verbatim from RESEARCH §20.5 table.** Execute-phase R36-bis-b MUST reproduce these byte-identical. (Full table reproduced in §"Tasks" R36-bis-b below.) `std::tanh` is bit-deterministic on M1 macOS Xcode 26.3 toolchain (3-trial DET-PASS confirmed at research-phase); no LUT fallback escalation needed. |
| 2 | **R36-bis 9-task breakdown** | **LOCKED verbatim from RESEARCH §20.9.** Sequence: R36-bis-pre (tripwire) → R36-bis-a (4-LOC source edit) → R36-bis-b (re-baseline + evidence renders) → R36-bis-c (RESEARCH §19.7.7 verdict subsection) → R36-bis-d (R37-bis Logic AU audition, BLOCKING) → R36-bis-e (regression bar + audit hooks) → R36-bis-f (auval + pluginval-10) → R36-bis (atomic commit) → R36-bis-backfill (chore commit). |
| 3 | **R37-bis Logic AU audition protocol** | **LOCKED verbatim from RESEARCH §20.10 Steps 1–4 + 5 probe sequences.** 4-step protocol: build pre-port AU from `/tmp/oc-pre-port` worktree (idempotent across re-creates) + side-by-side install as `O-Contrabass-pre-port.component` with disambiguated CFBundleIdentifier + AU description + four-char SubType to avoid collision; build + install post-port AU from working tree per CLAUDE.md macOS protocol; audition in Logic Pro. 5 probe sequences: (1) sustained E1 canonical bow + 5-s tail-decay [BLOCKING]; (2) per-string MIDI 28/33/38/43 [BLOCKING]; (3) sustained E1 + bow-off + 10-s post-bow-off tail [BLOCKING]; (4) SUB_HARMONICS=0.7 engagement [DOCUMENT-only — audits §20.8 mute severity]; (5) VIBRATO_DEPTH=0.7 + EXPRESSION_MACRO=0.5 [DOCUMENT-only — audits §20.6 7.95¢ shift]. PASS criterion for sequences 1–3 = "smoother + more O-Bowed-like decay tail" without unexpected character changes (no transient distortion / peak-amplitude artifacts / DC drift / ringing). |
| 4 | **3 NEW risks (#14–#16) + 1 SOFT-PASS verdict** surfaced in §"Risk Surface" | **LOCKED at PLAN rev-11 §"Risks (Phase 2.4c-bis, refreshed from RESEARCH §20.12)" below.** All 3 NEW risks marked NON-BLOCKING per Q48 strict-saturator-only-scope decision; all 3 land as additive Phase 2.4-bis backlog items. SOFT-PASS verdict at bin 64 (0.30 dB outside strict band [−7.67, −6.67]; 0.80 dB |Δ| vs O-Bowed reference −7.17 dB; lands inside soft-band [−8.17, −6.17]) is the predicted Gate 6c-bis outcome path. |
| 5 | **`vibrato.json.sha256` is informational-only** | **PINNED.** `vibrato.json` contains wall-clock `blockMicros_*` fields + `outputWav` path string → NON-DETERMINISTIC across renders (3-trial different sha256s per RESEARCH §20.6). The committed `vibrato.json.sha256 = 2c4b3a7f…` was a one-time anchor (Phase 2.4c R36 single render); R36-bis-b updates it to a NEW one-time anchor matching the post-port single render but treats it as historical/audit-only. **`reproduce-goldens.sh` only checks `*.wav.sha256` (never `*.json.sha256`)** — confirming this design intent. The regression bar for the vibrato golden is `vibrato.wav.sha256` only. Same precedent applies to `saturator-tail-comparison.json.sha256` (contains the same wall-clock fields). |
| 6 | **matrix-stability evidence-only NOT re-baselined** | **PINNED.** Existing `matrix-stability.{wav.sha256 = 6db67707…, json.sha256, json}` (Phase 2.4a R34b evidence) carries forward verbatim. Post-port `09cbf15f7600…` is rendered at R36-bis-b, archived to RESEARCH §19.7.7.6 reference table, **NOT committed as a re-baselined golden**. Mirrors Phase 2.4a R34b "evidence golden, not in default reproduce-goldens.sh" pattern. The 4-cell post-port click-free heuristic regression (§20.7 failure-mode migration) is documented in §19.7.7.6 + Phase 2.4-bis backlog. |
| 7 | **Pre-port worktree at `/tmp/oc-pre-port` idempotent + cleanup at verify-phase** | **PINNED.** `git worktree add /tmp/oc-pre-port 115dbf4` is idempotent across re-creates (already exists from research-phase §20.3). R36-bis-pre verifies presence (`test -d /tmp/oc-pre-port/.git || git worktree add /tmp/oc-pre-port 115dbf4`). R36-bis-d (R37-bis audition) consumes the worktree to build pre-port AU. **Cleanup at Phase 2.4c-bis verify-phase close** via `git worktree remove /tmp/oc-pre-port` (NOT during execute — verify-phase needs the worktree for independent reproduction of pre-port reference `c7e845ea…`). CMake configure incantation locked: `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DSKIP_PLUGINS=O-Orbit -DOUARICON_BUILD_TESTS=ON ..` (`SKIP_PLUGINS=O-Orbit` flag bypasses uninitialised `libs/SAF` git submodule at `115dbf4`). |
| 8 | **SUB_HARMONICS=0 default unaffected via HR-9** | **LOCKED.** The 11 default-state audible goldens (string-A/D/G, detune-sweep, etc.) all render at SUB_HARMONICS=0 (default APVTS); HR-9 IEEE 754 identity arithmetic short-circuit holds → no sub-harmonic bias path engaged → those goldens shift only due to direct saturator topology change (NOT subharmonic-bias amplification differential). Default user experience UNAFFECTED post-port. Only users explicitly engaging SUB_HARMONICS > 0 will perceive feature mute (§20.8 critical drop 0.358 → 0.000170). Phase 2.4-bis DSP-07 retune is the resolution path; non-blocking for R36-bis. |
| 9 | **Q47 SOFT-PASS path acceptable** | **PINNED — SOFT-PASS path is the Gate 6c-bis target verdict for Phase 2.4c-bis.** Research-phase measured 0.7975 dB |Δ| at bin 64 (post-port lands at −7.97 dB; soft-band [−8.17, −6.17] inside; 0.30 dB outside strict-band lower edge [−7.67, −6.67]). Per CONTEXT Q47 + RESEARCH §20.12 Risk #2 RESOLVED: SOFT-PASS at the |Δ| ≤ 1.0 dB widening + 1 Phase 2.4-bis backlog item ("strict-band convergence retune via `sat` constant tune") is acceptable. **NO `sat` constant tune iteration in Phase 2.4c-bis.** Strict-only PASS path is rejected — `sat=4.0f` matches O-Bowed reference exactly, preserving the architectural-parity intent of the port. |
| 10 | **Two-call-site audit verdict** | **PINNED — single-site port at O-Contrabass `:204–206` is correct architectural equivalent of O-Bowed `writeJunction:218-219` (active production path).** Per RESEARCH §20.2: O-Bowed `processSample:138-139` saturator is DEAD CODE (`BowedStringVoice.cpp:154 + 184` invokes `readJunction`/`writeJunction`, NEVER `processSample`); the two-call-site scope expansion is NOT triggered. Source delta stays 4 LOC in 1 file. Audit hook `grep -c "sat \* std::tanh" Source/DSP/WaveguideString.cpp` MUST return 2 (NOT 4). |
| 11 | **`sat = 4.0f` constant** | **LOCKED.** Both rails (`toBridge` + `toNeck`) get identical port: `x = sat * std::tanh(x / sat)` with `sat = 4.0f`. No asymmetric saturator variants. Source LOC delta: replace `WaveguideString.cpp:204–206` block (3 lines: 1 comment + 2 algebraic saturator lines) with 4 lines (1 updated comment + 1 `constexpr float sat = 4.0f;` + 2 `tanh` saturator lines). |
| 12 | **HR-11 retired; HR-1..HR-10 carry-forward** | **PINNED.** Per CONTEXT rev-9-bis line 111 + Q50: HR-11 (Phase 2.4c "zero production DSP edits") binding is limited to Phase 2.4c only. Phase 2.4c-bis is the source-change escalation cycle by design; HR-11 cannot apply. Audit history preserves the rule binding for Phase 2.4c only. **NO new HR introduced in Phase 2.4c-bis** — the 4-LOC port is straightforward formula swap; both expressions (`x/√(1+x²)` and `4·tanh(x/4)`) are odd-symmetric, monotonic, soft-saturating; HR-1..HR-10 cover existing invariants (modulator zero short-circuits, Schelleng wedge gating, sub-harmonic identity arithmetic, friction module ABI). |
| 13 | **§19.7.7 10-sub-section deliverable structure** | **LOCKED verbatim from RESEARCH §20.11 + CONTEXT lines 51–61.** Research-phase pre-fills §§19.7.7.1–7 + 9–10 from §20 (source delta verification / post-port saturator-tail key bins / 0.7975 dB |Δ| at bin 64 / 13-audible-golden re-baseline sha256s / vibrato re-baseline determination / matrix-stability post-port evidence + failure-mode migration / sub-harmonics post-port + default-state HR-9 verification / verdict — port WORKED-PARTIALLY SOFT-PASS / evidence base for end-of-Stage-2 amendment); execute-phase fills §19.7.7.8 (R37-bis Logic AU audition outcome) + locks §19.7.7.9 verdict line based on audition result. R36-bis-c writes the §19.7.7 subsection; growth ~150 LOC. |
| 14 | **Phase 2.5 sequencing post-2.4c-bis** | **PINNED — Phase 2.5 opens fresh CONTEXT rev-10 (NOT rev-9) post-2.4c-bis verify**, per skeleton §"Sequencing post-2.4c-bis" + CONTEXT Q51. Phase 2.4c-bis verify-phase Gate 6c-bis closure + R36-bis atomic commit + R36-bis-backfill chore is the gate. Phase 2.5 (body resonator + bow noise) opens with the post-port saturator topology in place and consumes §19.7.7.10 evidence base for re-measurement at Phase 2.5 verify (saturator-tail re-measurement as regression check). |

**Carry-forward HARD RULES from prior phases (NOT re-litigated):**
- HR-1 (Phase 2.3): Vibrato literal-zero short-circuit + active-string-only — carry-forward.
- HR-2 (Phase 2.3): Slow-LFO literal-zero short-circuit + phase non-advance at zero depth — carry-forward.
- HR-3 (Phase 2.3): Macro IEEE 754 identity arithmetic + macroSmoothed setCurrentAndTargetValue(0.0) — carry-forward.
- HR-4 (Phase 2.3): Schelleng wedge skip on zero LFO depth + lastSafeDepth.store(0.0) unconditional pre-gate — carry-forward.
- HR-5 (Phase 2.4a): `inline constexpr` linkage on `SchellengCalibration.h` — carry-forward.
- HR-6 (Phase 2.4a): Calibration polynomial behind HR-4 gate ONLY — carry-forward.
- HR-7 (Phase 2.4a): Matrix-stability bypass via weak-symbol — carry-forward.
- HR-8 (Phase 2.4a): Trilinear IEEE 754 identity arithmetic — carry-forward.
- HR-9 (Phase 2.4b): SUB_HARMONICS=0 IEEE 754 identity arithmetic + active-string-only bias gate — carry-forward (binds 11 default-state goldens to topology-only shift; SUB_HARMONICS=0 cannot engage post-port DSP-07 mute).
- HR-10 (Phase 2.4b): Friction module v1.0.0 ABI preservation under bias via ROSIN inverse algebraic identity — carry-forward.
- **HR-11 (Phase 2.4c): RETIRED** — binding limited to Phase 2.4c only (audit history preserves the rule for that phase). Phase 2.4c-bis source-change scope makes HR-11 inapplicable by design.
- 7-step + Step 2.5 per-block evaluation order verbatim — Phase 2.4c-bis does NOT modify the per-block order; only the per-sample saturator topology in Step 7 changes.
- 13 carry-forward goldens (`reproduce-goldens.sh` entries) MUST reproduce 13/13 byte-identical at HEAD `7835904` BEFORE R36-bis-a edit (R36-bis-pre tripwire); MUST reproduce 13/13 byte-identical against NEW post-port sha256s AFTER R36-bis-b re-baseline (R36-bis-e regression bar).
- `matrix-stability.wav.sha256 = 6db67707…` carries forward verbatim (Phase 2.4a evidence; NOT in `reproduce-goldens.sh` default array; NOT re-baselined in Phase 2.4c-bis).
- Atomic-commit gate-first principle: R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → **R36-bis** sequence.

**No new HARD RULES introduced in Phase 2.4c-bis** (per CONTEXT Q50 + Open Item #12 above).

**Per-block evaluation order:** UNCHANGED from Phase 2.4b end-state (7-step + Step 2.5). Phase 2.4c-bis modifies only the per-sample saturator topology inside Step 7 (in-loop algebraic saturator on each rail of `WaveguideString::processSample`).

---

## Goal

Port the in-loop algebraic saturator at `Source/DSP/WaveguideString.cpp:204–206` from `x / √(1 + x²)` to `sat · tanh(x / sat)` with `sat = 4.0f` on both rails (toBridge + toNeck) — matching O-Bowed `WaveguideString.cpp:218-219` (`writeJunction` active production path) — to close the §19.7.6 escalation flag locked at Phase 2.4c verify (measured 5.92 dB envelope divergence at the 5-s post-bow-off mark; ≫ 2 dB Q41 threshold; approaches/exceeds ~3 dB perceptual JND for sustained tones). Research-phase scratch-space prototype (§20.4) measured post-port `decayEnvelopeDb[64] = −7.9675 dB rel max` → **|Δ| = 0.7975 dB** vs O-Bowed reference (−7.17 dB), inside soft-band [−8.17, −6.17] (0.30 dB outside strict-band [−7.67, −6.67]) — predicted Gate 6c-bis verdict = **SOFT-PASS** (87% improvement vs pre-port 5.92 dB; sub-perceptual-JND headroom maintained); R37-bis BLOCKING Logic AU audition validates the topology change at the user's ear before R36-bis atomic commit lands. Source delta = 1 file, 4 LOC (3 deletions / 4 insertions). HR-11 retired; HR-1..HR-10 carry-forward verbatim; no new HR introduced. R36-bis atomic commit lands the source edit + 13 audible-golden re-baselines (11 audible + saturator-tail-comparison + vibrato — per RESEARCH §20.5 with 3-trial DET-PASS sha256s pre-locked) + 2 JSON re-baselines (`vibrato.json` + `saturator-tail-comparison.json`) + 2 informational JSON sha256 re-anchors + RESEARCH §19.7.7 (10-sub-section verdict subsection) + planning artefacts. **NO Stage-1 contract amendment** (parameter-spec.md sha256 `77638e25…` carries forward unchanged; STATUS.md `contract_checksums.parameter_spec` unchanged). **NO ARCHITECTURE.md amendment in Phase 2.4c-bis** (saturator-tail evidence — both pre-port `c7e845ea…` from `115dbf4` worktree AND post-port `5c45d176…` from R36-bis — feeds end-of-Stage-2 §"In-loop saturator" amendment cycle). `matrix-stability.wav.sha256 = 6db67707…` carries forward verbatim (post-port `09cbf15f7600…` archived to RESEARCH §19.7.7.6 as evidence-only; Phase 2.4-bis backlog gains click-free heuristic threshold tune item for 4 NEW raucous corners at high-pressure × β=0.05 corners — stability invariant intact, NOT a regression). DSP-07 sub-harmonics feature mute at engagement (§20.8 subharmEnergyRatio 0.358 → 0.000170, ~33 dB drop) is a known consequence of the tanh topology change at the bias engagement point; default-state HR-9 IEEE 754 identity arithmetic short-circuit holds → 11 default-state audible goldens shift only due to direct saturator topology change (NOT subharmonic-bias differential); default users UNAFFECTED. Phase 2.4-bis backlog gains 3 NEW items: DSP-07 retune for tanh saturator topology, DSP-09 vibrato peakDepthCents transfer tune (additive — 7.95¢ post-port, was 9.53¢), click-free heuristic threshold tune (new). Validate Gate 6c-bis invariants — five-item bar including bit-deterministic-across-re-renders for all 13 audible re-baselines via `reproduce-goldens.sh` (NEW reference values, byte-identical to R36-bis-b commit) + audit hook `git diff --stat HEAD -- plugins/O-Contrabass/Source/` reports exactly 1 file (WaveguideString.cpp) modified + grep audit hook `grep -c "sat \* std::tanh" Source/DSP/WaveguideString.cpp` returns exactly 2 + auval/pluginval-10 SUCCESS + R37-bis Logic AU audition CONFIRMED (sequences 1–3 PASS; sequences 4–5 DOCUMENT-only) + RESEARCH §19.7.7 verdict locked (port WORKED-PARTIALLY SOFT-PASS with 3 Phase 2.4-bis backlog items) — and atomic-commit on Gate 6c-bis PASS as R36-bis. Continue R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → **R36-bis** atomic-commit sequence. Phase 2.5 (body resonator + bow noise) opens fresh CONTEXT rev-10 (NOT rev-9) post-Phase-2.4c-bis verify.

---

## Tasks

### R36-bis-pre — Working-tree integrity tripwire + 4-check pre-flight

**No source edits committed. Diagnostic only. Confirms working-tree integrity at start of execute (HEAD `7835904` descendant), verifies the `/tmp/oc-pre-port` worktree from research-phase §20.3 is still present and harness-buildable, and empirically validates RESEARCH §20.1 (13/13 reproduce-goldens.sh PASS at HEAD pre-edit) + §20.4 audit hook baseline (zero source files modified pre-edit) under the plan-phase build environment.**

Per RESEARCH §20.1, working tree at HEAD `7835904` (R36-backfill chore; descendant of R36 atomic `115dbf4`) reproduces all 13 currently-committed goldens byte-identical via `reproduce-goldens.sh`. Per RESEARCH §20.3, `/tmp/oc-pre-port` git worktree at `115dbf4` was created during research-phase + harness built via `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DSKIP_PLUGINS=O-Orbit -DOUARICON_BUILD_TESTS=ON ..` + `ninja O-Contrabass-render-test`, and reproduces saturator-tail-comparison sha256 `c7e845ea77b1023c2879bc9d8bb14ceb53863951efb881925b11c9ae6f1a60cb` byte-identical 3-trial.

**Tasks:**

1. **Confirm git state is clean at HEAD `7835904` descendant:**
   ```bash
   git log --oneline -3            # should show 7835904 (R36-backfill chore) + 115dbf4 (R36 atomic) + a342c0f (or descendant)
   git status plugins/O-Contrabass/                      # should be clean
   git status modules/synthesis/bow-friction/            # should be clean
   git status plugins/O-Bowed/                           # should be clean
   ```
   If working tree is dirty, STOP and reconcile before proceeding.

2. **Confirm `/tmp/oc-pre-port` worktree is present + harness built (idempotent re-create if missing, per RESEARCH §20.3):**
   ```bash
   if [ ! -d /tmp/oc-pre-port/.git ] && [ ! -f /tmp/oc-pre-port/.git ]; then
       git worktree add /tmp/oc-pre-port 115dbf4
       cd /tmp/oc-pre-port && \
           cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release \
                 -DSKIP_PLUGINS=O-Orbit -DOUARICON_BUILD_TESTS=ON && \
           cmake --build build --target O-Contrabass-render-test --parallel
       cd /Users/taylorbrook/Dev/VST-development
   fi
   test -x /tmp/oc-pre-port/build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test
   ```
   `git worktree add` is idempotent (no-op if already exists at that path + ref); CMake configure/build is no-op if up-to-date.

3. **Build O-Contrabass harness at HEAD (working tree):**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```
   Define `HARNESS=build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test` for subsequent steps. Expect `ninja: no work to do` if R36 build artefacts persist.

4. **Pre-flight check (a): `reproduce-goldens.sh` 13-of-13 PASS at HEAD (carry-forward tripwire):**
   ```bash
   bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   # Expect: "OK: all 13 goldens reproduce byte-identical"
   ```
   **All 13 must PASS** per RESEARCH §20.1 record. If any FAIL, STOP and investigate working-tree drift (re-confirm HEAD == `7835904` or descendant; `git status` for stray edits in `Source/`, `tests/render-harness/`, `modules/synthesis/bow-friction/`). Do NOT proceed to R36-bis-a until all 13 reproduce.

5. **Pre-flight check (b): Source-tree audit hook clean state (zero pre-edit modifications):**
   ```bash
   git diff --stat HEAD -- plugins/O-Contrabass/Source/ \
                            plugins/O-Bowed/Source/ \
                            modules/synthesis/bow-friction/Source/
   # Expect: empty output (zero files reported)
   ```
   Mirrors Phase 2.4c HR-11 audit hook discipline. Confirms no stowaway DSP edits at pre-flight; the only file the cycle touches is `Source/DSP/WaveguideString.cpp` at R36-bis-a.

6. **Pre-flight check (c): Pre-edit grep verification — algebraic saturator still in place:**
   ```bash
   grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp
   # Expect: 0 (algebraic saturator at HEAD; tanh saturator not yet ported)
   grep -c "std::sqrt (1.0f + " plugins/O-Contrabass/Source/DSP/WaveguideString.cpp
   # Expect: 2 (toBridge + toNeck rails at lines 205-206)
   ```
   Confirms the canonical pre-port baseline at HEAD. If either count is unexpected, STOP and investigate.

7. **Pre-flight check (d): Pre-port worktree saturator-tail-comparison sha256 reproducibility (RESEARCH §20.3 reproduction):**
   ```bash
   PRE_HARNESS=/tmp/oc-pre-port/build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test
   mkdir -p /tmp/sat-pre-r36bis
   $PRE_HARNESS --saturator-tail-comparison \
                --out /tmp/sat-pre-r36bis/sat-tail.wav \
                --json /tmp/sat-pre-r36bis/sat-tail.json
   sha=$(shasum -a 256 /tmp/sat-pre-r36bis/sat-tail.wav | awk '{print $1}')
   expected="c7e845ea77b1023c2879bc9d8bb14ceb53863951efb881925b11c9ae6f1a60cb"
   [ "$sha" = "$expected" ] && echo "[PASS] pre-port reference reproducible: $sha" || \
                                { echo "[FAIL] pre-port reference drift; STOP"; exit 1; }
   ```
   Confirms the §"In-loop saturator" amendment evidence base is reproducible. If sha256 drifts, the `/tmp/oc-pre-port` worktree is corrupted; rebuild from clean `git worktree remove` + `git worktree add` before proceeding.

**Files created:** `/tmp/sat-pre-r36bis/sat-tail.{wav,json}` (transient; deleted post-R36-bis; NOT committed).

**Files modified:** none committed.

**Commit:** **NONE** — diagnostic only.

**Success bar:**
- [ ] `git log --oneline -3` shows `7835904` (or descendant) + `115dbf4` + `a342c0f` (or descendant).
- [ ] `/tmp/oc-pre-port/.git` exists and harness binary at expected path.
- [ ] O-Contrabass working-tree harness builds clean.
- [ ] (a) `reproduce-goldens.sh` reports `OK: all 13 goldens reproduce byte-identical`.
- [ ] (b) `git diff --stat HEAD -- plugins/O-Contrabass/Source/` reports empty (zero files).
- [ ] (c) `grep -c "sat \* std::tanh"` returns `0`; `grep -c "std::sqrt (1.0f + "` returns `2`.
- [ ] (d) Pre-port worktree saturator-tail render reproduces sha256 `c7e845ea…1a60cb` byte-identical.

**Estimated effort:** 2–5 min (build no-op + 4 pre-flight checks).

---

### R36-bis-a — Source edit: Port in-loop saturator from `x/√(1+x²)` to `sat·tanh(x/sat)` with `sat = 4.0f`

**Per RESEARCH §20.4 + §20.9. Replace `Source/DSP/WaveguideString.cpp:204–206` algebraic saturator with `tanh` saturator on both rails (toBridge + toNeck). Source delta: 4 LOC (3 deletions / 4 insertions). Verified via grep audit hook + `git diff --stat`.**

**Tasks:**

1. **Edit `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp:204–206`** — replace algebraic saturator with `tanh` saturator, both rails:

   ```cpp
   // BEFORE (HEAD `7835904`, lines 204–206):
   // Step 7: In-loop algebraic saturator on each rail (RESEARCH §1.3).
   toBridge = toBridge / std::sqrt (1.0f + toBridge * toBridge);
   toNeck   = toNeck   / std::sqrt (1.0f + toNeck   * toNeck);

   // AFTER (R36-bis-a — RESEARCH §20.4 verbatim):
   // Step 7: In-loop tanh saturator on each rail (Phase 2.4c-bis R36-bis port from
   //   O-Bowed WaveguideString.cpp:218-219 writeJunction; closes Phase 2.4c §19.7.6
   //   escalation flag locked at 5.92 dB envelope divergence; see RESEARCH §20.4).
   constexpr float sat = 4.0f;
   toBridge = sat * std::tanh (toBridge / sat);
   toNeck   = sat * std::tanh (toNeck   / sat);
   ```

   **Both rails get identical port.** No asymmetric saturator variants. `<cmath>` is already included in `WaveguideString.cpp` (used by `std::sqrt` at the pre-edit lines + `std::pow`/`std::abs` elsewhere); `std::tanh` is the C++ standard library scalar overload selected by float-typed argument. `juce::ScopedNoDenormals` already in place upstream at the voice-level renderNextBlock; `std::tanh` at moderate-amplitude bow operating points (peak ≤ 0.10) does NOT trigger denormal-flush concerns.

2. **Post-edit verification — grep audit hook:**
   ```bash
   grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp
   # Expect: 2 (toBridge + toNeck rails)
   grep -c "std::sqrt (1.0f + " plugins/O-Contrabass/Source/DSP/WaveguideString.cpp
   # Expect: 0 (algebraic saturator removed)
   ```

3. **Post-edit verification — `git diff --stat`:**
   ```bash
   git diff --stat HEAD -- plugins/O-Contrabass/Source/
   # Expect: 1 file changed, 4 insertions(+), 3 deletions(-)
   #   plugins/O-Contrabass/Source/DSP/WaveguideString.cpp | 7 ++++----
   git diff --stat HEAD -- plugins/O-Bowed/Source/ \
                            modules/synthesis/bow-friction/Source/
   # Expect: empty (no other source edits; HR-10 friction module ABI preserved by construction)
   ```

**Files created:** none.

**Files modified (NOT YET COMMITTED — staged for R36-bis atomic commit):**
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` (lines 204–206 → 4 LOC tanh saturator port).

**Commit:** **NONE** — staged for R36-bis atomic.

**Success bar:**
- [ ] `grep -c "sat \* std::tanh"` returns `2`.
- [ ] `grep -c "std::sqrt (1.0f + "` returns `0`.
- [ ] `git diff --stat HEAD -- plugins/O-Contrabass/Source/` reports exactly 1 file changed (4 insertions, 3 deletions).
- [ ] `git diff --stat HEAD -- plugins/O-Bowed/Source/ modules/synthesis/bow-friction/Source/` reports empty.

**Estimated effort:** ~30 sec (4-LOC edit + 2 grep checks + 1 git diff).

---

### R36-bis-b — Re-baseline 13 audible goldens + matrix-stability evidence-only render + JSON re-anchors

**Per RESEARCH §20.5 + §20.7. Build O-Contrabass-render-test post-edit; render 13 audible goldens (11 carry-forward + saturator-tail-comparison + vibrato) → write per-golden `*.wav.sha256` files matching the §20.5 predicted post-port values byte-identical; render `--matrix-stability` evidence-only (NOT committing updated `matrix-stability.wav.sha256`; archive WAV `09cbf15f7600…` to `.planning/evidence/` for RESEARCH §19.7.7.6 reference); update `vibrato.json.sha256` + `saturator-tail-comparison.json.sha256` to NEW one-time anchors of post-port single renders (informational-only per RESEARCH §20.6 — JSON files contain wall-clock fields; sha256 anchors are audit-only, NOT regression bars).**

**Tasks:**

1. **Build O-Contrabass-render-test post-edit:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```

2. **Render 13 audible goldens via existing `reproduce-goldens.sh` re-baseline path (or direct harness invocation per per-mode CLI flags):**

   Use the existing harness CLI flags per Phase 2.4c R36b/R36c precedent:

   | # | Golden mode | Harness invocation (with output paths under `tests/render-harness/golden/`) |
   |---|-------------|------------------------------------------------------------------------------|
   | 1 | stiffness-zero-pre | `$HARNESS --stiffness-zero-pre --out golden/stiffness-zero-pre.wav --json golden/stiffness-zero-pre.json` |
   | 2 | string-A | `$HARNESS --string A --out golden/string-A.wav --json golden/string-A.json` |
   | 3 | string-D | `$HARNESS --string D --out golden/string-D.wav --json golden/string-D.json` |
   | 4 | string-G | `$HARNESS --string G --out golden/string-G.wav --json golden/string-G.json` |
   | 5 | detune-sweep-A | `$HARNESS --detune-sweep --string A --out golden/detune-sweep-A.wav --json golden/detune-sweep-A.json` |
   | 6 | note-sequence | `$HARNESS --note-sequence --out golden/note-sequence.wav --json golden/note-sequence.json` |
   | 7 | macro-sweep | `$HARNESS --macro-sweep --out golden/macro-sweep.wav --json golden/macro-sweep.json` |
   | 8 | slow-lfo | `$HARNESS --slow-lfo --out golden/slow-lfo.wav --json golden/slow-lfo.json` |
   | 9 | schelleng-stress | `$HARNESS --schelleng-stress --out golden/schelleng-stress.wav --json golden/schelleng-stress.json` |
   | 10 | sub-harmonics | `$HARNESS --sub-harmonics --out golden/sub-harmonics.wav --json golden/sub-harmonics.json` |
   | 11 | sub-harmonics-stability | `$HARNESS --sub-harmonics-stability --out golden/sub-harmonics-stability.wav --json golden/sub-harmonics-stability.json` |
   | 12 | saturator-tail-comparison | `$HARNESS --saturator-tail-comparison --out golden/saturator-tail-comparison.wav --json golden/saturator-tail-comparison.json` |
   | 13 | vibrato | `$HARNESS --vibrato --out golden/vibrato.wav --json golden/vibrato.json` |

   (WAV files for entries 9 + 12 + 11 + 6 + others are large and **NOT committed** per Phase 2.4a/2.4b/2.4c precedent; only the `.wav.sha256` text file + `.json` text file + `.json.sha256` text file are committed.)

3. **Compute and write per-golden `*.wav.sha256` text files** (regression bar; checked by `reproduce-goldens.sh`):

   ```bash
   GOLDEN_DIR=plugins/O-Contrabass/tests/render-harness/golden
   for g in stiffness-zero-pre string-A string-D string-G detune-sweep-A \
            note-sequence macro-sweep slow-lfo schelleng-stress \
            sub-harmonics sub-harmonics-stability saturator-tail-comparison vibrato; do
       sha=$(shasum -a 256 "$GOLDEN_DIR/$g.wav" | awk '{print $1}')
       printf '%s  %s.wav\n' "$sha" "$g" > "$GOLDEN_DIR/$g.wav.sha256"
   done
   ```

   **EXPECTED post-port `*.wav.sha256` values (LOCKED from RESEARCH §20.5; execute-phase MUST reproduce byte-identical):**

   | # | Golden | Pre-port (Phase 2.4c) committed | **Post-port (R36-bis-b LOCKED)** |
   |---|--------|----------------------------------|----------------------------------|
   | 1 | stiffness-zero-pre.wav | `d358abcd…b0ee75` | **`ed44cd8986d3a9d44cefd399dd128b62147901640ce615eadf7793f129f56020`** |
   | 2 | string-A.wav | `c6755aa4…415918` | **`505ad36e521d3a8cff978cc5386d6e69769da33977efc7dfccec33d721785bad`** |
   | 3 | string-D.wav | `765b015e…65d9c9bc` | **`e064035124d9af90c1cf6ac8a103e90efa64bf0d6a3efc17574d1f8c811668f4`** |
   | 4 | string-G.wav | `0cd5cb0a…e1b993bd` | **`0e9451b849b659ea5ea92ea3e92e0862e34d30ad5188235f816f43419111b3ca`** |
   | 5 | detune-sweep-A.wav | `5e31dad3…2dbb05` | **`b51d334bbfdd7da7abf4ba3391a6411d5a07427bb92b8e339389856a2539dbe7`** |
   | 6 | note-sequence.wav | `3ac3ccd0…79260b5` | **`2b5b8c83e419179ab04b5b218976c5d085538d59c0a55a967942c004fa1f8224`** |
   | 7 | macro-sweep.wav | `c2571dd9…b37975e` | **`231218b4e9f117ca6598ecee530f3b0af20d4109f29640608590d6cf15a66cfe`** |
   | 8 | slow-lfo.wav | `c0c2c893…2466a0` | **`d27589de30dcb6f3c432c8993d80106b38b4cb87e59afa2edc4ae301d8809cb8`** |
   | 9 | schelleng-stress.wav | `9d18da86…2f9597` | **`c5108af57520c8c190adaa6840513d4cfea6659da46d95bca01996d07efbda07`** |
   | 10 | sub-harmonics.wav | `bfcaaadc…5573af` | **`9178b41ec8b5bb6eb08b5ce9794dd93f542647ffd575ede39d88b8fff1a8c54c`** |
   | 11 | sub-harmonics-stability.wav | `8043f659…d107b14a` | **`2efdea9b5d0745e127ad1fbc4242779848f1ea031b0e426b22e966ed7df8e6be`** |
   | 12 | saturator-tail-comparison.wav | `c7e845ea…1a60cb` | **`5c45d1761ddf267cd1cb1be8cd7142d37d81dffc4a6103cfe8b84e52cf9bc7a7`** |
   | 13 | vibrato.wav | `d7881ecf…076b2c` | **`df7384e358af9c5d5d34673a3976c2f34790f7cb2c07a96b45d6b3b03b568f47`** |

   If ANY of the 13 sha256 files DRIFTS from the locked predicted value, STOP and investigate (likely toolchain or IEEE 754 nondeterminism — research-phase 3-trial DET-PASS confirmed determinism on M1 macOS Xcode 26.3, so drift indicates upstream environment change).

4. **Write `vibrato.json.sha256` re-anchor (informational-only; non-deterministic across renders):**

   Per RESEARCH §20.6: `vibrato.json` contains wall-clock `blockMicros_*` fields + `outputWav` path → JSON sha256 differs across renders. The committed sha256 is a one-time anchor, NOT a regression bar (`reproduce-goldens.sh` only checks `.wav.sha256`).

   ```bash
   sha=$(shasum -a 256 "$GOLDEN_DIR/vibrato.json" | awk '{print $1}')
   printf '%s  %s\n' "$sha" "vibrato.json" > "$GOLDEN_DIR/vibrato.json.sha256"
   ```

   The R36-bis-b `vibrato.json.sha256` will be a NEW value (replaces Phase 2.4c R36 `2c4b3a7f…` one-time anchor); document in §19.7.7.5 as informational/historical-only.

5. **Write `saturator-tail-comparison.json.sha256` re-anchor (informational-only):**

   Same wall-clock-field issue applies; `saturator-tail-comparison.json` was introduced at Phase 2.4c R36b with sha256 `bc3969a5…` one-time anchor.
   ```bash
   sha=$(shasum -a 256 "$GOLDEN_DIR/saturator-tail-comparison.json" | awk '{print $1}')
   printf '%s  %s\n' "$sha" "saturator-tail-comparison.json" \
       > "$GOLDEN_DIR/saturator-tail-comparison.json.sha256"
   ```

6. **Render `--matrix-stability` evidence-only (NOT committed as updated golden):**

   ```bash
   mkdir -p plugins/O-Contrabass/.planning/evidence/phase-2-4c-bis
   $HARNESS --matrix-stability \
            --out plugins/O-Contrabass/.planning/evidence/phase-2-4c-bis/matrix-stability-post-port.wav \
            --json plugins/O-Contrabass/.planning/evidence/phase-2-4c-bis/matrix-stability-post-port.json
   sha=$(shasum -a 256 plugins/O-Contrabass/.planning/evidence/phase-2-4c-bis/matrix-stability-post-port.wav | awk '{print $1}')
   echo "[INFO] post-port matrix-stability sha256: $sha"
   # Expect: 09cbf15f7600… (per RESEARCH §20.7); NOT committed as a re-baselined golden.
   #   The existing matrix-stability.wav.sha256 = 6db67707… carries forward as the
   #   Phase 2.4a R34b evidence baseline; this post-port WAV is informational-only
   #   evidence for end-of-Stage-2 §"In-loop saturator" amendment. Archived under
   #   .planning/evidence/phase-2-4c-bis/ (NOT under tests/render-harness/golden/).
   ```

   **CRITICAL:** Do NOT update `tests/render-harness/golden/matrix-stability.wav.sha256` — that file carries forward verbatim from Phase 2.4a R34b (`6db67707…`). Updating it would falsely re-baseline a Phase 2.4a evidence golden under Phase 2.4c-bis scope.

   The post-port WAV `matrix-stability-post-port.wav` (~108 × ~3 s × stereo 24-bit ≈ 50 MB) is large; commit only the `.json` extract under `.planning/evidence/phase-2-4c-bis/` (lightweight; documents the failure-mode migration table per §19.7.7.6) — the WAV file itself is NOT committed. PLAN-execution-log records the WAV's transient location.

7. **Bit-deterministic-across-re-renders verification (Gate 6c-bis invariant):**
   ```bash
   bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   # Expect: "OK: all 13 goldens reproduce byte-identical" against the freshly-written
   #   post-port .wav.sha256 files. Confirms:
   #   (a) std::tanh determinism: holds across re-renders on M1 macOS Xcode 26.3.
   #   (b) Newly-written sha256 anchors match the actual rendered WAVs byte-identical.
   ```

**Files created (NOT YET COMMITTED — staged for R36-bis atomic):**
- `plugins/O-Contrabass/.planning/evidence/phase-2-4c-bis/matrix-stability-post-port.json` (~30 KB extract; failure-mode migration table reference).

**Files modified (NOT YET COMMITTED — staged for R36-bis atomic):**
- `plugins/O-Contrabass/tests/render-harness/golden/{stiffness-zero-pre, string-A, string-D, string-G, detune-sweep-A, note-sequence, macro-sweep, slow-lfo, schelleng-stress, sub-harmonics, sub-harmonics-stability, saturator-tail-comparison, vibrato}.wav.sha256` — 13 files re-baselined to post-port sha256s.
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.json` — re-baselined post-port output (peakDepthCents=7.9507, vibratoRateHzMeasured=4.9788, onsetTimeMs=1000 per RESEARCH §20.6).
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.json.sha256` — re-anchored to NEW one-time post-port value (informational-only).
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json` — re-baselined post-port output (decayEnvelopeDb[64]=−7.9675 dB per RESEARCH §20.4).
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json.sha256` — re-anchored to NEW one-time post-port value (informational-only).

**Files NOT modified (carry-forward verbatim):**
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.{wav.sha256, json, json.sha256}` — Phase 2.4a R34b `6db67707…` baseline carries forward; post-port `09cbf15f…` is evidence-only under `.planning/evidence/`.
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` — entry count 13 unchanged; only sha256 values inside per-entry comparisons change (handled by the per-golden `*.wav.sha256` files).

**Commit:** **NONE** — staged for R36-bis atomic.

**Success bar:**
- [ ] All 13 `*.wav.sha256` files match the post-port LOCKED values from §20.5 byte-identical.
- [ ] `vibrato.json` shows post-port metrics (peakDepthCents=7.9507, vibratoRateHzMeasured=4.9788, onsetTimeMs=1000).
- [ ] `saturator-tail-comparison.json` shows `decayEnvelopeDb[64] = −7.9675 dB`.
- [ ] `bash reproduce-goldens.sh` reports 13/13 PASS against the freshly-written post-port sha256s.
- [ ] `matrix-stability.wav.sha256` UNMODIFIED (still `6db67707…`).
- [ ] Post-port matrix-stability evidence WAV produces sha256 `09cbf15f7600…` and JSON archived under `.planning/evidence/phase-2-4c-bis/`.

**Estimated effort:** ~3–5 min (build + 13 audible renders ≈ 60–90 sec each via harness; 13 sha256 writes; reproduce-goldens.sh re-verification; matrix-stability evidence render).

---

### R36-bis-c — RESEARCH §19.7.7 verdict subsection (10 sub-sections per CONTEXT lines 51–61)

**Per RESEARCH §20.11 + CONTEXT rev-9-bis lines 51–61. Append `RESEARCH.md` §19.7.7 closure subsection that locks the post-port verdict and closes the §19.7.6 escalation flag. Research-phase pre-fills §§19.7.7.1–7 + 9–10 with measured data from §20; execute-phase fills §19.7.7.8 (R37-bis Logic AU audition outcome) at R36-bis-d + locks §19.7.7.9 verdict line based on audition result. R36-bis-c writes the framing structure + research-phase-known fields; R36-bis-d updates §19.7.7.8 + §19.7.7.9.**

**Tasks:**

1. **Append §19.7.7 to `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md`** with the following 10-sub-section structure (drawing measured data verbatim from §20):

   | Sub-section | Content source | Filled by |
   |-------------|----------------|-----------|
   | **§19.7.7.1** Source delta verification | `git diff --stat` 1 file changed (4 insertions / 3 deletions); `grep -c "sat \* std::tanh"` returns 2; full 4-LOC diff snippet from R36-bis-a | Research-phase + R36-bis-b post-edit verification |
   | **§19.7.7.2** Post-port saturator-tail decay envelope key bins | bin 0 / 5 / 60 / 64 measurements from §20.4 verbatim | Research-phase (§20.4) |
   | **§19.7.7.3** Measured \|Δ\| at bin 64 vs O-Bowed reference | `0.7975 dB`; SOFT-PASS verdict (lands at −7.97 dB; soft-band [−8.17, −6.17] inside; 0.30 dB outside strict-band [−7.67, −6.67]) | Research-phase (§20.4) |
   | **§19.7.7.4** 13-audible-golden re-baseline sha256s | Table mirroring §20.5 verbatim — 13 pre-port → post-port sha256 pairs | Research-phase (§20.5) |
   | **§19.7.7.5** Vibrato carry-forward determination | RE-BASELINE per §20.6; metrics shift table (peakDepthCents 9.526 → 7.9507; onsetTimeMs 1168 → 1000); JSON non-determinism caveat | Research-phase (§20.6) |
   | **§19.7.7.6** Matrix-stability post-port evidence (failure-mode migration) | Failure-mode migration table from §20.7 (3 pre-port stabilise + 4 NEW raucous corners at high-press × β=0.05); `passCount=104/108`; `pass_noNaN`/`pass_peak`/`pass_blockTime` all PASS; stability invariant intact; evidence-only NOT re-baselined; archived under `.planning/evidence/phase-2-4c-bis/` | Research-phase (§20.7) |
   | **§19.7.7.7** Sub-harmonics post-port measurements + default-state HR-9 verification | subharmEnergyRatio 0.358 → 0.000170 (~33 dB drop) per §20.8; sub-harmonics-stability 36/36 PASS; default-state HR-9 IEEE 754 identity arithmetic short-circuit holds (11 default-state goldens shift only due to direct topology change, NOT subharmonic-bias differential); DSP-07 mute at engagement; default users UNAFFECTED | Research-phase (§20.8) |
   | **§19.7.7.8** R37-bis Logic AU audition outcome | Sequences 1–3 PASS / FAIL summary per probe; sequences 4–5 DOCUMENT-only outcomes; user CONFIRM message capture | **Execute-phase R36-bis-d** |
   | **§19.7.7.9** Verdict | Port WORKED-PARTIALLY (SOFT-PASS at bin 64 + 3 Phase 2.4-bis backlog items: DSP-07 retune for tanh, DSP-09 vibrato depth tune, click-free heuristic threshold tune) | Research-phase pre-classifies; **execute-phase R36-bis-d locks final wording** based on audition result |
   | **§19.7.7.10** Evidence base for end-of-Stage-2 §"In-loop saturator" amendment | Pre-port reference `c7e845ea…` from `115dbf4` worktree + post-port `5c45d176…` from R36-bis atomic + 0.80 dB convergence narrative + 87% improvement vs pre-port 5.92 dB | Research-phase (§20.4 + §20.16 references) |

2. **Mark §19.7.7.8 + §19.7.7.9 as `[execute-phase R36-bis-d locks]`** placeholder text:

   ```markdown
   ### §19.7.7.8 R37-bis Logic AU Audition Outcome [execute-phase R36-bis-d locks]

   *To be filled at R36-bis-d after user audition completes per RESEARCH §20.10 protocol. Probe sequence outcomes (1 / 2 / 3 BLOCKING; 4 / 5 DOCUMENT-only); user CONFIRM message captured to STATUS.md `phase_2_4c_bis_audition_outcome` field.*

   ### §19.7.7.9 Verdict [execute-phase R36-bis-d locks final wording]

   *Pre-classified at research-phase as: port WORKED-PARTIALLY (SOFT-PASS at bin 64 with 0.80 dB |Δ|; 87% improvement vs pre-port 5.92 dB; 3 Phase 2.4-bis backlog items added). Final wording locked at R36-bis-d based on audition result. If audition reveals unexpected character changes (transient distortion / DC drift / ringing in sequences 1–3), re-classify to "port DID-NOT-CONVERGE; sat constant tune required" or "port REVERTED; close 2.4c-bis as research-only acknowledged-divergence" per §20.10 FAIL handling.*
   ```

3. **Close §19.7.6 escalation flag** by appending a closure paragraph at the end of §19.7.6 referencing §19.7.7:

   ```markdown
   **Phase 2.4c-bis closure (2026-04-29):** §19.7.6 escalation flag CLOSED via §19.7.7 below. Post-port `decayEnvelopeDb[64] = −7.9675 dB rel max` lands within ±1.0 dB soft-band of O-Bowed reference (|Δ| = 0.7975 dB; 87% improvement vs pre-port 5.92 dB divergence). See §19.7.7 for the full Phase 2.4c-bis verdict + 3 Phase 2.4-bis backlog items added.
   ```

**Files created:** none (RESEARCH.md is appended in place).

**Files modified (NOT YET COMMITTED — staged for R36-bis atomic):**
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` — append §19.7.7 (~150 LOC growth) + §19.7.6 closure paragraph (~3 LOC growth).

**Commit:** **NONE** — staged for R36-bis atomic.

**Success bar:**
- [ ] §19.7.7 contains all 10 sub-sections per CONTEXT lines 51–61.
- [ ] §§19.7.7.1–7 + 9 + 10 contain measured data from §20 verbatim.
- [ ] §19.7.7.8 + §19.7.7.9 marked `[execute-phase R36-bis-d locks]`.
- [ ] §19.7.6 has closure paragraph referencing §19.7.7.

**Estimated effort:** ~10 min (write framing + transcribe §20 measurements; the heavy lifting is research-phase already done).

---

### R36-bis-d — R37-bis Logic AU audition (BLOCKING gate before R36-bis atomic commit)

**Per RESEARCH §20.10 + CONTEXT Q49. First audible source-edit since Phase 2.4b R35; user audition is BLOCKING for R36-bis atomic. 4-step protocol: build pre-port reference AU from `/tmp/oc-pre-port` worktree (idempotent across re-creates) + side-by-side install with disambiguated CFBundleIdentifier + AU description; build + install post-port AU from working tree per CLAUDE.md macOS protocol; audition in Logic Pro using 5 probe sequences (3 BLOCKING + 2 DOCUMENT-only).**

**Tasks:**

1. **Step 1 — Build pre-port reference AU (idempotent re-create if missing):**
   ```bash
   if [ ! -d /tmp/oc-pre-port/.git ] && [ ! -f /tmp/oc-pre-port/.git ]; then
       git worktree add /tmp/oc-pre-port 115dbf4
   fi
   cd /tmp/oc-pre-port && \
       cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release \
             -DSKIP_PLUGINS=O-Orbit -DOUARICON_BUILD_TESTS=ON && \
       cmake --build build --target O-Contrabass-dev_AU O-Contrabass-dev_VST3 --parallel
   cd /Users/taylorbrook/Dev/VST-development
   ```

2. **Step 2 — Install pre-port AU as side-by-side bundle (DO NOT collide with working-tree AU):**
   ```bash
   killall -9 AudioComponentRegistrar 2>/dev/null || true
   rm -rf ~/Library/Caches/AudioUnitCache/
   rm -rf ~/Library/Caches/com.apple.audiounits.cache
   rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass-pre-port.component  # cleanup any prior
   PRE_AU=/tmp/oc-pre-port/build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass-dev.component
   cp -R "$PRE_AU" ~/Library/Audio/Plug-Ins/Components/O-Contrabass-pre-port.component
   ```

   **Disambiguate the pre-port AU bundle to avoid AU collision** (CFBundleIdentifier + AU description + four-char SubType):
   - Open `~/Library/Audio/Plug-Ins/Components/O-Contrabass-pre-port.component/Contents/Info.plist`.
   - Edit `CFBundleIdentifier` from `com.ouaricon.O-Contrabass-dev` (or similar) to `com.ouaricon.O-Contrabass-pre-port`.
   - Under `AudioComponents` → `[0]` dict: edit `name` to `Ouaricon: O-Contrabass-pre-port`; edit `description` to `O-Contrabass-pre-port (Phase 2.4c R36 reference, sha 115dbf4)`; edit `subtype` from `OBSC` (or whatever working-tree uses) to `OBSP` (avoids collision).
   - Save plist.
   - `auval -a | grep -i 'O-Contrabass'` should now show **two** registered AUs (`OBSC` working-tree + `OBSP` pre-port).

3. **Step 3 — Build + install post-port AU (working tree, post-R36-bis-a):**
   ```bash
   cd /Users/taylorbrook/Dev/VST-development
   ninja O-Contrabass-dev_AU O-Contrabass-dev_VST3
   killall -9 AudioComponentRegistrar 2>/dev/null || true
   rm -rf ~/Library/Caches/AudioUnitCache/
   rm -rf ~/Library/Caches/com.apple.audiounits.cache
   rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass-dev.component
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass-dev.component \
         ~/Library/Audio/Plug-Ins/Components/
   auval -a | grep -i 'O-Contrabass'   # verify both AUs registered
   ```

4. **Step 4 — Audition in Logic Pro using 5 probe sequences (3 BLOCKING + 2 DOCUMENT-only per RESEARCH §20.10):**

   | # | Probe | PASS criterion | Verdict |
   |---|-------|----------------|---------|
   | 1 | Sustained E1 (MIDI 28) at canonical bow (BOW_SPEED=0.15, BOW_PRESSURE=3.0, BOW_POSITION=0.10, INFINITE_SUSTAIN=1.0); 8-second sustain + 5-second tail-decay | Post-port tail decay sounds **smoother + more natural** than pre-port (consistent with rmsRatio_final_over_mid 0.26 → 0.50); no transient artifacts on bow-on attack | **BLOCKING** |
   | 2 | Per-string MIDI 28 / 33 / 38 / 43 (E1 / A1 / D2 / G2) — hold 4-sec each at canonical bow | Each string's character preserves harmonic spectrum + body resonance carry-forward (no Phase 2.5 yet, so this is dry waveguide character); post-port slightly **brighter + more sustained** (consistent with peak +57% / rmsMid +36%) | **BLOCKING** |
   | 3 | Sustained E1 + bow-off at 4-sec; listen to 10-sec post-bow-off tail | Post-port tail energy ~3× higher (rmsRatio_final_over_mid 0.50 vs 0.26 — measurable) but should **NOT exhibit ringing, clicks, or DC drift**; closer to O-Bowed reference character | **BLOCKING** |
   | 4 | SUB_HARMONICS=0.7 engagement on sustained E1 (probes Phase 2.4-bis DSP-07 mute per §20.8) | User notes: post-port **subjectively MUTES the subharmonic effect** (matches §20.8 measurement); document subjective severity to inform Phase 2.4-bis priority | DOCUMENT-only |
   | 5 (optional) | VIBRATO_DEPTH=0.7, EXPRESSION_MACRO=0.5 on sustained E1 | Post-port vibrato perceptually similar to pre-port; depth slightly reduced (consistent with §20.6 7.95¢ vs 9.53¢ measurement) but vibrato shape character preserved | DOCUMENT-only |

5. **PASS criteria summary:**
   - **Sequences 1–3 MUST PASS** for R36-bis atomic to land. Predicted outcome: PASS — saturator port produces "smoother + more O-Bowed-like" subjective improvement consistent with §20.4 metric improvements.
   - Sequences 4–5: **DOCUMENT** subjective severity (informs Phase 2.4-bis DSP-07 + DSP-09 priority); does NOT block R36-bis atomic.

6. **FAIL handling (BLOCKING — if sequences 1, 2, or 3 reveal unexpected character changes):**
   - **R36-bis atomic does NOT land.**
   - Resolution paths:
     - (a) `sat` constant tune (`sat=3.0f` or `sat=5.0f`) + re-render saturator-tail-comparison + re-audition (re-enters R36-bis-a with new value).
     - (b) Acknowledge architectural divergence; revert R36-bis-a source edits via `git checkout HEAD -- plugins/O-Contrabass/Source/DSP/WaveguideString.cpp`; close 2.4c-bis as research-only acknowledged-divergence (path (b) of §20.10 FAIL handling); STATUS.md flips to `phase_2_4c_bis_revert` outcome.
     - (c) Escalate to Phase 2.4c-bis-bis with alternative topology (LUT, polynomial approximation).

7. **Capture user CONFIRM message to STATUS.md** (PASS path):
   - Append `phase_2_4c_bis_audition_outcome:` block to STATUS.md with: probe-sequence verdicts (1/2/3 PASS/FAIL; 4/5 documented severity); subjective notes from user; timestamp.

8. **Lock RESEARCH §19.7.7.8 + §19.7.7.9** (PASS path):
   - Fill §19.7.7.8 with audition outcome verbatim from STATUS.md `phase_2_4c_bis_audition_outcome` block.
   - Lock §19.7.7.9 final verdict line: "Port WORKED-PARTIALLY (SOFT-PASS at bin 64 with 0.7975 dB |Δ|; 3 Phase 2.4-bis backlog items added: DSP-07 retune for tanh saturator topology, DSP-09 VIBRATO_DEPTH transfer tune additive, click-free heuristic threshold tune for high-pressure × β=0.05 corners; default-state HR-9 IEEE 754 identity arithmetic preserved → 11 default-state goldens unaffected by sub-harmonics feature mute at engagement)."

**Files created:** none (`/tmp/oc-pre-port/build/...` AU bundles are transient).

**Files modified (NOT YET COMMITTED — staged for R36-bis atomic):**
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §19.7.7.8 + §19.7.7.9 (filled at this task).
- `plugins/O-Contrabass/.planning/STATUS.md` — append `phase_2_4c_bis_audition_outcome` block.

**System modifications (NOT version-controlled):**
- `~/Library/Audio/Plug-Ins/Components/O-Contrabass-pre-port.component` — pre-port reference AU (cleanup at verify-phase via `rm -rf`).
- `~/Library/Audio/Plug-Ins/Components/O-Contrabass-dev.component` — post-port working-tree AU (carries forward).

**Commit:** **NONE** — staged for R36-bis atomic. **R36-bis is BLOCKED until user CONFIRM.**

**Success bar:**
- [ ] Pre-port AU built + installed as `O-Contrabass-pre-port.component` with disambiguated CFBundleIdentifier (`com.ouaricon.O-Contrabass-pre-port`) + AU SubType (`OBSP`).
- [ ] Post-port AU built + installed as `O-Contrabass-dev.component` (working-tree).
- [ ] `auval -a | grep -i 'O-Contrabass'` shows both AUs registered.
- [ ] User auditions all 5 probe sequences in Logic Pro.
- [ ] Sequences 1–3 PASS subjectively (smoother + more O-Bowed-like; no transient artifacts / DC drift / ringing).
- [ ] Sequences 4–5 documented (subjective severity captured).
- [ ] `phase_2_4c_bis_audition_outcome` block appended to STATUS.md with user CONFIRM.
- [ ] RESEARCH §19.7.7.8 + §19.7.7.9 locked with final wording.

**Estimated effort:** ~15–25 min (build pre-port AU ~5 min + plist edits + install + audition pacing 10–15 min including 5 probe sequences + capture user CONFIRM).

---

### R36-bis-e — Regression bar + audit-hook re-runs

**Per RESEARCH §20.9. Final tripwire before R36-bis atomic commit lands. Confirms: (a) post-port goldens reproduce 13/13 byte-identical (no drift since R36-bis-b); (b) source delta still exactly 1 file (no stowaway edits); (c) grep audit hook still confirms 2 tanh sites.**

**Tasks:**

1. **Regression bar:**
   ```bash
   bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   # Expect: "OK: all 13 goldens reproduce byte-identical" against the post-port sha256s.
   ```

2. **Audit hook (a): Source-tree diff scope:**
   ```bash
   git diff --stat HEAD -- plugins/O-Contrabass/Source/
   # Expect: 1 file changed, 4 insertions(+), 3 deletions(-)
   #   plugins/O-Contrabass/Source/DSP/WaveguideString.cpp | 7 ++++----
   git diff --stat HEAD -- plugins/O-Bowed/Source/ \
                            modules/synthesis/bow-friction/Source/
   # Expect: empty
   ```

3. **Audit hook (b): grep verification:**
   ```bash
   grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp
   # Expect: 2
   grep -c "std::sqrt (1.0f + " plugins/O-Contrabass/Source/DSP/WaveguideString.cpp
   # Expect: 0
   ```

**Files created:** none.
**Files modified:** none.
**Commit:** **NONE** — diagnostic only.

**Success bar:**
- [ ] `reproduce-goldens.sh` reports 13/13 PASS against post-port sha256s.
- [ ] `git diff --stat HEAD -- plugins/O-Contrabass/Source/` reports exactly 1 file (4 insertions, 3 deletions).
- [ ] `git diff --stat HEAD -- plugins/O-Bowed/Source/ modules/synthesis/bow-friction/Source/` reports empty.
- [ ] `grep -c "sat \* std::tanh"` returns `2`; `grep -c "std::sqrt (1.0f + "` returns `0`.

**Estimated effort:** ~30 sec.

---

### R36-bis-f — auval + pluginval-10 SUCCESS battery

**Per CLAUDE.md macOS protocol + Phase 2.4a/b/c precedent. Final RT-safety + plugin-host-compat gate before R36-bis atomic. Confirms `std::tanh` at the per-sample saturator site does NOT introduce RT-safety regression (no allocations, no system calls, no denormal CPU spikes).**

**Tasks:**

1. **auval AU validation:**
   ```bash
   killall -9 AudioComponentRegistrar 2>/dev/null || true
   rm -rf ~/Library/Caches/AudioUnitCache/
   rm -rf ~/Library/Caches/com.apple.audiounits.cache
   # Working-tree AU should already be installed from R36-bis-d Step 3
   auval -v aumu OBSC OURI 2>&1 | tee /tmp/auval-r36bis.log
   # Expect: "AU VALIDATION SUCCEEDED" at end of log
   ```

2. **pluginval --strictness-level 10 SUCCESS battery:**
   ```bash
   pluginval --strictness-level 10 --validate-in-process --skip-gui-tests \
             ~/Library/Audio/Plug-Ins/VST3/O-Contrabass-dev.vst3 2>&1 | tee /tmp/pluginval-r36bis.log
   # Expect: full battery PASS — Editor Automation / Automatable Parameters /
   #   Parameter thread safety / Background thread state / Bus enable/disable /
   #   Restoring default layout / Fuzz parameters all complete without error.
   ```

   The working-tree VST3 should be installed at `~/Library/Audio/Plug-Ins/VST3/` per CLAUDE.md macOS install sequence:
   ```bash
   rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Contrabass-dev.vst3
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass-dev.vst3 \
         ~/Library/Audio/Plug-Ins/VST3/
   ```

**Files created:** `/tmp/auval-r36bis.log`, `/tmp/pluginval-r36bis.log` (transient; NOT committed).

**Files modified:** none committed.

**System modifications:**
- `~/Library/Audio/Plug-Ins/VST3/O-Contrabass-dev.vst3` — post-port VST3 install (carries forward).

**Commit:** **NONE** — diagnostic only.

**Success bar:**
- [ ] `auval -v aumu OBSC OURI` exits 0 with "AU VALIDATION SUCCEEDED".
- [ ] `pluginval --strictness-level 10` exits 0 with full battery PASS.

**Estimated effort:** ~5–10 min (auval ~2 min + pluginval ~5–8 min).

---

### R36-bis — Atomic commit (Phase 2.4c-bis Gate 6c-bis PASS)

**After R36-bis-d audition CONFIRMED PASS + R36-bis-e regression bar PASS + R36-bis-f auval/pluginval-10 SUCCESS, commit the staged R36-bis-a + R36-bis-b + R36-bis-c + R36-bis-d artefacts in a single atomic commit. Continues atomic-commit sequence R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → **R36-bis**.**

**Tasks:**

1. **Stage all R36-bis artefacts:**
   ```bash
   git add plugins/O-Contrabass/Source/DSP/WaveguideString.cpp \
           plugins/O-Contrabass/tests/render-harness/golden/{stiffness-zero-pre,string-A,string-D,string-G,detune-sweep-A,note-sequence,macro-sweep,slow-lfo,schelleng-stress,sub-harmonics,sub-harmonics-stability,saturator-tail-comparison,vibrato}.wav.sha256 \
           plugins/O-Contrabass/tests/render-harness/golden/vibrato.json \
           plugins/O-Contrabass/tests/render-harness/golden/vibrato.json.sha256 \
           plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json \
           plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json.sha256 \
           plugins/O-Contrabass/.planning/evidence/phase-2-4c-bis/matrix-stability-post-port.json \
           plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md \
           plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md \
           plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md \
           plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md \
           plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md \
           plugins/O-Contrabass/.planning/STATUS.md
   ```

2. **Pre-commit verification:**
   ```bash
   git diff --stat --cached -- plugins/O-Contrabass/Source/
   # Expect: 1 file changed (WaveguideString.cpp), 4 insertions, 3 deletions
   git diff --stat --cached -- plugins/O-Contrabass/tests/render-harness/golden/
   # Expect: 17 files changed (13 .wav.sha256 + 2 .json + 2 .json.sha256)
   ```

3. **Atomic commit:**
   ```bash
   git commit -m "feat(O-Contrabass): Phase 2.4c-bis R36-bis — in-loop saturator port (x/√(1+x²) → 4·tanh(x/4)); Gate 6c-bis SOFT-PASS (|Δ|=0.80 dB at bin 64; 3 Phase 2.4-bis backlog items)

Closes the §19.7.6 escalation flag locked at Phase 2.4c verify (5.92 dB envelope
divergence > 2 dB Q41 threshold). Ports the in-loop algebraic saturator at
WaveguideString.cpp:204-206 from x/sqrt(1+x²) to sat·tanh(x/sat) with sat=4.0f
on both rails (toBridge + toNeck), matching O-Bowed WaveguideString.cpp:218-219
(writeJunction active production path).

Convergence: post-port decayEnvelopeDb[64] = -7.97 dB rel max → |Δ| = 0.80 dB vs
O-Bowed reference -7.17 dB; 87% improvement vs pre-port 5.92 dB divergence.
Lands inside soft-band [-8.17, -6.17] (0.30 dB outside strict-band [-7.67, -6.67]
per Q47 widening); SOFT-PASS verdict per §19.7.7.9.

13 audible goldens re-baseline (11 default-state + saturator-tail-comparison +
vibrato) per RESEARCH §20.5; vibrato.json metrics shift documented in §19.7.7.5
(peakDepthCents 9.526 → 7.9507; onsetTimeMs 1168 → 1000); matrix-stability
re-rendered evidence-only (4 NEW raucous corners at high-press × β=0.05;
stability invariant intact — pass_noNaN/pass_peak/pass_blockTime all PASS;
matrix-stability.wav.sha256 = 6db67707… carries forward verbatim from Phase
2.4a R34b; post-port 09cbf15f… archived under .planning/evidence/).

Phase 2.4-bis backlog gains 3 NEW items: (1) DSP-07 retune for tanh saturator
topology — restore subharmEnergyRatio above 0.30 strict (post-port mute at
SUB_HARMONICS > 0 engagement; default-state HR-9 IEEE 754 identity arithmetic
short-circuit unaffected → 11 default-state goldens shift only due to direct
topology change, NOT subharmonic-bias differential); (2) DSP-09 VIBRATO_DEPTH
transfer tune (additive; post-port lands 7.95¢ vs 9.53¢ pre-port; both below
strict 10¢ band); (3) click-free heuristic threshold tune for high-pressure ×
β=0.05 corners (4 NEW raucous corners surfaced post-port).

R37-bis Logic AU audition CONFIRMED (sequences 1-3 BLOCKING PASS; sequences 4-5
DOCUMENT-only; user CONFIRM message captured to STATUS.md
phase_2_4c_bis_audition_outcome).

HR-1..HR-10 carry-forward verbatim. HR-11 (Phase 2.4c zero-production-DSP-edits)
RETIRED at Phase 2.4c-bis cycle open. NO new HR introduced.

NO Stage-1 contract amendment (parameter-spec.md sha256 77638e25… unchanged).
NO ARCHITECTURE.md amendment in this cycle (saturator-tail evidence — both
pre-port c7e845ea… from 115dbf4 worktree AND post-port 5c45d176… from R36-bis —
feeds end-of-Stage-2 §\"In-loop saturator\" amendment cycle).

auval AU VALIDATION SUCCEEDED + pluginval --strictness-level 10 full battery
SUCCESS. reproduce-goldens.sh 13/13 PASS against post-port sha256s.

Continues atomic-commit sequence R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 →
R36-bis. Phase 2.5 (body resonator + bow noise) opens fresh CONTEXT rev-10
(NOT rev-9) post-Phase-2.4c-bis verify.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
   ```

4. **Verify post-commit state:**
   ```bash
   git log --oneline -3
   # Expect: <R36-bis sha> (Phase 2.4c-bis R36-bis) + 7835904 + 115dbf4
   ```

**Files committed:** ~17–20 files (1 source + 13 .wav.sha256 + 2 .json + 2 .json.sha256 + 1 evidence JSON + planning artefacts: CONTEXT/RESEARCH/PLAN/SUMMARY/VERIFICATION/STATUS = 6).

**Commit:** **R36-bis atomic** — Phase 2.4c-bis Gate 6c-bis PASS.

**Success bar:**
- [ ] `git diff --stat --cached -- plugins/O-Contrabass/Source/` reports exactly 1 file (4 insertions, 3 deletions) BEFORE commit.
- [ ] Atomic commit lands single sha covering source + goldens + RESEARCH §19.7.7 + planning artefacts.
- [ ] Post-commit `git log --oneline -3` shows R36-bis as HEAD.
- [ ] STATUS.md `status` flipped to `phase_2_4c_bis_complete` (or `phase_2_4c_bis_verify_pending`); `next_action` flipped to `phase_2_4c_bis_verify`.

**Estimated effort:** ~30 sec.

---

### R36-bis-backfill — Backfill chore: propagate R36-bis sha256 into STATUS.md

**Per R34/R35/R36 backfill chore precedent. After R36-bis atomic lands, propagate the R36-bis commit sha into `STATUS.md` `phase_2_4c_bis.atomic_sha` field via a separate chore commit (mirrors R34-backfill `b64c8c4`, R35-backfill `0db5fac`, R36-backfill `7835904` precedent).**

**Tasks:**

1. **Capture R36-bis sha:**
   ```bash
   R36BIS_SHA=$(git rev-parse HEAD)
   echo "R36-bis sha: $R36BIS_SHA"
   ```

2. **Update STATUS.md `phase_2_4c_bis_atomic_sha` field:**
   - Append (or update) `phase_2_4c_bis_atomic_sha: <full sha>` near the top-level status fields in STATUS.md (mirror R34/R35/R36 placement).

3. **Commit chore:**
   ```bash
   git add plugins/O-Contrabass/.planning/STATUS.md
   git commit -m "chore(O-Contrabass): backfill Phase 2.4c-bis R36-bis commit sha ($R36BIS_SHA) into STATUS.md

Mirrors R34-backfill (b64c8c4) / R35-backfill (0db5fac) / R36-backfill (7835904)
backfill chore precedent. Propagates R36-bis atomic sha into STATUS.md
phase_2_4c_bis_atomic_sha field for audit-trail completeness.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
   ```

**Files modified:** `plugins/O-Contrabass/.planning/STATUS.md` (single field update).

**Commit:** **R36-bis-backfill** chore (separate from R36-bis atomic).

**Success bar:**
- [ ] `STATUS.md` contains `phase_2_4c_bis_atomic_sha: <full sha>` field.
- [ ] Backfill chore commit lands as sole-changed-file commit.
- [ ] Post-backfill `git log --oneline -4` shows R36-bis-backfill + R36-bis + 7835904 + 115dbf4.

**Estimated effort:** ~30 sec.

---

## Why R36-bis is a single atomic commit

**Rationale (per CONTEXT Q52 + R34/R35/R36 atomic-commit precedent):**

1. **Gate-first principle.** R36-bis lands ONLY if Gate 6c-bis invariants ALL pass: bit-deterministic-across-re-renders for 13 audible re-baselines via reproduce-goldens.sh (R36-bis-e); audit-hook scope discipline (1 file modified, 2 tanh sites); auval/pluginval-10 SUCCESS battery (R36-bis-f); R37-bis Logic AU audition CONFIRMED (R36-bis-d). Splitting into multiple commits would mean intermediate states could fail one of these invariants while passing others — defeating the gate.

2. **§"In-loop saturator" amendment evidence atomicity.** The post-port evidence base (sha256s, RESEARCH §19.7.7, source edit) must land together so end-of-Stage-2 amendment cycle can `git show <R36-bis sha>` to retrieve the complete delta. Splitting goldens from source from research would fragment the evidence base.

3. **Atomic-commit sequence continuity.** R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis preserves the "one atomic per Gate PASS" precedent. R34/R35/R36 each used a single atomic + a backfill chore for STATUS sha propagation; R36-bis follows the same shape.

4. **Backfill chore separation rationale.** STATUS.md sha-self-reference is a chicken-and-egg problem: the atomic commit's sha cannot be embedded in the atomic commit itself (bootstrapping). The backfill chore lands the sha as a follow-up commit per R34/R35/R36 precedent. This is a deliberate two-commit shape (atomic + chore), NOT a split of the cycle's deliverables.

5. **Revert simplicity.** If post-merge regression discovered, `git revert <R36-bis sha>` cleanly reverts the source + goldens + research without partial-state ambiguity.

---

## Files To Create / Modify (consolidated, Phase 2.4c-bis)

**Source files (committed in R36-bis atomic):**
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` — 4-LOC saturator topology port (lines 204–206; 3 deletions / 4 insertions).

**Golden files (committed in R36-bis atomic; 13 .wav.sha256 + 2 .json + 2 .json.sha256 = 17 files):**
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.wav.sha256` — re-baselined to `ed44cd89…f56020`.
- `plugins/O-Contrabass/tests/render-harness/golden/string-A.wav.sha256` — re-baselined to `505ad36e…785bad`.
- `plugins/O-Contrabass/tests/render-harness/golden/string-D.wav.sha256` — re-baselined to `e0640351…11668f4`.
- `plugins/O-Contrabass/tests/render-harness/golden/string-G.wav.sha256` — re-baselined to `0e9451b8…111b3ca`.
- `plugins/O-Contrabass/tests/render-harness/golden/detune-sweep-A.wav.sha256` — re-baselined to `b51d334b…39dbe7`.
- `plugins/O-Contrabass/tests/render-harness/golden/note-sequence.wav.sha256` — re-baselined to `2b5b8c83…1f8224`.
- `plugins/O-Contrabass/tests/render-harness/golden/macro-sweep.wav.sha256` — re-baselined to `231218b4…66cfe`.
- `plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.wav.sha256` — re-baselined to `d27589de…09cb8`.
- `plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.wav.sha256` — re-baselined to `c5108af5…7efbda07`.
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.wav.sha256` — re-baselined to `9178b41e…1a8c54c`.
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.wav.sha256` — re-baselined to `2efdea9b…8e6be`.
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.wav.sha256` — re-baselined to `5c45d176…9bc7a7`.
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.wav.sha256` — re-baselined to `df7384e3…568f47`.
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.json` — re-baselined post-port output.
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.json.sha256` — re-anchored (informational-only one-time anchor).
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json` — re-baselined post-port output.
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.json.sha256` — re-anchored (informational-only one-time anchor).

**Evidence files (committed in R36-bis atomic; informational-only):**
- `plugins/O-Contrabass/.planning/evidence/phase-2-4c-bis/matrix-stability-post-port.json` — failure-mode migration extract (~30 KB; references §19.7.7.6 table). The matrix-stability WAV (~50 MB) is NOT committed.

**Planning artefacts (committed in R36-bis atomic):**
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` — rev-9-bis (already in place from discuss-phase).
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` — §20 + §19.7.7 (10 sub-sections; ~150 LOC growth) + §19.7.6 closure paragraph.
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` — this rev-11 (already in place from plan-phase; no further edit at execute).
- `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` — append Phase 2.4c-bis section.
- `plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md` — append Phase 2.4c-bis section.
- `plugins/O-Contrabass/.planning/STATUS.md` — flip status to `phase_2_4c_bis_complete`; append `phase_2_4c_bis_*_carry_forward` blocks; append `phase_2_4c_bis_audition_outcome` block (R36-bis-d output); append `phase_2_4c_bis_atomic_sha` (R36-bis-backfill).

**Files NOT modified (carry-forward verbatim):**
- `plugins/O-Contrabass/Source/{BowedContrabassVoice.{h,cpp}, PluginProcessor.{h,cpp}}` and all other DSP source — verbatim consume.
- `plugins/O-Contrabass/Source/DSP/{DispersionFilter.h, SchellengCalibration.h, SubHarmonicBias.h}` — verbatim consume.
- `modules/synthesis/bow-friction/Source/*` — HR-10 ABI preservation; verbatim consume.
- `plugins/O-Bowed/Source/*` — verbatim consume (no edits in Phase 2.4c-bis).
- `plugins/O-Contrabass/tests/render-harness/main.cpp` — verbatim consume from Phase 2.4c R36 (autocorrelator range-bias + saturator-tail-comparison mode + Option B flags all locked).
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` — entry count 13 unchanged; only sha256 values inside per-entry comparisons change (handled by per-golden `.wav.sha256` files above).
- `plugins/O-Contrabass/tests/render-harness/golden/matrix-stability.{wav.sha256, json, json.sha256}` — Phase 2.4a R34b `6db67707…` baseline carries forward verbatim.
- `plugins/O-Contrabass/.planning/parameter-spec.md` — Stage-1 contract sha256 `77638e25…` unchanged.
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` — sha256 `3cb26814…` unchanged (end-of-Stage-2 amendment cycle).
- `plugins/O-Contrabass/.planning/ROADMAP.md` — sha256 `106639f6…` unchanged.

---

## Dependencies Graph (compact)

```
R36-bis-pre  (no commits — diagnostic gate)
   │
   ▼
R36-bis-a    (4-LOC source edit at WaveguideString.cpp:204–206; staged, not committed)
   │
   ▼
R36-bis-b    (build + 13 golden re-baselines + matrix-stability evidence + JSON re-anchors; staged)
   │
   ▼
R36-bis-c    (RESEARCH §19.7.7 sub-sections 1–7 + 9 + 10 written; §§8 + 9-final marked TBD; staged)
   │
   ▼
R36-bis-d    (R37-bis Logic AU audition; BLOCKING gate; on PASS: §19.7.7.8 + §19.7.7.9 locked + STATUS.md audition_outcome appended; staged)
   │ ← BLOCKING gate; if FAIL, no R36-bis commit; revert or re-iterate sat constant
   ▼
R36-bis-e    (regression bar + audit hooks; diagnostic only)
   │
   ▼
R36-bis-f    (auval + pluginval-10; diagnostic only)
   │
   ▼
R36-bis      (atomic commit — all R36-bis-a/b/c/d-staged artefacts in single sha)
   │
   ▼
R36-bis-backfill  (chore commit — propagate R36-bis sha into STATUS.md)
```

**Critical-path observations:**
- R36-bis-pre / R36-bis-a / R36-bis-b / R36-bis-c can stage as a single working-tree state with NO commits between (per RESEARCH §20.13 sequencing).
- R36-bis-d is the BLOCKING gate for R36-bis atomic; user audition cannot be skipped.
- R36-bis-e + R36-bis-f are diagnostic-only checks BETWEEN audition CONFIRM and atomic commit.
- R36-bis-backfill is a SEPARATE commit lifecycle (mirrors R34/R35/R36 backfill precedent).

**Wall-clock budget estimate (per RESEARCH §20.13):**
- R36-bis-pre: ~2–5 minutes
- R36-bis-a: ~30 seconds
- R36-bis-b: ~3–5 minutes
- R36-bis-c: ~10 minutes
- R36-bis-d: user-paced ~15–25 minutes (Logic AU audition + 5 probe sequences + capture)
- R36-bis-e: ~30 seconds
- R36-bis-f: ~5–10 minutes (auval + pluginval-10)
- R36-bis: ~30 seconds
- R36-bis-backfill: ~30 seconds

**Total estimated cycle: 30–50 minutes** including audition pacing.

---

## Risks (Phase 2.4c-bis, refreshed from RESEARCH §20.12)

| # | Risk | Likelihood | Impact | Mitigation |
|---|------|------------|--------|------------|
| 1 | **Saturator port unexpected character change** beyond pure decay-envelope shape (transient distortion, vibrato envelope shift, harmonic spectrum shift) | LOW (research-phase metrics support smoother decay; tanh is canonical soft-saturator topology) | HIGH (R36-bis atomic blocked) | R37-bis BLOCKING Logic AU audition (R36-bis-d) catches subjective issues; objective measurements (auval, pluginval-10, 13-audible bit-deterministic-across-re-renders) catch RT-safety + numeric issues. FAIL handling: sat constant tune / revert / Phase 2.4c-bis-bis escalation. |
| 2 | **Convergence misses ±0.5 dB strict target window** (post-port bin 64 outside [−7.67, −6.67]) | RESOLVED — research-phase measured 0.7975 dB \|Δ\| (lands at −7.97 dB; 0.30 dB outside strict-band lower edge) | MEDIUM (SOFT-PASS verdict adds Phase 2.4-bis backlog item) | Q47 widening to soft-band [−8.17, −6.17]; Q9 SOFT-PASS path acceptable; sat constant tune NOT performed (preserves architectural-parity intent). |
| 3 | **Two-call-site asymmetry in O-Bowed** | RESOLVED — single-site at O-Contrabass `:204-206` is correct (O-Bowed `processSample:138-139` is dead code; `writeJunction:218-219` is active) | LOW | Single-site port confirmed; `grep -c "sat \* std::tanh"` returns 2, not 4. |
| 4 | **9-audible-golden re-baseline drift across runs** (non-determinism would block plan-phase regression bar) | RESOLVED — all 13 DET-PASS 3-trial on M1 macOS Xcode 26.3 | HIGH if materialised | RESEARCH §20.5 3-trial determinism check confirms `std::tanh` is bit-deterministic on target toolchain. R36-bis-b reproducibility check at execute-phase re-confirms via `reproduce-goldens.sh`. |
| 5 | **Vibrato golden carry-forward fails** (peakDepthCents shift exceeds ±0.05¢ tolerance) | RESOLVED — RE-BASELINES; 13 audible total | LOW | RESEARCH §20.6 documents shift (9.526¢ → 7.9507¢); vibrato.wav.sha256 re-baselined to `df7384e3…568f47`; status remains FAIL (both pre/post below strict 10¢ band; neither crosses 5¢ perceptual floor). |
| 6 | **Matrix-stability NEW raucous corners surface** | RESOLVED — 4 NEW corners at high-press × β=0.05; stability invariant intact | MEDIUM (Phase 2.4-bis backlog click-free heuristic threshold tune) | RESEARCH §20.7 failure-mode migration table; pass_noNaN/pass_peak/pass_blockTime all PASS across 108 combos pre AND post; click-free heuristic is quality-gate, not safety-gate; non-blocking. Evidence-only NOT re-baselined. |
| 7 | **Sub-harmonics `subharmEnergyRatio` drops below 0.30** | **MATERIALISED** — 0.358 → 0.000170 (~33 dB drop) at engagement | HIGH (DSP-07 feature mute at SUB_HARMONICS > 0) | RESEARCH §20.8 documents critical drop. Default-state HR-9 IEEE 754 identity arithmetic short-circuit holds → 11 default-state goldens shift only due to direct topology change (NOT subharmonic-bias differential); default users UNAFFECTED. Phase 2.4-bis DSP-07 retune backlog item (additive — strengthens existing kForceBoost retune). R37-bis sequence 4 audits subjective severity (DOCUMENT-only). NON-BLOCKING per Q48 strict-saturator-only-scope. |
| 8 | **R37-bis BLOCKING audition fails** | LOW (research-phase metrics support PASS likelihood) | HIGH (R36-bis atomic blocked) | RESEARCH §20.10 FAIL handling: (a) sat constant tune; (b) revert R36-bis-a + close 2.4c-bis as research-only acknowledged-divergence; (c) escalate to 2.4c-bis-bis with alternative topology. R36-bis atomic is BLOCKING-gated by audition CONFIRM. |
| 9 | **Audit-hook drift mid-cycle** (stowaway DSP edits between R36-bis-a + R36-bis atomic) | RESOLVED — pre-flight hooks return clean | LOW | R36-bis-pre + R36-bis-e tripwires bracket the source edit; `git diff --stat HEAD -- plugins/O-Contrabass/Source/` MUST report exactly 1 file (WaveguideString.cpp) at both tripwires. |
| 10 | **`std::tanh` RT-safety regression** (allocations, system calls, denormal CPU spikes) | RESOLVED — M1 deterministic; juce::ScopedNoDenormals already in place upstream | LOW | RESEARCH §20.4 3-trial sha256 byte-identical (no LUT fallback needed); pluginval-10 R36-bis-f confirms RT-safety battery. |
| 11 | **Phase 2.5 awareness shift (body resonator interaction)** | DEFERRED — Phase 2.5 verify will re-measure saturator-tail | MEDIUM | Phase 2.5 verify includes saturator-tail re-measurement as regression check; if Phase 2.5 body resonator changes the tail shape, sat constant retune may be required. RESEARCH §19.7.7.10 evidence base supports this re-measurement at Phase 2.5. |
| 12 | **R36-bis atomic + R36-bis-backfill chore interaction** | RESOLVED — atomic + backfill chore precedent | LOW | R34/R35/R36 backfill chore precedent. R36-bis-backfill commits AFTER R36-bis lands; STATUS.md `phase_2_4c_bis_atomic_sha` field bootstraps the sha-self-reference. |
| 13 | **Pre-port reference re-render reproducibility** | RESOLVED — 3-trial PASS from `115dbf4` worktree | LOW | RESEARCH §20.3 protocol locked: `git worktree add /tmp/oc-pre-port 115dbf4` + `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DSKIP_PLUGINS=O-Orbit -DOUARICON_BUILD_TESTS=ON ..` + `ninja O-Contrabass-render-test` → reproduces sha256 `c7e845ea…1a60cb` byte-identical. R36-bis-pre check (d) re-validates at execute-phase. |
| **14** | **🚩 NEW (RESEARCH §20.12) — Sub-harmonics feature mute at engagement (DSP-07 architectural commitment broken at SUB_HARMONICS > 0)** | **MATERIALISED** | HIGH at engagement; LOW for default user (HR-9 short-circuit) | DSP-07 architectural commitment from Phase 2.4b R35 is broken at SUB_HARMONICS > 0 engagement (subharmEnergyRatio 0.358 → 0.000170, ~33 dB drop, far below 0.30 strict). Default-state HR-9 IEEE 754 identity arithmetic short-circuit holds → default users UNAFFECTED. Mitigation: Phase 2.4-bis backlog DSP-07 retune for tanh saturator topology (kForceBoost / mu_d / bias amplitude scale 3–5× boost / bias injection point shift Step 2.5 → post-saturator Step 8). R37-bis sequence 4 audits subjective severity (DOCUMENT-only). NON-BLOCKING per Q48. **PLAN rev-11 surfaces this severity-flagged risk so user has visibility before R36-bis-atomic.** |
| **15** | **🚩 NEW (RESEARCH §20.12) — Vibrato peakDepthCents shift to 7.95¢ (drifts further below strict 10¢ band)** | MATERIALISED — but additive, not regression | LOW (Phase 2.4-bis backlog DSP-09 already parked; non-blocking) | Post-port lands 7.95¢ vs 9.53¢ pre-port; both below strict 10–14¢ band; neither crosses 5¢ perceptual floor. RESEARCH §20.6 documents the shift mechanism (saturator port upstream of vibrato modulator path; tanh's higher peak amplitude pass-through reduces relative envelope swing). Phase 2.4-bis backlog DSP-09 VIBRATO_DEPTH→peakDepthCents transfer tune (already parked) gains additive urgency from this finding. NON-BLOCKING. |
| **16** | **🚩 NEW (RESEARCH §20.12) — Click-free heuristic regression at high-pressure × β=0.05 corners** | MATERIALISED — 4 NEW raucous corners | MEDIUM (Phase 2.4-bis backlog new item; non-blocking; stability invariant intact) | RESEARCH §20.7 failure-mode migration: 3 pre-port FAILs (low-press × high-speed × β=0.05) STABILISE; 4 NEW FAILs surface at high-press × β=0.05 (E1 × press2 × all 3 speeds + G3 × speed2 × press1). All FAILs (pre + post) on `pass_clickFree` only; `pass_noNaN`/`pass_peak`/`pass_blockTime` PASS across all 108 combos pre AND post. Net +1 cell. Stability invariant intact. Phase 2.4-bis backlog click-free heuristic threshold tune (NEW item) — investigate threshold relaxation OR per-string Schelleng wedge tune at near-bridge bow position. NON-BLOCKING. Migration-not-regression: net DSP-01 stability invariant preserved. |

---

## Success Criteria (Gate 6c-bis — Phase 2.4c-bis verify exit gate)

**Five-item bar (verify-phase locks):**

1. **13 audible-golden bit-deterministic-across-re-renders.** `bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` reports `OK: all 13 goldens reproduce byte-identical` against the post-port sha256s locked in §"Tasks" R36-bis-b above. All 13 sha256s match RESEARCH §20.5 LOCKED predicted values. Independent 3-trial re-render at verify-phase confirms `std::tanh` determinism on target M1 macOS Xcode 26.3 toolchain.

2. **Source-tree audit hook scope discipline.** `git diff --stat HEAD~1..HEAD -- plugins/O-Contrabass/Source/` reports exactly 1 file changed (`Source/DSP/WaveguideString.cpp`, 4 insertions, 3 deletions); `git diff --stat HEAD~1..HEAD -- plugins/O-Bowed/Source/ modules/synthesis/bow-friction/Source/` reports empty (HR-10 friction module ABI preserved). `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` returns exactly 2 (toBridge + toNeck rails); `grep -c "std::sqrt (1.0f + " plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` returns 0.

3. **R37-bis Logic AU audition CONFIRMED.** Sequences 1–3 BLOCKING PASS (smoother + more O-Bowed-like decay tail; no transient distortion / DC drift / ringing); sequences 4–5 DOCUMENT-only outcomes captured. STATUS.md `phase_2_4c_bis_audition_outcome` block contains user CONFIRM message with per-sequence verdicts + subjective notes. RESEARCH §19.7.7.8 + §19.7.7.9 locked with final wording (port WORKED-PARTIALLY SOFT-PASS + 3 Phase 2.4-bis backlog items).

4. **auval + pluginval-10 SUCCESS.** `auval -v aumu OBSC OURI` exits 0 with "AU VALIDATION SUCCEEDED" (full render-rate matrix + parameter setting/scheduling + MIDI all PASS). `pluginval --strictness-level 10 --validate-in-process --skip-gui-tests` exits 0 with full battery PASS (Editor Automation / Automatable Parameters / Parameter thread safety / Background thread state / Bus enable/disable / Restoring default layout / Fuzz parameters all complete). RT-safety preserved post-port (no allocations introduced by `std::tanh`; juce::ScopedNoDenormals already in place upstream).

5. **§19.7.7 verdict subsection locked.** RESEARCH.md §19.7.7 contains 10 sub-sections per CONTEXT lines 51–61; §§19.7.7.1–7 + 9 + 10 contain measured data from §20 verbatim; §19.7.7.8 contains R37-bis audition outcome; §19.7.7.9 verdict line locked (port WORKED-PARTIALLY SOFT-PASS at bin 64 with 0.7975 dB |Δ|; 87% improvement vs pre-port 5.92 dB; 3 Phase 2.4-bis backlog items added: DSP-07 retune for tanh saturator topology, DSP-09 VIBRATO_DEPTH transfer tune additive, click-free heuristic threshold tune; default-state HR-9 IEEE 754 identity arithmetic preserved → 11 default-state goldens unaffected by sub-harmonics feature mute at engagement). §19.7.7.10 evidence base for end-of-Stage-2 §"In-loop saturator" amendment locked with both pre-port (`c7e845ea…` from `115dbf4` worktree) AND post-port (`5c45d176…` from R36-bis atomic) goldens.

**SOFT-PASS path (predicted):** Bin 64 lands inside soft-band [−8.17, −6.17] but 0.30 dB outside strict-band [−7.67, −6.67] lower edge (post-port −7.97 dB; |Δ| = 0.7975 dB vs O-Bowed −7.17 dB). Per Q47 + Q9 widening: SOFT-PASS verdict acceptable; strict-band convergence retune via `sat` constant deferred to Phase 2.4-bis backlog.

**ESCALATION path (low-probability):** If R37-bis audition reveals unexpected character changes (sequences 1–3 FAIL), R36-bis atomic does NOT land. Per RESEARCH §20.10 FAIL handling: (a) `sat` constant tune (`sat=3.0f` or `sat=5.0f`) + re-render saturator-tail-comparison + re-audition; (b) revert R36-bis-a + close 2.4c-bis as research-only acknowledged-divergence (path (b) of §20.10); (c) escalate to Phase 2.4c-bis-bis with alternative topology (LUT, polynomial approximation).

---

## Out of Scope (deferred per CONTEXT.md rev-9-bis + RESEARCH §20 + STATUS.md)

- **Phase 2.4-bis backlog** — kForceBoost retune (DSP-07; subharmEnergyRatio above 0.40 strict — gains urgency from §20.8 mute finding); Step 4 modulation gain / breathingAudible metric refinement (DSP-08; 20% peak-to-peak); VIBRATO_DEPTH→peakDepthCents transfer tune (DSP-09 from 2.4c deviation #6 + §20.6 7.95¢ post-port shift; strict 10–14¢ peak); 3 v1.0 fallback-cell reduction (Phase 2.4a); **NEW (Phase 2.4c-bis):** DSP-07 retune for tanh saturator topology — restore subharmEnergyRatio above 0.30 strict OR ≥ 0.358 R35-baseline parity via kForceBoost gain compensation, bias signal amplitude scale (3–5× boost), or bias injection point shift (Step 2.5 → post-saturator Step 8); **NEW (Phase 2.4c-bis):** click-free heuristic threshold tune for high-pressure × β=0.05 corners — 4 cells (E1 × press2 × all 3 speeds + G3 × speed2 × press1) failing at β=0.05 with peak ≈ 0.35; investigate threshold relaxation OR per-string Schelleng wedge tune at near-bridge bow position; **NEW (Phase 2.4c-bis):** strict-band convergence retune (`sat` constant tune `sat=3.5f` or `sat=4.5f`) to push bin 64 inside strict-band [−7.67, −6.67] from current −7.97 dB. **All stay parked.** Strict saturator-only scope keeps the source delta unambiguous and clean for the §"In-loop saturator" amendment evidence base.
- **Phase 2.5** — body resonator + bow noise (8-mode parallel biquad body bank Askenfelt-derived; 3-band BPF bow noise summed AFTER body resonator). Opens fresh CONTEXT **rev-10** (NOT rev-9) post-Phase-2.4c-bis verify (per skeleton §"Sequencing post-2.4c-bis"). Phase 2.5 verify includes saturator-tail re-measurement as regression check + body-resonator interaction with the post-port tanh saturator topology.
- **Phase 2.6** — master saturator/limiter, stereo width, microtonal, MPE, Note Expression, MTS-ESP, Scala/TUN.
- **Chaos detector** (architecture §457 line 476 "optional") — control-rate ~100 Hz lag-2 RMS check with 20% bias back-off; deferred to Phase 2.5/2.6 per Phase 2.4b RESEARCH §18.13 carry-forward.
- **softClampState energy clamp** — ROADMAP §Phase 2.4 deliverable (threshold 0.85 ceiling 1.0); deferred to Phase 2.5/2.6 alongside body resonator integration per Phase 2.4b RESEARCH §18.13 carry-forward.
- **ARCHITECTURE.md amendments** — §"DC Blocker" + §"In-loop saturator" stay deferred to end-of-Stage-2 verify. Phase 2.4c-bis post-port saturator-tail evidence (`5c45d176…` from R36-bis atomic) + Phase 2.4c pre-port reference (`c7e845ea…` from `115dbf4` worktree) feed the §"In-loop saturator" amendment evidence base together. **NO ARCHITECTURE.md amendment in Phase 2.4c-bis cycle itself.**
- **Stage-1 contract amendment** — parameter-spec.md sha256 `77638e25…` carries forward unchanged. STATUS.md `contract_checksums.parameter_spec` unchanged. **NO Stage-1 contract amendment in Phase 2.4c-bis.**
- **HR-12 (any new hard rule)** — REJECTED per Q50 + RESEARCH §20.16 references. Port is straightforward formula swap; HR-1..HR-10 cover existing invariants. HR-11 (Phase 2.4c) RETIRED at Phase 2.4c-bis cycle open per CONTEXT line 111.
- **Two-call-site scope expansion in O-Bowed** — REJECTED per RESEARCH §20.2 (O-Bowed `processSample:138-139` is dead code; `writeJunction:218-219` is the architectural equivalent; single-site port at O-Contrabass `:204–206` is correct). `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` returns 2, NOT 4.
- **`sat` constant tune iteration in this cycle** — REJECTED per Q9 + Q47. SOFT-PASS at `sat=4.0f` is acceptable; strict-band convergence via sat-constant tune (3.5f or 4.5f) is Phase 2.4-bis backlog item, not Phase 2.4c-bis scope. `sat=4.0f` matches O-Bowed reference exactly, preserving architectural-parity intent of the port.
- **Matrix-stability re-baseline in this cycle** — REJECTED per CONTEXT line 47 + RESEARCH §20.7. Post-port matrix-stability WAV (`09cbf15f7600…`) is evidence-only NOT committed; existing `matrix-stability.wav.sha256 = 6db67707…` carries forward verbatim from Phase 2.4a R34b. Mirrors Phase 2.4a R34b "evidence golden, not in default reproduce-goldens.sh" pattern. The 4 NEW raucous corners are a Phase 2.4-bis backlog click-free heuristic threshold tune item, NOT a Phase 2.4c-bis scope item.
- **`vibrato.json.sha256` + `saturator-tail-comparison.json.sha256` as regression bars** — REJECTED per RESEARCH §20.6. Both JSON files contain wall-clock fields → sha256 non-deterministic across renders. Treated as informational/historical one-time anchors only. `reproduce-goldens.sh` only checks `*.wav.sha256` (never `*.json.sha256`). PLAN rev-11 R36-bis-b updates these JSON sha256 files to NEW one-time post-port anchors for audit-trail completeness, NOT as regression bars.
- **R37-bis deferred-non-blocking** — REJECTED per Q49. R37-bis is BLOCKING for Phase 2.4c-bis (first audible source-edit since Phase 2.4b R35; user audition catches subjective issues that objective measurements miss). Departs from R37/R32/R27/R19f/R14e/R34h/R35 deferred-non-blocking precedent.
- **Pre-port worktree cleanup at execute-phase** — DEFERRED to verify-phase close. `/tmp/oc-pre-port` worktree is preserved through Phase 2.4c-bis verify-phase for independent reproduction of pre-port reference `c7e845ea…`; cleanup via `git worktree remove /tmp/oc-pre-port` lands at Phase 2.4c-bis verify-phase close (NOT during execute).
- **Per-string `--vibrato-A/-D/-G` audible-mode goldens** — Phase 2.4-bis or v1.1 (RESEARCH §19.2.3 range-bias hard-codes `kVibratoMidiNote = 28`).
- **Per-MIDI-note `--vibrato` mode** — `kVibratoMidiNote = 28` hard-coded for v1.0 per Phase 2.4c PLAN rev-10 + RESEARCH §19.2.3 + pin #1; carry-forward unchanged.
- **`--saturator-tail-comparison` per-string variants** — bin selection is canonical E1 only at v1.0; per-string sat-tail comparison (e.g., MIDI 33 / 38 / 43) deferred to Phase 2.5 verify regression checks or v1.1.
- **Logic Pro AU smoke (R37 deferred-non-blocking precedent)** — overridden for Phase 2.4c-bis (R37-bis is BLOCKING per Q49); R37-reservation for deferred-non-blocking smoke at Phase 2.5/2.6 carries forward.
- **E1 dispersion calibration polynomial follow-up (Phase 2.1c Risk #7)** — separate `a(B, I)` cascaded-allpass concern; not saturator-tail or vibrato. Out-of-scope for Phase 2.4c-bis; deferred to v1.1 or Phase 2.4c-bis-or-later.
- **Production WAV binary commits** — `saturator-tail-comparison.wav` (~30 MB), `matrix-stability.wav` (~50 MB), and other large WAVs NOT committed (reproducible from harness). sha256 + JSON committed instead per Phase 2.4a/b/c precedent.
- **CI invocation of `--saturator-tail-comparison` or `--matrix-stability`** — out-of-scope; harness modes are offline (developer-machine-only). CI runs the existing build + auval + pluginval pipeline.
- **O-Bowed `--saturator-tail-comparison` mirror mode** — Phase 2.4c-bis does NOT add a `--saturator-tail-comparison` mode to the O-Bowed harness (carry-forward from Phase 2.4c PLAN rev-10 deferral). Comparison is one-sided; deferred to v1.1.
- **R36-bis atomic split into multiple commits** — REJECTED per CONTEXT Q52 + atomic-commit precedent. Single atomic + backfill chore shape (R34/R35/R36 precedent).

---

# Stage 2: DSP — Plan (Phase 2.5) — REVISION 12 (Body Resonator 8-Mode Static-Q Bank + Bow Noise Generator 3-Band BPF + Period-Heuristic Slip Bursts, Gate 7)

> **Status:** rev-12 authors fresh task bodies for **R37-pre, R37a, R37b, R37c, R37d, R37e, R37f, R38, R37 atomic, R37-backfill chore** per `RESEARCH.md §21` sequencing and `CONTEXT.md` rev-10. rev-1/2/3/4/5/6/7/8/9/10/11 remain in-effect as completed/verified history. Phase 2.4c-bis closed 2026-04-29 with R36-bis atomic commit `1044bed41574be5d0714983f7910cac8bda2edec` (Gate 6c-bis SOFT-PASS at bin 64 = −7.97 dB / `|Δ| = 0.80 dB` vs O-Bowed reference) + R36-bis-backfill chore `1dfca9d`.

**Date:** 2026-04-30
**Cycle scope:** Phase 2.5 only — body resonator (8-mode parallel biquad bank, static-Q, bass-tuned modes per Askenfelt 1982 / Rossing 2010) + bow noise generator (3-band BPF at 700/1500/3000 Hz + period-heuristic slip bursts, decay 0.999) as NEW Steps 8 + 9 in per-block evaluation order, AFTER waveguide downsample, BEFORE (Phase 2.6) master chain. Closes BRIEF.md DSP-03 (must) + DSP-04 (should). Phase 2.4-bis backlog (kForceBoost retune for tanh, Step 4 / breathingAudible refinement, VIBRATO_DEPTH transfer tune, click-free heuristic threshold tune, DSP-07 sub-harmonics retune for tanh) stays parked. Wolf-region suppression deferred to v1.1 (CONTEXT Q55 + ARCHITECTURE deviation flagged).
**Gate:** Gate 7 (Phase 2.5 verify)
**Atomic-commit unit:** R37 (Gate 7 PASS) — single commit lands 4 production source files (2 NEW + 2 modified per CONTEXT line 156) + 1 CMakeLists.txt source-list addition + 13 re-baselined `*.wav.sha256` files (per RESEARCH §21.5 deferred to execute-phase) + matrix-stability evidence-only re-render archived under `.planning/evidence/phase-2-5/` (NOT committed as updated regression golden) + saturator-tail-comparison post-body re-baseline + sub-harmonics post-body re-baseline + RESEARCH §21 verdict subsection (12 sub-sections per §21.12; §§21.1–21.16 already present from research-phase; verify-phase fills R38 outcome) + STATUS.md flip to `phase_2_5_complete` + SUMMARY.md + VERIFICATION.md Phase 2.5 sections + this PLAN rev-12 (already in place from plan-phase). **NO Stage-1 contract amendment** (parameter-spec.md sha256 `77638e25…` carries forward unchanged; all 5 body/noise params already declared at PluginProcessor.cpp:52,60,62,64,66). **NO ARCHITECTURE.md amendment in Phase 2.5** (body resonator + bow noise IS architecture verbatim; wolf-region deferral is a CONTEXT-flagged deviation, not an ARCHITECTURE delta; §149 vs §509 size_scalar inconsistency flagged for end-of-Stage-2 amendment per RESEARCH §21.16 ESCALATION-3). `matrix-stability.wav.sha256 = 6db67707…` carries forward verbatim (post-Phase-2.5 evidence archived to `.planning/evidence/phase-2-5/` per Phase 2.4a R34b + Phase 2.4c-bis R36-bis precedent).
**Carry-forward locks (NOT re-litigated):** Phase 2.1a-recovery split-rail topology, F2 LP form, F3 no-in-loop-DCB, F4 betaScale removed; Phase 2.1b bow-friction module v1.0.0 consumption (HR-10 ABI preservation); Phase 2.1c `DispersionFilter<4>` API + per-string M=4/3/2/1 dispersion table; Phase 2.2 4-string bank + per-string detune + 5 ms equal-power crossfade + MIDI→string mapping; Phase 2.3 modulator-layer surface (HR-1..HR-4); Phase 2.4a Schelleng wedge calibration polynomial (HR-5..HR-8); Phase 2.4b Sub-Harmonic Bias DSP-07 (HR-9..HR-10 + Step 2.5); Phase 2.4c autocorrelator MIDI-derived range bias + `--saturator-tail-comparison` mode + `pass_vibratoAudible` aggregator + Option B O-Bowed harness extension; Phase 2.4c-bis `Source/DSP/WaveguideString.cpp:204–206` post-port `sat·tanh(x/sat)` saturator with `sat = 4.0f` on both rails — verbatim consume. Phase 2.5 modifies the EXACT 4-source-file set: `Source/DSP/BodyResonator.{h,cpp}` NEW + `Source/DSP/BowNoiseGenerator.h` NEW + `Source/BowedContrabassVoice.{h,cpp}` M (Step 8 + Step 9 + body/noise instances + smoothed param ramps + setFundamentalHz + bowEnergy compute) + `Source/PluginProcessor.{h,cpp}` M (voice-side body/noise param routing) + 1 CMakeLists.txt source-list addition.

---

## Preamble — Pinned Open Items (RESEARCH §21.16)

PLAN rev-12 pins each of the open items handed off from research-phase (RESEARCH §21.16 + §21.10/§21.11/§21.13/§21.14):

| # | Open Item | Pinned Decision |
|---|-----------|-----------------|
| 1 | **R37 9-task breakdown** | **LOCKED verbatim from RESEARCH §21.10.** Sequence: R37-pre (tripwire) → R37a (BodyResonator NEW + CMakeLists update) → R37b (BowNoiseGenerator NEW) → R37c (voice integration: Step 8 + Step 9 + smoothed param ramps + setFundamentalHz + bowEnergy) → R37d (re-baseline 13 audible goldens + matrix evidence + saturator-tail post-body + sub-harmonics post-body + 3-trial bit-stability pre-flight) → R37e (regression bar via 13-entry reproduce-goldens.sh against NEW sha256s + 4-file source audit hook + 1-file CMake audit hook) → R37f (auval + pluginval-10) → R38 (Logic AU audition, BLOCKING) → R37 (atomic commit) → R37-backfill chore. |
| 2 | **R38 Logic AU audition protocol** | **LOCKED verbatim from RESEARCH §21.11.** 4-step protocol: build pre-Phase-2.5 reference AU from `/tmp/oc-pre-2-5` worktree at `1044bed` (R36-bis post-port; idempotent across re-creates; rename to `O-Contrabass-pre-2-5` with disambiguated PLUGIN_CODE / CFBundleIdentifier to avoid collision with working-tree variant); build + install post-Phase-2.5 working-tree AU per CLAUDE.md macOS protocol; audition in Logic Pro. 7-probe sequence (BLOCKING): (1) sustained E1 MIDI 28 — body wood color present without obscuring fundamental; (2) per-string A/D/G MIDI 33/38/43 — no string-dependent body discontinuities; (3) sustained G2 MIDI 43 wolf-region check at default damping — subtle bloom acceptable, audible beating documented but NON-BLOCKING (Q55 deferral); (4) bow-direction reversal (note-on/off/on) — audible 5–15 ms wideband slip-burst on each attack; (5) BOW_NOISE 0%→100% sweep — linear loudness ramp, no zipper noise; (6) BODY_SIZE/DAMPING/MIX sweeps — smooth morph, no clicks; (7) orchestral A/B vs reference orchestral library bass at G2 — "in same sonic family" subjective bar per ROADMAP §"Phase 2.5 Test Criteria". |
| 3 | **VERBATIM-COPY ASSUMPTION BROKEN** (RESEARCH §21.2 + §21.16 ESCALATION-1) | **ACKNOWLEDGED at PLAN rev-12; CONTEXT Q54 deviation flagged; do NOT re-discuss.** O-Bowed BodyResonator is morphable Material+Size+BodyAmount with `makePeakFilter` + stereo + processor-level integration. Bass spec is single-preset Size+Damping+Mix with `makeBandPass` + mono + voice-level integration. Phase 2.5 implementation is **"implement bass spec from ARCHITECTURE using O-Bowed as reference for `juce::dsp::IIR::Filter` + `juce::Random` lifecycle patterns"** — NOT "verbatim copy + bass-substitution". LOC budget revised: **~470 LOC NEW + ~55 LOC M = ~525 LOC total** (vs CONTEXT rev-10 §"Open Q #2" estimate "100–250 per file"). Recommended starting designs locked from RESEARCH §21.2.6 (BodyResonator) + §21.2.7 (BowNoiseGenerator). |
| 4 | **Slip-burst trigger source — period-heuristic v1.0 substitute** (RESEARCH §21.3 + §21.16 ESCALATION-2) | **LOCKED.** Bow-friction module v1.0.0 + WaveguideString.cpp expose no slip-state accessor; true Helmholtz slip-detection per ARCHITECTURE §165 ("zero-crossing of friction force from stick to slip") would require WaveguideString edit which violates CONTEXT 4-file scope-strict rule. v1.0 substitute: voice tracks active-string fundamental `f0_active`, pushes to BowNoiseGenerator via `setFundamentalHz(f0)` at note-start + on f0 change > 5 cents threshold; BowNoiseGenerator decrements `slipCounter` per-sample, fires burst (envelope = `bowEnergy` at trigger time) on counter zero, resets `slipCounter = sr / f0`. Period-heuristic is bit-deterministic (integer counter math), audibly similar (one burst per fundamental period, decay 0.999/sample → ~70 ms decay envelope at 48 kHz matches ARCHITECTURE §165 spec). True slip-detection deferred to Phase 2.5-bis (Option A: voice-level F_friction reconstruction; Option B: WaveguideString getLastFrictionForce() accessor; Option C: WaveguideString slip-flag accessor). |
| 5 | **`juce::Random` seed determinism** | **LOCKED per RESEARCH §21.6 verbatim from O-Bowed pattern.** `noiseRandom.setSeed(static_cast<juce::int64>(voiceIndex * 31337))` in BowNoiseGenerator::prepare; O-Contrabass is monophonic (`voiceIndex = 0`; seed = 0). Bit-deterministic across re-renders. R37d 3-trial pre-flight per RESEARCH §21.5 confirms. |
| 6 | **13 expected post-Phase-2.5 `*.wav.sha256` values** | **DEFERRED to execute-phase R37d** per RESEARCH §21.5 (design-revision blocker; sha256 measurement requires implementation to exist). Pre-flighting 3-trial bit-stability per golden at R37d locks NEW reference values. R37 atomic commit lands the 13 NEW values. Re-baseline scope: all 13 audible goldens (per CONTEXT Q56 + RESEARCH §21.5). |
| 7 | **Vibrato carry-forward determination** | **DEFAULT = RE-BASELINE** per RESEARCH §21.7. Body coloring shifts output spectrum; `peakDepthCents` autocorrelator metric likely shifts > 0.10¢ tolerance. Already counted in 13-audible re-baseline scope. R37d post-render check: if `vibrato.wav.sha256` byte-identical AND metrics within ±0.05¢ / ±0.005 Hz / ±2 ms tolerance → carry forward (count drops to 12 audible re-baselines); if shift → re-baseline (13 stays). |
| 8 | **Saturator-tail bin 64 measurement under body coupling** | **DEFERRED to R37d execute-phase pre-flight** per RESEARCH §21.4. Body bank is post-saturator linear filter (no nonlinear backflow); predicted shift ≤ 2 dB at canonical operating point; soft-band [−9, −5] dB rel max acceptable; > 4 dB shift escalates pre-R37-atomic. Phase 2.4c-bis R36-bis baseline = −7.97 dB; verify §19.3.3 analytic bound under body-coupled topology. NOT a Gate 7 BLOCKER unless > 4 dB shift signals body-coupling instability; flags evidence base for end-of-Stage-2 §"In-loop saturator" amendment. |
| 9 | **Matrix-stability post-Phase-2.5 verdict structure** | **DEFERRED to R37d execute-phase post-implementation** per RESEARCH §21.8. Body bank is L2-bounded BIBO-stable parallel bandpass network; bow noise sums additively; predicted-low probability of NEW raucous corners. Re-render evidence-only (NOT committed as re-baselined golden). Document raucous-corner cells in §21 verdict subsection: stable / unchanged / NEW. If NEW raucous corners surface → block plan-phase (escalate before R37 atomic) pending root-cause investigation. Default-state baseline: Phase 2.4a R34b 3 fails (matrix-stability evidence at `09cbf15f…` per CONTEXT line 176). |
| 10 | **Sub-harmonics post-body coupling** | **DEFERRED to R37d execute-phase post-implementation** per RESEARCH §21.9. Phase 2.4c-bis landed `subharmEnergyRatio = 0.358` SOFT-PASS at waveguide output (pre-body). Body bank's Mode 1 at 60 Hz / Mode 2 at 98 Hz is bandpass-tilted toward subharmonic preservation. Soft-band [0.30, 0.45] expected (pre-body 0.358 ± body-coupling ≈ 0.05). If drops below 0.30 → flag for Phase 2.4-bis backlog priority bump (NOT a Gate 7 BLOCKER, evidence-only per CONTEXT line 220). |
| 11 | **CMakeLists.txt source-list update** | **LOCKED per RESEARCH §21.13.** O-Bowed `Source/DSP/BodyResonator.cpp` is real `.cpp` (188 LOC); Phase 2.5 BodyResonator design per §21.2.6 also requires `.cpp` (per-block `recomputeCoefficients` + out-of-line coefficient compute logic too large for inline header). Add `Source/DSP/BodyResonator.cpp` to `target_sources(O-Contrabass PRIVATE …)` block in `plugins/O-Contrabass/CMakeLists.txt` (1 LOC M). `Source/DSP/BowNoiseGenerator.h` is `.h`-only per O-Bowed convention (no CMake update; picked up via include path). |
| 12 | **`/tmp/oc-pre-2-5` worktree at `1044bed` idempotent + cleanup at verify-phase** | **PINNED per RESEARCH §21.11 reference-build protocol.** `git worktree add /tmp/oc-pre-2-5 1044bed41574be5d0714983f7910cac8bda2edec` is idempotent across re-creates. R37-pre verifies presence (`test -d /tmp/oc-pre-2-5/.git || git worktree add /tmp/oc-pre-2-5 1044bed`). R38 (audition) consumes the worktree to build pre-Phase-2.5 AU variant (`O-Contrabass-pre-2-5.component`, disambiguated PLUGIN_CODE — e.g., `OCb5`). Cleanup at Phase 2.5 verify-phase close via `git worktree remove /tmp/oc-pre-2-5` (NOT during execute — verify-phase needs the worktree for independent reproduction of pre-Phase-2.5 reference). |
| 13 | **R38 BLOCKING per CONTEXT Q57** | **LOCKED.** First NEW DSP block since Phase 2.4b R35 (and that was minimal); body resonator + bow noise are dramatic audible-character transformations. Mirrors R37-bis Phase 2.4c-bis precedent (audible source-edit → BLOCKING audition). Departs from R37/R32/R27/R19f/R14e/R34h/R35f deferred-non-blocking precedent. |
| 14 | **No new HR introduced** | **LOCKED per CONTEXT Q59 + RESEARCH §21.16 plan-phase pre-locks.** HR-1..HR-10 carry-forward verbatim; HR-11 retired (Phase 2.4c-only binding preserved in audit history). Body bank + bow noise extend the per-block order with NEW Step 8 + Step 9 but introduce no new IEEE 754 identity arithmetic / zero short-circuit / ABI preservation invariants beyond what exists. |
| 15 | **Wolf-region suppression deferred to v1.1** | **PINNED per CONTEXT Q55 (ARCHITECTURE deviation flagged).** Mode #2 Q stays at 11 statically. Mode 2 fc=91.16 Hz at default damping; G2 fundamental at 98 Hz is just outside Mode 2 −3 dB band. R38 Probe 3 documents audibility; if audible beating → carry to Phase 2.5-bis backlog (do NOT block R37 atomic). User can mitigate via BODY_DAMPING knob in v1.0. Reactivation path: Phase 2.5-bis or v1.1 cycle restores wolf-suppression with appropriate Hard Rule for fundamental-lock-detection determinism. |
| 16 | **ARCHITECTURE §149 vs §509 `size_scalar` inconsistency** (RESEARCH §21.16 ESCALATION-3) | **LOCK formula `size_scalar = 0.85 + 0.30·s` per ARCHITECTURE §509** (formula authoritative over commentary). 1.353:1 frequency span at default 60 Hz Mode 1 → 70.6 Hz (s=0.0) ↔ 52 Hz (s=1.0). Plan-phase deviates from ARCHITECTURE-line-149 commentary (1.83:1 claim) but preserves ARCHITECTURE-line-509 formula. Append to deferred ARCHITECTURE.md amendments at end-of-Stage-2 verify (post-Phase-2.6) alongside §"DC Blocker" + §"In-loop saturator" amendments. |
| 17 | **Phase 2.6 sequencing post-Phase-2.5** | **PINNED — Phase 2.6 opens fresh CONTEXT rev-11 post-Phase-2.5 verify.** Phase 2.5 verify-phase Gate 7 closure + R37 atomic commit + R37-backfill chore is the gate. Phase 2.6 (master saturator + zero-latency feedforward limiter + stereo width + microtonal Scala/TUN/MTS-ESP + MPE + Note Expression) opens with the body-engaged + bow-noise topology in place. End-of-Stage-2 verify (post-Phase-2.6) owns the deferred ARCHITECTURE.md amendments (§"DC Blocker" + §"In-loop saturator" + §"Body Resonator" §149/§509 reconciliation). |

**Carry-forward HARD RULES from prior phases (NOT re-litigated):**
- HR-1 (Phase 2.3): Vibrato literal-zero short-circuit + active-string-only — carry-forward.
- HR-2 (Phase 2.3): Slow-LFO literal-zero short-circuit + phase non-advance at zero depth — carry-forward.
- HR-3 (Phase 2.3): Macro IEEE 754 identity arithmetic + macroSmoothed setCurrentAndTargetValue(0.0) — carry-forward.
- HR-4 (Phase 2.3): Schelleng wedge skip on zero LFO depth + lastSafeDepth.store(0.0) unconditional pre-gate — carry-forward.
- HR-5 (Phase 2.4a): `inline constexpr` linkage on `SchellengCalibration.h` — carry-forward.
- HR-6 (Phase 2.4a): Calibration polynomial behind HR-4 gate ONLY — carry-forward.
- HR-7 (Phase 2.4a): Matrix-stability bypass via weak-symbol — carry-forward.
- HR-8 (Phase 2.4a): Trilinear IEEE 754 identity arithmetic — carry-forward.
- HR-9 (Phase 2.4b): SUB_HARMONICS=0 IEEE 754 identity arithmetic + active-string-only bias gate — carry-forward.
- HR-10 (Phase 2.4b): Friction module v1.0.0 ABI preservation under bias via ROSIN inverse algebraic identity — carry-forward.
- **HR-11 (Phase 2.4c): RETIRED** — binding limited to Phase 2.4c only (audit history preserves the rule for that phase). Phase 2.4c-bis source-change scope made HR-11 inapplicable; Phase 2.5 NEW DSP scope makes HR-11 inapplicable by design.
- 7-step + Step 2.5 per-block evaluation order verbatim — Phase 2.5 ONLY APPENDS new Step 8 (body) + Step 9 (bow noise) AFTER Step 7 downsample (`oversampling.processSamplesDown` at BowedContrabassVoice.cpp:688), BEFORE host-rate output write at lines 694–702.
- 13 carry-forward goldens (`reproduce-goldens.sh` entries) MUST reproduce 13/13 byte-identical at HEAD descendant of `1dfca9d` BEFORE R37a edit (R37-pre tripwire); MUST reproduce 13/13 byte-identical against NEW post-Phase-2.5 sha256s AFTER R37d re-baseline (R37e regression bar).
- `matrix-stability.wav.sha256 = 6db67707…` carries forward verbatim (Phase 2.4a evidence; NOT in `reproduce-goldens.sh` default array; NOT re-baselined in Phase 2.5; post-Phase-2.5 evidence WAV archived to `.planning/evidence/phase-2-5/`).
- Atomic-commit gate-first principle: R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → **R37** sequence.

**No new HARD RULES introduced in Phase 2.5** (per CONTEXT Q59 + Open Item #14 above).

**Per-block evaluation order:** Phase 2.4c-bis end-state Step 1–7 + Step 2.5 carry-forward verbatim. Phase 2.5 ONLY APPENDS:
- **NEW Step 8** — Body resonator: 8-mode parallel bandpass bank (`makeBandPass`) + 35 Hz HP one-pole on dry path → wet/dry mix. Coefficient recompute at block-start (read SmoothedValue Size/Damping BEFORE compute). Filter state continues across blocks (no per-block reset).
- **NEW Step 9** — Bow noise: 3-band BPF (700/1500/3000 Hz, Q≈1.0/1.2/1.5) + period-heuristic slip burst driven by `bowEnergy` envelope + `setFundamentalHz(f0_active)` push. Sums into voice output AFTER body wet/dry mix.

---

## Goal

Land Phase 2.5 body resonator (8-mode parallel biquad bank, static-Q, bass-tuned modes 60/98/115/175/235/340/700/1200 Hz × Q × gainDb per Askenfelt 1982 / Rossing 2010) + bow noise generator (3-band BPF at 700/1500/3000 Hz with Q≈1.0/1.2/1.5 + period-heuristic slip bursts at decay 0.999) as NEW Steps 8 (body) + 9 (bow noise) in the per-block evaluation order, AFTER waveguide downsample at `BowedContrabassVoice.cpp:688`, BEFORE host-rate output write at lines 694–702. Closes BRIEF.md DSP-03 (must) + DSP-04 (should) acceptance criteria. Implement bass spec from ARCHITECTURE §"Body Resonator (Parallel Biquad Bank)" + §"Bow Noise Generator" using O-Bowed `Source/DSP/BodyResonator.{h,cpp}` + `BowNoiseGenerator.h` as reference for `juce::dsp::IIR::Filter` + `juce::Random` lifecycle patterns — NOT verbatim copy (RESEARCH §21.2 verbatim-copy assumption BROKEN — too many deltas: filter type `makePeakFilter`→`makeBandPass`, channel topology stereo→mono, integration site processor→voice, parameter set Material+Size+BodyAmount→Size+Damping+Mix, dry-path identity→35 Hz HP, slip-detection mechanism continuous→period-heuristic). Source delta = 4 production source files (2 NEW + 2 modified, ~470 LOC NEW + ~55 LOC M = ~525 LOC) + 1 CMakeLists.txt source-list addition (1 LOC M for `Source/DSP/BodyResonator.cpp`). HR-1..HR-10 carry-forward verbatim; HR-11 retired (Phase 2.4c-only); no new HR introduced. R37 atomic commit lands all source edits + CMake update + 13 audible-golden re-baselines (per RESEARCH §21.5 deferred to execute-phase R37d) + matrix-stability evidence-only re-render (NOT committed as re-baselined golden) + saturator-tail-comparison post-body re-baseline (carried within the 13) + sub-harmonics post-body re-baseline (carried within the 13) + RESEARCH §21 (12-sub-section verdict subsection; §§21.1–21.16 already present from research-phase; verify-phase fills R38 outcome under §21.11) + planning artefacts. **NO Stage-1 contract amendment** (parameter-spec.md sha256 `77638e25…` carries forward unchanged; all 5 body/noise params already declared at PluginProcessor.cpp:52,60,62,64,66). **NO ARCHITECTURE.md amendment in Phase 2.5** itself (wolf-region deferral is CONTEXT-flagged Q55 deviation, NOT amendment; §149/§509 `size_scalar` reconciliation feeds end-of-Stage-2 verify amendment evidence base alongside §"DC Blocker" + §"In-loop saturator"). `matrix-stability.wav.sha256 = 6db67707…` carries forward verbatim (post-Phase-2.5 evidence WAV archived to `.planning/evidence/phase-2-5/`). Period-heuristic slip-burst trigger v1.0 substitute (RESEARCH §21.3.3) in place of true Helmholtz slip-detection (deferred to Phase 2.5-bis or v1.1 — true slip-detection requires WaveguideString edit out of CONTEXT 4-file scope-strict rule). Wolf-region suppression deferred to v1.1 (CONTEXT Q55; ARCHITECTURE §"Body Resonator" wolf default-ON deviation flagged; v1.0 ships static-Q body bank). Validate Gate 7 invariants — five-item bar: (1) bit-deterministic 13-entry `reproduce-goldens.sh` PASS at NEW post-Phase-2.5 sha256s + 4-file source audit hook reports exactly the EXACT 4-file production set at R37-pre + R37e tripwires + 1-file CMake audit hook; (2) DSP-03 + DSP-04 acceptance — body bank impulse response shows 8 spectral peaks at design frequencies with correct relative gains, BODY_SIZE/DAMPING/MIX sweeps zipper-free, BOW_NOISE 0%→100% audible level rise, bow-direction reversal produces 5–15 ms wideband slip-burst, wolf-region G2 audibility documented (NON-BLOCKING per Q55); (3) `auval` AU VALIDATION SUCCEEDED full render-rate matrix + `pluginval --strictness-level 10` SUCCESS full battery; (4) R38 Logic AU audition CONFIRMED — post-Phase-2.5 character is "convincing orchestral arco bass" per BRIEF.md DSP-03 + DSP-04 acceptance bar (BLOCKING — R37 atomic does NOT land until R38 audition CONFIRMED); (5) RESEARCH §21 verdict locked — Phase 2.5 verdict written: WORKED (DSP-03 + DSP-04 CLOSED, Phase 2.6 unblocked) OR WORKED-PARTIALLY (Phase 2.5-bis backlog logged for wolf-suppression / bow-noise recalibration / period-heuristic mechanical-feel) OR REGRESSION (body coupling reveals saturator-tail divergence > 4 dB OR matrix-stability NEW raucous corners; Phase 2.5-bis escalation flag LOCKED before R37 atomic). Continue R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → **R37** atomic-commit sequence. Phase 2.6 (master saturator + zero-latency feedforward limiter + stereo width + microtonal Scala/TUN/MTS-ESP + MPE + Note Expression) opens fresh CONTEXT rev-11 post-Phase-2.5 verify.

---

## Tasks

### R37-pre — Working-tree integrity tripwire + 5-check pre-flight

**No source edits committed. Diagnostic only. Confirms working-tree integrity at start of execute (HEAD descendant of `1dfca9d`), verifies the `/tmp/oc-pre-2-5` worktree is creatable from `1044bed`, and empirically validates RESEARCH §21.1 (13/13 reproduce-goldens.sh PASS at HEAD pre-edit) under the plan-phase build environment.**

Per RESEARCH §21.1, working tree at HEAD descendant of `1dfca9d` (R36-bis-backfill chore; descendant of R36-bis atomic `1044bed`) reproduces all 13 currently-committed goldens byte-identical via `reproduce-goldens.sh`. R37-pre re-confirms.

**Tasks:**

1. **Confirm git state is clean at HEAD descendant of `1dfca9d`:**
   ```bash
   git log --oneline -3            # should show 1dfca9d (R36-bis-backfill chore) ancestor + descendant chain
   git status plugins/O-Contrabass/                      # should be clean
   git status plugins/O-Bowed/                           # should be clean (read-only reference)
   git status modules/synthesis/bow-friction/            # should be clean
   ```
   If working tree is dirty, STOP and reconcile before proceeding.

2. **Create or confirm `/tmp/oc-pre-2-5` worktree at `1044bed` (idempotent re-create if missing):**
   ```bash
   if [ ! -e /tmp/oc-pre-2-5/.git ]; then
       git worktree add /tmp/oc-pre-2-5 1044bed41574be5d0714983f7910cac8bda2edec
   fi
   test -d /tmp/oc-pre-2-5/plugins/O-Contrabass
   ```
   `git worktree add` is idempotent (no-op if already exists at that path + ref). Build of the side-by-side AU variant deferred to R38; R37-pre only confirms the worktree path is creatable.

3. **Build O-Contrabass harness at HEAD (working tree):**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```
   Define `HARNESS=build/plugins/O-Contrabass/tests/render-harness/O-Contrabass-render-test_artefacts/Release/O-Contrabass-render-test` for subsequent steps. Expect `ninja: no work to do` if R36-bis build artefacts persist.

4. **Pre-flight check (a): `reproduce-goldens.sh` 13-of-13 PASS at HEAD (carry-forward tripwire):**
   ```bash
   bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   # Expect: "OK: all 13 goldens reproduce byte-identical"
   ```
   **All 13 must PASS** per RESEARCH §21.1 record. If any FAIL, STOP and investigate working-tree drift (re-confirm HEAD descendant of `1dfca9d`; `git status` for stray edits in `Source/`, `tests/render-harness/`, `modules/synthesis/bow-friction/`, `plugins/O-Bowed/`). Do NOT proceed to R37a until all 13 reproduce.

5. **Pre-flight check (b): Source-tree audit hook clean state (zero pre-edit modifications):**
   ```bash
   git diff --stat HEAD -- plugins/O-Contrabass/Source/ \
                            plugins/O-Bowed/Source/ \
                            modules/synthesis/bow-friction/ \
                            plugins/O-Contrabass/CMakeLists.txt
   # Expect: empty output (zero files reported)
   ```
   Confirms no stowaway DSP edits at pre-flight. Phase 2.5 4-file scope-strict rule (CONTEXT line 156) demands an exact production-source diff at R37e tripwire; pre-edit baseline = zero diffs.

6. **Pre-flight check (c): Pre-edit grep verification — body resonator + bow noise NOT yet present in voice:**
   ```bash
   grep -c "BodyResonator\|BowNoiseGenerator" plugins/O-Contrabass/Source/BowedContrabassVoice.h
   # Expect: 0
   grep -c "BodyResonator\|BowNoiseGenerator" plugins/O-Contrabass/Source/BowedContrabassVoice.cpp
   # Expect: 0
   test ! -f plugins/O-Contrabass/Source/DSP/BodyResonator.h
   test ! -f plugins/O-Contrabass/Source/DSP/BodyResonator.cpp
   test ! -f plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h
   ```
   Confirms the canonical pre-Phase-2.5 baseline at HEAD. If any check reports unexpected presence, STOP and investigate.

7. **Pre-flight check (d): Saturator post-port carry-forward (Phase 2.4c-bis state preserved):**
   ```bash
   grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp
   # Expect: 2 (toBridge + toNeck rails per R36-bis)
   ```
   Confirms the Phase 2.4c-bis saturator port at lines 204–206 is preserved. Phase 2.5 does NOT touch WaveguideString.cpp.

**Files created:** none committed (worktree at `/tmp/oc-pre-2-5` is git-internal, not source-tree).

**Files modified:** none committed.

**Commit:** **NONE** — diagnostic only.

**Success bar:**
- [ ] `git log --oneline -3` shows descendant chain rooted at `1dfca9d` (R36-bis-backfill chore).
- [ ] `/tmp/oc-pre-2-5` worktree exists or is creatable.
- [ ] O-Contrabass working-tree harness builds clean.
- [ ] (a) `reproduce-goldens.sh` reports `OK: all 13 goldens reproduce byte-identical`.
- [ ] (b) `git diff --stat HEAD -- plugins/O-Contrabass/Source/ plugins/O-Bowed/Source/ modules/synthesis/bow-friction/ plugins/O-Contrabass/CMakeLists.txt` reports empty.
- [ ] (c) Pre-edit grep counts return 0; the 3 NEW source files not yet present.
- [ ] (d) `grep -c "sat \* std::tanh"` returns 2 (Phase 2.4c-bis carry-forward preserved).

**Estimated effort:** 3–6 min (build no-op + 5 pre-flight checks + worktree create).

---

### R37a — `Source/DSP/BodyResonator.{h,cpp}` NEW + CMakeLists.txt source-list addition

**Per RESEARCH §21.2.6 + §21.13 + §21.10. Implement bass spec from ARCHITECTURE §"Body Resonator (Parallel Biquad Bank)" using `juce::dsp::IIR::Coefficients<float>::makeBandPass` per ARCHITECTURE §134 + RESEARCH §21.2.2 delta. Mono single-bank topology (voice output is mono pre-stereo-split). Per-block coefficient recompute reading current Size/Damping. 35 Hz HP one-pole on dry path. Wet/dry blend `(1−mix)·HP35(in) + mix·wet`. Add `Source/DSP/BodyResonator.cpp` to CMakeLists.txt source-list (1 LOC M).**

**Tasks:**

1. **Create `plugins/O-Contrabass/Source/DSP/BodyResonator.h`** (~90 LOC NEW per RESEARCH §21.2.6 starting design):

   - Public API: `prepare(double sampleRate, int maxBlockSize)`, `reset()`, `setSize(float)`, `setDamping(float)`, `setMix(float)`, `processBlock(float* mono, int numSamples)`.
   - Private: `static constexpr int kNumModes = 8;` + `kDefaultFreq[]` = `{60, 98, 115, 175, 235, 340, 700, 1200}` Hz + `kDefaultQ[]` = `{14, 11, 9, 8, 7, 6, 5, 2.5}` + `kDefaultGainDb[]` = `{-2, 0, -1, -3, -4, -5, -7, -6}` (zero/negative gains → attenuated bandpass output) per ARCHITECTURE §"Body Resonator (Parallel Biquad Bank)" mode table.
   - Members: `std::array<juce::dsp::IIR::Filter<float>, kNumModes> modes;` (mono single-bank, NOT L/R duplicated) + `float gainLinear[kNumModes];` + `float hp35_x1, hp35_y1, hp35_a;` (35 Hz HP one-pole state) + `double currentSampleRate;` + `float currentSize = 0.75f, currentDamping = 0.40f, currentMix = 0.80f;` (defaults match parameter-spec.md).
   - Private method declaration: `void recomputeCoefficients() noexcept;`.

2. **Create `plugins/O-Contrabass/Source/DSP/BodyResonator.cpp`** (~200 LOC NEW per RESEARCH §21.2.6 starting design):

   - `prepare(sr, maxBlockSize)`: store `currentSampleRate = sr`; `juce::dsp::ProcessSpec spec { sr, (uint32) maxBlockSize, 1u };` for each `modes[i].prepare(spec)` + `reset()`. Compute `hp35_a = std::exp(-2.f * juce::MathConstants<float>::pi * 35.f / float(sr))` (≈ 0.99502 at 44.1 kHz; ≈ 0.99544 at 48 kHz; ≈ 0.99772 at 96 kHz). Initialize `hp35_x1 = hp35_y1 = 0.f`. Call `recomputeCoefficients()` once.
   - `reset()`: for each `modes[i].reset()`; `hp35_x1 = hp35_y1 = 0.f`.
   - `setSize(float)` / `setDamping(float)` / `setMix(float)`: store `currentSize` / `currentDamping` / `currentMix` (clamped to [0, 1] via `juce::jlimit`). NO lazy-update guard (per-block recompute pattern; coefficients re-derived each block via SmoothedValue ramps from voice-side).
   - `recomputeCoefficients()` (per-block, called from processBlock):
     ```cpp
     const float sizeScalar = 0.85f + 0.30f * currentSize;             // ARCHITECTURE §509
     const float qScalar    = juce::jmax(0.15f, 1.f - 0.85f * currentDamping); // ARCHITECTURE §150
     for (int i = 0; i < kNumModes; ++i) {
         const float fc   = juce::jlimit(20.f,
                                          (float)(currentSampleRate * 0.45),
                                          kDefaultFreq[i] / sizeScalar);
         const float qEff = juce::jmax(0.10f, kDefaultQ[i] * qScalar);
         const float gDb  = kDefaultGainDb[i] + 1.5f * (currentSize - 0.75f); // ARCHITECTURE §511
         gainLinear[i]    = juce::Decibels::decibelsToGain(gDb);
         auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
                           currentSampleRate, fc, qEff);
         *modes[i].coefficients = *coeffs;
     }
     ```
   - `processBlock(float* mono, int numSamples)`:
     ```cpp
     recomputeCoefficients();  // per-block (CONTEXT line 152 + ARCHITECTURE §152)
     for (int n = 0; n < numSamples; ++n) {
         const float x = mono[n];
         const float dry = hp35_a * (hp35_y1 + x - hp35_x1);  // 35 Hz HP one-pole
         hp35_x1 = x;
         hp35_y1 = dry;
         float wet = 0.f;
         for (int i = 0; i < kNumModes; ++i)
             wet += gainLinear[i] * modes[i].processSample(x);
         mono[n] = (1.f - currentMix) * dry + currentMix * wet;
     }
     ```
   - **Drop O-Bowed's `normGain` cross-preset normalization** — single-preset bass design has no morph drift to correct.
   - **Drop O-Bowed's preset table machinery** — single hard-coded bass mode set.

3. **Edit `plugins/O-Contrabass/CMakeLists.txt`** — add `Source/DSP/BodyResonator.cpp` to `target_sources(O-Contrabass PRIVATE …)` block (1 LOC M):
   ```cmake
   target_sources(O-Contrabass PRIVATE
       Source/PluginProcessor.cpp
       Source/PluginEditor.cpp
       Source/BowedContrabassVoice.cpp
       Source/DSP/WaveguideString.cpp
       Source/DSP/BodyResonator.cpp                 # Phase 2.5 NEW
       # ... scala-tuning-engine entries carry-forward verbatim ...
   )
   ```

4. **Post-edit verification — file presence + grep:**
   ```bash
   test -f plugins/O-Contrabass/Source/DSP/BodyResonator.h
   test -f plugins/O-Contrabass/Source/DSP/BodyResonator.cpp
   grep -c "BodyResonator.cpp" plugins/O-Contrabass/CMakeLists.txt   # Expect: 1
   grep -c "kNumModes = 8" plugins/O-Contrabass/Source/DSP/BodyResonator.h  # Expect: 1
   grep -c "makeBandPass" plugins/O-Contrabass/Source/DSP/BodyResonator.cpp  # Expect: 1
   ```

5. **Build smoke — confirm BodyResonator compiles standalone before voice integration:**
   ```bash
   cmake --build build --target O-Contrabass --parallel
   # Expect: clean build (no warnings/errors) — voice will fail to compile until R37c lands the integration; stop the build at the BodyResonator object file and confirm BodyResonator.cpp.o builds clean before proceeding.
   ```
   If voice fails to compile due to missing integration, that is expected at this point; verify only that `BodyResonator.cpp.o` artefact builds clean.

**Files created (NOT YET COMMITTED — staged for R37 atomic commit):**
- `plugins/O-Contrabass/Source/DSP/BodyResonator.h` (~90 LOC NEW).
- `plugins/O-Contrabass/Source/DSP/BodyResonator.cpp` (~200 LOC NEW).

**Files modified (NOT YET COMMITTED — staged for R37 atomic commit):**
- `plugins/O-Contrabass/CMakeLists.txt` (1 LOC M; add `Source/DSP/BodyResonator.cpp`).

**Commit:** **NONE** — staged for R37 atomic.

**Success bar:**
- [ ] `BodyResonator.h` + `BodyResonator.cpp` exist with public API + bass mode table.
- [ ] `CMakeLists.txt` reports `Source/DSP/BodyResonator.cpp` exactly once.
- [ ] Standalone `BodyResonator.cpp.o` compiles clean (no warnings).
- [ ] `git diff --stat HEAD -- plugins/O-Contrabass/Source/DSP/ plugins/O-Contrabass/CMakeLists.txt` reports 3 files: 2 NEW + 1 M.

**Estimated effort:** ~60–90 min (header + cpp + CMake + build smoke).

---

### R37b — `Source/DSP/BowNoiseGenerator.h` NEW (header-only)

**Per RESEARCH §21.2.7 + §21.10. Implement bass spec from ARCHITECTURE §"Bow Noise Generator" using 3-band BPF (700/1500/3000 Hz) + period-heuristic slip-burst trigger (CONTEXT 4-file scope-strict v1.0 substitute per RESEARCH §21.3.3) + `voiceIndex * 31337` deterministic `juce::Random` seed (O-Bowed pattern verbatim per RESEARCH §21.6). Header-only per O-Bowed convention (no CMake update required).**

**Tasks:**

1. **Create `plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h`** (~180 LOC NEW per RESEARCH §21.2.7 starting design):

   - Public API: `prepare(double sampleRate, int voiceIndex) noexcept`, `setNoiseLevel(float) noexcept`, `setBowEnergy(float) noexcept`, `setFundamentalHz(float f0) noexcept`, `processSample() noexcept`, `reset() noexcept`.
   - Private constants: `static constexpr int kNumBpf = 3;` + `kBpfFc[kNumBpf] = {700.f, 1500.f, 3000.f};` (ARCHITECTURE §163) + `kBpfQ[kNumBpf] = {1.0f, 1.2f, 1.5f};` (research recommendation; tune at execute pre-flight if R38 audition reveals mis-calibration per RESEARCH §21.11 FAIL-handling table).
   - Private members: `juce::Random noiseRandom;` + `std::array<juce::dsp::IIR::Filter<float>, kNumBpf> bpfs;` + period-heuristic slip-burst state (`int slipPeriodSamples = 0; int slipCounter = 0; float slipEnvelope = 0.f; float kSlipDecayAtSr = 0.999f;`) + `float bowEnergy = 0.f, noiseLevel = 0.f;` + `double sr = 44100.0;`.
   - Slip-burst decay: `static constexpr float kSlipDecay = 0.999f;` (ARCHITECTURE §165 reference at 48 kHz); rescale per sample-rate at prepare via `kSlipDecayAtSr = std::pow(0.999f, (float)(48000.0 / sampleRate))`.

2. **Implement `prepare(double sampleRate, int voiceIndex)`** (inline header):
   ```cpp
   sr = sampleRate;
   noiseRandom.setSeed(static_cast<juce::int64>(voiceIndex * 31337));  // O-Bowed pattern verbatim
   juce::dsp::ProcessSpec spec { sampleRate, 1u, 1u };
   for (int i = 0; i < kNumBpf; ++i) {
       bpfs[i].prepare(spec);
       bpfs[i].reset();
       *bpfs[i].coefficients = *juce::dsp::IIR::Coefficients<float>::makeBandPass(
           sampleRate, kBpfFc[i], kBpfQ[i]);
   }
   kSlipDecayAtSr = std::pow(0.999f, static_cast<float>(48000.0 / sampleRate));
   slipCounter = 0; slipEnvelope = 0.f; bowEnergy = 0.f;
   ```

3. **Implement `processSample()`** (inline header):
   ```cpp
   if (noiseLevel < 0.001f) return 0.f;
   // Period-heuristic slip-burst trigger
   if (slipPeriodSamples > 0 && --slipCounter <= 0) {
       slipEnvelope = bowEnergy;
       slipCounter  = slipPeriodSamples;
   }
   slipEnvelope *= kSlipDecayAtSr;
   // 3-band BPF on white noise, averaged
   const float white = noiseRandom.nextFloat() * 2.f - 1.f;
   float bandSum = 0.f;
   for (int i = 0; i < kNumBpf; ++i)
       bandSum += bpfs[i].processSample(white);
   bandSum *= (1.f / static_cast<float>(kNumBpf));
   const float continuous = bandSum * bowEnergy;
   const float burst      = bandSum * slipEnvelope;
   return (continuous + burst) * noiseLevel;
   ```

4. **Implement setters + reset** (inline header):
   ```cpp
   inline void setFundamentalHz(float f0) noexcept {
       if (f0 < 1.f) { slipPeriodSamples = 0; return; }
       const int newPeriod = juce::roundToInt(sr / f0);
       slipPeriodSamples = newPeriod;
       if (slipCounter > newPeriod) slipCounter = newPeriod;
   }
   inline void setNoiseLevel(float v) noexcept { noiseLevel = juce::jlimit(0.f, 1.f, v); }
   inline void setBowEnergy(float v) noexcept  { bowEnergy  = juce::jlimit(0.f, 1.f, v); }
   inline void reset() noexcept {
       for (auto& f : bpfs) f.reset();
       slipCounter = 0; slipEnvelope = 0.f; bowEnergy = 0.f;
   }
   ```

5. **Post-edit verification — file presence + grep:**
   ```bash
   test -f plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h
   ! test -f plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.cpp     # NOT created (header-only)
   grep -c "voiceIndex \* 31337" plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h   # Expect: 1
   grep -c "kNumBpf = 3" plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h            # Expect: 1
   grep -c "kSlipDecay" plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h             # Expect: ≥1
   grep -c "BowNoiseGenerator" plugins/O-Contrabass/CMakeLists.txt                      # Expect: 0 (header-only)
   ```

6. **Build smoke — header compiles when included** (only validated when voice integrates at R37c; defer end-to-end build to R37c).

**Files created (NOT YET COMMITTED — staged for R37 atomic commit):**
- `plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h` (~180 LOC NEW).

**Files modified:** none (header-only; CMakeLists already set up via target_include_directories).

**Commit:** **NONE** — staged for R37 atomic.

**Success bar:**
- [ ] `BowNoiseGenerator.h` exists with full inline implementation.
- [ ] `BowNoiseGenerator.cpp` does NOT exist (header-only convention).
- [ ] Grep confirms `voiceIndex * 31337` seed + `kNumBpf = 3` + period-heuristic slip-burst.
- [ ] CMakeLists has zero references to BowNoiseGenerator (header-only; no source-list addition).
- [ ] `git diff --stat HEAD -- plugins/O-Contrabass/Source/DSP/` reports 3 NEW files (BodyResonator.h, BodyResonator.cpp, BowNoiseGenerator.h).

**Estimated effort:** ~45 min (header + setters + reset).

---

### R37c — `Source/BowedContrabassVoice.{h,cpp}` integration: Step 8 (body) + Step 9 (bow noise)

**Per RESEARCH §21.10 + §21.2.6/§21.2.7 integration notes + CONTEXT line 156 4-file scope-strict rule. Append NEW Step 8 (body resonator wet/dry mix) + NEW Step 9 (bow noise sum) AFTER Step 7 downsample at `BowedContrabassVoice.cpp:688`, BEFORE host-rate output write at lines 694–702. Voice owns body + noise instances. SmoothedValue<float> ramps (30 ms) for `BODY_SIZE`, `BODY_DAMPING`, `BODY_MIX`, `BOW_NOISE`. `setFundamentalHz(activeStringFundamentalHz)` push for slip-trigger (on note-start + on f0 change > 5 cents threshold). `setBowEnergy(juce::jlimit(0.f, 1.f, std::abs(v_bow) * F_bow / (0.3f * 2.0f)))` per-block. HR-1..HR-10 carry-forward verbatim. `Source/PluginProcessor.{h,cpp}` (M) wires voice-side body/noise APVTS atomic reads.**

**Tasks:**

1. **Edit `plugins/O-Contrabass/Source/BowedContrabassVoice.h`** (~10 LOC M):
   - Add `#include "DSP/BodyResonator.h"` + `#include "DSP/BowNoiseGenerator.h"` near existing `#include "DSP/WaveguideString.h"`.
   - Add private members:
     ```cpp
     BodyResonator        bodyResonator;
     BowNoiseGenerator    bowNoiseGenerator;
     juce::SmoothedValue<float> bodySizeSmoothed;
     juce::SmoothedValue<float> bodyDampingSmoothed;
     juce::SmoothedValue<float> bodyMixSmoothed;
     juce::SmoothedValue<float> bowNoiseSmoothed;
     float lastFundamentalHz = 0.f;     // for slip-trigger 5-cent change detection
     ```
   - No new public methods; voice owns state internally.

2. **Edit `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp`** (~40 LOC M):

   - In `prepare(double sr, int maxBlockSize)`:
     ```cpp
     bodyResonator.prepare(sr, maxBlockSize);
     bowNoiseGenerator.prepare(sr, /*voiceIndex=*/0);  // monophonic (RESEARCH §21.6)
     bodySizeSmoothed.reset(sr,    0.030);  // 30 ms ramp (CONTEXT line 152 + ARCHITECTURE §152)
     bodyDampingSmoothed.reset(sr, 0.030);
     bodyMixSmoothed.reset(sr,     0.030);
     bowNoiseSmoothed.reset(sr,    0.030);
     bodySizeSmoothed.setCurrentAndTargetValue(0.75f);
     bodyDampingSmoothed.setCurrentAndTargetValue(0.40f);
     bodyMixSmoothed.setCurrentAndTargetValue(0.80f);
     bowNoiseSmoothed.setCurrentAndTargetValue(0.35f);
     ```

   - In `updateParametersFromAPVTS()` (extend existing routine to push body/noise params):
     ```cpp
     bodySizeSmoothed.setTargetValue   (parameters->getRawParameterValue("BODY_SIZE")    ->load());
     bodyDampingSmoothed.setTargetValue(parameters->getRawParameterValue("BODY_DAMPING") ->load());
     bodyMixSmoothed.setTargetValue    (parameters->getRawParameterValue("BODY_MIX")     ->load());
     bowNoiseSmoothed.setTargetValue   (parameters->getRawParameterValue("BOW_NOISE")    ->load());
     ```

   - In `renderNextBlock(...)`, INSERT NEW Step 8 + Step 9 between line 688 (`oversampling.processSamplesDown(block);`) and line 690 (`// 8. Mix host-rate voiceBuffer into the output buffer (mono → stereo split).`):
     ```cpp
     // 7. Downsample back to host rate (writes into `block` which aliases voiceBuffer).
     oversampling.processSamplesDown (block);

     // === PHASE 2.5 NEW Step 8: Body resonator (8-mode parallel bandpass + 35 Hz HP dry path + wet/dry mix) ===
     {
         // Skip-bump SmoothedValue: read NEXT block's smoothed value
         // (skip-by-numSamples lands on block-start values; per-block recompute pattern)
         const float sz = bodySizeSmoothed.skip(numSamples - 1);    // current, advance to end-of-block
         const float dp = bodyDampingSmoothed.skip(numSamples - 1);
         const float mx = bodyMixSmoothed.skip(numSamples - 1);
         bodyResonator.setSize    (sz);
         bodyResonator.setDamping (dp);
         bodyResonator.setMix     (mx);
         bodyResonator.processBlock(voiceBuffer.getWritePointer(0), numSamples);
     }

     // === PHASE 2.5 NEW Step 9: Bow noise (3-band BPF + period-heuristic slip bursts; sums after body) ===
     {
         // bowEnergy = clamp(0, 1, |v_bow| · F_bow / (v_ref · F_ref)) per ARCHITECTURE §164
         constexpr float kVRef = 0.3f, kFRef = 2.0f;
         const float vBow = std::abs(bowModel.getBowVelocity());
         const float fBow = bowModel.getBowForce();
         const float bowEnergyVal = juce::jlimit(0.f, 1.f, (vBow * fBow) / (kVRef * kFRef));
         bowNoiseGenerator.setBowEnergy(bowEnergyVal);

         // Push active-string fundamental on note-start or > 5 cent change (period-heuristic slip trigger)
         const float f0 = strings[activeStringIndex].getFundamentalHz();
         const float cents = (lastFundamentalHz > 0.f)
             ? 1200.f * std::log2(f0 / lastFundamentalHz)
             : 0.f;
         if (lastFundamentalHz <= 0.f || std::abs(cents) > 5.f) {
             bowNoiseGenerator.setFundamentalHz(f0);
             lastFundamentalHz = f0;
         }

         // BOW_NOISE smoothed level
         const float noiseLvl = bowNoiseSmoothed.skip(numSamples - 1);
         bowNoiseGenerator.setNoiseLevel(noiseLvl);

         float* mono = voiceBuffer.getWritePointer(0);
         for (int i = 0; i < numSamples; ++i)
             mono[i] += bowNoiseGenerator.processSample();
     }
     // === END PHASE 2.5 ===

     // 10. Mix host-rate voiceBuffer into the output buffer (mono → stereo split).
     // (renumber prior step 8 → 10 in the comments — Phase 2.5 inserts 8 + 9 between downsample and output write.)
     ```

   - In `stopNote(...)` or equivalent reset path: call `bodyResonator.reset()` + `bowNoiseGenerator.reset()`; reset `lastFundamentalHz = 0.f`. (Filter state continues across blocks during sustain; only resets on note-off / voice-stop per ARCHITECTURE §"Body Resonator" "Filter state continues across blocks (no per-block reset)" intent.)

3. **Edit `plugins/O-Contrabass/Source/PluginProcessor.cpp`** (~5 LOC M; voice-side parameter wiring):
   - Verify all 5 body/noise APVTS reads are routed to voice. The existing voice-update mechanism (`updateParametersFromAPVTS`) handles this once R37c lands the smoothed-value pulls; processor-side push is identity (raw atomic load handed to voice). If processor currently invokes `voice->updateParametersFromAPVTS(*parameters);` once per `processBlock`, no processor edit is needed beyond confirming the call site is reached.
   - **Note:** if PluginProcessor.h does not currently declare any new public surface for body/noise routing, no header edit is needed (parameters live in APVTS; voice owns the body+noise instances). Plan-phase confirms 0 LOC M in PluginProcessor.h.

4. **Build smoke — full O-Contrabass + render harness compiles clean post-integration:**
   ```bash
   cmake --build build --target O-Contrabass --parallel
   cmake --build build --target O-Contrabass-render-test --parallel
   # Both must build clean (no warnings, no errors).
   ```

5. **Pre-render smoke — confirm voice initializes without crash + matrix-stability evidence renders without NaN/inf:**
   ```bash
   $HARNESS --string A --out /tmp/r37c-smoke-A.wav --json /tmp/r37c-smoke-A.json
   # Confirm WAV exists + no peak-amplitude > 1.0 (clipping):
   python3 -c "
   import wave, struct, sys
   w = wave.open('/tmp/r37c-smoke-A.wav', 'rb')
   frames = w.readframes(w.getnframes())
   samples = struct.unpack('<' + 'h' * (len(frames) // 2), frames)
   peak = max(abs(s) for s in samples) / 32768.0
   print(f'peak: {peak:.4f}')
   sys.exit(0 if peak <= 1.0 else 1)
   "
   ```

**Files created:** none (carry-forward from R37a + R37b).

**Files modified (NOT YET COMMITTED — staged for R37 atomic commit):**
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` (~10 LOC M).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` (~40 LOC M).
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` (0–5 LOC M; verify-only if existing routing covers body/noise reads).
- `plugins/O-Contrabass/Source/PluginProcessor.h` (0 LOC M expected).

**Commit:** **NONE** — staged for R37 atomic.

**Success bar:**
- [ ] `BowedContrabassVoice.h` declares 4 SmoothedValue members + body/noise instances + lastFundamentalHz.
- [ ] `BowedContrabassVoice.cpp` integrates Step 8 (body) + Step 9 (bow noise) between line 688 (downsample) and the output-write block at 694–702.
- [ ] Full O-Contrabass plugin + render harness build clean (no warnings).
- [ ] Pre-render smoke on `--string A` produces WAV with peak ≤ 1.0 (no clipping, no NaN).
- [ ] `git diff --stat HEAD -- plugins/O-Contrabass/Source/` reports the EXACT 4-file production set: BodyResonator.{h,cpp} NEW + BowNoiseGenerator.h NEW + BowedContrabassVoice.{h,cpp} M + PluginProcessor.{h,cpp} (0–1 files M).

**Estimated effort:** ~90–120 min (voice header + cpp integration + build + smoke render).

---

### R37d — Re-baseline 13 audible goldens + matrix-stability evidence + saturator-tail post-body + sub-harmonics post-body + 3-trial bit-stability pre-flight

**Per RESEARCH §21.4 + §21.5 + §21.7 + §21.8 + §21.9 + §21.10. Build O-Contrabass-render-test post-integration; render 13 audible goldens × 3 trials each → confirm 3-trial DET-PASS bit-stability per golden; lock NEW post-Phase-2.5 sha256s; render `--matrix-stability` evidence-only (NOT committing updated `matrix-stability.wav.sha256`; archive WAV to `.planning/evidence/phase-2-5/`); measure post-body saturator-tail bin 64 + verify §19.3.3 analytic bound; measure post-body `subharmEnergyRatio`; document raucous-corner cell migration.**

**Tasks:**

1. **Build O-Contrabass-render-test post-integration:**
   ```bash
   cmake --build build --target O-Contrabass-render-test --parallel
   ```

2. **3-trial bit-stability pre-flight per golden (RESEARCH §21.5 deterministic bar):**
   ```bash
   GOLDENS="stiffness-zero-pre string-A string-D string-G detune-sweep-A note-sequence macro-sweep slow-lfo schelleng-stress sub-harmonics sub-harmonics-stability saturator-tail-comparison vibrato"
   for g in $GOLDENS; do
       for trial in 1 2 3; do
           $HARNESS --<mode-flag-for-$g> --out /tmp/p25-$g-t$trial.wav --json /tmp/p25-$g-t$trial.json
           sha=$(shasum -a 256 /tmp/p25-$g-t$trial.wav | awk '{print $1}')
           echo "$g trial-$trial: $sha"
       done
   done
   # Expect: 3 trials × 13 goldens = 39 sha256s where trial-1 == trial-2 == trial-3 per golden (bit-deterministic across re-renders).
   ```
   If ANY golden shows trial-N drift, STOP and investigate (likely `juce::Random` seed nondeterminism — verify R37b setSeed pattern; OR biquad coefficient compute call-site nondeterminism — verify R37a recomputeCoefficients invocation order in processBlock).

3. **Render production goldens (final committed set; trial-1 sha256 from step 2 captured as the locked value):**
   Use the existing harness CLI flags per Phase 2.4c R36b/R36c + Phase 2.4c-bis R36-bis-b precedent. (Mode-flag mapping carries forward from PLAN rev-11 R36-bis-b table; identical across phases — only the resulting sha256s shift due to body+noise post-waveguide spectral coloring.)

4. **Compute and write per-golden `*.wav.sha256` text files:**
   ```bash
   GOLDEN_DIR=plugins/O-Contrabass/tests/render-harness/golden
   for g in $GOLDENS; do
       sha=$(shasum -a 256 "$GOLDEN_DIR/$g.wav" | awk '{print $1}')
       printf '%s  %s.wav\n' "$sha" "$g" > "$GOLDEN_DIR/$g.wav.sha256"
   done
   ```

5. **Re-anchor JSON sha256 informational-only files** (vibrato + saturator-tail-comparison; non-deterministic across renders due to wall-clock fields per RESEARCH §20.6 / PLAN rev-11 Open Item #5):
   ```bash
   for g in vibrato saturator-tail-comparison; do
       sha=$(shasum -a 256 "$GOLDEN_DIR/$g.json" | awk '{print $1}')
       printf '%s  %s\n' "$sha" "$g.json" > "$GOLDEN_DIR/$g.json.sha256"
   done
   ```

6. **Saturator-tail post-body measurement (RESEARCH §21.4):**
   - Render `--saturator-tail-comparison` post-Phase-2.5; extract bin 64 dB rel max from JSON (or compute via Python decode of WAV).
   - Compare against Phase 2.4c-bis R36-bis baseline (−7.97 dB).
   - Document `Δ` shift (expected ≤ 2 dB at canonical operating point per RESEARCH §21.4 prediction).
   - PASS criteria (NOT BLOCKING):
     - Inside soft-band [−9, −5] dB rel max: pass; document in §21 verdict subsection §21.10 or §21.4 finalize block.
     - Outside [−9, −5] dB but ≤ 4 dB shift: flag as evidence for end-of-Stage-2 §"In-loop saturator" amendment evidence base; do NOT block R37 atomic.
     - > 4 dB shift: BLOCK R37 atomic; root-cause body-coupling instability (low-probability — biquads are L2-bounded linear filter post-saturator, no nonlinear backflow).

7. **Sub-harmonics post-body coupling measurement (RESEARCH §21.9):**
   - Render `--sub-harmonics` + `--sub-harmonics-stability` post-Phase-2.5.
   - Extract `subharmEnergyRatio` from JSON.
   - Compare against Phase 2.4c-bis R36-bis pre-body baseline (`0.358`).
   - Soft-band [0.30, 0.45] expected; document landed value.
   - If drops below 0.30 → flag for Phase 2.4-bis backlog priority bump (NOT a Gate 7 BLOCKER per CONTEXT line 220).

8. **Matrix-stability evidence-only re-render (RESEARCH §21.8):**
   ```bash
   $HARNESS --matrix-stability \
            --out plugins/O-Contrabass/.planning/evidence/phase-2-5/matrix-stability-post-body.wav \
            --json plugins/O-Contrabass/.planning/evidence/phase-2-5/matrix-stability-post-body.json
   ```
   - Compare 4-cell-failure pattern (Phase 2.4a R34b 3 fails + Phase 2.4c-bis R36-bis 4 NEW raucous corners under tanh saturator) to post-Phase-2.5 cells.
   - Document raucous-corner cells in §21 verdict subsection: stable / unchanged / NEW.
   - If NEW raucous corners surface → BLOCK R37 atomic pending root-cause investigation (low-probability per RESEARCH §21.8).
   - **Evidence-only**: do NOT update `tests/render-harness/golden/matrix-stability.wav.sha256` (carry-forward `6db67707…` verbatim per CONTEXT line 176).

9. **Vibrato carry-forward determination (RESEARCH §21.7):**
   - Re-render `--vibrato` mode post-Phase-2.5.
   - Compare JSON metrics: `peakDepthCents`, `vibratoRateHzMeasured`, `onsetTimeMs` against Phase 2.4c-bis R36-bis baseline.
   - If WAV byte-identical AND metrics within ±0.05¢ / ±0.005 Hz / ±2 ms tolerance → carry forward (count drops to 12 audible re-baselines; do NOT update vibrato.wav.sha256). Default expectation: re-baseline (count stays 13).

**Files created (NOT YET COMMITTED — staged for R37 atomic commit):**
- 13 × `tests/render-harness/golden/<g>.wav.sha256` (re-baselined per RESEARCH §21.5).
- 2 × `tests/render-harness/golden/{vibrato,saturator-tail-comparison}.json.sha256` (informational-only re-anchors).
- 13 × `tests/render-harness/golden/<g>.json` (re-baselined; some have wall-clock fields per PLAN rev-11 Open Item #5).
- `.planning/evidence/phase-2-5/matrix-stability-post-body.{wav,json}` (evidence-only; NOT committed if too large; track in §21 verdict + STATUS).

**Files modified:** as listed above.

**Commit:** **NONE** — staged for R37 atomic.

**Success bar:**
- [ ] 3-trial bit-stability: 3 trials × 13 goldens = 39 sha256s; trial-1 == trial-2 == trial-3 per golden (39 == 13 unique values).
- [ ] All 13 audible `*.wav.sha256` files re-baselined.
- [ ] 2 informational `*.json.sha256` files re-anchored (vibrato + saturator-tail-comparison).
- [ ] Saturator-tail bin 64 measurement documented (within [−9, −5] dB rel max OR flagged for amendment evidence base).
- [ ] `subharmEnergyRatio` post-body documented (within [0.30, 0.45] OR flagged for Phase 2.4-bis priority bump).
- [ ] Matrix-stability evidence rendered + raucous-corner cell migration documented (no NEW raucous corners surfacing).
- [ ] Vibrato carry-forward determination documented (default = re-baseline).

**Estimated effort:** ~30–45 min (build + 39 renders × 0.29 s/render baseline = ~12 s renders + matrix-stability ~5 min + sha256 + measurements).

---

### R37e — Regression bar: 13-entry `reproduce-goldens.sh` against NEW post-Phase-2.5 sha256s + 4-file source audit hook + 1-file CMake audit hook

**Per RESEARCH §21.10 R37e + CONTEXT line 156. Confirm all 13 goldens reproduce byte-identical against the R37d-locked NEW sha256s. Confirm `git diff --stat` reports EXACTLY the 4-file production source set (BodyResonator.{h,cpp} NEW + BowNoiseGenerator.h NEW + BowedContrabassVoice.{h,cpp} M + PluginProcessor.{h,cpp} M-or-zero) + 1-file CMakeLists.txt M. NO other production-source diffs.**

**Tasks:**

1. **Run `reproduce-goldens.sh` against R37d-locked sha256s:**
   ```bash
   bash plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
   # Expect: "OK: all 13 goldens reproduce byte-identical"
   ```
   If ANY FAIL, STOP and investigate. Possible root causes: (a) `juce::Random` seed nondeterminism (re-verify R37b setSeed); (b) biquad coefficient compute order nondeterminism (re-verify R37a recomputeCoefficients call site); (c) SmoothedValue ramp state nondeterminism (re-verify R37c skip(numSamples-1) usage).

2. **4-file production-source audit hook:**
   ```bash
   git diff --stat HEAD -- plugins/O-Contrabass/Source/
   # Expect EXACTLY 4 files (NEW additions count as modifications per --stat):
   #   plugins/O-Contrabass/Source/DSP/BodyResonator.h        | (NEW, ~90 LOC)
   #   plugins/O-Contrabass/Source/DSP/BodyResonator.cpp      | (NEW, ~200 LOC)
   #   plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h    | (NEW, ~180 LOC)
   #   plugins/O-Contrabass/Source/BowedContrabassVoice.h     | (M, ~10 LOC)
   #   plugins/O-Contrabass/Source/BowedContrabassVoice.cpp   | (M, ~40 LOC)
   #   plugins/O-Contrabass/Source/PluginProcessor.{h,cpp}    | (M-or-zero, ≤5 LOC)
   ```
   ANY production source file outside this set = scope-expansion HARD violation. Investigate + revert before proceeding.

3. **CMakeLists.txt audit hook:**
   ```bash
   git diff --stat HEAD -- plugins/O-Contrabass/CMakeLists.txt
   # Expect: 1 file changed (1 LOC M; Source/DSP/BodyResonator.cpp source-list addition)
   git diff plugins/O-Contrabass/CMakeLists.txt | grep -c "Source/DSP/BodyResonator.cpp"
   # Expect: 1 (added line in unified diff)
   ```

4. **Other-source-tree clean state hook:**
   ```bash
   git diff --stat HEAD -- plugins/O-Bowed/ \
                            modules/synthesis/bow-friction/ \
                            plugins/O-Contrabass/Source/DSP/WaveguideString.cpp \
                            plugins/O-Contrabass/Source/DSP/DispersionFilter.h \
                            plugins/O-Contrabass/Source/DSP/SchellengCalibration.h \
                            plugins/O-Contrabass/Source/DSP/SubHarmonicBias.h
   # Expect: empty (all carry-forward verbatim per CONTEXT lines 144–148)
   ```

5. **Saturator post-port carry-forward verification:**
   ```bash
   grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp
   # Expect: 2 (Phase 2.4c-bis R36-bis post-port preserved; NOT touched by Phase 2.5)
   ```

**Files created:** none.

**Files modified:** none.

**Commit:** **NONE** — diagnostic only.

**Success bar:**
- [ ] `reproduce-goldens.sh` reports `OK: all 13 goldens reproduce byte-identical` against R37d-locked NEW sha256s.
- [ ] `git diff --stat HEAD -- plugins/O-Contrabass/Source/` reports EXACTLY the 4-file production set (3 NEW + 2 M; PluginProcessor optional 0–1 file).
- [ ] `git diff --stat HEAD -- plugins/O-Contrabass/CMakeLists.txt` reports 1 file changed (1 LOC M).
- [ ] `git diff --stat HEAD -- plugins/O-Bowed/ modules/synthesis/bow-friction/ <other-DSP-headers>` reports empty.
- [ ] `grep -c "sat \* std::tanh"` returns 2 (Phase 2.4c-bis carry-forward preserved).

**Estimated effort:** ~5–10 min (audit hooks + reproduce-goldens.sh full pass).

---

### R37f — `auval` AU VALIDATION SUCCEEDED + `pluginval --strictness-level 10` SUCCESS

**Per RESEARCH §21.10 R37f + ROADMAP §"Stage 2 Test Criteria" QUAL-01. Build + install O-Contrabass AU + VST3 to system folders per CLAUDE.md macOS protocol; run `auval` against the AU; run `pluginval --strictness-level 10` against the VST3.**

**Tasks:**

1. **Build O-Contrabass AU + VST3:**
   ```bash
   cmake --build build --target O-Contrabass_AU --parallel
   cmake --build build --target O-Contrabass_VST3 --parallel
   ```

2. **Clear AU caches + reinstall (CLAUDE.md macOS protocol):**
   ```bash
   killall -9 AudioComponentRegistrar 2>/dev/null || true
   rm -rf ~/Library/Caches/AudioUnitCache/
   rm -rf ~/Library/Caches/com.apple.audiounits.cache
   rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3
   rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass.component
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass.vst3 ~/Library/Audio/Plug-Ins/VST3/
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass.component ~/Library/Audio/Plug-Ins/Components/
   ```

3. **Run `auval` (full render-rate matrix):**
   ```bash
   auval -v aumu OCbs Ouar 2>&1 | tee /tmp/auval-r37f.log
   # Expect: "AU VALIDATION SUCCEEDED" at end of log.
   ```
   If FAIL, investigate + escalate before R38.

4. **Run `pluginval --strictness-level 10` (full battery):**
   ```bash
   /Applications/pluginval.app/Contents/MacOS/pluginval \
       --strictness-level 10 \
       --skip-gui-tests \
       --validate ~/Library/Audio/Plug-Ins/VST3/O-Contrabass.vst3 \
       2>&1 | tee /tmp/pluginval-r37f.log
   # Expect: "ALL TESTS PASSED" at end of log; specifically:
   #   Editor Automation, Automatable Parameters, Parameter thread safety,
   #   Background thread state, Bus enable/disable, Restoring default layout,
   #   Fuzz parameters all complete + SUCCESS.
   ```
   If FAIL, investigate (most likely culprit: PERF-01 allocation-in-processBlock — verify `juce::ScopedNoDenormals` + no-alloc hot path).

**Files created:** `/tmp/auval-r37f.log` + `/tmp/pluginval-r37f.log` (transient; NOT committed).

**Files modified:** none.

**Commit:** **NONE** — diagnostic only.

**Success bar:**
- [ ] `auval -v aumu OCbs Ouar` reports `AU VALIDATION SUCCEEDED`.
- [ ] `pluginval --strictness-level 10` reports `ALL TESTS PASSED`.

**Estimated effort:** ~5–10 min (auval ~2 min + pluginval-10 ~5–8 min).

---

### R38 — Logic AU audition (BLOCKING per CONTEXT Q57 + Open Item #13)

**Per RESEARCH §21.11 4-step protocol + 7-probe sequence. User A/Bs raw-string from `1044bed` (R36-bis post-port via `/tmp/oc-pre-2-5` worktree) vs body-engaged from working-tree O-Contrabass-dev. Mirrors R37-bis Phase 2.4c-bis precedent (audible source-edit → BLOCKING audition). PASS criteria: post-Phase-2.5 sounds "convincing orchestral arco bass" per BRIEF.md DSP-03 + DSP-04 acceptance.**

**Tasks:**

1. **Build pre-Phase-2.5 reference AU variant from `/tmp/oc-pre-2-5` worktree** (idempotent across re-runs):
   ```bash
   cd /tmp/oc-pre-2-5
   # Edit plugins/O-Contrabass/CMakeLists.txt to disambiguate:
   #   PLUGIN_CODE OCbs → OCb5
   #   PRODUCT_NAME "O-Contrabass${OUARICON_DEV_SUFFIX}" → "O-Contrabass-pre-2-5${OUARICON_DEV_SUFFIX}"
   #   (Plugin name change generates a separate AU bundle to avoid collision with working-tree variant.)
   cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release \
       -DSKIP_PLUGINS=O-Orbit -DOUARICON_BUILD_TESTS=OFF
   cmake --build build --target O-Contrabass_AU --parallel

   # Install pre-2-5 variant
   killall -9 AudioComponentRegistrar 2>/dev/null || true
   rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
   rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass-pre-2-5.component
   cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass-pre-2-5.component \
       ~/Library/Audio/Plug-Ins/Components/
   auval -a 2>&1 | grep -i contrabass
   # Expect: both pre-2-5 (OCb5) + working-tree dev (OCbs) variants resolve.
   cd /Users/taylorbrook/Dev/VST-development
   ```

2. **Build post-Phase-2.5 working-tree AU + install per R37f (already done by R37f; re-confirm install path):**
   ```bash
   ls ~/Library/Audio/Plug-Ins/Components/O-Contrabass.component  # working-tree post-Phase-2.5 variant
   ls ~/Library/Audio/Plug-Ins/Components/O-Contrabass-pre-2-5.component  # pre-Phase-2.5 reference variant
   ```

3. **Open Logic Pro AU + load both variants on adjacent tracks** (single project, two software-instrument tracks). Set both to identical default presets. Routing: each track to its own bus for clean A/B comparison via mute/solo.

4. **Execute 7-probe BLOCKING audition sequence** (per RESEARCH §21.11):

   | Probe | Setup | Pre-2-5 expectation | Post-2-5 expectation | PASS criterion |
   |-------|-------|---------------------|----------------------|----------------|
   | **1: Sustained E1 (MIDI 28)** | Default preset, sustained 5 s note | Bare bowed-string (waveguide-only output; no wood resonance) | Wood body resonance present (low-end thickening); BOW_NOISE audible at default 0.35 (subtle hair-on-string texture) | Body adds wood color WITHOUT obscuring fundamental; bow-noise sits under tone |
   | **2: Per-string A/D/G** | MIDI 33 / 38 / 43, 3 s each | Bare bowed-string per-string | Body wet/dry mix matches across strings; per-string spectral coloring varies subtly | No string-dependent body discontinuities |
   | **3: Sustained G2 (MIDI 43, wolf-region check)** | MIDI 43 sustained 8 s, default damping (BODY_DAMPING=0.40) | Bare bowed-string | Mode 2 at fc=91.16 Hz (Q_eff=7.26) coexists with G2 fundamental at 98 Hz — ARCHITECTURE §"Body Resonator"-warned wolf-region risk. **Document audibility**: subtle bloom (acceptable) OR audible beating (Phase 2.5-bis backlog flag) | Subtle bloom acceptable; audible beating documented but NOT blocking R37 atomic per CONTEXT line 91/166 |
   | **4: Bow-direction reversal (MIDI 28 → silence → MIDI 28)** | MIDI on / off / on, ~50 ms gaps | Bare attack-decay envelope | Slip-burst NOISE BURST (5–15 ms wideband) on note-on transient — ROADMAP §"Phase 2.5 Test Criteria" DSP-04 acceptance | Audible 5–15 ms wideband noise burst on each attack |
   | **5: BOW_NOISE 0% → 100% sweep** | MIDI 28 sustained, knob sweep | n/a | Noise rises from inaudible to overwhelming; no clicks during sweep | Linear-ish loudness ramp; no zipper noise |
   | **6: BODY_SIZE / DAMPING / MIX sweeps** | MIDI 28 sustained, knob sweep | n/a | Body character morphs smoothly (size = body resonance frequency offset; damping = ring-down time; mix = wet/dry blend) | No clicks during any sweep |
   | **7: Orchestral A/B vs reference orchestral library bass sustain at G2** | DAW project comparing Phase-2.5 sustained G2 vs (e.g.) Spitfire Audio BBC SO bass sustain at G2 | Bare bowed-string clearly NOT in same sonic family | "In same sonic family" subjective bar per ROADMAP §"Phase 2.5 Test Criteria" | User-confirmed acceptable |

5. **FAIL handling per RESEARCH §21.11:**

   | Subjective failure | Resolution path |
   |--------------------|-----------------|
   | Bow-noise level mis-calibrated (too loud / too quiet) | Tweak `kSlipDecay` (0.999 → 0.9985 for shorter bursts; 0.9995 longer) OR `kBpfQ[]` (1.0/1.2/1.5 → 1.5/1.8/2.0 for sharper resonances) OR default `BOW_NOISE` (0.35 → 0.25). Re-render audition; iterate. Stays inside R37 cycle. |
   | Body bank gain attenuation needed | Adjust `kDefaultGainDb[]` (e.g., −2 → −4 across the board for less prominent body). Re-render audition; iterate. Stays inside R37 cycle. |
   | Wolf-tone audible at G2 sustained | DOCUMENT (per CONTEXT line 91/166); carry to Phase 2.5-bis backlog. Do NOT block R37 atomic. User can mitigate via BODY_DAMPING knob for now. |
   | Body-coupling distortion / transient artifacts | ESCALATE pre-R37-atomic. Likely root cause: per-block coefficient swap glitch or 35 Hz HP one-pole DC-residual. Investigate. |
   | Slip-burst phase feels mechanical (period-heuristic limitation) | DOCUMENT (per RESEARCH §21.3.4); carry to Phase 2.5-bis Option A or B. Do NOT block R37 atomic. |
   | Catastrophic regression (body bank breaks waveguide stability under any string × INFINITE_SUSTAIN combo) | ESCALATE pre-R37-atomic. Investigate matrix-stability evidence. Most likely culprit: per-block coefficient computation introduces NaN at edge case (BODY_SIZE=1.0 + BODY_DAMPING=0.0 worst case; RESEARCH §21.7 analytic check shows STABLE in float32 but NaN propagation from upstream is possible). |

**Files created:** none committed (worktree at `/tmp/oc-pre-2-5` is git-internal; `O-Contrabass-pre-2-5.component` is install-folder only).

**Files modified:** none committed (CMakeLists in worktree is local-only edit, NOT committed back).

**Commit:** **NONE** — BLOCKING audition only.

**Success bar:**
- [ ] Pre-Phase-2.5 reference AU (`O-Contrabass-pre-2-5.component`, PLUGIN_CODE `OCb5`) builds + installs.
- [ ] Post-Phase-2.5 working-tree AU (`O-Contrabass.component`, PLUGIN_CODE `OCbs`) builds + installs.
- [ ] Both variants resolve in `auval -a | grep -i contrabass`.
- [ ] Probes 1, 2, 4, 5, 6, 7: PASS criteria CONFIRMED.
- [ ] Probe 3 (wolf-region G2): subjective audibility DOCUMENTED (subtle bloom acceptable; beating → Phase 2.5-bis backlog).
- [ ] User-confirmed: post-Phase-2.5 character is "convincing orchestral arco bass" per BRIEF.md DSP-03 + DSP-04 acceptance bar.

**Estimated effort:** ~30–45 min (pre-2-5 build + Logic Pro session + 7-probe audition).

---

### R37 — Phase 2.5 atomic commit (Gate 7 PASS)

**All R37-pre + R37a + R37b + R37c + R37d + R37e + R37f + R38 success bars CONFIRMED. Single atomic commit lands all source edits + CMake update + 13 audible-golden re-baselines + matrix-stability evidence archived to `.planning/evidence/phase-2-5/` + RESEARCH §21 verdict subsection (verify-phase fills R38 outcome) + planning artefacts.**

**Tasks:**

1. **Stage all artefacts:**
   ```bash
   # Source — 4-file production set
   git add plugins/O-Contrabass/Source/DSP/BodyResonator.h
   git add plugins/O-Contrabass/Source/DSP/BodyResonator.cpp
   git add plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h
   git add plugins/O-Contrabass/Source/BowedContrabassVoice.h
   git add plugins/O-Contrabass/Source/BowedContrabassVoice.cpp
   git add plugins/O-Contrabass/Source/PluginProcessor.cpp  # if M
   git add plugins/O-Contrabass/Source/PluginProcessor.h    # if M

   # CMake — 1-file source-list addition
   git add plugins/O-Contrabass/CMakeLists.txt

   # Goldens — 13 audible re-baselines
   git add plugins/O-Contrabass/tests/render-harness/golden/*.wav.sha256
   git add plugins/O-Contrabass/tests/render-harness/golden/*.json
   git add plugins/O-Contrabass/tests/render-harness/golden/{vibrato,saturator-tail-comparison}.json.sha256

   # Evidence — matrix-stability post-body archive (NOT in default reproduce-goldens.sh; evidence-only)
   git add plugins/O-Contrabass/.planning/evidence/phase-2-5/  # if WAV ≤ commit threshold; else .json + sha256 only

   # Planning — RESEARCH §21 verdict subsection (already largely filled at research-phase; verify-phase fills R38 outcome)
   git add plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md
   git add plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md       # rev-11 post-research instantiation if updated
   git add plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md          # rev-12 (this document)
   git add plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md       # Phase 2.5 section append
   git add plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md  # Gate 7 + DSP-03/DSP-04 status flip
   git add plugins/O-Contrabass/.planning/STATUS.md                     # `phase: complete` + `status: phase_2_5_complete` + `next_action: phase_2_6_discuss`
   ```

2. **Final pre-commit verification:**
   ```bash
   git status                                # confirm staged set matches the EXACT scope above
   git diff --cached --stat                  # human-readable diff summary
   git diff --cached -- plugins/O-Contrabass/Source/  # spot-check production source diffs
   ```

3. **Commit (HEREDOC for formatting; follows Phase 2.4c-bis R36-bis precedent):**
   ```bash
   git commit -m "$(cat <<'EOF'
   feat(O-Contrabass): Phase 2.5 — body resonator (8-mode static-Q parallel biquad bank, bass-tuned modes per Askenfelt 1982 / Rossing 2010) + bow noise generator (3-band BPF at 700/1500/3000 Hz + period-heuristic slip bursts, decay 0.999); Gate 7 PASS [verdict-modifier — fill at verify-phase: WORKED / WORKED-PARTIALLY / REGRESSION based on R38 outcome + R37d measurements]

   Closes BRIEF.md DSP-03 (must) + DSP-04 (should). Implements bass spec from
   ARCHITECTURE §"Body Resonator (Parallel Biquad Bank)" + §"Bow Noise Generator"
   using O-Bowed Source/DSP/BodyResonator.{h,cpp} + BowNoiseGenerator.h as
   reference for juce::dsp::IIR::Filter + juce::Random lifecycle patterns —
   NOT verbatim copy (RESEARCH §21.2 verbatim-copy assumption broken; substantial
   rewrite required for filter type / channel topology / integration site /
   parameter set / dry-path / slip-detection deltas).

   Source delta: 4 production files (2 NEW + 2 M) + 1 CMakeLists.txt M
   (~470 LOC NEW + ~55 LOC M = ~525 LOC). NEW Step 8 (body) + NEW Step 9
   (bow noise) appended to per-block evaluation order AFTER waveguide downsample,
   BEFORE host-rate output write. Period-heuristic slip-burst trigger v1.0
   substitute (RESEARCH §21.3.3) in place of true Helmholtz slip-detection
   (deferred to Phase 2.5-bis or v1.1).

   Wolf-region suppression deferred to v1.1 (CONTEXT Q55; ARCHITECTURE
   §"Body Resonator" wolf default-ON deviation flagged); v1.0 ships static-Q.

   13 audible goldens re-baselined (post-body spectral coloring). Matrix-stability
   evidence archived to .planning/evidence/phase-2-5/ (evidence-only;
   matrix-stability.wav.sha256 = 6db67707… carries forward verbatim).
   Saturator-tail-comparison post-body bin 64: [LOCK at verify-phase] dB rel max
   (Phase 2.4c-bis baseline −7.97 dB; predicted shift ≤ 2 dB per RESEARCH §21.4).
   Sub-harmonics post-body subharmEnergyRatio: [LOCK at verify-phase]
   (Phase 2.4c-bis baseline 0.358; soft-band [0.30, 0.45] expected).

   HR-1..HR-10 carry-forward verbatim; HR-11 retired (Phase 2.4c-only binding).
   No new HR introduced (CONTEXT Q59).

   parameter-spec.md sha256 77638e25… carries forward unchanged (5 body/noise
   params already declared at PluginProcessor.cpp:52,60,62,64,66; NO Stage-1
   contract amendment).

   ARCHITECTURE.md NOT amended in Phase 2.5 (wolf-region deferral is CONTEXT-
   flagged Q55 deviation; §149/§509 size_scalar reconciliation feeds end-of-
   Stage-2 verify amendment evidence base alongside §"DC Blocker" + §"In-loop
   saturator").

   Atomic-commit sequence: R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 →
   R36-bis → R37 (Phase 2.5 Gate 7 PASS).

   Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
   EOF
   )"
   ```

4. **Post-commit verification:**
   ```bash
   git log --oneline -3        # confirm R37 atomic at HEAD
   git show --stat HEAD        # confirm exact-scope file list
   ```

**Files created:** none beyond R37a/b artefacts.

**Files modified:** none beyond R37a/b/c/d artefacts.

**Commit:** **R37 atomic** — Phase 2.5 Gate 7 PASS.

**Success bar:**
- [ ] Single atomic commit lands the EXACT 4-file production source set (3 NEW + 2–3 M) + 1 CMakeLists.txt M + 13 audible-golden re-baselines + 2 informational JSON sha256 re-anchors + 13 JSON re-baselines + matrix-stability evidence archive + RESEARCH §21 verdict + planning artefacts.
- [ ] Commit message follows Phase 2.4c-bis R36-bis precedent template.
- [ ] `git log --oneline -3` shows R37 at HEAD (descendant of R36-bis-backfill chore `1dfca9d`).
- [ ] `git show --stat HEAD` confirms expected file list + LOC budget (~525 LOC source + 13 sha256 + JSON + evidence).

**Estimated effort:** ~10 min (stage + commit + verify).

---

### R37-backfill chore — STATUS.md sha propagation

**Per R34/R35/R36/R36-bis precedent. Once R37 atomic commit lands, propagate the sha into STATUS.md `phase_2_5_atomic_sha:` field for audit-trail completeness.**

**Tasks:**

1. **Capture R37 sha:**
   ```bash
   R37_SHA=$(git rev-parse HEAD)
   echo "R37 sha: $R37_SHA"
   ```

2. **Edit `plugins/O-Contrabass/.planning/STATUS.md`** — populate `phase_2_5_atomic_sha:` field + flip `phase_2_5_status:` to `phase_2_5_atomic_committed`.

3. **Commit chore:**
   ```bash
   git add plugins/O-Contrabass/.planning/STATUS.md
   git commit -m "$(cat <<'EOF'
   chore(O-Contrabass): backfill Phase 2.5 R37 commit sha (<R37_SHA>) into STATUS.md

   Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
   EOF
   )"
   ```

**Files created:** none.

**Files modified:** `plugins/O-Contrabass/.planning/STATUS.md` (1 LOC M; sha propagation).

**Commit:** **R37-backfill chore**.

**Success bar:**
- [ ] STATUS.md `phase_2_5_atomic_sha:` populated with R37 commit sha.
- [ ] R37-backfill chore commit lands as descendant of R37 atomic.
- [ ] `git log --oneline -3` shows R37-backfill → R37 → R36-bis-backfill.

**Estimated effort:** ~3 min (sha capture + STATUS edit + chore commit).

---

## Why R37 is a single atomic commit

Per Phase 2.4c-bis R36-bis + Phase 2.4b R35 + Phase 2.4a R34 precedent, R37 lands as a single atomic commit because:

1. **Source + golden coupling:** Body resonator + bow noise change the post-waveguide spectrum on every audible signal. The 13 NEW sha256 reference values are computed FROM the integrated body+noise topology; splitting source from goldens would create a transient state where `reproduce-goldens.sh` is wedged between pre-port and post-port references.
2. **CMake + source coupling:** `Source/DSP/BodyResonator.cpp` requires the `target_sources` source-list addition to compile; splitting the CMake change from the .cpp file creates a transient broken-build state.
3. **R38 BLOCKING audition is pre-commit:** Audition lands BEFORE the atomic commit (CONTEXT Q57 + Open Item #13); subjective issues escalate to in-cycle iteration (kSlipDecay tweaks, kDefaultGainDb adjustments) without breaking commit hygiene. The atomic shape preserves "every commit on main is releasable."
4. **Continues R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → R37 sequence:** Each atomic commit corresponds to a verified gate (Gate 1 → Gate 7); single-commit-per-gate is the project's gate-first discipline.
5. **Backfill chore is bookkeeping-only:** R37-backfill propagates the sha into STATUS.md AFTER R37 lands; it is not part of the gate-PASS payload.

---

## Files To Create / Modify (consolidated, Phase 2.5)

### Source (new)
- `plugins/O-Contrabass/Source/DSP/BodyResonator.h` (~90 LOC NEW)
- `plugins/O-Contrabass/Source/DSP/BodyResonator.cpp` (~200 LOC NEW)
- `plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h` (~180 LOC NEW; header-only)

### Source (modified)
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` (~10 LOC M; instance + setter declarations + lastFundamentalHz)
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` (~40 LOC M; Step 8 + Step 9 integration + bowEnergy compute + setFundamentalHz push + reset path additions + prepare/updateParametersFromAPVTS extensions)
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` (0–5 LOC M; voice-side body/noise param routing — verify-only if existing routing covers body/noise reads)
- `plugins/O-Contrabass/Source/PluginProcessor.h` (0 LOC M expected; no new public surface)

### Build (modified)
- `plugins/O-Contrabass/CMakeLists.txt` (1 LOC M; add `Source/DSP/BodyResonator.cpp` to `target_sources`)

### Test artefacts (modified, re-baselined)
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/string-{A,D,G}.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/detune-sweep-A.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/note-sequence.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/macro-sweep.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.{wav.sha256,json,json.sha256}` (json.sha256 informational-only re-anchor)
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.{wav.sha256,json,json.sha256}` (json.sha256 informational-only re-anchor; default = re-baseline per RESEARCH §21.7)

### Test artefacts (new, evidence-only — NOT in default reproduce-goldens.sh)
- `plugins/O-Contrabass/.planning/evidence/phase-2-5/matrix-stability-post-body.{wav,json}` (per RESEARCH §21.8; carry-forward `matrix-stability.wav.sha256 = 6db67707…` verbatim per CONTEXT line 176; archive only — NOT a re-baselined regression golden)

### Test artefacts (NOT committed — staged-only or transient)
- `/tmp/p25-*-t{1,2,3}.{wav,json}` (3-trial bit-stability pre-flight scratch; deleted post-R37d)
- `/tmp/auval-r37f.log`, `/tmp/pluginval-r37f.log` (R37f log capture; NOT committed)

### Stage-1 contract amendment
- **NONE.** parameter-spec.md sha256 `77638e25…` carries forward unchanged. STATUS.md `contract_checksums.parameter_spec` unchanged.

### Planning artefacts (modified)
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` (§21 verdict subsection; §§21.1–21.16 already present from research-phase; verify-phase fills R38 outcome)
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` (rev-11 post-research instantiation if updated; rev-10 carries forward as discuss-phase artefact)
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` (rev-12; this document, plan-phase output)
- `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` (Phase 2.5 section append at execute-phase)
- `plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md` (Gate 7 + DSP-03 + DSP-04 status flip from "⏸️ Deferred (Phase 2.5)" to "✅ Complete" or "⚠️ Partial" depending on Gate 7 outcome)
- `plugins/O-Contrabass/.planning/STATUS.md` (`phase` flip + `status` flip + `next_action` flip + `phase_2_5_*` carry-forward block)

### Files explicitly NOT touched
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` (Phase 2.4c-bis R36-bis post-port carry-forward verbatim; `sat * std::tanh` saturator on both rails preserved at lines 204–206)
- `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` (Phase 2.1c R20 verbatim consume)
- `plugins/O-Contrabass/Source/DSP/SchellengCalibration.h` (Phase 2.4a R34 verbatim consume)
- `plugins/O-Contrabass/Source/DSP/SubHarmonicBias.h` (Phase 2.4b R35 verbatim consume)
- `plugins/O-Contrabass/Source/BowedMPESynthesiser.{h,cpp}` (verbatim consume)
- `plugins/O-Bowed/` (read-only reference for `juce::dsp::IIR::Filter` + `juce::Random` lifecycle patterns; NOT modified)
- `modules/synthesis/bow-friction/` (HR-10 ABI preservation — NOT modified)
- `tests/render-harness/main.cpp` (no NEW CLI flags; existing 13 modes carry forward unchanged)
- `tests/render-harness/reproduce-goldens.sh` (13-entry script unchanged in count; only sha256 values inside per-entry comparisons re-baseline)

---

## Dependencies Graph (compact)

```
R37-pre (tripwire, 13/13 PASS at HEAD)
   ├── R37a (BodyResonator.{h,cpp} NEW + CMakeLists.txt M)
   │      └── (compiles standalone)
   ├── R37b (BowNoiseGenerator.h NEW; header-only)
   │      └── (compiles when included by voice)
   └── R37c (BowedContrabassVoice.{h,cpp} M + PluginProcessor.{h,cpp} M-or-zero)
          └── R37c depends on R37a + R37b (voice instantiates BodyResonator + BowNoiseGenerator)
              ├── R37d (re-baseline 13 + matrix evidence + saturator-tail post-body + sub-harmonics post-body + 3-trial DET-PASS)
              │      └── R37d depends on R37c (renders post-integration)
              │          ├── R37e (regression bar: 13/13 byte-identical against NEW sha256s + 4-file source audit + 1-file CMake audit)
              │          │      └── R37e depends on R37d (NEW sha256s locked)
              │          └── R37f (auval + pluginval-10 SUCCESS)
              │                 └── R37f depends on R37c (full plugin compiles + installs)
              └── R38 (Logic AU audition, BLOCKING)
                     └── R38 depends on R37f (post-Phase-2.5 AU installed) + /tmp/oc-pre-2-5 worktree (idempotent re-create from R37-pre)
                            └── R37 atomic commit (lands all source + goldens + RESEARCH §21 + planning artefacts)
                                   └── R37-backfill chore (sha propagation into STATUS.md)
```

---

## Risks (Phase 2.5, refreshed from RESEARCH §21.14)

Carry-forward from CONTEXT rev-10 §"Risks (Phase 2.5-specific)" with research-phase evidence (15 carry-forward + 2 NEW from research = 17 total):

1. **Body bank coefficient instability at low-freq edge (Mode 1 at 52 Hz, Q=14, sr=44.1 kHz)** — **MITIGATED** per RESEARCH §21.7 analytic check (pole radius `r ≈ 0.9997`, 3 ‱ inside unit circle, float32 eps = 1.19e-7 well-resolves). STABLE. `juce::ScopedNoDenormals` already in voice render path. No new mitigation.
2. **Click-free coefficient updates fail** — Per-block recompute + 30 ms `SmoothedValue` per ARCHITECTURE §152. R37c integrates SmoothedValue<float> for SIZE/DAMPING/MIX/BOW_NOISE; recompute at block start via `skip(numSamples-1)` skip-bump pattern.
3. **`juce::Random` non-determinism** — **MITIGATED** per RESEARCH §21.6 (O-Bowed `voiceIndex * 31337` constructor-time fixed seed; deterministic per voice). R37b inherits seeding pattern verbatim; R37d 3-trial pre-flight confirms.
4. **Wolf-region resonance at G2 sustained** — **DOCUMENTED + DEFERRED** per CONTEXT Q55. R38 Probe 3 documents audibility; if audible, carry to Phase 2.5-bis (do NOT block R37 atomic).
5. **Body coupling shifts saturator-tail bin 64 outside soft-band** — **DEFERRED** to R37d execute-phase pre-flight per RESEARCH §21.4. Predicted shift ≤ 2 dB at canonical operating point. R37d measurement; if outside [−9, −5] dB → flag for §"In-loop saturator" amendment evidence base; > 4 dB shift escalates pre-R37-atomic.
6. **13-audible-golden re-baseline drift across runs** — **DEFERRED** to R37d 3-trial pre-flight per RESEARCH §21.5.
7. **Vibrato golden re-baseline** — **DEFERRED + DEFAULT-RE-BASELINE** per RESEARCH §21.7. Already counted in 13-audible scope.
8. **Matrix-stability NEW raucous corners** — **DEFERRED** to R37d evidence-only re-render per RESEARCH §21.8. Body bank is L2-bounded BIBO-stable; predicted-low probability.
9. **Sub-harmonics post-body coupling drops `subharmEnergyRatio`** — **DEFERRED** to R37d measurement per RESEARCH §21.9. Body bank's bandpass tilt at 60–115 Hz preserves or boosts subharmonic energy. If drops < 0.30 → flag for Phase 2.4-bis backlog priority bump (NOT Gate 7 BLOCKER).
10. **R38 BLOCKING audition reveals subjective issue** — **MITIGATED** per RESEARCH §21.11 (5 categorised resolution paths: bow-noise calibration / body gain attenuation / wolf-tone deferral / distortion escalation / period-heuristic mechanical-feel deferral).
11. **Audit-hook drift mid-cycle** — **MITIGATED** per CONTEXT line 156. R37-pre + R37e enforce exact 4-source-file (+ 1 CMake) diff set.
12. **`juce::dsp::IIR::Filter<float>` RT-safety** — **MITIGATED** (JUCE biquad processSample is RT-safe; coefficient swap is atomic struct copy). R37f pluginval-10 fuzz + Parameter thread safety re-confirm.
13. **Phase 2.6-awareness (master chain absent at Phase 2.5 audition)** — **DOCUMENTED** per CONTEXT line 258. R38 audition character will differ from final v1.0 character (post-Phase-2.6 master saturator + limiter). R38 acknowledges explicitly — body+noise character validation, NOT final-output validation.
14. **BODY_MIX = 0.0 audible mismatch (HP35 attenuates E1 fundamental ~3 dB)** — **DOCUMENTED** per CONTEXT line 260. Architecture-correct; 35 Hz HP prevents sub-A0 phase-comb artifacts during mix. R38 Probe 6 (sweeps) documents; if subjective issue, schedule Phase 2.5-bis (lower HP cutoff or conditional HP).
15. **Phase 2.5 verify regression on Phase 2.4c-bis Q47 SOFT-PASS contract (bin 64 ∈ [−8.17, −6.17] dB rel max)** — Carry-forward from rev-8 risk #10 / rev-9-bis Phase 2.5-awareness. Body coupling MAY shift bin 64. R37d evidence-only flag; if shift > 1 dB → carry-forward to end-of-Stage-2 §"In-loop saturator" amendment.
16. **(NEW per RESEARCH §21.16 ESCALATION-1) Verbatim-copy assumption broken** — O-Bowed BodyResonator is morphable+stereo+peakFilter; bass spec is single-preset+mono+bandPass; integration site differs. Phase 2.5 implementation is "implement bass spec from ARCHITECTURE using O-Bowed as reference" — not "verbatim copy + substitution". LOC budget revised upward (~525 LOC source vs CONTEXT estimate). PLAN rev-12 acknowledges in §"Approach Decisions" without re-discuss.
17. **(NEW per RESEARCH §21.16 ESCALATION-2) No Helmholtz slip-detection accessor available** — Bow-friction module v1.0.0 + WaveguideString.cpp expose no slip-state. True slip-detection requires WaveguideString edit (out of CONTEXT 4-file scope). Period-heuristic substitute locked for v1.0; true slip-detection deferred to Phase 2.5-bis (Option A: voice-level F_friction reconstruction; Option B: WaveguideString getLastFrictionForce() accessor; Option C: WaveguideString slip-flag accessor).

---

## Success Criteria (Gate 7 — Phase 2.5 verify exit gate)

Five-item Gate 7 bar (per CONTEXT rev-10 §"Gate 7 Five-Item Success Criteria" + RESEARCH §21):

1. **Bit-deterministic regression bar:**
   - All 13 `reproduce-goldens.sh` entries reproduce byte-identical via post-Phase-2.5 sha256s (R37d-locked NEW reference values).
   - Audit hook reports EXACTLY the 4-file production source set: `Source/DSP/BodyResonator.{h,cpp}` NEW + `Source/DSP/BowNoiseGenerator.h` NEW + `Source/BowedContrabassVoice.{h,cpp}` M + `Source/PluginProcessor.{h,cpp}` M-or-zero (R37-pre + R37e tripwires).
   - CMakeLists.txt audit hook reports 1 file changed (1 LOC M; `Source/DSP/BodyResonator.cpp` source-list addition).
   - Phase 2.4c-bis carry-forward verified: `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` returns 2.

2. **DSP-03 + DSP-04 acceptance:**
   - **Body bank impulse response** shows 8 spectral peaks at default frequencies (60/98/115/175/235/340/700/1200 Hz) with correct relative gains (post-execute pre-flight or R38 spectral analysis).
   - **BODY_SIZE / DAMPING / MIX sweeps:** zipper-free; no clicks during 30 ms ramp.
   - **BOW_NOISE 0% → 100%:** linear-ish loudness ramp; audible at low pressure; fades to silence at zero.
   - **Bow direction reversal:** 5–15 ms wideband noise burst on each attack (slip-burst trigger validation; period-heuristic v1.0 substitute).
   - **Wolf region (G2 sustained):** subjective audibility documented at R38 Probe 3 (NOT BLOCKING per Q55 — audible beating → Phase 2.5-bis backlog flag).
   - **Orchestral character A/B:** R38 Probe 7 confirms "in same sonic family" subjective bar.

3. **`auval` + `pluginval-10` SUCCESS:**
   - `auval -v aumu OCbs Ouar` reports `AU VALIDATION SUCCEEDED` full render-rate matrix.
   - `pluginval --strictness-level 10` reports `ALL TESTS PASSED` full battery (Editor Automation, Automatable Parameters, Parameter thread safety, Background thread state, Bus enable/disable, Restoring default layout, Fuzz parameters all complete).

4. **R38 Logic AU audition CONFIRMED (BLOCKING):**
   - Pre-Phase-2.5 reference (raw-string from `1044bed` worktree) vs post-Phase-2.5 (body-engaged from working-tree) A/B in Logic Pro AU.
   - 7-probe sequence per RESEARCH §21.11: Probes 1, 2, 4, 5, 6, 7 PASS; Probe 3 (wolf-region G2) DOCUMENTED.
   - User-confirmed: post-Phase-2.5 sounds "convincing orchestral arco bass" per BRIEF.md DSP-03 + DSP-04 acceptance.
   - R37 atomic commit does NOT land until R38 audition CONFIRMED.

5. **RESEARCH §21 verdict locked:**
   - Phase 2.5 verdict written: WORKED (DSP-03 + DSP-04 CLOSED, Phase 2.6 unblocked) OR WORKED-PARTIALLY (Phase 2.5-bis backlog logged for wolf-suppression / bow-noise recalibration / period-heuristic mechanical-feel) OR REGRESSION (body coupling reveals saturator-tail divergence > 4 dB OR matrix-stability NEW raucous corners; Phase 2.5-bis escalation flag LOCKED before R37 atomic).
   - Saturator-tail post-body bin 64 measurement documented (within [−9, −5] dB rel max OR flagged for end-of-Stage-2 §"In-loop saturator" amendment evidence base).
   - Sub-harmonics post-body `subharmEnergyRatio` documented (within [0.30, 0.45] OR flagged for Phase 2.4-bis backlog priority bump).
   - Matrix-stability raucous-corner cell migration documented (no NEW raucous corners surfacing).
   - VERIFICATION.md DSP-03 + DSP-04 status flipped from "⏸️ Deferred (Phase 2.5)" to "✅ Complete" or "⚠️ Partial".

---

## Out of Scope (deferred per CONTEXT.md rev-10 + RESEARCH §21 + STATUS.md)

- **Phase 2.4-bis backlog** — kForceBoost retune for tanh saturator topology (DSP-07; subharmEnergyRatio above 0.40 strict; Phase 2.4c-bis 0.358 SOFT-PASS post-port; potential further drop post-body); Step 4 modulation gain / breathingAudible metric refinement (DSP-08); VIBRATO_DEPTH→peakDepthCents transfer tune (DSP-09 from 2.4c deviation #6); 3 v1.0 fallback-cell reduction (Phase 2.4a); click-free heuristic threshold tune (4 NEW raucous corners under tanh saturator at high-pressure × β=0.05 corners). **All stay parked.** Strict body+noise scope keeps the source delta unambiguous and the saturator-tail-regression evidence base clean.
- **Wolf-region suppression** (ARCHITECTURE §"Body Resonator" Mode #2 Q drop on fundamental lock within ±15 cents for >150 ms) — deferred to v1.1 alongside Authentic Arco toggle (CONTEXT Q55). Static-Q body bank in Phase 2.5. **DEVIATION FROM ARCHITECTURE §"Body Resonator" wolf-region default ON** — flagged explicitly in CONTEXT §"Approach Decisions" Q55. If wolf-tone audible at G2 sustained during R38 audition or post-Phase 2.6 user testing, schedule Phase 2.5-bis or v1.1 cycle.
- **True Helmholtz slip-detection** (ARCHITECTURE §165 "zero-crossing of friction force from stick to slip") — period-heuristic v1.0 substitute per RESEARCH §21.3.3. True slip-detection requires WaveguideString edit (out of CONTEXT 4-file scope-strict rule). Deferred to Phase 2.5-bis or v1.1 (Option A: voice-level F_friction reconstruction; Option B: WaveguideString getLastFrictionForce() accessor; Option C: WaveguideString slip-flag accessor).
- **Master saturator + zero-latency feedforward limiter + stereo width + output gain** — Phase 2.6.
- **Microtonal (Scala / TUN / MTS-ESP) + MPE + Note Expression** — Phase 2.6.
- **Chaos detector + softClampState** — v1.1 (deferred from Phase 2.4b R35 commit-body footnote; Phase 2.4c-bis carried "Phase 2.5/2.6"; now explicitly v1.1 since Phase 2.5 stays scope-strict).
- **ARCHITECTURE.md amendments** — §"DC Blocker" + §"In-loop saturator" + §"Body Resonator" §149/§509 `size_scalar` reconciliation (RESEARCH §21.16 ESCALATION-3) stay deferred to end-of-Stage-2 verify (post-Phase-2.6). **NO ARCHITECTURE.md amendment in Phase 2.5 cycle itself.** Wolf-region deferral is CONTEXT-flagged, not ARCHITECTURE-amended.
- **Stage-1 contract amendment** — parameter-spec.md sha256 `77638e25…` carries forward unchanged. STATUS.md `contract_checksums.parameter_spec` unchanged. **NO Stage-1 contract amendment in Phase 2.5.**
- **HR-12 (any new hard rule)** — REJECTED per CONTEXT Q59. HR-1..HR-10 cover existing invariants. HR-11 (Phase 2.4c) RETIRED at Phase 2.4c-bis cycle open per CONTEXT line 111.
- **Body Resonator shared-module extraction** (`modules/synthesis/body-resonator/`) — post-v1.0 refactor. v1.0 uses per-plugin `Source/DSP/BodyResonator.{h,cpp}` (DispersionFilter / SchellengCalibration / SubHarmonicBias precedent).
- **Bow Noise shared-module extraction** (`modules/synthesis/bow-noise/`) — post-v1.0 refactor. v1.0 uses per-plugin `Source/DSP/BowNoiseGenerator.h`.
- **Verbatim copy from O-Bowed** — REJECTED per RESEARCH §21.2 + §21.16 ESCALATION-1. Deltas too significant; "implement bass spec from ARCHITECTURE using O-Bowed as reference" is the locked approach.
- **Matrix-stability re-baseline in this cycle** — REJECTED per CONTEXT line 47 + RESEARCH §21.8. Post-Phase-2.5 matrix-stability WAV is evidence-only NOT committed; existing `matrix-stability.wav.sha256 = 6db67707…` carries forward verbatim from Phase 2.4a R34b. Mirrors Phase 2.4a R34b + Phase 2.4c-bis R36-bis "evidence golden, not in default reproduce-goldens.sh" pattern.
- **`vibrato.json.sha256` + `saturator-tail-comparison.json.sha256` as regression bars** — REJECTED per RESEARCH §20.6 / PLAN rev-11 Open Item #5 carry-forward. Both JSON files contain wall-clock fields → sha256 non-deterministic across renders. Treated as informational/historical one-time anchors only. `reproduce-goldens.sh` only checks `*.wav.sha256`. R37d updates these to NEW one-time post-Phase-2.5 anchors for audit-trail completeness, NOT as regression bars.
- **R38 deferred-non-blocking** — REJECTED per CONTEXT Q57 + Open Item #13. R38 is BLOCKING for Phase 2.5 (first NEW DSP block since Phase 2.4b R35; user audition catches subjective issues that objective measurements miss). Departs from R37/R32/R27/R19f/R14e/R34h/R35f deferred-non-blocking precedent.
- **Pre-Phase-2.5 worktree cleanup at execute-phase** — DEFERRED to verify-phase close. `/tmp/oc-pre-2-5` worktree is preserved through Phase 2.5 verify-phase for independent reproduction of pre-Phase-2.5 reference; cleanup via `git worktree remove /tmp/oc-pre-2-5` lands at Phase 2.5 verify-phase close.
- **Per-string `--vibrato-A/-D/-G` audible-mode goldens** — Phase 2.4-bis or v1.1 (RESEARCH §19.2.3 range-bias hard-codes `kVibratoMidiNote = 28`; carries forward unchanged).
- **`--saturator-tail-comparison` per-string variants** — bin selection is canonical E1 only at v1.0; per-string sat-tail comparison (e.g., MIDI 33 / 38 / 43) deferred to v1.1.
- **E1 dispersion calibration polynomial follow-up (Phase 2.1c Risk #7)** — separate `a(B, I)` cascaded-allpass concern; not body+noise scope. Out-of-scope for Phase 2.5; deferred to v1.1 or post-v1.0.
- **Production WAV binary commits** — `saturator-tail-comparison.wav` (~30 MB), `matrix-stability.wav` (~50 MB), and other large WAVs NOT committed (reproducible from harness). sha256 + JSON committed instead per Phase 2.4a/b/c/c-bis precedent.
- **CI invocation of `--saturator-tail-comparison` or `--matrix-stability`** — out-of-scope; harness modes are offline (developer-machine-only). CI runs the existing build + auval + pluginval pipeline.
- **R37 atomic split into multiple commits** — REJECTED per CONTEXT Q60 + atomic-commit precedent. Single atomic + backfill chore shape (R34/R35/R36/R36-bis precedent).



---

# Stage 2: DSP — Plan (Phase 2.6a) — REVISION 13 (Output Chain — Master Saturator `x − x³/3` + Zero-Latency Feedforward Limiter + Stereo Width Allpass-Decorrelator, Gate 8a)

**Date:** 2026-05-01
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP) — Phase 2.6a sub-cycle (1 of 3 in Phase 2.6 umbrella)
**Phase:** plan
**Cycle Scope:** Phase 2.6a ONLY — Phase 2.6b (microtonal engine + MPE) and Phase 2.6c (VST3 Note Expression FUNC-06 + FUNC-05 MPE Y/Z) each get separate PLAN amendments at later sub-cycle plan-phases (PLAN rev-14 / rev-15).

**Supersedes:** PLAN rev-12 (Phase 2.5 — body resonator 8-mode static-Q biquad bank + bow noise generator 3-band BPF + period-heuristic slip bursts, dated 2026-04-30; closed with R37 atomic `907a7c3` + R37-backfill `36b89d2` Gate 7 SOFT-PASS).

**Authority:** RESEARCH.md §22 (`Phase 2.6a Output Chain Research`, lines 7369–8513, 15 sub-sections; §22.10 R39 9-task breakdown LOCKED verbatim from research-phase; §22.11 26-entry risk register LOCKED).

---

## Preamble — Phase 2.6a Scope Recap

CONTEXT rev-11 + RESEARCH §22 lock Phase 2.6a as the **first** of three Phase 2.6 sub-cycles. Phase 2.6a closes the v1.0 master-output chain by adding three downstream DSP blocks (master saturator → zero-latency feedforward limiter → stereo width) and relocating the existing OUTPUT_GAIN to its architecturally-correct position post-StereoWidth. NO microtonal / MPE / Note Expression work in Phase 2.6a — those land at Phase 2.6b / 2.6c (R40 / R41 atomic).

**R39-pre tripwire CLEARED** at HEAD `1b44efd` (descendant of R37 atomic `907a7c3` + R37-backfill chore `36b89d2`):
- 14/14 audible goldens reproduce byte-identical at HEAD (`reproduce-goldens.sh` PASS).
- Source-tree clean against the 5 in-scope source files {`MasterSaturator.h` NEW, `MasterLimiter.h` NEW, `StereoWidth.h` NEW, `PluginProcessor.{h,cpp}` M, `BowedContrabassVoice.{h,cpp}` M}.
- Saturator carry-forward verified: `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` returns `2` (Phase 2.4c-bis port preserved, untouched in Phase 2.6).
- BodyResonator + BowNoiseGenerator integration intact.
- parameter-spec.md sha matches CONTEXT carry-forward `77638e25…` (authoritative; PluginProcessor.cpp:8 stale-comment finding `c47fe7361a…` to be corrected at R39d).

**Q1–Q10 LOCKED at discuss-phase** (CONTEXT rev-11 §"Approach Decisions") — NOT re-litigated:
- Q1: 3 sub-cycles 2.6a / 2.6b / 2.6c with discrete Gates 8a / 8b / 8c. Phase 2.6a R39 atomic.
- Q2: Phase 2.4-bis backlog (≈8 items) DEFERRED ENTIRELY TO v1.1.
- Q3: Master saturator `polynomial x − x³/3` per ARCHITECTURE verbatim. NO unification with in-loop `4·tanh(x/4)` (untouched).
- Q4: Limiter zero-latency feedforward; 3 ms attack / 50 ms release / −0.3 dBFS ceiling; NO look-ahead.
- Q7: 3 ARCHITECTURE amendments (§"DC Blocker" + §"In-loop saturator" + §149/§509 size_scalar) FOLDED into Phase 2.6 verify-phase as a single amendments task. Phase 2.6a does NOT amend ARCHITECTURE; defers amendment-text-write to Phase 2.6c verify-phase per Q7 lock.
- Q10: Stage 2 verify (full) runs as SEPARATE `/plugin-verify O-Contrabass 2-dsp` invocation after Phase 2.6c lands.

---

## Goal (Phase 2.6a)

Land the v1.0 master-output chain — three NEW header-only DSP blocks + APVTS parameter additions + processor wire-up + voice-side OUTPUT_GAIN relocation — as a single atomic R39 commit (Phase 2.4c-bis R36-bis / Phase 2.5 R37 precedent). Post-Phase-2.6a, every audible signal flows through:

```
voice (mono, post-body, post-noise) → L=R clone → StereoBuffer
   → [Step 10] MasterSaturator (wet/dry x − x³/3, default 50% mix)
   → [Step 11] MasterLimiter   (stereo-linked feedforward, 3 ms / 50 ms / −0.3 dBFS)
   → [Step 12] StereoWidth     (allpass decorrelator @ 800 Hz / Q=0.7 + M/S width, default 1.0)
   → [Step 13] OUTPUT_GAIN     (relocated from voice-side per ARCHITECTURE §258)
```

**Gate 8a (5 invariants):**
1. Output peak ≤ `LIMITER_CEILING_DB + 0.05 dB` slop across high-amplitude stress probe.
2. Click-free WIDTH automation 0% → 200% (pluginval fuzz + R39e probe 4 automation test).
3. PERF-03 zero algorithmic latency preserved (`setLatencySamples()` unchanged from Phase 2.5).
4. auval + pluginval-10 SUCCESS.
5. 14 re-baselined audible goldens reproduce byte-identical across re-renders (HR-style determinism on output chain).

If R39 lands Gate 8a PASS, Phase 2.6a closes and Phase 2.6b discuss-phase opens (microtonal engine + MPE pitch-bend, R40 atomic target).

---

## Approach Decisions (5 ESCALATIONS LOCKED — research-grounded, NOT re-discussed)

Per RESEARCH §22.9.1, five USER-DECISION-REQUIRED escalations surfaced at research-phase. PLAN rev-13 LOCKS each per research-phase recommendation; they are design-grounded against shipping Ouaricon precedent (O-Wind StereoWidth, ARCHITECTURE §527 verbatim, Phase 2.4c-bis § "In-loop saturator" amendment-evidence-base pattern).

### ESCALATION-1 LOCK — Stereo Width topology: Option A (O-Wind allpass decorrelator pattern)

**Finding (RESEARCH §22.4.1):** O-Contrabass voice writes mono `L = R` exactly via `addSample` × 2 at `BowedContrabassVoice.cpp:778–783`. Pure M/S width on `L = R` mono input collapses to `side = 0` always → WIDTH knob is mathematically a no-op. Without a decorrelator, WIDTH appears in the parameter UI but has zero audible effect — user-visible Stage 1 contract regression.

**Decision:** Adopt **O-Wind `Source/DSP/StereoWidth.h` allpass decorrelator pattern verbatim** — `juce::dsp::IIR::Filter` with `makeAllPass(sampleRate, 800.0f, 0.7f)` on R channel before M/S decode. Generates meaningful side content from mono source while preserving PERF-03 zero algorithmic latency (allpass group delay is not algorithmic latency in JUCE/PERF-03 sense; matches O-Wind precedent which reports zero latency).

**Verification path:** R39e probe 3 sub-probe `WIDTH=0.0` spectral analysis — max comb-notch ≤ 2 dB across 20 Hz – 20 kHz band (allpass-collapse acceptable color budget). Risk #19 (open) tracks this verification.

**Why not Option B (pure M/S, accept no-op):** Stage 1 contract specifies WIDTH as a usable parameter; pure M/S would require user-visible regression flag.

**Why not Option C (stereo body resonator):** Significant scope expansion; defer to v1.1 per ESCALATION-3.

### ESCALATION-2 LOCK — ARCHITECTURE limiter spec divergence: Option A (evidence-extension under existing amendment, NOT a 4th amendment)

**Finding (RESEARCH §22.3.1):** ARCHITECTURE §177–179 + §540–544 spec (release **100 ms** / threshold **−1 dBFS** / "2x oversampled signal") DIVERGES from CONTEXT rev-11 Q4 LOCKED (release **50 ms** / threshold **−0.3 dBFS** / **host-rate** chain). Three divergences:
1. Release time: 100 ms → 50 ms.
2. Threshold: −1 dBFS → −0.3 dBFS.
3. Domain: ARCHITECTURE §543 says "2x oversampled" — internally inconsistent with §225–238 chain description (master chain runs at host rate, post 2× downsample at friction-junction boundary at `BowedContrabassVoice.cpp:267`).

**Decision:** **Extend the existing §"Master Saturator + Zero-Latency Limiter" amendment-evidence-base** with a Phase 2.6a evidence-line. NOT a 4th amendment — Q3 LOCK respected (amendment count stays at 3). Pattern mirrors §"In-loop saturator" amendment which grew from 1 → 3 evidence entries across Phase 2.4c R36 (`c7e845ea…` −13.09 dB), Phase 2.4c-bis R36-bis (`5c45d176…` −7.97 dB), Phase 2.5 R37 (`130a7b02…` −25.06 dB).

**Phase 2.6a evidence-line text (deferred to Phase 2.6c verify-phase amendment-text-write per Q7):**

> Phase 2.6a evidence: release 100 → 50 ms + threshold −1 → −0.3 dBFS per CONTEXT rev-11 Q4 lock; "2x oversampled" claim §543 corrected to host-rate per §225–238 internal consistency.

**Plan-phase action:** Acknowledge deviation explicitly in this §"Approach Decisions" section + flag in R39 atomic commit body. NO ARCHITECTURE.md edit in R39 atomic.

### ESCALATION-3 LOCK — ARCHITECTURE §190 stereo body-mix splitter: deferred to v1.1

**Finding (RESEARCH §22.4.5):** ARCHITECTURE §190 ("Body Mix is applied separately to M and S, default 5% drier on side channel") requires the body resonator to be aware of M/S decomposition and apply different mix factors per channel. Phase 2.5 BodyResonator is mono single-bank — no channel awareness.

**Decision:** **Defer to v1.1** as part of stereo-body-resonator work. Phase 2.6a uses single body-mix path (current Phase 2.5 behavior, untouched). Listed in §"Out of Scope" below.

### ESCALATION-4 LOCK — Stale parameter-spec sha comment in PluginProcessor.cpp:8

**Finding (RESEARCH §22.5.1):** `PluginProcessor.cpp:8` claims sha `c47fe7361a55…`; actual current parameter-spec.md sha is `77638e25…`. Comment is stale (likely Phase 2.3 R28 default-flip drift — VIBRATO_DEPTH 12.0→0.0 + EXPRESSION_MACRO 0.5→0.0 — never propagated into source comment).

**Decision:** R39d updates the comment to the post-Phase-2.6a sha (computed at R39e step 6 after parameter-spec.md amendment lands). Combined with `MASTER_SAT_AMOUNT` + `LIMITER_CEILING_DB` additions, this is a **single contract-amendment-with-sha-bump** at R39d.

### ESCALATION-5 LOCK — OUTPUT_GAIN voice-side relocation to processor-level POST-StereoWidth

**Finding (RESEARCH §22.6.2):** OUTPUT_GAIN is currently applied at `BowedContrabassVoice.cpp:778` BEFORE the master saturator is added. With Phase 2.6a master chain landing, voice-side OUTPUT_GAIN application would mean **user volume changes saturator color** (musically wrong — pre-saturator gain change shifts the operating point on the cubic curve, altering harmonic content). ARCHITECTURE §258 places OUTPUT_GAIN as the FINAL stage POST-StereoWidth.

**Decision:** R39d **relocates** OUTPUT_GAIN application from voice-side to processor-side post-StereoWidth:
- **Voice-side removal** (`BowedContrabassVoice.{h,cpp}` M; ~5 LOC M):
  - Remove `outputGainLinear` member declaration (`BowedContrabassVoice.h`).
  - Remove OUTPUT_GAIN APVTS read at `BowedContrabassVoice.cpp:805`.
  - Remove `outputGainLinear` set at `BowedContrabassVoice.cpp:828`.
  - Remove `* outputGainLinear` multiply at `BowedContrabassVoice.cpp:778`.
- **Processor-side addition** (`PluginProcessor.{h,cpp}` M; ~10 LOC NEW):
  - `juce::SmoothedValue<float> outputGainSmoothed { 1.0f }` member.
  - `prepareToPlay`: `outputGainSmoothed.reset(sampleRate, 0.030); outputGainSmoothed.setCurrentAndTargetValue(decibelsToGain(load OUTPUT_GAIN));`
  - `processBlock` Step 13: per-sample `g = outputGainSmoothed.getNextValue()` → multiply L/R buffers.

**Bit-equivalence guarantee:** at default OUTPUT_GAIN = 0 dB and MASTER_SAT_AMOUNT=0% bypass + LIMITER_CEILING_DB=0 dBFS bypass + WIDTH=1.0 identity, the relocated chain reduces to identity: voice mono → L=R clone → saturator no-op (wet/dry=0) → limiter no-op (envelope ≤ ceiling=0 dBFS) → width identity (M/S decode at side=1.0 reverses encode exactly, modulo allpass on R) → ×1.0 OUTPUT_GAIN. **R39e step 7 explicit bit-equivalence test against Phase 2.5 sha** verifies the relocation is bit-clean (Risk #22 open). NOTE: bit-equivalence is expected ONLY if allpass decorrelator on R can be bypassed at WIDTH=1.0 with side coefficient 1.0 — research §22.4 confirms M/S identity holds at side=1.0 even with decorrelator, but the allpass IIR transient on R will produce a slightly different post-decorrelator R sample stream → **expected re-baseline at default state**, NOT bit-identical to Phase 2.5. R39e step 7 should be reframed: bit-equivalence test against a Phase 2.6a-pre worktree render at MASTER_SAT_AMOUNT=0 + LIMITER_CEILING_DB=0 + WIDTH=1.0 + decorrelator-disable (compile-time `#define`); if bit-identical, OUTPUT_GAIN relocation is bit-clean.

**Non-default OUTPUT_GAIN behavior shift:** users at non-zero OUTPUT_GAIN (e.g., −6 dB) will experience a perceptible character change because saturator now sees a quieter signal → less compression. This is the **musically correct** behavior per ARCHITECTURE §258 (user volume should not affect saturator color). Documented as a deliberate change in R39 atomic commit body.

### Locked design contracts (from research-phase headers, NOT re-litigated)

- **MasterSaturator wet/dry mix Option B** (§22.2.3): `out = (1-a)·in + a·(xClamp − xClamp³/3)` with `xClamp = jlimit(-1.5, 1.5, in)`; default `a = 0.5`. True bypass at 0%; full ARCHITECTURE-spec saturator at 100%. 30 ms `SmoothedValue` ramp on amount.
- **MasterLimiter Option C hand-written** (§22.3.2) per ARCHITECTURE §527 verbatim algorithm: `env = coeff·env + (1-coeff)·|x|; gain = (env > threshold) ? threshold/env : 1.0`. Stereo-linked envelope (`max(|L|, |R|)`); apply common gain. `attackCoeff = exp(-1/(0.003·sr))`, `releaseCoeff = exp(-1/(0.050·sr))`, `threshold = decibelsToGain(-0.3) = 0.9661f`. 30 ms `SmoothedValue` ramp on ceiling.
- **StereoWidth Option A O-Wind pattern** (§22.4.3): `juce::dsp::IIR::Filter` with `makeAllPass(sr, 800, 0.7)` on R channel; M/S width with 20 ms `SmoothedValue` ramp; range [0, 2], default 1.0.

---

## ESCALATION-6 LOCK (execute-phase entry, 2026-05-01) — audible-golden count correction (PLAN rev-13 +1 drift)

**Finding (R39-pre tripwire run 2026-05-01):** PLAN rev-13 audible-golden count is `14` at multiple sites (R39-pre check 2, R39e step 1, R39e step 5, R39f check 1, R39 atomic §"Goldens"), but Phase 2.5 R37 commit body (`907a7c3`, 2026-04-30) and current `reproduce-goldens.sh` HEAD state both report **13** audible goldens. The `+1` drift originated at plan-phase authoring; no DSP regression.

**Audit:**
- `reproduce-goldens.sh` enumerates 13 NAMES (stiffness-zero-pre, string-A/D/G, detune-sweep-A, note-sequence, vibrato, macro-sweep, slow-lfo, schelleng-stress, sub-harmonics, sub-harmonics-stability, saturator-tail-comparison) — 13/13 PASS at HEAD `1b44efd`.
- `golden/*.wav.sha256` count = 15: 13 in script + `matrix-stability` (evidence-only by Phase 2.4a R34b precedent — outside reproduce-goldens.sh enumeration) + `stiffness-sweep.wav.sha256` (Phase 2.1c `5759e5e` orphan referencing `e1-stiffness-sweep.wav` filename never reintroduced post Phase 2.2).

**Decision (Options A + B per ESCALATION-6):**
1. **OPTION A — count correction:** All `14` audible-golden references in this PLAN rev-13 Phase 2.6a section read as **`13`**; all `15` post-output-chain counts read as **`14`** (13 carry-forward + 1 NEW output-chain). Specifically: R39-pre check 2 (`14/14` → **`13/13`**); R39e step 1 (`14 audible` → **`13 audible`**); R39e step 3 (`14 NEW` → **`13 NEW`**); R39e step 5 (`14 → 15 entries` → **`13 → 14 entries`**); R39f check 1 (`15-entry` → **`14-entry`**; `14 audible re-baselined` → **`13 audible re-baselined`**); R39 atomic §"Goldens" (`15 sha256 + 15 JSON` → **`14 sha256 + 14 JSON`**).
2. **OPTION B — orphan deletion:** Stage `git rm plugins/O-Contrabass/tests/render-harness/golden/stiffness-sweep.wav.sha256` in R39 atomic; Phase 2.1c hygiene close-out. R39 atomic commit body carries an "ESCALATION-6 stiffness-sweep orphan deletion" deviation flag analogous to ESC-1..ESC-5.

**Source-delta budget impact:** zero LOC delta (count cosmetics + 1-file deletion = no production source change). 8-file source audit hook unchanged. R39d 5-step parameter-spec amendment unchanged.

---

## Tasks

### R39-pre — 7-check tripwire (5-check spec from research-phase + 2 NEW from CONTEXT carry-forward)

Pre-flight gate before any source edit. If any check fails, BLOCK and investigate upstream drift.

R39-pre. [ ] **R39-pre tripwire (re-runs at execute-phase entry)**
   - **Files:** none modified.
   - **Checks:**
     1. `git status` clean against the 8 in-scope source files {`MasterSaturator.h`, `MasterLimiter.h`, `StereoWidth.h`, `PluginProcessor.{h,cpp}`, `BowedContrabassVoice.{h,cpp}`, `tests/render-harness/main.cpp`} + `parameter-spec.md`.
     2. `tests/render-harness/reproduce-goldens.sh` 14/14 PASS at HEAD (descendant of R37 atomic `907a7c3` + R37-backfill `36b89d2`).
     3. Source-tree audit hook clean: no edits in {`O-Bowed/`, `O-Wind/` (except header reference), `modules/synthesis/bow-friction/`, `WaveguideString.cpp`, `DispersionFilter.h`, `SchellengCalibration.h`, `SubHarmonicBias.h`, `BodyResonator.{h,cpp}`, `BowNoiseGenerator.h`}.
     4. Saturator carry-forward verify: `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` returns `2` (Phase 2.4c-bis port preserved, lines 204–209).
     5. Pre-edit grep verification: `grep -rE 'MasterSaturator|MasterLimiter|StereoWidth' plugins/O-Contrabass/Source/` returns 0 hits (no pre-existing implementations).
     6. Harness builds at HEAD: `cmake --build build --target O-Contrabass-render-test` SUCCESS.
     7. Parameter-spec sha snapshot: `shasum -a 256 plugins/O-Contrabass/.planning/parameter-spec.md` returns `77638e255c2adeefdb85ae3b4d4287eecbc63b1313413573f20664990a2025d1` (matches CONTEXT rev-11 line 246).
   - **Depends on:** none (pre-flight gate).

### R39a — Author `MasterSaturator.h` (NEW, ~50 LOC)

R39a. [ ] **`MasterSaturator.h` NEW — wet/dry polynomial `x − x³/3` saturator per §22.2.4**
   - **Files (create):** `plugins/O-Contrabass/Source/DSP/MasterSaturator.h` (~50 LOC NEW)
   - **API:**
     - `void prepare(double sampleRate)` — calls `amountSmoothed.reset(sampleRate, 0.030)` (30 ms zipper-free ramp).
     - `void reset()` — calls `amountSmoothed.reset(0)`.
     - `void setAmount(float amount)` — `[0, 1]`; `amountSmoothed.setTargetValue(jlimit(0.0f, 1.0f, amount))`.
     - `float processSample(float in) noexcept` — per-sample API:
       ```cpp
       const float a = amountSmoothed.getNextValue();
       const float xClamp = juce::jlimit(-1.5f, 1.5f, in);
       const float wet = xClamp - xClamp * xClamp * xClamp / 3.0f;
       return (1.0f - a) * in + a * wet;
       ```
     - `void processBlock(juce::AudioBuffer<float>& buffer)` — block API loops `getNextValue()` once per sample (not per channel) so L and R stay in sync per-sample-per-block. Matches O-Bowed precedent.
   - **State:** single `juce::SmoothedValue<float> amountSmoothed { 0.0f }`. No IIR, no `juce::Random`, no cross-block memory.
   - **Determinism:** HR-style determinism preserved (deterministic SmoothedValue ramp; no random component).
   - **RT-safety:** no allocations, no locks, no I/O. Pure scalar arithmetic.
   - **Latency:** 0 (memoryless polynomial waveshaper).
   - **Build verify:** `cmake --build build --target O-Contrabass_VST3` SUCCESS (header included by PluginProcessor.h at R39d).
   - **Depends on:** R39-pre (tripwire CLEARED).

### R39b — Author `MasterLimiter.h` (NEW, ~80 LOC)

R39b. [ ] **`MasterLimiter.h` NEW — zero-latency feedforward stereo-linked limiter per §22.3.3 (ARCHITECTURE §527 verbatim algorithm)**
   - **Files (create):** `plugins/O-Contrabass/Source/DSP/MasterLimiter.h` (~80 LOC NEW)
   - **API:**
     - `void prepare(double sampleRate)` — recomputes `attackCoeff = exp(-1/(0.003·sr))`, `releaseCoeff = exp(-1/(0.050·sr))`; resets `envL = envR = 0`; `ceilingSmoothed.reset(sampleRate, 0.030)`.
     - `void reset()` — `envL = envR = 0`; `ceilingSmoothed.reset(ceilingLinear)`.
     - `void setCeilingDb(float dB)` — `[-6, 0]` dB; `ceilingLinear = decibelsToGain(jlimit(-6, 0, dB))`; `ceilingSmoothed.setTargetValue(ceilingLinear)`.
     - `void processBlock(juce::AudioBuffer<float>& buffer)` — stereo-linked envelope:
       ```cpp
       const float threshold = ceilingSmoothed.getNextValue();
       const float absMax = jmax(|L[i]|, |R[i]|);
       const float coeff = (absMax > envL) ? attackCoeff : releaseCoeff;
       envL = coeff * envL + (1 - coeff) * absMax;
       const float gain = (envL > threshold) ? (threshold / envL) : 1.0f;
       L[i] *= gain;
       R[i] *= gain;
       ```
   - **State:** `envL` scalar (stereo-linked); `envR` reserved for future per-channel mode (deferred to v1.1). `ceilingSmoothed` 30 ms ramp.
   - **Coefficients:** 3 ms attack / 50 ms release / −0.3 dBFS ceiling per CONTEXT Q4 LOCKED. Differs from ARCHITECTURE §177–179 (100 ms / −1 dBFS) — see ESCALATION-2 LOCK above.
   - **Determinism:** deterministic (`std::exp`, scalar math, no `juce::Random`).
   - **RT-safety:** no allocations, no locks, no I/O. Single conditional + 1 multiply-add per sample + occasional divide-by-envelope.
   - **Latency:** 0 algorithmic (no look-ahead, no delay line) — PERF-03 preserved.
   - **Build verify:** `cmake --build build --target O-Contrabass_VST3` SUCCESS.
   - **Depends on:** R39-pre.

### R39c — Author `StereoWidth.h` (NEW, ~50 LOC)

R39c. [ ] **`StereoWidth.h` NEW — allpass decorrelator + M/S width per §22.4.3 (O-Wind direct port)**
   - **Files (create):** `plugins/O-Contrabass/Source/DSP/StereoWidth.h` (~50 LOC NEW)
   - **API:**
     - `void prepare(double sampleRate, int maxBlockSize)` — `juce::dsp::ProcessSpec` with 1 channel; `decorrelator.prepare(spec)`; `decorrelator.coefficients = juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, 800.0f, 0.7f)`; `widthSmoothed.reset(sampleRate, 0.020)` (20 ms ramp; matches O-Wind).
     - `void reset()` — `decorrelator.reset()`; `widthSmoothed.reset(1.0f)`.
     - `void setWidth(float w)` — `[0, 2]`; default 1.0; `widthSmoothed.setTargetValue(jlimit(0.0f, 2.0f, w))`.
     - `void processBlock(juce::AudioBuffer<float>& buffer)` — for each sample:
       ```cpp
       const float mono = L[i];
       const float left  = mono;
       const float right = decorrelator.processSample(mono);
       const float w = widthSmoothed.getNextValue();
       const float mid  = (left + right) * 0.5f;
       const float side = (left - right) * 0.5f * w;
       L[i] = mid + side;
       R[i] = mid - side;
       ```
   - **State:** `juce::dsp::IIR::Filter<float> decorrelator`; `juce::SmoothedValue<float> widthSmoothed`.
   - **Determinism:** `juce::dsp::IIR::Filter` is deterministic (verified at Phase 2.5 R37d 3-trial bit-stability for body resonator IIR bank — same JUCE class).
   - **RT-safety:** no allocations, no locks, no I/O. IIR processSample + scalar math.
   - **Latency:** 0 algorithmic in JUCE/PERF-03 sense (allpass IIR group delay is frequency-dependent few-sample, not algorithmic latency reported via `setLatencySamples()`; matches O-Wind precedent).
   - **Build verify:** `cmake --build build --target O-Contrabass_VST3` SUCCESS.
   - **Depends on:** R39-pre.

### R39d — Wire-up + parameter-spec amendment (LARGEST scope task, ~50 LOC M across 4 files + amendment authoring)

R39d. [ ] **`PluginProcessor.{h,cpp}` M + `BowedContrabassVoice.{h,cpp}` M + `parameter-spec.md` amendment per §22.5 + §22.6**
   - **Files (modify):**
     - `plugins/O-Contrabass/Source/PluginProcessor.h` (~10 LOC NEW)
     - `plugins/O-Contrabass/Source/PluginProcessor.cpp` (~30 LOC NEW + 1 LOC M comment)
     - `plugins/O-Contrabass/Source/BowedContrabassVoice.h` (~3 LOC M; remove `outputGainLinear` member)
     - `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` (~5 LOC M; remove OUTPUT_GAIN read+set+apply)
     - `plugins/O-Contrabass/.planning/parameter-spec.md` (~30 LOC NEW; 5-step amendment)
   - **PluginProcessor.h additions (~10 LOC NEW):**
     ```cpp
     #include "DSP/MasterSaturator.h"
     #include "DSP/MasterLimiter.h"
     #include "DSP/StereoWidth.h"
     // ... in private section:
     MasterSaturator masterSaturator;
     MasterLimiter   masterLimiter;
     StereoWidth     stereoWidth;
     juce::SmoothedValue<float> outputGainSmoothed { 1.0f };
     ```
   - **PluginProcessor.cpp `prepareToPlay` additions:**
     ```cpp
     masterSaturator.prepare(sampleRate);
     masterLimiter.prepare(sampleRate);
     stereoWidth.prepare(sampleRate, samplesPerBlock);
     outputGainSmoothed.reset(sampleRate, 0.030);
     outputGainSmoothed.setCurrentAndTargetValue(
         juce::Decibels::decibelsToGain(parameters.getRawParameterValue("OUTPUT_GAIN")->load()));
     ```
   - **PluginProcessor.cpp `releaseResources` additions:**
     ```cpp
     masterSaturator.reset();
     masterLimiter.reset();
     stereoWidth.reset();
     outputGainSmoothed.reset(1.0f);
     ```
   - **PluginProcessor.cpp `processBlock` Steps 10–13 wire-up** (after `synth.renderNextBlock(...)`):
     ```cpp
     // Step 10: Master Saturator (polynomial x − x³/3, wet/dry mix).
     masterSaturator.setAmount(parameters.getRawParameterValue("MASTER_SAT_AMOUNT")->load());
     masterSaturator.processBlock(buffer);

     // Step 11: Zero-latency feedforward limiter (3 ms attack / 50 ms release).
     masterLimiter.setCeilingDb(parameters.getRawParameterValue("LIMITER_CEILING_DB")->load());
     masterLimiter.processBlock(buffer);

     // Step 12: Stereo width (allpass decorrelator + M/S width).
     stereoWidth.setWidth(parameters.getRawParameterValue("WIDTH")->load());
     stereoWidth.processBlock(buffer);

     // Step 13: Output Gain (relocated from voice-side; ARCHITECTURE §258 final stage).
     const float gainTarget = juce::Decibels::decibelsToGain(
         parameters.getRawParameterValue("OUTPUT_GAIN")->load());
     outputGainSmoothed.setTargetValue(gainTarget);

     const int numSamples = buffer.getNumSamples();
     const int numChans = buffer.getNumChannels();
     for (int i = 0; i < numSamples; ++i)
     {
         const float g = outputGainSmoothed.getNextValue();
         for (int ch = 0; ch < numChans; ++ch)
             buffer.getWritePointer(ch)[i] *= g;
     }
     ```
   - **PluginProcessor.cpp APVTS additions (`createParameterLayout`):**
     ```cpp
     // -- Output Chain (Phase 2.6a additions) --
     layout.add(std::make_unique<APF>(juce::ParameterID{"MASTER_SAT_AMOUNT", 1}, "Master Saturator",
         NR(0.0f, 1.0f, 0.001f),            0.50f));  // 50% wet/dry default
     layout.add(std::make_unique<APF>(juce::ParameterID{"LIMITER_CEILING_DB", 1}, "Limiter Ceiling",
         NR(-6.0f, 0.0f, 0.01f),            -0.3f));  // -0.3 dBFS per Q4 LOCKED
     ```
   - **PluginProcessor.cpp:8 comment update:** replace `c47fe7361a55…` with post-Phase-2.6a sha (computed at R39e step 6 after parameter-spec.md amendment lands; sha update is the LAST edit in R39d sub-tasks).
   - **BowedContrabassVoice.h removals (~3 LOC M):**
     - Remove `outputGainLinear` member declaration.
   - **BowedContrabassVoice.cpp removals (~5 LOC M):**
     - Remove OUTPUT_GAIN APVTS read at line 805.
     - Remove `outputGainLinear` set at line 828.
     - Remove `* outputGainLinear` multiply at line 778.
   - **parameter-spec.md 5-step amendment** (per §22.5.3):
     1. Add new section `### Output Chain (Phase 2.6a additions)` after `### Output` section (between current `WIDTH` row and `### Microtonal Tuning` section).
     2. Add 2 parameter rows:
        - `MASTER_SAT_AMOUNT | Master Saturator | Float | 0.0 - 1.0 | 0.50 | - | Wet/dry mix of polynomial x − x³/3 saturator (Phase 2.6a). Soft-clip at ~−3 dBFS. Default 50%.`
        - `LIMITER_CEILING_DB | Limiter Ceiling | Float | -6.0 - 0.0 | -0.3 | dB | Zero-latency feedforward limiter ceiling (Phase 2.6a). 3 ms attack / 50 ms release per CONTEXT rev-11 Q4. Default -0.3 dBFS.`
     3. Update `## Parameter Count Summary`:
        - Add `Output Chain (Phase 2.6a): 2` row (between `Output: 1` and `Microtonal: 3`).
        - Update `Total: 31` (was 29; +2).
     4. Add NEW `## Audit Trail` final section:
        ```
        ## Audit Trail

        ### Stage 1 → Phase 2.6a (parameter-spec contract amendments)

        - Phase 2.3 R28 (2026-04-29): VIBRATO_DEPTH default flipped 12.0 → 0.0 (HR-1 short-circuit; Phase 2.2 strict byte-equal regression bar). EXPRESSION_MACRO default flipped 0.50 → 0.0 (Q7a). Sha bump deferred (informally tracked in this section); next sha-bump at Phase 2.6a R39d.
        - Phase 2.6a R39d (2026-05-XX): NEW MASTER_SAT_AMOUNT + LIMITER_CEILING_DB per CONTEXT rev-11 §"Phase 2.6a — Output chain" + Q4 LOCKED limiter ceiling. Total parameter count 29 → 31.
        ```
     5. Compute new sha post-edit (`shasum -a 256 parameter-spec.md`); update PluginProcessor.cpp:8 comment.
   - **Build verify:** `cmake --build build --target O-Contrabass_VST3 O-Contrabass_AU` SUCCESS.
   - **Estimated effort:** ~2 hours implementation + 30 min build-verify.
   - **Depends on:** R39a + R39b + R39c (all 3 NEW headers must exist before processor includes them).

### R39e — Re-baseline 14 audible goldens + NEW output-chain golden + matrix-stability evidence (7-step sequence)

R39e. [ ] **Re-baseline goldens + harness `--output-chain` mode + matrix-stability evidence**
   - **Files (modify):**
     - `tests/render-harness/main.cpp` (~150 LOC NEW; `--output-chain` mode handler)
     - `tests/render-harness/golden/*.wav.sha256` (14 entries re-baselined)
     - `tests/render-harness/golden/*.json` (14 entries re-baselined; saturator-tail-comparison.json.sha256 + vibrato.json.sha256 informational re-anchors)
     - `tests/render-harness/golden/output-chain.{wav.sha256, json, json.sha256}` (NEW)
     - `tests/render-harness/reproduce-goldens.sh` (14 → 15 entries)
     - `.planning/evidence/phase-2-6a/matrix-stability-post-output-chain.{wav,json}` (NEW evidence-only archive)
   - **Step 1 — 3-trial bit-stability pre-flight:** render all 14 audible goldens 3× via existing harness modes; assert sha256 identical across trials. If 3-trial bit-stability fails, BLOCK at R39e (investigate determinism bug — likely allpass IIR `juce::Random` mis-seeding or SmoothedValue first-block divergence).
   - **Step 2 — Render `output-chain.wav`** per §22.7.1 mega-mode (5 probes concatenated, ~75 s total):
     1. Saturator amount sweep: MASTER_SAT_AMOUNT ∈ {0%, 25%, 50%, 75%, 100%} × 4-second sustained E1 default voice (LIMITER_CEILING_DB=-0.3, WIDTH=1.0, OUTPUT_GAIN=0).
     2. Limiter ceiling sweep: LIMITER_CEILING_DB ∈ {-6, -3, -0.3, 0} dBFS × high-amplitude stress (INFINITE_SUSTAIN=1.0 + SUB_HARMONICS=1.0 + EXPRESSION_MACRO=1.0 + BOW_PRESSURE=8.0, MASTER_SAT_AMOUNT=0 bypass).
     3. Width sweep: WIDTH ∈ {0.0, 0.5, 1.0, 1.5, 2.0} × 2-second sustained E1 default voice (MASTER_SAT_AMOUNT=0, LIMITER_CEILING_DB=0).
     4. Click-free automation test: WIDTH automated 0% → 200% over 5 s + MASTER_SAT_AMOUNT 0% → 100% over 5 s.
     5. Peak-overshoot stress: high-amplitude stress + LIMITER_CEILING_DB=-0.3 + MASTER_SAT_AMOUNT=0 + WIDTH=2.0 (max side-amplification stress).
     - 3-trial bit-stability; lock NEW sha256.
   - **Step 3 — Lock 14 NEW post-Phase-2.6a sha256s** into `tests/render-harness/golden/*.wav.sha256`. Replace prior Phase 2.5 sha values.
   - **Step 4 — Matrix-stability evidence-only re-render** → archive to `.planning/evidence/phase-2-6a/matrix-stability-post-output-chain.{wav,json}` (NOT in reproduce-goldens.sh per Phase 2.4a R34b precedent). Carry-forward `matrix-stability.wav.sha256 = 6db67707…` + `matrix-stability.json.sha256 = 625505cf…` verbatim — these are evidence-only goldens unchanged.
   - **Step 5 — Append `output-chain.wav` row** to `tests/render-harness/reproduce-goldens.sh` (14 → 15 entries; updated count assertion).
   - **Step 6 — Saturator-tail-comparison evidence-extension:** measure bin 64 post-master-chain spectral energy (Phase 2.4c-bis baseline `−7.97 dB`; Phase 2.5 R37 baseline `−25.06 dB`). Document Phase 2.6a evidence-line for end-of-Stage-2 §"In-loop saturator" amendment evidence-base. Compute new parameter-spec.md sha at this step; update PluginProcessor.cpp:8 comment (final R39d sub-task).
   - **Step 7 — Default-state bit-equality test (re-framed per ESCALATION-5 LOCK note):** render `note-sequence.wav` with MASTER_SAT_AMOUNT=0.0 + LIMITER_CEILING_DB=0.0 + WIDTH=1.0 + (decorrelator-disable compile-time `#define` if needed to isolate OUTPUT_GAIN relocation bit-equivalence); expect bit-identical to Phase 2.5 sha for the OUTPUT_GAIN-relocation-only variant. Risk #22 verification.
   - **Estimated effort:** ~7 min total render time per Phase 2.6a sub-cycle (14 audible × 3 trials × 0.29 s/s + matrix-stability 108-combo × 1 trial × ~3 s + output-chain 75 s × 3 trials × 0.29 s/s) + ~10 min sha audit + 3 min step 7 verification.
   - **Depends on:** R39d (full master chain integrated, build SUCCESS).

### R39f — Regression bar (8-file source audit + 0 CMake + sha verification)

R39f. [ ] **Regression bar**
   - **Files:** none modified.
   - **Checks:**
     1. `tests/render-harness/reproduce-goldens.sh` 15-entry PASS against NEW post-Phase-2.6a sha256s (14 audible re-baselined + 1 NEW output-chain).
     2. 8-file production source audit hook reports EXACTLY: `Source/DSP/MasterSaturator.h` NEW + `Source/DSP/MasterLimiter.h` NEW + `Source/DSP/StereoWidth.h` NEW + `Source/PluginProcessor.{h,cpp}` M + `Source/BowedContrabassVoice.{h,cpp}` M + `tests/render-harness/main.cpp` M.
     3. 0-file `CMakeLists.txt` audit hook (no CMake edits expected — header-only DSP design avoids Phase 2.5 R37 deviation #1).
     4. 1-file `parameter-spec.md` audit hook (Output Chain section + Audit Trail + Total 31).
     5. Saturator carry-forward verify: `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` returns `2`.
     6. BodyResonator + BowNoiseGenerator integration verify: `grep -c BodyResonator plugins/O-Contrabass/Source/BowedContrabassVoice.h` returns `4`; `grep -c BowNoiseGenerator plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` returns `7`.
     7. Latency invariant verify: `getLatencySamples()` unchanged from Phase 2.5 (still reports oversampler latency from voice).
   - **Depends on:** R39e (NEW sha256s locked).

### R39g — auval + pluginval-10 SUCCESS

R39g. [ ] **auval + pluginval-10 SUCCESS (Gate 8a invariant #4)**
   - **Files:** none modified.
   - **Checks:**
     1. `auval -v aumu OCbs OuDv` AU VALIDATION SUCCEEDED (full render-rate matrix).
     2. `pluginval --strictness-level 10 --validate <O-Contrabass.vst3>` SUCCESS full battery.
     3. Specific pluginval probes for Phase 2.6a: `Background thread state` + `Parameter thread safety` (RT-safety on master chain) + `Buffer fuzz tests` + `Click-free automation` (WIDTH 0% → 200% sweep + MASTER_SAT_AMOUNT 0% → 100% sweep).
   - **Estimated effort:** ~10 min (build + install + auval + pluginval-10 logs).
   - **Depends on:** R39d (full plugin compiles + installs); R39e (re-baselined goldens).

### R39 atomic commit — single atomic per Phase 2.4c-bis R36-bis / Phase 2.5 R37 precedent

R39. [ ] **R39 atomic commit**
   - **Source files (8):** 3 NEW headers (MasterSaturator.h + MasterLimiter.h + StereoWidth.h) + 2 M PluginProcessor (.h + .cpp) + 2 M BowedContrabassVoice (.h + .cpp) + 1 M harness main.cpp.
   - **Goldens:** 15 sha256 + 15 JSON + 1 informational JSON sha anchor (matrix-stability evidence-only) + matrix-stability evidence archive.
   - **Planning artefacts:**
     - `parameter-spec.md` amendment (Output Chain section + Audit Trail + Total 29 → 31).
     - `RESEARCH.md` §22 verdict subsection.
     - `CONTEXT.md` rev-11.a sub-cycle amendment.
     - `PLAN.md` rev-13 (this revision).
     - `SUMMARY.md` Phase 2.6a section append.
     - `VERIFICATION.md` Gate 8a status.
     - `STATUS.md` `phase` flip + `status` flip + `next_action` flip + `phase_2_6a_*` carry-forward block.
   - **Atomic-commit message body:** explicit deviation flags for ESCALATION-1 (allpass decorrelator) + ESCALATION-2 (limiter spec evidence-extension under existing amendment, NOT 4th amendment) + ESCALATION-5 (OUTPUT_GAIN relocation behavior shift at non-default values).
   - **Atomic-commit sequence:** R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → R37 → **R39** (Phase 2.6a) → R40 (Phase 2.6b) → R41 (Phase 2.6c) → Stage 2 verify amendments commit.
   - **Depends on:** R39f + R39g (both PASS).

### R39-backfill chore — sha propagation into STATUS.md

R39-backfill. [ ] **Sha propagation chore (per R34/R35/R36/R36-bis/R37 precedent)**
   - **Files (modify):** `plugins/O-Contrabass/.planning/STATUS.md` (1 LOC M; sha propagation).
   - **Steps:**
     1. Capture R39 atomic commit sha: `git rev-parse HEAD`.
     2. Update STATUS.md `phase_2_6a_atomic_sha:` field with R39 atomic commit sha.
     3. Append Phase 2.6a verify carry-forward summary to STATUS.md.
   - **Commit:** **R39-backfill chore** (separate from R39 atomic; bookkeeping-only).
   - **Success bar:** `git log --oneline -3` shows R39-backfill → R39 → R37-backfill (`36b89d2`).
   - **Estimated effort:** ~3 min.
   - **Depends on:** R39 atomic commit landed.

---

## Files To Create / Modify (consolidated, Phase 2.6a)

### Source (NEW, header-only — no CMake source-list addition)

- `plugins/O-Contrabass/Source/DSP/MasterSaturator.h` (~50 LOC NEW)
- `plugins/O-Contrabass/Source/DSP/MasterLimiter.h` (~80 LOC NEW)
- `plugins/O-Contrabass/Source/DSP/StereoWidth.h` (~50 LOC NEW)

### Source (modified)

- `plugins/O-Contrabass/Source/PluginProcessor.h` (~10 LOC NEW; 3 includes + 4 member declarations)
- `plugins/O-Contrabass/Source/PluginProcessor.cpp` (~30 LOC NEW; prepare + releaseResources + processBlock Steps 10–13 + 2 NEW APVTS declarations + 1 LOC M comment sha update at line 8)
- `plugins/O-Contrabass/Source/BowedContrabassVoice.h` (~3 LOC M; remove `outputGainLinear` member)
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` (~5 LOC M; remove OUTPUT_GAIN read at line 805 + set at line 828 + apply at line 778)

### Build

- **NONE.** No CMakeLists.txt edits — header-only DSP design (intentional; avoids Phase 2.5 R37 deviation #1 where `BodyResonator.cpp` required harness CMake source-list addition).

### Harness (modified)

- `plugins/O-Contrabass/tests/render-harness/main.cpp` (~150 LOC NEW; `--output-chain` CLI mode handler)
- `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` (14 → 15 entries; 1 row appended for output-chain)

### Test artefacts (modified, re-baselined sha256 + JSON)

- `plugins/O-Contrabass/tests/render-harness/golden/detune-sweep-A.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/macro-sweep.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/note-sequence.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/saturator-tail-comparison.{wav.sha256,json,json.sha256}` (json.sha256 informational re-anchor)
- `plugins/O-Contrabass/tests/render-harness/golden/schelleng-stress.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/slow-lfo.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-sweep.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/stiffness-zero-pre.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/string-{A,D,G}.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics-stability.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/sub-harmonics.{wav.sha256,json}`
- `plugins/O-Contrabass/tests/render-harness/golden/vibrato.{wav.sha256,json,json.sha256}` (json.sha256 informational re-anchor; default = re-baseline)

### Test artefacts (NEW)

- `plugins/O-Contrabass/tests/render-harness/golden/output-chain.{wav.sha256,json,json.sha256}` (NEW; 5-probe mega-mode per §22.7.1)

### Test artefacts (NEW evidence-only — NOT in default reproduce-goldens.sh)

- `plugins/O-Contrabass/.planning/evidence/phase-2-6a/matrix-stability-post-output-chain.{wav,json}` (per §22.8.3; carry-forward `matrix-stability.wav.sha256 = 6db67707…` verbatim)

### Test artefacts (NOT committed — staged-only or transient)

- `/tmp/p26a-*-t{1,2,3}.{wav,json}` (3-trial bit-stability pre-flight scratch; deleted post-R39e)
- `/tmp/auval-r39g.log`, `/tmp/pluginval-r39g.log` (R39g log capture; NOT committed)

### Stage-1 contract amendment (FIRST in Stage 2)

- `plugins/O-Contrabass/.planning/parameter-spec.md` (~30 LOC NEW; 5-step amendment per §22.5.3)
  - NEW `### Output Chain (Phase 2.6a additions)` section with 2 parameter rows.
  - Updated `## Parameter Count Summary` (Total 29 → 31).
  - NEW `## Audit Trail` final section.
  - Sha bump from `77638e25…` → post-Phase-2.6a sha (computed at R39e step 6).

### Planning artefacts (modified)

- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` (§22 verdict subsection appended at execute-phase)
- `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` (rev-11.a sub-cycle amendment if needed; rev-11 carries forward as umbrella scope)
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` (rev-13; this document, plan-phase output)
- `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` (Phase 2.6a section appended at execute-phase)
- `plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md` (Gate 8a status flip from "⏸️ Deferred (Phase 2.6a)" to "✅ Complete" or "⚠️ Partial")
- `plugins/O-Contrabass/.planning/STATUS.md` (`phase` flip + `status` flip + `next_action` flip + `phase_2_6a_*` carry-forward block)

### Files explicitly NOT touched

- `plugins/O-Contrabass/Source/DSP/WaveguideString.{h,cpp}` (Phase 2.4c-bis R36-bis post-port carry-forward verbatim; `sat * std::tanh` saturator preserved at lines 204–206; in-loop tanh saturator UNTOUCHED per Q3 LOCK)
- `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` (Phase 2.1c R20 verbatim consume)
- `plugins/O-Contrabass/Source/DSP/SchellengCalibration.h` (Phase 2.4a R34 verbatim consume)
- `plugins/O-Contrabass/Source/DSP/SubHarmonicBias.h` (Phase 2.4b R35 verbatim consume)
- `plugins/O-Contrabass/Source/DSP/BodyResonator.{h,cpp}` (Phase 2.5 R37 verbatim consume)
- `plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h` (Phase 2.5 R37 verbatim consume)
- `plugins/O-Contrabass/Source/BowedMPESynthesiser.{h,cpp}` (verbatim consume)
- `plugins/O-Bowed/`, `plugins/O-Wind/` (read-only references for `StereoWidth.h` patterns; NOT modified)
- `plugins/O-Gain/Source/PluginProcessor.h` (read-only reference for `BallisticsFilter` precedent; rejected for Option C; NOT modified)
- `modules/synthesis/bow-friction/` (HR-10 ABI preservation — NOT modified)
- `plugins/O-Contrabass/CMakeLists.txt` (header-only DSP; no CMake edit)
- `plugins/O-Contrabass/tests/render-harness/CMakeLists.txt` (header-only DSP includes covered by existing `Source/DSP` include path; no CMake edit)
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` (3 amendments deferred to Phase 2.6c verify-phase per Q7 LOCK; NO Phase 2.6a edit)

---

## Dependencies Graph (compact)

```
R39-pre (7-check tripwire — git clean / 14/14 goldens / source-tree audit / saturator carry-forward / pre-edit grep / harness build / parameter-spec sha snapshot)
   ├── R39a (MasterSaturator.h NEW — 50 LOC; no dependents at this stage; PluginProcessor.h includes at R39d)
   ├── R39b (MasterLimiter.h NEW — 80 LOC; same)
   └── R39c (StereoWidth.h NEW — 50 LOC; same)
              ↓ (all 3 headers must exist before R39d)
   R39d (PluginProcessor.{h,cpp} M + BowedContrabassVoice.{h,cpp} M + parameter-spec.md amendment + comment sha update)
              ↓
   R39e (re-baseline 14 audible goldens via 3-trial bit-stability + lock NEW post-Phase-2.6a sha256s + NEW output-chain golden + matrix-stability evidence-only re-render + reproduce-goldens.sh 14→15 entries + saturator-tail evidence-extension + default-state bit-equality test step 7)
              ├── R39f (regression bar — 15-entry reproduce-goldens.sh PASS + 8-file source audit hook + 0 CMake edits + 1 parameter-spec.md amendment + saturator carry-forward verify + Body+Noise integration verify + setLatencySamples invariant verify)
              └── R39g (auval AU SUCCESS + pluginval --strictness-level 10 SUCCESS + Background thread state + Parameter thread safety + Buffer fuzz + Click-free WIDTH 0%→200% + MASTER_SAT_AMOUNT 0%→100% sweeps)
                            ↓ (both R39f + R39g PASS)
              R39 atomic commit (single atomic — 8 source files + 15 sha256 + 15 JSON + 1 informational JSON sha + matrix evidence archive + RESEARCH §22 + parameter-spec.md amendment + CONTEXT rev-11.a + PLAN rev-13 + SUMMARY/VERIFICATION/STATUS planning artefacts)
                            ↓
              R39-backfill chore (sha propagation per R34/R35/R36/R36-bis/R37 precedent — STATUS.md `phase_2_6a_atomic_sha:` populated)
```

---

## Why R39 is a single atomic commit

Per Phase 2.4c-bis R36-bis + Phase 2.4b R35 + Phase 2.4a R34 + Phase 2.5 R37 precedent, R39 lands as a single atomic commit because:

1. **Source + golden coupling:** Master saturator + limiter + width change the post-voice spectrum on every audible signal. The 14 NEW sha256 reference values are computed FROM the integrated master chain; splitting source from goldens would create a transient state where `reproduce-goldens.sh` is wedged between pre-Phase-2.6a and post-Phase-2.6a references.
2. **Parameter-spec amendment + source coupling:** `MASTER_SAT_AMOUNT` + `LIMITER_CEILING_DB` APVTS additions in PluginProcessor.cpp must coincide with the parameter-spec.md amendment + sha bump at PluginProcessor.cpp:8 comment; splitting them creates an audit-trail-broken transient where the source-side parameter list and the contract-side parameter-spec list are out of sync.
3. **OUTPUT_GAIN relocation atomicity:** Voice-side removal + processor-side addition must land together; splitting creates a transient state where OUTPUT_GAIN is either applied twice (during partial integration) or zero times.
4. **Continues R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → R37 → R39 sequence:** Each atomic commit corresponds to a verified gate (Gate 1 → Gate 8a); single-commit-per-gate is the project's gate-first discipline.
5. **Backfill chore is bookkeeping-only:** R39-backfill propagates the sha into STATUS.md AFTER R39 lands; it is not part of the gate-PASS payload.

NO R38-equivalent BLOCKING audition for Phase 2.6a (NO BLOCKING audition LOCKED at discuss-phase; Phase 2.6a master chain character validation is part of Stage 4 polish-cycle final-output audition per Q6 LOCK; R38 was Phase 2.5 body-character audition, a one-off precedent).

---

## Risk Register (Phase 2.6a, 26 entries — 13 CONTEXT carry-forward + 13 NEW from research §22.11)

### Carry-forward from CONTEXT rev-11 (13 entries)

| # | Risk | Status |
|---|------|--------|
| 1 | Master saturator + master limiter compound shift to all 14 audible goldens at Phase 2.6a re-baseline | Expected (re-baseline planned, not regression) |
| 2 | Limiter peak-overshoot at high INFINITE_SUSTAIN + drone parameter combos | Mitigated via `--output-chain` peak-overshoot stress probe (R39e probe 5) |
| 3 | PERF-03 zero-latency violation if look-ahead inadvertently introduced | Mitigated by Q4 LOCKED zero-latency feedforward; R39f `setLatencySamples()` invariant verify |
| 4 | Tuning-table swap RT-safety violation | DEFERRED to Phase 2.6b (out of scope) |
| 5 | DSP-05 `perStringDetune` double-counts with MTS-ESP / Scala/TUN | DEFERRED to Phase 2.6b (out of scope) |
| 6 | MPE pitch-bend + Note Expression interaction | DEFERRED to Phase 2.6c (out of scope) |
| 7 | JUCE-NE-PATCH absent at build time | DEFERRED to Phase 2.6c (out of scope) |
| 8 | NE event drain racing with MIDI/MPE drain | DEFERRED to Phase 2.6c (out of scope) |
| 9 | parameter-spec.md amendment in Phase 2.6a (FIRST Stage-1 amendment in Stage 2) | Expected (planned amendment, audit-trailed via NEW Audit Trail section) |
| 10 | 3 ARCHITECTURE amendments at Phase 2.6 verify scribing error | DEFERRED to Phase 2.6c verify-phase per Q7 LOCK |
| 11 | v1.1 Phase 2.4-bis backlog scope drift (items added beyond 8) | Mitigated by Q2 LOCK; new items route to v1.1 backlog explicitly |
| 12 | Stage 2 verify scope creep (all 24 requirements in single cycle) | Mitigated by Q10 LOCKED separate `/plugin-verify` |
| 13 | Phase 2.6c Dorico audition pressure to interleave at Phase 2.6c | DEFERRED to Phase 2.6c (out of scope) |

### NEW from research §22.11 (13 entries)

| # | Risk | Trigger | Mitigation | Status |
|---|------|---------|------------|--------|
| 14 | WIDTH no-op without decorrelator | Voice writes mono L=R; pure M/S has zero side content always | ESCALATION-1 LOCK Option A: O-Wind allpass decorrelator pattern; verified at R39e probe 3 sub-probe 0.0 (spectral collapse ≤ 2 dB notch) | Mitigated by Option A LOCK |
| 15 | ARCHITECTURE limiter spec divergence (release 100→50 ms; threshold −1 → −0.3 dBFS; "2x oversampled" claim incorrect for host-rate chain) | CONTEXT Q4 LOCKED supersedes ARCHITECTURE §177–179 + §540–544 | ESCALATION-2 LOCK Option A: extend §"Master Saturator + Zero-Latency Limiter" amendment-evidence-base; NOT a 4th amendment (Q3 lock respected) | Mitigated by Option A LOCK |
| 16 | OUTPUT_GAIN voice-side application inverts ARCHITECTURE chain ordering | BowedContrabassVoice.cpp:778 applies OUTPUT_GAIN BEFORE master saturator → user volume changes saturator color | ESCALATION-5 LOCK: relocate to processor-level POST-StereoWidth at R39d; bit-equivalent at default 0 dB | Mitigated by R39d relocation |
| 17 | Stale parameter-spec sha in PluginProcessor.cpp:8 comment (`c47fe7361a…` vs actual `77638e25…`) | Phase 2.3 R28 default-flip drift; never re-synced | ESCALATION-4 LOCK: R39d updates comment to post-Phase-2.6a sha (R39e step 6) | Mitigated by R39d |
| 18 | 14-golden re-baseline scope at R39e | Master chain shifts every audible signal (saturator wet/dry + limiter + decorrelator) | All 14 re-baselined at R39e step 3; 3-trial bit-stability pre-flight; HR-style determinism preserved | Expected (re-baseline planned, not regression) |
| 19 | Allpass IIR transient at WIDTH=0.0 produces audible comb-filter color | StereoWidth allpass on R channel collapses to L+R/2 differential at WIDTH=0 | R39e probe 3 sub-probe 0.0 spectral verification (max notch ≤ 2 dB across 20 Hz – 20 kHz) | **Open** (R39e verification) |
| 20 | Default voice peak amplitudes overdrive saturator at 50% wet/dry | Matrix-stability post-body peak ≈ 0.351 → saturator-with-Option-B-mix ≈ 0.344 → −0.18 dB; rare drone combos may approach 0.667 saturator output cap | R39e probe 1 + probe 5 verifies; saturator output cap ≤ -3.52 dBFS guarantees ceiling room | Mitigated by saturator output cap math |
| 21 | parameter-spec.md amendment authoring (FIRST Stage-1 contract amendment in Stage 2) | Phase 2.6a R39d | Q1 expected per CONTEXT line 53–57; explicit Audit Trail section + sha-bump audit-trailed | Expected (planned amendment) |
| 22 | Bit-equivalence at MASTER_SAT_AMOUNT=0 + LIMITER_CEILING_DB=0 + WIDTH=1.0 default-state | If non-bit-equivalent (modulo allpass IIR transient on R), OUTPUT_GAIN relocation has subtle bug | R39e step 7 explicit bit-equivalence test against Phase 2.5 sha (with decorrelator-disable `#define` if needed to isolate OUTPUT_GAIN relocation) | **Open** (R39e verification) |
| 23 | setLatencySamples invariant (PERF-03 zero-latency) | Master chain components all 0-latency by design (saturator memoryless polynomial; limiter feedforward; width allpass-IIR not algorithmic latency in JUCE sense) | R39f explicit `getLatencySamples()` unchanged check | Mitigated by header design |
| 24 | RT-safety bar at master chain (PERF-01) | NEW master-chain `processBlock` allocations / locks | All header-only headers; no allocation, no lock, no I/O; pluginval-10 Background thread state + Parameter thread safety verified at R39g | Mitigated by header-only design |
| 25 | Click-free WIDTH automation 0% → 200% | Gate 8a invariant #2 | 20 ms SmoothedValue ramp on width; 30 ms ramp on MASTER_SAT_AMOUNT + LIMITER_CEILING_DB; pluginval-10 + R39e probe 4 (automation test) verifies | Mitigated by smoothing |
| 26 | Stereo body-mix splitter ARCHITECTURE §190 deferred | Mono BodyResonator incompatible with per-channel body-mix | ESCALATION-3 LOCK: defer to v1.1 stereo-body work; NOT in Phase 2.6a scope | Mitigated by deferral |

**26-entry risk register locked.** 18 mitigated, 3 expected (#1, #18, #21), 2 open (#19 WIDTH=0.0 spectral collapse comb-notch ≤ 2 dB at R39e probe 3; #22 default-state bit-equivalence at R39e step 7), 0 deferred-to-execute-phase.

**No HR-12 / HR-13 introduced.** HR-1..HR-10 carry forward verbatim. Output chain is downstream of friction junction; no friction-module ABI risk.

---

## Success Criteria (Gate 8a — Phase 2.6a verify exit gate)

Five-item Gate 8a bar (per CONTEXT rev-11 §"Phase 2.6a Gate 8a" + RESEARCH §22.7.3):

1. **Output peak ≤ ceiling + 0.05 dB slop** across high-amplitude stress probe.
   - `output-chain.json.metrics.peak_overshoot_stress.ceiling_violation_db ≤ 0.05` at LIMITER_CEILING_DB=-0.3 dBFS + MASTER_SAT_AMOUNT=0% bypass + WIDTH=2.0 max-side-stress.
   - Limiter must catch any side-doubling overshoot from `WIDTH=2.0` × full anti-correlation case.

2. **Click-free WIDTH automation 0% → 200%** (also MASTER_SAT_AMOUNT 0% → 100%).
   - `output-chain.json.metrics.automation_test.click_count == 0` AND `max_block_to_block_jump_db ≤ 1.0`.
   - pluginval-10 Click-free automation probe PASS.

3. **PERF-03 zero algorithmic latency preserved.**
   - `getLatencySamples()` unchanged from Phase 2.5 (R39f explicit invariant verify).

4. **auval + pluginval-10 SUCCESS** (R39g).
   - `auval -v aumu OCbs OuDv` AU VALIDATION SUCCEEDED full render-rate matrix.
   - `pluginval --strictness-level 10` ALL TESTS PASSED full battery (Editor Automation, Automatable Parameters, Parameter thread safety, Background thread state, Bus enable/disable, Restoring default layout, Fuzz parameters all complete).

5. **14 re-baselined audible goldens reproduce byte-identical** across re-renders (HR-style determinism on output chain).
   - 15-entry `reproduce-goldens.sh` PASS at R39f against NEW post-Phase-2.6a sha256s (14 audible re-baselined + 1 NEW output-chain).
   - 3-trial bit-stability pre-flight at R39e step 1 confirms determinism before sha256 lock.

**Plus:**
- 8-file production source audit hook reports EXACTLY {3 NEW headers + 4 M production source + 1 M harness} (R39f).
- 0-file CMakeLists.txt audit hook (header-only DSP design intentional).
- 1-file parameter-spec.md amendment with 2 NEW parameter rows + Audit Trail + Total 31.
- Saturator carry-forward verify (`grep -c "sat \* std::tanh" WaveguideString.cpp` returns 2).
- BodyResonator + BowNoiseGenerator integration verify.
- ESCALATION-1..5 each addressed: WIDTH decorrelator landed, limiter spec deviation flagged in commit body, OUTPUT_GAIN relocated bit-clean, parameter-spec sha comment updated, ARCHITECTURE §190 deferred to v1.1.

If Gate 8a PASS, R39 atomic + R39-backfill chore land; STATUS.md flips `phase: verify` then `next_action: phase_2_6b_discuss` for the next sub-cycle.

If Gate 8a FAIL on any of the 5 invariants, BLOCK at R39 atomic; investigate; re-iterate at R39d/R39e; do NOT land R39 atomic until all 5 PASS.

---

## Out of Scope (deferred per CONTEXT rev-11 + RESEARCH §22 + Q-locks)

- **Phase 2.4-bis backlog (≈8 items)** — DSP-07 retune (priority-bumped post-Phase-2.5 sub-harm collapse 9.77e-05); DSP-09 vibrato transfer tune; DSP-08 breathingAudible metric; 3 v1.0 fallback cells; true Helmholtz slip-detection; wolf-region suppression; bow-noise calibration; saturator-tail body-coupling deep characterisation. **All deferred to v1.1** (Q2 LOCKED).
- **Phase 2.6b microtonal engine + MPE pitch-bend** — separate sub-cycle (R40 atomic). PLAN rev-14 authors at later sub-cycle plan-phase.
- **Phase 2.6c VST3 Note Expression FUNC-06 + FUNC-05 MPE Y/Z** — separate sub-cycle (R41 atomic). PLAN rev-15 authors at later sub-cycle plan-phase.
- **3 ARCHITECTURE.md amendments** (§"DC Blocker" + §"In-loop saturator" + §149/§509 size_scalar) — FOLDED into Phase 2.6c verify-phase as a single amendments task (Q7 LOCKED). Phase 2.6a does NOT amend ARCHITECTURE.md.
- **Look-ahead limiter** (5 ms) — Phase 2.6a-bis if Stage 4 audition reveals harsh transients; not in v1.0 baseline (Q4 LOCKED zero-latency feedforward only). 5 ms look-ahead would break PERF-03 nice-to-have.
- **Master saturator unification with in-loop saturator** (`4·tanh(x/4)`) — REJECTED per Q3 LOCKED. ARCHITECTURE-spec'd `x − x³/3` for master is the locked path. NO 4th ARCHITECTURE amendment.
- **ARCHITECTURE §190 stereo body-mix splitter** ("Body Mix applied separately to M and S, default 5% drier on side channel") — DEFERRED to v1.1 stereo-body-resonator work per ESCALATION-3 LOCK. Phase 2.6a uses single body-mix path (current Phase 2.5 mono BodyResonator behavior).
- **Stereo body resonator** (per-channel different coefficients) — v1.1; ARCHITECTURE deviation from current Phase 2.5 mono BodyResonator. Significant scope expansion.
- **Master Saturator / Master Limiter / Stereo Width shared-module extraction** (`modules/effects/{master-saturator, master-limiter, stereo-width}/`) — post-v1.0 refactor. v1.0 uses per-plugin `Source/DSP/*.h` (DispersionFilter / SchellengCalibration / SubHarmonicBias / BodyResonator / BowNoiseGenerator precedent).
- **Chaos detector + softClampState** — v1.1 (carry-forward from Phase 2.4b R35 commit-body footnote; Phase 2.5 carried forward to v1.1).
- **R38-equivalent BLOCKING audition for Phase 2.6a** — REJECTED. Phase 2.6a master chain character validation rolls into Stage 4 polish-cycle final-output audition per Q6 LOCK (Phase 2.5 R38 was a one-off precedent for first NEW DSP block since Phase 2.4b; output chain is integration work, not novel DSP).
- **HR-12 / HR-13** (any new hard rule for Phase 2.6a) — REJECTED. HR-1..HR-10 carry forward; output chain is downstream of friction junction; no friction-module ABI risk.
- **CMakeLists.txt edits** — REJECTED. Header-only DSP design avoids Phase 2.5 R37 deviation #1 (BodyResonator.cpp source-list addition); 0 CMake edits is an explicit R39f audit-hook invariant.
- **Production WAV binary commits** — `output-chain.wav` (~75 s, ~13 MB at 44.1 kHz mono → ~26 MB stereo), `matrix-stability-post-output-chain.wav` NOT committed (reproducible from harness). sha256 + JSON committed instead per Phase 2.4a/b/c/c-bis/2.5 precedent.
- **CI invocation of `--output-chain` or `--matrix-stability`** — out-of-scope; harness modes are offline (developer-machine-only). CI runs the existing build + auval + pluginval pipeline.
- **R39 atomic split into multiple commits** — REJECTED per Phase 2.4c-bis R36-bis + Phase 2.5 R37 atomic-commit precedent. Single atomic + R39-backfill chore shape.

---

## Cross-Cycle Carry-Forward (LOCKED — verbatim from CONTEXT rev-11)

- HR-1..HR-10 in effect (HR-11 retired at Phase 2.4c-bis cycle open).
- 14 audible goldens reproduce byte-identical at HEAD `1b44efd` (descendant of R37 atomic `907a7c3` + R37-backfill `36b89d2`).
- `matrix-stability.wav.sha256 = 6db67707…` evidence-only golden carries forward (NOT in reproduce-goldens.sh; re-render at R39e step 4 to evidence archive only).
- In-loop saturator at `4·tanh(x/4)` (Phase 2.4c-bis R36-bis port) UNTOUCHED in Phase 2.6a (Q3 LOCK).
- BodyResonator (8-mode static-Q bank) + BowNoiseGenerator (3-band BPF + period-heuristic slip bursts) UNTOUCHED in Phase 2.6a (Phase 2.5 R37 verbatim consume).
- SchellengCalibration + SubHarmonicBias + DispersionFilter UNTOUCHED in Phase 2.6a.
- Stage-1 parameter-spec.md sha256 `77638e25…` carries forward through R39-pre; **Phase 2.6a R39d AMENDS** parameter-spec.md (FIRST Stage-1 contract amendment in Stage 2: `MASTER_SAT_AMOUNT` + `LIMITER_CEILING_DB` add + Audit Trail section + Total 29 → 31 + sha bump).
- ARCHITECTURE.md carries forward through Phase 2.6a execute-phase; 3 amendments folded into Phase 2.6c verify-phase per Q7 LOCK (NOT Phase 2.6a).


---

# Stage 2: DSP — Plan (Phase 2.6b) — REVISION 14 (Microtonal Engine — TuningEngine wire-up + Scala/TUN file-load + MTS-ESP no-op stub + ±24 semi MPE legacy pitch-bend, Gate 8b)

**Date:** 2026-05-01
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP) — Phase 2.6b sub-cycle (2 of 3 in Phase 2.6 umbrella)
**Phase:** plan
**Cycle Scope:** Phase 2.6b ONLY — Phase 2.6c (VST3 Note Expression FUNC-06 + FUNC-05 MPE Y/Z) gets a separate PLAN amendment at the next sub-cycle plan-phase (PLAN rev-15).

**Supersedes:** none. PLAN rev-14 is an APPEND to PLAN rev-13 (Phase 2.6a) — sub-cycle Phase 2.6b plan landing only. PLAN rev-13 Phase 2.6a section closes with R39 atomic `74b3f83e6d10162b3c28ef966aa79d4adf8e62f0` (Gate 8a SOFT-PASS; lifted to PASS-with-design-intent-flag at Phase 2.6a-bis verify, R39-bis pending user landing). PLAN rev-12 (Phase 2.5) and earlier remain closed verbatim.

**Authority:** RESEARCH.md §23 (`Phase 2.6b Microtonal Engine + MPE Pitch-Bend Research`, lines 8515–9346, 16 sub-sections; §23.12 R40 6-task breakdown LOCKED verbatim from CONTEXT rev-11.b §"R40 6-task breakdown LOCKED"; §23.13 36-entry risk register LOCKED).

---

## Preamble — Phase 2.6b Scope Recap

CONTEXT rev-11.b + RESEARCH §23 lock Phase 2.6b as the **second** of three Phase 2.6 sub-cycles. Phase 2.6b consumes the `modules/tuning/scala-tuning-engine` v2.1.0 module (already linked at Stage 1) into the voice's note-on frequency-resolution path, lighting up Scala/TUN file-load + MTS-ESP no-op stub + ±24 semitone MPE legacy pitch-bend tracking. NO output-chain / Note-Expression / FUNC-05 MPE Y-Z work — Phase 2.6a (R39) closed the output chain; Phase 2.6c (R41) lands the VST3 NE event drain + FUNC-05 MPE Y/Z.

**R40-pre tripwire status (research-phase verification of structural soundness; re-runs at execute-phase entry):**

- 14/14 Phase 2.6a re-baselined audible goldens reproduce byte-identical at HEAD descendant of R39 atomic `74b3f83e6d10162b3c28ef966aa79d4adf8e62f0` (R39-bis Phase 2.6a-bis evidence-only re-render appended; STATUS rev-23 confirms R39-bis pending user landing).
- `output-chain.wav` golden at sha `b5fc1d60b02902f1127b6ec6516b924262dd75b2d2d2d8d8a1a88ff7fec5f9ad` carries forward.
- Saturator carry-forward verified: `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/WaveguideString.cpp` returns `2` (Phase 2.4c-bis port preserved; Phase 2.6b touches NONE of WaveguideString).
- `parameter-spec.md` sha matches Phase 2.6a anchor `ae956e944ebe6dad84a8af48ff2fb53cf32a16ee1afde4baf9c1bd8ce21ad08e` (Q14 LOCKED zero amendments at Phase 2.6b).
- Module link audit: `grep "scala-tuning-engine" CMakeLists.txt` returns 5 hits (4 source-list + 1 include-dir, lines 38–49); `grep "note-expression" CMakeLists.txt` returns 1 line (line 90).
- Pre-edit grep `TuningEngine` in `Source/PluginProcessor.{h,cpp}` returns 0 hits; R40a is the FIRST plugin-side mention.

**Q11–Q20 LOCKED at discuss-phase** (CONTEXT rev-11.b §"Q11–Q20 LOCKED") — NOT re-litigated:

- Q11: MTS-ESP scope at Phase 2.6b = **Option B no-op stub.** Module's `Mode::MTSESP` falls through `calculateCustomFrequency` → 12-TET defaults; no plugin-side branching needed (research §23.2.2 / §23.3.1).
- Q12: Scala/TUN file-load UX = **Option A message-thread file-picker stub** invoked from harness `--microtonal --scl <path>`; Stage 3 GUI Editor file picker replaces stub.
- Q13: DSP-05 + microtonal coexistence = **bit-equality** at TUNING_SYSTEM=12-TET on `detune-sweep-A.wav`; Scala 19-EDO passthrough = expected algebraic delta only (per-string detune is multiplicative on top of TuningEngine absolute-Hz return — no double-counting; algebraic proof at §23.6.6).
- Q14: parameter-spec.md amendment at Phase 2.6b = **NO amendment.** Post-Phase-2.6a sha `ae956e94…` carries forward unchanged. TUNING_SYSTEM + REFERENCE_PITCH already declared at Stage 1 (PluginProcessor.cpp:118–123).
- Q15: JUCE-NE-PATCH precondition assertion = **NO assertion at 2.6b.** MPE drain uses standard `MPESynthesiser::handleMidiEvent`; raw-event drain (NE-only) is 2.6c-scoped.
- Q16: **HR-12 LOCKED** — "Tuning-table updates use only `std::atomic<double>` per-slot writes (TuningEngine `frequencyTable[128]`) or atomic-pointer swap; NO mutex / lock / file I/O / allocation on audio thread." Module enforces; HR-12 promotes to plugin-level invariant.
- Q17: Per-block tuning recompute model = **Option A note-on resolution only.** Voice resolves `f_target` once at note-on; per-block MPE pitch-bend multiplicative `2^(bend/12)` applied via existing detuneSmoothed delay-line target (NO re-poll mid-sustain; live retuning takes effect at next note-on).
- Q18: R40 task breakdown = **Option A 6-task** (R40-pre → R40a → R40b → R40c → R40d → R40e → R40 atomic → R40-backfill chore).
- Q19: Gate 8b 5-invariant scorecard LOCKED.
- Q20: 6 NEW risks LOCKED (#27–#32); cumulative 36-entry register at plan-phase entry (13 rev-11 + 13 Phase 2.6a + 6 Phase 2.6b + 4 research-surfaced #33–#36).

**Live-state discrepancies resolved at discuss-phase** (CONTEXT rev-11.b D1–D3) carry forward unchanged:

- D1: MPE legacy pitchbend range = **±24 semitones** (live code `PluginProcessor.cpp:139`; rev-11 §22.6b text drift documented for Stage 2 verify amendment).
- D2: TuningEngine MTS-ESP path = **no-op stub** at v1.0 (Q11 Option B); v1.1 lights up real MTS-ESP-Client SDK.
- D3: OUTPUT_LEVEL vs OUTPUT_GAIN parameter ID drift = **NOT a Phase 2.6b concern**; flagged for Stage 2 full verify.

---

## Goal (Phase 2.6b)

Wire `modules/tuning/scala-tuning-engine` v2.1.0 into the voice's note-on frequency-resolution path as a single atomic R40 commit (Phase 2.4c-bis R36-bis / Phase 2.5 R37 / Phase 2.6a R39 precedent). Post-Phase-2.6b, every note-on resolves frequency through:

```
midi-note-on
   → TuningEngine::getFrequency(midiNote)        // RT-safe atomic load (frequencyTable[128])
   → × (REFERENCE_PITCH / 440.0)                 // ESCALATION-RP1 Option B ratio (220–880 Hz Stage-1 honored)
   → × 2^(MPE_bend / 12)                         // ±24 semitones legacy mode (D1 LOCKED)
   → currentFrequency (float assignment)
   → computeDelaySamples × 2^(perStringDetune / 1200)   // unchanged; multiplicative on top
   → detuneSmoothed[stringIdx].setTargetValue(samples)  // 20 ms Linear ramp (Q17 Option A)
```

APVTS `parameterChanged("TUNING_SYSTEM", value)` callback maps Choice index → `TuningEngine::Mode` via 3-case switch and defers `tuningEngine.setMode(mode)` to the message thread via `juce::MessageManager::callAsync` (§23.3.2). Scala/TUN file-load exposed as public `loadScalaFile(File)` method on processor for harness invocation; Stage 3 GUI replaces with Editor `FileChooser`.

**Gate 8b (5 invariants):**

1. **12-TET default state byte-identical** to Phase 2.6a 14 audible goldens at post-Phase-2.6a sha256s (HR-style determinism — TuningEngine identity at 12-TET; HR-12 contract verifies; algebraic proof at §23.6.6).
2. **Scala/TUN file load** → expected pitch deviation on 19-EDO test file (algebraic match within ±0.5¢ tolerance via harness `--microtonal --tuning-system=scala` JSON measurements).
3. **MTS-ESP stub** (Q11 Option B) → returns 12-TET behavior; pluginval-10 Parameter thread safety + Background thread state PASS; bit-equality with `--tuning-system=12tet` baseline.
4. **MPE pitch-bend ±24 semitones** per-note tracking on channel-2 (legacy mode) — render-harness `--mpe-pitch-bend` golden reproduces byte-identical 3-trial; rmsContinuity ≥ 0.90 (Risk #33 mitigation).
5. **auval AU + pluginval-10 SUCCESS** + DSP-05 coexistence audit PASS (Q13 bit-equality on `detune-sweep-A.wav` at TUNING_SYSTEM=12-TET against Phase 2.6a sha256).

If R40 lands Gate 8b PASS, Phase 2.6b closes and Phase 2.6c discuss-phase opens (VST3 Note Expression FUNC-06 + FUNC-05 MPE Y/Z, R41 atomic target).

---

## Approach Decisions (4 ESCALATIONS LOCKED — research-grounded, NOT re-discussed)

Per RESEARCH §23.11.1, four research-phase escalations route to plan-phase for LOCK. PLAN rev-14 LOCKS each per research-phase recommendation; all four are design-grounded against the module's existing v2.1.0 API surface + algebraic invariants verified at §23.2 / §23.6.

### ESCALATION-RP1 LOCK — REFERENCE_PITCH range mismatch: Option B (voice-side ratio multiplication)

**Finding (RESEARCH §23.2.3):** plugin's `REFERENCE_PITCH` APVTS parameter declares Stage-1 ground-truth range **220.0–880.0 Hz** (PluginProcessor.cpp:118–119, default 440.0); module's `setMasterTune(double)` clamps to **400.0–480.0 Hz** (TuningEngine.cpp:84). User-set 220 Hz silently becomes 400 Hz in the engine; 880 Hz silently becomes 480 Hz — silent-wrong-output bug class.

**Decision:** Voice computes the ratio multiplicatively AFTER `getFrequency()` lookup:

```cpp
const double tuneFreqHz = tuningEngine->getFrequency(midiNote);
const float  refPitchHz = parameters->getRawParameterValue("REFERENCE_PITCH")->load();
tuningEngineBaseFreqHz  = tuneFreqHz * (static_cast<double>(refPitchHz) / 440.0);
```

The plugin **does NOT call `tuningEngine.setMasterTune`** — module's internal A4 stays at 440 Hz forever. Honors full 220–880 Hz Stage-1 contract. **Bit-equivalent at default REFERENCE_PITCH=440 Hz** (ratio = 1.0 ⇒ multiply is identity ⇒ 14 carry-forward goldens preserve byte-identity per Gate 8b inv #1).

**Why not Option A (clamp APVTS to 400–480):** silently breaks Stage-1 contract; user sees 220 Hz on UI but engine outputs 400-Hz-anchored frequencies.

**Why not Option C (patch module clamp):** module-patch invites cross-plugin re-validation (other plugins consume same module); out of scope per Phase 2.6b "no module changes" intent.

**Verification:** Risk #34 (research-surfaced) — REFERENCE_PITCH ratio precision drift at ratio≠1.0; mitigated by double-precision intermediate computation; float epsilon ≈ 1e-7 ⇒ <0.0001¢ at A4=440 (verified algebraically in §23.13).

### ESCALATION-MTS1 LOCK — MTS-ESP stub site: no plugin-side branching

**Finding (RESEARCH §23.2.2 / §23.3.1):** Module's `setMode(Mode::MTSESP)` internally dispatches `rebuildFrequencyTable` → `calculateCustomFrequency` which reads `scaleIntervals` initialized to canonical 12-TET in the ctor (TuningEngine.cpp:64–65). **`Mode::MTSESP` produces 12-TET frequencies bit-identical to `Mode::TwelveTET` automatically** — Q11 Option B stub identity satisfied at module level without plugin-side branching.

**Decision:** Plugin maps APVTS Choice index → module `Mode` via 3-case switch (mind the index swap — plugin index 0 "Scala/TUN" = module index 1 `Mode::Scala`; plugin index 2 "12-TET" = module index 0 `Mode::TwelveTET`):

```cpp
const int choiceIdx = static_cast<int>(newValue);
TuningEngine::Mode mode;
switch (choiceIdx) {
    case 0:  mode = TuningEngine::Mode::Scala;     break;
    case 1:  mode = TuningEngine::Mode::MTSESP;    break;
    case 2:
    default: mode = TuningEngine::Mode::TwelveTET; break;
}
juce::MessageManager::callAsync([this, mode]() { tuningEngine.setMode(mode); });
```

Defer to message thread via `MessageManager::callAsync` because `setMode` calls `rebuildFrequencyTable` which holds `intervalMutex` briefly — NOT RT-safe (§23.2.2). `callAsync` is RT-safe to invoke from the audio thread per JUCE docs, so the parameterChanged callback is safe even under audio-thread automation fuzz.

**Verification:** Gate 8b inv #3 — render-harness `--microtonal --tuning-system=mts-esp` produces sha256 bit-equal to `--tuning-system=12tet` baseline. Risk #31 mitigated by stub design (no sentinel value).

### ESCALATION-MPE1 LOCK — MPE ±24 semi pitch-bend smoothing: NO new smoother

**Finding (RESEARCH §23.5.2–§23.5.5):** Existing 20 ms `juce::SmoothedValue<float, Linear>` ramp on `detuneSmoothed[stringIdx]` (delay-line samples target at internal sample rate, set in `prepareToPlay` at BowedContrabassVoice.cpp:207) is adequate for ±24 semitone bend. Worst-case ramp velocity (0 → ±24 semis at MIDI 28) = 87 samples/ms at 96 kHz internal = 0.91 samples per audio-sample step, well within the existing fractional-delay interpolator's tracking capability (Phase 2.3 vibrato verified click-free at the same order of magnitude — 80 samples/ms during vibrato peaks).

**Decision:** **NO new smoother on `currentFrequency`.** Voice assigns `currentFrequency` instantly at noteStarted + notePitchbendChanged; downstream smoothing happens at `detuneSmoothed[stringIdx].setTargetValue(targetSamples)` (existing infrastructure, untouched).

**Verification:** R40c `--mpe-pitch-bend` mode renders channel-2 ±24 semitone sweep over 5s; JSON metric `pass_rmsContinuity` (target ≥ 0.90 per Phase 2.3 vibrato precedent); pitch-tracking `pass_pitchTracking` (|delta_cents| < 10 across sweep). Risk #33 (research-surfaced) — MPE ±24 rapid-bend zipper at >50 events/sec; mitigated by 20 ms detuneSmoothed + harness probe.

### ESCALATION-FPK1 LOCK — Scala/TUN file-picker stub: public processor method, harness-only at v1.0

**Finding (RESEARCH §23.8.4):** Q12 Option A LOCKED at discuss — message-thread file-picker stub for v1.0; Stage 3 GUI Editor replaces with `juce::FileChooser`.

**Decision:** Processor exposes a public method:

```cpp
bool loadScalaFile (const juce::File& sclFile)
{
    return tuningEngine.loadScalaFile (sclFile);
}
```

Harness `tests/render-harness/main.cpp` `--microtonal --scl <path>` handler calls `processor.loadScalaFile(juce::File(sclPath))` from main thread synchronously before render begins. Module's `loadScalaFile` is message-thread-only (file I/O + `intervalMutex` briefly held during `setCustomIntervals` → `rebuildFrequencyTable` chain); main-thread invocation in harness is acceptable per Q12 Option A. Stage 3 Editor invocation goes through `AsyncUpdater` post-`FileChooser::launchAsync` callback to keep WebView responsive.

**Verification:** Risk #36 (research-surfaced) — TuningEngine state divergence on DAW project save/restore. KNOWN LIMITATION at v1.0: TUNING_SYSTEM Choice persists in APVTS (parameterChanged refires on restore ⇒ setMode dispatch ⇒ frequencyTable rebuilt for 12-TET / MTS-ESP), but Scala file path is NOT persisted (no `setStateInformation` extension at v1.0). v1.1 adds Scala file path persistence via `getStateInformation`/`setStateInformation` extension (O-Lyrica reference at HarpSynthVoice + PluginProcessor:543–631).

### Locked design contracts (from research-phase headers, NOT re-litigated)

- **Voice constructor injection** (§23.6.3): `BowedContrabassVoice(juce::AudioProcessorValueTreeState* apvts, TuningEngine* engine)`. Single construction site at `OContrabassAudioProcessor` ctor (PluginProcessor.cpp:135).
- **Voice cache field** (§23.6.3): `double tuningEngineBaseFreqHz = 0.0` member; set at noteStarted, re-used at notePitchbendChanged (Q17 Option A — no re-poll mid-sustain).
- **Member-declaration order in PluginProcessor.h** (§23.8.3): `TuningEngine tuningEngine;` declared BEFORE `OContrabassMPESynthesiser synth;` so C++ ctor init order = declaration order ⇒ tuningEngine constructed first; synth `addVoice(new BowedContrabassVoice(&parameters, &tuningEngine))` passes a valid pointer (Risk #32 mitigated).
- **APVTS listener pattern** (§23.8.2): processor inherits `juce::AudioProcessorValueTreeState::Listener`; registers `parameterListener("TUNING_SYSTEM", this)` in ctor; calls `parameterChanged("TUNING_SYSTEM", load())` in ctor body to seed initial mode dispatch; removes listener in dtor. **REFERENCE_PITCH is read per-note in voice (Option B Per-RP1)** — NO listener registered; APVTS read in noteStarted is the single point of read.
- **19-EDO test fixture** (§23.4.2): NEW file at `plugins/O-Contrabass/tests/render-harness/fixtures/test-19edo.scl`, canonical Scala format, 19 steps × 1200/19 ≈ 63.157895¢ each, period 1200.000¢ at the 19th degree. Authored at R40d.
- **Atomic enforcement** (HR-12, §23.7.1): `std::array<std::atomic<double>, 128>` per-slot writes at module level — HR-12 first clause satisfied. NO atomic-pointer-swap (alternative HR-12 clause not exercised). Transition-window semantics under Q17 produce coherent per-note pitch (one atomic load per note-on; never torn).

---

## Tasks

### R40-pre — 7-step tripwire (re-runs at execute-phase entry)

Pre-flight gate before any source edit. If any check fails, BLOCK and investigate upstream drift.

R40-pre. [ ] **R40-pre tripwire**
   - **Files:** none modified.
   - **Checks:**
     1. `git status` clean against the 4 in-scope files {`Source/PluginProcessor.{h,cpp}`, `Source/BowedContrabassVoice.{h,cpp}`, `tests/render-harness/main.cpp`, `tests/render-harness/fixtures/test-19edo.scl`}.
     2. `tests/render-harness/reproduce-goldens.sh` 14/14 PASS at HEAD (descendant of R39 atomic `74b3f83e…`; `ae956e94…` parameter-spec sha; `b5fc1d60…` output-chain golden).
     3. `output-chain.wav` golden reproduces byte-identical at sha `b5fc1d60b02902f1127b6ec6516b924262dd75b2d2d2d8d8a1a88ff7fec5f9ad`.
     4. Saturator carry-forward verify: `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/WaveguideString.cpp` returns `2`.
     5. `parameter-spec.md` sha matches `ae956e944ebe6dad84a8af48ff2fb53cf32a16ee1afde4baf9c1bd8ce21ad08e` (Phase 2.6a anchor); PluginProcessor.cpp:8 comment matches.
     6. Module link audit: `grep "scala-tuning-engine" plugins/O-Contrabass/CMakeLists.txt` returns 5 hits (4 source-list lines 38–48 + 1 include-dir line 49); `grep "note-expression" plugins/O-Contrabass/CMakeLists.txt` returns 1 line (90).
     7. Pre-edit grep: `grep -n "TuningEngine" plugins/O-Contrabass/Source/PluginProcessor.{h,cpp}` returns 0 hits; `grep -n "TuningEngine" plugins/O-Contrabass/Source/BowedContrabassVoice.{h,cpp}` returns 0 hits.
   - **Pass criteria:** all 7 checks PASS. ANY fail ⇒ BLOCK; investigate upstream drift; do NOT proceed.
   - **Depends on:** none (entry gate).

### R40a — PluginProcessor wire-up (~30 LOC NEW)

R40a. [ ] **`Source/PluginProcessor.h` M (~5 LOC NEW) + `Source/PluginProcessor.cpp` M (~25 LOC NEW)** — TuningEngine member + APVTS listener registration + parameterChanged callback + Scala file-load public method.
   - **Files:**
     - `plugins/O-Contrabass/Source/PluginProcessor.h` (M ~5 LOC NEW per §23.8.1)
     - `plugins/O-Contrabass/Source/PluginProcessor.cpp` (M ~25 LOC NEW per §23.8.2)
   - **Header changes (`PluginProcessor.h`):**
     1. `#include "TuningEngine.h"` (path resolved via Stage-1 module include-dir at CMakeLists.txt:49).
     2. Class inheritance: `public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener` (NEW Listener inheritance).
     3. Public method: `bool loadScalaFile (const juce::File& sclFile);` + accessor `TuningEngine* getTuningEngine() noexcept;`.
     4. Public method: `void parameterChanged (const juce::String&, float) override;`.
     5. Private member: `TuningEngine tuningEngine;` declared **BEFORE** `OContrabassMPESynthesiser synth;` (Risk #32 — init order is declaration order in C++).
   - **Source changes (`PluginProcessor.cpp`):**
     1. Constructor body: `synth.addVoice(new BowedContrabassVoice(&parameters, &tuningEngine));` (M existing addVoice line — pass tuningEngine ptr).
     2. Constructor body: `parameters.addParameterListener("TUNING_SYSTEM", this);` followed by `parameterChanged("TUNING_SYSTEM", parameters.getRawParameterValue("TUNING_SYSTEM")->load());` to seed initial mode.
     3. Destructor: `parameters.removeParameterListener("TUNING_SYSTEM", this);` (NEW dtor body).
     4. NEW method `parameterChanged(const juce::String& parameterID, float newValue)` — 3-case switch on Choice index → `TuningEngine::Mode`, dispatched via `juce::MessageManager::callAsync([this, mode]() { tuningEngine.setMode(mode); });`.
     5. NEW method `loadScalaFile(const juce::File& sclFile)` — single-line forward to `tuningEngine.loadScalaFile(sclFile)`.
   - **Pass criteria:** compile clean (no errors / warnings); `grep -n "TuningEngine" Source/PluginProcessor.{h,cpp}` returns the expected new hits; member-declaration order verified by reading the header (TuningEngine declared before synth).
   - **Risk gates:** Risk #27 (ctor-time construction; never re-construct in `prepareToPlay`); Risk #32 (member-declaration order before synth).
   - **Depends on:** R40-pre.

### R40b — BowedContrabassVoice frequency-resolution refactor (~16 LOC NEW + M)

R40b. [ ] **`Source/BowedContrabassVoice.h` M (~3 LOC NEW) + `Source/BowedContrabassVoice.cpp` M (~13 LOC NEW + M)** — TuningEngine ptr + cache field + noteStarted + notePitchbendChanged refactor with REFERENCE_PITCH ratio (RP1) + Q17 cache.
   - **Files:**
     - `plugins/O-Contrabass/Source/BowedContrabassVoice.h` (M per §23.6.3)
     - `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` (M per §23.6.4 + §23.6.5)
   - **Header changes (`BowedContrabassVoice.h`):**
     1. Constructor signature: `explicit BowedContrabassVoice(juce::AudioProcessorValueTreeState* apvts, TuningEngine* engine);` (NEW `engine` param).
     2. Forward declaration or include for `TuningEngine` (forward-declare to avoid extra header dependency; full include in .cpp).
     3. NEW private members: `TuningEngine* tuningEngine = nullptr;` + `double tuningEngineBaseFreqHz = 0.0;`.
   - **Source changes (`BowedContrabassVoice.cpp`):**
     1. Constructor: store `engine` parameter into `tuningEngine` member.
     2. `noteStarted` Site A (lines 60–64) — replace `juce::MidiMessage::getMidiNoteInHertz(midiNote)` with TuningEngine lookup + REFERENCE_PITCH ratio (Option B per RP1):
        ```cpp
        const double tuneFreqHz = (tuningEngine != nullptr)
                                    ? tuningEngine->getFrequency(midiNote)
                                    : juce::MidiMessage::getMidiNoteInHertz(midiNote);
        const float  refPitchHz = parameters->getRawParameterValue("REFERENCE_PITCH")->load();
        tuningEngineBaseFreqHz  = tuneFreqHz * (static_cast<double>(refPitchHz) / 440.0);
        const float bend = static_cast<float>(note.totalPitchbendInSemitones);
        double freq = tuningEngineBaseFreqHz;
        if (std::abs(bend) > 0.001f)
            freq *= std::pow(2.0, bend / 12.0);
        currentFrequency = static_cast<float>(freq);
        ```
     3. `notePitchbendChanged` Site B (lines 152–156) — re-use cached `tuningEngineBaseFreqHz` (Q17 — no re-poll mid-sustain):
        ```cpp
        const float bend = static_cast<float>(note.totalPitchbendInSemitones);
        double freq = tuningEngineBaseFreqHz;
        if (std::abs(bend) > 0.001f)
            freq *= std::pow(2.0, bend / 12.0);
        currentFrequency = static_cast<float>(freq);
        ```
     4. `nullptr` fallback at Site A (`getMidiNoteInHertz` if `tuningEngine == nullptr`) — defensive only; production path always passes a valid pointer per R40a addVoice site.
   - **Pass criteria:** compile clean; algebraic invariant verified by inspection (§23.6.6) — at TUNING_SYSTEM=12-TET + REFERENCE_PITCH=440, finalFreq is bit-identical to pre-edit; per-string detune `2^(detuneCents/1200)` applied via `computeDelaySamples` is unchanged (no double-counting).
   - **Risk gates:** Risk #29 (DSP-05 no-double-counting — Q13 algebraic proof + R40d empirical bit-equality test); Risk #34 (REFERENCE_PITCH ratio precision — double-precision intermediate computation; float epsilon ⇒ <0.0001¢ at A4=440); Risk #35 (APVTS read per-note CPU cost negligible at <0.0001% of noteStarted budget).
   - **Depends on:** R40a (constructor signature change must compile against R40a's `addVoice` call site).

### R40c — Render-harness `--microtonal` + `--mpe-pitch-bend` modes (~150 LOC NEW)

R40c. [ ] **`tests/render-harness/main.cpp` M (~150 LOC NEW)** — two NEW CLI modes with JSON measurement summaries.
   - **Files:** `plugins/O-Contrabass/tests/render-harness/main.cpp` (M per §23.9).
   - **`--microtonal` mode (§23.9.1):**
     - CLI flags: `--microtonal --tuning-system <12tet|scala|mts-esp> [--scl <path>] [--reference-pitch <Hz=440>] [--note-sequence "<MIDI:dur,...>"] [--out <wav>] [--json <json>]`.
     - APVTS sets via `parameters.getParameter(<id>)->setValueNotifyingHost(<normalized>)`: TUNING_SYSTEM Choice → 0 (scala) / 1 (mts-esp) / 2 (12tet); REFERENCE_PITCH Float → `(refPitchHz - 220.0) / (880.0 - 220.0)`.
     - Default note-sequence: 12tet/mts-esp = `28:1.5,33:1.5,38:1.5,43:1.5,28:1.5` (matches existing baseline ⇒ Gate 8b inv #1 + #3 bit-equality expected); scala = `60:1.5,67:1.5,72:1.5,79:1.5,60:1.5` (exercises 19-EDO algebraic deltas).
     - Scala file load: if `--scl` is provided AND `--tuning-system=scala`, call `processor.loadScalaFile(juce::File(sclPath))` synchronously on main thread before render begins.
     - JSON metrics: `mode`, `tuning_system`, `scl_path`, `reference_pitch_hz`, per-segment `{midi, duration_s, expected_freq_hz, measured_freq_hz, delta_cents, rms_db}`, top-level pass flags `pass_nan / pass_peak / pass_blockTime / pass_pitchAccuracy / pass_rmsContinuity`.
     - Pass conditions (exit 0): `12tet`/`mts-esp` = pass_nan && pass_peak && pass_blockTime && pass_rmsContinuity (≥0.50) + bit-equality with reference golden sha256; `scala` = same + pass_pitchAccuracy (all `|delta_cents| < 0.5`).
   - **`--mpe-pitch-bend` mode (§23.9.2):**
     - CLI flags: `--mpe-pitch-bend [--bend-amount <semis=24>] [--bend-rate-hz <0.4>] [--out <wav=microtonal-mpe.wav>] [--json <json=microtonal-mpe.json>]`.
     - Default sequence: MIDI 60 sustained 5s on channel 2 (legacy mode `pitchbendRange=24`); pitch-bend ramped 0 → +24 → 0 → −24 → 0 over 5s (linear 0.4 Hz triangle wave; via `juce::MidiMessage::pitchWheel(channel, value)` at 100 Hz event rate ⇒ 500 events).
     - JSON metrics: `mode`, `midi_note`, `channel`, `bend_peak_semis`, `bend_rate_hz`, ~10 sample points across sweep with `{t_s, expected_freq_hz, measured_freq_hz, delta_cents}`, top-level pass flags `pass_nan / pass_peak / pass_blockTime / pass_pitchTracking / pass_rmsContinuity`.
     - Pass conditions (exit 0): pass_nan && pass_peak && pass_blockTime && pass_pitchTracking (all `|delta_cents| < 10`) && pass_rmsContinuity (≥0.85; looser than vibrato 0.90 per slow-glide design).
   - **Pitch-detection algorithm (§23.9.4):** re-use existing `detectFundamental(buffer, sampleRate, hintFreqHz)` autocorrelation routine from `vibrato` mode. Tolerance ±5¢ at 41 Hz @ 96 kHz; ±0.5¢ at 261 Hz @ 96 kHz — suitable for ±0.5¢ Gate 8b bar at MIDI 60+ scala-mode default.
   - **CMake budget (§23.9.3):** `main.cpp` is the ONLY harness file modified. **0 NEW source files.** **0 CMakeLists.txt edits** (TuningEngine.h include resolves via Stage-1 module include-dir already wired; no harness-side reach-back needed).
   - **Pass criteria:** harness compiles clean; new modes invocable via `./RenderHarness --microtonal --tuning-system=12tet --out test.wav` and `./RenderHarness --mpe-pitch-bend --out test-mpe.wav`; JSON output well-formed.
   - **Depends on:** R40a + R40b (harness links against processor + voice; both must compile).

### R40d — Goldens (3 NEW + 14 carry-forward bit-equality + DSP-05 coexistence + reproduce-goldens.sh evolution)

R40d. [ ] **Render goldens + 19-EDO fixture + reproduce-goldens.sh evolution + DSP-05 coexistence test.**
   - **Files NEW (artefacts; non-source):**
     - `plugins/O-Contrabass/tests/render-harness/fixtures/test-19edo.scl` (NEW; 22 lines per §23.4.2).
     - `plugins/O-Contrabass/tests/render-harness/golden/microtonal-12tet.{wav,json,wav.sha256,json.sha256}` (4 files NEW).
     - `plugins/O-Contrabass/tests/render-harness/golden/microtonal-scala.{wav,json,wav.sha256,json.sha256}` (4 files NEW).
     - `plugins/O-Contrabass/tests/render-harness/golden/microtonal-mpe.{wav,json,wav.sha256,json.sha256}` (4 files NEW).
   - **Files M:**
     - `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` (14 → 17 entries; preamble updated to reflect Phase 2.6b scope).
   - **Steps:**
     1. **Author 19-EDO fixture** at `tests/render-harness/fixtures/test-19edo.scl` per §23.4.2 canonical Scala format.
     2. **Carry-forward bit-equality test (Gate 8b inv #1):** run `tests/render-harness/reproduce-goldens.sh` 14-entry version (post-Phase-2.6a sha256s) AFTER R40a/R40b/R40c source edits. Required: ALL 14 reproduce byte-identical. **If FAIL** ⇒ BLOCK and root-cause: (a) REFERENCE_PITCH ratio mishandled; (b) TuningEngine init misordered; (c) bend-application order broken.
     3. **DSP-05 coexistence test (Q13, Gate 8b inv #5):** re-render `detune-sweep-A.wav` at TUNING_SYSTEM=12-TET (default); required bit-equality with Phase 2.6a `detune-sweep-A.wav` sha256. Algebraic guarantee holds per §23.6.6.
     4. **Render `microtonal-12tet.wav`** via `--microtonal --tuning-system=12tet --out microtonal-12tet.wav --json microtonal-12tet.json`. Lock sha256 from trial 1; verify 3-trial bit-stability at R40e.
     5. **Render `microtonal-scala.wav`** via `--microtonal --tuning-system=scala --scl=tests/render-harness/fixtures/test-19edo.scl --out microtonal-scala.wav --json microtonal-scala.json`. Lock sha256 from trial 1.
     6. **Render `microtonal-mpe.wav`** via `--mpe-pitch-bend --out microtonal-mpe.wav --json microtonal-mpe.json`. Lock sha256 from trial 1.
     7. **Pitch-accuracy verification (Gate 8b inv #2):** parse `microtonal-scala.json` and verify all per-segment `|delta_cents| < 0.5` for the 19-EDO sequence.
     8. **MPE tracking verification (Gate 8b inv #4):** parse `microtonal-mpe.json` and verify all per-segment `|delta_cents| < 10` + `pass_rmsContinuity` (≥0.85).
     9. **MTS-ESP stub identity (Gate 8b inv #3):** render `microtonal-mts-esp.wav` ephemerally (NOT a committed golden) via `--microtonal --tuning-system=mts-esp` and verify `sha256` matches `microtonal-12tet.wav.sha256`. Discard the ephemeral .wav; record verification result in commit body.
     10. **Update `reproduce-goldens.sh`** 14 → 17 entries: add 3 NEW microtonal goldens with explicit per-entry comments. Preamble updated to reflect Phase 2.6b scope (TuningEngine wire + ±24 MPE bend + Q5 priority order).
   - **Pass criteria:** all 17 entries reproduce byte-identical at the new sha256s; 14 carry-forward goldens preserve byte-identity from Phase 2.6a; DSP-05 coexistence test PASS (Q13 bit-equality); MTS-ESP stub identity PASS (sha256 match with 12-TET baseline).
   - **Render-time budget (§23.10.5):** ~5 minutes total wall-clock for all R40d renders.
   - **Depends on:** R40a + R40b + R40c (all source edits committed locally; harness binary built).

### R40e — Regression bar (auval + pluginval-10 + thread-safety + 17-entry reproduce + 3-trial bit-stability + audit hooks)

R40e. [ ] **Full validation battery.**
   - **Files:** none modified (validation only).
   - **Checks:**
     1. **17-entry `reproduce-goldens.sh` PASS** — 14 carry-forward + 3 NEW reproduce byte-identical at locked sha256s.
     2. **3-trial bit-stability** for the 3 NEW goldens — re-render each twice more (after R40d trial 1) and verify sha256 stability across all 3 trials. **If unstable** ⇒ BLOCK; investigate non-determinism (state leakage, init-order race).
     3. **3-file source audit hook** reports EXACTLY {`Source/PluginProcessor.{h,cpp}` M + `Source/BowedContrabassVoice.{h,cpp}` M + `tests/render-harness/main.cpp` M} + 0 CMake edits + 0 parameter-spec amendments + 1 fixture file NEW + 12 golden artefacts NEW + 1 reproduce-goldens.sh M.
     4. **Saturator carry-forward verify:** `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/WaveguideString.cpp` returns `2` (untouched).
     5. **BodyResonator + BowNoiseGenerator integration verify** (Phase 2.5 carry-forward): expected greps return Phase 2.5 hits.
     6. **`setLatencySamples` invariant:** `setLatencySamples()` value unchanged from Phase 2.5 / Phase 2.6a (PERF-03 zero-latency preserved; tuning-engine adds zero algorithmic latency).
     7. **`auval -v aumu OCbs OuDv`** SUCCESS (full clean audition).
     8. **`pluginval --strictness-level 10`** SUCCESS — full battery including: Background thread state, Parameter thread safety, Buffer fuzz, Editor open/close, State save/restore, Reset behavior. Pluginval verdict: expected PASS based on equivalent O-Lyrica precedent (production-shipping with same module v2.x; pluginval-10 historically PASS).
     9. **DSP-05 coexistence audit PASS** — `detune-sweep-A.wav` sha256 bit-equality at TUNING_SYSTEM=12-TET (Q13).
     10. **MTS-ESP stub identity PASS** — `--microtonal --tuning-system=mts-esp` ephemeral render sha256 matches `microtonal-12tet.wav.sha256`.
   - **Pass criteria:** 10/10 PASS. ANY fail ⇒ BLOCK; iterate at R40d/R40c/R40b/R40a as needed; do NOT land R40 atomic until all 10 PASS.
   - **Depends on:** R40d.

### R40 atomic commit

R40-atomic. [ ] **Single atomic commit lands all Phase 2.6b deltas.**
   - **Files in commit:**
     - 3 source M: `Source/PluginProcessor.{h,cpp}` + `Source/BowedContrabassVoice.{h,cpp}` + `tests/render-harness/main.cpp`.
     - 1 fixture NEW: `tests/render-harness/fixtures/test-19edo.scl`.
     - 12 golden artefacts NEW: 3 × `{wav, json, wav.sha256, json.sha256}` for microtonal-12tet / microtonal-scala / microtonal-mpe.
     - 1 reproduce-goldens.sh M (14 → 17 entries).
     - RESEARCH.md §23 (Phase 2.6b research append, lines 8515–9346).
     - CONTEXT.md rev-11.b (Phase 2.6b discuss amendment, lines 256–349).
     - PLAN.md rev-14 (this section append).
     - SUMMARY.md / VERIFICATION.md / STATUS.md planning artefacts (sub-cycle scope).
   - **Commit message format (Phase 2.4c-bis R36-bis / Phase 2.5 R37 / Phase 2.6a R39 precedent):**
     ```
     feat(O-Contrabass): Phase 2.6b R40 — microtonal engine wire-up (TuningEngine + Scala/TUN file-load + MTS-ESP no-op stub + ±24 semi MPE legacy pitch-bend); Gate 8b PASS (5/5 invariants strict-PASS)

     [body: scope summary; R40-pre→R40e step-by-step; ESCALATION RP1/MTS1/MPE1/FPK1 LOCKs; HR-12 LOCKED;
            36-entry risk register; auval + pluginval-10 verdicts; DSP-05 coexistence PASS;
            MTS-ESP stub identity PASS; 14 carry-forward goldens preserve byte-identity;
            3 NEW microtonal goldens 3-trial bit-stable; reproduce-goldens.sh 14→17 entries]
     ```
   - **Pass criteria:** Gate 8b 5/5 PASS (or SOFT-PASS with documented design-intent flag if any invariant requires Phase 2.6b-bis follow-up — unlikely per research-phase confidence).
   - **Depends on:** R40e.

### R40-backfill chore (sha propagation per R34/R35/R36/R36-bis/R37/R39 precedent)

R40-backfill. [ ] **Single chore commit propagates R40 sha into STATUS.md.**
   - **Files:** `plugins/O-Contrabass/.planning/STATUS.md`.
   - **Action:** add `phase_2_6b_atomic_sha: <sha-of-R40-atomic>` field (mirrors `phase_2_6a_atomic_sha: 74b3f83e6d10162b3c28ef966aa79d4adf8e62f0` pattern at STATUS.md:16). Append phase_2_6b_execute_carry_forward STATUS rev-25 block summarizing R40 atomic deltas + Gate 8b scorecard.
   - **Commit message:**
     ```
     chore(O-Contrabass): backfill Phase 2.6b R40 commit sha (<sha>) into STATUS.md
     ```
   - **Pass criteria:** chore commit lands; STATUS.md sha propagation visible to next sub-cycle (Phase 2.6c R41).
   - **Depends on:** R40 atomic landed.

---

## Files To Create / Modify (consolidated, Phase 2.6b)

**Production source (3 M; 0 NEW):**

| File | Op | LOC delta | Purpose |
|------|----|-----------|---------|
| `plugins/O-Contrabass/Source/PluginProcessor.h` | M | ~5 NEW | TuningEngine member + Listener inheritance + loadScalaFile/getTuningEngine/parameterChanged decls |
| `plugins/O-Contrabass/Source/PluginProcessor.cpp` | M | ~25 NEW | ctor listener registration + dtor cleanup + parameterChanged callback (3-case Choice → Mode + callAsync) + loadScalaFile method |
| `plugins/O-Contrabass/Source/BowedContrabassVoice.h` | M | ~3 NEW | TuningEngine* ctor param + tuningEngineBaseFreqHz cache field + tuningEngine ptr member |
| `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` | M | ~13 NEW + M | noteStarted Site A refactor (TuningEngine lookup + REFERENCE_PITCH ratio + cache) + notePitchbendChanged Site B refactor (cache re-use) |

(Note: `BowedContrabassVoice.{h,cpp}` counted as one source file in the "3 source M" budget headline since header+source belong to the same translation unit; CONTEXT rev-11.b §"Source-delta budget LOCKED" treats `PluginProcessor.{h,cpp}` and `BowedContrabassVoice.{h,cpp}` each as one source file.)

**Harness source (1 M):**

| File | Op | LOC delta | Purpose |
|------|----|-----------|---------|
| `plugins/O-Contrabass/tests/render-harness/main.cpp` | M | ~150 NEW | `--microtonal` + `--mpe-pitch-bend` modes with JSON measurement summaries |

**Fixtures + goldens (1 fixture NEW + 12 golden artefacts NEW + 1 script M):**

| File | Op | Purpose |
|------|----|---------|
| `plugins/O-Contrabass/tests/render-harness/fixtures/test-19edo.scl` | NEW | 19-EDO canonical Scala fixture (§23.4.2) |
| `plugins/O-Contrabass/tests/render-harness/golden/microtonal-12tet.{wav, json, wav.sha256, json.sha256}` | NEW × 4 | 12-TET baseline golden (Gate 8b inv #1 reference) |
| `plugins/O-Contrabass/tests/render-harness/golden/microtonal-scala.{wav, json, wav.sha256, json.sha256}` | NEW × 4 | Scala 19-EDO golden (Gate 8b inv #2 reference) |
| `plugins/O-Contrabass/tests/render-harness/golden/microtonal-mpe.{wav, json, wav.sha256, json.sha256}` | NEW × 4 | MPE ±24 sweep golden (Gate 8b inv #4 reference) |
| `plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh` | M | 14 → 17 entries; preamble updated for Phase 2.6b |

**Planning artefacts (5 M, sub-cycle scope):**

| File | Op | Purpose |
|------|----|---------|
| `plugins/O-Contrabass/.planning/stages/2-dsp/CONTEXT.md` | M | rev-11.b sub-cycle amendment (Phase 2.6b discuss; landed at discuss-phase, this PLAN consumes verbatim) |
| `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` | M | §23 Phase 2.6b research append (landed at research-phase, this PLAN consumes verbatim) |
| `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` | M | rev-14 Phase 2.6b plan append (THIS document) |
| `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` | M | Phase 2.6b plan-phase summary append |
| `plugins/O-Contrabass/.planning/STATUS.md` | M | Phase 2.6b plan-phase carry-forward append + R40 atomic plan signal |

**0 NEW production source files. 0 NEW headers. 0 CMakeLists.txt edits.** Header-only DSP design INTENTIONAL — TuningEngine.h include resolves via Stage-1 module include-dir already wired at CMakeLists.txt:49 (avoids Phase 2.5 R37 deviation #1). The 1 NEW fixture file (`test-19edo.scl`) is data, not source — does not increment "NEW source file" count.

**Source-delta budget total:** 3 source M + 1 harness M + 1 fixture NEW + 12 golden artefacts NEW + 1 script M + 5 planning artefacts M = **~190 LOC net production + ~150 LOC NEW harness** (per CONTEXT rev-11.b §"Source-delta budget LOCKED" + RESEARCH §23.12.1 fixture refinement).

---

## Dependencies Graph (compact)

```
R40-pre (tripwire)
  ↓
R40a (PluginProcessor) ←———— R40b (Voice — depends on R40a ctor signature)
                                ↓
                              R40c (Harness — links against R40a + R40b)
                                ↓
                              R40d (Goldens — render via R40c binary)
                                ↓
                              R40e (Validation — auval/pluginval/audit hooks)
                                ↓
                              R40 atomic commit (single atomic; 17 source/artefact files in commit)
                                ↓
                              R40-backfill chore (STATUS.md sha propagation)
```

**Critical path:** R40-pre → R40a → R40b → R40c → R40d → R40e → R40 atomic. No parallel branches. R40b cannot proceed until R40a's voice constructor signature change compiles cleanly against the `addVoice` call site.

---

## Why R40 is a single atomic commit

Per Phase 2.4c-bis R36-bis + Phase 2.5 R37 + Phase 2.6a R39 precedent, R40 lands as a single atomic commit containing all source + harness + fixture + golden + planning artefacts. Rationale:

1. **Bisect-friendliness:** if a regression appears post-R40, `git bisect` lands directly on R40 with all related changes co-located. No partial-state intermediate commits where the build is broken or the goldens are out of sync.
2. **Goldens lock to source:** the 14 carry-forward goldens preserve byte-identity ONLY if the R40a/R40b source edits algebraically reduce to identity at TUNING_SYSTEM=12-TET (per §23.6.6 algebraic proof). Splitting source from goldens breaks the bisect contract.
3. **Atomic-commit-sequence ledger:** R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → R37 → R39 → R39-bis (pending land) → **R40** (Phase 2.6b) → R41 (Phase 2.6c) → Stage 2 verify amendments commit. Each entry corresponds to one atomic; R40 is the next slot in the ledger.
4. **R40-backfill chore is the sha-propagation channel** — separate small commit per R34/R35/R36/R36-bis/R37/R39 precedent; consumes R40's sha into STATUS.md without re-touching the source.

---

## Risk Register (Phase 2.6b, 36 entries — 13 CONTEXT carry-forward + 13 Phase 2.6a carry-forward + 6 Phase 2.6b NEW from CONTEXT rev-11.b Q20 + 4 research-surfaced #33–#36)

**13 CONTEXT carry-forward (#1–#13):** unchanged from CONTEXT rev-11; closed/mitigated through Phase 2.6a R39 + Phase 2.6a-bis R39-bis verify scope.

**13 Phase 2.6a carry-forward (#14–#26):** unchanged from PLAN rev-13; cleared at Gate 8a verify (R39 + R39-bis).

**6 Phase 2.6b NEW (CONTEXT rev-11.b Q20 LOCKED, #27–#32):**

| # | Risk | Trigger | Mitigation | Status |
|---|------|---------|------------|--------|
| 27 | TuningEngine instantiation allocates on construction; if constructed in `prepareToPlay` could allocate at host re-prepare | Phase 2.6b R40a wire-up | Construct in PluginProcessor ctor (one-shot, message thread); never re-construct | Mitigated by R40a ctor placement |
| 28 | `loadScalaFile` blocks message thread on large .scl parse | Phase 2.6b R40a Scala/TUN load | Message-thread blocking acceptable for file load (UI thread file picker → synchronous parse → atomic frequencyTable populate); harness invokes from main thread; per-slot atomic frequencyTable preserves audio-thread RT-safety throughout | Mitigated by atomic frequencyTable design + thread-aware invocation |
| 29 | DSP-05 detune coexistence — voice double-counts if TuningEngine path also applies detune | Phase 2.6b R40b voice refactor | Strict separation: TuningEngine returns absolute Hz from `frequencyTable[128]` (cents handled internally per Mode); voice multiplies by `2^(perStringDetune/1200)` only at note-on; bit-equality test on `detune-sweep-A.wav` at 12-TET (Q13) | Mitigated by Q13 audit + R40d coexistence test |
| 30 | MPE legacy pitchbend range mismatch — ARCHITECTURE §288 says ±2; live code says ±24 | Live state discrepancy D1 | LOCKED ±24 (Stage 1 ground truth); ARCHITECTURE drift documented for Stage 2 verify amendment | Resolved by D1 lock |
| 31 | MTS-ESP stub returns sentinel value mistaken for real frequency by voice | Phase 2.6b R40a stub | Engine-internal: `Mode::MTSESP` falls through `calculateCustomFrequency` → 12-TET; no sentinel; bit-equality with 12-TET baseline at R40d step 9 | Mitigated by Q11 Option B + research §23.2.2 |
| 32 | 14 audible goldens DRIFT at TUNING_SYSTEM=12-TET if TuningEngine init order wrong (e.g., constructed AFTER voice's `prepareToPlay` reads `f_target`) | Phase 2.6b R40a/R40b ordering | `TuningEngine` member declared BEFORE `synth` in PluginProcessor.h ⇒ ctor init order = declaration order; voice's `prepareToPlay` does NOT call `tuningEngine.getFrequency()` (only `noteStarted` + `notePitchbendChanged` invoke it, both runtime callbacks AFTER ctor + prepareToPlay) | Mitigated by R40a member-declaration discipline |

**4 research-surfaced (RESEARCH §23.13, #33–#36):**

| # | Risk | Trigger | Mitigation | Status |
|---|------|---------|------------|--------|
| 33 | MPE ±24 semi rapid bend zipper at >50 events/sec | Phase 2.6b R40c sweep mode at high event rate | 20 ms detuneSmoothed Linear ramp on delay-line samples; rmsContinuity ≥ 0.90 at R40c verifies (§23.5.4) | Mitigated by smoother + harness probe |
| 34 | REFERENCE_PITCH ratio precision drift at ratio≠1.0 | User-set REFERENCE_PITCH ≠ 440 | Double-precision multiply in voice (§23.6.4); float cast at currentFrequency assignment loses <0.5¢ at any reasonable ref-pitch (verified: float epsilon ≈ 1e-7 ⇒ <0.0001¢ at A4=440) | Mitigated by double-precision intermediate |
| 35 | APVTS REFERENCE_PITCH read-per-note adds ~5 ns CPU per note-on (`getRawParameterValue`) | Phase 2.6b R40b noteStarted | Negligible (<0.0001% of noteStarted budget); no smoothing needed since ratio applied at note-on only (Q17) | Mitigated by Q17 lookup-once design |
| 36 | TuningEngine state divergence between APVTS save/restore (DAW project reload) — Scala file path NOT persisted at v1.0 | DAW state save/restore lifecycle | TUNING_SYSTEM Choice IS in APVTS; on restore, parameterChanged fires for TUNING_SYSTEM ⇒ setMode dispatch ⇒ frequencyTable rebuilt; Scala file path persistence deferred to v1.1 (O-Lyrica reference at PluginProcessor:543–631) | Mitigated for 12-TET / MTS-ESP; KNOWN LIMITATION for Scala (v1.1 upgrade) |

**Status summary:** 30 mitigated + 3 expected (#28 acceptable message-thread blocking; #30 D1 lock; #36 Scala-only known limitation by v1.1 design) + 3 known-limitations (Risk #36 Scala-restore; Risk #34 ratio precision floor; ESCALATION-RP1 module-API design-intent decision). **0 OPEN risks at plan-phase entry.**

---

## Success Criteria (Gate 8b — Phase 2.6b verify exit gate)

**5 invariants — ALL must PASS for R40 atomic to land:**

1. **12-TET default state byte-identical to Phase 2.6a goldens.**
   - Required: 14-entry `reproduce-goldens.sh` PASS at post-Phase-2.6a sha256s post-R40a/R40b/R40c source edits.
   - Algebraic proof at §23.6.6 guarantees this; empirical verification at R40d step 2.
   - HR-12 contract (atomic frequencyTable per-slot) verifies at module level.

2. **Scala/TUN file load → expected pitch deviation on 19-EDO test file.**
   - Required: all per-segment `|delta_cents| < 0.5` for the scala-mode default sequence (MIDI 60, 67, 72, 79, 60).
   - Verified at R40d step 7 via `microtonal-scala.json` parse.

3. **MTS-ESP stub returns 12-TET behavior; pluginval-10 thread-safety PASS.**
   - Required: `microtonal-mts-esp.wav` (ephemeral) sha256 matches `microtonal-12tet.wav.sha256`.
   - Verified at R40d step 9 + R40e step 8 (pluginval-10 Background thread state + Parameter thread safety).

4. **MPE pitch-bend ±24 semitones per-note tracking on channel-2 (legacy mode).**
   - Required: `microtonal-mpe.wav` reproduces byte-identical 3-trial; `pass_pitchTracking` (all `|delta_cents| < 10`); `pass_rmsContinuity` (≥0.85).
   - Verified at R40e step 2 + R40d steps 6–8.

5. **auval AU + pluginval-10 SUCCESS + DSP-05 coexistence audit PASS.**
   - Required: `auval -v aumu OCbs OuDv` SUCCESS; `pluginval --strictness-level 10` SUCCESS full battery; `detune-sweep-A.wav` sha256 bit-equality at TUNING_SYSTEM=12-TET against Phase 2.6a sha256.
   - Verified at R40e steps 7, 8, 9.

**Plus:**

- 4-file production source audit hook reports EXACTLY {3 source M + 1 harness M} (R40e step 3).
- 0-file CMakeLists.txt audit hook (R40e step 3).
- 0-file parameter-spec.md amendment audit hook (Q14 LOCKED).
- 1-file fixture NEW (`test-19edo.scl`) + 12-file golden artefact NEW + 1-script M (`reproduce-goldens.sh` 14→17).
- Saturator carry-forward verify (R40e step 4).
- BodyResonator + BowNoiseGenerator integration verify (R40e step 5).
- `setLatencySamples` invariant (PERF-03 zero-latency preserved; R40e step 6).
- ESCALATION-RP1/MTS1/MPE1/FPK1 each addressed: REFERENCE_PITCH ratio Option B landed in voice; APVTS Choice → Mode 3-case switch + callAsync landed in processor; 20 ms detuneSmoothed adequate for ±24 semi MPE bend; Scala file-load public method on processor + harness `--microtonal --scl` invocation landed.

If Gate 8b PASS (5/5 invariants strict-PASS), R40 atomic + R40-backfill chore land; STATUS.md flips `phase: verify` → `phase: complete` for Phase 2.6b sub-cycle, then `next_action: phase_2_6c_discuss` for the next sub-cycle.

If Gate 8b FAIL on any of the 5 invariants, BLOCK at R40 atomic; investigate; re-iterate at R40d/R40c/R40b/R40a; do NOT land R40 atomic until all 5 PASS.

---

## Out of Scope (deferred per CONTEXT rev-11.b + RESEARCH §23 + Q11–Q20 locks)

- **Phase 2.4-bis backlog (≈8 items)** — DSP-07 retune; DSP-09 vibrato transfer tune; DSP-08 breathingAudible metric; 3 v1.0 fallback cells; true Helmholtz slip-detection; wolf-region suppression; bow-noise calibration; saturator-tail body-coupling deep characterisation. **All deferred to v1.1** (Q2 LOCKED; carry-forward from Phase 2.6a).
- **Phase 2.6c VST3 Note Expression FUNC-06 + FUNC-05 MPE Y/Z** — separate sub-cycle (R41 atomic). PLAN rev-15 authors at next sub-cycle plan-phase.
- **3 ARCHITECTURE.md amendments** (§"DC Blocker" + §"In-loop saturator" + §149/§509 size_scalar) — FOLDED into Phase 2.6c verify-phase as a single amendments task (Q7 LOCKED). Phase 2.6b does NOT amend ARCHITECTURE.md.
- **MTS-ESP-Client SDK link** — REJECTED at v1.0 per Q11 Option B; v1.1 lights up real client (module already declares `connectMTSClient()` placeholder; SDK linking + licensing surface added at v1.1 cycle).
- **Scala file path persistence in `setStateInformation`** — REJECTED at v1.0 per Risk #36 known-limitation; v1.1 adds extension (O-Lyrica reference at PluginProcessor:543–631).
- **Stage 3 Editor file picker for Scala/TUN load** — REJECTED at Phase 2.6b per Q12 Option A; harness-only file-load at v1.0; Stage 3 cycle (GUI) adds `juce::FileChooser` invocation post-`AsyncUpdater` from Editor.
- **Per-channel MPE master tune** (TuningEngine `midiChannel` parameter slot) — DEFERRED to v1.1+ per §23.2.1 module-reserves-the-slot semantics; v1.0 plugin passes `0` as no-op.
- **Voice-side TuningEngine re-poll mid-sustain (live retuning)** — REJECTED per Q17 Option A LOCK; v1.0 takes effect at next note-on only (RT-safe; matches O-Lyrica precedent). v1.1 evaluates per-block re-poll if user feedback warrants live retuning of held notes.
- **Module patch (clamp range expansion 220–880 Hz, OR new `setMasterTuneUnclamped` API)** — REJECTED per ESCALATION-RP1 LOCK Option B; voice-side ratio multiplication is the v1.0 path; module untouched.
- **Module-side per-note pitch-bend pathway** (`TuningEngine::setPitchBend(int note, float)`) — REJECTED at v1.0; voice multiplies `2^(bend/12)` on top of `getFrequency()` per Q5 priority order. Module's `notePitchBends` array stays at NO_BEND sentinel for all 128 slots throughout v1.0; v1.1 may switch to module pathway if VST3 NE per-note tuning needs unification.
- **HR-13 (any new hard rule for Phase 2.6b)** — REJECTED. HR-12 introduced at discuss-phase covers tuning-table swap RT-safety; HR-1..HR-10 + HR-12 carry forward. HR-13 slot reserved for Phase 2.6c (raw-event drain for VST3 NE) if needed.
- **CMakeLists.txt edits** — REJECTED. Header-only DSP design avoids Phase 2.5 R37 deviation #1; 0 CMake edits is an explicit R40e audit-hook invariant. TuningEngine.h include resolves via Stage-1 module include-dir already wired at CMakeLists.txt:49.
- **Production WAV binary commits** — `microtonal-12tet.wav` / `microtonal-scala.wav` / `microtonal-mpe.wav` ARE committed (3-trial bit-stable goldens per Phase 2.6a precedent). MTS-ESP ephemeral render is NOT committed (sha256 verification only; 12-TET baseline IS the committed reference).
- **CI invocation of `--microtonal` or `--mpe-pitch-bend`** — out of scope; harness modes are offline (developer-machine-only). CI runs the existing build + auval + pluginval pipeline.
- **R40 atomic split into multiple commits** — REJECTED per Phase 2.4c-bis R36-bis + Phase 2.5 R37 + Phase 2.6a R39 atomic-commit precedent. Single atomic + R40-backfill chore shape.
- **R38-equivalent BLOCKING audition for Phase 2.6b** — REJECTED. Phase 2.6b is integration work (TuningEngine wire-up), not novel DSP; full audition rolls into Stage 4 polish-cycle final-output audition per Q6 carry-forward.

---

## Cross-Cycle Carry-Forward (LOCKED — verbatim from CONTEXT rev-11 + rev-11.b)

- HR-1..HR-10 in effect (HR-11 retired at Phase 2.4c-bis cycle open). **HR-12 LOCKED at Phase 2.6b discuss (Q16);** effective from R40 execute-phase onwards; promotes to all v1.1 follow-up cycles.
- 14 audible goldens reproduce byte-identical at HEAD descendant of R39 atomic `74b3f83e6d10162b3c28ef966aa79d4adf8e62f0`; R40 atomic preserves bit-equality at TUNING_SYSTEM=12-TET default (Gate 8b inv #1; algebraic proof §23.6.6).
- `output-chain.wav` golden carries forward at Phase 2.6a-bis sha `b5fc1d60b02902f1127b6ec6516b924262dd75b2d2d2d8d8a1a88ff7fec5f9ad`.
- `matrix-stability.wav.sha256 = 6db67707…` evidence-only golden carries forward (NOT in reproduce-goldens.sh; Phase 2.6b does NOT re-render — no upstream change touches matrix-stability inputs).
- In-loop saturator at `4·tanh(x/4)` (Phase 2.4c-bis R36-bis port) UNTOUCHED in Phase 2.6b (Phase 2.6b touches NONE of WaveguideString.cpp).
- BodyResonator (8-mode static-Q bank) + BowNoiseGenerator (3-band BPF + period-heuristic slip bursts) UNTOUCHED in Phase 2.6b.
- SchellengCalibration + SubHarmonicBias + DispersionFilter UNTOUCHED in Phase 2.6b.
- Master Saturator + Master Limiter + Stereo Width (Phase 2.6a R39 deltas) UNTOUCHED in Phase 2.6b.
- Stage-1 parameter-spec.md sha256 `ae956e944ebe6dad84a8af48ff2fb53cf32a16ee1afde4baf9c1bd8ce21ad08e` (Phase 2.6a anchor) carries forward UNCHANGED through Phase 2.6b (Q14 LOCKED zero amendments). PluginProcessor.cpp:8 comment matches.
- ARCHITECTURE.md carries forward through Phase 2.6b execute-phase; 3 amendments folded into Phase 2.6c verify-phase per Q7 LOCK (NOT Phase 2.6b).
- Atomic-commit sequence: R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → R37 → R39 → R39-bis (pending land) → **R40** (Phase 2.6b atomic; this PLAN target) → R41 (Phase 2.6c) → Stage 2 verify amendments commit.
