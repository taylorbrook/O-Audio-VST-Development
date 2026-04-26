# Spike Wrap-Up Summary

**Date:** 2026-04-23
**Spikes processed:** 3
**Feature areas:** 1 (VST3 Note Expression for Dorico microtonal playback)
**Skill output:** `./.claude/skills/spike-findings-VST-development/`

## Included Spikes

| # | Name | Verdict | Feature Area |
|---|------|---------|--------------|
| 001 | patch-build-load | ✓ VALIDATED | VST3 Note Expression for Dorico |
| 002 | quarter-sharp-end-to-end | ✓ VALIDATED | VST3 Note Expression for Dorico |
| 003 | attack-transient-check | ✓ VALIDATED | VST3 Note Expression for Dorico |

## Excluded Spikes

None.

## Key Findings

### Feasibility
The research direction (VST3 Note Expression via `kTuningTypeID`, locked in `.planning/notes/dorico-microtonal-vst-research.md`) is **viable and ready to ship.** Every identified kill-risk has been retired: the JUCE patch builds cleanly and doesn't break the wrapper, NE events do flow from Dorico to our plugin with sample-accurate timing, the pipeline produces mathematically exact pitch offsets (confirmed at -50.0 semitones for a quarter-sharp, = +50¢ net), and the first sample of a tuned note is at the tuned frequency with no audible zipper.

### Unexpected findings
1. **JUCE patch is smaller than estimated.** Research predicted ~30 LOC in `juce_VST3Common.h`. Actual: 0 lines there, ~40 in the extensions header, ~50 in the wrapper cpp — a cleaner intercept point exists before MIDI conversion.
2. **Dorico represents microtones by neighbor-semitone + NE delta.** A quarter-sharp C4 arrives as `pitch=C#4, NE=-50¢`. Plugins must correlate by `noteId`, not MIDI pitch.
3. **Dorico ignores the NEC handshake.** It trusts the expression map's Microtonality setting, never queries `INoteExpressionController`. The NEC is dead code for this host but kept for other VST3 hosts.
4. **End-user setup is non-trivial.** Default expression maps (NotePerformer / HSSE / HALion) route non-Steinberg VST3s to pitch bend or VST2 detune, not NE. Users must duplicate a map and set Microtonality = "VST3 Note Expression" explicitly. This is UX friction the real build must address — ship a pre-configured `.doricoexpmap` file.

### Dead code to strip before production merge
- All `OLyrica::detail::neTrace(...)` calls in spike files.
- The `detail::neTrace` function itself and `detail::iidToHex` helper.
- `#include <fstream>` in `NoteExpressionSupport.h`.

### Signal for real build
- Extract pattern to a shared Ouaricon module — applies to all pitched plugins (O-Lyrica, O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant).
- Add cross-block `noteId → voice` tracking (Dorico doesn't need it but other hosts might emit mid-note NE).
- Integrate with existing `TuningEngine` (spike bypasses it with a dumb `pow()` multiplier — real build should compose NE deltas with the plugin's tuning system so alternate tunings still work).
- Expand test matrix: quarter-flat, ¾-sharp, ¾-flat, chords with differing per-note inflections.

## Artifacts
- `.planning/spikes/001-patch-build-load/README.md`
- `.planning/spikes/002-quarter-sharp-end-to-end/README.md`
- `.planning/spikes/003-attack-transient-check/README.md`
- `.planning/spikes/MANIFEST.md`
- `.claude/skills/spike-findings-VST-development/` (full skill package)
