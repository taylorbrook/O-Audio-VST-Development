# O-Contrabass — Developer Notes

Physical-model bowed-contrabass synth: split-rail digital waveguide with hyperbolic
bow-friction excitation (2× oversampled), Schelleng-calibrated bow-force limiting,
cascaded-allpass dispersion, an 8-mode body resonator, 3-band bow-noise generator, and
a mono→stereo master chain, plus the shared Scala tuning engine + VST3 Note Expression.

**Status:** pre-release. Stage-2 DSP engine complete through Phase 2.6c; Stage-3 WebView
editor complete (mockup v1 integrated, 31 bindings, preset bar, full Tuning tab, three
real-data visualizations). PLUGINS.md still carried a stale `🚧 Stage 0` marker — the
source is Stage-3.

## Timeline

- **2026-07-10/11 — Stage 3 GUI (WebView editor).**
  Integrated finalized mockup v1 (1000×650 fixed): 31 relay/attachment bindings,
  preset-manager v1.0.4 (canonical CMake include + canonical JS; tuning state
  round-trips via custom save/load callbacks and now also through DAW session state),
  full Tuning tab (shared `tuning-panel.js` v3.0.0, 20 native fns, Main/Tuning tab bar),
  `.scl`-only picker (D1), and three real-data visualizations: post-limiter RMS VU
  (`vuLevel`), DSP-true Schelleng operating point (`bowState`, per-voice relaxed viz
  atomics + most-recently-started active-voice selection, D5), and the body spectrum
  recomputed in JS from the BodyResonator truth tables (D7). Render harness protected
  (`createEditor` guarded `#if JUCE_WEB_BROWSER`, PluginEditor.cpp dropped from harness
  sources). Validated: 19/19 goldens byte-identical, auval SUCCEEDED, pluginval-10
  SUCCESS, bridge gate 32 JS = 32 C++ (`tests/ui_frontend_check.js`).

- **2026-07-08 — Stage-2 DSP code-review resolution (v1.0.0-dev).**
  Resolved all Critical + Warning findings from `CODE_REVIEW.md` (CR-01..03, WR-01..13;
  16/16). Must-fix tier (CR-01..03, WR-01: RT-safety + ch16 drop + teardown UAF) applied
  earlier in the session; this pass added WR-02..WR-13 (uncached lookups, saturator
  fold-back, width-defeats-limiter chain reorder, mu_s friction leak, dispersion
  zero-crossing click, low-brightness pitch drift, slow-LFO half-rate, 2×-slow
  smoothers, 4-voice polyphony, two wrong-`reset()`-overload latent bugs). Validated:
  17/17 goldens reproduce byte-identical, auval SUCCEEDED, pluginval-10 SUCCESS.
  Design decisions taken with the user: WR-10 → 4 voices (EADG bank); WR-11 → keep
  legacy MPE, defer zone layout to Stage 3.

## Known Limitations

- **STRING_TENSION is known-inert in v1.0 (D2, user-confirmed 2026-07-10).** The knob
  is visible, bound, and its state round-trips (host automation / presets / session),
  but no DSP consumer reads it — only STRING_STIFFNESS is consumed
  (`BowedContrabassVoice.cpp` `updateParametersFromAPVTS`). DSP wiring is **v1.1
  backlog**: activating a previously-dead param changes the default timbre (its default
  0.5 is NOT a no-op value — see `pattern_activating_dead_param_default_timbre`), so it
  needs its own goldens re-baseline when wired.

- **`.tun` loading deferred (D1).** TuningEngine 2.1.0 has no AnaMark TUN parser
  (`loadScalaFile` only). v1.0 restricts the tuning picker filter + all labels to
  `.scl` (TUNING_SYSTEM choice label renamed "Scala/TUN" → "Scala"; index mapping
  frozen). An AnaMark TUN parser is **v1.1 backlog as a shared-module upgrade** —
  O-Bowed/O-Wind/O-Bassoon ship the same dead `*.tun` filter and would all benefit.

- **Dorico distribution artifacts → Stage 4 (D4).** The DSP/UI side is complete
  (VST3 Note Expression live since Phase 2.6c; NOTE_EXPRESSION toggle + Tuning tab
  drive the same TuningEngine the NE path reads). But per
  `critical_dorico_distribution_mechanism`, a bare `.doricoexpmap` is NOT auto-ingested
  by Dorico — shipping requires a Playback Template / EndpointConfig / `.doricolib`.
  That packaging work is a **Stage 4 task**.

- **`registry.yaml` staleness (R5).** The module registry lists preset-manager 1.0.2
  and scala-tuning-engine 2.0.0; the authoritative per-module `module.yaml` files are
  1.0.4 and 2.1.0 (js panel 3.0.0). Trust `module.yaml`; a registry refresh is an
  outstanding side task (repo-level, not plugin-level).

- **WR-11 — MPE legacy mode only (no zone layout).** `setZoneLayout` is never called;
  the instrument is permanently in MPE legacy mode. Per-channel pitchbend / pressure /
  CC74 route to notes on that channel (one-note-per-channel MPE works), but the master
  zone and MPE Configuration Messages (MCM / RPN zone reconfiguration) are ignored — a
  ROLI-style whole-instrument glissando on the master channel won't respond.
  **Decision (2026-07-08):** acceptable for this Stage-2 build; a proper lower-zone
  layout (with legacy as an explicit fallback) is deferred to Stage 3, where the editor
  can expose MPE configuration. Legacy pitch-bend range is ±24 semitones, channels 1–16.

- **Legacy-mode Y/Z stickiness across notes on a channel (risk #44).** In MPE
  legacy mode, JUCE's `MPEInstrument` keeps `lastValueReceivedOnChannel` for the
  timbre (CC74 / Y) and pressure (channel pressure / Z) dimensions — the last value
  received on a channel **persists into the next note-on on that channel**; there is
  no reset at note-off. A note struck after a high-Z passage starts at that Z until
  the host sends a new value. This is host-typical MPE behaviour, not a defect; the
  render harness's `--mpe-yz` mode sends explicit Y=64 / Z=0 resets before each
  segment's note-on for exactly this reason. Documented Phase 2.6c (R41d).

- **Info-tier cleanups deferred (IN-01..IN-16).** Non-blocking quality items from
  CODE_REVIEW.md, not addressed in this pass:
  - IN-01: `outputGainSmoothed.reset(1.0f)` wrong overload (same class as WR-12/13;
    harmless — `prepareToPlay` reseeds — but a trivial follow-up).
  - IN-04/IN-07: mono-bus width/decorrelation collapse + allpass combing when summed to
    mono (inherent to the decorrelation approach).
  - IN-05: zero-latency limiter overshoots the ceiling on transients (topology tradeoff;
    no look-ahead).
  - IN-06: saturator cubic has no oversampling → aliasing on bright content.
  - IN-09: long-term intra-loop DC in infinite-sustain drone (downstream 35 Hz HP
    protects the DAC; documented deferral).
  - IN-10/IN-11/IN-12: duplicated friction constants, dead members
    (`slowLfoSpeed/PressureSmoothed`, `currentMaxBlockSize`), stale doc comments.
  - Full list in `CODE_REVIEW.md`.

## v1.1 Deferrals (frozen for v1.0.0)

Intentionally deferred past the v1.0.0 release. Each is wired/stable at its
default state and bit-exact under the frozen render goldens — only the item's
*audible depth* or *optional path* is deferred, so none re-baselines the goldens
until actually wired. **Do not "fix" these during a v1.0.x patch without re-baselining.**

- **`STRING_TENSION` bound-but-inert** — DSP wiring is v1.1. Ships at default 0.5
  (not a no-op if wired). Factory presets deliberately omit it so it stays 0.5.
- **AnaMark `.tun` parser absent** — Scala picker is `.scl`-only; TUN import is a
  v1.1 shared-module upgrade. `TUNING_SYSTEM` choice map frozen (0=Scala, 1=MTS-ESP,
  2=12-TET).
- **DSP-07 sub-harmonic audible depth** — engagement collapses post-tanh-port/post-body;
  v1.1 retune (kForceBoost / bias-amplitude / injection-point). Wire-up stable,
  default-state bit-exact.
- **DSP-08 slow-LFO breathing** — 15.7% vs 20% target; v1.1 metric/gain tune.
- **DSP-09 vibrato depth** — peakDepthCents ~7.95¢ vs the 10–14¢ band; v1.1 transfer tune.
- **FUNC-07 MTS-ESP** — present-but-stub (returns 12-TET); v1.1 SDK linkage. Scala/TUN
  *import* itself is complete.
- **Dorico CC11 sustained-dynamics listener** — the Dorico bundle ships `kNoteVelocity`
  (velocity fixed at note-on). Continuous within-note crescendos/hairpins need a
  plugin-side CC11→`EXPRESSION_MACRO`/bow-pressure path, which touches param handling
  and risks the frozen goldens → v1.1.

## Regression harness

- Offline render harness is the Stage-2 correctness gate:
  `tests/render-harness/reproduce-goldens.sh` (19 goldens, sha256 truth-bar).
  Build with `-DOUARICON_BUILD_TESTS=ON`; the target is `O-Contrabass-render-test`.
- Acceptance criteria beyond byte-identity live in the per-mode JSON (`pass_nan`,
  `pass_peak`, `pass_blockTime`, `pass_rms*`, vibrato rate/depth ranges, stability
  matrices). Note: several modes (stiffness-zero-pre, macro-sweep, slow-lfo,
  sub-harmonics, output-chain) carry **pre-existing** acceptance-criteria FAILs that
  predate the code-review resolution — these are level/shape-tolerance gaps, not
  stability failures, and were unchanged by this pass.

## Build / validate

```bash
./scripts/build-and-install.sh O-Contrabass
auval -v aumu OCbs OuDv
# harness:
cd build && ninja O-Contrabass-render-test
plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
```
