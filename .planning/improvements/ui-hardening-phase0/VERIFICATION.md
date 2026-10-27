# Verification: Cross-plugin UI hardening, Phase 0

**Plugin:** cross-plugin (18 plugins + CI + generator templates)
**Milestone:** ui-hardening-phase0
**Created:** 2026-10-26
**Phase:** Verify (scope B1–B3; B4 deferred, blocked)
**Verified at HEAD:** `25b50e0b` (the milestone's last commit is `ce7f870e`; `25b50e0b` is the other session's O-Polystutter v1.17.0)

---

## Verification Summary

**Status:** PASSED WITH NOTES (overall verdict: **ACHIEVED WITH KNOWN ISSUES**, see the end)
**Version:** 18 PATCH bumps (table in SUMMARY.md). At HEAD all 18 agree across CMakeLists / CHANGELOG `## [X.Y.Z]` / PLUGINS.md. O-Polystutter is at 1.17.0, not 1.16.3: see G1.

Method: I checked each goal against the code, working back from the goal, with HEAD blobs (`git show HEAD:…`) and a fresh `git archive HEAD` in the scratchpad. I edited, staged and installed nothing. The only file written in the repo is this one.

---

## Requirements Verification

| # | Requirement (CONTEXT.md) | Status | Evidence |
|---|---|---|---|
| B1.a | `sync-ui-modules` step recreates the configure-time module copies | ✓ PASS | Fresh `git archive HEAD` → `node scripts/sync-ui-modules.js`: "14 written, 4 already current", exit 0 |
| B1.b | CI runs the sync step **before** check-i18n | ✓ PASS | `.github/workflows/ui-static-gates.yml`: step `sync configure-time module copies` comes right after setup-node and before `check-i18n` |
| B1.c | `--check` mode works | ✓ PASS | On the fresh archive before sync: exit 1, "14 drifted, 4 already current", one `DRIFT <path>` line per copy. After sync: exit 0 |
| B1.d | Acceptance: check-i18n + fr-lint (+ zh-lint) green on a fresh archive | ✓ PASS | After sync: check-i18n exit 0 ("ALL CHECKS PASS — 46 localized plugin(s)"), i18n-fr-lint exit 0, i18n-zh-lint exit 0, check-ui-canon (report-only) exit 0 |
| B1.e | Negative control: without sync, the baseline failure comes back | ✓ PASS | Second fresh archive, no sync: check-i18n **exit 11**. The same 11 `[12]` FAILs as the plan baseline (O-Bitrot, O-Emulator, O-MultiBandCompressor, O-Tapestop, O-simpleAmbisonics, O-simpleFM, O-simpleGrain, O-simplePhysicalModelSynth, O-simpleReverb, O-simpleSampler, O-simpleWavetable) |
| B1.f | O-DigiDelay `js.savePresetPrompt` fr finding fixed | ✓ PASS | At HEAD fr-lint is green. Negative control: putting back `da14304c^`'s `i18n.js` → fr-lint `T4 title js.savePresetPrompt "Nom de ce préréglage :"`, exit 2 |
| B2.a | Template `withBackend(webview2)` | ✓ PASS | `PluginEditor-webview.cpp:86`, `gui-agent.md:651,1174`. Every `.claude/agents` / `.claude/skills` file that builds `WebBrowserComponent::Options` has `withBackend`, except a historical snippet in `troubleshoot-agent.md:576` (G4, low) |
| B2.b | App-data user folder, created first; no bare `tempDirectory` | ✓ PASS | Template :79–89 (`userApplicationDataDirectory/…`, `createDirectory()` at :83 before options). `grep tempDirectory .claude` (excluding agent-memory) → 1 hit, prose `gui-agent.md:878` "(no `tempDirectory`)" |
| B2.c | `areOptionsSupported()` probe + notice; null-safe uses | ✓ PASS | Template :97 probe, :146/:224/:249 guards, :164–165 "guard EVERY webView-> use" note. `gui-agent.md:661,710,879` |
| B2.d | `withKeepPageLoadedWhenBrowserIsHidden` REQUIRED, not OPTIONAL | ✓ PASS | Template :43,:55–56 "REQUIRED"; `gui-agent.md:631,707`. grep for "optional"/"remove if" near the option → 0 hits |
| B2.e | CMake snippet `NEEDS_WEBVIEW2` + static-link define | ✓ PASS | `CMakeLists-webview-snippet.cmake:19` `NEEDS_WEBVIEW2 TRUE`, `:49` `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` |
| B2.f | `Juce.getToggleButtonState` → real ToggleState API | ⚠ PARTIAL | Template `index-template.html:340–360` uses `getToggleState` / `getValue()` / `setValue(!…)` / `valueChangedEvent.addListener(paint)` with no argument. This matches JUCE `~/JUCE/modules/juce_gui_extra/native/javascript/index.js:331,336,350` (`callListeners()` takes no payload). **But** the binding validator hook still parses only the dead API (G3) |
| T3.1 | O-Strata no `oprism.` storage key | ✓ PASS | `git grep -i oprism HEAD -- plugins/O-Strata/Source/ui` → one comment only (`index.html:2740`). Live keys `ostrata.tipsEnabled` at :2742, :2748. Suite-wide literal `localStorage` key census: 0 keys shared by two plugins |
| T3.2 | IntonationPad / Polystutter: no `prompt`/`confirm`/`alert`; i18n keys exist | ✓ PASS | Served JS (from each CMake `juce_add_binary_data`): 0 live calls, comments only. Polystutter's served `modules/preset-manager.js:336` `confirm()` lives in `promptDelete()`. It is wired only through `options.deleteButton` (:130–131), and `app.js:470–485` never passes `deleteButton`, so it is unreachable. Keys exist: IntonationPad `dialog.renamePrompt` (i18n.js:1004), `dialog.deleteConfirm` with `{name}` (:1011), `aria.rename`/`aria.delete` (:1265–1266). Polystutter `msg-delete-preset` with `{name}` (i18n.js:623), `aria.deletePreset` (:791). Natives `renamePreset`/`deletePreset` are registered (IntonationPad `PluginEditor.cpp:649,660`). `--danger`/`--danger-rgb` tokens exist |
| T3.3 | Bass/Comp/Bowed/Orbit: no `%f`/`%g` `String::formatted` payloads; non-finite handled | ✓ PASS | Whole-plugin sweep (`formatted\|to_string\|snprintf\|sprintf\|ostringstream\|setprecision` over `plugins/<P>/Source`): 0 live hits in all four. The only `%f` mentions are comments: Bass `PluginEditor.cpp:344`, Comp :363, Bowed :160, Orbit :740. Each payload goes through `num()` with an `std::isfinite` fallback: Bass :349–352 (−60 / 0; page `MIN_DB = -60`, index.html:1478), Comp :368–372 (−60/−60/0/−60), Bowed :165–172 (0), Orbit :746–753 (0, dist 1). Same arg count and keys as before. Bowed's module deps (note-expression, bow-friction) contain no `%f` formatted calls either. Suite-wide, the only `%f` formatted left is O-Gain, which is B4 |
| T3.4 | 14 plugins: app-data folder created before options; `withBackend`; probe + notice; null-guarded `webView->`; CMake flags | ✓ PASS | Census at HEAD, all 14: `userApplicationDataDirectory` ✓, `tempDirectory` 0, `Backend::webview2` 1, `areOptionsSupported` 1, `webView2RuntimeLink` in .h and .cpp, CMake `NEEDS_WEBVIEW2 TRUE` 1 + `…STATIC_LINKING=1` 1. Order: folder → `createDirectory` → options → probe → `make_unique` (IntonationPad through `buildWebViewOptions()` :143–155 → probe :50. Polystutter in the member-init lambda :231–243 → probe :586 → `return nullptr`). I read all 72 `webView->` uses: each sits behind `if (webView != nullptr)`, an early `if (webView == nullptr) return;` / `if (!webView) return;`, `if (webView)`, or inside a native-function / `launchAsync` callback (Marimba :707, :883 — these run only when a WebView exists). MBC timer → four `send*()` each open with `if (!webView \|\| !hasNavigated) return;` (:677,:713,:741,:768) |
| T3.x | Versions consistent, no tags | ✓ PASS (HEAD) / ⚠ see G1 | All 18 consistent at HEAD. `git tag --points-at` is empty for all 18 SHAs plus `670b2ebf`, `2a44ad87`, `25b50e0b` |
| T3.y | Diffs touch only editor / UI / CMake version / CHANGELOG (+ the T3.3 builders) | ⚠ PARTIAL | 17 of 18 clean: each commit touches only `PluginEditor.{cpp,h}` and/or UI files, CMakeLists (VERSION line only), CHANGELOG, and the plugin's own PLUGINS.md row. **O-Polystutter `ce7f870e` FAILS this** (G1) |

---

## Build Verification

Built in `build/` with ninja: `O-Comp_VST3/AU` (T3.3 + T3.4), `OuariconChorus_VST3/AU` (T3.4), `OPolystutter_VST3/AU` (T3.2 + T3.4). Ninja reported "no work to do", so the artefacts already matched the sources at HEAD: Comp 1.11.3, Chorus 1.10.3, Polystutter 1.17.0 (Info.plist). **Result:** success, 0 warnings. Nothing was installed.

## Pluginval Results (build artefacts, not installed bundles)

| Plugin | Strictness | Random seed | Result |
|---|---|---|---|
| O-Comp 1.11.3 | 10 (GUI tests on) | 0x282c8f | SUCCESS |
| O-Chorus 1.10.3 | 10 | 0x7e976a5 | SUCCESS |
| O-Polystutter 1.17.0 | 10 | 0x43bd2d1 | SUCCESS |

Logs: `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/833998d3-43c3-4512-9f60-48104a7ce599/scratchpad/logs/pv-*.log`

---

## O-Lyrica open item — **PRE-EXISTING CONFIRMED → RESOLVED in v2.6.6 (`11359791`)**

- Pre-change binary: there is no binary in `backups/O-Lyrica/v2.6.2/` (source only), so I built `38855299^` out of tree from `git archive` (CMakeLists, modules, vendored, plugins/O-Lyrica) in scratch → 2.6.2. Then I swapped in HEAD's `plugins/O-Lyrica` in the same build dir and rebuilt → 2.6.3. Same JUCE, same modules. The `git diff 38855299^ HEAD` outside O-Lyrica is empty.
- **SUMMARY's "seed 2/3/4" did not pin anything.** pluginval 1.0.4 has no `--seed` flag; it ignores it and draws a fresh random seed every run. The pinning flag is `--random-seed`.
- Fixed-seed A/B, strictness 10, using the seeds that failed:

| `--random-seed` | 2.6.2 (pre) | 2.6.3 (HEAD) |
|---|---|---|
| 0x69c6be7 | FAIL — Free Glissando not restored, actual 0.453177 | FAIL — identical |
| 0x14cb29d | FAIL — Free Glissando, 0.116289 | FAIL — identical |
| 0x2ed10e1 | FAIL — Scale-Locked Glissando, 0.1132 | FAIL — identical |

Both binaries fail the same test with the same value to six digits on every seed. The defect predates the change. ~~It still needs its own `/improve O-Lyrica`: the mutually exclusive META toggles don't round-trip a random state.~~

**Resolved 2026-10-27, O-Lyrica v2.6.6.** The diagnosis above was wrong about where the value is lost. The plugin's state round-trips. The host's cached value goes stale: a host-originated toggle change calls `parameterChanged()` inside the VST3 wrapper's `inParameterChangedCallback` scope, and the nested `setValueNotifyingHost` on the partner is never reported to the host. The exclusion now lives in the page click handler. All three seeds above, plus 0x1, pass at strictness 10.

**Same class elsewhere (not fixed):** O-simplePhysicalModelSynth `parameterChanged()` → `applyMaterialMacro()` writes `damping`/`decay` synchronously when it runs on the message thread (`PluginProcessor.cpp:288-305`). A host-set `material` leaves the host's damping/decay stale. The listeners in O-Bowed, O-Contrabass, O-simpleGrain and O-simpleSampler defer through `triggerAsyncUpdate`, which runs outside the scope, so they are unaffected.

---

## Issues Found

### Gaps

- **G1 — O-Polystutter `ce7f870e` swept in another session's DSP rewrite (Medium, commit hygiene / requirement "no DSP change").**
  - What the commit contains beyond the UI work:
    - `Source/DSP/{CounterRng.h, TriggerScheduler.*, RepeatLane.*, TapeDegrader.*}`, with `TriggerRouter.*` deleted
    - `PluginProcessor.cpp/.h` (+317/−)
    - `tests/scheduler-harness/*`
    - a CMake `target_sources` change, and CMake `VERSION 1.17.0`
  - At `ce7f870e` the version is inconsistent: CMake says 1.17.0, while CHANGELOG and the PLUGINS.md row say 1.16.3.
  - The `## [1.16.3]` CHANGELOG entry claims "No DSP, parameter, preset or state change". That is false for this commit.
  - `25b50e0b` (other session) admits this and repairs consistency at HEAD (1.17.0 everywhere). A shipped v1.16.3 binary never existed.
  - SUMMARY.md does not mention it.
  - Cause: the path-scoped commit `-- plugins/O-Polystutter` picked up unstaged working-tree files. The temp-index procedure guards against foreign *staging*, not against foreign *edits under your own path*.
  - Recommendation: correct the SUMMARY row (Polystutter UI fixes shipped as part of 1.17.0) and add a CHANGELOG footnote. Don't rewrite history.
  - Note: the other session is editing O-Polystutter again right now (DSP + `CMakeLists.txt` dirty).
- **G2 — O-Reed also has a bare `%TEMP%` WebView2 folder** (`plugins/O-Reed/Source/PluginEditor.cpp:100`). CONTEXT's B4 lists only its native-function gap, so add T3.4 to O-Reed's B4 scope.
- **G3 — `.claude/hooks/validators/validate-gui-bindings.py:42-43` still matches only `getToggleButtonState`.** Pages generated from the fixed template call `getToggleState`, so the SubagentStop / task validator no longer cross-checks toggle IDs against relays and attachments (it now misses toggles silently). This is outside the plan's file list (`.claude/agents`, `.claude/skills`). It is a one-line regex fix (`getToggle(?:Button)?State`).
- **G4 (low) — `.claude/agents/troubleshoot-agent.md:576`** has a historical `Options{}` snippet with no `withBackend`. It is a troubleshooting anecdote, not an emitted sample.

### Non-critical notes

- None of the 14 T3.4 plugins uses `withKeepPageLoadedWhenBrowserIsHidden()` (count 0). Fine for Phase 0, which did not scope it, but the template now calls it REQUIRED. Candidate for the P2 editor-shell work.
- IntonationPad in-place rename commits on `blur`. Per memory `pattern_value_entry_blur_commit_needs_capture_pointerdown`, a press on a dial that calls `preventDefault` on pointerdown won't blur the field, so it stays open until Enter / Esc / another click. No data loss. Check it during hands-on.
- **Scope leak sweep (bare temp WebView2 folders, repo-wide at HEAD):**
  - BARE: only O-Freeze and O-Reed, both B4.
  - Named `%TEMP%` subfolder: 29 plugins (the planned out-of-scope "later phase" set).
  - Missing `withBackend`: only O-simpleFM (B4).
  - **No leak outside the 18 + B4.**

---

## B4 — Deferred (blocked)

O-Reed, O-Bassoon, O-simpleFM, O-Gain and O-Freeze were not verified. They still have uncommitted work from another session (all five are dirty at verify time). Never touch O-GrainScatter.
**Unblock condition:** `git status --short plugins/<Name>` is empty. When it unblocks, add the T3.4 port to O-Reed (G2).

## Windows — unverified

There is no Windows host. All `#if JUCE_WINDOWS` code (folder creation, `withBackend`, the `areOptionsSupported` probe, the notice and link) was checked by reading only. Pluginval and builds ran on macOS, where that code is compiled out.

## DAW hands-on for Taylor (carried from SUMMARY)

- IntonationPad: preset browser ✎ rename in place (Enter / click away / Esc; also click a dial while the field is open) and ✕ two-click delete (Logic + Live, macOS)
- Polystutter: preset menu × two-click delete (now in 1.17.0)
- Strata + Prism open together: hover-help toggles are independent (Strata's resets to On once)
- Comp / Bass meters, Orbit motion display, Bowed visualization still animate, ideally once with the macOS Region set to France
- O-Orbit: the still-pending v1.4.2 halo check (now v1.4.5)
- Windows, when available: one T3.4 plugin opens normally; with the WebView2 runtime removed it shows the notice + link

---

## Verification Checklist

- [x] All B1–B3 requirements from CONTEXT.md verified (table above)
- [x] Build succeeds (3 representative plugins, 0 warnings)
- [x] Pluginval strictness 10 passes on the 3 build artefacts
- [x] O-Lyrica failure confirmed pre-existing (fixed-seed A/B)
- [ ] UI hands-on in DAW (Taylor)
- [ ] Windows path (no host)
- [ ] G1 documentation correction, G2 B4 scope addition, G3 validator regex

---

## Final Status

### ACHIEVED WITH KNOWN ISSUES ⚠

The B1–B3 goals hold in the code at HEAD:
- The CI gate is green on a fresh clone and red without the fix.
- The templates no longer emit the closed bugs.
- All 18 plugins carry their fixes with consistent versions and no tags.

Known issues: G1 (Polystutter commit swept another session's DSP rewrite; the docs misstate it), G2 (O-Reed missing from B4's T3.4 scope), G3 (binding validator blind to `getToggleState`), G4 (low). None of these is a functional regression in the 18 plugins.

---

*Generated by improve-milestone verify phase*
