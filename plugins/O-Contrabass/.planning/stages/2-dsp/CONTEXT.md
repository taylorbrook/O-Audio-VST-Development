# Stage 2: DSP — Context (rev-4)

**Date:** 2026-04-27
**Plugin:** O-Contrabass
**Stage:** 2 of 4 (DSP)
**Phase:** discuss
**Cycle Scope:** **Phase 2.2 — 4-String Bank + Per-String Detune + Per-String Dispersion Table**
**Supersedes:** rev-3 (Phase 2.1c — Cascaded Allpass Dispersion, Gate 3, dated 2026-04-27). rev-3 contracts that remain locked are inherited verbatim and not re-litigated.

---

## Discussion Summary

**Participants:** User, Claude

This discuss cycle opens Phase 2.2 — expansion from single-string E1 to the full 4-string EADG bank, with per-string detune ±1200¢, per-string dispersion (M=4/3/2/1 with B=1e-4/7e-5/5e-5/3e-5 prefactors), MIDI note → string mapping, ACTIVE_STRINGS handling, and click-free string-switching. Phase 2.1 closed 2026-04-27 with R20 atomic commit (`5759e5e`); the friction junction, split-rail waveguide, cascaded allpass dispersion, and bow-friction module are all in place and validated.

The Phase 2.2 scope is a single coupled cycle: four `WaveguideString` instances inside one mono voice; bow energy routes to one selected string at a time per `ACTIVE_STRINGS` and the MIDI note's open-string mapping. Detune ramps in delay-samples space (architecture-locked). Dispersion uses Phase 2.1c's runtime `setActiveSections()` API to configure M per string at `prepare()`. String switches mid-sustain crossfade equal-power over 5 ms. Atomic-commit on Gate 4 PASS.

After Phase 2.2 verifies, Phase 2.3 (modulators: vibrato + Slow-Bow LFO + Schelleng wedge) opens as a fresh GSD cycle.

---

## Cycle Scope

**Goal:** Add A1, D2, G2 strings to the existing E1 voice. All four strings allocated permanently (not stolen). Bow engages exactly one at a time, selected by MIDI note → open-string mapping (highest open-string at-or-below the note, clamped by ACTIVE_STRINGS). Per-string detune ±1200¢ ramps click-free. Per-string M=4/3/2/1 dispersion with locked B prefactors. String-to-string transitions during sustained bowing crossfade equal-power over 5 ms with no audible click. E1 behaviour at default tuning + STRING_STIFFNESS=0 stays bit-exact identical to the Phase 2.1c golden (regression bar — Phase 2.2 must not change E1 in any way).

**In scope:**
- `Source/BowedContrabassVoice.{h,cpp}` — replace single `waveguideString` member with `std::array<WaveguideString, 4>` keyed E/A/D/G. Add per-string `juce::SmoothedValue<float, Linear>` detune (4 instances, 20 ms ramp, smoothed in delay-samples). Add `activeStringIndex` + `previousStringIndex` + `crossfadeRemainingSamples` for string-switching. Add MIDI note → string mapping helper (closed-form thresholds 28/33/38/43, ACTIVE_STRINGS clamp). Per-block: advance all 4 detune smoothers, advance all 4 stiffness smoothers, recompute per-string dispersion `a` from current smoothed stiffness, push each via `setDispersionCoefficient()`. Per-sample: friction injection only into active string; tick all 4 strings (so silent strings keep idle leak state, no cold-start on reactivation); equal-power crossfade mix during the 5 ms transition window.
- `Source/BowModel.{h,cpp}` (review only) — confirm bow state is voice-level (one bow regardless of string count), no per-string changes needed. Bass detune defaults `0.85f / 0.25f` already wired Phase 2.1b.
- `Source/DSP/WaveguideString.{h,cpp}` — minor surface additions (NOT topology changes): per-instance `setStringIndex()` or `prepare()` parameter so each instance can apply its own M and B prefactor. The existing `MaxSections + activeSections` + `setDispersionCoefficient()` API from Phase 2.1c carries the per-string M without re-templating.
- `tests/render-harness/main.cpp` — add CLI flags for the new gate invariants:
  - `--string {E,A,D,G}` to override the MIDI note → string mapping (forces a specific string for per-string sustained-tone harnesses).
  - `--detune-sweep {E,A,D,G}` to ramp the chosen string's `DETUNE_*` parameter from −1200 → +1200 cents over the configured sustain duration; emit WAV + JSON with `mode: detune-sweep`, `string: <X>`, `detuneRamp: {start: -1200, end: 1200, shape: linear}`.
  - `--note-sequence "MIDI:duration[,MIDI:duration...]"` for the string-switching gate test (programmatic note-on sequence; e.g., `--note-sequence "28:2.0,33:2.0,38:2.0,43:2.0,28:2.0"` exercises E→A→D→G→E with 2 s per note).
- `Source/PluginProcessor.cpp` — no changes expected (APVTS already declares all four `DETUNE_*` parameters and `ACTIVE_STRINGS` from Stage 1; voice reads them directly).

**Out of scope (deferred to later Phase 2.x cycles):**
- Vibrato + Slow-Bow LFO + Schelleng wedge clamp (Phase 2.3)
- Sub-harmonic bias + 108-combo stability matrix (Phase 2.4)
- Body resonator + bow noise (Phase 2.5)
- Master saturator/limiter, stereo width, microtonal, MPE (Phase 2.6)
- ARCHITECTURE.md §"DC Blocker" + §"In-loop saturator" amendments (end-of-Stage-2 verify per locked decision)
- Phase 2.4 calibration polynomial follow-up for E1 closed-form clamp (parked per RESEARCH §14.10 Risk #7)

---

## Requirements Confirmed (Phase 2.2-relevant subsets of locked contracts)

- **DSP-01** (waveguide string, Lagrange3rd, 8192-sample buffer): primary deliverable — replicated × 4 instances, one per string. Buffer size carries forward verbatim (sized for E1 worst case at −1200¢ + 88.2 kHz = 4282 samples; rounded to 8192).
- **DSP-03** (cascaded allpass dispersion per string): per-string M-table now materialises (E=4, A=3, D=2, G=1). Phase 2.1c's runtime-`activeSections` design carries this without re-templating. Per-string B prefactors lock per ARCHITECTURE.md §"String Waveguide Bank" inharmonicity table (E1=1e-4, A1=7e-5, D2=5e-5, G2=3e-5, all multiplied by `STRING_STIFFNESS` ∈ [0,1]).
- **FUNC-01** (4-string EADG voicing): Phase 2.2 satisfies at acceptance level for the bow path. (MIDI tuning resolution incl. Note Expression / MTS-ESP / Scala / 12-TET comes in Phase 2.6.)
- **FUNC-02** (sustained tone, no runaway, no NaN): carry-forward from Phase 2.1; per-string sustained-tone harness on A1/D2/G2 + portamento sequence harness extend the bar to all 4 strings.
- **PERF-01** (no allocations, no locks, no file I/O in `processBlock`): enforced — all 4 string instances allocated in `prepareToPlay`; 4× detune smoothers + 4× stiffness smoothers preallocated; crossfade state is plain integer counter.
- **PERF-02** (< 5 % CPU on M1): tracked — 4 strings always tick (so silent strings stay warm via leak), but only the active string runs friction injection. Idle-string cost is ~3 multiplies + 1 delay-line `popSample`/`pushSample` + dispersion's M-section cascade. Estimated ~0.4 % CPU per idle string × 3 idle = ~1.2 % overhead. Under the 5 % budget.
- **PERF-03** (latency = oversampler only): in effect; per-string dispersion group-delay compensation already wired (Phase 2.1c R17 plumbing in `updateDelayLengths()`). Detune smoothing in delay-samples space does not change reported plugin latency.
- **QUAL-01** (no audible clicks during parameter sweeps): explicit Gate 4 invariants — DETUNE sweep ±1200¢ (per string) + ACTIVE_STRINGS toggle + string-switching mid-sustain all click-free.

---

## Constraints Identified

**Locked contracts (do NOT modify in this cycle):**
- All 29 APVTS parameter IDs, ranges, skews — `parameter-spec.md` (sha256:c47fe736…)
- DSP architecture (`research/ARCHITECTURE.md`, sha256:3cb26814…) — F3 deviation flagged through Phase 2.1; ARCHITECTURE amendment still deferred to end-of-Stage-2 verify
- ROADMAP phasing (sha256:106639f6…)
- `modules/synthesis/bow-friction/` v1.0.0 (Phase 2.1b) — module is value-class deterministic; Phase 2.2 does NOT touch friction
- `Source/DSP/DispersionFilter.h` (Phase 2.1c, R20 commit `5759e5e`) — Phase 2.2 consumes the public API verbatim; no edits
- `Source/DSP/WaveguideString.{h,cpp}` topology (split-rail bridgeDelay/neckDelay, F2 LP form, F3 no in-loop DCB, dispersion before bridge LP) — Phase 2.2 may add per-instance config surface but MUST NOT change the loop structure

**JUCE 8 critical patterns (auto-loaded `spike-findings-VST-development` + memory):**
- `getLatencySamples()` is non-virtual — keep using `setLatencySamples()` in `prepareToPlay`; per-string dispersion compensation does not change reported latency.
- `juce::ScopedNoDenormals` at `processBlock` entry (mandatory). Idle-string state benefits from FTZ; rely on existing scope. Constant `−1e-20` leak in bridge filter (already wired) keeps idle strings out of denormal territory.
- `juce::SmoothedValue<float, Linear>` per ARCHITECTURE.md §"String Waveguide Bank" line 84 — NOT `Multiplicative`. Smooth in **delay-samples space**, NOT cents (avoids logarithmic warping at low f0).
- `juce::dsp::DelayLine<float, Lagrange3rd>` per-sample `setDelay()` during ramp is JUCE-validated for click-free continuous modulation (vibrato pattern; Phase 2.1c implicit confirmation).
- `IS_SYNTH TRUE` + output-only `BusesProperties` already in place from Stage 1.
- Both WebView2 flags already in place from Stage 1.

**Phase 2.2-specific constraints:**
- **Single-voice, 4-string topology** — ONE `BowedContrabassVoice` instance, holding `std::array<WaveguideString, 4>` keyed E/A/D/G. NOT 4 voices in MPESynthesiser. Bow state is voice-level (one bow at a time, mono). MPESynthesiser keeps voice count = 1.
- **All strings always allocated and ticking** — silent strings still call `popSample`/`pushSample` per sample (no friction injection, but topology runs) so they stay warm via the existing leak. No cold-start when bow re-engages a previously-silent string.
- **MIDI → string mapping is closed-form** — `string = max{E:28, A:33, D:38, G:43} where threshold ≤ midiNote`. Clamp by `ACTIVE_STRINGS`: if the chosen string's index exceeds `ACTIVE_STRINGS - 1`, demote to the highest-allowed string. Notes below MIDI 28 → E1 string (detuned down via finger position). Notes above MIDI 55 (G3) → G2 string fingered very high (architecture line 280; musically thin but honest).
- **String-switching trigger semantics** — only on note-on transitions (new note maps to a different string than current). Mid-note pitchbend / portamento stays on the current string (DB players don't smoothly cross strings during a note — they un-fret up the neck). This keeps Phase 2.2 mechanics simple; revisit only if Phase 2.3+ vibrato/slow-bow modulation surfaces a need.
- **Crossfade is at voice-mix bus level** — equal-power (sin/cos) ramp over 5 ms from old string's mix coefficient to new string's. Both strings' DSP runs during the crossfade window; only the mix coefficients change. Friction injection switches to the new string at crossfade-start (sample-accurate); old string's bridge-rail energy decays naturally via leak.
- **ACTIVE_STRINGS is mid-sustain tolerant** — going 4→1 while bowing on (e.g.) D2 does NOT yank the string out. Current note keeps ringing on D2 until note-off; the next note-on respects ACTIVE_STRINGS=1 and remaps to E1. (Research-phase confirms whether this needs an explicit handler or falls out of the note-on-only switching policy naturally.)
- **E1 bit-exact regression bar** — Phase 2.2 must NOT change E1 behaviour at `DETUNE_E=0 + ACTIVE_STRINGS≥1 + STRING_STIFFNESS=0`. The Phase 2.1c golden `tests/render-harness/golden/stiffness-zero-pre.wav.sha256 = d358abcd…` carries forward as a Gate 4 invariant — render must be byte-identical. This proves the 4-string refactor is purely additive on the E-string code path.
- **No mid-cycle architecture amendment** — if any Phase 2.2 issue surfaces (e.g., string-switching click that 5 ms equal-power crossfade can't suppress, idle-string CPU exceeding budget, detune-sweep zipper at extreme cents), document as Phase 2.4+ follow-up RESEARCH note. Do not rework architecture mid-cycle.

**Working-tree starting state (locked from Phase 2.1c verify, R20 commit `5759e5e`):**
- `Source/BowedContrabassVoice.{h,cpp}` — single E1-only voice with one `WaveguideString`
- `Source/DSP/WaveguideString.{h,cpp}` — split-rail with M=4 dispersion (Phase 2.1c rev-3 R20)
- `Source/DSP/DispersionFilter.h` (130 LOC) — public API: `prepare(M)`, `setActiveSections(M)`, `setCoefficient(a)`, `processSample(x)`, `reset()`. Phase 2.2 uses this verbatim
- `Source/PluginProcessor.{h,cpp}` — 29 APVTS parameters incl. all 4 `DETUNE_*` and `ACTIVE_STRINGS` (Stage 1 commit)
- `modules/synthesis/bow-friction/` v1.0.0 — module is value-class deterministic; Phase 2.2 does NOT touch friction
- E1 sustained drone (bow-on-only 65 s @ INFINITE_SUSTAIN=1.0): sha256 `0cc6ed4c…` (deterministic across retries)
- E1 STRING_STIFFNESS=0 golden: sha256 `d358abcd…` (Phase 2.1c regression bar, carries forward as Gate 4 invariant)

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Q1 — Cycle scope** | **Single Phase 2.2 cycle** covering 4-string bank + per-string detune + per-string dispersion M-table + MIDI→string mapping + ACTIVE_STRINGS handling + 5 ms string-switching crossfade | Six tasks are tightly coupled. M-table just sets `activeSections` per string at prepare(); detune is a delay-samples offset on the existing Lagrange3rd buffer; ACTIVE_STRINGS is a clamp on the mapping output. Splitting buys little — there's no known sub-gate that would benefit from independent gating (unlike Phase 2.1's a/b/c which had genuinely distinct risk surfaces: friction stability, module surface, dispersion algorithm). User-confirmed. |
| **Q2 — Voice topology** | **Single voice holds `std::array<WaveguideString, 4>`** (keyed E/A/D/G) | Architecture line 28 ("Bow engages one string at a time (mono)") is the operative constraint. Bow state is global to the voice — one velocity, one position, one pressure. Single voice with internal string-routing is the natural mapping. MPESynthesiser stays at voice count = 1. ACTIVE_STRINGS as a global parameter aligns with single-voice semantics. User-confirmed. |
| **Q3 — MIDI → string mapping** | **Highest open-string-at-or-below the MIDI note**, with thresholds {E:28, A:33, D:38, G:43}. Clamp output by `ACTIVE_STRINGS - 1` (demote if chosen string index > active limit). Notes < 28 → E string. Notes > 55 (G3) → G string fingered up | Closed-form 4-way threshold ladder = 4 if-comparisons total. Idiomatic DB convention (architecture line 280 specifies exactly this). Matches user's "whatever is simpler to code" — single-pass thresholding is genuinely simpler than any closest-distance or lowest-then-up alternative. |
| **Q4 — ACTIVE_STRINGS behaviour for out-of-range notes** | **Remap to highest active string** (the one chosen by Q3 mapping, demoted to fit ACTIVE_STRINGS clamp). Plugin always speaks. | Silent-fail is bad UX. Demoting MIDI 50 (D2 string) to E1 with ACTIVE_STRINGS=1 fingers it way up the E string — sounds thin, but musically honest. User-confirmed. |
| **Q5 — String-switching click suppression** | **5 ms equal-power crossfade at voice mix-bus**. Both strings' DSP runs during the 5 ms window; only the mix coefficients ramp (sin/cos). Friction injection switches to new string at crossfade-start. Trigger only on note-on transitions that map to a different string than current | 5 ms = 220 samples @ 44.1k = 423 samples @ 88.2k. Equal-power (sin/cos) preserves perceived loudness through the transition. Fade-out-then-engage gives a 5 ms gap; not acceptable. Crossfade adds 1 mix multiply per sample × 4 strings = trivial CPU. User-confirmed. |
| **Q6 — Detune smoothing** | **Inherited verbatim from ARCHITECTURE.md §"String Waveguide Bank" line 84**: `juce::SmoothedValue<float, Linear>`, 20 ms ramp, smoothed in **delay-samples space** (NOT cents — avoids log warping at low f0). Per-sample `setDelay()` during ramp | Architecture-locked. JUCE-validated pattern (vibrato on Lagrange3rd is identical mechanic). User-confirmed (no override). |
| **Q7 — Per-string dispersion config** | **Inherited verbatim from ARCHITECTURE.md §"String Waveguide Bank" inharmonicity table**: E=`setActiveSections(4)` + B prefactor 1e-4 · STRING_STIFFNESS; A=3 + 7e-5; D=2 + 5e-5; G=1 + 3e-5. Applied at `prepareToPlay` per string instance | Phase 2.1c already shipped runtime `setActiveSections()` API specifically to enable this. No new template parameters; just per-instance config at prepare(). User-confirmed. |
| **Q8 — Gate 4 invariants** | **Eight-item bar:** (1) per-string sustained drone harness × 3 (A1/D2/G2 each: 60 s @ INFINITE_SUSTAIN=1.0, 4/4 invariants TRUE — same template as Phase 2.1a-recovery's bow-on-only harness, replicated per string); (2) DETUNE sweep ±1200¢ on A1 (or any one chosen string) over 30 s — RMS continuity ≥ 99 % adjacent-block ratio (no clicks); (3) string-switching test via `--note-sequence "28:2.0,33:2.0,38:2.0,43:2.0,28:2.0"` — no clicks, no NaN, all 4 strings produce tone; (4) ACTIVE_STRINGS=1 + MIDI 50 → produces tone on E1 (no silence); (5) auval AU VALIDATION SUCCEEDED; (6) pluginval --strictness-level 10 SUCCESS; (7) **E1 STRING_STIFFNESS=0 bit-exact regression** — render at default tuning + ACTIVE_STRINGS=4 + MIDI 28 + STRING_STIFFNESS=0 must be byte-identical to Phase 2.1c golden sha256 `d358abcd…`; (8) Logic AU smoke (user-deferred, non-blocking, mirroring R19f / R14e precedent) — audition E1→A1→D2→G2 sweep + ACTIVE_STRINGS knob | First six items extend Phase 2.1c's automated bar to the 4-string surface. Item (7) is the strongest possible "Phase 2.2 didn't break E1" check — same regression-bar philosophy as Phase 2.1b's bit-exact O-Bowed canonical render and Phase 2.1c's stiffness=0 identity. Item (8) is the qualitative "does it sound like a contrabass yet" smoke. User-confirmed. |
| **Q9 — Atomic commit unit** | **R21+ Phase 2.2 atomic commit** lands `BowedContrabassVoice.{h,cpp}` edits + minor `WaveguideString.{h,cpp}` per-instance config additions + `tests/render-harness/main.cpp` new CLI flags + new golden text files (per-string sustained-tone JSON+sha256, detune-sweep JSON+sha256, note-sequence JSON+sha256) + planning artefacts (CONTEXT/RESEARCH/PLAN/SUMMARY/VERIFICATION/STATUS updates) — all in one commit, only on Gate 4 PASS | Same gate-first principle as R7 / R15 / R20. Continues task-numbering sequence: R21 = Phase 2.2 implementation start. User-confirmed. |
| **Q10 — Primary listening DAW** | **Logic Pro (AU)** carry-forward | Same validated workflow used through Phase 2.1a/2.1b/2.1c. Manual smoke after R21+ commit on E1→A1→D2→G2 portamento sweep + ACTIVE_STRINGS knob audit (4 → 3 → 2 → 1 → 4 with note held mid-bow per locked decision: current note keeps ringing, next note-on remaps). User-confirmed. |
| Idle-string topology | All 4 strings always tick; only friction injection gates by `activeStringIndex` | Cold-start on bow re-engagement causes pluck-like attack transient. Existing leak (`−1e-20` constant in bridge filter from F2 carry-forward) keeps idle strings out of denormal territory at zero CPU cost beyond the per-sample `popSample`/`pushSample`/dispersion-cascade. ~1.2 % total idle-string CPU overhead (3 idle × 0.4 %). |
| Crossfade trigger | Only on note-on that maps to a different string than current | DB physical reality — players un-fret up the neck within a string, not across strings, during a note. Pitchbend / portamento mid-note keeps the current string. Simplifies state machine; revisit only if Phase 2.3+ surfaces a need. |
| Phase 2.2 listening test sequence | E1 sustained → A1 sustained → D2 sustained → G2 sustained (each ~3 s) → E1→A1→D2→G2 portamento at 1 s/string → ACTIVE_STRINGS knob sweep 4→3→2→1 with MIDI 50 held → STRING_STIFFNESS sweep on D2 | Covers per-string voicing, switching, ACTIVE_STRINGS demotion, and Phase 2.1c dispersion regression on a non-E string. ~30 s total. |

---

## Open Questions (handed to research-phase)

1. **String-switching trigger detection — exact code path.** `noteStarted()` is called per new note; need to query `getCurrentlyPlayingNote().initialNote`, run mapping → new string index, compare with `activeStringIndex`. If different, set `previousStringIndex = activeStringIndex; activeStringIndex = newIndex; crossfadeRemainingSamples = ceil(0.005 * sampleRateInternal)`. Friction injection routes to `strings[activeStringIndex]` immediately. Research-phase finalises the exact pseudocode + edge cases (rapid re-trigger during crossfade — does it cancel previous crossfade or queue?).
2. **Crossfade math choice — sin/cos vs cubic vs Hann.** Equal-power options:
   - sin/cos: `oldGain = cos(t · π/2)`, `newGain = sin(t · π/2)`, `t ∈ [0, 1]`. Two trig calls per sample × 4 strings = 8 trig per sample during crossfade — affordable but not free.
   - Lookup table: precompute 256-entry sin/cos at prepare(); read with `t * 256` index. Zero per-sample trig.
   - Linear: `oldGain = 1 - t`, `newGain = t`. Not equal-power but lossless during crossfade if old string is also fully decaying. Simplest.
   - Recommend: 256-entry LUT (matches the 1024-entry friction LUT pattern from `modules/synthesis/bow-friction/`). Research-phase finalises LUT size + interpolation.
3. **Per-string dispersion stiffness smoother — shared or per-string?** Current architecture has one `STRING_STIFFNESS` parameter that affects all strings (B prefactor differs per string but stiffness multiplier is global). Phase 2.1c has one `stiffnessSmoothed` in the voice. Phase 2.2 question: does each string get its own smoother (so per-string `a` updates independently as the smoother advances), or do all strings share one smoother and recompute their respective `a` from the same smoothed value? Recommend (b) shared — one smoother, four `computeAllpassCoefficient(f0_per_string, B_per_string, M_per_string)` calls per block. Research-phase confirms.
4. **Per-string `WaveguideString::prepare()` config surface.** Current API: `prepare(spec)`. Phase 2.1c adds `setActiveSections(M)` and `setCoefficient(a)`. Phase 2.2 adds: per-instance `f0` (the open-string frequency for delay-length seeding). Options:
   - Pass `openStringFrequency` to `prepare(spec, f0)` overload.
   - Keep `prepare(spec)` as-is and let voice call `setOpenStringFrequency(f0)` separately at prepare-time, before first render.
   - Recommend (b) — keeps `prepare()` signature stable across plugins (O-Bowed shares this class? — check; if so, additive setter is safer).
5. **MIDI-note → finger-position frequency.** For a MIDI note that maps to (e.g.) A1 string but is actually MIDI 40 (E2), the string is fingered up 7 semitones. The waveguide frequency is the actual MIDI-note frequency (E2 = 82.4 Hz), not the open-string frequency. So per-string `currentFrequency` is just `MidiMessage::getMidiNoteInHertz(midiNote)`. Detune (cents) modulates the delay-line length on top. Research-phase confirms there's no string-specific frequency offset (e.g., does string-tension-vs-pitch coupling produce a small frequency error at high finger positions? — architecture is silent; treat as 12-TET fingering for v1.0).
6. **Detune-sweep harness mode.** New `--detune-sweep {E,A,D,G}` flag emits WAV + JSON. JSON should include: `mode: "detune-sweep"`, `string: "<X>"`, `detuneRamp: {start: -1200, end: +1200, shape: "linear"}`, `rmsByDecade: [10 deciles]`, plus pass/fail invariant `rmsContinuityRatio ≥ 0.99` (max ratio of adjacent-block RMS values; clicks would spike this). Research-phase finalises the exact JSON schema + the RMS continuity check formula.
7. **Note-sequence harness mode.** New `--note-sequence "MIDI:dur,..."` flag — programmatically schedules note-on/note-off events at sample-accurate offsets. Research-phase confirms: does the existing harness `processBlock` driver support mid-render MIDI buffer injection, or does this need new plumbing? Probably just push `MidiMessage::noteOn/noteOff` into a `MidiBuffer` keyed by sample index, drained per block.
8. **Bit-exact regression at E1 — what exactly stays byte-identical?** Phase 2.1c golden was rendered at: MIDI 28, velocity 0.7, INFINITE_SUSTAIN=1.0, STRING_STIFFNESS=0, sustain=65 s. Phase 2.2 must reproduce this exact preset and get sha256 `d358abcd…`. The 4-string refactor must be additive on the E-string code path: when ACTIVE_STRINGS=4 + MIDI 28 + DETUNE_E=0, the E-string renders identically (other 3 strings idle-tick but contribute zero to mix because their friction injection is gated and their bridge-rail leak is below denormal threshold). Research-phase confirms idle-string contribution is mathematically zero (or below the 24-bit PCM least-significant-bit, which is ~6e-8 in normalized float — well above the leak's 1e-20). Critical question: does the harness output mix include idle strings' near-zero output, and does that perturb the LSB? Recommend: yes-by-design, AND verify via stub test rendering pre/post-Phase-2.2 with all-zero detune + ACTIVE_STRINGS=4. If LSB perturbs, gate 4 invariant (7) needs softening to "RMS-equivalent within 1 LSB" rather than strict bit-exact. Research-phase locks this.

---

## Risks (Phase 2.2-specific)

1. **String-switching click despite 5 ms equal-power crossfade.** Mitigation: the bridge-rail energy on the previously-bowed string takes longer than 5 ms to decay below audibility, so the crossfade window keeps both strings' contributions audible — the equal-power sum is the standard click-free transition. If clicks still appear, escalate to longer crossfade (10–20 ms) or per-sample `a` smoothing on the dispersion coefficient. Research-phase pre-flight: simulate the crossfade math (sin² + cos² = 1) to confirm no amplitude dip. Gate 3 (Phase 2.1c) bit-exact at stiffness=0 carries forward as the unbreakable lower bound.
2. **Idle-string CPU overshoot.** Mitigation: ~0.4 % per idle string × 3 = ~1.2 % overhead estimate; total Phase 2.2 voice CPU projects at ~1.2 + 0.8 (active) = ~2.0 % — well under the 5 % budget. If measured CPU exceeds 3 % on M1, flag for Phase 2.4+ optimization (e.g., gate idle-string dispersion cascade as well, accepting cold-start risk).
3. **Detune sweep clicks at extreme cents (±1200¢).** Mitigation: 20 ms `SmoothedValue<Linear>` in delay-samples space is JUCE-validated. At ±1200¢ on E1 (41.2 Hz → 20.6/82.4 Hz), the delay length doubles or halves — a large absolute change but smooth in samples-space. Per-sample `setDelay()` during ramp is the JUCE pattern. Gate 4 invariant (2) catches any failure (RMS continuity ≥ 99 %).
4. **MIDI-mapping edge cases — notes outside [28, 55].** Mitigation: closed-form thresholds clamp at boundaries (notes < 28 → E string fingered "down" — actually still rendered as MIDI-note-frequency, just below the open-string fundamental, which is musically OK on a detuned-down E string; notes > 55 → G string fingered very high). Gate 4 invariant (4) catches the ACTIVE_STRINGS demotion path; full out-of-range testing is implicit in the listening test sequence.
5. **E1 bit-exact regression failure** — idle strings perturbing the mix LSB. Mitigation: see Open Question #8 — research-phase locks the regression bar (strict bit-exact vs ≤1 LSB tolerance). Architecture says all strings tick; if ticking introduces non-zero contribution at idle, the strict bit-exact bar is geometrically impossible and we soften to RMS-equivalent. This is a research-phase decision, not a discuss-phase decision.
6. **`ACTIVE_STRINGS` mid-sustain edge case.** Mitigation: locked policy = current note keeps ringing on its current string until note-off; next note-on respects ACTIVE_STRINGS clamp. No explicit handler needed if this falls out of the note-on-only switching policy. Research-phase confirms (no surprise corner cases like ACTIVE_STRINGS=0 — parameter is Int 1–4 per parameter-spec.md, so always ≥ 1).
7. **`std::array<WaveguideString, 4>` allocation cost in `prepareToPlay`.** Mitigation: `WaveguideString` already preallocates 8192-sample delay-line buffer in its constructor; 4 instances = ~128 KiB total. Once-per-prepare cost is acceptable. No allocations in `processBlock`.
8. **Phase 2.1c golden dependence on E1-only voice topology.** If the Phase 2.1c regression bar fails because of the topology change alone (independent of detune/dispersion), the regression-bar bit-exact philosophy breaks down for Phase 2.2. Mitigation: research-phase Open Question #8 resolves whether to soften the bar; alternatively, document that "bit-exact at E1" is a Phase-2.1c-internal invariant and Phase 2.2 introduces a one-time refactoring boundary, with a new Phase 2.2 golden captured post-implementation. Latter is acceptable as long as the 8-item Gate 4 bar is otherwise PASS.

---

## Next Phase

Ready for: **research** phase — `/plugin-research O-Contrabass 2-dsp`

Research focus (Phase 2.2):

1. **Resolve Open Questions #1–#7** — pseudocode for string-switching trigger, crossfade LUT size + math, stiffness-smoother sharing, per-string `WaveguideString::prepare()` config surface, MIDI-note frequency derivation, harness JSON schemas.
2. **Resolve Open Question #8 (regression-bar tolerance)** — render pre-flight bit-exact baseline at MIDI 28 + ACTIVE_STRINGS=4 + DETUNE_E=0 + STRING_STIFFNESS=0 BEFORE any Phase 2.2 source edits; capture sha256 (should match Phase 2.1c golden `d358abcd…`). Then derive whether the planned 4-string topology refactor preserves bit-exactness mathematically (idle-string contribution analysis: do `popSample`/`pushSample` on a delay line seeded with all-zeros + leak `−1e-20` produce non-zero output? — analytically derive). Lock the Gate 4 invariant (7) tolerance: strict byte-equal or RMS-equivalent within 1 LSB.
3. **Pattern-confirm against O-Bowed** — does O-Bowed's voice hold one or many `WaveguideString` instances? `BowedStringVoice` likely is single-string per voice with multi-voice polyphony for chords. O-Contrabass is mono with multiple strings inside one voice. Confirm the surface change is O-Contrabass-local (not breaking the shared bow-friction module's API).
4. **Pattern-confirm `--detune-sweep` and `--note-sequence` against existing harness CLI** — confirm the existing CLI parser shape (`--note`, `--velocity`, `--sustain`, `--string-stiffness`, `--stiffness-sweep` already wired). New flags follow same `getopt`-like parsing pattern.
5. **Update RESEARCH.md** — append §15 documenting the resolutions above. (No §12/§13/§14 changes; those are Phase 2.4 follow-up + 2.1b/2.1c history.)

After research: plan-phase (PLAN rev-6) writes R21+ task breakdown verbatim against this CONTEXT + research findings; execute-phase performs the implementation + R21+ atomic commit; verify-phase confirms Gate 4 invariants + Logic AU smoke.

---

## Audit Trail (rev-4 supersedes rev-3)

**rev-1 (earlier 2026-04-26):** Phase 2.1 broad discuss. Cycle scope = Phase 2.1 (sub-phases a/b/c).

**rev-2 (later 2026-04-26):** Phase 2.1a closure (Option A, R7 commit) + Phase 2.1b opening (module extraction, Gate 2). 9 approach decisions, 5 open questions. Phase 2.1b verified 2026-04-27 (R8a `bd5fae0` + R15 `ef0604d` atomic commits, Gate 2 PASS bit-exact).

**rev-3 (2026-04-27):** Phase 2.1c opening — cascaded allpass dispersion (Rauhala/Välimäki 2006), bridge-rail-only on E-string, Gate 3 exit bar. 5 approach decisions, 5 open questions. Phase 2.1c verified 2026-04-27 (R20 atomic commit `5759e5e`, Gate 3 PASS).

**rev-4 (this document, 2026-04-27):** Phase 2.2 opening — 4-string EADG bank + per-string detune ±1200¢ + per-string M=4/3/2/1 dispersion table + MIDI→string mapping + ACTIVE_STRINGS handling + 5 ms string-switching crossfade. 13 approach decisions (Q1–Q10 user-confirmed: single cycle, 4 strings in 1 voice, highest-string-at-or-below-the-note mapping, remap-to-highest-active-string, 5 ms equal-power crossfade, detune in delay-samples space, per-string M-table verbatim from architecture, eight-item Gate 4 bar incl. E1 bit-exact regression, R21+ atomic commit, Logic AU primary; plus three derived: idle-string topology, crossfade trigger, listening test sequence). 8 open questions handed to research-phase: switching pseudocode, crossfade LUT, stiffness smoother sharing, prepare() surface, MIDI frequency derivation, two harness JSON schemas, bit-exact regression tolerance.

**Inherited verbatim from rev-3 (not re-litigated):**
- Split-rail topology (`bridgeDelay` + `neckDelay` per string)
- F2 LP form (`y = g·(1−p)·x + p·y_prev + leak`, drop `g` from feedback)
- F3 in-loop DCB removed (ARCH §"DC Blocker" amendment deferred to end-of-Stage-2 verify)
- F4 `betaScale` fudge removed
- Cascaded allpass dispersion (Rauhala/Välimäki 2006) with closed-form coefficient, bridge-rail-only placement, group-delay compensation in `updateDelayLengths()` (subtract from `bridgeSamples`, not `compensated`)
- Per-plugin `DispersionFilter.h` (NOT extracted to shared module)
- `MaxSections + activeSections` runtime config API
- Per-block coefficient cadence (per-sample `a` modulation reserved as click-fallback)
- Bow-friction module v1.0.0 at `modules/synthesis/bow-friction/` (Phase 2.1b)
- Primary listening DAW: Logic Pro (AU)
- Sample-rate strategy: internal 88.2 / 96 kHz at friction junction
- Atomic-commit gate-first principle (R7 → R15 → R20 → R21+)
- Saturator-tail Phase 2.4 follow-up parking + RESEARCH §12 footnote
- Phase 2.4 calibration polynomial follow-up parking (Risk #7, E1 closed-form clamp)

**New in rev-4:**
- Q1 single-cycle Phase 2.2 (no a/b/c sub-split)
- Q2 single voice with `std::array<WaveguideString, 4>` (NOT 4 voices in MPESynthesiser)
- Q3 closed-form thresholding 28/33/38/43 with ACTIVE_STRINGS clamp
- Q4 remap-to-highest-active-string (silent-fail rejected)
- Q5 5 ms equal-power crossfade at voice mix-bus, note-on-only trigger
- Q8 eight-item Gate 4 bar incl. per-string sustained-tone × 3 + detune-sweep + note-sequence + ACTIVE_STRINGS=1 + auval/pluginval-10 + E1 bit-exact regression (Phase 2.1c golden carry-forward) + Logic AU smoke
- Q9 R21+ Phase 2.2 atomic commit (lands ~6 source files + harness + per-string golden text files + planning artefacts)
- Idle-string topology: all 4 always tick; only friction injection gates by `activeStringIndex`
- String-switching trigger semantics: only on note-on transitions, not pitchbend/portamento
- `tests/render-harness/main.cpp` new CLI flags: `--string {E,A,D,G}`, `--detune-sweep {E,A,D,G}`, `--note-sequence "MIDI:dur,..."`
- Per-string golden text files NOT WAVs (sha256 + JSON only — RESEARCH §14.12 #5 pattern carry-forward from Phase 2.1c)
