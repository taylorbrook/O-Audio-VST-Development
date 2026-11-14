# Verification: Preset bank expansion

**Plugin:** O-GranulatorInstrument
**Milestone:** preset-bank-expansion
**Created:** 2026-11-14
**Phase:** Verify

---

## Verification Summary

**Status:** PASSED — automated gates. The listening hands-on (SUMMARY.md "For Taylor") is still open.
**Version:** 1.2.0 → 1.3.0
**Tree verified:** `1d616bf7` (plugin paths clean against HEAD before and after the run; only an untracked `stages/3-gui/gate-report.json`, not part of this milestone)

Every automated gate was re-run in this session on the committed tree, and all are green. The numbers below come from this run, not from SUMMARY.md. Three claims were also checked by a second method that does not use the harness's own rows:

1. **The preset table.** A Python parser (`evidence/verify/indep.py`) checks counts, names, seeds, `output_gain`, coverage of the sources on enabled streams, roots for off-C3 sources, and realism wobble. It has three negative controls, and each one fires.
2. **The 56 bitwise presets.** All 160 presets were dumped fresh and compared with `cmp` against the 1.2.0 reference `.f32` files.
3. **FP2 loudness.** The gate was re-implemented in numpy on the fresh renders.

All three agree with the harness.

The listening is not verified by any machine, and it stays with Taylor: how the 96 new presets and the 8 re-voices sound, and the menu in the real Standalone / DAW.

| Check | Result (this run) |
|---|---|
| Full harness | **564 PASS / 0 FAIL**, rc 0, 541 s wall (`evidence/verify/harness-full.txt`) |
| `--cmp-preset-renders ref-1.2.0 --all-bitwise --exclude-revoiced13` | PT0v13 64 old names / **0 missing** / 96 new; PT0 **56 / 56 bitwise** |
| `cmp` of a fresh `--dump-preset-renders` (160 files) against the 64 1.2.0 `.f32` | **56 identical, 8 differ**. The 8 are exactly the approved re-voices: Organ Loft, Hollow Wind, Glass Mallet, Kalimba, Warm Choir, Velvet Strings, Ember Field, Rain Glass |
| Independent table check (`indep.py`) | 160 = 7 × 20 + Live 8 + Field 12. Names ≤ 24 characters (the longest is "Expressive · Slide Glass", 24), unique, no `/ \ :`. Seeds unique and on the P9 pattern. No `output_gain`. **41 / 41 sources on an enabled stream in ≥ 2 presets** (minimum 2). Every off-C3 slot carries the manifest root. 24 realism-titled presets, 0 wobble. Controls fire: root −19 → −18 (Bassoon Pedal), an LFO → `s1_pitch` route on Keys · Kalimba, park → stream (coverage park 1) |
| Old-64 text diff against `832edfac` | 0 names missing. Data changed only on the 8 re-voices. Description-only changes: Orbit Lines (D7), Crush Tides (D8), Gated Pad (D11), Rust Cloud (D10). D6 is folded into the Glass Mallet re-voice. Nothing else touched (P1 / P2) |
| FP2 (harness) | gated −29.0 (Grain Piano) … −23.0 (Fifth Bloom), **spread 6.00 dB**, **max peak 0.673**. Every one of the 160 is inside [−29, −23] on the INFO line |
| FP2 re-implemented (numpy, same 50 ms / −70 dBFS gate, 2–10 s of the 10 s dump) | median +0.39 dB against the harness's 2–20 s value, worst 2.67 dB (Sub Legato, Crystal Bowl, Gravel Rain, Tide Machine: long envelopes, where the shorter window weighs differently). Spread 7.94 dB on the short window (≤ 9). Peak 0.673, the same |
| FP0 / FSU1 / FRT1 / PM0 | FP0 982 / 982 (FP0wn fires); FSU1 41 / 41 (FSU1n: 40 / 41, fires); FRT1 22 / 22 slots, 5 / 5 sources (FRT1n fires); PM0 20 × 7 / Field 12 / Live 8 / User 2 (PM0n fires) |
| FP1 / FP3 / FP4 / FP5 | FP1 160 / 160; FP3 1059 onsets, 0 off-grid (worst 0.968 samples); FP4 800 / 800 bitwise; FP5 160 files + sentinels + stamp, user copy reloaded |
| Real Factory folder | `.bank-hash` `ced2c6c50a602cb9`, `.factory-version` 1.3.0, 160 `.json` |
| Install | Installed VST3 and AU `diff -rq`-identical to the artefacts (ninja: no work to do at HEAD). Bundle 1.3.0 (AU 66304). Only `-dev` on disk, no orphaned unsuffixed variant |
| auval `aumu OuGI OuDv` | **AU VALIDATION SUCCEEDED** (the 4 known LFO-rate default warnings) |
| pluginval level 10 (in-process) | VST3 **SUCCESS**, AU **SUCCESS** |
| `ui_frontend_check.js` | ALL PASS (174 PASS lines) |
| `ui_preset_menu_check.js` | ALL PASS (PM1–PM4 + controls, 28 lines) |
| `ui_factory_picker_check.js` | ALL PASS (23) |
| `preset_name_fit.js` | ALL PASS (PN1 en / fr / zh in the right pane; PN1n / PN1n2 fire) |
| `ui_binding_probe.js` | ALL PASS, 293 / 293 both ways, ST1 50 states |
| check-i18n · fr lint `--strict` · zh lint | ALL CHECKS PASS · CLEAN · 0 findings (467 entries at `'mt'`, counted, not failed) |
| check-ui-labels | **ALL CHECKS PASSED**, default + 50 states (`preset-menu-open` last), 0 FAIL |

---

## Requirements Verification

### R1: 96 new presets, every factory source used — ✓ PASS

- 160 presets, with per-category counts exactly as D1 sets them. Checked by FP0 and independently by `indep.py`.
- 41 / 41 sources are read by an enabled stream in ≥ 2 presets (FSU1, and independently with a park-removal control).
- Names, seeds and the never-`output_gain` rule are checked independently. The category rules, the P7 Field rule, the P6 Expressive split (8 / 2 / 6 / 6) and the P3 / P5 realism wobble (24 titles) pass in FP0. The wobble part is independently re-checked with a control.

### R2: In tune — ✓ PASS

- FRT1: 22 / 22 off-C3 slots after apply. All 5 off-C3 sources appear in the bank. The FRT1n control fires.
- Independently: every slot with crystal / guitar_overtones / guitar_drift / contrabassoon / sub writes the manifest pair. A changed `root_fine` fails it.

### R3: Level — ✓ PASS

- FP2 spread 6.00 dB (≤ 9). Peak 0.673 (≤ 4.0). Every preset is inside [−29, −23], so every new preset is within −27 ± 2.
- The numpy re-implementation of the gate agrees to a median of +0.39 dB.
- The level table (re-run at T12, unchanged since) shows 0 old-preset outliers. Its exit 77 comes from the 96 new presets, which have no reference (SUMMARY open note).

### R4: Review of the old 64 — ✓ PASS

- `cmp` against the 1.2.0 references: 56 identical. The 8 that differ are exactly P1's list.
- The text diff shows no other data change. The description fixes are exactly D6 (in V8), D7, D8, D10, D11. D1 is removed with V4 (FP0wn exercises the 1.2.0 D1 detune as a failing control).

### R5: Two-pane grouped preset menu — ✓ PASS (served page). The real-WebView look is a hands-on item

- PM0: encoder groups, counts, and User-only placement of a user "Pads · Mine". PM1–PM4: markup, keys, ARIA, sizes, with 5 controls. PN1: short names fit in en / fr / zh. check-ui-labels covers the `preset-menu-open` state.
- The T11 captures were looked at again (en / fr / zh). There are 10 groups, nothing clipped, and the file row sits at the foot of the left pane.
- Visual note: about 200 px of empty left pane sits between the last group and "Load from file…". This is by design (fixed height, no scroll), but it is the one place in the menu that reads as unfinished. Worth a look in the hands-on.

### R6: Gates — ✓ PASS

- `kBankSize` 160 runs through FP0 / FP1 / FP2 / FP4 (800 pairs) / FP5 / dump / PN1 / compare.
- New rows FSU1 / FRT1 / PM0 / FP0wn / FP1c / FP3c each have a firing control.
- auval, pluginval 10 VST3 + AU, installed.

### Constraints — ✓

- No engine, parameter or state-format change: SS1–SS7, ST1–ST6, FR2 and G1 (293 params == golden) are green.
- No tag. Commits were path-scoped.

---

## Regression Check

- Old presets: 56 / 56 byte-identical renders against the 1.2.0 binary's dumps.
- Old sessions and user presets: the session and state rows are green, and FP5's user-copy arm is green.
- Factory source picker unchanged: `ui_factory_picker_check.js` ALL PASS.

---

## Open — hands-on (Taylor)

The 15-item listening list in SUMMARY.md, "For Taylor". In priority order:

1. Keys · Glass Mallet (the most changed re-voice).
2. The five Mono drum presets, and the four mono-glide Expressive presets.
3. Short-source Keys with sustain 25–50.
4. Field key track (Park Memory / Garden Dusk).
5. **The menu in the real Standalone / DAW in en / fr / zh.** Check the Nappes / 铺垫 labels, the 310 px pane, and that prev / next walk the flat list.

## Notes

- `--preset-level-table` exits 77 at 160 presets. This is a known gap: the tool has no "new preset" path.
- Group names in fr / zh are machine-drafted.

---

*Generated by improve-milestone verify phase*
