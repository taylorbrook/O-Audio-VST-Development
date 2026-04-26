Ouaricon VST3 Note Expression — Dorico Expression Map
======================================================

PURPOSE
-------
This file (Ouaricon-VST3-NoteExpression.doricoexpmap) configures Dorico to
route per-note microtonal pitch deltas as VST3 Note Expression events to
Ouaricon plugins. Without it, Dorico defaults to pitch-bend or VST2 detune
routing for non-Steinberg VST3s, and microtonal accidentals play as plain
12-TET (broken playback, no error).

INSTALL LOCATIONS
-----------------
The plugin installer wrote this file to two paths:
  • Editable canonical copy:
      macOS:   ~/Library/Application Support/Ouaricon/Expression Maps/
      Windows: %APPDATA%\Ouaricon\Expression Maps\
  • Dorico auto-scan path (for the picker to find it without manual import):
      macOS:   ~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/
      Windows: %APPDATA%\Steinberg\Dorico [N]\Expression Maps\User\
where [N] is the latest Dorico major version detected at install time.

MANUAL IMPORT FALLBACK
----------------------
If the file does not appear in Dorico's expression-map picker
(Library → Expression Maps…), import it manually:
  1. Dorico → Library → Expression Maps…
  2. Click the Import (folder) icon at the bottom of the list
  3. Navigate to the canonical copy path above and select
     Ouaricon-VST3-NoteExpression.doricoexpmap
  4. Save
Then assign it to your Ouaricon plugin's channel:
  • Play → Endpoint Setup → expression-map dropdown for the plugin's channel
    → select "Ouaricon VST3 Note Expression"

SOURCE OF TRUTH
---------------
This file is generated from the canonical copy in the Ouaricon module repo:
  modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap
Edits to installed copies are overwritten by the next plugin installer run.
Edit the canonical file in the repo if changes are needed.

SUPPORTED PLUGINS (v1.5 cohort)
-------------------------------
  O-Lyrica, O-Bells, O-IntonationPad, O-Prism,
  O-Wind, O-Reed, O-Bowed, O-Formant
