# O-Bowed Changelog

All notable changes to O-Bowed will be documented in this file.

## [1.0.7] - 2026-04-11

### Fixed
- Phantom second note playing in harmony alongside every MIDI note. Root cause: `stringCount` parameter range was 1-4 (minimum 1), forcing at least one drone string to always be active. Drone strings play at the fixed reference pitch (440Hz default), not the MIDI note frequency, producing an audible second tone at a different pitch from the played note. The drone persisted beyond note-off due to natural waveguide energy decay
  - Changed `stringCount` range from 1-4 to 0-4 with default 0 (no drones)
  - Updated `DroneStringEngine::setStringCount()` to accept 0
  - Recalculated all factory preset drone counts: realistic instruments (Violin, Cello, Viola, Double Bass, Erhu, Sarangi) now default to 0 drones; Nyckelharpa keeps 1 drone (authentic); Metal Drone and Impossible Strings keep 2 drones (intentional)

## [1.0.6] - 2026-04-11

### Fixed
- Sound design presets (Glass Bow, Metal Drone, Impossible Strings, Breath of Strings) produced saturated infinite tones with overtone dominance instead of musical bowed sounds. Three root causes:
  1. Loop gain reached 1.0 at high `infiniteSustain` (zero energy loss), causing waveguide saturation and harmonic distortion from tanh limiter. Capped max loop gain at 0.9995 (~15s decay at 440Hz)
  2. No feedback mechanism to reduce bow excitation at high waveguide energy. Added energy-aware excitation limiting: automatically scales down reflection coefficient when waveguide energy exceeds a sustain-dependent threshold (physically motivated — bow loses grip on strongly oscillating string). Zero effect when `infiniteSustain` = 0
  3. Reversed friction could push rho beyond stable range, causing excessive velocity injection. Clamped post-reversal rho to max 0.85
- Retuned all 4 sound design preset values for stability: Glass Bow (infSustain 0.8→0.45), Metal Drone (infSustain 0.5→0.3, reversed 0.4→0.2, subHarm 0.6→0.3), Impossible Strings (infSustain 0.7→0.4, reversed 0.6→0.3, subHarm 0.8→0.35), Breath of Strings (infSustain 0.3→0.15)

## [1.0.5] - 2026-04-09

### Fixed
- Output clipping at +6dB even with quiet settings. Root cause: `BodyResonator::processStereo()` summed 8 parallel peaking EQ filters without averaging — each filter outputs ~unity at non-peak frequencies, inflating the baseline by 8x (~+18dB). Fixed by dividing the parallel sum by `NUM_MODES`, restoring unity baseline gain while preserving relative resonant peak character

## [1.0.4] - 2026-04-09

### Fixed
- Drone strings produced continuous sound even with no MIDI notes held. Root cause: `DroneStringEngine::setStringCount()` unconditionally called `startBow()` on activation and never stopped bowing. Added MIDI note-activity gating via `setNotesActive()` — drones now only bow while at least one synth voice is active, with natural release decay when all notes are released

## [1.0.3] - 2026-04-09

### Fixed
- No sustained tone: `newVelocity = rho * v_delta` produced a monotonically increasing effective friction curve (no negative slope), making Helmholtz self-excitation impossible. Replaced with stick-slip injection model: reconstruct friction velocity from rho (`2*rho/(1-rho)`), clamp to `|v_delta|` for sticking limit. Creates the required friction peak + negative slope for sustained oscillation
- One-sided bow injection (bridge-only) starved the nut-bound wave of energy. Restored symmetric injection to both outgoing waves per standard scattering junction
- Hard clipping (`jlimit ±1.5`) inside the waveguide feedback loop generated DC offset. Replaced with `tanh` soft saturation at ±4.0 (odd-symmetric, no DC generation)

## [1.0.2] - 2026-04-06

### Fixed
- Critical waveguide scattering junction bug: outgoing waves were pushed back into the same delay line they came from instead of crossing to the opposite side, causing incorrect wave propagation in both `processSample()` and `writeJunction()`
- Bridge output now correctly reads from nut reflection (wave traveling toward bridge)
- Critical sustained oscillation failure: bow injection was added to BOTH outgoing delay lines (common-mode), causing the termination reflections to cancel the injection after each round trip — only a transient click was audible. Fixed by injecting only into the bridge delay line (one-sided injection), which breaks the cancellation and allows energy to accumulate across round trips

## [1.0.1] - 2026-04-06

### Fixed
- All 11 factory presets produced near-silent output due to inverted skew normalization formula for bowSpeed and bowPressure parameters (used `pow(proportion, 1/skew)` instead of `pow(proportion, skew)`)
- Presets now produce correct bow speed (e.g., Violin: 0.2 m/s instead of 0.02 m/s) and pressure values

## [1.0.0] - 2026-04-05

### Added
- Initial release
- Physical modeling bowed string synthesis via digital waveguide + nonlinear friction junction
- Tiered friction model: Core (hyperbolic), Enhanced (elasto-plastic), Quality (thermal)
- Morphable body resonator with Material and Size controls (membrane/wood/metal/glass)
- 1-4 active bowed strings with per-string tuning offsets
- Sympathetic string coupling (0-12 passive waveguide strings)
- Impossible physics: Infinite Sustain, Reversed Friction, Sub-Harmonics
- MPE support (per-note pitch bend, pressure, slide)
- Microtonal tuning: Scala/TUN import, MTS-ESP, 12-TET
- WebView UI with Naturalist aesthetic
- 11 factory presets (7 realistic instruments + 4 sound design)
- Passes pluginval level 10 (VST3 + AU)

### Technical Notes
- 2x oversampling on friction junction
- 8-mode parallel biquad body resonator
- Zero algorithmic latency (waveguide is causal)
