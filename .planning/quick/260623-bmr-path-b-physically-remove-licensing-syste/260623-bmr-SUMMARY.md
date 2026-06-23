---
quick_id: 260623-bmr
status: complete
date: 2026-06-23
commit: 77b6b5a
verification: passed
---

# Summary — 260623-bmr (Path B: physical licensing removal)

Physically removed the now-inert licensing system from the Ouaricon plugin suite,
completing Path B of the pay-what-you-want move (Path A flipped the CI switch in
260622-pwy). Pure dead/inert-code cleanup — **no DSP or runtime behavior changed.**
Licensing never gated audio; the `#if OUARICON_LICENSING_ENABLED` regions were
already compiled out everywhere (`OUARICON_LICENSING` OFF locally, and Path A
stopped CI passing it), so local build behavior is identical before/after.

## What changed (28 atomic commits)

- **16 integrated plugins** (one commit each): stripped every `#if OUARICON_LICENSING_ENABLED`
  guarded region from `PluginProcessor.{h,cpp}` + `PluginEditor.{h,cpp}` and the
  `if(OUARICON_LICENSING)` block from `CMakeLists.txt` — O-AnalogEQ, O-AnalogSaturation,
  O-Bells, O-Chorus, O-Comp, O-Detune, O-DigiDelay, O-Freeze, O-FreqPulse, O-IntonationPad,
  O-Lyrica, O-MicrotonalSampler, O-Polystutter, O-Prism, O-SimpleReverb, O-Tremolo.
  (11 guarded regions per plugin: 3 in Processor.h, 1 in Processor.cpp, 3 in Editor.h, 4 in Editor.cpp.)
- **8 dead-wiring plugins** (one commit each): removed only the dead `if(OUARICON_LICENSING)`
  CMake block (no source wiring) — O-Bassoon, O-Bowed, O-GrainScatter, O-Reed, O-simpleFM,
  O-Texture, O-TextureForge, O-Wind.
- **Root `CMakeLists.txt`** (`dbd9eee`): removed the `OUARICON_LICENSING` option, its
  force-OFF-outside-CI safety lines, and the `if(OUARICON_LICENSING)` add_compile_definitions
  block (incl. `OUARICON_SUPABASE_*` defines). All `OUARICON_RELEASE` branding logic untouched;
  trailing licensing-only comments tidied.
- **`modules/registry.yaml`** (`9a41879`): deleted the `- name: licensing` module entry.
- **`modules/core/licensing/`** (`acd1ca6`): deleted entirely (cpp, module.yaml, README, snippets).
- **`.claude/skills/add-licensing/`** (`77b6b5a`): retired the skill.

Commit order was dependency-safe: plugin consumers first, then root CMake/registry, then the
module + skill directories last.

## Execution note

Per-plugin source edits were delegated to parallel subagents (edit-only, content-matched on
the `#if OUARICON_LICENSING_ENABLED` / `if(OUARICON_LICENSING)` markers, never line numbers);
the orchestrator handled all atomic commits, the global-file edits, and verification. Every
agent self-verified its plugin grepped clean before commit.

## Verification

- Repo grep for `OUARICON_LICENSING` / `OuariconLicense` / `OUARICON_SUPABASE` → zero hits in
  any source or build config. Remaining hits live only in `.planning/` docs, `CHANGELOG.md`
  history, and a stray untracked build log — all intentionally left.
- `modules/cmake/OuariconModules.cmake` unchanged; `.github/` already licensing-free (Path A).
- `cmake -B build` + `ninja` of O-Tremolo, O-Prism, O-Lyrica, O-MicrotonalSampler VST3 →
  clean compile (NINJA_EXIT=0, 176/176).

## Manual follow-ups (out of scope — NOT done)

1. Delete GitHub repo secrets `SUPABASE_URL` / `SUPABASE_ANON_KEY` (`gh secret delete SUPABASE_URL`,
   `gh secret delete SUPABASE_ANON_KEY`).
2. Retire the Supabase `activate` / `validate` / `deactivate` edge functions (oaudio.io backend repo).
