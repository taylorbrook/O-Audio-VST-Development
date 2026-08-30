# O-Wind Changelog

## [1.17.0] - 2026-08-29

### Added — the PAGE speaks French (Stage K batch K4, canon v2)

67 label entries over 65 keyed elements and 22 keyed accessible names, a gear
popover with the language selector, the C++ language pair with persistence, and
`Resources/ui/js/i18n.js` embedded and served in this same commit. No hover-help
copy: v1.16.3 had none and authoring it is Stage M. MINOR — new user-facing
feature, no parameter, range or state-format change.

**Measured, against the batch inventory's 61 / 7 / 3 / 3 / 4.** The extractor
reports LABEL 61, READOUT 7, UNSURE 3, attributes 3, js-prose 4, and every one
of those numbers is right about what it can see. Two things it cannot see are
the real finding here:

- **Sixteen Effects-tab knob captions.** `makeFxKnob(id, label)` took the
  caption as an ordinary function argument and interpolated it into an innerHTML
  template; the English words sat in object literals one frame from the write.
  The extractor's js-prose scan and check-i18n assertion 12 both look for a
  prose LITERAL on an assignment's right-hand side, so all sixteen would have
  shipped English inside a French UI with every gate GREEN. Each now carries a
  LITERAL key at its own call site — `setLabel(addFxKnob('reverb-knobs',
  'reverbSize'), 'label.fx.size')` — because a key read out of a data table
  fails assertion 13 and is invisible to assertion 15.
- **Sixteen native `title=` attributes written from script.**
  `setupFxKnob()` did `valueDisplay.title = 'Double-click to edit'` on every FX
  readout. The page rendered NINETEEN native titles against the THREE the markup
  declares; assertion 11 reads index.html only, so it counted three. Contract §4
  deletes a native title rather than localizing it, and where the title is an
  element's only help its text moves to `data-i18n-aria` — the same eighteen
  characters, verbatim, no new prose. boot-all-uis now reports `title= 0` for
  this plugin.

Two of the nine non-literal `textContent` writes on this page were also carrying
English one frame away: the FX bypass face (`bypassed ? 'Off' : 'On'`, now two
literal `setLabel` calls, not a ternary — contract §6) and the tuning panel's
load-failure notice (was an `innerHTML` string, now createElement + setLabel).
The other seven are numeric readouts, a preset filename, or an empty string.

**Deliberately NOT localized, with reasons on the record in `I18N_EXEMPT`:**

- **The Tuning tab, entirely.** O-Wind embeds
  `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js`
  BY REFERENCE from the module tree (CMakeLists.txt:92) rather than carrying a
  plugin-owned copy. Localizing it is a cross-plugin change and a local edit
  would be reverted by `/module-upgrade`. **A French user reads one of this
  plugin's three tabs in English.**
- **The eight instrument `<option>` captions.** Not an arm-1 case —
  `instrumentPreset` is an `AudioParameterInt`, not a Choice — but they are
  byte-identical to the eight FACTORY PRESET NAMES, and a preset name IS the
  JSON filename (D-02). The preset browser 30 px above lists the same eight in
  English, so localizing the selector would put two languages on one set of
  eight names inside one frame.
- **`Normal` / `PingPong`.** delayMode `AudioParameterChoice` option strings
  VERBATIM — D-01 arm 1.

### Changed — geometry pins, each negative-controlled

Eight width pins, every one REVERTED ALONE and confirmed to re-break its gate.
Zero decoration.

| pin | reverted alone, `[7]` fails with |
|---|---|
| `.preset-save-btn` 68px | `#preset-name` dw=-11.5 |
| `.toggle-label` 86px | `#tone-hole-toggle` dw=9.7, `#instrument-select` dx=9.7 |
| `.instrument-selector label` 80px | `.instrument-selector` dw=33.9 |
| `.adsr-section .section-label > span` 160px | `#adsr-toggle` dx=7.7 |
| `.fx-title` 64px | `#delay-knobs` dx=-1.6 (67 moved) |
| `.fx-bypass-btn` 52px | every fx knob dx=-9.1 (118 moved) |
| `#tab-effects .knob-container` 66px | 96 moved across four rows |
| `.settings-popover` 190px | `#settings-popover` dx=16.0 dw=-16.0 |

Before: **8 / 10 / 99** non-label elements moved EN→FR in the default, popover
and Effects states. After: **0 / 0 / 0**, and the page HOLDS STILL — 284 of 284
elements identical at 180 ms and at 1.7 s in both languages on the Sound tab,
164 of 164 on the Effects tab. Every EN→FR difference that remains is a keyed
caption's own box.

**Twelve of the 43 Sound-tab captions SHRANK in French** — Tone Color −24.6,
Sound −21.3, Vib Tremolo −19.6, Drift Speed −18.7, Air Column −15.2, Drift Depth
−11.4, Inf. Sustain −8.9, Effects −8.6, Output −4.8 / −3.9, Flut Rate −2.6,
Rev. Jet −1.8. A clip-only check would have certified this page.

`.knob-label` is `max-width: 72px; nowrap; ellipsis`, and ENGLISH is already
within 1.27 px of it — "Embouchure" measures 70.73. No French caption on this
page was drafted longer than ten characters for that reason; the widest is
"Embouchure" itself at 70.73 and the next is "Sous-harm." at 62.44.

Three ENGLISH design consequences, all horizontal, no row rewraps, nothing
vanishes: the preset bar loses 45.4 px of elastic name display to the gear
cluster and the wider Save button; the instrument strip's selector moves 45.8 px
right; and each Effects knob cell goes from a shrink-wrapped 44–58 px to a
uniform 66 px, re-centring the four rows. The 66 px floor is set by ENGLISH
("Feedback" 57.97) as much as by French ("Fréq. méd." 63.50).

All French is a machine draft: 67/67 entries `reviewed: false`.

## [1.16.3] - 2026-07-10

### Fixed — final CODE_REVIEW.md info-finding sweep (IN-01, IN-08..10, IN-12..15, IN-17)

Resolves the 9 remaining deferred info findings from the 2026-07-09 deep review
(selected via /improve-review). CODE_REVIEW.md is now fully resolved (40/40).
No parameter, range, or state-format changes — PATCH.

- **IN-01 — ~35 string-keyed APVTS lookups per block per voice.** Every
  `updateParametersFromAPVTS()` call did ~30 `getRawParameterValue(name)` map
  walks (×8 voices ≈ 280 O(log n) string-compare tree walks per block on the
  audio thread), plus 3 more in the processor's post-voice width/formant path.
  Added a `VoiceParamCache` of `std::atomic<float>*` cached once in
  `prepareToPlay()` (same pattern as the processor's existing `fxCache`, which
  gained width/formant/instrumentPreset). startNote and applyPresetCoefficients
  read the cache too. (FluteSynthVoice.{h,cpp}, PluginProcessor.{h,cpp})
- **IN-09 — 8 registered native functions never called from any served JS
  removed:** `savePreset`, `getInstrumentPresets`, `getInstrumentPreset`,
  `setInstrumentPreset`, `setTuningIntervals`, `setTemperamentPreset`,
  `getTemperamentPreset`, `getEmbeddedTuningCategories`. The review's 9th
  (`getMasterTune`) became used by the v1.16.1 WR-10 fix and is kept.
  Verified with a both-direction grep-diff: every `getNativeFunction` name in
  index.html + tuning-panel.js has a registration and vice versa.
  (PluginEditor.cpp)
- **IN-12 — Hand-built JSON in tuning native fns.** `getTuningIntervals`,
  `generateEDO/HarmonicSeries/Rank2` (new `intervalsToJson()` helper) and
  `getEmbeddedTuningList` (DynamicObject + `juce::JSON::toString`) now serialize
  through juce::JSON — a future tuning name containing `"` can no longer break
  the whole list. (PluginEditor.cpp)
- **IN-13 — 52 document-level mouse listeners (2 per knob).** `bindSliderParam`
  registered a document mousemove+mouseup per parameter; every mouse move ran
  26 handlers. Hoisted a single shared `knobDrag` handler pair, matching the
  effects tab's existing `fxKnobDrag` pattern. (index.html)
- **IN-10 — Instrument-preset count hardcoded as `7` in three places.** The JS
  selector now derives the index count from the backend-pushed
  `state.properties.numSteps` (fallback 7), so adding a 9th preset in C++ can't
  silently break the mapping. (index.html)
- **IN-14 — FX wheel edits sent no drag gesture.** The effects-knob wheel
  handler now brackets `setNormalisedValue` in
  `sliderDragStarted()`/`sliderDragEnded()`, so hosts that gate automation
  recording on gestures record wheel edits. (The dblclick value editor already
  bracketed correctly.) (index.html)
- **IN-15 — `exportTuningHTML` reported success on write failure.** The
  `file.replaceWithText()` result is now returned to JS instead of an
  unconditional `true`, matching saveScalaFile/saveKBMFile. (PluginEditor.cpp)
- **IN-08 — `StereoWidthProcessor::reset()` disabled width smoothing.**
  `widthSmoothed.reset(0)` set steps-to-target to 0 until the next prepare
  (only reachable via releaseResources, so impact was nil in practice); now
  snaps via `setCurrentAndTargetValue` without touching the ramp config.
  (DSP/StereoWidth.h)
- **IN-17 — shared module `preset-manager` v1.0.4:**
  `initializeFactoryPresets()` now sanitizes `preset.name` before building the
  filename, matching load/save/delete. O-Wind's 8 factory names were already
  safe; this closes the gap fleet-wide for plugins that include the module
  header directly. (modules/persistence/preset-manager/cpp/OuariconPresetManager.h)

## [1.16.2] - 2026-07-10

### Fixed — CODE_REVIEW.md info-finding sweep (IN-02..07, IN-11, IN-18, IN-19)

Resolves 9 opt-in info findings from the 2026-07-09 deep review (selected via
/improve-review). No parameter, range, or state-format changes — PATCH.

- **IN-05 — CC overrides could never return to zero.** The `> 0.0f` test meant
  CC2=0 (breath fully off) fell back to the knob instead of silencing — a breath
  controller couldn't end a phrase. Replaced with per-controller "CC seen"
  latches: once CC2/CC74/CC1 sends any value it owns the destination, including 0.
  (FluteSynthVoice.{h,cpp})
- **IN-07 — FX `mix > 0.001` gating froze effect state and cut tails.** Automating
  a mix knob to 0 skipped `process()`, freezing delay/reverb buffers with content
  (hard-cut tail now, stale-audio replay when mix rose later). Chorus/delay/reverb
  now keep processing for a bounded tail-out after mix hits 0 (0.2 s / 10 s / 12 s)
  so state decays naturally, then the CPU-saving gate re-engages.
  (PluginProcessor.{h,cpp})
- **IN-03 — Per-voice oversampler never reset.** Voice cleanup and hard stopNote
  reset jet/bore/DC/delay but not `oversampling`; polyphase half-band state
  survived into the next note (sub-audible onset artifact). Added
  `oversampling.reset()` at both full-reset sites. (FluteSynthVoice.cpp)
- **IN-06 — `juce::Random::getSystemRandom()` on the audio thread** in startNote
  for the three vibrato/drift phases (shared global, not thread-safe against
  message-thread use inside JUCE). Now uses the voice's own `voiceRng`, matching
  the humanization draws. (FluteSynthVoice.cpp)
- **IN-02 — Dead silence-tracking counter removed.** `silentSampleCount` /
  `silentThreshold` were incremented/reset but never compared; the release-fade
  backstop is the actual cleanup mechanism. (FluteSynthVoice.{h,cpp})
- **IN-04 — Dead DSP scaffolding removed.** Deleted never-instantiated
  `ToneHoleSystem.h` and never-included `SubHarmonics.h` (subharmonics live
  inline in BoreWaveguide); removed the never-read bore delay lookup table
  (`buildBoreDelayTable`/`getDelayForNote`) and the 8 never-read
  `InstrumentPreset` fields (noiseLevel, noiseCutoffBase, boreLossCutoff,
  boreLossQ, embouchureMin/Max, defaultBreath, attackTimeMs). Factory presets no
  longer set the no-op `toneHoleToggle` (3 presets set it to 1.0, implying an
  effect that doesn't exist). The `toneHoleToggle` param and UI toggle are KEPT
  (removing the param would break sessions) and documented as a no-op pending
  tone-hole DSP — see NOTES.md.
- **IN-18 — Dead APVTS listeners removed** for `instrumentPreset` and
  `toneHoleToggle` (handlers were explicit no-ops; both are read per-block by the
  voice). (PluginProcessor.cpp)
- **IN-19 — Second file dialog dropped the first.** All six chooser native fns
  (save preset, load/save .scl, load/save .kbm, export HTML) shared one
  `fileChooser` member; launching a second replaced the shared_ptr so the first
  completion never fired, leaving its JS `await` pending forever. Added a
  `fileDialogOpen` re-entry guard — a second request completes immediately with
  the cancel value; the flag clears in each completion. (PluginEditor.{h,cpp})
- **IN-11 — Classic `<script src>` tag for the ES-module `juce/index.js` removed**
  (guaranteed `Unexpected token 'export'` console noise masking real errors; the
  module import on the next line is the real loader). (index.html)

Still deferred (info): IN-01 (per-block string-keyed APVTS lookups), IN-08,
IN-09, IN-10, IN-12 (hand-built JSON escaping), IN-13, IN-14, IN-15, IN-17
(shared-module factory-name sanitization). IN-16 was resolved by v1.16.1's
registry/NOTES updates.

## [1.16.1] - 2026-07-10

### Fixed — CODE_REVIEW.md resolution sweep (CR-01..08, WR-01..13)

Resolves all 8 critical and 13 warning findings from the 2026-07-09 deep code review.
IN-* info findings deferred (documented in NOTES.md).

**Critical:**

- **CR-01 — Entire Effects tab was dead.** The v1.14.0 release added 21 effects
  parameters and their JS bindings but never wrote the C++ relays/attachments, so
  every FX knob, bypass toggle, and the delay-mode dropdown were inert and (with all
  mix defaults at 0) the whole effects chain was unreachable from the UI. Added 16
  `WebSliderRelay`, 4 `WebToggleButtonRelay`, 1 `WebComboBoxRelay` + matching
  attachments in relay → webView → attachment order. (PluginEditor.{h,cpp})
- **CR-02 — Growl, Formant, Drift Depth/Speed knobs were dead** since v1.12.0 (same
  missing-relay class). Notably `formant` was stuck at 0.5, permanently applying a
  +3 dB headjoint peak that couldn't be turned off from the UI. Added 4 relays +
  attachments.
- **CR-03 — ~80 heap alloc/free pairs per audio block.** The voice filter-update path
  wrapped `ArrayCoefficients` results in temporary `IIR::Coefficients` objects, which
  heap-allocate twice per assignment — unconditionally, every block, all 8 voices.
  Now assigns the stack arrays in place (allocation-free after first use): both bore
  loss filters, end-reflection shelf, radiation HP (BoreWaveguide.h), inharmonicity
  allpasses (raw-array assign), and Strouhal bandpass (JetExciter.h).
- **CR-04 — Stuck drone when ADSR disabled mid-release.** `updateParametersFromAPVTS`
  now resolves a pending deferred jet release (calls `jetExciter.stopNote()`, clears
  `pendingJetRelease`, idles the stage) when `adsrEnabled` goes false, so releasing
  voices can't be orphaned at full level. (FluteSynthVoice.cpp)
- **CR-05 — Tuning state lost on session reload.** `setCustomStateCallbacks` was never
  registered, so Scala/KBM/embedded tunings, tonic, octave stretch, and mode silently
  reverted to 12-TET on reload. Registered the O-Lyrica-pattern save/load lambdas
  (intervals, scale name, tonic, built-in preset, octave stretch, mode + APVTS
  tuningSystem sync). (PluginProcessor.cpp)
- **CR-06 — Bundle reported version 1.0.0.** `juce_add_plugin` has no `PLUGIN_VERSION`
  keyword (silently dropped; fell back to the root project's 1.0.0), which also froze
  the factory-preset version sentinel forever. Changed to `VERSION "1.16.1"` — same
  fix as O-IntonationPad e87ae36. Factory presets regenerate once via the new stamp.
- **CR-07 — FileChooser `launchAsync` UAF on editor teardown.** All four completions
  (save preset, load .scl, load .kbm, export HTML) captured raw `this` and called
  `complete()` on every path. Now capture `Component::SafePointer` and bare-return
  when the editor is gone (calling `complete` there is itself a UAF — the callback is
  owned by the destroyed WebView). Fleet pattern from O-MicrotonalSampler v1.23.5 W12.
- **CR-08 — Rank-2 generator and Save SCL/KBM buttons dead.** Registered the three
  missing native fns (`generateRank2`, `saveScalaFile`, `saveKBMFile`) following the
  O-Bells implementations, with CR-07-style SafePointer completions.

**Warnings:**

- **WR-01 — Dblclick-reset ignored skew:** ADSR knobs reset to ~1 ms instead of
  10/100/200 ms. Reset now derives min/max/skew from backend `SliderState.properties`
  and applies the skew. (index.html)
- **WR-02 — Fixed delay-line capacity mistuned low notes at ≥96 kHz:** bore lines
  (2048) and jet line (1024) now sized from the prepared internal rate for MIDI 0.
  (BoreWaveguide.h, FluteSynthVoice.cpp)
- **WR-03/WR-04 — Audio-thread coeff allocations in EQProcessor and the formant
  filter:** switched `Coefficients::makeXXX` to in-place `ArrayCoefficients` assigns.
  (EQProcessor.cpp, PluginProcessor.cpp)
- **WR-05 — `airColumn` was computed then discarded (dead knob):** now wired as a bore
  geometry macro scaling the bore-loss corner ±½ octave, **neutral at the 0.5
  default** so factory/default timbre is unchanged; preset values 0.3–0.7 become
  audibly distinct. `updateBoreLossFilter` dropped its ignored `q` argument.
- **WR-06 — All 8 voices always ran the full 2× model:** `renderNextBlock` early-outs
  on `!isVoiceActive()` (state is reset at clear time).
- **WR-07 — FX delay capacity exceeded at high rates:** lines sized for the full 2.0 s
  range at the prepared rate; `setTime` clamps to capacity. (DelayProcessor)
- **WR-08 — Delay time / reverb size snapped instantly (clicks):** delay time now a
  ~30 ms `SmoothedValue` per sample; FDN tank delay lengths glide toward
  size-derived targets via ~50 ms one-pole. (DelayProcessor, ReverbProcessor)
- **WR-09 — Knob readouts derived from a hardcoded JS range map:** all readouts now
  use `SliderState.getScaledValue()`; FX knob ranges derive from backend-pushed
  `properties` with display-unit factors. PARAMS retained only for decimals/units and
  reset defaults. (index.html)
- **WR-10 — Reference-pitch knob always dragged from 440 and never synced:** shared
  module `tuning-panel.js` now tracks the current Hz, starts drags from it, and syncs
  from `getMasterTune` in `loadInitialState`. Fixed in
  `modules/tuning/scala-tuning-engine` (other plugins pick it up on their next
  build/sync); docstring also corrected to require the `Juce` ES-module namespace.
- **WR-11 — `setMasterTune` bypassed the `referencePitch` parameter** (panel setting
  snapped back to 440 on reload): the native fn now routes through
  `setValueNotifyingHost`; the existing listener updates the engine.
- **WR-12 — Save-preset dialog ignored the chosen directory:** saves inside the
  user-presets dir use `savePreset(name)`; anywhere else uses `savePresetToFile()`.
- **WR-13 — attackChiff/humanize/vibratoOnset/inharmonicity have no UI knobs:**
  documented as deliberate (host-automation/preset-only) in NOTES.md; UI knobs are a
  candidate future MINOR.

**Version:** 1.16.0 → 1.16.1 (PATCH — bug fixes only; no param IDs, ranges, or state
format changed. The `airColumn` activation is neutral at default.)

## [1.16.0] - 2026-04-26

### Added — VST3 Note Expression Microtonal Support for Dorico

O-Wind adds VST3 Note Expression microtonal support for Dorico. The plugin responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events). Composition order: TuningEngine → NE delta → pitch-bend → bore-delay derivation. The bore waveguide period sizes to the tuned frequency on the first sample (no attack zipper). End users must set Microtonality to "VST3 Note Expression" on the Dorico expression map.

**Files Modified:** `Source/PluginProcessor.{h,cpp}`, `Source/FluteSynthVoice.{h,cpp}`, `CMakeLists.txt` (added `PLUGIN_VERSION` line + `ouaricon_add_module(O-Wind note-expression)`).

**Version:** 1.15.1 → 1.16.0 (MINOR — new user-visible feature, backward compatible).

## [1.15.1] - 2026-04-13

### Fixed — ADSR Release Envelope Cut Short

The ADSR amplitude release was inaudible because the voice-clearing mechanism terminated the note ~60ms after noteOff, before the ADSR release could complete.

**Root cause:** The `releaseFading` cleanup in `renderNextBlock()` triggers when the JetExciter breath envelope drops below threshold (~50ms after noteOff) + a 10ms fade. This fired regardless of the ADSR release state, killing a voice that might have seconds of release remaining.

**Fix:** Gate the `releaseFading` trigger to defer while the ADSR is still in its Release stage. Once the ADSR envelope reaches zero (stage → Idle), the existing cleanup mechanism fires normally.

**Files Modified:** Source/FluteSynthVoice.cpp (1 condition added)

## [1.15.0] - 2026-04-13

### Improved — Sound Tab Layout

Rearranged the Sound tab to eliminate scrolling and remove wasted blank space.

**Layout changes:**
- Instrument strip (Tone Holes + Preset) moved to top as a compact bar
- ADSR Envelope moved into the 3-column grid (replaces Expression)
- Expression section now full-width — all 8 knobs in a single horizontal row
- Output and Impossible Physics remain as 2-column bottom row
- Tighter padding/margins scoped to Sound panel only (other tabs unaffected)

**Root cause:** Expression's 8 knobs were in the smallest grid column (0.8fr), causing 4 rows of wrapping (~400px) which forced the entire tab to scroll.

**Files Modified:** Resources/ui/index.html (CSS + HTML restructure)

## [1.14.0] - 2026-04-13

### Added — Effects Panel (Chorus, Delay, Reverb, EQ)

Added a full effects chain matching O-Lyrica's effects panel — 4 professional effects with 21 new parameters, replacing the "Coming Soon" placeholder in the Effects tab.

**Effects Chain (processing order: Chorus → Delay → Reverb → EQ):**

- **Chorus** — JUCE built-in chorus with Rate (0.1-10 Hz), Depth, Mix, and bypass toggle
- **Delay** — Lagrange-interpolated stereo delay with Time (1-2000 ms), Feedback (0-95%), Normal/PingPong mode, Mix, and bypass toggle
- **3-Band EQ** — Low shelf (200 Hz), parametric mid (200-8000 Hz), high shelf (8000 Hz), each ±12 dB, with bypass toggle
- **FDN Plate Reverb** — 8-channel Feedback Delay Network with Householder matrix, 4-stage input diffusion, Size, Damping, Pre-delay (0-200 ms), Modulation, Shimmer (octave-up feedback), Mix, and bypass toggle

**DSP:** Ported from O-Lyrica — DelayProcessor, EQProcessor, ReverbProcessor. All effects use atomic parameter caching for thread-safe real-time access. Each effect has a mix gate (skips processing when mix < 0.001) and bypass toggle. Tail length updated to 3.0s for reverb/delay tails.

**UI:** SVG vine-arc knobs (44×44px) with drag, scroll, and double-click-to-edit. Bypass toggles per effect (On/Off). Delay mode dropdown (Normal/PingPong). Matches O-Wind's naturalist aesthetic.

**Files Added:** DSP/DelayProcessor.h/.cpp, DSP/EQProcessor.h/.cpp, DSP/ReverbProcessor.h/.cpp
**Files Modified:** PluginProcessor.h, PluginProcessor.cpp, CMakeLists.txt, Resources/ui/index.html

## [1.13.0] - 2026-04-13

### Added — Optional ADSR Amplitude Envelope

Added a toggleable ADSR envelope that applies amplitude shaping on top of the physical model's natural breath dynamics. Disabled by default since it's non-physical, but useful for synth-style control.

**New Parameters:**
- **ADSR Enabled** (`adsrEnabled`, toggle, default OFF) — Enables/disables the ADSR envelope
- **ADSR Attack** (`adsrAttack`, 1ms-5s, default 10ms, skewed) — Attack time
- **ADSR Decay** (`adsrDecay`, 1ms-5s, default 100ms, skewed) — Decay time
- **ADSR Sustain** (`adsrSustain`, 0-100%, default 80%) — Sustain level
- **ADSR Release** (`adsrRelease`, 1ms-10s, default 200ms, skewed) — Release time

**DSP:** Linear ADSR state machine runs per-sample at 2x oversampled rate, applied as amplitude multiplier after the physical model output but before output gain. When disabled, multiplier is 1.0 (transparent passthrough). Disabling mid-note smoothly restores full level.

**UI:** New "ADSR Envelope" section at bottom of Sound tab with on/off toggle inline with the label. Knobs dim when ADSR is off. Time values display in ms below 1s, seconds above.

**Files Modified:** PluginProcessor.cpp, FluteSynthVoice.h, FluteSynthVoice.cpp, PluginEditor.h, PluginEditor.cpp, Resources/ui/index.html

## [1.12.0] - 2026-04-11

### Added — User-Controllable Vibrato Drift (Evolution) Parameters

Exposed the vibrato evolution system as two new user-facing parameters, allowing control over how organically the vibrato character wanders over time.

**New Parameters:**
- **Vibrato Drift Depth** (`vibratoDriftDepth`, 0-1, default 0.5) — Scales how much the vibrato rate and depth wander. 0 = perfectly static vibrato, 1 = full organic evolution with ±0.75 Hz rate drift and ±25% depth modulation.
- **Vibrato Drift Speed** (`vibratoDriftSpeed`, 0.1-2.0 Hz, default 0.4 Hz) — Controls how fast the vibrato character evolves. Lower values = slow, breath-like wandering; higher values = faster, more restless modulation.

**Implementation:** Two independent sine-wave drift oscillators modulate the vibrato LFO rate and depth per-sample. The rate drift oscillator runs at 1.175x the base speed, the depth drift at 0.775x, maintaining the original ~1.5:1 frequency ratio for natural-sounding decorrelation.

**UI:** Two new knobs added to the Expression section (Drift Depth, Drift Speed).

**Factory Presets:** All 8 presets updated with musically appropriate drift values — higher drift for organic instruments (Shakuhachi 0.7, Native Am. Flute 0.8), lower for precise ones (Recorder 0.2, Piccolo 0.3).

**Files Modified:** PluginProcessor.cpp, FluteSynthVoice.cpp, FluteSynthVoice.h, Resources/ui/index.html

## [1.11.6] - 2026-04-11

### Fixed — Tuning Panel Layout

Tuning tab had incorrect layout — viz mode tabs (Circle, Polar, etc.) and visualization content were misplaced in the CSS grid, pushing the tuning library/controls to a second row instead of the rightmost column.

**Root Cause:** O-Wind used the shared tuning module's CSS grid layout (`grid-template-columns: 140px 1fr 200px`) with 4 direct grid children, causing auto-placement to put viz-container in the right column and controls on a second row.

**Fix:** Added absolute positioning CSS overrides in index.html matching the O-Prism/O-Lyrica pattern: interval list (left), viz tabs + content (center), controls panel (right).

**File Modified:** Resources/ui/index.html

## [1.11.5] - 2026-04-11

### Fixed — Instrument Plays Flat (Missing Embouchure Sign Inversion)

Notes played consistently flat compared to other instruments at the same MIDI note and tuning settings.

**Root Cause:** The waveguide feedback loop had only ONE sign inversion (at the open/far end via `-endReflectionCoeff`), but a flute is an open-open tube requiring TWO inversions per round trip. The bore feedback entered the jet exciter with positive coupling, creating a closed-open (clarinet) loop topology where the linear resonance is at `sampleRate/(2D)` instead of `sampleRate/D`. The tanh nonlinearity forced oscillation near the target pitch, but with a systematic flat offset because the loop phase was π off from proper resonance.

**Fix:** Negated bore feedback before jet exciter coupling (`-boreFeedback` instead of `boreFeedback`). This adds the physically correct embouchure-end pressure inversion (open end = pressure node). Combined with the far-end inversion, the loop now has two sign inversions per round trip — the correct open-open flute topology where `f = sampleRate/D`.

**Files Modified:** FluteSynthVoice.cpp

## [1.11.4] - 2026-04-09

### Fixed — Physical Model Instability, Distortion, and Excessive Output Level

Waveguide now oscillates cleanly without octave jumping, clipping distortion, or excessively hot output.

**Root Cause 1 — Jet amplification (mu) 10-20x too high:** `jetAmplification` values across all 8 instrument presets (12.0-35.0) produced small-signal loop gains of ~26-36x. A stable waveguide model needs 3-7x. The massively overdriven loop caused deep tanh saturation every cycle (square-wave-like oscillation), mode-locking onto the 2nd harmonic (octave jumping), and signals exceeding safety clamps.

**Fix 1:** Scaled `jetAmplification` down by ~5x across all presets (Concert Flute 25→5, Recorder 35→7, Piccolo 30→6, etc.) to bring loop gain into the 3-7x range.

**Root Cause 2 — Hard clip before tanh nonlinearity:** `JetNonlinearity::processSample()` hard-clamped input to ±3.0 before the tanh soft-limiter. With the high jet amplification, excitation signals regularly exceeded ±3.0 and were hard-clipped, generating harsh harmonics that the tanh was supposed to prevent.

**Fix 2:** Widened safety clamp from ±3.0 to ±10.0 so the tanh does the actual soft-limiting.

**Root Cause 3 — Hard safety clip in voice output:** `renderNextBlock()` used `jlimit(-2.0, 2.0)` as a safety clip on the oversampled signal. Hard discontinuities in the oversampled domain create aliasing artifacts after downsampling.

**Fix 3:** Replaced hard clip with `tanh(sample * 0.5) * 2.0` soft-clip — transparent below ±1.5, gentle compression above, no hard edges.

**Output level reduction:** Added -6 dB fixed voice attenuation (`* 0.5f`) before the output gain stage. Combined with the reduced jet amplification, raw waveguide output now sits at a comfortable level.

**Files Modified:** DSP/InstrumentPresets.h, DSP/JetNonlinearity.h, FluteSynthVoice.cpp

## [1.11.3] - 2026-04-09

### Fixed — Tuning Changes Not Affecting Pitch (Two Root Causes)

Selecting an embedded tuning now correctly changes the instrument's pitch — both for new notes and held notes.

**Root Cause 1 — Mode never switched to Scala:** `TuningEngine::setCustomIntervals()` stored new scale intervals but never switched `currentMode` from `TwelveTET` to `Scala`. Since `rebuildFrequencyTable()` only uses custom intervals in `Scala` mode, the new intervals were silently ignored.

**Fix 1:** Added `currentMode.store(Mode::Scala)` in `setCustomIntervals()`, consistent with `setSingleInterval()` which already auto-switched.

**Root Cause 2 — Voice frequency only queried at note-on:** `FluteSynthVoice` called `tuningEngine->getFrequency()` only in `startNote()`, never per-block. Tuning changes during held notes had no effect on bore delay, so pitch stayed locked to the note-on frequency.

**Fix 2:** Added per-block tuning re-query in `updateParametersFromAPVTS()` — re-reads `tuningEngine->getFrequency(currentMidiNote)` every block and smoothly updates `totalDelaySmoothed` when the frequency changes. Matches O-Reed's `getBaseFrequencyFromTuning()` pattern.

**Files Modified:** modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp (shared module), FluteSynthVoice.cpp

## [1.11.2] - 2026-04-07

### Fixed — Tuning Module Not Affecting Pitch

Tuning panel changes (reference pitch, temperament presets, Scala files, custom intervals) now correctly affect played notes. Previously, all tuning modifications were immediately overwritten and had no audible effect.

**Root Cause:** `processBlock()` contained a block-time sync that overwrote the TuningEngine state every audio callback with stale APVTS default values (440 Hz / 12-TET). The WebView tuning panel updates the TuningEngine directly via native functions — but the block-time sync clobbered those changes ~1000x/second, making every tuning modification inaudible.

**Fix:** Removed the block-time APVTS→TuningEngine sync from `processBlock()`. The TuningEngine is now the source of truth, updated directly by native functions (UI) and `parameterChanged` listener (automation/presets).

**Files Modified:** PluginProcessor.cpp

## [1.11.1] - 2026-04-07

### Fixed — Waveguide Pitch Tracking Accuracy

MIDI notes now produce correct pitches across the entire range. Previously, played notes were audibly sharp — worst at low pitches (~70 cents at C4) and negligible at high pitches (~3 cents at C6).

**Root Cause:** Two uncompensated delay sources in the waveguide feedback loop:
1. **Implicit 1-sample feedback delay** — `boreWaveguide.getFeedback()` returns the value computed in the *previous* iteration, adding 1 sample to the loop that was never subtracted from `totalDelay`.
2. **DC blocker phase advance** — The DC blocker (`y[n] = x[n] - x[n-1] + 0.995*y[n-1]`) has significant frequency-dependent phase advance at audio frequencies relative to the 88.2kHz internal rate (e.g. -5 samples at A4, -14 samples at C4), but was excluded from `getFilterPhaseDelay()`.

**Fix:** Extended the dynamic loop delay compensation in `updateParametersFromAPVTS()` to include the DC blocker's phase delay and the implicit 1-sample feedback delay alongside the existing bore filter compensation. Added `DCBlocker::getPhaseDelay()` method for frequency-dependent phase delay calculation.

**Files Modified:** DSP/DCBlocker.h, FluteSynthVoice.cpp

## [1.11.0] - 2026-04-07

### Added — Phase-Locked Vibrato Tremolo

Amplitude modulation locked to vibrato LFO phase — replicates the natural coupling between pitch and loudness variation heard in real flute playing. Highest pitch = loudest, lowest pitch = softest.

**New APVTS Parameter:**
- `vibratoTremolo` (0.0-1.0, default 0.0) — tremolo depth, at max produces ±2.5 dB amplitude variation

**DSP Implementation (FluteSynthVoice — renderNextBlock):**
- Reuses the existing vibrato LFO signal (including onset ramp, rate drift, depth drift, and asymmetric shape)
- Computes `tremoloGain = 1.0 + depth * depthScale * onsetGain * vibratoShape * 0.3` per sample
- Applied to output after outputGainLinear, before safety clip
- Zero CPU cost when vibratoTremolo = 0

**UI:** Existing "Vib Depth" knob renamed to "Vib Pitch" for clarity. New "Vib Tremolo" knob added to Expression section.

**Factory Presets:** All 8 presets updated with musically appropriate tremolo depths (0.05-0.25).

**Files Modified:** PluginProcessor.cpp, FluteSynthVoice.h, FluteSynthVoice.cpp, PluginEditor.h, PluginEditor.cpp, Resources/ui/index.html

## [1.10.1] - 2026-04-07

### Fixed — Air Column Parameter No Longer Bends Pitch

Removed erroneous cutoff frequency reduction from air column parameter in bore loss filter. Air column was modifying the filter cutoff by up to 70%, which changed the filter's phase delay and shifted pitch through the dynamic delay compensation loop.

**Root Cause:** `cutoffReduction = 1.0 - airColumn * 0.7` was multiplied into `lossCutoff`, causing phase delay changes that the delay compensation subtracted from the total loop delay, bending the fundamental frequency.

**Fix:** Air column now only controls the Q (rolloff steepness) of the bore loss filter as originally intended by the architecture. Tone color remains the sole control for bore loss cutoff frequency.

**Files Modified:** FluteSynthVoice.cpp

## [1.10.0] - 2026-04-06

### Added — Allpass Inharmonicity Filters

Bore waveguide allpass inharmonicity for natural partial detuning and conical bore approximation per RESEARCH-realism-v2.md §2.6:

**New APVTS Parameter:**
- `inharmonicity` (0.0-1.0, default 0.2) — controls allpass partial detuning amount

**DSP Implementation (BoreWaveguide — processSample):**
- Two cascaded first-order allpass filters in bore backward delay path (after end reflection, after backward delay pop)
- Allpass coefficient `a = effective * 0.05`, where `effective = APVTS_param * preset.inharmonicityBase`
- Frequency-dependent phase delay detunes upper harmonics by ~5-15 cents — matching measured flute inharmonicity
- Coefficients updated once per block via `setInharmonicity(effective)` with change detection
- Allpass phase delay included in `getFilterPhaseDelay()` for dynamic loop delay compensation
- Zero CPU cost when inharmonicity = 0 (filters bypassed)

**Per-Instrument Preset Base Values (InstrumentPresets — inharmonicityBase):**
- Concert Flute: 0.15 (cylindrical bore, minimal inharmonicity)
- Shakuhachi: 0.5 (conical bore, high inharmonicity)
- Bansuri: 0.35 (bamboo bore irregularities)
- Native Am. Flute: 0.4 (dual-chamber conical bore)
- Recorder: 0.3 (slightly tapered bore)
- Pan Flute: 0.35 (closed-end cylindrical, end correction effects)
- Piccolo: 0.2 (short conical bore)
- Ocarina: 0.4 (Helmholtz resonator, inherently inharmonic)

**Files Modified:** BoreWaveguide.h, InstrumentPresets.h, PluginProcessor.cpp, FluteSynthVoice.cpp

## [1.9.0] - 2026-04-06

### Added — Register-Dependent Spectral Shaping

Automatic pitch-aware timbral adaptation per RESEARCH-realism-v2.md §2.5:

**Bore Loss Filter — Register Cutoff Modulation (FluteSynthVoice — updateParametersFromAPVTS):**
- Cutoff multiplier formula: `0.6 + (currentMidiNote / 127.0) * 0.8`
- Applied to toneColor-derived cutoff after material scaling, before `boreWaveguide.updateBoreLossFilter()`
- Low notes (C3, MIDI 48): ~0.9x multiplier — richer harmonic content relative to fundamental
- High notes (C7, MIDI 96): ~1.2x multiplier — purer tone relative to fundamental
- Smooth continuous scaling across full MIDI range

**Breath Noise — Inverse Register Scaling:**
- Noise multiplier formula: `1.4 - (currentMidiNote / 127.0) * 0.8`
- Applied to breathNoise parameter before `jetExciter.setBreathNoise()`
- Low notes: ~1.1x noise gain — more audible breath turbulence
- High notes: ~0.8x noise gain — cleaner, less breathy tone
- Symmetric inverse of bore loss scaling

**No new APVTS parameters** — fully automatic behavior driven by current MIDI note number. Zero additional CPU cost (two multiply-adds per block).

## [1.8.0] - 2026-04-06

### Added — Material Macro Parameter

Continuous wood-to-metal timbral blending macro per RESEARCH-realism-v2.md §2.3:

**New APVTS Parameter:**
- `material` (0.0-1.0, default 0.5) — timbral macro (0 = dark wood/bamboo, 1 = bright metal)

**DSP Implementation (FluteSynthVoice — updateParametersFromAPVTS):**
- Bore loss filter cutoff multiplier: 0.6x (wood) to 1.4x (metal) of base toneColor cutoff
- Strouhal noise bandpass center freq: same 0.6x-1.4x scaling (darker/brighter turbulence)
- Radiation filter cutoff: 0.7x (wood) to 1.3x (metal) of preset base cutoff
- End reflection filter cutoff: same 0.7x-1.3x scaling as radiation
- End reflection coefficient: nudged +/-0.05 from base (metal = more reflective, wood = more damped)
- No new DSP components — multipliers applied in updateParametersFromAPVTS before passing to existing filters
- Works as additive offset on top of instrument presets (material=0.5 = no change from preset base)

**WebView UI:**
- New "Material" knob added to Resonator section on SOUND tab (first position)
- All 8 factory presets updated (material=0.5 default — neutral, no timbral offset)

## [1.7.0] - 2026-04-06

### Added — Headjoint Formant Resonance Filter

Models the characteristic resonant peak of the headjoint/embouchure cavity in the radiation output path per RESEARCH-realism-v2.md §2.4:

**New APVTS Parameter:**
- `formant` (0.0-1.0, default 0.5) — formant resonance prominence (0 = flat/0dB, 1 = full/+6dB)

**DSP Implementation (PluginProcessor):**
- Stereo IIR biquad parametric EQ (Q = 1.5) applied post-StereoWidth, pre-output
- Center frequency is preset-dependent via new `formantCenterHz` field in InstrumentPreset:
  - Piccolo: 4000 Hz, Recorder: 3000 Hz, Concert Flute: 2500 Hz
  - Pan Flute: 2200 Hz, Bansuri: 2000 Hz, Ocarina: 2000 Hz
  - Shakuhachi: 1800 Hz, Native Am. Flute: 1500 Hz
- Gain mapped linearly: formant=0 → 0dB (flat), formant=1 → +6dB
- Coefficients updated only on parameter or preset change (not per-sample)
- Processing skipped entirely when gain < 0.05dB (formant near 0)

**WebView UI:**
- New "Formant" knob added to Output section on SOUND tab
- All 8 factory presets updated (formant=0.5 default)

## [1.6.0] - 2026-04-06

### Added — Growl Effect (Vocal-Fold Coupling)

Models growl/roughness via secondary low-frequency sawtooth oscillator modulating bore feedback per RESEARCH-realism-v2.md §2.2:

**New APVTS Parameter:**
- `growl` (0.0-1.0, default 0.0) — growl depth (0 = off, 1 = full roughness)

**DSP Implementation (FluteSynthVoice):**
- Sawtooth oscillator (70-120 Hz, randomized per-note) modulates bore feedback signal
- Formula: `boreFeedback *= (1.0 - growl * 0.6 * sawPhase)` where `sawPhase` ramps 0→1
- At growl=0: multiplier is 1.0 (no effect, zero CPU cost via early-exit)
- At growl=1: bore feedback reduced up to 60% at sawtooth peak, creating characteristic roughness
- Per-note frequency randomization (70-120 Hz range) + random start phase via per-voice RNG
- Simulates vocal-fold coupling where sub-glottal turbulence modulates the air column

**WebView UI:**
- New "Growl" knob added to Expression section on SOUND tab
- All 8 factory presets updated (growl=0 — articulation effect, not default sound)

## [1.5.0] - 2026-04-06

### Added — Flutter Tongue Articulation

Models flutter tongue (Flatterzunge) via amplitude modulation of breath pressure per RESEARCH-realism-v2.md §2.1:

**New APVTS Parameters:**
- `flutterTongue` (0.0-1.0, default 0.0) — flutter tongue AM depth (0 = off, 1 = full modulation)
- `flutterRate` (15-30 Hz, default 22 Hz) — flutter tongue oscillation rate

**DSP Implementation (JetExciter):**
- In `processSample()`, effectivePressure is multiplied by `(1 - ft + ft * (0.5 + 0.5 * sin(phase)))` where `ft` = flutterTongue
- At flutterTongue=0: multiplier is 1.0 (no effect, zero CPU cost via early-exit)
- At flutterTongue=1: full AM from 0 to 1× breath pressure at flutter rate
- Per-cycle rate randomization (+/-5%) applied at each phase wraparound for naturalism
- Jittered rate stored per-voice, no per-sample random calls

**WebView UI:**
- Two new knobs added to Expression section on SOUND tab: "Flutter" and "Flut Rate"
- All 8 factory presets updated (flutterTongue=0, flutterRate=22 Hz — articulation, not default sound)

## [1.4.0] - 2026-04-06

### Added — Per-Note Humanization System

Eliminates "machine gun" effect on repeated notes per RESEARCH-realism-v2.md §1.3:

**New APVTS Parameter:**
- `humanize` (0.0-1.0, default 0.3) — master scale for all per-note randomization amounts

**Per-Note Random Offsets (drawn at each noteOn via per-voice juce::Random):**

| Offset | Range | Effect |
|--------|-------|--------|
| Attack time | +/-20% of base | No two attacks identical |
| Noise burst amplitude | +/-30% of chiff level | Varied chiff intensity |
| Embouchure delay | +/-1% of bore delay | Slight timbre shift per note |
| Strouhal noise center | +/-10% of center freq | Subtle breath color variation |
| Vibrato onset delay | +/-50ms | Natural onset variation |

**Implementation Details:**
- Offsets stored as member variables in FluteSynthVoice, applied continuously per note
- Attack time and noise burst scales passed to JetExciter::startNote()
- Embouchure offset applied as bore delay multiplier in render loop
- Strouhal freq scale passed to JetExciter::updateStrouhalBandpass()
- Vibrato onset offset added to base vibratoOnset parameter (clamped >= 0)
- All offset magnitudes scale linearly with humanize parameter (0 = no randomization)
- Zero per-sample CPU cost — random numbers drawn only at noteOn

## [1.3.0] - 2026-04-06

### Improved — Vibrato Humanization

Replaces mechanical sine-wave LFO vibrato with organic, human-like modulation per RESEARCH-realism-v2.md §1.2:

**Delayed Onset (FluteSynthVoice):**
- Vibrato depth ramps linearly from 0 to target over configurable onset delay (0-1000ms)
- Prevents instant vibrato on note attack — matches real flautist technique where vibrato develops after tone stabilizes

**Rate Drift:**
- Slow oscillator (~0.47 Hz) modulates LFO rate by +/- 0.75 Hz
- Per-note random drift phase eliminates locked periodicity between notes

**Depth Drift:**
- Independent slow oscillator (~0.31 Hz) modulates vibrato depth +/- 25%
- Creates natural amplitude variation in vibrato intensity

**Shape Asymmetry:**
- Vibrato waveform changed from `sin(phase)` to `sin(phase) + 0.1*sin(2*phase)`
- Adds slight second-harmonic content matching diaphragm-driven vibrato asymmetry

**Random Initial Phase:**
- Vibrato LFO starts at random phase on each noteOn
- Eliminates phase-locked vibrato across simultaneous or sequential notes

**New APVTS Parameter:**
- `vibratoOnset` (0-1000ms, default 300ms) — delay before vibrato ramp-in after noteOn

**Factory Preset Values:**
- Shakuhachi: 500ms, Pan Flute: 450ms, Native Am. Flute: 400ms, Bansuri: 350ms, Ocarina: 350ms
- Concert Flute: 300ms (default), Piccolo: 250ms, Recorder: 200ms

## [1.2.0] - 2026-04-06

### Added — Attack Transient "Chiff" Modeling

Models the four-phase flute onset (noise burst, vortex shedding, edge-tone, pipe-tone) per Auvray et al. (2014):

**Chiff Noise Burst (JetExciter):**
- On noteOn, turbulence noise is boosted 3-6x above steady-state level (scaled by `attackChiff * velocity`)
- Exponential decay over 20-40ms (faster at higher velocity) back to normal noise floor
- Near-zero CPU cost — envelope-gated, active only during first ~40ms per note

**Pitch Overshoot (FluteSynthVoice):**
- Bore delay starts 1-2% shorter than target at noteOn (sharper pitch), scaled by `attackChiff * velocity`
- Settles to target pitch over 50-100ms via one-pole smoothing filter
- Models jet-bore coupling delay before pipe-tone regime locks

**New APVTS Parameter:**
- `attackChiff` (0.0-1.0, default 0.5) — controls transient noise burst intensity and pitch overshoot amount

**Factory Preset Values:**
- Shakuhachi: 0.70 (prominent chiff), Pan Flute: 0.65, Bansuri: 0.55
- Concert Flute: 0.50, Piccolo: 0.45, Native Am. Flute: 0.40
- Recorder: 0.25, Ocarina: 0.15 (minimal chiff)

## [1.1.0] - 2026-04-06

### Improved — Noise Model & Spectral Realism

Three-pass physical model refinement for more realistic flute timbre:

**Pass A: Jet-Bore Energy Balance**
- Replaced single 2nd-order lowpass bore loss filter with two cascaded 1st-order lowpass filters (~2kHz base damping + ~8kHz harmonic rolloff) for frequency-dependent viscothermal loss — higher harmonics now lose more energy per round trip, creating natural spectral thinning
- Lowered radiation filter cutoff across all presets (concert flute 300→150Hz, proportional for others) to retain more fundamental energy in low notes

**Pass B: Register Transitions**
- End reflection filter replaced with high-shelf (-6dB above ~2kHz) — higher frequencies reflect less from the open bore end, modeling realistic radiation impedance and improving octave transition behavior

**Pass C: Noise & Spectral Realism**
- Replaced single lowpass noise filter with 2nd-order bandpass centered on Strouhal frequency (f_s = 0.2 × jet_velocity / jet_diameter) — turbulence spectrum now physically tracks breath pressure
- Added `jetDiameter` to InstrumentPreset struct (~7mm piccolo to ~15mm shakuhachi) for per-instrument Strouhal tuning

## [1.0.1] - 2026-04-05

### Fixed

- **Stuck voice / infinite sustain bug:** Notes triggered on note-off and sustained forever
  - **Root cause 1:** `JetNonlinearity` velocity floor (`max(0.01, vel)`) provided residual drive to bore waveguide even after breath stopped, keeping the feedback loop alive indefinitely
  - **Root cause 2:** Voice clearing required waveguide silence (`silentSampleCount >= 512`), which never occurred due to the residual drive, trapping voices in permanent "releasing" state
  - **Fix:** Gate nonlinearity output to zero when jet velocity < 0.001 (breaks feedback loop on release); add 10ms release tail fade with guaranteed voice clearing after breath envelope completes

## [1.0.0] - 2026-04-05

- Initial release: Physical modeling flute synthesiser (Verge 1995 jet-drive model)
