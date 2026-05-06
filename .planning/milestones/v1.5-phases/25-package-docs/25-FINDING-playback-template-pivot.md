---
phase: 25-package-docs
date: 2026-04-26
status: open
type: architectural-finding
supersedes_plan: 25-01-author-and-plumbing-PLAN.md
reverted_commits: [cd2c2c6, 496d4c4, 029b12b]
revert_commit: d2c86c5
---

# Phase 25 Finding — Pivot to Dorico Playback Template

## What was tried (Plan 25-01 v1)

Author a canonical `.doricoexpmap` (kScoreLibrary > expressionMapDefinitions schema, Microtonality field set to `kVST3NoteExpression`) and ship it via the note-expression module's `module.cmake` install rules — dual-write to:

- `~/Library/Application Support/Ouaricon/Expression Maps/` (canonical, editable)
- `~/Library/Application Support/Steinberg/Dorico [N]/Expression Maps/User/` (auto-discovery scan path)

End-to-end canary: cmake --install on O-Lyrica writes both copies byte-identically. Dorico restarted. Then user opens Library → Expression Maps to find "Ouaricon VST3 Note Expression" in the picker.

## Why it failed

**Dorico does not auto-ingest standalone `.doricoexpmap` files dropped into `User/`.**

Verified failure mode:

- File timestamp `14:41`, Dorico 6 launch `14:53`, Score Library init `14:53:56`. File was present before init. No errors logged — silent skip.
- The XML asset is structurally valid (kScoreLibrary > expressionMapDefinitions > entities > ExpressionMapDefinition matches factory `expressionMapsDefinitions.xml` schema).
- `microtonalPlaybackMethod = kVST3NoteExpression` is a valid enum value (confirmed against Dorico 6 binary string table — kAuto, kPitchBend, kVST3NoteExpression all present).
- The `.doricoexpmap` extension itself does not appear in Dorico 6 binary strings as a file-format marker — the only `.dorico*` extensions found in Dorico's installed files are `.dorico` (project files, used by guided tutorials).
- Dorico's UI provides only "Import Library" (`.doricolib`) and "Import Cubase Expression Map" (legacy `.expressionmap`) — there is no "Import Expression Map" command for `.doricoexpmap`.

**Conclusion:** the asset is fine; the distribution mechanism is wrong.

## What we know about Playback Templates so far

Dorico has heavy first-class infrastructure for Playback Templates (binary strings):

- `PlaybackTemplateSpecification`, `IPlaybackTemplateDataProvider`
- `PlaybackTemplateApplyAndEditorController`, `IApplyPlaybackTemplateDialogParent`
- `findEndpointConfigsUsedByPlaybackTemplates`
- `PlaybackTemplateSpecEntriesModel`, `PlaybackTemplateIDTag`
- User-side: `~/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateGenerators/Auto/playbacktemplategen.xml`

Sample `playbacktemplategen.xml` (Dorico 6 default Auto template):

```xml
<?xml version="1.0" encoding="utf-8"?>
<PlaybackTemplateGenerator>
  <fileVersion>1.3</fileVersion>
  <specID>Auto</specID>
  <name>Auto</name>
  <type>kHALion</type>
  <associatedSpaceTemplateID>spacetemplate.reverencecompressor</associatedSpaceTemplateID>
  <singlePluginDefinition>
    <pluginID>8070628D8A894713A7FF69E89507DAEA</pluginID>
    <pluginName>HALion Sonic</pluginName>
    <pluginStateFile>HSSE multi output.pluginstate</pluginStateFile>
    <pluginPresetLibraryIDs>...</pluginPresetLibraryIDs>
    <numChannels>16</numChannels>
    ...
  </singlePluginDefinition>
  <drumKitPluginDefinition>...</drumKitPluginDefinition>
</PlaybackTemplateGenerator>
```

Reference (provided by user): https://www.steinberg.help/r/dorico/doricofirststeps/6.1/en/dorico_first_steps/topics/first_steps_playback/first_steps_playback_template_applying_t.html

## Why Playback Template is the right primitive

A Dorico Playback Template ships an end-to-end routing config:

- **Plugin selection** — names the VST3 by ID, includes a `pluginStateFile` (saved plugin state)
- **Channel assignment** — maps Dorico instruments to plugin channels
- **Expression-map assignment** — embeds or references the right expression map per channel
- **Microtonality** — routed via the embedded expression map

Once imported, the user applies the template via Play → Playback Template, and Dorico auto-loads Ouaricon plugins on the right channels with the right microtonality routing — no per-channel manual setup.

## Open questions for replan

The discuss/research phase should answer at least:

1. **File extension and schema for distributable Playback Templates.** Is it `.doricolib` (a generic library file containing playback templates), a separate `.dorico_pt`-style file, or some other format? The `playbacktemplategen.xml` filename suggests templates are XML; how Dorico imports them from a distributable archive needs to be confirmed (Library → Library Manager? a specific Playback Templates dialog?).

2. **Plugin ID acquisition.** The Auto template uses `8070628D8A894713A7FF69E89507DAEA` for HALion Sonic. We need each Ouaricon plugin's GUID-style `pluginID` to embed in the template. Likely sourced from the plugin's `juce_add_plugin(... PLUGIN_CODE ... PLUGIN_MANUFACTURER_CODE ...)` config or the VST3's UID in the binary.

3. **`pluginStateFile` requirement.** Is a saved plugin state mandatory? If yes, who authors the canonical state for each of the 8 plugins?

4. **One template per plugin, or one omnibus template?** Two design options:
   - **Per-plugin templates:** ship `Ouaricon-O-Lyrica.dorico_pt`, `Ouaricon-O-Bells.dorico_pt`, etc. — user applies the one matching their target plugin.
   - **One Ouaricon Suite template:** ship a single `Ouaricon-Microtonal-Suite.dorico_pt` that knows how to route Dorico's instrument families (winds → O-Wind, strings → O-Bowed, etc.). More user-friendly, more authoring work.

5. **Distribution mechanism.** Bundle the `.dorico_pt` (or `.doricolib`) inside each plugin's PKG/EXE installer (Plan 25-02 would still apply, just with a different file). Or one-time install via a separate Ouaricon "Suite Installer." Or both.

6. **Auto-discovery vs explicit import.** Does Dorico auto-scan a Playback Templates user directory (similar to `PlaybackTemplateGenerators/`)? Or is explicit "Import Library / Apply Playback Template" required? This determines whether the installer can land the file in a useful location.

7. **Carry-forward: is the `.doricoexpmap` still useful?** The reverted asset's XML may still be embeddable inside the Playback Template structure. Don't author from scratch — recover the XML body from the reverted commit cd2c2c6 if/when needed.

## Recommended replan path

1. **`/clear`** — drop execution context.
2. **`/gsd-research-phase 25`** — research-only pass to answer Q1, Q3, Q5, Q6 by exploring Dorico binary, factory `.doricolib` files (if any ship with Dorico), and the Steinberg Help docs the user referenced.
3. **`/gsd-discuss-phase 25 --replan`** — capture answers to Q4 (one-vs-many templates) and Q5 (distribution channel) as locked decisions.
4. **`/gsd-plan-phase 25`** — replan with the Playback Template approach. Plans 25-02 and 25-03 may need adjustment downstream (25-02 is now bundling a Playback Template, not an expression map; 25-03 documentation needs to describe the Playback Template flow, not standalone expression-map import).

## Carry-forward artifacts

- The XML body of the canonical expression map (~60 lines, kScoreLibrary > expressionMapDefinitions > ExpressionMapDefinition with Microtonality = kVST3NoteExpression) is recoverable from `git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` if the new format embeds the same expression-map structure.
- The README-doricoexpmap.txt fallback content is similarly recoverable from `git show cd2c2c6:modules/tuning/note-expression/resources/README-doricoexpmap.txt`.
- The configure_file + install(SCRIPT) CMake pattern (commit 496d4c4) is reusable for the Playback Template install rules — only the source/destination paths and per-platform branches need adjustment.
- System filesystem cleanup performed: removed both Ouaricon-shared and Dorico User/ copies before revert commit (no orphan files left on disk).
