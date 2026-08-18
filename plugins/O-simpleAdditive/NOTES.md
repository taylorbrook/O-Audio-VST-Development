# O-simpleAdditive — Notes

Pedagogical 16-voice additive/wavetable-scan synth ("Additive Field Guide"):
16 drawbars → band-limited single-cycle table per note, Frame A→B spectral
morph, spectral-decay tilt, bit-depth quantizer.

## Timeline

- **2026-06-22 — v1.0.0** — Stage 4 complete; pluginval strictness 8 (VST3 + AU)
  and auval pass. Preset save/load bar deferred to v1.1.
- **2026-06-25 — v1.0.1** — Code-quality bundle: band-limited live drawbar glow,
  robust `frameBSource` resolve, single source of truth for partial IDs and
  lesson captions.
- **2026-06-25 — v1.0.2** — Refill-cadence cap (~5 ms) for continuous wavetable
  motion: 64-sample-block MOVING CPU 36.7 % → 9.0 % of a core (16-voice chord);
  static patches bit-identical. Added `tests/render-harness/` (profile + golden
  battery, `-DOUARICON_BUILD_TESTS=ON`).
  ⚠ v1.0.1/v1.0.2 source was never committed and the tree silently reverted to
  v1.0.0; recovered 2026-07-15 from `backups/O-simpleAdditive/v1.0.2/`.
- **2026-07-15 — v1.0.3** — Code-review resolution (CODE_REVIEW.md WR-01..06):
  sine-LUT init moved off the audio thread, spectral-decay knob dead-zone fixed
  (rate changes re-dirty the table after tau saturates; rate→0 restores
  brightness), lesson-preset writes gestured, pointer-capture stuck-drag fix,
  stuck-note panic path, version drift reconciled (restored lost v1.0.2 then
  bumped).
- **2026-07-15 — v1.0.4** — Code-review resolution part 2 (CODE_REVIEW.md
  IN-01..06): removed dead `getSampleRate` bridge endpoint and dead `currentNote`
  member, `Kmax = 0` silence for f0 ≥ Nyquist (no aliased fundamental at exotic
  sample rates), `uiMidi` boundary validation (note clamp + NaN velocity guard),
  `midiCollector` timestamp base seeded at construction, ARIA value attributes
  (+ knob labels) on all `role="slider"` controls. Render-harness golden battery
  and auval pass.
- **2026-08-17 — v1.0.5** — UI layout pass: the whole page now fits the 860×980
  editor without scrolling. The four stacked control groups became two horizontal
  rows (`.group-row`), which cut `.controls` from 542 px to 247 px and closed a
  277 px overflow that had put the lesson-preset row and the entire on-screen
  keyboard below the fold. Reclaimed space was spent back on the design — drawbars
  keep their full 168 px travel, knobs their 56/48 px diameters, and the keyboard
  is now taller (96 px, was 92). The oscilloscope is the one elastic section
  (`flex: 1 0 136px`), absorbing leftover slack so the keyboard sits on the bottom
  edge. Measured 912 px of content in 974 px usable (62 px headroom, ≥59 px across
  every serif fallback); verified in Standalone/WKWebView and auval.

## Known Limitations

Deferred from Stage 4: preset save/load bar (planned v1.1).

All findings from the 2026-07-15 code review (CR: none, WR-01..06, IN-01..06)
are now resolved — WR in v1.0.3, IN in v1.0.4.
