# O-MicrotonalSampler Dorico Playback Template Smoke Test

Goal: confirm that after installing the bundled EndpointConfigs +
PlaybackTemplate, a fresh Dorico project plays microtonal notation
through the plugin with **family-aware** technique-keyswitch routing
wired correctly.

Estimated time: 10–15 minutes (more test cases than v1.16.2's single-family
suite).

## Prerequisites

- O-MicrotonalSampler installed in `~/Library/Audio/Plug-Ins/VST3/`
  (verify: `auval -v aumu OMtS OuDv` returns SUCCESS for AU; `pluginval`
  for VST3 if needed).
- All four EndpointConfig folders + the PlaybackTemplateSpec folder + the
  `.doricolib` copied into the Dorico user library (see `INSTALL-DORICO.md`).
- Dorico restarted after the copy.
- A small sample folder loaded in O-MicrotonalSampler covering at least
  two techniques (e.g. `C4_v1_ord.wav`, `C4_v1_sp.wav`) so technique
  switching is audible.

## Test cases

### TC-1: Template appears in the dropdown

1. Open Dorico.
2. Create a new empty project (`File → New`).
3. `Play → Playback Template…` — verify "O-MicrotonalSampler" appears
   in the list with creator "Ouaricon Audio".

**Pass:** name appears.
**Fail:** check `application.log` for `PlaybackTemplate` ingest errors.

---

### TC-2: Family-aware endpoint loads on apply (fixed in v1.16.5)

1. With the new project open, add four staves: Solo Violin, Flute, Trumpet,
   Marimba (`Setup → Add Player → Solo / Section`).
2. Apply the O-MicrotonalSampler playback template (`Play → Playback
   Template…` → Apply).
3. Open the Mixer (Play mode, `M`).
4. Verify **four separate plugin instances** are loaded, one per stave,
   each named "O-MicrotonalSampler" (or "-dev").
5. In `application.log`, confirm there's NO line reading
   `Can't find a template spec or endpoint config for routing this instrument`.

**Pass:** all four plugin instances load with no warning.

**Fail (slot empty / warning in log):** routing in Dorico is driven by
the `<instruments array="true">` block at the END of each
`endpointconfig.xml`, NOT by `<instrumentFamilies>` in
`playbacktemplatespec.xml` (the spec-level filter is vestigial). v1.16.5
ships endpoint configs with full instrument enumeration:
- Strings endpoint: 19 IDs (`instrument.strings.*`)
- Brass endpoint: 100 IDs (`instrument.brass.*`)
- Winds endpoint: 84 IDs (`instrument.wind.*`)
- Generic endpoint: 345 IDs (pitched-perc, unpitched-perc, keyboards,
  singers, fretted, orff, electronics, gamelan, sketch)

Escalation paths if the warning persists:
1. **Confirm the instrument's entityID is enumerated.** Check
   `application.log` for `In selectFamilyForInstrumentSelection()` lines
   to see what family Dorico assigned the stave; look up that family's
   instrument IDs in
   `/Applications/Dorico 6.app/Contents/Resources/instrumentFamiliesDefinitions.xml`
   and confirm the matching endpoint config lists it.
2. **Verify the four endpoint config dirs were copied.** Per
   INSTALL-DORICO.md, all four must be in
   `~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/`.
3. **Confirm each `<endpointConfig><configID>` in the spec matches a
   `configID` in one of the four endpoint configs.**

---

### TC-3: Per-family expression-map binding

For each of the four staves loaded in TC-2:

1. Click the slot's "i" (info) icon in the Mixer.
2. Verify the Expression Map dropdown shows the family-correct map:
   - Violin → "O-MicrotonalSampler — Strings"
   - Flute → "O-MicrotonalSampler — Winds"
   - Trumpet → "O-MicrotonalSampler — Brass"
   - Marimba (or any non-orchestral instrument) → "O-MicrotonalSampler — Generic"

**Pass:** each stave's exp-map matches its family.
**Fail (all bind Strings or Default):** the `<expressionMapID>` in one of the
new EndpointConfig files doesn't match its corresponding `<entityID>` in
`playbacktemplatedeps.doricolib`.

---

### TC-4: Microtonal pitch playback (load-bearing test, all 4 maps)

For each of the four staves:

1. Apply a quarter-tone-capable tonality system: `Setup → Tonality
   System → 24-EDO`.
2. On the stave, type a quarter-sharp note (e.g. `ce` then up-arrow until
   quarter-sharp accidental appears) at C4.
3. Hit playback. The note should play AT THE CORRECT MICROTONAL PITCH
   (50 cents above C4) — NOT at C4 with a separate pitch-bend message.

**Pass:** audible quarter-sharp tuning on all four staves.
**Fail:** microtonality is silently falling back to pitch-bend on the failing
family. Check that `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
and `<pitchBendRange>2</pitchBendRange>` are present in **all four**
ExpressionMapDefinition blocks in `playbacktemplatedeps.doricolib`.

---

### TC-5a: Strings keyswitch on notation

1. With a multi-technique library loaded in the Violin's plugin instance,
   type a sustained half-note at C4.
2. Above the note, type `sul pont.` (Shift-X to insert text, type the
   string, Esc).
3. Hit playback. The technique-bar in the plugin's WebView UI should
   visibly switch to the "sp" tab when the keyswitch fires.
4. Repeat for each Strings articulation:
   - `Sul tasto` → tab `st` (slot 2)
   - `Stacc.` → tab `stacc` (slot 3) — **NEW in v1.16.3**
   - `Con sord.` → tab `cs` (slot 4)
   - `Pizz.` → tab `pizz` (slot 5)
   - `Harm.` → tab `harm` (slot 6)
   - `Trem.` → tab `trem` (slot 7) — **NEW in v1.16.3**
   - `Ord.` → tab `ord` (slot 0)
5. Type `Senza vib.`, `Mart.`, or `Flaut.` — these should NOT fire any
   keyswitch (Dorico holds the active technique).

**Pass:** every notation in the table switches its tab; the three dropped
notations do not switch and do not produce log warnings.

---

### TC-5b: Winds keyswitch on notation (NEW in v1.16.3)

1. On the Flute stave, with a wind sample library loaded:
2. Type `flutter.` (text expression) above a note → tab `flutter` (slot 1)
3. Type `key click` → tab `keyclick` (slot 4)
4. Type `multi.` → tab `multi` (slot 3)
5. Type `slap` → tab `slap` (slot 5)
6. Type `Ord.` → tab `ord` (slot 0)

**Pass:** wind-specific notations fire the correct slots.
**Note:** "breathy" / "aeolian" notation routes to slot 2 via `pt.whisper`
in v1.16.3 — Dorico's catalog has no canonical `pt.aeolian`, so the closest
semantic match is used. If your score uses a different notation for breathy
playing, manually bind via Library → Expression Maps.

---

### TC-5c: Brass keyswitch on notation (NEW in v1.16.3)

1. On the Trumpet stave:
2. Type `Mute` → tab `mute` (slot 1)
3. Type `Cuivré` → tab `cuivre` (slot 2)
4. Type `flutter.` → tab `flutter` (slot 3)
5. Type `Stopped` → tab `stopped` (slot 5)
6. Type `Growl` → tab `growl` (slot 6) — Dorico ID is `pt.growl` (singular)
7. Type `Fall` → tab `fall` (slot 7) — Dorico ID is `pt.fallDrop`
8. Type `Ord.` → tab `ord` (slot 0)

**Pass:** all listed notations fire the correct slots.
**Note:** "Half valve" (slot 4) is intentionally unbound in v1.16.3 because
Dorico's catalog has no `pt.halfValve` entry. To fire slot 4 from notation,
manually bind via Library → Expression Maps.

---

### TC-5d: Generic fallback (NEW in v1.16.3)

1. On the Marimba stave (or any non-routed instrument):
2. Verify the loaded plugin instance has the "Generic" exp-map bound (per TC-3).
3. Send keyswitch MIDI notes 0..7 (C-2..G-2) directly via a MIDI track
   or external controller. Each note should switch the plugin's WebView UI
   to the corresponding tab (slots 0..7).
4. The Generic map's slots 1..7 ship unbound — typing notation text won't
   fire them by default. This is intentional (customization seed).

**Pass:** keyswitch MIDI input switches tabs even though no notation
triggers are bound.

---

### TC-6: Dynamics path (CC11) on all four maps

1. Add a crescendo hairpin from `pp` to `ff` over a held note on EACH of
   the four staves (Violin, Flute, Trumpet, Marimba).
2. Hit playback. Each held note should swell continuously throughout the
   sustain (NOT change layer abruptly).

**Pass:** continuous swell on all four.
**Fail (stepped layer change on a specific family):** the `<volumeType>` is
`kNoteVelocity` instead of `kCC` / `param1=11` in that family's exp-map.
Check `playbacktemplatedeps.doricolib`.

---

### TC-7: Dropped Strings articulations no longer fire (regression check)

In v1.16.2 the Strings map had 10 slots including `pt.nonVibrato` (sv),
`pt.martele` (mart), and `pt.flautando` (flaut). v1.16.3 trims to 8 slots
and drops these three.

1. On the Violin stave, type `Senza vib.`, `Mart.`, and `Flaut.` markings.
2. Hit playback.

**Pass:** no keyswitch fires for these markings; Dorico holds the previously
active slot. No warning in `application.log`.
**Fail:** if the plugin's WebView technique tab unexpectedly changes or the
log shows "no matching combo", the v1.16.2 doricolib is still being used —
clear cache and restart.

---

## Acceptance

All test cases (TC-1 through TC-7) pass on a fresh Dorico session.
Re-run after each v1.16.x patch that touches `Resources/dorico/`.

**Severity priorities if a test fails:**
- **TC-4 (microtonal pitch)** — P0. Plugin's core value proposition.
- **TC-2 (family-aware endpoint loading)** — P1. The headline TC-2 fix in v1.16.3.
- **TC-5a / TC-5b / TC-5c (per-family keyswitch firing)** — P1. New articulation coverage.
- **TC-3 (correct family map binding)** — P1. Without this, TC-5 tests can't pass.
- **TC-7 (regression of dropped articulations)** — P2. Cosmetic for users with old scores.
- **TC-6 (dynamics)** — P1 across families.
- **TC-1 (template appears)** — P0 (gates everything).
