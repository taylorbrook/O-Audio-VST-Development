# O-Contrabass - Implementation Plan

**Date:** 2026-04-25
**Complexity Score:** 5.0 (Complex — capped from raw 16.0)
**Strategy:** Phase-based implementation across all stages with explicit DSP sub-phases
**Tier:** 6 (Deep — multi-feature physical model + drone features + microtonal + MPE + Note Expression)

---

## Complexity Factors

- **Parameters:** 29 parameters → min(29/5, 2.0) = **2.0** (capped)
- **Algorithms:** 9 distinct DSP components = **9**
  1. Friction Junction (hyperbolic, oversampled, sub-harmonic biased)
  2. String Waveguide (4-string, with dispersion + bridge LP + DC blocker + in-loop saturator)
  3. Cascaded Allpass Dispersion (Rauhala/Välimäki, M=1–4 sections per string)
  4. Body Resonator (8-mode parallel biquad bank + 35 Hz HP dry path)
  5. Bow Noise Generator (3-band BPF + slip bursts)
  6. Slow-Bow LFO (Schelleng-aware diagonal modulation)
  7. Vibrato Section (delay-line modulation + onset envelope)
  8. Master Saturator + Zero-Latency Limiter
  9. Microtonal Tuning Engine (Scala/TUN + MTS-ESP + VST3 Note Expression + MPE pitch-bend)
- **Features:** **5** complexity points
  - Feedback loops (waveguide round-trip) +1
  - Modulation systems (Slow-Bow LFO + Vibrato + Expression Macro) +1
  - External MIDI control (MPE + VST3 Note Expression) +1
  - Sub-harmonic generation (period-doubling nonlinearity) +1
  - Multi-band processing (8-mode parallel body bank) +1
- **Raw Total:** 16.0
- **Capped Total:** **5.0** (saturated at maximum)

This is a Tier 6 (Deep) plugin requiring fully phased implementation.

---

## Stages

- Stage 0: Research & Planning ✓ (this document)
- Stage 1: Foundation ← **Next**
- Stage 2: DSP (6 sub-phases)
- Stage 3: GUI (3 phases)
- Stage 4: Polish (presets, pluginval, Dorico verification, changelog)

---

## Open Decisions Resolved

These six decisions were surfaced by Stage 0 research. Recommendations made here, defer-to-implementation flagged where appropriate.

### 1. Friction tier in v1.0 (hyperbolic only vs + elasto-plastic toggle)

**Recommendation: Hyperbolic only.** Defer elasto-plastic to v1.1.

**Rationale:** Hyperbolic curve is monotonic (no Newton convergence risk), validated in O-Bowed production, and bass slow-attack character means elasto-plastic memory effect is least audible here vs violin. Adding Newton-Raphson on top of E1+drone-mode stability challenge multiplies risk without proportional sonic gain. Sub-harmonic period-doubling works equally well on hyperbolic friction.

**Implementation:** Phase 2.1 ports `O-Bowed/Source/DSP/HyperbolicFriction.h` directly with bass-tuned defaults (`mu_s=0.85`, `mu_d=0.25`, `v_0=0.05 m/s`).

---

### 2. Module extraction timing (before vs during Stage 2)

**Recommendation: Extract DURING Stage 2, mid-Phase 2.1.**

**Rationale:**
- Extracting before Stage 2 is premature — bass-tuned defaults aren't yet validated.
- Extracting after v1.0 ships duplicate code that's harder to refactor.
- The Phase 2.1 mid-point (after E-string single-voice validation) is the natural extraction moment: bass defaults proven, O-Bowed reference is known-good, both plugins can wire up the new module simultaneously.

**Implementation plan:**
- **Phase 2.1a (E-string single-voice):** Inline copy of `HyperbolicFriction.h` from O-Bowed into `plugins/O-Contrabass/Source/DSP/`. Validate 60-second sustain at max INFINITE_SUSTAIN.
- **Phase 2.1b (extraction):** Create `modules/dsp/bow-friction/` (proposed name `ouaricon_bow_friction` if naming convention permits). Move `HyperbolicBowTable`, `BowState` envelope, `SchellengGuard` to module. Both O-Bowed and O-Contrabass switch to consuming module. Re-validate O-Bowed (1 day regression).
- **Phase 2.1c (sub-harmonic bias):** Add `SubHarmonicBias` to module with `apply()` API, called from O-Contrabass voice but not O-Bowed (initially).

**Cost:** +1 day O-Bowed regression test. Acceptable.

**Fallback:** If extraction proves harder than expected, keep inline copy and schedule extraction as v1.1 task.

---

### 3. Wolf "Authentic Arco" toggle in v1.0 vs v1.1

**Recommendation: Defer to v1.1.**

**Rationale:**
- Default (suppressed wolf) is the more useful character for both orchestral and drone presets.
- The "authentic" wolf coupling violates the parallel-only body bank architecture (re-injects body output back into bridge termination) — adds debugging surface.
- Risk: poorly-tuned wolf coupling can sound "glitchy" rather than "authentic."
- v1.0 still gets default wolf suppression (Q-modulation on Mode #2 when fundamental locks within ±15 cents).

**Implementation:** v1.0 ships with `wolfCoupling` parameter hidden (compile-time disabled) but architecture supports it. v1.1 exposes the toggle and tunes coupling gain (research suggests 0.02–0.05 with 40 Hz HP).

---

### 4. Sub-harmonic depth at max (1 octave vs 2)

**Recommendation: 1 octave at maximum (default tuning).**

**Rationale:**
- Period-doubling bifurcation produces stable f0/2 (1 octave down) reliably.
- f0/3, 2f0/3, 3f0/4 patterns are accessible but increasingly unstable — entering chaotic regime past f0/2 is a known failure mode.
- For the bass register specifically, f0/2 of E1 (41 Hz) is 20.5 Hz — already infrasound, perceived more as "weight" than "pitch."
- A 2-octave-down setting would target ~10 Hz — sub-audible and likely to crash sub speakers.
- 1-octave maximum keeps the regime "musically stable" per the brief's QUAL-02 requirement.

**Implementation:**
- `SUB_HARMONICS = 1.0` biases friction toward strong f0/2 generation but clamps `kForceBoost` at 1.8 (research §1.3 default).
- Lag-2 RMS chaos detector at control rate as runtime backstop.
- If f0/3 or chaos detected, auto-back-off bias by 20%.

---

### 5. Body Size knob physical mapping (1/4 → 4/4 vs 1/2 → full)

**Recommendation: Map 0–100% to 1/4 child bass → 4/4 jumbo orchestral (full span).**

**Rationale:**
- Research §3 already specifies this mapping: `size_scalar = 0.85 + 0.30 · s` produces 1.83:1 frequency span across the knob (E1 A0 mode at 95 Hz @ 0% → 52 Hz @ 100%).
- 75% default = 3/4 standard bass (the most common professional instrument size).
- Using full 1/4 → 4/4 span gives users access to character extremes (small student bass for percussive low-mid focus; jumbo for orchestral fullness).
- Restricting to 1/2 → full would lose the "small bass" character useful for chamber/jazz contexts.

**Implementation:** Use research §3 mapping verbatim. Knob label: "Body Size" with tooltip "1/4 (small) — 4/4 (jumbo)".

---

### 6. Wood material variants (single vs spruce-bright/maple-warm)

**Recommendation: Single fixed wood for v1.0. Re-evaluate for v1.1 based on user feedback.**

**Rationale:**
- O-Contrabass is bass-only by design; morphable material is O-Bowed's territory.
- Single fixed material reinforces "deep specialization" identity.
- Default 8-mode bank (Askenfelt-derived) is already the population-mean for 3/4 spruce-top bass — covers the most common use case.
- Body Damping/Mix offers enough flexibility to span "bright" → "warm" character without changing the underlying mode set.
- Adding spruce/maple toggle now adds UI complexity and tuning effort (per-variant Q/gain tweaks). Defer.

**Implementation:**
- v1.0: single material, mode set per architecture §"Body Resonator" table.
- v1.1: add `WOOD_VARIANT` choice param (Spruce-Bright / Maple-Warm / Aged-Mellow); each variant ships a different default mode set.

---

## Stage 1: Foundation

**Goal:** JUCE plugin skeleton, build system, parameter spec, module dependencies.

**Components:**
- Create `plugins/O-Contrabass/CMakeLists.txt` with:
  - `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE` (juce8-critical-patterns #22)
  - `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (Windows WebView)
  - `NEEDS_WEB_BROWSER TRUE` for VST3
  - Modules: `juce_audio_basics`, `juce_audio_processors`, `juce_dsp`, `juce_audio_plugin_client`, `juce_gui_basics`, `juce_gui_extra`, `juce_audio_utils`
  - Shared modules: `modules/tuning/scala-tuning-engine`, `modules/tuning/note-expression`
- `Source/PluginProcessor.{h,cpp}` skeleton:
  - `BusesProperties` output-only in constructor (synth pattern)
  - APVTS with all 29 parameters
  - `prepareToPlay` initializes oversampler, reports latency via `setLatencySamples()` (NOT override `getLatencySamples()` — non-virtual in JUCE 8)
- `Source/PluginEditor.{h,cpp}` minimal stub (full GUI in Stage 3)
- Verify pluginval strictness 10 baseline (COMPAT-01)

**Test Criteria:**
- [ ] `ninja O-Contrabass_VST3 O-Contrabass_AU` succeeds on macOS
- [ ] `cmake --build build --config Release --target O-Contrabass_VST3` succeeds on Windows
- [ ] All 29 APVTS parameters appear in DAW automation menu
- [ ] Plugin loads in Logic Pro, Ableton, Reaper, Dorico, Cubase without error
- [ ] pluginval strictness 10 passes (no audio output yet — bypass mode only)
- [ ] `auval -a | grep -i contrabass` shows AU registered

---

## Stage 2: DSP (6 phases)

### Phase 2.1: Core Single-String Engine (HIGHEST RISK — DO FIRST)

**Goal:** Stable E-string waveguide with hyperbolic friction at all `INFINITE_SUSTAIN` settings.

**Sub-phases:**

**2.1a — E-string + hyperbolic friction (inline):**
- Port `O-Bowed/Source/DSP/HyperbolicFriction.h` to `plugins/O-Contrabass/Source/DSP/HyperbolicFriction.h`
- Apply bass-tuned defaults (`mu_s=0.85`, `mu_d=0.25`, `v_0=0.05`)
- Single `juce::dsp::DelayLine<float, Lagrange3rd>` for E1
- One-pole bridge filter with `INFINITE_SUSTAIN` → `g` mapping (0.997 → 0.99995, ceiling 0.9999999)
- Constant `−1e-20` denormal leak in bridge filter
- 2x oversampling via `juce::dsp::Oversampling<float>` (`filterHalfBandPolyphaseIIR`)
- DC blocker (R=0.999) inside loop after bridge filter
- In-loop algebraic saturator (`x / sqrt(1 + x²)`)
- `juce::ScopedNoDenormals` at `processBlock` entry

**2.1b — Module extraction:**
- Create `modules/dsp/bow-friction/` (or appropriate path per registry)
- Move `HyperbolicBowTable`, `BowState`, `SchellengGuard` from inline copy to module
- O-Bowed switches to consuming module
- Re-validate O-Bowed (no regression — full QC test pass)
- O-Contrabass switches to consuming module

**2.1c — Cascaded allpass dispersion:**
- Implement `DispersionFilter<MaxSections=4>` per architecture §"Cascaded Allpass Dispersion"
- E-string uses M=4 sections at full STIFFNESS
- Coefficient computation via Rauhala/Välimäki closed-form (research §2.3)

**Test Criteria:**
- [ ] E1 played at default params produces stable bowed tone (no NaN, no denormal CPU spikes)
- [ ] 60-second sustain at max INFINITE_SUSTAIN, default other params: no runaway, no NaN
- [ ] STIFFNESS 0% → 100% sweep produces continuous timbral change (no clicks)
- [ ] Dispersion at 100% audibly affects attack character but not steady-state pitch (mode-locking)
- [ ] BRIGHTNESS sweep 80 Hz → 12 kHz: no clicks
- [ ] Module extraction: O-Bowed full QC pass (no regressions)
- [ ] CPU < 1% on M1 at 44.1 kHz / 256-sample block

---

### Phase 2.2: 4-String + Per-String Detune

**Goal:** Add A1, D2, G2 strings + per-string detune ±1200 cents click-free.

**Components:**
- 4 `juce::dsp::DelayLine<float, Lagrange3rd>` instances (8192 samples each)
- Per-string dispersion: A1=M3, D2=M2, G2=M1 (architecture §"String Waveguide Bank")
- 4 `juce::SmoothedValue<float, Linear>` for `DETUNE_E/A/D/G` (20 ms ramp, smoothed in delay-samples space)
- Active string selection: bow energy routed to one string at a time
- MIDI note → string mapping: closest-open-string-then-fingered logic
- String switching: 5 ms fade on previous string state to avoid click
- ACTIVE_STRINGS parameter restricts available strings (1=E only, 4=all)

**Test Criteria:**
- [ ] All 4 strings produce tone at default tuning (E1, A1, D2, G2)
- [ ] MIDI notes E1 through G3 select correct strings + fingered positions
- [ ] String switching during sustained notes: no audible click (5 ms fade)
- [ ] Per-string detune ±1200 cents: no clicks during automation sweep
- [ ] Vibrato with detune active: no zipper noise
- [ ] Just-intoned drone preset (DETUNE_A=+204¢, DETUNE_D=−14¢, DETUNE_G=+182¢ for 7-limit) produces beating-free chord
- [ ] CPU < 2% on M1 (4 strings always allocated, only one active at a time)

---

### Phase 2.3: Modulators (Vibrato + Slow-Bow LFO + Schelleng Guard)

**Goal:** Bass-tuned vibrato section + Schelleng-aware slow-bow modulation.

**Components:**
- Vibrato: sine LFO + `VibratoOnsetEnvelope` (0–3000 ms onset, S-curve fade-in over 300 ms)
- Vibrato modulates delay-line length via `setDelay()` (Lagrange3rd absorbs cleanly)
- Slow-Bow LFO: 0.05–2 Hz sine, 23° pressure phase-lag
- Schelleng wedge bounds (`F_min`, `F_max`) computed once per block from current `v_b`, `beta`
- Slow-Bow LFO depth clamped to 80% of remaining wedge headroom
- Anti-correlation guard: vibrato rate offset by +0.13 Hz when SLOW_LFO_DEPTH > 0
- 20 ms `SmoothedValue` on speed/pressure modulators
- EXPRESSION_MACRO knob: layers onto BOW_SPEED ×1.0–1.4, BOW_PRESSURE ×1.0–1.6, VIBRATO_DEPTH ×1.0–1.3, BRIGHTNESS +0–500 Hz

**Test Criteria:**
- [ ] Vibrato at default (5 Hz, 12 cents, 600 ms onset) sounds natural on sustained note
- [ ] Vibrato onset envelope: silence → fade-in starts at 600 ms → full depth at 900 ms (S-curve)
- [ ] Slow-Bow LFO at 0.3 Hz, 50% depth produces audible breathing without leaving wedge
- [ ] At extreme bow params (high pressure + low speed), Slow-Bow LFO depth auto-clamped
- [ ] EXPRESSION_MACRO sweep 0 → 100% layers correctly across all four destinations
- [ ] No friction-junction artifacts at LFO zero-crossings (20 ms smoothing)

---

### Phase 2.4: Drone Features (Sub-Harmonics + Infinite Sustain stress test)

**Goal:** Period-doubling sub-harmonic bias + max sustain stability.

**Components:**
- `SubHarmonicBias` integrated into friction junction (architecture §"Sub-Harmonic Bias")
- 30 ms cross-fade on `SUB_HARMONICS` parameter changes
- Optional control-rate (~100 Hz) lag-2 RMS chaos detector with 20% auto-back-off
- Energy clamp `softClampState` at junction (threshold 0.85, ceiling 1.0)
- Opposite-sign denormal guards (+1e-20 and −1e-20) on delay-line state
- Final stability test matrix: `{INFINITE_SUSTAIN ∈ {0, 0.5, 1.0}}` × `{SUB_HARMONICS ∈ {0, 0.5, 1.0}}` × `{BODY_DAMPING ∈ {0, 0.5, 1.0}}` × 4 strings = 108 combinations × 60 s hold

**Test Criteria:**
- [ ] SUB_HARMONICS at 50% on E1 produces audible f0/2 (~20.5 Hz, perceived as "weight")
- [ ] SUB_HARMONICS at 100% does not enter chaotic regime (lag-2 RMS detector active)
- [ ] All 108 stability matrix combinations: no NaN, no runaway, no denormal CPU spike, peak ≤ 0 dBFS
- [ ] Bow disengage (note-off) with INFINITE_SUSTAIN=100%: tone decays gracefully (does not blow up)
- [ ] Re-trigger note during high-state sustain: no thump, no level spike
- [ ] QUAL-01 acceptance: no audible clicks during any parameter sweep at any combination

---

### Phase 2.5: Body Resonator + Bow Noise

**Goal:** Bass-tuned wood body + close-mic bow noise texture.

**Components:**
- 8-mode parallel biquad body bank (per architecture §"Body Resonator")
- 35 Hz HP on dry path
- Body Size scaling (`size_scalar = 0.85 + 0.30·s`), Body Damping (`Q · (1 − 0.85·d)`), Body Mix (wet/dry)
- Per-block coefficient recomputation + 30 ms `SmoothedValue` on Size/Damping/Mix
- Default wolf-region suppression (Mode #2 Q drop on fundamental lock)
- 3-band BPF bow-noise generator (700 Hz / 1500 Hz / 3000 Hz) driven by `bowEnergy = |v_b|·F_bow / (v_ref·F_ref)`
- Per-period slip bursts (exponential decay, `decay = 0.999`)
- Bow noise sums AFTER body resonator

**Test Criteria:**
- [ ] Impulse response shows 8 spectral peaks at 60/98/115/175/235/340/700/1200 Hz with correct relative gains
- [ ] BODY_SIZE 0% → 100% sweep: peaks slide smoothly, no zipper noise
- [ ] BODY_DAMPING sweep: ring-down time changes audibly without clicks
- [ ] BODY_MIX at low frequencies (E1 = 41 Hz): smooth amplitude blend, no comb-filter teeth
- [ ] Wolf region (G2 sustained, default damping): slight bloom but no audible beating
- [ ] BOW_NOISE 0% → 100%: noise level audible at low pressure, fades to silence at zero
- [ ] Bow direction reversal: brief noise burst (5–15 ms wideband decay)
- [ ] Orchestral character: A/B vs Spitfire Albion bass sustain at G2 — should be in same sonic family

---

### Phase 2.6: Output Chain + Microtonal + MPE

**Goal:** Master saturator/limiter, stereo width, full Ouaricon microtonal convention.

**Components:**
- Master saturator (polynomial `x − x³/3` with input pre-clamp ±1.5)
- Zero-latency feedforward limiter (3 ms attack / 100 ms release / −1 dBFS, computed on 2x oversampled signal)
- Stereo width (M/S encode → side scale → decode); side-channel slightly drier on body mix
- Output gain (post-limiter, `juce::Decibels::decibelsToGain`)
- Integrate `modules/tuning/scala-tuning-engine` v2.1.0 (Scala/TUN + MTS-ESP)
- Integrate `modules/tuning/note-expression` (VST3 Note Expression for Dorico)
- Implement `BowedContrabassMPESynthesiser` extending `juce::MPESynthesiser`
- `enableLegacyMode(2, {1, 17})` for non-MPE host fallback
- Voice receives `TuningEngine*` and `APVTS*` pointers (O-Lyrica pattern)
- Voice-side tuning drain: `pendingTuningSemis[pitch].exchange(0.0)` in `noteStarted` BEFORE friction-junction trigger
- `VST3ClientExtensions::queryIEditController` returns `INoteExpressionController` advertising `kTuningTypeID`

**Test Criteria:**
- [ ] PERF-01: no allocations, locks, or file I/O in `processBlock` (verified by code review + pluginval RT-safety check)
- [ ] PERF-03: `setLatencySamples()` reports oversampler latency only (1–3 samples), no algorithmic latency
- [ ] QUAL-02: extreme drone settings + max output gain produce limited output ≤ −1 dBFS
- [ ] WIDTH 0% (mono) → 200% (wide): smooth stereo expansion, no phase issues
- [ ] FUNC-05: MPE pitch/Y/Z all control corresponding bow params per-note
- [ ] FUNC-06: Dorico microtonal score plays back with correct per-note tuning offsets
- [ ] FUNC-07: Scala/TUN file load → all subsequent notes use loaded tuning
- [ ] FUNC-07: MTS-ESP master plugin retunes O-Contrabass live
- [ ] CPU at typical settings: < 5% on M1 (PERF-02 target)

---

## Stage 3: GUI (3 phases)

### Phase 3.1: Layout and Basic Controls

**Goal:** WebView UI integration with mockup HTML, basic parameter binding for all 29 parameters.

**Components:**
- Copy mockup HTML to `Source/ui/public/index.html` (mockup created in separate workflow)
- WebView setup in `PluginEditor.{h,cpp}` per juce8-critical-patterns #3, #7, #11
- Resource provider with explicit URL mapping (juce8-critical-patterns #8)
- Cross-platform WebView (juce/https URL scheme switching)
- Windows: `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `withUserDataFolder()`
- 29 `WebSliderParameterAttachment` bindings (juce8-critical-patterns #12)
- 7 logical sections per UI-01: Bow / Body / Strings / Expression / Drone / Output / Microtonal

**Test Criteria:**
- [ ] WebView opens on plugin launch in Logic, Ableton, Reaper, Cubase
- [ ] All 29 parameters visible and styled per mockup
- [ ] Layout responsive within plugin window bounds (no overflow, no scrolling)
- [ ] Sections grouped per UI-01 requirement
- [ ] Resource provider returns correct paths (no "Frame load interrupted" errors)
- [ ] Windows: WebView2 backend confirmed (not IE fallback)

---

### Phase 3.2: Parameter Binding and Interaction

**Goal:** Two-way parameter sync, host automation, preset loading.

**Components:**
- JS → C++ relay calls via `valueChangedEvent` callback (juce8-critical-patterns #15)
- C++ → JS parameter updates (host automation, preset changes)
- Boolean parameter use `getToggleState` (juce8-critical-patterns #19)
- Knob relative-drag interaction (juce8-critical-patterns #16)
- `check_native_interop.js` included (juce8-critical-patterns #13)
- Preset selection menu populated from preset bank
- Tuning system choice + Scala/TUN file picker integration

**Test Criteria:**
- [ ] Knob movement updates DSP parameter (audio reflects change)
- [ ] DAW automation moves UI control
- [ ] Preset selection updates all 29 controls atomically
- [ ] Scala/TUN file picker opens, loads file, displays tuning name
- [ ] Tuning System dropdown switches priority correctly
- [ ] Note Expression toggle enables/disables NE controller advertisement

---

### Phase 3.3: Advanced UI Elements

**Goal:** Visual polish — Schelleng wedge indicator, body bank visualization, expression meter.

**Components:**
- Optional: Schelleng wedge visualization (X=bow speed, Y=bow pressure, current point + wedge bounds)
- Optional: Body bank spectrum display (8 modes with current Q-modulated peaks)
- Optional: VU/RMS meter (juce8-critical-patterns #20)
- Visual style supports dual cinematic-orchestral / drone-experimental identity (UI-02)

**Test Criteria:**
- [ ] Schelleng wedge updates in real-time as bow params change
- [ ] Body spectrum reflects current Size/Damping settings
- [ ] No CPU spikes from visualization (60 fps max update rate, throttled)
- [ ] Visual style matches mockup aesthetic

**Note:** UI-02 is `nice` priority. If Phase 3.3 runs over budget, ship Phase 3.2 only and defer visualization to v1.1.

---

## Stage 4: Polish (Validation, Presets, Changelog)

**Goal:** Production-ready plugin with full preset banks, Dorico verification, cross-DAW testing.

**Components:**

**4.1 — Preset Banks:**
- Orchestral bank (5 presets):
  - Cinematic Bass Sustain (default)
  - Section Bass
  - Solo Arco Bass
  - Pianissimo Bass
  - Forte Bass
- Drone bank (5 presets):
  - Infinite Drone
  - Just-Intoned Drone (7-limit detune on D/A/G)
  - Scordatura Bass
  - Sub Drone
  - Dark Pad Bass

**4.2 — Verification:**
- pluginval strictness 10: VST3 macOS, AU macOS, VST3 Windows
- Cross-DAW load test: Logic Pro, Ableton Live 11+, Reaper, Cubase 13, Dorico 6, Bitwig (MPE)
- Dorico microtonal playback (COMPAT-02): pre-configured `.doricoexpmap` file in installer
- 60+ second sustain stability (FUNC-02 acceptance)
- A/B vs reference: Spitfire Albion bass sustain (orchestral) + SunnO))) (drone)
- CPU benchmark at typical settings → confirm < 5% (PERF-02)

**4.3 — Documentation:**
- Update `CHANGELOG.md` for v1.0.0
- Write user manual section for Ouaricon microtonal setup (Scala/MTS-ESP/Dorico)
- Document the 6 open decisions and rationale (carry-forward for v1.1)
- Update `PLUGINS.md` to "Working" then "Installed" status

**Test Criteria:**
- [ ] All 10 presets load + sound musically correct
- [ ] pluginval strictness 10 passes all 3 builds
- [ ] Dorico microtonal score plays back correctly (verified using spike pattern)
- [ ] 60-second sustain at default params: no artifacts, no drift
- [ ] CPU < 5% at typical settings on M1 / Apple Silicon
- [ ] CHANGELOG.md and user manual complete

---

## Implementation Flow Summary

```
Stage 1 (Foundation, ~1 day)
  ├─ CMakeLists.txt + APVTS skeleton
  └─ pluginval baseline pass (silent plugin)

Stage 2 (DSP, ~6-8 days)
  ├─ Phase 2.1 (Core Single-String, ~2 days) ← HIGHEST RISK
  │   ├─ 2.1a E-string + hyperbolic
  │   ├─ 2.1b Module extraction + O-Bowed regression
  │   └─ 2.1c Cascaded allpass dispersion
  ├─ Phase 2.2 (4-String + Detune, ~1 day)
  ├─ Phase 2.3 (Modulators + Schelleng, ~1 day)
  ├─ Phase 2.4 (Drone Features + 108-combo stability matrix, ~1.5 days)
  ├─ Phase 2.5 (Body + Bow Noise, ~1 day)
  └─ Phase 2.6 (Output + Microtonal + MPE, ~1.5 days)

Stage 3 (GUI, ~2-3 days)
  ├─ Phase 3.1 (Layout + Basic Bindings)
  ├─ Phase 3.2 (Two-way Sync + Presets)
  └─ Phase 3.3 (Optional Visualization)

Stage 4 (Polish, ~1-2 days)
  ├─ Preset banks
  ├─ Cross-DAW validation
  ├─ Dorico verification
  └─ Changelog + user manual
```

**Total estimate: 10-15 days of focused work** (subject to module-extraction surprises, GUI mockup complexity, Dorico setup edge cases).

---

## Implementation Notes

### Thread Safety

- All APVTS reads via `getRawParameterValue()->load()` (atomic).
- Tuning table swap: background → audio thread via atomic pointer.
- VST3 raw event queue: lock-free SPSC (single producer = wrapper, single consumer = audio thread).
- `juce::ScopedNoDenormals` at top of `processBlock` (mandatory).
- Body coefficient recompute happens in audio thread (no allocations).
- LFO/vibrato phase state per voice (mono = single voice; pattern preserves correctness).

### Performance

Estimated CPU on M1, 44.1 kHz, 256-sample block, single voice:
- Total: **~3.2%** (well under PERF-02's 5% target)
- Dominant components: oversampling (0.4%), waveguide @ 2x (0.7%), body bank (0.4%), bow noise (0.4%)
- Headroom for: Authentic Arco wolf coupling, bow-noise spectral shaping, optional FFT body convolution

### Latency

- Algorithmic: 0 samples (PERF-03 requirement met)
- Oversampler: 1–3 samples (`filterHalfBandPolyphaseIIR`)
- Reported via `setLatencySamples(static_cast<int>(std::ceil(oversampler.getLatencyInSamples())))` in `prepareToPlay`
- **CRITICAL:** Do NOT override `getLatencySamples()` — non-virtual in JUCE 8 (per memory file)

### Denormal Protection

- `juce::ScopedNoDenormals` at `processBlock` entry (handles MXCSR FTZ+DAZ + ARM FZ).
- Bridge filter constant leak `−1e-20` outside drone mode.
- Opposite-sign delay-line state guards (`+1e-20`, `−1e-20`) at junction.
- 1024-entry friction lookup table (no `expf`/`sqrtf` per sample in hot path).

### Known Challenges

- **E1 + drone-mode stability:** Phase 2.1 must validate this BEFORE adding any further features. Use lag-2 RMS chaos detector as runtime backstop.
- **Module extraction mid-Phase 2:** O-Bowed regression test cost (1 day) is unavoidable. Plan for it.
- **JUCE-NE-PATCH dependency:** Build will fail loudly without the patch on `~/JUCE/`. Document in README.
- **Dorico expression map setup:** Ship pre-configured `.doricoexpmap` in installer + clear setup docs.
- **`getLatencySamples()` non-virtual:** Use `setLatencySamples()` in `prepareToPlay`.
- **WebView2 on Windows:** `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (memory: dynamic loading without static linking causes blank UI silently).
- **Resource provider receives PATHS, not full URLs** (memory: don't strip scheme/host).

---

## References

### Contracts

- Creative brief: `plugins/O-Contrabass/.planning/BRIEF.md`
- Requirements: `plugins/O-Contrabass/.planning/REQUIREMENTS.md`
- Parameter spec (draft): `plugins/O-Contrabass/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md`

### Research Documents (canonical inputs)

- `research/O-Contrabass-research-synthesis.md` — top-level synthesis
- `research/O-Contrabass-bass-waveguide-stability.md` — waveguide DSP detail
- `research/O-Contrabass-body-acoustics.md` — body bank acoustics
- `research/O-Contrabass-drone-and-subharmonics.md` — drone features detail
- `research/bow-string-friction-models.md` — friction theory (general reference)
- `research/O-Bowed-research-synthesis.md` — sibling plugin (general bowed strings)

### Reference Plugins (sibling implementations)

- **O-Bowed** — friction junction, waveguide string, body resonator (morphable). Source of `HyperbolicFriction.h` extraction. Reference for `BowedStringVoice` MPE pattern.
- **O-Lyrica** — Note Expression integration pattern (Patterns 1–5). Voice receives `TuningEngine*` + `APVTS*`. Reference for SynthesiserVoice with microtonal layer.

### Spike-Validated Patterns

- `.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md` — JUCE-NE-PATCH + Patterns 1–5 for Dorico microtonal playback.

### Critical JUCE Patterns

- `troubleshooting/patterns/juce8-critical-patterns.md` — 22 patterns, especially:
  - #1 CMakeLists.txt header generation
  - #3 WebView module requirements
  - #4 Bus configuration (effects vs instruments)
  - #7 WebView ↔ Parameter binding
  - #8 WebView resource provider explicit URL mapping
  - #11 WebView member initialization (unique_ptr)
  - #12 WebSliderParameterAttachment (3 params required)
  - #15 valueChangedEvent callback (no parameters)
  - #16 WebView knob relative drag
  - #19 WebView boolean params (getToggleState)
  - #22 IS_SYNTH flag for instruments

### Shared Modules

- `modules/tuning/scala-tuning-engine` v2.1.0 — Scala/TUN file import + MTS-ESP runtime retuning
- `modules/tuning/note-expression` — VST3 Note Expression helper + JUCE patch
- `modules/dsp/bow-friction` — TO BE CREATED in Phase 2.1b (extracted from O-Bowed `HyperbolicFriction.h`)
