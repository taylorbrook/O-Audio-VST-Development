# O-Contrabass Dorico Playback Template Smoke Test

Goal: confirm that after installing the bundled EndpointConfig +
PlaybackTemplateSpec + `.doricolib`, a fresh Dorico project plays a **microtonal
contrabass** part through the plugin at the correct pitch.

Estimated time: 5–8 minutes.

## Prerequisites

- **O-Contrabass installed as VST3** in `~/Library/Audio/Plug-Ins/VST3/`
  (Note Expression is VST3-only — AU will silently fall back to 12-TET).
- The three bundle files copied to the Dorico user library and Dorico
  **fully relaunched** (see `INSTALL-DORICO.md`).
- The O-Contrabass template **applied** to the project
  (`Play → Playback Template… → tick O-Contrabass → Apply and Close`).
  Remember: "Loaded" ≠ "Applied".

---

## TC-4 — 24-EDO quarter-sharp microtonal pitch  ·  **P0 (acceptance gate)**

This is the **only** test that catches a dropped top-level microtonal field. It
is the acceptance criterion for the whole bundle. TC-1..TC-3 can all pass while
this one fails.

1. `File → New` to create an empty project.
2. `Setup → Add Player → Solo` and add a **Contrabass** (or **Double Bass /
   Upright Bass**).
3. Apply the tonality system: `Setup → Tonality System → Equal Temperament 24
   (Quarter-Tone)` (24-EDO).
4. Apply the O-Contrabass playback template (`Play → Playback Template…` → tick
   **O-Contrabass** → **Apply and Close**).
5. On the contrabass stave, write a note in a comfortable arco register
   (e.g. **G2**), then raise it a **quarter-sharp**: input the note, then invoke
   the quarter-sharp accidental until the quarter-sharp glyph appears (≈ 50
   cents above the natural).
6. Hit **Play**.

**PASS:** the note sounds a **quarter-tone (≈ 50 cents) above** the natural
pitch — a distinct, in-between-the-keys pitch. The quarter-sharp is audibly
higher than the plain note but lower than a semitone up.

**FAIL:** the note plays at the **nearest 12-TET** pitch (identical to the plain
natural, no quarter-tone inflection). Dorico has silently dropped the microtonal
info. Fix, in priority order:

1. **Top-level `<pitchBendRange>2</pitchBendRange>` +
   `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
   present in the `<ExpressionMapDefinition>`** of the installed
   `O-Contrabass.doricolib`, placed between `<applyStageTemplateSettings>` and
   `<initSwitchData>`. **These are LOAD-BEARING.** The per-combination copies do
   NOT substitute. If a "cleanup" removed them, restore them and bump
   `<version>`.
2. **Dorico fully relaunched** after editing the `.doricolib`
   (`DefaultLibraryAdditions/` loads at app startup, not project open). Verify:
   `grep "Loading Extra Library" "$HOME/Library/Application Support/Steinberg/Dorico 6/application.log" | tail`.
3. **24-EDO tonality system actually applied** to the project.
4. **Plugin loaded as VST3, not AU** — check the Mixer slot; confirm the
   `<pluginID>` in the endpoint config matches the installed VST3 (dev vs
   release, see `INSTALL-DORICO.md`).

---

## TC-1 — Template appears in the picker  ·  P0 (gates everything)

1. `File → New`.
2. `Play → Playback Template…`.

**PASS:** **O-Contrabass** appears in the list with creator "Ouaricon Audio".
**FAIL:** check `application.log` for `PlaybackTemplateSpec` ingest errors, and
confirm all three files copied + Dorico relaunched (see `INSTALL-DORICO.md`).

---

## TC-2 — Bass stave routes to O-Contrabass and sounds  ·  P1

1. In the TC-4 project (Contrabass added, template applied), open the Mixer
   (Play mode, `M`).
2. Confirm the contrabass stave's instance is loaded and named
   **O-Contrabass** (or **O-Contrabass-dev**).
3. In `application.log`, confirm there is **no**
   `Can't find a template spec or endpoint config for routing this instrument`
   line for the contrabass.
4. Write a plain arco natural note and hit Play.

**PASS:** one O-Contrabass instance loads on the bass stave and sounds.
**FAIL (empty slot / routing warning):** routing is driven by the endpoint
config's `<instruments>` list. Confirm the stave's instrument ID is one of
`instrument.strings.contrabass`, `instrument.strings.contrabass.d`,
`instrument.strings.uprightbass`, and that `<pluginID>` / `<pluginName>` match
the installed binary (dev vs release).

---

## TC-3 — Expression map bound  ·  P1 (prerequisite for TC-4)

1. In the Mixer, click the contrabass slot's "i" (info) icon.
2. Verify the **Expression Map** dropdown shows **"O-Contrabass"**.

**PASS:** the O-Contrabass map is bound.
**FAIL (shows "Default" or blank):** the `<expressionMapID>` in
`endpointconfig.xml` (`xmap.ouaricon.o_contrabass`) does not match the
`<entityID>` in the `.doricolib`. They must be byte-identical.

---

## TC-5 — Dynamics are velocity-fixed (expected behavior, not a bug)  ·  P2

1. On a held arco note, add a `pp → ff` crescendo hairpin.
2. Hit Play.

**EXPECTED:** the note does **not** swell across its duration — the level is
fixed at note-on by velocity. This is the documented v1.0 limitation
(`volumeType = kNoteVelocity`; no CC11 listener). Do **not** file this as a
defect. Continuous within-note dynamics are deferred to v1.1.

---

## Acceptance

- **TC-4 (microtonal pitch)** — **P0.** Must pass. Core value proposition and
  the only check that reveals a dropped top-level microtonal field.
- **TC-1 (template appears)** — P0, gates everything.
- **TC-2 (routing + sound)** / **TC-3 (map bound)** — P1.
- **TC-5 (velocity dynamics)** — P2, informational (documents a known v1.0
  limitation).

Re-run TC-4 after any edit to `Resources/dorico/`.
