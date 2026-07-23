# Stage 1: Foundation — CONTEXT

**Plugin:** O-ReverseDelay
**Stage:** 1-foundation
**Phase:** discuss — complete
**Date:** 2026-07-23
**Source:** Interactive discuss session (manual mode) + Stage 0 contracts

## Stage Goal

Build system + APVTS parameter shell that compiles, installs, and passes pluginval
strictness 10 (COMPAT-01). No DSP — processBlock is a clean passthrough. Render
harness explicitly deferred to Phase 2.1.

## Decisions From Discussion

### D1. Bus layouts: mono AND stereo, in and out (user decision — deviates from suite stereo-only convention)
- Supported: mono→mono, mono→stereo, stereo→stereo. Reject stereo→mono (no down-mix path).
- Rationale: brief's guitar-swell use case is commonly a mono source/insert.
- Stage 2 consequence: capture ring handles 1-or-2-channel input (mono duplicated
  into both capture channels); `width` is inert when the output bus is mono.
- `isBusesLayoutSupported`: main out ∈ {mono, stereo}; main in ∈ {mono, stereo};
  disallow in-channels > out-channels.

### D2. Pluginval strictness 10 from Stage 1 (COMPAT-01 gate)
- Run strictness 10 on VST3 + AU now, not just at Stage 4.
- Suite lesson: `pattern_ci_pluginval10_catches_latent_nan` — level-10 fuzz catches
  what lower levels miss; the shell should pass trivially.

### D3. Harness deferred to Phase 2.1 (per roadmap)
- Stage 1 is purely build system + APVTS shell. The offline render harness is the
  FIRST deliverable of Phase 2.1.

## Foundation Specification (from contracts + suite conventions)

### CMake target
- Target: `OuariconReverseDelay` (folder `plugins/O-ReverseDelay/` — note target ≠ folder;
  `build-and-install.sh` resolves via `resolve_cmake_target()`)
- `juce_add_plugin` keywords: `COMPANY_NAME "${OUARICON_COMPANY_NAME}"`,
  `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}`, `PLUGIN_CODE ORvD`
  (verified unique in suite), `FORMATS VST3 AU Standalone`,
  `PRODUCT_NAME "O-ReverseDelay${OUARICON_DEV_SUFFIX}"`, **`VERSION 1.0.0`**
  (JUCE reads `VERSION`, not `PLUGIN_VERSION` — `critical_plugin_version_keyword_ignored_by_juce`)
- Link `juce::juce_dsp` + `juce::juce_audio_processors`; `juce_generate_juce_header()`
  AFTER `target_link_libraries` (critical-patterns §1)
- No WebView flags yet (Stage 3); no BinaryData targets yet

### APVTS — 10 parameters (contract: parameter-spec-draft.md, checksummed in STATUS.md)
| ID | Type | Range / Choices | Default | Skew |
|----|------|-----------------|---------|------|
| delayTime | Float | 50–2000 ms | 500 | setSkewForCentre(316) — geometric mean; Stage-3 spot-check expects midpoint ≈ 315 ms |
| syncMode | Choice | Free, Sync | Sync | — |
| noteDivision | Choice | 1/16, 1/16D, 1/16T, 1/8, 1/8D, 1/8T, 1/4, 1/4D, 1/4T, 1/2, 1/2D, 1/2T, 1/1 | 1/4 | — |
| grainSize | Float | 50–500 ms | 200 | setSkewForCentre(158) |
| density | Float | 0–100 % | 60 | linear |
| feedback | Float | 0–100 % | 40 | linear |
| lowCut | Float | 20–2000 Hz | 100 | setSkewForCentre(200) |
| highCut | Float | 500–20000 Hz | 8000 | setSkewForCentre(3162) |
| width | Float | 0–100 % | 60 | linear |
| mix | Float | 0–100 % | 35 | linear |

- Param IDs exactly as above (camelCase, matches draft spec — the Stage-3 UI binds to these strings)
- Skew centres = geometric mean of range endpoints unless the final UI spec overrides
- Labels/units: ms, %, Hz per table; noteDivision choice strings exactly as listed (13 entries)
- No bypass parameter (none in contract)

### Processor shell
- `processBlock`: passthrough (input → output untouched), `juce::ScopedNoDenormals`
- `getStateInformation`/`setStateInformation`: standard APVTS XML round-trip
- Zero latency (no `setLatencySamples` call needed)
- `createEditor`: `GenericAudioProcessorEditor` for Stage 1 (replaced in Stage 3)
- No MIDI (effect; `isMidiEffect false`, no MIDI in/out)

## Success Criteria (Stage 1)
- [ ] Clean ninja build: `OuariconReverseDelay_VST3` + `OuariconReverseDelay_AU`
- [ ] Installs via `./scripts/build-and-install.sh O-ReverseDelay` (cache-clear + dual-variant sweep)
- [ ] `auval` lists the AU; component version encodes 1.0.0
- [ ] pluginval strictness 10 passes on VST3 and AU (COMPAT-01)
- [ ] All 10 params visible in GenericAudioProcessorEditor with correct ranges/defaults/units
- [ ] State save/restore round-trips all 10 params
- [ ] All bus layouts per D1 accepted; stereo→mono rejected

## Constraints Carried Forward
- Stage 2 will need `juce_dsp` (already linked in Stage 1)
- Harness (Phase 2.1) will compile the processor without the editor — keep
  PluginProcessor free of editor-only includes
