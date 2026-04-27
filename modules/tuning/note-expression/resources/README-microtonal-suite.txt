Ouaricon Microtonal Suite — Dorico Integration Resources
=========================================================

PURPOSE
-------
Adds Dorico-aware microtonal playback for the Ouaricon v1.5 cohort via
VST3 Note Expression. Two files ship together:

  • Ouaricon-Microtonal-Suite.dorico_pt
      A Dorico Playback Template archive (zip). Routes Dorico instruments
      to the cohort plugins on per-channel slots and binds the canonical
      expression map to each slot.

  • Ouaricon-VST3-NoteExpression.doricolib
      A standalone Dorico expression-map library bundle. Makes "Ouaricon
      VST3 Note Expression" available in Library → Expression Maps even
      when the Playback Template has not been applied.

Both encode microtonalPlaybackMethod=kVST3NoteExpression, which is the
load-bearing setting for non-Steinberg VST3 microtonal routing. Dorico's
default Auto setting falls back to pitch-bend and silently breaks
microtonal playback.

INSTALL LOCATIONS
-----------------
The plugin installer writes both files to multiple locations.

macOS:
  • Canonical (editable) shared copy:
      ~/Library/Application Support/Ouaricon/Microtonal Suite/
        Ouaricon-Microtonal-Suite.dorico_pt
        Ouaricon-VST3-NoteExpression.doricolib
  • Dorico Playback Template (auto-discovered, extracted from the .dorico_pt):
      ~/Library/Application Support/Steinberg/Dorico [N]/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/
        playbacktemplatespec.xml
      ~/Library/Application Support/Steinberg/Dorico [N]/EndpointConfigs/Ouaricon Microtonal Suite/
        endpointconfig.xml
        playbacktemplatedeps.doricolib
  • Dorico Default Library Additions (auto-merged at startup):
      ~/Library/Application Support/Steinberg/Dorico [N]/Default Library Additions/
        Ouaricon-VST3-NoteExpression.doricolib
      (NOTE: directory name has spaces on macOS.)

Windows:
  • Canonical (editable) shared copy:
      %APPDATA%\Ouaricon\Microtonal Suite\
        Ouaricon-Microtonal-Suite.dorico_pt
        Ouaricon-VST3-NoteExpression.doricolib
  • Dorico Playback Template:
      %APPDATA%\Steinberg\Dorico [N]\PlaybackTemplateSpecs\Ouaricon Microtonal Suite\
      %APPDATA%\Steinberg\Dorico [N]\EndpointConfigs\Ouaricon Microtonal Suite\
  • Dorico Default Library Additions:
      %APPDATA%\Steinberg\Dorico [N]\DefaultLibraryAdditions\
        Ouaricon-VST3-NoteExpression.doricolib
      (NOTE: directory name has NO spaces on Windows. The asymmetry vs
       macOS is intentional — Dorico's discovery code uses different
       names per platform.)

[N] is the latest Dorico major version detected at install time
(probed in descending order: 6 → 5 → 4).

MANUAL IMPORT FALLBACK
----------------------
If Dorico did not auto-discover the files (e.g. Dorico was running during
installation, or a Dorico version was installed AFTER the plugin), import
manually from the canonical shared copy:

  • Playback Template:
      Play → Playback Template → Import…
      Select Ouaricon-Microtonal-Suite.dorico_pt from
        macOS:   ~/Library/Application Support/Ouaricon/Microtonal Suite/
        Windows: %APPDATA%\Ouaricon\Microtonal Suite\
      Then Apply and Close.

  • Expression Map library (standalone):
      Library → Import Library…
      Select Ouaricon-VST3-NoteExpression.doricolib from the same
      shared directory.

After import, the routing is identical to the auto-discovered case.

SOURCE OF TRUTH
---------------
Both files are generated from the canonical sources in the Ouaricon
VST-development repo:
  modules/tuning/note-expression/resources/playback-template/
  modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib
Edits to installed copies are overwritten by the next plugin installer
run. Bug reports and revisions go upstream against the module repo, not
against the installed copies.

SUPPORTED PLUGINS (v1.5 cohort)
-------------------------------
  O-Lyrica, O-Bells, O-IntonationPad, O-Prism,
  O-Wind, O-Reed, O-Bowed, O-Formant

The Playback Template defines slots for all 8 plugins. Dorico will warn
about any plugin that is not currently installed when the template is
applied, but the template still applies cleanly for the plugins that
ARE installed (graceful missing-plugin behavior).

— Ouaricon Audio
