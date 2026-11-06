# O-Granulator - Implementation Plan

**Date:** 2026-11-04
**Complexity Score:** 5.0 (capped; raw 22.0) — Complex
**Strategy:** Phase-based implementation

complexity_score: 5.0
complexity_tier: 6
research_depth: DEEP
staged_implementation: true

---

## PREREQUISITE (blocking Stage 1) — ✓ DONE 2026-11-04

UI mockup v2 finalized (`mockups/v2-ui.yaml`, `finalized: true`) and the LOCKED `parameter-spec.md` written from it:
1. ✓ UI mockup — v2 (STREAMS / FX · FEEDBACK / MOD pages, buffer strip with write head + stream bands + grain dots, 16-slot matrix with drag-to-route mod rings, 32-entry microtonal scales with degree chips and one shared custom scale). Implementation files: `mockups/v2-ui.html`, `v2-i18n.js`, `v2-PluginEditor.h/.cpp`, `v2-CMakeLists.txt`, `v2-integration-checklist.md`.
2. ✓ `parameter-spec.md` — 220 parameters; `scale_root`, `sN_onset_trigger` ×4 and `grain_limit` confirmed from ARCHITECTURE.md "Design Sync".
3. ✓ Mod-destination enumeration frozen (113 entries, append-only) in parameter-spec.md, plus the non-parameter state (custom scale, per-stream × per-collection degree masks, seed, table version) and the PitchCollections extraction note.

Stage 1 builds from `parameter-spec.md` (NOT the draft).

---

## Complexity Factors

- **Parameters:** 214 draft (220 with proposed additions) → 214/5 = 42.8, capped = **2.0**
- **Algorithms:** 17 core components = **17**
  - I/O adapter, capture ring (MipRing<2>), write arbiter (freeze/xfade), stream schedulers ×4, grain spawner, shared pool (stream quotas), grain renderer, window morph bank, per-grain FX chain, level normaliser (two laws), stream mixer, feedback network, output reverb, dry/wet + wet safety, mod sources (LFO/env/onset/MPM/macros), control grid + matrix, transport tracker
- **Features:** 3 points
  - Feedback loop (+1)
  - FFT processing — MPM pitch follower (+1)
  - Modulation system — 4 LFOs, followers, 16-slot matrix (+1)
  - (No multiband, no external MIDI)
- **Total:** 2.0 + 17 + 3 = 22.0 → **capped at 5.0**

The raw score overstates novelty: ring/reader, pool, scheduler, RNG, loop topology, reverb and the matrix slot model are proven sibling code. The genuinely new work is the 32-grid pass engine around feedback, stream-fair quotas, the window morph bank, MPM + onset detection, and absolute-mode pitch follow. Effort estimate: Stage 2 is the bulk (6 phases).

---

## Stages

- Stage 0: Research & Planning ✓ (this document + research/ARCHITECTURE.md)
- **Prerequisite:** mockup + final parameter-spec.md ✓ (v2, 2026-11-04)
- Stage 1: Foundation ← Next — build system, ~220 APVTS params, pass-through shell, COMPAT-01
- Stage 2: DSP — 6 phases (below)
- Stage 3: GUI — 3 phases (below)
- Stage 4: Polish/Validation — presets, state, mono/Windows, i18n, hands-on, pluginval 10

---

## Stage 1: Foundation

**Goal:** Plugin builds (VST3 + AU + Standalone), all parameters registered, bitwise pass-through, pluginval strictness 10.

- CMake: `juce_audio_utils`, `juce_dsp` (FFT, ReverbUnit), WebView (`NEEDS_WEB_BROWSER TRUE`, `juce_generate_juce_header`), grain-engine + reverb-unit + preset-manager via `OuariconModules.cmake`; C++20 (grain-engine requires it).
- `GranulatorParamIds.h`: loop-generated IDs for `sN_`, `lfoN_`, `macroN`, `modM_` + the **append-only destination table** (113) with a golden test of the ID order.
- Buses: mono→mono, mono→stereo, stereo→stereo; channel counts from buses.
- `setLatencySamples(0)` in `prepareToPlay`.
- Engine state ValueTree child: `seed` (int64), `stateVersion`, `modDestTableVersion`.
- Render harness scaffold (guard `createEditor` under `#if JUCE_WEB_BROWSER`), seed injection hook, synthetic AudioPlayHead.

**Test criteria:**
- [ ] VST3 + AU + Standalone build; auval PASS; pluginval strictness 10 VST3 + AU (COMPAT-01)
- [ ] Output bitwise equals input at all params default with mix at 0 % (shell)
- [ ] Destination-table golden test passes
- [ ] Parameter count matches parameter-spec.md exactly

---

## Complex Implementation (Score ≥ 3.0)

### Stage 2: DSP Phases

#### Phase 2.1: Ring + pass engine + one stream

**Goal:** The load-bearing skeleton: absolute 32-grid passes, write-before-read, MipRing<2>, one stream, pool, Hann window, no FX, no feedback yet (return delay present and zero).

**Components:** I/O adapter, capture ring (41 s, static_assert derivation), write arbiter (live only), StreamScheduler (Free, telescoping jitter), spawner (position/spray/size/pitch/reverse, reach clamp, double read positions), GrainPool<Grain,128,16>, renderer (readStereo + window last), output law (incoherent, cap-aware N_eff), dry/wet equal-power + output gain, control grid with base ramps.

**Test criteria:**
- [ ] Input audible within one onset at position 0 (FUNC-01)
- [ ] buffer_length change: no click, no stale region read (FUNC-01)
- [ ] mix 0 % bitwise dry; mix 100 % no dry (FUNC-07)
- [ ] +24 st swept sine: aliases ≥ 60 dB down (DSP-02)
- [ ] Long-buffer pitched read: periodic-corpus null + double-reference at 25 s (DSP-01)
- [ ] Block-size null 32/441/512/4096/variable, randomisation ON, seeded + different-seed control (PERF-03)
- [ ] Allocation gate across spawn/steal (PERF-01)

#### Phase 2.2: Four streams, randomisation, pitch/scale, window morph, normalisation

**Goal:** Full grain parameter set across 4 streams sharing one pool.

**Components:** 4 schedulers + (offset, stream) onset merge, stream-fair stealing (parameter-derived quotas), WindowMorphBank (65 tables, half-window moments), skew warp, scale quantiser (interval mode), stereo spread, gain rand, coherence-blended output law, stream mixer (level/pan/mute/solo, 10 ms ramps).

**Test criteria:**
- [ ] Each stream independently audible; disabled streams spawn nothing (FUNC-03)
- [ ] Total grains never exceed cap; a dense stream cannot starve the others (FUNC-03, PERF-02)
- [ ] Quantised grains on scale degrees within 1 cent (DSP-01)
- [ ] Window morph RMS within ±1 dB (DSP-03); level flat ±1.5 dB across density 1→200 with averaged stationary-noise renders + old-vs-old row
- [ ] reverseProb 0/100 % exact, 50 % within binomial bounds; spray/spread 0 deterministic (DSP-04)
- [ ] Steal click gate: tonal source at ~0.001, pool overflow verified (QUAL-01)
- [ ] Block-size null still bit-identical

#### Phase 2.3: Per-grain FX + CPU budget

**Goal:** DSP-05 and the first CPU measurement against the budget.

**Components:** per-grain TPT SVF (LP/BP/HP, latched coefficients, ≤ 0.45·fs), drive tanh(dx)/d, crush (hold + mid-rise bits, probability), ring mod (quadrature rotator), latched `active` flags, NaN kill on SVF state.

**Test criteria:**
- [ ] Each FX audibly per-grain; randomised values differ between grains (DSP-05)
- [ ] No bleed: slot reuse after a resonant grain starts from zero state (DSP-05)
- [ ] CPU worst case (128 grains, all FX) ≤ 30 % of one core at 48 k/128 as a ratio vs engine-bypass baseline; typical preset ≤ 10 % (PERF-02). If over: apply levers in ARCHITECTURE.md order before moving on
- [ ] Max resonance + max drive stress: no NaN/Inf (QUAL-02)

#### Phase 2.4: Freeze, feedback network, output reverb

**Goal:** FUNC-02, FUNC-05, DSP-06, DSP-07.

**Components:** head-stop freeze with heldSpan latch, freeze micro-spray floor, engage forced-release, 10 ms content crossfade on release; feedback tap (coherent law ÷ Σlevel), HP → LP → diffuser mix → tanh(dx)/d → AGC → NaN scrub → z⁻³²; ReverbUnit via raw-pointer bind (module G4 or local copy); wet safety.

**Test criteria:**
- [ ] Frozen: level-0 ring hash identical across 10 s of new input (FUNC-02)
- [ ] Freeze on/off click gate vs same-motion control (FUNC-02, QUAL-01)
- [ ] Frozen crest factor at spray 0 ≤ 1.5× live; freeze 0.5 s after load not silent; 60 s drift < 0.5 dB
- [ ] Feedback recirculates; tone/sat/diffusion each change the loop output (FUNC-05)
- [ ] Freeze + feedback behaves as documented (return discarded, chain warm) (FUNC-05)
- [ ] 60 s at feedback 100 %, density 200, pitch +12, 4 streams +6 dB, res 100 %: peak ≤ +6 dBFS, no NaN (DSP-06)
- [ ] dB/generation < 0 with input removed, crediting 48.6 ms diffuser delay, measured at −52 dBFS
- [ ] Reverb audible, tails fade on mix → 0 (DSP-07)
- [ ] Block-size null with feedback 80 % + freeze toggles at shared boundaries

#### Phase 2.5a: Tempo sync + LFO/env/macro sources + modulation matrix

*Split from the original Phase 2.5 at the 2.5 discuss, 2026-11-05 (CONTEXT-2.5a D13).*

**Goal:** FUNC-04, FUNC-06, QUAL-03. Also re-gates QUAL-01, PERF-01 and PERF-03 under modulation.

**Components:** TempoTracker port (ppqAt), grid lock + re-time on discontinuity, sync divisions for size/density/LFO, power-of-two density modulation in Tempo mode; LFO ×4 (hybrid phase, D15; S&H/smooth via hash(cycle)); per-sample env follower; macros; 16-slot matrix in normalised space with 20 ms slot ramps and route-change shadow crossfades; the matrix writes the reverb atomics; telemetry atomics for 113 destinations and the source values. The Pitch and Onset sources are wired into the matrix as inert (0) slots until 2.5b.

**Test criteria:**
- [ ] Neutral null first: every slot None, sync Free, bitwise vs the 2.4 tree
- [ ] Synced onsets within 1 sample of the host grid at jitter 0 (synthetic playhead) (FUNC-04)
- [ ] Free density within 2 % over 10 s at jitter 0 (FUNC-04)
- [ ] Every 2.5a source modulates any destination, bipolar; removing a route returns to base without a step (FUNC-06)
- [ ] Sweeps of position/pitch/cutoff/mix via LFO free of zipper (QUAL-03)
- [ ] Host automation of matrix params: host cache matches plugin (no nested writes; pedalboard + CFRunLoop pump)
- [ ] Block-size null with LFO + env + macro routings active, tempo synced
- [ ] DAW tempo check at verify (D16)

#### Phase 2.5b: Onset detector + burst + MPM pitch follower + absolute pitch follow

*Split from the original Phase 2.5 (CONTEXT-2.5a D13).*

**Goal:** FUNC-08, DSP-08. Also completes FUNC-06 for the Pitch and Onset sources.

**Components:** onset detector + burst trigger (`sN_onset_trigger`, re-phase + clock runs, D14); decimator + MPM pitch follower (juce::dsp::FFT order 11) + absolute pitch follow with `scale_root`; the Pitch and Onset matrix sources go live.

**Test criteria:**
- [ ] Onset burst fires at the detected sample on enabled streams (FUNC-08)
- [ ] Pitch follower: sine 55–1000 Hz within 10 cents, clarity hold on silence/noise (DSP-08)
- [ ] Pitch and Onset sources modulate any destination (FUNC-06 completion)
- [ ] Block-size null with LFO + env + onset + pitch routings active
- [ ] DAW check at verify: onset bursts on drums (D16)

#### Phase 2.6: Hardening

**Goal:** Close QUAL-01/02, PERF-01/02/03 across the whole feature set.

**Test criteria:**
- [ ] Extreme-settings stress render (all maxima, every source routed): no NaN/Inf/denormal (QUAL-02)
- [ ] Click gates: freeze toggle, grain steal, enable/mute/solo, matrix route edits (QUAL-01)
- [ ] Allocation gate: zero allocations over spawn/steal/freeze/matrix edits/transport jumps (PERF-01)
- [ ] Worst-case CPU within budget; `grain_limit` engages deterministically (PERF-02)
- [ ] Full-feature block-size null 32/441/512/4096/variable + oversized host block (8192 after prepare 512) (PERF-03)
- [ ] 96 k and 192 k smoke renders (filters clamp, ring memory as documented)

### Stage 3: GUI Phases

#### Phase 3.1: Layout + controls + bindings
- Copy mockup HTML; WebView setup (resource provider explicit URL map, `Juce` namespace, `check_native_interop.js`, WebView2 static + `withBackend`/`withUserDataFolder` on Windows)
- Stream selector (4 tabs/strips) with per-stream panels; global, feedback, reverb, LFO/followers/macros panels
- All ~220 relays + attachments (WebSlider 3-arg form); readouts via `getScaledValue()`; tempo/free swap shows the right control without writing params
- **Test:** every parameter reachable and bound both ways (UI-01); host automation reflected

#### Phase 3.2: Buffer display + live grains
- Waveform overview from the peak ring, write head, heldSpan when frozen, per-stream position/spray bands (modulated values), grain dots from the seqlock snapshot at 30 Hz; canvas text after `document.fonts.load()` repaint
- **Test:** UI-02; no audio-thread contention (telemetry is copy-only)

#### Phase 3.3: Mod matrix view + mod rings
- 16-slot source → destination → depth editor (destination picker grouped by stream/section), mod-amount rings on targets from the 113 atomics, source meters
- **Test:** UI-03; route edits click-free (audio)

### Stage 4: Polish / Validation
- Factory presets: ambient, harmoniser (scale_root + pitch_follow), rhythmic (Tempo), feedback-cascade (FUNC-09) via preset-manager; presets never set output_gain; apply resets to defaults first
- State round-trip on a fresh instance: all params + 16 routings + seed (COMPAT-02)
- Mono/stereo layouts; Windows VST3 build (COMPAT-03)
- EN/FR/zh-Hans (UI-04): glossary + lint first; pin content-sized boxes at the widest language
- Hands-on DAW listening pass incl. freeze-semantics A/B (head-stop vs virtual-head fallback) (QUAL-04)
- pluginval strictness 10 VST3 + AU, auval, CHANGELOG `## [1.0.0]`

---

## Implementation Flow

```
Mockup + parameter-spec.md ──► Stage 1 ──► 2.1 ──► 2.2 ──► 2.3 ──► 2.4 ──► 2.5a ──► 2.5b ──► 2.6 ──► 3.1 ──► 3.2 ──► 3.3 ──► Stage 4
```
Each phase ends with a git commit (path-scoped to `plugins/O-Granulator`) and its gates green.

---

## Implementation Notes

### Thread Safety
- Audio thread owns all DSP state; no background threads (determinism).
- No parameter writes from `parameterChanged` or the audio thread (VST3 swallows nested host-originated writes).
- Telemetry: relaxed atomics + seqlock/triple buffer; editor timer 30 Hz.

### Performance
- Budget: worst case ≤ 30 % of one core @ 48 k/128; typical ≤ 10 %.
- Deterministic degradation only: pool 128, stream quotas, `grain_limit`.
- Levers: FX active flags → Hermite reads for rate ≤ 1 → SIMD-4 grains → pool 96.

### Latency
- Report 0. Dry path untouched. MPM ≈ 45 ms affects only the pitch source.

### Denormal Protection
- `ScopedNoDenormals`; NaN scrubs on loop return, grain SVF, followers; custom ramps (not SmoothedValue reset, which copies a NaN target).

### Known Challenges
- **Feedback runaway** at high overlap: coherent tap law + 0.95 cap + tanh + AGC + wet safety. Measure dB/generation, not dB/s.
- **Freeze buzz / silence** with head-stop: micro-spray floor, heldSpan latch, forced release, content crossfade. Fallback virtual head.
- **Block-size invariance** with 4 streams + matrix + followers: absolute 32-grid; null gate every phase.
- **CPU** at 128 grains with FX.
- **Module gaps G1–G4** are optional; G4 (ReverbUnit raw bind) is the only one on the critical path, with a local-copy fallback.

---

## References

- Contract: `plugins/O-Granulator/.planning/research/ARCHITECTURE.md`
- Requirements: `plugins/O-Granulator/.planning/REQUIREMENTS.md`
- Draft params: `plugins/O-Granulator/.planning/parameter-spec-draft.md`
- Substrate: `modules/synthesis/grain-engine` v1.1.0, `modules/effects/reverb-unit` v1.1.0, `modules/persistence/preset-manager`
- Siblings: O-ReverseDelay (loop), O-GrainScatter (tempo, freeze, tests), O-Freeze, O-simpleGrain, O-Prism (matrix)
