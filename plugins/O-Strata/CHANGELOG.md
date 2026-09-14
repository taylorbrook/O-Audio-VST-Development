# O-Strata Changelog

## v1.0.0 — 2026-09-13

O-Strata is a microtonal **wave-terrain** synthesizer: two oscillators each trace an
orbit across a 2-D height field and read the surface as their waveform, with the whole
microtonal engine, preset system and UI shell inherited from O-Prism. This is the first
complete version — installed locally, not published.

### Added

- **The live wave-terrain oscillator.** Six analytic terrains (Sine Product, Radial
  Rings, Saddle, Ridged Cosines, Mitsuhashi, Cosine Wells) × eleven orbit curves
  (Ellipse, Superellipse, Limaçon, Epitrochoid 3 / 5 / 7, Hypocycloid 3 / 5 / 7,
  Butterfly, Squarcle), each with Size, Aspect, Rotation, Centre X / Y and Orbit Mod;
  Terrain Freq, Mod X, Mod Y, Saturation; **Feedback** with Damp (the scan point rides
  its own displaced trajectory); pitch tracking; per-oscillator unison 1–4 with detune
  and width. Quality is 2× (default), 4×, or **Bandlimited** — a degree-16 Chebyshev
  projection of the terrain, truncated per pitch so the orbit's own harmonic bound sits
  under Nyquist with no oversampling at all.
- **PNG terrain import.** By chooser or by dropping a greyscale PNG on the big view or
  either oscillator's mini canvas, with Blur and Edge Mode (Mirror / Window). The image
  is projected into the same Chebyshev set in Bandlimited mode, and the readout reports
  how much of the picture survives the bandlimit.
- **Imports persist in both forms.** A PNG of 2 MB or less is embedded in the session
  and in the preset; a larger one (up to 8 MB through Import… / Locate…) is stored as
  its absolute path plus SHA-256. A missing source plays **Sine Product** rather than
  silence, shows a sticky notice, and offers **Locate…**, which re-links only a file
  whose hash still matches the preset.
- **18 factory presets** across Init, Pads, Drone, Lead, Bass, Pluck, Keys, Sequence and
  FX, covering every terrain, every orbit, feedback and Bandlimited. Two notes the
  preset files cannot carry themselves: **Pierce Bell** wants the Tuning tab at
  Bohlen-Pierce, and **Your Terrain Here** is the placeholder for a dropped PNG.
- **The Terrain tab**: a real 3-D view of the surface with the orbit drawn on it, the
  scan point where the voice actually is, the amber feedback trail, drag / wheel /
  ⌥-drag gestures that edit the active oscillator, the ≋ last-cycle view, and a readout
  that says exactly what the mode is doing (`2× · 16 partials at C4`,
  `Bandlimited · … · approx.`, `image projected at F = 1 · fit N %`,
  `silent above <note>`).
- **Inherited from O-Prism v1.24.0**: the microtonal tuning engine (24+ factory tunings,
  Scala / KBM import, EDO generators), the preset system, the five effects, the
  modulation matrix (46 destinations) and the interface in **English, French and
  Simplified Chinese**.

### Behaviour to know

- **Latency** is a constant **+1 sample** on top of whatever the distortion oversampler
  reports, on every Quality setting (ARCHITECTURE Decision 5). It is reported to the
  host, so a DAW compensates it.
- **There is no wavetable mode.** O-Strata reads a live surface, not a baked table; the
  superseded baked-geometry sources ship as an O-Prism factory bank instead
  (ARCHITECTURE Decision 6). Baked sources are a v1.1 item.
- **2× is the default Quality**, and **Bandlimited is the showpiece** (ARCHITECTURE
  Decision 8) — it is the mode that makes the aliasing floor disappear, at the cost of
  Terrain Freq being fixed per set.
- The terrain oscillator's CPU cost (PERF-02) is measured as a *delta* against a
  kernel-bypass baseline, not as a total, so the figure does not move with the rest of
  the synth.

### Known limits

**Bandlimited mode.** Terrain Freq is clamped to 0.25–2 and is not a live per-voice
destination (the coefficient set is per-F, published off-thread). At the top of the
keyboard the per-pitch truncation can remove every harmonic a pair has: 17 of the 144
terrain × orbit rows measured at A6 are muted this way, all of them
Epitrochoid 5 / 7 or Hypocycloid 7. This is audible as the note going silent, so the
readout carries a **`silent above <note>`** line whenever it applies. Feedback breaks
the trigonometric-polynomial bound the truncation depends on, which is why the readout
says *approx.* once Feedback > 0.

**Persistence.** A PNG above 2 MB is linked by **absolute path** — a preset made that
way opens on another machine with the "Source missing" notice and Locate…. Drop embeds
up to 2 MB; larger PNGs go through Import…, which links them by path. A preset without
an image clears both terrain slots on load. A user preset stores normalised parameter
values, so reloading one moves a skewed-range parameter by at most one float ulp against
the live patch (render delta ≤ 5e-8) — preset storage, not the image path, which is
SHA-exact.

**Windows.** *Named deferral — owner none, blocked on hardware. No human sees the
Windows UI this milestone.* CI now builds the VST3 under MSVC with WebView2 and runs
pluginval at strictness 10, which opens the editor: that proves the code compiles, the
plugin loads, and a silently-blank WebView would surface as an Editor-Automation failure
rather than passing green. It does not prove the UI is correct.

**PERF-03 (WKWebView frame time)** has no measured figure yet. The scripted Standalone
burst could not reach the WebView without taking the screen from the user; the row is
pending and carried as `stages/4-polish/LISTENING.md` Table C row 1 (Stage 3 human
row 1). No `gl.finish()` flag was added.

**Sample & Hold LFOs** draw from a clock-seeded RNG in production, so a patch using the
S&H shape is not sample-identical between runs (it is seeded deterministically only
under the offline render harness). Every other LFO shape is bit-reproducible.

**Inherited:** `stereoWidth` has no UI binding (from O-Prism).

### Technical notes

**Harness.** `--gate all` is 162 checks, 0 failures, 111.7 s on an M4 Max. H2 passes all
20 factory-preset rows (18 presets; two of them also gate oscillator B) and is now
bit-stable across runs. H6 holds `nonharm/max ≤ −60 dB` on all 198 grid rows at 2×
(worst −70.7 dB, Mitsuhashi × Epitrochoid 7 at A2) and `≤ −90 dB` on all 127 sounding
Bandlimited rows (worst −98.4 dB). H8 counts 0 allocations across 100 Quality / terrain /
orbit changes under 16 held notes. The orbit golden holds the JS port of `Orbits.h`
within 2.93e-6 over 44 curves.

**The audio-thread Chebyshev evaluator (Stage 4 Round B, D4).** `clenshaw2D` was
replaced on the audio thread by `chebEvalPadded`, which reads a padded 17 × 20
coefficient copy (pad lanes zero, `alignas (16)`) with fixed trip counts and per-row
4-lane dot products — plain portable C, no intrinsics. Measured over 1e6
dependency-carried evaluations on an M4 Max: **83.9–95.2 ns → 28.0–30.3 ns** (2.8–3.4×),
taking the H7 Bandlimited oscillator delta from **11.69 % to 4.46–4.54 %**, level with
the 2× path's own 4.46 %. Accuracy is contracted rather than assumed: max
|`chebEvalPadded` − `clenshaw2D`| = 9.775e-06 over 100 000 uniform points on a random
coefficient set (bound 2e-5) and 2.980e-07 on a projected set (bound 1e-6); both float
forms sit within 3.4e-06 of a double reference. Regression: the 144 Bandlimited H6 rows
moved by at most **0.100 dB**, with the truncation-muted and bound-equality sets
identical and the 2× / 4× / 1× rows bit-identical. `clenshaw2D` is unchanged and still
serves the three off-thread consumers.

**CI (COMPAT-02).** `.github/workflows/ci-tests.yml` takes a `plugin` dispatch input;
`gh workflow run ci-tests.yml --ref main -f plugin=O-Strata` builds the render harness on
macOS and runs `--gate all` plus the orbit golden, and builds the VST3 under MSVC with
WebView2 on Windows and runs pluginval at strictness 10. Three dispatches were made from `origin/main`. The **Windows job passed all three times** (6m35s / 6m01s / 6m12s): MSVC compiled the plugin TUs, WebView2 linked, the VST3 loaded, and pluginval strictness 10 completed every suite including Editor, Open-editor-whilst-processing, Editor Automation and Fuzz parameters, exiting 0 under `pipefail`. The **macOS job built the harness and ran the orbit golden at 44 / 44, worst 2.93e-6 — identical to the local figure** — and ran all 162 `--gate all` checks, of which two are wall-clock rows that the runner (Apple M1 Virtual, ~2.6× slower than the dev machine at 288–304 s against 111 s) does not meet: the scheduler's publish-latency row (264 / 176 / 154 ms against a 120 ms threshold) and, on two of three runs, the H7 Bandlimited row. **The local thresholds were deliberately not loosened**; a runner-appropriate band is a v1.1 item. Latest run: <https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/34809565500> at `dcbc609a`.

**Validation.** pluginval strictness 10 `SUCCESS` on the installed VST3 **and** AU;
`auval -v aumu OuSt OuDv` → `AU VALIDATION SUCCEEDED`; `auval -a` lists the plugin once
on a cold rescan; the installed binaries are byte-identical to the build tree's. The
on-disk factory bank is 18 JSONs in 9 category folders under the content stamp
`1.0.0+18c17735821c`.

### Stage history

The per-stage entries below are the development record, kept verbatim.

**Foundation.** O-Strata is forked from O-Prism v1.24.0 (commit `e88ec412`,
2026-09-07) as a new, independent plugin: `PLUGIN_CODE OuSt`, product name
`O-Strata`, APVTS state identifier `OStrataParameters`, its own preset folder.
O-Prism is untouched and the two plugins coexist in a host.

Stage 4, Round B — audio-thread evaluator, harness determinism, CI Windows, release-ready (Phase 4.2, 2026-09-13):
- **The audio thread stopped running Clenshaw.** `chebEvalPadded` reads a padded 17 × 20
  coefficient copy (pad lanes zero, `alignas (16)`) with fixed trip counts and per-row
  4-lane dot products — portable C, no intrinsics, no architecture `#if`.
  `buildChebWeights` writes the padded layout by loop index, so DSP-05's
  branch-free-on-data property is unchanged. Measured 83.9–95.2 ns → 28.0–30.3 ns
  (2.8–3.4×); H7's Bandlimited oscillator delta 11.69 % → 4.46–4.54 %, now a **check**
  against the same run's 2× delta + 2.0 rather than a reported row, with the `machine:`
  line reading `juce::SystemStats::getCpuModel()`. `clenshaw2D` is untouched and still
  serves `ChebyshevProjector`, the `TerrainScheduler` readout and `TerrainViewFeed`.
- **Accuracy is contracted, not assumed.** `--gate clenshaw` grew three checks: both
  evaluators' ns figures (reported), `max |chebEvalPadded − clenshaw2D| ≤ 2e-5` over
  100 000 uniform points on a random set (measured 9.775e-06), the same `≤ 1e-6` on a
  `projectAnalytic` set (measured 2.980e-07), and the distance to a 17-term double
  recurrence (3.41e-06 / 2.49e-07, reported). Regression: the 144 Bandlimited H6 rows
  moved by at most 0.100 dB, with the truncation-muted (17) and bound-equality (13) sets
  identical and the 2× / 4× / 1× rows bit-identical; H1's Chebyshev identity row is
  unchanged at −128.3 dB and H9's Bandlimited row is still `0.000e+00`.
- **Harness renders are now fully deterministic.** The Sample & Hold LFO drew from a
  clock-seeded `juce::Random`, so the one preset that uses it (Squarcle Storm) reported a
  different H2 figure on every run. `LFO::seed()` is now called for the four voice LFOs
  from the harness phase seed at note-on; **production is untouched** and keeps the
  clock-seeded RNG. `--gate H2` ×3 is byte-identical, Squarcle Storm reads −3.6 dB /
  100 % every time, and all 20 preset rows pass.
- **CI covers O-Strata.** `ci-tests.yml` takes a `plugin` dispatch input; the macOS job
  builds the render harness and runs `--gate all` plus the orbit golden, the Windows job
  builds the VST3 under MSVC with WebView2 and runs pluginval at strictness 10. The
  O-Octagon steps are unchanged behind an `if:`.
- **Listening material.** New `--gate exportPresets` renders all 18 factory presets
  through the real signal path (no FX bypass) to `tests/exports/presets/`; the 198-file
  grid golden is now LF-terminated so `shasum -a 256 -c` reads it directly.
  `.planning/stages/4-polish/LISTENING.md` carries the sitting.
- **Build**: `if(OUARICON_BUILD_TESTS AND APPLE)` makes the harness's macOS-only TU an
  explicit guard rather than an omission; `logViewPerf` writes through
  `juce::FileLogger::getSystemLogFileFolder()`, unchanged on macOS and no longer a fake
  `Library/Logs` tree on Windows.

Stage 4, Round A — persistence, fallback, Locate…, factory bank (Phase 4.1, 2026-09-12):
- **The imported PNG survives the session and the preset.** The `terrainImports`
  state child carries one `<slot osc form name sha256 size data|path/>` per oscillator
  that holds an import: a PNG of **2 MB or less is embedded** as standard base64, a
  larger one is stored as its absolute **path + SHA-256 + name**; the same fields ride
  every preset as `customState`. A restore re-imports through the Stage 2 API before
  the view's re-push generation moves, so the map lands within one push interval; a
  path-form file is re-read only when its contents still hash to the slot's SHA-256.
- **A missing source is heard, not silent.** An `Imported…` oscillator with no image
  now plays **Sine Product** at the same Terrain Freq / Mod X / Mod Y (before this
  round it produced nothing), the wireframe shows the same surface, and the sticky
  "Source missing — using library fallback" notice grows a **Locate…** button. Locate…
  re-links a file only when its SHA-256 matches the preset; a different file is refused
  for 4 s with "Different file — hash does not match the preset" and the terrain is left
  alone (Import… is one click away and re-stamps by definition).
- **Two import caps.** A dropped PNG still embeds up to 2 MB (`Image over 2 MB — not
  imported` unchanged). **Import…** and Locate… now accept up to **8 MB** (`Image over
  8 MB — not imported` above that); a PNG between 2 and 8 MB is linked by path. The cap
  constants live on the processor; the editor's copy is gone.
- **Factory bank: 18 presets** (Init + 17) covering all six terrains, all eleven
  orbits, feedback (Chatter Lead, Folded Bass, Squarcle Storm), Bandlimited (Pierce
  Bell, Wells Drone, Glass Rings) and the five BRIEF use cases (Breathing Pad, Chatter
  Lead, Pierce Bell, Your Terrain Here, PD Organ). Categories Init, Pads, Drone, Lead,
  Bass, Pluck, Keys, Sequence, FX; `Init` sorts first in the dropdown. Two notes the
  preset files cannot carry: **Pierce Bell** wants the Tuning tab set to Bohlen-Pierce
  (a preset never changes the tuning); **Your Terrain Here** is the placeholder for a
  dropped greyscale PNG — a saved User preset keeps the image. The bank is stamped
  `1.0.0+<sha256(bank)[0:12]>`; on a stamp mismatch the factory folder is swept and
  regenerated, so the Stage 1 `Init`-only bank (and any future orphan) cannot survive a
  bank edit. Every preset passes H2 at C4 on oscillator A (and B where it sounds):
  Breathing Pad −1.9 dB, PD Organ −3.3 dB, Butterfly Choir −4.5 dB, Squarcle Storm
  −3.8 dB at its centre (0.23, 0.21) (the default centre read −6.9 dB / 0 %), all others
  0.0 dB with 100 % of windows.
- **preset-manager v1.0.7** (module + O-Strata's copy): `customLoad` fires with an empty
  var when a preset carries no `customState`, so an image-less preset after an image
  preset clears the terrain slots instead of leaving the image live.
- **i18n**: five new keys (`label.locate`, `label.locateMismatch`,
  `label.importTooLargeFile`, `tip.terImport`, `tip.terLocate`; fr unreviewed, zh-Hans
  at `'mt'` → 53 rows); Import… and Locate… now carry tooltips (tip gate 120 → 122).
- **Harness**: `--gate H10` grows the state round trip (bytes form), the cap form with an
  incompressible 3.6 MB fixture (present / deleted / rewritten → Sine Product + `sourceMissing`),
  the preset `customState` round trip through the public file loader, the
  no-`customState` clear, two 1.9 MB slots published under `pump` (< 1 s; measured ≈ 60 ms)
  and the UI-04 ordering row; smoke [5] asserts the 18-file bank + stamp; the H2 preset
  rows honour Coarse / Fine, gate oscillator B when it sounds, bypass the FX chain (the
  pre-filter tap is a voice tap — reverb was smearing the h1 windows) and print the
  neighbours around the preset's own centre; `--real id=eng` and `--gate h2cli` run H2 on
  any patch without a rebuild; the layout gate gains a `locate` section.

**Known limits — persistence (Round A):**
- A PNG above 2 MB is linked by **absolute path**; a preset made that way opens on
  another machine with the "Source missing" notice and Locate….
- Drop embeds up to 2 MB; larger PNGs go through Import…, which links them by path.
- A preset without an image clears both terrain slots when loaded.
- A user preset is stored as normalised parameter values; reloading it moves a
  skewed-range parameter by at most one float ulp against the live patch (render delta
  ≤ 5e-8), which is preset storage, not the image path (the image path itself is
  SHA-exact).

Stage 3, Round A — Terrain tab, oscillator panels, readout, cycle view, i18n (Phase 3.1, 2026-09-12):
- **Shell re-forked from O-Prism v1.26.0** (`4f12ef57`): the Wavetable tab, its
  editor, modal and natives are gone; the card grid, the full Tuning tab, the
  tooltip canon and the preset modal are byte-identical to O-Prism (shell-diff
  gate: 0 px outside the masks on synth / mod / tuning / effects).
- **Oscillator cards**: Terrain and Orbit dropdowns bound to the C++ choice lists
  (`WebComboBoxRelay`), Orbit Size (default 50 %), unison 1–4, ⬡ / ≋ view toggle on
  each mini canvas.
- **Terrain tab** (fifth tab): one panel for whichever oscillator is active (Osc A / B
  repoints, never copies), Terrain / Orbit / Quality / Import… toolbar, 14 knobs
  through proxies, inert rules (Blur + Edge unless Imported…, Orbit Mod on Ellipse,
  Pitch Track in Bandlimited), the readout (`2× · 16 partials at C4`,
  `Bandlimited · … · approx.`, `image projected at F = 1 · fit N %`, and the new
  **`silent above <note>`** form from the scheduler's top-note probe), the ≋
  last-cycle view fed by the display voice's ring, the botanical plate. The 3D view
  is the mockup's placeholder renderer until Round B ports `Orbits.h`.
- **Terrain Freq readout / reset are exact on the log range** (page-side adapter —
  the lambda `NormalisableRange` never reaches the JUCE JS frontend).
- **Pushes, never `evaluateJavascript`**: `heldNotes`, `terrainStatus`,
  `terrainCycle` (512 × {px, py, y}, base64 Float32, ≤ 15 Hz) through
  `emitEventIfBrowserIsVisible`; `requestTerrainRepush` handshake at boot and after
  every preset apply.
- **DSP side**: `CycleCapture` ring 2048 → 8192 (a full cycle down to 23 Hz at 2×),
  display-voice re-election at block end (the ≋ view keeps moving while older notes
  sound), `chebTopNote` (strongest harmonic of the tapered set on the base orbit,
  ≥ 2e-3 ≈ −54 dBFS, bisection over MIDI 0–127, hidden ≥ C8), `TerrainViewFeed`,
  `getTerrainStatus`. Audio unchanged: `--gate all` 139 / 139 (135 + 4 `topnote`).
- **i18n**: 32 new / changed keys + 14 Terrain tooltips (en / fr / zh-Hans; fr
  unreviewed, zh at `'mt'` pending the back-translation read); `label.oscAShort` /
  `BShort` restored, `label.position` retired.
- **Gates**: new `tests/ui_layout_check.js` (666 budget, one-row `.osc-params`,
  width pins across languages, hit tests, 46 destinations) and
  `tests/ui_shell_diff_check.js`; stub fixtures generated from the binary
  (`--dump-choices` → `tests/tools/gen-stub-overrides.mjs`); 13 Terrain states;
  tip gate over five tabs (120 bindings); CDP font probe for the Terrain tab.

Stage 3, Round B — 3D terrain view, playhead, feedback trail, preset re-push (Phase 3.2) and view interaction, PNG import via chooser and drag-and-drop (Phase 3.3), 2026-09-12:
- **The 3D view is real**: the placeholder look-alikes are gone. `js/terrain-view.js`
  (one ES module, embedded and served) carries `Orbits.h` ported verbatim — the
  33 × 129 Superellipse LUT, the Padé-tanh Squarcle, the 64-point radius
  normalisation, the oscillator's affine order — held within 1e-4 of the binary by
  a new harness dump (`O-Strata-render-test --gate orbits --out PATH` +
  `tests/orbit-golden.mjs`; measured 2.9e-6 over 44 curves, base + affine), the
  camera, the heightmap sampler, Canvas 2D and WebGL2 renderers (R32F heightmap
  texture, screen-space ribbons, one static VBO for the dashed edge, `low-power`,
  context-loss recovery) and the PERF-03 frame ring.
- **Everything the view draws is pushed from C++**, never simulated on the page:
  `terrainHeightmap` (the ACTIVE surface — analytic at Terrain Freq, the imported
  image with Blur / Edge, or the untapered Chebyshev set — sampled on the 64 × 64
  grid by `TerrainViewFeed::copyHeightmap`, ≤ 10 Hz, key-gated), `terrainState`
  (the ring's newest slot {θ, x, y, h}: the DISPLACED point after feedback and its
  saturated value, 30 Hz, change-gated — the scan point rides where the voice
  is), the Round A `terrainCycle` (now also the amber feedback trail, shown only
  while Feedback > 0) and `terrainStatus`. Until the first heightmap arrives the
  wireframe is flat.
- **Preset apply and session restore re-push**: `stateGeneration` on the processor
  is bumped at the end of `setStateInformation` and by the four preset natives; the
  editor forces all five pushes once when it moved, even when every value landed
  identical.
- **View interaction** (big view only): drag → Orbit Centre X / Y under the cursor
  (unprojection onto the ground plane), wheel → Orbit Size (≤ 5 % per notch, one
  gesture per burst), ⌥-drag → Rotation (0.5° per px), ⌥-click → Rotation 0°. One
  host undo step per gesture; the gesture is captured on the oscillator it started
  on, so an Osc A ↔ B switch mid-drag cannot straddle two parameters.
- **PNG import**: `Import…` opens a native chooser (`*.png`); a PNG dropped on the
  big view or on either oscillator's mini canvas is streamed as bytes (WKWebView
  strips file paths). Both paths cap at 2 MiB, hand the bytes to the Stage 2 import
  API and select Imported… from C++ inside one gesture (Blur / Edge un-grey). A
  file over the cap or an undecodable one shows a transient notice for 4 s
  (`label.importTooLarge` / `label.importFailed`, en / fr / zh-Hans) and leaves the
  terrain unchanged; cancel leaves the parameter alone.
- **WebGL2 unavailable** (a 10.13–11 host): the Canvas 2D path draws the same view
  and the localised badge stays visible.
- **PERF-03 instrumentation**: the page's 300-frame ring (finish-inclusive and
  submit-only) reports through the native `reportViewPerf` to
  `~/Library/Logs/O-Strata/view-perf.log`; ⌥-click on the HUD runs a 300-frame
  burst so a Release host with no inspector can be measured.
- **i18n**: two keys (`label.importTooLarge`, `label.importFailed`; fr unreviewed,
  zh at `'mt'` → 48).
- **Gates**: `tests/ui_layout_check.js` gains `webgl` (DPR 1 + DPR 2 contexts,
  backing store, NO_ERROR, lose / restore), `fallback` (forced-null WebGL2 in three
  languages), `interaction` (page.mouse gestures with a Started / Ended spy on the
  stub instances, A ↔ B repoint) and `drop` (synthetic Files through the real
  handlers, `window.__stubNativeCalls`) sections plus source greps; 8 new Terrain
  states (default + 41); stub natives regenerated from the binary; the generic
  stub records native calls (`test(ui-stub)` commit). Windows / WebView2 halves of
  UI-01 / PERF-03 / FUNC-07 → Phase 4.2; `terrainImports` persistence,
  `sourceMissing`, factory presets → Stage 4.1.

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
  instance counters as the leak verdict — on this machine (macOS 26 / Darwin
  25.6, Xcode 26.3 clang 17) the ASan runtime hangs in its own shadow-memory
  initialisation before `main`, so the ASan rows are recorded as not runnable
  here; the instance-counter rows (storm, H8 image row) carry the verdict.
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
