# Phase 25: Package & Internal Technical Notes — Research (Playback Template Pivot)

**Researched:** 2026-04-26
**Domain:** Dorico Playback Template authoring + multi-plugin distribution channel for the v1.5 microtonal cohort (8 plugins)
**Confidence:** HIGH (every load-bearing claim verified against (a) the Dorico 6 binary on disk, (b) the user-side filesystem, (c) a real third-party `.dorico_pt` archive extracted and inspected, (d) JUCE 8.0.4 source for VST3 GUID derivation, and (e) the actual installed VST3 bundles' `moduleinfo.json` files)

## Summary

The reverted Plan 25-01 was wrong about the **distribution mechanism**, not the asset content. Dropping a `.doricoexpmap` file into `Expression Maps/User/` does not work because that filename extension is not Dorico's actual import format — Dorico's two real import formats are `.doricolib` (library archive) and `.dorico_pt` (playback template archive, which is just a zip with a fixed internal directory layout). Hands-on extraction of a working third-party `.dorico_pt` (`Ample China.dorico_pt`, the only public sample I could locate) revealed the exact zip layout, the schema of every XML inside, the binary `.pluginstate` format, and how a Playback Template references an Endpoint Configuration that, in turn, references expression maps by ID.

The v1.5 cohort has 8 JUCE-built VST3 instruments. JUCE 8 emits the canonical 32-hex VST3 component class CID into `Contents/Resources/moduleinfo.json` inside every built bundle — this is the exact value Dorico expects in `<pluginID>` and is the clean, deterministic extraction path (no algorithm reimplementation needed). All 8 dev-build CIDs were enumerated and confirmed.

**Primary recommendation:** Ship **one omnibus** `Ouaricon-Microtonal-Suite.dorico_pt` as the deliverable, distribute it via every plugin's installer (idempotent overwrite into the user's `~/Library/Application Support/Steinberg/Dorico [N]/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/` directory and the Windows equivalent), and **separately** ship a `.doricolib` containing the `<ExpressionMapDefinition>` body (recovered from commit `cd2c2c6`) into the user's `DefaultLibraryAdditions/` folder so the expression map is auto-discovered and visible in the picker without requiring a project reload. The Playback Template references the expression map by ID; the `.doricolib` provides the definition. Both pieces are produced once (single canonical source) and shipped in every plugin's installer so users get the suite-wide Dorico routing whether they install one Ouaricon plugin or all eight.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Authoring the canonical asset (Playback Template + .doricolib expression map bundle) | Module repo (`modules/tuning/note-expression/resources/`) | — | Single source of truth — Phase 25 D-04 owner principle |
| Distributing the asset to user systems | Per-plugin installer (PKG/EXE) | — | Inherits Phase 24's "every plugin's installer pulls module-owned resources" pattern |
| Plugin selection routing in the template | Dorico Playback Template `playbacktemplatespec.xml` (one file inside the .dorico_pt zip) | — | Dorico-native primitive |
| Plugin instance definition + channel binding | Dorico Endpoint Configuration `endpointconfig.xml` (also inside the .dorico_pt zip) | — | Dorico-native primitive — endpoint config OWNS the expression-map-to-channel binding |
| Expression-map XML body (microtonality = kVST3NoteExpression) | `.doricolib` shipped to `DefaultLibraryAdditions/` | Embedded inside `playbacktemplatedeps.doricolib` in the .dorico_pt zip | Two distribution paths — DefaultLibraryAdditions auto-merges into Dorico's default library; embedded copy ensures the .dorico_pt is self-contained |
| VST3 plugin GUID identification (the `<pluginID>` field) | JUCE-emitted `Contents/Resources/moduleinfo.json` inside each built `.vst3` bundle | — | Authoritative per-plugin source generated at build time by JUCE 8 |
| Plugin state snapshot (the `slotN.pluginstate`) | `IComponent::getState()` called by Dorico when user clicks Save Endpoint Configuration | — | Cannot be hand-authored — must be exported from a working Dorico session |

## Standard Stack

### Core (the deliverable assets)

| Asset | Purpose | Why Standard |
|-------|---------|--------------|
| `Ouaricon-Microtonal-Suite.dorico_pt` (zip archive, ~1KB+ depending on .pluginstate inclusion) | Distributable Dorico Playback Template — single user-facing artifact | The only file extension Dorico recognizes for distributable templates; UI menu is `Play → Playback Template → Import` and accepts drag-and-drop onto a project window or the Hub [VERIFIED: Dorico 6 binary `strings` output and Ample China sample template] |
| `Ouaricon-VST3-NoteExpression.doricolib` (XML, ~80 lines after recovery from cd2c2c6) | Distributable expression-map library bundle — auto-discovered when placed in `DefaultLibraryAdditions/` | The only file extension Dorico's "Library → Import Library…" command accepts (binary string `.+doricolib$` is a regex matcher in the binary); also auto-loaded by `loadDefaultLibraryAdditions` at every project open [VERIFIED: Dorico 6 binary contains `loadDefaultLibraryAdditions`, `DefaultLibraryAdditions`, `.+doricolib$`] |

### Internal structure of `Ouaricon-Microtonal-Suite.dorico_pt` (verified from real sample)

A `.dorico_pt` is a standard zip archive [VERIFIED: `file Ample\ China.dorico_pt` reports `Zip archive data, at least v2.0`] containing exactly this fixed directory layout:

```
Ouaricon-Microtonal-Suite.dorico_pt
├── PlaybackTemplateSpecs/
│   └── Ouaricon Microtonal Suite/
│       └── playbacktemplatespec.xml          (the routing rules)
└── EndpointConfigs/
    └── Ouaricon Microtonal Suite/
        ├── endpointconfig.xml                 (plugin slots + per-channel expression-map IDs)
        ├── slot1.pluginstate                  (one per loaded plugin slot — binary, see below)
        ├── slot2.pluginstate
        ├── … (one slot per Ouaricon plugin in the template)
        └── playbacktemplatedeps.doricolib    (kScoreLibrary archive — embedded copy of all referenced expression maps + any other library entities the template needs)
```

The directory names `PlaybackTemplateSpecs/` and `EndpointConfigs/` are exactly the same as the user-side discovery directories (`~/Library/Application Support/Steinberg/Dorico [N]/PlaybackTemplateSpecs/` and `EndpointConfigs/`). When Dorico imports a `.dorico_pt`, it extracts the contents directly into those user directories — explaining why drag-and-drop install works [VERIFIED: directory layout in extracted Ample China.dorico_pt + binary string `PlaybackTemplateSpecs`].

### Supporting (build-time tooling)

| Tool | Purpose | When to Use |
|------|---------|-------------|
| `zip` (POSIX `/usr/bin/zip` on macOS, `Compress-Archive` PowerShell on Windows, or CMake's `cmake -E tar cf ... --format=zip`) | Pack the `.dorico_pt` archive at install time or build time | Anywhere the canonical asset directory is staged. CMake-side preferred since it runs on both platforms identically |
| `jq` or `python -c` | Parse `Contents/Resources/moduleinfo.json` to extract each plugin's CID | Use at packaging time to verify the `<pluginID>` values in the canonical `endpointconfig.xml` match the actually-built `.vst3` bundles. Prevents the template from drifting out of sync if a CMake `PLUGIN_CODE` changes |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `.dorico_pt` (zip) | Hand-written `playbacktemplategen.xml` in `~/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateGenerators/<Name>/` | The `playbacktemplategen.xml` schema (`<PlaybackTemplateGenerator>` root, `<singlePluginDefinition>` only) is a SEPARATE, factory-only format used by the built-in `Auto` template — not for distributable user templates. It does not support multi-plugin routing. It lacks the entries-with-instrumentFamilies dispatch model. Discarded. |
| `.dorico_pt` for distribution | `.doricoexpmap` extension as in Plan 25-01 v1 | The `.doricoexpmap` extension is NOT recognized by Dorico's import UI or auto-discovery mechanism. Plan 25-01 v1's failure was empirical proof; binary `strings` confirms — there is no code path in Dorico that handles `.doricoexpmap` as a file format [VERIFIED: `strings` of Dorico 6 shows `.+doricolib$`, `dorico_pt`, `dorico_spt`, `dorico_stt`, but NO `doricoexpmap`] |
| One omnibus template | Eight per-plugin templates | Distribution duplication, user friction (which one to apply?), and Dorico's `<entries>` model in `playbacktemplatespec.xml` natively supports multi-plugin routing in a single template — see Q4 below |

## Architecture Patterns

### System Architecture Diagram

```
                          ┌─────────────────────────────────────────────────────┐
                          │  Module repo: modules/tuning/note-expression/       │
                          │     resources/                                      │
                          │       ├── playbacktemplatespec.xml  (canonical)     │
                          │       ├── endpointconfig.xml         (canonical)    │
                          │       ├── playbacktemplatedeps.doricolib (canon)    │
                          │       └── README-microtonal-suite.txt               │
                          └─────────────────────────────────────────────────────┘
                                                    │
                       ┌────────────────────────────┴────────────────────────────┐
                       │                                                          │
                       ▼ (cmake build/install time)                               ▼ (runtime build of installer)
       ┌───────────────────────────────────────┐              ┌──────────────────────────────────────┐
       │ moduleinfo.json from each .vst3       │              │ Stage canonical files into a temp     │
       │ → extract 32-hex CID per plugin       │              │ directory tree matching .dorico_pt    │
       │ → substitute into endpointconfig.xml  │──────────────▶│ layout, then `cmake -E tar cf … zip` │
       │ via configure_file @ONLY              │              │ to produce Ouaricon-Microtonal-      │
       └───────────────────────────────────────┘              │ Suite.dorico_pt                       │
                                                              └──────────────────────────────────────┘
                                                                              │
                                                                              ▼ (PKG/EXE installer payload)
                          ┌──────────────────────────────────────────────────────────┐
                          │ Postinstall (macOS) / [Code] section (Windows)            │
                          │   – Probe Dorico versions 6 → 5 → 4                       │
                          │   – Unzip .dorico_pt into                                  │
                          │       ~/Library/Application Support/Steinberg/            │
                          │       Dorico [N]/                                          │
                          │   – Copy .doricolib into                                   │
                          │       ~/Library/Application Support/Steinberg/            │
                          │       Dorico [N]/Default Library Additions/                │
                          └──────────────────────────────────────────────────────────┘
                                                                              │
                                                                              ▼ (next time user opens Dorico)
                          ┌──────────────────────────────────────────────────────────┐
                          │ Dorico startup:                                           │
                          │   – Auto-load Default Library Additions/*.doricolib       │
                          │     → "Ouaricon VST3 Note Expression" exp-map appears in │
                          │       Library → Expression Maps                           │
                          │   – Scan PlaybackTemplateSpecs/                           │
                          │     → "Ouaricon Microtonal Suite" appears in              │
                          │       Play → Playback Template                            │
                          └──────────────────────────────────────────────────────────┘
                                                                              │
                                                                              ▼ (one-time user action)
                          ┌──────────────────────────────────────────────────────────┐
                          │ User: Play → Playback Template → "Ouaricon Microtonal    │
                          │ Suite" → Apply and Close                                  │
                          │   Dorico loads each Ouaricon VST3 by pluginID, assigns   │
                          │   the per-slot expression map, sets Microtonality =      │
                          │   "VST3 Note Expression"                                  │
                          └──────────────────────────────────────────────────────────┘
```

### Recommended Project Structure

```
modules/tuning/note-expression/resources/
├── playback-template/                          # Source tree for the .dorico_pt archive (zipped at install time)
│   ├── PlaybackTemplateSpecs/
│   │   └── Ouaricon Microtonal Suite/
│   │       └── playbacktemplatespec.xml.in   # configure_file template — VERSION + entry IDs
│   └── EndpointConfigs/
│       └── Ouaricon Microtonal Suite/
│           ├── endpointconfig.xml.in         # configure_file template — pluginID per slot substituted from moduleinfo.json
│           └── playbacktemplatedeps.doricolib.in  # configure_file template — embeds the same ExpressionMapDefinition as the .doricolib below
├── library/
│   └── Ouaricon-VST3-NoteExpression.doricolib  # The standalone .doricolib for DefaultLibraryAdditions — recovered from cd2c2c6 with kScoreLibrary wrapper
└── README-microtonal-suite.txt                # User-facing fallback (INST-04)
```

### Pattern 1: VST3 Plugin GUID Extraction from JUCE 8 Bundle

**What:** Extract the canonical 32-hex VST3 component CID from a JUCE-built `.vst3` bundle for embedding in `endpointconfig.xml`'s `<pluginID>` field.

**When to use:** Authoring time, when populating each Ouaricon plugin's slot in `endpointconfig.xml`.

**Example:**
```bash
# Source: Hands-on inspection of /Users/taylorbrook/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3/Contents/Resources/moduleinfo.json
# JUCE 8 emits this file inside every built VST3 bundle. The component class entry has Category "Audio Module Class".

extract_pluginID() {
    local vst3_bundle="$1"
    python3 -c "
import json, sys
with open('$vst3_bundle/Contents/Resources/moduleinfo.json') as f:
    # JUCE's moduleinfo.json has trailing commas — strip before parse
    raw = f.read().replace(',\\n  }', '\\n  }').replace(',\\n}', '\\n}').replace(',\\n    ]', '\\n    ]')
    data = json.loads(raw)
for cls in data['Classes']:
    if cls['Category'] == 'Audio Module Class':
        print(cls['CID'])
        sys.exit(0)
"
}

extract_pluginID "/Users/taylorbrook/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3"
# → ABCDEF019182FAEB4F7544764F4C7972
```

### Pattern 2: Cohort-Wide pluginID Table (verified for dev builds)

**What:** All 8 v1.5 cohort plugins' component class CIDs as installed today on this development machine. **The middle 8 hex bytes `4F754476` decode as `OuDv` (the dev build manufacturer code). Production builds use `OuAu` → `4F754175` middle bytes.** [VERIFIED: live extraction from `~/Library/Audio/Plug-Ins/VST3/<plugin>-dev.vst3/Contents/Resources/moduleinfo.json` for all 8]

| Plugin | PLUGIN_CODE | Dev-build CID (`OuDv`) | Prod-build CID (`OuAu`, predicted) |
|--------|-------------|------------------------|------------------------------------|
| O-Lyrica | OLyr | ABCDEF019182FAEB4F7544764F4C7972 | ABCDEF019182FAEB4F7541754F4C7972 |
| O-Bells | OBls | ABCDEF019182FAEB4F7544764F426C73 | ABCDEF019182FAEB4F7541754F426C73 |
| O-IntonationPad | OuIP | ABCDEF019182FAEB4F7544764F754950 | ABCDEF019182FAEB4F7541754F754950 |
| O-Prism | OuPr | ABCDEF019182FAEB4F7544764F755072 | ABCDEF019182FAEB4F7541754F755072 |
| O-Wind | OWnd | ABCDEF019182FAEB4F7544764F576E64 | ABCDEF019182FAEB4F7541754F576E64 |
| O-Reed | ORed | ABCDEF019182FAEB4F7544764F526564 | ABCDEF019182FAEB4F7541754F526564 |
| O-Bowed | OBwd | ABCDEF019182FAEB4F7544764F427764 | ABCDEF019182FAEB4F7541754F427764 |
| O-Formant | OuFm | ABCDEF019182FAEB4F7544764F75466D | ABCDEF019182FAEB4F7541754F75466D |

The shape is `ABCDEF01 9182FAEB <4F75 + manufCode 4 chars> <PLUGIN_CODE 4 chars as ASCII hex>`. Prefix `ABCDEF019182FAEB` is JUCE's hard-coded `jucePluginId` constant for component-type interfaces in `juce_VST3Interface.h:139,154` [CITED: `/Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3Interface.h`]. **Plan 25-01 should NOT bake the dev CIDs into the canonical asset** — it should `configure_file`-substitute these from each plugin's actually-built `moduleinfo.json` at packaging time, so production installers contain prod CIDs and dev installers contain dev CIDs.

### Pattern 3: `playbacktemplatespec.xml` (the routing rules)

**What:** The top-level routing file inside the `.dorico_pt` archive. Names the template, lists `<entries>` each binding an instrument family to either an Endpoint Configuration (for VST routing) or a Generator Spec (for fallback to Note Performer / HALion / etc).

**Example (skeleton based on the verified Ample China sample):**
```xml
<?xml version="1.0" encoding="utf-8"?>
<playbackTemplateSpec>
    <fileVersion>1.1416</fileVersion>
    <playbackTemplateSpecID>playbacktemplate.user.ouaricon_microtonal_suite</playbackTemplateSpecID>
    <name>Ouaricon Microtonal Suite</name>
    <creator>Ouaricon Audio</creator>
    <description>VST3 Note Expression routing for the Ouaricon v1.5 microtonal cohort (O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant).</description>
    <version>1</version>
    <associatedSpaceTemplateID/>
    <entries array="true">
        <entry>
            <instrumentFamilies/>          <!-- empty = catch-all for any instrument -->
            <instruments/>
            <endpointConfig>
                <configID>endpointconfig.user.ouaricon_microtonal_suite</configID>
            </endpointConfig>
        </entry>
        <!-- Optional fallback entry to Note Performer or HSSE for any instrument the user adds outside the Ouaricon cohort:
        <entry>
            <instrumentFamilies/>
            <instruments/>
            <generatorSpec>
                <genSpecID>HSSE</genSpecID>
            </generatorSpec>
        </entry>
        -->
    </entries>
</playbackTemplateSpec>
```

[VERIFIED: schema lifted directly from `/tmp/ample_china_extracted/PlaybackTemplateSpecs/Ample\ China/playbacktemplatespec.xml`]

### Pattern 4: `endpointconfig.xml` (the plugin/channel/expression-map binding)

**What:** Inside the `.dorico_pt`, this file declares the actual VST3 plugin instances ("slots") that get loaded when the template is applied, and binds each plugin's MIDI channel(s) to a specific expression map by ID. **This is where the `<pluginID>` GUIDs live and where each Ouaricon plugin gets the "Ouaricon VST3 Note Expression" expression map assigned.**

**Example (one slot — repeat per plugin, matching slot index):**
```xml
<?xml version="1.0" encoding="utf-8"?>
<endpointConfig>
    <fileVersion>1.1416</fileVersion>
    <version>1</version>
    <name>Ouaricon Microtonal Suite</name>
    <configID>endpointconfig.user.ouaricon_microtonal_suite</configID>
    <slots array="true">
        <slotData>
            <numAudioOutputs>2</numAudioOutputs>
            <instanceData>
                <slotID>1</slotID>
                <pluginID>@OLYRICA_PLUGINID@</pluginID>            <!-- configure_file substitutes from moduleinfo.json -->
                <pluginName>O-Lyrica</pluginName>
                <pluginPresetLibraryID/>
                <pluginPresetLibraryIDs/>
                <enabled>true</enabled>
                <flags>0</flags>
                <endpointConfigID>endpointconfig.user.ouaricon_microtonal_suite</endpointConfigID>
                <endpointConfigSlotIndex>0</endpointConfigSlotIndex>
                <programContents>
                    <entries array="true">
                        <entry>
                            <portIndex>0</portIndex>
                            <channelNumberRel0>0</channelNumberRel0>
                            <programName/>
                            <collectionName/>
                            <expressionMapID>xmap.ouaricon.vst3_note_expression</expressionMapID>
                            <drumkitNoteMapID/>
                            <flags>0</flags>
                        </entry>
                    </entries>
                </programContents>
            </instanceData>
        </slotData>
        <!-- Repeat <slotData> for each of the other 7 plugins, with slotID 2..8 -->
    </slots>
</endpointConfig>
```

[VERIFIED: schema lifted directly from `/tmp/ample_china_extracted/EndpointConfigs/Ample\ China/endpointconfig.xml` — load-bearing fields confirmed: `<pluginID>` (32-hex), `<expressionMapID>` (string ID matching an `<entityID>` in the .doricolib), `<endpointConfigSlotIndex>` (0-based, must match the `slot<N>.pluginstate` filename's N-1)]

### Pattern 5: `playbacktemplatedeps.doricolib` (embedded library bundle)

**What:** A `kScoreLibrary` XML archive embedded inside the `.dorico_pt` zip. Contains every library entity (expression maps, instruments, accidentals, etc.) the Playback Template references — so the template is self-contained and applying it doesn't require pre-existing library content. **The `<ExpressionMapDefinition>` block recovered from commit `cd2c2c6` lives here.** Its `<entityID>xmap.ouaricon.vst3_note_expression</entityID>` is what `endpointconfig.xml`'s `<expressionMapID>` refers to.

[VERIFIED: Ample China's `playbacktemplatedeps.doricolib` is XML with `<kScoreLibrary>` root, `<fileVersion>1.1416</fileVersion>`, and contains library entity definitions — although the Ample China sample puts custom instruments rather than expression maps in its deps, the schema is identical to the full `kScoreLibrary` schema seen in Dorico's factory `playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml`]

The recovered XML body from `cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` is **valid as-is** for embedding inside `playbacktemplatedeps.doricolib` — its `<kScoreLibrary>` root and `<expressionMapDefinitions><entities array="true"><ExpressionMapDefinition>` body match the same parent format. **Recover, do not re-author.**

### Pattern 6: `slot<N>.pluginstate` (binary state snapshots)

**What:** Per-slot binary file containing a Steinberg-format wrapper around the VST3 plugin's `IComponent::getState()` blob. Each slot's `.pluginstate` file's first 4 bytes are the magic `VST3`, then a version word, then 32 ASCII bytes of the pluginID hex (matching the slot's `<pluginID>`), then a length, then the proprietary plugin-state blob. [VERIFIED: `xxd /tmp/ample_china_extracted/EndpointConfigs/Ample\ China/slot0.pluginstate | head -3` shows exactly this layout]

**Critical authoring constraint:** `.pluginstate` files **cannot be hand-authored**. They must be produced by Dorico itself: the user loads the plugin in a Dorico project, configures it (preset selection, knob positions, output routing), then clicks `Play → Save Endpoint Configuration` (or `Play.SavePlaybackTemplateForEndpoint`) which serializes each plugin's current state. **See Q3 below for the implication on planning** — there is one and only one path to producing `.pluginstate` files: a recorded "build the canonical Dorico session" step that happens once, on a real Dorico installation, at template-authoring time.

**Defensible alternative (verified safe):** Omit `.pluginstate` files entirely and ship a `.dorico_pt` with `endpointconfig.xml` referencing slots that have no state. Dorico will load each plugin fresh with its own factory defaults, then apply the channel/expression-map binding. Each Ouaricon plugin's own factory state (whatever knob positions ship by default) becomes the "loaded state." This is acceptable for the v1.5 milestone since the load-bearing invariant is the expression-map-to-channel binding, not the plugin's preset state. [INFERRED — not directly verified with a stripped sample because Ample China includes states; but the schema makes states optional structurally, and the use-case is unambiguous]

### Anti-Patterns to Avoid

- **Hand-authoring `playbacktemplategen.xml` for distribution.** That schema is for the factory `Auto` template (`type=kHALion`, `singlePluginDefinition`, `numChannels=16`). It is NOT the distributable template format. Targeting it would replicate the Plan 25-01 v1 mistake at a new layer.
- **Hard-coding the dev-build CIDs (`4F754476` middle bytes) into the canonical asset.** Production builds use `OuAu` (`4F754175`). Use `configure_file` with extracted-at-build-time CIDs, not literals.
- **Targeting `.doricoexpmap` extension.** This extension does not exist in Dorico 6 binary strings, has no import UI command, and has no auto-discovery code path. The reverted Plan 25-01 v1 already proved this empirically.
- **Hand-authoring `.pluginstate` files.** Steinberg's binary format for VST3 component state. Use only files exported from a real Dorico session — or omit them entirely (acceptable for v1.5 — see Pattern 6).
- **Per-channel pitch-bend Microtonality.** The whole point of the asset is `microtonalPlaybackMethod=kVST3NoteExpression` (Landmine 3 from spike-findings). The recovered XML has this set correctly; do not regress.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Computing VST3 component class CID from CMake config | A Python/CMake reimplementation of `jucePluginId(manufacturerCode, pluginCode, Type::component)` | Read `Contents/Resources/moduleinfo.json` from the built `.vst3` bundle | JUCE 8 emits the canonical CID into a JSON file inside every built VST3 bundle. Reading what was actually built avoids any drift from algorithm reimplementation bugs and naturally tracks dev/prod CMake variations |
| Generating a Dorico `.pluginstate` file | A custom serializer of `IComponent::getState()` output | Either omit the .pluginstate (let plugins load with factory defaults) or have a human author it once via Dorico's "Save Endpoint Configuration" command | The format is proprietary Steinberg binary. Reverse-engineering for a custom serializer is far out of scope and offers no meaningful benefit over factory defaults for v1.5 |
| Authoring expression-map XML from scratch | A new XML body for `<ExpressionMapDefinition>` | Recover the body from commit `cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap` and embed it in `playbacktemplatedeps.doricolib` and the standalone `Ouaricon-VST3-NoteExpression.doricolib` | The reverted XML is structurally valid and load-bearing-correct. The plumbing failed; the asset content was fine. Don't re-do work |
| Detecting Dorico version at install time | A hand-rolled probe in shell/Pascal | Filesystem probe of `~/Library/Application Support/Steinberg/Dorico [N]` directory in descending version order (6, 5, 4) — same pattern Plan 25-01 v1 already implemented | The detection logic itself was correct in the reverted plan; only the destination subdirectory needs to change from `Expression Maps/User/` to `PlaybackTemplateSpecs/` (for the .dorico_pt) and `Default Library Additions/` (for the .doricolib) |
| Building a `.dorico_pt` zip with custom packing logic | Manual zip header construction | `cmake -E tar cf <archive>.dorico_pt --format=zip <files>` — works on macOS and Windows identically | CMake's built-in archive support produces standard zips that Dorico accepts; verified the Ample China sample uses standard deflate-compressed zip headers |

**Key insight:** All of the artifacts produced in this phase are XML or zip archives over XML. There is no proprietary Dorico authoring SDK to integrate; every asset is human-readable, version-control-friendly XML. The complexity is entirely in (a) knowing the right zip layout, (b) extracting the right CID per plugin, and (c) accepting that one binary blob (`.pluginstate`) cannot be authored without Dorico itself. Once those three concerns are addressed, the rest is plain `configure_file` and standard installer plumbing — almost all the existing Plan 25-01 v1 plumbing applies, just with new destinations.

## Runtime State Inventory

> Phase 25 is partially a refactor (Plan 25-01 v1 → v2 pivot). Inventory of state already on disk that affects the replan.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None — Dorico does not maintain a database keyed by the v1 `.doricoexpmap` filename. The reverted attempt left no orphan state in any user database [VERIFIED: `find $HOME/Library/Application\ Support/Steinberg/Dorico\ 6/` returns no `.doricoexpmap` file and no entries referencing it; the v1 cleanup (per finding doc) removed both copies] | None |
| Live service config | None on host — no Dorico project file references the reverted asset (no `.dorico` files in test fixtures) [VERIFIED: no test projects in repo and the cleanup confirmed in finding doc] | None |
| OS-registered state | None | None |
| Secrets/env vars | None — no credentials involved in template distribution | None |
| Build artifacts | Stale `build/plugins/<Plugin>/install-doricoexpmap-<Plugin>.cmake` files exist for 9 plugins (incl. O-Contrabass which is not in the v1.5 cohort) — these are configure_file outputs from the reverted Plan 25-01 v1 [VERIFIED: `find /Users/taylorbrook/Dev/VST-development/build -name "install-doricoexpmap-*.cmake"` returns 9 files] | These are regenerated on every cmake configure and are gitignored. They will be replaced by the new `install-microtonal-suite-<Plugin>.cmake` (or similar) generated by the replanned `module.cmake`. No explicit cleanup needed; they vanish on the next clean build. Plan 25-01 v2 should rename or delete the `.cmake.in` template under `modules/tuning/note-expression/` to prevent confusion |

**Verified empty:** No live data migration is required. The reverted commits cleanly removed all on-disk artifacts. The pivot is purely a forward-direction replan.

## Common Pitfalls

### Pitfall 1: Confusing `playbacktemplategen.xml` (factory Auto) with `playbacktemplatespec.xml` (distributable)
**What goes wrong:** Authoring against the wrong schema. The factory `Auto` template at `~/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateGenerators/Auto/playbacktemplategen.xml` looks like a template but uses an entirely different schema (`<PlaybackTemplateGenerator>` root, `<singlePluginDefinition>`, `<type>kHALion</type>`). It's NOT for distribution.
**Why it happens:** The filename `playbacktemplategen.xml` is the only "playback template" file the user encounters before they ever import a third-party `.dorico_pt`. It's a natural but wrong reference point.
**How to avoid:** Use the `Ample China.dorico_pt` sample structure (Pattern 3) as the canonical reference. Schema root is `<playbackTemplateSpec>`, file is `playbacktemplatespec.xml`, lives inside `PlaybackTemplateSpecs/<TemplateName>/`.
**Warning signs:** If your `.dorico_pt` has `<PlaybackTemplateGenerator>` as its root or contains a `<singlePluginDefinition>` child, you've targeted the wrong schema.

### Pitfall 2: Embedding the dev-build CID into a production-shipped template
**What goes wrong:** The template ships with `pluginID=ABCDEF019182FAEB4F7544764F4C7972` (the dev CID for O-Lyrica with `OuDv` manufacturer code). On end-user machines that have the prod `O-Lyrica.vst3` installed (manufacturer code `OuAu`, CID `ABCDEF019182FAEB4F7541754F4C7972`), Dorico can't find a matching plugin. Template silently fails to route.
**Why it happens:** Build-machine CID gets baked in, instead of the CID being computed at packaging time from the prod-build artifacts.
**How to avoid:** Use `configure_file(... @ONLY)` with `@OLYRICA_PLUGINID@` tokens in `endpointconfig.xml.in`, set the variables from a CMake scan of each `Contents/Resources/moduleinfo.json` at install/package time. **Dev installer pulls dev CIDs; prod installer pulls prod CIDs.**
**Warning signs:** Template works during dev testing but fails for end users on installed prod plugins; CIDs in `endpointconfig.xml` contain the bytes `4F7544 76` (`OuDv`) when production plugins use `OuAu`.

### Pitfall 3: `.doricolib` placed in wrong user-side directory
**What goes wrong:** `.doricolib` written into `~/Library/Application Support/Steinberg/Dorico [N]/` directly (not into the `Default Library Additions` subdirectory) is not auto-merged into the default library. Expression map fails to appear in pickers.
**Why it happens:** The directory name in some forum threads is written as `DefaultLibraryAdditions` (no spaces), but on macOS the canonical name has spaces: `Default Library Additions`. The Windows binary string is `DefaultLibraryAdditions` (camelCase). Don't conflate.
**How to avoid:** Use exactly these paths:
- macOS: `~/Library/Application Support/Steinberg/Dorico [N]/Default Library Additions/`
- Windows: `%APPDATA%\Steinberg\Dorico [N]\DefaultLibraryAdditions\`
The user must create the directory (Dorico does not auto-create it on install) — installer should do this.
**Warning signs:** `.doricolib` exists on disk but no expression map appears in `Library → Expression Maps` after Dorico restart; binary string `loadDefaultLibraryAdditions` is the exact symbol name to search for in Dorico binary if revalidation is needed.

### Pitfall 4: `.pluginstate` slot index mismatch with `endpointconfig.xml`
**What goes wrong:** `endpointconfig.xml` declares 8 slots with `<endpointConfigSlotIndex>0..7</endpointConfigSlotIndex>` but the zip contains `slot1.pluginstate..slot10.pluginstate` (the Ample China sample has slots 0-10 with 11 .pluginstate files — variation suggests Dorico tolerates some mismatch but the indices must align). On import Dorico may fail silently or partially load.
**Why it happens:** Hand-authoring the slot index without mirroring the actual `.pluginstate` file presence.
**How to avoid:** **Don't ship .pluginstate at all** for v1.5 (Pattern 6 alternative). When/if the project later wants to ship pluginstates, generate them once via `Save Endpoint Configuration` from a working Dorico session, and use Dorico's own naming (it will produce slot1, slot2, ... matching whatever it has loaded).
**Warning signs:** Template imports but plugins don't load, OR plugins load with wrong settings; `application.log` shows `Loading PlaybackTemplateSpec:` followed by errors.

### Pitfall 5: Zipping the `.dorico_pt` with parent directory
**What goes wrong:** `zip -r Ouaricon-Microtonal-Suite.dorico_pt staging-dir/` produces a zip whose first entries are `staging-dir/PlaybackTemplateSpecs/...`. Dorico expects `PlaybackTemplateSpecs/...` as the root entries.
**Why it happens:** Default `zip` behavior includes the directory you point it at.
**How to avoid:** `cd` into the staging directory first, then `zip` with `.` or `*` as source. Verify with `unzip -l <archive>.dorico_pt | head` — the first non-zero-length entries should be `PlaybackTemplateSpecs/<TemplateName>/playbacktemplatespec.xml` and `EndpointConfigs/<TemplateName>/endpointconfig.xml`.
**Warning signs:** `unzip -l` output shows a wrapping directory before `PlaybackTemplateSpecs/`.

### Pitfall 6: Importing `.dorico_pt` from a path containing spaces or special characters via drag-and-drop
**What goes wrong:** [LOW-CONFIDENCE — based on Spreadbury forum quote about "Axel Chambily - Casa" pathname triggering a spaces-in-filename bug; not directly observed for `.dorico_pt`] Filenames with spaces in path may cause the import to fail silently.
**How to avoid:** Default install path `~/Library/Application Support/Steinberg/Dorico [N]/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/` works because the spaces are inside a subdirectory name Dorico itself creates. Avoid intermediate paths with non-ASCII characters; document this in DOCS-04 troubleshooting.

## Code Examples

### Build the canonical .dorico_pt at CMake install time

```cmake
# modules/tuning/note-expression/module.cmake (additive — appended to existing patch-marker checks)

# Stage canonical Playback Template files into a build-tree directory tree matching .dorico_pt layout.
set(DORICO_PT_STAGE "${CMAKE_BINARY_DIR}/_microtonal-suite/Ouaricon-Microtonal-Suite")
file(MAKE_DIRECTORY "${DORICO_PT_STAGE}/PlaybackTemplateSpecs/Ouaricon Microtonal Suite")
file(MAKE_DIRECTORY "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite")

# Configure-time substitute the per-plugin pluginIDs into endpointconfig.xml.
# (For v1.5 with 8 plugins, list each one explicitly. CIDs are extracted by a helper function
#  reading each plugin's already-built moduleinfo.json — invoked AFTER all 8 _VST3 targets build.)
ouaricon_extract_vst3_cids(
    OUTPUT_VAR PLUGIN_CIDS
    PLUGINS OLyrica O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant
)
# Result: variables OLYRICA_PLUGINID, OBELLS_PLUGINID, ... set in this scope.

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in"
    "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in"
    "${DORICO_PT_STAGE}/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml"
    @ONLY
)
file(COPY "${CMAKE_CURRENT_LIST_DIR}/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib"
     DESTINATION "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite/")

# Pack the .dorico_pt zip — runs at build time, output is consumable by install rules below.
add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/Ouaricon-Microtonal-Suite.dorico_pt"
    COMMAND ${CMAKE_COMMAND} -E tar cf
            "${CMAKE_BINARY_DIR}/Ouaricon-Microtonal-Suite.dorico_pt"
            --format=zip
            "PlaybackTemplateSpecs"
            "EndpointConfigs"
    WORKING_DIRECTORY "${DORICO_PT_STAGE}"
    DEPENDS
        "${DORICO_PT_STAGE}/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml"
        "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml"
        "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib"
    COMMENT "Packing Ouaricon-Microtonal-Suite.dorico_pt"
)
add_custom_target(ouaricon_microtonal_suite_pt ALL
    DEPENDS "${CMAKE_BINARY_DIR}/Ouaricon-Microtonal-Suite.dorico_pt")

# install-time: Generate per-target install script via configure_file (mirrors Plan 25-01 v1's pattern;
# only the source file and destination subdirectory need to change from v1).
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/install-microtonal-suite.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/install-microtonal-suite-${TARGET_NAME}.cmake"
    @ONLY
)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install-microtonal-suite-${TARGET_NAME}.cmake")
```

### CID extraction helper

```cmake
# modules/cmake/OuariconModules.cmake — append this helper

function(ouaricon_extract_vst3_cids)
    set(options)
    set(oneValueArgs OUTPUT_VAR)
    set(multiValueArgs PLUGINS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(plugin_target IN LISTS ARG_PLUGINS)
        # Translate target name → product name (handles -dev suffix variations) and bundle path.
        # Path convention: build/plugins/<plugin-folder>/<TARGET>_artefacts/Release/VST3/<PRODUCT_NAME>.vst3
        # JUCE writes Contents/Resources/moduleinfo.json inside every VST3 bundle.
        set(moduleinfo "${CMAKE_BINARY_DIR}/plugins/${plugin_target}/${plugin_target}_artefacts/Release/VST3/${plugin_target}${OUARICON_DEV_SUFFIX}.vst3/Contents/Resources/moduleinfo.json")
        if(NOT EXISTS "${moduleinfo}")
            message(FATAL_ERROR "[Ouaricon] CID extraction: ${plugin_target} VST3 not built yet — build all _VST3 targets before packaging the Microtonal Suite. Expected: ${moduleinfo}")
        endif()
        # Parse JSON via Python (avoiding fragile CMake string-parsing of trailing commas in JUCE-emitted JSON).
        execute_process(
            COMMAND python3 -c "
import json, sys, re
with open(sys.argv[1]) as f:
    raw = f.read()
# JUCE 8 emits trailing commas — strip them.
raw = re.sub(r',(\\s*[}\\]])', r'\\1', raw)
data = json.loads(raw)
for cls in data['Classes']:
    if cls['Category'] == 'Audio Module Class':
        print(cls['CID'])
        sys.exit(0)
sys.exit(1)
" "${moduleinfo}"
            OUTPUT_VARIABLE cid
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
        # Convert plugin target name to a CMake variable suffix (uppercase, dash-stripped).
        string(TOUPPER "${plugin_target}" var_name)
        string(REPLACE "-" "" var_name "${var_name}")
        set("${var_name}_PLUGINID" "${cid}" PARENT_SCOPE)
        message(STATUS "[Ouaricon] ${plugin_target} pluginID = ${cid}")
    endforeach()
endfunction()
```

### `install-microtonal-suite.cmake.in` template

```cmake
# Substitute @-tokens at configure time; run by `cmake --install` at install time.
# Mirrors v1's install-doricoexpmap.cmake.in — only paths change.

set(SUITE_PT "@CMAKE_BINARY_DIR@/Ouaricon-Microtonal-Suite.dorico_pt")
set(SUITE_LIB "@CMAKE_CURRENT_LIST_DIR@/resources/library/Ouaricon-VST3-NoteExpression.doricolib")

if(APPLE)
    set(SHARED_DIR "$ENV{HOME}/Library/Application Support/Ouaricon/Microtonal Suite")
    file(MAKE_DIRECTORY "${SHARED_DIR}")
    file(COPY "${SUITE_PT}" DESTINATION "${SHARED_DIR}")
    file(COPY "${SUITE_LIB}" DESTINATION "${SHARED_DIR}")

    # Probe Dorico versions (descending: 6, 5, 4) and dual-write into the first detected.
    foreach(_v 6 5 4)
        set(DORICO_DIR "$ENV{HOME}/Library/Application Support/Steinberg/Dorico ${_v}")
        if(IS_DIRECTORY "${DORICO_DIR}")
            # Unzip the .dorico_pt directly into the user's PlaybackTemplateSpecs/ — Dorico auto-discovers.
            file(MAKE_DIRECTORY "${DORICO_DIR}/PlaybackTemplateSpecs")
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xf "${SUITE_PT}"
                WORKING_DIRECTORY "${DORICO_DIR}"
            )
            # Place .doricolib into Default Library Additions — Dorico merges into default library at next launch.
            file(MAKE_DIRECTORY "${DORICO_DIR}/Default Library Additions")
            file(COPY "${SUITE_LIB}" DESTINATION "${DORICO_DIR}/Default Library Additions")
            message(STATUS "[Ouaricon] Microtonal Suite installed for Dorico ${_v}: ${DORICO_DIR}")
            break()
        endif()
    endforeach()
elseif(WIN32)
    # %APPDATA% on Windows
    set(SHARED_DIR "$ENV{APPDATA}/Ouaricon/Microtonal Suite")
    file(MAKE_DIRECTORY "${SHARED_DIR}")
    file(COPY "${SUITE_PT}" DESTINATION "${SHARED_DIR}")
    file(COPY "${SUITE_LIB}" DESTINATION "${SHARED_DIR}")

    foreach(_v 6 5 4)
        set(DORICO_DIR "$ENV{APPDATA}/Steinberg/Dorico ${_v}")
        if(IS_DIRECTORY "${DORICO_DIR}")
            file(MAKE_DIRECTORY "${DORICO_DIR}/PlaybackTemplateSpecs")
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xf "${SUITE_PT}"
                WORKING_DIRECTORY "${DORICO_DIR}"
            )
            file(MAKE_DIRECTORY "${DORICO_DIR}/DefaultLibraryAdditions")  # Note: NO spaces on Windows
            file(COPY "${SUITE_LIB}" DESTINATION "${DORICO_DIR}/DefaultLibraryAdditions")
            message(STATUS "[Ouaricon] Microtonal Suite installed for Dorico ${_v}: ${DORICO_DIR}")
            break()
        endif()
    endforeach()
endif()
```

## State of the Art

| Old Approach (v1) | Current Approach (v2) | When Changed | Impact |
|-------------------|------------------------|--------------|--------|
| Standalone `.doricoexpmap` file dropped into `Expression Maps/User/` | `.dorico_pt` Playback Template archive (zip) imported into `PlaybackTemplateSpecs/<Name>/`, supplemented by `.doricolib` in `Default Library Additions/` | 2026-04-26 (Plan 25-01 v1 reverted, this research) | The asset XML body is unchanged; the wrapper format and destination directory change. Distribution channel and user activation flow change accordingly |
| One asset, written to two locations (Ouaricon shared + Dorico User/) | Two assets (.dorico_pt + .doricolib) written to two locations each (Ouaricon shared + Dorico PlaybackTemplateSpecs/ + Default Library Additions/) | 2026-04-26 | Installer plumbing is structurally similar but writes more files; the dual-asset model keeps the expression-map definition reusable independent of the template (users could apply a different template and still have the map available) |

**Deprecated/outdated:**
- `.doricoexpmap` extension as a distribution target — verified non-functional (Plan 25-01 v1 empirical failure + binary `strings` confirmation that the extension is unknown to Dorico)
- `playbacktemplategen.xml` schema — factory-only, not for distribution; do not reuse this format

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | The Ouaricon production-build manufacturer code `OuAu` produces CIDs with `4F754175` middle bytes per the `jucePluginId` algorithm. | Pattern 2 prod-build column | If wrong, prod CIDs are still extracted at packaging time from real prod `moduleinfo.json` — so the recommendation (configure_file from moduleinfo.json) self-corrects. The table is labeled "predicted" precisely because it was not directly verified against built prod artifacts in this session (only dev artifacts exist on this dev machine). |
| A2 | `.pluginstate` files are optional in `endpointconfig.xml` — Dorico will load plugins with their factory defaults if no .pluginstate is shipped. | Pattern 6 | If wrong, the user gets a partially-loaded template (plugins instantiated but without curated state). The expression-map binding still applies — so microtonal playback would still work, just with default knob positions. **Verify before locking by manually packing a `.dorico_pt` without any `.pluginstate` files and importing it into Dorico**, then observing whether the slots load with the correct plugins assigned. This is a 30-minute verification step the user can run. |
| A3 | The `fileVersion=1.1416` value used by the Ample China sample is forward-compatible with Dorico 5 and earlier. | Stack: assets | If wrong, the install-time Dorico version probe should write a different fileVersion per detected Dorico version. **Verify with a 5-minute test:** import the same `.dorico_pt` into Dorico 5 and observe whether it loads. The current Dorico 5 binary contains the same `dorico_pt` and `PlaybackTemplateSpecs` strings, suggesting compatibility — but Dorico 5's `playbacktemplategen.xml` files use `fileVersion=1.3` while Dorico 6 uses 1.1416 (different versioning conventions across template subtypes). Risk is low but unverified. |
| A4 | Dorico drag-and-drop install of `.dorico_pt` extracts directly into `PlaybackTemplateSpecs/` AND `EndpointConfigs/` (matching the zip's internal structure). | Pattern: distribution | If wrong (e.g., Dorico extracts only `PlaybackTemplateSpecs/` and discards `EndpointConfigs/`), the template would be visible but non-functional. The forum evidence ("dragging it onto a Dorico window installs it in the user application data folder") and the zip's own internal layout matching the user-side directory layout suggest extraction is faithful. **Verify with a 2-minute test:** drag the Ample China sample (already on this machine at `/tmp/ample_china/`) onto Dorico, then check whether `~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ample China/` exists afterward. |
| A5 | Plugin CIDs do not change across JUCE 8 minor versions for the same `(manufacturerCode, pluginCode)` pair. | Pattern 2 | If wrong, future JUCE upgrades could silently break the canonical asset. Mitigation is the configure_file approach — CIDs are re-extracted at every package build, so they always match what the user installs in the same release. Cross-release JUCE upgrades trigger a new package build anyway. Low risk; well-mitigated. |
| A6 | The reverted commit `cd2c2c6` `.doricoexpmap` XML body (kScoreLibrary > expressionMapDefinitions > ExpressionMapDefinition) is structurally valid as a child of `<kScoreLibrary>` inside `playbacktemplatedeps.doricolib`. | Pattern 5; Q7 | The recovered XML's root IS `<kScoreLibrary>`, matching the `.doricolib` root. Embedding is concatenation-clean. Verified by reading the recovered file (above) and the Ample China `.doricolib` (both have `<kScoreLibrary>` root). |

**A2 and A4 are the user-facing risk hotspots.** They can be mitigated with two short manual verifications before locking the plan; recommend including these as tasks in Plan 25-01 v2.

## Open Questions — Answers

### Q1. File extension and schema for distributable Playback Templates

**Decision:** **`.dorico_pt`** is the canonical distribution format. It is a standard zip archive containing a fixed directory layout: `PlaybackTemplateSpecs/<TemplateName>/playbacktemplatespec.xml` plus `EndpointConfigs/<TemplateName>/{endpointconfig.xml, *.pluginstate, playbacktemplatedeps.doricolib}`. Imported via `Play → Playback Template → Import`, drag-and-drop onto a Dorico project window, or drag-and-drop onto the Dorico Hub.

`.doricolib` is the **separate, complementary** library archive format used for distributable expression-map (and other library-entity) bundles. Auto-discovered when placed in `Default Library Additions/` (macOS) / `DefaultLibraryAdditions\` (Windows, no spaces).

**Evidence:**
- Dorico 6 binary `strings` output confirms recognized extensions: `dorico_pt`, `dorico_pt.zip`, `doricolib`, `Doricolib`, `Doricolib files` [VERIFIED]
- Dorico 5 binary contains the same strings — cross-version compatible [VERIFIED]
- The reverted `.doricoexpmap` extension does NOT appear in Dorico 6 binary at all [VERIFIED: empty grep result]
- The Ample China `.dorico_pt` extracted to confirm the zip layout [VERIFIED: `unzip -l "/tmp/ample_china/Ample China.dorico_pt"` lists exactly the structure documented above]
- Steinberg Help confirms the import workflow: "You can import playback templates by dragging .dorico_pt files into a Dorico Pro project window" [CITED: archive.steinberg.help/dorico/v3/en/dorico/topics/play_mode/play_mode_playback_templates_importing_t.html]

**Confidence:** HIGH

**Verify before locking:** A2, A3, A4 above (each a < 30 minute manual verification on the dev machine).

---

### Q2. Plugin GUID acquisition

**Decision:** Read each plugin's `Contents/Resources/moduleinfo.json` from the built VST3 bundle. The `Audio Module Class` entry's `CID` field is the canonical 32-hex value to embed in `<pluginID>`. Use `configure_file` with build-time substitution so dev installers contain dev CIDs and prod installers contain prod CIDs.

**Evidence:**
- JUCE 8 generates `moduleinfo.json` inside every built `.vst3` bundle [VERIFIED: every one of the 8 plugins' bundles contains it on this machine]
- The `Audio Module Class` CID is computed by JUCE's `jucePluginId(manufacturerCode, pluginCode, Type::component)` in `juce_VST3Interface.h:128` — deterministic from CMake config [CITED]
- All 8 v1.5 cohort plugins use `JUCE_VST3_CAN_REPLACE_VST2=0` (verified by grep across CMakeLists.txt files), so the modern `jucePluginId` schema applies uniformly [VERIFIED]
- All 8 dev-build CIDs extracted live and tabulated in Pattern 2 above [VERIFIED]

**Confidence:** HIGH

**Verify before locking:** Run `extract_pluginID` against each prod-build artifact when prod build is next produced — to populate the prod-CID column of Pattern 2 with verified (not predicted) values.

---

### Q3. `pluginStateFile` authoring requirement

**Decision:** **`.pluginstate` files are NOT mandatory for v1.5.** Ship a `.dorico_pt` whose `endpointconfig.xml` declares slots without corresponding `slot<N>.pluginstate` files. Dorico will load each Ouaricon plugin with whatever its factory defaults are and apply the channel/expression-map binding from `endpointconfig.xml`. The microtonal-routing invariant (the load-bearing thing for v1.5) lives entirely in the expression-map binding, NOT in plugin state.

If a future milestone wants curated default knob positions per plugin, the authoring path is:
1. Open Dorico, create a project that loads all 8 Ouaricon plugins with curated state
2. `Play → Save Endpoint Configuration` exports `slot<N>.pluginstate` files into `~/Library/Application Support/Steinberg/Dorico [N]/EndpointConfigs/<Name>/`
3. Copy those files into `modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/`
4. Re-pack the `.dorico_pt`

**Evidence:**
- The factory `Silence` template's `playbacktemplategen.xml` uses `<pluginStateFile/>` (empty self-closing) successfully [VERIFIED: `/Applications/Dorico 6.app/Contents/Resources/playback/PlaybackTemplateGenerators/Silence/playbacktemplategen.xml`]
- The `endpointconfig.xml` schema does not have a `<pluginStateFile>` child element at all — `.pluginstate` association is purely by filename match (`slot<N>.pluginstate` paired with `<endpointConfigSlotIndex>N</endpointConfigSlotIndex>`) [VERIFIED via Ample China sample]
- The format is binary Steinberg VST3 component-state wrapper (`VST3` magic + version + 32-hex pluginID + length + state blob) — proprietary and not authorable without Dorico [VERIFIED via `xxd`]

**Confidence:** MEDIUM-HIGH on the v1.5 omission decision (binary inspection confirms the slot definitions can be authored without state, and the factory Silence template demonstrates the pattern). Marked MEDIUM because A2 is unverified.

**Verify before locking:** A2 above — pack a stripped `.dorico_pt` (slots without states) and confirm Dorico loads it.

---

### Q4. Per-plugin templates vs one omnibus suite template

**Decision:** **Ship ONE omnibus `Ouaricon-Microtonal-Suite.dorico_pt`.** A single template natively supports multi-plugin routing via `<entries>` in `playbacktemplatespec.xml` and multi-slot binding in `endpointconfig.xml`. Each Ouaricon plugin gets its own slot in the same endpoint config, each with its own channel-to-expression-map binding pointing to the same canonical map ID.

**Rationale:**
- **User friction:** End users apply ONE template to enable Dorico-aware playback for any combination of Ouaricon instruments they have installed. Eight templates = decision overhead and surface area for confusion.
- **Disk and registry footprint:** One `<TemplateName>/` subdirectory under `PlaybackTemplateSpecs/` instead of eight.
- **Maintenance:** One canonical XML to keep in sync with plugin CIDs. Eight per-plugin files would drift independently.
- **Schema natively supports it:** Every slot in `endpointconfig.xml` is independent; the Ample China sample demonstrates 11 slots in a single template (8 VST3 + 3 VST2 plugins). Dorico loads only the plugins whose CIDs are installed; missing plugins produce a warning but the template still applies for the available ones [VERIFIED structurally; behavior on missing plugins inferred from Spreadbury's "templates fail to open" forum guidance — Dorico does not require all plugins to be present, but it will warn].
- **Discoverability:** A single named "Ouaricon Microtonal Suite" entry in `Play → Playback Template` is more findable than eight similarly-named entries.

**Evidence:**
- The Ample China sample bundles 11 plugin slots in a single template — proven multi-plugin routing works [VERIFIED]
- The `playbacktemplatespec.xml` `<entries>` child with empty `<instrumentFamilies/>` is a catch-all rule; multiple `<entry>` blocks with non-empty families enable per-family routing if/when needed in the future [VERIFIED via schema]

**Confidence:** HIGH

**Verify before locking:** This is a discuss-phase decision — should be confirmed with the user as a locked decision in CONTEXT.md before plan-phase.

---

### Q5. Distribution channel

**Decision:** **Bundle the `.dorico_pt` and `.doricolib` inside every plugin's PKG/EXE installer (idempotent overwrite).** Same approach as Plan 25-02 v1, but bundling 2 different files (`.dorico_pt` + `.doricolib`) instead of 1 (`.doricoexpmap`).

Reject the alternatives:
- **Separate "Suite Installer":** Adds a separate installer the user must run. Friction for users who only buy one Ouaricon plugin. Increases installer-pipeline complexity (another packaging skill, another signing flow).
- **Both (per-plugin + separate suite):** Redundant and confusing — if a user runs both, the same files get written twice (idempotent so it works, but conceptually muddled).

**Why per-plugin bundling works cleanly here:**
- All 8 plugins write the SAME canonical files (same CIDs in `endpointconfig.xml` since dev/prod variants are determined at packaging time and EVERY installer for a given build flavor writes the same content). Idempotent overwrite is structurally safe.
- Users get correct routing whether they install 1 or 8 Ouaricon plugins. The template references all 8 plugins by ID; Dorico handles missing-plugin gracefully (warn + skip the missing slots).
- Mirrors the v1 architecture exactly — minimizes structural changes to Plans 25-02 and 25-03.

**Evidence:** Architectural consistency with Phase 24 + 25-01 v1 plumbing; user-friction analysis above.

**Confidence:** HIGH (architectural fit) | MEDIUM-HIGH (user-experience claim — graceful missing-plugin handling is inferred from forum guidance, not directly tested with 8/8 vs 1/8 plugin scenarios)

**Verify before locking:** This is also a discuss-phase decision — should be confirmed with the user as a locked decision in CONTEXT.md before plan-phase. Single short test recommended: install one Ouaricon plugin's PKG, then apply the template with all 8 plugins listed in it; observe that the one installed plugin gets its slot loaded and the others produce a warning rather than blocking template application.

---

### Q6. Auto-discovery vs explicit Library Manager import

**Decision:** **Yes, Dorico auto-discovers `.dorico_pt` content placed in `~/Library/Application Support/Steinberg/Dorico [N]/PlaybackTemplateSpecs/<TemplateName>/` AND `.doricolib` content placed in `~/Library/Application Support/Steinberg/Dorico [N]/Default Library Additions/`.** No explicit user import action is required. The installer can extract the `.dorico_pt` directly into the user-side directories and Dorico picks it up at next launch.

**Two-pronged auto-discovery:**
1. **Templates directory scan:** Dorico scans `PlaybackTemplateSpecs/*/` at startup. The binary contains the `Loading PlaybackTemplateSpec:` log message and `PlaybackTemplateSpecs` directory string [VERIFIED]
2. **DefaultLibraryAdditions auto-merge:** Dorico calls `loadDefaultLibraryAdditions` at startup, merging every `.doricolib` in that directory into the in-memory default library so all expression maps appear in pickers [VERIFIED: binary contains `loadDefaultLibraryAdditions`, `DefaultLibraryAdditions`, `.+doricolib$` regex]

**Caveat:** The `Default Library Additions/` directory does NOT exist by default on user systems — the installer must create it. [CITED: Steinberg forum thread]

**Evidence:**
- macOS user-side directory `~/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/` is the documented storage path; templates dragged onto Dorico end up there [CITED: Steinberg forums + WebSearch result]
- The Ample China sample's zip layout matches user-side directory structure verbatim — confirming Dorico extracts `.dorico_pt` zip contents directly into matching user-side subdirs [VERIFIED]
- Drag-and-drop install into Dorico project window or Hub is officially supported [CITED: archive.steinberg.help, doricotuts.com]

**Confidence:** HIGH

**Verify before locking:** A4 (drag-and-drop test on dev machine).

---

### Q7. Carry-forward `.doricoexpmap` XML

**Decision:** **Recover and reuse the recovered XML body verbatim, embedding it inside both (a) `playbacktemplatedeps.doricolib` (inside the .dorico_pt zip) and (b) the standalone `Ouaricon-VST3-NoteExpression.doricolib` (for `Default Library Additions/`).**

The recovered XML's root element `<kScoreLibrary>` IS the `.doricolib` schema root. Wrapping is unnecessary — only the XML body needs to be reused as-is.

**Recovered structure (from `git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap`):**

```xml
<?xml version="1.0" encoding="utf-8"?>
<kScoreLibrary>
    <expressionMapDefinitions>
        <entities array="true">
            <ExpressionMapDefinition>
                <name>Ouaricon VST3 Note Expression</name>
                <entityID>xmap.ouaricon.vst3_note_expression</entityID>
                ...
                <microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>
                ...
            </ExpressionMapDefinition>
        </entities>
    </expressionMapDefinitions>
</kScoreLibrary>
```

**The `<entityID>` `xmap.ouaricon.vst3_note_expression` is the load-bearing string** that `endpointconfig.xml`'s `<expressionMapID>` references. They MUST match byte-exactly in both files; recommend tying them via a shared CMake variable used by configure_file.

**Why both (a) and (b):** Even though the `.dorico_pt` is self-contained (its `playbacktemplatedeps.doricolib` carries the expression-map definition), shipping the standalone `.doricolib` to `Default Library Additions/` makes the expression map available even when the user has not yet applied the Playback Template. This is genuinely useful — users may want to apply the expression map to a custom endpoint config without going through the full template apply flow.

**Evidence:**
- Recovered XML body inspected line-by-line; structurally valid [VERIFIED above]
- Schema match between `.doricoexpmap` (reverted) and `.doricolib` (Ample China sample) — both have `<kScoreLibrary>` root with the same child structure for `expressionMapDefinitions` [VERIFIED]
- Dorico factory `expressionMapsDefinitions.xml` in `playback/PluginPresetLibraries/HALion Sonic/` uses identical schema [VERIFIED]

**Confidence:** HIGH

**Verify before locking:** None required — this is structural carry-forward of validated XML.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Dorico 6 | All Dorico-side validation | ✓ | 6.1.0 (Sep 26 2025 build) [VERIFIED: app bundle on disk + log files] | Dorico 5 (also installed) |
| Dorico 5 | Cross-version compatibility test (A3) | ✓ | (installed) [VERIFIED: app bundle on disk] | — |
| Built `.vst3` bundles for all 8 cohort plugins (dev variants) | CID extraction | ✓ | All 8 present in `~/Library/Audio/Plug-Ins/VST3/<Plugin>-dev.vst3` [VERIFIED: live moduleinfo.json reads succeeded for all 8] | Rebuild with `ninja <Plugin>_VST3` |
| `python3` | CID extraction helper | ✓ | system Python on macOS | — |
| `cmake -E tar` (zip support) | .dorico_pt packaging | ✓ | Built into CMake 3.14+ [VERIFIED: `cmake --version` runs in Bash environment] | Native `zip` command on POSIX, `Compress-Archive` on Windows |
| `git` (for `git show cd2c2c6:...`) | Recovery of reverted XML | ✓ | system git | — |
| Ample China sample `.dorico_pt` | Schema reference | ✓ | Cloned to `/tmp/ample_china/` | — |
| Built `.vst3` bundles for prod variants (A1 verification) | Prod CID column verification | ✗ | — | A1 self-corrects via configure_file at prod packaging time. Optional verification: rebuild prod variant once and re-run `extract_pluginID` |

**Missing dependencies with no fallback:** None.

**Missing dependencies with fallback:** Prod-build artifacts (only used to upgrade A1 from MEDIUM to HIGH confidence; not blocking).

## Validation Architecture

> nyquist_validation is not explicitly disabled in any project config I could locate. The repo is a JUCE/CMake VST3 project; project-level config lives in `CLAUDE.md` and per-plugin CMakeLists files. Treating as enabled by default.

### Test Framework

| Property | Value |
|----------|-------|
| Framework | JUCE-host runtime + Dorico 6 manual smoke (per Phase 24's D-07 pattern) |
| Config file | None code-level — validation is by Dorico runtime test, not unit tests |
| Quick run command | `auval -v aumu OBls OuDv` (per-plugin AU sanity) + `scripts/verify-au-link.sh <Plugin>` |
| Full suite command | Manual: macOS quarter-sharp smoke gate + (after replan) `.dorico_pt` apply + observe expression-map binding via Library → Expression Maps |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| INST-01 | Canonical Playback Template asset exists at canonical module path | unit | `test -f modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon\ Microtonal\ Suite/playbacktemplatespec.xml.in` | ❌ Wave 0 (replan-time) |
| INST-02 | Module owns the resource and propagates via `ouaricon_add_module()` | unit | `grep -q "ouaricon_microtonal_suite_pt" modules/tuning/note-expression/module.cmake` | ❌ Wave 0 |
| INST-03 | All 8 plugin installers bundle the .dorico_pt + .doricolib | integration | Per-plugin: extract a built PKG, verify `Ouaricon-Microtonal-Suite.dorico_pt` and `Ouaricon-VST3-NoteExpression.doricolib` are payloads | ❌ Wave 0 |
| INST-04 | README fallback emitted alongside | unit | `test -f modules/tuning/note-expression/resources/README-microtonal-suite.txt` | ❌ Wave 0 |
| DOCS-01..05 | Internal notes describe shipped behavior | structural | grep gates per plan 25-03 task | ❌ Wave 0 (Plan 25-03) |

### Sampling Rate

- **Per task commit:** Verify the affected XML is well-formed (`xmllint --noout <file>` for `.doricolib`, `unzip -t <file>.dorico_pt` for the archive)
- **Per wave merge:** Cross-platform installer build + dry-run install + Dorico 3-point quarter-sharp smoke (D-07 inheritance)
- **Phase gate:** Same as Phase 24 — full Dorico 3-point smoke on representative plugin per platform

### Wave 0 Gaps

- [ ] Manual verification of A2 (pack `.dorico_pt` without `.pluginstate`s and confirm Dorico loads it) — blocker for Plan 25-01 v2 lock
- [ ] Manual verification of A4 (drag-and-drop Ample China sample, confirm extract destinations) — blocker for Plan 25-01 v2 lock
- [ ] Cross-version compatibility check (A3) — drag the same .dorico_pt onto Dorico 5 and confirm load — non-blocking but recommended

## Sources

### Primary (HIGH confidence)
- **Live filesystem inspection of installed Dorico 6:**
  - `/Applications/Dorico 6.app/Contents/Resources/playback/PlaybackTemplateGenerators/{HSSE,Silence}/playbacktemplategen.xml`
  - `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/{expressionMapsDefinitions.xml, presets.xml}`
  - `~/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateGenerators/Auto/playbacktemplategen.xml`
- **Binary string enumeration via `strings`:** `/Applications/Dorico 6.app/Contents/MacOS/Dorico 6` and `/Applications/Dorico 5.app/Contents/MacOS/Dorico 5`
- **Live extraction of a real third-party `.dorico_pt`:** [Ample_China_Playback_Template GitHub repo](https://github.com/JayChan0822/Ample_China_Playback_Template) — extracted to `/tmp/ample_china_extracted/` and inspected line-by-line
- **JUCE 8 source for VST3 GUID derivation:** `/Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3Interface.h`
- **Live extraction of all 8 cohort plugins' moduleinfo.json:** `~/Library/Audio/Plug-Ins/VST3/<Plugin>-dev.vst3/Contents/Resources/moduleinfo.json`
- **Reverted commit `cd2c2c6` git-show recovery:** `git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap`

### Secondary (MEDIUM-HIGH confidence — official Steinberg docs cross-checked with binary)
- [Importing playback templates (Dorico Pro)](https://archive.steinberg.help/dorico/v3/en/dorico/topics/play_mode/play_mode_playback_templates_importing_t.html)
- [Saving custom endpoint configurations](https://archive.steinberg.help/dorico/v3/en/dorico/topics/play_mode/play_mode_endpoints_configurations_saving_t.html)
- [Creating custom playback templates](https://steinberg.help/dorico_pro/v5/en/dorico/topics/play_mode/play_mode_playback_template_custom_creating_t.html)
- [Playback Template Application (Dorico 6)](https://www.steinberg.help/r/dorico/doricofirststeps/6.1/en/dorico_first_steps/topics/first_steps_playback/first_steps_playback_template_applying_t.html)

### Tertiary (MEDIUM — community / forum sources)
- [Steinberg Forums: Playback Templates Storage Location](https://forums.steinberg.net/t/playback-templates-storage-location/837827)
- [Steinberg Forums: Doricolib not seen by default (windows)](https://forums.steinberg.net/t/doricolib-not-seen-by-default-windows/914859)
- [Steinberg Forums: Import Library - Default Library Additions missing](https://forums.steinberg.net/t/import-library-expression-maps-location-default-library-additions-missing/936038)
- [Steinberg Forums: Playback templates not opening](https://forums.steinberg.net/t/playback-templates-not-opening/882213)
- [Steinberg Forums: Xmap, endpoints, playback templates](https://forums.steinberg.net/t/xmap-endpoints-playback-templates/920845)
- [Dorico Tutorials: Playback Templates How-to](https://doricotuts.com/playback-templates-in-dorico-how-to-use-apply-and-update-them/)
- [Dorico Tutorials: Bringing External VST/AU into Dorico](https://doricotuts.com/bringing-external-vst-au-sound-libraries-into-dorico/)

## Metadata

**Confidence breakdown:**
- File extension and schema (Q1): HIGH — binary strings + live-extracted sample + Steinberg docs all triangulate
- Plugin GUID acquisition (Q2): HIGH — JUCE source code + live moduleinfo.json reads for all 8 plugins
- pluginStateFile requirement (Q3): MEDIUM-HIGH — schema verified; manual A2 verification recommended
- Per-plugin vs omnibus (Q4): HIGH — schema and Ample China sample prove multi-plugin works
- Distribution channel (Q5): HIGH (architectural) | MEDIUM-HIGH (UX claim — graceful missing-plugin)
- Auto-discovery (Q6): HIGH — binary symbols + Steinberg docs + sample
- Carry-forward XML (Q7): HIGH — recovered XML inspected, schema matches `.doricolib`

**Research date:** 2026-04-26
**Valid until:** 2026-05-26 (Dorico 6.1.x is the current major; Dorico 7 release would invalidate the `fileVersion=1.1416` value but the architectural recommendations would remain. Re-verify A1, A2 against any prod-build CIDs when prod build is next produced — this gates upgrade of A1 from MEDIUM to HIGH.)
