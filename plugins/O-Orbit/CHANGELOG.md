# Changelog — O-Orbit

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
