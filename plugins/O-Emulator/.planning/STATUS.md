---
plugin: O-Emulator
stage: 4
phase: research
status: stage_4_research_complete
last_updated: 2026-08-21
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: stage_4_plan
next_stage: 4
ready_for_implementation: true
contract_checksums:
  brief: sha256:334b83d216a014678702b5cc20f08a84029b540f7c81e92e94df980e696ea01a
  parameter_spec_draft: sha256:6b6abad34dd0640ab4ba554a710342d8a9fdb7e05e4fc6cbaf4fa32ec7b9f5d5
  parameter_spec: sha256:b38a4b91ebd4b3118f6556869e151332f8670f0dc10b9ee6ed8ee4708a22d770
  architecture: sha256:afa8e778cd9beef0ed6b227d5b45ef1325c1d4e236c2c80da86aeb47bcac7a67
  roadmap: sha256:14000c30527a64adac26a98686d567c86955951012909d33b31ae63df0612402
---

# O-Emulator Status

## Current Position

Stage: 4 (Polish) — research ✓ (2026-08-21)
Phase: research complete → next: Stage 4 plan
Status: Integration approach pinned — `ouaricon_add_module` (NOT vendoring; JS configure-copied + gitignored), O-Bitrot v1.13.0 as reference, 10 native fns (bridge audit re-anchors 10↔10), factory presets authored denormalized + convertTo0to1 batch, flat list keeps module prev/next; findings in stages/4-polish/RESEARCH.md
Progress: [##################..] 91%

## Phase Progress

### Stage 1: Foundation
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-08-20 | |
| research | ✓ | 2026-08-20 | (ran in main checkout; RESEARCH.md copied here) |
| plan | ✓ | 2026-08-20 | (ran in main checkout; PLAN.md copied here) |
| execute | ✓ | 2026-08-20 | |
| verify | ✓ | 2026-08-20 | |

### Stage 2: DSP
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-08-20 | (ran in main checkout; CONTEXT.md moved here) |
| research | ✓ | 2026-08-20 | (ran in main checkout; RESEARCH.md moved here) |
| plan | ✓ | 2026-08-20 | (ran in main checkout; PLAN.md moved here) |
| execute | ✓ | 2026-08-20 | |
| verify | ✓ | 2026-08-20 | |

### Stage 3: GUI
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-08-21 | |
| research | ✓ | 2026-08-21 | |
| plan | ✓ | 2026-08-21 | |
| execute | ✓ | 2026-08-21 | |
| verify | ⚠ | 2026-08-21 | (automated ✓; human gates → Stage 4) |

### Stage 4: Polish
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-08-21 | |
| research | ✓ | 2026-08-21 | |

## Completed So Far

**Ideation:** ✓ Complete
- Creative brief, requirements (15), draft parameters (5)

**Stage 2 discuss phase:** ✓ Complete (2026-08-20)
- `stages/2-dsp/CONTEXT.md`: reverb register-model-first, closed-loop encoders, harness gate per phase + one /plugin-verify, internal-probe validation

**Stage 2 research phase:** ✓ Complete (2026-08-20)
- `stages/2-dsp/RESEARCH.md`: JUCE 8.0.14 APIs verified from source (DryWetMixer ctor-sizing gotcha, Interpolators consumed-count semantics, Butterworth Q table for 8th-order AA); O-Bitrot harness identified as primary template with exact files/probe bodies to copy; fixed-chunk FIFO precedents (O-Octagon GainStage counter, O-Texture OverlapAddProcessor, O-Bitrot CodecStage); no reusable DSP modules — all codec/resampler/reverb code net-new; dsp-agent.md pitfalls mapped to phases

**Stage 2 plan phase:** ✓ Complete (2026-08-20)
- `stages/2-dsp/PLAN.md`: 20 tasks across the 4 ROADMAP phases (2.1 SNES end-to-end 8 tasks, 2.2 PS1+reverb 3, 2.3 quantizers+switching 4, 2.4 age+polish 5); resolves research open items (per-phase digest anchors with re-anchor discipline, kMaxWetLatencySamples=1024, Butterworth Q table, 5 ms crush micro-fades); two `⟨STAGE-1⟩` placeholders (harness CMake target name, final param IDs) — resolve against Stage 1 artifacts before execution

**Stage 0:** ✓ Complete
- Plugin type: Audio Effect (retro console emulation, stereo in/out)
- Complexity tier 3, research depth DEEP
- 9 features researched and documented (5 codecs, resampling, output stages, SPU reverb, age model, crush macro, mix, switching, invariance infra)
- Professional examples researched: 5 (Plogue Chipcrusher, D16 Decimort 2, Inphonik RX950, C700/SnesPass, psx-reverb ports)
- JUCE classes verified against local 8.0.14 source: Interpolators::Lagrange/WindowedSinc, dsp::DryWetMixer (+setWetLatency), dsp::IIR ArrayCoefficients, dsp::FirstOrderTPTFilter; dsp::Oversampling rejected (2^N only)
- All HIGH/MEDIUM risks have documented fallbacks (SPU reverb → tuned Schroeder; closed-loop → open-loop encoders; arbitrary-ratio → integer-divisor rates)
- Complexity score: 5.0 (capped) — Staged implementation (DSP 4 phases, GUI 2 phases)
- ARCHITECTURE.md + ROADMAP.md documented

## Completed — Stage 1 Execute (2026-08-20)

- CMake target `OEmulator` (PLUGIN_CODE OEmu), APVTS 5 binding params, stereo passthrough shell, GenericAudioProcessorEditor
- Render harness scaffolded (P0 contract / P1 passthrough / P2 ragged invariance) — ALL PASS, baseline digest `fnv1a64=28e7675cdbec475c`
- Installed `O-Emulator-dev.{vst3,component}`; auval registered; **pluginval strictness 10 SUCCESS VST3+AU (COMPAT-01)**
- PLUGINS.md own row → 🚧 Stage 1
- Details: `stages/1-foundation/SUMMARY.md` (incl. reconciliation of discuss-worktree + plan-main-checkout strands)

## Stage 1 Verification (2026-08-20)

- ✅ VERIFIED — `stages/1-foundation/VERIFICATION.md`
- COMPAT-01 → complete (pluginval strictness 10 SUCCESS VST3+AU, independently re-run)
- Harness re-run ALL PASS (18 checks), digest matches baseline `28e7675cdbec475c`
- REQUIREMENTS.md updated: 1 complete, 14 deferred to stage-2/3

## Completed — Stage 2 Execute (2026-08-20)

- Full DSP engine: 5 codec round-trips (BRR, SPU-ADPCM, DPCM, GB 4-bit, Genesis 8-bit+ladder), console-domain resampling (Gaussian/ZOH), psx-spx SPU reverb @ 22.05 kHz, per-console output stages, age model (bed/hum/dulling/drift), crush macro with 5 ms micro-fades, 30 ms equal-power console crossfade, DryWetMixer latency-paired mix (116 samples @ 48 kHz, constant across modes)
- Harness: 52 checks ALL PASS; digest anchors re-anchored per discipline (active: 9cf6baa8d3b61b14 / b23fe10b74526fab / dad157a01f7c393f); CPU ratio 0.017; offline==realtime bit-identical
- PERF-01 audit clean; installed `O-Emulator-dev.{vst3,component}`; auval registered
- Details + deviations: `stages/2-dsp/SUMMARY.md`

## Stage 2 Verification (2026-08-20)

- ✅ VERIFIED — `stages/2-dsp/VERIFICATION.md`
- Harness independently re-run: 52 checks ALL PASS; digest anchors match (9cf6baa8d3b61b14 / b23fe10b74526fab / dad157a01f7c393f)
- pluginval strictness 10 re-run post-Stage-2: SUCCESS VST3 + AU; auval SUCCEEDED (aufx OEmu OuDv)
- REQUIREMENTS.md: 12 stage-2 requirements pending → complete (13/15 total complete; UI-01/UI-02 remain for stage-3)
- RT-safety spot-check: allocation sites confirmed prepare-path only

## Completed — Stage 3 Discuss (2026-08-21)

- `stages/3-gui/CONTEXT.md`: Ouaricon Naturalist aesthetic (dino-skeleton specimen, unused elsewhere); segmented console selector top + info readout (`BRR 4-bit · 32 kHz · Gaussian` style, static JS table) + single row of 4 knobs, ~620×400; per-console accent color within the earth-tone palette; drag + double-click value entry (O-ReverseDelay + O-Prism families); preset-manager deferred to Stage 4 (header reserves space)
- Constraints noted: parameter-spec frozen; avoid the watermarked Adobe Stock paper texture; segmented control is a deliberate deviation from the template's 4+-choice dropdown default

## Completed — Stage 3 Research (2026-08-21)

- `stages/3-gui/RESEARCH.md`: O-Bitrot identified as primary template (same aesthetic, proven `.seg` segmented combos via `getComboBoxState`, WebView+harness coexistence, clean paper.jpg md5-verified); 5 per-console accent hexes designed with contrast numbers (plum/slate/brick/olive/teal, all ≥3.66:1 vs paper); dino PNG verified (1007×665 RGBA, real alpha, wide 1.96:1 content bbox → crop+WebP, right-bleed placement numbers); knob = O-Bitrot setupKnob base + O-Prism attachValueEntry on dblclick (reset → Alt-click); window 620×430; preset-load freshness needs NO revision counter (zero native fns, relay listeners fire on preset load); CMake needs only binary-data target + editor sources (WebView flags pre-planted Stage 1)

## Completed — Stage 3 Execute (2026-08-21)

- WebView GUI: Naturalist frame 620×430, 5-segment console selector + per-console accents + static info readout, 4×60px knobs (drag/shift/wheel/dblclick entry/Alt-reset), dino specimen overlay; 5 relays + 5 three-arg attachments, zero native fns
- Details + deviations: `stages/3-gui/SUMMARY.md`

## Stage 3 Verification (2026-08-21)

- ⚠ PARTIAL (automated ✓) — `stages/3-gui/VERIFICATION.md`
- Independently re-run: harness ALL PASS, digest anchors identical to Stage-2 baseline; pluginval strictness 10 SUCCESS VST3+AU; auval `* * PASS`
- Audits: bridge 0↔0, `window.__JUCE__` 0 authored, param IDs = binding spec, paper.jpg md5 clean, one binary-data target, no hyphenated resources, harness CMake untouched
- REQUIREMENTS.md: UI-01 → partial (human gates only); UI-02 → deferred to stage-4 (preset manager lands there)
- 6 human DAW checks pending (visual pass, crossfade audibility, knob feel, automation refresh, preset reload, console errors) — fold into Stage 4

## Completed — Stage 4 Discuss (2026-08-21)

- `stages/4-polish/CONTEXT.md`: preset-manager v1.0.6 vendored + 15+ factory presets (2 per-console signatures ×5 + ~5–8 cross-console utility), flat alphabetical, authored in denormalized values; 6 inherited human gates run in Logic Pro (inspector check via dev Standalone); final validation (pluginval 10, auval, harness digests unchanged, CHANGELOG, docs); release target = local install only
- Constraints: bridge audit counts change when preset-manager adds native fns (re-anchor the grep-diff), applyPresetJson resets to defaults first, no "/" in preset names, harness must stay digest-identical (UI-layer-only change)

## Completed — Stage 4 Research (2026-08-21)

- `stages/4-polish/RESEARCH.md`: O-Bitrot v1.13.0 confirmed reference (newest preset work, same aesthetic); house pattern is `ouaricon_add_module(OEmulator preset-manager)` — canonical header via include dir + JS configure-copied to `Source/ui/public/modules/` (gitignored), NOT vendoring; module already carries WR-01 reset-to-defaults, sentinel-gated factory writes, name sanitization, migration hook (unused at v1.0.0); editor registers 10 native fns (dialog fns: hoisted SafePointer, bare-return on dead editor, `{success,name}` DynamicObject); bridge audit re-anchors 10↔10 with JS hits in the generated module file, index.html stays 0; factory presets authored denormalized in C++ + one `convertTo0to1` batch pass, all 5 IDs per preset; flat list ⇒ pass prev/next straight to the JS module (no walker override); harness compiles the new processor member cleanly and digests must stay identical

## Next Steps

1. `/plugin-plan O-Emulator 4-polish` — task breakdown: module wiring, 10 native fns, preset-band UI, factory bank values, gates + validation

**Note:** UI mockup phase skipped (user decision, 2026-08-20). parameter-spec.md was promoted directly from parameter-spec-draft.md + ARCHITECTURE.md Parameter Mapping and is the BINDING Stage 1+ contract. Stage 3 (GUI) will design the UI from the brief's UI Concept (console selector focal + 4 macro knobs) without a pre-existing mockup.

## Context to Preserve

- Architecture: `plugins/O-Emulator/.planning/research/ARCHITECTURE.md`
- Roadmap: `plugins/O-Emulator/.planning/ROADMAP.md`
- Stage 0 context: `plugins/O-Emulator/.planning/stages/0-ideation/CONTEXT.md`
- Complexity 5.0, staged; highest risk = SPU reverb register-model port
- Constant worst-case latency across console modes; mix path uses exact setWetLatency pairing
- GPL hygiene: implement codecs from specs, never port blargg/Nuked GPL code

## Files Created

- plugins/O-Emulator/.planning/research/ARCHITECTURE.md
- plugins/O-Emulator/.planning/ROADMAP.md
- plugins/O-Emulator/.planning/stages/0-ideation/CONTEXT.md
- plugins/O-Emulator/.planning/parameter-spec.md (promoted from draft 2026-08-20, mockup skipped)
