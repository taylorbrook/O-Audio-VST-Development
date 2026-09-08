# O-Prism — Geometry wavetables (factory bank)

**Created:** 2026-09-08
**Type:** Feature (factory content + parameter range + UI dropdown + i18n + preset migration)
**Aspect:** Wavetable library
**Version Impact:** MINOR (v1.25.0) — a factory table bank is added; `oscATable` / `oscBTable` widen from 0–27 to 0–35; no DSP change, no new parameter
**Invoke:** `/improve O-Prism` → pick this brief (or `/improve-milestone O-Prism` if the migration-hook port grows)

**Status (2026-09-08): IMPLEMENTED as v1.25.0.** Deviations from the candidate table below, all forced by the gates in §1 and recorded in the CHANGELOG "Notes from the bake": every orbit rides a (1,q) torus-knot path, not (2,3), because p = 2 folds the period and fails gate (a) at every offset; *Star Hollow* (centroid distance, h5 dominant by construction) and *Blob* (XY slices of a blob are near-circles, no travel) were replaced by *Star Points* and *Blob Orbit*; *Terrain Rings* uses the sine-sum field rather than the product, whose h2 wins past r ≈ 0.5. No Geometry factory preset was authored (the optional ≤ 3). Final bank: Star Tilt, Star Points, Blob Orbit, Knot Torus, Knot Box, Gyroid Orbit, Noise Knot, Terrain Rings.

**Origin:** the O-Strata critique and listening pass of 2026-09-08 (`plugins/O-Strata/.planning/evidence/critique-check-2026-09-08.md`, WAVs in `critique-wavs/`). Verdict: the geometry-baked tables sound good but do not justify a separate plugin. They become an O-Prism factory category; O-Strata is re-planned around the live terrain oscillator.

---

## What ships

A **"Geometry"** category of 8 factory wavetables, baked **offline** by a Python script from the committed research prototype, embedded in the binary as int16 frame data through a generated header, and appended to `WavetableFactory` at indices 28–35. No generator code runs in the plugin. No new parameters. The stacked-frame display, user-wavetable import and editor are untouched.

**Rejected alternatives (recorded so they are not re-litigated):**
- *Live geometry generator inside O-Prism* (48 parameters, bake scheduler, 3D view): rejected — regression surface on a shipped 173-parameter plugin with a three-language corpus, for tables that are static content.
- *Install WAVs into `~/.ouaricon/wavetables` on first run*: rejected — user tables are user-managed and deletable, sit outside the factory dropdown, and would not be in presets by name.

## Curated candidates (Taylor picks the final 8 by ear; names ≤ 16 chars in the existing style)

| Working name | Source | Sweep | Notes from the check |
|---|---|---|---|
| Star Tilt | twisted star, XY projection | **Tilt X 0 → 45°** (not z — the z-sweep is spectrally static) | 12–17 partials, −12 dB/oct; travel comes from tilt only |
| Star Hollow | twisted star, Centroid Distance | tilt or φ | h5 dominant, h1 −16 dB — the "hollow" one |
| Blob | fBm-displaced sphere, XY | z | 8–12 partials, moderate travel |
| Knot Torus | Torus SDF, (2,3) knot **off-centre (0.15, 0.10, 0.05)** | orbit scale | centred version plays a twelfth up — never ship centred |
| Knot Box | Box SDF (0.5, 0.4, 0.3), (2,3) knot off-centre | orbit scale | 20–31 partials |
| Gyroid Orbit | gyroid, knot off-centre | orbit scale | untested in the check — bake and listen |
| Noise Knot | fBm volume, knot | orbit scale | 43 → 176 partials, centroid 4 → 15; the rich one |
| Terrain Rings | sin·sin, ellipse **off-centre (0.13, 0.21), aspect 0.7** | radius | monotonic brightness; centred plays an octave up |

Frame count: **32** for all (matches the existing 32-frame factory tables; 128 KB int16 each, ~1 MB total in the binary; runtime mipmaps ≈ 2.6 MB per table as today). 64 only for a table whose travel needs it.

## Deliverables

### 1. Baker script — `scripts/geometry-wavetables/bake_geometry_tables.py`
- Imports `research/wavetable-synthesis-3d-geometry-prototypes/mesh-slice/mesh_slice_wavetable.py` (numpy 2.4, scipy present) rather than copying its maths; adds the missing pieces: box SDF, gyroid, tilt sweep, orbit offset, off-centre ellipse terrain.
- Conditioning per the O-Strata contract, which mirrors O-Prism's own importer (`WavetableImporter.cpp:189-205`): per-frame DC removal → chained FFT cross-correlation alignment with polarity → **global** peak normalisation across all frames. Never per-frame peak.
- **Gates, script exits non-zero on failure:** (a) pitch — harmonic 1 is the strongest partial or within 6 dB of it in ≥ 95 % of frames; (b) travel — end-to-end RMS between frame 0 and frame N−1 ≥ 0.3 or spectral-centroid ratio ≥ 2; (c) adjacent-frame RMS ≤ 0.05; (d) no NaN, |peak| = 1.0. These would have caught every defect the check found.
- Writes `plugins/O-Prism/Source/dsp/GeometryTablesData.h` (one `constexpr int16_t` array per table + a table of `{name, frames, ptr}`), `scripts/geometry-wavetables/manifest.json` (names, frames, SHA-256 of each array) and preview WAVs to `plugins/O-Prism/.planning/evidence/geometry-wavs/` (gitignored). The header is committed; the WAVs are not.
- Deterministic: fixed noise seed, no `np.random` without a seed; re-running yields byte-identical output (manifest SHA check).

### 2. `WavetableFactory` (`Source/dsp/WavetableFactory.{h,cpp}`)
- `kNumFactoryTables` 28 → 36. `getTableInfoList()` gains 8 rows with category `"Geometry"` (indices 28–35, append-only).
- New loader for embedded tables: `allocate(frames)`, int16 → float into level 0, `generateMipmaps()`. **Do not route through `buildTable`/`normalizeFrame`** — that helper normalises per frame and would undo the global-peak conditioning (the O-Strata DSP-05 lesson).
- `FactoryPresets.cpp` `WT_` enum extended (`WT_StarTilt = 28 …`). No factory preset changes its table unless a new "Geometry" preset is authored (optional, ≤ 3).

### 3. Parameter range and preset migration
- `oscATable` / `oscBTable` are `AudioParameterInt` 0–27 (`PluginProcessor.cpp:81`), becoming 0–35. Their **normalised** encoding changes (27 → 35 steps).
- Presets store normalised values (preset-manager `savePreset` uses `getValue()`), so every **user** preset saved before 1.25.0 would silently repoint its tables. O-Prism's vendored `OuariconPresetManager.h` (605 lines, heavily customised) has **no** `setMigrationCallback` — port the v1.0.6 hook **surgically** (memory: `project_preset_manager_v102_rollout`; do not copy the canonical over the customised copy). Migration for presets < 1.25.0: `idx = round(v · 27)`, `v' = idx / 35`, for both table params.
- Factory presets regenerate at the version bump through the factory-version sentinel and are written from `WT_` indices, so they land correctly — verify by loading three presets that use non-zero tables (e.g. any `WT_VowelMorph` preset) and reading back the table name.
- DAW sessions are safe (APVTS stores the denormalised index). VST3 automation lanes on the table parameter are not migratable — one line in the CHANGELOG.

### 4. UI and i18n
- `index.html`: the two hard-coded dropdowns (osc A `<optgroup>` block near line 1451, osc B near 1505) gain `<optgroup label="Geometry">` with the 8 `<option>` names, in index order.
- `i18n.js`: the 28-name exemption list (near line 2497 — "hand-mirrored copy of `getTableInfoList()`", arm-1 exempt as C++-owned content) is extended with the 8 names, and the comment's count/line references updated. The optgroup label follows whatever treatment `Organic` / `Spectral` have today (check before assuming it is localised).
- Gates: `check-i18n --plugin O-Prism`, `i18n-fr-lint --strict`, `i18n-zh-lint`, `check-ui-labels`, `boot-all-uis --strict-tips`, `tests/ui_tip_render_check.js` — all green. No new prose in JS.

### 5. Build, validation, docs
- `CMakeLists.txt` `VERSION 1.25.0`; header added to sources (no `juce_add_binary_data` — plain C++ arrays avoid the hyphen-stripping and namespace-collision traps).
- `./scripts/build-and-install.sh O-Prism`; pluginval strictness 10 VST3 + AU; `auval`; param-dump still 173 rows (only the two ranges change).
- Smoke: instantiate, set `oscATable` = 28…35, render a C4 note for each, assert non-silent and finite; compare the level-0 frames against the manifest SHA (proves the embed round-trip).
- `CHANGELOG.md` v1.25.0 (bank, range change, migration, automation-lane caveat); PLUGINS.md row; backup snapshot per `/improve` convention.

## Acceptance criteria

- [ ] 8 Geometry tables appear under a "Geometry" group in both oscillator dropdowns; each plays at the written pitch (C4 measures 261.6 Hz fundamental ± 1 cent) with harmonic 1 strongest or within 6 dB
- [ ] Position sweep on each table audibly morphs (travel gate ≥ 0.3 RMS end-to-end) and clicks nowhere (adjacent RMS ≤ 0.05)
- [ ] A user preset saved under v1.24.0 with `oscATable = Vowel Morph` still loads as Vowel Morph in v1.25.0 (migration hook proven, not assumed)
- [ ] All 96 factory presets load with their intended tables (three spot-checked by name)
- [ ] `bake_geometry_tables.py` re-run produces a byte-identical header (manifest SHA match)
- [ ] pluginval strictness 10 VST3 + AU and auval pass; every UI/i18n gate green
- [ ] Binary growth ≤ 1.5 MB per format

## Traps carried from memory

`critical_apvts_denormalised_vs_preset_normalised` (widening shifts presets, not sessions) · `project_preset_manager_v102_rollout` (hook, surgical port) · `pattern_factory_preset_normalized_ignores_skew` · `critical_binary_data_strips_hyphens` / `critical_dual_binary_data_namespace_collision` (hence plain arrays) · `pattern_golden_tracked_as_checksum_only` (manifest) · `pattern_test_fixture_mirrors_drift_silently` (the i18n exemption list is already a hand mirror of the C++ list — extend both in the same commit) · `feedback_never_tag_unless_publish`.
