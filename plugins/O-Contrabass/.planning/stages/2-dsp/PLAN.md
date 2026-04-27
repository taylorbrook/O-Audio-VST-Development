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
