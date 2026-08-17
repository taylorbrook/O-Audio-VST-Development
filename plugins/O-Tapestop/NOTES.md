# O-Tapestop Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.3.1
- **Type:** Audio Effect (Tapestop/Start + Scratch/Continuous Varispeed)

## Lifecycle Timeline

- **2026-08-17 (v1.3.1):** Audit queue item 1 (B1) — scratch LUT
  double-buffer race. The transport latched a raw `const float*` into one of
  ScratchEnvelope's two bake buffers at the engage edge and read it for the
  whole pass (up to 8 s), so the second curve edit inside one pass baked over
  the buffer the audio thread was reading — torn envelope + data race.
  TapestopTransport now memcpys the LUT into its own `std::array<float,2048>`
  in `engageScratch()`. New harness probe
  `scratch-envelope-edit-midpass-inert` (verified FAILING pre-fix at sample
  52096 = the second commit). Harness 66/66; pluginval s10 VST3 ×2 SUCCESS;
  auval SUCCEEDED. Installed.
- **2026-08-16 (v1.3.0):** Factory bank doubled to 28 presets (+5 Tape
  Stops, +4 Scratch with new envelope blobs, +3 Wobble & Warp, +2 Glitch &
  Chaos) and a themed preset dropdown added on the preset-name readout
  (4 factory themes + dynamic User group; display-side map in js/app.js,
  shared preset-manager module untouched; carousel unchanged). Browser
  harness verified (stubbed JUCE bridge); pluginval s10 VST3 SUCCESS;
  auval SUCCEEDED. Installed.
- **2026-08-16 (v1.2.2):** UI fix — Continuous pane overflowed the center
  panel (~20 px both sides; CHARACTER caption widened its column past the
  gap budget). Pane gap 34 → 16 px; measured 7.3 px clearance per side.
  pluginval s10 VST3 ×2 + AU SUCCESS; auval SUCCEEDED. Installed.
- **2026-08-16 (v1.2.1):** Glitch pushed further off the grid — slot-start
  jitter (0–35 % of a slot), 1→5 slots per cell, ⅛-cell micro-bursts,
  tame-family fade 65 %. All gated above CHAOS 0.5 (≤ 0.5 bit-identical to
  v1.2.0). No new params. Harness 65/65; pluginval s10 VST3 ×2 + AU ×2
  SUCCESS; auval SUCCEEDED. Installed.
- **2026-08-16 (v1.2.0):** Glitch character overhauled — sub-cell event
  slots (1→4 with chaos), burst-length events, new palette (freeze, slam,
  chatter, shuffle, stutter roll/pitch-ramp), chaos remap. No new params.
  Harness 65/65; pluginval s10 VST3 ×2 + AU ×2 SUCCESS; auval SUCCEEDED.
  Installed.
- **2026-08-16 (v1.1.0):** Continuous mode added (Wobble / Random / Glitch
  characters, CHARACTER + CONT_RATE/DEPTH/CHAOS params, preset-migration
  hook for the MODE choice append, 6 new factory presets). Harness 62/62;
  pluginval s10 VST3 ×2 + AU ×2 SUCCESS; auval SUCCEEDED. Installed.
- **2026-08-16:** Installed to system folders (VST3 + AU)
- **2026-08-15:** Stage 4 (Polish) verified — plugin complete at v1.0.0

## Known Issues

- None

## Additional Notes

**Installation Locations:**
- VST3: `~/Library/Audio/Plug-Ins/VST3/O-Tapestop-dev.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/O-Tapestop-dev.component`

**Formats:** VST3, AU, Standalone

Dev branding — bundles carry the `-dev` suffix (AU triple `aufx OTsp OuDv`). Release
branding (unsuffixed `O-Tapestop`) is produced by CI only.
