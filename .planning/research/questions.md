# Open Research Questions

Questions surfaced during exploration that need deeper investigation before acting on.

## Microtonal / Dorico

### Q1: How does Dorico handle microtonal playback when the user hosts the plugin as AU?

**Context:** Our JUCE plugins ship both VST3 and AU on macOS. VST3 Note Expression is VST3-only. Initial research focused on the VST3 path (what we're implementing) and didn't fully pin down AU behavior.

**Specifics to answer:**
- Does Dorico on macOS even emit microtonal data to AU-hosted plugins, or does it route all microtonal content exclusively through VST3?
- If it does emit to AU, which mechanism — AU's own note-param equivalent (`kAudioUnitEvent_ParameterValueChange` at note level?), channel pitch bend, or something else?
- If Dorico just falls back to pitch bend on AU, should we document "for microtonal playback, use the VST3 version" as the official guidance, or is it worth implementing a parallel AU path?

**Why it matters:** Affects plugin documentation, user guidance, and whether we need a parallel AU implementation. Likely a documentation-only fix ("use VST3 for Dorico microtonal"), but worth confirming before we commit.

**Surfaced:** 2026-04-22 during `/gsd-explore` on Dorico microtonal VST communication.
**Related:** `.planning/notes/dorico-microtonal-vst-research.md`
