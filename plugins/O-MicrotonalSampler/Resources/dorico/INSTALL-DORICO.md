# Installing the O-MicrotonalSampler Dorico Playback Template

This bundle ships expression maps and endpoint configs for **four instrument
families** (Strings, Winds, Brass, Generic fallback) in a single Playback
Template. Dorico routes each stave to the appropriate family map automatically
based on the stave's instrument family.

## Distribution mechanism (validated against Dorico 6)

Standalone `.doricoexpmap` files are **not** auto-ingested by Dorico's
Library scanner (silently skipped — verified the hard way in 2026-04-26).
Library Manager's "Import" only accepts `.dorico` project files, not
`.doricolib` files directly.

The validated distribution path is the **multi-folder layout** Dorico itself
uses for user-saved templates, plus `DefaultLibraryAdditions/` for global
expression-map registration:

| Folder | Purpose |
|---|---|
| `EndpointConfigs/O-MicrotonalSampler/` | Strings endpoint config + shared `.doricolib` (4 exp-maps) |
| `EndpointConfigs/O-MicrotonalSampler-Winds/` | Winds endpoint config (binds Winds exp-map) |
| `EndpointConfigs/O-MicrotonalSampler-Brass/` | Brass endpoint config (binds Brass exp-map) |
| `EndpointConfigs/O-MicrotonalSampler-Generic/` | Generic fallback endpoint config |
| `PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml` | User-facing Playback Template with per-family routing entries |
| `DefaultLibraryAdditions/O-MicrotonalSampler.doricolib` | **Auto-merged into every project's library on Dorico launch.** Contains all 4 expression maps so they appear in the Track Inspector → Expression Map dropdown. |

## Install steps

### macOS

1. **Quit Dorico** (5 or 6 — both work; the bundle is `fileVersion 1.1416`-compatible).

2. Copy the bundle into your Dorico user library:

   ```sh
   # For Dorico 6 — copy all 4 endpoint config folders + the playback template:
   cp -R Resources/dorico/EndpointConfigs/O-MicrotonalSampler \
         Resources/dorico/EndpointConfigs/O-MicrotonalSampler-Winds \
         Resources/dorico/EndpointConfigs/O-MicrotonalSampler-Brass \
         Resources/dorico/EndpointConfigs/O-MicrotonalSampler-Generic \
       ~/Library/Application\ Support/Steinberg/Dorico\ 6/EndpointConfigs/

   cp -R Resources/dorico/PlaybackTemplateSpecs/O-MicrotonalSampler \
       ~/Library/Application\ Support/Steinberg/Dorico\ 6/PlaybackTemplateSpecs/

   mkdir -p ~/Library/Application\ Support/Steinberg/Dorico\ 6/DefaultLibraryAdditions
   cp Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib \
       ~/Library/Application\ Support/Steinberg/Dorico\ 6/DefaultLibraryAdditions/O-MicrotonalSampler.doricolib
   ```

   (Replace `Dorico 6` with `Dorico 5` if you're on Dorico 5.)

3. **Launch Dorico.** On launch, Dorico auto-merges every `.doricolib` in
   `DefaultLibraryAdditions/` into every project's library and loads the
   EndpointConfigs + PlaybackTemplateSpec from the other folders.

4. Verify ingest by tailing the log:
   ```
   ~/Library/Application Support/Steinberg/Dorico 6/application.log
   ```
   Look for these lines (in any order):
   ```
   Loading Extra Library: O-MicrotonalSampler
   Loading PlaybackTemplateSpec: O-MicrotonalSampler
   Loading Endpoint Config: O-MicrotonalSampler — Strings
   Loading Endpoint Config: O-MicrotonalSampler — Winds
   Loading Endpoint Config: O-MicrotonalSampler — Brass
   Loading Endpoint Config: O-MicrotonalSampler — Generic
   ```
   followed by `Loading Extra Library...done`. If you see `Error opening file:
   invalid file format`, the file's structure is wrong — see Troubleshooting.

5. Open or create a project. In Play mode, Track Inspector → Expression Map
   dropdown should now list four entries: **"O-MicrotonalSampler — Strings"**,
   **"— Winds"**, **"— Brass"**, **"— Generic"**.

### Windows

Equivalent paths under `%APPDATA%\Steinberg\Dorico 6\`:

```
EndpointConfigs\O-MicrotonalSampler\
EndpointConfigs\O-MicrotonalSampler-Winds\
EndpointConfigs\O-MicrotonalSampler-Brass\
EndpointConfigs\O-MicrotonalSampler-Generic\
PlaybackTemplateSpecs\O-MicrotonalSampler\
DefaultLibraryAdditions\O-MicrotonalSampler.doricolib
```

Copy the four `EndpointConfigs/` subfolders, the `PlaybackTemplateSpecs/`
subfolder, and the single `.doricolib` into those paths.

## Multi-family routing

Routing in Dorico playback templates is driven by an `<instruments
array="true">` block inside each endpoint config. Each `<instrumentData>`
declares an `<entityID>` (canonical instrument ID like
`instrument.strings.violin`) — when applying the template, Dorico picks
the endpoint config whose `<instruments>` block contains the stave's
instrument and instantiates a fresh plugin from that endpoint's slot
template for that stave.

The four endpoint configs enumerate instrument IDs from the
authoritative source
`/Applications/Dorico 6.app/Contents/Resources/instrumentFamiliesDefinitions.xml`:

| Endpoint Config | Family / Coverage | Instrument count | Expression Map | Articulation slots (0..7) |
|---|---|---:|---|---|
| O-MicrotonalSampler | `instrument family.strings` (`instrument.strings.*`) | 19 | Strings | ord, sp, st, **stacc**, cs, pizz, harm, **trem** |
| O-MicrotonalSampler-Winds | `instrument family.woodwinds` (`instrument.wind.*`) | 84 | Winds | ord, flutter, breathy (whisper), multi, keyclick, slap, harm, stacc |
| O-MicrotonalSampler-Brass | `instrument family.brass` (`instrument.brass.*`) | 100 | Brass | ord, mute, cuivre, flutter, **(unbound)**, stopped, growl, fall |
| O-MicrotonalSampler-Generic | pitched-perc, unpitched-perc, keyboards, singers, fretted, orff, electronics, gamelan, sketch | 345 | Generic | ord, **(unbound × 7)** — bind manually in Library → Expression Maps |

Note: `<instrumentFamilies>` in `playbacktemplatespec.xml` is a vestigial
field that Dorico parses but doesn't consult for routing (verified in
v1.16.4 by isolating that change and observing no behavior delta). The
spec keeps the textual `instrument family.X` IDs there as a forward-compat
hedge in case a future Dorico version starts honoring them.

Notes on the `(unbound)` cells:
- **Brass slot 4 (halfvalve):** Dorico's playing-technique catalog has no
  canonical `pt.halfValve` entry. Slot 4 is reserved for half-valve sample
  content but ships unbound. Manually bind via Library → Expression Maps if
  you have a notation that should fire it.
- **Generic slots 1..7:** intentionally unbound so users can map per-instrument
  techniques (percussion, voice, keyboard, etc.) without conflicting with the
  family defaults.

## Articulations dropped from the v1.16.2 Strings map

These notation triggers no longer fire keyswitches and fall back to ord:

- **Senza vib.** (`pt.nonVibrato`) — was slot 3
- **Mart.** (`pt.martele`) — was slot 7
- **Flaut.** (`pt.flautando`) — was slot 9 (no longer in the 8-slot cap)

If you have existing v1.16.2 scores using these markings, the audible
result will be the previously held articulation continuing through the
marked region. Replace with `Stacc.`, `Trem.`, or `Ord.` as needed.

## Using the template

After install + Dorico launch, two paths to put the plugin into your project:

### A. Apply the Playback Template (auto-load with family routing)

1. `Play → Playback Template…`
2. Pick **"O-MicrotonalSampler"** from the list. Click Apply.
3. Each stave's instrument family routes to the appropriate plugin instance
   with the matching exp-map already bound. No manual load step.

### B. Manual VST load + manual exp-map binding

1. In Play mode, open the Mixer (`M`).
2. On any instrument's slot, click the load dropdown and pick
   **VST Instruments → O-MicrotonalSampler-dev** (or `O-MicrotonalSampler`
   for release builds).
3. In Track Inspector → Expression Map, set to whichever family map fits
   the instrument (Strings / Winds / Brass / Generic).
4. Microtonal accidentals (e.g. quarter-sharps in 24-EDO) play at correct
   pitch via VST3 Note Expression.

## Caveat: dev vs release builds

The bundled `endpointconfig.xml` files reference the VST3 plugin ID for the
**dev-branded** binary (`O-MicrotonalSampler-dev`, manufacturer code `OuDv`):
`ABCDEF019182FAEB4F7544764F4D7453`.

Release builds use a different manufacturer code (`OuAu`) which produces a
different VST3 plugin ID. If you installed a release-branded
O-MicrotonalSampler (no `-dev` suffix), the templates will load but the
plugin slots will be empty. Edit each of the four installed `endpointconfig.xml`
files in `~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/`
and replace `<pluginName>` and `<pluginID>` with the release values
(find the release CID in the installed bundle's
`Contents/Resources/moduleinfo.json`, the 32-char `CID` of the
"Audio Module Class" entry).

Release-CI may eventually ship a parallel artifact tree under
`Resources/dorico/release/` with the release-branded IDs pre-baked.

## Troubleshooting

### Dorico crashes on launch with "Error opening file: invalid file format"

A `.doricolib` or XML file in `DefaultLibraryAdditions/`, `EndpointConfigs/`,
or `PlaybackTemplateSpecs/` has a leading XML comment before the root
element. Dorico's strict parser rejects these. Either remove the comment
or remove the file from the user library.

### Expression maps not in Track Inspector dropdown

Dorico didn't auto-merge the `.doricolib` from `DefaultLibraryAdditions/`.
Either the folder doesn't exist (create it), the file isn't named with a
`.doricolib` extension (rename), the XML is malformed (run
`xmllint --noout file.doricolib`), or Dorico cached an older parse and
needs the cache cleared:

```sh
rm -f "/Users/taylorbrook/Library/Caches/Dorico 6/cachedFileDataProvider/O-MicrotonalSampler-"*.dtn \
      "/Users/taylorbrook/Library/Caches/Dorico 6/cachedFileDataProvider/cachesummary.xml"
```

Bumping `<version>N</version>` in the doricolib (currently `<version>8</version>`
for Strings) forces a re-merge.

### After updating the bundled `.doricolib`, Dorico still uses the old version

Same caching issue. Bump `<version>` AND clear the cache (above).
Restart Dorico. New projects see the updated exp-maps; existing projects
may have the old definitions embedded in their project library and need
either Library Manager intervention or recreation.

### Wrong family routing (Trumpet binds Strings map, etc.) or "Can't find a template spec…" warning on apply

Routing is driven by `<instruments array="true">` at the endpoint-config
level — each `<instrumentData>` declares an `<entityID>` like
`instrument.strings.violin` that the endpoint config handles. v1.16.5
ships endpoint configs with full enumeration:

- Strings endpoint: 19 IDs from `instrument.strings.*`
- Brass endpoint: 100 IDs from `instrument.brass.*`
- Winds endpoint: 84 IDs from `instrument.wind.*`
- Generic endpoint: 345 IDs (everything not in the above three families)

History: v1.16.3 left endpoint configs with no `<instruments>` block,
which is what the "Can't find a template spec…" warning reported.
v1.16.4 fixed the spec-level `<instrumentFamilies>` text (kStrings →
instrument family.strings) — harmless but routing was still broken.
v1.16.5 added the endpoint-config enumeration that actually drives
matching.

If `application.log` still shows `Can't find a template spec or endpoint
config for routing this instrument` after applying the v1.16.5 template:

1. **Verify endpoint configs were re-copied.** Each of the four
   `endpointconfig.xml` files in
   `~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/O-MicrotonalSampler*/`
   should end with `</instruments>` followed by `</endpointConfig>` and
   contain dozens of `<entityID>instrument.*</entityID>` lines.
2. **Cache clear.**
   ```sh
   rm -f "/Users/taylorbrook/Library/Caches/Dorico 6/cachedFileDataProvider/cachesummary.xml" \
         "/Users/taylorbrook/Library/Caches/Dorico 6/cachedFileDataProvider/O-MicrotonalSampler-"*.dtn
   ```
   Quit Dorico, relaunch.
3. **Look for the stave's instrument ID** in the appropriate endpoint
   config. Find Dorico's assigned ID via the score: select the stave →
   Properties → Instrument; the instrument label maps to one of the
   `instrument.X.Y` IDs in `instrumentFamiliesDefinitions.xml`. If
   missing from our enumeration (rare instrument or new addition in a
   newer Dorico version), add an `<instrumentData>` block manually.
4. **Confirm each `<endpointConfig><configID>` in the spec** matches
   exactly one `<configID>` in one of the four endpoint configs.

See `SMOKE-TEST.md` next to this file for a step-by-step verification
procedure.
