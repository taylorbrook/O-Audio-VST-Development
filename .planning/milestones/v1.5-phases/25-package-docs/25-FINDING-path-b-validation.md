---
phase: 25-package-docs
date: 2026-04-27
status: open
type: architectural-finding
supersedes_plan: 25-01-author-and-plumbing-PLAN.md (v2)
related_finding: 25-FINDING-playback-template-pivot.md (v1 → v2 pivot)
test_log: 25-01-WAVE-0-VERIFICATION.md (A2 + Path B sections)
---

# Phase 25 Finding — Path B (standalone .doricolib) supersedes Path A (Playback Template)

## What was tried (Plan 25-01 v2)

Author a Dorico Playback Template (`.dorico_pt` archive containing `playbacktemplatespec.xml`, `endpointconfig.xml` with 8 plugin slots, `playbacktemplatedeps.doricolib`) plus a standalone `.doricolib` library file. CMake plumbing in `modules/tuning/note-expression/module.cmake` packs the `.dorico_pt` zip via `cmake -E tar`, runs `configure_file @ONLY` to substitute per-plugin CIDs into `endpointconfig.xml`, and dual-installs both assets to Ouaricon's canonical path AND Dorico's auto-discovery directories (`PlaybackTemplateSpecs/`, `EndpointConfigs/`, `Default Library Additions/`). Per-plugin install component named `ouaricon_note_expression_<TARGET>`.

End-to-end design: user installs any Ouaricon plugin → `cmake --install` lands the suite resources in Dorico's directories → Dorico auto-discovers them → user opens `Play → Playback Template`, picks "Ouaricon Microtonal Suite", applies. Template loads all 8 plugins on auto-assigned channels with the microtonal expression map pre-bound.

## Why it failed

Two concrete bugs and one architectural mismatch.

### Bug 1: invalid `.doricolib` format

The recovered XML body at `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` (and its byte-exact embedded copy at `…/playbacktemplate/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in`) is structured as:

```xml
<kScoreLibrary>
  <expressionMapDefinitions>
    <entities array="true">
      <ExpressionMapDefinition>…</ExpressionMapDefinition>
    </entities>
  </expressionMapDefinitions>
</kScoreLibrary>
```

A factory-valid Dorico `.doricolib` (compared against `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml`) requires **all 48 top-level library containers** as siblings — `<temperaments>`, `<accidentalSystems>`, `<accidentalDefinitions>`, `<tonalitySystemDefinitions>`, `<ensembles>`, `<instruments>`, `<instrumentNames>`, `<instrumentFamilies>`, …, `<expressionMapDefinitions>`, …, `<lineStyleCollectionDefinition>` — even if all but the relevant one are empty (`<entities array="true"/>`).

Dorico 6's Library Manager rejects the truncated form with "Error opening file: invalid file format". The recovered cd2c2c6 XML body is an expression-map definition *fragment*, not a complete library bundle. Plan 25-01 v2's D-03 directive ("recover, do not re-author") inherited this defect from the v1 reverted asset without noticing — neither v1 nor v2 actually tested the file's import-validity in Dorico (v1 tested the wrong path; v2 deferred the canary).

### Bug 2: dev/prod plugin-name divergence

`endpointconfig.xml.in` substitutes `@OLYRICA_PLUGINID@`, `@OBELLS_PLUGINID@`, … from the `ouaricon_extract_vst3_cids` helper, which honors `OUARICON_DEV_SUFFIX` (so dev installers get dev CIDs and prod installers get prod CIDs). But `<pluginName>` is hard-coded to the prod names ("O-Lyrica", "O-Bells", …). Dorico's slot-resolution requires name+CID match — verified by inspecting Ample China's `endpointconfig.xml`, where `<pluginName>` is the bundle's exact advertised name (`Ample China Qudi`, etc., matching the moduleinfo.json `Audio Module Class > Name` field).

On a dev machine, applying the Microtonal Suite Playback Template silently fails to load any plugin slots: the prod `<pluginName>` ("O-Lyrica") doesn't match the dev VST3's advertised name ("O-Lyrica-dev" — confirmed via `moduleinfo.json: Classes[].Name`).

### Architectural mismatch: Playback Template is over-engineered

A `.dorico_pt` Playback Template is Steinberg's primitive for "one-click full project setup": it loads VST3 plugins on specific channels AND assigns expression maps AND optionally provides curated `.pluginstate` snapshots. The Ample China model is the canonical example — pick the template and a complete sample-library project is configured.

The v1.5 microtonal-routing use case is narrower. A user already loads Ouaricon plugins in their Dorico projects via the normal `Play → Endpoints → Add Plug-in` flow. They don't want a template that replaces their endpoint setup; they want the **microtonal expression map** available so they can assign it to their already-loaded plugin's channel.

## What works (Path B — validated 2026-04-27)

Distribution: ship a single Dorico-valid `.doricolib` to `~/Library/Application Support/Steinberg/Dorico 6/Expression Maps/User/`. (Or the user explicit-imports it once via `Library → Library Manager → Import…` if they prefer.)

User flow:

1. User installs any Ouaricon plugin via PKG/EXE installer; the installer lands the `.doricolib` in Dorico's auto-discovery path.
2. User restarts Dorico (or imports the library on demand).
3. User loads any Ouaricon plugin in their project via the normal Dorico flow (`Play → Endpoints → Add Plug-in`).
4. User assigns the "Ouaricon VST3 Note Expression" expression map to the plugin's channel via `Play → Endpoints → Expression Map` dropdown.
5. Quarter-sharp accidentals on any pitch play at the correct microtonal frequency via VST3 Note Expression routing.

Verified end-to-end on macOS Dorico 6 with O-Lyrica-dev: quarter-sharp C4 plays at ~269 Hz (between C4 = 261.63 Hz and C♯ = 277.18 Hz). See `25-01-WAVE-0-VERIFICATION.md` `## A2 Result` section for the full test log.

### Carry-forward

- The recovered `<ExpressionMapDefinition>` element from cd2c2c6 — its content (kVST3NoteExpression microtonal method, xmap.ouaricon.vst3_note_expression entityID, the technique combination block) is correct and load-bearing. Only the wrapping skeleton needs replacement.
- The `module.yaml` v1.0.0 → v1.1.0 bump and `registry.yaml` entry — reusable as-is.
- The README.md "Dorico End-User Setup" rewrite — needs revision for Path B's manual-assign flow but the structure is reusable.
- The Dorico-version probe pattern in `install-microtonal-suite.cmake.in` (descending 6 → 5 → 4) — reusable.
- The user-facing fallback README pattern (`README-microtonal-suite.txt`) — reusable shell, content needs Path B rewrite.

### Trash

- `module.cmake` Microtonal Suite block's `cmake -E tar` packing logic — `.dorico_pt` is no longer shipped.
- `ouaricon_extract_vst3_cids` helper in `OuariconModules.cmake` — its only consumer was endpointconfig CID substitution; Path B doesn't need plugin CIDs in the asset at all.
- `install-microtonal-suite.cmake.in`'s dual-write logic — only one asset to one path.
- The three `.xml.in` template files under `playback-template/` — entirely unneeded.
- The `playbacktemplatedeps.doricolib.in` — the standalone `.doricolib` IS the distribution unit; no embedded copy needed.

### Optional UX win

Populate the `<pluginNames>` array in the `.doricolib`'s `<ExpressionMapDefinition>` with all 8 Ouaricon plugin advertised names (e.g. `<entry>O-Lyrica</entry>`, `<entry>O-Lyrica-dev</entry>`, …). Dorico uses this list to **auto-suggest** the expression map when the user loads any of those plugins. Reduces step 4 from "scroll the dropdown looking for the right map" to "Dorico pre-fills it." This requires verifying `<pluginNames>` schema with a Dorico binary string check or sample factory file, but is purely additive.

## Open questions for replan

1. **Filename and path strategy.** Confirm `~/Library/Application Support/Steinberg/Dorico 6/Expression Maps/User/<file>.doricolib` is the canonical user-extension path on Dorico 6. Verify Windows equivalent (`%APPDATA%\Steinberg\Dorico 6\Expression Maps\User\` based on Plan 25-01 v1 conventions; needs reverification under Path B).
2. **Auto-discovery vs explicit import.** The Library Manager `Import…` flow worked. Auto-discovery on next launch was not tested with the wrapped (valid) file — needs a separate test pass to confirm whether Dorico picks up `.doricolib` files from `Expression Maps/User/` automatically at startup.
3. **`<pluginNames>` schema.** Verify the exact element name and entry format for auto-suggestion. Sample from a factory `.doricolib` that ships with `<pluginNames>` populated, or check Dorico binary strings.
4. **Dev install UX.** With dev-suffix builds, the plugin advertises as `O-Lyrica-dev`. If `<pluginNames>` is populated for prod names only, dev users won't get auto-suggestion. Either ship dev names too OR document that auto-suggestion is prod-only.
5. **Plan 25-02 simplification.** Installer scripts (PKG postinstall, Inno Setup template) need to bundle one file instead of two and dual-write to one path instead of two.
6. **Plan 25-03 refocus.** Internal docs need to describe Path B's user flow, not Path A's "apply Playback Template" flow.

## Recommended replan path

1. **`/clear`** — drop execution context.
2. **`/gsd-discuss-phase 25 --replan`** — capture Path B as the locked architecture; surface the open questions above as gray areas to resolve.
3. **`/gsd-plan-phase 25`** — replan with three new plans:
   - 25-01 v3: author Dorico-valid `.doricolib` (full skeleton, populated expression-map block) + simplified module.cmake plumbing (single-asset dual-write only) + Path B README + Wave 0 retest of auto-discovery.
   - 25-02: simplified installer sweep (1 file, 1 dest path) — much smaller scope.
   - 25-03: internal docs reframed for Path B's manual-assign flow.

## What stays from commit `819b2b4`

The merged work is not a total loss. Once Plan 25-01 v3 lands, these pieces survive:
- `module.yaml` v1.1.0 + `registry.yaml` entry (no change needed).
- `README.md`'s "Dorico End-User Setup" structure (content rewrite, structure preserved).
- `install-microtonal-suite.cmake.in`'s Dorico-version probe (logic preserved, dual-write of two files collapsed to single-write of one).
- The recovered `<ExpressionMapDefinition>` body (re-wrapped in a valid `.doricolib` skeleton).

Whether to revert commit `819b2b4` cleanly and reland in a single v3 plan, or to amend forward in v3 plan tasks, is a planning decision for the next replan cycle. Cleaner audit trail favors a revert; faster execution favors amend-forward.

## Reference: a Dorico-valid `.doricolib` skeleton

See `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` (6,431 B) for a reference implementation built during this finding's verification. Generated by:

1. Loading the HALion Sonic factory `expressionMapsDefinitions.xml` from the Dorico app bundle (provides the full 48-container kScoreLibrary skeleton).
2. Replacing its `<expressionMapDefinitions>/<entities>` body with the recovered Ouaricon `<ExpressionMapDefinition>` element.
3. Writing as `.doricolib`.

Confirmed by Dorico 6 Library Manager → Import: success. Expression map appeared in `Play → Endpoints` dropdown. Quarter-sharp playback verified.

The Plan 25-01 v3 task that authors the canonical `.doricolib` should follow this pattern: bootstrap from the factory skeleton, inject the recovered expression map, never re-author the skeleton from scratch.
