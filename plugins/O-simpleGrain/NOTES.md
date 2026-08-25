# O-simpleGrain Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.2.1
- **Type:** Synth (Pedagogical Granular)

## Lifecycle Timeline

- **2026-06-24 (Stage 1–3):** Foundation → DSP → GUI implemented (8-voice granular engine, field-guide WebView UI).
- **2026-06-25 (v1.0.0):** First release — Stage 4 validation complete (render-harness 8/8 automated gates PASS, auval SUCCEEDED).
- **2026-06-25 (v1.0.1):** Code-review fixes — see CHANGELOG. Two correctness bugs (velToDensity 100× over-scale; user-source restore clobber), two RT hot-loop simplifications (precomputed pan gains + AA coefficient), and a new render-harness gate (`velToDensity-depth`). All 9 gates PASS, auval SUCCEEDED, installed (VST3 + AU).
- **2026-06-25 (v1.0.2):** User-reported bug fixes — see CHANGELOG. Wired the missing on-screen-keyboard MIDI bridge (`uiMidi` native fn + `MidiMessageCollector` merge — ported from O-simpleFM; keys were silent), replaced the fixed `kHeadroom=0.5` with √overlap normalization (sparse/default patches were ~6 dB too quiet), and the output scope is restored as a consequence (no scope code changed). New render gate (`ui-midi-keyboard`). All 10 gates PASS, auval SUCCEEDED, installed (VST3 + AU).
- **2026-06-25 (v1.1.0):** `adsrEnabled` envelope-bypass toggle (19th param) — see CHANGELOG.
- **2026-07-16 (source recovery):** Discovered the working tree had been reverted to v1.0.1 with the v1.0.2/v1.1.0 source uncommitted — the installed 1.1.0 binary was the only surviving v1.1.0 artifact. Restored v1.0.2 from `backups/O-simpleGrain/v1.0.2/`; recovered the v1.1.0 UI byte-exact from the installed binary's embedded resources and re-implemented its C++ side from the recovered spec.
- **2026-07-16 (v1.1.1):** CODE_REVIEW.md resolution (CR-01..02, WR-02..05; WR-01 was the v1.0.2 fix resurfacing via the reverted tree) — see CHANGELOG. Dropped-source survival across re-prepare (retained bytes + keep-live), sourceStateLock for the identity race, pre-decode 10 s cap, presets-keep-source contract (Granular Fire force-loads), single-sourced version, lock-free audio-thread source view (retired-list reap). New render gate (`adsr-bypass`). All 11 gates PASS, auval SUCCEEDED, pluginval strictness-10 SUCCESS, installed (VST3 + AU).
- **2026-07-16 (v1.1.2):** CODE_REVIEW.md deferred-findings resolution (IN-01..IN-09) — see CHANGELOG. Spray-spawn wrap (no more edge-pinned DC thumps), τ-derived rest-ease (rate-independent glide), double read positions, event-driven UI source refresh (`sourceChanged`), cached typed voice pointers (no per-block RTTI), skip-unchanged-rate re-decode, dead member removed, grain cap pushed via initialisation data, gestured preset writes. All 11 gates PASS, auval SUCCEEDED, pluginval strictness-10 SUCCESS, installed (VST3 + AU).
- **2026-08-08 (v1.1.3):** First published cross-platform release (GitHub Actions; AGPL-3.0 headers) — see CHANGELOG.
- **2026-08-09 (v1.2.0):** Header/tooltip UX — see CHANGELOG. Title no longer line-breaks (title block `flex-shrink: 0` + nowrap; preset bar pushed right), new "?" button toggles tooltips (persisted, default on), duplicate tooltips fixed (native `title=` fallback replaced with `aria-label`). UI-only; installed (VST3 + AU).
- **2026-08-25 (v1.2.1):** Note-off click fix (ported from O-simpleFM v1.2.5) — see CHANGELOG. Per-block `juce::ADSR::setParameters()` clobbered the note-off release rate; with sustain 0 it hard-reset the envelope one block after note-off (audible click at any release setting). GrainVoice now pushes envelope params only on value change and never mid-release. New render gate (`noteoff-click`, negative-control verified against v1.2.0). All 12 gates PASS, installed (VST3 + AU).

## Known Issues

None blocking. Deferred polish (optional follow-ups):
- √overlap normalization (v1.0.2) is a coarse global trim, not a limiter; extreme transposition stress (e.g. grainPitch +24 st + octave-up note) can still transiently exceed 0 dBFS. A true limiter remains a 2.x option.
- Stereo sources: only channel 0 is granulated (right channel dropped) — consider mono-summing on decode.
- A dropped source larger than the 32 MB retention cap survives re-prepares, but a sample-rate change mid-session plays it transposed (bytes not retained to re-resample). A restored session referencing a dropped file in a *fresh* instance still falls back to fire with a notice (bytes are session-local by design).

(The 2026-07-15 review is now fully resolved: CR/WR in v1.1.1, IN-01..IN-09 in v1.1.2.)

## Additional Notes

- 19-parameter APVTS; pedagogical "Naturalist" field-guide WebView UI with four live visualizations (grain cloud, source-waveform playheads, scope/spectrum, grain meter) and an 8-preset concept tour.
- Engine constants: 8 voices, 24 grains/voice, 192 global grain cap, root C3 (note 60), 10 s source cap, 2048-pt window LUTs.
- DSP correctness gate: `tests/render-harness/` (build with `-DOUARICON_BUILD_TESTS=ON`, target `O-simpleGrain-render-test`) — 11 automated gates.
- Installed (dev branding): `O-simpleGrain-dev.vst3` + `O-simpleGrain-dev.component`.
