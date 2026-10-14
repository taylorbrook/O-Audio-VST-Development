# Stage 3 — GUI — SUMMARY (Part 1: 3.1 + 3.2 · Part 2: 3.3)

**Plugin:** O-simpleAmbisonics · **Executor:** gui-agent · **Date:** 2026-10-13 · **Commits:** `5d9d351b` (3.1), `bc1645ef` (3.2) · visual checkpoint **approved 2026-10-13** (icon style `3d` kept) · Part 2 (3.3, T10–T13) below.

## Tasks and files

| Task | Files | Result |
|---|---|---|
| T1 | `Source/MeterFramePayload.h` | JUCE-free `ui::` quantise (non-finite → 0, ±1e6 clamp, `!live` zeroes peaks), FNV-1a hash incl. `live`, `to_chars` bounded format, `LiveTracker` (`kStaleTicksIdle` 3), `sampleFrame` (hasNewData before read) |
| T2 | `Source/PluginProcessor.{h,cpp}` | Guarded include + `createEditor`; `uiLanguage` (code string, `isVoid()` restore); `editorScale` atomic, written as `"0.850"` when > 0 and removed from the copy otherwise, restored only when finite and in 0.6–1.5; `applyFactoryPreset` (8 lessons, one pass, never `outputLevel`, skips within 1e-6, begin/set/end). No changes to prepareToPlay / processBlock / fillParams / createParameterLayout |
| T3 | `Source/PluginEditor.{h,cpp}`, CMake, `ui/public/{index.html,i18n.js,juce/*.js,insects.png}`, `.gitkeep` removed | Template + C1 fixes 1–4 + C7. 12 relays → webView → 12 attachments, 5 natives, `setSize (1180, 780)` last. Assets copied verbatim (md5 01ff05e3 / 3c4db27f / 6d824230) |
| T4 | Harness `main.cpp` (+ST1–6, PL1–4), `tests/ui-stub/generic-overrides.json`, `tests/pedalboard/verify_3_state.py` | Real ranges, choices and defaults (skew 0.2890648); own JUCE-base64 decoder |
| T5 | `tests/ui-fixtures/frames.json` | 15 fixtures, byte-identical across two runs |
| T6 | `scripts/generate_sh_icons.mjs` → `js/shicons.js` (233 705 B), `sh-icons-contact.html`; CMake + `/js/shicons.js` resource branch | SN3D matches an independent recursion to 4.70e-16. Letters W Y Z X V T R S U Q O M K L N P. All anchors > 0. Two runs identical (sha in `SH_ICONS_META`). 110–199 paths per icon, stroke 0.6. `--check` exits 0 |
| T7 | `ui/public/index.html` | U1 `effectiveFrame`/`paramFrame` + `#chanWell.idle` (0.8). C13 pole hold + half-alpha tick. C12 heading rose, fade, ▲▼ chevron. C14 dead zone = rHead, `180.0°`, `orderIdx()`. C15 bands + station counts from `orderActive`. C16 dirty flags, ease snap 1e-3, puck sprites, fonts/lang marks all. C11 `SH_ICON_STYLE='3d'`. 0 `'mousedown'` listeners, 2 anchors, no innerHTML outside comments |
| T8 | `tests/ui-probes/frame-probe.mjs` | 13/13. The fuma / pole / idle negative controls fail as designed |
| T9 | Build / install | See Numbers |

## Numbers

- **Build:** 0 warnings (the template's deprecated display APIs were replaced).
- **Install:** `build-and-install.sh` exited 0; Standalone rebuilt.
- **auval:** SUCCEEDED.
- **pluginval s10** (now opens the WebView editor): SUCCESS 2× VST3 + 2× AU at T5, 1× each at T9.
- **Harness:** 251/251 (241 + 10), run twice, identical except PERF-02 (210× real time).
  - Digests: 2.1 `9189d911a1fe98d5` PASS, 2.2 `7f35bc9f91e321b9` PASS, 2.3 `edd1acfd21f54bcd` reproduces.
  - ST1: zh-Hans → 2.
  - ST2: absent / "de" → 0.
  - ST3: 0.8500; a fresh instance writes no editorScale.
  - ST4: abc / 3.0 / 0.1 / 1.6 / nan / inf rejected and absent from the re-save.
  - ST5: 0 mismatches over 2 × 12; distance raw 1.00000011920929.
  - ST6: 84 changes over 8 lessons, gestures balanced, idempotent, unknown ids inert, outputLevel raw −11.999999 unchanged.
  - PL1: 390 B typical, 485 B worst case.
  - PL2: left Y 1000 / X 0; yaw90 azRot 9000; pole elRot 9000 / 9000 with azRot 3000 / 15000; stereo mix 0.
  - PL3: one idle emission, then silence; a publish revives.
  - PL4: 15 fixtures / 9 002 B, identical.
- **Pedalboard:**
  - `verify_2_2` 35/35, `verify_2_3` 42/42, `verify_3_state` S1–S3 pass.
  - `verify_2_1` 22/25: the 3 Binaural V3 checks have been stale since Phase 2.3. They expect a clean impulse from the 2.1 placeholder, and the real HRTF convolver smears it. Pre-existing; the file was not edited. **Needs a user decision.**
- **Tracer gate:**
  - Harness 241/0 on the unchanged harness.
  - Natives census 5 = 5. Boot clean, 0 404s.
  - Standalone OK after the occlusion nudge.
  - C++ → page proven via a patched session state (az 180 / order 3 / yaw 60).
  - Page → C++ in WKWebView can't be driven headless and is **untested** (checklist item 9).
- **frame-probe:**
  - A1: W / X / Y 0.997 / 0.997 / 0.
  - A2: behind, X −1.000.
  - A3: bands 2/3/4, inactive rows at 0.3 opacity, 16 icons.
  - A4: puck at (84.79, 166.71).
  - A5: puck unchanged, rim tick to the left, ▲ glyph IoU 0.88 (control 0.16).
  - A6: 1/1 gesture, 0 rear samples, final 90.00.
  - A7: 1/1, final 45.00.
  - A8: 109.5° → 68.5° → 49.5°.
  - A9: tick held at (−140.3, −0.25).
  - A10: 16.33 % of decoder pixels change.
  - A11: rotation station lights; labels 4/9/16.
  - A12: param-drawn W = Y = 0.997; idle opacity 0.8 → 1.0.
  - A13: peaks fall to 0 in 309 ms; the puck follows params while idle and not while live; a silent frame is not dimmed.

## Deviations

- **D1:** The `silent_after_play` fixture adds a second silent tick, because the output tail is held as the peak since the last read.
- **D2:** ST6 asserts Output Level is bit-unchanged at −11.999999, not −12.0f (a host-path set always snaps).
- **D3:** editorScale is persisted, with remove-on-copy.
- **D4:** C7: the timer is the only frame sampler.
- **D5:** C9 corrected pole fixture pair.
- **D6:** C15: bands and station counts come from orderActive.
- **D7:** C17: UI-03 and UI-05 land in 3.2.
- **D8:** `fitMaxForDisplay` returns 1.0 when there is no display.
- **D9:** Deprecated display APIs replaced with their JUCE 8.0.15 equivalents.
- **D10:** The pitch chevron moved to R − 40; at R − 30 it overlapped ▲ and read as a "Y".
- **D11:** `inject-context.py` failed with KeyError `'stages'`. Worked from PLAN, CONTEXT and RESEARCH only.
- **Probe fix:** canvas pixels are blended over the well background before classifying.

## Visual checkpoint (approved by the user 2026-10-13; icon style `3d` kept; no visual fix requested)

Check in the Standalone (`build/plugins/O-simpleAmbisonics/O-simpleAmbisonics_artefacts/Release/Standalone/O-simpleAmbisonics-dev.app`), in Logic, and on the icon sheet `sh-icons-contact.html`:

1. **Meters:** front → left, X hands over to Y while W holds at +1. Behind, X turns amber. Order 1 → 2 → 3 lights 4 / 9 / 16 rows. Peak overlays follow the input.
2. **Sphere:** dragging from above sets azimuth (+90 = left). Crossing behind shows `180.0°`. Clicking on the head does nothing. The side drag sets elevation.
3. **Rotation:** yaw turns the rose, beam and rim tick, never the puck. Pitch past ±90° flips and fades the rose, with the ▲/▼ chevron. At elevation 90 the rim tick holds at half brightness.
4. **Beam and decoder view:** the beam sharpens from order 1 to 3. The decoder view crossfades between the KU100 head and the cardioid pair.
5. **Icons:** pick 3D or slice (the 0.6× and 1× rows decide). If slice: set `SH_ICON_STYLE = 'slice'`, remove the `shicons.js` import, its binary-data entry and its resource branch, then re-run frame-probe and the T9 build.
6. **Resize** 0.6×–1.5×: the aspect holds, text stays sharp, drags track the pointer, and the size persists across reopen.
7. **Idle fallback:** set the output device to none. The well dims, peaks drop to 0, and the puck, bars and beam still follow drags. Restore the device and it is live again.
8. **Look:** spacing, colour and type against the mockup. Known nits: the "3rd order · (3+1)² = 16 chan…" readout is truncated, and the chevron can sit next to the puck when azimuth ≈ yaw.
9. **Hands-on (page → C++):** a knob drag moves its readout, and a lesson moves exactly its recipe controls with Output untouched.

**Open items:** icon pick · `verify_2_1` V3 decision · page → C++ hands-on · truncated order readout · chevron/puck proximity.

---

# Part 2: 3.3 (T10–T13)

**Date:** 2026-10-13 · **Executor:** gui-agent · **Commits:** `bf3e12d9` (T10 shared glossaries), 3.3 commit (this part; SHA in `git log`).
**Icon pick:** the user kept **3D** (the default) at the checkpoint. No slice change: `shicons.js`, its binary-data entry and its `getResource` branch stay. `PluginEditor.cpp` and `CMakeLists.txt` are untouched in 3.3, so pluginval was not re-run (PLAN T13 step 3).

## Tasks and files

| Task | Files | Result |
|---|---|---|
| T10 | `scripts/i18n-fr-glossary.js`, `scripts/i18n-zh-glossary.js` (repo root, shared) | Both were clean in git before the edit. Added an "ambisonics" section to each. Own commit `bf3e12d9`. |
| T11 | `Source/ui/public/js/i18n.js`, `Source/ui/public/index.html` (CSS / markup pins only) | en copy freeze, fr/zh glossary pass, width fixes (tables below) |
| T12 | `tests/i18n-states.json`, `tests/ui-probes/lesson-row-budget.mjs`, `tests/ui-probes/tooltip-clamp-probe.mjs` | 17 C++-producible states; the budget probe with a natives arm; the tooltip clamp probe at 1180×780 and 708×468 |
| T13 | build / install / gates / this SUMMARY / STATUS | All gates exit 0 (table below) |

## T10: glossary entries

| en key | fr | zh-Hans |
|---|---|---|
| azimuth | azimut | (already 方位角) |
| elevation | élévation, élév | (already 仰角) |
| yaw | lacet | 偏航 |
| roll | roulis | 横滚 |
| order | ordre | 阶数 |
| channel | canal | 声道 |
| channels | canaux, can | 声道 |
| b-format | format b, b-format | B 格式 |
| b-format channels | canaux du format b, canaux b-format | B 格式声道 |
| encoder | encodeur | 编码器 |
| decoder | décodeur | 解码器 |
| binaural | binaural | 双耳 |
| headphones | casque | 耳机 |
| speakers | enceintes | 音箱 |
| soundfield | champ sonore | 声场 |
| spherical harmonic | harmonique sphérique | 球谐函数 |
| beam | faisceau | 波束 |
| orbit | orbite | 环绕, 环形, 轨道 |
| latency | latence | 延迟 |

- Rotation pitch and the ear-level "level" stay as `termNote` exemptions in the plugin table. A second glossary rendering for them would loosen `pitch` and `level` suite-wide.
- zh `orbit` also accepts 环形 (O-Orbit `label.pathOrbit`) and 轨道 (O-Strata `label.orbit`). Both renderings already existed, so the suite stays green.
- All new renderings are [ASSUMED] (RESEARCH A5).

**Suite-wide lints:**
- `i18n-zh-lint` (no `--plugin`): exit 0, 0 findings across 46 plugins.
- `i18n-fr-lint` (no `--plugin`): exit 2 both before and after the edit, with **output byte-identical to the pre-edit baseline**. Its one finding is pre-existing and committed on main: O-DigiDelay `js.savePresetPrompt` "Nom de ce préréglage :" (T4, last touched `aeebc2a2`). It is not caused or touched here. **Deviation from T10's literal acceptance** ("suite-wide exit 0"); the gate applied instead is "no new finding".

## T11: copy

**en copy freeze:**
- `chanWell` body now opens "The soundfield as the channels that carry it — FuMa letters, ACN order." The fr and zh bodies say the same.
- `azimuth` body adds: "Crossing behind jumps from +180° to −180°: host automation drawn between the two sweeps through the front." (R2) The fr and zh bodies say the same.
- The `pitch` gimbal note was already present.
- The other UI-06 topics were confirmed present:
  - W/X/Y/Z: `chanWell`
  - (N+1)²: `order`
  - scene vs source rotation: `yaw`
  - ACN / SN3D / AmbiX: `chipFormat`
  - front/back confusion on headphones: `decWrap`
  - Stereo ignores orders 2–3: `decoder`
  - the exaggerated air model: `distance`

**fr glossary pass (root "format B"):**
- B-format → format B in:
  - `chanWell` title
  - `decoder` body
  - `lesson.pointSource` body
  - `label.chanTitle`: Canaux du format B
  - `label.routeEncoder`: en format B
  - `label.stEncoder`: format B · {n} can.
  - `label.tourDefault`
- The zh pass needed no term change: every zh label already matched the glossary.
- Flags stay fr `reviewed: false` and zh `reviewed: 'mt'`. That is 149 entries: 35 tooltips, 114 labels, and the new `label.routeOutput`.

## T11: width fixes (before → after, px; check-ui-labels / Playwright measurements at 1180×780)

**Copy changes:**

| Key | Lang | Before | After | Box |
|---|---|---|---|---|
| `label.subtitle` | fr | "…Un guide de terrain du champ sonore" 501.6 | "…Un guide du champ sonore" 417.6 | 460 max, then the 425 pin |
| `label.decLineBin` | fr | "…latence de 64 échantillons" 223.8 | "…latence de 64 éch." 189.8 | 206 |
| `label.decLineSt` | fr | "…W X Y seulement · 64 échantillons" 217.9 | "…· 64 éch." 184.0 | 206 |
| `label.latencyBody` | fr | "64 échantillons ·" (chip ellipsised "48 k…") | "64 éch. ·" (chip text 131.3) | 172 |
| `label.orbitOn` | fr | Marche 53.7 | **Activé 45.2** | 49 pill |
| `label.routeEncoder` | fr | "vers le B-format" 71.6 | "en format B" 53.8 | 57 |
| `label.routeDecoder` | fr | "vers deux oreilles" 74.9 | "aux oreilles" 50.6 | 67 |
| `label.orbitRate` | fr | "Vit. d’orbite" spill 11.3 (en 6.3) | "Vit. orbite" spill 5.7 | knob cell 52 |
| `label.distOpen` | fr / zh | "air ouvert" spill 6.2 / "空气全开" spill 8.6 (en 2.9) | "air libre" 3.1 / "全开" 0 | knob cell 58 |
| `label.distAir` | zh | "{db} · 空气 {fc}" spill 20.2 (en 15.4) | "{db} · {fc}" (≤ en) | knob cell 58 |
| `label.yawIdle` | fr | "la scène tourne autour du zénith" spill 17.2, crossed `.rot-side` | "tourne sur l’axe vertical" 103.9, spill 0 | hero cell 108 |
| `label.yawHeard` | fr | "l’avant s’entend à {deg}" spill 0.6 | "avant entendu à {deg}" 0 | hero cell |
| `label.pitchSub` | fr | "l’avant se relève" spill 1.1 | "avant relevé" 0 | 58 |
| `label.rollSub` | fr | "le côté gauche monte" spill 9.6 | "gauche relevée" 0 | 58 |
| `label.el.level` | fr | "à l’horizontale" 56.9; crossed `label.distAir` at 12 m | "horizon" 29.8 (`termNote` kept) | — |
| `label.stDecoderSt` | zh | "立体声 · 16 声道中的 4 个 → 2" 133.3 | "立体声 · 4 / {n} 声道 → 2" 104.8 | 128 |

**Pins and markup (index.html):**
- **`.title-block`:** pinned at 425 px (the en subtitle is 424.7). Before the pin, the zh subtitle shrank the block by 164.4 px and slid `.header-strip`, and fr grew it by 35.3 px.
- **Corner readouts in `#topWrap`:**
  - Widths pinned: `.sph-tl` 130, `.sph-tr` 150, `.sph-bl` 140, `.sph-br` 110, `.side-bl` 60.
  - Line boxes pinned at 12 px on `.sph-sub`, `.sph-br` and `.side-bl`. The zh fallback face grew them by 4 px and moved `#sphDist` and `#sideEl` by 2 px.
  - `.sph-sub` now reads "1.00 m · in front": the value first, so a translated word cannot move it.
  - The orbit text is on its own line.
  - In `.sph-tr`:
    - Yaw sits on line 2.
    - Pitch and roll sit on a third line, shown when tilted.
    - Each k/v pair is a right-aligned, width-pinned `.pair`: yaw 62, pitch 74, roll 64.
  - This removed the fr `behindRight × wellYaw` and zh `unitAz × wellYaw` intersections in the tilted states.
- **`.hero-sub`:** pinned at 15 px (zh grew it to 17 and moved the yaw knob by 1 px).
- **`.dec-read`:** a fixed 29 px box (was `max-height: 29`). A one-line zh reading moved `.dec-note` up by 13.2 px.
- **B-format well caption:** now a block line of four `viz-hint` labels. The non-label wrapper moved −75.8 px in zh and +7.1 in fr. The "/" between the polarity words is a CSS `::before`.
- **Output group route word:** now the label `label.routeOutput` (en "L / R", fr "G / D", zh `sameAsEn`). As a bare span it moved −31.7 px in zh.
- **Lesson row:**
  - `.tour-label` pinned at 52 px (en "Lessons" is 51.4).
  - `.tour-hint` pinned at 154 px (fr is the widest at 153.4).
  - `.tour-buttons` flexes between them.
  - Before: the strip moved −27.6 px and shrank 184.6 px in zh.
- **Band-tag column:** 34 → 40 px, padding 6 → 4. The "+3 ch" count is clipped by the tag's own `overflow:hidden`, which check-ui-labels [4] cannot see. fr "+3 can." is 29.6 px and zh "+3 声道" is 33.5 px; the old column showed 26 px, the new one 34. The bar column is 6 px narrower in every language; frame-probe re-passed.

An ad-hoc ancestor-clip scan (every visible `[data-i18n]` vs every `overflow≠visible` ancestor, 18 states × 3 languages) is clean except the known en nit below.

## T12: states, budget, tips, lessons

- **`tests/i18n-states.json`** has 17 states, all C++-producible:
  - settings open; hover help off; back on and closed
  - all defaults (resets the gate's own state pass)
  - Stereo; order 3; orbit on; orbit off
  - tilted (pitch 10, roll −15, yaw 30)
  - one state per lesson: its recipe's exact values set through `window.__stubStates`, everything else default and `outputLevel` untouched, plus the button click for the caption
- **Coverage:** check-ui-labels now reports **0 never-visible labels** (it was 8 with the default state only).
- **Lesson-row budget** (`lesson-row-budget.mjs`): exit 0. All 8 buttons sit on one line in every language. Strip left is 85.0 in all three (pinned label).

  | Lang | Σ buttons + 7 × 6 | Hint text left | Slack |
  |---|---|---|---|
  | en | 842.7 | 1054.1 | **118.4 px** |
  | fr | 836.6 | 1003.6 | **74.0 px** (≥ 8 required) |
  | zh-Hans | 658.1 | 1046.7 | **295.6 px** |

  The fr slack is 74.0 against RESEARCH's 86.1 because the label pin moved strip.left from 80.9 to 85.0, and slack is measured to the hint TEXT.
- **Natives arm:** 8 clicks → `applyFactoryPreset` × 8 with the stable ids in order. Exactly the last-clicked button is lit. 0 page errors.
- **Tooltip clamp** (`tooltip-clamp-probe.mjs`): exit 0, **31/31 PASS**.
  - Viewports: 1180×780 (no transform) and 708×468 (`scale(0.6)`), `innerWidth` / `innerHeight` asserted first, hover help forced on through `localStorage['osamb.tipsEnabled']`.
  - 3 languages × 4 anchors (azimuth knob, Output Level knob, gear, last lesson). Every card lies inside the window by ≥ 4 px, at its natural width (300 = max-width) vs a detached clone.
  - The right-edge clamp engaged 6× per viewport (vacuity guard).
  - Gear tips flip below. At 708×468 the Output Level, gear and lesson cards end at x = 700 (clamped).
- **Lessons live in C++:** gated by harness ST6 (unchanged, PASS). The Standalone page → C++ path (a lesson click moving its controls, Output untouched) was **not drivable from this session**: synthetic clicks are refused without the Accessibility permission (System Events −25208; CGEvent posts were ignored). This stays in the human checklist. A Standalone window capture after the rebuild confirms the WKWebView page renders the 3.3 tree: EB Garamond, the "1.00 m · in front" readout, the "+ sage / − amber" caption, the lesson row.

## T13: gates on the real tree (all from the repo root)

| Gate | Exit | Numbers |
|---|---|---|
| `check-i18n --plugin O-simpleAmbisonics` | **0** | ALL CHECKS PASS, canon v2; 149/149 fr unreviewed (35 tooltip, 114 label) |
| `i18n-fr-lint --plugin O-simpleAmbisonics` | **0** | CLEAN; termNote exemptions: pitch ×2, ear-level |
| `i18n-zh-lint --plugin O-simpleAmbisonics` | **0** | GATE PASSED, 0 findings; 149 at `'mt'` (counted, not failed) |
| `check-ui-labels --plugin O-simpleAmbisonics` | **0** | `seed=overrides`, default + 17 states, 0 never-visible, ALL CHECKS PASSED (it was exit 8 on the real tree before T11) |
| `boot-all-uis --plugin O-simpleAmbisonics --strict-tips` | **0** | clean 1/1, 0 warn, 0 failed, **0 DEAD, 0 late**, 0 i18n diagnostics; the stub invented `uiReady` only |
| `frame-probe.mjs` | **0** | 13/13 |
| `frame-probe --negative-control fuma / pole / idle` | 0 / 0 / 0 | A1 / A9 / A12 each "FAILS as designed" |
| `lesson-row-budget.mjs` | **0** | slack per language as above |
| `tooltip-clamp-probe.mjs` | **0** | 31/31 |
| `generate_sh_icons.mjs --check` | **0** | no drift |
| suite-wide `i18n-zh-lint` | 0 | 0 findings across 46 plugins |
| suite-wide `i18n-fr-lint` | 2 | identical to the pre-T10 baseline (O-DigiDelay T4 only, pre-existing) |

**Build / install:**
- **Build:** `ninja` on the four targets (VST3, AU, Standalone, render-test) exited 0 with **0 warnings**.
- **Harness:** **251/251 PASS, 0 FAIL.** The binary is unchanged since 3.2; no C++ changed in 3.3.
  - digest-2.1 `9189d911a1fe98d5` PASS
  - digest-2.2 `7f35bc9f91e321b9` PASS
  - digest-2.3 `edd1acfd21f54bcd` reproduced
  - PERF-02 195× real time
- **Install:** `build-and-install.sh` exited 0 (0 warnings). The installed VST3 is `cmp`-identical to the build and carries the new copy. The Standalone is current.
- **auval:** `auval -v aufx OSAm OuDv` → **AU VALIDATION SUCCEEDED**.
- **pluginval:** not re-run. No editor or CMake change in 3.3; the last run was SUCCESS at T9.

## Deviations (Part 2)

- **D12:** T10's suite-wide fr-lint acceptance cannot be exit 0 on today's main because of the pre-existing O-DigiDelay finding. The gate applied is "no new finding": output byte-identical to the baseline.
- **D13:** fr Orbit On face → `Activé`. This is the PLAN's first option and a glossary alternate, kept masculine as a generic state word. The pill was not widened.
- **D14:** zh drops 空气 from the two distance readouts. One Han glyph cannot fit the en-authored overhang; the tooltip names the air filter.
- **D15:** fr `label.el.level` → `horizon`, not `à l’horizontale`. It collided with the distance readout at 12 m.
- **D16:** layout pins beyond T11's named list, all forced by check-ui-labels [7] / [8] once the states file made them visible:
  - corner readout boxes and line heights
  - the sph-tl / sph-tr reflow (value first; orbit and tilt on their own lines)
  - hero-sub and dec-read heights
  - the B-format caption unwrap with a CSS slash
  - `label.routeOutput`
  - lesson-row pins
  - the band-tag column 34 → 40

  These are small en-visible changes after the visual checkpoint. Check them by eye at verify.
- **D17:** `lesson-row-budget.mjs` measures slack to the hint's TEXT. The hint box is now pinned at the fr width, so the box edge no longer marks the visual limit.
- **D18:** the tooltip probe runs 3 languages (PLAN: unspecified) and adds the natural-width-vs-clone check.

## Human checklist: still open (verify closes these)

- [ ] **Logic, R1 (U1):** stopped, non-armed track with the editor open. Does the well dim, and do the puck, bars and beam follow parameters and automation? Press play: undimmed and frame-driven. Record whether Logic suspends `processBlock`.
- [ ] **Logic, R2:** record a rear-crossing sphere drag as automation; on playback, does the source sweep through the front? (The azimuth tip documents it.)
- [ ] **Logic, R3:** CPU with the editor open on a static scene vs closed.
- [ ] **Logic, general:**
  - automation moves the UI
  - one drag = one gesture; closing mid-drag ends it
  - hide / re-show resumes the meters
  - save / reopen keeps the size and the language
  - 10× close / reopen without a crash
- [ ] **Standalone:**
  - Tab and arrows reach and move every knob
  - a release outside the window ends the drag
  - double-click resets to default
  - the hover-help switch persists
  - fr / zh relabel without clipping; the select and the Decoder segment stay English
  - Reduce Motion stops every transition
  - **each lesson moves exactly its recipe controls and never Output** (page → C++; not drivable headless)
  - Overhead at order 3 does not clip the output meter in either decoder
- [ ] **Look at the D16 reflow by eye:**
  - "1.00 m · in front"
  - the orbit line
  - the tilt line
  - the band-tag column
  - the lesson row
- [ ] Optional: Taylor reviews the fr copy (149 entries). After any change, re-run `lesson-row-budget.mjs` and `check-ui-labels.js`.
- [ ] **R9:** a host block longer than ≈ 100 ms flips to idle between blocks. Report it if seen. The fix is `kStaleTicksIdle`, changed only on the user's say-so.

## Known nits (not fixed, user-visible)

- **en `label.encReadN`:** "(3+1)² = 16 channels" is ellipsised by `.enc-read`: 8.0 px at order 3, 4.4 px at order 2. This is the checkpoint nit. No fix was requested, so it was left. A candidate fix is "= {ch} ch".
- **Chevron / puck proximity** when azimuth ≈ yaw: the checkpoint nit, unchanged.
- **`verify_2_1.py` V3 Binaural arm** (3 checks): stale since Phase 2.3. Pre-existing; needs a user decision. The Stage-2 file was not edited.

## Deferred

- bank-fade indicator
- preset-manager natives + the FUNC-08 bank (Stage 4)
- QUAL-01 preset-load clause
- Windows build (COMPAT-02, Stage 4)
- v1.1 parameters (spread / weighting / solo)
- the TripleBuffer → `core/triple-buffer` module
- fr native review (`reviewed: true`)
- zh back-translation to `'bt'` (the ship bar; `'mt'` never ships)
