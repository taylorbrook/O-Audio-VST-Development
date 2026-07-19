# Quick Task 260719-k5o: Re-base JUCE-NE-PATCH onto JUCE 8.0.14 - Context

**Gathered:** 2026-07-19
**Status:** Ready for planning

<domain>
## Task Boundary

Re-base the note-expression vendored patch (JUCE-NE-PATCH) onto JUCE 8.0.14, per the concrete procedure in `research/framework-updates-2026-07.md` §Risk Assessment:

1. Re-vendor pristine 8.0.14 copies of `juce_audio_plugin_client_VST3.cpp` (re-stitch NE block at the surviving `toMidiBuffer` anchor) and `juce_VST3ClientExtensions.h` at its NEW location `modules/juce_audio_processors_headless/utilities/` (decide if the new companion `.cpp` needs patching).
2. Update `vendored/JUCE-overrides/` layout, the CI `cp -R` copy step, and both JUCE-NE-PATCH grep-gate paths in `.github/workflows/build-and-release.yml`.
3. Rename `scripts/juce-patches/note-expression-juce-8.0.4.patch` to its true base and regenerate against 8.0.14.

**Out of scope:** Do NOT bump the local JUCE install (`/Users/taylorbrook/JUCE`) or CI `JUCE_VERSION`. Work happens on dedicated branch `quick/260719-k5o-juce-ne-rebase-8014`.

</domain>

<decisions>
## Implementation Decisions

### Vendored layout staging
- **Replace in place.** The branch carries the 8.0.14 layout under `vendored/JUCE-overrides/` and the repointed CI paths. The branch is intentionally unmergeable until the JUCE_VERSION bump follows — CI on this branch is broken-by-design against 8.0.9.

### Old patch file
- **Keep both, renamed.** Rename `note-expression-juce-8.0.4.patch` → `note-expression-juce-8.0.9.patch` (its true base) and add `note-expression-juce-8.0.14.patch` regenerated against pristine 8.0.14. Local 8.0.9 dev patching keeps working; the 8.0.9 patch gets deleted when the JUCE bump lands.

### Verification depth
- **Compile-check against scratch 8.0.14.** Download pristine JUCE 8.0.14 to a scratch directory (NOT `/Users/taylorbrook/JUCE`), apply the re-based overrides, and build one note-expression plugin against it via a temporary CMake configure. Proves the re-stitched code actually compiles, not just that the patch applies.

### Claude's Discretion
- Whether the new companion `juce_VST3ClientExtensions.cpp` in the headless module needs patching (decide by inspecting the 8.0.14 file against what the NE patch touches).
- Choice of NE plugin for the compile-check (O-Lyrica is the validated NE spike reference).
- Scratch-dir mechanics for the 8.0.14 download and temp build.

</decisions>

<specifics>
## Specific Ideas

- NE insertion anchor in the `.cpp`: `if (isMidiInputBusEnabled && data.inputEvents != nullptr)` immediately before `MidiEventList::toMidiBuffer` — survives verbatim in 8.0.14 (lines ~3590–3591), not inside a churned hunk.
- `.h` patch content: `struct Vst3RawEvent` + virtual hook inside `struct VST3ClientExtensions`, now at `modules/juce_audio_processors_headless/utilities/`.
- CI touchpoints: `build-and-release.yml` copy step at ~:102/:451 and grep gates at ~:104-105/:453-454 (verify live line numbers).
- The JUCE-NE-PATCH marker string must survive in both vendored files (CI grep gate).

</specifics>

<canonical_refs>
## Canonical References

- `research/framework-updates-2026-07.md` §Risk Assessment item 1 (concrete re-base procedure) and §Verification Strategy.
- Memory: `critical_juce_vendor_overrides_for_ci.md` — vendored files are copied over fresh JUCE downloads in CI; `apply-juce-patches.sh` is local-dev only.

</canonical_refs>
