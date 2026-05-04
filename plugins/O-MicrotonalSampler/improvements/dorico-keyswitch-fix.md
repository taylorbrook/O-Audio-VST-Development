# Improvement Brief — Fix Dorico Keyswitch-from-Notation Routing

**Target version:** v1.16.x patch (likely v1.16.2 or v1.17.0)
**Severity:** P1 — broken end-to-end feature in v1.16.0 ship
**Type:** Distribution-artifacts bug. No source-code change to the plugin should be needed.

## Problem

v1.16.0 ships a Dorico expression map (`Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib`) that defines 10 `<playingTechniqueCombination>` entries — one per default technique (ord, sul pont, sul tasto, senza vib, con sord, pizz, harm, mart, trem, flautando) — each with a `<switchOnAction>` of type `kKeySwitch` firing MIDI note 0..9 at velocity 127.

When a user types a playing-technique marking in Dorico (`Shift-P` popover with "sul pont.", "pizz.", "Ord.", etc.) on a staff bound to the "O-MicrotonalSampler" expression map, the keyswitch does NOT fire. The plugin's WebView technique-tab strip does not change during playback. The whole notation→playback pipeline for techniques is non-functional.

## What IS confirmed working in v1.16.0 (do not break)

| Test | Status | Notes |
|---|---|---|
| TC-1: Playback Template appears in `Play → Playback Template` dropdown | ✓ PASS | Reads from `~/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml` |
| TC-3: Expression map appears in Track Inspector → Expression Map dropdown | ✓ PASS — but only after `DefaultLibraryAdditions/` distribution path was discovered | The `playbacktemplatedeps.doricolib` inside `EndpointConfigs/` is endpoint-scoped; the expression map is only globally registered when a `.doricolib` is also placed in `~/Library/Application Support/Steinberg/Dorico 6/DefaultLibraryAdditions/` (auto-merged into every project's library on launch). |
| TC-4: Microtonal pitch via VST3 Note Expression (kVST3NoteExpression) | ✓ PASS | LOAD-BEARING. Quarter-sharp accidentals play at +50¢ via VST3 NE. Microtonality is the plugin's reason to exist; this MUST keep working. |

## What's confirmed NOT working

| Test | Status | Notes |
|---|---|---|
| TC-2: Auto-load Playback Template loads the plugin slot | ✗ FAIL | Dorico log: `[warning] Can't find a template spec or endpoint config for routing this instrument`. The `<entries>` in `playbacktemplatespec.xml` use empty `<instrumentFamilies/>` and `<instruments/>` — Dorico doesn't treat empty as catch-all. Working reference (`EndpointConfigs/Ample China/`) has TWO entries: one endpoint + one fallback `<generatorSpec>`. **This is a separate bug; out of scope for THIS brief unless trivially fixable in passing.** |
| TC-5: Playing-technique text fires keyswitches | ✗ FAIL | The whole subject of this brief. |

## Distribution mechanism findings (already validated, do not re-spike)

A spike against the user's installed Dorico 6 library (Dorico 5 also installed) confirmed the canonical 3-folder layout:

```
~/Library/Application Support/Steinberg/Dorico 6/
├── EndpointConfigs/<Name>/
│   ├── endpointconfig.xml                  ← VST slot + exp-map binding
│   └── playbacktemplatedeps.doricolib      ← endpoint-scoped exp-map(s)
├── PlaybackTemplateSpecs/<Name>/
│   └── playbacktemplatespec.xml            ← user-facing template
└── DefaultLibraryAdditions/                ← auto-merged at launch (REQUIRED for global exp-map registration)
    └── *.doricolib
```

A `.doricolib` placed inside `EndpointConfigs/<Name>/playbacktemplatedeps.doricolib` is **endpoint-scoped** — its expression map is NOT globally registered for use in the Track Inspector dropdown unless a copy is also placed in `DefaultLibraryAdditions/`.

Library Manager's "Import" only accepts `.dorico` project files, not `.doricolib`. Library Manager is NOT the user-facing import path.

## XML comment caveat (already fixed in source tree, ship state)

Dorico's parsers for `endpointconfig.xml`, `playbacktemplatespec.xml`, and `.doricolib` files in `DefaultLibraryAdditions/` reject XML comments BEFORE the root element. They throw "Error opening file: invalid file format" and Dorico crashes on launch. Comment AFTER the root opening tag is fine. The `playbacktemplatedeps.doricolib` parser INSIDE `EndpointConfigs/<Name>/` is more lenient (tolerates pre-root comments) — but it's safest to keep all three files comment-free before root for cross-context portability.

**The current source-tree files are already comment-free before root.** Do not re-add header comments to those three files.

## What's been tried for the KS firing issue (and didn't work)

### Attempt 1: switchOffActions on non-natural slots

**Hypothesis:** Dorico's behavior on "Ord." text is to deactivate the prior technique (e.g. sul pont.). If the prior technique has `<switchOffActions>` with KS 0, Dorico fires KS 0 → plugin returns to ord.

**Change:** Added `<switchOffAction>` of type `kKeySwitch`, `param1`=0, `param2`=127 inside `<switchOffActions>` of all 9 non-natural slots. Slot 0 (pt.natural) kept empty switchOffActions.

**Result:** REGRESSION. Even sul pont's `switchOnAction` stopped firing. Reverted.

### Attempt 2: per-combination `<exclusionGroup>1</exclusionGroup>`

**Hypothesis:** HSO factory exp-maps include `<exclusionGroup>1</exclusionGroup>` per combination. Without it, Dorico's mutual-exclusion logic doesn't fire `pt.natural`'s switchOn when other techniques deactivate.

**Change:** Inserted `<exclusionGroup>1</exclusionGroup>` after `<conditionString/>` in all 10 combinations. Kept `<autoMutualExclusion>true</autoMutualExclusion>` at top level.

**Result:** Did not fix. Sul pont still didn't fire. Reverted.

### Attempt 3: `<version>` bumps + cache clears

Bumped `<version>` 1 → 2 → 3 → 4 with each iteration. Cleared `~/Library/Caches/Dorico 6/cachedFileDataProvider/O-MicrotonalSampler-*.dtn` and `cachesummary.xml`. User restarted Dorico between each iteration.

**Result:** Inconclusive. After full-wipe-and-redeploy with the original known-good shape, sul pont STILL didn't fire — suggesting the original "TC-5 partial PASS where sul pont fired" report was likely a false positive (the user mistook a different audible change for a technique switch — possibly velocity/dynamics/CC11 modulation, NOT the technique sample).

## Current source-tree state (the shipping baseline)

`Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib`:

- Top-level `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>` ✓
- `<autoMutualExclusion>true</autoMutualExclusion>` ✓
- `<volumeType><type>kCC</type><param1>11</param1></volumeType>` (CC11 expression) per combo
- 10 `<playingTechniqueCombination>` entries, each with one `<switchOnAction>` (`kKeySwitch`, `param1`=slot index, `param2`=127)
- All `<switchOffActions>` empty
- No `<exclusionGroup>` per combination
- `<version>4</version>` (bump on next change to force re-merge)

## Key reference: HSO factory expression map

`/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Symphonic Orchestra/expressionMapsDefinitions.xml`

Per-combination shape (e.g. for `pt.pizzicato`):

```xml
<playingTechniqueCombination>
    <baseSwitchID>...</baseSwitchID>
    <techniqueIDs>pt.pizzicato</techniqueIDs>
    <enabled>true</enabled>
    <flags>0</flags>
    <conditionString/>
    <exclusionGroup>1</exclusionGroup>
    <velocityRange>0,127</velocityRange>
    <pitchRange>0,127</pitchRange>
    <transpose>0</transpose>
    <ticksBefore>0</ticksBefore>
    <millisecondsBefore>0</millisecondsBefore>
    <velocityFactor>1.000000</velocityFactor>
    <lengthFactor>1.000000</lengthFactor>
    <pitchBendRange>2</pitchBendRange>
    <microtonalPlaybackMethod>kAuto</microtonalPlaybackMethod>
    <volumeType>
        <type>kCC</type>
        <param1>1</param1>
    </volumeType>
    <attackType>
        <type>kNoteVelocity</type>
        <param1>0</param1>
    </attackType>
    <switchOnActions array="true">
        <switchOnAction>
            <type>kKeySwitch</type>
            <param1>25</param1>
            <param2>127</param2>
        </switchOnAction>
    </switchOnActions>
    <switchOffActions array="true"/>
</playingTechniqueCombination>
```

Differences from our current authored shape:

| Field | HSO | Ours |
|---|---|---|
| `<exclusionGroup>` per combo | `1` | absent |
| `<pitchBendRange>` per combo | `2` | only top-level |
| `<microtonalPlaybackMethod>` per combo | `kAuto` | only top-level (`kVST3NoteExpression`) |
| `<volumeType>` | `kCC param1=1` | `kCC param1=11` |
| `<flags>`, `<monophonic>`, `<applyMillisecondsBefore...>` fields | absent | present |

Note: HSO's `kAuto` in `<microtonalPlaybackMethod>` per combo means the per-combo setting OVERRIDES the top-level `kVST3NoteExpression` and falls back to Auto routing — which would BREAK microtonal playback. Be careful: if the next Claude adds per-combo `<microtonalPlaybackMethod>`, it MUST be `kVST3NoteExpression`, not `kAuto`.

## Suggested investigation paths (in priority order)

### Path A (highest priority): isolate Dorico → plugin MIDI path

Verify whether Dorico is sending the keyswitch notes at all when technique markings are encountered. The plugin's KS handler is known-working from non-Dorico DAW testing in v1.14.0; the unknown is whether Dorico routes the KS through.

**Diagnostics:**
1. **Manual tab click in WebView UI** — confirms plugin state machine reachable from JS layer.
2. **Manual MIDI note entry** — write a literal C-1 note (MIDI 0) on the staff (no technique text). With `ks_enabled=true`, the plugin should absorb the note and switch tab. If yes, plugin's KS handler responds to Dorico-routed MIDI; if no, MIDI isn't reaching the plugin OR plugin's MIDI input is filtered.
3. **MIDI logger** — insert temporary `juce::Logger::writeToLog()` calls in `Source/PluginProcessor.cpp processBlock` for any incoming `juce::MidiMessage` (note-on bytes only). Rebuild dev plugin, retry, inspect log at `~/Library/Logs/O-MicrotonalSampler.log` (or wherever JUCE routes default logs). Confirms whether ANY MIDI reaches the plugin during Dorico playback.
4. **Third-party MIDI monitor** — apps like MIDI Monitor (https://www.snoize.com/MIDIMonitor/) can tap into Dorico's MIDI output. Use to confirm whether Dorico is firing the KS notes at all.

### Path B: replicate working factory exp-map shape

Author the exp-map with the EXACT HSO shape (per-combination exclusionGroup + pitchBendRange + microtonalPlaybackMethod=kVST3NoteExpression — DO NOT use kAuto, it'll break TC-4). Apply HSO's other field ordering. Rebuild + cache-clear + re-merge. Test.

If this works, the issue was missing per-combination fields (ours used a more terse shape than HSO).

### Path C: cross-plugin sanity check

Apply our `playbacktemplatedeps.doricolib` (renamed) to a HALion / HSSE slot in Dorico (load the user's "Test State-less" endpoint and substitute the exp-map). Type "sul pont." text on a staff bound to it. If keyswitches fire on HALion, the exp-map XML is correct and the issue is plugin-side MIDI routing. If they don't fire on HALion either, the exp-map XML is the problem.

### Path D: TC-2 fix (optional bonus)

`playbacktemplatespec.xml` `<entries>` need either populated `<instrumentFamilies>` or a fallback `<generatorSpec>` entry (modeled on Ample China spec). Without this, the auto-load template never fires the endpoint, and users have to manually load O-MicrotonalSampler as a VST. Fixing TC-2 is independent of TC-5 but cheap to bundle in the same patch.

Reference: `~/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ample China/playbacktemplatespec.xml`.

## Files

| Path | Role |
|---|---|
| `plugins/O-MicrotonalSampler/Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib` | Source of truth exp-map (THE file to edit) |
| `plugins/O-MicrotonalSampler/Resources/dorico/EndpointConfigs/O-MicrotonalSampler/endpointconfig.xml` | Endpoint VST slot + exp-map binding |
| `plugins/O-MicrotonalSampler/Resources/dorico/PlaybackTemplateSpecs/O-MicrotonalSampler/playbacktemplatespec.xml` | User-facing Playback Template spec |
| `plugins/O-MicrotonalSampler/Resources/dorico/INSTALL-DORICO.md` | End-user install guide (updated for `DefaultLibraryAdditions/` path) |
| `plugins/O-MicrotonalSampler/Resources/dorico/SMOKE-TEST.md` | Manual smoke procedure (TC-1..TC-6) |
| `~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/O-MicrotonalSampler/` | Deploy mirror (test with) |
| `~/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/O-MicrotonalSampler/` | Deploy mirror |
| `~/Library/Application Support/Steinberg/Dorico 6/DefaultLibraryAdditions/O-MicrotonalSampler.doricolib` | Deploy mirror (REQUIRED for global exp-map registration) |
| `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Symphonic Orchestra/expressionMapsDefinitions.xml` | Working factory reference |
| `~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Test State-less/` | User's stateless reference |
| `~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ample China/` | User's full-template reference (with PlaybackTemplate fallback `<generatorSpec>` entry) |
| `~/Library/Application Support/Steinberg/Dorico 6/application.log` | Dorico log (filter for `MicrotonalSampler`, `Endpoint`, `PlaybackTemplate`, `error`) |
| `~/Library/Caches/Dorico 6/cachedFileDataProvider/` | Parsed cache. Clear `O-MicrotonalSampler-*.dtn` AND `cachesummary.xml` after every change. |

## Test loop (per change)

1. Quit Dorico (`Cmd+Q`). Wait for the process to fully exit.
2. Edit source files under `plugins/O-MicrotonalSampler/Resources/dorico/`.
3. Bump `<version>N</version>` in `playbacktemplatedeps.doricolib` (this defeats some of Dorico's caching).
4. `xmllint --noout <file>` on every file changed — Dorico's "invalid file format" parse errors will crash launch.
5. Redeploy:
   ```bash
   cp plugins/O-MicrotonalSampler/Resources/dorico/EndpointConfigs/O-MicrotonalSampler/*.{xml,doricolib} \
       "/Users/taylorbrook/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/O-MicrotonalSampler/"
   cp plugins/O-MicrotonalSampler/Resources/dorico/PlaybackTemplateSpecs/O-MicrotonalSampler/*.xml \
       "/Users/taylorbrook/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/O-MicrotonalSampler/"
   cp plugins/O-MicrotonalSampler/Resources/dorico/EndpointConfigs/O-MicrotonalSampler/playbacktemplatedeps.doricolib \
       "/Users/taylorbrook/Library/Application Support/Steinberg/Dorico 6/DefaultLibraryAdditions/O-MicrotonalSampler.doricolib"
   ```
6. Clear cache:
   ```bash
   rm -f "/Users/taylorbrook/Library/Caches/Dorico 6/cachedFileDataProvider/O-MicrotonalSampler-"*.dtn \
         "/Users/taylorbrook/Library/Caches/Dorico 6/cachedFileDataProvider/cachesummary.xml"
   ```
7. Launch Dorico. Open the test project (or a fresh one). Verify Dorico launches without crashing (parse errors crash on launch).
8. Tail the log:
   ```bash
   grep -i -E "MicrotonalSampler|invalid|error" \
       "/Users/taylorbrook/Library/Application Support/Steinberg/Dorico 6/application.log" | tail -10
   ```
9. Test: bind exp-map → write note → add technique text → play → check WebView technique-tab strip.

## Risk envelope

- **DO NOT modify plugin C++ source code unless investigation conclusively isolates the issue to plugin MIDI handling.** This brief is for fixing the distribution artifacts. Plugin source is at `Source/PluginProcessor.cpp` `processBlock` method (KS scanning) and `Source/TriggerMapping.h` (resolver). v1.14.0 + v1.15.0 already ship and are validated against non-Dorico DAWs.
- **DO NOT add XML comments before root elements.** They crash Dorico on launch with "invalid file format".
- **DO NOT change `<microtonalPlaybackMethod>` to `kAuto`.** That breaks TC-4 (microtonal pitch — load-bearing).
- **DO NOT delete or modify the comment in `playbacktemplatedeps.doricolib`'s description field that documents kVST3NoteExpression** — that's a load-bearing schema element, not a free-text comment.

## Acceptance for the v1.16.x patch

1. Type "sul pont." text (Shift-P popover) on a Solo Violin staff with the bound exp-map "O-MicrotonalSampler". Hit play. Plugin's WebView UI technique-tab strip shows "sp" tab highlighted while the note plays.
2. Type "Ord." text on the next note. Plugin's tab returns to "ord".
3. Microtonal pitch (TC-4) still works.
4. Dorico still launches without crashing on session start.
5. All other existing Dorico-distribution behaviors preserved (template appears in dropdown, exp-map binds in Track Inspector dropdown).
6. (Bonus, not required) TC-2 fix — apply Playback Template auto-loads the plugin slot.

## Versioning notes

- v1.16.0 was committed (`feat(O-MicrotonalSampler): v1.16.0 — Dorico distribution`, commit `7e56e16`, tag `v1.16.0-O-MicrotonalSampler`) BEFORE the comment-strip + DefaultLibraryAdditions findings.
- The current working tree includes the comment-strip patches (in `endpointconfig.xml`, `playbacktemplatespec.xml`, `playbacktemplatedeps.doricolib`) and a few iteration leftovers (version=4 in the doricolib). These should ship as **v1.16.1 (PATCH)** — pre-fix the launch crash + add `DefaultLibraryAdditions/` distribution path + revise `INSTALL-DORICO.md`.
- The KS-firing fix from this brief should ship as **v1.16.2 (PATCH)** unless investigation reveals a schema-level redesign of the exp-map (in which case MINOR).

A v1.16.1 commit may or may not exist at the time the next Claude reads this. Check `git log` first; if the working tree has uncommitted edits to `Resources/dorico/`, the v1.16.1 patch is still pending and should be committed FIRST as a clean baseline before the v1.16.2 work.
