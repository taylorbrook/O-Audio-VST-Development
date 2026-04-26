# Spike Manifest

## Idea

Implement VST3 Note Expression `kTuningTypeID` reception on O-Lyrica so Dorico's tonality system (custom accidentals, tuning deltas in tenths-of-cents) drives per-note microtonal playback. Requires a local JUCE patch because JUCE 8.0.4 silently drops `kNoteExpressionValueEvent` in its VST3 wrapper. Research: `.planning/notes/dorico-microtonal-vst-research.md`.

## Spikes

| # | Name | Validates | Verdict | Tags |
|---|------|-----------|---------|------|
| 001 | patch-build-load | Given a patched JUCE + O-Lyrica NEC wiring + voice-side tuning hook, when built on macOS, then VST3 builds, pluginval passes, plugin loads in Dorico without crash | ✓ VALIDATED | juce-patch, vst3, note-expression, build |
| 002 | quarter-sharp-end-to-end | Given a Dorico quarter-sharp accidental, when the note plays through O-Lyrica, then emitted pitch is +50¢ ±5¢ above 12-TET reference | ✓ VALIDATED | dorico, microtonal, end-to-end |
| 003 | attack-transient-check | Given a tuned note at block boundary with NE at sampleOffset 0, when voice starts, then first output sample is at tuned frequency (no pitch glide/zipper) | ✓ VALIDATED | dsp, transients, refinement |
