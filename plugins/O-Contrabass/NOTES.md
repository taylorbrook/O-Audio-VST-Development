# O-Contrabass — Developer Notes

Physical-model bowed-contrabass synth: split-rail digital waveguide with hyperbolic
bow-friction excitation (2× oversampled), Schelleng-calibrated bow-force limiting,
cascaded-allpass dispersion, an 8-mode body resonator, 3-band bow-noise generator, and
a mono→stereo master chain, plus the shared Scala tuning engine + VST3 Note Expression.

**Status:** pre-release. Stage-2 DSP engine complete through Phase 2.6b; Stage-3 WebView
editor pending (placeholder editor today). PLUGINS.md still carried a stale `🚧 Stage 0`
marker — the source is Stage-2.

## Timeline

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

- **WR-11 — MPE legacy mode only (no zone layout).** `setZoneLayout` is never called;
  the instrument is permanently in MPE legacy mode. Per-channel pitchbend / pressure /
  CC74 route to notes on that channel (one-note-per-channel MPE works), but the master
  zone and MPE Configuration Messages (MCM / RPN zone reconfiguration) are ignored — a
  ROLI-style whole-instrument glissando on the master channel won't respond.
  **Decision (2026-07-08):** acceptable for this Stage-2 build; a proper lower-zone
  layout (with legacy as an explicit fallback) is deferred to Stage 3, where the editor
  can expose MPE configuration. Legacy pitch-bend range is ±24 semitones, channels 1–16.

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

## Regression harness

- Offline render harness is the Stage-2 correctness gate:
  `tests/render-harness/reproduce-goldens.sh` (17 goldens, sha256 truth-bar).
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
