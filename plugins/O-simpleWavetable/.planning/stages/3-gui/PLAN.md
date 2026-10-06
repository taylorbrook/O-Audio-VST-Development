# Stage 3 (GUI) — PLAN

**Plugin:** O-simpleWavetable · **Stage:** 3 of 4 (GUI) · **Phases:** 3.1 Layout & Bindings · 3.2 Visualization Panels · 3.3 Import UX, Tooltips, i18n · **Date:** 2026-10-06
**Inputs:** `stages/3-gui/CONTEXT.md` (locked user decisions), `stages/3-gui/RESEARCH.md` (authoritative on *how*: Q1–Q7, P1–P19, Plan A/B/C, Validation Architecture, contradictions 1–8, assumptions A1–A6), `research/ARCHITECTURE.md` (row 17, §Visualization Data Path, §A9, Amendments 12–13), `ROADMAP.md` §Stage 3, `REQUIREMENTS.md` (UI-01..06, PERF-03), `parameter-spec.md` (LOCKED, 21 params), `mockups/v1-{ui.html, i18n.js, i18n-states.json, PluginEditor.h, PluginEditor.cpp, CMakeLists.txt, integration-checklist.md}`, `stages/2-dsp/{PLAN, SUMMARY, VERIFICATION}.md`, the current `Source/`, `tests/` and `CMakeLists.txt`.
**Precedence when sources disagree:** user decisions (CONTEXT + the table below) > this plan's D-table > RESEARCH > mockup v1 + integration checklist > parameter-spec > ARCHITECTURE > ROADMAP > agent-template defaults.

---

## Decisions locked for this plan

### User decisions (CONTEXT 2026-10-06; locked, not re-asked)

| Ref | Decision | Effect |
|-----|----------|--------|
| **CTX-cadence** | 3.1 + 3.2 → build/install → **blocking visual checkpoint** → 3.3 | Part 1 = Tasks 1–12 (execute stops at Task 12). Part 2 = Tasks 13–20. |
| **CTX-lessons** | `applyFactoryPreset(lessonId)` wired **live** with the five checklist recipes. Reset to defaults, never set `output_level`. `aliasDemo` octave jump is page-side. | Task 4, G-LESSON, Task 12. FUNC-08 stays Stage 4. |
| **CTX-venue** | Hands-on in the Standalone + one DAW (Logic, chosen at plan sign-off) | Tasks 17–18. The DAW pass closes the Stage 2 deferred item "save/reopen with Imported" (2-dsp/VERIFICATION.md §Human Verification). |
| **CTX-mockup** | Layout, style, controls, event contract and i18n = mockup v1 + checklist | `index.html` / `i18n.js` copied byte-identical. Changes only for a real gate finding (Task 15). |
| **CTX-natives** | All 8 natives registered | D-Q |
| **CTX-keyboard** | 2 octaves + C, A–K / W–U, Z/X octave, blur releases, via `handleUiMidi` | Page verbatim + `uiMidi` native (Task 3); hands-on Task 17 |
| **CTX-noscope** | No live output scope / spectrum | Out of scope |
| **PLAN-signoff** (Taylor, 2026-10-06, plan time) | D-U = bins 1..1023 (kept); D-Y = single-pass apply (confirmed); D-Z = `importFromBase64` in the processor (approved); DAW for Task 18 = **Logic** | D-U not flipped; Task 18 runs in Logic (AU) |

### Planner resolutions (RESEARCH recommendations adopted; no user input needed)

Continues the plugin-wide series after Stage 2's D-A…D-O.

| ID | Resolution |
|----|-----------|
| **D-P** | **Hash the quantized output payload, not the inputs** (RESEARCH Q1, P1). `lfo = lfo_depth > 0 ? dispLfo : 0`. One helper quantizes every wire value once: `q = llround(x·10^d)`, non-finite → 0. Quanta: 4 dp for cycle / preQ / pos / f0 / nyquistH, 3 dp for thumbs / lfo / menv / amp, 2 dp for dB. FNV-1a runs over the int64 quanta, and the var is built from the same quanta (`q / 10^d`). The hash allocates nothing; the var is built **only on emit**. Result: idle means 0 `cycleUpdate` events and 0 heap allocations per tick, and no `-0`, NaN or inf ever reaches the wire. |
| **D-Q** | **All 8 natives registered in 3.1**, including `importAudio` / `importDroppedAudio` (the import API has existed since 2.4). Gate: the `comm -23` page-vs-C++ diff is empty and `withNativeFunction` count = 8 (P11). |
| **D-R** | **`frame = −1` when Interp is On**, derived from the `interp` param. With Interp On, `dispFrame` holds a rounded frame (WtVoice.h:436, P3). **Silent** (`dispSounding` false, P4): pos = knob, level 0, `frame = interp ? −1 : wt::latchFrame (knob, nF)`, note −1, f0 = nyquistH = 0. |
| **D-S** | **The `bankUpdate` filename comes from `cachedBlob.filename`**, read in the same `bankStateLock` scope as the `importedOwner` copy (P8). Thumbnails are copied outside the lock from the held `shared_ptr`. `ImportStatus.filename` feeds only `importStatus`. |
| **D-T** | **Bank watch + order.** The editor timer emits `bankUpdate` when `getBankDisplayGeneration()` **or** the APVTS bank index changes (P2: the gen bump is audio-thread only). A content hash (bank, imported, numFrames, filename, thumb quanta) drops the identical resend that the audio thread's follow-up gen bump would cause. `uiReady` forces it. The order every tick and in `uiReady` is `bankUpdate` → `importStatus` → `cycleUpdate` (P10). `uiReady` then refreshes `lastBankGeneration`, `lastBankIndex` and `lastImportVersion`. |
| **D-U** | **Discretion call (RESEARCH A4):** the harmonic dB reference is the max \|X_k\| over **bins 1..1023** of the heard cycle. A frame whose strongest partial lies above h32 then shows honestly lower bars. A single named constant `kRefMaxBin = 1023` flips this to 1..32 if Taylor prefers. |
| **D-V** | **Alias Demo = Sine→Saw**, Pos 1.0, Band-limit Off, per the locked checklist. ARCHITECTURE §A9 uses Drive; Stage 4 (FUNC-08) may move it there. |
| **D-W** | **Placement + harness.** Put `CycleView`, `BankThumbs` and `CycleRenderer` in `Source/CycleView.{h,cpp}` (juce_core + juce_dsp only; never the editor or `BinaryData.h`). Add a header-only `Source/VizPayload.h` (quantize, hashes, `cycleToVar` / `bankToVar` / `importToVar`). The processor owns `CycleRenderer vizRenderer`, declared **before** `importPool`, which stays last. It also declares `using CycleView = ::CycleView; using BankThumbs = ::BankThumbs;` so the template's qualified names compile. The console helper compiles every non-editor `.cpp` (ParamDump.cmake:151-159), so the new `viz-check` driver tests the exact renderer and the real wire. **UI-03 is gated offline (RESEARCH Q6)**; Playwright is used only for the layout and tooltip gates. |
| **D-X** | **New display state:** `std::atomic<int> dispNote { -1 }`, `std::atomic<float> dispHz { 0 }`, `std::atomic<double> displayFs { 44100 }` (relaxed, with `static_assert (std::atomic<double>::is_always_lock_free)`). Add a `WtVoice` member `int lastNote = -1;` and `getLastNote()`, set in `startNote`, `noteOnDirect` and `setPitchNote`. **Named `lastNote`, not RESEARCH's `midiNote`:** all three functions take a parameter called `midiNote` (WtVoice.h:263/451/482), and JUCE's `-Wshadow-all` (JUCEHelperTargets.cmake:54) would warn. |
| **D-Y** | **`applyFactoryPreset` is a single pass over `OSimpleWavetable::ParamIDs::all`** via `parameters.getParameter (id)`. This also avoids RESEARCH A5 and the `dynamic_cast`.<br>- The target is the normalized default unless the recipe overrides it.<br>- `output_level` is skipped.<br>- Params already within 1e-6 of their target are skipped.<br>- Each changed param gets begin → `setValueNotifyingHost` → end.<br>- An unknown id returns false and changes nothing. Imported is untouched.<br>This reaches the same end state as "reset to defaults first" (CTX-lessons) without a transient bank switch or doubled host edits. In the definition the parameter is named `id`, and each lesson id is compared as `id == "steppedSmooth"` (etc.), the form the `data-preset` diff gate (S3) parses. |
| **D-Z** | **Drop decode lives in the processor:** `bool importFromBase64 (const juce::String& name, const juce::String& base64)`.<br>- The length cap `WavetableImporter::kMaxMemoryBytes / 3 * 4 + 4` chars is checked **before** decoding. Over it → status error `tooLarge`, return false.<br>- Decode with `juce::Base64::convertFromBase64`. Failure → `unreadable`, return false.<br>- Otherwise call `importFromMemory`.<br>The `importDroppedAudio` native becomes a one-liner, and G-DROP gates the path offline. Instead of a bare `false` (which the page shows as `generic`), the page receives a localized error code. |

### Already settled (do not re-ask)

- **D-O:** VERSION stays `0.1.0` through Stage 3, and the bus layout is unchanged (Logic's per-version AU I/O cache is unaffected).
- **Parameters and layout:**
  - 21 params; ROADMAP's "22" is the documented slip.
  - The bank `<select>` stays English, because its entries are the automation names.
  - The i18n width pins hold (`.title-block` 384, `.import-btn` 124, `.bank-readout` 300, `.tour-buttons` 458, …).
  - The window is fixed at 1120 × 780 and not resizable.
- Empty Imported = silence plus a prompt. The quantizer is mid-rise. Imports shorter than 2048 samples are rejected.
- **Reference code:**
  - The v1 template is the editor base.
  - From O-simpleAdditive, take the JUCE frontend, `insects.png` and the resource branches. Do **not** take its resizable / `editorScale` block or its `applyFactoryPreset` loop, which resets `output_level` (P9).
  - O-Prism is the FileChooser / base64 precedent.
  - **O-simpleFM is not used:** another session has in-flight edits there.
- The Windows build is Stage 4 (COMPAT-02), but `withUserDataFolder()` and static WebView2 linking ship now.
- The FUNC-08 preset-manager bank is Stage 4.

---

## Goal

Replace the Stage 1 generic editor with the finalized single-page WebView "Wavetable Field Guide":
- **Bindings:** all 21 params bound two-way (13 slider, 6 combo and 2 toggle relays), and all 8 natives live.
- **Visualization panels** (bank stack with highlight + marker, current cycle, harmonics 1–32) show the cycle the lead voice is reading now:
  - rendered on the message thread with the voice's own `wt::readSample` + `BitQuantizer`, then FFT'd;
  - gated offline against an independent DFT of the rendered audio;
  - pushed at 30 Hz, with zero audio-thread cost and zero idle traffic.
- **Lesson buttons:** the five buttons apply their recipes live and never touch the output level.
- **Import:** works from the button and from a drop, and shows the filename, the frame count, the empty-Imported state and localized errors.
- **Tooltips and i18n:** every control has a plain-language tooltip in en / fr / zh-Hans, and all five UI gates pass on the real tree.
- **Regression:** Stage 2's DSP output stays bit-identical (goldens unchanged).

---

## Execution model

**Part 1** (3.1 + 3.2) ends in its own commit and a **blocking visual checkpoint**. **Part 2** (3.3) ends with hands-on, then docs and a commit (CTX-cadence).

| Who | Tools | Tasks |
|-----|-------|-------|
| **gui-agent**, four dispatches:<br>- **A** (3.1, the bridge tracer)<br>- **B** (3.2, viz, *including the DSP-side display stores*)<br>- **C** (3.3 import-path gates)<br>- **D** (UI-gate fixes, only if Task 15 finds real defects) | Read / Write / Edit. Bash is for `cp` / `cmp` / `grep` file operations only. **No builds** (gui-agent contract). | 2–4 (A), 6–9 (B), 14 (C), Task 15 fixes (D) |
| **Orchestrator** (plugin-workflow execute) | Bash. Long commands run with `run_in_background: true` and log to `$SCRATCH`, the execute session's scratchpad (600 s watchdog, memory `pattern_executor_watchdog_stall_run_long_commands_in_background`). | 1, 5, 10, 11, 13, 15, 16, 19, 20 |
| **Taylor** (human-verify) | Standalone, Logic | 12, 17, 18 |

**Why gui-agent also does the RT-side additions (dispatch B, Task 6):**
- The audio-thread change is 2 relaxed stores per block plus one `int` assignment at three note sites.
- Keeping the whole viz path in one agent context (atomics → renderer → payload → editor) keeps one contract owner.
- The change is fenced by Task 6's must-nots, an additions-only diff gate on `WtVoice.h`, Stage 2's alloc gate and the golden-log diff (R-GOLD). No dsp-agent dispatch.

**Tracer:** dispatch A + Task 5 prove the WebView bridge end-to-end on the real build before any viz code lands:
- page load → resource provider (BinaryData);
- 21 relays ↔ APVTS;
- 8 natives → processor;
- the timer push (`importStatus`).

Dispatch B then expands from that proven slice.

**Execute-phase stop rules:**
- **Part 1:** `/plugin-execute O-simpleWavetable 3-gui` runs Tasks 1–11, presents Task 12 and **STOPS**. STATUS = `stage_3_part1_complete_visual_pending`.
  - After Taylor's sign-off, a second `/plugin-execute O-simpleWavetable 3-gui` reads STATUS and resumes at Task 13.
  - If the checkpoint finds problems, fix them first: a gui-agent dispatch with Taylor's notes, re-run Tasks 10–11 (a path-scoped fix commit), then repeat Task 12.
- **Part 2:** Tasks 17–18 are presented together after Task 16, and the orchestrator waits in-session for the results.
  - If the session must end first, commit the Part 2 code via R-COMMIT (message suffix `, hands-on pending`) and set STATUS to `stage_3_part2_handson_pending`.
  - The next `/plugin-execute O-simpleWavetable 3-gui` resumes at result intake → Task 19.

**gui-agent overrides** (apply to every dispatch; supersede the gui-agent template):

*Sources and transcription*
- **Read:** CONTEXT, the RESEARCH sections named in the task, this PLAN, parameter-spec, the mockup files named, and `troubleshooting/patterns/juce8-critical-patterns.md`. Where RESEARCH or the template gives code, **transcribe it** with the amendments listed in the task.
- **ASCII-only C++ sources, comments included** (Stage 1 gate; current `Source/` has 0 non-ASCII bytes).
  - `v1-PluginEditor.h` has 11 non-ASCII lines and `v1-PluginEditor.cpp` has 30: emoji markers, `→`, `—`, `≤`. Transliterate them to `->`, `-`, `<=`, `1)` and so on.
  - The copied UI files (`index.html`, `js/i18n.js`) are exempt and stay byte-identical.

*Compiler rules*
- `-Wfloat-equal` is on: use `juce::exactlyEqual`, never `==` on floats.
- `-Wshadow-all` is on: no member, local or parameter may shadow another (hence `lastNote`, D-X).
- Choice and bool params resolve through the processor's `choiceIndex` / `finiteOr` helpers (PluginProcessor.cpp:39-51), exactly as `processBlock` does at cpp:404-406.

*Build wiring*
- Every new `.cpp` goes in `target_sources` (the console helper compiles only what is listed there).
- `BinaryData.h` is included **only** in `PluginEditor.cpp` (P7).
- `OSIW_TEST_HOOKS` code compiles only under `#if OSIW_TEST_HOOKS`.

*Thread safety*
- **Imported bank:** read only through `getImportedBankSnapshot()` or inside one `bankStateLock` scope. Never load `importedForAudio` off the audio thread, and never cache the `shared_ptr` across ticks (Amendment 12, P6).
- **Audio thread:** no allocation, lock or new branch in `processBlock` / `renderBlock` / `renderMono` / `WtVoice::renderNextBlock` beyond exactly what Task 6 specifies.

*Files the agent must not touch*
- Do **not** edit STATUS.md, PLUGINS.md, ARCHITECTURE, parameter-spec, REQUIREMENTS, ROADMAP or CHANGELOG. The report says `"stateUpdated": false`.
- Do **not** touch `mockups/`, other plugins, the root `CMakeLists.txt`, `.gitignore` or `modules/`.
- Do **not** touch `Source/ui/public/modules/webview-drop-streaming.js`: it is generated at configure time and gitignored.

*Contract invariants*
- Keep `setSize (1120, 780)` as a numeric literal, and keep `if (url == "/js/i18n.js")` in that exact form (the gates parse both).
- Lesson presets never set `output_level`.

---

## Recipes (referenced by the tasks)

**R-DBG: out-of-repo Debug harness build.** Never set `OUARICON_BUILD_TESTS` / `SKIP_PLUGINS` in the shared `build/` (its cache has `OUARICON_BUILD_TESTS:BOOL=OFF`). Run in the background.
```bash
bash -c 'SKIP=$(for d in plugins/*/; do n=$(basename "$d"); [ -f "${d}CMakeLists.txt" ] && [ "$n" != O-simpleWavetable ] && printf "%s;" "$n"; done); \
  cmake -S . -B "$SCRATCH/build-oswt" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOUARICON_BUILD_TESTS=ON "-DSKIP_PLUGINS=$SKIP"' > "$SCRATCH/cfg.log" 2>&1
cmake --build "$SCRATCH/build-oswt" --target O-simpleWavetable-bank-check O-simpleWavetable-dsp-check \
  O-simpleWavetable-mod-check O-simpleWavetable-import-check O-simpleWavetable-state-check \
  O-simpleWavetable-param-dump [O-simpleWavetable-viz-check] > "$SCRATCH/build-dbg.log" 2>&1
```
- `viz-check` is built from Task 5 on.
- Each console target depends on the plugin's shared-code target (`add_dependencies`), so this build also compiles `PluginEditor.cpp` and `O-simpleWavetable_UIResources` in Debug.
- **Accept:** exit 0, and `grep -c 'plugins/O-simpleWavetable/.*warning:' "$SCRATCH/build-dbg.log"` = 0.

**R-RUN: run the drivers.**
- Locate each binary with `find "$SCRATCH/build-oswt" -name 'O-simpleWavetable-<x>-check' -type f -perm +111`.
- Run it into `$SCRATCH/<x>.log 2>&1`, and run `dsp-check --alloc-check` into `$SCRATCH/dsp-alloc.log`.
- **Accept:**
  - every exit code is 0;
  - `bank` / `dsp` / `mod` / `import` / `viz`: the last line is `<x>-check: ALL PASS`;
  - `state-check`: 0 `FAIL` lines and 11 PASS;
  - `--alloc-check`: G-ALLOC PASS with 0 allocations and liveness counted;
  - `grep -c 'JUCE Assertion failure'` = 0 in every log.

**R-GOLD: Stage 2 goldens unchanged.** For each of the 6 Stage 2 logs (bank, dsp, dsp-alloc, mod, import, state), diff the `PASS` / `FAIL` lines against Task 1's baseline:
```bash
diff <(grep -E '^(PASS|FAIL)' "$SCRATCH/base-<x>.log") <(grep -E '^(PASS|FAIL)' "$SCRATCH/<x>.log")
```
- **Allowed differences:** only timing / scheduling counts (the bank-check `G-TIME` / `buildMillis` value and the import-check soak block count).
- **Anything else is a regression → stop.**
- For the record, the reference values (2-dsp/VERIFICATION.md §Re-verification):

| Gate | Value |
|------|-------|
| G-PITCH | 0.0009 c |
| G-Q2-C8 | −114.3 dB |
| G-Q2-SWEEP | −74.9 dB |
| G-Q2-PULSE | −67.0 dB |
| G-POLY | stolen 49 −88.5 dB |
| G-RETRIG | 0.00856 / 0.00856 |
| G-VEL | −11.905 dB |
| G-BLOCK | bit-identical |
| G-CLICK-SQ / G-CLICK-SH | 1.414 / 1.144 |
| G-STEAL | 1.076× |
| G-SWITCH-TAIL | 0.656× |
| G-RETRIG-VEL | 0.589× |
| QUAL-03 | exact ≤ 6e-8, ratio worst 1.31 |
| `--alloc-check` | 0 |

**R-UISTATIC: the three CI gates** (repo root, no Playwright; `.github/workflows/ui-static-gates.yml` runs them on every push):
- `node scripts/check-i18n.js --plugin O-simpleWavetable` → exit 0 (exit = failed-assertion count) and 16/16 assertions.
- `node scripts/i18n-fr-lint.js --plugin O-simpleWavetable` → exit 0, CLEAN.
- `node scripts/i18n-zh-lint.js --plugin O-simpleWavetable` → exit 0 **and** the output names O-simpleWavetable with its entry count (142 at `reviewed: 'mt'` at finalization). This script `return`s with exit 0 when `--plugin` matches nothing (i18n-zh-lint.js:652), so exit 0 alone is vacuous.

**R-UIPW: the two Playwright gates** (repo root):
- `node scripts/check-ui-labels.js --plugin O-simpleWavetable` → exit 0 across the 8 states of `tests/i18n-states.json`, including the [7] pins.
- `node scripts/boot-all-uis.js --plugin O-simpleWavetable --strict-tips` → exit 0. The O-simpleWavetable row must show 0 DEAD, 0 late tips, 0 404 and 0 page errors. Exit 2 means a DEAD tip binding.
- **Exit 77 = Playwright not resolved = NOTHING verified, never a pass** (P17). Run `npx playwright install chromium` and re-run. At plan time chromium / headless-shell 1228 and 1234 are cached.
- The gate output must name O-simpleWavetable's page; another session's test server on the same port serves a look-alike page (memory `pattern_ui_test_server_port_clash_serves_other_session`).
- Screenshots go to `$SCRATCH`, never the repo.

**R-INSTALL: build, install, validate** (each long step in the background):
1. `./scripts/build-and-install.sh O-simpleWavetable` (cache clear + `-dev` / unsuffixed sweep; VST3 + AU only).
2. `ninja -C build O-simpleWavetable_Standalone`. The script never rebuilds the Standalone (P14).
3. Freshness:
   - `strings <installed VST3 binary> | grep -c cycleUpdate` ≥ 1, where the binary is `~/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3/Contents/MacOS/O-simpleWavetable-dev`;
   - the same ≥ 1 on `build/plugins/O-simpleWavetable/O-simpleWavetable_artefacts/Release/Standalone/O-simpleWavetable-dev.app/Contents/MacOS/O-simpleWavetable-dev`;
   - the Standalone's mtime ≥ the VST3's.
4. `bash scripts/verify-au-link.sh O-simpleWavetable` (= `auval -v aumu OSiW OuDv`; **never `auval -a`**).
   - A cold registry rescan after the cache clear can take minutes; wait it out.
   - **Accept:** `AU VALIDATION SUCCEEDED`; only the 2 known benign skew-default warnings (`lfo_rate`, `amp_attack`).
5. `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate <bundle>` for the VST3 and for `~/Library/Audio/Plug-Ins/Components/O-simpleWavetable-dev.component`.
   - Keep the GUI tests **on**: the WebView editor must open.
   - **Accept:** exit 0, `SUCCESS`, 0 `FAILED`.
- If the shared `build/` re-configure fails on another plugin's in-flight CMake edit (O-simpleFM, O-Gain, O-Reed, O-Bassoon and O-Freeze are dirty), **stop and report**. Never edit foreign files.

**R-COMMIT: path-scoped temp-index CAS commit** (memory `index_git_shared_checkout`; run under `bash -c`, the zsh pathspec and `:`-modifier traps):
1. **Immediately before**, check:
   - `git branch --show-current` = `main`;
   - `git status --short`;
   - `git diff --cached --stat`. Note any foreign-staged files; PLUGINS.md is `MM` at plan time. Never stage or reset them.
2. `export GIT_INDEX_FILE="$SCRATCH/idx"; old=$(git rev-parse HEAD); git read-tree "$old"; git add -- plugins/O-simpleWavetable`.
   - This respects `.gitignore`, so the drop module stays out.
   - A pathspec `git commit` would skip the new untracked files.
3. **Part 2 only:** build the PLUGINS.md blob from `git show "${old}:PLUGINS.md"`, substituting **only** the O-simpleWavetable row. Then `git hash-object -w` → `git update-index --cacheinfo "100644,<blob>,PLUGINS.md"`.
4. `tree=$(git write-tree); new=$(git commit-tree "$tree" -p "$old" -F "$SCRATCH/msg")`.
5. `git update-ref refs/heads/main "$new" "$old"`. This is a CAS: if HEAD moved, redo steps 2–5 from the new HEAD. Then `unset GIT_INDEX_FILE`.
6. **After:**
   - `git rev-parse HEAD` = `$new`;
   - `git show --stat --format= HEAD` lists only `plugins/O-simpleWavetable/**` (+ `PLUGINS.md` in Part 2);
   - `git ls-tree -r --name-only HEAD -- plugins/O-simpleWavetable/Source/ui/public/modules` is empty (P15).
7. **Resync:**
   - `git reset -q -- plugins/O-simpleWavetable` (own paths only; never PLUGINS.md).
   - In Part 2, inspect `git diff --cached -- PLUGINS.md`. If the foreign staged blob would revert this row, patch **only this row** inside their blob.
8. Never `git add -A`, `commit -a`, a bare `reset`, or any tag.

---

## Tasks

### Part 1: Phases 3.1 + 3.2

#### Task 1 — Pre-flight + Stage 2 baseline  *(orchestrator)*
- **Do:**
  - **Branch and tree:** `git branch --show-current` = `main`. `git status --short -- plugins/O-simpleWavetable` must be empty (the plan-phase commit landed this PLAN). Otherwise stop and report.
  - **Contract tamper check:** compare `shasum -a 256` of `BRIEF.md`, `parameter-spec.md`, `research/ARCHITECTURE.md` and `ROADMAP.md` against STATUS `contract_checksums`.
    - **Known at plan time:** `architecture` mismatches. STATUS holds c9727200's hash (`c1f8bc0d…`); the file is 850df9b9's (`7753e1a2…`), from the gap-closure W5 edit, 6+/4−, Amendments 12–13.
    - Accept that mismatch only if `git diff c9727200 HEAD -- plugins/O-simpleWavetable/.planning/research/ARCHITECTURE.md` is exactly that dated W5 edit. Then set `contract_checksums.architecture: sha256:7753e1a28f1e7762fd8418d0c7ca3c9f6ac579fca423ad97efd477189bb4b2d5`; it rides in the Part 1 commit.
    - Any other mismatch → stop.
  - Stage 2 is VERIFIED (2-dsp/VERIFICATION.md §Re-verification).
  - Run `bash -c '.planning/workflow/scripts/run-gate.sh O-simpleWavetable 2-dsp 3-gui --skip-review'`.
    - Exit 0 → proceed.
    - Exit 1 → bypass with `--force` plus a stdin justification **only** for a known gate limitation, never a real build or pluginval failure.
  - **Baseline:** R-DBG (no `viz-check` yet) on the unmodified tree, then R-RUN. Save the logs as `$SCRATCH/base-{bank,dsp,dsp-alloc,mod,import,state}.log`; R-GOLD diffs against them. This build also warms the Debug tree.
- **Depends on:** none.
- **Accept:**
  - on `main` with a clean tree;
  - checksums match, or the W5 refresh is logged;
  - the gate passed, or the bypass is logged;
  - 6 baseline logs that satisfy R-RUN.

#### Task 2 — UI files + CMake  *(gui-agent, dispatch A)*
- **Files:**
  - create `Source/ui/public/index.html`, `Source/ui/public/js/i18n.js`, `Source/ui/public/js/juce/index.js`, `Source/ui/public/js/juce/check_native_interop.js`, `Source/ui/public/img/insects.png` and `tests/i18n-states.json`;
  - edit `CMakeLists.txt`.
- **Do:**
  - **Copy with Bash `cp`** (byte-identical; `insects.png` is binary, so never via Write):
    - `.planning/mockups/v1-ui.html` → `index.html`;
    - `v1-i18n.js` → `js/i18n.js`;
    - `plugins/O-simpleAdditive/Source/ui/public/js/juce/{index.js, check_native_interop.js}` → `js/juce/`;
    - `plugins/O-simpleAdditive/Source/ui/public/img/insects.png` → `img/`;
    - `v1-i18n-states.json` → `tests/i18n-states.json`.
    - EB Garamond is **not** copied: CMake embeds it from `modules/ui/eb-garamond`.
  - **CMake** (merge `v1-CMakeLists.txt`; RESEARCH §CMake Changes):
    - Add `juce_add_binary_data(O-simpleWavetable_UIResources SOURCES …)` **before** `target_link_libraries`, with exactly the 10 v1 entries:
      - the 6 `Source/ui/public/…` files, including `modules/webview-drop-streaming.js`;
      - `${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/css/eb-garamond.css`;
      - the 3 `EBGaramond-{Regular,Italic,Bold}.woff2`.
      - Use the default `BinaryData` namespace (there is no second binary-data target).
    - Add `O-simpleWavetable_UIResources` to the existing `PRIVATE` link list.
    - In the `OUARICON_BUILD_TESTS` block, add `ouaricon_add_processor_console(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source ${CMAKE_CURRENT_SOURCE_DIR}/tests/viz-check/main.cpp viz-check)` and `target_compile_definitions(O-simpleWavetable-viz-check PRIVATE OSIW_TEST_HOOKS=1)`.
    - **Verify, don't duplicate:**
      - `ouaricon_add_module(O-simpleWavetable webview-drop-streaming)` (already at CMakeLists.txt:52);
      - `IS_SYNTH`, `NEEDS_MIDI_INPUT`, `NEEDS_WEB_BROWSER`, `NEEDS_WEBVIEW2`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`;
      - `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`;
      - `juce::juce_gui_extra` and `juce::juce_dsp`.
    - `VERSION "0.1.0"` is unchanged (D-O).
- **Must not:** create or edit anything under `Source/ui/public/modules/`; edit O-simpleAdditive or the mockups.
- **Depends on:** Task 1.
- **Accept** (checked in Task 5):
  - `cmp` on the 6 copy pairs prints nothing;
  - `grep -c 'O-simpleWavetable_UIResources' CMakeLists.txt` = 2;
  - `grep -c 'ouaricon_add_module(O-simpleWavetable webview-drop-streaming)' CMakeLists.txt` = 1;
  - `sed -n '/juce_add_binary_data(O-simpleWavetable_UIResources/,/^)/p' CMakeLists.txt | grep -cE '\.(html|js|png|css|woff2)'` = 10;
  - `git check-ignore -q Source/ui/public/modules/webview-drop-streaming.js` exits 0.

#### Task 3 — Editor from the v1 template (bridge)  *(gui-agent, dispatch A)*
- **Files:** rewrite `Source/PluginEditor.h` and `Source/PluginEditor.cpp`.
- **Do:** adapt `mockups/v1-PluginEditor.{h,cpp}` per RESEARCH Q5 ("adapt, don't paste"). Keep:
  - the class name and constructor, `private juce::Timer`, `kTimerHz = 30`, `startTimerHz (kTimerHz)`, and `stopTimer()` in the dtor;
  - **member order:** 21 relays (13 `WebSliderRelay`, 6 `WebComboBoxRelay`, 2 `WebToggleButtonRelay`), then `webView`, then 21 attachments in the same order;
  - `.withOptionsFrom` on all 21, and 3-arg attachments through `attach<>()`;
  - `.withNativeIntegrationEnabled()` and `.withKeepPageLoadedWhenBrowserIsHidden()`;
  - **resource provider:** bare-path equality branches for exactly these 11 URLs, with `charset=utf-8` on text and the exact `if (url == "/js/i18n.js")` form (check-i18n [8]):
    - `/`, `/index.html`
    - `/js/i18n.js`, `/js/juce/index.js`, `/js/juce/check_native_interop.js`
    - `/modules/webview-drop-streaming.js`
    - `/img/insects.png`
    - `/css/eb-garamond.css`
    - `/fonts/EBGaramond-{Regular,Italic,Bold}.woff2`
  - `#if JUCE_WINDOWS`: `withWinWebView2Options` with `withUserDataFolder (tempDirectory/"OsimpleWavetable_WebView")`, set **unconditionally** inside the block, plus status bar and error page disabled;
  - `setSize (1120, 780)` (numeric literal) and `setResizable (false, false)`.
- **The 8 natives (D-Q):**
  - `getParameterDefaults`: iterate `OSimpleWavetable::ParamIDs::all`; drop the template's duplicate `kAllParamIds`.
  - `getUiLanguage` / `setUiLanguage`: as the template.
  - `uiReady`: see "Dispatch A push scope" below.
  - `importAudio`: call `complete (juce::var (true))` **before** `launchAsync`. The callback captures `SafePointer` + `chooser`. If `safeThis == nullptr`, `return` without touching the bridge (P12).
  - `importDroppedAudio`: `args.size() >= 2`, then `complete (juce::var (processorRef.importFromBase64 (args[0].toString(), args[1].toString())))` (D-Z). No decoding in the editor.
  - `uiMidi`: as the template.
  - `applyFactoryPreset`: as the template; it completes with the bool.
- **Dispatch A push scope:**
  - `emitImportStatus()` exists, and the timer watches `getImportStatusVersion()`.
  - `uiReady` calls `emitImportStatus()`, refreshes `lastImportVersion` and completes `true`.
  - `bankUpdate` / `cycleUpdate` arrive in Task 8. This is a functionality gap, not an architectural one: the page tolerates absent events.
- **Transcription fixes:**
  - make it ASCII-only;
  - replace the stale "worker → callAsync publish" comment (RESEARCH contradiction 6): the publish is D-M, an AsyncUpdater;
  - no resizable / `editorScale` block;
  - `#include "BinaryData.h"` in this file only.
- **Depends on:** Task 2.
- **Accept:** Task 5 static gates S1–S10.

#### Task 4 — Lesson presets, drop API, `viz-check` skeleton + G-LESSON  *(gui-agent, dispatch A)*
- **Files:** edit `Source/PluginProcessor.h` and `Source/PluginProcessor.cpp`; create `tests/viz-check/main.cpp`.
- **Do:**
  - **`bool applyFactoryPreset (const juce::String& id)`** (message thread; documented, not asserted). Keep the parameter name `id` and the comparison form `id == "<lesson>"`; S3 parses it:
    - Transcribe RESEARCH Q7's recipe table verbatim. These are checklist §Lesson preset ids, choice indices verified at cpp:129-160:
      - `steppedSmooth`: bank 0, pos 0.5, interp 0, lfoSync 0, lfoShape 1, lfoRate 0.18, lfoDepth 1.0
      - `aliasDemo`: bank 0, pos 1.0, bandlimit 0 (D-V)
      - `driveSweep`: bank 4, pos 0, envAmount 1.0, menv A 0.9 / D 1.6 / S 0.25 / R 0.8
      - `vowelPad`: bank 3, pos 0.5, lfoSync 0, lfoShape 0, lfoRate 0.12, lfoDepth 0.9, ampAttack 0.6, ampRelease 1.4
      - `ppg8bit`: bank 1, pos 0.6, interp 0, bitDepth 9, lfoSync 0, lfoShape 4, lfoRate 3.0, lfoDepth 0.4
    - Apply rule: **D-Y**.
    - Add `jassert (parameters.getParameter (s.id) != nullptr)` for every recipe entry. The Debug run turns a typo into a logged assertion.
    - The recipe container may be a `std::vector` (message thread). Never assign a braced list to an `initializer_list`.
  - **`bool importFromBase64 (const juce::String& name, const juce::String& base64)`** (D-Z; any non-audio thread). Use `WavetableImporter::sanitiseName (name)` for every status it sets.
  - **`tests/viz-check/main.cpp`:**
    - **Scaffold:**
      - `report()` / `info()`, with the final line `viz-check: ALL PASS` or `viz-check: FAILURES = n`, and exit non-zero on failure;
      - dsp-check's `setParam` / `Rig` (tests/dsp-check/main.cpp:234-327);
      - the `malloc_logger` alloc hook (dsp-check:105-200), with the counted thread made **settable** (`gArmedThread`) instead of fixed to the audio thread.
    - **G-LESSON** (CTX-lessons, D-Y):
      - **Setup:** set all 21 params to fixed-seed random normalized values, with `output_level` = −17.3 dB. Attach an `AudioProcessorParameter::Listener` to every param that counts `parameterGestureChanged` begin/end per param.
      - **For each of the 5 ids, assert:**
        - it returns true;
        - `output_level` is still −17.3 (normalized, within 1e-6) with 0 gestures;
        - every other param equals its recipe value if listed, else its default (normalized, 1e-6);
        - each changed param got exactly 1 begin + 1 end, and unchanged params got 0;
        - the `getImportedBankSnapshot()` pointer is unchanged.
      - Re-applying the same id → 0 gestures.
      - `"nope"` and `""` → false, all 21 values unchanged, 0 gestures.
      - **Negative control:** a test-local naive loop that resets every param *including* `output_level`, run through the same assertion function, must FAIL the `output_level` check. Print `FAILS as designed`.
- **Depends on:** Tasks 2–3.
- **Accept** (Task 5):
  - G-LESSON PASS for all 5 ids plus the unknown-id case;
  - the negative control prints `FAILS as designed`;
  - 0 `JUCE Assertion failure`.

#### Task 5 — 3.1 gate: static, offline, CI UI gates, boot  *(orchestrator)*
- **Static gates** (from `plugins/O-simpleWavetable`):
  - **S1 ASCII:** `LC_ALL=C grep -n '[^[:print:][:space:]]' Source/*.h Source/*.cpp tests/*/main.cpp CMakeLists.txt` prints nothing. `grep -c PLUGIN_VERSION CMakeLists.txt` = 0.
  - **S2 Native diff** (RESEARCH, run under bash) prints nothing, and `grep -c 'withNativeFunction ("' Source/PluginEditor.cpp` = 8:
    ```bash
    comm -23 \
      <(grep -o "getNativeFunction('[A-Za-z]*')" Source/ui/public/index.html | sed "s/.*('\(.*\)')/\1/" | sort -u) \
      <(grep -o 'withNativeFunction ("[A-Za-z]*"' Source/PluginEditor.cpp | sed 's/.*("\(.*\)"/\1/' | sort -u)
    ```
  - **S3 Lesson id diff** prints nothing (5 ids on each side):
    ```bash
    comm -3 <(grep -o 'data-preset="[A-Za-z0-9]*"' Source/ui/public/index.html | sed 's/.*="\(.*\)"/\1/' | sort -u) \
            <(grep -o 'id == "[A-Za-z0-9]*"' Source/PluginProcessor.cpp | sed 's/.*"\(.*\)"/\1/' | sort -u)
    ```
  - **S4 Resource branches:** for each of the 11 URLs in Task 3, `grep -qF "url == \"$u\"" Source/PluginEditor.cpp || echo "MISSING $u"` prints nothing.
  - **S5 Relays and attachments:** in `PluginEditor.h`, `std::unique_ptr<juce::WebSliderRelay>` = 13, `WebComboBoxRelay>` = 6, `WebToggleButtonRelay>` = 2, `Web[A-Za-z]*ParameterAttachment>` = 21. `grep -c '\.withOptionsFrom (' Source/PluginEditor.cpp` = 21.
  - **S6 Member order:** the line of the last `…Relay>` declaration < the `std::unique_ptr<juce::WebBrowserComponent>` line < the line of the first `…ParameterAttachment>` declaration.
  - **S7 Template invariants:** `grep -c` = 1 for each of `setSize (1120, 780)`, `setResizable (false, false)`, `url == "/js/i18n.js"`, `withUserDataFolder` and `withKeepPageLoadedWhenBrowserIsHidden`.
  - **S8 BinaryData:** `grep -l 'BinaryData.h' Source/*.h Source/*.cpp` = `Source/PluginEditor.cpp` only.
  - **S9 Base64:** `grep -rn 'fromBase64Encoding\|toBase64Encoding' Source/` = 0; `grep -c 'convertFromBase64' Source/PluginProcessor.cpp` ≥ 1.
  - **S10 Chooser:** in the `importAudio` lambda, the `complete (juce::var (true))` line precedes `launchAsync`, and the callback's first statement is `if (safeThis == nullptr) return;` (inspect + `grep -n`).
  - **S11 Copies and ignore:** Task 2's `cmp` and `git check-ignore` checks.
- **Offline:**
  - R-DBG, now including `O-simpleWavetable-viz-check`. The CMake change triggers a reconfigure.
  - R-RUN on all 6 drivers + `--alloc-check`.
  - R-GOLD.
  - The viz-check log shows G-LESSON as Task 4 accepts.
- **UI:**
  - R-UISTATIC must be green **before** the commit that adds `index.html` / `i18n.js` (P18).
  - `node scripts/boot-all-uis.js --plugin O-simpleWavetable --strict-tips` must pass per R-UIPW (exit 77 handling).
  - If a 404 hits only the fonts or the drop module, first confirm the URL against S4. Gate-built trees can 404 module assets that the binary serves (memory `pattern_hand_built_gate_tree_404s_module_embedded_assets`).
- **On failure:** send the failing output back through gui-agent dispatch A. Nothing is loosened.
- **Depends on:** Tasks 2–4.

#### Task 6 — Display state for note, pitch and rate  *(gui-agent, dispatch B; touches the audio thread)*
- **Files:** edit `Source/WtVoice.h`, `Source/PluginProcessor.h` and `Source/PluginProcessor.cpp`.
- **Do (D-X):**
  - **`WtVoice`:**
    - add `int lastNote = -1;` and `int getLastNote() const noexcept { return lastNote; }` beside the display getters (WtVoice.h:493-501);
    - add `lastNote = midiNote;` next to each `noteHz = …` in `startNote` (:270), `noteOnDirect` (:454) and `setPitchNote` (:484).
    - **Nothing else in WtVoice changes.**
  - **Processor:**
    - Add the three atomics, the `static_assert`, and getters `getDisplayNote()`, `getDisplayHz()` and `getDisplaySampleRate()` in the style of the existing ones (PluginProcessor.h:178-184).
    - `updateDisplayFromLeadVoice()`: the lead branch adds `dispNote.store (lead->getLastNote(), relaxed)` and `dispHz.store ((float) lead->getCurrentHz(), relaxed)`. The no-lead branch stores −1 and 0.
    - `prepareToPlay()`: next to the existing disp resets (cpp:328-331), store `displayFs = sampleRate`, `dispNote = -1` and `dispHz = 0`.
- **Must not:**
  - change any other line of `processBlock`, `renderBlock`, `renderMono` or any WtVoice render path;
  - add a lock, an allocation or a branch;
  - read `getSampleRate()` off the audio thread (a plain double: data race).
- **Depends on:** Task 5.
- **Accept** (Task 10):
  - `git diff --numstat -- Source/WtVoice.h` shows 0 deleted lines and ≤ 6 added;
  - R-GOLD is clean and `--alloc-check` = 0;
  - G-VIZ-NOTE passes.

#### Task 7 — Renderer, payload, processor viz API  *(gui-agent, dispatch B)*
- **Files:**
  - create `Source/CycleView.h`, `Source/CycleView.cpp` and `Source/VizPayload.h`;
  - edit `Source/PluginProcessor.h` and `Source/PluginProcessor.cpp`;
  - edit `CMakeLists.txt` (`target_sources`: `Source/CycleView.cpp`, `Source/CycleView.h`, `Source/VizPayload.h`).
- **`CycleView.h`** (D-W): the RESEARCH Q1 shapes.
  - **`CycleView`:**
    - `kPoints = 256`, `kHarmonics = 32`;
    - arrays `cycle`, `preQ`, `harmonicsDb`, `harmonicsRawDb`;
    - scalars `pos`, `frame`, `level`, `kmax`, `nyquistH`, `note`, `f0`, `lfo`, `menv`, `amp`;
    - flags `sounding`, `quantized`, `empty`.
  - **`BankThumbs`:** `bank`, `imported`, `numFrames`, `filename`, `std::vector<float> points`.
  - `static_assert (WavetableBank::kTableSize == 2048 && CycleView::kPoints * 8 == WavetableBank::kTableSize)` and `static_assert (WavetableBank::kmax (0) == 1023 && WavetableBank::kmax (10) == 1)`.
  - Includes only `<juce_core>`, `<juce_dsp>`, `WavetableBank.h`, `WtRead.h` and `BitQuantizer.h`.
- **`CycleRenderer`** (`CycleView.cpp`, RESEARCH Q3):
  - **Members:** `juce::dsp::FFT fft { 11 }`, `std::array<float, 4096> work` (FFT needs 2·size, juce_FFT.h:111), and `std::array<float, 2048> heard, pre, raw`. All are sized at construction.
  - **Signature:** `void render (const WavetableBank* b, int level, bool interp, float pos, int latched, int bitIdx, CycleView& v) noexcept`.
  - **Body** (transcribe Q3's algorithm):
    1. For j = 0..2047: `wt::readSample (*b, level, j / 2048.0, interp, pos, latched)`, then `BitQuantizer::apply`. Same order as WtVoice.h:398.
    2. Point-sample every 8th sample into `cycle` / `preQ`.
    3. Zero-pad `work`, then `performFrequencyOnlyForwardTransform (work.data(), true)`.
    4. `ref` = max over bins 1..`kRefMaxBin` (D-U).
    5. dB clamped to [−60, 0].
    6. `harmonicsRawDb`: when level > 0, render the level-0 cycle (same quantizer) and measure it against the **same** `ref`, clamped ≤ 0; when level = 0, copy.
    7. `b == nullptr` → zeros, −60 everywhere, `empty = true`.
  - No allocation and no lock. Message thread only and non-reentrant: documented, not asserted (the harness calls it from main).
- **Processor:**
  - Type aliases `using CycleView = ::CycleView; using BankThumbs = ::BankThumbs;`.
  - `CycleRenderer vizRenderer;` declared **before** `importPool` (importPool stays last; destroyed first).
  - `int getSelectedBankIndex() const noexcept`: `choiceIndex (pBank->load(), kNumBanks)`.
  - **`void buildCycleView (CycleView& v)`** follows RESEARCH Q3's sketch:
    - the bank: a built-in via `builtIns->get`, or Imported via a local `hold = getImportedBankSnapshot()` for this call only;
    - `interp` / `bit_depth` read as cpp:404-406;
    - `sounding` → `dispPos` / `dispLevel` / `dispFrame`; silent → D-R;
    - `kmax = WavetableBank::kmax (level)`;
    - `note = sounding ? dispNote : −1`, `f0 = sounding ? dispHz : 0`, and `nyquistH = sounding && f0 > 0 ? 0.5·displayFs/f0 : 0`;
    - `lfo` per D-P; `menv` / `amp` from `dispMenv` / `dispAmp`.
  - **`void getBankThumbnails (BankThumbs& t) const`** per D-S:
    - built-in: copy `bank.thumbs`;
    - Imported: inside one `bankStateLock` scope copy `importedOwner` + `cachedBlob.filename`, then copy the thumbs outside the lock;
    - empty: `numFrames` 0, filename empty;
    - `points.assign(...)` reuses the capacity.
- **`VizPayload.h`** (header-only, `inline`; D-P, D-T):
  - `quantize (double, int dp) -> juce::int64`: non-finite → 0.
  - `cycleHash (const CycleView&) -> juce::uint64`: FNV-1a over the quanta of every field the var carries, including `quantized` and `sounding`. No allocation.
  - `cycleToVar` keys, exactly: `cycle, harmonics, harmonicsRaw, preQ` (only when `quantized`), `pos, frame, level, kmax, nyquistH, note, f0, lfo, menv, amp, sounding`.
  - `bankHash` / `bankToVar` keys: `bank, imported, numFrames, filename, frames` (N arrays of 128, 3 dp).
  - `importToVar (const OSimpleWavetableAudioProcessor::ImportStatus&)` keys: `state` (`idle|busy|done|error`), `filename, frames, error`.
  - It includes `PluginProcessor.h`; the processor never includes `VizPayload.h`.
- **Must not:**
  - touch `importedForAudio`;
  - keep a snapshot past the call;
  - include `BinaryData.h`;
  - allocate inside `render`, `buildCycleView` or `cycleHash`.
- **Depends on:** Task 6.

#### Task 8 — Editor viz wiring  *(gui-agent, dispatch B)*
- **Files:** edit `Source/PluginEditor.h` and `Source/PluginEditor.cpp`.
- **Do:**
  - **Members:** `CycleView cycleView;`, `BankThumbs bankThumbs;` (with `bankThumbs.points.reserve (256 * 128)` in the ctor), `int lastBankIndex`, `juce::uint64 lastBankHash`, `lastCycleHash`, and `bool forceCycleEmit = true`.
  - **Emitters:**
    - `emitBankUpdate (bool force)`: `getBankThumbnails` → `bankHash` → skip if `! force` and unchanged (D-T) → `bankToVar` → `emitEventIfBrowserIsVisible ("bankUpdate", …)`.
    - `emitCycleUpdate (bool force)`: `buildCycleView (cycleView)` → `cycleHash` → skip if `! force` and unchanged → `cycleToVar` → emit (D-P).
    - `emitImportStatus()` uses `importToVar`.
  - **`timerCallback`:**
    1. If gen ≠ `lastBankGeneration` **or** `getSelectedBankIndex()` ≠ `lastBankIndex`: update both, `emitBankUpdate (false)`, `forceCycleEmit = true`.
    2. Import version change → `emitImportStatus()`.
    3. `emitCycleUpdate (std::exchange (forceCycleEmit, false))`.
  - **`uiReady`:** `emitBankUpdate (true)` → `emitImportStatus()` → `forceCycleEmit = true` → refresh the three counters → `complete (true)`. Keep both the page's load-time call and its `visibilitychange` re-send (RESEARCH contradiction 5: harmless belt-and-braces).
  - **`applyFactoryPreset` native:** set `forceCycleEmit = true` after the apply. The bank change itself reaches the page through the index watch.
  - **Ctor:** snapshot gen, index and import version after the WebView exists (template).
- **Depends on:** Task 7.

#### Task 9 — `viz-check` gates (UI-01..03, PERF-03)  *(gui-agent, dispatch B)*
- **Files:** edit `tests/viz-check/main.cpp`.
- **Do:** every RESEARCH Q6 gate, plus G-VIZ-NOTE.
  - **Every comparison goes through the real wire:** `buildCycleView` → `cycleToVar` / `bankToVar` → `juce::JSON::toString (var, true)` → `juce::JSON::parse`.
  - Assert the **exact key set** of each parsed object, and that every number is finite.
  - Print the measured values. A negative control prints `FAILS as designed (×margin)`; one that *passes* counts as a failure.
- **G-VIZ-EXACT:**
  - *Stimulus:* fs = 901120 (= 440·2048), note 69, amp A 0.001 / D 0.001 / S 1, output 0 dB, LFO depth 0, env amount 0. Take 2048 samples a whole number of periods after note-on.
  - *Matrix:* banks 0–4 × pos {0, 0.37, 1} × interp {On, Off} × bit_depth idx {0, 9, 14}.
  - *Assert:*
    - `cycle[j]·g` vs `y[8j]` ≤ 2e-4 abs, where g is the least-squares gain;
    - `harmonics` vs an **independent double-precision direct DFT** of y (ref = max over 1..1023, not the JUCE FFT) ≤ 0.02 dB for bars > −59.5;
    - `level` 1, `kmax` 512, `frame` = −1 (On) or `latchFrame (pos, 32)` (Off), `pos` = knob ± 1e-4, `note` 69, `f0` 440 ± 1e-3, `nyquistH` 1024 ± 1e-2, `sounding` true.
- **G-VIZ-CEIL:**
  - *Stimulus:* fs 48000, Sine→Saw pos 1, note 96. Render 1 s; measure with a Blackman-Harris window and a double-precision Goertzel at k·f0.
  - *Assert:*
    - Band-limit On: `level` 7, `kmax` 8; k > 8 at −60; k ≤ 8 within ±0.5 dB; `nyquistH` 11.47 ± 1e-2.
    - Band-limit Off: `level` 0, `kmax` 1023, k ≤ 11 within ±0.5 dB.
- **G-VIZ-SILENT** (P1, P4, D-R):
  - *Stimulus:* no note, knob 0.6, Interp Off, Sine→Square, LFO free 5 Hz at depth 0.
  - *Assert:* `sounding` false, `note` −1, `level` 0, `frame` 19, `pos` 0.6; and two builds 100 blocks apart give the **same** `cycleHash` (idle-quiet).
  - *Negative control:* at depth 0.5 the two hashes must differ.
- **G-VIZ-NOTE** (P5, D-X):
  - Poly note 60 → `note` 60, `f0` 261.63 ± 0.01. Wheel at +8191 → `f0` 293.66 ± 0.05 and `nyquistH` = 0.5·fs/f0 ± 1e-2.
  - Mono: hold 60, legato 64 → 64; release 64 → 60 (return to held); release all → after the tail `sounding` false and `note` −1. The values must change at every step.
- **G-VIZ-IMPORTED** (UI-01, P8, D-S). Use the **real** import path; `publishImportedBankForTesting` never sets `cachedBlob` (cpp:1020-1025).
  1. Build an in-memory 16-bit WAV of 3 × 2048 samples (3 distinct sines). Call `importFromMemory ("three frames.wav", …)`, poll the status until `done` (≤ 10 s), then `handleUpdateNowIfNeeded()`. Assert:
     - bank = 5;
     - thumbnails: `imported`, `numFrames` 3, `filename` "three frames.wav", `points` `memcmp`-equal to the snapshot's `thumbs`;
     - wire thumbs within 5e-4;
     - the cycle equals `readSample` on the snapshot (level 0, pos 0.5, Interp On).
  2. **P8:** `importFromMemory ("bad.wav", garbage)` → status `error` / `unreadable` / "bad.wav", while the thumbnails still say "three frames.wav", 3.
  3. **Empty:** `setStateInformation` with a fresh instance's state (no `IMPORTED_BANK`), then bank = 5. Assert:
     - thumbs: `imported`, `numFrames` 0, filename empty;
     - cycle view: `empty`, cycle all 0, harmonics all −60, `sounding` false.
- **G-VIZ-ALLOC** (PERF-03):
  - **(a)** Run the dsp-check alloc stimulus while a second `std::thread` loops `buildCycleView` + `getBankThumbnails` + `cycleToVar` + `JSON::toString`:
    - stimulus: a 16-note chord, a Mono legato run, LFO S&H at depth 1, bank cycling 0..5;
    - Imported published from main between blocks via `publishImportedBankForTesting`.
    - Pass = **0 audio-thread allocations**, after the `malloc(64)` liveness probe counts 1.
  - **(b)** On the main thread with no render running, after 1 warm-up call: 1000 × (`buildCycleView` + `cycleHash`) for bank 0 and for a non-empty Imported bank, armed for this thread = **0 allocations**, with its own liveness probe.
- **G-VIZ-TIME** (log only, no wall-clock verdict): 1000 × (build + hash + var + `JSON::toString`), and 20 × `bankUpdate` at N = 256 (a 256-frame bank from `WavetableImporter::buildImportedBank`). Print µs and bytes (RESEARCH A1, A2, A6).
- **Negative controls:**

| NC | Setup | Must fail by |
|----|-------|-------------|
| N1 | Render at idx 14, set `bit_depth` = 0 *after* the render, then build | G-VIZ-EXACT by > 10× tolerance |
| N2 | In G-VIZ-CEIL On, `harmonicsRaw` k = 9..32 vs the render | must sit ≥ 20 dB above −60 where the render is at the floor |
| N3 | Sine→Saw, Interp Off: compare against a render at pos + 1/31 | some bar must differ by ≥ 3 dB |
| N4 | Liveness | `harmonics[0] == 0 dB` and `max\|cycle\| > 0.5` in every non-empty case |

- **Depends on:** Task 8.

#### Task 10 — 3.2 gate: static + full offline suite  *(orchestrator)*
- **Static:**
  - S1–S11 again.
  - `git diff --numstat -- Source/WtVoice.h` → 0 deletions, ≤ 6 additions.
  - `grep -n 'importedForAudio' Source/CycleView.* Source/VizPayload.h Source/PluginEditor.*` = 0.
  - `buildCycleView` / `getBankThumbnails` contain no `importedForAudio` (inspect).
  - `Source/CycleView.cpp` is in `target_sources`.
  - `grep -c 'kTimerHz *= *30' Source/PluginEditor.cpp` = 1.
  - `OSIW_TEST_HOOKS` occurs only inside `#if` blocks and in the test CMake lines.
- **Offline:**
  - R-DBG, R-RUN (6 drivers + `--alloc-check` + `viz-check`), R-GOLD.
  - `viz-check` prints a PASS line for G-LESSON, G-VIZ-EXACT (all 90 cases), CEIL, SILENT, NOTE, IMPORTED, ALLOC (a) + (b), plus the TIME log, and N1–N4 `FAILS as designed`.
  - Save the logs; SUMMARY quotes the key lines.
- **On failure:** route the fix back through gui-agent dispatch B with the gate output. Thresholds are frozen.
- **Depends on:** Task 9.

#### Task 11 — Build, install, host-validate, commit Part 1  *(orchestrator)*
- **Do:**
  1. Re-run R-UISTATIC if `Source/ui/` changed since Task 5.
  2. R-INSTALL (VST3 + AU + explicit Standalone, freshness `strings`, auval, pluginval VST3 / AU strictness 10).
  3. R-UIPW boot-all-uis line only (`--strict-tips`).
  4. **STATUS.md** (fold into the commit):
     - `status: stage_3_part1_complete_visual_pending`;
     - `current_phase: execute`;
     - `next_action: user_visual_checkpoint_then_plugin_execute_stage_3`;
     - `last_updated`;
     - the Task 1 architecture checksum refresh;
     - a "Stage 3 execute Part 1 (3.1+3.2)" line under Completed So Far, with the key gate values.
  5. **R-COMMIT**, Part 1. Message: `feat(O-simpleWavetable): Stage 3 Part 1 - WebView layout/bindings + visualization panels (3.1+3.2)`. The body lists:
     - D-P…D-Z;
     - the gates that passed (G-VIZ max errors, ALLOC 0/0, G-LESSON, R-GOLD clean);
     - auval and pluginval.
- **Depends on:** Task 10 all green.

#### Task 12 — **Visual checkpoint**  *(human-verify, Taylor; BLOCKING)*
- **Surface:** the freshly built Release **Standalone** `build/plugins/O-simpleWavetable/O-simpleWavetable_artefacts/Release/Standalone/O-simpleWavetable-dev.app`.
  - Turn the audio output **on** (Options → Audio Settings). With no running device the audio thread idles: the note readout and the marker freeze (P2), and the bank stack still updates through the index watch.
- **Script:**
  1. **Bank stack (UI-01):**
     - Drag Position: the highlight and marker follow.
     - Interp **On**: the marker sits between frames (fractional).
     - Interp **Off**: it jumps frame to frame.
     - LFO Triangle 0.5 Hz at depth 100 % with a held note: the marker and highlight sweep.
     - Env Amount +100 %: each new note sweeps.
     - Every bank in the select redraws the stack.
     - Imported with nothing imported: dashed hollow stack + prompt.
  2. **Cycle (UI-02):** Bit Depth **3** shows an 8-level staircase plus the unquantized ghost; Full is smooth.
  3. **Harmonics (UI-03):** hold a high note (click **Alias Demo**; the keyboard jumps to octave 6).
     - Band-limit **On**: the green ceiling at kmax, with dashed ghost bars above it.
     - Band-limit **Off**: rust alias bars above the Nyquist line.
     - The note name and frequency read correctly.
  4. **Lessons:** each of the 5 buttons sets the controls as its recipe says; Output Level never moves; Alias Demo shifts the keyboard.
  5. **Smoothness (PERF-03):** panels move smoothly while the LFO runs, and audio has no dropouts.
- **Known until Part 2** (not regressions):
  - the Playwright label / tooltip gates and fr / zh hands-on are not done yet;
  - the import UX (dialog, drop, errors) is not hands-on verified;
  - no DAW pass yet.
- **Outcomes:**
  - **Sign-off** → Task 13.
  - **Visual or behaviour issue** → fix it within Part 1 (gui-agent with Taylor's notes), re-run Tasks 10–11 (fix commit), then repeat this checkpoint.
- The execute phase **STOPS here** and presents this script.
- **Handoff:** Step 1 `/clear`; Step 2 `/plugin-execute O-simpleWavetable 3-gui`, after the visual sign-off.

---

### Part 2: Phase 3.3  (resume after Task 12 sign-off)

#### Task 13 — Part 2 pre-flight  *(orchestrator)*
- STATUS shows the Task 12 sign-off. Record Taylor's notes in STATUS and set `status: stage_3_part2_in_progress`.
- Re-check `main` and a clean plugin tree. `git merge-base --is-ancestor <Part 1 commit> HEAD` must be true.
- **Depends on:** Task 12.

#### Task 14 — Import-path gates (UI-04)  *(gui-agent, dispatch C)*
- **Files:** edit `tests/viz-check/main.cpp`. Edit no `Source/` file unless a gate exposes a defect; report any such defect first.
- **Do:** through the processor API that the natives call:
  - **G-DROP** (D-Z):
    - **(a)** Encode the bytes of an in-memory 3-frame 16-bit WAV with `juce::Base64::toBase64` (standard alphabet, same as `btoa` and the module's `arrayBufferToBase64`). Then `importFromBase64 ("drop me.wav", b64)` → true → poll until `done` → `handleUpdateNowIfNeeded()`. Assert:
      - bank 5;
      - thumbnails filename "drop me.wav", 3 frames;
      - all 11 levels `memcmp`-equal to importing the same bytes through `importFromMemory`.
    - **(b)** A string of cap + 1 chars → false; status `error` / `tooLarge`; snapshot pointer unchanged.
    - **(c)** `"@@@@"` → false; status `unreadable`; snapshot unchanged.
    - **(d)** Name `"../../x\n.wav"` → status and thumbnails carry the sanitised basename with no control characters.
  - **G-IMPORT-ERR:**
    - A 2047-sample WAV → status `error` / `tooShort`, with the new filename on the status.
    - The bank and the thumbnails filename are unchanged (P8).
    - `importToVar` has exactly the keys `state, filename, frames, error`.
    - Every code the processor can emit is in {`tooShort`, `unreadable`, `tooLarge`, `unsupported`}. The page localizes the first three and maps the rest to generic (PluginProcessor.h:202).
  - **Negative control:** feed (a)'s payload re-encoded with JUCE's non-standard `MemoryBlock::toBase64Encoding`. It must **not** import: either `importFromBase64` returns false, or the status ends `error` / `unreadable` with the snapshot unchanged. This proves the gate distinguishes the alphabets. (Test-side only; `Source/` keeps 0 hits.)
- **Depends on:** Task 13. It can run in parallel with Task 15: the files are disjoint.

#### Task 15 — The five UI gates on the real tree (UI-05, UI-06, UI-04 states)  *(orchestrator; fixes via gui-agent dispatch D)*
- **Do:**
  - Run R-UISTATIC and R-UIPW (all five gates).
  - **Static page checks:**
    - `grep -c '100v[hw]' Source/ui/public/index.html` = 0;
    - `grep -c 'user-select: *none' Source/ui/public/index.html` ≥ 1;
    - the MIME types in the S4 branches are correct (html, javascript, css, png, woff2).
  - **Findings:**
    - Fix only **real** findings, through gui-agent dispatch D with the gate output.
    - The mockup is locked: a fix that changes layout or copy must keep the pins, so re-run `check-ui-labels`.
    - An i18n change must keep the fr and zh lints clean (glossary first; memory `pattern_parallel_localizers_need_glossary_first`).
    - Never "fix" a gate-tree artifact (R23) by editing the page.
- **Accept:** all five exit 0 (check-i18n 16/16; fr CLEAN; zh with its plugin line; check-ui-labels 8 states; boot-all-uis 0 DEAD / 0 404). **No exit 77 is ever recorded as a pass.**
- **Depends on:** Task 13.

#### Task 16 — Full regression, install, host  *(orchestrator)*
- **Static:** S1–S11 + Task 10's static gates.
- **Offline:** R-DBG, R-RUN (6 drivers + alloc + `viz-check`, now including G-DROP / G-IMPORT-ERR), R-GOLD.
- **Debug Standalone** for checklist §4 (Inspect, console): `cmake --build "$SCRATCH/build-oswt" --target O-simpleWavetable_Standalone` (background).
- **Release:** R-INSTALL.
- **Test hooks absent from the shipped binary:** `nm -gU <installed VST3 binary> | grep -c ForTesting` = 0.
- **Depends on:** Tasks 14–15.

#### Task 17 — Standalone hands-on  *(human-verify, Taylor; blocks Task 19)*
- **Surface:** the Release Standalone, plus the Debug Standalone from Task 16 for §4.
- **§4 (Debug):** right-click → Inspect works, the console shows no JS errors, and `window.__JUCE__` exists.
- **§6:**
  - All 21 controls ↔ params.
  - Bank-stack drag and wheel = one gesture per drag or burst.
  - Rate ↔ Division swap on Sync, with the Division pips shown the first time.
  - Readouts match the host text (`-inf`, `Full`).
- **§9:**
  - Tab reaches every knob (including Bit Depth and Division), and the arrow keys move it.
  - Dragging out of the window and releasing outside ends the gesture.
  - Double-click resets each knob to its default (stepped knobs to their index).
  - The hover-help switch toggles tooltips and persists across reopen (`oswt.tipsEnabled`); every control shows a plain-language tip.
  - Switching the language to **fr** and **zh-Hans** relabels the page with no clipping; the bank menu stays English.
  - With macOS **Reduce Motion** on, no animation runs (static amber ring).
  - **Import button** → dialog → `Slicing…` → Imported, with the filename and frame count.
  - **Drop a WAV** → same result. Drop a **folder** or a **.txt** → the localized refusal.
  - Import a **≥ 12 s** file → 256 frames; the stack draws (the N = 256 `bankUpdate`, ~209 KB, RESEARCH A2).
  - Import a too-short file → the localized `tooShort` error.
  - Empty Imported: dashed stack, prompt, amber button, silence.
  - The keyboard plays with A–K / W–U; Z / X change octave; a window blur releases held notes.
- **Outcomes:** a finding → fix (gui-agent), re-run Tasks 15–16, then repeat the affected items.

#### Task 18 — DAW pass (Logic)  *(human-verify, Taylor; blocks Task 19)*
- **Automation → UI:** automate Position and Bank. On playback the knobs, the bank stack and the marker follow. Host preset recall updates every control and redraws the stack (forced `bankUpdate` / `cycleUpdate`).
- **Hide / re-show:** close and reopen the editor, switch tracks and minimise. The stack, the source line and the cycle come back (`uiReady` re-send). On a non-selected Logic track, changing Bank from the plugin header still redraws the stack (P2 index watch).
- **Save / reopen with Imported** (Stage 2 deferred item): import a file, save the project, quit, reopen. Imported is restored with its filename and frame count, plays, and the stack draws.
- **Lifecycle:**
  - Close the editor **while the Import dialog is open** → no crash (P12).
  - Insert and remove the plugin 10 times → no crash (member order).
  - In Logic, the AU appears under Instruments (Stage 1 Task 14 leftover).
- **Outcomes:** as Task 17.

#### Task 19 — Contract notes  *(orchestrator)*
- **Files:** `research/ARCHITECTURE.md`, `STATUS.md` (checksum).
- **Do:** append a dated **"Stage 3 Amendments (2026-10-xx)"** section. Rewrite nothing else.
  - **14.** Change detection hashes the quantized output payload; `lfo` is gated on depth; idle means 0 events (D-P). This supersedes "when the inputs changed" in §Visualization Data Path.
  - **15.** Display state adds `dispNote`, `dispHz`, `displayFs` and `WtVoice::lastNote`. The `cycleUpdate` payload is the checklist superset (D-X).
  - **16.** The harmonic dB reference is the max over bins 1..1023 (D-U).
  - **17.** The Stage 3 lesson recipes are the checklist's (Alias Demo on Sine→Saw); §A9 remains the FUNC-08 Stage 4 target. Apply is a single pass, with `output_level` untouched (D-V, D-Y).
  - **18.** `bankUpdate` triggers add the editor's bank-index watch and the content dedupe (D-T). The filename comes from the cached blob (D-S).
  - Recompute `contract_checksums.architecture` in STATUS.
- **Depends on:** Tasks 17–18 signed off.

#### Task 20 — SUMMARY, STATUS, PLUGINS row, commit Part 2, hand off  *(orchestrator)*
- **SUMMARY.md** (`stages/3-gui/SUMMARY.md`):
  - the files created and changed;
  - D-P…D-Z as applied, including whether Taylor flipped D-U;
  - the measured values:
    - G-VIZ-EXACT worst cycle error and worst dB;
    - CEIL;
    - ALLOC (a) / (b);
    - TIME µs and bytes;
    - the N1–N4 margins;
    - G-LESSON;
    - G-DROP / G-IMPORT-ERR;
  - the R-GOLD result;
  - the five UI-gate outputs;
  - auval and pluginval;
  - the Task 12 / 17 / 18 notes;
  - deviations.
- **STATUS.md:**
  - `status: stage_3_execute_complete`;
  - `current_phase: verify`;
  - `next_action: plugin_verify_stage_3`;
  - the phase table shows execute ✓;
  - a Part 2 line under Completed So Far.
- **PLUGINS.md:** the `O-simpleWavetable` row only → `| O-simpleWavetable | 🚧 Stage 3 | 0.1.0 | Synth (Pedagogical Wavetable) | <YYYY-MM-DD> |`. Build it from the `HEAD` blob inside R-COMMIT step 3. Never stage the working-tree PLUGINS.md (foreign rows).
- **R-COMMIT**, Part 2. Message: `feat(O-simpleWavetable): Stage 3 Part 2 - import UX, tooltips, i18n gates (3.3)`. If Part 2 code was already committed as "hands-on pending", this commit carries the docs and the row only.
- **Hand off:** Step 1 `/clear`; Step 2 `/plugin-verify O-simpleWavetable 3-gui`. Then **STOP**.
- **Depends on:** Task 19.

---

## Files to create / modify

All paths are relative to `plugins/O-simpleWavetable/`, except `PLUGINS.md`.

| Path | Action | Task |
|------|--------|------|
| `Source/ui/public/index.html` | create (`cp` v1-ui.html, byte-identical) | 2 (15 if a real finding) |
| `Source/ui/public/js/i18n.js` | create (`cp` v1-i18n.js) | 2 (15) |
| `Source/ui/public/js/juce/index.js`, `check_native_interop.js` | create (`cp` from O-simpleAdditive) | 2 |
| `Source/ui/public/img/insects.png` | create (`cp` from O-simpleAdditive) | 2 |
| `tests/i18n-states.json` | create (`cp` v1-i18n-states.json) | 2 |
| `CMakeLists.txt` | UIResources binary data + link + `viz-check` target (A); `target_sources` for CycleView / VizPayload (B) | 2, 7 |
| `Source/PluginEditor.{h,cpp}` | rewrite from the template (A); viz wiring (B) | 3, 8 |
| `Source/PluginProcessor.{h,cpp}` | **A:** `applyFactoryPreset`, `importFromBase64`<br>**B:** `dispNote` / `dispHz` / `displayFs`, `vizRenderer`, `buildCycleView`, `getBankThumbnails`, `getSelectedBankIndex`, type aliases | 4, 6, 7 |
| `Source/WtVoice.h` | `lastNote` + `getLastNote()` (additions only) | 6 |
| `Source/CycleView.{h,cpp}`, `Source/VizPayload.h` | create | 7 |
| `tests/viz-check/main.cpp` | create (G-LESSON); extend (G-VIZ-*, N1–N4); extend (G-DROP, G-IMPORT-ERR) | 4, 9, 14 |
| `.planning/research/ARCHITECTURE.md` | dated Stage 3 amendments | 19 |
| `.planning/stages/3-gui/SUMMARY.md` | create | 20 |
| `.planning/STATUS.md` | update | 11, 13, 19, 20 |
| `PLUGINS.md` (this row only, from the HEAD blob) | update | 20 |

**Must not touch:**
- `Source/ui/public/modules/**` (configure-generated, gitignored, never committed);
- `mockups/`;
- `REQUIREMENTS.md` (status flips belong to `/plugin-verify`), `parameter-spec.md`, `ROADMAP.md`, `CHANGELOG.md`;
- the root `CMakeLists.txt`, `.gitignore`, `modules/`;
- `build/CMakeCache.txt` options;
- other plugins, in particular the dirty O-simpleFM, O-Gain, O-Reed, O-Bassoon and O-Freeze trees.

---

## Dependency graph / waves

```
PART 1
Wave 0 [orch]        T1  pre-flight + 2->3 gate + Stage 2 baseline logs (Debug tree warm)
Wave 1 [gui-agent A] T2 files+CMake -> T3 editor/bridge -> T4 lessons + importFromBase64 + viz-check G-LESSON
Wave 2 [orch]        T5  3.1 gate: S1-S11, Debug build, 6 drivers + R-GOLD, R-UISTATIC, boot-all-uis   (tracer proven)
Wave 3 [gui-agent B] T6 display state -> T7 renderer/payload/API -> T8 editor viz wiring -> T9 viz-check gates
Wave 4 [orch]        T10 static + full offline + R-GOLD -> T11 install/auval/pluginval/Standalone + commit P1
Wave 5 [HUMAN]       T12 VISUAL CHECKPOINT   ---- execute STOPS here ----

PART 2  (second /plugin-execute O-simpleWavetable 3-gui)
Wave 6 [orch]        T13 pre-flight
Wave 7 [parallel]    [gui-agent C] T14 G-DROP / G-IMPORT-ERR    ||    [orch] T15 five UI gates (-> gui-agent D on real findings)
Wave 8 [orch]        T16 full regression + Debug/Release Standalone + install/auval/pluginval
Wave 9 [HUMAN]       T17 Standalone hands-on  ||  T18 DAW pass
Wave 10 [orch]       T19 contract notes -> T20 SUMMARY/STATUS/PLUGINS + commit P2 -> handoff /plugin-verify
```

T14 (`tests/viz-check/`) and Task 15's fixes (`Source/ui/**`) touch disjoint files, so Wave 7 runs in parallel. Sequential dispatches A → B both edit `PluginEditor.*` / `PluginProcessor.*`, so they never overlap.

---

## Success criteria (goal-backward; all must be TRUE for the verify phase)

| # | Observable truth | Evidence | Task |
|---|------------------|----------|------|
| S1 | Every control drives its param, and host automation moves the UI | S5 / S6 counts and order; boot-all-uis 0 DEAD; §6 hands-on; DAW automation → UI | 3, 5, 17, 18 |
| S2 | The bridge is complete | 8/8 natives (S2 diff empty); lesson ids match (S3); 11 resource branches (S4); 10 binary-data files; no 404; `uiReady` re-send proven by DAW hide/re-show | 3, 5, 15, 18 |
| S3 | **UI-01:** stack, highlight and marker follow the effective position | G-VIZ-EXACT / SILENT / NOTE fields (`pos`, `frame`, `sounding`); G-VIZ-IMPORTED thumbs `memcmp`-exact; Task 12 items 1 and 4 | 9, 10, 12 |
| S4 | **UI-02:** the cycle panel is the quantized cycle being read | G-VIZ-EXACT cycle ≤ 2e-4, incl. the 3-bit rows; N1 fails as designed; Task 12 staircase | 9, 10, 12 |
| S5 | **UI-03:** bars = an offline FFT of the heard cycle, including band-limit | G-VIZ-EXACT ≤ 0.02 dB vs an independent DFT; G-VIZ-CEIL ceiling / kmax / nyquistH; N2 / N3 / N4 fire; Task 12 ceiling and alias bars | 9, 10, 12 |
| S6 | **PERF-03:** smooth at 30 Hz with no audio-thread cost | ALLOC (a) 0 audio-thread allocs under concurrent viz; ALLOC (b) 0 allocs per idle tick; idle-quiet hash (SILENT); `kTimerHz` 30; TIME logged; `--alloc-check` 0; Task 12 smoothness | 9, 10, 12 |
| S7 | Lesson buttons apply their recipes without touching the output | G-LESSON (5 ids, gestures, unknown id, Imported untouched, NC); S3 id diff; Task 12 | 4, 5, 12 |
| S8 | **UI-04:** import from the button and a drop, with filename, frame count, errors and the empty state | G-DROP, G-IMPORT-ERR, G-VIZ-IMPORTED (P8); check-ui-labels states; §9 import / drop / refusals / empty; DAW save/reopen | 9, 14, 15, 17, 18 |
| S9 | **UI-05:** a plain-language, localized tooltip on every control | boot-all-uis `--strict-tips` 0 DEAD; check-i18n 16/16; fr CLEAN; zh 0 findings with its count; §9 language switch | 15, 17 |
| S10 | **UI-06:** a projector-readable single page, consistent with the siblings | check-ui-labels 8 states incl. [7] pins; fixed 1120 × 780; §9 Reduce Motion / no clipping | 15, 17 |
| S11 | Stage 2 is untouched | R-GOLD clean at Tasks 5, 10 and 16; WtVoice diff additions-only; auval; pluginval VST3 / AU strictness 10 with the WebView editor; no `ForTesting` symbols shipped | 5, 10, 11, 16 |
| S12 | Hosts accept it | DAW pass: save/reopen with Imported (Stage 2 deferred item closed), editor lifecycle, reload ×10 | 18 |
| S13 | Taylor signed off | Task 12 visual sign-off; Tasks 17–18 results recorded in STATUS / SUMMARY | 12, 17, 18 |
| S14 | Clean, scoped commits | Part 1, Part 2 (+ any fix commits) touch only `plugins/O-simpleWavetable/**` (+ 1 PLUGINS.md row in Part 2); drop module absent from every tree; no tag | 11, 20 |
| S15 | Contracts are consistent | ARCHITECTURE Stage 3 amendments 14–18; STATUS checksums current (incl. the W5 refresh) | 1, 19 |

### Coverage audit

| Source item | Covered by |
|-------------|-----------|
| **UI-01** bank panel, highlight, marker | D-R, D-S, D-T; T7, T8; G-VIZ-EXACT / SILENT / NOTE / IMPORTED; T12 → S3 |
| **UI-02** quantized cycle panel | T7 renderer; G-VIZ-EXACT (bit idx 9, 14); N1; T12 → S4 |
| **UI-03** harmonics 1–32 incl. band-limit | D-U; T7; G-VIZ-EXACT dB, G-VIZ-CEIL; N2–N4; T12 → S5 |
| **UI-04** import button, filename, frame count | D-Q, D-S, D-Z; T3, T4, T14; G-DROP, G-IMPORT-ERR, G-VIZ-IMPORTED; check-ui-labels; T17, T18 → S8 |
| **UI-05** tooltips, localized | Page verbatim (T2); T15 boot-all-uis `--strict-tips`, check-i18n, fr / zh lint; T17 → S9 |
| **UI-06** projector-readable single page | CTX-mockup; T15 check-ui-labels [7]; S7 `setSize`; T17 → S10 |
| **PERF-03** ≥ 30 fps, no audio-thread stall | D-P; T6 must-nots; G-VIZ-ALLOC (a)(b), SILENT, TIME; `--alloc-check`; T12 → S6 |
| Lessons (CTX-lessons) | D-V, D-Y; T4, T8; G-LESSON + NC; S3 diff; T12 → S7 |
| Bridge (CTX-natives, P11) | D-Q; T3; S2 / S4 / S5 / S6 diffs; boot-all-uis → S2 |
| Regression (Stage 2 goldens) | T1 baseline; R-GOLD at T5 / T10 / T16; WtVoice additions-only; host checks → S11 |
| ROADMAP 3.1 / 3.2 / 3.3 acceptance | 3.1 → S1; 3.2 → S3–S6 (harness compares the JSON payload: D-W); 3.3 → S8–S10 + the T17 real file drop |
| CONTEXT cadence / venue / keyboard | Stop rule + T12; T17 / T18; page + `uiMidi` (T3), T17 |
| CONTEXT constraints | - Snapshot-only: overrides, T7, T10 grep<br>- No audio-thread cost: T6, ALLOC<br>- `uiReady` + forced updates: D-T, T8, T18<br>- English select: settled<br>- Pins: T15<br>- 5 UI gates: T15<br>- Gitignored module: R-COMMIT<br>- `withUserDataFolder`: T3, S7<br>- build/install + Standalone: R-INSTALL<br>- Path-scoped commits: R-COMMIT |
| CONTEXT open questions | - Struct shapes / hash: D-P, D-W<br>- Atomics: D-X<br>- Payload cost: D-P, G-VIZ-TIME<br>- Reference editor: settled (template + O-simpleAdditive; not O-simpleFM)<br>- UI-03 harness: D-W<br>- Lesson reset / gestures: D-Y |
| RESEARCH Q1–Q7 | Q1 D-P / T7; Q2 D-R / D-X / T6; Q3 T7; Q4 D-P / D-T / T8; Q5 T3; Q6 T9; Q7 D-Y / T4 |
| RESEARCH P1–P19 | P1 D-P; P2 D-T; P3 / P4 D-R; P5 D-X; P6 T7; P7 D-W; P8 D-S; P9 D-Y; P10 D-T; P11 D-Q; P12 T3 / T18; P13 R12; P14 R-INSTALL; P15 R-COMMIT; P16 page verbatim + T12 / T17; P17 R-UIPW; P18 T5 / T11; P19 R-COMMIT |
| RESEARCH contradictions 1–8 | 1 T4 / T7; 2 T6; 3 D-R; 4 D-P; 5 T8 (keep both re-sends); 6 T3 comment fix; 7 settled (21); 8 D-V |
| RESEARCH assumptions A1–A6 | A1 / A6 G-VIZ-TIME; A2 G-VIZ-TIME + T17 256-frame import; A3 T12 / T17 (natives work); A4 D-U (flip-able); A5 avoided (D-Y uses `ParamIDs::all`) |
| Stage 2 deferred: save/reopen with Imported | T18 → S12 |

No unplanned items.

---

## Out of scope (Stage 3)

- **Stage 4:**
  - FUNC-08 factory-preset bank / preset manager and the §A9 recipe refinement;
  - PERF-02 CPU optimisation;
  - COMPAT-02 Windows build;
  - QUAL-04;
  - VERSION 1.0.0, CHANGELOG, the CODE_REVIEW pass;
  - Stage 2's deferred W3 / W4 and notes 3–8.
- **Not built:** a live output scope or spectrum (CTX-noscope); a resizable editor; a preset-manager UI.
- **`/plugin-verify` owns:** the REQUIREMENTS status flips for UI-01..06 / PERF-03.

---

## Risks and gotchas

| # | Risk | Mitigation |
|---|------|-----------|
| R1 | Idle `cycleUpdate` churn from the free-running LFO (P1) | D-P (lfo gated; output hash over quanta); G-VIZ-SILENT + NC |
| R2 | Stale stack when the host idles the audio thread (P2) | D-T index watch; T18 non-selected-track check |
| R3 | Wrong frame / stale atomics after release (P3, P4) | D-R; G-VIZ-EXACT / SILENT |
| R4 | Mono note name wrong on a legato move (P5) | D-X `lastNote` at all three sites; G-VIZ-NOTE |
| R5 | Imported bank raced or freed under the renderer (P6, Amendment 12) | Snapshot-only, held for one call; never `importedForAudio`; T10 grep |
| R6 | `BinaryData.h` pulled into a console TU (P7) | Only in `PluginEditor.cpp` (S8); the Debug console build is the check |
| R7 | Wrong filename after a failed import (P8) | D-S; G-VIZ-IMPORTED and G-IMPORT-ERR assert it |
| R8 | A lesson resets `output_level` (P9, O-simpleAdditive's loop) | D-Y; G-LESSON + NC |
| R9 | An `importError` cleared by an out-of-order `bankUpdate` (P10) | D-T fixed order |
| R10 | A silently dead native (P11; the boot stub cannot see it) | D-Q; the S2 `comm` diff |
| R11 | FileChooser use-after-free after the editor closes (P12) | Complete first + SafePointer bail (S10); T18 close-with-dialog |
| R12 | A 96 MB drop stalls the message thread (P13) | Accepted (rare action; the page shows busy first); cap checked before decode (D-Z) |
| R13 | Visual checks against a stale Standalone (P14) | R-INSTALL step 2 + `strings` freshness + mtime |
| R14 | The gitignored drop module gets committed (P15) | Temp-index `git add` respects `.gitignore`; post-commit `ls-tree` check |
| R15 | Canvas text in the fallback face (P16) | The page is verbatim (`document.fonts.load` → repaint); T12 / T17 eyes |
| R16 | Vacuous UI gates: exit 77 (P17), zh-lint on a mistyped `--plugin`, a port-clash page | R-UIPW / R-UISTATIC require a named plugin line + count; 77 ≠ pass |
| R17 | CI turns main red (P18) | R-UISTATIC green before the Part 1 commit (T5, T11) |
| R18 | Shared checkout (P19): foreign-staged PLUGINS.md; other sessions' staging | R-COMMIT (temp-index CAS, re-check immediately before, resync own paths only) |
| R19 | Template drift: 41 non-ASCII lines, nested struct names, raw-float hash, stale `callAsync` comment | Transcription rules (T3, T7); S1; D-W aliases; D-P quanta |
| R20 | `-Wshadow-all` warning from RESEARCH's `midiNote` member | D-X names it `lastNote`; R-DBG 0-warning check |
| R21 | RT regression from the display additions | Additions-only WtVoice diff; R-GOLD; `--alloc-check`; ALLOC (a) |
| R22 | A foreign CMake edit breaks the shared `build/` configure | Stop and report; offline gates use the out-of-repo tree with `SKIP_PLUGINS` |
| R23 | A gate tree 404s the module fonts that the binary serves (memory R5) | Cross-check against the S4 branch list before any "fix" |
| R24 | `juce::dsp::FFT` allocates inside perform on some engine path | ALLOC (b) catches it; fix = pre-sized real-only transform + manual magnitude |
| R25 | NaN / inf → invalid JSON → the page silently drops the event | D-P maps non-finite to 0; the `viz-check` JSON round trip asserts finiteness |
| R26 | Standalone with no audio device looks broken (P2) | The Task 12 script turns audio on first |
| R27 | The STATUS architecture checksum is stale (gap-closure W5) | Task 1 verifies the diff is exactly W5, then refreshes it |
| R28 | Untrusted WebView input (ASVS V5) | - `applyFactoryPreset`: 5-id allow-list<br>- `setUiLanguage`: allow-list<br>- `uiMidi`: clamps (cpp:597-603)<br>- `importFromBase64`: cap before decode, standard decoder, 96 MB processor cap, `sanitiseName`<br>- Chooser paths go through `importFromFile`'s existence check<br>- The filename renders as text, and `emitEvent` escapes it |
| R29 | 600 s watchdog on long builds and auval | Background + poll (R-DBG, R-INSTALL) |
