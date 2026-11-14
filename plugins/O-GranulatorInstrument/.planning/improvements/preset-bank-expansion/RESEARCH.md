# Research: Preset bank expansion

**Plugin:** O-GranulatorInstrument · **Milestone:** preset-bank-expansion · **Date:** 2026-11-14 · **Version:** 1.2.0 → 1.3.0

Detail notes (this file wins where they differ):
- `research/old-bank-review.md`: the review of the 64, defects and re-voice proposals.
- `research/preset-menu.md`: the two-pane menu, i18n, the tests, layout measures.

---

## 1. Summary

The work is **data, one UI feature and a gate re-scale**. No engine, parameter, range or state-format change.

| Area | Files | Size |
|------|-------|------|
| 96 new presets plus edits to the old ones | `Source/FactoryPresets.h` | large (about +1500 lines) |
| Two-pane menu | `Source/ui/public/index.html`, `Source/GiUiJson.h`, `PluginEditor.cpp` (one native), `js/i18n.js` | medium |
| Gates | `tests/render-harness/preset_gates_4.h`, `main.cpp` (`--dump-factory-presets`), `tests/preset_name_fit.js`, `tests/ui_frontend_check.js`, `tests/ui_binding_probe.js`, `tests/i18n-states.json`, new `tests/ui_preset_menu_check.js` | medium |

The domain is mixed: preset authoring, UI and harness. Execute runs as general-purpose, the same as 1.2.0.

---

## 2. Harness: what has to change

### 2.1 Hard-coded 64 and exact counts (`preset_gates_4.h`)

| Line | Today | 1.3.0 |
|------|-------|-------|
| 1103–1107 | 8 per category, 8 categories, 64 total | per-category table: 20 × 7, Live 8, Field 12; 9 categories; 160 total |
| 1229 | `mpe == 4 && monoGlide == 2 && playCtl == 2` (exact) | `>=` with the 1.0.0 floors, or exact new counts fixed at plan |
| 1225 | `padsCustom == 2` | `>= 2` |
| 1237 | `checks = 64*4 + 9 + 64 + 7` | computed from N |
| 1304 / 1306 | FP1 `N == 64` | `N == kBankSize` |
| 1405 | FP3 `presets == 8` | 20 |
| 1509 / 1512 | FP4 `nPairs == 320` | `5 * N` = 800 |
| 1583 / 1691 | FP4b / FP5 text and count 64 | N |
| `main.cpp:1858` | `--dump-factory-presets` returns `n == 64` | 160 |
| `preset_name_fit.js:51 / :67` | category regex has no Field; count 64 | add Field; 160 |

One constant, `F::kBankSize = 160`, sits next to the table, plus a per-category expected-count map. FP0 checks the table against both.

### 2.2 Runtime (measured 2026-11-14, 16 cores)

- `OGI_ROWS=FP2 --pg4`: 4.95 s for 64 presets, so about 12 s at 160.
- `OGI_ROWS=FP4 --pg4`: 12.2 s for 320 pairs, so about 30 s for 800.
- FP4 stays whole: 5 offsets × N. The offsets {1, 8, 13, 32, 63} are still distinct mod 160.
- The full harness gains well under a minute. No split and no sampling needed.

### 2.3 Render compare against 1.2.0

- `--cmp-preset-renders` matches presets by file name (`<Cat>__<Short>.f32`, `ptFileOf`, line 1999). The 96 new presets have no reference file and land in "missing".
- 1.3.0 needs a mode in which "missing" is expected for names that are not in the 1.2.0 bank, and a fail for any old name.
- Approved re-voices leave the compare by name. This is a `kRevoiced13[]` list modelled on `kShowcase` (line 1969), with the same isShowcase-style exclusion.
- Pass condition: the old 64 minus the re-voiced presets are bitwise against `harness-rb` 1.2.0 dumps.

### 2.4 New rows

- **FSU1, source coverage:** every factory source id in the manifest appears in at least 2 presets' slots across the bank.
  - FSU1n control: the bank with one source's uses removed must FAIL.
- **FRT1, preset roots:** for every preset slot with an off-C3 source, the stream's `root_note` / `root_fine` after apply equals the manifest root. The off-C3 sources are crystal 73 / +17, guitar_overtones 72 / −32, guitar_drift 38 / −10, contrabassoon 20 / −19 and sub 24 / 0.
  - FRT1n control: one root dropped from a copy must FAIL.
  - FPT1, which already exists for the picker path, keeps its job.
- **FP0 Field rule:** see §4.
- **FP0 wobble rule widened:** the realism wobble check moves from Keys only to every preset whose name matches a realism title. The list is fixed at plan: Piano, Rhodes, Celesta, Harp, Strings, Choir, Organ, Kalimba, Marimba, Guitar, Bassoon. The check is no pitch_rand and no LFO → pitch / fine route on any enabled stream, and no sN_fine detune between streams that read the same source. This is the class fix for the Velvet Strings defect (memory: a gate written to the one instance in a review finding leaves the class open).

---

## 3. Level budget

- **FP2 today:** gated −30.0 dB (Textures · Ember Field) to −23.0 dB (Pads · Fifth Bloom). The spread is 7.00 dB against a 9 dB limit.
- **New-preset target:** gated −27 dB ± 2. That keeps the bank inside [−30, −23] and leaves the limit untouched.
- **Ember Field is the floor.** Its `s1_level` is already at the +6 ceiling. The re-voice to fire (§5) is the way to lift it. If it is not approved, the floor stays at −30.
- **Calibration loop:**
  1. Author the preset at a nominal level.
  2. Run `OGI_ROWS=FP2 --pg4`. It prints `INFO FP2 gated / plain RMS / active s` per preset.
  3. A scratch script reads that line and computes trim = −27 − gated for each new preset.
  4. Apply the trim to every enabled `sN_level` (clamp −60…+6). If a trim would pass +6, raise density or `vel_to_amp` instead of the level.
  5. Re-run once.
  The script lives in the scratchpad, not the repo. The trimmed values are written into the table rounded to 0.1 dB, the same as the existing ones.
- **Level outliers:** `--preset-level-table` against the 1.2.0 dumps applies only to old presets. New presets are judged by FP2 alone.

---

## 4. Field category rule (proposed for FP0)

A Field preset:
- lists at least one field-group source (stream, park, waves, woods, garden) on an enabled stream;
- sets `key_track` ≤ 50 on that stream, so the recordings are not transposed into chipmunk or sludge;
- sets `size` ≥ 150 ms and `amp_release` ≥ 2000;
- uses Free-run or Ping-pong scan (`scan_mode` ≠ Retrigger), so a held note walks the recording;
- sets `amp_attack` ≥ 100.

Companion sources (fire, bubbles, rain_synth, wind, breath, shimmer) may sit on the other streams. The five field sources appear 2 or 3 times each across the 12.

---

## 5. Old 64: review result (summary of `research/old-bank-review.md`)

**Defects to fix:**
- **Pads · Velvet Strings:** detune −8 / +8 ct between two streams at the octave beats at about 2.4 Hz. That breaks the realism rule. Fix: remove both detunes. The render moves.
- **Textures · Ember Field:** the bank's quietest preset at −30.0, with no level headroom. Fix: via re-voice 5 below.
- **6 descriptions** that don't match their values: Glass Mallet, Orbit Lines, Crush Tides, Ember Field, Rust Cloud, Gated Pad. Descriptions are not written to the preset or the hash, so these are free fixes.
- **Clean:** no output_gain anywhere, no Keys wobble, no dead routes, no filter settings with the filter Off, no settings on disabled streams.

**Re-voice proposals (ranked, each needs Taylor's approval at plan):**

| # | Preset | Change | Note |
|---|--------|--------|------|
| 1 | Keys · Kalimba | pluck → kalimba | the name calls for it |
| 2 | Drones · Organ Loft | harmonic ×2 → organ | |
| 3 | Pads · Warm Choir | vowel ×2 → choir | the choir source has its own detune / vibrato; Taylor's call |
| 4 | Pads · Velvet Strings | harmonic ×2 → bowed | plus the detune fix |
| 5 | Textures · Ember Field | dust → fire | fixes the level floor too |
| 6 | Drones · Hollow Wind | noise_bed → wind | |
| 7 | Textures · Rain Glass | stream 1 dust → rain_synth | |
| 8 | Keys · Glass Mallet | stream 1 bell → glass (stream 2 → marimba_bar optional) | glass is 2 s, so a held note re-strikes; lower sustain or accept |

- None of the eight uses an off-C3 source, so no root writes are needed.
- Kalimba, Glass Mallet, Rain Glass and Ember Field already sit at +6. If the new source is quieter, use density or vel_to_amp, not level.
- Old-bank usage: harmonic is in 20 of 82 lanes (6 of the 8 Pads). The re-voices spread that load.

---

## 6. Two-pane menu (summary of `research/preset-menu.md`)

**Today:**
- The menu is built entirely in `index.html` (`openPresetMenu()` at :4844–4872).
- The real box is 280 px at left 280 (the CSS at :554 overrides :142).
- Escape is the only key it handles.
- The page cannot tell factory from user: `getPresetList` returns the names merged, and only `isFactoryPreset(name)` exists, one name per call.

**Approach: plugin-local, with no change to the shared `preset-manager` module** (no fork, so no drift).
- One new native, `getPresetMenu`, from a pure encoder in `GiUiJson.h`.
- It returns `[{group, presets:[{name, short}]}]` in the group order of CONTEXT R5, with factory membership taken from the Factory folder. A user `Pads · X` therefore goes to User.
- Precedent: O-Orbit's `getPresetListGrouped`. The harness stub dump uses the same encoder.

**Layout:**
- 460 px at left 280 ends at x 740 of 1200.
- Split about 150 px groups and 310 px presets.
- A 20-row group is about 452 px tall, so `max-height` goes from 420 to about 480.
- Widest short name today: 78 px ("Threshold Catch"); worst case about 110 px. Widest French group: "Rythmiques", 57 px.

**Accessibility:**
- Two `role="listbox"` panes with `aria-activedescendant`, the container `role="group"`. This copies the factory picker at :4026–4107.
- Keys: Up / Down within a pane, Left / Right between panes, Enter loads, Escape closes. Hover on a group switches the right pane.
- "Load from file…" stays as a row under the panes.

**i18n:**
- 9 `list.pg*` keys in en / fr / zh, reusing existing terms (Bourdons, Terrain, Rythmiques, Nappes, 铺垫, 运动…). User reuses `label.user`. Textures takes `sameAsEn: true` in French.
- Headers are static `data-i18n` markup (check-i18n refuses computed keys).
- Append a `preset-menu-open` state LAST in `tests/i18n-states.json` and bump the state-count pin in `ui_binding_probe.js:2284` (49/50 → 50/51).

**Tests:**
- `ui_frontend_check.js`: page natives 22 → 23, `N_NATIVE` 36 → 37, and a new static check [18] modelled on [17].
- `preset_name_fit.js`: measure the short names in the right pane in en / fr / zh.
- New `ui_preset_menu_check.js`, ported from `ui_factory_picker_check.js`: group count, per-group counts 20 / 8 / 12 / User, the keyboard path, the ARIA wiring, plus a negative control.

---

## 7. Source allocation draft for the 96 (plan finalizes)

Every new source gets at least 2 uses. Multi-stream presets carry companions.

| Category (+12) | Lead sources |
|----------------|--------------|
| Keys | kalimba, marimba_bar, fm_bell, glass, crystal*, organ, metal_plate, guitar_drift*, guitar_overtones*, chord_sus, saw_stack, bowed |
| Pads | choir, bowed, organ, chord_minor, chord_sus, saw_stack, shimmer, reed, crystal*, breath, guitar_overtones*, sub* |
| Drones | sub*, contrabassoon*, organ, bowed, saw_stack, shimmer, wind, chord_minor, metal_plate, breath, fire, waves |
| Textures | rain_synth, wind, breath, fire, bubbles, metal_grate, shimmer, scissors, stick_snaps, clicks, glass, crystal* |
| Rhythmic | clicks, pulse_triplet, stick_snaps, scissors, metal_grate, marimba_bar, kalimba, bubbles, sub*, fm_bell, pulse, chord_minor |
| Motion | saw_stack, chord_sus, shimmer, guitar_drift*, reed, bubbles, metal_plate, crystal*, choir, waves, garden, fm_bell |
| Expressive | reed, bowed, breath, choir, saw_stack, contrabassoon*, sub*, guitar_drift*, organ, glass, shimmer, wind |
| Field | stream, park, waves, woods, garden (×2–3 each) with fire, bubbles, rain_synth, wind, breath |

\* = off-C3 root, written by the preset (FRT1).

**Notes:**
- **Keys:** the rule requires attack ≤ 5 ms, so slow-onset sources (bowed, organ) read from a position past their onset. That is a value choice, not a rule change.
- **Rhythmic:** Pulse Triplet's built-in grid is triplet, so pair it with a triplet `density_sync` (9 = 1/8 T, 6 = 1/16 T).
- **Expressive:** the new presets follow the existing split (MPE / mono-glide / played-control). The plan sets the new counts and FP0 is updated to match (§2.1).
- **Names:** each is `<Cat> · <Name>` ≤ 24 characters. "Expressive · " already takes 13, which leaves 11 for the name.
- **Seeds:** continue the scheme `0x4F4749 CC 01 0000 NN`, with Field as category 09.

---

## 8. Risks

| Risk | Mitigation |
|------|-----------|
| Loudness spread pushed past 9 dB | the −27 ± 2 target and the calibration loop; FP2 per run |
| Off-C3 source out of tune in a preset | FRT1 + FRT1n |
| Bank-hash move rewrites users' factory files | the existing stamp mechanism (FP5 covers it); user presets untouched |
| 160 near-duplicate presets | FP4 "fresh renders pairwise distinct" already covers it; the plan also gives each preset a distinct role |
| Menu probe state pins | append the state last and bump the pin (memory pattern) |
| Authoring volume: one context can't hold 96 presets | execute in waves: one category per wave, with FP0 / FP2 after each |
