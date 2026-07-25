# Stage 4: Polish / Validation — Execution Plan

**Created:** 2026-07-24
**Source of truth:** `RESEARCH.md` (this directory) — its §8 gate deltas **override** CONTEXT.md wherever they conflict. `CONTEXT.md` D11–D16 / C1–C10 — decisions and constraints locked.
**Reference implementation:** `plugins/O-Contrabass/{CMakeLists.txt, Source/PluginProcessor.cpp, Source/PluginEditor.cpp, Source/ui/public/index.html}` — copy *structure*, with two mandatory deviations (F6 MSVC SafePointer, Naturalist styling).

---

## Goal

Close O-ReverseDelay at ship-ready **v1.0.0**: close the D11 wash-decay question by ear, integrate OuariconPresetManager v1.0.5 with 8 factory presets, ship a 44 px preset bar (940×**484**) and tooltips on all 10 controls, re-confirm the full validation surface, and author the CHANGELOG — with **zero regression** to the Stage-3 GUI, which is a live surface here, not a frozen asset.

**Ground truth verified this session (all seven research findings confirmed against the tree):**

| Fact | Evidence |
|------|----------|
| `preset-manager.js` fetches **10** native fns incl. `savePresetWithDialog` | `modules/persistence/preset-manager/js/preset-manager.js:89-98` |
| Module version **1.0.5**; `registry.yaml` stale | `module.yaml` (`version: 1.0.5`) — note its own `native-functions:` list is stale at 9, missing `savePresetWithDialog`; the JS is authoritative |
| Editor has exactly **1** native fn today | `Source/PluginEditor.cpp:114` ≡ `app.js:266` |
| Harness inherits the plugin's include dirs → **no harness CMake edit** | `tests/render-harness/CMakeLists.txt:37-38` (`$<TARGET_PROPERTY:OuariconReverseDelay,INCLUDE_DIRECTORIES>`) |
| `ui_frontend_check.js` §3 hard-asserts surface **== 1**, scanning `appJs` only | `tests/ui_frontend_check.js:110-131` |
| `ui_frontend_check.js` §9 embedded regex is `Source/ui/public/(\S+)`; only **static** `from "./…"` imports harvested | `tests/ui_frontend_check.js:209, 226` |
| `serve-stub.sh` copies `Source/ui/public/.` only | `tests/ui-stub/serve-stub.sh:16` |
| Stub `getNativeFunction` rejects every name but `getParameterDefaults` | `tests/ui-stub/juce-stub.js:106-110` |
| Edit sites | `PluginEditor.cpp:168` (setSize) · `styles.css:54, 88` (heights) · `index.html:18/21` (bar insertion) · `PluginProcessor.cpp:361` (feedback tap) · `CMakeLists.txt:25-28, 59-68` |

**Plan-level decisions on RESEARCH §10 open items:**

1. **Makeup constant** — conditional Task 3, `k = 2.0f` pencilled in. Not authorized until Task 1 reports.
2. **`highCut` audit tolerance** — start at `< 2.0` Hz; Task 7 prints the actual round-trip delta and the tolerance is set from the measurement, not assumed.
3. **Near-Infinite render length** — **30 s** (vs 10 s for the other seven). Costs ~20 s of harness runtime for a materially stronger DSP-03 statement.
4. **Delete button — YES, the bar ships five controls, not D15's four.** CONTEXT human-checklist item 6 ("Save a user preset, reload it, delete it — round-trip through the new bar") is already locked and is unreachable without it. D15's ASCII sketch is schematic, not a control inventory. Bar reads `◀ ▶ [ name ] Save Load Delete`. **Flagged for the user** — if the bar must stay at four, drop Delete and checklist item 6 must be re-scoped in the same breath.

---

## Phase 4.0 — Entry Gate (blocking; no product code)

CONTEXT forbids any Stage-4 code before this phase is green.

### Task 1: D7/D6 Standalone audition — HUMAN, BLOCKING

**Action:** `/show-standalone O-ReverseDelay`. Judge by ear: smear character, **wash length**, width.
The specific question: at `feedback=100`, does the tail decay too fast? Stage 2 measured an inherent **−7.3 dB/generation** pre-damping (−4.3 dB Hann² duty + −3.0 dB pan→mono-sum round trip; linear loop gain ≈ 0.43×).

**Outcome is a recorded decision, either way:**
- *"wash too short"* → Task 3 runs with `k = 2.0f`.
- *"wash is right"* → makeup constant **declined**, recorded in SUMMARY.md. Stage 4 carries **zero** DSP diff.

**Depends on:** nothing. **Gate:** the D11 question is *closed* (CONTEXT success-criterion #1 requires closure, not silence).

### Task 2: Harness baseline re-run — 33/33

**Action:**
```bash
cmake -B build -G Ninja -DOUARICON_BUILD_TESTS=ON
ninja -C build O-ReverseDelay-render-test && ./build/.../O-ReverseDelay-render-test
```
Expect **33/33 PASS, exit 0** against the *current* tree.

**Why first:** `pattern_render_harness_breaks_on_webview_editor` — the harness can go un-buildable the moment the editor gains new types, and it must be proven green *before* the preset diff so any later failure is attributable.

**Depends on:** nothing (runs in parallel with Task 1).
**Gate:** exit 0. If it fails to build, fix the harness before anything else.

### Task 3 (CONDITIONAL — only if Task 1 says "too short"): feedback makeup constant

**Files (modify):** `Source/PluginProcessor.cpp:361`

```cpp
// Compensates the topology's inherent −7.3 dB/generation (Hann² duty + pan→monoSum).
// Pre-tanh by construction: the soft-clip still bounds the loop at any setting,
// so this changes how LONG the tail lasts, never WHETHER it is bounded (DSP-03).
static constexpr float kFeedbackMakeup = 2.0f;   // +6.0 dB → residual 0.86 (−1.3 dB/gen)
...
const float g = feedbackSmoothed.getNextValue() * kFeedbackMakeup;
```

**Do NOT use 2.32× (full compensation).** It puts the loop at unity pre-damping and hands all decay authority to the two damping filters — a materially different plugin from the one Stage 2 verified.

**Re-gate (all four, in order):**
1. Full harness **33/33** — not just probe G.
2. **Probe G** (60 s @ fb=100): `peak < 1.0`, `tailPeak < 1.0`, zero NaN/Inf **unchanged**, plus a **new assertion** that `washRms[5..10 s]` has *increased* vs the pre-change baseline captured in Task 2 (proves the constant did something). This is the +1 check → harness becomes 34 at this point, 42 at exit.
3. **Probe M** re-run — its two-tier click detector runs with the loop live; confirm the `kStepFactor 8.0` latched-param tier still holds.
4. **Re-audition** in Standalone.

**Depends on:** Tasks 1, 2.

---

## Phase 4.1 — Preset System (C++ / CMake / harness)

### Task 4: CMake wiring — two additive edits, ONE binary-data target

**Files (modify):** `plugins/O-ReverseDelay/CMakeLists.txt`

```cmake
target_include_directories(OuariconReverseDelay
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp)   # header-only, no vendored copy

juce_add_binary_data(OuariconReverseDelay_UIResources
    NAMESPACE UIBinaryData
    HEADER_NAME UIBinaryData.h
    SOURCES
        ...existing six...
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/js/preset-manager.js)
```

**C3 — do NOT add a second `juce_add_binary_data` target.** A default `NAMESPACE BinaryData` duplicate-symbols against `UIBinaryData` (`critical_dual_binary_data_namespace_collision`). Symbol becomes `UIBinaryData::preset_manager_js` / `preset_manager_jsSize`.

**No harness CMake edit** — it inherits the include via `$<TARGET_PROPERTY:...>` (verified). `OuariconPresetManager` is header-only, needs only `juce_core`/`juce_data_structures`/`juce_audio_processors` (all linked) and no WebView, so it compiles under `JUCE_WEB_BROWSER=0`.

**Depends on:** Phase 4.0 green.

### Task 5: Processor integration + factory table

**Files (modify):** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`

**H:** `#include "OuariconPresetManager.h"`; public accessor `OuariconPresetManager& getPresetManager() noexcept { return presetManager; }`; member `OuariconPresetManager presetManager { parameters, "O-ReverseDelay" };` declared **after** `parameters`.

Hardcode `"O-ReverseDelay"` — **no `OUARICON_DEV_SUFFIX`** — so dev and release builds share one library (matches every sibling). Real path is `~/Library/O-ReverseDelay/Presets/{Factory,User}/` (**F5** — CONTEXT's `Application Support` path is wrong; the human checklist and CHANGELOG must use the corrected path).

**CPP — state routing** (replaces the current bodies at `:467` / `:474`):
```cpp
void ReverseDelayProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary (*xml, destData);
}

void ReverseDelayProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        presetManager.setStateFromXml (xml.get());
}
```
`getStateAsXml()` wraps the **same** APVTS root plus a `currentPreset` attribute and `setStateFromXml()` accepts plain pre-Stage-4 APVTS XML → backward compatible with Stage 1–3 sessions. No `setCustomStateCallbacks` (no state outside the APVTS).

**CPP — factory table** in the constructor, after `parameters` is live. All eight carry **all ten keys explicitly**.

| # | Name | sync | div | delay ms | grain ms | dens % | fb % | loCut Hz | hiCut Hz | width % | mix % |
|---|------|------|-----|---------:|---------:|-------:|-----:|---------:|---------:|--------:|------:|
| 1 | Reverse Bloom | 0 | 6 | 500 | 200 | 60 | 40 | 100 | 8000 | 60 | 40 |
| 2 | Guitar Swell | 0 | 6 | 700 | 300 | 55 | 45 | 120 | 6500 | 55 | 55 |
| 3 | Vocal Halo | 0 | 6 | 380 | 180 | 70 | 30 | 300 | 7000 | 70 | 25 |
| 4 | Slow Wash | 0 | 6 | 1400 | 450 | 30 | 65 | 80 | 5000 | 85 | 50 |
| 5 | Tight Smear | 0 | 6 | 180 | 70 | 90 | 35 | 150 | 11000 | 35 | 45 |
| 6 | Dark Cavern | 0 | 6 | 850 | 320 | 65 | 70 | 220 | 1800 | 75 | 55 |
| 7 | Near-Infinite | 0 | 6 | 900 | 350 | 70 | **100** | 180 | 2500 | 80 | 50 |
| 8 | Rhythmic Reverse | **1** | **4** (1/8D) | 500 | 120 | 80 | 50 | 140 | 9000 | 50 | 45 |

**C1 — values authored in ENGINEERING UNITS, converted through the params' own ranges:**
```cpp
// C1: engineering units → normalised through each param's own NormalisableRange.
// Handles skew, int, and choice uniformly. A hand-written fraction on any of the
// four skewed params recalls 10–30× wrong (pattern_factory_preset_normalized_ignores_skew).
for (auto& preset : factoryPresets)
    for (auto& [id, value] : preset.parameters)
        if (auto* p = parameters.getParameter (id))
            value = p->convertTo0to1 (value);

presetManager.initializeFactoryPresets (factoryPresets);
```
Spot-check against the Stage-3 verify midpoints — normalised 0.5 ⇒ delayTime **316 ms**, grainSize **158 ms**, lowCut **200 Hz**, highCut **3162 Hz**.

- **C8:** no `/` in any name — #8 is "Rhythmic Reverse", never "Reverse 1/8".
- **C2 needs zero code (F8):** v1.0.5 `applyPresetJson()` already resets every `RangedAudioParameter` to default (meta-first) before applying. **Verify by reading the module, do not write a reset pass.**
- `AudioParameterChoice` range is `0 … n-1` step 1, so the same loop handles `syncMode`/`noteDivision` (`convertTo0to1(6.0f)` on the 13-entry division list = 0.5).

**Depends on:** Task 4.

### Task 6: Editor — 10 native fns, MSVC-safe; resource branch

**Files (modify):** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`

**H — new members:** `std::shared_ptr<juce::FileChooser> fileChooser;` and `bool fileDialogOpen = false;` (re-entrancy guard: a second click while a dialog is up must complete `{false, ""}` immediately, never stack a chooser). Update the header comment block — it currently states "Exactly ONE native function."

**CPP — register all ten** alongside the existing `getParameterDefaults` (total **11**):
`savePreset` · `savePresetWithDialog` · `loadPreset` · `loadPresetFromFile` · `getPresetList` · `getCurrentPreset` · `selectNextPreset` · `selectPreviousPreset` · `deletePreset` · `isFactoryPreset`

**Both dialog fns must resolve `{success: bool, name: string}`** — a bare bool silently no-ops the bar (`saveWithDialog()` checks `result && result.success`).

**F6 — MANDATORY deviation from O-Contrabass.** Its dialog lambdas write `[safeThis = juce::Component::SafePointer<...>(this), ...]` *inside* the `withNativeFunction` lambda. On MSVC `this` in a nested lambda's capture-initialiser resolves to the **enclosing closure** → hard compile error, invisible to Apple Clang, kills Windows CI and blocks `/publish` (`critical_msvc_safepointer_init_capture_nested_lambda`; O-Lyrica died on exactly this). **Hoist to a local first:**
```cpp
.withNativeFunction ("savePresetWithDialog", [this] (const auto&, auto complete)
{
    auto safeThis = juce::Component::SafePointer<ReverseDelayEditor> (this);   // hoisted FIRST
    ...
    fileChooser->launchAsync (flags, [safeThis, complete, makeResult] (const juce::FileChooser& fc)
    {
        if (safeThis == nullptr) return;   // C9: BARE return — complete() here is itself a UAF
        ...
    });
})
```
`savePresetWithDialog` honours the directory the user picked (O-Wind WR-12):
`result.isAChildOf (pm.getUserPresetsDirectory()) ? pm.savePreset (name) : pm.savePresetToFile (result.withFileExtension ("json"))`.

**Resource provider:** add a `getResource()` branch for `"/js/preset-manager.js"` → `UIBinaryData::preset_manager_js`, mime `application/javascript; charset=utf-8`.

**Depends on:** Task 5.

### Task 7: Harness probe N — factory-preset audit (33 → 41)

**Files (modify):** `tests/render-harness/main.cpp`

Place **last, after probe M** — it mutates APVTS state and leaves the plugin on the final preset's values, so nothing may run after it. Audit through the **shipping** `OuariconPresetManager` (not a re-typed normalised table) — that is the only thing that proves the engineering-unit → `convertTo0to1` → `convertFrom0to1` round-trip survives skew.

```cpp
// --- Probe N: factory-preset audit (D16 / C1) --------------------------------
for (const auto& e : kFactoryExpect)          // the §3 table, in ENGINEERING units
{
    const bool loaded = proc.getPresetManager().loadPreset (e.name);
    proc.prepareToPlay (fs, block);

    // (a) skew round-trip — the C1 assertion
    const bool values = loaded
        && std::abs (paramValue (apvts, "delayTime") - e.delay) < 0.5f
        && std::abs (paramValue (apvts, "grainSize") - e.grain) < 0.5f
        && std::abs (paramValue (apvts, "lowCut")    - e.lo)    < 0.5f
        && std::abs (paramValue (apvts, "highCut")   - e.hi)    < 2.0f
        && /* density, feedback, width, mix + the two choice indices */ ;

    // (b) render — finite, bounded, non-silent
    auto y = renderEffect (proc, e.seconds, fs, block, excite2s);   // 10 s; 30 s for Near-Infinite
    const bool audio = allFinite (y.L) && allFinite (y.R)
                    && juce::jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0
                    && rms (y.L, (int)(6.0*fs), (int)(2.0*fs)) > 1.0e-7;

    check (e.name, values && audio, ...);
}
```

- One `check()` per preset → **8 new checks, harness 33 → 41** (42 if Task 3 shipped).
- **Near-Infinite renders 30 s** at `feedback=100` / damping 180 Hz–2.5 kHz — the preset-driven DSP-03 statement D16 promised.
- `Rhythmic Reverse` needs a playhead supplying BPM, or it exercises the COMPAT-02 no-BPM fallback — either is valid, but **assert the one you chose**.
- **Print the actual per-param deltas** on the first run and set the `highCut` tolerance from the measurement (open item #2), rather than shipping the assumed `< 2.0`.

**Depends on:** Tasks 5, 6 (build must be clean).

### Task 8: Factory re-seed discipline (F7 dev-loop trap)

`initializeFactoryPresets()` early-returns when `Factory/.factory-version` already holds `JucePlugin_VersionString`. O-ReverseDelay is frozen at **1.0.0** for all of Stage 4, so **every factory-table edit after the first run is a silent no-op.**

**After ANY edit to the factory table — every time, no exceptions:**
```bash
rm -rf ~/Library/O-ReverseDelay/Presets/Factory
```
Then re-run the harness (it constructs the processor, so it re-seeds from a clean dir) and confirm 8 `.json` files + `.factory-version` land in `~/Library/O-ReverseDelay/Presets/Factory/`.

**Depends on:** Task 7.
**Phase 4.1 gate:** build clean · harness **41/41 exit 0** · 8 factory JSONs verifiably re-seeded after an `rm -rf`.

---

## Phase 4.2 — Preset Bar + Tooltips (frontend)

### Task 9: Geometry — 940 × 484

| File | Line | Change |
|------|------|--------|
| `Source/PluginEditor.cpp` | 168 | `setSize (940, 440)` → `setSize (940, 484)` |
| `Source/ui/public/css/styles.css` | 54 | `html, body { height: 440px }` → `484px` |
| `Source/ui/public/css/styles.css` | 88 | `.frame { height: 440px }` → `484px` |

`.frame` is `flex-direction: column` with `.groups { flex: 1 }` and fixed-height (215 px) `align-items:center` panels — adding 44 px of band *and* 44 px of frame height consumes exactly the new slack, leaving panel heights and the footer untouched. This is why D15 is the low-regression choice.

**The `.time-slot` box legitimately shifts `y` by +44** (Stage-3 verify recorded `x:117 y:198 w:86 h:100`). The re-verify assertion is **mode-invariance** — identical box in Free and Sync — **never** the old absolute `y:198`. Update any assertion that pinned the absolute y.

**Depends on:** Phase 4.1 gate.

### Task 10: Bar markup + Naturalist CSS

**Files (modify):** `Source/ui/public/index.html` (insert between `</header>` at :18 and `<main class="groups">` at :21), `Source/ui/public/css/styles.css`

Five controls (see plan-level decision #4): `◀ ▶ [ name ] Save Load Delete`.

**Styling — borrow O-Contrabass's *structure*, dress it in O-ReverseDelay's own vocabulary.** The aesthetic (`ouaricon-naturalist-001`) has **no preset-bar component** (confirmed). O-Contrabass's `.preset-bar` is a dark brown strip that *carries the plugin name* — importing it would give this page two competing title bars.

- **Band:** transparent over the paper gradient; hairline `1px solid rgba(139,115,85,0.45)` **below** it, mirroring `.header`'s existing bottom rule. Reads as a second field-guide register line, not chrome.
- **Buttons:** reuse the existing `.segment` treatment (`styles.css:273-314`) — `var(--btn-default)` fill, `2px solid var(--btn-border)`, 5 px radius, `translateY(-1px)` hover lift. Nav arrows 26×26 squares; Save/Load/Delete `padding: 5px 12px`, 10 px uppercase serif.
- **Name field:** recessed cartouche identical to `.division-select` — `background: var(--bg-paper-light)`, `2px solid var(--brown-border)`, `inset 1px 1px 3px rgba(0,0,0,.18)`; **italic** serif 13 px, `width: 300px`, `white-space:nowrap; overflow:hidden; text-overflow:ellipsis`.
- Flank the row with the existing `❧` fleuron to match the footer's typographic furniture.

**IDs the module binds:** `#preset-name` `#preset-prev` `#preset-next` `#preset-save` `#preset-load` `#preset-delete`.

**C6 — button copy lives in HTML, never in JS:**
```html
<button id="preset-delete" data-label="Delete" data-confirm="Confirm?">Delete</button>
```
The `#preset-name` element is the *exception* — `_updateDisplay()` writing `textContent` is the module's intended behaviour on its own element (`preset-manager.js:363`). Author its placeholder as **`Default`**: a fresh instance genuinely reports `"Default"`, which is deliberately not a list member.

**Preset list order** (Factory+User, case-insensitive sort) — affects ◀ ▶ and human-checklist item 5:
`Dark Cavern · Guitar Swell · Near-Infinite · Reverse Bloom · Rhythmic Reverse · Slow Wash · Tight Smear · Vocal Halo`.
A fresh instance lands on **`Default`**, *not* "Reverse Bloom" — D16 called that one "default-adjacent" about its **values**, not its position.

**Depends on:** Task 9.

### Task 11: `initPresetBar()` — TDZ-safe wiring + inline delete confirm

**Files (modify):** `Source/ui/public/js/app.js`

**C7 is the whole risk here.** `app.js` currently ends with a single bottom-of-file `init();` and `ui_frontend_check.js` §2 asserts `init();` is the last meaningful line with **no module-level declaration after it**. A top-level init touching state declared lower down throws a ReferenceError that escapes module evaluation and **silently kills every already-working knob** — build, auval and static checks all still pass (`pattern_module_toplevel_init_tdz`).

So, exactly:
- Declare `let presetManager = null;` in the **existing top block** beside `paramDefaults` (`app.js:60`).
- Add a **hoisted** `async function initPresetBar()` among the other function declarations.
- Call it **from inside `init()`**, fire-and-forget like `loadParameterDefaults(Juce)` at `:279`. **No** second top-level call. **No** module-top-level `import()`.

```js
async function initPresetBar() {
  try {
    const { PresetManager } = await import("./preset-manager.js");
    presetManager = new PresetManager({
      displayElement: document.getElementById("preset-name"),
      prevButton:  document.getElementById("preset-prev"),
      nextButton:  document.getElementById("preset-next"),
      saveButton:  document.getElementById("preset-save"),
      loadButton:  document.getElementById("preset-load"),
      deleteButton: document.getElementById("preset-delete"),
      getNativeFunction: Juce.getNativeFunction,
      onConfirmDelete: confirmDeleteInline,
    });
    await presetManager.initialize();
  } catch (e) {
    console.error("[preset-bar] init failed:", e);   // bar dies alone; the 10 knobs survive
  }
}
```
The `try/catch` is **load-bearing** — it contains a bar failure so the ten already-verified controls are not taken down with it.

**Delete confirmation — never `window.confirm`.** `promptDelete()` falls back to it, and it is a silent no-op or throw in some JUCE WebView backends (module IN-05). Human-checklist item 6 needs a working round-trip:
```js
// Two-click inline confirm. Copy comes from HTML data-attrs (C6), never invented by JS.
function confirmDeleteInline(name, _message) {
  const btn = document.getElementById("preset-delete");
  if (!btn) return false;
  if (btn.dataset.armed === "1") { disarm(btn); return true; }
  btn.dataset.armed = "1";
  btn.textContent = btn.dataset.confirm;
  setTimeout(() => disarm(btn), 3000);
  return false;
}
function disarm(btn) { btn.dataset.armed = "0"; btn.textContent = btn.dataset.label; }
```

**Import specifier:** `preset-manager.js` is served at `/js/preset-manager.js` and `app.js` lives at `/js/app.js` → `"./preset-manager.js"`. (Task 14 handles the fact that `ui_frontend_check.js` cannot see a dynamic `import()`.)

**Depends on:** Task 10.

### Task 12: Tooltips on all 10 controls

**Files (modify):** `Source/ui/public/index.html`, `css/styles.css`, `js/app.js`

Ten anchors: the 8 `.knob` elements, `#combo-noteDivision`, `#syncSegments`. **No help toggle, no persistence** — O-MultiBandCompressor's version adds a `setTooltipsEnabled` native fn, which would push the bridge to 12 and add state D13 explicitly excluded.

**Mechanism — lift `showTooltip()` from `plugins/O-MultiBandCompressor/Source/ui/public/js/app.js` (~:940) verbatim.** It already carries the v1.4.1 measure-then-pin fix that **C5** demands:
```js
tooltipEl.style.width = '';        // release
tooltipEl.style.left  = '0px';     // measure from the LEFT edge, not the previous offset
tooltipEl.style.top   = '0px';
const width = tooltipEl.getBoundingClientRect().width;
tooltipEl.style.width = `${width}px`;                        // PIN before placing
const height = tooltipEl.getBoundingClientRect().height;     // stable only once width is definite
```
Measuring at the previous offset under-reports width; applying a near-edge `left` afterwards re-wraps a 230 px tip into a ~70 px ribbon. **In O-ReverseDelay the exposed control is `mix`** — right-most, OUTPUT panel — exactly C5's prediction. Invisible to build/auval/pluginval.

Also copy: clamp `left` into `[MARGIN, innerWidth - width - MARGIN]`; prefer `above`, flip `below` only when `top < MARGIN`; drive the arrow with `--arrow-x` so it still points at the control after clamping; suppress on `pointerdown` / re-enable on `mouseup` (`tooltipSuppressed`) so a tip does not hang over a knob mid-drag.

**CSS:** `.tooltip`, `.tooltip-title`, `.tooltip-body`, `.tooltip::after` transplant nearly unchanged — O-MBC's palette is already Naturalist. Swap literals for `var(--bg-paper-light)`, `var(--brown-frame)`, `var(--green-mid)`, `var(--brown-text)`. `position: fixed; z-index: 1000` puts it above `.botanical-overlay` (z-index 1) and outside `.frame`'s inset shadow.

**Copy — authored as `data-tip-title` / `data-tip` attributes in index.html, not a JS table** (C6: copy is content, content lives in HTML):

| Anchor | Title | Body |
|--------|-------|------|
| `#syncSegments` | Sync Mode | Free reads the delay in milliseconds; Sync locks it to the host's tempo grid. |
| `#combo-noteDivision` | Division | The note value the delay follows while Sync is lit — dotted (D) and triplet (T) included. |
| `#knob-delayTime` | Delay | How far back the grains reach. Long settings read as separate reversed phrases; short ones fuse into a smear. |
| `#knob-grainSize` | Grain Size | Length of each reversed fragment. Long grains bloom and swell; short grains chatter. |
| `#knob-density` | Density | How many grains overlap at once. Sparse settings stutter; dense settings pour. |
| `#knob-feedback` | Feedback | How much of the wash returns to the buffer. Each pass re-reverses, so the tail keeps folding back on itself. |
| `#knob-lowCut` | Low Cut | Trims low frequencies inside the feedback loop — every pass grows lighter. |
| `#knob-highCut` | High Cut | Trims high frequencies inside the feedback loop — every pass grows darker and further away. |
| `#knob-width` | Width | Spreads grains across the stereo field. At zero they stack in the centre. |
| `#knob-mix` | Mix | Balance of dry input against the reversed wash. Equal-power, so the total stays level. |

**Depends on:** Task 10 (independent of Task 11).

### Task 13: Repair the browser stub (F4 — two independent blockers)

The stub render is the automatable gate for the bar. Two things break it today.

**13a — `tests/ui-stub/serve-stub.sh`:** it copies `Source/ui/public/.` only; `preset-manager.js` lives in the module tree and is embedded straight from `modules/…` by CMake, so `await import("./preset-manager.js")` **404s**. One line after the existing `cp` at `:16`:
```bash
cp "$HERE/../../../../modules/persistence/preset-manager/js/preset-manager.js" "$ROOT/js/preset-manager.js"
```
This keeps the script's promise intact — what is served stays byte-identical to what the resource provider serves.

**13b — `tests/ui-stub/juce-stub.js`:**
1. No `window.__JUCE__` → `_waitForNative()` polls **100 × 50 ms = 5 s** then `console.error`s (`preset-manager.js:129`). That alone fails the zero-console-errors criterion *and* adds 5 s to every stub run.
2. `getNativeFunction()` (`:106-110`) **rejects every name but `getParameterDefaults`** — by design, as a bridge-gap detector. All ten preset calls would reject.

```js
// The module polls window.__JUCE__.backend before doing anything; without this
// shim _waitForNative() burns 5 s and console.errors (preset-manager.js:129).
if (typeof window !== "undefined") window.__JUCE__ = { backend: {} };

const FACTORY = ["Dark Cavern", "Guitar Swell", "Near-Infinite", "Reverse Bloom",
                 "Rhythmic Reverse", "Slow Wash", "Tight Smear", "Vocal Halo"];
const userPresets = new Set();
let currentPreset = "Default";
const PRESET_FNS = { savePreset, savePresetWithDialog, loadPreset, loadPresetFromFile,
                     getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset,
                     deletePreset, isFactoryPreset };
```
Gate `getNativeFunction` on `name === "getParameterDefaults" || name in PRESET_FNS`, **rejecting everything else** — the detector's value is preserved, its whitelist just grows 1 → 11. **Both dialog fns must resolve `{success, name}`** in the stub too, or `saveWithDialog()` silently reports failure.

**Depends on:** Tasks 11, 12.

### Task 14: `ui_frontend_check.js` — REPAIR, then extend

Two sections **FAIL on correct Stage-4 code** and must be fixed before any extension is meaningful:

**§3 (`:110-131`) — native-fn bridge gaps.** All ten preset `getNativeFunction` calls live in `preset-manager.js`; `app.js` keeps exactly one (`getParameterDefaults`). The check reads `appJs` only and hard-asserts `called.size === 1 && registered.size === 1` → would read **1 ≡ 11** and false-FAIL.
- Build `called` from **`appJs` + `presetManagerJs`** (read from `modules/persistence/preset-manager/js/preset-manager.js`).
- Change the surface assertion to **11**.
- **Leave the `missing` / `dead` set logic untouched** — that is the part with real value (C4: an unregistered fn passes build/auval/pluginval while the control is silently dead).

**§9 (`:209, 226`) — three-way closure.** The embedded-set regex is `/Source\/ui\/public\/(\S+)/g`; the new CMake entry is a `modules/…` path, so `embedded` lacks `/js/preset-manager.js` while `provided` has it → `notEmbedded` FAILs on correct code. Also the ref harvester matches only **static** `from "./…"` imports, so the dynamic `await import("./preset-manager.js")` is never seen.
- Add a second extraction: `modules\/.*\/js\/([^/\s]+\.js)` → `/js/$1`, mirroring how `getResource()` serves it.
- Add `refs.add('/js/preset-manager.js')` explicitly (or extend the regex to catch `import(`).
- The on-disk existence check must resolve module-tree entries against the repo root, not `publicDir`.

**Then extend** with new assertions: bar element IDs present in HTML; `data-label`/`data-confirm` on `#preset-delete`; `initPresetBar` called from **inside** `init()` and not at top level; all 10 tooltip anchors carry `data-tip` + `data-tip-title`; the measure-then-pin sequence present (width pinned **before** `left` is applied); heights read `484px` in both CSS spots and `setSize (940, 484)` in the editor.

**Depends on:** Tasks 9–13.

### Task 15: Phase 4.2 gate — stub render + grep-diff

1. `tests/ui-stub/serve-stub.sh` → browser at the served root. **Exactly 940×484**, zero overflow, **zero JS console errors** (the `__JUCE__` shim must have eliminated the 5 s `_waitForNative` error; a stub-server favicon 404 remains the one known benign entry, absent in the WebView path).
2. Drive the bar in the browser: ◀ ▶ cycle the 8 names · Save · Load · Delete two-click confirm.
3. **Hover the `mix` knob** — its tooltip must be full width, **not** shrink-wrapped (C5).
4. `.time-slot` box **identical across Free and Sync** (mode-invariance, not absolute y).
5. `node tests/ui_frontend_check.js` → **exit 0**.
6. Native-fn grep-diff reads **11 ≡ 11** (`app.js` + `preset-manager.js` vs `PluginEditor.cpp`).

**Depends on:** Task 14.

---

## Phase 4.3 — Validation + Release Prep

### Task 16: Build + install with dual-variant sweep

```bash
./scripts/build-and-install.sh O-ReverseDelay
```
Phase 4 of the script sweeps **both** `-dev` and unsuffixed bundles and clears the AU cache (C10) — a leftover alternate variant pins Logic's registry slot to whichever was installed first. Watch for the `⚠ Sweeping ALTERNATE-variant` warning.

**Depends on:** Phase 4.2 gate.

### Task 17: Full validation surface

- `pluginval` strictness **10 ×3** — VST3 (3/3, zero failures)
- `pluginval` strictness **10 ×3** — AU (3/3, zero failures)
- `auval -v aufx ORvD OuDv` → SUCCEEDED
- **Harness exit re-run: 41/41, exit 0** (42 if Task 3 shipped) — the ×3 repeat gate is Stage 4's; Stage 3 only ran singles.

Windows is **explicitly deferred to CI** (D13/D14) — the CMake already carries `NEEDS_WEBVIEW2` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, verified statically in Stage 3, and Task 6's hoisted SafePointer is what keeps that deferral safe.

**Depends on:** Task 16.

### Task 18: CHANGELOG at v1.0.0

**Files (create):** `plugins/O-ReverseDelay/CHANGELOG.md` — does not exist; authored from scratch.

Shape follows O-Contrabass: `# Changelog — O-ReverseDelay`, a note that 1.0.0 is the first shipped version, then `## [1.0.0] — 2026-07-XX — first release` with an `### Added` block per stage and a validation line quoting the **actual** harness / auval / pluginval numbers from Task 17 (not the planned ones). Confirm `VERSION 1.0.0` still reads correctly at `CMakeLists.txt:11` and that the AU component version is **65536**.

Record in the CHANGELOG and SUMMARY.md: the D11 makeup-constant outcome (**implemented at k=2.0×** *or* **explicitly declined**) and the corrected preset path `~/Library/O-ReverseDelay/Presets/{Factory,User}/`.

**Depends on:** Task 17.

### Task 19: Hand the human checklist to the user

Batched at the very end, one pass:
1. **D7 audition** — re-confirmed after any makeup-constant edit.
2. Load in **Logic and Ableton**: WebView renders at 940×484; automation round-trips in-host.
3. Mono→stereo listen (Stage-1 carryover; probe measured Δ0.0000 dB but never heard).
4. Session save/reload round-trip (Stage-1 carryover).
5. All 8 factory presets audibly distinct, none runaway/NaN — especially **Near-Infinite**.
6. Save a user preset → reload → delete, through the new bar.
7. All 10 tooltips appear on hover; the right-most (`mix`) tooltip is **not** shrink-wrapped.

Stage-4 verify records these as `human_needed` until confirmed.

**Depends on:** Task 18.

---

## Success Criteria (Stage 4 complete when)

1. D11 makeup-constant question **closed** — implemented with probe-G delta green, or explicitly declined and recorded in SUMMARY.md.
2. Render harness green **at entry (33/33)** and **at exit (41/41**, 42 if the constant shipped**)**, exit 0.
3. OuariconPresetManager **v1.0.5** integrated via CMake-include; 8 factory presets authored in **engineering units** + `convertTo0to1`; all 8 audited clean through the real `loadPreset()`; re-seed verified after an `rm -rf`.
4. Preset bar shipped at **940×484**; `ui_frontend_check.js` **repaired** (§3, §9) **and** extended, exit 0; native-fn grep-diff **11 ≡ 11**.
5. Tooltips on all 10 controls, edge-safe — `mix` verified full-width.
6. Stub render: exactly 940×484, zero overflow, **zero console errors**; bar round-trip driven in the browser.
7. pluginval strictness 10 **×3** on VST3 **and** AU, zero failures; `auval` SUCCEEDED.
8. Fresh install via `build-and-install.sh` with dual-variant sweep + AU cache clear.
9. CHANGELOG authored at v1.0.0; `VERSION 1.0.0` and AU component version 65536 confirmed.
10. 7-item human checklist handed to the user; Windows explicitly marked deferred-to-CI; `/publish` left as a separate user-triggered step.

---

## Risk Register (what silently passes every automated gate)

| Risk | Where it bites | Mitigation |
|------|----------------|------------|
| Unregistered native fn | Bar control dead; build/auval/pluginval all green | Task 15 grep-diff **11 ≡ 11**, scanning both JS files |
| TDZ throw in `app.js` | **All 10 knobs die**; build + static check pass | Task 11: hoisted fn, called from inside `init()`, `try/catch` |
| Factory preset normalized-vs-skew | Recalls 10–30× wrong; audibly "wrong preset" only | Task 5 `convertTo0to1` loop + Task 7 real-manager round-trip audit |
| `.factory-version` no-op at frozen 1.0.0 | Edited table never reaches disk; you debug the wrong thing | Task 8 `rm -rf` after **every** edit |
| MSVC nested `SafePointer(this)` | Windows CI compile error — invisible on macOS, blocks `/publish` | Task 6 hoisted local |
| Tooltip shrink-to-fit at right edge | `mix` tip becomes a ~70 px ribbon; invisible to every automated gate | Task 12 measure-then-pin + Task 15 manual hover |
| JS overwriting HTML-authored labels | Buttons read wrong text in every DAW since launch | Task 10 `data-label`/`data-confirm`; `#preset-name` is the sanctioned exception |
| `window.confirm` in WebView | Delete silently no-ops; checklist item 6 unpassable | Task 11 `onConfirmDelete` inline two-click |
| Absolute-y assertion on `.time-slot` | False FAIL after the legitimate +44 shift | Task 9: assert **mode-invariance**, never absolute y |

---

## Next Phase

Ready for: **execute** phase
