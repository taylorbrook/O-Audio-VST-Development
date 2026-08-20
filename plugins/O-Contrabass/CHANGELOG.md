# Changelog — O-Contrabass

All notable changes to the O-Contrabass physical-model bowed-contrabass synth.
Format loosely follows [Keep a Changelog]. **v1.0.0 is the first shipped product
version** — the pre-release `1.x-dev` engine track collapses into it.

## [1.5.0] — 2026-08-19 — bow noise made realistic (pitched, jittered, body-colored)

User report: "the noise part of the sound doesn't come off as realistic sounding."

### Root cause

The bow noise never touched the instrument. It was band-passed white noise
(700/1500/3000 Hz) summed into the output **after** both the waveguide string and
the body resonator — an uncorrelated hiss layer pasted on top of the tone. Real
bow noise originates at the bow-string contact and is comb-filtered by the string
(pitched at f0 harmonics) and colored by the body. Two secondary defects: the
slip-burst envelope retriggered *exactly* every period at *identical* amplitude
(a metronomic buzz), and the static noise spectrum ignored the played note.

### Changed — `BowNoiseGenerator` (Source/DSP/BowNoiseGenerator.h)

- **Pitch-synchronous comb filter**: feedback comb tuned to the tracked
  fundamental (loop gain 0.85, one-pole ~4 kHz damping in the loop,
  ≈ unity-RMS normalisation, 70/30 pitched/raw blend). The hiss now sings at
  harmonics of the played note.
- **Jittered slip bursts**: re-trigger period wanders ±4% and burst amplitude
  ±30% per event, from a *second* deterministic RNG stream (event-rate draws,
  sample-position determined — block-size invariance preserved; one stream per
  phase per pattern_rng_stream_interleave_blocksize).
- `prepare()` is no longer `noexcept` (allocates the comb delay buffer;
  prepare-time only, never on the audio thread).

### Changed — noise routing (Source/BowedContrabassVoice.cpp)

- Noise now sums **before** the body resonator (Step 8/9 swapped), so it picks
  up the same body coloration as the string signal.
- +12 dB (`4.0×`) pre-body makeup at the sum site: the body's wet bank tops out
  at 1.2 kHz, so the noise bands survive mainly via the `(1−mix)·dry` path — a
  −14 dB hit at default MIX=0.80 the old post-body sum never saw. Measured on
  the string-A render: 700–3000 Hz restored to within +0.8 dB of v1.4.0;
  3–6 kHz −3.0 dB / 6–12 kHz −4.8 dB remain by design (comb damping taming the
  formerly over-bright hiss). Overall RMS unchanged (−26.4 dBFS both).

### Compatibility

No parameter, state, or preset format changes — `BOW_NOISE` range and preset
values unchanged; sessions and presets load as-is. **All 20 render goldens
re-baselined** (`reproduce-goldens.sh --regenerate`) — the noise path is audible
in every render at the default BOW_NOISE=0.35, so byte-drift is the intended
outcome of this change, not a regression.

## [1.4.0] — 2026-08-13 — legato string changes speak too (crossfade-seed carve-out removed)

Removes the v1.2.0 "don't seed across a string crossfade" carve-out. Notes that land
on a freshly-crossfaded string are now **13.2 dB louder** and their own fundamental
**26.0 dB louder**, and they are *better* in tune, not worse. **All 20 pre-existing
render goldens are byte-identical** — the carve-out was unreachable in every one of
them, which is the whole story.

### Fixed — a carve-out justified by a measurement that never ran

`noteStarted()` skipped `seedFundamental()` whenever `needsCrossfade` was true. The
stated reason was that seeding into a crossfade window "superimposes a fresh
full-amplitude fundamental on a decaying neighbour a few semitones away", citing a
`microtonal-scala` segment that read 230 cents. Both halves were wrong:

1. **The calibrating probe never executed the branch.** `needsCrossfade` requires
   `activeStringIndex >= 0`, and every note-on in `28:1.5,33:1.5,38:1.5,43:1.5,28:1.5`
   landed on a *fresh* voice. Instrumented with the new liveness counter, that
   sequence reports **0 crossfades across 5 note-ons** — and under v1.2.0 its 5th
   note-on was dropped outright by the voice-allocation bug fixed in 1.3.0. What the
   230 cents actually measured was four ringing strings summing, not a retrigger.
2. **The 230 cents was an estimator artifact.** Autocorrelation on a harmonic bowed
   string peaks nearly as strongly at `2T` as at `T`; an unconstrained search reports
   a spurious ≈−1200 cents. It reproduces in *both* arms of the A/B and vanishes once
   the search is constrained to ±6 semitones.

Reaching the path at all is subtle: JUCE's voice-stealing heuristic hands a recycled
voice back the pitch it last played, so any repeating passage yields
`newStringIndex == activeStringIndex` and no crossfade. It takes filling the 4-voice
pool on one string and then leaping to another.

Measured that way, over 5 string pairs (E→G, G→E, E→D, D→A, A→G):

| crossfaded note | carve-out (v1.3.0) | seeded (v1.4.0) |
|---|---|---|
| RMS | −43.1 dBFS | **−29.9 dBFS** |
| level at own f0 | −60.8 dBFS | **−34.8 dBFS** |
| max pitch error | 6.7 ¢ | **1.1 ¢** |

Unseeded, the new string is so quiet that the segment's pitch tracks the *outgoing*
string — segment 5 of the first probe measured 49.03 Hz for a note played at 98 Hz,
which is the previous note (MIDI 31 = 49.00 Hz) still ringing. That is the actual
audible defect the carve-out caused: a legato string change did not speak.

- `BowedContrabassVoice::noteStarted()` — seeds **every** note-on; the
  `if (! needsCrossfade)` guard is gone.

### Added — `--crossfade-seed` render golden (21st)

A probe that genuinely reaches the legato string-change path, with a **liveness gate**:
it FAILS if `crossfade_note_ons < 5` rather than reporting a confident PASS over a
branch that never ran. Also gates the crossfaded notes at ≥ −36 dBFS RMS, a floor
sitting between the seeded (−29.9) and carve-out (−43.1) populations. Verified
discriminating by negative control: reinstating the carve-out fails the level gate
(0.0088 vs 0.0158) while liveness stays green, so it fails for the right reason.

- New voice counters `seed_applied` / `crossfade_note_ons` are emitted in every
  harness JSON. `note-sequence` and `voice-recycling` both report
  `crossfade_note_ons = 0`, confirming neither ever covered this path.
- `reproduce-goldens.sh` now **hard-fails** on a golden listed in `NAMES` with no
  committed baseline. Previously such an entry was silently skipped while the summary
  still read "all N reproduce byte-identical" — a new golden could look covered while
  contributing nothing.

## [1.3.0] — 2026-08-13 — the instrument keeps speaking (voice release + stealing)

Fixes the plugin going **completely silent after four note-ons** and staying silent for
minutes. Adds a `RELEASE` parameter. Sustain-phase audio is **bit-identical** to 1.2.0;
only post-note-off tails change.

### Fixed — every note-on past the fourth was silently discarded

Two defects compounded, both in voice allocation rather than the DSP:

1. **Voice stealing was never enabled.** `juce::MPESynthesiser` defaults
   `shouldStealVoices` to `false` (`juce_MPESynthesiser.h:317`), and the processor never
   called `setVoiceStealingEnabled`. `noteAdded()` therefore ran
   `findFreeVoice(note, false)`, which returns `nullptr` once all voices are busy — and
   there is no fallback path, so the note-on was **dropped without a trace**.
2. **A voice was never freed.** `renderNextBlock` releases a slot only when the bow is
   inactive and all four strings sit under a `1e-7` energy floor (−140 dBFS). Lifting the
   bow removes the energy *source* but left the loop gain untouched, and that gain floors
   at 0.997 per round trip — about **0.2 dB/s** at E1.

Measured on a single 0.5 s note with a 180 s tail: the string was still ringing at
**−68.5 dBFS after three minutes** and had not freed its voice. With `kNumVoices = 4`,
four note-ons took every slot and the instrument went dead.

Eight repeated E2 notes, per-note RMS:

| note | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| v1.2.0 | .0387 | .0399 | .0304 | .0342 | **.0139** | **.0087** | **.0085** | **.0084** |
| v1.3.0 | .0387 | .0389 | .0381 | .0382 | .0373 | .0410 | .0412 | .0410 |

Notes 5–8 in v1.2.0 are not quiet notes — they are the *decaying tail of voices 1–4*.

- `synth.setVoiceStealingEnabled(true)` — a note-on can no longer be discarded.
- `WaveguideString::startRelease()` / `cancelRelease()` scale the loop gain on note-off
  so the string reaches −60 dB in the configured time. The target is derived from the
  **played frequency** (`g = 10^(-3/(f0·T60))`), because round trips per second scale
  with f0 — a fixed per-round-trip gain would make an E1 release last 2.4× a G2 one.
  Ramped over 30 ms so the bow-lift reads as a gesture. The multiplier is exactly `1.0f`
  while not releasing, which is what keeps sustain bit-identical.
- All four strings are released, not just the active one — a voice that changed string
  mid-phrase leaves the previous one ringing, and one un-damped string pins the slot.

### Added

- **`RELEASE`** (0.05–20 s, skew 0.35, default **2.0 s**) — T60 of the string after the
  bow lifts. Pairs with `INFINITE_SUSTAIN`, which governs decay while the bow is *down*.
  New knob in the **Bow** panel's existing third slot; no panel geometry changed.
  The default is deliberately **not** a no-op: the behaviour it replaces is the defect,
  so sessions and presets saved before 1.3.0 adopt 2.0 s on load.

### Changed — test harness

- **New gate `pass_segmentLevelConsistency`** (min/median segment RMS ≥ 0.50).
  `pass_allSegmentsAudible` was vacuous for this failure: a dropped note still measures
  above its −60 dBFS floor because the previous notes are still ringing, so it returned
  `true` while half the sequence never sounded. Verified in both directions against the
  same binary — v1.2.0 scores **0.2748 (fail)**, v1.3.0 **0.9589 (pass)**.
- **New golden `voice-recycling`** — 8× the same note into a 4-voice pool, so note-ons
  past the fourth must recycle a voice. The existing `note-sequence` golden *cannot* see
  this bug: it plays one note per string, so only its final note-on is dropped and four
  ringing voices mask it (0.671 consistency — a pass). The two scenarios are not
  interchangeable.
- `pass_rmsContinuityAtTransitions` relaxed 0.50 → 0.35. Not a concession: the two
  transitions that were real articulations under *both* versions are unchanged to three
  decimals (1→2: 0.541→0.540, 3→4: 0.571→0.568). Every transition that "worsened" is one
  that previously produced no note at all, so the old bar was set partly by non-events.

### Validation

- **20/20 render goldens** reproduce byte-identical (19 re-baselined + 1 new).
- **Sustain phase provably untouched:** on `string-A` (60 s sustain, 5 s release) the
  first differing sample vs the v1.2.0 golden is **2646002**, and note-off is at sample
  2646000 — the entire sustain phase is bit-identical, with the change beginning two
  samples later (oversampler latency).
- `auval -v aumu OCbs OuDv` **SUCCEEDED**; pluginval `--strictness-level 10` **SUCCESS**
  on 3 consecutive runs of the same binary.

### Known gaps

- Not yet re-checked in Logic or Dorico — the four human gates carried forward from
  Stage 4 still stand.
- The v1.2.0 "don't seed across a crossfade" carve-out was calibrated on the 5th segment
  of the `microtonal-scala` probe. On a *fresh* voice `activeStringIndex < 0`, so
  `needsCrossfade` is false — that segment was a dropped note-on, not a retrigger. The
  carve-out was inert in practice before this release and becomes live now that voices
  are recycled. Worth revisiting on its own; deliberately left alone here.

## [1.2.0] — 2026-08-13 — the instrument speaks (note-on string seed)

Fixes the long-standing "sometimes it sounds, sometimes it's silent" behaviour.
**This is the first release since 1.0.0 that changes audio** — all 19 render goldens
are re-baselined.

### Fixed — notes never spoke at playable lengths

`WaveguideString::trigger()` zeroed both delay rails and seeded nothing, so the
string had to accumulate to Helmholtz motion purely through friction in a loop whose
gain floor is 0.997. Loudness therefore tracked **how long the key was held**, not how
it was played:

| hold | v1.1 | v1.2 |
|---|---|---|
| 0.1 s | −38.7 dBFS (inaudible) | speaks immediately |
| 8.0 s | −22.1 dBFS | — |
| **spread** | **17.6 dB** | **3.1 dB** |

On a real phrase of 0.35 s notes the plugin peaked at **−36.4 dBFS**; it now reaches
**−18.7 dBFS (+17.7 dB)**. Velocity was also nearly inert (1.6× across its whole
range) because it only trimmed bow force into a loop that was still climbing; the
seed is velocity-scaled, so key velocity now sets initial amplitude directly.

- `WaveguideString::seedFundamental()` lays one period of the fundamental across the
  round trip at note-on, amplitude `kSeedGain · velocity · bowSpeed` (`kSeedGain = 1.5`,
  chosen by ear from a rendered A/B set). Deterministic single period rather than a
  noise burst: noise needs an audio-thread RNG (breaks block-size invariance) and adds
  a chiff a bow attack should not have. Endpoints are zero-valued, so no click.
  Attack transient is 2.08× peak-over-sustain vs 1.95× unseeded — essentially
  unchanged, while hold-dependence drops 17.6 dB → 3.1 dB.
- **Rails are pushed AND popped in lockstep.** `juce::dsp::DelayLine` keeps
  independent read/write pointers and `pushSample()` advances only the write pointer,
  so filling a rail with bare pushes adds N samples to the effective delay. That
  detuned the string 2–3 semitones and broke every pitch gate (vibrato read 306¢ and
  20.4 Hz). Popping in lockstep preserves the configured delay.
- Measured DSP stability **improves**: 66/108 → 88/108 matrix cells. The unseeded
  build failed 42 cells because notes never settled — the same defect users heard.
  All 108 cells are NaN-free and peak-clean; the 20 remaining failures are
  `clickFree` at BOW_PRESSURE = 7.0, where a real string goes raucous above
  Schelleng's maximum bow force.

### Fixed — the stability matrix could not detect a stability regression

Each of the 108 cells computed `pass_combo = … && pass_blockTime`, putting **wall-clock
timing inside the DSP stability verdict**. Three consecutive runs of the same binary
returned passCount **97 / 102 / 98**. R36b had already relaxed the threshold 5.0× →
50.0× on the stated grounds that btRatio "is dominated by OS scheduling noise, not DSP
stability"; this completes that reasoning by removing it from the verdict entirely.
Stability is now deterministic (102/102/102 on repeat runs); timing is still measured
and reported as `blockTimePassCount`, explicitly outside the verdict. Applied to the
36-cell sub-harmonics matrix for the same reason.

### Fixed — `pass_rms` compared sustain against the release tail

`rmsFinal` used `totalSeconds`, which on a 60 s sustain + 5 s release lands **inside
the release tail**, so the ratio compared mid-sustain against a decaying tail. It
passed in v1.1 only because the instrument was broken — notes were still ramping at
s5–s6, so `rmsMid` was as anemic as the tail. Once notes speak, a released string is
legitimately ~15 dB down (string-A: mid 0.0535, tail 0.0093 → ratio 0.17), tripping
`pass_rms` across nearly every sustained mode. Now measured sustain-to-sustain.

### Added — harness bow-operating-point overrides

`--bow-speed` / `--bow-pressure` / `--bow-position` in engineering units, converted
through each parameter's own `NormalisableRange` so the 0.5 skews are handled by the
range rather than re-derived. Added because the Schelleng playable region
(`F_min ∝ v_b / β²`) was unmappable without them. Also `reproduce-goldens.sh
--regenerate`, driven from the same NAMES/INVOCS arrays as verification so a
re-baseline can never drift from the invocations it verifies.

### Known issues

- **The microtonal probe's final segment is unreliable, in both tunings.** v1.0
  shipped `microtonal-12tet` segment 5 at **386.8¢** and called it PASS — the
  tolerance is only enforced when `tuningSystemArg == "scala"`, so 12tet could never
  fail on pitch. v1.2 reads 344.77¢ there (improved) and 230.49¢ on scala (v1.0:
  26.42¢, itself already past the 10¢ tolerance while its golden recorded
  `pass_pitchAccuracy: true`). Pre-existing probe defect, magnitude shifted by the
  seed; almost certainly the analysis window running past the sequence end, the same
  out-of-domain class as the `rmsMid` bug. **Not root-caused — the instrument is in
  tune on segments 1–4 (≤0.5¢) and `microtonal-12tet` / `microtonal-mpe` /
  `note-expression` all pass.**
- Legato string changes are **not** seeded (`needsCrossfade`), so they keep the v1.1
  onset; the outgoing string covers the transition.

## [1.1.0] — 2026-08-12 — v1.1 measurement correction (DSP-07/08/09)

Closes the three v1.1 DSP deferrals — **by fixing the measurements, not the DSP.**
Investigation established that two of the three "DSP defects" were harness bugs and
the third had an unreachable acceptance bar. **No audio-path source changed: all 19
render goldens remain byte-identical** and the frozen-DSP invariant holds end to end.

### Fixed — DSP-09: vibrato depth was a meter artifact, not a transfer deficit

`peakDepthCents` read 7.42¢ against a 12¢ setting, and `NOTES.md` prescribed
"tune the VIBRATO_DEPTH→peakDepthCents transfer" to land in `[10,14]`. That would
have driven **real vibrato to ~19.4¢ to satisfy a broken meter.** The DSP was correct
throughout — `kVibFactorScale = -ln(2)/1200` is exact.

- `kAcWindowSize` **4096 → 1024**. Autocorrelation correlates `[s, s+N)` against
  `[s+tau, s+tau+N)`, spanning `N + tau ≈ 5166` samples = **117 ms**, against a
  200 ms vibrato cycle — averaging pitch over 58.6% of a cycle. Proven by holding
  the stimulus at 5 Hz and sweeping only this constant: **4096→7.42¢, 2048→10.70¢,
  1024→11.18¢**, converging on the true 12¢ as the span → 0.
- `perCycleDeltaCents` stride **28 → derived** (`kHopsPerVibCycle`). 28 hops is
  162.5 ms against a 200 ms cycle, so it walked through the vibrato phase and emitted
  sign-flipped values (v1.0 golden: `+7.07, +7.05, −0.76, −6.50, −3.44`) that read as
  instability but were pure aliasing. Now reads `7.53, 9.87, 9.97, 9.88, 6.79, 5.08`.
- Peak-swing window **36 hops → `kHopsPerVibCycle + 2`**. The old comment claimed
  "≈3 vibrato cycles (600 ms)"; 36 hops is actually 209 ms ≈ **one** cycle.

`pass_vibratoDepthInRange` now **true** at 11.18¢; mode status FAIL → PASS.

### Fixed — DSP-08: breathing metric could not see the LFO it gated on

`pass_breathingAudible` gated on `rmsByDecadePeakToPeakPct`, which buckets a 60 s
sustain into 10 decades — **6 s per bucket against a 3.33 s LFO period**, averaging
~1.8 full cycles so the modulation cancels. Its 0.694 was the drone's monotonic
build-up (decades rise 0.0263→0.0602 strictly), so the gate passed **without ever
measuring breathing.** The documented symptom ("15.7% vs 20% target") does not
reproduce at all.

- Added `lfoBreathingDepthPct` — windows of ⅛ LFO period, depth per period as
  `(max−min)/max`, reduced by **median** across periods so build-up cannot inflate it.
- Gate moved onto it at the unchanged 0.15 bar. **Real breathing measures 0.4524**,
  comfortably past the 20% architectural calibration target the LFO was assumed to miss.

### Changed — DSP-07: acceptance re-specified to a reachable metric

`subharmEnergyRatio ≥ 0.40` is **not reachable at the plugin output** and was never
a retune problem. The 0.40/0.358/0.241 figures come from RESEARCH §18.3/§18.5, which
measured **pre-port and pre-body**; the gate evaluates **post-body**, downstream of a
resonator whose lowest mode is 60 Hz (`BodyResonator.h:85`) plus a 35 Hz one-pole HP
(`BodyResonator.cpp:62`) — together attenuating 20.60 Hz by ~13 dB relative to
41.20 Hz before measurement. Coefficient sweeps confirm a reachable maximum of
**~2e-04, three orders of magnitude short**:

| coefficient | swept | result |
|---|---|---|
| `kForceBoost` | 0.8 → 12.0 | **bit-identical** — Schelleng ceiling pins `F_bow`; a dead knob |
| `kV0Reduction` | 0.5 → 0.95 | **bit-identical** — `kV0Floor=0.005` clamp binds; a dead knob |
| `kFmaxScalar` | 0.95 → 20.0 | saturates at 1.99e-04 by ~3.0; degrades `peakOverFloor` 3.02→1.61 |

The committed baseline note claiming `0.241 at SUB_HARMONICS=0` also does not
reproduce — measured directly it is **6.38e-05**, the same order as the engaged
value, which is precisely why the ratio cannot discriminate.

- Acceptance moved to **`subharmPeakOverFloor ≥ 2.5`** (soft band 2.0–2.5), a local
  signal-to-floor measure at f0/2 so the shared output-chain attenuation divides out.
  It tracks the feature properly: **1.888 disengaged → 3.019 engaged.**
- **No DSP coefficients changed.** A `kGapWiden` sweep on the corrected metric shows
  the shipped 0.25 already at the optimum (0.0→1.367, 0.10→1.506, **0.25→3.019**,
  0.50→2.001); response near it is chaotic (0.22→3.367, 0.28→2.025), so chasing
  ~12% would overfit one note and one operating point.

`pass_subharmAudible` now **true** at 3.019; mode status FAIL → PASS.

### Fixed — mis-scoped and stale-documented gates

- **`rmsContinuity` per-mode thresholds.** A minimum-adjacent-window-ratio gate at
  ≥0.90 is a steady-state measure; it was applied to modes that modulate amplitude by
  design. slow-lfo **0.85** (92.9 ms window = 2.79% of a 3.33 s cycle, so ±60% bow-speed
  modulation moves ~10.5% between adjacent windows), vibrato **0.75** (92.9 ms against a
  200 ms cycle = 46.4%, so adjacent windows sample near-opposite phases). v1.0 already
  shipped 0.85 for macro-sweep on this exact reasoning. Real dropouts drive the ratio
  toward 0, so click detection is preserved.
- **`rmsMid` domain guard.** The window was hard-coded to seconds 5–6; `--vibrato`
  renders 3 s total, so it clamped to an empty span, returned 0, and tripped the
  "engine never started" branch on a healthy render. Falls back to the middle second
  of the actual render below 6 s; longer renders are untouched.
- **Header pass-condition block corrected** to match the enforced code: depth band is
  `[9,14]` not `[10,14]`, onset `[800,1200] ms` not `[800,1000]`. This drift is what
  sent the v1.1 plan chasing phantoms in the first place.

### Fixed — build: plugin version never reached the bundle

`juce_add_plugin` carried **no `VERSION` keyword**, so the artefact reported JUCE's
1.0.0 default — v1.0.0 was correct only by coincidence and any bump would have been
silently discarded. (`PLUGIN_VERSION` is *not* a JUCE keyword.) Added `VERSION 1.1.0`;
verified `CFBundleShortVersionString` = **1.1.0** in both the VST3 and AU bundles.

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
