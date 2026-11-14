---
milestone: factory-sources-expansion
domain: mixed
execute_agent: general-purpose
version_bump: minor
base_version: 1.1.0
target_version: 1.2.0
created: 2026-11-13
---

# Plan: Factory sources expansion

**Plugin:** O-GranulatorInstrument
**Milestone:** factory-sources-expansion
**Created:** 2026-11-13
**Phase:** Plan

---

## Summary

**Improvement:** 41 embedded factory sources (the 8 existing untouched, 14 samples from Taylor's recordings, 19 synthetic), each with a category and a root that the Factory picker writes to the stream; a grouped picker for the larger set.
**Approach:** One manifest generates the table, the CMake list and the golden sha256. Samples go through an ingest script (48 kHz, 16-bit, level rule, folded seam on the 7 long textures); synthetics come from `factory_gen.cpp`. The root is written in `loadFactorySource`. No engine, parameter or state-format change.
**Version:** 1.1.0 → 1.2.0 (MINOR)

---

## Decisions settled at plan (Taylor, 2026-11-13)

These replace the matching lines of CONTEXT.md and close RESEARCH.md C1–C6.

| # | Decision |
|---|---|
| C1 | Crystal root **Db5 (73) +17 ct** (was Db4 (61) +18). |
| C2 | Guitar Overtones root **D2 (38) +3 ct** (was D2 −10). Guitar Drift stays D2 (38) −10 ct. |
| C3 | Contrabassoon root **G#0 (20) −19 ct**. |
| C4 | Stick Snaps and Scissors ship as cut (0.18 s gaps). The level row uses peak plus loud-frame level, not RMS. |
| C5 | `vinyl` is replaced by **`breath`** (air noise through a resonance at C3). The synthetic count stays 19. |
| C6 | Picker is a **headed 4-column panel**, no scrolling at 41; `max-height` + scroll stay as a guard. |

### The 41, as the manifest will carry them

Name is explicit per entry. For the old 8 it equals `factoryDisplayName (id)`. Root is the sounding fundamental; C3 / 0 ct where not stated.

| Column | Category | Sources (id → name where they differ) |
|---|---|---|
| 1 | Tonal (8) | harmonic, bowed, reed, organ, saw_stack, chord_minor, chord_sus, crystal (73 / +17) |
| 1 | Voice (2) | vowel, choir |
| 2 | Plucked / Struck (9) | pluck, bell, fm_bell → "FM Bell", glass, kalimba, marimba_bar, metal_plate, guitar_overtones (38 / +3), guitar_drift (38 / −10) |
| 3 | Texture (9) | noise_bed, dust, wind, rain_synth → "Rain", breath, shimmer, fire, bubbles, metal_grate |
| 3 | Drone (3) | drone, sub, contrabassoon (20 / −19) |
| 4 | Rhythmic (5) | pulse, clicks, pulse_triplet, stick_snaps, scissors |
| 4 | Field (5) | stream, park, waves, woods, garden |

- Table order is this reading order (column 1 top to bottom, then column 2, and so on), with one exception that is not optional: **indices 0–7 stay the old 8 in today's order**; the picker sorts by category for display. Stubs move to 41 / 42.
- `sub` and the two chords: the manifest root is whatever fundamental the recipe actually sounds (a chord's root is its lowest chord tone). If `sub` is rendered below C3, its root says so.
- Seam flag: stream, park, waves, woods, garden, fire, bubbles.

---

## Task Breakdown

### Wave 1 — pipeline, with the 8 existing sources only

#### Task 1: Manifest and table generator

**Outcome:** `Resources/factory/manifest.json` describes the 8 existing sources (id, name, category, root note, root cents, channels, frames, kind, seam, reference partial, `kHash`, `kFull`). `tools/factory_table.py` generates from it: the table include for `Source/dsp/GiFactory.h`, the CMake source list, and `tests/golden/factory-sources.sha256`. `FactoryEntry` gains `name`, `category`, `rootNote`, `rootFineCt`. `SlotLoader::requestFactoryLoad` takes the name from the entry.

**Verification:**
- [x] The generated `factory-sources.sha256` is byte-identical to the committed one.
- [x] The 8 pins in the generated table equal today's literals; FSP1 passes unchanged.
- [x] `factory_table.py --check` passes on a clean tree and fails when one manifest field or one FLAC byte is changed (run both, report both).
- [x] FSP1 asserts `entry.name == factoryDisplayName (id)` for the old 8.
- [x] `--pg4` and the preset gates pass with no row changed; 64 presets bitwise.

**Dependencies:** None
**Estimated effort:** Medium

#### Task 2: Harness pin mode and a generator that cannot touch the old 8

**Outcome:** `--pin-factory <dir>` decodes every FLAC in a folder and prints `kHash` / `kFull` in manifest form. `--gen-factory` writes only ids whose file is missing; `--force` no longer reaches the old 8.

**Verification:**
- [x] `--pin-factory Resources/factory` prints the 8 existing pins exactly.
- [x] `--gen-factory Resources/factory --force` leaves the 8 files byte-identical (`shasum -c tests/golden/factory-sources.sha256`).

**Dependencies:** Task 1
**Estimated effort:** Small

#### Task 3: Ingest script

**Outcome:** `tools/factory_ingest.py` (from `research/ingest_proto.py`) reads a window from an original, resamples to 48 kHz, applies the level rule (90th-percentile 400 ms frame at −20 dBFS, peak ≤ −1 dBFS), applies either the 0.5 s equal-power fold (seam sources, no end fades) or 10 ms fades, dithers to 16-bit (TPDF, fixed seed) and writes FLAC. Windows, gains and source paths live in the manifest. The one-shot collections are assembled as in `audition_cut.py`.

**Verification:**
- [x] Two runs give byte-identical FLACs.
- [x] Output sizes within 1 % of RESEARCH.md table 1.
- [x] The script refuses a source path outside the allowed folders listed in CONTEXT.md (plus the one contrabassoon file).

**Dependencies:** None (parallel with Tasks 1–2)
**Estimated effort:** Medium

### Wave 2 — content

#### Task 4: Ingest the 14 samples

**Outcome:** 14 FLACs in `Resources/factory/`, manifest entries complete except pins.

**Verification:**
- [x] Each decodes to 48 kHz at the documented length (seam sources 0.5 s shorter than the BRIEF.md cut).
- [x] Sample total ≤ 7.6 MB; largest ≤ 3 MB.

**Dependencies:** Task 3
**Estimated effort:** Small

#### Task 5: The 19 synthetic recipes

**Outcome:** `factory_gen.cpp` generates fm_bell, glass, bowed, reed, organ, choir, sub, saw_stack, chord_minor, chord_sus, kalimba, marimba_bar, metal_plate, wind, rain_synth, breath, shimmer, clicks, pulse_triplet. Recipes per RESEARCH.md §8 (`fm_bell` two-operator FM with a decaying index; `glass` 2 s, partials 2.32 / 4.25 / 6.63, no hum; `kalimba` one strong overtone near 5.4×; `marimba_bar` 4× and 10×, fast decay). Deterministic, fixed seeds.

**Verification:**
- [x] Two runs give byte-identical files.
- [x] Synthetic total ≤ 11 MB; stereo noise sources ≤ 8 s.
- [x] A likeness table in SUMMARY.md (spectral centroid, decay time, strongest three partials) for bell / fm_bell / glass and pluck / kalimba / marimba_bar. Two that read alike are reported to Taylor, not swapped.
- [x] Each pitched recipe's measured fundamental is within ±3 ct of its manifest root.

**Dependencies:** Task 2
**Estimated effort:** Large

#### Task 6: Pins and FSP1 over 41

**Outcome:** Build, `--pin-factory`, pins into the manifest, regenerate, rebuild. `kCuratedFactoryCount` is 41. FSP1 covers all 41 with the old 8 pins kept as literals in the row (not read from the generated table), total ≤ 25 165 824 B (24 MiB), each ≤ 3 145 728 B. FSP1n still fails. CMake budget comment updated.

**Verification:**
- [x] FSP1 41 / 41, FSP1n fires.
- [x] `factory_table.py --check` passes; the first 8 lines of the golden sha256 unchanged.
- [x] `ui_gates_31.h`, `ui_gates_32.h`, `engine_gates_26b.h` (stubs by id) pass.
- [x] Preset gates: 64 presets bitwise.
- [x] Measured embedded total recorded in SUMMARY.md and in the CMake comment.

**Dependencies:** Tasks 1, 4, 5
**Estimated effort:** Medium

### Wave 3 — behaviour, rows, picker

#### Task 7: Root write

**Outcome:** `GranulatorInstrumentProcessor::loadFactorySource` writes `sN_root_note` and `sN_root_fine` for the picked lane's stream after `requestFactoryLoad` returns true: gesture begin / `setValueNotifyingHost (convertTo0to1 (v))` / gesture end, per parameter, only when the value differs. No write on `recording`, `unknown`, `badSlot`. No new native function.

**Verification (new rows in `preset_gates_4.h`):**
- [x] **FR1**: for all 41, pick through `loadFactorySource` from a non-default root and read back note exactly and cents within 0.001. Lane 3 writes stream 3 only.
- [x] **FR1r**: each refusal leaves both parameters as they were.
- [x] **FR1e**: a pick whose root already matches produces zero gestures (listener count).
- [x] **FR1n** control: the write removed by a test hook → FR1 fails.
- [x] Preset load, session restore and import leave the root alone (one row, three cases).

**Dependencies:** Task 6
**Estimated effort:** Small

#### Task 8: Level, seam and pitch rows

**Outcome:** Three rows with controls.
- **FLV1**: all 41, peak ≤ −1.0 dBFS and loud-frame level in [−33, −18] dBFS. Control: one source scaled +12 dB fails.
- **FSM1**: the 7 seam sources read circularly, centre 100 ms of the fold within ±1.5 dB of the mean power of the two mixed segments, wrap step ≤ the body's 99.9th-percentile sample step. Control: the linear-fade cut of the same windows (harness fixture, not shipped) fails. If the garden control margin is under 0.3 dB, tighten to ±1.2 and say so.
- **FPT1**: crystal, guitar_overtones, guitar_drift, contrabassoon picked through `loadFactorySource`, C3 played, long grains, no spray; the manifest's reference partial within ±10 ct of k × 130.81 Hz (k = 1, 7, 2, 1; contrabassoon by autocorrelation). Control: root left at C3 fails for all four. Model: `engine_gates_21.h:1189–1244`.

**Verification:**
- [x] The three rows pass, the three controls fire, measured values in SUMMARY.md.

**Dependencies:** Tasks 6, 7 (FPT1 needs the root write)
**Estimated effort:** Medium

#### Task 9: Picker

**Outcome:** `getFactorySources` returns id, name and category. `#factory-list` becomes a headed 4-column panel on the `#scale-list` pattern (20 px rows, small-caps group headings with the rule, 140 px columns, 12 px gaps), groups placed as in the table above, `max-height` + `overflow-y: auto` as the guard, clamped inside the window on all four sides. Empty lane only, as in 1.1.0. Keyboard: Up / Down and type-ahead in flat reading order, Left / Right to the same row of the neighbouring column (clamped to its last item), Home / End, Enter picks, Escape closes and returns focus. Type-ahead matches on names. Seven category keys in en / fr / zh; source names not localized.

**Verification:**
- [x] `ui_frontend_check.js`: native pins stay 36 / 22; new checks for 41 items, 7 headings, 4 columns, no vertical scroll at 41, panel inside the window when opened from each of the 4 lanes, the keyboard path above, `aria-activedescendant` following the highlight.
- [x] check-i18n, fr / zh lint, `tests/i18n-states.json` state for the open picker, three-language fit (the widest heading is the French "Plucked / Struck" key; measured with `nowrap`).
- [x] Headless capture of the open panel in en / fr / zh attached to SUMMARY.md, checked against the chosen layout.
- [x] Picking a source updates the lane name and the stream's Root fields without reopening anything.

**Dependencies:** Task 6 (categories), Task 7 (Root readout)
**Estimated effort:** Medium

#### Task 10: Version, build, validators, records

**Outcome:** Version 1.2.0 in `CMakeLists.txt`; CHANGELOG `## [1.2.0]` (bracket form) with the note that a 1.2.0 session using a new source shows that lane as missing in 1.1.0 / 1.0.0; PLUGINS.md row; `./scripts/build-and-install.sh O-GranulatorInstrument`; SUMMARY.md.

**Verification:**
- [x] Build without warnings; binary-data compile time recorded.
- [x] Full harness: every existing mode that passes at 1.1.0 passes (`--pg4`, `--egr1`, `--egr2`, `--eg24` … `--ug33`), same PASS counts plus the new rows.
- [x] auval passes; pluginval level 10 on VST3 and AU; Standalone opens and the picker loads Contrabassoon in tune at C3 by ear check left for Taylor.
- [x] `check-ui-labels`, repo check-i18n.

**Dependencies:** Tasks 7, 8, 9
**Estimated effort:** Medium

---

## Dependency Graph

```
T1 ──> T2 ──> T5 ──┐
T3 ──> T4 ─────────┴──> T6 ──> T7 ──┬──> T8 ──┐
                                    └──> T9 ──┴──> T10
```

**Parallelizable:** T3 with T1–T2; T4 with T5; T8 with T9.

---

## Execution Order

| Order | Task | Dependencies | Parallelizable With |
|-------|------|--------------|---------------------|
| 0 | Backup `backups/O-GranulatorInstrument/v1.1.0/` | None | - |
| 1 | T1 Manifest and generator | None | T3 |
| 1 | T3 Ingest script | None | T1, T2 |
| 2 | T2 Pin mode, safe generator | T1 | T3 |
| 3 | T4 Ingest 14 samples | T3 | T5 |
| 3 | T5 19 synthetic recipes | T2 | T4 |
| 4 | T6 Pins, FSP1 over 41 | T1, T4, T5 | - |
| 5 | T7 Root write | T6 | - |
| 6 | T8 Level / seam / pitch rows | T6, T7 | T9 |
| 6 | T9 Picker | T6, T7 | T8 |
| 7 | T10 Version, build, validators | T7, T8, T9 | - |

---

## Risk Notes

1. **The old 8 change by accident.**
   - Impact: 64 presets stop being bitwise; sessions relink.
   - Mitigation: Wave 1 runs with only the 8 and must reproduce today's golden file byte for byte (T1); write-missing-only generator (T2); old pins as literals in FSP1 (T6); `shasum -c` after every generator run.

2. **Budget margin (about 2.4 MB) rests on an estimate for the synthetics.**
   - Impact: over 24 MiB at T6.
   - Mitigation: 11 MB cap and the 8 s rule in T5, checked before pinning. If over: shorten stereo noise sources first; do not drop a source without asking.

3. **FLAC bytes land in a public history for good.**
   - Impact: every re-commit of a changed FLAC adds its size to the repo permanently (about 18 MB for one full set).
   - Mitigation: nothing under `Resources/factory/` is committed before T6 passes; one commit for the final set.

4. **Samples are not reproducible from the repo.**
   - Impact: the originals live under `/Users/taylorbrook/Projects`.
   - Mitigation: the committed FLACs are the source of truth; the manifest records path, window and gain.

5. **Keyboard path across columns of unequal height.**
   - Mitigation: flat order for Up / Down and type-ahead, clamped Left / Right, gated in T9.

6. **Root write on a loaded lane** (harness only): the old tape plays at the new root for the length of the decode. Not reachable from the page; rows that pick onto a loaded lane wait for the swap before measuring.

7. **Windows arm** runs only at `/publish`. Binary data is numeric arrays (no MSVC string limit); nothing predicts a failure, nothing here proves its absence.

---

## Domain Agent Instructions

**Execute Agent:** general-purpose

**Rules:**
- No engine change, no parameter change, no state-format change. `rootNoteHint` stays unwritten.
- Never rewrite, re-encode or reorder the 8 existing FLACs or their table indices.
- The root write is a message-thread call in `loadFactorySource`; nothing in `parameterChanged`, nothing on the audio thread.
- Native function names stay at 36 (page uses 22).
- Every new row has a control that is shown to fire, and a stimulus that is shown to reach the thing it measures.
- A deviation from a row's threshold, a source that has to be dropped or swapped, and two synthetics that sound alike are reported, not decided.
- Long commands (full harness, pluginval) run in the background. Commits are path-scoped (`git commit -- plugins/O-GranulatorInstrument PLUGINS.md`), with branch and staging re-checked right before each. No tag.
- After the build: the cache-clear and dual-variant sweep, by `./scripts/build-and-install.sh`.

**Read first:**
- `CONTEXT.md`, `RESEARCH.md`, `BRIEF.md` in this folder — decisions, measurements, sample windows. The table at the top of this plan wins where they differ.
- `Source/dsp/GiFactory.h` — the table and its pin contract.
- `tests/render-harness/factory_gen.cpp`, `preset_gates_4.h` (FSP1, line 172 on) — generator and row style.
- `Source/PluginProcessor.cpp:1678` (`loadFactorySource`), `:270` (the `rec_arm` gesture pattern).
- `Source/ui/public/index.html` — `#scale-list` (CSS 474–494, builder near 3516), `#factory-list` (610–614, 3961–4042).
- `research/ingest_proto.py`, `research/seam.py`, `research/pitch.py`, `audition_cut.py`.

---

## Success Criteria

1. [ ] 41 sources listed by the picker, in 7 categories, in three languages.
2. [ ] The 8 old pins, their sha256 lines and the 64 presets unchanged.
3. [ ] Every pick writes the stream's root; crystal, guitar_overtones, guitar_drift and contrabassoon play at the key's pitch (class and octave).
4. [ ] Embedded total ≤ 24 MiB, each source ≤ 3 MiB.
5. [ ] The seven long textures wrap without a dip or click.
6. [ ] All tasks completed and verified, every control fired.
7. [ ] Build without warnings; auval passes; pluginval passes at level 10.

---

## Approval

Approved 2026-11-13 by running the execute phase. Executed the same day; deviations D1–D8 are in SUMMARY.md.

---

*Generated by improve-milestone plan phase*
