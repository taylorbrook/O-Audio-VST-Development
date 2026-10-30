---
plugin: O-Lyrica
version: 2.6.6
stage: 4
phase: verify
status: complete
last_updated: 2026-10-29
workflow_mode: manual
next_action: plugin_complete
---

# O-Lyrica Status

## Current Position

Stage: 4 of 4 — PLUGIN COMPLETE at v2.6.6 (📦 Installed, last release 2026-10-27)
Status: complete and installed. Changes now go through `/improve O-Lyrica`.

Backfilled 2026-10-29 (audit 261029-md9 F-13) from `CMakeLists.txt` VERSION and `CHANGELOG.md`.
This plugin predates the BRIEF.md / staged layout; its planning docs are the legacy set: creative-brief.md (no parameter-spec.md).

## Latest Changes

- **2.6.6** (2026-10-27): Free / Scale-Locked Glissando are independent parameters, so the host no longer shows a stale value (pluginval state-restore fix).
- **2.6.5** (2026-10-27): tuning mode changed while bypassed now survives save/reload; saved parameter wins over the stale tuning record.
- **2.6.4** (2026-10-26): custom glissando degrees survive reopen and travel with presets; stale `<CustomState>` child removed after restore.

See `CHANGELOG.md` for the full history.
