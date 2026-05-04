# Installing the O-MicrotonalSampler Dorico Playback Template

This bundle ships three files under two folder structures that tell Dorico how
to load O-MicrotonalSampler with the right expression map, keyswitch routing,
and microtonal pitch handling pre-wired.

## Why three files, not one `.doricolib`?

Standalone `.doricoexpmap` files are **not** auto-ingested by Dorico's
Library scanner (silently skipped — verified the hard way in 2026-04-26).
A single `.doricolib` import via Library Manager only registers the
expression map definition, not the EndpointConfig or PlaybackTemplate
that wire the plugin slot and routing.

The validated distribution mechanism is the **3-folder layout** Dorico
itself uses for user-saved templates: `EndpointConfigs/<Name>/` (with a
nested `playbacktemplatedeps.doricolib`) plus `PlaybackTemplateSpecs/<Name>/`.

## Install steps

### macOS

1. **Quit Dorico** (5 or 6 — both work; the bundle is `fileVersion 1.1416`-compatible).

2. Copy the two folders inside `Resources/dorico/` into your Dorico user library:

   ```sh
   # For Dorico 6:
   cp -R Resources/dorico/EndpointConfigs/O-MicrotonalSampler \
       ~/Library/Application\ Support/Steinberg/Dorico\ 6/EndpointConfigs/

   cp -R Resources/dorico/PlaybackTemplateSpecs/O-MicrotonalSampler \
       ~/Library/Application\ Support/Steinberg/Dorico\ 6/PlaybackTemplateSpecs/
   ```

   (Replace `Dorico 6` with `Dorico 5` if you're on Dorico 5.)

3. **Launch Dorico** and open or create any project.

4. `Play → Playback Template…` — "O-MicrotonalSampler" should now appear in
   the list. Apply it. The plugin loads on slot 1, channel 1, with the
   bundled expression map active.

### Windows

The equivalent paths are:

```
%APPDATA%\Steinberg\Dorico 6\EndpointConfigs\O-MicrotonalSampler\
%APPDATA%\Steinberg\Dorico 6\PlaybackTemplateSpecs\O-MicrotonalSampler\
```

Copy the two folders into those paths.

## Caveat: dev vs release builds

The bundled `endpointconfig.xml` references the VST3 plugin ID for the
**dev-branded** binary (`O-MicrotonalSampler-dev`, manufacturer code `OuDv`).
Release builds use a different manufacturer code (`OuAu`) which produces a
different VST3 plugin ID.

If you installed a **release-branded** O-MicrotonalSampler (no `-dev` suffix),
the template will load but the plugin slot will be empty. Two options:

1. **Manual swap (one-time):** open `endpointconfig.xml`, replace
   `<pluginName>O-MicrotonalSampler-dev</pluginName>` with
   `<pluginName>O-MicrotonalSampler</pluginName>`, and replace the
   `<pluginID>` with the release CID from the installed bundle's
   `Contents/Resources/moduleinfo.json` (the 32-char `CID` of the
   "Audio Module Class" entry).

2. **Future shipping:** the plugin's release CI will eventually ship a
   parallel artifact tree under `Resources/dorico/release/` with the
   release-branded IDs pre-baked. Track via plugin v1.16.x patch series.

## Verifying the install

After applying the template:

- The Mixer (Play mode) shows one slot named "O-MicrotonalSampler".
- The Track Inspector shows "O-MicrotonalSampler" as the Expression Map
  on every staff routed to that slot.
- A staff with a quarter-tone accidental plays at the correct microtonal
  pitch (microtonality is locked to VST3 Note Expression — auto routing
  silently falls back to pitch-bend and breaks playback for non-Steinberg
  VST3s).
- Typing `pizz.`, `sul pont.`, or `senza vib.` over a note triggers the
  matching technique keyswitch (visible in the Play mode Playback Technique
  lane and audible in the plugin's WebView UI when a multi-technique
  library is loaded).

If the template doesn't appear in the dropdown, check the Dorico log at
`~/Library/Application Support/Steinberg/Dorico 6/application.log` —
filter for `EndpointConfig` and `PlaybackTemplate` to see Dorico's
ingest decisions.

See `SMOKE-TEST.md` next to this file for a step-by-step verification
procedure.
