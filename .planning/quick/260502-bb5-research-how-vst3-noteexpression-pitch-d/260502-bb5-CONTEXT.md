---
quick_id: 260502-bb5
type: context
status: ready-for-research
date: 2026-05-02
---

# Quick Task 260502-bb5: Microtonal MIDI Transport Research — Context

**Gathered:** 2026-05-02
**Status:** Ready for research

<domain>
## Task Boundary

We've integrated microtonal playback in Dorico via VST3 NoteExpression (proven in O-Lyrica spike, generalizing to O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant). The VST3 NoteExpression pitch data lives inside Dorico's runtime — when Dorico exports MIDI, that per-note tuning data does NOT come along.

**Question:** How is per-note pitch data conventionally embedded in MIDI files, and can DAWs (Logic, Ableton, Cubase, Reaper, FL, Bitwig) consume it to drive our same plugins outside Dorico?

**Why it matters:** Composers want to author in Dorico but mix/render in a DAW with our plugins. Today the microtonal pitch info is lost at the Dorico→DAW boundary.

</domain>

<decisions>
## Implementation Decisions

### Research goal: All of the above
- Map the standards landscape (what's possible)
- Identify Dorico's export limitations and any workarounds
- Catalog DAW import support for each transport
- Recommend a concrete path forward (what would we need to build/configure)

### Target DAWs (priority order)
- Logic Pro (macOS, primary user base)
- Ableton Live (Live 11+ MPE, Live 12 expanded)
- Cubase / Nuendo (best VST3 NoteExpression support — same vendor as Dorico)
- Reaper, FL Studio, Bitwig (Bitwig has strong MPE support)

### Standards in scope
- **MPE (MIDI 1.0 Polyphonic Expression)** — per-note pitchbend via channel-per-note
- **VST3 NoteExpression internals** — how the data lives inside VST3 events; whether any host serializes it to file
- **MusicXML / alternative formats** — non-MIDI carriers Dorico can export (microtone accidentals, custom export)

### Out of scope (per user)
- MIDI 2.0 / UMP — explicitly excluded (adoption still nascent; focus on what works today)

### Claude's Discretion
- How deep to go on each DAW's specific MPE/microtonal config (provide enough detail to act, not exhaustive tutorials)
- Whether to surface non-Dorico authoring paths (e.g. MPE controllers like LinnStrument, Roli Seaboard) as comparison
- Whether to identify which of our plugins already accept MPE input vs. which would need new input handling

</decisions>

<specifics>
## Specific Ideas

- Dorico is Steinberg's tool — same vendor as Cubase. There may be a Dorico→Cubase bridge that preserves NoteExpression that doesn't exist for other DAWs.
- O-Lyrica is the validated spike/reference (per project memory: `project_o_lyrica_spike_reference.md`). Pattern is being generalized to all pitched plugins via `generalize-microtones` skill.
- The plugins themselves already accept VST3 NoteExpression — the question is purely about transport from a DAW.
- Project memory note: `critical_dorico_distribution_mechanism.md` — Dorico distribution requires Playback Templates, not standalone .doricoexpmap. Suggests Dorico's export ecosystem has its own conventions worth investigating.

</specifics>

<canonical_refs>
## Canonical References

- VST3 SDK NoteExpression docs (Steinberg)
- MIDI Polyphonic Expression spec (MMA, 2018)
- Dorico Pro 5 manual — MIDI Export, MusicXML Export sections
- Each DAW's official MPE / microtonal documentation
- Project spike: `plugins/O-Lyrica/` (validated VST3 NoteExpression pattern)

</canonical_refs>
