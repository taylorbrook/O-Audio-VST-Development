# O-GranulatorInstrument - Implementation Plan

**Date:** 2026-11-07
**Complexity Score:** 5.0 (capped; raw 27.0) — Complex
**Strategy:** Phase-based implementation

complexity_score: 5.0
complexity_tier: 6
research_depth: DEEP
staged_implementation: true

---

## PREREQUISITE (blocking Stage 1)

1. **UI mockup** (`/start O-GranulatorInstrument` → option 3). Inherit the O-Granulator v2 language and add the items listed in ARCHITECTURE.md "Design Sync". These include the waveform lanes (drop target, relink/trimmed badge, loop handles, record head, per-voice playheads, grain dots), the voice/amp section with a voice-activity strip, Record-in with availability / "no signal" states, the MOD page with instrument sources and "mono" route badges, and the `grain_limit` "heavy" hint.
2. **LOCKED `parameter-spec.md`**, written from the finalized mockup. Apply Design Sync S1–S11 (notably `grain_limit` = 32/64/128/256, the `pitch_bend_range` MPE semantics, `feedback` = convex write amount). Freeze the 126-entry destination order and the 18-entry source order.
3. **O-Granulator 3.3 closed** before Phase 2.1 starts. The fork pins to that commit (ARCHITECTURE §A1). Stage 1 does not depend on it.

Stage 1 builds from `parameter-spec.md` (NOT the draft).

---

## Complexity Factors

- **Parameters:** 261 (draft) → 261/5 = 52.2, capped = **2.0**
- **Algorithms:** 22 core components = **22**
  - The 22 core components in ARCHITECTURE.md:
    - I/O and voices: bus/I-O adapter, MIDI/MPE/NE front end, voice allocator, per-voice envelopes
    - Slots: slot loader, slot handoff + reaper, MipTape<4>, scan playheads
    - Grain engine: voice×stream schedulers (thinning), spawner (region fit + head clamp), pool + quotas, renderer, window morph bank, per-grain FX, level laws, voice-stream mixer
    - Feedback and output: working-copy feedback (4 chains + record heads), reverb + wet safety
    - Modulation: mod sources, control grid + matrix, tempo tracker
    - Record-in capture
- **Features:** 3 points
  - Feedback loops (+1): working-copy feedback, 4 chains
  - Modulation system (+1): global and per-voice LFOs, 2 mod ADSRs, 16-slot matrix
  - External MIDI control (+1): MPE, poly AT, VST3 note expression
  - (No FFT, no multiband)
- **Total:** 2.0 + 22 + 3 = 27.0 → **capped at 5.0**

About 60 % of the DSP is forked, proven O-Granulator code. The genuinely new work:
- The voice dimension: allocator, per-voice matrix, quotas over 80 pairs, onset thinning.
- MipTape with record-head writes and the head clamp.
- The off-thread slot loader and the audio-owned swap.
- Record-in.
- The MPE parser.
- State and relink.

---

## Stages

- Stage 0: Research & Planning ✓ (this document + research/ARCHITECTURE.md)
- **Prerequisite:** mockup + LOCKED parameter-spec.md ← Next
- Stage 1: Foundation — build system, ~261 APVTS params, silent synth shell, buses, COMPAT-01
- Stage 2: DSP — 7 phases (2.1–2.4, 2.5a, 2.5b, 2.6)
- Stage 3: GUI — 3 phases (3.1–3.3)
- Stage 4: Polish/Validation — factory content + presets, DAW passes (Logic AU sidechain, Dorico NE, MPE controller), Windows, i18n, pluginval 10, hands-on

---

## Stage 1: Foundation

**Goal:** The plugin builds (VST3 + AU + Standalone), every parameter is registered, the output is silent, the buses and MIDI are correct, and pluginval passes at strictness 10.

- CMake: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, a new 4-char `PLUGIN_CODE` (verify it is unused across `plugins/*/CMakeLists.txt`), C++20, `juce_audio_formats`, `juce_audio_utils`, `juce_dsp`, WebView (`NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `juce_generate_juce_header`). Modules: grain-engine, pitch-collections, reverb-unit, preset-manager, eb-garamond, note-expression, webview-drop-streaming.
- Buses: `withInput ("Sidechain", stereo, true)`, `withOutput ("Output", stereo, true)`. The layout check accepts the input disabled, mono or stereo, and the output as stereo only. A VST3 extensions subclass (note-expression `VST3Extensions` + `getPluginHasMainInput() = false`).
- `processBlock` clears the output (the shell proves no sidechain leak). `setLatencySamples (0)` in `prepareToPlay`.
- `GiParamIds.h`: loop-generated IDs, the append-only destination table (126) and the source table (18), each with a golden order test.
- State scaffold: `<Engine>` (seed int64, stateVersion, modDestTableVersion = 1, uiLanguage) and an empty `<Slots>`.
- Render harness scaffold (`createEditor` guarded with `#if JUCE_WEB_BROWSER`) with: a MIDI script injection, a synchronous `loadSlotSync` test hook, seed injection, and a synthetic AudioPlayHead.

**Test criteria:**
- [ ] VST3 + AU + Standalone build; auval PASS; pluginval strictness 10 VST3 + AU (COMPAT-01), both with the input bus enabled and with it disabled
- [ ] Sidechain-only render → output bitwise zero
- [ ] The destination and source table golden tests pass; parameter count == parameter-spec.md
- [ ] The NE module configure check passes (JUCE-NE-PATCH marker)

---

## Complex Implementation (Score ≥ 3.0)

### Stage 2: DSP Phases

#### Phase 2.1: Slots, tape, voices, one stream

**Goal:** The load-bearing skeleton: loader → MipTape → audio-owned swap, the voice allocator plus the amp env, one stream per voice, the absolute 32-grid pass engine with MIDI applied at offsets, and the pool. No FX and no feedback (the return path is present and zero).

**Components:**
- Fork step (FORK-MANIFEST.md, namespace `ogi`, drift check script).
- Slot loader (worker thread, Kaiser sinc resample, native pristine, 30 s / 2.88 M-frame cap, `SlotInfo` under an off-audio lock).
- MipTape<4> (`writeRun`, positional reads).
- Slot handoff (`pending` exchange, kill-fade on new file, retire SPSC ring + reaper).
- MIDI front end (non-MPE), voice allocator (20 slots, WtSynthesiser policy port, 5 ms steal), custom amp ADSR (2 ms floors).
- Key-tracked pitch (root/fine/keyTrack/bend, ±48 clamp, tapeRate/fs), scan playheads (all 3 modes, loop region), schedulers (Free), spawner (region fit), `GrainPool<Grain,256,32>`, renderer (Hann), output law per (k,s), voice-stream mixer (env, voice headroom), wet safety, output gain.

**Test criteria:**
- [ ] A 4-note chord gives 4 independently pitched clouds; notes beyond polyphony steal with no click above the gate threshold (FUNC-01)
- [ ] Root note + keyTrack 100 % = file pitch ±1 cent (autocorrelation probe); keyTrack 0 = fixed across keys (FUNC-03)
- [ ] A 44.1 k file in a 96 k session plays at the correct pitch; +24 / +48 st alias ≥ 60 dB (DSP-07)
- [ ] Scan 100 % traverses the region in real time; 0 % holds; Retrigger restarts, Free-run does not (FUNC-04)
- [ ] Load while notes sound: no click, no dropout, 0 audio-thread allocations (FUNC-02, PERF-01/03)
- [ ] MipTape `writeRun` ≡ MipRing (bitwise, settled indices)
- [ ] Host-idle: 100 publishes with no processBlock → ≤ 1 pending per slot; prepareToPlay / setState stress clean
- [ ] Block-size null with a scripted MIDI sequence (32/441/512/4096/var) + different-seed control
- [ ] ADSR stages audible and click-free; velocity scales level per vel_to_amp (DSP-02)

#### Phase 2.2: Four streams, randomisation, windows, scale, quotas + thinning

**Goal:** Full grain parameter parity with O-Granulator across 4 streams × N voices sharing one pool.

**Components:** 80 voice×stream schedulers with jitter, (offset, serial, stream) onset merge, forked WindowMorphBank, ScaleQuantiser (interval mode, degree masks, custom scale), spray/reverse/spread/gain-rand, water-fill over 80 demands, onset thinning, transition stealing with priority, cap lowered live (converging), coherence-blended output law with full-window N_eff, stream mute/solo/enable ramps, per-(k,s) level/pan.

**Test criteria:**
- [ ] DSP-01 parity: 1 voice, root key, scan 0 vs the O-Granulator frozen stream on the same material, within the stated tolerance (−90 dB after gain alignment)
- [ ] Scale quantisation and degree masks are identical to O-Granulator (FUNC-09)
- [ ] Level flat ±1.5 dB across density 1→200 and size with thinning engaged (stationary noise, 4-render average, old-vs-old row)
- [ ] Total grains never exceed the cap; a dense voice/stream cannot starve the others; cap lowered live converges (no-converge control differs) (PERF-02)
- [ ] Steal click gate on a tonal source with verified overflow (QUAL-01)
- [ ] Block-size null still bit-identical; kVoiceGainDb set at the listening pass

#### Phase 2.3: Per-grain FX + CPU budget

**Goal:** DSP-03 and the PERF-02 budget at 16 voices × 4 streams.

**Components:** Forked GrainFx (TPT SVF, drive, crush, ring) with latched `active` flags; NaN kill on SVF state.

**Test criteria:**
- [ ] Each FX audibly per grain; no per-grain allocation; no state bleed on slot reuse (DSP-03)
- [ ] CPU worst case (16 × 4 saturating 256, all FX) ≤ 40 % of one core at 48 k/128 (ratio vs bypass + machine-speed witness); typical ≤ 12 %. If over, apply the levers in ARCHITECTURE order before moving on (PERF-02)
- [ ] Max resonance + drive stress: no NaN/Inf

#### Phase 2.4: Working-copy feedback, Freeze, Reset, reverb

**Goal:** DSP-04, DSP-05.

**Components:**
- Per-(k,s) `g_fb`, amp-weighted mean tap per stream.
- 4 forked FeedbackChains, 32-sample returns.
- Record heads (1×, loop-region wrap, run condition), convex write λ = 0.95·feedback, write-before-read.
- Spawner record-head clamp (with forced-crossing telemetry).
- Freeze (heads stop, chains warm).
- Reset (rebuild from pristine + 20 ms dual-read crossfade swap).
- SR rebuild path.
- ReverbUnit (atomic bind).

**Test criteria:**
- [ ] Feedback 100 % with 16 voices: tape peak ≤ max(pristine, 0.7), output ≤ +6 dBFS, no NaN/Inf/denormal over 60 s (DSP-04)
- [ ] Reset → tape hash == pristine build; Freeze → tape hash unchanged over 10 s; feedback 0 → tape hash == pristine after 60 s (DSP-04)
- [ ] Click gates: reset, freeze toggle, head start/stop vs same-motion controls (QUAL-01)
- [ ] Head-clamp: no crossings when the region allows (telemetry 0); grain read never inside the unsettled mip margin
- [ ] Reverb audible; tails fade on mix → 0 (DSP-05)
- [ ] Block-size null with feedback 80 % + freeze toggles

#### Phase 2.5a: Modulation sources, matrix, tempo (split 2026-11-08, D26)

**Goal:** DSP-06; the final PERF-02 worst-case verdict with the matrix live (D28).

**Components:**
- Mod ADSR ×2, global/per-voice LFOs (`lfoN_mode`), MIDI sources in non-MPE form (velocity, key, note random, mod wheel, channel/poly AT, CC74 slide, channel bend), macros.
- Forked ControlGrid + ModMatrix with per-voice evaluation, the newest-voice rule for global destinations, 20 ms ramps and shadow crossfades (parent D36 closed with no DSP change).
- Forked TempoTracker, Tempo divisions, thinning in Tempo mode (power of two).

**Test criteria:**
- [ ] Parity nulls vs parent @ 82406fce ≤ −90 dB on global LFO / macro / ramps / shadow crossfade (D27)
- [ ] Each listed source routes to a continuous destination with bipolar depth; per-voice sources modulate each voice independently (DSP-06)
- [ ] Per-voice source → global destination follows the newest voice with no step on handover
- [ ] Sync onsets within 1 sample on a synthetic playhead; route-edit click gate; block-size null with every source live
- [ ] CPU worst case with 16 per-voice routes ≤ 40 % (D28)
- [ ] Allocation gate across route edits / mode switches / tempo changes / steals

#### Phase 2.5b: Voice modes, MPE + note expression (split 2026-11-08, D26)

**Goal:** FUNC-05, the MPE/NE engine path (COMPAT-02 is DAW-verified in Stage 4).

**Components:**
- Mono/Legato + glide (note stack), sustain pedal.
- MPE lower-zone parser (RPN 0/6), member-channel rerouting of Pressure / Slide / Bend, note-expression drain + per-note cents.

**Test criteria:**
- [ ] Mono/Legato glide correct; Legato does not retrigger env or scan (FUNC-05)
- [ ] MPE: member-channel bend/pressure/slide move only their own note; RPN 0 sets the member range
- [ ] NE −50 ¢ on C#4 renders a quarter-sharp C4 (harness raw-event injection)
- [ ] 2.5a goldens with `mpe_enable` off unchanged
- [ ] Allocation gate across the whole MIDI/MPE script

#### Phase 2.6: Record-in, state/relink, factory sources

**Goal:** FUNC-06, FUNC-07, the QUAL-02 engine path, FUNC-08 groundwork.

**Components:**
- Capture buffer, triggers (Manual / Threshold / Next Note), length (s / bars, capped), signal-presence discard, copy-out → tape → swap, WAV save to Recordings.
- Availability states.
- `<Slots>` state (source schemes, hash, embed FLAC-24 ≤ 16 MB, resolve order, missing → relink).
- Factory BinaryData target + `factory:` resolution.
- Telemetry (lanes, record head, voices, grain dots, rec state, meters).

**Test criteria:**
- [ ] Capture into each target slot by each trigger; the captured slot plays polyphonically (FUNC-07)
- [ ] Unrouted (silent) sidechain → capture discarded, slot unchanged; no bus → arm inert; output never contains sidechain audio (QUAL-02 engine path)
- [ ] Save/reopen restores 4 files, loop regions, scale state; a missing file gives relink and the plugin stays stable; embedded blob round-trips bit-identical (FUNC-06)
- [ ] Factory sources load with no disk access
- [ ] Allocation gate across record commit and state load; host-idle bound still holds

---

### Stage 3: GUI Phases

#### Phase 3.1: Layout and basic controls
**Goal:** The finalized mockup in the WebView, with every APVTS control bound.
**Components:** Copy the mockup HTML/i18n; PluginEditor WebView setup (explicit resource map, `withBackend(webview2)` + `withUserDataFolder()` on Windows, Juce namespace); relative-drag knobs; the parameter relay for ~261 params.
**Test criteria:**
- [ ] Window opens at 1200×800; layout matches the mockup
- [ ] All pages render; controls move and are bound two-way; host automation updates the UI

#### Phase 3.2: Waveform lanes, file I/O UI, Record-in UI
**Goal:** UI-01.
**Components:** webview-drop-streaming drop into a lane, the file browser, name/relink/trimmed/missing states, loop-region handles (no parameter writes from listeners: handles set each parameter directly), the evolving overview, the record head, per-voice playheads, grain dots by stream, Record-in controls and states, Reset/Embed/Clear native functions.
**Test criteria:**
- [ ] Drag-drop and browser load each slot on macOS (WKWebView path) and Windows
- [ ] Loop handles, playheads, record head and grain dots track the engine at 30–60 Hz
- [ ] Relink flow restores a missing file; Record-in shows "unavailable" / "no signal" correctly

#### Phase 3.3: MOD page, voice section, meters
**Goal:** UI-02.
**Components:** Instrument sources on the MOD page, drag-to-modulate, "mono" badges for per-voice → global routes, mod rings from the newest voice, the voice/amp section, the voice-activity strip, output and sidechain meters (max since read), and the `grain_limit` heavy hint.
**Test criteria:**
- [ ] Drag-to-modulate works from every source chip; rings move for per-voice routes
- [ ] Voice strip reflects notes, steals and releases; meters match the known sine levels
- [ ] HANDS-ON-3.3 + Logic AU DAW pass (sidechain routing, Record-in)

---

### Stage 4: Polish / Validation
- Factory sources (6–8 licence-clean FLACs, ≤ 8 MB) + factory presets (preset-manager; never set `output_gain`; reset-to-defaults first) (FUNC-08).
- DAW passes: Logic (AU sidechain Record-in, unrouted case), Live + Reaper (VST3 aux input), Dorico (NE microtonal, COMPAT-02), an MPE controller (Seaboard/LinnStrument or MPE test MIDI).
- Windows VST3 build + WebView2. i18n (FR, zh-Hans) per the suite lint. pluginval 10 on both formats. CHANGELOG. Fork drift review, recorded in FORK-MANIFEST.md.
- **Addendum, rework R1 (2026-11-12; CONTEXT-R1 D60–D69, PLAN-R1):** after the first execute + verify (PARTIAL) the hands-on session produced findings F1 and F2, and Stage 4 gained one rework cycle before 1.0.0. R1 makes feedback (Amount, Low Cut, High Cut, Saturate, Diffuse, on / off) and reverb (Mix, Size, Damp, Pre / Post) per stream and removes the 8 global parameters (261 → 293 parameters, 126 → 150 destinations, destination table version 2); Freeze and Reset All stay global. It also re-lays the FX · Feedback page behind an approved v2 mockup (D65, Variant B), copies the 64 presets to the new parameters and re-voices 10 as a per-stream showcase, returns Record-in Arm to Off when a take ends or cannot start (D66, amends D34), and ships the "Select a sidechain input" copy (F1). VERSION stays 1.0.0, no tag, no state migration (nothing was published). The cycle ends with the re-authored `HANDS-ON-SESSION-4.md` and a second verify.

---

### Implementation Flow

- Prerequisite: mockup + LOCKED parameter-spec.md
- Stage 1: Foundation (build, params, buses, shell, harness)
- Stage 2: DSP — 6 phases
  - Phase 2.1 Slots / tape / voices / one stream
  - Phase 2.2 Four streams / quotas / thinning
  - Phase 2.3 Per-grain FX + CPU
  - Phase 2.4 Working-copy feedback / Freeze / Reset / reverb
  - Phase 2.5a Modulation sources / matrix / tempo
  - Phase 2.5b Voice modes / MPE + NE
  - Phase 2.6 Record-in / state / factory sources
- Stage 3: GUI — 3 phases (3.1 layout, 3.2 lanes + file I/O, 3.3 MOD + voices + meters)
- Stage 4: Polish/Validation

---

## Implementation Notes

### Thread Safety
- Audio thread: no locks, no allocation, no shared_ptr. Tapes arrive via `pending[s].exchange` and leave via the SPSC retire ring (audio is the only producer).
- `SlotInfo` is guarded by a `CriticalSection` that only off-audio threads take (`prepareToPlay` and `setStateInformation` run on host threads).
- No listener writes another parameter. Actions are native functions.

### Performance
- Grains ≈ 0.11 % of one core each at the worst FX config (parent C7). Projected worst at 256 ≈ 34 %; budget 40 %.
- Per-voice overhead (envs, matrix offsets, 80 schedulers, water-fill) ≈ 2–3 %. 4 feedback chains ≈ 2 %. Reverb ≈ 1.5 %.

### Latency
- 0 samples reported. A note-on spawns its first grain at the event sample.

### Denormal Protection
- `ScopedNoDenormals` per block. NaN scrubs on the loop return, the SVF state and the tape write. Envelopes end at −96 dB.

### Known Challenges
- MipTape incremental mips (bitwise gate vs MipRing).
- Record-head clamp in modular coordinates.
- Instrument + input bus host quirks: kAux typing; fallback is no input bus on VST3.
- Fork drift while the parent finishes Stage 3/4: manifest + hunk review.
- 126-entry host menu for `modM_dest` (acceptable; the UI is the primary surface).

---

## References

- Creative brief: `plugins/O-GranulatorInstrument/.planning/BRIEF.md`
- Requirements: `plugins/O-GranulatorInstrument/.planning/REQUIREMENTS.md`
- Parameter spec (draft): `plugins/O-GranulatorInstrument/.planning/parameter-spec-draft.md` → LOCKED `parameter-spec.md` (pending mockup)
- DSP architecture: `plugins/O-GranulatorInstrument/.planning/research/ARCHITECTURE.md`
- Stage 0 context: `plugins/O-GranulatorInstrument/.planning/stages/0-ideation/CONTEXT.md`

Sibling references:
- O-Granulator: the engine leaves to fork, the pass engine, CPU measurements, the gate harness patterns
- O-simpleWavetable `WtSynthesiser.h`: the steal policy
- O-Contrabass: MPE pitfalls (why the JUCE MPE classes are not used)
- O-MicrotonalSampler `RetiredMapReaper.h`, O-TextureForge 1.6.2, O-simpleSampler: the swap / reaper / lock patterns
- O-simpleGrain: embedded factory sources, FLAC state embedding
- note-expression module (via O-Lyrica / O-Contrabass usage): the Dorico NE path
