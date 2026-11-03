# Module convergence progress (261029-md9 FORK-REVIEW)

The only state shared between fresh `/converge-modules` contexts. Step 6 copies are intentional forks and never appear here.

## Step 1: upstream into scala-tuning-engine 3.3.0

Module work, done as one unit. Plugins unchanged. Each box is a source copy whose listed fixes are now in the module.

- [x] O-Bells `ScaleGenerator.cpp` (fork+ahead) — 55763211
- [x] O-Bells `TuningEngine.cpp` (fork+ahead) — 55763211
- [x] O-Bells `TuningExporter.cpp` (fork+ahead) — 55763211
- [x] O-Formant `EmbeddedTunings.cpp` (fork+ahead) — 55763211
- [x] O-Formant `ScaleGenerator.cpp` (fork+ahead) — 55763211
- [x] O-Formant `TuningEngine.cpp` (fork+ahead) — 55763211
- [x] O-Formant `TuningEngine.h` (fork+ahead) — 55763211
- [x] O-Formant `TuningExporter.cpp` (fork+ahead) — 55763211
- [x] O-Prism `ScaleGenerator.cpp` (fork+ahead) — 55763211
- [x] O-Prism `TuningEngine.cpp` (fork+ahead) — 55763211
- [x] O-Prism `TuningEngine.h` (fork+ahead) — 55763211
- [x] O-Prism `TuningExporter.cpp` (fork+ahead) — 55763211
- [x] O-Strata `ScaleGenerator.cpp` (fork+ahead) — 55763211
- [x] O-Strata `TuningEngine.cpp` (fork+ahead) — 55763211
- [x] O-Strata `TuningEngine.h` (fork+ahead) — 55763211
- [x] O-Strata `TuningExporter.cpp` (fork+ahead) — 55763211

Gate: `tests/scala_tuning_tests.cpp` (34 checks, 0 failed; 22 of 32 fail on 3.2.0) and `tests/parity_gate.py` (Prism 17/17, Strata 17/17, Formant 23/23, Bells 3/3, 3.2.0 default paths 20/20, 2 accepted divergences). All 6 consumers build; O-Bowed default render bit-identical to pristine HEAD (`b7b1740f`), and its 60 @ 261.6256 .kbm gives referencePitch 440.0, note 60 at 261.6256 Hz, reopen render identical.

## Step 2: preset-manager drift

O-DigiDelay needed bypass-param exclusion first (FORK-REVIEW caveat); done in 1.9.4 with `setExcludedParameters`, plus the IN-07 apply upstreamed as module 1.1.1.

- [x] O-Bass `OuariconPresetManager.h` (drift) — 687a182a
- [x] O-Bass `ui/public/modules/preset-manager.js` (drift) — 687a182a
  O-Bass 1.8.4 gate: `tests/preset-gate/gate.py` 187 preset and state lines identical to 1.8.3, 10 factory files match; sentinel rewrites 0 files at the same version and 10 when stale. Old-vs-new renders bit-identical (3 settings). auval and pluginval 5 pass.
- [x] O-Comp `OuariconPresetManager.h` (drift) — cfec3132
- [x] O-Comp `ui/public/modules/preset-manager.js` (drift) — cfec3132
  O-Comp 1.11.4 gate: `tests/preset-gate/gate.py` 10/10. The `scdefault` run matches 1.11.3 on 314/314 lines, state blobs included. The full-perturb run has 72 differing lines, all expected: the factory presets omit the 4 `sc_*` params, which the module resets to their defaults (WR-01). 88 lines of 1.11.3 session blobs reopen identically. 8 factory files match. The sentinel rewrites 0 files at the same version and 8 when stale. With no in-list anchor, Prev now goes to the first preset, not the last (IN-03). auval and pluginval 5 pass. No old-vs-new render: the header is not on the audio path, and the parameter outcomes are gated.
- [x] O-DigiDelay `OuariconPresetManager.h` (drift) — a5aaaa11
- [x] O-DigiDelay `ui/public/modules/preset-manager.js` (drift) — a5aaaa11
  O-DigiDelay 1.9.4 gate: `tests/preset-gate/gate.py` 12/12. 762/762 lines match 1.9.3, state blobs included; 216 lines of 1.9.3 session blobs reopen identically; 24 factory files match; the sentinel rewrites 0 files at the same version and 24 when stale. The local copy was ahead of the module twice. FORK-REVIEW named the bypass exclusion, now kept with `setExcludedParameters({"bypass"})`. It missed the v1.8.0 IN-07 apply: a gesture per set, values clamped, a non-number value loads as the default. That was upstreamed as the opt-in `setValidatedApply` in **preset-manager 1.1.1**. Off by default, the call sequence is unchanged for other consumers. Bypass stays On through all 25 loads, and every set is inside a balanced gesture. Negative control (both calls off): 4 checks fail, 100 dump lines differ. Unanchored Prev already went to the first preset in 1.9.3. auval and pluginval 5 pass.
- [x] O-Polystutter `ui/public/modules/preset-manager.js` (drift) — 5d5d8cac
  O-Polystutter 1.19.1 gate: `tests/preset-gate/gate.mjs` 6/6. JS only: the C++ side is the plugin's own `OPolystutterPresetManager.h`, which is untouched and not a FORK-REVIEW row, so the state and factory-preset paths don't change. The 1.19.0 copy and the module copy give identical 88-line traces against one mock backend (native calls with arguments, display, callbacks, returns). All 10 requested native names are registered. With no backend, the bounded wait resolves at 5058 ms and logs; the 1.19.0 control is still pending at 6 s. The served file is byte-identical to the module copy, and both built binaries embed it. `promptDelete` and `onConfirmDelete` stay dormant because `app.js` deletes through its own two-click arm. Pre-change run: C1 and D FAIL. The last `.gitignore` exception for a hand-owned `preset-manager.js` is gone. auval and pluginval 5 pass.
- [x] O-SpectralShaper `OuariconPresetManager.h` (drift) — 550772d1
  O-SpectralShaper 1.10.3 gate: `tests/preset-gate/gate.py` 10/10. 880/880 lines match 1.10.2: the 7 params and both 32-band curves (customState) per preset, state blobs, navigation and the user life cycle. 261 lines of 1.10.2 session blobs reopen identically, and a second reopen on one instance matches too (no stale `<CustomState>`). All 29 factory presets carry all 7 keys plus customState, so WR-01 reset and the empty `customLoad` change no value. A fresh instance anchors on "Default", so IN-03 never fires. The 29 factory files match. The sentinel rewrites 0 files at the same version and 29 when stale. `PluginEditor.h` moved behind `#if JUCE_WEB_BROWSER` so the console links. auval and pluginval 5 pass. **Unlisted drift found:** the plugin embeds its own `Resources/ui/modules/preset-manager.js` (1.0.0-era: no bounded `_waitForNative`, no `onConfirmDelete`). FORK-REVIEW scanned only `Source/ui/public/modules/`, so it missed this copy. It is not converged, and there is no row for it yet.

## Step 3: scala 3.2.0 KBM reference + 1c5dca67 Scala-mode fix

FORK-REVIEW step 3 counts 7 TuningEngine copies and names O-Lyrica, O-IntonationPad and O-Marimba for the 1c5dca67 fix. Step 6 lists those same three as intentional forks and `/converge-modules` says never to touch step 6 copies, so they are left out here. Of the four below, only O-Bells needs the 1c5dca67 fix.

- [ ] O-Bells `TuningEngine.cpp` (fork+ahead)
- [ ] O-Bells `TuningEngine.h` (drift)
- [ ] O-Formant `TuningEngine.cpp` (fork+ahead)
- [ ] O-Formant `TuningEngine.h` (fork+ahead)
- [ ] O-Prism `TuningEngine.cpp` (fork+ahead)
- [ ] O-Prism `TuningEngine.h` (fork+ahead)
- [ ] O-Strata `TuningEngine.cpp` (fork+ahead)
- [ ] O-Strata `TuningEngine.h` (fork+ahead)

## Step 4: O-Lyrica small scala files

One `toHTML(engine, pluginName)` call site changes.

- [ ] O-Lyrica `DSP/EmbeddedTunings.cpp` (drift)
- [ ] O-Lyrica `DSP/EmbeddedTunings.h` (drift)
- [ ] O-Lyrica `DSP/ScaleGenerator.cpp` (drift)
- [ ] O-Lyrica `DSP/ScaleGenerator.h` (drift)
- [ ] O-Lyrica `DSP/TuningExporter.cpp` (drift)
- [ ] O-Lyrica `DSP/TuningExporter.h` (drift)

## Step 5: grain-engine CounterRng

Gate: render bit-identical. O-GrainScatter keeps its Stream/slot enums. Skip O-simpleGrain while `git status` shows it dirty.

- [ ] O-Freeze `CounterRng.h` (drift)
- [ ] O-GrainScatter `dsp/CounterRng.h` (drift)
- [ ] O-simpleGrain `dsp/CounterRng.h` (drift)
- [ ] O-TextureForge `dsp/CounterRng.h` (drift)
