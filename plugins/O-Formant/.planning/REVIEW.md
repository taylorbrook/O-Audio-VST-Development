---
plugin: O-Formant
version_reviewed: 1.25.0 (CHANGELOG head; PLUGINS.md registry shows 1.24.1 installed)
reviewed: 2026-07-01
depth: deep
review_type: gsd-code-review (4 parallel subsystem reviewers + main-thread verification)
files_reviewed: 36
loc_reviewed: 9151
findings:
  critical: 5
  warning: 13
  info: 19
  total: 37
criticals_verified_against_source: 5
status: issues_found
---

# O-Formant — GSD Code Review

**Reviewed:** 2026-07-01 · **Depth:** deep · **Scope:** all 36 C++ source files (`Source/**`, ~9,150 LOC)

## Method

Four `gsd-code-reviewer` agents ran in parallel over subsystem partitions, each at deep depth:

| Group | Subsystem | Findings |
|-------|-----------|----------|
| A | RT audio path — `PluginProcessor`, `FormantVoice` | 1C / 2W / 4I |
| B | Voice-source & formant DSP — glottal source, biquads, formant banks, vowel morph | 0C / 4W / 6I |
| C | Consonant articulation + output effects — `ConsonantEngine`, delay/EQ/reverb | 0C / 2W / 4I |
| D | Microtonal tuning, presets, WebView editor | 4C / 5W / 5I |

All **5 Critical findings were re-verified against source on the main thread** before inclusion — every one reproduces. Per-group reports remain in `.planning/code-review/GROUP-{A,B,C,D}-*.md`.

## What's solid (verified, cleared)

- **Audio-thread frequency lookup is RT-safe.** `TuningEngine::getFrequency()` reads a lock-free `std::array<std::atomic<double>,128>` table; the message thread rebuilds it under `intervalMutex` and the audio thread never touches `scaleIntervals`. No data race.
- **Note-Expression path is correct.** `PendingTuningTable` is atomic, drained on the audio thread only; `exchange(0.0)` slot consumption matches the documented design.
- **Filter stability holds.** All resonators clamp pole radius `r ≤ 0.9999` (poles strictly inside the unit circle), bandpass Q to `[0.5, 25]`, formant freq to `[20, Nyquist−100]`. Coefficient factories return stack `std::array` (no RT heap). No unconditional blow-up exists.
- **v1.24.2 consonant envelope rework is correct.** Attack/Hold/Decay can't exceed 1.0 or go negative; burst is summed outside the envelope as intended; all sample counts `jmax(1,…)`-guarded.
- **WebView bridge is intact.** All 36 JS `getNativeFunction` calls have matching C++ `withNativeFunction` registrations — no silently-dead controls.
- `setLatencySamples(0)` is the correct JUCE 8 idiom (no illegal `getLatencySamples` override).

---

## Critical (5) — all verified against source

### CR-01 — Pitch bend (standard MIDI + MPE per-note) is completely dead
**File:** `Source/FormantVoice.cpp:331-334`, `660-665`; `Source/dsp/PitchGlide.h`
`notePitchbendChanged()` is an empty stub whose comment claims *"Pitchbend handled per-sample via getCurrentlyPlayingNote().getFrequencyInHertz()"* — but nothing in `renderNextBlock` ever reads per-sample note pitch. Sounding pitch = `pitchGlide.getNextFrequency()`, whose target is set **once** at note-on; `PitchGlide` has no bend input. **Verified:** the render loop at line 661 reads only the glide ramp.
**Impact:** the bend wheel and MPE per-note pitch produce zero pitch change. The primary microtonal path (TuningEngine + Dorico NE) is unaffected, but MPE pitch is an advertised feature and is non-functional.
**Fix:** in `renderNextBlock`, recompute a live bend ratio per sample/coeff-block from `getCurrentlyPlayingNote().getFrequencyInHertz()` vs the bend-free note-on reference, and fold it multiplicatively into `pitchGlide.setTarget(tunedF0 * bendRatio)` so tuning + NE + bend stack correctly. Delete the misleading comment.

### CR-02 — Malformed `.kbm` map-size → unbounded allocation → crash
**File:** `Source/TuningEngine.cpp:533, 549, 562`
`newMapSize` is read via `getIntValue()` and never clamped (unlike firstNote/lastNote/middle/ref, which are `jlimit`ed at 542-545). `mappingCount = newMapSize` (549), then the fill loop at 562 pushes entries until `size() < mappingCount`. **Verified:** a `.kbm` whose first line is e.g. `2000000000` drives ~2 billion `push_back`s → `std::bad_alloc`/crash on a user-supplied file.
**Fix:** `newMapSize = juce::jlimit(0, 512, newMapSize);` before use; clamp `mappingCount` to `[1,512]`; also clamp `newOctaveDegree` (line 539/575) — see WR-05.

### CR-03 — All 24 embedded factory tunings load mistuned (period dropped)
**File:** `Source/PluginEditor.cpp:552-566` + `TuningEngine::setCustomIntervals` (`TuningEngine.cpp:240-269`)
`EmbeddedTuning::intervals` is documented *"Cents from unison (excluding period)"* (`EmbeddedTunings.h:30`), period stored separately in `->period`. `loadEmbeddedTuning` calls `setCustomIntervals(tuningData->intervals, …)` **without appending `->period`**, and `setCustomIntervals` doesn't append one — so `scaleIntervals.back()` becomes the last real degree, and both the C++ audio math (`calculateCustomFrequency` uses `back()` as period) and the JS panel treat that degree as the octave. **Verified:** the built-in-preset path `push_back`es the period correctly at `TuningEngine.cpp:172-176` — the embedded path is inconsistent with it.
**Impact:** `young1799` repeats every 1091.7c instead of 1200; non-octave tunings (Bohlen-Pierce, Carlos Alpha) lose their defining period entirely. Every factory tuning is audibly wrong.
**Fix:** append the period before handing off:
```cpp
std::vector<double> withPeriod = tuningData->intervals;
withPeriod.push_back(tuningData->period);
processorRef.tuningEngine.setCustomIntervals(withPeriod, tuningData->name);
```

### CR-04 — Preset name used verbatim as filename → `/` data loss + `..` path traversal
**File:** `Source/OuariconPresetManager.h:229, 253, 268, 289, 328` (name from `main.js:933` `prompt()`, `.trim()` only)
`savePreset` builds `getUserPresetsDirectory().getChildFile(presetName + ".json")` with no sanitization (**verified**, line 229). This is the documented Ouaricon gotcha, unmitigated:
- **`/` in the name** ("Koto / Harp") → treated as a subdirectory separator; save silently fails while the UI shows it as saved.
- **`..` in the name** → arbitrary-path `.json` **write** (`savePreset`), **read** (`loadPreset` 268, `loadPresetFromCategory` 289 — `category` also unsanitized), and **delete** (`deletePreset` 328). `isFactoryPreset` (159) is bypassable.
**Fix:** `auto safe = juce::File::createLegalFileName(presetName).trim();` (strips `/ \ .. :`); bail if empty; apply to `presetName` **and** `category` across save/load/delete/loadFromCategory/isFactoryPreset. See [[critical_preset_name_slash_path_separator]].

### CR-05 — Editing one interval silently wipes any 12-entry scale to 12-TET
**File:** `Source/TuningEngine.cpp:279`
`setSingleInterval` resets the whole scale to 12-TET when `scaleIntervals.size() < 2 || scaleIntervals.size() == 12`. **Verified.** A valid custom scale can hold exactly 12 stored values — 11-EDO (`generateEDO(11)` → 12 entries), any 12-value `setTuningIntervals` payload, or a 12-note embedded tuning. The first drag of a degree discards the scale and replaces it with 12-TET, then applies the edit on top. Silent data loss on a normal UI action. **Interacts with CR-03:** embedded 12-note tunings loaded via the broken path present as exactly 12 entries and are especially exposed.
**Fix:** remove the `|| scaleIntervals.size() == 12` branch; track initialization with an explicit flag instead of guessing from size.

---

## Warnings (13)

### WR-01 — State round-trip loses tuning on headless reload  *(Group A)*
`Source/PluginProcessor.cpp:856-926` — `getStateInformation` saves the built-in `preset` but `setStateInformation` never restores it; master-tune/octave-stretch/mode/preset only reach `TuningEngine` via editor-only WebView callbacks. Offline bounce with no UI open renders in default 12-TET at A=440 even if the session used Werckmeister III at A=442. Custom `.scl` intervals do survive. **Fix:** push restored APVTS/tuning values into the engine directly in `setStateInformation` (apply built-in preset before custom intervals so a custom `.scl` still wins).

### WR-02 — `onVst3RawEvent` can heap-allocate on the audio thread  *(Group A — shared module)*
`modules/tuning/note-expression/cpp/NoteExpression.h:167-173` (drained via `PluginProcessor.cpp:750`) — `blockEvents.push_back` is `reserve(64)`'d; a block with >64 raw NE events (dense Dorico divisi at small buffers) reallocates on the RT thread. **Fix upstream:** drop overflow instead of growing (`if (size() < capacity()) push_back(e);`). Fix benefits all plugins using the module.

### WR-03 — VowelMorpher IDW weight can overflow to Inf → NaN formant freqs  *(Group B)*
`Source/dsp/VowelMorpher.h:48-53` — `1.0f / std::pow(dist, focus)` has no upper bound; just above the `1e-6` snap epsilon with large `focus`, `pow` underflows → weight `Inf` → `weightSum Inf` → `invSum 0` → `weight = Inf*0 = NaN` → NaN formant freqs. Combined with WR-05 this yields a persistent per-formant dropout. **Fix:** floor `dist` (`max(dist,1e-3f)`), cap each weight, and guard `weightSum` finite/`>0` before dividing. Clamp `cursorX/cursorY` to `[0,1]` on entry (IN-11).

### WR-04 — LFGlottalSource phase wrap is single-`if` → out-of-bounds wavetable read  *(Group B)*
`Source/dsp/LFGlottalSource.h:78-82, 107` (read via `GlottalWavetable::getSample`) — a single `if (phase>=1.0) phase-=1.0` only corrects `[1.0,2.0)`. If one sample advances phase `≥1.0` (`f0 ≥ sampleRate`, reachable at `sr=8000` with high notes), post-wrap phase stays `≥1.0` → `getSample(idx≥2048)` over-reads the 2049-sample frame with no bounds check. `setFrequency` never clamps. **Fix:** `while` loop for the wrap **and** clamp increment (`min(0.5, f0/sr)`).

### WR-05 — FormantBiquad NaN guard leaves poisoned coefficients → sticky silence  *(Group B)*
`Source/dsp/FormantBiquad.h:33-42` — the guard resets state (`z1=z2=0`) but not the coefficients; if `b0..a2` are NaN/Inf (e.g. from WR-03), every subsequent sample re-trips the guard and outputs 0 **until the next valid block-rate update** — a one-sample transient becomes a persistent per-formant dropout. **Fix:** validate at `setCoefficients` and keep last-known-good.

### WR-06 — No denormal flushing in per-voice feedback filter state  *(Group B)*
`FormantBiquad.h:27-42`, `CascadeFormantBank.h:191-208` (`r≈0.997`), `AspirationNoise.h:95-96`, `NasalPoleZero.h:158-160` — long decaying tails rely entirely on the caller's `ScopedNoDenormals`. If any per-voice processing ever runs outside that scope, x86 traps on denormals → CPU spike. **Fix:** guarantee (and document) `ScopedNoDenormals` at the voice/process entry, or add a cheap flush in `processSample`.

### WR-07 — Delay buffer under-sized for 2.0 s above 96 kHz  *(Group C)*
`Source/dsp/DelayProcessor.h:29-30`, `.cpp:45,87-88` — fixed `DelayLine{192000}`; `delayTime` allows 2.0 s and `setTime` never clamps. Above 96 kHz a long delay overruns the buffer. **Verified in JUCE 8.0.9:** `popSample` masks `% totalSize` (no OOB read) but the delay **silently aliases to a wrong, shorter time**; Debug `jassert` fires. **Fix:** clamp to `delayL.getMaximumDelayInSamples()`, or size the line from `2.0 * spec.sampleRate`.

### WR-08 — EQ recomputes IIR coefficients on the audio thread (heap alloc)  *(Group C)*
`Source/dsp/EQProcessor.cpp:50-73` — `process()` calls `makeLowShelf/makePeakFilter/makeHighShelf`, each of which heap-allocates a ref-counted `Coefficients` object, on every parameter change — i.e. most blocks during automation. RT-safety violation (priority-inversion/dropout risk). **Fix:** assign raw coefficients into `state->coefficients` in place, or swap prepared coefficients from the message thread (the `Coefficients::Ptr` swap pattern from O-AnalogSaturation v1.1.4). See [[critical_oversampled_path_filter_rate]].

### WR-09 — Unescaped scale name/description → HTML/DOM injection  *(Group D)*
`Source/TuningExporter.cpp:392, 398` + WebView `innerHTML` sinks (`tuning-panel.js:297,536`) — `scaleName` from a `.scl` first line is concatenated unescaped into exported HTML `<title>`/`<h1>` and reaches the plugin's own WebView via `innerHTML`, where native functions can write/delete files (CR-04). **Fix:** HTML-encode in `toHTML`; prefer `textContent` over `innerHTML` for backend-supplied names in JS.

### WR-10 — `generateRank2` clamps generator against the un-clamped period  *(Group D)*
`Source/ScaleGenerator.cpp:59-60` — `generatorCents` clamped to `periodCents-1.0` **before** `periodCents` itself is clamped. A period below the 100.0 floor produces a scale that doesn't match the requested period. **Fix:** clamp `periodCents` first.

### WR-11 — `calculateETDeviation` divides by caller-supplied `totalDegrees`  *(Group D)*
`Source/TuningExporter.cpp:118` — `(degree/totalDegrees)*period` with no guard; a `public static` API, so a `totalDegrees==0` external call yields `inf`/`nan`. **Fix:** `if (totalDegrees <= 0) return 0.0;`.

### WR-12 — `reduceAndSort` infinite-loops if `period <= 0`  *(Group D)*
`Source/ScaleGenerator.cpp:87-90` — `while (c < 0) c += period;` never terminates for `period ≤ 0`. Currently unreachable (callers pass positive periods) but one refactor from an audible hang. **Fix:** `if (period <= 0.0) period = 1200.0;` at entry.

### WR-13 — KBM octave-degree read from file not range-validated  *(Group D)*
`Source/TuningEngine.cpp:539, 575` — `newOctaveDegree` stored with only a `>0` check; a wild value flows into KBM math and export. **Fix:** `jlimit(1, 512, …)` alongside the CR-02 fix.

---

## Info (19)

**Group A** — IN-01 `midiNote` shadowing in `noteStarted` (`FormantVoice.cpp:187` vs `225`); IN-02 `processBlock` does ~25 string-keyed APVTS lookups/block instead of caching pointers (`PluginProcessor.cpp:759-816`; not an RT violation but inconsistent with the voice's cached pointers); IN-03 NaN guard resets filters but not excitation sources (`FormantVoice.cpp:739-747` — also reset `glottalSource`/`aspirationNoise` or clamp `finalF0`); IN-04 include-path case mismatch `DSP/` vs on-disk `dsp/` (`PluginProcessor.h:16-18` — breaks on case-sensitive filesystems).

**Group B** — IN-05 mipmap floor → residual aliasing above Nyquist (`LFGlottalSource.h:86-90`); IN-06 VowelMorpher assumes cursor pre-clamped to `[0,1]` (`VowelMorpher.h:24-34`); IN-07 per-sample transcendental coeff recompute during morph transitions (`FormantFilterBank.h:117-129` et al.); IN-08 `updateCoefficients` uses passed `sr` while `process()` uses member `sampleRate` (`FormantFilterBank.h:69,107` vs `125`); IN-09 `AspirationNoise::reset` sets breath to 0.1 not target (`AspirationNoise.h:105` — attack glitch); IN-10 `solveAlpha` Newton iteration can overflow `exp()` offline (`GlottalTableGenerator.cpp:64-98`; sanitized downstream).

**Group C** — IN-11 dead members `tankState`/`prevSize`/`prevDamping` in `ReverbProcessor`; IN-12 reverb diffusion delay not SR-scaled at read time (`ReverbProcessor.cpp:158,205,208`); IN-13 `LyricsEngine::peekCurrent` reads shared array lock-free (safe only by undocumented message-thread-only invariant — add a comment); IN-14 mono delay double-writes the aliased channel pointer (`DelayProcessor.cpp:69,93-94`).

**Group D** — IN-15 unused native-fn registrations with no JS caller (`PluginEditor.cpp:250,283,325,340,352,540`); IN-16 `setSingleIntervalEncoded` is a byte-identical duplicate of `setSingleInterval`; IN-17 non-atomic reads of `pitchBendRange`/`a4Frequency`/`octaveStretch` (`TuningEngine.cpp:896,291-293` — benign scalar race); IN-18 transient frequency-table inconsistency during rebuild (`TuningEngine.cpp:900-917` — self-correcting); IN-19 unused member flags `mtsSynthClientConnected`/`scalaFileLoaded` (`TuningEngine.h:333-334`).

**Housekeeping (not a code defect):** CHANGELOG head is v1.25.0 but PLUGINS.md registry shows 1.24.1 Installed — reconcile the registry/version after the next build.

---

## Suggested fix ordering

1. **CR-03 + CR-05** — both silently corrupt tuning on normal use; smallest, highest-value fixes (append period; drop the `==12` branch). Fix together.
2. **CR-04** — preset-name sanitization (`createLegalFileName`); one helper, applied across five call sites.
3. **CR-02 + WR-13** — clamp `.kbm` header fields; parser hardening.
4. **CR-01** — restore pitch-bend/MPE pitch in the render loop (largest change; verify composition order vs tuning + NE).
5. **WR-08 + WR-02** — RT-safety: EQ coefficient allocation, then the shared NE-buffer overflow (module-wide benefit).
6. **WR-03 → WR-05 → WR-06** — the NaN/denormal robustness chain in the formant DSP.
7. **WR-01 / WR-07** — headless-render tuning restore; delay clamp at high SR.
8. Info items — sweep opportunistically (IN-04 case mismatch is a cheap CI-safety win; IN-16/IN-15/IN-11/IN-19 are dead-code deletions).

_Per-group detail: `.planning/code-review/GROUP-{A,B,C,D}-*.md`_
