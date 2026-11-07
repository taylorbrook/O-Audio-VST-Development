# Stage 3: GUI — Phase 3.3 (Mod matrix view + mod rings) — Context

## Discussion Summary

**Date:** 2026-11-06
**Participants:** Taylor, Claude
**Inputs:** ROADMAP.md Phase 3.3; REQUIREMENTS UI-03; VERIFICATION-3.2.md (Phase 3.2 ✓, carried list); HANDS-ON-3.2 "Expected 3.3 stubs"; `PluginProcessor.h` `Telemetry`; `PluginProcessor.cpp` `readTelemetry()`; `ScaleStateJson.h` `telemetryToVar()`; `Source/ui/public/index.html` consumers (`ingestTelemetry`, `drawMeter`, `drawLfos`, `drawFollowers`, `updateReadouts`, `updateLiveModulation`, `renderModRing`)

## Starting state (observed)

Most of UI-03 has been live since 3.1 (VERIFICATION-3.1: "routing + modNorm already live in 3.1"):

- **Matrix view:** 16 `.mrow` rows (source select → destination select → depth fader), slot count, clear buttons, drag-a-chip-to-route, row flash. Bound to the `modN_source/dest/depth` params.
- **Rings:** `renderModRing` draws the range arc and the latest-route arc on every routed knob (`routesFor(dest)`). Alt-drag edits the latest route's depth. `updateLiveModulation` places the ring dot at `telemetry.modNorm[dest]`, which is live from `engine.getDestTelemetry`.
- **Source meters:** the source-chip tints follow `telemetry.sources[11]`, which is live.

Stubs left for 3.3 (each emitter is already in place in `telemetryToVar`, behind its `has*` flag):

| Field | Struct | JSON key | Page consumer | Today |
|---|---|---|---|---|
| Output / input peaks | `hasMeters`, `outPeakL/R`, `inPeak` | `out[2]`, `inPeak` | `ingestTelemetry` → `drawMeter` (−48…+6 dB, page decay 0.88 / 0.94), env lane `lvl01`, onset lane | `hasMeters` is never set: the meter and level lanes are flat |
| LFO phase | `lfoPhase[4]` | `lfoPhase` | `drawLfos`: dot X = `(phase % 1) / 2` across the 2-cycle figure, Y = the live value | zeros: the dot sits at the left |
| Onset | `hasOnset`, `onsetPulse`, `onsetFired` | `onset`, `onsetFired` | onset-lane tick marks (`onsetTimes`); trigger pills `.fire` when `onset > 0.5` | never set |
| Loop gain | `hasLoopGain`, `loopGainDb` | `loopGainDb` (finite only) | `updateReadouts`: "Open" when absent or < −120 dB, else ±x.x dB; bar = `10^(dB/20) / 0.95` | never set: reads "Open" |

## Requirements Confirmed

- **UI-03** (should): a mod matrix view showing source → destination → depth, with the mod amount visible on targets. **Test (ROADMAP):** UI-03 holds, and route edits are click-free in the audio.
- The phase closes the Stage 3 carried list: meters, LFO phase, onset pulse, loop gain, the s2–s4-enabled i18n state (3.1 deviation 10), and the deferred DAW pass (D27).

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| D34 | Loop-gain readout | **Effective live gain:** `loopGainDb = 20·log10(g_knob_ramped × g_agc)`. `g_knob` = 0.95 · feedback (the engine ramp's current value), and `g_agc` = the FeedbackChain AGC's current gain `min(1, kAgcCeil / env)`. The field is absent (the page shows "Open") when feedback = 0 or freeze is on, because freeze beats feedback. The bar stays scaled to the 0.95 cap, as the page already does. | The readout's note says "saturation and AGC hold the cascade". A live value shows that happening. A knob-only number never moves, and a measured dB/generation is noisy on real material. |
| D35 | UI-03 scope | **v2 as-is (D28 holds).** Source-chip tints = the source meters. Rings and live ring dots = the mod amount on targets. The 16-row matrix = the view. 3.3 fixes defects only, with no new matrix UI. Hands-on rows cover the matrix and the rings end to end. | It already meets UI-03. v2 is locked. |
| D36 | Route-edit clicks | **Fix in 3.3** if the gate finds a click on a source / destination / depth change mid-stream. A DSP touch is allowed (for example a short crossfade or ramp on a slot's source/dest switch). Constraint: renders with no route edits stay **byte-identical**, so the carried diff vs the 3.2 baseline is limited to the new rows. | It is this phase's ROADMAP test. Deferring it would ship Stage 3 with a known UI-03 defect. |
| D37 | DAW pass host | **Logic Pro (AU)**, run once at 3.3 verify, in the same session as HANDS-ON-3.3: DAW-2.5a 1–8, DAW-2.5b 1–5, DAW-2.6 1–3, plus the 3.3 items. A FAIL reopens FUNC-04 / QUAL-03 (2.5a) or FUNC-08 / DSP-08 (2.5b). A FAIL on DAW-2.6 item 1 triggers the D21 fallback (600 s). | Taylor's pick. The VST3 path is already covered by pluginval 10. A VST3 DAW pass can come in Stage 4 (COMPAT-03) if needed. |
| D38 | Meter definitions (Claude default, no objection raised) | `out` = the peak of the final output buffer (what the host receives, post output gain and wet safety). `inPeak` = the peak after input gain (what the ring captures). Both are **max since the last read**: the audio thread does a relaxed fetch-max, and the reader does an exchange(0). The page keeps its own decay. | The meters match what the user hears and what the grains read. Max-since-read can't miss a transient between 30 Hz pulls. |
| D39 | LFO phase domain (Claude default) | `lfoPhase[n]` = the LFO's cycle position **without** the `lfoN_phase` offset, in [0, 1). The figure already starts at `ph0`, so the dot sits on the drawn curve. It is valid in Free and Tempo mode. For the S&H / random shapes the dot Y is the live value and may leave the stylised curve. That is accepted, and noted in hands-on. | It matches the existing `drawLfos` geometry without a page edit. |
| D40 | Onset fields (Claude default) | `onsetFired` = at least one detector event since the last read (audio-thread flag, cleared by the reader with exchange). `onsetPulse` = 1 on an event, then a ~100 ms decay computed at read time from the time of the last event, so the trigger pills light for about 3 frames. | No event is lost between pulls, and there is no extra audio-thread work beyond one store per event. |
| D41 | Verification | Automated gates plus **HANDS-ON-3.3.md**. The hands-on covers the meters, the LFO dot, the onset ticks and pills, loop gain under feedback / AGC / freeze, the matrix and rings end to end, and route edits by ear. The DAW pass runs in the same Logic session. | Same pattern as 3.1 and 3.2. UI-03 and the click test are partly perceptual. |

## Constraints Identified

- **Audio must not move** for renders without route edits. The default harness carried diff stays empty against the 3.2 baseline except for the new rows. N3 / N2 / N1 / N0 `--strict` and R12 stay at rc 0. Telemetry writes are write-only side effects and never feed DSP state. A D36 fix is the one sanctioned audio change, and it applies only on route-edit transitions.
- **0 allocations on the audio thread** with all telemetry active (M8 row extended).
- The audio thread writes and the message thread reads or exchanges. Meter, onset and loop-gain values use relaxed atomics only. There are no locks on the audio thread and no `juce::Timer` / `emitEvent` push: the page pulls at ~30 Hz (D26).
- **Block-size invariance:** meter and onset fields are telemetry only. The route-edit fix (if any) must sit on the absolute 32-sample grid and pass the block-size null.
- **Page:** v2 locked (D28). The consumers already exist, so a page edit happens only for a defect found in 3.3, scoped, with all five UI gates and the binding probe green. Add the **s2–s4-enabled state** to `tests/i18n-states.json` (3.1 deviation 10), since counts are live now. `generic-overrides.json` and the probe stub regenerate if the telemetry shape changes.
- **Loop gain** must agree with the engine: "Open" exactly when the loop contributes nothing (feedback 0 or frozen). The value must not exceed 20·log10(0.95) ≈ −0.45 dB.
- The Standalone target must be rebuilt after any install before hands-on (3.2 process note).
- Windows: no new WebView surface.

## Open Questions (for research)

- **Click gate design for route edits:** which edits to cover (source change, destination change, depth step, slot clear, drag-to-route). Use a step verdict against a same-motion reference (pattern_step_click_gate_needs_same_motion_control) with a tonal source, not noise. Is the matrix output already ramped per destination, and is a dest switch an instant jump in the target value? What is the smallest fix: a ramp on the slot's contribution, or a crossfade of old to new dest?
- Where `g_knob`'s ramped value and the AGC `env` live, and whether they are readable as relaxed atomics without adding state that feeds DSP. Is one shared AGC per chain stereo-linked (a single value)?
- The LFO accumulator in `ModSources.h`: is the phase stored with or without the offset? How do Tempo-sync and re-phase on onset-trigger bursts show in the dot?
- Onset: where the detector event is visible (`Analysis.h` / Engine event offset), and how to record its time for the read-side decay without a clock dependency (sample count vs the snapshot's `fs`).
- Harness rows: meter peak vs a known sine level (out and in, with input gain), max-since-read across several blocks, onset fired/cleared, LFO phase vs the analytic phase at a known rate, loop gain = 20·log10(0.95·fb) with the AGC idle and lower when it engages, "Open" at fb 0 and frozen, the click gate with negative controls, M8 0 allocs, carried diff, N-suites.
- Is the `pulse > 0.5` pill threshold compatible with the D40 decay at 30 Hz pulls, or is a hold needed?

## Carried items (not 3.3 scope)

- Stage 4: Env 1e30 pin (predicate at the env input). Harness items: C7 machine-speed calibration, event-aware `renderPool` (Z6a). W2o: widen seeds, never the threshold. A VST3-host DAW pass if COMPAT-03 calls for it.
- NOTE from 3.2: `uiSnap` is not reset in `prepare` (possible one stale frame, visual only). No action.

## Next Phase

Ready for: research phase (`/plugin-research O-Granulator 3-gui`)
