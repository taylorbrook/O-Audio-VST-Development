# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-08-15
**Participants:** User, Claude

## Requirements Confirmed

- Wire the shared **preset-manager module v1.0.5** into the WebView UI (preset band, save/load/browse, factory presets). Fresh integration at the current version — no vendored-copy drift to inherit.
- Factory bank: **~8 presets** covering the 6 degradation families (Tape, CD, Vinyl, Packet, Codec, Crush/Quant), both sync and free clocking, and a couple of extreme/combo patches. Author list finalized at plan phase.
- Validation gate is **automated only**: pluginval strictness-10 VST3 + AU (run 2–3× locally per the latent-NaN pattern), auval, render harness 44/44 re-run. The carried manual DAW checklist (per-family LED semantics in Logic, dice/seed persistence, sync clocking against host tempo, Stage-2 listening items) stays open as non-gating user follow-ups — the O-Tapestop pattern.
- CHANGELOG.md created; version 0.1.0 → **bump to 1.0.0 at stage-4 verify**.
- Release target: **local install only** via `./scripts/build-and-install.sh O-Bitrot`. No /publish, no /package this cycle.
- EB Garamond woff2 bundling **declined** — system Garamond fallback stays.

## Constraints Identified

- Factory presets must be authored in **engineering units + convertTo0to1** — linear-fraction authoring ignores NormalisableRange skew (memory: `pattern_factory_preset_normalized_ignores_skew`). O-Bitrot has skewed ranges (skew centres set at Stage 1).
- **applyPresetJson must reset all 31 params to defaults first** so partial presets don't inherit stale state (memory: `pattern_preset_apply_needs_reset_to_defaults`).
- AsyncUpdater + restore-guard flag needs **cancelPendingUpdate()** — queued applies stomp restored params (memory: `pattern_asyncupdater_guard_flag_needs_cancel`).
- FileChooser completions (preset import/export) need **SafePointer**; `complete(false)` after editor teardown is itself a UAF (memory: `pattern_webview_launchasync_safepointer_no_complete`).
- Preset names containing "/" silently fail to save (name is the JSON filename) — sanitize.
- **Seed determinism contract:** presets carry SEED; preset apply is a message-thread param write and the audio thread reseeds all 8 RNG streams on seed change. No special preset path needed, but the harness must confirm apply doesn't break bit-identity per seed.
- Stage-2 DSP stays frozen: render harness must hold **44/44** after ANY processor edit (the preset apply path touches the processor — re-run the harness).
- preset-manager adds WebView native functions — the **grep-diff parity check** (getNativeFunction JS ↔ withNativeFunction C++) that Stage-3 verify established must pass again.
- `juce_add_binary_data` strips hyphens (`preset-manager.js` → `presetmanager_js`); if the module adds a second binary-data target it needs a **distinct NAMESPACE** (memories: `critical_binary_data_strips_hyphens`, `critical_dual_binary_data_namespace_collision`).
- Fixed 900×620 editor — the preset band must fit without resizing or crowding the 3×2 panel grid + Tab. VII global strip.
- Choice params (5) and bools (7) in presets: store engineering values, not indices-as-floats, per module conventions.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Preset system | preset-manager module v1.0.5 via module-add | Current shared module; preset band UI + factory/user presets; proven this week on O-Tapestop planning |
| Factory bank size | ~8 (per-family showcases + combos) | Covers 6 families, sync + free clocking, extremes without a large authoring/listening burden |
| Validation scope | Automated only (pluginval ×2–3, auval, harness 44/44) | Manual DAW checklists stay open as user-driven follow-ups; not gating |
| Windows CI | Not gating stage 4 | Local-only release; Windows pluginval deferred to a future publish cycle |
| Release | Local install via build-and-install.sh | Prove it in sessions before packaging/publishing |
| Version | 1.0.0 at stage-4 verify | Standard completion bump; CHANGELOG.md initialized |
| EB Garamond bundling | Declined | System Garamond fallback acceptable; deterministic typography deferred |

## Open Questions

- Preset-band placement in the fixed 900×620 Naturalist frame — above or below the 3×2 grid / global strip? (research: check module's default band height vs current layout margins; O-Tapestop research may already have the answer for its 860×580 frame)
- Does preset-manager v1.0.5 handle AudioParameterChoice/Bool serialization natively, or does O-Bitrot need adapter code for its 5 choices + 7 bools? (research: module docs + an existing integration, e.g. O-Tremolo)
- Factory preset author list: which 8 patches best showcase the families (research/plan: draft names + settings in engineering units)

## Next Phase

Ready for: research phase
