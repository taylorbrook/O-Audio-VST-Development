# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-10-06
**Participants:** User (Taylor), Claude

Stage 4 ships O-simpleWavetable v1.0.0. The DSP has been frozen since the Stage 2 re-verify, and the Stage 3 binary null test against `850df9b9` was 6/6 bit-exact. Every Stage 4 DSP change is a named fix from a critic finding, and each one needs its own gate.

## Requirements Confirmed

- **FUNC-08 (factory presets): full preset manager.**
  - Integrate the suite-canonical `preset-manager` module (registry v1.0.9, `OuariconPresetManager` + `preset-manager.js`, served via the resource provider).
  - Add a preset browser panel to the UI: list, prev/next, save and delete. It must fit the fixed 1120 × 780 page.
  - **Factory bank:** Init / Additive Build plus the 8 §A9 recipes. Raw → normalized via `convertTo0to1` (memory: factory presets ignore skew).
  - Presets **never set `output_level`** (suite rule; the same rule as `applyFactoryPreset`).
  - Keep `getNumPrograms() == 1`. Presets live in the WebView only, not in the host program menu (suite norm).
- **Lesson buttons reconciled to §A9.** They stay. Alias Demo moves from Sine→Saw to the **Drive** bank (Pos 100 %, Band-limit Off), closing ARCHITECTURE row 17's "§A9 remains the Stage 4 target". The lesson recipe ids and the factory preset recipes should come from one table, not two copies.
- **Critic fixes, all warnings:**
  - Stage 3 W1: read the counters in `uiReady` before emitting.
  - Stage 3 W2: the 96 MB drop freeze. Measure first, then lower the cap, pre-size the block, or decode on `importPool`.
  - Stage 3 W3: accumulate trackpad wheel px and step every `WHEEL_PX_PER_NUDGE`.
  - Stage 3 W4: reset the stale import error when the selection leaves Imported.
  - Stage 3 W5: track UI-held notes and release them in the editor destructor.
  - Stage 2 W3: seed the Mono wheel after Poly→Mono.
  - Stage 2 W4: the upward-legato crossfade source mip.
- **Critic fixes, triaged notes:**
  - **Fix:**
    - Stage 3 N1: make the FileChooser a member.
    - N4: clear the display state in `releaseResources`.
    - N7: strip C1 and bidi characters in `sanitiseName`.
    - N9: add the `unsupported` key for newer-format sessions.
    - N10: the stale lesson highlight.
    - N11: tooltips on `#octDown` and `#octUp`.
    - N12: a Unicode minus for `-inf`.
    - N13: one gesture per stepped-knob drag.
    - Stage 2 note 3: the MIDI scratch buffer growth path.
    - Stage 2 note 4: a guard for `processBlock` before `prepareToPlay`.
    - Stage 2 note 8: the `-Wundef` `OSIW_TEST_HOOKS` include.
  - **Accept and log in CODE_REVIEW.md as by-design:**
    - Stage 3 N6: a lesson during Mono gets the 2 ms hard-stop tail.
    - Stage 2 note 1: the Poly↔Mono hard stop.
    - Stage 2 note 2: Interp-Off frame-step ticks, the accepted lesson.
  - **Research decides fix or log:**
    - Stage 3 N2: torn display atomics.
    - N3: `importedOwner` read under the lock (allowed by Amendment 18).
    - N5: a pending auto-select overriding a lesson.
    - N8: two gesture owners on `position`.
    - Stage 2 note 5: Mono ignores sustain.
    - Stage 2 note 6: no import cancel.
    - Stage 2 note 7: save/restore decoding under `bankStateLock`.
- **PERF-02:** 16 voices at 44.1, 48, 88.2 and 96 kHz, measured on the Release build. Report CPU per block as a fraction of the real-time budget.
- **COMPAT-02 (Windows VST3, WebView2 static): CI only.**
  - Confirm the CMake config: `NEEDS_WEBVIEW2` + static linking, and `withUserDataFolder()` (memory: WebView2 static linking, runtime gotchas).
  - Get a green Windows build + pluginval from `build-and-release.yml` via `workflow_dispatch` validate-only. That mode publishes no release and needs no tag.
  - No hands-on Windows DAW test this stage.
- **QUAL-04: measured gates plus a by-ear sign-off.**
  - New measured contrast gates:
    - stepped vs smooth: step discontinuity energy, Interp Off vs On
    - aliasing vs clean: alias dB, Band-limit Off vs On, on Alias Demo
    - bit depth: SNR per bit setting, monotonic
  - Then Taylor listens to the factory presets and signs off (human checkpoint).
- **Release:**
  - VERSION 1.0.0 in CMakeLists
  - CHANGELOG.md with a bracket-form `## [1.0.0]` heading (memory: the heading must be in bracket form)
  - `plugins/O-simpleWavetable/CODE_REVIEW.md` at the canonical path
  - Update the PLUGINS.md row
  - **No tag and no publish.** That is a separate `/publish` (memory: never tag unless /publish).

## Constraints Identified

- **Regression guard:**
  - All 6 offline drivers + viz-check, `--alloc-check` 0, and R-GOLD must stay green.
  - Re-run the Stage 3 binary null test against `f4eea85a` (or the Stage 3 install) on the cases no Stage 4 fix touches.
  - A case a fix does touch must show the intended difference only.
- **Every DSP fix needs a gate with a negative control** (Stage 2 W3, W4; notes 3, 4). G-* goldens move only where a fix intends it, and that must be documented.
- **Preset-manager traps (suite memory):**
  - The stale `<CustomState>` child shadows the 2nd reopen. Verify that v1.0.9 contains that fix, or work around it.
  - `savePresetToFile()` has no factory guard. Keep overwrite protection on the factory presets.
  - A preset apply resets to defaults first.
  - An AsyncUpdater guard flag needs a cancel.
  - Bool params round-trip as STRING through ValueTree XML.
  - Preset names with "/" fail.
- **Imported bank × presets:** a user preset saved with `bank = Imported` cannot carry the audio, unless it does. That is an open question (below). Loading a preset must never clear or replace the session's `IMPORTED_BANK`.
- **Editor member order** stays relays → WebView → attachments. `getNativeFunction` comes from the `Juce` ES-module namespace, not `window.__JUCE__`.
- **UI gates stay green:** check-i18n, fr lint CLEAN, zh lint (new preset-panel strings at `mt`), check-ui-labels, and boot-all-uis `--strict-tips`.
  - The new strings and tooltips need en, fr and zh-Hans.
  - Pin the browser panel at its widest language (memory: language-width pins).
- **Validation:** auval (targeted `auval -v aumu OSiW OuDv`, not `-a`), and pluginval VST3 + AU at strictness 10 with the editor tests on.
- **Commits:** path-scoped to `plugins/O-simpleWavetable/**` + PLUGINS.md, using a temp-index commit (shared checkout; other sessions have O-simpleFM, O-Gain and others staged or modified).
- **The CI `workflow_dispatch` run is outward-facing.** Pushing `main` and dispatching need Taylor's explicit go-ahead at that step.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Preset scope | Full preset-manager module + browser panel; factory = Init + 8 §A9 | Suite norm (O-simpleFM, O-AnalogEQ); users can save their own experiments |
| Lesson buttons | Kept, reconciled to §A9 (Alias Demo → Drive bank); one shared recipe table | Removes the Stage 3 / §A9 split noted in ARCHITECTURE row 17 |
| Critic scope | All W (Stage 2 W3/W4 + Stage 3 W1–W5) + triaged N; by-design notes logged | Ship-quality fixes without churning accepted behaviour |
| Windows | CI build + pluginval via `workflow_dispatch` validate-only; no hands-on | Proves COMPAT-02 without a Windows machine and without publishing |
| QUAL-04 | Measured contrast gates + Taylor by-ear sign-off | A gate catches regressions; the ear confirms the teaching value |
| Version | 1.0.0, CHANGELOG, CODE_REVIEW; no tag | The release is a separate `/publish` |
| Cadence | Suggested: Part 1 = critic fixes + preset manager → build/install → preset-panel visual checkpoint; Part 2 = QUAL-04/PERF-02 gates, CI Windows, release docs, listening sign-off | Mirrors the Stage 2/3 checkpoint rhythm; the planner may adjust |

## Open Questions (for research)

1. **The preset-manager integration surface.**
   - Does v1.0.9 include the stale `<CustomState>` fix?
   - How does `getStateAsXml`/`setStateFromXml` coexist with the strip-on-load `IMPORTED_BANK` state and `uiLanguage`?
   - Which native functions are needed, given the 8 the editor already has?
2. **Imported bank in user presets:**
   - (a) store the bank choice only, and show the empty-Imported prompt if no import is present
   - (b) embed the flac16 bank in the preset JSON
   - (c) block saving while on Imported

   Recommend one, with its size and load cost.
3. **Browser panel placement** in the fixed 1120 × 780 Field Guide layout without crowding the viz panels: which existing region, and the widest-language width.
4. **W2 drop cap:** measure the decode time at 96 MB, then choose between lowering the cap (the importer keeps ≤ 524,288 samples, so the useful cap is small), pre-sizing, or a worker decode.
5. **Stage 2 W4 fix shape:** the upward-legato crossfade source read at the new mip level vs the old one. Gate it with an alias measurement on a 60→96 legato jump, with a negative control.
6. **QUAL-04 gate metrics and thresholds** for each contrast. A ratio verdict, not slack (DSP gate design index).
7. **The PERF-02 harness:**
   - offline 16-voice render timing vs the real-time budget at each rate, plus the pedalboard check on the installed binary
   - the pass threshold, e.g. < 25 % of one core at 96 kHz
8. The fix-or-log verdicts for Stage 3 N2, N3, N5, N8 and Stage 2 notes 5, 6, 7.

## Next Phase

Ready for: research phase
