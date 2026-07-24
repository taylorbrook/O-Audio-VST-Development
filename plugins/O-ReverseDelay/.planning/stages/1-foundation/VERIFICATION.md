# Stage 1: Foundation - Verification

## Verification Date

2026-07-23

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md)

1. CMake build target `OuariconReverseDelay` (VST3 + AU + Standalone) that compiles clean and installs
2. 10-parameter APVTS shell matching the checksummed parameter contract (IDs, ranges, defaults, skews, units)
3. Bus layouts per D1: mono→mono, mono→stereo, stereo→stereo accepted; stereo→mono rejected
4. Clean passthrough processBlock; state save/restore; GenericAudioProcessorEditor
5. pluginval strictness 10 passes on VST3 and AU (COMPAT-01, D2 gate)

### Deliverables (from SUMMARY.md + code inspection)

1. `plugins/O-ReverseDelay/CMakeLists.txt` — target `OuariconReverseDelay`, PLUGIN_CODE `ORvD`, `VERSION 1.0.0` (correct keyword), FORMATS VST3 AU Standalone, `juce_dsp` linked
2. `Source/PluginProcessor.h/.cpp` — all 10 params with `ParameterID {id, 1}`, skews via `setSkewForCentre` (delayTime 316, grainSize 158, lowCut 200, highCut 3162), labels ms/%/Hz, no bypass
3. `isBusesLayoutSupported` implements the D1 matrix exactly (`in.size() <= out.size()` guard + mono/stereo whitelist + disabled-bus rejection)
4. Passthrough with `ScopedNoDenormals`, empty-buffer early return, ch0 duplicated into extra outputs; APVTS XML round-trip in get/setStateInformation; `GenericAudioProcessorEditor`; zero editor-only includes (harness constraint honored)
5. Installed bundles pass pluginval 10 on both formats

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| Build target compiles + installs | ✅ Achieved | ninja up-to-date (67/67 at execute); bundles present in system folders |
| 10-param APVTS contract | ✅ Achieved | Code inspection: all 10 IDs character-exact, ranges/defaults/skews/labels match CONTEXT.md table |
| D1 bus layout matrix | ✅ Achieved | Code inspection PluginProcessor.cpp:106-119; pluginval basic-bus tests pass |
| Passthrough + state + editor | ✅ Achieved | Code inspection; pluginval state-restoration tests pass |
| pluginval 10 both formats | ✅ Achieved | Execute: VST3 3/3 + AU 3/3; verify: +1 confirmation run each, SUCCESS |

## Requirements Verification

**Stage:** 1-foundation
**Requirements for this stage:** 1 total (1 must)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| COMPAT-01: Passes pluginval validation (VST3 and AU) | must | ✅ Complete | auval lists AU (`aufx ORvD OuDv`); pluginval strictness 10 SUCCESS on VST3 and AU (4 total runs VST3, 4 total AU across execute+verify, zero failures) |

**Requirements Summary:**
- ✅ Complete: 1
- ⚠️ Partial: 0
- ⏸️ Deferred (later stage): 13 (stage-2: 10, stage-3: 2, stage-4 rollup)
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU + Standalone) | ✅ Pass | ninja: no work to do (clean, up to date) |
| Install to system folders | ✅ Pass | `O-ReverseDelay-dev.vst3` + `.component` present |
| auval registration | ✅ Pass | `aufx ORvD OuDv — Ouaricon Audio Development: O-ReverseDelay-dev` |
| AU component version | ✅ Pass | AudioComponents version = 65536 = 1.0.0 (VERSION-keyword regression avoided) |
| pluginval 10 — VST3 | ✅ Pass | SUCCESS (verify-phase confirmation run; 3/3 at execute) |
| pluginval 10 — AU | ✅ Pass | SUCCESS (verify-phase confirmation run; 3/3 at execute) |
| Parameter contract | ✅ Pass | 10/10 IDs, ranges, defaults, skew centres, choice orders verified against CONTEXT.md by inspection |
| delayTime skew spot-check | ✅ Pass | `setSkewForCentre(316)` maps normalized 0.5 → 316 ms by construction (within 315–316 gate) |
| State round-trip | ✅ Pass | APVTS copyState/replaceState XML pattern; exercised by pluginval state-restoration tests |
| Bus layouts (D1) | ✅ Pass | Code implements exact matrix; pluginval exercises mono/stereo layouts at level 10 |

## Human Verification

- [ ] Load in Logic/DAW on a mono track: both output channels carry signal (mono→stereo passthrough)
- [ ] GenericAudioProcessorEditor shows all 10 params with ms/%/Hz units; delayTime mid-travel reads ≈316 ms
- [ ] Save/reload a session with non-default values; all 10 params restore

## Issues Found

- **0→1 quality gate bypassed with `--force`** (logged in `gate-bypasses.log`): gate's build check demands target `O-ReverseDelay_VST3`, which cannot exist before Stage 1 creates the project (real target is `OuariconReverseDelay_VST3`). Not a quality shortfall — the gate's checks (build, pluginval) were executed and passed. Pre-existing script limitation (target ≠ folder name, see `build_script_target_name_vs_folder`).

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes

**Blockers:** None
