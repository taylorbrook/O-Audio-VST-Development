# Stage 3 (GUI) Integration Checklist — v2

**Plugin:** O-Granulator
**Mockup Version:** v2 (finalized 2026-11-04, `v2-ui.yaml` `finalized: true`)
**Generated:** 2026-11-04 by ui-finalization-agent
**Parameter contract:** `../parameter-spec.md` (LOCKED, 220 parameters)

Files in this directory:

| File | Goes to | What it is |
|------|---------|------------|
| `v2-ui.html` | `Source/ui/public/index.html` | The complete production page: inline `<style>` + one inline `<script type="module">` controller (Family A knobs, canonical i18n, hover help). |
| `v2-i18n.js` | `Source/ui/public/js/i18n.js` | The en / fr / zh-Hans table module (exports only). 90 I18N + 210 LABELS + 298 TIP_BINDINGS. fr `reviewed: false`, zh-Hans `reviewed: 'mt'`. |
| `v2-PluginEditor.h` / `.cpp` | `Source/PluginEditor.h` / `.cpp` | Editor TEMPLATE: 220 relays/attachments, 21 native functions, resource provider, Windows WebView2 block. Adapt names to the Stage 1/2 processor; the shapes are the contract. |
| `v2-CMakeLists.txt` | `CMakeLists.txt` | The COMPLETE plugin CMakeLists (Stage 1 lands it; Stage 3 verifies the WebView block). |

Pre-verified on a scratch tree built from exactly these files (Source/ui/public + the CMake + the editor):
`check-i18n` ALL PASS · `i18n-fr-lint` CLEAN · `i18n-zh-lint` 0 findings (300 at `'mt'`) ·
`check-ui-labels` 0 failures with each of STREAMS / FX / MOD, the scale list, the custom-scale editor and the
settings popover forced visible · `boot-all-uis --strict-tips` clean, 0 DEAD / 0 late bindings ·
native-function parity 21 = 21 (page + preset-manager.js vs `withNativeFunction`). Re-run all of them on the real
tree — section 8 — because the real processor and real stub data are what Stage 3 ships.

## 1. Copy UI files
- [ ] Copy `v2-ui.html` to `Source/ui/public/index.html`
- [ ] Copy `v2-i18n.js` to `Source/ui/public/js/i18n.js`
- [ ] Copy the JUCE frontend library to `Source/ui/public/js/juce/index.js` and `check_native_interop.js` (from a working plugin, e.g. `git show HEAD:plugins/O-ReverseDelay/Source/ui/public/js/juce/index.js`)
- [ ] Do NOT copy `preset-manager.js` or the EB Garamond files into `Source/ui/public/` — CMake embeds them from `modules/` and `getResource` serves them at `/js/preset-manager.js`, `/css/eb-garamond.css`, `/fonts/*.woff2`. (`ouaricon_add_module(... preset-manager)` also copies the JS to `Source/ui/public/modules/`: gitignored build artefact, never committed — pattern_module_js_move_build_recopies_tracked_file.)
- [ ] **Settings popover was ADDED** — the mockup had none. `#gear-btn` + `#settings-popover` (`#lang-select`, `#tips-toggle`) sit top-right in the header at (1124, 62), 24 × 24, under the OUTPUT knob's text column; the popover opens down-left over the buffer strip. Designer: confirm the placement.

## 2. Update PluginEditor and processor files
- [ ] Replace `PluginEditor.h` / `.cpp` with the v2 templates; rename `GranulatorEditor` / `GranulatorProcessor` to the Stage 1 class names
- [ ] Verify member order: relays (3 vectors) → `webView` → attachments (3 vectors) → `webView2RuntimeLink`
- [ ] Verify the constructor creates them in the same order (relays → options → `webView` → attachments)
- [ ] `kSliderIds` (140) / `kComboIds` (59) / `kToggleIds` (21) equal `createParameterLayout()` by TYPE as well as by ID — a Bool on a slider relay, or a Choice on a slider relay, attaches cleanly and never updates. Diff both directions.
- [ ] Native functions registered — exactly 21, and the page calls no other:
  `getParameterDefaults`, `getUiLanguage`, `setUiLanguage`, `getTelemetry`, `getWindowCurve`,
  `getScaleState`, `setDegreeMask`, `setCustomScale`, `parseScaleText`, `loadScala`, `saveScala`,
  `savePreset`, `loadPreset`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`,
  `deletePreset`, `isFactoryPreset`, `savePresetWithDialog`, `loadPresetFromFile`.
  Diff `grep -o "getNativeFunction('[A-Za-z]*')" index.html` + preset-manager.js against `withNativeFunction ("…"` — an unregistered fn hangs its promise silently (pattern_webview_native_fn_bridge_gap).
- [ ] `getParameterDefaults` reports every Float AND every Choice (choice = index) — the stepped division knobs reset through it
- [ ] `getResource` serves `/js/i18n.js` as `application/javascript; charset=utf-8`; every `juce_add_binary_data` source has a branch and vice versa (index.html, i18n.js, juce/index.js, juce/check_native_interop.js, preset-manager.js, eb-garamond.css, 3 × woff2); bare-path comparisons only
- [ ] Windows block kept exactly: `withBackend (webview2)`, user-data folder `%APPDATA%/Ouaricon/WebView2/O-Granulator` created first, `areOptionsSupported()` probe, `webView` null on failure + native notice; every `webView->` use outside the constructor null-guarded; never `setVisible (false)` on the WebView (critical_webview_completion_gated_on_isvisible)
- [ ] Window: fixed 1200 × 800, `setResizable (false, false)` (ui-design-rules Rule 4 — fixed, exactly 800 tall). The page's `body` and `#plugin` are 1200 × 800 px; keep all three in sync
- [ ] **Processor — UI language:** `std::atomic<int> uiLanguage { 0 };` + `static juce::String languageCode (int)` (1 → "fr", 2 → "zh-Hans", else "en") + `static int languageIndex (const juce::String&)` (unknown → 0); persisted as a non-parameter `uiLanguage` property on `parameters.state`, written in `getStateInformation` BEFORE the XML copy and restored in `setStateInformation` only when present (O-ReverseDelay `PluginProcessor.h/.cpp` is the reference)
- [ ] **Processor — scale STATE** (message thread; not parameters — parameter-spec.md "Non-parameter state"):
  - `const PitchCollections::Table* getCollectionTable (int choiceIndex) const` — nullptr for 0 (Off); 31 → the shared custom table
  - `CustomScale getCustomScale() const` → `{ name, text, period, version }`
  - `juce::uint64 getDegreeMask (int stream, int choiceIndex) const` (absent = all on) / `void setDegreeMask (int, int, juce::uint64)` — publish to the engine lock-free
  - `juce::String setCustomScale (name, text, juce::String& badToken)` → `""` on success, else `noPitches` / `badPitch` / `badPeriod` / `tooMany` / `badEdo`; on success bump `customVersion`, and if the degree list changed reset every stream's Custom mask to all-on
  - `std::uint32_t getScaleStateRevision() const` — bumped ONLY by `setStateInformation` and preset apply (customLoad), never by the setters (pattern_webview_one_shot_state_push_stale_on_preset_load). The page re-pulls `getScaleState` whenever `getTelemetry.scaleRev` moves.
  - Persist masks as 16-hex-digit strings, never `getLargeIntValue` (critical_juce_getlargeintvalue_clamps_unsigned_64); presets carry custom scale + masks via `setCustomStateCallbacks`; preset apply resets them to defaults first; strip the stale `<CustomState>` child after `setStateFromXml` (critical_preset_manager_stale_customstate_child)
- [ ] **PitchCollections module** (Stage 1 task): extract from `plugins/O-GrainScatter/Source/dsp/PitchEngine.h` into `modules/tuning/pitch-collections` (provisional), add `edo N [period]` + `badEdo`, a `tableFromText (text, table, error, badToken)` overload and `toScala (name, text)`; GrainScatter consumes the module too. Then uncomment `ouaricon_add_module (... pitch-collections)` in CMakeLists. Never fork a copy into `Source/`.
- [ ] **Processor — live data** for `getTelemetry` (a PULL at ~30 Hz; no juce::Timer, no emitEvent): `bool readTelemetry (Telemetry&)` (seqlock or try-read; return false on a torn read — the page keeps its last frame). Fields the editor reads:
  `frozen`, `spanSec` (bufferLength live / heldSpan frozen), `wave[464]` + `waveCount` (peak per bin, oldest → newest across the span),
  `grains[≤128]` + `grainCount` (`stream` 1–4, `delaySec` behind the head, `pitchSt`, `pan` −1…1, `env` = window value now, `gain`, `lengthSec`),
  `counts[4]`, `limit`, `outPeakL/R` + `inPeak` (consuming peaks), `sources[11]` (LFO 1–4 −1…1, Env, Pitch, Onset, Macro 1–4 0…1), `lfoPhase[4]`,
  `onsetPulse`, `onsetFired` (since last read), `pitchMidi` (≤ 0 = none) / `pitchCents` / `pitchClarity`, `loopGainDb` (non-finite = loop open),
  `modNorm[113]` + `modRouted[113]` (modulated normalised value per routed destination). Never let the editor touch the pool or scheduler directly.
- [ ] **Processor — window figure:** `void getWindowCurve (int stream, float* out, int numPoints) const` sampled from the engine's WindowMorphBank at the stream's base `window` / `window_skew` (the figure must be the engine's curve, never a JS model — O-ReverseDelay `getWindowCurve` precedent). The filter / drive / ring / reverb figures are schematic "fig." engravings drawn from the knob values; they are not measurements.
- [ ] `modules/core/webview-relay-manager` is deliberately NOT used: it has no `WebComboBoxRelay` (59 of 220 are choices) and no WebView2 handling.

## 3. Update CMakeLists.txt
- [ ] `v2-CMakeLists.txt` is the whole file (not a snippet). Land it as `plugins/O-Granulator/CMakeLists.txt`
- [ ] `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_CURL=0`
- [ ] `juce_generate_juce_header (OuariconGranulator)`; `target_compile_features (... cxx_std_20)`
- [ ] `ouaricon_add_module` for grain-engine, reverb-unit, preset-manager, eb-garamond (pitch-collections once extracted)
- [ ] ONE `juce_add_binary_data` target, `NAMESPACE UIBinaryData`, `HEADER_NAME UIBinaryData.h`, including `Source/ui/public/js/i18n.js`
- [ ] `juce::juce_gui_extra` linked; `PLUGIN_CODE OuGn` still unused (`grep -h PLUGIN_CODE plugins/*/CMakeLists.txt`)

## 4. Build and test (Debug)
- [ ] Build succeeds without warnings
- [ ] Standalone loads the WebView (not blank); right-click → Inspect works
- [ ] Console shows no JavaScript errors (a module-evaluation throw kills the whole UI silently — pattern_module_toplevel_init_tdz)
- [ ] `window.__JUCE__` exists; every knob shows a value (no `—` left after load)

## 5. Build and test (Release)
- [ ] Release build succeeds
- [ ] No crash on editor close/reopen ×10 and on plugin reload ×10 (member order)

## 6. Test parameter binding
- [ ] All 220 parameters sync UI ↔ APVTS (generic editor side by side, or a param-dump diff)
- [ ] Skewed readouts are right at the default: Size `100 ms`, Density `10 gr/s`, Cutoff `20.0 kHz`, Ring `200 Hz`, FB Low Cut `80 Hz`, FB High Cut `12.0 kHz`, LFO Rate `0.50 Hz`, Length `8.0 s`, Env Attack `10.0 ms`
- [ ] Automation and preset recall update the UI, including the scale chips, the custom-scale name and the editor (scale revision re-pull)
- [ ] Sync = Tempo swaps Size / Density / LFO Rate for their division knobs; dragging a mod chip onto a division knob routes the Free-mode destination
- [ ] Drag a source chip onto a knob → first empty slot gets source / destination / +40 %, the ring and the row flash; Alt-drag edits that depth; slot-number click clears
- [ ] Values and scale state persist across save → reopen → change → save → reopen (TWO reopens — critical_preset_manager_stale_customstate_child)

## 7. WebView-specific validation
- [ ] No viewport units in the page (`100vh`, `100vw`, `dvh`, `svh`) — verified 0
- [ ] `user-select: none` on body; context menu and dragstart disabled in JS
- [ ] Resource provider returns every file (no 404s in the Inspector Network tab); MIME types as in `getResource`
- [ ] Canvas text appears in EB Garamond, not the fallback (the page waits on `document.fonts.load` before the first paint)

## 8. UI gates
Run from the repo root. ALL must pass before the Stage 3 commit.

- [ ] `node scripts/check-i18n.js --plugin O-Granulator` — exit code = number of failed assertions [1]–[16]; 0 = pass
- [ ] `node scripts/i18n-fr-lint.js --plugin O-Granulator` — exit 2 on any finding; 0 = clean
- [ ] `node scripts/i18n-zh-lint.js --plugin O-Granulator` — exit 2 on any finding; 0 = clean (entries at `reviewed: 'mt'` are counted, not failed)
- [ ] `node scripts/check-ui-labels.js --plugin O-Granulator` — needs Playwright; 0 = pass, n > 0 = n assertions failed
- [ ] `node scripts/boot-all-uis.js --plugin O-Granulator --strict-tips` — needs Playwright; 0 = run completed (read the table), 2 = a DEAD tip binding

**Exit 77 from check-ui-labels or boot-all-uis means Playwright could not be resolved and NOTHING was verified. It is not a pass.** Install it (`npx playwright install chromium`) and re-run.

The first three also run in CI on every push to main (`.github/workflows/ui-static-gates.yml`), so a failure there turns main red.

**Coverage hole to close in Stage 3:** check-ui-labels measures only what is visible at load — the STREAMS page. FX and MOD are `hidden` tabs, and the scale list, custom-scale editor, settings popover and Tempo-mode division knobs only appear on interaction (339 of 472 `[data-i18n]` nodes unmeasured at load). Add `plugins/O-Granulator/tests/i18n-states.json` driving: each tab, Sync = Tempo, the scale list open, the custom editor open, the settings popover open. (All six were measured clean on the scratch tree by forcing them visible.)

## 9. Hands-on (Standalone or DAW)
- [ ] Tab reaches every knob, and the arrow keys move it (division knobs step one division)
- [ ] Drag a knob out of the window and release outside: the gesture ends and the knob stops following the cursor
- [ ] Double-click resets each knob to its default (including the division knobs and the matrix depth bars → 0 %)
- [ ] The hover-help switch toggles tooltips, and its state persists across closing and reopening the editor
- [ ] Switching the language to fr and to zh-Hans relabels the page with no clipping — all three tabs, the scale list and the custom-scale editor
- [ ] With macOS Reduce Motion on (System Settings → Accessibility → Display), no animation runs (knob hover scale, drop flash, matrix row flash, toggles, tooltip fade)
- [ ] Scales: grouped list opens under the trigger, arrows / Home / End / Enter / Escape work, outside press closes it; Custom opens the editor; chips toggle and paint-drag; All / None / Invert; 53-EDO shows 2 rows; Load .scl fills a draft, Apply commits, Save .scl writes a valid file
- [ ] Freeze: the strip outline turns ochre, the head reads FROZEN, "writing stopped · N s held" appears
- [ ] Frame is fixed (not resizable) — nothing to drag-resize

## Parameter list (from parameter-spec.md)

220 parameters. Relay type per parameter (the editor's three tables are in this order):

| Relay → attachment | Count | IDs |
|--------------------|-------|-----|
| `WebSliderRelay` → `WebSliderParameterAttachment` | 140 | `input_gain`, `buffer_length`, `mix`, `output_gain`; per stream ×4: `sN_level`, `sN_pan`, `sN_position`, `sN_spray`, `sN_size`, `sN_size_rand`, `sN_density`, `sN_jitter`, `sN_pitch`, `sN_pitch_rand`, `sN_reverse_prob`, `sN_window`, `sN_window_skew`, `sN_stereo_spread`, `sN_gain_rand`, `sN_filter_cutoff`, `sN_filter_rand`, `sN_filter_res`, `sN_drive`, `sN_crush`, `sN_crush_prob`, `sN_ring_freq`, `sN_ring_rand`, `sN_ring_mix`; `feedback`, `fb_low_cut`, `fb_high_cut`, `fb_saturation`, `fb_diffusion`, `reverb_mix`, `reverb_size`, `reverb_damp`; ×4: `lfoN_rate`, `lfoN_phase`; `env_attack`, `env_release`, `env_gain`, `onset_threshold`, `macro1`…`macro4`; ×16: `modM_depth` |
| `WebComboBoxRelay` → `WebComboBoxParameterAttachment` | 59 | `sync_mode`, `scale_root`, `grain_limit`; ×4: `sN_size_sync`, `sN_density_sync`, `sN_scale`, `sN_filter_type`; ×4: `lfoN_shape`, `lfoN_rate_sync`; ×16: `modM_source`, `modM_dest` |
| `WebToggleButtonRelay` → `WebToggleButtonParameterAttachment` | 21 | `freeze`; ×4: `sN_enable`, `sN_mute`, `sN_solo`, `sN_onset_trigger`; `pitch_follow_s1`…`pitch_follow_s4` |

UI presentation: 124 Family A knobs (`bindKnob`), 16 matrix depth bars (native range over the normalised domain, `bindFader` — the design draws a bipolar bar in a 26 px row, the one sanctioned non-knob continuous control), 12 stepped division knobs over a ComboBoxState (`bindChoiceKnob`, same capture / keyboard / wheel / dblclick lifecycle), 9 authored segment rows (`sync_mode`, `sN_filter_type`, `lfoN_shape`), 2 + 32 `<select>`s (`scale_root`, `grain_limit`, matrix source / destination), 4 grouped scale triggers, 21 toggles.
