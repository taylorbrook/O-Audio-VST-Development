# Stage 4 (Validation / Polish) — RESEARCH

**Researched:** 2026-06-25
**Domain:** JUCE 8.0.9 factory-preset architecture, APVTS host-notifying writes, drum-machine pattern design, cross-format validation sweep
**Confidence:** HIGH (all JUCE APIs verified against local source `/Users/taylorbrook/JUCE`; all code paths read from the actual Stage-1→3 build)
**Mode:** express (non-interactive) — derived from CONTEXT.md + the live source tree.

---

## Summary

Stage 4 is **content + validation only** — no new DSP or UI mechanism. The work is four things: (1) a lightweight C++ factory-preset table that drives the six existing tour buttons, (2) six concept-isolating velocity patterns + their timing-feel param settings, (3) a one-time playability sanity pass on defaults (recommend: change nothing blindly — the kit is already tuned and verified in Stage 2), and (4) a full VST3+AU+Standalone validation sweep ending in a hands-on QUAL-02 audible-vs-visible check and a CHANGELOG v1.0.0.

The preset mechanism is fully specified by the existing code: `setStep`/`clearGrid` are thread-safe atomic writes, `getStateInformation` rebuilds the PATTERN ValueTree from those atomics at save time (so **presets persist automatically — no extra ValueTree work**), the knob/combo attachments two-way bind to the host, and `setValueNotifyingHost` pushes a param change all the way back to the JS knob visuals for free. The single new surface is one native function (`applyPreset`) wired into the already-stubbed `initPresetTour()`.

**Primary recommendation:** Add `BeatPresets.h` (a `constexpr` table of 6 presets) + `OSimpleBeatmakerAudioProcessor::applyConceptPreset(int)` + one `applyPreset` native fn. Apply timing-feel params via `param->setValueNotifyingHost(param->convertTo0to1(realValue))`, write the grid via `clearGrid()` then `setStep()`, and have JS call `refreshGridFromBackend()` after. Make **zero** blind default-param changes for FUNC-08 — control playability through preset velocities and flag a single audible check.

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
1. **Preset mechanism = lightweight C++ factory table, NOT OuariconPresetManager.** FUNC-05 is read-only concept lesson patterns, not user save/load. `BeatPresets.h` data table + processor `applyConceptPreset(int)` + one new `applyPreset` native function. `getNumPrograms()` stays 1.
2. **Apply path is message-thread + host-notifying.** `applyConceptPreset` sets APVTS params via `setValueNotifyingHost` (knobs + host automation update for free) and writes the grid via existing thread-safe `setStep`/`clearGrid` atomics, then JS refreshes the grid. No audio-thread work, no new lock.
3. **Playability (FUNC-08) = tuning only, no new params.** The 42-param contract is frozen. Any default change is a value change in `createParameterLayout`, regression-checked.
4. **Validation sweep is the gate.** Build VST3+AU+Standalone clean → cache-clear + dual-variant sweep install → `auval` → `pluginval --strictness-level 10` → render-harness re-run (6 probes green) → screenshot UI with a preset loaded → hands-on QUAL-02 spot check.
5. **CHANGELOG v1.0.0** authored as the first release entry (covers the whole staged build).

### Constraints / Non-negotiables
- **No regression** of the Stage-3 verified build. Grid/playhead/lane/MIDI/all-42-bindings must still pass.
- **42-parameter APVTS contract frozen** — presets set existing params, never add params. Grid stays custom `std::atomic<uint8_t>[6×32]` + PATTERN ValueTree.
- **PERF-01 real-time safety unchanged** — preset apply is message-thread only.
- **Cross-platform** — any new native fn keeps JS `getNativeFunction` ⇿ C++ `withNativeFunction` parity exact.
- **BinaryData namespace** — still a single `O-simpleBeatmaker_UIResources` target; no second `juce_add_binary_data`. Preset data is C++ source, not a binary-data blob.

### Deferred Ideas (OUT OF SCOPE)
- User-savable presets, disk JSON, preset browser UI (v1.1+).
- Song mode / pattern chaining, per-step probability, new voices, FX.
- Any new DSP mechanism or UI panel.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| FUNC-05 | Concept-isolating factory pattern presets (straight / backbeat+accents / ghost notes / swing / humanized / quantize demo) | §1 preset architecture + §2 six concrete velocity grids |
| FUNC-08 | Playable/musical enough to double as a simple 808/909 instrument | §3 playability tuning (recommend no blind default changes; preset-velocity discipline + one audible check) |
| (sweep) | Final validation: pluginval VST3+AU, auval, no regressions | §4 validation sweep commands |
| QUAL-02 | Hands-on confirm timing lane / playhead / velocity / MIDI readout match what is heard | §4 QUAL-02 audit procedure (truth-by-construction already wired; this is the hands-on confirmation) |
</phase_requirements>

---

## 1. Factory-Preset Architecture (FUNC-05)

The CONTEXT lightweight approach is **confirmed correct and fully supported by the existing code**. Three additions, no new param, no new binary-data target.

### 1a. Data structure — `Source/BeatPresets.h`

A new header (C++ source, NOT a binary-data blob — satisfies the BinaryData constraint). `constexpr` table, included by `PluginProcessor.cpp`.

```cpp
// Source/BeatPresets.h
#pragma once
#include <array>
#include <cstdint>
#include "BeatmakerIDs.h"   // kNumVoices, Voice enum order

namespace OSimpleBeatmaker
{
    // One concept-isolating lesson preset. All grids are 16 steps (one bar);
    // columns 16..31 are left empty (cleared) — patternLength is 16 for all six.
    struct BeatPreset
    {
        const char* name;             // matches the data-preset button label
        int   patternLengthChoice;    // APVTS choice index: 0=8, 1=16, 2=32  (all use 1)
        float swing01;                // stored 0..1  (display ×75 → %)
        float humanize01;             // stored 0..1  (display ×100 → %)
        float quantize01;             // stored 0..1  (display ×100 → %)
        float tempoBpm;               // 40..240 (free-run only; ignored while host plays)
        std::array<std::array<uint8_t, 16>, (size_t) kNumVoices> grid;  // row=voice, 0=off,1..127=vel
    };
}
```

**Row order is the Voice enum order** (0 Kick, 1 Snare, 2 Clap, 3 ClosedHat, 4 OpenHat, 5 Tom) — identical to `kVoicePrefix`, `kGmNotes`, and the JS `VOICES[]` array. `[VERIFIED: BeatmakerIDs.h:25-28, app.js:18-25]`

**Decision: no per-voice param overrides in the table.** The six presets isolate *timing-feel + velocity* concepts; the verified Stage-2 voice defaults (tune 0, decay 0.5, tone 0.5, level 0 dB) already sound good and are the FUNC-08 starting point. Adding per-voice columns would (a) bloat the table for no pedagogical gain and (b) risk a preset overwriting a default the user just dialed in. If a future preset needs a voice tweak, the struct can grow an optional `std::array<...> tune/decay/...` — but v1.0 keeps it to the 5 timing-feel params + grid. `[ASSUMED — pedagogical scope call; flag if a preset audibly needs a voice tweak]`

### 1b. Processor method — exact apply sequence (message thread)

Add to `PluginProcessor.h` (public, near the step-grid API) and `.cpp`:

```cpp
// PluginProcessor.cpp
#include "BeatPresets.h"

void OSimpleBeatmakerAudioProcessor::applyConceptPreset (int index) noexcept_FALSE
{
    if (index < 0 || index >= (int) OSimpleBeatmaker::kBeatPresets.size())
        return;

    const auto& p = OSimpleBeatmaker::kBeatPresets[(size_t) index];
    using namespace OSimpleBeatmaker::ParamIDs;

    // --- 1. Timing-feel params, host-notifying. setValueNotifyingHost takes a
    //        NORMALISED 0..1 value, so convert each real value via the param's
    //        own NormalisableRange (convertTo0to1). This updates host automation
    //        AND the two-way-bound JS knobs/combo for free. Message thread only.
    auto setReal = [this] (const char* id, float real)
    {
        if (auto* prm = parameters.getParameter (id))   // RangedAudioParameter*
            prm->setValueNotifyingHost (prm->convertTo0to1 (real));
    };

    setReal (swing,            p.swing01);      // unitRange 0..1 → convertTo0to1 is ~identity
    setReal (humanize,         p.humanize01);
    setReal (quantizeStrength, p.quantize01);
    setReal (tempo,            p.tempoBpm);     // 40..240 → convertTo0to1((bpm-40)/200)

    // Choice param: convertTo0to1(choiceIndex) over range {0, numChoices-1, 1}.
    // For {8,16,32}, index 1 ("16") → 1/2 = 0.5 → setValueNotifyingHost(0.5).
    if (auto* prm = parameters.getParameter (patternLength))
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) p.patternLengthChoice));

    // --- 2. Grid via the existing thread-safe atomic writers. clearGrid() zeros
    //        all 6×32 cells (so columns 16..31 are silent); then stamp the 16-col
    //        preset. setStep clamps 0..127 and stores relaxed-atomic — lock-free.
    clearGrid();
    for (int v = 0; v < OSimpleBeatmaker::kNumVoices; ++v)
        for (int s = 0; s < 16; ++s)
            if (p.grid[(size_t) v][(size_t) s] > 0)
                setStep (v, s, p.grid[(size_t) v][(size_t) s]);
}
```

> Use a plain `void applyConceptPreset (int index)` signature (drop the `noexcept_FALSE` placeholder above — that is just a note that `setValueNotifyingHost` is not `noexcept`). It runs on the message thread from the native-fn lambda, so it may take the (uncontended) parameter lock that `setValueNotifyingHost` uses internally; that is fine off the audio thread.

**Verified API facts:**
- `juce::AudioProcessorParameter::setValueNotifyingHost (float newValue)` — argument is **normalised 0–1**. `[VERIFIED: juce_AudioProcessorParameter.h:141]`
- `juce::RangedAudioParameter::convertTo0to1 (float v) const noexcept` — converts a **real** value to 0–1 using the parameter's `NormalisableRange`. `[VERIFIED: juce_RangedAudioParameter.h:123]`
- `AudioProcessorValueTreeState::getParameter(id)` returns `RangedAudioParameter*` — so `convertTo0to1` and `setValueNotifyingHost` are both directly available. `[VERIFIED: juce_AudioProcessorValueTreeState.h:338]`
- `NormalisableRange<float>::convertTo0to1` underlies the parameter method. `[VERIFIED: juce_NormalisableRange.h:136]`

**Persistence — no extra ValueTree rebuild needed.** `getStateInformation` calls `buildPatternTree()`, which reads the current grid atomics at save time and base64-encodes them into a fresh PATTERN child (after removing any stale one). So once `applyConceptPreset` has written the atomics via `setStep`, a host save captures the preset pattern automatically. Param changes persist through the normal APVTS `copyState()`. **Nothing to add.** `[VERIFIED: PluginProcessor.cpp:438-498 buildPatternTree/getStateInformation]`

This mirrors the O-simpleFM precedent (`FactoryPresets.cpp` converts raw→normalised via `convertTo0to1` then applies with `setValueNotifyingHost`), but lighter: no disk JSON, no preset-manager module. `[CITED: O-simpleFM/Source/FactoryPresets.cpp:27-45]`

### 1c. Native function — `applyPreset` (PluginEditor.cpp)

Add inside the existing `options = options.withNativeFunction(...)` chain (alongside `setStep`/`getGrid`/`clearGrid`/`getSampleRate`):

```cpp
    .withNativeFunction ("applyPreset", [this] (const juce::Array<juce::var>& args, auto complete) {
        if (args.size() >= 1)
            processorRef.applyConceptPreset ((int) args[0]);   // index marshalled as juce::var int
        complete (juce::var());
    })
```

`args[0]` arrives as a `juce::var` holding the JS number; `(int) args[0]` extracts it (same marshalling as the existing `setStep` which reads `(int) args[0..2]`). `[VERIFIED: PluginEditor.cpp:101-104]`

### 1d. JS wiring — drive the six tour buttons

`initPresetTour()` is already the hook and already iterates `.tour-btn` in DOM order. The six buttons in `index.html` are in **exact preset-index order** (0 Straight … 5 Quantize Demo). `[VERIFIED: index.html:110-115]`

```js
// near the other native-fn lookups in boot()
let applyPresetFn = null;
try { applyPresetFn = Juce.getNativeFunction("applyPreset"); }
catch (e) { applyPresetFn = null; console.error("applyPreset native fn unavailable", e); }

// initPresetTour(): use the forEach index as the preset index (DOM order == table order)
function initPresetTour() {
  const caption = document.getElementById("tourCaption");
  document.querySelectorAll(".tour-btn").forEach((btn, index) => {
    btn.addEventListener("click", async () => {
      document.querySelectorAll(".tour-btn").forEach((b) => b.classList.remove("armed"));
      btn.classList.add("armed");
      if (applyPresetFn) {
        try { await applyPresetFn(index); } catch (e) { console.error("applyPreset failed", e); }
        await refreshGridFromBackend();   // repaint grid velocities from C++ atomics
      }
      if (caption) caption.textContent = `“${btn.getAttribute("data-preset")}” loaded — tweak a knob to hear the concept.`;
    });
  });
}
```

**Why JS needs only `refreshGridFromBackend()` and nothing else:**
- **Knobs auto-update.** `setValueNotifyingHost` → relay → JS `sliderState.valueChangedEvent` → `updateKnobVisual(id)`. The JS never re-reads each knob. `[VERIFIED: app.js:132-134 bindKnob listeners]`
- **Pattern-length combo auto-updates.** `setValueNotifyingHost` on `patternLength` → combo `valueChangedEvent` → `refresh()` → sets `patternLen` + calls `renderGridColumns()` (rebuilds cells for the new length). `[VERIFIED: app.js:200-202]`
- The grid **velocities** are the only thing not carried by an attachment, so `refreshGridFromBackend()` (already present, reads `getGrid()` and repaints) closes the loop. The `raf` poll (~4×/s, app.js:478-479) is a backstop if the explicit call races the combo's async re-render. `[VERIFIED: app.js:304-318, 472-481]`

### 1e. Native-fn parity discipline (suite critical pattern)

An unregistered native fn passes build + auval + render-harness but the control is silently dead (`pattern_webview_native_fn_bridge_gap`). Before building, grep-diff the two sides:

```bash
cd plugins/O-simpleBeatmaker/Source
grep -o 'withNativeFunction ("[a-zA-Z]*"' PluginEditor.cpp | sort -u
grep -o 'getNativeFunction("[a-zA-Z]*")'   ui/public/js/app.js | sort -u
# Expect both sets: setStep getGrid clearGrid getSampleRate applyPreset
```

---

## 2. The Six Preset Patterns — concrete velocity grids

All presets: **16 steps**, `patternLengthChoice = 1`. Velocity legend matches the JS tier thresholds (`velTier`: ≤55 ghost, 56–112 normal, >112 accent) so the cell display reads cleanly: **ghost = 40, normal = 100, accent = 127** (intermediate 50/80/90/110/120 used where musical). `[VERIFIED: app.js:30-37]`

Step index is 0-based (column 1 = index 0). Beats fall on indices **0, 4, 8, 12**. Off-beat 16ths (what swing delays) are the **odd** indices. `.` = off (0).

### Preset 0 — Straight  ·  swing 0 · humanize 0 · quantize 100% · tempo 120
*Baseline / no-feel. 4-on-floor kick, 8th hats, snare on 2 & 4 — every hit the same velocity. The dead-flat reference.*

| Voice \ step | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Kick      |100| . | . | . |100| . | . | . |100| . | . | . |100| . | . | . |
| Snare     | . | . | . | . |100| . | . | . | . | . | . | . |100| . | . | . |
| Clap      | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |
| ClosedHat |100| . |100| . |100| . |100| . |100| . |100| . |100| . |100| . |
| OpenHat   | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |
| Tom       | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |

### Preset 1 — Backbeat + Accents  ·  swing 0 · humanize 0 · quantize 100% · tempo 100
*Same skeleton as Straight; velocity ALONE makes the groove. Downbeat kick + backbeat snare accented; off-beat hats quiet.*

| Voice \ step | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Kick      |127| . | . | . | 90| . | . | . |100| . | . | . | 90| . | . | . |
| Snare     | . | . | . | . |127| . | . | . | . | . | . | . |127| . | . | . |
| Clap      | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |
| ClosedHat |100| . | 50| . |100| . | 50| . |100| . | 50| . |100| . | 50| . |
| OpenHat   | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |
| Tom       | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |

### Preset 2 — Ghost Notes  ·  swing 0 · humanize 0 · quantize 100% · tempo 90
*Backbeat with quiet ghost snares tucked between 2 & 4 — the pattern breathes. Dynamics are the only variable.*

| Voice \ step | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Kick      |110| . | . | . | . | . | . | . |100| . | . | . | . | . | . | . |
| Snare     | . | . | 40| . |127| . | 40| . | . | . | 40| . |127| . | 40| . |
| Clap      | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |
| ClosedHat |100| . |100| . |100| . |100| . |100| . |100| . |100| . |100| . |
| OpenHat   | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |
| Tom       | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |

### Preset 3 — Triplet Swing  ·  **swing 60% (swing01 = 0.80)** · humanize 0 · quantize 100% · tempo 95
*Off-beat 16ths slide late into a shuffle, cleanly, with no random scatter. 16th hats make the swing audible/visible (odd-index hats are the ones that move).*

| Voice \ step | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Kick      |100| . | . | . | . | . | . | . |100| . | . | . | . | . | . | . |
| Snare     | . | . | . | . |110| . | . | . | . | . | . | . |110| . | . | . |
| Clap      | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |
| ClosedHat |100| 80|100| 80|100| 80|100| 80|100| 80|100| 80|100| 80|100| 80|
| OpenHat   | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |
| Tom       | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |

> swing display = swing01 × 75. **0.80 → 60%** (inside CONTEXT's 58–66% window). Engine ratio `s = 0.5 + swing01/3 = 0.767`. To land nearer 66%, use swing01 = 0.88.

### Preset 4 — Humanized  ·  swing 0 · **humanize 70% (0.70)** · **quantize 25% (0.25)** · tempo 110
*A tight pattern loosened — every hit scatters slightly off the grid. "Loosely played."*

| Voice \ step | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Kick      |100| . | . | . | . | . | . | . |100| . | . | . | . | . | . | . |
| Snare     | . | . | . | . |110| . | . | . | . | . | . | . |110| . | . | . |
| Clap      | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |
| ClosedHat |100| . |100| . |100| . |100| . |100| . |100| . |100| . |100| . |
| OpenHat   | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |
| Tom       | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |

### Preset 5 — Quantize Demo  ·  **swing 45% (0.60)** · **humanize 85% (0.85)** · **quantize 50% (0.50)** · tempo 100
*Both swing AND scatter present. The student sweeps the Quantize knob and watches/hears the humanize scatter pull back toward the grid WHILE the swing stays put (the DSP-04 invariant, interactive). 16th hats expose both effects.*

| Voice \ step | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Kick      |100| . | . | . | . | . | . | . |100| . | . | . | . | . | . | . |
| Snare     | . | . | . | . |120| . | . | . | . | . | . | . |120| . | . | . |
| Clap      | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |
| ClosedHat | 90| 80| 90| 80| 90| 80| 90| 80| 90| 80| 90| 80| 90| 80| 90| 80|
| OpenHat   | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |
| Tom       | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . | . |

> Clap/OpenHat/Tom are intentionally unused in v1.0 presets — the lessons read cleaner on Kick/Snare/Hat. They remain available to the user as a real instrument (FUNC-08). If a preset wants an open-hat "&" accent for color, add OpenHat at index 14 (vel 90) — but keep the closed-hat choke in mind (a ClosedHat hit on a later step will fast-fade the open tail).

### Transcription helper — `BeatPresets.h` table body

```cpp
namespace OSimpleBeatmaker
{
    inline constexpr std::array<BeatPreset, 6> kBeatPresets {{
        // name                 len  swing hum  quant tempo  grid {Kick,Snare,Clap,ClosedHat,OpenHat,Tom}
        { "Straight",            1, 0.00f,0.00f,1.00f,120.f, {{
            {100,0,0,0,100,0,0,0,100,0,0,0,100,0,0,0},
            {  0,0,0,0,100,0,0,0,  0,0,0,0,100,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {100,0,100,0,100,0,100,0,100,0,100,0,100,0,100,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0} }} },
        { "Backbeat + Accents",  1, 0.00f,0.00f,1.00f,100.f, {{
            {127,0,0,0, 90,0,0,0,100,0,0,0, 90,0,0,0},
            {  0,0,0,0,127,0,0,0,  0,0,0,0,127,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {100,0,50,0,100,0,50,0,100,0,50,0,100,0,50,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0} }} },
        { "Ghost Notes",         1, 0.00f,0.00f,1.00f, 90.f, {{
            {110,0,0,0,  0,0,0,0,100,0,0,0,  0,0,0,0},
            {  0,0,40,0,127,0,40,0,  0,0,40,0,127,0,40,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {100,0,100,0,100,0,100,0,100,0,100,0,100,0,100,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0} }} },
        { "Triplet Swing",       1, 0.80f,0.00f,1.00f, 95.f, {{
            {100,0,0,0,  0,0,0,0,100,0,0,0,  0,0,0,0},
            {  0,0,0,0,110,0,0,0,  0,0,0,0,110,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {100,80,100,80,100,80,100,80,100,80,100,80,100,80,100,80},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0} }} },
        { "Humanized",           1, 0.00f,0.70f,0.25f,110.f, {{
            {100,0,0,0,  0,0,0,0,100,0,0,0,  0,0,0,0},
            {  0,0,0,0,110,0,0,0,  0,0,0,0,110,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {100,0,100,0,100,0,100,0,100,0,100,0,100,0,100,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0} }} },
        { "Quantize Demo",       1, 0.60f,0.85f,0.50f,100.f, {{
            {100,0,0,0,  0,0,0,0,100,0,0,0,  0,0,0,0},
            {  0,0,0,0,120,0,0,0,  0,0,0,0,120,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            { 90,80,90,80,90,80,90,80,90,80,90,80,90,80,90,80},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0},
            {  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0} }} },
    }};
}
```

> The button `data-preset` labels in `index.html` must match `BeatPreset::name` (they do, as transcribed). The name field is for caption text only — the apply path keys on **index**, not name.

---

## 3. Playability Tuning (FUNC-08)

**Primary recommendation: change NO default values in `createParameterLayout`.** Justification:

1. The voice defaults (all decay 0.5, tone 0.5, tune 0, level 0 dB) were tuned in Stage 2 and **already passed QUAL-01** (no clicks/zipper/aliasing at normal ranges) and render-harness Probe 5. The DSP maps them to musical ranges (kick 0.625 s boom, closed hat 55 ms tick, open hat 425 ms sizzle, snare balanced body/noise). `[VERIFIED: DrumVoiceEngine.h:76-86,132-138,187-199,321-338; QUAL-01 Stage-2 verified]`
2. Master output default 0 dB matches the suite convention (O-simpleFM also defaults `outputLevel` to 0 dB). Changing it would diverge from siblings and from saved-session expectations. `[VERIFIED: O-simpleFM/Source/PluginProcessor.cpp:99]`
3. FUNC-08 is `nice` priority. The lowest-risk lever for "sounds good out of the box" is **preset velocity discipline**, which §2 already applies: the downbeat of each preset never stacks all six voices at 127 (kick 100–127, hats 50–100, snare on a different step), so the full-kit sum stays clear of hard clipping.

**One audible check the executor must perform (do NOT skip):** load **Preset 1 (Backbeat + Accents)** at the loudest moment (downbeat: kick 127 + accented hat 100) and watch the output meter / listen for clipping at master 0 dB. The snare and clap voices apply a `× 2.0` gain on their noise component, so an accented snare+kick coincidence is the hottest realistic case. `[VERIFIED: DrumVoiceEngine.h:219 snare noise ×2, :287 clap]`

**Conditional fallback (apply only if that check reveals clipping — regression-check after):**

| If observed | Conservative fix | Why justified |
|-------------|------------------|---------------|
| Full-kit downbeat clips at 0 dB | Lower `outputLevel` default −60..0 from `0.0f` → `-3.0f` in `createParameterLayout` | 3 dB summing headroom; reversible; one-line value change |
| Snare/clap accents harsh vs kit | Lower `snareLevel` / `clapLevel` default `0.0f` → `-2.0f` | Compensates the noise `×2` hot path without touching DSP |

Each fallback is a **default value change only** (no new param, contract intact) and must be followed by a save/reload round-trip + full render-harness re-run to confirm no regression. Flag any change applied in the SUMMARY so verify-phase knows the 42-param *defaults* moved. `[ASSUMED — pending the audible check; the recommendation is "no change" unless clipping is heard]`

---

## 4. Validation Sweep + QUAL-02 Audit

### 4a. Build (macOS: VST3 + AU + Standalone)

```bash
cd /Users/taylorbrook/Dev/VST-development/build
cmake .. -G Ninja -DOUARICON_BUILD_TESTS=ON      # reconfigure to (re)build the harness
ninja O-simpleBeatmaker_VST3 O-simpleBeatmaker_AU O-simpleBeatmaker_Standalone
```

> `PLUGIN_CODE = OSiB`; dev branding ⇒ `PLUGIN_MANUFACTURER_CODE = OuDv`, product suffix `-dev`; release branding (CI) ⇒ `OuAu`, no suffix. `[VERIFIED: CMakeLists.txt:13-16,25-31]`

### 4b. Cache-clear + dual-variant sweep install (from project CLAUDE.md)

**Preferred — the script already does the Phase-4 dual-variant sweep:**
```bash
cd /Users/taylorbrook/Dev/VST-development
./scripts/build-and-install.sh O-simpleBeatmaker
```

**Manual equivalent (substitute the suffix your build produced — `-dev` locally):**
```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
# sweep BOTH variants so the AU triple (aumu/OSiB/Ou*) isn't pinned to a stale bundle
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-simpleBeatmaker.vst3 ~/Library/Audio/Plug-Ins/VST3/O-simpleBeatmaker-dev.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-simpleBeatmaker.component ~/Library/Audio/Plug-Ins/Components/O-simpleBeatmaker-dev.component
cp -R build/plugins/O-simpleBeatmaker/O-simpleBeatmaker_artefacts/Release/VST3/O-simpleBeatmaker*.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-simpleBeatmaker/O-simpleBeatmaker_artefacts/Release/AU/O-simpleBeatmaker*.component ~/Library/Audio/Plug-Ins/Components/
```

### 4c. auval (AU validation)

```bash
auval -a | grep -i beatmaker                 # confirm registration
auval -v aumu OSiB OuDv                       # dev build (use OuAu for a release-branded build)
```
Type is `aumu` (music device / instrument). Expect `AU VALIDATION SUCCEEDED`. `[VERIFIED: CMakeLists.txt PLUGIN_CODE/MANUFACTURER_CODE; IS_SYNTH → aumu]`

### 4d. pluginval (strictness 10, VST3 + AU)

```bash
pluginval --strictness-level 10 --validate-in-process \
  ~/Library/Audio/Plug-Ins/VST3/O-simpleBeatmaker-dev.vst3
pluginval --strictness-level 10 --validate-in-process \
  ~/Library/Audio/Plug-Ins/Components/O-simpleBeatmaker-dev.component
```
Expect `ALL TESTS PASSED`. This covers COMPAT-01 (VST3 + AU) at the final gate.

### 4e. Render-harness re-run (6 probes — DSP regression gate)

```bash
cd /Users/taylorbrook/Dev/VST-development/build
ninja O-simpleBeatmaker_RenderTest        # target name per tests/render-harness/CMakeLists.txt
./plugins/O-simpleBeatmaker/tests/render-harness/O-simpleBeatmaker_RenderTest   # exit 0 = all pass
```
The harness sets params **directly** (not via presets), so presets cannot regress it — a green run confirms the timing DSP is untouched. Probes: 1 grid accuracy, 2 swing, 3 humanize+quantize (incl. DSP-04 swing-survives-quantize), 4 block-boundary, 5 MIDI voices + choke + aliasing, 6 viz truth (QUAL-02 by construction). `[VERIFIED: tests/render-harness/main.cpp:9-25,80-82,407-422]`

### 4f. Hands-on QUAL-02 audible-vs-visible audit (the must-do human check)

QUAL-02 is already truth-by-construction: the viz event carries `appliedSampleInBar − nominalSampleInBar` (the exact Δt baked into the emitted MIDI), and the lane draws that, never a UI recompute. Probe 6 proves the number. Stage 4 is the **hands-on confirmation that the human perceives the match.** `[VERIFIED: PluginProcessor.cpp:200-212 emitSequencerHit; PluginEditor.cpp:200 d = applied−nominal]`

Procedure (in a DAW, transport playing, and in Standalone free-run):
1. Load **Straight** → every lane dot sits exactly on its grid line; playhead crosses a lit cell exactly when you hear that voice; MIDI readout prints SEQ note-ons (36/38/42) with the right velocities.
2. Load **Backbeat + Accents** → the accented kick/snare cells are visibly taller/brighter AND audibly louder; quiet off-beat hats are shorter AND quieter.
3. Load **Triplet Swing** → off-beat hat dots sit to the **right** of their grid lines (late) by a steady amount; the shuffle is audible; quantize at 100% does **not** pull them back (swing survives).
4. Load **Humanized** → dots scatter both sides of the line; raise Quantize live → scatter visibly collapses toward the grid as the feel tightens audibly.
5. Load **Quantize Demo** → sweep Quantize 0→100%: the random scatter pulls in **while the swing lateness remains** — confirm the lane and the ear agree on both halves (DSP-04 made interactive).
6. Screenshot the UI with a preset loaded (grid populated, knobs at preset values, lane animating) — **not** a blank grid — for the verify artifact.

### 4g. CHANGELOG v1.0.0

`CHANGELOG.md` does not yet exist. Author it as the first release entry covering the whole staged build (Foundation → DSP → GUI → Polish): synthesized 808/909 six-voice kit, host-synced sample-accurate sequencer, swing/humanize/quantize feel engine with the DSP-04 invariant, WebView teaching UI (grid + playhead + timing lane + MIDI readout + tooltips), six concept presets, cross-platform WebView2 config. Match sibling CHANGELOG style (e.g. `plugins/O-simpleGrain/CHANGELOG.md`). `[VERIFIED: no CHANGELOG.md present in plugin dir]`

---

## 5. Common Pitfalls

| # | Pitfall | How it bites | Avoidance |
|---|---------|--------------|-----------|
| P1 | **Native-fn bridge gap** | `applyPreset` registered on one side only → buttons silently dead; passes build/auval/harness. | Grep-diff `withNativeFunction` ⇿ `getNativeFunction` (§1e). Expect 5 names both sides. |
| P2 | **`setValueNotifyingHost` normalisation** | Passing a real value (e.g. `120.0f` for tempo, `0.8f` for swing) straight in → param jams to max (clamped to 1.0). | Always `prm->setValueNotifyingHost(prm->convertTo0to1(real))`. Verified arg is 0–1 (juce_AudioProcessorParameter.h:141). |
| P3 | **Choice-param (patternLength) normalisation** | Treating choice index as already-normalised → wrong length. | `convertTo0to1((float) choiceIndex)` over range {0,2,1}: idx 1 → 0.5. Don't hand-compute. |
| P4 | **Regressing the frozen 42-param contract** | Adding a "preset id" param, or renaming an ID, breaks saved sessions + the editor's relay/attachment lists. | Presets set existing params only. No `createParameterLayout` additions. Default *value* changes (FUNC-08) are allowed but must round-trip-test. |
| P5 | **Second `juce_add_binary_data` target** | Embedding presets as a data blob → BinaryData namespace collision (O-simpleGrain Stage 3.1 lesson) = duplicate-symbol link fail. | Preset data is `constexpr` C++ in `BeatPresets.h`. No new binary-data target. |
| P6 | **JS helper-ref regression on module load** | A `ReferenceError` in `app.js` (e.g. calling `applyPresetFn` before it's declared, or a typo) fails silently to build/auval/harness but kills the entire WebView UI. | Declare `applyPresetFn` in `boot()` with the other native-fn lookups; null-guard the call; test the live UI, not just the build. |
| P7 | **Grid not repainting after apply** | Knobs update (attachments) but velocities don't (no attachment for the grid). | Call `refreshGridFromBackend()` in the button handler; the raf poll is the backstop. |
| P8 | **Preset overwriting columns 16–31** | If `clearGrid()` is skipped, a prior 32-step pattern's tail survives behind a 16-step preset. | `clearGrid()` first (zeros all 6×32), then stamp 16 columns. |
| P9 | **Stale host instance during QUAL-02** | A bug clean in harness + Standalone but present in the DAW = stale cached in-memory plugin, not a defect (suite pattern). | Quit/reopen the DAW after install before auditing. |
| P10 | **Choke interaction in presets** | A ClosedHat hit on a later step fast-fades a preset's OpenHat tail. | v1.0 presets avoid OpenHat; if added, place it where no later ClosedHat chokes it. |

---

## 6. Sources

### Primary (HIGH confidence — verified this session)
- Local JUCE 8.0.9 source `/Users/taylorbrook/JUCE`:
  - `juce_AudioProcessorParameter.h:141` — `setValueNotifyingHost(float)` takes normalised 0–1.
  - `juce_RangedAudioParameter.h:114,123` — `convertTo0to1`, `getNormalisableRange`.
  - `juce_NormalisableRange.h:136` — `convertTo0to1`.
  - `juce_AudioProcessorValueTreeState.h:338` — `getParameter` returns `RangedAudioParameter*`.
- O-simpleBeatmaker source tree (Stage 1–3 build): `PluginProcessor.{h,cpp}` (grid API, persistence, processBlock), `PluginEditor.{h,cpp}` (native fns, relays/attachments, Timer frame push), `BeatmakerIDs.h` (roster/GM/IDs), `DrumVoiceEngine.h` (voice DSP + level hot paths), `ui/public/{index.html,js/app.js}` (tour hook, grid refresh, knob/combo binding), `CMakeLists.txt` (codes, test flag), `tests/render-harness/main.cpp` (6 probes).
- `.planning/parameter-spec.md`, `CONTEXT.md`, `ROADMAP.md`, `REQUIREMENTS.md`.

### Secondary (CITED — sibling precedent)
- `O-simpleFM/Source/FactoryPresets.cpp:27-45,77-80` — raw→normalised via `convertTo0to1` + `setValueNotifyingHost` apply pattern (we adopt the technique, drop the disk-JSON manager).
- `O-simpleFM/Source/PluginProcessor.cpp:99` — `outputLevel` default 0 dB (suite convention).
- Project CLAUDE.md — cache-clear + dual-variant sweep install sequence; build targets.
- Project memory — `pattern_webview_native_fn_bridge_gap`, `feedback_module_extraction_regression_check`, `critical_dual_binary_data_namespace_collision`, `pattern_stale_host_instance_vs_offline_repro`.

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | No per-voice param overrides needed in the preset table; verified voice defaults suffice | §1a | Low — a preset could later add voice columns; struct extends cleanly |
| A2 | swing01 0.80 (=60% display) is the right Triplet-Swing value within 58–66% | §2 P3 | Low — single constant; tune to taste in the audible check (0.88 → 66%) |
| A3 | No default-param change needed for FUNC-08; preset velocities suffice | §3 | Medium — gated behind a mandatory audible clipping check with a one-line fallback |
| A4 | Render-test target is `O-simpleBeatmaker_RenderTest` | §4e | Low — confirm exact target name in `tests/render-harness/CMakeLists.txt` at execute time |

---

## Metadata

**Confidence breakdown:**
- Preset architecture / apply path: **HIGH** — every API verified against local JUCE source; every code seam read from the live tree; persistence path confirmed.
- Preset velocity grids: **HIGH** (mechanically correct, transcribe-ready) / musical taste: MEDIUM — confirm in the audible pass.
- Playability: **MEDIUM** — recommendation is "no blind change" + one human check; conditional fallbacks specified.
- Validation sweep: **HIGH** — codes, flags, and CLAUDE.md sequence all verified.

**Research date:** 2026-06-25
**Valid until:** stable (no fast-moving deps; JUCE pinned 8.0.9) — re-verify only if JUCE bumps or the 42-param contract changes.
