# O-Strata Notes

## Status
- **Current Status:** 📦 Installed (not published)
- **Version:** 1.0.0
- **Type:** Synth (Microtonal Wave Terrain)

## Lifecycle Timeline

- 2026-09-07 — Research (Level 2 + Level 3): `research/wavetable-synthesis-3d-geometry.md`
- 2026-09-07 — Ideation complete: BRIEF.md + REQUIREMENTS.md
- 2026-09-07 — Stage 0 complete: ARCHITECTURE.md + ROADMAP.md; mockup v1 finalized; parameter-spec.md locked (219 params)
- 2026-09-07 — Stage 1 (Foundation) executed: forked O-Prism v1.24.0 (commit e88ec412) into `plugins/O-Strata/` as an independent plugin (`OuSt`, VERSION 1.0.0, every `Prism` identifier renamed); wavetable library removed (`WavetableFactory`, `UserWavetableManager`, `WavetableImporter`, `WavetableEditor`, `oscATable`/`oscBTable`, 14 native functions, wavetable tab); 48 inert geometry parameters added; `juce_cryptography` linked; both oscillators play the sine placeholder via `oscTablePtr[]`. Details: `.planning/stages/1-foundation/first-pass-baked/SUMMARY.md`
- 2026-09-08 — Design v2 (live wave-terrain oscillator): the baked-geometry design is superseded (`.planning/superseded-baked-v1/`); ARCHITECTURE / ROADMAP / REQUIREMENTS v2
- 2026-09-10 — Stage 1 second pass (re-parameterise) executed: 48 baked params → 34 live-oscillator params (205 total), mod destinations 26 → 46, `terrainImports` state child, 8 combo relays, factory bank reset to a single `Init`, COMPAT-01 re-verified. Details: `.planning/stages/1-foundation/SUMMARY.md`
- 2026-09-10 — Mockup v2 finalised + scaffolded; `parameter-spec.md` v2 locked (205 params, 46 mod destinations)
- 2026-09-11 / 12 — Stage 2 (DSP, live wave-terrain oscillator) VERIFIED in two rounds (`stages/2-dsp/`); Stage 3 (GUI: Terrain tab, 3D view, PNG import) VERIFIED in two rounds (`stages/3-gui/`)
- 2026-09-12 — Stage 4 Round A (Phase 4.1) executed: `terrainImports` persistence (bytes ≤ 2 MiB inline / path form above, SHA-256 checked), Sine Product fallback for a missing source + Locate…, the 18-preset factory bank under a content stamp, preset-manager v1.0.7. Details: `.planning/stages/4-polish/round-a/SUMMARY.md`
- 2026-09-13 — Stage 4 Round A VERIFIED (`.planning/stages/4-polish/round-a/VERIFICATION.md`): every Round A gate re-measured; FUNC-08 / FUNC-11 / QUAL-03 complete; the S&H LFO's non-reproducible H2 row recorded as a Round B item.
- 2026-09-13 — Stage 4 Round B (Phase 4.2) executed: the padded Chebyshev evaluator on the audio thread (H7 Bandlimited 11.69 % → 4.5 %, accuracy contracted in `--gate clenshaw`), the S&H LFO under the harness phase seed (all 20 H2 preset rows bit-stable), `ci-tests.yml` with a `plugin` dispatch input and O-Strata's macOS-harness + Windows-VST3 jobs (COMPAT-02), the listening material and `LISTENING.md`, CHANGELOG v1.0.0, local install + validators. Details: `.planning/stages/4-polish/SUMMARY.md`

## Attribution & licence

- **Licence:** AGPL-3.0-or-later, SPDX headers on the source files, consistent with the suite's JUCE licence election (2026-08-01).
- **Terrain mathematics — clean room.** The analytic terrains and orbit curves are implemented from published mathematics, not from anyone's source. The Mitsuhashi terrain follows Mitsuhashi 1982 as described in Mills & de Souza 1999 (*Gestural Control of Wave Terrain Synthesis*); the remaining terrains and all eleven orbit curves are standard closed forms (products and sums of sinusoids, radial cosines, superellipse, limaçon, epitrochoids, hypocycloids, the butterfly curve, a tanh-shaped "squarcle"). The Chebyshev bandlimit identity is Puckett, *Theory and Technique of Electronic Music*, §node80; the 2-D Clenshaw recurrence is textbook. ARCHITECTURE Decision 10.
- **Inspiration only, no code:** *Terrain* (GPL-3.0) was looked at as a reference for what wave-terrain synthesis feels like to play. Nothing was copied from it — neither source nor data — which is why the mathematics above is written out from the literature.
- **Fork base:** O-Prism v1.24.0 (engine, UI shell, tuning engine, preset manager, i18n), same repository and licence.
- **Artwork:** the Terrain tab's botanical is `Source/ui/public/img/shell_conchologiaiconi12reev_0090.png`, a scan of a plate from Lovell Reeve's *Conchologia Iconica* vol. 12 (1854–1878) — a public-domain 19th-century natural-history work, in keeping with the suite's "Ouaricon Naturalist" aesthetic. *(No credit line for this plate existed elsewhere in the repository at the time of writing; this one was written from the asset's own filename rather than copied — see `stages/4-polish/SUMMARY.md`, deviations.)*

## Known Issues

- `stereoWidth` has no UI binding (inherited from O-Prism).
- A preset's imported PNG above 2 MiB is linked by **absolute path**; on another machine it opens with the "Source missing" notice and Locate…. A preset without an image clears both terrain slots on load.
- **Windows:** named deferral — owner none, blocked on hardware. No human sees the Windows UI this milestone. CI proves the code compiles under MSVC, the VST3 loads and pluginval strictness 10 opens the editor; it does not prove the UI is correct.
- **PERF-03 (WKWebView frame time)** has no measured figure yet — the scripted Standalone burst could not reach the WebView without taking the screen from the user. See `stages/4-polish/LISTENING.md` Table C row 1.
- ~~Factory bank is `Init` only until Phase 4.1~~ — closed 2026-09-12: 18 presets, and the constructor sweeps `Factory/` on a content-stamp mismatch (`1.0.0+<sha256(bank)[0:12]>`), so orphans cannot survive a bank edit.

## Human rows open

All fifteen are folded into one list with their sources: **`.planning/stages/4-polish/LISTENING.md` Table C** (Stage 3 rows 1–6, Stage 4 Round A rows 1–6, three Round B rows). Tables A and B of the same file are the QUAL-04 listening pass itself.

## v1.1 candidates

- Bandlimited: the analytic fallback above the muting note, and a soft knee on pitch tracking.
- The F-lattice (13 coefficient sets over F ∈ [0.25, 2], per-voice block-rate lerp) so Terrain Freq is a live per-voice destination in Bandlimited mode.
- Baked sources (the superseded v1 mesh / volume / terrain bakes) and RGB morphing for imported images.
- The O-Prism v1.26.1 `.octave-stretch-slider { min-width: 0 }` + `.settings-toggle` port (a separate `/improve O-Prism`).
- Promote `tests/tools/cdp-font-probe.js` to `scripts/`; delete the now-unused `TerrainImage::view`.
- Re-run ASan once the macOS 26 / Xcode 26.3 init hang is resolved.
- A `STRATA_CI=1` wall-clock band for the CI runner, so H7 / scheduler / import / H10 timing rows have runner-appropriate thresholds — and a **ratio** form for the H7 Bandlimited row, whose `+2.0` absolute slack holds on the dev machine (Bandlimited / 2× = 1.02×) but not on the CI runner (1.15–1.97×).
- Memoise the base64 string per import slot revision (persistence currently re-encodes on every save).

## Additional Notes

- Fork base: O-Prism v1.24.0 (engine, UI shell, tuning engine, preset manager, i18n).
- v1.0 is the live wave-terrain oscillator (design v2, 2026-09-08); baked sources (the v1 mesh / volume / terrain bakes) are v1.1.
- Research roadmap stage numbers (§7.4) do not map 1:1 onto workflow stages — see BRIEF.md Technical Notes.
