# O-Prism Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.20.0
- **Type:** Synth (Microtonal Wavetable)

## Lifecycle Timeline

- **2026-08-20 (v1.20.0):** Double-click value entry across all 84 controls (67 knobs, 16 mod-matrix amount sliders, A4 Ref). Reset-to-default moved to Alt/Option-click. Each of the 21 format helpers gained a matching `.parse`, so all 70 `bindKnob()` call sites are untouched; typed text converts to the parameter's *scaled* value and normalises through `state.properties` rather than the JS-mirrored ranges, so the entry path cannot drift from C++. Also fixed the A4 Ref readout, which used a 415–465 Hz inline mapping against a 420–460 Hz parameter — right at centre, 5 Hz out at both ends (display only; tuning was always correct). Also fixed 11 skewed knobs whose reset-to-default targeted the wrong value (`bindKnob`'s 3rd arg is normalised; those call sites passed the scaled default — attack reset to 1 ms instead of 10 ms, delay time to 17.6 ms instead of 375 ms, eqMidFreq to 211 Hz instead of 1 kHz). Linear params were unaffected, which is why it survived ~20 versions. Verified by a 354-check round-trip harness with 4 negative controls, 30 browser-driven DOM interaction checks, and a script comparing all 68 reset targets against `createParameterLayout()`.
- **2026-07-02 (v1.19.1):** Code-review batch 3 (final) — all 17 Info findings dispositioned. Fixed: last uncached per-block APVTS lookups (IN-01), smoothed delay time (IN-03), latency follows distortion bypass (IN-04), sub-osc/noise legato retrigger click (IN-06), Bohlen-Pierce 13th degree (IN-07), isNoteMapped lock (IN-09), generateRank2 clamp order (IN-10), KBM formal-octave in frequency math + exporter unison overlap (IN-11), smoothFrames semantics (IN-12), applyGeneratedScale name (IN-14), toJsonFloatArray non-finite clamp + name escaping (IN-16), tonic bridge failures logged+reverted (IN-17). Removed dead code: mono oscillator path (IN-02), duplicated kDivBeats → NoteDivisions.h (IN-05), 11 uncalled native fns (IN-13), unused webview-relay-manager CMake link (IN-15). IN-08 (Zarlino≡JI) was a reviewer error — arrays already differ at degree 10. auval + pluginval L5 PASS.
- **2026-07-02 (v1.19.0):** Code-review batch 2 — all remaining Criticals + Warnings (25 findings). RT-safety: EQ ArrayCoefficients (CR-04), TuningEngine mutation deferred to message thread via AsyncUpdater (CR-05), cached LFO/tuning param pointers (CR-06), SVF Nyquist clamp + NaN flush (WR-01). Correctness: KBM ref-freq unclamped from master-tune (CR-07), .scl/.kbm/importer parser hardening (WR-11/12/13), exporter period (WR-15), full tuning+KBM session persistence (WR-17). Safety: preset/wavetable filename sanitization (WR-09/10), applyPresetJson reset-to-defaults + factory JSON version-stamped regeneration (WR-08). Dead features implemented: tempo-synced delay + new `delayDivision` param (WR-03, unblocks ~20 factory presets), all 9 silent mod destinations (WR-02), Phase knobs (WR-04), distinct Legato/Always glide (WR-06). UI: macOS preset Save modal (CR-10), redo shortcut (WR-18), tuning-tab refresh on preset load (WR-19), mod names from native fns (WR-20). Info findings deferred.
- **2026-07-02 (v1.18.2):** Crash/UAF batch from the 2026-07-02 deep code review (criticals batch 1) — CR-01 unbounded phase accumulator (OOB heap read at high pitch; floor-wrap + Nyquist clamp), CR-02/CR-03 wavetable use-after-free on editor close / delete / save-over (retired-table reaper with block-generation handshake), CR-09 SafePointer sweep across all 6 FileChooser launchAsync completions, plus the CR-08 delete-before-save WAV append fix the CR-03 fix depends on. auval PASS.
- **2026-05-06 (v1.17.4):** Distribution fix — Logic Pro pinned to v1.17.0 because a stale non-suffixed `O-Prism.component` (legacy dev build, pre-`OUARICON_DEV_SUFFIX`) was shadowing `O-Prism-dev.component` (current). Hardened `scripts/build-and-install.sh` Phase 4 to sweep the dev↔release alternate-variant bundle on every install. No plugin code change.
- **2026-04-11 (v1.13.5):** Code quality — extracted `PrismParamIds::kCustomTuningPresetIndex` constant to replace hardcoded `10.0f` literal across 5 `setValueNotifyingHost` call sites in `PluginEditor.cpp`. Zero behavior change.
- **2026-04-11 (v1.13.4):** DSP perf — hoisted per-sample key tracking `std::pow` out of render loop (block-constant multipliers). Zero audible change.
- **2026-02-16:** Ideated — Creative brief and requirements created

## Description

Microtonal wavetable synthesizer rivaling Xfer Serum. Features 2 wavetable oscillators with morphing, sub oscillator, noise generator, dual multi-mode filters, 5 effects (reverb, delay, chorus, distortion, EQ), and the complete microtonal engine from O-Lyrica/O-Bells (24+ factory tunings, Scala/KBM import, EDO generators). 16-voice polyphony, 100+ factory wavetables, audio file import. WebView UI at 1200x800 with Ouaricon Naturalist aesthetic.

## Roadmap
- **v1.0 Core:** Oscillators + filters + effects + microtonal engine + factory wavetables
- **v2.0 Full:** Drag-and-drop modulation matrix + wavetable editor + additional effects/filters + preset browser

## Known Issues

None (not yet implemented)

## Additional Notes

- Complexity: Tier 4 (Very High) — most complex plugin in catalog
- 60+ APVTS parameters
- Inspiration: Xfer Serum + O-Lyrica/O-Bells tuning engine
- Unique selling proposition: First wavetable synth with professional microtonal support
