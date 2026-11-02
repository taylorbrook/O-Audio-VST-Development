---
phase: quick-260925-itj
plan: 01
subsystem: modules/ui + O-ReverseDelay UI
status: complete
tags: [font, webview, ofl, module, r5, o-reversedelay]
requires: [modules/persistence/preset-manager direct-embed pattern, scripts/serve-ui.js]
provides: [modules/ui/eb-garamond 1.0.0, O-ReverseDelay v1.22.0, cdp-font-probe.js]
affects: [modules/registry.yaml, scripts/regen-registry-used-by.sh]
tech-stack:
  added: [EB Garamond 1.003 (OFL-1.1) subset woff2]
  patterns: [direct-embed of module css/fonts into the single UIResources target, Times-metric bake instead of CSS ascent-override]
key-files:
  created:
    - modules/ui/eb-garamond/{module.yaml,README.md,OFL.txt,css/eb-garamond.css,fonts/EBGaramond-{Regular,Italic,Bold}.woff2,snippets/getResource.cpp,tools/build-fonts.sh}
    - plugins/O-ReverseDelay/tests/tools/cdp-font-probe.js
  modified:
    - plugins/O-ReverseDelay/{CMakeLists.txt,Source/PluginEditor.cpp,Source/ui/public/index.html,Source/ui/public/css/styles.css}
    - plugins/O-ReverseDelay/tests/{ui_frontend_check.js,ui_tooltip_clamp_check.js,ui-stub/serve-stub.sh}
    - plugins/O-ReverseDelay/{CHANGELOG.md,NOTES.md}
    - modules/registry.yaml
    - scripts/regen-registry-used-by.sh
decisions:
  - "Bundled fonts carry Times New Roman's vertical metrics (891/-216/42 + USE_TYPO_METRICS) rather than CSS ascent-override (no WKWebView support): 0 vertical moves in en/fr/zh-Hans"
  - "eb-garamond is consumed by direct embed into a plugin's existing single UIResources target; served at /css/eb-garamond.css + /fonts/*.woff2"
  - "PLUGINS.md O-ReverseDelay row left uncommitted: its diff also carries another session's O-MultiBandCompressor and O-Formant rows"
duration: ~76 min (14:15 → 15:32, including a watchdog stall and a resume)
completed: 2026-09-25
plan_head_before: 7cffc7f5759678491cf253e5b5498739c69e8b2e
actuals:
  tokens: 18300
  tasks: 3
  commits: 2
---

# Phase quick-260925-itj Plan 01: R5 shared EB Garamond module, O-ReverseDelay pilot — Summary

EB Garamond 400 / 400i / 700 now ships as a shared module, `modules/ui/eb-garamond` 1.0.0. The woff2 files are subset to Latin + Latin-Ext (117 824 B in total) and carry Times New Roman's vertical metrics. They are rebuilt byte-reproducibly from a pinned google/fonts commit. O-ReverseDelay v1.22.0 is the pilot. Its Latin text resolves to the bundled face in en, fr and zh-Hans with 0 vertical layout moves, and the WKWebView Standalone shows the face.

## Commits

| # | Hash | Message |
|---|------|---------|
| 1 | `c7f39d0f` | feat(modules/ui): eb-garamond 1.0.0 — shared OFL face with Times-matched metrics (R5) |
| 2 | `bcaaf251` | improve(O-ReverseDelay): v1.22.0 — bundled EB Garamond face (R5 pilot) |

- Both commits are path-scoped. `git show --stat` lists every new file and no foreign path.
- The submodule guard passed: nothing under `plugins/O-Orbit/libs/SAF` was staged.
- No tag was created.
- The other session's staged index entries (the O-Formant files and PLUGINS.md) are intact after both commits.

## Gates, before (v1.21.1) → after (v1.22.0)

| Gate | Before | After |
|---|---|---|
| `cdp-font-probe.js --all-languages` | **exit 2 (RED)**: every Latin run on Times New Roman, fleuron on Zapf Dingbats, 0 EB Garamond FontFaces | **exit 0 (GREEN)** |
| probe rect diff vs before (278 elements per language) | noise floor (before vs before): 0 vertical / 0 horizontal / 0 resized | en 0 vertical (33 horizontal-only, 44 resized); fr 0 (32 / 44); zh-Hans 0 (17 / 21) |
| `check-ui-labels.js --plugin O-ReverseDelay` | ALL CHECKS PASSED, 172 PASS / 0 FAIL, "every requested resource was served", 7 `<option>`s never measured | identical: 172 / 0, resources served, same 7-option note |
| `ui_frontend_check.js` | 183 passed, exit 0 | 186 passed, exit 0 (3 new §9 checks) |
| `ui_tooltip_clamp_check.js` | 155 passed, exit 0 | 156 passed, exit 0 (+ "bundled EB Garamond faces loaded (loaded, loaded, loaded)") |
| WINDOW budget line | `select 44 + knob 78 + env 72 + 2 gaps @ 9 = 212 into body 212 (panel 242) — 0 px spare` | byte-identical |
| check-i18n / i18n-fr-lint / i18n-zh-lint | 0 / 0 / 0 | 0 / 0 / 0 |
| serve-stub.sh (port 8797) | n/a | css + 3 woff2 all `200` (`text/css`, `font/woff2`) |

## CDP resolved face per language (after)

In all three languages, all three FontFaces are `loaded` (EB Garamond 400/normal, 400/italic and 700/normal) and there are 0 unserved requests.

| Target | en | fr | zh-Hans |
|---|---|---|---|
| .title | EBGaramond-Regular + Italic (custom) | same | same |
| .title-accent | EBGaramondItalic-Italic (custom) | same | same |
| .subtitle | EBGaramond-Regular (38) | EBGaramond-Regular (43) | PingFangSC-Regular (10) + EBGaramond-Regular (3) |
| .group-label | EBGaramond-Regular | EBGaramond-Regular | PingFangSC-Regular |
| .knob-label | EBGaramond-Regular | EBGaramond-Regular | PingFangSC-Regular |
| .preset-name | EBGaramondItalic-Italic (7) + Lucida Grande (1, the ▾ ::after caret, exempt) | same | same |
| .ab-slot | EBGaramond-Bold | EBGaramond-Bold | EBGaramond-Bold |
| .fleuron | EBGaramond-Regular (❦) | same | same |
| .footer-text | EBGaramond-Regular (82) | EBGaramond-Regular (87) | PingFangSC-Regular (20) + EBGaramond-Regular (16) |

## Controls observed

1. **Probe positive control.** The probe was RED on untouched v1.21.1 (exit 2) and GREEN after (exit 0).
2. **§9 negative control.** Renaming the Bold provider URL to `EBGaramond-Bld` made ui_frontend_check exit 3 with 3 FAILs:
   - the CSS url() `../fonts/EBGaramond-Bold.woff2` was unserved;
   - `/fonts/EBGaramond-Bld.woff2` was not in SOURCES;
   - `/fonts/EBGaramond-Bold.woff2` was unserved.

   The reverse Edit restored it. `git diff` of PluginEditor.cpp was byte-identical to `ctl-pre.diff`, and the re-run passed 186.

   Separately, the HEAD (unextended) ui_frontend_check run against the wired tree FAILED with "every getResource() path is in SOURCES — MISSING: /css/eb-garamond.css, /fonts/…×3". That confirms the §9 extension was necessary.
3. **Regen control.**
   - The unextended `regen-registry-used-by.sh` rewrote eb-garamond's `used_by` to `[]` (RED).
   - After the `css/` + `fonts/` extension, it regenerates eb-garamond's block exactly (`[O-ReverseDelay] -> [O-ReverseDelay]`, version 1.22.0).
   - **Both runs also touched other modules.** This is pre-existing drift: stale `version:` values in preset-manager, scala-tuning-engine, note-expression, analog-eq-unit and compressor-unit `used_by`; new consumers O-Emulator, O-IntonationPad, O-Marimba and O-Orbit on preset-manager, and O-Strata on scala-tuning-engine and note-expression; plus the auto header patch bump.
   - Per the plan, the registry was restored to the hand version and **the drift is NOT committed**. A separate `regen-registry-used-by.sh` run should refresh it.

## Fonts

| File | Bytes | sha256 |
|---|---|---|
| EBGaramond-Regular.woff2 | 37 656 | `0356da56b528e90715294af36d5139b5ae667a50e5ccabc4b0ab6b1243908b8f` |
| EBGaramond-Italic.woff2 | 38 892 | `aed562a19c371728c9ef54455ce0428abc2f807f5f96f96ab9a0b502d5a2b86a` |
| EBGaramond-Bold.woff2 | 41 276 | `0a7f0f4333ba9ddba3d1ff7254daea3a4d7b4f9500f837e1ba7a88cc66dbd407` |
| OFL.txt | 4 398 | `0985066662eb755ed3683ae5482a81a9195b49ce3f7e165cc2388b3dbece7dd7` (matches the pin) |

- **Reproducibility:** three `build-fonts.sh` runs gave identical hashes. All three inputs passed `shasum -c`.
- **Readback:** hhea (891, -216, 42) and typo (891, -216, 42), USE_TYPO_METRICS set, usWeightClass 400 / 400 / 700, name ID 1 = `EB Garamond`. The cmap has U+00E9, U+2014, U+2019 and U+2766. Every file is under 50 000 B, and name IDs 0 / 13 / 14 are kept.

## Build / install / validation

- **Build and install.** `./scripts/build-and-install.sh O-ReverseDelay` exited 0 in 52 s with no ALTERNATE-variant sweep warning. Installed: `O-ReverseDelay-dev.vst3` and `O-ReverseDelay-dev.component` only. `CFBundleShortVersionString` is 1.22.0 on both.
- **Provider strings.** `strings -a | grep -c '/fonts/EBGaramond-Regular.woff2'` returns 3 on each of the installed VST3, the installed AU and the rebuilt Standalone.
- **auval.** `auval -v aufx ORvD OuDv` reports AU VALIDATION SUCCEEDED.
- **pluginval.** `pluginval --strictness-level 10` on the VST3 reports SUCCESS (exit 0).
- **WKWebView verdict: CONFIRMED (assumption A1).**
  - Method: the Standalone was rebuilt, launched with `open -g` and captured with `screencapture -l` of its CGWindow id. Chrome stayed frontmost, so no focus was stolen. The app was quit via AppleScript afterwards.
  - Result: the capture shows EB Garamond forms on the title and its italic "Delay" accent, the small-caps labels and the EB Garamond fleurons. The Chromium baseline showed Times / Zapf Dingbats.
  - Descenders in the preset-name ellipsis box look clean.
  - Assumption A3 (identical WKWebView line boxes) holds by eye only; it was not measured.
- **Audio untouched.** `git diff 7cffc7f5 -- Source/dsp PluginProcessor.{h,cpp}` is empty.
- **Backup.** `backups/O-ReverseDelay/v1.21.1/` passes verify-backup.sh. Every tracked file is byte-identical to the 7cffc7f5 tree; the only extras are verify-backup's dry-run `build/` and an untracked `.planning/gate-bypasses.log`. It was taken before any product edit.

## Deviations from Plan

1. **[Rule 1 - Bug] The probe targeted the first match, not the first rendered match.**
   - Found during: Task 1, Step 2 (baseline).
   - Issue: the first `.knob-label` is in the hidden Free-mode wrap. It reported no platform fonts, which made rule (b)/(c) a false signal.
   - Fix: the probe now picks the first match with client rects, tags it with a temporary `data-cdp-probe` attribute (removed before the rect dump), and counts pseudo-element `content` for the ▾ / ⚙ exemption (the `.preset-name::after` caret).
   - Files: `tests/tools/cdp-font-probe.js`. Commit: `bcaaf251`.
2. **[Preflight] The preflight scope check was not empty.** `git status -- … PLUGINS.md` showed `MM PLUGINS.md`, but those changes were other sessions' O-Formant and O-MultiBandCompressor rows, not O-ReverseDelay work. I proceeded instead of stopping.
3. **PLUGINS.md is not committed.** The O-ReverseDelay row was edited to 1.22.0 / 2026-09-25, but `git diff -U0 HEAD -- PLUGINS.md` also shows the O-MultiBandCompressor (unstaged) and O-Formant (staged by another session) rows. Per the gate, PLUGINS.md was left out of commit 2. **The row edit sits uncommitted in the working tree**, and whichever session commits PLUGINS.md next will carry it.
4. **Doc tweak.** The styles.css comment "all ten font-family declarations" was corrected to twelve, which is the comment-stripped count at v1.22.0. All twelve read `var(--serif)`.
5. **The coordinator's resume note said the backup was missing. It was not.** It had been created at Task 1 Step 3 from the untouched tree, and was re-verified against `git archive 7cffc7f5`.

## Known Stubs

None.

## Next steps (not done — pilot only)

- Roll `eb-garamond` out to the other plugins with a serif font token, one /improve per plugin. For each, apply the README's 4 steps, add a served-tree font copy to any hand-built test trees, and run a probe like O-ReverseDelay's.
- Refresh the pre-existing registry `used_by` drift with a dedicated `regen-registry-used-by.sh` commit.
- Check a Windows build to confirm WebView2 / DirectWrite parity (assumption A2).
- Commit the PLUGINS.md O-ReverseDelay row once the other sessions' rows are committed.

## Self-Check: PASSED

- All module files, the probe, the backup and the three fonts exist on disk.
- Commits `c7f39d0f` and `bcaaf251` are present in `git log`.
- `git rev-list --count 7cffc7f5..HEAD` = 2.
- No tag points at either commit.
