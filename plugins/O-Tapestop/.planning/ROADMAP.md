# O-Tapestop - Implementation Plan

**Date:** 2026-08-15
**Complexity Score:** 5.0 (capped; raw 8.0) — Complex
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 14 APVTS parameters (14/5 = 2.8, capped at 2.0) = **2.0**
- **Algorithms:** 5 DSP components = **5**
  - Varispeed voice engine (fractional playhead + 2-voice crossfade)
  - Transport state machine + curve-morph ramp generator
  - Resync controller (fall-behind → accelerate → crossfade-skip)
  - Scratch envelope system (bake + LUT playback)
  - toneTrack speed-tracking LPF
- **Features:** 1 point
  - Modulation system (drawn bipolar speed envelope) (+1)
  - No feedback loops, no FFT, no multiband, no external MIDI
- **Total:** 2.0 + 5 + 1 = 8.0 → **capped at 5.0**

Score ≥ 3.0 → phased implementation. (Note: the raw score overstates effort — 3 of 5 components adapt proven O-ReverseDelay/O-Polystutter substrate; brief's ~1-week estimate stands.)

---

## Stages

- Stage 0: Research & Planning ✓ (this document + research/ARCHITECTURE.md)
- Stage 1: Foundation ← Next (build system, APVTS 14 params, bitwise pass-through shell, COMPAT-01 gate)
- Stage 2: DSP — 3 phases (below)
- Stage 3: GUI — 3 phases (below)
- Stage 4: Validation/Polish — presets, pluginval strictness 10 (VST3+AU), changelog

---

## Complex Implementation (Score ≥ 3.0)

### Stage 2: DSP Phases

#### Phase 2.1: Core Varispeed + Stop/Start

**Goal:** Capture ring + single voice + transport state machine; Stop mode works end-to-end in Free timing; bitwise-bypass path proven.

**Components:**
- CaptureBuffer (from O-ReverseDelay, 26 s, kCaptureSeconds derivation comment + debt jassert)
- VarispeedVoice (fractional position, Catmull-Rom, stereo readAbs)
- TapestopTransport: Bypassed/SpinDown/Stopped/SpinUp states; curve morph p = 2^(2c); mid-ramp reversal via inverse-curve seed; 10 ms stopped-silence fade
- STOP_FREE_MS/START_FREE_MS/curves/MIX/OUTPUT_GAIN wired; Bypassed = hard pass-through
- Offline render harness scaffold (guard createEditor with `#if JUCE_WEB_BROWSER` now)

**Test Criteria:**
- [ ] Plugin loads, passes audio; disengaged output is BITWISE dry (null probe)
- [ ] Engage → spin-down → silence; release → spin-up (no resync yet: simple ramp back onto lagging playhead)
- [ ] Ratio-trace probe: curve 50 % matches x² within tolerance; 0 %/100 % audibly distinct (DSP-02)
- [ ] Discontinuity scan across engage/release sweeps at 3 stop times: no clicks above windowing floor (DSP-01)
- [ ] 512-vs-4096 bit-identity with block-aligned engage edges (QUAL-01)
- [ ] Mid-ramp reversal is speed-continuous (FUNC-01 partial)

#### Phase 2.2: Resync + Tempo Sync

**Goal:** DSP-03 complete (fall-behind → 1.25× catchup ≤250 ms → 50 ms crossfade-skip); FUNC-03 complete.

**Components:**
- Resync controller + second voice + WindowLut Hann-half equal-power crossfade
- Tempo sync: division table + BPM fallback/clamp (O-Polystutter pattern), SYNC_MODE routing, edge-latched durations

**Test Criteria:**
- [ ] Post-resync null vs dry passes (null window starts one crossfade after Catchup ends) (DSP-03)
- [ ] Sync divisions track host tempo changes; Free times match wall-clock within one block (FUNC-03)
- [ ] Skip splice A/B'd (equal-power vs linear) on sustained material; decision recorded
- [ ] Block-size invariance still holds with sync active
- [ ] Rapid engage/release stress (10 Hz toggling) stays click- and NaN-free

#### Phase 2.3: Scratch Mode + toneTrack

**Goal:** FUNC-02, DSP-04, DSP-05 complete; full DSP contract met.

**Components:**
- ScratchEnvelope: JSON blob state (versioned "v":1), sanitize-on-parse, default gentle wobble; message-thread bake → 2048-pt LUT → atomic double-buffer publish; edge-latched pointer
- Scratch pass playback (r = 2y ∈ [−2,+2], reverse via sign, abort-to-resync on release, auto-resync at φ=1)
- toneTrack: FirstOrderTPTFilter, log cutoff law, 16-sample absolute-grid updates, engage-edge state reset
- ENV_SYNC_DIV/ENV_FREE_MS/TONE_TRACK wired

**Test Criteria:**
- [ ] Scratch pass plays drawn LUT once per engage; disengaged mode switch silent (FUNC-02)
- [ ] Bipolar envelope produces reverse playback; direction flips artifact-free (DSP-04)
- [ ] Worst-case full-reverse envelope at 8 s: debt stays within ring bound (probe P4)
- [ ] toneTrack darkens with falling |r|; amount 0 transparent; no zipper on cutoff glide (DSP-05)
- [ ] Pathological input (silence/DC/full-scale impulse) → no NaN/Inf, no sticky state (QUAL-01)
- [ ] processBlock allocation/lock audit clean (PERF-01)

---

### Stage 3: GUI Phases

#### Phase 3.1: Layout and Basic Controls

**Goal:** Mockup HTML integrated, WebView boots, static layout correct.

**Components:**
- Finalized mockup → Source/ui/public/index.html; WebView setup per juce8-critical-patterns (unique_ptr order, explicit resource map, check_native_interop.js, NEEDS_WEB_BROWSER TRUE)
- Knob/select controls for times, curves, toneTrack, mix, gain

**Test Criteria:**
- [ ] WebView opens at correct size; layout matches mockup; no console errors
- [ ] ES6 module loading correct (type="module" + imports)

#### Phase 3.2: Parameter Binding and Engage Control

**Goal:** Two-way binding for all 14 params; engage as a performance control.

**Components:**
- Slider relays (3-arg WebSliderParameterAttachment), getToggleState for ENGAGE, getComboBoxState for choices
- Prominent engage control (UI-02): click == host automation (both go through setValueNotifyingHost)
- SYNC_MODE-driven show/hide of sync-div vs free-ms controls

**Test Criteria:**
- [ ] All controls two-way bind; host automation updates UI; presets update all elements
- [ ] Engage from UI behaves identically to automation (FUNC-01 acceptance)
- [ ] Readouts use getScaledValue (skew-correct)

#### Phase 3.3: Drawable Envelope Editor

**Goal:** UI-01 — bipolar speed-vs-time canvas editor with persistence and pass playhead.

**Components:**
- Canvas editor from Path C §2.2 reference (add/drag/delete points, per-segment curve), adapted bipolar with a labelled 1× line at y=+0.5 and 0/reverse zones
- Native fns: commitEnvelope(json) / requestEnvelope() (SafePointer-hardened; verify getNativeFunction wiring both sides)
- Pass playhead + state readback via atomic + timer + emitEventIfBrowserIsVisible
- Canvas sizing: explicit width/height + DPR backing store (memory: canvas is a replaced element)

**Test Criteria:**
- [ ] Draw → engage plays the drawn curve; edits mid-pass apply next pass
- [ ] Envelope survives save/reload and preset changes; malformed state falls back to default
- [ ] Playhead tracks the pass; hidden-UI edge cases don't hang promises
- [ ] Mode switch shows/hides editor appropriately

---

### Implementation Flow

- Stage 1: Foundation — project structure, 14-param APVTS, pass-through shell, pluginval gate
- Stage 2: DSP — Phase 2.1 → 2.2 → 2.3 (git commit per phase; render-harness probes are the gates)
- Stage 3: GUI — Phase 3.1 → 3.2 → 3.3
- Stage 4: Validation — factory presets (incl. classic 1/2-bar stop, DJ spinup, 2 scratch gestures), pluginval strictness 10 VST3+AU, CHANGELOG

---

## Implementation Notes

### Thread Safety
- APVTS atomics with cached raw pointers; no locks/allocations in processBlock (PERF-01)
- ScratchLut double-buffer + atomic pointer, edge-latched; JSON parse strictly message-thread
- UI readback: atomics + editor timer, emission gated on visibility

### Performance
- < 5 % single core estimated @ 48 kHz (2-voice interp worst case, one pow/sample, tan per 16 samples)
- Memory: 26 s stereo ring (9.2 MB @ 44.1 k, 40 MB @ 192 k) — allocate in prepareToPlay only

### Latency
- Zero; no setLatencySamples needed

### Denormal Protection
- ScopedNoDenormals in processBlock; TPT snapToZero; stopped fade lands on exact 0.0f

### Known Challenges
- Resync splice aesthetics (fallback: repeated small skips) — see ARCHITECTURE.md Implementation Risks
- Block-size invariance: engage edges are block-quantized — harness must schedule edges on 4096-aligned boundaries; cutoff updates on absolute 16-sample grid
- Factory presets: author in engineering units + convertTo0to1 (memory: linear-fraction presets ignore skew)
- Choice params need ≥2 choices (all do); avoid param-ID identifiers that shadow juce:: free functions

---

## References

- Creative brief: `plugins/O-Tapestop/.planning/BRIEF.md`
- Requirements: `plugins/O-Tapestop/.planning/REQUIREMENTS.md`
- Parameter spec: `plugins/O-Tapestop/.planning/parameter-spec-draft.md` (promote to parameter-spec.md at mockup finalization)
- DSP architecture: `plugins/O-Tapestop/.planning/research/ARCHITECTURE.md`
- UI mockup: none yet — envelope editor is the main design challenge

**Reference plugins:**
- O-ReverseDelay — CaptureBuffer/WindowLut/voice-POD substrate; render-harness probe style
- O-Polystutter — tempo sync + BPM fallback, UI progress bridge, varispeed read loop precedent
- O-Freeze — engage-style gate state handling
