---
plugin: O-GranulatorInstrument
stage: 3
phase: "3.1"
title: "Layout, controls, bindings: the v1 page in the WebView, 261 parameters bound, 34 native functions, presets, slot actions, value entry"
executed: 2026-11-10
version: 0.1.0 (unchanged, PD22; no tag)
commits: C1 35524e1b (processor UI surface + harness rows + overrides dump) · C2 12b210a4 (WebView editor, CMake, DevLoader removed) · C3 6ea16b17 (value entry + UI proof scripts + 19 states) · C4 (this summary, HANDS-ON-3.1, checklist, STATUS)
result: executed — every automated gate green; Stage 2 audio unchanged; HANDS-ON-3.1 written for verify
---

# O-GranulatorInstrument — Stage 3 / Phase 3.1 SUMMARY

The finalized v1 page now ships in the WebView editor at a fixed 1200 × 800. All 261 parameters are bound both ways by ID and type (176 slider / 70 combo / 15 toggle), all 34 native functions are registered and complete on every path, presets and slot actions are live, the DevLoader is gone, and the parent 1.2.0 value entry is ported. Stage 2 audio did not move. 3.1 closes neither UI-01 nor UI-02.

## What was built

- **Processor surface (T2–T4, C1; audio-neutral).**
  - Preset manager (`OuariconPresetManager`, files only): `setValidatedApply (true)`, `output_gain` + `rec_arm` excluded, `freeze` kept. Custom state = seed (int64 decimal string), custom scale, masks, slots. The session codec is untouched; `getStateAsXml` / `setStateFromXml` are never called.
  - Slots in presets as **sources without blobs** (PD1 sign-off): `index, source, name, fileSize, contentHash, embed, trimmed`. Key present → `restoreSlots`; key absent → slots untouched.
  - `getScaleStateRevision`, `getSlotStateRevision` (fingerprint), `getOverviewRev`, `getSlotOverview` (656 peaks folded from the loader's pristine overview), `getWindowCurve` over `Engine::windowCurve` (the real morph bank).
  - `Source/GiUiJson.h`: one set of JSON encoders shared by the editor and the harness.
- **Harness (T5, C1).** `tests/render-harness/ui_gates_31.h` (17 rows), `--ug31`, `--dump-ui-overrides` → `tests/ui-stub/generic-overrides.json` (141,773 bytes, LF, byte-identical twice, diff-clean at T14).
- **Editor (T7–T8, C2).** `GranulatorInstrumentEditor`: 261 relays + attachments, 34 natives (20 editor + 4 drop streaming + 10 preset manager), explicit resource ladder (10 resources), Windows block verbatim, `setSize (1200, 800)`, no timer, no `emitEvent`. UI files copied (4 `cmp` clean). CMake: exactly three edits. `PluginEditorDevLoader.{h,cpp}` and `OGI_DEV_LOADER` removed; `createEditor` switched in the same step.
- **Value entry (T11, C3).** 206 insertions / 4 deletions in `index.html`, identical in `mockups/v1-ui.html`. 161 knobs (144 + 17). New parsers `fCount`, `fBend`; scan mode by localized face + English choices; both `updateChoiceKnobVisual` branches guarded; capture-phase `pointerdown` commit.
- **UI proof (T12–T14, C3).** `tests/i18n-states.json` 16 → 19; `tests/ui_frontend_check.js`; `tests/ui_binding_probe.js`.

## Gate results (final tree, T15 unless noted)

| Gate | Result |
|---|---|
| Quality gate 2-dsp → 3-gui | PASSED (build, pluginval, DSP critic; review skipped, phase inside the stage) |
| Harness | **374 / 0**, 0 NOT FIRED; the 357 carried rows: diff vs baseline **empty**; 17 new rows PASS; UPM2n / UPM3n / UPM5n / USR1n each FIRED (predicate failed) |
| Parity | **168 / 0** + GV **2 / 0** |
| Renders | renders-2.4 + legacy **byte-identical** (also after T2–T4 and after T8) |
| param-dump | **identical** (261) |
| Fork drift | **0 / 13** |
| Build | VST3 + AU + Standalone + consoles rc 0; **0 warnings in plugin sources**. The first full build after the reconfigure printed 2 warnings outside the plugin (VST3 SDK `funknown.h` shadow, note-expression `NoteExpression_VST3.cpp:192` non-virtual dtor); not checked against a pre-3.1 build |
| auval | PASS (`aumu OuGI OuDv`) |
| pluginval 10 | VST3 SUCCESS (0 `!!!`) + AU SUCCESS (1 `!!!`, the informational "Current program is -1"), **sidechain on and off** (`OGI_GATE_SC_DEFAULT_OFF`); cache restored, shipping build reinstalled. Run at T9 and again at T15 |
| X2 | 0 `ogran::` and 0 test-hook symbols in the installed VST3 and AU |
| `generic-overrides.json` | twice byte-identical; `git diff --exit-code` clean |
| `ui_frontend_check.js` | ALL PASS (99 checks); 176 / 70 / 15; 34 = 34; NC1 (`glide` slider → toggle) and NC2 (`getWindowCurve` registration removed) both FAIL |
| `ui_binding_probe.js` | **261 / 261** host → UI, **261 / 261** UI → host; value entry **14 / 14**; NC (one `bindKnob` removed) FAILS at 260; on the unported page the VE rows FAIL loudly |
| check-i18n · fr-lint · zh-lint | 0 · 0 · 0 (454 zh entries at `reviewed: 'mt'`, counted) |
| check-ui-labels | ALL CHECKS PASSED; `seed=param-dump (261 rows) + overrides`, `states: default + 19` — before the port (16 states) and after |
| boot-all-uis `--strict-tips` | completes; 0 DEAD, 0 late |
| `cmp index.html v1-ui.html` | identical |

Binder counts (probe): knob 144, fader 16, handle 8, field 8, choice knob 17, segment 15, select 33, radio 1, scale 4, toggle 15 = 261.

## Deviations (each with its reason)

1. **PD1 sign-off vs `parameter-spec.md:355`.** Presets store slot sources, never embedded blobs. Signed off by Taylor at plan; the LOCKED spec is not edited. Revisit at Stage 4 with factory presets.
2. **`ovRev` (PD6) folds the loader's publish count:** `2 · (generation + publishedCount) + loaded`. `generation` moves when a job is requested, not when its tape publishes, and never on Reset (`SlotLoader.cpp:484, 614, 620-685`); with the plan's formula a reloaded lane would fetch the old waveform once and never refetch. The plan's fallback `getOverviewEpoch` is bumped on the audio thread and would not move with audio idle. The count is global, so a publish on one slot refetches all four lanes (redundant, never missed). USR2 asserts only the touched slot.
3. **Slot fingerprint (PD3) also folds the publish count** (a Reset changes no `SlotInfo` field). `testSlotRevNoGeneration` (test hooks only) drops both for USR1n.
4. **Row names carry a `U` prefix** (`UPM1` … `UTL31`); `PM1` and `SR1` already exist in the harness.
5. **UOV1 "ends < 0.05" judged per sample rate.** The loader's 512-frame bins are wider than the stub's 10 ms ramp at 48 k (head bin 0.248). The row checks the tail at 48 k and the head at 192 k and adds an exact comparison against an independent fold (worst diff 0).
6. **Checkpoint after T2–T4 taken once**, not after each task (one agent pass; renders, legacy and param-dump identical, so no attribution was needed).
7. **T10 ran before T9** (the gate baseline needs only source files); T11 was prepared in a scratch copy in parallel and applied after C2.
8. **Language-apply close is a wrapper in `init()`,** not an edit of `applyI18n`: that region is byte-compared against `scripts/i18n-canon.js` by check-i18n rule 6. `window.__setLanguage` is reassigned to the wrapper.
9. **Programmatic closes cancel** (language apply, Sync twin hidden) rather than commit; pointer and focus paths have already committed by then.
10. **PD17 states are not per-language.** The gate sweeps every state in en / fr / zh-Hans and the page closes an open field on each language apply, so the three states re-open the field after every apply and throw if it is not open (negative control: a wrong selector FAILS the gate). States: 54 px header cell (`output_gain`), a scan-mode knob, a division knob in Tempo.
11. **VE rows number 14** (PD19's list has 13 items; VE12 split into a / b, plus VE12c Sync-twin cancel).
12. **Probe loads a file host-side for four lane-handle rows** (`s3_loop_*`, `s4_loop_*` are hidden while the fixture's slots 3 / 4 are missing / empty), then restores the fixture.
13. **Drop natives registered by literal name** instead of the template's loop, so the closure counts 34.
14. **Slot JSON omits `path`, adds `loading` and `error`;** Choice defaults are emitted as ints.
15. **`applyNonParameterState` bumps the scale revision** (a session restore moves it too).
16. **T9 smoke: one window shot (default page), not four pages, and no open-field shot at T14.** Switching pages needs synthetic clicks on a machine the user was using. The default page was read: 1200 × 800, not blank, readouts bound, window curves real. The other pages are covered headless (19 states × 3 languages) and by HANDS-ON-3.1 rows 1, 8. A first region capture caught an unrelated window and was deleted; later captures were window-only.
17. **Editor close / reopen ×10** is pluginval's editor pass only (run 8 times across T9 / T15); no separate loop.

No audio-moving change, no contract edit, no module / `scripts/` / forked-file edit, no design change beyond PD16.

## Open items for verify / later phases

- **Dropped files are not durable (3.2).** A drop loads from the drop session's temp dir (reaped after ~5 min); a session saved afterwards reopens with that slot missing unless embedded. The path is live now.
- **Scan-mode `aria-valuetext` is not re-localized on a language switch** (pre-existing v1; readout is correct). Fix available: re-run `updateChoiceKnobVisual` over `CHOICE_KNOB_IDS` after a language apply. D40 call.
- **`output_gain` default reads 7.15e-07 dB,** not 0, in `getParameterDefaults` (the stepped range with a negative start); a double-click reset writes that value.
- **`kMul`, `PARSERS`, `openEntry` are declared mid-module** in the page, outside the block where module-level bindings normally live; harmless (`init()` runs last).
- **Not exercised anywhere automated:** JUCE relay ↔ attachment plumbing in a host, pointer drags, integer snapping in the real frontend (`fCount` / `fBend` round in the parser), the Windows block (presence only).

## Requirements

None moved in 3.1 (UI-01 / UI-02 stay pending until 3.2 / 3.3).

## HANDS-ON-3.1

`stages/3-gui/HANDS-ON-3.1.md`, 16 rows + the list of expected 3.1 placeholders. Run by Taylor at 3.1 verify.
