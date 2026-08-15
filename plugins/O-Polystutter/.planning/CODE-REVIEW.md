# O-Polystutter — Code Review

**Plugin:** O-Polystutter (Beat Repeater audio effect)
**Version reviewed:** 1.12.2
**Date:** 2026-07-01
**Reviewer:** Claude (adversarial code review)

## Files Reviewed

| File | Lines |
|------|-------|
| Source/PluginProcessor.cpp | 1829 |
| Source/PluginProcessor.h | 236 |
| Source/PluginEditor.cpp | 987 |
| Source/PluginEditor.h | 429 |
| Source/DSP/RepeatLane.cpp / .h | 751 / 170 |
| Source/DSP/TapeDegrader.cpp / .h | 361 / 83 |
| Source/DSP/TriggerRouter.cpp / .h | 69 / 43 |
| Source/OPolystutterPresetManager.h | 552 |
| CMakeLists.txt | 85 |
| Source/ui/public/index.html | 1742 |
| Source/ui/public/js/parameter-bindings.js | 1021 |
| Source/ui/public/modules/preset-manager.js | 387 |

## Summary

| Severity | Count |
|----------|-------|
| CRITICAL | 2 |
| WARNING | 8 |
| IMPROVEMENT | 11 |
| NOTE | 6 |
| **Total** | **27** |

Overall: the parameter/relay/attachment plumbing is complete and consistent (all 10 native functions used by JS are registered; no orphans in either direction), and the CMake config is one of the few in the suite with correct Windows WebView2 flags. The two critical items are a real-time-safety violation in the tape rolloff filter and the known WebView FileChooser UAF pattern. The most surprising correctness finding is that the Bjorklund/Euclidean generator is mathematically wrong for most pulse counts — including the tresillo/cinquillo patterns the factory presets are named after — and two advertised lane features (FILTER knob, MAN toggle) are wired to parameters that no DSP code reads.

---

## CRITICAL

### CR-01: Heap allocation on the audio thread — `IIR::Coefficients::makeLowPass` in rolloff path

**File:** `Source/DSP/TapeDegrader.cpp:187-191` → `TapeDegrader.cpp:325-340`

`processBlock` calls `updateRolloffFilter()` whenever `rolloffAmount` changes by >0.001:

```cpp
if (std::abs(rolloffAmount - lastRolloffAmount) > 0.001f)
{
    updateRolloffFilter();   // called from the audio thread
    ...
}
```

`updateRolloffFilter()` uses `juce::dsp::IIR::Coefficients<float>::makeLowPass(...)`, which **heap-allocates a new reference-counted Coefficients object** and frees it at scope exit — on the audio thread, once per block, for the entire duration of a `tape_rolloff` knob drag or automation ramp. The comment on line 186 ("avoid allocation in audio thread") only avoids the *per-block-when-static* case; every actual change still allocates.

**Failure scenario:** Automate `tape_rolloff` (a continuous 0–100% float, so it changes every block during a ramp) → malloc/free on the RT thread every block → allocator lock contention / priority inversion → audible dropouts under load.

**Fix:** Use the RT-safe pattern already shipped in O-Formant v1.25.1: `juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sampleRate, cutoffFreq, 0.707f)` returns a stack `std::array` with identical math; assign in place:

```cpp
const auto c = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sampleRate, cutoffFreq, 0.707f);
rolloffFilterLeft.coefficients->coefficients  = c;  // or memcpy into *state
rolloffFilterRight.coefficients->coefficients = c;
```

(`updateHissBandpass` at `TapeDegrader.cpp:342-361` has the same `makeHighPass`/`makeLowPass` calls but is only invoked from `prepare()`, so it is message/host-thread only — fix it too while there for consistency.)

### CR-02: FileChooser `launchAsync` completions capture raw `this` and call `complete()` unconditionally — UAF on editor teardown

**File:** `Source/PluginEditor.cpp:397-427` (`savePresetWithDialog`) and `PluginEditor.cpp:434-463` (`loadPresetFromFile`)

Both native functions do:

```cpp
auto* chooser = new juce::FileChooser("Save Preset", userDir, "*.json");
chooser->launchAsync(..., [this, chooser, complete](const juce::FileChooser& fc) {
    ...
    processorRef.presetManager.savePreset(name);   // uses captured `this`
    complete(juce::var(result));                   // completion owned by WebView Impl
    delete chooser;
});
```

Problems:
1. **Raw `this` capture, no `Component::SafePointer`.** If the user closes the plugin window (or the host destroys the editor) while the OS file dialog is open, the async completion later dereferences a dangling editor pointer.
2. **`complete()` is owned by the (now-destroyed) WebView Impl.** Per the suite-wide finding shipped in O-MicrotonalSampler v1.23.5 (W12), even calling `complete(false)` after teardown is itself a UAF — the completion must bail with a bare `return`.

**Failure scenario:** Open Save Preset dialog → close the plugin editor in the DAW → click Save/Cancel in the still-open dialog → crash (or silent memory corruption) in the host.

**Fix (pattern from O-MicrotonalSampler v1.23.5):**

```cpp
juce::Component::SafePointer<OPolystutterAudioProcessorEditor> safeThis(this);
auto chooser = std::make_shared<juce::FileChooser>("Save Preset", userDir, "*.json");
chooser->launchAsync(flags, [safeThis, chooser, complete](const juce::FileChooser& fc) {
    if (safeThis == nullptr)
        return;   // bare return — do NOT call complete() on the null path
    ...
    complete(juce::var(result));
});
```

Using `std::shared_ptr` for the chooser also removes the manual `new`/`delete` pair.

---

## WARNING

### WR-01: Bjorklund/Euclidean algorithm drops pulses — most canonical patterns are wrong

**File:** `Source/DSP/RepeatLane.cpp:654-751` (`generateEuclideanPattern`); mirrored in `Source/ui/public/js/parameter-bindings.js:785-846`

The iteration computes the leftover group as `remaining = totalSeqs - numA - pairs`, which only counts leftover **B** sequences. Whenever an iteration has `numA > numB`, the `numA - numB` leftover **A** sequences (each containing a pulse) sit at indices `[pairs, numA)` but are neither moved nor counted — `totalSeqs = pairs + remaining` silently drops them.

Concrete trace, E(3,8) (the "Cuban tresillo" the comment on line 672 promises):
- Init: A=3×[1], B=5×[0], totalSeqs=8
- Iter 1: pairs=3 → A=[1,0]×3; remaining=8−3−3=2 leftover B's moved; totalSeqs=5, numA=3, numB=2
- Iter 2: pairs=2 → A0=A1=[1,0,0]; **remaining=5−3−2=0 → A2=[1,0] is dropped**; totalSeqs=2
- Result: `[1,0,0,1,0,0,0,0]` — 2 pulses over 6 steps, not `[1,0,0,1,0,0,1,0]`.

Similarly E(5,8) (cinquillo) yields `[1,0,1,0,1,0,…]` with only 3 pulses. Every pattern where any iteration ends with `numA > numB` is wrong — that includes E(3,8), E(5,8), E(7,16), E(5,12) and E(5,16), i.e. **all the rhythms used by the "Euclidean Groove", "Afro-Latin Stutter" and "Minimal Pulse" factory presets** (PluginProcessor.cpp:1063-1118). The JS preview (parameter-bindings.js:806-835) is an exact port of the same bug, so the UI preview "confirms" the wrong pattern.

**Fix:** Account for leftover sequences of *either* group. After pairing, the remainder group is `abs(numA - numB)` sequences; when `numA > numB` the leftovers are already contiguous at `[pairs, numA)` and just need counting:

```cpp
int leftoverA = numA - pairs;          // when numA > numB
int leftoverB = numB - pairs;          // when numB > numA
int remaining = juce::jmax(leftoverA, leftoverB);
// move leftover B's down (existing loop) only when leftoverB > 0
totalSeqs = pairs + remaining;
numA = pairs;
numB = remaining;
```

Also update the loop exit: standard Bjorklund stops when the remainder count ≤ 1 (which `numB > 1` then correctly expresses). Fix the JS mirror identically, and add unit assertions for E(3,8)/E(5,8)/E(5,16) against the canonical sequences already written in the comment.

### WR-02: Lane FILTER knob is completely non-functional (dead parameter) — ✅ RESOLVED v1.13.0 (implemented as spec'd bipolar LP/HP sweep; liveness-probe verified −54.5 dB LP / −45.3 dB HP)

**File:** `Source/PluginProcessor.cpp:58-63` (param), `PluginEditor.cpp:23/218/505` (relay/attachment), `index.html:1041-1044` (knob with tooltip "Applies low-pass (negative) or high-pass (positive) filtering to repeats")

`laneN_filter` is declared, bound to the UI, saved in state — but **no DSP code ever reads it**: there is no cached `getRawParameterValue("laneX_filter")` in the processor, no `setFilter` on `RepeatLane`, and no filter in the lane signal path. The knob rotates, automates, and does nothing.

**Failure scenario:** User turns FILTER to −100 expecting a low-pass on repeats; nothing changes. Automation of the parameter is silently ignored.

**Fix:** Either implement the per-lane filter (state-variable LP/HP in `RepeatLane::processBlock`, coefficients via `ArrayCoefficients` per CR-01's pattern) or remove the parameter + knob. Do not ship a knob whose tooltip describes behavior that doesn't exist.

### WR-03: Lane MAN (manual-time) toggle is dead — `manualTimeEnabled` is stored but never read

**File:** `Source/DSP/RepeatLane.cpp:558-561`, `RepeatLane.h:106`; UI at `index.html:1092` (tooltip: "Ignores beat-sync timing. Use with TRIG button for one-shot stutters")

`setManualTimeEnabled()` writes the member; nothing in `RepeatLane` or the processor ever branches on `manualTimeEnabled`. Beat-sync triggering in `updateBeatSync` (PluginProcessor.cpp:1754-1799) fires regardless of the MAN state, so "ignores beat-sync timing" is false — a MAN-enabled lane still re-triggers every subdivision while the transport runs.

**Fix:** In `updateBeatSync`, skip the beat-sync `lane->trigger()` for lanes whose manual-time parameter is set (the per-lane values are already read at PluginProcessor.cpp:1258-1270 — pass them into `updateBeatSync` or check the cached params there). Or remove the toggle.

### WR-04: Beat-sync boundary detection drops the trigger at DAW loop-wrap / backward relocate

**File:** `Source/PluginProcessor.cpp:1757-1799`

Boundary detection is `currentSubdiv > lastSubdiv` with `int` truncation. When the host loops (ppq jumps from e.g. 32.0 back to 1.0) or the user relocates backward, `currentSubdiv < lastSubdiv` and no lane triggers that block — the loop-start downbeat, the most audible trigger point, is silently skipped on every loop pass. Triggers resume one block later once `lastPPQPosition` has been rewritten (line 1803), so exactly one boundary is lost per wrap. Additionally, `static_cast<int>` truncates toward zero, so during a count-in (negative ppq) positions −0.9…+0.9 all map to subdivision 0 and pre-roll boundaries are mis-detected.

**Fix:** Detect wrap explicitly: if `ppqPosition < lastPPQPosition`, treat it as a fresh boundary (trigger if the lane is enabled, or re-seed `lastPPQPosition = ppqPosition - subdivPPQ` so the next comparison fires). Use `std::floor(ppq / subdivPPQ)` instead of `static_cast<int>` for correct negative-ppq behavior.

### WR-05: Capture buffer is overwritten by the live write head during long repeat tails

**File:** `Source/DSP/RepeatLane.cpp:75-86` (write always advances) vs `RepeatLane.cpp:325-329` (read region fixed at trigger)

The circular capture buffer is 5 s (`prepare`, line 23). While a lane plays repeats, incoming audio keeps writing forward every sample. The read region `[captureStartPosition, captureStartPosition + captureLength)` is fixed at trigger time, so once `maxCaptureSamples − captureLength` samples have elapsed since the trigger (~5 s), the write head wraps into the read region and later repeats read **live input** instead of the captured slice.

**Failure scenario:** "Ambient Freeze" factory preset (1/4-note, 16 repeats, decay 98%): at 120 BPM the tail is 16 × 0.5 s = 8 s > ~4.5 s available. Trigger it (or stop the transport so beat-sync stops re-triggering), keep feeding input: after ~4.5 s the "frozen" repeat audibly mutates into delayed live input. Any repeats × subdivision product exceeding ~5 s hits this; pitch-down (ratio < 1) makes it worse by lengthening the effective read span.

**Fix:** Either pause capture writes while `isTriggered && currentRepeat < maxRepeats` (classic freeze semantics), or copy the captured slice into a dedicated per-lane snapshot buffer at trigger time (memcpy of ≤5 s, still RT-safe as a bounded copy of pre-allocated memory — or copy incrementally), or clamp `maxRepeats × subdivisionSamples` against the buffer headroom.

### WR-06: MIDI trigger notes contradict UI tooltip and code comments

**File:** `Source/DSP/TriggerRouter.cpp:50-66`; `index.html:1534`

Code triggers on MIDI notes **60, 61, 62, 63** (lanes 1-4) and **67** (all lanes). The UI tooltip says: "Notes **C1-B1** trigger lanes 1-4, **any other note triggers all enabled lanes**." Both claims are false — C1-B1 (notes 24-35) do nothing, and all notes except 60-63/67 are ignored entirely. The C++ comment is also internally wrong: it labels 60-63 as "C3 D3 E3 F3", but 61-63 are C#3/D3/D#3 — whoever edits this next will "fix" the wrong side.

**Failure scenario:** User enables MIDI mode, plays the notes the tooltip names, nothing triggers; concludes the plugin is broken.

**Fix:** Pick one spec. Either (a) implement the tooltip (notes 24-27 → lanes, others → all-lanes fallback), or (b) correct the tooltip and comments to the actual notes (60-63 + 67). Given the tooltip's "any other note = all lanes" is more forgiving, (a) is the better UX.

### WR-07: TriggerRouter keeps only one trigger per block — simultaneous lane triggers are dropped

**File:** `Source/DSP/TriggerRouter.cpp:39-68`, `TriggerRouter.h:37`

`midiTriggeredLane` is a single `int` overwritten by each matching note-on in the buffer. Playing a chord (e.g. notes 60+62 to fire lanes 1 and 3 together — the natural gesture for this instrument) fires only the **last** note in the buffer; the others are silently discarded. Triggers are also block-quantized (the sample offset in `metadata.samplePosition` is ignored), adding up to one buffer of jitter.

**Fix:** Replace the single int with a 5-bit mask (`uint8` lane bits + all-lanes bit), OR-in every matching note, and have the processor trigger every set lane. Sample-accurate triggering (splitting the block at trigger offsets) is a larger change and can be deferred, but the mask fix is small.

### WR-08: Factory presets don't specify pattern-step / pitch-rand / Euclidean parameters — loading a preset inherits stale state

**File:** `Source/PluginProcessor.cpp:854-919` (`makeParams` — no `pattern_*`, no `*_pitch_rand_*`; presets 1-12 also lack `*_euclidean_*`); `Source/OPolystutterPresetManager.h:269-282` (`applyPresetJson` only sets keys present in the JSON)

`applyPresetJson` applies only the parameters listed in the preset file. The 12 non-Euclidean factory presets omit 64 pattern steps, 16 pitch-rand params, and 12 Euclidean params. If the user has toggled pattern steps off or enabled Euclidean/pitch-rand on a lane, then loads "Classic Stutter", the leftover state persists and the preset audibly does not sound as authored (e.g. Euclidean stays on and gates the pattern).

**Fix:** Either have `applyPresetJson` reset all parameters to defaults before applying the preset's keys, or make `makeParams` emit the complete parameter set (defaults included). The former fixes user presets saved by older versions too.

---

## IMPROVEMENT

### IN-01: "Retrigger crossfade" blends against its own previous output, and fade-out holds a frozen sample

**File:** `Source/DSP/RepeatLane.cpp:260-273` and `RepeatLane.cpp:103-118`

`lastOutputLeft/Right` are updated *after* the blend each sample (lines 272-273), so during the retrigger crossfade the "old" signal is the previous **blended** sample, not the old stream — the math degenerates to a recursive smoother, not a crossfade (the old stream no longer exists, so this is a reasonable approximation, but the comment overclaims). Similarly the post-repeat fade-out (lines 107-118) fades a **frozen single sample value** (a decaying DC step), not the tail of the audio. Both are click-safe but crude; if fidelity matters, keep a short history buffer of real output for the fade source. At minimum, correct the comments.

### IN-02: Loop-boundary interpolation can read one sample of just-written live input

**File:** `Source/DSP/RepeatLane.cpp:182-190`

`nextPos = (basePos + 1) % maxCaptureSamples` at `readOffset ≈ captureLength − 1` lands on the slot the live write head refilled earlier in the same block, so the interpolated last sample of each repeat mixes in one sample of unrelated live audio. Clamp `nextPos` inside the capture region (or wrap to `captureStartPosition` for loop-consistent interpolation).

### IN-03: Dead code in processor: `subdivisionSamples`, `samplesSinceLastBeat`, duplicated subdivision math

**File:** `Source/PluginProcessor.h:87-88`, `PluginProcessor.cpp:1160-1164, 1807-1823`

`samplesSinceLastBeat` and the processor-level `subdivisionSamples` are written in `prepareToPlay` and never read. `getSubdivisionSamples()` duplicates `RepeatLane::calculateSubdivisionSamples()` line-for-line, and `getSubdivisionPPQ` (lambda in `updateBeatSync`) duplicates `RepeatLane::updatePatternPosition`'s switch. Remove the dead members and centralize the subdivision table in one place — three copies of the same table is how the next subdivision addition gets missed in one of them.

### IN-04: Hiss is broadband (0-15 kHz), not the documented 5-15 kHz band; highpass coefficients computed and discarded

**File:** `Source/DSP/TapeDegrader.cpp:342-361`

`updateHissBandpass` designs a 5 kHz highpass (`coefficientsHigh`) and then throws it away ("use lowpass only"), so the noise fed to both channels is white noise lowpassed at 15 kHz — including low-frequency energy that reads as rumble/hash, not tape hiss. Also the *same* noise sample drives both channels (line 169-173), so the hiss is mono/correlated, which sounds unnatural on headphones. Chain the highpass through a second filter pair and use independent noise samples per channel.

### IN-05: No NaN/Inf guard — a NaN input latches the tape IIR filters permanently

**File:** `Source/DSP/TapeDegrader.cpp:193-208` (rolloff), `166-180` (hiss); `Source/DSP/RepeatLane.cpp:75-86`

A single NaN sample from upstream propagates into `rolloffFilterLeft/Right` state and stays there (IIR feedback) until the next `prepare()` — permanent silence/NaN output. The capture buffer will also loop NaN for the full repeat tail. Per the suite pattern (O-Formant v1.25.2): sanitize block input (or at least flush filter state when a non-finite output is detected), and remember the guard must reset *all* state, not just the filters.

### IN-06: Factory-preset initialization does file I/O in the processor constructor

**File:** `Source/PluginProcessor.cpp:849-1121`

The constructor calls `findChildFiles()` and, on first run, writes 15 JSON files. This runs during host plugin scans and `auval` — exactly what the preset manager's own comment (`OPolystutterPresetManager.h:201-203`, "deferred to avoid file I/O during AU validation") tries to avoid. Move factory-preset seeding to first editor open or a lazy check in `getPresetList()`.

### IN-07: Preset name used verbatim as filename — "/" in a name silently fails (known suite bug)

**File:** `Source/OPolystutterPresetManager.h:308` (`getChildFile(presetName + ".json")`)

Same as the O-simplePhysicalModelSynth finding: a name containing "/" becomes a nested path; the write fails or lands in a subdirectory invisible to the non-recursive `getPresetList()`. Exposure here is reduced because the save UI goes through a native file dialog (name = `getFileNameWithoutExtension`), but the `savePreset` native fn still accepts arbitrary strings from JS. Sanitize the name (`removeCharacters("/\\:")` or `File::createLegalFileName`).

### IN-08: Preset dropdown injects preset names via unescaped innerHTML

**File:** `Source/ui/public/index.html:1608-1611`

`item.innerHTML = `<span ...>${name}</span>...`` — a preset file named `<img src=x onerror=...>.json` (user-writable directory) executes script in the plugin UI, and any name containing `<`/`&` breaks rendering. Use `textContent` for the name span and build the badge/delete elements with `createElement`.

### IN-09: `confirm()` used for preset deletion — typically a no-op in JUCE WKWebView

**File:** `index.html:1617`, `modules/preset-manager.js:316-320`

WKWebView requires a host-provided JS dialog delegate for `confirm()`; JUCE's WebBrowserComponent does not supply one, so `confirm()` returns falsy and the delete button silently does nothing on macOS (the module's own docstring warns about this). Replace with an in-DOM confirmation (two-click "Delete? ✓" pattern used elsewhere in the suite).

### IN-10: Concurrent `updateDropdownMenu()` invocations can interleave

**File:** `index.html:1596-1628`

The function `await`s `isFactoryPreset` inside the item loop after clearing `innerHTML`. It is invoked from both `onPresetChanged` and `onPresetListUpdated` (which fire back-to-back on every load/save), so two in-flight invocations can interleave and append duplicate items. Guard with a generation counter or build the list off-DOM and swap once. Also `isPercentage` in `parameter-bindings.js:128` is computed and never used — delete it.

### IN-11: WebView2 user-data folder set to the bare shared temp directory

**File:** `Source/PluginEditor.cpp:207-210`

`withUserDataFolder(tempDirectory)` points at the shared temp root rather than a plugin-scoped subfolder (`tempDirectory.getChildFile("OPolystutter_WebView")` per the suite pattern). Different plugins sharing one WebView2 profile directory can hit browser-data lock contention on Windows.

---

## NOTE

### NT-01: CMake Windows WebView2 config is correct

`CMakeLists.txt:15` has `NEEDS_WEBVIEW2 TRUE` and line 83 has `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` — this plugin is one of the few in the suite (per the 2026-02-06 audit) already carrying both required Windows flags. No action needed.

### NT-02: WebView bridge audit is clean

All 10 `getNativeFunction` names used by JS (`savePreset`, `savePresetWithDialog`, `loadPreset`, `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset` — preset-manager.js:83-92) have matching `withNativeFunction` registrations (PluginEditor.cpp:391-493), and no registered function is unused. Relay/attachment/param ID triples (128 params) are consistent.

### NT-03: Default mix (50% dry) means −6 dB pass-through when lanes are idle

With `mix_dry` defaulting to 50% (PluginProcessor.cpp:633-639) and lanes outputting silence when not repeating, the plugin at rest (transport stopped, or MIDI mode with no notes) passes audio at half level relative to bypass. Intentional mix design, but worth knowing when users report "the plugin makes everything quieter."

### NT-04: Latency reporting is correctly absent

The signal path has no lookahead; zero latency is correct, and the code rightly avoids trying to override the non-virtual `getLatencySamples()` (JUCE 8).

### NT-05: Triggers are block-quantized

Beat-sync, MIDI, and manual triggers all fire at block boundaries (trigger decisions are made once per `processBlock` before the sample loop), giving up to one buffer (~10 ms at 512/48 k) of timing jitter on stutter onsets. Acceptable for this design; sample-accurate splitting would be the upgrade path.

### NT-06: Preset directory comment mismatch

`OPolystutterPresetManager.h:23-24` documents `~/Library/Application Support/{plugin}/Presets/` but `getPresetsDirectory()` (line 205-212) actually builds `~/Library/{plugin}/Presets/`. Behavior is consistent with the rest of the suite; fix the comment (or the path) so future maintenance doesn't "correct" the wrong one.

---

*Review depth: standard+ (full read of all in-scope files, cross-file trace of parameter → DSP consumption and JS → native-function bridge).*
*No source files were modified by this review.*
