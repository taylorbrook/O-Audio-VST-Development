# Changelog — O-Orbit

## v1.1.0 — 2026-08-19

Feature release: Parts B–D of the v1.1 review-findings brief (suite parity + motion and speaker-editor upgrades). C2 Doppler and C4 custom path remain deferred per the brief.

### Added
- **Preset manager (B1)** — migrated to the shared preset-manager module (v1.0.6). Categorized preset menu (Stereo / Surround / Creative / User), prev/next stepping that walks the menu order (not the alphabetical list), user preset save/load/delete with native file dialogs, two-click armed delete. The 12 factory presets keep their exact v1.0.0 values (converted skew-aware through each parameter's own range at registration). The legacy Programs API is collapsed to a single program like the rest of the suite.
- **Hover help (B2)** — "?" toggle in the header + hover tooltips on all 17 parameters, the view toggle, preset band, editor toolbar, and downmix badge. Measure-then-pin fixed-position tooltips (viewport-clamped, arrow tracks the anchor). Preference persists with the session (plain tree property, `isVoid()`-gated restore).
- **PPQ-locked tempo sync (C1)** — when synced and the transport is playing, motion phase derives directly from the host beat position (`phase = fmod(ppq × cyclesPerBeat, 1) × 2π`, computed in double). Offline bounces are deterministic and downbeat-aligned; Drift's noise time locks too. Free-run remains the fallback when stopped or without a playhead (Standalone still moves).
- **Ping-Pong path (C3)** — 5th path choice (appended, never reordered — APVTS sessions store the index). Triangle-wave azimuth: sweeps −w/2 → +w/2 and folds back with no positional discontinuity, unlike Linear's saw-wrap.
- **Speaker editor: elevation + distance editing (D1)** — shift+vertical-drag edits elevation (±90°), alt-drag or scroll edits distance (0.1–30 m), plain drag still edits azimuth. Speakers draw at a distance-scaled radius (same mapping as the hit-test) with an elevation badge, and a hover/drag readout shows az/el/dist.
- **Named layout library (D2)** — save/load/delete named custom layouts (`~/Library/Ouaricon Orbit/Layouts/*.json`, same schema as export/import) from a dropdown in the editor toolbar. File export/import unchanged.
- **Height visualization (D3)** — source dot scales and brightens with elevation, side elevation gauge in motion view, height-layer speakers drawn with a dashed second ring in both views.
- **Resizable editor (D4)** — 600×450 to 1600×1200 at a fixed 4:3 aspect (default 800×600). Canvas re-rasterizes on resize (`setTransform`, not cumulative `scale`); visualizer height is now vh-based.

### Changed
- `moveSpeakerInLayout()` takes distance and re-derives the layout's `is3D` flag; the `moveSpeaker` native fn accepts an optional 4th argument (older callers keep the speaker's current distance).
- Session state now stores `currentPreset` and `tooltipsEnabled` as plain tree properties (absent in older sessions → defaults stand; pre-1.1.0 sessions unaffected).

### Compatibility
- All parameter IDs, ranges, and defaults unchanged; the only layout change is the appended 5th "path" choice. Saved sessions restore identically (APVTS stores the choice index). Normalized "path" *automation lanes* recorded against the 4-choice range decode against 5 — same accepted trade-off as prior suite choice-append releases.
- Factory presets are regenerated under the v1.1.0 sentinel; values match v1.0.0's Programs byte-for-byte in effect.

### Testing
- pluginval strictness 10 (VST3), auval (AU) — see build log
- Factory presets A/B'd against v1.0.1 Programs values (skew-aware conversion verified)
- PPQ sync: phase derived from beat position — two offline bounces produce identical motion

## v1.0.1 — 2026-08-19

Defect-fix release (Part A of the v1.1 review-findings brief; features B/C/D deferred to a later milestone).

### Fixed
- **Depth parameter was dead** — `MotionEngine` computed a Depth-driven distance but `processBlock` never read it; the distance model and visualizer only ever saw the static Distance param. Effective distance is now `Distance × motion distance` (floor-clamped at 0.1 m), fed to both L/R distance models and the visualizer dot.
  - *Behavior note:* factory presets with Depth > 0 (Fast Spiral, Ambient Drift, Deep Space, …) gain audible near/far motion for the first time — intended.
- **Audio-thread allocation every block** — `DistanceModel::updateDistance()` heap-allocated IIR coefficients (`Coefficients::makeLowPass`) per block for both channels. Now assigns `ArrayCoefficients::makeLowPass` (stack array) into the existing storage, and skips recomputation entirely when (distance, air absorption, curve) are unchanged.
- **Dead parameter smoothers / mix zipper** — five smoothers were reset in `prepareToPlay` but never advanced or read. Mix is now genuinely smoothed per-sample (20 ms ramp, snapped on prepare to avoid a fade-in); the four redundant motion smoothers were deleted (motion is phase-integrated and VBAP gains already interpolate per block). Distance gain now ramps per-sample inside `DistanceModel` — necessary since the Depth fix makes distance move every block.
- **Dry/wet mix tilted surround output frontward** — the mix loop only blended the first `min(in, out)` channels, so on 5.1/7.1.4 outputs at mix < 100% front L/R got dry-blended while all other channels stayed full wet. Now wet scales on ALL output channels and dry blends only into its native input channels (constant spatial balance).
- **Double-click knob reset landed off-default on skewed knobs** — reset went to normalized 0.5 instead of the parameter default (e.g. Speed, skew 0.5, default 1.0 Hz). Now resets to each knob's `data-default` via a skew-aware inverse of the parameter range.

### Root cause
v1.0.0 shipped with the Depth→distance wiring dropped between MotionEngine and processBlock, and per-block parameter application throughout the mix/distance path. Found by full code review 2026-08-19 (`.planning/improvements/v1.1-review-findings.md`).

### Testing
- pluginval strictness 10 (VST3), auval (AU) — see build log
- Depth sweep: audible level/HF motion + visualizer dot radius follows
- Mix automation sweep: no zipper (per-sample ramp)
