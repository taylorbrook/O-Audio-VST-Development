# O-Emulator — Stage 1 (Foundation) Context

**Date:** 2026-08-20
**Phase:** discuss (manual mode, interactive)
**Worktree:** `~/Dev/VST-development-emulator` (branch `feat/o-emulator-impl`)

## Stage Goal

Build system, project structure, and APVTS parameter shell for O-Emulator — a stereo
audio effect (retro console emulation). Output: a plugin that builds (VST3 + AU),
loads, passes audio untouched, exposes all 5 parameters to the host, and passes
pluginval at strictness 10.

## Requirements (from ROADMAP.md / parameter-spec.md)

1. **CMake target** `O-Emulator` — JUCE 8.0.14, audio EFFECT (not synth), stereo in/stereo out.
2. **APVTS with exactly 5 parameters** per the BINDING parameter-spec.md:
   - `console` — AudioParameterChoice, 5 entries (SNES / PS1 / NES / Game Boy / Genesis), default SNES
   - `crush` — Float 0–100 %, default 50, linear
   - `age` — Float 0–100 %, default 20, linear
   - `reverb` — Float 0–100 %, default 0, linear
   - `mix` — Float 0–100 %, default 100, linear
   - IDs, types, ranges, defaults are FROZEN — any deviation is a contract violation.
3. **State save/restore** via APVTS (getStateInformation/setStateInformation). No non-parameter state in v1.0.
4. **Pluginval smoke** at **strictness 10**, VST3 + AU (COMPAT-01) — match CI from day one.

## Decisions (discuss phase, 2026-08-20)

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Shell audio path | **Pure passthrough** — processBlock copies input→output; params exist but drive nothing | DryWetMixer + latency reporting land together in Phase 2.1 where the FIFO exists; keeps the `setLatencySamples` ↔ `setWetLatency` pairing atomic (house pattern) |
| Latency in Stage 1 | Report 0 (implicit) | Passthrough shell has no latency; the constant worst-case figure is computed in Phase 2.1 |
| Render harness | **Scaffold in Stage 1** — harness CMake target + passthrough digest baseline | Phase 2.1's block-size-invariance gate (64/512/4096) is ready before any DSP lands; house pattern: harness is the Stage-2 correctness gate |
| Pluginval level | **Strictness 10** locally | CI runs 10 anyway (Windows CI at 10 has caught latent NaN missed elsewhere); passthrough should pass trivially |

## Constraints & House Patterns to Honor

- **AudioParameterChoice ≥ 2 choices** — `console` has 5, satisfied by spec.
- **ASCII-only host-facing strings** (`juce::String(const char*)` is ASCII-only).
- **Param ID identifiers must not shadow juce:: free functions** (repo critical pattern) —
  check generated ID constants (`begin`/`end` shadowing).
- **`juce_add_plugin PLUGIN_VERSION` is NOT a JUCE keyword** — set VERSION correctly or it ships 1.0.0.
- **Dev branding**: local builds produce `O-Emulator-dev.{vst3,component}`; install via
  `./scripts/build-and-install.sh O-Emulator` (does the dual-variant sweep + AU cache clear).
- **Worktree build caveat**: a fresh worktree may fail first CMake configure on untracked
  build deps (O-Orbit SAF submodule pattern) — verify configure early; copy/init missing
  deps rather than fighting the configure.
- **Float params smoothed later** (~20 ms SmoothedValue) — smoothing is Stage 2 scope, not Stage 1.
- **GPL hygiene** (applies from Stage 2 on): codecs from specs, never blargg/Nuked GPL code.

## Out of Scope for Stage 1

- Any DSP (codecs, resampling, reverb, age model) — Stage 2, Phases 2.1–2.4
- WebView UI (JUCE GenericAudioProcessorEditor or headless is fine) — Stage 3
- Preset-manager module — Stage 3/4
- Latency reporting / DryWetMixer — Phase 2.1

## Success Criteria

- [ ] `ninja O-Emulator_VST3 O-Emulator_AU` builds clean in the worktree
- [ ] All 5 params visible in host with correct names/ranges/defaults; state round-trips
- [ ] Audio passes bit-transparent (passthrough)
- [ ] `auval` passes; pluginval strictness 10 passes for VST3 + AU
- [ ] Render-harness target builds and produces a stable passthrough digest
- [ ] PLUGINS.md row updated (own row only — union-merge duplicate check after any merge)

## Source Documents

- `plugins/O-Emulator/.planning/parameter-spec.md` (BINDING)
- `plugins/O-Emulator/.planning/research/ARCHITECTURE.md`
- `plugins/O-Emulator/.planning/ROADMAP.md`
- `plugins/O-Emulator/.planning/BRIEF.md`
