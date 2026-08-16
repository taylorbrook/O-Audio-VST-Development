# Stage 3 (GUI) Integration Checklist — O-Bitrot mockup v1

**Plugin:** O-Bitrot (CMake target `OBitrot`, editor `OBitrotAudioProcessorEditor`)
**Mockup version:** v1 (finalized 2026-08-15)
**Window:** 900 × 620, fixed (non-resizable)
**Parameter contract:** `.planning/parameter-spec.md` — BINDING, 31 params (19 slider / 7 toggle / 5 combo). IDs are UPPERCASE.

## 1. Copy UI files

- [ ] Copy `v1-ui.html` → `Source/ui/public/index.html`
- [ ] Verify `Source/ui/public/js/juce/index.js` + `check_native_interop.js` present (already scaffolded)
- [ ] Verify `Source/ui/public/img/paper.jpg` + `img/specimen.webp` present (already scaffolded; clean-texture md5 gate + `img/PROVENANCE.md` — NOT the watermarked Adobe Stock texture)
- [ ] Note: production HTML deliberately drops the Google-Fonts `<link>` from the test mockup (WebView must work offline); serif stack falls back to system Garamond/Times. If EB Garamond must ship, add a bundled woff2 + resource-provider entry (`font/woff2`) as a follow-up.

## 2. PluginEditor

- [ ] Replace `Source/PluginEditor.h` with `v1-PluginEditor.h` content
- [ ] Replace `Source/PluginEditor.cpp` with `v1-PluginEditor.cpp` content
- [ ] Verify member order: 31 relays → `webView` → 31 attachments (destruction is reverse; wrong order = release-build crash on reload)
- [ ] Verify processor exposes public `apvts` and public `std::atomic<uint32_t> uiActivityMask` (both exist as of Stage 2)
- [ ] Verify NO `withNativeFunction` calls (this plugin has zero native functions)
- [ ] `#if JUCE_WEB_BROWSER` guard on `createEditor()` if the render harness builds the processor headless (repo pattern: harness silently breaks when Stage 3 swaps in a WebView editor)

## 3. CMakeLists.txt

- [ ] Diff against `v1-CMakeLists.txt` — the in-tree CMakeLists already carries the WebView blocks; verify, do not duplicate:
  - `NEEDS_WEB_BROWSER TRUE` + `NEEDS_WEBVIEW2 TRUE` in `juce_add_plugin`
  - `juce::juce_gui_extra` linked
  - `OBitrot_UIResources` binary data lists all 5 UI files
  - `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`

## 4. Build and test (Debug + Release)

- [ ] Build succeeds; Standalone shows the naturalist UI (not blank)
- [ ] Right-click → Inspect: no JS console errors; `window.__JUCE__` exists
- [ ] Release build: reload plugin 10× — no crash (member-order test)
- [ ] Install via `./scripts/build-and-install.sh O-Bitrot` (handles AU cache + dual-variant sweep)

## 5. Parameter binding (31/31)

- [ ] 18 float knobs + drag/dblclick/wheel write back with sliderDragStarted/Ended gesture brackets
- [ ] Readouts show engineering values from `getScaledValue()` — spot-check the two skewed params: CRUSH_RATE (500 Hz–20 kHz exp, "k" formatting ≥1 kHz) and CLOCK_FREE_RATE (0.1–20 Hz exp)
- [ ] 6 enable toggles dim their panel body (opacity 0.45 + pointer-events none); enable button stays interactive at full opacity
- [ ] HARD_EDGES button lights when on
- [ ] Segmented combos: VINYL_RPM (33⅓/45), CODEC_MODE (μ-law/GSM), CLOCK_MODE (Sync/Free)
- [ ] Dropdowns: CLOCK_SYNC_DIV (7 divisions), PACKET_CONCEAL (4 modes)
- [ ] CLOCK_MODE swaps division dropdown ↔ free-rate mini knob (150 ms crossfade); swap also follows host automation/preset recall (listener-driven)
- [ ] Host automation + preset recall update all UI controls (attachments → valueChangedEvent)
- [ ] DAW UI-side check: knob gestures create single undo/automation gestures (drag brackets)

## 6. SEED + reseed die

- [ ] Seed ledger renders `getScaledValue()` as 4-digit zero-padded integer
- [ ] Die click = ONE host gesture: `sliderDragStarted → setNormalisedValue(Math.random()) → sliderDragEnded`; die face changes; readout pulses
- [ ] SEED persists across save/reload (Standalone SEED persistence eyeball — carried from Stage-2 verify)

## 7. Event LED bridge

- [ ] Processor writes `uiActivityMask` (bits 0-3 = tape, cd, vinyl, packet) on the audio thread; editor Timer at 30 Hz emits `ledUpdate` `{"mask":N}` via `emitEventIfBrowserIsVisible` (fire-and-forget)
- [ ] JS listens via `window.__JUCE__.backend.addEventListener("ledUpdate", ...)` — the ONE legitimate `__JUCE__` use in this UI
- [ ] Tape/CD/Vinyl/Packet LEDs pulse-stretch ≥120 ms after last active tick
- [ ] Codec/Crush LEDs steady-lit from their enable toggles (no event bits)
- [ ] LEDs go dark when playback stops (decay interval fires)

## 8. WebView compliance (repo patterns)

- [ ] No viewport units in CSS (`100vh`/`100vw`) — uses `html, body { height: 100% }`
- [ ] `user-select: none`, context menu + dragstart disabled
- [ ] `<script type="module">` importing the `Juce` namespace from `./js/juce/index.js` — params NEVER bound via `window.__JUCE__`
- [ ] Bare relative asset paths (`img/paper.jpg`) — resource provider matches bare paths, no hard-coded scheme
- [ ] `withUserDataFolder` set for WebView2 (Windows DAW-host gotcha)
- [ ] All 5 resources served with correct MIME types (no 404 in DevTools network log)

## 9. Regression gates (from Stage-3 PLAN)

- [ ] Render harness re-run after any processor edit (44/44 probes)
- [ ] pluginval strictness 10, both formats
- [ ] Logic smoke check + Stage-2 carried listening items (MIX 50%/0% + HARD_EDGES, ENV_AMT voicing)

## Parameter → relay map (31)

| Family | Slider (WebSliderRelay) | Toggle (WebToggleButtonRelay) | Combo (WebComboBoxRelay) |
|--------|--------------------------|-------------------------------|--------------------------|
| Tape   | TAPE_PROB, TAPE_STOP_PROB, TAPE_RAMP | TAPE_ENABLE | — |
| CD     | CD_PROB, CD_SEVERITY, CD_SEGMENT | CD_ENABLE | — |
| Vinyl  | VINYL_PROB, VINYL_POP | VINYL_ENABLE | VINYL_RPM |
| Packet | PACKET_LOSS, PACKET_BURST | PACKET_ENABLE | PACKET_CONCEAL |
| Codec  | CODEC_MIX | CODEC_ENABLE | CODEC_MODE |
| Crush  | CRUSH_BITS, CRUSH_RATE, CRUSH_JITTER, CRUSH_ENV_AMT, CRUSH_DITHER | CRUSH_ENABLE | — |
| Global | CLOCK_FREE_RATE, SEED, MIX | HARD_EDGES | CLOCK_MODE, CLOCK_SYNC_DIV |

Counts: 19 sliders (incl. SEED Int) / 7 toggles / 5 combos = 31.
