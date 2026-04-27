OUARICON MICROTONAL SUITE — DORICO EXPRESSION MAP
=================================================

PURPOSE
-------
Adds an "Ouaricon VST3 Note Expression" expression map to Dorico, which
routes microtonal pitches as VST3 Note Expression events to Ouaricon
plugins. This is a one-time, manual import per machine — Dorico does
not auto-discover the file. Once imported, the expression map appears
under Library → Expression Maps and can be assigned to any Ouaricon
plugin's channel under Play → Endpoints. The map encodes
microtonalPlaybackMethod=kVST3NoteExpression, which is the load-bearing
setting for non-Steinberg VST3 microtonal routing; Dorico's default
Auto setting falls back to pitch-bend and silently breaks microtonal
playback for these plugins.

INSTALL LOCATIONS
-----------------
The plugin installer drops the .doricolib at one platform-specific
shared path. The installer does NOT write to Dorico's user-data
directories under v1.5.

  macOS:
    ~/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib

  Windows:
    %APPDATA%\Ouaricon\Microtonal Suite\Ouaricon-VST3-NoteExpression.doricolib

MANUAL IMPORT
-------------
One-time per machine, after installing any Ouaricon plugin from the
v1.5 cohort:

  1. Open Dorico (any project).
  2. Library → Library Manager → Import…
  3. Select the .doricolib from the install location above.
  4. Confirm the import. The expression map "Ouaricon VST3 Note
     Expression" now appears under Library → Expression Maps.
  5. Per project: load any Ouaricon plugin via Play → Endpoints → Add
     Plug-in, then assign the expression map to the plugin's channel
     via Play → Endpoints → Expression Map dropdown.

The import persists across Dorico restarts and across Dorico version
upgrades. Re-importing after a plugin reinstall is harmless but not
required (the .doricolib content is stable).

SOURCE OF TRUTH
---------------
This file ships from
  modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib
in the Ouaricon plugin source repo. DO NOT edit installed copies —
they are overwritten by the next plugin installer run. Bug reports
and revisions go upstream against the module repo.

SUPPORTED PLUGINS (v1.5 cohort)
-------------------------------
  O-Lyrica, O-Bells, O-IntonationPad, O-Prism,
  O-Wind, O-Reed, O-Bowed, O-Formant

The expression map is plugin-agnostic — it routes Note Expression
events to whatever VST3 plugin the user assigns it to. The cohort
list is informational; any future Ouaricon plugin built on the
shared note-expression module will work with the same .doricolib
without changes.

— Ouaricon Audio, v1.5 (Phase 25 v3 / Path B)
