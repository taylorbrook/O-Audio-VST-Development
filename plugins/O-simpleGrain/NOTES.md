# O-simpleGrain Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.1.0
- **Type:** Synth (Pedagogical Granular)

## Lifecycle Timeline

- **2026-06-24 (Stage 1–3):** Foundation → DSP → GUI implemented (8-voice granular engine, field-guide WebView UI).
- **2026-06-25 (v1.0.0):** First release — Stage 4 validation complete (render-harness 8/8 automated gates PASS, auval SUCCEEDED).
- **2026-06-25 (v1.0.1):** Code-review fixes — see CHANGELOG. Two correctness bugs (velToDensity 100× over-scale; user-source restore clobber), two RT hot-loop simplifications (precomputed pan gains + AA coefficient), and a new render-harness gate (`velToDensity-depth`). All 9 gates PASS, auval SUCCEEDED, installed (VST3 + AU).
- **2026-06-25 (v1.0.2):** User-reported bug fixes — see CHANGELOG. Wired the missing on-screen-keyboard MIDI bridge (`uiMidi` native fn + `MidiMessageCollector` merge — ported from O-simpleFM; keys were silent), replaced the fixed `kHeadroom=0.5` with √overlap normalization (sparse/default patches were ~6 dB too quiet), and the output scope is restored as a consequence (no scope code changed). New render gate (`ui-midi-keyboard`). All 10 gates PASS, auval SUCCEEDED, installed (VST3 + AU).
- **2026-06-25 (v1.1.0):** User request — ADSR on/off toggle. Investigation confirmed the amp envelope was *already* per-voice/polyphonic (8-voice synth, note-stealing, per-`GrainVoice` `juce::ADSR`), so no polyphony change was needed. Added `adsrEnabled` bool param (default on; backward-compatible). Off = flat velocity-scaled gate; note-off stops grain spawning and lets the active grains drain through their own Window envelopes (no declick). UI: "ADSR" toggle in the Amplitude Envelope panel that dims/locks the A/D/S/R knobs when off. Two new render gates (`adsr-toggle-bypass`, `adsr-off-drains`). All 12 gates PASS, auval SUCCEEDED, installed (VST3 + AU).

## Known Issues

- **"Held notes fade to silence" (2026-06-25) — NOT a code defect; stale host instance.** Reported during the v1.1.0 cycle (Logic @ 48k, with/without ADSR). Exhaustively non-reproducible in the render-harness (single/poly/retrigger, 44.1 & 48 k, all source/spray/scan, 6 s holds — all steady) and confirmed fade-free in the Standalone build. Root cause: installing while Logic was open left a stale cached plugin instance in memory (the build script clears the on-disk AU cache but cannot evict an open host's loaded copy). Resolution: fully quit/reopen the host after install. Guarded by `held-no-fade@48k`, `poly-no-fade`, `kbd-lifecycle`.

None blocking. Deferred polish (from the 2026-06-25 code review, optional 1.x follow-ups):
- √overlap normalization (v1.0.2) is a coarse global trim, not a limiter; extreme transposition stress (e.g. grainPitch +24 st + octave-up note) can still transiently exceed 0 dBFS. A true limiter remains a 2.x option.
- `std::atomic_load/store` on `shared_ptr` is deprecated in C++20 (works; optional migration to `std::atomic<std::shared_ptr<>>`).
- Stereo sources: only channel 0 is granulated (right channel dropped) — consider mono-summing on decode.
- `kRestEase` per-sample constant is sample-rate-dependent (minor playhead rest-ease variance across SR).

## Additional Notes

- 19-parameter APVTS; pedagogical "Naturalist" field-guide WebView UI with four live visualizations (grain cloud, source-waveform playheads, scope/spectrum, grain meter) and an 8-preset concept tour.
- Engine constants: 8 voices, 24 grains/voice, 192 global grain cap, root C3 (note 60), 10 s source cap, 2048-pt window LUTs.
- DSP correctness gate: `tests/render-harness/` (build with `-DOUARICON_BUILD_TESTS=ON`, target `O-simpleGrain-render-test`) — 15 automated gates.
- Installed (dev branding): `O-simpleGrain-dev.vst3` + `O-simpleGrain-dev.component`.
