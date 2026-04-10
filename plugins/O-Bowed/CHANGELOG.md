# O-Bowed Changelog

All notable changes to O-Bowed will be documented in this file.

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
