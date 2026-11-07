# Stage 3: GUI — Phase 3.2 (Buffer display + live grains) — Context

## Discussion Summary

**Date:** 2026-11-06
**Participants:** Taylor, Claude
**Inputs:** ROADMAP.md Phase 3.2; REQUIREMENTS UI-02; VERIFICATION-3.1.md (Phase 3.1 ✓, buffer strip empty = expected stub); `Source/ui/public/index.html` `drawBuffer()` / grains-readout consumer; `PluginProcessor.h` `Telemetry` struct

## Starting state (observed)

- `Telemetry` already declares the 3.2 fields: `hasBuffer`, `frozen`, `spanSec`, `wave[kWaveBins = 464]` + `waveCount`, `grains[kMaxGrains = 128]` (stream, delaySec, pitchSt, pan, env, gain, lengthSec) + `grainCount`. `readTelemetry()` is relaxed-atomics only and always returns true (no seqlock yet).
- `counts[4]` + `limit` (`hasCounts`) are tagged 3.3 in the struct.
- `getWindowCurve` is already real (R107, 3.1). It is not 3.2 scope.
- Page consumer (`drawBuffer`, v2 locked):
  - x axis: `xOf(sec)` puts "now" at the right edge (Rr = 940) and `spanSec` at the left edge (L = 14). `span` falls back to `scaled('buffer_length')` when `spanSec ≤ 0`.
  - `wave[i]` runs left to right, oldest to newest. It is resampled to 926 columns and drawn as a linear amplitude `min(1, v)`.
  - Grain dot: x = `delaySec` (time behind the head), y = `pitchSt / 24` plus a `pan × 6` offset, radius from `lengthSec`, alpha from `env × gain`. Dots with `delay > span` are skipped.
  - Position/spray bands currently read the BASE knob values (`scaled(sN_position / sN_spray)`).
  - The "Grains sounding N / limit" readout plus the per-stream bars read `t.counts` / `t.limit`.
  - The frozen caption uses `spanSec` for "held for N s". In 3.1 it showed the Buffer length knob (HANDS-ON-3.1 §H).

## Requirements Confirmed

- **UI-02** (should): buffer waveform display with write head, per-stream position/spray regions, and live grain indicators.
- Waveform overview comes from a peak ring written on the audio thread. It shows the write head, and the held span when frozen.
- Grain dots come from a seqlock snapshot, pulled by the page at ~30 Hz. On a torn read, `readTelemetry` returns false and the page keeps its last frame.
- Canvas text repaints after `document.fonts.load()` (already in place from 3.1, re-check in hands-on).

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| D30 | Position/spray bands | **Modulated values.** Telemetry carries the effective (post-mod) position + spray for each stream. The page's `drawBuffer` reads them, with a fallback to the base knob values when the field is absent. This is a **sanctioned D28 exception**: a scoped page edit, and all five UI gates plus the binding probe must stay green. | The ROADMAP specifies modulated values. With base values, the bands sit still while LFOs move the grains. |
| D31 | Waveform amplitude scale | **dB-mapped in C++:** `v = clamp((20·log10(peak) + 48) / 48, 0, 1)` (−48 dB floor, the same floor as the output meter). The page is unchanged and still draws `min(1, v)`. | Quiet material (−30 dBFS) is otherwise visually flat. Mapping on the C++ side keeps the page contract intact. |
| D32 | Grains-sounding counts | **Pulled into 3.2.** `counts[4]` + `limit` (`hasCounts`) are filled from the same grain snapshot. | It is the same data source, so the buffer-strip area is complete in one phase. 3.3 keeps meters / LFO phase / onset / loop gain. |
| D33 | Verification | **Automated gates + HANDS-ON-3.2.md** (Standalone): live waveform, write head, freeze shows the held span plus the dashed head, grain dots track streams / pitch / pan, bands move under LFO modulation, counts match the limit, EB Garamond canvas text. | UI-02 is visual, and gates alone can't judge legibility. Same pattern as 3.1 (R113). |

## Constraints Identified

- **Audio must not move:** the default harness (474) carried diff stays empty against baseline B, and N3 / N2 / N1 / N0 `--strict` + R12 stay at rc 0. Telemetry writes are write-only side effects on the audio thread and must never feed DSP state.
- **0 allocations on the audio thread** with telemetry writing active, including block 0. Fixed-size peak ring and grain snapshot are allocated in `prepareToPlay`.
- Editor/message thread is **copy-only**: it never touches the pool or the scheduler. The snapshot is published by the audio thread (seqlock or triple buffer). On a torn read the method returns false and the page keeps its last frame.
- No `juce::Timer` / `emitEvent` push; the page pulls at ~30 Hz (D26).
- Frozen semantics: `frozen = true` → `spanSec` = held span (not `buffer_length`), wave bins cover the held content, and grain `delaySec` is measured from the frozen head. Must agree with the R24 / R27 frozen geometry from 2.4.
- Wave orientation: `wave[0]` = oldest (span seconds behind the head), `wave[463]` = newest. `waveCount` = number of valid bins.
- Grain `delaySec` must be in the same time domain as `spanSec` (ring-domain read position relative to the write head, R23 ring-domain anchor), so dots line up with the waveform.
- JSON payload size at 30 Hz: 464 wave floats + ≤ 128 × 7 grain values. Keep the serialisation allocation off the audio thread (the message thread may allocate).
- The page edit for D30 is confined to `drawBuffer`'s band computation. `i18n-states.json`, check-ui-labels, check-i18n / fr / zh, boot-all-uis `--strict-tips`, `ui_frontend_check` and `ui_binding_probe` stay green. `generic-overrides.json` regenerates if the telemetry stub shape changes.
- Windows: no new WebView surface; existing null-guards cover it.

## Open Questions (for research)

- Peak ring design: per-cell peaks at what granularity (fixed samples per bin vs a time-based bin), and how 464 bins are derived for any `buffer_length` / `spanSec` (decimate on the message thread from a finer ring, or bin on the audio thread?). Cost at 192 k.
- Seqlock vs triple buffer for the 128-grain snapshot. Writer cadence: per block, or per 32-sample pass? Torn-read retry policy (one retry, then return false?).
- Which pass/pool fields give `delaySec`, `env` (current window value), `gain`, and effective `pitchSt` / `pan` per live grain without touching DSP state. Are latched FX/pitch values available read-only?
- Effective position/spray per stream for D30: where the post-mod value exists (ramp domain after base ramp, R47). Is it per-block or per-spawn (latched R48)? Which one should the band show?
- Frozen: exact held-span definition (R27 `heldCaptured` span vs floor) and how wave bins map onto the held region after the R24 floor/margin.
- Harness rows: snapshot correctness against a known grain set, delay/span domain agreement (live and frozen), torn-read path (forced writer collision), 0-alloc row, counts == pool census, dB-map row, carried-diff-empty + N-suites.
- `generic-overrides.json` / binding-probe stub: does the C++-seeded stub need the new fields so gates exercise the D30 band path?

## Carried items (not 3.2 scope)

- 3.3: meters, LFO phase, onset pulse, loop gain, mod matrix view + rings, source meters; deferred DAW pass (D27: DAW-2.5a 1–8, DAW-2.5b 1–5, DAW-2.6 1–3) at 3.3 verify.
- Stage 4: Env 1e30 pin. Harness items: C7 machine-speed calibration, event-aware `renderPool` (Z6a). W2o: widen seeds, never the threshold.

## Next Phase

Ready for: research phase (`/plugin-research O-Granulator 3-gui`)
