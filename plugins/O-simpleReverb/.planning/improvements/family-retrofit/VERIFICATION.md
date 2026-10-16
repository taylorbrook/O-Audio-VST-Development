# Verification: O-simple family retrofit

**Plugin:** O-simpleReverb (was O-SimpleReverb)
**Milestone:** family-retrofit
**Created:** 2026-10-15
**Phase:** Verify

---

## Verification Summary

**Status:** AUTOMATED PASS. One human gate is open: the DAW hands-on (Logic + Ableton).
**Version:** 2.0.0 → 3.0.0. The bump waits until the DAW gate closes. No tag (tags are cut by `/publish` only).

Every number below was re-produced in this phase, not copied from SUMMARY.md. The v2.0.0 reference was rebuilt out of tree from the tag `O-SimpleReverb-v2.0.0`. All the null and state runs used the **installed** `-dev` VST3.

The independent code review found one real compatibility defect: a v2 user preset named like a lesson was hidden. It was **fixed in this phase** (below), and the whole matrix was re-run on the rebuilt binary.

| What was re-run | Result |
|---|---|
| Clean rebuild of `O-simpleReverb-render-check` (`ninja -t clean`, 65 steps) | 0 warnings |
| `render-check` (after the fix) | **139 PASS, 0 FAIL, 0 PEND** (137 + 2 new gates) |
| `render-check --mutants` | **32 of 32 caught** |
| `build-and-install.sh O-simpleReverb` (after the fix) | 0 warnings. Only `O-simpleReverb-dev.{vst3,component}` are installed |
| `auval -v aufx OuSr OuDv` | AU VALIDATION SUCCEEDED. `auval -a` lists one entry, named O-simpleReverb-dev |
| pluginval strictness 10, VST3, GUI tests skipped / with GUI tests | SUCCESS / SUCCESS |
| pluginval strictness 10, AU | SUCCESS |
| `verify_3_null`: v2.0.0 vs v2.0.0 | 290 buffers, BIT-EXACT (deterministic) |
| `verify_3_null`: v2.0.0 vs installed v3 (before and after the fix) | 290 buffers, **BIT-EXACT** |
| `verify_3_null`: positive control (`--flip-lpon`) | DIFFERS, as it must |
| `verify_3_state` (v2.0.0 dump → installed v3 check) | S0–S5 ALL PASS |
| `check-i18n` / `i18n-fr-lint` / `i18n-zh-lint` | ALL PASS / CLEAN / PASSED (181 zh entries at `'mt'`, counted, not failed) |
| `check-ui-labels --plugin O-simpleReverb` | ALL CHECKS PASSED |
| `boot-all-uis --strict-tips` | clean 1/1, 0 dead tip bindings, no i18n diagnostics |
| frame-probe / header-fit / lesson-row-budget / tooltip-clamp | 36/36 · ALL PASS · ALL PASS · ALL PASS |
| Negative controls freeze / slope / chipbody | F3 / F2 / H1 FAIL as designed |
| `Source/dsp/*.h`, `ModulationFx.h` vs v2.0.0 | Banner comments only |
| `params.tsv` vs v2.0.0 | Only the `# plugin` name line differs |

---

## Requirements Verification (CONTEXT.md success criteria)

| # | Criterion | Status | Evidence |
|---|---|---|---|
| 1 | Mockup signed off before plan | ✓ PASS | 2026-10-15 (STATUS.yaml `mockupSignedOff`) |
| 2 | v2.0.0 sessions and presets load with identical values; auval + pluginval 10 for VST3 + AU | ✓ PASS | verify_3_state S0–S5; render-check state gates; auval; pluginval ×3 |
| 3 | Four wells respond live; the decay well tracks the real tail | ✓ PASS (automated) | frame-probe F1–F4; render-check: tone well = running filters, ER native sample-exact, decay-well slope gate. Live in a DAW → human gate |
| 4 | Ten lessons + Init from PresetRecipes.h; 48 utility presets browsable | ✓ PASS | render-check lesson gates (11/11 resolve, exact, idempotent); catalog 11 + 48; lesson-row-budget 11 clicks → 11 ids |
| 5 | Family gates green; render-check + mutants | ✓ PASS | Table above |
| 6 | Render nulls against v2.0.0 | ✓ PASS | 290 buffers bit-exact on the installed VST3, after the fix too |
| 7 | 0 warnings; old bundle names swept | ✓ PASS | install log 0 warnings; `ls` shows only the new-name `-dev` bundles |

---

## Code review (independent, read-only)

| Area | Verdict |
|---|---|
| Audio thread | Clean. No allocation, lock or logging added. The meter accumulators are bounded by the halving cap. The tone-map statics keep the original float expressions exactly |
| State | Clean. The guarded folder rename cannot lose presets: it moves only when the new folder is absent and the old one exists, and is a no-op on APFS and NTFS |
| Presets | Correct (reset → recipe via `convertTo0to1`, factory and lesson save refusal, walk order) |
| Editor | Correct. Relays → WebView → attachments; 20 = 20 natives; SafePointer on the chooser; Windows `withBackend(webview2)` + `withUserDataFolder` |

### Findings

1. **FIXED: a v2 user preset named like a lesson was hidden.** v2.0.0 had no lessons, so `User/Init.json` was a legal name. In v3, `isFactoryPresetName` matched it, so it dropped out of the list, `loadPresetByName("Init")` loaded the lesson, and the preset could not be deleted.
   - **Fix:** `migrateLessonNameCollisions(userDir)` runs in the constructor, after the folder rename. It renames each colliding file to `<name> (user).json`, or `<name> (user 2)` etc. if that is taken. It never deletes, and a failed rename leaves the file as it was.
   - **Session restore:** a restored lesson name records a candidate. The editor-timer refresh (message thread, where file reads already happen) relabels the session to `Init (user)` only when the session values match the renamed file and not the lesson.
   - **Gates:** the rename on a temp dir (2 renamed, re-run renames 0, a pre-existing `Init (user)` gives `Init (user 2)`, a non-colliding name is untouched); the restore decision (file / lesson / neither); a restored lesson stays the lesson after the refresh. CHANGELOG Notes entry added.
2. **OPEN, family-wide: the editor scale is overwritten by the display cap.** `resized()` stores the capped (or auto-fit) scale, so a 1.3× chosen in the studio becomes about 0.9× after one session on a laptop. The code is identical in O-simpleAmbisonics (`PluginEditor.cpp:446,563`), so this is a faithful port, not a regression. Fixing it belongs to a family-wide pass, not to this milestone.
3. **OPEN, low, pre-existing:** the module's `currentPresetName` (a plain `juce::String`) is written on the message thread and read by `getStateInformation` with no lock. The race existed in v2; v3 has more writers.
4. **OPEN, low:** a second "Load file…" while the first chooser is open leaves the first promise pending. Harmless.

---

## Human gate (open)

- [ ] Logic: a rescan lists "O-simpleReverb-dev"; a v2.0.0 session reconnects with its values; the wells move live; the resize persists across a reopen
- [ ] Ableton: the same

When the gate closes, run the post-milestone step: CMake `VERSION "3.0.0"`, the CHANGELOG date, the NOTES status, the PLUGINS.md version, a registry regen (Reverb hunks only), and clear `activeMilestone`. **No tag.**

---

*Generated by improve-milestone verify phase*
