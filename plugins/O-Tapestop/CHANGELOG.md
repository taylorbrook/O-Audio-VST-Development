# Changelog — O-Tapestop

All notable changes to O-Tapestop are documented here.

## [1.1.0] — 2026-08-16

### Added
- **Continuous mode** (MODE gains a third choice) — tape-speed motion runs
  continuously while ENGAGE is latched, instead of as a single gesture.
  Three characters via the new **CHARACTER** parameter:
  - **Wobble** — deterministic wow sine + 3-harmonic flutter stack
    (ChowTapeModel motor-model ratios 0.56/0.20/0.24); CHAOS morphs toward
    per-cycle rate/amp jitter, slow Ornstein-Uhlenbeck drift and a filtered
    noise band.
  - **Random** — Ornstein-Uhlenbeck octave stack (RATE sets correlation
    time; CHAOS adds octaves and widens excursion), 20 Hz post-LP, plus a
    weak position-debt servo (±0.2 %, ~5 s time constant) so a long hold
    never walks into the capture-ring rail.
  - **Glitch** — tempo-grid event scheduler (p = chaos² per cell):
    tapestop-dip, half-speed drag, speed-jump, reverse-flick, stutter-repeat
    (2-voice splices at slice/4, 3–50 ms), resync-snap. Event selection is
    debt-biased with a 3 s soft budget and 6 s hard resync-snap, so debt is
    self-centering by construction.
- New parameters: `CHARACTER`, `CONT_RATE_SYNC_DIV` / `CONT_RATE_HZ`
  (Sync/Free twin, 0.05–20 Hz), `CONT_DEPTH` (log-perceptual 0.1 %→12 %
  peak speed deviation ≈ ±2 cents→±2 semitones), `CONT_CHAOS`. DEPTH/RATE
  are live (16-sample absolute grid); CHARACTER + RNG seeds latch at the
  engage edge (deterministic, repeatable bounces).
- Release from Continuous rides the existing SpinUp → Catchup → ResyncXfade
  path — START time/curve shape the return, post-resync output stays
  bitwise dry.
- 6 new factory presets: Subtle Wobble, Warped Record, Drunk Tape, Seasick,
  Glitch, Total Meltdown (14 total).
- UI: MODE segment is 3-way (Stop / Scratch / Motion); new Continuous pane
  (Character stack, Rate slot, Depth + Chaos knobs). The three mode buttons
  stack vertically at full ENGAGE width (140px) — the first cut squeezed
  them side-by-side into thirds of the 148px TRIGGER content box, which
  clipped SCRATCH/MOTION even at 8px type.
- Render harness extended 47 → 62 checks: per-character determinism /
  block-size invariance / post-release null, 40 s debt-bound probes,
  continuity (first-difference) scans, depth liveness + zipper probe,
  preset-migration probe.

### Changed
- Preset-manager module v1.0.6: optional `setMigrationCallback()` hook,
  invoked with each preset's parameters + saved version before apply.
- Presets saved by v1.0.0 are migrated on load: MODE was stored normalized
  over 2 choices — 1.0 (Scratch) would decode as Continuous over 3. The
  migration remaps pre-1.1.0 MODE fractions (round(n)·0.5).

### Known caveats
- VST3 automation lanes of MODE recorded before 1.1.0 store the normalized
  value host-side and cannot be migrated: a lane sitting at 1.0 (formerly
  Scratch) now selects Continuous. MODE is a setup control (ENGAGE is the
  performance param), so exposure is expected to be rare.
- At MIX < 100 %, Continuous mode blends wet near-unity-speed audio against
  dry — tape-flanging combing is audible by design.

## [1.0.0] — 2026-08-15

Initial release.

### Added
- **Stop mode** — tape-stop / tape-start varispeed transport: tempo-synced
  (1/16 – 4 bars) or free-time (10 ms – 8 s) spin-down and spin-up with
  independent curve shaping, Signalsmith-style resync (1.25× catch-up +
  crossfade skip, bitwise-dry post-resync null).
- **Scratch mode** — drawable bipolar speed-vs-time envelope (2–64 points,
  per-segment curvature), r ∈ [−2, +2], tempo-synced or free pass length.
- **Tone Track** — speed-tracking low-pass that darkens the wet path as the
  tape slows, wet-path/engaged-only.
- **Mix / Output Gain** on the engaged chain only; the Bypassed state is a
  true bitwise pass-through.
- **WebView UI** (860 × 580, Ouaricon Naturalist): large latching ENGAGE,
  mode-switched center panel, drawable envelope editor with live playhead,
  live playback-ratio readout.
- **Preset system** — shared preset-manager module v1.0.5: save / save-as /
  load / load-from-file / prev / next / two-click delete, factory + user
  banks under `~/Library/Ouaricon Tapestop/Presets/`. The scratch envelope
  rides inside preset JSON as an opaque `customState` blob.
- **Factory bank** — 8 presets: Classic Half-Bar Stop, Classic 1-Bar Stop,
  DJ Spinup, Baby Scratch, Chirp Flare, Tempo-Synced Short Stop,
  Slow-Tape Drag, Stutter-Scratch.
