# Stage 1: Foundation — Context

**Plugin:** O-Bitrot
**Stage:** 1-foundation
**Phase:** discuss (manual mode, interactive)
**Date:** 2026-08-15

## Stage Goal

Build system + project structure + full APVTS (31 params) + stereo passthrough shell that
passes pluginval (COMPAT-01). No DSP — all degradation families land in Stage 2.

## Scope

**In scope:**
- `plugins/O-Bitrot/CMakeLists.txt` (juce_add_plugin, VST3 + AU, effect, stereo in/out)
- `Source/PluginProcessor.{h,cpp}` — APVTS with all 31 parameters (versioned `ParameterID{id, 1}`),
  cached `std::atomic<float>*` pointers, plain passthrough `processBlock` (`ScopedNoDenormals`)
- `Source/PluginEditor.{h,cpp}` — placeholder generic editor (WebView comes in Stage 3)
- State persistence: APVTS `getStateInformation`/`setStateInformation` (SEED persists)
- Build via ninja, install via `./scripts/build-and-install.sh O-Bitrot`, `auval` check
- pluginval **strictness 10** gate (discuss decision — repo pattern: level 10 catches latent NaN)

**Out of scope (deferred):**
- CaptureRing, MediaClock, RngBank, all DSP components → Stage 2
- Latency reporting: Stage 1 reports **0**; the constant `ceil(0.020·fs)` scheme is a
  Phase 2.1 component (ROADMAP) — reporting 20 ms without delaying audio would misalign
  passthrough in compensated hosts
- WebView UI, relays/attachments → Stage 3
- libgsm vendoring → Stage 2 Phase 2.5
- Offline render harness → Stage 2 Phase 2.1

## Decisions (from interactive discuss)

1. **Parameter contract promoted:** `parameter-spec-draft.md` + ARCHITECTURE.md Parameter Mapping
   deltas promoted to `parameter-spec.md` (now BINDING). Deltas folded in: CLOCK_SYNC_DIV = 7
   divisions (1/16, 1/8T, 1/8, 1/4T, 1/4, 1/2, 1 bar), CRUSH_RATE = 500 Hz–20 kHz exp skew
   (fs-independent, runtime clamp fs/2), CLOCK_FREE_RATE exp skew. UI mockup may refine
   layout/UI labels only — not IDs/types/ranges/defaults. STATUS.md `parameter_spec` checksum
   updated to the NEW file (repo pattern: promotion must not pin the replaced file).
2. **ASCII-safe host-facing labels:** CODEC_MODE choices `Mu-law`/`GSM`, VINYL_RPM `33 1/3`/`45`.
   Reason: `juce::String(const char*)` is ASCII-only; non-ASCII literals mangle in automation
   lanes. WebView UI renders the real glyphs (μ-law, 33⅓) in Stage 3.
3. **pluginval strictness 10** for the COMPAT-01 gate from Stage 1 onward.

## Constraints & Known Traps (repo patterns)

- `juce_add_plugin` version keyword is `VERSION`, not `PLUGIN_VERSION` (ships as 1.0.0 otherwise)
- `AudioParameterChoice` needs ≥ 2 choices (all five Choice params here have ≥ 2 — OK)
- Param-ID identifiers must not shadow `juce::` free functions (avoid bare `begin`/`end` symbols)
- Dev branding produces `O-Bitrot-dev.{vst3,component}` — build-and-install.sh Phase 4 sweeps
  both variants; never leave the alternate variant installed
- The 0-ideation→1-foundation gate ALWAYS blocks on its unconditional build check —
  run with `--force --skip-review` + justification naming the missing CMakeLists, and verify
  the bypass is logged (`.planning/gate-bypasses.log`, exit 2)
- SEED is an automatable `AudioParameterInt` (ARCHITECTURE decision) — reseed dice is a UI
  trigger writing this param, NOT a separate parameter
- Branching mode `none`: all work on `main` (matches Stage 0 practice)

## Inputs

- `plugins/O-Bitrot/.planning/parameter-spec.md` (BINDING, promoted this phase)
- `plugins/O-Bitrot/.planning/research/ARCHITECTURE.md` (BINDING)
- `plugins/O-Bitrot/.planning/ROADMAP.md`
- Reference CMake/processor structure: recent effect plugins (e.g. O-Polystutter, O-ReverseDelay)

## Success Criteria (feed into PLAN.md)

- [ ] `ninja O-Bitrot_VST3 O-Bitrot_AU` builds clean
- [ ] All 31 params present with exact IDs/types/ranges/defaults/skews from parameter-spec.md
- [ ] Passthrough is bit-transparent (all families' enables have no DSP yet)
- [ ] State save/restore round-trips (including SEED)
- [ ] `auval` passes; pluginval strictness 10 passes (VST3 + AU)
- [ ] Installed via build-and-install.sh with cache clear + dual-variant sweep
