# Stage 3: GUI (Terrain tab, oscillator cards, readout, ≋ cycle view, i18n), Round A — Verification

## Verification Date

2026-09-12

**Plugin:** O-Strata · **Stage:** 3 of 4 (GUI) · **Round:** A = ROADMAP Phase 3.1 (CONTEXT D1) · **Mode:** manual
**Inputs:** `CONTEXT.md` (D1–D4, requirements map), `RESEARCH.md` (§5.4 gate list A1–A12), `PLAN.md` (Round A — 15 tasks, Decisions 1–26, success criteria), `SUMMARY.md` (execute report, commits `9e4a6e65` Phase 3.1a + `9699a166` Phase 3.1), `round-a-zh-triples.md`, `REQUIREMENTS.md` v2.0.0, the O-Prism page at `4f12ef57`.
**Method:** every SUMMARY figure was re-measured in this session from the committed tree at `9699a166` (working tree identical for `plugins/O-Strata` — `git status` shows only the two untracked `gate-report.json` files): `ninja` reported no work for the plugin, Standalone, param-dump and harness targets; then A1–A12 re-run from the repo root, the nine inherited page regions re-diffed against `git show 4f12ef57:…/index.html`, the grep gates, the param-dump diff, a warning census of the six touched translation units (forced recompile), pluginval ×2, auval, and one scripted Standalone pass (Terrain tab + pushed readout). Nothing below is copied from SUMMARY without a fresh run. No plugin source changed in this phase.

## Goal-Backward Analysis

### Original Goals (CONTEXT.md "Goal", Round A scope per D1; PLAN success criteria)

1. Replace the fork's v1.24.0 page with the locked v2 shell re-forked from O-Prism v1.26.0 (`4f12ef57`): the two oscillator cards, the fifth **Terrain** tab with one proxy-bound panel, inert rules, the readout, the ≋ cycle view, the botanical plate — with the inherited regions byte-identical and no visual regression on the four inherited tabs.
2. Bind the 34 new parameters to the existing relays; every control moves its parameter, follows automation, and refreshes on preset apply (UI-04 controls half); the mod-destination dropdowns list 46 entries (FUNC-05 UI half).
3. Readout from the Stage 2 atomics plus the D3 `silent above <note>` form computed by the scheduler; ≋ view from a `terrainCycle` push that keeps updating after the display voice ends (ring 8192, block-end re-election).
4. Editor hygiene: dead stubs removed, `evaluateJavascript` 0 / 0, every push through `emitEventIfBrowserIsVisible`, `requestTerrainRepush` registered, native parity.
5. en / fr / zh-Hans with every UI gate green at 1200 × 800 (check-i18n, lints, check-ui-labels, boot, tip gate, measure-ui, CDP face probe, the two new layout / shell-diff gates), fr at `reviewed: false` for Taylor's read, zh at `'mt'` with a blind back-translation on file.
6. COMPAT-01 regression (pluginval ×2 + auval), `--gate all` 139 / 139 ≤ 3 min, `params.tsv` unchanged, two path-scoped commits, no tag.

### Deliverables (SUMMARY.md)

1. `Source/ui/public/index.html` replaced (base (i) merge), `js/i18n.js` (+32 keys, +14 tips), `img/shell_conchologiaiconi12reev_0090.png`, `#terrain-tab` after `#effects-tab`, the ⬡ placeholder renderer under the `PLACEHOLDER (Stage 3 Round B replaces)` token.
2. `bindKnob (…, stateOverride)`, `bindCombo`, `SliderProxy` / `ComboProxy`, the TerFreq log adapter (RESEARCH C1), the 46-entry fallback, `__refreshAllControls` wrapped so every preset apply also calls `requestTerrainRepush`.
3. `CycleCapture::kPoints` 8192 + `static_assert`, `StrataVoice::startSerial` + `reelectDisplayVoice()`, `chebTopNote[2]` + `TerrainScheduler::refreshTopNote`, `Source/TerrainViewFeed.{h,cpp}`, `TerrainStatus` / `getTerrainStatus`, harness `--gate topnote` + `--dump-choices`.
4. `PluginEditor`: `getActiveOscInfo` / `getActiveOscFrame` deleted, `requestTerrainRepush` registered (30 natives), `pushHeldNotes` / `pushStatus` / `pushCycle` from `timerCallback`, PNG route.
5. `tests/ui_layout_check.js`, `tests/ui_shell_diff_check.js`, `tests/tools/{gen-stub-overrides.mjs, cdp-font-probe.js}`, `tests/ui-stub/generic-overrides.json` regenerated, `tests/i18n-states.json` +13, `tests/ui_tip_render_check.js` five tabs at 120, `round-a-zh-triples.md`.
6. CHANGELOG, PLUGINS.md row, STATUS, SUMMARY; commits `9e4a6e65` + `9699a166`.

### Goal Achievement

| Goal | Status | Evidence (re-measured 2026-09-12) |
|---|---|---|
| 1 shell re-fork, byte-identical regions, no visual regression | ✅ Achieved | `initializeTipsToggle`, `applyTipsEnabled`, `setupTooltips`, `#tuning-tab` markup, `#preset-save-modal-overlay` block, `.tooltip` CSS and the trailing tooltip div each `diff` empty against `4f12ef57`; check-i18n [6] canon v2 passes; shell-diff gate **0 px outside the masks** on synth / mod / tuning / effects (inside 48278 / 9003 / 3655 / 3655), self-diff 0, planted control 1576 px |
| 2 controls bound, 46 destinations | ✅ (automated) · ⏳ human rows | check-ui-labels 1431 PASS / 0 FAIL over `default + 33` states seeded from the 205-row param-dump; layout gate 358 / 0 incl. 46 options × 16 destination selects and `#val-terFreq === '1.00×'`; scripted Standalone: Terrain tab opens, Osc A active, Terrain / Orbit combos show `Sine Product` / `Ellipse`, Freq `1.00×`, Orbit Size 50 %, Pitch Track 100 %, Blur / Edge and Orbit Mod greyed. Host-automation follow, preset apply refresh and the Osc A / B repoint remain human rows (below) |
| 3 readout + ≋ view + D3 | ✅ (automated + readout live) · ⏳ ≋ with a note | Readout in the Standalone reads the pushed **`2× · 16 partials at C4`** (not the JS estimate 12); `--gate topnote` 4 / 4 (Radial Rings × Epi 3 = 92, Sine Product × Epi 3 = 93, Mitsuhashi −1, 2× −1); `terrain-cycle-pushed` state renders in the stub. The ≋ view with a held note and its re-election after release need a MIDI source (human row — see Issues 2) |
| 4 editor hygiene | ✅ Achieved | `evaluateJavascript` 0 / 0; natives C++ 30 = page 30, `comm -3` empty; viewport units 0; exactly one `^<script type="module">`; member-order comment tokens at `PluginEditor.h:57 / 66 / 69` ascending |
| 5 i18n + UI gates | ✅ Achieved | Table below — every A1–A11 threshold of Decision 22 met on re-run; fr 46 rows at `reviewed: false`, zh 46 at `'mt'`, 60 triples on file |
| 6 COMPAT-01 regression, harness, commits | ✅ Achieved | pluginval strictness 10 VST3 + AU `SUCCESS`; `auval -v aumu OuSt OuDv` `AU VALIDATION SUCCEEDED`; `--gate all` **139 / 139 in 114.8 s** from the repo root; live param-dump vs `params.tsv` diff empty (209 lines); commits `9e4a6e65` + `9699a166` path-scoped, no tag |

## Requirements Verification

**Stage:** stage-3 (Round A = Phase 3.1) · **Requirements mapped to Round A (CONTEXT "Requirements map"):** UI-04 (controls half), FUNC-05 (UI list). Stage-3 rows for Round B: UI-01, UI-02, UI-03, PERF-03, FUNC-07.

| Requirement | Priority | Status after Round A | Acceptance evidence |
|---|---|---|---|
| FUNC-05: 10 new destinations per oscillator, 46 in the list | must | ✅ complete (was complete at stage-2; UI half now verified) | layout gate: 46 options in every `.mod-col-dst select` (16 × 46); stub overrides generated from `--dump-choices`; scripted Standalone shows the dropdowns |
| UI-04: 3D view and playhead re-push after preset apply / session restore | must | ⚠️ partial — controls half | `__refreshAllControls` wrapper calls `requestTerrainRepush` on every preset apply (code); layout gate `#val-terFreq` 1.00× at default; preset-apply refresh in a host = human row; view / playhead re-push = Round B 3.2 |
| COMPAT-01 | must | ✅ complete (regression) | pluginval ×2 SUCCESS, auval SUCCEEDED (this session) |
| UI-01, UI-02, UI-03, PERF-03, FUNC-07 (UI halves) | should / must | ⏸️ pending — Round B | ⬡ placeholder present under the labelled token; `#terrain-webgl-unavailable` markup and `#btn-terImport` in place; WebView2 halves Phase 4.2 (D2) |

**Requirements Summary:** ✅ complete 2 (FUNC-05, COMPAT-01 regression) · ⚠️ partial 1 (UI-04) · ⏸️ deferred to Round B 5 · ❌ failed 0.

## Automated Checks

All numbers measured in this session (Apple M4 Max, macOS 26 / Darwin 25.6, Release build, node v24, Playwright from the npx cache). Logs in the session scratchpad `logs/`.

| # | Check | Result | Threshold (Decision 22) |
|---|---|---|---|
| — | `ninja O-Strata_VST3 O-Strata_AU O-Strata_Standalone O-Strata-render-test O-Strata-param-dump` | `no work to do` — binaries match the committed tree | build clean |
| A1 | `check-i18n.js --plugin O-Strata` | `ALL CHECKS PASS`, 16 / 16 | 16 / 16 |
| A2 | `i18n-fr-lint.js` | `CLEAN — exit 0` (36 straight copies all covered, 3 termNote exemptions) | exit 0 |
| A3 | `i18n-zh-lint.js` | `GATE PASSED — exit 0`, 0 findings, **46 at `'mt'`** counted | exit 0, 46 `'mt'` |
| A4 | `check-ui-labels.js` | `ALL CHECKS PASSED`, **1431 PASS / 0 FAIL**, `states: default + 33`, `seed=param-dump (205 rows) + overrides` | exit 0, +33, ≥ 885 |
| A5 | `boot-all-uis.js --strict-tips --verbose` | `clean 1 / 1`, `text=1266 aria=12 title=0 i18n=202`, DEAD 0 / late 0, no `stub invented:` / pageerror / 404 | DEAD 0 / late 0 / title 0 |
| A6 | `tests/ui_tip_render_check.js` | `ALL CHECKS PASSED (3163)` | ALL PASS at 120 bindings |
| A7 | `measure-ui.js --mode box --report all` | `undeclared-font: 5`, `wrap-count: 0` | ≤ 5, 0 |
| A8 | `tests/tools/cdp-font-probe.js` | 16 runs, 0 nodes missing, **0 Han runs off PingFang SC** | every Han run on the declared face |
| A9 | `tests/ui_layout_check.js` | `ALL PASS — 358 passed, 0 failed`; liveness `--viewport 1200x780` → `FAILED — 307 passed, 51 failed`, exit 2 | ALL PASS; liveness fails |
| A10 | native parity + push purity | `comm -3` empty (30 = 30); `evaluateJavascript` 0 / 0; viewport units 0; module scripts 1; member-order tokens ascending | empty; 0 / 0; 0; 1 |
| A11 | `tests/ui_shell_diff_check.js` | synth / mod / tuning / effects **0 px outside masks**; self-diff 0; planted `.section-header{margin-left:1px}` → 1576 px | 0 outside; control fires |
| A12 | pluginval strictness 10 VST3 + AU; `auval -v aumu OuSt OuDv`; `--gate all` | `SUCCESS` ×2; `AU VALIDATION SUCCEEDED`; **139 / 139, 114.8 s** (`topnote` 92 / 93 / −1 / −1) | green; 139 / 139 ≤ 3 min |
| — | live param-dump vs `.planning/params.tsv` | diff empty (209 / 209 lines) | empty |
| — | inherited regions vs `4f12ef57` | tips toggle, `applyTipsEnabled`, `setupTooltips`, Tuning markup, preset modal, `.tooltip` CSS, trailing tooltip div: all `diff` empty | byte-identical |
| — | warning census (forced recompile of `TerrainViewFeed.cpp`, `PluginEditor.cpp`, `PluginProcessor.cpp`, `StrataVoice.cpp`, `TerrainOscillator.cpp`, `TerrainScheduler.cpp`) | 11 warnings: `-Wfloat-equal` ×6 (`TerrainOscillator.cpp`, `PluginProcessor.cpp`), `-Wunused-lambda-capture` ×4 (`PluginEditor.cpp`), `-Wswitch-enum` ×1 (`TerrainOscillator.cpp:321`, `warpType` switch — dates from Phase 2.1 `4640a1a7`, not Round A). `TerrainViewFeed.cpp`, `StrataVoice.cpp`, `TerrainScheduler.cpp`: 0 | 0 new |
| — | scripted Standalone (window capture + process-scoped clicks, session state stripped then restored) | Terrain tab renders at 1200 × 800: toolbar, readout **`2× · 16 partials at C4`** (pushed value), ⬡ placeholder surface with the orbit and HUD `θ 0.00 · r 0.53 · c (0.13, 0.21)`, both knob cards with the inert greys, botanical plate, hint line | readout = pushed status |

## Human Verification

Open rows for Taylor. None of them blocks the Round B plan; rows 3–8 need a MIDI source or Logic.

- [ ] **1. French read** — 46 rows at `reviewed: false` (32 labels + 14 tip titles). Flip to `reviewed: true` by hand. The full list, en → fr:
  `label.subtitle` Synthétiseur microtonal à terrain d’onde · `tab.terrain` / `label.terrain` Terrain · `label.orbit` Orbite · `label.orbitSize` **Taille** (shortened from "Taille orbite", 64 px Synth-card column) · `label.terrainFreq` / `tip.terFreq` Fréq. · `label.terrainModX` / `tip.terModX` Mod X · `label.terrainModY` / `tip.terModY` Mod Y · `label.pitchTrack` **Suivi haut.** (shortened from "Suivi hauteur", 82 px column) · `tip.terTrack` Suivi de hauteur · `label.saturation` / `tip.terSat` Saturation · `label.imageBlur` / `tip.terBlur` Flou image · `label.edgeMode` Mode bord · `label.orbitAspect` / `tip.terAspect` Aspect · `label.orbitRotation` / `tip.terRot` Rotation · `label.orbitCentreX` / `tip.terCX` Centre X · `label.orbitCentreY` / `tip.terCY` Centre Y · `label.orbitMod` / `tip.terOrbMod` Mod orbite · `label.feedbackDamp` Amort. réinj. · `tip.terFb` Réinjection · `tip.terFbDamp` Amortissement de la réinjection · `tip.terSize` Taille · `label.quality` Qualité · `label.bandlimited` Bande limitée · `label.import` Importer… · `label.dropPng` Déposer PNG · `label.webglUnavailable` WebGL indisponible — repli 2D · `label.sourceMissing` Source manquante — repli sur la bibliothèque · `aria.view3d` Vue 3D · `aria.viewWaveform` Vue forme d’onde · `label.hintView` Glisser : centre · Molette : taille · ⌥ glisser : rotation · `label.readoutPartials` {n} partiels à {note} · `label.approx` approx. · `label.imageProjected` image projetée à F = {f} · ajustement {pct} % · `label.silentAbove` muet au-dessus de {note} · `label.modMatrixInfo` Routez n’importe quelle source vers n’importe quelle destination. 16 emplacements, 46 destinations.
  Reader's notes (not corrections): "Suivi haut." is the only abbreviation a player may not decode at a glance — "Suivi" alone fits the 82 px column if the tip title carries the full form; "Mode bord" could read "Bords"; "ajustement" for *fit* is the glossary-consistent choice but "fidélité" is the closer musical sense.
- [ ] **2. zh-Hans triples** — `round-a-zh-triples.md` (60 rows, worst drift first, all ≥ 0.39; the two lowest-scoring rows are abbreviation expansions "Fb Damp → Feedback Damping", "Freq → Frequency", not meaning drift; `label.silentAbove` back-translates "Muted above {note}" — same meaning). Flip `'mt'` → `'bt'` by hand where accepted.
- [ ] **3. ≋ view with a held note** — the trace follows the note, holds the last cycle after release, and keeps updating after the display voice ends while an older note still sounds (Decision 10 re-election).
- [ ] **4. Host automation** — spot-check 6 of the 34 new parameters (incl. both TerFreq knobs: `1.00×` at default, `2.00×` at normalised 0.6) follow automation lanes in Logic.
- [ ] **5. Preset apply** — `Init` refreshes every control, including the Terrain-tab proxies and the readout (the `requestTerrainRepush` wrapper).
- [ ] **6. Osc A / B toggle repoints** — edit B's Orbit, toggle to A: A's values unchanged; toggle back: B keeps the edit.
- [ ] **7. Tuning held-notes bar** — still tracks keys through the pushed `heldNotes` (was `evaluateJavascript`).
- [ ] **8. Logic (WKWebView) pass** — the Terrain tab, readout forms (`2× …`, `Bandlimited · … · approx.` on Superellipse, `… · silent above G#6` on Radial Rings × Epitrochoid 3 in Bandlimited) and inert rules in the AU.
- [ ] **9. D3 feel** — τ = 2e-3 / hide ≥ 108: does "silent above G#6" on Radial Rings × Epitrochoid 3 match where the note actually goes quiet?
- [ ] **10. `#terrain-readout` two-line wrap** for the longest Bandlimited form (`… · approx. · silent above A#6`) — accept the wrap, or shorten `label.readoutPartials` / narrow the Import button.

**How to feed MIDI to the Standalone for rows 3–7:** the JUCE Standalone on macOS does **not** auto-enable MIDI inputs (`autoOpenMidiDevices` is false outside iOS / Android), so a virtual port (e.g. IAC Bus 1, or `mido.open_output ('name', virtual=True)`) must be ticked once under Options → Audio/MIDI Settings → *Active MIDI inputs*; the tick persists in `~/Library/Application Support/O-Strata-dev.settings` (`audioSetup`). Without the tick the ≋ view stays empty and the readout still shows the pushed status — which is what this session saw.

## Issues Found

1. **No defect in the delivered code.** Every SUMMARY figure reproduced on re-measurement (gate-all 114.8 s vs 112.8 s; every other number identical).
2. **MIDI hands-on not completed by script (process, not product).** A virtual MIDI source was created (`mido` / rtmidi, visible to CoreMIDI) but the Standalone never received notes because macOS Standalones do not auto-open MIDI inputs (above), and enabling it through the Options dialog needs the app frontmost — at that point the Standalone was behind Taylor's active Chrome window and the scripted `activate` was stealing focus from live work, so the pass was stopped. The Standalone instance launched by this session exited once between two captures with no crash report under `~/Library/Logs/DiagnosticReports` and nothing in the unified log; the most likely cause is that it was closed by hand while the machine was in use. It was relaunched, exercised (Terrain tab + readout), quit cleanly, and the saved session state was restored from the backup.
3. **`-Wswitch-enum` at `TerrainOscillator.cpp:321`** is pre-existing (Phase 2.1) and outside Round A's "0 new warnings" claim; recorded here so the Round B census does not re-discover it.
4. **Carried, unchanged:** O-Prism v1.26.1 `.octave-stretch-slider { min-width: 0 }` + `.settings-toggle` pin port (D4, separate `/improve O-Prism`); muted Bandlimited rows + soft knee (optional Stage 4 / v1.1 discuss); vectorised Chebyshev basis (Stage 4 PERF); ASan re-run when the toolchain allows; WebView2 evidence (Phase 4.2); promoting `cdp-font-probe.js` to `scripts/`; repo-wide glossary TERMS.

## Stage Verdict

**Round A (Phase 3.1) status:** ✅ VERIFIED — every automated gate of Decision 22 re-measured green from the committed tree; the shell merge is pinned (byte-identical regions + 0 px shell diff), the relays are bound, the readout is live from the atomics, COMPAT-01 holds. Ten human rows are open (French read, zh flips, the MIDI / Logic hands-on set); none changes the code that Round B builds on.

**Stage 3 as a whole:** ⏳ in progress — UI-01 / UI-02 / UI-03 / PERF-03 / FUNC-07 are Round B (Phases 3.2 + 3.3).

**Ready for the Round B plan:** Yes — `/plugin-plan O-Strata 3-gui` (Round B: 3D view + pushes, then interaction + import). Round A artifacts move to `stages/3-gui/round-a/` per CONTEXT D1.

**Blockers:** none.
