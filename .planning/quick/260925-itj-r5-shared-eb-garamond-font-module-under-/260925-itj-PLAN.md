---
phase: quick-260925-itj
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - plugins/O-ReverseDelay/tests/tools/cdp-font-probe.js
  - modules/ui/eb-garamond/tools/build-fonts.sh
  - modules/ui/eb-garamond/OFL.txt
  - modules/ui/eb-garamond/css/eb-garamond.css
  - modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2
  - modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2
  - modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2
  - plugins/O-ReverseDelay/CMakeLists.txt
  - plugins/O-ReverseDelay/Source/PluginEditor.cpp
  - plugins/O-ReverseDelay/Source/ui/public/index.html
  - plugins/O-ReverseDelay/Source/ui/public/css/styles.css
  - modules/ui/eb-garamond/module.yaml
  - modules/ui/eb-garamond/README.md
  - modules/ui/eb-garamond/snippets/getResource.cpp
  - modules/registry.yaml
  - scripts/regen-registry-used-by.sh
  - plugins/O-ReverseDelay/tests/ui_frontend_check.js
  - plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js
  - plugins/O-ReverseDelay/tests/ui-stub/serve-stub.sh
  - plugins/O-ReverseDelay/CHANGELOG.md
  - plugins/O-ReverseDelay/NOTES.md
  - PLUGINS.md
autonomous: true
requirements: [R5]
source: .planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md (R5, line 38)

estimate:
  tokens: 170000
  raw_tokens: 170000
  tasks: 3
  confidence: low

must_haves:
  truths:
    - "On the served O-ReverseDelay page, every Latin text run in the 9 probe targets (.title, .title-accent, .subtitle, .group-label, .knob-label, .preset-name, .ab-slot, .fleuron, .footer-text) resolves to the bundled custom face EB Garamond in en, fr and zh-Hans. Han runs still resolve to PingFang SC. All three EB Garamond FontFaces (400, 400 italic, 700) report loaded, and serve-ui records 0 unserved requests. cdp-font-probe.js --all-languages exits 0 after the change, and exited 2 (RED, Latin on Times New Roman) on untouched v1.21.1"
    - "No element moves vertically by more than 0.5 px against the v1.21.1 baseline, in any of en, fr or zh-Hans. A before-vs-before noise-floor run also shows 0 vertical moves. The ui_tooltip_clamp_check WINDOW budget line is byte-identical to the baseline (212 into body 212)"
    - "check-ui-labels.js --plugin O-ReverseDelay passes before and after, including 'every requested resource was served'. ui_frontend_check.js and ui_tooltip_clamp_check.js exit 0 after the change. check-i18n, i18n-fr-lint and i18n-zh-lint exit with their baseline codes"
    - "The font is embedded directly from ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond into O-ReverseDelay's ONE existing UIResources target, and is served by four exact-match getResource() branches (text/css, font/woff2). ui_frontend_check §9 maps css/ and fonts/ module paths, resolves the 3 CSS font URLs to provider paths, and asserts font/woff2. A deliberately broken font URL made it FAIL"
    - "modules/ui/eb-garamond ships static 400 / 400i / 700 woff2 files, each under 50 000 B, subset to Latin + Latin-Ext, with Times-matched vertical metrics (hhea and OS/2 typo 891/-216/42, USE_TYPO_METRICS). It also ships OFL.txt, an @font-face stylesheet that carries the OFL notice, and build-fonts.sh. The script is pinned to google/fonts f8c1d3d6 with full sha256 checks, and running it twice produces byte-identical fonts"
    - "modules/registry.yaml lists eb-garamond 1.0.0 (category ui) with used_by O-ReverseDelay 1.22.0. regen-registry-used-by.sh regenerates exactly that block after its css/ + fonts/ token extension. The unextended script was first observed rewriting it to []"
    - "O-ReverseDelay v1.22.0 is built and installed as the -dev variant only, with the AU cache cleared. The installed VST3 and AU binaries contain the /fonts/EBGaramond-Regular.woff2 provider string. auval -v aufx ORvD OuDv and pluginval strictness 10 (VST3) succeed. CHANGELOG, NOTES and the PLUGINS.md row read 1.22.0. There are two path-scoped commits, no tag, and no other plugin is touched"
  artifacts:
    - path: plugins/O-ReverseDelay/tests/tools/cdp-font-probe.js
      provides: "Resolved-face probe (CDP CSS.getPlatformFontsForNode after document.fonts.ready) + FontFace status + serve misses + --dump-rects/--diff-rects vertical-move gate + --screenshot"
      contains: "getPlatformFontsForNode"
    - path: modules/ui/eb-garamond/tools/build-fonts.sh
      provides: "Reproducible pinned-SHA download, sha256 check, instancer, subset, Times-metric bake"
      contains: "f8c1d3d6cc75e30d77130bdcbfbff27e3b6233fe"
    - path: modules/ui/eb-garamond/css/eb-garamond.css
      provides: "Three @font-face rules (family 'EB Garamond', 400/400i/700, font-display: block) + AGPL header + EB Garamond copyright + full OFL text"
      contains: "@font-face"
    - path: modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2
      provides: "400 roman, subset, Times-matched metrics"
    - path: modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2
      provides: "400 italic, subset, Times-matched metrics"
    - path: modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2
      provides: "700 roman, subset, Times-matched metrics"
    - path: modules/ui/eb-garamond/OFL.txt
      provides: "Verbatim OFL from google/fonts@f8c1d3d6 (sha256 0985066662eb755ed3683ae5482a81a9195b49ce3f7e165cc2388b3dbece7dd7)"
      contains: "SIL Open Font License"
    - path: modules/ui/eb-garamond/README.md
      provides: "Integration steps, metric rationale, served-tree pitfalls, rebuild recipe, OFL obligations"
    - path: modules/registry.yaml
      provides: "eb-garamond 1.0.0 entry under category ui, used_by O-ReverseDelay 1.22.0; header 1.2.0 / 2026-09-25"
      contains: "name: eb-garamond"
    - path: plugins/O-ReverseDelay/Source/PluginEditor.cpp
      provides: "Four getResource() branches for /css/eb-garamond.css and /fonts/EBGaramond-{Regular,Italic,Bold}.woff2"
      contains: "EBGaramondRegular_woff2"
    - path: plugins/O-ReverseDelay/Source/ui/public/css/styles.css
      provides: "--serif: 'EB Garamond', 'Georgia', 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif"
      contains: "--serif: 'EB Garamond', 'Georgia'"
  key_links:
    - from: plugins/O-ReverseDelay/Source/ui/public/index.html
      to: plugins/O-ReverseDelay/Source/PluginEditor.cpp
      via: "<link href=\"css/eb-garamond.css\"> → getResource() branch for /css/eb-garamond.css → UIBinaryData::ebgaramond_css"
      pattern: "css/eb-garamond.css"
    - from: modules/ui/eb-garamond/css/eb-garamond.css
      to: plugins/O-ReverseDelay/Source/PluginEditor.cpp
      via: "url('../fonts/EBGaramond-*.woff2') resolves against /css/ to /fonts/*.woff2 → three font/woff2 branches"
      pattern: "font/woff2"
    - from: plugins/O-ReverseDelay/CMakeLists.txt
      to: modules/ui/eb-garamond
      via: "${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/... entries inside the single OuariconReverseDelay_UIResources SOURCES block (hyphens stripped in symbols)"
      pattern: "modules/ui/eb-garamond"
    - from: plugins/O-ReverseDelay/Source/ui/public/css/styles.css
      to: modules/ui/eb-garamond/css/eb-garamond.css
      via: "--serif names the @font-face family 'EB Garamond' first; every font-family in styles.css reads var(--serif)"
      pattern: "--serif"
---

<objective>
R5 from the UI design review: bundle a single OFL serif as a shared module and prove it on one plugin.

Build `modules/ui/eb-garamond/`: EB Garamond Regular 400, Italic 400 and Bold 700 as woff2, subset to Latin + Latin-Ext, with Times New Roman's vertical metrics baked in. Ship it with `OFL.txt`, an `@font-face` stylesheet, a reproducible build script, a README, a registry entry and a C++ provider snippet. Deliver it to O-ReverseDelay by direct embed from `${CMAKE_SOURCE_DIR}/modules`, the way `tuning-panel.js` and `preset-manager.js` already reach their plugins. Pin O-ReverseDelay's font token so EB Garamond comes first and Georgia comes before Times.

The pilot is an /improve-style MINOR bump, v1.21.1 (committed at 18b358ed) → v1.22.0:
- a verified backup,
- gates captured before any product edit and re-run after,
- build + install per CLAUDE.md,
- targeted auval and pluginval-10,
- CHANGELOG, NOTES and PLUGINS.md updates,
- two path-scoped commits.

Stop after the pilot. No other plugin is migrated.

Purpose:
- Today O-ReverseDelay depends on whatever serif the host machine has. Windows has no Garamond, and macOS falls back to Times.
- One bundled face gives brand fidelity and Windows parity.
- The Times-matched metrics keep every line box where it was, so the 0 px-spare WINDOW panel cannot move.

Output: the module tree, O-ReverseDelay v1.22.0 installed, the gate tooling taught about fonts, and a SUMMARY with before/after evidence.
</objective>

<execution_context>
@~/.claude/gsd-core/workflows/execute-plan.md
@~/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@CLAUDE.md
@.planning/quick/260925-itj-r5-shared-eb-garamond-font-module-under-/260925-itj-RESEARCH.md
@.claude/skills/plugin-improve/SKILL.md
@.claude/skills/plugin-improve/assets/backup-template.sh
@plugins/O-Strata/tests/tools/cdp-font-probe.js

Key facts, pre-verified during planning:
- Working tree at plan time: `git status --short -- plugins/O-ReverseDelay PLUGINS.md modules scripts` is clean.
- Foreign dirt from other sessions is in `plugins/O-MultiBandCompressor/Source/ui/public/index.html` and O-Formant. Never stage those.
- `backups/` is gitignored, so the backup is never committed.
- Release tags exist for this plugin. Never create one.
- AU triple: `aufx ORvD OuDv` (dev manufacturer code). Installed today: `O-ReverseDelay-dev.vst3` and `O-ReverseDelay-dev.component` only.
- pluginval binary: `/Applications/pluginval.app/Contents/MacOS/pluginval` (not on PATH).
- `scripts/serve-ui.js` `binaryDataSources()` (lines 141-172) tokenises the WHOLE `juce_add_binary_data(...)` block, comments included, with a balanced-paren scan. Any comment word ending in a file extension becomes a phantom source, and unbalanced parentheses truncate the block. `urlSymbolMap()` (229-259) pairs each `url == "..."` with the first `BinaryData::sym` inside a 400-char window.
- `tests/ui_frontend_check.js` §9 (380-451) strips `#` comments before matching. Its `embeddedFiles` map knows only `Source/ui/public/...` and `modules/.../js/*.js`.
- `scripts/regen-registry-used-by.sh` `module_tokens()` (around line 62) walks only `cpp/` and `js/` for `.h/.cpp/.js`. Its modules section ends at the first column-0 line (the `# ====` HOW-TO block at the end of registry.yaml). No other module has a `css/` or `fonts/` directory, and no plugin references `eb-garamond`/`EBGaramond` today.
- Visible bold and italic usage, so all three faces load: `.ab-slot` uses `font-weight: 700` (styles.css:616) and `.title-accent` uses `font-style: italic` (styles.css:304-307).
- Research scratch artifacts may still exist at `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/46787b9e-fcae-4567-b112-06c7a7566134/scratchpad/ebg/`: `probe.js` has the rect-diff keying, `bake2.py` the metric bake, and `out-tnr/` the expected Times-metric outputs. Use them as reference only. Committed outputs come from build-fonts.sh.
- Full sha256 of the pinned inputs, from google/fonts @ f8c1d3d6cc75e30d77130bdcbfbff27e3b6233fe `ofl/ebgaramond/`:
  - `EBGaramond[wght].ttf` = ef9512f92f6d579e5dc75af59a5a4b1b8b47d2eda89e00b954d44520e5369027
  - `EBGaramond-Italic[wght].ttf` = bba2c4499c93c9612b90b9825d32b07da52fce2fe57562a1eb6b833553f93c4e
  - `OFL.txt` = 0985066662eb755ed3683ae5482a81a9195b49ce3f7e165cc2388b3dbece7dd7
- The pinned commit date, 2026-06-17 00:00:00 UTC, is epoch 1781654400.

Execution rules (from CLAUDE.md, memory and the task constraints):
- Trunk-based on main. No branches, no worktrees, no tags.
- Every build, `check-ui-labels`, `ui_tooltip_clamp_check`, `auval` and `pluginval` call runs with `run_in_background: true`, with stdout+stderr going to a log under `$ITJ` (600 s watchdog).
- `$ITJ` = `<your session scratchpad>/itj`. Never `/tmp`, never the repo.
- Revert any temporary control edit by a REVERSE targeted Edit. Never `git checkout`/`git restore`: uncommitted work shares those files.
- Commits happen ONLY in Task 3, after every after-gate and the build pass. Between Tasks 1 and 2, ui_frontend_check §9 is expected to be red, so HEAD must never carry the half-wired state. Do not make per-task commits.
</context>

<tasks>

<task type="tracer">
  <name>Task 1: Baseline v1.21.1, then take EB Garamond end to end — pinned build → module css/fonts → CMake embed → getResource() → link → --serif — proven by a CDP probe that is RED on the baseline and GREEN after</name>
  <files>plugins/O-ReverseDelay/tests/tools/cdp-font-probe.js, modules/ui/eb-garamond/tools/build-fonts.sh, modules/ui/eb-garamond/OFL.txt, modules/ui/eb-garamond/css/eb-garamond.css, modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2, modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2, modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2, plugins/O-ReverseDelay/CMakeLists.txt, plugins/O-ReverseDelay/Source/PluginEditor.cpp, plugins/O-ReverseDelay/Source/ui/public/index.html, plugins/O-ReverseDelay/Source/ui/public/css/styles.css</files>
  <read_first>
    - .planning/quick/260925-itj-r5-shared-eb-garamond-font-module-under-/260925-itj-RESEARCH.md: Findings 1-5 and "Recommended concrete layout + edits". Its CMake/provider/CSS snippets are the reference for the edits below.
    - plugins/O-Strata/tests/tools/cdp-font-probe.js (the boot, serve and CDP pattern to copy)
    - scripts/serve-ui.js lines 118-172 and 229-259 (SOURCES tokenising, url↔symbol pairing, `serve(root, onMiss)`)
    - plugins/O-ReverseDelay/CMakeLists.txt lines 1-20 and 60-92
    - plugins/O-ReverseDelay/Source/PluginEditor.cpp lines 141-149 and 179-228
    - plugins/O-ReverseDelay/Source/ui/public/index.html lines 20-30
    - plugins/O-ReverseDelay/Source/ui/public/css/styles.css lines 218-234
    - .claude/skills/plugin-improve/assets/backup-template.sh
  </read_first>
  <action>
**Step 0: preflight.**
- `mkdir -p "$ITJ/before" "$ITJ/after"`.
- Record `git rev-parse HEAD`, `git branch --show-current` (must be main) and `git status --short` in `$ITJ/preflight.txt`.
- `git status --short -- plugins/O-ReverseDelay PLUGINS.md modules scripts/regen-registry-used-by.sh` must print nothing. If it prints anything, STOP and report: another session is in this plugin.
- Confirm `plugins/O-ReverseDelay/CMakeLists.txt` line 16 reads `VERSION 1.21.1`.

**Step 1: write the probe.** Create `plugins/O-ReverseDelay/tests/tools/cdp-font-probe.js`. It is a new test file and does not touch the product.
- Header: the plugin's AGPL block (copy the wording from the top of styles.css, as a JS block comment), then a short design note citing memory pattern_declared_font_stack_is_not_the_resolved_face.
- Boot, copied from the O-Strata probe:
  - `S.resolvePlaywright()`. Exit 77 if it is unresolvable.
  - `S.buildRoot('O-ReverseDelay', { repoRoot })` and `S.readEditorSize`.
  - `LANGUAGES` imported from the built `js/i18n.js`.
  - `S.serve(built.root, (rel) => misses.push(rel))`, where the miss callback is the 404 recorder.
- Languages: all of `LANGUAGES` with `--all-languages`, otherwise en only.
- Per language:
  - `page.goto(..., { waitUntil: 'networkidle' })`
  - `window.__setLanguage(lang)`
  - `await page.evaluate(() => document.fonts.ready)`
  - wait 250 ms
- Targets: `.title`, `.title-accent`, `.subtitle`, `.group-label`, `.knob-label`, `.preset-name`, `.ab-slot`, `.fleuron`, `.footer-text`. For each, query the first match and print the faces (family / postScriptName / glyphCount / custom flag) plus the trimmed text.
- PASS rules, per language:
  - (a) every target node is found;
  - (b) every target whose text contains a Latin letter, a digit or U+2766 reports a face with familyName `EB Garamond`, `isCustomFont` true and glyphCount > 0. Any additional non-custom face on that node is allowed only if it is CJK (familyName `PingFang SC`, postScriptName prefix `PingFangSC`, or `Microsoft YaHei`), or if the text contains U+25BE or U+2699 (both absent from the font by design);
  - (c) under zh-Hans, every target whose text contains Han reports a PingFangSC-prefixed face;
  - (d) `[...document.fonts]` holds exactly three `EB Garamond` faces (400 normal, 400 italic, 700 normal), all with status `loaded`;
  - (e) `misses` is empty. The miss list is printed on failure.
- Options:
  - `--dump-rects FILE` writes `{ lang: { key: [x, y, w, h] } }` over every `body *` element. The key is `#id`, or else tagName plus `.`-joined classes, then `#` plus the document-order index (same keying as the research scratch probe.js). Values are rounded to 0.1 px.
  - `--diff-rects FILE` loads a previous dump and prints, per language: element count, vertical moves (|dy| > 0.5), horizontal-only moves, resized count, and up to 8 vertical movers with their dy.
  - `--screenshot DIR` saves `DIR/<lang>.png` after settling.
- Exit codes:
  - 2 if any PASS rule fails. The dump and screenshots are still written.
  - otherwise 3 if `--diff-rects` found any vertical move;
  - otherwise 0.
  - 1 on crash.

**Step 2: baselines on untouched v1.21.1.** No product file has been edited yet. Run each command in the background with its log in `$ITJ/before/`, and record its exit code:
- `node scripts/check-ui-labels.js --plugin O-ReverseDelay` (research: ALL CHECKS PASSED, exit 0, with 7 `<option>`s noted as never measured)
- `node plugins/O-ReverseDelay/tests/ui_frontend_check.js` (research: 183 passed, exit 0)
- `node plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js` (research: 155 passed, exit 0). Copy its `WINDOW budget` line verbatim into `$ITJ/before/window-line.txt`.
- `node scripts/check-i18n.js --plugin O-ReverseDelay`
- `node scripts/i18n-fr-lint.js --plugin O-ReverseDelay`
- `node scripts/i18n-zh-lint.js --plugin O-ReverseDelay`
- The probe: `node plugins/O-ReverseDelay/tests/tools/cdp-font-probe.js --all-languages --dump-rects "$ITJ/before/rects.json" --screenshot "$ITJ/before/shots"`.
  - EXPECTED RED: exit 2, with Latin runs on Times New Roman and no EB Garamond FontFaces. This is the probe's positive control.
  - If it exits 0 on v1.21.1, the probe is vacuous. Fix it before continuing (memory pattern_probe_must_target_the_branch_the_fix_changed).
- Noise-floor control: run the probe again with `--dump-rects "$ITJ/before/rects-2.json" --diff-rects "$ITJ/before/rects.json"`. Its printed diff must show 0 vertical moves in every language. If it does not, the stub animates some node vertically: name those nodes, exclude exactly them from the vertical count through a documented `NOISE` list in the probe, and re-run this control.

Whatever the baselines report is the "before" record. The after-gates in Task 3 must be no worse.

**Step 3: backup** (plugin-improve Phase 0.9, a CRITICAL gate).
- Create `backups/O-ReverseDelay/v1.21.1/` with the rsync from backup-template.sh (excluding build/, build.log, .DS_Store, *.user, .cache/).
- Run `./scripts/verify-backup.sh O-ReverseDelay 1.21.1`.
- HALT the plan if verification fails.

**Step 4: the module font assets.**
- Write `modules/ui/eb-garamond/tools/build-fonts.sh` as bash with `set -euo pipefail`, a short SPDX AGPL-3.0-or-later header comment and a usage line. It resolves the module dir from its own path. Always write variables with braces (`${SHA}`), never `$SHA:` (memory pattern_zsh_var_colon_modifier_eats_git_revpath). The script:
  1. Exports `SOURCE_DATE_EPOCH=1781654400` so fontTools timestamps are fixed.
  2. Downloads into a `mktemp -d` work dir, removed by `trap`, with `curl -fsSL --retry 3`:
     - `EBGaramond%5Bwght%5D.ttf`
     - `EBGaramond-Italic%5Bwght%5D.ttf`
     - `OFL.txt`

     from `https://raw.githubusercontent.com/google/fonts/${SHA}/ofl/ebgaramond/`, with SHA=f8c1d3d6cc75e30d77130bdcbfbff27e3b6233fe.
  3. Checks all three against the full sha256 values in `<context>` using `shasum -a 256 -c`, and aborts non-zero on any mismatch.
  4. Instances with `fonttools varLib.instancer ... --update-name-table`: roman wght=400 → Regular, roman wght=700 → Bold, italic wght=400 → Italic.
  5. Subsets each instance with `pyftsubset`, using the research's `UNI` unicode list and `FEAT` layout-feature list verbatim, `--name-IDs='0,1,2,3,4,5,6,13,14'` and `--flavor=woff2`.
  6. Bakes metrics with an inline python3 fontTools step per file (logic of the research's bake2.py):
     - hhea: ascent 891, descent -216, lineGap 42;
     - OS/2: sTypoAscender 891, sTypoDescender -216, sTypoLineGap 42;
     - `fsSelection |= 1<<7` (USE_TYPO_METRICS);
     - usWinAscent = max(933, head.yMax), usWinDescent = max(216, -head.yMin);
     - saves as woff2 to `fonts/EBGaramond-Regular.woff2`, `fonts/EBGaramond-Italic.woff2` and `fonts/EBGaramond-Bold.woff2`.
  7. Copies the verified `OFL.txt` to the module root.
  8. Prints each output's sha256 and byte size.

  The downloaded TTFs never land in the repo.
- Run the script twice. The three output sha256 values must be identical across the two runs. If not, find and fix the non-determinism before going on; do not commit non-reproducible fonts.
- Read the metrics back with a python3 fontTools one-off. For each file check:
  - the hhea and typo values above;
  - bit 7 of fsSelection is set;
  - usWeightClass is 400 / 400 / 700;
  - name ID 1 is `EB Garamond`;
  - the cmap contains U+00E9, U+2014, U+2019 and U+2766;
  - size is under 50 000 B.

  Save the output to `$ITJ/font-readback.txt`.
- Write `modules/ui/eb-garamond/css/eb-garamond.css` with, in order:
  1. The AGPL block comment in the preset-manager.js style ("This file is part of the Ouaricon Audio eb-garamond module", Copyright (C) 2026 Ouaricon Audio, SPDX AGPL-3.0-or-later, the standard notice).
  2. A second block comment stating:
     - the served font files are EB Garamond, Copyright 2017 The EB Garamond Project Authors (https://github.com/octaviopardo/EBGaramond12), under the SIL Open Font License 1.1;
     - these files are an OFL Modified Version: subset, static instances, Times-matched vertical metrics, with no Reserved Font Name clause, so the name is kept;
     - then the full OFL.txt text verbatim. First confirm that OFL.txt contains no `*/` sequence.
  3. Exactly the research's three `@font-face` rules: family `'EB Garamond'`, `src: url('../fonts/EBGaramond-<Face>.woff2') format('woff2')`, weights and styles 400 normal / 400 italic / 700 normal, `font-display: block`.

  Deliberately omit three things, per research §4:
  - any locally-installed-font source, because a user's installed EB Garamond with native metrics would undo the bake;
  - `unicode-range`;
  - preload.

**Step 5: wire O-ReverseDelay.** This is also the version bump: plugin-improve Phase 1, MINOR, a new bundled face.
- `CMakeLists.txt`:
  - line 16 `VERSION 1.21.1` becomes `VERSION 1.22.0`;
  - inside the EXISTING `juce_add_binary_data(OuariconReverseDelay_UIResources ...)` SOURCES block, after the preset-manager.js entry, append the four `${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/...` entries for `css/eb-garamond.css` and `fonts/EBGaramond-{Regular,Italic,Bold}.woff2`;
  - add a comment above them naming the module and the hyphen-stripped symbols (ebgaramond_css, EBGaramondRegular_woff2, EBGaramondItalic_woff2, EBGaramondBold_woff2). That comment must keep parentheses balanced and must contain NO word ending in a file extension, because serve-ui tokenises comments as sources. Write "the eb-garamond stylesheet and three woff2 faces", not filenames;
  - never add a second binary-data target.
- `Source/PluginEditor.cpp`:
  - insert the research's four branches immediately before the final `return std::nullopt;` of `getResource()`, in exactly the literal `if (url == "...") return makeBinaryResource (UIBinaryData::<sym>, UIBinaryData::<sym>Size, "<mime>");` shape;
  - MIME is `"text/css; charset=utf-8"` for the stylesheet and `"font/woff2"` for the three fonts;
  - use a one-line comment above the group; never write the `url ==` text inside a comment.
- `index.html`: insert `<link rel="stylesheet" href="css/eb-garamond.css" />` on its own line directly before the existing styles.css link.
- `styles.css`:
  - the `--serif` declaration becomes exactly `--serif: 'EB Garamond', 'Georgia', 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif;`;
  - rewrite the comment above it as a v1.22.0 paragraph plus the kept v1.12.0 CJK-tail rationale. It should say:
    - the face is bundled (modules/ui/eb-garamond, served at /css/eb-garamond.css) and comes first;
    - Georgia precedes Times so any fallback converges on the R5 suite order;
    - the old bare-Garamond first entry is gone, so an Office-installed Windows Garamond cannot outrank the bundled face;
    - the CJK tail still sits before the generic.
  - Confirm with a comment-stripped scan that every `font-family` declaration in styles.css still reads `var(--serif)`.
  </action>
  <verify>
    <automated>node plugins/O-ReverseDelay/tests/tools/cdp-font-probe.js --all-languages --dump-rects "$ITJ/after/rects.json" --diff-rects "$ITJ/before/rects.json" --screenshot "$ITJ/after/shots"; echo "probe exit=$?" # must be 0: EB Garamond (custom) on every Latin run in en/fr/zh-Hans, PingFang on Han, 3 FontFaces loaded, 0 misses, 0 vertical moves</automated>
    <automated>bash modules/ui/eb-garamond/tools/build-fonts.sh > "$ITJ/build-fonts-run3.log" 2>&1 && shasum -a 256 modules/ui/eb-garamond/fonts/*.woff2 # hashes equal the two earlier runs</automated>
    <automated>grep -c "^\s*--serif: 'EB Garamond', 'Georgia', 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif;" plugins/O-ReverseDelay/Source/ui/public/css/styles.css # 1</automated>
  </verify>
  <done>
- The baseline logs exist in `$ITJ/before/`, and the probe exited 2 on v1.21.1 (RED recorded).
- The noise-floor diff shows 0 vertical moves.
- `backups/O-ReverseDelay/v1.21.1/` passes verify-backup.sh.
- The three woff2 files are reproducible and read back with 891/-216/42 + USE_TYPO_METRICS, each under 50 000 B.
- The CSS carries the AGPL + OFL notices and three @font-face rules.
- CMake VERSION is 1.22.0, with 4 new SOURCES in the single target.
- The 4 provider branches and the link are in place, and the `--serif` line is exact.
- The probe exits 0 with 0 vertical moves in all three languages.
- Nothing is committed yet.
  </done>
</task>

<task type="auto">
  <name>Task 2: Package the module (module.yaml, README, snippet, registry) and teach every served tree and gate about the font — each change shown to bite</name>
  <files>modules/ui/eb-garamond/module.yaml, modules/ui/eb-garamond/README.md, modules/ui/eb-garamond/snippets/getResource.cpp, modules/registry.yaml, scripts/regen-registry-used-by.sh, plugins/O-ReverseDelay/tests/ui_frontend_check.js, plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js, plugins/O-ReverseDelay/tests/ui-stub/serve-stub.sh</files>
  <read_first>
    - plugins/O-ReverseDelay/tests/ui_frontend_check.js lines 378-452 (§9)
    - plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js lines 150-185 and the goto near line 277
    - plugins/O-ReverseDelay/tests/ui-stub/serve-stub.sh
    - scripts/regen-registry-used-by.sh (header lines 1-20 and `module_tokens()` near line 62)
    - modules/registry.yaml (header lines 1-41 and the last entry, compressor-unit, up to the column-0 HOW-TO block)
    - modules/persistence/preset-manager/module.yaml (shape to mirror)
  </read_first>
  <action>
**A. ui_frontend_check.js §9** (research §5 item 3; it is red right now because of Task 1's module paths).
1. Next to the existing `modules/.../js/*.js` loop, add a second `matchAll` over the comment-stripped binary block. It maps `${CMAKE_SOURCE_DIR}/modules/<anything>/(css|fonts)/<file>` to the served path `/<css|fonts>/<file>`, resolved on disk against repoRoot.
2. Add a check that every `url(...)` in every embedded `.css` file resolves to a `getResource()` path. Skip `data:` and absolute URLs, and resolve relative to that CSS file's served directory with path.posix. Print how many URLs were resolved, and assert that the eb-garamond stylesheet contributed exactly 3 font URLs, so the check cannot go vacuous.
3. Add a check that every provider branch whose URL ends in `.woff2` passes `"font/woff2"`. Use the same 400-char window serve-ui uses.

Keep every existing check unchanged, including "exactly ONE juce_add_binary_data target". Run it: exit 0, and record the new pass count.

Negative control (required):
- Save `git diff -- plugins/O-ReverseDelay/Source/PluginEditor.cpp` to `$ITJ/ctl-pre.diff`.
- Change the Bold branch's URL to a misspelled name (`EBGaramond-Bld`) with a targeted Edit.
- Re-run. §9 must FAIL, naming the Bold font URL.
- Undo with the reverse targeted Edit, re-run to a pass, and confirm `git diff` of PluginEditor.cpp is byte-identical to `$ITJ/ctl-pre.diff`.

**B. ui_tooltip_clamp_check.js.**
- In `buildRoot()`, copy `modules/ui/eb-garamond/css/eb-garamond.css` to `<root>/css/eb-garamond.css` and every `modules/ui/eb-garamond/fonts/*.woff2` into `<root>/fonts/` (mkdir first). Add a comment in the same voice as the preset-manager copy explaining why.
- Add `'.woff2': 'font/woff2'` to `MIME`.
- After the networkidle `page.goto`, `await page.evaluate(() => document.fonts.ready)`.
- Add one check: "bundled EB Garamond faces loaded". It passes when ≥ 1 `EB Garamond` FontFace has status `loaded` and none has status `error`.
- Run in the background: exit 0. The `WINDOW budget` line must be byte-identical to `$ITJ/before/window-line.txt`.

**C. serve-stub.sh.** After the preset-manager copy, add `mkdir -p "$ROOT/fonts"` and `cp` lines for the css and the three woff2, with a comment matching the existing one. Verify:
- launch it in the background on port 8797 with root `$ITJ/stubroot`;
- `curl -s -o /dev/null -w '%{http_code} %{content_type}\n'` the css and each font URL. All four must return 200;
- kill the server. Port 8797 avoids the shared-port clash in memory pattern_ui_test_server_port_clash_serves_other_session.

**D. Module packaging.**
- `module.yaml`, mirroring preset-manager's shape:
  - name `eb-garamond`, version `1.0.0`, category `ui`, author Ouaricon Audio;
  - licence note: fonts OFL-1.1, css/snippet/tools AGPL-3.0-or-later;
  - `provides`: the stylesheet, the three woff2 files, the served URLs, font-family `EB Garamond`, and weights 400 / 400 italic / 700;
  - `source`: the google/fonts commit, the three input sha256 values and the output sha256 values from `$ITJ/font-readback.txt`;
  - `metrics`: 891/216/42 with USE_TYPO_METRICS;
  - `integration: direct-embed`;
  - `dependencies: []`.
- `README.md`:
  - The four integration steps:
    1. append to the plugin's EXISTING single UIResources SOURCES;
    2. paste the getResource branches;
    3. add the link before the plugin stylesheet;
    4. put `'EB Garamond'` first in the font token, with Georgia before Times and the CJK tail before generic `serif`.
  - The hyphen-stripped symbol names.
  - Why the Times metrics are baked in (research §3 numbers: native 1.305 em moved 208/278 elements; Times-exact moved 0), and why CSS ascent-override is not used (no WKWebView support).
  - Why font-display block, and why there is no local/preload/unicode-range.
  - The three served-tree pitfalls: hand-built test trees must copy css + fonts; MIME maps need `.woff2`; §9-style gates must map css/ and fonts/.
  - The serve-ui comment-tokenising caveat.
  - The glyph gaps U+25BE / U+2699.
  - Windows parity marked ASSUMED until a Windows build is looked at.
  - The rebuild recipe (`bash tools/build-fonts.sh`) and the OFL obligations.
- `snippets/getResource.cpp`: the AGPL header plus the four branches as a copy-paste snippet, with a comment that it assumes the plugin's `makeBinaryResource (data, size, mime)` helper and a `UIBinaryData` namespace. `snippets/` is not globbed by OuariconModules.cmake, which only globs `cpp/`.

**E. Registry + regen tool** (research §6 pitfall).
1. `cp modules/registry.yaml "$ITJ/registry.head.yaml"`.
2. Hand-add an indented `  # UI MODULES` section banner and the `eb-garamond` entry:
   - fields: path `ui/eb-garamond`, version 1.0.0, a description, category ui, provides, `dependencies: []`, tags `[ui, font, typography, webview, ofl]`, a reuse_score, and `used_by` O-ReverseDelay version 1.22.0;
   - place it AFTER compressor-unit's used_by and BEFORE the column-0 HOW-TO block;
   - bump the header to `version: 1.2.0` and `last_updated: 2026-09-25`;
   - widen the `ui` category description to include shared UI assets (fonts).

   Then `cp modules/registry.yaml "$ITJ/registry.hand.yaml"`.
3. RED: run `bash scripts/regen-registry-used-by.sh` with the script still unextended. Confirm it rewrites eb-garamond's used_by to `[]`, then `cp "$ITJ/registry.hand.yaml" modules/registry.yaml`. Note in the SUMMARY whether that run also touched other modules.
4. Extend `module_tokens()` to walk `('cpp', 'js', 'css', 'fonts')` and accept `.css` and `.woff2` basenames. Update the script's header comment and the docstring to match.
5. GREEN: run the regen again. `diff "$ITJ/registry.hand.yaml" modules/registry.yaml` must be empty.
   - If it differs ONLY in other modules' used_by blocks, that is pre-existing drift: restore registry.hand.yaml and record the drift in the SUMMARY; do not commit it.
   - If eb-garamond's block differs, fix the extension.
  </action>
  <verify>
    <automated>node plugins/O-ReverseDelay/tests/ui_frontend_check.js > "$ITJ/after/ui_frontend_check.log" 2>&1; echo "exit=$?"; grep -c "FAIL:" "$ITJ/after/ui_frontend_check.log" # exit 0, 0 FAIL</automated>
    <automated>node plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js > "$ITJ/after/ui_tooltip_clamp_check.log" 2>&1; echo "exit=$?"; diff <(grep "WINDOW budget" "$ITJ/after/ui_tooltip_clamp_check.log") "$ITJ/before/window-line.txt" && echo WINDOW-identical # run in background; exit 0 + WINDOW-identical</automated>
    <automated>bash scripts/regen-registry-used-by.sh && diff "$ITJ/registry.hand.yaml" modules/registry.yaml && echo REGEN-IDEMPOTENT</automated>
  </verify>
  <done>
- ui_frontend_check exits 0 with the new §9 checks. The misspelled-Bold control failed it, and the revert restored the exact prior diff.
- The tooltip-clamp check exits 0 with the fonts loaded and the WINDOW line unchanged.
- serve-stub serves the css + 3 fonts with 200.
- module.yaml, README.md and snippets/getResource.cpp exist.
- The registry carries eb-garamond 1.0.0 (used_by O-ReverseDelay 1.22.0), and the extended regen reproduces it exactly, after the unextended regen was observed rewriting it to [].
- Nothing is committed yet.
  </done>
</task>

<task type="auto">
  <name>Task 3: After-gates vs baseline, build + install + auval + pluginval, WKWebView face check, v1.22.0 docs, two path-scoped commits — then stop</name>
  <files>plugins/O-ReverseDelay/CHANGELOG.md, plugins/O-ReverseDelay/NOTES.md, PLUGINS.md</files>
  <read_first>
    - plugins/O-ReverseDelay/CHANGELOG.md lines 1-70 (entry shape; the [1.21.1] entry carries the "Since the last published release" block)
    - plugins/O-ReverseDelay/NOTES.md lines 1-8 and 110-118 (Version line + timeline format)
    - PLUGINS.md line 68 (the O-ReverseDelay row)
    - ~/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/pattern_standalone_hands_on_via_ax_scripting_and_window_capture.md
  </read_first>
  <action>
**A. After-gates.** Run all in the background, with logs in `$ITJ/after/`, and compare each against `$ITJ/before/`:
- `node scripts/check-ui-labels.js --plugin O-ReverseDelay`:
  - exit 0 with ALL CHECKS PASSED;
  - "every requested resource was served" passes;
  - no FAIL that is not in the baseline;
  - the never-measured `<option>` note is unchanged.
- The ui_frontend_check and tooltip-clamp results from Task 2 must still hold. Re-run both if any file changed since.
- check-i18n, i18n-fr-lint and i18n-zh-lint (each `--plugin O-ReverseDelay`) exit with their baseline codes. No copy changed.
- Probe: `--all-languages --diff-rects "$ITJ/before/rects.json"` exits 0 with 0 vertical moves.
- Also check:
  - `grep -c "GNU Affero General Public License"` is ≥ 1 in each of eb-garamond.css, snippets/getResource.cpp and cdp-font-probe.js;
  - `git diff --stat HEAD -- plugins/O-ReverseDelay/Source/dsp plugins/O-ReverseDelay/Source/PluginProcessor.cpp plugins/O-ReverseDelay/Source/PluginProcessor.h` is empty, which proves audio is untouched (no harness --digest run is needed for a UI-only change).

If any after-gate regresses or a label overflows, fix it inside O-ReverseDelay only (CSS or width pins) and re-run every after-gate. If it cannot be fixed there, STOP and report. Never touch another plugin.

**B. Build + install** per CLAUDE.md.
- Run `./scripts/build-and-install.sh O-ReverseDelay` in the background, logging to `$ITJ/build.log`. Its Phase 4 kills AudioComponentRegistrar, clears both AU caches, and sweeps the -dev and unsuffixed variants. Note any "Sweeping ALTERNATE-variant" warning.
- Confirm `ls ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components | grep -i ReverseDelay` lists only `O-ReverseDelay-dev.vst3` and `O-ReverseDelay-dev.component`.
- `CFBundleShortVersionString` in both installed Info.plist files (PlistBuddy) is `1.22.0`.
- `strings -a` on each installed Contents/MacOS binary, piped to `grep -c '/fonts/EBGaramond-Regular.woff2'`, is ≥ 1. This proves the binaries carry the provider and are not stale.

**C. auval.** Run `auval -v aufx ORvD OuDv` in the background; it may do a slow cold registry rescan. The log must contain `AU VALIDATION SUCCEEDED`. Never run the all-plugins auval sweep: it SIGABRTs on this machine.

**D. pluginval.** Run `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/O-ReverseDelay-dev.vst3` in the background. Expect SUCCESS, as for v1.21.1.

**E. WKWebView face check.** This is the only proof for research assumptions A1 and A3.
- `build-and-install.sh` never rebuilds the Standalone. Run `cmake --build build --target OuariconReverseDelay_Standalone` in the background.
- Locate the app with `ls -d build/plugins/O-ReverseDelay/*_artefacts/Release/Standalone/*.app`, and run the same `strings` check on its binary.
- Launch with `open -g`. Find the window id with the CGWindowList Swift helper and capture it with `screencapture -x -l <id>` to `$ITJ/after/standalone.png`, following the memory pattern:
  - check the foreground app first;
  - never `activate`;
  - static DOM text paints even when the window is occluded.
- Read the PNG. Compare the title, its italic accent, the group labels and the fleurons against `$ITJ/after/shots/en.png` (EB Garamond) and `$ITJ/before/shots/en.png` (Times). Look at descenders inside the two text-overflow ellipsis boxes (styles.css:653, 742).
- Quit with `tell application ... to quit`.
- If a capture is impossible without taking focus, record `WKWebView visual: PENDING (human — Standalone or DAW)` in the SUMMARY and the CHANGELOG Tests list, and continue. Do not block.

**F. Docs** (plugin-improve Phases 4 and 7).
- `CHANGELOG.md`: add a new `## [1.22.0] — 2026-09-25` entry directly above `## [1.21.1]`. It is MINOR. Leave the [1.21.1] entry, including its "Since the last published release" block, untouched: 1.21.1 is the published release. Sections:
  - **Added:** the bundled face from the shared module eb-garamond 1.0.0 (400 / 400i / 700, Latin + Latin-Ext subset, ~117 KB, embedded and served at /css + /fonts).
  - **Changed:** the font token order. EB Garamond comes first, Georgia before Times, and the bare Garamond entry is removed. Text is ~5% narrower with a smaller x-height; line boxes are unchanged thanks to the baked Times metrics.
  - **Tests:** the probe RED→GREEN, the 0 vertical moves, the gate counts before → after, check-ui-labels, auval, pluginval-10 and the WKWebView check result.
  - **Notes:** Windows parity is assumed until a Windows build is checked; U+25BE / U+2699 still fall back per glyph as before; audio is untouched; factory presets are re-seeded by the VERSION bump with identical content.
- `NOTES.md`: line 5 becomes `- **Version:** 1.22.0`. Append a `- **2026-09-25 (v1.22.0):** Minor. ...` timeline entry in the house style, with the key numbers.
- `PLUGINS.md`: in the O-ReverseDelay row only, the version becomes 1.22.0 and the date 2026-09-25.

**G. Commits.** Two, path-scoped. NO tag at any point: this overrides the improve skill's Phase 6 tag step, per memory feedback_never_tag_unless_publish.
1. Immediately before each commit, re-run `git branch --show-current` (must be main) and `git status --short`. `git status --short -- plugins/O-ReverseDelay modules scripts PLUGINS.md` must list only this plan's files.
2. `git add` the NEW paths explicitly, because pathspec commits skip untracked files (memory pattern_git_commit_pathspec_takes_only_tracked_files):
   - `modules/ui/eb-garamond`
   - `plugins/O-ReverseDelay/tests/tools/cdp-font-probe.js`
3. Commit 1: `git commit -m "feat(modules/ui): eb-garamond 1.0.0 — shared OFL face with Times-matched metrics (R5)" -- modules/ui/eb-garamond modules/registry.yaml scripts/regen-registry-used-by.sh`.
4. PLUGINS.md gate: `git diff -U0 -- PLUGINS.md` must show exactly one removed and one added line, both the O-ReverseDelay row. If it shows anything else, leave PLUGINS.md out of Commit 2 and report it.
5. Commit 2: `git commit -m "improve(O-ReverseDelay): v1.22.0 — bundled EB Garamond face (R5 pilot)" -- plugins/O-ReverseDelay PLUGINS.md`.
6. After each commit, `git show --stat HEAD` must list every new file and no foreign path (nothing under O-Formant or O-MultiBandCompressor). Use the repo's usual Co-Authored-By trailer.

Then STOP. Do not migrate any other plugin. List the other font-token plugins as the next step in the SUMMARY only.
  </action>
  <verify>
    <automated>grep -c "AU VALIDATION SUCCEEDED" "$ITJ/auval.log"; grep -c "SUCCESS" "$ITJ/pluginval.log"; /usr/libexec/PlistBuddy -c 'Print CFBundleShortVersionString' ~/Library/Audio/Plug-Ins/VST3/O-ReverseDelay-dev.vst3/Contents/Info.plist # >=1, >=1, 1.22.0</automated>
    <automated>git --no-pager log -2 --stat --format='%h %s' && TAGS="$(git tag --points-at HEAD)" && [ -z "$TAGS" ] && echo NO-TAGS # two commits as specified, NO-TAGS printed</automated>
    <automated>grep -c "^## \[1.22.0\] — 2026-09-25" plugins/O-ReverseDelay/CHANGELOG.md; grep -c "Version:\*\* 1.22.0" plugins/O-ReverseDelay/NOTES.md; grep -c "| O-ReverseDelay | .* | 1.22.0 |" PLUGINS.md # 1, 1, 1</automated>
  </verify>
  <done>
- Every after-gate is no worse than baseline: check-ui-labels passes, including served resources; ui_frontend_check and tooltip-clamp are 0 FAIL with the WINDOW line identical; i18n gates are at baseline codes; the probe exits 0 with 0 vertical moves.
- v1.22.0 is installed as the -dev variant only, and its binaries carry the font provider.
- auval and pluginval-10 succeed.
- The WKWebView face is confirmed by capture, or recorded as PENDING.
- CHANGELOG, NOTES and PLUGINS.md read 1.22.0.
- The two path-scoped commits list all new files and no foreign paths. No tag. No other plugin touched.
  </done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| internet → build-fonts.sh | Font binaries and the licence are downloaded from raw.githubusercontent.com at build-recipe time |
| page → getResource() | The WebView requests URLs; the provider answers from embedded bytes |
| concurrent sessions → shared checkout | Other sessions share `.git/index` and HEAD in the same working tree |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-itj-SC | Tampering | build-fonts.sh download | high | mitigate | Pinned google/fonts commit f8c1d3d6. Full sha256 of all three inputs is checked with `shasum -c`, and the script aborts on mismatch. Outputs are reproducible (two runs, identical hashes). No npm or pip installs: fontTools 4.61.1 and brotli are already present |
| T-itj-01 | Elevation of Privilege | PluginEditor.cpp getResource() | low | mitigate | Four exact-match `url ==` branches, with no path joining or traversal. Unknown URLs still return nullopt (404). §9 asserts provider == CMake == HTML/CSS refs |
| T-itj-02 | Denial of Service | font-display: block + a missing font = invisible text for ~3 s | medium | mitigate | The probe asserts all 3 FontFaces loaded and 0 serve misses. check-ui-labels asserts every requested resource was served. The tooltip-clamp check asserts no FontFace errored. `strings` proves the installed and Standalone binaries carry the branches |
| T-itj-03 | Repudiation | OFL licence notice inside a binary-embedded font | low | mitigate | OFL.txt in the module. The EB Garamond copyright and full OFL text in the embedded CSS comment. Name IDs 0/13/14 kept by the subset |
| T-itj-04 | Tampering | git commits in a shared checkout | high | mitigate | Only named new paths are `git add`ed. Path-scoped `git commit -- <paths>`, with branch and status re-checked immediately before each commit and `git show --stat` checked after. O-Formant and O-MultiBandCompressor are never staged. PLUGINS.md is committed only if its diff is the O-ReverseDelay row alone. No tags |
| T-itj-05 | Information Disclosure | static font/CSS assets | low | accept | Public OFL assets; no user data crosses any boundary |
</threat_model>

<verification>
- Before: `$ITJ/before/` holds the six gate logs with exit codes, the WINDOW line, rects.json and rects-2.json (noise floor 0 vertical), screenshots, and the probe RED (exit 2).
- After: the probe exits 0 (EB Garamond custom on all Latin runs, PingFang on Han, 3 faces loaded, 0 misses) with 0 vertical moves in en, fr and zh-Hans. check-ui-labels, ui_frontend_check and ui_tooltip_clamp_check exit 0 with the WINDOW line identical. The i18n gates are at baseline codes.
- Controls observed: probe RED on v1.21.1; §9 FAIL on the misspelled Bold URL (reverted, diff byte-identical); regen rewrote used_by to [] before the extension.
- Fonts: two builds hash-identical; readback shows 891/-216/42 + USE_TYPO_METRICS, 400/400/700, each under 50 000 B.
- Ship: build-and-install succeeded, -dev only installed, bundle version 1.22.0, font provider string in the VST3, AU and Standalone binaries. `auval -v aufx ORvD OuDv` succeeded, and pluginval-10 VST3 reported SUCCESS.
- Git: two path-scoped commits, every new file listed, no foreign paths, no tags.
</verification>

<success_criteria>
- `modules/ui/eb-garamond/` exists with 3 woff2 files (~117 KB total), OFL.txt, the css, module.yaml, README.md, snippets/getResource.cpp and tools/build-fonts.sh, and is registered as eb-garamond 1.0.0, used_by O-ReverseDelay 1.22.0.
- O-ReverseDelay v1.22.0 renders its Latin UI in the bundled EB Garamond with no vertical layout change, passes every UI and i18n gate it passed at v1.21.1, is installed, and validates under auval and pluginval-10.
- The pilot is complete and stops there. The SUMMARY records before/after numbers, the WKWebView result (confirmed or PENDING) and any registry drift found. It lists the rollout to other plugins as a next step only.
</success_criteria>

<output>
Create `.planning/quick/260925-itj-r5-shared-eb-garamond-font-module-under-/260925-itj-SUMMARY.md` when done. Include the before → after gate table, the three control observations, the font hashes and sizes, the commit SHAs, and the WKWebView verdict.
</output>
