# Summary: Preset bank expansion

**Plugin:** O-GranulatorInstrument
**Milestone:** preset-bank-expansion
**Created:** 2026-11-14
**Phase:** Execute

---

## Implementation Summary

**Version:** 1.2.0 → 1.3.0 (MINOR)
**Domain:** mixed (preset authoring, harness, WebView UI)
**Agent:** general-purpose (one wave per task, T0–T12, one path-scoped commit each)

96 new factory presets (Drones, Expressive, Keys, Motion, Pads, Rhythmic, Textures +12 → 20 each; Live stays 8; new Field category of 12): bank 160. Every one of the 41 factory sources is played by an enabled stream in ≥ 2 presets. 8 approved re-voices of old presets and 5 description fixes; the other 56 old presets render bitwise as 1.2.0. Two-pane grouped preset menu, plugin-local (`preset-manager` module untouched). No engine, parameter, range or state-format change. Built and installed as 1.3.0; not tagged.

**Result in one table** (T12, final tree)

| Check | Result |
|---|---|
| Full harness | **564 PASS / 0 FAIL**, rc 0, 474 s (1.2.0: 555; + FSU1 / FSU1n / FRT1 / FRT1n / FP0wn / FP1c / FP3c / PM0 / PM0n) |
| FP0 | 982 / 982 checks: 160 = 7 × 20 + Live 8 + Field 12, names / seeds unique, every category rule, Field rule (P7), Expressive split 8 / 2 / 6 / 6, realism-titled 24, none trips the wobble rule; FP0wn fires |
| FSU1 / FRT1 / PM0 | FSU1 41 / 41 (FSU1n: harmonic dropped → 40 / 41, fires); FRT1 22 / 22 off-C3 slots, 5 / 5 sources (FRT1n fires); PM0 20 × 7 / Field 12 / Live 8 / User 2 (PM0n: user merged into Pads, fires) |
| FP1 / FP4 / FP4b / FP6 | FP1 160 / 160; FP4 800 / 800 bitwise, 0 equal fresh pairs; FP4b 160 / 160; FP6 worst 0.911× (FP6n fires) |
| FP2 (gated loudness) | spread **6.00 dB** (−29.0 Grain Piano … −23.0 Fifth Bloom), max peak **0.673** (≤ 4.0); new presets −28.9 … −26.4; P4 (Fifth Bloom trim) not fired |
| FP3 | 20 Rhythmic × 2 tempi, 1059 onsets, 0 off-grid |
| `--cmp-preset-renders ref-1.2.0 --all-bitwise --exclude-revoiced13` | PT0v13 64 old names, **0 missing, 96 new**; PT0 **56 / 56 bitwise** |
| `--preset-level-table ref-1.2.0` | old 64 rows byte-identical to T1's table: **0 level outliers** (largest +1.85 dB, Glass Mallet, < +3.0), 6 centroid flags all on re-voices (new source), 56 bitwise. Exit 77 at the end because the 96 new presets have no 1.2.0 reference (the table has no "new" path; the rows are complete) |
| FP5 / bank stamp | FP5 (temp HOME): 160 files + sentinels + stamp, read-back 0 bad, user copy saved / reloaded. Real `~/Library/O-GranulatorInstrument/Presets/Factory` after the 1.3.0 binary first loaded: `.bank-hash` 1a994b77e891b91b → **ced2c6c50a602cb9**, `.factory-version` 1.2.0 → 1.3.0, 64 → 160 `.json` |
| Old sessions / presets | state format unchanged; SS1–SS7, ST1–ST6, FR2 and FP5's user-copy arm green |
| auval | `auval -a`: `aumu OuGI OuDv` listed; `auval -v aumu OuGI OuDv` **AU VALIDATION SUCCEEDED** (the 4 known LFO-rate default warnings) |
| pluginval level 10 | VST3 **SUCCESS**, AU **SUCCESS** (installed bundles, in-process) |
| UI gates | re-run at T12: `ui_frontend_check.js` ALL PASS, `ui_preset_menu_check.js` ALL PASS. From T11 (UI unchanged since): `ui_factory_picker_check.js`, `ui_binding_probe.js` 293 / 293 (ST1 50 states), `preset_name_fit.js` (PN1n / PN1n2 fire), check-i18n, fr lint `--strict`, zh lint, check-ui-labels (default + 50 states) |
| Install | `./scripts/build-and-install.sh O-GranulatorInstrument` rc 0; installed VST3 and AU `diff -rq`-identical to the artefacts; bundle version 1.3.0 (AU version 66304); only the `-dev` variant on disk; Standalone 1.3.0 boots (`evidence/t12/standalone-boot-1.3.0.jpg`) |

Evidence: `evidence/t12/` (harness-full, cmp, level-table, auval-pluginval, Standalone boot).

---

## For Taylor — to hear or decide (hands-on, all waves)

Every level and rule below is measured; none of it has been listened to.

**Old presets (T1 re-voices)**
1. **Keys · Glass Mallet** — s1 bell → glass. Glass + sustain 20 read −36.8 dB (8.6 under 1.2.0) with the level already at +6, so it was lifted by voicing: ping-pong scan over 0–25 %, decay 5000, window 0.8, s2 pluck −6, vel_to_amp 40, reverb 32. −29.0 gated (−0.8 vs 1.2.0), no wrap re-strike. The most changed of the eight: check it still reads as a mallet on glass.
2. The other seven re-voices (Kalimba, Organ Loft, Warm Choir, Velvet Strings, Ember Field, Hollow Wind, Rain Glass) sit within −0.7 … +1.4 dB of 1.2.0 (Ember Field +1.4, deliberately off the floor).

**Keys (T3)**
3. **Marimba, FM Tines, Ice Bells, Gamelan Plate — sustain.** Short percussive sources read −36 … −38 on the gated FP2 at sustain 0 (tails count), so they carry sustain 25–50 / density 200 / vel_to_amp 40–50. Check a held note does not turn into a pad.

**Pads (T4)**
4. **Breath Cloud** — re-voiced twice (band-pass read −44 / −36): density 150, resonance 0, shimmer +3.5, s1 at +6. The least certain Pad.
5. **Deep Floor / Analog Sweep** — sub −12 (root 24 / 0) under the chord / saw. Check the low end on real monitors.

**Drones (T5)**
6. **Bassoon Pedal** — key track 0 + s1_pitch 4.19 = a fixed C1 pedal; the second contrabassoon tracks the key an octave under. Check the pedal pitch.
7. **Wind Tunnel** — its loop moves ~1.6 dB per dB of level; three passes to −27.2. Check it does not swell on a long hold.
8. **Saw Monolith** (drive 25 + fb saturation 45) and **Plate Gong** (metal_plate −12 over its ring, fb 65) — the two harshest drones.

**Textures (T6)**
9. **Paper Cuts / Breath Static / Twig Storm / Cicada Night** — sparse one-shots and breath read 10–17 dB under target at +6; lifted by long grains (120–350 ms), drive and feedback (Paper Cuts fb 72 + drive 75, five passes). Check they still sound like their names.

**Rhythmic (T7)**
10. **The five drum-hit presets run voice_mode Mono** (Click Hats, Stick Groove, Snip Claps, Iron Shaker, Bubble Seq): poly chords stacked identical in-phase hits (peaks 1.1–1.9), so a chord now plays one hit; level from drive 60–85, vel_to_amp 0. **Snip Claps** moved to eighths (quarters read −31 at +6). **Stick Groove** carries the highest Rhythmic peak (0.609).

**Motion (T8)**
11. **Bubble Orbit** (−28.9, quietest new preset), **Reed Pulsar** (square LFO → level: depth 12 + −9 dB after depth 20 read −17.8), **Plate Spin** (a 0–25 % loop lifted it −43.7 → −27).

**Expressive (T9)**
12. **Mono-glide presets** (Saw Glide, Sub Legato, Deep Slur, Drift Slide) — measured on one voice of the script's dyads; −34 at 0 dB lifted by density 120–140, longer grains, vel_to_amp 40. Check glide feel and level next to the poly presets.
13. **Wheel Organ** (realism title; wheel → drive, no wobble), **Velo Glass** (ping-pong 0–30 % after −40.7 on a forward scan), **Bow Press** (bend range 12, bend → cutoff) — try on a real MPE / wheel controller.

**Field (T10)**
14. **Distant Park** — the wind companion dominates (s2 −16.6); −26.5, the loudest Field preset. **Park Memory / Garden Dusk** key track 50 / 25: check the recording does not sound transposed across the keyboard. **Sea Cave vs Undertow** (Drones, also waves): distinct enough?

**Menu (T11)**
15. **The menu in the real Standalone / DAW in en / fr / zh** — not captured in the real WebView (see open notes). Check: Nappes / 铺垫 for Pads (machine-drafted), the 310 px preset pane width, and that the **prev / next** buttons walk the flat list (User presets mixed in by name), not the menu's grouping.

## Open notes

- The `s*_` shortcut parameters some presets carry write to streams that are disabled in that preset. This is the 1.0.0 convention (the old presets do the same); not changed in this milestone.
- French (and Chinese) group names are machine-drafted (`reviewed: false` / `'mt'`), like the rest of the tables.
- **Real-Standalone open-menu capture: skipped.** The 1.3.0 Standalone was launched in the background (`open -g`) and its boot window captured; opening the menu needs a click delivered to the WKWebView, which only reaches a frontmost window (T11: posted clicks to a background window do not arrive). That means taking focus from Taylor's session, so it was not done. The menu is gated in the served page (PM1–PM4, WebKit engine checked at T11). Left for the hands-on.
- `--preset-level-table` ends with exit 77 at 160 presets (no 1.2.0 reference for the 96 new). The 64 rows it prints are complete; a "new preset" path in the table would make the exit 0. Not changed here (out of T12 scope).

---

## Tasks Completed

| Task | Commit | Outcome |
|---|---|---|
| T0 backup + 1.2.0 references | 05e46f9c | `backups/O-GranulatorInstrument/v1.2.0/`; 1.2.0 harness kept (`build/.../pbe-ref120-bin/harness-120`); 64 renders in `evidence/ref-1.2.0` (f32 gitignored); 1.2.0 FP2 spread 7.00 dB |
| T1 old-bank re-voices | 05e46f9c | V1–V8 + D1 removed + D6 / D7 / D8 / D10 / D11; spread 7.00 → 6.00 dB; PT0 56 / 56 bitwise |
| T2 harness re-scale | de50f696 | `F::kBankSize` 160 + `kCategoryCounts`; FP0 Field rule (P7), Expressive split (P6), realism wobble (P3 / P5); FSU1 / FSU1n, FRT1 / FRT1n, FP0wn; the compare fails a missing old name |
| T3 Keys +12 | 3abded8c | FP2 −28.3 … −26.4 |
| T4 Pads +12 | 1874b476 | −27.1 … −27.0 |
| T5 Drones +12 | 75802b14 | −27.2 … −27.0; FRT1 green (5 / 5 sources) |
| T6 Textures +12 | 3cd7695f | −27.4 … −27.0 |
| T7 Rhythmic +12 | fe362e86 | −28.4 … −26.9; FP3 0 off-grid |
| T8 Motion +12 | 68c2ba4a | −28.9 … −27.0 |
| T9 Expressive +12 | c574eeb1 | −28.0 … −26.9; split 8 / 2 / 6 / 6 |
| T10 Field 12 | efa1cc54 | −27.1 … −26.5; expected-fail list empty; `--pg4` 40 / 0 |
| T11 two-pane menu | f313c947 | `GiUiJson::presetMenuToVar` + native `getPresetMenu` (natives 37); PM0 / PM0n; `ui_preset_menu_check.js` PM1–PM4 |
| T12 gate, build, install, docs | this commit | the table above |

Per-wave figures are in each commit message and in `STATUS.yaml` `execute.notes`.

### Task 12: Full gate, build, install, docs ✓

**Changes made:**
- `CMakeLists.txt` `VERSION 1.2.0` → `1.3.0` (the only product version string; the other `1.2.0` mentions in `Source/` are history comments or module versions).
- Full harness, compare, level table, build-and-install, auval, pluginval 10, Standalone boot; UI gates re-run.
- `CHANGELOG.md` `## [1.3.0] - 2026-11-14`; this SUMMARY; `PLUGINS.md` row 1.3.0 (no duplicate rows); plugin `STATUS.md`; milestone `STATUS.yaml` execute complete; PLAN T12 + milestone criteria ticked.

**Verification result:** Pass with notes (menu capture in the real WebView left for the hands-on; level-table exit 77 explained above).

---

## Files Changed Summary (since 1.2.0, `832edfac`, outside `.planning/`)

| File | Change |
|---|---|
| `Source/FactoryPresets.h` | 96 presets, 8 re-voices, `kBankSize`, `kCategoryCounts` (+1394) |
| `Source/GiUiJson.h` | `presetMenuToVar` encoder |
| `Source/PluginEditor.cpp` | native `getPresetMenu` |
| `Source/ui/public/index.html` | two-pane menu (markup, CSS, keys) |
| `Source/ui/public/js/i18n.js` | `list.pg*`, `list.pgEmpty`, `aria.presetGroups` (en / fr / zh) |
| `tests/render-harness/preset_gates_4.h`, `main.cpp` | bank-size re-scale, FSU1 / FRT1 / PM0 rows + controls, compare by name list |
| `tests/ui_preset_menu_check.js` | new (PM1–PM4) |
| `tests/ui_frontend_check.js`, `ui_binding_probe.js`, `preset_name_fit.js`, `i18n-states.json`, `ui-stub/generic-overrides.json` | menu checks, ST1 pin 49 → 50, PN1 right-pane fit, 160-name fixture |
| `CMakeLists.txt` | VERSION 1.3.0 |
| `CHANGELOG.md` | `## [1.3.0] - 2026-11-14` |

**Total:** 15 files outside `.planning/`, +3319 / −131.

---

## Implementation Notes

### Technical decisions made

1. **Level by voicing, not level** (P10). When `sN_level` would pass +6: density, grain length, `vel_to_amp`, feedback. Short percussive and sparse sources read 10–17 dB low on the gated FP2 because the tails count.
2. **Density saturates near 150 / s** at grain_limit 128 (Breath Cloud: 200 reads as 150).
3. **Feedback and level-LFO presets calibrate non-linearly** (Wind Tunnel ~1.6 dB per dB of level; Reed Pulsar); 3–5 passes.
4. **Hit presets in Mono** (T7): poly chords stacked identical in-phase hits.
5. **Field sources read loud**: every Field preset trimmed down (s1 −10.6 … −0.2), no density lift.
6. **Menu plugin-local** (P12): one native, one encoder shared by the editor and the harness fixture, so the stub cannot drift.

### Deviations from plan

- None. No threshold loosened; P4 (Fifth Bloom trim) not needed (spread 6.00 ≤ 8.5 dB).

### Issues encountered

- T11: check-ui-labels' first run failed `#pm-presets` dh −2 in zh; fixed by pinning `.pm-empty` to 22 px.
- Real-Standalone menu capture not possible without taking focus (T11 and T12).

---

## Build Status

**Pre-implementation backup:** `backups/O-GranulatorInstrument/v1.2.0/`
**Build command:** `./scripts/build-and-install.sh O-GranulatorInstrument` (harness: `ninja O-GranulatorInstrument-render-harness`)
**Build result:** Success (only JUCE's libc++ platform notice and the existing harness warnings)

---

## Quick Test Results

**Standalone launch:** Pass (1.3.0 boots, default page)
**Parameter response / audio output:** harness (FP1 / FP2 / FP4)
**No crashes:** Pass (pluginval 10 VST3 + AU, auval)

---

## Ready for Verification

The execute phase is complete. Next: `/improve-milestone O-GranulatorInstrument` (verify).

---

*Generated by improve-milestone execute phase*
