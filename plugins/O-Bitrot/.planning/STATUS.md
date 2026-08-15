---
plugin: O-Bitrot
stage: ideation
status: creative_brief_complete
last_updated: 2026-08-14 12:00:00
---

# Resume Point

## Current State: Creative Brief Complete

Creative brief has been finalized for O-Bitrot. Ready to proceed to Stage 0 planning or UI mockup.

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined (broken-media degradation box, clocked stochastic state machine)
- All six degradation families scoped for v1.0 (tape, CD, vinyl, packet, codec, crush)
- Parameters specified per-module (no macro dice knobs)
- Use cases identified
- Requirements extracted with acceptance criteria

## Next Steps

1. Stage 0 planning (`/plan O-Bitrot`) — recommended; heavy DSP research already exists
2. Create UI mockup (six-panel layout)
3. Start implementation

## Context to Preserve

**Key Decisions:**
- Plugin type: Effect
- Control philosophy: per-module explicit sections, NOT RSBrokenMedia-style probability macros
- Scope: all six families in v1.0
- Clock: tempo-synced default + free-running mode
- Randomness: seeded + reseed button, seed persisted in state (reproducible bounces)

**Research base:** `research/glitch-effects/degradation-dsp-deep-dive.md` (formulas, module decomposition §6, anti-zipper rules §2.5) and `research/glitch-effects/README.md` concept 2.

**License caution:** RSBrokenMedia is GPL-3.0 — patterns only, verify AGPL compatibility before code reuse. Airwindows (MIT) safe to adapt.

**Files Created:**
- plugins/O-Bitrot/.planning/BRIEF.md
- plugins/O-Bitrot/.planning/REQUIREMENTS.md
