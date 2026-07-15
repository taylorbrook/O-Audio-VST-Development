# O-Texture — Development Notes

**Status:** 📦 Installed
**Version:** 0.1.2

## Known Limitations

- **Placeholder models.** The embedded decoder is a placeholder; real trained
  models are pending. Only the Rain source has a dim map — the other five source
  buttons and Transform mode are disabled in the UI ("Coming soon"). The SOURCE/MODE
  parameters and sidechain bus are kept so the parameter contract is stable when
  models land (CODE_REVIEW.md WR-07 resolution).
- **First-hop silence in realtime.** With async inference the first decoded block
  lands at the first hop boundary (~43 ms @ 48 kHz), so a fresh transport start has
  one hop of silence. Offline bounces decode synchronously and are unaffected.
- **MIDI input declared but unused (IN-07, constrained).** An `aumu` MusicDevice
  must accept MIDI — auval fails `MusicDeviceMIDIEventList` (error -4) with
  `NEEDS_MIDI_INPUT FALSE`, verified during v0.1.2. The declaration stays until
  MIDI triggering is actually implemented; hosts route MIDI to the plugin with no
  effect.
- **ANIRA fetch retained (build-time only).** As of v0.1.2 `libanira` is no longer
  linked or embedded; the FetchContent step remains solely as the per-platform
  ONNX Runtime downloader (`ANIRA_ONNXRUNTIME_SHARED_LIB_PATH`). Replace with a
  direct ORT fetch if ANIRA is ever dropped from the monorepo.
- **Stale `libanira.*` in incremental build trees.** Bundles built before v0.1.2
  keep orphaned `libanira` dylibs in `Contents/Frameworks/` (`copy_if_different`
  never removes). Swept manually for this release; a clean rebuild produces
  correct bundles.

## Timeline

- **2026-07-15 — v0.1.2**: Resolved CODE_REVIEW.md IN-01, IN-03–IN-10. ANIRA
  unlinked (ORT linked directly; fetch kept as ORT provisioner), arch-correct ORT
  dylib path, encoder/prior.onnx dropped from BinaryData, dead `decoderReady` and
  unused JS removed, Perlin cursors wrap at period 256, DPR-aware ResizeObserver
  canvas sizing, dead read-loop fallback → jassert+break. IN-07 investigated and
  kept: aumu requires MIDI input (auval-verified), now documented at both sites.
  auval + pluginval (strictness 5) PASS.
- **2026-07-15 — v0.1.1**: Resolved CODE_REVIEW.md CR-01, CR-02, WR-01–WR-07.
  Inference moved to a background thread (seq-counter handoff, offline-sync path
  for deterministic bounces); stereo-only bus layout enforced + TiltFilter channel
  clamp; Perlin state staging/snapshot (no host↔audio races, restore survives
  prepareToPlay); inactive latent dims fixed per seed (FREEZE freezes, projects
  reproduce); latency 6144→0; MSVC-safe HannWindow; per-control double-click reset
  defaults; unimplemented SOURCE/MODE controls greyed out. auval + pluginval
  (strictness 5) PASS.
- **2026-02-15 — v0.1.0**: Initial implementation (see CHANGELOG.md).
