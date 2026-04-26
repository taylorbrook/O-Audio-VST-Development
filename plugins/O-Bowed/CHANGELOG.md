# O-Bowed Changelog

All notable changes to O-Bowed will be documented in this file.

## [1.3.0] - 2026-04-26

### Added

- **adds VST3 Note Expression microtonal support for Dorico.** O-Bowed responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events), enabling microtonal playback of quarter-tones and arbitrary tuning deltas authored in Dorico's tonality system. End users must set Microtonality to "VST3 Note Expression" on the assigned expression map (see O-Lyrica 2.3.0 for procedure).
- **Shared `note-expression` module adoption.** O-Bowed consumes the Ouaricon module at `modules/tuning/note-expression` (v1.0.0), same shape as O-Lyrica v2.3.0 / O-Bells v4.1.0 / O-Prism v1.17.0 / O-Wind v1.16.0 / O-IntonationPad v2.8.0 / O-Reed v1.1.0.

### Technical Notes

- **Second MPE consumer of the shared note-expression module.** `BowedStringVoice` extends `juce::MPESynthesiserVoice` (via `BowedMPESynthesiser`) and reads MIDI pitch via `getCurrentlyPlayingNote().initialNote` from `noteStarted()` and `notePitchbendChanged()` (no parameter form). Pattern 1 (`noteId` correlation in the shared module's `updatePendingFromEvents`) holds regardless of MPE channel — same as O-Reed v1.1.0.
- **Helper-based composition (single source of truth).** `applyPendingTuning` is invoked INSIDE `getBaseFrequencyFromTuning(midiNote)` so BOTH call sites — `noteStarted()` (line 32) and `notePitchbendChanged()` (line 71) — inherit the NE delta with one insertion. `exchange(0.0)` consume semantics correct for one-NE-per-noteOn delivery: first call (in `noteStarted`) consumes the slot; the `notePitchbendChanged` call during a held note returns base unchanged (NE applies once per noteStarted; MPE pitch-bend updates per-block on top).
- **Composition order:** tuning engine → NE delta → MPE pitch-bend → `waveguideString.trigger(currentFrequency)` (waveguide string period sized to the final tuned frequency on sample 0; Pattern 2 satisfied — first sample at tuned pitch, no attack zipper).
- **Files modified:** `Source/PluginProcessor.{h,cpp}`, `Source/BowedStringVoice.{h,cpp}`, `CMakeLists.txt` (added `PLUGIN_VERSION "1.3.0"` line + `ouaricon_add_module(O-Bowed note-expression)`).
- **Version:** 1.2.1 → 1.3.0 (MINOR — new user-visible feature, backward compatible, no preset impact).

## [1.2.1] - 2026-04-19

### Fixed
- **Humanize panel layout** — column labels (Speed/Pressure/Position/Rosin) were overflowing their 42px-wide dial cells in the 200px-wide left panel, causing visual crowding. Reduced column-label font from 9px → 8px, tightened letter-spacing from 0.5px → 0.3px, narrowed dial cells from 42px → 38px, and reduced grid gap from 4px → 1px so all four labels and their dial pairs fit cleanly within the section box

## [1.2.0] - 2026-04-17

### Added
- **Humanize** section with per-parameter random-walk variation on the four primary bow controls (Speed, Pressure, Position, Rosin). Each parameter exposes two dials:
  - **Amt** (range) — peak deviation. At 0 the walk is inactive; at 1 the parameter drifts across its full musically-tuned deviation (speed ±0.30 m/s, pressure ±0.60 N, position ±0.05, rosin ±0.20).
  - **Rate** — drift speed, mapped 0→0.15 Hz to 1→8 Hz as the corner frequency of a 1-pole smoother fed with uniform-noise targets (sum of 3 uniforms for a Gaussian-ish distribution).
- The walk state lives in a new processor-level `HumanizeEngine` (`Source/DSP/HumanizeEngine.h`), updated once per `processBlock` and shared across all 8 voices so the instrument breathes coherently. State is continuous — never reset on note-on — matching the natural drift of a live player
- Default Rate values (0.20–0.35) chosen to feel like subtle natural bow fluctuation (≈2–3 Hz). All Amt dials default to 0 so existing presets are unchanged

## [1.1.2] - 2026-04-16

### Fixed
- Audible thump at the start of every new note whenever Reversed Friction was non-zero. Root cause: the reversal formula `rho = rho + reversedAmount * (1 - 2*rho)` evaluates to `reversedAmount` when base `rho ≈ 0` (which it is while `F_bow` is still ramping through the attack envelope). That pinned `rho` from the very first sample of the note, making `frictionVelocity = 2ρ/(1-ρ)` large enough for the sticking branch (`|v_delta| < frictionVelocity`) to always hold — so the bow-velocity attack ramp was injected directly into a freshly reset waveguide as a velocity step, producing a broadband click. Fixed with a per-voice one-pole smoother on `reversedAmount` that resets to 0 on `noteStarted()` and ramps to the knob value over ~25 ms at the oversampled rate, letting physical friction build up first before the reversal takes effect. Sustain-time character is unchanged
- `BowModel::startBow()` now zeros `v_bow`/`F_bow` before setting new targets, so retriggering a note while a previous note's release envelope is still decaying no longer carries residual bow state into the new attack

## [1.1.1] - 2026-04-11

### Fixed
- Enhanced (elasto-plastic) and Quality (thermal) friction models produced no sustained tones. Root cause: bristle stick-slip thresholds (`z_ss`, `z_ba`) were hardcoded constants that didn't scale with bristle stiffness (`sigma_0`). The steady-state bristle displacement (`mu*F_bow/sigma_0 ≈ 6.7e-5`) fell far below the stick threshold (`z_ss = 5e-4`), trapping the model in the elastic region where no stable equilibrium exists — bristle state oscillated erratically instead of converging. Fixed by making thresholds scale inversely with `sigma_0` so the transition zone always surrounds the physical equilibrium point
- Friction state now resets on note-on to prevent stale bristle displacement from previous notes affecting attack

## [1.1.0] - 2026-04-11

### Removed
- Drone string functionality entirely — removed `stringCount`, `stringTuning1-4` parameters, `DroneStringEngine` class, and all associated DSP/UI code. Drone strings were a persistent source of bugs (v1.0.3, v1.0.4, v1.0.7) and redundant with simply playing the desired note
- "Metal Drone" factory preset (drone-dependent)

### Changed
- Factory presets reduced from 11 to 10 (7 realistic + 3 sound design)
- Nyckelharpa preset: removed drone string, retains sympathetic strings for authentic character
- Impossible Strings preset: removed drone dependency, retains sympathetic strings + impossible physics
- Full 8-voice polyphony always available (no longer dynamically capped based on drone count)

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
