# Changelog — O-Tapestop

All notable changes to O-Tapestop are documented here.

## [1.3.0] — 2026-08-16

### Added
- **14 new factory presets — the bank doubles to 28.** New Tape Stops
  (Power Cut, Cassette Eject, Two-Bar Dive, Snap Back, Half-Mix Stop),
  Scratch shapes with four new envelope blobs (Transformer, Tape Rewind,
  Orbit, Crab Roll), Wobble & Warp (Tape Flutter, Pitch Tide, Loose
  Capstan), and Glitch & Chaos (Sputter, Data Rot). All authored in
  engineering units through the CR-02 conversion, every preset carries all
  19 param IDs + an envelope blob, ENGAGE stays 0 everywhere. The
  sentinel-gated factory writer re-runs on the version-string change, so
  the new files land on first scan.
- **Themed preset dropdown.** Clicking the preset name opens a grouped
  panel — Tape Stops / Scratch / Wobble & Warp / Glitch & Chaos, plus a
  dynamic User group for anything not in the factory map. The prev/next
  carousel is unchanged. Grouping is display-side only (`PRESET_THEMES` in
  js/app.js): `getPresetList()` stays a flat alphabetical sort, the preset
  JSON format is untouched, and the shared preset-manager module (C++ and
  JS) is unmodified. Panel is rebuilt on every open, closes on outside
  click / Escape / selection, and stays inert until the manager's
  initialize() resolves (same honest-disable contract as the buttons).

### Testing
- UI exercised in a Playwright browser harness with the JUCE bridge
  stubbed at `js/juce/index.js` (route interception; the stub feeds the
  real alphabetized 28-name factory list + 2 fake user presets): all 5
  groups render in order, 30 items, current-preset marker tracks
  selection, click-to-load closes the panel, Escape and outside-click
  dismiss, panel bottom lands at y=508 in the 580 px window and scrolls
  internally. Honest-disable held: with the stub absent the band and
  dropdown stay inert.
- auval PASS (aufx OTsp OuDv); pluginval strictness-10 SUCCESS on the
  installed VST3.

## [1.2.2] — 2026-08-16

### Fixed
- **Continuous pane overflowed the center panel — CHARACTER label crossed
  the left border, CHAOS knob crossed the right.** Root cause: the pane's
  34 px column gap was budgeted against the 58 px segment column, but the
  CHARACTER caption widens its column to ~71 px, so the centered flex row
  (71 + 3×88 + 3×34 = 437 px) overflowed the 398 px content box ~20 px on
  BOTH sides — invisible to the column-sum arithmetic, caught only by
  measuring rendered boxes. Gap reduced 34 → 16 px; measured clearance is
  now 7.3 px per side.

## [1.2.1] — 2026-08-16

### Changed
- **Glitch pushed further off the grid — more erratic at high CHAOS.**
  v1.2.0's barrage was dense but still grid-locked: every event started
  exactly on a slot boundary, which reads as rhythmic rather than erratic.
  All four changes are gated on `g = 2·max(0, chaos − 0.5)` (no RNG draw at
  g = 0), so CHAOS ≤ 0.5 stays bit-identical to v1.2.0:
  - **Slot-start jitter** — each slot's event attempt is deferred by a
    random 0–35 % of a slot (scaled by g), so high-chaos events fall
    off the tempo grid instead of quantizing to it.
  - **Density** — slots per cell now unlock 1→5 above CHAOS 0.4 (was
    1→4), max-chaos cadence up 25 %.
  - **⅛-cell micro-bursts** — a fourth event-length tier below the ¼
    fraction (weight 1.4·g·chaos²), and the ¼ weight itself rises with g
    (2.2 → 3.8 at max): more blink-length freezes/slams/stutter blips.
  - **Tame fade deepened** — dip/half-speed weights fade 65 % at max
    chaos (was 50 %), so the top of the CHAOS range is dominated by the
    extreme family.
  - Debt safety unchanged (3 s soft / 6 s hard budgets); measured 40 s
    worst-case debt 1.77 s (bound 8 s). Render harness: 65/65 pass with
    no bound recalibration — C-P6 glitch continuity 0.0576 vs 0.0904.

## [1.2.0] — 2026-08-16

### Changed
- **Glitch character overhauled — sudden, extreme, dense.** Root cause of
  the tame v1.1 sound: one event per grid cell, always full-cell length,
  with a mostly-smooth palette (shaped dips, flat half-speed holds).
  - **Sub-cell scheduling** — 1→4 event slots per cell unlock as CHAOS
    rises past 0.4, and event lengths draw from {1, ½, ¼}×cell with short
    bursts favored at high chaos: max-chaos Glitch is now a rapid-fire
    barrage instead of one texture per cell.
  - **New events** (unlock above CHAOS 0.5): dead-stop **freeze** (r = 0,
    instant resume), **slam** (holds the +2× engine rail), square-wave
    **chatter** (1 ± depth at 10–30 Hz), buffer-**shuffle** (spliced
    back-jump of 1–4 cells, replaying recent audio), and stutter
    **halving-roll** / **±2-semitone pitch-ramp** variants with per-event
    random slice depth (event/2..event/8 — micro-buzz at the bottom).
  - **Reverse-flick boosted** — chaos pushes the flick toward the −2 rail
    (was capped at −depth).
  - **Chaos remap** — tame-family weights fade 50 % as extremes ramp, so
    the top quarter of CHAOS reads as mayhem, not decorated wobble.
    CHAOS ≤ 0.4 keeps the v1.1 cadence; existing Glitch presets at high
    chaos ("Glitch", "Total Meltdown") now sound substantially wilder.
  - Debt safety unchanged: new events carry Δdebt signs in the exp bias
    (shuffle capped at 2 s + hard-budget headroom); 3 s soft / 6 s
    hard-snap budgets as before. Measured 40 s worst-case debt: 2.12 s.
- Render harness 62 → 65 checks: new C-P0x max-chaos probes (two-instance
  determinism, 512-vs-4096 bit-identity, post-release bitwise null with
  the full v1.2 palette reachable); C-P6 glitch continuity bound
  recalibrated from the r = 1.85 precedent to the ±2 rail (slam + ramped
  stutter splices legitimately exceed the old bound: measured 0.0576 vs
  old 0.0553 / new 0.0904 — click-detection power intact, a real click
  sits ~10× above the new bound).

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
