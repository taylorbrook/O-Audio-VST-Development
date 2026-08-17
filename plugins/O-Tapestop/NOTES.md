# O-Tapestop Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.3.3
- **Type:** Audio Effect (Tapestop/Start + Scratch/Continuous Varispeed)

## Lifecycle Timeline

- **2026-08-17 (v1.3.3):** Audit queue item 3 (B2a) — documentation only, zero
  DSP change (both DSP headers verified byte-identical from `#pragma once`
  onward). `SpliceLaw::EqualPower` is really an equal-GAIN law: `fadeOut =
  hann(0.5+phi/2) = cos^2(pi*phi/2)` and `fadeIn = hann(phi/2) =
  sin^2(pi*phi/2)` sum to 1 in AMPLITUDE, so the identity `sin^2 + cos^2 = 1`
  constrains the amplitude sum, not the power sum. Consequence, and the exact
  inverse of what the old comments claimed: the law is flat (0 dB) on
  CORRELATED material and DIPS on DECORRELATED material (power sum
  `(1 + cos^2(pi*phi))/2` → 0.5 at phi = 0.5, an analytic 3.01 dB floor). The
  old TapestopTransport.h note that it "over-sums CORRELATED material by up to
  +3 dB" described the TRUE equal-power law — the square roots of these gains
  — which is not implemented. Since resync splices the live-head rider against
  a voice seconds behind it, the dipping (decorrelated) case is the one in
  play. Enum name deliberately retained; whether to add a `sqrt` variant is
  audit item 4 (B2b).

  **Splice A/B evidence (harness, v1.3.3)** — this is item 4's step-1
  measurement, taken while verifying the doc fix, so item 4 does not need to
  repeat it:

  | law | bump | dip |
  |---|---|---|
  | `AB-splice-equal-power` (raised cosine, shipped) | −0.48 dB | **−6.21 dB** |
  | `AB-splice-linear` | −0.58 dB | **−6.99 dB** |

  Two things to carry forward. (1) The near-zero **bump** on both laws is the
  direct empirical confirmation that neither over-sums correlated material —
  the old "+3 dB" comment was describing the unimplemented sqrt law. (2) The
  **dip is ~6 dB, not the analytic 3.01 dB**: that floor assumes two
  equal-power decorrelated sources, whereas the fading voice is a varispeed
  read of different material at a different level. Do not quote 3 dB as the
  plugin's resync dip. The raised cosine beats linear by 0.78 dB, consistent
  with its power sum falling off more slowly away from the midpoint (0.750 vs
  0.625 at phi = 0.25) despite both sharing the same 0.5 floor at phi = 0.5.
  Note the probe asserts only `|bump| < 4.0`, so the dip is reported but
  ungated — tightening that is item 4 step 3.

- **2026-08-17 (v1.3.2):** Audit queue item 2 (B3) — the engaged-trim blend
  never landed on exactly 0 at the resync→Bypassed handoff. Two defects: (a)
  `trimTarget` was computed BELOW the crossfade-completion block, so the
  `ResyncXfade → Bypassed` flip retargeted the ramp to 1 on the final fade
  sample and trim ended at `2/xfLen` (8.3e-4 @ 48 kHz) — now latched between
  the state machine and the completion block, the only point that sees
  `ResyncXfade` on all `xfLenSamples` ticks (an entry-to-`tick()` latch, the
  audit's prescription, loses the first tick and lands on `1/xfLen`); (b) the
  ramp was a float accumulator whose rails were rate-dependent — exact at
  44.1/48/192 kHz but 4.07e-5 / 1.88e-5 / 6.73e-5 at 88.2/96/176.4 kHz — now an
  integer position over `[0, xfLenSamples]` with both rails snapped. New
  transport-level probe `B3-trim-exact-rails-6-rates` (rate-swept; verified
  FAILING against both defects independently). Output byte-identical at the
  0 dB OUTPUT_GAIN default; worst case 0.0025 dB during the 50 ms fade at
  +12 dB. Harness 67/67; pluginval s10 VST3 SUCCESS; auval SUCCEEDED. Installed.
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
