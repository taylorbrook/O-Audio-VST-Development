# O-Prism Code Quality Improvement - In Progress

## Version: 1.8.0 → 1.9.0 (MINOR)
## Backup: backups/O-Prism/v1.8.0/

## Completed Fixes (3/40)
1. ✅ Removed dead `ActiveNote` struct from `PluginProcessor.h:133`
2. ✅ Removed unused `#include <atomic>` from `WavetableOscillator.h:14`
3. ✅ Removed unused `mipmapLevel` param from `readSample()` (header + cpp + 2 call sites)

## Remaining HIGH Severity (7 left)
4. `PrismVoice.cpp:412` — Remove no-op `* 12.0 / 12.0` in pitch mod formula
5. `TuningEngine.cpp:763` — Fix mutex: lock once in `rebuildFrequencyTable()` instead of 128x in `calculateCustomFrequency()`
6. `SVFFilter.cpp` — Unify SVF core (3 copies → 1): fold `processNotch` into `processSingleSVF` as case 6, parameterize 24dB stage
7. `WavetableGenerator.cpp` — Replace 3x inline normalization with `WavetableFactory::normalizeFrame()`
8. `EnsembleChorus.cpp` — Consolidate 6 delay buffers → 2 shared (all voices write same input)
9. `LFO.cpp:44` — Replace `juce::MathConstants<double>::pi` with `kTwoPi` from `MathConstants.h`
10. `SVFFilter.cpp:136-137` — Replace magic `0.707` with named `kButterworthQ` constant

## Remaining MEDIUM Severity (18)
11. `PluginEditor.cpp` (6 places) — Extract `kCustomPresetIndex = 10.0f` constant
12. `PluginProcessor.cpp:549-622` — Reduce repetitive effects parameter reading (~75 lines)
13. `PluginProcessor.cpp` (6 comments) — Fix incorrect parameter count comments: subNoise→6, chorus→4, distortion→4, EQ→5, LFO→16, global→4
14. `PluginProcessor.cpp:677` — Comment says "60 Hz" but timer is `30 Hz`
15. `PrismVoice.cpp:189-221` — Deduplicate oscillator setup for Osc A/B
16. `PrismParamIds.h:107` — Add `stereoWidth` to `allSliderIds()`
17. `ScaleGenerator.cpp:107-143` — Remove 3 dead `getDescription()` methods
18. `TuningEngine.h:329` — Remove dead `mtsSynthClientConnected`
19. `TuningEngine.h:330` — Remove write-only `scalaFileLoaded`
20. `TuningEngine.h:45` — Remove dead `Mode::MTSESP` enum + reference in `getActiveTuningName()`
21. `PluginEditor.cpp:106-121, 404-423` — Deduplicate JSON-parse-to-intervals pattern
22. `EmbeddedTunings.cpp:301-322` — Remove dead `getTuningsByCategory()` and `getTuningCount()`
23. `TuningExporter.cpp:379-383` — Remove dead conditional branch (redundant period assignment)
24. `SubOscillator.cpp` — Extract `updatePhaseIncrement()` helper (3 copies → 1)
25. `ModulationMatrix.cpp:78-81` — Remove redundant `clearOffsets()` method
26. `DelayProcessor.h/.cpp` — Remove dead `tempoSync`, `audioPlayHead`, `setSync()`, `setPlayHead()`
27. `SVFFilter.cpp:56-67` — Fix `setKeyTrack` to use separate `baseCutoff` (fragile API)
28. `NoiseGenerator.cpp` — Name ~15 magic numbers (5512, 0.707, 44100 ref rate, etc.)

## Remaining LOW Severity (12)
29. `PluginProcessor.h:46` — Named constant for tail length `5.0`
30. `PrismVoice.h:134` — Named constant for pitch wheel center `8192`
31. `PrismVoice.cpp:432` — Named constant for MIDI note max `127.0f`
32. `ScaleGenerator.cpp:19-20` — Use `std::clamp` instead of `std::max(std::min())`
33. `TuningExporter.cpp:311` — Use `kTwoPi` from MathConstants instead of hand-written constant
34. `TuningExporter.cpp:284` — Remove unused `scaleName` parameter from `generatePitchCircleSVG`
35. `EmbeddedTunings.cpp:123-129` — Local `generateEDO` lambda duplicates `ScaleGenerator::generateEDO()`
36. `TuningEngine.cpp:186 + 8 more` — Remove DBG statements from production code
37. `TuningEngine.cpp:168-169` — Remove unreachable `case Custom` in switch
38. `WavetableFactory.cpp:395` — Move `generateFormantTable` into class as static method
39. `DelayProcessor.h/.cpp` — Move `= default` constructor to header
40. `EnsembleChorus.cpp:91` — Simplify redundant `outR` initialization

## Build Status
- VST3 builds clean (5 warnings — pre-existing unused lambda captures in PluginEditor.cpp)
- No errors from applied fixes
