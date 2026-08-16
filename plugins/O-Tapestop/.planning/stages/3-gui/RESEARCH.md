# Stage 3: GUI — Research

**Researched:** 2026-08-15
**Domain:** JUCE 8 WebView UI (Ouaricon house style), canvas envelope editor, param bridge
**Confidence:** HIGH — every recommendation is grounded in an in-repo, shipped precedent read this session

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **UI-01** — Drawable bipolar speed-vs-time envelope editor for Scratch mode (canvas, add/drag/delete points, per-segment curve, labelled 1× line, reverse zone, pass playhead). Path C §2.2 WebView editor is the reference implementation.
- **UI-02** — Prominent engage control usable as a live performance gesture; UI click and host automation are identical (both via setValueNotifyingHost).
- Parameter set is FIXED (14 APVTS params + scratchEnvelope blob). UI owns layout only.
- SYNC_MODE drives show/hide of sync-division vs free-ms controls.
- Design path: direct in Stage 3, no ui-mockup workflow; iterate in Standalone (`/show-standalone`).
- Aesthetic: **ouaricon-naturalist-001**; closest reference O-ReverseDelay `Source/ui/public/`.
- Stop/Scratch layout: **mode-switched center panel** (Stop controls ↔ Scratch canvas).
- Live visualization: **playback-ratio indicator only** (incl. reverse); no reel animation, no transport lamp.

### Constraints
- Fixed-px frame, no viewport units; frame size mirrored in PluginEditor.cpp setSize.
- Frame fits 1080p including DAW header + menu bar; measure rendered boxes, don't derive slack.
- Canvas: explicit width/height + DPR backing store.
- Readouts via `SliderState.getScaledValue()`.
- Native-fn bridge SafePointer-hardened; grep-diff getNativeFunction vs withNativeFunction.
- UI readback: atomics + editor timer + `emitEventIfBrowserIsVisible`.

### Open Questions settled below
Frame dimensions · ratio-indicator form · preset-band sequencing · timer cadence · mode-switch transition.
</user_constraints>

---

## Q1 — Frame dimensions: **860 × 580**

**Calibration point:** O-ReverseDelay is `setSize (940, 768)` [VERIFIED: plugins/O-ReverseDelay/Source/PluginEditor.cpp:562 — `setSize (940, 768);`]. Its CSS frame matches exactly (`.frame { width: 940px; height: 768px; padding: 18px 22px 14px; border: 3px solid … }` [VERIFIED: styles.css:257-266]). Other suite points: O-Bitrot `setSize(520, 640)`, O-Polystutter `setSize(1000, 690)` [VERIFIED: respective PluginEditor.cpp via grep + read of matched lines].

**Panel inventory (from CONTEXT decisions):**

| Panel | Contents | Approx interior need |
|---|---|---|
| TRIGGER (left col) | ENGAGE big control, MODE segments (Stop/Scratch), SYNC segments (Sync/Free), ratio indicator | ~180 w × ~380 h |
| CENTER (mode-switched) | Stop: STOP time-slot + STOP_CURVE knob, START time-slot + START_CURVE knob (2×2) ↔ Scratch: canvas ~400×240 + ENV time-slot row | ~430 w × ~380 h |
| OUTPUT (right col) | TONE_TRACK, MIX, OUTPUT_GAIN knobs stacked | ~180 w × ~380 h |

**Width:** 180 + 430 + 180 + 2×12 gaps = 814 content + 50 frame chrome (2×22 padding + 2×3 border, per ORD .frame) ≈ 864 → **860** (trim gaps to 10).

**Height:** frame chrome 38 (18+14 padding + 6 border) + header ~73 (25 px title + subtitle + 10 pad + rule + 16 margin, per ORD `.header`/`.title` [VERIFIED: styles.css:270-296]) + preset band 44 (`32 px band + 12 px margin` [VERIFIED: styles.css:302-313 comment + `.preset-bar { height: 32px; margin-bottom: 12px; }`]) + panel row ~380 + row margin ~16 + footer ~28 = ~579 → **580**.

1080p check: available height ≈ 1080 − menu bar (~25) − DAW plugin-window header (~50–80) ≈ 975–1005. 580 clears it with large margin (ORD's 768 already did). Knob vocabulary transfers directly: house knob is 56×56 (`.knob { width: 56px; height: 56px; }` [VERIFIED: styles.css:760-761]); three stacked knob-cells ≈ 340 px, fits the 380 panel height.

**Recommendation:** `setSize(860, 580)` mirrored exactly by `.frame { width: 860px; height: 580px; }`. Lock it in Phase 3.1 and never resize (ORD's index.html comments record that frame resizes are what invalidate layout/tooltip verification — the Row-2 empty-panel reserve existed specifically to avoid them [CITED: plugins/O-ReverseDelay/Source/ui/public/index.html:66-90]).

## Q2 — Playback-ratio indicator: **horizontal bipolar bar + numeric ×-readout**

- **Form:** a slim horizontal bar spanning −2×…+2× with the zero mark at 40 % (asymmetric span matches the actual range: Stop mode lives in [0, 1.25], Scratch in [−2, +2]). Ticks/labels at −1×, 0, 1×, 2×; fill drawn from 0 toward the current ratio; reverse fill styled distinctly (e.g. the naturalist red-brown vs green). Numeric readout beneath: `0.62×`, `−1.40×` (2 decimals; shows reverse unambiguously).
- **Why not a needle:** skeuomorphic chrome foreign to the field-guide aesthetic; harder to read a bipolar zero. **Why not numeric-only:** a flickering number gives no at-a-glance direction/magnitude during a fast scratch pass.
- **Placement:** bottom of the TRIGGER panel, under ENGAGE — it reads as "what the transport is doing" next to the control that causes it.
- **Rendering:** plain DOM (a div fill-width + text), not canvas — 30 Hz style/text updates on two elements are trivially cheap. Guard: write only when the displayed value changed (avoid layout churn at steady 1.00×). Note the readout element must be JS-owned from birth (placeholder `—` in HTML), so the label-erasure pattern doesn't apply to it, only to its caption.
- **Readback rate:** the shared 30 Hz transport event (Q4).

## Q3 — Preset band: **author the band in Phase 3.1, wire preset-manager in Stage 4**

Precedents:
- O-ReverseDelay shipped its preset bar **in Stage 4** — the index.html section is literally headed `Preset bar (Stage 4)` [VERIFIED: index.html:39], and the CSS comment records the frame **grew by 44 px at that point** ("32 px band + 12 px margin = the 44 px the frame grew by" [VERIFIED: styles.css:302-304]) — i.e. a Stage-4 frame resize.
- O-Bitrot's Stage-3 CONTEXT contains no preset scope — also deferred to Stage 4 [VERIFIED: plugins/O-Bitrot/.planning/stages/3-gui/CONTEXT.md — no preset mentions].
- ROADMAP Stage 4 owns factory presets ("Stage 4: Validation — factory presets (incl. classic 1/2-bar stop, DJ spinup, 2 scratch gestures)" [VERIFIED: ROADMAP.md:146]).
- Module: `preset-manager` v1.0.5, provides `preset-manager.js` + native fns savePreset/loadPreset/getPresetList/deletePreset/selectNextPreset/selectPreviousPreset [VERIFIED: modules/registry.yaml:140-157].

**Recommendation:** split the ORD sequencing at the layout/behavior seam. Phase 3.1 ships the 44 px band **markup + CSS with disabled controls** (IDs `preset-prev/next/name/save/load/delete` exactly as preset-manager.js binds them — copy from ORD index.html:54-64), so the 580 px frame is final on day one and Stage 4 never resizes or re-verifies layout. Stage 4 adds preset-manager.js (module v1.0.5), the native fns, and factory presets, and un-disables the buttons. This avoids ORD's Stage-4 frame-grow while keeping presets out of Stage-3 scope.

## Q4 — Timers: **one editor `juce::Timer` at 30 Hz, one event, both consumers**

- Precedent: O-Polystutter uses `startTimerHz(30)` in the editor; `timerCallback()` reads processor atomics with `memory_order_relaxed` and pushes one payload via `webView->emitEventIfBrowserIsVisible("laneProgress", …)` [VERIFIED: plugins/O-Polystutter/Source/PluginEditor.cpp:882, 980-1023].
- O-ReverseDelay's alternative (JS `setInterval(66 ms)` polling a native fn, app.js:814-856) is **not** recommended here: its in-flight guard means a completion dropped while the view is hidden (memory: critical_webview_completion_gated_on_isvisible) leaves `meterInFlight` stuck and the poll dead. CONTEXT already locks the atomics + editor-timer + `emitEventIfBrowserIsVisible` approach, which has no un-settled-promise failure mode.
- **One event, not two cadences:** emit `"transportFrame"` `{ state:int, ratio:float, phase:float }` at 30 Hz. JS single listener routes: ratio bar always; canvas playhead redraw only when MODE=Scratch and state==ScratchPass (skip redraws when nothing changed — steady Bypassed costs ~nothing). 30 Hz is ample for a playhead line and a ratio bar; 60 Hz doubles canvas repaints for no perceptual gain on a moving cursor.
- **Same timer** also polls the envelope generation counter (see bridge inventory) — no second timer.
- JS listens via `window.__JUCE__.backend.addEventListener("transportFrame", …)` — the Polystutter precedent [VERIFIED: plugins/O-Polystutter/Source/ui/public/js/parameter-bindings.js:1004-1005]. This is the sanctioned `__JUCE__` use (backend events); parameter state must still come through the `Juce` ES-module namespace (memory: critical_juce_webview_namespace_vs_postmessage).

## Q5 — Mode-switch transition: **instant class toggle (house precedent), fade optional later**

O-ReverseDelay's FREE↔SYNC swap is the exact template: both wraps absolutely positioned into one fixed-dimension slot, `.hidden` class toggled — "the FREE↔SYNC swap produces zero layout shift" [VERIFIED: index.html:110-133 + comment]. Apply the same pattern at panel scale: `.mode-stop` and `.mode-scratch` panes absolutely positioned in the fixed center box; MODE combo listener toggles `.hidden`. Instant is the house behavior and can't thrash layout. If Standalone iteration finds it abrupt, add a CSS-only `opacity` transition (~120 ms) with `visibility` (not `display`) gating — still zero layout work. Do **not** animate size/position. Note: the hidden pane's canvas stops receiving redraws automatically (playhead draw is gated on Scratch state), so there's no wasted paint.

---

## Parameter Binding Inventory (all 14)

Param IDs verified verbatim from `createParameterLayout()` [VERIFIED: plugins/O-Tapestop/Source/PluginProcessor.cpp:104-197 — `"ENGAGE"`, `"MODE"`, `"SYNC_MODE"`, `"STOP_SYNC_DIV"`, `"STOP_FREE_MS"`, `"STOP_CURVE"`, `"START_SYNC_DIV"`, `"START_FREE_MS"`, `"START_CURVE"`, `"ENV_SYNC_DIV"`, `"ENV_FREE_MS"`, `"TONE_TRACK"`, `"MIX"`, `"OUTPUT_GAIN"`].

| Param ID | Relay (C++) | JS state API | Control | Notes |
|---|---|---|---|---|
| ENGAGE | WebToggleButtonRelay + WebToggleButtonParameterAttachment | `getToggleState("ENGAGE")` | Large engage control (UI-02) | Attachment path == setValueNotifyingHost with gesture brackets ⇒ UI click ≡ automation by construction. Latching click for v1; a momentary hold gesture (pointerdown=on / pointerup=off) is a possible later polish — Claude's discretion, ship latching first |
| MODE | WebComboBoxRelay + WebComboBoxParameterAttachment | `getComboBoxState("MODE")` | 2-segment pair (Stop/Scratch) | ORD `syncSegments` pattern (segments bound to combo state, labels HTML-authored, JS toggles classes only) |
| SYNC_MODE | WebComboBoxRelay + attachment | `getComboBoxState("SYNC_MODE")` | 2-segment pair (Sync/Free) | Drives the three time-slot swaps |
| STOP_SYNC_DIV | WebComboBoxRelay + attachment | `getComboBoxState` | `<select class="division-select">` in stop time-slot | Options built at runtime from `properties.choices` — C++ StringArray is the single source (ORD index.html:126 comment) |
| STOP_FREE_MS | WebSliderRelay + 3-arg WebSliderParameterAttachment | `getSliderState` | Knob in stop time-slot | Skewed range (freeMsRange, skew 0.35) — readout MUST be `getScaledValue()` |
| STOP_CURVE | WebSliderRelay + attachment | `getSliderState` | Knob | % readout |
| START_SYNC_DIV | WebComboBoxRelay + attachment | `getComboBoxState` | Select in start time-slot | same as STOP_SYNC_DIV |
| START_FREE_MS | WebSliderRelay + attachment | `getSliderState` | Knob in start time-slot | skewed |
| START_CURVE | WebSliderRelay + attachment | `getSliderState` | Knob | % |
| ENV_SYNC_DIV | WebComboBoxRelay + attachment | `getComboBoxState` | Select in env time-slot (Scratch pane) | same division list |
| ENV_FREE_MS | WebSliderRelay + attachment | `getSliderState` | Knob in env time-slot | skewed |
| TONE_TRACK | WebSliderRelay + attachment | `getSliderState` | Knob (OUTPUT) | % |
| MIX | WebSliderRelay + attachment | `getSliderState` | Knob (OUTPUT) | % |
| OUTPUT_GAIN | WebSliderRelay + attachment | `getSliderState` | Knob (OUTPUT) | dB, plain range |

**SYNC_MODE show/hide:** three fixed-dimension slots (stop, start, env), each holding an absolutely-positioned select-wrap and knob-wrap; `getComboBoxState("SYNC_MODE").valueChangedEvent` listener toggles `.hidden` on all three pairs. Zero layout shift (ORD time-slot precedent). The env slot lives inside the Scratch pane; its visibility is the AND of SYNC_MODE and MODE — express as two independent class toggles on nested wrappers, never computed into one flag.

**Relay/attachment mechanics (ORD PluginEditor.cpp:199-525, read this session):** relays constructed first, `Options{}.withOptionsFrom(*relay)` per relay, then `webView = make_unique<WebBrowserComponent>(options)`, then attachments (param, relay, nullptr undoManager), then `goToURL(getResourceProviderRoot())`. Member declaration order must keep relays before webView before attachments (destruction order).

---

## Native-Fn Bridge Inventory

| Name | Direction | Registered via | JS side | Payload |
|---|---|---|---|---|
| `getParameterDefaults` | JS→C++ (once at init) | `withNativeFunction` | `getNativeFunction` | DynamicObject of engineering-unit defaults via `param->convertFrom0to1(param->getDefaultValue())` — house standard for double-click reset, skew-exact [VERIFIED: ORD PluginEditor.cpp:238-251] |
| `commitEnvelope` | JS→C++ (mouse-up + 50 ms debounce) | `withNativeFunction` | `getNativeFunction` | arg: JSON string → `processorRef.commitScratchEnvelopeJson(json)` [VERIFIED: PluginProcessor.h:94-97 — public, message-thread, sanitize+bake+publish]; completes **synchronously** with the sanitized `toJson()` so the page redraws the post-sanitize truth (clamps/sort/pins applied) |
| `requestEnvelope` | JS→C++ (page init) | `withNativeFunction` | `getNativeFunction` | completes with current envelope JSON. **Requires a new public accessor** — `scratchEnvelope` is private; add `juce::String getScratchEnvelopeJson() const { return scratchEnvelope.toJson(); }` (ScratchEnvelope::toJson exists [VERIFIED: ScratchEnvelope.h:116-125]) |
| `transportFrame` (event) | C++→JS, 30 Hz | `emitEventIfBrowserIsVisible` | `window.__JUCE__.backend.addEventListener` | `{state, ratio, phase}` from three new processor atomics (below) |
| `envelopeState` (event) | C++→JS, on change | `emitEventIfBrowserIsVisible` | backend `addEventListener` | timer compares a generation counter; on change emits the sanitized JSON — covers host session-reload / preset-load while the editor is open, without any JS promise that could be dropped while hidden |

Preset fns (savePreset/loadPreset/getPresetList/deletePreset/selectNext/Previous/…) are **Stage 4** (module registry list, ORD precedent).

**Processor additions needed (small, Stage 3 scope, DSP-frozen-safe):**
- `std::atomic<float> uiRatio, uiScratchPhase; std::atomic<int> uiState;` — relaxed stores once per block at the end of `processBlock` (audio thread publishes, never reads). Pure stores; the render harness (compiles processor with `JUCE_WEB_BROWSER=0`, no editor sources) is unaffected — but **re-run the 47-probe harness after touching PluginProcessor.cpp** anyway (regression guard; the DSP is verified frozen).
- `std::atomic<uint32_t> uiEnvGeneration` — incremented in `commitScratchEnvelopeJson` and in `setStateInformation` after envelope restore.
- `juce::String getScratchEnvelopeJson() const` accessor.
- Transport currently exposes `getState()` [VERIFIED: TapestopTransport.h:149] but no ratio/phase getters — either add tiny getters or publish from the values processBlock already has in hand. Stage 2 exposed **no** UI readback atomics; this is genuinely new Stage-3 work.

**SafePointer hardening:** commit/request are synchronous lambdas capturing `this` on a webView the editor owns — safe as-is (the fns cannot outlive the editor). SafePointer is mandatory only if any completion is deferred (none planned; no FileChoosers until Stage 4 — where pattern_webview_launchasync_safepointer_no_complete applies to the preset save/load dialogs). Still run the **grep-diff check**: every JS `getNativeFunction("X")` name must appear in a C++ `withNativeFunction("X")` and vice versa (pattern_webview_native_fn_bridge_gap) — make it a Phase 3.3 verification step.

---

## Envelope Canvas Editor — Implementation Notes

**Reference:** Path C §2.2 `EnvelopeEditor` class [VERIFIED: research/stutter-effects/path-c-playhead-modulator.md:500-809] — point model `{x, y, curve}`, drag/dblclick-add/rightclick-delete, endpoint pinning, 0.01 min x-separation, `getValueAt` with per-segment pow curve. The C++ side already ports this verbatim: ScratchEnvelope's curve law is `curve>0: t = pow(t, 1+2c)`, `curve<0: t = 1−pow(1−t, 1−2c)` [VERIFIED: plugins/O-Tapestop/Source/dsp/ScratchEnvelope.h:242-247, header comment "ported VERBATIM from Path C getValueAt()" at :32-33].

**Required adaptations from the reference:**

1. **DPR backing store (reference is DPR-naive).** Path C maps coordinates via `canvas.width` directly (§2.2 lines 532-545) — wrong once the backing store is `cssSize × dpr`. Use the house `envResize()` pattern verbatim [VERIFIED: ORD app.js:635-653]: `canvas.width = round(clientWidth·dpr)` only when changed (assignment clears the canvas), `ctx.setTransform(dpr,0,0,dpr,0,0)`, then draw in CSS px; mouse mapping via `getBoundingClientRect()` in CSS px. Canvas element gets **explicit width/height attributes + CSS px dimensions** — it is a replaced element; `left+right` will not stretch it (memory: o-textureforge-cursor-bug). Fixed frame ⇒ fixed canvas box (~400×240); DPR migration self-heals because the editor redraws on every drag and every playhead frame (ORD's documented stale-DPR caveat doesn't bite here).
2. **Bipolar y-axis.** Envelope y ∈ [−1, +1], speed r = 2y. Mapping: `canvasY = (1 − (y+1)/2) · h`. Draw: labelled **1× line at y=+0.5** (solid, prominent — it is the "normal speed" reference), 0-line at y=0 (stop), reverse zone y<0 tinted (naturalist wash, e.g. faded red-brown at low alpha), faint gridlines at ±0.5/±1 with labels `+2×, 1×, 0, −1×, −2×` in the page serif. Read palette from CSS custom properties, not hardcoded hexes (ORD drawEnvelope precedent, app.js:667-670).
3. **Per-segment curve handles.** The reference stores `curve` but has **no edit gesture for it** — invent one: a small diamond handle at each segment's visual midpoint; vertical drag adjusts `curve` ∈ [−1, +1] (drag up bows the segment up); double-click on the handle resets to 0. This matches Ableton/TimeShaper conventions and needs no extra chrome.
4. **Sanitize parity.** Mirror the C++ constraints in JS interaction (2–64 points, endpoints pinned at x=0/1, min separation 0.01, clamp x/y/curve) — [VERIFIED: ScratchEnvelope.h:28 — "{ x ∈ [0,1], y ∈ [−1,+1], curve ∈ [−1,+1] }, 2–64 points, sorted by x"]. On commit, redraw from the **returned sanitized JSON**, so any disagreement resolves to the C++ truth. The JS curve-preview formula must be copied character-for-character from ScratchEnvelope.h:242-247 with a comment binding the two (the getWindowCurve rationale — a drifted preview keeps looking plausible [CITED: ORD PluginEditor.cpp:297-310 comment] — is mitigated here by the commit-echo redraw; full LUT round-trip fetching per drag frame is not worth the latency).
5. **Commit cadence.** Local draw during drag (60 fps-ish, event-driven); `commitEnvelope(json)` on mouse-up debounced 50 ms (matches ARCHITECTURE bake spec: "editor edit commit (mouse-up + 50 ms debounce)"). Edits mid-pass apply at the next engage edge by DSP contract — no UI special-casing.
6. **Playhead.** Vertical line at `x = phase·w` drawn from the `transportFrame` event, only while `state == ScratchPass`; skip redraw when phase unchanged. Playhead is drawn last, over the curve.
7. **Pointer handling.** Prefer `pointerdown/pointermove/pointerup` + `setPointerCapture` over the reference's mouse listeners (drag continues cleanly outside the canvas; no document-level listener bookkeeping). Right-click delete keeps `contextmenu` preventDefault; also support alt-click delete (right-click is awkward in some hosts).
8. **Module file** `js/envelope-editor.js`, ES module imported by app.js. **Binary symbol will be `envelopeeditor_js`** — juce_add_binary_data strips hyphens (memory: critical_binary_data_strips_hyphens; ORD's `preset-manager.js → presetmanager_js` precedent [VERIFIED: ORD PluginEditor.cpp:168-175 comment]). Consider naming it `envelope_editor.js` to dodge the trap entirely.

---

## CMake / WebView Wiring

**Stage 1 already wired** [VERIFIED: plugins/O-Tapestop/CMakeLists.txt:14-62]: `NEEDS_WEB_BROWSER TRUE`; `JUCE_WEB_BROWSER=1` compile definition on the plugin target; `juce::juce_gui_extra` linked; `Source/PluginEditor.cpp` on the plugin target only (harness excludes it); placeholder comment at :55-56: "Stage 3 adds the juce_add_binary_data UI-resources target here (distinct NAMESPACE/HEADER_NAME — critical_dual_binary_data_namespace_collision)".

**Stage 3 must add** (mirror ORD CMakeLists.txt:64-97):

```cmake
juce_add_binary_data(OuariconTapestop_UIResources
    NAMESPACE UIBinaryData
    HEADER_NAME UIBinaryData.h
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/envelope_editor.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/<specimen>.png)
target_link_libraries(OuariconTapestop PRIVATE OuariconTapestop_UIResources)
```

- Distinct `NAMESPACE UIBinaryData` + `HEADER_NAME UIBinaryData.h` from day one (Stage-4 preset assets defaulting to `BinaryData` would duplicate-symbol otherwise — ORD comment :64-67).
- Copy `js/juce/` (index.js + check_native_interop.js) from O-ReverseDelay [VERIFIED: dir listing — those two files].
- Resource provider: explicit `if (url == "/…")` map returning bare-path resources with `charset=utf-8` MIME on text (UTF-8 fleurons mojibake otherwise — ORD PluginEditor.cpp:149-156). Provider receives **bare paths**, never scheme-prefixed (memory: critical_webview_resource_provider_and_schemes). `goToURL(WebBrowserComponent::getResourceProviderRoot())`.
- Windows (CI): `Options::WinWebView2{}` with `withUserDataFolder` (ORD :483; memory: critical_webview2_runtime_gotchas_windows) and static-linking define already handled at repo level (memory: critical_webview2_static_linking) — copy ORD's block verbatim.
- Rebuild note: after any plugin build, the CLAUDE.md cache-clear + dual-variant sweep sequence applies (`./scripts/build-and-install.sh O-Tapestop` preferred).

---

## Pitfalls (memory patterns — applicability verified)

| Pattern | Applies to | How it's handled |
|---|---|---|
| `pattern_webview_knob_readout_scaled_value` | STOP/START/ENV_FREE_MS (skew 0.35), all knob readouts | Readouts + reset round-trip exclusively via `SliderState.getScaledValue()` / `convertFrom0to1` native fn; no JS range tables (ORD app.js:305-313 is the template) |
| `critical_webview_completion_gated_on_isvisible` | Live readback | Push model (`emitEventIfBrowserIsVisible` + atomics + editor timer); **no** polling native fn for live data, so no promise can hang hidden. requestEnvelope runs only at init/commit when the view is visible |
| `critical_juce_webview_namespace_vs_postmessage` | app.js + envelope module | `import * as Juce from "./juce/index.js"`; pass the `Juce` namespace into panel/editor init functions; `window.__JUCE__` used only for `backend.addEventListener` (Polystutter precedent) |
| `pattern_module_toplevel_init_tdz` | app.js / envelope_editor.js | No top-level statements touching later `let/const`; all init inside `init()` called after DOM + Juce ready; envelope module exports a class/factory only |
| `pattern_js_state_updater_overwrites_html_labels` | Segments (MODE, SYNC_MODE), preset-band placeholders, tooltips | Button/segment copy authored in HTML (`data-label`); JS toggles classes/aria only; JS-owned text limited to readout elements born with `—` placeholders |
| `pattern_render_harness_breaks_on_webview_editor` | createEditor | Already guarded: `#if JUCE_WEB_BROWSER` in createEditor/hasEditor [VERIFIED: PluginProcessor.cpp:483-497]; PluginProcessor must stay free of editor includes (header comment :52-54). Stage 3 keeps all WebView code in PluginEditor.*; re-run the 47-probe harness after the processor readback-atomic additions |
| Fixed-px frame | index.html/styles.css | No vh/vw/%: fixed `.frame` w/h == setSize; ORD stylesheet is the base to copy |
| `critical_binary_data_strips_hyphens` | UI asset filenames | Name JS modules with underscores, or map the stripped symbol correctly (ORD comment precedent) |
| `critical_dual_binary_data_namespace_collision` | CMake | `NAMESPACE UIBinaryData` + `HEADER_NAME UIBinaryData.h` from the start |
| Canvas replaced element (o-textureforge-cursor-bug) | Envelope canvas | Explicit width/height + DPR backing store via envResize pattern; never `left+right` stretch |
| `pattern_webview_native_fn_bridge_gap` | Bridge | Phase 3.3 verification: grep-diff `getNativeFunction` names vs `withNativeFunction` names, both directions |
| `critical_juce_string_char_ctor_is_ascii_only` | Any C++-built strings with ×/– glyphs | Keep non-ASCII glyphs in HTML entities/JS only; C++ payloads stay ASCII |
| `pattern_webview_launchasync_safepointer_no_complete` | Not in Stage 3 (no FileChoosers) | Becomes relevant in Stage 4 preset dialogs — note in handoff |

---

## Recommendations for PLAN.md

1. **Frame:** `setSize(860, 580)` mirrored in `.frame`; 3-column row (TRIGGER 180 / mode-switched CENTER 430 / OUTPUT 180) under header (~73) + preset band (44, markup shipped disabled in 3.1, wired Stage 4) + footer. Never resize after 3.1.
2. **Phase 3.1:** copy ORD `ui/public` scaffolding (styles.css vocabulary, js/juce/, resource-provider editor skeleton, WinWebView2 options); author index.html directly (no mockup); add CMake `OuariconTapestop_UIResources` (NAMESPACE UIBinaryData); iterate in `/show-standalone`.
3. **Phase 3.2:** 8 WebSliderRelays, 5 WebComboBoxRelays, 1 WebToggleButtonRelay (+3-arg attachments, declaration order relays→webView→attachments); ENGAGE as large latching toggle; SYNC_MODE triple time-slot swap (ORD fixed-slot pattern); `getParameterDefaults` native fn; readouts via getScaledValue.
4. **Phase 3.3:** `envelope_editor.js` (underscore name) adapted from Path C §2.2 with DPR envResize, bipolar axis (1× line at y=+0.5, tinted reverse zone), midpoint curve handles, pointer capture, commit-on-mouseup+50 ms, redraw from sanitized echo; native fns `commitEnvelope`/`requestEnvelope`; processor additions: `getScratchEnvelopeJson()`, `uiRatio/uiScratchPhase/uiState` atomics, `uiEnvGeneration`; editor `startTimerHz(30)` emitting `transportFrame` (+`envelopeState` on generation change) via `emitEventIfBrowserIsVisible`.
5. **Ratio indicator:** DOM bipolar bar (−2×…+2×, zero-anchored fill, distinct reverse styling) + 2-decimal ×-readout in TRIGGER panel; fed by the same 30 Hz event; change-gated writes.
6. **Mode switch:** instant `.hidden` toggle of two absolutely-positioned panes in a fixed center box; optional 120 ms opacity polish only if Standalone iteration wants it.
7. **Verification gates:** re-run render harness (47 probes) after processor edits; grep-diff native-fn names; auval + build-and-install sweep per CLAUDE.md; TDZ smoke = load page against a Juce-bridge stub / check console clean.
