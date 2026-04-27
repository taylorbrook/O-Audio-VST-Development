---
title: "O-Bassoon Stage 0 (Ideation / Research-Planning) — Discuss Phase Findings"
created: 2026-04-27
last_verified: 2026-04-27
juce_version: "8.0.4"
summary: "Discuss-phase synthesis for O-Bassoon Stage 0. Captures the architectural decisions, complexity tier (3 / MODERATE depth), risk profile, deferred concerns, and open follow-ups produced during research-planning before code generation begins. Companion to ARCHITECTURE.md (system spec) and ROADMAP.md (phase breakdown)."
domain: workflow
type: guide
keywords:
  - stage-0
  - research-planning
  - bassoon
  - modal-synthesis
  - decisions
  - microtonal
  - juce8
stages: [0]
agents: [research, dsp, build]
---

# O-Bassoon — Stage 0 Context (Discuss Phase)

This document captures the **discuss-phase findings** from research-planning. ARCHITECTURE.md is the system contract; ROADMAP.md is the phased plan; this CONTEXT.md is the *reasoning log* — the "why" behind both.

---

## TL;DR

- **Complexity tier:** 3 (MODERATE research depth)
- **Final score:** 9.0 raw → **5.0 capped**
- **Strategy:** Phased — 4 DSP phases + 2 GUI phases
- **Highest risk:** Bassoon partial-table spectral tuning (Phase 2.2). All other components are LOW-risk / well-precedented.
- **Architecture:** Modal synthesis (parallel biquad bank, 16 modes/voice, 8-voice default polyphony). NO waveguide. NO reed self-oscillation. NO O-Reed dependency.
- **Microtonality:** Reuse shared `note-expression` module v1.1.0 (no inline copy of spike code).
- **GUI:** Mockup deferred — Stage 3 blocked; Stages 1-2 unblocked.

---

## Decisions

### D1: Modal synthesis (NOT waveguide, NOT additive, NOT reed model)

The brief mandated modal synthesis. Reaffirmed because:
- Long-tone use case is exactly what modal banks excel at (no reed feedback complexity, no waveguide oversampling)
- No CPU spent on iterative solvers, oversampling, or stability monitoring
- Per-mode T60 damping is naturally controlled by the `tone` parameter without algorithm changes

**Stage 0 contributed:** the partial-ratio table (near-integer with formant-region weighting at 475 Hz) and the rationale for choosing modal-vs-additive.

### D2: 16 modes/voice (fixed)

Bassoon has ~10-12 audible partials in low/mid register; 16 covers it with headroom. 16 is also a power of 2 (SIMD-friendly later if needed). Single fixed count keeps Stage 2 simple; frequency-adaptive count deferred to v1.1.

### D3: `juce::Synthesiser` (NOT `MPESynthesiser`)

O-Lyrica + O-Wind both use plain `juce::Synthesiser` and successfully integrate Note Expression + MPE pitch-bend in production. `MPESynthesiserVoice` has 6 pure-virtual methods → larger integration cost without v1.0 benefit (we don't need MPE pressure / timbre at v1.0).

### D4: Aftertouch → vibrato deferred to v1.1

The brief flagged this as TBD. Decision: skip for v1.0 to avoid the per-voice aftertouch state complexity in `juce::Synthesiser` (vs. MPESynthesiser). Revisit in v1.1 if MPE expansion is on the table.

### D5: Reuse shared `note-expression` module v1.1.0

Production-validated by O-Lyrica 2.3.0. Spike-diagnostic landmines (`/tmp/olyrica-ne-trace.log` audio-thread file I/O) are stripped. CMake-time JUCE-NE-PATCH marker check is built in. No reason to inline.

### D6: TuningEngine wired headless at v1.0

Voices use `tuningEngine.getFrequency(note)` from day one. Default state is 12-TET A4=440 — functionally identical to a plugin without an engine. Cost: ~50 lines of plumbing. Benefit: v1.1 can ship Scala/TUN UI without architectural change.

### D7: 4-phase DSP staging

Even though most components are LOW-risk individually, the partial-table tuning (Phase 2.2) deserves its own phase with an A/B-vs-recording listening loop. Staging also lets Phase 2.1 ship a shippable basic-tone proof of life before timbre tuning starts.

### D8: 2-phase GUI (no Phase 3.3)

No advanced UI elements at v1.0 (no VU meters, no spectrum displays, no waveform visualizations). Phase 3.3 isn't needed.

---

## Complexity Calculation Notes

The raw 9.0 score (capped to 5.0) reflects:
- 10 parameters → 2.0 (hits the param cap)
- 5 discrete DSP algorithms (Mode Bank, Excitation, ADSR, Vibrato LFO, Output Gain) → 5.0
- 2 features (modulation system, external MIDI control / NE) → 2.0

The "Voice class" and "Voice Manager" are integration glue, NOT discrete DSP algorithms — they don't contribute to the algorithm count. Same for tuning engine / NE module — those are external integrations counted in the features bucket.

Comparable plugins:
- O-Wind (waveguide flute): score in this same range. Confirmed by checking O-Wind ROADMAP — 3+ DSP phases.
- O-Lyrica (waveguide harp + Note Expression): also staged.
- O-Bells (modal synthesis bells): less complex parameter set; would score lower.

---

## Risk Profile

### Highest risk: Bassoon partial table (Phase 2.2)

Quality of bassoon timbre depends on this getting right. Mitigation: dedicated phase with iterative listening against reference recording; 8-mode fallback documented; documentation flags this as "tune by ear" (not "set and forget").

### Secondary risks (all LOW-MEDIUM)

- **CPU at 16-voice polyphony** — could exceed 30%. Documented as known limit; PERF-02 only specifies 8-voice.
- **Voice stealing edge cases** — pluginval strict catches most. Stuck notes are the canonical failure.
- **JUCE-NE-PATCH absence post-upgrade** — caught by CMake-time marker check (built into note-expression module).
- **Dorico end-user expression-map setup** — documentation issue, not implementation. Module ships .doricolib + Playback Template.

### Risks explicitly designed out

- Feedback loops: NONE (modal synth is feed-forward by design)
- Oversampling: NOT REQUIRED (no nonlinearities in resonator path)
- File I/O: NONE at v1.0 (deferred to v1.1)
- Multi-output routing: NOT REQUIRED (single stereo output)

---

## Cross-Plugin Pattern Reuse

| Pattern | Source | What we copy |
|---------|--------|-------------|
| Voice class structure | O-Lyrica `HarpSynthVoice` | Member layout: APVTS pointer, pendingTuningSource pointer, tuningEngine pointer, JUCE ADSR member |
| NE drain at top of processBlock | O-Lyrica `PluginProcessor::processBlock` | Identical pattern — drain returns deltas, then `synth.renderNextBlock()` |
| `applyPendingTuning` in startNote | O-Lyrica `HarpSynthVoice::startNote` | Direct copy: compute base freq → apply NE delta via `exchange` → use composite freq for DSP |
| Voice ownership / setup | O-Wind `FluteSynthVoice` | Pre-allocate N voices in `prepareToPlay`, register single Sound, pass shared pointers via setter |
| Mode bank custom biquad struct | O-Formant filter banks | 32-byte biquad struct, ProcessSpec/AudioBlock NOT used at this scale |
| 3-arg WebSliderParameterAttachment | All recent WebView plugins | Always pass `nullptr` undoManager (JUCE 8 requirement) |
| TuningEngine integration | O-Lyrica / O-Wind | TuningEngine is in PluginProcessor; voices hold raw pointer; `getFrequency(note)` for base freq |

---

## Deferred to v1.1+ (Out of Scope for v1.0)

- Aftertouch / channel pressure → vibrato_depth modulation
- Scala / TUN file UI (engine wired in, UI not exposed)
- Multiple register-specific partial tables (low / high)
- Frequency-adaptive mode count (16 for low, 8 for high)
- Analysis-resynthesis-derived partial table (from real bassoon recording)
- Preset browser
- Optional key-click / multiphonics articulation layer
- MPESynthesiserVoice migration (only if MPE pressure becomes a v1.1 requirement)
- SIMD biquad evaluation
- Per-voice dropping of inaudible upper modes (CPU optimization)

---

## Open Follow-ups Before Stage 1

1. **UI mockup needed for Stage 3** (Stages 1-2 are unblocked).
2. **Reference bassoon recording for Phase 2.2 listening loop** — identify a royalty-free C3 sustained recording before Phase 2.2 starts.
3. **Verify `Ouaricon::TuningEngine` API signature** — exact namespace / header path may differ from O-Wind's. Confirm via `ls modules/tuning/scala-tuning-engine/cpp/` before Stage 1.
4. **Module dependency declaration in CMake** — confirm the canonical CMake function name (`ouaricon_link_modules` vs. direct `target_link_libraries(... Ouaricon::note_expression)`) by inspecting another consumer's CMakeLists.txt during Stage 1.

---

## What Stage 0 explicitly did NOT decide

- **Final partial-ratio numerical values** — the table in ARCHITECTURE.md is a starting point, not a contract. Phase 2.2 will tune these by ear.
- **First-formant frequency** — 475 Hz is the central hypothesis from research; final value is a Phase 2.2 listening-loop output.
- **Vibrato sine vs. triangle vs. random LFO shape** — Stage 0 selects sine (matches real bassoon vibrato character); revisit only if Phase 2.3 listening reveals issues.
- **MIDI program change handling** — out of scope for v1.0 (no factory presets selectable via MIDI).

---

## Family Lineage

O-Bassoon enters the Ouaricon physical-model family as the third sustained-tone synth (after O-Bowed, O-Wind) but is the **first to use modal synthesis** rather than waveguide. This is intentional: it expands the family's algorithmic vocabulary and provides a clean reference implementation for future modal-synth plugins (e.g., a hypothetical O-Marimba-Sustained or O-VibratingPlate).

The "Ouaricon family visual language" UI direction means the Stage 3 mockup pass should align with O-Wind / O-Lyrica's existing UI conventions. Same parameter knob style, same color palette, same overall layout density.
