---
plugin: O-GranulatorInstrument
stage: 3
status: stage_3_phase_3_1_discuss_complete
current_phase: 3-gui/3.1
gsd_phase: discuss_complete
last_updated: 2026-11-10
complexity_score: 5.0
complexity_tier: 6
staged_implementation: true
orchestration_mode: true
next_action: research_3_1 (Stage 3 Phase 3.1 layout + basic controls; HANDS-ON-2.1–2.6 run once at 3.3 verify, D39)
next_stage: 3
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
  requirements: sha256:fc13b0027bd3d43160729edd04d76d0f221259219bc8aaca95d7c5728980dabb   # Phase 2.6 verify: FUNC-06 / FUNC-07 / QUAL-02 pending -> partial; PERF-01 partial -> complete (AL26)
  parameter_spec_draft: sha256:3c4918748a4375940684a22ed6850a3184bdd96df53d152a8900d5f0e4260796
  architecture: sha256:8c44e54f86994b1b84e1f8a516a95215d654d552aaa9e4b406eff8a8ed241f79
  roadmap: sha256:34fe684374ac5167e609f6fa6b9046548fc00e89f6bd52444cf6d1eac5a2867e   # 2.5 split into 2.5a / 2.5b (D26)
---

# O-GranulatorInstrument Status

## Current Position

Stage: 3 of 4 (GUI) — **Phase 3.1 discuss ✓** 2026-11-10 (`stages/3-gui/CONTEXT-3.1.md`, D37–D42); next: research 3.1. Stage 2 (DSP) — **all 6 phases verified ⚠️ PARTIAL** (2.6 verified 2026-11-09, `stages/2-dsp/VERIFICATION-2.6.md`): every automated gate re-run green (harness 357/0, 159 controls fire, 0 NOT FIRED; parity 168/0 + GV 2/0; renders-2.4 + legacy byte-identical; param-dump identical; drift 0/13 + NC-a/NC-b; C25m 27.85 % ≤ 40, C25tm 6.55 % ≤ 12; auval PASS; pluginval 10 VST3 + AU SUCCESS with SC on and off; X2 0). HANDS-ON-2.1–2.6 (human) deferred post-Stage-3 (D14 override). Next: Stage 3 Phase 3.1.
Stage 1: VERIFIED
Status: Stage 1 executed (74606bb3): 0.1.0 VST3 + AU + Standalone built; 261 params (176/70/15), dest 126 / src 18 goldens; harness 55 PASS / 0 FAIL + golden and spec negative controls; pluginval 10 VST3 ×3 + AU SUCCESS with SC active (2 ch) and gate variant inactive (0 ch), auval PASS [1, 2] [2, 2] (frozen); D9 contingency did not fire; shipping build restored + installed. See stages/1-foundation/SUMMARY.md. Verified 2026-11-07 (stages/1-foundation/VERIFICATION.md): independent re-run on the installed shipping build — goldens + param-dump byte-identical, harness 55/0, auval PASS [1, 2] [2, 2], pluginval 10 VST3 + AU SUCCESS (2 ch in); COMPAT-01 complete. Next: Stage 2 discuss (Phase 2.1 fork).
Progress: [##########..........] 50%

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
7. Stage 2 Phase 2.4: discuss ✓ 2026-11-08 (`stages/2-dsp/CONTEXT-2.4.md`, D22–D25). research ✓ 2026-11-08 (`stages/2-dsp/RESEARCH-2.4.md`; wetKnee adopted by user). plan ✓ 2026-11-08 (`stages/2-dsp/PLAN-2.4.md`, 21 tasks / 8 waves, PD1–PD21). execute ✓ 2026-11-08 (`SUMMARY-2.4.md`; C1 3d71bebe + C2; harness 179/0, parity 133/0, C25w 26.92 %; S6 → local κ, CK2 / CK3 INFO). verify ⚠️ partial 2026-11-08 (`VERIFICATION-2.4.md`; automated green incl. C25w 26.59 % ≤ 40; DSP-04/05 complete; HANDS-ON-2.1–2.4 deferred).
8. Stage 2 Phase 2.5a (split D26): discuss ✓ 2026-11-08 (`CONTEXT-2.5a.md`, D26–D28), research ✓ (`RESEARCH-2.5a.md`), plan ✓ 2026-11-09 (`PLAN-2.5a.md`), execute ✓ 2026-11-09 (`SUMMARY-2.5a.md`), verify ⚠️ partial 2026-11-09 (`VERIFICATION-2.5a.md`; automated green incl. C25m 26.81 % ≤ 40; HANDS-ON-2.1–2.5a deferred). 
9. Stage 2 Phase 2.5b: discuss ✓ 2026-11-09 (`CONTEXT-2.5b.md`, D29–D32), research ✓ 2026-11-09 (`RESEARCH-2.5b.md`), plan ✓ (`PLAN-2.5b.md`), execute ✓ (`SUMMARY-2.5b.md`), verify ⚠️ partial 2026-11-09 (`VERIFICATION-2.5b.md`; automated green incl. C25m 27.62 % ≤ 40; HANDS-ON-2.1–2.5b deferred).
10. Stage 2 Phase 2.6 (Record-in, state / relink, factory sources): discuss ✓ 2026-11-09 (`CONTEXT-2.6.md`, D33–D36), research ✓ 2026-11-09 (`RESEARCH-2.6.md`), plan ✓ 2026-11-09 (`PLAN-2.6.md`, 20 tasks / 8 waves), execute ✓ 2026-11-09 (`SUMMARY-2.6.md`; harness 357/0, parity 168/0, C25m 29.26 %; HANDS-ON-2.6 deferred), verify ⚠️ partial 2026-11-09 (`VERIFICATION-2.6.md`; automated green incl. C25m 27.85 % ≤ 40, pluginval SC on/off; PERF-01 → complete; HANDS-ON-2.1–2.6 deferred). **Stage 2 verified (automated).** Next: `/plugin-discuss O-GranulatorInstrument 3-gui` (Phase 3.1).
11. Stage 3 Phase 3.1 (layout + basic controls): discuss ✓ 2026-11-10 (`stages/3-gui/CONTEXT-3.1.md`): D37 all 34 native functions registered, slot actions live (load / relink / reset / embed / slot state), telemetry = what exists + well-formed empties; D38 DevLoader removed; D39 HANDS-ON-2.1–2.6 (79 rows) run once at 3.3 verify; D40 v1 locked, defects only, gear accepted as placed; D41 port parent 1.2.0 value entry (knob name → type a value) in 3.1; D42 `tests/i18n-states.json` lands in 3.1. Next: `/plugin-research O-GranulatorInstrument 3-gui` (Phase 3.1).

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
