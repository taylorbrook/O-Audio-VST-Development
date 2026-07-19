# Installing the O-Contrabass Dorico Playback Template

O-Contrabass is a **sustained-arco solo contrabass** physical model. This
bundle ships a single-family Dorico Playback Template that routes contrabass /
upright-bass staves to the plugin, with **microtonal pitch via VST3 Note
Expression**. It is **NoteExpression-only** — there are no keyswitches (one
natural/arco playing technique).

## Why a Playback Template (not a standalone `.doricoexpmap`)

Standalone `.doricoexpmap` files dropped into Dorico's `User/Expression Maps/`
folder are **silently skipped** by Dorico's library scanner. The validated
distribution path is a Playback Template plus a `.doricolib` merged from
`DefaultLibraryAdditions/`. That is what this bundle provides.

## Install map (macOS, Dorico 6)

Three files, three destinations. **Rename the `.doricolib`** to
`O-Contrabass.doricolib` at the destination (source filename is
`playbacktemplatedeps.doricolib`).

| Source (in this bundle) | Destination |
|---|---|
| `EndpointConfigs/O-Contrabass/playbacktemplatedeps.doricolib` | `~/Library/Application Support/Steinberg/Dorico 6/DefaultLibraryAdditions/O-Contrabass.doricolib` |
| `EndpointConfigs/O-Contrabass/endpointconfig.xml` | `~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/O-Contrabass/endpointconfig.xml` |
| `PlaybackTemplateSpecs/O-Contrabass/playbacktemplatespec.xml` | `~/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/O-Contrabass/playbacktemplatespec.xml` |

(Replace `Dorico 6` with `Dorico 5` if you're on Dorico 5 — the bundle is
`fileVersion 1.1416`-compatible with both.)

## Activation flow — **quit → copy → relaunch → apply**

`DefaultLibraryAdditions/` is read **once at app startup**, so the copy must
happen while Dorico is closed.

1. **Quit Dorico** (Cmd-Q — a full quit, not just closing the project).

2. **Copy the three files** into the destinations above:

   ```sh
   D6="$HOME/Library/Application Support/Steinberg/Dorico 6"

   mkdir -p "$D6/DefaultLibraryAdditions" \
            "$D6/EndpointConfigs/O-Contrabass" \
            "$D6/PlaybackTemplateSpecs/O-Contrabass"

   cp Resources/dorico/EndpointConfigs/O-Contrabass/playbacktemplatedeps.doricolib \
      "$D6/DefaultLibraryAdditions/O-Contrabass.doricolib"

   cp Resources/dorico/EndpointConfigs/O-Contrabass/endpointconfig.xml \
      "$D6/EndpointConfigs/O-Contrabass/endpointconfig.xml"

   cp Resources/dorico/PlaybackTemplateSpecs/O-Contrabass/playbacktemplatespec.xml \
      "$D6/PlaybackTemplateSpecs/O-Contrabass/playbacktemplatespec.xml"
   ```

3. **Relaunch Dorico.** On launch it auto-merges every `.doricolib` in
   `DefaultLibraryAdditions/` into every project's library and loads the
   EndpointConfig + PlaybackTemplateSpec. Confirm ingest by tailing the log:

   ```
   ~/Library/Application Support/Steinberg/Dorico 6/application.log
   ```

   Look for (in any order):

   ```
   Loading Extra Library: O-Contrabass
   Loading PlaybackTemplateSpec: O-Contrabass
   Loading Endpoint Config: O-Contrabass
   ```

   If you see `Error opening file: invalid file format`, an XML file is
   malformed (see Troubleshooting).

4. **Apply the template.** `Play → Playback Template…` → tick **O-Contrabass**
   → **Apply and Close**.

### ⚠ "Loaded" ≠ "Applied"

A fully ingested template still leaves every stave on **NotePerformer** (or
whatever template is currently active) until you explicitly **Apply** it. Seeing
the `Loading …` lines in the log only proves the files were *ingested* — it does
**not** mean any stave is bound to O-Contrabass. Confirm the apply with:

```sh
grep "Applying playback template" \
  "$HOME/Library/Application Support/Steinberg/Dorico 6/application.log" | tail -1
```

The last line must reference `playbacktemplate.user.o_contrabass`. If it says
`…noteperformer…` (or anything else), the template loaded but was never applied
— go back to `Play → Playback Template…` and click **Apply and Close**.

## Routing scope — solo-instrument add-on

Routing is driven by the **endpoint config's `<instruments>` list**
(`instrument.strings.contrabass`, `instrument.strings.contrabass.d`,
`instrument.strings.uprightbass`), **not** by the spec-level
`<instrumentFamilies>` field (which Dorico parses but does not consult for
routing). Only contrabass / upright-bass staves route to O-Contrabass.

Because this is a single-instrument template, **non-bass staves are left
unrouted** when you apply it standalone (they will not sound). For a mixed
score, either:

- assign O-Contrabass per stave via the Mixer (Play mode → Mixer → the bass
  stave's slot → load **O-Contrabass-dev** → set its Expression Map to
  "O-Contrabass"), leaving other staves on their existing template; or
- keep your main template (NotePerformer / HSSE / etc.) applied and add
  O-Contrabass only to the bass endpoint.

## Dynamics caveat (velocity-only, v1.0)

This map uses `volumeType = kNoteVelocity`. O-Contrabass has **no CC11
listener** in v1.0, so dynamics are fixed at note-on by MIDI velocity.
**Sustained crescendos / hairpins within a single bowed note will not render**
mid-note — the level is set when the note starts and holds. Continuous
within-note dynamics are deferred to v1.1 (which would add a CC11 listener; the
map would then switch to `volumeType = kCC` / `param1 = 11`).

## Dev vs release builds — GUID / name swap

This bundle ships **dev-branded** IDs (the locally-built validation binary):

| Field | Dev build (this bundle) | Release build (future) |
|---|---|---|
| Manufacturer code | `OuDv` (Ouaricon Audio Development) | `OuAu` (Ouaricon Audio) |
| `<pluginName>` | `O-Contrabass-dev` | `O-Contrabass` |
| `<pluginID>` | `ABCDEF019182FAEB4F7544764F436273` | **different** — read from the release bundle |

A release build uses a different manufacturer code, which produces a **different
VST3 plugin ID and a different plugin name**. If you install a release-branded
O-Contrabass (no `-dev` suffix), the template will load but the plugin slot will
be **empty** (silent slot). To fix, edit the installed
`~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/O-Contrabass/endpointconfig.xml`
and change exactly two fields:

1. `<pluginName>` → `O-Contrabass`
2. `<pluginID>` → the release Audio-Module CID

Find the release CID in the installed bundle at
`…/O-Contrabass.vst3/Contents/Resources/moduleinfo.json` — the 32-char `CID`
whose `Category` is **"Audio Module Class"** (NOT the "Component Controller
Class" CID). For the dev build that value is
`ABCDEF019182FAEB4F7544764F436273`; the controller CID
`ABCDEF011234ABCD4F7544764F436273` is **not** the one to use.

The bundle intentionally ships dev-branded for validation (matches the
O-MicrotonalSampler precedent). Release-CI may later ship a parallel
`Resources/dorico/release/` tree with release IDs pre-baked.

## Troubleshooting

### Dorico crashes on launch with "Error opening file: invalid file format"

An XML file has a leading comment **before** its root element, or is otherwise
malformed. Dorico's parser is strict — comments must live **inside** the root
element (`<kScoreLibrary>` for the `.doricolib`). Validate with
`xmllint --noout <file>`, or remove the offending file from the user library.

### "O-Contrabass" not in the Playback Template list

Dorico didn't ingest the PlaybackTemplateSpec or `.doricolib`. Check that the
`.doricolib` is named with a `.doricolib` extension in
`DefaultLibraryAdditions/`, that all three files copied to the right paths, and
that Dorico was **fully relaunched** after the copy. Bumping `<version>` in the
`.doricolib` and clearing the cache forces a re-merge:

```sh
rm -f "$HOME/Library/Caches/Dorico 6/cachedFileDataProvider/cachesummary.xml"
```

### Bass stave plays but at nearest 12-TET (no microtonal pitch)

See `SMOKE-TEST.md` TC-4. Almost always the top-level
`<pitchBendRange>2</pitchBendRange>` + `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>`
were removed from the `<ExpressionMapDefinition>`. Restore them, bump
`<version>`, clear the cache, and relaunch. Also confirm the plugin loaded as
**VST3** (Note Expression is VST3-only, not AU) and that the project uses a
24-EDO tonality system.

See `SMOKE-TEST.md` next to this file for the full verification procedure.
