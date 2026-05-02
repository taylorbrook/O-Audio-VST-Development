# Stage 2: DSP — Context (rev-11)

**Date:** 2026-05-01
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP)
**Phase:** discuss
**Cycle Scope:** **Phase 2.6 — Output chain (master saturator + zero-latency feedforward limiter + stereo width) + Microtonal engine (Scala/TUN + MTS-ESP + MPE pitch-bend) + VST3 Note Expression (FUNC-05 / FUNC-06 / FUNC-07)**, executed as **3 sub-cycles** (2.6a / 2.6b / 2.6c) with discrete Gates 8a / 8b / 8c.
**Supersedes:** rev-10 (Phase 2.5 — body resonator 8-mode static-Q biquad bank + bow noise generator 3-band BPF + period-heuristic slip bursts, dated 2026-04-30). rev-10 closed 2026-04-30 with R37 atomic commit (`907a7c3409b1c2e74734f7d835ab3a934bb123fa`, Gate 7 SOFT-PASS — 5/5 invariants cleared with saturator-tail bin 64 design-intent flag (|Δ| = 17.09 dB) + sub-harm collapse 0.358 → 9.77e-05 (Phase 2.4-bis priority bump LOCKED for DSP-07 retune); matrix-stability post-body 108/108 PASS — *improvement* over Phase 2.4c-bis 4-corner regression; R38 Logic AU audition CONFIRMED PASS by user 2026-04-30 — "convincing orchestral arco bass" per BRIEF DSP-03+DSP-04 acceptance bar). All rev-10 contracts that remain locked are inherited verbatim and not re-litigated. Phase 2.5 R37-backfill chore (`36b89d2`) propagated R37 sha into STATUS.md per R34/R35/R36/R36-bis precedent.

---

## Discussion Summary

**Participants:** User, Claude

This discuss cycle activates **Phase 2.6** — the final DSP cycle that closes Stage 2 and turns the bowed-string + body-coupled voice from Phase 2.5 into a shippable, host-integrated plugin: output chain (saturator + limiter + width) + microtonal stack (Scala/TUN + MTS-ESP + MPE pitch-bend) + VST3 Note Expression for Dorico microtonal playback.

**Phase 2.6 is sub-phased into 3 cycles** (Q1 LOCKED) because the six in-scope components have very different risk profiles:

- **Phase 2.6a — Output chain (Gate 8a, low risk):** master saturator (`polynomial x − x³/3` per ARCHITECTURE §"Master Saturator" verbatim) + zero-latency feedforward limiter (RMS sidechain, 3 ms attack / 50 ms release, −0.3 dBFS ceiling, no look-ahead per PERF-03 zero-latency must-bar) + stereo width (M/S encode → side scale → decode). Pure host-rate output processing; no parameter ABI change (4 params already declared: `OUTPUT_LEVEL`, plus NEW v1.0 master sat / limiter / width — research-phase confirms parameter-spec.md amendment needed for sat amount + limiter ceiling exposure).
- **Phase 2.6b — Microtonal engine + MPE pitch-bend (Gate 8b, medium risk):** wire `modules/tuning/scala-tuning-engine` v2.1.0 (already linked at Stage 1) into voice-frequency-resolution path. Per Q8 LOCKED: tuning lookup happens once per note-on (`f_target = baseFreq · 2^((semis + perStringDetune)/12) · scalaCentsOffset · mtsEspOffset`); MPE pitch-bend deltas applied per-block via existing pitch-bend smoothing. Priority order LOCKED verbatim from ARCHITECTURE §"Microtonal Tuning Engine": **Note Expression > MTS-ESP > Scala/TUN > MPE pitch-bend > 12-TET** (Q5 LOCKED).
- **Phase 2.6c — VST3 Note Expression FUNC-06 (Gate 8c, must-bar by FUNC-06 priority):** wire `modules/tuning/note-expression` shared module + per-voice NE event drain. JUCE-NE-PATCH at `~/JUCE/` already applied (ARCHITECTURE §201 + Phase 2.6c verify confirms patch presence as a build-time precondition). O-Lyrica `Source/HarpSynthVoice.{h,cpp}` is the validated reference (production-proven, spike-validated 2026-04-23). Phase 2.6c lands wire-up + pluginval-10 + AU smoke + harness goldens; **full Dorico audition + COMPAT-02 verification is deferred to Stage 4** (Q6 LOCKED). FUNC-05 (MPE Y/Z per-note expression mapping) ships with Phase 2.6c since MPE per-note infrastructure overlaps with Note Expression event drain.

Phase 2.6 is the **most integration-heavy cycle in Stage 2**. Unlike Phases 2.1–2.5 which were single-DSP-block cycles with mature golden coverage (13 audible goldens + matrix-stability evidence + sub-harmonics-stability + saturator-tail-comparison), Phase 2.6 touches MIDI/MPE/NE event handling, host-rate output processing, and tuning-table swap (background thread → atomic pointer → audio thread read). RT-safety scrutiny rises: PERF-01 must hold under message-thread Scala/TUN file-load + atomic-swap + audio-thread render concurrency. Goldens for Phase 2.6 are mostly NEW (output-chain test modes; tuning-engine test modes; MPE test mode; NE test mode); pre-existing 14 audible goldens (13 from Phase 2.5 + saturator-tail-comparison) re-baseline at each sub-phase boundary if upstream output shifts.

**Phase 2.4-bis backlog stays parked. All ≈8 items deferred to v1.1** (Q2 LOCKED): DSP-07 retune (priority-bumped post-Phase-2.5 sub-harm collapse to 9.77e-05); DSP-09 vibrato transfer tune; DSP-08 breathingAudible metric; 3 v1.0 fallback cells; true Helmholtz slip-detection; wolf-region suppression; bow-noise calibration; saturator-tail body-coupling deep characterisation. Rationale: master saturator + limiter shift downstream metrics; chasing Phase 2.4-bis tuning targets before output chain lands risks chasing a moving target. v1.1 milestone owns the entire Phase 2.4-bis backlog as a single follow-up cycle.

**3 ARCHITECTURE.md amendments fold into Phase 2.6 verify-phase** (Q7 LOCKED) as a single amendments task, NOT a separate cycle:
1. §"DC Blocker" — F3 in-loop DCB removal evidence (Phase 2.1a F3).
2. §"In-loop saturator" — 3-evidence base (pre-port `c7e845ea…` Phase 2.4c R36 −13.09 dB, post-port `5c45d176…` Phase 2.4c-bis R36-bis −7.97 dB, post-body `130a7b02…` Phase 2.5 R37 −25.06 dB).
3. §149 vs §509 size_scalar reconciliation (formula §509 LOCKED authoritative over commentary §149).

Q3 LOCKED keeps the master saturator at ARCHITECTURE-spec'd `polynomial x − x³/3` (in-loop tanh saturator from Phase 2.4c-bis stays untouched). NO 4th amendment from Q3 — this is the architecture-faithful path.

After Phase 2.6c verifies (Gate 8c PASS), **Stage 2 verify (full)** runs as a SEPARATE `/plugin-verify O-Contrabass 2-dsp` invocation (Q10 LOCKED) — not folded into Phase 2.6c verify-phase. Stage 2 verify scope: all 24 requirements promoted to `complete` or explicitly logged-to-v1.1; 3 ARCHITECTURE amendments landed; Phase 2.4-bis backlog logged as v1.1 milestone; all goldens reproduce byte-identical against post-Phase-2.6c sha256s; auval + pluginval-10 + Logic AU smoke. Stage 3 (GUI) opens with fresh CONTEXT rev-12 post-Stage-2-verify.

---

## Cycle Scope (Phase 2.6 umbrella)

**Goal:** Land the full output chain (master saturator + zero-latency feedforward limiter + stereo width) + microtonal stack (Scala/TUN + MTS-ESP + MPE pitch-bend) + VST3 Note Expression for Dorico microtonal playback, executed as 3 sequential sub-cycles 2.6a → 2.6b → 2.6c with discrete atomic commits R39 / R40 / R41 (numbering picks up from R37 atomic + R37-backfill chore = "R38" Logic-AU-audition probe was non-source; next source-edit atomic is R39). Each sub-cycle has its own GSD discuss/research/plan/execute/verify cycle but they share this CONTEXT rev-11 as the umbrella scope contract — sub-cycle CONTEXT amendments append per sub-cycle (rev-11.a / rev-11.b / rev-11.c).

**Stage 2 closes ONLY after Phase 2.6c verify-phase + the separate full Stage 2 verify-phase** both report PASS (Q10).

### Phase 2.6a — Output chain (Gate 8a)

**In scope:**

- **`plugins/O-Contrabass/Source/DSP/MasterSaturator.h`** — NEW per-plugin header, `polynomial x − x³/3` saturator per ARCHITECTURE §"Master Saturator" verbatim (Q3). Soft-clip at ~−3 dBFS; ~+6 dB internal pre-gain headroom. Per-channel state (or stateless if pure waveshaper). Header-only `inline` per BodyResonator / BowNoiseGenerator / SubHarmonicBias precedent.
- **`plugins/O-Contrabass/Source/DSP/MasterLimiter.h`** — NEW per-plugin header, zero-latency feedforward RMS-sidechain limiter (Q4). 3 ms attack / 50 ms release / −0.3 dBFS ceiling. NO look-ahead (PERF-03 zero-latency must hold). Single-channel or stereo-linked (research confirms link mode). `juce::dsp::Compressor`-style implementation OR custom per `juce::dsp::BallisticsFilter` for envelope follower.
- **`plugins/O-Contrabass/Source/DSP/StereoWidth.h`** — NEW per-plugin header, M/S encode → side-channel scale → M/S decode per ARCHITECTURE §"Stereo Width" verbatim. WIDTH ∈ [0, 2] mapped to side-channel gain. Stereo-only (mono input → stereo output via copy + side=0 path).
- **`plugins/O-Contrabass/Source/PluginProcessor.{h,cpp}`** — wire master saturator → limiter → width as new Steps **10** (saturator), **11** (limiter), **12** (width) in the per-block processing order (between Step 9 bow-noise sum from Phase 2.5 and Step 13 output gain). Voice writes mono signal to a temporary buffer; processor steps 10–12 run host-rate post-voice on the processor's stereo output buses.
- **Parameter additions** (parameter-spec.md amendment required — first Stage-1 contract amendment since `77638e25…`):
  - `MASTER_SAT_AMOUNT` (0–100%, default 50%) — drives saturator pre-gain into `x − x³/3` curve
  - `LIMITER_CEILING_DB` (−6 to 0 dBFS, default −0.3 dBFS) — limiter ceiling
  - `WIDTH` already declared in BRIEF.md / ROADMAP — confirm declared in PluginProcessor.cpp; if not, add (default 100% = 1.0)
  - `OUTPUT_LEVEL` already declared per BRIEF.md (−inf to +12 dB, default 0 dB)
- **Render harness** — NEW CLI mode `--output-chain` for saturator/limiter/width characterisation (sweep WIDTH 0/100/200%; sweep MASTER_SAT_AMOUNT 0/50/100%; verify peak ≤ ceiling at LIMITER_CEILING_DB across high-amplitude input).
- **Goldens** — re-baseline ALL pre-existing 14 audible goldens at end of Phase 2.6a (output chain materially shifts every audible signal). NEW `output-chain.{wav.sha256, json, json.sha256}` golden.
- **R39 atomic commit** — single commit lands source + parameter-spec amendment + 14 re-baselined audible goldens + new output-chain golden + RESEARCH §22 verdict.

**Phase 2.6a Gate 8a (5 invariants):**
1. Output peak never exceeds `LIMITER_CEILING_DB + 0.05 dB` slop across high-amplitude input.
2. Click-free WIDTH automation 0% → 200% (pluginval fuzz + dedicated harness sweep).
3. PERF-03 zero algorithmic latency preserved (`setLatencySamples()` unchanged from Phase 2.5).
4. auval + pluginval-10 SUCCESS.
5. 14 re-baselined audible goldens reproduce byte-identical across re-renders (HR-style determinism on output chain).

### Phase 2.6b — Microtonal engine + MPE pitch-bend (Gate 8b)

**In scope:**

- **`modules/tuning/scala-tuning-engine` v2.1.0** — already linked at Stage 1; consume in voice/processor.
- **`plugins/O-Contrabass/Source/PluginProcessor.{h,cpp}`** — instantiate `TuningEngine` (or equivalent module API); wire APVTS reads for `TUNING_SYSTEM` (Choice: Scala/TUN | MTS-ESP | 12-TET) + `REFERENCE_PITCH` (Float, 220–880 Hz). Atomic-pointer tuning-table swap pattern (background thread parses Scala/TUN file → atomic pointer swap → audio thread reads through atomic).
- **`plugins/O-Contrabass/Source/BowedContrabassVoice.{h,cpp}`** — frequency-resolution at note-on (Q8 LOCKED): `f_target = TuningEngine::resolve(midiNote) · 2^(perStringDetune/1200)` with priority order **NE > MTS-ESP > Scala/TUN > MPE pitch-bend > 12-TET** (Q5 LOCKED). Per-block MPE pitch-bend smoothing applied as multiplicative factor on `f_target`.
- **`OContrabassMPESynthesiser`** — already enabled at Stage 1; confirm `enableLegacyMode(pitchbendRange=2, channelRange={1,17})` per ARCHITECTURE §288 for non-MPE host compatibility.
- **MTS-ESP integration** — research-phase confirms whether `scala-tuning-engine` module provides MTS-ESP client polling (likely yes per O-Lyrica precedent) or whether a separate MTS-ESP-Client SDK link is needed.
- **DSP-05 + microtonal coexistence audit** (Q9 LOCKED, deferred to research-phase) — confirm `perStringDetune` (already complete from Phase 2.2) does NOT double-count when MTS-ESP/Scala active. Render-harness bit-stability test: detune-sweep-A.wav golden under default 12-TET tuning vs Scala/TUN passthrough must produce expected delta only.
- **Render harness** — NEW CLI mode `--microtonal` (sweep TUNING_SYSTEM Choice; load test Scala/TUN file; verify output frequency matches expected). NEW CLI flag `--mpe-pitch-bend` (channel-2 pitch-bend ±100¢ per-note → output frequency tracking).
- **Goldens** — NEW `microtonal-12tet.{wav.sha256, json, json.sha256}` (baseline 12-TET — should match pre-Phase-2.6b note-sequence.wav modulo output-chain re-baseline). NEW `microtonal-scala.{wav.sha256, json, json.sha256}` (Scala/TUN file load → 19-EDO or similar test scale). NEW `microtonal-mpe.{wav.sha256, json, json.sha256}` (MPE pitch-bend response).
- **R40 atomic commit** — single commit lands source + 14 re-baselined audible goldens carry-forward (tuning system at default 12-TET should preserve byte-equality if Phase 2.6a goldens re-baselined correctly) + 3 new microtonal goldens + RESEARCH §23 verdict.

**Phase 2.6b Gate 8b (5 invariants):**
1. 12-TET default state produces byte-identical output to Phase 2.6a goldens (HR-style identity arithmetic — tuning engine is no-op at 12-TET).
2. Scala/TUN file load (e.g., 19-EDO test file) produces expected pitch deviations on note-sequence rendering.
3. MTS-ESP polling integrates without RT-safety violation (pluginval Parameter thread safety + Background thread state PASS).
4. MPE pitch-bend ±100¢ produces expected per-note pitch tracking (no interaction with NE which is Phase 2.6c).
5. auval + pluginval-10 SUCCESS.

### Phase 2.6c — VST3 Note Expression FUNC-06 + FUNC-05 MPE Y/Z (Gate 8c)

**In scope:**

- **`modules/tuning/note-expression` shared module** — already linked at Stage 1; consume in processor.
- **JUCE-NE-PATCH precondition** — Phase 2.6c verify-phase asserts patch presence at `~/JUCE/`. If absent, BLOCK with explicit guidance per spike-findings reference `vst3-note-expression-dorico.md`.
- **`plugins/O-Contrabass/Source/PluginProcessor.{h,cpp}`** — VST3 raw-event queue drain at `processBlock` entry, before MPE event drain. Per-note NE-tuning offset accumulates into voice's `f_target` priority chain (NE wins per Q5).
- **`plugins/O-Contrabass/Source/BowedContrabassVoice.{h,cpp}`** — per-voice NE-tuning offset application (O-Lyrica `HarpSynthVoice` reference). FUNC-05 MPE Y → BOW_POSITION (β) per-note; MPE Z → BOW_PRESSURE additive per-note. Per-block smoothing.
- **Render harness** — NEW CLI mode `--note-expression` (synthetic NE event stream → output frequency deviation matches expected). NEW CLI mode `--mpe-yz` (channel-2 CC74/Pressure → BOW_POSITION/BOW_PRESSURE tracking).
- **Goldens** — NEW `note-expression.{wav.sha256, json, json.sha256}` + NEW `mpe-yz.{wav.sha256, json, json.sha256}`.
- **NO Dorico audition in Phase 2.6c** (Q6 LOCKED) — pluginval-10 + AU smoke + harness goldens ONLY. Full Dorico audition + COMPAT-02 verification owns Stage 4.
- **R41 atomic commit** — single commit lands source + 17+ re-baselined audible goldens carry-forward (NE/MPE-default-state should preserve byte-equality) + 2 new NE/MPE-YZ goldens + RESEARCH §24 verdict.

**Phase 2.6c Gate 8c (5 invariants):**
1. NE-default-state (no NE events) produces byte-identical output to Phase 2.6b goldens.
2. Synthetic NE-tuning event stream produces expected per-note pitch deviation.
3. MPE Y/Z per-note expression maps to BOW_POSITION / BOW_PRESSURE without RT-safety violation.
4. auval + pluginval-10 SUCCESS.
5. JUCE-NE-PATCH presence asserted (build-time precondition check).

**Phase 2.6c BLOCKING for Stage 2 close.** Stage 2 verify (full) runs ONLY after Gate 8c PASS.

---

## Out of scope (deferred elsewhere)

- **Phase 2.4-bis backlog (≈8 items)** — DSP-07 retune (priority-bumped post-Phase-2.5 sub-harm collapse 9.77e-05), DSP-09 vibrato transfer tune (peakDepthCents 9.53 → 7.95 post-tanh-port), DSP-08 breathingAudible metric, 3 v1.0 fallback cells (Phase 2.4a raucous corners), true Helmholtz slip-detection (period-heuristic v1.0 substitute landed at Phase 2.5), wolf-region suppression, bow-noise calibration, saturator-tail body-coupling deep characterisation (|Δ| 17.09 dB design-intent flag from Phase 2.5). **All deferred to v1.1 milestone** (Q2 LOCKED). v1.1 owns the entire Phase 2.4-bis backlog as a single follow-up cycle, not interleaved into Phase 2.6.
- **Dorico audition + COMPAT-02 verification** — Stage 4 (Q6 LOCKED). Phase 2.6c ships Note Expression wire-up + harness goldens only; Stage 4 polish cycle owns end-to-end Dorico verification.
- **Look-ahead limiter** — Phase 2.6a-bis if R38-style audition reveals harsh transients at high INFINITE_SUSTAIN; not in v1.0 baseline (Q4 LOCKED zero-latency feedforward only). 5 ms look-ahead would break PERF-03 nice-to-have, not must-bar.
- **Master saturator unification with in-loop saturator** (Q3 LOCKED) — alternative path that would unify master + in-loop to `4·tanh(x/4)`. NOT taken; ARCHITECTURE-spec'd `x − x³/3` for master is the locked path. NO 4th ARCHITECTURE amendment from Q3.
- **Chaos detector + softClampState** — v1.1 (carry-forward from Phase 2.4b R35 commit-body footnote; rev-9-bis carried forward "Phase 2.5/2.6"; rev-10 carried forward "v1.1"; rev-11 LOCKS v1.1).
- **Body Resonator / Bow Noise / Master Saturator / Master Limiter / Stereo Width shared-module extraction** (`modules/synthesis/{body-resonator, bow-noise, master-saturator, master-limiter, stereo-width}/`) — post-v1.0 refactor. v1.0 uses per-plugin `Source/DSP/*.h` (DispersionFilter / SchellengCalibration / SubHarmonicBias / BodyResonator / BowNoiseGenerator precedent).
- **HR-12 / HR-13 (any new hard rule for Phase 2.6)** — research-phase decides whether output-chain or microtonal integration warrants a new HR. Default: NO new HR (HR-1..HR-10 carry forward). Research-phase may surface a "no-double-counting microtonal+detune" HR or "atomic-pointer tuning-table swap" HR if needed.

---

## Approach Decisions (Q1–Q10 LOCKED)

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| Q1 | Phase 2.6 scope | **3 sub-cycles (2.6a / 2.6b / 2.6c)** with discrete Gates 8a / 8b / 8c | Six components × different risk profiles. Sub-phasing gives 3 atomic commits, easier bisect on failure, clean audit trail per sub-cycle. |
| Q2 | Phase 2.4-bis interleaving | **Defer entire Phase 2.4-bis backlog (≈8 items) to v1.1.** Stage 2 closes on Phase 2.6c verify | DSP-07 retune is non-blocking per CONTEXT line 220; output chain shifts downstream metrics → chasing tuning targets before Phase 2.6 lands risks chasing a moving target. |
| Q3 | Master saturator topology | **`polynomial x − x³/3`** (ARCHITECTURE §"Master Saturator" verbatim) | Master saturator is at output (post-body), not in feedback loop — different role, different curve shape musically defensible (cubic gives gentler knee than tanh for output-stage warmth). In-loop tanh from Phase 2.4c-bis stays untouched. NO 4th ARCHITECTURE amendment. |
| Q4 | Limiter design | **Zero-latency feedforward** (RMS sidechain → gain reduction, NO look-ahead). 3 ms attack / 50 ms release / −0.3 dBFS ceiling | Trades transient peak preservation for FUNC-02 long-form sustain priority + PERF-03 zero-latency. Phase 2.6a-bis can add 5 ms look-ahead if Stage 4 audition reveals harsh transients. |
| Q5 | Microtonal priority order | **Note Expression > MTS-ESP > Scala/TUN > MPE pitch-bend > 12-TET** (ARCHITECTURE §"Microtonal Tuning Engine" verbatim) | `scala-tuning-engine` module already implements this ladder. NO deviation. |
| Q6 | FUNC-06 Dorico verification scope | **Phase 2.6c ships wire-up + pluginval-10 + AU smoke + harness goldens ONLY.** Full Dorico audition + COMPAT-02 deferred to Stage 4 | O-Lyrica is the validated reference; pattern is known-good (spike-validated 2026-04-23). Stage 4 polish cycle owns Dorico end-to-end. |
| Q7 | End-of-Stage-2 ARCHITECTURE amendments | **Fold all 3 amendments (§"DC Blocker" + §"In-loop saturator" + §149/§509 size_scalar) into Phase 2.6 verify-phase as a SINGLE amendments task**, NOT a separate cycle | Already characterised; just needs scribing. Q3 keeps amendment count at 3 (no master-saturator-unification 4th amendment). |
| Q8 | Per-block tuning recompute frequency | **Tuning lookup at note-on** (resolve `f_target` once → push to voice). MPE pitch-bend + Note Expression deltas applied per-block via pitch-bend smoothing | Standard pattern in O-Lyrica. RT-safe; no per-sample tuning-table reads. |
| Q9 | DSP-05 + microtonal coexistence audit | **Confirm in research-phase (Phase 2.6b)**, not at discuss-phase | Will surface in Phase 2.6b research as a render-harness bit-stability test; per-string detune offset is applied per-string in waveguide; tuning-table offset is applied at note-on frequency resolution — composition needs explicit no-double-counting verification. |
| Q10 | Stage 2 closure pathway | **Stage 2 verify (full) runs as SEPARATE `/plugin-verify O-Contrabass 2-dsp` invocation after Phase 2.6c lands** | Phase 2.6c verify closes Phase 2.6c only; full Stage 2 verify is a distinct GSD cycle. Promotes all 24 requirements → `complete` or v1.1; lands 3 ARCHITECTURE amendments; logs Phase 2.4-bis backlog as v1.1 milestone. |

---

## Requirements Confirmed (Phase 2.6-relevant subsets of locked contracts)

- **FUNC-02 / Sustained-First Articulation** (must, currently pending stage-2): Phase 2.6a master saturator + limiter define the release-tail envelope shape post-body. Acceptance criterion "release on note-off produces a natural bow-lift tail" is exercised end-to-end at Phase 2.6a goldens + R38-style audition probe deferred to Stage 4 (or rolled into Phase 2.6a R39-pre audition if user opts).

- **FUNC-05 / MPE per-note pitch / pressure / slide** (should, currently pending stage-2): Phase 2.6c lands. Acceptance criterion "MPE controllers (Linnstrument, Seaboard) drive expression" verified via harness `--mpe-yz` golden at Phase 2.6c; full Linnstrument hardware audition is Stage 4.

- **FUNC-06 / VST3 Note Expression for Dorico** (must, currently pending stage-2): Phase 2.6c lands wire-up; Stage 4 verifies Dorico playback. Per Q6, Phase 2.6c does NOT itself verify Dorico — pluginval-10 + AU smoke + synthetic NE event golden only.

- **FUNC-07 / MTS-ESP and Scala/TUN tuning import** (should, currently pending stage-2): Phase 2.6b lands via `scala-tuning-engine` v2.1.0 module consume. Acceptance criterion "loads .scala / .tun / MTS-ESP host published tuning" verified via harness `--microtonal` golden at Phase 2.6b.

- **DSP-06 / Infinite Sustain** (must, currently pending stage-2): waveguide-side already proven from Phase 2.1a; Phase 2.6 does NOT modify INFINITE_SUSTAIN behavior. Status promotes to `complete` at Stage 2 full verify-phase based on accumulated evidence (Phase 2.1a 65 s sustain + Phase 2.4a 108-combo matrix + Phase 2.5 108/108 post-body matrix + Phase 2.6 14-golden re-baseline byte-determinism).

- **DSP-09 / Layered Expression** (must, currently partial): Phase 2.6c MPE Y/Z mapping completes the "intrinsic CC + dedicated vibrato + Expression Macro" stack. EXPRESSION_MACRO knob already lands at Phase 2.3 (R28 cycle); CC11/CC2/CC74 already routes via APVTS at Stage 1. Status promotes to `complete` at Stage 2 verify if MPE Y/Z verify PASS at Phase 2.6c.

- **DSP-10 / Slow expressive attack** (must, currently partial): subjective character bar; rolls into Stage 4 audition. Phase 2.6 master saturator + limiter must NOT introduce a hard transient at note-on that breaks the "long natural bow-on transient" character. Verified at Phase 2.6a R39-pre audition (subjective check on default-preset note-on).

- **PERF-01** (must, currently pending stage-2): RT-safety bar (no allocations / no locks / no file I/O in `processBlock`) holds across Phase 2.6 message-thread Scala/TUN file load + MTS-ESP polling + atomic tuning-table swap + audio-thread render. pluginval-10 fuzz + Parameter thread safety + Background thread state PASS at each sub-phase.

- **PERF-03** (nice, currently pending stage-2): zero algorithmic latency. Phase 2.6a feedforward limiter (Q4 LOCKED, NO look-ahead) preserves PERF-03. Master saturator + width are zero-delay waveshapers/encoders. Phase 2.6b/c tuning + NE event drain are upstream of audio path (no delay).

- **QUAL-01** (must, currently partial): no audio artifacts at normal ranges. Phase 2.6 adds master-stage processing — peak ≤ ceiling guarantee from limiter; click-free WIDTH automation; click-free MASTER_SAT_AMOUNT automation.

- **COMPAT-01** (must, currently partial): pluginval-10 PASS at each Phase 2.6 sub-phase atomic commit.

- **COMPAT-02** (must, currently pending stage-4): Dorico playback verification. Phase 2.6c wires the path; Stage 4 verifies (Q6 LOCKED).

---

## Constraints Identified

**Locked contracts (do NOT modify in this cycle):**

- All Phase 2.5 contracts (rev-10) carry forward verbatim. No re-litigation of body resonator (8-mode bank, 35 Hz HP dry path, Size/Damping/Mix smoothing) or bow noise (3-band BPF + period-heuristic slip bursts + bowEnergy envelope).
- All Phase 2.4c-bis contracts carry forward verbatim. In-loop saturator at `4·tanh(x/4)` stays UNTOUCHED in Phase 2.6. Master saturator at `x − x³/3` is a SEPARATE block at output (Q3).
- All Phase 2.4b contracts carry forward verbatim. HR-9 + HR-10 sub-harmonic bias short-circuit + friction module ABI preservation untouched.
- All Phase 2.4a contracts carry forward verbatim. SchellengCalibration polynomial + 108-combo matrix evidence untouched.
- All Phase 2.3 / 2.2 / 2.1 contracts carry forward verbatim.
- Stage 1 parameter-spec.md sha256 `77638e25…` carries forward through Phase 2.6b; **Phase 2.6a amends parameter-spec.md** (FIRST Stage-1 contract amendment in Stage 2) to add `MASTER_SAT_AMOUNT` + `LIMITER_CEILING_DB` and confirm `WIDTH` declaration.
- ARCHITECTURE.md carries forward through Phase 2.6 execute-phases; Phase 2.6 verify-phase folds 3 amendments (Q7) — §"DC Blocker" + §"In-loop saturator" + §149/§509 size_scalar.
- HR-1..HR-10 carry forward. NO new HR introduced by default (research-phase may add HR-12 / HR-13 if integration risk surfaces).

**v1.0 deliberate non-coverage (deferred to v1.1):**

- Wolf-region suppression (carry-forward from Phase 2.5 deviation; ARCHITECTURE §"Body Resonator" Mode #2 Q drop default-ON intent preserved for v1.1 reactivation).
- Authentic Arco wolf-coupling toggle (ARCHITECTURE Open Decision §3 deferral carry-forward).
- Chaos detector (architecture §457 line 476 lag-2 RMS; v1.0 relies on Schelleng F_max clamp + algebraic→tanh saturator + body bank L2 boundedness + master saturator + master limiter as layered defences).
- softClampState energy clamp (ROADMAP §Phase 2.4 deliverable; v1.0 master limiter covers the role).
- True Helmholtz slip-detection accessor (carry-forward from Phase 2.5; period-heuristic v1.0 substitute is the Phase 2.5 landed spec).
- Phase 2.4-bis backlog (8 items, Q2 LOCKED).

**Hardware audition not required at Phase 2.6:**

- Dorico audition (Q6 LOCKED → Stage 4)
- Linnstrument MPE hardware audition (FUNC-05 acceptance → Stage 4)
- Reference orchestral library A/B (FUNC-03 acceptance → Stage 4)

**R39 / R40 / R41 pre-flight tripwires (each sub-cycle):**

- 14 Phase 2.5 audible goldens reproduce byte-identical at HEAD descendant of `907a7c3` (R37 atomic) before sub-cycle execute begins.
- matrix-stability evidence golden `6db67707…` carries forward byte-identical (HR-style determinism on baseline state).
- Source-tree clean (`git status` clean against WIP scope).
- Saturator carry-forward verify (`grep -c "sat \* std::tanh"` returns 2 in WaveguideString.cpp from Phase 2.4c-bis port; Phase 2.6 does NOT modify in-loop saturator).
- BodyResonator + BowNoiseGenerator integration verify (`grep` audit returns Phase 2.5 expected hits).

---

## Risk Register (Phase 2.6 starting set; sub-cycle research-phases expand)

| # | Risk | Trigger | Mitigation | Status |
|---|------|---------|------------|--------|
| 1 | Master saturator + master limiter compound shift to all 14 audible goldens | Phase 2.6a re-baseline | All 14 re-baselined at R39 atomic; HR-style determinism check at re-render | Expected (re-baseline planned, not regression) |
| 2 | Limiter peak-overshoot at high INFINITE_SUSTAIN + drone parameter combos | Phase 2.6a output-chain harness | `--output-chain` mode tests peak ≤ ceiling at 108-combo matrix-style stress | Mitigated via dedicated test mode |
| 3 | PERF-03 zero-latency violation if look-ahead inadvertently introduced | Phase 2.6a limiter implementation | Q4 LOCKED zero-latency feedforward; verify `setLatencySamples()` unchanged from Phase 2.5 | Mitigated by Q4 lock |
| 4 | Tuning-table swap RT-safety violation (allocation / lock in `processBlock`) | Phase 2.6b TuningEngine wire-up | Atomic-pointer pattern (background thread parses → atomic swap → audio thread reads); pluginval-10 Parameter thread safety + Background thread state | Mitigated by O-Lyrica precedent |
| 5 | DSP-05 `perStringDetune` double-counts with MTS-ESP / Scala/TUN | Phase 2.6b coexistence | Q9 deferred to research-phase; render-harness bit-stability test detune-sweep-A.wav at default 12-TET vs Scala passthrough | Open (research-phase) |
| 6 | MPE pitch-bend + Note Expression interaction (both adjust `f_target`) | Phase 2.6c | Q5 priority order LOCKED (NE > MTS-ESP > Scala > MPE > 12-TET); only highest-priority source contributes | Mitigated by Q5 lock |
| 7 | JUCE-NE-PATCH absent at build time | Phase 2.6c precondition | Build-time assert / explicit guidance per spike-findings reference | Mitigated by explicit precondition |
| 8 | NE event drain racing with MIDI/MPE drain | Phase 2.6c | Drain order: NE first, then MPE, then MIDI legacy (per O-Lyrica pattern) | Mitigated by O-Lyrica precedent |
| 9 | parameter-spec.md amendment in Phase 2.6a (FIRST Stage-1 amendment in Stage 2) | Phase 2.6a | Add `MASTER_SAT_AMOUNT` + `LIMITER_CEILING_DB`; confirm `WIDTH` + `OUTPUT_LEVEL` declared; bump parameter-spec.md sha256 with explicit amendment note | Expected (planned amendment, audit-trailed) |
| 10 | 3 ARCHITECTURE amendments at Phase 2.6 verify scribing error | Phase 2.6 verify-phase | Q7 LOCKED single-task amendment; verify-phase has explicit amendments-task with author-and-review pattern | Mitigated by Q7 lock |
| 11 | v1.1 Phase 2.4-bis backlog scope drift (items added beyond 8) | Phase 2.6 sub-cycles | Q2 LOCKED; new items surfacing during Phase 2.6 explicitly route to v1.1 backlog at sub-cycle execute-phase, not interleaved | Mitigated by Q2 lock |
| 12 | Stage 2 verify scope creep (all 24 requirements in single cycle) | Stage 2 full verify | Q10 LOCKED separate `/plugin-verify`; goal-backward analysis covers requirements promotion + 3 amendments + v1.1 backlog logging | Mitigated by Q10 lock |
| 13 | Phase 2.6c Dorico audition pressure to interleave at Phase 2.6c | Q6 deviation pressure | Q6 LOCKED Stage-4 deferral; Phase 2.6c Gate 8c does NOT include Dorico audition | Mitigated by Q6 lock |

---

## Open Questions (research-phase scope)

- **§22 (Phase 2.6a research):** master saturator pre-gain calibration (ARCHITECTURE soft-clip at ~−3 dBFS; what input amplitude triggers soft-clip onset under Phase 2.5 voice output peak amplitudes?); limiter implementation choice (`juce::dsp::Compressor` derivative vs custom envelope follower per `juce::dsp::BallisticsFilter`); stereo width topology (M/S encode formula sanity vs `juce::dsp::Matrix2x2` operations); R39 task breakdown (R39-pre / R39a / R39b / R39c / R39d / R39e / R39f / R39 atomic / R39-backfill).
- **§23 (Phase 2.6b research):** `scala-tuning-engine` v2.1.0 API surface (review `modules/tuning/scala-tuning-engine/` API; identify expected client callsites in PluginProcessor + voice); MTS-ESP client polling thread (background or message? RT-safety analysis); DSP-05 + MTS-ESP coexistence test design (Q9); Scala/TUN file-load thread (background; UI thread file picker); harness `--microtonal` mode design.
- **§24 (Phase 2.6c research):** `modules/tuning/note-expression` API surface; JUCE-NE-PATCH presence assertion mechanism (CMake check vs runtime check); per-voice NE-tuning offset accumulation pattern (O-Lyrica `HarpSynthVoice` line-by-line review); FUNC-05 MPE Y/Z mapping pattern (additive vs multiplicative on BOW_POSITION / BOW_PRESSURE); harness `--note-expression` + `--mpe-yz` mode design.
- **Cross-phase:** ARCHITECTURE-amendments authoring scope for Phase 2.6 verify (Q7 — exact section text proposals for all 3 amendments based on Phase 2.5 + Phase 2.4c-bis + Phase 2.1a evidence); Stage 2 full verify-phase scope (Q10 — goal-backward analysis template against 24 requirements); R39 / R40 / R41 atomic-commit-sequence numbering convention (continuation from R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → R37).

---

## Cross-Cycle Carry-Forward (LOCKED — verbatim from rev-10)

- HR-1..HR-10 in effect (HR-11 retired at Phase 2.4c-bis).
- 14 audible goldens (13 Phase 2.5 + saturator-tail-comparison) reproduce byte-identical at HEAD descendant of R37 atomic `907a7c3`.
- matrix-stability evidence golden `6db67707…` reproduces byte-identical (carry-forward from Phase 2.4a R34b).
- In-loop saturator at `4·tanh(x/4)` (Phase 2.4c-bis port) UNTOUCHED in Phase 2.6.
- BodyResonator (8-mode static-Q bank) + BowNoiseGenerator (3-band BPF + period-heuristic slip bursts) UNTOUCHED in Phase 2.6.
- SchellengCalibration polynomial (Phase 2.4a) + SubHarmonicBias (Phase 2.4b HR-9 + HR-10) + DispersionFilter (Phase 2.1c) UNTOUCHED in Phase 2.6.
- Stage-1 parameter-spec.md sha256 `77638e25…` carries forward through Phase 2.6a-pre; **Phase 2.6a amends** (`MASTER_SAT_AMOUNT` + `LIMITER_CEILING_DB`).

---

## Next Phase

Ready for: **research phase** — Phase 2.6 umbrella research (or Phase 2.6a research if sub-cycles are dispatched independently). Research-phase produces RESEARCH §22 (Phase 2.6a output chain) initially; §23 (Phase 2.6b microtonal) and §24 (Phase 2.6c Note Expression) append at later sub-cycle research-phases per umbrella plan.

---

## rev-11.b — Phase 2.6b sub-cycle amendment (Microtonal engine + MPE pitch-bend)

**Date:** 2026-05-01
**Phase:** 2.6b discuss — LOCKED
**Atomic target:** R40
**Supersedes:** none (sub-cycle amendment to umbrella rev-11)
**Predecessor sub-cycle:** Phase 2.6a-bis verify (Gate 8a PASS-with-design-intent-flag at HEAD descendant of R39 atomic `74b3f83e6d10162b3c28ef966aa79d4adf8e62f0`; 14 audible goldens at post-Phase-2.6a sha256s; parameter-spec.md sha `ae956e94…`).

### Live-state discrepancies resolved against rev-11 §22.6b

- **D1 — MPE legacy mode pitchbend range:** rev-11 §22.6b mentioned `pitchbendRange=2, channelRange={1,17}`. **Live code** at `PluginProcessor.cpp:139` is `enableLegacyMode(/*pitchbendRange*/ 24, juce::Range<int>(1, 16))`. **LOCKED: keep ±24** semitones (Stage 1 ground truth; matches BRIEF per-string detune ±1200¢ headroom; `juce::Range<int>(1, 16)` is exclusive upper, functionally equivalent to {1,17} expression). rev-11 §22.6b text drift; this amendment is authoritative.
- **D2 — TuningEngine MTS-ESP path:** module v2.1.0 declares `Mode::MTSESP // placeholder - future implementation` and `connectMTSClient()` but ships no real MTS-ESP-Client SDK linkage. **LOCKED: ship as no-op stub in v1.0** (Q11 Option B below).
- **D3 — `OUTPUT_LEVEL` vs `OUTPUT_GAIN` parameter ID drift:** BRIEF says "Output Level"; Phase 2.6a OUTPUT_GAIN voice→processor relocation used `OUTPUT_GAIN`. NOT a Phase 2.6b concern; flagged for Stage 2 full verify-phase audit.

### Q11–Q20 LOCKED

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| Q11 | MTS-ESP scope at Phase 2.6b | **Option B — no-op stub.** TuningEngine MTS-ESP path returns 12-TET frequencies; APVTS Choice value "MTS-ESP" preserved (Stage 1 contract intact); v1.1 lights up real MTS-ESP-Client SDK | Module already a placeholder. Linking SDK at v1.0 = scope expansion + new licensing surface. Stage 1 contract preserved. Risk #5 (DSP-05 + MTS-ESP coexistence) becomes degenerate — stub is identity. |
| Q12 | Scala/TUN file-load UX scope at Phase 2.6b | **Option A — message-thread file-picker stub** invoked from harness `--microtonal` flag with explicit path; Stage 3 GUI replaces stub with proper Editor file picker | Allows Phase 2.6b to render `microtonal-scala.wav` golden via harness; Stage 3 replaces stub without touching DSP wire. |
| Q13 | DSP-05 + microtonal coexistence audit (Q9 resolution) | **Option A — bit-equality at TUNING_SYSTEM=12-TET** on detune-sweep-A.wav (TuningEngine identity at 12-TET); Scala passthrough at 19-EDO test file = expected algebraic delta only (per-string detune applied multiplicatively after TuningEngine lookup; provable no-double-counting since TuningEngine returns absolute Hz, detune is cents offset on top) | HR-style determinism on default state (Phase 2.5 + Phase 2.6a precedent); double-counting math is provable, golden bit-equality is the strict bar. |
| Q14 | Phase 2.6b parameter-spec.md amendment | **NO amendment.** Post-Phase-2.6a sha `ae956e94…` carries forward unchanged through Phase 2.6b. TUNING_SYSTEM + REFERENCE_PITCH already declared at Stage 1 (`PluginProcessor.cpp:118-121`); Phase 2.6b is pure wire-up, no parameter additions | Q7 LOCKED keeps total Stage-2 amendments at 3. Phase 2.6a was the FIRST + LAST Stage-1 contract amendment in Stage 2. |
| Q15 | JUCE-NE-PATCH precondition assertion at Phase 2.6b | **Option A — NO assertion at 2.6b.** MPE drain in 2.6b uses standard `juce::MPESynthesiser::handleMidiEvent`; raw-event drain (NE-only) lights up at Phase 2.6c. Patch presence assertion is 2.6c-scoped (per rev-11 §22.6c) | Phase 2.6b has zero NE event handling; asserting patch at 2.6b would be premature gating. |
| Q16 | HR-12 — tuning-table swap RT-safety contract | **NEW HR-12 LOCKED:** "Tuning-table updates use only `std::atomic<double>` per-slot writes (TuningEngine `frequencyTable[128]`) or atomic-pointer swap; NO mutex / lock / file I/O / allocation on audio thread." Module already enforces via `std::array<std::atomic<double>, 128>`; promoting to plugin HR makes the contract durable across v1.1 custom TuningEngine extension | Module already enforces; HR-12 promotes inheritance to plugin-level invariant. Pluginval-10 thread-safety probes verify. |
| Q17 | Per-block tuning recompute model (Q8 carry-forward refinement) | **Option A — note-on resolution only.** Voice resolves `f_target = TuningEngine::getFrequency(midiNote) · 2^(perStringDetune/1200)` once at note-on. Per-block: MPE pitch-bend multiplicative `2^(bend·24/1200)` applied via existing pitch-bend smoothing. Voice does NOT re-poll TuningEngine per-block; live retuning takes effect at next note-on only | RT-safe; matches O-Lyrica precedent. Live retuning mid-sustain is v1.1 "should". Bowed bass long-form sustain note duration = seconds, not minutes; re-trigger covers most cases. |
| Q18 | R40 task breakdown | **Option A — 6-task** (R40-pre → R40a → R40b → R40c → R40d → R40e → R40 atomic → R40-backfill chore) per breakdown below | Compressed vs Phase 2.6a 9-task — fewer NEW source files (no new headers; only M to PluginProcessor + voice + harness). |
| Q19 | Phase 2.6b Gate 8b 5-invariant scorecard | **LOCKED** as below | Carries rev-11 §22.6b LOCKED + Q11–Q17 refinements. |
| Q20 | Phase 2.6b risk register additions | **6 NEW risks LOCKED** (#27–#32 below); cumulative starting set 32 entries (13 rev-11 carry-forward + 13 Phase 2.6a + 6 Phase 2.6b) | Tuning-engine integration surfaces 6 new failure modes; all mitigated except #29 (DSP-05 coexistence — Q13 audit closes). |

### R40 6-task breakdown LOCKED

- **R40-pre** (tripwire — re-runs at execute-phase entry):
  1. `git status` clean against WIP scope.
  2. 14 audible goldens reproduce byte-identical at HEAD descendant of R39 atomic `74b3f83e…`.
  3. `output-chain.wav` golden reproduces byte-identical at sha `b5fc1d60…`.
  4. Saturator carry-forward verify: `grep -c "sat \* std::tanh" plugins/O-Contrabass/Source/WaveguideString.cpp` returns 2.
  5. parameter-spec.md sha matches `ae956e94…` (post-Phase-2.6a anchor).
  6. Module link audit: `grep "scala-tuning-engine" plugins/O-Contrabass/CMakeLists.txt` returns 4 source-list lines + 1 include-dir line; `grep "note-expression" plugins/O-Contrabass/CMakeLists.txt` returns 1 `ouaricon_add_module` line.
  7. Pre-edit greps: `grep -n "TuningEngine" plugins/O-Contrabass/Source/PluginProcessor.{h,cpp}` returns 0 hits (Phase 2.6b adds them).
- **R40a** — `Source/PluginProcessor.{h,cpp}` M (~30 LOC NEW): TuningEngine member declaration + construction in PluginProcessor ctor (NOT prepareToPlay — Risk #27); APVTS reads for TUNING_SYSTEM (Choice → `TuningEngine::Mode`) + REFERENCE_PITCH (Float → `TuningEngine::setReferencePitch`); MTS-ESP stub path returns 12-TET frequencies (Q11 Option B); Scala/TUN file-picker stub (Q12 Option A) — `loadScalaFile(File)` callable from harness `--microtonal` flag.
- **R40b** — `Source/BowedContrabassVoice.{h,cpp}` M (~10 LOC M): note-on `f_target` resolution refactor — replace direct `juce::MidiMessage::getMidiNoteInHertz` (or equivalent baseFreq computation) with `TuningEngine::getFrequency(midiNote)` lookup; preserve per-string detune as `2^(perStringDetune/1200)` multiplicative on top (Q13 no-double-counting).
- **R40c** — `tests/render-harness/main.cpp` M (~150 LOC NEW): `--microtonal` mode (sweep TUNING_SYSTEM Choice; load test Scala/TUN file via `--scl` flag; verify output frequency matches expected) + `--mpe-pitch-bend` mode (channel-2 pitch-bend ±24 semitones per-note → output frequency tracking). Both modes emit JSON measurement summary alongside .wav.
- **R40d** — golden artefacts:
  - 3 NEW: `microtonal-12tet.wav` (12-TET baseline; should match note-sequence.wav re-baselined at Phase 2.6a) + `microtonal-scala.wav` (19-EDO test file from `tests/fixtures/test-19edo.scl`) + `microtonal-mpe.wav` (channel-2 ±24 pitch-bend sweep).
  - 14 audible goldens carry-forward bit-identical at TUNING_SYSTEM=12-TET default (Gate 8b invariant 1).
  - 3-trial bit-stability per RESEARCH §22 R39e precedent.
  - `reproduce-goldens.sh` 14→17 entries.
  - DSP-05 coexistence test: re-render `detune-sweep-A.wav` under TUNING_SYSTEM=12-TET; bit-equality required at sha256 from Phase 2.6a re-baseline.
- **R40e** — regression bar: 17-entry `reproduce-goldens.sh` PASS + 3-file source audit hook reports EXACTLY {PluginProcessor.{h,cpp} M + BowedContrabassVoice.{h,cpp} M + tests/render-harness/main.cpp M} + 0 CMake edits + 0 parameter-spec amendment + saturator carry-forward + Body+Noise integration + setLatencySamples invariant. `auval -v aumu OCbs OuDv` SUCCESS + `pluginval --strictness-level 10` SUCCESS full battery + Background thread state + Parameter thread safety + Buffer fuzz + DSP-05 coexistence test PASS.
- **R40 atomic commit** — single atomic per Phase 2.4c-bis R36-bis / Phase 2.5 R37 / Phase 2.6a R39 precedent: 3 source M + 3 NEW goldens + reproduce-goldens.sh M + RESEARCH §23 + CONTEXT rev-11.b + PLAN rev-14 (Phase 2.6b plan-phase author) + SUMMARY/VERIFICATION/STATUS planning artefacts.
- **R40-backfill chore** — sha propagation per R34/R35/R36/R36-bis/R37/R39 precedent.

### Phase 2.6b Gate 8b 5-invariant scorecard LOCKED

1. **12-TET default state byte-identical** to Phase 2.6a 14 audible goldens at post-Phase-2.6a sha256s (HR-style determinism — TuningEngine identity at 12-TET; HR-12 contract verifies).
2. **Scala/TUN file load** → expected pitch deviation on 19-EDO test file (algebraic match within ±0.5¢ tolerance via harness `--microtonal` mode JSON measurements).
3. **MTS-ESP stub** (Q11 Option B) → returns 12-TET behavior; pluginval-10 Parameter thread safety + Background thread state PASS; no audible distinction from 12-TET path.
4. **MPE pitch-bend ±24 semitones** per-note tracking on channel-2 (legacy mode) — render-harness `--mpe-pitch-bend` golden reproduces byte-identical 3-trial.
5. **auval AU + pluginval-10 SUCCESS** + DSP-05 coexistence audit PASS (Q13 bit-equality on detune-sweep-A.wav at 12-TET).

### Phase 2.6b risk register (6 NEW; cumulative 32 entries)

| # | Risk | Trigger | Mitigation | Status |
|---|------|---------|------------|--------|
| 27 | TuningEngine instantiation allocates on construction; if constructed in `prepareToPlay` could allocate at host re-prepare | Phase 2.6b R40a wire-up | Construct in PluginProcessor ctor (one-shot, message thread); never re-construct | Mitigated by R40a placement |
| 28 | `loadScalaFile` blocks message thread on large .scl parse | Phase 2.6b R40a Scala/TUN load | Document message-thread blocking acceptable for file load (UI thread file picker → synchronous parse → atomic frequencyTable populate); harness invokes from main thread; Stage 3 Editor file picker invokes from message thread post-`AsyncUpdater` | Mitigated by atomic frequencyTable design + thread-aware invocation |
| 29 | DSP-05 detune coexistence — voice double-counts if TuningEngine path also applies detune | Phase 2.6b R40b voice refactor | Strict separation: TuningEngine returns absolute Hz from frequencyTable[128] (cents handled internally per Mode); voice multiplies by `2^(perStringDetune/1200)` only at note-on; bit-equality test on detune-sweep-A.wav at 12-TET | Mitigated by Q13 audit + R40d coexistence test |
| 30 | MPE legacy pitchbend range mismatch with ARCHITECTURE §288 (rev-11 §22.6b mentioned ±2; live code uses ±24) | Live state discrepancy D1 | LOCKED ±24 (Stage 1 ground truth); rev-11 §22.6b text drift documented in this amendment | Resolved by D1 lock |
| 31 | MTS-ESP stub returns sentinel value mistaken for real frequency by voice | Phase 2.6b R40a stub implementation | Stub explicitly returns same as 12-TET path (`getFrequency(midiNote)` falls through to TwelveTET branch); no sentinel | Mitigated by Q11 Option B stub design |
| 32 | 14 audible goldens DRIFT at TUNING_SYSTEM=12-TET if TuningEngine init order wrong (e.g., TuningEngine constructed AFTER voice's `prepareToPlay` reads `f_target`) | Phase 2.6b R40a/R40b ordering | R40-pre tripwire step 7 grep + R40d step 1 (14 goldens reproduce byte-identical post-wire-up at 12-TET default); init order: TuningEngine member declared BEFORE voice member in PluginProcessor.h ⇒ ctor init order is TuningEngine first | Mitigated by R40-pre + R40d invariant |

### Open Questions for Phase 2.6b research-phase (RESEARCH §23)

- TuningEngine API surface review: confirm `getFrequency(midiNote)` is the correct lookup site (vs `getFrequencyForMidiNote` or `frequencyTable[midiNote].load()` direct access pattern); confirm `setMode(TwelveTET | Custom | MTSESP)` signature; confirm thread-safety of `loadScalaFile` parse-then-swap pattern.
- MTS-ESP stub implementation site: does TuningEngine internally fall through `Mode::MTSESP` to `Mode::TwelveTET`, or does the plugin-side code branch?
- 19-EDO test Scala file location: `tests/fixtures/test-19edo.scl` exists in modules/tuning/scala-tuning-engine/snippets? If not, author at R40d.
- Per-block MPE pitch-bend smoothing — is existing `juce::SmoothedValue` adequate, or does `±24 semitones` bend require resampling-aware interpolation? (Likely SmoothedValue is fine; verify in research.)
- Voice's existing baseFreq computation site — `BowedContrabassVoice.cpp` line audit needed to identify the exact replacement point at R40b.
- Atomic-pointer vs `std::atomic<double>[128]` swap pattern (HR-12 enforcement detail).

### Cross-Cycle Carry-Forward (Phase 2.6b additions)

- HR-12 LOCKED at Phase 2.6b discuss; effective from Phase 2.6b execute onwards; promoted to all v1.1 follow-up cycles.
- 14 audible goldens carry-forward at post-Phase-2.6a sha256s through Phase 2.6b execute-phase entry; Phase 2.6b R40 atomic preserves bit-equality at TUNING_SYSTEM=12-TET default (Gate 8b invariant #1).
- `output-chain.wav` golden carry-forward at post-Phase-2.6a-bis sha `b5fc1d60…`.
- parameter-spec.md sha `ae956e94…` carries forward UNCHANGED through Phase 2.6b (Q14 lock).
- ARCHITECTURE.md carries forward through Phase 2.6b execute-phase; 3 amendments still fold into Phase 2.6 verify-phase as single task (Q7 LOCKED).
- HR-1..HR-10 + HR-12 in effect; HR-11 retired; HR-13 not introduced at discuss.
- Atomic-commit sequence: R7 → R15 → R20 → R26 → R33 → R34 → R35 → R36 → R36-bis → R37 → R39 → R39-bis (pending land) → **R40** (Phase 2.6b atomic) → R41 (Phase 2.6c) → Stage 2 verify amendments commit.

### Next Phase

Ready for: **research phase** — Phase 2.6b RESEARCH §23 author. Scope LOCKED in this amendment + Q11–Q20 + R40 6-task breakdown + Gate 8b 5-invariant scorecard + 6 NEW risks + 6 open research questions above. RESEARCH §22 (Phase 2.6a) + §22-bis (Phase 2.6a-bis) closed; §23 is fresh append at later sub-cycle research-phase. §24 (Phase 2.6c Note Expression) deferred to its own sub-cycle.
