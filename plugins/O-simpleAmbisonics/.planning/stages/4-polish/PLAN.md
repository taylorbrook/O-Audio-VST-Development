# Stage 4 — Polish (Part 1: critic + carried fixes + preset manager → visual checkpoint; Part 2: validation sweep, Windows CI, release docs, hands-on + QUAL-02) — PLAN

**Plugin:** O-simpleAmbisonics · **Stage:** 4 of 4 (Polish) · **Cycle:** single (one RESEARCH / PLAN / SUMMARY / VERIFICATION) · **Target:** v1.0.0, **no tag** · **Date:** 2026-10-14
**Inputs:** `CONTEXT.md` (locked; amended by RESEARCH §0), `RESEARCH.md` (R-D1…R-D4 locked; §1–§10 + Risks), `ROADMAP.md` §Stage 4, `REQUIREMENTS.md` (FUNC-08, FUNC-09, COMPAT-02, QUAL-02 + the open QUAL-01 clause "no clicks on preset load"), `parameter-spec.md` (12 frozen params), `research/ARCHITECTURE.md` §State / §Thread Boundaries, `stages/3-gui/{PLAN,VERIFICATION}.md` (carried items), sibling model `plugins/O-simpleWavetable/.planning/stages/4-polish/{PLAN,SUMMARY}.md` (preset surface, gate shapes, R-COMMIT, CI checkpoint). Every `file:line` below was re-verified at plan time against HEAD `679421f3`.
**Executor:** plugin-execute orchestrator (Bash). Critic via `critic-orchestrator` (read-only). Code edits may be dispatched to dsp-agent (DSP fixes) / gui-agent (processor, editor, page, i18n) **strictly sequentially — never two writers on one file**. Every long command (build, harness, build-and-install, auval, pluginval, Playwright, pedalboard, `gh run watch`) runs with `run_in_background: true`, logging to `$SCRATCH` (600 s watchdog).
**Branch:** `main`, trunk-based. Other sessions are live in this checkout (O-simpleFM, O-Gain, O-Reed, O-Bassoon, O-Freeze, PLUGINS.md dirty at plan time) — commit only this plan's paths.
**Precedence:** user decisions (CONTEXT + R-D1…R-D4) > this plan's P-table > RESEARCH > ARCHITECTURE > parameter-spec > ROADMAP.

## Goal

Ship O-simpleAmbisonics v1.0.0 (untagged): a factory bank of **Init + the 8 lessons** and user scenes through the preset-manager module and a header preset bar, read from **one** recipe table shared with the lesson buttons; every critic W-tier finding and the triaged notes fixed (by-design items logged in `CODE_REVIEW.md`); the three carried fixes; a full validation sweep with 0 warnings and unchanged Stage-2 digests; a green Windows VST3 CI build (only with Taylor's go-ahead); Taylor's hands-on in Logic / Ableton and the QUAL-02 headphone sign-off.

**Invariants (never move in this stage):** 12 parameters, IDs / ranges / skews frozen; latency 64 (`setLatencySamples (kPartition)`, one call site); `processBlock` allocation count 0 (PERF-01 re-gated); presets / lessons / user JSON **never** set `outputLevel` **or** `inputMode` (R-D3); no recipe or orderStep step yields order 3 with distance < 1 m; lesson ids stable; `getNumPrograms() == 1`; one `juce_add_binary_data` target; `setSize (1180, 780)` the last numeric `setSize`; fr `reviewed: false` / zh `reviewed: 'mt'` (ship bar unchanged). digest-2.1 `9189d911a1fe98d5` and digest-2.2 `7f35bc9f91e321b9` stay asserted verdicts; digest-2.3 `edd1acfd21f54bcd` must reproduce — a moved digest is a defect unless a named critic DSP fix intends it and the user approved (P14).

## Requirement coverage

| Item | Built in | Gated by |
|---|---|---|
| **FUNC-08** (should) factory bank + user scenes | T6, T7, T8, T9 | G-FACTORY, G-LESSON, ST6, G-PRESET-*, R-BRIDGE, header-fit H2/H3, CP1, CP3 |
| **FUNC-09** (nice) binaural panner usable in a mix | — (no feature) | CP3 item 6 |
| **COMPAT-02** (must) Logic mono + stereo, Ableton, auval, Windows VST3 | T12, T13 | auval, pluginval s10, MSVC statics, CI run (CP2), CP3 items 1–2 |
| **QUAL-02** (must) headphone localisation at order 3 | — | CP3 item 5 (+ existing DSP-03 ILD-sign gate) |
| **QUAL-01** open clause "no clicks on preset load" | T6 | G-PRESET-CLICK (+ splice NC), CP3 item 3 |
| Critic pass (CONTEXT) | T2, T3 | per-fix gate + NC; `CODE_REVIEW.md` `open_findings: none` |
| Carried: verify_2_1 V3 → Stereo | T4 | verify_2_1 22/22 |
| Carried: `label.encReadN` ellipsis | T5 | header-fit H1 (+ `encreadn` NC), check-ui-labels |
| Carried: chevron / puck proximity | T5 | frame-probe A14 (+ `chevron` NC) |
| Carried human list (Stage 3 VERIFICATION) | — | CP3 items 1–4, 7–8 |
| Validation sweep + release docs | T12, T14 | §8 commands, CHANGELOG checks, CODE_REVIEW, PLUGINS row |

## Decisions adopted (binding on the executor)

| # | Decision | Source |
|---|---|---|
| **R-D1** | Retire `#chipLatency`; FM-style one-row preset bar (◀ name ▶ Save Delete) in the header strip | user (RESEARCH §0) |
| **R-D2** | Lesson = full reset to defaults + recipe (as today). ST6 stays, amended for R-D3 only | user |
| **R-D3** | `inputMode` is user-owned like `outputLevel`: never reset, set, or counted in "modified" by lessons, factory loads or user JSON | user |
| **R-D4** | Lesson "Order 1 → 2 → 3" walks order 1→2→3→1 on repeated clicks while the scene is the unmodified orderStep scene, else starts at 1; the factory preset of that name sets order 1 | user |
| **P1 — one recipe table** | New `Source/PresetRecipes.h`, `namespace ambpresets`, header-only: `struct Entry { const char* paramId; float raw; }`, `struct Recipe { const char* id; const char* fileName; int count; Entry entries[4]; }`, `inline constexpr Recipe kFactory[9]` in the table below, `findById`, `findByName` (case-insensitive). Raw engineering units; choices / bool by index. `kLessons` (`PluginProcessor.cpp:421-442`) is deleted; lesson buttons, factory loads, `buildFactoryPresetDefs()`, `getPresetCatalog` and the modified check all read this table. **Recipes = the Stage-3 values (each already isolates one idea) except orderStep (R-D4)**; any by-ear change goes through the CP3 loop. File names are ASCII and `createLegalFileName`-stable (the lesson captions contain `/` and `→`, which are not); the page shows localized labels from i18n keys, never the file name | CONTEXT FUNC-08; RESEARCH §1 |
| **P2 — one apply core** | `bool applyNormalisedTargets (const std::array<float, 12>& t)` (message thread, APVTS only, no `JUCE_ASSERT_MESSAGE_THREAD` — the harness calls it): one pass in APVTS order; **skip `outputLevel` and `inputMode`**; target = `t[i]` if finite else `getDefaultValue()` (defaults first — memory `pattern_preset_apply_needs_reset_to_defaults`); `jlimit (0, 1)`; snap through `convertTo0to1 (convertFrom0to1 (·))`; skip within 1e-6; else begin / `setValueNotifyingHost` / end per changed parameter. Recipe → targets via **each parameter's own `convertTo0to1 (raw)`** (skew-correct: `distance` 0.2–20 centre 2, `orbitRate` 0.02–2 centre 0.2 — memory `pattern_factory_preset_normalized_ignores_skew`); unlisted = NaN. Three callers: lesson click, factory load, user JSON | RESEARCH §1 |
| **P3 — lesson vs load** | `applyFactoryPreset (id)` = the **lesson** entry (native unchanged; accepts any table id, unknown / `""` → `false`, nothing touched); orderStep walks per P4. `loadPresetByName (name)` = the **browser** entry: must be a member of the walk order (blocks traversal); factory → table recipe with **no** walk (orderStep → order 1); user → `loadFileAsString` + `JSON::parse` → `applyUserPresetJson`. Both set identity (P5) and bump the revision | R-D2, R-D4 |
| **P4 — orderStep walk** | On a lesson click for `orderStep`: `continuing = getPresetId() == "orderStep" && ! isPresetModified()`; next order index = continuing ? (current order index + 1) % 3 : 0; apply the recipe with that order override (full reset — distance resets to the 1 m default, so the order-3 headroom rule holds). Cached targets = the applied targets, so each step reads unmodified. `outputLevel` / `inputMode` moves never break the walk (they are not in "modified"). **Restore:** a restored `currentPreset` of the orderStep file name rebuilds targets with the restored order index (clamped 0..2), so a reopened session shows the lesson lit and continues the walk (planner decision — without it every stepped session restores as "modified") | R-D4; RESEARCH Risks |
| **P5 — identity + `presetState`** | Members `juce::CriticalSection presetLock`, `juce::String presetName`, `std::array<float, 12> presetTargets`, `bool presetTargetsValid`, `std::atomic<juce::uint32> presetRevision`. Public: `getPresetName()`, `getPresetId()` (table id or `""`), `isPresetFactory()`, `getPresetRevision()`, `isPresetModified()` (= any parameter except `outputLevel` / `inputMode` differs from the targets by > 1e-4; unnamed → false), `refreshPresetTargetsIfNeeded()`. Ctor: name = Init's file name, targets = Init's, unmodified. `presetLock` never on the audio thread, never held across `setValueNotifyingHost`. Editor event `presetState { name, id, factory, modified }` emitted from the 30 Hz timer when the revision changes or `modified` flips (memory `pattern_webview_one_shot_state_push_stale_on_preset_load`); `uiReady` snapshots the counters **first**, then forces `emitFrame (true)` + `presetState` | RESEARCH §1 State; Wavetable D-AG / D-AN |
| **P6 — state** | `getStateInformation` (copy): `setProperty ("currentPreset", getPresetName())` beside the `uiLanguage` line (`PluginProcessor.cpp:383`). `setStateInformation`: read like `uiLanguage` (`:410`, `isVoid()` + `toString()`) → `sanitisePresetName`; **absent → `""`**; factory name → table targets (+ P4 override); user name → targets stale until the first editor tick re-reads that file (missing file → unmodified). **Never** the module's XML state pair (stale `<CustomState>` child — memory `critical_preset_manager_stale_customstate_child`) | RESEARCH §1 |
| **P7 — module surface (narrow)** | `modules/persistence/preset-manager` **v1.0.9** (`module.yaml:6`); member `OuariconPresetManager presetManager { apvts, "O-simpleAmbisonics" }` (literal name → `~/Library/O-simpleAmbisonics/Presets/{Factory,User}`, shared by dev + release builds) declared after `apvts`. Used ONLY for: the folders, `savePreset (name)`, `deletePreset (name)`, `initializeFactoryPresets` + its `.factory-version` sentinel, `preset-manager.js`. Never called: its XML state pair, its JSON apply (resets **and sets** `outputLevel`, `OuariconPresetManager.h:358-375`), `savePresetToFile` (no factory guard — memory `pattern_savepresettofile_has_no_factory_guard`), its custom-state callbacks, its prev / next / list. **Lazy factory write:** `ensureFactoryBankOnDisk()` (once per instance, message thread) is called by the preset natives only — never the ctor, `prepareToPlay`, state calls or the harness (the harness stamps the real version, `render-harness/CMakeLists.txt:81`, so a call would write Taylor's real Factory folder). Factory **storage is in code** — no BinaryData XML, so no second binary-data target (if one is ever added it needs a distinct `NAMESPACE` + `HEADER_NAME`). v1.0.6 migration hook: not used at 1.0.0 (note for future range changes) | RESEARCH §1; CONTEXT constraints |
| **P8 — natives 5 → 16, events 1 → 2** | Keep the 5 (`getParameterDefaults`, `getUiLanguage`, `setUiLanguage`, `uiReady`, `applyFactoryPreset`). Add the 10 names `preset-manager.js` resolves — `savePreset`, `savePresetWithDialog`*, `loadPreset`, `loadPresetFromFile`*, `getPresetList` (walk order), `getCurrentPreset`, `selectNextPreset` / `selectPreviousPreset` (**return the neighbour name without loading**; the module JS then calls `loadPreset`), `deletePreset`, `isFactoryPreset` — plus `getPresetCatalog` → `{ factory: [{ name, id } × 9], user: [names] }`. * = refusal stubs `complete ({ success: false, name: "" })` so no promise hangs (memory `pattern_webview_native_fn_bridge_gap`). Every native validates `args.size()` / types and completes on every path; every preset native calls `ensureFactoryBankOnDisk()` first. Update the registry comment (`PluginEditor.cpp:181-185`). Events: `meterFrame`, `presetState` | RESEARCH §1 Natives |
| **P9 — names + walk** | `static sanitisePresetName (const String&)`: trim → strip C0, DEL, C1, bidi controls (U+200E/F, U+202A–202E, U+2066–2069) → `File::createLegalFileName` → strip leading dots → cap 64 → `""` = refused. Pure statics for gating: `buildWalkOrder (const StringArray& userStems)` = 9 factory file names in table order, then user stems sorted case-insensitively minus any equal (case-insensitive) to a factory name; `neighbourIn (order, current, dir)` wraps at both ends; an unnamed / unknown current enters at the top going forward, the bottom going back. List, ◀ ▶ and the dropdown use this one order (memory `pattern_grouping_preset_dropdown_breaks_prev_next`). Save refuses a factory file name **or any factory display label in en / fr / zh** (page) and any factory file name case-insensitively (C++) — **before** any module call or disk access. A confirmed Replace saves under the **existing stem's spelling** (Wavetable 1.0.1 fix) | Wavetable D-AE |
| **P10 — header bar (R-D1)** | Delete `#chipLatency` markup (`index.html:742-745`) and CSS (`:211`). The bar takes the chip's slot (right of `#chipFormat`, beside the gear): ◀ 22 · `#presetName` button (opens the list) · ▶ 22 · Save · Delete, gap 5, FM visual language (read `git show HEAD:plugins/O-simpleFM/Source/ui/public/index.html` `:41-62` and `…/css/styles.css` `:791-830` — **never O-simpleFM's working tree**, another session's WIP). Markup + CSS hand-written and localized; **never** the module's `createPresetBar()` (unlocalized `innerHTML`). Pins: Save / Delete `width` at the widest of en / fr / zh caption + padding; `#presetName` a fixed width (ellipsis) = the largest that keeps `.header-strip` slack ≥ 4 px **and** ≥ the widest factory label in any language + 20 px (if both cannot hold → STOP, report for CP1). `.title-block` 425 pin and the gear position unchanged. List = page-built absolute listbox under the name (factory group via a literal `FACTORY_LABELERS` id → `setLabel` map, one id per line; a disabled separator; user names via `setRaw`), dismiss by capture-phase `pointerdown` outside + Escape (0 `'mousedown'`). Save / Replace? / Delete? = one page-owned popover under the bar (title, `input`, OK / Cancel, `nowrap` message line), Enter submits, key handlers ignore `INPUT` targets. Delete enabled iff `presetState.factory === false` and name non-empty, keyed on `presetState.name`. Modified = a small dot on the bar (no text). `preset-manager.js` imported **dynamically** inside `initPresets (Juce)` (a failed import disables the bar, never blanks the page); `new PresetManager ({ getNativeFunction: Juce.getNativeFunction, prevButton, nextButton, onPresetChanged, onPresetListUpdated })` — no `displayElement` / `saveButton` / `loadButton` / `deleteButton` | RESEARCH §2a; Wavetable D-AI |
| **P11 — lesson light + caption from `presetState`** | Delete the click-time `.active` toggle and caption set in `initLessons` (`index.html:2418-2422`); the click only calls `applyFactoryPreset (id)`. `presetState` lights `.tour-btn[data-preset=id]` iff `!modified`, sets the caption from `LESSON_CAPTIONERS[id]` (`:1083-1092`) else `label.tourDefault`. **`label.tourCaption.orderStep` (`i18n.js:691`) is rewritten** in en / fr / zh — it still describes the Stage-3 recipe ("3rd order at 45°. Step Order back to 1…"); new en draft: "Click again to step the order 1 → 2 → 3: each step lights a band and narrows the beam." | R-D4; RESEARCH §2a |
| **P12 — `label.encReadN`** | en `({n}+1)² = {ch} ch` · fr `({n}+1)² = {ch} can.` · zh `({n}+1)² = {ch} 声道` (matches `label.chipOrder` / `stEncoder` / `bandCh`). Box `.enc-read` 144 px, nowrap + ellipsis (`index.html:614`). check-ui-labels [4] skips inline boxes (`scripts/check-ui-labels.js:798-810`) → an explicit fit arm (header-fit H1) with the old copy as its negative control | RESEARCH §2b |
| **P13 — chevron / puck** | In `drawTop()` (`index.html:1929`): (1) draw the front-elevation chevron **after** `drawPuck` (`:2044`) with `translate (cx, cy)` / `rotate (−heading)` re-applied and a 3 px `--well-bot` backing stroke; (2) a proximity-aware tangential slide away from the puck when the rose-frame distance < 7 + (8 + 10·alt) + 6, anchored `// [probe-anchor:chevron-slide]`. Rejected: outside the rim (collides with the sage rim tick), radius change (D10 already moved R−30 → R−40) | RESEARCH §2c |
| **P14 — critic triage** | `critic-orchestrator` runs **all** critics (dsp, ui, architecture, foundation) over Stage 2–3 output **before** any preset code. Verdicts: blocker → fix (mandatory); warning → fix, **unless** the fix would touch a frozen contract (params, latency, state format, digest-2.1 / 2.2 verdicts) → STOP and ask Taylor before editing; note / info → fix when user-visible or a real defect **and** local (no contract touch), else "accepted by design" / "reviewed, not changed" with one line of reason. Every DSP fix (anything under `Source/dsp/` or in `processBlock` / `prepareToPlay` / `fillParams`) gets a harness gate whose negative control (the pre-fix behaviour) FAILS, plus a PERF-01 re-run. Findings that duplicate a carried item map to T4 / T5; findings in lesson / preset code fold into T6. All logged in `plugins/O-simpleAmbisonics/CODE_REVIEW.md` (canonical path, memory `project_code_review_artifact_path_canon`) | CONTEXT critic row; RESEARCH §3 |
| **P15 — harness growth** | One new Stage-4 block after PL4 and before the digest prints (fresh processors, `rtcheck` disarmed). The 241 Stage-2 lines, ST1–ST5 and PL1–PL4 keep their exact text; **ST6 is amended** (R-D3: `inputMode` raw bit-unchanged like `outputLevel`; literal recipe table re-transcribed, orderStep `{ order 0, azimuth 45 }`; its re-apply-idempotence arm excludes orderStep, whose second click walks by design). New lines carry an `S4 ` / `G-` prefix; each negative control is its own line that passes only when the control FAILS. Final denominator = 251 + the new lines, stated in SUMMARY. Expected tables in ST6 / G-LESSON / G-FACTORY are **literal** (never `ambpresets::`). Gates exercise pure functions + in-memory JSON only; the harness never calls `ensureFactoryBankOnDisk`, a legal-name save, a delete, or a user-name load (memory `pattern_factory_bank_sentinel_makes_preset_gates_read_stale_bank`) | RESEARCH §7; Stage-3 C8 |
| **P16 — QUAL-01 preset-load click gate** | RESEARCH §7 omits the open REQUIREMENTS clause; added as `G-PRESET-CLICK` (T6). A preset load is a parameter jump through the normal 20 ms smoothers (ARCHITECTURE `:215` "by construction") — this proves it | REQUIREMENTS QUAL-01 |
| **P17 — commits** | R-COMMIT (below) for every commit: temp index, path-scoped, CAS; never `git add -A` / `commit -a` / bare `reset` / `stash`; **no tag** (the future `/publish` tag is `O-simpleAmbisonics-v1.0.0`). PLUGINS.md only via its HEAD blob with this one row swapped | CLAUDE.md; memory `index_git_shared_checkout`, `feedback_never_tag_unless_publish` |
| **P18 — references at HEAD** | O-simpleFM is dirty (another session) → `git show HEAD:` only. O-simpleWavetable and `modules/persistence/preset-manager` are clean — read directly. `modules/**`, `.planning/mockups/*`, `parameter-spec.md` are read-only | Stage-3 C22 |

### Factory table (`Source/PresetRecipes.h`, table order = walk order = bank order)

| # | id | file name | display key | recipe (raw; everything else → default; `outputLevel` + `inputMode` never) |
|---|---|---|---|---|
| 0 | `init` | `Init` | `preset.init` (new) | azimuth 0 (one entry = the default) |
| 1 | `pointSource` | `Point Source (FOA)` | `label.lesson.pointSource` | order 0, azimuth 30, elevation 0, distance 1 |
| 2 | `orderStep` | `Order 1-2-3` | `label.lesson.orderStep` | order 0, azimuth 45 — lesson clicks walk order 0 → 1 → 2 → 0 (P4) |
| 3 | `behindYou` | `Behind You` | `label.lesson.behindYou` | azimuth 180 |
| 4 | `overhead` | `Overhead` | `label.lesson.overhead` | elevation 90, order 1 |
| 5 | `rotateRoom` | `Rotate the Room` | `label.lesson.rotateRoom` | azimuth 60, yaw 120, order 1 |
| 6 | `farNear` | `Far Away - Up Close` | `label.lesson.farNear` | distance 12 |
| 7 | `slowOrbit` | `Slow Orbit` | `label.lesson.slowOrbit` | orbit 1, orbitRate 0.05, order 1 |
| 8 | `speakersHeadphones` | `Speakers vs Headphones` | `label.lesson.speakersHeadphones` | decoder 1, order 2, azimuth 45 |

Order index 0 / 1 / 2 = orders 1 / 2 / 3. Only rows 2 (step 3) and 8 reach order 3; both leave distance at the 1 m default.

---

## Recipes (referenced by the tasks; from the repo root)

**R-BUILD:** `ninja -C build O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics_Standalone O-simpleAmbisonics-render-test > "$SCRATCH/build.log" 2>&1`; accept `grep -c "warning:" "$SCRATCH/build.log"` = 0 for plugin paths (module-header warnings, if any, are counted separately and logged upstream). The CMake change in T7 reconfigures; if the shared `build/` reconfigure fails on another plugin's in-flight CMake edit → **stop and report**, never edit foreign files.

**R-HARNESS:** `H=build/plugins/O-simpleAmbisonics/tests/render-harness/O-simpleAmbisonics-render-test_artefacts/Release/O-simpleAmbisonics-render-test`; `"$H" > "$SCRATCH/h.log" 2>&1`; accept exit 0, `[FAIL]` 0, both digest verdict lines PASS, digest-2.3 prints `edd1acfd21f54bcd`, PERF-01 0 allocations, PERF-02 ≥ 20×; every negative-control line PASS (= its control FAILS as designed). Diff vs `$SCRATCH/base-h.log` (T1): only PERF-02 timing, the amended ST6 line, T3's fix lines and new `S4 ` / `G-` lines may differ. Before and after: `ls -la ~/Library/O-simpleAmbisonics 2>&1` identical (the harness never writes the real preset folder).

**R-INSTALL:** `./scripts/build-and-install.sh O-simpleAmbisonics` (cache clear + `-dev` / unsuffixed sweep), then `ninja -C build O-simpleAmbisonics_Standalone` (the script never rebuilds it). Freshness: `strings ~/Library/Audio/Plug-Ins/VST3/O-simpleAmbisonics-dev.vst3/Contents/MacOS/O-simpleAmbisonics-dev | grep -c presetState` ≥ 1, same on the Standalone binary; installed bundles `cmp`-identical to the artefacts. `auval -v aufx OSAm OuDv` → AU VALIDATION SUCCEEDED (never `auval -a`; a cold rescan may be slow — wait). pluginval s10 per §8.

**R-STATIC** (plugin dir; comment lines stripped where noted):
- **S1 table authority:** `grep -c kLessons Source/PluginProcessor.cpp` = 0; `grep -vE '^\s*//' Source/PresetRecipes.h | grep -cE 'ParamIDs::(outputLevel|inputMode)'` = 0; no `ambpresets::` inside the ST6 / G-LESSON / G-FACTORY expected tables in `tests/render-harness/main.cpp` (inspect).
- **S2 module traps:** `grep -vhE '^\s*//' Source/*.cpp Source/*.h | grep -cE 'setStateFromXml|getStateAsXml|savePresetToFile|applyPresetData|applyPresetJson|setCustomStateCallbacks|presetManager\.(loadPreset|getNextPreset|getPreviousPreset|getPresetList|selectNext|selectPrevious)'` = 0; `grep -cE 'presetManager\.savePreset *\(' Source/PluginProcessor.cpp` ≥ 1. Comments say "the module's XML state path" — never the function names.
- **S3 ids:** `data-preset` ids in `index.html` = the 8 lesson ids = table ids minus `init`; `FACTORY_LABELERS` keys = the 9 table ids; `LESSON_IDS` in `lesson-row-budget.mjs` unchanged.
- **S4 resources:** every SOURCE in the `juce_add_binary_data` block has a `url == "…"` branch in `PluginEditor.cpp`, incl. `/modules/preset-manager.js` → `BinaryData::presetmanager_js` (hyphen stripped); `grep -c juce_add_binary_data CMakeLists.txt` = 1; `git check-ignore -q Source/ui/public/modules/preset-manager.js` exit 0 (never committed).
- **S5 invariants:** `git diff <T1 base> -- Source/PluginProcessor.cpp` has no hunk in `createParameterLayout`; `grep -cE '^\s+setLatencySamples \(' Source/*.cpp` = 1 (the `osa_dsp::kPartition` call in `prepareToPlay`; the header-comment mention is excluded by the anchor); `getNumPrograms() override { return 1; }` present; 12 relays, 12 attachments, 12 `.withOptionsFrom (`; `setSize (1180, 780)` the last numeric `setSize`; `withUserDataFolder`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` present.
- **S6 page:** `grep -c "'mousedown'" Source/ui/public/index.html` = 0; no `innerHTML` outside the canon block; no `title=` added; `grep -c chipLatency Source/ui/public/index.html Source/ui/public/js/i18n.js` = 0; keys `label.latencyBody` / `label.latencyDetail` gone; `grep -c '__JUCE__' index.html` unchanged (event backend only).
- **S7 MSVC traps** (re-run after T7 and in T12): `grep -rn "= juce::Component::SafePointer" Source` = 0; `grep -rn "M_PI" Source` = 0; every `constexpr` local captured by a lambda inspected (C3493); every `constexpr` function body free of uninitialised locals (C3615 — look at `ShRotator.h:105,111`, `ShEncoder.h:141`); `BinauralFilterData.h` unchanged.

**R-BRIDGE** (needs the configure-time `Source/ui/public/modules/preset-manager.js`):
`comm -3 <(cat Source/ui/public/index.html Source/ui/public/modules/preset-manager.js | grep -oE "getNativeFunction\([\"'][A-Za-z]+[\"']\)" | sed -E "s/.*\([\"']([A-Za-z]+)[\"']\)/\1/" | sort -u) <(grep -o 'withNativeFunction ("[A-Za-z]*"' Source/PluginEditor.cpp | sed 's/.*("\(.*\)"/\1/' | sort -u)` prints nothing; `grep -c 'withNativeFunction ("' Source/PluginEditor.cpp` = 16; the `emitEventIfBrowserIsVisible ("…"` set = the page's `addEventListener('…'` backend set = {`meterFrame`, `presetState`}.

**R-UI** (exit 77 = Playwright not resolved = **nothing verified, never a pass**; output must name O-simpleAmbisonics's page — port-clash guard): `check-i18n`, `i18n-fr-lint`, `i18n-zh-lint` (each `--plugin O-simpleAmbisonics`, exit 0; zh-lint output must name the plugin with its entry count — exit 0 alone is vacuous); `check-ui-labels.js --plugin O-simpleAmbisonics` exit 0 with banner `seed=overrides`; `boot-all-uis.js --plugin O-simpleAmbisonics --strict-tips` 0 page errors / 0 404 / 0 DEAD / 0 late; frame-probe 14/14 + NCs `fuma` / `pole` / `idle` / `chevron` each "FAILS as designed"; header-fit probe exit 0 + NC `encreadn` FAILS; lesson-row-budget exit 0 (fr slack ≥ 8, `lesson-row-budget.mjs:64`); tooltip-clamp-probe exit 0; `generate_sh_icons.mjs --check` exit 0. Suite-wide fr-lint (no `--plugin`) exits 2 on main because of O-DigiDelay — if run, compare to that baseline ("no new finding"), never treat as ours. NC trees built from `git archive` must get the gitignored `Source/ui/public/modules/*.js` copied in (memory `pattern_git_archive_ui_tree_misses_gitignored_module_copies`).

**R-COMMIT** (run under `bash -c`; zsh pathspec / `:`-modifier traps):
1. Immediately before: `git branch --show-current` = `main`; `git status --short`; `git diff --cached --stat` — note foreign staged / dirty files; never stage, reset or commit them.
2. `export GIT_INDEX_FILE="$SCRATCH/idx"; old=$(git rev-parse HEAD); git read-tree "$old"; git add -- plugins/O-simpleAmbisonics` (respects `.gitignore` → `Source/ui/public/modules/*` stays out; a plain pathspec `git commit` would drop new untracked files — memory `pattern_git_commit_pathspec_takes_only_tracked_files`).
3. PLUGINS.md (T14 only): `git show "${old}:PLUGINS.md"` with **only** the `| O-simpleAmbisonics |` row (line 69) replaced → `git hash-object -w` → `git update-index --cacheinfo "100644,<blob>,PLUGINS.md"`.
4. `tree=$(git write-tree); new=$(git commit-tree "$tree" -p "$old" -F "$SCRATCH/msg")`; `git update-ref refs/heads/main "$new" "$old"` (CAS: if HEAD moved, redo 2–4 from the new HEAD); `unset GIT_INDEX_FILE`.
5. After: `git show --stat --format= HEAD` lists only `plugins/O-simpleAmbisonics/**` (+ `PLUGINS.md` in T14); `git ls-tree -r --name-only HEAD -- plugins/O-simpleAmbisonics/Source/ui/public/modules` empty; resync own paths only `git reset -q -- plugins/O-simpleAmbisonics` (skip PLUGINS.md — foreign-dirty; if their staged blob would revert our row, patch only our row in it).

---

## Execution parts and the checkpoints

```
PART 1  T1 pre-flight ─→ T2 critic + triage ─→ T3 critic fixes ─┬─→ T4 verify_2_1 V3 ─┐
                                                                └─→ T5 encReadN + chevron (+A14, header-fit H1)
        ─→ T6 recipes + apply core + lessons + identity/state (harness) ─→ T7 module + natives + presetState ══ TRACER GATE ══
        ─→ T8 header preset bar ─→ T9 i18n + states + stub ─→ T10 Part-1 gate sweep ─→ T11 install + validate + commit P1
        ⏸ CP1 preset-bar visual (Taylor; execute STOPS)
PART 2  T12 full validation sweep (+ commit if anything changed) ─→ ⏸ CP2 CI go-ahead ─→ T13 push + dispatch (ONLY on "yes")
        ─→ T14 release docs + REQUIREMENTS + PLUGINS row + commit ─→ ⏸ CP3 hands-on + QUAL-02 sign-off ─→ T15 record + SUMMARY + commit + handoff
```

**Tracer gate (end of T7):** the preset path is proven end to end on the real build before any page work — table → apply core → identity → state → module compiled with `presetmanager_js` → natives registered → `presetState` emitted. A build that cannot compile the module, a harness red, or a broken Standalone lesson click is architectural: stop and report, do not start T8.

---

## Tasks

### Part 1

#### T1 — Pre-flight + baselines *(orchestrator)*
**Files:** `.planning/STATUS.md`. **Depends on:** none.
- `git branch --show-current` = `main`; `git status --short` recorded (foreign dirty paths listed in SUMMARY); `git status --short -- plugins/O-simpleWavetable modules/persistence/preset-manager` empty.
- R-BUILD; R-HARNESS ×1 → `$SCRATCH/base-h.log`: 251/251, digests as in the Invariants. `ls -la ~/Library/O-simpleAmbisonics` recorded (expected absent).
- Header baseline for T10: open the current page via the generic stub (serve-ui) at 1180×780 and record `#gear-btn` `getBoundingClientRect().left` and `.title-block` width in en / fr / zh at order 3 (`$SCRATCH/header-base.json`).
- STATUS: `status: stage_4_execute_part1_in_progress`, `current_phase: execute`.
- **Acceptance:** baseline numbers equal the Stage-3 VERIFICATION values; scratch path recorded in STATUS (`s4_part1_scratch:`).

#### T2 — Critic pass + triage *(critic-orchestrator, read-only; then orchestrator)*
**Files:** create `.planning/stages/4-polish/critic/{dsp,ui,architecture,foundation}.json`, `critic/unified.json`; create `CODE_REVIEW.md` (draft). **Depends on:** T1.
- Spawn `critic-orchestrator` for O-simpleAmbisonics, stage "4-polish (review of Stage 2–3 output)", **all four critics** (Stage-4 row "all domain critics if changes detected" — this is the plugin's first real review; Stage 2's dsp-critic was a smoke stub, `2-dsp/gate-report.json`). Scope: `Source/**`, `tests/**`, `scripts/**`, `CMakeLists.txt` at HEAD. `python3 .claude/hooks/merge-critic-reports.py --stage 4-polish --plugin O-simpleAmbisonics critic/*.json > critic/unified.json` (exit 1 = blockers present).
- Triage every finding per P14 into a table (id, severity, file:line, verdict fix / by-design / reviewed-not-changed / STOP-ask, owning task). `CODE_REVIEW.md` draft at the plugin root: suite frontmatter shape of `plugins/O-simpleWavetable/CODE_REVIEW.md:1-20` (`phase: stage-4-polish`, `files_reviewed_list`, `findings` counts), IDs WR-xx (blocker / warning) and IN-xx (note / info), plus rows for the three carried items and an "Upstream (not plugin code; not called)" section: module stale `<CustomState>` child, its JSON apply that resets and sets `outputLevel`, `savePresetToFile` without a factory guard.
- **STOP conditions:** any finding whose fix needs a frozen-contract change (P14) → present it with options, wait for Taylor.
- **Acceptance:** `unified.json` exists; every finding has a verdict and an owner; the triage table is reproduced in SUMMARY Part 1.

#### T3 — Critic fixes *(dsp-agent for DSP items, gui-agent for the rest; sequential)*
**Files:** as triaged (expected within `Source/**`, `tests/render-harness/main.cpp`, `Source/ui/public/**`). **Depends on:** T2 (and any STOP answered).
- Apply every "fix" verdict. Each DSP fix: a new harness line + its negative-control line (pre-fix behaviour FAILS), PERF-01 re-run, digest rule (Invariants). Preset / lesson-code findings are fixed inside T6, not here.
- **Acceptance:** R-BUILD 0 warnings; R-HARNESS green with only allowed diffs + the new fix lines; every fix's status `resolved` in the `CODE_REVIEW.md` draft with its gate name; 0 blockers open.

#### T4 — Carried: `verify_2_1.py` V3 → Stereo *(user-approved)*
**Files:** modify `tests/pedalboard/verify_2_1.py`. **Depends on:** T1 (independent of T3 / T5).
- `:79` `for dec in ("Stereo", "Binaural")` → Stereo only (its 3 checks kept). Header (`:1-5`): one note that the Binaural V3 arm was retired at Stage 4 because it asserted the 2.1 single-sample response; the real Binaural onset / latency is `verify_2_3.py` V2 (`:157-172`, onset = 100 + `kBinauralOnsetFrontSamples[order]`) and the IR identity V1 (`:140-155`). Relabel V1 (`:47`) to "default (Binaural) is NOT silent" — the word "placeholder" leaves the file.
- **Acceptance:** `grep -ci placeholder tests/pedalboard/verify_2_1.py` = 0; on the installed VST3 (T11) → **22/22 ALL PASS**.

#### T5 — Carried UI fixes: `encReadN` copy + chevron / puck *(gui-agent)*
**Files:** modify `Source/ui/public/js/i18n.js` (`label.encReadN`, `:663`), `Source/ui/public/index.html` (`drawTop`), `tests/ui-probes/frame-probe.mjs`; create `tests/ui-probes/header-fit-probe.mjs`. **Depends on:** T3.
- P12 copy in en / fr / zh (fr `reviewed: false`, zh `'mt'` unchanged).
- P13 in `drawTop`; anchor exactly once.
- **frame-probe A14** (no new fixture — harness PL4 pins 15): via `__probe.setParams` az = yaw = 20, pitch 40, el 30, d ≈ 4.4 (idle fixture so the puck follows the parameters): amber chevron pixels present above threshold and ≥ 4 px gap between the amber chevron mask and the brass puck / halo / ▲ mask, excluding the beam's low-alpha amber lobe. `--negative-control chevron` serves an `index.html` copy with the `probe-anchor:chevron-slide` slide disabled → A14 FAILS as designed (a patch matching 0 or > 1 anchors exits non-zero).
- **header-fit probe** (serve-ui + generic stub, `--root`, exit = failed arms, 77 = no Playwright; viewport 1180×780; asserts the served title names O-simpleAmbisonics and banner `seed=overrides`). **H1:** order 3 and order 2, en / fr / zh via `window.__setLanguage`: the `#encReadN` parent `.enc-read` `scrollWidth <= clientWidth`. NC `--negative-control encreadn`: `#encReadN.textContent = "(3+1)² = 16 channels"` → H1 en FAILS as designed. (H2 / H3 land in T10.)
- **Acceptance:** frame-probe 14/14 + all four NCs FAIL as designed; header-fit H1 PASS + its NC FAILS; check-i18n / fr-lint / zh-lint exit 0.

#### T6 — Recipe table, apply core, lessons, identity, state *(gui-agent; R-D2 / R-D3 / R-D4)*
**Files:** create `Source/PresetRecipes.h`; modify `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `CMakeLists.txt` (`target_sources` += `Source/PresetRecipes.h`), `tests/render-harness/main.cpp`. **Depends on:** T3 (file-disjoint from T5; runs after it to keep one writer at a time).
- P1 table; P2 core; P3 `applyFactoryPreset` (lesson) — replaces the body at `PluginProcessor.cpp:445-482`, now also skipping `inputMode`; P4 walk; P5 identity (no event yet); P6 state. Update the header comments ("Stage 4": recipes in `PresetRecipes.h`, `outputLevel` + `inputMode` user-owned, `currentPreset` property) and the `.h` lesson comment (`PluginProcessor.h:126-134`).
- Harness (P15) — new / amended lines:

| Line | PASS condition |
|---|---|
| **ST6** (amended) | as Stage 3, plus `inputMode` (set to 1) raw bit-unchanged after every lesson; literal table with orderStep `{ order 0, azimuth 45 }`; idempotence arm on the 7 non-walking ids |
| **G-LESSON[walk]** | fresh: `applyFactoryPreset ("orderStep")` ×4 → order index 0, 1, 2, 0; every step azimuth 45, distance default, all else default; `outputLevel` −12 dB and `inputMode` 1 bit-unchanged |
| **G-LESSON[restart]** | after step 2, azimuth +5 → next click → order index 0 |
| **G-LESSON[owned]** | after step 1, move only `outputLevel` and `inputMode` → next click → order index 1 (walk continues) |
| **G-LESSON-NC** | a test-local naive reset loop over all 12 params moves `outputLevel` / `inputMode` → the owned arm FAILS as designed |
| **G-FACTORY[table]** | 9 rows, ids = the literal list in table order, unique; file names ASCII, no `/\:`, `== File::createLegalFileName (name)`, unique case-insensitively; every raw finite and `convertFrom0to1 (convertTo0to1 (raw)) ≈ raw` (rel 1e-4, choices integral, no clamp); no row lists `outputLevel` / `inputMode`; ≤ 4 entries |
| **G-FACTORY[init]** | Init's targets == every parameter's default (1e-6); a fresh instance reports `{ "Init", "init", factory, unmodified }` |
| **G-FACTORY[apply]** | after each factory load every listed parameter's `convertFrom0to1 (getValue()) ≈ raw` (rel 1e-4) — the skew arm on `distance` (farNear 12) and `orbitRate` (slowOrbit 0.05) |
| **G-FACTORY[skew-NC]** | a test-local linear normalise `(raw − start) / (end − start)` fails the skew arm → FAILS as designed |
| **G-ORDER3-DIST** | for every row and every orderStep step, the applied state never has order index 2 with distance < 1.0 m − 1e-6; NC: an injected test-local row `{ order 2, distance 0.5 }` is flagged → FAILS as designed |
| **G-PRESET-STATE[identity]** | lesson `behindYou` → `{ "Behind You", "behindYou", factory, unmodified }`, revision bumped; azimuth +1° → modified; only `outputLevel` moved → unmodified; only `inputMode` moved → unmodified |
| **G-PRESET-STATE[restore]** | `currentPreset` survives get → fresh set → get → second fresh set (two reopens); property absent → `""`, unmodified; hostile `"../x" + U+202E + "y"` (built from hex) → no `/`, no bidi; orderStep saved at step 3 → restored unmodified, next click → order index 0 |
| **G-PRESET-CLICK** (QUAL-01) | 48 k, 64-sample blocks, 100 Hz sine −12 dBFS on both inputs: the factory bank applied in table order, then orderStep clicks 0→1→2→0, each at a block boundary mid-tone; per transition, max \|Δ²y\| over [load, load + 60 ms] ÷ max (the same window of the static render at the source state, of the static render at the destination state) **≤ 2.0** on both channels |
| **G-PRESET-CLICK-NC** | a hard splice of the two static renders at the load sample, on the transitions with a decoder or order change (speakersHeadphones load, orderStep 0→1, 2→0) → ratio ≥ 5 → FAILS as designed |

  If a legitimate transition exceeds 2.0 against the static pair, switch that arm to the same-motion reference (as the Stage-2 B8 yaw-step gate does — memory `pattern_step_click_gate_needs_same_motion_control`); never raise the ceiling.
- **Acceptance:** R-BUILD 0 warnings; R-HARNESS green, digests unchanged (no DSP file touched); PERF-01 0; `git diff` of `PluginProcessor.cpp` has no hunk inside `prepareToPlay`, `processBlock`, `fillParams`, `createParameterLayout`; S1 clean.

#### T7 — Module, natives, `presetState`, user presets *(gui-agent)* — ends with the TRACER GATE
**Files:** modify `CMakeLists.txt`, `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.{h,cpp}`, `tests/render-harness/main.cpp`. **Depends on:** T6.
- **CMake:** `ouaricon_add_module(O-simpleAmbisonics preset-manager)` after `target_sources` and **before** `juce_add_binary_data` (`CMakeLists.txt:65`; the configure copies `preset-manager.js` into `Source/ui/public/modules/`, gitignored at `.gitignore:26`); add `Source/ui/public/modules/preset-manager.js` to the **existing** `O-simpleAmbisonics_UIResources` SOURCES; keep the "only binary-data target" comment true. The harness picks up the module include dir through `$<TARGET_PROPERTY:O-simpleAmbisonics,INCLUDE_DIRECTORIES>` (`tests/render-harness/CMakeLists.txt:33-35`; `ouaricon_add_module` adds a PRIVATE include) — add an explicit include only if the harness fails to compile.
- **Processor (P7, P9):** `#include "OuariconPresetManager.h"`; the member; `sanitisePresetName`, `buildWalkOrder`, `neighbourIn` (pure statics), `getPresetWalkOrder()` (reads the User folder), `getNeighbourPreset (int)` (no load, no parameter touched), `buildFactoryPresetDefs()` (pure: 9 defs, table order, exactly **10** keys each — every id but `outputLevel` / `inputMode` — values = recipe targets else defaults), `ensureFactoryBankOnDisk()`, `applyUserPresetJson (const var&)` (strict: the `parameters` object; known ids; `isDouble / isInt / isInt64 / isBool` only, else that parameter's default; clamp 0..1; `outputLevel` / `inputMode` ignored even if present; non-object → `false`, nothing changed), `loadPresetByName`, `saveUserPreset` (sanitise → refuse empty / factory name case-insensitively **before** `ensureFactoryBankOnDisk` → `presetManager.savePreset (clean)` → name, targets = current values, revision), `deleteUserPreset` (factory → false; current → name `""`), `getPresetCatalog()`, `refreshPresetTargetsIfNeeded()`.
- **Editor (P5, P8):** resource branch `if (url == "/modules/preset-manager.js")` → `BinaryData::presetmanager_js` (`application/javascript; charset=utf-8`); the 11 new natives; members `juce::uint32 lastPresetRevision`, `bool lastPresetModified` above the relay block (member order relays → WebView → attachments unchanged); `emitPresetState (bool force)`; `timerCallback` (`PluginEditor.cpp:381-385`): `refreshPresetTargetsIfNeeded()`, `sampleFrame`, `emitFrame`, then `presetState` if the revision changed or `modified` flipped; `uiReady` (`:230-237`) snapshots first, then forces both events; `applyFactoryPreset` native keeps `forceFrameEmit = true`.
- Harness additions:

| Line | PASS condition |
|---|---|
| **G-FACTORY[defs]** | `buildFactoryPresetDefs()` = 9 defs, names = the file names in table order, exactly 10 keys each, values = recipe targets else defaults (1e-6) |
| **G-PRESET-USER[roundtrip]** | module-format JSON `{ parameters: { id: normalised × 12 }, version, plugin }` from an off-default state with `outputLevel` 0.9 / `inputMode` 1, applied to a processor at `outputLevel` −12 dB / `inputMode` 0 → the 10 match within 1e-6; the two owned params bit-unchanged |
| **G-PRESET-USER[malformed]** | string / array / object / `null` values → that parameter's default; `parameters` missing, not an object, or a top-level non-object → `false`, nothing changed, 0 gestures |
| **G-PRESET-USER[missing]** | absent keys → defaults |
| **G-PRESET-USER[clamp]** | 1.7 → 1.0, −0.3 → 0.0 |
| **G-PRESET-USER[refuse]** | `saveUserPreset` of every factory file name (`"behind you"`, `"INIT"`, …), `""`, `"   "`, `".."`, `"/"` → `false`, nothing written (`~/Library/O-simpleAmbisonics` listing unchanged) |
| **G-PRESET-USER-NC** | a test-local apply that copies every key moves `outputLevel` / `inputMode` → FAILS as designed |
| **G-PRESET-NAME** | sanitise: trim; C0 / DEL / C1 / each bidi control stripped (code points from hex — memory `critical_juce_string_char_ctor_is_ascii_only`); `/\:` handled by `createLegalFileName`; leading dots stripped; 64-char cap; all-stripped → `""` |
| **G-PRESET-WALK** | `buildWalkOrder ({ "b", "A", "behind you", "Zed" })` = the 9 file names then `A`, `b`, `Zed`; `neighbourIn` next / prev incl. wrap and the unnamed entry rule; `getNeighbourPreset (±1)` leaves every parameter bit-equal and the revision unchanged |

- **TRACER GATE (all must hold before T8):** (1) R-BUILD 0 warnings (module-header warning count recorded); (2) R-HARNESS green (`~/Library/O-simpleAmbisonics` untouched); (3) C++ native count 16 and `strings` on the built VST3 finds `presetState`; (4) S2, S4, S7 clean; (5) `node scripts/boot-all-uis.js --plugin O-simpleAmbisonics --strict-tips` still green (page unchanged; proves the resource ladder); (6) Standalone (window partly visible) — a lesson click still moves its controls and Output does not move.

#### T8 — Page: header preset bar *(gui-agent)*
**Files:** modify `Source/ui/public/index.html`. **Depends on:** T7 tracer.
- P10 markup + CSS + behaviour, P11 lesson coupling. Static markup for every tip anchor (`#presetPrev`, `#presetName`, `#presetNext`, `#presetSave`, `#presetDelete`, aria via `data-i18n-aria`); every pin carries a comment with its three measured widths. `presetState` listener registered beside `meterFrame` (`:2433-2435`): name button shows the factory label (`FACTORY_LABELERS`), the user name (`setRaw`), or a raw `—` when unnamed; dot iff `modified`; Delete enable rule; lesson light + caption.
- Save flow: trimmed empty → `preset.errEmpty`; a factory file name or any factory label in any language (case-insensitive) → `preset.errFactoryName`; an existing user name not yet confirmed → `preset.confirmReplace {name}`, OK again saves under the existing stem; `savePreset` false → `preset.errSave`; success closes and refreshes the catalog.
- **Must not:** touch the 12 binding tables, existing pins, `Source/ui/public/modules/**`, or use `window.__JUCE__` for natives.
- **Acceptance:** S3 / S6 clean; gates in T10.

#### T9 — i18n, states, stub *(gui-agent)*
**Files:** modify `Source/ui/public/js/i18n.js`, `tests/i18n-states.json`, `tests/ui-stub/generic-overrides.json`. **Depends on:** T8.
- Glossary check first: `scripts/i18n-{fr,zh}-glossary.js` already carry preset / save / delete / next / previous / browse (fr `:81-99`, zh `:190-201`). If a new term is genuinely missing → glossary edit **first**, own R-COMMIT (shared files; `git status --short -- scripts/i18n-*-glossary.js` must be empty before editing, else STOP and ask), suite-wide lints "no new finding".
- New entries (en; fr `reviewed: false`; zh `reviewed: 'mt'`): LABELS `label.presetSave`, `label.presetDelete`, `preset.init`, `preset.saveTitle`, `preset.namePlaceholder`, `preset.ok`, `preset.cancel`, `preset.errEmpty`, `preset.errFactoryName`, `preset.errSave`, `preset.confirmReplace`, `preset.confirmDelete`, `aria.presetPrev / presetName / presetNext / presetSave / presetDelete / presetList`; I18N tips `presetPrev`, `presetName`, `presetNext`, `presetSave`, `presetDelete` (bodies say presets never move Output Level or Input Mode). TIP_BINDINGS (`:726-767`): remove `['#chipLatency', 'chipLatency']` (`:732`), add the 5. Remove I18N `chipLatency` (`:113-125`) and LABELS `label.latencyBody` / `label.latencyDetail` (`:563-564`) (check-i18n orphan rule); append the latency sentence (from the retired chip tip) to the `decWrap` tip body (`:371-387`) in all three languages. Rewrite `label.tourCaption.orderStep` (P11). fr typography per `i18n-fr-lint` (U+00A0, U+2019); no `sauver` / `sauvegarder`.
- `tests/i18n-states.json` (C++-producible only — memory `pattern_ui_fixture_state_must_be_producible_by_cpp`): orderStep state `order 2 → 0`; every lesson state also emits `window.__stubEmit ('presetState', { name: <file name>, id, factory: true, modified: false })` (the click no longer lights). Append: Init; the widest factory label (measured across languages) with `modified: true`; a 64-char user name (`factory: false`); list open (click `#presetName`); Save popover open; factory-label save refused; Replace? confirm; Delete? confirm on a user preset; each popover state ends with an Escape eval.
- `generic-overrides.json` `natives`: `getPresetList` (9 file names + 2 user), `getPresetCatalog`, `getCurrentPreset` ("Init"), `isFactoryPreset`; `_why` line updated.
- **Acceptance:** check-i18n, fr-lint, zh-lint (`--plugin`) exit 0; zh count recorded (149 at Stage 3 → new).

#### T10 — Part-1 gate sweep *(orchestrator; fixes back to gui-agent)*
**Files:** modify `tests/ui-probes/header-fit-probe.mjs` (H2 / H3), `tests/ui-probes/tooltip-clamp-probe.mjs` (`ANCHORS` `:72-77` += `['preset Delete', '#presetDelete']`). **Depends on:** T9 and a configure that placed `modules/preset-manager.js`.
- **H2** (order 3 — `label.chipOrder` at its widest — en / fr / zh): `.title-block` width 425 ± 0.5; `.header-strip` `scrollWidth <= clientWidth`; `#chipFormat .chip-body` and every preset-bar caption `scrollWidth <= clientWidth` (pinned `[data-i18n]` check-ui-labels cannot see — memory `pattern_ui_labels_gate_blind_to_width_pinned_data_i18n`); `#gear-btn` left = T1 baseline ± 0.5 px. **H3:** `presetState` with each factory label → `#presetName` not ellipsised; the 64-char user name → ellipsised, box width and every bar element's left unchanged.
- R-STATIC S1–S7, R-BRIDGE, R-UI (all), R-HARNESS.
- **Acceptance:** every check exit 0 (none 77); R-BRIDGE empty with 16 natives / 2 events; header-fit H1–H3 PASS and NC fails; tooltip-clamp PASS at 1180×780 and 708×468 with the new anchor.

#### T11 — Build, install, validate, commit Part 1 *(orchestrator)*
**Files:** modify `tests/pedalboard/verify_3_state.py`, create `.planning/stages/4-polish/SUMMARY.md` (Part 1), modify `.planning/STATUS.md`. **Depends on:** T4, T10.
- `verify_3_state.py` (RESEARCH §7; S1–S3 untouched): **S4** a fresh instance's decoded XML carries `currentPreset="Init"`; patch it to a 4-char user name (`"Test"`), re-encode, assign `raw_state`, decode again → `"Test"`; **S5** attribute removed → round-trips as `currentPreset=""`; the 12 values unchanged through S4 / S5.
- R-BUILD; R-HARNESS ×1; R-INSTALL; pluginval s10 ×1 VST3 + ×1 AU (editor + processor changed); verify_2_1 (22/22), verify_2_2, verify_2_3 (exit 0), verify_3_state (S1–S5).
- SUMMARY Part 1: critic triage table; fixes with gate values + NC margins; carried fixes; recipe table as shipped; harness denominator + every new line's detail; digests; R-BRIDGE; UI gates; pins measured per language; deviations; open items for CP1.
- STATUS: `status: stage_4_execute_part1_complete_visual_pending`, `next_action: user_visual_checkpoint_then_plugin_execute`.
- R-COMMIT. Message: `feat(O-simpleAmbisonics): Stage 4 Part 1 — critic fixes, carried fixes, preset manager + header bar (FUNC-08), harness <N>/<N>`.
- **Acceptance:** all of the above green; commit passes R-COMMIT step 5.

### ⏸ CP1 — Preset-bar visual checkpoint *(Taylor; BLOCKING — execute STOPS)*
Surface: the Release Standalone `build/plugins/O-simpleAmbisonics/O-simpleAmbisonics_artefacts/Release/Standalone/O-simpleAmbisonics-dev.app`, audio on.
1. **Header** in en / fr / zh at order 3: bar not clipped, Format chip intact, title and gear where they were; latency now explained in the Decoder tooltip.
2. **Browser:** each of the 9 factory presets loads (localized names); ◀ ▶ walk factory then user; Save "My Scene" → appears; Save "Behind You" (or its fr / zh label) → refused; Save "my scene" → Replace? → keeps "My Scene"; Delete disabled on factory; Delete "My Scene" → confirm → gone.
3. **Never moved by a preset or lesson:** Output Level and Input Mode.
4. **Lessons:** click → lit + caption; nudge any knob → unlit + dot; "Order 1 → 2 → 3" clicks 1 → 2 → 3 → 1; move azimuth then click → back to 1; close / reopen the editor → same name, light and dot.
5. **Nits:** "(3+1)² = 16 ch" reads in full; chevron visibly separate from the puck at azimuth = yaw (e.g. az 20, yaw 20, pitch 40, elevation 30, distance 4.4 m).
6. **Look:** spacing, colour, type of the bar against the header.

On a finding → fix in Part 1 (owning agent), re-run T10–T11 (fix commit), repeat CP1. Handoff when stopping: Step 1 `/clear`, Step 2 `/plugin-execute O-simpleAmbisonics` (resumes at T12 from STATUS). Alternatives: `/plugin-status O-simpleAmbisonics`, `/show-standalone O-simpleAmbisonics`.

### Part 2

#### T12 — Full validation sweep *(orchestrator)*
**Depends on:** CP1 sign-off. Every command in the background with a log; RESEARCH §8 **verbatim**:
1. R-BUILD → 0 `warning:`.
2. Harness **×2** → all PASS, digest-2.1 `9189d911a1fe98d5` / 2.2 `7f35bc9f91e321b9` PASS, 2.3 `edd1acfd21f54bcd` reproduced; the two logs identical except PERF-02.
3. R-INSTALL (installed `cmp`-identical, Standalone rebuilt); `plutil -extract CFBundleShortVersionString raw` = `1.0.0` on the installed VST3, AU and the Standalone.
4. `auval -v aufx OSAm OuDv` → SUCCEEDED.
5. pluginval s10 **×2** VST3 + **×2** AU (§8 command lines) → SUCCESS ×4.
6. `for v in 2_1 2_2 2_3; do uv run --no-project --python 3.12 --with pedalboard --with numpy --with scipy python plugins/O-simpleAmbisonics/tests/pedalboard/verify_$v.py; done` → 22 / 35 / 42 ALL PASS; `uv run --no-project --python 3.12 --with pedalboard python plugins/O-simpleAmbisonics/tests/pedalboard/verify_3_state.py` → S1–S5.
7. R-UI in the §8 order + header-fit probe + its NC; frame-probe 14/14 + NCs `fuma` / `pole` / `idle` / `chevron`.
8. R-STATIC S1–S7 (S7 re-grep after the preset code), R-BRIDGE.
- Any finding → fix via the owning agent, re-run the affected steps, then the full list once more. If anything changed since the Part-1 commit → R-COMMIT (`fix(O-simpleAmbisonics): Stage 4 validation sweep — …`).
- **Acceptance:** every line above as stated; none 77; nothing uncommitted under `plugins/O-simpleAmbisonics/` except STATUS / SUMMARY.

### ⏸ CP2 — COMPAT-02 Windows CI go-ahead *(Taylor; BLOCKING)*
The executor refreshes and presents these facts, then asks **"Push `main` and dispatch the validate-only Windows build? (yes / no)"**, and never pushes or dispatches without an explicit "yes":
1. `git fetch origin && git log --oneline origin/main..main` — the exact list (45 at plan time: 35 O-simpleAmbisonics + 10 O-Octagon, plus Stage 4's own); **every other session's commit rides along — list them by name**.
2. `git ls-tree -r --name-only origin/main -- plugins/O-simpleAmbisonics | wc -l` (0 at plan time): the push publishes the whole plugin source to the public AGPL repo.
3. The push fires `ui-static-gates.yml`, which runs **suite-wide** `i18n-fr-lint` — red on main already (O-DigiDelay finding), not caused by this plugin.
4. The dispatch runs build-windows (VST3 + pluginval v1.0.3 s10, `build-and-release.yml:603-620`) **and** build-macos (universal, Developer-ID sign + notarize); signed binaries upload as workflow artifacts downloadable by signed-in users for the retention period; `create-release` is skipped (`:701-706`); no tag.

#### T13 — Push + dispatch + verdict *(orchestrator; ONLY after "yes")*
**Depends on:** CP2 "yes".
1. `git fetch origin`; re-list `origin/main..main` — if it differs from what Taylor approved → **stop and re-ask**.
2. `git push origin main`.
3. `gh workflow run build-and-release.yml --ref main -f plugin_name=O-simpleAmbisonics -f version=1.0.0-validate -f validate_only=true`.
4. `gh run list --workflow build-and-release.yml -L 1` → id; `gh run watch <id> --exit-status` in the background; poll.
5. `gh run view <id>`; `gh run download <id> -n pluginval-windows-log -D "$SCRATCH/ci"`.
- **Pass:** build-windows ✓, the pluginval step exited 0 (pipefail) with 0 `FAILED` and all test groups completed — the log has **no** SUCCESS line (pluginval prints it to stderr; Wavetable deviation 7); create-release skipped; build-macos ✓ (if red only because of a sibling, report it — COMPAT-02 is judged on the Windows job).
- **Fail:** `gh run view <id> --log-failed`; fix Windows-only causes (MSVC C3493 / C3615, WebView2) via the owning agent, fix commit; **a re-dispatch needs a new explicit "yes"**.
- **On "no":** record "config verified (S5, S7), CI run not authorized" for `/plugin-verify`; continue with T14.

#### T14 — Release docs, REQUIREMENTS, PLUGINS row, commit *(orchestrator)*
**Files:** create `CHANGELOG.md`; finalize `CODE_REVIEW.md`; modify `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/research/ARCHITECTURE.md`, `.planning/STATUS.md`; root `PLUGINS.md` (one row). **Depends on:** T12 (T13 result if run).
1. `CMakeLists.txt:24` already `VERSION "1.0.0"` — no change; `grep -c PLUGIN_VERSION CMakeLists.txt` = 0.
2. **CHANGELOG.md** (model `plugins/O-simpleWavetable/CHANGELOG.md`): exactly one `## [1.0.0] - 2026-10-xx`; `###` subsections only (Added: the chain, orders 1–3, rotation, Binaural MagLS / Stereo, orbit, the Field Guide UI, lessons, presets; Fixed in Stage 4: each critic item + carried fix with its gate; Known limitations: by-design list; Development history as bullets). Checks: `awk '/^## \[1.0.0\]/{f=1;next} /^## \[/{f=0} f' plugins/O-simpleAmbisonics/CHANGELOG.md | wc -l` > 0; `grep -cE '^## \[' CHANGELOG.md` = 1; `grep -cE '^## v?[0-9]|^## Version' CHANGELOG.md` = 0 (memory `pattern_changelog_heading_must_be_bracket_form_and_partial_fix_is_worse`).
3. **CODE_REVIEW.md** final: `status: resolved`, `open_findings: none`, resolution log `| Finding | Status | Where |` (resolved in v1.0.0 + gate / accepted by design / reviewed, not changed / upstream).
4. **REQUIREMENTS.md** (gate-proven only; human items wait for T15): FUNC-08 → complete (stage-4) with G-FACTORY / G-LESSON / G-PRESET-* / CP1 evidence; QUAL-01 → complete, tick "No clicks on preset load" with G-PRESET-CLICK values; COMPAT-02 tick `auval -v` (+ CI run id / verdict, or "not authorized"); `lastUpdated`.
5. **ARCHITECTURE.md:** append "Stage 4 Amendments (2026-10-xx)" — A1 custom state `currentPreset` (+ Stage-3 `uiLanguage` / `editorScale`) supersedes "Custom state: none" (`:214`); A2 one recipe table + apply core, presets never set `outputLevel` **or `inputMode`** (R-D3); A3 orderStep walk (R-D4); A4 `#chipLatency` retired for the header preset bar (R-D1), latency copy moved to the Decoder tip; A5 module used narrowly (P7). Rewrite nothing else. **ROADMAP.md** §Stage 4: one dated note pointing at the amendments.
6. **PLUGINS.md** via R-COMMIT step 3: `| O-simpleAmbisonics | 📦 Installed | 1.0.0 | Audio Effect (Pedagogical Ambisonic Spatializer) | 2026-10-xx |` (model: row 63).
7. STATUS: `status: stage_4_part2_signoff_pending`, `next_action: user_handson_qual02_signoff`.
8. R-COMMIT with step 3. Message: `docs(O-simpleAmbisonics): v1.0.0 — CHANGELOG, CODE_REVIEW, requirements, architecture amendments, PLUGINS row`.
- **Acceptance:** CHANGELOG checks pass; `git show HEAD:PLUGINS.md | grep '^| O-simpleAmbisonics'` shows `📦 Installed | 1.0.0`; the commit touches only `plugins/O-simpleAmbisonics/**` + that one row.

### ⏸ CP3 — Hands-on + QUAL-02 sign-off *(Taylor; BLOCKING)*
Surface: the installed v1.0.0 in Logic Pro, Ableton Live and the Standalone; headphones for 5–6.
1. **COMPAT-02:** Logic — insert on a **mono** and on a **stereo** audio track with no I/O configuration; audio passes; latency compensated. Ableton Live — same on a mono-source and a stereo track. Typing a preset name in the Save field reaches the field (not host shortcuts) in both hosts.
2. **Carried Logic list:** R1 stopped non-armed track — the well dims, puck / bars / beam follow parameters (record whether Logic suspends `processBlock`); R2 a rear-crossing drag recorded as automation — record whether playback sweeps through the front; R3 CPU editor open (static) vs closed; automation moves the UI; one drag = one gesture; closing mid-drag ends it; hide / re-show resumes meters; save / reopen keeps size, language **and preset name**; 10× close / reopen.
3. **Standalone list:** Tab / arrows; release outside the window; double-click default; hover-help persists; fr / zh relabel; Reduce Motion; **each lesson moves exactly its recipe, never Output Level or Input Mode**; orderStep walk 1 → 2 → 3 → 1; Overhead at order 3 does not clip the output meter in either decoder; preset loads sound click-free.
4. **D16 reflow eye-check:** "1.00 m · in front", orbit line, tilt line, band-tag column, lesson row — in en / fr / zh.
5. **QUAL-02:** Binaural, order 3, distance 1 m, a dry mono source: front (az 0), back (180), left (+90), right (−90), above (el 90) audibly distinct; **+90° louder in the LEFT ear**.
6. **FUNC-09:** Slow Orbit on a source inside a headphone mix — usable as a creative panner.
7. **R9:** long host blocks (> ≈ 100 ms) flipping the well to idle — report if seen.
8. Optional: by-ear recipe notes; fr copy review.

Outcomes: sign-off → T15. A recipe change → `PresetRecipes.h` + the literal ST6 / G-LESSON / G-FACTORY tables + captions if the idea changes; re-run T12; delete `~/Library/O-simpleAmbisonics/Presets/Factory/.factory-version` so disk copies refresh at the same 1.0.0 (the sound always comes from the table); fix commit; if CI already ran, ask whether to re-dispatch (a new "yes").

#### T15 — Record, SUMMARY, STATUS, commit, handoff *(orchestrator)*
**Files:** `.planning/REQUIREMENTS.md`, `.planning/stages/4-polish/SUMMARY.md` (Part 2), `.planning/STATUS.md`. **Depends on:** CP3 (and CP2 / T13 resolved).
- REQUIREMENTS: COMPAT-02 host criterion, QUAL-02 both criteria, FUNC-09 — ticked with Taylor's recorded results (or left open with the reason); statuses / Verified At `stage-4` where proven.
- SUMMARY Part 2: §8 results ×2, digests, pluginval / auval, CI run id + verdict (or "not authorized"), docs, CP1 / CP2 / CP3 notes, deviations.
- STATUS: `status: stage_4_execute_complete`, `current_phase: execute`, `next_phase: verify`, `next_action: plugin-verify`.
- R-COMMIT. Message: `docs(O-simpleAmbisonics): Stage 4 SUMMARY + STATUS (execute complete; hands-on + QUAL-02 recorded)`. **No tag.**
- Handoff, then STOP: Step 1 `/clear`; Step 2 `/plugin-verify O-simpleAmbisonics`. Alternatives: `/plugin-status O-simpleAmbisonics`; after verify, `/publish O-simpleAmbisonics 1.0.0`.

## Dependency graph

```
T1 ─→ T2 ─→ T3 ─┬─→ T5 ─→ T6 ─→ T7 (tracer) ─→ T8 ─→ T9 ─→ T10 ─┐
T1 ─→ T4 ───────┴───────────────────────────────────────────────┴─→ T11 ─→ ⏸CP1 ─→ T12 ─→ ⏸CP2 ─→ T13 ─→ T14 ─→ ⏸CP3 ─→ T15
```
T4 touches only `verify_2_1.py` and may run any time after T1. Everything else is sequential: T3 / T5 / T6 / T7 / T8 / T9 share `PluginProcessor.*`, `PluginEditor.*`, `index.html`, `i18n.js`, `main.cpp`. On "no" at CP2, T13 is skipped.

## Files touched

| Action | Path (under `plugins/O-simpleAmbisonics/` unless noted) | Task |
|---|---|---|
| create | `Source/PresetRecipes.h` | T6 |
| modify | `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp` | T3 (if triaged), T6, T7 |
| modify | `Source/PluginEditor.h`, `Source/PluginEditor.cpp` | T3 (if triaged), T7 |
| modify | `CMakeLists.txt` (`PresetRecipes.h`, `ouaricon_add_module`, one binary-data SOURCE) | T6, T7 |
| modify | `Source/ui/public/index.html` | T3 (if triaged), T5, T8 |
| modify | `Source/ui/public/js/i18n.js` | T5, T9 |
| modify | `tests/render-harness/main.cpp` | T3 (if triaged), T6, T7 |
| modify | `tests/pedalboard/verify_2_1.py` | T4 |
| modify | `tests/pedalboard/verify_3_state.py` | T11 |
| modify | `tests/ui-probes/frame-probe.mjs`, `tests/ui-probes/tooltip-clamp-probe.mjs` | T5, T10 |
| create | `tests/ui-probes/header-fit-probe.mjs` | T5, T10 |
| modify | `tests/i18n-states.json`, `tests/ui-stub/generic-overrides.json` | T9 |
| create | `CHANGELOG.md`, `CODE_REVIEW.md` | T2, T14 |
| create | `.planning/stages/4-polish/critic/*.json`, `.planning/stages/4-polish/SUMMARY.md` | T2, T11, T15 |
| modify | `.planning/{STATUS,REQUIREMENTS,ROADMAP}.md`, `.planning/research/ARCHITECTURE.md` | T1, T11, T14, T15 |
| modify (repo root, one row) | `PLUGINS.md` | T14 |
| conditional (repo root, own commit) | `scripts/i18n-fr-glossary.js`, `scripts/i18n-zh-glossary.js` | T9 |
| generated, never committed | `Source/ui/public/modules/preset-manager.js` | T7 configure |

**Must not touch:** `Source/dsp/*` except a triaged DSP fix (T3, gated); `parameter-spec.md`; `.planning/mockups/*`; `modules/**`; `tests/render-harness/{CMakeLists.txt, ReferenceModel.h, BinauralFilterGoldens.h}` (unless T7's include fallback is needed); `verify_2_2.py` / `verify_2_3.py`; other plugins; the root `CMakeLists.txt`, `.gitignore`, `build/CMakeCache.txt`; `.github/**`.

## Threat register (ASVS L1; boundaries: WebView → natives, disk → preset JSON, host → state blob, repo → public CI)

| ID | Category | Component | Severity | Disposition | Mitigation |
|---|---|---|---|---|---|
| T-4-01 | Tampering | user preset JSON from disk | medium | mitigate | `applyUserPresetJson`: known ids, number / bool only, clamp, owned params ignored, non-object → false (G-PRESET-USER) |
| T-4-02 | Tampering / EoP | names from the page (`savePreset` / `deletePreset` / `loadPreset`) | high | mitigate | `sanitisePresetName`; `loadPreset` accepts walk-order members only (no traversal); factory refusal before I/O; no `savePresetToFile` (S2) |
| T-4-03 | Spoofing | names with bidi / C1 / invisible characters | medium | mitigate | stripped in `sanitisePresetName` (G-PRESET-NAME) |
| T-4-04 | Tampering | restored `currentPreset` from host state | low | mitigate | `isVoid()` + sanitise; absent → `""` (G-PRESET-STATE[restore]) |
| T-4-05 | Tampering | native arguments | low | mitigate | `args.size()` / type checks; every path completes |
| T-4-06 | Info disclosure | signed CI artifacts downloadable ≈ 90 days | medium | accept with consent | listed at CP2 before any dispatch |
| T-4-07 | Integrity | pushing other sessions' commits | medium | mitigate | `origin/main..main` re-listed after fetch; stop on any change (T13 step 1) |
| T-4-SC | Tampering | package installs | low | accept | none added; pedalboard / numpy / scipy in ephemeral `uv run --with` as in Stages 1–3; Playwright chromium already cached |

## Success criteria (goal-backward; verify checks these)

- [ ] **FUNC-08:** 9 factory presets (Init + 8 lessons) from one table that the lesson buttons also read; user scenes save / replace / delete / ◀ ▶ in the header bar; names truthful through edits, loads, restores and reopen — G-FACTORY (+ skew NC), G-LESSON (+ NC), ST6, G-PRESET-* (+ NC), R-BRIDGE 16 / 2, S1–S3, CP1.
- [ ] **R-D2 / R-D3 / R-D4:** lessons = full reset + recipe; `outputLevel` and `inputMode` never moved and never in "modified"; orderStep walks 1 → 2 → 3 → 1 and restarts after a change — G-LESSON[walk / restart / owned], ST6, CP1 item 4, CP3 item 3.
- [ ] **Headroom:** no recipe or step reaches order 3 below 1 m — G-ORDER3-DIST (+ NC).
- [ ] **QUAL-01 preset-load clause:** G-PRESET-CLICK ≤ 2.0 on every transition with the splice NC ≥ 5; CP3 item 3.
- [ ] **Module traps closed:** S2 = 0; state via our own get / set with `currentPreset` (G-PRESET-STATE, verify_3_state S4 / S5); the harness never writes `~/Library/O-simpleAmbisonics`.
- [ ] **Critic:** every finding triaged; W-tier + triaged N fixed with gates + NCs; `CODE_REVIEW.md` at the canonical path, `open_findings: none`.
- [ ] **Carried fixes:** verify_2_1 22/22; header-fit H1 + `encreadn` NC; frame-probe A14 + `chevron` NC.
- [ ] **No regression:** 0 warnings; harness 251 + N all PASS ×2 (logs identical but PERF-02); digests 2.1 / 2.2 PASS, 2.3 reproduced; PERF-01 0; PERF-02 ≥ 20×; verify_2_2 / 2_3 exit 0; `createParameterLayout` / latency untouched (S5).
- [ ] **UI gates:** check-i18n, fr-lint, zh-lint (named + count), check-ui-labels (`seed=overrides`), boot-all-uis `--strict-tips`, frame-probe 14/14, header-fit H1–H3, lesson-row-budget, tooltip-clamp (+ preset anchor), icons `--check` — all exit 0, none 77; fr `reviewed: false`, zh `'mt'` unchanged.
- [ ] **COMPAT-02:** auval SUCCEEDED; pluginval s10 ×2 VST3 + ×2 AU; Windows CI green (or "not authorized" recorded); Logic mono + stereo and Ableton hands-on recorded.
- [ ] **QUAL-02:** Taylor's headphone sign-off recorded (front / back / left / right / above distinct; +90° louder left). **FUNC-09** note recorded.
- [ ] **Release:** installed bundles report 1.0.0; CHANGELOG one `## [1.0.0]`; PLUGINS row `📦 Installed | 1.0.0`; ARCHITECTURE amendments; REQUIREMENTS updated; **no tag**; every commit path-scoped (R-COMMIT step 5).

## Risks

| # | Risk | Mitigation |
|---|---|---|
| K1 | A critic W-fix needs a contract change (params, latency, digest verdicts) | P14 STOP-and-ask before any edit |
| K2 | Header after chip retirement: bar + Format chip overflow in fr / zh at order 3; title / gear shift | P10 pins at the widest language; header-fit H2 / H3 vs the T1 baseline; CP1 language pass |
| K3 | orderStep "unmodified" detection broken by owned-param moves or a restore | P4 / P5 exclude `outputLevel` / `inputMode`; restore override; G-LESSON[owned], G-PRESET-STATE[restore] |
| K4 | Harness or gates write Taylor's real preset folder (harness stamps 1.0.0) | lazy `ensureFactoryBankOnDisk` from natives only; P15; folder listing before / after every harness run |
| K5 | pluginval's editor test loads the page, whose preset JS calls a native → one sentinel-guarded factory write during validation (RESEARCH's "pluginval stays I/O-free" holds for the processor path only) | accepted: message thread, once per version; never on ctor / prepare / process / state |
| K6 | The shared `build/` reconfigure (new module) fails on another session's in-flight CMake | stop and report; never edit foreign files |
| K7 | Static-pair click verdict trips on a legitimate smoothed move | same-motion reference for that arm (T6 note); never raise 2.0 |
| K8 | Stale factory disk copies after a late recipe / name change at the same 1.0.0 | sound and identity always from the table; delete `.factory-version` in the CP3 fix loop |
| K9 | Save field keystrokes swallowed by the host (`EDITOR_WANTS_KEYBOARD_FOCUS FALSE`, as in Wavetable / FM) | CP3 item 1 checks it in Logic and Ableton |
| K10 | The push publishes the plugin and carries 10 O-Octagon commits; suite-wide fr-lint already red on CI | CP2 lists both; T13 re-lists after fetch |
| K11 | MSVC compile of `BinauralFilterData.h` (277 KB constexpr) slow / C3615 in `ShRotator.h` / `ShEncoder.h` | S7 inspection; CI log; Windows-only fixes need a new "yes" to re-dispatch |
| K12 | Long commands stall the 600 s watchdog (cold auval, pluginval ×4, CI watch) | background + poll for every long step |
| K13 | PLUGINS.md is foreign-dirty; a stale-base commit reverts our row or carries theirs | HEAD-blob single-row swap (R-COMMIT 3, 5) |
