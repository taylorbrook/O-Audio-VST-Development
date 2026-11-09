---
plugin: O-GranulatorInstrument
stage: 2
status: stage_2_phase_2_4_verified_partial_hands_on_pending
current_phase: 2-dsp/2.4
gsd_phase: verify_complete
last_updated: 2026-11-08
complexity_score: 5.0
complexity_tier: 6
staged_implementation: true
orchestration_mode: true
next_action: discuss_2_5 (HANDS-ON-2.1 + 2.2 + 2.3 + 2.4 deferred by user to post-Stage-3; D14 overridden 2026-11-08)
next_stage: 2
ready_for_implementation: true
blocked_on: none (fork pin 82406fce, D11)
latest_mockup_version: 1
mockup_finalized: true
finalized_version: 1
stage_0_status: ui_design_complete
ui_scaffolding_phase_complete: true
ui_scaffolding_timestamp: "2026-11-07"
parameter_spec_locked: true
brief_updated_from_mockup: true
mockup_version_synced: 1
brief_update_timestamp: "2026-11-07T19:40:17Z"
contract_checksums:
  parameter_spec: sha256:b6bc70e3a9dcd2611f241452b893f8318956cb946b1ac0694fe3957837ca751b
  parameter_spec_body: sha256:c5553e0ccda2c7f5320a1891ba20c5a92913e089ab1fa1e47b0e33fa7c543191
  brief: sha256:74e7f8f24468ed9658aa8533733f45da24c61ad116d6ae47cd5c2af0e8524722
  requirements: sha256:75e405e4b928d10b2be93c6aeae60162175d0ebc45bd4c0b6b95765fbf14195b   # Phase 2.4 verify: DSP-04 + DSP-05 pending -> complete; DSP-01 complete, 12 stage-2 reqs partial
  parameter_spec_draft: sha256:3c4918748a4375940684a22ed6850a3184bdd96df53d152a8900d5f0e4260796
  architecture: sha256:8c44e54f86994b1b84e1f8a516a95215d654d552aaa9e4b406eff8a8ed241f79
  roadmap: sha256:ca57fdb8f1fbeaa8abad8d40be8e37a84e8a96491e31cb786c1fcc54d738caf0
---

# O-GranulatorInstrument Status

## Current Position

Stage: 2 of 4 (DSP) — Phase 2.1 verified 2026-11-07 (`stages/2-dsp/VERIFICATION-2.1.md`): ⚠️ PARTIAL — every automated gate re-run green in verify (harness 88/0, 22 controls fire; param-dump byte-identical; fork drift 0 with NC-a/NC-b re-made, rc 1; auval PASS; pluginval 10 VST3 + AU SUCCESS, 2 ch in; console exclusion 0 refs). HANDS-ON-2.1 (human, 13 rows) pending — closes FUNC-01–04, DSP-02, DSP-07, PERF-01–03, QUAL-01 partials (REQUIREMENTS). Phase 2.2 discuss ✓ 2026-11-08 (`stages/2-dsp/CONTEXT-2.2.md`, D14–D17). Phase 2.2 research ✓ 2026-11-08 (`stages/2-dsp/RESEARCH-2.2.md`). Phase 2.2 plan ✓ 2026-11-08 (`stages/2-dsp/PLAN-2.2.md`, 15 tasks / 6 waves). Phase 2.2 execute ✓ 2026-11-08 (`stages/2-dsp/SUMMARY-2.2.md`): D14 OVERRIDDEN by user ("skip ahead I'll test everything once UI is in place") — HANDS-ON-2.1 + HANDS-ON-2.2 deferred to after Stage 3. Harness 117/0 (every control fires), parity 41/0 + GV1 (DSP-01 null −123…−132 dB vs parent @ 82406fce; PS1–4, SC1 864/864, SC3), param-dump byte-identical, drift 0 (NC-a/NC-b fire), RB1 pinned = 2.1 7/7, renders-2.2.sha256 re-baselined; kVoiceGainMeasuredDb −6.0 → 0.02 (GV0 +0.0212 dB); auval PASS; pluginval 10 VST3×2 + AU SUCCESS; X1/X2 exclusion 0; C22 INFO median 14.36 %. Phase 2.2 verified 2026-11-08 (`stages/2-dsp/VERIFICATION-2.2.md`): ⚠️ PARTIAL — independent re-run all green (harness 117/0, 51 controls fire; fresh parent build at 82406fce + `--parity` 41/0, GV 2/0, PN −110.9…−120.2 dB at HEAD (SUMMARY's −123…−132 were pre-T12, stale); renders-2.2 re-dump byte-identical, pinned = 2.1 7/7; param-dump identical; drift 0, NC-a/NC-b rc 1; X1 0 (ctrl 92), X2 0 (ctrl 76); auval PASS; pluginval 10 VST3 + AU SUCCESS; Standalone relinked (was stale, pre-2.2) + alive; C22 INFO 14.45 %). DSP-01 → complete; FUNC-09 → partial. HANDS-ON-2.1 + 2.2 deferred to post-Stage-3. Phase 2.3 discuss ✓ 2026-11-08 (`stages/2-dsp/CONTEXT-2.3.md`, D18–D21). Phase 2.3 research ✓ 2026-11-08 (`stages/2-dsp/RESEARCH-2.3.md`): pin GrainFx carries parent D10 make-down (latch svfMake + fitted-gain band row); −40 dBFS parity material makes crush/BP rows vacuous → `wetSafetyOff` hook + per-row matPeak; crush nulls need bitwise pre-FX reads (PF0 ≤ −130 dB precondition, stop-and-ask); clamp rows not constructible at 48 k → unit + 44.1 k census; D19 projection ≈ 30–31 % vs 36 % + calibration-loop witness; D20 proxy refined (density 40 / size 150 ms, ≈ 72 live). Phase 2.3 plan ✓ 2026-11-08 (`stages/2-dsp/PLAN-2.3.md`, 14 tasks / 7 waves, PD1–PD15): C23t proxy frozen; calibration-loop witness (ref frozen in T2, `tests/golden/cpu-calib-2.3.txt`); FV2–FV4 census in; PF0 ≤ −130 dB + gain band 1e-4 (legacySvfMake control); lever 1 (Hermite) never auto, levers 2→3 auto then ask; readSet23 fixes eg21 N2 + N2s; PFC8 crush 100 % at 0.4 peak; targets harness 117→150, parity 41→99, --cpu23 2 verdict rows. Phase 2.3 execute ✓ 2026-11-08 (`stages/2-dsp/SUMMARY-2.3.md`): per-grain FX latched at spawn (`GiFxLatch.h`, parent planFx/assignFxLatch incl. svfMake) + gather → FX → scatter kernel with the SVF NaN kill; ControlGrid binds the 10 FX rows × 4 (re-hashed). Harness 150/0 (FU 15, FN 3, FR 1, FV 7, FB 4, FZ 2, FA 1; 0 NOT FIRED); FR1 14/14 = renders-2.2 (true bypass), RB1 7/7; --parity 99/0 (PF0 −149.3 dB, g₀ 0.9977; 48 PF −152.1…−126.1 dB, band ≤ 1e-7; PFc1–6 fire incl. PFc6 make-down; PS5–7 pass), GV 2/0, PN residuals identical to baseline; --cpu23 D19 C23w 26.08 % ≤ 36 % PASS, D20 C23t 5.41 % ≤ 12 % PASS (live 72), witness valid (ref_ns 40 523 708 frozen in tests/golden/cpu-calib-2.3.txt), 0.050 %/grain, C23s −0.44 pt → levers not fired; param-dump identical; drift 0/13 (NC-a/NC-b fire); X1/X2 0; auval PASS; pluginval 10 VST3×2 + AU SUCCESS; Standalone rebuilt + alive. HANDS-ON-2.3 written (9 rows), deferred with 2.1/2.2. Phase 2.3 verified 2026-11-08 (`stages/2-dsp/VERIFICATION-2.3.md`): ⚠️ PARTIAL — independent re-run all green at 9fc9289c (harness 150/0, 70 controls fire, 0 NOT FIRED; fresh parent build + `--parity` 99/0, GV 2/0, PF0 −149.3 dB, 48 PF ≤ −126.1 dB, band ≤ 1e-7, PFc1–6 fire, PN unchanged −110.9…−120.2 dB; renders-2.2 re-dump byte-identical; param-dump identical; drift 0/13 + NC-a/NC-b rc 1; X1 0 (ctrl 91), X2 0 (ctrl 76); auval PASS; pluginval 10 VST3 + AU SUCCESS; Standalone relinked + alive; `--cpu23` C23w 26.01 % ≤ 36, C23t 5.38 % ≤ 12, witness valid). DSP-03 → partial (automated arm complete; "audibly" = HANDS-ON-2.3). FZ1 stress bus peak 1.73 (no ceiling until 2.4) — carry into the 2.4 ceiling row. Phase 2.4 discuss ✓ 2026-11-08 (`stages/2-dsp/CONTEXT-2.4.md`, D22–D25): D22 FeedbackChain + ReverbUnit/knee leaf nulls vs parent (−90 dB, gain band) + ARCH Algorithm §3 law gates for the novel tape loop (peak bound, freeze/reset/fb-0 hashes, 1-vs-16-voice runaway guard, head-clamp telemetry); D23 ceiling = parent D9 knee ≤ +6 dBFS pre-output_gain (FZ1 carried); D24 freeze/feedback λ ramp 20 ms, click gates vs same-motion + hard-stop control; D25 final PERF-02 worst case + 4 chains + reverb ≤ 40 %, reset dual-read fade INFO. Phase 2.4 research ✓ 2026-11-08 (`stages/2-dsp/RESEARCH-2.4.md`): USER DECISION — adopt the parent `ogi::wetKnee` (identity ≤ −6 dBFS) in place of our pure kWetCeil·tanh; renders re-baselined once, with TB1 byte-identity under a `legacyWetTanh` hook first. No new parent hooks (reverb via mix 100; chain via tapProbe + per-32 getFbDelayForTest, frozen geometry); chain knob must be unity (`chainKnobParity` hook for the null); head clamp g_a = kCell + 8 = 40 (not 16); new `MipTape::settleRun` at wrap/park (stale children past hi); head start/stop uses a 20 ms runRamp, parked only at λ_eff == 0; Reset = the 2.1 rebuild crossfade + loader `requestReset` + swap-waits-for-park; AGC 1 ms attack makes the 0.7 bound structural only at sat ≥ 6.2 % → writer clamp recommended (plan decision); D25 projection ≈ 28–30 % vs 40 %. Phase 2.4 plan ✓ 2026-11-08 (`stages/2-dsp/PLAN-2.4.md`, 21 tasks / 8 waves, PD1–PD21): knee adopted alone in W1 — legacyWetTanh proves renders-2.1 7/7 + renders-2.2 14/14 byte-identical (RB1/FR1 under legacy), renders-2.4.sha256 pinned once on the shipping path in a separate commit C1, then TB0 is the bypass invariant (bypass-hash checkpoint after every engine task); writer clamp DECIDED option (a): |r| ≤ kAgcCeil 0.7 on the writer's copy + a convex envelope (exact bound, FL1s sat-0 row, rClampOff control); whole cell written at the grid tick (kFbDelay == kCell); the run condition's voice term requires an enabled + audible stream; every pending tape waits for a parked head; requestReset / resetSync reuse the 2.1 rebuild crossfade; PB rows de-duplicated (sat 0/50/100, diffusion 25/50/100) + PBd delay row; CK4 control = same-motion freeze pulse; MS1 bitwise vs an offline mip build; readSet24 162/99 + FL7; targets harness 150→152 (W1)→181, parity 99→133, --cpu25 2 verdict rows (C25w ≤ 40 %, C25t ≤ 12 %). Phase 2.4 execute ✓ 2026-11-08 (`stages/2-dsp/SUMMARY-2.4.md`; C1 3d71bebe knee adoption, attributed: legacy == renders-2.2 14/14 + renders-2.1 7/7, renders-2.4 pinned, 12/14 moved (V3 log hashes do not see the output), PN −143.0…−151.3 dB). Harness 179/0, 0 NOT FIRED (CK2 / CK3 INFO by user decision at S6: local κ adopted; their hard controls cannot fire by construction → HANDS-ON-2.4 rows 3–4); --parity 133/0 (PB0 tap −152.0 dB g_PB 1.0000000 after the enable-at-T_e alignment; PB1–12 −116.3…−141.8 dB, band ≤ 1e-8; PR1–12 + PRk −96.8…−112.0 dB, band ≤ 3e-7), GV 2/0; --cpu25 C25w 26.92 % ≤ 40 PASS, C25t 5.54 % ≤ 12 PASS (C25s +1.03 FLAG, INFO), levers not fired; FL1 / FL1s exact bound, MS1 bitwise (settle span 2048, clipped to the flushed span), HC1 0 violations / 4.01 M segments; param-dump identical; drift 0/13 + NC-a / NC-b; X1 / X2 0; auval PASS; pluginval 10 VST3 ×2 + AU SUCCESS; Standalone rebuilt + alive. HANDS-ON-2.4 written (11 rows), deferred. Phase 2.4 verified 2026-11-08 (`stages/2-dsp/VERIFICATION-2.4.md`): ⚠️ PARTIAL — independent re-run all green at 3d68135b (harness 179/0, 99 controls fire, 0 NOT FIRED, CK2/CK3 INFO; fresh parent build + `--parity` 133/0, GV 2/0, PB0 −152.0 dB, PB −116.3…−141.8, PR −96.8…−112.0, PN −143.0…−151.3; shipping dump == renders-2.4, legacy == renders-2.2, legacy pinned == renders-2.1 7/7; param-dump identical; drift 0/13 + NC-a/NC-b rc 1; X1 0 (ctrl 92), X2 0 (ctrl 76); auval PASS; pluginval 10 VST3 + AU SUCCESS; Standalone relinked + alive; `--cpu25` C25w 26.59 % ≤ 40, C25t 5.51 % ≤ 12, C25s +0.27 (SUMMARY's FLAG did not reproduce)). DSP-04 + DSP-05 → complete. Next: `/plugin-discuss O-GranulatorInstrument 2-dsp` (Phase 2.5).
Stage 1: VERIFIED
Status: Stage 1 executed (74606bb3): 0.1.0 VST3 + AU + Standalone built; 261 params (176/70/15), dest 126 / src 18 goldens; harness 55 PASS / 0 FAIL + golden and spec negative controls; pluginval 10 VST3 ×3 + AU SUCCESS with SC active (2 ch) and gate variant inactive (0 ch), auval PASS [1, 2] [2, 2] (frozen); D9 contingency did not fire; shipping build restored + installed. See stages/1-foundation/SUMMARY.md. Verified 2026-11-07 (stages/1-foundation/VERIFICATION.md): independent re-run on the installed shipping build — goldens + param-dump byte-identical, harness 55/0, auval PASS [1, 2] [2, 2], pluginval 10 VST3 + AU SUCCESS (2 ch in); COMPAT-01 complete. Next: Stage 2 discuss (Phase 2.1 fork).
Progress: [######..............] 30%

## Completed So Far

**Ideation:** ✓ Complete (BRIEF, REQUIREMENTS, parameter-spec-draft with 261 params)

**Stage 0:** ✓ Complete. Research & Planning: ARCHITECTURE.md and ROADMAP.md written (complexity 5.0).
- Plugin type: Synth (sample-based polyphonic granular, MIDI/MPE/NE, sidechain Record-in)
- Professional examples researched: 5 (Granulator III, Pigments, Clouds/Beads, Thor, Logic EVOC/AU side-chain)
- JUCE / modules: juce_audio_formats, juce_dsp, juce_audio_utils, WebView; grain-engine 1.1.0, pitch-collections 1.0.0, reverb-unit 1.2.0, note-expression 1.1.1, webview-drop-streaming 1.0.0, preset-manager
- Decisions D1–D7 made (see ARCHITECTURE.md "Stage 0 Decisions"):
  - D1 fork the leaves, converge later
  - D2 30 s per slot / 2.88 M-frame cap
  - D3 grain_limit ≤ 256, 40 % budget, onset thinning
  - D4 newest-voice rule
  - D5 Dorico NE in v1
  - D6 Record-in decided by signal presence
  - D7 1× record head, convex write
- Strategy: Staged implementation (Stage 2 × 6 phases, Stage 3 × 3 phases)

## Next Steps

1. ✓ UI mockup v1 finalized 2026-11-07 (Phase 5.5); Phase B scaffolding generated: `v1-ui.html`, `v1-i18n.js`, `v1-PluginEditor.{h,cpp}` (261 relays, 34 native fns, sidechain-aware Record-in telemetry), `v1-CMakeLists.txt`, `v1-integration-checklist.md`, `v1-i18n-states.json`. UI gates pre-verified on a scratch tree (check-i18n / fr / zh / check-ui-labels / boot-all-uis all clean).
2. ✓ `parameter-spec.md` LOCKED (Stage 1 contract): 261 params, Design Sync S1–S11 applied, 126-entry destination order and 18-entry source order frozen (append-only). One description-level draft conflict recorded in its "Draft vs mockup" section (`polyphony` steal wording) — resolved Stage 1 discuss D10: keep mockup wording.
3. Stage 1: Foundation — discuss ✓ (`stages/1-foundation/CONTEXT.md`), research ✓ (`stages/1-foundation/RESEARCH.md`), plan ✓ (`stages/1-foundation/PLAN.md`); execute ✓ (`SUMMARY.md`, 74606bb3), verify ✓ (`VERIFICATION.md`). Next: `/plugin-discuss O-GranulatorInstrument 2-dsp` (Phase 2.1 fork pin).
4. Stage 2 Phase 2.1: discuss ✓ (`stages/2-dsp/CONTEXT-2.1.md`). Fork pin = O-Granulator `82406fce` (D11, supersedes the 3.3-close pin). research ✓ (`RESEARCH-2.1.md`), plan ✓ (`PLAN-2.1.md`), execute ✓ (`SUMMARY-2.1.md`), verify ⚠️ partial (`VERIFICATION-2.1.md`, automated green; HANDS-ON-2.1 pending). Next: run HANDS-ON-2.1, then `/plugin-discuss O-GranulatorInstrument 2-dsp` (Phase 2.2).
5. Stage 2 Phase 2.2: discuss ✓ (`stages/2-dsp/CONTEXT-2.2.md`): D14 HANDS-ON-2.1 gates 2.2 execute only; D15 DSP-01 = derandomised −90 dB null vs parent @ 82406fce + statistical arm on latched grain fields; D16 kVoiceGainDb from measured RMS match to parent default (±0.5 dB) + ear row; D17 scale state (customScale + degreeMasks hex) engine API + `<Engine>` save/restore, harness-only. CPU INFO only (verdict 2.3). research ✓ (`RESEARCH-2.2.md`): parent-at-pin parity via out-of-tree extract+build (n1/run.sh precedent, `--parity` harness mode); null rows need parent `freezeNoMinSpray` + integer Δ multiple of 2^level; stats arm n=4000/side KS+variance; parent scale state = relaxed atomics + SpinLock try-lock copy, `<CustomScale>`/`<DegreeMasks>` siblings of `<Engine>`; D16 parent default s1-only, mix 50 % → target ≈ 0 dB vs −6 dB placeholder; muted streams keep demand (confirmed); window-0 bitwise needs the parent closed-form Hann fast path. Planner decisions: D16 reference at mix 100 % vs 50 %; coherence input on a stationary tape; try-lock vs no-locks; steady-state cap kills when pairs > cap. plan ✓ (`PLAN-2.2.md`, 15 tasks / 6 waves, PD1–PD16): user decisions 2026-11-08 — D16 reference = parent mix 100 % (expect kVoiceGainDb ≈ 0 dB, measured + ear-trim split), coherence option (b) relative velocity per (k,s), parent non-blocking try-lock accepted for the custom-scale publish. Level/pan ramps stay 20 ms (tree), enable/mute/solo 10 ms. execute ✓ 2026-11-08 (`SUMMARY-2.2.md`; D14 overridden by user, hands-on deferred to post-Stage-3). verify ⚠️ partial 2026-11-08 (`VERIFICATION-2.2.md`; automated green, hands-on deferred). Next: Phase 2.3.
6. Stage 2 Phase 2.3: discuss ✓ 2026-11-08 (`stages/2-dsp/CONTEXT-2.3.md`): D18 DSP-03 = FX null vs parent @ 82406fce (−90 dB, each FX + chained, crush_prob 0/100) + ported parent GrainFx law rows + stats arm (filter_rand/ring_rand/crush_prob) + no-bleed-on-slot-reuse; D19 CPU worst case (16×4 saturating 256, all FX) PASS/FAIL ≤ 36 % now, ≤ 40 % with chains+reverb at 2.4; D20 typical proxy ≤ 12 % now, factory presets Stage 4; D21 levers 1–3 auto in ARCH order, grain_limit 256→128 fallback asks. FX-off renders must stay byte-identical to renders-2.2. HANDS-ON-2.3 deferred post-Stage-3. research ✓ (`RESEARCH-2.3.md`). plan ✓ (`PLAN-2.3.md`, 14 tasks / 7 waves). execute ✓ 2026-11-08 (`SUMMARY-2.3.md`: harness 150/0, parity 99/0, D19 26.08 % / D20 5.41 % PASS, levers not fired). verify ⚠️ partial 2026-11-08 (`VERIFICATION-2.3.md`; automated green incl. C23w 26.01 % / C23t 5.38 %; HANDS-ON-2.1/2.2/2.3 deferred). Next: Phase 2.4.
7. Stage 2 Phase 2.4: discuss ✓ 2026-11-08 (`stages/2-dsp/CONTEXT-2.4.md`, D22–D25). research ✓ 2026-11-08 (`stages/2-dsp/RESEARCH-2.4.md`; wetKnee adopted by user). plan ✓ 2026-11-08 (`stages/2-dsp/PLAN-2.4.md`, 21 tasks / 8 waves, PD1–PD21). execute ✓ 2026-11-08 (`SUMMARY-2.4.md`; C1 3d71bebe + C2; harness 179/0, parity 133/0, C25w 26.92 %; S6 → local κ, CK2 / CK3 INFO). verify ⚠️ partial 2026-11-08 (`VERIFICATION-2.4.md`; automated green incl. C25w 26.59 % ≤ 40; DSP-04/05 complete; HANDS-ON-2.1–2.4 deferred). Next: Phase 2.5 — `/plugin-discuss O-GranulatorInstrument 2-dsp`.

## Context to Preserve

- Architecture: `plugins/O-GranulatorInstrument/.planning/research/ARCHITECTURE.md`
- Roadmap: `plugins/O-GranulatorInstrument/.planning/ROADMAP.md`
- Stage 0 context: `plugins/O-GranulatorInstrument/.planning/stages/0-ideation/CONTEXT.md`
- Stage 1 context: `plugins/O-GranulatorInstrument/.planning/stages/1-foundation/CONTEXT.md`
- Stage 1 plan: `plugins/O-GranulatorInstrument/.planning/stages/1-foundation/PLAN.md` (authoritative for execute)
- Stage 1 research: `plugins/O-GranulatorInstrument/.planning/stages/1-foundation/RESEARCH.md` (open: uiLanguage on state root not <Engine>; skip D10 header note; note-expression registry 1.1.1 vs disk 1.1.2)
- Complexity 5.0 (raw 27.0), tier 6, phased
- Parent: O-Granulator @ 3043b72a (fork pin moves to the 3.3-closing commit)
- Design Sync changes (no param IDs added or removed; still 261):
  - grain_limit 32/64/128/256
  - pitch_bend_range = MPE master only (members use RPN 0)
  - feedback = convex write amount
  - ±48 st clamp
  - <Slots> state keys
  - factory sources embedded as BinaryData

## Files Created
- plugins/O-GranulatorInstrument/.planning/research/ARCHITECTURE.md
- plugins/O-GranulatorInstrument/.planning/ROADMAP.md
- plugins/O-GranulatorInstrument/.planning/stages/0-ideation/CONTEXT.md
- plugins/O-GranulatorInstrument/.planning/BRIEF.md (ideation)
- plugins/O-GranulatorInstrument/.planning/REQUIREMENTS.md (ideation)
- plugins/O-GranulatorInstrument/.planning/parameter-spec-draft.md (ideation, 2026-11-07)
- plugins/O-GranulatorInstrument/.planning/mockups/v1-ui.yaml (UI mockup v1, 2026-11-07)
- plugins/O-GranulatorInstrument/.planning/mockups/v1-ui-test.html (UI mockup v1 browser test, 2026-11-07; gitignored)
- plugins/O-GranulatorInstrument/.planning/parameter-spec.md (LOCKED, 2026-11-07)
- plugins/O-GranulatorInstrument/.planning/mockups/v1-ui.html, v1-i18n.js, v1-PluginEditor.h, v1-PluginEditor.cpp, v1-CMakeLists.txt, v1-integration-checklist.md, v1-i18n-states.json (ui-finalization, 2026-11-07)
