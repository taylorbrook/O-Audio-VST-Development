# Context: Preset bank expansion

**Plugin:** O-GranulatorInstrument
**Milestone:** preset-bank-expansion
**Created:** 2026-11-14
**Phase:** Discuss
**Version:** 1.2.0 → 1.3.0 (MINOR)

---

## Original Request

> Review and expand the presets to use all the different factory sounds. Make a new 96 presets and review the old ones.

---

## Decisions (Taylor, 2026-11-14)

| # | Question | Call |
|---|----------|------|
| D1 | Layout of the 96 | Drones, Expressive, Keys, Motion, Pads, Rhythmic, Textures each +12 → 20. Live stays 8 (Live templates carry no slots, so they can't show factory sources). New category **Field** with 12. Total 160 = 7 × 20 + 8 + 12. |
| D2 | Old 64 | Fix real defects, and re-voice a few where a new source clearly fits the name. Every proposed change to an old preset is listed (name, what changes, why) and approved by Taylor before it is written. A sound change to an old preset is accepted once approved. |
| D3 | Preset menu | Grouped menu, **two-pane**: left column the groups, right column the presets of the hovered / selected group. Wider menu (~460 px). No long scroll. |
| D4 | User presets | Own group, **User**, listed last. A user preset is never merged into a factory group, even when its name starts with a factory prefix. |
| D5 | Version | 1.3.0, MINOR. No parameter, range or state-format change. |

---

## Requirements

### R1 — 96 new presets, every factory source used

- 12 new per sourced category (7 × 12 = 84) + 12 Field = 96. Bank total 160.
- Every one of the 41 factory sources appears in at least **2** presets across the bank (old + new). The 33 sources new in 1.2.0 are currently in zero presets.
- Field draws on the field recordings (stream, park, waves, woods, garden) and related long textures (fire, bubbles, rain_synth, wind…); its own authoring rule is set at plan.
- Names: `"<Category> · <Name>"`, ≤ 24 characters, no `/ \ :`, unique; unique seeds; never localized.
- Every new preset in a category meets that category's FP0 rule (Keys: no pitch wobble, key track 100, attack ≤ 5 ms, release ≤ 2 s, vel_to_amp, window ≥ 0.5, spray ≤ 10; Drones: key track ≤ 50, ping-pong scan, feedback 30–80 with diffusion, release ≥ 5 s; Pads: ≥ 2 streams with an interval, slow envelopes, reverb 30–60, an LFO on position; Textures: density ≥ 70, spray 100; Rhythmic: Tempo sync, jitter 0, retrigger scan, density_sync on every stream; Motion: ≥ 6 routes with a per-voice LFO; Expressive: the existing split rule is extended, not broken). Plan may loosen a rule only with a stated reason.
- Keys / realism-titled presets: no chorus-like wobble (memory: realism titles must not wobble). Presets never set output_gain.

### R2 — In tune

- A preset load does not write the root. A preset that uses an off-C3 source writes that stream's `sN_root_note` / `sN_root_fine` itself: crystal 73 / +17, guitar_overtones 72 / −32, guitar_drift 38 / −10, contrabassoon 20 / −19, sub 24 / 0. Unpitched sources need no root.
- Acceptance: a harness row loads every preset that lists an off-C3 source and checks the root pair equals the manifest root for that lane (with a negative control).

### R3 — Level

- Every new preset level-trimmed so the bank loudness spread stays ≤ 9 dB (FP2) and no new preset is a level outlier in `--preset-level-table`; peak ≤ 4.0 over the FP2 script.

### R4 — Review of the old 64

- Review each for defects (level, tuning, dead / no-op routes, a description that no longer matches, a source that does not fit the name).
- Candidates for re-voicing named in discuss: Keys · Kalimba (→ kalimba), Pads · Warm Choir (→ choir), Pads · Velvet Strings (→ bowed), Keys · Glass Mallet (→ glass / marimba_bar), Textures · Rain Glass (→ rain_synth / glass). Research reviews all 64; the list is final only after Taylor approves it at plan.
- Old presets not on the approved list stay bitwise (render compare vs the 1.2.0 binary).

### R5 — Two-pane grouped preset menu

- Groups in this order: the 9 factory categories (alphabetical, as today: Drones, Expressive, Field, Keys, Live, Motion, Pads, Rhythmic, Textures), then User.
- Group headers localized en / fr / zh; preset names are not.
- Right pane shows the short name (prefix dropped); the preset bar keeps the full name.
- Opening the menu selects the current preset's group. Keyboard: Up / Down within a pane, Left / Right between panes, Enter loads, Escape closes. Hover on a group switches the right pane.
- The existing file items (save / import / export, `.pm-file`) stay reachable.
- Acceptance: ui_frontend_check, check-i18n, fr / zh lint, check-ui-labels, PN1 (names fit in the right pane in en / fr / zh), a menu probe (groups count, per-group counts 20 / 8 / 12 / user, keyboard path).

### R6 — Gates

- Every hard-coded 64 in the harness becomes the bank size (160): FP0 (counts per category: 20 × 7, Live 8, Field 12), FP1, FP2, FP4 (pairs scale; watch wall time — background long runs), FP5, `--dump-factory-presets`, PN1, preset render dump / compare.
- New coverage row: every factory source used ≥ 2 times (with a negative control).
- Full harness green, auval, pluginval 10 VST3 + AU, rebuilt and installed.

---

## Constraints

- No engine, parameter, range or state-format change. Sessions and user presets from 1.0.0–1.2.0 load unchanged.
- The bank stamp (`.bank-hash`) moves; the constructor rewrites the factory files (existing mechanism).
- Release tags only via `/publish`.
- Trunk-based; path-scoped commits.

---

## Out of scope

- New factory sources or edits to the 41.
- Preset morphing, favourites, search.
- Localizing preset names.

---

## Open for research

- Field category rule (what makes a Field preset).
- FP4 runtime at 160 presets (pairs ×2.5) — keep, sample, or split rows.
- How the preset-manager module (shared) is touched for the two-pane menu: plugin-local in `index.html` vs the module (FORK-REVIEW drift risk).
- Level calibration loop for 96 presets (script reading the gated loudness).
