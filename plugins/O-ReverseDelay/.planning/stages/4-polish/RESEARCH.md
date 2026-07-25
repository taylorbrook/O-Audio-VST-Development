# Stage 4 (Polish / Validation) — RESEARCH

**Plugin:** O-ReverseDelay
**Stage:** 4 of 4 (Polish / Validation)
**Date:** 2026-07-24
**Inputs:** `stages/4-polish/CONTEXT.md` (D11–D16, C1–C10), Stage-3 VERIFICATION.md, Stage-2 SUMMARY.md
**Reference implementation:** **O-Contrabass** — the only sibling that combines preset-manager v1.0.x, the CMake-include pattern, a WebView preset bar, engineering-unit factory presets, AND is the source of O-ReverseDelay's `ui_frontend_check.js`. Read `plugins/O-Contrabass/{CMakeLists.txt, Source/PluginProcessor.cpp:250-375, Source/PluginEditor.cpp:134-238, Source/ui/public/index.html:93-155,599-605,1625-1645}`.

---

## Executive summary

Stage 4 is mostly a **wiring** stage — the module does the hard parts. But research turned up **six corrections to CONTEXT.md** that change the plan's task list and its exit gates. Three of them (F1, F3, F4) would otherwise land as a *red gate* during execute; two (F5, F6) are silent-until-later defects; one (F7) is a dev-loop trap that wastes an hour.

| # | Finding | Impact |
|---|---------|--------|
| **F1** | `preset-manager.js` requires **10** native fns, not 9 — D12 omitted `savePresetWithDialog`. Total surface is **11**, not 10. | C4 gate expectation wrong |
| **F2** | All 10 preset `getNativeFunction` calls live in `preset-manager.js`, **not** `app.js`. The grep-diff currently scans `app.js` only. | Gate reads 1 ≡ 11 → false FAIL |
| **F3** | `ui_frontend_check.js` §9 three-way closure **breaks** on a module-tree binary-data source (regex is `Source/ui/public/…`). | Hard FAIL on an otherwise-correct build |
| **F4** | Browser stub has no `window.__JUCE__` → `_waitForNative()` polls **5 s** then `console.error`s. Also `preset-manager.js` isn't under `Source/ui/public`, so `serve-stub.sh` never copies it → 404. | Stub gate unusable for the bar |
| **F5** | Preset dir is `~/Library/O-ReverseDelay/Presets/{Factory,User}/` — **not** `~/Library/Application Support/…` as D12 states. | Docs / human-checklist path |
| **F6** | O-Contrabass's dialog lambdas use the **MSVC-breaking** nested `SafePointer(this)` init-capture. Copying verbatim plants a Windows-CI compile error. | Blocks `/publish` |
| **F7** | `.factory-version` sentinel is keyed on `JucePlugin_VersionString`. At a frozen `1.0.0`, **edits to the factory table never re-seed**. | Dev-loop trap |

Good news: **C2 needs zero plugin-side work** (F8), and the harness inherits the preset-manager include for free (F9), which unlocks a *real* end-to-end preset audit rather than a re-typed table.

---

## 1 · Preset system (D12) — the actual contract

### F1 — The native-function surface is 11, not 10

`modules/persistence/preset-manager/js/preset-manager.js:88-98` fetches **ten** functions in `initialize()`:

```
savePreset            savePresetWithDialog   loadPreset       loadPresetFromFile
getPresetList         getCurrentPreset       selectNextPreset selectPreviousPreset
deletePreset          isFactoryPreset
```

D12's list has nine — it dropped **`savePresetWithDialog`**, which is the one the Save button actually calls (`saveButton → saveWithDialog() → _savePresetWithDialog`). O-Contrabass registers all ten and its own comment reads *"10 fns required by js/preset-manager.js"* (`PluginEditor.cpp:132`).

Both dialog fns must return `{success: bool, name: string}` — a bare bool silently no-ops the bar (`saveWithDialog` checks `result && result.success`).

**Corrected C4 gate: `11 ≡ 11`** (10 preset + the existing `getParameterDefaults`).

### F2 — The grep-diff must scan the module JS too

`app.js` will contain exactly **one** `getNativeFunction(...)` call after Stage 4 (`getParameterDefaults`); the other ten are inside `preset-manager.js`. `ui_frontend_check.js:110-131` reads only `appJs` and hard-asserts `called.size === 1 && registered.size === 1`.

**Required edit:** build the `called` set from `appJs + presetManagerJs` (read from `modules/persistence/preset-manager/js/preset-manager.js`), and change the surface assertion to 11. Keep the `missing` / `dead` set logic unchanged — it is the part that has real value.

### F3 — Three-way closure check breaks on module-tree sources

`ui_frontend_check.js:226` extracts the embedded set with `/Source\/ui\/public\/(\S+)/g`. The new CMake entry is `${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/js/preset-manager.js`, which does **not** match — so `embedded` lacks `/js/preset-manager.js` while `provided` (getResource) has it, and the `notEmbedded` assertion FAILs on correct code.

**Required edit:** add a second extraction for `modules/…/js/([^/\s]+\.js)` mapped to `/js/$1`, mirroring how `getResource()` will serve it (O-Contrabass serves the module file at `/js/preset-manager.js`, `PluginEditor.cpp:711`).

### F5 — Real preset path

`OuariconPresetManager::getPresetsDirectory()` = `~/Library/{pluginName}/Presets/`, split into `Factory/` and `User/`. For `OuariconPresetManager presetManager { parameters, "O-ReverseDelay" }` that is:

```
~/Library/O-ReverseDelay/Presets/Factory/    ← the 8 factory .json + .factory-version sentinel
~/Library/O-ReverseDelay/Presets/User/       ← user saves
```

Hardcode the name **`"O-ReverseDelay"`** (no `OUARICON_DEV_SUFFIX`) so dev and release builds share one preset library — matches every sibling.

### F7 — `.factory-version` sentinel is a dev-loop trap

`initializeFactoryPresets()` early-returns when `Factory/.factory-version` already holds `JucePlugin_VersionString`. O-ReverseDelay's version is frozen at **1.0.0** for all of Stage 4, so **every edit to the factory table after the first run is a no-op**.

**Mitigation (put it in the plan as an explicit step):**
```bash
rm -rf ~/Library/O-ReverseDelay/Presets/Factory   # after ANY factory-table edit
```
The same applies to the harness preset-audit run (F9) — it constructs the processor, so it re-seeds from a clean dir.

### F8 — C2 is already satisfied; do nothing

`applyPresetJson()` (v1.0.5) already runs a full reset-to-defaults pass over every `RangedAudioParameter` before applying, meta-parameters first, then applies the preset's keys meta-first. O-ReverseDelay has no meta parameters, so the two-pass loop is a no-op — but the reset is exactly what C2 asks for. **No plugin-side code satisfies C2; the module does.** Verify by reading, not by writing.

### Integration shape (copy from O-Contrabass, adjust)

**PluginProcessor.h**
```cpp
#include "OuariconPresetManager.h"
...
OuariconPresetManager& getPresetManager() noexcept { return presetManager; }
...
OuariconPresetManager presetManager { parameters, "O-ReverseDelay" };   // after `parameters`
```

**PluginProcessor.cpp** — factory table in the constructor (§3 below), then:
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
`getStateAsXml()` wraps the **same** APVTS XML root with a `currentPreset` attribute, and `setStateFromXml()` accepts plain pre-Stage-4 APVTS XML — so this is backward compatible with sessions saved during Stage 1–3. No `setCustomStateCallbacks` needed (O-ReverseDelay has no state outside the APVTS).

**CMakeLists.txt** — two edits, both additive:
```cmake
target_include_directories(OuariconReverseDelay PRIVATE
    Source
    ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp)   # header-only, no drift

juce_add_binary_data(OuariconReverseDelay_UIResources
    NAMESPACE UIBinaryData
    HEADER_NAME UIBinaryData.h
    SOURCES
        ...
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/js/preset-manager.js)  # C3: SAME target
```
→ `UIBinaryData::preset_manager_js` / `preset_manager_jsSize`. Add a `getResource()` branch for `"/js/preset-manager.js"` with `application/javascript; charset=utf-8`.

### F6 — MSVC-safe dialog lambdas (mandatory deviation from O-Contrabass)

O-Contrabass writes `[safeThis = juce::Component::SafePointer<OContrabassAudioProcessorEditor>(this), complete, makeResult]` **inside** the `withNativeFunction` lambda. On MSVC, `this` in a nested lambda's capture-initialiser resolves to the *enclosing closure*, not the editor → hard C2440 (`critical_msvc_safepointer_init_capture_nested_lambda`; O-Lyrica's first Windows CI run died on exactly this). Apple Clang accepts it, so it is invisible locally.

**Use the hoisted-local form:**
```cpp
.withNativeFunction ("savePresetWithDialog", [this] (const auto&, auto complete)
{
    auto safeThis = juce::Component::SafePointer<ReverseDelayEditor> (this);   // hoisted FIRST
    ...
    fileChooser->launchAsync (flags, [safeThis, complete, makeResult] (const juce::FileChooser& fc)
    {
        if (safeThis == nullptr) return;           // C9: BARE return — complete() is a UAF here
        ...
    });
})
```
Editor members needed: `std::shared_ptr<juce::FileChooser> fileChooser;` and `bool fileDialogOpen = false;` (re-entrancy guard — a second click while the dialog is up must complete `{false, ""}` immediately, not stack a chooser).

`savePresetWithDialog` should honour the directory the user picked (O-Wind WR-12): `result.isAChildOf(pm.getUserPresetsDirectory()) ? pm.savePreset(name) : pm.savePresetToFile(result.withFileExtension("json"))`.

---

## 2 · Preset bar UI (D15) — geometry, styling, TDZ

### Geometry: three CSS numbers + one C++ number

| File | Line | Change |
|------|------|--------|
| `Source/PluginEditor.cpp` | 168 | `setSize(940, 440)` → `setSize(940, 484)` |
| `css/styles.css` | 54 | `html, body { height: 440px }` → `484px` |
| `css/styles.css` | 88 | `.frame { height: 440px }` → `484px` |

The `.frame` is `display:flex; flex-direction:column` with `.groups { flex: 1 }` and fixed-height (215 px) panels that are `align-items:center`. Adding a 44 px band **and** 44 px of frame height leaves the flex slack, the panel heights, and the footer untouched — this is why D15 is the low-regression choice. The `.time-slot` box shifts **y +44** with **w/h unchanged**; the re-verify assertion is *mode-invariance* (identical box in Free and Sync), never the old absolute `y:198`.

Bar markup goes between `</header>` (index.html:18) and `<main class="groups">` (index.html:21).

### Styling: derive from O-ReverseDelay's own vocabulary, not O-Contrabass's chrome

CONTEXT open-question #2 asked how to style the bar in the Naturalist idiom. `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md` has **no preset-bar component** — confirmed. O-Contrabass's `.preset-bar` is a **dark brown header strip that carries the plugin name**; O-ReverseDelay already has a separate paper header, so importing that treatment would give the page two competing title bars.

**Recommendation — borrow O-Contrabass's *structure* (flex row, 26 px nav squares, centred name field, ellipsis overflow) and dress it in the existing local vocabulary:**

- Band: transparent over the paper gradient, hairline `1px solid rgba(139,115,85,0.45)` **below** it (mirrors `.header`'s existing bottom rule) — the bar reads as a second field-guide register line, not a chrome strip.
- Buttons: reuse the **`.segment`** treatment already in `styles.css:273-314` — `var(--btn-default)` fill, `2px solid var(--btn-border)`, 5 px radius, `translateY(-1px)` hover lift. Nav arrows as 26×26 squares, Save/Load/Delete as `padding: 5px 12px` with 10 px uppercase serif.
- Name field: recessed cartouche — `background: var(--bg-paper-light)`, `2px solid var(--brown-border)`, `inset 1px 1px 3px rgba(0,0,0,.18)` (identical to `.division-select`), **italic** serif 13 px, `width: 300px`, `white-space:nowrap; overflow:hidden; text-overflow:ellipsis`.
- Flank the row with the existing `❧` fleuron so the band matches the footer's typographic furniture.

### C7 — TDZ-safe bar init

`app.js` currently ends with a single bottom-of-file `init();` and `ui_frontend_check.js` §2 asserts that `init();` is the **last meaningful line** and that no module-level declaration follows it. So:

- Declare any bar state (`let presetManager = null;`) in the **existing top block** next to `paramDefaults`.
- Add a hoisted `async function initPresetBar() { ... }` among the other function declarations.
- Call it **from inside `init()`**, fire-and-forget like `loadParameterDefaults(Juce)` — do **not** add a second top-level call, and do **not** put the dynamic `import()` at module top level.

```js
async function initPresetBar() {
  try {
    const { PresetManager } = await import("./preset-manager.js");
    presetManager = new PresetManager({
      displayElement: document.getElementById("preset-name"),
      prevButton:     document.getElementById("preset-prev"),
      nextButton:     document.getElementById("preset-next"),
      saveButton:     document.getElementById("preset-save"),
      loadButton:     document.getElementById("preset-load"),
      deleteButton:   document.getElementById("preset-delete"),
      getNativeFunction: Juce.getNativeFunction,
      onConfirmDelete: confirmDeleteInline,     // see below
    });
    await presetManager.initialize();
  } catch (e) {
    console.error("[preset-bar] init failed:", e);   // bar dies alone; knobs unaffected
  }
}
```
The `try/catch` is load-bearing: it contains a bar failure so the ten already-verified controls survive it.

**Import path note:** `preset-manager.js` is served at `/js/preset-manager.js` and `app.js` lives at `/js/app.js`, so the specifier is `"./preset-manager.js"`. `ui_frontend_check.js:209` already harvests `from ["']\./…` in `app.js` into the three-way closure — but that regex only matches **static** `from` imports. A dynamic `await import("./preset-manager.js")` will **not** be harvested, so add `refs.add('/js/preset-manager.js')` explicitly (or extend the regex to `import\(`).

### Delete confirmation — avoid `window.confirm` entirely

`promptDelete()` falls back to `window.confirm()`, which is a silent no-op or throw in some JUCE WebView backends (module IN-05). Human-checklist item 6 requires a working delete round-trip, so supply `onConfirmDelete`:

```js
// Two-click inline confirm — no native dialog, no window.confirm.
// Copy lives in HTML (data-label / data-confirm), so the button text is never
// invented by JS (C6, pattern_js_state_updater_overwrites_html_labels).
function confirmDeleteInline(name, _message) {
  const btn = document.getElementById("preset-delete");
  if (!btn) return false;
  if (btn.dataset.armed === "1") { disarm(btn); return true; }
  btn.dataset.armed = "1";
  btn.textContent = btn.dataset.confirm;      // authored in index.html
  setTimeout(() => disarm(btn), 3000);
  return false;
}
function disarm(btn) { btn.dataset.armed = "0"; btn.textContent = btn.dataset.label; }
```
with `<button id="preset-delete" data-label="Delete" data-confirm="Confirm?">Delete</button>`.

### C6 note — the name field is *meant* to be written

`_updateDisplay()` does `displayElement.textContent = this.currentPreset` (preset-manager.js:363). That is the module's own element and the write is intended. Author the placeholder as `Default` (a fresh instance reports `"Default"`, which is deliberately **not** in the preset list). C6 applies to the *buttons* — hence `data-label` above — and to the untouched FREE/SYNC segments.

### Preset list ordering (affects ◀ ▶ and the human checklist)

`getPresetList()` concatenates Factory then User, then `sort(true)` (case-insensitive). The eight ship in this order:

`Dark Cavern · Guitar Swell · Near-Infinite · Reverse Bloom · Rhythmic Reverse · Slow Wash · Tight Smear · Vocal Halo`

A fresh instance displays **`Default`** (not a list member) until the user navigates; `getNextPreset()` handles the out-of-list case via `lastListIndex`. Do not expect "Reverse Bloom" to be the landing preset — D16 called it "default-adjacent", which is about its *values*, not its position.

---

## 3 · Factory presets (D16 + C1) — authored table

All eight carry **all ten keys explicitly**. `applyPresetJson` resets omitted keys to defaults, so omission is safe — but explicit keys make the harness audit (§5) a direct value comparison instead of a defaults lookup.

Skewed params (`delayTime`, `grainSize`, `lowCut`, `highCut`) are the C1 hazard; the loop below converts every value, skewed or not, and handles the two Choice params uniformly (`AudioParameterChoice`'s range is `0 … n-1` step 1, so `convertTo0to1(6.0f)` on the 13-entry `noteDivision` = 0.5).

| # | Name | sync | div | delay ms | grain ms | dens % | fb % | loCut Hz | hiCut Hz | width % | mix % |
|---|------|------|-----|---------:|---------:|-------:|-----:|---------:|---------:|--------:|------:|
| 1 | Reverse Bloom | Free (0) | 6 | 500 | 200 | 60 | 40 | 100 | 8000 | 60 | 40 |
| 2 | Guitar Swell | Free (0) | 6 | 700 | 300 | 55 | 45 | 120 | 6500 | 55 | 55 |
| 3 | Vocal Halo | Free (0) | 6 | 380 | 180 | 70 | 30 | 300 | 7000 | 70 | 25 |
| 4 | Slow Wash | Free (0) | 6 | 1400 | 450 | 30 | 65 | 80 | 5000 | 85 | 50 |
| 5 | Tight Smear | Free (0) | 6 | 180 | 70 | 90 | 35 | 150 | 11000 | 35 | 45 |
| 6 | Dark Cavern | Free (0) | 6 | 850 | 320 | 65 | 70 | 220 | 1800 | 75 | 55 |
| 7 | Near-Infinite | Free (0) | 6 | 900 | 350 | 70 | **100** | 180 | 2500 | 80 | 50 |
| 8 | Rhythmic Reverse | **Sync (1)** | **4** (1/8D) | 500 | 120 | 80 | 50 | 140 | 9000 | 50 | 45 |

Notes:
- Seven presets are **Free** so they audition identically with or without a host tempo; #8 is the only Sync preset (FUNC-02 / COMPAT-02 coverage per D16) and still carries a sane `delayTime` for the COMPAT-02 no-BPM fallback path.
- **Near-Infinite** at `feedback=100` doubles as a preset-driven DSP-03 check (§5).
- No name contains `/` (C8) — #8 is "Rhythmic Reverse", never "Reverse 1/8".
- `grainSize=70` in Tight Smear is deliberately near the 50 ms floor; `delayTime=1400` in Slow Wash near the 2000 ms ceiling. Both exercise skew extremes in the audit.

Seeding code (constructor, after `parameters` is live):
```cpp
std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = { /* table above */ };

// C1: engineering units → normalised through each param's own NormalisableRange.
// Handles skew, int, and choice uniformly. A hand-written fraction on any of the
// four skewed params recalls 10–30× wrong (pattern_factory_preset_normalized_ignores_skew).
for (auto& preset : factoryPresets)
    for (auto& [id, value] : preset.parameters)
        if (auto* p = parameters.getParameter (id))
            value = p->convertTo0to1 (value);

presetManager.initializeFactoryPresets (factoryPresets);
```

**Reference midpoints for spot-checking the conversion** (from Stage-3 verify): normalised 0.5 ⇒ delayTime 316 ms, grainSize 158 ms, lowCut 200 Hz, highCut 3162 Hz.

---

## 4 · Tooltips (D13 + C5)

### Scope

Ten anchors: the 8 `.knob` elements, `#combo-noteDivision`, and the `#syncSegments` group. **No help toggle, no preference persistence** — O-MultiBandCompressor's version adds a `setTooltipsEnabled` native fn, which would push the bridge to 12 and add state D13 explicitly excluded.

### Mechanism — lift `showTooltip()` from O-MultiBandCompressor verbatim

`plugins/O-MultiBandCompressor/Source/ui/public/js/app.js` (`showTooltip`, ~line 940) already carries the **v1.4.1 measure-then-pin fix** that C5 demands:

```js
tooltipEl.style.width = '';        // release
tooltipEl.style.left  = '0px';     // measure from the LEFT edge
tooltipEl.style.top   = '0px';
const width = tooltipEl.getBoundingClientRect().width;
tooltipEl.style.width = `${width}px`;   // PIN before placing
const height = tooltipEl.getBoundingClientRect().height;   // stable only once width is definite
```
Measuring at the previous offset under-reports the width, and applying a near-edge `left` afterwards re-wraps a 230 px tip into a ~70 px ribbon. In O-ReverseDelay the exposed control is **`mix`** (right-most, OUTPUT panel) — exactly the C5 prediction.

Also copy: clamp `left` into `[MARGIN, innerWidth - width - MARGIN]`, prefer `above` and flip `below` only when `top < MARGIN`, and drive the arrow with `--arrow-x` so it still points at the control after clamping.

CSS (`.tooltip`, `.tooltip-title`, `.tooltip-body`, `.tooltip::after`) transplants nearly unchanged — O-MBC's tooltip palette (`#f5f0e6 → #e8dfcd`, border `#5C4033`, title `#556B2F`, body `#3C2F2F`) is already the Naturalist palette. Swap the literals for O-ReverseDelay's `var(--bg-paper-light)`, `var(--brown-frame)`, `var(--green-mid)`, `var(--brown-text)`.

`position: fixed; z-index: 1000` also puts the tip above the `.botanical-overlay` (z-index 1) and outside `.frame`'s inset shadow.

Suppress on `pointerdown` and re-enable on `mouseup` (O-MBC's `tooltipSuppressed`) — otherwise a tip hangs over the knob during a drag.

### Copy — one line each, field-guide register

Matches the existing footer voice (*"Drag vertically · wheel or arrows to trim · double-click to reset"*).

| Anchor | `data-tip-title` | `data-tip` |
|--------|------------------|------------|
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

Author as `data-tip` / `data-tip-title` **attributes in index.html** (not a JS table) — same reasoning as C6: the copy is content, and content lives in HTML.

---

## 5 · Preset audit placement (CONTEXT open question #4) — **answer: extend the existing harness**

### F9 — The harness gets the preset-manager include for free

`tests/render-harness/CMakeLists.txt:36-38` already does:
```cmake
target_include_directories(O-ReverseDelay-render-test PRIVATE
    $<TARGET_PROPERTY:OuariconReverseDelay,INCLUDE_DIRECTORIES>)
```
So the moment the plugin target adds `modules/persistence/preset-manager/cpp`, the harness resolves `OuariconPresetManager.h` with **no harness CMake edit**. `OuariconPresetManager` is header-only and touches only `juce_core` / `juce_data_structures` / `juce_audio_processors`, all already linked, and it needs no WebView — it compiles cleanly under `JUCE_WEB_BROWSER=0`.

### Recommendation: audit through the real manager, not a re-typed table

A separate binary, or a harness table that re-states the eight presets, would validate the *typing* and not the *product*. Instead, the harness's `ReverseDelayProcessor proc;` already seeds the factory presets in its constructor, so the audit can be genuinely end-to-end:

```cpp
// --- Probe N: factory-preset audit (D16 / C1) --------------------------------
// Loads each preset through the SHIPPING OuariconPresetManager — this is the
// only check that proves the engineering-unit → convertTo0to1 → convertFrom0to1
// round-trip survives skew for delayTime / grainSize / lowCut / highCut.
static const struct { const char* name; float delay, grain, dens, fb, lo, hi, wid, mix;
                      int sync, div; } kFactoryExpect[] = { /* the §3 table */ };

for (const auto& e : kFactoryExpect)
{
    const bool loaded = proc.getPresetManager().loadPreset (e.name);
    proc.prepareToPlay (fs, block);

    // (a) skew round-trip — the C1 assertion
    const bool values = loaded
        && std::abs (paramValue (apvts, "delayTime") - e.delay) < 0.5f
        && std::abs (paramValue (apvts, "grainSize") - e.grain) < 0.5f
        && std::abs (paramValue (apvts, "lowCut")    - e.lo)    < 0.5f
        && std::abs (paramValue (apvts, "highCut")   - e.hi)    < 2.0f
        && /* linear params + the two choice indices */ ;

    // (b) render 10 s (2 s excitation + 8 s tail) — finite, bounded, non-silent
    auto y = renderEffect (proc, 10.0, fs, block, excite2s);
    const bool audio = allFinite (y.L) && allFinite (y.R)
                    && jmax (peakAbs (y.L), peakAbs (y.R)) < 1.0
                    && rms (y.L, (int)(6.0*fs), (int)(2.0*fs)) > 1.0e-7;

    check (e.name, values && audio, ...);
}
```

Tolerances: the APVTS `interval` is `0.01` on the skewed params and `0.1` on the linear ones, so `< 0.5` is generous but catches the 10–30× class of C1 failure by orders of magnitude. `highCut` gets `< 2.0` (11 kHz at 0.01 interval through a skewed round-trip).

**Bookkeeping:** this takes the harness from **33 → 41 checks** (8 presets, one `check()` each). Every gate in CONTEXT that says "33/33" must be restated as *33/33 at entry, 41/41 at exit*. If the makeup constant ships (§6), probe G's `check()` count is unchanged but a new decay-rate probe would make it 42.

**Near-Infinite is the DSP-03 preset check** D16 promised: `feedback=100` for a 10 s render with damping at 180 Hz / 2.5 kHz. Consider giving it a longer render (30 s) than the other seven.

**Ordering caveat:** the audit mutates APVTS state. Place probe N **last**, after probe M, and note that it leaves the plugin on the final preset's values — nothing runs after it.

---

## 6 · Makeup constant (D11 / CONTEXT open question #1) — derivation, not a guess

**Do not implement before the audition.** This section exists so that *if* the audition asks for a longer wash, the value is derived rather than dialled.

### The measured loss

Stage-2 SUMMARY quantifies **−7.3 dB per generation at `feedback=100`, pre-damping**, decomposed as:
- **−4.3 dB** — Hann² duty cycle from the RMS-flat overlap compensation
- **−3.0 dB** — the pan → mono-sum round trip (grain reads `0.5(L+R)`, equal-power pan re-places it)

Linear loop gain ≈ **0.43×** before the damping filters take their cut.

### Where it goes

One constant at the feedback tap, `PluginProcessor.cpp:361`, multiplying the smoothed feedback gain **before** the filters and **before** `tanh`:

```cpp
// Compensates the topology's inherent −7.3 dB/generation (Hann² duty + pan→monoSum).
// Pre-tanh by construction: the soft-clip still bounds the loop at any setting.
static constexpr float kFeedbackMakeup = 2.0f;   // +6.0 dB
...
const float g = feedbackSmoothed.getNextValue() * kFeedbackMakeup;
```

Pre-`tanh` placement matters: `tanh` remains the bound, so DSP-03 cannot be broken by the constant — the constant changes *how long* the tail lasts, not *whether* it is bounded.

### Candidate values

| k | dB | Residual loop gain (pre-damping) | Character |
|---|-----|----------------------------------|-----------|
| 1.50× | +3.5 | 0.65 (−3.8 dB/gen) | Modestly longer; conservative |
| **2.00×** | **+6.0** | **0.86 (−1.3 dB/gen)** | **Recommended** — roughly doubles wash length, still clearly decaying before damping |
| 2.32× | +7.3 | 1.00 (0 dB/gen) | Full compensation — damping alone decides decay. Not recommended: at `highCut=20 kHz` / `lowCut=20 Hz` the loop sits at unity and the character becomes "sustain until tanh" |

**Recommend 2.0× if the audition asks for more wash.** Full compensation (2.32×) hands all decay authority to the two damping filters, which is a materially different plugin from the one Stage 2 verified.

### Re-gating if it ships

- **Probe G** (60 s @ fb=100) — re-run; assert unchanged (`peak < 1.0`, `tailPeak < 1.0`, zero NaN/Inf), plus a **new** assertion that `washRms[5..10 s]` has *increased* versus the pre-change baseline (proves the constant did something).
- **Probe M** (all-parameter sweep at defaults, `feedback=40`) — its two-tier click detector runs with the loop live; re-run and confirm the `kStepFactor 8.0` latched-param tier still holds.
- **Probes E and F** are unaffected in principle (E runs at `feedback=0`, where `g * k == 0`; F measures spectral centroid, not level) — but re-run the **full 33** regardless.
- **Re-audition** afterwards (CONTEXT human-checklist item 1 already calls for this).

If the audition passes, record the decision as **declined** in SUMMARY.md — CONTEXT success-criterion #1 requires the question be *closed*, not merely unaddressed.

---

## 7 · Browser-stub gate (F4) — two blockers before the bar is testable

The stub render is CONTEXT's automatable gate for the bar (*"exactly 940×484, zero overflow, zero JS console errors"*). Two things break it today.

### 7a — `serve-stub.sh` never copies the module JS

`tests/ui-stub/serve-stub.sh:16-17` copies `Source/ui/public/.` and swaps in the stub. `preset-manager.js` lives in the **module tree** (embedded straight from `modules/…` by CMake, never copied into `Source/ui/public`), so `await import("./preset-manager.js")` 404s under the stub server.

**Fix — one line after the existing `cp`:**
```bash
cp "$HERE/../../../../modules/persistence/preset-manager/js/preset-manager.js" "$ROOT/js/preset-manager.js"
```
This also keeps the script's promise intact: what is served is byte-identical to what the resource provider serves.

### 7b — `juce-stub.js` needs a `__JUCE__` shim and the 10 preset fns

Two independent failures:

1. `_waitForNative()` polls `window.__JUCE__ && window.__JUCE__.backend` **100 × 50 ms = 5 s**, then resolves with a `console.error`. That alone fails the zero-console-errors criterion and adds 5 s to every stub run.
2. `getNativeFunction()` in the stub (`juce-stub.js:108-115`) **rejects every name except `getParameterDefaults`** — by design, as a bridge-gap detector. All ten preset calls would reject.

**Fix:** add to `juce-stub.js`

```js
// The module polls window.__JUCE__.backend before doing anything; without this
// shim _waitForNative() burns 5 s and console.errors (preset-manager.js:129).
if (typeof window !== "undefined") window.__JUCE__ = { backend: { } };

// In-memory preset store — enough to exercise the bar's full round-trip in the
// browser: list / current / next / prev / save / load / delete / isFactory.
const FACTORY = ["Dark Cavern", "Guitar Swell", "Near-Infinite", "Reverse Bloom",
                 "Rhythmic Reverse", "Slow Wash", "Tight Smear", "Vocal Halo"];
const userPresets = new Set();
let currentPreset = "Default";
const PRESET_FNS = { savePreset: …, savePresetWithDialog: …, loadPreset: …,
                     loadPresetFromFile: …, getPresetList: …, getCurrentPreset: …,
                     selectNextPreset: …, selectPreviousPreset: …, deletePreset: …,
                     isFactoryPreset: … };
```
and gate `getNativeFunction` on `name === "getParameterDefaults" || name in PRESET_FNS`, rejecting everything else — the detector's value is preserved, its whitelist just grows from 1 to 11.

Both dialog fns must resolve `{success, name}` in the stub too, or `saveWithDialog()` silently reports failure.

---

## 8 · Consolidated gate deltas

CONTEXT's Validation Baseline needs these restatements before the plan is written:

| CONTEXT says | Corrected |
|--------------|-----------|
| Native-fn grep-diff `10 ≡ 10` | **`11 ≡ 11`**, scanning `app.js` **+** `modules/…/preset-manager.js` (F1, F2) |
| Harness "33 probes" at entry and exit | **33/33 at entry**, **41/41 at exit** (+8 preset checks); +1 more if a decay probe ships with the makeup constant (F9, §6) |
| `ui_frontend_check.js` — 45 assertions, "extend for preset bar + tooltips" | Also **repair** §3 (surface count + source set) and §9 (module-tree embedded regex + dynamic-import ref) or it FAILs on correct code (F2, F3) |
| Stub "must gain the 9 preset native fns" | **10** fns **plus** a `window.__JUCE__.backend` shim **plus** a `serve-stub.sh` copy line (F1, F4) |
| Presets in `~/Library/Application Support/O-ReverseDelay/Presets/` | `~/Library/O-ReverseDelay/Presets/{Factory,User}/` (F5) |
| — | **New:** hoisted-local `SafePointer` in both dialog fns, or Windows CI fails to compile (F6) |
| — | **New:** `rm -rf ~/Library/O-ReverseDelay/Presets/Factory` after every factory-table edit (F7) |

Unchanged and still correct: pluginval-10 ×3 on VST3 **and** AU, `auval -v aufx ORvD OuDv`, `build-and-install.sh O-ReverseDelay` dual-variant sweep, CHANGELOG at v1.0.0, Windows deferred to CI, `/publish` separate.

### CHANGELOG

`plugins/O-ReverseDelay/CHANGELOG.md` **does not exist** — Stage 4 authors it from scratch. Follow O-Contrabass's shape: a `# Changelog — O-ReverseDelay` heading, a note that v1.0.0 is the first shipped version, then `## [1.0.0] — <date> — first release` with an `### Added` block per stage and a validation line quoting the actual harness / auval / pluginval results.

---

## 9 · Suggested phase split for the plan

Stage 4 ships real code in three loosely-coupled areas plus a validation tail. A 3-phase split keeps each one independently gate-able:

- **Phase 4.0 — Entry gate (no code).** D7 Standalone audition; harness re-run 33/33 at the current tree; makeup-constant decision recorded either way. If it ships: edit, re-run 33/33 + probe-G delta, re-audition. *Blocking — CONTEXT requires this before any Stage-4 code.*
- **Phase 4.1 — Preset system.** CMake include + binary-data entry; `presetManager` member + factory table + state routing; 10 native fns (hoisted SafePointer); resource-provider branch; `serve-stub.sh` + `juce-stub.js` extensions; harness probe N. Gate: build clean, harness 41/41, `rm -rf` + re-seed verified.
- **Phase 4.2 — Bar + tooltips (frontend).** 940×484 in three CSS spots + `PluginEditor.cpp:168`; bar markup + Naturalist CSS; `initPresetBar()` inside `init()`; inline delete confirm; 10 tooltip anchors + measure-then-pin placement. Gate: stub render at exactly 940×484, zero overflow, **zero console errors**, `mix` tooltip not shrink-wrapped; `ui_frontend_check.js` repaired + extended, exit 0; grep-diff 11 ≡ 11.
- **Phase 4.3 — Validation + release prep.** pluginval-10 ×3 VST3 + ×3 AU; `auval`; `build-and-install.sh` with dual-variant sweep + AU cache clear; CHANGELOG at v1.0.0; hand the 7-item human checklist to the user.

---

## 10 · Open items carried to plan

1. **Makeup constant** — value only decidable after the D7 audition. §6 gives the derivation, candidates, placement, and re-gate list; the plan should carry it as a *conditional* task with `k = 2.0f` pencilled in.
2. **`highCut` audit tolerance** — §5 proposes `< 2.0 Hz` at 11 kHz; confirm against an actual round-trip during execute rather than assuming.
3. **Near-Infinite render length** — 10 s like the rest, or 30 s for a stronger DSP-03 statement? Recommend 30 s; costs ~20 s of harness runtime.
4. **Delete button in the bar** — D15's sketch shows four controls (`◀ name ▶ ⤓ ⤑`) with no delete, but human-checklist item 6 requires a delete round-trip. §2 recommends adding a fifth button with the inline two-click confirm. Flag for the user if the bar is meant to stay at four.

---

## Next Phase

Ready for: **plan** phase
