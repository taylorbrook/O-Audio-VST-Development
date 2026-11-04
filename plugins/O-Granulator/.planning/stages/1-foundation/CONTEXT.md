# O-Granulator — Stage 1 (Foundation) — CONTEXT

**Date:** 2026-11-04
**Source:** Interactive discuss session (manual mode)
**Contracts:** `BRIEF.md`, `REQUIREMENTS.md`, `parameter-spec.md` (LOCKED, 220 params — NOT the draft), `research/ARCHITECTURE.md`, `ROADMAP.md` § Stage 1, `mockups/v2-CMakeLists.txt`

## Goal

Plugin builds (VST3 + AU + Standalone), all 220 parameters registered in contract order, bitwise pass-through shell, pluginval strictness 10 VST3 + AU, auval PASS (COMPAT-01).

## Decisions (locked this session)

### D1 — PitchCollections: extract AND migrate O-GrainScatter in Stage 1
- Create shared module `modules/tuning/pitch-collections` from `plugins/O-GrainScatter/Source/dsp/PitchEngine.h` `namespace PitchCollections` (Id enum, names, `builtIns()` tables, `Table`, `parsePitch`, `tableFromPitches`, `tableFromText`, `tableFromScala`). Max 64 degrees (`kMaxPitches`).
- Module additions: `edo N [period]` shorthand (error `badEdo`), a `tableFromText` overload reporting the offending token, `toScala (name, text)`.
- Register in `modules/registry.yaml` + `module.yaml`; consumed via `ouaricon_add_module(... pitch-collections)` (uncomment the line in v2-CMakeLists).
- **Migrate O-GrainScatter** to consume the module in the same stage — delete the in-tree namespace, include the module header. Minor bump **3.5.1 → 3.6.0** (edo shorthand is user-visible), CHANGELOG `## [3.6.0]` entry.
- **GS regression gate (mandatory):** existing GS tests (blocksize-null, alias-gate, density-gain, adapt-gate, skip-tilt) still pass; a render null vs. pre-migration GS binary (seeded out-of-tree pair via `git archive`) is bit-identical for all built-in collections + Custom; custom-scale text/Scala parse results unchanged for existing inputs. No tag (only `/publish` tags).
- O-Granulator maps choice index → Id at the parameter boundary (0 Off, 1…5 → 0…4, 6…31 → 6…31; GS "Free" Id 5 dropped). Do NOT copy the file into `plugins/O-Granulator/Source/`.

### D2 — Shell editor: GenericAudioProcessorEditor (Stages 1–2)
- `createEditor()` returns `GenericAudioProcessorEditor` for now. Stage 3 swaps in `v2-PluginEditor`.
- WebView build plumbing still wired in Stage 1 (`NEEDS_WEB_BROWSER TRUE`, `juce_generate_juce_header`, binary-data target for `v2-ui.html` + i18n + preset-manager.js + eb-garamond assets) so Stage 3 is an editor swap, not a CMake rework. Windows WebView2 block (static linking, `withBackend` + `withUserDataFolder`) is a Stage 3 concern — leave the CMake defines in place from the scaffold.
- Render harness guards `createEditor` under `#if JUCE_WEB_BROWSER`.

### D3 — Bus layouts: all three
- mono→mono, mono→stereo, stereo→stereo. Default stereo→stereo. Mono input feeds both ring channels (Stage 2). Channel counts read from buses, never assumed (AudioChannelSet is a bitset; mono-out bus may still get 2 ch buffers).

## Requirements carried from ROADMAP / ARCHITECTURE

- CMake from `mockups/v2-CMakeLists.txt`: `PLUGIN_CODE OuGn`, `VERSION 0.1.0`, `FORMATS VST3 AU Standalone`, C++20, `juce_audio_utils`, `juce_dsp`, modules grain-engine v1.1.0, reverb-unit v1.1.0, preset-manager, eb-garamond, pitch-collections.
- `GranulatorParamIds.h`: loop-generated IDs for `sN_`, `lfoN_`, `macroN`, `modM_`; **append-only mod-destination table (113 entries)**.
- `setLatencySamples(0)` in `prepareToPlay()` (JUCE 8: getLatencySamples non-virtual).
- Engine state ValueTree child: `seed` (int64 — never uint64 via getLargeIntValue), `stateVersion`, `modDestTableVersion`; plus placeholders for non-parameter state from parameter-spec (shared custom scale, per-stream × per-collection uint64 degree masks stored safely, uiLanguage).
- Render harness scaffold, seed injection hook, synthetic AudioPlayHead.
- No parameter writes from `parameterChanged` or the audio thread.

## Test criteria (Stage 1 gate)

- [ ] VST3 + AU + Standalone build; auval PASS; pluginval strictness 10 VST3 + AU
- [ ] Output bitwise equals input at defaults with mix 0 % (shell) across all three layouts
- [ ] Destination-table golden test passes (113 entries, order frozen)
- [ ] Parameter count = 220 and ID order matches `parameter-spec.md` § Full ID list exactly (golden test); types 140 Float / 59 Choice / 21 Bool
- [ ] pitch-collections module unit checks (edo, badEdo, offending-token report, toScala round-trip)
- [ ] O-GrainScatter 3.6.0 regression gate green (D1)

## Out of scope for Stage 1

DSP (Stage 2), WebView editor integration (Stage 3), presets/i18n (Stage 4).

## Risks to flag

- D1 touches a shipped plugin; any GS behavioural diff is a blocker — null test, not listening.
- `#include` path collisions: GS `PitchEngine.h` keeps its engine code; only the `PitchCollections` namespace moves.
