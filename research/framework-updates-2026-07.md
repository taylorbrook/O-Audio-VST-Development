# Framework Updates — Review & Integration Plan (2026-07)

> **PRECONDITION BANNER — review + plan only; NO upgrade is performed this session.**
> The working tree is dirty (extensive uncommitted changes across many plugins) and `HEAD`
> is 5 commits ahead of `origin/main`. **No JUCE / CMake / `vendored/` / plugin-build change may
> be made** until the tree is clean, HEAD is pushed, and the developer green-lights execution on a
> dedicated branch. Every number below is grounded in the companion evidence file
> `.planning/quick/260719-hn4-check-juce-and-framework-updates-review-/260719-hn4-EVIDENCE.md`
> (upstream fetched at pinned tags on 2026-07-19, read-only).

## Summary

| Dependency | Pinned | Upstream latest | Delta | Risk | Recommended action |
|------------|--------|-----------------|-------|------|--------------------|
| JUCE | 8.0.9 | 8.0.14 | +5 point releases (8.0.10–8.0.14) | **HIGH** (note-expression patch re-base) | stage + verify (dedicated branch) |
| ANIRA (+ ONNX Runtime) | v2.0.3 / ORT 1.19.2 | v2.2.1 / ORT 1.26.0 | +2 minor / ORT +7 minor | MEDIUM | stage + verify (or defer to v2.1.0 for a zero-string-change bump) |
| nanoflann | v1.6.2 | **1.10.1** (tags ≥v1.8.0 drop the `v` prefix in releases) | +7 patch/minor | LOW | upgrade now / defer (optional) |
| umappp | v3.2.0 | v3.3.2 | +2 patch/minor | LOW | upgrade now / defer (optional) |
| pluginval | 1.0.4 | 1.0.4 | none | — | up-to-date (no action) |

Action legend: *upgrade now* = safe drop-in · *stage + verify* = do on a branch behind the full gate
battery · *defer* = low value, revisit later · *up-to-date* = nothing to do.

## Per-Dependency Review

### JUCE 8.0.9 → 8.0.14
Touches **all 37 plugins** (core framework). Headline changes across 8.0.10–8.0.14:
- **8.0.13**: `createEditor()` made private (direct calls only); `AlertWindow::show()` return-value
  normalized; hosting-side removals (`getPlatformSpecificData`, `ExtensionsVisitor`,
  `VSTPluginFormatHeadless` API); `Typeface`/`Font` deprecated-member removals; `Displays`
  `Point<int>` deprecations; ARA SDK → 2.3.0. Also general gui/painting perf and rendering work.
- **8.0.11**: `var` deep-equality for stored `DynamicObject`s; `JUCE_ASIO` now defaults to bundled
  sources; Windows minimum target and rendering/resizing fixes; MIDI fixes; reduced build-tree depth.

Relevance to this suite (WebView-heavy UIs, macOS-primary + Windows-CI): the Typeface/Font and
Displays changes are drawing-API churn that a WebView-rendered UI largely sidesteps; the
`var` deep-equality change is the only behavioural item that *could* touch preset/state code, but
no address-identity `var` comparison is known here. The dominant concern is not any public-API break
— it is the **re-base of the vendored note-expression patch** (see Risk Assessment).

### ANIRA v2.0.3 → v2.2.1 (+ ONNX Runtime)
O-Texture uses ANIRA **solely for its per-platform ONNX Runtime download logic** — anira is *not*
linked as an inference engine (`plugins/O-Texture/CMakeLists.txt:6-8`; inference runs on our own
thread via the raw ORT C++ API). The coupled ORT version per tag (EVIDENCE):

| ANIRA tag | ORT provisioned |
|-----------|-----------------|
| v2.0.3 (pin) | 1.19.2 |
| v2.1.0 | 1.19.2 |
| v2.2.0 / v2.2.1 | 1.26.0 |

O-Texture **hardcodes** `set(ONNXRUNTIME_VERSION 1.19.2)` (`CMakeLists.txt:22`), reused for the lib
path (`:24`) and the embedded dylib filename (`:168`). A bump to **v2.1.0 is a zero-string-change
drop-in** (ORT stays 1.19.2). A bump to **v2.2.x requires editing that hardcoded string to `1.26.0`**
or the dylib-embed + `@rpath` post-build step breaks on a missing `libonnxruntime.1.19.2.dylib`.
(Note: scout's "ORT 1.27.1" is the upstream ORT project version; the *coupled* number for the
latest ANIRA tag is **1.26.0**.)

### nanoflann v1.6.2 → 1.10.1
Header-only KD-tree. Usage is confined to `Source/dsp/KDTreeSearch.{h,cpp}` using the stock
`KDTreeSingleIndexAdaptor` / `KNNResultSet` / `SearchParameters` API. Changelog through 1.7.1 is
build-tooling (1.6.3) plus `ResultSet::worstDist()` semantics (1.7.0/1.7.1) that only affect *custom*
ResultSets — not the stock adaptor used here. Releases v1.8.0–1.10.1 exist as tags (verified via
`git ls-remote`; the GitHub releases list drops the `v` prefix from 1.10.1, which can hide them from
release-API queries) — review their changelogs for the stock-adaptor API before pinning, or take the
conservative v1.7.1 step. **Verdict: drop-in through v1.7.1; 1.8.0+ needs a changelog pass.**

### umappp v3.2.0 → v3.3.2
Header-only UMAP. Usage confined to `Source/dsp/UMAPProjection.{h,cpp}` using
`umappp::Options{num_neighbors, initialize_method}`, `umappp::initialize<int,double>()`,
`Status::num_epochs()`, `Status::run()`. The `Options.hpp` diff shows a threading-model refactor
(`parallel_optimization` removed → `num_threads_spectral` + `num_threads_optimize`), but
O-TextureForge does not reference the removed field, and `initialize()/num_epochs()/run()` are
unchanged. **Verdict: drop-in for this code** (the `Options` field rename exists but is unused here).

### pluginval 1.0.4
Installed version equals upstream latest. **No action.**

## JUCE Breaking-Change Review

Every scout-listed break was grep-proven against `plugins/ modules/ scripts/` (EVIDENCE §Repo Exposure):

| Breaking change (8.0.13) | Repo grep | Hits | Verdict |
|--------------------------|-----------|------|---------|
| `createEditor()` made private | direct-call pattern `(\.|->)createEditor\s*\(` | **0** | **No exposure.** Direct calls were grepped and none exist; the 61 files with `createEditor()` are plugin *overrides*, which the change explicitly leaves working. |
| `AlertWindow::show()` return change | `AlertWindow::show` / `.show()` alert | **0** in source | **No exposure.** The only 2 hits are `AlertWindow::showAsync` in a `.planning/…/04-RESEARCH.md` doc (not compiled; `showAsync` return unaffected). |
| `getPlatformSpecificData()` removed | `getPlatformSpecificData` | **0** | **No exposure** — hosting-side API, unused. |
| `VSTPluginFormatHeadless` API changes/removals | `VST(3)?PluginFormatHeadless` | **0** | **No exposure** — hosting-side API, unused. |

Bonus: `ExtensionsVisitor` (also removed in 8.0.13) → **0 hits**. Net result: **no 8.0.13/8.0.11
public-API break requires a plugin code edit.** The upgrade risk lives entirely in the vendored patch.

## Risk Assessment

Ranked highest-first.

### 1. HIGH — Note-expression vendored-patch re-base (JUCE-NE-PATCH → 8.0.14)
This is the headline risk. Two files under `vendored/JUCE-overrides/` carry the patch and are copied
over the vanilla JUCE download in CI (`build-and-release.yml:102/451`), grep-gated at `:104-105/453-454`.

**`juce_audio_plugin_client_VST3.cpp`** — re-base rating **MODERATE**:
- Upstream churn 8.0.9→8.0.14: **325 changed lines / 32 hunks**. Vendored→8.0.14: **373 lines / 33 hunks**.
- The NE insertion anchor (`if (isMidiInputBusEnabled && data.inputEvents != nullptr)` immediately
  before `MidiEventList::toMidiBuffer`) **survives verbatim** in 8.0.14 (lines 3590–3591) and is not
  inside a churned hunk; `toMidiBuffer`'s signature is unchanged. Re-apply is conceptually clean, but
  a `patch -p1` of the old diff will reject on drifted context — re-vendor fresh and re-stitch the block.

**`juce_VST3ClientExtensions.h`** — re-base rating **HIGH** (structural, not size):
- Diff is small (upstream 34 lines/2 hunks; vendored→8.0.14 68 lines/3 hunks) and the
  `struct VST3ClientExtensions` anchor survives — **but the file MOVED**. In 8.0.14 the old path
  `modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h` returns **404**; it relocated
  to a new module `modules/juce_audio_processors_headless/utilities/` and gained a companion `.cpp`.
- Consequence: the CI `cp -R vendored/JUCE-overrides/modules/. JUCE/modules/` would copy into a stale
  directory, the real 8.0.14 header would stay un-patched, and the grep-gate would **fail on
  file-not-found**. The re-base MUST relocate the vendored override to the new module path, decide
  whether the new `.cpp` also needs patching, and update the CI copy target + both grep-gate paths.

**Re-base procedure (concrete):**
1. On a dedicated branch, download pristine JUCE 8.0.14 and copy the two (relocated) upstream files.
2. Re-apply the NE-PATCH: `.cpp` block goes at the surviving `toMidiBuffer` anchor; the `.h`
   `struct Vst3RawEvent` + virtual hook goes into `struct VST3ClientExtensions` at the **new**
   `juce_audio_processors_headless/utilities/` path.
3. Update `vendored/JUCE-overrides/` to mirror the new module path; update the CI copy step and the
   two `grep -q "JUCE-NE-PATCH"` gate paths in `build-and-release.yml`.
4. Regenerate the patch source and re-run `scripts/apply-juce-patches.sh`; confirm the JUCE-NE-PATCH
   marker survives the CI grep in both files.
5. **Housekeeping:** rename `scripts/juce-patches/note-expression-juce-8.0.4.patch` to reflect the
   true base (it is currently an 8.0.9 diff mis-named 8.0.4), and regenerate it against 8.0.14 once
   the re-base lands.

### 2. MEDIUM — ANIRA/ORT version-string coupling (O-Texture only)
A v2.2.x bump changes the embedded ORT dylib version (1.19.2 → 1.26.0) and requires editing the
hardcoded string + validating the `@rpath`/`install_name_tool` post-build step. A v2.1.0 bump avoids
the string change entirely. Scoped to one plugin.

### 3. LOW — header-only bumps (nanoflann/umappp)
Drop-in for O-TextureForge's actual API usage; only risk is a fresh `FetchContent` compile surfacing
a transitive-dep quirk (umappp pulls Eigen/knncolle). Verify by building O-TextureForge only.

### 4. LOW — latent `var` deep-equality (8.0.11)
Behavioural, suite-wide in principle, but no address-identity `var` comparison is known in preset/state
code. Watch preset round-trips during the JUCE verification pass.

## Recommended Upgrade Order

1. **JUCE 8.0.9 → 8.0.14 first** — it underpins all 37 plugins. Do it on a **dedicated branch**;
   bump the **local `/Users/taylorbrook/JUCE` pin and CI `JUCE_VERSION` together** (they must match).
   Land the note-expression patch re-base (incl. the new headless-module path) before touching any
   plugin. Gate with the full battery below.
2. **ANIRA/ORT for O-Texture only** — staged after JUCE is green. Prefer **v2.1.0** for a
   zero-string-change ORT-1.19.2 bump; choose **v2.2.1** only if 1.26.0 is wanted, and then update the
   hardcoded `ONNXRUNTIME_VERSION` + re-validate the dylib-embed/rpath step.
3. **nanoflann (v1.7.1 conservative, 1.10.1 after changelog pass) / umappp v3.3.2** — optional low-risk bumps; may ride along with the O-Texture
   work or be deferred. Verify by building O-TextureForge alone.
4. **pluginval** — no action (at latest).

## Verification Strategy

Run this gate battery after the JUCE bump (and re-run the relevant subset after each dep bump):
- **Full build matrix** — macOS VST3 + AU locally; Windows VST3 via CI `build-and-release.yml`.
- **AU-link gate** — `auval` + `scripts/verify-au-link.sh` per plugin (the gate that caught the
  Steinberg-symbol leak in Phase 23; canonical D-08 path).
- **pluginval** across the suite.
- **Offline render harnesses** — compiled under `JUCE_WEB_BROWSER=0`; guard `createEditor` with
  `#if JUCE_WEB_BROWSER` per the known harness-vs-WebView pitfall so the harness stays buildable.
- **Dorico 3-point microtonal smoke test** (the true note-expression acceptance gate that proves the
  patch re-base preserved microtonal playback): quarter-sharp C4 ≈ 269 Hz; no attack zipper;
  polyphonic isolation (detuned note isolated, others play 12-TET).
- **DAW smoke tests** (Logic / Ableton) with the mandatory cache-clear per `CLAUDE.md`.

## Rollback Strategy

The branch-based approach makes the primary rollback a **branch discard**. Per-item back-outs:
- **JUCE** — revert `JUCE_VERSION` in `build-and-release.yml` and the local pin; restore the prior
  `vendored/JUCE-overrides/` files and the prior `scripts/juce-patches/*.patch` from git; `git revert`
  or discard the branch.
- **ANIRA/ORT** — restore `GIT_TAG v2.0.3` and the `ONNXRUNTIME_VERSION 1.19.2` string (and the `:24`
  / `:168` references) in `plugins/O-Texture/CMakeLists.txt`.
- **Header-only** — revert the `GIT_TAG` pin (v1.6.2 / v3.2.0) in `plugins/O-TextureForge/CMakeLists.txt`.

Because nothing here is installed system-wide, rollback is purely a source revert + rebuild + the
CLAUDE.md cache-clear/re-install of the prior binaries.

## Deferred and Up-to-date

- **pluginval 1.0.4** — up to date; no action.
- **Toolchain** (CMake 4.2.1, Xcode 26.3, ninja 1.13.2) — working; out of scope.
- **nanoflann / umappp** — safe to **defer**: low functional value versus the JUCE work, and both are
  drop-in whenever convenient. Recommend bundling them opportunistically with the O-Texture ANIRA
  work rather than as a standalone upgrade.
- **ANIRA** — if the goal is only to stay current with minimal churn, **defer past v2.1.0** (or take
  v2.1.0 for the zero-string ORT bump) and hold v2.2.x until there's a reason to move ORT to 1.26.0.
