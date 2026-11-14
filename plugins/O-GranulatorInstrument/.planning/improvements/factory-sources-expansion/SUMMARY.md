# Summary: Factory sources expansion

**Plugin:** O-GranulatorInstrument
**Milestone:** factory-sources-expansion
**Created:** 2026-11-13
**Phase:** Execute

---

## Implementation Summary

**Version:** 1.1.0 → 1.2.0 (MINOR)
**Domain:** mixed
**Agent:** general-purpose (run in the main session)

41 embedded factory sources (the 8 of 1.0.0 untouched, 14 samples, 19 generated), each with a category and a root. A Factory pick writes the lane's stream root. The picker is a headed 4-column panel. No engine, parameter or state-format change. Built and installed as 1.2.0; not tagged.

**Result in one table**

| Check | Result |
|---|---|
| Full harness, final tree | **555 PASS / 0 FAIL** (1.1.0 baseline run the same day: 544 / 0; the 11 new rows account for the difference) |
| 64 preset renders against the 1.1.0 binary | 64 / 64 bitwise (PT0), at Wave 1 and again on the final tree |
| The 8 old sources | sha256 lines 1–8 unchanged, `shasum -c` 8 / 8 after every generator run, FSP1 8 / 8 on hand-written literal pins |
| Embedded total | 19 750 021 B of 25 165 824 B (78 %); largest `stream.flac` 1 364 847 B of 3 145 728 B |
| auval | PASS (the 4 known LFO-rate warnings) |
| pluginval level 10 | VST3 SUCCESS, AU SUCCESS |
| UI gates | `ui_frontend_check.js` ALL PASS (166 lines, natives 36 / 22), `--nc` live; `ui_factory_picker_check.js` ALL PASS (22 lines, 4 controls fire); `ui_binding_probe.js` ALL PASS, `--nc` live; `preset_name_fit.js` ALL PASS; check-i18n, fr lint, zh lint clean; check-ui-labels ALL CHECKS PASSED (49 states) |
| Build | no warning from `plugins/O-GranulatorInstrument/Source`; factory binary data compiles clean in 31 s |

---

## For Taylor — things to hear or decide

1. **Ear check (left for you, as planned).** In the Standalone or a DAW: empty lane → Factory → Contrabassoon, play C3. It should sound a C3. The harness measures −1.2 ct there (FPT1), but nobody has listened.
2. **Two pairs read alike by the numbers.** Reported, not swapped:
   - `bell` and `glass`: both lead with the fundamental and three inharmonic partials (attack centroid 217 Hz vs 246 Hz). They differ in length (5.9 s vs 1.7 s to −30 dB) and in the partial series (2.00 / 2.76 / 5.40 with a hum vs 2.32 / 4.25 / 6.63, no hum).
   - `pluck` and `kalimba`: attack centroid 233 Hz vs 247 Hz, both ring for the length of their file. They differ in overtones (harmonic 2 / 3 / 5 vs one overtone at 5.4×, 6 dB under the fundamental).
   `kalimba` and `marimba_bar` were closer still on the first render (148 vs 137 Hz); I retuned both recipes before anything was pinned (kalimba overtone up and longer, marimba shorter with a stronger 4× and mallet). They now read 247 Hz / 2.96 s vs 161 Hz / 1.00 s.
3. **`sub` is rendered at C1 (32.70 Hz), root 24.** The plan left the octave open. With the root written by the pick, C1 on the keyboard plays it untransposed; at C3 it is a 131 Hz tone. Harmonics 2–4 are mixed in quietly so it reads on small speakers.
4. **`breath` reads −3.8 ct, outside the ±3 ct criterion.** It is noise through a resonance; the resonance coefficient is exactly C3, and the band-centroid estimate on 8 s of noise carries a few cents of spread whatever the seed. I did not add a tone or change the seed to move the number. Every other pitched recipe is within ±1.5 ct (table in Task 5).
5. **Stick Snaps sits 0.14 dB above the level row's floor** (loud frame −32.86 dBFS, floor −33). It passes; the margin is thin. This is the C4 call (ship as cut) showing up in the row.
6. **French and Chinese category names are machine-drafted** (`reviewed: false` / `'mt'`), like the rest of the table: Tonales, Voix, Pincées / Frappées, Textures, Bourdons, Rythmiques, Terrain; 乐音, 人声, 拨弦 / 敲击, 纹理, 持续音, 节奏, 实地录音.

---

## Change after execute: Guitar Overtones (Taylor, 2026-11-13)

Taylor: the source held two separate notes; trim to the first and check the pitch.

- **Cut.** The take has a note from 6.43 s and a second from 11.38 s (a finger noise at 10.95 s). The window is now 6.3–10.85 s of `Dovertones.wav` (was 6.3–16.2): 4.55 s, 108 632 B (was 9.90 s, 241 442 B). The note has fallen 30 dB by the cut; 10 ms fades as before.
- **Pitch.** The first note is one natural harmonic, steady at 513.75 Hz = **C5 −32 ct** (four windows read −31.5 … −31.9; its octave partial agrees at −31.4). Nothing sounds below it: 73 Hz and 147 Hz are 86 dB or more under it. So the root is now **C5 (72) −32 ct**, replacing D2 (38) +3 (call C2 at plan), which named the open string, not a pitch anyone hears in this cut. With the old root a C3 key would have sounded near Bb5.
- **Row.** FPT1 reads this source at k = 1 now: +0.5 ct at C3; its control still fires (−61.7 ct, strength 0.00).
- Re-pinned, table regenerated, `--check` OK, the 8 old sha256 lines unchanged. CHANGELOG and the CMake size comment updated.

---

## Deviations from the plan

| # | Plan | What was done | Why |
|---|---|---|---|
| D1 | FLV1: peak ≤ −1.0 dBFS | peak ≤ −1.0 dBFS + one 16-bit step (−0.9997) | `dust` (1.0.0, frozen) peaks at −0.99985 dBFS: its generator capped at −1 dBFS and then rounded to the 16-bit grid. `rain_synth` and `clicks` follow the same rule and land on the same value. The clause as written fails on a file that cannot change. |
| D2 | Picker checks in `ui_frontend_check.js` | Static half there (section [17], 7 checks); layout and keyboard half in a new `tests/ui_factory_picker_check.js` (Playwright) | `ui_frontend_check.js` is a static closure with no layout engine; scroll, frame bounds and focus need a browser. |
| D3 | FSM1 reference: "the mean power of the two mixed segments" | Read from a fixture, `tests/fixtures/factory-seam/<id>.flac` (7 files, 168 kB in total), written by the ingest script | The two segments the fold mixed are not in the shipped file. Each fixture holds 3 × 100 ms: head centre, tail centre, and the centre of the same fold made with linear gains (the control). |
| D4 | — | `tests/ui-stub/generic-overrides.json` regenerated by the harness | Its `getFactorySources` block was typed in by hand at 1.1.0 although the file says "never hand-edit". The editor and the dump now share one encoder (`GiUiJson::factorySourcesToVar`); the regenerated file differs from the committed one in that block only. |
| D5 | Left / Right: "the same row of the neighbouring column (clamped to its last item)" | The nearest rendered row of the neighbouring column | Columns carry headings at different heights, so "row k" of two columns is not level on screen. Nearest-by-position gives the clamp as a consequence and is gated in FK3. |
| D6 | — | The open-picker state sits last in `tests/i18n-states.json`; the probe's state-count pin moved 48 → 49 | check-ui-labels flips every toggle after each state and does not put them back, so a state inserted mid-file changed the precondition of `streams-2-4-on` (it failed on the first run for that reason, not for a label). |
| D7 | — | `openScaleList` also closes the Factory list | The two panels could otherwise be open together when the scale list is opened by script or keyboard. |
| D8 | `--gen-factory` budget verdict | The generator prints totals only; the budget is FSP1's | The generator no longer writes the old 8, so its own total is not the embedded total. |

Nothing was dropped or swapped. No threshold was loosened apart from D1.

---

## Tasks Completed

### Task 1: Manifest and table generator ✓

- `Resources/factory/manifest.json` (41 entries; id, name, category, root, channels, frames, kind, seam, reference partial, pins, and for samples the ingest block with source, window and gain).
- `tools/factory_table.py` writes `Source/dsp/GiFactoryTable.inc`, `Resources/factory/sources.cmake`, `tests/golden/factory-sources.sha256`, `tests/render-harness/factory_specs.inc`. `--check` compares all four with the disk, reads each FLAC header against the manifest, and fails on a missing pin. `--apply-pins` refuses to move a pin of the old 8.
- `FactoryEntry` gains `name`, `category`, `rootNote`, `rootFineCt`; `GiFactory.h` includes the generated table; `SlotLoader::requestFactoryLoad` takes the name from the entry.

**Verification (run with only the 8, before any new content):** generated sha256 byte-identical to the committed file; 8 pins equal; `--check` clean rc 0, one field changed rc 1 (2 files drift), one FLAC byte flipped rc 1 (sha256 drifts), restored rc 0; FSP1 8 / 8 including `name == factoryDisplayName (id)`; `--pg4` 22 / 0 as on 1.1.0; 64 presets bitwise against the 1.1.0 binary.

### Task 2: Pin mode, safe generator ✓

- `--pin-factory <dir>` prints one JSON line per FLAC. `--gen-factory` writes only missing recipes; `--force` rewrites existing ones except the old 8, which the function never writes (a missing one is an error).

**Verification:** `--pin-factory Resources/factory` printed the 8 existing pins exactly; `--gen-factory Resources/factory --force` → "19 written, 8 kept", `shasum -c` 8 / 8 OK.

### Task 3: Ingest script ✓

- `tools/factory_ingest.py`: window → 48 kHz → fold or 10 ms fades → level rule (gain floored to 0.01 dB and recorded in the manifest) → TPDF dither seeded by the id → 16-bit FLAC. Collections assembled as in `audition_cut.py`.

**Verification:** three runs byte-identical (two with the recorded gain, one re-measuring from a null gain; the manifest comes out the same). Total 7 424 568 B against the research table's 7 425 528 B (−0.01 %). Refuses a file in another folder, a `..` escape and a `jeffAudio` path (rc 1 each, nothing written).

### Task 4: The 14 samples ✓

14 FLACs, 7 424 568 B, largest 1 364 847 B. Lengths: seam sources 0.5 s under the BRIEF cut (stream 10.30, park 9.90, waves 10.40, woods 10.20, garden 9.80, fire 11.50, bubbles 8.94 s).

### Task 5: The 19 recipes ✓

`tests/render-harness/factory_gen.cpp`: 19 recipes with `finishNew` (the old level targets, and never above a −20 dBFS loud frame).

**Verification:** two fresh runs and the files in `Resources/` byte-identical (19 / 19). Synthetic total 9 002 930 B (cap 11 MB); stereo noise sources 8 s.

Measured fundamental against the manifest root (`tools/factory_measure.py`; ensembles by the power centroid of the cluster):

| source | cents | source | cents |
|---|---|---|---|
| bowed, reed, organ, fm_bell, glass, kalimba, marimba_bar, sub | 0.00 | chord_minor | −1.19 |
| saw_stack | +0.03 | choir | −0.82 |
| chord_sus | +0.05 | shimmer (8th harmonic) | −1.50 |
| metal_plate | −0.07 | **breath** | **−3.76** (item 4 above) |

Likeness, first 250 ms:

| source | centroid | −30 dB at | strongest partials (ratio, dB) |
|---|---|---|---|
| bell | 217 Hz | 5.90 s | 1.00 (0), 2.00 (−5), 2.76 (−6), 5.40 (−11) |
| fm_bell | 1117 Hz | 4.94 s | 11.5 (0), 9.5 (0), 8.0 (−1), 6.0 (−1) |
| glass | 246 Hz | 1.70 s | 1.00 (0), 2.32 (−3), 4.25 (−8), 6.63 (−13) |
| metal_plate | 785 Hz | 5.91 s | 1.00 (0), 2.50 (−4), 6.50 (−6), 4.00 (−6) |
| pluck | 233 Hz | 2.97 s | 1.00 (0), 2.00 (−9), 3.01 (−10), 5.04 (−15) |
| kalimba | 247 Hz | 2.96 s | 1.00 (0), 5.40 (−6), 2.00 (−28) |
| marimba_bar | 161 Hz | 1.00 s | 1.00 (0), 4.00 (−11), 10.0 (−34) |

### Task 6: Pins and FSP1 over 41 ✓

- **FSP1** 41 / 41, the old 8 on literal pins 8 / 8, total 19 750 021 B (after the Guitar Overtones re-cut, below), largest 1 364 847 B, stubs at 41 / 42. **FSP1n** fires (40 / 41, `dust PIN`).
- `factory_table.py --check` OK; first 8 sha256 lines unchanged; stub rows (`ui_gates_31`, `ui_gates_32`, `engine_gates_26b`) pass in the full run.
- By kind: old 8 3 455 333 B, generated 9 002 930 B, samples 7 291 758 B (7 424 568 B before the Guitar Overtones re-cut). Recorded in the CMake comment.

### Task 7: Root write ✓

`GranulatorInstrumentProcessor::loadFactorySource` (`Source/PluginProcessor.cpp`): after a queued load, one gesture per parameter, skipped when the value is already there. Test hook `testNoFactoryRootWrite`.

| Row | Measured |
|---|---|
| FR1 | 41 / 41 from sentinel roots, other three streams untouched each time; 5 sources off C3 |
| FR1n | hook on → 0 / 41 (control fires) |
| FR1r | "unknown" / "badSlot" / "badSlot" and Recording → "recording": roots kept; the same pick on another lane writes 20 / −19 |
| FR1e | begins / ends / values: drone on a fresh lane 0 / 0 / 0, contrabassoon 2 / 2 / 2, again 0 / 0 / 0, sub 2 / 2 / 2, drone 1 / 1 / 1 |
| FR2 | session restore, preset load, dropped file, Clear: root stays 55 / +12 with contrabassoon on the lane |

### Task 8: Level, seam and pitch rows ✓

| Row | Measured | Control |
|---|---|---|
| FLV1 | 41 / 41; peak max −0.9999 dBFS; loud frame −32.86 (stick_snaps) … −18.92 (vowel) | FLV1n: harmonic + 12 dB → loud −8.07, fails |
| FSM1 | 7 / 7; fold centre fire +0.44, bubbles −0.10, stream +0.46, park −1.03, waves +0.14, woods +0.70, garden +0.94 dB; wrap step ratio ≤ 0.44 | FSM1n: linear folds −1.97 … −4.03 dB, 7 / 7 fail; nearest margin 0.47 dB (garden), so ±1.5 is kept |
| FPT1 | crystal +3.8 ct, guitar_overtones +0.5 (k 1, after the re-cut; −3.7 at k 7 before), guitar_drift −5.7, contrabassoon −1.2 (period); margin 4.3 ct | FPT1n: root left at C3 → 4 / 4 fail |

FPT1n note: crystal fails on the strength clause only (a 0.07 component sits 3.5 ct from 130.81 Hz in the untransposed file; the threshold is 0.15). The other three fail on cents.

### Task 9: Picker ✓

- `getFactorySources` returns id, name, category. `#factory-list`: 620 × 308 px, 4 columns of 140 px, 7 headings, 20 px rows, `max-height` + scroll as the guard, clamped to the frame. The listbox takes focus while open and carries `aria-activedescendant`.
- Keys as planned (see D5). Seven keys `list.src*` in en / fr / zh-Hans.

**Verification:**
- `ui_frontend_check.js` ALL PASS, pins 36 / 22, section [17].
- `ui_factory_picker_check.js` ALL PASS: FK1 content and order; FK2 en / fr / zh-Hans × lanes 1–4 inside the frame, no scroll (308 / 308), all 41 items uncovered, names and headings fit (widest heading "Pincées / Frappées" 116.2 of 124 px); FK3 keyboard, type-ahead, `aria-activedescendant`, Enter → one `loadFactorySlot {slot: 3, id: "contrabassoon"}`, Escape → focus back on the link; FK4 dismissal and click.
- check-i18n pass; fr lint clean; zh lint clean (`list.srcVoice` 人声 carries a termNote: the sung voice, not a synth voice).
- check-ui-labels, default + 49 states, en / fr / zh-Hans: ALL CHECKS PASSED, 0 FAIL (second run; the first failed on state order, D6).
- Captures: `captures/picker-en.jpg`, `picker-fr.jpg`, `picker-zh-Hans.jpg` (JPEG copies of the PNGs the check writes with `--shots`) (the stub's lanes are forced empty for the shot, which is why loaded lanes show the link).
- **Not exercised here:** the lane name and the Root fields updating in the real plugin after a pick. The stub records the call; the processor side is FR1. This is part of the ear check.

### Task 10: Version, build, validators, records ✓

Version 1.2.0 in `CMakeLists.txt` (bundle reads 1.2.0); CHANGELOG `## [1.2.0]` with the older-version note; PLUGINS.md row; NOTES.md; built and installed by `./scripts/build-and-install.sh` (`-dev` variant, 26 MB). Standalone builds and opens. Results in the table at the top.

---

## Files

**New:** `Resources/factory/manifest.json`, `sources.cmake`, 33 `.flac`; `Source/dsp/GiFactoryTable.inc`; `tools/factory_table.py`, `factory_ingest.py`, `factory_measure.py`; `tests/render-harness/factory_specs.inc`; `tests/fixtures/factory-seam/*.flac` (7); `tests/ui_factory_picker_check.js`.

**Changed:** `CMakeLists.txt`, `CHANGELOG.md`, `NOTES.md`, `Source/dsp/GiFactory.h`, `Source/dsp/SlotLoader.cpp`, `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.cpp`, `Source/GiUiJson.h`, `Source/ui/public/index.html`, `Source/ui/public/js/i18n.js`, `tests/render-harness/{factory_gen.cpp, main.cpp, preset_gates_4.h}`, `tests/golden/factory-sources.sha256` (+33 lines), `tests/ui-stub/generic-overrides.json`, `tests/i18n-states.json`, `tests/ui_frontend_check.js`, `tests/ui_binding_probe.js`; repo `PLUGINS.md`.

**Backup:** `backups/O-GranulatorInstrument/v1.1.0/` (taken before the first edit).

## Open

- The ear check (item 1) and the two pairs to listen to (item 2).
- Windows arm: unchanged position, runs at `/publish`.

---

*Generated by improve-milestone execute phase*
