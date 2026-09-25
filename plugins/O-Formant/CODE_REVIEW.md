---
phase: O-Formant code review
reviewed: 2026-09-24T00:00:00Z
version_reviewed: 1.29.0
depth: deep
files_reviewed: all Source/*.cpp|h, Source/dsp/*.cpp|h, Source/ui/public/index.html, js/main.js, js/i18n.js, js/tuning-panel.js, modules/tuning/note-expression (as consumed)
method: 5 parallel subsystem reviewers (RT path, source+formant DSP, consonant+FX DSP, tuning+presets, WebView UI); every Critical re-verified against source on the main thread; CR-01 verified numerically (float32 port of the solver)
findings:
  critical: 9
  warning: 20
  info: 24
  total: 53
status: issues_found
verified: 2026-09-24 — v1.31.1 (IN-03/07/09/16/19/20/21-save/22a,c/23a,b,d,e/24g-k) via /improve-verify; previously v1.31.0 (CR-01, WR-01, WR-11, WR-15, WR-17)
supersedes: .planning/REVIEW.md (v1.25.0) — its CR-01/03/04/05, WR-01..08 are resolved (fixes re-verified correct); its CR-02, WR-10, WR-12 were still open and are carried forward here as CR-08 / CR-09
---

# O-Formant: Code Review Report (v1.29.0)

## Carry-over from the v1.25.0 review

| Prior ID | Status |
|---|---|
| CR-01 bend, CR-03 embedded period, CR-04 preset path, CR-05 12-entry wipe | ✅ fixes verified correct |
| WR-01..08, IN-02/03/06/11/14/15/16/19 | ✅ fixes verified correct (WR-06 denormal flush survives the build — no `-ffast-math`/`-Ofast`/`/fp:fast` anywhere) |
| **CR-02** `.kbm` map size unbounded | ❌ **still open** → CR-08 |
| **WR-10 + WR-12** rank-2 clamp order + `reduceAndSort` loop | ❌ **still open**, and together they are a reachable UI hang → CR-09 |
| WR-09 HTML injection | half open — WebView side safe (`textContent`), HTML exporter still unescaped → WR-20 |
| WR-11, WR-13 | open but unreachable / export-only → IN-level |

Known-ecosystem gotchas checked: **preset-manager stale `<CustomState>` — NOT hit** (O-Formant uses its own get/setState, never `setStateFromXml`); XML string round-trip — handled; factory-preset skew — solid (`convertTo0to1`); preset `/` sanitizing — solid; **reset-to-defaults — FAILS** (WR-09); **outputGain in presets — FAILS** (WR-09); **per-block ADSR `setParameters` — FAILS** (CR-02).

---

## Critical

### CR-01 — Glottal alpha solver converges to a false root for Rd ≤ ~0.8: the pressed half of Voice Quality is the wrong waveform
**File:** `Source/dsp/GlottalTableGenerator.cpp:74-118` (`solveAlpha`), consumed at `:166`, `:187`
`f(α) = (e^{αTe}(α sinωTe − ω cosωTe) + ω)/(α²+ω²)` → 0 as α → −∞, so Newton runs to α ≈ −4000…−7900 and passes `|f| < 1e-6`. **Verified numerically** (float32 port, scratchpad `lf.py`): Rd steps 0-57 (Rd 0.30-0.80) all return α < 0; at Rd 0.8 α=46 with an open-phase end of −3.9e12. `exp(α·Te)` underflows → `Ee = 0` → the `Ee = -1` fallback → near-silent open phase + a **positive-polarity** spike at Te, opposite to every other step.
**Impact:** every pressed setting (factory preset with `glottalRd 0.4`, `PluginProcessor.cpp:482`) renders the wrong timbre; the `rdFrac` crossfade near Rd ≈ 0.8 blends opposite-polarity frames → level dip + timbre flip when Rd is modulated across it.
**Root cause:** the area balance integrates only the open phase; the LF model balances open + return over the whole period, with α > 0.
**Fix:** solve the full-period zero-area condition with a bracketed bisection/Brent search restricted to α > 0; `jassert` convergence. Re-renders the whole table → timbre change across Rd → MINOR bump + preset listen pass.

### CR-02 — `adsr.setParameters` every block kills / distorts the release
**File:** `Source/FormantVoice.cpp:384-392`
JUCE 8 `ADSR::recalculateRates()` sets `releaseRate = sustain / (release·sr)`, overwriting the level-based rate `noteOff()` computed. Sustain = 0 → release rate 0 → the envelope ends and resets one block after note-off (click, dead stop) for any note released during attack/decay. Sustain > 0 → wrong release duration (released at 1.0, sustain 0.1, release 10 s → ~100 s tail, eating voices, see WR-03). Same defect as `pattern_adsr_setparameters_per_block_kills_release`, fixed elsewhere in the suite, never here.
**Fix:** cache the last `ADSR::Parameters`; call `setParameters` only on change **and** when not releasing; apply deferred params at the next note-on.

### CR-03 — Formant transition smoothers are `reset()` every block: glides truncated, sound depends on buffer size
**File:** `Source/FormantVoice.cpp:527-528` → `FormantFilterBank.h:64-73`, `CascadeFormantBank.h:67-76`
`setTransitionTime` → `SmoothedValue::reset(sr, t)`, which snaps current = target (`juce_SmoothedValue.h:274-277`). Every ramp is cut at the block boundary. /a/→/i/ at Transition 0.4 should glide F1 800→300 Hz over ~20 ms; at 256-sample/48 kHz it ramps 5.3 ms then jumps; at 64 samples the change is near-instant. Transition knob barely works; XY-pad / MPE timbre moves zipper; offline ≠ realtime. Also means the per-sample smoothing path never runs, so cascade coefficients step every `kCoeffUpdateInterval` (32) samples.
**Fix:** only call `setTransitionTime` when the value changes, and set ramp length without `reset()` (store step count, apply on next `setTargetValue`).

### CR-04 — Consonant Atk / Hold / Decay knobs are dead (no relays)
**File:** `Source/ui/public/js/main.js:308-310` vs `Source/PluginEditor.{h,cpp}` (relay list `:28-85`)
JS requests `consonantAttackSlider`, `consonantHoldSlider`, `consonantDecaySlider`; C++ creates no relay/attachment for them (**verified**: zero hits in `PluginEditor.*`). The params exist and the DSP reads them (`FormantVoice.cpp:64-66`). With Auto off, moving them does nothing, and readouts flip from "20 ms" to "0.00" ~100 ms after load. Previously noted at CHANGELOG.md:285, never fixed.
**Fix:** add three `WebSliderRelay` (declared before `webView`), `.withOptionsFrom(...)`, and three `WebSliderParameterAttachment` (declared after `webView`).

### CR-05 — Preset Save is a no-op on macOS (`prompt()` unsupported)
**File:** `Source/ui/public/js/main.js:1358-1359`
JUCE's macOS `WKWebView` delegate implements only the file-open panel (`juce_WebBrowserComponent_mac.mm:315,574`), no `runJavaScriptTextInputPanel`, so `prompt()` returns `null` → the handler returns. Save silently does nothing in AU / VST3 / Standalone on macOS. CHANGELOG.md:359 called this "likely". *Confirm with one Standalone click before fixing.*
**Fix:** inline name field in the preset bar, or a native `promptPresetName` backed by a JUCE `AlertWindow` that `complete`s on close.

### CR-06 — Lyrics only have syllables when the editor is open
**File:** `Source/PluginProcessor.cpp` `setStateInformation` (lyrics block, ~1040); `main.js:1678-1686`; `LyricsEngine.h:71`
Only the page's JS parser (`parseArpabet` → `setLyrics`) turns text into syllables; state restores text only. Reload a session and bounce offline without opening the UI → lyrics mode has 0 syllables. Opening the editor re-sends syllables and `setSyllables` resets position to 0, restarting the lyric mid-song. Position is also never reset on transport start, so two bounces can start on different syllables.
**Fix:** persist the parsed syllable targets in the `lyricsEngine` child (or port the parser to C++); on editor open only redraw — don't re-send if unchanged; reset position on transport start.

### CR-07 — Loading a `.kbm` clamps its reference frequency to 400–480 Hz and overwrites A4
**File:** `Source/TuningEngine.cpp:600-603` → `setMasterTune` `:103`; used at `:857`
**Verified.** A standard KBM (ref note 60 @ 261.6256 Hz) is clamped to 400 Hz → every note ~735 cents sharp. It also clobbers the user's A4, which `getStateInformation` then saves as `masterTune=400` while the KBM itself is not saved (WR-06) → the reopened session plays linear mapping at A=400.
**Fix:** separate `kbmReferenceFreq` member (clamp ~1–20000 Hz) used at `:857`; leave `a4Frequency` untouched.

### CR-08 — Malformed `.kbm` map size → unbounded allocation → crash *(prior CR-02, still open)*
**File:** `Source/TuningEngine.cpp:552` → `:568` → fill loop `:581-584` → stored raw `:589`
Map size `2000000000` → ~2 billion `push_back` on the message thread → `bad_alloc` / OOM.
**Fix:** `newMapSize = jlimit(0, 128, newMapSize)`; `newOctaveDegree = jlimit(1, 128, …)`.

### CR-09 — Rank-2 generator can hang the DAW UI *(prior WR-10 + WR-12)*
**File:** `Source/ScaleGenerator.cpp:78-79`, `:101-109`; inputs from `tuning-panel.js:885-888`
HTML `min`/`max` don't bound typed values and JS doesn't clamp. Generator 1e12 / period 1e13: the generator is clamped against the *unclamped* period (:78), then `reduceAndSort` subtracts 2400 per iteration (~6e9 iterations); above ~4e19 `c - 2400 == c` and it never terminates. Runs on the message thread.
**Fix:** clamp period first, then generator to `[1, period-1]`; in `reduceAndSort` guard `period <= 0` and use `c = fmod(c, period); if (c < 0) c += period;`. Clamp in JS too.

---

## Warning

### WR-01 — Breathy end (Rd ≥ ~2.1) has no ε root → abrupt closure
`GlottalTableGenerator.cpp:47-57`, `:121-148`. The OQ regression reaches 0.98 at Rd 2.7 (Fant gives ~0.79), so `Tc−Te < Ta` and `1−e^{−εd} = εTa` has no positive root; Newton parks at the `ε = 0.1` floor. Return phase starts at 17–80 % of `-Ee` → a step at Te (83 % of peak at Rd 2.7). Breathiest setting has the *sharpest* closure (backwards). Affects `glottalRd 2.5` preset. **Fix:** Fant's `Rg = 0.25·Rk / (0.11·Rd/(0.5+1.2·Rk) − Ra)`; clamp `Ta ≤ 0.9·(Tc−Te)` before `solveEpsilon`. Ship with CR-01.

### WR-02 — Mid-burst manner change overflows the burst envelope
`ConsonantEngine.h:155-158`, `:274-276`; `updateCoefficients` runs every block (`FormantVoice.cpp:500`). **Verified.** Burst length/decay are recomputed while a burst is live: manner 1→0 one block in at 44.1 kHz → progress ≈ −7.6 → `exp(90.8)` = inf → ~68 ms full-scale noise into the frication bank (or NaN → voice reset). Lyrics mode is safe. **Fix:** latch `burstTotalSamples` and decay rate in `triggerBurst()`.

### WR-03 — Voice stealing disabled; notes silently dropped
`PluginProcessor.cpp:714-726`. `MPESynthesiser` defaults to no stealing; 16 voices held through releases up to 10 s. **Fix:** `setVoiceStealingEnabled(true)` (prefer released voices) + a short steal fade — `noteStopped(false)` currently hard-resets the envelope (`FormantVoice.cpp:338-343`).

### WR-04 — Pitch-bend range is fixed at ±2 st; `tuning_*` params are inert
`PluginProcessor.cpp:726` (`enableLegacyMode(2, …)`); `tuning_pitchBendRange` only reaches `TuningEngine::pitchBendRange`, used via `setPitchBend()` which nothing calls. The five `tuning_*` params are host-automatable but only read in `setStateInformation`, where saved engine values override them. **Fix:** compute bend from `note.pitchbend.asSignedFloat() * range` with a cached param pointer (never call `setLegacyModePitchbendRange` on the audio thread — it releases all notes); either wire or hide (non-automatable) the other `tuning_*` params.

### WR-05 — note-expression module still allocates on the audio thread (module-wide)
`modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp:209` builds a `std::map<int32_t,int> noteIdToPitch` on every block containing a VST3 note-on. Prior WR-02 fix covered only `push_back`. Affects all 8 consumers. **Fix:** fixed `std::array<std::pair<int32_t,int>,64>` + linear scan (count already capped at 64) → module patch bump.

### WR-06 — KBM mapping not persisted, not clearable, sticks across scale loads
`PluginProcessor.cpp:922-962` saves no KBM fields; `resetKeyboardMapping()` is only called from the constructor (`TuningEngine.cpp:91`, verified). A KBM session reopens unmapped; loading session B keeps A's mapping; a 12-key map stays applied to a newly loaded 19-EDO. **Fix:** `kbm` child in tuning state (read back via `toString()`), `clearKBMFile` native fn, `resetKeyboardMapping()` on restore when absent.

### WR-07 — KBM semantics diverge from the Scala spec
`TuningEngine.cpp:801, 806, 839-841` (`x` keys and out-of-range keys play 12-TET instead of silence; `isNoteMapped()` `:683` has no callers); `:813` (`kbmOctaveDegree` ignored — always repeats at `activeIntervals.back()`); `:568` (map size 0 = linear per spec, becomes 12-key); `:852` (degrees > scale size clamped instead of wrapping).

### WR-08 — `.scl` parser misreads valid files
`TuningEngine.cpp:470` skips a blank description line (legal) → count line becomes the name, first pitch becomes the count. `:493` drops negative-cent pitches (−1 used as error sentinel) and malformed lines with no check against the declared count. **Fix:** first non-comment line is the description even if empty; `std::optional` from `parseScalaPitch`; reject count mismatches.

### WR-09 — Presets: no reset-to-defaults, and outputGain is saved/applied
`OuariconPresetManager.h:224-245` applies only params present in the file; the 16 factory presets omit all chorus/delay/reverb/EQ params plus `consonantVOT`, `consonantTransition`, `lyricsEnabled` → carried over from the previous preset. All 16 set `outputGain` (e.g. `PluginProcessor.cpp:492, 506, 520, 534`) and `createPresetJson` writes it into user presets. **Fix:** reset all (except outputGain) to defaults before applying; exclude outputGain on save and load. Also: factory presets are rewritten to disk on every construction (`:711`), including host scans — write only when missing/changed.

### WR-10 — UI goes stale when the host restores state with the editor open; clearing lyrics doesn't clear the engine
Preset name (`main.js:1339`), lyrics text/loop (`:1678-1689`), language (`:131`) and the tuning panel are read once. Host preset menu / A-B / undo leaves them stale (and with CR-06, engine gets new text + old syllables). Separately `PluginEditor.cpp:598` skips `setSyllables` when the list is empty (**verified**), so deleting all lyrics keeps singing the old ones. **Fix:** state-generation counter bumped in `setStateInformation`, editor timer emits `stateRestored`, page re-reads everything incl. `tuningPanel.refreshState()`; call `le.clear()` on empty.

### WR-11 — MPE pressure/timbre zeroed at note-on + a constant −0.4 Rd bias
`FormantVoice.cpp:176-177` zeroes `mpeBreathOffset`/`mpeVowelYOffset` instead of reading the note's current pressure/timbre (legacy mode carries them). The Rd target adds `(mpeBreathOffset − 0.5)·0.8` (`:463`) — −0.4 with no pressure — but note-on Rd (`:250`) omits it, so every note sweeps Rd by −0.4×modDepth over 20 ms and non-MPE users sit permanently offset. **Fix:** seed from `getCurrentlyPlayingNote().pressure/.timbre`; make the bias consistent (centre or remove) in both places. *Changes default timbre — listen pass.*

### WR-12 — Re-enabling an effect replays stale buffers
`PluginProcessor.cpp:816-856`: bypassed / mix ≤ 0.001 effects skip processing and are never reset; turning the delay back on replays up to 2 s of old echoes, the reverb its old tail. **Fix:** `reset()` on inactive→active.

### WR-13 — Output gain starts at unity for 50 ms
`PluginProcessor.h:119` constructs the smoother at 1.0; `reset()` in `prepareToPlay` (`:771`) doesn't set it. A −60 dB session plays its first 50 ms at 0 dB into the clipper. **Fix:** `setCurrentAndTargetValue(decibelsToGain(outputGain))` in `prepareToPlay`.

### WR-14 — Onset/release breath boost is block-rate → buffer-size dependent
`FormantVoice.cpp:419-448`: the 50 ms onset boost and 40 ms release burst are evaluated once per block (smoother only 20 ms). 2048-sample blocks hold +4.5 dB the whole block. **Fix:** evaluate on the existing 32-sample update inside the sample loop.

### WR-15 — Shimmer pitches +19 st, not +12
`ReverbProcessor.cpp:126` (**verified**): `delay = G − readPos` with `readPos += 2` per sample while the write head advances 1 → read speed 3×. **Fix:** `readPos += 1.0f` (4-head Hann overlap stays unity). Also PLAUSIBLE: shimmer 1 + size 1 injects loop gain > 1 (`:363-367`) and pins at the ±2 tank clamp — scale injection by `(1 − feedbackGain)`.

### WR-16 — Unsmoothed delay time, reverb size and pre-delay → clicks; stale pre-delay
`DelayProcessor.cpp:68-70, 112` (delay samples jump at block rate); `ReverbProcessor.cpp:332-338, 346, 357-358` (all 8 tank lengths + pre-delay step). Pre-delay line only written while pre-delay > 0 (`:353`) → 0→up replays stale audio. **Fix:** per-sample `SmoothedValue` on delay samples / size / pre-delay; always push into the pre-delay line.

### WR-17 — Cascade/hybrid ignore vowel gains and the Singer's Formant boost; near-Nyquist gain estimate undershoots
`CascadeFormantBank.h:97-125` hard-codes `gain = 1` → hybrid's parallel F4/F5 sit at 0 dB for every vowel (/o/ F5 should be −40 dB) → back vowels bright; the +4 dB Singer's Formant (`FormantVoice.cpp:652,657`) only reaches the parallel bank. `:130-137, 159` (PLAUSIBLE): `2(1−r)sinθ` floor underestimates the peak near Nyquist (R/L vowel, Shift +24, Spread 2 → F5 at 21.95 kHz ≈ +55 dB whistle, normalization drops everything else ~20 dB). **Fix:** pass `gain[]` through; exact `|H(e^{jθ})|`; clamp formants to ~0.45·sr.

### WR-18 — Consonant→vowel transitions step in one sample; ping-pong doesn't bounce centred sources
`ConsonantEngine.h:236, 310-317`: continuous suppression drops from manner×(1−voicing) to 0 when the envelope hits Off (/s/ → vowel: glottal source 0.3→1.0 at arbitrary phase); aspiration end steps likewise. **Fix:** scale by the envelope value; 2–5 ms ramp on aspiration end. `DelayProcessor.cpp:106-110`: with L == R, cross-feedback gives identical channels. **Fix:** in PingPong feed `(L+R)/2` into L only.

### WR-19 — Knob needles pivot off-centre; readouts change format after load (visual)
Border-box sizing puts the rotation origin inside the 2 px border: main knob pivot 27.5 vs centre 25.5 → `transform-origin: 50% 21.5px`; FX knobs → `50% 17px`; `.knob-sm` → `50% 14px`; tuning `.ref-knob-indicator` pivots 3 px high. Tip-to-rim gap swings 0.5 px (extremes) to 4 px (12 o'clock). `formatValue` (`main.js:933`) shows times in seconds to 2 dp (1–4 ms → "0.00 s"; placeholders "10 ms"/"375 ms" → "0.01 s"/"0.38 s"); FX "50 %" → "0.50". **Fix:** per-param formatters (ms below 1 s, % for 0–1).

### WR-20 — Exporters write wrong or unsafe output
`TuningExporter.cpp:411, 417`: `.scl` description into HTML `<title>`/`<h1>` unescaped (crafted .scl → script in exported page). `:394` forces period 1200 when last interval ≤ 1200 (wrong ET deviations for e.g. Carlos Gamma 737.1c); `:329` counts the period as a note ("13 notes" for 12). `TuningEngine.cpp:640-667` exported `.kbm` with no KBM loaded always writes the 12-entry map at note 69 / `a4Frequency`, while the engine anchors on 60+tonic → exported pair plays differently elsewhere. **Fix:** HTML-escape; use the real period; export map size 0, ref = 60+tonic at its 12-TET freq, octave degree = scale size.

---

## Info

- **IN-01** Glide lags pitch-wheel moves (bend goes through the portamento smoother, `FormantVoice.cpp:570`); a note-on with the wheel already bent slides from unbent. Apply `bendRatio` after `pitchGlide.getNextFrequency()`. **Resolved in v1.32.0**
- **IN-02** Glide starts from whatever the voice last played — `wasActive` never cleared on natural note end (`:813-816`), so chord notes slide from unrelated pitches. **Resolved in v1.32.0**
- **IN-03** NaN guard doesn't clear `spectralTiltPrev` (`:748, 756, 787-802`) → a NaN there silences the voice until next note-on. Also snap `rdSmoothed`, `sourceFilterGain`, glide. **Resolved in v1.31.1**
- **IN-04** `getTailLengthSeconds()` = 5 s vs release up to 10 s + delay feedback 0.95 × 2 s. **Resolved in v1.32.0**
- **IN-05** Output clipper `jlimit` (`PluginProcessor.cpp:887`) passes NaN; reverb ±2 tank clamp and delay feedback also latch NaN (`ReverbProcessor.cpp:400-401`, `DelayProcessor.cpp:103`). Add a finite guard at the output.
- **IN-06** `VibratoLFO` `juce::Random` clock-seeded (`VibratoLFO.h:107`) → renders not reproducible. **Resolved in v1.32.0**
- **IN-07** `LFGlottalSource.h:144-146`: phase in `[1−2⁻²⁵, 1)` rounds to `1.0f` → reads index 2049 of the last frame (one past end; `frac = 0`, ASan-visible UB). Compute in double or `min(idx0, kTableSize-1)`. **Resolved in v1.31.1**
- **IN-08** Glottal table carries DC equal to the return-phase area (mipmap stage removes it); resolves with CR-01. **Already fixed in v1.31.0** (CR-01 solver: net area over the period is 0; mipmap still zeroes DC).
- **IN-09** `FricationFormantBank.h:125-127` F6F fixed 6 kHz with no Nyquist clamp (garbage coeffs below ~12 kHz SR). **Resolved in v1.31.1**
- **IN-10** Aspiration closure burst at fixed phase 0.6 (`AspirationNoise.h:78`) while Te spans 0.30–0.99 — pass Te in.
- **IN-11** Topology switch leaves resonator state paired with band-pass coefficients / stale skipped bank → one-block click. Reset on change. **Resolved in v1.32.0**
- **IN-12** Reverb damping (`damping*0.7` per-sample) and mod excursion (16 samples) are SR-dependent (`ReverbProcessor.cpp:342, 384`); IN-12 of the prior review aliases stage-0 diffusion below 44.1 kHz. **Resolved in v1.32.0**
- **IN-13** Pre-delay 0 < d < 1 sample blends newest with a ~370 ms-old sample (`ReverbProcessor.cpp:62-67`). **Already fixed in v1.30.0** (WR-16).
- **IN-14** Burst envelope truncated at `exp(-2)` = 13.5 % for high manner (`ConsonantEngine.h:276`); normalise. Aspiration-active test uses the bipolar noise sign (`:236`). *(Aspiration sign test: **already fixed in v1.30.0**, WR-18. Burst truncation: **Resolved in v1.32.0**.)* **Resolved in v1.32.0**
- **IN-15** EQ coefficient steps at block rate (mild zipper on fast sweeps). **Resolved in v1.32.0**
- **IN-16** Delay knob silently tops out at 1 s at 192 kHz (buffer 192000 samples) — size the buffer from SR in `prepare`. **Resolved in v1.31.1**
- **IN-17** Partch 43-Tone has 41 entries (`EmbeddedTunings.cpp:130-133`) — missing 11/10 and 20/11.
- **IN-18** Tonic semantics differ: linear mode shifts the anchor by 12-TET semitones (`TuningEngine.cpp:885`), KBM mode rotates by scale degrees (`:811, :352`).
- **IN-19** Session restore always lands in Scala mode (`setCustomIntervals` after `setBuiltInPreset`, `PluginProcessor.cpp:1004-1017`); pitches identical, UI label differs. `"preset"` int not range-checked. **Resolved in v1.31.1**
- **IN-20** `scaleName` (`juce::String`) read without the lock in `getActiveTuningName` (`:338`) / `generateScalaFileContent` (`:615-617`) — racy if a host calls `setStateInformation` off the message thread (PLAUSIBLE). **Resolved in v1.31.1**
- **IN-21** Preset category dropdown desyncs from prev/next (which walk all categories, `OuariconPresetManager.h:82`); "All" and re-selecting a category do nothing. Save shows the unsanitised name ("a/b" vs file "ab"); save failures silent. *(Save name + silent failure: **Resolved in v1.31.1**. Category / prev-next navigation: open.)*
- **IN-22** Tuning panel: interval-row labels don't follow a tonic change (call `updateIntervalList()` after `setTonic`); A4 knob uses mouse events without pointer capture (drag sticks when released outside); canvas DPR captured once (blurry after moving between Retina/non-Retina). *(Tonic → interval rows: **Resolved in v1.31.1**. Canvas DPR: **Resolved in v1.31.1** — the real site was main.js (XY pads, ADSR), not the tuning panel. A4 drag: stale premise — listeners are on `document`.)*
- **IN-23** Consonant pad: p/t/k glyphs (y≈70/80) collide with Lab/Alv/Vel captions; "1.3kHz Mixed" readout runs into the 'f' glyph — worse in fr/zh. `.botanical-overlay` sits in the scrolling column, adding 30 px empty scroll. Lyrics poll runs every 80 ms (comment says 50) and redraws both pads unconditionally. *(Glyph/caption and readout/f collisions: **Resolved in v1.31.1** (measured, 0 px² in en/fr/zh-Hans). Poll comment + unconditional redraw: **Resolved in v1.31.1**. `.botanical-overlay` scroll: not reproduced — measured 0 px added to 350 px of real overflow.)*
- **IN-24** Accessibility / i18n / comments: tabs are `<div>`s with no role/tabindex; knobs unfocusable; no wheel, double-click-reset or fine-drag; gear lacks `aria-expanded`. Generated scale names ("Harmonics a–b", "Rank-2 (… notes)", `tuning-panel.js:871-889`), the "cents" unit and `.interval-unit` "c" leak English. Stale comments at `PluginEditor.cpp:172-174` and `PluginProcessor.cpp:980` (zh-Hans is index 2); VowelMorpher/VowelData say "5 cardinal vowels" (there are 7). Tuning-library JSON built by string concatenation (`PluginEditor.cpp:529-531`). *(Stale comments (PluginEditor/PluginProcessor languageIndex, VowelData/VowelMorpher "5 vowels"), `c`/`ct` exemption rows, JSON by concatenation: **Resolved in v1.31.1**. "cents" leak: stale — the UI shows `ct`. Tabs/knobs/gear a11y, wheel/dblclick/fine drag, generated scale names: open.)*

---

## What's solid

- Audio thread: no allocation, locks or logging in `processBlock`/voice (except WR-05's module map); tuning table read via atomics; lyrics try-lock; cached param pointers (prior IN-02) correct; `ScopedNoDenormals` covers voices + FX.
- Prior CR-01 bend fix exact (MPESynthesiser sub-block splits at events); stacks multiplicatively with tuning + NE.
- State: A4/stretch/language restore uses `isVoid()` + `toString()` correctly; no `<CustomState>` duplication.
- Filters: `r = exp(−π·BW/sr)` with `r ≤ 0.9999`; Klatt unity-DC resonator; log-domain vowel interpolation keeps F2 < F3; vowel data matches the Csound bass table; mipmap selection SR-aware with guard sample.
- FX: delay feedback ≤ 0.95 through a unity-gain SVF; reverb Householder matrix orthogonal, RT60 gain < 1, buffers sized for max size + mod + interp; EQ `ArrayCoefficients` assignment genuinely allocation-free; consonant timings all ms × SR.
- UI: all 7 resources served with correct MIME; the other 54 relays match JS + APVTS IDs; no lambda ranges; every native fn `complete`s on every path; en/fr/zh-Hans key sets complete (scripted check); tooltip renderer uses `textContent` and clamps all four edges; no module-level TDZ.

## Suggested fix order

1. **v1.29.1 (PATCH, no timbre change):** CR-02, CR-04, CR-05, CR-07, CR-08, CR-09, WR-02, WR-05 (module), WR-10, WR-12, WR-13, IN-05.
2. **v1.30.0 (MINOR, behaviour changes):** CR-03, CR-06, WR-03, WR-04, WR-06, WR-09, WR-14, WR-16, WR-18.
3. **v1.31.0 (MINOR, timbre re-render — listen pass on all 16 presets):** CR-01 + WR-01 together, WR-11, WR-15, WR-17.

---

## Resolved

| Version | Commit | Findings |
|---|---|---|
| v1.29.1 | `b7528dc3` | CR-02/04/05/07/08/09, WR-02/05/10/12/13, IN-05 |
| v1.30.0 | `265eb24f` | CR-03/06, WR-03/04/06/07/08/09/14/16/18/19/20 (IN-13, IN-14b as side effects) |
| v1.31.0 | `7c0baac5` | CR-01, WR-01, WR-11, WR-15, WR-17 (IN-08 as a side effect) |
| v1.31.1 | `6bd37253` | IN-03, IN-07, IN-09, IN-16, IN-19, IN-20, IN-21 (save), IN-22a/c, IN-23a/b/d/e, IN-24g/h/i/j/k |
| v1.32.0 | `PENDING` | IN-01, IN-02, IN-04, IN-06, IN-11, IN-12, IN-14a, IN-15 |

Closed without change: IN-22b (drag listeners already on `document`), IN-23c (overlay adds 0 px — measured), IN-24f (UI shows `ct`).

Still open: IN-10, IN-17, IN-18, IN-21 (navigation), IN-24a–e.

Chosen resolutions for the next sweep (decided 2026-09-25): IN-17 — fix O-Formant's table only (add 11/10 = 165.0 ¢ and 20/11 = 1035.0 ¢; the scala-tuning-engine module and the O-Bells/Prism/Strata/Lyrica/IntonationPad copies as a separate follow-up); IN-18 — document and leave; IN-24 — semantics + i18n only (tab role/tabindex, gear `aria-expanded`, localized generated scale names); knob focus, wheel, double-click reset and fine drag stay open.
