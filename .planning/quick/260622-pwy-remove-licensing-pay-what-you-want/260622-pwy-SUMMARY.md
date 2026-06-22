---
quick_id: 260622-pwy
mode: quick
status: complete
description: Switch plugin distribution to pay-what-you-want via Path A — flip the CI licensing switch by removing the licensing/Supabase flags from build-and-release.yml. Path B (full physical removal across 24 plugins) deferred and fully scoped in PLAN.md.
started: 2026-06-22T22:47:39Z
completed: 2026-06-22T22:47:39Z
duration: ~1m
task_count: 1
files_modified:
  - .github/workflows/build-and-release.yml
deferred_followup: Path B — full physical removal (module + 16 guarded plugins + 8 dead CMake blocks + root CMake option + registry block + add-licensing skill + Supabase secrets). See PLAN.md "Deferred".
---

# Quick Task 260622-pwy — SUMMARY

## What was done

Flipped the Ouaricon suite to **pay-what-you-want** by removing the licensing activation gate from CI release builds — the only place it was ever enabled.

Removed from both `Configure CMake` steps in `.github/workflows/build-and-release.yml`:

- `-DOUARICON_LICENSING=ON`
- `-DOUARICON_SUPABASE_URL="${{ secrets.SUPABASE_URL }}"`
- `-DOUARICON_SUPABASE_ANON_KEY="${{ secrets.SUPABASE_ANON_KEY }}"`

Kept `-DOUARICON_RELEASE=ON` and `-DSKIP_PLUGINS="O-Orbit"` intact.

## Effect

- Next CI release builds define no `OUARICON_LICENSING_ENABLED` → every published plugin ships with the full WebView UI and **no activation overlay**.
- **Zero source changes; zero DSP impact** — licensing was a UI-only overlay, never in the audio path. Local/dev builds were already unlicensed (option force-OFF outside CI), so behavior is now uniform across dev and release.
- Fully reversible: re-add the three flags to restore licensing. Dormant licensing code remains in the tree until Path B cleanup.

## Verification

- `grep -rn "OUARICON_LICENSING\|SUPABASE" .github/` → NONE (clean).
- Both CMake configure sites confirmed to retain only `OUARICON_RELEASE` + `SKIP_PLUGINS`.

## Deferred

Path B (full physical removal) is fully scoped in PLAN.md — run as a separate task once PWYW is confirmed permanent.

## Not in scope

Storefront / payment surface (Gumroad, itch.io, Bandcamp PWYW, or donate link) — distribution decision, outside this repo.
