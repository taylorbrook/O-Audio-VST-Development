# O-Formant Changelog

All notable changes to O-Formant will be documented in this file.

## [1.26.0] - 2026-08-30

### Added — The PAGE speaks French (Stage K batch K4, canon v2)

Every visible string on the interface is now owned by a key table with an English and a French rendering, selectable from a new gear popover in the header and remembered with the session. This localizes the plugin's EXISTING text; it does **not** author hover-help prose (`TIP_BINDINGS` is `[]`, and hover help is a later stage).

**What was localized — measured, not estimated.** 94 LABEL text nodes and 5 attributes from `node scripts/i18n-extract.js --plugin O-Formant`, plus 4 JS-written strings, plus 35 strings in `js/tuning-panel.js` that the extractor never reports (`scripts/i18n-extract.js:442` skips that filename unconditionally, with no ownership test). 119 `LABELS` keys and 5 `I18N` keys in total, every French entry `reviewed: false` — no native speaker has read it.

- **`Source/ui/public/js/i18n.js` (new)** — `LANGUAGES`, `I18N`, `LABELS`, `I18N_EXEMPT`, `TIP_BINDINGS`, `tr()`. Embedded in `juce_add_binary_data` SOURCES **and** served from a `getResource()` branch in `PluginEditor.cpp`; a file embedded but not served is a 404 that presents as a blank panel and nothing else.
- **The canon v2 runtime block** copied verbatim from `scripts/i18n-canon.js` into `js/main.js`, called from the existing `DOMContentLoaded` handler inside `try`/`catch`.
- **C++ language pair** — `getUiLanguage` / `setUiLanguage` native functions and a `std::atomic<int> uiLanguage` riding the APVTS state tree as a plain `"uiLanguage"` property, saved as `"en"`/`"fr"`. Deliberately not an `AudioParameterChoice`: it must not appear in a DAW automation lane, and a preset must not change which language somebody reads. Restored with an `isVoid()` guard and `toString()`, because a non-parameter property round-trips through XML as a **string** `var`.
- **`plugins/O-Formant/tests/i18n-states.json` (new)** — 15 states so the label gate can reach the effects, lyrics and tuning tabs, the popover, the five visualisation modes and the three generator forms.

**The tuning panel is IN SCOPE, and that widens a divergence on purpose.** `Source/ui/public/js/tuning-panel.js` is O-Formant's own copy — its header says so, and it is 45 lines diverged from `modules/tuning/scala-tuning-engine/js/tuning-panel.js`. O-Formant has no `dependencies.json` listing that module, so `/module-upgrade` will not revert this. Its ~35 captions are now keyed inside the templates that inject them, re-swept by a `localize()` method after every `innerHTML` rebuild. **The module's own copy is untouched**; this deliberately widens the divergence rather than leaving the Tuning tab speaking English inside a French plugin.

**Canvas text.** Five `ctx.fillText` sites. Two paint prose and are localized through `I18N` (the `LYRICS` badge on the vowel pad, and `plosive`/`fricative`/`mixed` in the consonant pad readout); three paint notation and are exempt — the IPA vowel and consonant glyphs and the `F1`..`F5` formant markers. **No gate can see any of this**: assertion 10 walks text nodes, assertion 12 scans `textContent` writes, and `fillText` is neither, so leaving them in English passes green. Verified with a `fillText`-recording probe, en→fr→en, at three consonant-pad positions; negative-controlled by restoring one hard-coded English literal, which the probe caught and `check-i18n --strict-v2` did not.

**D-01 arm 1, verified verbatim against the parameter layout.** `Cascade` / `Parallel` / `Hybrid` are `formantTopology` `AudioParameterChoice` options and `Normal` / `PingPong` are `delayMode` options, byte-identical on the page. All five stay English so the page and the host automation lane agree.

### Fixed — a pre-existing ENGLISH layout defect, exposed by keying the tuning panel

The octave-stretch readout in the Tuning tab rendered **nine pixels past the right edge of the 800 px plugin frame** and was clipped, in English, in every shipped build up to and including v1.25.4. `.octave-stretch-slider` used `flex: 1`, which leaves `min-width` at `auto`, and an `<input type=range>`'s intrinsic minimum is 129 px; the row's content box is 184 px and its content summed to 215 px, so the row overflowed and `#octave-stretch-value` landed at x=779..809. Measured at v1.25.4 before any French existed. `min-width: 0` lets the slider absorb the slack; the readout now sits at x=748..778 in both languages.

### Fixed — the syllable-count caption was about to be repainted in the counter's font

Splitting `Syllables <span id="lyrics-counter">` into two spans (contract §5) brought the caption under the loose `.lyrics-syllable-label span` rule, which is the counter's Courier-green styling. The rule is now scoped to `#lyrics-counter`.

### Changed — five geometry pins, each negative-controlled

French moved geometry in five places; each pin was removed alone and confirmed to re-break assertion 7 on exactly the elements it holds. None is decoration.

| Pin | What moved without it | Delta |
|---|---|---|
| `.preset-save-btn { min-width: 65px }` | `#preset-category`, `#preset-prev`, `#preset-name`, `#preset-next` — the bar is `justify-content: center` | dx −8.6 |
| `.tonic-label { min-width: 40px }` (+ row gap 8→4 px to return the space) | `#tonic-down`, `#tonic-value`, `#tonic-up` | dx +6.3 |

| `.octave-stretch-label { min-width: 51px }` | `#octave-stretch` | dx +11.0, dw −11.0 |
| `.lyrics-btn { min-width: 66px }` | `.lyrics-controls`, the Enable toggle and its thumb — `.lyrics-header` is `justify-content: space-between` | dx −20.0 |
| `.lyrics-syllable-label span[data-i18n] { min-width: 75px }` | `#lyrics-counter` — the one site where **French is SHORTER** | dx −8.1 |

Two French captions were shortened rather than pinned, because pinning them would have moved English much further: the preset button reads `Sauver` (not `Enregistrer`, which moved four siblings 25 px) and the lyrics reset button reads `Réinit.` (not `Réinitialiser`, which moved four elements 59 px). The full phrasing survives on the accessible names and in the Save prompt, neither of which has a box to fit.

### Changed — native `title=` deleted (contract §4)

A native `title` renders a second, untranslated OS tooltip. Four markup attributes moved their text verbatim to `data-i18n-aria` (`Previous preset`, `Next preset`, `Toggle loop`, `Reset to first syllable`). The fifth, a per-chip `chip.title = 'Syllable N: ...'` written from `renderSyllables()`, was **deleted outright rather than moved**: the chip's own text already reads the phonemes and `#lyrics-counter` already reads the index, so it carried nothing new, and an `aria-label` would have overridden the phonemes a screen reader otherwise reads straight off the chip. `boot-all-uis` now reports `title= 0` for this plugin.

### Not fixed, deliberately

- **`prompt('Save preset as:')`** is localized but the call itself is left alone. JUCE's `WebBrowserComponent` does not implement the WebKit text-input-panel delegate on macOS, so `window.prompt` is likely to return `null` in the plugin and Save may never have worked outside the browser harness. That is a functional defect, not an i18n one, and replacing it needs a native dialog.
- **Note names (`C`, `C#`, …) and embedded tuning names** stay English. They are pitch-class notation and engine data respectively; the French octave numbering for A4 is La3, so renaming them would silently change what the `.scl`/`.kbm` files are written against.
- **`#tonic-value` renders `undefined`** in the headless harness because the generic stub answers `getTonicNote` with a shape `noteNames[]` cannot index. Harness artefact; the plugin returns an int.

**Testing:** `check-i18n --strict-v2` and `check-ui-labels` both green, `boot-all-uis` 43/43 clean, build (VST3 + AU) + `auval`. The `fillText` probe and all six negative controls are described above.

## [1.25.4] - 2026-07-01

### Changed — Low-risk Info-item sweep from REVIEW.md (IN-01, IN-04, IN-05, IN-09, IN-11, IN-12, IN-13, IN-14, IN-15, IN-16, IN-17, IN-18, IN-19)

Mechanical, non-behavioral cleanup pass over the code-review Info items. No change to the emitted audio on valid input; the audio-path items whose "fix" would alter the sound are documented in-place rather than changed (flagged below).

**Applied (mechanical / dead-code / non-behavioral):**

- **IN-04 — include-path casing (Linux/CI safety).** `PluginProcessor.h` included `"DSP/DelayProcessor.h"` / `"DSP/EQProcessor.h"` / `"DSP/ReverbProcessor.h"` (uppercase) while the git-tracked files live under `Source/dsp/`; `CMakeLists.txt` listed the same three sources as `Source/DSP/*.cpp`. Both compile on case-insensitive macOS/Windows but fail to resolve on a case-sensitive filesystem. Normalized to lowercase `dsp/` in both the header includes and the CMake source list, matching every other include.
- **IN-16 / IN-15 — removed duplicate + unused native-fn registrations.** Deleted the byte-identical `setSingleIntervalEncoded` duplicate of `setSingleInterval` (IN-16) and five further `withNativeFunction` registrations with no JS caller (verified against all `getNativeFunction` sites): `setTuningIntervals`, `getMasterTune`, `setTemperamentPreset`, `getTemperamentPreset`, `getEmbeddedTuningCategories`. No control is affected (the dangerous gap is the reverse — a JS call with no C++ backing — and every JS-called native remains registered).
- **IN-11 — removed dead `ReverbProcessor` members.** `tankState` (zeroed in prepare/reset, never read), `prevSize` and `prevDamping` (set to `-999` in prepare, never compared — damping recomputes unconditionally; size is gated by the separate `prevSizeForDelays`). `prevMix` is genuinely used and kept.
- **IN-19 — removed dead `TuningEngine` "future expansion" flags.** `mtsSynthClientConnected` (never written or read) and `scalaFileLoaded` (written in three places, never read), plus the three now-dead assignments.
- **IN-01 — renamed shadowed local.** In `FormantVoice::noteStarted`, the inner `float midiNote` in the Rd-init block shadowed the outer `int midiNote`; renamed to `midiNoteF` (both held the same note number — behavior unchanged).
- **IN-13 — corrected a misleading thread comment.** `LyricsEngine::peekCurrent` was labelled "Called from audio thread"; its sole caller is the editor lyrics-poll on the **message thread**. Comment now documents the message-thread-only invariant that makes the lock-free read safe.
- **IN-14 — guarded the mono delay double-write.** In a mono block `rightData` aliases `leftData`, so `leftData[i]=wetL; rightData[i]=wetR;` clobbered left with the right line's output. Guarded the second write with `getNumChannels() > 1`. Provably non-behavioral today (both lines get identical input/time → `wetL == wetR`); makes the intent explicit.

**Documented in place (behavioral fix deferred to preserve the sound; say the word to apply):**

- **IN-05** — `LFGlottalSource` mipmap uses `floor` + crossfade, leaking mild residual aliasing above Nyquist. Biasing to the safe level would change the source timbre.
- **IN-09** — `AspirationNoise::reset` seeds breath to `0.1`, so breath ramps `0.1 → target` at every note-on (small attack-time sweep). Resetting to the current target would remove it but alters the onset.
- **IN-12** — reverb input-diffusion read uses the raw sample constant (fixed in samples, not SR-scaled like the tank delays); SR-scaling it would shift the diffusion colour at non-44.1 kHz rates.
- **IN-17** — `pitchBendRange` / `a4Frequency` / `octaveStretch` are plain scalars with a benign message↔audio read race (no torn read on target ISAs; worst case one-block-stale). Left non-atomic.
- **IN-18** — `rebuildFrequencyTable` stores its 128 entries individually, so a note started mid-rebuild can read a transiently mixed table — momentary and self-correcting.

**Testing:** Build (VST3 + AU) + auval + pluginval. All changes are mechanical (casing, dead-code removal, rename, comments) or a provably non-behavioral guard (IN-14); no DSP behavior changes.

## [1.25.3] - 2026-07-01

### Fixed — State round-trip + delay buffer sizing (REVIEW.md WR-01, WR-07)

- **Headless reload rendered in default 12-TET at A=440 (WR-01).** `getStateInformation` persisted the built-in temperament (`"preset"`) and the master-tune / octave-stretch / pitch-bend-range APVTS values, but `setStateInformation` never pushed any of them into `TuningEngine` — they only reached the engine through editor-only WebView native-function callbacks (`PluginEditor.cpp:314-353`). Reloading a session and bouncing offline **without opening the UI** left the engine at its construction defaults (A=440, stretch=1.0, Equal 12-TET), so a project saved with e.g. Werckmeister III at A=442 rendered mistuned. Fix: in `setStateInformation`, after `replaceState`, push the restored `tuning_masterTune` / `tuning_octaveStretch` / `tuning_pitchBendRange` values and the saved built-in preset straight into the engine. The built-in preset is applied **before** the saved custom intervals so a custom `.scl` still wins when one was loaded (`setBuiltInPreset(Custom)` is a no-op on intervals, so the custom-scale path is unaffected). Tuning mode is a derived side effect of the interval/preset setters (the `tuning_tuningMode` param is never wired to `setMode`), so it is restored implicitly — matching the editor-open behaviour exactly.
- **Delay buffer under-sized for 2.0 s above 96 kHz (WR-07).** `DelayProcessor`'s lines are fixed at 192000 samples but `delayTime` allows up to 2.0 s and `setTime` never clamped, so `seconds * sampleRate` exceeded the buffer above 96 kHz (2.0 s = 211680 samples at 105.84 kHz, > 192000 at 176.4/192 kHz for times above ~1.09/~1.0 s). In JUCE 8.0.9 Release `popSample` masks the read index by `% totalSize` (no OOB read) but the delay **silently aliases to a wrong, much shorter time**, and a Debug `jassert` fires. Fix: `setTime` now clamps the requested delay to `delayL.getMaximumDelayInSamples()`. No change at ≤96 kHz where 2.0 s already fits.

**Testing:** Build (VST3 + AU) + pluginval. The delay clamp is a no-op below 96 kHz; the state fix is only observable on headless/offline reload (no behavioural change when the editor is open, which already pushed these values).

## [1.25.2] - 2026-07-01

### Fixed — NaN / denormal robustness in the formant DSP (REVIEW.md WR-03, WR-04, WR-05, WR-06, IN-03, IN-06)

This closes the NaN/denormal robustness chain flagged in the code review (fix-ordering item 6). None of these change the sound on valid input — they harden the voice against the poisoned-state cascade where one bad value silences a formant (or the whole voice) until the next block-rate update.

- **VowelMorpher IDW weight could overflow to Inf → NaN formant frequencies (WR-03).** `compute` did `1.0f / std::pow(dist, focus)` with no lower bound on `dist` and no cap on the weight. Just above the `1e-6` snap epsilon with a large focus, `pow` can underflow toward 0 → weight `Inf` → `weightSum Inf` → `invSum 0` → `weight = Inf*0 = NaN` → NaN formant frequencies, which then poison the biquads (WR-05). Fix: floor `dist` to `1e-3f`, cap each weight to `1e12f`, and guard `weightSum` finite/`> 0` before dividing (equal-blend fallback otherwise). The vowel nearest the cursor still dominates, so near-snap morphing is audibly unchanged.
- **VowelMorpher assumed a pre-clamped cursor (IN-06).** `compute` now clamps `cursorX`/`cursorY` to `[0,1]` on entry. Lyric syllable targets and the MPE-timbre `vowelY` offset are not guaranteed to pre-clamp, and an out-of-range cursor skews the weights.
- **LFGlottalSource phase wrap was a single `if` → out-of-bounds wavetable read (WR-04).** `getNextSample` corrected only `[1.0, 2.0)`; if one increment advanced phase ≥ 2.0 (`f0 ≥ sampleRate`, reachable at low sample rates with high notes) the post-wrap phase stayed ≥ 1.0 and `samplePos` over-read the 2049-sample frame. Fix: `while`-loop the wrap **and** clamp `phaseIncrement` to `≤ 0.5` (the wavetable's Nyquist) in `setFrequency` — phase is now guaranteed to stay in `[0,1)`.
- **FormantBiquad NaN guard left poisoned coefficients → sticky silence (WR-05).** The `processSample` guard reset the state (`z1=z2=0`) but not the coefficients; NaN/Inf `b0..a2` (e.g. from WR-03) re-tripped the guard on every subsequent sample, turning a one-sample transient into a persistent per-formant dropout until the next valid update. Fix: `setCoefficients` now validates all six incoming coefficients with `std::isfinite` and keeps the last-known-good set if any is non-finite.
- **No denormal flushing in the per-voice feedback state (WR-06).** The resonator tails (`r` up to ~0.9999 in the cascade / nasal banks) relied entirely on the caller's `ScopedNoDenormals`; if a voice ever renders outside that scope, x86 traps on denormals → CPU spike. Fix: documented the guarantee (`juce::ScopedNoDenormals` in `PluginProcessor::processBlock` wraps every `renderNextBlock`; a note at the voice entry warns any future direct caller), and added a cheap add-then-subtract denormal flush to `FormantBiquad::processSample` (covers all five formant banks + nasal pole-zero) and to the `AspirationNoise` tilt one-pole. Normal-range state is unchanged; subnormals round to exactly 0.
- **Voice NaN guard reset the filters but not the excitation sources (IN-03).** The `renderNextBlock` NaN/Inf guard reset the filter banks but left `glottalSource`/`aspirationNoise` running, so a NaN originating in the source kept re-injecting garbage and the guard never cleared. Fix: the guard now also resets `glottalSource`, `aspirationNoise`, and `fricationBank`.

## [1.25.1] - 2026-07-01

### Fixed — Tuning correctness (REVIEW.md CR-03, CR-05)

- **All 24 embedded factory tunings loaded mistuned (CR-03).** `loadEmbeddedTuning` (PluginEditor.cpp) passed `EmbeddedTuning::intervals` straight to `setCustomIntervals`, but that field *excludes* the period (which lives in the separate `->period` member). `setCustomIntervals` derives `scaleDegrees = size() - 1` and treats the last element as the period, so every embedded tuning was off by one degree — the period was silently dropped and the top interval was reinterpreted as the octave. Fix: append `tuningData->period` to a local copy before the call, matching the built-in-preset path in `TuningEngine::setBuiltInPreset` (TuningEngine.cpp:172-178).
- **Editing any 12-value scale silently reset it to 12-TET (CR-05).** `TuningEngine::setSingleInterval` gated its "initialize to 12-TET" branch on `scaleIntervals.size() < 2 || scaleIntervals.size() == 12`. The `== 12` arm wiped real 12-value scales (11-EDO, embedded 12-note tunings — all of which store exactly 12 entries) to 12-TET on the very first interval edit. Fix: gate on `size() < 2` only, so the reset fires solely for effectively-empty (unison-only) scales.

### Fixed — Preset name sanitization / path traversal (REVIEW.md CR-04)

- **Preset name used verbatim as a filename (CR-04).** `OuariconPresetManager` built every preset path as `getChildFile(name + ".json")` with no sanitization, so a user-supplied name flowed straight into the filesystem. Two failure modes: a `/` in the name ("Koto / Harp") was treated as a subdirectory separator — the save silently failed while the UI still showed the preset as saved (the documented `critical_preset_name_slash_path_separator` gotcha); and a `..`-bearing name was an arbitrary-path **write** (`savePreset`), **read** (`loadPreset` / `loadPresetFromCategory`), and **delete** (`deletePreset`) primitive that also bypassed the `isFactoryPreset` overwrite guard. Fix: added a `sanitizePresetName` helper (`juce::File::createLegalFileName().trim()`) that strips path separators and other illegal filename characters — collapsing any traversal attempt into one harmless literal segment — and bails on an empty result. Applied to the preset name (and the `category` argument) across `savePreset`, `loadPreset`, `loadPresetFromCategory`, `deletePreset`, and `isFactoryPreset`. `currentPresetName` is now tracked as the sanitized name so it stays consistent with the on-disk filename that `getPresetList` reports.

### Fixed — Pitch bend / MPE per-note pitch was dead (REVIEW.md CR-01)

- **Pitch bend and MPE per-note pitch produced zero pitch change (CR-01).** `FormantVoice::notePitchbendChanged` was an empty stub whose comment claimed pitch bend was "handled per-sample via `getCurrentlyPlayingNote().getFrequencyInHertz()`" — but `renderNextBlock` never read per-sample note pitch. Sounding pitch came solely from `pitchGlide.getNextFrequency()`, whose target is set once at note-on, and `PitchGlide` has no bend input; the wheel and MPE per-note pitch (legacy MPE, ±2 st) moved nothing. The primary microtonal path (`TuningEngine` + Dorico Note-Expression) was unaffected — only bend was dead. Fix: `renderNextBlock` now computes a live bend ratio once per block — `getCurrentlyPlayingNote().getFrequencyInHertz()` (which folds in channel/master + per-note pitchbend) divided by the bend-free reference (`440·2^((initialNote−69)/12)`; the A=440 basis cancels in the ratio) — and folds it into the glide target as `pitchGlide.setTarget(tunedF0 · bendRatio)`. Because `tunedF0` already carries tuning + NE, the three now stack multiplicatively; `bendRatio == 1.0` when the wheel is centred, so the tuning/NE/glide path is unchanged bit-for-bit. Computing once per `renderNextBlock` is exact (not coarse): `MPESynthesiser` sub-block-splits the buffer at each bend event, so the note's pitchbend is constant within a single call. This makes the v1.25.0 "MPE pitch-bend stacks on top" changelog note actually true. The `notePitchbendChanged` stub comment was corrected.

### Fixed — Real-time safety (REVIEW.md WR-08, WR-02, IN-02)

- **EQ recomputed IIR coefficients with a heap allocation on the audio thread (WR-08).** `EQProcessor::process` called `juce::dsp::IIR::Coefficients::makeLowShelf` / `makePeakFilter` / `makeHighShelf` on every changed block (i.e. most blocks while an EQ knob is automated). Each of those factories does `*new Coefficients(...)` — a ref-counted heap allocation on the RT thread, a priority-inversion / dropout risk. Fix: switched to the sibling `juce::dsp::IIR::ArrayCoefficients::makeXXX` factories, which return a stack `std::array<float,6>` (identical coefficient math, zero heap), and assign it into the existing `*state` in place. `prepare()` uses the same array path so each `Coefficients` array reaches full storage on the message thread; the in-place assignments in `process()` then reuse that storage and never reallocate. Bit-for-bit identical filter response — only the allocation is removed. See `critical_oversampled_path_filter_rate`.
- **Note-Expression raw-event buffer could reallocate on the audio thread (WR-02).** `Ouaricon::NoteExpression::VST3Extensions::onVst3RawEvent` (shared `note-expression` module) `push_back`ed into a `reserve(64)`'d vector; a block carrying >64 raw NE events (dense Dorico divisi at tiny buffers) would grow the vector — a heap allocation on the RT thread. Fix (upstream, in `modules/tuning/note-expression`): push only while `size() < capacity()`, dropping the overflow instead of reallocating. Dropped deltas fall back to the previous per-pitch offset for that block. Module bumped **1.1.0 → 1.1.1**; header-only, no API/ABI change, benefits all 8 consumer plugins on their next build.
- **~22 string-keyed APVTS lookups per block in the effects chain (IN-02).** `processBlock` fetched every chorus/delay/reverb/EQ/output parameter via `parameters.getRawParameterValue("id")` (a hashed name lookup) on every block. Fix: cache the `std::atomic<float>*` pointers once in `prepareToPlay` (`cacheParamPointers()` → `fxParams` struct) and read `->load()` directly in `processBlock`, matching how the voice already caches its pointers. Not an RT-safety violation on its own (no allocation), but removes ~22 hash lookups per block.

### Technical Notes
- **Non-breaking.** No parameter IDs, ranges, defaults, or state format changed. Existing presets, sessions, and automation load identically. Preset names that were already legal filenames (all normal names — spaces are legal) sanitize to themselves, so existing user presets remain loadable.
- The tuning bugs originate in the shared `scala-tuning-engine` embedded-tuning path; CR-03 is the documented recurring "embedded tunings load without their period" pattern.
- CR-04 fix lives in `Source/OuariconPresetManager.h` (O-Formant's local copy). `loadPresetFromFile` is unaffected — its name is derived from an already-resolved `juce::File`, not a user string.
- CR-01 fix is confined to `Source/FormantVoice.cpp` (`renderNextBlock` + `notePitchbendChanged`). Spectral tilt and source-filter-coupling still key off the unbent `tunedF0` (bend's effect on a 2·f0 tilt cutoff and harmonic-proximity is negligible) — deliberately out of scope to avoid touching the NE path.
- The WR-02 fix is in the shared `note-expression` module, not O-Formant's tree. The other 7 consumer plugins (O-Lyrica, O-Bells, O-Prism, O-Wind, O-IntonationPad, O-Reed, O-Bowed) reference the module directly and pick up the fix on their next rebuild; their installed binaries are unchanged until then.

## [1.25.0] - 2026-04-26

### Added — VST3 Note Expression Microtonal Support for Dorico

adds VST3 Note Expression microtonal support for Dorico. O-Formant responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events). The voice's cached `tunedF0` composes Dorico's NE delta multiplicatively after `TuningEngine::getFrequency` and before `PitchGlide` / glottal source frequency assignment, so the glottal source `LFGlottalSource` samples the correct fundamental from sample 0 (no attack zipper). Downstream consumers of `tunedF0` in `renderNextBlock` — spectral tilt (line ~488) and source-filter coupling (line ~616) — all see the tuned value. ConsonantEngine articulation is independent of pitched fundamental and remains intelligible at microtonal shifts. MPE pitch-bend stacks on top via `getCurrentlyPlayingNote().getFrequencyInHertz()` (per-sample lookup unaffected by NE). End users must set Microtonality to "VST3 Note Expression" on the Dorico expression map.

### Technical Notes

- **Composition order** (D-10 with MPE): `TuningEngine::getFrequency(midi)` → `applyPendingTuning(table, midi, tunedF0)` → `pitchGlide.snapTo/setTarget(f0)` → glottal source uses `pitchGlide.processSample()` per-sample. NE applies once per `noteStarted` (slot consumed via `exchange(0.0)` semantics); MPE pitch-bend updates compose multiplicatively per-sample on top via the existing pitchGlide / vibrato / jitter chain.
- **MPE pitch source for NE correlation**: `int midiNote = currentlyPlayingNote.initialNote` (the noteOn MIDI pitch). The shared module's `updatePendingFromEvents` correlates by `noteId` regardless of MPE channel.
- **Per-call-site composition** (vs helper-based in O-Reed/O-Bowed): `FormantVoice` has no `getBaseFrequencyFromTuning` helper to wrap. The `tunedF0` cached field is assigned in `noteStarted()` and consumed by the per-sample `pitchGlide`. NE is applied at the single assignment site in `noteStarted()` immediately after the `TuningEngine` query and before `pitchGlide.snapTo/setTarget`.
- **Files Modified:** `CMakeLists.txt` (added `include(OuariconModules.cmake)` + `ouaricon_add_module(O-Formant note-expression)` — note: O-Formant was the only Phase 24 plugin missing the module-system include), `Source/PluginProcessor.{h,cpp}`, `Source/FormantVoice.{h,cpp}`. Version: 1.24.2 → 1.25.0.

### Based On

- Phase 23 textbook reference (O-Lyrica v2.3.0 module extraction).
- Phase 24 patterns 24-01..24-06 (canary + 6 propagation waves established the 8-file atomic-commit playbook + composition order conventions).

## [1.24.2] - 2026-04-19

### Fixed
- **Lyrics mode: fricative consonants (/sh f s th hh/) now sound at note-on instead of mid-note.** Previously, in syllables like "SH IY", the SH was perceived as triggering at the *end* of the note rather than the beginning. Plosives were unaffected (their 1 ms attack already produced sharp onsets); the bug was specific to fricatives.
- **Root cause:** Two compounding issues in `ConsonantEngine::getNextSample`:
  1. The Klatt-style consonant envelope's attack scaled steeply with manner — `1 + manner * 39` ms — so for fricatives (manner=1.0) the envelope took 40 ms to ramp from 0 to 1.0. With a 140 ms total envelope (40 attack / 60 hold / 40 decay), the perceived energy peak landed at 40–100 ms after note-on.
  2. The output multiplier `output *= advanceEnvelope()` applied the envelope to *both* the continuous frication noise *and* the burst transient. The burst (which represents the immediate aerodynamic onset and starts at full amplitude at sample 0) was therefore silenced by the envelope=0 ramp at note-on, eliminating the only fast cue that could have signaled "consonant happens here".
- For notes shorter than ~150 ms, the consonant peak fell at or after note-off — exactly matching the user-reported "SH at the end" perception.

### Technical Notes
- **Non-breaking.** No parameter IDs, ranges, or defaults changed. Existing presets and automation load identically.
- `envAttackMs` slope reduced from `1 + manner*39` to `1 + manner*7` (Stevens 1998: natural fricative onsets reach plateau in 5–15 ms; 8 ms attack at manner=1.0 matches that range while preserving a click-free ramp).
- `envHoldMs` lengthened from `manner*60` to `manner*90` to keep the total fricative duration close to the previous ~140 ms (now ~138 ms) so the consonant doesn't perceptually shorten.
- Burst component moved *outside* the envelope multiplier — applied directly to `output` after the continuous component is enveloped. The burst already has its own intrinsic decay (`exp(-cachedBurstDecayRate * progress)`) that shapes its amplitude.
- Plosives unaffected: their attack was already 1 ms, and the burst-bypass change only matters when the envelope is non-1.0 in the first ~8 ms (which it now isn't for fricatives either).

### Based On
- Stevens, K. N. (1998). *Acoustic Phonetics*, MIT Press — fricative amplitude rise times.
- Code-trace investigation (Tier 2). No new literature beyond reference confirmation.

## [1.24.1] - 2026-04-17

### Fixed
- **Plosive consonants (/k/, /t/, /p/) now trigger consistently on note-on** — previously inconsistent, especially after fricatives like /s/. Fricative consonants were unaffected because their dominant sustained-noise component used updated coefficients; plosives rely entirely on the 8 ms burst which used stale values.
- **Root cause:** In `FormantVoice::noteStarted()`, `consonantEngine.triggerBurst()` and `fricationBank.snapToTargets()` ran BEFORE `renderNextBlock()` could update coefficients with the new note's parameters. This caused three stale-cache failures:
  1. `burstSamplesRemaining = cachedBurstDuration` was set from the previous syllable. After fricative (80 ms, manner=1) → plosive (8 ms, manner=0), the first-sample progress calculation became `1 - 3528/353 = -9.0`, causing `exp(-12 × -9) = exp(108)` overflow — the burst envelope clipped to max amplitude for ~72 ms, masking the plosive character.
  2. The VOT/aspiration trigger condition `cachedVoicing < 0.5 && cachedManner < 0.3` was evaluated against stale values, so voiceless plosives following fricatives never got their aspiration phase (the 40–80 ms noise through the cascade bank that makes /k t p/ sound distinctly plosive).
  3. `fricationBank.snapToTargets()` snapped smoothed amplitudes to the previous syllable's place target, so the 8 ms burst was filtered by ~"s"-like F6F @ 6 kHz amplitudes instead of the current consonant's target.

### Technical Notes
- **Non-breaking.** No parameter IDs, ranges, or defaults changed. Existing presets and automation load identically.
- Fix is ~35 LOC in `FormantVoice::noteStarted()`: priming block reads the new syllable's consonant parameters (from `currentSyllable` in lyrics mode or APVTS otherwise) and calls `consonantEngine.updateCoefficients()`, `setVOTScale()`, `setManualEnvelope()` (when auto off), and `fricationBank.setPlace()` BEFORE `snapToTargets()` and `triggerBurst()`.
- The existing `renderNextBlock()` block-rate updates at lines 415–426 are still in place; they're now redundant for the first block after note-on but remain necessary for subsequent blocks when parameters change mid-note.

### Based On
- Investigation via root-cause code trace (Tier 2). No new literature.

## [1.24.0] - 2026-04-16

### Added
- **Locus-based F2/F3 formant transitions** (Delattre-Liberman-Cooper 1955) — for 40–50 ms after note onset, when a consonant is active (consonantLevel > 0.1 at onset), F2 and F3 are biased toward place-specific loci and decay exponentially (τ=15 ms per Kewley-Port 1982) toward the vowel morpher target. This provides the dominant place-of-articulation cue that listeners use when bursts are masked or ambiguous, and was previously missing entirely.
  - F2 locus table: Labial 720 Hz → Alveolar 1800 Hz → Palatal 2200 Hz → Velar back-V pinch 1200 Hz (continuous piecewise-linear interpolation across the place axis).
  - F3 locus table: Labial 2000 Hz, Alveolar 2700 Hz, Palatal 3000 Hz, Velar 2200 Hz.
  - Block-rate application (every 32 samples) inside `FormantVoice::renderNextBlock` immediately after `vowelMorpher.compute()` — so existing per-formant smoothing (`transitionTime`), Singer's Formant clustering, breathiness/nasal BW scaling, and source-filter coupling all ride naturally on top of the post-biased frequencies.
- **`consonantTransition` parameter** (0–1, default 0.5) — user scale on the locus pull. 0.0 = legacy behavior (no locus bias, equivalent to v1.23.0). 0.5 = half-strength DLC locus. 1.0 = full Delattre-Liberman-Cooper locus at onset.
- **Transition knob in UI** — added to the consonant envelope controls row next to Atk/Hold/Decay.

### Technical Notes
- **Non-breaking, additive.** All existing APVTS parameter IDs, ranges, and defaults are unchanged. Existing presets and automation load identically. The default 0.5 produces audible but subtle locus onsets; users set to 0.0 to restore exact v1.23.0 formant trajectories.
- Trigger decision is made once at `noteStarted()` (reads `consonantLevel` and `consonantTone` via lyrics-aware priority). The transition does not retrigger mid-note.
- Effective weight `w = exp(-t/τ) * consonantTransition` interpolates `formantFreqs[1]` and `formantFreqs[2]` from their post-vowel-morph values toward the loci. At `t=50 ms`, `exp(-50/15) ≈ 0.036`, contributing <2% pull even at full amount, so the transition naturally fades into the standard vowel formant path.
- Implementation is entirely inside `FormantVoice` (no new files) plus one APVTS entry and one WebView relay/attachment pair. ~110 LOC C++, ~6 LOC UI wiring.
- No interaction hazards verified for `transitionTime` (per-formant SmoothedValue — still smooths toward the biased target) or Singer's Formant (clustering runs on post-locus F3 frequency so the effect is preserved relative to wherever F3 currently sits).

### Based On
- Research doc: `improvements/consonant-realism-research.md` Item 5
- Delattre, Liberman & Cooper (1955). Acoustic loci and transitional cues for consonants. JASA 27:769.
- Kewley-Port (1982). Measurement of formant transitions in naturally produced stop CV syllables. JASA 72:379.

## [1.23.0] - 2026-04-16

### Added
- **Klatt frication formant bank** (`dsp/FricationFormantBank.h`) — 3 parallel BPFs at fixed Klatt frequencies (F3F=2500 Hz Q=2.0, F4F=3500 Hz Q=1.8, F6F=6000 Hz Q=2.5) plus a flat bypass path for weak fricatives /f v θ ð/. Uses eSpeak-NG alternating-sign summation for inter-formant notches without explicit anti-resonators. Continuous place axis drives a 5-anchor piecewise-linear amplitude table (Labial=bypass / Alveolar=F6F dominant / Post-alveolar=F3F dominant / Velar=F3F+F4F mid).
- **Stevens-Blumstein plosive burst spectral templates** (in `ConsonantEngine::applyBurstTemplate`) — place-dependent burst coloration via LP@800Hz (labial /p b/ diffuse-falling), HP@3.5kHz (alveolar /t d/ diffuse-rising), and BP Q=3 @2kHz (velar /k g/ compact). Crossfaded along the place axis; activates only for plosives (manner<0.3) with smooth blend to the generic shape at higher manner values.
- **VOT / aspiration phase** for voiceless stops — when voicing<0.5 AND manner<0.3, an aspiration window of `(1-voicing)*80+5 ms` is inserted between the burst and voice onset. Aspiration noise is routed through the cascade bank (not the frication bank) so it is shaped by the opening vocal tract toward the following vowel (Klatt AH behavior). Glottal source is fully suppressed during aspiration.
- **Klatt MOD voiced-fricative noise gating** — when voicing>0.5 AND manner>0.3, noise is multiplied by a 50% square gate synced to F0 (`phase<0.5 ? 1.0 : 0.5`). Makes /z/, /ʒ/, /v/, /ð/ distinct from voiceless + voice-bar bleed.
- **`consonantVOT` parameter** (0–1, default 0.5) — user scale for aspiration duration. 0.0 = no aspiration (suppresses VOT phase entirely), 0.5 = nominal Klatt VOT, 1.0 = 2× nominal.

### Technical Notes
- **Non-breaking:** all existing parameter IDs and ranges are unchanged. Existing presets and automation continue to load identically. New `consonantVOT` defaults to 0.5 (nominal VOT) which matches the prior fixed 25 ms onset behavior closely enough that preset character is preserved while adding the new realism when voicing/manner enter voiceless-plosive territory.
- Aspiration noise routing uses the existing `AspirationNoise` module's pipeline (aspiration injected before cascade-bank processing), avoiding a separate noise generator.
- `FricationFormantBank` replaces `filterBank.process()` for consonant noise in Cascade (topology 0) and Hybrid (topology 2) branches of `FormantVoice::renderNextBlock` (lines 570-580). Parallel legacy topology (1) is unchanged for back-compat.
- ~360 LOC total, 1 new file. No new dependencies.

### Based On
- Research doc: `improvements/consonant-realism-research.md` (Klatt 1980; Stevens & Blumstein 1978/79; Lisker & Abramson 1964; KlattGrid; eSpeak NG klatt.c)

## [1.22.0] - 2026-04-13

### Improved
- **Lyrics mode XY pad animation** — When lyrics mode is active, vowel and consonant XY pads on the Synth tab now show the dot moving to each syllable's target position as notes are played
- **XY pads disabled during lyrics mode** — Pointer interaction is blocked on both pads when lyrics are driving articulation, preventing accidental overrides
- **Visual lyrics mode indicator** — XY pads show "LYRICS" label, dimmed overlay, and deeper green dot/crosshair color when in lyrics-driven mode
- **Extended `getLyricsPosition()`** — Native function now returns current syllable's vowelX, vowelY, consonantTone, and sibilance for UI animation

## [1.21.0] - 2026-04-13

### Added
- **Lyrics Engine (MVP)** — New "Lyrics" tab for phoneme-driven vocal articulation
  - ARPABET phonetic input: type phoneme sequences and the synth performs them one syllable per MIDI note
  - Automatic syllabification via Maximum Onset Principle (MOP) algorithm
  - Global syllable stepping: each note-on advances to the next syllable
  - Loop mode (default on) with Reset button
  - Per-syllable parameter automation: vowelX/Y, consonant place/manner/voicing/level, nasal coupling/place
  - Phoneme-to-parameter mapping for all English ARPABET phonemes (15 vowels + 24 consonants)
  - Visual syllable display with real-time position highlighting
  - Lyrics text persists across DAW save/load
- **`lyricsEnabled` parameter** — Toggle to activate/deactivate lyrics-driven articulation
- **7 new native functions** for WebView-C++ lyrics communication

### Technical Notes
- New DSP file: `dsp/LyricsEngine.h` — Thread-safe syllable queue with SpinLock, atomic index
- LyricsEngine acts as parameter overlay: when enabled, overrides vowelX/Y and consonant params at note-on
- When disabled, all existing parameters and presets work exactly as before

## [1.20.0] - 2026-04-12

### Added
- **Consonant voicing parameter** (`consonantVoicing`, 0–1, default 0.5) — Distinguishes voiced consonants (B, D, G, V, Z) from voiceless (P, T, K, F, S, SH, TH) independently from manner
- **Voicing-aware onset suppression** — Formula: `voicelessFactor = 1 - voicing * (0.3 + 0.7 * manner)`. Voiced plosives retain 30% voice bar; voiceless fricatives get full suppression
- **Continuous voiceless fricative suppression** — New `getContinuousSuppression()` suppresses glottal source for the full consonant envelope duration (not just the 25ms onset window), scaled by fricative-ness and voicelessness
- **Voicing knob in UI** — Added to consonant controls section alongside Level and Auto toggle

### Notes
- Default 0.5 preserves existing patch sound — no breaking changes
- All 16 factory presets updated with `consonantVoicing: 0.5`

## [1.19.0] - 2026-04-11

### Added
- **Effects tab** — New dedicated Effects tab with four signal processors matching O-Lyrica's effects chain:
  - **Chorus** — JUCE built-in chorus with Rate (0.1–10 Hz), Depth, and Mix controls
  - **Delay** — Stereo delay with Lagrange interpolation, 8 kHz feedback filter, Normal/PingPong modes, Time (1ms–2s), Feedback (0–95%), and Mix
  - **Reverb** — 8-channel FDN plate reverb with input diffusion, Householder feedback matrix, multi-LFO modulation, shimmer (octave-up pitch shifter with HP filter), configurable pre-delay (0–200ms), Size, Damping, Mod, Shimmer, and Mix
  - **EQ** — 3-band parametric EQ with Low Shelf (200 Hz), Mid Peak (200–8000 Hz), High Shelf (8 kHz), all ±12 dB
- **Per-effect bypass buttons** — Each effect section has an On/Off toggle
- **22 new APVTS parameters** for full effects control and automation
- Effects chain order: Chorus → Delay → Reverb → EQ → Output Gain

### Technical Notes
- New DSP files: `DSP/DelayProcessor.cpp`, `DSP/EQProcessor.cpp`, `DSP/ReverbProcessor.cpp`
- Effects processing inserted between synthesiser output and output gain stage
- All effects use thread-safe atomic parameter targets with dirty-flag coefficient updates
- Tail length updated to 5.0s to account for reverb/delay tails
- No existing parameter IDs changed — existing presets and automation unaffected

## [1.18.0] - 2026-04-11

### Added
- **Tuning module with dedicated tab** — Full microtonal tuning system integrated via the scala-tuning-engine module v2.0.0. UI converted from single-page to tabbed layout (Synth + Tuning tabs). Tuning tab provides interval editing, pitch circle/polar/matrix/TrueKeys visualizations, embedded tuning library (24+ scales across Historical, Just Intonation, EDO, Non-Octave, and World categories), scale generator (EDO, Harmonic Series, Rank-2), Scala file I/O (.scl/.kbm), adjustable master tune (A4 reference), octave stretch for physical modeling, and tonic selection.
- **5 new APVTS parameters:** `tuning_masterTune` (400–480 Hz), `tuning_tuningMode` (12-TET/Custom/MTS-ESP), `tuning_octaveStretch` (0.95–1.25), `tuning_pitchBendRange` (1–48 st), `tuning_temperamentPreset` (11 built-in temperaments + Custom).
- **Tuning state persistence** — Custom intervals, scale name, and tonic are saved/restored with DAW sessions via ValueTree child node.

### Changed
- FormantVoice now uses TuningEngine for MIDI-to-frequency conversion instead of standard 12-TET `getFrequencyInHertz()`. Affects note onset frequency, spectral tilt reference, and source-filter coupling estimation.

### Technical Notes
- New C++ files: `TuningEngine.cpp`, `ScaleGenerator.cpp`, `TuningExporter.cpp`, `EmbeddedTunings.cpp` (from scala-tuning-engine module)
- New JS: `tuning-panel.js` (self-contained TuningPanel component)
- 25 native functions registered for C++ ↔ JS tuning bridge
- No existing parameter IDs changed — existing presets and automation unaffected.

## [1.17.0] - 2026-04-11

### Added
- **Velocity-to-amplitude dynamics** — Voice output now scales with MIDI velocity (~12 dB dynamic range). Low velocity produces softer notes, high velocity produces full amplitude. Previously velocity only affected glottal Rd (timbral character) with no amplitude response, making soft playing nearly silent.

### Fixed
- **Cascade normalization too aggressive** — Relaxed formant cascade gain compensation from `1/maxPeakGain` to `1/sqrt(maxPeakGain)`, recovering ~12 dB of headroom. The v1.14.1 normalization prevented clipping but over-attenuated the signal, making `tanh()` soft-clipper a no-op instead of providing gentle saturation. Peaks now reach 2–4× into tanh for natural warmth.
- **Nasality amplitude compensation** — Reduced excessive volume drop when `nasalCoupling` is active. Direct attenuation reduced from -8 dB to -3 dB at full coupling; formant bandwidth widening scaled from 2× max to 1.6× max; aspiration suppression reduced from 80% to 50%.

### Technical Notes
- `FormantVoice.cpp`: block-rate `velocityGain = 0.25 + 0.75 * noteVelocity` applied to both voiced source and consonant noise before formant filters
- `CascadeFormantBank.h` line 139: normalization changed from `1.0f / maxPeakGain` to `1.0f / std::sqrt(maxPeakGain)`
- `FormantVoice.cpp` line 236: `nasalAmpGain` changed from `-8.0f * nasalCouplingVal` to `-3.0f * nasalCouplingVal`
- `FormantVoice.cpp` line 243: aspiration suppression coefficient changed from `0.8f` to `0.5f`
- `FormantVoice.cpp` line 392: `nasalBWScale` changed from `1.0f + nasalCouplingVal` to `1.0f + nasalCouplingVal * 0.6f`
- No parameter ID changes — existing presets and automation unaffected.

## [1.16.0] - 2026-04-11

### Added
- **Nasal consonant support (/m/, /n/, /ŋ/)** — Klatt-style pole-zero filter added to the voice signal chain with two new parameters: `nasalCoupling` (0–1 velum opening) and `nasalPlace` (0=bilabial /m/, 0.5=alveolar /n/, 1.0=velar /ŋ/). The pole-zero chain consists of a fixed nasal-cavity resonator at 270 Hz and two anti-formant notches whose center frequencies interpolate across the place axis (/m/ 800/2700 Hz → /n/ 1700/3500 Hz → /ŋ/ 3200/4800 Hz). Two new knobs labeled **Nasality** and **Nasal Place** added to the Voice Character section of the UI. Default `nasalCoupling = 0` means existing presets are unaffected.
- **Nasal-aware voice shaping** — When `nasalCoupling > 0`, aspiration noise is suppressed (up to 80% at full coupling, since nasals are purely voiced), formant bandwidths widen up to 2× (nasal-cavity wall damping), and overall amplitude is reduced by up to 8 dB (nasals are ~6–10 dB quieter than vowels). All scaled linearly with coupling.

### Technical Notes
- New file `Source/dsp/NasalPoleZero.h` — three `FormantBiquad` stages in series (fixed-frequency nasal pole via `makeResonator` + two `juce::dsp::IIR::ArrayCoefficients<float>::makeNotch` anti-formants). `SmoothedValue` ramps for coupling (20 ms) and place sweeps (50 ms).
- **Transparency at coupling=0 via wet/dry mix**, not pole/zero cancellation: `output = input + mix * (wet - input)` where `mix = smoothedCoupling`. Guarantees bit-exact passthrough when the parameter is at default and eliminates the risk of imperfect cancellation in the filter chain. Cost: ~1 multiply-add per sample beyond the 3 biquads.
- **Integration at FormantVoice layer, not CascadeFormantBank** (deviation from original plan): the nasal stage runs on the final filter output after either the cascade or parallel path completes, so all three topologies (Cascade / Parallel / Hybrid) are handled uniformly without requiring a duplicate NasalPoleZero instance for the parallel path. Cleaner than dual-instance routing.
- APVTS: two new `AudioParameterFloat`s in a new "Nasal (2)" section of `createParameterLayout`; plugin now has 34 parameters (up from 32). No breaking changes — existing presets load with defaults (`nasalCoupling=0`, `nasalPlace=0.5`).
- WebView bindings: `nasalCouplingRelay` + `nasalPlaceRelay` in PluginEditor, matching JS relays in `main.js`, two new `knob-wrap` elements in `index.html` Voice Character section.
- `noteStarted()` calls `nasalPoleZero.reset()` and `snapToTargets()` for click-free onset.
- CPU: ~15 FLOPS/sample/voice (3 biquads + wet/dry mix), ≈0.3% single-core at 16 voices/48 kHz — negligible.
- CMakeLists VERSION 1.15.0 → 1.16.0.

## [1.15.0] - 2026-04-11

### Added
- **Liquid consonants /r/ and /l/ as morph targets** — Extended the 2D vowel morph space from 5 to 7 anchor points. Retroflex /r/ placed at (0.12, 0.72) with its characteristic very-low F3 (1600 Hz) signature, and dark (velarized) /l/ at (0.55, 0.85) with low F2 (900 Hz) and raised F3. Both are sonorants driven by the glottal source — no new DSP filters or parameters needed. Shepard IDW interpolation in `VowelMorpher` auto-adapts via `kNumVowels`, so sweeping the XY pad now smoothly transitions between vowels and liquid consonants.

### Technical Notes
- `VowelData.h`: `kNumVowels` 5→7, added R and L VowelEntry structs. /r/ formants F1=340, F2=1050, F3=1600, F4=3500, F5=4300 Hz (BW 60/90/130/250/280, gains 0/-8/-14/-24/-30 dB from Espy-Wilson 1992). /l/ dark F1=400, F2=900, F3=2600, F4=3400, F5=4200 Hz (BW 80/120/150/250/280, gains 0/-6/-16/-22/-28 dB from Stevens 1998).
- `VowelMorpher.h`: unchanged — already iterates over `VowelData::kNumVowels`.
- `Source/ui/public/js/main.js`: added 'r' and 'l' labels to `vowelLabels` array (XY pad rendering) and matching R/L entries to the `VOWELS` JS mirror array (spectrum display).
- No APVTS changes (32 params unchanged), existing presets load unaffected.
- CPU impact: negligible — Shepard IDW is O(N) over vowels (7 vs 5) at block rate.
- CMakeLists VERSION bumped 1.12.2 → 1.15.0 (CMake version field had drifted from CHANGELOG — aligning it).

## [1.14.1] - 2026-04-10

### Fixed
- **Cascade formant bank clipping** — Added dynamic gain normalization to the cascade (series) resonator bank. All-pole resonators with narrow bandwidths produced peak gains of 10–20× at formant frequencies, driving the per-voice `tanh()` soft-clipper into heavy saturation that sounded like hard clipping. The normalization estimates each resonator's peak gain from its bandwidth/frequency ratio and scales the cascade output by the inverse of the maximum, keeping levels in a range where `tanh()` provides gentle limiting rather than audible distortion. Smoothed over 10ms to prevent clicks during vowel transitions.

### Technical Notes
- Root cause: `makeResonator()` uses unity DC gain (`A = 1 − 2r·cos(θ) + r²`), but peak gain at resonance ≈ `A / (2(1−r)|sin(θ)|)` — ranges from 9× (F1, BW=60Hz) to 20× (F3–F5, BW=100–130Hz). Singer's Formant narrowing BW by 40% pushed peak gains to ~33×
- Fix in `CascadeFormantBank::updateCoefficients()`: computes `maxPeakGain` across all cascade stages, sets `normGainSmoothed` target to `1/maxPeakGain`
- Fix in `CascadeFormantBank::process()`: multiplies cascade output by `normGainSmoothed.getNextValue()`
- `normGainSmoothed`: 10ms ramp, snapped on note onset via `snapToTargets()`
- No new APVTS parameters (32 total unchanged), no breaking changes

## [1.14.0] - 2026-04-10

### Changed
- **Asymmetric triangular glottal noise envelope** — Replaced symmetric cosine window with a piecewise linear envelope matching real dual-peak glottal noise patterns. Linear ramp from 30% floor to peak over the open phase (0–0.6), brief noise burst at the glottal closure instant (0.6–0.65 rapid decay), then floor during closed phase. Produces more realistic aspirated-to-closed transitions than the previous smooth cosine.
- **Breathiness-dependent spectral tilt filter** — Added one-pole lowpass on aspiration noise output whose cutoff varies with the breathiness parameter: high breathiness → 2kHz cutoff (warm, airy turbulence), low breathiness → 6kHz cutoff (hissy, pressed character). Previously the noise was spectrally flat (white) before reaching the formant bank.
- **Stochastic breath drift** — Added slow random walk (~75ms update interval) that modulates noise amplitude by ±1–2dB and spectral tilt cutoff by ±200Hz. Two independent walks with SmoothedValue interpolation prevent the frozen-noise quality of deterministic envelopes. Real breath turbulence is non-stationary — this models that.

### Technical Notes
- All changes contained in `AspirationNoise.h` — no interface changes, `FormantVoice.cpp` unmodified
- Triangular envelope: `noiseFloor=0.3`, `openPhaseEnd=0.6`, `burstWidth=0.05` (5% of cycle)
- Spectral tilt: `cutoff = 6000 - breath * 4000 + driftHz`, one-pole α from `exp(-2πf/sr)`
- Drift: independent Xorshift random walks — amplitude (±0.5dB steps, ±2dB clamp), tilt (±50Hz steps, ±200Hz clamp), SmoothedValue ramps (50ms) for click-free interpolation
- No new APVTS parameters (32 total unchanged), no breaking changes

## [1.13.0] - 2026-04-10

### Added
- **Envelope-aware breath amplitude modulation** — Aspiration noise now varies by ADSR phase to model realistic vocal onset and release behavior:
  - **Attack onset (0–50ms):** Breath boosted +4.5dB (exponential decay, τ=15ms) simulating aspirated vocal fold engagement
  - **Sustain:** Breath at user-set level (unchanged behavior)
  - **Release onset (0–40ms):** Breath boosted +3dB (exponential decay, τ=12ms) simulating vocal fold disengagement, then fades with main envelope

### Technical Notes
- New `releaseSampleCount` member tracks ADSR phase (noteOn resets to -1, noteOff sets to 0)
- `breathEnvMul` computed per-block from elapsed time, applied to `effectiveBreath` before `setBreathiness()`
- Clamped to [0, 1] after multiplication — high breathiness settings saturate naturally at pure noise during onset
- No new APVTS parameters (32 total unchanged), no breaking changes

## [1.12.1] - 2026-04-07

### Changed
- **Consonant Level parameter range doubled** — Extended from 0.0–1.0 to 0.0–2.0 so consonants can be mixed louder. At max (2.0), consonant noise is twice the previous maximum amplitude. Existing presets (all ≤ 0.8) load unchanged. Signal path protected by per-voice `tanh()` soft-clip and brickwall limiter.

### Technical Notes
- `consonantLevel` NormalisableRange max changed from 1.0f to 2.0f in APVTS layout
- No new parameters (32 total unchanged), no breaking changes — stored parameter values remain valid

## [1.12.0] - 2026-04-07

### Added
- **Singer's Formant control** — Models the trained operatic singing voice phenomenon (Sundberg) where the pharynx-to-epilarynx tube ratio causes F3, F4, and F5 to cluster into a single reinforced spectral peak around 2.5-3.5 kHz, enabling vocal projection over orchestral accompaniment.
- **New APVTS parameter: `singersFormant`** (float 0-1, default 0) — At 0 no effect. At 1.0, F3/F4/F5 frequencies are pulled toward 3000 Hz with per-formant cluster strengths (F3: 0.7, F4: 0.8, F5: 0.6 — F4 clusters most strongly as the primary contributor). Bandwidths of F3-F5 narrow by up to 40% to sharpen the cluster peak. Gains of F3-F5 boosted by up to +4 dB to model acoustic reinforcement.
- **UI: Singer's F knob** — Added to the Character parameter group in the WebView UI.
- **Factory presets updated** — 5 presets include characterful singer's formant values: Overtone Chant=0.7, Pressed Baritone=0.7, Natural Tenor=0.5, Sci-Fi Choir=0.4, Breathy Soprano=0.3. Remaining presets default to 0 (speech-like character).

### Technical Notes
- Clustering applied in FormantVoice::renderNextBlock at block rate (every 32 samples) AFTER vowel morph + dynamic bandwidth variation, BEFORE passing to filter banks — both parallel and cascade topologies benefit
- Formula: `F_clustered = F_base + singersFormant * (3000 - F_base) * clusterStrength[i]`
- Bandwidth narrowing: `BW *= (1.0 - singersFormant * 0.4)`
- Gain boost: `gain *= dB_to_linear(4.0 * singersFormant)`
- 1 new APVTS parameter (32 total), no breaking changes

## [1.11.1] - 2026-04-07

### Fixed
- **Held tone now audible in Cascade/Hybrid topology** — The cascade formant bank was using `makeBandPass` (zero-pole BPF) instead of all-pole resonators for series-chained filters. Bandpass filters at different center frequencies (F1=700Hz, F2=1200Hz, etc.) have non-overlapping passbands, so cascading them produced ~60+ dB cumulative attenuation — effectively silence for the voiced path. Consonant attack was still audible because consonants route through the parallel bank, which sums BPF outputs correctly.
- **Root cause:** `CascadeFormantBank` used `juce::dsp::IIR::ArrayCoefficients::makeBandPass` which creates a filter with zeros at DC and Nyquist, explicitly cutting frequencies outside the passband. Klatt cascade synthesis requires all-pole resonators that add peaks without cutting other frequencies.
- **Fix:** Replaced `makeBandPass` with custom `makeResonator()` — all-pole second-order resonator with unity DC gain normalization (`A = 1 - 2r·cos(θ) + r²`). Cascade filters now correctly shape the broadband glottal excitation into vowel spectra. Hybrid mode parallel filters (F4-F5) retain BPF for proper band isolation.
- **Removed 12 dB compensation gain** — No longer needed since resonators don't attenuate the signal like bandpass filters did.

### Technical Notes
- `CascadeFormantBank::makeResonator(sr, freq, bw)` — static method computing all-pole resonator coefficients: `r = exp(-π·BW/Fs)`, `θ = 2π·F/Fs`, feedback coeffs `a1 = -2r·cos(θ)`, `a2 = r²`, feedforward `b0 = A, b1 = b2 = 0`
- Pole radius clamped to r ≤ 0.9999 for stability; frequency and bandwidth clamped to safe ranges
- Both `updateCoefficients()` and `process()` (smoothing path) use resonators for cascade indices and BPF for parallel indices
- Existing `tanh()` soft-clip in FormantVoice handles resonator peak amplitudes
- No parameter changes (31 total unchanged), no breaking changes

## [1.11.0] - 2026-04-07

### Added
- **Source-filter coupling** — Subtle harmonic reinforcement approximation inspired by Titze (2008, JASA). At block rate, checks if harmonics 2f0–4f0 fall within ±bandwidth of F1 or F2. When a harmonic is near a formant peak, applies a smoothed gain boost (up to +2 dB) scaled by proximity: `boost = 2dB * (1 - |harmonic - formantFreq| / bandwidth) * coupling`. Also increases pitch jitter by up to +0.3% when harmonics cross formant boundaries, simulating the f0 instabilities documented in source-filter interaction research.
- **New APVTS parameter: `sourceFilterCoupling`** (float 0-1, default 0.3) — Scales both the harmonic reinforcement gain and the jitter modulation. At 0 the effect is disabled (pure linear source-filter model). At 1 full +2 dB boost and +0.3% jitter when harmonics align with formants.
- **Factory presets updated** — All 16 presets include characterful coupling values (e.g., Robotic Speech=0.0 disabled, Overtone Chant=0.7 strong reinforcement, Formant Bass=0.6, Glitch Vocal=0.1 minimal).

### Technical Notes
- FormantVoice: block-rate proximity detection (every 32 samples) scans harmonics 2-4 against F1/F2 using `formantFreqs[]`/`formantBWs[]`; best proximity across all pairs drives both gain and jitter
- Gain applied via `SmoothedValue<float>` (10ms ramp) between formant filter output and `tanh()` soft-clip
- Jitter boost additive to existing VibratoLFO micro-jitter offset in per-sample pitch computation
- F0 estimate for block-rate check uses `currentlyPlayingNote.getFrequencyInHertz()` (MIDI note frequency, adequate accuracy for proximity detection)
- 1 new APVTS parameter (31 total), no breaking changes

## [1.10.0] - 2026-04-07

### Changed
- **Pitch-synchronous aspiration noise** — Breathiness is no longer constant white noise. Aspiration amplitude is now modulated by the glottal cycle phase, peaking during the open phase (~0.0–0.6) and dipping during the closed phase (~0.6–1.0). Uses a cosine window centered at phase=0.3 with a 30% floor: `noiseGain = 0.3 + 0.7 * (0.5 + 0.5 * cos(2π(phase - 0.3)))`. This makes breathiness sound throaty and organic rather than like added static.
- **Upgraded aspiration noise filter** — Replaced single-pole IIR lowpass at 4kHz with a biquad bandpass filter centered at 3kHz (Q=1.0, ~3kHz bandwidth) to better match real aspiration spectra. Uses transposed direct form II for numerical stability.

### Technical Notes
- LFGlottalSource: added `getPhase()` public getter exposing the [0,1) phase accumulator
- AspirationNoise: added `setGlottalPhase(float)` for per-sample phase injection; replaced `lpCoeff`/`prevFilterOutput` with biquad state (`bpB0/B1/B2`, `bpA1/A2`, `bpZ1/Z2`); `computeBandpassCoeffs()` derives coefficients from Audio EQ Cookbook BPF formula
- FormantVoice: calls `aspirationNoise.setGlottalPhase(glottalSource.getPhase())` each sample after `getNextSample()`
- No new parameters (30 total unchanged), no breaking changes

## [1.9.0] - 2026-04-07

### Added
- **Smooth formant transitions with configurable timing** — Per-formant `SmoothedValue<float>` objects (5 frequencies + 5 bandwidths = 10 total) provide sample-rate interpolation when vowel morph position changes. Eliminates step-function jumps from the previous 32-sample block-rate updates. Transition times differ per formant to mimic real articulatory dynamics:
  - F1: up to 50ms (jaw — fastest articulator)
  - F2-F3: up to 80ms (tongue body — medium)
  - F4-F5: up to 120ms (slowest articulatory gestures)
- **New APVTS parameter: `transitionTime`** (float 0-1, default 0.4) — Scales per-formant ramp durations. At 0 = instant transitions (backward compatible with previous behavior). At 1 = maximum transition times (F1: 50ms, F2-F3: 80ms, F4-F5: 120ms). The formula: `rampTime = transitionTime * perFormantMaxTime`.
- **Zero-overhead steady state** — Biquad coefficients are only recomputed per-sample when `SmoothedValue::isSmoothing()` returns true. When formants are stable, processing cost is identical to previous version.
- **Click-free note onset** — SmoothedValues snap to current targets via `setCurrentAndTargetValue()` on `noteStarted()`, preventing formant smearing at note attack.
- **Both filter topologies supported** — Smoothing applied to both `FormantFilterBank` (parallel) and `CascadeFormantBank` (cascade/hybrid) with identical per-formant timing schedules.
- **UI: Transition knob** — Added to the Character parameter group in the WebView UI.
- **Factory presets updated** — All 16 presets include characterful `transitionTime` values (e.g., Robotic Speech=0.0 instant, Ethereal Drone=0.8 slow morph, Glitch Vocal=0.1 near-instant).

### Technical Notes
- FormantFilterBank: `setTransitionTime(float)` configures 10 SmoothedValues with per-formant ramp durations; `snapToTargets()` for note onset; `updateCoefficients()` sets targets instead of direct application; `process()` advances smoothing and recomputes biquad coefficients per-sample only during active transitions
- CascadeFormantBank: identical smoothing treatment for cascade/hybrid topologies
- FormantVoice: reads `transitionTime` at block rate, calls `setTransitionTime()` on both banks each block, snaps on noteStarted
- 1 new APVTS parameter (30 total), with WebView relay + attachment, pluginval pending

## [1.8.0] - 2026-04-07

### Added
- **Cascade formant filter topology** — Klatt-style series resonator bank as alternative to existing parallel topology. Cascade chains 5 bandpass filters in series (F1→F2→F3→F4→F5), automatically producing correct relative formant amplitudes without per-formant gain control (Klatt, 1980). This is the most physically accurate model for vowel synthesis.
- **New APVTS parameter: `formantTopology`** (Choice: Cascade/Parallel/Hybrid, default Cascade) — Three topology modes:
  - *Cascade* (0): All 5 formants in series for voiced path. Correct 1/f spectral envelope slope, natural amplitude relationships. Consonant noise routed through parallel bank.
  - *Parallel* (1): Legacy behavior — voice and consonant mixed through 5 parallel bandpass filters with explicit per-formant gains. Preserves backward compatibility with existing presets.
  - *Hybrid* (2): F1-F3 in cascade (low formants benefit most from series topology), F4-F5 in parallel. Best of both worlds.
- **Split voiced/consonant signal paths** — In Cascade and Hybrid modes, glottal source routes through cascade bank while consonant noise routes independently through parallel bank. Consonants need per-formant gain control that parallel provides. Spectral tilt applied only to voiced path (physically correct — models glottal spectral slope, not vocal tract).
- **Gain compensation** — Cascade output boosted ~12 dB (5 stages) or ~7 dB (3 stages in hybrid) to compensate for series filter attenuation. Uses `2^(numStages * 0.4)` scaling.
- **UI: Topology selector** — Segmented control (Cascade/Parallel/Hybrid) in the Character parameter group.
- **Factory presets updated** — All 16 presets include `formantTopology` set to Cascade for improved vowel naturalness.

### Technical Notes
- New class `CascadeFormantBank` in `Source/dsp/CascadeFormantBank.h` — reuses `FormantBiquad` struct, shares coefficient computation with `FormantFilterBank` but sets per-filter gain to unity
- `CascadeFormantBank::process()` chains first N filters in series, sums remaining in parallel (supports both full cascade and hybrid)
- `setNumCascadeStages(n)` configures cascade/hybrid split and auto-computes compensation gain
- FormantVoice topology routing: block-rate parameter read, per-sample branched signal flow
- Topology=1 (Parallel) uses exact legacy code path for backward compatibility
- WebView: `WebComboBoxRelay` + `WebComboBoxParameterAttachment` for topology selector
- 1 new APVTS parameter (29 total), pluginval validated at strictness 5

## [1.7.0] - 2026-04-06

### Added
- **Spectral tilt filter** — Independent voice brightness/darkness control decoupled from Rd voice quality. One-pole IIR filter between source and formant filter bank shapes the H1-H2 balance (the #1 perceptual correlate of phonation type per Kreiman et al., 2015).
- **New APVTS parameter: `spectralTilt`** (float -12 to +12 dB, default 0, label "Spectral Tilt") — Positive values attenuate upper harmonics (darker/breathier), negative values boost upper harmonics relative to fundamental (brighter/more pressed). Enables combinations like "breathy but bright" or "pressed but dark" that the Rd parameter alone cannot produce.
- **UI: Tilt knob** — Added to the Glottal Source parameter group in the WebView UI.
- **Factory presets updated** — All 16 presets include characterful spectralTilt values (e.g., Creature Growl=-3dB bright+pressed, Alien Whisper=+4dB dark+breathy, Breath Texture=+5dB very dark).

### Technical Notes
- One-pole lowpass: `lp = (1-alpha)*x + alpha*prev`, cutoff at 2*f0, alpha updated at block rate from current pitch
- Unified tilt formula: `output = x - tiltNorm * (x - lp)` handles both positive (blend toward lowpass) and negative (boost highpass complement) in a single expression
- tiltNorm = tiltDdB / 12.0 normalizes range to [-1, +1]; at 0 the filter is transparent (output = input)
- Filter state reset on noteStarted() for click-free onset
- 1 new APVTS parameter (28 total), with WebView relay + attachment

## [1.6.0] - 2026-04-06

### Added
- **Dynamic formant bandwidth variation** — Bandwidths now modulate based on vowel openness and breathiness instead of remaining fixed from VowelData interpolation. Two modulation sources applied at block rate after Shepard interpolation:
  - *Vowel openness:* F1 frequency used as proxy for jaw opening. B1 scaled by `(1.0 + 0.4 * (F1 - 400) / 800)`, clamped to [40, 200] Hz. B2-B5 receive 30% of B1's scaling. Open vowels (/a/) get wider B1 (~+12%), closed vowels (/i/) slightly narrower (~-4%).
  - *Breathiness coupling:* All BW1-BW5 scaled by `(1.0 + breathiness * 0.5)`, giving breathy voices up to 50% wider bandwidths for a more diffuse, airy resonance.

### Technical Notes
- No new parameters — uses existing `breathiness` (0-1) and derived F1 from vowel morpher output
- Modulation inserted in FormantVoice::renderNextBlock between vowelMorpher.compute() and filterBank.updateCoefficients(), preserving existing shift/spread/Q pipeline
- Perceptual effect: breathy voices sound more diffuse; open vowels have warmer, less resonant quality (ref: Fleischer et al., 2015 vocal tract measurements)
- 27 APVTS parameters unchanged

## [1.5.0] - 2026-04-06

### Added
- **Dynamic Rd (vocal effort) modulation** — Glottal source Rd now responds automatically to pitch, velocity, and expression instead of remaining static per voice. Three modulation sources combined at block rate:
  - *Pitch tracking:* -0.3 Rd per octave above middle C (higher pitch = more pressed voice)
  - *Velocity mapping:* MIDI velocity 0-127 scales to 0 to -0.5 Rd offset (harder attack = more effort)
  - *Expression:* MPE pressure / aftertouch maps 0-1 to +/-0.4 Rd offset (continuous effort control)
- **New APVTS parameter: `rdModDepth`** (float 0-1, default 0.5) — Master depth control that scales all three Rd modulation sources. At 0 the voice quality knob behaves exactly as before (static Rd). At 1 full dynamic modulation is applied.
- **20ms SmoothedValue ramp** — Per-sample linear smoothing on the effective Rd prevents clicks during rapid modulation changes.
- **UI: Rd Mod knob** — Added to the Glottal Source parameter group in the WebView UI.
- **Factory presets updated** — All 16 presets include characterful rdModDepth values (e.g., Robotic Speech=0.0, Natural Tenor=0.6, Pressed Baritone=0.7).

### Technical Notes
- FormantVoice: effective Rd computed at block rate from base knob + modDepth * (pitch + velocity + expression offsets), clamped to valid LF model range [0.3, 2.7]
- Per-sample `glottalSource.setRd(rdSmoothed.getNextValue())` replaces previous block-rate static setRd
- Note onset initializes rdSmoothed with `setCurrentAndTargetValue()` for click-free first block
- 1 new APVTS parameter (27 total), with WebView relay + attachment

## [1.4.0] - 2026-04-06

### Added
- **Jitter and shimmer modeling** — Per-cycle random perturbation of f0 (jitter) and amplitude (shimmer) in the LF glottal source for vocal naturalness. Uses 1/f noise approximation via EMA-filtered random values (~50ms time constant). Perturbations applied once per glottal cycle at phase wrap, not per-sample.
- **Inverse scaling** — Both jitter and shimmer scale inversely with pitch (more perturbation at low f0, ref 200Hz) and inversely with velocity (controlled singing = less perturbation).
- **Two new APVTS parameters** — `jitter` (0-1, default 0.15, maps to 0-2% relative f0 perturbation) and `shimmer` (0-1, default 0.1, maps to 0-5% relative amplitude perturbation). Defaults sit within healthy voice norms (<1% jitter, <3% shimmer).
- **Per-voice RNG seeding** — Each voice gets a unique Xorshift32 seed for uncorrelated perturbation patterns across polyphonic notes.
- **UI: Jitter and Shimmer knobs** — Added to the Glottal Source parameter group.
- **Factory presets updated** — All 16 presets now include characterful jitter/shimmer values (e.g., Robotic Speech=0/0, Creature Growl=0.3/0.3, Natural Tenor=0.15/0.1).

### Technical Notes
- LFGlottalSource: `setJitterShimmer()` and `setSeed()` methods, cycle detection via phase accumulator wrap
- Jitter modifies `phaseIncrement` multiplier per cycle; shimmer modifies output gain multiplier per cycle
- EMA alpha computed from cycle period: `alpha = 1 - exp(-cyclePeriod / 0.050)`
- 2 new APVTS parameters (26 total), with WebView relays + attachments

## [1.3.0] - 2026-04-06

### Added
- **Manual consonant envelope parameters** — Three new knobs (Cons Attack 1-100ms, Cons Hold 0-200ms, Cons Decay 5-200ms) for user control of the consonant envelope when Auto is off. When Auto is on, timing is derived from manner parameter as before.
- **Consonant envelope always triggers at note onset** — Both auto and manual modes now trigger the consonant envelope on every note. Auto toggle switches between manner-derived timing and user knob timing.
- **UI: consonant envelope knobs** — Three small knobs appear in the consonant section, dimmed when Auto is active (timing from manner), fully interactive when Auto is off.

### Changed
- **autoConsonant parameter behavior** — Now toggles between auto-derived envelope timing (from manner) and manual envelope timing (from knobs). Both modes have transient consonant behavior. Previously, turning auto off meant continuous noise with no independent envelope.

### Technical Notes
- ConsonantEngine: new `setManualEnvelope()` overrides cached attack/hold/decay sample counts
- `getNextSample()` signature simplified — removed `autoConsonant` parameter, envelope always applied
- `triggerBurst()` always called at note onset regardless of auto setting
- 3 new APVTS parameters: consonantAttack, consonantHold, consonantDecay (24 total)

## [1.2.0] - 2026-04-06

### Changed
- **Dedicated consonant envelope independent of main ADSR** — Consonants now have their own 3-phase envelope (Attack/Hold/Decay) triggered at note onset, with timing derived from manner parameter. Plosives: 1ms attack, 0ms hold, 15ms decay (~16ms total). Fricatives: 40ms attack, 60ms hold, 40ms decay (~140ms total). Values informed by Klatt synthesis research and measured speech data.
- **Split voice/consonant envelope paths** — Main ADSR envelope now applies only to the voiced glottal source. Consonant noise uses its own internal envelope when autoConsonant is enabled, preventing slow ADSR attacks from destroying consonant transients. When autoConsonant is off, consonant noise still follows the main ADSR for continuous texture use.
- **Consonants are now transient speech events** — With autoConsonant enabled, consonant output decays to zero after the envelope completes (16-140ms depending on manner), matching natural speech consonant durations instead of sustaining indefinitely.

### Technical Notes
- ConsonantEngine: new `EnvPhase` state machine (Off/Attack/Hold/Decay) with `advanceEnvelope()` per-sample processing
- Envelope timing computed in `updateCoefficients()` from manner parameter, cached as sample counts
- FormantVoice signal path: ADSR applied to voiceSource before mixing with consonant, both still routed through shared formant filter bank for vocal tract resonance
- ADSR before filter is more physically accurate — models glottal amplitude variation feeding into vocal tract resonator

## [1.1.1] - 2026-04-05

### Fixed
- **Output safety: Q clamping + bandwidth scaling** — Formant filter Q was unbounded (reaching 549 at extreme shift/spread), producing ~148 dB above 0dBFS. Bandwidth now scales proportionally with shift factor, Q clamped to [0.5, 25].
- **Per-voice soft clip** — Added `tanh()` saturation after formant filtering to prevent extreme amplitudes from resonant filters. Transparent at normal levels.
- **Consonant output hard clamp** — ConsonantEngine output now clamped to [-1, 1] before entering formant filter bank.
- **Brickwall limiter in processBlock** — Final hard clip at 0 dBFS after output gain as last line of defense against dangerous levels.

## [1.1.0] - 2026-04-05

### Changed
- **Consonant engine replaced with place/manner articulation model** — Previous LP/HP crossfade + sibilance bandpass produced indistinct continuous noise. New engine uses dual resonant bandpass filters shaped by place-of-articulation (X axis: Labial 500Hz -> Alveolar 3kHz -> Palatal 6kHz -> Velar 2kHz) with manner-dependent temporal profiles (Y axis: Plosive short burst with glottal mute -> Fricative sustained noise). Root cause: old architecture had no spectral place modeling and fixed temporal shape regardless of consonant type.
- **Consonant XY pad replaces Tone/Sibilance knobs** — New interactive 2D pad for place (X) and manner (Y) control with frequency readout, place labels (Lab/Alv/Pal/Vel), and manner labels (Plos/Fric). Matches vowel XY pad visual style.
- **Parameter display names updated** — "Consonant Tone" -> "Place", "Sibilance" -> "Manner" (APVTS IDs unchanged, existing automation compatible)
- **All 16 factory presets updated** — Each preset now has meaningful place/manner values matching its character (e.g., Creature Growl = velar plosive, Robotic Speech = alveolar fricative)
- **Default sibilance (manner) changed from 0.0 to 0.5** — Mid-manner default produces a balanced blend of burst and sustained noise instead of pure plosive

### Technical Notes
- Onset suppression now scales with (1 - manner): full glottal mute for plosives, none for fricatives
- Burst duration varies with manner: 8ms (plosive) to 80ms (fricative)
- Burst decay rate varies with manner: exp(-12t) (plosive, sharp) to exp(-2t) (fricative, gentle)
- Place Q varies: Labial Q=1.5 (broad) -> Alveolar Q=4.0 (tight) -> Palatal Q=3.0 -> Velar Q=2.0
- Preserves v1.0.1 consonant-through-formant routing and onset suppression architecture

## [1.0.1] - 2026-04-05

### Fixed
- **Consonant noise now routed through formant filter bank** — Previously consonant noise bypassed the vocal tract model and was mixed directly into the output, producing raw noise instead of speech-like consonants. Now passes through the same 5-formant parallel BPFs as the glottal source, giving consonants proper vocal tract resonance.
- **Plosive burst sharpened** — Reduced burst duration from 15ms to 8ms with faster exponential decay (exp(-10t) vs exp(-5t)) for more realistic plosive transients.
- **Glottal source suppression during plosive onset** — Added 25ms onset envelope that briefly suppresses the glottal source (70% at peak) during auto-consonant bursts, letting noise dominate at attack before voice takes over. Simulates vocal fold closure during /p/, /t/, /k/ consonants.

## [1.0.0] - 2026-04-05

### Added
- **Physical model vocal synthesizer** - LF glottal pulse model with Fant 1995 Rd voice quality control (0.3-2.7)
- **5-formant parallel bandpass filter bank** - Vocal tract modeling with per-formant frequency, bandwidth, and gain
- **2D vowel morph pad** - XY cursor with 5 cardinal vowels at acoustic positions, Shepard IDW interpolation
- **Consonant noise engine** - KLATT dual-branch topology with tone shaping, sibilance, and auto-consonant plosive burst
- **Vibrato system** - Per-voice sine LFO with rate, depth, onset delay, and micro-jitter
- **Pitch glide** - Exponential portamento between notes (0-1000ms)
- **ADSR envelope** - Per-voice amplitude envelope with full parameter automation
- **16-voice polyphony** - MPE-ready via juce::MPESynthesiser with legacy MIDI fallback
- **MPE support** - Pressure->breathiness, slide->vowel Y, velocity->attack character
- **Formant shift and spread** - Semitone-based frequency shifting and center-of-mass spacing control
- **Output stage** - Smoothed output gain (dB) and per-voice stereo width (equal-power pan by MIDI note)
- **WebView UI** - Naturalist aesthetic with XY vowel pad, formant overlay, organized parameter sections
- **16 factory presets** - 4 categories (Cinematic, Electronic, Ambient, Speech), 4 presets each
- **Preset browser** - OuariconPresetManager with prev/next, category dropdown, save/load
- **Mipmapped glottal wavetable** - 128 Rd steps x 10 mipmap levels, FFT-based anti-aliasing

### Technical Notes
- Domain: C++ DSP + WebView UI
- 21 parameters across 7 groups (Vowel, Glottal, Consonant, Envelope, Character, Output, Control)
- JUCE 8.0.4, CMake + Ninja build system
- VST3 + AU formats, macOS
- Pluginval validated at strictness level 10
