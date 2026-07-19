# Changelog — O-Contrabass

All notable changes to the O-Contrabass physical-model bowed-contrabass synth.
Format loosely follows [Keep a Changelog]. **v1.0.0 is the first shipped product
version** — the pre-release `1.x-dev` engine track collapses into it.

## [1.0.0] — 2026-07-15 — first release (engine + WebView editor + polish)

First shipped product version. Stage 4 (Polish) adds factory presets, the Dorico
distribution bundle, the Windows/pluginval CI gate, and the PERF-02 benchmark on top
of the Stage-2 DSP engine and Stage-3 WebView editor. **DSP is frozen** — Stage 4
wrote parameter state + docs + distribution artifacts + tooling only. Validated:
**19/19 render goldens byte-identical**, `auval -v aumu OCbs OuDv` **SUCCEEDED**,
pluginval `--strictness-level 10` **SUCCESS** (macOS), **PERF-02 0.59% / 0.65%
CPU/voice** (44.1 / 48 kHz, 256-block, defaults; budget 5%).

### Added — Stage 4: Polish

- **Factory presets (FUNC-04):** 10 presets seeded to
  `~/Library/O-Contrabass/Presets/Factory/` on first run, authored in engineering
  units and converted skew-safe through each param's `NormalisableRange`
  (`convertTo0to1` — BOW_SPEED / BOW_PRESSURE / BRIGHTNESS / VIBRATO_ONSET are
  skewed). Orchestral bank — **Cinematic Bass Sustain** (default landing preset),
  Section Bass, Solo Arco Bass, Pianissimo Bass, Forte Bass. Drone bank — Infinite
  Drone, **Just-Intoned Drone** (7-limit `DETUNE_A=+204 / D=−14 / G=+182`),
  Scordatura Bass (C–G–D–A fifths), Sub Drone, Dark Pad Bass. Drone presets carry
  explicit `TUNING_SYSTEM`(12-TET) / `NOTE_EXPRESSION` so the preset-load tuning
  reset can't clobber intent; per-string pitch rides the independent `DETUNE_*`
  params. `STRING_TENSION` intentionally left at its inert 0.5 default (v1.1).
- **Dorico Playback Template bundle (COMPAT-02):** `Resources/dorico/` — single-
  family `.doricolib` + `endpointconfig.xml` + `playbacktemplatespec.xml` for
  microtonal VST3 Note Expression playback (one sustained-arco `pt.natural`
  technique; **load-bearing** top-level `<pitchBendRange>2` +
  `<microtonalPlaybackMethod>kVST3NoteExpression`; `kNoteVelocity` dynamics) plus
  `INSTALL-DORICO.md` / `SMOKE-TEST.md` (P0 = TC-4 24-EDO quarter-sharp) and a CMake
  `install(DIRECTORY …)` rule. Ships dev-branded (`OuDv` / `-dev`); release-GUID
  swap documented.
- **Windows cross-platform gate (COMPAT-01):** `workflow_dispatch` validate-only
  path in `.github/workflows/build-and-release.yml` builds a single plugin and runs
  `pluginval --strictness-level 10` on `windows-latest` **without publishing a
  Release** (log uploaded as an artifact; skips macOS + release jobs on that path).
- **PERF-02 benchmark:** isolated `--perf` render-harness mode
  (`--sample-rate` / `--block-size`, RTF + CPU%/voice); measured **0.587% @44.1 kHz,
  0.652% @48 kHz** (256-block, defaults). The golden gate is WAV-sha256-only, so the
  timing additions cannot perturb the 19/19 invariant.

### Stage 3: WebView GUI

Full WebView editor from finalized mockup v1 (1000×650 fixed, parchment/naturalist
house style). Validated: **19/19 render goldens byte-identical** (GUI touched zero
signal-path arithmetic), `auval -v aumu OCbs OuDv` **SUCCEEDED**, pluginval
`--strictness-level 10` **SUCCESS**, native-fn bridge gate clean (32 JS = 32 C++,
`tests/ui_frontend_check.js` 14/14 PASS).

### Added

- **WebView editor (Phase 3.1):** 31 parameter bindings (29 WebSliderRelay +
  WebComboBoxRelay + WebToggleButtonRelay) with locked member order
  relays → webView → attachments; bare-path resource provider; skew-correct
  dblclick-reset via `getParameterDefaults`; readouts from `getScaledValue()`.
- **Preset bar (D6):** preset-manager v1.0.4 via canonical CMake include +
  canonical `js/preset-manager.js` (10 native fns). Tuning-engine state (intervals,
  scale name, tonic, octave stretch) round-trips through user presets AND DAW session
  state (`getStateInformation` now routes through `OuariconPresetManager::getStateAsXml`;
  backward compatible with pre-Stage-3 plain-APVTS session XML).
- **Full Tuning tab (D3 — user scope expansion):** shared `tuning-panel.js` v3.0.0
  behind a Main/Tuning tab bar in the 42 px header — intervals table, embedded tuning
  library (period-intact loads), scale generators (EDO / harmonic series / rank-2),
  .scl/.kbm round-trip, octave stretch, HTML export; 20 native fns. Panel receives the
  `Juce` ES-module namespace (never `window.__JUCE__`). REFERENCE_PITCH ↔ panel
  masterTune coherence: the APVTS param is the single source of truth (`setMasterTune`
  routes through the param; the engine's own masterTune stays 440 by design — the
  voice applies the refPitch/440 ratio itself).
- **Real visualization feeds (Phase 3.3):**
  - `vuLevel` — post-limiter/post-gain RMS dB from a read-only relaxed atomic stored
    at the END of `processBlock`.
  - `bowState` (D5) — DSP-true effective bow speed/pressure/β (post-LFO/macro/MPE)
    via per-voice relaxed viz atomics; the processor publishes the most-recently-
    started ACTIVE voice (fixes the voice-0 hardcode for viz). JS dot eases to the
    feed when active, falls back to knob values at silence.
  - Body spectrum (R1/D7) — mockup mode table replaced with the BodyResonator truth:
    freqs {60,98,115,175,235,340,700,1200} Hz, Q {14,11,9,8,7,6,5,2.5}, gains
    {−2,0,−1,−3,−4,−5,−7,−6} dB + the exact `recomputeCoefficients()` formulas; pure
    JS recompute, no data feed (reserved `bodyModes` event intentionally unused).
- **Render-harness protection:** `createEditor()` guarded `#if JUCE_WEB_BROWSER`;
  `PluginEditor.cpp` removed from harness sources
  (`pattern_render_harness_breaks_on_webview_editor`). Goldens re-baselined at execute
  START and re-verified at stage exit — byte-identical both times.
- **`tests/ui_frontend_check.js`:** ported from O-MicrotonalSampler v1.23.7 —
  inline-module syntax, bridge closure (32-fn surface), getScaledValue/paramDefaults
  pins, window.confirm ban, resource-provider closure.

### Changed

- **TUNING_SYSTEM choice label "Scala/TUN" → "Scala" (D1):** no TUN parser exists in
  TuningEngine 2.1.0; the picker filter and all UI labels are `.scl`-only for v1.0.
  Choice index mapping frozen (0=Scala, 1=MTS-ESP, 2=12-TET); label is cosmetic.
  AnaMark TUN parser → v1.1 backlog (shared-module upgrade).

### Known-inert

- **STRING_TENSION ships bound but inert (D2, user-confirmed):** state round-trips but
  no DSP consumer. Wiring deferred to v1.1 (activating it changes the default timbre —
  default 0.5 is not a no-op; needs its own goldens re-baseline). See NOTES.md.

### Deferred

- Dorico distribution artifacts (Playback Template / EndpointConfig / .doricolib) →
  Stage 4 packaging (D4). `registry.yaml` module-version refresh (R5) — trust
  per-module `module.yaml` (preset-manager 1.0.4, scala-tuning-engine 2.1.0/js 3.0.0).

## [1.0.0-dev] — 2026-07-08 — Stage-2 DSP code-review resolution

Resolution of `CODE_REVIEW.md` (deep three-subsystem review, 2026-07-08). All
**Critical** and **Warning** findings resolved (16 of 16); Info findings (IN-01..16)
deferred as opt-in cleanups. Validated: 17/17 render-harness goldens reproduce
byte-identical (10 baselines intentionally refreshed for the corrected paths — see
below), `auval -v aumu OCbs OuDv` SUCCEEDED, `pluginval --strictness-level 10`
SUCCESS (Parameter thread safety + Fuzz parameters).

### Fixed — Critical (must-fix tier; applied earlier this session, now recorded)

- **CR-01 — Per-block heap allocation in body resonator (RT-safety).**
  `BodyResonator.cpp` recomputed coefficients every block via the `Ptr`-returning
  `Coefficients::makeBandPass` (heap `new`, 8× per block per voice). Switched to the
  allocation-free `ArrayCoefficients::makeBandPass` `std::array` overload; warm-up
  assignment lands in `prepare()`.
- **CR-02 — Legacy-mode channel range dropped MIDI channel 16 (off-by-one).**
  `juce::Range<int>` is end-exclusive; `Range(1,16)` covered channels 1–15 only.
  Changed to `Range(1,17)` (channels 1–16). `PluginProcessor.cpp`.
- **CR-03 / WR-01 — `MessageManager::callAsync` on the audio thread (RT-safety +
  teardown UAF).** APVTS `parameterChanged` fires synchronously on the setter's
  thread (the audio thread under host automation); `callAsync` heap-allocates a
  message and the captured `this` had no lifetime guard. Replaced with an
  `AsyncUpdater` + `std::atomic<int> pendingTuningChoice`: `parameterChanged` stores
  the choice and `triggerAsyncUpdate()` (RT-safe, preallocated message);
  `handleAsyncUpdate()` applies `setMode` on the message thread; the destructor
  `cancelPendingUpdate()`s. Constructor seeds the initial mode synchronously.

### Fixed — Warning (this pass: WR-02..WR-13)

- **WR-02 — Uncached per-block APVTS lookups.** `processBlock` walked the APVTS
  `std::map` 4× per block for MASTER_SAT_AMOUNT / LIMITER_CEILING_DB / WIDTH /
  OUTPUT_GAIN. Cached the four `std::atomic<float>*` as members, resolved once in the
  constructor. (Numerically identical → golden-neutral.)
- **WR-03 — Master saturator fold-back for |in| > 1.0.** `f(x)=x−x³/3` has
  `f'(x)=1−x²` ≤ 0 for `|x|>1`; the ±1.5 clamp admitted the fold-back region where a
  louder input produced a *quieter*, distorted output. Clamped to ±1.0 (output
  plateaus at ±2/3, monotonic). `MasterSaturator.h`. Golden-neutral for single-voice
  renders (per-voice output is already clamped to ±1.0 upstream).
- **WR-04 — Width > 1 defeated the limiter ceiling.** The chain ran Sat → Limiter →
  Width, so the unbounded M/S side gain (width up to 2.0) pushed peaks back above the
  ceiling the limiter had just enforced (+2·ceiling ≈ +5.7 dBFS at width=2). Reordered
  to **Sat → Width → Limiter → OUTPUT_GAIN** so the limiter is the last dynamics stage
  and genuinely bounds the widened output. `PluginProcessor.cpp`.
- **WR-05 — `mu_s` (static friction) leaked after SUB_HARMONICS returned to 0.** The
  sub-harmonic bias branch widened `mu_s` but only restored it while active; once
  SUB_HARMONICS returned to 0 the branch was skipped and `mu_s` stayed elevated
  (harsher tone) for every subsequent note until `prepareToPlay`. Now reset to the
  bass default (0.85) unconditionally each block, mirroring the unconditional
  `setRosin`. `BowedContrabassVoice.cpp`. Idempotent at the default → golden-neutral.
- **WR-06 — Dispersion coefficient sign-flipped on deeply-bent low notes.**
  `k = k1 + k2·I + k3·I²` has a real zero near I ≈ 2.33 (f0 ≈ 30 Hz), inside the
  playable range; bending the E string toward ~30 Hz drove `−C/k` through a divergence
  → coefficient snapped ±0.99 across blocks → click. Floored `I` at 8 (E1, the
  formula's validity envelope); sub-E1 pitches reuse E1's coefficient.
  `DispersionFilter.h`. No-op at/above E1 → golden-neutral.
- **WR-07 — Pitch drifted sharp at low BRIGHTNESS.** `filterGroupDelay = sr/(2π·f)`
  grew unbounded as brightness dropped, but the bridge-LP pole is clamped to ≤ 0.95,
  so the real group delay saturates at ~19 samples below ~720 Hz — the loop was
  over-compensated (too short) → pitch sharp (~1.3 semitones at BRIGHTNESS=80 Hz on
  E1). Floored the brightness used for group-delay compensation at the pole-clamp
  frequency. `WaveguideString.cpp` (new `bridgeGroupDelaySamples()` shared by both
  compute sites). No-op for brightness ≥ ~720 Hz (incl. 4500 Hz default) → golden-neutral.
- **WR-08 — Slow-bow LFO ran at half the set rate.** The per-block phase delta divided
  the host-rate `numSamples` by the 2× internal rate, halving the increment
  (SLOW_LFO_RATE=0.3 Hz → 0.15 Hz). Now divides by the host rate.
  `BowedContrabassVoice.cpp`.
- **WR-09 — Per-block smoothers ramped 2× too slowly.** `macroSmoothed` and
  `subHarmonicsSmoothed` (voice) and `stiffnessSmoothed` (waveguide) were `reset()` at
  the 2× internal rate but advanced by host-rate step counts, so their 20/30 ms ramps
  took 40/60 ms. Reset all three at the host rate. `BowedContrabassVoice.cpp`,
  `WaveguideString.cpp`. Default-state (macro=0 / sub=0 / stiffness steady) →
  golden-neutral.
- **WR-10 — Effectively monophonic → 4-voice polyphony.** Only one voice was added, so
  a second note-on stole the first — breaking the double-stop drone use case the
  ACTIVE_STRINGS / DETUNE_* / INFINITE_SUSTAIN parameter set is built for. Added 4
  voices (one per EADG string). `PluginProcessor.cpp`. (Improved the `note-sequence`
  acceptance test: transition continuity FAIL→PASS.)
- **WR-11 — MPE legacy-mode-only: documented (no code change).** Decision: keep legacy
  mode for this Stage-2 build; a full MPE zone layout is deferred to Stage 3 when the
  editor can expose MPE configuration. See NOTES.md → Known Limitations.
- **WR-12 — `StereoWidth::reset()` wrong `SmoothedValue` overload.** `reset(1.0f)` bound
  `reset(int numSteps)` (1.0f → int 1), destroying the 20 ms ramp instead of seeding
  width=1.0. Changed to `setCurrentAndTargetValue(1.0f)`. `StereoWidth.h`.
- **WR-13 — `MasterSaturator::reset()` clobbered the 30 ms ramp.** `reset(0)` set
  `stepsToTarget=0`. Changed to `setCurrentAndTargetValue(0.0f)`. `MasterSaturator.h`.

### Test baselines

- 7 render goldens byte-identical, unchanged (string-A/D/G, detune-sweep-A, vibrato,
  saturator-tail-comparison, microtonal-mpe) — confirms no collateral drift on paths
  the fixes don't touch at default settings.
- 10 goldens intentionally refreshed to capture the corrected DSP (stiffness-zero-pre,
  note-sequence, macro-sweep, slow-lfo, schelleng-stress, sub-harmonics,
  sub-harmonics-stability, output-chain, microtonal-12tet, microtonal-scala). Each
  drift maps to a specific fix (WR-04/05/06/08/09/10); acceptance-criteria pass/fail
  flags are unchanged vs the prior baseline except note-sequence (FAIL→PASS). No
  stability gate (NaN/peak/blockTime/clickFree) regressed on any mode.

### Deferred (opt-in)

- IN-01..IN-16 (info/nitpick): dead members, stale doc comments, duplicated friction
  constants, mono-bus width collapse, limiter transient overshoot, saturator
  oversampling, etc. See CODE_REVIEW.md. IN-01 (`outputGainSmoothed.reset(1.0f)` wrong
  overload) is the same class as WR-12/13 and a trivial follow-up.
