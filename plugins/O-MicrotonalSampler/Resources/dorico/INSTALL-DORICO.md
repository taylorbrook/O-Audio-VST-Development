# Installing the O-MicrotonalSampler Dorico Playback Template

This bundle ships three files under three folder structures that tell Dorico how
to register the expression map and load O-MicrotonalSampler with the right
microtonal pitch routing pre-wired.

## Distribution mechanism (validated against Dorico 6)

Standalone `.doricoexpmap` files are **not** auto-ingested by Dorico's
Library scanner (silently skipped — verified the hard way in 2026-04-26).
Library Manager's "Import" only accepts `.dorico` project files, not
`.doricolib` files directly.

The validated distribution path is the **3-folder layout** Dorico itself
uses for user-saved templates, plus `DefaultLibraryAdditions/` for global
expression-map registration:

| Folder | Purpose |
|---|---|
| `EndpointConfigs/<Name>/endpointconfig.xml` | VST plugin slot definition + exp-map binding |
| `EndpointConfigs/<Name>/playbacktemplatedeps.doricolib` | Endpoint-scoped exp-map (deps for the endpoint) |
| `PlaybackTemplateSpecs/<Name>/playbacktemplatespec.xml` | User-facing Playback Template (appears in `Play → Playback Template`) |
| `DefaultLibraryAdditions/<Name>.doricolib` | **Auto-merged into every project's library on Dorico launch.** Required for the expression map to appear in the Track Inspector → Expression Map dropdown. |

## Install steps

### macOS

1. **Quit Dorico** (5 or 6 — both work; the bundle is `fileVersion 1.1416`-compatible).

2. Copy the bundle into your Dorico user library:

   ```sh
   # For Dorico 6:
   cp -R Resources/dorico/EndpointConfigs/O-MicrotonalSampler \
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
   EndpointConfig + PlaybackTemplateSpec from the other folders.

4. Verify ingest by tailing the log:
   ```
   ~/Library/Application Support/Steinberg/Dorico 6/application.log
   ```
   Look for these lines (in any order):
   ```
   Loading Extra Library: O-MicrotonalSampler
   Loading PlaybackTemplateSpec: O-MicrotonalSampler
   Loading Endpoint Config: O-MicrotonalSampler
   ```
   followed by `Loading Extra Library...done`. If you see `Error opening file:
   invalid file format`, the file's structure is wrong — see Troubleshooting.

5. Open or create a project. In Play mode, Track Inspector → Expression Map
   dropdown should now list **"O-MicrotonalSampler"** (under user / extra
   libraries).

### Windows

Equivalent paths under `%APPDATA%\Steinberg\Dorico 6\`:

```
EndpointConfigs\O-MicrotonalSampler\
PlaybackTemplateSpecs\O-MicrotonalSampler\
DefaultLibraryAdditions\O-MicrotonalSampler.doricolib
```

Copy the two folders + the single `.doricolib` into those paths.

## Using the template

After install + Dorico launch, two paths to put the plugin into your project:

### A. Manual VST load + manual exp-map binding (recommended for v1.16.x)

1. In Play mode, open the Mixer (`M`).
2. On any instrument's slot, click the load dropdown and pick
   **VST Instruments → O-MicrotonalSampler-dev** (or `O-MicrotonalSampler`
   for release builds).
3. In Track Inspector → Expression Map, set to **"O-MicrotonalSampler"**.
4. Microtonal accidentals (e.g. quarter-sharps in 24-EDO) now play at
   correct pitch via VST3 Note Expression.

### B. Apply the Playback Template (auto-load — currently buggy in v1.16.1)

1. `Play → Playback Template…`
2. Pick **"O-MicrotonalSampler"** from the list. Click Apply.
3. **Known issue (v1.16.1):** the auto-load currently does NOT load the
   plugin slot — Dorico's log reports
   `Can't find a template spec or endpoint config for routing this instrument`.
   You'll still need to manually load the plugin per path A above. Tracked
   for a v1.16.x patch (instrument-claim fields in `playbacktemplatespec.xml`
   need a fallback `<generatorSpec>` entry, modelled on user-saved
   templates like Ample China).

## What works in v1.16.1

| Feature | Status |
|---|---|
| Expression map registers globally (Track Inspector dropdown) | ✓ Works (via `DefaultLibraryAdditions/`) |
| Microtonal pitch via VST3 Note Expression (kVST3NoteExpression) | ✓ Works — quarter-sharp accidentals play at +50¢ correctly |
| Playback Template appears in `Play → Playback Template` dropdown | ✓ Works |
| Dorico launches without crashing | ✓ Works (v1.16.0's pre-root XML comments were the crash cause; v1.16.1 fixes that) |

## Known issues (deferred to v1.16.x patches)

| Feature | Status | Workaround |
|---|---|---|
| Apply Playback Template auto-loads the plugin slot | ✗ Broken | Manually load O-MicrotonalSampler in the Mixer per path A above |
| Typing `pizz.`/`sul pont.`/`Ord.` text fires the keyswitch | ✗ Broken | Use the plugin's WebView UI to switch techniques manually, or send the keyswitch MIDI notes (C-1..A-1 = MIDI 0..9) directly via MIDI track or external controller |

The keyswitch path may be a Dorico exp-map-shape issue (per-combination
fields like `<exclusionGroup>`, `<pitchBendRange>`, `<microtonalPlaybackMethod>`
that match HSO factory exp-maps) or a deeper MIDI-routing matter — actively
under investigation.

## Caveat: dev vs release builds

The bundled `endpointconfig.xml` references the VST3 plugin ID for the
**dev-branded** binary (`O-MicrotonalSampler-dev`, manufacturer code `OuDv`):
`ABCDEF019182FAEB4F7544764F4D7453`.

Release builds use a different manufacturer code (`OuAu`) which produces a
different VST3 plugin ID. If you installed a release-branded
O-MicrotonalSampler (no `-dev` suffix), the template will load but the
plugin slot will be empty. Manual swap path:

1. Open `~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/O-MicrotonalSampler/endpointconfig.xml`
2. Replace `<pluginName>O-MicrotonalSampler-dev</pluginName>` with
   `<pluginName>O-MicrotonalSampler</pluginName>`
3. Replace `<pluginID>ABCDEF019182FAEB4F7544764F4D7453</pluginID>` with
   the release CID. Find it in the installed bundle's
   `Contents/Resources/moduleinfo.json` (the 32-char `CID` of the
   "Audio Module Class" entry).

Release-CI may eventually ship a parallel artifact tree under
`Resources/dorico/release/` with the release-branded IDs pre-baked.
Track via plugin v1.16.x patch series.

## Troubleshooting

### Dorico crashes on launch with "Error opening file: invalid file format"

A `.doricolib` or XML file in `DefaultLibraryAdditions/`, `EndpointConfigs/`,
or `PlaybackTemplateSpecs/` has a leading XML comment before the root
element. Dorico's strict parser rejects these. Either remove the comment
or remove the file from the user library.

### Expression map "O-MicrotonalSampler" not in Track Inspector dropdown

Dorico didn't auto-merge the `.doricolib` from `DefaultLibraryAdditions/`.
Either the folder doesn't exist (create it), the file isn't named with a
`.doricolib` extension (rename), the XML is malformed (run
`xmllint --noout file.doricolib`), or Dorico cached an older parse and
needs the cache cleared:

```sh
rm -f "/Users/taylorbrook/Library/Caches/Dorico 6/cachedFileDataProvider/O-MicrotonalSampler-"*.dtn \
      "/Users/taylorbrook/Library/Caches/Dorico 6/cachedFileDataProvider/cachesummary.xml"
```

Bumping `<version>N</version>` in the doricolib also helps force a re-merge.

### After updating the bundled `.doricolib`, Dorico still uses the old version

Same caching issue. Bump `<version>` AND clear the cache (above).
Restart Dorico. New projects will see the updated exp-map; existing
projects may have the old definition embedded in their project library
and need either Library Manager intervention or recreation.

See `SMOKE-TEST.md` next to this file for a step-by-step verification
procedure.
