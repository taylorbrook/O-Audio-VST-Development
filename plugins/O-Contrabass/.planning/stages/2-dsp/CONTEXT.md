# Stage 2: DSP — Context (rev-3)

**Date:** 2026-04-27
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP)
**Phase:** discuss
**Cycle Scope:** **Phase 2.1c — Cascaded Allpass Dispersion (Gate 3)**
**Supersedes:** rev-2 (Phase 2.1a closure + 2.1b opening, dated 2026-04-26). rev-2 contracts that remain locked are inherited verbatim and not re-litigated.

---

## Discussion Summary

**Participants:** User, Claude

This discuss cycle opens Phase 2.1c — cascaded allpass dispersion on the bridge rail of the E-string waveguide — the third and final sub-phase of Phase 2.1. Phase 2.1b verified ✅ on 2026-04-27 (R8a `bd5fae0` + R15 `ef0604d` atomic commits, Gate 2 PASS bit-exact); the friction module is now extracted and consumed by both O-Bowed and O-Contrabass.

The Phase 2.1c scope is single-purpose: implement the Rauhala/Välimäki 2006 cascaded first-order allpass dispersion filter, wire it onto the bridge rail of the existing split-rail waveguide for the E-string (M=4), validate via STRING_STIFFNESS sweep + 60 s sustained-tone harness rerun, and atomic-commit on Gate 3 PASS.

After Phase 2.1c verifies, Phase 2.1 (the highest-risk phase, ~50% of project risk) closes and Phase 2.2 (per-string detune + A/D/G strings) opens as a fresh GSD cycle.

---

## Cycle Scope

**Goal:** Implement cascaded allpass dispersion (Rauhala/Välimäki 2006) on the E-string waveguide's bridge rail. Validate that STRING_STIFFNESS produces continuous, click-free timbral change; that dispersion at 100 % affects attack character but not steady-state pitch (mode-locking); and that the engine remains stable under the Gate 1 bow-on-only 65 s harness. Atomic-commit on Gate 3 PASS.

**In scope:**
- `Source/DSP/DispersionFilter.h` — new file, per-plugin (NOT extracted to a shared module — coefficient closed-form is plugin-agnostic but topology integration is plugin-specific; module-promotion can be revisited if O-Bowed grows a dispersion filter later).
- `Source/DSP/WaveguideString.{h,cpp}` — wire `DispersionFilter<4>` onto the bridge rail at the locked placement (see Approach Decisions Q1).
- `Source/BowedContrabassVoice.{h,cpp}` — advance the existing 20 ms `stiffnessSmoothed` per block; recompute coefficient `a` once per block from current smoothed stiffness; push to `WaveguideString::setDispersionCoefficient(a)` (or equivalent setter — research-phase finalises API shape).
- `tests/render-harness/main.cpp` — add `--stiffness-sweep` CLI mode (ramp STRING_STIFFNESS 0→1 over 60 s; dump WAV).

**Out of scope (deferred to later Phase 2.x cycles):**
- A1/D2/G2 strings + per-string M=3/2/1 dispersion table (Phase 2.2)
- Per-string detune ±1200 cents (Phase 2.2)
- Vibrato + Slow-Bow LFO + Schelleng wedge clamp (Phase 2.3)
- Sub-harmonic bias + 108-combo stability matrix (Phase 2.4)
- Body resonator + bow noise (Phase 2.5)
- Master saturator/limiter, stereo width, microtonal, MPE (Phase 2.6)
- ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments (end-of-Stage-2 verify per locked decision)

---

## Requirements Confirmed (Phase 2.1c-relevant subsets of locked contracts)

- **DSP-03** (cascaded allpass dispersion, M=4 for E-string): primary deliverable of this cycle.
- **DSP-01** (waveguide string, Lagrange3rd, 8192-sample buffer): in effect; Phase 2.1c does NOT touch the delay-line topology — split-rail bridgeDelay/neckDelay stays exactly as committed in `ef0604d`.
- **FUNC-02** (sustained tone, no runaway, no NaN): carry-forward from Phase 2.1a-recovery + Phase 2.1b verify; dispersion must not regress.
- **PERF-01** (no allocations, no locks, no file I/O in `processBlock`): enforced — coefficient computation runs in `BowedContrabassVoice::renderNextBlock` *before* the per-sample loop, on smoothed stiffness; per-sample dispersion processing is 1 multiply + 1 add + 1 state load per section, no allocs.
- **PERF-02** (< 5 % CPU on M1): tracked — dispersion at M=4 adds ~0.3 % per the architecture's per-component CPU budget table; well within margin.
- **PERF-03** (latency = oversampler only): in effect; dispersion's group delay is *subtracted* from the base delay-line length in `updateDelayLengths()` (Phase 2.1c R17 plumbing) so reported plugin latency is unchanged.
- **QUAL-01** (no audible clicks during parameter sweeps): explicit Gate 3 invariant — STRING_STIFFNESS sweep 0→100 % produces continuous timbral change, no clicks.

---

## Constraints Identified

**Locked contracts (do NOT modify in this cycle):**
- All 29 APVTS parameter IDs, ranges, skews — `parameter-spec.md` (sha256:c47fe736…)
- DSP architecture (`research/ARCHITECTURE.md`, sha256:3cb26814…) — F3 deviation flagged in PLAN rev-3 + R7 commit; ARCHITECTURE amendment still deferred to end-of-Stage-2 verify
- ROADMAP phasing (sha256:106639f6…)
- `modules/synthesis/bow-friction/` (extracted Phase 2.1b) — module is value-class deterministic; Phase 2.1c does NOT touch friction.

**JUCE 8 critical patterns (auto-loaded `spike-findings-VST-development` + memory):**
- `getLatencySamples()` is non-virtual — keep using `setLatencySamples()` in `prepareToPlay`; dispersion's compensated group delay does not change reported latency.
- `juce::ScopedNoDenormals` at `processBlock` entry (mandatory). Allpass cascades can produce small persistent state values that benefit from FTZ; rely on the existing scope.
- `IS_SYNTH TRUE` + output-only `BusesProperties` already in place from Stage 1.
- Both WebView2 flags already in place from Stage 1.

**Phase 2.1c-specific constraints:**
- **Bridge-rail-only placement** — dispersion lives on the bridge rail's path between `popSample` and the bridge LP (locked Q1 decision). Nut rail remains untouched (no dispersion, no LP, no saturator — `-1` boundary only). Mirrors O-Bowed's bridge-rail-only loop chain.
- **Identity at stiffness=0** — `B = 1e-4 · STRING_STIFFNESS`; at STRING_STIFFNESS=0, `B → 0`, closed-form `a → 0`, allpass becomes identity. Gate 3 includes a bit-exact regression at STRING_STIFFNESS=0 to prove this.
- **Coefficient cadence: per-block** — `a` recomputed once per block from `stiffnessSmoothed.getNextValue()` advanced by `numSamples` (skip-ahead). Per-sample `a` modulation is the click-fallback (only invoked if R18 sweep produces clicks).
- **No mid-stage architecture amendment** — if dispersion behaviour disappoints sonically (e.g., too subtle on E1), address via Phase 2.4 follow-up RESEARCH note, not architecture rework. Q3 prefactor `1e-4` is locked unless R18 reveals an outright bug.

**Working-tree starting state (locked from Phase 2.1b verify):**
- `WaveguideString.{h,cpp}` rev-3 split-rail (committed `ef0604d`)
- `BowedContrabassVoice.{h,cpp}` Phase 2.1b extraction-consumer state (committed `ef0604d`)
- `modules/synthesis/bow-friction/` v1.0.0 (committed `ef0604d`, registry entry at `modules/registry.yaml:292-293`)
- O-Bowed canonical preset golden render: sha256 `93124fb8…34c8891` (committed at `plugins/O-Bowed/tests/render-harness/golden/canonical-preset.wav.sha256`, harness committed `bd5fae0`)
- O-Contrabass bow-on-only reference render: sha256 `00431582…d5e60`

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Q1 — Dispersion placement on bridge rail** | **(a) Before bridge LP** — `pop → dispersion → bridge LP → −1 boundary → friction → inject → saturator → push` | Direct match to ARCHITECTURE.md §"Cascaded Allpass Dispersion" (line 417 "immediately before bridge filter on the right-going wave") + §"Processing Order" (line 267 "dispersion → bridge LP → saturator → DC blocker → fractional delay"). Canonical Smith PASP chain `dispersion → loss → nonlinearity → delay`. The placeholder comment at `WaveguideString.cpp:170-171` (which suggests after-saturator placement) is **stale** — written before split-rail rev-3 landed; supersede in Phase 2.1c R17 with the correct (a) placement and update the comment. |
| **Q2 — M for Phase 2.1c (E1 only)** | **Hardcode M=4 for E1** — do NOT pre-wire the per-string M=4/3/2/1 table | Single-string scope; A/D/G strings come in Phase 2.2 with the per-string bank, which is the natural home for the M-table. Don't pay table-plumbing cost twice. `DispersionFilter<4>` template parameter at compile time; per-string M parameterisation deferred to Phase 2.2. |
| **Q3 — Inharmonicity B mapping** | **`B = 1e-4 · STRING_STIFFNESS`** verbatim per ARCHITECTURE.md §"String Waveguide Bank" (line 81) | No mid-stage architecture override. Prefactor `1e-4` is the locked E1 value. If R18 sweep reveals the audible range is too subtle/aggressive, file as Phase 2.4 follow-up RESEARCH note (matches the saturator-tail Phase 2.4 parking pattern from rev-2). |
| **Q4 — Gate 3 invariants** | **Six-item bar:** STRING_STIFFNESS sweep no clicks; 100 %-stiffness affects attack but not steady-state pitch (mode-locking); BRIGHTNESS sweep no clicks; auval + pluginval-10 PASS; bow-on-only 65 s harness 4/4 TRUE (no regression of Gate 1); **bit-exact regression at STRING_STIFFNESS=0** (dispersion identity check). | First five items carry forward verbatim from PLAN rev-3 §"Success Criteria". Sixth item (bit-exact at stiffness=0) is the cheap-strong regression bar that proves the dispersion path correctly degenerates to identity when `B → 0` — same regression-bar philosophy as Phase 2.1b's bit-exact O-Bowed canonical render. |
| **Q5 — Stiffness-sweep validation harness** | **(a) Extend `tests/render-harness/main.cpp` with `--stiffness-sweep` CLI mode** | Automated, reproducible, matches Gate 1/Gate 2 cadence. ~30 LOC of CLI plumbing — ramp STRING_STIFFNESS 0→1 linearly over 60 s, dump WAV with sha256 + JSON metadata. User auditions in Logic post-render for the "continuous timbral change, no clicks" qualitative invariant; harness mechanically captures the WAV + sha256 for repeatability. |
| Coefficient cadence | Per-block from smoothed stiffness | Already-wired 20 ms `stiffnessSmoothed` (`WaveguideString.cpp:38`) advances per block in voice's `renderNextBlock` (R17); coefficient `a` recomputed once per block. Per-sample `a` modulation reserved as click-fallback if R18 sweep fails. |
| DispersionFilter location | **Per-plugin** at `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` — NOT a shared module | O-Bowed has no dispersion filter (verified — `plugins/O-Bowed/Source/DSP/` has no `Dispersion*` files). Module promotion premature with one consumer. Revisit in a future cycle if O-Bowed adds dispersion. Keeps Phase 2.1c surface tight. |
| Latency compensation | Subtract dispersion group delay from base delay-line length in `updateDelayLengths()` | Standard Smith PASP technique. Group delay at the fundamental frequency `D_disp(f0) = M · (1 − a²) / |1 + a·e^{-j·2π·f0/sr}|²` (closed-form per Rauhala/Välimäki paper §III.B). Base length stays bit-exact when `a=0` (M·(1−0)/1 = M ≈ 4 samples, but at `a=0` group delay = M, so subtract 4 samples; at `a=0` the allpass IS identity so the subtraction is exactly compensated by the M unit-delays it inserts — net delay unchanged). Research-phase derives the exact subtraction formula. |
| Atomic commit unit | **R20 (Phase 2.1c) atomic commit** lands `DispersionFilter.h` + `WaveguideString.{h,cpp}` edits + `BowedContrabassVoice.{h,cpp}` edits + `tests/render-harness/main.cpp` `--stiffness-sweep` mode + planning artefacts (CONTEXT/RESEARCH/PLAN/SUMMARY/VERIFICATION/STATUS updates) — all in one commit, only on Gate 3 PASS. | Same gate-first principle as R7 / R15. R20 numbering continues the Phase 2.1 task sequence (R1–R7 = 2.1a-recovery, R8–R15 = 2.1b extraction, R16–R20 = 2.1c dispersion). |
| Phase 2.1c primary listening DAW | **Logic Pro (AU)** (rev-1 / rev-2 carry-forward) | Same validated workflow used through Phase 2.1a/2.1b. AU is the stricter validation surface. Manual smoke after R20 commit on E1 sustained tone with STRING_STIFFNESS at 0 / 50 / 100 % to confirm "attack changes, steady-state pitch locks". |

---

## Phase 2.1c Test Criteria (locked — Gate 3 exit bar)

- [ ] **STRING_STIFFNESS 0%→100% sweep** produces continuous timbral change, no audible clicks (validated via `--stiffness-sweep` harness WAV + Logic audition).
- [ ] **STRING_STIFFNESS = 100 %** audibly affects attack character but **not** steady-state pitch (mode-locking — Helmholtz period dominates after the first ~50 ms).
- [ ] **BRIGHTNESS sweep 80 Hz → 12 kHz** produces no clicks (regression check — bridge LP path now has dispersion upstream of it).
- [ ] **`auval -v aumu OCbs OuDv` PASS** for O-Contrabass.
- [ ] **`pluginval --strictness-level 10 --validate-in-process` PASS** for O-Contrabass VST3.
- [ ] **Bow-on-only 65 s render-harness** at INFINITE_SUSTAIN=1.0: 4/4 invariants TRUE (no regression of Gate 1 stability).
- [ ] **Bit-exact regression at STRING_STIFFNESS=0** — render bow-on-only 65 s with STRING_STIFFNESS=0 *before* R16 (golden) → render same after R19 → `cmp` byte-equal. Proves the dispersion path is identity when `a → 0`.
- [ ] **R20 atomic commit landed** — module + harness CLI + planning artefacts in one commit.

---

## Open Questions (for research phase)

The following questions remain genuinely-open and are handed to Phase 2.1c research:

1. **Closed-form `a` coefficient — exact paper constants.** ARCHITECTURE.md §"Cascaded Allpass Dispersion" gives the structural form (`I = log2(f0/440)·12 + 49`, `lB = log(B)`, `lM = log(M)`, `C = m1·lB + m2·lM + m3·lB·lM + m4`, `k = k1 + k2·I + k3·I²`, `a = clamp(-C/k, -0.99, 0.99)`) but does NOT pin the literal `m1..m4, k1..k3` values. Research-phase must extract them from Rauhala/Välimäki 2006 IEEE SP Letters Table 1 (or the equivalent published table) and pin them in `DispersionFilter.h` as `constexpr` values with a citation comment.

2. **Group-delay subtraction formula in `updateDelayLengths()`.** What is the exact closed-form for `D_disp(f0)` of an M-section first-order allpass cascade at the fundamental? Two candidates from the literature:
   - (a) Per-section group delay at DC: `D_section(f=0) = (1 − a²) / (1 + a)² = (1 − a) / (1 + a)`; total = M · D_section. Cheap; correct only at DC.
   - (b) Per-section group delay at `f0`: `D_section(f0) = (1 − a²) / |1 + a·e^{-j·2π·f0/sr}|²`; total = M · D_section. Slightly more expensive; matches the actual phase delay at the fundamental.
   Research-phase picks (a) or (b) and pins the formula. **Recommend (b)** — accuracy at `f0` is what mode-locking cares about; cost is negligible (one block-rate computation).

3. **Per-sample setter API on `WaveguideString` for dispersion coefficient.** Two shapes:
   - (a) `setDispersionCoefficient(float a)` — voice computes `a` from smoothed stiffness once per block, calls setter. Symmetric with existing `setBrightness(cutoffHz)` etc.
   - (b) `setDispersionStiffness(float stiffness01)` — voice forwards smoothed stiffness; `WaveguideString` computes `a` internally. Encapsulates the `a` math inside the waveguide.
   **Recommend (a)** — the closed-form computation depends on `f0` (the per-note fundamental), which is voice-state, not waveguide-state. Voice computes once per block; waveguide accepts the result. Clean separation.

4. **DispersionFilter.h template/class shape.** Three shapes:
   - (a) `template <int MaxSections> class DispersionFilter` — fixed cascade depth at compile time (M=4 for E1 here).
   - (b) `class DispersionFilter` with runtime `int M` parameter — dynamic depth, configurable at `prepare()` time.
   - (c) `template <int MaxSections> class DispersionFilter` with runtime `int activeSections ≤ MaxSections` — both compile-time max and runtime active count.
   **Recommend (c)** — Phase 2.1c uses M=4 for E1 hard-locked; Phase 2.2 will introduce per-string M (4/3/2/1) and benefits from a runtime `activeSections` selector without changing the template. Pre-allocates state for the worst case (M=4) at compile time.

5. **`--stiffness-sweep` harness output format.** The existing harness writes `e1-max-sustain.wav` + `e1-max-sustain.json`. Should `--stiffness-sweep` emit:
   - (a) Single WAV `e1-stiffness-sweep.wav` (60 s mono float, MIDI E1, STRING_STIFFNESS ramps 0→1 linearly over duration, all other params at defaults), plus JSON metadata documenting ramp params + sha256.
   - (b) Three discrete WAVs at STRING_STIFFNESS = 0 / 50 / 100 % (5 s each, separate files).
   **Recommend (a)** — single file is easier to listen-test continuously in Logic; ramp-discontinuity audibility is the actual click-detection invariant. (b) is captured implicitly by listening to specific time-windows of the (a) WAV.

---

## Files / Artefacts to Produce in Phase 2.1c

**Source (new):**
- `plugins/O-Contrabass/Source/DSP/DispersionFilter.h` — Rauhala/Välimäki 2006 closed-form, `template<int MaxSections=4> class DispersionFilter`, `prepare`, `reset`, `setCoefficient(float a)`, `processSample(float x)`, `getGroupDelaySamples(float f0, float sr) const`.

**Source (modified):**
- `plugins/O-Contrabass/Source/DSP/WaveguideString.h` — add `DispersionFilter<4> bridgeDispersion` member; add `setDispersionCoefficient(float a)` setter; update `processSample` to insert dispersion BEFORE bridge LP on the bridge rail; update `updateDelayLengths()` to subtract dispersion group delay from base length.
- `plugins/O-Contrabass/Source/DSP/WaveguideString.cpp` — corresponding implementations; update the stale "Phase 2.1c placeholder" comment at line 170-171 to reflect the (a)-placement decision (before bridge LP, not before saturator).
- `plugins/O-Contrabass/Source/BowedContrabassVoice.cpp` — in `renderNextBlock`, advance `stiffnessSmoothed` per block, compute `a` from current smoothed stiffness via `DispersionFilter::computeCoefficient(f0, B, M)` static helper, push to `waveguideString.setDispersionCoefficient(a)`.
- `plugins/O-Contrabass/tests/render-harness/main.cpp` — add `--stiffness-sweep` CLI flag; ramp STRING_STIFFNESS 0→1 over 60 s; emit `e1-stiffness-sweep.wav` + `.json` with sha256.

**Test artefacts:**
- `e1-stiffness-sweep.wav` + `.json` (Phase 2.1c R18 validation render).
- `e1-bowon-only-stiffness-zero-pre.wav` (golden — captured BEFORE R16 source edits).
- `e1-bowon-only-stiffness-zero-post.wav` (Gate 3 verification — bit-exact match to golden required).

**Verification artefacts:**
- `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` — append §14 (Phase 2.1c dispersion research) resolving the 5 Open Questions above.
- `plugins/O-Contrabass/.planning/stages/2-dsp/PLAN.md` — append rev-5 (R16–R20 task bodies authored fresh; the rev-1/rev-2/rev-3 references to "Tasks 17–20 carry forward verbatim" were placeholders — actual task bodies need to be written in this rev because the prior PLANs only described scope, not execution detail).
- `plugins/O-Contrabass/.planning/stages/2-dsp/SUMMARY.md` — append "Phase 2.1c execute" section after R20 lands.
- `plugins/O-Contrabass/.planning/stages/2-dsp/VERIFICATION.md` — append "Phase 2.1c verify" section with Gate 3 pass-bar evidence.
- `plugins/O-Contrabass/.planning/STATUS.md` — flip `next_action` to `phase_2_2_discuss` after Gate 3 PASS.

---

## Risks

1. **Dispersion produces audible clicks under STRING_STIFFNESS automation.** Mitigation: per-block coefficient cadence with 20 ms `stiffnessSmoothed` is the first defence; if R18 sweep produces clicks, fall back to per-sample `a` interpolation (cubic interp between block-boundary `a` values, ~5 LOC change in `WaveguideString::processSample`). Research-phase confirms per-sample fallback is a known O-Bells/O-Lyrica pattern.
2. **Group-delay subtraction wrong → pitch drifts as STRING_STIFFNESS changes.** Mitigation: bit-exact regression at STRING_STIFFNESS=0 catches the degenerate case (M unit delays + M·D_section subtraction must net to zero). For STRING_STIFFNESS > 0, the "mode-locking" Gate 3 invariant (steady-state pitch unchanged at 100 % stiffness) is the audible regression bar; quantitative check via FFT-bin peak detection on the sustained tone is the harness-level fallback if the audible test is ambiguous.
3. **Bridge LP recurrence regression after dispersion is inserted upstream.** Mitigation: F2 LP form (`y = g·(1−p)·x + p·y_prev + leak`, locked Phase 2.1a-recovery) is independent of dispersion — dispersion only changes the input `x`. Bow-on-only 65 s harness invariants (4/4 TRUE) catches any LP regression. The bit-exact regression at STRING_STIFFNESS=0 is the strongest possible bar — any LP-touching change shows as byte-difference.
4. **Coefficient overflow / NaN at extreme STRING_STIFFNESS.** Mitigation: `a = clamp(-C/k, -0.99, 0.99)` is in the closed form; values outside `(-1, 1)` would make the allpass unstable. Research-phase confirms the clamp range matches the paper's stated stability bounds.
5. **Per-string M-table absence makes Phase 2.2 wiring non-trivial.** Acknowledged but acceptable: Phase 2.1c's `DispersionFilter<4>` template-max + runtime `activeSections` (Q4 (c) recommendation) lets Phase 2.2 wire per-string M without re-templating — `activeSections` becomes the per-string variable.
6. **Harness `--stiffness-sweep` adds CLI complexity.** Acceptable: ~30 LOC, isolated to harness, no production-code coupling. Pattern matches the existing CLI structure (already accepts `--note`, `--velocity`, `--sustain` flags per Phase 2.1b R8 implementation).

---

## Next Phase

Ready for: **research** phase — `/plugin-research O-Contrabass 2-dsp`

Research focus (Phase 2.1c):

1. **Pin Rauhala/Välimäki 2006 closed-form constants** (`m1..m4, k1..k3`) — extract from the IEEE SP Letters paper Table 1 (or the equivalent table in `research/O-Contrabass-bass-waveguide-stability.md`) and lock as `constexpr` values in the planned `DispersionFilter.h`.
2. **Resolve Open Question #2** — pick the group-delay formula (recommend `D_section(f0) = (1 − a²) / |1 + a·e^{-j·2π·f0/sr}|²`) and document the closed-form derivation.
3. **Resolve Open Questions #3–#5** — setter API shape (recommend `setDispersionCoefficient(float a)`), `DispersionFilter.h` template/class shape (recommend (c) — fixed `MaxSections` + runtime `activeSections`), harness output format (recommend single WAV ramp).
4. **Pre-flight bit-exact baseline render** — capture `e1-bowon-only-stiffness-zero-pre.wav` BEFORE any R16 source edits, log sha256. This is the Gate 3 STRING_STIFFNESS=0 regression-bar golden reference.
5. **Pattern-confirm `--stiffness-sweep` against existing harness CLI** — confirm the existing CLI parser shape, note any required `getopt`-style flag handling that needs new branches.
6. **Update RESEARCH.md** — append §14 documenting the resolutions above. (No §12/§13 changes; those are Phase 2.4 follow-up + 2.1b history.)

After research: plan-phase (PLAN.md rev-5) writes R16–R20 task breakdown verbatim against this CONTEXT + research findings; execute-phase performs the implementation + R20 atomic commit; verify-phase confirms Gate 3 invariants + Logic AU smoke.

---

## Audit Trail (rev-3 supersedes rev-2)

**rev-1 (earlier 2026-04-26):** Phase 2.1 broad discuss. Cycle scope = Phase 2.1 (sub-phases a/b/c).

**rev-2 (later 2026-04-26):** Phase 2.1a closure (Option A, R7 commit) + Phase 2.1b opening (module extraction, Gate 2). 9 approach decisions, 5 open questions. Phase 2.1b verified 2026-04-27 (R8a `bd5fae0` + R15 `ef0604d` atomic commits, Gate 2 PASS bit-exact).

**rev-3 (this document, 2026-04-27):** Phase 2.1c opening — cascaded allpass dispersion (Rauhala/Välimäki 2006), bridge-rail-only on E-string, Gate 3 exit bar. 5 approach decisions (Q1–Q5 all confirmed by user as recommendations: placement before bridge LP, M=4 hardcoded, B=1e-4·stiffness verbatim, six-item Gate 3 bar with bit-exact regression at stiffness=0, harness `--stiffness-sweep` mode). 5 open questions handed to research-phase: closed-form constants, group-delay formula, setter API shape, template/class shape, harness output format.

**Inherited verbatim from rev-2 (not re-litigated):**
- All Phase 2.1a-recovery contracts (split-rail topology, F2 LP form, F3 no in-loop DCB, F4 betaScale removed)
- All Phase 2.1b contracts (bow-friction module v1.0.0 at `modules/synthesis/bow-friction/`, both plugins consume)
- ARCH §"DC Blocker" + §"In-loop saturator" amendments deferred to end-of-Stage-2 verify
- Saturator-tail Phase 2.4 follow-up parking + RESEARCH §12 footnote
- Primary listening DAW: Logic Pro (AU)
- Sample-rate strategy: internal 88.2 / 96 kHz at friction junction
- Atomic-commit gate-first principle (R7 → R15 → R20)

**New in rev-3:**
- Q1 dispersion placement locked: before bridge LP on bridge rail (overrides stale `WaveguideString.cpp:170-171` placeholder comment)
- Q2 M=4 hardcoded for E1; per-string M-table deferred to Phase 2.2
- Q3 `B = 1e-4 · STRING_STIFFNESS` locked verbatim per ARCHITECTURE
- Q4 Gate 3 bar = six items including bit-exact regression at STRING_STIFFNESS=0
- Q5 `--stiffness-sweep` CLI mode added to render-harness
- DispersionFilter location: per-plugin (not extracted to shared module)
- Latency compensation: subtract dispersion group delay from base length in `updateDelayLengths()`
- Atomic commit: R20 lands all Phase 2.1c work in one commit on Gate 3 PASS
