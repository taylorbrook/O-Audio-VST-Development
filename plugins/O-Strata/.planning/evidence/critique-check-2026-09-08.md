# Critique check — 2026-09-08 (uncommitted evidence; decision pending)

An external critique of the O-Strata concept was tested against the committed Python prototype
(`research/wavetable-synthesis-3d-geometry-prototypes/mesh-slice/mesh_slice_wavetable.py`) with
`scratchpad/critique_check.py` (128 frames × 2048, largest loop, xcorr-aligned, per-frame peak as the prototype does).
WAVs for listening are in `critique-wavs/` (gitignored). O-Prism's `WavetableImporter` slices any WAV into
2048-sample frames, so each file drags straight into O-Prism's user-wavetable import.

Metrics: **f0-absent** = % of frames whose lowest partial within 30 dB of the strongest sits above harmonic 1;
**k_eff** = that lowest partial (mode over frames); **h1** = harmonic 1 level relative to the strongest partial;
**travel** = RMS difference and correlation between frame 0 and frame 127.

## Claims confirmed

| Claim | Measured | Verdict |
|---|---|---|
| (2,3) torus-knot orbit through the torus SDF, centred: pitch at 3·f0 | f0-absent 100 %, k_eff = 3, h1 = −∞ | **Confirmed.** (3,5) knot → k_eff = 5. Z-offset sweep centred: end-to-end RMS 0.000 (palindrome). |
| Twisted star in Centroid Distance: five peaks per cycle | k_eff = 1 but h1 = −15.8 dB below h5; 21 partials, centroid 4.8 | **Partly.** Fundamental present but weak; pitch ambiguous, timbre hollow. XY Projection (the shipped default) has h1 at 0 dB. |
| Star frames barely differ across the sweep | d(s): end-to-end RMS 0.077, corr +0.98; x(s): spectrum identical first/last (13 partials, centroid 1.4 → 1.4; only the phase rotates with the twist) | **Confirmed.** The twist rotates the star; the contour shape is z-invariant, so the sweep is spectrally static. Only Tilt gives travel. |
| Mesh contours are mild sources | star 12–21 partials > −40 dB, centroid 1.3–4.8; sphere/torus = 1 partial | **Confirmed** (matches research §7.1). |
| fBm volume is the rich source | 43 → 176 partials, centroid 4.4 → 15.0, h1 at −4 dB, f0 present in every frame | **Confirmed.** Also the one library source with large, monotonic travel. |

## Findings the critique missed (worse than stated)

1. **Terrain default is also an octave up.** Ellipse centred at (0,0) over Sine Product: f0-absent 100 %, k_eff = 2 (the field is centrosymmetric, so the orbit at θ+π repeats the value → period halves). Off-centre (0.13, 0.21, aspect 0.7): k_eff = 1, h1 −2.3 dB, centroid 1.0 → 5.3, monotonic. **Same symmetry class as the knot bug.** Every ARCHITECTURE default (Torus SDF + centred knot, Sine Product + centred ellipse) plays the wrong pitch out of the box.
2. **Crescent gives zero travel.** It is an extrusion: every slice is identical (adjacent RMS 0.000, end-to-end 0.000). As a bottom→top sweep it is a single-cycle wave, not a table.
3. **Sphere and torus are sines in every frame** (already known) — so of six library meshes, only the fBm blob and (weakly) the star produce a morph along z. The library is surfaces of revolution and extrusions, which is exactly the class whose slices do not change with height.
4. **Knot Phase sweep on a z-symmetric field is inert.** Rotating the knot about z inside the torus/sphere SDF changes nothing after alignment; the prototype frame at scale 1.0 also sits near-constant inside the tube wall and per-frame peak normalisation blew numerical noise to full scale (1022 "partials") — the global-peak rule in ARCHITECTURE avoids that, but the sweep axis is still a no-op for two of five fields.

## Fixes that survive any packaging decision

- **Break the symmetry by default.** Volume: bake a fixed asymmetric orbit offset (≈ (0.15, 0.10, 0.05) in unit space) into every orbit — restores k_eff = 1 on every field tested, no new parameter. Terrain: ship `TerCX/TerCY` defaults off-origin (≈ 0.13 / 0.21) and Aspect < 1, and document that a centred orbit over an even terrain doubles the pitch. Mesh: XY Projection default already immune; keep Centroid Distance as the "hollow" mode.
- **Rebuild the mesh library around z-variation**, not revolution/extrusion: shapes whose cross-section changes with height (gourd, spiral shell, tapered star, twisted star with inner ratio varying along z, a blob). Crescent and plain torus should not be sweep sources; keep sphere as the "pure sine" reference only if wanted.
- **Add a harness gate:** every library default must have h1 as the strongest partial (or within 6 dB) in ≥ 95 % of frames, and end-to-end travel RMS ≥ 0.3 or centroid ratio ≥ 2. This would have caught all of the above.
