# O-MicrotonalSampler Dorico Playback Template Smoke Test

Goal: confirm that after installing the bundled EndpointConfig +
PlaybackTemplate, a fresh Dorico project plays microtonal notation
through the plugin with technique-keyswitch routing wired correctly.

Estimated time: 5–10 minutes.

## Prerequisites

- O-MicrotonalSampler installed in `~/Library/Audio/Plug-Ins/VST3/`
  (verify: `auval -v aumu OMtS OuDv` returns SUCCESS for AU; `pluginval`
  for VST3 if needed).
- The two folders from `Resources/dorico/` copied into the Dorico user
  library (see `INSTALL-DORICO.md`).
- Dorico restarted after the copy.
- A small sample folder loaded in O-MicrotonalSampler covering at least
  two techniques (e.g. `C4_v1_ord.wav`, `C4_v1_sp.wav`) so technique
  switching is audible. If you don't have a multi-technique library
  handy, single-technique behaviour still verifies microtonal pitch
  routing — the keyswitch test points will simply route to the same
  sample.

## Test cases

### TC-1: Template appears in the dropdown

1. Open Dorico.
2. Create a new empty project (`File → New`).
3. `Play → Playback Template…` — verify "O-MicrotonalSampler" appears
   in the list with creator "Ouaricon Audio".

**Pass:** name appears.
**Fail:** check `application.log` for `PlaybackTemplate` ingest errors
and verify the folder copy paths.

---

### TC-2: Endpoint loads on apply

1. With the new project open, select the O-MicrotonalSampler template
   and click "Apply".
2. Open the Mixer (Play mode, `M`).
3. Verify a slot named "O-MicrotonalSampler" (or "O-MicrotonalSampler-dev"
   for dev builds) is loaded.
4. Verify the plugin's WebView UI opens when the slot's edit button is
   clicked.

**Pass:** plugin loads, UI opens, no error dialogs.
**Fail (slot empty):** likely a dev-vs-release CID mismatch — see
`INSTALL-DORICO.md` § "Caveat: dev vs release builds".

---

### TC-3: Expression map binding

1. In the Mixer, click the slot's "i" (info) icon.
2. Verify the Expression Map dropdown shows "O-MicrotonalSampler"
   (not "Default" / unset).
3. Confirm "Play → Endpoint Setup → Expression Map" reflects the same.

**Pass:** expression map is bound.
**Fail:** verify `<expressionMapID>` in `endpointconfig.xml` matches
`<entityID>` in `playbacktemplatedeps.doricolib`.

---

### TC-4: Microtonal pitch playback (load-bearing test)

1. Add a single staff (e.g. Violin) to the project.
2. Apply a quarter-tone-capable tonality system: `Setup → Tonality
   System → 24-EDO`.
3. Type a quarter-sharp note (e.g. `ce` then up-arrow until quarter-
   sharp accidental appears) at C4.
4. Hit playback. The note should play AT THE CORRECT MICROTONAL PITCH
   (50 cents above C4) — NOT at C4 with a separate pitch-bend message.

**Pass:** audible quarter-sharp tuning. (Verify by loading a tuner
plugin downstream, or by ear if the difference is obvious.)
**Fail:** microtonality is silently falling back to pitch-bend.
Check that `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
is present in `playbacktemplatedeps.doricolib`.

---

### TC-5: Technique keyswitch on notation

1. With a multi-technique library loaded in the plugin, type a sustained
   half-note at C4 in the staff.
2. Above the note, type `sul pont.` (Shift-X to insert text, type the
   string, Esc).
3. Hit playback. The technique-bar in the plugin's WebView UI should
   visibly switch to the "sul pont" tab when the keyswitch fires.
   The audible sample should change accordingly.
4. Repeat for `senza vib.` and `pizz.` text. Each should switch the
   technique tab in the UI.
5. Type `Ord.` to switch back. The "ord" tab should highlight again.

**Pass:** technique tab in plugin UI switches in sync with the playback
position; sample changes audibly.
**Fail (no UI change):** check that the keyswitch range in the plugin
(default C-1 to A-1, MIDI notes 0..9) is enabled and that
`ks_enabled = true` in the plugin's APVTS state.

---

### TC-6: Dynamics path (CC11)

1. Add a crescendo hairpin from `pp` to `ff` over a held note.
2. Hit playback. The note should swell continuously throughout the
   sustain (NOT change layer abruptly at the start).
3. Optional: insert a tuner / level meter downstream and verify smooth
   gain change.

**Pass:** continuous swell.
**Fail (stepped layer change):** the volumeType is `kNoteVelocity`
instead of `kCC` / param1=11. Check `playbacktemplatedeps.doricolib`.

---

## Acceptance

All six test cases pass on a fresh Dorico session. Re-run after each
v1.16.x patch that touches `Resources/dorico/`.

If TC-4 (microtonal pitch) fails, that is a P0 regression — the plugin's
core value proposition. TC-5 / TC-6 failures are P1 (still useful as a
microtonal sampler without technique notation, but degraded).
