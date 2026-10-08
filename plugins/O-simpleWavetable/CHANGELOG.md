# Changelog - O-simpleWavetable

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.0.1] - 2026-10-07

Maintenance release from a full code review (DSP core, processor / state, editor /
page). No parameter, range, preset or state-format change: v1.0.0 sessions and
presets load unchanged.

### Fixed
- **Mono retrigger from a release tail could re-attack from silence.** A deferred
  ADSR change was applied before `noteOn()`; `juce::ADSR::setParameters` in the
  release state with sustain 0 resets the envelope to idle, so a note played into a
  plucky tail after an envelope knob had moved stepped to 0 in one sample. The same
  path reset the mod envelope (default sustain 0) and restarted the position
  contour. Root cause: call order in `WtVoice::noteOnDirect` / `startNote`; the
  deferred parameters now apply after `noteOn()`, where the attack state never resets.
- **A repeated note-off lengthened the release.** `juce::ADSR::noteOff` recomputes
  the release rate from the current level, so overlapping same-pitch notes (a second
  note-off while already releasing) restarted the ramp, up to 2x the set time. The
  voice now ignores a note-off while releasing, and Mono releases only on the
  held-stack's non-empty-to-empty transition.
- **Mono cut CC123 All Notes Off in 2 ms where Poly rang out.** Both modes now treat
  CC120 / CC123 as a release (JUCE's Poly mapping), so a transport stop sounds the
  same in both.
- **Import error status race.** A superseded worker's error could overwrite the newer
  job's "busy" (the stale check ran outside the lock). Worker-side status writes are
  now generation-checked under the lock.
- **Editor closed mid-drag left a host gesture open** on the 13 slider parameters
  (`~WebSliderParameterAttachment` does not end it). The editor now tracks every
  gesture and ends any still open in its destructor; the stepped knobs already did.
- **Saving a preset under a case-variant of an existing name** overwrote the file but
  recorded the typed spelling, leaving the panel on a greyed name with Delete
  disabled. A confirmed replace saves under the existing stem.
- **Drop-target ring blinked off** while a file was dragged across the page
  (`dragover` never re-asserted hover after an inner `dragleave`).
- **Forward compatibility of session restore.** A parameter absent from an incoming
  state now restores to its default instead of the instance's live value (APVTS
  fills a missing child from the current value). Matters for the first parameter
  added after 1.0.
- Page: a clicked knob now takes keyboard focus (arrow keys work after a click); a
  note held by both a computer key and the mouse no longer cuts off when one is
  released; an arrow key during a stepped-knob drag no longer nests a gesture.

### Changed (internal, no behaviour change)
- Removed the never-adopted two-pole position-smoother scaffolding, the unused
  velocity side of the mono note stack, four unused voice test getters, three unused
  LFO getters, six unused display getters, an unused crossfade argument, a redundant
  block-context seed and the 3-argument voice `prepareToPlay` (moved to the test rig).
- The 21 named raw-parameter pointers are one array indexed by `ParamIDs::Slot`;
  `isPresetOutputSlot` is a constexpr index compare; the restore path's three
  "empty Imported" branches share one helper; `WtSynthesiser::findVoiceToSteal`
  iterates `voices` directly (no per-voice lock).
- Page: the knob ring is a `repeating-conic-gradient`; one `capturePointer` helper
  replaces three copies of the pointer-capture lifecycle; one `bindSegments` serves
  toggles and choice rows; one `paintKnob` serves both knob kinds; a dead flat-array
  thumbnail fallback and ten no-op `try / catch` wrappers are gone.

### Testing
- Debug gate tree, 0 warnings: bank-check 14 / 14, dsp-check ALL PASS
  (`--alloc-check` 0 allocations), mod-check 19 / 19, import-check 34 / 34
  (G-REAP-SOAK 44 reaped), state-check 12 / 12, viz-check 88 / 88.
- UI: check-i18n ALL PASS, check-ui-labels ALL PASS, boot-all-uis 0 DEAD / 0 late,
  G-S3W3-WHEEL 6 / 6 (the shared pointer helper keeps N8 / N13 exact).
- Installed VST3 (pedalboard): see the review addendum in CODE_REVIEW.md.

## [1.0.0] - 2026-10-06

First release. A pedagogical 16-voice wavetable synth with a "Wavetable Field Guide"
WebView: every control has a plain-language tooltip, and the panels show the exact
cycle you hear, its harmonics, and where you are in the bank.

### Added
- **Synth engine.** 16-voice Poly, or true-legato Mono (last-note priority, no glide).
  2048-sample frames read with strict per-octave mipmaps (11 levels), so Band-limit On
  keeps aliasing far below audibility. Band-limit Off is the intended aliasing switch.
  Interpolation On / Off (smooth morph vs stepped frames), Bit Depth Full, 16 ... 3
  (mid-rise quantizer), squared velocity, +/-2 semitone pitch bend, Output Level.
- **Position modulation.** Position knob plus a global LFO (Sine, Triangle, Saw, Square,
  S&H; free rate or tempo-synced), a per-voice mod envelope with a bipolar Env Amount,
  and an amp ADSR. A 5 ms frozen-cycle crossfade covers bank, interpolation,
  band-limit and mip-level changes.
- **Five built-in banks** of 32 frames each: Sine -> Saw, Sine -> Square, Pulse Width,
  Formant, Drive.
- **Import.** Slice your own audio into an Imported bank (up to 256 frames of 2048
  samples) from the Import button or by drag-and-drop (drops up to 16 MiB). The bank
  is saved inside the session (FLAC16) and restores bit-identically. An empty Imported
  bank plays silence and shows a prompt.
- **Field Guide UI** (1120 x 780, fixed): the bank stack with the current frame and
  Position marker, the quantized current cycle, harmonics 1-32 with the band-limit
  ceiling, an on-screen keyboard with octave buttons, tooltips on every control, and
  English, French and Simplified Chinese.
- **Lessons.** Five lesson buttons, each isolating one idea, with a caption that says
  what to listen for.
- **Preset manager.** A preset panel (list, previous / next, Save, Delete) with 9 factory
  presets: Init - Additive Build, Stepped Scan, Smooth Scan, Alias Demo, Drive Sweep,
  Vowel Pad, Pulse Narrowing, 8-bit PPG, 4-bit PPG. The lessons are a subset of the
  same table. User presets save, replace and delete in the page; factory names are
  protected. The panel shows the current name, a dot when it has been edited, and
  lights the matching lesson. Presets never change Output Level. Presets live in
  `~/Library/O-simpleWavetable/Presets`.
- **Measured quality and performance.**
  - Stepped vs smooth scan: cycle-difference energy ratio 54x (G-Q4-STEP, >= 10).
  - Aliasing vs clean (Alias Demo, C7 / C8): Band-limit Off aliases reach -41 dB or louder
    below 8 kHz; Band-limit On sits at -107 dB or lower; contrast at least 69 dB
    (G-Q4-ALIAS).
  - Bit depth: 6.03 dB SNR per bit, 95.1 dB at 16 bits down to 16.5 dB at 3 bits
    (G-Q4-BITS).
  - CPU, 16 voices on the worst-case patch (Release, Apple Silicon): 0.67 % of one core's
    real-time budget at 96 kHz / 512 (0.31 % at 44.1 kHz); 1.9 % with a crossfade
    every block (G-PERF02-*).

### Fixed during Stage 4
- Mono: the first note after a Poly -> Mono switch could play bent by a pitch-wheel move
  made earlier in Poly (G-MONO-WHEEL; was +199.98 cents, now 0.00).
- Mono: a large upward legato jump could alias for about 5 ms; the outgoing cycle now
  keeps its own pitch through the crossfade (G-LEGATO-XF, G-LEGATO-ALIAS,
  G-LEGATO-CLICK; alias -24.3 dB -> the -102.0 dB floor).
- A block carrying thousands of MIDI events could grow a scratch buffer on the audio
  thread (G-MIDI-FLOOD, `--alloc-check` 0 allocations).
- Audio processing before the host prepared the plugin is now safely silent
  (G-UNPREPARED).
- Headers that test `OSIW_TEST_HOOKS` now include its default first and build clean under
  `-Wundef` (R-UNDEF).
- An import that finished while the editor was opening could leave Import and drop
  disabled (W1, `uiReady` reads its counters first; inspection + hands-on).
- Dropping a very large file froze the editor for seconds; the drop cap is now 16 MiB
  with "too large to drop - use the Import button" (G-DROP-CAPSYNC, G-DROP).
- A trackpad flick on Bit Depth, LFO Division, the bank stack or a continuous knob
  jumped many steps; the wheel now steps per distance scrolled (G-S3W3-WHEEL).
- An old import error reappeared on a built-in bank after the editor reopened
  (G-S3W4-ERRCLEAR).
- A note held on the on-screen keyboard stuck when the editor closed mid-hold
  (G-S3W5-UIHELD).
- A fast double-click on Import opened two file dialogs (N1; hands-on).
- The cycle display kept showing a sounding note after the host released resources
  (G-VIZ-RELEASE).
- A pending auto-select from a finishing import could override a lesson's bank
  (G-S3N5).
- Imported file names now strip C1 control, bidi and invisible characters
  (G-SANITISE).
- Wheel and drag on the Position knob and the bank stack no longer open two host
  gestures at once (G-S3W3-WHEEL, N8 arm).
- A session from a newer format showed "import failed" instead of a specific message
  (N9; check-i18n, UI state 6).
- The lesson highlight went stale after edits, loads and reopen; it now follows the
  preset state (G-PRESET-STATE).
- The octave buttons had no tooltip (N11; boot-all-uis `--strict-tips`).
- Silence now reads with a typographic minus, "-inf" -> "−inf", in the page only
  (N12; check-i18n).
- Dragging a stepped knob wrote one host automation gesture per detent; a drag is now
  one gesture (G-S3N13).

### Known limitations
- Switching Voice Mode (Poly <-> Mono) ends sounding notes through a 2 ms fade; held
  notes do not carry into the new mode.
- With Interpolation Off, modulated Position steps once per cycle; on Formant and
  Imported frames that step can tick. The stepped sound is the lesson; Interpolation On
  removes it.
- A lesson or factory preset clicked during Mono playback switches to Poly (its default),
  so the note ends through the same 2 ms fade.
- Mono ignores the sustain pedal. A pedal pressed in Mono is not seen by Poly after a
  switch until it is pressed again.
- Imports cannot be cancelled; a newer import or a session restore replaces one in
  flight (a full 256-frame import takes about 20 ms).
- A user preset stores the bank choice, not the imported audio. Loading a preset never
  replaces the session's Imported bank.
- At 88.2 / 96 kHz the Alias Demo is clearly audible only in the top octave.

### Development history

#### Stage 1 - Foundation (2026-10-05)
- Silent 16-voice synth shell, 21-parameter APVTS, `uiLanguage` and Imported-bank state
  stub. auval, pluginval VST3 / AU strictness 10, state-check 9/9.

#### Stage 2 - DSP (2026-10-05 to 2026-10-06)
- Part 1: the five banks, strict mipmaps with a level floor of 1 at Band-limit On,
  the voice and oscillator, Poly / Mono. G-Q2-C8 -114.3 dB, G-Q2-SWEEP -74.9 dB,
  G-PITCH 0.0009 c, `--alloc-check` 0. An allocation-free voice-steal port replaces
  JUCE's `Synthesiser::findVoiceToSteal`.
- Part 2: LFO, mod envelope, raised-cosine frozen-cycle crossfader, reaper, worker
  importer and FLAC16 session persistence. bank 13/13, dsp 20/20, mod 18/18,
  import 34/34, state 11/11.
- Gap closure (`850df9b9`): click-free voice steal and Poly <-> Mono switch (2 ms tail),
  velocity ramp on a Mono retrigger. G-STEAL 1.076x, G-SWITCH-TAIL 0.656x,
  G-RETRIG-VEL 0.589x; on the installed VST3 the steal went from 8.7x to 0.86x.

#### Stage 3 - GUI (2026-10-06)
- Part 1: the Field Guide WebView, 21 parameter bindings, bank / cycle / harmonics
  panels pushed at 30 Hz from the message thread, five lesson buttons. viz-check
  G-VIZ-EXACT 90/90; 0 audio-thread allocations.
- Part 2: import UX (button and drop), tooltips, English / French / Simplified Chinese.
  All five UI gates green; Standalone and Logic hands-on passed.
- Binary null test against the Stage 2 build: 6/6 cases bit-exact.

#### Stage 4 - Polish (2026-10-06)
- Part 1: every critic fix above and the preset manager. All offline test drivers PASS,
  state 12/12, `--alloc-check` 0 including a 6,000-event MIDI flood. Null test against
  the Stage 3 binary: untouched cases bit-exact; differences only where a fix intends
  them.
- Part 2: QUAL-04 contrast gates, the PERF-02 harness, release documents.
