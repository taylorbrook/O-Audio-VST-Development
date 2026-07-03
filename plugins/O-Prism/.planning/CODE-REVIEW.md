---
phase: O-Prism code review
reviewed: 2026-07-02T00:00:00Z
depth: deep
files_reviewed: 57
files_reviewed_list: all Source/*.cpp|h, Source/dsp/*.cpp|h, ui/public/index.html, ui/public/js/wavetable-editor.js, CMakeLists.txt
findings:
  critical: 10
  warning: 20
  info: 17
  total: 47
status: resolved
resolution: |
  All 47 findings dispositioned across three batches:
  v1.18.2 (CR-01/02/03/08/09), v1.19.0 (CR-04..07, CR-10, WR-01..20),
  v1.19.1 (IN-01..17; IN-08 was a reviewer error — PRESET_ZARLINO and
  PRESET_JUST_INTONATION already differ at degree 10, 9/5 vs 16/9).
---

# O-Prism: Code Review Report

**Reviewed:** 2026-07-02 (v1.18.1, 📦 Installed)
**Depth:** deep (3 parallel domain reviews: RT audio core / tuning-preset-data / editor-UI-build)
**Files Reviewed:** 57 (~17.5k lines)
**Status:** issues_found

## Summary

O-Prism's architecture shows real RT awareness in places — cached FX parameter pointers, lock-free tuning frequency table, cached mod-matrix slot atomics, `ScopedNoDenormals` — but the discipline is inconsistent, leaving a host-crash-capable out-of-bounds wavetable read, three wavetable-lifetime UAF paths, and three per-block RT violations (heap alloc / mutex / String building). Several known suite-wide bugs reproduce here unpatched: the launchAsync SafePointer UAF (all 6 file choosers), `applyPresetJson` without reset-to-defaults, and "/" preset names. Beyond safety there is a striking cluster of silently dead user-facing features (tempo-synced delay + 20 factory presets that depend on it, 9 mod destinations, Phase knobs, glide modes, the macOS preset Save button) — the parameter surface outgrew the DSP/UI implementation without a reconciliation pass.

Verified **clean** (known suite patterns handled correctly): the O-Formant embedded-tuning period bug (`loadEmbeddedTuning` pushes `tuning->period` before `setCustomIntervals`, PluginEditor.cpp:436-437); resource provider bare-path handling; relay/WebView/attachment ordering and destruction order; full Windows WebView2 config (`NEEDS_WEBVIEW2 TRUE` + static-linking flag + `withUserDataFolder`); single BinaryData target; dev-suffix branding; editor timer stopped in destructor; JS global-listener cleanup; `.scl` round-trip and ScaleGenerator wrap loops.

---

## Critical

### CR-01: WavetableOscillator phase accumulator unbounded when phaseIncrement ≥ 1.0 → out-of-bounds heap read (crash)
**File:** `Source/dsp/WavetableOscillator.cpp:236-238` (non-sync wrap), `:220-228` (sync re-seed), `:120-162` (`readSample` — no clamp)

`setFrequency` computes `phaseIncrement = freq / sampleRate` with no upper bound, and the render wrap subtracts 1.0 **exactly once**. When `freq > sampleRate` the accumulator grows without bound; `readSample()` never wraps its `phase` arg, so `samplePos = phase * 2048` walks past the frame, past adjacent frames, and off the end of `WavetableData::data` — UB/crash. The Bend warp (`pow(phase, 1..4)`) accelerates it; the Sync path re-seeds the slave with `masterPhases[i] * syncRatio`, which exceeds 1.0 whenever `masterInc > 0.25`.

**Failure:** fs 44.1k, MIDI 127 + `oscACoarse +24` → increment ≈ 1.14; reachable from note ~96 with fine/unison/pitch-mod. One held note ⇒ host crash.
**Fix:** wrap with `phase -= std::floor(phase)` in both advance paths and after the sync re-seed; defensively wrap in `readSample`; optionally clamp `setFrequency` to `0.5 * sampleRate`. Apply to the (currently dead) mono path too if kept (IN-08).

### CR-02: Wavetable use-after-free — editor close and user-table delete free buffers the audio thread still reads
**File:** `Source/PluginProcessor.cpp:889-922` (`stopEditing` → `clearWorkingTable()`), `Source/PluginEditor.cpp:661-664` (delete flow), `Source/PluginProcessor.cpp:774-794` (`updateWavetableAssignments`)

Voices hold raw `const WavetableData*`, refreshed only at the top of the *next* `processBlock`. `stopEditing()` frees the working table via `unique_ptr::reset()` immediately after swapping pointers; `deleteWavetable(name)` frees the entry while voices may still read it this block. Secondary: during an active edit session, `setFrameHarmonics` rewrites the working table in place while the audio thread reads it (torn floats — audible garbage, non-crashing).

**Failure:** close the wavetable editor (or delete a user table) while a pad chord sustains → freed-memory read on the RT thread. Same class as O-MicrotonalSampler v1.23.2.
**Fix:** never free a table the audio thread may see — generation-counter handshake or `shared_ptr` publish + message-thread reaper (`pattern_retired_map_reaper_rt_free`).

### CR-03: `saveAsUserWavetable` → `loadFromDisk()` frees every user wavetable — permanent dangling pointer on the other oscillator
**File:** `Source/dsp/WavetableEditor.cpp:402`, `Source/dsp/UserWavetableManager.cpp:25` (`entries.clear()`), `Source/PluginProcessor.cpp:837-846` (`resolveActiveTable`)

Saving an edited wavetable ends with `manager.loadFromDisk()` — `entries.clear()` destroys **all** existing `WavetableData` objects and re-imports fresh ones. Nothing re-syncs `userTablePtrA/B`.

**Failure:** Osc B uses user table "Alpha"; user saves an edit on Osc A → "Alpha"'s object is destroyed; `userTablePtrB` dangles **permanently** — every subsequent block dereferences freed heap until the user manually reselects.
**Fix:** after reload, re-resolve both osc pointers by name (mirror `stopEditing`'s logic); better, insert/replace only the saved entry and retire replaced tables via the reaper pattern.

### CR-04: EQProcessor heap-allocates IIR coefficients on the audio thread every block
**File:** `Source/dsp/EQProcessor.cpp:56-75`; call site `Source/PluginProcessor.cpp:728-736`

`IIR::Coefficients::makeLowShelf/makePeakFilter/makeHighShelf` each `new` a ref-counted object; the four setters run unconditionally **every block** whenever `eqBypass` is off (the default) — 4 allocs + 4 frees per callback even at 0 dB. Exactly the O-Formant v1.25.1 WR-08 pattern.
**Fix:** `ArrayCoefficients<float>::makeXXX` assigned in place into `*state`, plus change-detection to skip the common no-change case.

### CR-05: TuningEngine mutated from processBlock — mutex (×128) + heap alloc on the audio thread; message-thread getters read shared Strings without the lock (UB)
**File:** `Source/PluginProcessor.cpp:630-649`; `Source/TuningEngine.cpp:82-90, 115-187, 240-266, 316-324, 763, 884-901`

`processBlock` calls `setMasterTune/setOctaveStretch/setPitchBendRange` every block and `setBuiltInPreset/setTonicNote` on change. On change: `setBuiltInPreset` builds a `std::vector<double>` and a `juce::String` (heap) on the RT thread; `setCustomIntervals` takes `intervalMutex` — the same mutex held by message-thread `loadScalaFile`/`getIntervals` (priority inversion); `rebuildFrequencyTable` re-acquires the mutex **per MIDI note** (128 lock cycles per rebuild). Meanwhile `getActiveTuningName()` reads `scaleName` (ref-counted String) with **no lock** while the audio thread reassigns it — as do `getTemperamentPreset`/`getMasterTune` native fns for `currentPreset`/`a4Frequency`.

**Failure:** automate `tuningPreset` while the tuning panel polls `getTuningName` → concurrent String write/read → intermittent crash; or a Scala load holds the mutex when a block starts → dropout.
**Fix:** `processBlock` only detects changes and defers engine mutation to the message thread (`triggerAsyncUpdate`), keeping the lock-free `frequencyTable` atomics as the sole audio-thread interface; lock (or make atomic) the message-thread getters.

### CR-06: `advanceGlobalLfoPhases` builds ~28 `juce::String`s + 12 APVTS map lookups every block on the audio thread
**File:** `Source/PluginProcessor.cpp:543-577` (runs every block at `:674`)

`juce::String(i+1)` + two `operator+` per parameter, 3 params × 4 LFOs — juce::String always heap-allocates. The rest of the codebase caches these pointers; this function was missed.
**Fix:** cache `pLfoSync/pLfoRate/pLfoDiv[4]` as `std::atomic<float>*` members in the constructor alongside the FX pointer block (`:510-534`).

### CR-07: `.kbm` reference frequency clamped through the 400–480 Hz A4 master-tune clamp — middle-C-referenced KBMs mistune the whole instrument ~7 semitones
**File:** `Source/TuningEngine.cpp:577-581` (routes ref freq via `setMasterTune`), `:84` (`jlimit(400.0, 480.0, ...)`)

A standard KBM with reference note 60 / 261.63 Hz (Scala's own examples) clamps to 400 Hz — everything ~7.3 semitones sharp, silently, with `loadKBMFile` returning `true`.
**Fix:** store the KBM reference frequency in its own member (validate 8–20000 Hz) and use it as `refFreq` in the KBM branch of `calculateCustomFrequency`; only fall back to `a4Frequency` when no KBM is loaded.

### CR-08: WAV save opens `FileOutputStream` on an existing file — JUCE appends, so overwriting a wavetable silently discards the user's edits
**File:** `Source/dsp/WavetableEditor.cpp:384-396`, `Source/dsp/UserWavetableManager.cpp:136-148`

`FileOutputStream` positions at end-of-file; re-saving under an existing name appends a second WAV after the old one. Next `loadFromDisk()` parses the **original** header at offset 0 — the edit is silently lost and the file grows every save. (`makeUniqueName` only checks in-memory entries, not disk.)
**Fix:** `destFile.deleteFile()` before constructing the stream (or `juce::TemporaryFile` + `overwriteTargetFileWithTemporary()`), in both save paths.

### CR-09: All 6 `FileChooser::launchAsync` completions capture raw `this` — UAF on editor teardown (known suite bug, unpatched)
**File:** `Source/PluginEditor.cpp:279` (loadScalaFile), `:307` (loadKBMFile), `:327` (saveScalaFile), `:352` (saveKBMFile), `:537` (exportTuningHTML), `:576` (importUserWavetable)

Every completion captures `this` and the WebView-owned `complete` with no lifetime guard; the cancel paths (`:291-294, 311-312, 335-338, 359-362, 546-549, 588`) all call `complete(...)`, which is itself a UAF after teardown. Exactly `pattern_webview_launchasync_safepointer_no_complete` (O-MicrotonalSampler v1.23.5 W12), whose note says "audit all WebView editors".
**Fix:** capture `juce::Component::SafePointer<OPrismAudioProcessorEditor>`; on null, bail with a **bare `return`** (never `complete(false)` on the dead path). Apply to all six sites.

### CR-10: Preset Save button is dead on macOS — `window.prompt()` always returns null under WKWebView
**File:** `Source/ui/public/index.html:1785`

JUCE 8.0.9's WKWebView backend implements no `runJavaScriptTextInputPanel` UI delegate (verified against the local JUCE tree), so `window.prompt` behaves as Cancel: returns `null` instantly, no dialog. The only preset-save path silently no-ops on the primary platform (Windows WebView2 does render prompts, masking it in cross-platform testing).
**Fix:** replace with an in-DOM modal — reuse the existing wavetable-editor save modal pattern (`wt-save-modal-overlay`, index.html:1544-1554, wavetable-editor.js:413-445).

---

## Warning

### WR-01: SVF cutoff not clamped below Nyquist — unstable filter and sticky NaN at fs < 40 kHz
**File:** `Source/dsp/SVFFilter.cpp:69-78`
Cutoff clamps to [20, 20000] but never to `fs/2`. At 32 kHz with the **default** 20 kHz cutoff, `g = tan(π·20000/32000) < 0` → TPT integrator blows up to NaN; the delay's recirculating feedback filter (`DelayProcessor.cpp:100-101`) and the reverb tank then hold NaN **permanently** (voice resets don't clear FX buffers). Fix: `fc = std::min(fc, 0.49 * sampleRate)` + last-known-good/NaN-flush per the O-Formant v1.25.2 pattern.

### WR-02: Nine mod-matrix destinations selectable but never applied — silent no-ops
**File:** `Source/dsp/ModulationMatrix.h:47-59`; consumer `Source/PrismVoice.cpp:465-620`
`LFO1..4Rate`, `OscA/BDetune`, `Reverb/Delay/Chorus/DistMix`, `MasterVol` are offered in every slot's Dest choice, but `getModOffset` is never called for any of them (grep-verified). Implement or remove from `getModDestNames()`; if removing, keep enum order stable for preset compat.

### WR-03: Delay tempo-sync is dead — `delaySync` never read, `delayDivision` param doesn't exist, ~20 factory presets silently unsynced
**File:** `Source/PluginProcessor.cpp:267-268, 702-711`; `Source/dsp/DelayProcessor.cpp:58-71`, `.h:43-44`; `FactoryPresets.cpp:966, 1343, 1395, …`
The UI toggle works but the delay lambda never reads `delaySync`; `setSync` is never called; `tempoSync`/`audioPlayHead` are unused. 20 factory presets set `delaySync: 1` + a `delayDivision` value **that has no parameter in the layout** — every "synced delay" preset free-runs at 0.375 s. Fix: add `delayDivision`, wire BPM math (mirror `kDivBeats`), or strip toggle + preset fields.

### WR-04: `oscAPhase`/`oscBPhase` params and knobs are dead — never read by the DSP
**File:** `Source/PluginProcessor.cpp:68-70`; `Source/PrismVoice.cpp:221, 238`; UI `index.html:991, 1045, 2035, 2048`
`startNote` unconditionally uses `resetWithRandomPhases()`; nothing reads the Phase params. `resetWithPhase()` exists and is unused. Wire it (phase > 0 → deterministic start, optional unison spread) or remove the knobs.

### WR-05: Hard-coded delay-line capacities clamp at high sample rates
**File:** `Source/dsp/DelayProcessor.h:31-32` (`{192000}`), `Source/dsp/ReverbProcessor.h:147-148` (`{19200}`)
At 192 kHz, 2.0 s delay needs 384000 samples (max 192000) and 200 ms predelay needs 38400 (max 19200) — jassert in debug, silent clamp in release. Size in `prepare()` from `spec.sampleRate`.

### WR-06: Glide "Legato" and "Always" are identical, and glide almost never engages polyphonically
**File:** `Source/dsp/GlideProcessor.h:30-37, 47-57`; `Source/PrismVoice.cpp:160, 200-205`
Modes 1 and 2 share every code path, and `setTarget` snaps unless the voice was stolen — with 16 voices, fresh notes land on idle voices, so glide is inaudible except under stealing pressure. Seed "Always" from a processor-level last-played-frequency atomic; gate "Legato" on the existing `noteStates` any-note-held.

### WR-07: FX mix≈0 process gate leaves stale delay/reverb/chorus buffers that replay when mix rises
**File:** `Source/PluginProcessor.cpp:686-738`
Skipping `fx.process()` at mix ≤ 0.001 freezes buffered audio; automating delay mix 0 → 0.5 replays a "ghost echo" from arbitrarily long ago. Call `fx.reset()` once on the mix zero-crossing transition.

### WR-08: `applyPresetJson` doesn't reset params to defaults first; factory preset JSON never regenerated after first run
**File:** `Source/OuariconPresetManager.h:197-226`; `Source/PluginProcessor.cpp:479-481`
Known suite bug (O-Polystutter v1.12.3 WR-08). Today's factory presets are complete snapshots, but old-version user presets — and on-disk factory JSON after any future param addition (`factoryPresetsExist()` short-circuits regeneration) — silently inherit stale state. Reset all non-excluded params to defaults before applying; version-stamp and regenerate factory sets.

### WR-09: Preset names used verbatim as file paths — "/" silently breaks save/list; "../" escapes the preset dir
**File:** `Source/OuariconPresetManager.h:240, 360`; `Source/PluginEditor.cpp:954-962`
Known suite bug (O-simplePhysicalModelSynth). Sanitize with `juce::File::createLegalFileName()` in `savePreset`/`deletePreset`, reject empty/`..`.

### WR-10: User wavetable names from the WebView are unsanitized file paths — arbitrary `.wav` write/delete outside the wavetable dir
**File:** `Source/dsp/WavetableEditor.cpp:382`; `Source/dsp/UserWavetableManager.cpp:64-88, 96`; names arrive from JS at `PluginEditor.cpp:603, 875-878`
`saveEditedWavetable("../../Desktop/x")` writes to the Desktop; `deleteUserWavetable("../…")` is a relative-path deletion primitive. Sanitize + verify `isAChildOf(wavetableDir)` before delete.

### WR-11: `.scl` parser breaks on blank description lines (valid per spec) and silently drops negative-cents pitches
**File:** `Source/TuningEngine.cpp:443-479, 391-421`
Blank-line skipping consumes the count line as description and the first pitch as the count; `parseScalaPitch` returns −1 for both errors and legal negative cents (skipped at `:471`); `expectedDegrees == 0` truncates to 2 entries yet "succeeds". Track line position; use an optional/NaN sentinel; fail on count mismatch.

### WR-12: `loadKBMFile` trusts the header-claimed map size — unbounded allocation from a hostile file
**File:** `Source/TuningEngine.cpp:530, 546-562`
A `.kbm` claiming size 2000000000 attempts an ~8 GB vector. Clamp to ≤128 (or 1024) and fail beyond.

### WR-13: `WavetableImporter` trusts audio headers — int overflow in the 30 s cap and unclamped channel count allow multi-GB allocations / negative buffer sizes
**File:** `Source/dsp/WavetableImporter.cpp:31-40, 60-69`
`(int)jmin(lengthInSamples, (int64)(sampleRate * 30))` wraps on hostile headers; `numChannels` up to 65535 multiplies the allocation. Clamp channels to 2 and compute the cap in int64 with an absolute ceiling; reject ≤ 0.

### WR-14: `setSingleInterval` wipes any legitimate 11-degree scale (12 interval entries) to 12-TET
**File:** `Source/TuningEngine.cpp:276-284`
The `scaleIntervals.size() == 12` guard can't distinguish a period-less 12-degree list from a valid 11-note scale — load an 11-note `.scl`, nudge one interval in the UI, lose the whole scale. Remove the `== 12` clause; detect period-less arrays explicitly at ingestion.

### WR-15: `TuningExporter::toHTML` period detection ignores periods ≤ 1200 cents — Carlos Gamma exports with wrong period/deviations/pitch circle
**File:** `Source/TuningExporter.cpp:373-382`
`period` only takes `intervals.back()` when `> 1200.0`; Carlos Gamma (737.1¢, an embedded tuning) is documented as 1200¢ and every derived figure is wrong. Fix: use `intervals.back()` whenever `> 0.0`.

### WR-16: `setMasterTune`/`setOctaveStretch` native fns silently overridden by the per-block APVTS→engine sync
**File:** `Source/PluginEditor.cpp:219-228, 236-245` vs `Source/PluginProcessor.cpp:630-631`
Unlike `setTonicNote`/`setTemperamentPreset`, these don't write the APVTS param, so any call is reverted within one block and never persists. Mirror the `setValueNotifyingHost` pattern or delete the fns (relays are the live path).

### WR-17: Session state omits KBM mapping, engine mode, and current preset — `.kbm` silently lost on reload; `getTemperamentPreset` reports 12-TET after every reload
**File:** `Source/PluginProcessor.cpp:928-1003`
Restore always routes through `setCustomIntervals` (forces `Mode::Scala`); KBM state isn't saved at all. Persist `mode`, `currentPreset`, and the KBM block; add a proper `restoreState` entry point on the engine.

### WR-18: Redo shortcut (Ctrl/Cmd+Shift+Z) can never fire — `e.key` is `'Z'` when Shift is held
**File:** `Source/ui/public/js/wavetable-editor.js:592-598`
`e.key === 'z' && e.shiftKey` is unreachable. Compare `e.key.toLowerCase()`.

### WR-19: Preset load never refreshes native-fn-backed tuning UI — `__refreshAllControls` undefined, `refreshTuningState` has zero callers
**File:** `Source/ui/public/index.html:1680-1682, 1728-1730, 3401-3404`; relay listeners `:2357-2358` are empty
Loading a 19-EDO preset retunes the synth but the Tuning tab keeps showing the previous scale's intervals/name/circle — edits then target the wrong degree. Wire the guarded call sites and the `tuningPreset`/`tonic` relay listeners to `refreshTuningState()`.

### WR-20: Mod-matrix source/dest names hard-coded in JS while `getModSourceNames`/`getModDestNames` go uncalled — silent misrouting on enum drift
**File:** `Source/ui/public/index.html:2218-2223, 2318, 2328`; `Source/PluginEditor.cpp:722-736`
Dropdown→param mapping normalizes by array **length**; any future C++ add/reorder silently routes modulation to the wrong destination. Populate from the already-registered native fns at startup (same hazard: `factoryCount = 28` at index.html:2487 vs `getActiveOscInfo.numTables`).

---

## Info

### IN-01: Uncached per-block APVTS string lookups in processBlock
`Source/PluginProcessor.cpp:630-637, 742-743, 843-845` — masterTune/octaveStretch/pitchBendRange/tuningPreset/tonic/stereoWidth/masterVol/oscA-BTable looked up by string every block while the 25 FX params are cached. Cache alongside the FX pointer block.

### IN-02: Dead code — `WavetableOscillator::getNextSample()` (mono) and `resetWithPhase()` unused
`Source/dsp/WavetableOscillator.cpp:186-243, 67-74` — the mono path duplicates sync/warp logic (any CR-01 fix must hit both or delete it); `resetWithPhase` is the natural WR-04 implementation.

### IN-03: Delay time changes unsmoothed — zipper/clicks under automation
`Source/dsp/DelayProcessor.cpp:43-46, 97-98` — smooth `delaySamples` per sample or crossfade taps.

### IN-04: Latency reported unconditionally even when distortion (its only source) is bypassed
`Source/PluginProcessor.cpp:605`; `Source/dsp/DistortionProcessor.h:27` — truncated fractional latency stays reported while `distBypass` skips the oversampler.

### IN-05: `kDivBeats` table duplicated in processor and voice
`Source/PluginProcessor.cpp:546-550`; `Source/PrismVoice.cpp:360-364` — move to a shared header; divergence silently desyncs global vs voice LFO rates.

### IN-06: `subOsc.reset()`/`noiseGen.reset()` unconditional in startNote — click on legato/glide retrigger
`Source/PrismVoice.cpp:248, 253` — gate with the same `(!wasActive || glideMode == 0)` condition the main oscillators use.

### IN-07: Built-in Bohlen-Pierce preset omits the 13th degree
`Source/TuningEngine.cpp:52-55, 173-174` — inconsistent with the embedded `nonoctave/bohlenpierceET` (correct, all 13); the top step is a double-jump.

### IN-08: `PRESET_ZARLINO` and `PRESET_JUST_INTONATION` are byte-identical
`Source/TuningEngine.cpp:24-26 vs 48-50` — two menu entries, one tuning. Differentiate or drop one.

### IN-09: `isNoteMapped` reads KBM state without `intervalMutex`
`Source/TuningEngine.cpp:648-671` — currently uncalled (module API surface); first caller inherits a data race.

### IN-10: `generateRank2` clamps the generator against the unclamped period
`Source/ScaleGenerator.cpp:59-60` — swap the two clamp lines.

### IN-11: KBM formal octave degree parsed but never used in frequency math; exporter pitch-circle double-draws the unison
`Source/TuningEngine.cpp:536, 572`; `Source/TuningExporter.cpp:310-350` — spec-compliance gap (rare) + cosmetic SVG overlap.

### IN-12: `smoothFrames` strength semantics appear inverted
`Source/dsp/WavetableEditor.cpp:325` — strength 0.0 = max smoothing, 1.0 = no-op; verify against the JS slider meaning (likely needs `1 − strength`).

### IN-13: Eleven registered native functions never called from JS — dead bridge code
`Source/PluginEditor.cpp:154, 214, 219, 236, 248, 264, 420, 469, 486, 507, 964` — no JS→C++ gaps exist (no dead controls from that direction), but these registrations are unreachable; the tuning-library category dropdown hard-codes what `getEmbeddedTuningCategories` provides.

### IN-14: `applyGeneratedScale` ignores the scale-name argument — name reverts to "Generated" on reopen
`Source/PluginEditor.cpp:449-466`; `index.html:3382-3385` — read `args[1]` and pass it to `setCustomIntervals`.

### IN-15: `webview-relay-manager` module linked in CMake but never included by any source
`CMakeLists.txt:81` — adopt the module or `/module-remove O-Prism webview-relay-manager`.

### IN-16: Hand-built JSON unescaped; `toJsonFloatArray` emits invalid JSON for non-finite floats — JS `JSON.parse` throws, waveform display silently freezes
`Source/PluginEditor.cpp:119-129, 410-417, 496-499, 693-694, 725-735` — clamp non-finite to 0.0f; route dev-controlled string fields through `juce::JSON::toString`.

### IN-17: Tonic-change bridge failures swallowed by empty catch blocks — silent UI/DSP desync
`Source/ui/public/index.html:3069, 3113, 3122` — log and revert `currentTonic`/display on failure.

---

## Suggested fix order

1. **Crash/UAF batch (CR-01, CR-02, CR-03, CR-09):** phase wrapping + a single wavetable retire/reaper mechanism covers CR-02+CR-03; SafePointer sweep across all six choosers is mechanical.
2. **RT-violation batch (CR-04, CR-05, CR-06):** ArrayCoefficients for EQ, defer TuningEngine mutation to the message thread, cache LFO param pointers — all three are the established suite patterns.
3. **Data-loss/correctness batch (CR-07, CR-08, WR-11–WR-14):** KBM ref-freq member, delete-before-save, importer clamps.
4. **Dead-feature reconciliation (CR-10, WR-02–WR-04, WR-06, WR-18–WR-20):** decide ship-or-strip per feature; CR-10 (macOS preset save) is the most user-visible.
