---
plugin: O-Marimba
version: 1.16.3
stage: 4
phase: verify
status: complete
last_updated: 2026-10-29
workflow_mode: manual
next_action: plugin_complete
---

# O-Marimba Status

## Current Position

Stage: 4 of 4 — PLUGIN COMPLETE at v1.16.3 (📦 Installed, last release 2026-10-26)
Status: complete and installed. Changes now go through `/improve O-Marimba`.

Backfilled 2026-10-29 (audit 261029-md9 F-13) from `CMakeLists.txt` VERSION and `CHANGELOG.md`.
This plugin predates the BRIEF.md / staged layout; its planning docs are the legacy set: creative-brief.md, plan.md, architecture.md, parameter-spec.md.

## Latest Changes

- **1.16.3** (2026-10-26): Windows WebView2 hardening (user-data folder under `%APPDATA%`, native fallback notice). No DSP/parameter/state change.
- **1.16.2** (2026-10-19): Effects-tab dials take typed values; clicking another dial applies a typed value. Modules: analog-eq-unit 1.4.0, compressor-unit 1.4.0, reverb-unit 1.1.0.
- **1.16.1** (2026-10-19): output gain defaults to −6 dB, is last in the chain, is smoothed and is no longer set by presets; double-click value entry on Sound-tab knobs.

See `CHANGELOG.md` for the full history.
