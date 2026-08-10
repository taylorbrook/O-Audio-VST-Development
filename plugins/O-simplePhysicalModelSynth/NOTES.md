# O-simplePhysicalModelSynth Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.1.0
- **Type:** Synth (Pedagogical Physical Modeling)

## Lifecycle Timeline

- **2026-08-09 (v1.1.0):** Added "?" tooltip toggle button to the header (right of the preset bar) via /improve — toggles on-hover tooltips, persists via localStorage, `aria-pressed` state. Same pattern as O-simpleGrain v1.2.0.
- **2026-08-08 (v1.0.3):** Maintenance patch — MSVC SafePointer init-capture hoist for Windows CI + AGPL-3.0 notice headers. First published release since v1.0.1.
- **2026-06-26 (Stage 0):** Research & Planning complete — ARCHITECTURE.md + ROADMAP.md. Complexity 5.0, staged implementation.
- **2026-06-27 (Stages 1–4, v1.0.0):** Implemented & shipped. 3 exciters (Pluck/Strike/Bow) × 2 resonators (KS String/Modal), Material macro, 6 concept-isolating factory presets, WebView UI (energy-loop diagram, spectrum, scope, preset bar, on-screen keyboard). Validation: render harness 22/22, pluginval L10 VST3+AU, auval PASS.
- **2026-07-16 (v1.0.2):** VR-01 (verify-pass residual of CR-03) via /improve: `setStateInformation` now calls `cancelPendingUpdate()` while `restoringState` is up, so a Material-macro AsyncUpdate queued from audio-thread automation before a restore can't fire afterwards and stomp restored Damping/Decay (destructor already had the pattern). Render harness 22/22 ALL PASS post-fix.
- **2026-07-16 (v1.0.1):** Resolved all Critical + Warning findings from the 2026-07-15 deep code review (CODE_REVIEW.md CR-01..CR-03, WR-01..WR-05) via /improve-review. CR-01: String-keyed param map replaced with named cached atomics (~32 heap allocs/block on the audio thread → 0). CR-02: FileChooser `launchAsync` completions guarded with `Component::SafePointer` + bare return (UAF fix, W12 pattern). CR-03: Material macro no longer stomps explicitly saved Damping/Decay — preset-manager module v1.0.5 applies meta params first; `restoringState` flag suppresses the macro during session restore. WR-01: hard voice stop (CC120/steal/releaseResources) now resets env + string + modal (voice actually silences, stops burning CPU). WR-02: macro listener defers to the message thread via AsyncUpdater when fired from host automation on the audio thread. WR-03: KS delay sized `fs/8+100` to match the 8 Hz frequency clamp (low notes no longer pin at ~19 Hz). WR-04: pitch bend (±2 st) wired into `computeF0` — wheel was previously dead. WR-05: `getTailLengthSeconds` 5 s → 30 s (resonator-dominated tail, T60 ≈ 6904/f0 at max decay). Validation: render harness 22/22 ALL PASS (tuning C1 −0.00¢ / C7 1.76¢ unchanged), auval PASS, pluginval L10 SUCCESS VST3+AU.

## Known Issues / Limitations

- **IN-01..IN-10 from CODE_REVIEW.md are open (Info tier, opt-in).** Notables: Bow mode leaves Color/Position/Vel→Bright lit but inert (IN-01); "Waveguide" string-model option is a selectable no-op until v1.1 (IN-02); no double-click knob reset (IN-09); Save dialog ignores the chosen directory (IN-10).
- Very low notes at max decay can exceed the reported 30 s tail (accepted approximation, WR-05).
- DSP-06 (dual-rail waveguide string) deferred to v1.1.

## Additional Notes

Teaching synth that makes excitation→resonator synthesis legible: pick how a string/bar is driven (Pluck/Strike/Bow) and what resonates (KS String/Modal bank), swap either mid-note, and watch the energy-recirculation loop, spectrum, and scope respond in lockstep with the sound. Material macro co-moves Damping+Decay along the steel↔nylon axis.

See `.planning/` for ARCHITECTURE.md/ROADMAP.md; CODE_REVIEW.md (2026-07-15) retained as the review record.
