# Framework-Update Evidence — 260719-hn4

Working evidence file for `research/framework-updates-2026-07.md`. All upstream content
was fetched at **pinned version tags** (never branch tips), read/diffed only, never
compiled or executed (threat T-hn4-01). Fetched 2026-07-19.

Source pins confirmed in-repo:
- CI `JUCE_VERSION: '8.0.9'` — `.github/workflows/build-and-release.yml:39`
- Vendored overrides copied over the vanilla JUCE download — `build-and-release.yml:102` (macOS) / `:451` (Windows), grep-gated at `:104-105` / `:453-454`
- O-Texture ANIRA `GIT_TAG v2.0.3` + hardcoded `set(ONNXRUNTIME_VERSION 1.19.2)` — `plugins/O-Texture/CMakeLists.txt:15,22`
- O-TextureForge `nanoflann GIT_TAG v1.6.2` (`:10`), `umappp GIT_TAG v3.2.0` from libscran/umappp (`:20`)

---

## JUCE Override Diff — note-expression patch re-base sizing

Two override files under `vendored/JUCE-overrides/`. Diffed local vendored copies and
upstream 8.0.9 vs 8.0.14. Diff metric = `diff -u` changed (+/-) lines minus the 2 header
lines; hunks = `@@` count.

### File 1 — `modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp`

| Diff | Changed lines | Hunks |
|------|---------------|-------|
| (a) upstream 8.0.9 → upstream 8.0.14 (pure upstream churn) | 325 | 32 |
| (b) local vendored override → upstream 8.0.14 (re-base effort) | 373 | 33 |

- Upstream line count: 8.0.9 = 4165, 8.0.14 = 4062 (file still lives at the same path in 8.0.14).
- The delta between (b) and (a) (≈48 lines / +1 hunk) is the JUCE-NE-PATCH insertion itself.
- **NE-PATCH anchor survival:** the vendored insertion sits inside the
  `#if JucePlugin_WantsMidiInput` → `if (isMidiInputBusEnabled && data.inputEvents != nullptr)`
  block, immediately before `MidiEventList::toMidiBuffer(...)`. In upstream 8.0.14 that exact
  anchor is intact:
  - `vst3client_8.0.14.cpp:3587` `processParameterChanges (*data.inputParameterChanges);`
  - `vst3client_8.0.14.cpp:3590` `if (isMidiInputBusEnabled && data.inputEvents != nullptr)`
  - `vst3client_8.0.14.cpp:3591` `MidiEventList::toMidiBuffer (midiBuffer, *data.inputEvents);`
  The `toMidiBuffer` signature is unchanged, and `getVST3ClientExtensions()` is still called
  in 6 places in the 8.0.14 wrapper. The insertion region is NOT inside a churned hunk.
- **Re-base difficulty: MODERATE.** The single NE insertion anchor survives verbatim, so the
  patch re-applies conceptually, but 32 hunks / 325 lines of surrounding upstream churn mean the
  vendored copy must be regenerated from fresh 8.0.14 sources and the NE block re-stitched — a
  `patch -p1` of the old diff will reject on drifted context. Re-vendor fresh, re-insert, re-diff.

### File 2 — `modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h`

| Diff | Changed lines | Hunks |
|------|---------------|-------|
| (a) upstream 8.0.9 → upstream 8.0.14 (pure churn) | 34 | 2 |
| (b) local vendored override → upstream 8.0.14 (re-base effort) | 68 | 3 |

- Upstream/vendored line counts: upstream 8.0.9 = 273, vendored (8.0.9 + NE-PATCH) = 307, upstream 8.0.14 = 305.
- **STRUCTURAL SURPRISE — the file MOVED in 8.0.14.** At tag 8.0.14 the old path
  `modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h` returns **HTTP 404**.
  The file relocated to a **new module**:
  `modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h` (305 lines),
  and gained a companion `.../juce_VST3ClientExtensions.cpp` (53 lines) that did not exist as a
  separate TU at 8.0.9. Confirmed via the 8.0.14 git tree API.
- **NE-PATCH anchor survival:** the vendored patch adds a `struct Vst3RawEvent` + a virtual hook
  inside `struct VST3ClientExtensions`. That struct still exists in 8.0.14 at the new path:
  - `vst3ext_8.0.14.h:59` `struct VST3ClientExtensions`
  - `vst3ext_8.0.14.h:61` `virtual ~VST3ClientExtensions() = default;`
  So the insertion still fits, but at a **different file path in a different module**.
- **Re-base difficulty: HIGH** — driven by the relocation, not by diff size. The CI copy step
  `cp -R vendored/JUCE-overrides/modules/. JUCE/modules/` copies into the OLD
  `juce_audio_processors/utilities/` path, which no longer exists in 8.0.14 → the override would
  land in a stale directory, the real 8.0.14 header would remain un-patched, and the grep-gate at
  `build-and-release.yml:104` (`grep -q "JUCE-NE-PATCH" .../juce_VST3ClientExtensions.h`) would
  **fail on file-not-found**. Re-base MUST move the vendored override to the new
  `juce_audio_processors_headless/utilities/` path, evaluate whether the new companion `.cpp`
  needs patching too, and update both the CI copy target and the two grep-gate paths.

---

## JUCE BREAKING_CHANGES — 8.0.10 → 8.0.14

Fetched `BREAKING_CHANGES.md` at tag 8.0.14. The file has version headers for **8.0.13** and
**8.0.11** in the 8.0.10–8.0.14 window (no separate 8.0.10 / 8.0.12 / 8.0.14 sections → no
breaking changes recorded for those point releases). Entries quoted below.

### Version 8.0.13

1. **createEditor() made private** (scout item 1, CONFIRMED):
   > "made AudioProcessor::createEditor() private. It also incorrectly renamed
   > createEditorIfNeeded() to createEditorIfNecessary(). The old naming has now be reinstated."
   > Possible Issues: "Code that calls createEditor() directly will fail to compile."
   > Workaround: "call AudioProcessor::createEditorAndMakeActive()."
   Plugin `createEditor()` **overrides are unaffected** — only direct *calls* break.

2. **AlertWindow::show() return value changed** (scout item 2, CONFIRMED):
   > "The value returned by AlertWindow::show() has been changed so that it is consistent
   > between native and non-native windows."

3. **AudioPluginInstance::getPlatformSpecificData() removed** (scout item 3, CONFIRMED):
   > "AudioPluginInstance::getPlatformSpecificData() has been removed." Hosting-side.
   Replaced by `getVSTClient()/getVST3Client()/getAudioUnitClient()/getARAClient()`.

4. **VSTPluginFormatHeadless signature changes + removals** (scout item 4, CONFIRMED — two entries):
   > New signatures: "VSTPluginFormatHeadless::loadFromFXBFile() / setChunkData() / setExtraFunctions()."
   > Removed: "getVSTXML() / loadFromFXBFile() / saveToFXBFile() / getChunkData() / setChunkData() /
   > setExtraFunctions() / dispatcher()" and "VST3PluginFormatHeadless::setStateFromVSTPresetFile()."
   Hosting-side.

Additional 8.0.13 breaking changes NOT in scout notes (captured for completeness):
5. **ExtensionsVisitor type removed** — > "The ExtensionsVisitor type has been removed." Hosting-side.
6. **Typeface / Font member removals** — `getStringWidth()`, `getGlyphPositions()`,
   `getEdgeTableForGlyph()`, `applyVerticalHintingTransform()`, `Font::getStringWidth[Float]()`
   removed; `getOutlineForGlyph()/getGlyphBounds()/getLayersForGlyph()` drop the
   `TypefaceMetricsKind` arg.
7. **Displays deprecations** — `logicalToPhysical`/`physicalToLogical`/`getDisplayForPoint`
   `Point<int>` overloads deprecated; `totalArea`/`userArea`/`topLeftPhysical` deprecated
   (use `logicalBounds`/`userBounds`/`physicalBounds`). Deprecation warnings, not hard breaks.
8. **ARA** — new `ARAConfigurationType` member; ARA SDK bumped to 2.3.0 (ARA plugins only).

### Version 8.0.11

9. **var deep equality** — > "var::equals(), var::operator==(), and var::operator!=() will now
   carry out a deep equality check when comparing two stored DynamicObjects." Behavioural change.
10. **JUCE_ASIO bundled sources** — enabling `JUCE_ASIO` now defaults to bundled ASIO sources;
    set `JUCE_ASIO_USE_EXTERNAL_SDK` to override. Windows-audio-host concern only.

---

## Repo Exposure Grep — which breaking changes actually touch this repo

Greps over `plugins/ modules/ scripts/` (source only; excludes node_modules where noted).

| Breaking change | Grep | Hits | Verdict |
|-----------------|------|------|---------|
| 1. createEditor() private | direct-call `(\.|->)createEditor\s*\(` | **0** | No exposure — no direct calls |
| 1. createEditor() overrides | `createEditor()  override/;` decls | 61 files | Unaffected (overrides don't break) |
| 2. AlertWindow::show() | `AlertWindow::show` + `.show()` alert | **0** in source | No exposure (2 hits are `AlertWindow::showAsync` in a `.planning/…/04-RESEARCH.md` doc, not source; `showAsync` return is unaffected) |
| 3. getPlatformSpecificData() | `getPlatformSpecificData` | **0** | No exposure (hosting-side API) |
| 4. VSTPluginFormatHeadless | `VST(3)?PluginFormatHeadless` | **0** | No exposure (hosting-side API) |
| 5. ExtensionsVisitor | `ExtensionsVisitor` | **0** | No exposure |

Conclusion: **zero source exposure to all four scout-listed breaking changes** and to
ExtensionsVisitor. The 61 `createEditor()` matches are plugin *overrides*, which the change
explicitly leaves working. No 8.0.13 hard-break requires a plugin code edit. The var deep-equality
(8.0.11) behavioural change is a latent risk only if any code relies on address-identity var
comparison — not grepped exhaustively but no known preset/state code depends on it.

---

## ANIRA → ONNX Runtime version coupling

O-Texture uses ANIRA **only** for its per-platform ONNX Runtime download logic (comment at
`plugins/O-Texture/CMakeLists.txt:6-8`: "anira itself is NOT linked or embedded"). The plugin
**hardcodes** the ORT version string at `CMakeLists.txt:22` `set(ONNXRUNTIME_VERSION 1.19.2)`,
reused at `:24` (lib path) and `:168` (embedded dylib filename `libonnxruntime.${VER}.dylib`).

ORT version each ANIRA tag provisions (macOS/Windows path; source file per tag):

| ANIRA tag | ORT version | Source (fetched) |
|-----------|-------------|------------------|
| v2.0.3 (current pin) | **1.19.2** | `cmake/SetupOnnxRuntime.cmake` `set(ONNXRUNTIME_VERSION 1.19.2)` |
| v2.1.0 | **1.19.2** | `cmake/SetupOnnxRuntime.cmake` `set(LIBONNXRUNTIME_VERSION 1.19.2)` |
| v2.2.0 | **1.26.0** | `cmake/AniraBackends.cmake` `_anira_engine_version(onnxruntime) → "1.26.0"` (line 72) |
| v2.2.1 (upstream latest) | **1.26.0** | `cmake/AniraBackends.cmake` `_anira_engine_version(onnxruntime) → "1.26.0"` (line 72) |

Notes:
- The ORT provisioning file was **renamed** between v2.1.0 (`SetupOnnxRuntime.cmake`) and
  v2.2.0 (`AniraBackends.cmake`) — the old filename 404s at v2.2.0/v2.2.1.
- The `1.19.2` seen at `AniraBackends.cmake:403` in v2.2.x is a **Linux-armv7l-only** special
  case, irrelevant to O-Texture's macOS/Windows targets; the general default (line 72) is 1.26.0.
- Scout said "upstream ORT ~1.27.1" — the **coupled** version for the latest ANIRA tag is 1.26.0,
  not 1.27.1. Use the coupled number.
- **Impact:** bumping ANIRA to **v2.1.0 needs no version-string change** (still 1.19.2 → pure
  drop-in for ORT provisioning). Bumping to **v2.2.x requires updating the hardcoded `1.19.2`
  string to `1.26.0` at O-Texture `CMakeLists.txt:22` (and it flows to `:24` and `:168`)**, or the
  dylib-embed + `@rpath` post-build step (MEMORY: ANIRA shared-lib distribution) will reference a
  non-existent `libonnxruntime.1.19.2.dylib` and fail.

---

## Header-only deps — nanoflann + umappp compat

Usage is isolated to 4 files: `Source/dsp/KDTreeSearch.{h,cpp}`, `Source/dsp/UMAPProjection.{h,cpp}`.

### nanoflann v1.6.2 → v1.7.1 (latest; scout's "v1.10.0" tag does not exist upstream — jlblancoc/nanoflann tops out at 1.7.1)

API surface actually used (from KDTreeSearch): `nanoflann::KDTreeSingleIndexAdaptor`,
`KDTreeSingleIndexAdaptorParams`, `L2_Simple_Adaptor`, `KNNResultSet<float,uint32_t>`,
`SearchParameters`. Changelog entries above the 1.6.2 pin:
- 1.6.3 — cmake_required_version → 3.10; clang-format → 14 (build-tooling only).
- 1.7.0 — `ResultSet::worstDist()` return-value semantics clarified/changed (only affects custom
  ResultSet implementations; O-TextureForge uses the stock `KNNResultSet`).
- 1.7.1 — `worstDist()` negative-index static-analysis fix.
None touch the stock `KDTreeSingleIndexAdaptor` / `KNNResultSet` API used here.
**Verdict: DROP-IN** (upgrade target is v1.7.1, not the scout's non-existent v1.10.0).

### umappp v3.2.0 → v3.3.2 (libscran/umappp; tag confirmed via releases API)

API surface actually used (from UMAPProjection.cpp): `umappp::Options{num_neighbors,
initialize_method=InitializeMethod::SPECTRAL}`, `umappp::initialize<int,double>(...)`,
`Status::num_epochs()`, `Status::run(...)`. Diff of `include/umappp/Options.hpp` v3.2.0 vs v3.3.2:
- Public field **removed/renamed**: `int parallel_optimization` → replaced by `int num_threads_spectral`
  and `int num_threads_optimize` (threading model refactor).
- The `initialize<>()` / `Status::num_epochs()` / `Status::run()` signatures are unchanged.
- O-TextureForge sets **only** `num_neighbors` and `initialize_method` — it does **not** reference
  `parallel_optimization`, so the removed field does not break this code.
**Verdict: DROP-IN for O-TextureForge's usage** (note the `Options` field rename exists but is unused here).

---

## Up-to-date and out-of-scope

- **pluginval 1.0.4** == upstream latest — **no action** (scout-confirmed; installed == latest).
- **Toolchain** (CMake 4.2.1, Xcode 26.3, ninja 1.13.2) — working; **out of scope** for this review.

## Curl / fetch failures encountered (none silently omitted)

- `juce_VST3ClientExtensions.h` @ 8.0.14 old path → **404**; resolved by locating the file at the
  new `juce_audio_processors_headless/utilities/` path via the git-tree API (recorded above as a finding).
- ANIRA `SetupOnnxRuntime.cmake` @ v2.2.0/v2.2.1 → **404**; resolved via `AniraBackends.cmake` in
  those tags (recorded above).
- nanoflann `CHANGELOG.md` @ `master` → 404; the tagged `v1.7.1/CHANGELOG.md` (HTTP 200) was used.
  Scout's `v1.10.0` tag does not exist on jlblancoc/nanoflann; latest is v1.7.1.
