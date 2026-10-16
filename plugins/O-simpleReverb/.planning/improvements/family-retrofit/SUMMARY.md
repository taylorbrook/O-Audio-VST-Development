# Summary: O-simple family retrofit — Execute (IN PROGRESS: T0–T7 done, T8–T10 open)

**Plugin:** O-simpleReverb (was O-SimpleReverb) · **Milestone:** family-retrofit · **Base:** 2.0.0 → **Target:** 3.0.0 (bump post-verify)
**Executed:** 2026-10-15 · **Backup:** `backups/O-SimpleReverb/v2.0.0/` (diff-clean at T0)

## Commits (path-scoped, main)

| Commit | Tasks | Content |
|---|---|---|
| `8f6d3b19` | T1 | Case-only rename (two-step `git mv`), CMake family flags, literals, Q4 guarded folder move, canary + README, retire `ui_tip_render_check.js`, milestone docs |
| `030c0eb2` | T2 | `tests/pedalboard/verify_3_null.py` |
| `cc3e9f3c` | T3–T5 | Tone-map statics; MeterFrame + TripleBuffer + MeterFramePayload; PresetRecipes.h + preset core + editorScale; toneGainDb / earlyTaps models |
| `fa19554d` | T6–T7 | Editor rewrite (20 natives), page + i18n.js, insects.png, `tests/ui-stub/generic-overrides.json`, `tests/ui-fixtures/frames.json` |

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

## Remaining (resume here)

- **T8:** port the 4 Amb probes to `tests/ui-probes/` (frame-probe on `frames.json` — hall/booth/ambient bursts, clip_sine, idle_edge; header-fit `NAME='O-simpleReverb'`; lesson-row-budget for the TWO rows pinned at fr; tooltip-clamp) and run each at 0.6 / 1.0 / 1.5 × en / fr / zh-Hans; rewrite `tests/i18n-states.json` (LPON out, Plate/Spring wells, preset popover, gear popover) and run `check-ui-labels --plugin O-simpleReverb`; write `tests/pedalboard/verify_3_state.py` (v2.0.0 raw_state → v3 values by name + uiLanguage; editorScale round-trip).
- **T9:** `./scripts/build-and-install.sh O-simpleReverb`; auval; pluginval level 10 VST3 + AU; render-check full + mutants; `verify_3_null` on the INSTALLED VST3; `verify_3_state`; `ls … | grep -i simplereverb` new names only.
- **T10:** `.planning/` BRIEF / REQUIREMENTS / ROADMAP / STATUS (Amb frontmatter shape, keep `activeMilestone`), regen `params.tsv`, CHANGELOG 3.0.0 draft (rename, why no migration, guarded rename, ◀/▶ order, dropped Save-to-file), NOTES.md, PLUGINS.md Reverb row only, `regen-registry-used-by.sh` Reverb hunks only, duplicate-row check.

Harness notes for resume: v2 out-of-tree build + reference renders live in the session scratchpad (gone after /clear) — rebuild with `git archive O-SimpleReverb-v2.0.0 CMakeLists.txt modules plugins/O-SimpleReverb scripts | tar -x -C <dir>` + copy `Source/ui/public/modules/*.js`, configure, `ninja O-SimpleReverb_VST3`; render with `uv run --no-project --python 3.12 --with pedalboard --with numpy python plugins/O-simpleReverb/tests/pedalboard/verify_3_null.py render <bundle> <out.npz>` (zsh: do not put the uv command in a variable).
