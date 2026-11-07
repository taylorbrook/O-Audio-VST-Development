# Stage 3 (GUI) Integration Checklist — v1

**Plugin:** O-GranulatorInstrument
**Mockup Version:** v1 (finalized 2026-11-07T19:40:17Z, `v1-ui.yaml` `finalized: true`)
**Generated:** 2026-11-07 by ui-finalization-agent
**Parameter contract:** `../parameter-spec.md` (LOCKED, 261 parameters — the Stage 1 contract)
**Parent:** O-Granulator v2 (`plugins/O-Granulator/.planning/mockups/v2-*`) — the knob, scale, preset, tooltip and i18n machinery is copied verbatim from it.

Files in this directory:

| File | Goes to | What it is |
|------|---------|------------|
| `v1-ui.html` | `Source/ui/public/index.html` | The complete production page: inline `<style>` + one inline `<script type="module">` controller (Family A knobs, canonical i18n, hover help, 4 pages, lanes, Record-in, VOICE page). |
| `v1-i18n.js` | `Source/ui/public/js/i18n.js` | The en / fr / zh-Hans table module (exports only). 135 I18N + 319 LABELS + 446 TIP_BINDINGS. Entries shared unchanged with O-Granulator v2 keep its text; all other fr `reviewed: false`, zh-Hans `reviewed: 'mt'`. |
| `v1-PluginEditor.h` / `.cpp` | `Source/PluginEditor.h` / `.cpp` | Editor TEMPLATE: 261 relays/attachments (176 slider, 70 combo, 15 toggle), 34 native functions, resource provider, Windows WebView2 block, drag-drop sessions. Adapt names to the Stage 1/2 processor; the shapes are the contract. |
| `v1-CMakeLists.txt` | `CMakeLists.txt` | The COMPLETE plugin CMakeLists (Stage 1 lands it; Stage 3 verifies the WebView block). |
| `v1-i18n-states.json` | `tests/i18n-states.json` | The check-ui-labels state sweep (16 states: every page, Free/Tempo, scale list, custom editor, settings, loaded-lane badges, drag-over, the longest Record-in states). |

Pre-verified on a scratch tree built from exactly these files (plugin dir = the CMake + editor + `Source/ui/public` + the state sweep; `scripts/` copied, `modules/` linked):
`check-i18n` ALL PASS · `i18n-fr-lint` 0 findings · `i18n-zh-lint` 0 findings (453 at `'mt'`) ·
`check-ui-labels` 0 failures over the default state + 16 states (473 of 605 `[data-i18n]` measured) ·
`boot-all-uis --strict-tips` clean, 0 DEAD / 0 late bindings ·
native-function parity 34 = 34 (page + preset-manager.js + webview-drop-streaming.js vs `withNativeFunction`).
A Playwright probe with a rich telemetry fixture also rendered all four pages in en and fr with no page error, ended a knob drag released outside the window, and moved a loop handle. Re-run every gate on the real tree (section 8): the real processor and real data are what Stage 3 ships.

## 1. Copy UI files
- [ ] Copy `v1-ui.html` to `Source/ui/public/index.html`
- [ ] Copy `v1-i18n.js` to `Source/ui/public/js/i18n.js`
- [ ] Copy the JUCE frontend library to `Source/ui/public/js/juce/index.js` and `check_native_interop.js` (from a working plugin, e.g. `git show HEAD:plugins/O-Granulator/Source/ui/public/js/juce/index.js`)
- [ ] Copy `v1-i18n-states.json` to `tests/i18n-states.json`
- [ ] Do NOT copy `preset-manager.js`, `webview-drop-streaming.js` or the EB Garamond files into `Source/ui/public/` — CMake embeds them from `modules/` and `getResource` serves them at `/js/preset-manager.js`, `/js/webview-drop-streaming.js`, `/css/eb-garamond.css`, `/fonts/*.woff2`. (`ouaricon_add_module` also copies module JS to `Source/ui/public/modules/`: a gitignored build artefact, never committed — pattern_module_js_move_build_recopies_tracked_file.)
- [x] **Settings popover** (`#gear-btn` + `#settings-popover` with `#lang-select` and `#tips-toggle`) — the mockup had none, so it is placed exactly where the parent's was accepted (O-Granulator CONTEXT-3.1 D28): top-right at (1124, 62), 24 × 24, under the OUTPUT knob's text column, opening down-left over the lane strip's globals column. Designer: confirm it still reads well beside the 604 px source dock.

## 2. Update PluginEditor and processor files
- [ ] Replace `PluginEditor.h` / `.cpp` with the v1 templates; rename `GranulatorInstrumentEditor` / `GranulatorInstrumentProcessor` to the Stage 1 class names
- [ ] Verify member order: relays (3 vectors) → `dropSessions` → `webView` → attachments (3 vectors) → `webView2RuntimeLink`
- [ ] Verify the constructor creates them in the same order (relays → drop sessions → options → `webView` → attachments)
- [ ] `kSliderIds` (176) / `kComboIds` (70) / `kToggleIds` (15) equal `createParameterLayout()` by TYPE as well as by ID — a Bool on a slider relay, or a Choice on a slider relay, attaches cleanly and never updates. Diff both directions against parameter-spec.md "Full ID list".
- [ ] Native functions registered — exactly 34, and the page calls no other:
  `getParameterDefaults`, `getUiLanguage`, `setUiLanguage`, `getTelemetry`, `getWindowCurve`,
  `getScaleState`, `setDegreeMask`, `setCustomScale`, `parseScaleText`, `loadScala`, `saveScala`,
  `getSlotState`, `getSlotOverview`, `loadSlot`, `relinkSlot`, `clearSlot`, `resetSlot`, `resetAll`, `setSlotEmbed`, `saveRecording`,
  `dropSessionStart`, `dropSessionAddFile`, `dropSessionCommitFolder`, `dropSessionCommitFile` (webview-drop-streaming 1.0.0),
  `savePreset`, `loadPreset`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`, `savePresetWithDialog`, `loadPresetFromFile`.
  Diff `grep -o "getNativeFunction('[A-Za-z]*')" index.html` + the page's `callNative('…')` / `slotAction('…')` names + preset-manager.js + webview-drop-streaming.js against `withNativeFunction ("…"` — an unregistered fn hangs its promise silently (pattern_webview_native_fn_bridge_gap).
  - Naming note: the invocation said `resetAll` / `setEmbed`, v1-ui.yaml `state.native_functions` said `resetBuffers` / `setSlotEmbed` / `chooseFileForSlot`. The template and the page settle on **`resetAll`**, **`setSlotEmbed`**, and a single **`loadSlot`** that opens the chooser (a page cannot hand C++ a path under WKWebView; drops go through the drop-session functions). **Record arm is the `rec_arm` parameter**, not a native function.
- [ ] `getParameterDefaults` reports every Float AND every Choice (choice = index) — the stepped knobs, root fields and loop handles reset through it
- [ ] `getResource` serves `/js/i18n.js` as `application/javascript; charset=utf-8`; every `juce_add_binary_data` source has a branch and vice versa (index.html, i18n.js, juce/index.js, juce/check_native_interop.js, preset-manager.js, webview-drop-streaming.js, eb-garamond.css, 3 × woff2); bare-path comparisons only
- [ ] Windows block kept exactly: `withBackend (webview2)`, user-data folder `%APPDATA%/Ouaricon/WebView2/O-GranulatorInstrument` created first, `areOptionsSupported()` probe, `webView` null on failure + native notice; every `webView->` use outside the constructor null-guarded; never `setVisible (false)` on the WebView (critical_webview_completion_gated_on_isvisible)
- [ ] Window: fixed 1200 × 800, `setResizable (false, false)` (ui-design-rules Rule 4 — fixed, exactly 800 tall). The page's `body` and `#plugin` are 1200 × 800 px; keep all three in sync
- [ ] **Processor — buses (ARCHITECTURE §1, D6):** stereo output + ONE optional stereo "Sidechain" input, active by default; VST3 `getPluginHasMainInput() == false` (in the note-expression `VST3Extensions` subclass, D5) so it is `kAux`; channel counts from the BUS, never `buffer.getNumChannels()`; the output is cleared before rendering (the sidechain never reaches the output). It must degrade cleanly: no bus / disabled / 0 ch → `getTelemetry.rec.bus = 0` → the page shows "No sidechain" and the cluster goes inert; bus present but silent → `rec.signal = false` → "No signal", and a capture with peak < −90 dBFS is discarded (`rec.run = 5`), slot unchanged (critical_au_hands_unrouted_sidechain_silence)
- [ ] **Processor — UI language:** `std::atomic<int> uiLanguage { 0 };` + `static juce::String languageCode (int)` (1 → "fr", 2 → "zh-Hans", else "en") + `static int languageIndex (const juce::String&)` (unknown → 0); persisted as a non-parameter `uiLanguage` property on `parameters.state`, written in `getStateInformation` BEFORE the XML copy and restored in `setStateInformation` only when present (O-ReverseDelay / O-Granulator are the reference; O-Granulator stores the code string and reads tolerantly)
- [ ] **Processor — scale STATE** (inherited from O-Granulator, unchanged): `getCollectionTable`, `getCustomScale`, `getDegreeMask` / `setDegreeMask`, `setCustomScale`, `getScaleStateRevision` (bumped by the setters, `setStateInformation` and preset apply); masks as 16-hex-digit strings; presets carry custom scale + masks; reset to defaults before apply; strip the stale `<CustomState>` child (critical_preset_manager_stale_customstate_child). pitch-collections 1.0.0 is a module — never fork it.
- [ ] **Processor — slot STATE + actions** (parameter-spec.md "Plugin state & native functions", S9–S11), message thread, never the audio thread:
  - `SlotInfo getSlotInfo (int slot)` → `{ scheme ("" | "factory" | "file" | "rec"), factoryId, name, path, seconds, sourceSeconds, trimmed, missing, embed, encodedBytes, dirty }`
  - `std::uint32_t getSlotStateRevision()` — bumped on load / clear / relink / record commit / embed change / reset-done / `setStateInformation` / preset apply. The page re-pulls `getSlotState` whenever `getTelemetry.slotRev` moves
  - `bool getSlotOverview (slot, float* peaks, uint8* dirty, 656, uint32& rev)` — peaks of the WORKING COPY (0..1), `dirty` = how far feedback moved each bin (0..255); `rev` rides in `getTelemetry.lanes[i].ovRev` and the page refetches at ≤ 10 Hz
  - `loadSlotFromFile`, `relinkSlot` (matches `fileSize` / `contentHash`), `clearSlot` (refused while that slot records), `resetSlot`, `resetAllSlots`, `setSlotEmbed` (`"tooLarge"` above 16 MB FLAC-24, `"factory"`, `"empty"`), `saveRecordingTo` — all enqueue loader jobs; nothing decodes inline
  - Drag-drop: `SessionManager::Config::onCommitFile (file, midi = slot, vel)` → `loadSlotFromFile`; `tempDirPrefix = "ogranulatorinstrument-drop-"`
- [ ] **Processor — live data** for `getTelemetry` (a PULL at ~30 Hz; no juce::Timer, no emitEvent): `bool readTelemetry (Telemetry&)` (seqlock or try-read; false on a torn read — the page keeps its last frame). Fields the editor reads:
  `frozen`; `lanes[4]` `{ overviewRev, fbHead (0..1 of the file, < 0 = none), recHead (< 0 = none), regionLo, regionHi }` (the MODULATED loop region);
  `voices[16]` `{ state 0 off · 1 held · 2 releasing · 3 stolen, note, amp, scan[4] (0..1, < 0 = none), wantSt (pre-clamp semitones on the first enabled stream), menv[2], menvStage[2], ampStage }` + `newestVoice` (−1 = none);
  `lfoGlobalPhase[4]`, `lfoVoicePhase[4][16]` (< 0 = voice off);
  `grains[≤ 288]` `{ stream 1–4, normPos 0..1, pan, env, gain, lengthSec, voice, flags (1 reversed | 2 clamped) }` + `grainCount`, `counts[4]`, `limit`;
  `outPeakL/R` (consuming); `rec { busPresent, signal, run (0 idle · 1 armed · 2 recording · 3 committing · 4 done · 5 discarded), slot, pos, lengthSec, capped, scPeakL/R (consuming, post rec_gain) }`;
  `sources[17]` (modM_source order, per-voice ones from the newest voice); `modNorm[126]` + `modRouted[126]`.
  Never let the editor touch the pool, the voices or the tapes directly.
- [ ] **Processor — window figure:** `void getWindowCurve (int stream, float* out, int numPoints) const` from the engine's WindowMorphBank (never a JS model).
- [ ] `modules/core/webview-relay-manager` is deliberately NOT used: no `WebComboBoxRelay` (70 of 261 are choices) and no WebView2 handling.

## 3. Update CMakeLists.txt
- [ ] `v1-CMakeLists.txt` is the whole file (not a snippet). Land it as `plugins/O-GranulatorInstrument/CMakeLists.txt`
- [ ] `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_CURL=0`
- [ ] `juce_generate_juce_header (OuariconGranulatorInstrument)`; `target_compile_features (... cxx_std_20)`
- [ ] `ouaricon_add_module` for grain-engine, pitch-collections, reverb-unit, preset-manager, eb-garamond, note-expression, webview-drop-streaming
- [ ] ONE UI `juce_add_binary_data` target, `NAMESPACE UIBinaryData`, `HEADER_NAME UIBinaryData.h`, including `Source/ui/public/js/i18n.js`; the factory-source target (S10) gets its OWN namespace (`FactorySourceData`) when Stage 4 adds the FLAC files
- [ ] `juce::juce_gui_extra` linked; `PLUGIN_CODE OuGI` still unused (`grep -h PLUGIN_CODE plugins/*/CMakeLists.txt`)
- [ ] No comment in CMakeLists.txt spells the plugin-target call with its opening parenthesis (resolve-target.sh — pattern_resolve_target_matches_juce_add_plugin_in_comment)

## 4. Build and test (Debug)
- [ ] Build succeeds without warnings
- [ ] Standalone loads the WebView (not blank); right-click → Inspect works
- [ ] Console shows no JavaScript errors (a module-evaluation throw kills the whole UI silently — pattern_module_toplevel_init_tdz)
- [ ] `window.__JUCE__` exists; every knob and field shows a value (no `—` left after load)

## 5. Build and test (Release)
- [ ] Release build succeeds
- [ ] No crash on editor close/reopen ×10 and on plugin reload ×10 (member order)
- [ ] pluginval 10 on VST3 and AU with the sidechain bus present (an input bus on an instrument — juce8-critical-patterns §4)

## 6. Test parameter binding
- [ ] All 261 parameters sync UI ↔ APVTS (generic editor side by side, or a param-dump diff)
- [ ] Skewed readouts are right at the default: Amp Attack `20.0 ms`, Amp Release `1.50 s`, Glide `0.0 ms`, Size `100 ms`, Density `10 gr/s`, Cutoff `20.0 kHz`, Ring `200 Hz`, FB Low Cut `80 Hz`, FB High Cut `12.0 kHz`, LFO Rate `0.50 Hz`, Record Length `4.0 s`; Root `C3` / `0 ct`
- [ ] Automation and preset recall update the UI, including scale chips, lane names/badges (slot revision re-pull) and the loop handles
- [ ] Sync = Tempo swaps Size / Density / LFO Rate / Record Length for their division knobs; dragging a mod chip onto a division knob routes the Free-mode destination
- [ ] Drag a source chip onto a knob or a lane loop handle → first empty slot gets source / destination / +40 %, the ring or handle and the row flash; a per-voice source on a global destination toasts "mono" and its row shows the ochre `mono` badge; LFO N's badge follows `lfoN_mode`
- [ ] Values, scale state and slot sources persist across save → reopen → change → save → reopen (TWO reopens — critical_preset_manager_stale_customstate_child)

## 7. WebView-specific validation
- [ ] No viewport units in the page (`100vh`, `100vw`, `dvh`, `svh`) — verified 0
- [ ] `user-select: none` on body; context menu and dragstart disabled in JS
- [ ] Resource provider returns every file (no 404s in the Inspector Network tab); MIME types as in `getResource`
- [ ] Canvas text appears in EB Garamond, not the fallback (the page waits on `document.fonts.load` before the first paint)
- [ ] Drop a WAV on a lane in Logic / Live (macOS WKWebView): it streams through the drop session and loads; a folder or a drop outside a lane is refused with a toast

## 8. UI gates
Run from the repo root. ALL must pass before the Stage 3 commit.

- [ ] `node scripts/check-i18n.js --plugin O-GranulatorInstrument` — exit code = number of failed assertions [1]–[16]; 0 = pass
- [ ] `node scripts/i18n-fr-lint.js --plugin O-GranulatorInstrument` — exit 2 on any finding; 0 = clean
- [ ] `node scripts/i18n-zh-lint.js --plugin O-GranulatorInstrument` — exit 2 on any finding; 0 = clean (entries at `reviewed: 'mt'` are counted, not failed)
- [ ] `node scripts/check-ui-labels.js --plugin O-GranulatorInstrument` — needs Playwright; 0 = pass, n > 0 = n assertions failed
- [ ] `node scripts/boot-all-uis.js --plugin O-GranulatorInstrument --strict-tips` — needs Playwright; 0 = run completed (read the table), 2 = a DEAD tip binding

**Exit 77 from check-ui-labels or boot-all-uis means Playwright could not be resolved and NOTHING was verified. It is not a pass.** Install it (`npx playwright install chromium`) and re-run.

The first three also run in CI on every push to main (`.github/workflows/ui-static-gates.yml`), so a failure there turns main red.

**Coverage hole left for Stage 3:** 132 `[data-i18n]` elements stay unmeasured even with the 16-state sweep — mostly toasts, the per-voice cell states (`held` / `release` / `stolen` / `clamp`), the remaining Record-in run states and the scale editor's error lines. Drive them with real telemetry once the processor exists, or add stub-overrides states.

## 9. Hands-on (Standalone or DAW)
- [ ] Tab reaches every knob, field, loop handle and record pip, and the arrow keys move it
- [ ] Drag a knob out of the window and release outside: the gesture ends and the knob stops following the cursor (same for a loop handle and a root field)
- [ ] Double-click resets each knob, field and loop handle to its default (including the division knobs and the matrix depth bars → 0 %)
- [ ] The hover-help switch toggles tooltips, and its state persists across closing and reopening the editor
- [ ] Switching the language to fr and to zh-Hans relabels the page with no clipping — all four tabs, the lane badges, Record-in states, the scale list and the custom-scale editor
- [ ] With macOS Reduce Motion on (System Settings → Accessibility → Display), no animation runs (knob hover scale, drop flash, handle flash, matrix row flash, armed lamp blink, toggles, tooltip fade)
- [ ] Lanes: empty → "Drop a file or Record" (Record targets the lane and arms); factory / rec / save… / trimmed / relink badges; Embed tool disabled for factory, warns above 16 MB; Reset dot marks a dirty copy; Reset all clears every dot
- [ ] Record-in: no sidechain (AU host that never routes one) → "No sidechain", cluster dimmed and inert; routed but silent → "No signal", a Manual capture is discarded; Threshold waits with the ochre tick on the input meter; Tempo + 32 bars at a slow tempo reads "capped 30 s"
- [ ] Freeze: lane record heads turn dashed and stop; Write amount reads "Held · frozen · no writes"
- [ ] VOICE: header dots and the 16 cells track held / releasing / stolen voices; a note past ±48 st reads "clamp" in warning tone; the newest voice carries the ochre rule and drives the MOD page's per-voice LFO dots
- [ ] Frame is fixed (not resizable) — nothing to drag-resize

## Parameter list (from parameter-spec.md)

261 parameters. Relay type per parameter (the editor's three tables are in registration order):

| Relay → attachment | Count | IDs |
|--------------------|-------|-----|
| `WebSliderRelay` → `WebSliderParameterAttachment` | 176 | `polyphony`, `glide`, `pitch_bend_range`, `output_gain`; `amp_attack`, `amp_decay`, `amp_sustain`, `amp_release`, `vel_to_amp`; ×2: `menvN_attack`, `menvN_decay`, `menvN_sustain`, `menvN_release`; per stream ×4: `sN_level`, `sN_pan`, `sN_position`, `sN_spray`, `sN_scan_rate`, `sN_loop_start`, `sN_loop_end`, `sN_root_note`, `sN_root_fine`, `sN_key_track`, `sN_size`, `sN_size_rand`, `sN_density`, `sN_jitter`, `sN_pitch`, `sN_pitch_rand`, `sN_reverse_prob`, `sN_window`, `sN_window_skew`, `sN_stereo_spread`, `sN_gain_rand`, `sN_filter_cutoff`, `sN_filter_rand`, `sN_filter_res`, `sN_drive`, `sN_crush`, `sN_crush_prob`, `sN_ring_freq`, `sN_ring_rand`, `sN_ring_mix`; `rec_threshold`, `rec_length`, `rec_gain`; `feedback`, `fb_low_cut`, `fb_high_cut`, `fb_saturation`, `fb_diffusion`; `reverb_mix`, `reverb_size`, `reverb_damp`; ×4: `lfoN_rate`, `lfoN_phase`; `macro1`…`macro4`; ×16: `modM_depth` |
| `WebComboBoxRelay` → `WebComboBoxParameterAttachment` | 70 | `voice_mode`, `grain_limit`, `sync_mode`; ×4: `sN_scan_mode`, `sN_size_sync`, `sN_density_sync`, `sN_scale`, `sN_filter_type`; `rec_target`, `rec_trigger`, `rec_length_sync`; ×4: `lfoN_shape`, `lfoN_rate_sync`, `lfoN_mode`; ×16: `modM_source`, `modM_dest` |
| `WebToggleButtonRelay` → `WebToggleButtonParameterAttachment` | 15 | `mpe_enable`, `freeze`; ×4: `sN_enable`, `sN_mute`, `sN_solo`; `rec_arm` |

UI presentation: 144 Family A knobs (`bindKnob`); 16 matrix depth bars (native range over the normalised domain, `bindFader`); 8 lane loop handles (`bindHandle`: horizontal capture drag over the lane, arrows, dblclick — the sanctioned non-knob continuous control for a region edge); 8 root value fields (`bindField`: vertical step drag, wheel, arrows, dblclick); 17 stepped knobs over a ComboBoxState (`bindChoiceKnob`: divisions, Record Length bars, Scan mode with localized faces); 15 authored segment rows (`sync_mode`, `voice_mode`, `rec_trigger`, `sN_filter_type`, `lfoN_shape`, `lfoN_mode`); 1 + 32 `<select>`s (`grain_limit`, matrix source / destination); 1 radio (`rec_target`, the lane record pips); 4 grouped scale triggers; 15 toggles.
