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
