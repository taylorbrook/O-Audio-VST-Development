---
status: issues
plugin: O-simplePhysicalModelSynth
version: 1.0.0
date: 2026-07-15
depth: deep
files_reviewed: 23
findings:
  critical: 3
  warning: 5
  info: 10
  total: 18
---

# O-simplePhysicalModelSynth v1.0.0 — Code Review (deep)

**Scope:** all 23 submitted source files (C++ DSP/processor/editor, WebView UI HTML/CSS/JS, plugin + render-harness CMake), plus cross-module tracing into `modules/persistence/preset-manager/cpp/OuariconPresetManager.h` and `modules/synthesis/bow-friction/cpp/HyperbolicFriction.h` (consumed by this plugin, not themselves in scope).

**Known-pattern audit (12 checks from suite history):**

| # | Pattern | Result |
|---|---------|--------|
| 1 | RT-safety (alloc/locks on audio thread) | **FAIL — CR-01** (String-keyed map lookups in `processBlock`); IIR coeffs OK (`ArrayCoefficients` in-place in BowNoiseGenerator, raw biquads elsewhere) |
| 2 | Factory presets: engineering units + reset-before-apply | Units **PASS** (raw values + `convertTo0to1`); reset-before-apply present in module (WR-01 fix) but the Material macro breaks user-preset/session recall — **CR-03** |
| 3 | "/" in factory preset name ("Koto / Harp") | **VERIFIED FIXED** — `FactoryPresets.cpp:90` names it "Koto Harp", with an authored comment explaining why |
| 4 | JS↔C++ native-fn bridge symmetry | **PASS** — 12 registered (`savePreset`, `savePresetWithDialog`, `loadPreset`, `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`, `uiMidi`, `getSampleRate`); all 12 JS call sites (10 in preset-manager.js + 2 in app.js) match. Doc comment says "10" — see IN-07 |
| 5 | Knob readouts via `getScaledValue()` | **PASS** — `app.js:107` uses `st.getScaledValue()`; no hardcoded JS range map. No dblclick-reset at all — see IN-09 |
| 6 | NaN guards reset state but not coeffs | **PASS** — ModalResonator guard resets state; coeffs are recomputed at block rate from clamped params so they cannot go stale; excitation is feed-forward. String loop has no guard but all its inputs are clamped and `startNote` resets it |
| 7 | Async FileChooser + SafePointer / no complete() on null | **FAIL — CR-02** |
| 8 | Audio-thread frees via shared_ptr swap | **PASS** — no shared-state swap pattern exists in this plugin |
| 9 | Viz cross-thread handoff | **PASS** — VizTap/VizRing are atomics-only, audio thread copy-only, FFT on the editor Timer; editor dtor stops the Timer first |
| 10 | CMake WebView2 flags / dual BinaryData namespace | **PASS** — `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`; single binary-data target so default namespace is fine |
| 11 | Harness vs WebView editor seam | **PASS** — harness compiles PluginProcessor.cpp at `JUCE_WEB_BROWSER=0`, no PluginEditor.cpp, guarded `#include`, FactoryPresets.cpp linked |
| 12 | Physical-model DSP correctness | Mostly **PASS** (phase-delay-at-f0 compensation, Thiran ≥2 clamp, feedback <1, Q/Nyquist clamps, harness gates tuning ±5¢) — but delay sizing doesn't cover the tuning range (WR-03) and pitch bend is dead (WR-04) |

---

## Critical

### CR-01: Heap allocation on the audio thread — `juce::String` temporaries in every `processBlock` parameter read

**File:** `Source/PluginProcessor.h:142-143`, used from `Source/PluginProcessor.cpp:194-213` (`readParams`), `:260`, `:318`

```cpp
std::atomic<float>* p (const char* id) const { return rawParams.count (id) ? rawParams.at (id) : nullptr; }
std::map<juce::String, std::atomic<float>*> rawParams;
```

**Issue:** `rawParams` is keyed by `juce::String`, and `p()` takes `const char*`. Every call constructs **two** `juce::String` temporaries (one for `count`, one for `at`), and `juce::String(const char*)` heap-allocates a `StringHolder`. `readParams()` makes 14 `p()` calls and `processBlock` two more (`outputLevel`) — **~32 heap allocations per audio block, every block**. This directly contradicts the design comment on the same lines ("no string hashing in processBlock") and defeats the entire cached-atomics scheme. Malloc on the audio thread is a priority-inversion / RT-violation hazard (glitch under memory pressure).

**Fix:** Replace the map with named members or a fixed array — zero lookups at render time:
```cpp
// cached once in the constructor:
std::atomic<float>* pExcitationType  = nullptr;
std::atomic<float>* pExcitationPos   = nullptr;
// ... one per param, assigned in cacheParamPointers() ...
pp.excitationType = (int) pExcitationType->load();
```
(Or `std::array<std::atomic<float>*, 17>` indexed by an enum.)

---

### CR-02: FileChooser `launchAsync` completions capture raw `this` and call `complete()` unconditionally — use-after-free on editor teardown

**File:** `Source/PluginEditor.cpp:107-126` (`savePresetWithDialog`) and `:133-152` (`loadPresetFromFile`)

```cpp
fileChooser->launchAsync (
    juce::FileBrowserComponent::saveMode | ...,
    [this, complete] (const juce::FileChooser& fc) {
        ...
        bool ok = processorRef.getPresetManager().savePreset (name);   // `this` may be dead
        ...
        complete (juce::var (result));                                 // `complete` owned by dead WebView Impl
    });
```

**Issue:** This is the exact pattern already shipped as a fix in O-MicrotonalSampler v1.23.5 (W12) and flagged for audit across all WebView editors. If the host destroys the editor while the native save/load dialog is open (window closed, plugin removed, project closed), the async completion fires afterwards:
1. `this->processorRef` and the `fileChooser` member are dangling → UAF.
2. `complete` is a callback owned by the destroyed WebView `Impl` — calling it **on any path, including the cancel path** is itself a UAF. The review-standard "`complete(false)` on cancel" is not safe here either.

**Fix:** Guard with `Component::SafePointer` and bail with a bare `return` when null:
```cpp
fileChooser->launchAsync (flags,
    [safeThis = juce::Component::SafePointer (this), complete] (const juce::FileChooser& fc) {
        if (safeThis == nullptr)
            return;                       // editor gone — do NOT touch complete()
        ...
        complete (juce::var (result));
    });
```
Apply to both native functions.

---

### CR-03: Material macro stomps explicitly saved Damping/Decay on user-preset load and DAW session restore

**Files:** `Source/PluginProcessor.cpp:219-237` (`parameterChanged`), interacting with `modules/persistence/preset-manager/cpp/OuariconPresetManager.h:298-308` (`applyPresetJson`) and `:547-556` (`setStateFromXml` → `replaceState`)

**Issue:** The macro listener re-derives `damping` + `decay` whenever `material` changes value. Both persistence paths apply `material` **after** `damping`/`decay`:

- **User presets:** `createPresetJson` (module `:257-264`) saves all 17 params in layout order — `damping` (idx 8), `decay` (idx 9), `material` (idx 10). `applyPresetJson` resets all params to defaults, then applies saved values in that same order. If the saved `material` differs from its post-reset value (default 30 — i.e. almost always), the listener fires **after** the saved `damping`/`decay` were applied and overwrites them with macro-derived values. Any user preset where Damping/Decay were hand-tweaked after setting Material recalls wrong. Example: material=80 + hand-set damping=20 → recalls damping≈71. The JSON on disk is correct; recall is not — silent settings loss.
- **Session restore:** `setStateInformation` → `presetManager.setStateFromXml` → `parameters.replaceState`. Parameter adapters update in the same layout order and fire `parameterChanged` synchronously, so the restored `material` re-derives and stomps the restored `damping`/`decay` the same way. Every saved DAW project with tweaked Damping/Decay reopens with the wrong timbre.

Factory presets are unaffected only because they follow the never-co-author rule (`FactoryPresets.cpp:14-20`) — but that rule cannot be imposed on user saves, which always contain all three.

**Fix (two parts):**
1. **Preset path:** apply `material` first, then the remaining keys, so explicit saved values win while factory String presets (which omit damping/decay) still get the derivation. Either special-case in `applyPresetJson` (module change) or plugin-side by ordering; the listener must keep firing for factory presets.
2. **State path:** all 17 params are always present in the XML, so no derivation is needed — suppress the macro during restore:
```cpp
std::atomic<bool> restoringState { false };
// setStateInformation:
restoringState = true;
presetManager.setStateFromXml (xml.get());
restoringState = false;
// parameterChanged:
if (restoringState.load()) return;
```
Related note: the reset-to-defaults pass inside `applyPresetJson` also fires the macro (material reset X→30), leaving the post-reset "default" damping/decay at ≈32/≈88 rather than the layout defaults 60/70 — harmless today (every factory preset sets one side or the other) but worth knowing when authoring future presets.

---

## Warnings

### WR-01: Hard voice stop (CC120 All Sound Off / `stopNote(…, false)`) does not silence the voice and leaves it burning CPU forever

**File:** `Source/PhysicalModelVoice.h:167-174` (`stopNote`), `:180-218` (`renderNextBlock`)

**Issue:** The hard-stop path calls `clearCurrentNote()` but never resets `ampEnv`. The env was noteOn'd with sustain=1 and never noteOff'd, so `ampEnv.isActive()` stays true **forever**. `juce::Synthesiser` calls `renderNextBlock` on every voice every block regardless of active state, and the only render guard is `ampEnv.isActive()` — so a hard-stopped voice (a) keeps adding its ringing string/modal tail to the output at full envelope (a MIDI CC120 "All Sound Off" panic audibly fails to silence the synth), and (b) keeps running the full KS + modal + exciter DSP indefinitely, permanently, until that voice happens to be reused by a new note. Voice-steal masks this (steal is immediately followed by `startNote`), but CC120 (`Synthesiser::handleController` → `allNotesOff(ch, false)`) and `releaseResources` hit it directly.

**Fix:**
```cpp
void stopNote (float, bool allowTailOff) override
{
    noteHeld = false;
    if (allowTailOff)
        ampEnv.noteOff();
    else
    {
        ampEnv.reset();        // env goes inactive → render guard exits
        string.reset();        // kill the circulating energy too
        modal.reset();
        clearCurrentNote();
    }
}
```

### WR-02: Material macro listener can run on the audio thread under host automation → `setValueNotifyingHost` from `processBlock`

**File:** `Source/PluginProcessor.cpp:219-237`

**Issue:** The comment claims the listener "runs on the message thread — never on the audio thread", but APVTS listeners fire synchronously on whatever thread changed the value. In VST3, host automation of `material` is applied on the **audio thread** during processing; `parameterChanged` then calls `setValueNotifyingHost` on `damping`/`decay`, which drives the host edit callback (`performEdit`) from the processing thread — a VST3 spec violation that can deadlock or allocate in some hosts. `withMeta(true)` is correctly set, but that only informs the host; it doesn't move the callback.

**Fix:** Defer off-thread work: have `parameterChanged` stash the target values and `triggerAsyncUpdate()` (message thread performs the two `setValueNotifyingHost` calls), or early-out via `MessageManager::getInstance()->isThisTheMessageThread()` and post otherwise. Keep the derivation math where it is.

### WR-03: String delay line sized for 20 Hz but the tuning range reaches 8 Hz — low notes clamp mistuned (+ debug assert storm)

**File:** `Source/StringResonator.h:39` (`maxDelay = fs/20 + 100`), `:69-77` (`setFrequency` clamps f0 ≥ 8 Hz), with `Source/PhysicalModelVoice.h:243-249` (`computeF0`)

**Issue:** `computeF0` can legitimately request well below 20 Hz: MIDI 0-13 alone (8.18-18.9 Hz), and `coarseTune = −24` pushes anything below MIDI 38 (~61.7 Hz) under 20 Hz. `setFrequency` clamps at 8 Hz — implying 8 Hz is supported — but `recomputeDelay` then asks the DelayLine for up to `fs/8 ≈ 5513` samples against a max of `fs/20+100 ≈ 2305`. `juce::dsp::DelayLine::setDelay` jasserts and clamps, so every such note (a) fires a per-set assert in debug builds and (b) plays at a fixed ~19 Hz regardless of the requested pitch.

**Fix:** Size for the actual floor: `const int maxDelay = (int)(sampleRate / 8.0) + 100;` — or clamp `f0` in `setFrequency` to `sampleRate / maxDelaySamples` so the clamp and the sizing agree.

### WR-04: Pitch bend is tracked but never applied — pitch wheel silently does nothing

**File:** `Source/PhysicalModelVoice.h:176` (`pitchWheelMoved`), `:243-249` (`computeF0`), `:141` (`startNote` stores it)

**Issue:** `pitchWheelPos` is faithfully captured in `startNote` and `pitchWheelMoved` but `computeF0` never reads it. The wheel is dead: no bend on any note, ever — while the code structure (stored member, handler override) tells every future reader it works. Either it's an unshipped feature or dead code; as shipped it's a silently broken standard MIDI behavior on an instrument plugin.

**Fix:** Fold it into `computeF0` (block-rate is fine — `setParams` already re-runs `computeF0`/`setFrequency` for active voices every block):
```cpp
const float bendSemis = 2.0f * (float)(pitchWheelPos - 8192) / 8192.0f;  // ±2 st
const float semis = ... + bendSemis;
```
(Or delete the member + override and document no-bend — but supporting it is ~3 lines.)

### WR-05: `getTailLengthSeconds() = 5.0` under-reports the KS ring by an order of magnitude

**File:** `Source/PluginProcessor.h:94`; `Source/StringResonator.h:89-94`

**Issue:** The comment says "max amp release", but the tail is dominated by the resonator, not the amp env (sustain=1; the release only gates the resonator's own ring-out). At `decay=100` the loop feedback is 0.999 → T60 ≈ 6904 loop passes ≈ `6904 / f0` seconds: **15.7 s at A4, ~106 s at C2**. The modal bank reaches T60 7.2 s (6.0 × 1.2 multiplier). Hosts that honor tail length for offline bounce / freeze will truncate long rings audibly.

**Fix:** Report a value covering the real worst case (or compute from the current decay param), e.g. `return 30.0;` with a comment deriving it, accepting that very low notes at max decay are still approximate — or clamp the effective feedback so T60 is bounded and report that bound.

---

## Info

### IN-01: Bow mode leaves three dead-but-live controls ungated (Color, Position, Vel→Bright)

**File:** `Source/ui/public/js/app.js:217-224` (`applyEngineGating`); `Source/BowExciter.h:38` (`color01` explicitly ignored); `Source/PhysicalModelVoice.h:262-284` (position/cutoff only used by Pluck/Strike)

**Issue:** The D5 grey-out gates `bowForce`/`inharmonicity`/`modeBrightness`/`stringModel`, but with Excitation=Bow, `excitationColor`, `excitationPosition`, and `velToBrightness` have no effect (BowExciter discards color; position comb and exciter cutoff are Pluck/Strike-only) yet remain fully lit — a confusing dead surface on a pedagogical plugin. Also: `.pm-disabled` uses `pointer-events:none` but knobs keep `tabindex=0`, so Tab + arrow keys can still edit "disabled" controls.

**Fix:** Add `setDisabled("excitationColor", exc === 2)` etc. in `applyEngineGating` (or wire color into the bow's noise bandpass in DSP), and toggle `tabindex`/`aria-disabled` alongside the class.

### IN-02: `stringModel` "Waveguide" option is a selectable no-op

**File:** `Source/PluginProcessor.cpp:75-77`; `Source/FactoryPresets.cpp:23-24`; parameter never read by any DSP

**Issue:** The choice param exists for v1.1 forward-compat and the tooltip discloses it, but selecting "Waveguide" changes nothing — the param is not read anywhere in the voice. A student toggling it hears no difference and gets no cue beyond the tooltip.

**Fix:** Disable the "Waveguide" `<option>` (not the combo) in `bindCombo`, or append "(v1.1)" to the option label.

### IN-03: Spectrum axis Nyquist fetched once at boot — stale if the editor opens before `prepareToPlay` or the rate changes

**File:** `Source/ui/public/js/app.js:434-441` (`fetchSampleRate`), `Source/PluginEditor.cpp:189-191`

**Issue:** `getSampleRate()` returns 0 before the host prepares; the `sr > 0` guard then leaves `nyquistHz` at the 22.05 k default permanently, and a later sample-rate change is never picked up — tick labels drift from the analyzer's real 20 Hz→Nyquist mapping (analyzer uses the live rate each tick).

**Fix:** Include the sample rate in the 30 Hz `loopUpdate` payload (one extra property) and update `nyquistHz` from it, or re-fetch on each `spectrumUpdate` until non-zero.

### IN-04: Exciter cutoff clamp (200-18000 Hz) not bounded by Nyquist

**File:** `Source/PhysicalModelVoice.h:259`; `Source/OnePoleLPF.h:34-42`; `Source/PluckExciter.h:46`; `Source/StrikeExciter.h:49`

**Issue:** `OnePoleLPF::setCutoff` computes `tan(π·fc/fs)` with no Nyquist clamp. At fs < 36 kHz (e.g. 22.05/32 kHz hosts), a bright Color + high velocity pushes fc past Nyquist → `tan` past π/2 → negative `n` → invalid coefficients. `StringResonator::setLoopCutoff` clamps to 0.49·fs; the exciter path does not.

**Fix:** Clamp in `setCutoff` itself: `cutoffHz = jmin(cutoffHz, (float)(0.49 * sampleRate));` — one definition, all three consumers covered.

### IN-05: PositionComb 8192-sample buffer clamps low-note positions at ≥96 kHz

**File:** `Source/PositionComb.h:35, 56`

**Issue:** At 96 kHz, `pos·(fs/f0)` for low notes exceeds `MAX_DELAY−2` (e.g. f0=20 Hz, pos=0.5 → 2400 OK; f0=10 Hz → 4800 OK; pos 0.95 at f0 ≤ 12 Hz → >8190) and at 192 kHz it clamps for much of the bass range — the comb nulls land at the wrong positions (Position knob compresses at one end). Safe (clamped), just wrong.

**Fix:** Size from sample rate at prepare time (`fs/8 · 0.95 + margin`) or accept and document the 44.1/48 k design point.

### IN-06: 32 `dynamic_cast`s per audio block

**File:** `Source/PluginProcessor.cpp:311-313` and `:347-353`

**Issue:** Both the param-push loop and `publishViz` `dynamic_cast` all 16 voices every block. Not an RT violation, but it's avoidable per-block RTTI cost — and the voices are known-typed at construction.

**Fix:** Keep a `std::array<PhysicalModelVoice*, kNumVoices>` filled in the constructor next to `synth.addVoice(v)` and iterate that.

### IN-07: Stale stage/inventory comments contradict the shipped code

**Files:** `CMakeLists.txt:5-14` (claims "D1: no juce_add_binary_data / WebView editor" — both exist below in the same file); `Source/PluginProcessor.h:10-15` and `Source/PluginProcessor.cpp:6-10` ("silent synth shell… no audio rendering yet"); `tests/render-harness/CMakeLists.txt:1-7` ("Stage-1 build/link smoke-stub" vs the Stage-2.3 harness in main.cpp); `Source/PluginEditor.cpp:6-7` and `PluginEditor.h:10` ("10 native fns" — 12 are registered).

**Issue:** The stage-progression comments were never updated after Stages 2/3 landed; several now assert the opposite of what the file does, which will mislead the next maintainer (and the D1 CMake comment sits directly above the binary-data target it denies).

**Fix:** One doc sweep: update the four headers to describe v1.0.0 as shipped.

### IN-08: Dead CSS selectors + preset bar boots showing a nonexistent "Default" preset

**Files:** `Source/ui/public/css/styles.css:222-229` (`.reserved-note` — no such element remains), `:505` (`.tour-btn` — no such element); `Source/ui/public/index.html:23` (initial label "Default")

**Issue:** Two dead selectors are leftover from the reserved-panel phase / a copied theme. Separately, the preset button's initial text is "Default", and `setStateFromXml` falls back to `"Default"` — but this plugin deliberately ships no Default preset (`FactoryPresets.h:14-17`), so a fresh instance displays a preset name that doesn't exist in the dropdown.

**Fix:** Delete the dead rules; boot the label as "—" or "Init" (and align the module fallback string) so the displayed name always corresponds to a loadable entry or an explicit "no preset" state.

### IN-09: Knob drag lacks pointer capture; no double-click reset-to-default

**File:** `Source/ui/public/js/app.js:143-175`

**Issue:** (a) The drag uses `window` pointermove/up without `setPointerCapture` — releasing the mouse outside the plugin window can drop the `pointerup`, leaving the knob glued to the cursor (and the drag gesture open host-side) until the next click. (b) The suite's knob pattern includes double-click reset via a `getParameterDefaults` native fn; this UI has no reset gesture at all.

**Fix:** `knob.setPointerCapture(e.pointerId)` in pointerdown (listeners then live on the knob, guaranteed `pointerup`/`pointercancel`); add a `dblclick` handler that fetches real defaults from C++ (never hardcode them in JS).

### IN-10: `savePresetWithDialog` ignores the directory the user picks

**File:** `Source/PluginEditor.cpp:107-126`

**Issue:** The completion takes only `getFileNameWithoutExtension()` and calls `savePreset(name)`, which always writes to the user-presets directory. A user who navigates the save dialog to `~/Desktop/foo.json` gets a file silently written to `~/Library/.../Presets/User/foo.json` instead. Nothing is lost, but the dialog's folder chooser is decorative.

**Fix:** Either write the JSON to the chosen path (`createPresetJson` → `File::replaceWithText`) in addition to registering it, or use a name-entry flow instead of a full file dialog so no false affordance is shown.

---

## Cross-file observations (verified clean)

- **Editor member destruction order** (relays → WebView → attachments) is correct for JUCE 8 WebView teardown (`PluginEditor.h:43-60`), and the destructor stops the Timer before members die.
- **Resource provider** matches bare paths by direct equality (`PluginEditor.cpp:38-58`) — every file referenced by `index.html`/`app.js` (including `../modules/preset-manager.js` and `js/juce/check_native_interop.js`) has a mapping.
- **Percent params** are true 0-100 ranges (D3 contract held); factory presets author raw units through `convertTo0to1`, so the `kTimeSkew` on ampAttack/ampRelease is handled correctly.
- **Harness** re-derives nothing from the plugin target except include dirs (compile defs not inherited), so its `JUCE_WEB_BROWSER=0` seam holds; `FactoryPresets.cpp` is linked as required.
- **ModalResonator** math checks out: Fletcher stretch, Q = πfT60/ln(1000) ≈ 0.4548·f·T60, Nyquist/Q clamps, sum-normalization; feed-forward bank cannot go unstable, and the per-biquad isfinite guard returns 0 (not the NaN) on trip.
- **KS loop stability**: feedback hard-clamped ≤ 0.999 with a ≤ unity-gain loop LPF and an output DC blocker; harness gates blow-up, DC, tuning (±5¢ C1-C7), bow sustain at max force/decay, and modal cross-driving.

---

_Reviewed: 2026-07-15_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
