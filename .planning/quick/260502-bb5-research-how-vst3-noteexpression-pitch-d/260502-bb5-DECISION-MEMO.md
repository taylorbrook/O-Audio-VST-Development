---
quick_id: 260502-bb5
type: decision-memo
date: 2026-05-02
status: ready-for-user-decision
---

# Microtonal MIDI Transport — Decision Memo

**Question:** Can per-note pitch ride MIDI from Dorico into a DAW driving our plugins?
**Answer:** Yes — via MPE — with one moderate plugin-side build and one composer-side workflow constraint.

---

## TL;DR

- **Per-note pitch is NOT in plain MIDI 1.0 SMF.** The only practical SMF-compatible carrier is **MPE** (channel-per-note + per-channel pitchbend, ±48 semi default).
- **VST3 NoteExpression never serializes to a portable file** — not even between Steinberg's own products. No Dorico→Cubase privileged path exists. MusicXML is a dead end (Dorico won't write fractional `<alter>`).
- **Dorico DOES export microtonal pitch-bend MIDI** (`Library → Expression Maps → Microtonality Playback = Pitch bend`) — but it is **monophonic** (one channel per voice), **not true MPE**.
- **5 of 6 target DAWs play MPE SMF cleanly** (Logic, Ableton 11+, Cubase, Reaper, Bitwig). **FL Studio is the lone dead zone** — workaround is the [MPE Emulator](https://attilammagyar.github.io/mpe-emulator).
- **Recommended path:** add MPE input to the existing `note-expression` shared module, then propagate to the v1.5 Phase 24 cohort of 8 plugins. Roughly the same cost shape as v1.5's propagation cycle.

---

## Five Key Findings

1. **VST3 NoteExpression is host-private.** Lives only in `.cpr`/`.dorico`/`.rpp`/`.bwproject` runtime; never written to a portable interchange container. Confirmed by Daniel Spreadbury (Steinberg).
2. **MPE is the only practical SMF-compatible per-note pitch transport.** Channel-per-note + pitchbend layered on MIDI 1.0; default member bend range ±48 semitones (configurable via RPN 0).
3. **Dorico exports microtonal pitch-bend MIDI but it is monophonic, not MPE.** No MCM, no master channel, no RPN 0; one channel per voice. Chords with mixed inflections must be split to separate Dorico voices.
4. **MusicXML microtones are a dead end.** Spec supports decimal `<alter>`; Dorico does not export microtuning to MusicXML; DAWs round on import. Blocked on Steinberg.
5. **DAW MPE coverage is strong except FL Studio.** Logic / Ableton 11+ / Cubase / Reaper / Bitwig all route per-channel pitchbend to a single VST3 instance. FL Studio refuses MPE; Image-Line is targeting MIDI 2.0 instead.

---

## DAW Support Summary

| DAW            | MPE SMF playback | Per-channel PB → single VST3 instance | Default member bend range | One-line note |
|----------------|------------------|---------------------------------------|---------------------------|---------------|
| Logic Pro      | Yes              | Yes (when track MPE-enabled)          | ±48 semi                  | Manual MIDI Mono Mode toggle required after import. |
| Ableton 11/12  | Yes              | Yes (per-track MPE flag)              | ±48 semi                  | Live 12.1 added MPE re-channelizing MIDI tools. |
| Cubase/Nuendo  | Partial          | Yes (Note Expression MIDI Setup)      | Configurable              | Prefers its own VST Note Expression workflow. |
| Reaper         | Yes              | Yes (per-track channel routing)       | ±48 semi (configurable)   | Strong support; explicit setup per track. |
| Bitwig Studio  | Yes              | Yes (via "PB Expressions" flag)       | ±48 semi                  | Native per-note PB does NOT route to VSTs by default. |
| FL Studio      | No               | No                                    | n/a                       | Will not add MPE; use MPE Emulator workaround. |

---

## Recommended Path (tiered)

1. **Fixed scale** → use existing `.scl` import (already shipped via `scala-tuning-engine`).
2. **Variable per-note pitch in Cubase** → use Vienna Ensemble Pro shared instance between Dorico and Cubase (docs only).
3. **Variable per-note pitch in any other DAW** → enable Dorico's `Microtonality Playback = Pitch bend` MIDI export, set DAW track to MPE/multi-channel mode, route into our plugins via the **new MPE input mode** added to the `note-expression` shared module.
4. **FL Studio users** → recommend the [MPE Emulator](https://attilammagyar.github.io/mpe-emulator) as the only viable workaround.

---

## Where the v1.6 work would land

- **Module enhancement target:** `modules/tuning/note-expression/` — add a parallel MIDI input path that feeds the existing `PendingTuningTable` consumer. Reuse the dispatch-slot pattern from Phase 23 Plan 05 (`g_neUpdate` / `g_neQuery`) so AU link cleanliness is preserved.
- **Cohort to propagate to:** the same 8 plugins from v1.5 Phase 24 — **O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant**.
- **Cost shape:** 1 module enhancement plan (~1–2 days) + 8 thin per-plugin propagation plans (~30 min each via `/improve`), mirroring v1.5 Phase 24's propagation pattern.
- **Suggested entry point:** `/gsd-discuss-phase` with goal _"Add MPE input to note-expression shared module so DAW-hosted plugins can receive per-note pitch from Dorico-exported MIDI."_

---

## Top 3 Critical Gotchas

1. **Dorico pitch-bend MIDI is monophonic.** Chords with mixed microtonal inflections in a single voice will silently drop inflections — composer must split to separate voices in Dorico.
2. **Bend-range mismatch is the #1 failure mode.** DAW may send ±2 default while plugin assumes ±48 → microtones play at 24× intended depth. Plugin MUST read RPN 0 and default to ±48 when MPE is detected.
3. **Double-bend risk.** When MPE input is active, the plugin must NOT also apply its own scale-tuning on top of incoming pitchbend. Either bypass internal tuning in MPE mode or expose a "trust incoming pitchbend" toggle.

---

## Out of Scope (acknowledged)

- **MIDI 2.0 / UMP** — natively encodes per-note pitch; re-evaluate v1.7+ as DAW adoption matures.
- **MTS-ESP** — runtime tuning protocol, not a file transport; already on v1.6 carry-forward as FUT-03.
- **MusicXML microtone transport** — blocked on Steinberg adding microtone export to Dorico's MusicXML writer.

---

## Sources

Full research with citations: [`260502-bb5-RESEARCH.md`](./260502-bb5-RESEARCH.md). Confidence: HIGH on protocol layer; valid until ~2026-08-02.
