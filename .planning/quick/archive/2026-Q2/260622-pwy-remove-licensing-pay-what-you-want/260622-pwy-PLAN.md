---
quick_id: 260622-pwy
mode: quick
task_count: 1
description: Switch plugin distribution from licensed/activation to pay-what-you-want. Path A — flip the CI switch (remove licensing flags from build-and-release.yml). Path B (full physical removal) deferred as a scoped follow-up.
---

# Quick Task 260622-pwy — PLAN

## Goal

Move the Ouaricon plugin suite from a license-gated model to **pay-what-you-want** (free, no activation gate) with the lowest-risk change that actually flips the outcome.

## Key findings (investigation)

- **Licensing never gates audio.** The only effect of a missing license is a native JUCE overlay (`OuariconLicenseOverlay`) that hides the WebView UI (`webView->setVisible(false)`). No `processBlock` checks a license — no silence, demo timeout, or nag. Removal changes zero DSP behavior.
- **Already OFF everywhere except CI.** `OUARICON_LICENSING` defaults `OFF` (CMakeLists.txt:11) and is force-OFF outside CI (:19–28). The `OUARICON_LICENSING_ENABLED` guards only compile in when CI passes `-DOUARICON_LICENSING=ON` (build-and-release.yml:80 and :435).
- Therefore PWYW is primarily a **CI + storefront** decision, not code surgery.

## Decision (user)

**A now + B as a scheduled cleanup.** Flip the CI switch now; defer full physical removal as a separate scoped task.

## Task (Path A — executed)

Remove the three licensing flags from both `Configure CMake` steps in `.github/workflows/build-and-release.yml`, preserving `OUARICON_RELEASE` and `SKIP_PLUGINS`:

- `-DOUARICON_LICENSING=ON`
- `-DOUARICON_SUPABASE_URL="${{ secrets.SUPABASE_URL }}"`
- `-DOUARICON_SUPABASE_ANON_KEY="${{ secrets.SUPABASE_ANON_KEY }}"`

Sites: per-plugin release job (lines ~80, 83–84) and batch/all-plugins job (line ~435).

Result: next CI release builds define no `OUARICON_LICENSING_ENABLED`, so every published plugin ships with the full WebView UI and no activation overlay. No source files touched.

## Deferred (Path B — full physical removal, NOT in this task)

Separate scoped follow-up once PWYW is confirmed permanent:

1. Delete `modules/core/licensing/` (cpp, module.yaml, README, snippets).
2. Strip `#if OUARICON_LICENSING_ENABLED` guarded blocks from 16 fully-integrated plugins (Processor + Editor, .h/.cpp). Targets: O-AnalogEQ, O-AnalogSaturation, O-Bells, O-Chorus, O-Comp, O-Detune, O-DigiDelay, O-Freeze, O-FreqPulse, O-IntonationPad, O-Lyrica, O-MicrotonalSampler, O-Polystutter, O-Prism, O-SimpleReverb, O-Tremolo.
3. Remove the dead `if(OUARICON_LICENSING)` CMake block (no source touch) from 8 plugins: O-Bassoon, O-Bowed, O-GrainScatter, O-Reed, O-simpleFM, O-Texture, O-TextureForge, O-Wind.
4. Remove the `OUARICON_LICENSING` option + force-OFF safety + `if(OUARICON_LICENSING)` define block from root CMakeLists.txt (lines 11, 23–28, 42–57).
5. Drop the `licensing` block from `modules/registry.yaml` (lines 47–71).
6. Retire the `.claude/skills/add-licensing/` skill.
7. Remove the now-unused `SUPABASE_URL` / `SUPABASE_ANON_KEY` GitHub repo secrets (and optionally retire the Supabase activate/validate/deactivate edge functions).

## Out of scope (storefront)

PWYW also needs a payment surface (Gumroad / itch.io / Bandcamp PWYW listing, or a "donate" link beside a free download). Distribution decision, not this repo.
