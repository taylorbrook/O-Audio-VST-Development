---
plugin: O-Octagon
stage: 0
status: complete
last_updated: 2026-08-11
complexity_tier: 6
complexity_score: 5.0
research_depth: DEEP
staged_implementation: true
orchestration_mode: true
next_action: invoke_foundation_shell_agent
next_stage: 1
ready_for_implementation: true
build_target: OuariconOctagon
plugin_code: OuOc
musical_parameter_count: 17
venue_value_count: 42
contract_checksums:
  brief: sha256:697a4f32890d7420cdef85bafbf8fe45775bf805cf1ff7b449ed2c14f6b9fbd6
  parameter_spec: sha256:5c5f4f06dcb3f344bea400232d1c91e25518974a0a2a9581825619fa93f7022f
  architecture: sha256:bff8a83b379113ac8b1e2a8915d6f1edc7183558b992bdc3808877f86c406cfe
  roadmap: sha256:aec7d0ce0db9ad6c78cb1c9e9574a0a2f8ddb1cf258e6e4b701f2e2e0137ee29
---

# O-Octagon Status

## Current Position

Stage: 0 of 4 (Ideation → Research & Planning) — **complete**
Status: Architecture and roadmap documented, ready for implementation
Progress: `[##..................]` 10%

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined (Logic-native 8-channel DBAP spatializer for an irregular, non-flat concert array)
- Architecture inherited as locked constraints from `research/logic-pro-multichannel-octaphonic-dbap.md`
- **17** musical parameters + 42 venue values specified *(corrected from 18 at Stage 0 — see ARCHITECTURE §11)*
- Preset strategy settled (two separate stores)
- Signal flow, UI concept, use cases captured
- 30 requirements extracted with acceptance criteria
- v1.1+ deferrals recorded explicitly

**Stage 0:** ✓ Complete
- Complexity tier 6 (DEEP research); complexity score **5.0** (capped; raw 13.0)
- 9 features researched: bus transport, channel map, venue model, convex hull, DBAP solver,
  source shaping, outside-hull processing, gain stage, verify-ping
- All JUCE APIs verified **directly against the local JUCE 8.0.14 source tree** with file:line
  references (Context7 doc-fetch was unavailable; local source is the stronger authority)
- 8 core DSP components specified with full algorithms
- 3-layer channel-map test strategy designed (runtime invariant → source-parsed golden with a
  committed SHA → offline tone-per-speaker render)
- All 5 open questions resolved with concrete defaults
- Parameter-count discrepancy resolved: **17**, arithmetic slip demonstrated
- 8 risks registered; 2 design defects found and fixed before code exists
- Strategy: **staged implementation** — Stage 2 in 3 phases, Stage 3 in 3 phases
- ARCHITECTURE.md and ROADMAP.md documented

## Stage 0 Findings Worth Carrying

1. **`kAudioChannelLayoutTag_Emagic_Default_7_1`** (`juce_CoreAudioLayouts_mac.h:117`) shows Logic's
   native 7.1 ordering corresponds to JUCE's `create7point1SDDS()` membership, not `create7point1()`.
   `isBusesLayoutSupported()` therefore accepts all three 8-channel containers, and the label map is
   keyed on `ChannelType`. Settled at Stage 4.
2. **For 7.1 the enum-bit order coincidentally equals the initializer-list order** — a hardcoded
   0..7 map would appear correct today. The locked constraint is *more* important because of this,
   not less.
3. **PERF-02 and QUAL-03 are incompatible under a per-block solve.** Resolved with a fixed
   64-sample absolute-sample-aligned control grid.
4. **Centre-crossing L/R flip** in the stereo sub-point geometry found at design time; fixed with an
   `rFade` spread collapse.

## Next Steps

1. **Stage 1: Foundation** — CMake, bus layout, 17 APVTS parameters, pluginval 10.
   Run `/implement O-Octagon` (orchestrated) or `/plugin-discuss O-Octagon` (manual).
2. Review `research/ARCHITECTURE.md` and `ROADMAP.md`
3. UI mockup — two screens, Room + Venue. Due before Stage 3.1; not a Stage 1/2 blocker.
4. Measure Roy Barnett Recital Hall — 8 × (x, y, z) metres + front/rear ear heights
5. Pause here

## Context to Preserve

**Build constraints for Stage 1:**
- Target `OuariconOctagon`, folder `plugins/O-Octagon`, `PRODUCT_NAME "O-Octagon${OUARICON_DEV_SUFFIX}"`
- `PLUGIN_CODE OuOc` (verified unused across all 39 existing plugins)
- **`VERSION 1.0.0`**, never `PLUGIN_VERSION`
- **No `PLUGIN_CHANNEL_CONFIGURATIONS`**
- `juce::juce_dsp` linked; `BusesProperties` in the constructor init list
- **Must not link SAF** (unlike sibling O-Orbit)

**Locked by prior research — do NOT re-litigate:**
- mono/stereo in → `AudioChannelSet::create7point1()` out; 7.1 is only an 8-channel carrier
- Never `octagonal()` or `discreteChannels(8)` — Logic ignores both
- Not multi-output (`aumu` only; `aufx` gets one bus)
- Speaker→buffer map built ONCE in `prepareToPlay()` via `getChannelIndexForType()` — a wrong map
  is SILENT and passes every automated gate
- DBAP per the **2011-04-14 revised** equations

**Deferred to v1.1+ — do not plan work for these:**
VBAP A/B mode; binaural/stereo fold-down; quadraphonic variant; internal diffuse reverb; motion
engine; multiple simultaneous sources.

**Highest risk:** the speaker→buffer channel map (R1, CRITICAL, silent failure).

## Files Created

- `plugins/O-Octagon/.planning/BRIEF.md` *(Ideation; parameter count corrected at Stage 0)*
- `plugins/O-Octagon/.planning/REQUIREMENTS.md` *(Ideation)*
- `plugins/O-Octagon/.planning/parameter-spec-draft.md` *(Ideation)*
- `plugins/O-Octagon/.planning/research/ARCHITECTURE.md` *(Stage 0)*
- `plugins/O-Octagon/.planning/ROADMAP.md` *(Stage 0)*
- `plugins/O-Octagon/.planning/stages/0-ideation/CONTEXT.md` *(Stage 0)*
- `plugins/O-Octagon/.planning/STATUS.md`
