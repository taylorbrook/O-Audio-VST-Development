---
title: "O-MicrotonalSampler Stage 4 (Polish) — Context"
created: 2026-04-28
stage: 4-polish
phase: discuss
status: complete
verifies_requirements:
  - PERF-02
  - QUAL-01
inputs:
  - .planning/BRIEF.md
  - .planning/REQUIREMENTS.md
  - .planning/STATUS.md
  - .planning/stages/3-gui/VERIFICATION.md
---

# Stage 4 (Polish) — Context

## Discussion Summary

**Date:** 2026-04-28
**Participants:** User, Claude
**Predecessor:** Stage 3 VERIFIED (commit `b89b6f0`) — all five Stage 3
requirements (FUNC-05, FUNC-06, DSP-06, UI-01, UI-02) complete; pluginval
--strictness 5 + auval green.

Stage 4 is the **final v1.0 close-out**. Scope is intentionally narrow:
finish the two `partial` Stage 2 requirements, plumb the dynamic version
string flagged in the Stage 3 hand-off, and run a strict-10 + DAW gate.
Preset system, installers, public distribution, and Windows builds are
**explicitly deferred** to v1.1+ per user direction.

## Requirements In Scope

| ID | Description | Carries from | Stage 4 action |
|----|-------------|--------------|----------------|
| **PERF-02** | 16 voices ≤ 5 % CPU on Apple Silicon @ 48 kHz / 256 buffer | Stage 2 partial | Run Logic Pro CPU-meter benchmark; convert to `complete` |
| **QUAL-01** | No clicks / zipper / aliasing at ±50 c retune across full vel/poly range | Stage 2 partial | Targeted artifact-pass listening test; convert to `complete` |

Plus one Stage 3 hand-off carry-forward (no requirement ID, polish item):

- **Version pill plumbing** — replace hard-coded `v0.1.0` in `index.html`
  `.about-card` with a runtime-resolved string sourced from
  `JucePlugin_VersionString` / `CMakeLists.txt PLUGIN_VERSION`.

## Out of Scope (this stage)

| Item | Reason | Future |
|------|--------|--------|
| Preset save/load (factory + user) | Brief did not call this out; user opted out for v1.0 | v1.1 |
| Per-slot crossfade-length control | Phase 2.5 RP3-2 — global-only stays for v1.0 | v1.1 |
| PKG / EXE installer | Internal-use-only release target | v1.1 if shared externally |
| Windows VST3 build | macOS-only for v1.0 | v1.1 if Windows host needed |
| Code signing / notarization | Not distributed publicly | Public-release milestone |
| Octave grouping for narrow grid | Phase 3.x RP3-5 — horizontal scroll suffices | v1.1 |
| Render-harness regression target | Stage 4 testing covered by pluginval-10 + DAW smoke | If preset round-trip needed later |

## Locked Decisions (D4-1 .. D4-7)

| ID | Decision | Choice | Rationale |
|----|----------|--------|-----------|
| **D4-1** | Stage 4 scope | Version plumbing + PERF-02 + QUAL-01 | Intentional narrow close-out — no scope creep into v1.1 features |
| **D4-2** | Release target | **Internal / personal use only** | User builds for own DAW workflow; skip signing / notarization / public distribution |
| **D4-3** | Platforms | **macOS only** (VST3 + AU + Standalone) | Defer Windows VST3 to v1.1 — avoids WebView2 static-link verification work for this stage |
| **D4-4** | Version plumbing mechanism | **Native function `getPluginVersion()`** | Returns `JucePlugin_VersionString` from C++ to JS at editor mount; About tab renders into existing version pill. Matches established native-function pattern (`getTuningName`, etc.). No build-time codegen needed |
| **D4-5** | Final automated gate | **pluginval `--strictness 10` + auval + latency-invariant grep + memory-pattern grep** | Bumps Stage 3's strictness-5 bar to max. Adds RT-safety guard at the highest level pluginval supports |
| **D4-6** | DAW smoke test | **Logic Pro + Dorico** | Logic = strictest AU host. Dorico = the canonical microtonal note-expression target (DSP-07/08). Live + Reaper deferred unless smoke surfaces a host-specific issue |
| **D4-7** | PERF-02 method | **Logic Pro CPU meter, 16-voice held chord, 48 kHz / 256 buffer, Apple Silicon** | Real-DAW measurement. Held chord with looping samples + ADSR + vel-xfade weights — matches BRIEF target verbatim |
| **D4-7a** | QUAL-01 method | **Targeted artifact pass** (~10 min) per Stage 2 VERIFICATION human checklist | Sustained sine + cello vibrato + transient + ±50 c retune sweep + voice-steal stress |

## Sub-stage Plan (provisional — for /plugin-plan to formalise)

| Sub | Goal | Verifies | Atomic gate |
|-----|------|----------|-------------|
| 4.1 | Version-pill native function + JS wire-up | (polish) | Build green; About pill reflects `JucePlugin_VersionString`; pluginval-10 SUCCESS |
| 4.2 | PERF-02 measurement run | PERF-02 → complete | Logic CPU meter ≤ 5 % at 16 voices / 48 k / 256; result logged in VERIFICATION |
| 4.3 | QUAL-01 listening pass | QUAL-01 → complete | Targeted artifact-pass checklist signed off; defects (if any) reopen relevant Stage 2 sub-phase before closure |
| 4.4 | Final stage gate | (closure) | pluginval --strictness 10 SUCCESS (skip-gui + with-gui) + auval SUCCEEDED + Logic + Dorico smoke + invariant greps + cache-clear/install per CLAUDE.md |

Sub-stage order is strict: 4.1 → 4.2 → 4.3 → 4.4. A failure in 4.2 / 4.3
**does not** auto-promote to Stage 4 close — it reopens the relevant
Stage 2 sub-phase per the Stage 3 verify pattern. Only 4.4 can close
Stage 4.

## Constraints & Invariants (must-preserve)

- **Latency-zero contract** — `setLatencySamples` grep must keep returning
  the single comment-only hit at `PluginProcessor.cpp:133`. PERF-04.
- **RT-safe processBlock** — no allocations, no I/O, no locks. PERF-01.
- **Stage 2 audio path** — no edits to `MicrotonalSamplerVoice.{h,cpp}`,
  `LoopDetector.{h,cpp}`, `SampleLoader.cpp` audio-thread code paths.
  4.x is bounded to (a) version-string plumbing and (b) measurement.
- **Cross-platform WebView memory patterns** — `NEEDS_WEBVIEW2 TRUE` +
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `withUserDataFolder` +
  path-equality resource provider. Even though Windows is out of v1.0
  release scope (D4-3), the build flags must remain correct so v1.1
  Windows build is a no-cost flip.
- **`JucePlugin_VersionString` source of truth** — `CMakeLists.txt`
  `VERSION` argument to `juce_add_plugin`. `index.html` must not have a
  hard-coded fallback string after 4.1 lands; it must read from the
  native function and render an empty pill if the native function is
  unavailable (defensive — should never happen in practice).

## Open Questions (for /plugin-research)

- **RQ4-1** — Is `JucePlugin_VersionString` a string macro available at
  C++ compile time, or does JUCE expose a runtime accessor? (Likely the
  former — verify the include needed and the exact macro name in JUCE
  8.0.4. Consult the local JUCE tree at `/Users/taylorbrook/JUCE` and
  prior plugins' patterns.)
- **RQ4-2** — Does pluginval `--strictness 10` add any test that fails
  for WebView-based editors that strictness-5 tolerated? Quick check
  against O-Bells / O-Bassoon Stage 4 history.
- **RQ4-3** — Logic Pro's CPU meter: does it report per-track plugin
  CPU directly, or do we measure delta between empty track and loaded
  track? Specify the protocol so the PERF-02 number is reproducible.
- **RQ4-4** — Dorico smoke procedure: what's the minimum viable
  Playback Template / endpoint config to route a Dorico microtonal
  passage through O-MicrotonalSampler for the smoke test? (See
  `~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/critical_dorico_distribution_mechanism.md`
  — Dorico does not auto-ingest standalone .doricoexpmap files. May need
  an ad-hoc patch endpoint mapping for smoke purposes only.)

## Verification Bar (for /plugin-verify at stage close)

- [ ] PERF-02 → `complete` in REQUIREMENTS.md, with measured CPU value
      logged in VERIFICATION.md
- [ ] QUAL-01 → `complete` in REQUIREMENTS.md, with signed-off
      listening-test checklist in VERIFICATION.md
- [ ] About-tab version pill reflects current `JucePlugin_VersionString`
      (visual confirmation in DAW + code grep showing no `v0.1.0` literal
      in `index.html`)
- [ ] `pluginval --strictness 10 --validate-in-process --skip-gui-tests`
      SUCCESS
- [ ] `pluginval --strictness 10 --validate-in-process` (with GUI)
      SUCCESS
- [ ] `auval -v aumu OMtS OuDv` AU VALIDATION SUCCEEDED
- [ ] Logic Pro smoke: load folder, play 16-voice chord, audition
      tuning + loop edit, no crashes / no AU revalidation prompts
- [ ] Dorico smoke: route a microtonal passage to O-MicrotonalSampler
      via Playback Template / endpoint mapping; per-note retune
      audible; no zipper / clicks under sustained playback
- [ ] Latency-invariant grep — single comment-only hit
- [ ] Memory-pattern grep — `NEEDS_WEBVIEW2` + `STATIC_LINKING` +
      `withUserDataFolder` all present
- [ ] All 22 REQUIREMENTS rows show `complete` (or explicit OOS)

## Next Phase

Ready for: **research** — `/plugin-research O-MicrotonalSampler 4-polish`
to resolve RQ4-1 .. RQ4-4 before plan.
