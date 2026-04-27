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
