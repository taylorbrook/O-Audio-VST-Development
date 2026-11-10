# Stage 3: GUI — Phase 3.1 (Layout and basic controls) - Context

## Discussion Summary

**Date:** 2026-11-10
**Participants:** User, Claude
**Phase goal (ROADMAP):** the finalized v1 mockup in the WebView, with every APVTS control bound.

## Starting state (observed)

- Stage 2 verified ⚠️ PARTIAL (automated green, `stages/2-dsp/VERIFICATION-2.6.md`); HANDS-ON-2.1–2.6 (79 rows) deferred by the user.
- `Source/PluginEditor.cpp` is the 12-line Stage 1 placeholder; `createEditor()` returns the DevLoader editor (`PluginEditorDevLoader.{h,cpp}`). `Source/ui/public/` holds only the gitignored module copies. No `index.html`, no `js/i18n.js`, no `tests/i18n-states.json`.
- Templates ready in `.planning/mockups/`: `v1-ui.html` (4871 lines), `v1-i18n.js`, `v1-PluginEditor.{h,cpp}` (261 relays: 176 slider / 70 combo / 15 toggle; 34 native functions), `v1-i18n-states.json` (16 states), `v1-integration-checklist.md` (authoritative step list). Gates were pre-verified on a scratch tree only.
- Processor API already present from Stage 2: `uiLanguage` + `languageCode` / `languageIndex`, scale state (`getCustomScale`, `setCustomScale`, `getDegreeMask`, `setDegreeMask`, `getCollectionTable`), `requestSlotLoad`, `getSlotInfo`, `requestReset`, `requestRelink`, `setSlotEmbed`, `getRecordState`, `getSidechainChannelCount`, `GiTelemetry` (peaks, head positions, swap state, UI grain / voice frame, live overview).
- Not yet present in the shape the template expects (research confirms each): `readTelemetry (Telemetry&)` aggregate, `getSlotStateRevision`, `getScaleStateRevision`, `getSlotOverview (…, 656, rev)`, `getWindowCurve`, `clearSlot`, `saveRecordingTo`, the drop `SessionManager` wiring, the preset-manager instance.
- The v1 page predates O-Granulator 1.2.0 (`e77d29c8`, click a knob's name to type a value) and does not contain it.

## Requirements Confirmed

- Window fixed 1200 × 800, not resizable; page `body` / `#plugin` the same size.
- All four pages render; all 261 parameters bound two-way by TYPE as well as ID; host automation and preset recall update the UI.
- Skewed readouts correct at the defaults listed in checklist §6.
- Explicit resource map with bare paths, `Juce` namespace passed, `withBackend (webview2)` + `withUserDataFolder()` on Windows, WebView never hidden.
- `webview-relay-manager` is not used (70 choice params, no WebView2 handling).
- UI gates on the real tree: `check-i18n`, `i18n-fr-lint`, `i18n-zh-lint`, `check-ui-labels`, `boot-all-uis --strict-tips`; exit 77 is not a pass.
- UI-01 / UI-02 stay pending until 3.2 / 3.3; 3.1 closes neither.

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| D37 | 3.1 native-function scope | **Register all 34 in 3.1, slot actions live.** Parameters, language, scale state, presets are complete. Slot functions that already have a processor API are wired for real: `loadSlot` (chooser → `requestSlotLoad`), `relinkSlot`, `resetSlot`, `resetAll`, `setSlotEmbed`, `getSlotState` (+ the slot revision). `getTelemetry` returns what exists plus well-formed empties. Filled later: lane overview / drop sessions / record head / playheads / grain dots / `clearSlot` / `saveRecording` / Record-in states (3.2); voices, sources, `modNorm` / `modRouted`, meters (3.3). A function not finished in 3.1 still completes its promise with a well-formed empty or a refusal, never hangs. | The DevLoader goes away in 3.1; files must still be loadable from the UI between 3.1 and 3.2. An unregistered function hangs its promise silently. |
| D38 | DevLoader | **Removed in 3.1** (`PluginEditorDevLoader.{h,cpp}` + its CMake entries), as carried from VERIFICATION-2.6. | D37 makes it redundant. |
| D39 | Deferred HANDS-ON-2.1–2.6 (79 rows) | **Run once at 3.3 verify** with the finished UI. A ❌ routes to `/plugin-execute O-GranulatorInstrument 2-dsp` for the owning phase. HANDS-ON-3.1 covers only 3.1's own rows. | Lanes, meters and the voice strip make every row observable; one session. O-Granulator D27 precedent. |
| D40 | Design latitude | **v1 locked, defects only.** Change only for breakage (clipping, dead bindings, contrast / fit gate failures). Settings gear `#gear-btn` at (1124, 62), 24 × 24, opening down-left, **accepted as placed**; the user looks at it beside the 604 px source dock at HANDS-ON-3.1. Closes checklist §1's designer item. | Mockup finalized and pre-verified; no taste edits mid-integration. |
| D41 | Value entry (parent 1.2.0) | **Port in 3.1** with the knob machinery: click a knob's name → readout becomes a text field; Enter applies, Esc cancels, click elsewhere applies; units optional, clamped, unchanged text writes nothing, one gesture per typed value; names light on hover. Covers the 144 Family A knobs and the 17 stepped / division knobs. Root fields, loop handles and matrix depth bars stay as designed. Include the capture-phase `pointerdown` blur commit. This is the one sanctioned addition to the locked v1 page. | The knobs are being bound now; porting later means touching the same code twice. |
| D42 | State-sweep gate | **`tests/i18n-states.json` (16 states) lands in 3.1.** The 132 unmeasured `[data-i18n]` nodes (toasts, voice cell states, remaining Record-in run states, scale-editor errors) get stub-override states in the phase that makes them live (3.2 / 3.3); 3.1 adds states for anything D41 introduces. | Parent D29 precedent; default applied, not asked. |

## Constraints Identified

- Member order: relays (3 vectors) → `dropSessions` → `webView` → attachments (3 vectors) → `webView2RuntimeLink`; constructor in the same order.
- `getParameterDefaults` reports every Float and every Choice (index).
- `getTelemetry` is a ~30 Hz pull; no `juce::Timer` push, no `emitEvent`. The editor never touches the pool, voices or tapes directly.
- No parameter writes from `parameterChanged` listeners (VST3 nested-write trap); loop handles set each parameter directly.
- Stage 2 audio must not move: harness 357 / 0, parity 168 / 0, renders-2.4 + legacy byte-identical, param-dump identical, fork drift 0 / 13 re-run after the editor swap. X2 (test hooks absent from shipping binaries) stays 0.
- Preset apply: reset to defaults first, strip the stale `<CustomState>` child, two-reopen persistence test.
- Value entry (D41): the readout node is a clamp-gate census member (`.knob-value`); the text field must not break `check-ui-labels`, and French / zh widths must hold with the field open.
- Canvas text waits on `document.fonts.load` before first paint.
- CMake: no comment spelling the plugin-target call with its parenthesis; module JS under `Source/ui/public/modules/` is a gitignored build artefact and is never committed; check-i18n rule [8] needs the resource map in the real `PluginEditor.cpp`.
- pluginval 10 VST3 + AU and auval re-run with the WebView editor, sidechain on and off.
- Path-scoped commits on `main`; cache clear + dual-variant sweep on every install.

## Open Questions (for research)

- Exact `getTelemetry` JSON shape the page's 30 Hz consumer reads (read the controller, not the checklist prose); confirm the 3.1 empties (grains 0, voices off, `rec.bus` from `getSidechainChannelCount`) do not throw and leave the lanes / VOICE / MOD pages visually sane.
- Map `GiTelemetry` + `getRecordState` (state | reason << 8 | heldArm << 16) onto the template's `rec.run` 0–5. Carried: "Done / Discarded" = Idle + reason (SUMMARY-2.6 dev. 13) and the SC peak choice, raw `scInPeak` vs post-gain `takeScPeak` (dev. 12). The mapping is decided in 3.2; 3.1 only needs a stable placeholder.
- Which of the 34 need new processor surface in 3.1 (`getSlotStateRevision`, `getScaleStateRevision`, preset-manager instance, `getWindowCurve` real or flat stub) and whether any of it touches the audio thread.
- Does `loadSlot` in 3.1 without `getSlotOverview` leave the lane readable (name / badge shown, empty waveform), or should a pristine-overview read land in 3.1?
- Parent 1.2.0 diff (`e77d29c8`): which hunks port verbatim, which need instrument parsers (new units: st, ct, bars for Record Length, root note names are excluded), and what the parent's gates for it were.
- `getParameterDefaults` for the 17 stepped Choice knobs vs `bindChoiceKnob` reset.
- Method for the 261-param two-way / automation check (Standalone + Playwright param-dump diff, as the parent did for 220).
- Whether `v1-CMakeLists.txt` and the tree's `CMakeLists.txt` have diverged since Stage 1 (FactorySources target, test hooks) and what the WebView block still needs.

## Carried items (not 3.1 scope)

- Lanes display, drop sessions, overview, record head, playheads, grain dots, Record-in UI, clear / save recording (3.2).
- MOD sources, mono badges, mod rings, voice strip, meters, `grain_limit` heavy hint (3.3).
- HANDS-ON-2.1–2.6 and the Logic AU DAW pass (3.3 verify, D39).
- Factory sources + presets (FUNC-08), cross-host sidechain (QUAL-02), COMPAT-02 (Stage 4).

## Next Phase

Ready for: research phase (Phase 3.1)
