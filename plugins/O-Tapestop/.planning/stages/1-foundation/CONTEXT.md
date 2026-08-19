# Stage 1 (Foundation) — Context

**Plugin:** O-Tapestop
**Date:** 2026-08-15
**Mode:** manual (interactive discuss)
**Branch/worktree:** `feat/o-tapestop` @ `~/Dev/VST-development-tapestop` (sibling worktree per parallel-plugin rules; O-Bitrot may proceed in parallel in the main checkout)

## Goal

Build system + project structure + 14-parameter APVTS + bitwise pass-through shell. Gate: pluginval passes, disengaged output is bit-identical to input (COMPAT-01 / null probe).

## Decisions made this phase

1. **parameter-spec promoted, not rewritten.** `parameter-spec-draft.md` → `parameter-spec.md` via `git mv`; sha256 `9c7cb391…` matches the Stage 0 `contract_checksums.parameter_spec` pin exactly, so all Stage 0 contracts remain valid. Parameter set was declared fixed at Stage 0 ("mockup owns layout"). UI mockup is deferred to before Stage 3.
2. **Worktree per repo rules.** Implementation lives on `feat/o-tapestop` cut from `main` at `e4ed46a7`, in sibling worktree `VST-development-tapestop`.
3. **Editor in Stage 1: none/generic.** No WebView work; `createEditor()` returns `juce::GenericAudioProcessorEditor` for Standalone sanity checks. Repo pattern (render-harness vs WebView) applies at Stage 3, not now.
4. **State scaffold includes the envelope blob slot from day one.** `getStateInformation`/`setStateInformation` persist APVTS XML with string property `scratchEnvelopeJson` (`"v":1`); Stage 1 stores/restores the string verbatim (default = gentle wobble constant), no parsing/baking yet — that is Stage 2 Phase 2.3 / Stage 3 work.

## Requirements (from contracts)

- **14 APVTS parameters** exactly as `parameter-spec.md` / ARCHITECTURE.md Parameter Mapping table: ENGAGE (Bool), MODE + SYNC_MODE + 3 sync-division Choices (7 divisions: 1/16, 1/8, 1/4, 1/2, 1 bar, 2 bars, 4 bars), 3 free-ms Floats (10–8000 ms, **skew 0.35**, defaults 500/250/1000), 2 curve Floats (0–100 %, default 50), TONE_TRACK (0–100 %, default 60), MIX (0–100 %, default 100), OUTPUT_GAIN (−24…+12 dB, default 0).
- **processBlock = hard pass-through** (no processing, no gain staging, no smoothing touching the signal) so the disengaged null-test criterion is trivially provable now and stays the Bypassed-state contract for Stage 2.
- **Stereo in/out only**, no MIDI, no sidechain, zero latency (no `setLatencySamples`).
- **CMake:** `juce_add_plugin` with `VERSION` (NOT `PLUGIN_VERSION` — JUCE ignores it), VST3 + AU targets, dev branding suffix per repo default.

## Constraints & known traps (repo memory)

- `AudioParameterChoice` needs ≥ 2 choices (all Choices here have 2–7 — fine).
- Param ID symbols must not shadow `juce::` free functions (`end`/`begin`); current IDs are safe.
- `juce_add_plugin` target name must be resolvable by `build-and-install.sh` (target ≙ folder name `O-Tapestop`).
- The 0-ideation → 1-foundation gate script ALWAYS needs `--force` (its build check is unconditional on stage).
- Build/install: use `./scripts/build-and-install.sh O-Tapestop` (Phase 4 sweeps dev/release variant bundles + AU cache).

## Success criteria

- [ ] Project builds clean (VST3 + AU) via ninja
- [ ] All 14 params visible in generic editor with correct ranges/defaults/units
- [ ] Disengaged output bitwise-equal to input (pass-through shell)
- [ ] State save/restore round-trips APVTS + `scratchEnvelopeJson` string
- [ ] pluginval passes at baseline strictness; `auval -a` lists the AU

## Out of scope for Stage 1

DSP of any kind (capture ring, transport, ramps), envelope JSON parsing/baking, WebView UI, tempo sync math, tone filter, presets.
