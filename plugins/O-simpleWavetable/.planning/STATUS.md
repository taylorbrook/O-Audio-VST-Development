---
plugin: O-simpleWavetable
stage: 4
status: stage_4_discuss_complete
last_updated: 2026-10-06 23:59:30
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: plugin_research_stage_4
next_stage: 4
ready_for_implementation: true
latest_mockup_version: 1
mockup_finalized: true
finalized_version: 1
stage_0_status: ui_design_complete
ui_scaffolding_phase_complete: true
parameter_count: 21
current_phase: research
brief_updated_from_mockup: true
mockup_version_synced: 1
brief_update_timestamp: "2026-10-06T02:25:53Z"
contract_checksums:
  brief: sha256:9cee315a17720edd08c2e635537eb1af1c6e131bcff10fcb8090370688f49e9c
  parameter_spec: sha256:e77ae9445099369a1a40daef12e0a4be46bfcca37a4937eae069aa56efa2ab2b
  architecture: sha256:b7ddf56f37f57e68f6f2945d1ccb88a2388edc1a499b9e3d0c88dec18c38150b
  roadmap: sha256:f657e02a22d6dee0e4d8f2a08ec8736c6479a4eebed7e14dc78c014bf71f32b5
---

# O-simpleWavetable Status

## Current Position

Stage: 4 of 4 (Polish) — discuss ✓ 2026-10-06. **Next: `/plugin-research O-simpleWavetable 4-polish`.**
Status: Research & Planning complete; UI mockup v1 finalized with implementation scaffolding; parameter-spec.md locked (21 parameters). Ready for Stage 1.
Progress: [################....] 80%

## Completed So Far

**Ideation:** ✓ Complete (BRIEF.md, REQUIREMENTS.md, parameter-spec-draft.md)

**Stage 0:** ✓ Research & Planning complete — ARCHITECTURE.md and ROADMAP.md documented (Complexity 5.0)
- Plugin type: Synth (pedagogical wavetable), Tier 5, DEEP research
- Professional/reference designs: PPG Wave, Waldorf Microwave, Serum, Vital, Hive/Zebra, Pigments
- JUCE modules: juce_audio_basics, juce_audio_formats, juce_audio_processors, juce_audio_utils, juce_dsp, juce_gui_extra, juce_core
- DSP feasibility verified numerically (strict per-octave mips + linear read: aliases at least 74 dB down, −121 dB at C8)
- 18 open questions resolved (see ARCHITECTURE.md summary table); 6 non-blocking user questions listed
- Strategy: phased (Stage 2: 4 phases, Stage 3: 3 phases)

**UI mockup v1:** ✓ Finalized + Phase B scaffolding (mockups/v1-ui.html, v1-i18n.js, v1-PluginEditor.{h,cpp}, v1-CMakeLists.txt, v1-integration-checklist.md, v1-i18n-states.json); parameter-spec.md locked from ARCHITECTURE.md §Parameter Mapping

**Stage 1 execute:** ✓ 2026-10-05 — Silent 16-voice synth shell (OSiW, 0.1.0), 21-param APVTS, `uiLanguage` + strip-on-load `IMPORTED_BANK` state stub. VST3/AU/Standalone build clean; auval (targeted) + pluginval VST3/AU strictness 10 pass; state-check 9/9 PASS. See stages/1-foundation/SUMMARY.md.

**Stage 1 verify:** ✓ 2026-10-05 — VERIFIED (stages/1-foundation/VERIFICATION.md). Independently re-run on the installed binaries: auval (2 benign skew warnings), pluginval VST3+AU strictness 10, pedalboard (21 params + Bypass, instrument, silent MIDI render, state round trip). COMPAT-01 complete. Task 14 DAW smoke left as a non-blocking human checklist.

**Stage 2 execute Part 1 (2.1+2.2):** ✓ 2026-10-05 — five 32-frame built-in banks (strict 11-level mips, buildMillis ~26 ms Debug), WtVoice (band-limited linear read, D-A floor L >= 1, latched/interp frames, mid-rise quantizer, squared velocity, +/-2 st bend), Poly 16 / true-legato Mono, seeded output stage. Gates (Debug, out-of-repo): bank-check ALL PASS (incl. G-NEG); dsp-check ALL PASS — G-Q2-C8 -114.3 dB, G-Q2-SWEEP -74.9 dB, G-Q2-PULSE -67.0 dB equal-RMS (NAMED EXCEPTION D-B), G-PITCH 0.0009 c, G-VEL -11.905 dB; --alloc-check 0 allocs; state-check P6' pass. Orchestrator fix: JUCE 8.0.15 `Synthesiser::findVoiceToSteal` mallocs on every steal (`Array::clear` frees) -> `Source/WtSynthesiser.h` alloc-free port of the same policy. auval (targeted) + pluginval VST3/AU strictness 10 pass; pedalboard on installed VST3: C4 -0.0000 c, Imported = exact 0, C7 Saw32 band-limit Off -21.3 / On -114.1 dB. **Task 11 listening checkpoint: signed off by Taylor 2026-10-05 (resumed via `/plugin-execute O-simpleWavetable 2-dsp`; no tone/feel notes given).**

**Stage 2 execute Part 2 (2.3+2.4):** ✓ 2026-10-06 — PositionLfo (free / PPQ tempo, splitmix64 S&H), per-voice mod env + 2 ms one-pole smoother (D-N: one-pole shipped, click 1.414), frozen-cycle crossfader (**raised-cosine law — deviation from RESEARCH's linear ramp, ARCHITECTURE amendment 11**) + D-K hysteresis, REG-01 reaper + D-C `audioHeldBank` amendment, worker importer + import API, IMPORTED_BANK flac16/pcm16gz persistence. Gates (Debug, out-of-repo): bank 13/13, dsp 20/20 (+alloc 0), mod 18/18, import 34/34, state 11/11 — QUAL-03 exact ≤ 6e-8, ratio worst 1.31; D-C neg control deref 1 → 0 with fix; soak held ≤ 2, live 5+1; COMPAT-03 9/9 bit-identical; FUNC-04 memcmp equal + neg fails; G-Q2 unchanged after D-K (−114.3 / −74.9 dB). auval + pluginval VST3/AU strictness 10 pass; pedalboard ALL PASS incl. IMPORTED_BANK round trip through the real VST3 raw_state. See stages/2-dsp/SUMMARY.md.

**Stage 2 verify:** ⚠ PARTIAL 2026-10-06 (stages/2-dsp/VERIFICATION.md).
- **Re-run independently and green:**
  - all 5 offline drivers, plus `--alloc-check` 0
  - auval, and pluginval VST3/AU at strictness 10
  - a new pedalboard probe, 27/27
- **Requirements:** 17 of 18 stage-2 complete.
- **Critic review:** 0 blockers.
- **Confirmed on the installed VST3:**
  - W1: a voice steal hard-stops, |Δy| 8.7× steady state
  - W2: a Mono retrigger during release steps velGain, 0.489 → 0.042 in one sample
- **To do before Stage 3:** the W5 ARCHITECTURE §17 doc fix (Stage 3 must read the bank through `getImportedBankSnapshot()`).

**Stage 2 gap closure execute:** ✓ 2026-10-06 — W1 2 ms hard-stop tail (steal, Poly↔Mono, all-sound-off), W2 3 ms velGain ramp on a sounding retrigger, W5 ARCHITECTURE row 17 / §12 / Threading / Amendments 12–13. New gates G-STEAL 1.076× (neg 4.99×), G-SWITCH-TAIL 0.656× (neg 20.0×), G-RETRIG-VEL 0.589× (neg 35.8×); all 5 drivers + alloc 0 green, goldens unchanged. auval + pluginval VST3/AU s10 pass; installed-VST3 repros W1 1.084× (was 8.7×), W2 0.964× (was ~10×). See SUMMARY.md §Gap closure.

**Stage 2 re-verify:** ✓ VERIFIED 2026-10-06 (stages/2-dsp/VERIFICATION.md §Re-verification). All re-run independently:
- the 5 drivers in a fresh Debug tree: ALL PASS, state 11/11, `--alloc-check` 0. New gates G-STEAL 1.076×, G-SWITCH-TAIL 0.656×, G-RETRIG-VEL 0.589×, with their negative controls firing. Goldens unchanged.
- auval, and pluginval VST3/AU strictness 10
- a new pedalboard probe on the installed VST3, 13/13: W1 0.86× (was 8.7×), W2 0.74× (was ~10×), Poly↔Mono tails 0.32× / 0.45×
- QUAL-01 complete, so all 18 stage-2 requirements are complete. W3, W4 and notes 3–8 are deferred to Stage 4.

**Stage 3 execute Part 1 (3.1+3.2):** ✓ 2026-10-06 — WebView "Wavetable Field Guide" (v1 mockup byte-identical; 21 relays/attachments, 8 natives, 11 resource branches, 10 binary-data files), `applyFactoryPreset` (D-Y single pass, output_level untouched), `importFromBase64` (D-Z), display state `dispNote`/`dispHz`/`displayFs` + `WtVoice::lastNote` (+5/−0), `CycleRenderer` (FFT 11, ref bins 1..1023 = D-U), `VizPayload.h` (quantized-payload FNV hash, D-P), editor bank/cycle push (D-T order, index watch). Gates (Debug, out-of-repo): 0 warnings; bank/dsp/mod/import ALL PASS, state 11/11, `--alloc-check` 0, R-GOLD clean (timing/soak counts only); viz-check ALL PASS — G-LESSON 5 ids + reapply + unknown (NC FAILS as designed), G-VIZ-EXACT 90/90 worst cycle 3.8e-5 (tol 2e-4) / bar 0.005 dB (tol 0.02) vs direct DFT, CEIL On/Off, SILENT idle-quiet (+NC), NOTE poly/mono, IMPORTED import/P8/empty, ALLOC (a) 0 audio-thread / (b) 0 idle, N1 ×348 / N2 ×60 / N3 ×1886 / N4 ×2491; TIME (Debug) cycleUpdate 813 µs, 5.2 KB; bankUpdate N=256 38 ms, 238 KB. UI: check-i18n PASS, fr CLEAN, zh 142 entries 0 findings, boot-all-uis --strict-tips clean 0 DEAD / 0 late. Release: auval SUCCEEDED (2 benign skew warnings), pluginval VST3+AU strictness 10 SUCCESS (editor tests on); Standalone rebuilt + `strings` fresh. Deviations: S9 grep excludes the generated `modules/` dir; N4 split into liveness (N4-LIVE) + dead-payload NC; Formant liveness anchored at its strongest bar (D-U consequence).

**Stage 3 Task 12 visual checkpoint:** ✓ signed off by Taylor 2026-10-06 (resumed via `/plugin-execute O-simpleWavetable 3-gui`; no visual/behaviour notes given). Part 2 (3.3) started.

**Stage 3 execute Part 2 (3.3):** ✓ 2026-10-06 — Task 14 import-path gates in viz-check (no `Source/` change): G-DROP (a) base64 drop → 11/11 mip levels memcmp-equal to `importFromMemory`, (b) cap+1 → `tooLarge`, (c) `unreadable`, (d) `../../x\n.wav` → `x.wav`; G-DROP-N1 (`MemoryBlock::toBase64Encoding`) FAILS as designed; G-IMPORT-ERR tooShort keeps bank/filename (P8), `importToVar` keys exact, codes ⊆ {tooLarge, tooShort, unreadable, unsupported}. Task 15 five UI gates green on the real tree, no fixes (check-i18n 0 FAIL; fr CLEAN; zh 142 / 0; check-ui-labels 8 states; boot-all-uis 0 DEAD / 0 late). Task 16: Debug 0 warnings, 6 drivers + viz ALL PASS, `--alloc-check` 0, R-GOLD clean (timing/soak counts only); auval SUCCEEDED (2 benign skew), pluginval VST3+AU s10 SUCCESS, 0 `ForTesting` symbols shipped. **Task 17 Standalone hands-on + Task 18 Logic AU pass: all pass (Taylor)** — closes Stage 2 deferred save/reopen-with-Imported and Stage 1 AU-under-Instruments. ARCHITECTURE Stage 3 Amendments 14–18. See stages/3-gui/SUMMARY.md.

**Stage 3 verify:** ✓ VERIFIED 2026-10-06 (stages/3-gui/VERIFICATION.md). Re-run independently at `f4eea85a`:
- a fresh Debug tree with 0 warnings: 6 drivers + viz ALL PASS, state 11/11, `--alloc-check` 0, 0 assertions. R-GOLD equals the Stage 2 reference.
- **a binary null test:** the installed Stage 3 VST3 vs an out-of-tree build of `850df9b9` (Stage 2) in pedalboard, 6/6 cases bit-exact.
- a bridge cross-check: 21/21 params, 8/8 natives, 5/5 lesson ids.
- all five UI gates green.
- auval, and pluginval VST3/AU strictness 10 with the editor tests on.

UI-01..06 and PERF-03 are complete. The critic review found 0 blockers, 5 warnings and 13 notes. **Taylor chose VERIFIED, with W1–W5 and N1–N13 carried into Stage 4.**

## Phase Progress

### Stage 1: Foundation
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-10-05 | |
| research | ✓ | 2026-10-05 | |
| plan | ✓ | 2026-10-05 | |
| execute | ✓ | 2026-10-05 | |
| verify | ✓ | 2026-10-05 | |

Stage 0 open conflicts resolved (user, 2026-10-05): silence+prompt for empty Imported; keep ±2 st bend; mid-rise quantizer; reject <2048-sample imports. See stages/1-foundation/CONTEXT.md.

### Stage 2: DSP
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-10-05 | |
| research | ✓ | 2026-10-05 | |
| plan | ✓ | 2026-10-05 | |
| execute | ✓ | 2026-10-06 | |
| verify | ✓ (partial → gap closure → re-verified) | 2026-10-06 | |

Stage 2 decisions (user, 2026-10-05): checkpoint after 2.1+2.2 for listening; velocity squared; mono true legato; Interp-Off step click accepted. See stages/2-dsp/CONTEXT.md.

Stage 2 research (2026-10-05): stages/2-dsp/RESEARCH.md. Decisions pending for plan: D-A (96 kHz band-limit level floor ≥1), D-B (narrow-pulse QUAL-02 metric), D-C (REG-01 reaper amendment `audioHeldBank` — verbatim port is a UAF with the crossfader).

Stage 2 plan (2026-10-05): stages/2-dsp/PLAN.md, 24 tasks in two parts. User signed off D-A (level floor 1), D-B (narrow pulses on the equal-RMS metric, named exception), D-C (audioHeldBank amendment + seq_cst). Planner resolved D-D…D-O. Execute stops at Task 11 (listening checkpoint) after the Part 1 commit; re-run `/plugin-execute O-simpleWavetable 2-dsp` to resume Part 2.

### Stage 3: GUI
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-10-06 | |
| research | ✓ | 2026-10-06 | |
| plan | ✓ | 2026-10-06 | |
| execute | ✓ (Part 1 + visual sign-off + Part 2 + hands-on) | 2026-10-06 | |
| verify | ✓ | 2026-10-06 | |

Stage 3 decisions (user, 2026-10-06): 3.1+3.2 → build/install → visual checkpoint → 3.3; lesson buttons wired live now (`applyFactoryPreset`, mockup recipes; FUNC-08 bank stays Stage 4); hands-on in Standalone + one DAW. See stages/3-gui/CONTEXT.md.

Stage 3 plan (2026-10-06): stages/3-gui/PLAN.md, 20 tasks in two parts (Part 1 = 3.1+3.2, Tasks 1–12; Part 2 = 3.3, Tasks 13–20). Planner D-P…D-Z. User sign-offs: D-U dB ref = bins 1..1023; D-Y single-pass lesson apply; D-Z `importFromBase64` in processor (+ G-DROP); DAW pass in Logic. Execute stops at Task 12 (visual checkpoint); re-run `/plugin-execute O-simpleWavetable 3-gui` to resume Part 2. ARCHITECTURE checksum refreshed (W5 gap-closure edit, 850df9b9).

### Stage 4: Polish
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-10-06 | |
| research | | | |
| plan | | | |
| execute | | | |
| verify | | | |

Stage 4 decisions (user, 2026-10-06): full preset-manager module + browser panel, factory = Init + 8 §A9, lessons reconciled to §A9 (Alias Demo → Drive); fix all W (S2 W3/W4, S3 W1–W5) + triaged N; Windows = CI `workflow_dispatch` validate-only build + pluginval (no hands-on); QUAL-04 measured gates + by-ear sign-off; v1.0.0 + CHANGELOG + CODE_REVIEW, no tag. See stages/4-polish/CONTEXT.md.

## Next Steps

1. `/plugin-research O-simpleWavetable 4-polish`.
2. Stage 4 entry items:
   - Stage 3 critic W1–W5: the uiReady counter race, the 96 MB drop freeze, trackpad wheel stepping, the stale import error, and a stuck UI-held note on editor close
   - Stage 3 critic N1–N13 (stages/3-gui/VERIFICATION.md §Issues Found)
3. Stage 4 backlog:
   - Stage 2 W3, W4 and notes 3–8
   - FUNC-08 / §A9 lesson recipes
   - PERF-02, COMPAT-02 (Windows) and QUAL-04
   - VERSION 1.0.0, CHANGELOG and CODE_REVIEW
4. Optional (non-blocking): Stage 2 Task 23 by-ear checks (17-note steal, fast Mono retrigger).

## Context to Preserve

- Architecture: `plugins/O-simpleWavetable/.planning/research/ARCHITECTURE.md`
- Roadmap: `plugins/O-simpleWavetable/.planning/ROADMAP.md`
- Stage 0 context: `plugins/O-simpleWavetable/.planning/stages/0-ideation/CONTEXT.md`
- Complexity 5.0, phased. Highest risk: Phase 2.4 (import swap/reaper/persistence).
- Do NOT port O-Prism's trilinear mip-level blend (it fails QUAL-02); zero the Nyquist bin in the mip builder.
- **21** params (the planning docs' "22" is a miscount — same 21 IDs everywhere; ROADMAP Stage 1 `jassert(params.size() == 22)` must be 21); `lfo_div` default 1/1; bit depth choice Full,16..3 (mid-rise).
- Stage 3 integrates mockups/v1-* per v1-integration-checklist.md (event contract: cycleUpdate / bankUpdate / importStatus + `uiReady` handshake).

## Files Created
- plugins/O-simpleWavetable/.planning/research/ARCHITECTURE.md
- plugins/O-simpleWavetable/.planning/ROADMAP.md
- plugins/O-simpleWavetable/.planning/stages/0-ideation/CONTEXT.md
- plugins/O-simpleWavetable/.planning/parameter-spec.md
- plugins/O-simpleWavetable/.planning/mockups/v1-ui.html, v1-i18n.js, v1-PluginEditor.h, v1-PluginEditor.cpp, v1-CMakeLists.txt, v1-integration-checklist.md, v1-i18n-states.json
