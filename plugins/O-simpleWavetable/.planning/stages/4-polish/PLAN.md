# Stage 4 (Polish) — PLAN

**Plugin:** O-simpleWavetable · **Stage:** 4 of 4 (Polish) · **Target:** v1.0.0 (no tag, no publish) · **Date:** 2026-10-06
**Inputs:** `stages/4-polish/CONTEXT.md` (locked user decisions), `stages/4-polish/RESEARCH.md` (authoritative on *how*: synthesis, §A planner inputs A1–A15, §B planner inputs rows 1–10 + suggested order, §C planner inputs, cross-part reconciliation, suggested cadence), prototype evidence in `research-probes/{A,B,C}/` (DSP fix prototypes `B/proc.diff` + `B/w4.diff`, patched tree `B/src-fix/`, gate prototypes `B/fixprobe.cpp`, `B/w3repro.py`, `B/undef/`, `C/perf/perf.cpp`, `C/q4*.py`, `A/measure*.js`), `research/ARCHITECTURE.md` (row 17, §State Persistence, §A9, Amendments 1–18), `ROADMAP.md` §Stage 4, `REQUIREMENTS.md` (FUNC-08, PERF-02, COMPAT-02, QUAL-04), `parameter-spec.md` (LOCKED, 21 params), `stages/3-gui/{PLAN, SUMMARY, VERIFICATION}.md` (critic W1–W5, N1–N13; gate inventory), `stages/2-dsp/{SUMMARY, VERIFICATION}.md` (critic W3, W4, notes 1–8), `modules/persistence/preset-manager/` v1.0.9 (read-only), and the current `Source/`, `tests/`, `CMakeLists.txt`. Every `file:line` below was re-verified at plan time against `b71b7888`.
**Precedence when sources disagree:** user decisions (CONTEXT + PLAN-signoff below) > this plan's D-table > RESEARCH (planner-input tables > prose > prototype diffs) > ARCHITECTURE (+ amendments) > parameter-spec > ROADMAP > agent-template defaults.

---

## Decisions locked for this plan

### User decisions (CONTEXT 2026-10-06 + plan sign-off; locked, not re-asked)

| Ref | Decision | Effect |
|---|---|---|
| **CTX-presets** | FUNC-08 = the suite `preset-manager` module (v1.0.9) + a browser panel (list, ◀ ▶, Save, Delete) inside the fixed 1120 × 780 page. Factory bank = Init / Additive Build + the 8 §A9 recipes, raw → `convertTo0to1`. Presets **never set `output_level`**. `getNumPrograms() == 1` (presets live in the WebView only). | D-AA…D-AI; Tasks 6–11 |
| **CTX-lessons** | Lesson buttons stay, reconciled to §A9 (Alias Demo → Drive, Pos 100 %, Band-limit Off). Lesson ids and factory recipes come from **one** table. | D-AD; Task 6 |
| **CTX-critic** | Fix every W: Stage 2 W3, W4; Stage 3 W1–W5. Fix N: Stage 3 N1, N4, N7, N9–N13; Stage 2 notes 3, 4, 8. Accepted by design (log): Stage 3 N6; Stage 2 notes 1, 2. Research decides: Stage 3 N2, N3, N5, N8; Stage 2 notes 5, 6, 7. | D-AJ…D-AQ; Tasks 2–4, 6–8, 10, 19 |
| **CTX-perf** | PERF-02: 16 voices at 44.1 / 48 / 88.2 / 96 kHz on the Release build; report CPU per block as a fraction of the real-time budget. | D-AS; Tasks 17–18 |
| **CTX-windows** | COMPAT-02 is CI only: confirm `NEEDS_WEBVIEW2` + static linking + `withUserDataFolder()`, then a green Windows build + pluginval from `build-and-release.yml` `workflow_dispatch` validate-only (no release, no tag). No hands-on Windows DAW test. | D-AW; Task 22 |
| **CTX-qual04** | Three new measured contrast gates (stepped vs smooth, aliasing vs clean, bit depth), then Taylor's by-ear sign-off on the factory presets. | D-AR; Tasks 16, 23 |
| **CTX-release** | VERSION 1.0.0 in CMakeLists; CHANGELOG with a bracket-form `## [1.0.0]` heading; `plugins/O-simpleWavetable/CODE_REVIEW.md`; PLUGINS.md row. **No tag, no publish.** | D-AV; Tasks 19, 21 |
| **CTX-regression** | 6 offline drivers + viz-check, `--alloc-check` 0, R-GOLD green. Stage 3 binary null test on untouched cases; a touched case shows the intended difference only. **Every DSP fix (Stage 2 W3, W4; notes 3, 4) has a gate with a negative control.** G-* goldens move only where a fix intends it, documented. | D-AT, D-AU; Tasks 1, 5, 9, 12, 13, 18, 20 |
| **CTX-cadence** | Part 1 = critic fixes + preset manager → build/install → visual checkpoint. Part 2 = QUAL-04 / PERF-02, CI Windows, release docs, listening sign-off. | Part 1 = Tasks 1–14 (execute stops at 14). Part 2 = Tasks 15–24. |
| **PLAN-signoff 1** (Taylor, 2026-10-06, plan time) | **Drive Sweep:** keep §A9's recipe (menv A 0.01 / D 1.5 / S 0); rewrite the en / fr / zh-Hans copy to match (`lessonDriveSweep` tip body + `tour.caption.driveSweep`). zh at `reviewed: 'mt'`; fr lint must pass. | Task 11; Task 23 listens for it |
| **PLAN-signoff 2** | **Continuous-knob wheel (W3b): FIX** with the same px accumulator as the stepped-knob W3 fix (`WHEEL_PX_PER_NUDGE`). **Gate it.** | D-AO; Task 10; G-S3W3-WHEEL |
| **PLAN-signoff 3** | **Keyboard at 37 px white keys** (preset panel 352 px in the keyboard row): adopted; Taylor confirms at the Part 1 visual checkpoint. | D-AI; Task 14 |
| **PLAN-signoff 4** | **COMPAT-02 push + `workflow_dispatch` validate-only = a BLOCKING explicit go-ahead checkpoint in Part 2.** The executor lists the RESEARCH facts and stops. It never pushes or dispatches on its own. | Task 22 |
| **PLAN-signoff 5** | **The listening sign-off adds the Mono legato octave jump** (C4 → C7 on Drive; RESEARCH decision 5). | Task 23 |

### Planner resolutions (RESEARCH recommendations adopted; no user input needed)

Continues the plugin-wide series after Stage 3's D-P…D-Z.

| ID | Resolution |
|----|-----------|
| **D-AA** | **Preset-manager surface (OQ1).** `OuariconPresetManager` v1.0.9 is used ONLY for: the Factory / User folders (`~/Library/O-simpleWavetable/Presets`, literal name, shared by dev and release builds), `savePreset(name)` (factory guard + `/\:` sanitize), `deletePreset(name)` (factory guard), `initializeFactoryPresets` with its `.factory-version` sentinel, and `preset-manager.js`. The plugin **never** calls the module's XML state pair, its JSON load/apply path, its file-dialog save/load, its custom-state callbacks or its prev/next: v1.0.9 still carries the stale `<CustomState>` child (`OuariconPresetManager.h:617-644`), and its apply resets and sets `output_level` (`:360-376`). The plugin keeps its own `getStateInformation` / `setStateInformation`. The module defects are logged as upstream in CODE_REVIEW.md, not fixed (`modules/` is outside this stage's commit scope). |
| **D-AB** | **One apply core** (RESEARCH A §1): `bool applyNormalisedTargets (const std::array<float, 21>& targets)`, message thread. (1) `pendingAutoSelect = false` under `bankStateLock` — the **Stage 3 N5 fix**; (2) one pass over `ParamIDs::all`, `output_level` skipped; (3) target = `jlimit(0, 1, t)` if finite, else the parameter's default (defaults first); (4) skip if within 1e-6; (5) begin → `setValueNotifyingHost` → end per changed parameter. Three callers: recipe → targets (`convertTo0to1(raw)` per listed entry, NaN for the rest); user JSON → targets (only numeric / bool values under the file's `parameters` object for known ids; `output_level` and unknown keys ignored even when present; a non-object → `false`, no change; ASVS V5); factory name → recipe (never the disk JSON). The applied targets are cached for the modified check (D-AG). |
| **D-AC** | **Imported × presets = option (a), bank choice only (OQ2).** A user preset stores `bank` as a normal parameter (Imported = normalized 1.0). The apply never touches `importedOwner`, `cachedBlob`, `importStatus` or the `IMPORTED_BANK` child; its only `bankStateLock` write is `pendingAutoSelect = false`. With nothing imported the Stage 3 empty-Imported prompt shows. The Save tooltip says the audio is not saved. No `customState`. Embedding (b) costs 0.24–1.31 MB of base64 per preset (measured) and would replace the session bank. **Supersedes** ARCHITECTURE §State Persistence's sentence "a user preset with an imported bank must replace, not merge, the IMPORTED_BANK child" (Amendment 19, Task 19). |
| **D-AD** | **One recipe table: `Source/PresetRecipes.h`** (header-only, C++17, ASCII), transcribed from RESEARCH A §4: `namespace wtpresets { Entry, Recipe, make<N>, kInit … kPpg4, kFactory[9] }`, raw engineering units. **Fill rule:** §A9 wins where it names a value; the Stage 3 lesson recipe fills where §A9 is silent; the 4-bit PPG variant is the 9th entry. Ids → file names: `init` "Init - Additive Build", `steppedSmooth` "Stepped Scan", `smoothScan` "Smooth Scan", `aliasDemo` "Alias Demo", `driveSweep` "Drive Sweep", `vowelPad` "Vowel Pad", `pulseNarrowing` "Pulse Narrowing", `ppg8bit` "8-bit PPG", `ppg4bit` "4-bit PPG". The 5 lesson `data-preset` ids are a subset (contract kept). **The table is the sound authority**: lesson clicks, factory loads, `buildFactoryPresetDefs()`, `getPresetCatalog` and the modified check read it; disk copies are never read back. `kInit` == the APVTS defaults, so a fresh instance reports "Init - Additive Build", unmodified (name set in the ctor). Changes vs the Stage 3 lessons (Task 23 listens for each): steppedSmooth Tri → Saw, 0.18 → 0.25 Hz; aliasDemo Sine→Saw → Drive; driveSweep A 0.9 → 0.01, D 1.6 → 1.5, S 0.25 → 0; vowelPad Sine → Tri, 0.12 → 0.1 Hz, depth 0.9 → 1.0; ppg8bit 3 → 4 Hz. |
| **D-AE** | **Walk order and names.** `getPresetWalkOrder()` = the 9 factory names in table order, then `User/*.json` stems sorted case-insensitively, minus any that match a factory name (case-insensitive). The list, prev / next and the page select all use this one order (memory `pattern_grouping_preset_dropdown_breaks_prev_next`). An unnamed or missing current name enters at the top going forward and at the bottom going back. `sanitisePresetName` (save and restore): trim → remove C0, DEL, C1 (U+0080–009F), bidi controls (U+200E/F, U+202A–202E, U+2066–2069) → `juce::File::createLegalFileName` → strip leading dots → cap 64 chars → empty = refused. Never reuse `WavetableImporter::sanitiseName` (it never returns empty). C++ refuses a factory name (case-insensitive) **before** calling the module; the page also refuses any factory **display** label in any language. Unnamed (`""`) displays as a raw `—` option (`setRaw`, no i18n key). |
| **D-AF** | **State.** `getStateInformation` sets root property `currentPreset` (= `getPresetName()`) next to the `uiLanguage` line (`PluginProcessor.cpp:747-748`), before `writeImportedBank`. `setStateInformation` reads it from the incoming tree before `replaceState` (beside `:768-770`): present → `sanitisePresetName`; **absent → `""`** (an old session is unnamed, never the ctor's Init name). A restored factory name rebuilds its targets from the table; a restored user name marks the targets stale and re-reads that file on the first editor tick (missing file → modified = false). Name, targets and their validity live under a new `juce::CriticalSection presetLock`; `std::atomic<juce::uint32> presetRevision` bumps on every name / target change. `presetLock` is never taken on the audio thread and never held across `setValueNotifyingHost`. No call to the module's XML state pair (D-AA). |
| **D-AG** | **`presetState` push (N10).** New event `presetState { name, id, factory, modified }` (`id` = table id or `""`). `isPresetModified()` = any parameter except `output_level` differs from the cached targets by > 1e-4; unnamed → false. The editor timer emits when `getPresetRevision()` changes or `modified` flips; `uiReady` forces it. Order every tick: `bankUpdate` → `importStatus` → `presetState` → `cycleUpdate`. The page lights `.tour-btn[data-preset=id]` iff `!modified`, shows the panel dot iff `modified`, and sets the caption from `CAPTION_LABELERS[id]` (else `tour.hint`). `LESSON_OCTAVE` (`index.html:1155`) stays click-driven (a lesson click or a browser load), never a restore. |
| **D-AH** | **Natives 8 → 20, events 3 → 4.** Adds the 10 names `preset-manager.js` resolves (`savePreset`, `savePresetWithDialog`, `loadPreset`, `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`) plus `getPresetCatalog` and `stepKnobDrag`. `savePresetWithDialog` / `loadPresetFromFile` are refusal stubs (`complete({success:false, name:""})`): no promise hangs and the bridge stays exact. `selectNextPreset` / `selectPreviousPreset` **return the neighbour name without loading it** (the module JS then calls `loadPreset(name)`, `preset-manager.js` `selectNext()`). Every preset native first calls `ensureFactoryBankOnDisk()` (lazy, message thread, once per instance) — never the processor ctor, so auval, pluginval and the console gates do no file I/O. Gates test pure functions only (`recipeTargets`, `buildFactoryPresetDefs()`, `applyUserPresetJson(var)`) and never touch `~/Library` (memory `pattern_factory_bank_sentinel_makes_preset_gates_read_stale_bank`). |
| **D-AI** | **Panel geometry and page rules (OQ3; RESEARCH A §3).** Keyboard row: `[kbd-side 150] 12 [keyboard 556] 12 [.preset-panel 352]` = 1082 (white keys 61 → 37 px; black keys stay 22). Panel = column of two rows: row 1 (h 20) `PRESETS` label (88) … Save (74) 6 Delete (72); row 2 (h 26) ◀ (22) 6 `select#preset-select` (296; reuse `select.combo` at 13 px, explicit width) 6 ▶ (22). Popover (Save / Replace? / Delete?) absolute above the panel, fixed 352 × 92, z-index 40, message line `nowrap` 12 px italic `--warn-text`; capture-phase `pointerdown` outside + Escape dismiss (the `initSettingsPopover` pattern, `index.html:2446-2475`); Enter submits. Modified marker = a 6 px brass dot via `.preset-panel.modified::before` (no text). `preset-manager.js` is imported **dynamically** inside `initPresets(Juce)`; failure disables the panel (a static import 404 blanks the page). Construct `new PresetManager({ getNativeFunction: Juce.getNativeFunction, prevButton, nextButton, onPresetChanged, onPresetListUpdated })`; never pass `displayElement`, `saveButton`, `loadButton` or `deleteButton` (the module's `promptDelete` deletes its own cached `currentPreset`, which goes stale after a lesson click or restore — Delete is page-owned and keyed on `presetState.name`). Factory options use a literal `FACTORY_LABELERS` id → `setLabel(opt, '<key>')` map, one `id:` per line; user options use `setRaw`; a disabled `────` separator sits between the groups; no optgroups. |
| **D-AJ** | **Stage 2 DSP fixes = the prototypes** (`B/proc.diff`, `B/w4.diff`, measured in `B/fixprobe.out`, `B/w4probe.out`), with three named deviations: (1) note 4's `prepared` is **not** cleared in `releaseResources` (`proc.diff` clears it; RESEARCH B §3 overrides: a host that processes after release must not go silent); (2) W4's `curOffset == 0.0` becomes `juce::exactlyEqual (curOffset, 0.0)` (`-Wfloat-equal`); (3) every new hook is set only under `#if OSIW_TEST_HOOKS` (`nm` on shipped binaries = 0 `ForTesting`). New hooks: `setPreparedGuardForTesting`, `setMidiCapacityGuardForTesting`, `setMonoWheelSeedForTesting`, `setXfadeKeepRateForTesting`, `setReleaseClearsDisplayForTesting`. |
| **D-AK** | **Stage 3 W2 = lower the drop cap to 16 MiB (OQ4).** `WavetableImporter::kMaxMemoryBytes` (`WavetableImporter.h:89`) and the page `DROP_MAX_BYTES` (`index.html:1144`) both become 16 MiB; the `:89` comment keeps naming the page constant. Measured message-thread C++ cost ≈ 170 ms (vs ≈ 1.03 s at 96 MiB); a full 256-frame mono / stereo source needs ≤ 4 MiB; the Import button streams from disk and has no cap. No pre-size (measured no gain), no worker decode (keeps G-DROP[c]'s synchronous contract). `import.err.tooLarge` copy → "too large to drop — use the Import button" (3 languages, ≤ 240 px). Page mapping fix (RESEARCH B §4): when `importBytesFn` returns false, keep a specific error the processor already reported for that drop; fall back to `generic` only when none arrived (`index.html:2422`). |
| **D-AL** | **Stage 3 W4 = processor-side status clear.** Public `pollBankForImportStatus()` (message thread), called from the processor's 250 ms `timerCallback` (`PluginProcessor.cpp:952-956`) before the sweep: if the bank index ≠ `lastStatusBank`, store it, and if `importStatus.state == error` set `importStatus = {}` and `++importStatusVersion` (under `bankStateLock`). `std::atomic<int> lastStatusBank` is **seeded** in the ctor and at the end of `setStateInformation` (planner refinement: unseeded, the first poll would clear a restore's `unsupported` notice and void the control arm). Works with the editor closed; the passthrough blob is untouched. |
| **D-AM** | **Stage 3 W5 = the processor owns UI-held notes.** `std::array<bool, 128> uiHeld` (message thread). `handleUiMidi` (`:709-724`) sets an entry on note-on with velocity > 0 and clears it otherwise. `releaseUiHeldNotes()` queues a note-off per held note and clears all. The editor dtor calls it before `stopTimer()`. Belt and braces: the page calls `allNotesOff()` on `visibilitychange` → hidden. |
| **D-AN** | **Stage 3 W1 = snapshot first.** `uiReady` assigns `lastBankGeneration`, `lastBankIndex`, `lastImportVersion`, `lastPresetRevision` and `lastPresetModified` **first**, then `emitBankUpdate (true)`, `emitImportStatus()`, `emitPresetState (true)`, `forceCycleEmit = true` (the `timerCallback` order). A transition inside the window re-sends next tick: a duplicate, never a loss. |
| **D-AO** | **Stage 3 W3 + W3b = one burst-aware wheel accumulator** shared by all three handlers: stepped knobs (`index.html:1477-1490`), the bank stack (`:2349-2360`) and continuous knobs (`:1344-1356`; PLAN-signoff 2). Rules (RESEARCH A §5): a discrete event (`deltaMode !== 0`, or the first event of a burst = no wheel event in the last `WHEEL_GESTURE_MS`) steps exactly once; later pixel events add `dir·px` to `acc`; a direction reversal zeroes `acc`; step `trunc(acc / WHEEL_PX_PER_NUDGE)` clamped to `WHEEL_MAX_NUDGES`, keep the remainder; `acc` resets 250 ms after the last event. The bank stack steps `1/(N−1)` per step; continuous knobs keep their existing nudge size. Constants unchanged (`:1130-1134`: 100 px, 4, 250 ms). Gate: the G-S3W3-WHEEL Playwright probe, with the shipped Stage 3 page as its negative control (R-WHEEL). |
| **D-AP** | **Small UI critic fixes.** **N13:** the gesture bracket lives in the **processor** — `bool stepKnobGesture (const juce::String& id, int phase, int index)` (`id` ∈ {`bit_depth`, `lfo_div`}; begin opens once; move sets `convertTo0to1 ((float) jlimit (0, n−1, index))` if it differs; end closes once) + `closeStepKnobGestures()` — so viz-check can gate it; the `stepKnobDrag` native is a one-liner, and the editor dtor closes open gestures. (RESEARCH put the bracket in the editor TU, which no harness compiles; behaviour is identical.) Page: pointerdown → begin, move → a local `dragIdx` for the visual + move, up / cancel / lost capture → end; arrows, wheel and dblclick keep `setChoiceIndex` (a discrete step is one gesture). **N8:** page `gestureBegin(id)` / `gestureEnd(id)` per-id refcount (`sliderDragStarted` only on 0→1, `sliderDragEnded` only on 1→0), used by `bindKnob`, `nudge`, `resetToDefault` and `bindBankDrag`. **N1:** editor member `std::unique_ptr<juce::FileChooser> importChooser` + `bool importDialogInFlight` (O-AnalogEQ `PluginEditor.h:112-132`, `.cpp:107-260`); the callback clears only the flag, first. **N9:** `renderImportError` gains the `unsupported` branch. **N11:** TIP_BINDINGS `#octDown`, `#octUp`. **N12:** page-only `'−inf'` (U+2212); EXEMPT text and the three "(−inf)" tip bodies follow; the C++ host text `"-inf"` and its parser (`PluginProcessor.cpp:81-98`, state-check P0) stay ASCII. |
| **D-AQ** | **Fix-or-log verdicts (OQ8; RESEARCH synthesis row 10, B §5).** **Fix:** Stage 3 N5 (D-AB), N8 (D-AP). **Reviewed, not changed (log):** Stage 3 N2 (one-tick torn display atomics), N3 (Amendment 18 single-scope read), Stage 2 note 5 (Mono ignores CC64; plus the held-pedal Mono → Poly side note), note 6 (no import cancel; a full job ≈ 17–24 ms), note 7 (premise false: decoding runs before the lock). **Accepted by design:** Stage 3 N6; Stage 2 notes 1, 2. Also logged: the W4 fold residual and the note-3-adjacent host-owned collector buffer. Wording = RESEARCH B §5 draft. |
| **D-AR** | **QUAL-04 gates live in `tests/dsp-check` (OQ6; RESEARCH C §1)** and take their recipes through `applyFactoryPreset(id)`. `G-Q4-STEP`: A4 @ 56 320 Hz, Stepped vs Smooth Scan, cycle-difference energy over t ∈ [1.0, 3.8] s, **R ≥ 10** (measured 54.3 model / 53.9 installed). `G-Q4-ALIAS`: Alias Demo, MIDI 84 / 96 / 108 × 44.1 / 48 / 88.2 / 96 kHz × Band-limit Off / On; gated cells C7 + C8 at every rate and C6 at 44.1 / 48; **A_8k(Off) ≥ −50 dB**, **contrast ≥ 40 dB**, **A_full(On) ≤ −90 dB** (measured worst −41.3 / min 69.1 / −107.3). `G-Q4-BITS`: Sine→Saw frame 32, A4 48 kHz, SNR vs Full; steps in **[4.5, 7.5] dB** for 16..4, LS slope **6.02 ± 0.6 dB/bit** (measured 6.029), SNR(3) ≤ 25, SNR(16) ≥ 85. All ratio verdicts, each with negative controls. |
| **D-AS** | **PERF-02 harness (OQ7; RESEARCH C §2) = new console `tests/perf-check`**, **without** `OSIW_TEST_HOOKS`, built in an out-of-repo **Release** tree. Thread-CPU per block × fs / bs on the worst-case patch (16 voices MIDI 84..99 vel 100, Interp On, BL On, 3-bit, LFO Saw 5 Hz depth 1, Env +1, menv S 0.5, amp S 1); rates 44.1 / 48 / 88.2 / 96 kHz × bs 64 / 512 × steady / crossfade-once-per-block; 1 s warm-up, 4 s measured, best-of-3 median + p99 + max. Gates `G-PERF02-LIVE`, `-SCALE`, `-STEADY` (median **and** p99 ≤ 25 %), `-XFADE` (bs 512 median ≤ 25 %); bs ≤ 64 storms printed only. Duty witness (user / wall ≥ 80 %) else `SKIPPED` + exit 77, never a pass. Expected 0.75 % steady / 1.97 % crossfade at 96 kHz. pedalboard cross-checks the steady case only (each `raw_value` set costs ≈ 1.5 ms). |
| **D-AT** | **Null baseline = a snapshot of the installed Stage 3 VST3**, taken in Task 1 before any Stage 4 install (byte-identical to the `f4eea85a` artefacts, Stage 3 VERIFICATION). Fallback only if the snapshot check fails: RESEARCH B §6 `git archive f4eea85a` Release build. Expected: `default_chord`, `bank_saw_pos1_blOff`, `lfo_tri_interp`, `bits3_noInterp`, `steal17` max abs diff **0.0**; `mono_legato` non-zero **only at samples 28801–29039** (W4: 65 → 67 crosses L4 → L5 at 48 kHz); positive controls `w3_poly_to_mono` (baseline ≈ +200 c → Stage 4 0 c) and `legato_60_96_drive` (diff confined to t+1 … t+239); Stage 4 rendered twice = bit-identical. Part 2 adds a second snapshot (the installed Part 1 binary, Task 15): Part 2 makes no DSP change, so Part 2 vs Part 1 = bit-identical on every case. |
| **D-AU** | **R-GOLD covers 7 logs (adds viz-check) with a closed list of named diffs** (Recipes): bank `G-TIME` buildMillis; import soak block count + its `G-FINITE` count; dsp `G-BLOCK` `peak` detail (W4: its Mono arm 60 → 64 → 67 crosses L4 → L5, `dsp-check/main.cpp:1317-1318`); dsp-alloc `G-ALLOC` scenario counts (flood blocks added; allocations stay 0); viz `G-LESSON[...]` (re-transcribed to §A9, 5 → 9 ids) and `G-DROP[b]` char count (134,217,733 → 22,369,625); `G-FINITE` sample counts in drivers that gain gates. New Stage 4 gate lines are filtered by `S4NEW` and must each PASS. Anything else = regression → stop. |
| **D-AV** | **Release docs.** `CMakeLists.txt:14` `VERSION "1.0.0"` (never `PLUGIN_VERSION`). New `CHANGELOG.md` with exactly one `## [1.0.0] - YYYY-MM-DD` heading; Stage 1–4 history as `###` subsections (CI extractor `build-and-release.yml:723-737`). New `CODE_REVIEW.md` at `plugins/O-simpleWavetable/CODE_REVIEW.md` in the suite format (frontmatter as `O-simpleAdditive/CODE_REVIEW.md:1-20`; resolution log as `O-Prism/CODE_REVIEW.md:30-43`). IDs: WR-01…WR-10 = Stage 2 W1–W5, Stage 3 W1–W5; IN-01…IN-21 = Stage 2 notes 1–8, Stage 3 N1–N13; module defects under "Upstream (not plugin code)". PLUGINS.md row → `📦 Installed | 1.0.0`. No tag; the later `/publish` tag is `O-simpleWavetable-v1.0.0` (CI glob `'*-v*'`; CLAUDE.md's `vX.Y.Z-<Plugin>` wording loses to CI). |
| **D-AW** | **COMPAT-02 run protocol.** The config is already complete (`CMakeLists.txt:20`, `:103-109`; `PluginEditor.cpp:301-310`; 0 MSVC trap carriers); the MSVC greps re-run after Part 1. Push + dispatch happen **only** after Taylor's explicit "yes" at Task 22, with `version=1.0.0-validate`. Pass: build-windows ✓, `pluginval-windows-log` SUCCESS / 0 FAILED, create-release skipped, build-macos not red. A "no" leaves COMPAT-02 at "config verified, CI run not authorized" for `/plugin-verify` to judge. |

### Already settled (do not re-ask)

- **Parameters:** 21 (ROADMAP's "22" is the documented slip); ids, ranges and skews LOCKED. `getNumPrograms() == 1`.
- **Editor contract:** member order relays → WebView → attachments; 13 slider / 6 combo / 2 toggle relays; natives via the `Juce` ES-module namespace (`index.html:1057`); `window.__JUCE__` only for the event backend (`:2274`); `setSize (1120, 780)` literal, not resizable; the bank `<select>` stays English.
- **Every Stage 3 pin holds** (`.title-block` 384, `.import-btn` 124, `.bank-readout` 300, `.tour-buttons` 458, `.src-meta` content 240 …). The keyboard is flex, not pinned.
- Presets and lessons never set `output_level`. Empty Imported = silence + prompt.
- D-U kept (harmonic dB reference = bins 1..1023).
- VERSION changes only at Task 19; Logic re-reads the AU I/O on a version change (harmless, memory `critical_logic_caches_au_io_config_per_version`).
- **O-simpleFM is not a reference** (another session's in-flight edits; its state routes through the module's XML path and its factory table sets the output level). Only its CMake ordering idea (module before `juce_add_binary_data`) is reused. O-AnalogEQ is the clean reference for the FileChooser member and the served `presetmanager_js`.
- No tag, no publish, no hands-on Windows.

### Source contradictions resolved at plan time

| # | Sources | Conflict | Resolution |
|---|---|---|---|
| 1 | ARCHITECTURE §State Persistence vs CONTEXT + RESEARCH A §2 | "A user preset with an imported bank must replace, not merge, `IMPORTED_BANK`" vs "loading a preset must never clear or replace the session's `IMPORTED_BANK`" | CONTEXT wins → D-AC; ARCHITECTURE Amendment 19 (Task 19) |
| 2 | `B/proc.diff` vs RESEARCH B §3 prose | The prototype clears `prepared` in `releaseResources`; the prose says do not | Prose wins → D-AJ(1); G-UNPREPARED arm (c) proves it |
| 3 | `B/w4.diff` vs the codebase rule | `curOffset == 0.0` vs `-Wfloat-equal` | `juce::exactlyEqual` → D-AJ(2) (RESEARCH B §6 agrees) |
| 4 | RESEARCH A §1 code vs prose | Restore code `if (! cp.isVoid())` keeps the current name; the prose says absent → `""` | Absent → `""` (D-AF) |
| 5 | RESEARCH A §5 W4 | `lastStatusBank` unseeded: the first poll clears a restored `unsupported` notice, and the control arm "no bank change → error persists" fails | Seed in the ctor + at the end of `setStateInformation` (D-AL) |
| 6 | RESEARCH A §5 N13 vs CTX-regression | Bracket in the editor TU, which no harness compiles → no automated gate | Bracket in the processor, native one-liner (D-AP) → G-S3N13 |
| 7 | RESEARCH A §1 natives table | "Call `presetManager.ensure…`" — the module has no such method | The processor's `ensureFactoryBankOnDisk()` (D-AH) |
| 8 | RESEARCH A §1 natives table vs `preset-manager.js:204-235` | "selectNext = neighbour of the current name" is ambiguous; the module JS loads the name the native returns | The native returns the neighbour and does not load (D-AH) |
| 9 | RESEARCH A §1 (pass `deleteButton` + `onConfirmDelete`) vs `preset-manager.js:352-380` | The module deletes its own cached `currentPreset`, which goes stale after a lesson click or restore | Page-owned Delete keyed on `presetState.name` (D-AI) |
| 10 | RESEARCH C §5 / B §6 R-GOLD lists | Named only G-BLOCK + G-DROP[b]; the new gates also move `G-FINITE` sample counts, G-LESSON changes, and Stage 3's R-GOLD covered only the 6 Stage 2 logs | Closed allowlist over 7 logs (D-AU) |
| 11 | PLAN-signoff 2 vs RESEARCH A §5 | RESEARCH's W3 / W3b proof was hands-on only; Taylor: "Gate it" | Playwright probe G-S3W3-WHEEL + Stage-3-page negative control (D-AO, R-WHEEL) |
| 12 | RESEARCH anchors | `LESSON_OCTAVE` cited `:1156` (actual `:1155`), `tour.caption.driveSweep` `:567` (actual `:566`), the "-inf" text helper `:83-97` (actual `:81-98`) | This plan uses the verified anchors |
| 13 | CLAUDE.md tag wording vs CI | `vX.Y.Z-<Plugin>` vs the `'*-v*'` glob | No tag this stage; the `/publish` tag is `O-simpleWavetable-v1.0.0` (D-AV) |
| 14 | Session boundary (implicit in RESEARCH C §5 row 0) | Part 2 runs in a new session; Task 1's `$SCRATCH` baselines may be gone | Task 13 records the Part 1 scratch path in STATUS; Task 15 re-baselines from the Part 1 tree and snapshots the Part 1 binary (D-AT) |

---

## Goal

Ship O-simpleWavetable v1.0.0:
- **FUNC-08:** a full preset manager inside the Field Guide page — 9 factory presets from one §A9 table (each isolating one concept), user presets (save, replace, delete, ◀ ▶), a truthful name / modified / lesson display — that never sets `output_level`, never touches the imported bank, and never uses the module's broken state path.
- **Critic fixes:** every Stage 2 / Stage 3 warning and the triaged notes, each DSP fix behind a gate with a negative control; everything else logged in CODE_REVIEW.md.
- **QUAL-04:** three measured contrast gates, then Taylor's ears. **PERF-02:** measured on Release at four rates. **COMPAT-02:** a green Windows CI build + pluginval, only with Taylor's go-ahead.
- **Release:** VERSION 1.0.0, CHANGELOG, CODE_REVIEW, PLUGINS row. No tag.
- **Regression:** the Stage 3 binary renders bit-identically wherever no fix intends a change.

---

## Execution model

**Part 1** (Tasks 1–14) ends in its own commit and a **blocking visual checkpoint**. **Part 2** (Tasks 15–24) ends with the CI go-ahead, the listening sign-off, docs and a commit (CTX-cadence).

| Who | Tools | Tasks |
|-----|-------|-------|
| **dsp-agent**, two dispatches:<br>- **A** (Part 1: Stage 2 notes 8, 4, 3, W3, W4; Stage 3 N4, N7, W2 cap + every gate for them)<br>- **E** (Part 2: QUAL-04 gates, `perf-check`) | Read / Write / Edit. **No Bash, no builds** (dsp-agent contract). | 2–4 (A), 16–17 (E) |
| **gui-agent**, three dispatches:<br>- **B** (Part 1 C++: recipe table, apply core, module, state, natives, C++ critic fixes + viz-check / state-check gates)<br>- **C** (Part 1 page: panel, page critic fixes, i18n, gate fixtures, wheel probe)<br>- **D** (only for real findings in Tasks 12 / 20) | Read / Write / Edit. Bash only for `cp` / `cmp` / `grep` / `git show` and, in C / D, the R-UISTATIC lints + `research-probes/A/measure*.js` width probe. **No builds, no Playwright gates** (the orchestrator runs those). | 6–8 (B), 10–11 (C), fixes (D) |
| **Orchestrator** (plugin-workflow execute) | Bash. Every long command (`cmake --build`, build-and-install, auval, pluginval, Playwright gates, pedalboard renders, `gh run watch`) runs with `run_in_background: true`, logging to `$SCRATCH` = this execute session's scratchpad (600 s watchdog, memory `pattern_executor_watchdog_stall_run_long_commands_in_background`). | 1, 5, 9, 12, 13, 15, 18–22, 24 |
| **Taylor** (human checkpoints) | Standalone, Logic, the CI decision | 14, 22, 23 |

`polish-agent` is not dispatched: Stage 4's work splits cleanly by file owner.

**File-ownership sequencing** (RESEARCH cross-part reconciliation): `PluginProcessor.cpp` is edited by dispatch A (notes 3 / 4, W3, N4) and then dispatch B (apply core + N5, state, W4 poll, W5 `uiHeld`, preset API). The dispatches are strictly sequential, and B starts only after Task 5 is green. `index.html`: A changes one constant (`:1144`), then C owns it. `CMakeLists.txt`: A (`TestHooks.h`), B (`PresetRecipes.h`, module, binary data), E (`perf-check`). `tests/viz-check/main.cpp`: A, then B. `tests/dsp-check/main.cpp`: A, then E. **No two dispatches ever run concurrently.**

**Tracers** (each proven on the real build before the next layer lands):
- **DSP:** Task 5 proves TestHooks + the processor block path + the voice fix through all 7 drivers, R-GOLD and R-UNDEF.
- **Preset architecture:** Task 9 proves module CMake → configure-time JS copy → `BinaryData::presetmanager_js` compiled into the editor → recipe table → apply core → state round trip → natives compiled, all offline-gated, before any page code. Dispatch C then expands from that proven slice.

**Execute-phase stop rules:**
- **Part 1:** `/plugin-execute O-simpleWavetable 4-polish` runs Tasks 1–13, presents Task 14 and **STOPS**. STATUS = `stage_4_part1_complete_visual_pending`. After Taylor's sign-off, a second `/plugin-execute O-simpleWavetable 4-polish` reads STATUS and resumes at Task 15. A checkpoint finding → fix first (the owning dispatch with Taylor's notes), re-run Tasks 12–13 (a path-scoped fix commit), repeat Task 14.
- **Part 2:** Tasks 22 and 23 are presented together after Task 21; the orchestrator waits in-session. **Nothing is pushed or dispatched without Taylor's explicit "yes" in Task 22.** If the session must end first, STATUS = `stage_4_part2_signoff_pending`; the next `/plugin-execute O-simpleWavetable 4-polish` resumes at result intake.

**Agent overrides** (apply to every dispatch; supersede the agent templates):

*Sources and transcription*
- **Read:** CONTEXT, the RESEARCH sections named in the task, this PLAN, parameter-spec, and `troubleshooting/patterns/juce8-critical-patterns.md`. Where RESEARCH or a prototype diff gives code, **transcribe it as hunks** with this plan's D-table deviations. `research-probes/B/src-fix/` is reference only: never copy whole files from it.
- **ASCII-only C++** (comments included; Stage 1 gate). Test strings with non-ASCII code points are built from hex, e.g. `juce::String::charToString ((juce::juce_wchar) 0x202E)` (memory `critical_juce_string_char_ctor_is_ascii_only`). Lone surrogates and values > U+10FFFF are tested through the predicate, never through a `juce::String` (Debug UTF-8 validity assertions would trip R-RUN's 0-assertion rule). `index.html` / `i18n.js` / `*.json` are UTF-8 and exempt.
- **Comment discipline:** comments never name the module functions the static gates forbid (R-STATIC S10) — write "the module's XML state path" instead — and never write the quoted output-level id inside `PresetRecipes.h`.

*Compiler rules*
- `-Wfloat-equal`: `juce::exactlyEqual`, never `==` on floats or doubles.
- `-Wshadow-all`: no member, local or parameter shadows another (e.g. the preset-name member vs a `name` parameter: use distinct names).
- Every header that tests `OSIW_TEST_HOOKS` includes `TestHooks.h` first (R-UNDEF).
- Choice / bool parameters resolve through `choiceIndex` / `finiteOr` (`PluginProcessor.cpp:40-52`).

*Build wiring*
- Every new header goes in `target_sources`. `BinaryData.h` is included **only** in `PluginEditor.cpp`.
- Test hooks compile only under `#if OSIW_TEST_HOOKS`. `perf-check` never gets `OSIW_TEST_HOOKS`.

*Thread safety*
- **Audio thread:** no allocation, lock, file I/O or new branch in `processBlock` / `renderBlock` / `renderMono` / `WtVoice::renderNextBlock` beyond exactly what Tasks 2–3 specify.
- `presetLock` is never taken on the audio thread and never held across `setValueNotifyingHost`. Module calls happen on the message thread only.
- Imported bank: only via `getImportedBankSnapshot()` or one `bankStateLock` scope (Amendment 12).

*Files the agents must not touch*
- STATUS.md, PLUGINS.md, ARCHITECTURE, parameter-spec, REQUIREMENTS, ROADMAP, CHANGELOG, CODE_REVIEW (orchestrator-owned). Reports say `"stateUpdated": false`.
- `modules/**`, `Source/ui/public/modules/**` (configure-generated, gitignored), `mockups/`, other plugins, the root `CMakeLists.txt`, `.gitignore`, `build/CMakeCache.txt`.

*Contract invariants*
- `setSize (1120, 780)` literal; `if (url == "/js/i18n.js")` form; `int getNumPrograms() override { return 1; }`; 21 relays + 21 attachments unchanged.
- Presets and lessons never set `output_level`; a preset load never touches the imported bank.

---

## Recipes (referenced by the tasks)

All commands run from the repo root unless noted; plugin-relative paths are under `plugins/O-simpleWavetable/`. Long steps run in the background and are polled.

**R-DBG: out-of-repo Debug harness build.** Never set `OUARICON_BUILD_TESTS` / `SKIP_PLUGINS` in the shared `build/` (its cache: `OUARICON_BUILD_TESTS:BOOL=OFF`).
```bash
bash -c 'SKIP=$(for d in plugins/*/; do n=$(basename "$d"); [ -f "${d}CMakeLists.txt" ] && [ "$n" != O-simpleWavetable ] && printf "%s;" "$n"; done); \
  cmake -S . -B "$SCRATCH/build-oswt" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOUARICON_BUILD_TESTS=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "-DSKIP_PLUGINS=$SKIP"' > "$SCRATCH/cfg.log" 2>&1
cmake --build "$SCRATCH/build-oswt" --target O-simpleWavetable-bank-check O-simpleWavetable-dsp-check \
  O-simpleWavetable-mod-check O-simpleWavetable-import-check O-simpleWavetable-state-check \
  O-simpleWavetable-viz-check O-simpleWavetable-param-dump > "$SCRATCH/build-dbg.log" 2>&1
```
- Each console target depends on the plugin's shared-code target, so this also compiles `PluginEditor.cpp` (it proves every `BinaryData::` symbol the editor names exists).
- The configure also copies the module JS (`webview-drop-streaming.js`, and from Task 7 `preset-manager.js`) into `Source/ui/public/modules/` (gitignored).
- **Accept:** exit 0; `grep -c 'plugins/O-simpleWavetable/.*warning:' "$SCRATCH/build-dbg.log"` = 0. Record (not gating; logged upstream) `grep -c 'modules/persistence/preset-manager/.*warning:'`.

**R-RUN: run the drivers.** Locate each with `find "$SCRATCH/build-oswt" -name 'O-simpleWavetable-<x>-check' -type f -perm +111`; run into `$SCRATCH/<x>.log 2>&1`; `dsp-check --alloc-check` into `$SCRATCH/dsp-alloc.log`.
- **Accept:** every exit 0; bank / dsp / mod / import / viz end with `<x>-check: ALL PASS`; state-check 0 FAIL with 11 PASS (12 from Task 7 on: P11); `--alloc-check` G-ALLOC 0 allocations with liveness counted; `grep -c 'JUCE Assertion failure'` = 0 in every log; every negative-control line says `FAILS as designed`, and none says `vacuous`.

**R-GOLD: goldens move only where intended (D-AU).**
```bash
S4NEW='G-(UNPREPARED|MIDI-FLOOD|MONO-WHEEL|LEGATO-|VIZ-RELEASE|SANITISE|DROP-CAPSYNC|FACTORY|PRESET-|S3N5|S3N13|S3W4-|S3W5-|Q4-|PERF02-)|^(PASS|FAIL) P11 '
for x in bank dsp dsp-alloc mod import state viz; do
  diff <(grep -E '^(PASS|FAIL)' "$BASE/base-$x.log" | grep -vE "$S4NEW") \
       <(grep -E '^(PASS|FAIL)' "$SCRATCH/$x.log"   | grep -vE "$S4NEW") > "$SCRATCH/gold-$x.diff"
  grep -E "$S4NEW" "$SCRATCH/$x.log" | grep -c '^FAIL'      # must print 0
done
```
- `$BASE` = Task 1's scratch in Part 1; Task 15's `p1-*` logs in Part 2.
- **Allowed diff hunks, Part 1 (closed list):** bank `G-TIME` buildMillis; import soak block count + its `G-FINITE` sample count; dsp `G-BLOCK` detail `peak x.xxx` (W4) and `G-FINITE` sample count; dsp-alloc `G-ALLOC` scenario / block counts (allocation count stays 0); viz `G-LESSON[...]` lines (9 ids replace 5, all PASS, NC still `FAILS as designed`), `G-DROP[b]` char count 134,217,733 → 22,369,625, and `G-FINITE` sample count. **Part 2:** timing counts + dsp `G-FINITE` sample count only.
- Anything else is a regression → stop. Reference values that must not move: G-PITCH 0.0009 c, G-Q2-C8 −114.3 dB, G-Q2-SWEEP −74.9 dB, G-Q2-PULSE −67.0 dB, G-POLY stolen 49 −88.5 dB, G-RETRIG 0.00856 / 0.00856, G-VEL −11.905 dB, G-CLICK-SQ / -SH 1.414 / 1.144, G-STEAL 1.076×, G-SWITCH-TAIL 0.656×, G-RETRIG-VEL 0.589×, QUAL-03 exact ≤ 6e-8 / ratio worst 1.31, `--alloc-check` 0.

**R-UNDEF: per-header `-Wundef` (Stage 2 note 8).**
```bash
# FLAGS = the compile line of the plugin target's PluginProcessor.cpp entry in
# $SCRATCH/build-oswt/compile_commands.json (NOT a console entry: those define OSIW_TEST_HOOKS),
# minus -c / -o / the source; every -I to JUCE modules or JuceLibraryCode rewritten to -isystem.
for h in plugins/O-simpleWavetable/Source/*.h; do
  printf '#include "%s"\n' "$(basename "$h")" > "$SCRATCH/undef.mm"
  xcrun clang++ $FLAGS -fsyntax-only -Wundef -Werror=undef -x objective-c++ "$SCRATCH/undef.mm" \
    > /dev/null 2>&1 || echo "UNDEF $h"
done > "$SCRATCH/undef.log"
```
- **Accept:** `undef.log` empty. **Negative control:** Task 1's run on the unmodified tree lists `WtVoice.h`, `PositionSmoother.h` and `PositionLfo.h` (RESEARCH B §3: 2 / 1 / 5 warnings).

**R-STATIC** (from `plugins/O-simpleWavetable`; full-line comments stripped where noted):
- **S1 ASCII:** `LC_ALL=C grep -n '[^[:print:][:space:]]' Source/*.h Source/*.cpp tests/*/main.cpp CMakeLists.txt` prints nothing; `grep -c PLUGIN_VERSION CMakeLists.txt` = 0.
- **S2 Hooks:** `grep -ln 'define OSIW_TEST_HOOKS' Source/*.h` = `Source/TestHooks.h` only; every other `OSIW_TEST_HOOKS` in `Source/` is an `#if`; `OSIW_TEST_HOOKS=1` appears only on the 5 hooked console targets' CMake lines.
- **S3 Lesson ids:** prints nothing, and the table holds 9 unique ids:
  ```bash
  comm -23 <(grep -o 'data-preset="[A-Za-z0-9]*"' Source/ui/public/index.html | sed 's/.*="\(.*\)"/\1/' | sort -u) \
           <(grep -o 'make ("[A-Za-z0-9]*"' Source/PresetRecipes.h | sed 's/.*("\(.*\)"/\1/' | sort -u)
  ```
- **S4 Resource branches:** for each of the 12 URLs (Stage 3's 11 + `/modules/preset-manager.js`), `grep -qF "url == \"$u\"" Source/PluginEditor.cpp || echo "MISSING $u"` prints nothing.
- **S5 Relays:** in `PluginEditor.h`, `WebSliderRelay>` 13, `WebComboBoxRelay>` 6, `WebToggleButtonRelay>` 2, `Web[A-Za-z]*ParameterAttachment>` 21; `grep -c '\.withOptionsFrom (' Source/PluginEditor.cpp` = 21.
- **S6 Member order:** last `…Relay>` line < `std::unique_ptr<juce::WebBrowserComponent>` line < first `…ParameterAttachment>` line; every new editor member (chooser, flags, preset counters) sits above the relay block.
- **S7 Invariants:** `grep -c` = 1 each for `setSize (1120, 780)`, `setResizable (false, false)`, `url == "/js/i18n.js"`, `withUserDataFolder`, `withKeepPageLoadedWhenBrowserIsHidden` (PluginEditor.cpp), `int getNumPrograms() override { return 1; }` (PluginProcessor.h); `NEEDS_WEBVIEW2 TRUE` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (CMakeLists.txt).
- **S8 BinaryData:** `grep -l 'BinaryData.h' Source/*.h Source/*.cpp` = `Source/PluginEditor.cpp` only; `grep -c 'BinaryData::presetmanager_js' Source/PluginEditor.cpp` ≥ 1 (hyphen stripped, memory `critical_binary_data_strips_hyphens`).
- **S9 Binary-data list:** `sed -n '/juce_add_binary_data(O-simpleWavetable_UIResources/,/^)/p' CMakeLists.txt | grep -cE '\.(html|js|png|css|woff2)'` = 11.
- **S10 Preset-manager traps** (D-AA): `grep -vhE '^\s*//' Source/*.cpp Source/*.h | grep -cE 'setStateFromXml|getStateAsXml|savePresetToFile|applyPresetData|setCustomStateCallbacks|presetManager\.(loadPreset|getNextPreset|getPreviousPreset|getPresetList)'` = 0; `grep -cE 'presetManager\.savePreset *\(' Source/PluginProcessor.cpp` ≥ 1.
- **S11 Recipe table:** `grep -vE '^\s*//' Source/PresetRecipes.h | grep -c '"output_level"'` = 0; the G-LESSON expected table in `tests/viz-check/main.cpp` is literal values (inspect: no `wtpresets::` reference inside `lessonTable()`).
- **S12 Generated module copies:** `git check-ignore -q Source/ui/public/modules/preset-manager.js` and `…/webview-drop-streaming.js` exit 0.
- **S13 MSVC traps** (RESEARCH C §3.1): `grep -rn "= juce::Component::SafePointer" Source` = 0; `grep -nE "^\s+constexpr " Source/*.cpp | grep -v static` → each hit inspected (not captured by a lambda; MSVC C3493); every `constexpr` function body has no uninitialised local (C3615).
- **S14 Native namespace:** `grep -c '__JUCE__' Source/ui/public/index.html` = 1 (the event backend line).

**R-BRIDGE: page ↔ C++ cross-check** (from Task 12 on; needs the configure-time `modules/preset-manager.js` copy):
```bash
comm -3 \
  <(cat Source/ui/public/index.html Source/ui/public/modules/preset-manager.js \
      | grep -oE "getNativeFunction\([\"'][A-Za-z]+[\"']\)" | sed -E "s/.*\([\"']([A-Za-z]+)[\"']\)/\1/" | sort -u) \
  <(grep -o 'withNativeFunction ("[A-Za-z]*"' Source/PluginEditor.cpp | sed 's/.*("\(.*\)"/\1/' | sort -u)
```
- **Accept:** prints nothing; `grep -c 'withNativeFunction ("' Source/PluginEditor.cpp` = 20.
- Events: the `emitEventIfBrowserIsVisible ("<name>"` set in `PluginEditor.cpp` = the page's `addEventListener('<name>'` backend set = {`bankUpdate`, `cycleUpdate`, `importStatus`, `presetState`}.
- `FACTORY_LABELERS` keys (`sed -n '/const FACTORY_LABELERS = {/,/^};/p' Source/ui/public/index.html | grep -oE '^\s+[A-Za-z0-9]+:' | tr -d ' :' | sort`) = the 9 table ids.
- Relay census 13 / 6 / 2 unchanged (N13 adds a native, not a relay).

**R-UISTATIC: the three CI gates** (`.github/workflows/ui-static-gates.yml` runs them on every push):
- `node scripts/check-i18n.js --plugin O-simpleWavetable` → exit 0, 0 FAIL.
- `node scripts/i18n-fr-lint.js --plugin O-simpleWavetable` → exit 0, CLEAN.
- `node scripts/i18n-zh-lint.js --plugin O-simpleWavetable` → exit 0 **and** the output names O-simpleWavetable with its entry count (142 at Stage 3 → ≈ 168–172 after Task 11), 0 findings (`mt` entries are counted, not failed). Exit 0 alone is vacuous (the script returns 0 when `--plugin` matches nothing).

**R-UIPW: the two Playwright gates.** Run only after a configure has placed `preset-manager.js` in `Source/ui/public/modules/` (serve-ui places module JS at the editor's url branches; a missing copy is a 404, not a page defect — memory `pattern_hand_built_gate_tree_404s_module_embedded_assets`).
- `node scripts/check-ui-labels.js --plugin O-simpleWavetable` → exit 0 across all states of `tests/i18n-states.json` (8 → 16 after Task 11), including the [7] pins and "every requested resource was served".
- `node scripts/boot-all-uis.js --plugin O-simpleWavetable --strict-tips` → exit 0; the O-simpleWavetable row shows 0 DEAD, 0 late tips, 0 404, 0 page errors.
- **Exit 77 = Playwright not resolved = nothing verified, never a pass** (`npx playwright install chromium`, re-run). The output must name O-simpleWavetable's page (memory `pattern_ui_test_server_port_clash_serves_other_session`). Screenshots go to `$SCRATCH`.

**R-WHEEL: G-S3W3-WHEEL probe (D-AO, D-AP)** — `node plugins/O-simpleWavetable/tests/ui-probes/wheel-gesture-probe.mjs` → exit 0 (same 77 rule).
- **Negative control:** build the shipped Stage 3 page tree — `git archive f4eea85a plugins/O-simpleWavetable/Source/ui | tar -x -C "$SCRATCH/s3ui"`, then copy the gitignored `Source/ui/public/modules/*.js` into it (memory `pattern_git_archive_ui_tree_misses_gitignored_module_copies`) — and run the probe with `--root` pointing at it. It must exit non-zero with the stepped, stack, continuous, N8 and N13 arms each printing `FAILS as designed`.

**R-INSTALL: build, install, validate** (each long step in the background):
1. `./scripts/build-and-install.sh O-simpleWavetable` (cache clear + `-dev` / unsuffixed sweep; VST3 + AU only).
2. `ninja -C build O-simpleWavetable_Standalone` (the script never rebuilds it; memory `pattern_build_install_skips_standalone_stale_ui`).
3. Freshness: `strings <bin> | grep -c presetState` ≥ 1 on `~/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3/Contents/MacOS/O-simpleWavetable-dev` and on `build/plugins/O-simpleWavetable/O-simpleWavetable_artefacts/Release/Standalone/O-simpleWavetable-dev.app/Contents/MacOS/O-simpleWavetable-dev`; the Standalone's mtime ≥ the built VST3 artefact's; installed bundles `diff -rq`-equal to the artefacts; `nm -gU <installed VST3 bin> | grep -c ForTesting` = 0.
4. `bash scripts/verify-au-link.sh O-simpleWavetable` (= `auval -v aumu OSiW OuDv`; **never `auval -a`**). The first run after the cache clear is a cold rescan (83 s – 17 min): wait. **Accept:** `AU VALIDATION SUCCEEDED`, only the 2 known benign skew-default warnings (`lfo_rate`, `amp_attack`).
5. `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate <bundle>` for the VST3 and for `~/Library/Audio/Plug-Ins/Components/O-simpleWavetable-dev.component`. GUI tests **on**. **Accept:** exit 0, `SUCCESS`, 0 `FAILED`.
- If the shared `build/` re-configure fails on another plugin's in-flight CMake edit (O-simpleFM, O-Gain, O-Reed, O-Bassoon, O-Freeze are dirty at plan time), **stop and report**. Never edit foreign files.

**R-NULL: binary null test (D-AT).** `$NULL = $BASE/null`. Each binary renders in **its own process** (`uv run --no-project --python 3.12 --with pedalboard --with numpy --with mido python null_render.py <vst3> <out.npz>`), params set by `parameters[k].raw_value`, 48 kHz, 3.5 s stereo.
- **Cases:** the 6 Stage 3 cases from the Stage 3 script — `default_chord`; `bank_saw_pos1_blOff` (seq 84 88 91 96); `lfo_tri_interp` (pos 0.5, depth 1, 0.4 Hz); `bits3_noInterp` (bit_depth 1.0, interp 0, pos 0.3; seq 40 52 64 76); `mono_legato` (voice_mode 1; 60 62 64 65 67 every 0.15 s, length 0.3); `steal17` — plus 2 positive controls: `w3_poly_to_mono` (chunked `process(reset=False)` calls: Poly A4 + wheel +8191 → note off → wheel back to centre → `voice_mode` = 1 → A4; f by zero crossings over 0.2–0.9 s of the last note) and `legato_60_96_drive` (Mono, `bank` 0.8, `position` 1.0; 60 held, 96 pressed at 0.5 s).
- **Accept (vs the Stage 3 snapshot):** the 5 untouched cases max abs diff 0.0 per channel; `mono_legato`: `idx = np.nonzero(a != b)[0]; assert idx.min() >= 28801 and idx.max() <= 29039`; `w3_poly_to_mono`: snapshot ≈ +200 c, Stage 4 |c| ≤ 1; `legato_60_96_drive`: diff confined to t+1 … t+239. The Stage 4 binary rendered twice → bit-identical. Control: different cases differ (≈ 0.43), so the comparison can fail.
- **Part 2 (vs the Part 1 snapshot, Task 15):** all 8 cases max abs diff 0.0.

**R-COMMIT: path-scoped temp-index CAS commit** (memory `index_git_shared_checkout`; run under `bash -c` — zsh pathspec and `:`-modifier traps):
1. **Immediately before:** `git branch --show-current` = `main`; `git status --short`; `git diff --cached --stat`. Note foreign-staged files (PLUGINS.md is `MM` at plan time with another session's blob that downgrades O-AnalogSaturation 1.8.0 → 1.7.0 and O-DigiDelay 1.9.1 → 1.7.0). Never stage, reset or commit them.
2. `export GIT_INDEX_FILE="$SCRATCH/idx"; old=$(git rev-parse HEAD); git read-tree "$old"; git add -- plugins/O-simpleWavetable` (respects `.gitignore`, so `Source/ui/public/modules/*` stays out; a pathspec `git commit` would skip new untracked files).
3. **Task 21 only:** build the PLUGINS.md blob from `git show "${old}:PLUGINS.md"`, substituting **only** the `| O-simpleWavetable |` row → `git hash-object -w` → `git update-index --cacheinfo "100644,<blob>,PLUGINS.md"`.
4. `tree=$(git write-tree); new=$(git commit-tree "$tree" -p "$old" -F "$SCRATCH/msg")`.
5. `git update-ref refs/heads/main "$new" "$old"` (CAS: if HEAD moved, redo 2–5 from the new HEAD); `unset GIT_INDEX_FILE`.
6. **After:** `git rev-parse HEAD` = `$new`; `git show --stat --format= HEAD` lists only `plugins/O-simpleWavetable/**` (+ `PLUGINS.md` in Task 21); `git ls-tree -r --name-only HEAD -- plugins/O-simpleWavetable/Source/ui/public/modules` is empty.
7. **Resync own paths only:** `git reset -q -- plugins/O-simpleWavetable`. **Skip PLUGINS.md** (foreign-staged). In Task 21, if `git diff --cached -- PLUGINS.md` shows their staged blob would revert this row, patch **only this row** inside their staged blob (memory `pattern_stale_base_temp_index_commit_reverts_shared_row`); touch nothing else in it.
8. Never `git add -A`, `commit -a`, a bare `reset`, `stash`, or any tag.

---

## Tasks

### Part 1: critic fixes + preset manager (Tasks 1–14)

#### Task 1 — Pre-flight, baselines, Stage 3 null snapshot  *(orchestrator; BEFORE any build-and-install)*
- **Files:** none in the repo; `$SCRATCH` only.
- **Depends on:** none.
- **Steps:**
  1. **Branch and tree:** `git branch --show-current` = `main`; `git status --short -- plugins/O-simpleWavetable` empty (the plan commit landed). Record `S4BASE=$(git rev-parse HEAD)` and `BASE=$SCRATCH`.
  2. **Contract tamper check:** `shasum -a 256` of `BRIEF.md`, `parameter-spec.md`, `research/ARCHITECTURE.md`, `ROADMAP.md` equal STATUS `contract_checksums` (all four match at plan time: `9cee315a…`, `e77ae944…`, `b7ddf56f…`, `f657e02a…`). Any mismatch → stop.
  3. **Stage gate:** `bash -c '.planning/workflow/scripts/run-gate.sh O-simpleWavetable 3-gui 4-polish --skip-review'`. Exit 1 → `--force` with a stdin justification only for a known gate limitation, never a real build / pluginval failure.
  4. **Null baseline snapshot (D-AT) — before anything is installed:**
     - The installed `-dev` VST3 must be the Stage 3 binary: `diff -rq ~/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3 build/plugins/O-simpleWavetable/O-simpleWavetable_artefacts/Release/VST3/O-simpleWavetable-dev.vst3` prints nothing; `strings <bin> | grep -c cycleUpdate` ≥ 1; `strings <bin> | grep -c presetState` = 0.
     - `mkdir -p "$SCRATCH/s3-baseline"`; `cp -R` the VST3 and `~/Library/Audio/Plug-Ins/Components/O-simpleWavetable-dev.component` into it; `shasum -a 256` both Mach-O binaries into `$SCRATCH/s3-baseline/SHA256`. (The bundles also serve as the binary rollback.)
     - Check fails → fallback: RESEARCH B §6 recipe (`git archive f4eea85a` whole repo → Release `O-simpleWavetable_VST3`, background) and use that artefact; record which baseline was used.
  5. **Null scripts:** copy `null_render.py` and `run.sh` from `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/d9bfa2c4-aba1-4f1f-a353-7d429b726ff5/scratchpad/` into `$SCRATCH/null/` (present at plan time; rebuild from the R-NULL case table if gone) and append the 2 positive-control cases. Render the snapshot twice → `base.npz`, `base2.npz`; they must be bit-identical (determinism).
  6. **Offline baseline:** R-DBG on the unmodified tree (with `viz-check` + compile commands), R-RUN → `$SCRATCH/base-{bank,dsp,dsp-alloc,mod,import,state,viz}.log`.
  7. **R-UNDEF negative control:** run R-UNDEF on the unmodified tree → `$SCRATCH/base-undef.log`.
- **Accept:** on `main`, clean; checksums match; gate passed or the bypass logged; snapshot + SHA256 present (or the fallback recorded); `base.npz` == `base2.npz`; 7 base logs satisfy R-RUN (state 11 PASS); `base-undef.log` lists `WtVoice.h`, `PositionSmoother.h`, `PositionLfo.h`.

#### Task 2 — Stage 2 note 8 + note 4 + note 3 + W3: the processor block path  *(dsp-agent, dispatch A)*
- **Files:** create `Source/TestHooks.h`; edit `Source/WavetableBank.h`, `Source/WtVoice.h`, `Source/PositionSmoother.h`, `Source/PositionLfo.h`, `Source/MipmapBuilder.h`, `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `CMakeLists.txt`, `tests/dsp-check/main.cpp`.
- **Depends on:** Task 1.
- **Steps:**
  1. **Note 8 first (RESEARCH B §3).** `Source/TestHooks.h` = `#pragma once` + the 3-line default (`#ifndef OSIW_TEST_HOOKS` / `#define OSIW_TEST_HOOKS 0` / `#endif`) + one comment line. Include it **first** in `WavetableBank.h` (replacing `:46-50`), `WtVoice.h` (before `:104`), `PositionSmoother.h` (before `:51`), `PositionLfo.h` (before `:94`), `MipmapBuilder.h` (before `:80`) and `PluginProcessor.h` (before `:74`). Add `Source/TestHooks.h` to `target_sources` (`CMakeLists.txt:24-49`).
  2. **Note 4 (`proc.diff`; D-AJ(1)).** `std::atomic<bool> prepared { false };` among the audio-thread members (`PluginProcessor.h` near `:445-447`). `prepared.store (true)` is the **last** statement of `prepareToPlay` (after `setLatencySamples (0)`, `:337`). `processBlock` (`:355-377`): `const bool ready = prepared.load();` and the render condition at `:362` becomes `ready && numSamples > 0 && buffer.getNumChannels() > 0`. The single REG-01 exit is unchanged (`blockEntries` first, `blockGeneration` last). `releaseResources` does **not** touch `prepared`. Hook: `setPreparedGuardForTesting (bool)` (default guard on).
  3. **Note 3 (`proc.diff` chunk hunk).** Constants (`:57-58`): `kWheelMidiBytes = kMidiChunkBytes`; `kMidiEventHeader = (int) (sizeof (juce::int32) + sizeof (juce::uint16))`. Replace the per-chunk MIDI copy (`:444-475`, copy at `:460-466`) with the iterator-based, byte-capped fill: one forward iterator across all chunks; skip events with `numBytes > 3`; if `chunkBytes + kMidiEventHeader + numBytes > kMidiChunkBytes`, end the chunk at that event (`n = p − start`, may be 0); `start += n` at the loop end; `lastChunkLen` set only when `n > 0`. The normal path stays bit-identical (same events, order, chunks). Hook: `setMidiCapacityGuardForTesting (bool)`.
  4. **Stage 2 W3 (`proc.diff`; RESEARCH B §1).** Audio-thread member `int wheelNow = 8192;`, updated in the chunk fill for every 3-byte `0xE0` event (`(d1 & 0x7f) | ((d2 & 0x7f) << 7)`). The mode switch (`:388-393`) seeds `wtVoices[0]->pitchWheelMoved (wheelNow)` when the new mode is Mono. Hook: `setMonoWheelSeedForTesting (bool)`.
  5. **Gates in `tests/dsp-check/main.cpp`** (wired into `main()` after `gateBlock()`, `:1490`; alloc arms inside `allocScenario()`, `:1336`). Prototype: `research-probes/B/fixprobe.cpp`.
- **Gates** (thresholds frozen; measured prototype values in brackets):
  - **R-UNDEF:** 0 `UNDEF` lines. NC = Task 1's 3-header list.
  - **G-UNPREPARED:** a **self-exec** child, `posix_spawn (argv[0], "--child-unprepared" [, "--guard-off"])` — never `fork` (a fork after JUCE threads start can deadlock the child). Child arms: (a) construct, `setPlayConfigDetails`, two `processBlock` calls on a buffer prefilled with 1.0 plus a note-on → output exactly 0; (b) `prepareToPlay` → note-on → render → peak > 0.01; (c) `releaseResources()` → note-on → `processBlock` → peak > 0.01 (D-AJ(1)). Fixed child exits 0. **NC** `--guard-off`: the child dies by a signal [SIGSEGV 11] → `FAILS as designed`.
  - **G-MIDI-FLOOD** (Poly arm, Mono arm): one 512-sample host block carrying 6,000 events at one sample, then a pitch wheel +2 st after them, with a held note → pitch after the flood +2 st ± 1 c [493.88 Hz exact]; the note-off still lands (tail RMS < 1e-6). The gate asserts `6000 * 9 > 49160` (the flood exceeds the 49,160-byte reserve; `ensureSize(32768)` reserves `(n + n/2 + 8) & ~7`).
  - **`--alloc-check`:** the Poly and Mono flood blocks join `allocScenario()` (armed) → 0 allocations. A separate arm on a fresh Rig with `setMidiCapacityGuardForTesting (false)` must count ≥ 1 allocation [1 / 2] → `FAILS as designed`.
  - **G-MONO-WHEEL:** Rig 48 kHz, block 512, `baseParams (sineSaw, 0)`; 56 blocks of Poly history; `set (voiceMode, 1)` on a block boundary; note-on A4 at +64 samples; render 1 s; f by zero crossings over 0.2–0.9 s. Arms: **A** Poly A4, wheel +8191, note off, wheel back to centre while idle; **B** no note, wheel −4096 (−1 st); **C** no wheel. Fix: |cents| ≤ 1 on A, B, C [+0.00 / −0.00 / +0.00]. **NC** (`setMonoWheelSeedForTesting (false)`): A ≥ +150 c, B ≥ +75 c [+199.98 / +100.00] → `FAILS as designed`.
- **Must not:** change `renderMono`, any voice code, or any other `processBlock` / `renderBlock` line; allocate or lock on the audio path; define a hook outside `#if OSIW_TEST_HOOKS`.
- **Done when:** the files compile in Task 5 and every gate above passes there with its NC firing.

#### Task 3 — Stage 2 W4: the frozen cycle keeps the rate it was heard at  *(dsp-agent, dispatch A)*
- **Files:** `Source/WtVoice.h`, `Source/PluginProcessor.h` (hook forwarder), `tests/dsp-check/main.cpp`.
- **Depends on:** Task 2 (same dispatch, sequential).
- **Steps (transcribe `research-probes/B/w4.diff`; RESEARCH B §2):**
  1. Members beside the crossfade state (`WtVoice.h` near `:799-826`): `double xfPhase = 0.0, xfInc = 0.0, renderedInc = 0.0;`.
  2. Render loop crossfade branch (`:401-409`): read the frozen cycle at `xfPhase` (not `phase`), then `xfPhase += xfInc; if (xfPhase >= 1.0) xfPhase -= 1.0;`. After the loop (`:432`): `renderedInc = inc;`.
  3. `checkConfig` (`:686-729`): `curOffset = (folding && keepRate) ? xfPhase − phase : 0.0`; the fold reads the running frozen cycle phase-aligned (`juce::exactlyEqual (curOffset, 0.0) ? cur[i] : readFrozen (cur, wrap01 (i / 2048.0 + curOffset))`; D-AJ(2)); after the capture `xfPhase = phase; xfInc = keepRate ? renderedInc : inc;`.
  4. Hook `bool xfKeepRate = true;` in WtVoice's `#if OSIW_TEST_HOOKS` block (`:507-535`) + processor forwarder `setXfadeKeepRateForTesting (bool)` beside `setXfadeLenOverrideForTesting` (`PluginProcessor.h:298-302`).
- **Gates** (dsp-check, 48 kHz, processor Rig, block 64, pos 1; prototype `fixprobe.cpp::w4`):
  - **G-LEGATO-XF** (exactness on the real 5 ms fade), 5 arms: legato 60→96 Sine→Saw; legato 60→96 Drive; Mono fallback 60→96 Drive; sounding retrigger 60→96 Drive; Poly wheel F#4 +2 st Drive. Reference `(1 − w_k)·y_old[t+k] + w_k·y_hard[t+k]` for k < 240 (`y_old` = the same render without the pitch change — arm c retriggers the same note; `y_hard` = `setXfadeLenOverrideForTesting (0)`). Fix: max |y − ref| ≤ 1e-6 [5.08e-8 … 7.10e-8]. **NC** (`setXfadeKeepRateForTesting (false)`): ≥ 0.1 [0.544 / 1.001 / 1.000 / 0.998 / 0.848]. **Confinement:** fixed vs NC renders are bit-identical before t and from t+240 on [0 differing samples].
  - **G-LEGATO-ALIAS** (magnifier): Sine→Saw and Drive, 60→96, at **48 kHz** (the magnifier is blind at 96 kHz: folds land inside the ±14-bin mask), `setXfadeLenOverrideForTesting (96000)` (2 s), Kaiser-38 FFT 2^15 at t + 0.35 s masking ±14 bins around k·f60 (Q2Analyzer-style, `dsp-check/main.cpp:467-530`). Fix: ≤ the gate's own no-jump floor + 1 dB [−102.0 vs −102.0; −107.2 vs −107.2]. **NC:** ≥ floor + 40 dB [−24.3 / −26.0].
  - **G-LEGATO-CLICK:** max |Δy| over the fade ÷ steady max |Δy|, Drive 60→96: fix ≤ 1.5 [1.000]; NC printed [1.532] → `FAILS as designed`.
- **Must not:** change any non-pitch trigger's output (bank, interp, band-limit, `forceLevel` triggers stay bit-identical: import-check QUAL-03 exactness ≤ 6e-8 and G-BLOCK PASS unchanged under R-GOLD).
- **Done when:** Task 5 shows the three gates PASS with their NCs firing.

#### Task 4 — Stage 3 N4 + N7 + W2 cap  *(dsp-agent, dispatch A)*
- **Files:** `Source/PluginProcessor.{h,cpp}`, `Source/WavetableImporter.{h,cpp}`, `Source/ui/public/index.html` (**the `:1144` constant only**), `tests/viz-check/main.cpp`.
- **Depends on:** Task 3.
- **Steps:**
  1. **N4 (`proc.diff`; RESEARCH B §5).** Extract `void resetDisplayState (double fs) noexcept` holding the 7 stores at `PluginProcessor.cpp:329-335`; `prepareToPlay` calls it with `sampleRate`; `releaseResources` (`:340-345`) calls it with `displayFs.load (relaxed)`. Hook: `setReleaseClearsDisplayForTesting (bool)`.
  2. **N7 (RESEARCH B §5).** Add `bool isStrippedNameChar (juce::juce_wchar c) noexcept` to the `WavetableImporter` namespace (declared in `WavetableImporter.h` beside `sanitiseName`, `:104-106`), with exactly RESEARCH's strip set: `c < 0x20`, `0x7f`, `0x80–0x9f`, `0x061c`, `0x200b`, `0x200e`, `0x200f`, `0x202a–0x202e`, `0x2028`, `0x2029`, `0x2060`, `0x2066–0x2069`, `0xfeff`, `0xd800–0xdfff`, `0xfffe`, `0xffff`, `> 0x10ffff`, `0xe0000–0xe007f`. Keep U+200C / U+200D. `sanitiseName` (`WavetableImporter.cpp:167-185`) keeps a char iff `! isStrippedNameChar (c)`. Update the `:104-106` comment ("control, C1, bidi and invisible characters removed").
  3. **W2 constants (D-AK).** `kMaxMemoryBytes = (std::size_t) 16 * 1024 * 1024` (`WavetableImporter.h:89`; the comment still names the page `DROP_MAX_BYTES`); `index.html:1144` → `const DROP_MAX_BYTES = 16 * 1024 * 1024;`. No other page edit in this dispatch (copy and mapping are Tasks 10–11).
- **Gates** (viz-check):
  - **G-VIZ-RELEASE:** hold a note, render 0.2 s, `isDisplaySounding()` true (liveness) → `releaseResources()` → sounding false, note −1, hz 0, amp 0, and `buildCycleView` returns the silent payload (note −1, f0 0; D-R). **NC** (`setReleaseClearsDisplayForTesting (false)`): still sounding [1 / 64 / 329.6 / 1.00] → `FAILS as designed`.
  - **G-SANITISE:** (a) predicate table: every listed code point / range endpoint → stripped; U+200C, U+200D, `a`, `é` → kept. (b) Strings: `"a" + charToString (cp) + "b.wav"` → `"ab.wav"` for every listed **BMP non-surrogate** code point and U+E0001; ZWJ / ZWNJ kept. (c) **Per-class NC:** a test-local copy of the legacy predicate (`c >= 0x20 && c != 0x7f`) keeps every non-C0 stripped code point → `FAILS as designed` (proves each row exercises the new rule). (d) End to end: `importFromBase64` named `"x" + U+202E + "gnp.wav"` → status, thumbnails and wire all say `"xgnp.wav"`; a state blob with `filename="a&#x202E;b.wav"` restores as `"ab.wav"`. G-DROP[d] (`"x.wav"`) unchanged.
  - **G-DROP-CAPSYNC:** parse `DROP_MAX_BYTES = A * 1024 * 1024` from `index.html` (located via `__FILE__`, as G-IMPORT-ERR does, `viz-check/main.cpp:2094`) → `A · 2^20 == WavetableImporter::kMaxMemoryBytes`. **NC:** an in-memory copy of the page text with `96` substituted → mismatch detected → `FAILS as designed`. G-DROP[b] follows the constant automatically (`:2018`); its char count becomes 22,369,625 (named R-GOLD diff).
- **Done when:** Task 5 shows these PASS with NCs firing.

#### Task 5 — DSP gate: static, offline, R-GOLD  *(orchestrator; the DSP tracer)*
- **Do:**
  - R-STATIC S1, S2, S5–S8, S13 (S3 / S4 / S9–S12 apply from Task 9 on).
  - `git diff --stat -- plugins/O-simpleWavetable` touches only the files listed in Tasks 2–4. `git diff -- Source/WtVoice.h` = the TestHooks include + the `w4.diff` hunks only (inspect).
  - R-DBG, R-RUN, R-GOLD (Part 1 allowlist; at this point the viz `G-LESSON` lines must still be **unchanged**), R-UNDEF.
  - Every new gate line PASS; every NC prints `FAILS as designed`.
- **On failure:** route the gate output back through dsp-agent dispatch A. Thresholds are frozen.
- **Depends on:** Tasks 2–4.
- **Done when:** 0 plugin warnings; 7 logs ALL PASS (state 11); `--alloc-check` 0 incl. the flood blocks; R-GOLD diffs only on the allowlist (G-BLOCK `peak`, G-DROP[b] count, G-FINITE / alloc counts); `undef.log` empty; every new NC fires.

#### Task 6 — Recipe table + apply core + N5  *(gui-agent, dispatch B)*
- **Files:** create `Source/PresetRecipes.h`; edit `CMakeLists.txt` (`target_sources`), `Source/PluginProcessor.{h,cpp}`, `tests/viz-check/main.cpp`.
- **Depends on:** Task 5 green.
- **Steps:**
  1. **`Source/PresetRecipes.h`** (D-AD): transcribe RESEARCH A §4's `wtpresets` block verbatim (9 arrays + `kFactory`), plus inline helpers `findById (const char*)` and `findByName (const juce::String&)` (case-insensitive). C++17 only (no `std::span`); ASCII; choice values as indices (bank 0..4, `lfo_shape` 0..4, `bit_depth` 9 = "8", 13 = "4").
  2. **Apply core** (D-AB): `bool applyNormalisedTargets (const std::array<float, 21>&)` + `std::array<float, 21> recipeTargets (const wtpresets::Recipe&) const` (NaN = unlisted).
  3. **`applyFactoryPreset (id)`** (`PluginProcessor.cpp:1136-1203`): body → `findById` → `recipeTargets` → `applyNormalisedTargets` → `setCurrentPresetLocked (recipe.name, applied targets)` (name, targets, valid = true, `++presetRevision`, all under `presetLock`). Unknown id → `false`, no change (Stage 3 contract). Keep the parameter name `id`. Remove the inline `std::vector<RecipeEntry>` recipes.
  4. **Preset identity members** (D-AF): `juce::CriticalSection presetLock`; `juce::String presetName`; `std::array<float, 21> presetTargets`; `bool presetTargetsValid`; `std::atomic<juce::uint32> presetRevision { 0 }`. Public: `juce::String getPresetName() const`, `juce::String getPresetId() const` (table id or `""`), `bool isPresetFactory() const`, `juce::uint32 getPresetRevision() const noexcept`, `bool isPresetModified() const` (D-AG; 1e-4; `output_level` excluded; unnamed → false). The ctor sets "Init - Additive Build" with `recipeTargets (kInit)` targets.
  5. **viz-check** (prototype shapes: RESEARCH A §4 pitfalls, §5 N5):
     - **G-LESSON re-transcription:** replace `lessonTable()` (`viz-check/main.cpp:393-409`) with a literal transcription of ARCHITECTURE §A9 + the D-AD fill rule for **all 9 ids**, typed independently (never `wtpresets::`; R-STATIC S11). Keep every existing assertion (values within 1e-6 normalized; exactly 1 begin + 1 end per changed param, 0 for unchanged; `output_level` −17.3 dB untouched with 0 gestures; Imported pointer unchanged; reapply → 0 gestures; `"nope"` / `""` → false, no change). Add: `getPresetName()` = the recipe's file name, `isPresetModified()` = false. Keep G-LESSON-NC.
- **Gates:**
  - **G-FACTORY** (includes `PresetRecipes.h`): per entry `std::isfinite (raw)`; raw round trip `convertFrom0to1 (convertTo0to1 (raw)) ≈ raw` (rel 1e-4; choice values integral; no clamp); ids unique; names ASCII, no `/\:`, `== juce::File::createLegalFileName (name)`; no recipe lists `output_level`; no recipe sets `bank` = 5; after `applyFactoryPreset (id)` every listed parameter's `convertFrom0to1 (getValue())` ≈ raw (rel 1e-4, the **skew arm**); `output_level` set to −17.3 dB first is untouched by all 9; `kInit` targets == the APVTS defaults; a fresh instance reports `"Init - Additive Build"`, unmodified. **NC (skew trap, memory `pattern_factory_preset_normalized_ignores_skew`):** a test-local apply that normalises linearly (`(raw − start) / (end − start)`) fails the skew arm on `lfo_rate` and the env times → `FAILS as designed`.
  - **G-LESSON:** 9 ids PASS + reapply + unknown; NC fires.
  - **G-S3N5:** `importFromMemory` (valid 3-frame 16-bit WAV) → poll until `done` → `applyFactoryPreset ("aliasDemo")` → `handleUpdateNowIfNeeded()` → bank index == 4 (Drive). **Sensitivity arm:** the same sequence without the apply → bank == 5 (proves the auto-select was pending).
- **Must not:** read the disk for a factory preset; touch the imported bank in any apply.
- **Done when:** Task 9 shows the gates PASS.

#### Task 7 — Module integration, state, natives, `presetState`  *(gui-agent, dispatch B)*
- **Files:** `CMakeLists.txt`, `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.{h,cpp}`, `tests/viz-check/main.cpp`, `tests/state-check/main.cpp`.
- **Depends on:** Task 6.
- **Steps:**
  1. **CMake:** `ouaricon_add_module(O-simpleWavetable preset-manager)` directly after the `webview-drop-streaming` line (`CMakeLists.txt:55`, before `juce_add_binary_data` at `:62`); add `Source/ui/public/modules/preset-manager.js` to the `O-simpleWavetable_UIResources` SOURCES (11 entries).
  2. **Processor (D-AA, D-AE, D-AF, D-AH):** `#include "OuariconPresetManager.h"`; member `OuariconPresetManager presetManager { parameters, "O-simpleWavetable" };` declared after `parameters` (`PluginProcessor.h:348`). Public, message thread:
     - `static juce::String sanitisePresetName (const juce::String&)` (6 steps; `""` = refused).
     - `std::vector<OuariconPresetManager::FactoryPresetDef> buildFactoryPresetDefs() const` (pure: 9 defs, table order, full 20-parameter normalized maps = recipe targets else defaults, `output_level` omitted).
     - `void ensureFactoryBankOnDisk()` (once per instance → `presetManager.initializeFactoryPresets (buildFactoryPresetDefs())`).
     - `juce::StringArray getPresetWalkOrder() const`; `juce::String getNeighbourPreset (int dir) const` (D-AE entry rules; no load).
     - `bool applyUserPresetJson (const juce::var&)` (D-AB caller 2; the file's `parameters` object; known ids; `isDouble / isInt / isInt64 / isBool` only; else default).
     - `bool loadPresetByName (const juce::String&)`: must be in the walk order (blocks traversal); factory → `applyFactoryPreset (id)`; user → `loadFileAsString` + `juce::JSON::parse` → `applyUserPresetJson` → name = stem, cache targets, `++presetRevision`.
     - `bool saveUserPreset (const juce::String& raw)`: clean → empty → false; factory name (case-insensitive) → false; `ensureFactoryBankOnDisk()`; `presetManager.savePreset (clean)`; name = `presetManager.getCurrentPresetName()`; cache current normalized values as targets; `++presetRevision`.
     - `bool deleteUserPreset (const juce::String&)`: factory → false; `presetManager.deletePreset`; if it was current → name `""`; `++presetRevision`.
     - `juce::var getPresetCatalog() const` → `{ factory: [{ name, id } × 9], user: [names] }`.
     - `void refreshPresetTargetsIfNeeded()` (D-AF: a restored user name re-reads its file once).
  3. **State (D-AF):** `getStateInformation` adds `state.setProperty ("currentPreset", getPresetName(), nullptr)` beside `:747-748`; `setStateInformation` reads `currentPreset` from the incoming `state` beside `:768-770` (present → sanitised; absent → `""`; factory → table targets; user → stale until refreshed). Never the module's XML state pair.
  4. **Editor (D-AH, D-AG, D-AN):**
     - Resource branch `if (url == "/modules/preset-manager.js") return makeBinaryResource (BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize, "application/javascript; charset=utf-8");` beside `:103-105`.
     - 12 new `withNativeFunction` entries (D-AH): `getPresetList` (walk order), `getCurrentPreset`, `selectNextPreset` / `selectPreviousPreset` (neighbour name, no load), `loadPreset`, `savePreset`, `deletePreset`, `isFactoryPreset` (table lookup), `savePresetWithDialog` / `loadPresetFromFile` (refusal stubs), `getPresetCatalog`, `stepKnobDrag` (calls `processorRef.stepKnobGesture`, Task 8). Each validates `args.size()` / types (ASVS V5); every preset native (all but `stepKnobDrag`) calls `processorRef.ensureFactoryBankOnDisk()` first (a no-op after the first call). Update the native-list comment (`:181-185`).
     - `emitPresetState (bool force)` → `presetState { name, id, factory, modified }`; members `juce::uint32 lastPresetRevision`, `bool lastPresetModified` (above the relay block).
     - `timerCallback` (`:411-436`): `processorRef.refreshPresetTargetsIfNeeded()`, then emit order `bankUpdate` → `importStatus` → `presetState` (revision changed or modified flipped) → `cycleUpdate`.
     - **W1 (D-AN):** `uiReady` (`:227-237`) assigns the five counters first, then `emitBankUpdate (true)`, `emitImportStatus()`, `emitPresetState (true)`, `forceCycleEmit = true`, `complete (true)`. Rewrite the comment at `:221-226` to match.
     - The `applyFactoryPreset` native (`:293-299`) keeps its shape; the name / revision bump happens inside the processor.
- **Gates:**
  - **G-FACTORY[defs]:** `buildFactoryPresetDefs()` = 9 defs, names = table names in order, exactly 20 keys each (every id but `output_level`), values = recipe targets else defaults.
  - **G-PRESET-USER:** in-memory module-format JSON (`{ "parameters": { id: normalized }, "version", "plugin" }`) from a random state with `output_level` 0.9 → the 20 parameters match within 1e-6 and `output_level` is untouched; unknown keys ignored; malformed values (string, array, object, `null`) → that parameter to its default; missing parameters → defaults; 1.7 → clamped 1.0; `parameters` absent / not an object / a top-level non-object → `false`, nothing changed. **NC:** a test-local apply that also copies `output_level` fails the `output_level` arm → `FAILS as designed`.
  - **G-PRESET-STATE:** fresh instance → `{ "Init - Additive Build", "init", factory, false }`; `applyFactoryPreset ("driveSweep")` → name "Drive Sweep", id `driveSweep`, revision bumped, unmodified; nudge `position` +0.01 → modified; re-apply → unmodified; change only `output_level` → still unmodified; `getStateInformation` → fresh instance `setStateInformation` → "Drive Sweep", unmodified; a restored user name whose file is missing → name kept, unmodified.
  - **G-PRESET-IMPORTED:** publish a bank through the real import path; snapshot `getImportedBankSnapshot().get()` and the `IMPORTED_BANK` data string from `getStateInformation`; apply a user JSON with `bank` = 1.0, then one with `bank` = Drive → pointer and string unchanged, bank index 5 then 4. **NC:** apply through a module-style XML restore (`setStateInformation` of a state without the child) → the pointer arm fails → `FAILS as designed`.
  - **state-check P11 `currentPreset-roundtrip`:** (a) set `currentPreset="Bright Pad"` on a saved state's XML → fresh instance restore → `getPresetName() == "Bright Pad"`, and its own `getStateInformation` carries it; (b) property removed → `""`; (c) hostile `"../x" + U+202E + "y"` → no `/`, no bidi; P0–P10 unchanged → **12 PASS**.
- **Done when:** Task 9 is green.

#### Task 8 — C++ critic fixes: Stage 3 W4 poll, W5, N1, N13  *(gui-agent, dispatch B)*
- **Files:** `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.{h,cpp}`, `tests/viz-check/main.cpp`.
- **Depends on:** Task 7.
- **Steps:**
  1. **W4 (D-AL):** public `void pollBankForImportStatus()`; `std::atomic<int> lastStatusBank`, seeded in the ctor (`getSelectedBankIndex()`) and as the last statement of `setStateInformation`; called first in `timerCallback` (`:952-956`), outside its sweep lock (it takes `bankStateLock` itself).
  2. **W5 (D-AM):** `std::array<bool, 128> uiHeld {}`; `handleUiMidi` (`:709-724`) updates it after the clamp; public `void releaseUiHeldNotes()`. Editor dtor (`PluginEditor.cpp:360-363`): `processorRef.releaseUiHeldNotes(); processorRef.closeStepKnobGestures();` then `stopTimer()`.
  3. **N1 (D-AP):** editor members `std::unique_ptr<juce::FileChooser> importChooser; bool importDialogInFlight = false;` (above the relay block). `importAudio` (`:241-262`): `complete (true)`; in flight → return; set the flag; assign the member; `launchAsync`. The callback clears **only** the flag, first, above every return, then the SafePointer bail; it never resets the chooser.
  4. **N13 (D-AP):** processor `bool stepKnobGesture (const juce::String& id, int phase, int index)` (phase 0 begin / 1 move / 2 end; allow-list `bit_depth`, `lfo_div`; per-id open flag) + `void closeStepKnobGestures()`. The `stepKnobDrag` native forwards `(id, phase, index)` with type checks.
- **Gates:**
  - **G-S3W4-ERRCLEAR:** fresh instance (bank 0) → `importFromMemory` with a 2047-sample WAV → poll until `error / tooShort` → `pollBankForImportStatus()` with no bank change → the error **persists** (control arm) → set bank 1 → poll → state `idle` and `getImportStatusVersion()` bumped. **Restore arm:** `setStateInformation` with `bank` = 5 and a `version="2"` `IMPORTED_BANK` child → `error / unsupported` → poll → persists (seeding) → bank 0 → poll → `idle`. **Sensitivity:** the bank change without a poll leaves the error (the poll is what clears it).
  - **G-S3W5-UIHELD:** `handleUiMidi (60, true, 0.8f)` → render 100 ms → `releaseUiHeldNotes()` → render 250 ms (default amp release 0.2 s + 50 ms) → `getSoundingVoiceCountForTesting() == 0`; a second `releaseUiHeldNotes()` queues nothing. **NC:** the same without the release → still sounding → `FAILS as designed`.
  - **G-S3N13:** a gesture-counting listener on `bit_depth`: begin → move 3 → move 7 → move 12 → end → exactly 1 begin + 1 end, final index 12; a move without a begin changes nothing; id `position` → false, 0 gestures; begin + `closeStepKnobGestures()` → exactly 1 end; a double begin → still 1 begin. **NC:** a test-local per-detent complete gesture (the `ComboBoxState` path, `juce_ParameterAttachments.cpp:425-434`) → 3 begin / end pairs → `FAILS as designed`.
  - **W1, N1:** code inspection here; hands-on in Task 14 (the editor TU is not in a harness).
- **Done when:** Task 9 is green.

#### Task 9 — Preset tracer gate: the C++ path end to end on the real build  *(orchestrator)*
- **Do:**
  - R-DBG (the CMake change reconfigures; it copies `preset-manager.js` into `Source/ui/public/modules/` and compiles `PluginEditor.cpp`, proving `BinaryData::presetmanager_js`), R-RUN (state 12 PASS), R-GOLD (Part 1 allowlist), R-UNDEF (now includes `PresetRecipes.h` and the module include path from the compile commands).
  - R-STATIC S1–S13 (S3 against the 5 existing `data-preset` ids; S14 page unchanged).
  - C++ native count: `grep -c 'withNativeFunction ("' Source/PluginEditor.cpp` = 20.
  - Record the module-header warning count from `build-dbg.log` (logged upstream, not gating).
  - `node scripts/boot-all-uis.js --plugin O-simpleWavetable --strict-tips` still green (the page is unchanged; proves the editor's resource provider still serves every URL).
- **On failure:** gui-agent dispatch B with the output.
- **Depends on:** Tasks 6–8.
- **Done when:** the build compiles the module + `presetmanager_js`; viz-check (G-LESSON 9 ids, G-FACTORY, G-S3N5, G-PRESET-*, G-S3W4-ERRCLEAR, G-S3W5-UIHELD, G-S3N13) and state-check (12 PASS) green with every NC firing; R-GOLD on the allowlist; S1–S13 clean; boot-all-uis still 0 DEAD.

#### Task 10 — Page: preset panel, wiring, lesson coupling, page critic fixes, wheel probe  *(gui-agent, dispatch C)*
- **Files:** `Source/ui/public/index.html`; create `tests/ui-probes/wheel-gesture-probe.mjs`.
- **Depends on:** Task 9 green.
- **Steps:**
  1. **Markup + CSS (D-AI):** in `.keyboard-panel` (`index.html:676`, markup `:1039-1050`) add `<section class="preset-panel" id="preset-panel">` after the keyboard, CSS `flex: 0 0 352px`; the keyboard stays flex (→ 556 px). Static markup (strict-tips): `#preset-label` (`data-i18n="label.presets"`, width 88), `#preset-save` (74) and `#preset-delete` (72) in the `.tour-btn` look, `#preset-prev` / `#preset-next` (`.oct-btn`, 22 × 22, `data-i18n-aria`), `select#preset-select.combo` (296, 13 px), `#preset-popover` (`hidden`; `#preset-pop-title`, `input#preset-name` with `data-i18n-placeholder="preset.namePlaceholder"`, `#preset-pop-ok` (74, label switches between the literal keys `preset.save` / `preset.delete`), `#preset-pop-cancel` (62), `#preset-pop-msg` nowrap). Each pin carries a comment with its three measured widths (en / fr / zh); no `title=`.
  2. **`initPresets (Juce)`** (async; called from `init()` `:2833-2860` in its own try / catch): dynamic `import('./modules/preset-manager.js')` (failure → disable the panel); natives `getPresetCatalog`, `loadPreset`, `savePreset`, `deletePreset` via `Juce.getNativeFunction`; `new PresetManager({ getNativeFunction: Juce.getNativeFunction, prevButton, nextButton, onPresetChanged, onPresetListUpdated })` (D-AI). `presetState` listener registered in `initBackendEvents` (`:2273-2287`).
  3. **Select / load:** options = `FACTORY_LABELERS` (literal map, one id per line: `init` → `preset.init`, `steppedSmooth` → `preset.steppedScan`, `smoothScan` → `preset.smoothScan`, `aliasDemo` → `label.lessonAliasDemo`, `driveSweep` → `label.lessonDriveSweep`, `vowelPad` → `label.lessonVowelPad`, `pulseNarrowing` → `preset.pulseNarrowing`, `ppg8bit` → `label.lessonPpg8bit`, `ppg4bit` → `preset.ppg4bit`), a disabled `────` separator, user names via `setRaw`; option values = file names; re-label on language change. `change` → `loadPreset (value)`. `onPresetChanged (name)` and the select path apply `LESSON_OCTAVE` when the loaded name is the Alias Demo file name (click-driven, D-AG).
  4. **Save flow:** Save → popover (`preset.saveTitle`, input focused, OK = `preset.save`); Enter / OK: trimmed empty → `preset.errEmpty`; equals a factory file name or any factory **display** label in any of the 3 languages (case-insensitive) → `preset.errFactoryName`; equals an existing user name (catalog, case-insensitive) and not yet confirmed → `preset.confirmReplace {name}`, OK again saves; else `savePreset (name)` → true → close + refresh the catalog; false → `preset.errSave`. The keyboard handler already ignores `INPUT` targets (`:2565-2566`).
  5. **Delete (page-owned, D-AI):** enabled iff `presetState.factory === false` and the name is non-empty; click → popover `preset.confirmDelete {name}` (OK = `preset.delete`) → `deletePreset (presetState.name)` → refresh.
  6. **`presetState` handler (D-AG / N10):** select shows the factory option by file name, the user option, a raw `—` when unnamed, or a transient raw option when the name is not in the catalog; `.preset-panel.modified` toggles the dot; `.tour-btn[data-preset]` gets `active` iff its id matches and `!modified`; the caption follows `CAPTION_LABELERS[id]`, else `tour.hint`. Delete the click-time `.active` toggle in `initLessons` (`:2439`); the click still calls `applyFactoryPreset` and applies `LESSON_OCTAVE`.
  7. **W3 + W3b (D-AO):** one helper (e.g. `makeWheelAccumulator`) replaces the step math of the continuous knob (`:1344-1356`), the stepped knob (`:1477-1490`) and the bank stack (`:2349-2360`). Stepped knobs keep one `setChoiceIndex` per step; the continuous knob and stack keep their burst gesture through N8's helpers.
  8. **N8 (D-AP):** `gestureBegin (id)` / `gestureEnd (id)` refcount; every `sliderDragStarted` / `sliderDragEnded` in `bindKnob` (`:1261-1367`), `nudge` (`:1237`), `resetToDefault` (`:1243`) and `bindBankDrag` (`:2289-2362`) goes through them.
  9. **N13 (D-AP):** in `bindStepKnob` (`:1414-1498`), pointerdown → `stepKnobDrag (id, 0, idx)`; move → local `dragIdx` for the visual + `stepKnobDrag (id, 1, idx)`; pointerup / cancel / lostpointercapture → `stepKnobDrag (id, 2, idx)`. Arrows, wheel, dblclick unchanged.
  10. **N9:** `renderImportError` (`:2225-2232`) gains `else if (importError === 'unsupported') setLabel (el, 'import.err.unsupported');` (literal key). **N12:** `fmtDb` (`:1098`) returns `'−inf'` (U+2212). **W5:** in the `visibilitychange` listener (`:2282`), `if (document.visibilityState === 'hidden') allNotesOff();`. **W2 mapping (D-AK):** at `:2422`, keep a specific error the processor already reported for this drop; `generic` only when none arrived.
  11. **Probe `tests/ui-probes/wheel-gesture-probe.mjs`** (R-WHEEL; built on `scripts/serve-ui.js` + the generic stub, mirroring `scripts/measure-ui.js`; accepts `--root <dir>`; exit 0 / n failures / 77 no Playwright). Read values through the stub's relay state, never DOM text; if the stub does not record slider drag events, wrap `window.__JUCE__.backend.emitEvent` in an init script to count them. Arms:
      - **stepped** (`bit_depth`): 50 pixel events of `deltaY` −4 at 8 ms (one 200 px burst) → moves ≤ 3 detents; one notch (`deltaMode` 1, 3 lines) → exactly 1.
      - **bank stack:** the same flick → ≤ 3 frames; one notch → 1 frame.
      - **continuous** (`lfo_depth`; W3b): the same flick → ≤ 3 nudges; one notch → 1 nudge.
      - **N8:** a stack wheel burst, then a hero-knob drag starting within 250 ms → `position` drag-started / ended strictly alternate.
      - **N13:** a 3-detent pointer drag on `bit_depth` → `stepKnobDrag` calls = begin, ≥ 1 move, end, and 0 `bit_depth` combo value events during the drag.
      - **Liveness:** every arm moves its control at least once.
- **Must not:** touch the 21 binding tables, any existing pin, `Source/ui/public/modules/**`; use `window.__JUCE__` for natives; add `title=`.
- **Done when:** Task 12's gates pass.

#### Task 11 — i18n + gate fixtures  *(gui-agent, dispatch C)*
- **Files:** `Source/ui/public/js/i18n.js`, `tests/i18n-states.json`; create `tests/ui-stub/generic-overrides.json`.
- **Depends on:** Task 10.
- **Steps (glossary first — memory `pattern_parallel_localizers_need_glossary_first`; fr `scripts/i18n-fr-glossary.js:81-92, 385-397`; zh `scripts/i18n-zh-glossary.js:124, 137-138, 191-218`):**
  1. **New entries:** transcribe RESEARCH A §3's "Labels", "Factory display names" (5 new keys) and "Tooltips" tables verbatim — en; fr `reviewed: false`; zh-Hans `reviewed: 'mt'`. Includes `label.presets`, `preset.save / delete / cancel / saveTitle / namePlaceholder / errFactoryName / errEmpty / errSave / confirmReplace / confirmDelete`, `aria.presetPrev / presetNext`, `import.err.unsupported` (N9), `preset.init / steppedScan / smoothScan / pulseNarrowing / ppg4bit`, and the TIP entries `presetSelect / presetPrev / presetNext / presetSave / presetDelete / octDown / octUp`. fr typography: U+00A0 before `? : ; !` and inside `« »`, U+2019 apostrophes; no `sauver` / `sauvegarder` / `lire`.
  2. **Drive Sweep (PLAN-signoff 1):** rewrite `lessonDriveSweep.b` (`i18n.js:396-405`) and `tour.caption.driveSweep` (`:566`) in all 3 languages. Drafts:
     - en b: "The envelope sweeps each note back through the Drive bank: every note strikes fully driven and relaxes to a clean sine as the envelope decays, like turning a drive knob down." · en caption: "Each note strikes driven and relaxes back to a sine as the envelope decays."
     - fr b: "L’enveloppe fait retraverser la banque Drive à chaque note : chaque note attaque pleinement saturée, puis revient à une sinusoïde propre à mesure que l’enveloppe décroît, comme si l’on baissait un bouton de saturation." · fr caption: "Chaque note attaque saturée, puis revient à une sinusoïde à mesure que l’enveloppe décroît."
     - zh b: "包络让每个音符反向扫过 Drive 波表库：每个音符起音时过载最强，随着包络衰减回到干净的正弦波，就像把过载旋钮调低。" · zh caption: "每个音符起音时过载最强，随着包络衰减回到正弦波。"
     - The fr caption must measure ≤ 480 px (the row leaves 492; `research-probes/A/measure.js`).
  3. **W2 copy (D-AK):** `import.err.tooLarge` (`:469`) → en "too large to drop — use the Import button"; fr "trop volumineux pour un dépôt — utilisez le bouton Importer"; zh "文件过大，无法拖放——请使用导入按钮". Each ≤ 240 px (`.src-meta` content, 10.5 px italic).
  4. **N12:** the three "(−inf)" tip bodies (`:366-371`); EXEMPT (`:589`) → `['−inf', 'typographic minus, matching the page's −6.0 dB; the host string stays ASCII -inf']`.
  5. **TIP_BINDINGS +7** (`:593-639`): `['#preset-select','presetSelect']`, `['#preset-prev','presetPrev']`, `['#preset-next','presetNext']`, `['#preset-save','presetSave']`, `['#preset-delete','presetDelete']`, `['#octDown','octDown']`, `['#octUp','octUp']` (N11). Remove any LABELS key that ends up unused (check-i18n [15]).
  6. **`tests/i18n-states.json` +8 states** (appended; the walk is cumulative, so each popover state ends with an Escape eval):
     1. `__stubEmit('presetState', { name: 'Init - Additive Build', id: 'init', factory: true, modified: false })` (widest fr name, 140.3 px)
     2. the same with `modified: true` (dot)
     3. click `#preset-save` (title, placeholder, OK / Cancel)
     4. type "Alias Demo" + submit (`preset.errFactoryName`)
     5. `__stubEmit('presetState', { name: 'Bright Pad', id: '', factory: false, modified: false })`, then click `#preset-delete` (`preset.confirmDelete` with "Bright Pad")
     6. `__stubEmit('importStatus', { state: 'error', filename: 'future.wav', frames: 0, error: 'unsupported' })` (N9)
     7. `__stubEmit('importStatus', { state: 'error', filename: 'huge.wav', frames: 0, error: 'tooLarge' })` (W2 copy)
     8. `__stubEmit('presetState', { name: 'Drive Sweep', id: 'driveSweep', factory: true, modified: false })` (lit lesson + new caption)
  7. **`tests/ui-stub/generic-overrides.json`** (mechanism `scripts/ui-stub/generic-juce-stub.js:411-420`, README `:108-126`): a `_why` line + `natives`: `getPresetList` = the 9 file names + "Bright Pad"; `getPresetCatalog` = `{ factory: [{ name, id } × 9], user: ["Bright Pad"] }`; `getCurrentPreset` = "Init - Additive Build".
- **Gates (Task 12):** R-UISTATIC; R-UIPW with the pins panel 352, label 88, Save / OK 74, Delete 72, Cancel 62, select 296, arrows 22, popover 352 × 92, `.src-meta` 240; the measured en / fr / zh widths logged per pin (memory `pattern_language_width_pins_content_sized_boxes_move`).
- **Done when:** check-i18n 0 FAIL, fr CLEAN and zh lint (run by the agent) are green, every new key has 3 languages, and Task 12's R-UIPW passes all 16 states.

#### Task 12 — Part 1 regression sweep  *(orchestrator; fixes via gui-agent dispatch D or B)*
- **Do:**
  - R-STATIC S1–S14, R-BRIDGE (20 / 20 natives, 4 events, `data-preset` ⊂ table, `FACTORY_LABELERS` = 9 table ids, relays 13 / 6 / 2), R-DBG, R-RUN, R-GOLD (Part 1 allowlist), R-UNDEF.
  - R-UISTATIC (zh count recorded), R-UIPW (16 states), R-WHEEL **and its Stage 3 page negative control**.
  - Page statics: `grep -c '100v[hw]' Source/ui/public/index.html` = 0; `grep -c 'user-select: *none'` ≥ 1; no `title=` added.
  - Optional (not gating): `node scripts/measure-ui.js --plugin O-simpleWavetable --mode box` into `$SCRATCH` for the visual checkpoint notes.
- **On findings:** fix only real findings (page / i18n → gui-agent D; C++ → gui-agent B) with the gate output; re-run the affected gates. Never "fix" a gate-tree artifact by editing the page. No exit 77 is ever recorded as a pass.
- **Depends on:** Tasks 10–11.
- **Done when:** every check above exits 0 (none 77), R-BRIDGE prints nothing with 20 natives and 4 events, R-WHEEL passes while its Stage 3 page NC fails, and R-GOLD shows only allowlisted hunks.

#### Task 13 — Build, install, host-validate, null test, commit Part 1  *(orchestrator)*
- **Do:**
  1. R-INSTALL (VST3 + AU + explicit Standalone; freshness `presetState`; `nm` 0 `ForTesting`; auval; pluginval VST3 + AU strictness 10, editor tests on).
  2. R-NULL against the Stage 3 snapshot (D-AT expectations).
  3. R-STATIC S13 (MSVC greps after Parts A and B; RESEARCH C §3.1 watch item).
  4. **STATUS.md:** `status: stage_4_part1_complete_visual_pending`; `current_phase: execute`; `next_action: user_visual_checkpoint_then_plugin_execute_stage_4`; `last_updated`; `s4_part1_scratch: <$SCRATCH path>` (D-AT, contradiction 14); a "Stage 4 execute Part 1" line under Completed So Far with the key gate values (G-MONO-WHEEL, G-LEGATO-XF / ALIAS, G-MIDI-FLOOD, G-UNPREPARED, G-FACTORY, R-GOLD diffs, null-test verdict, zh count, auval / pluginval).
  5. **R-COMMIT**, Part 1. Message: `feat(O-simpleWavetable): Stage 4 Part 1 - critic fixes (S2 W3/W4, notes 3/4/8; S3 W1-W5, N1/N4/N5/N7-N13) + preset manager (FUNC-08)`. Body: D-AA…D-AQ as applied; every gate + NC with values; R-GOLD named diffs; null-test verdict; auval / pluginval.
- **Depends on:** Task 12 all green.
- **Done when:** auval SUCCEEDED (2 known warnings), pluginval VST3 + AU s10 SUCCESS / 0 FAILED, the Standalone is fresh, R-NULL matches D-AT exactly, and the Part 1 commit passes R-COMMIT step 6.

#### Task 14 — **Visual checkpoint**  *(human-verify, Taylor; BLOCKING)*
- **Surface:** the Release **Standalone** `build/plugins/O-simpleWavetable/O-simpleWavetable_artefacts/Release/Standalone/O-simpleWavetable-dev.app`, audio output **on** (Options → Audio Settings).
- **Script:**
  1. **Panel layout** in en, fr, zh-Hans: nothing clipped; the panel's left edge lines up with the Amp + Output group above it; the dot appears after any edit.
  2. **Keyboard at 37 px white keys (PLAN-signoff 3):** comfortable with the mouse; A–K / W–U and Z / X still work; ◀ / ▶ octave buttons now show tooltips (N11).
  3. **Browser:** select each of the 9 factory presets — Output Level never moves; Alias Demo moves the keyboard to octave 6; ◀ / ▶ walk factory then user presets; save "Bright Pad" → it appears; save "Alias Demo" → refused; save "bright pad" → Replace? confirm; Delete is disabled on factory presets; delete "Bright Pad" → confirm → gone; a preset saved on the Imported bank with no import shows the empty-Imported prompt.
  4. **Lesson highlight (N10):** click a lesson → lit + caption; nudge any knob → unlit + dot; reselect → lit again; close and reopen the editor → the same state returns.
  5. **Wheel (W3 + W3b):** a trackpad flick on Bit Depth moves about 3 detents at most; one mouse notch = one detent; the same on the bank stack and on a continuous knob (LFO Depth).
  6. **Behaviour:** a stepped-knob drag is smooth (N13); a fast double-click on Import opens one dialog (N1); close the editor while holding a keyboard note → the note stops (W5); reopen the editor while an import is running → the status is not stuck (W1).
  7. **Drive Sweep:** the new caption matches what you hear (strikes driven, relaxes to a sine) — a quick check only; full listening is Task 23.
- **Known until Part 2:** QUAL-04 gates, PERF-02, CI, VERSION / CHANGELOG / CODE_REVIEW, full listening.
- **Outcomes:** sign-off → Task 15. A finding → fix within Part 1 (owning dispatch with Taylor's notes), re-run Tasks 12–13 (fix commit), repeat this checkpoint.
- The execute phase **STOPS here** and presents this script. **Handoff:** Step 1 `/clear`; Step 2 `/plugin-execute O-simpleWavetable 4-polish` (after the visual sign-off).

---

### Part 2: gates, perf, release, CI, listening (Tasks 15–24; resume after the Task 14 sign-off)

#### Task 15 — Part 2 pre-flight + Part 1 baselines  *(orchestrator)*
- **Do:**
  - STATUS shows the Task 14 sign-off; record Taylor's notes; `status: stage_4_part2_in_progress`.
  - `main`; `git status --short -- plugins/O-simpleWavetable` empty; `git merge-base --is-ancestor <Part 1 commit> HEAD`.
  - **Re-baseline in this session** (contradiction 14): `BASE=$SCRATCH`. R-DBG + R-RUN on the Part 1 tree → `$SCRATCH/p1-{bank,dsp,dsp-alloc,mod,import,state,viz}.log` (Part 2's R-GOLD baseline, renamed `base-*` for the recipe).
  - **Part 1 binary snapshot:** the installed `-dev` VST3 must equal the Part 1 artefact (`diff -rq`, `strings | grep -c presetState` ≥ 1) → `cp -R` into `$SCRATCH/p1-baseline/`. Copy the R-NULL scripts from `s4_part1_scratch` (STATUS) or rebuild them; render the Part 1 snapshot → `p1.npz`.
  - If the Task 1 Stage 3 snapshot still exists at `s4_part1_scratch`, keep its path for an optional repeat of the Stage 3 comparison in Task 20.
- **Depends on:** Task 14.
- **Done when:** 7 `p1-*` logs satisfy R-RUN (state 12 PASS), the Part 1 snapshot + `p1.npz` exist, and STATUS records the sign-off.

#### Task 16 — QUAL-04 measured gates  *(dsp-agent, dispatch E)*
- **Files:** `tests/dsp-check/main.cpp`.
- **Depends on:** Task 15.
- **Do (D-AR; RESEARCH C §1, probes `research-probes/C/q4probe*.py`, `q4alias.py`, `q4bits.py`):** add `gateQ4Step()`, `gateQ4Alias (Q2Analyzer&)`, `gateQ4Bits()` and wire them into `main()` after `gateFull()` (`dsp-check/main.cpp:1478`). Recipes come from `applyFactoryPreset (id)` on the Rig's processor **before** `prepareToPlay`, then only the contrast parameter is overridden. Print every measured value / table.
- **Gates:**
  - **G-Q4-STEP:** fs = 56 320 Hz, A4 (MIDI 69) vel 100 at sample 0 (inc = 2^-7, period P = 128 exactly); render ≥ 3.9 s; `applyFactoryPreset ("steppedSmooth")` (Off) vs `("smoothScan")` (On). `D_k = Σ_n (c_{k+1}[n] − c_k[n])²` over cycles; W = cycles with 1.0 s ≤ t_k ≤ 3.8 s (after the amp decay settles, before the Saw LFO reset at 4.000 s; the LFO phase resets in `prepare()`). **R = S_Off / S_On ≥ 10** [54.3]. Liveness: Off — the count of k ∈ W with D_k > 1e-3 · max D equals the number of `wt::latchFrame (raw)` changes in W (21 ± 1); On — S_On > 0 and ≥ 90 % of W has D_k > 0; both renders RMS > 0.05. **NCs:** (1) R(Off, Off) = 1.0 fails R ≥ 10; (2) LFO depth 0 → Off step count 0 → the liveness term fails (its own NC line); (3) record-only: R with W stretched across the saw reset (≈ 3–5).
  - **G-Q4-ALIAS:** `applyFactoryPreset ("aliasDemo")` (Drive, Pos 1, BL Off), then `bandlimit` per cell; MIDI 84 / 96 / 108 × 44.1 / 48 / 88.2 / 96 kHz × BL Off / On; skip 50 ms, then 2^17 samples (`q2Proc`-style, `:627-633`). Metric: `A_full` = today's `inhDb` (`:467-530`); `A_8k` = the same with inharmonic bins restricted to ≤ 8 kHz. Gated cells: C7 + C8 at all 4 rates, C6 at 44.1 / 48 (C6 at 88.2 / 96 record-only: −60.0 / −64.4). Terms: (1) **A_8k(Off) ≥ −50 dB** on every gated cell [worst −41.3, C7 @ 96k]; (2) **A_8k(Off) − A_8k(On) ≥ 40 dB** [min 69.1]; (3) **A_full(On) ≤ −90 dB** on all 12 cells [worst −107.3]; (4) max harmonic bin > −40 dBFS in every render. **NCs:** NC1 the Stage 3 recipe (Sine→Saw, Pos 1, BL Off) at C6 44.1 kHz fails term 1 [−101.9]; NC2 On vs On fails term 2.
  - **G-Q4-BITS:** two Rigs identical except `bit_depth` (index i = 1..14 → bits 17 − i); Sine→Saw Pos 1.0 (frame 32), A4, 48 kHz, amp sustain 1.0, no LFO; `SNR(b) = 10·log10 (Σ y_Full² / Σ (y_b − y_Full)²)` over t ∈ [0.25, 1.0] s, sample-aligned. Terms: (1) every SNR finite; (2) each step SNR(b) − SNR(b−1) ∈ **[4.5, 7.5] dB** for b = 16..4 [5.70–6.27]; (3) LS slope over 16..3 = **6.02 ± 0.6 dB/bit** [6.029]; (4) SNR(3) ≤ 25 dB [16.5] and SNR(16) ≥ 85 dB [95.1]. **NCs (analyzer side):** (a) 14 copies of the Full render → +inf fails term 1; (b) the reversed SNR vector fails term 2; (c) a 1-sample offset fails terms 2 and 4. Optional record-only arm: the 8-bit PPG recipe (irregular but monotonic).
- **Done when:** Task 18 shows the three gates PASS, every NC prints `FAILS as designed`, and the measured values sit within the RESEARCH bands above.

#### Task 17 — PERF-02 harness `tests/perf-check`  *(dsp-agent, dispatch E)*
- **Files:** create `tests/perf-check/main.cpp`; edit `CMakeLists.txt` (inside `if(OUARICON_BUILD_TESTS AND APPLE)`, `:113-143`).
- **Depends on:** Task 16.
- **Do (D-AS; RESEARCH C §2; prototype `research-probes/C/perf/perf.cpp`):**
  - CMake: `ouaricon_add_processor_console(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source ${CMAKE_CURRENT_SOURCE_DIR}/tests/perf-check/main.cpp perf-check)` with **no** `target_compile_definitions(... OSIW_TEST_HOOKS=1)` and a comment saying why (the timed code must equal the plugin's).
  - Per cell: a fresh processor; params set before `prepareToPlay`; 16 note-ons (MIDI 84..99, vel 100) in block 0; 1 s warm-up unmeasured, then 4 s measured; per block `CLOCK_THREAD_CPUTIME_ID` delta × fs / bs; median, p99, max; each cell 3× → best-of-3 median. Cells: {44.1, 48, 88.2, 96} kHz × bs {64, 512} × {steady worst patch, `bandlimit` toggled every block}; the 1-voice steady case at 96 kHz / 512 for SCALE. Print a table like RESEARCH C §2.1 A ("CPU per block as a fraction of the real-time budget").
  - Gates: **G-PERF02-LIVE** (output finite; RMS(16) / RMS(1) ≥ 3 [4.37]); **G-PERF02-SCALE** (cost(16) / cost(1) ≥ 4 [10.7]); **G-PERF02-STEADY** (median **and** p99 ≤ 25 % at every rate × {64, 512} [worst p99 1.02 %]); **G-PERF02-XFADE** (bs 512 median ≤ 25 % at every rate [1.97 % @ 96k]). bs 64 crossfade storms and bs ≤ 32 are printed, not gated.
  - **Duty witness:** `getrusage` user / wall over the run ≥ 80 %, else print `SKIPPED (machine contended, duty N %)` and exit **77** (never recorded as a pass). Exit 0 = all gates pass; 1 = a gate failed.
- **Done when:** Task 18's Release run exits 0 with the four G-PERF02 lines PASS and the table printed.

#### Task 18 — Part 2 gate run: offline, Release perf, pedalboard cross-checks  *(orchestrator)*
- **Do:**
  - R-DBG, R-RUN (dsp-check now carries G-Q4-*), R-GOLD (Part 2 allowlist vs `p1-*`), R-STATIC S1–S2.
  - **Release tree** `$SCRATCH/build-oswt-rel`: the R-DBG configure with `-DCMAKE_BUILD_TYPE=Release` (same SKIP recipe; never the shared `build/`), then `cmake --build "$SCRATCH/build-oswt-rel" --target O-simpleWavetable-perf-check` (background; it also builds the plugin shared code). `nm <perf-check bin> | grep -c ForTesting` = 0. Run it on a quiet machine: exit 0 → record the table; exit 77 → re-run later, never a pass.
  - **pedalboard PERF cross-check** (installed Part 1 VST3; DSP unchanged in Part 2): `research-probes/C/perf_pb.py` copied to `$SCRATCH`, steady worst patch only, ≥ 4 s per rate, bs 512, `reset=False` after a 0.5 s settle; params by `parameters[name].raw_value` (keys RESEARCH C §2.3). Pass: within 2× of perf-check's steady median and < 25 % at every rate [expected 0.72 % @ 96k].
  - **pedalboard QUAL-04 cross-check:** `research-probes/C/q4_pb.py` + `q4_pb_a.py` on the installed VST3. Pass: within ±1 dB (ALIAS, BITS) / ±10 % (STEP R) of the dsp-check G-Q4 values.
- **On failure:** dsp-agent dispatch E with the output; thresholds frozen.
- **Depends on:** Tasks 16–17.
- **Done when:** dsp-check ALL PASS with G-Q4-*; R-GOLD vs `p1-*` clean; perf-check exit 0 (≤ 25 % everywhere, expected ≈ 0.75 % / 1.97 % at 96 kHz); both pedalboard cross-checks inside their bands.

#### Task 19 — Release docs + contract notes  *(orchestrator)*
- **Files:** `CMakeLists.txt` (`:14`), create `CHANGELOG.md`, create `CODE_REVIEW.md`, `.planning/research/ARCHITECTURE.md`, `.planning/STATUS.md`.
- **Do:**
  1. **VERSION:** `CMakeLists.txt:14` → `VERSION "1.0.0"`. `grep -c PLUGIN_VERSION CMakeLists.txt` = 0. (The factory sentinel re-materializes once at the new version — expected.)
  2. **CHANGELOG.md** (D-AV): exactly one `## [1.0.0] - <YYYY-MM-DD>` heading. Under it, `###` subsections only: Added (the synth, 5 banks + import, the Field Guide UI, lessons, the preset manager with 9 factory presets); Fixed during Stage 4 (each critic item, one line, with its gate); Known limitations (the by-design list; "at 88.2 / 96 kHz the Alias Demo is clearly audible only in the top octave"); Development history (Stages 1–4 as `####` or bullets — never `##`). Dry run: `awk '/^## \[1.0.0\]/{flag=1; next} /^## \[/{flag=0} flag' plugins/O-simpleWavetable/CHANGELOG.md | wc -l` > 0; `grep -cE '^## \[' CHANGELOG.md` = 1; `grep -cE '^## v?[0-9]|^## Version' CHANGELOG.md` = 0 (memory `pattern_changelog_heading_must_be_bracket_form_and_partial_fix_is_worse`).
  3. **CODE_REVIEW.md** at `plugins/O-simpleWavetable/CODE_REVIEW.md` (memory `project_code_review_artifact_path_canon`): suite frontmatter (`phase: stage-4-polish`, `reviewed: <date>`, `depth`, `files_reviewed_list` = every `Source/` file + the test drivers, `findings: { critical: 0, warning: 10, info: 21, total: 31 }`, `status: resolved`, `open_findings: none`). Body: the finding list with IDs (D-AV), then a resolution-log table `| Finding | Status | Where |`:
     - "✅ Resolved in **v1.0.0**" + gate: WR-03 S2 W3 (G-MONO-WHEEL), WR-04 S2 W4 (G-LEGATO-XF / ALIAS / CLICK), WR-06…WR-10 S3 W1–W5, IN-03 / IN-04 / IN-08 (notes 3 / 4 / 8), IN-09 N1, IN-12 N4, IN-13 N5, IN-15 N7, IN-16…IN-21 N8–N13.
     - "✅ Resolved in the Stage 2 gap closure (`850df9b9`)": WR-01, WR-02, WR-05.
     - "Accepted (by design)": IN-01, IN-02, IN-14 (S3 N6) — RESEARCH B §5 wording.
     - "Reviewed, not changed": IN-05, IN-06, IN-07, IN-10 (N2), IN-11 (N3), plus the W4 fold residual and the collector-buffer note — RESEARCH B §5 wording; IN-05 includes the CC64 Mono → Poly side note.
     - "Upstream (not plugin code; not called)": preset-manager v1.0.9 stale `<CustomState>` child (`OuariconPresetManager.h:617-644`) and its apply that resets / sets `output_level` (`:360-376`); `std::map` relying on JUCE's transitive `<map>` (`:217`); any module-header warning count from R-DBG.
  4. **ARCHITECTURE.md:** append a dated "Stage 4 Amendments (2026-10-xx)" section; rewrite nothing else.
     - **19.** Imported × presets = bank choice only; a preset load never touches `IMPORTED_BANK` (D-AC). Supersedes the "replace, not merge" sentence in §State Persistence.
     - **20.** Preset manager: module for files / guards / sentinel / JS only; the plugin's own state + `currentPreset` root property; one apply core (D-AA, D-AB, D-AF).
     - **21.** §A9 as shipped: one table, the fill rule, the 4-bit PPG variant, Alias Demo on Drive, Drive Sweep copy rewritten; supersedes Amendment 17's lesson recipes (D-AD).
     - **22.** Frozen-cycle crossfade keeps the outgoing cycle's own phase and rate (W4); the fold is phase-aligned (D-AJ).
     - **23.** Block-path hardening: byte-capped MIDI chunk fill, the `prepared` guard, the Mono wheel seed at Poly → Mono; drop cap 16 MiB on both sides (D-AJ, D-AK).
     - Recompute `contract_checksums.architecture` in STATUS.
  5. Prepare the PLUGINS.md row text `| O-simpleWavetable | 📦 Installed | 1.0.0 | Synth (Pedagogical Wavetable) | <YYYY-MM-DD> |` (committed in Task 21 via R-COMMIT step 3).
- **Depends on:** Task 18.
- **Done when:** VERSION reads 1.0.0; the CHANGELOG awk dry run and heading census pass; CODE_REVIEW.md lists all 31 findings with a status each and `open_findings: none`; Amendments 19–23 are appended and the STATUS checksum matches the file.

#### Task 20 — Final regression sweep, install, host, null test  *(orchestrator)*
- **Do:**
  - R-STATIC S1–S14, R-BRIDGE, R-DBG, R-RUN, R-GOLD (Part 2 allowlist), R-UNDEF, R-UISTATIC, R-UIPW, R-WHEEL (+ its negative control).
  - R-INSTALL. Version freshness: `plutil -extract CFBundleShortVersionString raw <installed VST3>/Contents/Info.plist` = `1.0.0` (same for the AU and the Standalone).
  - R-NULL: Part 1 snapshot vs installed → all 8 cases bit-identical; Stage 4 rendered twice → bit-identical. If the Stage 3 snapshot is still available, repeat the D-AT comparison (same expectations as Task 13).
  - On findings → gui-agent D / dsp-agent E as owned; re-run.
- **Depends on:** Task 19.
- **Done when:** every recipe above passes (no 77), the installed bundles report 1.0.0, and Part 2 renders bit-identically to Part 1.

#### Task 21 — Commit Part 2 (code + docs + PLUGINS row)  *(orchestrator)*
- **Do:**
  - **STATUS.md:** `status: stage_4_part2_signoff_pending`; `next_action: user_ci_goahead_and_listening_signoff`; a "Stage 4 execute Part 2" line (G-Q4 values, PERF-02 table summary, cross-checks, null verdict, auval / pluginval); the architecture checksum.
  - **R-COMMIT**, Part 2, with step 3 (this row only). Message: `feat(O-simpleWavetable): Stage 4 Part 2 - QUAL-04 + PERF-02 gates, v1.0.0, CHANGELOG, CODE_REVIEW`. After: `git show HEAD:PLUGINS.md | grep '^| O-simpleWavetable'` shows `📦 Installed | 1.0.0`; the foreign staged PLUGINS.md blob handled per R-COMMIT step 7.
- **Depends on:** Task 20.
- **Done when:** HEAD is the Part 2 commit, it touches only `plugins/O-simpleWavetable/**` + the one PLUGINS row, and the main index is resynced per step 7.

#### Task 22 — **COMPAT-02 CI go-ahead**  *(Taylor; BLOCKING; presented together with Task 23)*
- **The executor presents these facts (refreshed, not copied) and STOPS for an explicit answer:**
  1. `git fetch origin && git log --oneline origin/main..main` — the exact list of commits that would be published (23 unpushed at plan time: all O-simpleWavetable plus `18638e14 chore(claude)`; Stage 4 adds its own; **any other session's commit on `main` rides along — list them by name**).
  2. `origin/main` has **0** O-simpleWavetable files (re-check after the fetch): the push publishes the whole plugin source to the public repo (AGPL-3.0, public since 2026-08-03).
  3. The push fires `ui-static-gates.yml` (the 3 static UI gates for **every** plugin); another plugin's red gate would show red on `main`.
  4. The validate-only dispatch runs **build-windows** (VST3, pluginval v1.0.3 strictness 10, editor tests on) **and build-macos** (universal build, **Developer-ID codesign + Apple notarization**, a signed stapled PKG). The macOS configure does not skip sibling plugins, so a broken sibling on `main` can fail that job.
  5. Both jobs upload **signed binaries as workflow artifacts** (`windows-build`, `macos-build`, `pluginval-windows-log`); any signed-in GitHub user can download them from the public repo for the retention period (repo default, ≈ 90 days, unverified).
  6. No tag and no release: `create-release` is skipped when `validate_only` is true (`build-and-release.yml:701-706`). Version label: `1.0.0-validate`.
  - Ask: **"Push `main` and dispatch the validate-only Windows build? (yes / no)"**. Never push or dispatch without an explicit "yes".
- **On "yes"** (orchestrator, under `bash -c`):
  1. `git fetch origin`; re-list `origin/main..main`. If it differs from the list Taylor approved (a new foreign commit) → **stop and re-ask**.
  2. `git push origin main`.
  3. `gh workflow run build-and-release.yml --ref main -f plugin_name=O-simpleWavetable -f version=1.0.0-validate -f validate_only=true`.
  4. `gh run list --workflow build-and-release.yml -L 1` → run id; `gh run watch <id> --exit-status` in the background; poll.
  5. Done → `gh run view <id>` (job conclusions); `gh run download <id> -n pluginval-windows-log -D "$SCRATCH/ci"`; grep `SUCCESS` and `FAILED`.
  - **Pass:** build-windows ✓; the pluginval log ends in SUCCESS with 0 FAILED; create-release skipped; build-macos ✓ (if red only because of a sibling plugin, report it; COMPAT-02 is judged on the Windows job).
  - **Fail:** `gh run view <id> --log-failed`; fix (Windows-only causes: MSVC traps, WebView2) via the owning dispatch; a fix commit; **a re-dispatch needs a new explicit "yes"**.
- **On "no":** record it; COMPAT-02 = "config verified (S7, S13), CI run not authorized" for `/plugin-verify`.

#### Task 23 — **QUAL-04 listening sign-off**  *(human-verify, Taylor; presented together with Task 22)*
- **Surface:** the installed v1.0.0 in the Standalone and / or Logic, audio on.
- **Script:**
  1. **Each factory preset** (◀ / ▶ through all 9): does it isolate its one idea? Listen especially to the D-AD changes: Stepped Scan (Saw LFO 0.25 Hz), Alias Demo (Drive), Drive Sweep (strikes driven, relaxes to a sine — the rewritten copy must match; PLAN-signoff 1), Vowel Pad (Tri 0.1 Hz, full depth), 8-bit PPG (S&H 4 Hz), 4-bit PPG.
  2. **Stepped vs smooth:** Stepped Scan ↔ Smooth Scan on a held note — clearly stepped vs clearly morphing.
  3. **Aliasing vs clean:** Alias Demo, play C6–C8 with Band-limit Off → On (at 44.1 / 48 kHz; at 88.2 / 96 kHz the top octave).
  4. **Bit depth:** 8-bit / 4-bit PPG, sweep Bit Depth Full → 3: grit grows step by step.
  5. **Mono legato octave jump (PLAN-signoff 5):** Voice Mode Mono, Drive bank, Position 100 %, hold C4, press C7 while holding → a smooth 5 ms two-pitch crossfade, no alias burst, no click.
  6. Optional carry-over (Stage 2 / 3): a 17-note steal and a fast Mono retrigger.
- **Outcomes:** sign-off → Task 24. A recipe or tone request → change `PresetRecipes.h` (+ the G-LESSON transcription, + copy if the idea changes) via gui-agent B; re-run Tasks 18 (G-Q4) and 20; delete `~/Library/O-simpleWavetable/Presets/Factory/.factory-version` so the disk copies refresh at the same 1.0.0 (the sound always comes from the table); a fix commit; if CI already ran, ask Taylor whether to re-dispatch (a new "yes").

#### Task 24 — SUMMARY, STATUS, commit, hand off  *(orchestrator)*
- **SUMMARY.md** (`stages/4-polish/SUMMARY.md`): files created / changed; D-AA…D-AW as applied (incl. PLAN-signoff 1–5 outcomes); every gate value + NC margin (Tasks 2–8, 16–17); R-GOLD named diffs; the null-test verdicts (Stage 3 → Part 1, Part 1 → Part 2); the 5 UI gates + R-WHEEL; PERF-02 table; QUAL-04 values + cross-checks; auval / pluginval; CI run id + verdict (or "not authorized"); Tasks 14 / 22 / 23 notes; deviations.
- **STATUS.md:** `status: stage_4_execute_complete`; `current_phase: verify`; `next_action: plugin_verify_stage_4`; phase table execute ✓; a final Completed line.
- **R-COMMIT** (no PLUGINS.md). Message: `docs(O-simpleWavetable): Stage 4 SUMMARY + STATUS (execute complete)`.
- **Hand off:** Step 1 `/clear`; Step 2 `/plugin-verify O-simpleWavetable 4-polish`. Alternatives: `/plugin-status O-simpleWavetable`; (after verify) `/publish O-simpleWavetable 1.0.0`. Then **STOP**. No tag.
- **Depends on:** Tasks 22–23.
- **Done when:** SUMMARY.md exists, STATUS says `stage_4_execute_complete`, the docs commit passes R-COMMIT step 6, and the handoff is shown.

---

## Files to create / modify

All paths are relative to `plugins/O-simpleWavetable/`, except `PLUGINS.md`.

| Path | Action | Task |
|------|--------|------|
| `Source/TestHooks.h` | create | 2 |
| `Source/WavetableBank.h`, `WtVoice.h`, `PositionSmoother.h`, `PositionLfo.h`, `MipmapBuilder.h` | include `TestHooks.h` first (+ W4 in `WtVoice.h`) | 2, 3 |
| `Source/PluginProcessor.h`, `.cpp` | **A:** `prepared`, MIDI byte cap, `wheelNow` seed, `resetDisplayState`, hooks<br>**B:** apply core + N5, preset identity / API, module member, state `currentPreset`, `pollBankForImportStatus`, `uiHeld`, `stepKnobGesture` | 2–4, 6–8 |
| `Source/WavetableImporter.h`, `.cpp` | 16 MiB cap; `isStrippedNameChar` | 4 |
| `Source/PresetRecipes.h` | create | 6 |
| `Source/PluginEditor.h`, `.cpp` | 12 natives, `presetState`, W1, N1, W5 / N13 dtor, resource branch | 7, 8 |
| `Source/ui/public/index.html` | `DROP_MAX_BYTES` (A); panel, popover, preset wiring, lesson coupling, W2 mapping, W3 / W3b, W5, N8, N9, N12, N13 (C) | 4, 10 |
| `Source/ui/public/js/i18n.js` | ≈ 26 new entries, Drive Sweep + tooLarge rewrites, N12, TIP_BINDINGS +7 | 11 |
| `CMakeLists.txt` | `TestHooks.h`, `PresetRecipes.h`, `ouaricon_add_module(... preset-manager)`, binary-data entry, `perf-check` target, VERSION 1.0.0 | 2, 6, 7, 17, 19 |
| `tests/dsp-check/main.cpp` | G-UNPREPARED, G-MIDI-FLOOD, alloc flood arms, G-MONO-WHEEL, G-LEGATO-*; G-Q4-* | 2, 3, 16 |
| `tests/viz-check/main.cpp` | G-VIZ-RELEASE, G-SANITISE, G-DROP-CAPSYNC; G-LESSON re-transcription, G-FACTORY, G-S3N5, G-PRESET-*, G-S3W4-ERRCLEAR, G-S3W5-UIHELD, G-S3N13 | 4, 6–8 |
| `tests/state-check/main.cpp` | P11 | 7 |
| `tests/perf-check/main.cpp` | create | 17 |
| `tests/ui-probes/wheel-gesture-probe.mjs` | create | 10 |
| `tests/i18n-states.json` | +8 states | 11 |
| `tests/ui-stub/generic-overrides.json` | create | 11 |
| `CHANGELOG.md`, `CODE_REVIEW.md` | create | 19 |
| `.planning/research/ARCHITECTURE.md` | dated Stage 4 Amendments 19–23 | 19 |
| `.planning/stages/4-polish/SUMMARY.md` | create | 24 |
| `.planning/STATUS.md` | update | 13, 15, 19, 21, 24 |
| `PLUGINS.md` (this row only, from the HEAD blob) | update | 21 |

**Must not touch:** `Source/ui/public/modules/**` (configure-generated, never committed); `modules/**`; `mockups/`; `REQUIREMENTS.md` (flips belong to `/plugin-verify`), `parameter-spec.md`, `ROADMAP.md`; the root `CMakeLists.txt`, `.gitignore`; `build/CMakeCache.txt`; other plugins (O-simpleFM, O-Gain, O-Reed, O-Bassoon, O-Freeze are dirty); the foreign-staged PLUGINS.md beyond R-COMMIT step 7.

---

## Dependency graph / waves

```
PART 1
Wave 0 [orch]          T1  pre-flight + Stage 3 VST3/AU snapshot + base.npz + 7 base logs + R-UNDEF NC
Wave 1 [dsp-agent A]   T2 note 8/4/3 + S2 W3 -> T3 S2 W4 -> T4 N4 + N7 + W2 cap   (+ every gate + NC)
Wave 2 [orch]          T5  DSP gate: statics, 7 drivers, R-GOLD, R-UNDEF            (DSP tracer proven)
Wave 3 [gui-agent B]   T6 table + apply core + N5 -> T7 module/state/natives/presetState + W1 -> T8 W4 poll/W5/N1/N13
Wave 4 [orch]          T9  preset tracer gate: module -> BinaryData -> table -> apply -> state -> natives
Wave 5 [gui-agent C]   T10 page panel + critic page fixes + wheel probe -> T11 i18n + fixtures
Wave 6 [orch]          T12 full Part 1 sweep (-> gui-agent D / B on real findings) -> T13 install/null/commit P1
Wave 7 [HUMAN]         T14 VISUAL CHECKPOINT   ---- execute STOPS here ----

PART 2  (second /plugin-execute O-simpleWavetable 4-polish)
Wave 8  [orch]         T15 pre-flight + Part 1 re-baseline + Part 1 VST3 snapshot
Wave 9  [dsp-agent E]  T16 G-Q4-* -> T17 perf-check
Wave 10 [orch]         T18 gates + Release perf + pedalboard -> T19 docs -> T20 final sweep/install/null -> T21 commit P2
Wave 11 [HUMAN]        T22 CI GO-AHEAD (-> orch push + dispatch ONLY on "yes")  ||  T23 LISTENING SIGN-OFF
Wave 12 [orch]         T24 SUMMARY/STATUS + commit -> handoff /plugin-verify
```

Every wave is sequential: the dispatches share `PluginProcessor.*`, `CMakeLists.txt`, `index.html` and the test drivers (file-ownership sequencing above). Only Tasks 22 and 23 run side by side (one is a decision plus a CI wait, the other is listening).

---

## Success criteria (goal-backward; all must be TRUE for the verify phase)

| # | Observable truth | Evidence | Task |
|---|------------------|----------|------|
| S1 | **FUNC-08:** 9 factory presets from one §A9 table, each isolating one concept; the 5 lessons are a subset | G-FACTORY (+ skew NC), G-LESSON 9 ids (+ NC), S3 ids ⊂ table, R-BRIDGE `FACTORY_LABELERS` = 9, Task 14 browse, Task 23 by ear | 6, 12, 14, 23 |
| S2 | **FUNC-08:** user presets save / replace / delete / ◀ ▶ in the page; factory names refused; names sanitised | G-PRESET-USER (+ NC), G-FACTORY[defs], R-BRIDGE 20 / 20, i18n states 3–5, Task 14 item 3 | 7, 10–12, 14 |
| S3 | **FUNC-08 constraints:** no preset sets `output_level`; no preset touches `IMPORTED_BANK`; `getNumPrograms() == 1`; the module's XML / apply path never called | output arms of G-LESSON / G-FACTORY / G-PRESET-USER; G-PRESET-IMPORTED (+ NC); P11; S7; S10 | 6–9 |
| S4 | **FUNC-08 / N10:** name, dot, lesson light and caption stay truthful through edits, automation, loads, restores and reopen | G-PRESET-STATE; `presetState` in R-BRIDGE; states 1 / 2 / 8; Task 14 item 4 | 7, 10, 14 |
| S5 | **PERF-02:** 16 voices well inside budget at 44.1–96 kHz, Release, no hooks | G-PERF02-LIVE / SCALE / STEADY / XFADE PASS, exit 0 (not 77); `nm` 0 `ForTesting`; printed table; pedalboard within 2× and < 25 % | 17, 18 |
| S6 | **COMPAT-02:** Windows VST3 with static WebView2 builds and validates | S7 + S13 (config, 0 MSVC carriers); CI: build-windows ✓, pluginval SUCCESS / 0 FAILED, create-release skipped — or "CI run not authorized" recorded | 9, 12, 22 |
| S7 | **QUAL-04 (measured):** each contrast clearly measurable | G-Q4-STEP R ≥ 10, G-Q4-ALIAS (4 terms), G-Q4-BITS (4 terms), all NCs firing; pedalboard ±1 dB / ±10 % | 16, 18 |
| S8 | **QUAL-04 (by ear):** each contrast clearly audible; Mono legato octave jump clean | Task 23 sign-off recorded | 23 |
| S9 | **Regression:** nothing moves that no fix intends | R-GOLD closed allowlist; R-NULL (5 bit-exact + `mono_legato` window 28801–29039 + both positive controls; Part 2 = Part 1 bit-exact; determinism); `--alloc-check` 0; 0 JUCE assertions; auval; pluginval VST3 + AU s10 | 5, 9, 12, 13, 18, 20 |
| S10 | **UI gates green** | check-i18n 0 FAIL; fr CLEAN; zh named with count, 0 findings; check-ui-labels 16 states + pins; boot-all-uis 0 DEAD / late / 404; R-WHEEL + its NC | 12, 20 |
| S11 | **Release:** v1.0.0 documented, untagged | `CFBundleShortVersionString` 1.0.0; CHANGELOG awk dry-run + 1 bracket heading; CODE_REVIEW at the canonical path, `open_findings: none`; PLUGINS row `📦 Installed | 1.0.0`; no tag | 19–21 |
| S12 | **Scoped, consented commits** | every commit touches only `plugins/O-simpleWavetable/**` (+ 1 PLUGINS row in Task 21); `modules/` never in a tree; no tag; push / dispatch only after an explicit "yes" | 13, 21, 22, 24 |
| S13 | **Contracts consistent** | ARCHITECTURE Amendments 19–23; STATUS checksums current | 19 |
| S14 | **Taylor signed off** | Task 14 visual; Task 22 decision recorded; Task 23 listening | 14, 22, 23 |

### Critic-item coverage

| Item | Verdict | Fix | Gate (+ negative control) | Task |
|---|---|---|---|---|
| S2 W1, W2, W5 | resolved in the Stage 2 gap closure | — | CODE_REVIEW rows | 19 |
| S2 W3 Mono wheel after Poly→Mono | fix | `wheelNow` seed | G-MONO-WHEEL (+ seed-off NC); R-NULL `w3_poly_to_mono` | 2, 13 |
| S2 W4 upward-legato crossfade source | fix | `xfPhase` / `xfInc` | G-LEGATO-XF / ALIAS / CLICK (+ keep-rate-off NC, confinement); R-NULL `mono_legato` window, `legato_60_96_drive` | 3, 13 |
| S2 note 1, note 2 | by design | — | CODE_REVIEW | 19 |
| S2 note 3 MIDI scratch growth | fix | byte-capped fill | G-MIDI-FLOOD; alloc flood 0 (+ guard-off ≥ 1 NC) | 2 |
| S2 note 4 pre-prepare | fix | `prepared` | G-UNPREPARED a / b / c (+ guard-off SIGSEGV NC) | 2 |
| S2 notes 5, 6, 7 | log | — | CODE_REVIEW | 19 |
| S2 note 8 `-Wundef` | fix | `TestHooks.h` | R-UNDEF (+ pre-fix 3-header NC) | 1, 2 |
| S3 W1 `uiReady` order | fix | snapshot first | inspection + Task 14 item 6 | 7, 14 |
| S3 W2 96 MB drop freeze | fix | 16 MiB caps + copy + mapping | G-DROP-CAPSYNC (+ 96 NC), G-DROP[b], state 7 | 4, 10, 11 |
| S3 W3 trackpad wheel (stepped + stack) | fix | burst accumulator | G-S3W3-WHEEL (+ Stage 3 page NC), Task 14 | 10, 12, 14 |
| W3b continuous-knob wheel (PLAN-signoff 2) | fix | same accumulator | G-S3W3-WHEEL continuous arm (+ NC) | 10, 12 |
| S3 W4 stale import error | fix | processor poll | G-S3W4-ERRCLEAR (+ control, restore arm, sensitivity) | 8 |
| S3 W5 stuck UI note | fix | `uiHeld` + dtor + `visibilitychange` | G-S3W5-UIHELD (+ NC), Task 14 | 8, 10, 14 |
| N1 FileChooser | fix | member + in-flight flag | Task 14 double-click | 8, 14 |
| N2, N3 | log | — | CODE_REVIEW | 19 |
| N4 display after release | fix | `resetDisplayState` | G-VIZ-RELEASE (+ NC) | 4 |
| N5 pending auto-select vs lesson | fix | apply clears the flag | G-S3N5 (+ sensitivity) | 6 |
| N6 | by design | — | CODE_REVIEW | 19 |
| N7 C1 / bidi names | fix | `isStrippedNameChar` | G-SANITISE (+ per-class NC, 3 surfaces) | 4 |
| N8 two gesture owners | fix | refcount helpers | G-S3W3-WHEEL N8 arm (+ NC) | 10 |
| N9 `unsupported` | fix | key + branch | state 6; check-i18n | 10, 11 |
| N10 stale lesson highlight | fix | `presetState` | G-PRESET-STATE; states 1 / 2 / 8; Task 14 | 7, 10, 14 |
| N11 octave tooltips | fix | TIP_BINDINGS | boot-all-uis `--strict-tips` (+2 bound), check-i18n [2] | 11 |
| N12 `-inf` | fix | U+2212 page-only | check-i18n [14]; state-check P0 unchanged | 10, 11 |
| N13 gesture per detent | fix | `stepKnobGesture` + drag path | G-S3N13 (+ NC); probe N13 arm (+ NC); pluginval s10 | 8, 10 |
| Module stale child / `output_level` apply | upstream | never called (S10) | CODE_REVIEW "Upstream" | 9, 19 |

### Coverage audit

| Source item | Covered by |
|---|---|
| **FUNC-08** | D-AA…D-AI; Tasks 6–11, 14, 23 → S1–S4 |
| **PERF-02** | D-AS; Tasks 17–18 → S5 |
| **COMPAT-02** | D-AW; S7 / S13 statics (9, 12); Task 22 → S6 |
| **QUAL-04** | D-AR; Tasks 16, 18, 23 → S7, S8 |
| CONTEXT open questions 1–8 | OQ1 D-AA / T7; OQ2 D-AC; OQ3 D-AI; OQ4 D-AK; OQ5 D-AJ / T3; OQ6 D-AR; OQ7 D-AS; OQ8 D-AQ |
| CONTEXT constraints | regression guard → D-AT / D-AU; DSP gate + NC → Tasks 2–4; preset traps → D-AA (stale child), D-AE + S10 (no `savePresetToFile`), D-AB (defaults first), D-AB / D-AL (AsyncUpdater: the flag is the authority; no cancel needed), D-AB (bool / number types), D-AE (`/` in names); Imported × presets → D-AC; member order + `Juce` namespace → S5 / S6 / S14; UI gates + pins → R-UISTATIC / R-UIPW; auval / pluginval → R-INSTALL; temp-index commits → R-COMMIT; CI consent → Task 22 |
| RESEARCH §A A1–A15 | A1 T6; A2 T6; A3 T7; A4 T7; A5 T7; A6 T7 (G-PRESET-IMPORTED); A7 T10; A8 T11; A9 T11; A10 T7 (W1) + T8 (N1, W5); A11 T8 + T10; A12 T8; A13 T10; A14 PLAN-signoff 1–3; A15 T19 |
| RESEARCH §B rows 1–10 | 1 T2; 2 T3; 3 T2; 4 T2; 5 T2; 6 T4 + T10 + T11; 7 T4; 8 T4; 9 T19; 10 T1 + T13 + T20 |
| RESEARCH §C planner inputs | QUAL-04 gates T16; human checkpoint T23; PERF-02 harness / cases / threshold T17; cross-check T18; COMPAT-02 config T9 / T12, run T22; VERSION / CHANGELOG / CODE_REVIEW / PLUGINS T19 / T21; null baseline T1; regression sweeps T5, T9, T12, T13, T18, T20; no tag T24 |
| RESEARCH decisions for Taylor 1–5 | PLAN-signoff 1–5 |
| RESEARCH cross-part reconciliation | gate names (as RESEARCH); W2 copy in the A i18n batch (T11); G-LESSON re-transcribed (T6) before G-Q4-ALIAS uses the recipe (T16); N5 / W4 poll / notes 3–4 file ownership (sequential dispatches); `TestHooks.h` first (T2); module bugs upstream (T19) |

No unplanned items. No deferred idea is planned (RESEARCH's optional `"importedName"` in user JSON and the "two concurrent fades" W4 scheme stay out).

---

## Out of scope (Stage 4)

- Hands-on Windows DAW testing (CI only). Tagging or publishing (a separate `/publish`; tag `O-simpleWavetable-v1.0.0`).
- Fixing the preset-manager module (`modules/` is outside the commit scope; logged upstream). Mono sustain pedal semantics, an import cancel control, a display seqlock (logged, D-AQ).
- Embedding imported audio in presets; an `"importedName"` hint; a "two concurrent fades" W4 scheme.
- REQUIREMENTS status flips (`/plugin-verify`); a native fr / zh review (ship-bar item, as in Stage 3).

---

## Open risks

| # | Risk | Mitigation |
|---|------|-----------|
| R1 | The shared `build/` re-configure (new module + binary data) fails on another session's in-flight CMake edit | Stop and report (R-INSTALL); offline gates use the out-of-repo trees with `SKIP_PLUGINS` |
| R2 | PLUGINS.md is foreign-staged (`MM`, with downgrades): our row gets reverted, or their rows ride in | Temp-index CAS with the HEAD blob (R-COMMIT 3); resync skips PLUGINS.md; patch only our row in their blob if it would revert (step 7) |
| R3 | Part 2 runs in a new session and Task 1's scratch is gone | Task 13 records `s4_part1_scratch`; Task 15 re-baselines from the Part 1 tree and snapshots the Part 1 binary |
| R4 | WKWebView mouse-notch `deltaY` differs from the probe's synthetic events (RESEARCH X4) | "First event of a burst = one step" makes a notch exact by construction; Task 14 item 5 is the real-device check |
| R5 | Pins measured in headless Chromium vs WKWebView (±1 px, X6) | ≥ 2 px slack in the pins; check-ui-labels re-measures after the CSS lands; Task 14 language pass |
| R6 | fr / zh copy is machine-drafted (X7) | fr lint CLEAN + glossary; zh at `mt`, counted; native review stays a ship-bar item |
| R7 | CI: the windows-latest image lacks the WebView2 runtime; build-macos fails on a sibling; artifacts are public | Precedent run 30848239205 was green; inspect logs, never skip GUI tests; Task 22 lists every fact before consent |
| R8 | The factory sentinel keeps stale disk copies at an unchanged version after a late recipe change | The sound always comes from the table (D-AD); Task 23's fix loop deletes `.factory-version` |
| R9 | The 16 MiB drop stall is larger than the measured 170 ms (NSString / JS share unmeasured, B A3) | It still scales 6× down from 96 MiB; 8 MiB (≈ 84 ms) is the documented fallback if Taylor feels a hitch |
| R10 | W4 fold residual: a second trigger within 5 ms of a level-crossing pitch jump | Measured not worse than shipped (−24.3 vs −24.5 dB; click 1.000 vs 1.765); logged in CODE_REVIEW |
| R11 | `perf-check` on a loaded machine | Duty witness → exit 77, re-run quietly; timing never decides a correctness verdict |
| R12 | Module-header warnings fall outside the plugin's 0-warning grep | R-DBG records the count; logged upstream |
| R13 | A gate built only on the page stub misses a real WebView bridge defect (a native with no C++ twin) | R-BRIDGE exact both ways (20 / 20); Task 14 hands-on on the real WebView |
| R14 | 37 px keys rejected at Task 14 | Re-layout + re-pin in Part 1 before the commit stands (Task 14 outcome loop) |
| R15 | Cold auval rescans / pluginval runs exceed the 600 s watchdog | Background + poll (R-INSTALL); targeted `auval -v` only |
| R16 | An executor pushes or dispatches without consent | PLAN-signoff 4; Task 22 is BLOCKING; a re-dispatch needs a new "yes" |

---

## Threat register (ASVS L1; trust boundaries: WebView → natives, disk → preset JSON, host → state blob, repo → public CI)

| ID | Category | Component | Severity | Disposition | Mitigation |
|---|---|---|---|---|---|
| T-4-01 | Tampering | User preset JSON from disk (`loadPreset`) | medium | mitigate | `applyUserPresetJson`: known ids, numeric / bool only, clamp01, `output_level` ignored, non-object → false (G-PRESET-USER) |
| T-4-02 | Tampering / EoP | Preset names from the page (`savePreset` / `deletePreset` / `loadPreset`) | high | mitigate | `sanitisePresetName` (`createLegalFileName`, leading dots, 64 chars); `loadPreset` only accepts walk-order members (blocks traversal); factory refusal; no `savePresetToFile` (S10) |
| T-4-03 | Spoofing | File / preset names with bidi, C1 or invisible characters | medium | mitigate | `isStrippedNameChar` (G-SANITISE); bidi + C1 stripped in preset names |
| T-4-04 | Tampering | Restored `currentPreset` from host state XML | low | mitigate | `isVoid` gate + sanitise (P11 hostile arm) |
| T-4-05 | DoS | Drop payload size on the message thread | medium | mitigate | 16 MiB on both sides, cap before decode (G-DROP-CAPSYNC, G-DROP[b]) |
| T-4-06 | Tampering | `stepKnobDrag` / preset native arguments | low | mitigate | id allow-list, index clamp, `args.size()` / type checks (G-S3N13) |
| T-4-07 | Information disclosure | Signed CI artifacts downloadable ≈ 90 days | medium | accept with explicit consent | Task 22 lists the fact before any dispatch |
| T-4-08 | Repudiation / integrity | Pushing other sessions' commits | medium | mitigate | `origin/main..main` re-listed after the fetch; stop on any change |
| T-4-SC | Tampering | Package installs | low | accept | No dependency added to the project. pedalboard / numpy / mido run in ephemeral `uv run --with` environments (used since Stages 1–3); Playwright chromium is already cached. Nothing ships from them. |

---

## Rollback

- **Source:** `git restore --source=$S4BASE -- plugins/O-simpleWavetable` (path-scoped; `git revert` takes no pathspec), or `--source=<Part 1 commit>` to keep Part 1.
- **Binaries:** reinstall the Task 1 Stage 3 bundles (or the Task 15 Part 1 bundles) with the CLAUDE.md cache-clear + dual-variant sweep sequence.
- **PLUGINS.md:** a CAS commit of the HEAD blob with the previous row.
- **CI:** nothing to undo (no release, no tag); artifacts expire on their own, or are deleted on Taylor's request only.
