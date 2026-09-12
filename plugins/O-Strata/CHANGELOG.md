# O-Strata Changelog

## v1.0.0 (unreleased)

**Foundation.** O-Strata is forked from O-Prism v1.24.0 (commit `e88ec412`,
2026-09-07) as a new, independent plugin: `PLUGIN_CODE OuSt`, product name
`O-Strata`, APVTS state identifier `OStrataParameters`, its own preset folder.
O-Prism is untouched and the two plugins coexist in a host.

Stage 2, Round B — Bandlimited mode + PNG terrain path (Phases 2.4–2.5, 2026-09-12):
- **Bandlimited mode** (`osc?Quality` = Bandlimited): the terrain is a degree-16
  Chebyshev triangle (153 coefficients, `dsp/ChebyshevSet.h`) evaluated per sample
  by 2-D Clenshaw at 1×, truncated per voice to the diagonals D ≤ ⌊fs / (2·K·f) − 1⌋
  with a continuous raised-cosine taper (K = the orbit's trigonometric degree;
  h1 never mutes) — truncation is the mip, so nothing aliases: 127 / 127 sounding
  exact-cycle rows ≤ −90 dB nonharm/max on the eight exact orbits. Coefficient
  sets are projected off the audio thread by `dsp/TerrainScheduler` (50 ms poll,
  2-thread pool, ModWheel / Aftertouch folded into the key, keys quantised to
  1/1024), published by atomic pointer on the message thread, retired through
  the type-erased reaper and crossfaded on arrival (64 base samples, equal-gain —
  also analytic ↔ Chebyshev). The oscillator copies the tapered coefficients at
  block start and never dereferences a set inside the sample loop. Readout
  atomics for Stage 3: generation, fit %, partials at C4, approximate flag.
- **PNG terrain path (DSP side)**: `dsp/TerrainImage` decodes a PNG (box-downsampled
  to ≤ 1024², luminance → [−1, 1]), pre-blurs it (3-pass box blur, σ = Blur·W/32 px,
  mirror-padded), embeds its own Chebyshev set projected at F = 1 for Bandlimited
  mode, and is read bilinearly with **Mirror** (even reflection) or **Window**
  (raised-cosine) edge handling. Import API on the processor:
  `importTerrainImage (osc, bytes, name)` / `importTerrainFile (osc, file)` (message
  thread; never touches a parameter; missing / undecodable → false). Blur / Edge
  changes re-run the job at a 150 ms cadence.
- **Known limits — Bandlimited mode:** Terrain Freq is clamped to 0.25–2.0× in this
  mode (one coefficient set per oscillator; the F-lattice is a v1.1 candidate —
  REQUIREMENTS "Out of Scope"); Pitch Track is inert (truncation is the mip); the
  three terrain destinations follow ModWheel / Aftertouch at the 20 Hz swap
  cadence and ignore per-voice sources; Superellipse / Butterfly / Squarcle and
  any Feedback > 0 are approximate (readout says so). Above ≈ A6 with a degree-6
  or -8 orbit (Epitrochoid 5 / 7, Hypocycloid 7) only the linear diagonal survives
  the truncation law, which is zero for an even terrain — those notes are silent
  in Bandlimited mode (17 of the 144 gate rows; reported).
- Harness: `--gate H6` gains the 144 Bandlimited rows; new `clenshaw`, `scheduler`,
  `storm`, `import` gates and H10 / H11; the message loop is pumped by exactly one
  call site (`pump`); an AddressSanitizer configuration (`build-asan/`) with
  instance counters as the leak verdict.
- Deviations from the plan: Cosine Wells' fit is re-measured at πF (99.99 / 98.63 %
  at F = 1 / 2; the architecture's 98 / 76 % was the 2πF form); Mitsuhashi's fit is
  98.3 / 87.6 % (its `tri()` wrap is piecewise-linear — the "≈ 100 %" was an
  estimate); the Chebyshev value is not clamped to [−1, 1] (a clamp is a hard
  nonlinearity that re-introduces the harmonics the taper removed); the scheduler
  is one class (`TerrainScheduler`, not `ChebyshevScheduler`) with throttle-plus-
  trailing debounce semantics; the blur is a 3-pass box, not a separable Gaussian.

Stage 2, Round A — live wave-terrain oscillator (Phases 2.1–2.3, 2026-09-11):
- `TerrainOscillator` replaces the wavetable oscillator in place: a closed orbit
  at the note frequency scans an analytic terrain per sample; θ is the phase
  accumulator, so Sync / Bend / Window / FM, unison (capped at 4), Phase, Coarse
  / Fine apply unchanged. Libraries: 11 orbits (`dsp/Orbits.h`, normalised to
  max radius 1 per block; Superellipse from a 33 × 129 LUT) and 6 terrains
  (`dsp/Terrains.h`, πF convention — Cosine Wells too, see below).
- Orbit Size and the 10 new per-oscillator mod destinations are read per sample
  through 5 ms base-value ramps shared across voices (24 processor ramp rows,
  read by absolute sample index); Terrain Freq ramps and modulates in log2.
- Pitch-tracked terrain frequency F_eff = F · min(1, C4 / f_note)^Track from the
  glide target (block-rate); trajectory feedback per partial (two-sample average
  + leaky integrator, damp law a = 1 − 2^(−1 − 9·Damp) rate/OS-corrected, ±0.5
  clamp, NaN scrub, displacement along the orbit's rotation vector); 5 Hz DC
  blocker per oscillator; Saturation tanh(g·y)/tanh(g), g = 1 + 4·Sat, exact
  identity at 0.
- Per-oscillator, per-partial 2× / 4× oversampling with hand-rolled polyphase-IIR
  halfband decimators (JUCE `FilterDesign` coefficients: 0.06 / −70 dB and
  0.15 / −60 dB), a 64-sample equal-gain crossfade on a Quality change mid-note,
  no allocation on any change. (In Round A `Bandlimited` ran the analytic path
  at 1×; the Chebyshev mode landed in Round B.)
- **Latency: +1 sample constant** (the decimators' 1.26 / 1.74 samples at 2× / 4×),
  added to the distortion oversampler's report in every Quality combination.
- Wavetable path deleted (`WavetableOscillator`, `WavetableData`,
  `WavetableGenerator`, `oscTablePtr` / placeholder / `getActiveOscTable`); the
  reaper is type-erased (`Retirable`); `getActiveOscFrame` / `getActiveOscInfo`
  answer `[]` / `{}` until Phase 3.1 removes them.
- Offline render harness `O-Strata-render-test` (`tests/render-harness/`,
  `OUARICON_BUILD_TESTS=ON`): gates H1–H9, tuning, smoke, centroids,
  saturation, decimator, crossfade, latency, `--gate export`.
- Deviation: Cosine Wells uses cos(πF·x)·cos(πF·(½+mx)·y) — the architecture's
  2πF form contradicts the stated πF convention and fails the symmetry gate on
  10 of 11 orbits at the locked defaults.

Stage 1 (this entry):
- Removed the wavetable library: `WavetableFactory`, `UserWavetableManager`,
  `WavetableImporter`, `WavetableEditor`, the `oscATable`/`oscBTable`
  parameters, the user-wavetable state child, 14 wavetable native functions,
  the wavetable tab/editor/selector UI and their i18n rows. Both oscillators
  read a published table pointer (`oscTablePtr[]`), initialised to the sine
  placeholder; the retire/reaper machinery stays for Phase 2.1's bake scheduler.
- Added the 48 geometry parameters of `parameter-spec.md` v1 (24 per
  oscillator: Source/Frames/Shape Drive, Mesh ×6, Volume ×5, Terrain ×10).
  Inert in Stage 1 — automatable, persisted, not mod-matrix destinations.
  219 parameters total.
- `delayDivision` gains a slider relay (it was bound in O-Prism's page but
  never followed host automation).
- Linked `juce_cryptography` (SHA-256 for Stage 4.1 path-linked imports).
- Factory presets carried over with their table entries stripped; every preset
  now plays the sine placeholder and is re-authored in Stage 4.1.

Stage 1, second pass (re-parameterise, 2026-09-10):
- The 48 baked-geometry parameters are replaced by the 34 live wave-terrain
  parameters of `parameter-spec.md` v2 (17 per oscillator: Terrain (7 surfaces),
  Terrain Freq (exact-log 0.25–8×), Terrain Mod X/Y, Pitch Track, Saturation,
  Terrain Blur, Terrain Edge, Orbit (11 shapes), Orbit Aspect, Orbit Rotation,
  Orbit Centre X/Y, Orbit Mod, Feedback, Feedback Damp, Quality (Bandlimited /
  2× / 4×, default 2×)) — **205 parameters**. Inert until Phase 2.1.
- `osc?Pos` is now **Orbit Size** (default 0.5, was 0.0); `osc?Unison` is 1–4
  (was 1–8). **This is the last free range change**: no O-Strata preset has
  shipped; from v1.0.0 every range or list change needs its own migration gate.
- Mod-matrix destinations 26 → **46**: indices 1/2 relabelled `OscA/OscB Orbit
  Size`; 20 per-oscillator destinations appended (Orbit Aspect, Orbit Rot,
  Orbit CX, Orbit CY, Orbit Mod, Terrain Freq, Terrain Mod X/Y, Feedback,
  Saturation — all A then all B). Accumulated by the matrix, not read by the
  voice until Phase 2.1.
- State child `geometryImports` → `terrainImports` (still written empty).
- Factory bank reset: the inherited O-Prism presets are removed; Stage 1 ships
  one `Init` preset at the 205-parameter defaults. The real bank is Phase 4.1.
- 8 `WebComboBoxRelay`s added for the four Choice parameters per oscillator
  (relays only — the page is unchanged until Stage 3).
