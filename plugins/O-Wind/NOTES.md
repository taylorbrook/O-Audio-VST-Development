# O-Wind — Development Notes

**Status:** 📦 Installed
**Version:** 1.16.1
**Type:** Synth (Physical Model Flute) — 2× oversampled jet-drive waveguide (Verge 1995)

## Known Limitations

- **attackChiff, humanize, vibratoOnset, inharmonicity have no UI knobs (deliberate,
  WR-13, 2026-07-10).** All four are automatable and set by every factory preset, but
  are exposed only via the host's generic parameter list. Adding knobs is a candidate
  future MINOR (needs Sound/Expression tab layout work).
- **Deferred review findings (IN-01..IN-19, 2026-07-09 review):** info-level items
  from CODE_REVIEW.md were intentionally left unresolved in the v1.16.1 sweep.
  Notables: per-block string-keyed APVTS lookups in the voice (IN-01 — cache pointers
  like the processor's `fxCache`), dead ToneHoleSystem/SubHarmonics scaffolding and
  no-op `toneHoleToggle` preset values (IN-04), CC overrides can't return control to
  the knob or reach zero (IN-05), FX `mix > 0.001` gating freezes tails (IN-07),
  hand-built JSON in tuning native fns doesn't escape strings (IN-12), classic-script
  tag for the ES module logs a benign SyntaxError (IN-11), single shared `fileChooser`
  drops a first dialog if a second opens (IN-19). See CODE_REVIEW.md for the full
  list.
- **Rank-2 / Save SCL / Save KBM buttons require v1.16.1+** — the native fns did not
  exist before (silently dead buttons).

## Timeline

- **2026-07-10 — v1.16.1:** CODE_REVIEW.md resolution sweep. All 8 criticals + 13
  warnings resolved: Effects tab and 4 Sound-tab knobs wired (relays/attachments were
  never written in v1.14.0/v1.12.0), RT allocations removed from the voice filter
  path, ADSR-disable stuck-drone fixed, tuning-state persistence added
  (`setCustomStateCallbacks`), CMake `VERSION` keyword fixed (bundle reported 1.0.0),
  FileChooser SafePointer completions, missing tuning native fns registered,
  `airColumn` wired (neutral at default), delay-line capacities rate-derived,
  delay/reverb parameter smoothing, skew-aware dblclick reset, readouts via
  `getScaledValue()`, master-tune knob synced + routed through `referencePitch`.
  Shared-module fix in `scala-tuning-engine/js/tuning-panel.js` (WR-10) propagates to
  sibling plugins on their next build.
- **2026-04-26 — v1.16.0:** VST3 Note Expression microtonal support for Dorico.
- **2026-04-13 — v1.15.x:** ADSR release fix; Sound tab layout.
- **v1.14.0:** Effects chain (Chorus/Delay/EQ/FDN Reverb) — UI wiring completed in
  v1.16.1.
- **v1.12.0:** Vibrato drift + growl/formant — UI wiring completed in v1.16.1.
