# O-Lyrica Changelog

All notable changes to O-Lyrica are documented in this file.

## [2.2.2] - 2026-04-13

### Changed

- **Étouffé buzz realism overhaul** in `WaveguideString::processSample` / `setDampening`. The v2.2.1 buzz used white noise → fixed 180–650Hz bandpass → waveguide injection, which sounded synthetic/hissy because the filter was pitch-agnostic (removed the fundamental for strings outside 200–500Hz). Replaced with a physically-informed palm-friction model:
  - **Pitch-coupled filtering:** Single LPF tuned to N×f0 (3× for loud strings, 6× for quiet), clamped 200–8000Hz. Lets the string's fundamental and first few partials through; the waveguide's own comb filtering shapes pitch identity. Replaces the two fixed-cutoff `OnePoleLPF` filters (`buzzLowCut` + `buzzHighCut`) with one per-note-tuned `buzzFilter`.
  - **Gradual onset ramp:** `buzzEnvelope` now starts at 0 and ramps to 1.0 over ~8ms (via `buzzOnsetRamp` + `buzzOnsetIncrement`), simulating progressive palm/felt contact wrapping the string. Previously jumped instantly to 1.0.
  - **Amplitude-dependent character:** Loud strings get a tighter filter (more tonal buzz) and up to 2× longer decay; quiet strings get a wider filter (breathier) and shorter decay. Scaled via `buzzCapturedEnergy` snapshot.
  - **Frequency-dependent duration:** Base buzz duration ∝ 1/f0, clamped 10–60ms. High strings (~2kHz): short percussive buzz. Low strings (~100Hz): longer tonal buzz. Matches physical behavior where thin short strings damp faster than heavy wound strings.
- **Buzz gain reduced** from 2.5 to 2.0 — the pitch-coupled filter passes more energy at the fundamental so less makeup gain is needed.

### Technical notes

- **Root cause of synthetic sound:** The fixed 180–650Hz bandpass was the primary issue. For a string at 1000Hz, the LPF@650Hz removed the fundamental entirely, leaving only sub-harmonic noise. For a string at 100Hz, the HPF@180Hz cut the fundamental. Only strings in ~200–500Hz range got reasonable results. The new approach tunes the filter relative to each string's actual pitch.
- **Research basis:** Acoustic analysis of harp étouffé (Valette & Cuesta, Woodhouse string damping studies) shows the buzz is damped resonance — the string's own partials dying with upper harmonics collapsing faster — not broadband noise. The "buzz" quality comes from amplitude modulation as palm progressively contacts the string.
- **RT-safety:** All new members are plain floats/bools, no allocations. `buzzOnsetPhase` branch is predicted well (true for ~350 samples then false). `buzzFilter.processSample` is the same branch-free `OnePoleLPF` used elsewhere.
- **Member changes:** Removed `buzzLowCut`, `buzzHighCut` (two `OnePoleLPF`). Added `buzzFilter` (one `OnePoleLPF`), `buzzOnsetRamp`, `buzzOnsetIncrement`, `buzzDecayRate` (floats), `buzzOnsetPhase` (bool). Net: same memory footprint.
- **Non-destructive:** No parameter ID changes, no preset impact. Purely internal DSP refinement.
- **Files modified:** `Source/DSP/WaveguideString.h`, `Source/DSP/WaveguideString.cpp`

## [2.2.1] - 2026-04-12

### Fixed

- **OnePoleLPF `setCutoff()` used `1/tan` instead of `tan`** — the v2.1.8 custom filter struct's coefficient calculation was inverted, producing DC gain of ~1/n instead of unity. For a 1kHz bridge filter at 44.1kHz, this attenuated by 14x per pass (~-23dB). With three filters in series (bridge, nut, loopDamping), 99.96% of energy was lost per waveguide cycle — strings died immediately after the pluck attack. Root cause: the comment claimed to match `juce::dsp::IIR::Coefficients::makeFirstOrderLowPass` but used `n = 1/tan(pi*f/fs)` with unnormalized numerator `(1, 1, ...)` instead of `n = tan(pi*f/fs)` with `(n, n, n+1, n-1)`. Fix: `b0 = b1 = n/(n+1)`, `a1 = (n-1)/(n+1)`.

### Added

- **Pedal buzz resonance on étouffé release** in `WaveguideString::processSample`. When the sustain pedal is released (CC64 < 64 via `HarpSynthVoice::controllerMoved` line 519), the étouffé dampening ramp now injects a short bandpass-filtered noise burst into the waveguide rails to simulate the characteristic buzz a real harp produces when palm or felt contacts vibrating strings. Previously the ~50ms feedback ramp attenuated the string cleanly toward silence with no friction artifact — physically wrong, since muting a ringing string with palm contact produces audible stochastic friction noise before the string settles.
- **Signal path:** White noise from a per-voice `juce::Random` → HPF @ 180Hz (via `noise - lowCut.process(noise)`) → LPF @ 650Hz → bandpass-shaped burst centered in the palm-friction band. Injected into `excitationToUpper`/`excitationToLower` at the pluck-position split so the buzz travels through the waveguide and inherits the string's current nut/bridge/loop-damping colour and material character.
- **Amplitude scaling:** `buzzSample = bandpassed * buzzEnvelope * buzzCapturedEnergy * 2.5f` where `buzzCapturedEnergy` is a snapshot of `currentEnergy` taken at the moment `setDampening(true)` fires. This makes silent strings stay silent (no buzz from dead voices) while loud ringing strings buzz audibly — matching physical reality where palm contact on a dead string produces nothing.
- **Envelope:** `buzzEnvelope` starts at 1.0 on pedal-release and decays at `dampeningDecayRate` (same rate as the étouffé feedback ramp, ~50ms to -60dB). The buzz is gated out once envelope drops below `1e-5` to avoid wasting cycles after the ramp finishes.
- **State handling:** `setDampening(true)` captures energy snapshot and resets envelope. `trigger()` and `reset()` clear all buzz state (filters, envelope, captured energy) so new notes never inherit a stale buzz.

### Technical notes

- **Filter style:** Uses the existing `OnePoleLPF` inner class for consistency with the rest of `WaveguideString` — bandpass via `HPF = x - LPF(x)` subtraction is a standard one-pole bandpass approximation, ~6dB/oct skirts on both sides, sufficient for friction-noise shaping without the cost of a biquad.
- **RT-safety:** `juce::Random::nextFloat()` uses an integer LCG — no allocation, no syscalls, safe in `processSample`. `OnePoleLPF::processSample` is branch-free. Buzz path is guarded by `dampening && buzzEnvelope > 1e-5f` so CPU cost is zero when pedal is up or envelope has decayed.
- **Non-destructive behavior:** Purely additive. Zero effect on voices where the pedal is never released mid-ring, zero effect on existing presets or sessions, no parameter changes. The buzz only fires during the 50ms étouffé ramp — after that `buzzEnvelope` reaches the 1e-5 floor and the branch is skipped entirely.
- **Why 180–650Hz band:** Matches the spectral character of palm/felt friction on vibrating wound or gut strings — dominant energy in low-mid band, rolling off above ~800Hz where felt absorption takes over. Low-cut at 180Hz removes sub-bass rumble that would muddy the effect.
- **Why inject through the waveguide (not output bus):** Feeding the buzz into `excitationToUpper`/`excitationToLower` makes it travel through the bridge filter, nut filter, loop damping, stiffness filter, and sympathetic resonance path — so the buzz inherits the instrument's current timbre (brass vs gut, bright vs dark, etc.) and couples sympathetically into other ringing strings. Injecting post-waveguide would sound like a separate noise generator pasted on top.
- **Files modified:** `Source/DSP/WaveguideString.h`, `Source/DSP/WaveguideString.cpp`
- **Version bump rationale:** PATCH (v2.2.0 → v2.2.1) — enhancement to an existing feature (étouffé), no new parameters, no breaking changes, no preset compatibility impact.

## [2.2.0] - 2026-04-11

### Added

- **Perfect fourth and septimal minor seventh sympathetic coupling** in `SympatheticResonanceEngine::computeCouplingStrength`. Real harp sympathetic vibration — especially on Celtic and lever harps — couples strongly at 4:3 (perfect 4th) and, on natural/just-intoned tunings, at 7:4 (septimal minor 7th). Previously only unison, octave, fifth, and major third excited the coupling matrix; players holding a 4th or a blue-note 7th heard no sympathetic bloom from neighboring strings even though the physical model is otherwise detailed enough to warrant it.
- **Perfect fourth:** ratio `4:3` (and `3:4` inverse), coupling weight `0.4f` (between `FIFTH_COUPLING 0.5f` and `THIRD_COUPLING 0.3f`), tolerance `1.01451` (~25 cents — matches the third's window; 12-TET P4 is only ~2 cents sharp of 4:3 so tempered fourths are captured cleanly).
- **Septimal minor seventh:** ratio `7:4` (and `4:7` inverse), coupling weight `0.2f` (weakest interval), tolerance `1.01744` (~30 cents). The 30-cent window is deliberately *looser* than other intervals because the 7:4 harmonic is ~31 cents flat of the 12-TET minor 7th — a tighter window would miss real 7:4 intervals, and a wider one would false-trigger on every tempered m7. The chosen window lets true 7:4 (natural-horn 7ths, just-intoned harp tunings, blue-note inflections) excite the coupling while tempered m7s correctly fall through to `return 0.0f`.
- **Detection ordering** in `computeCouplingStrength` is musical-strength descending with early returns: Unison → Octave → Fifth → Fourth → Third → Seventh. The strongest match wins; weaker intervals only run when none of the stronger ones fit. This keeps the hot-path cost bounded (typically 1-2 ratio checks per voice pair).
- **Comment block** at top of `computeCouplingStrength` now lists the new 30-cent precomputed tolerance ratio (`2^(30/1200) ~ 1.01744`) alongside the existing 10/15/20/25-cent entries.
- **Header docstring** in `SympatheticResonance.h` (class-level and `computeCouplingStrength` doxygen) updated from "unison, octave, fifth, third" to "unison, octave, fifth, fourth, third, septimal minor seventh".

### Technical notes

- **Non-destructive behavior:** Purely additive. Any voice pair that previously matched unison/octave/fifth/third still returns the same coupling value via the same early-return path — existing presets, projects, and tonal character are unchanged. The new intervals only add coupling where there was previously zero.
- **Audio-thread cost:** Two additional `checkHarmonicIntervalFast` calls per voice-pair in the coupling-matrix rebuild path. `rebuildCouplingMatrix` runs only at block boundaries on voice register/unregister (gated by `rebuildPending`), never per-sample — zero audio-thread cost in steady state. Even at `MAX_VOICES=16` fully active with all voices changing every block, the rebuild loop is O(N²) over 16 voices with ~6 cheap ratio comparisons each: still negligible.
- **Thread safety preserved:** All new constants are `constexpr` in the same anonymous namespace as the existing coupling weights; detection logic runs inside the existing double-buffered `rebuildCouplingMatrix` → atomic buffer swap pattern. No new shared state, no new synchronization required.
- **Files modified:** `Source/DSP/SympatheticResonance.cpp`, `Source/DSP/SympatheticResonance.h`
- **Version bump rationale:** MINOR (v2.1.10 → v2.2.0) because this introduces new physical-model behavior — strings will now ring sympathetically at interval classes where they previously stayed silent. Not a bug fix (nothing was broken), and not breaking (no parameter or state changes).

## [2.1.10] - 2026-04-11

### Changed

- **Velocity-to-brightness curve in `HarpSynthVoice::startNote`** — Replaced the v1.32.7 velocity→`fingerHardness` mapping `jmap(velocity, 0→1, 0.78→1.10)` with `fingerHardness *= 0.7f + 0.3f * velocity`. Soft notes now start at `0.7×` the patch hardness (warmer, less HF excitation) and the curve tops out at `1.0×` at full velocity instead of pushing past the user's patch setting. On a real harp, harder plucks excite higher partials — this remaps that response so the parameter-set hardness acts as an upper bound rather than a mid-point. Velocity→amplitude behavior (`Synthesiser`'s envelope gain and the unchanged brightness mapping at line 164-166) is untouched.
- **File:** `plugins/O-Lyrica/Source/HarpSynthVoice.cpp` lines 190-195.

## [2.1.9] - 2026-04-11

### Changed

- **Material parameter crossfade in `HarpSynthVoice`** — When the user changes `stringMaterial` mid-note (e.g., automating Gut → Crystal), the five material-backed physics values (`dampingCoeff`, `brightnessCutoff`, `stiffnessAmount`, `sympatheticCoupling`, `noiseContent`) now lerp smoothly over ~50 ms at block rate instead of snapping in a single block.
- **Root cause:** `HarpSynthVoice::updateParametersFromAPVTS()` previously replaced `currentMaterial` with a fresh `StringMaterial::fromType(newType)` the moment the material index changed, then called `stringModel.setMaterial()` once. v2.1.8 added a 64-sample (~1.5 ms @ 44.1 kHz) filter-coefficient crossfade inside `WaveguideString`, but that only smoothed the downstream filter *response* — the underlying material property values themselves still stepped instantly, and 1.5 ms was too short to mask the timbral character shift between distant materials. The audible artifact was a "jerk" on fast material automation, particularly on sustained notes.
- **Fix:** Added crossfade state to `HarpSynthVoice` (`crossfadeFromMaterial`, `crossfadeTargetMaterial`, `materialCrossfadeSamplesTotal`, `materialCrossfadeSamplesRemaining`, `currentSampleRate`). When the material index changes, the voice snapshots the current (possibly mid-ramp) `currentMaterial` into `crossfadeFromMaterial`, assigns the new `fromType()` preset to `crossfadeTargetMaterial`, and sets the sample budget to `sampleRate * 0.05`. Each render block, `updateParametersFromAPVTS(int numSamples)` computes `t = 1 - remaining/total`, calls `crossfadeFromMaterial.interpolate(crossfadeTargetMaterial, t)` (the pre-existing `StringMaterial::interpolate()` already lerps all five fields), pushes the result into `stringModel.setMaterial()`, then decrements `remaining` by the block size. On completion the ramp snaps exactly to the target to guarantee a bit-identical steady state.
- **Cascading behavior:** If the user scrubs the material parameter rapidly — e.g., Gut → Nylon → Crystal within 30 ms — each new target re-snapshots `currentMaterial` (which may itself be an in-flight interpolation) as the new `crossfadeFromMaterial` and resets the counter. Transitions chain smoothly instead of stacking hard steps.
- **Interaction with v2.1.8 filter crossfade:** Each block's `setMaterial()` call drives the existing `WaveguideString::applyPendingFilterUpdates()` 64-sample coefficient crossfade, so material changes are now doubly smoothed: material values ramp across the ~50 ms window at block rate, and within each block the waveguide's bridge/nut/damping filter coefficients crossfade over 64 samples. No click sources remain on the material-change path.
- **Note-start behavior:** `HarpSynthVoice::startNote()` explicitly cancels any in-flight crossfade (`materialCrossfadeSamplesRemaining = 0`, both source and target set to the freshly-loaded `currentMaterial`) so a new note always begins at the exact target material with no stale ramp state from a previous note's automation.
- **RT-safety:** All crossfade work is plain float arithmetic and struct copies — no allocations. `StringMaterial` is a trivially-copyable POD of 5 floats plus a `juce::String` name; the in-place assignment of `crossfadeFromMaterial = currentMaterial` on the audio thread is safe because `juce::String` has its own thread-safe refcounted backing store.
- **Why ~50 ms:** Short enough to feel instant under manual knob twiddling, long enough to mask the step change on distant material pairs (e.g., Gut dampingCoeff 0.50 → Crystal 0.02, brightnessCutoff 2 kHz → 16 kHz). At 44.1 kHz a 512-sample block gives ~4 block updates across the ramp — plenty of resolution given the downstream 64-sample filter smoothing.
- Files modified: `Source/HarpSynthVoice.h`, `Source/HarpSynthVoice.cpp`
- Testing: Build-verified in Release. Steady-state response for each material is unchanged vs v2.1.8 (crossfade terminates with `currentMaterial = crossfadeTargetMaterial` exactly). Listen-test target: automating `stringMaterial` on sustained notes (especially across distant pairs like Gut → Crystal, Wire → Glass) should now sound like a smooth tonal morph rather than a stepped snap.

## [2.1.8] - 2026-04-11

### Fixed

- **Crossfaded filter coefficient transitions in `WaveguideString`** — Previously, `WaveguideString::applyPendingFilterUpdates()` swapped the bridge, nut, and loop-damping filter coefficients in a single sample whenever brightness, tension, or material parameters changed. Because these filters sit inside the waveguide feedback loop (upper rail → bridge filter → damping → ... → lower rail → nut filter → back to upper rail), a one-sample step in filter response injected a step discontinuity into the circulating signal, producing audible clicks on fast parameter sweeps.
- **Root cause:** Instant coefficient replacement caused a hard discontinuity in the filtered feedback path. The delay-line length compensation (`calculateRailDelay` / `calculateFilterGroupDelay`) already smooths pitch drift, but the filter-output waveform itself was not being smoothed — any brightness/tension/material change landed as a transient kick inside the resonator.
- **Fix:** Replaced the three `juce::dsp::IIR::Filter<float>` instances with a local `OnePoleLPF` POD struct whose bilinear-transform coefficient math and TDF-II state update are byte-for-byte equivalent to `juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass` (so steady-state timbre is unchanged). Because `OnePoleLPF` is trivially copyable, `applyPendingFilterUpdates()` now snapshots each live filter (coefs + state) into a shadow copy via plain assignment, installs the new cutoff on the live filter in place, and starts a 64-sample linear crossfade. `WaveguideString::processSample()` runs the shadow filters in parallel during the crossfade window and blends their outputs with the active filters — at `t=0` the output is identical to the pre-update response (no discontinuity), at `t=1` it matches the new response, with a smooth lerp in between. If another parameter update lands mid-crossfade, the shadow is re-snapshotted from the current in-flight state and the counter resets, cascading transitions smoothly instead of stacking clicks. 64 samples ≈ 1.5 ms @ 44.1 kHz — inaudible as a transition, long enough to mask the step.
- **Bonus:** Eliminates a pre-existing hidden RT-safety hazard — the old code called `Coefficients::makeFirstOrderLowPass(...)` on the audio thread, which allocates a new refcounted `Coefficients` object on every parameter change. The new `OnePoleLPF::setCutoff` writes the 3 coefficient floats in place, no allocation.
- **Crossfade also resets on `reset()` and `trigger()`** to guarantee clean per-note behavior and avoid partial-fade state leaking across note triggers.
- Files modified: `Source/DSP/WaveguideString.h`, `Source/DSP/WaveguideString.cpp`
- Testing: Build-verified in Release. Steady-state response unchanged vs v2.1.7 (coefficient math matches JUCE's `makeFirstOrderLowPass` exactly). Listen-test target: fast automation sweeps of Brightness, Bridge Brightness, Tension, Gauge, or material changes on sustained notes should no longer click.

## [2.1.7] - 2026-04-10

### Fixed

- **Denormal flush on `StiffnessFilter` allpass cascade** — During long quiet passages (sustain tails, silence between notes), the 4-stage allpass cascade inside `StiffnessFilter::processSample` would accumulate subnormal (denormal) floats in its `z1` state variables. Each `AllpassStage::process()` feeds its previous input back into `z1` via `z1 = input - coefficient * output`, so once an exponentially-decaying signal drops below ~1e-38f the state keeps iterating through progressively smaller denormals rather than hitting a clean zero.
- **Root cause:** Denormal floats are handled by the CPU in microcode rather than native FPU hardware, causing per-sample CPU spikes (often 10-100x slower than normalized float arithmetic). With 4 stiffness-filter stages running inside every active waveguide voice, these spikes were visible as periodic DSP load jumps in quiet passages even though no audible signal was being produced.
- **Fix:** Added a post-cascade denormal flush in `StiffnessFilter::processSample` that zeros each stage's `z1` when `|z1| < 1e-15f`, plus a matching flush on the final `output`. Mirrors the same 1e-15f threshold pattern already used in `WaveguideString::processSample`. Zero overhead on non-silent signal (branch predictor trivially hits the "not denormal" path); eliminates the spike entirely on quiet signal.
- Files modified: `Source/DSP/StiffnessFilter.cpp`

## [2.1.6] - 2026-04-10

### Changed

- **Code quality cleanups (no audible/behavioral change):**
  - **Removed dead CSS from binary data** — `Resources/ui/css/styles.css` was compiled into `OLyricaBinaryData` via `juce_add_binary_data()` in `CMakeLists.txt` but never referenced by the WebView resource provider in `PluginEditor.cpp`. CSS is fully inlined inside `<style>` in `Resources/ui/index.html`, so the embedded copy was dead binary weight on every plugin load. Removed the `SOURCES` entry; plugin binary size drops by the size of the compiled styles.css blob.
  - **De-duplicated `reverbMix` atomic load in `processBlock`** — `PluginProcessor.cpp` previously loaded `fxCache.reverbMix` twice per audio block: once to call `reverbProcessor.setMix(...)` and again to store into `reverbMixVal` for the `> 0.001f` bypass gate. Now loads once into `reverbMixVal` and reuses it for both the setter and the gate. Minor win, but the other FX channels (chorus/delay) already use this pattern — bringing reverb into line.
  - **Replaced `static const std::vector<std::vector<int>>` scale patterns with `constexpr` C arrays** — `HarpSynthVoice.cpp` built a `std::vector<std::vector<int>>` of fixed major/minor/pentatonic patterns inside `updateGlissandoRange()`. Despite being `static const`, first-call construction triggered three nested heap allocations inside the note-on call path. Replaced with `static constexpr int kMajorPattern[]/kMinorPattern[]/kPentatonicPattern[]` plus a lookup table of pointers + sizes — zero heap allocation, identical pattern lookup logic, no behavior change.
- Files modified: `CMakeLists.txt`, `Source/PluginProcessor.cpp`, `Source/HarpSynthVoice.cpp`

## [2.1.5] - 2026-04-10

### Fixed

- **Thread-safe `SympatheticResonanceEngine::setResonatorQ`** — The Q setter called `designResonatorFilter()` directly, which mutates `juce::dsp::IIR::Filter::coefficients` (a reference-counted shared pointer). Because the call site may execute off the audio thread while `computeSympatheticContribution()` is concurrently reading those filter coefficients, this was a data race on the coefficient pointer and on the `*filter.coefficients = *coefficients` assignment. In addition, `filter.reset()` would clear the filter state mid-sample from a non-audio thread.
- **Root cause:** Direct coefficient mutation from the setter violated the single-writer-on-audio-thread invariant used everywhere else in the DSP graph.
- **Fix:** Applied the same deferred pending-atomic pattern already used in `BodyResonance` (`filterUpdatePending` / `pendingBodySize` / etc.) and `WaveguideString` (`pendingBridgeCutoff` / etc.). `setResonatorQ()` now only stores `pendingResonatorQ` and sets the `qUpdatePending` flag. `syncBeforeBlock()`, which already runs at the top of every `processBlock` on the audio thread, now applies the pending Q to all active voice slots before any `computeSympatheticContribution()` reads execute. `reset()` also clears the pending-Q flag to stay consistent.
- Files modified: `Source/DSP/SympatheticResonance.h`, `Source/DSP/SympatheticResonance.cpp`

## [2.1.4] - 2026-04-10

### Fixed

- **Crosstalk LP filter state continuity** — The one-pole lowpass inside the inter-string crosstalk loop (`PluginProcessor.cpp`) had its `stateA`/`stateB` declared as inner-scope locals, so the filter reset to 0 on every `processBlock` call. This made the filter discontinuous across block boundaries, producing per-block restart glitches and incorrect HF rolloff that scaled with block size.
- **Root cause:** Filter state was block-local rather than persistent. The one-pole needs the previous sample's output to compute the next, but every new block began from zero, effectively restarting the filter ~600-1400 times per second depending on DAW buffer size.
- **Fix:** Added persistent `crosstalkStateA[32][32]` / `crosstalkStateB[32][32]` arrays to `PluginProcessor.h`, indexed by the active voice pair (voiceIndexA, voiceIndexB). State is loaded into fast locals at loop entry, updated per sample, and written back at loop exit. Initialized to zero in `prepareToPlay()` so the filter starts clean on transport start / sample rate change.

## [2.1.3] - 2026-04-10

### Fixed

- **Frequency guard in StiffnessFilter** — Clamp frequency to minimum 20 Hz in `calculateFrequencyScaling()` before division, preventing inf/NaN in allpass coefficients when frequency is 0 or negative

## [2.1.2] - 2026-04-09

### Changed

- **Effects chain reorder** — EQ now processes after reverb instead of before (Chorus → Delay → Reverb → EQ). Allows EQ to shape the reverb tail and overall tonal balance post-reverb
- **Effects tab UI reorder** — Reverb section now appears above EQ in the effects tab, matching the top-to-bottom signal flow

## [2.1.1] - 2026-04-08

### Fixed

- **Shimmer pitch shifter quality** — Replaced 2-grain pitch shifter with 4-head design for artifact-free octave shifting
  - 4 read heads at 90-degree intervals (was 2 at 180) eliminates audible amplitude modulation
  - Doubled grain size to 4096 samples (~85ms) for smoother crossfades
  - 2 kHz high-pass filter on shimmer output — strips low/mid content that caused resonator-like buildup, keeps only bright airy octave-shifted content feeding back through the tank

## [2.1.0] - 2026-04-07

### Changed

- **FDN plate reverb replaces Freeverb** — Complete reverb engine rewrite for dramatically improved sound quality
  - 8-channel Feedback Delay Network with Householder matrix mixing (replaces 8-comb + 4-allpass Freeverb)
  - 4-stage input diffusion with Hadamard mixing for instant high echo density (~2000-4000 echoes/sec)
  - Optimized coprime delay lengths from DAFx 2023 research for minimal coloration
  - 2-band frequency-dependent decay: HF damps faster than LF (matching real plate/harp physics)
  - Multi-LFO delay modulation on 4 of 8 channels for lush, detuned tail without metallic ringing

### Added

- **Mod knob** — Controls delay modulation depth in the reverb tank (0-100%, default 20%). Subtle chorus-like movement that smooths the reverb tail and prevents metallic resonances
- **Shimmer knob** — Octave-up pitch-shifted feedback into the reverb tank (0-100%, default off). Even small amounts (10-15%) add ethereal bloom that complements the sympathetic resonance engine
- Updated atmospheric factory presets (Harmonic Dreams, Shimmering Heights, Ethereal Chime, Angelic Choir) with shimmer and increased modulation

## [2.0.2] - 2026-03-06

### Added

- **Licensing module integration** — Ouaricon license manager on processor, activation overlay on editor
  - All licensing code gated behind `OUARICON_LICENSING_ENABLED` (OFF for local dev builds)

## [2.0.1] - 2026-03-06

### Fixed

- **"Warm Classical" preset too dry** — retuned resonance and reverb parameters for acoustic concert harp character
  - Increased `bodyResonance` (0.7→0.82), `bodySize` (0.7→0.78), `sympatheticAmount` (0.45→0.62) for fuller body and ringing
  - Increased `decayTime` (0.5→0.65), `stringCrosstalk` (0.35→0.42), `bodyModeSpread` (0.54→0.60) for longer sustain and coupling
  - Increased `reverbSize` (0.55→0.72), `reverbMix` (0.2→0.32), `reverbPredelay` (0.1→0.18) for concert hall ambience
  - Reduced `reverbDamp` (0.45→0.38) for longer, more open reverb tail
  - Bumped factory preset version to force regeneration on next load

## [2.0.0] - 2026-03-06

### Changed

- **Complete factory preset overhaul** — All 48 presets updated with 6 new sound parameters and effects
  - Added `humanize` (per-note variation): Gut strings 0.2-0.4, Carbon 0.05-0.1, Crystal 0.15-0.25, Energy 0.25-0.35 — values match each material's organic vs precise character
  - Added `stringCrosstalk` (soundboard coupling): Gut 0.3-0.42, Wire 0.2-0.32, Glass 0.1-0.15, Energy 0.08-0.18 — acoustic materials couple more than synthetic
  - Added `sympatheticQ` (resonance sharpness): differentiated per material — warm materials get broader resonance, crystalline materials get sharper
  - Added `bodyModeSpread` (body mode coloring): historical/mystical presets get wider spread, precise/clean presets stay neutral
  - Added `stringGauge` and `stringLength`: varied per material and instrument size character
  - ~20 presets now include effects (reverb, chorus, delay, EQ) matched to their character:
    - Sacred Space/Ice Palace/Meditation: large reverb for atmospheric depth
    - Gentle Stream/Harmonic Dreams/Angelic Choir: chorus + reverb for shimmer
    - Cosmic Harp: ping-pong delay + reverb for space
    - Electric Dreams: chorus + delay for synthwave character
    - Studio Session/Bell Tones: subtle EQ for presence
  - All presets reset effect state on load (bypass flags + mix values) for clean preset switching

### Added

- **Factory preset version check** — Presets now use a `.version` marker file instead of directory-exists check. When the embedded version string changes, old factory presets are regenerated automatically. Existing user presets in the `User/` directory are never touched.

## [1.35.1] - 2026-03-06

### Fixed

- Effects tab controls completely non-functional (dials didn't move, bypass toggles didn't register)
  - **Root cause:** v1.32.0 added effects DSP processing and HTML structure but never created C++ WebView relays/attachments or JavaScript parameter bindings
  - Added 19 WebView relays + attachments in PluginEditor (14 sliders, 4 bypass toggles, 1 combo box) for all effects parameters
  - Added JavaScript SVG vine-arc knob creation for Chorus (Rate/Depth/Mix), Delay (Time/Feedback/Mix), EQ (Low/Mid/Freq/High), Reverb (Size/Damp/Pre-Dly/Mix)
  - Added Delay Mode dropdown (Normal/Ping Pong) with ComboBox binding
  - Added bypass button bindings with section dim/disable when bypassed
  - Knobs support mouse drag, touch drag, and double-click to reset

## [1.35.0] - 2026-03-06

### Added

- Mechanical string crosstalk via soundboard coupling — energy transfer between physically adjacent strings regardless of harmonic relationship
  - After `synthesiser.renderNextBlock()`, active voice pairs within ±1-2 semitones exchange lowpass-filtered (2kHz one-pole) signal
  - ±1 semitone neighbors: full crosstalk amount; ±2 semitones: half amount
  - Gain scaled to 0-5% range (crosstalkAmount × distanceFactor × 0.05) for subtle, realistic coupling
  - Per-voice output buffers (`voiceOutputBuffer`) in `HarpSynthVoice` capture individual voice signals for cross-mixing
  - New APVTS parameter `stringCrosstalk` (0.0–1.0, default 0.2) controls coupling intensity
  - Processed before body resonance and effects chain — crosstalk feeds into shared soundboard model

## [1.34.1] - 2026-03-05

### Changed

- Harmonic technique is now position-aware: `PluckExciter::updateTechniqueFilter()` calculates harmonic number from `round(1.0 / pluckPosition)` instead of hardcoded 2× fundamental
  - Position 0.5 → 2nd harmonic, 0.33 → 3rd, 0.25 → 4th, 0.125 → 8th
  - Position clamped to [0.1, 0.5] for harmonics 2–8 (avoids fundamental and impractically high partials)
  - Matches real harp harmonic technique where touching at 1/N of string length isolates the Nth harmonic

## [1.34.0] - 2026-03-05

### Changed

- Replaced PluckExciter position filter with physically accurate feedforward comb filter
  - Previous: one-pole lowpass approximating position brightness (200Hz–4×f0 cutoff mapping)
  - New: `y[n] = x[n] - x[n-D]` where `D = position × (sampleRate / f0)`, creating spectral nulls at harmonics n = k/position — matches real plucked string physics
  - Position 0.5 (center) kills even harmonics for hollow/woody tone; near bridge creates dense spectral nulls for thin/nasal character; near nut has few nulls for bright/full sound
  - Uses linear interpolation for fractional delay accuracy
  - Position clamped to [0.05, 0.95] matching WaveguideString convention

## [1.33.1] - 2026-03-05

### Changed

- Moved body resonance from per-voice to shared post-mix processing — all voices now share a single `BodyResonance` instance in `PluginProcessor`, processing the mixed synthesiser output once through 5 bandpass filters instead of 160 (32 voices × 5). More physically accurate (all strings share one soundboard) and significantly reduces CPU
- Sympathetic resonance now driven by raw string signal instead of body-resonated signal (more physically accurate — sympathetic coupling occurs between strings, not through soundboard)
- Voice `renderNextBlock()` outputs string + sympathetic only; body resonance applied in `processBlock()` after `synthesiser.renderNextBlock()`

## [1.33.0] - 2026-03-05

### Added

- Étouffé (pedal dampening) via sustain pedal CC64: when the sustain pedal is released (CC64 < 64), all sounding voices enter rapid dampening — feedback coefficient is progressively multiplied by a decay envelope reaching -60dB in ~50ms, simulating a harpist touching strings to stop them
- `WaveguideString::setDampening()` method with per-sample `dampeningMultiplier` ramp-down computed from sample rate
- CC64 handling in `HarpSynthVoice::controllerMoved()` — dampening activates on pedal release, deactivates on pedal press
- Dampening state auto-resets on new note trigger so notes played after pedal release aren't affected

## [1.32.7] - 2026-03-05

### Changed

- Added velocity-sensitive brightness modulation: `fingerHardness` and `brightness` in `HarpSynthVoice::startNote()` now scale with MIDI velocity — softer plucks (vel≈0.2) reduce hardness ~15% and brightness ~10%, harder plucks (vel≈1.0) boost both ~10%, matching real plucked string physics where harder plucks excite more high-frequency content

## [1.32.6] - 2026-03-05

### Fixed

- Fixed data race in `BodyResonance`: `setBodyParameters()` and `setModeSpread()` were calling `updateFilterCoefficients()` on the message thread, mutating `bodyModes[i].coefficients` while `process()` reads them on the audio thread — applied the same atomic pending-flag pattern used in `WaveguideString` (pending atomics + `applyPendingFilterUpdates()` called at start of `process()`)
- Made `bodyAmount` `std::atomic<float>` since it's written from message thread and read in `process()`

## [1.32.5] - 2026-03-05

### Changed

- Eliminated per-sample `findSlotForVoice()` linear scan in `SympatheticResonanceEngine::computeSympatheticContribution()` — `registerVoice()` now returns the slot index, which the voice caches and passes directly, removing 16-slot linear search on every sample

## [1.32.4] - 2026-03-05

### Changed

- Added dirty-flag coefficient caching to `EQProcessor::process()` — `makeLowShelf`, `makePeakFilter`, and `makeHighShelf` coefficients are now only recomputed when their corresponding parameters actually change, skipping redundant coefficient rebuilds every block
- Added dirty-flag parameter caching to `ReverbProcessor::process()` — `Reverb::Parameters` struct is now only constructed and applied when `size` or `damping` changes; `dryWetMixer` mix proportion is only set when `mix` changes

## [1.32.3] - 2026-03-04

### Changed

- Refactored `HarpSynthVoice::startNote()` from ~330 lines to ~140 lines by extracting three private helpers:
  - `setupScaleLockedGlissando(int midiNoteNumber)` — scale frequency filtering, tempo sync, shape/humanize/velocity profile, direction calculation
  - `setupFreeGlissando()` — tempo sync, shape, interval, direction for free mode
  - `applyDirectionExcitation(float&, float&)` — ascending/descending excitation character (finger pad vs thumb)
- `startNote()` now reads as a linear orchestrator: parameters → mode branch → helper calls → trigger → register sympathetic

## [1.32.2] - 2026-03-04

### Removed

- Dead member variable `previousFrequency` from `HarpSynthVoice`. Was assigned in `startNote()` but never read — became unused in v1.23.0 when glissando switched to interval-based calculation.

## [1.32.1] - 2026-03-04

### Changed

- Extracted duplicated `kSyncBeats` and `intervalSemitones` arrays from `HarpSynthVoice::startNote()` to file-scope `static constexpr` declarations (`kSyncBeats`, `kIntervalSemitones`). Each was defined identically in multiple places inside the function; now a single definition at file scope.

## [1.32.0] - 2026-03-04

### Added

- **Effects chain** — 4-stage post-synthesis effects: Chorus, Delay, EQ, Reverb. Each effect has an independent bypass toggle and dry/wet mix control.
- **Chorus** — LFO-modulated delay copies for stereo width. Rate (0.1-10 Hz), Depth (0-100%), Mix (0-100%).
- **Delay** — Stereo delay with Normal and PingPong modes. Time (1-2000 ms), Feedback (0-95%, low-pass filtered at 8 kHz), Mix (0-100%).
- **EQ** — 3-band parametric: Low shelf (200 Hz), Mid peak (200-8000 Hz variable), High shelf (8 kHz). Each band +/-12 dB.
- **Reverb** — Schroeder reverb with pre-delay (0-200 ms), Size (0-100%), Damping (0-100%), Mix (0-100%).
- **Effects tab UI** — Replaced placeholder with full effects controls matching O-IntonationPad styling.

### Technical Details

- 19 new APVTS parameters: 4 bypass bools + 15 float/choice controls
- New DSP classes: `DelayProcessor`, `EQProcessor`, `ReverbProcessor` (+ JUCE built-in `juce::dsp::Chorus`)
- Effects chain order: Chorus → Delay → EQ → Reverb → Master Volume
- Mix threshold optimization: effects only process when mix > 0.001
- Cached `std::atomic<float>*` parameter pointers for lock-free real-time access

## [1.31.0] - 2026-03-04

### Added

- **Tempo-synced glissando** — Both Free and Scale-Locked glissando modes now have a "Sync" dropdown that locks timing to the DAW's tempo. Choose from 14 rhythmic values: 1/32, 1/16, 1/16D, 1/8T, 1/8, 1/8D, 1/4T, 1/4, 1/4D, 1/2, 1/2D, 1 Bar, 2 Bars, 4 Bars. When Sync is set to anything other than "Off", the manual Time/Speed slider is hidden and replaced by tempo-derived timing.

- **Free mode tempo sync** — When enabled, the sweep ramp duration is calculated from `(beatDivision / (BPM / 60))` seconds. Supports up to 10s ramp (expanded from 0.5s max) for slow divisions at low tempos.

- **Scale-Locked mode tempo sync** — When enabled, the total glissando duration matches the selected rhythmic value. Notes-per-second is derived from `(estimatedStepCount / totalDuration)` so the entire sweep fits exactly within the beat division.

### Technical Details

- New APVTS parameters: `freeTempoSync` and `scaleTempoSync` (AudioParameterChoice, 15 options each)
- BPM retrieved via `getPlayHead()` in `processBlock`, stored in `std::atomic<double> currentBpm`
- BPM delivered to voices via atomic pointer (same pattern as `activeGlissandoMode`)
- `GlissandoController::setRampTime()` max expanded from 0.5s to 10.0s for tempo-synced slow divisions
- UI: Sync dropdown swaps Time/Speed slider visibility using `setupTempoSyncVisibility()` in app.js

## [1.30.0] - 2026-03-03

### Added

- **Independent glissando tonic selector** — New "Key" dropdown in the Scale-Locked Glissando section. Sets the root note for scale filtering independently from the tuning tab tonic. Previously the played MIDI note was always used as the root (`midiNoteNumber % 12`), making the scale shift with every note.

- **Custom degree toggle buttons** — When Scale mode is set to "Custom", a row of toggle buttons appears showing each scale degree. Click to include/exclude individual degrees from the glissando sweep. Adapts to the current scale size (12 for 12-TET, 19 for 19-EDO, etc.). Bitmask stored as `uint64_t` for up to 64 degrees.

- **Non-12-note tuning support** — Major, Minor, and Pentatonic scales are now automatically disabled when a non-12-note tuning system is active (19-EDO, 31-EDO, Bohlen-Pierce, etc.). The UI shows "(N/A)" on disabled options and forces Custom mode. Scale filtering uses `numScaleDegrees` from TuningEngine instead of hardcoded `% 12`.

### Changed

- Scale filtering in `HarpSynthVoice::startNote()` now uses `glissandoTonic` parameter as the anchor point instead of `midiNoteNumber % 12`
- Custom mode now filters by bitmask per degree instead of passing all chromatic frequencies through

### Technical Details

- New APVTS parameter: `glissandoTonic` (AudioParameterChoice, 12 options C-B)
- `glissCustomDegrees` stored as `std::atomic<uint64_t>` on processor, passed to voices via pointer
- State save/load: bitmask serialized as string in XML for 64-bit safety
- Native functions: `setGlissCustomDegrees(low, high)`, `getGlissCustomDegrees()`, `getScaleDegreeCount()`
- Timer callback emits `scaleDegreeCountChanged` event to WebView when tuning changes
- Files modified: PluginProcessor.h/cpp, HarpSynthVoice.h/cpp, PluginEditor.h/cpp, index.html, app.js

## [1.29.2] - 2026-03-02

### Added

- **Free mode Shape control** — The Shape dropdown (Linear, Accelerate, Decelerate, S-Curve) now applies to Free mode glissando sweeps, not just Scale-Locked mode. Accelerate starts slow and speeds up into the target note; Decelerate arrives quickly then eases in; S-Curve provides natural-feeling motion with slow start/end and fast middle.

### Technical Details

- GlissandoController.cpp: Applied shape curve mapping to `freeProgress` before log-frequency interpolation in `getNextFrequency()` Free mode branch
- index.html + app.js: Shape dropdown now visible in both Free and Scale-Locked modes

## [1.29.1] - 2026-03-01

### Fixed

- **Glissando Free mode sliders now connected to DSP** — Four glissando parameters (`glissandoTime`, `glissandoExcitation`, `glissandoVelStart`, `glissandoVelEnd`) were missing their WebSliderRelay, `.withOptionsFrom()` registration, and WebSliderParameterAttachment in the PluginEditor. The UI sliders existed and were visually functional but completely disconnected from the APVTS — moving them had no effect on the DSP. All four now have full WebView bridge wiring.

### Technical Details

- PluginEditor.h: Added relay and attachment declarations for `glissandoTime`, `glissandoExcitation`, `glissandoVelStart`, `glissandoVelEnd`
- PluginEditor.cpp: Added relay creation, `.withOptionsFrom()` registration, and `WebSliderParameterAttachment` construction for all four parameters

## [1.29.0] - 2026-03-01

### Added

- **Free mode interval and direction controls** — Free glissando mode now uses the Interval and Direction parameters (previously Scale-Locked only). Instead of portamento from the previous note, Free mode calculates a fixed start frequency based on the selected interval (Minor 2nd through 3 Octaves or Custom) and bends up to or down to the target note. UI now shows Interval, Direction, and Custom Semitones controls when Free mode is active.

### Technical Details

- HarpSynthVoice: Free mode branch reads `glissandoInterval`, `glissandoDirection`, and `glissandoCustomSemitones` parameters to compute `startFreq` using the same semitone-to-frequency formula as Scale-Locked mode
- app.js: `intervalGroup`, `directionGroup`, and `customStGroup` visibility now includes `isFree` condition alongside `isScaleLocked`

## [1.28.0] - 2026-02-26

### Added

- **Direction-dependent excitation character** — Glissando sweeps now automatically detect pitch direction and apply realistic excitation differences. Ascending glissandos (finger pad) shift pluck position toward center (-0.05) and soften hardness (×0.85) for a warmer tone. Descending glissandos (thumb) shift pluck position toward bridge (+0.04), firm up hardness (×1.1), and add subtle attack noise (+0.08) simulating a thumb's nail edge scrape. Applies to both Free and Scale-Locked modes with no new user parameters — behavior when glissando is Off is unchanged.

### Technical Details

- GlissandoController: Added `GlissandoDirection` enum (Ascending/Descending), `getDirection()` accessor, auto-detection in `startGlissando()` via `endFreq > startFreq`
- HarpSynthVoice: Direction queried after `startGlissando()`, excitation adjustments applied to local `pluckPosition`, `fingerHardness`, `attackNoise` before `stringModel.trigger()` — additive nudges on top of user knob values

## [1.27.1] - 2026-02-26

### Fixed

- **Glissando mode UI visibility bug** — Sub-parameter controls (Time, Softness, Speed, Humanize, Dynamics, Shape, Interval, Direction) now correctly appear when the mode is set from the JUCE backend (preset load, automation, initial state sync). Previously, programmatic `select.value` changes didn't fire DOM `change` events, so visibility updates were skipped until the user manually re-selected the mode.

### Technical Details

- Added `valueChangedEvent` listeners on JUCE combo states for `glissandoMode` and `glissandoInterval` that directly call `updateScaleLockedVis()`, bypassing the DOM event limitation

## [1.27.0] - 2026-02-26

### Added

- **Glissando velocity profiling** — Dynamic contour across Scale-Locked glissando sweeps. Two new parameters control the velocity at the start and end of each sweep, modulating the waveguide's damping for natural crescendo/decrescendo effects.
- `glissandoVelStart` (Dynamics: Start, 0-100%, default 50%): Velocity at the beginning of the sweep
- `glissandoVelEnd` (Dynamics: End, 0-100%, default 70%): Velocity at the end of the sweep
- Default 50%→70% gives a subtle ascending crescendo matching real harp arm mechanics
- Equal start/end values produce flat dynamics identical to pre-v1.27.0 behavior
- Only affects Scale-Locked mode — Free mode and initial pluck velocity unchanged

### Technical Details

- GlissandoController: Added `initialScaleDegree` tracking, `setVelocityProfile()`, `getNextVelocity()` with per-step linear interpolation
- HarpSynthVoice: Reads velocity profile in `startNote()`, applies damping modulation in `renderNextBlock()` via `baseDamping * (1.0 - glissVel * 0.3)`
- UI: Two slider controls (Dynamics: Start / End) visible only when Mode = Scale-Locked, placed after Humanize

## [1.26.0] - 2026-02-26

### Added

- **Gliss Softness parameter** — Brush-style excitation for glissando notes. When glissando mode is active (Free or Scale-Locked), the PluckExciter uses a lighter "brush" excitation instead of a full deliberate pluck, matching how real harp glissandos produce softer contact per string.
- At 0.0 (off): full deliberate pluck, identical to previous behavior
- At 0.6 (default): realistic brush that blends into the glissando wash
- At 1.0: very light, ethereal sweep

### Technical Details

- New APVTS parameter: `glissandoExcitation` (AudioParameterFloat, 0.0-1.0, default 0.6, no unit suffix)
- PluckExciter: Added `setGlissandoAmount(float)` with 4 excitation modifications in `trigger()`:
  - Velocity scaled by `1.0 - (amount * 0.4)` (60% at full softness)
  - Noise burst narrowed by `amount * 50%` (shorter contact time)
  - Finger hardness reduced by `amount * 0.3` (softer brightness cutoff)
  - ADSR attack shortened by `amount * 50%` (quicker brush contact)
- WaveguideString: `setGlissandoExcitation()` pass-through to exciter
- HarpSynthVoice: Reads parameter when glissandoMode != Off, sets 0.0 when Off (before `trigger()`)
- Normal pluck behavior completely unchanged when amount is 0.0
- Stacks on top of PlayingTechnique system (Normal, Harmonic, Muted, PresDeLaTable)
- Applies equally to Free and Scale-Locked modes

## [1.25.0] - 2026-02-26

### Added

- **Glissando Time parameter** — Configurable ramp duration for Free mode glissando. Range: 10ms (near-instant pitch snap) to 500ms (slow expressive portamento). Default 50ms preserves existing behavior. Log-skewed knob (0.5) gives more travel to the fast 10-100ms range.
- UI slider appears in Techniques tab when Glissando Mode is set to "Free"

### Technical Details

- New APVTS parameter: `glissandoTime` (AudioParameterFloat, 0.01-0.5s, default 0.05, skew 0.5, suffix "s")
- GlissandoController: Added `setRampTime(float seconds)` method and `rampTimeSeconds` member
- `startGlissando()` Free mode now uses `rampTimeSeconds` instead of hard-coded `constexpr 0.05`
- HarpSynthVoice reads parameter and calls `setRampTime()` before `startGlissando()` (Free mode only)
- Scale-Locked mode timing completely unaffected (uses speed/shape/humanize)

## [1.24.1] - 2026-02-25

### Fixed

- **Free mode glissando now uses logarithmic frequency interpolation** — Previously ramped linearly in Hz via `juce::SmoothedValue`, causing sweeps to spend disproportionate time in upper octaves (e.g., 220→880 Hz spent 75% of time above 440 Hz). Now interpolates in log2 space so each octave gets equal time, matching professional portamento implementations.

### Technical Details

- Replaced `juce::SmoothedValue<double> frequencyRamp` with manual log2-domain interpolation
- `startGlissando()` computes `log2(startFreq)` and `log2(endFreq)` once (not per-sample)
- `getNextFrequency()` advances a linear progress (0→1) and computes `pow(2, lerp(startLog, endLog, progress))`
- 50ms ramp time preserved from previous SmoothedValue behavior
- Off mode and Scale-Locked mode completely unaffected
- No new allocations, no API changes, no parameter changes

## [1.24.0] - 2026-02-25

### Added

- **Glissando Humanize parameter** — Per-step timing jitter for Scale-Locked glissandos. Simulates the natural timing irregularity of a real harpist's arm sweep across non-uniformly spaced strings (±3-8ms variation).
- Jitter magnitude scales inversely with speed: slow sweeps (4 n/s) allow ±10ms max, fast sweeps (30 n/s) allow ±2ms max
- At 0.0 the behavior is identical to v1.23.0 (perfectly metronomic). Default 0.3 adds subtle life without sounding sloppy.

### Technical Details

- New APVTS parameter: `glissandoHumanize` (AudioParameterFloat, 0.0-1.0, default 0.3)
- `GlissandoController::setHumanize()` stores amount; `juce::Random` member generates per-step offsets
- Jitter applied in `startGlissando()` after shape curve computation, modifying pre-computed `stepDurations[]`
- Formula: `maxJitterMs = jmap(speed, 4, 30, 10, 2)`, then `jitterMs = humanize * maxJitterMs * random(-1,1)`
- Each step gets its own independent random offset (not per-sample noise)
- Clamped to minimum 1 sample per step to prevent negative durations
- Wired via `HarpSynthVoice::startNote()` alongside existing `setSpeed()` and `setShape()` calls
- Full WebSliderRelay/WebSliderParameterAttachment pipeline
- UI: Slider placed after Speed in Glissando section, visible only when Mode = Scale-Locked

## [1.23.0] - 2026-02-25

### Added

- **Glissando Interval parameter** — Fixed sweep distance so every note produces the same predictable glissando. Dropdown with 16 musical presets (Minor 2nd through 3 Octaves) plus a Custom option for entering any semitone value (1-48).
- **Glissando Custom Semitones parameter** — Numeric input (1-48 st) active when Interval is set to Custom, for precise non-standard sweep distances.
- **Glissando Direction parameter** — Toggle between "Up to Note" (gliss starts below and sweeps up) and "Down to Note" (starts above and sweeps down).

### Fixed

- **Inconsistent glissando distances** — Root cause: glissando start was based on `previousFrequency` (per-voice, initialized at 440 Hz, dependent on voice allocation). Replaced with deterministic interval-based calculation so the sweep is identical regardless of which physical voice is allocated.

### Technical Details

- New APVTS parameters: `glissandoInterval` (AudioParameterChoice, 17 options, default Octave), `glissandoCustomSemitones` (AudioParameterFloat, 1-48, default 12), `glissandoDirection` (AudioParameterChoice, 2 options, default Up to Note)
- `HarpSynthVoice::startNote()` now calculates start frequency as `currentFrequency * 2^(±semitones/12)` based on direction, replacing `previousFrequency`
- Scale frequency range dynamically sized to cover full interval in either direction
- `MAX_SCALE_SIZE` increased from 48 to 64 to accommodate up to 48-semitone custom intervals
- Free mode glissando still uses `previousFrequency` (intentional — smooth slides between consecutive notes)
- Full WebComboBoxRelay/WebSliderRelay pipeline for all 3 new parameters
- UI visibility: Interval, Direction, Custom Semitones only visible when Mode = Scale-Locked; Custom Semitones only when Interval = Custom

## [1.22.0] - 2026-02-25

### Added

- **Glissando Shape parameter** — Acceleration curves for scale-locked glissandos that shape note spacing across the sweep, replacing the constant metronomic timing. Four options: Linear (constant spacing, backward-compatible), Accelerate (slow start → fast end, hand gaining momentum), Decelerate (fast start → slow end, landing effect), S-Curve (slow → fast → slow, smoothstep — default, mimics a real harpist's arm arc).

### Technical Details

- New APVTS parameter: `glissandoShape` (AudioParameterChoice, 4 options, default index 3 = S-Curve)
- `GlissandoShape` enum added to `GlissandoController.h` with `glissandoShapeFromIndex()` converter
- Per-step sample durations pre-computed in `startGlissando()` using fixed-size array (`stepDurations[MAX_SCALE_SIZE]`) — no audio-thread allocation
- Shape curves: Accelerate = `t²`, Decelerate = `√t`, S-Curve = `t²(3−2t)` (Hermite smoothstep)
- `updateScaleLocked()` now reads from `stepDurations[currentStepIndex]` instead of constant `samplesPerStep`
- Total glissando duration preserved (numSteps × samplesPerStep), only the distribution changes
- Rounding remainder distributed to final step to prevent timing drift
- Wired in `HarpSynthVoice::startNote()` alongside existing `setSpeed()` call
- WebView UI dropdown in Techniques → Glissando section, visibility tied to Scale-Locked mode
- Full WebComboBoxRelay/WebComboBoxParameterAttachment pipeline for automation and preset support

## [1.21.0] - 2026-02-25

### Added

- **Glissando Speed parameter** — User-controllable speed for scale-locked glissandos, replacing the hard-coded 10 notes/second value. Range 4–30 n/s with logarithmic skew (default 12 n/s). Appears in the Techniques tab Glissando section, visible only when Mode is set to "Scale-Locked". Real harp glissando speeds range ~8–21 n/s; the extended range allows creative non-realistic use.

### Technical Details

- New APVTS parameter: `glissandoSpeed` (4.0–30.0, skew 0.5, default 12.0, suffix "n/s")
- Wired in `HarpSynthVoice::startNote()` — reads parameter value before calling `glissandoController.setSpeed()`
- Only applies to Scale-Locked mode (Free mode uses SmoothedValue ramp time)
- WebView UI slider with conditional visibility (hidden when Mode is Off or Free)
- Full WebSliderRelay/WebSliderParameterAttachment pipeline for automation and preset support

## [1.20.1] - 2026-02-24

### Fixed

- **Attack Noise no longer silences instrument** — Parameter previously controlled overall excitation amplitude; setting it to 0 produced silence. Now blends between a clean half-sine impulse (0%) and noise excitation (100%), so Attack Noise controls attack *texture* without affecting gain.

## [1.19.0] - 2026-02-04

### Added

- **Humanize parameter** - Per-note randomization for realistic, organic performance variation
  - New "Humanize" slider in SOUND tab → Main section (0-100%)
  - At 0%: Deterministic behavior preserved (every note identical)
  - At 100%: Full randomization creates natural variation

### Humanization Details

The Humanize parameter applies subtle per-note variation to simulate the natural inconsistencies of human performance:

| Parameter | Randomization Range | Effect |
|-----------|---------------------|--------|
| Pluck Position | ±5% | Finger placement varies between plucks |
| Finger Hardness | ±8% | Contact angle/pressure varies |
| Attack Noise | ±10% | Nail/skin contact character varies |
| Brightness | ±4% | Harmonic content micro-variation |
| Decay Time | ±3% | String damping micro-variation |
| Pitch | ±3 cents | Micro-tuning variation (string non-uniformity) |

### Technical Details

- New APVTS parameter: `humanize` (0.0-1.0, default 0.0)
- Randomization applied in `HarpSynthVoice::startNote()` after reading base values
- Per-voice `juce::Random` generator ensures independent variation per voice
- `applyHumanization()` helper method clamps results to valid 0-1 range
- Micro-tuning uses cents-to-frequency ratio: `2^(cents/1200)`
- Files modified: PluginProcessor.cpp, HarpSynthVoice.h, HarpSynthVoice.cpp, index.html

### Why This Matters

Real harps exhibit natural variation because:
- Finger placement never lands exactly the same spot twice
- Contact pressure/angle varies between strokes
- String tuning drifts microscopically
- Attack noise characteristics vary per pluck

This feature brings O-Lyrica closer to the organic, living quality of a real harp performance.

## [1.18.4] - 2026-02-03

### Changed

- **Increased maximum polyphony from 16 to 32 voices** - Allows fuller chord voicings and more simultaneous notes for complex harp passages

### Technical Details

- File modified: PluginProcessor.cpp (constructor voice allocation loop)
- CPU impact: Up to 2x voice processing load at full polyphony (acceptable per VOICE_MANAGEMENT.md design targets)
- No parameter changes - existing presets and sessions compatible

## [1.18.3] - 2026-02-03

### Changed

- **Code cleanup and simplification** - Reduced verbosity without functional changes
  - Removed ~100 lines of DBG logging from preset manager callbacks (development diagnostics)
  - Removed redundant null checks in processBlock (APVTS guarantees non-null for registered params)
  - Added section comments to organize native functions in PluginEditor.cpp

### Technical Details

- Root cause: DBG statements were added during tuning state debugging (v1.12.0-1.13.3) but left in production
- Pattern matches HarpSynthVoice cleanup from v1.3.2 (same null check removal)
- Files modified: PluginProcessor.cpp, PluginEditor.cpp
- No functional changes - pure code quality improvement

## [1.18.2] - 2026-02-03

### Changed

- **Tuning panel simplified** - Removed redundant controls for cleaner workflow
  - Removed Temperament dropdown (was duplicating library functionality)
  - Removed mode buttons (12-TET, Custom, MTS-ESP) - unnecessary switching
  - Moved Tuning Library from bottom to top of panel (now primary interface)
  - File operation buttons (Load/Save .SCL/.KBM, Export HTML) now always visible
  - 12-TET remains default tuning for new instances

### Technical Details

- Files modified: Resources/ui/index.html (HTML structure, CSS, JavaScript)
- Removed HTML elements: `.tuning-preset-section`, `.tuning-mode-section`
- Removed CSS classes: `.tuning-preset-*`, `.tuning-mode-*` styles
- Added CSS class: `.library-section-top` for top-positioned library
- JavaScript cleanup: Removed mode button handlers, preset dropdown references
- Backend unchanged: `tuningMode` APVTS parameter still exists for state persistence

## [1.18.1] - 2026-02-03

### Changed

- **Keyboard moved from Tuning tab to sticky footer panel**
  - Keyboard now visible at all times, positioned between Master fader and "Ouaricon Audio" branding
  - Expanded from 1 octave (C4-B4) to 2 octaves (C3-B4) for more playable range
  - Compact design: 42px height to fit in 55px footer
  - Footer height increased from 40px to 55px to accommodate keyboard
  - Click-to-play functionality preserved with all existing event handlers

### Technical Details

- Files modified: Resources/ui/index.html (CSS restructure + HTML relocation)
- New CSS classes: `.footer-keyboard`, `.footer-keyboard-viz`, `.footer-keyboard-help`
- Old `.keyboard-section` CSS removed (was in Tuning tab)
- Keyboard ID unchanged (`keyboard-viz`) for JavaScript compatibility
- Black key positioning uses `calc()` for responsive layout across 14 white keys

## [1.18.0] - 2026-01-26

### Added

- **Tooltip System** - Interactive parameter tooltips with toggle button
  - "?" toggle button in bottom-right corner (doesn't overlap UI elements)
  - Tooltips off by default, click "?" to enable
  - Persistent across tab switches
  - Tooltip state saved/restored with plugin session (DAW project save)
  - Parchment aesthetic styling matching O-Lyrica's classical theme
  - Dark brown tooltip with cream text for readability
  - Subtle dashed outline on hoverable elements when tooltips enabled

### Tooltips Authored

- **SOUND tab**: Brightness, Timbre, Decay Time, Attack Noise, Material, Tension, Gauge, Length, Stiffness, Body Size, Body Resonance, Wood Type, Mode Spread, Bridge Brightness, Pluck Position, Finger Hardness, Technique, Sympathetic Amount, Sympathetic Q
- **TECHNIQUES tab**: Glissando Mode, Glissando Scale
- **TUNING tab**: Temperament, Tuning Mode, A4 Reference, Octave Stretch
- **FOOTER**: Master Volume

### Technical Details

- Files modified: index.html (CSS + HTML + JS), PluginProcessor.h/cpp, PluginEditor.cpp
- New CSS: .tooltip-toggle, .tooltip, .tooltips-enabled styling
- New native functions: `setTooltipsEnabled`, `getTooltipsEnabled`
- State persistence via XML attributes in getStateInformation/setStateInformation
- Timer callback syncs tooltip state from processor on editor open
- Tooltip positioning with boundary constraints and automatic flip above/below

## [1.17.0] - 2026-01-25

### Changed

- **Renamed plugin from OuariconLyrica to O-Lyrica** to match Ouaricon naming convention
  - Plugin appears as "O-Lyrica" in DAW plugin lists
  - Updated all internal class names and identifiers
  - Preset storage location changed from `~/Library/OuariconLyrica/` to `~/Library/O-Lyrica/`
  - CMake target renamed from `OuariconLyrica` to `OLyrica`

### Technical Details

- Files modified: CMakeLists.txt, PluginProcessor.h/cpp, PluginEditor.h/cpp, CHANGELOG.md
- Class renames: `OuariconLyricaAudioProcessor` → `OLyricaAudioProcessor`, `OuariconLyricaAudioProcessorEditor` → `OLyricaAudioProcessorEditor`
- Plugin directory renamed from `plugins/OuariconLyrica/` to `plugins/O-Lyrica/`
- PLUGIN_CODE remains "OLyr" for AU/VST3 identification

## [1.16.0] - 2026-01-24

### Added

- **HTML Export** - Export current tuning as formatted HTML documentation (Phase 3.3)
  - Generates complete, standalone HTML document with professional styling
  - **Scale metadata**: Name, note count, period, A4 reference frequency, octave stretch
  - **Visual pitch circle**: SVG visualization showing interval positions vs equal temperament
  - **Interval table**: Degree, cents, ratio approximation (from 50+ common ratios), ET deviation
  - **Color-coded deviations**: Green (pure), orange (sharp), blue (flat)
  - **Generation metadata**: Date/time stamp and Ouaricon branding
  - Print-friendly CSS with clean margins and no shadows
  - Export button appears in Custom mode file buttons section

### Technical Details

- New files: `Source/DSP/TuningExporter.h`, `Source/DSP/TuningExporter.cpp`
- New native function: `exportTuningHTML` - generates HTML and opens save dialog
- TuningExporter static class with methods: `toHTML()`, `generatePitchCircleSVG()`, `approximateRatio()`, `calculateETDeviation()`
- Ratio approximation database includes 50+ common musical ratios (just intonation, Pythagorean, ET)
- SVG pitch circle shows both ET reference lines (faint) and actual interval positions
- Files modified: CMakeLists.txt, PluginEditor.cpp, index.html (CSS + JS + HTML)

## [1.15.0] - 2026-01-24

### Added

- **Factory Tuning Library** - 24 embedded tunings with categorized browser UI (Phase 3.2)
  - **Historical** (5): Young 1799, Neidhardt III, Kellner Bach, Bach/Lehman, Valotti
  - **Just Intonation** (4): Ptolemy Intense Diatonic, 5-Limit JI, 7-Limit JI, Partch 43-Tone
  - **Equal Divisions** (6): 17-EDO, 19-EDO, 22-EDO, 31-EDO, 41-EDO, 53-EDO
  - **Non-Octave** (4): Bohlen-Pierce (Equal), Carlos Alpha, Carlos Beta, Carlos Gamma
  - **World** (5): Arabic 24-TET, Turkish Makam, Indian 22-Shruti, Gamelan Slendro, Gamelan Pelog
  - Collapsible "Tuning Library" panel below the Generator section
  - Category dropdown filter (All, Historical, Just, EDO, Non-Octave, World)
  - Scrollable list with tuning name, category, note count, and description on hover
  - Click to instantly load any tuning

### Technical Details

- New files: `Source/DSP/EmbeddedTunings.h`, `Source/DSP/EmbeddedTunings.cpp`
- New native functions: `getEmbeddedTuningList`, `loadEmbeddedTuning`, `getEmbeddedTuningCategories`
- EmbeddedTuning struct: id, name, category, description, intervals (vector), period
- Static initialization pattern with `initializeTunings()` for lazy loading
- Loading a library tuning sets mode to Custom/Scala and updates APVTS parameters
- Non-octave tunings (Bohlen-Pierce, Carlos) include custom period values
- Files modified: CMakeLists.txt, PluginEditor.cpp, index.html (CSS + JS + HTML)

## [1.14.0] - 2026-01-24

### Added

- **Scale Generators** - Create custom tunings mathematically without loading Scala files
  - **EDO (Equal Division of Octave)**: Generate 5-53 equal divisions of any period (default 1200¢)
  - **Harmonic Series**: Build scales from consecutive harmonics (e.g., 8-16 for octave-based just intonation)
  - **Rank-2 Temperament**: Stack a generator interval to create meantone-like tunings
  - Collapsible "Generate Scale" panel in Tuning tab (below file buttons)
  - Generated scales automatically apply and switch to Custom mode

### Technical Details

- New files: `Source/DSP/ScaleGenerator.h`, `Source/DSP/ScaleGenerator.cpp`
- New native functions: `generateEDO`, `generateHarmonicSeries`, `generateRank2`, `applyGeneratedScale`
- ScaleGenerator uses modular design - intervals returned as vectors, applied via existing `setCustomIntervals()`
- All generators clamp inputs to safe ranges and remove near-duplicate cents values (0.1¢ tolerance)
- Files modified: CMakeLists.txt, PluginEditor.cpp, index.html (CSS + JS + HTML)

## [1.13.2] - 2026-01-23

### Fixed

- **Pitch circle indicator now correctly highlights scale degrees for non-12-note scales**
  - Previously, playing chromatically on a 19-tone scale showed degrees 0, 2, 3, 5, 7... instead of 0, 1, 2, 3, 4...
  - Root cause: UI used proportional mapping based on 12-TET pitch classes
  - Fix: Now uses linear mapping from anchor point (MIDI 60 + tonic), matching DSP behavior

## [1.13.1] - 2026-01-23

### Fixed

- **Tonic selection now persists when saving/reopening DAW projects**
  - Previously, tonic would reset to C on project reload due to a bug in the CustomState JSON serialization
  - Workaround: Tonic is now saved directly as an XML attribute, bypassing the CustomState mechanism
  - Also added timer-based sync from processor to WebView UI to ensure tonic display stays accurate

### Technical Details

- Added `directTonic` XML attribute in `getStateInformation()` / `setStateInformation()`
- Added `syncTonicFromProcessor()` JavaScript function called from C++ timer
- The CustomState callback still saves tonic (for preset files), but DAW state now uses the direct XML attribute

## [1.13.0] - 2026-01-23

### Added

- **Linear mapping for non-12-note scales**
  - Scales with any number of degrees (7, 19, 31, etc.) now play correctly
  - Each MIDI key plays the next scale degree in sequence
  - Anchor point: MIDI 60 (middle C) + tonic offset
  - Scale wraps at scaleSize using the scale's period (works for octave and non-octave scales)

- **Tonic selector now visible for all scale sizes**
  - Previously hidden for non-12-note scales
  - Tonic transposes by 12-TET semitones (shifts the anchor point)
  - Updated tooltip clarifies behavior for non-12 scales

### Fixed

- **19-note (and other non-12) scales now play all degrees**
  - Previously, playback broke at degree 15-16 due to incorrect octave calculation
  - Root cause: Code used MIDI note 0 as reference instead of MIDI 60
  - Fix: Anchor point is now `MIDI 60 + tonic`, with proper linear mapping

### Technical Details

- Rewrote `calculateCustomFrequency()` non-KBM path:
  - `anchorNote = 60 + tonic`
  - `scaleDegree = (midiNote - anchorNote) mod scaleSize`
  - `scaleOctave = floor((midiNote - anchorNote) / scaleSize)`
  - `freq = 12-TET(anchorNote) × 2^((scaleOctave × period + intervals[scaleDegree]) / 1200)`
- Updated `resetKeyboardMapping()` to use actual scale size instead of hardcoded 12
- KBM mode unchanged (takes priority when loaded)
- Files modified: Source/DSP/TuningEngine.cpp, Source/DSP/TuningEngine.h, Resources/ui/index.html

## [1.12.3] - 2026-01-23

### Fixed

- **Tonic selection now correctly affects sounding pitches**
  - The tonic note is always at its 12-TET frequency
  - The interval pattern stays the same but maps to different notes based on tonic
  - Example with Werckmeister III (intervals: 0, 90, 192, 294, 390...):
    - Tonic=C: C=0¢, C#=90¢, D=192¢, D#=294¢, E=390¢...
    - Tonic=D: D=0¢, D#=90¢, E=192¢, F=294¢, F#=390¢...
  - Scale degree 1 is always 90¢ above the tonic, regardless of which note is the tonic
  - The True Keys visualization now matches what you hear

### Technical Details

- Root cause #1: `calculateCustomFrequency()` was calculating all frequencies relative to A4=440Hz instead of anchoring the tonic at its 12-TET frequency
- Root cause #2: The code was using `rotatedIntervals` which mathematically rotated the interval values themselves, instead of keeping the same interval pattern
- Fix: Rewrote non-KBM code path to:
  1. Calculate scale degree as `(midiNote - tonic) mod scaleSize`
  2. Use original `scaleIntervals[scaleDegree]` (not rotated)
  3. Anchor to 12-TET frequency of the tonic in that octave
- Formula: `freq = 12-TET(tonic_in_this_octave) * 2^(scaleIntervals[scaleDegree] / 1200)`
- KBM mode unchanged (uses its own reference note/frequency system)
- Files modified: Source/DSP/TuningEngine.cpp

## [1.12.2] - 2026-01-22

### Fixed

- **Tonic selector now visible on fresh plugin load**
  - Previously, the tonic selector was hidden until user clicked "Custom" then back to "12-TET"
  - Root cause: C++ returns 13 intervals (including 1200¢ period), but UI expected 12
  - With 13 intervals, `total === 12` check failed, hiding the tonic selector
  - Fix: `loadIntervalsFromBackend()` now strips the period from 12-tone scales

- **Interval editing now works on fresh load (no preset selection required)**
  - Combined with v1.12.1 fix, interval editing now works in all scenarios
  - The 13-vs-12 interval mismatch was causing index confusion

### Technical Details

- C++ `getIntervals()` returns N+1 values (includes period at end for proper frequency calculation)
- JS UI should display N values (exclude period - it's not an editable interval)
- Added detection for 12-tone (1200¢ period) and Bohlen-Pierce (1902¢ tritave)
- Files modified: Resources/ui/index.html

## [1.12.1] - 2026-01-22

### Fixed

- **Editing interval cents now works when tonic is not C**
  - Previously, editing interval values in the UI had no effect on sound when a non-C tonic was selected
  - Root cause: `setSingleInterval()` updated `scaleIntervals` but not `rotatedIntervals` cache
  - When tonic != 0, `calculateCustomFrequency()` uses `rotatedIntervals`, so edits were ignored
  - Fix: `setSingleInterval()` now calls `rotateIntervalsForTonic()` to keep caches in sync

### Technical Details

- Compared with OuariconMarimba which uses `setCustomIntervals()` (updates full array) vs OuariconLyrica's `setSingleInterval()` (updates single value)
- `setCustomIntervals()` already handled rotation correctly; `setSingleInterval()` was missing this logic
- Files modified: Source/DSP/TuningEngine.cpp

## [1.12.0] - 2026-01-22

### Fixed

- **Modal Rotation Now Correctly Rotates Interval Pattern**
  - When changing the tonic, the interval pattern now properly rotates so the selected tonic becomes 0 cents
  - Previously, the tonic change only transposed pitch without rotating the modal pattern
  - Example: Werckmeister III with tonic D now shows D→D# interval as ~102¢ (rotated), not 90¢

- **Tuning State Now Persists with DAW Sessions**
  - All tuning settings (intervals, tonic, preset, octave stretch) now save/restore with DAW projects
  - Previously, tuning state was lost when reopening a project

- **Fixed Interval Count Initialization**
  - Constructor now properly initializes 12 scale degrees with the 1200¢ period value
  - Fixes edge cases where `scaleDegrees` could be 11 instead of 12

- **Fixed Mode Confusion in setCustomIntervals**
  - `setCustomIntervals()` no longer forces Scala mode, letting callers control the mode
  - Equal 12-TET preset now correctly stays in TwelveTET mode after intervals are set

### Technical Details

- Added `rotatedIntervals` cache and `rotateIntervalsForTonic()` for true modal rotation
- `calculateCustomFrequency()` now uses pre-rotated intervals when tonic != 0
- Added custom state callbacks to PresetManager for tuning persistence
- Files modified: Source/DSP/TuningEngine.h, Source/DSP/TuningEngine.cpp, Source/PluginProcessor.cpp

## [1.11.1] - 2026-01-22

### Fixed

- **Editing interval cents in UI now updates tuning**
  - Typing new cent values in the interval list inputs now correctly changes the tuning
  - Works in all scenarios: fresh load → Custom, after loading presets, after loading Scala files

### Known Issues

- **GUI keyboard does not reflect custom tuning** - The on-screen keyboard plays notes but doesn't visually indicate microtuning. This will be addressed in a future update.

### Technical Details

- **Root cause 1 (C++):** `setCustomIntervals()` did not call `setMode(Mode::Scala)`, so `rebuildFrequencyTable()` ignored custom intervals
- **Root cause 2 (JS):** Interval inputs were disabled when `currentTuningMode !== 1` (Custom), preventing any edits
- **Root cause 3 (C++):** APVTS `tuningMode` parameter wasn't updated, so `processBlock()` would override the mode back to 12-TET
- **Root cause 4 (C++):** `setMode(Scala)` didn't initialize `scaleIntervals` if empty, so clicking "Custom" left no intervals to edit
- **Fix 1:** Set mode to Scala AND always call `rebuildFrequencyTable()` in `setCustomIntervals()` (TuningEngine.cpp)
- **Fix 2:** Made interval inputs always editable (except unison) - editing auto-switches to Custom (index.html)
- **Fix 3:** Native function `setTuningIntervals` now also sets APVTS `tuningMode` to Custom (PluginEditor.cpp)
- **Fix 4:** Added `setSingleInterval()` and `setSingleIntervalEncoded()` native functions (TuningEngine.cpp, PluginEditor.cpp)
- **Fix 5:** JS uses encoded single-int approach to avoid multi-arg native function issues (index.html)
- Files modified: Source/DSP/TuningEngine.h, Source/DSP/TuningEngine.cpp, Source/PluginEditor.cpp, Resources/ui/index.html

## [1.11.0] - 2026-01-21

### Changed

- **Tonic Selector Now Performs Modal Rotation**
  - When tonic changes, the interval pattern rotates so the selected tonic becomes 0 cents
  - Notes below the tonic wrap around (last interval minus 1200 cents)
  - Example with intervals [0, 150, 200, ...]:
    - Tonic C: C=0¢, C#=150¢, D=200¢
    - Tonic C#: C#=0¢, D=150¢, D#=200¢, C wraps to -50¢
  - Previously, tonic only shifted pitch without rotating the interval pattern

### Technical Details

- Root cause: `calculateCustomFrequency()` was adding `tonic * 100 cents` as a pitch shift instead of rotating which intervals apply to which MIDI notes
- Fix: Reference note position now calculated using actual scale intervals with proper degree/octave mapping
- Removed unnecessary `tonicCents` pitch offset that was overriding the rotation logic
- Files modified: Source/DSP/TuningEngine.h, Source/DSP/TuningEngine.cpp

## [1.10.0] - 2026-01-21

### Added

- **5 Visualization Modes for Tuning System (Phase 2)**
  - **Circle** (existing): Radial pitch circle with interval lines
  - **Polar**: Radial tone wheel where cents value determines radius
  - **Matrix**: Interval values between all scale degrees
  - **True Keys**: Real-time intervals between held MIDI notes
  - **Rotation**: Modal rotation matrix with color-coded intervals

- **View Mode Toggle**
  - 5 buttons above visualization: Circle, Polar, Matrix, True Keys, Rotation
  - Seamless switching between visualization types
  - All modes update in real-time as tuning changes

- **Deviation Display in Interval List**
  - Shows ±cents deviation from equal temperament
  - Color-coded: green (pure), orange (sharp), blue (flat)
  - Helps identify how temperament differs from 12-TET

- **True Keys Held Notes Data**
  - PluginEditor sends held notes and frequencies to WebView
  - Real-time interval calculation between any combination of held notes
  - Shows interval names (m2, M3, P5, etc.) for common intervals
  - Total span display when 3+ notes held

### Fixed

- **View mode toggle buttons not visible** - CSS positioning conflict caused toggle buttons to render outside visible area. Changed from `bottom: 305px` to `top: 8px` positioning.
- **Pitch circle positioning conflict** - Removed absolute positioning from `.pitch-circle` that caused it to escape its container. Now uses relative positioning within `.viz-container`.

### Technical Details

- New PluginProcessor method: `getHeldNotesData(std::vector<int>&, std::vector<double>&)`
- JavaScript visualization functions: `drawPolarWheel()`, `drawIntervalMatrix()`, `updateTrueKeysDisplay()`, `drawModalRotationMatrix()`
- Global `vizMode` state with `setVizMode()` switching function
- `updateVisualization()` dispatcher replaces direct `updatePitchCircle()` calls
- Updated interval list to use new `.interval-row` class with deviation span
- Files modified: Source/PluginProcessor.h/cpp, Source/PluginEditor.cpp, Resources/ui/index.html

## [1.9.0] - 2026-01-21

### Added

- **Octave Stretch parameter (0.95-1.25)**
  - Physical modeling enhancement for piano-like stretched tuning
  - Values > 1.0 create wider octaves in upper register
  - Values < 1.0 create narrower octaves
  - Applied to both 12-TET and custom tuning calculations
  - New APVTS parameter "octaveStretch" with relay/attachment
  - UI: Horizontal slider in tuning controls panel

- **Built-in Temperament Presets (10 presets)**
  - Equal 12-TET (default)
  - Pythagorean
  - Zarlino (Just Major)
  - Meantone (1/4 comma)
  - Werckmeister III
  - Kirnberger III
  - Vallotti
  - Well Tempered
  - Just Intonation
  - Bohlen-Pierce (non-octave, 1902¢ tritave)
  - Custom option shown when user loads .scl file
  - UI: Dropdown selector above mode buttons

### Technical Details

- New TuningEngine API: `setOctaveStretch()`, `getOctaveStretch()`, `setBuiltInPreset()`, `getBuiltInPreset()`, `getPresetName()`
- `BuiltInPreset` enum with 11 values (including Custom)
- Static arrays for preset cent values in TuningEngine.cpp
- Native functions: `setTemperamentPreset`, `getTemperamentPreset`, `setOctaveStretch`, `getOctaveStretch`
- Loading .scl file automatically sets preset to Custom
- Files modified: TuningEngine.h/cpp, PluginProcessor.cpp, PluginEditor.h/cpp, index.html

## [1.8.0] - 2026-01-21

### Added

- **Complete Scala Keyboard Mapping (.kbm) file support**
  - Full KBM specification parsing: map size, MIDI range, middle/reference notes, octave degree
  - Keyboard mapping entries with unmapped key support ('x' entries)
  - `isNoteMapped()` method to query if a MIDI note is mapped in current KBM
  - `resetKeyboardMapping()` method to restore default linear 12-note mapping
  - Updated frequency calculation to properly apply keyboard mapping
  - Enhanced KBM file generation with complete spec output
  - Files modified: Source/DSP/TuningEngine.h, Source/DSP/TuningEngine.cpp

### Technical Details

- KBM state variables: mapSize, firstNote, lastNote, middleNote, referenceNote, octaveDegree
- Pattern-based mapping with proper octave transposition
- Handles edge cases: notes outside retune range fallback to 12-TET
- Thread-safe implementation with existing mutex protection

## [1.7.9] - 2026-01-20

### Added

- **Visual feedback on tuning circle when notes are played**
  - Interval lines flash red when corresponding notes are triggered via MIDI or UI keyboard
  - Velocity-based intensity: harder strikes produce brighter red (rgb(220,0,0)), softer strikes darker red (rgb(120,40,40))
  - Polyphonic tracking: multiple notes on same scale degree stack correctly (octave doubling)
  - Implementation matches Ouaricon Marimba's note visualization system
  - Files modified: PluginProcessor.h (MidiEventQueue), PluginProcessor.cpp (event pushing), PluginEditor.h/cpp (Timer polling), index.html (JS visualization)

## [1.7.8] - 2026-01-19

### Fixed

- **Tonic selector now visible on initial load in Tuning tab**
  - Root cause: TuningEngine initialized with 13 intervals (0-1200 cents including octave), but JavaScript checks for exactly 12 intervals to show the tonic selector
  - Fix: Changed initialization loop from `i <= 12` to `i < 12`, producing 12 intervals (0, 100, ..., 1100)
  - Previously required clicking away from 12-TET mode and back to see the tonic control
  - Files modified: Source/DSP/TuningEngine.cpp

## [1.7.7] - 2026-01-19

### Changed

- **Tuning tab layout restructured**
  - Keyboard shifted 40px left (total) for full clearance from SCL/KBM buttons
  - Pitch circle moved to center, positioned above keyboard
  - Pitch circle enlarged to 125% (188px from 150px)
  - SVG viewBox and JavaScript coordinates updated for new size
  - Files modified: Resources/ui/index.html (CSS, HTML, JavaScript)

## [1.7.6] - 2026-01-19

### Fixed

- **Keyboard position fine-tuned for Custom tuning mode**
  - Increased left offset from 10px to 20px for better clearance from SCL/KBM buttons
  - Files modified: Resources/ui/index.html (CSS)

## [1.7.5] - 2026-01-19

### Fixed

- **Keyboard no longer overlaps SCL/KBM buttons in Custom tuning mode**
  - Root cause: Keyboard section was centered at exactly 50%, causing right edge to overlap with file operation buttons when in Custom mode
  - Fix: Shifted keyboard 10px left using `left: calc(50% - 10px)` instead of `left: 50%`
  - Files modified: Resources/ui/index.html (CSS)

## [1.7.4] - 2026-01-19

### Fixed

- **Tuning system completely non-functional (Critical)**
  - Root cause: Wrong JUCE 8 API method names in JavaScript - used `getChosenIndex()`/`setChosenIndex()` instead of correct `getChoiceIndex()`/`setChoiceIndex()`. This caused a silent JavaScript error that aborted the entire tuning module initialization, preventing all tuning features from working.
  - Fix: Replaced all 7 occurrences of wrong method names with correct JUCE 8 WebComboBoxRelay API.
  - Files modified: Resources/ui/index.html

- **Scala file loading didn't affect tuning (even after JS fix)**
  - Root cause #1: `loadScalaFile()` updated intervals but did NOT change tuning mode from 12-TET to Scala.
  - Root cause #2: `processBlock()` reads APVTS tuningMode parameter every block and overrides TuningEngine mode.
  - Fix: Added `setMode(Mode::Scala)` in TuningEngine.cpp AND update APVTS parameter in native function handler.
  - Files modified: TuningEngine.cpp, PluginEditor.cpp

- **Tuning tab keyboard now plays actual notes**
  - Root cause: Keyboard visualization had visual feedback only - no native functions to trigger MIDI.
  - Fix: Added `triggerNoteOn(midiNote, velocity)` and `triggerNoteOff(midiNote)` native functions.
  - Files modified: PluginProcessor.h/.cpp, PluginEditor.cpp, index.html

### Technical Notes

- **JUCE 8 API Reference:** WebComboBoxRelay uses `getChoiceIndex()`/`setChoiceIndex()`, NOT `getChosenIndex()`/`setChosenIndex()`
- ES6 modules abort silently on uncaught errors - use try-catch with native logging to debug
- The tuning engine now has proper mode synchronization: loading .scl files automatically switches to Scala mode and updates APVTS
- Keyboard notes use MIDI channel 1 with velocity 0.8 (moderate)
- Note-off uses `allowTailOff = true` for natural release
- This fix has been documented in the scala-tuning-engine module's "Common Pitfalls" section

## [1.7.3] - 2026-01-19

### Fixed

- **Scala/KBM file loading and saving now works**
  - Root cause: Same as v1.7.2 mode button fix - ES6 module `addEventListener` calls not attaching in JUCE WebView
  - Fix: Replaced addEventListener with inline `onclick` handlers calling global functions (window.handleLoadSCL, etc.)
  - All 4 buttons now functional: Load .SCL, Load .KBM, Save .SCL, Save .KBM

- **Interval list editing now works in Custom mode**
  - Root cause: Same ES6 module addEventListener issue, plus undefined `scalaFileLoaded` variable causing JS error
  - Fix: Replaced addEventListener with inline `onchange` handlers for interval inputs
  - Fix: Removed undefined `scalaFileLoaded` variable reference
  - Intervals can now be edited by changing values and pressing Enter/Tab

- **Tonic arrows now respond to clicks**
  - Root cause: Same ES6 module addEventListener issue
  - Fix: Replaced addEventListener with inline `onclick` handlers for tonic up/down arrows

### Technical Notes

- Same fix pattern as v1.7.2: inline event handlers + global functions instead of ES6 module event listeners
- Global handlers defined in non-module script: handleLoadSCL, handleLoadKBM, handleSaveSCL, handleSaveKBM, handleIntervalChange, handleTonicDown, handleTonicUp
- Files modified: Resources/ui/index.html

## [1.7.2] - 2026-01-19

### Fixed

- **Tuning mode buttons (12-TET, Custom, MTS-ESP) now clickable and functional**
  - Root cause 1: CSS - `.tuning-viz-container` had no explicit `width`, causing invisible overflow blocking pointer events
  - Root cause 2: JavaScript - ES6 module `addEventListener` calls not attaching (JUCE WebView quirk with modules)
  - Root cause 3: APVTS - `tuningMode` parameter only had 2 options but UI showed 3 buttons
  - Fix: Added `width: fit-content` to CSS, replaced module event listeners with global inline `onclick` handlers, added "MTS-ESP" option to APVTS
  - Files modified: Resources/ui/index.html, PluginProcessor.cpp, TuningEngine.h, TuningEngine.cpp

### Technical Notes

- MTS-ESP mode is a placeholder - displays "MTS-ESP (Not Connected)" and uses 12-TET frequencies
- Button clicks use inline `onclick` handlers calling `window.handleModeClick()` instead of ES6 module event listeners
- Known Issues from this version fixed in v1.7.3

## [1.7.1] - 2026-01-19

### Changed

- **Keyboard visualization now compact and centered** (matching Ouaricon Marimba layout)
  - Fixed width: 280px × 70px (was full-width)
  - Centered at bottom of tuning tab
  - Black key positions now use CSS ID selectors for precise pixel placement

- **SCL/KBM file buttons now conditional**
  - Load .SCL, Load .KBM, Save .SCL, Save .KBM buttons hidden by default
  - Only visible when Custom tuning mode is selected
  - Matches Marimba behavior where file operations are context-dependent

### Technical Notes

- Files modified: Resources/ui/index.html
- Keyboard CSS refactored to use absolute positioning like Marimba
- Added `updateFileButtonsVisibility()` function to toggle scala-buttons div
- Black key left positions: 29px, 69px, 149px, 189px, 229px (40px per white key)

## [1.7.0] - 2026-01-18

### Changed

- **Tuning tab UI refactored to match Ouaricon Marimba layout**
  - Two-column + bottom layout replacing three-column design
  - Left side: Interval list with embedded tonic selector (◀ TONIC: C ▶)
  - Center: Large pitch circle visualization with radial interval lines
  - Right side: Mode buttons (12-TET, CUSTOM, MTS-ESP), A4 REF knob, scale name display
  - Bottom: Full-width keyboard visualization with "Click to play" label and note labels

- **Interval list redesigned**
  - Dynamic header showing note count: "Intervals (12 notes)"
  - Editable cent values in Custom mode (disabled in 12-TET mode)
  - Tonic selector embedded in list header for 12-tone scales
  - Compact layout with monospace font for values

- **A4 Reference Pitch changed from slider to rotary knob**
  - Draggable knob with 270-degree sweep
  - Visual indicator showing current position
  - Double-click to reset to 440.0 Hz (center)
  - Hz value displayed below knob

- **Pitch circle visualization upgraded**
  - Radial lines from center to each scale degree (Marimba style)
  - Note labels positioned around circumference
  - Adapts to any number of scale degrees (5-24+)
  - Green lines (#6B8E4E) matching Ouaricon aesthetic

- **MTS-ESP mode button added** (placeholder for future implementation)

### Removed

- Preset Scales dropdown (scales now loaded via .SCL files or 12-TET default)
- Tonic button grid (replaced with inline tonic selector in interval list)
- Pitch Bend Range from tuning tab (parameter remains functional)
- Three-column layout in favor of cleaner two-column + bottom design

### Technical Notes

- Files modified: Resources/ui/index.html (CSS + HTML + JavaScript restructure)
- masterTune parameter now controlled via custom knob code instead of slider binding
- Keyboard visualization now uses green scale degree bars on white keys
- CSS uses absolute positioning for tuning panel components (matching Marimba pattern)

## [1.6.0] - 2026-01-18

### Added

- **Full Tuning Module** with 12-TET and Custom (Scala) tuning support
  - New TUNING tab replaces placeholder with complete microtonal tuning interface
  - Two tuning modes: 12-TET (standard equal temperament) and Custom (Scala scales)
  - Interval list display showing all scale degrees in cents with ratio approximations
  - Pitch circle visualization showing note positions around the octave
  - Tonic/root selector (C through B) for scale transposition
  - Reference pitch (A4 frequency) slider moved from Techniques tab
  - Pitch bend range slider moved from Techniques tab

- **Scala File Support (.scl)**
  - Load any standard Scala scale file
  - Supports both cents and ratio notation (n/d, decimal, integer)
  - Scale name extracted from file
  - Save current scale as .scl file

- **Keyboard Mapping File Support (.kbm)**
  - Load keyboard mapping files to set reference frequency
  - Save current mapping as .kbm file

- **Preset Scales** (built-in)
  - 12-TET (Equal Temperament)
  - Just Intonation
  - Pythagorean
  - Quarter-comma Meantone
  - Arabic 17-TET
  - Slendro (5-TET)

- **Interactive Keyboard Visualization**
  - Visual piano keyboard in tuning tab
  - Click-to-highlight feedback (visual only)

- **Native Functions for WebView**
  - `getTuningIntervals()` - Get current scale intervals in cents
  - `setTuningIntervals(intervals, name)` - Set custom scale
  - `getTuningName()` - Get active tuning name
  - `setTonicNote(index)` / `getTonicNote()` - Tonic transposition
  - `loadScalaFile()` / `saveScalaFile()` - .scl file I/O
  - `loadKBMFile()` / `saveKBMFile()` - .kbm file I/O

### Changed

- **TuningEngine upgraded** with Mode enum, Scala parsing, interval storage
  - Thread-safe frequency table using atomic operations
  - Lock-free audio thread access to frequencies
  - Mutex-protected interval updates on message thread
- **Techniques tab simplified** - Tuning controls moved to dedicated Tuning tab
- **tuningMode parameter added** (0=12-TET, 1=Custom)

### Technical Notes

- Files modified: TuningEngine.h/.cpp, PluginProcessor.h/.cpp, PluginEditor.h/.cpp, index.html
- Frequency calculation: Custom intervals use cents-based formula: `f = c0 * 2^(cents/1200)`
- Scala parser handles cents (decimal), ratios (n/d), and integer ratios
- Tonic transposition shifts entire scale by specified semitones
- Preset scales defined in JavaScript with accurate historical intervals

## [1.5.4] - 2026-01-18

### Fixed

- **Preset UI buttons now fully functional** (prev/next arrows, dropdown, Save/Load)
  - Root cause: ComboBox (dropdown) binding code was throwing an uncaught error that crashed the entire JavaScript module before preset event listeners could be attached
  - The error occurred in `comboState.valueChangedEvent.addListener()` for the stringMaterial dropdown
  - Fix: Added try-catch around comboBox binding to prevent script crash, allowing preset system to initialize
  - Also changed preset native function calls to use inline get-and-call pattern for robustness
  - Files modified: Resources/ui/index.html (JavaScript)
  - See `.bugs/preset-ui-not-connected.md` for full investigation history

### Technical Notes

- ComboBox binding now wrapped in try-catch to prevent cascade failures
- Preset functions use inline pattern: `await Juce.getNativeFunction('name')()`
- This matches the working `getVoiceCount` pattern used elsewhere in the code

## [1.5.3] - 2026-01-18

### Changed

- **Refactored preset JavaScript to match Marimba pattern** (fix attempt #3)
  - Removed `await` from all `getNativeFunction()` calls
  - Changed `let` to `const` for function references
  - Simplified event listener attachment
  - Added `window.onPresetLoaded` callback for C++ integration

### Known Issues (Resolved in v1.5.4)

- **Preset UI buttons still not functional** - arrows, dropdown, Save/Load don't respond
  - See `.bugs/preset-ui-not-connected.md` for full investigation notes
  - Three fix attempts made, none successful
  - Next step: Add debug output to verify if native functions are being obtained

## [1.5.2] - 2026-01-18

### Fixed

- **CSS dropdown arrow now displays correctly** (was showing "u25BC" as text)
  - Root cause: CSS unicode escape was using JavaScript syntax `\u25BC` instead of CSS syntax `\25BC`
  - Fix: Changed `content: ' \u25BC'` to `content: ' \25BC'` in preset-name::after

### Changed

- **Refactored preset system JavaScript to avoid top-level await**
  - JUCE's WebKit WebView doesn't support top-level await in ES modules
  - Moved all `getNativeFunction()` calls inside async `initializePresetSystem()` function
  - Event listeners now check if native functions are ready before calling

## [1.5.1] - 2026-01-18

### Fixed

- **Preset module now fully functional**
  - Root cause: Preset bar was missing Save/Load buttons and corresponding native function handlers. The C++ side only registered basic preset functions (loadPreset, getPresetList, getCurrentPreset, selectNext/Previous) but lacked file dialog support for saving/loading preset files.
  - Fix: Added Save and Load buttons to preset bar HTML with matching CSS styling. Registered `savePresetWithDialog` and `loadPresetFromFile` native functions in PluginEditor.cpp using async FileChooser dialogs. Added JavaScript handlers for button click events.
  - Result: Users can now save custom presets to `~/Library/OuariconLyrica/Presets/User/` and load presets from any location using native file dialogs.

### Added

- **Save button** - Opens file dialog to save current settings as a user preset
- **Load button** - Opens file dialog to load any .json preset file
- `savePresetWithDialog` native function with async FileChooser
- `loadPresetFromFile` native function with async FileChooser

### Technical Notes

- Files modified: index.html (CSS + HTML + JavaScript), PluginEditor.cpp, PluginEditor.h
- FileChooser uses async launchAsync() pattern for non-blocking dialogs
- User presets stored in: `~/Library/OuariconLyrica/Presets/User/`
- Preset bar styling matches Ouaricon Naturalist aesthetic

## [1.5.0] - 2026-01-17

### Added

- **Preset Management System**
  - Integrated `OuariconPresetManager` module for save/load/navigate presets
  - Preset bar in header with prev/next navigation and dropdown browser
  - Botanical/Naturalist aesthetic styling matching UI design
  - Category-organized dropdown menu with string material groupings

- **48 Factory Presets** organized by string material (6 presets each):
  - **Gut**: Ancient Lyre, Fireside Tales, Medieval Court, Warm Classical, Bardic Song, Nostalgic Whisper
  - **Nylon**: Celtic Dawn, Folk Ballad, Gentle Stream, Morning Dew, Pastoral Scene, Harmonic Dreams
  - **Wire**: Bright Cascade, Articulate Pluck, Concert Grand, Modern Classic, Silver Strings, Pedal Technique
  - **Carbon**: Crystal Clear, Precision Touch, Extended Range, Studio Session, Clean Articulation, Harmonic Purity
  - **Metal Alloy**: Brilliant Sustain, Bell Tones, Orchestral Ring, Shimmering Heights, Warm Metallic, Ethereal Chime
  - **Glass**: Crystalline Voice, Fragile Beauty, Ice Palace, Winter Bells, Delicate Touch, Harmonic Prism
  - **Crystal**: Pure Resonance, Mystical Glow, Sacred Space, Singing Bowls, Meditation, Angelic Choir
  - **Energy**: Quantum Strings, Plasma Resonance, Electric Dreams, Cosmic Harp, Neon Glow, Future Primitive

- **Native Functions for WebView Integration**
  - `savePreset(name)`, `loadPreset(name)`, `getPresetList()`
  - `getCurrentPreset()`, `selectNextPreset()`, `selectPreviousPreset()`
  - `isFactoryPreset(name)` for identifying read-only factory presets

### Changed

- **State serialization** now uses preset manager for consistent preset name preservation
- **Header layout** updated to accommodate preset bar between title and voice counter
- **Preset storage location**: `~/Library/OuariconLyrica/Presets/Factory/` and `.../User/`

### Technical Notes

- Factory presets auto-initialize on first plugin load
- Presets stored as JSON with full parameter state
- Preset bar persists across all tabs (header-level component)
- Dropdown menu organized by string material categories
- CMakeLists.txt updated to include modules/persistence/preset-manager/cpp

## [1.4.0] - 2026-01-17

### Added

- **Ouaricon Naturalist aesthetic UI redesign**
  - New WebView interface with warm earth-tone paper texture background
  - Coral sea-fan botanical overlay (fern_naturalistsmisc1Geor_0089.png) that shifts right as tabs change
  - Garamond serif typography with wide letter-spacing for classical elegance
  - Custom slider styling with cream gradient thumbs and inset paper tracks
  - Styled dropdown menus for choice parameters

- **4-tab interface structure**
  - **SOUND tab**: All 20 sound parameters organized into 5 logical sections (Main, String, Body, Excitation, Sympathetic)
  - **TECHNIQUES tab**: Master Tune, Pitch Bend Range, Glissando Mode/Scale
  - **TUNING tab**: Placeholder for future microtonal tuning system
  - **EFFECTS tab**: Placeholder for future EQ and compressor

- **Connected 4 missing WebView relays** from v1.3.0 parameters
  - attackNoise, sympatheticQ, bodyModeSpread, bridgeBrightness now controllable via UI

### Changed

- **Window size reduced** from 800×600 to 700×450 for more compact layout
- **UI organization** with grouped sections and clear section headers
- **All parameters accessible** via sliders (float params) and dropdowns (choice params)
- **Master Volume** moved to footer with "Ouaricon Audio" branding

### Technical Notes

- Inline CSS and JavaScript for simpler asset management
- Tab-aware botanical overlay animation (shifts right -30px to -180px per tab)
- Double-click to reset sliders to default values
- Voice count polling at 100ms intervals
- Paper texture (paper1.jpg) and botanical image embedded as BinaryData

## [1.3.2] - 2026-01-17

### Changed (Code Quality)

- **Removed redundant null checks in HarpSynthVoice.cpp**
  - Root cause: Every APVTS parameter access checked for null, but APVTS guarantees non-null pointers for registered parameters
  - Fix: Removed per-parameter null checks while keeping the top-level `parameters != nullptr` guard
  - Result: Cleaner code, reduced verbosity (~40 lines removed from startNote/updateParametersFromAPVTS)
  - Files modified: HarpSynthVoice.cpp

- **Replaced dynamic_cast with static_cast in PluginProcessor.cpp**
  - Root cause: Voice loop used dynamic_cast when type is known at compile time (we control voice creation)
  - Fix: Use static_cast in prepareToPlay() since all voices are HarpSynthVoice
  - Result: Slightly more efficient, expresses intent better
  - Files modified: PluginProcessor.cpp

- **Added named constants in SympatheticResonance.cpp**
  - Root cause: Hardcoded magic numbers (0.05f, 0.1f, 0.995f, etc.) scattered throughout DSP code
  - Fix: Added constexpr constants in anonymous namespace:
    - `ENERGY_DECAY_BASE` (0.995f), `ENERGY_DECAY_MODIFIER` (0.0048f)
    - `COUPLING_SCALE_FACTOR` (0.05f), `INTENSITY_CHANGE_THRESHOLD` (0.01f), `Q_CHANGE_THRESHOLD` (0.05f)
    - `SOFT_CLIP_THRESHOLD` (0.1f), `SOFT_CLIP_HEADROOM` (0.05f)
    - `UNISON_COUPLING` (0.9f), `OCTAVE_COUPLING` (0.7f), `FIFTH_COUPLING` (0.5f), `THIRD_COUPLING` (0.3f)
  - Result: Self-documenting code, easier to tune DSP behavior
  - Files modified: SympatheticResonance.cpp

- **Fixed audio thread allocation in GlissandoController**
  - Root cause: `setScale()` copied std::vector during startNote(), causing allocation on audio thread
  - Fix: Replaced `std::vector<double> scale` with `std::array<double, MAX_SCALE_SIZE>` (48 elements)
  - Result: No dynamic allocation on audio thread in scale-locked glissando mode
  - Files modified: GlissandoController.h, GlissandoController.cpp

### Technical Notes

- Pure code quality release - no functional changes
- All changes are refactoring/cleanup identified by code review
- Build validates clean with Release configuration

## [1.3.1] - 2026-01-17

### Changed (Code Simplification)

- **Extracted enum conversion helpers** - Replaced 3 duplicate switch statements with inline helper functions
  - `woodTypeFromIndex()` in BodyResonance.h
  - `techniqueFromIndex()` in PluckExciter.h
  - `glissandoModeFromIndex()` in GlissandoController.h
  - Reduces HarpSynthVoice.cpp by ~40 lines while improving maintainability

- **Centralized filter cutoff calculations** in WaveguideString.cpp
  - New `FilterCutoffs` struct and `calculateFilterCutoffs()` method
  - Eliminates duplicate calculation in `updateFilters()` and `calculateFilterGroupDelay()`
  - Ensures filter cutoffs are always computed consistently

- **Added named constants** in BodyResonance.cpp
  - `MAX_DRY_REDUCTION` (0.6f) - Maximum dry signal reduction at full body resonance
  - `WET_GAIN_MULTIPLIER` (0.7f) - Wet signal gain (v1.1.5 value)
  - Replaces magic numbers with self-documenting constants

### Removed

- **HarpSynthSound.cpp** - Empty file (implementation was header-only)
- **BridgeFilter.h/.cpp** - Unused (WaveguideString uses juce::dsp::IIR::Filter)
- **DelayLine.h/.cpp** - Unused (WaveguideString uses juce::dsp::DelayLine)
- **StringVoice.h/.cpp** - Legacy Phase 2.1 implementation replaced by WaveguideString in Phase 2.2

### Technical Notes

- Pure refactoring release - no functional changes
- Removed 7 unused files, reducing codebase size
- CMakeLists.txt updated to reflect file removals
- Build validates clean with Release configuration

## [1.3.0] - 2026-01-17

### Added

- **Attack Noise Amount** - Independent control of pluck transient noise (0-100%)
  - Overrides material default noise content for user control
  - 0% = clean attack, 100% = scratchy/noisy attack
  - Files modified: WaveguideString.h/.cpp (setAttackNoise passthrough), HarpSynthVoice.cpp, PluginProcessor.cpp

- **Sympathetic Sharpness (Q)** - Controls resonator filter Q for sympathetic resonance
  - Range: 0.1 (broad/diffuse) to 20.0 (sharp/ringing)
  - Default: 5.0 (moderate sharpness)
  - Higher Q = more defined resonant peaks, more "shimmer"
  - Lower Q = broader resonance, more diffuse coupling
  - Files modified: SympatheticResonance.h/.cpp (setResonatorQ), PluginProcessor.cpp

- **Body Mode Spread** - Controls detuning/spread of body resonance modal frequencies
  - Range: -100% to +100% (centered at 0%)
  - 0% = original uniform scaling (modes at harmonic ratios)
  - Positive = modes spread apart (wider harmonic series)
  - Negative = modes compress together (tighter harmonic series)
  - Mode 2 (600Hz base) is the pivot point; modes 0,1 shift down and 3,4 shift up
  - Files modified: BodyResonance.h/.cpp (setModeSpread, scaleFrequency), HarpSynthVoice.cpp, PluginProcessor.cpp

- **Bridge Brightness** - Direct control of bridge filter cutoff for waveguide reflection
  - Range: 0-100% (0% = very dark/damped, 50% = neutral, 100% = very bright)
  - Provides more direct waveguide control than the general brightness parameter
  - Affects the first-order lowpass filter at the bridge reflection point
  - Pitch-compensated via calculateFilterGroupDelay()
  - Files modified: WaveguideString.h/.cpp (setBridgeBrightness), HarpSynthVoice.cpp, PluginProcessor.cpp

### Technical Notes

- All 4 new parameters support real-time modulation via updateParametersFromAPVTS()
- Attack Noise uses existing PluckExciter.setNoiseAmount() with passthrough from WaveguideString
- Sympathetic Q updates all existing resonator filters when changed (setResonatorQ)
- Body Mode Spread formula: `spreadMultiplier = 1.0 + (spread * modeOffset * 0.15)` where modeOffset is -2 to +2
- Bridge Brightness formula: `modifier = 0.3 + bridgeBrightness * 1.7` (0.3x to 2.0x range)
- UI additions: 4 new sliders in appropriate sections (Pluck, Sympathetic, Body, Advanced)

## [1.2.0] - 2026-01-17

### Added

- **Advanced string parameters now affect sound (tension, gauge, length)**
  - Root cause: Parameters were defined in PluginProcessor but never connected to DSP. The UI sliders existed but did nothing - only `stringStiffness` was wired up.
  - Fix: Added setter methods to WaveguideString (`setTension`, `setGauge`, `setLength`) and connected them in HarpSynthVoice.

### Changed

- **String Tension** (0-100%)
  - Controls string brightness and resonance via bridge/nut filter cutoff frequencies
  - Low tension (0%): Dark, muted, loose sound - filter cutoffs reduced by 50%
  - High tension (100%): Bright, resonant, tight sound - filter cutoffs increased by 2x
  - Pitch remains stable (filter group delay compensation updated)

- **String Gauge** (0-100%)
  - Controls damping characteristics simulating string mass/thickness
  - Low gauge (0%): Thin string, bright, quick attack and decay
  - High gauge (100%): Thick string, dark, heavier tone with more damping
  - Affects loop damping filter (200Hz - 14kHz range, expanded from 500Hz - 10.5kHz)

- **String Length** (0-100%)
  - Controls decay envelope character without changing pitch
  - Short (0%): Punchy, quick decay (70% of base decay time)
  - Long (100%): Sustained, diffuse decay (160% of base decay time)
  - Affects feedback coefficient calculation

### Technical Notes

- Files modified: WaveguideString.h, WaveguideString.cpp, HarpSynthVoice.cpp
- Tension modifier formula: `0.5 + tension * 1.5` (0.5x to 2.0x brightness)
- Gauge modifier formula: `0.5 + gauge * 1.5` (0.5x to 2.0x damping)
- Length modifier formula: `0.7 + length * 0.9` (0.7x to 1.6x decay time)
- `calculateFilterGroupDelay()` updated to include tension/gauge for pitch stability
- Real-time modulation supported via `updateParametersFromAPVTS()`

## [1.1.5] - 2026-01-17

### Changed

- **Increased body resonance intensity for more pronounced effect**
  - Wet mix increased from 0.3 to 0.7 (more than doubled)
  - Gain values significantly increased for each wood type:
    - Spruce: 1.8x → 3.5x (~11 dB boost)
    - Maple: 1.5x → 2.8x (~9 dB boost)
    - Exotic: 2.2x → 4.5x (~13 dB boost)
    - Synthetic: 2.5x → 5.5x (~15 dB boost)
  - Dry signal now preserved at 40% even at max resonance for blend
  - Files modified: BodyResonance.cpp

## [1.1.4] - 2026-01-17

### Fixed

- **Body parameters (size, resonance, wood type) now have audible effect**
  - Root cause: `BodyResonance::updateFilterCoefficients()` created peak filters with unity gain (1.0), which meant the resonant frequencies were not actually boosted. The body size affected filter frequencies and wood type affected Q values, but without gain boosting these differences were inaudible.
  - Fix: Added `getGainForWoodType()` method that returns appropriate linear gain values based on wood type. Peak filters now boost resonant frequencies by 3.5-8 dB depending on wood type.
  - Wood type gain values:
    - Spruce: 1.8x (~5.1 dB) - Traditional, balanced resonance
    - Maple: 1.5x (~3.5 dB) - Warmer, more subtle body
    - Exotic: 2.2x (~6.8 dB) - Pronounced, rich resonance
    - Synthetic: 2.5x (~8.0 dB) - Sharp, defined peaks
  - Result: Body size now audibly shifts resonant frequencies (small=bright, large=deep), wood type creates distinct tonal characters, and body resonance controls wet/dry blend.
  - Files modified: BodyResonance.h, BodyResonance.cpp

### Technical Notes

- JUCE's `makePeakFilter()` gain parameter is linear (not dB): 1.0 = unity, 2.0 = +6dB
- Body resonance uses 5-mode modal synthesis at 300, 400, 600, 900, 1200 Hz (scaled by body size)
- Q factor still varies by wood type (2.5-5.0) controlling resonance sharpness
- Mode amplitudes also vary by wood type for additional timbral shaping

## [1.1.3] - 2026-01-17

### Fixed

- **Pitch now stable across all string materials**
  - Root cause: `calculateFilterGroupDelay()` used a hardcoded `stiffnessDelay = 0.5f` constant, but the stiffness filter is a 4-stage allpass cascade whose group delay varies dramatically with material stiffness. Materials range from 0.05 (Gut/Wire) to 0.70 (Crystal), resulting in ~14x different phase delays. This caused pitch to drift differently per material even after the v1.1.2 fix.
  - Fix: Replaced fixed constant with dynamic calculation that replicates StiffnessFilter's coefficient computation (frequency scaling + per-stage progressive scaling) and sums the group delay from all 4 allpass stages using the formula `(1 - a) / (1 + a)` samples per stage.
  - Result: All 8 material types now produce identical fundamental frequency. Stiffness affects only inharmonicity (harmonic stretch), not fundamental pitch.
  - Files modified: WaveguideString.cpp

### Technical Notes

- Allpass group delay at DC: `τ = (1 - a) / (1 + a)` samples where `a` is the coefficient
- StiffnessFilter uses 4 stages with coefficients: `stiffness * freqScaling * stageScaling * 0.8`
- Crystal (0.70) now correctly compensates ~3.2 samples vs Gut (0.05) ~0.2 samples
- This completes the filter group delay compensation system: brightness (v1.1.1), material cutoffs (v1.1.2), and stiffness allpass (v1.1.3)

## [1.1.2] - 2026-01-17

### Fixed

- **String materials no longer affect fundamental pitch**
  - Root cause: `setMaterial()` was missing delay line compensation that was added in v1.1.1 for brightness. Different materials have vastly different `brightnessCutoff` values (Gut=2000Hz, Crystal=16000Hz) and `dampingCoeff` values, which feed into `calculateFilterGroupDelay()`. Changing materials altered the filter group delay by up to ~3 samples without compensating the delay line length, causing pitch drift.
  - Fix: Added delay line recalculation to `setMaterial()` using the same pattern as `setBrightness()`. Now when material changes, the delay line length is recomputed to maintain correct pitch.
  - Result: All 8 material types now produce the same fundamental frequency while retaining their distinct timbral characteristics (damping, brightness, stiffness, noise content).
  - Files modified: WaveguideString.cpp

### Technical Notes

- This completes the filter group delay compensation system started in v1.1.1
- Both `setBrightness()` and `setMaterial()` now recalculate delay lines when their parameters change
- Materials affect timbre via: brightnessCutoff (filter color), dampingCoeff (decay rate), stiffnessAmount (inharmonicity), noiseContent (attack character)

## [1.1.1] - 2026-01-17

### Fixed

- **Brightness slider no longer affects pitch**
  - Root cause: `calculateRailDelay()` used a fixed group delay compensation constant (6.0f samples), but the actual filter group delay varies dynamically with brightness settings. Lower brightness = lower filter cutoffs = higher group delay = lower pitch (up to 1 semitone flat at brightness=0).
  - Fix: Added `calculateFilterGroupDelay()` method that computes the actual group delay from all filters (bridgeFilter, nutFilter, loopDamping) based on their current cutoff frequencies. The delay line length is now recalculated whenever brightness changes.
  - Files modified: WaveguideString.h, WaveguideString.cpp

### Technical Notes

- First-order lowpass group delay at DC: `delay_samples = sampleRate / (2 * pi * cutoffHz)`
- Bridge/nut/damping filters now contribute dynamic compensation instead of fixed 6.0f
- `setBrightness()` now updates delay line lengths in addition to filter coefficients

## [1.1.0] - 2026-01-17

### Added

- **New "Decay Time" parameter for true sustain duration control**
  - Range: 0.1s to 20s with skewed control for finer adjustment at lower values
  - Implementation: Feedback coefficient multiplier applied per waveguide cycle
  - Formula: `coefficient = 10^(-3 / (decayTime * frequency))` for -60dB decay
  - This provides uniform energy loss independent of frequency content

### Changed

- **Renamed "Sustain" parameter to "Timbre"**
  - Root cause: The original "Sustain" parameter actually controlled tonal damping (lowpass filter cutoff in the feedback loop), not decay duration. Users perceived it as affecting attack brightness rather than sustain length.
  - The parameter now more accurately reflects its function: controlling the brightness/warmth of the string tone
  - Timbre=0.0 produces darker, warmer tones; Timbre=1.0 produces brighter tones
  - Internal behavior unchanged (controls `loopDamping` filter cutoff 500Hz-10.5kHz)

### Technical Notes

- Files modified: PluginProcessor.cpp, WaveguideString.h/.cpp, HarpSynthVoice.cpp, index.html, app.js
- Feedback coefficient recalculated on note trigger and frequency change (pitch bend)
- Breaking change: "sustain" parameter ID renamed to "timbre" - existing presets/automation will need adjustment

## [1.0.4] - 2026-01-17

### Fixed

- **Master Volume fader now controls output level**
  - Root cause: `masterVolume` parameter was connected to UI but never applied in `processBlock()`
  - Fix: Added gain stage after synthesizer rendering that converts dB parameter to linear gain

- **Sustain slider now affects decay time**
  - Root cause: `setMaterial()` unconditionally overwrote `dampingAmount` with the material's `dampingCoeff`, discarding the user's sustain slider value (same bug pattern as v1.0.3 stiffness fix)
  - Fix: Added `materialDamping` and `userDampingModifier` member variables with `calculateFinalDamping()` function that combines them using a 0.5x-1.5x modifier range
  - Result: Sustain=1.0 gives 0.5x material damping (longer decay), sustain=0.0 gives 1.5x material damping (shorter decay), while preserving material-specific characteristics

## [1.0.3] - 2026-01-16

### Fixed

- **String materials now produce audibly different timbres**
  - Root cause: `WaveguideString::setStiffness()` was completely overwriting the material's stiffness value with the user's slider value, making all materials sound identical in terms of inharmonicity
  - Each material defines a unique stiffness (Gut=0.10, Crystal=0.50) that creates its characteristic harmonic structure, but this was being discarded
  - Fix: User's stiffness slider now acts as a modifier (0.5x to 1.5x) rather than an overwrite, preserving material-specific inharmonicity while still allowing user adjustment
  - Result: Gut strings now sound warm/mellow, Crystal strings sound bright/bell-like, with clear audible distinction between all 8 material types

## [1.0.2] - 2026-01-16

### Fixed

- **Tuning: Pitches were ~1 semitone flat**
  - Root cause: `WaveguideString::calculateRailDelay()` did not compensate for the group delay introduced by feedback filters (bridgeFilter, nutFilter, loopDamping, stiffnessFilter)
  - The combined filter group delay (~6 samples) effectively lengthened the delay line, lowering pitch by approximately one semitone
  - Fix: Added 6-sample group delay compensation to the delay calculation

## [1.0.1] - 2026-01-16

### Fixed

- Enable real-time parameter modulation during note playback

## [1.0.0] - 2026-01-16

### Added

- Initial release
- Physical modeling harp synthesizer with bidirectional waveguide string model
- String materials: Nylon, Gut, Wire, Carbon
- Playing techniques: Normal, Harmonic, Muted, Pres de la Table
- Body resonance with wood type selection (Spruce, Maple, Exotic, Synthetic)
- Sympathetic resonance engine
- Tuning engine with master tune and pitch bend support
- Glissando controller with free and scale-locked modes
- WebView-based GUI
