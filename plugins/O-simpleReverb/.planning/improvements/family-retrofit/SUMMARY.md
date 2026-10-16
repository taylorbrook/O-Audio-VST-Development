# Summary: O-simple family retrofit — Execute (COMPLETE: T0–T10)

**Plugin:** O-simpleReverb (was O-SimpleReverb) · **Milestone:** family-retrofit · **Base:** 2.0.0 → **Target:** 3.0.0 (bump post-verify)
**Executed:** 2026-10-15 · **Backup:** `backups/O-SimpleReverb/v2.0.0/` (diff-clean at T0)

## Commits (path-scoped, main)

| Commit | Tasks | Content |
|---|---|---|
| `8f6d3b19` | T1 | Case-only rename (two-step `git mv`), CMake family flags, literals, Q4 guarded folder move, canary + README, retire `ui_tip_render_check.js`, milestone docs |
| `030c0eb2` | T2 | `tests/pedalboard/verify_3_null.py` |
| `cc3e9f3c` | T3–T5 | Tone-map statics; MeterFrame + TripleBuffer + MeterFramePayload; PresetRecipes.h + preset core + editorScale; toneGainDb / earlyTaps models |
| `fa19554d` | T6–T7 | Editor rewrite (20 natives), page + i18n.js, insects.png, `tests/ui-stub/generic-overrides.json`, `tests/ui-fixtures/frames.json` |
| `724166cb` | T8 | `tests/ui-probes/` (4 probes), `tests/i18n-states.json` (38 states), `tests/pedalboard/verify_3_state.py`; language-width pins in index.html |
| (T10 commit) | T10 | `.planning/` BRIEF / REQUIREMENTS / ROADMAP / STATUS, params.tsv, CHANGELOG 3.0.0 draft, NOTES, PLUGINS.md row, registry `used_by` Reverb hunks, this SUMMARY |

## Verified so far

- **Null vs v2.0.0: bit-exact** after T1, T3, T4, T5, T6 (290 buffers: 48 presets × 3 stimuli × 44.1/96 k + automation). v2 vs v2 identical (deterministic). Positive control (LPON flipped) DIFFERS.
- **render-check: 137 PASS, 0 FAIL; mutants 32/32** (new gates: meter hold, 4 payload gates, 5 preset gates, tone well = running filters 0.000 dB over 420 points, ER native sample-exact 12/12 + first arrivals, decay-well slope 0.96–1.10 of drawn; new mutants `frameNoReset`, `toneCurveStale`).
- Negative control: new `main.cpp` compiles against the v2.0.0 Source tree (`__has_include` guards).
- auval PASS (T1 build). Build 0 warnings (T7).
- Census: page + preset-manager.js natives == editor `withNativeFunction` (20 = 20).
- check-i18n ALL PASS, i18n-fr-lint CLEAN, i18n-zh-lint PASSED, boot-all-uis clean (0 dead tip bindings). `grep -i absorption Source/ui` = 0.
- Screenshots en / fr at 1.0× with a real C++ Hall burst (frames.json): wells live, brass slope on the tail, M1 baselines aligned.

## Deviations (decided during execute)

1. **Recipes** follow RESEARCH §8 (plan-locked), not the mockup's stand-in RECIPES; captions/tooltips were rewritten to match §8 values (e.g. Low Cut 250 Hz, sizeVsDecay = Room SIZE 0 DECAY 2.0).
2. **Sigpath columns rebalanced** (input 112 / early 152 / engine 216, sum unchanged = Engine group width) so the plan's Engine sub-lines fit ("16 lines · Hadamard · slow modulation", "Dattorro figure-8 · octave shimmer in the loop").
3. **Spring ER well** draws the springs' own echoes (33/37/41 ms × size) as brass markers — the mockup's "starts diffuse" was inaccurate (RESEARCH §3).
4. **Tone native** also returns `typeEqDb[]`, `characterDb[]`, `warmCutoffHz`, `brightDb` (per-stage curves + readouts from C++, no JS map copy).
5. **Decay-well gate stimulus** = pink noise (RESEARCH §6.2 "programme-like"); white noise read ~15 % short because the top octaves dominate the mean square and decay fastest. Fit window 3–45 dB under the stop level (≥ 4 points even for Booth 0.5×).
6. **Tone station sub-line** split into two literally-keyed spans (check-i18n [15] does not credit keys passed as var values).
7. **Band heights:** viz-band 334 → 324, controls 214 → 224 (M1's dropped Tone/Mix knobs collided with the group notes).
8. fr output-meter caption "Crête · dir. + traité" (the full form truncated).
9. Build-tree trap: `build/plugins/O-SimpleReverb/` kept the old case for artefact bundles — deleted and reconfigured (memory written).

## T8 — test tree and suite gates

- **4 probes** in `tests/ui-probes/`, each at 0.6 / 1.0 / 1.5 × en / fr / zh-Hans:
  - `frame-probe.mjs` — 36 / 36. F1 hall burst: sage tail, amber input, anchored brass slope, held value at the right edge, scrolls while live. F2 slope width = (anchor + 60) / 60 × baseT60 × DECAY over 6 s for Hall (148.5 vs 146.3 px) and Booth (23.5 vs 20.9), anchor recomputed from the payloads. F3 live → idle freezes (DECAY 1.0 → 0.5 halves the slope, x 0.498; the sage edge stays put). F4 clip lamps + full bars on clip_sine, empty on idle, lamps clear after the hold. Negative controls `freeze` → F3 FAILS, `slope` → F2 FAILS.
  - `header-fit-probe.mjs` — ALL PASS. H1 tail chip × 6 types × DECAY 0.5 / 2.0; H2 header (title 425, gear 1135 in every language); H3 11 lessons + 48 bank names unellipsised, 64-char user name ellipsised with no geometry move; H4 ellipsis census (41 boxes × 9 states). Control `chipbody` → H1 FAILS.
  - `lesson-row-budget.mjs` — ALL PASS. Two rows; fr slack Spaces 434.8 px, Ideas 460.3 px; 11 clicks → 11 `applyFactoryPreset` ids in order; P11 (click lights nothing; push lights; modified lights nothing).
  - `tooltip-clamp-probe.mjs` — ALL PASS. 7 anchors × 3 viewports × 3 languages; right clamp engaged 9× per viewport; left clamp required at 0.6× / 1.0× (6× / 3×), geometrically impossible at 1.5× (noted, not required).
- **check-ui-labels** — first run on the v3 page failed 78 (all [7] geometry: content-sized well captions, the composed Tone station sub-line, the zh Character readout line box, the NONE caption). Pinned at the measured widest language (comment beside the pins) → **ALL CHECKS PASSED** over 38 states. check-i18n, i18n-fr-lint, i18n-zh-lint, boot-all-uis (0 dead tip bindings) green after the pins; all 4 probes re-run green.
- **verify_3_state.py** — S0 v1.14.0 blob, S1 v2.0.0 session (built out of tree from the tag) → identical values by name and in the re-saved XML; S2 uiLanguage fr; S3 no editorScale; S4 1.250 round-trips, 2.000 / "abc" dropped; S5 values unchanged. ALL PASS (build artefact, then the installed VST3).

## T9 — install and validate

- `build-and-install.sh O-simpleReverb`: 0 warnings; installed `O-simpleReverb-dev.{vst3,component}` only (`ls … | grep -i simplereverb`).
- auval `aufx OuSr OuDv`: SUCCEEDED.
- pluginval strictness 10: VST3 SUCCESS (with and without the editor tests), AU SUCCESS.
- render-check: 137 PASS / 0 FAIL; `--mutants` 32 / 32 caught; render-check build 0 warnings.
- verify_3_null on the **installed** VST3 vs v2.0.0 (rebuilt out of tree): 290 buffers, **BIT-EXACT**.

## T10 — docs and registry

- `.planning/BRIEF.md`, `REQUIREMENTS.md` (12 reqs), `ROADMAP.md`, `STATUS.md` (frontmatter first; `activeMilestone: family-retrofit` kept). The v1 `creative-brief.md` / `architecture.md` / `plan.md` stay as history.
- `params.tsv` regenerated by param-dump: only the plugin-name line changed (the 8 parameters are identical).
- `CHANGELOG.md` `## [3.0.0] - unreleased` draft (rename, wells, lessons, ◀ / ▶ order, Save-to-file dropped, no migration + guarded rename); finalise the date at the bump.
- `NOTES.md` timeline row; `PLUGINS.md` Reverb row only; `modules/registry.yaml` Reverb hunks only (preset-manager O-SimpleReverb 1.5.7 → O-simpleReverb 2.0.0, eb-garamond + O-simpleReverb 2.0.0) and the header bump — the full regen also rewrote ~60 other plugins' stale versions, left for whoever owns that sweep.
- Live `O-SimpleReverb` hits: zero (left by design: improvement briefs, the guarded-rename literal, the v1.14.0 fixture writer, repo-level history docs).

## For verify

- DAW hands-on not done in execute: Logic + Ableton rescan shows O-simpleReverb, a v2.0.0 session reconnects with its values, the wells move live, resize persists across reopen.
- At the post-verify bump (2.0.0 → 3.0.0): CMake `VERSION "3.0.0"`, CHANGELOG date, NOTES status, PLUGINS.md version, and re-run the registry regen keeping the Reverb hunks (the `used_by` version reads CMake). No tag.

Harness notes: v2 out-of-tree build + `v2state.json` + `v2.npz` live in this session's scratchpad (gone after /clear) — rebuild with `git archive O-SimpleReverb-v2.0.0 CMakeLists.txt modules plugins/O-SimpleReverb scripts | tar -x -C <dir>` + copy `Source/ui/public/modules/*.js`, `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release`, `ninja O-SimpleReverb_VST3`; then `verify_3_state.py dump <v2.vst3> <state.json>` / `check <v3.vst3> <state.json>` and `verify_3_null.py render` / `compare`.
