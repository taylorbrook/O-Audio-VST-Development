# Stage 1: Foundation — Plan

**Plugin:** O-Bitrot
**Stage:** 1-foundation
**Phase:** plan
**Date:** 2026-08-15
**Inputs:** `parameter-spec.md` (BINDING), `stages/1-foundation/CONTEXT.md`, `stages/1-foundation/RESEARCH.md`

## Goal

Build system + project structure + full APVTS (31 params, exact IDs/types/ranges/defaults/skews
from parameter-spec.md) + bit-transparent stereo passthrough shell that installs cleanly and
passes auval + pluginval strictness 10 (COMPAT-01). No DSP — all degradation families are Stage 2.

## Tasks

1. [ ] Pass the 0-ideation→1-foundation gate with forced bypass
   - Command: gate runs inside execute; invoke with `--force --skip-review`, justification:
     "CMakeLists.txt does not exist yet — Stage 1 creates it; build check is unconditional"
   - Verify bypass logged in `.planning/gate-bypasses.log` (exit 2)
   - Depends on: none

2. [ ] Create `plugins/O-Bitrot/CMakeLists.txt`
   - Target **`OBitrot`** (no hyphen — resolve-target.sh handles folder≠target)
   - `juce_add_plugin(OBitrot ... PLUGIN_CODE OBrt, FORMATS VST3 AU Standalone,
     PRODUCT_NAME "O-Bitrot${OUARICON_DEV_SUFFIX}", VERSION 0.1.0, IS_SYNTH FALSE,
     NEEDS_MIDI_INPUT FALSE)` — template: O-Polystutter with all WebView bits stripped
   - `VERSION` keyword (NOT `PLUGIN_VERSION`); `0.1.0` for the Stage 1 shell, bump to 1.0.0 at Stage 4
   - `juce_generate_juce_header` AFTER `target_link_libraries`; `JUCE_VST3_CAN_REPLACE_VST2=0`,
     `JUCE_USE_CURL=0`; no `NEEDS_WEB_BROWSER`, no `juce_add_binary_data` (Stage 3)
   - Files: `plugins/O-Bitrot/CMakeLists.txt`
   - Depends on: Task 1

3. [ ] Create `Source/PluginProcessor.h/.cpp` — APVTS with all 31 parameters
   - Static `createParameterLayout()` (O-Polystutter pattern), APVTS in member-initializer
   - Every param versioned: `juce::ParameterID{ "ID", 1 }`
   - IDs verbatim UPPER_SNAKE from parameter-spec.md; C++ identifier names must not shadow
     `juce::` free functions (no bare `begin`/`end`)
   - Type map: 7 × `AudioParameterBool` (6 enables + HARD_EDGES), 5 × `AudioParameterChoice`
     (all ≥2 choices), 1 × `AudioParameterInt` (SEED 0–9999, automatable), 18 × `AudioParameterFloat`
   - Choice strings ASCII-exact: CLOCK_MODE `{Sync, Free}` d=0; CLOCK_SYNC_DIV
     `{1/16, 1/8T, 1/8, 1/4T, 1/4, 1/2, 1 bar}` d=2; VINYL_RPM `{33 1/3, 45}` d=0;
     PACKET_CONCEAL `{Silence, Repeat, Decay, Substitute}` d=2; CODEC_MODE `{Mu-law, GSM}` d=0
   - Skews via `setSkewForCentre` at geometric mean: CLOCK_FREE_RATE 0.1–20 Hz → 1.414 Hz;
     CRUSH_RATE 500–20000 Hz → 3162 Hz; all other floats linear
   - Defaults per spec (Tape/CD/Vinyl enables ON, Packet/Codec/Crush enables OFF, MIX 100,
     CRUSH_BITS 16, CRUSH_RATE 20000, …)
   - Cache `std::atomic<float>*` per param via `getRawParameterValue` in the constructor
     (unused in Stage 1 processBlock; wired now so Stage 2 only adds DSP)
   - Files: `plugins/O-Bitrot/Source/PluginProcessor.h`, `.cpp`
   - Depends on: Task 2

4. [ ] Implement passthrough shell + bus layout + state persistence
   - `processBlock`: `ScopedNoDenormals` + clear-extra-output-channels loop only; all channel
     loops bounded by `buffer.getNumChannels()` (Standalone canonical-channelset OOB trap)
   - `isBusesLayoutSupported`: stereo-in/stereo-out only
   - **No `setLatencySamples` call** — Stage 1 reports 0; the `ceil(0.020·fs)` scheme lands
     with the compensated read head in Stage 2 Phase 2.1
   - State: `copyXmlToBinary(*apvts.copyState().createXml(), ...)` /
     `getXmlFromBinary` → `apvts.replaceState(...)` (O-ReverseDelay pattern,
     PluginProcessor.cpp:2448-2457); SEED rides along as an APVTS param
   - Files: `plugins/O-Bitrot/Source/PluginProcessor.cpp`
   - Depends on: Task 3

5. [ ] Create `Source/PluginEditor.h/.cpp` — placeholder editor
   - `juce::GenericAudioProcessorEditor` (WebView is Stage 3; no `#if JUCE_WEB_BROWSER` guard yet)
   - Files: `plugins/O-Bitrot/Source/PluginEditor.h`, `.cpp`
   - Depends on: Task 3

6. [ ] Build and verify parameter fidelity
   - `cd build && cmake .. ` (regenerate so the auto-discovery glob picks up the new folder),
     then `ninja OBitrot_VST3 OBitrot_AU`
   - Audit all 31 params against parameter-spec.md: ID, type, range, step, default, skew,
     choice strings — a line-by-line check, not a spot check
   - Depends on: Tasks 2–5

7. [ ] Install with cache clear + dual-variant sweep
   - `./scripts/build-and-install.sh O-Bitrot` (Phase 4 sweeps `-dev` ↔ unsuffixed variants)
   - Depends on: Task 6

8. [ ] Validation gate
   - `auval -a | grep -i bitrot` — AU registered (aufx / OBrt)
   - pluginval strictness 10, run directly (battery default is 8), 2–3 runs each:
     `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --skip-gui-tests --validate <installed bundle>` for VST3 AND AU
   - SEED persistence eyeball: Standalone → set seed → quit → relaunch
   - Depends on: Task 7

9. [ ] Update STATUS.md + commit
   - Mark Stage 1 execute/verify progress, registry row untouched (PLUGINS.md row exists?
     if adding, only O-Bitrot's row; dedupe check after any merge)
   - Branching mode `none`: commit directly on `main`
   - Depends on: Task 8

## Success Criteria

- [ ] `ninja OBitrot_VST3 OBitrot_AU` builds clean (note: targets are `OBitrot_*`, not `O-Bitrot_*`)
- [ ] All 31 params present with exact IDs/types/ranges/defaults/skews from parameter-spec.md
- [ ] Passthrough is bit-transparent (no DSP in any path)
- [ ] State save/restore round-trips, including SEED
- [ ] Latency reported = 0 (no setLatencySamples call anywhere)
- [ ] `auval` passes; pluginval strictness 10 passes on VST3 + AU (2–3 consecutive runs)
- [ ] Installed via build-and-install.sh with cache clear + dual-variant sweep
- [ ] Gate bypass logged in `.planning/gate-bypasses.log`

## Traps Checklist (carry into execute)

1. `VERSION` not `PLUGIN_VERSION` in juce_add_plugin
2. `juce_generate_juce_header` after `target_link_libraries`
3. Choice params ≥2 choices (spec min is 2 — OK)
4. ASCII-only host strings (`Mu-law`, `33 1/3`); build concatenations with `<<`/`+` on an lvalue
5. Param C++ identifiers must not shadow `juce::` free functions
6. Dual-variant sweep on every install
7. Gate needs `--force --skip-review` (unconditional build check, no CMakeLists yet)
8. Channel loops bounded by `buffer.getNumChannels()`
9. Skew conversions outside APVTS must use the range's `convertTo0to1/convertFrom0to1`
