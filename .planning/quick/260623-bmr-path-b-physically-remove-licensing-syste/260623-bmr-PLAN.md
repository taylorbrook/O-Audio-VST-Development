---
quick_id: 260623-bmr
mode: quick-full
task_count: 6
description: Path B — physically remove the licensing system from the Ouaricon plugin suite now that pay-what-you-want is live.
must_haves:
  truths:
    - Pay-what-you-want is live; CI stopped compiling licensing in (quick task 260622-pwy, Path A).
    - Licensing never gated audio — removal changes zero DSP/runtime behavior. Pure dead/inert-code cleanup.
    - In fully-integrated plugins every licensing reference is enclosed in a `#if OUARICON_LICENSING_ENABLED ... #endif` guard, so deleting each guarded region (guards included) removes all of it.
    - Each plugin CMakeLists.txt has exactly one `if(OUARICON_LICENSING) ... endif()` block to delete.
  artifacts:
    - modules/core/licensing/ deleted entirely.
    - 16 fully-integrated plugins stripped of all `#if OUARICON_LICENSING_ENABLED` regions + their CMake block (one commit each).
    - 8 dead-wiring plugins stripped of only their CMake `if(OUARICON_LICENSING)` block (one commit each).
    - Root CMakeLists.txt — OUARICON_LICENSING option, force-OFF safety lines, and add_compile_definitions block removed; OUARICON_RELEASE branding untouched.
    - modules/registry.yaml licensing module block deleted.
    - .claude/skills/add-licensing/ deleted.
  key_links:
    - CMakeLists.txt
    - modules/registry.yaml
    - modules/core/licensing/
    - .claude/skills/add-licensing/
    - plugins/O-Tremolo/Source/PluginProcessor.h
---

# Quick Task 260623-bmr — PLAN (Path B: physical licensing removal)

## Goal

Physically delete the now-inert licensing system from the repo. PWYW is live and CI no
longer compiles licensing in (Path A, 260622-pwy). This is pure cleanup of dead/inert
code: **change NO DSP or other runtime behavior.** The `#if OUARICON_LICENSING_ENABLED`
regions are already compiled out everywhere; this removes the source so it stops
shadowing greps and confusing future readers.

## Recipe (verified against O-Tremolo + root files)

- **Source (fully-integrated plugins):** every licensing reference is inside a
  `#if OUARICON_LICENSING_ENABLED ... #endif` guard. Delete each guarded region in full
  (the `#if`, the `#endif`, and everything between). Locations: PluginProcessor.h (license
  include, `getLicenseManager()`, `licenseManager` member), PluginProcessor.cpp (ctor
  `make_unique<OuariconLicense>`), PluginEditor.h (UI include, `OuariconLicense::Listener`
  base class, `licenseOverlay` member + `licenseStatusChanged` decl), PluginEditor.cpp
  (ctor overlay setup, dtor `removeListener`, `resized()` overlay bounds,
  `licenseStatusChanged` body).
- **CMake (all 24 plugins):** delete the block
  `# Licensing module (compile-flag gated, OFF for local dev)` + `if(OUARICON_LICENSING) ... endif()`.
- Match by **content**, not line numbers. Leave no orphaned `#if`/`#endif` or dangling comment.

## Tasks (one atomic commit each)

1. **chore: delete licensing module** — `git rm -r modules/core/licensing/`.
2. **chore: strip licensing from 16 integrated plugins** — one commit PER plugin:
   O-AnalogEQ, O-AnalogSaturation, O-Bells, O-Chorus, O-Comp, O-Detune, O-DigiDelay,
   O-Freeze, O-FreqPulse, O-IntonationPad, O-Lyrica, O-MicrotonalSampler, O-Polystutter,
   O-Prism, O-SimpleReverb, O-Tremolo. (source guards + CMake block)
3. **chore: strip licensing CMake block from 8 dead-wiring plugins** — one commit PER plugin:
   O-Bassoon, O-Bowed, O-GrainScatter, O-Reed, O-simpleFM, O-Texture, O-TextureForge,
   O-Wind. (CMake block only; no source)
4. **chore: remove licensing from root CMakeLists.txt** — option (lines ~10-11), force-OFF
   safety lines (the `if(OUARICON_LICENSING)` warning + `set(OUARICON_LICENSING OFF)`), and
   the `if(OUARICON_LICENSING) ... else() ... endif()` add_compile_definitions block. KEEP
   all OUARICON_RELEASE branding.
5. **chore: drop licensing module from registry.yaml** — delete the `- name: licensing` entry.
6. **chore: retire add-licensing skill** — `git rm -r .claude/skills/add-licensing/`.

## Guardrails

- Do NOT modify `modules/cmake/OuariconModules.cmake` (no licensing-specific code) or any DSP.
- After removal: grep repo for `OUARICON_LICENSING`, `OuariconLicense`, `OUARICON_SUPABASE`
  → zero hits in source/build config (remaining hits in `.planning/` docs, `CHANGELOG.md`
  historical entries, and stray build logs are expected and NOT rewritten).
- Configure + build O-Tremolo, O-Prism, O-Lyrica, O-MicrotonalSampler locally → clean compile.

## Out of scope (manual follow-ups — report, do not attempt)

- Delete GitHub repo secrets `SUPABASE_URL` / `SUPABASE_ANON_KEY` (`gh secret delete`).
- Retire the Supabase activate/validate/deactivate edge functions (live in the oaudio.io
  backend, not this repo).
