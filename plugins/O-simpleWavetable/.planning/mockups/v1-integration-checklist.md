# Stage 3 (GUI) Integration Checklist - v1

**Plugin:** O-simpleWavetable
**Mockup Version:** v1 (finalized 2026-10-06T02:25:53Z)
**Generated:** 2026-10-05
**Frame:** 1120 x 780, fixed (`ui-design-rules.md` Rule 4 — ≤ 800 px tall, no resize)

Files in this directory:

| File | Becomes |
|------|---------|
| `v1-ui.html` | `Source/ui/public/index.html` (inline style + inline module controller) |
| `v1-i18n.js` | `Source/ui/public/js/i18n.js` (en / fr / zh-Hans table) |
| `v1-PluginEditor.h` / `v1-PluginEditor.cpp` | `Source/PluginEditor.h` / `.cpp` (TEMPLATES — adapt, don't paste blind) |
| `v1-CMakeLists.txt` | merged into `plugins/O-simpleWavetable/CMakeLists.txt` |
| `v1-i18n-states.json` | `plugins/O-simpleWavetable/tests/i18n-states.json` (check-ui-labels states) |

Pre-verified at finalization on a fixture tree (these files placed at their Stage 3 paths, JUCE frontend + fonts + drop module copied in): `check-i18n` 16/16 PASS, `i18n-fr-lint` CLEAN, `i18n-zh-lint` 0 findings (142 entries at `reviewed: 'mt'`), `check-ui-labels` ALL PASS across 8 states, `boot-all-uis --strict-tips` clean boot with 0 DEAD and 0 late tip bindings. Re-run all five on the real tree — the fixture is not the build.

## 1. Copy UI Files
- [ ] Copy `v1-ui.html` to `Source/ui/public/index.html`
- [ ] Copy `v1-i18n.js` to `Source/ui/public/js/i18n.js`
- [ ] Copy the JUCE 8 frontend library to `Source/ui/public/js/juce/index.js` + `js/juce/check_native_interop.js` (from O-simpleAdditive)
- [ ] Copy `img/insects.png` to `Source/ui/public/img/insects.png` (the botanical overlay, from O-simpleAdditive)
- [ ] EB Garamond is NOT copied: CMake embeds it from `modules/ui/eb-garamond` (css + 3 woff2)
- [ ] `Source/ui/public/modules/webview-drop-streaming.js` is written by `ouaricon_add_module` at configure time (gitignored — do not commit it)
- [ ] Copy `v1-i18n-states.json` to `plugins/O-simpleWavetable/tests/i18n-states.json`
- [ ] Settings popover: the mockup's gear held hover help only; finalization added `#lang-select` (en / Français / 简体中文) as the first row, top-right of the header as the YAML specified

## 2. Update PluginEditor and Processor Files
- [ ] Replace `PluginEditor.h` with `v1-PluginEditor.h` content (class `OSimpleWavetableAudioProcessorEditor`, Timer 30 Hz)
- [ ] Verify member order: 21 relays → `webView` → 21 attachments
- [ ] Update the class / processor names if Stage 1 chose different ones
- [ ] Replace `PluginEditor.cpp` with `v1-PluginEditor.cpp` content
- [ ] Verify initialization order matches declaration order (relays in the ctor init list, webView then attachments in the body)
- [ ] `.withOptionsFrom()` for ALL 21 relays — 13 slider, 6 combo, **2 toggle** (`interp`, `bandlimit`). A toggle relay left out = `getToggleState()` bound to a state nothing updates
- [ ] Native functions registered — every name the page calls (grep `getNativeFunction(` in index.html and diff):
  `getParameterDefaults`, `getUiLanguage`, `setUiLanguage`, `uiReady`, `importAudio`, `importDroppedAudio`, `uiMidi`, `applyFactoryPreset`.
  An unregistered one never settles its promise; its control is silently dead while build, auval and pluginval pass.
- [ ] `getResource` serves (bare paths, `charset=utf-8` on text): `/`, `/index.html`, `/js/i18n.js` (`application/javascript; charset=utf-8`), `/js/juce/index.js`, `/js/juce/check_native_interop.js`, `/modules/webview-drop-streaming.js`, `/img/insects.png`, `/css/eb-garamond.css`, `/fonts/EBGaramond-{Regular,Italic,Bold}.woff2`
- [ ] Windows: `withWinWebView2Options` with `withUserDataFolder(tempDirectory/OsimpleWavetable_WebView)` — always, not conditionally
- [ ] `importAudio`: `complete()` is called BEFORE `launchAsync`; the chooser completion holds a `SafePointer` and on a null editor returns WITHOUT touching `complete` (pattern_webview_launchasync_safepointer_no_complete)
- [ ] `importDroppedAudio`: decode with `juce::Base64::convertFromBase64` (NOT `MemoryBlock::fromBase64Encoding`); size-capped (96 MB file, matching `DROP_MAX_BYTES` in the page)
- [ ] Processor: `std::atomic<int> uiLanguage { 0 }` + `languageCode(int)` (1 → "fr", 2 → "zh-Hans", else "en") / `languageIndex(const String&)` (unknown → 0); persisted as a `uiLanguage` property on `parameters.state` in `getStateInformation` (before the XML copy) and restored in `setStateInformation` only when present (O-simpleAdditive / O-ReverseDelay are the reference)
- [ ] Processor display/import API the editor template calls (names are proposals — keep the shapes; full list in the `v1-PluginEditor.cpp` header):
  - `handleUiMidi(note, isOn, vel)` via `MidiMessageCollector`
  - `applyFactoryPreset(lessonId)` for the five lesson ids below — reset to defaults first, never set `output_level`
  - `importFromFile(File)`, `importFromMemory(name, MemoryBlock&&)`, `getImportStatus()`, `getImportStatusVersion()` (Phase 2.4)
  - `getBankDisplayGeneration()`, `getBankThumbnails(BankThumbs&)` — N × 128 level-0 points (Phase 3.2)
  - `buildCycleView(CycleView&)` — the exact 2048-sample cycle rendered on the message thread with the voice's read code, quantized, FFT'd, point-sampled to 256 (Phase 3.2)
- [ ] Window size: `setSize (1120, 780)` + `setResizable (false, false)` (Rule 4)

### Event contract (C++ → page)

| Event | When | Payload |
|-------|------|---------|
| `bankUpdate` | bank param change, import publish, state restore, `uiReady` | `{ bank, imported, numFrames, filename, frames: [[128 floats] × N] }` (a flat `N*128` array is accepted too) |
| `cycleUpdate` | 30 Hz timer, hash-gated, forced after `uiReady` / bank change | `{ cycle[256], harmonics[32] (dB re heard max, −60 floor), pos, frame (latched, −1 when Interp On), level, sounding }` + optional `note`, `f0`, `kmax`, `nyquistH`, `harmonicsRaw[32]`, `preQ[256]` (only when quantized), `lfo` (−1..1), `menv`, `amp` (0..1) |
| `importStatus` | every importer transition, `uiReady` | `{ state: 'idle'\|'busy'\|'done'\|'error', filename, frames, error }`; error codes the page localizes: `tooShort`, `unreadable`, `tooLarge` (anything else → generic) |

The page calls `uiReady` once its listeners exist and again on every `visibilitychange` → visible, because `emitEventIfBrowserIsVisible` drops events while the browser is hidden.

### Lesson preset ids (`applyFactoryPreset`)

| id | Button | Recipe (from the v1 mockup; Stage 4 may refine per ARCHITECTURE §A9) | UI side |
|----|--------|------|------|
| `steppedSmooth` | Stepped vs Smooth | Sine→Saw, Pos 0.5, Interp Off, LFO Free Triangle 0.18 Hz depth 1.0 | — |
| `aliasDemo` | Alias Demo | Sine→Saw, Pos 1.0, Band-limit Off | keyboard jumps to octave 6 |
| `driveSweep` | Drive Sweep | Drive, Pos 0, Env Amount +1.0, menv A 0.9 D 1.6 S 0.25 R 0.8 | — |
| `vowelPad` | Vowel Pad | Formant, Pos 0.5, LFO Free Sine 0.12 Hz depth 0.9, amp A 0.6 R 1.4 | — |
| `ppg8bit` | 8-bit PPG | Sine→Square, Pos 0.6, Interp Off, Bit Depth 8 (index 9), LFO Free S&H 3 Hz depth 0.4 | — |

## 3. Update CMakeLists.txt
- [ ] Merge `v1-CMakeLists.txt` into `CMakeLists.txt` (`ouaricon_add_module(O-simpleWavetable webview-drop-streaming)`, the `O-simpleWavetable_UIResources` binary-data target, the link line)
- [ ] Verify `juce_add_binary_data` lists all UI files, including `Source/ui/public/js/i18n.js`
- [ ] Verify `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`
- [ ] Verify `juce::juce_gui_extra` linked; `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`
- [ ] BinaryData symbols strip hyphens: `webviewdropstreaming_js`, `ebgaramond_css`

## 4. Build and Test (Debug)
- [ ] Build succeeds without warnings
- [ ] Standalone loads the WebView (not blank)
- [ ] Right-click → Inspect works (the page itself disables the context menu)
- [ ] Console shows no JavaScript errors
- [ ] `window.__JUCE__` object exists

## 5. Build and Test (Release)
- [ ] Release build succeeds
- [ ] No crashes on plugin reload (test 10 times) — exercises the member order
- [ ] Close the editor while the Import file dialog is open → no crash

## 6. Test Parameter Binding
- [ ] All 21 parameters sync UI ↔ APVTS (13 knobs incl. the hero, 2 stepped knobs, 2 bool segment pairs, 3 choice segment rows, the bank select)
- [ ] Bank-stack drag and wheel move `position` with one automation gesture per drag / wheel burst
- [ ] Rate ↔ Division swap on `lfo_sync`; the Division pips appear when first shown
- [ ] Automation updates UI; preset recall updates UI; values persist after reload
- [ ] Readouts match the host's parameter text (`-inf` at the output floor, `Full` at bit depth 0)

## 7. WebView-Specific Validation
- [ ] No viewport units in CSS (`100vh`, `100vw`)
- [ ] Native feel CSS present (`user-select: none`)
- [ ] Resource provider returns all files (no 404s) — including the fonts and the drop module
- [ ] Correct MIME types for all resources
- [ ] Hide and re-show the editor (minimise / switch tracks): the bank stack and the source line come back (the `uiReady` re-send)

## 8. UI gates
Run from the repo root. ALL must pass before the Stage 3 commit.

- [ ] `node scripts/check-i18n.js --plugin O-simpleWavetable` — exit code = number of failed assertions [1]–[16]; 0 = pass
- [ ] `node scripts/i18n-fr-lint.js --plugin O-simpleWavetable` — exit 2 on any finding; 0 = clean
- [ ] `node scripts/i18n-zh-lint.js --plugin O-simpleWavetable` — exit 2 on any finding; 0 = clean (entries at `reviewed: 'mt'` are counted, not failed)
- [ ] `node scripts/check-ui-labels.js --plugin O-simpleWavetable` — needs Playwright; 0 = pass, n > 0 = n assertions failed
- [ ] `node scripts/boot-all-uis.js --plugin O-simpleWavetable --strict-tips` — needs Playwright; 0 = run completed (read the table), 2 = a DEAD tip binding

**Exit 77 from check-ui-labels or boot-all-uis means Playwright could not be resolved and NOTHING was verified. It is not a pass.** Install it (`npx playwright install chromium`) and re-run. (At finalization the newest npx-cached Playwright wanted chromium-headless-shell 1243, which was not installed; the gates ran against the cached 1.62.1 / chromium 1234 instead.)

The first three also run in CI on every push to main (`.github/workflows/ui-static-gates.yml`), so a failure there turns main red.

## 9. Hands-on (Standalone or DAW)
- [ ] Tab reaches every knob (incl. Bit Depth and Division), and the arrow keys move it
- [ ] Drag a knob out of the window and release outside: the gesture ends and the knob stops following the cursor (same for the bank-stack drag)
- [ ] Double-click resets each knob to its default (stepped knobs to their default index)
- [ ] The hover-help switch toggles tooltips, and its state persists across closing and reopening the editor (`oswt.tipsEnabled`)
- [ ] Switching the language to fr and to zh-Hans relabels the page with no clipping
- [ ] With macOS Reduce Motion on (System Settings → Accessibility → Display), no animation runs (the empty-Imported Import button shows a static amber ring instead of the pulse)
- [ ] Import button → file dialog → `Slicing…` → bank switches to Imported with the filename and frame count
- [ ] Drop a WAV onto the plugin (Standalone, macOS): same result; drop a folder or a `.txt` → the localized refusal on the source line
- [ ] Bank = Imported with nothing imported: dashed hollow stack, "Import audio to fill this bank", amber Import button, silence
- [ ] Hold a high note with Band-limiting On / Off: the harmonics panel shows the octave ceiling / the rust aliasing bars
- [ ] On-screen keyboard and computer keys A–K / W–U play; Z / X change octave; releasing the window focus releases held notes

## Parameter List (from parameter-spec.md — 21 parameters)

| ID | Type | Relay | Attachment | UI |
|----|------|-------|------------|----|
| `bank` | Choice | WebComboBoxRelay | WebComboBoxParameterAttachment | `<select>` |
| `position` | Float | WebSliderRelay | WebSliderParameterAttachment | hero knob + bank-stack drag |
| `interp` | Bool | WebToggleButtonRelay | WebToggleButtonParameterAttachment | Off \| On |
| `bandlimit` | Bool | WebToggleButtonRelay | WebToggleButtonParameterAttachment | Off \| On |
| `bit_depth` | Choice | WebComboBoxRelay | WebComboBoxParameterAttachment | stepped knob |
| `lfo_rate` | Float | WebSliderRelay | WebSliderParameterAttachment | knob |
| `lfo_sync` | Choice | WebComboBoxRelay | WebComboBoxParameterAttachment | Free \| Tempo |
| `lfo_div` | Choice | WebComboBoxRelay | WebComboBoxParameterAttachment | stepped knob |
| `lfo_shape` | Choice | WebComboBoxRelay | WebComboBoxParameterAttachment | 5 icon segments |
| `lfo_depth` | Float | WebSliderRelay | WebSliderParameterAttachment | knob |
| `menv_attack` | Float | WebSliderRelay | WebSliderParameterAttachment | knob |
| `menv_decay` | Float | WebSliderRelay | WebSliderParameterAttachment | knob |
| `menv_sustain` | Float | WebSliderRelay | WebSliderParameterAttachment | knob |
| `menv_release` | Float | WebSliderRelay | WebSliderParameterAttachment | knob |
| `env_amount` | Float | WebSliderRelay | WebSliderParameterAttachment | bipolar knob |
| `amp_attack` | Float | WebSliderRelay | WebSliderParameterAttachment | knob |
| `amp_decay` | Float | WebSliderRelay | WebSliderParameterAttachment | knob |
| `amp_sustain` | Float | WebSliderRelay | WebSliderParameterAttachment | knob |
| `amp_release` | Float | WebSliderRelay | WebSliderParameterAttachment | knob |
| `voice_mode` | Choice | WebComboBoxRelay | WebComboBoxParameterAttachment | Poly \| Mono |
| `output_level` | Float | WebSliderRelay | WebSliderParameterAttachment | knob |

## Notes for gui-agent

- **21, not 22.** Every planning doc says "22 parameters" but lists 21 IDs; parameter-spec.md explains. ROADMAP Stage 1's `jassert (params.size() == 22)` must be 21.
- **`getToggleState`, not `getToggleButtonState`.** `html-generation.md` names the latter; the JUCE 8 frontend (`js/juce/index.js`) exports `getToggleState`, which is what the page calls.
- **Stepped knobs are choices.** `bit_depth` and `lfo_div` look like Family A knobs but bind through `getComboBoxState`; their angle is `index / (count − 1)` from the live `properties.choices`, and dblclick resets to the index `getParameterDefaults` reports.
- **The Bank menu stays English** (its entries are the C++ option strings = automation names); the `lang-select` tooltip says so.
- **Layout pins for i18n.** `.title-block` (384 px), `.strip-label` (50 px), `.import-btn` (124 px), `.bank-key` (104 px), `.bank-readout` (300 px), `.tour-label` (100 px), `.tour-buttons` (458 px), `.harm-tr` (full width) are pinned so nothing moves between languages (check-ui-labels [7]); the LFO cells widened 58 → 78 px with unchanged knob centres, `.knob-cell.sm` 52 → 54 px, and the Bit Depth cell 84 px, so the French captions fit. Measured positions still match v1-ui.yaml (Import button x = 663, Bank select x = 481).
