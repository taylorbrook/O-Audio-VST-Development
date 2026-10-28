# O-GrainScatter Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 3.4.0
- **Type:** Audio Effect (Granular Stutter Engine)

## Lifecycle Timeline

- **2026-10-28 (v3.4.0):** Grain Skip (`grain_skip` 0–100 %) + Shape Tilt (`shape_tilt` ±100 %), research 2026-update #6 (Tempera 2.3). Skip: per-grain counter-RNG slot `SlotSkip`, drawn only when > 0, so the counter is consumed and the grains that play are unchanged. Tilt: piecewise-linear phase warp moves the window peak, with a 3 ms floor on each side and the tilt latched per grain; the moments of symmetric shapes are unchanged. Stutter Gate opens only when the trigger's primary grain plays (skipped primary = dry stays open). Gate `tests/skip-tilt` (+ `-v330` twin from d920bedd) 31/31: defaults bit-exact on 7 paths + v3.3.0 session, step ≤ 0.022 at ±100 % (bound 0.025; 2 ms floor control 0.033). UI: Tilt beside Shape, Skip beside Probability, left grid column +20 px.
- **2026-10-27 (v3.3.0):** Band-limited reader (grain-engine 1.1.0 BandlimitedReader): pitched-up grains alias ≤ −60 dB (gate `tests/alias-gate`).
- **2026-10-27 (v3.2.0):** Wet `tanh` (stereo + post-binaural) replaced by closed-form live-N density compensation: `P = N·μ₂ + ρN(N−1)μ₁²`, 100 ms one-pole on P, `g = 1/√max(1,P)`. ρ ← Spread (0 at 25 %), the read-floor pinned fraction, pitch span and reverse. Feedback tanh kept. Freeze scatter ≥ ±1 grain length. Gate `tests/density-gain` (+ `-baseline` twin on the v3.1.0 backup): level −1.2…+0.6 dB across N 4–50 and coherent/incoherent/pinned (v3.1.0: +0.7…+18 dB). Spatial-mode level not gated.
- **2026-10-27 (v3.1.0):** Deterministic, block-size-independent engine. Counter-based seeded RNG (`dsp/CounterRng.h`; seed saved as `rngSeed`). Sync repeats past the block end carry over: they used to be dropped, so at 512 only primaries played. The stutter gate spans blocks. The standalone tempo clock is integer-sample, the spatial control rate runs on the absolute clock, and ring/grain read positions are double. Gate `tests/blocksize-null`: 32/512/variable bit-exact in free/sync/spatial/freeze/pitch.
- **2026-10-26 (v3.0.5):** Fade-on-steal. A stolen grain is copied into one of 8 spare slots and fades out over 5 ms. The new grain keeps the slot it always took, so steal-free output is bit-identical to v3.0.4.
- **2026-10-26 (v3.0.4):** Freeze fixed (research: `research/granular-synthesis-2026-update.md` §2.2). Frozen grains all started at snapshot index 0, so Scan and Spread did nothing and the hold buzzed at the spawn rate. They now read through the snapshot from a virtual write head (`head − position` latched at spawn). Two snapshot slots, with each grain latched to one, replace the 5 ms timer that switched grains to the live line mid-grain. The loop seam gets a 10 ms equal-power crossfade, the loop length is latched to the audio actually written, and frozen grains get ±6 ct of detune.
- **2026-10-15 (v3.0.1):** Custom collection Apply fixed. Typed bare integers were parsed as Scala ratios (1200 → 1200/1), so every typed list failed the period check. Bare integers are now cents in the text field; `.scl` keeps its ratio rule via an `a/1` rewrite. Apply now shows a success line.
- **2026-10-15 (v3.0.0):** Pitch collections + separate Dry/Wet. A cents-based `PitchEngine` replaces the 12-TET `ScaleQuantizer`. It ships 32 collections (the 5 v2 scales, modes, 19/22/24/31/53-EDO, 5- and 7-limit JI, harmonics, Bohlen-Pierce, slendro/pelog/maqam, Free, Custom via typed cents/ratios or a Scala .scl file), per-degree toggle chips saved with the session, Range/Transpose/Fine/Direction, and `dry_level`/`wet_level` replacing `dry_wet` (state migration, null-tested). Fixed grains reading ahead of the write head: Scan 0 % had been a 2 s echo, and pitched-up/reverse grains wrapped to the oldest audio. Each grain is now pre-rolled and the delay buffer is 12 s. MAJOR (dry_wet removed, scale list 5 → 32).
- **2026-09-26 (v2.9.0):** UI pass R4/R5 (review 260924-nho). Adds `:root` palette tokens (127 hex → 0 outside `:root`), AA inks from ouaricon-naturalist-001 (#6A5641 walnut text over the gradient, #4E6839 lit switch), the 9px floor (captions, hints and canvas labels), and bundled EB Garamond via `modules/ui/eb-garamond`. Active text below AA 12/41 → 0/41 (min 2.28 → 5.14, pixel probe). Knob keyboard/ARIA (R7) still open. The inactive dimmed controls (opacity 0.25) are unchanged. No DSP change.
- **2026-08-19 (v2.4.4):** UI layout fix (authored as 2.4.3 pre-licensing-release; renumbered at merge) — the Spatial Audio section was clipped off the bottom of the editor (content ran to y=884.5 in an 850 px window, cutting the bottom row of spatial knob readouts and the mode hint). Root cause: `.controls-area` used `grid-template-rows: 1fr 1fr`, forcing the Beat Sync / Euclidean Rhythm row to match the taller Core Engine / Pitch & Scale row (199.5 px rendered for 111 px of content). Changed to `auto auto` (row 2 now 112 px), shortened the editor 850 → 800 px, made `.plugin-container` size from `setSize()` rather than hard-coding 900×850, and let `.viz-area` absorb the remaining slack via `flex: 1 1 auto; min-height: 200px`.
- **2026-08-19 (v2.4.3):** Licensing release — AGPL-3.0 notice headers added to all Ouaricon-authored source files. No audible or behavioral change.
- **2026-07-09 (v2.4.2):** Info-finding cleanup sweep (CODE_REVIEW.md IN-* items). Applied IN-01 (removed dead `getCrossfadeGain()` + `crossfadeDirection`), IN-02 (removed dead `TempoTracker::lastPpq`), IN-05 (`GrainPool::clearVoices()` in `prepareToPlay`), IN-09 (cached HOA write pointers per block), IN-10 (documented distance split-semantics), IN-11 (`reset()` resets `TempoTracker`), IN-14 (removed dead `.dimmed-spatial` CSS), IN-15 (`timerCallback` early-returns when hidden). Reviewed & no change needed: IN-03 (not a bug), IN-04/IN-06 (already fixed in v2.4.1), IN-07 (intended stutter), IN-08 (negligible), IN-12/IN-13 (acceptable/documented). No audible or behavioral change.
- **2026-07-08 (v2.4.1):** Code-review resolution pass — fixed all 2 critical + 12 warning CODE_REVIEW.md findings. CR-01 dead Scan knob (missing relay/attachment), CR-02 RT realloc in reset(), WR-01 block-held spatial feedback, WR-02 swing dropping off-beats + Euclidean desync, WR-03 freeze-engage memcpy, WR-04 spawn cap, WR-05 feedback/LPF NaN guards, WR-06 control-rate SH trig, WR-07 block-size clamp, WR-08 distance-LPF smoothing, WR-09 bpm<=0 guard, WR-10/WR-11 skew-correct readouts + `getParameterDefaults` native fn, WR-12 CMake version 2.1.0→2.4.1
- **2026-03-09 (v2.4.0):** Added grain scan position (`scan_position` 0-100%) — sweeps base grain read position through delay/freeze buffer (0-2 seconds)
- **2026-03-08 (v2.2.0):** Added grain size randomization (`size_random`) and per-grain amplitude randomization (`amp_random`) — both 0-100% with UI knobs
- **2026-03-08 (v2.0.3):** Fixed zipper noise on feedback/dry-wet automation in spatial mode (SmoothedValue bypass)
- **2026-03-08 (v2.0.2):** Fixed critical stack buffer overflow in binaural decode + hoaBus sizing
- **2026-02-09 (v2.0.1):** Fixed density parameter exponential curve
- **2026-02-06:** Ideated — Creative brief and requirements created
- **2026-02-06:** Research reference: `research/stutter-effects/path-a-granular-stutter-engine.md`

## Heritage

Built on the granular engine from the Scatter plugin:
- 64-voice grain pool, Lagrange3rd delay buffer, Hann window, scale quantization
- Extended with beat-sync, freeze, pitch ladder, Euclidean rhythms, texture morph

## Known Issues

- **Spatial-mode feedback is mono (by design, since v2.4.1).** The WR-01 fix derives the per-sample feedback signal from the HOA omni (W) channel — the mono grain sum available inside the main loop — rather than the post-decode binaural stereo pair (which was one block late). The recirculated signal is mono, but the grains re-spatialize it on the next pass, so the audible field stays spatial. A future enhancement could reconstruct a per-sample stereo feedback from the W/Y channels if stereo feedback character is desired.
- **Trajectory SH targets update at a 16-sample control rate (since v2.4.1, WR-06).** Imperceptible for grains ≥10 ms (≥441 samples at 44.1 kHz); the one-pole SH smoother fills the gaps. Trajectory position and Doppler remain per-sample.

## Additional Notes

### Unique Value Proposition
"Harmonic Stutter" — the only stutter effect combining 64-voice granular synthesis, musical scale quantization, beat-synchronized triggering, and density-based texture morphing.

### Key Parameters (~20 total)
- Core granular: Grain Size, Density, Scan, Spread, Reverse, Feedback, Dry Level, Wet Level, Size/Amp Random, Shape, Shape Tilt
- Pitch: Pitch Random, Range, Transpose, Fine, Pan Random, Collection (32), Root, Pitch Mode, Direction + per-degree chips
- Beat sync: Sync Mode, Probability, Grain Skip, Repeats
- Texture/Pitch: Texture (stutter-to-cloud morph), Pitch Mode (Random/Ladder/Pendulum), Freeze
- Euclidean: Pulses, Steps

### Differentiation from O-Freeze
O-Freeze = spectral freeze (FFT-based, static textures)
O-GrainScatter = granular stutter (time-domain, rhythmic/evolving grain effects)
