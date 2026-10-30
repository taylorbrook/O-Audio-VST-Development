# Module fork-vs-drift review (261029-md9, F-11 follow-up)

Date: 2026-10-29. Read-only review. No plugin source was changed.

## Method

- Population: the 79 same-basename copies under `plugins/*/Source` whose blob differs from the module file at HEAD (the AUDIT C9 scan, re-run; same 79).
- Each copy was compared to the module file at HEAD and to up to 11 earlier module revisions. No copy equals any historical module blob, so none is a clean old snapshot.
- Diffs were taken with whitespace ignored and the `This file is part of …` license line normalised. `+` = lines only in the plugin, `−` = lines only in the module.
- The class was then read from the diff content, the module changelog (`module.yaml`) and `git log` of the plugin copy:
  - **fork**: plugin-specific behaviour or a different component under the same name. Leave it, or converge only by design.
  - **fork+ahead**: carries local code-review fixes the module never received. It also lacks later module work. Upstream first, then converge.
  - **drift**: stale. The differences are module fixes the copy is missing, plus cosmetics. Safe to upgrade to the module.
  - **nominal**: not a real divergence (license header line only, or an `#include` redirect into `modules/`).

## Headline

| Class | Count |
|---|---|
| fork | 24 |
| fork+ahead | 16 |
| drift | 20 |
| nominal | 19 |
| **total** | **79** |

Real forks total 40. Real stale drift totals 20. The remaining 19 are cosmetic: 15 scala-tuning-engine copies differ only in the license header line, and O-Bowed's 4 are include redirects.

| Module | fork | fork+ahead | drift | nominal |
|---|---|---|---|---|
| grain-engine | 6 | 0 | 4 | 0 |
| preset-manager | 4 | 0 | 8 | 0 |
| scala-tuning-engine | 14 | 16 | 8 | 19 |

## Recommended upgrades (in order)

1. **Upstream into scala-tuning-engine first (3.3.0).** 16 fork+ahead copies hold review fixes the module lacks:
   - O-Formant: CR-09 non-finite period/generator guard, WR-08 Scala parser, IN-17 missing 11/10 + 20/11 degrees in the 43-tone table, CR-07/CR-08 KBM.
   - O-Prism (and its fork O-Strata, which is identical to it): WR-11/12/14, WR-15 sub-octave period export, CR-05 lock-once rebuild, CR-07 KBM reference, IN-11.
   - O-Bells: IN-07/08/09.
   Until these land, converging any of these copies onto the module would regress them.
2. **preset-manager drift: move to the module (8 copies, all clean trees).**
   - `OuariconPresetManager.h` in O-Bass, O-SpectralShaper (1.0.2 snapshots), O-Comp (1.0.0) and O-DigiDelay (1.0.3).
   - `preset-manager.js` in O-Bass, O-Comp, O-Polystutter and O-DigiDelay.
   - These copies miss meta-first apply, the migration hook, the empty-`customLoad` clear, `capturePresetData`/`applyPresetData`, exclusions, the bounded `_waitForNative`, `onConfirmDelete` and localized titles.
   - **Caveat:** O-DigiDelay skips the host bypass parameter on save and apply (its v1.8.0 change). The module does not (0 `getBypassParameter` references), so upstream that first or keep it via `setExcludedParameters`.
   - Gate each move with the plugin's state-persistence + factory-preset tests (memory: factory sentinel / reset-to-defaults patterns).
3. **scala-tuning-engine 3.2.0 KBM reference (`kbmFileReferenceFrequency`)**
   - Missing from all 7 non-redirect TuningEngine copies.
   - The 1c5dca67 "embedded tunings switch to Scala mode" fix is also missing from O-Bells, O-IntonationPad, O-Lyrica and O-Marimba.
   - Port it per plugin where KBM reference behaviour matters, after step 1.
4. **O-Lyrica small scala files (6 drift): replace with the module copies.** The differences are cosmetic (doc headers, the `¢` glyph) plus the `toHTML(engine, pluginName)` signature, so one call site changes.
5. **grain-engine CounterRng in O-Freeze, O-TextureForge, O-simpleGrain and O-GrainScatter (4 drift): migrate when next touched.**
   - The core is bit-identical, and `tests/grain_engine_tests.cpp` pins golden values from these copies, so migrating should leave the audio unchanged.
   - Only the namespace (`ouaricon::grain::CounterRng`) and the `Draw` helper differ.
   - O-GrainScatter keeps its own Stream/slot enums in the plugin.
   - Low priority. O-simpleGrain is in flight, so do it after that lands.
6. **Leave as intentional forks:**
   - grain-engine: O-Polystutter CounterRng (incompatible hash), O-Tapestop WindowLut (Hann-only reduction), and the same-name engines in O-GrainScatter GrainPool/GrainScheduler, O-ReverseDelay GrainScheduler and O-TextureForge GrainScheduler.
   - O-Marimba TuningEngine (independent threading design).
   - O-Lyrica and O-IntonationPad TuningEngine, and the O-Formant and O-IntonationPad tuning panels.
   - The 4 category-extended preset managers (O-Bells, O-Formant, O-Prism, O-Strata). Option: upstream category support as an opt-in, then converge them.

## Side notes

- `scripts/regen-registry-used-by.sh` matches module basenames anywhere in `Source/`, comments included. It listed O-Marimba and O-IntonationPad as preset-manager consumers only because of a comment naming `OuariconPresetManager.h`. Those two were removed by hand in the registry commit, and a re-run will add them back. A follow-up should strip comments or match only `#include`/`<script>`/CMake lines.
- The scan covers `plugins/*/Source` only. O-Bells and O-MicrotonalSampler also carry `tuning-panel.{js,css}` under `Resources/ui/` (v2.0.0 headers vs module 3.1.0), and those are not in the 79.

## Per-copy table

| # | Module | Plugin | File | +plugin / −module | Class | Evidence |
|---|---|---|---|---|---|---|
| 1 | grain-engine | O-Freeze | `CounterRng.h` | +5 / −38 | drift | pre-extraction origin copy; mix/hash/uniform core bit-identical, differs in namespace + Draw helper |
| 2 | grain-engine | O-GrainScatter | `dsp/CounterRng.h` | +22 / −33 | drift | pre-extraction origin copy; mix/hash/uniform core bit-identical, differs in namespace + Draw helper + plugin Stream/slot enums |
| 3 | grain-engine | O-GrainScatter | `dsp/GrainPool.h` | +423 / −172 | fork | same filename, different class (plugin engine predates the module) |
| 4 | grain-engine | O-GrainScatter | `dsp/GrainScheduler.h` | +141 / −127 | fork | same filename, different class (plugin engine predates the module) |
| 5 | grain-engine | O-Polystutter | `DSP/CounterRng.h` | +20 / −53 | fork | 4-arg hash (seed, stream, index, slot) — values differ; module README says not interchangeable |
| 6 | grain-engine | O-ReverseDelay | `dsp/GrainScheduler.h` | +176 / −123 | fork | same filename, different class (plugin engine predates the module) |
| 7 | grain-engine | O-simpleGrain | `dsp/CounterRng.h` | +6 / −38 | drift | pre-extraction origin copy; mix/hash/uniform core bit-identical, differs in namespace + Draw helper |
| 8 | grain-engine | O-Tapestop | `dsp/WindowLut.h` | +57 / −778 | fork | Hann-only ~40-line reduction for a 2-voice crossfade |
| 9 | grain-engine | O-TextureForge | `dsp/CounterRng.h` | +5 / −38 | drift | pre-extraction origin copy; mix/hash/uniform core bit-identical, differs in namespace + Draw helper |
| 10 | grain-engine | O-TextureForge | `dsp/GrainScheduler.h` | +146 / −137 | fork | same filename, different class (plugin engine predates the module) |
| 11 | preset-manager | O-Bass | `OuariconPresetManager.h` | +14 / −128 | drift | 1.0.2 snapshot; lacks later module fixes (meta-first, migration hook, empty customLoad, capture/apply, exclusions) |
| 12 | preset-manager | O-Bass | `ui/public/modules/preset-manager.js` | +7 / −62 | drift | 1.0.0-era JS: lacks 1.0.1 bounded _waitForNative + onConfirmDelete, 1.0.9 localized titles |
| 13 | preset-manager | O-Bells | `OuariconPresetManager.h` | +221 / −354 | fork | category-folder extension (Factory/{Category}, flattened nav); also lacks 1.0.5 meta-first, 1.0.7 empty customLoad, 1.0.8 capture/apply, 1.1.0 exclusions |
| 14 | preset-manager | O-Comp | `OuariconPresetManager.h` | +36 / −187 | drift | Jan 2026 (1.0.0) snapshot; lacks later module fixes (meta-first, migration hook, empty customLoad, capture/apply, exclusions) |
| 15 | preset-manager | O-Comp | `ui/public/modules/preset-manager.js` | +7 / −62 | drift | 1.0.0-era JS: lacks 1.0.1 bounded _waitForNative + onConfirmDelete, 1.0.9 localized titles |
| 16 | preset-manager | O-DigiDelay | `OuariconPresetManager.h` | +41 / −129 | drift | 1.0.3 snapshot + local bypass-skip (v1.8.0); lacks later module fixes (meta-first, migration hook, empty customLoad, capture/apply, exclusions) |
| 17 | preset-manager | O-DigiDelay | `ui/public/modules/preset-manager.js` | +45 / −129 | drift | 1.0.0-era JS: lacks 1.0.1 bounded _waitForNative + onConfirmDelete, 1.0.9 localized titles (local _safeCall refactor) |
| 18 | preset-manager | O-Formant | `OuariconPresetManager.h` | +230 / −347 | fork | category-folder extension (Factory/{Category}, flattened nav); also lacks 1.0.5 meta-first, 1.0.7 empty customLoad, 1.0.8 capture/apply, 1.1.0 exclusions |
| 19 | preset-manager | O-Polystutter | `ui/public/modules/preset-manager.js` | +7 / −62 | drift | 1.0.0-era JS: lacks 1.0.1 bounded _waitForNative + onConfirmDelete, 1.0.9 localized titles |
| 20 | preset-manager | O-Prism | `OuariconPresetManager.h` | +255 / −338 | fork | category-folder extension (Factory/{Category}, flattened nav); also lacks 1.0.5 meta-first, 1.0.7 empty customLoad, 1.0.8 capture/apply, 1.1.0 exclusions |
| 21 | preset-manager | O-SpectralShaper | `OuariconPresetManager.h` | +9 / −132 | drift | 1.0.2 snapshot; lacks later module fixes (meta-first, migration hook, empty customLoad, capture/apply, exclusions) |
| 22 | preset-manager | O-Strata | `OuariconPresetManager.h` | +242 / −347 | fork | category-folder extension (Factory/{Category}, flattened nav); also lacks 1.0.5 meta-first, 1.0.7 empty customLoad, 1.0.8 capture/apply, 1.1.0 exclusions |
| 23 | scala-tuning-engine | O-Bells | `EmbeddedTunings.cpp` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 24 | scala-tuning-engine | O-Bells | `EmbeddedTunings.h` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 25 | scala-tuning-engine | O-Bells | `ScaleGenerator.cpp` | +4 / −2 | fork+ahead | local code-review fix the module never received (period-first clamp / divide-by-zero / WR-15 sub-octave period / CR-09 / IN-17) |
| 26 | scala-tuning-engine | O-Bells | `ScaleGenerator.h` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 27 | scala-tuning-engine | O-Bells | `TuningEngine.cpp` | +14 / −37 | fork+ahead | plugin review fixes (parser hardening / CR-07 KBM ref / lock-once) not in module; lacks 3.2.0 KBM reference and the 1c5dca67 Scala-mode fix |
| 28 | scala-tuning-engine | O-Bells | `TuningEngine.h` | +1 / −19 | drift | only missing 3.2.0 KBM reference accessors |
| 29 | scala-tuning-engine | O-Bells | `TuningExporter.cpp` | +4 / −0 | fork+ahead | local code-review fix the module never received (period-first clamp / divide-by-zero / WR-15 sub-octave period / CR-09 / IN-17) |
| 30 | scala-tuning-engine | O-Bells | `TuningExporter.h` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 31 | scala-tuning-engine | O-Bowed | `EmbeddedTunings.h` | +2 / −74 | nominal | include redirect into modules/ (consumes the module) |
| 32 | scala-tuning-engine | O-Bowed | `ScaleGenerator.h` | +2 / −88 | nominal | include redirect into modules/ (consumes the module) |
| 33 | scala-tuning-engine | O-Bowed | `TuningEngine.h` | +2 / −354 | nominal | include redirect into modules/ (consumes the module) |
| 34 | scala-tuning-engine | O-Bowed | `TuningExporter.h` | +2 / −78 | nominal | include redirect into modules/ (consumes the module) |
| 35 | scala-tuning-engine | O-Formant | `EmbeddedTunings.cpp` | +5 / −4 | fork+ahead | local code-review fix the module never received (period-first clamp / divide-by-zero / WR-15 sub-octave period / CR-09 / IN-17) |
| 36 | scala-tuning-engine | O-Formant | `EmbeddedTunings.h` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 37 | scala-tuning-engine | O-Formant | `ScaleGenerator.cpp` | +18 / −6 | fork+ahead | local code-review fix the module never received (period-first clamp / divide-by-zero / WR-15 sub-octave period / CR-09 / IN-17) |
| 38 | scala-tuning-engine | O-Formant | `ScaleGenerator.h` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 39 | scala-tuning-engine | O-Formant | `TuningEngine.cpp` | +235 / −201 | fork+ahead | plugin review fixes (parser hardening / CR-07 KBM ref / lock-once) not in module; lacks 3.2.0 KBM reference |
| 40 | scala-tuning-engine | O-Formant | `TuningEngine.h` | +38 / −28 | fork+ahead | plugin review fixes (parser hardening / CR-07 KBM ref / lock-once) not in module; lacks 3.2.0 KBM reference |
| 41 | scala-tuning-engine | O-Formant | `TuningExporter.cpp` | +20 / −10 | fork+ahead | local code-review fix the module never received (period-first clamp / divide-by-zero / WR-15 sub-octave period / CR-09 / IN-17) |
| 42 | scala-tuning-engine | O-Formant | `TuningExporter.h` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 43 | scala-tuning-engine | O-Formant | `ui/public/js/tuning-panel.js` | +155 / −134 | fork | plugin-local UI/i18n/token edits (multiple plugin commits); not on module 3.1.0 |
| 44 | scala-tuning-engine | O-IntonationPad | `DSP/EmbeddedTunings.cpp` | +9 / −31 | fork | v2.4.8 thread-safety (once_flag, snapshot publish), trimmed API, ScaleGenerator adapter |
| 45 | scala-tuning-engine | O-IntonationPad | `DSP/EmbeddedTunings.h` | +2 / −13 | fork | v2.4.8 thread-safety (once_flag, snapshot publish), trimmed API, ScaleGenerator adapter |
| 46 | scala-tuning-engine | O-IntonationPad | `DSP/ScaleGenerator.cpp` | +2 / −41 | fork | v2.4.8 thread-safety (once_flag, snapshot publish), trimmed API, ScaleGenerator adapter |
| 47 | scala-tuning-engine | O-IntonationPad | `DSP/ScaleGenerator.h` | +0 / −7 | drift | only missing module description helpers (unused here) |
| 48 | scala-tuning-engine | O-IntonationPad | `DSP/TuningEngine.cpp` | +96 / −175 | fork | v2.4.8 thread-safety (once_flag, snapshot publish), trimmed API, ScaleGenerator adapter |
| 49 | scala-tuning-engine | O-IntonationPad | `DSP/TuningEngine.h` | +10 / −37 | fork | v2.4.8 thread-safety (once_flag, snapshot publish), trimmed API, ScaleGenerator adapter |
| 50 | scala-tuning-engine | O-IntonationPad | `DSP/TuningExporter.cpp` | +2 / −5 | fork | v2.4.8 thread-safety (once_flag, snapshot publish), trimmed API, ScaleGenerator adapter |
| 51 | scala-tuning-engine | O-IntonationPad | `DSP/TuningExporter.h` | +2 / −30 | fork | v2.4.8 thread-safety (once_flag, snapshot publish), trimmed API, ScaleGenerator adapter |
| 52 | scala-tuning-engine | O-IntonationPad | `ui/public/css/tuning-panel.css` | +162 / −137 | fork | plugin-local UI/i18n/token edits (multiple plugin commits); not on module 3.1.0 |
| 53 | scala-tuning-engine | O-IntonationPad | `ui/public/js/tuning-panel.js` | +267 / −213 | fork | plugin-local UI/i18n/token edits (multiple plugin commits); not on module 3.1.0 |
| 54 | scala-tuning-engine | O-Lyrica | `DSP/EmbeddedTunings.cpp` | +7 / −7 | drift | pre-module v1.14-1.16 ancestor: doc headers, ¢ glyph, toHTML() without pluginName |
| 55 | scala-tuning-engine | O-Lyrica | `DSP/EmbeddedTunings.h` | +2 / −2 | drift | pre-module v1.14-1.16 ancestor: doc headers, ¢ glyph, toHTML() without pluginName |
| 56 | scala-tuning-engine | O-Lyrica | `DSP/ScaleGenerator.cpp` | +6 / −4 | drift | pre-module v1.14-1.16 ancestor: doc headers, ¢ glyph, toHTML() without pluginName |
| 57 | scala-tuning-engine | O-Lyrica | `DSP/ScaleGenerator.h` | +1 / −1 | drift | pre-module v1.14-1.16 ancestor: doc headers, ¢ glyph, toHTML() without pluginName |
| 58 | scala-tuning-engine | O-Lyrica | `DSP/TuningEngine.cpp` | +204 / −78 | fork | origin of module v2 (octave stretch, rotated-interval cache) + IN fixes; lacks 3.2.0 KBM reference |
| 59 | scala-tuning-engine | O-Lyrica | `DSP/TuningEngine.h` | +74 / −38 | fork | origin of module v2 (octave stretch, rotated-interval cache) + IN fixes; lacks 3.2.0 KBM reference |
| 60 | scala-tuning-engine | O-Lyrica | `DSP/TuningExporter.cpp` | +9 / −9 | drift | pre-module v1.14-1.16 ancestor: doc headers, ¢ glyph, toHTML() without pluginName |
| 61 | scala-tuning-engine | O-Lyrica | `DSP/TuningExporter.h` | +2 / −3 | drift | pre-module v1.14-1.16 ancestor: doc headers, ¢ glyph, toHTML() without pluginName |
| 62 | scala-tuning-engine | O-Marimba | `TuningEngine.cpp` | +297 / −766 | fork | independent design (tuningLock + requestRebuild/serviceRebuild, no intervalMutex); pre-module ancestor |
| 63 | scala-tuning-engine | O-Marimba | `TuningEngine.h` | +63 / −305 | fork | independent design (tuningLock + requestRebuild/serviceRebuild, no intervalMutex); pre-module ancestor |
| 64 | scala-tuning-engine | O-Prism | `EmbeddedTunings.cpp` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 65 | scala-tuning-engine | O-Prism | `EmbeddedTunings.h` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 66 | scala-tuning-engine | O-Prism | `ScaleGenerator.cpp` | +3 / −2 | fork+ahead | local code-review fix the module never received (period-first clamp / divide-by-zero / WR-15 sub-octave period / CR-09 / IN-17) |
| 67 | scala-tuning-engine | O-Prism | `ScaleGenerator.h` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 68 | scala-tuning-engine | O-Prism | `TuningEngine.cpp` | +181 / −74 | fork+ahead | plugin review fixes (parser hardening / CR-07 KBM ref / lock-once) not in module; lacks 3.2.0 KBM reference |
| 69 | scala-tuning-engine | O-Prism | `TuningEngine.h` | +24 / −24 | fork+ahead | plugin review fixes (parser hardening / CR-07 KBM ref / lock-once) not in module; lacks 3.2.0 KBM reference |
| 70 | scala-tuning-engine | O-Prism | `TuningExporter.cpp` | +11 / −9 | fork+ahead | local code-review fix the module never received (period-first clamp / divide-by-zero / WR-15 sub-octave period / CR-09 / IN-17) |
| 71 | scala-tuning-engine | O-Prism | `TuningExporter.h` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 72 | scala-tuning-engine | O-Strata | `EmbeddedTunings.cpp` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 73 | scala-tuning-engine | O-Strata | `EmbeddedTunings.h` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 74 | scala-tuning-engine | O-Strata | `ScaleGenerator.cpp` | +3 / −2 | fork+ahead | local code-review fix the module never received (period-first clamp / divide-by-zero / WR-15 sub-octave period / CR-09 / IN-17) |
| 75 | scala-tuning-engine | O-Strata | `ScaleGenerator.h` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
| 76 | scala-tuning-engine | O-Strata | `TuningEngine.cpp` | +181 / −74 | fork+ahead | plugin review fixes (parser hardening / CR-07 KBM ref / lock-once) not in module; lacks 3.2.0 KBM reference |
| 77 | scala-tuning-engine | O-Strata | `TuningEngine.h` | +24 / −24 | fork+ahead | plugin review fixes (parser hardening / CR-07 KBM ref / lock-once) not in module; lacks 3.2.0 KBM reference |
| 78 | scala-tuning-engine | O-Strata | `TuningExporter.cpp` | +11 / −9 | fork+ahead | local code-review fix the module never received (period-first clamp / divide-by-zero / WR-15 sub-octave period / CR-09 / IN-17) |
| 79 | scala-tuning-engine | O-Strata | `TuningExporter.h` | +0 / −0 | nominal | license header line only — byte-equal otherwise |
