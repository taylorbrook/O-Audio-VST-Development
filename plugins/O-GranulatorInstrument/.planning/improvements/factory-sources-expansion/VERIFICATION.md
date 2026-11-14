# Verification: Factory sources expansion

**Plugin:** O-GranulatorInstrument
**Milestone:** factory-sources-expansion
**Created:** 2026-11-13
**Phase:** Verify

---

## Verification Summary

**Status:** PASSED (hands-on recorded 2026-11-13)
**Version:** 1.1.0 → 1.2.0
**Tree verified:** `5296d8a9` (plugin paths clean against HEAD before and after the run)

Every automated gate was re-run in this session on the committed tree and is green. The numbers below are from this run, not copied from SUMMARY.md. Three things were checked by a second method that does not use the project's own rows: the pitch of the off-C3 sources, the wrap of the seven textures, and the 64 preset renders. All three agree with the harness.

Two things are not verified by any machine and stay with Taylor: how the sources sound, and the lane name and Root fields updating in the real plugin after a pick.

| Check | Result (this run) |
|---|---|
| Full harness | **555 PASS / 0 FAIL**, rc 0 |
| 64 preset renders, 1.1.0 binary against 1.2.0 | 65 files, 0 differ by `cmp`; PT0 64 / 64 bitwise |
| The 8 old sources | FLAC blobs identical to `63ca6a70`; sha256 lines 1–8 identical; `shasum -c` 41 / 41 |
| `factory_table.py --check` | OK, 41 sources, 19 750 021 B |
| auval `aumu OuGI OuDv` | AU VALIDATION SUCCEEDED (the 4 known LFO-rate warnings) |
| pluginval level 10 | VST3 SUCCESS, AU SUCCESS |
| `ui_frontend_check.js` | ALL PASS; `--nc` all 4 copies fail the closure |
| `ui_factory_picker_check.js` | ALL PASS, 23 lines, 4 controls fire, no page errors |
| `ui_binding_probe.js` | ALL PASS, 293 / 293 both ways |
| `preset_name_fit.js` | ALL PASS, control fires |
| check-i18n, fr lint, zh lint | pass, clean, 0 findings |
| check-ui-labels | ALL CHECKS PASSED, default + 49 states, 0 FAIL |

---

## Requirements Verification

Criteria from CONTEXT.md as amended by PLAN.md (C1–C6) and by the Guitar Overtones re-cut.

### Criterion 1: 41 sources listed by the picker, in 7 categories, in three languages

**Status:** ✓ PASS

- Manifest read directly: 41 entries; tonal 8, voice 2, plucked 9, texture 9, drone 3, rhythmic 5, field 5, which is the plan's table. 14 samples, 27 generated (8 old + 19 new). Indices 0–7 are the old 8 in the old order.
- `ui_frontend_check.js` [17]: the stub fixture dumped from the table equals the manifest in id, name, category and order (41 / 41), with a control; 7 headings each with en / fr / zh-Hans.
- `ui_factory_picker_check.js`: en / fr / zh-Hans × lanes 1–4 inside the frame, no scroll, all 41 items uncovered, names and headings fit.
- The French capture was looked at: four columns, groups placed as planned, nothing clipped.
- The category names in fr and zh-Hans are machine-drafted (`reviewed: false` / `'mt'`), like the rest of the table.

### Criterion 2: The 8 old pins, their sha256 lines and the 64 presets unchanged

**Status:** ✓ PASS

- `git diff 63ca6a70 HEAD` over the 8 FLACs present at 1.1.0: empty.
- The first 8 lines of `tests/golden/factory-sources.sha256` equal the 1.1.0 file.
- FSP1: 41 / 41, the 1.0.0 set 8 / 8 on its literal pins and names. FSP1n fires (40 / 41, `dust`).
- Presets: the 1.1.0 harness binary kept from execute (`harness-110`, 19.7 MB) and the binary built here each dumped the 64 renders into fresh folders. 65 files, 235 MB, none differ. This rests on that binary being 1.1.0; it was not rebuilt from `63ca6a70` here. Supporting it: the diff since 1.1.0 touches no engine file (see Regression Check).

### Criterion 3: Every pick writes the stream's root; the off-C3 samples play at the key's pitch

**Status:** ✓ PASS (machine); ear check open

- FR1 41 / 41 from sentinel roots, other streams untouched; FR1n 0 / 41. FR1r, FR1e and FR2 pass with their controls.
- FPT1 at C3: crystal +3.8 ct, guitar_overtones +0.5, guitar_drift −5.7, contrabassoon −1.2; margin 4.3 ct of 10. FPT1n fails on all four.
- Second method, on the embedded FLACs alone (FFT peak with parabolic interpolation; autocorrelation for the contrabassoon), against the manifest root:

| source | manifest root | target | measured | error |
|---|---|---|---|---|
| crystal | 73 / +17 | 559.84 Hz | 559.86 Hz | +0.1 ct |
| guitar_overtones | 72 / −32 | 513.67 Hz | 513.75 Hz | +0.3 ct |
| guitar_drift (2nd harmonic) | 38 / −10 | 145.99 Hz | 145.46 Hz | −6.2 ct |
| contrabassoon | 20 / −19 | 25.673 Hz | 25.655 Hz | −1.2 ct |
| sub | 24 / 0 | 32.70 Hz | 32.70 Hz | 0.0 ct |

- Code read: the write is in `loadFactorySource` after a queued load, on the message thread, one gesture per parameter, none when the value is already there. Nothing in a listener, nothing on the audio thread.

### Criterion 4: Embedded total ≤ 24 MiB, each source ≤ 3 MiB

**Status:** ✓ PASS

- `ls` over `Resources/factory/*.flac`: 41 files, 19 750 021 B (78 % of 25 165 824), largest `stream.flac` 1 364 847 B. FSP1 reads the same two numbers from the binary.
- All 41 decode at 48 kHz with the manifest's frame and channel counts. The 14 samples are 16-bit; the 27 generated are 24-bit.

### Criterion 5: The seven long textures wrap without a dip or click

**Status:** ✓ PASS

- FSM1 7 / 7: fold centre −1.03 … +0.94 dB (limit ±1.5), wrap step ratio ≤ 0.44 (limit 1). FSM1n: linear folds −1.97 … −4.03 dB, 7 / 7 fail.
- Second method: each file read circularly in 100 ms frames across the wrap and the fold. Wrap step against the 99.9th-percentile sample step: 0.01 … 0.44, the harness's figures. On the four steady textures the fold centre sits within +0.5 / −1.6 dB of its neighbours (stream +0.5, waves −0.3, garden +0.5, woods −1.6). Fire, bubbles and park change level by themselves over that second, so this measure cannot rule on them; the frames show a ramp or separate events, no notch. FSM1's fixture is the row that rules there.

### Criterion 6: Build without warnings; auval passes; pluginval passes at level 10

**Status:** ✓ PASS

- `ninja` on VST3, AU, Standalone, harness and param-dump at HEAD: up to date apart from two stale console links; the only warning is JUCE's libc++ platform notice.
- Plugin sources were not recompiled in this run. The execute logs show no warning from `Source/`; the 14 they do show are in `tests/render-harness/` files that predate this milestone.
- Installed VST3 and AU binaries are byte-identical to the build artefacts; bundle version 1.2.0; no unsuffixed variant on disk.

### Pipeline (CONTEXT requirement 7)

**Status:** ✓ PASS

- One manifest generates the table, the CMake list, the golden sha256 and the harness specs; `--check` is clean.
- Sample source paths in the manifest are relative, all under the folders CONTEXT.md allows plus the one contrabassoon file.
- FLV1 41 / 41: peak max −0.9999 dBFS, loud frame −32.86 (stick_snaps) … −18.92 (vowel). FLV1n fires.

---

## Regression Check

**Baseline:** 1.1.0 (`63ca6a70`). **Backup:** `backups/O-GranulatorInstrument/v1.1.0/` present.

- `git diff --stat 63ca6a70 HEAD -- Source`: `GiUiJson.h`, `PluginEditor.cpp`, `PluginProcessor.{h,cpp}`, `dsp/GiFactory.h`, `dsp/GiFactoryTable.inc` (new), `dsp/SlotLoader.cpp` (one line), `index.html`, `i18n.js`. No engine file, no parameter layout, no state code.
- The harness's 544 rows from 1.1.0 all pass; the 11 new rows account for 555.
- Binding probe 293 / 293 both ways: no parameter added, removed or unbound.
- Old presets load: yes (64 bitwise). Old sessions: state format unchanged.

---

## Issues Found

### Critical

None.

### Noted

1. **PLAN Task 9, last box, is ticked but was not exercised.** "Picking a source updates the lane name and the stream's Root fields without reopening anything" has no automated check in the real plugin; SUMMARY.md says so itself. By code read the path is sound: the Root fields are ordinary bound parameters (the probe covers them), and the lane name arrives through `pullSlotState` after the call and again when telemetry's `slotRev` moves. Left for the hands-on check.
2. **`breath` reads −3.8 ct against a ±3 ct criterion (PLAN Task 5), box ticked.** Reported in SUMMARY item 4. Noise through a resonance set exactly at C3; the estimate carries a few cents of spread. Recommendation: accept.
3. **FPT1n's crystal case fails on the strength clause only** (3.5 ct, strength 0.07 against 0.15). The untransposed file happens to hold a weak component near 130.81 Hz. The control still fires; the independent measurement above covers crystal's root without that coincidence.
4. **Stick Snaps is 0.14 dB above FLV1's floor.** Passes; thin margin, from the "ship as cut" call.
5. **Windows arm not run.** Unchanged position: runs at `/publish`.
6. **The 1.1.0 reference binary was reused, not rebuilt** (criterion 2).

---

## For Taylor (hands-on, about five minutes)

1. Empty lane → Factory → Contrabassoon, play C3: it sounds a C3. The lane shows "Contrabassoon"; Root reads G#0 / −19 without reopening anything.
2. Same with Crystal, Guitar Overtones, Guitar Drift, Sub.
3. Listen to bell against glass, and pluck against kalimba (they read alike by the numbers).
4. One long texture with a full-length loop: no dip at the wrap.

---

## Hands-on result

Taylor, 2026-11-13: all four items pass (reported as a whole, "all pass"). This closes note 1 (lane name and Root readout in the real plugin) and the ear check; note 2 (`breath` −3.8 ct) is accepted with it. Open: the Windows arm at `/publish`.

---

## Verification Checklist

- [x] All requirements from CONTEXT.md verified
- [x] Build succeeds without errors
- [x] Pluginval passes level 10 (VST3 + AU)
- [x] UI loads and responds (stub page, browser gates)
- [x] No regressions in existing features
- [x] Preset compatibility confirmed
- [x] Ear check and live-plugin readout (Taylor, 2026-11-13: all pass)

---

## Final Status

### PASSED ✓

All six criteria hold on the committed tree. Version, CHANGELOG, PLUGINS.md and the install were done at execute and are confirmed here. No tag (tags come from `/publish`).

---

*Generated by improve-milestone verify phase*
