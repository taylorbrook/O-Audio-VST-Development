---
milestone: ui-hardening-phase0
scope: cross-plugin
domain: mixed (CI script + generator templates + C++ editor + page JS)
execute_agent: general-purpose
version_bump: patch (per touched plugin)
created: 2026-10-26
verified_at_head: ea44af28
---

# PLAN — ui-hardening-phase0

All line refs below were re-verified at plan time (2026-10-26, HEAD `ea44af28`). Re-grep before
each edit anyway: other sessions are live in this checkout.

## Ground rules (apply to every task)

- **Unblock check before touching any plugin:** `git status --short plugins/<Name>` must be empty.
  If not, skip that plugin, record it in SUMMARY.md, move on.
- **Never touch:** O-Reed, O-Bassoon, O-simpleFM, O-Gain, O-Freeze, O-GrainScatter (B4 / live work).
- **Backup** `plugins/<Name>` → `backups/<Name>/v<current>/` before the first edit to that plugin.
- **One PATCH bump + CHANGELOG `## [X.Y.Z] - 2026-10-26` entry + PLUGINS.md row per plugin**, bundling
  every Phase-0 fix that plugin needs (table at the end).
- **Commits:** path-scoped, one per plugin (`git commit -- plugins/<Name> PLUGINS.md`). Re-run
  `git branch --show-current && git status --short` immediately before each commit; if anything
  foreign is staged, use the temp-index procedure (memory `index_git_shared_checkout.md`).
  Message: `improve: <Name> v<X.Y.Z> - ui-hardening-phase0 (<fixes>)`. **NO tags.**
- No visual change on the normal path. No DSP / parameter / state-format change.

---

## B1 — CI gate (repo-level, no plugin bumps except O-DigiDelay)

### T1.1 `scripts/sync-ui-modules.js` (new)
Mirror `ouaricon_add_module()`'s JS copy (`modules/cmake/OuariconModules.cmake:104-118`) in plain Node:
- For each `plugins/*/CMakeLists.txt`, strip `#` comments, match `ouaricon_add_module(<target> <module>)`.
- Resolve the module dir by searching categories in CMake's order
  `core persistence metering tuning modulation synthesis effects ui` (line 34). Unknown module → exit 1
  (CMake would FATAL_ERROR).
- If `<moduleDir>/js/*.js` exists, copy each into `plugins/<P>/Source/ui/public/modules/` (mkdir -p),
  byte-for-byte, exactly like `configure_file(COPYONLY)` (overwrites).
- Print one line per copy; `--check` flag = report what would change, write nothing, exit 1 on drift.
- Header comment explains: CI never configures CMake, the copies are gitignored (`.gitignore:26`), so
  without this `check-i18n [12]` fails on fresh clones. Direct embed deferred to Phase 3 kit.
- Note in the header: running it locally writes only gitignored paths, plus 4 tracked copies
  (O-AnalogEQ/Chorus/Detune/Tremolo) that are currently byte-identical to the module — same effect
  as a CMake configure.

**Verify:** on a fresh `git archive HEAD scripts plugins modules` extract, run sync then
`node scripts/check-i18n.js` → the 11 `[12]` FAILs disappear (baseline reproduced at plan time:
O-Bitrot, O-Emulator, O-MultiBandCompressor, O-Tapestop, O-simpleAmbisonics, O-simpleFM, O-simpleGrain,
O-simplePhysicalModelSynth, O-simpleReverb, O-simpleSampler, O-simpleWavetable — exit 11).

### T1.2 Wire into `.github/workflows/ui-static-gates.yml`
Add step `sync configure-time module copies` → `node scripts/sync-ui-modules.js` after setup-node,
before `check-i18n`. Update the header comment (one line: why the step exists).

### T1.3 O-DigiDelay fr-lint T4
`plugins/O-DigiDelay/Source/ui/public/js/i18n.js:408` `'Nom de ce préréglage :'` → U+00A0 before the
colon (`'Nom de ce préréglage :'`, match how sibling entries spell it in that file). Bump
1.9.2 → 1.9.3.

**B1 acceptance:** fresh `git archive` of the new HEAD + sync → `check-i18n` exit 0, `i18n-fr-lint`
exit 0, `i18n-zh-lint` exit 0. Commit B1 as two commits: `scripts/ .github/` (ci:) and O-DigiDelay.

---

## B2 — Generator templates (no plugin bumps)

Files (all hits from `grep -rln "tempDirectory\|getToggleButtonState" .claude/agents .claude/skills`):
- `.claude/skills/ui-mockup/assets/webview-templates/PluginEditor-webview.cpp`
- `.claude/skills/ui-mockup/assets/webview-templates/PluginEditor-webview.h`
- `.claude/skills/ui-mockup/assets/webview-templates/CMakeLists-webview-snippet.cmake`
- `.claude/skills/ui-mockup/assets/webview-templates/index-template.html`
- `.claude/agents/gui-agent.md` (canonical block :618-645, checklist :826-827, troubleshooting :1114-1128)
- `.claude/agents/ui-finalization-agent.md`
- `.claude/skills/ui-mockup/references/ui-design-rules.md`, `references/html-generation.md`

### T2.1 Editor template + gui-agent canonical block
Canonical Windows block = the O-simpleSubtractive v1.7.4 convention (`git show 15968d17`):
- `.withBackend (Backend::webview2)` inside `#if JUCE_WINDOWS`.
- User-data folder `userApplicationDataDirectory/Ouaricon/WebView2/<PluginName>`, `createDirectory()`
  before building options.
- `areOptionsSupported (options)` probe; `webView` null on failure → native paint notice +
  `HyperlinkButton` to the WebView2 Runtime; attachments still constructed; `resized()` null-safe.
- `withKeepPageLoadedWhenBrowserIsHidden()` marked **REQUIRED** (remove "OPTIONAL … Remove if not
  targeting FL Studio").
- Header template: add `std::unique_ptr<juce::HyperlinkButton> webView2RuntimeLink;`.

### T2.2 CMake snippet
Add `NEEDS_WEBVIEW2 TRUE` (to `juce_add_plugin`, documented as such) and
`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` in the compile definitions, with a one-line why (memory
`critical_webview2_static_linking.md`). Mirror O-simpleSubtractive's CMakeLists:22,97.

### T2.3 Toggle API
`Juce.getToggleButtonState` → `Juce.getToggleState` (JUCE `index.js:374`) at `index-template.html:340`,
the template .cpp comment (:62), and the other 10 hits in agents/skills (`grep -rn getToggleButtonState
.claude/ | grep -v agent-memory`).

**B2 acceptance:** `grep -rn "getToggleButtonState\|tempDirectory" .claude/agents .claude/skills` → 0
hits (except prose explicitly describing the old bug); every emitted-editor sample contains
`withBackend`, `NEEDS_WEBVIEW2` appears in the CMake snippet. Commit: `-- .claude/agents .claude/skills`.

---

## B3 — Defects in free trees (per plugin)

### T3.1 O-Strata storage key
`plugins/O-Strata/Source/ui/public/index.html:2739,2745` `'oprism.tipsEnabled'` → `'ostrata.tipsEnabled'`.
Side effect (CHANGELOG): Strata's hover-help preference resets to default once. No migration (the old
key is shared with O-Prism, so it cannot be attributed).

### T3.2 In-page dialogs (no-op `prompt`/`confirm` in WKWebView)
Suite census at plan time: exactly these three live calls.
- **O-IntonationPad** `Source/ui/public/js/app.js:1302` rename → inline edit: swap the item's name node for
  an `<input>` prefilled with the name; Enter/blur commits (via existing `renamePreset` native), Escape
  cancels. `:1320` delete → armed-confirm on the ✕ button (first click arms + label swap, second click
  within 2.5 s deletes; reference `armedConfirmDelete` in `plugins/O-Tapestop/Source/ui/public/js/app.js:703`).
- **O-Polystutter** `Source/ui/public/js/app.js:553` → same armed-confirm shape.
- Reuse existing i18n keys where possible; any new string gets en/fr/zh-Hans rows (fr reviewed per
  glossary; zh `reviewed: 'mt'`). `check-i18n` + both lints must stay green.
- Styling: reuse existing button/input classes so the idle look is unchanged.
- Leave O-Polystutter's vendored `modules/preset-manager.js:336` `promptDelete()` alone — unreferenced by
  `app.js` (verify), and module copies are Phase 3.

### T3.3 Locale-independent JSON / JS payloads
Replace `String::formatted("%f" …)` with `juce::String (value, decimals)` concatenation (JUCE formats
with the classic locale) and map non-finite values to the plugin's own floor (read the JS meter floor —
do not invent one). **Payload type and shape unchanged** — check the JS consumer before editing.
- O-Comp `Source/PluginEditor.cpp:286` (`updateMeters` call, 4 floats)
- O-Bass `Source/PluginEditor.cpp:268` (`updateMeters` call, 2 floats)
- O-Bowed `Source/PluginEditor.cpp:160` (native fn returns a JSON string, `%.4f` ×6)
- O-Orbit `Source/PluginEditor.cpp:740` (`motionUpdate` event, JSON string). **Flag:** O-Orbit's STATUS
  still lists a pending DAW check (v1.4.2 halo; tree is now v1.4.4). Include that check in the DAW list.

### T3.4 WebView2 user-data folder + availability notice (the 14 bare-`%TEMP%` plugins)
Re-verified: these pass `File::tempDirectory` with **no** child folder. All already have
`withBackend(webview2)` and the CMake WebView2 flags; none has the probe.
O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bells, O-Chorus, O-Comp, O-Detune, O-IntonationPad, O-Lyrica,
O-Marimba, O-MultiBandCompressor, O-Polystutter, O-SpectralShaper, O-Tremolo.

Port `15968d17` per plugin: app-data folder `Ouaricon/WebView2/<Name>` created first, `areOptionsSupported`
probe, conditional WebView creation, native notice + runtime link (colours from that plugin's own CSS
tokens, wording adapted: "effect" vs "synth"), null-safe `resized()` / `timerCallback()` / every other
`webView->` use (grep them all — timers and `evaluateJavascript` calls crash on a null WebView).
Out of scope: the 29 plugins already using a named temp subfolder (later phase).

**B3 per-plugin verify:** `./scripts/build-and-install.sh <Name>` (background it — 600 s watchdog),
`auval` for the AU triple, pluginval strictness 10 on the VST3, `node scripts/check-i18n.js`, both lints.
Windows path cannot be exercised here — say so in SUMMARY; review the `#if JUCE_WINDOWS` code by reading.

---

## Version table

| Plugin | Fixes | Version |
|---|---|---|
| O-DigiDelay | T1.3 | 1.9.2 → 1.9.3 |
| O-Strata | T3.1 | 1.1.2 → 1.1.3 |
| O-IntonationPad | T3.2, T3.4 | 2.11.2 → 2.11.3 |
| O-Polystutter | T3.2, T3.4 | 1.16.2 → 1.16.3 |
| O-Comp | T3.3, T3.4 | 1.11.2 → 1.11.3 |
| O-Bass | T3.3, T3.4 | 1.8.2 → 1.8.3 |
| O-Bowed | T3.3 | 1.10.2 → 1.10.3 |
| O-Orbit | T3.3 | 1.4.4 → 1.4.5 |
| O-AnalogEQ | T3.4 | 1.5.6 → 1.5.7 (`set(OAEQ_VERSION …)` :10) |
| O-AnalogSaturation | T3.4 | 1.8.2 → 1.8.3 |
| O-Bells | T3.4 | 4.9.2 → 4.9.3 |
| O-Chorus | T3.4 | 1.10.2 → 1.10.3 |
| O-Detune | T3.4 | 1.10.1 → 1.10.2 |
| O-Lyrica | T3.4 | 2.6.2 → 2.6.3 |
| O-Marimba | T3.4 | 1.16.2 → 1.16.3 |
| O-MultiBandCompressor | T3.4 | 1.14.1 → 1.14.2 (`set(OMBC_VERSION …)` :9) |
| O-SpectralShaper | T3.4 | 1.10.1 → 1.10.2 |
| O-Tremolo | T3.4 | 1.11.1 → 1.11.2 |

18 plugins. Re-read each version at execute time — another session may bump first.

## Order / dependencies

1. B1 (T1.1 → T1.2 → T1.3) — unblocks a green CI signal for everything after.
2. B2 (independent; can run alongside B3).
3. B3 per plugin, any order; the multi-fix plugins (IntonationPad, Polystutter, Comp, Bass) get all
   their fixes in one bump.
4. Suite-wide after B3: `node scripts/boot-all-uis.js` (no new boot failures vs baseline — capture the
   baseline BEFORE B3), fresh-archive CI gate run.

## Deliverables for verify

- SUMMARY.md: per-plugin result (built / auval / pluginval / skipped-because-dirty), commit SHAs,
  Windows-unverified note, DAW hands-on list for Taylor:
  - IntonationPad: rename inline + delete armed-confirm (macOS Logic/Live)
  - Polystutter: delete armed-confirm
  - Strata + Prism open together: hover-help toggles independent
  - Comp/Bass meters, Orbit motion, Bowed visual state still animate (optionally with a comma-decimal
    system locale, e.g. French region)
  - O-Orbit pending v1.4.2 halo check
  - Windows (when available): one T3.4 plugin opens normally; with WebView2 runtime removed shows the notice
- B4 remains blocked; record the unblock condition.
