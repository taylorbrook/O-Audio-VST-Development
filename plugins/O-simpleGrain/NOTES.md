# O-simpleGrain Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.0.1
- **Type:** Synth (Pedagogical Granular)

## Lifecycle Timeline

- **2026-06-24 (Stage 1–3):** Foundation → DSP → GUI implemented (8-voice granular engine, field-guide WebView UI).
- **2026-06-25 (v1.0.0):** First release — Stage 4 validation complete (render-harness 8/8 automated gates PASS, auval SUCCEEDED).
- **2026-06-25 (v1.0.1):** Code-review fixes — see CHANGELOG. Two correctness bugs (velToDensity 100× over-scale; user-source restore clobber), two RT hot-loop simplifications (precomputed pan gains + AA coefficient), and a new render-harness gate (`velToDensity-depth`). All 9 gates PASS, auval SUCCEEDED, installed (VST3 + AU).

## Known Issues

None blocking. Deferred polish (from the 2026-06-25 code review, optional 1.x follow-ups):
- Dense clouds can clip — fixed `kHeadroom=0.5`, no limiter; peak can reach ~+12 dBFS under max load.
- `std::atomic_load/store` on `shared_ptr` is deprecated in C++20 (works; optional migration to `std::atomic<std::shared_ptr<>>`).
- Stereo sources: only channel 0 is granulated (right channel dropped) — consider mono-summing on decode.
- `kRestEase` per-sample constant is sample-rate-dependent (minor playhead rest-ease variance across SR).

## Additional Notes

- 18-parameter APVTS; pedagogical "Naturalist" field-guide WebView UI with four live visualizations (grain cloud, source-waveform playheads, scope/spectrum, grain meter) and an 8-preset concept tour.
- Engine constants: 8 voices, 24 grains/voice, 192 global grain cap, root C3 (note 60), 10 s source cap, 2048-pt window LUTs.
- DSP correctness gate: `tests/render-harness/` (build with `-DOUARICON_BUILD_TESTS=ON`, target `O-simpleGrain-render-test`) — 9 automated gates.
- Installed (dev branding): `O-simpleGrain-dev.vst3` + `O-simpleGrain-dev.component`.
