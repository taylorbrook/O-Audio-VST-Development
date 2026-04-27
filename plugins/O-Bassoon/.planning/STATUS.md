---
plugin: O-Bassoon
stage: ideation
status: creative_brief_complete
last_updated: 2026-04-27
---

# Resume Point

## Current State: Creative Brief Complete

Creative brief and requirements have been finalized for O-Bassoon. Ready to proceed to Stage 0 planning or UI mockup.

## Completed So Far

**Ideation:** Complete
- Core concept: simple modal-synthesis bassoon for sustained microtonal long tones
- DSP approach: modal synthesis (no O-Reed dependency)
- Range: C1-C6 extended, polyphonic 1-16 voices (default 8)
- Expression: vibrato, breath/dynamics (CC2 + velocity), tone/brightness, attack character
- Microtonal: VST3 Note Expression + MPE pitch-bend (Ouaricon family pattern)
- Requirements extracted with acceptance criteria

## Next Steps

1. Stage 0 planning — research modal synthesis bassoon spectra, lock architecture
2. UI mockup (can run in parallel with Stage 0)
3. Implementation after Stage 0 plan approved

## Context to Preserve

**Key Decisions:**
- Plugin type: Synth (Physical Model Bassoon)
- DSP: Modal synthesis (NOT waveguide, NOT additive, NOT hybrid)
- Explicit non-goal: Do NOT depend on O-Reed (which is broken)
- Microtonal pattern: reuse the validated O-Lyrica VST3 Note Expression spike
- Polyphony: 8-voice default, 16-voice cap

**Files Created:**
- plugins/O-Bassoon/.planning/BRIEF.md
- plugins/O-Bassoon/.planning/REQUIREMENTS.md
- plugins/O-Bassoon/.planning/STATUS.md
