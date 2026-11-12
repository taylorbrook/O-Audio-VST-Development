# Stage 4: Polish / Validation - Context

## Discussion Summary

**Date:** 2026-11-11
**Participants:** User, Claude

**Inputs:** `ROADMAP.md` § Stage 4, `REQUIREMENTS.md` (FUNC-08, COMPAT-02, QUAL-02, every `partial` row), `stages/3-gui/VERIFICATION-3.3.md` (Stage 3 automated complete; UI-02 partial), carried lists D50 / PLAN-3.3:322 / VERIFICATION-3.3:118–119, PD1 (PLAN-3.1).

**Entry state:** Stage 3 verify ⚠️ PARTIAL on `77d5ad04` / `bf236204`. Every automated gate green (harness 431 / 0, parity 168 / 0 + GV 2 / 0, drift 0 / 13, `--cpu26` C25m 29.62 % / C25tm 6.91 %, auval + pluginval 10 SC on / off, five UI gates 46 states). HANDS-ON-SESSION-3.3 (D47) not yet run.

## Requirements Confirmed

- **FUNC-08** — generated factory source set + **64 factory presets** that load out of the box.
- **PERF-02** — the typical-budget verdict (≤ 12 %, D20) is re-checked on the real factory presets; the frozen proxy stays as a regression row.
- **QUAL-02** — sidechain absent / unrouted degrades cleanly, verified in Logic (AU unrouted key bus) and Live (VST3 no aux).
- **COMPAT-02** (nice) — engine path proven at 2.5b (MP / NE rows). No Dorico or MPE-controller DAW pass this stage (D57); stays **partial**, carried to 1.0.x.
- **UI-01 / UI-02 / FUNC-01–07 / FUNC-09 / DSP-02/03/07 / QUAL-01 / PERF-03** — the remaining `partial` rows close on the combined human session (D51). UI-01's Windows-drop arm stays open (D54).
- **Release** — 1.0.0, ready for `/publish` (tag only via `/publish`).

## Constraints Identified

- Trunk-based, path-scoped commits (`plugins/O-GranulatorInstrument`, `PLUGINS.md`); never tag outside `/publish`.
- LOCKED contracts: `parameter-spec.md` (261 params, dest 126 / src 18 append-only). The `output_gain` default fix (D55) moves the param-dump golden → re-pin with a recorded reason, checksum updated in STATUS.
- Presets (suite rules): never set `output_gain`; reset-to-defaults first; factory normalised values ignore skew → store denormalised / verify round-trip; `rec_arm` excluded (3.1).
- Factory sources ≤ 8 MB total FLAC in binary data; pinned decoded XXH64 per source (2.6 FA1 pattern); stubs `stub_tone` / `stub_noise` replaced by id.
- No Windows machine in the loop. No Reaper, Dorico or MPE hardware pass available.
- Parity rows stay against the fork pin `82406fce` (D11).
- CHANGELOG headings must be `## [X.Y.Z]` (release-notes extraction).
- auval: 4 LFO-rate default warnings recorded at 3.3 verify — check and resolve or document.

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| D51 | D47 hands-on | **Folded into Stage 4.** One combined human session at Stage 4 verify: HANDS-ON-2.1–2.6 + 3.1–3.3 (79 + 61 rows) + new Stage 4 rows (presets, DAW passes). Plan / execute not gated on it. | Keep momentum; one listening session instead of two. Defects found go into a fix wave before 1.0.0. |
| D52 | Factory sources | **Generated in-harness**, extending `--gen-factory` (2.6). 6–8 sources (e.g. harmonic tone, noise bed, pluck, formant / vocal-ish, metallic / bell, texture, drone, rhythmic). Deterministic, pinned hashes, byte-identical regeneration. | Zero licence risk, reproducible across platforms, no provenance paperwork. |
| D53 | Factory presets | **64 presets, sources-only (PD1 kept).** Each references factory sources by id; no embedded blobs. `parameter-spec.md:355` gets an addendum note (not an edit). | User choice. Bank spans categories; sources-only keeps preset files small and matches 3.1 behaviour. |
| D54 | Windows | **CI build only.** Windows VST3 must build green in CI; WebView2 static link + `withBackend(webview2)` + `withUserDataFolder()` grep-gated. Drop / browse + DAW load = a deferred hands-on row run when a machine is available; UI-01's Windows arm stays open. | No machine in the loop; CI catches compile / link regressions. |
| D55 | Carried D50 fixes | **All four:** `WavFailed` surfaced after a take (toast), one toast per failing lane, bounded save-worker join on a hung disk at close, `output_gain` default → exact 0 dB (param-dump re-pinned). | User choice. Last one is a contract move — flagged, re-pin required. |
| D56 | Release | **1.0.0 public**, ready for `/publish`. `VERSION 0.1.0 → 1.0.0`, CHANGELOG `## [1.0.0]`, PLUGINS.md row. Stage ends ready-to-tag; tagging only via `/publish`. | User choice. |
| D57 | DAW passes | **Logic (AU, SC Record-in, unrouted key bus) + Ableton Live (VST3, aux / sidechain into Record-in).** Reaper, Dorico NE and MPE controller not run; COMPAT-02 stays partial (harness-proven). | Hosts actually available. |
| D58 | Fork drift | **Record only.** Diff parent O-Granulator HEAD vs `82406fce`, write `FORK-MANIFEST.md` classifying each change; port only defect fixes touching shared leaves (each with a parity re-run). Pin unchanged. | Avoid 1.0.0 scope creep. |
| D59 | i18n | FR + zh-Hans for every new string (toasts, preset category labels if shown) through the suite lint gates; fr `vstate.held` "tenue" confirmed at the hands-on row. | Suite rule. |

## Open Questions (for research)

1. **Preset bank design for 64:** category split (pads / textures / plucks / keys / rhythmic / drones / FX / MPE / Record-in templates / init variants), naming scheme, and whether preset names are localized (suite precedent).
2. **CPU verdict over 64 presets:** every preset ≤ 40 % worst-case at 16 × 4? Typical ≤ 12 % on median / per-preset single-note-chord script? Define the render script and thresholds.
3. **Preset proof rows:** load-from-fresh, load-after-dirty state (reset-to-defaults), save / reload round-trip bit-identical, `output_gain` untouched, factory sources resolve with `fsTouches 0`, no clicks on preset switch while playing.
4. **Source synthesis:** which 6–8 recipes give the bank enough range; size budget per source; mono / stereo; rate (48 k as stubs?); root notes for key tracking.
5. **`output_gain` fix mechanics:** range interval vs snap (see memory: stepped range + negative start); preset / state migration for saved sessions (old default → new); exact param-dump delta.
6. **auval LFO-rate default warnings:** cause and fix vs document.
7. **CI:** does the release workflow configure this plugin cleanly (tagged-plugin-only configure, Windows WebView2, binary-data size); signing / notarization of the AU + VST3 + Standalone.
8. **Fork drift:** parent commits since `82406fce` and which touch forked leaves.
9. **Human session layout:** merge HANDS-ON-2.1–3.3 + Stage 4 rows into one ordered `HANDS-ON-SESSION-4.md` (Logic pass, Live pass, preset audition pass).

## Out of Scope (Stage 4)

- Dorico playback template / `.doricolib` and MPE hardware pass (COMPAT-02 → 1.0.x).
- Reaper pass.
- Moving the fork pin.
- Real Windows hands-on (deferred row).
- Formant-preserving scan (v2).

## Next Phase

Ready for: research phase (`/plugin-research O-GranulatorInstrument 4-polish`).

> Rework R1 (per-stream feedback + reverb, D60–D69): see `CONTEXT-R1.md`.
