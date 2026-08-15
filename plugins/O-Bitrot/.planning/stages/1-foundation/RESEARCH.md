# Stage 1: Foundation — Research

**Plugin:** O-Bitrot
**Stage:** 1-foundation
**Phase:** research
**Date:** 2026-08-15
**Inputs:** `parameter-spec.md` (BINDING), `stages/1-foundation/CONTEXT.md`, reference plugins O-Polystutter / O-ReverseDelay

## 1. Build System (CMake)

### Auto-discovery — no root CMake edit needed
Root `CMakeLists.txt:47-58` globs `plugins/*` and calls `add_subdirectory` for any
folder containing a `CMakeLists.txt`. Creating `plugins/O-Bitrot/CMakeLists.txt` is
the only registration step.

### Target name: `OBitrot` (no hyphen)
Repo convention keeps hyphens out of the `juce_add_plugin` target (folder `O-Polystutter`
→ target `OPolystutter`). `scripts/resolve-target.sh` (sourced by build-and-install.sh)
already handles folder≠target — 11/37 plugins differ, this is the supported path.
Ninja targets become **`OBitrot_VST3 OBitrot_AU`**.

### CMakeLists template (from O-Polystutter, WebView bits stripped for Stage 1)
```cmake
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(OBitrot
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OBrt
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Bitrot${OUARICON_DEV_SUFFIX}"
    VERSION 0.1.0            # JUCE reads VERSION — PLUGIN_VERSION is silently ignored
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
)
# target_sources, target_include_directories(Source), standard juce module list,
# juce_generate_juce_header AFTER target_link_libraries,
# JUCE_VST3_CAN_REPLACE_VST2=0, JUCE_USE_CURL=0
```

Key points:
- **`PLUGIN_CODE OBrt`** — verified unique against all 40 existing codes (grep sweep;
  `OBit`/`OBrt` both free, `OBrt` chosen). Four chars, uppercase first.
- **No WebView flags in Stage 1** — `NEEDS_WEB_BROWSER`/`JUCE_WEB_BROWSER=1`/
  `juce_add_binary_data` all land in Stage 3. Keeps the shell build minimal and avoids
  the dual-BinaryData-namespace trap entirely for now.
- **Effect, not synth:** `IS_SYNTH FALSE`, `NEEDS_MIDI_INPUT FALSE` (no MIDI in the brief).
- Dev branding: `OUARICON_DEV_SUFFIX` produces `O-Bitrot-dev.{vst3,component}` locally;
  build-and-install.sh Phase 4 sweeps both variants (never leave the alternate installed).

## 2. APVTS — 31 Parameters

### Layout pattern (O-Polystutter `createParameterLayout`, PluginProcessor.cpp:33+)
- Static `createParameterLayout()` returning `ParameterLayout`, passed to the APVTS
  constructor member-initializer.
- Every param uses **versioned `juce::ParameterID{ "ID", 1 }`** (AU compatibility hint).
- IDs come verbatim from parameter-spec.md (`CLOCK_MODE`, `TAPE_PROB`, …). UPPER_SNAKE
  differs from O-Polystutter's lower_snake but the spec is BINDING — spec wins.
  None of the 31 IDs shadow `juce::` free functions (no bare `begin`/`end` — checked).

### Type mapping (per spec)
| Spec type | JUCE class | Count | Notes |
|---|---|---|---|
| Bool | `AudioParameterBool` | 7 | 6 enables + HARD_EDGES |
| Choice | `AudioParameterChoice` | 5 | All have ≥2 choices (min-2 trap satisfied) |
| Int | `AudioParameterInt` | 1 | SEED 0–9999, automatable; dice = UI writes this param |
| Float | `AudioParameterFloat` | 18 | `NormalisableRange<float>` with step + optional skew |

### Choice params — exact ASCII strings (binding, discuss decision)
- `CLOCK_MODE`: `{"Sync", "Free"}` default 0
- `CLOCK_SYNC_DIV`: `{"1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 bar"}` default 2 (1/8)
- `VINYL_RPM`: `{"33 1/3", "45"}` default 0 — ASCII only; WebView renders 33⅓ in Stage 3
- `PACKET_CONCEAL`: `{"Silence", "Repeat", "Decay", "Substitute"}` default 2 (Decay)
- `CODEC_MODE`: `{"Mu-law", "GSM"}` default 0 — ASCII only; μ-law glyph is Stage 3 UI
- Build any label strings with `<<`/`+` on an lvalue if concatenation is ever needed —
  `juce::String(const char*)` is ASCII-only and `String("...") << x` doesn't compile.

### Exponential skews — use `setSkewForCentre` at the geometric mean
Repo precedent is a numeric skew factor (O-IntonationPad 0.25–0.4, O-Bass 0.5), but for
a spec that says "exponential" the self-documenting form is:
```cpp
auto r = juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f);
r.setSkewForCentre(1.414f);   // sqrt(0.1*20) — geometric mean = exponential feel
```
- `CLOCK_FREE_RATE` 0.1–20 Hz → centre ≈ **1.414 Hz**
- `CRUSH_RATE` 500–20000 Hz → centre ≈ **3162 Hz**
- All other floats linear per spec. Note for later stages: any code converting these
  outside APVTS must go through the range's `convertTo0to1/convertFrom0to1`
  (skew traps: `pattern_factory_preset_normalized_ignores_skew`,
  `critical_apvts_denormalised_vs_preset_normalised`).

### Cached atomics
`std::atomic<float>* cached = apvts.getRawParameterValue("ID");` for each param in the
processor constructor (after APVTS init). Stage 1 doesn't read them in processBlock
(pure passthrough) but wiring them now means Stage 2 only adds DSP.

## 3. Processor Shell

- `processBlock`: `juce::ScopedNoDenormals` + the standard clear-extra-output-channels
  loop, nothing else. Bound all channel loops by `buffer.getNumChannels()` (Standalone
  canonical-channelset OOB trap). This gives bit-transparency by construction.
- `isBusesLayoutSupported`: stereo-in/stereo-out only (ARCHITECTURE: stereo effect).
  Reject others; Standalone still hosts it fine.
- **Latency: report nothing in Stage 1** (implicitly 0). The `ceil(0.020·fs)` constant
  arrives with the compensated read head in Stage 2 Phase 2.1 — reporting 20 ms now
  without delaying audio would misalign passthrough in compensated hosts. JUCE 8:
  `setLatencySamples()` in `prepareToPlay()` when the time comes (non-virtual getter).
- State: exact O-ReverseDelay pattern (PluginProcessor.cpp:2448-2457) —
  `copyXmlToBinary(*apvts.copyState().createXml(), destData)` /
  `getXmlFromBinary` → `apvts.replaceState(ValueTree::fromXml(...))`. SEED rides along
  automatically as an APVTS param.
- Editor: `juce::GenericAudioProcessorEditor` placeholder (WebView is Stage 3; no
  `#if JUCE_WEB_BROWSER` guard needed until then).

## 4. Verification Toolchain

- Build: `cd build && ninja OBitrot_VST3 OBitrot_AU`
- Install: `./scripts/build-and-install.sh O-Bitrot` — resolves the `OBitrot` target via
  resolve-target.sh, clears AU caches, sweeps dev/release variant bundles (Phase 4).
- `auval -a | grep -i bitrot` after install (AU triple: aufx / OBrt / manufacturer code).
- pluginval **strictness 10** (discuss decision; battery script's default is 8, so run
  directly): `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --skip-gui-tests --validate <installed .vst3>` — and the AU component. Level 10 is the
  repo gate that catches latent NaN (`pattern_ci_pluginval10_catches_latent_nan`);
  run 2–3× since some failures are intermittent.
- State round-trip: pluginval's state tests cover it at level 10; SEED persistence can be
  eyeballed in Standalone (set seed → quit → relaunch — Standalone persists state).

## 5. Known Traps Checklist (for PLAN.md)

1. `VERSION` not `PLUGIN_VERSION` in juce_add_plugin — otherwise ships 1.0.0.
2. `juce_generate_juce_header` must come **after** `target_link_libraries`.
3. All Choice params ≥2 choices — satisfied by spec (min is 2).
4. ASCII-only host-facing strings (`Mu-law`, `33 1/3`).
5. Param-ID C++ identifier names must not shadow `juce::` free functions.
6. Dual-variant sweep on every install (`-dev` ↔ unsuffixed shadowing).
7. **0-ideation→1-foundation gate always blocks** on its unconditional build check
   (CMakeLists doesn't exist yet at gate time): run with `--force --skip-review` +
   justification naming the missing CMakeLists; verify the bypass is logged in
   `.planning/gate-bypasses.log` (exit 2).
8. Channel loops bounded by `buffer.getNumChannels()`, never by the layout.
9. Branching mode `none` — all work directly on `main` (Stage 0 practice).

## 6. Module Reuse

No shared Ouaricon modules apply to Stage 1 (preset-manager, scala-tuning-engine etc.
are DSP/UI-stage concerns). Nothing to `/module-add` now; revisit at Stage 3 for
preset-manager (v1.0.5).

## 7. Open Questions for Plan Phase

- None blocking. One recommendation to pin in PLAN.md: use `0.1.0` as the Stage 1
  VERSION (pre-release shell), bumping to 1.0.0 at Stage 4 — matches the staged
  rollout used elsewhere in the suite.
