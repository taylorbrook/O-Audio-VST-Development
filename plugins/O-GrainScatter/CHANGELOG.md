# Changelog

## [3.5.0] - 2026-10-28

### Added
- **Grain Adapt** (`grain_adapt`, 0–100 %, default 0), research 2026-update #11 after Nielsen, *Keyframe Time Stretching via Extrema Sampling* (DAFx26). While Freeze is held, each new grain takes its length from the captured audio where it starts. Grains run longer over sparse passages (a bass note: few, widely spaced local extrema) and shorter over dense ones (noise, a hit: extrema packed tight). Grain Size stays the average length. Live grains are never touched, at any setting.
  - **Analysis:** each Freeze engage analyses the held loop as it is heard (the mid channel, with the seam crossfade). The steps are the cubic B-spline derivative (zero at Nyquist), a −60 dBFS hysteresis threshold, and sub-sample extremum positions by reverse linear interpolation. The work is spread over the first blocks, 32 analysis samples per output sample, and allocates nothing.
  - **Ready time:** the analysis is ready exactly `ceil((2048 + loop + 5 × 4096) / 32)` samples after engage (76.9 ms for a 2 s capture at 48 kHz) whatever the block size. Grains spawned before that keep the fixed length.
  - **Length:** the span L of the next K0 extrema ahead of the grain's start, mapped as `len = clamp(GS · exp(a · (ln L − ln L_median) + c), 10 ms, 500 ms)`.
    - K0 is fixed per capture so the median span is 100 ms.
    - c is re-solved whenever Grain Size or Adapt moves, so the clamped mean over the loop equals Grain Size. Turning Grain Size moves every unclamped grain proportionally, with no re-analysis.
    - a = Adapt / 100 is the contrast exponent. On a 1 s bass-sine / 1 s noise capture at Grain Size 100 ms, the sine half's grains average 2.8× the noise half's at Adapt 50 and 5.8× at Adapt 100.
  - **Reverse grains:** they keep their Adapt 0 start and measure the extrema behind it, which is the audio they read.
  - **Interactions:**
    - Size Random scales the adapted length with the same random draw.
    - Shape Tilt follows the adapted length.
    - Freeze scatter stays ±1 Grain Size, so a grain starts at the same point at every Adapt value.
    - Lengths are in output samples at every pitch.
  - **Fallbacks to the fixed length:** a capture with fewer than 8 extrema (silence, noise under about −70 dBFS) and a loop shorter than 0.2 s.
- UI: an **Adapt** knob beside Grain Size in Core Engine, with tooltips in English, French and Simplified Chinese (fr at `reviewed: false`, zh-Hans at `'mt'`). The tip says it acts only while Freeze is held.
  - Core Engine cells are 58 px with a 3 px gap (were 62 / 6), so seven knobs fit in row 1. The dial pitch is unchanged at 61 px and the 900 × 800 frame is unchanged.
  - FR Tilt is now captioned "Inclin.": "Inclinaison" (65 px) already overran its 62 px cell in 3.4.0.

New parameter only: no preset or state change. A session saved by v3.4.0 or earlier loads with Adapt 0 and renders bit-identically. AU parameter version hint 5.

### Known limitations
- **Ramp-up:** for ~75 ms after each engage, grains play at the fixed length while the analysis runs.
- **Clamp edges:** adaptation fades near the 10 ms and 500 ms ends of Grain Size, because the mean is pinned. At Grain Size 400 ms and Adapt 100, sparse/dense lengths only span 261–500 ms.
- **Quiet material:** the −60 dBFS threshold is absolute, so quiet dense material reads as sparse (long grains). Anti-phase stereo content cancels in the mid channel and also reads as sparse.
- **Low density thins out:** at low Density, short grains over dense passages stop overlapping. The cloud thins there and density compensation does not boost, the same as turning Grain Size down.
- **Level at Scan 0:** the level moves with Adapt when Scan is near 0. Freeze scatter puts about half the grains on the read-head floor, where they start in step and sum coherently. Density compensation uses one coherence figure for the whole cloud, but this cluster is coherent over sustained audio and decorrelates over noisy audio (the ±6 ct freeze detune). On a 1 s sine / 1 s noise capture this measured +1.3 dB (Adapt 50) and +1.7 dB (Adapt 100) against Adapt 0. With Scan away from the floor (30 %) it is 0.0 / −0.2 dB.

### Testing
- New gate `tests/adapt-gate` with three binaries from one driver:
  - `O-GrainScatter-adapt-gate` (this build, with a spawn log compiled into the test target only);
  - `-adapt-gate-v340`, the same driver against v3.4.0 (commit 144a73f3, extracted with git archive), which dumps the references;
  - `-adapt-gate-fixedk`, the negative control: CONTEXT's first guess, K = GS · M / len with no calibration.

  Run: `O-GrainScatter-adapt-gate-v340 <dir> && O-GrainScatter-adapt-gate <dir> && O-GrainScatter-adapt-gate-fixedk`. 47 / 47 PASS plus the negative control:

  | Row | Result |
  |---|---|
  | ExtremaMap vs the research prototype: sine 55 Hz / noise / silence | M 220 / 47 031 / 0 (proto 220 / 47 030 / 0), K0 11.0 / 2366.4 |
  | Adapt 0 vs v3.4.0: free, sync, spatial, freeze, freeze + reverse + Size Rnd + Tilt, freeze +24 st, freeze spatial, freeze sync re-engage | peak diff 0, 8 / 8 |
  | Adapt 100 on live paths (free, sync, spatial) vs v3.4.0 | peak diff 0, 3 / 3 |
  | v3.4.0 session into an instance with Adapt 50 | Adapt reads 0, bit-exact |
  | Sine-half / noise-half mean length, GS 100: Adapt 100 / 50 / 0 | 5.78 / 2.80 / 1.0000 |
  | Mean adapted length vs Grain Size (2387 spawns each), GS 50 / 100 / 200, sparse/dense + bass+snare | within −0.2 … +2.0 % forward, −1.6 … +1.3 % reverse (bound 5 %) |
  | GS 50 → 100, unclamped grains | ratio 2.000000 on 2387 / 2387; logged lengths = the map's lookup, 0 mismatches |
  | Fallbacks: silent capture, −80 dB noise, 85 ms loop | 0 adapted, bit-exact vs Adapt 0 |
  | Spawns before ready | output bit-exact vs Adapt 0 up to readyAbs; first adapted spawn after it |
  | 32 / variable vs 512 blocks: engage, Adapt 0 → 100 + size jump, re-engage mid-analysis, spatial | −400 dBFS, 4 / 4 |
  | First adapted spawn across splits | identical; readyAbs = engage + 3690 (76.9 ms) |
  | Allocations in processBlock (32 + variable blocks through engage, analysis, re-engage, release, Adapt 0 ↔ 100) | 0 (liveness probe counts 1) |
  | Level, sparse/dense GS 200, Scan 30, Adapt 50 vs 0 (16 seeds × 8 s) | −0.003 dB (old-vs-old 0.007 dB, length ratio 2.83) |
  | Dense noise cloud, Adapt 100 vs 0 | 0.000 dB |
  | Analysis step, worst 32-sample block (median of 9) | 11.1 µs at 48 kHz, 14.1 µs at 96 kHz (bound 50) |
  | Negative control, fixed K | mean 237 ms at GS 100 (+137 %): FAILS as it must |

- Existing gates rerun: blocksize-null 22, density-gain 25, skip-tilt 31 and alias-gate all PASS; the v3.2.0 alias negative control still fails. `tests/alias-gate` passes the engage sample to `FreezeManager::engage`.
- UI: check-i18n, i18n-fr-lint, i18n-zh-lint and check-ui-labels PASS at EN / FR / zh-Hans, and `tests/ui_tip_render_check.js` passes 1366 / 1366.

## [3.4.0] - 2026-10-28

### Added
- **Grain Skip** (`grain_skip`, 0–100 %, default 0). Each grain is dropped with this probability just before it starts (research 2026-update #6, after Tempera 2.3). It works in Free and Sync mode and on every Sync repeat individually, so a stutter of four repeats loses single hits instead of whole triggers; Probability still cancels whole triggers. The decision is a counter-RNG draw in its own slot, keyed to the grain:
  - a skipped grain still uses up its counter, so the grains that do play get exactly the pitch / pan / size / position they would get at Skip 0;
  - at Skip 0 nothing is drawn, and renders are bit-identical to v3.3.0;
  - Stutter Gate opens only when the trigger's first grain actually plays. If Skip drops it, the dry stays open for that trigger and any surviving repeats play over it, so Skip 100 % with Stutter Gate on leaves the dry untouched. The gate assignment simply moved after the skip check: no added state or per-sample cost. Opening the gate on the first *surviving* repeat instead would have meant carrying each trigger's gate length through the pending-repeat queue, so it was not done.
- **Shape Tilt** (`shape_tilt`, −100 to +100 %, default 0). It moves the peak of the grain window: −100 gives a plucked attack with a long fade, +100 a slow swell that stops short, and 0 is the current symmetric window. The window's phase is warped piecewise-linearly so its peak lands at the tilt point, with the same shape on each side:
  - each side keeps at least 3 ms, so neither edge becomes a click. On grains of 6 ms or less the tilt has no effect, and on a 10 ms grain the peak travels from 30 % to 70 %;
  - the tilt is latched when each grain starts, so automating it never reshapes a grain that is already playing;
  - reverse grains keep the tilt in time order: a plucked reverse grain still starts hard;
  - for the five symmetric shapes the warp leaves the window moments μ₁ and μ₂ unchanged, so density compensation and loudness are unaffected. Exp Decay is not symmetric: tilt changes its decay rate and its energy (−100: −11.3 dB in a 10-grain cloud). The moments follow the tilt and the compensation still never boosts, so the level tracks the energy, as with choosing a quieter shape.
- UI: a **Tilt** knob next to Shape (Core Engine) and a **Skip** knob next to Probability (Beat Sync), with tooltips in English, French and Simplified Chinese (fr at `reviewed: false`, zh-Hans at `'mt'`). The left column of the lower panel is 20 px wider (451 / 411 px) to fit them. The 900 × 800 frame and every other control are unchanged.

New parameters only: no preset or state change. A session saved by v3.3.0 or earlier loads with Skip 0 and Tilt 0 and renders bit-identically. AU parameter version hint 4.

### Testing
- New gate `tests/skip-tilt` (target `O-GrainScatter-skip-tilt`). Its twin `-skip-tilt-v330` compiles the same driver against v3.3.0 (commit d920bedd, extracted with git archive) and dumps the references: `O-GrainScatter-skip-tilt-v330 <dir> && O-GrainScatter-skip-tilt <dir>`. 31 / 31 PASS:

  | Row | Result |
  |---|---|
  | Defaults vs v3.3.0: free, sync (repeats, swing, Euclid, gate), spatial (trajectory, Doppler), freeze, +24 st, Trapezoid, Exp Decay | peak diff 0 (bit-exact), 7 / 7 |
  | v3.3.0 session loaded into an instance with Skip 50 / Tilt 80 | params reset to 0 / 0, bit-exact |
  | Skip 100 %: free / sync / spatial | wet silent, 0 spawned (600 / 190 / 600 skipped) |
  | Skip 25 / 50 % (600 grains) | 29.8 / 51.2 % skipped (within 4 sd) |
  | Sync Repeats 4, Skip 50 % | 190 scheduled with and without Skip, 53.7 % skipped (skipping only the first grain of each trigger would cap it at 25 %) |
  | Stutter Gate, wet muted: Skip 100 % / 0 % / 50 % | dry untouched, 0 samples gated / 100 % gated / 75 % of the Skip 0 count |
  | Skip 50 + Tilt at 32 / variable blocks vs 512: free, sync, spatial | −400 / −400 dBFS (bit-exact) |
  | Peak position: tilt −1 / 0 / +1 on 100 ms; −1 on 10 ms; 6 ms grain | 0.03 / 0.50 / 0.97; 0.30; 0.50 |
  | Envelope step at ±100 %, 5 shapes, 10 ms – 1 s (bound 0.025) | worst 0.022 per sample (Tukey); symmetric 0.013 |
  | Negative controls: floor removed / 2 ms floor | step 1.0 / 0.033, both over the bound (FAIL as intended) |
  | μ₁ / μ₂ vs numeric integral, 6 shapes × peak 0.02 – 0.98 | rel. err < 1e-6 |
  | Rendered DC grains at ±100 % (5 shapes, 10 / 200 ms) | step / peak 0.022 (tilt 0: 0.013) |
  | Tilt −100 → +100 jump mid-grain, 500 ms grains | step / peak 0.007 |
  | Dense noise cloud at ±100 % vs 0 (Hann, Trapezoid) | ≤ 0.014 dB |
  | Exp Decay level vs moments prediction | −11.33 vs −11.39 dB; +0.00 vs +0.00 dB |

## [3.3.0] - 2026-10-27

### Changed
- **Pitched-up grains no longer alias.** v3.2.0 read the ring with 4-point Lagrange and no filtering, so from +12 st up every component above the output Nyquist folded back into the audible band at full level (0 dB). Grains now read through the grain-engine 1.1.0 band-limited reader:
  - the 12 s ring also keeps 1/2, 1/4 and 1/8-rate copies, built on the write path by a 127-tap half-band with its delay compensated, so the copies are time-aligned with the ring;
  - a grain latches the octave copy for its rate at spawn (+12 st and up read a decimated copy) and reads it with a 16-tap windowed sinc whose cutoff follows the remaining rate;
  - every audible alias is now at or below −60 dB (gate below). The spatial path re-picks the sinc cutoff per sample as Doppler moves the rate.
- **The interpolator changes at unity pitch too.** Grains at any pitch now use the sinc, not Lagrange. Its −1 dB point at unity is 16.2 kHz (44.1 kHz) / 17.7 kHz (48 kHz); Lagrange's was 12.4 / 13.5 kHz averaged over fractions. White noise above about 20 kHz is now filtered out: the coherent density rows read about 0.7 dB lower than in v3.2.0.
- **The top end moves within each octave of pitch.** With 16 taps the output −1 dB point (44.1 kHz) runs from 16.2 kHz just above an octave boundary down to 8.3 kHz just below the next one, so +11.9 st is duller than +12.1 st:

  | Pitch | −1 dB @ 44.1 k | @ 48 k |
  |---|---|---|
  | 0 / +12 / +24 / +36 st | 16.2 kHz | 17.7 kHz |
  | +3 st | 13.5 | 14.7 |
  | +6 st | 13.0 | 14.2 |
  | +9 st | 9.6 | 10.4 |
  | +11.9 / +23.9 / +35.9 st | 8.3 | 9.1 |
  | −6 / −12 st | 11.5 / 8.1 | 12.5 / 8.8 |
- **Freeze captures audio ending about 10 ms before you engage it** (506 samples). That is where every octave copy is settled. The loop length is unchanged.
- **Fix: Doppler could read past the write head.** The spawn pre-roll used the base rate only, while the spatial path advances by rate × Doppler (up to 2×). A grain at the read-head floor could step ahead of the write head and read 12 s-old audio. Every read is now clamped to the settled history (9 / 80 / 222 / 506 samples behind the head for the four octave copies). The spawn guard and the density estimate use the same margins; grains pinned to the floor sit up to 11 ms further back at +36 st.
- **Memory and CPU.** The octave copies add 0.875× the ring (192 kHz: 18.4 MB → 34.6 MB) and each freeze slot grows 1.875×. All of it is allocated in prepare. 64 grains at +24 st, 48 kHz: 871 ns per output sample stereo (v3.2.0 477), 1234 ns spatial (v3.2.0 765). That is 4.2 % / 5.9 % of one core, against 2.3 % / 3.7 %.

No parameter, preset or state change.

**Known limits.** Doppler above 1.41 on a grain near the top of an octave (remaining rate 2.83–4) is best effort: about −38 dB at +35 st with Doppler 2. A Doppler below about 0.9 on a grain at +12 st or higher can image the 10–11 kHz band (per octave) at up to −30 dB. Doppler in normal motion stays within ±1 %, so both only happen on trajectory jumps.

### Testing
- New gate `tests/alias-gate` (target `O-GrainScatter-alias-gate`). Its twin `-alias-gate-v320` compiles the same driver against v3.2.0 (commit 9c6c22d0, extracted with git archive) as the negative control. Total audible spurious power, 44.1 kHz:

  | Row | v3.2.0 | v3.3.0 |
  |---|---|---|
  | +12 st stereo: tones / out-of-band sweep | 0.0 / −0.3 dB | −95.8 / −108.2 dB |
  | +24 st stereo | 0.0 / −0.4 dB | −95.9 / −99.1 dB |
  | +36 st stereo | 0.0 / −0.5 dB | −95.4 / −80.8 dB |
  | +24 st through processBlock | −0.2 dB | −117.0 dB |
  | +36 st spatial, Doppler 1.0 / 1.4 | 0.0 / 0.0 dB | −80.8 / −63.5 dB |
  | Frozen +36 st | 0.0 dB | −95.3 dB |
  | Doppler 2 (report): +36 st / +35 st | 0.0 / 0.0 dB | −67.3 / −38.5 dB |
  | Octave-boundary lag, rate 1.99 vs 2.01 | — | 0.000 samples (a 0.2-sample offset reads 0.197) |

- `tests/blocksize-null`: every render row still nulls bit-exact across 32 / 512 / variable blocks. The ring-precision row now compares reads on a ring whose head is past 2^21 against the same reads on a freshly started ring, on all four octave copies: −400 dB (threshold −100 dB unchanged). The analytic-sine reference measured Lagrange, not position precision. Negative control: a float read position gives −47.9 dB (FAIL).
- `tests/density-gain`: ALL PASS. The sparse row now nulls against this build with density compensation forced off (`O-GrainScatter-density-gain-compoff`), because the new reader changes every grain's samples; the result is bit-identical (threshold −70 dB unchanged). Negative control: a 0.99 gain gives −40 dB (FAIL).
- grain-engine module tests: 38/38. They cover unity DC on every sinc phase, level alignment (≤ 0.003 sample), the half-band stopband (−92.0 dB), the driven S0–S6 stopbands (−60.6 to −94.4 dB), block-split invariance, no allocation, and snapshot reads that are bit-identical to live reads.

## [3.2.0] — 2026-10-27

### Changed
- **Density no longer changes loudness or adds distortion.** The wet signal used to go through `tanh`, in both the stereo path and after the binaural decode. Grains summed with no level correction, so the wet level rose with overlap. At 500 ms grains and 100 g/s it was +17 dB over the input before the clip, and a 0.5-peak sine came out with H3 at −33 dB. Density was acting as a drive control. The wet now gets a closed-form gain from the live grain count N instead (research 2026-update §3.2):
  - power `P = N·μ₂ + ρ·N(N−1)·μ₁²`, gain `1/√max(1, P)`;
  - μ₁ and μ₂ are the moments of the selected grain shape;
  - ρ is a stand-in for grain correlation. It is 1 when grains read the same audio in step (Spread 0, one pitch, no reverse) and falls to 0 by Spread 25%. It is 0 while frozen, and also 0 once Pitch Random spreads the pitches. Grains pinned to the read-head floor (small Scan, wide Spread) count as coherent with each other;
  - P is formed per sample and smoothed with a 100 ms one-pole;
  - fewer than one grain is never boosted, so sparse settings are unchanged.
  
  The feedback `tanh` is unchanged. It is fed from the compensated wet.
- **Freeze scatters grains by at least one grain length.** At low Spread, frozen grains re-read nearly the same snapshot audio at a fixed hop, so a hold played back as the captured loop. While frozen, the position scatter is now at least ±1 grain length (Spread above 100% still widens it), and the hold becomes a texture. It uses the same random draw as before, so live grains are unchanged. The per-grain freeze detune stays at ±6 ct (v3.0.4).

No parameter, preset or state change. Dense settings are quieter and cleaner than in v3.1.0. Sparse settings sound the same. Wet peaks are no longer soft-clipped, so a hot input with a single grain can exceed 0 dBFS. Use Wet Level.

### Testing
- New gate `tests/density-gain` (target `O-GrainScatter-density-gain`). Its twin `-baseline` compiles the same driver against the v3.1.0 backup as the negative control. Levels are relative to the input at centre pan:

  | Case | v3.1.0 | v3.2.0 |
  |---|---|---|
  | Incoherent, N = 4 → 20 (200 ms) | +0.7 → +7.3 dB | −1.0 → −1.2 dB |
  | Incoherent, N = 50 (500 ms), Scan 10% / 60% | +17.3 / +11.1 dB | +0.6 / −1.1 dB |
  | Incoherent, N = 20, Scan 0 (half the cloud on the read floor) | +14.4 dB | +0.4 dB |
  | Coherent, N = 4 → 20 | +5.9 → +18.0 dB | −0.5 → −0.1 dB |
  | Sine, 0.5 peak, N = 20: H3/H1 | −33 dB | −158 dB |
  | Freeze, Spread 0: output correlation one capture loop apart | 0.29 | 0.004 |
  | Frozen level, N = 12.6 | +6.5 dB | −1.1 dB |

  Other rows:
  - μ₁/μ₂ for all six shapes match a numeric integral of the envelope.
  - A sparse render (N ≈ 0.08) nulls against v3.1.0 to −78.6 dB. The residual is v3.1.0's `tanh` curvature, so the gain is 1 below one grain.
  - Cold-start overshoot into N = 20 is +1.7 dB for the worst 20 ms.
- `tests/blocksize-null`: all 21 rows still pass, bit-exact in every path.
- pluginval strictness 10 (VST3) passes. The AU is registered (`aufx OuGS OuDv`).
- Not measured: spatial-mode level vs density. The same gain is applied to the HOA sum, but no gate renders it.

## [3.1.0] — 2026-10-27

### Fixed
- **Sync repeats now play.** Repeat grains were scheduled at `trigger + r × one subdivision` and dropped if that landed past the end of the current block. A subdivision is thousands of samples (6000 for 1/16 at 120 BPM), so at normal block sizes almost every repeat was lost. At 512-sample blocks the scheduler emitted 48 of 186 grains in the gate's 6 s test (primaries only). Pending repeats now wait in a fixed 256-entry queue on an absolute sample clock and fire in the block they fall in. **Audible:** with the default Repeats 4, sync mode now plays up to four grains per trigger.
- **Stutter Gate spans blocks.** The gate was clamped to the block end and reset every block, so it opened for at most one block. It is now an absolute range set by each primary grain, `repeats × subdivision` long.
- **Output no longer depends on block size.** Three causes:
  - **RNG:** `grainRng` and the scheduler's RNG were clock-seeded `juce::Random`.
  - **Standalone tempo:** the 120 BPM clock (no playhead) summed ppq one block at a time, which rounds differently for each block split and can move a trigger by a sample.
  - **Spatial control rate:** trajectory SH updates used `i % 16` on the block index, not the absolute sample.
- **Read precision on the 12 s ring.** `DelayBuffer::readSample`, the grain read position and the grain delay offset were floats. Past 2^19 (48 kHz) to 2^21 (192 kHz), a float keeps only 1/16 to 1/4 of a sample, so the Lagrange fraction was quantised. They are now doubles.

### Changed
- **Counter-based seeded RNG** (`dsp/CounterRng.h`, splitmix64). Every draw is `hash(seed, stream, event, slot)`:
  - grains are keyed by spawn index, with a fixed slot for each of size, amp, spread, pitch, pan, reverse, azimuth, elevation and freeze detune;
  - Free-mode ticks are keyed by tick index;
  - sync triggers are keyed by division index.
  
  A conditional draw (Size Random at 0, spatial off, not frozen) no longer shifts any other value.
- The seed is per instance (random at creation) and saved with the session as `rngSeed`, so a reloaded session renders the same grains. Pre-3.1.0 sessions keep the instance's fresh seed. The event counters restart on `reset()`/`prepareToPlay()`.

No parameter change. Sessions load unchanged. Random output differs from v3.0.5, which was never reproducible.

### Testing
- New gate `tests/blocksize-null` (target `O-GrainScatter-blocksize-null`, `OUARICON_BUILD_TESTS=ON`). The same seeded state and stimulus, rendered at 32-sample and variable (1–512) blocks, matches the 512-sample render **bit for bit** (residual −400 dBFS, gate ≤ −120) in every path tested:
  - Free;
  - Sync 1/16 with Repeats 4, swing, Euclid 5/8 and Stutter Gate;
  - Trajectory/Random spatial with Doppler;
  - Freeze engaged mid-render;
  - +24 st with 500 ms grains.
- Controls: a different seed differs by −3.3 dBFS peak. The seed survives a state round-trip (512 vs restored-at-32 is bit-exact). The v3.0.5 scheduler, compiled from the backup, emits 48 of 186 expected grains. Ring read at 192 kHz with the write head at 2 303 991: worst error −141 dB against the analytic sine.

## [3.0.5] — 2026-10-26

### Fixed
- **Voice steals clicked.** With all 64 grains sounding, the oldest grain was cut off mid-window to make room, a step of up to its full level. The stolen grain now moves to one of 8 spare slots and fades out over 5 ms while the new grain starts. Renders with no steals are bit-identical to v3.0.4: the new grain takes the same slot as before, and the fade gain stays exactly 1.

## [3.0.4] — 2026-10-26

### Fixed
- **Freeze replayed the same few milliseconds in every grain.** Each frozen grain started at index 0 of the snapshot (or the grain's own length when reversed), so Scan and Spread had no effect while frozen and every grain repeated the same samples. The result was a buzz at the spawn rate, not a held texture. Frozen grains now read the snapshot from where the live tap would have been: a virtual write head keeps advancing through the held audio, and each grain latches `head − position` at spawn. Scan and Spread work again, and the hold is the captured loop repeating.
- **Release clicked.** The "release crossfade" was a 5 ms timer, not a gain ramp (review item IN-01). When it expired, grains still playing from the snapshot jumped to the live delay line partway through the grain. Each grain now latches its snapshot at spawn and finishes on it. Two snapshot slots let a fresh freeze capture without overwriting audio that grains from the previous hold are still reading.
- **Loop seam was a splice.** The point where the newest captured sample meets the oldest now gets an equal-power crossfade of 10 ms, or a quarter of the capture if that is shorter.
- **Freeze right after load looped through silence.** The capture always took 2 s, including parts of the delay line that had never been written. The loop length is now latched to how much audio has actually been written, capped at 2 s.

### Changed
- Frozen grains get a random detune of ±6 cents each, so a long hold drifts slowly instead of locking into fixed beating. Live (unfrozen) grains are unchanged: the detune is drawn last and only while frozen, so the live path's random sequence is identical to v3.0.3.

No parameter, preset or state change.

## [3.0.3] — 2026-10-19

### Fixed
- **Clicking another dial now applies a typed value.** With a value-entry field open, pressing on a different dial left the field open and its readout frozen. The press now applies the typed value and closes the field before the new drag starts.

## [3.0.2] — 2026-10-19

### Changed

- **Double-click a dial to type a value** — Enter or click away applies, Esc cancels. **Alt/Option-click resets** a control to its default (this was double-click). Double-clicking the readout under a dial opens the same field.
- Typed values use the readout's own units (ms, %, dB, st, ct, degrees, counts) and go through the parameter's live range and skew. On the level dials `-inf` (or `−∞`) means the −60 dB floor.

## [3.0.1] - 2026-10-15

### Fixed

- **Custom collection: Apply rejected typed cents.** The text field used the
  Scala-file rule, where a bare integer is a ratio (`2` = 2/1). So
  `100 200 … 1200` parsed as the ratios 100/1 … 1200/1, the period 1200/1
  (~12 274 ct) failed the 4-octave check, and Apply was refused every time.
  The field's own default text failed the same way, so reopening the editor
  showed `100 200 …` again. In typed input a bare integer is now cents, and a
  ratio needs a `/`. `.scl` files keep the Scala rule: their bare integers are
  rewritten as `a/1` before they reach the text form.

### Added

- Apply confirms success with "Applied: N pitches per period." under the field.

### Testing

- Parser unit test compiled against juce_core (text and `.scl` paths, including
  the `.scl` → text round-trip the processor performs): 9/9 pass. The same test
  on v3.0.0 fails 3 (`100 200 … 1200`, `100 250 1200`, `146.3 292.6 1902`, all
  `badPeriod`), which confirms the root cause.

## [3.0.0] - 2026-10-15

Pitch collections and separate Dry / Wet levels. MAJOR: `dry_wet` is removed
(two new level parameters replace it) and the `scale` choice list grows from 5
to 32 entries, so host automation lanes on those two parameters need redrawing.
Saved sessions migrate automatically (see Migration).

### Breaking Changes

- **`dry_wet` removed** → `dry_level` + `wet_level` (−60 dB = −∞ to +6 dB,
  default 0 dB each). Automation drawn on Dry/Wet does not carry over.
- **`scale` grows 5 → 32 choices.** Indices 0–4 (Chromatic … Whole Tone) are
  unchanged, so a saved session still names the same scale, but an automation
  lane's normalised values now land on different entries.
- **Scan 0 % is now live, not a 2 s echo.** Measured on v2.9.0: with Scan at 0 %
  (the default) the wet signal arrived 2.000 s late, the same as Scan 100 %,
  while Scan 1 % arrived after 0.020 s. A grain's delay tap of 0 landed on the
  write head and wrapped to the oldest slot of the 2 s ring. Scan now sweeps
  ~0 → 2 s continuously. Sessions saved at Scan 0 lose the echo; set Scan to
  100 % to get it back.
- **Pitch semantics changed.** Ladder modes walk the collection's real pitches
  across the whole Range instead of scaling each degree by Pitch Rnd and
  truncating it (v2 at 50 % turned Major's 7th, 11 st, into 5 st). Random
  picks evenly among the active pitches in the span. Root Note now builds the
  collection on that tonic, with the source heard as C.

### Added

- **32 pitch collections** in a cents-based engine (`dsp/PitchEngine.h`,
  replacing the 12-TET `ScaleQuantizer`):
  - the five v2 scales; Free (unquantized, continuous);
  - Dorian, Phrygian, Lydian, Mixolydian, Locrian, Harmonic Minor, Melodic Minor,
    Minor Pentatonic, Blues, Octatonic, Augmented, Fifths + Octaves, Octaves;
  - 19-, 22-, 24-, 31- and 53-EDO;
  - 5-limit and 7-limit just intonation, Harmonics 8–16;
  - Bohlen-Pierce (13 steps of the 3:1 tritave), Slendro, Pelog, Maqam Rast;
  - **Custom**: type pitches as cents or ratios (Scala convention, last pitch =
    period, up to 64 per period) or load a Scala `.scl` file.
- **Per-degree toggles.** A chip row shows every degree of the selected
  collection (note names where a degree is a 12-TET semitone, cents otherwise);
  click to switch one off. All / None / Invert tools. Masks are per collection
  and saved with the session (not automatable).
- **Range** (0–24 st, default 12 = the v2 span), **Transpose** (±24 st, whole
  semitones), **Fine** (±100 ct) and **Direction** (Up + Down / Up / Down).
  Transpose and Fine apply to every grain even at Pitch Rnd 0. Total pitch is
  clamped to ±36 st.
- `pitchStateChanged` editor event + `getPitchState` / `setPitchMask` /
  `setCustomPitches` / `loadScalaFile` native functions.

### Fixed

- **Grains reading ahead of the write head.** A forward grain above unity rate,
  every reverse grain, and a negative Spread offset all reached a delay tap
  below 0 and wrapped to the oldest audio in the ring. Each grain is now
  pre-rolled at spawn so its tap stays ≥ 4 samples (the Lagrange kernel's
  look-ahead plus one): forward `(rate − 1) · length`, reverse `length − 1`.
  The delay buffer grows 2 → 12 s to hold that pre-roll; Scan still sweeps the
  newest 2 s.

### Changed

- Pitch & Scale group: Pitch Rnd, Range, Transpose, Fine, Pan Rnd; Collection,
  Root Note, Pitch Mode, Direction; the degree chips. The Pitch Rnd hint now
  sits over the dimmed chips instead of taking its own row, so the group fits
  the Core group's height and the viz keeps its size (245 → 244 px).
- Core group: the Dry/Wet knob becomes Dry and Wet knobs.

### Migration

- `setStateInformation` converts `dry_wet` m into `dry_level = 20·log10(1 − m)`
  and `wet_level = 20·log10(m)` (floored at −60). At 50 % that is −6.02 dB
  each, gain 0.5 each — the same output.
- Every parameter missing from the saved tree is written at its default before
  `replaceState()`, which otherwise keeps a missing parameter at the live
  instance's value.
- A v2 session has no masks or custom table: every degree is on and Custom is
  12-EDO.

### Testing

- Per-grain pitch on the built VST3 in pedalboard (isolated grains, 440 Hz):
  Transpose/Fine 0, +700, −450, +1175, ±2400 ct all within 0.3 ct.
  24-EDO grains land on multiples of 50 ct; Bohlen-Pierce on k·146.3 ct;
  Ladder Up on Major walks 0 → 1200 ct and wraps.
- Scan delay: 0 % → 0.0001 s, 1 % → 0.020 s, 50 % → 1.000 s, 100 % → 2.000 s.
- `check-i18n`, `i18n-fr-lint`, `i18n-zh-lint`, `check-ui-labels` (with a new
  custom-editor state), `tests/ui_tip_render_check.js`: pass. 36 new zh-Hans
  rows are at `reviewed: 'mt'`.

## [2.9.0] - 2026-09-26

UI pass R4/R5 from review 260924-nho: palette tokens, AA text inks, the 9px
floor and the bundled EB Garamond face. MINOR: a visible colour, face and size
change. No DSP, parameter, range or state-format change. Knob interaction JS is
untouched, so keyboard/ARIA knobs (R7) remain open.

### Changed

- **Palette tokens (R4).** The sheet had 0 custom properties and 127 hex
  literals. It now has a `:root` block, and no hex literal remains outside it.
  rgba() washes keep their alpha inline. Canvas colours stay literal because a
  2D context cannot resolve `var()`; `TEXT_MUTED` and `CANVAS_FONT` in `app.js`
  mirror the tokens.
- **AA text inks from ouaricon-naturalist-001.** The ground is the body
  gradient, which `measure-ui --contrast` cannot sample (it flags it as
  background-image and scores against white). So the change was measured with an
  image-aware probe: text hidden, transitions off, 2× screenshot, and the worse
  of p5/p95 luminance under each glyph band.
  - Walnut text (#8B7355) moves to `--text-muted` #6A5641, the template's ink
    for text anywhere over this gradient. This covers the tagline, group labels,
    viz labels and both hints. Group labels went from 3.34–3.55 to 5.1+.
  - Viz labels lose their `opacity: 0.7` (2.28, zh 2.01 → 5.1+).
  - The lit hover-help switch fill moves from #6B8E4E to `--leaf-fill` #4E6839
    (cream text 3.52 → ~6.0).
  - The ⚙ glyph moves to `--ink` (4.05 → pass).
  - Canvas text (axis labels, grain count, Euclidean step count) drops the
    0.3/0.4-alpha walnut for solid #6A5641.
- **9px floor.** Knob and dropdown captions go from 8 to 9px at `line-height: 1`,
  so a two-line caption still fills exactly the 18px box it was pinned to. The
  pitch and spatial hints go from 8 to 9px at `line-height: 1`, which keeps the
  same 9px box in the zero-slack spatial row. Canvas axis labels go from 8 to 9px.
- **`+24st` axis label moved from baseline 14 to 26.** It sat under the
  `GRAIN SCATTER` caption. The old 0.3-alpha ink hid the clash, and the AA ink
  exposed it.
- **Bundled EB Garamond (R5).** `modules/ui/eb-garamond` is direct-embedded:
  4 SOURCES in the existing UI binary-data target and 4 `url ==` getResource
  branches. The stylesheet is linked before the page `<style>`. One
  `--font-serif` token replaces nine stacks, including the bare `'Garamond'`
  entries and v2.8.0's `'Times'` stack on `.toggle`. The Times vertical metrics
  are baked into the face, so that stack's 1.3 line-height pin still holds. CDP
  resolves every Latin run to EB Garamond and every Han run to PingFang SC in
  en, fr and zh-Hans.
- **`#stutter-gate-btn` re-pinned from 110 to 111px.** EB Garamond caps set
  "STUTTER GATE" at 110.50px, so English wrapped to two lines (+13px) and
  check-ui-labels [7] reported the wrapper moving.
- Fleuron marked `aria-hidden="true"`. It stays decorative at low opacity, as
  the template allows.

### Testing

- **Pixel probe, active text** (en/fr/zh-Hans): 12/41 below AA → **0/41**.
  Minimum ratio 2.28 (zh 2.01) → **5.14**.
- **`measure-ui --contrast`**: 36/79 below AA → 25/79. The remaining 25 are the
  24 inactive dimmed controls (the pitch gate and spatial Mode = Off at
  `opacity: 0.25`, which WCAG 1.4.3 exempts) plus the aria-hidden fleuron.
  Nodes under 9px: 36 → **0**.
- `check-ui-labels`: pass. `check-i18n`, `i18n-fr-lint` and `i18n-zh-lint`:
  pass.
- `ui_tip_render_check`: pass, 1054 checks. Its [6b] clamp plant grew from
  130 to 190 repeats. The narrower face shrank the old plant to 478px, so it
  fit above the cursor and stopped exercising the clamp.

## [2.8.0] - 2026-09-07

Simplified Chinese joins English and French (task 260907-ja8, wave 4f of the
zh-Hans rollout; O-GrainScatter is the wave's tracer). MINOR: a third language,
no parameter, range, type or state format changed.

### Added

- **Simplified Chinese across the whole interface** — 131 rows (39 hover-help
  titles, 39 hover-help bodies, 53 captions), covering every caption, every
  section heading, every hover-help body and every accessible name the table
  reaches. `简体中文` joins the language selector, written as numeric character
  references in the markup and exempted by its decoded form.
- **DISCLOSED QUALITY LEVEL: `reviewed: 'bt'`, not `'native'`.** Every row was
  machine-drafted, then read back through a BLIND reverse pass — the Chinese
  handed to an independent reader with the English withheld and the row ids
  blinded behind a per-batch salt — and each returned triple was read and either
  accepted with a written reason or re-authored. **No native Chinese speaker has
  read this file.** `reviewed: 'native'` stays open and `i18n-zh-lint` prints
  that disclosure on every run.
- ONE row was re-authored on the strength of the reverse read: the spatial
  smoothing body rendered "ambisonic" as a phrase meaning high-fidelity STEREO,
  and this page already talks about a stereo path elsewhere, so it read as a
  collision on the page rather than a translation preference. Re-read by a fresh
  reader on a different model with a fresh salt, which recovered the source.
- ONE reasoned glossary exemption, on the caption AND its tooltip title:
  **Dist LPF is DISTANCE low-pass on this page, not distortion.** The shared
  glossary's root renders the other sense of the abbreviation, and O-GrainScatter
  is the only site in the corpus that carries the term.
- **A CJK face is named on every stack the page declares**, before the trailing
  generic — a bare generic resolves against the document's language, so a tail
  written after it is never consulted. One declaration named only a family that
  is not installed, leaving a bare generic as its only survivor; it now names the
  face it was already resolving to, measured rather than assumed, so the English
  and French arms do not move.
- **Line boxes pinned as ratios** on the seven selectors that rendered at
  `line-height: normal` and grew under Han. Every ratio is derived from this
  page's own measured English content box, padding and border subtracted first,
  so it reproduces English exactly: **Han line boxes run roughly 30 % taller than
  Latin at the same size**, and left unpinned the sum shrank the visualisation
  area by 12 px and moved 140 elements. Geometry is now identical on all three
  languages, in both measured states.

### Changed

- **The settings hover-help no longer claims the panel holds the interface
  language alone.** That became false in v2.7.0, when the hover-help switch was
  added inside the same popover. The clause is deleted rather than widened to
  name two controls: a body that inventories a panel needs re-editing in every
  language every time the panel grows.
- **The language hover-help no longer names the selector's options.** It was true
  while the selector held two entries and this release adds a third. The selector
  lists the languages in their own endonyms, which is the one place the list
  cannot go stale.
- The plugin's own render gate now DERIVES its language list from the table
  rather than naming the languages it sweeps, and refuses to run rather than
  sweep a guessed list. Its height comparison asserts that another language
  measures DIFFERENT boxes from English rather than TALLER ones — Chinese
  measured 0 taller and 32 shorter, so the old direction was wrong for a correct
  page. 816 assertions before, 1054 now.

## [2.7.1] - 2026-09-03

The French rendering of the hover-help surface changes suite-wide (task
260903-ukp; O-Gain 1.3.3 was the tracer). PATCH: French strings and source
comments only — no parameter, range, type or state format changed.

### Changed

- **The French caption is now `Infobulles`** (feminine plural). The superseded
  rendering named the ACTION — help on hover; *infobulle* is the noun French
  DAW and OS interfaces use for the surface itself. The glossary root moved
  with it, ROOT-ONLY: `scripts/i18n-fr-glossary.js` now reads
  `'hover help': ['infobulles']` and
  `'toggle hover help': ['activer ou désactiver les infobulles']`, with the old
  rendering REMOVED rather than kept as an accepted alternate — so a plugin
  drifting back is a red G1 gate, not a silent pass.
- **Every sentence re-agreed from feminine singular to feminine plural**, not
  substituted: `cette …` → `ces infobulles`, `l’…` → `les infobulles`,
  `de l’…` → `des infobulles`, `toute l’…` → `toutes les infobulles`,
  `Une fois désactivée` → `Une fois désactivées`; the distributive `chaque …`
  → `chaque infobulle` is the one place the new term stays singular.
  Bare back-references that carried no occurrence of the old phrase — clauses
  reading *le réglage de l’aide*, *l’état de l’aide*, *son affichage ou non*,
  and the pronouns in *Lorsqu’elle est désactivée … la réactiver* — were
  rewritten too. A regex pass would have left every one of them pointing at an
  antecedent that no longer exists.
- Every changed body was read by the developer at a blocking checkpoint
  *before* it was written, so each ships `reviewed: true` legitimately and the
  repo-wide unreviewed-French TOTAL stays at 0.


## [2.7.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `ogs.tipsEnabled`.
- **`data-tip-always` on `#gear-btn` and on `#tips-toggle`, and on nothing
  else.** Those two controls are the ones that REACH and RESTORE the help
  layer, so they keep explaining themselves while it is off. `#lang-select`
  deliberately does not carry it: it is only reachable through the gear, which
  already explained itself on the way in.
- **Five i18n keys, four of them settled roots copied rather than authored.**
  `label.hoverHelp`, `ui.on`, `ui.off` and `aria.helpToggle` take the French
  glossary roots verbatim from `scripts/i18n-fr-glossary.js` — *Aide au survol
  / Marche / Arrêt / Activer ou désactiver l'aide au survol*. The fifth,
  `tip.tipsToggle`, is the tooltip's own title and body.

### Measured

- **The second row costs nothing.** With the popover forced open it occupies **y 39..94, 170 x 55 px — byte-identical in English and French** — inside a 900 x 800 frame. The switch face measures **42.00 x 16.00 px in both languages**.
- **The switch's face is a `min-width: 42px` floor, not a pinned width**, so a
  longer French face grows LEFTWARD into slack the popover already has. The row
  is `space-between` and the button is a `[data-i18n]` node, so nothing the
  geometry gate measures moves. `check-ui-labels` [7] reports **0 non-label
  elements displaced** between English and French, and the visible element set
  identical in both.
- Every declaration in `.settings-toggle` above the four switch-specific ones
  is **copied from this page's own language `<select>`** — its font stack, ink,
  plate, hairline and radius — so the two controls in the popover match by
  construction rather than by a second designer re-deciding them.

### Decided

- **Default is ON.** The previous version showed hover help unconditionally, so
  ON is the setting that leaves an existing user's plugin behaving exactly as
  it did. Default OFF would additionally have made `boot-all-uis --strict-tips`
  measure an empty tip surface and call it correct.

### Also driven

- **The renderer here is `js/app.js`, not the inline module the sibling plugins
  use**, so the `show()` gate, the `hideTip` publication and the guarded
  `initializeTipsToggle()` call all live in that file.
- `tests/ui_tip_render_check.js` [8] counts **three** chrome anchors, not two.


## [2.6.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout — the 38 hover-help entries and 49 captions drafted at v2.6.0 were read one by one against their English and against the suite glossary (`scripts/i18n-fr-glossary.js`), which did not exist when they were written.

### Changed
- **43 French entries revised** against the suite glossary and lint — 16 terminology, 21 typography, 5 grammar/register, 1 meaning, out of 87. The visible ones: **Dispersion → Étalement** for *Spread* and **Disp. az. / Disp. él. → Étal. az. / Étal. él.**, because the draft spent one French word on both *Spread* and *Scatter* while this page has a real Scatter (the spatial-mode option and the grain-canvas caption); **Inverse → Inversion** for *Reverse*; **Réinject. → Réinj.**, the glossary's listed abbreviation for *Feedback*; and typographic no-break spaces before `%` `:` `;` and between every number and its unit (*0 à 100 %*, *10 à 500 ms*, *20 kHz*), 51 in all.
- **Six sentences rewritten for French rather than for the lint.** `tip.scan` had dropped the control's own name (the English opens "Scan Position moves the read point"); `tip.doppler`'s "une sirène qui passe monte puis descend" was a garden path; `tip.spread`'s "Un peu estompe l'attaque" was a calque with no French subject; `tip.reverse` was missing a partitive ("mêlent grains"); `tip.syncMode` read "qu'une fois une division choisie"; `tip.feedback` carried a comma splice.
- **`<html lang>` now follows the language selector** (canon change, all plugins), so assistive technology reads the page in the language it is displayed in.
- **`Grain Size` keeps the caption `Taille`**, with the measurement recorded on the entry as a `termNote`: both glossary forms — *Taille de grain* (76.94 px) and *Taille grain* (62.28 px) — wrap to two 8.80 px lines inside an 18.00 px content box, leaving 0.20 px against `check-ui-labels` assertion 4 on a page whose Windows metrics are unmeasured.

### Fixed
- Nothing behavioural. No English copy, key, tip binding, exemption, selector, CSS rule, parameter, preset or DSP path was touched, and `reviewed: false` stays on all 87 entries — that flag records a native speaker, and none has read this file yet.

### Testing
- `i18n-fr-lint --plugin O-GrainScatter --strict`: **52 findings → 0, exit 0** (17 T3, 15 T4, 7 T5, 5 T7, 8 G1 closed). Two `termNote` exemptions, both on Grain Size, both carrying their measured px.
- **Scope control, not a diff read:** both revisions imported as ES modules and compared field by field — **0 `en` values changed**, `TIP_BINDINGS` / `I18N_EXEMPT` / `LANGUAGES` byte-identical, no key added or removed, no `reviewed` or `sameAsEn` flag moved. All 51 U+00A0 land inside a French `t:`/`b:` value; none in a key, a selector, a comment or a `termNote`.
- `check-ui-labels`: **output structurally identical to the pre-change run** — 78 PASS, 0 FAIL, `[7]` no non-label element moved in either, same `[8b]` decoration counts of 65 / 27 / 59, same 900 x 800 scroll extent in both languages. The five renamed captions were measured in this page's own node first: Étalement 53.83 px (0.19 *narrower* than Dispersion), Inversion 49.63, Réinj. 29.39, Étal. az. 41.30, Étal. él. 41.17, all in 62.00 px cells whose min-content driver is the longest word.
- `tests/ui_tip_render_check.js`: **796 / 796**, unchanged, 38 anchors driven in `en → fr → en` with both feature gates opened through the page's own listeners. Per-anchor tip heights read before and after: **2 of 38 moved** — `scan_position` 108.5 → 123.9 px (the restored control name) and `spatial_width` 108.5 → 93.2 px — tallest tip unchanged at 139.3 px in an 800 px frame, 19 flips and 0 clamp-edge contacts in both languages, and 0 English tip heights changed.
- `check-i18n`: ALL CHECKS PASS, canon v2, 38 tips bound. `boot-all-uis`: **43 / 43 clean, 0 warn, 0 failed, 0 DEAD bindings**, and O-GrainScatter contributes none of the 19 late bindings in the suite.
- `auval -v aufx OuGS OuDv`: **AU VALIDATION SUCCEEDED**. The installed bundle was verified to carry a changed French **value** (`t: 'Étalement'`) with `LC_ALL=C grep -a`, and its `Info.plist` was read for the new version — `strings` splits on the multi-byte `é`, and a re-embedded binary-data blob can ship beside a stale `Info.plist`.

## [2.6.0] - 2026-08-30

Hover-help, in both languages. Every one of the 36 parameters now carries a tooltip, as do the gear and the language selector — 38 entries, English and French, drafted for review. **And the copy is not the whole change: this page had no way to paint a tooltip at all**, so a renderer and a surface land in the same commit.

### Added
- **38 `I18N` hover-help entries in `js/i18n.js`**, one per on-page control plus two for the chrome, each with an English and a French `{t, b}`. `I18N` and `TIP_BINDINGS` were both empty through v2.5.0, which was that version's correct state; `check-i18n` assertion 2 reported it as "0 tip(s) bound". It now reports **38**.
- **`setupTooltips()` in `js/app.js` and the `.tooltip` rules + `#tooltip` surface in `index.html`.** `applyI18n()` writes `data-tip-title` and `data-tip` onto the bound anchors and stops there; the code that *reads* those attributes and paints a surface is per-plugin, and this page had none. Authoring the copy alone would have shipped **38 invisible strings past three green gates** — `check-i18n` only counts bindings, `check-ui-labels` has no tooltip awareness at all, and `boot-all-uis` counts `aria-label` and `title` and never `data-tip`. Ported from O-simpleFM's delegated renderer and styled in this plugin's own parchment palette.
- **`tests/ui_tip_render_check.js`** — the only gate in this repo that can see a *rendered* tooltip. 796 assertions at the shipping 900 x 800 frame, in `en → fr → en`.

### Changed
- **Tip titles are the PAGE's caption, not the parameter's name.** Twelve of the 36 differ, and six of those are abbreviations forced by the 62 px `.knob-container` cap (`Size Rnd`, `Traj Speed`, `Dist LPF`, and the French `Alé. haut.`, `Vit. traj.`, `PB dist.`). A 280 px tooltip has no such cap, so every abbreviated title's **body opens by naming the control in full** — the title matches what the user is looking at, the body says what it is.
- **All 36 units were recovered from the page's own formatters**, because all 36 rows of `.planning/params.tsv` carry an **empty `label` column**. `pctFormatter` (`app.js:249`) gives `%`, `grainSizeFormatter` (`:250`) and `spatialSmoothFormatter` (`:266`) give `ms`, `degreeFormatter` (`:259`) gives degrees, and `repeatsFormatter` (`:251`) gives a bare integer. Nothing was invented.
- **Option words stay English inside French sentences.** `Free`, `Hann`, `Scatter`, `Trajectory` and the rest are `AudioParameterChoice` options, exempt on the page under D-01 arm 1 so the host automation lane agrees character for character — a French sentence that renamed them would be an instruction the user cannot follow. The page already made this choice once, in `label.spatialHint` at v2.5.0.

### Fixed
- Nothing. No existing behaviour changed, and no parameter ID, range, type, default, preset or DSP path was touched.

### Testing
- `check-i18n --strict-v2`: canon v2, ALL CHECKS PASS; **`[2] 38 tip(s) bound`**, every key resolved, zero orphans, zero dangling. Repo-wide, 43 plugins, ALL CHECKS PASS.
- **`check-ui-labels` output is BYTE-IDENTICAL before and after this change** — same three states, same `moved=0`, same `[8b]` decoration counts of 65 / 27 / 59. That is the number that matters rather than the verdict: the O-Emulator defect was caught by an `[8b]` count moving 7 → 9 inside a *passing* run, and the focus latch below is why it does not move here.
- `boot-all-uis`: 43 / 43 clean, 0 warn, 0 failed. O-GrainScatter's row is unchanged — `text=75 aria=3 title=0 i18n=47` — confirming the hidden surface adds no text node, and repo-wide native `title=` stays at **0**.
- `tests/ui_tip_render_check.js`: **796 / 796**, 38 anchors driven in `en → fr → en`, every rendered title and body **byte-equal** to the table (not "contains"), every tip rect inside 900 x 800.
- **French grows 25 of the 36 tips** against the 280 px cap — `93 → 109` px on most, `109 → 139` on `pitch_random`, `pitch_mode`, `spatial_mode` and `spatial_smooth`. None shrank. So the French pass measures different boxes from the English one rather than repeating it.
- **19 of 36 tips place by FLIP and none needed the clamp**, on shipped copy — which is why the gate carries a *positive* control for the clamp as well as a negative one: a 616 px plant fits on neither side of the cursor, flips to `top = −180`, and is clamped to exactly **8.0 px**. Without the clamp-after-flip it would leave the frame (O-Bass's carried finding: one flip is not enough).
- **Negative control:** a 4800-character plant renders **280 x 1570.7 px in an 800 px frame** and assertion 4 reports the overflow; restored from the table and green again at the same anchor. Sized against *this* frame deliberately — O-Tremolo's habit-sized plant fitted inside a 400 px frame and reported nothing.
- **The focus latch, controlled both ways.** Deleting only `if (lastInputWasPointer) return;` — the declaration, the `pointerdown` write and the `keydown` clear all survive, so every static grep still matches — makes the gate report the gear's own tip **covering the settings popover by 3650 px²**, with the keyboard half still green. Deleting the gate's `activeElement.blur()` as well turns the suite back to **796 / 796**, which proves the blur line is load-bearing rather than tidying. Both plants restored from a namespaced scratchpad copy and checksum-verified — never `git checkout`, which is how this plugin lost a whole uncommitted edit in Stage K.
- All 38 French entries are machine drafts, every one `reviewed: false`. Scanned for a decimal point, an angle bracket and a missing space before `%`: zero of each.

### Notes
- **FOURTEEN OF THE 36 CONTROLS CANNOT BE HOVERED IN THE PLUGIN'S DEFAULT STATE, and that is shipped behaviour rather than a tooltip bug.** `setupPitchGate` (`app.js:326-345`) dims Scale, Root Note and Pitch Mode while `pitch_random` is below 0.01 — via `.dimmed`, which is `opacity: .25` **plus `pointer-events: none`** (`index.html:287`) — and `setupSpatialGate` (`:351-375`) sets `pointerEvents = 'none'` on the ten spatial knobs and the Trajectory dropdown while Spatial Mode is Off. Both are the defaults. The information is not lost: `#pitch-hint` and `#spatial-hint` are visible in exactly those states and say what to raise. The gating was **not changed to suit the tooltips** — the render gate drives the page out of both states through the page's own `valueChangedEvent` listeners, and pins the rest state as an assertion so a future change to either gate is visible.
- **`pointer-events: none` does not remove an element from the tab ring**, so the three gated `<select>`s still open their tip from the keyboard while being unreachable by mouse. Recorded, not fixed.
- **The 27 knobs are `<div>`s and are not focusable**, so the keyboard half of hover-help reaches the 7 dropdowns, the 2 toggles and the 2 chrome controls — 11 of 38. Adding `tabindex` would change the page's tab order, which is a feature decision rather than a Stage M side effect.
- **No hover-help on/off toggle**, matching the other 19 tooltip plugins in the suite; only O-Tapestop and O-Bitrot have one.
- No geometry pin was added, so no negative control is owed for one.


## [2.5.0] - 2026-08-29

The PAGE speaks French. Every visible caption on the UI is now localized, English or French, chosen from a gear popover and remembered with the session. No hover-help copy was authored — this plugin has none, and inventing it is a separate job.

### Added
- **`Source/ui/public/js/i18n.js`** — 49 keys: 47 visible captions and 2 accessible names. `I18N` and `TIP_BINDINGS` are both **empty**, which is this plugin's correct state: v2.4.4 carried zero `title=`, zero `aria-label`, zero `placeholder` and zero `data-tip` anywhere in `index.html`. `check-i18n` assertion 2 reports that as "0 tip(s) bound" rather than passing silently.
- **A settings popover carrying the language selector**, absolutely positioned in the header's empty right margin so that not one existing element moves to make room for it. Styled in this plugin's own visual system — Garamond, the `#8B7355` rules and `#FFF8DC` cream of the knobs, and the olive `rgba(107,142,78)` the freeze toggle already uses.
- **`getUiLanguage` / `setUiLanguage`** native functions and session persistence. `uiLanguage` is a `std::atomic<int>` behind a two-function string codec, written onto the APVTS state tree as a plain non-parameter property so it never appears in a DAW automation lane and no preset can change which language somebody reads their plugin in.
- **`plugins/O-GrainScatter/tests/i18n-states.json`** — two states for the label render gate, so all 47 captions are measured rather than 46.

### Changed
- **Canon v2 i18n runtime in `js/app.js`**, placed at the TOP of the module rather than the bottom. `init()` is the last statement in this file and is the page's only reader of `trLabel()`; a block placed below it would be a TDZ `ReferenceError` that takes the whole UI (`pattern_module_toplevel_init_tdz`). `initI18n()` is still called last, inside `init()`, guarded by try/catch so a throw there cannot cost the page a knob.
- **`#stutter-gate-btn { width: 110px }`** — a load-bearing pin, not decoration. The button is a `[data-i18n]` element so the geometry gate does not measure it, but its parent flex wrapper is not keyed and shrink-wraps it. 110 px is the English border box (109.95) rounded up: English moves 0.05 px, and the rectangle becomes language-invariant. Removed alone, the gate reports the wrapper at `dw=-16.9`.
- **`.settings-popover { width: 170px }`** — the same pin for the same reason, on the new panel: LANGUAGE → LANGUE *shrinks*, so an auto-width panel would contract in French. Removed alone, 2 elements move at `dx=14.6 dw=-14.6`.

### Testing
- `check-i18n --strict-v2`: 39 assertions, canon v2, ALL CHECKS PASS.
- `check-ui-labels`: all assertions pass in three states (default, settings popover open, pitch gate closed); 47 of 47 keyed elements measured; no page error; every resource served.
- `boot-all-uis`: 43 / 43 clean, 0 warn, 0 failed. O-GrainScatter reads text 72 → 75, aria 0 → 3, title 0 → 0, i18n 0 → 47.
- **English v2.4.4 → v2.5.0: zero of 192 elements moved** at the gate's 0.5 px tolerance. At 0.01 px exactly one moved, by `dw=+0.05`, and it is the stutter-gate wrapper taking the named pin. Nothing moved vertically; document scroll extent stays 900 x 800.
- **French vs English at v2.5.0: zero non-label elements moved.** Measured independently of the gate at 0.01 px over all 195 boxes — the only 32 rectangles that differ in the whole document are keyed captions whose own width changed, and 13 of those 32 are *narrower* in French.
- **The page holds still**: zero of 193 elements differ between a 180 ms and a 1.7 s settle, in either language, before or after the change, despite two `requestAnimationFrame` canvas loops. Canvas drawing does not enter `getBoundingClientRect`.
- Three geometry cliffs found by planting, each caught by an assertion blind to the other two: a caption overflowing its knob box into a neighbour (74 px, caught by `[8b]` only), one overflowing the 900 px frame from the last spatial knob (116 px, caught by `[5][6][8][8b]`), and one wrapping to a third line out of the fixed 18 px caption box (caught by `[4]` only, with zero `[7]`).
- Every pin was reverted alone and confirmed to re-break the gate. The one guard whose control passes — `.settings-label { white-space: nowrap }` — is labelled a guard, not claimed as a fix.
- No parameter IDs, ranges, types, defaults, presets or DSP behaviour changed.

### Notes
- **The 62 px knob container does NOT grow.** `.knob-container` carries an explicit `width: 62px`, so its automatic minimum size is clamped to that specified size and a caption wider than 62 px simply overflows it symmetrically, pushing nothing. This page therefore has only two shrink-wrapping push sites — the stutter-gate wrapper and the settings popover — and both are pinned.
- **`GrainScatterViz.draw()` paints `"<n> grains"` with `ctx.fillText`, and it stays English.** A 2D-context string is not a DOM node: the canon sweep cannot reach it and neither gate can see it. Recorded as a reasoned `I18N_EXEMPT` entry, matching O-Orbit, O-MultiBandCompressor and O-simpleSampler. Carried to Stage M.
- All 49 French strings are machine drafts, every entry `reviewed: false`. No native speaker has read them.

## [2.4.4] - 2026-08-19

UI layout fix. The Spatial Audio section was clipped by the bottom edge of the editor window; the control grid is now content-sized and the window is 50 px shorter.

### Fixed
- **Spatial Audio section clipped off the bottom of the editor:** at the 900 x 850 editor size the page content ran to y=884.5, so the last ~34 px — the bottom row of Spatial Audio knob readouts and the "Set Mode to Scatter or Trajectory to enable" hint — were cut off by `body { overflow: hidden }` and unreachable. *Root cause:* `.controls-area` used `grid-template-rows: 1fr 1fr`, which forces the second row (Beat Sync / Euclidean Rhythm) to match the height of the first (Core Engine / Pitch & Scale). Row 2 needs only 111 px of content but was rendered at 199.5 px, and that 88.5 px of dead space pushed Spatial Audio past the window. Changed to `grid-template-rows: auto auto` so each row sizes to its own content — Beat Sync and Euclidean Rhythm are now 112 px (56 % of the top row) and the dead space is gone.

### Changed
- **Editor height 850 -> 800 px** (`PluginEditor.cpp`): with the grid fix the natural content height is 797 px, so the window was shortened by 50 px rather than leaving the reclaimed space empty. Width is unchanged at 900 px. Measured content stack at 900x800: header 42 + viz 243 + freeze 33.5 + fleuron 16.5 + controls 331.5 + spatial 133.5 = 800 exactly, nothing clipped.
- **`.plugin-container` sizes from the editor instead of hard-coding 900x850** — now `width: 100%; height: 100%`, so `setSize()` in `PluginEditor.cpp` is the single source of truth for the window size and the CSS can't drift from it.
- **`.viz-area` absorbs the layout slack** — the fixed `height: 240px` became `flex: 1 1 auto; min-height: 200px`. The visualizations render at 243 px at the current size, and because the viz is the only flex-grow item, any font-metric difference between WKWebView/WebView2 and the layout-test engine is taken out of the viz height rather than clipping the bottom of the page.

### Testing
- Layout measured headlessly (Chromium, 900x800) against the shipped `index.html`: Spatial Audio bottom edge = 800.0, spatial hint bottom = 787.0, last knob readout bottom = 775.5 — zero overflow.
- Negative control: re-injecting the old `grid-template-rows: 1fr 1fr` at the new 800 px height puts the Spatial Audio bottom back at 884.5 (84.5 px clipped), confirming the grid change is what fixes the layout.
- No DSP, parameter, preset, or state changes — CSS and one `setSize()` call only.

### Notes
- This fix was authored as "2.4.3" on a branch cut before the 2.4.3 licensing release shipped; renumbered to 2.4.4 at merge time since 2.4.3 was already published.

## [2.4.3] - 2026-08-19

Licensing release — no audible or behavioral change.

### Changed
- Added AGPL-3.0 license notice headers to all Ouaricon-authored source files (repo relicensed to AGPL-3.0 on 2026-08-01; JUCE used under AGPLv3).

## [2.4.2] - 2026-07-09

Info-finding cleanup sweep (CODE_REVIEW.md v2.4.0 review, IN-* items — the 2 critical + 12 warning findings were resolved in v2.4.1). No audible or behavioral change; dead-code removal, a per-block micro-optimization, and defensive state resets.

### Changed
- **IN-01 — dead `FreezeManager::getCrossfadeGain()` removed:** the method was never called (the freeze crossfade is implicit — grains switch source at spawn and `advanceCrossfade` delays `active=false` by ~5 ms). Removed it and the now-write-only `crossfadeDirection` member. No behavior change.
- **IN-02 — dead `TempoTracker::lastPpq` removed:** written in both update branches, never read.
- **IN-05 — grain voices now cleared in `prepareToPlay`:** added `GrainPool::clearVoices()` and call it from `prepareToPlay` (previously voices were deactivated only in `reset()`), so a sample-rate/block-size change without a host `reset()` can't leave stale grains active. `reset()` now shares the same `clearVoices()` path.
- **IN-09 — HOA write pointers cached per block:** the spatial inner loop stored via `hoaBus.setSample(ch, i, …)` (a `getWritePointer` + bounds check per channel per sample); now caches the 16 write pointers once per block and indexes directly.
- **IN-10 — distance split-semantics documented:** added a comment noting grain gain uses the spawn-frozen `v.distance` while the distance-LPF uses the live per-block value (intended: per-grain gain snapshot, continuous filter tone).
- **IN-11 — `reset()` now resets `TempoTracker`:** `reset()` re-`prepare()`s the tempo tracker so the standalone `manualPpq` counter doesn't keep advancing across a transport stop/seek (completeness gap in "clear all DSP state").
- **IN-14 — dead `.dimmed-spatial` CSS rule removed:** the spatial gate dims via inline `style.opacity`/`pointerEvents`, never applying this class.
- **IN-15 — `timerCallback` early-returns when hidden:** the 30 Hz viz JSON (String allocations) was built every tick even when the WebView wasn't showing (the emit was already visibility-gated). Now skips construction entirely when `!webView->isShowing()`.

### Reviewed — no change needed
- **IN-03** (Euclidean generator is a rotation of canonical Bjorklund) — valid maximally-even pattern, not a bug.
- **IN-04** (`isEvenSubdiv` misnamed) — already resolved by the WR-02 scheduler rewrite (now `isOffBeat`).
- **IN-06** (harden `repeatIntervalSamples` divide) — already done in v2.4.1 alongside WR-09 (`jmax(1.0, bpm)`).
- **IN-07** (repeat grains re-trigger at future subdivisions) — intended stutter/repeat-burst behavior (a shipped core feature); left as-is.
- **IN-08** (grain envelope never reaches phase 1.0) — ~5e-7 error at typical grain lengths; changing the phase formula would alter the grain sound for no audible benefit.
- **IN-12** (`releaseResources()` empty) — acceptable; JUCE re-`prepareToPlay`s before reuse.
- **IN-13** (Doppler uses smoothed SH `current[1]` as previous-azimuth proxy) — documented heuristic, not a defect.

## [2.4.1] - 2026-07-08

Code-review resolution pass (CODE_REVIEW.md, v2.4.0 deep review). All 2 critical + 12 warning findings fixed.

### Fixed
- **CR-01 — dead "Scan" knob:** `scan_position` (the v2.4.0 flagship control) had a param, DSP, DOM knob, and JS binding but **no editor relay/attachment**, so it was uncontrollable from the UI *and* host automation. Added the `WebSliderRelay` + `.withOptionsFrom` + `WebSliderParameterAttachment` triplet. *Root cause:* the relay/attachment pair was never added when the param was introduced.
- **CR-02 — RT reallocation in `reset()`:** `reset()` called `delayBuffer.prepare()` and `freezeManager.prepare()`, which `setSize()` the 2 s delay + freeze buffers (~6 MB of free/alloc) on a thread hosts may run in real time. Added alloc-free `clear()` methods (zero the already-sized buffers, no `setSize`) and call those instead.
- **WR-01 — spatial-mode feedback was a block-held constant:** in Scatter/Trajectory mode `feedbackL/R` were updated only in the post-decode loop, so every input sample of a block was fed the *same*, one-block-late feedback value → DC offset + block-rate buzz. Feedback is now derived **per-sample inside the main loop** from the HOA omni (W) channel — a true per-sample recursion. (Spatial feedback is now mono; the grains re-spatialize it on the next pass.)
- **WR-02 — swing dropped every off-beat + desynced Euclidean:** the swing gate reused the straight-boundary crossing and could only *reject*, so swung (odd) subdivisions were never spawned, and `euclideanStep` advanced only past the gate → pattern drift. Rewrote the scheduler to detect each division's **own** trigger time (straight for on-beats, swing-offset for off-beats) and advance the Euclidean step once per division, in order → phase-locked. Straight-grid behaviour (swing = 50 %) is bit-identical to before.
- **WR-03 — freeze-engage click/xrun:** `engage()` copied up to ~176 k samples element-by-element (`getSample`/`setSample` + modulo) on the audio thread. Replaced with two contiguous `AudioBuffer::copyFrom` memcpys spanning the ring wrap.
- **WR-04 — `spawnRequests` could exceed its `reserve(128)`** on large/offline blocks → audio-thread realloc. Added a hard `kMaxSpawnsPerBlock = 128` cap at every push site (free + sync + repeats).
- **WR-05 — no NaN/Inf guard on recursive state:** a single non-finite sample could latch `feedbackL/R` or `distanceLpfState` to NaN → permanent silence. Added `isfinite` flush-to-zero on both feedback paths and both LPF states.
- **WR-06 — per-sample SH trig in Trajectory mode:** `encodeSH16` (16-coeff trig) ran for every active voice every sample. It now updates the SH *target* only at a 16-sample control rate; the existing one-pole SH smoother interpolates between updates (trajectory position + Doppler stay per-sample, unchanged).
- **WR-07 — no block-size clamp:** `hoaBus`/`binaural` buffers are sized to `samplesPerBlock`; added a `jassert` + defensive `jmin` clamp on `numSamples` so an over-sized host block can't write past the allocations (the class v2.0.2 fixed).
- **WR-08 — distance-LPF zipper:** the cutoff coefficient was recomputed per block with no smoothing → stepping on Distance / Distance-LPF automation. Wrapped it in a per-sample `SmoothedValue`.
- **WR-09 — `bpm <= 0` silently stopped Sync scheduling:** `TempoTracker` took a host-reported non-positive BPM verbatim (ppqPerSample = 0 → no subdivision crossings). It now falls through to the 120 BPM fallback; the repeat-interval divide is additionally hardened with `jmax(1.0, bpm)` (IN-06).
- **WR-10 — `spatial_smooth` reset default wrong:** the hardcoded JS normalized default `0.1` ignored the 0.4 skew (reset snapped to ≈1.6 ms instead of 5 ms). Now sourced from C++ (see WR-11).
- **WR-11 — JS re-implemented C++ ranges/skew:** knob readouts and reset defaults duplicated each `NormalisableRange` (incl. skew) in JS — a latent drift class (WR-10 was the first crack). Readouts now use `state.getScaledValue()` and double-click resets pull skew-correct defaults from a new `getParameterDefaults` native function. No hand-coded ranges/defaults remain.
- **WR-12 — `CMakeLists` version drift:** `VERSION` was pinned at `2.1.0` (three minors behind the shipped 2.4.0 features) → binaries reported the wrong version. Bumped to `2.4.1`.

## [2.4.0] - 2026-03-09

### Added
- **Grain scan position** (`scan_position` 0-100%): sets the base grain read position in the delay buffer, mapping 0% (write head / most recent audio) to 100% (2 seconds back). Replaces the previous fixed `basePosition = grainSizeSamples` with user-controllable buffer scanning
- Spread parameter now scatters grains around the scan position instead of around `grainSizeSamples`
- In freeze mode, the full 2-second delay buffer is captured so scan position can sweep through the entire frozen buffer
- "Scan" knob in Core Engine UI group (between Density and Spread)

## [2.3.0] - 2026-03-08

### Added
- **Euclidean rotation** (`euclidean_rotation` 0-15): rotates the Euclidean pattern by reading `pattern[(step + rotation) % steps]`, shifting which pulses land on which subdivisions without regenerating the pattern
- **Swing** (`euclidean_swing` 50-75%): offsets even-numbered (off-beat) subdivision boundaries forward in time — 50% = straight, 75% = maximum shuffle
- "Rotation" and "Swing" knobs in Euclidean Rhythm UI group
- Euclidean circle visualization now reflects rotation offset: dots show the rotated pattern readout, dashed line indicates rotation origin, center label shows `r{N}` when rotation > 0

## [2.2.0] - 2026-03-08

### Added
- **Grain size randomization** (`size_random` 0-100%): each grain's duration is varied by `grainSize * (1.0 + random * sizeRandom)`, creating more organic, less mechanical grain textures
- **Per-grain amplitude randomization** (`amp_random` 0-100%): each grain's amplitude is scaled by `1.0 - random * ampRandom`, adding natural dynamic variation to the grain cloud
- Two new knobs ("Size Rnd", "Amp Rnd") in the Core Engine UI group
- Both parameters default to 0% (no change to existing behavior)

## [2.1.0] - 2026-03-08

### Added
- Grain envelope shape selection: new `grain_shape` parameter with 6 window types
  - **Hann** (default): smooth cosine bell — classic granular sound, zero at edges
  - **Triangle**: linear attack/decay — brighter, more percussive than Hann
  - **Trapezoid**: flat sustain (20-80%) with linear ramps — preserves transients
  - **Tukey** (α=0.5): cosine taper first/last 25%, flat middle — hybrid of Hann and rectangular
  - **Blackman**: narrower main lobe than Hann — reduced spectral leakage, darker tone
  - **Exp Decay**: exponential falloff — plucked/percussive character with sharp attack
- UI dropdown in Core Engine group for shape selection
- Visualization reflects selected envelope shape in real-time

## [2.0.5] - 2026-03-08

### Changed
- Removed dead code: `lastSubdivIndex` (GrainScheduler), `ppqJumped`/`didPpqJump` (TempoTracker), `scratchL`/`scratchR` (BinauralDecoder), `getActiveCount` (GrainPool), duplicate `probabilityFormatter` (app.js)
- Extracted shared `lagrangeInterpolate()` function (LagrangeInterpolation.h) used by DelayBuffer and FreezeManager — eliminates duplicated 3rd-order Lagrange interpolation code
- Consolidated duplicate degree formatters in app.js into `degreeFormatter(range, offset)` factory
- Extracted shared `resizeCanvas()` function for GrainScatterViz and EuclideanCircleViz — eliminates duplicated DPR-aware canvas sizing code
- Moved `setSpatialSmoothTime()` call from inside per-sample loop to once-per-block before the loop
- Named magic feedback constants: `kFeedbackDrive` (3.0), `kTanhCompensation` (1.00497), `kStabilityMargin` (0.95)
- Named distance attenuation constant: `kDistanceScale` (3.0) in GrainPool spatial processing
- Extracted `numHoaChannels` and `numChannels` local variables for HOA bus size expressions in BinauralDecoder and PluginProcessor

## [2.0.4] - 2026-03-08

### Fixed
- Thread safety: replaced visualization double-buffer with lock-free triple buffer to prevent torn reads when audio thread publishes faster than GUI consumes
- Thread safety: made `cachedEuclideanSteps`/`cachedEuclideanPulses` `std::atomic<int>` and moved euclidean pattern + step data into `GrainVizSnapshot` — GUI no longer holds a direct reference to audio-thread-owned `euclideanPattern` array
- Added `reset()` override to clear all DSP state (grain voices, delay buffer, feedback, freeze, scheduler, distance LPF, HOA bus) on transport stop/seek/loop — prevents stale audio artifacts after DAW transport jumps
- Root cause: double-buffer allowed audio thread to overwrite the slot GUI was reading mid-frame; euclidean data was exposed via raw `const&` across threads with no synchronization

## [2.0.3] - 2026-03-08

### Fixed
- Zipper noise on feedback/dry-wet automation in spatial mode: post-processing loop was reading raw `feedbackParam->load()` and `dryWetParam->load()` per-sample instead of using `feedbackSmoothed`/`dryWetSmoothed` SmoothedValue instances
- Root cause: stereo path correctly used SmoothedValues, but spatial post-processing loop bypassed them entirely
- Removed redundant SmoothedValue advancement in per-sample spatial branch; values now consumed in the post-processing loop where they're actually needed

## [2.0.2] - 2026-03-08

### Fixed
- Critical stack buffer overflow: replaced stack-allocated `binauralL/R[2048]` arrays with heap-allocated member buffers sized to actual `samplesPerBlock`
- Incorrect `hoaBus` and `binauralDecoder` sizing: was using `sampleRate * 0.02 + 1024` (arbitrary formula), now uses `samplesPerBlock` from host
- Root cause: `prepareToPlay` ignored its `samplesPerBlock` parameter entirely

## [2.0.1] - 2026-02-09

### Fixed
- Density parameter now uses exponential curve for perceptible control across full knob range
- Previously: 75% of knob range only varied from ~1 to ~4 grains/sec (linear interval mapping)
- Now: 50% knob = ~10 grains/sec, smooth exponential scaling from ~1/sec to ~100/sec

## [1.0.1] - 2026-02-07

### Improved
- Scale, Root Note, and Pitch Mode dropdowns now dim when Pitch Random is at 0%, with hint text "Increase Pitch Rnd to activate" — clarifies that pitch randomization must be active for scale controls to have effect

## [1.0.0] - 2026-02-07

### Added
- Granular scatter engine with 64-voice polyphonic grain pool
- Delay buffer with Lagrange 3rd-order interpolation for smooth pitched reads
- Free mode: density-controlled grain spawning (10ms to 1000ms intervals)
- Beat sync mode: 6 subdivision options (1/4, 1/8, 1/16, 1/32, 1/8T, 1/16T)
- Euclidean rhythm patterns for gating grain triggers (2-16 steps, 1-16 pulses)
- Repeat burst system (1-16 repeats per trigger) with stutter gate
- Freeze: capture and loop audio with 5ms crossfade on engage and release
- 5 musical scales: Chromatic, Major, Minor, Pentatonic, Whole Tone
- 4 pitch modes: Random, Ladder Up, Ladder Down, Pendulum
- Scale quantizer with root note selection (C through B)
- Spread control for grain position scatter
- Pan randomization with equal-power panning law
- Reverse grain probability
- Feedback with soft-clipping (tanh) to prevent runaway
- Smoothed dry/wet mix crossfade
- Output soft-clipping to prevent digital clipping with many overlapping grains
- Standalone tempo tracker with 120 BPM fallback and DAW loop detection
- WebView UI with vintage Naturalist aesthetic (Garamond serif, parchment palette)
- Real-time grain scatter visualization (Canvas 2D, position vs pitch)
- Euclidean circle visualizer with polygon overlay and step indicator
- Freeze glow animation on toggle button
- Double-click knob reset to default values
- 18 automatable parameters across 4 groups (Core, Sync, Spread, Euclidean)
- Cross-platform WebView2 support with static linking for Windows
- State save/restore via XML serialization
