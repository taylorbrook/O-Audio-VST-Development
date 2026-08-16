# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-08-15
**Participants:** User, Claude

## Requirements Confirmed

- Wire the shared **preset-manager module v1.0.5** into the WebView UI (preset band, save/load/browse, factory presets). This is the current module version — no vendored-copy drift to inherit.
- The scratch **envelope JSON blob must ride inside preset JSON** (save + apply), not just APVTS params.
- Factory bank: **~8 presets** — the 4 roadmap-named ones (classic 1/2-bar stop, DJ spinup, 2 scratch gestures) plus ~4 extras covering tempo-synced stops, slow-tape drag, and a stutter-scratch. Both modes and sync/free timing represented.
- Validation gate is **automated only**: pluginval strictness-10 VST3 + AU (run 2–3× locally per the latent-NaN pattern), auval, render harness 47/47 re-run. Manual DAW checklist items from stages/2-dsp and 3-gui VERIFICATION.md remain open and are NOT a stage-4 gate.
- CHANGELOG.md created; version stays 0.1.0 → bump to 1.0.0 at stage-4 verify.
- Release target: **local install only** via `./scripts/build-and-install.sh O-Tapestop`. No /publish, no /package this cycle.

## Constraints Identified

- Factory presets must be authored in **engineering units + convertTo0to1** — linear-fraction authoring ignores NormalisableRange skew (memory: `pattern_factory_preset_normalized_ignores_skew`).
- **applyPresetJson must reset all params to defaults first** so partial presets don't inherit stale state (memory: `pattern_preset_apply_needs_reset_to_defaults`).
- AsyncUpdater + restore-guard flag needs **cancelPendingUpdate()** — queued applies stomp restored params (memory: `pattern_asyncupdater_guard_flag_needs_cancel`).
- FileChooser completions (preset import/export) need **SafePointer**; `complete(false)` after editor teardown is itself a UAF (memory: `pattern_webview_launchasync_safepointer_no_complete`).
- Preset names containing "/" silently fail to save (name is the JSON filename) — sanitize.
- Stage-2 DSP stays frozen: render harness must hold 47/47 after ANY processor edit (preset apply path touches the processor — re-run the harness).
- Preset apply must also push the sanitized envelope echo to the UI (generation-counter path already exists from stage 3).
- WebView native-fn additions must pass the grep-diff parity check (JS ↔ C++) that stage-3 verify established.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Preset system | preset-manager module v1.0.5 via module-add | Current shared module; preset band UI + factory/user presets; envelope blob extension point exists |
| Factory bank size | 8 (roadmap 4 + 4 extras) | Covers both modes, sync + free timing, curve extremes without a large authoring/listening burden |
| Validation scope | Automated only (pluginval ×2–3, auval, harness) | Manual DAW checklists stay open as user-driven follow-ups; not gating |
| Windows CI | Not gating stage 4 | Local-only release; Windows pluginval deferred to a future publish cycle |
| Release | Local install via build-and-install.sh | Prove it in sessions before packaging/publishing |
| Version | 1.0.0 at stage-4 verify | Standard completion bump; CHANGELOG.md initialized |

## Open Questions

- Does preset-manager v1.0.5 need a schema extension for the envelope blob, or does its opaque-extra-state hook cover it? (research phase: check module docs + how O-TextureForge/O-Polystutter carry non-APVTS state in presets)
- Exact preset-band placement in the 860×580 naturalist frame without crowding the mode-switched center panel (research: check module's default band height + O-Bitrot's stage-3 layout if it already integrated one)

## Next Phase

Ready for: research phase
