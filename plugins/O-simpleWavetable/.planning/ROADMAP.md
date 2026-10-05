# O-simpleWavetable - Implementation Plan

**Date:** 2026-10-05
**Complexity Score:** 5.0 (Complex)
**Strategy:** Phase-based implementation

complexity_score: 5.0
staged_implementation: true

---

## Complexity Factors

- **Parameters:** 22 parameters (22/5 = 4.4, capped at 2.0) = **2.0**
- **Algorithms:** 15 components in ARCHITECTURE.md §Core Components = **15**
  - WavetableBank, BankFactory (5 formula banks), MipmapBuilder (FFT), Wavetable oscillator (linear read + frame lerp/latch), frozen-cycle crossfader, bit-depth quantizer, amp ADSR, mod ADSR, global LFO (5 shapes + tempo sync), effective-position smoothing, voice manager (Poly/Mono + bend), bank store/swap/reaper, importer, visualization renderer (FFT), output stage
- **Features:** 3 points
  - FFT / frequency domain (+1): mipmap build, import DC removal, harmonics panel
  - Modulation systems (+1): global LFO + per-voice mod env → Position
  - External MIDI control (+1): MIDI notes, velocity, pitch bend, poly/mono
  - (No feedback loops, no multiband.)
- **Total:** 2.0 + 15 + 3 = 20.0 → **capped at 5.0**

Tier 5 (file I/O + embedded binary state + live visualization), DEEP research. Risk is concentrated in Phase 2.4 (import, swap, persistence).

---

## Stages

- Stage 0: Research & Planning ✓
- Stage 1: Foundation ← Next (CMake, APVTS 22 params, shell editor, pluginval)
- Stage 2: DSP (4 phases)
- Stage 3: GUI (3 phases)
- Stage 4: Polish (presets, perf, Windows, validation)

---

## Stage 1: Foundation

**Goal:** buildable synth shell, matching O-simpleAdditive's structure.

- CMake: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, WebView2 static-link define, `JUCE_USE_CURL=0`. Link juce_audio_utils, juce_audio_formats, juce_dsp, juce_gui_extra. Add `webview-drop-streaming` via `ouaricon_add_module` (as O-simpleGrain does).
- APVTS with the 22 parameters exactly as in ARCHITECTURE.md §Parameter Mapping (or `parameter-spec.md` once the mockup is finalized). `jassert(params.size() == 22)`.
- `juce::Synthesiser` + 16 silent `WtVoice` + `WtSound`; MidiMessageCollector for the on-screen keyboard; `setLatencySamples(0)`.
- getState/setState with an empty `IMPORTED_BANK` round-trip stub.

**Test Criteria:**
- [ ] Builds VST3 + AU (+ Standalone); `auval -v` (targeted, not `auval -a`, per memory) and pluginval pass (COMPAT-01)
- [ ] All 22 params visible to the host with the correct ranges and text ("-inf", "Full")

---

## Stage 2: DSP Phases

### Phase 2.1: Bank Engine (offline-verifiable)

**Goal:** banks and mipmaps exist and are correct before any voice plays them.

**Components:** `WavetableBank`, `BankFactory` (Sine→Saw, Sine→Square, Pulse Width, Formant, Drive per §A3–A7), `MipmapBuilder` (11 levels, Kmax = min(1023, 1024>>L), DC + Nyquist zeroed), `SharedResourcePointer<BuiltInBanks>`.

**Test Criteria (unit/harness, FFT checks):**
- [ ] Each built-in bank: 32 × 2048, frame peaks = 1.0 ± 0.5 dB, DC ≤ −80 dB (DSP-04)
- [ ] Sine→Saw frame k contains exactly harmonics 1..k; frame 1 pure sine (FUNC-02)
- [ ] Sine→Square even harmonics = 0 (≤ −60 dB); frame 32 top = h31; all 32 frames distinct
- [ ] Drive h3/h5/h7 strictly increasing across frames; frame 1 h3 ≈ −46 dB
- [ ] Level L contains no energy above Kmax_L (≤ −120 dB)
- [ ] Build time logged (expected tens of ms; one-off per process)

### Phase 2.2: Voice + Oscillator Core

**Goal:** play the banks: pitch, read path, band-limiting, stepping, bit depth, amp env, voices, output.

**Components:** `WtVoice` (double phase, linear in-cycle read, guard sample, frame lerp vs cycle-wrap latch), strict mip-level selection, bit quantizer (mid-rise, Full = bypass), amp ADSR (dirty-checked `setParameters`), velocity, Poly 16 / Mono (Subtractive `renderMonoLegato` pattern, Mono only), pitch bend ±2 st, output stage (−60 dB = 0 gain), isfinite scrub, lead-voice display atomics.

**Test Criteria:**
- [ ] Fundamental within ±1 cent from A0 to C8 (FUNC-01); Position changes timbre, not pitch
- [ ] **QUAL-02 gate:** Drive frame 32 and a 1023-harmonic frame, band-limit On, C8 at 44.1/48/96 kHz → max inharmonic ≤ −100 dB (Stage 0 model: −121 dB); sweep A0..C8 ≤ −70 dB
- [ ] DSP-02: band-limit Off, Sine→Saw frame 32 at C7 shows aliases
- [ ] DSP-01: Interp Off slow sweep → 32 discrete spectra; On → continuous
- [ ] DSP-03: 3 bits → exactly 8 distinct sample values; Full bit-identical to bypass
- [ ] Poly 16 simultaneous notes; Mono last-note priority (FUNC-07)
- [ ] Alloc gate on processBlock (PERF-01)

### Phase 2.3: Modulation → Position

**Goal:** the movement section.

**Components:** knob `SmoothedValue` (20 ms) into a block buffer; global LFO rendered per sample into a preallocated buffer (Sine/Tri/Saw/Square/S&H, free 0.01–20 Hz, tempo sync from PPQ with the 16-entry division table, BPM fallback 120); deterministic S&H RNG; per-voice mod ADSR × `env_amount`; clamp; per-voice 2 ms one-pole (Interp On only), seeded at note-on.

**Test Criteria:**
- [ ] DSP-05: effective Position clamped to [0,1]; no zipper on knob automation with Interp On (click detector)
- [ ] FUNC-05: all 5 shapes; tempo sync phase-locked to a synthetic AudioPlayHead (harness); one global phase across voices
- [ ] FUNC-06: positive amount sweeps up, negative sweeps down; long mod release doesn't keep a voice alive
- [ ] Square/S&H at depth 100% with Interp On → no clicks above the detector threshold

### Phase 2.4: Bank Switching, Import, Persistence (HIGH risk)

**Goal:** RT-safe bank changes and the import → state round trip.

**Components:** per-block bank-pointer resolve; frozen-cycle crossfader (ping-pong 2049-float buffers, 5 ms; triggers: bank ptr, level, interp, bandlimit); `std::atomic<const WavetableBank*>` imported publish + Prism REG-01 reaper (`blockEntries` / `blockGeneration`, retire + timer sweep); importer worker (§A8: ≤ 256·2048 samples, channel mean, slice, FFT DC removal, +24 dB-capped per-frame normalize, int16 canonicalize, FLAC16 + base64 cache, mip build, `callAsync` publish, auto-select Imported); `IMPORTED_BANK` state child save/restore (fallback `pcm16gz` reader); empty-Imported = silent.

**Test Criteria:**
- [ ] QUAL-03: click detector on bank-change automation, import-during-held-notes, bandlimit/interp toggles, pitch bend across a level boundary
- [ ] DSP-06: alloc gate passes during bank switch and import publish
- [ ] FUNC-03: 10 s / 44.1 kHz file → 215 frames; > 256·2048 samples → 256; < 2048 samples → rejected with message; WAV/AIFF/FLAC at 44.1/48/96 kHz (COMPAT-03)
- [ ] FUNC-04: save → reload restores the bank **bit-identically** and the filename; the int16 reload gate compares every float
- [ ] Reaper: retired-count returns to 0 with the host idle (quiescent rule) and while rendering (+2 rule)
- [ ] Rapid repeated imports: no leak (held-bank count bounded)

---

## Stage 3: GUI Phases

### Phase 3.1: Layout and Bindings
- Finalized mockup HTML; WebView setup (sibling member order, bare-path resource provider, ES6 module, `check_native_interop.js`).
- Relays/attachments for all 22 params (sliders, toggles via `getToggleState`, combos via `getComboBoxState`); Rate ↔ Division swap on `lfo_sync`; Bit Depth 15-step control with "Full".
- [ ] Every control drives its param, and host automation moves the UI

### Phase 3.2: Visualization Panels
- C++ editor Timer 30 Hz: atomics → exact cycle render (shared read code) → quantize → 2048 FFT → `cycleUpdate {cycle[256], harmonics[32], pos, frame, level, sounding}` (hash-gated); `bankUpdate` (N × 128 thumbnails) on bank change / import / `uiReady`.
- JS: 3D/stacked bank panel with highlight + marker (between frames for Interp On, jumping for Off), cycle panel (point-sampled, staircases visible), harmonics 1–32 bar graph.
- [ ] UI-01..03 acceptance: marker follows LFO/env motion; 3-bit staircase visible; bars match an offline FFT of the rendered cycle (harness compares the JSON payload)
- [ ] ≥ 30 fps, no audio-thread stall (PERF-03)

### Phase 3.3: Import UX, Tooltips, i18n
- Import button (FileChooser via a native function) + WebView drop (`webview-drop-streaming`); filename + frame count; empty-Imported state; `importStatus` errors.
- Plain-language tooltips for every control (the aliasing, formant-shift and loop-buzz lessons named); localization per suite convention (glossary first, then lint gates).
- [ ] UI-04, UI-05, UI-06; Standalone hands-on with a real file drop

---

## Stage 4: Polish
- Factory presets per ARCHITECTURE.md §A9 (built-in banks only, never set `output_level`) (FUNC-08)
- CPU check: 16 voices at 96 kHz (PERF-02); Windows VST3 build with WebView2 static (COMPAT-02)
- QUAL-04 listening pass (stepped vs smooth, alias vs clean, bit depth); pluginval VST3 + AU; CHANGELOG

---

## Implementation Notes

### Thread Safety
- Audio thread: no alloc, no locks, no FFT, no file I/O. Banks are immutable after publish. Bank pointers are resolved once per block; voices never keep a retired pointer (frozen-cycle crossfade).
- Reaper counters are incremented on **every** processBlock return path (zero-channel included).
- `getStateInformation` can run on any thread: it reads the cached base64 under a CriticalSection that the audio thread never takes.

### Performance
- ~30 flops per voice-sample; crossfade capture of 2048 lerps per voice per event. Built-ins (~14 MB) are shared process-wide; the imported bank is up to 23 MB per instance.

### Latency
- Zero. `setLatencySamples(0)` in `prepareToPlay` (`getLatencySamples` is non-virtual in JUCE 8).

### Denormal Protection
- `ScopedNoDenormals` + block `isfinite` scrub (sibling).

### Known Challenges
- Do **not** port O-Prism's trilinear level crossfade: it fails QUAL-02 (measured −15.6 dB aliasing at C8). Use strict `ceil(log2(inc))` selection.
- O-Prism's mip builder keeps the Nyquist bin at level 0; zero it.
- Dirty-check ADSR `setParameters` (project memory: per-block calls kill the release).
- `emitEventIfBrowserIsVisible` drops events while hidden, so re-send the bank on `uiReady`.
- ValueTree XML round-trip returns properties as strings; parse `numFrames` explicitly.
- ASan hangs on macOS 26 (memory), so lifetime gates are harness counters + alloc gate, not ASan.

## References
- `research/ARCHITECTURE.md` (this plugin) for decisions, formulas and the verification table
- O-simpleAdditive (voice/viz/output conventions), O-simpleSubtractive (Mono path, bend), O-Prism (bank layout, mips, importer, reaper, NoteDiv), O-simpleGrain (import UX, drop streaming, state child)
