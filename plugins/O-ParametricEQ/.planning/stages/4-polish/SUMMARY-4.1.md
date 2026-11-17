---
title: "O-ParametricEQ Stage 4 (Polish) — Phase 4.1 Summary (build: factory bank, host text, page fixes, i18n, G10 mode, Windows prep)"
date: 2026-11-16
phase: "4.1"
plan: PLAN-4.1.md
version: 0.10.0
status: executed
---

# O-ParametricEQ — Phase 4.1 Summary

Everything Stage 4 ships is now in source, gated, built and installed. 4.2 only has to validate it.

- **Factory bank (FUNC-07):**
  - 13 presets, written in engineering units and converted once each through the live parameter.
  - A bank hash refreshes `Factory/` on any re-voice or rename at an unchanged version, and leaves no orphan.
  - `User/` is never touched.
- **Host text:** every float shows in its unit (`1.20 kHz`, `+3.0 dB`, `0.71`, `100 %`). Typed text parses back; text with no number gives the parameter's default.
- **Page fixes:**
  - Delete ends listen at once.
  - The point tip keeps 11.5 px through its fade.
  - Two fixes found during execute, both decided by Taylor: the header at 800 px, and the meter hold under jitter (see Deviations).
- **Localisation:**
  - 18 EQ entries were added to both suite glossaries and to the French prose companion.
  - Every fr and zh row was revised against them. fr stays `reviewed: false`, zh stays `'mt'`.
- **G10:** the gated `--g10` mode passes every row, after one gate amendment (G10c, Taylor's call).
- **Windows prep:** the WebView2 static gate passes and its 7 controls fire. `ci-tests.yml` can dispatch O-ParametricEQ.

The audio path is frozen. The 16 sessions at 0.10.0 are SHA-256-equal to the 0.5.0 hashes, and probe N1e reads 16 / 16 with hash `6274ff9b5b0e488a`.

## Results at a glance

| Check | Result |
|---|---|
| Gate 3-gui → 4-polish | Critical checks PASSED (build, pluginval, DSP / UI critics). Schema SKIPPED (no `HANDOFF.json`, as at earlier transitions; rc 1 from that only) |
| W0 baseline (0.9.0, HEAD `4acfc9c1`) | designer 150 / 0 / 18; render 171 / 0 with `--lid-dir`; sessions 16 / 16 equal to `verify-3.3/sessions-090.sha`; `--dump-params` = golden; probe N1e 16 / 16, `6274ff9b5b0e488a` (`execute-4.1/baseline/`) |
| Designer harness | **150 PASS / 0 FAIL / 18 INFO** |
| Render harness, default with `--lid-dir` | **183 PASS / 0 FAIL** (171 + FP1, FP1o, FP2, FP3, FP4, FP5, FP5k, FP5r, FP5i, HT1, HT1g, M1j) on the final tree (`harness/render-final.log`) |
| Audio frozen | sessions at 0.10.0 SHA-equal to 0.5.0, 16 / 16 (`harness/sessions-0100.sha`); probe N1e 16 / 16, hash `6274ff9b5b0e488a` |
| G9 (b) `tests/g9b/run.sh` | PASS |
| `tests/meter-ballistics/run.sh` | PASS |
| `--g10` (4.1 reading; 4.2 is binding) | 5 / 0 (load 3.6). G10a 3.11 %; G10b 5.63 %; G10c 0.0027 % (pass-through 0.0004 %, passes on the amended floor); G10d net 3.11 ≤ 3.28 %; G10i 28.1 % INFO (under the 35 % line); G10d0 self-check PASS |
| PN1 / PN1n `tests/preset-name-fit/` | PASS en / fr / zh-Hans at 800 × 500: widest "Gentle Master Tilt" 109 / 135 px; the 40-character control overflows |
| WebView2 static gate `tests/windows-webview2-gate.sh` | PASS; 7 / 7 negative controls fire (one per check) |
| UI gates | `check-i18n` 0; `i18n-fr-lint` 0 / 49 plugins; `i18n-zh-lint` 0 / 49; `check-ui-labels --plugin O-ParametricEQ` 0; `boot-all-uis --strict-tips` 0 DEAD |
| Independent glossary scan | 20 rows carry a new key; 0 mismatches (`gates/independent-scan.log`). The first lint run fired G1 / Z5 on the old drafts, so the lint is live on these keys |
| Install | `build-and-install.sh` rc 0; `O-ParametricEQ-dev.vst3` / `.component` at 0.10.0 |
| pluginval 10 VST3 / AU, `auval -v aufx OuPQ OuDv` | SUCCESS / SUCCESS / AU VALIDATION SUCCEEDED |
| Real `~/Library/O-ParametricEQ/Presets/` | `Factory/`: 13 JSON + `.factory-version` (0.10.0) + `.bank-hash`. `User/`: absent before and after |
| Editor bench (Release, one process; Debug `--live`) | 6 new rows PASS; the 3.3 rows ALL-PASS as regression; Debug 0 assertions, 0 leak lines; screen unlocked; real `Factory/` unchanged by the bench |
| Mutants | **13 / 13 scored caught**, controls pass. Harness M4, M6 – M14 (`mutants/mutants41-results.json`); page M1 – M3 (`mutants/mutants41-page-results.json`). M5 is equivalent (below) |
| Build warnings in `Source/` | none new. Three in the harness are pre-existing at HEAD (`owedSame`, the `b1` / `b2` captures) |

## New rows

| Row | Reading |
|---|---|
| FP1 | 13 presets loaded from random state; 152 named values read back (worst freq / Q rel 5.3e-7, gain 1.7e-6 dB); 2535 / 2535 checks, every other parameter at default |
| FP1o | output_gain +5 dB and bypass on held through all 13 loads; 13 JSON files scanned, no forbidden key |
| FP2 | (a) – (e) all ok, and the shipping bank restored after |
| FP3 | savePreset ("Vocal Presence") refused, file byte-identical; 3 refusals, 2 allowed |
| FP4 | "abc" → default (0.5), 1.7 → 1, −3 → 0; 196 gesture pairs, 0 unpaired |
| FP5 / FP5k | 260 / 260 finite (520 renders); load transient κ worst 1.12× (Telephone 44.1 k mono random) against 2.5× |
| FP5r | 130 / 130 within 0.05 dB of the closed form; worst 0.0011 dB (Rumble Cut 96 k stereo); 59 824 points |
| FP5i | No auto gain at the clamp. Telephone (auto gain on): +6.7 dB. Mono Low End: −0.71 dB mono vs −0.40 dB stereo |
| HT1 / HT1g | 100 / 100 round trips, 19 / 19 parses, 11 / 11 formats; golden byte-identical (17 634 bytes) |
| M1j | 1024 / 44.1 k H 3: 0 empty in 592 ticks; 2048 / 44.1 k H 5: 0 empty. The H − 1 control fires at both |
| E-del-listen | `setListenBand(0)` 0 – 1 ms after Delete (limit 20 ms); M1 reads 80 ms |
| E-listen33-aria | 30 frames, 0 read "true"; M3 shows 17 / 30 |
| E-tipfade | 11.5 px at every frame of the fade; class gone after; re-show mid-fade keeps it; M2 reads 12 px |
| E-tip-lang | width re-measures (en 120 → fr 170.5, zh 141.1 px), inside the viewport |
| E-tip-v13 | hover tip carries no `bands-full` binding |
| E-preset-listen | Kick Tighten: listen holds at 400 ms and 1 s, through b03's enabled toggling twice. De-Mud: listen ends |

## Deviations

1. **The 800 px header (Taylor's call, recommended option).**
   - PN1 failed every name at 800 × 500. The preset bar sat at its 150 px min-width, so the name box was 43 px and even "Default" (54 px) was ellipsised. This had been true since 3.1.
   - Below 960 px the header gap is now 8 px and the "Analyzer" caption is hidden. The segment keeps its aria-label and tooltip.
   - The name box is now 135 px. Nothing changes at 960 px and up.
2. **Meter hold widened (Taylor's call, recommended option).**
   - M1j failed at 2048 / 44.1 kHz with H = ceil(B/fs·60)+1: 4 empty reads in 587 ticks. With ±30 % jitter, a gap reaches 1.6 block periods.
   - `PeakHold::depthFor` is now ceil(1.3·B/fs·60)+1: 1024 stays at 3, and 2048 goes from 4 to 5. This is editor-only.
3. **G10c amended (Taylor's call, recommended option).**
   - The first reading: 0 bands cost 0.0026 % against a 0.0004 % pass-through, 6.3×. That ratio is against a near-zero baseline.
   - G10c now also passes when the cost is ≤ 0.01 % of a core above the pass-through. The 0.3 % budget is unchanged.
   - ARCHITECTURE's G10 rows and amendment log are updated, and the STATUS checksum is refreshed.
4. **zh Pre segment pinned.** zh 处理前 (36.8 px) overflowed the 35 px Pre segment. Pre is now 39 px, the same as Post (zh 处理后), per the plan's width-pin rule. The caption was not shrunk.
5. **Glossary 'pre' / 'post' carry two senses.** The new zh entry flagged O-GranulatorInstrument's reverb 前置 / 后置, a position in the feedback loop, which is correct there. The glossary accepts both renderings, with a comment, as `mid` already does. The suite stays at 0 findings and no other plugin was edited.
6. **"L/R Pair Match" renamed "L-R Pair Match".** The preset manager rewrites "/" to "_" in the file stem, which is the name the bar shows.
7. **M5 is an equivalent mutant.** A choice parameter's range is 0 … n − 1 with interval 1, so index / (n − 1) is exactly `convertTo0to1`. It is recorded, not counted.
8. **Bench.**
   - The aria row is named `E-listen33-aria`, so it does not merge with 3.3's `E-listen33`.
   - The 3.3 `E-bind-in` check parsed host text as a bare number. With unit text now in place, it compares the page with the host's exact value instead.
   - The bench's fake-home check and the wipe of its `Library/` dir moved before the processor is built. Left after construction, the wipe would delete the bank the constructor just wrote.
   - Bench note, not user-visible: after the last band is deleted while latched, the hidden empty strip's listen button keeps `aria-pressed="true"` until a band is selected.
9. **FP5r scope.** Each preset's bands run through the G9 (a) closed-form check with auto gain off, as the G9 (a) draws do. FP5i reads auto gain separately as the measured on / off level ratio.
10. **Harness flags.** `--rows41` (the fast 4.1 rows, for the mutant runs) and `--g10`. `g10SelfCheckRow` runs in both.

## Other plugins: lint findings

None remain. The only cross-plugin finding (O-GranulatorInstrument `opt.reverb.pre` / `opt.reverb.post`) is resolved by deviation 5, with no edit to that plugin.

## For 4.2

- **Binding reading:** a fresh rebuild; the full battery, including `--long` and `--g10`; the bench.
- **C1 push:** ask Taylor again at the moment of the push, then `gh workflow run ci-tests.yml -f plugin=O-ParametricEQ`.
- **Hands-on checklist draft** (P6 plus 4.1 items):
  - [ ] Analyzer modes Off / Pre / Post / Pre+Post, in Logic and Reaper
  - [ ] Meter against Logic's at a 2048 buffer: steady, no dips (the H change)
  - [ ] Listen by ear: latch, drag, Delete ends it at once
  - [ ] Standalone with live input
  - [ ] Meter look at 800 × 500 and 1600 × 1000; at 800, the header with no Analyzer caption and the preset names in full
  - [ ] Every factory preset, in the listening pass (re-voice as needed; the bank hash carries it with no version bump, but a reinstall is needed)
  - [ ] Mono Low End on a mono track: the whole signal is cut below 120 Hz (C2)
  - [ ] Host text in Logic (AU) and Reaper (VST3): `1.20 kHz`, `+3.0 dB`; typing `1k` and `-3 dB`
  - [ ] French spot read in the Standalone, including "signal central" and the preset bar; then flip fr to `reviewed: true`
  - [ ] zh at 800 px: 处理前 / 处理后 in the analyzer segment

## Files

- **Source:**
  - `Source/FactoryPresets.h` and `Source/ParamText.h` (new)
  - `Source/PluginProcessor.{h,cpp}`: `initFactoryBank`, float attributes
  - `Source/editor/PeakHold.h`
  - `Source/ui/public/index.html`: deleteBand, point-tip fade, 800 px header, Pre width
  - `Source/ui/public/js/i18n.js`
- **Build and docs:** `CMakeLists.txt` (0.10.0, `OPEQ_TEST_HOOKS` on the harness), `CHANGELOG.md`
- **Tests:**
  - `tests/render-harness/main.cpp`: FP / HT / M1j rows, `--g10`, `--rows41`
  - `tests/preset-name-fit/preset_name_fit.js` (new)
  - `tests/windows-webview2-gate.sh` (new)
- **Planning:** `.planning/research/ARCHITECTURE.md` (G10 amendment)
- **Outside the plugin:**
  - `.github/workflows/ci-tests.yml`: one option line
  - `scripts/i18n-fr-glossary.js`, `scripts/i18n-zh-glossary.js`
  - `.planning/quick/archive/2026-Q3/260826-ieq-…/260826-ieq-FR-GLOSSARY.md`
- **Evidence:** `stages/4-polish/execute-4.1/`
  - `baseline/`, `harness/`, `gates/`, `validators/`, `mutants/`, `editor-bench/`
  - Session dumps and probe builds are gitignored; their SHA lists are kept.
