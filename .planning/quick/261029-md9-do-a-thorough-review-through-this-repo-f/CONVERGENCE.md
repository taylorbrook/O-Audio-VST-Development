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

O-DigiDelay needs bypass-param exclusion first (FORK-REVIEW caveat).

- [ ] O-Bass `OuariconPresetManager.h` (drift)
- [ ] O-Bass `ui/public/modules/preset-manager.js` (drift)
- [ ] O-Comp `OuariconPresetManager.h` (drift)
- [ ] O-Comp `ui/public/modules/preset-manager.js` (drift)
- [ ] O-DigiDelay `OuariconPresetManager.h` (drift)
- [ ] O-DigiDelay `ui/public/modules/preset-manager.js` (drift)
- [ ] O-Polystutter `ui/public/modules/preset-manager.js` (drift)
- [ ] O-SpectralShaper `OuariconPresetManager.h` (drift)

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
