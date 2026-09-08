# O-Strata - Creative Brief

## Overview

**Type:** Synth
**Core Concept:** A microtonal wave-terrain synthesizer: each oscillator is a closed orbit scanning a 2D terrain at the note frequency, live, per sample — with orbit size, centre, rotation and shape, terrain frequency and trajectory feedback all modulatable at audio rate, per-oscillator oversampling, a provably bandlimited Chebyshev terrain mode that needs no oversampling, PNG terrains, and a 3D view of the orbit crawling over the surface.
**Status:** 💡 Ideated (re-planned 2026-09-08; supersedes the baked-geometry design in `superseded-baked-v1/`)
**Created:** 2026-09-07 · **Re-planned:** 2026-09-08

## Vision

O-Strata is O-Prism's voice, filters, modulation, effects and tuning engine with the wavetable oscillator replaced by a **live wave-terrain oscillator**. Mitsuhashi's 1982 idea: `y = f(x(θ), y(θ))` — a point runs around a closed orbit once per cycle and the height of the terrain under it is the waveform. What makes it sound different from a wavetable is that nothing is baked: the orbit's size, centre, rotation and shape, and the terrain's spatial frequency, can move **every sample**, driven by the mod matrix, and the previous output can push the next orbit point (trajectory feedback). Size sweeps are brightness ramps; centre offsets add even harmonics; rotation gives phase-distortion and PWM-like motion; feedback turns a static terrain into something that chatters and folds. That is the part of terrain synthesis Aaron Anderson's *Terrain* made people want, and the part a baked table cannot do.

Where O-Strata goes past *Terrain*: (1) **a bandlimited mode** — terrains expressed in the Chebyshev basis with a circular, elliptical or epitrochoid orbit produce a trigonometric polynomial with a *known* maximum harmonic, so per-pitch coefficient truncation is an exact, zero-oversampling mipmap (verified in `research/wavetable-synthesis-3d-geometry.md` §7.2: −116 dB non-harmonic floor at 1×); (2) **pitch-tracked terrain frequency**, so a patch keeps its harmonic count across the keyboard instead of aliasing at the top; (3) **per-oscillator 2×/4× oversampling** rather than whole-synth oversampling, so the O-Prism filter/LFO/mod-matrix voice loop is not doubled; (4) a **real microtonal engine** (scala-tuning-engine v3.0.1: 24+ factory tunings, Scala/KBM, EDO / harmonic / rank-2 generators); (5) PNG terrains with pre-blur and Mitsuhashi edge handling, projected onto the Chebyshev basis for the bandlimited mode. Nobody ships (1), (2) or (4) with terrain synthesis (§5 landscape, September 2026).

**What changed on 2026-09-08.** The first design baked geometry (sliced meshes, SDF orbits, terrain sweeps) into 256-frame wavetables on a background thread. A listening pass on the prototype tables found them pleasant but static — mesh sweeps are spectrally near-constant, centred orbits over symmetric fields play the wrong pitch, and the result is "O-Prism with a procedural table generator". Those tables now ship as an O-Prism factory bank. O-Strata keeps the name, the fork and the 3D view, and becomes the instrument with sonic identity: the live oscillator, which the research had deferred to v1.1. Mesh slicing and volume orbits are v1.1 candidates as a secondary *Baked* source type.

## Parameters

O-Strata inherits O-Prism v1.24.0's parameter set unchanged for the **Sub / Noise, Envelope, Filter, LFO / Mod Matrix, Tuning, Effects and Global** sections (see `plugins/O-Prism/.planning/BRIEF.md`). In the oscillator section, `oscATable` / `oscBTable` are removed and the 48 geometry parameters of the superseded design are replaced by a per-oscillator **Terrain** block and **Orbit** block. All other Osc A/B parameters carry over: Level, Pan, Coarse, Fine, Phase, Unison, Detune, Width, Warp Type, Warp Amount, and **Position, which is relabelled Orbit Size** (same ID `osc?Pos`, same 0–1 range, new default 0.5, maps to orbit radius 0.05–1.0; it stays a mod-matrix destination, so every existing "Position under an LFO" idiom becomes "orbit size under an LFO").

Every parameter below exists for Osc A (`oscA…`) and Osc B (`oscB…`). Parameters marked **(mod)** are appended to the mod-matrix destination list and are smoothed per sample; the others are block-rate. Draft IDs, ranges and defaults are in `parameter-spec-draft.md` (34 new = 17 × 2; total 205 APVTS parameters).

### Terrain (what is scanned)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Terrain | Sine Product / Radial Rings / Saddle / Ridged Cosines / Mitsuhashi / Cosine Wells / Imported… | Sine Product | Analytic terrains (clean-room formulas) or the imported PNG |
| Terrain Freq (mod) | 0.25–8.0 (log) | 1.0 | Spatial frequency multiplier — the main brightness / harmonic-count control; pitch-tracked (below) |
| Terrain Mod X (mod) | 0.0–1.0 | 0.5 | Terrain-specific shape input (ring spacing, saddle skew, well depth …), documented per terrain |
| Terrain Mod Y (mod) | 0.0–1.0 | 0.5 | Second terrain-specific shape input |
| Pitch Track | 0.0–1.0 | 1.0 | How much Terrain Freq scales down with note pitch: 1.0 keeps the harmonic count constant across the keyboard (the "terrain mip"), 0.0 keeps the spatial frequency fixed |
| Saturation (mod) | 0.0–1.0 | 0.0 | tanh drive on the scanned value before the oscillator output; identity at 0 |
| Image Blur | 0.0–1.0 | 0.2 | Gaussian pre-blur on PNG terrains (pixel detail is broadband); inert for analytic terrains |
| Edge Mode | Mirror / Window | Mirror | Mitsuhashi edge handling for PNG terrains; inert for analytic terrains |

### Orbit (how it is scanned)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Orbit | Ellipse / Superellipse / Limaçon / Epitrochoid 3 / 5 / 7 / Hypocycloid 3 / 5 / 7 / Butterfly / Squarcle | Ellipse | Closed curve traced once per cycle; θ is the oscillator phase accumulator, so Sync / Bend / Window warps, unison and FM apply to it unchanged |
| Orbit Size (mod) | 0.0–1.0 | 0.5 | = `osc?Pos`, relabelled; radius 0.05–1.0. Larger orbit crosses more terrain features → brighter |
| Orbit Aspect (mod) | 0.1–1.0 | 0.7 | Minor/major ratio |
| Orbit Rotation (mod) | 0–360° | 0 | Phase relationship between X and Y → PWM / phase-distortion-like effects |
| Orbit Centre X (mod) | −1.0–1.0 | 0.13 | Centre offset → asymmetry, even harmonics; dragged in the 3D view |
| Orbit Centre Y (mod) | −1.0–1.0 | 0.21 | As above |
| Orbit Mod (mod) | 0.0–1.0 | 0.5 | Shape parameter of the chosen orbit: superellipse exponent, limaçon loop size, epitrochoid inner ratio, squarcle corner sharpness; inert for Ellipse |
| Feedback (mod) | 0.0–1.0 | 0.0 | Trajectory feedback: the previous output displaces the next orbit point; bounded and damped so no setting runs away |
| Feedback Damp | 0.0–1.0 | 0.5 | One-pole smoothing on the feedback displacement (spatial compression) — the stability / character control |

### Quality

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Quality | Bandlimited / 2× / 4× | 2× | Bandlimited = Chebyshev terrain with per-pitch truncation at 1× (exact for Ellipse / Epitrochoid / Hypocycloid orbits, approximate for the others); 2× / 4× = per-oscillator halfband oversampling of the analytic terrain |

**Symmetry rule (from the 2026-09-08 check):** a centred orbit over an even terrain repeats itself every half-turn and plays an octave up (a knot through a symmetric field plays a twelfth up). Defaults are therefore off-centre and non-circular, and every factory preset and library default must pass a harness gate: harmonic 1 is the strongest partial or within 6 dB of it.

### Non-parameter state

- The imported PNG is persisted in plugin state and presets (raw PNG bytes, base64, ≤ 2 MB; above the cap: path + SHA-256 + name), with a "source missing — using library fallback" notice on load when the file is gone.
- In Bandlimited mode the Chebyshev coefficient set for the current terrain parameters is computed off the audio thread and swapped atomically (the same publish/reaper mechanism O-Prism uses for tables); it is regenerated from parameters, never stored.

## UI Concept

**Mockup:** to be created (v2). `superseded-baked-v1/mockups/v1-*` is the previous design and is not a source of truth; its shell, palette and the ⬡/≋ canvas toggle idea carry over.

**Layout:** O-Prism's 1200 × 800 shell unchanged — header bar with preset browser, five-tab bar, footer with Master / Osc Mix. Tabs: **Synth · Mod · Tuning · Effects · Terrain** (the old Wavetable tab slot).

**Synth tab:** identical to O-Prism except the two oscillator panels: the Shape dropdown becomes **Terrain▾** and a second dropdown **Orbit▾**; the 160 × 90 canvas shows the terrain wireframe with the orbit and a live scan point (3D) or the last cycle's waveform (≋), switched by the ⬡/≋ glyph. Knob order: Size (was Position), Level, Pan, Coarse, Fine, Phase, Unison, Detune, Width, Warp, Warp Amt. The drop overlay reads "Drop PNG".

**Terrain tab:** the showpiece. Top row: Osc A / Osc B toggle · Terrain▾ · Orbit▾ · Quality segmented control · **Import…** · right-aligned readout ("2× · 12 partials at C4"). Left: a 700 × 540 3D view — displaced 64 × 64 wireframe terrain, the orbit drawn on the surface, the scan point, and with Feedback > 0 the actual displaced trajectory trail so the player sees what feedback does. Monospace HUD ("θ 0.37 · r 0.61 · c (0.13, 0.21)"). Right (~440 px): **Terrain** group (Freq, Mod X, Mod Y, Pitch Track, Saturation; Image Blur and Edge Mode greyed unless Imported), hairline, **Orbit** group (Size mirrored, Aspect, Rotation, Centre X/Y, Orbit Mod, Feedback, Feedback Damp), and the hidden amber "Source missing" notice. The nautilus botanical (`mockups/img/shell_conchologiaiconi12reev_0090.png`) sits low-right behind the panel at 0.3 opacity, Terrain tab only.

**Interaction:** drag on the view moves Orbit Centre X/Y; wheel edits Orbit Size; alt-drag rotates the orbit; alt-click resets a knob. Document-level move/up pattern, `wheel {passive:false}`, drag-start/drag-end through the slider relay so hosts record one undo step. Drag-and-drop PNG onto either view (macOS `webkitGetAsEntry` pattern) plus the Import button.

**Visual Style:** Ouaricon Naturalist brand (ouaricon-naturalist-001), O-Prism's palette, Garamond stack and SVG vine-arc knobs; O-Prism CSS class names reused so Stage 3 can diff against the fork base.

**Key Elements:**
- 3D view: hand-rolled WebGL2 with a Canvas 2D fallback in the same file, no library (prototype: `research/wavetable-synthesis-3d-geometry-prototypes/webgl-3d/terrain-proto.html`, 5.5 KB gzipped, already draws exactly this scene).
- Playhead at 30 Hz via `emitEventIfBrowserIsVisible` (change-gated `{osc, theta, x, y}`), never `evaluateJavascript`; re-push after every preset apply.
- "WebGL unavailable" placeholder as a localised `data-i18n` node (en / fr / zh-Hans); all new labels carry `data-i18n` keys.

## Use Cases

- **Breathing pad.** Sine Product, Ellipse, Size under a slow LFO and Centre X under a second LFO at a different rate: brightness and even-harmonic content drift independently, and the player watches the ellipse breathe across the surface.
- **Feedback lead.** Ridged Cosines, Epitrochoid 5, Feedback at 0.4 with Damp at 0.3 and the mod wheel on Terrain Freq: the trajectory trail folds on itself and the tone chatters — the sound that no baked table makes.
- **Microtonal bandlimited bell.** Bohlen-Pierce from the tuning tab, Cosine Wells in Bandlimited mode, Hypocycloid 3: formant-like resonance at the third harmonic, alias-free to the top of the keyboard at 1×.
- **Bring your own terrain.** Drop a greyscale texture PNG; blur 0.2, Mirror edges; drag the orbit around the image to find its sweet spots; the preset stores the image.
- **Phase-distortion organ.** Saddle, Squarcle, Orbit Rotation swept by an envelope: PWM-like motion from a static terrain, run through the O-Prism FX rack.

## Inspirations

- **Aaron Anderson — *Terrain*** (JUCE, GPL-3.0, 2024): live analytic wave terrain, ~20 orbit curves, trajectory feedback with spatial compression, whole-synth 2–16× oversampling, OpenGL view. The reference for what "alive" means here. Formulas re-derived clean-room; no code copied (O-Strata is AGPL-3.0, Terrain is GPL-3.0).
- **Mitsuhashi (1982), Borgonovo & Haus, Mills & de Souza (1999), Stuart James (2005)**: the wave-terrain literature — orbits, windowing/edge constraints, stereo via displaced orbit pairs.
- **Conductive Labs *Terrain Synth*** (hardware, 2025) and Steven Barile's ADC25 talk: PNG terrains, sync and phase distortion framed as path–terrain relationships.
- **O-Prism** (v1.24.0): the engine, UI shell, tuning engine and preset system O-Strata forks from — and, since 2026-09-08, the home of the baked geometry tables.

## Technical Notes

Full research: `research/wavetable-synthesis-3d-geometry.md` §1 (paradigm), §7.2 (live-oscillator benchmark on Apple M4 Max, aliasing measurements, Chebyshev bound, architecture must-haves 1–7), §7.3 (WebView 3D view). Benchmark source: `research/wavetable-synthesis-3d-geometry-prototypes/terrain-bench/`. Listening evidence for the re-plan: `.planning/evidence/`.

**Fork base.** `plugins/O-Strata/Source/` already exists: O-Prism v1.24.0 fully renamed to `Strata`, wavetable library and editor removed, `JUCE_WEB_BROWSER`-guarded editor, `juce_cryptography` linked, pluginval/auval verified (`stages/1-foundation/`). The 48 baked-geometry parameters it carries are replaced in the Stage 1 re-parameterise pass. `WavetableOscillator` / `WavetableData` / mipmaps become dead code once the terrain oscillator lands — Stage 0 decides whether they are deleted or kept dormant for the v1.1 Baked source.

**Live oscillator (must-haves from §7.2, all mandatory):**
1. Interface parity with `WavetableOscillator` (`prepare / setFrequency / setPosition / setUnison / setWarp* / setFMInput / getNextSampleStereo`) with θ as the phase accumulator, so Sync, Bend, FM and Window warps, unison and the voice code apply unchanged; `setPosition` drives orbit size.
2. **Per-oscillator** 2× oversampling (halfband polyphase IIR up/down, ~9 ns per base sample), parameters held over the sub-samples; 4× "HQ". Not whole-synth oversampling: O-Prism's voice loop carries filters, LFOs and the mod matrix, and doubling it costs +10–15 % of a core and re-prepares every filter.
3. Pitch-tracked terrain spatial frequency ("terrain mip"): measured, an unbounded spatial-frequency parameter aliases at +16 dB at 1× and is not fixable by oversampling alone.
4. **Bandlimited (Chebyshev) mode**: terrain `f = Σ c_nm T_n(x) T_m(y)`, total degree ≤ 16 → with trig-polynomial orbits of degree K the maximum harmonic is (n+m)·K, so coefficients are truncated per pitch exactly like a mipmap level (−116 dB floor at 1×, verified). Coefficient sets are computed off the audio thread per (terrain, Freq, Mod X, Mod Y) and swapped atomically; in this mode the terrain parameters are block-rate at the swap cadence while orbit parameters stay audio-rate. PNG terrains are projected onto the basis offline at import ("bandlimited image"). Superellipse / Butterfly / Squarcle orbits are not trig polynomials — bandlimited mode is approximate for them and the UI says so.
5. No `pow` and no `std::function` in the per-sample path: `switch` on the terrain / orbit enums, `FastMathApproximations::tanh`, `juce::SmoothedValue` per modulated parameter; no per-block buffers that allocate on a quality change.
6. Unison cap 4 for the live oscillator (35 % of a core at unison 4, 2×, 16 voices × 2 osc).
7. Image terrain published through `std::atomic<const TerrainImage*>` with the existing generation-counted reaper; decode and pre-blur on a background thread; orbit clamped to [−1, 1]² and `isfinite`-guarded before the bilinear read.

**Feedback.** `p[n] = orbit(θ[n]) + fb · d[n]`, `d[n] = damp · d[n−1] + (1 − damp) · y[n−1] · û`, with `û` a fixed direction rotated with the orbit, `d` clamped to ±0.5 and the sampled point clamped to [−1,1]². Stability is a harness gate (no NaN, no runaway, bounded output at every Feedback × Damp × terrain × orbit grid point).

**CPU budget (Apple M4 Max, clang −O2, §7.2):** 16 voices × 2 osc, unison 1 — wavetable today 2.2 %; live analytic 2× 8.8 %; live PNG 512² 2× 11 %; worst orbit + terrain 2× 16 %; Chebyshev 16 × 16 at 1× 8.5 %. Gate: ≤ 12 % at the default patch.

**Aliasing (§7.2, C6):** butterfly + sine product −18.5 dB at 1×, **−60 dB at 2×**, −105 dB at 4×; epitrochoid-7 + sine product −18 dB at 2×, −66 dB at 4×. Gate: every library terrain × orbit default ≤ −60 dB non-harmonic at C6 at 2× (with pitch tracking on), ≤ −90 dB in Bandlimited mode with a trig-polynomial orbit.

**3D view.** As the superseded design: hand-rolled WebGL2 + Canvas 2D fallback in one file, R32F heightmap texture, `webglcontextlost / restored`, no GLSL `flat`; PERF-03 measured in WKWebView and WebView2 at DPR 2, ≤ 2 ms mean.

**Licence.** AGPL-3.0-or-later, as O-Prism. *Terrain* (GPL-3.0) formulas re-derived from the mathematics with attribution; no source copied.

## Out of Scope (v1.0)

| Feature | Reason | Target |
|---------|--------|--------|
| Baked geometry sources (mesh slicing, SDF / fBm volume orbits, terrain orbit sweeps → wavetable) | Listening pass 2026-09-08: pleasant but static; ships as an O-Prism factory bank instead. The full design is preserved in `superseded-baked-v1/` for a "Baked" source type | v1.1 |
| Wavetable oscillator mode in O-Strata | O-Prism is the wavetable synth; O-Strata is terrain-only by design | none |
| RGB-channel terrain morphing (three terrains from one PNG) | Extension once PNG import is stable | v1.1 |
| Stereo via displaced orbit pairs (Mills & de Souza) | Unison Width covers the basic case; true dual-orbit stereo is a per-voice CPU doubling | v1.x |
| Scanned synthesis (mass-spring mesh) | Stability + block-rate simulation | v1.2+ |
| ADAA for terrains | Mathematically unsound for arbitrary composite paths | none |
| three.js / lit rendering | No bundler; ESM-only | v1.x |

## Next Steps

- [ ] (Recommended, 1 hour) Play Aaron Anderson's *Terrain* to confirm audio-rate orbit modulation and feedback are the sound wanted — the same listening test the baked tables just failed
- [ ] `/plan O-Strata` → new ARCHITECTURE.md + ROADMAP.md from this brief and `parameter-spec-draft.md`
- [ ] UI mockup v2 (`design UI for O-Strata`), lock `parameter-spec.md` v2
- [ ] Stage 1 re-parameterise pass on the existing fork (`/plugin-discuss O-Strata 1-foundation`)
