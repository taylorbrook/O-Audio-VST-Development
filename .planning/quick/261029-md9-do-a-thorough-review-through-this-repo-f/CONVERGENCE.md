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

- [x] O-Bells `TuningEngine.cpp` (fork+ahead) — 697df9a9
- [x] O-Bells `TuningEngine.h` (drift) — 697df9a9
  O-Bells 4.9.4 gate: both files are the module 3.3.0 copies, except O-Bells' always-Scala fork (the engine starts in Scala mode and `setBuiltInPreset(Equal12TET)` stays in Scala, so the tonic, a .kbm and the scale name apply to 12-TET). Module parity dump, new tree vs module: every TuningEngine case equal, including the 13 load/KBM/edit cases that differ from 4.9.3 (`kbm.ref_c`: 4.9.3 a4=400, 4.9.4 and module a4=440). The `kbm.degree_wrap` divergence accepted in step 1 is gone: O-Bells now wraps. Old-vs-new renders 165/165 bit-identical (55 jobs: defaults, master tune 432, stretch 1.02, 12 temperaments, 40 factory presets; × note 60 dry, note 41 with FX, note 79 1 s hold). Module unit tests built against the O-Bells tree: 27/34, and all 7 failures are in ScaleGenerator/EmbeddedTunings/TuningExporter (CR-09, IN-17 ×2, IN-11 export, WR-15 ×2, WR-20). auval and pluginval 5 pass. **Unlisted drift:** O-Bells `ScaleGenerator.cpp`, `TuningExporter.cpp` and `EmbeddedTunings.cpp` are still the pre-3.3.0 copies. Step 1 upstreamed their fixes, but no step converges them back, so there is no row for them yet.
- [x] O-Formant `TuningEngine.cpp` (fork+ahead) — 8b645942
- [x] O-Formant `TuningEngine.h` (fork+ahead) — 8b645942
  O-Formant 1.34.3 gate. **Not converged to the module copy: it is still a fork, now level with the module wherever the module was ahead.** FORK-REVIEW's "lacks 3.2.0 KBM reference" was half right. Formant already had its own CR-07 reference (`kbmReferenceFreq`), and its KBM engine is ahead of module 3.3.0 in ways step 1 never upstreamed:
  - WR-07: `x` keys and out-of-range keys are silent (0 Hz), not 12-TET; a size-0 map is linear; the octave degree is resolved at lookup.
  - v1.30.1: a loaded .kbm applies in 12-TET mode too.
  - WR-06: `loadKBMFromString` / `getKBMState` / `isKBMLoaded` for session restore.
  - WR-20: the linear-mapping .kbm export.
  - IN-20: `scaleName` locking.
  - `kTwelveTetModeName`.

  The parity dump has no case for any of these, so the step 1 gate passed without them. Swapping in the module copy would regress all of them and break the build, since `PluginProcessor` calls the WR-06 API.

  What 1.34.3 takes from the module:
  - the 3.2.0 API (`getKbmReferenceNote`, `getKbmReferenceFrequency`, `setKbmReferenceFrequency`, with the file and active reference split)
  - a map size over 128 is rejected (WR-12)
  - the octave degree is clamped to 4096
  - a non-finite or out-of-band key (outside 1 Hz–100 kHz) falls back to 12-TET
  - the .scl count must be 1–4096, and a tab after the count is accepted (IN-08/WR-11)
  - the table rebuild locks once (CR-05)

  1c5dca67 was already in Formant.

  Gate results:
  - Module `parity_gate.py`: formant 23/23, and the `kbm.map_huge` divergence accepted in step 1 now prints STALE (equal). Drop it from `ACCEPTED` when the module tests are next touched.
  - 1.34.2-vs-1.34.3 engine dump: 143/149 cases equal. That covers all 120 no-KBM tables (10 presets × tonic 0/2/7 × A4 440/432 × stretch 1/1.02, export included) and every WR-07/12-TET/tonic KBM case. The 6 cases that differ are the intended malformed inputs: map 200, octave degree 300, reference 30 kHz, BP linear top keys over 100 kHz, 7-key map bottom keys under 1 Hz. All 13 .kbm session round-trips reproduce.
  - Module unit tests built against the 1.34.3 tree: 32/34. Both failures are in TuningExporter (IN-07, IN-11 export); none are in TuningEngine. Built against the 1.34.2 tree, they don't compile (3.2.0 API missing).
  - Installed-binary pedalboard renders: 14/14 bit-identical to 1.34.2 (7 notes 36–96 × 2 runs; the old binary is deterministic run to run).
  - auval and pluginval 5 pass.

  **New finding, no row yet:** to converge Formant fully, module 3.4.0 first needs Formant's WR-07/WR-06/WR-20/IN-20 and the KBM-in-12-TET behaviour upstreamed. That changes `x`/out-of-range keys from 12-TET to silent for every consumer, so it needs a decision. Unrelated to this step, Formant's `TuningExporter.cpp` still differs from the module (22 lines).
- [x] O-Prism `TuningEngine.cpp` (fork+ahead) — f200af3f
- [x] O-Prism `TuningEngine.h` (fork+ahead) — f200af3f
  O-Prism 1.30.4 gate. **Still a fork:** the module 3.3.0 copy, plus 4 things Prism has that the module lacks and step 1 never upstreamed:
  - WR-17 `writeStateTo`/`restoreStateFrom`. `PluginProcessor` calls them, so a straight swap would not build.
  - IN-07: the 13-degree Bohlen-Pierce preset (the module still has 12).
  - Atomic `pitchBendRange`, which the audio thread reads.
  - Prism IN-09: `isNoteMapped` under `intervalMutex`. The label clashes with the module's IN-09, which is the degree wrap.

  Taken from the module:
  - the 3.2.0 KBM reference API with the file and active reference split
  - WR-08 first-token pitch parsing, `3/2/5` rejected, the inf guard, a tab after the count
  - octave degree clamped to 4096
  - the IN-09 degree wrap
  - mapped frequencies outside 1 Hz–100 kHz fall back to 12-TET

  1c5dca67 was already in Prism. The session tree adds `kbmActiveRefFrequency` and `kbmFileRefFrequency`; `kbmRefFrequency` keeps its old meaning, so 1.30.3 opens 1.30.4 sessions in tune.

  Gate results:
  - Module `parity_gate.py`: prism 17/17.
  - Module unit tests built against the 1.30.4 tree: 29/34. All 5 failures are in ScaleGenerator/EmbeddedTunings/TuningExporter (CR-09, IN-17 ×2, IN-07 export, WR-20); none are in TuningEngine.
  - 1.30.3-vs-1.30.4 engine dump: 582/600 equal. That covers all 120 preset tables (10 × tonic 0/2/7 × A4 440/432 × stretch 1/1.02), custom .scl, edits, bend, and every well-formed .kbm. The 18 that differ are the intended malformed .kbm: 7-key map out of band, reference 0/5 Hz/30 kHz.
  - Session reopen: 440/442 1.30.3 states reopen equal in 1.30.4, and 147/147 1.30.4 states reopen equal to the round trip.
  - Installed-binary pedalboard: 40/40 parameter renders bit-identical (1.30.3 run-to-run 40/40).
  - Sequential state injection: 146/147 renders identical. The 1 that differs is explained below; rendered alone, it is identical.
  - auval and pluginval 5 pass.

  **Unlisted bits Prism has that the module lacks (no row yet):** the 4 items above. Upstreaming them in module 3.4.0 would let Prism/Strata converge fully.
  
  **New finding, pre-existing in 1.30.3:** `restoreStateFrom` keeps a stale .kbm when the incoming state has none, so a setState on a used instance inherits the previous mapping. This is what the sequential injection exposed. Not fixed here, because it is out of scope for a convergence step.
  
  Also: `parity_gate.py` (`kbm.degree_wrap`) and `scl.tworatios` now differ from the 1.30.3 copy as intended.
- [x] O-Strata `TuningEngine.cpp` (fork+ahead) — 34b082af
- [x] O-Strata `TuningEngine.h` (fork+ahead) — 34b082af
  O-Strata 1.1.4 gate. **Still a fork, the same one as O-Prism:** both files are O-Prism 1.30.4's copies byte for byte apart from the licence line, and the 1.1.3 copies were byte-identical to O-Prism 1.30.3's (FORK-REVIEW "identical to it" verified). So Prism's 4 kept items (WR-17 state, BP-13, atomic bend, locked `isNoteMapped`), its module takings, and its engine-dump/session-reopen results all carry over. `PluginProcessor` makes the same engine calls as Prism's.
  - Module `parity_gate.py`: strata 17/17; Strata and Prism parity dumps identical. 1.1.3-vs-1.1.4 parity dump 43/45, the 2 diffs the intended `scl.tworatios` and `kbm.degree_wrap`.
  - Module unit tests built against the 1.1.4 tree: 29/34, the same 5 ScaleGenerator/EmbeddedTunings/TuningExporter failures as Prism (CR-09, IN-17 ×2, IN-07 export, WR-20).
  - Seeded render harness, 1.1.3 tree vs 1.1.4 tree: 396/396 bit-identical (11 presets × tonic 0/2/7 × A4 440/432 × stretch 1/1.02 × notes 36/60/84; 128 distinct outputs, all finite). **Harness trap:** `--print-only` never pumps the message loop, so tuning params set with `--real` never reach the engine (it syncs through an AsyncUpdater); a first run gave 3 distinct digests for 396 jobs, one per note. The scratch copies pumped 100 ms after a warm-up block. `--gate tuning` 6/6.
  - auval and pluginval 5 pass.
  - Same pre-existing `restoreStateFrom` stale-.kbm finding as Prism, not fixed. Strata's `ScaleGenerator.cpp`/`TuningExporter.cpp` remain the pre-3.3.0 forks (no step converges them; same unlisted drift as O-Bells).

## Step 4: O-Lyrica small scala files

One `toHTML(engine, pluginName)` call site changes.

- [x] O-Lyrica `DSP/EmbeddedTunings.cpp` (drift) — 5db17ef9
- [x] O-Lyrica `DSP/EmbeddedTunings.h` (drift) — 5db17ef9
- [x] O-Lyrica `DSP/ScaleGenerator.cpp` (drift) — 5db17ef9
- [x] O-Lyrica `DSP/ScaleGenerator.h` (drift) — 5db17ef9
- [x] O-Lyrica `DSP/TuningExporter.cpp` (drift) — 5db17ef9
- [x] O-Lyrica `DSP/TuningExporter.h` (drift) — 5db17ef9
  O-Lyrica 2.6.7 gate. All 6 files are byte-identical to module 3.3.0. The call site is now `toHTML(engine, "O-Lyrica")`. **FORK-REVIEW's "cosmetic" no longer holds:** it diffed against module 3.1.0, and step 1 since put real fixes into these files. So the swap also brings:
  - IN-17: Partch 43 gets its two missing degrees
  - CR-09: the Rank-2 period is clamped first, with a NaN/huge-input guard
  - export fixes IN-07, IN-11, WR-15 (sub-octave period) and WR-20 (escaping)

  None of these reach audio. Sessions store intervals, so only a fresh library load of Partch 43 changes.
  - Module `parity_dump`, 2.6.6 tree vs 2.6.7 tree: 39/45 equal, including every `scl`/`kbm`/`edit`/`dflt` TuningEngine case. The 6 that differ are the fixes above (`gen.rank2_order`, `emb.partch43`, 4 `exp.*`). The 2.6.6 tree hangs forever on `gen.rank2_huge` and `gen.rank2_nonfinite` (that is CR-09 itself), so the comparison ran from a scratch copy of the dump with those two cases skipped.
  - 2.6.7 vs module: every `gen`/`emb`/`exp` case is equal. The `scl`/`kbm`/`edit`/preset-table cases differ because Lyrica's TuningEngine is the step-6 fork.
  - The module unit tests don't compile against the Lyrica tree: the fork lacks the 3.2.0 KBM API. The files under test are byte-identical to the module, so the module's own 34/34 run covers them.
  - auval and pluginval 5 pass.

## Step 5: grain-engine CounterRng

Gate: render bit-identical. O-GrainScatter keeps its Stream/slot enums. Skip O-simpleGrain while `git status` shows it dirty.

- [ ] O-Freeze `CounterRng.h` (drift)
- [ ] O-GrainScatter `dsp/CounterRng.h` (drift)
- [ ] O-simpleGrain `dsp/CounterRng.h` (drift)
- [ ] O-TextureForge `dsp/CounterRng.h` (drift)
