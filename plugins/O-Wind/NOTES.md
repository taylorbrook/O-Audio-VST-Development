# O-Wind — Development Notes

**Status:** 📦 Installed
**Version:** 1.16.3
**Type:** Synth (Physical Model Flute) — 2× oversampled jet-drive waveguide (Verge 1995)

## Known Limitations

- **attackChiff, humanize, vibratoOnset, inharmonicity have no UI knobs (deliberate,
  WR-13, 2026-07-10).** All four are automatable and set by every factory preset, but
  are exposed only via the host's generic parameter list. Adding knobs is a candidate
  future MINOR (needs Sound/Expression tab layout work).
- **`toneHoleToggle` ("Tone Hole") is a no-op (IN-04, 2026-07-10).** Tone-hole
  scattering DSP was never implemented; the v1.16.2 sweep removed the dead
  scaffolding (`ToneHoleSystem.h`, bore delay table) and factory-preset values, but
  the param and UI toggle are kept so existing sessions/automation stay valid.
  Either implement tone-hole DSP (MINOR) or remove the param + toggle (MAJOR,
  breaking) in a future release.
- **Rank-2 / Save SCL / Save KBM buttons require v1.16.1+** — the native fns did not
  exist before (silently dead buttons).

## Timeline

- **2026-07-10 — v1.16.3:** Final info-finding sweep (IN-01, IN-08..10, IN-12..15,
  IN-17 via /improve-review) — CODE_REVIEW.md fully resolved (40/40): voice +
  processor raw-param-pointer caches (no string-keyed APVTS lookups on the audio
  thread), 8 dead native-fn registrations removed (getMasterTune kept — used since
  WR-10), tuning JSON via juce::JSON, shared knob-drag handler (52→2 document
  listeners), preset-selector count from numSteps, wheel-edit drag gestures,
  exportTuningHTML write-result reporting, StereoWidth reset smoothing fix,
  preset-manager module v1.0.4 (factory preset name sanitization).
- **2026-07-10 — v1.16.2:** Opt-in info-finding sweep (IN-02..07, IN-11, IN-18,
  IN-19 via /improve-review): CC-seen latches so breath controllers can reach zero,
  FX tail-out instead of hard mix-gating, per-voice oversampler reset, voiceRng on
  the audio thread, dead scaffolding removal (ToneHoleSystem/SubHarmonics/bore delay
  table/unused preset fields), dead APVTS listeners removed, file-dialog re-entry
  guard, classic-script tag removed.
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
