# Stage 3 (GUI) Integration Checklist — O-Strata v1

**Plugin:** O-Strata (fork of O-Prism v1.24.0, `PLUGIN_CODE OuSt`, VERSION 1.0.0)
**Mockup version:** v1 (`mockups/v1-ui.yaml`, finalized 2026-09-08T01:59:53Z)
**Generated:** 2026-09-07 by ui-finalization-agent
**Contract:** `.planning/parameter-spec.md` (219 params, 48 Geometry, locked v1)

Steps are in **workflow order** and cite the REQUIREMENTS IDs that ROADMAP.md maps to each phase (Stage 3: UI-01..05, FUNC-06/07, PERF-03; Stage 1/2 prerequisites listed where the GUI depends on them). Every step names the file it touches and the artefact in this directory it copies from.

---

## 0. Prerequisites (must be true before Phase 3.1)

- [ ] Stage 1 done: `plugins/O-Strata/` forked from O-Prism; `PrismParamIds::oscIds()` carries the 24 geometry suffixes (`v1-PluginEditor-TEMPLATE.cpp` §0); `allSliderIds()` = 187; `param-dump` = 219 rows; `oscATable`/`oscBTable` gone (FUNC-09 diff = −2 +48)
- [ ] Stage 1 done: `createEditor()` guarded by `#if JUCE_WEB_BROWSER`; `juce_cryptography` linked (`v1-CMakeLists-SNIPPET.txt` hunks 3, 5)
- [ ] Stage 2 done: `GeometryBakeScheduler` publishes tables; `bakeGeneration[osc]`, `importRevision`, `getViewCache()`, `getViewMeshBlob()`, `submitGeometryImport()` exist on the processor (API block at the end of `v1-PluginEditor-TEMPLATE.cpp`)
- [ ] Optional Stage 1 fix: add `"delayDivision"` to `allSliderIds()` (unbound in O-Prism; see parameter-spec.md "Relay / attachment plumbing")

## 1. Copy UI files (Phase 3.1)

- [ ] `mockups/v1-ui.html` → `Source/ui/public/index.html`, then **merge** with O-Prism's `index.html`: keep O-Prism's Mod / Tuning / Effects tab markup and every block marked `INHERITED FROM O-PRISM` in the script (i18n canon v2 region byte-identical — check-i18n assertion 6); replace the two oscillator panels and the fifth tab with the mockup markup; drop every block named under `REMOVED FROM O-PRISM`
- [ ] `mockups/img/shell_conchologiaiconi12reev_0090.png` → `Source/ui/public/img/` (referenced as `img/shell_conchologiaiconi12reev_0090.png`, unchanged)
- [ ] `Source/ui/public/js/juce/index.js` + `check_native_interop.js` — keep O-Prism's copies (JUCE 8.0.14)
- [ ] Delete `Source/ui/public/js/wavetable-editor.js`, `Source/ui/public/css/wavetable-editor.css`
- [ ] `Source/ui/public/js/i18n.js`: add every key in `v1-ui.yaml` `i18n_new_keys.spec` + `added_by_mockup` (en now; fr through `scripts/i18n-fr` glossary + lint gate exit 2; zh-Hans through `scripts/i18n-zh-glossary.js` + lint Z5 — `reviewed:'mt'` until read). Remove the wavetable-tab keys. Readout nodes (`#geo-hud`, `#geo-status-ms`, `#geo-status-frames`, knob values) carry NO `data-i18n`
- [ ] Verify: `grep -c "100vh\|100vw\|100dvh\|100svh" Source/ui/public/index.html` = 0; `user-select: none` on `html, body`; `contextmenu` disabled; `<script type="module">` present

## 2. Update PluginEditor files (Phase 3.1 + 3.2 + 3.3)

- [ ] `Source/PluginEditor.h` ← apply `v1-PluginEditor-TEMPLATE.h` to O-Prism's header: class rename `OStrataAudioProcessorEditor`, NEW members (`lastPlayhead[2]`, `lastBakeGeneration[2]`, `lastImportRevision`, `viewMode[2]`, `pendingImport[2]`, four `push*` methods)
- [ ] **Verify member order:** relays (6 members) → `webView` → attachments (6 members). `grep -n "RELAYS FIRST\|WEBVIEW SECOND\|ATTACHMENTS LAST" Source/PluginEditor.h` must print ascending line numbers
- [ ] `Source/PluginEditor.cpp` ← apply `v1-PluginEditor-TEMPLATE.cpp` §2 (resource provider: −2 routes, +3 routes incl. `/geo/mesh-*`), §4 (−14 native fns, +5), §6 (timer → `emitEventIfBrowserIsVisible`, four push fns)
- [ ] Verify initialization order in the constructor matches declaration order (relays → options with `.withOptionsFrom()` for **every** relay → `addNativeFunctions` → WebView → attachments); `jassert (sliderAttachments.size() == sliderRelays.size())` holds (187)
- [ ] `grep -c evaluateJavascript Source/PluginEditor.cpp` = 0 (ARCHITECTURE Decision 7)
- [ ] Every `launchAsync` completion captures `Component::SafePointer` and returns **without** `complete()` on the null path (memory `pattern_webview_launchasync_safepointer_no_complete`) — `importGeometry` plus the inherited Scala/KBM/export choosers
- [ ] Windows: `withUserDataFolder(... "OStrata_WebView")` (memory `critical_webview2_runtime_gotchas_windows`)

## 3. Update CMakeLists.txt (Stage 1, re-verified here)

- [ ] Apply `v1-CMakeLists-SNIPPET.txt` hunks 1–6 to the forked file; root `CMakeLists.txt` has `add_subdirectory(plugins/O-Strata)`
- [ ] `juce_add_binary_data(O-Strata_UIResources …)` lists `index.html`, `js/i18n.js`, `js/juce/index.js`, `js/juce/check_native_interop.js`, `js/geometry-view.js`, `img/shell_conchologiaiconi12reev_0090.png` — and **nothing** hyphen-named without checking the stripped symbol (`geometryview_js`)
- [ ] Exactly **one** `juce_add_binary_data` target in the plugin (memory `critical_dual_binary_data_namespace_collision`)
- [ ] `juce::juce_gui_extra` + `juce::juce_cryptography` linked; `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`; `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`
- [ ] Offline render harness still configures/builds with `JUCE_WEB_BROWSER=0` (memory `pattern_render_harness_breaks_on_webview_editor`)

## 4. Phase 3.1 — Geometry panel, stacked-frame view, bake progress (UI-04)

- [ ] Build (Debug + Release): `cd build && ninja O-Strata_VST3 O-Strata_AU O-Strata_Standalone` — no warnings from PluginEditor.cpp
- [ ] Standalone shows the WebView (not blank); Inspect works; console has no errors; `window.__JUCE__` exists; every `data-i18n` key resolves (no `i18n: missing label key` warnings)
- [ ] All 48 geometry controls move their parameter and follow host automation (two-way relay round-trip); mirrored knobs (`osc?Pos`, `osc?GeoDrive` ×3) move together
- [ ] Source segmented control ⇄ Synth-tab Source dropdown ⇄ `osc?GeoSource` stay in sync; switching family swaps the visible panel; hidden controls keep their values; a knob in a hidden family never triggers a bake
- [ ] `TerBlur` / `TerEdge` greyed (`.inert`) unless Terrain = Imported…; `MeshPhi` greyed in Centroid Distance
- [ ] `bakeProgress` event → 2 px sage bake bar (canvas + big view) + amber "Baking…" status, then "Baked · NN ms · N frames"; hidden when idle (UI-04)
- [ ] ≋ view shows the new table within one push after a bake (UI-04); `setViewMode` round-trips
- [ ] `heldNotes` event drives the TrueKeys bar exactly as O-Prism's `updateHeldNotes` did
- [ ] `check-ui-labels`, check-i18n (assertions 6 + 8 + 10), boot-all-uis (no late/dead tip bindings), fr glossary lint exit 0, zh lint Z5 pass
- [ ] Commit: `feat(O-Strata): Phase 3.1 — geometry panel, stacked-frame view, bake progress, i18n`

## 5. Phase 3.2 — 3D view: WebGL2 + Canvas 2D, playhead, preset re-push (UI-01, UI-02, UI-05, PERF-03)

- [ ] Port `research/wavetable-synthesis-3d-geometry-prototypes/webgl-3d/terrain-proto.html` into `Source/ui/public/js/geometry-view.js` at the `// TODO Stage 3.2: WebGL2 path` block in `index.html`'s `GeoView` constructor (R32F 64×64 texture, `texSubImage2D` on `geometryState`, mesh from `geo/mesh-<osc>-<gen>.bin`; **no GLSL `flat`**; `webglcontextlost` → `preventDefault()` + fallback; `webglcontextrestored` → rebuild + forced relayout)
- [ ] Add `/js/geometry-view.js` to both the binary-data target and `getResource()` (embedded AND served)
- [ ] `geometryState` payload audit: largest event ≤ 32 KB; the 256×2048 table never crosses as text; mesh via `.bin` with the generation in the path
- [ ] WebGL2 view renders in Standalone (WKWebView), Logic (AU, WKWebView) and a Windows host (WebView2) (UI-01)
- [ ] Force `getContext('webgl2')` → null: Canvas 2D fallback draws the same primitives and `label.webglUnavailable` shows in en/fr/zh-Hans (UI-02)
- [ ] `WEBGL_lose_context` simulated: view recovers on `webglcontextrestored`, no stuck frame (UI-02)
- [ ] Playhead advances with Position/phase at 30 Hz; unchanged state sends **no** event (event counter in DevTools) (UI-01)
- [ ] After a preset apply and after session restore with the editor open, the view shows the new geometry within one push interval — `lastBakeGeneration`/`lastImportRevision` gate (UI-05; memory `pattern_webview_one_shot_state_push_stale_on_preset_load`)
- [ ] **PERF-03 frame-time gate:** `window.__geoDrawStats` (mean/max over 300 frames, reported via `reportDrawStats`) measured at DPR 2 in **WKWebView (Standalone + Logic)** and **WebView2 (Windows host)** — Chromium numbers are not evidence. Pass: ≤ 2 ms mean. **Fail: Canvas 2D becomes the default renderer** and the WebGL2 path stays behind a flag
- [ ] Commit: `feat(O-Strata): Phase 3.2 — 3D geometry view (WebGL2 + Canvas 2D), playhead, preset re-push`

## 6. Phase 3.3 — View interaction + OBJ/STL/PNG import (UI-03, FUNC-06, FUNC-07)

- [ ] Drag on the terrain view moves `osc?TerCX/CY` through the relay with `sliderDragStarted`/`sliderDragEnded` — host shows **one** undo step per gesture (UI-03)
- [ ] Drag on the mesh view moves `osc?MeshTiltY` (dx) / `osc?MeshTiltX` (dy); volume drag rotates the camera only; wheel edits `MeshPhi` (wraps) / `VolSweepRange` / `TerSweepRange`; no page scroll (`{passive:false}`) (UI-03)
- [ ] Import… button → `importGeometry` → `FileChooser::launchAsync` (SafePointer); bytes read on the message thread, parsed in the bake job (Decision 6, thread-name assert never fires) (FUNC-06)
- [ ] Drag-and-drop OBJ / STL ASCII / STL binary / PNG onto either oscillator canvas and the big view, macOS (`webkitGetAsEntry`, ≤ 4 MB base64 chunks → `importGeometryChunk` / `importGeometryCommit`) and Windows (same JS path) (FUNC-06, FUNC-07)
- [ ] `juce::Base64::convertFromBase64` on the C++ side — never `MemoryBlock::fromBase64Encoding` (memory `critical_webview_drag_drop_macos`)
- [ ] On success the backend selects `Imported…` (last index) and pushes `geometryState`; parse errors / open-mesh warnings arrive as localised notices; 1 M-triangle OBJ imports + bakes ≤ 2 s with the UI responsive (FUNC-06)
- [ ] PNG import honours Image Blur and Edge Mode (visible in the view and the table) (FUNC-07)
- [ ] Commit: `feat(O-Strata): Phase 3.3 — view interaction, OBJ/STL/PNG import`

## 7. Install + DAW test (every build that is tested in a host)

Per CLAUDE.md — preferred: `./scripts/build-and-install.sh O-Strata` (does the sweep below). Manual sequence:

```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Strata.vst3 ~/Library/Audio/Plug-Ins/VST3/O-Strata-dev.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Strata.component ~/Library/Audio/Plug-Ins/Components/O-Strata-dev.component
cp -R build/plugins/O-Strata/O-Strata_artefacts/Release/VST3/O-Strata*.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-Strata/O-Strata_artefacts/Release/AU/O-Strata*.component ~/Library/Audio/Plug-Ins/Components/
```

- [ ] Both `-dev` and unsuffixed variants swept before install (memory `critical_dev_release_variant_shadowing`)
- [ ] `auval -a | grep -i strata` lists the AU (cold auval rescans ~15 min — defer to batch end)
- [ ] Release build: reload the plugin 10× in Logic — no crash / freeze (member-order test)
- [ ] Automation and preset recall update every geometry control; values persist after reload
- [ ] No 404s in the resource provider (Inspect → Network); MIME types: `.js` = `application/javascript`, `.png` = `image/png`, `.bin` = `application/octet-stream`
- [ ] Standalone is NOT installed by the script — rebuild it explicitly when testing the WKWebView path (memory `pattern_build_install_skips_standalone_stale_ui`)

## 8. WebView-specific validation (final pass)

- [ ] No viewport units anywhere in CSS
- [ ] `user-select: none` on the page; `user-select: text` only on the tuning inputs
- [ ] `Juce` ES-module namespace used for every state/native call; `window.__JUCE__.backend` used only for `addEventListener` (memory `critical_juce_webview_namespace_vs_postmessage`)
- [ ] No `evaluateJavascript`; every push is `emitEventIfBrowserIsVisible` with a change gate and a re-send-on-visible cache
- [ ] Every `launchAsync` completion: SafePointer + no `complete()` on the null path

---

## Parameter list (from parameter-spec.md)

**Geometry (48, all `WebSliderRelay` + `WebSliderParameterAttachment`, bake: yes, mod destination: no):**

| Suffix (×2: `oscA…`, `oscB…`) | Type | UI (v1-ui.yaml) |
|---|---|---|
| `GeoSource` | Choice 3 | Synth dropdown + Geometry segmented control |
| `GeoFrames` | Choice 3 | Geometry segmented control |
| `GeoDrive` | Float 0–1 | knob (first in every family row) |
| `Mesh` | Choice 7 | library dropdown (Synth + Geometry), last = Imported… |
| `MeshTiltX`, `MeshTiltY` | Float −90–90° | knobs; view drag dy / dx |
| `MeshPhi` | Float 0–360° | knob; view wheel (wraps) |
| `MeshUnwrap`, `MeshLoop` | Choice 2 / 2 | dropdowns |
| `VolField` | Choice 5 | library dropdown (Synth + Geometry) |
| `VolOrbit`, `VolSweepAxis` | Choice 4 / 3 | dropdowns |
| `VolSweepRange`, `VolDetail` | Float 0–1 | knobs; SweepRange = view wheel |
| `Terrain` | Choice 3 | library dropdown (Synth + Geometry), last = Imported… |
| `TerOrbit`, `TerSweepAxis`, `TerEdge` | Choice 4 / 4 / 2 | dropdowns |
| `TerCX`, `TerCY` | Float −1–1 | knobs; view drag dx / −dy |
| `TerAspect` | Float 0.1–1 | knob |
| `TerRot` | Float 0–360° | knob |
| `TerSweepRange`, `TerBlur` | Float 0–1 | knobs; SweepRange = view wheel |

**Carried-over oscillator params (22):** `osc?{Pos,Level,Pan,Coarse,Fine,Phase,Unison,Detune,Width,WarpType,WarpAmt}` — slider relays, unchanged; `osc?Pos` readout "Frame N / F".

**Inherited (149) + toggles:** unchanged from O-Prism — 117 more slider relays, 30 toggle relays. Relay total 217 = attachments 217 (of 219 params; `delayDivision`, `stereoWidth` unbound as in O-Prism).
