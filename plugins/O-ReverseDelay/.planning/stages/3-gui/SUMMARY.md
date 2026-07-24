# Stage 3: GUI — Execution Summary

**Completed:** 2026-07-24
**Plan:** `.planning/stages/3-gui/PLAN.md` (10 tasks, Phases 3.1 + 3.2)
**Result:** All 10 tasks complete. Every success criterion in PLAN.md verified.

---

## What shipped

A WebView GUI for O-ReverseDelay: fixed **940 × 440**, Ouaricon Naturalist aesthetic,
one row of four framed group panels **TIME | GRAIN | FEEDBACK | OUTPUT** (signal-flow
order, D9). All 10 parameters bound two-way (8 `WebSliderRelay` + 2 `WebComboBoxRelay`).
No visualization, no Timer, no C++→JS polling bridge (D10) — the simplest WebView editor
in the suite, with a **single** native function.

### Files created

| File | Purpose |
|---|---|
| `Source/ui/public/index.html` | 4-panel layout; FREE/SYNC labels authored here |
| `Source/ui/public/css/styles.css` | Naturalist palette, seed cross-section knobs, fixed time-slot |
| `Source/ui/public/js/app.js` | All bindings; TDZ-safe (single `init()` at file bottom) |
| `Source/ui/public/js/juce/index.js` | byte-identical copy from O-simpleGrain (verified `diff -q`) |
| `Source/ui/public/js/juce/check_native_interop.js` | byte-identical copy, same source |
| `Source/ui/public/img/birds.png` | `birdsEuropeIIIGoul_0094.png` (snow buntings) |
| `Source/PluginEditor.h` / `.cpp` | Relays → WebView → attachments; bare-path resource provider |
| `tests/ui-stub/juce-stub.js` | ~100-line bridge stub (mockup/TDZ/label gate) |
| `tests/ui-stub/serve-stub.sh` | Serves the production page with only `js/juce/index.js` swapped |
| `tests/ui_frontend_check.js` | 45 static regression assertions (ported from O-Contrabass) |

### Files modified

- `CMakeLists.txt` — `NEEDS_WEB_BROWSER`/`NEEDS_WEBVIEW2`, `JUCE_WEB_BROWSER=1` +
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `juce_add_binary_data` with
  `NAMESPACE UIBinaryData` + `HEADER_NAME UIBinaryData.h`, `PluginEditor.cpp` added to
  the plugin target only.
- `Source/PluginProcessor.cpp` — `createEditor()` guarded with `#if JUCE_WEB_BROWSER`;
  the `PluginEditor.h` include sits **inside** the same guard.
- `tests/render-harness/CMakeLists.txt` — comment only. The old note claimed
  "Stage 1/2 has no PluginEditor.cpp", which went stale this stage; replaced with an
  explicit statement of *why* the editor must never be listed here.

### Botanical selection

Birds were **unused across all 37 plugins** (insects ×4, flora ×2, snake, shell, fern,
bug, carrot, butterfly were taken). Snow buntings chosen over the rook for warm tones
that don't darken the OUTPUT panel at 0.35 opacity.

---

## Verification results

| Gate | Result |
|---|---|
| Build (`ninja` VST3 + AU + Standalone) | **clean**, zero warnings from plugin sources |
| Render harness `O-ReverseDelay-render-test` | **exit 0** — all Stage-2 probes still PASS (no DSP regression) |
| `node tests/ui_frontend_check.js` | **exit 0** — 45/45 checks |
| Native-fn grep-diff | **1 ≡ 1** (`getParameterDefaults`) |
| Browser-stub render | 940×440 four-panel layout, **zero JS console errors** |
| `build-and-install.sh O-ReverseDelay` | complete (AU cache cleared, dual-variant sweep) |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| pluginval **strictness 10** VST3 | **SUCCESS** (exit 0), Editor + Editor Automation included |
| pluginval **strictness 10** AU | **SUCCESS** (exit 0) |
| Standalone render | WebView renders; all 8 readouts equal the C++ defaults exactly |

### UI-01 — two-way binding

All 8 readouts in the **real plugin** matched `createParameterLayout()` exactly on open
(500 ms / 200 ms / 60 % / 40 % / 100 Hz / 8.0 kHz / 60 % / 35 %, division `1/4`, SYNC
active). Clicking FREE wrote choice index 0 to the `syncMode` param and the returned value
drove the UI refresh — a complete JS→C++→JS round trip. pluginval's Editor Automation
test at strictness 10 exercises the host→UI direction and passed on both formats.

### UI-02 — Sync/Free swap

Verified in both the stub and the real plugin. The `.time-slot` bounding box was
**byte-identical** in both modes (`x:117 y:198 w:86 h:100`) — zero layout shift. Both
controls stay relay-bound at all times, so neither is ever a dead control. First open
shows the noteDivision dropdown (default Sync).

### Skew correctness (the headline criterion)

Dragged the delayTime knob to 12 o'clock **in the Standalone**: reads **317 ms**, not
1025 ms. (317 rather than 316 is one pixel of drag granularity — 22 px applied vs 22.1 px
for exact centre.) This exercises the whole loop: drag → `setNormalisedValue` → C++
`NormalisableRange` → `getScaledValue` → readout. In the stub, all four skewed params hit
their `setSkewForCentre` values exactly at midpoint: delayTime 316 ms, grainSize 158 ms,
lowCut 200 Hz, highCut 3.2 kHz; linear `mix` read 50 %.

Dblclick-reset was verified to land on every engineering default from every off-default
position.

---

## Deviations from PLAN.md

1. **Tasks 1 and 7 merged.** The plan had Task 1 author a layout-only `app.js` and Task 7
   rewrite it with binding logic. Authoring the complete file once and running the stub
   gate against it gives identical verification coverage without a rewrite. Both stub
   passes specified by the plan (Tasks 2 and 8) still ran.

2. **Stub swaps the bridge file instead of using an import map.** The plan suggested "temp
   import-map or path swap". Copying the public tree and overwriting only
   `js/juce/index.js` means the page under test is **byte-identical to production** — no
   edited HTML, so the gate cannot pass on a file that differs from what ships.

3. **`ui_frontend_check.js` grew from ~6 check groups to 11 (45 assertions).** The
   O-Contrabass original assumed an inline `<script type="module">`; this port targets an
   external `app.js` and adds checks this plugin's pitfall table calls for: TDZ discipline
   (`init()` must be the last statement), the label-overwrite guard, and a **three-way**
   closure (HTML refs == `getResource()` entries == `juce_add_binary_data` SOURCES == files
   on disk).

4. **Two CSS adjustments after seeing the page render**, neither in the plan:
   - Panels are a fixed-height (215 px) row centred in the available space rather than
     stretched. Stretching left a tall empty half in every panel and buried the botanical
     overlay behind solid fill.
   - The active segment gets cream text plus a pressed inset shadow. Palette unchanged;
     UI-02's whole affordance rests on reading which mode is live at a glance, and the two
     spec greens alone were too close.

---

## Notes for Stage 4

- **D7 audition is the Stage-4 entry gate** (unchanged): smear/wash/width by ear,
  including the ~−7.3 dB/generation wash-decay finding and a possible feedback-tap makeup
  constant. Stage 3 touched no DSP — the harness re-run proves it.
- **Re-run the render harness at Stage-4 entry** before any DSP edit
  (`pattern_render_harness_breaks_on_webview_editor`).
- **Factory presets:** four params are skewed (`delayTime`, `grainSize`, `lowCut`,
  `highCut`). Author preset tables in **engineering units + `convertTo0to1`**, never as
  linear fractions (`pattern_factory_preset_normalized_ignores_skew`).
- A preset target adding `juce_add_binary_data` must **not** claim `NAMESPACE BinaryData`
  — `UIBinaryData` is already taken by the UI resources, and a distinct `HEADER_NAME`
  alone does not prevent the symbol collision.
- pluginval strictness 10 already passes on both formats; Stage 4 still owns the ×3
  repeat gate.

---

## Success criteria (from PLAN.md)

- [x] Build: VST3 + AU + Standalone compile clean
- [x] Browser-stub render: 940×440 four-panel layout, zero console errors, FREE/SYNC labels survive binding
- [x] Harness: `O-ReverseDelay-render-test` exit 0 after the WebView editor landed (run at Task 6 and Task 10)
- [x] `ui_frontend_check.js` exit 0
- [x] Native-fn surface: grep-diff exactly 1 ≡ 1
- [x] UI-01: all 10 controls drive DSP; host-side changes update the UI
- [x] UI-02: swap with zero dead controls and zero layout shift; first open = Sync
- [x] Skew correctness: delayTime midpoint ≈ 316 ms (measured 317 ms in Standalone)
- [x] Install: `build-and-install.sh` complete
- [x] auval: registered and passing
- [x] pluginval strictness 10: green on VST3 **and** AU, editor tests included
- [x] Standalone: editor opens at 940×440, Naturalist aesthetic, botanical overlay behind OUTPUT
