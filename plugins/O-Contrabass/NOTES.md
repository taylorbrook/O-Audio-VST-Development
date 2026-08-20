# O-Contrabass — Developer Notes

Physical-model bowed-contrabass synth: split-rail digital waveguide with hyperbolic
bow-friction excitation (2× oversampled), Schelleng-calibrated bow-force limiting,
cascaded-allpass dispersion, an 8-mode body resonator, 3-band bow-noise generator, and
a mono→stereo master chain, plus the shared Scala tuning engine + VST3 Note Expression.

**Status:** 📦 Installed — **v1.7.0** (2026-08-20). Stage-2 DSP engine complete through
Phase 2.6c; Stage-3 WebView editor complete (mockup v1 integrated, 31 bindings, preset
bar, full Tuning tab, three real-data visualizations); Stage-4 polish shipped as v1.0.0.
v1.1.0 closed the DSP-07/08/09 deferrals as **measurement** corrections with no audio-path
change (19/19 goldens byte-identical).

## Timeline

- **2026-08-20 — v1.7.0 hover-help tooltips ("?" toggle) + one-line title.**
  Ported O-Bitrot v1.12.0's verified measure-then-pin hover-help system: every
  Main-tab/header control carries data-tip copy, 350 ms dwell, viewport-clamped
  with anchor-tracking arrow, hidden on pointer-down. "?" toggle in the header
  (tab-strip active vocabulary); preference persists as a root XML *attribute*
  in get/setStateInformation (not a ValueTree property —
  critical_valuetree_xml_roundtrip_loses_type) and is PULLED by the page at
  init (O-FreqPulse WR-01 race avoided). Bridge surface 32 → 34, parity gate
  updated. Title fix: `.plugin-name` got `white-space: nowrap` +
  `flex-shrink: 0` — it was wrapping at the hyphen under flex min-content.
  DSP untouched: 21/21 goldens byte-identical, auval PASS, ui_frontend_check
  ALL PASS, browser visual check of title/toggle/tip styling.
  **Not yet checked in a DAW; the persistence round-trip (toggle → close →
  reopen session) needs a live host to verify.**

- **2026-08-20 — v1.6.0 10 new factory presets + preset browser dropdown.**
  Two new 5-preset banks (Expressive: bowing techniques; Texture: sound design),
  engineering-unit authored, skew-safe, 20 factory presets total. The preset-name
  display now opens a dropdown of all presets (factory + user) — one flat list in
  the C++ order so the ◀/▶ walk stays in sync; frontend-only, bridge surface
  unchanged at 32. DSP untouched: 21/21 goldens byte-identical, auval PASS,
  ui_frontend_check ALL PASS, re-seed sentinel produced all 20 preset JSONs with
  exact skew round-trip. **Dropdown not yet visually checked in a DAW.**
  User report: the noise component sounded fake. Root cause: band-passed white noise
  summed *after* both string and body — an uncorrelated hiss layer that never touched
  the instrument. Fix: pitch-synchronous feedback comb tuned to the tracked f0 (loop
  gain 0.85, ~4 kHz damped, 70/30 blend), slip-burst jitter (±4% period / ±30%
  amplitude from a second deterministic RNG stream), and the noise sum moved *before*
  the body resonator with a +12 dB pre-body makeup (the body's wet bank tops out at
  1.2 kHz, so noise survives via `(1−mix)·dry` — a −14 dB hit at default MIX=0.80).
  Measured on string-A: 700–3000 Hz within +0.8 dB of v1.4, top octaves −3 to −5 dB
  by design (damped comb tames the formerly over-bright hiss), overall RMS unchanged.
  All 21 goldens re-baselined and reproduce byte-identical; no new quality-gate
  failures vs v1.4 (a blockTime FAIL during batch regen was a wall-clock flake,
  re-anchored from a quiet run; the old `sub-harmonics` audibility FAIL now passes).
  No parameter/state/preset changes. **Not yet checked in Logic or Dorico.**
- **2026-08-13 — v1.4.0 crossfade-seed carve-out removed; 21st golden.**
  v1.2 skipped `seedFundamental()` on legato string changes, citing a `microtonal-scala`
  segment reading 230 cents. That branch **never executed in the probe that justified
  it**: `needsCrossfade` requires `activeStringIndex >= 0` and every note-on there
  landed on a fresh voice — instrumented, it reports 0 crossfades across 5 note-ons.
  The 230 cents was also an estimator artifact (autocorrelation on a harmonic string
  peaks nearly as hard at `2T` as `T`, giving a spurious ≈−1200 ¢; it reproduces in
  *both* arms of an A/B and vanishes once the search is constrained to ±6 semitones).
  Measured on a probe that does reach the path: seeding is **+13.2 dB** RMS, **+26.0 dB**
  at the note's own f0, and *better* in tune (6.7 ¢ → 1.1 ¢ max). Unseeded, a legato
  string change did not speak — the segment's pitch tracked the ringing outgoing string
  (49.03 Hz measured for a note played at 98 Hz). All 20 prior goldens byte-identical,
  because none of them ever reached the branch.
  ⚠️ Reaching `needsCrossfade` is not just "play fast": JUCE's voice-stealing heuristic
  hands a recycled voice back the pitch it last played, so a repeating passage always
  gives `newStringIndex == activeStringIndex`. Fill the 4-voice pool on ONE string, then
  leap to another. Any future claim about legato string changes must gate on the
  `crossfade_note_ons` liveness counter or it is measuring nothing.
  **Not yet checked in Logic or Dorico.**
- **2026-08-13 — v1.3.0 voice release + stealing; new `RELEASE` param (32 params).**
  The plugin went silent after four note-ons and stayed silent for minutes. Two causes:
  `MPESynthesiser` defaults voice stealing OFF, so `findFreeVoice` returned `nullptr`
  and the note-on was **silently discarded**; and a voice was only freed once every
  string fell under a −140 dBFS floor, which a 0.997 loop gain (~0.2 dB/s) never reached
  — measured still ringing at −68.5 dBFS 180 s after note-off. Fixed with
  `setVoiceStealingEnabled(true)` + `WaveguideString::startRelease()` (loop-gain damping
  to a pitch-independent T60). Sustain audio bit-identical: on `string-A` the first
  differing sample is 2646002 with note-off at 2646000. 20/20 goldens byte-identical
  (19 re-baselined + new `voice-recycling`), auval SUCCEEDED, pluginval-10 SUCCESS ×3.
  **Not yet checked in Logic or Dorico.**
- **2026-08-13 — v1.2.0 note-on string seed (first audio change since 1.0.0).**
  `trigger()` zeroed the rails and seeded nothing, so loudness tracked key hold-time
  rather than playing: a 0.35 s phrase peaked at −36.4 dBFS. `seedFundamental()` lays
  one velocity-scaled period of f0 across the round trip at note-on (`kSeedGain=1.5`).
  Phrase level +17.7 dB, hold-spread 17.6 → 3.1 dB, matrix stability 66 → 88/108.
  ⚠️ Rails must be pushed AND popped in lockstep — `DelayLine::pushSample()` advances
  only the write pointer, so bare pushes add N samples to the delay and detune the
  string by semitones. Also removed wall-clock `pass_blockTime` from the stability
  matrix verdict (it made passCount vary 97/102/98 across identical runs) and fixed
  `pass_rms` comparing sustain against the release tail. All 19 goldens re-baselined.
  See CHANGELOG [1.2.0], including the known microtonal segment-5 probe defect.

- **2026-08-12 — v1.1.0 measurement correction.**
  Closed DSP-07/08/09. DSP-09 (vibrato) and DSP-08 (breathing) were harness bugs; DSP-07's
  acceptance bar was unreachable at the output and re-specified to `subharmPeakOverFloor`.
  No DSP coefficient changed — all 19 render goldens byte-identical. Also added the missing
  `VERSION` keyword to `juce_add_plugin` (the bundle had been reporting JUCE's 1.0.0
  default regardless of the declared version). See CHANGELOG [1.1.0].

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

- **A tracked `.json.sha256` is written but never verified, and encodes wall-clock.**
  `reproduce-goldens.sh` checks only `<name>.wav.sha256`; the four tracked
  `<name>.json.sha256` files (microtonal-12tet / -scala / -mpe, vibrato) are rewritten
  on every `--regenerate` and compared by nothing. They could not be a gate as they
  stand: the JSON carries `blockMicros_median` / `blockTime_max_over_median`, which are
  wall-clock and differ run to run on the same binary. Either drop them or strip the
  timing fields before checksumming — do not "fix" it by starting to verify them.

- **Two stale AU variants remain installed:** `O-Contrabass-pre-2-5-dev.component` and
  `O-Contrabass-pre-port.component`. They carry distinct names so they do not shadow the
  shipped bundle in the AU registry (unlike the dev↔release pair the build script
  sweeps), but they are dead weight in Logic's plugin list. Safe to delete.

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
- ~~**DSP-07 sub-harmonic audible depth**~~ — **RESOLVED v1.1 (2026-08-12), acceptance
  re-specified; no DSP change.** The old bar `subharmEnergyRatio ≥ 0.40` is not
  reachable at the output and was never a retune problem: 0.40/0.358/0.241 were
  measured **pre-port/pre-body**, but the gate evaluates **post-body**, behind a
  resonator whose lowest mode is 60 Hz plus a 35 Hz one-pole HP (~13 dB of
  attenuation at 20.60 Hz vs 41.20 Hz). Sweeps put the reachable maximum at ~2e-04.
  **The two remedies this entry used to prescribe are dead knobs:** `kForceBoost`
  0.8→12.0 is bit-identical (the Schelleng ceiling pins `F_bow`) and `kV0Reduction`
  0.5→0.95 is bit-identical (`kV0Floor` clamp binds). `kFmaxScalar` is live but
  saturates at 1.99e-04. Acceptance is now `subharmPeakOverFloor ≥ 2.5`
  (1.888 disengaged → 3.019 engaged). Shipped `kGapWiden=0.25` is already optimal on
  that metric. Remaining option if a genuinely audible sub-octave is ever wanted:
  **post-body injection** (a topology change — a synthesised sub rather than emergent
  period-doubling — not a retune).
- ~~**DSP-08 slow-LFO breathing**~~ — **RESOLVED v1.1 (2026-08-12), harness bug.**
  The "15.7% vs 20%" symptom **does not reproduce**. `rmsByDecadePeakToPeakPct`
  buckets a 60 s sustain into 6 s decades against a 3.33 s LFO period, averaging ~1.8
  cycles per bucket, so it measured the drone's monotonic build-up rather than the LFO.
  Replaced by `lfoBreathingDepthPct` (⅛-period windows, per-period `(max−min)/max`,
  median across periods): **real breathing is 0.4524**, already past the 20% target.
- ~~**DSP-09 vibrato depth**~~ — **RESOLVED v1.1 (2026-08-12), harness bug — the DSP was
  always correct.** ⚠️ This entry previously prescribed tuning the
  VIBRATO_DEPTH→peakDepthCents transfer; **doing so would have driven real vibrato to
  ~19.4¢ to satisfy a broken meter.** `kVibFactorScale = −ln(2)/1200` is exact. The
  meter's `kAcWindowSize=4096` spanned 117 ms of a 200 ms vibrato cycle; sweeping only
  that constant gives 4096→7.42¢, 2048→10.70¢, 1024→11.18¢, converging on the true 12¢.
  Fixed to 1024 plus a cycle-derived per-cycle stride and peak window.
- **FUNC-07 MTS-ESP** — present-but-stub (returns 12-TET); v1.1 SDK linkage. Scala/TUN
  *import* itself is complete.
- **Dorico CC11 sustained-dynamics listener** — the Dorico bundle ships `kNoteVelocity`
  (velocity fixed at note-on). Continuous within-note crescendos/hairpins need a
  plugin-side CC11→`EXPRESSION_MACRO`/bow-pressure path, which touches param handling
  and risks the frozen goldens → v1.1.

## Regression harness

- Offline render harness is the Stage-2 correctness gate:
  `tests/render-harness/reproduce-goldens.sh` (**20** goldens, sha256 truth-bar).
  Build with `-DOUARICON_BUILD_TESTS=ON`; the target is `O-Contrabass-render-test`.
- ⚠️ **An absolute audibility floor cannot detect a dropped note.** `pass_allSegmentsAudible`
  (segment RMS > 1e-3) returned `true` all through v1.2.0 while note-ons 5–8 of an
  8-note sequence were being discarded, because the four voices still ringing held every
  segment above the floor. v1.3 added `pass_segmentLevelConsistency` (min/median ≥ 0.50),
  which scores that render 0.2748 instead. **The scenario matters as much as the gate:**
  the 5-note `note-sequence` golden plays one note per string, so only its last note-on
  was dropped and the ringing voices masked it (0.671 — a pass). The `voice-recycling`
  golden (8× the same note) is the probe that actually exercises voice reuse; keep both.
- Acceptance criteria beyond byte-identity live in the per-mode JSON (`pass_nan`,
  `pass_peak`, `pass_blockTime`, `pass_rms*`, vibrato rate/depth ranges, stability
  matrices). As of **v1.1**, `vibrato`, `slow-lfo` and `sub-harmonics` all PASS —
  their v1.0 FAILs were measurement defects, not DSP defects (see the resolved
  DSP-07/08/09 entries above). Remaining acceptance FAILs (stiffness-zero-pre,
  macro-sweep, detune-sweep-A, output-chain, string-*) are **pre-existing**
  level/shape-tolerance gaps, not stability failures, and were unchanged by this pass.
- ⚠️ **`pass_blockTime` is wall-clock and non-deterministic.** It gates
  `blockTime_max_over_median ≤ 5.0`; a single scheduler hiccup on a loaded machine
  spikes it (observed: vibrato 3.35 idle vs >5 under concurrent compilation). Re-run
  on a quiet machine before treating a `pass_blockTime` FAIL as a regression. Several
  modes zero these fields in the committed JSON precisely for sha256 stability.
- ⚠️ **Metric windows must be checked against the thing being measured.** Every v1.1
  defect was a window/stride sized wrongly relative to a modulation period or a render
  length, and each one passed review for a full release cycle while reporting a
  confident wrong number. When a metric disagrees with the design intent, sweep the
  metric's own constants before changing DSP to satisfy it.

## Build / validate

```bash
./scripts/build-and-install.sh O-Contrabass
auval -v aumu OCbs OuDv
# harness:
cd build && ninja O-Contrabass-render-test
plugins/O-Contrabass/tests/render-harness/reproduce-goldens.sh
```
