# Stage 3 (GUI) — Verification

**Date:** 2026-06-22
**Verdict:** ✓ PASS

## Build / validation gates

| Gate | Result |
|------|--------|
| VST3 build | ✓ clean (link OK, ad-hoc signed) |
| AU build | ✓ clean |
| `auval -v aumu OSiA OuDv` | ✓ **AU VALIDATION SUCCEEDED** (incl. Test MIDI render) |
| Install (dual-variant sweep) | ✓ VST3 + AU installed to user plug-in folders |
| Standalone render | ✓ WebView loads, full UI renders (screenshot-verified) — no blank UI |

## Test criteria (ROADMAP §Stage 3)

### Phase 3.1
- [x] WebView opens, single-page layout renders, classroom/projector-readable.
- [x] All 16 drawbars + all knobs/menus present and two-way bound (relays → attachments,
      proven O-simpleFM pattern; `jassert(param)` guards against ID drift).
- [x] Renders on macOS (VST3+AU). Windows path wired (NEEDS_WEBVIEW2 + static-linking flag
      + `withUserDataFolder`) — not built here (Windows CI gate).

### Phase 3.2
- [x] The 16 bars are the primary control surface AND read as the live spectrum (live glow
      from the exact morphed+decayed active-spectrum snapshot).
- [x] Oscilloscope wired to `scopeUpdate`; DPR-aware canvas; analyzer copies the scope window
      before the in-place FFT (no corruption).
- [x] No audio-thread FFT/alloc — analyzer runs on the 30 Hz message-thread Timer; audio
      thread is copy-only into the lock-free VizRing (unchanged from Stage 2).

### Phase 3.3
- [x] Every parameter has a working hover tooltip (incl. per-partial harmonic tips).
- [x] 6 lesson presets load via `applyFactoryPreset` (full APVTS snapshots; UI syncs through
      relays); captions isolate each concept.
- [x] Layout stays single-page (frame scrolls within the fixed border on short screens).

## Visual confirmation (Standalone screenshot)
Title + Field Guide subtitle; 16 drawbars with H1 full brass / H2–16 empty (= default pure
sine); oscilloscope; Morph·Wavetable (Frame B="Saw"), Spectral Shaping (Bit Depth="Off"),
Amp Env, Mod Env→Scan, Output groups; botanical overlay. Matched sibling aesthetic.

## Not regressed
- Default patch unchanged (H1=100%, rest 0 → pure sine). Stage 2 DSP untouched except the
  additive on-screen-keyboard MIDI drain (`removeNextBlockOfMessages`) and the read-only
  `isSounding`/`getCurrentSampleRate` accessors — no change to the render path.

## Residual (manual, in-DAW)
- Interactive drag / live-glow / scope-morph / audible lesson presets / on-screen keyboard
  best confirmed by playing in a DAW or the open Standalone (auval covers headless render +
  MIDI; the viz + keyboard follow the shipped O-simpleFM wiring verbatim).
