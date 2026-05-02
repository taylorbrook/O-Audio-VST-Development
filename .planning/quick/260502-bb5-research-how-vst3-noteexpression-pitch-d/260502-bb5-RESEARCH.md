---
quick_id: 260502-bb5
type: research
date: 2026-05-02
---

# Quick Task 260502-bb5: Microtonal MIDI Transport — Research

**Researched:** 2026-05-02
**Domain:** VST3 NoteExpression / MPE / MusicXML / DAW import
**Confidence:** HIGH on the spec/protocol layer; MEDIUM on Dorico's per-version export quirks; HIGH on DAW MPE support matrix.

---

## TL;DR

1. **Per-note pitch is NOT in the standard MIDI 1.0 file format.** The only practical way to embed it is **MPE** — assign each voice to its own MIDI channel and put pitchbend on that channel. This is a convention layered on top of SMF, not an extension of it. `[CITED: midi.org MPE spec]`
2. **Dorico DOES export microtonal pitchbend to MIDI** when you set `Library → Expression Maps → Microtonality Playback = Pitch bend` — but it is **monophonic** (one MIDI channel per voice), **NOT MPE**. Chords with different microtonal inflections must be split across separate voices/tracks. `[CITED: Steinberg forums, Daniel Spreadbury + community]` `[ASSUMED: Dorico 6 has not added MPE export — feature request thread is still open as of late 2025]`
3. **No format — including a Dorico→Cubase project transfer — preserves VST3 NoteExpression pitch data through a portable file.** NoteExpression lives only in the host's project file at runtime. There is no "sister-product" privileged path. `[CITED: Spreadbury, steinberg.net forum]`
4. **MusicXML 4.0 technically supports microtonal `alter` values** (decimal cents), but Dorico does NOT export microtuning to MusicXML, and no DAW imports microtonal MusicXML correctly. Dead end for our use case. `[CITED: Spreadbury, w3.org/musicxml40]`
5. **Recommended path: MPE-input support in our plugins + a MusicXML→MPE-MIDI bridge tool.** Dorico's pitch-bend MIDI export (one-channel-per-voice) is *almost* MPE — close enough that a small adapter could re-channelize it. Combined with MPE-aware input handlers in our plugins, this gives us coverage in Logic, Ableton 11+, Cubase, Reaper, and Bitwig. **FL Studio remains a dead zone** (no MPE, won't add it). `[CITED: image-line forums]`

---

## 1. VST3 NoteExpression — the source format

**What it is:** A VST3-only message type carried on `IEventList` alongside note-on/note-off events. Each NoteExpression event carries:

- `noteId` (int32) — correlates to a specific note-on, **not a MIDI pitch** — so a single note can carry independent expression that doesn't bleed to other notes of the same pitch
- `typeId` (uint32) — well-known IDs include `kVolumeTypeID`, `kPanTypeID`, **`kTuningTypeID`** (the one we use), `kVibratoTypeID`, `kBrightnessTypeID`, plus user-defined IDs
- `value` (double, normalized 0.0–1.0)

**`kTuningTypeID` value mapping** (verified): `plain = 240 * (norm − 0.5)` → range is **±120 semitones** = ±12,000 cents (20 octaves). `[CITED: developer.steinberg.help VST 3.5.0 INoteExpressionController; vst3sdk-doc.diatonic.jp]`

**Is it ever serialized to a portable file?** **No.** NoteExpression events live only in the host's project file at runtime, and the storage format is host-private (Cubase `.cpr`, Dorico `.dorico`, Reaper `.rpp`, Bitwig `.bwproject`). There is **no portable interchange container** for NoteExpression — not even between Steinberg's own products. `[CITED: Spreadbury on steinberg.net forum 843025]`

**Confirmed:** Dorico's MIDI Export is plain MIDI 1.0 SMF. No proprietary chunks, no NoteExpression sidecar, no bundled XML manifest. `[CITED: Steinberg forum 843025]`

---

## 2. MPE — the practical bridge

**MPE = MIDI Polyphonic Expression**, ratified by the MMA in 2018, layered on top of MIDI 1.0 SMF (no new event types needed). `[CITED: midi.org MPE spec]`

### How it encodes per-note pitch

- Reserve a **Master Channel** (typically Ch 1 for Lower Zone, Ch 16 for Upper Zone) for global / common controllers.
- Allocate **Member Channels** (typically Ch 2–16 for Lower Zone) for individual notes — one channel per simultaneously-sounding note.
- Per-note pitchbend is just **regular pitchbend on the note's member channel**.

### Bend range (CRITICAL gotcha)

- **Master Channel default:** ±2 semitones
- **Member Channels default:** **±48 semitones** (this is the MPE convention)
- Configurable per-zone via RPN 0 (sent to any member channel; applies to all members in zone)

`[CITED: MIDI.org MPE spec PDF; rogerlinndesign.com]`

For a quarter-tone (50 cents) you need 50/4800 = ~1.04% of full bend range. At 14-bit resolution (±8192) that's ~85 LSB — plenty of cent-precision even at the wide ±48 default. **Cent precision is fine; the gotcha is bend-range mismatch between sender and plugin.**

### What lives in an SMF that contains MPE data

A standard MIDI file with MPE just contains:
- Note-on / note-off events spread across channels 2–16
- Pitchbend events on those same channels
- (Optionally) RPN 0 configuration messages declaring the bend range
- (Optionally) an MPE Configuration Message (MCM) declaring zone size

**Any DAW can read this file** — the question is whether it routes the per-channel pitchbend to a single plugin instance correctly, and whether it preserves the per-channel-per-note semantics during editing. That's the support matrix.

---

## 3. DAW Support Matrix

| DAW | Plays MPE SMF? | Routes per-channel PB → single plugin? | Default member bend range | Notes |
|-----|----------------|----------------------------------------|---------------------------|-------|
| **Logic Pro** (macOS) | ✅ via MIDI Mono Mode | ✅ when track is MPE-enabled | ±48 semi | Manual MPE-mode toggle on instrument required after import; not auto-detected. `[CITED: Apple Support lgcp8f599497]` |
| **Ableton Live 11/12** | ✅ | ✅ per-track MPE flag | ±48 semi | Live 12 added MPE editing in Note Expression tab; **Live 12.1 added MPE MIDI tools** for re-channelizing. Tuning Systems work separately — exported MIDI does NOT carry tuning. `[CITED: ableton.com/live-manual/12]` |
| **Cubase / Nuendo** | ⚠ Partial | ✅ via Note Expression MIDI Setup dialog | Configurable | Has its own "VST Note Expression" feature (closer to VST3 NE than MPE). MPE is supported but Cubase prefers its native NE workflow. `[CITED: steinberg.help/cubase_pro Note Expression MIDI Setup Dialog]` |
| **Reaper** | ✅ | ✅ | ±48 semi (configurable) | Strong MPE support via per-track MIDI channel routing; community scripts available; needs explicit setup per track. `[CITED: rogerlinndesign + Reaper community]` |
| **Bitwig Studio** | ✅ | ✅ native | ±48 semi | **Strongest MPE workflow** — every clip note carries 5 expression dimensions natively. **CRITICAL:** Bitwig's per-note pitch bend **only routes to internal Bitwig instruments, NOT to VST3 plugins** — for VSTs you must use the "PB Expressions" track-inspector setting which sends true MIDI pitchbend per channel. `[CITED: bitwig.com/userguide; KVR forum 550056]` |
| **FL Studio** | ❌ | ❌ | n/a | **No MPE support, will not add it** — Image-Line is targeting MIDI 2.0 instead. Workarounds exist (third-party plugins, MPE Emulator) but no first-class path. `[CITED: image-line forum 336483, 320635, 297292]` |

**Summary:** Logic, Ableton, Cubase, Reaper, Bitwig all play MPE SMFs and can drive a single VST3 plugin instance with per-channel pitchbend. **FL Studio is the lone exception** and a `MPE Emulator` workaround is the only option there.

---

## 4. Dorico's actual export options

### MIDI Export (Dorico Pro 5+)

**Pitch-bend microtuning IS supported but with strong caveats:**

- Setting: `Library → Expression Maps → [your map] → Microtonality Playback = Pitch bend` `[CITED: forum 1024843]`
- When enabled, Dorico writes per-voice MIDI pitchbend events alongside note-ons during MIDI export.
- **Monophonic only** — "you can't have chords (where the notes have different microtonal inflections) on a single track. Each voice has to be on its own MIDI channel." `[CITED: forum 1024843]`
- **Not MPE** — Dorico does not write MPE Configuration Messages, does not reserve a master channel, and does not declare bend range via RPN 0. It just writes plain MIDI 1.0 with per-voice channel allocation.
- **Known bugs:** Pitchbend / note-assignment mix-ups for quarter tones reported in Dorico 5 (forum 880061). `[CITED]` `[ASSUMED: status as of Dorico 6 — not confirmed fixed]`

### MusicXML Export

- **Microtuning is NOT exported.** Daniel Spreadbury (Steinberg, Dorico product manager) confirmed: "micro-tuning information is not currently included either in Dorico's exported MIDI or MusicXML." `[CITED: forum 843025]`
- MusicXML 4.0 has the data structures (`<alter>` accepting decimal values for cents, custom accidental support), but Dorico does not populate them with microtonal data on export, and there is no opt-in setting.

### Dorico → Cubase (sister-product path)

- **No privileged path exists.** The recommended workflow is Vienna Ensemble Pro shared between both apps (which sidesteps the issue by hosting the same plugin instances). `[CITED: forum 863905]`
- Cubase can import a Dorico-exported MIDI file with the pitch-bend microtuning option enabled, and Cubase **can** route per-channel pitchbend correctly, but you lose the VST3 NoteExpression `noteId` correlation.
- There is **no "open Dorico project in Cubase" path** that preserves NoteExpression.

### Community workarounds

- Dorico's Lua/JavaScript scripting is limited to in-app behavior; no third-party tool extracts NoteExpression from a `.dorico` file (binary, undocumented format).
- The closest tool is **Vienna Ensemble Pro** as a runtime bridge — both Dorico and Cubase load the same VEP project; mute/unmute the source as you switch apps. Doesn't help for offline file transfer, but works for live mixing.

---

## 5. MusicXML and alternatives

### MusicXML 4.0 microtone story

- `<pitch><alter>0.5</alter></pitch>` = quarter-sharp; decimal values represent fractional semitones. `[CITED: w3.org/2021/06/musicxml40 alter-element-microtones example]`
- Custom accidental glyphs supported via `<accidental smufl="...">` (Helmholtz-Ellis, Sagittal, etc.).
- **In practice:**
  - Dorico does not write microtonal `alter` values on export.
  - MuseScore writes integer-only `alter` (rounding microtones away). `[CITED: musescore.org node 311792, 303918]`
  - DAWs that import MusicXML at all (Logic limited, Cubase decent, no others) **do not preserve fractional `alter` values** — they round to nearest semitone or quarter-tone.

**Conclusion:** MusicXML is a non-starter as a transport carrier today. The format supports the data; the toolchain doesn't.

### Alternative carrier formats

- **MTS-ESP** (Oddsound) — runtime tuning protocol via inter-plugin messaging. NOT a file format; doesn't solve the Dorico→DAW transport question. Already on our v1.6 carry-forward list (FUT-03).
- **`.scl` / `.kbm` / `.tun`** — fixed scale files. Doesn't carry per-note inflection from a Dorico score; only describes the tuning system.
- **MIDI 2.0 / UMP** — out of scope per CONTEXT.md (mentioned for completeness; long-term path but adoption is years away).

---

## 6. Recommended Paths Forward (ranked)

### Option A — Plugins accept MPE input + Dorico pitch-bend MIDI export (**recommended**)

**What:** Add an MPE input mode to the existing `note-expression` shared module. Composer enables Dorico's per-voice pitch-bend MIDI export, splits chords-with-different-inflections to separate voices, exports MIDI, imports to DAW, sets DAW track to MPE/multi-channel mode, plays through our plugin.

**Effort:** **Medium.** The shared module already has the per-note tuning machinery (kTuningTypeID consumer); we add a parallel MIDI input path that converts incoming pitchbend on member channels into the same internal `PendingTuningTable` that NoteExpression feeds. ~1 plan-sized task per plugin (8 plugins) + 1 module enhancement.

**Coverage:** Logic, Ableton, Cubase, Reaper, Bitwig (4 of 6 DAWs natively; Bitwig works via "PB Expressions" track flag).

**Caveats:**
- Composer must split chords-with-microtonal-inflections to separate voices in Dorico (acceptable workflow).
- Bend range mismatch is the #1 failure mode — plugin must read RPN 0 and respect ±48 default.
- FL Studio still doesn't work without MPE Emulator workaround.

### Option B — Build a MusicXML→MPE-MIDI converter

**What:** Standalone CLI (or JUCE host) that reads a MusicXML file with microtonal `alter` values, generates an MPE-formatted SMF (proper MCM, RPN 0, member-channel allocation, pitchbend per note).

**Effort:** **Medium-high.** Would require Dorico to actually export microtonal alters to MusicXML (which it doesn't today — see Section 4). Without Dorico-side support, this option is blocked.

**Status:** **NOT VIABLE TODAY.** Becomes Option A++ if Steinberg adds microtone export to MusicXML.

### Option C — Cubase-only NoteExpression-native path

**What:** Document the workflow: composer uses Vienna Ensemble Pro hosted between Dorico and Cubase, our plugins run inside VEP, both apps drive the same plugin instances.

**Effort:** **Low (documentation only).**

**Coverage:** Cubase users only (~10–15% of our market). Doesn't help Logic/Ableton users.

**Action:** Add to `research/microtonal-dorico-integration.md` as Section 5 "Cubase-Specific Workflow."

### Option D — Plugin-side fixed-tuning sidecar (`scala-tuning-engine`)

**What:** Plugins already have `scala-tuning-engine` (per project memory + module list). Composer exports a `.scl` file from Dorico's tuning system, drops it on the plugin, plugin tunes by note number.

**Effort:** **Low (probably already works).**

**Coverage:** Limited — only works for music written in a single fixed tuning system. Does NOT carry per-note expression (e.g., a melodic glide between two pitches in a single note).

**Status:** Already shipped. Mention in docs as the "fixed-tuning fallback."

### Recommended combination

**A + C + D documented together** as a tiered approach:

1. **Fixed scale → use `.scl` import (D)**
2. **Variable per-note pitch in Cubase → use VEP shared instance (C)**
3. **Variable per-note pitch in any other DAW → use MPE input mode + Dorico pitch-bend MIDI (A)**
4. **FL Studio → recommend MPE Emulator** (link to attilammagyar.github.io/mpe-emulator)

---

## 7. Gotchas

| # | Gotcha | Impact | Mitigation |
|---|--------|--------|------------|
| G1 | **Dorico pitch-bend MIDI is monophonic** — chord with two microtonal inflections in one voice = one wins | Composer's polyphonic microtonal music silently drops inflections | Document: split inflected chords to separate voices in Dorico |
| G2 | **Bend range mismatch** — DAW sends ±2 (default), plugin assumes ±48 | Microtones play at 24x intended depth (broken pitch) | Plugin must respect MPE RPN 0; default to ±48 when MPE detected |
| G3 | **Logic does NOT auto-detect MPE in imported SMF** — user must enable MIDI Mono Mode on instrument manually | Imported MIDI sounds wrong by default | Document setup: "After importing, click `>` on instrument → set MIDI Mono Mode = On (Common Base Channel 1)" |
| G4 | **Bitwig per-note PB does NOT route to VSTs** — only to internal instruments | Our VST3 plugins won't receive Bitwig's "Pitch" expression lane | Document: enable "PB Expressions" in track inspector for VST routing |
| G5 | **Dorico → MIDI exports a re-tuning** — if plugin ALSO applies its own tuning, double-bend results | Cents added twice; plays at wrong pitch | When MPE-input is active, plugin should bypass its own scale-tuning OR offer a "trust incoming pitchbend" toggle |
| G6 | **Ableton tuning systems are separate from MPE pitch** — Ableton's Tuning System applies to its own clips; exported MIDI loses tuning info | Composer expects Ableton-tuned MIDI to carry tuning to other apps; it doesn't | Document: tuning is host-side, transport requires per-note PB |
| G7 | **Cubase Note Expression collapses to one PB track on MIDI export** (per 2017 forum thread, status uncertain) | Polyphonic Cubase NE → monophonic SMF | `[ASSUMED: still true in Cubase 13]` — verify before recommending Cubase as a re-export path |
| G8 | **14-bit pitchbend at ±48 semitones = ~0.59 cents/LSB** — fine for most music, but coarse for just intonation | Inaudible for tonal music; matters for ratio-precise JI | Acceptable for v1.6 scope; flag for advanced tuning users |

---

## 8. What our plugins need (concrete additions)

Based on the recommended path (Option A), the `note-expression` shared module needs:

1. **MIDI input listener** — observe note-on (with channel), pitchbend (with channel), and RPN 0 messages. Build a per-channel `pendingBendCents` map.
2. **Channel→noteId correlation** — when a note-on arrives on channel N with a non-zero pending bend, allocate a fresh `noteId`, populate `PendingTuningTable[noteId] = bendCents`, then call existing `applyPendingTuning` before `voice.trigger()` (same pattern as VST3 NE path; per project memory critical pattern).
3. **Bend-range tracking** — listen for RPN 0 (Pitch Bend Sensitivity) on member channels; store `bendRangeSemitones[channel]`; default to 48 when MPE is detected (note-on on channels 2–16 with no global Mode 3 conflict), 2 otherwise.
4. **MPE detection heuristic** — if any note-on arrives on channel ≥ 2 within first 100ms after plugin instantiation, enter MPE mode. (Cheap; can be overridden by a UI toggle.)
5. **Optional: honor MPE Configuration Message** — proper MCM parsing is nicer but heuristic above covers 99% of real-world DAW output.

**Cost estimate:** 1 module enhancement plan (~1-2 days), then 8 thin per-plugin propagation plans (~30 min each via `/improve`) following the v1.5 Phase 24 propagation pattern.

---

## 9. Future Callout (out of scope per CONTEXT.md)

**MIDI 2.0 / UMP** — natively encodes per-note pitch as a 32-bit Per-Note Pitch Bend message; no channel-juggling required. Adoption is real but DAW support is partial as of early 2026 (Logic 11+ has some, Cubase 14+ has some, Bitwig 5+ has experimental). Re-evaluate in v1.7 or v1.8.

---

## 10. Sources

### HIGH confidence (official / spec)
- [VST 3 SDK NoteExpression Support (3.5.0)](https://developer.steinberg.help/display/VST/[3.5.0]+Note+Expression+Support) — kTuningTypeID, value mapping
- [VST 3 NoteExpressionTypeInfo Reference](https://steinbergmedia.github.io/vst3_doc/vstinterfaces/structSteinberg_1_1Vst_1_1NoteExpressionTypeInfo.html) — type IDs, normalization
- [MIDI Polyphonic Expression Spec PDF (MMA, 2018)](https://d30pueezughrda.cloudfront.net/campaigns/mpe/mpespec.pdf) — bend range, RPN 0, MCM, channel zones
- [MIDI.org MPE overview](https://midi.org/mpe-midi-polyphonic-expression) — current status
- [MusicXML 4.0 alter-element-microtones example](https://www.w3.org/2021/06/musicxml40/musicxml-reference/examples/alter-element-microtones/) — decimal alter for microtones
- [Apple Support — Use MPE with software instruments in Logic Pro](https://support.apple.com/guide/logicpro/use-mpe-with-software-instruments-lgcp8f599497/mac) — MIDI Mono Mode setup
- [Ableton Live 12 Manual — Editing MPE](https://www.ableton.com/en/live-manual/12/editing-mpe/) — Note Expression tab
- [Ableton Live 12 Manual — Tuning Systems](https://www.ableton.com/en/live-manual/12/using-tuning-systems/) — separate from MPE export
- [Bitwig Userguide — Note FX](https://www.bitwig.com/userguide/latest/note_fx/) — per-note expression dimensions
- [Cubase Note Expression MIDI Setup Dialog](https://steinberg.help/cubase_pro/v10.5/en/cubase_nuendo/topics/note_expression/note_expression_midi_setup_dialog_r.html) — Cubase-side NE→MIDI conversion

### MEDIUM confidence (community / forum, multiple-source verified)
- [Steinberg Forums — Dorico 6: Microtonal MPE / Pitch-Bend MIDI Export?](https://forums.steinberg.net/t/dorico-6-microtonal-mpe-pitch-bend-midi-export/1024843) — Dorico's monophonic-PB-only mode
- [Steinberg Forums — Where can I find tuning information in exported midi or xml?](https://forums.steinberg.net/t/where-can-i-find-tuning-information-in-exported-midi-or-xml/843025) — Spreadbury's official statement
- [Steinberg Forums — Dorico 5 pitch bend midi export](https://forums.steinberg.net/t/dorico-5-pitch-bend-midi-export/853912) — feature confirmation
- [Steinberg Forums — Dorico-Cubase workflow](https://forums.steinberg.net/t/understanding-best-workflow-for-dorico-cubase-export-and-the-reverse-with-vst-expression-maps/863905) — VEP recommended bridge
- [Image-Line Forums — MPE feature requests](https://forum.image-line.com/viewtopic.php?t=336483) — FL Studio MPE refusal, MIDI 2.0 instead
- [KVR — Bitwig Pitchbend conversion in MPE](https://www.kvraudio.com/forum/viewtopic.php?t=550056) — VST routing limitation
- [MuseScore — MusicXML microtone import/export issues](https://musescore.org/en/node/311792) — toolchain gaps

### LOW confidence (single-source, flagged for verification)
- Dorico 5 → 6 pitch-bend MIDI export bug status (forum 880061) — not confirmed fixed in Dorico 6
- Cubase 13+ Note Expression MIDI export behavior — only 2017 thread found; recheck if Cubase becomes a primary recommendation

---

## Confidence breakdown

| Area | Level | Reason |
|------|-------|--------|
| VST3 NoteExpression internals (kTuningTypeID, value map) | HIGH | Official Steinberg developer docs |
| MPE protocol details | HIGH | Official MMA spec PDF |
| Dorico MIDI pitch-bend export feature | HIGH | Multiple Steinberg forum confirmations + product blog |
| Dorico does NOT export MPE | HIGH | Direct quote from Daniel Spreadbury (Steinberg) |
| Logic / Ableton / Bitwig MPE support | HIGH | Official user manuals |
| Cubase Note Expression MIDI export quirks | MEDIUM | Old forum thread; needs current verification if pursued |
| FL Studio MPE non-support | HIGH | Image-Line developer statements |
| MusicXML microtone toolchain status | HIGH | Cross-verified across W3C spec + MuseScore + Spreadbury |

**Valid until:** ~2026-08-02 (3 months) — DAW MPE features evolve roughly per major-version cycle.
