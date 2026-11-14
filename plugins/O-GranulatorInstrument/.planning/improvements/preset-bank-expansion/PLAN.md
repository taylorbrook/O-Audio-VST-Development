---
milestone: preset-bank-expansion
domain: mixed
execute_agent: general-purpose
version_bump: minor
base_version: 1.2.0
target_version: 1.3.0
created: 2026-11-14
---

# Plan: Preset bank expansion

**Plugin:** O-GranulatorInstrument
**Milestone:** preset-bank-expansion
**Created:** 2026-11-14
**Phase:** Plan

---

## Summary

**Improvement:** 96 new factory presets (7 sourced categories +12 → 20 each, Live stays 8, new Field 12; bank 160) so every one of the 41 factory sources is used at least twice; 8 approved re-voices and fixes to the old 64; a two-pane grouped preset menu.
**Approach:** Data in `Source/FactoryPresets.h`, authored one category per wave with FP0 / FP2 after each; the harness re-scaled to one `kBankSize` with two new rows (source coverage, preset roots); the menu plugin-local (one new native `getPresetMenu`, no change to the shared `preset-manager` module). No engine, parameter, range or state-format change.
**Version:** 1.2.0 → 1.3.0 (MINOR)

Domain is mixed (preset authoring, harness, WebView UI), so execute runs as general-purpose, as in 1.2.0.

---

## Decisions settled at plan (Taylor, 2026-11-14)

These close RESEARCH.md §2.4, §5, §7 and the CONTEXT "Open for research" list.

| # | Decision |
|---|---|
| P1 | **All 8 re-voices approved** (V1–V8 of `research/old-bank-review.md`): Keys · Kalimba (pluck → kalimba), Drones · Organ Loft (harmonic ×2 → organ), Pads · Warm Choir (vowel ×2 → choir), Pads · Velvet Strings (harmonic ×2 → bowed + D1), Textures · Ember Field (dust → fire), Drones · Hollow Wind (noise_bed → wind), Textures · Rain Glass (s1 dust → rain_synth; s2 noise_bed stays), Keys · Glass Mallet (s1 bell → glass; s2 pluck stays; `amp_sustain` lowered to 15–20 so the 2 s wrap does not re-strike audibly). Optional extras in the review (V7 s2 → glass, V8 s2 → marimba_bar) are **not** taken. |
| P2 | **D1** (Velvet Strings `s1_root_fine −8` / `s2_root_fine +8`) removed with V4. **Description fixes D6, D7, D8, D10, D11** applied as worded in the review; D9 is superseded by V5's rewrite. D3, D4, D5: no change. No other old preset changes. |
| P3 | **Wobble gate = preset-added only.** The realism check covers what the preset adds: `pitch_rand`, LFO / mod-env → `_pitch` / `_pitch_rand` / `_fine` routes, and `root_fine` detune between enabled streams reading the same source. A source's baked-in ensemble detune / vibrato (choir, bowed) is the instrument's character and is accepted. Player-driven routes (MPE / bend / wheel, sources ≥ 7) are not wobble. |
| P4 | **Pads · Fifth Bloom** stays bitwise unless the 160-bank FP2 spread exceeds 8.5 dB; then trim its three streams ~1 dB, drop it from the bitwise set, and report it in SUMMARY. |
| P5 | **Realism titles** (gated by P3, any category): Piano, Rhodes, Celesta, Harp, Strings, Choir, Organ, Kalimba, Marimba, Guitar, Bassoon. Matched as a whole word in the short name. |
| P6 | **Expressive split for the 12 new:** 4 MPE (≥ 1 with a bend route), 4 mono-glide, 4 played-control. Bank totals become exact: MPE 8, mono-glide 6, played-control 6. FP0 checks the exact new counts. |
| P7 | **Field rule** (FP0, every Field preset): ≥ 1 enabled stream on a field source (stream, park, waves, woods, garden), and on that stream `key_track` ≤ 50, `size` ≥ 150 ms, `scan_mode` ≠ Retrigger; global `amp_attack` ≥ 100 and `amp_release` ≥ 2000. Field-source counts across the 12: stream 3, park 3, waves 2, woods 2, garden 2. Companion streams may carry fire, bubbles, rain_synth, wind, breath, shimmer. |
| P8 | **Pads custom-tuning count** becomes `>= 2` (was `== 2`); the other coverage floors stay as `>=`. |
| P9 | **Seeds** `0x4F4749 CC 01 0000 NN`: CC = 01 Drones, 02 Expressive, 03 Keys, 04 Live, 05 Motion, 06 Pads, 07 Rhythmic, 08 Textures (existing), **09 Field**; new presets continue NN 09…20 in their category, Field NN 01…12. Re-voiced presets keep their seed. |
| P10 | **Level target** for new presets: FP2 gated −27 dB ± 2. When `sN_level` would pass +6, raise density or `vel_to_amp`, not level. |
| P11 | **FP4** stays whole at 5 × N = 800 pairs (≈ 30 s); no sampling. Long runs go to the background (600 s watchdog). |
| P12 | **Menu** is plugin-local per RESEARCH §6 / `research/preset-menu.md`; `preset-manager` module untouched. |

---

## Source allocation (lead sources per category, the 96)

Final per-preset choice is the executor's within this table; FSU1 (Task 2) enforces ≥ 2 presets per source across the whole bank. `*` = off-C3 root, the preset writes `sN_root_note` / `sN_root_fine` itself (crystal 73 / +17, guitar_overtones 72 / −32, guitar_drift 38 / −10, contrabassoon 20 / −19, sub 24 / 0).

| Category (+12) | Lead sources |
|---|---|
| Keys | kalimba, marimba_bar, fm_bell, glass, crystal*, organ, metal_plate, guitar_drift*, guitar_overtones*, chord_sus, saw_stack, bowed |
| Pads | choir, bowed, organ, chord_minor, chord_sus, saw_stack, shimmer, reed, crystal*, breath, guitar_overtones*, sub* |
| Drones | sub*, contrabassoon*, organ, bowed, saw_stack, shimmer, wind, chord_minor, metal_plate, breath, fire, waves |
| Textures | rain_synth, wind, breath, fire, bubbles, metal_grate, shimmer, scissors, stick_snaps, clicks, glass, crystal* |
| Rhythmic | clicks, pulse_triplet ×2, stick_snaps, scissors, metal_grate, marimba_bar, kalimba, bubbles, sub*, fm_bell, chord_minor |
| Motion | saw_stack, chord_sus, shimmer, guitar_drift*, reed, bubbles, metal_plate, crystal*, choir, waves, garden, fm_bell |
| Expressive | reed, bowed, breath, choir, saw_stack, contrabassoon*, sub*, guitar_drift*, organ, glass, shimmer, wind |
| Field | stream ×3, park ×3, waves ×2, woods ×2, garden ×2 (companions per P7) |

Checked at plan: with the old bank (after P1) this table alone covers all 41 sources ≥ 2 (pulse_triplet was 1 in the research draft; Rhythmic now takes it twice in place of a second `pulse`). Pulse Triplet pairs with a triplet `density_sync` (9 = 1/8 T, 6 = 1/16 T). Keys on slow-onset sources (bowed, organ) read from a position past the onset to keep `amp_attack` ≤ 5.

---

## Task Breakdown

Waves T3–T10 each end with a path-scoped commit (`git commit -- plugins/O-GranulatorInstrument`) after re-checking `git branch --show-current` and `git status --short`, so any wave can be restored on its own. No tag.

### Task 0: Backup and 1.2.0 references

**Outcome:** `backups/O-GranulatorInstrument/v1.2.0/` exists; the 1.2.0 harness binary's preset render dump (64 `.f32`), `--preset-level-table` and FP2 line are saved under `.planning/improvements/preset-bank-expansion/evidence/ref-1.2.0/`.

**Verification:**
- [x] Backup present and matches the tree at HEAD for `Source/`, `tests/`, `CMakeLists.txt`
- [x] 64 reference dumps; FP2 spread reads 7.00 dB (−30.0 … −23.0), matching R2

**Dependencies:** None · **Effort:** Small

---

### Task 1: Old-bank edits (P1, P2)

**Outcome:** V1–V8 and D1 applied in `FactoryPresets.h`; lane names (`customState` slot names) follow the new source; descriptions rewritten for the re-voices and D6/D7/D8/D10/D11. Each re-voiced preset re-trimmed toward its 1.2.0 gated loudness (± 1.5 dB), Ember Field lifted off the −30 floor by density / reverb (level is at +6). Glass Mallet `amp_sustain` 15–20. Velvet Strings / Organ Loft read past the source's start fade (`sN_position` ≈ 5–10 / `loop_start` ≈ 2 %) per the review.

**Verification:**
- [x] `kRevoiced13[]` (8 names) added beside `kShowcase`; `--cmp-preset-renders` against Task 0 dumps: the other 56 bitwise
- [x] FP0, FP1, FP2 green at 64 (harness still the 1.2.0 shape); spread ≤ 7.0 dB or better
- [x] The 8 re-voiced presets pass their category FP0 rule and the P3 wobble rule (checked by hand until Task 2 lands)
- [x] Path-scoped commit "improve: … T1 old-bank re-voices"

**Dependencies:** Task 0 · **Effort:** Medium

---

### Task 2: Harness re-scale and new rows

**Outcome:** In `tests/render-harness/preset_gates_4.h` / `main.cpp` / `tests/preset_name_fit.js`:
- `F::kBankSize = 160` and a per-category expected-count map (7 × 20, Live 8, Field 12) next to the table; FP0 checks the table against both; `checks` computed from N.
- FP0 lines 1103–1107, 1225 (P8), 1229 (P6 exact 8 / ≥ 1 / 6 / 6), 1237; FP1 `N == kBankSize`; FP3 per-category count 20; FP4 `nPairs == 5 * N`; FP4b / FP5 text and counts; `--dump-factory-presets` `n == kBankSize`; PN1 category regex adds Field and count 160.
- FP0 Field rule (P7); wobble rule widened (P3 / P5) to every realism-titled preset in any category.
- **FSU1** every manifest source in ≥ 2 presets' slots; **FSU1n** a bank copy with one source's uses removed FAILS.
- **FRT1** every preset slot with an off-C3 source: after apply, the stream's `root_note` / `root_fine` equal the manifest root; **FRT1n** a copy with one root dropped FAILS.
- `--cmp-preset-renders` mode: names absent from the reference are expected only if not in the 1.2.0 name list (a missing old name FAILS); `kRevoiced13` excluded by name.

**Verification:**
- [x] Old 64 (post-T1) pass the widened wobble rule and every unchanged FP0 rule; if any old preset trips P3, stop and report to Taylor (no unapproved old-preset change)
- [x] FSU1n, FRT1n fire; FSU1 / FRT1 / counts FAIL at 64 as expected (the only FP0 failures until T10 are the count / coverage lines — record the expected-fail list)
- [x] FPT1 unchanged and green

**Dependencies:** Task 1 · **Effort:** Medium

---

### Tasks 3–10: New presets, one category per wave (12 each)

Order: **T3 Keys, T4 Pads, T5 Drones, T6 Textures, T7 Rhythmic, T8 Motion, T9 Expressive, T10 Field.**

**Outcome per wave:** 12 presets appended to the category in `FactoryPresets.h`, each:
- name `"<Cat> · <Name>"` ≤ 24 chars, no `/ \ :`, unique; seed per P9; a distinct role in the category (stated in its description)
- lead source per the allocation table; off-C3 sources write their root pair
- the category FP0 rule (CONTEXT R1; Field per P7; Expressive per P6); realism titles per P3; never `output_gain`; no settings on disabled streams, no dead routes (the review's no-op list)
- level-calibrated (P10): author → `OGI_ROWS=FP2 --pg4` → scratch script computes −27 − gated → apply to enabled `sN_level` (clamp −60…+6, 0.1 dB) → re-run once

**Waves done:** [x] T3 Keys · [x] T4 Pads · [x] T5 Drones · [x] T6 Textures · [x] T7 Rhythmic · [x] T8 Motion · [x] T9 Expressive · [ ] T10 Field

**Verification per wave:**
- [ ] FP0 green except the recorded expected-fail count / coverage lines (Task 2)
- [ ] FP2 for the wave's 12 within −27 ± 2; bank spread ≤ 9 dB, peak ≤ 4.0
- [ ] FRT1 green for any off-C3 slot added
- [ ] The 56 still bitwise (`--cmp-preset-renders`)
- [ ] Path-scoped commit "improve: … T<n> <Category> +12"

**Dependencies:** Task 2 (each wave depends on the previous only for the commit sequence) · **Effort:** Large (total)

---

### Task 11: Two-pane grouped preset menu

**Outcome:** per `research/preset-menu.md`:
- `GiUiJson.h` pure encoder → `[{group, presets:[{name, short}]}]`, groups in R5 order (9 factory alphabetical, then User), factory membership from the Factory folder (a user `Pads · X` goes to User); native `getPresetMenu` in `PluginEditor.cpp`; harness stub dump uses the same encoder.
- `index.html` `openPresetMenu()`: 460 px box at left 280, ~150 px groups / ~310 px presets, `max-height` ≈ 480; two `role="listbox"` panes with `aria-activedescendant` in a `role="group"`; Up / Down within a pane, Left / Right between panes, Enter loads, Escape closes; hover on a group switches the right pane; opening selects the current preset's group; right pane shows the short name, preset bar keeps the full name; "Load from file…" / `.pm-file` items stay reachable under the panes.
- 9 `list.pg*` keys en / fr / zh (Textures `sameAsEn: true` in fr; User reuses `label.user`), static `data-i18n` headers.
- Tests: `ui_frontend_check.js` page natives 22 → 23, `N_NATIVE` 36 → 37, new static check [18]; `preset_name_fit.js` measures short names in the right pane en / fr / zh; `tests/i18n-states.json` appends `preset-menu-open` **last** and `ui_binding_probe.js` state-count pin bumps; new `tests/ui_preset_menu_check.js` (ported from `ui_factory_picker_check.js`): 10 groups, counts 20 × 7 / 8 / 12 / User, keyboard path, ARIA wiring, negative control.

**Verification:**
- [ ] ui_frontend_check (+ `--nc`), ui_preset_menu_check (+ negative control), ui_factory_picker_check, ui_binding_probe, check-i18n, fr / zh lint, check-ui-labels, PN1 all green
- [ ] Standalone screenshot of the open menu (en / fr / zh) in evidence; nothing clipped at 1200 px
- [ ] Path-scoped commit

**Dependencies:** Task 10 (menu counts are final only then; the UI code can be written earlier) · **Effort:** Medium

---

### Task 12: Full gate, build, install, docs

**Outcome:** 1.3.0 built, gated and installed.

**Verification:**
- [ ] Version 1.3.0 in `CMakeLists.txt` (and any version string)
- [ ] Full harness green (FP4 at 800 pairs in the background), FSU1 / FRT1 / controls fire
- [ ] `--cmp-preset-renders`: 56 bitwise vs 1.2.0 (55 if P4 fired), 96 new expected-missing, 0 old names missing
- [ ] `--preset-level-table`: no old-preset outlier; FP2 spread ≤ 9 dB, peak ≤ 4.0
- [ ] Bank stamp moves; FP5 (factory-file rewrite) green; 1.0.0–1.2.0 sessions / user presets load (existing rows)
- [ ] `./scripts/build-and-install.sh O-GranulatorInstrument`; auval; pluginval 10 VST3 + AU; Standalone opens
- [ ] CHANGELOG `## [1.3.0] - <date>`, SUMMARY.md, PLUGINS.md 1.3.0, STATUS.md / STATUS.yaml → execute complete; no tag

**Dependencies:** Task 11 · **Effort:** Medium

---

## Dependency Graph

```
T0 → T1 → T2 → T3 Keys → T4 Pads → T5 Drones → T6 Textures → T7 Rhythmic → T8 Motion → T9 Expressive → T10 Field → T11 menu → T12 gate / install
```

The UI code of T11 may be drafted in parallel with T3–T10; its counts and probe pins land after T10.

---

## Verification Criteria (milestone)

- [ ] 160 presets: 7 × 20, Live 8, Field 12; names / seeds unique; FP0 green with every category rule
- [ ] Every one of the 41 sources in ≥ 2 presets (FSU1 + FSU1n)
- [ ] Every off-C3 slot in tune (FRT1 + FRT1n)
- [ ] FP2 spread ≤ 9 dB, peak ≤ 4.0; new presets within −27 ± 2
- [ ] Old 64: the 8 approved re-voices + D1 + description fixes only; the other 56 bitwise vs 1.2.0
- [ ] Two-pane menu works by mouse and keyboard in en / fr / zh; file items reachable; user presets in User only
- [ ] auval, pluginval 10 VST3 + AU, installed

---

## Rollback Plan

- Per wave: `git restore --source=<prev wave commit> -- plugins/O-GranulatorInstrument`
- Whole milestone: `backups/O-GranulatorInstrument/v1.2.0/` or `git restore --source=832edfac -- plugins/O-GranulatorInstrument`

---

## Risks

| Risk | Mitigation |
|---|---|
| Spread past 9 dB | P10 target, per-wave FP2, P4 fallback |
| An old preset trips the widened wobble gate | Task 2 stops and reports; no unapproved change |
| Context overflow authoring 96 | One category per wave, committed; execute may `/clear` between waves and resume from the last wave commit (record the wave in STATUS.yaml) |
| Menu probe pins | state appended last, pin bumped (memory pattern) |
| Bank-hash move | existing stamp mechanism, FP5 |
